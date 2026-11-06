#ifndef __DRIVERS_USB_CVI_CORE_H
#define __DRIVERS_USB_CVI_CORE_H

#include <linux/usb/otg.h>
#include <linux/usb/otg-fsm.h>

#include <linux/phy/phy.h>

#if IS_ENABLED(CONFIG_USB_CVITEK_MISC)
#include "cviusb_misc.h"
#endif

#include "drd_regs_map.h"
#include "debug.h"

#define OTG_HW_VERSION		0x100

/* STRAP 14:12 */
#define STRAP_NO_DEFAULT_CFG	0x00
#define STRAP_HOST_OTG		0x01
#define STRAP_HOST		0x02
#define STRAP_GADGET		0x04

#define A_HOST_SUSPEND		0x06
#define CVI_ALIGN_MASK		(16 - 1)

enum cviusb_role {
	CVI_ROLE_HOST = 0,
	CVI_ROLE_GADGET,
	CVI_ROLE_OTG,
	CVI_ROLE_END,
};

#if !IS_ENABLED(CONFIG_USB_CVITEK_A_BIDL_ADIS_HW_TMR)
struct a_bidl_adis_worker_data {
	int worker_running;
	struct otg_fsm *fsm;
	struct delayed_work a_bidl_adis_work;
};
#endif

enum CHG_PORT_E {
	CHGDET_SDP,	/* standard downstream port. */
	CHGDET_DCP,	/* dedicated charging port. */
	CHGDET_CDP,	/* charging downstream port. */
	CHGDET_NUM
};

struct cvi_usb_clk {
	int				is_on;
	struct clk			*clk_o;
};

/**
 * struct cviusb - Representation of Cvitek DRD OTG controller.
 * @lock: for synchronizing
 * @dev: pointer to Cvitek device struct
 * @xhci: pointer to xHCI child
 * @fsm: pointer to FSM structure
 * @otg_config: OTG controller configuration
 * @regs: pointer to base of OTG registers
 * @regs_size: size of OTG registers
 * @dr_mode: definition of OTG modes of operations
 * @otg_protocol: current OTG mode of operation
 * @otg_irq: number of OTG IRQs
 * @current_mode: current mode of operation written to PRTCAPDIR
 * @otg_version: version of OTG
 * @strap: strapped mode
 * @ss_en: super speed support
 * @otg_int_vector: OTG interrupt vector
 * @mem: points to start of memory which is used for this struct
 * @cviusb_misc: misc device for userspace communication
 * @a_bidl_adis_data: structure for a_bidl_adis timer implementation
 */
struct cviusb_dev {
	/* device lock */
	spinlock_t		lock;

	struct device		*dev;

	struct platform_device	*xhci;

	struct otg_fsm		*fsm;
	struct usb_otg_config	otg_config;

	struct usbdrd_register_block_type __iomem *regs;
	size_t			regs_size;

	struct reset_control *usb_reset;
	void __iomem		*regs_addrRemap;
	void __iomem		*regs_ctrl0;
	void __iomem		*regs_usbPhyCtrl;
	void __iomem		*regs_forcefs;
	void __iomem		*regs_usb20phy;

	enum usb_dr_mode	dr_mode;

	int			otg_protocol;

	int			otg_irq;
	u32			current_mode;
	u16			otg_version;
	u8			strap;
	u8			ss_dis;
	u32			otg_bypass;
	u32			otg_int_vector;
	void			*mem;

	int vbus_pin;
	int vbus_pin_inverted;
	int pre_vbus_status;

#if IS_ENABLED(CONFIG_USB_CVITEK_MISC)
	struct cviusb_drd_misc	cviusb_misc;
#endif

#if !IS_ENABLED(CONFIG_USB_CVITEK_A_BIDL_ADIS_HW_TMR)
	struct a_bidl_adis_worker_data a_bidl_adis_data;
#endif
	struct cvi_usb_clk	clk_axi;
	struct cvi_usb_clk	clk_apb;
	struct cvi_usb_clk	clk_125m;
	struct cvi_usb_clk	clk_33k;
	struct cvi_usb_clk	clk_12m;
	int			id_override;
	u8			dcd_dis;
	u8			chgdet;
};

/* prototypes */
void cviusb_set_mode(struct cviusb_dev *cviusb, u32 mode);
void cviusb_otg_enable_irq(struct cviusb_dev *cviusb, u32 irq);
void cviusb_otg_disable_irq(struct cviusb_dev *cviusb, u32 irq);
int cviusb_global_synchro_timer_setup(struct cviusb_dev *cviusb);
int cviusb_global_synchro_timer_start(struct cviusb_dev *cviusb);
int cviusb_global_synchro_timer_stop(struct cviusb_dev *cviusb);

#if IS_ENABLED(CONFIG_USB_CVITEK_MISC)
void cviusb_otg_fsm_sync(struct cviusb_dev *cviusb);
int cviusb_otg_standby_allowed(struct cviusb_dev *cviusb);
int cviusb_otg_set_standby(struct cviusb_dev *cviusb);
int cviusb_otg_clear_standby(struct cviusb_dev *cviusb);
#endif

#endif /* __DRIVERS_USB_CVI_CORE_H */
