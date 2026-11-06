#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/spinlock.h>
#include <linux/platform_device.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/usb/of.h>
#include <linux/usb/otg.h>
#include <linux/of.h>
#include <linux/of_platform.h>
#include <linux/workqueue.h>
#include <linux/of_gpio.h>

#include <linux/reset.h>

#include "bmusb_drd.h"
#include "io.h"
#include "otg_fsm.h"

/**
 * STRAP=NON
 * start as device:ok			otg switch:stuck at wait host ready
 * start as host:stuck at wait host ready
 *
 * STRAP=HOST
 * start as device:err
 * start as host:OK				otg switch:ok
 *
 * STRAP=DEVICE
 * start as device:ok			otg switch:stuck at wait host ready
 * start as host:stuck at wait host ready
 *
 * so we use OTG_BYPASS
 */

static struct bmusb_dev *_bmusb;

/**
 * bmusb_set_mode - change mode of OTG Core
 * @bmusb: pointer to our context structure
 * @mode: selected mode from bmusb_role
 */
void bmusb_set_mode(struct bmusb_dev *bmusb, u32 mode)
{
	u32 reg;

	switch (mode) {
	case BM_ROLE_GADGET:
		bmusb_writel(&bmusb->regs->OTGCMD,
				OTGCMD_TYPE__DEV_BUS_REQ__MASK
				| OTGCMD_TYPE__OTG_DIS__MASK);
		break;
	case BM_ROLE_HOST:
		bmusb_writel(&bmusb->regs->OTGCMD,
				OTGCMD_TYPE__HOST_BUS_REQ__MASK
				| OTGCMD_TYPE__OTG_DIS__MASK);
		break;
	case BM_ROLE_OTG:
		reg = bmusb_readl(&bmusb->regs->OTGCTRL1);
		bmusb_writel(&bmusb->regs->OTGCTRL1,
				OTGCTRL1_TYPE__IDPULLUP__SET(reg));
		/*
		 * wait until valid ID (ID_VALUE) can be sampled (50ms)
		 */
#ifdef USB_SIM_SPEED_UP
		udelay(50);
#else
		mdelay(50);
#endif
		/*
		 * OTG mode is initialized later
		 */
		break;
	default:
		bmusb_err(bmusb->dev, "Unsupported mode of operation %d\n",
			  mode);
		return;
	}
	bmusb->current_mode = mode;
}

/**
 * bmusb_core_init - Low-level initialization of Bitmain OTG Core
 * @bmusb: Pointer to our controller context structure
 *
 * Returns 0 on success otherwise negative errno.
 */
static int bmusb_core_init(struct bmusb_dev *bmusb)
{
	u32	reg;

	reg = bmusb_readl(&bmusb->regs->OTGVERSION);

	if ((OTGVERSION_TYPE__OTGVERSION__READ(reg)) != OTG_HW_VERSION) {
		bmusb_err(bmusb->dev, "this is not a Bitmain USB3 OTG Core\n");
		return -ENODEV;
	}
	bmusb->otg_version = OTGVERSION_TYPE__OTGVERSION__READ(reg);

	reg = bmusb_readl(&bmusb->regs->OTGSTS);
	if (OTGSTS_TYPE__OTG_NRDY__READ(reg) != 0) {
		bmusb_err(bmusb->dev, "Bitmain USB3 OTG device not ready\n");
		return -ENODEV;
	}

#ifdef USB_SIM_SPEED_UP
	bmusb_dbg(bmusb->dev, "Enable fast simulation timing modes\n");
	bmusb_writel(&bmusb->regs->OTGSIMULATE,
			OTGSIMULATE_TYPE__OTG_CFG_FAST_SIMS__MASK);
#endif
	return 0;
}

/**
 * bmusb_otg_fsm_sync - Get OTG events and sync it to OTG fsm
 * @bmusb: Pointer to our controller context structure
 */
void bmusb_otg_fsm_sync(struct bmusb_dev *bmusb)
{
	u32 reg;
	int id, vbus;

	reg = bmusb_readl(&bmusb->regs->OTGSTS);

	id = OTGSTS_TYPE__ID_VALUE__READ(reg);
	vbus = OTGSTS_TYPE__SESSION_VALID__READ(reg);

	bmusb->fsm->id = id;
	bmusb->fsm->b_sess_vld = vbus;
	bmusb->fsm->overcurrent =
		OTGIEN_TYPE__OVERCURRENT_INT_EN__READ(bmusb->otg_int_vector);

	if (OTGIEN_TYPE__SRP_NOT_COMP_DEV_REMOVED_INT_EN__READ(
			bmusb->otg_int_vector
			)) {
		bmusb->fsm->a_srp_det_not_compliant_dev = 0;
		bmusb_otg_disable_irq(bmusb,
			OTGIEN_TYPE__SRP_NOT_COMP_DEV_REMOVED_INT_EN__MASK);
	}

	if (bmusb->fsm->a_srp_det_not_compliant_dev)
		bmusb->fsm->a_srp_det = 0;
	else
		bmusb->fsm->a_srp_det =
			OTGIVECT_TYPE__SRP_DET_INT__READ(bmusb->otg_int_vector);

	if (OTGIVECT_TYPE__TB_AIDL_BDIS_MIN_TMOUT_INT__READ(
						bmusb->otg_int_vector))
		bmusb->fsm->b_aidl_bdis_tmout = 1;

#if IS_ENABLED(CONFIG_USB_BM_A_BIDL_ADIS_HW_TMR)
	if (OTGIVECT_TYPE__TA_BIDL_ADIS_TMOUT_INT__READ(bmusb->otg_int_vector))
		bmusb->fsm->a_bidl_adis_tmout = 1;
#endif

	if (OTGIVECT_TYPE__TB_ASE0_BRST_TMOUT_INT__READ(bmusb->otg_int_vector))
		bmusb->fsm->b_ase0_brst_tmout = 1;

	usb_otg_sync_inputs(bmusb->fsm);
}

/**
 * bmusb_otg_standby_allowed - standby (aka slow reference clock)
 * is allowed only when both modes (host/device) are off
 * @bmusb: Pointer to our controller context structure
 *
 *  Returns 1 if allowed otherwise 0.
 */
int bmusb_otg_standby_allowed(struct bmusb_dev *bmusb)
{
	if ((bmusb->fsm->otg->state == OTG_STATE_B_IDLE) ||
			(bmusb->fsm->otg->state == OTG_STATE_A_IDLE))
		return 1;
	return 0;
}

/**
 * bmusb_otg_set_standby - set standby mode aka slow reference clock
 * @bmusb: Pointer to our controller context structure
 *
 * Returns 0 on success otherwise negative errno.
 */
int bmusb_otg_set_standby(struct bmusb_dev *bmusb)
{
	u32 reg;
	int phy_refclk_valid = 0;

	if (!bmusb_otg_standby_allowed(bmusb))
		return -EPERM;
	reg = bmusb_readl(&bmusb->regs->OTGREFCLK);
	bmusb_writel(&bmusb->regs->OTGREFCLK,
			OTGREFCLK_TYPE__OTG_STB_CLK_SWITCH_EN__SET(reg));
	/*
	 * signal from the PHY Reference Clock Control interface
	 * should fall to 0
	 */
	do {
		reg = bmusb_readl(&bmusb->regs->OTGSTATE);
		phy_refclk_valid = OTGSTATE_TYPE__PHY_REFCLK_VALID__READ(reg);
		if (phy_refclk_valid)
#ifdef USB_SIM_SPEED_UP
			udelay(100);
#else
			mdelay(10);
#endif
	} while (phy_refclk_valid);
	return 0;
}

/**
 * bmusb_otg_clear_standby - switch off standby mode aka slow reference clock
 * @bmusb: Pointer to our controller context structure
 *
 * Returns 0 on success otherwise negative errno.
 */
int bmusb_otg_clear_standby(struct bmusb_dev *bmusb)
{
	u32 reg;
	int phy_refclk_valid = 0;

	reg = bmusb_readl(&bmusb->regs->OTGREFCLK);
	if (!OTGREFCLK_TYPE__OTG_STB_CLK_SWITCH_EN__READ(reg))
		/* Don't try to stop clock which has been already stopped */
		return -EPERM;
	bmusb_writel(&bmusb->regs->OTGREFCLK,
		OTGREFCLK_TYPE__OTG_STB_CLK_SWITCH_EN__CLR(reg));
	/*
	 * signal from the PHY Reference Clock Control interface
	 * should rise to 1
	 */
	do {
		reg = bmusb_readl(&bmusb->regs->OTGSTATE);
		phy_refclk_valid = OTGSTATE_TYPE__PHY_REFCLK_VALID__READ(reg);
		if (!phy_refclk_valid)
#ifdef USB_SIM_SPEED_UP
			udelay(100);
#else
			mdelay(10);
#endif
	} while (!phy_refclk_valid);
	return 0;
}

/**
 * bmusb_otg_mask_irq - Mask all interrupts
 * @bmusb: Pointer to our controller context structure
 */
static void bmusb_otg_mask_irq(struct bmusb_dev *bmusb)
{
	bmusb_writel(&bmusb->regs->OTGIEN, 0);
}

/**
 * bmusb_otg_unmask_irq - Unmask id and sess_valid interrupts
 * @bmusb: Pointer to our controller context structure
 */
static void bmusb_otg_unmask_irq(struct bmusb_dev *bmusb)
{
	bmusb_writel(&bmusb->regs->OTGIEN,
			OTGIEN_TYPE__OTGSESSVALID_FALL_INT_EN__MASK
			| OTGIEN_TYPE__OTGSESSVALID_RISE_INT_EN__MASK
			| OTGIEN_TYPE__ID_CHANGE_INT_EN__MASK
			| OTGIEN_TYPE__OVERCURRENT_INT_EN__MASK
			| OTGIEN_TYPE__TIMER_TMOUT_INT_EN__MASK);
}

/**
 * bmusb_otg_enable_irq - enable desired interrupts
 * @bmusb: Pointer to our controller context structure
 * @irq: interrupt mask
 */
void bmusb_otg_enable_irq(struct bmusb_dev *bmusb, u32 irq)
{
	u32 reg;

	reg = bmusb_readl(&bmusb->regs->OTGIEN);
	bmusb_writel(&bmusb->regs->OTGIEN, reg | irq);
}

/**
 * bmusb_otg_disable_irq - disable desired interrupts
 * @bmusb: Pointer to our controller context structure
 * @irq: interrupt mask
 */
void bmusb_otg_disable_irq(struct bmusb_dev *bmusb, u32 irq)
{
	u32 reg;

	reg = bmusb_readl(&bmusb->regs->OTGIEN);
	bmusb_writel(&bmusb->regs->OTGIEN, reg & ~irq);
}

/**
 * bmusb_otg_irq - interrupt thread handler
 * @irq: interrupt number
 * @bmusb_ptr: Pointer to our controller context structure
 *
 * Returns IRQ_HANDLED on success.
 */
static irqreturn_t bmusb_otg_thread_irq(int irq, void *bmusb_ptr)
{
	struct bmusb_dev *bmusb = bmusb_ptr;
	unsigned long flags;

	spin_lock_irqsave(&bmusb->lock, flags);
	bmusb_otg_fsm_sync(bmusb);
	spin_unlock_irqrestore(&bmusb->lock, flags);

	return IRQ_HANDLED;
}

/**
 * bmusb_otg_irq - interrupt handler
 * @irq: interrupt number
 * @bmusb_ptr: Pointer to our controller context structure
 *
 * Returns IRQ_WAKE_THREAD on success otherwise IRQ_NONE.
 */
static irqreturn_t bmusb_otg_irq(int irq, void *bmusb_ptr)
{
	struct bmusb_dev *bmusb = bmusb_ptr;
	irqreturn_t ret = IRQ_NONE;
	u32 reg;

	spin_lock(&bmusb->lock);

	reg = bmusb_readl(&bmusb->regs->OTGIVECT);
	bmusb->otg_int_vector = reg;
	if (reg) {
		bmusb_writel(&bmusb->regs->OTGIVECT, reg);
		ret = IRQ_WAKE_THREAD;
	}

	spin_unlock(&bmusb->lock);

	return ret;
}

/**
 * bmusb_wait_for_ready - wait for host or gadget to be ready
 * for working
 * @bmusb: Pointer to our controller context structure
 * @otgsts_bit_ready: which bit should be monitored
 *
 * Returns 0 on success otherwise negative errno
 */
static int bmusb_wait_for_ready(struct bmusb_dev *bmusb, int otgsts_bit_ready)
{
	char ready = 0;
	u32 reg;

	do {
		/*
		 * TODO: it should be considered to add
		 * some timeout here and return error.
		 */
		reg = bmusb_readl(&bmusb->regs->OTGSTS);
		ready = (reg >> otgsts_bit_ready) & 0x0001;
		if (!ready)
#ifdef USB_SIM_SPEED_UP
			udelay(100);
#else
			mdelay(100);
#endif
	} while (!ready);

	return 0;
}

/**
 * bmusb_wait_for_idle - wait for host or gadget switched
 * to idle
 * @bmusb: Pointer to our controller context structure
 * @otgsts_bit_ready: which bit should be monitored
 *
 * Returns 0 on success otherwise negative errno
 */
static int bmusb_wait_for_idle(struct bmusb_dev *bmusb, int otgsts_bits_idle)
{
	char not_idle = 0;
	u32 reg;

	do {
		/*
		 * TODO: it should be considered to add
		 * some timeout here and return error.
		 */
		reg = bmusb_readl(&bmusb->regs->OTGSTATE);
		not_idle = otgsts_bits_idle & reg;
		if (not_idle)
#ifdef USB_SIM_SPEED_UP
			udelay(100);
#else
			mdelay(100);
#endif
	} while (not_idle);

	return 0;
}
#define DAMR_REG_USB_REMAP_ADDR_39_32_MASK		0xFF0000
#define DAMR_REG_USB_REMAP_ADDR_39_32_OFFSET	16

#define UCR_MODE_STRAP_OFFSET	0
#define UCR_MODE_STRAP_NON		0x0
#define UCR_MODE_STRAP_HOST		0x2
#define UCR_MODE_STRAP_DEVICE	0x4
#define UCR_MODE_STRAP_MSK		(0x7)
#define UCR_PORT_OVER_CURRENT_ACTIVE_OFFSET		10

#define UPCR_EXTERNAL_VBUS_VALID_OFFSET			0U
#define UPCR_IDPAD_C_OW_OFFSET				(1U<<6)
#define UPCR_IDPAD_C_SW_OFFSET				(1U<<7)

static int _bmusb_drd_override_id(struct bmusb_dev *bmusb, int value)
{
	bmusb_dbg(bmusb->dev, "%s: %d\n", __func__, value);

	if (bmusb->otg_bypass) {
		/* Enable idpad override. */
		u32 tmp = readl(bmusb->regs_usbPhyCtrl)
				| UPCR_IDPAD_C_OW_OFFSET;

		tmp |= (value ? UPCR_IDPAD_C_SW_OFFSET : 0);
		writel(tmp, bmusb->regs_usbPhyCtrl);
	}

	return 0;
}

int bmusb_drd_override_id(int value)
{
	struct bmusb_dev *bmusb = _bmusb;

	if (!bmusb)
		return -ENODEV;

	return _bmusb_drd_override_id(bmusb, value);
}
EXPORT_SYMBOL_GPL(bmusb_drd_override_id);

/**
 * bmusb_drd_start_host - start/stop host
 * @fsm: Pointer to our finite state machine
 * @on: 1 for start, 0 for stop
 *
 * Returns 0 on success otherwise negative errno
 */
static int bmusb_drd_start_host(struct otg_fsm *fsm, int on)
{
	struct device *dev = usb_otg_fsm_to_dev(fsm);
	struct bmusb_dev *bmusb = dev_get_drvdata(dev);
	int ret;

	bmusb_dbg(dev, "%s: %d\n", __func__, on);

	/* switch OTG core */
	if (on) {
		u32 value;

		value =	OTGCMD_TYPE__HOST_BUS_REQ__MASK
			| OTGCMD_TYPE__OTG_EN__MASK
			| OTGCMD_TYPE__A_DEV_EN__MASK;

		if (bmusb->ss_dis)
			value |= OTGCMD_TYPE__SS_HOST_DISABLED_SET__MASK;

		bmusb_writel(&bmusb->regs->OTGCMD, value);

		bmusb_dbg(bmusb->dev, "Waiting for XHC...\n");

		ret = bmusb_wait_for_ready(bmusb,
					   OTGSTS_TYPE__XHC_READY__SHIFT);
		if (ret)
			return ret;
		/* start the HCD */
		usb_otg_start_host(fsm, true);
	} else {
		/* stop the HCD */
		usb_otg_start_host(fsm, false);

		/* stop OTG */
		bmusb_writel(&bmusb->regs->OTGCMD,
				OTGCMD_TYPE__HOST_BUS_DROP__MASK
				| OTGCMD_TYPE__HOST_POWER_OFF__MASK
				| OTGCMD_TYPE__DEV_BUS_DROP__MASK
				| OTGCMD_TYPE__DEV_POWER_OFF__MASK);
		ret = bmusb_wait_for_idle(bmusb,
				OTGSTATE_TYPE__HOST_OTG_STATE__MASK);
	}

	return 0;
}

/**
 * bmusb_drd_start_gadget - start/stop gadget
 * @fsm: Pointer to our finite state machine
 * @on: 1 for start, 0 for stop
 *
 * Returns 0 on success otherwise negative errno
 */
static int bmusb_drd_start_gadget(struct otg_fsm *fsm, int on)
{
	struct device *dev = usb_otg_fsm_to_dev(fsm);
	struct bmusb_dev *bmusb = dev_get_drvdata(dev);
	int ret;

	bmusb_dbg(dev, "%s: %d\n", __func__, on);

	/* switch OTG core */
	if (on) {
		u32 value;

		value = OTGCMD_TYPE__DEV_BUS_REQ__MASK
			| OTGCMD_TYPE__OTG_EN__MASK
			| OTGCMD_TYPE__A_DEV_DIS__MASK;

		if (bmusb->otg_bypass)
			value |= OTGCMD_TYPE__A_SESS_USE_CLR__MASK;

		if (bmusb->ss_dis)
			value |= OTGCMD_TYPE__SS_PERIPH_DISABLED_SET__MASK;

		bmusb_writel(&bmusb->regs->OTGCMD, value);

		bmusb_dbg(dev, "Waiting for BM_GADGET...\n");
		ret = bmusb_wait_for_ready(bmusb,
					   OTGSTS_TYPE__DEV_READY__SHIFT);
		if (ret)
			return ret;

		/* start the UDC */
		usb_otg_start_gadget(fsm, true);
	} else {
		/* stop the UDC */
		usb_otg_start_gadget(fsm, false);

		/*
		 * driver should wait at least 10us after disabling Device
		 * before turning-off Device (DEV_BUS_DROP)
		 */
		udelay(30);

		/* stop OTG */
		bmusb_writel(&bmusb->regs->OTGCMD,
				OTGCMD_TYPE__HOST_BUS_DROP__MASK
				| OTGCMD_TYPE__HOST_POWER_OFF__MASK
				| OTGCMD_TYPE__DEV_BUS_DROP__MASK
				| OTGCMD_TYPE__DEV_POWER_OFF__MASK);
		ret = bmusb_wait_for_idle(bmusb,
				OTGSTATE_TYPE__DEV_OTG_STATE__MASK);
	}

	return 0;
}

static struct otg_fsm_ops bmusb_drd_ops = {
	.start_host = bmusb_drd_start_host,
	.start_gadget = bmusb_drd_start_gadget,
};

/**
 * bmusb_drd_register - register out drd controller
 * into otg framework
 * @bmusb: Pointer to our controller context structure
 *
 * Returns 0 on success otherwise negative errno
 */
static int bmusb_drd_register(struct bmusb_dev *bmusb)
{
	int ret;

	/* register parent as DRD device with OTG core */
	bmusb->fsm = usb_otg_register(bmusb->dev,
			&bmusb->otg_config, bmusb_usb_otg_work);
	if (IS_ERR(bmusb->fsm)) {
		ret = PTR_ERR(bmusb->fsm);
		if (ret == -ENOTSUPP)
			bmusb_err(bmusb->dev, "CONFIG_USB_OTG needed for dual-role\n");
		else
			bmusb_err(bmusb->dev, "Failed to register with OTG core\n");

		return ret;
	}

	/*
	 * The memory of host_req_flag should be allocated by
	 * controller driver, otherwise, hnp polling is not started.
	 */
	bmusb->fsm->host_req_flag =
		kmalloc(sizeof(*bmusb->fsm->host_req_flag), GFP_KERNEL);
	if (!bmusb->fsm->host_req_flag)
		return -ENOTSUPP;

	INIT_DELAYED_WORK(&bmusb->fsm->hnp_polling_work, otg_hnp_polling_work);

	return 0;
}

int bmusb_global_synchro_timer_setup(struct bmusb_dev *bmusb)
{
	/* set up timer for fsm synchronization */
#ifdef USB_SIM_SPEED_UP
	bmusb_writel(&bmusb->regs->OTGTMR,
		    OTGTMR_TYPE__TIMER_WRITE__MASK |
		    OTGTMR_TYPE__TIMEOUT_UNITS__WRITE(0) |
		    OTGTMR_TYPE__TIMEOUT_VALUE__WRITE(1)
		);
#else
	bmusb_writel(&bmusb->regs->OTGTMR,
		    OTGTMR_TYPE__TIMER_WRITE__MASK |
		    OTGTMR_TYPE__TIMEOUT_UNITS__WRITE(2) |
		    OTGTMR_TYPE__TIMEOUT_VALUE__WRITE(1)
		);
#endif
	return 0;
}

int bmusb_global_synchro_timer_start(struct bmusb_dev *bmusb)
{
	bmusb_writel(&bmusb->regs->OTGTMR,
		    bmusb_readl(&bmusb->regs->OTGTMR) |
		    OTGTMR_TYPE__TIMER_START__MASK);
	return 0;
}

int bmusb_global_synchro_timer_stop(struct bmusb_dev *bmusb)
{
	bmusb_writel(&bmusb->regs->OTGTMR,
		    bmusb_readl(&bmusb->regs->OTGTMR) |
		    OTGTMR_TYPE__TIMER_STOP__MASK);
	return 0;
}


/**
 * bmusb_drd_init - initialize our drd controller
 * @bmusb: Pointer to our controller context structure
 *
 * Returns 0 on success otherwise negative errno
 */
static int bmusb_drd_init(struct bmusb_dev *bmusb)
{
	int ret;
	struct usb_otg_caps *otgcaps = &bmusb->otg_config.otg_caps;
	unsigned long flags;
	u32 reg;

	reg = bmusb_readl(&bmusb->regs->OTGCAPABILITY);
	otgcaps->otg_rev = OTGCAPABILITY_TYPE__OTG2REVISION__READ(reg);
	otgcaps->hnp_support = OTGCAPABILITY_TYPE__HNP_SUPPORT__READ(reg);
	otgcaps->srp_support = OTGCAPABILITY_TYPE__SRP_SUPPORT__READ(reg);
	otgcaps->adp_support = OTGCAPABILITY_TYPE__ADP_SUPPORT__READ(reg);

	/* Update otg capabilities by DT properties */
	ret = of_usb_update_otg_caps(bmusb->dev->of_node, otgcaps);
	if (ret)
		return ret;

	bmusb->otg_config.fsm_ops = &bmusb_drd_ops;

	bmusb_dbg(bmusb->dev, "rev:0x%x, hnp:%d, srp:%d, adp:%d\n",
			otgcaps->otg_rev, otgcaps->hnp_support,
			otgcaps->srp_support, otgcaps->adp_support);

	ret = bmusb_drd_register(bmusb);
	if (ret)
		goto error0;

	/* global fsm synchronization timer */
	bmusb_global_synchro_timer_setup(bmusb);
	/* bmusb_global_synchro_timer_start(bmusb); */

	/* disable all irqs */
	bmusb_otg_mask_irq(bmusb);
	/* clear all interrupts */
	bmusb_writel(&bmusb->regs->OTGIVECT, ~0);

	ret = devm_request_threaded_irq(bmusb->dev, bmusb->otg_irq,
					bmusb_otg_irq,
					bmusb_otg_thread_irq,
					IRQF_SHARED, "bmusb-otg", bmusb);
	if (ret) {
		bmusb_err(bmusb->dev, "failed to request irq #%d --> %d\n",
			bmusb->otg_irq, ret);
		ret = -ENODEV;
		goto error1;
	}

	spin_lock_irqsave(&bmusb->lock, flags);

	/* we need to set OTG to get events from OTG core */
	bmusb_set_mode(bmusb, BM_ROLE_OTG);
	if (bmusb->otg_bypass)
		bmusb_set_state(bmusb->fsm, OTG_STATE_A_HOST);

	/* Enable ID ans sess_valid event interrupt */
	bmusb_otg_unmask_irq(bmusb);

	spin_unlock_irqrestore(&bmusb->lock, flags);

	bmusb_otg_fsm_sync(bmusb);
	usb_otg_sync_inputs(bmusb->fsm);

	return 0;

error1:
	usb_otg_unregister(bmusb->dev);
error0:
	return ret;
}

/**
 * bmusb_drd_exit - unregister and clean up drd controller
 * @bmusb: Pointer to our controller context structure
 */
static void bmusb_drd_exit(struct bmusb_dev *bmusb)
{
	usb_otg_unregister(bmusb->dev);
}

/**
 * bmusb_core_init_mode - initialize mode of operation
 * @bmusb: Pointer to our controller context structure
 *
 * Returns 0 on success otherwise negative errno
 */
static int bmusb_core_init_mode(struct bmusb_dev *bmusb)
{
	int ret;

	switch (bmusb->dr_mode) {
	case USB_DR_MODE_PERIPHERAL:
		bmusb_set_mode(bmusb, BM_ROLE_GADGET);
		break;
	case USB_DR_MODE_HOST:
		bmusb_set_mode(bmusb, BM_ROLE_HOST);
		break;
	case USB_DR_MODE_OTG:
		ret = bmusb_drd_init(bmusb);
		if (ret) {
			bmusb_err(bmusb->dev, "limiting to peripheral only\n");
			bmusb->dr_mode = USB_DR_MODE_PERIPHERAL;
			bmusb_set_mode(bmusb, BM_ROLE_GADGET);
		}
		break;
	default:
		bmusb_err(bmusb->dev, "Unsupported mode of operation %d\n",
				bmusb->dr_mode);
		return -EINVAL;
	}

	return 0;
}

/**
 * bmusb_core_exit_mode - clean up drd controller
 * @bmusb: Pointer to our controller context structure
 */
static void bmusb_core_exit_mode(struct bmusb_dev *bmusb)
{
	switch (bmusb->dr_mode) {
	case USB_DR_MODE_PERIPHERAL:
		break;
	case USB_DR_MODE_HOST:
		break;
	case USB_DR_MODE_OTG:
		bmusb_drd_exit(bmusb);
		break;
	default:
		/* do nothing */
		break;
	}
}

static int vbus_is_present(struct bmusb_dev *bmusb)
{
	if (gpio_is_valid(bmusb->vbus_pin))
		return gpio_get_value(bmusb->vbus_pin) ^
			bmusb->vbus_pin_inverted;

	/* No Vbus detection: Assume always present */
	return 1;
}

static irqreturn_t vbus_irq_handler(int irq, void *devid)
{
	struct bmusb_dev *bmusb = devid;
	u32 reg;
	int vbus, id;

	reg = bmusb_readl(&bmusb->regs->OTGSTS);
	id = OTGSTS_TYPE__ID_VALUE__READ(reg);
	vbus = vbus_is_present(bmusb);
	/* do nothing if we are an A-device (vbus provider). */
	if (id == 0)
		return IRQ_HANDLED;
	bmusb_dbg(bmusb->dev, "vbus int = %d\n", vbus);
	return IRQ_WAKE_THREAD;
}

static irqreturn_t vbus_irq_thread(int irq, void *devid)
{
	struct bmusb_dev *bmusb = devid;
	struct usb_gadget *gadget = bmusb->fsm->otg->gadget;
	int vbus;

	if (!gadget)
		return IRQ_HANDLED;

	/* debounce */
	udelay(10);
	vbus = vbus_is_present(bmusb);
	if (bmusb->pre_vbus_status != vbus) {
		bmusb_dbg(bmusb->dev, "vbus thread = %d\n", vbus);
		usb_udc_vbus_handler(gadget, (vbus != 0));
		bmusb->pre_vbus_status = vbus;
	}
	return IRQ_HANDLED;
}

/**
 * bmusb_probe - bind our drd driver
 * @pdev: Pointer to Linux platform device
 *
 * Returns 0 on success otherwise negative errno
 */
static int bmusb_probe(struct platform_device *pdev)
{
	struct device		*dev = &pdev->dev;
	struct resource		*res;
	struct bmusb_dev		*bmusb;
	int			ret;
	u32			status;
	void __iomem		*regs;
	void			*mem;
	enum of_gpio_flags	flags;
	const char		*tmp_char;

	bmusb_dbg(dev, "Bitmain usb driver: probe()\n");

	mem = devm_kzalloc(dev, sizeof(*bmusb) + BM_ALIGN_MASK, GFP_KERNEL);
	if (!mem)
		return -ENOMEM;

	bmusb = PTR_ALIGN(mem, BM_ALIGN_MASK + 1);
	bmusb->mem = mem;
	bmusb->dev = dev;

	res = platform_get_resource(pdev, IORESOURCE_IRQ, 0);
	if (!res) {
		bmusb_err(dev, "missing IRQ\n");
		return -ENODEV;
	}
	bmusb->otg_irq = res->start;

	/*
	 * Request memory region
	 */
	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	regs = devm_ioremap_resource(dev, res);
	if (IS_ERR(regs)) {
		ret = PTR_ERR(regs);
		goto err0;
	}

	bmusb->regs	= regs;
	bmusb->regs_size	= resource_size(res);

	bmusb->usb_reset = devm_reset_control_get(&pdev->dev, "usb");

	res = platform_get_resource(pdev, IORESOURCE_MEM, 1);
	regs = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(regs)) {
		ret = PTR_ERR(regs);
		goto err0;
	}
	bmusb->regs_addrRemap = regs;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 2);
	regs = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(regs)) {
		ret = PTR_ERR(regs);
		goto err0;
	}
	bmusb->regs_ctrl0 = regs;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 3);
	regs = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(regs)) {
		ret = PTR_ERR(regs);
		goto err0;
	}
	bmusb->regs_usbPhyCtrl = regs;

	status = bmusb_readl(&bmusb->regs->OTGSTS);

	platform_set_drvdata(pdev, bmusb);
#if IS_ENABLED(CONFIG_USB_BM_MISC)
	bmusb_drd_misc_register(bmusb, res->start);
#endif
	spin_lock_init(&bmusb->lock);

	bmusb->vbus_pin = of_get_named_gpio_flags(pdev->dev.of_node,
				"vbus-gpio", 0, &flags);
	bmusb->vbus_pin_inverted = (flags & OF_GPIO_ACTIVE_LOW) ? 1 : 0;
	bmusb_dbg(bmusb->dev, "vbus_pin = %d, flags = %d\n",
			bmusb->vbus_pin, bmusb->vbus_pin_inverted);
	if (gpio_is_valid(bmusb->vbus_pin)) {
		if (!devm_gpio_request(&pdev->dev,
			bmusb->vbus_pin, "bmusb-otg")) {
			irq_set_status_flags(gpio_to_irq(bmusb->vbus_pin),
					IRQ_NOAUTOEN);
			ret = devm_request_threaded_irq(&pdev->dev,
					gpio_to_irq(bmusb->vbus_pin),
					vbus_irq_handler,
					vbus_irq_thread,
					IRQF_SHARED |
					IRQF_TRIGGER_RISING |
					IRQF_TRIGGER_FALLING,
					"bmusb-otg", bmusb);
			if (ret) {
				bmusb->vbus_pin = -ENODEV;
				bmusb_err(bmusb->dev,
					"failed to request vbus irq\n");
			} else {
				bmusb->pre_vbus_status = vbus_is_present(bmusb);
				enable_irq(gpio_to_irq(bmusb->vbus_pin));
				bmusb_dbg(bmusb->dev,
					"enable vbus irq, vbus status = %d\n",
					bmusb->pre_vbus_status);
			}
		} else {
			/* gpio_request fail so use -EINVAL for gpio_is_valid */
			bmusb->vbus_pin = -EINVAL;
			bmusb_err(bmusb->dev, "request gpio fail!\n");
		}
	}
	/* register for address 39to32 of usb */
	writel((readl(bmusb->regs_addrRemap)
		& (~DAMR_REG_USB_REMAP_ADDR_39_32_MASK))
		| (1 << DAMR_REG_USB_REMAP_ADDR_39_32_OFFSET),
		bmusb->regs_addrRemap);

	reset_control_assert(bmusb->usb_reset);
	udelay(50);

	/* bypass OTG and start as host? */
	ret = device_property_read_string(dev, "otg_bypass", &tmp_char);
	if (ret < 0) {
		bmusb->otg_bypass = 0;
	} else {
		if (!strcmp(tmp_char, "true"))
			bmusb->otg_bypass = 1;
		else
			bmusb->otg_bypass = 0;
	}

	/* Controller initially configured as Host */
	writel((bmusb->otg_bypass ? UCR_MODE_STRAP_HOST : UCR_MODE_STRAP_NON)
		| (1 << UCR_PORT_OVER_CURRENT_ACTIVE_OFFSET),
		bmusb->regs_ctrl0);

	reset_control_deassert(bmusb->usb_reset);
	udelay(50);
	/* External VBUS valid. (external_vbusvalid) */
	writel(readl(bmusb->regs_usbPhyCtrl)
		| (1 << UPCR_EXTERNAL_VBUS_VALID_OFFSET),
		bmusb->regs_usbPhyCtrl);
	/* Enable idpad override and set to 0 if support otg_bypass. */
	_bmusb_drd_override_id(bmusb, 0);
	/* Support super speed. */
	ret = device_property_read_string(dev, "dis_ss", &tmp_char);
	if (ret < 0) {
		bmusb->ss_dis = 0;
	} else {
		if (!strcmp(tmp_char, "true"))
			bmusb->ss_dis = 1;
		else
			bmusb->ss_dis = 0;
	}

	/* device tree set dr_mode */
	bmusb->dr_mode = usb_get_dr_mode(dev);
	if (bmusb->dr_mode == USB_DR_MODE_UNKNOWN) {
		bmusb->strap = OTGSTS_TYPE__STRAP__READ(status);
		if (bmusb->strap == STRAP_HOST)
			bmusb->dr_mode = USB_DR_MODE_HOST;
		else if (bmusb->strap == STRAP_GADGET)
			bmusb->dr_mode = USB_DR_MODE_PERIPHERAL;
		else if (bmusb->strap == STRAP_HOST_OTG)
			bmusb->dr_mode = USB_DR_MODE_OTG;
		else {
			bmusb_err(dev, "No default configuration, configuring as OTG.");
			bmusb->dr_mode = USB_DR_MODE_OTG;
		}
	}

	ret = bmusb_core_init(bmusb);
	if (ret) {
		bmusb_err(dev, "failed to initialize core\n");
		goto err0;
	}

	ret = bmusb_core_init_mode(bmusb);
	if (ret)
		goto err1;

	_bmusb = bmusb;

	return 0;

err1:
	bmusb_core_exit_mode(bmusb);
err0:
	return ret;
}

/**
 * bmusb_remove - unbind our drd driver and clean up
 * @pdev: Pointer to Linux platform device
 *
 * Returns 0 on success otherwise negative errno
 */
static int bmusb_remove(struct platform_device *pdev)
{
	struct bmusb_dev	*bmusb = platform_get_drvdata(pdev);

	kfree(bmusb->fsm->host_req_flag);
	cancel_delayed_work(&bmusb->fsm->hnp_polling_work);
#if !IS_ENABLED(CONFIG_USB_BM_A_BIDL_ADIS_HW_TMR)
	cancel_delayed_work(&(bmusb->a_bidl_adis_data.a_bidl_adis_work));
#endif

	bmusb_core_exit_mode(bmusb);
	_bmusb = NULL;

	return 0;
}

#ifdef CONFIG_OF
static const struct of_device_id of_bmusb_match[] = {
	{ .compatible = "bitmain,usb-otg" },
	{ },
};
MODULE_DEVICE_TABLE(of, of_bmusb_match);
#endif

static struct platform_driver bmusb_driver = {
	.probe		= bmusb_probe,
	.remove		= bmusb_remove,
	.driver		= {
		.name	= "bmusb-otg",
		.of_match_table	= of_match_ptr(of_bmusb_match),
	},
};

module_platform_driver(bmusb_driver);

MODULE_ALIAS("platform:bmusb");
MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("Bitmain USB3 DRD Controller Driver");
