#include <linux/kernel.h>
#include <linux/usb/otg.h>
#include <linux/usb/hcd.h>
#include <linux/usb/ch11.h>

#include "../host/xhci.h"

#include "../core/otg_whitelist.h"

#include "bmusb_drd.h"
#include "otg_fsm.h"
#include "io.h"

static int bmusb_statemachine(struct otg_fsm *fsm);

/**
 * bmusb_get_port_status - reads status of HCD port.
 * @fsm: pointer to otg_fsm structure
 * @port: number of port that status will be read of
 *
 * Returns status of a port.
 */
static int bmusb_get_port_status(struct otg_fsm *fsm, unsigned int port)
{
	unsigned int buf[16];

	fsm->otg->primary_hcd.hcd->driver->hub_control(
		fsm->otg->primary_hcd.hcd,
		GetPortStatus,
		0,
		port,
		(unsigned char *)buf,
		4);
	return buf[0];
}

/**
 * bmusb_check_port_connect - checks if a device is connected to port of HCD.
 * @fsm: pointer to otg_fsm structure
 * @port: number of port that connection status will be checked
 *
 * Returns 1 if there is a device connected to the port, 0 if there is
 * no device connected to the port.
 */
static int bmusb_check_port_connect(struct otg_fsm *fsm, unsigned int port)
{
	int ret = 0;

	if (fsm != NULL)
		ret = bmusb_get_port_status(fsm, port) & PORT_CONNECT;
	return ret;
}

/**
 * bmusb_suspend_port - suspends port of HCD.
 * @fsm: pointer to otg_fsm structure
 * @port: number of port that will be suspended
 *
 * This function does not return any value.
 */
static void bmusb_suspend_port(struct otg_fsm *fsm, unsigned int port)
{
	unsigned int buf[16];

	fsm->otg->primary_hcd.hcd->driver->hub_control(
		fsm->otg->primary_hcd.hcd,
		SetPortFeature,
		USB_PORT_FEAT_SUSPEND,
		port,
		(unsigned char *)buf,
		4);
}

/**
 * is_test_device - checks if device on the bus is a test device.
 * @fsm: pointer to otg_fsm structure
 *
 * Returns 1 if connected device is a test device, 0 if connected
 * device is not a test device, -1 if there is no device connected
 */
static int is_test_device(struct otg_fsm *fsm)
{
	struct usb_otg *otgd = container_of(fsm, struct usb_otg, fsm);
	struct usb_device *udev;

	udev = usb_hub_find_child(otgd->primary_hcd.hcd->self.root_hub, 1);
	if (!udev) {
		bmusb_dbg(otgd->dev,
			"no usb dev connected, can't check if device is test device\n");
		return -1;
	}
	return is_targeted(udev);
}


/**
 * bmusb_set_protocol - switches mode of operation between HOST and DEVICE.
 * @fsm: pointer to otg_fsm structure
 * @protocol: new mode of operation
 *
 * Returns 0 if change of mode was successful.
 */
static int bmusb_set_protocol(struct otg_fsm *fsm, int protocol)
{
	struct usb_otg *otgd = container_of(fsm, struct usb_otg, fsm);
	int ret = 0;

	if (fsm->protocol != protocol) {
		bmusb_dbg(otgd->dev, "otg: changing role fsm->protocol= %d; new protocol= %d\n",
			fsm->protocol, protocol);
		/* stop old protocol */
		if (fsm->protocol == PROTO_HOST)
			ret = otg_start_host(fsm, 0);
		else if (fsm->protocol == PROTO_GADGET)
			ret = otg_start_gadget(fsm, 0);
		if (ret)
			return ret;

		/* start new protocol */
		if (protocol == PROTO_HOST)
			ret = otg_start_host(fsm, 1);
		else if (protocol == PROTO_GADGET)
			ret = otg_start_gadget(fsm, 1);
		if (ret)
			return ret;

		fsm->protocol = protocol;
		return 0;
	}

	return 0;
}

/**
 * otg_hnp_polling_work - finds polls connected device for Host Request Flag.
 * @work: pointer to structure of work_struct
 *
 * This function does not return any value.
 */
void otg_hnp_polling_work(struct work_struct *work)
{
	struct otg_fsm *fsm = container_of(to_delayed_work(work),
				struct otg_fsm, hnp_polling_work);
	struct usb_otg *otgd = container_of(fsm, struct usb_otg, fsm);
	struct usb_device *udev;
	enum usb_otg_state state = fsm->otg->state;
	u8 flag;
	int retval;

	if (state != OTG_STATE_A_HOST)
		return;
	if (!otgd->primary_hcd.hcd->self.root_hub) {
		schedule_delayed_work(&fsm->hnp_polling_work,
#ifdef USB_SIM_SPEED_UP
				      usecs_to_jiffies(T_HOST_REQ_POLL)
#else
				      msecs_to_jiffies(T_HOST_REQ_POLL)
#endif
				      );
		return;
	}

	udev = usb_hub_find_child(otgd->primary_hcd.hcd->self.root_hub, 1);
	if (!udev) {
		schedule_delayed_work(&fsm->hnp_polling_work,
#ifdef USB_SIM_SPEED_UP
				      usecs_to_jiffies(T_HOST_REQ_POLL)
#else
				      msecs_to_jiffies(T_HOST_REQ_POLL)
#endif
				      );
		return;
	}

	*(fsm->host_req_flag) = 0;
	/* Get host request flag from connected USB device */
	retval = usb_control_msg(udev,
				 usb_rcvctrlpipe(udev, 0),
				 USB_REQ_GET_STATUS,
				 USB_DIR_IN | USB_RECIP_DEVICE,
				 0,
				 OTG_STS_SELECTOR,
				 fsm->host_req_flag,
				 1,
				 USB_CTRL_GET_TIMEOUT);
	if (retval != 1) {
		//dev_err(&udev->dev, "Get one byte OTG status failed\n");
		schedule_delayed_work(&fsm->hnp_polling_work,
#ifdef USB_SIM_SPEED_UP
				      usecs_to_jiffies(T_HOST_REQ_POLL)
#else
				      msecs_to_jiffies(T_HOST_REQ_POLL)
#endif
				      );

		return;
	}

	flag = *fsm->host_req_flag;
	if (flag == 0) {
		/* Continue HNP polling */
		schedule_delayed_work(&fsm->hnp_polling_work,
#ifdef USB_SIM_SPEED_UP
				      usecs_to_jiffies(T_HOST_REQ_POLL)
#else
				      msecs_to_jiffies(T_HOST_REQ_POLL)
#endif
				      );

		return;
	} else if (flag != HOST_REQUEST_FLAG) {
		dev_err(&udev->dev, "host request flag %d is invalid\n", flag);
		return;
	}

	/* Host request flag is set */
	if (state == OTG_STATE_A_HOST) {
		/* Set b_hnp_enable */
		if (!fsm->otg->host->b_hnp_enable) {
			retval = usb_control_msg(udev,
						 usb_sndctrlpipe(udev, 0),
						 USB_REQ_SET_FEATURE, 0,
						 USB_DEVICE_B_HNP_ENABLE,
						 0, NULL, 0,
						 USB_CTRL_SET_TIMEOUT);
			if (retval >= 0)
				fsm->otg->host->b_hnp_enable = 1;
		}
	}

	bmusb_statemachine(fsm);
}

/**
 * otg_start_hnp_polling - starts HNP polling of device for Host Request Flag.
 * @fsm: pointer to otg_fsm structure
 *
 * This function does not return any value.
 */
static void otg_start_hnp_polling(struct otg_fsm *fsm)
{
	if (!fsm->host_req_flag)
		return;

	schedule_delayed_work(&fsm->hnp_polling_work,
#ifdef USB_SIM_SPEED_UP
				      usecs_to_jiffies(T_HOST_REQ_POLL)
#else
				      msecs_to_jiffies(T_HOST_REQ_POLL)
#endif
				      );

}


#if !IS_ENABLED(CONFIG_USB_BM_A_BIDL_ADIS_HW_TMR)
/**
 * a_bidl_adis_timeout - controls a_bidl_adis timer
 * @work: pointer to structure of work_struct
 *
 * This function does not return any value.
 */
static void a_bidl_adis_timeout(struct work_struct *work)
{
	struct otg_fsm *fsm = container_of(to_delayed_work(work),
					   struct a_bidl_adis_worker_data,
					   a_bidl_adis_work)->fsm;

	fsm->a_bidl_adis_tmout = 1;
	usb_otg_sync_inputs(fsm);
}
#endif


/**
 * clean_vars_of_a - clears variables of USB A-side state machine.
 * @fsm: pointer to otg_fsm structure
 * @retval:  0 upon success
 *
 * Returns -1 if fsm pointer is not valid.
 */
int clean_vars_of_a(struct otg_fsm *fsm)
{
	if (!fsm)
		return 1;
	fsm->a_bidl_adis_tmout = 0;
	fsm->otg->host->b_hnp_enable = 0;
	return 0;
}

/**
 * clean_vars_of_b - clears variables of USB B-side state machine.
 * @fsm: pointer to otg_fsm structure
 *
 * Returns 0 upon success, -1 if fsm pointer is not valid.
 */
int clean_vars_of_b(struct otg_fsm *fsm)
{
	if (!fsm)
		return 1;
	fsm->otg->gadget->host_request_flag = 0;
	fsm->otg->gadget->b_hnp_enable = 0;
	fsm->b_ase0_brst_tmout = 0;
	fsm->b_aidl_bdis_tmout = 0;
	return 0;
}

/**
 * bmusb_set_state - is called on transition of DRD state machine.
 * @fsm: pointer to otg_fsm structure
 * @new_state: destination state of transition
 *
 * This function does not return any value.
 */
void bmusb_set_state(struct otg_fsm *fsm, enum usb_otg_state new_state)
{
	struct usb_otg *otgd = container_of(fsm, struct usb_otg, fsm);
	struct device *dev = usb_otg_fsm_to_dev(fsm);
	struct bmusb_dev *bmusb = dev_get_drvdata(dev);
	u32 reg;
	u32 reg_value = 0;
	char ready = 0;

	bmusb_global_synchro_timer_stop(bmusb);

	if (fsm->otg->state == new_state)
		return;

	fsm->state_changed = 1;
	bmusb_dbg(otgd->dev, "otg: set state: %s\n",
		usb_otg_state_string(new_state));
	switch (new_state) {
	case OTG_STATE_A_IDLE:
		clean_vars_of_a(fsm);
		bmusb_set_protocol(fsm, PROTO_UNDEF);
		bmusb_writel(&bmusb->regs->OTGCMD,
			OTGCMD_TYPE__OTG_EN__MASK
			| OTGCMD_TYPE__A_DEV_EN__MASK);
		reg = bmusb_readl(&bmusb->regs->OTGSTS);
		/* enable SRP detection interrupt */
		if (bmusb->otg_config.otg_caps.srp_support)
			bmusb_otg_enable_irq(bmusb,
				OTGIEN_TYPE__SRP_DET_INT_EN__MASK);
		break;
	case OTG_STATE_B_IDLE:
		clean_vars_of_b(fsm);
		bmusb_set_protocol(fsm, PROTO_UNDEF);
		bmusb_writel(&bmusb->regs->OTGCMD,
			OTGCMD_TYPE__OTG_EN__MASK
			| OTGCMD_TYPE__A_DEV_DIS__MASK);
		break;
	case OTG_STATE_B_PERIPHERAL:
		bmusb_set_protocol(fsm, PROTO_GADGET);
		usb_otg_sync_inputs(fsm);
		break;
	case OTG_STATE_A_HOST:
		clean_vars_of_a(fsm);
		bmusb_set_protocol(fsm, PROTO_HOST);
		otg_start_hnp_polling(fsm);
		break;
	case OTG_STATE_B_SRP_INIT:
		/* enable interrupts */
		bmusb_otg_enable_irq(bmusb,
				OTGIEN_TYPE__SRP_CMPL_INT_EN__MASK
				| OTGIEN_TYPE__SRP_FAIL_INT_EN__MASK);
		/* initiates SRP pulse */
		bmusb_writel(&bmusb->regs->OTGCMD, OTGCMD_TYPE__INIT_SRP__MASK);
		break;
	case OTG_STATE_B_WAIT_ACON:
		usb_otg_start_gadget(fsm, false);

		fsm->otg->gadget->suspended = 0;
		bmusb_writel(&bmusb->regs->OTGCMD,
				OTGCMD_TYPE__HOST_BUS_REQ__MASK |
				OTGCMD_TYPE__OTG_EN__MASK |
				OTGCMD_TYPE__A_DEV_DIS__MASK);
		bmusb_writel(&bmusb->regs->OTGCMD,
				OTGCMD_TYPE__SS_HOST_DISABLED_SET__MASK);
		bmusb_writel(&bmusb->regs->OTGCMD,
				OTGCMD_TYPE__A_SET_B_HNP_EN_CLR__MASK);

		bmusb_dbg(bmusb->dev, "Waiting for XHC...\n");
		do {
			reg = bmusb_readl(&bmusb->regs->OTGSTS);
			ready = OTGSTS_TYPE__XHC_READY__READ(reg);
			if (!ready) {
#ifdef USB_SIM_SPEED_UP
				udelay(100);
#else
				mdelay(100);
#endif
			}
		} while (!ready);

		usb_otg_start_host(fsm, true);
#ifndef USB_SIM_SPEED_UP
		bmusb_otg_enable_irq(bmusb,
				OTGIEN_TYPE__TB_ASE0_BRST_TMOUT_INT_EN__MASK);
#endif
		usb_otg_sync_inputs(fsm);
		break;

	case OTG_STATE_B_HOST:
		usb_otg_sync_inputs(fsm);
		break;

	case OTG_STATE_A_SUSPEND:
		bmusb_otg_enable_irq(bmusb,
				OTGIEN_TYPE__TA_AIDL_BDIS_TMOUT_INT_EN__MASK);
		usb_otg_sync_inputs(fsm);
		break;

	case OTG_STATE_A_PERIPHERAL:
		while (1) {
			reg = bmusb_readl(&bmusb->regs->OTGSTATE);
			reg_value = OTGSTATE_TYPE__HOST_OTG_STATE__READ(reg);
			if (reg_value == A_HOST_SUSPEND)
				break;
			udelay(100);
		}
		usb_otg_start_host(fsm, false);

		bmusb_writel(&bmusb->regs->OTGCMD,
			    OTGCMD_TYPE__OTG2_SWITCH_TO_PERIPH__MASK);
		bmusb_writel(&bmusb->regs->OTGCMD,
			    OTGCMD_TYPE__HOST_POWER_OFF__MASK);
		bmusb_writel(&bmusb->regs->OTGCMD,
			    OTGCMD_TYPE__SS_PERIPH_DISABLED_SET__MASK);
		bmusb_writel(&bmusb->regs->OTGCMD,
			    OTGCMD_TYPE__DEV_BUS_REQ__MASK |
			    OTGCMD_TYPE__DEV_DEVEN_FORCE_SET__MASK |
			    OTGCMD_TYPE__DEV_VBUS_DEB_SHORT_SET__MASK);

		bmusb_writel(&bmusb->regs->OTGCMD,
			    OTGCMD_TYPE__A_SET_B_HNP_EN_CLR__MASK);

		usb_otg_start_gadget(fsm, true);

#if IS_ENABLED(CONFIG_USB_BM_A_BIDL_ADIS_HW_TMR)
		bmusb_otg_enable_irq(bmusb,
				OTGIEN_TYPE__TA_BIDL_ADIS_TMOUT_INT_EN__MASK);
#else
		bmusb->a_bidl_adis_data.fsm = fsm;
		usb_otg_sync_inputs(fsm);
#endif
		break;

	case OTG_STATE_A_EXIT_HNP:
		fsm->otg->host->b_hnp_enable = 0;
		usb_otg_sync_inputs(fsm);
		break;

	case OTG_STATE_UNDEFINED:
	case OTG_STATE_A_WAIT_VRISE:
	case OTG_STATE_A_WAIT_BCON:
	case OTG_STATE_A_WAIT_VFALL:
	case OTG_STATE_A_VBUS_ERR:
	default:
		dev_warn(otgd->dev, "%s: otg: invalid state: %s\n",
			 __func__, usb_otg_state_string(new_state));
		break;
	}

	fsm->otg->state = new_state;
}

static void bmusb_otg_a_idle_handler(struct otg_fsm *fsm,
				     struct bmusb_dev *bmusb)
{
	u32 reg;

	/* disable SRP detection interrupt */
	bmusb_otg_disable_irq(bmusb,
		OTGIEN_TYPE__SRP_DET_INT_EN__MASK
		| OTGIEN_TYPE__SRP_NOT_COMP_DEV_REMOVED_INT_EN__MASK);
	if (fsm->id && fsm->b_sess_vld)
		bmusb_set_state(fsm, OTG_STATE_B_PERIPHERAL);
	else if (fsm->id && !fsm->b_sess_vld)
		bmusb_set_state(fsm, OTG_STATE_B_IDLE);
	else if (!fsm->id && fsm->a_srp_det) {
		reg = bmusb_readl(&bmusb->regs->OTGSTS);
		if (OTGSTS_TYPE__SRP_DET_NOT_COMPLIANT_DEV__READ(reg)) {
			bmusb_dbg(bmusb->dev, "OTG A-device detected SRP not compliant device\n");
			fsm->a_srp_det_not_compliant_dev = 1;
			bmusb_otg_enable_irq(bmusb,
			    OTGIEN_TYPE__SRP_NOT_COMP_DEV_REMOVED_INT_EN__MASK);
		}
		bmusb_set_state(fsm, OTG_STATE_A_HOST);
	} else {
		/* enable SRP detection interrupt */
		if (fsm->a_srp_det_not_compliant_dev)
			bmusb_otg_enable_irq(bmusb,
			    OTGIEN_TYPE__SRP_NOT_COMP_DEV_REMOVED_INT_EN__MASK);
		else
			bmusb_otg_enable_irq(bmusb,
			    OTGIEN_TYPE__SRP_DET_INT_EN__MASK);
	}
}

static void bmusb_otg_b_idle_handler(struct otg_fsm *fsm,
				     struct bmusb_dev *bmusb)
{
#if IS_ENABLED(CONFIG_USB_BM_MISC)
	u32 reg;

#endif
	if (!fsm->id)
		bmusb_set_state(fsm, OTG_STATE_A_HOST);
	else if (fsm->b_sess_vld)
		bmusb_set_state(fsm, OTG_STATE_B_PERIPHERAL);
#if IS_ENABLED(CONFIG_USB_BM_MISC)
	else if (bmusb->otg_config.otg_caps.srp_support
			&& bmusb->bmusb_misc.force_b_srp_init) {
		reg = bmusb_readl(&bmusb->regs->OTGSTS);
		bmusb->bmusb_misc.force_b_srp_init--;
		if (OTGSTS_TYPE__SRP_INITIAL_CONDITION_MET__READ(reg)) {
			bmusb->bmusb_misc.force_b_srp_init = 0;
			bmusb_set_state(fsm, OTG_STATE_B_SRP_INIT);
		}
	}
#endif
}

static void bmusb_otg_b_peripheral_handler(struct otg_fsm *fsm,
				     struct bmusb_dev *bmusb)
{
	if (!fsm->id)
		bmusb_set_state(fsm, OTG_STATE_A_HOST);
	/* should it go into A_IDLE  (in the above line) */
	else if (!fsm->b_sess_vld)
		bmusb_set_state(fsm, OTG_STATE_B_IDLE);
	else if (fsm->otg->gadget->b_hnp_enable ||
		 fsm->otg->gadget->host_request_flag) {
		/* HNP switching */
		if (fsm->otg->gadget->b_hnp_enable
			&& !fsm->b_aidl_bdis_tmout
			&& fsm->otg->gadget->suspended
			&& OTGIEN_TYPE__TB_AIDL_BDIS_MIN_TMOUT_INT_EN__READ(
			   bmusb_readl(&bmusb->regs->OTGIEN)) == 0) {

			bmusb_writel(&bmusb->regs->OTGCMD,
				     OTGCMD_TYPE__B_HNP_EN_SET__MASK);
			bmusb_writel(&bmusb->regs->OTGCMD,
				     OTGCMD_TYPE__SS_PERIPH_DISABLED_SET__MASK);
			bmusb_otg_enable_irq(bmusb,
			  OTGIEN_TYPE__TB_AIDL_BDIS_MIN_TMOUT_INT_EN__MASK);

		} else if (fsm->otg->gadget->b_hnp_enable
			&& fsm->b_aidl_bdis_tmout
			&& fsm->otg->gadget->suspended) {

			bmusb_otg_disable_irq(bmusb,
			  OTGIEN_TYPE__TB_AIDL_BDIS_MIN_TMOUT_INT_EN__MASK);
			bmusb_set_state(fsm, OTG_STATE_B_WAIT_ACON);
		}
		if (fsm->otg->gadget
			&& fsm->otg->gadget->host_request_flag
			&& fsm->otg->gadget->b_hnp_enable)

			fsm->otg->gadget->host_request_flag = 0;

		/* enable the global synchro timer */
		bmusb_global_synchro_timer_start(bmusb);
	}
}

static void bmusb_otg_a_host_handler(struct otg_fsm *fsm,
				     struct bmusb_dev *bmusb)
{
	struct usb_otg *otgd = container_of(fsm, struct usb_otg, fsm);
	struct usb_hcd *hcd = otgd->primary_hcd.hcd;

	if (fsm->overcurrent)
		bmusb_set_state(fsm, OTG_STATE_A_IDLE);
	else if (fsm->id && fsm->b_sess_vld)
		bmusb_set_state(fsm, OTG_STATE_B_PERIPHERAL);
	else if (fsm->id && !fsm->b_sess_vld)
		bmusb_set_state(fsm, OTG_STATE_B_IDLE);
	else if (fsm->otg->host->b_hnp_enable) {
		bmusb_writel(&bmusb->regs->OTGCMD,
			OTGCMD_TYPE__A_SET_B_HNP_EN_SET__MASK);
		bmusb_suspend_port(fsm, 1);
		hcd->driver->bus_suspend(otgd->primary_hcd.hcd);
		bmusb_set_state(fsm, OTG_STATE_A_SUSPEND);
	}
#if IS_ENABLED(CONFIG_USB_BM_MISC)
	else if (!fsm->id && bmusb->bmusb_misc.force_a_idle) {
		bmusb->bmusb_misc.force_a_idle = 0;
		bmusb_set_state(fsm, OTG_STATE_A_IDLE);
	}
#endif
}

static void bmusb_otg_a_peripheral_handler(struct otg_fsm *fsm,
					   struct bmusb_dev *bmusb)
{
	struct a_bidl_adis_worker_data *a_bidl_adis_data =
					&bmusb->a_bidl_adis_data;
	if (fsm->id)
		bmusb_set_state(fsm, OTG_STATE_B_IDLE);
#if !IS_ENABLED(CONFIG_USB_BM_A_BIDL_ADIS_HW_TMR)
	else if (fsm->otg->gadget->suspended &&
		 fsm->a_bidl_adis_tmout) {
		fsm->a_bidl_adis_tmout = 0;
		bmusb_set_state(fsm, OTG_STATE_A_EXIT_HNP);
	} else if (fsm->otg->gadget->suspended &&
		   !fsm->a_bidl_adis_tmout) {
		if (a_bidl_adis_data->worker_running == 0) {
			INIT_DELAYED_WORK(&(a_bidl_adis_data->a_bidl_adis_work),
					  a_bidl_adis_timeout);
			a_bidl_adis_data->worker_running = 1;
			schedule_delayed_work(
			    &(a_bidl_adis_data->a_bidl_adis_work),
			    msecs_to_jiffies(155));
		}
	} else if (!fsm->otg->gadget->suspended &&
		   !fsm->a_bidl_adis_tmout) {
		if (a_bidl_adis_data->worker_running) {
			a_bidl_adis_data->worker_running = 0;
			cancel_delayed_work(
			    &(a_bidl_adis_data->a_bidl_adis_work));
		}
	}
	bmusb_global_synchro_timer_start(bmusb);
#else
	else if (fsm->a_bidl_adis_tmout) {
		fsm->a_bidl_adis_tmout = 0;
		bmusb_otg_disable_irq(bmusb,
		  OTGIEN_TYPE__TA_BIDL_ADIS_TMOUT_INT_EN__MASK);
		bmusb_set_state(fsm, OTG_STATE_A_EXIT_HNP);
	}
#endif
	if (fsm->otg->gadget &&
	    fsm->otg->gadget->host_request_flag)
		fsm->otg->gadget->host_request_flag = 0;
}

/**
 * bmusb_statemachine - defines condifitons for transitions depending on
 * current state of state machine and otg_fsm structure vars.
 * @fsm: pointer to otg_fsm structure
 *
 * Returns 1 state was not changed, 0 if state was changed
 * (conditions for state change ware met, bmusb_set_state() function
 * was called).
 */
static int bmusb_statemachine(struct otg_fsm *fsm)
{
	struct usb_otg *otgd = container_of(fsm, struct usb_otg, fsm);
	struct device *dev = usb_otg_fsm_to_dev(fsm);
	struct bmusb_dev *bmusb = dev_get_drvdata(dev);
	u32 reg;
	enum usb_otg_state state;
	int res;

	mutex_lock(&fsm->lock);

	state = fsm->otg->state;
	fsm->state_changed = 0;

	switch (state) {
	case OTG_STATE_UNDEFINED:
		if (!fsm->id)
			bmusb_set_state(fsm, OTG_STATE_A_HOST);
		else if (fsm->id && fsm->b_sess_vld)
			bmusb_set_state(fsm, OTG_STATE_B_PERIPHERAL);
		else
			bmusb_set_state(fsm, OTG_STATE_B_IDLE);
		break;
	case OTG_STATE_A_IDLE:
		bmusb_otg_a_idle_handler(fsm, bmusb);
		break;
	case OTG_STATE_B_IDLE:
		bmusb_otg_b_idle_handler(fsm, bmusb);
		break;
	case OTG_STATE_B_PERIPHERAL:
		bmusb_otg_b_peripheral_handler(fsm, bmusb);
		break;
	case OTG_STATE_A_HOST:
		bmusb_otg_a_host_handler(fsm, bmusb);
		break;
	case OTG_STATE_B_SRP_INIT:
		bmusb_otg_disable_irq(bmusb,
			OTGIEN_TYPE__SRP_DET_INT_EN__MASK
			| OTGIEN_TYPE__SRP_NOT_COMP_DEV_REMOVED_INT_EN__MASK);
		if (OTGIVECT_TYPE__SRP_CMPL_INT__READ(bmusb->otg_int_vector)
				&& (fsm->b_sess_vld))
			bmusb_set_state(fsm, OTG_STATE_B_PERIPHERAL);
		else
			bmusb_set_state(fsm, OTG_STATE_B_IDLE);
		break;

	case OTG_STATE_B_WAIT_ACON:
		if (fsm->b_ase0_brst_tmout) {
			fsm->b_ase0_brst_tmout = 0;
			bmusb_otg_disable_irq(bmusb,
				OTGIEN_TYPE__TB_ASE0_BRST_TMOUT_INT_EN__MASK);
			bmusb_set_state(fsm, OTG_STATE_B_PERIPHERAL);
		} else if (!fsm->id || !fsm->b_sess_vld)
			bmusb_set_state(fsm, OTG_STATE_B_IDLE);
		else if (bmusb_check_port_connect(fsm, 1)) {
			bmusb_set_state(fsm, OTG_STATE_B_HOST);
		}
		bmusb_global_synchro_timer_start(bmusb);
		break;

	case OTG_STATE_B_HOST:
		fsm->b_aidl_bdis_tmout = 0;
		fsm->otg->gadget->b_hnp_enable = 0;

		if (!fsm->id || !fsm->b_sess_vld) {
			usb_otg_start_host(fsm, false);
			bmusb_writel(&bmusb->regs->OTGCMD,
				    OTGCMD_TYPE__HOST_BUS_DROP__MASK |
				    OTGCMD_TYPE__HOST_POWER_OFF__MASK);
			bmusb_writel(&bmusb->regs->OTGCMD,
				    OTGCMD_TYPE__DEV_BUS_REQ__MASK);
			usb_otg_start_gadget(fsm, true);

			bmusb_set_state(fsm, OTG_STATE_B_IDLE);
		} else {
			res = is_test_device(fsm);
			switch (res) {
			case -1: /* there is no udev,  reschedule checking */
				bmusb_global_synchro_timer_start(bmusb);
				break;
			case 0: /* it is not test device */
				break;
			case 1: /* it is test device */
				usb_otg_start_host(fsm, false);
				bmusb_writel(&bmusb->regs->OTGCMD,
					    OTGCMD_TYPE__HOST_BUS_DROP__MASK |
					    OTGCMD_TYPE__HOST_POWER_OFF__MASK);
				otg_start_gadget(fsm, true);
				bmusb_set_state(fsm, OTG_STATE_B_PERIPHERAL);
				break;
			default:
				bmusb_err(bmusb->dev,
				"is_test_device returned unexpected value:%d\n",
				res);
				return -ENOTSUPP;
			}
		}
		break;

	case OTG_STATE_A_SUSPEND:
		if (fsm->id || fsm->a_aidl_bdis_tmout)
			bmusb_set_state(fsm, OTG_STATE_B_IDLE);
		else if (!bmusb_check_port_connect(fsm, 1))
			bmusb_set_state(fsm, OTG_STATE_A_PERIPHERAL);
		bmusb_global_synchro_timer_start(bmusb);
		break;

	case OTG_STATE_A_PERIPHERAL:
		bmusb_otg_a_peripheral_handler(fsm, bmusb);
		break;

	case OTG_STATE_A_EXIT_HNP:
		reg = bmusb_readl(&bmusb->regs->OTGSTATE);
		usb_otg_start_gadget(fsm, false);
		bmusb_writel(&bmusb->regs->OTGCMD,
			    OTGCMD_TYPE__DEV_BUS_DROP__MASK |
			    OTGCMD_TYPE__DIS_VBUS_DROP__MASK |
			    OTGCMD_TYPE__DEV_POWER_OFF__MASK);
		bmusb_writel(&bmusb->regs->OTGCMD,
			    OTGCMD_TYPE__HOST_BUS_REQ__MASK |
			    OTGCMD_TYPE__OTG_EN__MASK |
			    OTGCMD_TYPE__A_DEV_EN__MASK);

		reg = bmusb_readl(&bmusb->regs->OTGSTATE);
		usb_otg_start_host(fsm, true);
		bmusb_set_state(fsm, OTG_STATE_A_HOST);
		break;

	/* invalid states for DRD */
	case OTG_STATE_A_WAIT_VRISE:
	case OTG_STATE_A_WAIT_BCON:
	case OTG_STATE_A_WAIT_VFALL:
	case OTG_STATE_A_VBUS_ERR:
		bmusb_err(otgd->dev, "%s: otg: invalid usb-otg state: %s\n",
			__func__, usb_otg_state_string(state));
		bmusb_set_state(fsm, OTG_STATE_UNDEFINED);
	break;
	}

	mutex_unlock(&fsm->lock);
	return fsm->state_changed;
}

/**
 * Bitmain OTG FSM/DRD work function called by FSM worker.
 */
void bmusb_usb_otg_work(struct work_struct *work)
{
	struct usb_otg *otgd = container_of(work, struct usb_otg, work);

	/* BM OTG state machine */
	bmusb_statemachine(&otgd->fsm);
}
