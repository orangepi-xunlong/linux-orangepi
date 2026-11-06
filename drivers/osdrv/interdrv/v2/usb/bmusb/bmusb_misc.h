#ifndef __BM_MISC_H
#define __BM_MISC_H

#include <linux/miscdevice.h>
#include <linux/usb/phy.h>
#include <linux/usb/otg-fsm.h>

#include "bmusb_cmd.h"
#include "otg_fsm.h"

struct bmusb_dev;
struct usb_ss_dev;

struct bmusb_drd_misc {
	struct		miscdevice miscdev;
	int			force_a_idle;
	int			force_b_srp_init;
};

extern void bmusb_dev_misc_register(struct usb_ss_dev *usb_ss, int res_address);
extern void bmusb_drd_misc_register(struct bmusb_dev *bmusb, int res_address);

#endif /* __BM_MISC_H */
