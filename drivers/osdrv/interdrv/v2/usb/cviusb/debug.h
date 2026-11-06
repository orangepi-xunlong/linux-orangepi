#ifndef __DRIVERS_USB_SS_DEBUG
#define __DRIVERS_USB_SS_DEBUG

#include <linux/usb/ch9.h>
#include <linux/usb/gadget.h>
#include "cviusb_gadget.h"

void display_ep_desc(struct usb_ss_dev *usb_ss, struct usb_ep *usb_endpoint);
void print_all_ep(struct usb_ss_dev *usb_ss);
void usb_ss_dump_regs(struct usb_ss_dev *usb_ss);
void usb_ss_dump_reg(struct usb_ss_dev *usb_ss, uint32_t __iomem *reg);

void usb_ss_dbg_dump_lnx_usb_ep(struct device *dev, struct usb_ep *ep, int tab);
void usb_ss_dbg_dump_lnx_usb_gadget(struct usb_gadget *gadget);
void usb_ss_dbg_dump_lnx_usb_request(struct usb_ss_dev *usb_ss,
		struct usb_request *usb_req);

void usb_ss_dbg_dump_cviusb_usb_ss_dev(struct usb_ss_dev *usb_ss);
void usb_ss_dbg_dump_cviusb_usb_ss_ep(struct usb_ss_dev *usb_ss,
		struct usb_ss_endpoint *usb_ss_ep);
void usb_ss_dbg_dump_cviusb_usb_ss_trb(struct usb_ss_dev *usb_ss,
		struct usb_ss_trb *usb_trb);

/* Enable Cvitek USB debugging */
/* define CVI_DBG_ENABLED */

#ifdef CVI_DBG_ENABLED
#define cviusb_dbg(dev, fmt, args...) \
	pr_emerg(fmt, ## args)
#define cviusb_err(dev, fmt, args...) \
	pr_emerg(fmt, ## args)
#define cviusb_warn(dev, fmt, args...) \
	pr_emerg(fmt, ## args)
#define cviusb_warn_ratelimited(dev, fmt, args...) \
	pr_emerg(fmt, ## args)
#define cviusb_info(dev, fmt, args...) \
	pr_emerg(fmt, ## args)
#else
#define cviusb_dbg(dev, fmt, args...) \
	dev_dbg(dev, fmt, ## args)
#define cviusb_err(dev, fmt, args...) \
	dev_err(dev, fmt, ## args)
#define cviusb_warn(dev, fmt, args...) \
	dev_warn(dev, fmt, ## args)
#define cviusb_warn_ratelimited(dev, fmt, args...) \
	dev_warn_ratelimited(dev, fmt, ## args)
#define cviusb_info(dev, fmt, args...) \
	dev_info(dev, fmt, ## args)
#endif

#endif
