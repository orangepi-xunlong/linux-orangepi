/*
 * drivers/usb/sunxi_usb/hcd/include/sunxi_hcd_core.h
 * (C) Copyright 2010-2015
 * Allwinner Technology Co., Ltd. <www.allwinnertech.com>
 * javen, 2010-12-20, create this file
 *
 * usb host contoller driver
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 */

#ifndef  __SUNXI_HCD_CORE_H__
#define  __SUNXI_HCD_CORE_H__

#include <linux/slab.h>
#include <linux/list.h>
#include <linux/interrupt.h>
#include <linux/errno.h>
#include <linux/clk.h>
#include <linux/device.h>
#include <linux/usb/ch9.h>

#include <linux/dma-mapping.h>

#include <linux/usb.h>
#include <linux/usb/hcd.h>

#include  "sunxi_hcd_config.h"

struct sunxi_hcd;
struct sunxi_hcd_hw_ep;

#include  "sunxi_hcd_regs_i.h"

#include  "sunxi_hcd_board.h"
#include  "sunxi_hcd_host.h"
#include  "sunxi_hcd_virt_hub.h"
#include  "sunxi_hcd_dma.h"

#define is_host_active(m)			((m)->is_host)

#define	is_host_capable()			(1)
#define SW_HCD_C_NUM_EPS      			USBC_MAX_EP_NUM
#define	is_host_enabled(sunxi_usb)		is_host_capable()

/* host side ep0 states */
enum sunxi_hcd_h_ep0_state {
	SUNXI_HCD_EP0_IDLE,
	SUNXI_HCD_EP0_START,		/* expect ack of setup */
	SUNXI_HCD_EP0_IN,		/* expect IN DATA */
	SUNXI_HCD_EP0_OUT,		/* expect ack of OUT DATA */
	SUNXI_HCD_EP0_STATUS,		/* expect ack of STATUS */
}__attribute__ ((packed));


/*
* struct sunxi_hcd_hw_ep - endpoint hardware (bidirectional)
*
* Ordered slightly for better cacheline locality.
*/
typedef struct sunxi_hcd_hw_ep{
	struct sunxi_hcd *sunxi_hcd;    /* ep owner                          */
	void __iomem *fifo;             /* fifo base address                 */
	void __iomem *regs;             /* USB controller reg base addr      */

	u8 epnum;      	                /* index in sunxi_hcd->endpoints[]   */

	/* hardware configuration, possibly dynamic */
	bool is_shared_fifo;            /* if share fifo                     */
	bool tx_double_buffered;        /* Flag. if double fifo?             */
	bool rx_double_buffered;        /* Flag. if double fifo?             */
	u16 max_packet_sz_tx;           /* max packet size                   */
	u16 max_packet_sz_rx;           /* max packet size                   */

	void __iomem *target_regs;      /* the dest device addr on hub       */

	/* currently scheduled peripheral endpoint */
	struct sunxi_hcd_qh *in_qh;     /* store in ep schedule info         */
	struct sunxi_hcd_qh *out_qh;    /* store out ep schedule info        */

	u8 rx_reinit;                   /* flag. if re-initialize            */
	u8 tx_reinit;                   /* flag. if re-initialize            */
}sunxi_hcd_hw_ep_t;

/*
 * struct sunxi_hcd - Driver instance data.
 */
typedef struct sunxi_hcd{
	/* device lock */
	spinlock_t lock;                    /* mutex lock */
	irqreturn_t (*isr)(int, void *);
	struct work_struct irq_work;

	char driver_name[32];
	__u32 usbc_no;

	/* this hub status bit is reserved by USB 2.0 and not seen by usbcore */
#define SUNXI_HCD_PORT_STAT_RESUME	(1 << 31)
	u32 port1_status;                   /* virtual hub's port status      */
	unsigned long rh_timer;             /* root hub's delay time          */

	enum sunxi_hcd_h_ep0_state ep0_stage; /* ep0's status                 */

	/* bulk traffic normally dedicates endpoint hardware, and each
	 * direction has its own ring of host side endpoints.
	 * we try to progress the transfer at the head of each endpoint's
	 * queue until it completes or NAKs too much; then we try the next
	 * endpoint.
	 */
	struct sunxi_hcd_hw_ep *bulk_ep;

	struct list_head control;	    /* of sunxi_hcd_qh               */
	struct list_head in_bulk;	    /* of sunxi_hcd_qh               */
	struct list_head out_bulk;	    /* of sunxi_hcd_qh               */

	/* called with IRQs blocked; ON/nonzero implies starting a session,
	 * and waiting at least a_wait_vrise_tmout.
	 */
	void (*board_set_vbus)(struct sunxi_hcd *, int is_on);

	sunxi_hcd_dma_t sunxi_hcd_dma;

	struct device *controller;
	void __iomem *ctrl_base;            /* USB controller reg base addr  */
	void __iomem *mregs;                /* USB controller reg base addr  */

	/* passed down from chip/board specific irq handlers */
	u8 int_usb;                         /* USB irq                       */
	u16 int_rx;                         /* rx irq                        */
	u16 int_tx;                         /* tx irq                        */

	int nIrq;                           /* irq no                        */
	unsigned irq_wake:1;                /* flag. irq enable flag         */

	struct sunxi_hcd_hw_ep endpoints[SW_HCD_C_NUM_EPS]; /* sunxi_hcd all ep info */
#define control_ep	endpoints

#define VBUSERR_RETRY_COUNT	3
	u16 vbuserr_retry;                  /* times of host retry when vbus error */
	u16 epmask;                         /* ep mask, bitn = 1, indicate epn valid */
	u8 nr_endpoints;                    /* valid ep count                 */

	u8 board_mode;		            /* enum sunxi_hcd_mode            */

	int (*board_set_power)(int state);
	int (*set_clock)(struct clk *clk, int is_active);

	u8 min_power;	                    /* vbus for periph, in mA/2       */

	bool is_host;                       /* flag. if is host transfer      */
	int a_wait_bcon;	            /* VBUS timeout in msecs          */
	unsigned long idle_timeout;	    /* Next timeout in jiffies        */

	/* active means connected and not suspended */
	unsigned is_active:1;
	unsigned is_connected:1;
	unsigned is_reset:1;
	unsigned is_suspend:1;

	unsigned is_multipoint:1;           /* flag. is multiple transaction ep? */
	unsigned ignore_disconnect:1;	    /* during bus resets              */

	unsigned bulk_split:1;
#define	can_bulk_split(sunxi_usb, type)       (((type) == USB_ENDPOINT_XFER_BULK) && (sunxi_usb)->bulk_split)

	unsigned bulk_combine:1;
#define	can_bulk_combine(sunxi_usb, type)     (((type) == USB_ENDPOINT_XFER_BULK) && (sunxi_usb)->bulk_combine)

	struct sunxi_hcd_config	*config;    /* sunxi_hcd config information    */

	sunxi_hcd_io_t	*sunxi_hcd_io;
	u32 enable;
	u32 init_controller;
	u32 suspend;
	u32 session_req_flag;
	u32 reset_flag;
	u32 vbus_error_flag;
}sunxi_hcd_t;

struct sunxi_hcd_ep_reg{
	__u32 USB_CSR0;
	__u32 USB_TXCSR;
	__u32 USB_RXCSR;
	__u32 USB_COUNT0;
	__u32 USB_RXCOUNT;
	__u32 USB_ATTR0;
	__u32 USB_EPATTR;
	__u32 USB_TXFIFO;
	__u32 USB_RXFIFO;
	__u32 USB_FADDR;
	__u32 USB_TXFADDR;
	__u32 USB_RXFADDR;
};

struct sunxi_hcd_context_registers {
	/* FIFO Entry for Endpoints */
	__u32 USB_EPFIFO0;
	__u32 USB_EPFIFO1;
	__u32 USB_EPFIFO2;
	__u32 USB_EPFIFO3;
	__u32 USB_EPFIFO4;
	__u32 USB_EPFIFO5;

	/* Common Register */
	__u32 USB_GCS;
	__u32 USB_EPINTF;
	__u32 USB_EPINTE;
	__u32 USB_BUSINTF;
	__u32 USB_BUSINTE;
	__u32 USB_FNUM;
	__u32 USB_TESTC;

	/* Endpoint Index Register */
	struct sunxi_hcd_ep_reg ep_reg[SW_HCD_C_NUM_EPS];

	/* Configuration Register */
	__u32 USB_CONFIGINFO;
	__u32 USB_LINKTIM;
	__u32 USB_OTGTIM;

	/* PHY and Interface Control and Status Register */
	__u32 USB_ISCR;
	__u32 USB_PHYCTL;
	__u32 USB_PHYBIST;
};

static inline struct sunxi_hcd *dev_to_sunxi_hcd(struct device *dev)
{
	/* usbcore insists dev->driver_data is a "struct hcd *" */
	return hcd_to_sunxi_hcd(dev_get_drvdata(dev));
}

/* vbus ops */
static inline void sunxi_hcd_set_vbus(struct sunxi_hcd *sunxi_hcd, int is_on)
{
	/* check argment */
	if (sunxi_hcd == NULL) {
		printk("ERR: invalid argment sunxi_hcd is null\n");
		return;
	}
	if (sunxi_hcd->board_set_vbus) {
		sunxi_hcd->board_set_vbus(sunxi_hcd, is_on);
	}
}

/* read fifo size */
static inline int sunxi_hcd_read_fifosize(struct sunxi_hcd *sunxi_hcd, struct sunxi_hcd_hw_ep *hw_ep, u8 epnum)
{
	void __iomem *xbase = sunxi_hcd->mregs;
	u8 reg = 0;

	/* read from core using indexed model */
	reg = USBC_Readb(USBC_REG_TXFIFOSZ(xbase));
	/* 0's returned when no more endpoints */
	if (!reg) {
		return -ENODEV;
	}
	hw_ep->max_packet_sz_tx = 1 << (reg & 0x0f);

	sunxi_hcd->nr_endpoints++;
	sunxi_hcd->epmask |= (1 << epnum);

	/* read from core using indexed model */
	reg = USBC_Readb(USBC_REG_RXFIFOSZ(xbase));
	/* 0's returned when no more endpoints */
	if (!reg) {
		return -ENODEV;
	}
	/* shared TX/RX FIFO? */
	if ((reg & 0xf0) == 0xf0) {
		hw_ep->max_packet_sz_rx = hw_ep->max_packet_sz_tx;
		hw_ep->is_shared_fifo = true;
		return 0;
	} else {
		hw_ep->max_packet_sz_rx = 1 << ((reg & 0xf0) >> 4);
		hw_ep->is_shared_fifo = false;
	}

	return 0;
}

/* config ep0 */
static inline void sunxi_hcd_configure_ep0(struct sunxi_hcd *sunxi_hcd)
{
	sunxi_hcd->endpoints[0].max_packet_sz_tx = USBC_EP0_FIFOSIZE;
	sunxi_hcd->endpoints[0].max_packet_sz_rx = USBC_EP0_FIFOSIZE;
	sunxi_hcd->endpoints[0].is_shared_fifo = true;
}

#define  SUNXI_HCD_HST_MODE(sunxi_hcd) 	{ (sunxi_hcd)->is_host = true; }
#define  is_direction_in(qh)		(qh->hep->desc.bEndpointAddress & USB_ENDPOINT_DIR_MASK)

void sunxi_hcd_write_fifo(struct sunxi_hcd_hw_ep *hw_ep, u16 len, const u8 *src);
void sunxi_hcd_read_fifo(struct sunxi_hcd_hw_ep *hw_ep, u16 len, u8 *dst);
void sunxi_hcd_load_testpacket(struct sunxi_hcd *sunxi_hcd);
void sunxi_hcd_generic_disable(struct sunxi_hcd *sunxi_hcd);

irqreturn_t generic_interrupt(int irq, void *__hci);

void sunxi_hcd_soft_disconnect(struct sunxi_hcd *sunxi_hcd);
void sunxi_hcd_start(struct sunxi_hcd *sunxi_hcd);
void sunxi_hcd_stop(struct sunxi_hcd *sunxi_hcd);

void sunxi_hcd_platform_try_idle(struct sunxi_hcd *sunxi_hcd, unsigned long timeout);
void sunxi_hcd_platform_enable(struct sunxi_hcd *sunxi_hcd);
void sunxi_hcd_platform_disable(struct sunxi_hcd *sunxi_hcd);
int sunxi_hcd_platform_set_mode(struct sunxi_hcd *sunxi_hcd, u8 sunxi_hcd_mode);
int sunxi_hcd_platform_init(struct sunxi_hcd *sunxi_hcd);
int sunxi_hcd_platform_exit(struct sunxi_hcd *sunxi_hcd);
int sunxi_hcd_platform_suspend(struct sunxi_hcd *sunxi_hcd);
int sunxi_hcd_platform_resume(struct sunxi_hcd *sunxi_hcd);

#endif   //__SUNXI_HCD_CORE_H__

