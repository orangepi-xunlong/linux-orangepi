#ifndef __DRIVERS_USB_BM_CORE_H
#define __DRIVERS_USB_BM_CORE_H

#include <linux/usb/otg.h>
#include <linux/usb/otg-fsm.h>

#include <linux/phy/phy.h>

#if IS_ENABLED(CONFIG_USB_BM_MISC)
#include "bmusb_misc.h"
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
#define BM_ALIGN_MASK		(16 - 1)

enum bmusb_role {
	BM_ROLE_HOST = 0,
	BM_ROLE_GADGET,
	BM_ROLE_OTG,
	BM_ROLE_END,
};

#if !IS_ENABLED(CONFIG_USB_BM_A_BIDL_ADIS_HW_TMR)
struct a_bidl_adis_worker_data {
	int worker_running;
	struct otg_fsm *fsm;
	struct delayed_work a_bidl_adis_work;
};
#endif

/**
 * struct bmusb - Representation of Bitmain DRD OTG controller.
 * @lock: for synchronizing
 * @dev: pointer to Bitmain device struct
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
 * @bmusb_misc: misc device for userspace communication
 * @a_bidl_adis_data: structure for a_bidl_adis timer implementation
 */
struct bmusb_dev {
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

#if IS_ENABLED(CONFIG_USB_BM_MISC)
	struct bmusb_drd_misc	bmusb_misc;
#endif

#if !IS_ENABLED(CONFIG_USB_BM_A_BIDL_ADIS_HW_TMR)
	struct a_bidl_adis_worker_data a_bidl_adis_data;
#endif
};

/* prototypes */
void bmusb_set_mode(struct bmusb_dev *bmusb, u32 mode);
void bmusb_otg_enable_irq(struct bmusb_dev *bmusb, u32 irq);
void bmusb_otg_disable_irq(struct bmusb_dev *bmusb, u32 irq);
int bmusb_global_synchro_timer_setup(struct bmusb_dev *bmusb);
int bmusb_global_synchro_timer_start(struct bmusb_dev *bmusb);
int bmusb_global_synchro_timer_stop(struct bmusb_dev *bmusb);

#if IS_ENABLED(CONFIG_USB_BM_MISC)
void bmusb_otg_fsm_sync(struct bmusb_dev *bmusb);
int bmusb_otg_standby_allowed(struct bmusb_dev *bmusb);
int bmusb_otg_set_standby(struct bmusb_dev *bmusb);
int bmusb_otg_clear_standby(struct bmusb_dev *bmusb);
#endif

#endif /* __DRIVERS_USB_BM_CORE_H */
