#ifndef __CVI_MISC_H
#define __CVI_MISC_H

#include <linux/miscdevice.h>
#include <linux/usb/phy.h>
#include <linux/usb/otg-fsm.h>

#include "cviusb_cmd.h"
#include "otg_fsm.h"

struct cviusb_dev;
struct usb_ss_dev;

struct cviusb_drd_misc {
	struct		miscdevice miscdev;
	int			force_a_idle;
	int			force_b_srp_init;
};

extern void cviusb_dev_misc_register(struct usb_ss_dev *usb_ss, int res_address);
extern void cviusb_drd_misc_register(struct cviusb_dev *cviusb, int res_address);

#endif /* __CVI_MISC_H */
