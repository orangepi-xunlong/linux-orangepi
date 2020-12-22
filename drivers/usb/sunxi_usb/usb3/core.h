/*
 * drivers/usb/sunxi_usb/udc/usb3/core.d
 * (C) Copyright 2013-2015
 * Allwinner Technology Co., Ltd. <www.allwinnertech.com>
 *
 *
 * usb3.0 contoller driver
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 */

#ifndef __DRIVERS_USB_SUNXI_CORE_H
#define __DRIVERS_USB_SUNXI_CORE_H

#include <linux/device.h>
#include <linux/spinlock.h>
#include <linux/ioport.h>
#include <linux/list.h>
#include <linux/dma-mapping.h>
#include <linux/mm.h>
#include <linux/debugfs.h>

#include  <linux/gpio.h>
#include <linux/clk.h>
#include <linux/clk/sunxi_name.h>
#include  <mach/sys_config.h>
#include <linux/usb/ch9.h>
#include <linux/usb/gadget.h>

#if defined (CONFIG_FPGA_V4_PLATFORM) || defined (CONFIG_FPGA_V7_PLATFORM)
#define   SUNXI_USB_FPGA
#endif

#define SUNXI_ENDPOINTS_NUM	32
#define SUNXI_EVENT_BUFFERS_SIZE	PAGE_SIZE

struct sunxi_otgc_trb;

/**
 * struct sunxi_otgc_event_buffer - Software event buffer representation
 * @list: a list of event buffers
 * @buf: _THE_ buffer
 * @length: size of this buffer
 * @dma: dma_addr_t
 * @otgc: pointer to OTG controller
 */
struct sunxi_otgc_event_buffer {
	void			*buf;
	unsigned		length;
	unsigned int		lpos;

	dma_addr_t		dma;

	struct sunxi_otgc		*otgc;
};

#define SUNXI_EP_FLAG_STALLED	(1 << 0)
#define SUNXI_EP_FLAG_WEDGED	(1 << 1)

#define SUNXI_EP_DIRECTION_TX	true
#define SUNXI_EP_DIRECTION_RX	false

#define SUNXI_TRB_NUM		32
#define SUNXI_TRB_MASK		(SUNXI_TRB_NUM - 1)

/**
 * struct sunxi_otgc_ep - device side endpoint representation
 * @endpoint: usb endpoint
 * @request_list: list of requests for this endpoint
 * @req_queued: list of requests on this ep which have TRBs setup
 * @trb_pool: array of transaction buffers
 * @trb_pool_dma: dma address of @trb_pool
 * @free_slot: next slot which is going to be used
 * @busy_slot: first slot which is owned by HW
 * @desc: usb_endpoint_descriptor pointer
 * @otgc: pointer to OTG controller
 * @flags: endpoint flags (wedged, stalled, ...)
 * @current_trb: index of current used trb
 * @number: endpoint number (1 - 15)
 * @type: set to bmAttributes & USB_ENDPOINT_XFERTYPE_MASK
 * @res_trans_idx: Resource transfer index
 * @interval: the intervall on which the ISOC transfer is started
 * @name: a human readable name e.g. ep1out-bulk
 * @direction: true for TX, false for RX
 * @stream_capable: true when streams are enabled
 */
struct sunxi_otgc_ep {
	struct usb_ep		endpoint;
	struct list_head	request_list;
	struct list_head	req_queued;

	struct sunxi_otgc_trb		*trb_pool;
	dma_addr_t		trb_pool_dma;
	u32			free_slot;
	u32			busy_slot;
	const struct usb_endpoint_descriptor *desc;
	const struct usb_ss_ep_comp_descriptor *comp_desc;
	struct sunxi_otgc		*otgc;

	unsigned		flags;
	unsigned		current_trb;

	u8			number;
	u8			type;
	u8			res_trans_idx;
	u32			interval;

	char			name[20];

	unsigned		direction:1;
	unsigned		stream_capable:1;
};

enum sunxi_phy {
	SUNXI_PHY_UNKNOWN = 0,
	SUNXI_PHY_USB3,
	SUNXI_PHY_USB2,
};

enum sunxi_ep0_next {
	SUNXI_EP0_UNKNOWN = 0,
	SUNXI_EP0_COMPLETE,
	SUNXI_EP0_NRDY_SETUP,
	SUNXI_EP0_NRDY_DATA,
	SUNXI_EP0_NRDY_STATUS,
};

enum sunxi_ep0_state {
	EP0_UNCONNECTED		= 0,
	EP0_SETUP_PHASE,
	EP0_DATA_PHASE,
	EP0_STATUS_PHASE,
};

enum sunxi_link_state {
	/* In SuperSpeed */
	SUNXI_LINK_STATE_U0		= 0x00, /* in HS, means ON */
	SUNXI_LINK_STATE_U1		= 0x01,
	SUNXI_LINK_STATE_U2		= 0x02, /* in HS, means SLEEP */
	SUNXI_LINK_STATE_U3		= 0x03, /* in HS, means SUSPEND */
	SUNXI_LINK_STATE_SS_DIS		= 0x04,
	SUNXI_LINK_STATE_RX_DET		= 0x05, /* in HS, means Early Suspend */
	SUNXI_LINK_STATE_SS_INACT	= 0x06,
	SUNXI_LINK_STATE_POLL		= 0x07,
	SUNXI_LINK_STATE_RECOV		= 0x08,
	SUNXI_LINK_STATE_HRESET		= 0x09,
	SUNXI_LINK_STATE_CMPLY		= 0x0a,
	SUNXI_LINK_STATE_LPBK		= 0x0b,
	SUNXI_LINK_STATE_MASK		= 0x0f,
};

enum sunxi_device_state {
	SUNXI_DEFAULT_STATE,
	SUNXI_ADDRESS_STATE,
	SUNXI_CONFIGURED_STATE,
};

struct sunxi_otgc_trb {
	u32		bpl;
	u32		bph;
	u32		size;
	u32		ctrl;
} __packed;

/**
 * sunxi_hwparams - copy of HWPARAMS registers
 * @hwparams0 - GHWPARAMS0
 * @hwparams1 - GHWPARAMS1
 * @hwparams2 - GHWPARAMS2
 * @hwparams3 - GHWPARAMS3
 * @hwparams4 - GHWPARAMS4
 * @hwparams5 - GHWPARAMS5
 * @hwparams6 - GHWPARAMS6
 * @hwparams7 - GHWPARAMS7
 * @hwparams8 - GHWPARAMS8
 */
struct sunxi_otgc_hwparams {
	u32	hwparams0;
	u32	hwparams1;
	u32	hwparams2;
	u32	hwparams3;
	u32	hwparams4;
	u32	hwparams5;
	u32	hwparams6;
	u32	hwparams7;
	u32	hwparams8;
};

struct sunxi_otgc_request {
	struct usb_request	request;
	struct list_head	list;
	struct sunxi_otgc_ep		*dep;

	u8			epnum;
	struct sunxi_otgc_trb		*trb;
	dma_addr_t		trb_dma;

	unsigned		direction:1;
	unsigned		mapped:1;
	unsigned		queued:1;
};

/* pio info */
typedef struct usb_gpio{
	__u32 valid;				/* pio valid, 1 - valid, 0 - invalid */
	script_item_u gpio_set;
}usb_gpio_t;

struct sunxi_otgc_port_ctl {
	struct clk	*otg_clk;
	struct clk	*otg_phyclk;

	usb_gpio_t drv_vbus;			/* usb drv_vbus pin info	*/

	char* regulator_io;
	int   regulator_value;
	struct regulator* regulator_io_hdle;

};

/**
 * struct sunxi_otgc - representation of our controller
 * @ctrl_req: usb control request which is used for ep0
 * @ep0_trb: trb which is used for the ctrl_req
 * @ep0_bounce: bounce buffer for ep0
 * @setup_buf: used while precessing STD USB requests
 * @ctrl_req_addr: dma address of ctrl_req
 * @ep0_trb: dma address of ep0_trb
 * @ep0_usb_req: dummy req used while handling STD USB requests
 * @ep0_bounce_addr: dma address of ep0_bounce
 * @lock: for synchronizing
 * @dev: pointer to our struct device
 * @xhci: pointer to our xHCI child
 * @event_buffer_list: a list of event buffers
 * @gadget: device side representation of the peripheral controller
 * @gadget_driver: pointer to the gadget driver
 * @regs: base address for our registers
 * @regs_size: address space size
 * @irq: IRQ number
 * @num_event_buffers: calculated number of event buffers
 * @u1u2: only used on revisions <1.83a for workaround
 * @maximum_speed: maximum speed requested (mainly for testing purposes)
 * @revision: revision register contents
 * @mode: mode of operation
 * @is_selfpowered: true when we are selfpowered
 * @three_stage_setup: set if we perform a three phase setup
 * @ep0_bounced: true when we used bounce buffer
 * @ep0_expect_in: true when we expect a DATA IN transfer
 * @start_config_issued: true when StartConfig command has been issued
 * @setup_packet_pending: true when there's a Setup Packet in FIFO. Workaround
 * @needs_fifo_resize: not all users might want fifo resizing, flag it
 * @resize_fifos: tells us it's ok to reconfigure our TxFIFO sizes.
 * @ep0_next_event: hold the next expected event
 * @ep0state: state of endpoint zero
 * @link_state: link state
 * @speed: device speed (super, high, full, low)
 * @mem: points to start of memory which is used for this struct.
 * @hwparams: copy of hwparams registers
 * @root: debugfs root folder pointer
 */
struct sunxi_otgc {
	struct usb_ctrlrequest	*ctrl_req;
	struct sunxi_otgc_trb		*ep0_trb;
	void			*ep0_bounce;
	u8			*setup_buf;
	dma_addr_t		ctrl_req_addr;
	dma_addr_t		ep0_trb_addr;
	dma_addr_t		ep0_bounce_addr;
	struct sunxi_otgc_request	ep0_usb_req;
	/* device lock */
	spinlock_t		lock;
	struct device		*dev;

	struct platform_device	*xhci;
	struct resource		*res;

	struct sunxi_otgc_event_buffer **ev_buffs;
	struct sunxi_otgc_ep		*eps[SUNXI_ENDPOINTS_NUM];

	struct usb_gadget	gadget;
	struct usb_gadget_driver *gadget_driver;

	void __iomem		*regs;
	size_t			regs_size;

	int			irq;

	u32			num_event_buffers;
	u32			u1u2;
	u32			maximum_speed;
	u32			revision;
	u32			mode;
	/* used for suspend/resume */
	u32			dcfg;
	u32			gctl;
	unsigned		is_selfpowered:1;
	unsigned		three_stage_setup:1;
	unsigned		ep0_bounced:1;
	unsigned		ep0_expect_in:1;
	unsigned		start_config_issued:1;
	unsigned		setup_packet_pending:1;
	unsigned		delayed_status:1;
	unsigned		needs_fifo_resize:1;
	unsigned		resize_fifos:1;

	enum sunxi_ep0_next	ep0_next_event;
	enum sunxi_ep0_state	ep0state;
	enum sunxi_link_state	link_state;
	enum sunxi_device_state	dev_state;

	u8			speed;
	void			*mem;

	struct sunxi_otgc_hwparams	hwparams;
	struct dentry		*root;

	u8			test_mode;
	u8			test_mode_nr;
	struct sunxi_otgc_port_ctl port_ctl;
};

/* -------------------------------------------------------------------------- */

/* -------------------------------------------------------------------------- */

struct sunxi_otgc_event_type {
	u32	is_devspec:1;
	u32	type:6;
	u32	reserved8_31:25;
} __packed;


/**
 * struct sunxi_otgc_event_depvt - Device Endpoint Events
 * @one_bit: indicates this is an endpoint event (not used)
 * @endpoint_number: number of the endpoint
 * @endpoint_event: The event we have:
 *	0x00	- Reserved
 *	0x01	- XferComplete
 *	0x02	- XferInProgress
 *	0x03	- XferNotReady
 *	0x04	- RxTxFifoEvt (IN->Underrun, OUT->Overrun)
 *	0x05	- Reserved
 *	0x06	- StreamEvt
 *	0x07	- EPCmdCmplt
 * @reserved11_10: Reserved, don't use.
 * @status: Indicates the status of the event. Refer to databook for
 *	more information.
 * @parameters: Parameters of the current event. Refer to databook for
 *	more information.
 */
struct sunxi_otgc_event_depevt {
	u32	one_bit:1;
	u32	endpoint_number:5;
	u32	endpoint_event:4;
	u32	reserved11_10:2;
	u32	status:4;

/* Within XferNotReady */
#define DEPEVT_STATUS_TRANSFER_ACTIVE	(1 << 3)

/* Within XferComplete */
#define DEPEVT_STATUS_BUSERR	(1 << 0)
#define DEPEVT_STATUS_SHORT	(1 << 1)
#define DEPEVT_STATUS_IOC	(1 << 2)
#define DEPEVT_STATUS_LST	(1 << 3)

/* Stream event only */
#define DEPEVT_STREAMEVT_FOUND		1
#define DEPEVT_STREAMEVT_NOTFOUND	2

/* Control-only Status */
#define DEPEVT_STATUS_CONTROL_SETUP	0
#define DEPEVT_STATUS_CONTROL_DATA	1
#define DEPEVT_STATUS_CONTROL_STATUS	2

	u32	parameters:16;
} __packed;

/**
 * struct sunxi_otgc_event_devt - Device Events
 * @one_bit: indicates this is a non-endpoint event (not used)
 * @device_event: indicates it's a device event. Should read as 0x00
 * @type: indicates the type of device event.
 *	0	- DisconnEvt
 *	1	- USBRst
 *	2	- ConnectDone
 *	3	- ULStChng
 *	4	- WkUpEvt
 *	5	- Reserved
 *	6	- EOPF
 *	7	- SOF
 *	8	- Reserved
 *	9	- ErrticErr
 *	10	- CmdCmplt
 *	11	- EvntOverflow
 *	12	- VndrDevTstRcved
 * @reserved15_12: Reserved, not used
 * @event_info: Information about this event
 * @reserved31_24: Reserved, not used
 */
struct sunxi_otgc_event_devt {
	u32	one_bit:1;
	u32	device_event:7;
	u32	type:4;
	u32	reserved15_12:4;
	u32	event_info:8;
	u32	reserved31_24:8;
} __packed;

/**
 * struct sunxi_otgc_event_gevt - Other Core Events
 * @one_bit: indicates this is a non-endpoint event (not used)
 * @device_event: indicates it's (0x03) Carkit or (0x04) I2C event.
 * @phy_port_number: self-explanatory
 * @reserved31_12: Reserved, not used.
 */
struct sunxi_otgc_event_gevt {
	u32	one_bit:1;
	u32	device_event:7;
	u32	phy_port_number:4;
	u32	reserved31_12:20;
} __packed;

/**
 * union sunxi_event - representation of Event Buffer contents
 * @raw: raw 32-bit event
 * @type: the type of the event
 * @depevt: Device Endpoint Event
 * @devt: Device Event
 * @gevt: Global Event
 */
union sunxi_event {
	u32				raw;
	struct sunxi_otgc_event_type		type;
	struct sunxi_otgc_event_depevt	depevt;
	struct sunxi_otgc_event_devt		devt;
	struct sunxi_otgc_event_gevt		gevt;
};


/* prototypes */
void sunxi_set_mode(struct sunxi_otgc *otgc, u32 mode);
int sunxi_gadget_resize_tx_fifos(struct sunxi_otgc *otgc);

int sunxi_host_init(struct sunxi_otgc *otgc);
void sunxi_host_exit(struct sunxi_otgc *otgc);

int sunxi_gadget_init(struct sunxi_otgc *otgc);
void sunxi_gadget_exit(struct sunxi_otgc *otgc);

extern int sunxi_get_device_id(void);
extern void sunxi_put_device_id(int id);

int sunxi_usb_device_enable(void);
int sunxi_usb_device_disable(void);
int sunxi_usb_host0_enable(void);
int sunxi_usb_host0_disable(void);
#ifdef CONFIG_USB_XHCI_ENHANCE
int sunxi_usb_get_udev_connect(void);
int switch_to_usb2(void);
int switch_to_usb3(void);
void sunxi_usb3_connect(void);
void sunxi_usb3_disconnect(void);
#endif
int sunxi_gadget_suspend(struct sunxi_otgc *otgc);
int sunxi_gadget_resume(struct sunxi_otgc *otgc);
int sunxi_gadget_enable(struct sunxi_otgc *otgc);
int sunxi_gadget_disable(struct sunxi_otgc *otgc);

#endif /* __DRIVERS_USB_SUNXI_CORE_H */
