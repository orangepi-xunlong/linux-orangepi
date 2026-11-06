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
#include <linux/clk.h>
#include "pinctrl-cv1835.h"

#include <linux/reset.h>
#include <linux/ctype.h>
#include <linux/version.h>
#ifdef CONFIG_PROC_FS
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/uaccess.h>
#endif

#include "cviusb_drd.h"
#include "io.h"
#include "otg_fsm.h"

static int force_fs;
module_param(force_fs, int, 0644);
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

static struct cviusb_dev *_cviusb;

/**
 * cviusb_set_mode - change mode of OTG Core
 * @cviusb: pointer to our context structure
 * @mode: selected mode from cviusb_role
 */
void cviusb_set_mode(struct cviusb_dev *cviusb, u32 mode)
{
	u32 reg;

	switch (mode) {
	case CVI_ROLE_GADGET:
		cviusb_writel(&cviusb->regs->OTGCMD,
				OTGCMD_TYPE__DEV_BUS_REQ__MASK
				| OTGCMD_TYPE__OTG_DIS__MASK);
		break;
	case CVI_ROLE_HOST:
		cviusb_writel(&cviusb->regs->OTGCMD,
				OTGCMD_TYPE__HOST_BUS_REQ__MASK
				| OTGCMD_TYPE__OTG_DIS__MASK);
		break;
	case CVI_ROLE_OTG:
		reg = cviusb_readl(&cviusb->regs->OTGCTRL1);
		cviusb_writel(&cviusb->regs->OTGCTRL1,
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
		cviusb_err(cviusb->dev, "Unsupported mode of operation %d\n",
			  mode);
		return;
	}
	cviusb->current_mode = mode;
}

/**
 * cviusb_core_init - Low-level initialization of Cvitek OTG Core
 * @cviusb: Pointer to our controller context structure
 *
 * Returns 0 on success otherwise negative errno.
 */
static int cviusb_core_init(struct cviusb_dev *cviusb)
{
	u32	reg;

	reg = cviusb_readl(&cviusb->regs->OTGVERSION);

	if ((OTGVERSION_TYPE__OTGVERSION__READ(reg)) != OTG_HW_VERSION) {
		cviusb_err(cviusb->dev, "this is not a Cvitek USB3 OTG Core\n");
		return -ENODEV;
	}
	cviusb->otg_version = OTGVERSION_TYPE__OTGVERSION__READ(reg);

	reg = cviusb_readl(&cviusb->regs->OTGSTS);
	if (OTGSTS_TYPE__OTG_NRDY__READ(reg) != 0) {
		cviusb_err(cviusb->dev, "Cvitek USB3 OTG device not ready\n");
		return -ENODEV;
	}

#ifdef USB_SIM_SPEED_UP
	cviusb_dbg(cviusb->dev, "Enable fast simulation timing modes\n");
	cviusb_writel(&cviusb->regs->OTGSIMULATE,
			OTGSIMULATE_TYPE__OTG_CFG_FAST_SIMS__MASK);
#endif
	return 0;
}

/**
 * cviusb_otg_fsm_sync - Get OTG events and sync it to OTG fsm
 * @cviusb: Pointer to our controller context structure
 */
void cviusb_otg_fsm_sync(struct cviusb_dev *cviusb)
{
	u32 reg;
	int id, vbus;

	reg = cviusb_readl(&cviusb->regs->OTGSTS);

	id = OTGSTS_TYPE__ID_VALUE__READ(reg);
	vbus = OTGSTS_TYPE__SESSION_VALID__READ(reg);

	cviusb->fsm->id = id;
	cviusb->fsm->b_sess_vld = vbus;
	cviusb->fsm->overcurrent =
		OTGIEN_TYPE__OVERCURRENT_INT_EN__READ(cviusb->otg_int_vector);

	if (OTGIEN_TYPE__SRP_NOT_COMP_DEV_REMOVED_INT_EN__READ(
			cviusb->otg_int_vector
			)) {
		cviusb->fsm->a_srp_det_not_compliant_dev = 0;
		cviusb_otg_disable_irq(cviusb,
			OTGIEN_TYPE__SRP_NOT_COMP_DEV_REMOVED_INT_EN__MASK);
	}

	if (cviusb->fsm->a_srp_det_not_compliant_dev)
		cviusb->fsm->a_srp_det = 0;
	else
		cviusb->fsm->a_srp_det =
			OTGIVECT_TYPE__SRP_DET_INT__READ(cviusb->otg_int_vector);

	if (OTGIVECT_TYPE__TB_AIDL_BDIS_MIN_TMOUT_INT__READ(
						cviusb->otg_int_vector))
		cviusb->fsm->b_aidl_bdis_tmout = 1;

#if IS_ENABLED(CONFIG_USB_CVITEK_A_BIDL_ADIS_HW_TMR)
	if (OTGIVECT_TYPE__TA_BIDL_ADIS_TMOUT_INT__READ(cviusb->otg_int_vector))
		cviusb->fsm->a_bidl_adis_tmout = 1;
#endif

	if (OTGIVECT_TYPE__TB_ASE0_BRST_TMOUT_INT__READ(cviusb->otg_int_vector))
		cviusb->fsm->b_ase0_brst_tmout = 1;

	usb_otg_sync_inputs(cviusb->fsm);
}

/**
 * cviusb_otg_standby_allowed - standby (aka slow reference clock)
 * is allowed only when both modes (host/device) are off
 * @cviusb: Pointer to our controller context structure
 *
 *  Returns 1 if allowed otherwise 0.
 */
int cviusb_otg_standby_allowed(struct cviusb_dev *cviusb)
{
	if ((cviusb->fsm->otg->state == OTG_STATE_B_IDLE) ||
			(cviusb->fsm->otg->state == OTG_STATE_A_IDLE))
		return 1;
	return 0;
}

/**
 * cviusb_otg_set_standby - set standby mode aka slow reference clock
 * @cviusb: Pointer to our controller context structure
 *
 * Returns 0 on success otherwise negative errno.
 */
int cviusb_otg_set_standby(struct cviusb_dev *cviusb)
{
	u32 reg;
	int phy_refclk_valid = 0;

	if (!cviusb_otg_standby_allowed(cviusb))
		return -EPERM;
	reg = cviusb_readl(&cviusb->regs->OTGREFCLK);
	cviusb_writel(&cviusb->regs->OTGREFCLK,
			OTGREFCLK_TYPE__OTG_STB_CLK_SWITCH_EN__SET(reg));
	/*
	 * signal from the PHY Reference Clock Control interface
	 * should fall to 0
	 */
	do {
		reg = cviusb_readl(&cviusb->regs->OTGSTATE);
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
 * cviusb_otg_clear_standby - switch off standby mode aka slow reference clock
 * @cviusb: Pointer to our controller context structure
 *
 * Returns 0 on success otherwise negative errno.
 */
int cviusb_otg_clear_standby(struct cviusb_dev *cviusb)
{
	u32 reg;
	int phy_refclk_valid = 0;

	reg = cviusb_readl(&cviusb->regs->OTGREFCLK);
	if (!OTGREFCLK_TYPE__OTG_STB_CLK_SWITCH_EN__READ(reg))
		/* Don't try to stop clock which has been already stopped */
		return -EPERM;
	cviusb_writel(&cviusb->regs->OTGREFCLK,
		OTGREFCLK_TYPE__OTG_STB_CLK_SWITCH_EN__CLR(reg));
	/*
	 * signal from the PHY Reference Clock Control interface
	 * should rise to 1
	 */
	do {
		reg = cviusb_readl(&cviusb->regs->OTGSTATE);
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
 * cviusb_otg_mask_irq - Mask all interrupts
 * @cviusb: Pointer to our controller context structure
 */
static void cviusb_otg_mask_irq(struct cviusb_dev *cviusb)
{
	cviusb_writel(&cviusb->regs->OTGIEN, 0);
}

/**
 * cviusb_otg_unmask_irq - Unmask id and sess_valid interrupts
 * @cviusb: Pointer to our controller context structure
 */
static void cviusb_otg_unmask_irq(struct cviusb_dev *cviusb)
{
	cviusb_writel(&cviusb->regs->OTGIEN,
			OTGIEN_TYPE__OTGSESSVALID_FALL_INT_EN__MASK
			| OTGIEN_TYPE__OTGSESSVALID_RISE_INT_EN__MASK
			| OTGIEN_TYPE__ID_CHANGE_INT_EN__MASK
			| OTGIEN_TYPE__OVERCURRENT_INT_EN__MASK
			| OTGIEN_TYPE__TIMER_TMOUT_INT_EN__MASK);
}

/**
 * cviusb_otg_enable_irq - enable desired interrupts
 * @cviusb: Pointer to our controller context structure
 * @irq: interrupt mask
 */
void cviusb_otg_enable_irq(struct cviusb_dev *cviusb, u32 irq)
{
	u32 reg;

	reg = cviusb_readl(&cviusb->regs->OTGIEN);
	cviusb_writel(&cviusb->regs->OTGIEN, reg | irq);
}

/**
 * cviusb_otg_disable_irq - disable desired interrupts
 * @cviusb: Pointer to our controller context structure
 * @irq: interrupt mask
 */
void cviusb_otg_disable_irq(struct cviusb_dev *cviusb, u32 irq)
{
	u32 reg;

	reg = cviusb_readl(&cviusb->regs->OTGIEN);
	cviusb_writel(&cviusb->regs->OTGIEN, reg & ~irq);
}

/**
 * cviusb_otg_irq - interrupt thread handler
 * @irq: interrupt number
 * @cviusb_ptr: Pointer to our controller context structure
 *
 * Returns IRQ_HANDLED on success.
 */
static irqreturn_t cviusb_otg_thread_irq(int irq, void *cviusb_ptr)
{
	struct cviusb_dev *cviusb = cviusb_ptr;
	unsigned long flags;

	spin_lock_irqsave(&cviusb->lock, flags);
	cviusb_otg_fsm_sync(cviusb);
	spin_unlock_irqrestore(&cviusb->lock, flags);

	return IRQ_HANDLED;
}

/**
 * cviusb_otg_irq - interrupt handler
 * @irq: interrupt number
 * @cviusb_ptr: Pointer to our controller context structure
 *
 * Returns IRQ_WAKE_THREAD on success otherwise IRQ_NONE.
 */
static irqreturn_t cviusb_otg_irq(int irq, void *cviusb_ptr)
{
	struct cviusb_dev *cviusb = cviusb_ptr;
	irqreturn_t ret = IRQ_NONE;
	u32 reg;

	spin_lock(&cviusb->lock);

	reg = cviusb_readl(&cviusb->regs->OTGIVECT);
	cviusb->otg_int_vector = reg;
	if (reg) {
		cviusb_writel(&cviusb->regs->OTGIVECT, reg);
		ret = IRQ_WAKE_THREAD;
	}

	spin_unlock(&cviusb->lock);

	return ret;
}

/**
 * cviusb_wait_for_ready - wait for host or gadget to be ready
 * for working
 * @cviusb: Pointer to our controller context structure
 * @otgsts_bit_ready: which bit should be monitored
 *
 * Returns 0 on success otherwise negative errno
 */
static int cviusb_wait_for_ready(struct cviusb_dev *cviusb, int otgsts_bit_ready)
{
	char ready = 0;
	u32 reg;

	do {
		/*
		 * TODO: it should be considered to add
		 * some timeout here and return error.
		 */
		reg = cviusb_readl(&cviusb->regs->OTGSTS);
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
 * cviusb_wait_for_idle - wait for host or gadget switched
 * to idle
 * @cviusb: Pointer to our controller context structure
 * @otgsts_bit_ready: which bit should be monitored
 *
 * Returns 0 on success otherwise negative errno
 */
static int cviusb_wait_for_idle(struct cviusb_dev *cviusb, int otgsts_bits_idle)
{
	char not_idle = 0;
	u32 reg;

	do {
		/*
		 * TODO: it should be considered to add
		 * some timeout here and return error.
		 */
		reg = cviusb_readl(&cviusb->regs->OTGSTATE);
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
#define UPCR_EXTERNAL_VBUS_EN_OFFSET			1U
#define UPCR_IDPAD_C_OW_OFFSET				(1U<<6)
#define UPCR_IDPAD_C_SW_OFFSET				(1U<<7)

#define UPCR_FORCE_FS_OFFSET				16

static int _cviusb_drd_override_id(struct cviusb_dev *cviusb, int value)
{
	cviusb_dbg(cviusb->dev, "%s: %d\n", __func__, value);

	if (cviusb->otg_bypass) {
		/* Enable idpad override. */
		u32 tmp = readl(cviusb->regs_usbPhyCtrl)
				| UPCR_IDPAD_C_OW_OFFSET;
		if (value)
			tmp |= UPCR_IDPAD_C_SW_OFFSET;
		else
			tmp &= ~UPCR_IDPAD_C_SW_OFFSET;

		writel(tmp, cviusb->regs_usbPhyCtrl);
		cviusb->id_override = value;
	}

	return 0;
}

int cviusb_drd_override_id(int value)
{
	struct cviusb_dev *cviusb = _cviusb;

	if (!cviusb)
		return -ENODEV;

	return _cviusb_drd_override_id(cviusb, value);
}
EXPORT_SYMBOL(cviusb_drd_override_id);

/**
 * cviusb_drd_start_host - start/stop host
 * @fsm: Pointer to our finite state machine
 * @on: 1 for start, 0 for stop
 *
 * Returns 0 on success otherwise negative errno
 */
static int cviusb_drd_start_host(struct otg_fsm *fsm, int on)
{
	struct device *dev = usb_otg_fsm_to_dev(fsm);
	struct cviusb_dev *cviusb = dev_get_drvdata(dev);
	int ret;

	cviusb_dbg(dev, "%s: %d\n", __func__, on);

	/* switch OTG core */
	if (on) {
		u32 value;

		writel(readl(cviusb->regs_usbPhyCtrl)
			| (1 << UPCR_EXTERNAL_VBUS_EN_OFFSET),
			cviusb->regs_usbPhyCtrl);

		value =	OTGCMD_TYPE__HOST_BUS_REQ__MASK
			| OTGCMD_TYPE__OTG_EN__MASK
			| OTGCMD_TYPE__A_DEV_EN__MASK;

		if (cviusb->ss_dis)
			value |= OTGCMD_TYPE__SS_HOST_DISABLED_SET__MASK;

		cviusb_writel(&cviusb->regs->OTGCMD, value);

		cviusb_dbg(cviusb->dev, "Waiting for XHC...\n");

		ret = cviusb_wait_for_ready(cviusb,
					   OTGSTS_TYPE__XHC_READY__SHIFT);
		if (ret)
			return ret;

		if (force_fs) {
			cviusb_dbg(cviusb->dev, "Force FS Enable\n");
			writel(readl(cviusb->regs_forcefs)
				| (1 << UPCR_FORCE_FS_OFFSET),
				cviusb->regs_forcefs);
		} else {
			cviusb_dbg(cviusb->dev, "Force FS Disable\n");
			writel(readl(cviusb->regs_forcefs)
				& ~(1 << UPCR_FORCE_FS_OFFSET),
				cviusb->regs_forcefs);
		}

		/* start the HCD */
		usb_otg_start_host(fsm, true);
	} else {
		/* stop the HCD */
		usb_otg_start_host(fsm, false);

		/* stop OTG */
		cviusb_writel(&cviusb->regs->OTGCMD,
				OTGCMD_TYPE__HOST_BUS_DROP__MASK
				| OTGCMD_TYPE__HOST_POWER_OFF__MASK
				| OTGCMD_TYPE__DEV_BUS_DROP__MASK
				| OTGCMD_TYPE__DEV_POWER_OFF__MASK);
		ret = cviusb_wait_for_idle(cviusb,
				OTGSTATE_TYPE__HOST_OTG_STATE__MASK);
		writel(readl(cviusb->regs_usbPhyCtrl)
			& ~(1 << UPCR_EXTERNAL_VBUS_EN_OFFSET),
			cviusb->regs_usbPhyCtrl);
	}

	return 0;
}

/**
 * cviusb_drd_start_gadget - start/stop gadget
 * @fsm: Pointer to our finite state machine
 * @on: 1 for start, 0 for stop
 *
 * Returns 0 on success otherwise negative errno
 */
static int cviusb_drd_start_gadget(struct otg_fsm *fsm, int on)
{
	struct device *dev = usb_otg_fsm_to_dev(fsm);
	struct cviusb_dev *cviusb = dev_get_drvdata(dev);
	int ret;

	cviusb_dbg(dev, "%s: %d\n", __func__, on);

	/* switch OTG core */
	if (on) {
		u32 value;

		value = OTGCMD_TYPE__DEV_BUS_REQ__MASK
			| OTGCMD_TYPE__OTG_EN__MASK
			| OTGCMD_TYPE__A_DEV_DIS__MASK;

		if (cviusb->otg_bypass)
			value |= OTGCMD_TYPE__A_SESS_USE_CLR__MASK;

		if (cviusb->ss_dis)
			value |= OTGCMD_TYPE__SS_PERIPH_DISABLED_SET__MASK;

		cviusb_writel(&cviusb->regs->OTGCMD, value);

		cviusb_dbg(dev, "Waiting for CVI_GADGET...\n");
		ret = cviusb_wait_for_ready(cviusb,
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
		cviusb_writel(&cviusb->regs->OTGCMD,
				OTGCMD_TYPE__HOST_BUS_DROP__MASK
				| OTGCMD_TYPE__HOST_POWER_OFF__MASK
				| OTGCMD_TYPE__DEV_BUS_DROP__MASK
				| OTGCMD_TYPE__DEV_POWER_OFF__MASK);
		ret = cviusb_wait_for_idle(cviusb,
				OTGSTATE_TYPE__DEV_OTG_STATE__MASK);
	}

	return 0;
}

static struct otg_fsm_ops cviusb_drd_ops = {
	.start_host = cviusb_drd_start_host,
	.start_gadget = cviusb_drd_start_gadget,
};

/**
 * cviusb_drd_register - register out drd controller
 * into otg framework
 * @cviusb: Pointer to our controller context structure
 *
 * Returns 0 on success otherwise negative errno
 */
static int cviusb_drd_register(struct cviusb_dev *cviusb)
{
	int ret;

	/* register parent as DRD device with OTG core */
	cviusb->fsm = usb_otg_register(cviusb->dev,
			&cviusb->otg_config, cviusb_usb_otg_work);
	if (IS_ERR(cviusb->fsm)) {
		ret = PTR_ERR(cviusb->fsm);
		if (ret == -ENOTSUPP)
			cviusb_err(cviusb->dev, "CONFIG_USB_OTG needed for dual-role\n");
		else
			cviusb_err(cviusb->dev, "Failed to register with OTG core\n");

		return ret;
	}

	/*
	 * The memory of host_req_flag should be allocated by
	 * controller driver, otherwise, hnp polling is not started.
	 */
	cviusb->fsm->host_req_flag =
		kmalloc(sizeof(*cviusb->fsm->host_req_flag), GFP_KERNEL);
	if (!cviusb->fsm->host_req_flag)
		return -ENOTSUPP;

	INIT_DELAYED_WORK(&cviusb->fsm->hnp_polling_work, otg_hnp_polling_work);

	return 0;
}

int cviusb_global_synchro_timer_setup(struct cviusb_dev *cviusb)
{
	/* set up timer for fsm synchronization */
#ifdef USB_SIM_SPEED_UP
	cviusb_writel(&cviusb->regs->OTGTMR,
		    OTGTMR_TYPE__TIMER_WRITE__MASK |
		    OTGTMR_TYPE__TIMEOUT_UNITS__WRITE(0) |
		    OTGTMR_TYPE__TIMEOUT_VALUE__WRITE(1)
		);
#else
	cviusb_writel(&cviusb->regs->OTGTMR,
		    OTGTMR_TYPE__TIMER_WRITE__MASK |
		    OTGTMR_TYPE__TIMEOUT_UNITS__WRITE(2) |
		    OTGTMR_TYPE__TIMEOUT_VALUE__WRITE(1)
		);
#endif
	return 0;
}

int cviusb_global_synchro_timer_start(struct cviusb_dev *cviusb)
{
	cviusb_writel(&cviusb->regs->OTGTMR,
		    cviusb_readl(&cviusb->regs->OTGTMR) |
		    OTGTMR_TYPE__TIMER_START__MASK);
	return 0;
}

int cviusb_global_synchro_timer_stop(struct cviusb_dev *cviusb)
{
	cviusb_writel(&cviusb->regs->OTGTMR,
		    cviusb_readl(&cviusb->regs->OTGTMR) |
		    OTGTMR_TYPE__TIMER_STOP__MASK);
	return 0;
}


/**
 * cviusb_drd_init - initialize our drd controller
 * @cviusb: Pointer to our controller context structure
 *
 * Returns 0 on success otherwise negative errno
 */
static int cviusb_drd_init(struct cviusb_dev *cviusb)
{
	int ret;
	struct usb_otg_caps *otgcaps = &cviusb->otg_config.otg_caps;
	unsigned long flags;
	u32 reg;

	reg = cviusb_readl(&cviusb->regs->OTGCAPABILITY);
	otgcaps->otg_rev = OTGCAPABILITY_TYPE__OTG2REVISION__READ(reg);
	otgcaps->hnp_support = OTGCAPABILITY_TYPE__HNP_SUPPORT__READ(reg);
	otgcaps->srp_support = OTGCAPABILITY_TYPE__SRP_SUPPORT__READ(reg);
	otgcaps->adp_support = OTGCAPABILITY_TYPE__ADP_SUPPORT__READ(reg);

	/* Update otg capabilities by DT properties */
	ret = of_usb_update_otg_caps(cviusb->dev->of_node, otgcaps);
	if (ret)
		return ret;

	cviusb->otg_config.fsm_ops = &cviusb_drd_ops;

	cviusb_dbg(cviusb->dev, "rev:0x%x, hnp:%d, srp:%d, adp:%d\n",
			otgcaps->otg_rev, otgcaps->hnp_support,
			otgcaps->srp_support, otgcaps->adp_support);

	ret = cviusb_drd_register(cviusb);
	if (ret)
		goto error0;

	/* global fsm synchronization timer */
	cviusb_global_synchro_timer_setup(cviusb);
	/* cviusb_global_synchro_timer_start(cviusb); */

	/* disable all irqs */
	cviusb_otg_mask_irq(cviusb);
	/* clear all interrupts */
	cviusb_writel(&cviusb->regs->OTGIVECT, ~0);

	ret = devm_request_threaded_irq(cviusb->dev, cviusb->otg_irq,
					cviusb_otg_irq,
					cviusb_otg_thread_irq,
					IRQF_SHARED, "cviusb-otg", cviusb);
	if (ret) {
		cviusb_err(cviusb->dev, "failed to request irq #%d --> %d\n",
			cviusb->otg_irq, ret);
		ret = -ENODEV;
		goto error1;
	}

	/* we need to set OTG to get events from OTG core */
	cviusb_set_mode(cviusb, CVI_ROLE_OTG);
	if (cviusb->otg_bypass)
		cviusb_set_state(cviusb->fsm, OTG_STATE_A_HOST);

	spin_lock_irqsave(&cviusb->lock, flags);

	/* Enable ID ans sess_valid event interrupt */
	cviusb_otg_unmask_irq(cviusb);

	spin_unlock_irqrestore(&cviusb->lock, flags);

	cviusb_otg_fsm_sync(cviusb);
	usb_otg_sync_inputs(cviusb->fsm);

	return 0;

error1:
	usb_otg_unregister(cviusb->dev);
error0:
	return ret;
}

/**
 * cviusb_drd_exit - unregister and clean up drd controller
 * @cviusb: Pointer to our controller context structure
 */
static void cviusb_drd_exit(struct cviusb_dev *cviusb)
{
	usb_otg_unregister(cviusb->dev);
}

/**
 * cviusb_core_init_mode - initialize mode of operation
 * @cviusb: Pointer to our controller context structure
 *
 * Returns 0 on success otherwise negative errno
 */
static int cviusb_core_init_mode(struct cviusb_dev *cviusb)
{
	int ret;

	switch (cviusb->dr_mode) {
	case USB_DR_MODE_PERIPHERAL:
		cviusb_set_mode(cviusb, CVI_ROLE_GADGET);
		break;
	case USB_DR_MODE_HOST:
		cviusb_set_mode(cviusb, CVI_ROLE_HOST);
		break;
	case USB_DR_MODE_OTG:
		ret = cviusb_drd_init(cviusb);
		if (ret) {
			cviusb_err(cviusb->dev, "limiting to peripheral only\n");
			cviusb->dr_mode = USB_DR_MODE_PERIPHERAL;
			cviusb_set_mode(cviusb, CVI_ROLE_GADGET);
		}
		break;
	default:
		cviusb_err(cviusb->dev, "Unsupported mode of operation %d\n",
				cviusb->dr_mode);
		return -EINVAL;
	}

	return 0;
}

/**
 * cviusb_core_exit_mode - clean up drd controller
 * @cviusb: Pointer to our controller context structure
 */
static void cviusb_core_exit_mode(struct cviusb_dev *cviusb)
{
	switch (cviusb->dr_mode) {
	case USB_DR_MODE_PERIPHERAL:
		break;
	case USB_DR_MODE_HOST:
		break;
	case USB_DR_MODE_OTG:
		cviusb_drd_exit(cviusb);
		break;
	default:
		/* do nothing */
		break;
	}
}

static int vbus_is_present(struct cviusb_dev *cviusb)
{
	if (gpio_is_valid(cviusb->vbus_pin))
		return gpio_get_value(cviusb->vbus_pin) ^
			cviusb->vbus_pin_inverted;

	/* No Vbus detection: Assume always present */
	return 1;
}

static irqreturn_t vbus_irq_handler(int irq, void *devid)
{
	struct cviusb_dev *cviusb = devid;
	u32 reg;
	int vbus, id;

	reg = cviusb_readl(&cviusb->regs->OTGSTS);
	id = OTGSTS_TYPE__ID_VALUE__READ(reg);
	vbus = vbus_is_present(cviusb);
	/* do nothing if we are an A-device (vbus provider). */
	if (id == 0)
		return IRQ_HANDLED;
	cviusb_dbg(cviusb->dev, "vbus int = %d\n", vbus);
	return IRQ_WAKE_THREAD;
}

static irqreturn_t vbus_irq_thread(int irq, void *devid)
{
	struct cviusb_dev *cviusb = devid;
	struct usb_gadget *gadget;
	int vbus;

	if (!cviusb->fsm)
		return IRQ_HANDLED;
	if (!cviusb->fsm->otg)
		return IRQ_HANDLED;
	gadget = cviusb->fsm->otg->gadget;
	if (!gadget)
		return IRQ_HANDLED;

	/* debounce */
	udelay(10);
	vbus = vbus_is_present(cviusb);
	if (cviusb->pre_vbus_status != vbus) {
		cviusb_dbg(cviusb->dev, "vbus thread = %d\n", vbus);
		usb_udc_vbus_handler(gadget, (vbus != 0));
		cviusb->pre_vbus_status = vbus;
	}
	return IRQ_HANDLED;
}

#define PHY_REG(x)	(x)

#define REG014				PHY_REG(0x014)
#define REG014_UTMI_RESET		(1 << 8)
#define REG014_DMPULLDOWN		(1 << 7)
#define REG014_DPPULLDOWN		(1 << 6)
#define REG014_TERMSEL			(1 << 5)
#define REG014_XCVRSEL_MASK		(0x3 << 3)
#define REG014_XCVRSEL_SHIFT		3
#define REG014_OPMODE_MASK		(0x3 << 1)
#define REG014_OPMODE_SHIFT		1
#define REG014_UTMI_OVERRIDE		(1 << 0)

#define REG020				PHY_REG(0x020)
#define REG020_DP_DET			(1 << 17)
#define REG020_CHG_DET			(1 << 16)
#define REG020_VDM_SRC_EN		(1 << 5)
#define REG020_VDP_SRC_EN		(1 << 4)
#define REG020_DM_CMP_EN		(1 << 3)
#define REG020_DP_CMP_EN		(1 << 2)
#define REG020_DCD_EN			(1 << 1)
#define REG020_BC_EN			(1 << 0)

#ifdef CONFIG_PROC_FS
#define CVIUSB_CHGDET_PROC_NAME "cviusb/chg_det"

static struct proc_dir_entry *cviusb_proc_dir;
static struct proc_dir_entry *cviusb_chgdet_proc_entry;

#define TDCD_TIMEOUT_MAX	900	//ms
#define TDCD_TIMEOUT_MIN	300	//ms
#define TDCD_DBNC		10	//ms
#define TVDMSRC_EN		20	//ms
#define TVDPSRC_ON		40	//ms
#define TVDMSRC_ON		40	//ms

static u8 *dcd_en[] = {
	"dcd_off",
	"dcd_on",
};

static u8 *chg_port[CHGDET_NUM] = {
	"sdp",
	"dcp",
	"cdp",
};

static void utmi_chgdet_prepare(struct cviusb_dev *cviusb)
{
	cviusb_writel(cviusb->regs_usb20phy + REG014,
			REG014_UTMI_OVERRIDE |
			(REG014_OPMODE_MASK & (0x1 << REG014_OPMODE_SHIFT)) |
			(REG014_XCVRSEL_MASK & (0x1 << REG014_XCVRSEL_SHIFT)));
}

static void utmi_reset(struct cviusb_dev *cviusb)
{
	cviusb_writel(cviusb->regs_usb20phy + REG014, 0);
}

static void dcd_det(struct cviusb_dev *cviusb)
{
	int cnt = 0;
	u32 dbnc = 0;

	/* 1. utmi prepare */
	utmi_chgdet_prepare(cviusb);
	/* 2. Enable bc and dcd*/
	cviusb_writel(cviusb->regs_usb20phy + REG020, REG020_BC_EN | REG020_DCD_EN);
	/* 3. DCD det in 900ms */
	while (cnt++ < TDCD_TIMEOUT_MAX) {
		if (cviusb_readl(&cviusb->regs->PAD2_26) & BIT(14))
			dbnc += 1;
		else
			dbnc = 0;
		if (dbnc >= TDCD_DBNC)
			break;
		usleep_range(1000, 1010);
	}
	/* 4. Disable bc dcd. */
	cviusb_writel(cviusb->regs_usb20phy + REG020, 0);
	/* 5. utmi reset */
	utmi_reset(cviusb);
	usleep_range(1000, 1010);
}

static int chg_det(struct cviusb_dev *cviusb)
{
	int cnt = 0;
	int det = 0;
	u32 reg;

	/* 1. Enable bc */
	cviusb_writel(cviusb->regs_usb20phy + REG020,
			REG020_BC_EN | REG020_VDP_SRC_EN | REG020_DM_CMP_EN);
	/* need 2ms delay to avoid the unstable value on DM CMP. */
	usleep_range(2000, 2010);
	/* 2. Dm det in 40ms */
	while (cnt++ < TVDPSRC_ON) {
		reg = cviusb_readl(cviusb->regs_usb20phy + REG020);
		if (reg & REG020_CHG_DET)
			det = 1;
		if (!det && cnt > TVDMSRC_EN)
			break;
		usleep_range(1000, 1010);
	}
	/* 3. Disable bc. */
	cviusb_writel(cviusb->regs_usb20phy + REG020, 0);

	return det;
}

static int cdp_det(struct cviusb_dev *cviusb)
{
	int cnt = 0;
	int det = 0;

	/* 1. Enable bc */
	cviusb_writel(cviusb->regs_usb20phy + REG020,
			REG020_BC_EN | REG020_VDM_SRC_EN | REG020_DP_CMP_EN);
	usleep_range(1000, 1010);
	/* 2. Dp det in 40ms */
	while (cnt++ < TVDMSRC_ON) {
		if ((cviusb_readl(cviusb->regs_usb20phy + REG020) & REG020_DP_DET))
			det = 1;
		/* 5ms for 2nd detection. */
		if (!det && cnt > 5)
			break;
		usleep_range(1000, 1010);
	}
	/* 3. Disable bc. */
	cviusb_writel(cviusb->regs_usb20phy + REG020, 0);

	return !det;
}

static int proc_chgdet_show(struct seq_file *m, void *v)
{
	struct cviusb_dev *cviusb = (struct cviusb_dev *)m->private;

	if (!cviusb->id_override)
		return -EPERM;

	/* Run dcd detection or wait TDCD_TIMEOUT_MIN. */
	if (cviusb->dcd_dis)
		msleep(TDCD_TIMEOUT_MIN);
	else
		dcd_det(cviusb);
	/* Run chgdet */
	if (chg_det(cviusb)) {
		usleep_range(1000, 1010);
		if (cdp_det(cviusb))
			cviusb->chgdet = CHGDET_CDP;
		else
			cviusb->chgdet = CHGDET_DCP;
	} else
		cviusb->chgdet = CHGDET_SDP;

	/* Run dcpdet */
	seq_printf(m, "%s\n", chg_port[cviusb->chgdet]);

	return 0;
}

static int dcd_en_hdler(struct cviusb_dev *cviusb, char const *input)
{
	u32 num;
	u8 str[80] = {0};
	u8 t = 0;
	u8 i, n;
	u8 *p;

	num = sscanf(input, "%s", str);
	if (num > 1) {
		return -EINVAL;
	}

	/* convert to lower case for following type compare */
	p = str;
	for (; *p; ++p)
		*p = tolower(*p);
	n = ARRAY_SIZE(dcd_en);
	for (i = 0; i < n; i++) {
		if (!strcmp(str, dcd_en[i])) {
			t = i;
			break;
		}
	}
	if (i == n)
		return -EINVAL;

	switch (t) {
	case 0:
		/* dcd off */
		cviusb->dcd_dis = 1;
		break;
	case 1:
		/* dcd on */
		cviusb->dcd_dis = 0;
		break;
	default:
		break;
	}

	return 0;
}

static ssize_t chgdet_proc_write(struct file *file, const char __user *user_buf, size_t count, loff_t *ppos)
{
	struct cviusb_dev *cviusb = PDE_DATA(file_inode(file));

	dcd_en_hdler(cviusb, user_buf);

	return count;
}

static int proc_chgdet_open(struct inode *inode, struct file *file)
{
	struct cviusb_dev *cviusb = PDE_DATA(inode);

	return single_open(file, proc_chgdet_show, cviusb);
}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
static const struct proc_ops chgdet_proc_fops = {
	.proc_open = proc_chgdet_open,
	.proc_read = seq_read,
	.proc_write = chgdet_proc_write,
	.proc_lseek = seq_lseek,
	.proc_release = single_release,
};
#else
static const struct file_operations chgdet_proc_fops = {
	.owner		= THIS_MODULE,
	.open		= proc_chgdet_open,
	.read		= seq_read,
	.write		= chgdet_proc_write,
	.llseek		= seq_lseek,
	.release	= single_release,
};
#endif

#endif

/**
 * cviusb_probe - bind our drd driver
 * @pdev: Pointer to Linux platform device
 *
 * Returns 0 on success otherwise negative errno
 */
static int cviusb_probe(struct platform_device *pdev)
{
	struct device		*dev = &pdev->dev;
	struct resource		*res;
	struct cviusb_dev		*cviusb;
	int			ret;
	u32			status;
	void __iomem		*regs;
	void			*mem;
	enum of_gpio_flags	flags;
	const char		*tmp_char;
	uint32_t		use_vbus = 0;

	cviusb_dbg(dev, "Cvitek usb driver: probe()\n");

	mem = devm_kzalloc(dev, sizeof(*cviusb) + CVI_ALIGN_MASK, GFP_KERNEL);
	if (!mem)
		return -ENOMEM;

	cviusb = PTR_ALIGN(mem, CVI_ALIGN_MASK + 1);
	cviusb->mem = mem;
	cviusb->dev = dev;

	/* init and enable usb clk */
	cviusb->clk_axi.clk_o = devm_clk_get(&pdev->dev, "clk_axi");
	if (cviusb->clk_axi.clk_o) {
		clk_prepare_enable(cviusb->clk_axi.clk_o);
		cviusb->clk_axi.is_on = 1;
		dev_info(&pdev->dev, "axi clk installed\n");
	}
	cviusb->clk_apb.clk_o = devm_clk_get(&pdev->dev, "clk_apb");
	if (cviusb->clk_apb.clk_o) {
		clk_prepare_enable(cviusb->clk_apb.clk_o);
		cviusb->clk_apb.is_on = 1;
		dev_info(&pdev->dev, "apb clk installed\n");
	}
	cviusb->clk_125m.clk_o = devm_clk_get(&pdev->dev, "clk_125m");
	if (cviusb->clk_125m.clk_o) {
		clk_prepare_enable(cviusb->clk_125m.clk_o);
		cviusb->clk_125m.is_on = 1;
		dev_info(&pdev->dev, "125m clk installed\n");
	}
	cviusb->clk_33k.clk_o = devm_clk_get(&pdev->dev, "clk_33k");
	if (cviusb->clk_33k.clk_o) {
		clk_prepare_enable(cviusb->clk_33k.clk_o);
		cviusb->clk_33k.is_on = 1;
		dev_info(&pdev->dev, "33k clk installed\n");
	}
	cviusb->clk_12m.clk_o = devm_clk_get(&pdev->dev, "clk_12m");
	if (cviusb->clk_12m.clk_o) {
		clk_prepare_enable(cviusb->clk_12m.clk_o);
		cviusb->clk_12m.is_on = 1;
		dev_info(&pdev->dev, "12m clk installed\n");
	}

	res = platform_get_resource(pdev, IORESOURCE_IRQ, 0);
	if (!res) {
		cviusb_err(dev, "missing IRQ\n");
		return -ENODEV;
	}
	cviusb->otg_irq = res->start;

	/*
	 * Request memory region
	 */
	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	regs = devm_ioremap_resource(dev, res);
	if (IS_ERR(regs)) {
		ret = PTR_ERR(regs);
		goto err0;
	}

	cviusb->regs	= regs;
	cviusb->regs_size	= resource_size(res);

	cviusb->usb_reset = devm_reset_control_get(&pdev->dev, "usb");

	res = platform_get_resource(pdev, IORESOURCE_MEM, 1);
	regs = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(regs)) {
		ret = PTR_ERR(regs);
		goto err0;
	}
	cviusb->regs_addrRemap = regs;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 2);
	regs = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(regs)) {
		ret = PTR_ERR(regs);
		goto err0;
	}
	cviusb->regs_ctrl0 = regs;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 3);
	regs = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(regs)) {
		ret = PTR_ERR(regs);
		goto err0;
	}
	cviusb->regs_usbPhyCtrl = regs;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 4);
	regs = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(regs)) {
		ret = PTR_ERR(regs);
		goto err0;
	}
	cviusb->regs_usb20phy = regs;

	cviusb->regs_forcefs = ioremap(0x040D8130, 0x4);

	status = cviusb_readl(&cviusb->regs->OTGSTS);

	platform_set_drvdata(pdev, cviusb);
#if IS_ENABLED(CONFIG_USB_CVITEK_MISC)
	cviusb_drd_misc_register(cviusb, res->start);
#endif
	spin_lock_init(&cviusb->lock);

	cviusb->vbus_pin = of_get_named_gpio_flags(pdev->dev.of_node,
				"vbus-gpio", 0, &flags);
	cviusb->vbus_pin_inverted = (flags & OF_GPIO_ACTIVE_LOW) ? 1 : 0;
	cviusb_dbg(cviusb->dev, "vbus_pin = %d, flags = %d\n",
			cviusb->vbus_pin, cviusb->vbus_pin_inverted);
	if (gpio_is_valid(cviusb->vbus_pin)) {
		PINMUX_CONFIG(USB_VBUS_DET, XGPIOB_25);
		if (!devm_gpio_request(&pdev->dev,
			cviusb->vbus_pin, "cviusb-otg")) {
			irq_set_status_flags(gpio_to_irq(cviusb->vbus_pin),
					IRQ_NOAUTOEN);
			ret = devm_request_threaded_irq(&pdev->dev,
					gpio_to_irq(cviusb->vbus_pin),
					vbus_irq_handler,
					vbus_irq_thread,
					IRQF_TRIGGER_RISING |
					IRQF_TRIGGER_FALLING,
					"cviusb-vbus", cviusb);
			if (ret) {
				cviusb->vbus_pin = -ENODEV;
				cviusb_err(cviusb->dev,
					"failed to request vbus irq\n");
			} else {
				cviusb->pre_vbus_status = vbus_is_present(cviusb);
				use_vbus = 1;
				cviusb_dbg(cviusb->dev,
					"enable vbus irq, vbus status = %d\n",
					cviusb->pre_vbus_status);
			}
		} else {
			/* gpio_request fail so use -EINVAL for gpio_is_valid */
			cviusb->vbus_pin = -EINVAL;
			cviusb_err(cviusb->dev, "request gpio fail!\n");
		}
	}
	/* register for address 39to32 of usb */
	writel((readl(cviusb->regs_addrRemap)
		& (~DAMR_REG_USB_REMAP_ADDR_39_32_MASK))
		| (1 << DAMR_REG_USB_REMAP_ADDR_39_32_OFFSET),
		cviusb->regs_addrRemap);

	reset_control_assert(cviusb->usb_reset);
	udelay(50);

	/* bypass OTG and start as host? */
	ret = device_property_read_string(dev, "otg_bypass", &tmp_char);
	if (ret < 0) {
		cviusb->otg_bypass = 0;
	} else {
		if (!strcmp(tmp_char, "true"))
			cviusb->otg_bypass = 1;
		else
			cviusb->otg_bypass = 0;
	}

	/* Controller initially configured as Host */
	writel((cviusb->otg_bypass ? UCR_MODE_STRAP_HOST : UCR_MODE_STRAP_NON)
		| (1 << UCR_PORT_OVER_CURRENT_ACTIVE_OFFSET),
		cviusb->regs_ctrl0);
	/* Enable vbus when host. */
	if (cviusb->otg_bypass) {
		writel(readl(cviusb->regs_usbPhyCtrl)
			| (1 << UPCR_EXTERNAL_VBUS_EN_OFFSET),
			cviusb->regs_usbPhyCtrl);
	}

	reset_control_deassert(cviusb->usb_reset);
	udelay(50);
	/* External VBUS valid. (external_vbusvalid) */
	writel(readl(cviusb->regs_usbPhyCtrl)
		| (1 << UPCR_EXTERNAL_VBUS_VALID_OFFSET),
		cviusb->regs_usbPhyCtrl);
	/* Enable idpad override and set to 0 if support otg_bypass. */
	_cviusb_drd_override_id(cviusb, 0);
	/* Support super speed. */
	ret = device_property_read_string(dev, "dis_ss", &tmp_char);
	if (ret < 0) {
		cviusb->ss_dis = 0;
	} else {
		if (!strcmp(tmp_char, "true"))
			cviusb->ss_dis = 1;
		else
			cviusb->ss_dis = 0;
	}

	/* device tree set dr_mode */
	cviusb->dr_mode = usb_get_dr_mode(dev);
	if (cviusb->dr_mode == USB_DR_MODE_UNKNOWN) {
		cviusb->strap = OTGSTS_TYPE__STRAP__READ(status);
		if (cviusb->strap == STRAP_HOST)
			cviusb->dr_mode = USB_DR_MODE_HOST;
		else if (cviusb->strap == STRAP_GADGET)
			cviusb->dr_mode = USB_DR_MODE_PERIPHERAL;
		else if (cviusb->strap == STRAP_HOST_OTG)
			cviusb->dr_mode = USB_DR_MODE_OTG;
		else {
			cviusb_err(dev, "No default configuration, configuring as OTG.");
			cviusb->dr_mode = USB_DR_MODE_OTG;
		}
	}

	ret = cviusb_core_init(cviusb);
	if (ret) {
		cviusb_err(dev, "failed to initialize core\n");
		goto err0;
	}

	ret = cviusb_core_init_mode(cviusb);
	if (ret)
		goto err1;

	if (use_vbus)
		enable_irq(gpio_to_irq(cviusb->vbus_pin));
	_cviusb = cviusb;

#ifdef CONFIG_PROC_FS
	cviusb_proc_dir = proc_mkdir("cviusb", NULL);
	cviusb_chgdet_proc_entry = proc_create_data(CVIUSB_CHGDET_PROC_NAME, 0644, NULL,
					  &chgdet_proc_fops, cviusb);
	if (!cviusb_chgdet_proc_entry)
		cviusb_err(dev, "cviusb: can't create chgdet procfs.\n");
#endif

	return 0;

err1:
	cviusb_core_exit_mode(cviusb);
err0:
	return ret;
}

/**
 * cviusb_remove - unbind our drd driver and clean up
 * @pdev: Pointer to Linux platform device
 *
 * Returns 0 on success otherwise negative errno
 */
static int cviusb_remove(struct platform_device *pdev)
{
	struct cviusb_dev	*cviusb = platform_get_drvdata(pdev);

	kfree(cviusb->fsm->host_req_flag);
	cancel_delayed_work(&cviusb->fsm->hnp_polling_work);
#if !IS_ENABLED(CONFIG_USB_CVITEK_A_BIDL_ADIS_HW_TMR)
	cancel_delayed_work(&(cviusb->a_bidl_adis_data.a_bidl_adis_work));
#endif

	cviusb_core_exit_mode(cviusb);

	if (cviusb->clk_axi.clk_o) {
		clk_disable_unprepare(cviusb->clk_axi.clk_o);
		dev_info(&pdev->dev, "axi clk disable\n");
	}
	if (cviusb->clk_apb.clk_o) {
		clk_disable_unprepare(cviusb->clk_apb.clk_o);
		dev_info(&pdev->dev, "apb clk disable\n");
	}
	if (cviusb->clk_125m.clk_o) {
		clk_disable_unprepare(cviusb->clk_125m.clk_o);
		dev_info(&pdev->dev, "125m clk disable\n");
	}
	if (cviusb->clk_33k.clk_o) {
		clk_disable_unprepare(cviusb->clk_33k.clk_o);
		dev_info(&pdev->dev, "33k clk disable\n");
	}
	if (cviusb->clk_12m.clk_o) {
		clk_disable_unprepare(cviusb->clk_12m.clk_o);
		dev_info(&pdev->dev, "12m clk disable\n");
	}

	iounmap(cviusb->regs_forcefs);
#ifdef CONFIG_PROC_FS
	proc_remove(cviusb_chgdet_proc_entry);
	proc_remove(cviusb_proc_dir);
	cviusb_proc_dir = NULL;
	cviusb_chgdet_proc_entry = NULL;
#endif

	kfree(cviusb);
	_cviusb = NULL;

	return 0;
}

#ifdef CONFIG_OF
static const struct of_device_id of_cviusb_match[] = {
	{ .compatible = "cvitek,usb-otg" },
	{ },
};
MODULE_DEVICE_TABLE(of, of_cviusb_match);
#endif

static struct platform_driver cviusb_driver = {
	.probe		= cviusb_probe,
	.remove		= cviusb_remove,
	.driver		= {
		.name	= "cviusb-otg",
		.of_match_table	= of_match_ptr(of_cviusb_match),
	},
};

module_platform_driver(cviusb_driver);

MODULE_ALIAS("platform:cviusb");
MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("Cvitek USB3 DRD Controller Driver");
