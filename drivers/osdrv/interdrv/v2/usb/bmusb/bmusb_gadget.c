#include <linux/platform_device.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/interrupt.h>
#include <linux/dma-mapping.h>
#include <linux/usb/composite.h>
#include <linux/of_platform.h>
#include <linux/usb/gadget.h>
#include <linux/delay.h>
#include <uapi/asm-generic/errno-base.h>
#include <uapi/asm-generic/errno.h>
#include <linux/byteorder/generic.h>
#include <linux/irq.h>
//#include <linux/irqdesc.h>

#include "bmusb_gadget.h"
#include "debug.h"
#include "io.h"

/*-------------------------------------------------------------------------*/
/* Structs declaration */

static const struct usb_ep_ops usb_ss_gadget_ep0_ops;
static const struct usb_ep_ops usb_ss_gadget_ep_ops;
static const struct usb_gadget_ops usb_ss_gadget_ops;


/*-------------------------------------------------------------------------*/
/* Function declarations */

static u32 gadget_readl(struct usb_ss_dev *usb_ss,
	uint32_t __iomem *reg);
static void gadget_writel(struct usb_ss_dev *usb_ss,
	uint32_t __iomem *reg, u32 value);

static void select_ep(struct usb_ss_dev *usb_ss, u32 ep);
static int usb_ss_allocate_trb_pool(struct usb_ss_endpoint *usb_ss_ep);
static void bmusb_ep_stall_flush(struct usb_ss_endpoint *usb_ss_ep);
static void bmusb_ep0_config(struct usb_ss_dev *usb_ss);
static void bmusb_gadget_unconfig(struct usb_ss_dev *usb_ss);
static void bmusb_ep0_run_transfer(struct usb_ss_dev *usb_ss,
	dma_addr_t dma_addr, unsigned int length, int erdy);
static int bmusb_ep_run_transfer(struct usb_ss_endpoint *usb_ss_ep);
static int bmusb_get_setup_ret(struct usb_ss_dev *usb_ss,
	struct usb_ctrlrequest *ctrl_req);
static int bmusb_req_ep0_set_address(struct usb_ss_dev *usb_ss,
	struct usb_ctrlrequest *ctrl_req);
static int bmusb_req_ep0_get_status(struct usb_ss_dev *usb_ss,
	struct usb_ctrlrequest *ctrl_req);
static int bmusb_req_ep0_handle_feature(struct usb_ss_dev *usb_ss,
	struct usb_ctrlrequest *ctrl_req, int set);
static int bmusb_req_ep0_set_sel(struct usb_ss_dev *usb_ss,
	struct usb_ctrlrequest *ctrl_req);
static int bmusb_req_ep0_set_isoch_delay(struct usb_ss_dev *usb_ss,
	struct usb_ctrlrequest *ctrl_req);
static int bmusb_req_ep0_set_configuration(struct usb_ss_dev *usb_ss,
	struct usb_ctrlrequest *ctrl_req);
static int bmusb_ep0_standard_request(struct usb_ss_dev *usb_ss,
	struct usb_ctrlrequest *ctrl_req);
static void bmusb_ep0_setup_phase(struct usb_ss_dev *usb_ss);
static int bmusb_check_ep_interrupt_proceed(struct usb_ss_endpoint *usb_ss_ep);
static void bmusb_check_ep0_interrupt_proceed(struct usb_ss_dev *usb_ss,
	int dir);
static void bmusb_check_usb_interrupt_proceed(struct usb_ss_dev *usb_ss,
	u32 usb_ists);
#ifdef BM_THREADED_IRQ_HANDLING
static irqreturn_t bmusb_irq_handler(int irq, void *_usb_ss);
#endif
static int usb_ss_gadget_ep0_enable(struct usb_ep *ep,
	const struct usb_endpoint_descriptor *desc);
static int usb_ss_gadget_ep0_disable(struct usb_ep *ep);
static int usb_ss_gadget_ep0_set_halt(struct usb_ep *ep, int value);
static int usb_ss_gadget_ep0_queue(struct usb_ep *ep,
	struct usb_request *request, gfp_t gfp_flags);
static void bmusb_ep_config(struct usb_ss_endpoint *usb_ss_ep, int index);
static int usb_ss_gadget_ep_enable(struct usb_ep *ep,
	const struct usb_endpoint_descriptor *desc);
static int usb_ss_gadget_ep_disable(struct usb_ep *ep);
static struct usb_request *usb_ss_gadget_ep_alloc_request(struct usb_ep *ep,
	gfp_t gfp_flags);
static void usb_ss_gadget_ep_free_request(struct usb_ep *ep,
	struct usb_request *request);
static int usb_ss_gadget_ep_queue(struct usb_ep *ep,
	struct usb_request *request, gfp_t gfp_flags);
static int usb_ss_gadget_ep_dequeue(struct usb_ep *ep,
	struct usb_request *request);
static int usb_ss_gadget_ep_set_halt(struct usb_ep *ep, int value);
static int usb_ss_gadget_ep_set_wedge(struct usb_ep *ep);
static int usb_ss_gadget_get_frame(struct usb_gadget *gadget);
static int usb_ss_gadget_wakeup(struct usb_gadget *gadget);
static int usb_ss_gadget_set_selfpowered(struct usb_gadget *gadget,
	int is_selfpowered);
static int usb_ss_gadget_pullup(struct usb_gadget *gadget, int is_on);
static int usb_ss_gadget_udc_start(struct usb_gadget *gadget,
	struct usb_gadget_driver *driver);
static int usb_ss_gadget_udc_stop(struct usb_gadget *gadget);
static int usb_ss_init_ep(struct usb_ss_dev *usb_ss);
static int usb_ss_init_ep0(struct usb_ss_dev *usb_ss);
static void usb_ss_turn_on_ref_clock(struct usb_ss_dev *usb_ss);
static void usb_ss_ensure_clock_on(struct usb_ss_dev *usb_ss);
static int gadget_probe(struct platform_device *pdev);
static int gadget_remove(struct platform_device *pdev);

#if !IS_ENABLED(CONFIG_USB_BM_MISC)
static void usb_ss_turn_on_ref_clock(struct usb_ss_dev *usb_ss) { }
static void usb_ss_ensure_clock_on(struct usb_ss_dev *usb_ss) { }
#else
static void usb_ss_turn_off_ref_clock(struct usb_ss_dev *usb_ss);
#endif

static u32 gadget_readl(struct usb_ss_dev *usb_ss,
	uint32_t __iomem *reg)
{
	if (IS_REG_REQUIRING_ACTIVE_REF_CLOCK(usb_ss, reg))
		usb_ss_ensure_clock_on(usb_ss);
	return bmusb_readl(reg);
}

static void gadget_writel(struct usb_ss_dev *usb_ss,
	uint32_t __iomem *reg, u32 value)
{
	if (IS_REG_REQUIRING_ACTIVE_REF_CLOCK(usb_ss, reg))
		usb_ss_ensure_clock_on(usb_ss);
	bmusb_writel(reg, value);
}

#define BM_THREADED_IRQ_HANDLING
/**
 * next_request - returns next request from list
 * @list: list containing requests
 *
 * Retuns request or NULL if no requests in list
 */
static struct usb_request *next_request(struct list_head *list)
{
	if (list_empty(list))
		return NULL;
	return list_first_entry(list, struct usb_request, list);
}

/**
 * select_ep - selects endpoint
 * @usb_ss: extended gadget object
 * @ep: endpoint address
 */
static void select_ep(struct usb_ss_dev *usb_ss, u32 ep)
{
	if (!usb_ss || !usb_ss->regs) {
		bmusb_err(usb_ss->dev, "Failed to select endpoint!\n");
		return;
	}

	gadget_writel(usb_ss, &usb_ss->regs->ep_sel, ep);
}

/**
 *usb_ss_allocate_trb_pool - Allocates TRB's pool for selected endpoint
 * @usb_ss_ep: extended endpoint object
 *
 * Function will return 0 on success or -ENOMEM on allocation error
 */
static int usb_ss_allocate_trb_pool(struct usb_ss_endpoint *usb_ss_ep)
{
	struct usb_ss_dev *usb_ss = usb_ss_ep->usb_ss;

	if (usb_ss_ep->trb_pool)
		return 0;
	dma_set_coherent_mask(usb_ss->dev, DMA_BIT_MASK(64));
	usb_ss_ep->trb_pool = dma_alloc_coherent(usb_ss->dev,
			sizeof(struct usb_ss_trb) * USB_SS_TRBS_NUM,
		&usb_ss_ep->trb_pool_dma, GFP_DMA);

	if (!usb_ss_ep->trb_pool) {
		bmusb_err(usb_ss->dev,
				"Failed to allocate TRB pool for endpoint %s\n",
				usb_ss_ep->name);
		return -ENOMEM;
	}

	return 0;
}

/**
 * bmusb_ep_stall_flush - Stalls and flushes selected endpoint
 * @usb_ss_ep: extended endpoint object
 *
 * Endpoint must be selected before call to this function
 */
static void bmusb_ep_stall_flush(struct usb_ss_endpoint *usb_ss_ep)
{
	struct usb_ss_dev *usb_ss = usb_ss_ep->usb_ss;

	gadget_writel(usb_ss, &usb_ss->regs->ep_cmd,
		EP_CMD__DFLUSH__MASK | EP_CMD__ERDY__MASK |
		EP_CMD__SSTALL__MASK);

	/* wait for DFLUSH cleared */
	while (gadget_readl(usb_ss,
		&usb_ss->regs->ep_cmd) & EP_CMD__DFLUSH__MASK)
		;

	usb_ss_ep->stalled_flag = 1;
}

/**
 * bmusb_ep0_config - Configures default endpoint
 * @usb_ss: extended gadget object
 *
 * Functions sets parameters: maximal packet size and enables interrupts
 */
static void bmusb_ep0_config(struct usb_ss_dev *usb_ss)
{
	u32 max_packet_size = 0;

	switch (usb_ss->gadget.speed) {
	case USB_SPEED_UNKNOWN:
		max_packet_size = ENDPOINT_MAX_PACKET_SIZE_0;
		break;

	case USB_SPEED_LOW:
		max_packet_size = ENDPOINT_MAX_PACKET_SIZE_8;
		break;

	case USB_SPEED_FULL:
		max_packet_size = ENDPOINT_MAX_PACKET_SIZE_64;
		break;

	case USB_SPEED_HIGH:
		max_packet_size = ENDPOINT_MAX_PACKET_SIZE_64;
		break;

	case USB_SPEED_WIRELESS:
		max_packet_size = ENDPOINT_MAX_PACKET_SIZE_64;
		break;

	case USB_SPEED_SUPER:
	case USB_SPEED_SUPER_PLUS:
		max_packet_size = ENDPOINT_MAX_PACKET_SIZE_512;
		break;
	}

	/* init ep out */
	select_ep(usb_ss, USB_DIR_OUT);

	gadget_writel(usb_ss, &usb_ss->regs->ep_cfg,
		EP_CFG__ENABLE__MASK |
		EP_CFG__MAXPKTSIZE__WRITE(max_packet_size));
	gadget_writel(usb_ss, &usb_ss->regs->ep_sts_en,
		EP_STS_EN__SETUPEN__MASK |
		EP_STS_EN__DESCMISEN__MASK |
		EP_STS_EN__TRBERREN__MASK);

	/* init ep in */
	select_ep(usb_ss, USB_DIR_IN);

	gadget_writel(usb_ss, &usb_ss->regs->ep_cfg,
		EP_CFG__ENABLE__MASK |
		EP_CFG__MAXPKTSIZE__WRITE(max_packet_size));
	gadget_writel(usb_ss, &usb_ss->regs->ep_sts_en,
		EP_STS_EN__SETUPEN__MASK |
		EP_STS_EN__TRBERREN__MASK);
}

/**
 * bmusb_gadget_unconfig - Unconfigures device controller
 * @usb_ss: extended gadget object
 */
static void bmusb_gadget_unconfig(struct usb_ss_dev *usb_ss)
{
	/* RESET CONFIGURATION */
	gadget_writel(usb_ss, &usb_ss->regs->usb_conf,
		USB_CONF__CFGRST__MASK);

	usb_ss->hw_configured_flag = 0;
}

/**
 * bmusb_ep0_run_transfer - Do transfer on default endpoint hardware
 * @usb_ss: extended gadget object
 * @dma_addr: physical address where data is/will be stored
 * @length: data length
 * @erdy: set it to 1 when ERDY packet should be sent -
 *        exit from flow control state
 */
static void bmusb_ep0_run_transfer(struct usb_ss_dev *usb_ss,
		dma_addr_t dma_addr, unsigned int length, int erdy)
{
	usb_ss_ensure_clock_on(usb_ss);

	usb_ss->trb_ep0[0] = TRB_SET_DATA_BUFFER_POINTER(dma_addr);
	usb_ss->trb_ep0[1] = TRB_SET_TRANSFER_LENGTH((u32)length);
	usb_ss->trb_ep0[2] = TRB_SET_CYCLE_BIT |
		TRB_SET_INT_ON_COMPLETION | TRB_TYPE_NORMAL;


	select_ep(usb_ss, usb_ss->ep0_data_dir
		? USB_DIR_IN : USB_DIR_OUT);

	gadget_writel(usb_ss, &usb_ss->regs->ep_traddr,
			EP_TRADDR__TRADDR__WRITE(usb_ss->trb_ep0_dma));
	gadget_writel(usb_ss, &usb_ss->regs->ep_cmd,
		EP_CMD__DRDY__MASK); /* drbl */

	if (erdy)
		gadget_writel(usb_ss, &usb_ss->regs->ep_cmd,
			EP_CMD__ERDY__MASK);
}

/**
 * bmusb_ep_run_transfer - Do transfer on no-default endpoint hardware
 * @usb_ss: extended gadget object
 */
static int bmusb_ep_run_transfer(struct usb_ss_endpoint *usb_ss_ep)
{
	dma_addr_t trb_dma;
	struct usb_request *request = next_request(&usb_ss_ep->request_list);
	struct usb_ss_dev *usb_ss = usb_ss_ep->usb_ss;

	if (request == NULL)
		return -EINVAL;

	if (request->num_sgs > USB_SS_TRBS_NUM)
		return -EINVAL;

	usb_ss_ensure_clock_on(usb_ss);
	if (request->req_map > 0)
		request->dma = request->dma + (request->req_map) *
			MAX_TRANSFER_LENGTH;
	usb_ss_ep->hw_pending_flag = 1;
	trb_dma = request->dma;

	/* must allocate buffer aligned to 8 */
	if (request->dma % ADDR_MODULO_8) {
		memcpy(usb_ss_ep->cpu_addr, request->buf, request->length);
		trb_dma = usb_ss_ep->dma_addr;
	}

	/* fill TRB */
	usb_ss_ep->trb_pool->offset0 = TRB_SET_DATA_BUFFER_POINTER(trb_dma);
	/*
	 * Use precise-8 burst length to prevent cross axi 4KB boundary
	 * when the dma address is not 128B-aligned.
	 */
	if (trb_dma & 0x7F) {
		usb_ss_ep->trb_pool->offset4 =
			(TRB_SET_TRANSFER_LENGTH(request->length)
			| TRB_SET_BURST_LENGTH(8));
		bmusb_dbg(usb_ss->dev, "address is not 128B-aligned 0x%x\n",
			usb_ss_ep->trb_pool->offset0);

	} else
		usb_ss_ep->trb_pool->offset4 =
			(TRB_SET_TRANSFER_LENGTH(request->length)
			| TRB_SET_BURST_LENGTH(16));
	usb_ss_ep->trb_pool->offset8 = TRB_SET_CYCLE_BIT
			| TRB_SET_INT_ON_COMPLETION
			| TRB_SET_INT_ON_SHORT_PACKET
			| TRB_TYPE_NORMAL;

	bmusb_dbg(usb_ss->dev, "0x%x, 0x%x, 0x%x\n",
			usb_ss_ep->trb_pool->offset0,
			usb_ss_ep->trb_pool->offset4,
			usb_ss_ep->trb_pool->offset8);
	/* arm transfer on selected endpoint */
	select_ep(usb_ss_ep->usb_ss,
			usb_ss_ep->endpoint.desc->bEndpointAddress);
	gadget_writel(usb_ss, &usb_ss->regs->ep_traddr,
			EP_TRADDR__TRADDR__WRITE(usb_ss_ep->trb_pool_dma));
	gadget_writel(usb_ss, &usb_ss->regs->ep_cmd,
		EP_CMD__DRDY__MASK); /* DRDY */
	return 0;
}

/**
 * bmusb_get_setup_ret - Returns status of handling setup packet
 * Setup is handled by gadget driver
 * @usb_ss: extended gadget object
 * @ctrl_req: pointer to received setup packet
 */
static int bmusb_get_setup_ret(struct usb_ss_dev *usb_ss,
		struct usb_ctrlrequest *ctrl_req)
{
	int ret;


	spin_unlock(&usb_ss->lock);
	usb_ss->setup_pending = 1;
	ret = usb_ss->gadget_driver->setup(&usb_ss->gadget, ctrl_req);
	usb_ss->setup_pending = 0;
	spin_lock(&usb_ss->lock);
	return ret;
}

/**
 * bmusb_req_ep0_set_address - Handling of SET_ADDRESS standard USB request
 * @usb_ss: extended gadget object
 * @ctrl_req: pointer to received setup packet
 *
 * Returns 0 if success, error code on error
 */
static int bmusb_req_ep0_set_address(struct usb_ss_dev *usb_ss,
		struct usb_ctrlrequest *ctrl_req)
{
	enum usb_device_state device_state = usb_ss->gadget.state;
	u32 reg;
	u32 addr;

	addr = le16_to_cpu(ctrl_req->wValue);

	if (addr > DEVICE_ADDRESS_MAX) {
		bmusb_err(usb_ss->dev,
			"Device address (%d) cannot be greater than %d\n",
				addr, DEVICE_ADDRESS_MAX);
		return -EINVAL;
	}

	if (device_state == USB_STATE_CONFIGURED) {
		bmusb_err(usb_ss->dev, "USB device already configured\n");
		return -EINVAL;
	}

	reg = gadget_readl(usb_ss, &usb_ss->regs->usb_cmd);

	gadget_writel(usb_ss, &usb_ss->regs->usb_cmd, reg
			| USB_CMD__FADDR__WRITE(addr)
			| USB_CMD__SET_ADDR__MASK);

	usb_gadget_set_state(&usb_ss->gadget,
		(addr ? USB_STATE_ADDRESS : USB_STATE_DEFAULT));

	gadget_writel(usb_ss, &usb_ss->regs->ep_cmd,
		EP_CMD__ERDY__MASK | EP_CMD__REQ_CMPL__MASK);
	return 0;
}

/**
 * bmusb_req_ep0_set_address - Handling of GET_STATUS standard USB request
 * @usb_ss: extended gadget object
 * @ctrl_req: pointer to received setup packet
 *
 * Returns 0 if success, error code on error
 */
static int bmusb_req_ep0_get_status(struct usb_ss_dev *usb_ss,
		struct usb_ctrlrequest *ctrl_req)
{
	u16 usb_status = 0;
	unsigned int length = 2;
	u32 recip = ctrl_req->bRequestType & USB_RECIP_MASK;
	u32 reg;

	switch (recip) {

	case USB_RECIP_DEVICE:
		/* handling otg features */
		if (ctrl_req->wIndex == OTG_STS_SELECTOR) {
			length = 1;
			usb_status = usb_ss->gadget.host_request_flag;
		} else {

			reg = gadget_readl(usb_ss, &usb_ss->regs->usb_sts);

			if (reg & USB_STS__U1ENS__MASK)
				usb_status |= 1uL << USB_DEV_STAT_U1_ENABLED;

			if (reg & USB_STS__U2ENS__MASK)
				usb_status |= 1uL << USB_DEV_STAT_U2_ENABLED;

			if (usb_ss->wake_up_flag)
				usb_status |= 1uL << USB_DEVICE_REMOTE_WAKEUP;

			/* self powered */
			usb_status |= 1uL << USB_DEVICE_SELF_POWERED;
		}
		break;

	case USB_RECIP_INTERFACE:
		return bmusb_get_setup_ret(usb_ss, ctrl_req);

	case USB_RECIP_ENDPOINT:
		/* check if endpoint is stalled */
		select_ep(usb_ss, ctrl_req->wIndex);
		if (gadget_readl(usb_ss, &usb_ss->regs->ep_sts)
			& EP_STS__STALL__MASK)
			usb_status = 1;
		break;

	default:
		return -EINVAL;
	}

	*(u16 *)usb_ss->setup = cpu_to_le16(usb_status);

	usb_ss->actual_ep0_request = NULL;
	bmusb_ep0_run_transfer(usb_ss, usb_ss->setup_dma, length, 1);
	return 0;
}

/**
 * bmusb_req_ep0_set_address - Handling of GET/SET_FEATURE standard USB request
 * @usb_ss: extended gadget object
 * @ctrl_req: pointer to received setup packet
 * @set: must be set to 1 for SET_FEATURE request
 *
 * Returns 0 if success, error code on error
 */
static int bmusb_req_ep0_handle_feature(struct usb_ss_dev *usb_ss,
		struct usb_ctrlrequest *ctrl_req, int set)
{
	u32 recip = ctrl_req->bRequestType & USB_RECIP_MASK;
	struct usb_ss_endpoint *usb_ss_ep;
	u32 reg;

	switch (recip) {

	case USB_RECIP_DEVICE:

		switch (ctrl_req->wValue) {

		case USB_DEVICE_U1_ENABLE:
			if (usb_ss->gadget.state != USB_STATE_CONFIGURED)
				return -EINVAL;
			if (usb_ss->gadget.speed != USB_SPEED_SUPER)
				return -EINVAL;

			reg = gadget_readl(usb_ss, &usb_ss->regs->usb_conf);
			if (set)
				/* set U1EN */
				reg |= USB_CONF__U1EN__MASK;
			else
				/* set U1 disable */
				reg |= USB_CONF__U1DS__MASK;
			gadget_writel(usb_ss, &usb_ss->regs->usb_conf, reg);
			break;

		case USB_DEVICE_U2_ENABLE:
			if (usb_ss->gadget.state != USB_STATE_CONFIGURED)
				return -EINVAL;
			if (usb_ss->gadget.speed != USB_SPEED_SUPER)
				return -EINVAL;

			reg = gadget_readl(usb_ss, &usb_ss->regs->usb_conf);
			if (set)
				/* set U2EN */
				reg |= USB_CONF__U2EN__MASK;
			else
				/* set U2 disable */
				reg |= USB_CONF__U2DS__MASK;
			gadget_writel(usb_ss, &usb_ss->regs->usb_conf, reg);
			break;

		case USB_DEVICE_A_ALT_HNP_SUPPORT:
			break;

		case USB_DEVICE_A_HNP_SUPPORT:
			break;

		case USB_DEVICE_B_HNP_ENABLE:
			if (!usb_ss->gadget.b_hnp_enable && set)
				usb_ss->gadget.b_hnp_enable = 1;
			break;

		case USB_DEVICE_REMOTE_WAKEUP:
			usb_ss->wake_up_flag = !!set;
			break;

		default:
			return -EINVAL;

		}
		break;

	case USB_RECIP_INTERFACE:
		return bmusb_get_setup_ret(usb_ss, ctrl_req);

	case USB_RECIP_ENDPOINT:
		select_ep(usb_ss, ctrl_req->wIndex);

		if (set) {
			/* set stall */
			gadget_writel(usb_ss, &usb_ss->regs->ep_cmd,
			EP_CMD__SSTALL__MASK);

			/* handle non zero endpoint software endpoint */
			if (ctrl_req->wIndex & 0x7F) {
				usb_ss_ep = usb_ss->eps[CAST_EP_ADDR_TO_INDEX(
						ctrl_req->wIndex)];
				usb_ss_ep->stalled_flag = 1;
			}
		} else {
			struct usb_request *request;

			if (ctrl_req->wIndex & 0x7F) {
				if (usb_ss->eps[CAST_EP_ADDR_TO_INDEX(
					ctrl_req->wIndex)]->wedge_flag)
					goto jmp_wedge;
			}

			/* clear stall */
			gadget_writel(usb_ss, &usb_ss->regs->ep_cmd,
			EP_CMD__CSTALL__MASK | EP_CMD__EPRST__MASK);
			/* wait for EPRST cleared */
			while (gadget_readl(usb_ss, &usb_ss->regs->ep_cmd)
					& EP_CMD__EPRST__MASK)
				;

			/* handle non zero endpoint software endpoint */
			if (ctrl_req->wIndex & 0x7F) {
				usb_ss_ep = usb_ss->eps[CAST_EP_ADDR_TO_INDEX(
						ctrl_req->wIndex)];
				usb_ss_ep->stalled_flag = 0;

				request = next_request(
						&usb_ss_ep->request_list);
				if (request)
					bmusb_ep_run_transfer(usb_ss_ep);
			}
		}
jmp_wedge:
		select_ep(usb_ss, 0x00);
		break;

	default:
		return -EINVAL;
	}

	gadget_writel(usb_ss, &usb_ss->regs->ep_cmd,
		EP_CMD__ERDY__MASK | EP_CMD__REQ_CMPL__MASK);

	return 0;
}

/**
 * bmusb_req_ep0_set_sel - Handling of SET_SEL standard USB request
 * @usb_ss: extended gadget object
 * @ctrl_req: pointer to received setup packet
 *
 * Returns 0 if success, error code on error
 */
static int bmusb_req_ep0_set_sel(struct usb_ss_dev *usb_ss,
		struct usb_ctrlrequest *ctrl_req)
{
	if (usb_ss->gadget.state < USB_STATE_ADDRESS)
		return -EINVAL;

	if (ctrl_req->wLength != 6) {
		bmusb_err(usb_ss->dev, "Set SEL should be 6 bytes, got %d\n",
				ctrl_req->wLength);
		return -EINVAL;
	}

	usb_ss->ep0_data_dir = 0;
	usb_ss->actual_ep0_request = NULL;
	bmusb_ep0_run_transfer(usb_ss, usb_ss->setup_dma, 6, 1);

	return 0;
}

/**
 * bmusb_req_ep0_set_isoch_delay -
 * Handling of GET_ISOCH_DELAY standard USB request
 * @usb_ss: extended gadget object
 * @ctrl_req: pointer to received setup packet
 *
 * Returns 0 if success, error code on error
 */
static int bmusb_req_ep0_set_isoch_delay(struct usb_ss_dev *usb_ss,
		struct usb_ctrlrequest *ctrl_req)
{
	if (ctrl_req->wIndex || ctrl_req->wLength)
		return -EINVAL;

	usb_ss->isoch_delay = ctrl_req->wValue;
	gadget_writel(usb_ss, &usb_ss->regs->ep_cmd,
	EP_CMD__ERDY__MASK | EP_CMD__REQ_CMPL__MASK);
	return 0;
}

static int bmusb_gadget_config(struct usb_ss_dev *usb_ss)
{
	int i;

	for (i = 0; i < usb_ss->ep_nums; i++)
		if (usb_ss->eps[i]->endpoint.enabled)
			bmusb_ep_config(usb_ss->eps[i], i);
#ifdef BM_THREADED_IRQ_HANDLING
	usb_ss->ep_ien = gadget_readl(usb_ss, &usb_ss->regs->ep_ien)
	| EP_IEN__EOUTEN0__MASK | EP_IEN__EINEN0__MASK;
#endif
	/* SET CONFIGURATION */
	select_ep(usb_ss, 0x00);
	gadget_writel(usb_ss, &usb_ss->regs->usb_conf,
		USB_CONF__CFGSET__MASK);
	gadget_writel(usb_ss, &usb_ss->regs->ep_cmd,
		EP_CMD__ERDY__MASK |
		EP_CMD__REQ_CMPL__MASK);
	while (!(gadget_readl(usb_ss,
		&usb_ss->regs->usb_sts)
		& USB_STS__CFGSTS__MASK))
		;
	/* check whether the resource is enough. */
	if ((gadget_readl(usb_ss,
		&usb_ss->regs->usb_sts)
		& USB_STS__MEMOV__MASK))
		bmusb_err(usb_ss->dev, "resource overflow!\n");

	return 0;
}

/**
 * bmusb_req_ep0_set_configuration - Handling of SET_CONFIG standard USB request
 * @usb_ss: extended gadget object
 * @ctrl_req: pointer to received setup packet
 *
 * Returns 0 if success, 0x7FFF on deferred status stage, error code on error
 */
static int bmusb_req_ep0_set_configuration(struct usb_ss_dev *usb_ss,
		struct usb_ctrlrequest *ctrl_req)
{
	enum usb_device_state device_state = usb_ss->gadget.state;
	u32 config = le16_to_cpu(ctrl_req->wValue);
	int i, result = 0;

	switch (device_state) {

	case USB_STATE_ADDRESS:

		result = bmusb_get_setup_ret(usb_ss, ctrl_req);

		if (result != 0)
			return result;

		if (config) {
			if (!usb_ss->hw_configured_flag) {
				bmusb_gadget_config(usb_ss);
				usb_ss->hw_configured_flag = 1;
			}
		} else {
			bmusb_gadget_unconfig(usb_ss);
			for (i = 0; i < usb_ss->ep_nums; i++)
				usb_ss->eps[i]->endpoint.enabled = 0;
			usb_gadget_set_state(&usb_ss->gadget,
				USB_STATE_ADDRESS);
		}
		break;

	case USB_STATE_CONFIGURED:
		result = bmusb_get_setup_ret(usb_ss, ctrl_req);
		if (!config && !result) {
			bmusb_gadget_unconfig(usb_ss);
			for (i = 0; i < usb_ss->ep_nums; i++)
				usb_ss->eps[i]->endpoint.enabled = 0;
			usb_gadget_set_state(&usb_ss->gadget,
				USB_STATE_ADDRESS);
		}
		break;

	default:
		result = -EINVAL;
	}

	return result;
}

/**
 * bmusb_ep0_standard_request - Handling standard USB requests
 * @usb_ss: extended gadget object
 * @ctrl_req: pointer to received setup packet
 */
static int bmusb_ep0_standard_request(struct usb_ss_dev *usb_ss,
		struct usb_ctrlrequest *ctrl_req)
{
	switch (ctrl_req->bRequest) {
	case USB_REQ_SET_ADDRESS:
		return bmusb_req_ep0_set_address(usb_ss, ctrl_req);

	case USB_REQ_SET_CONFIGURATION:
		return bmusb_req_ep0_set_configuration(usb_ss, ctrl_req);

	case USB_REQ_GET_STATUS:
		return bmusb_req_ep0_get_status(usb_ss, ctrl_req);

	case USB_REQ_CLEAR_FEATURE:
		return bmusb_req_ep0_handle_feature(usb_ss, ctrl_req, 0);

	case USB_REQ_SET_FEATURE:
		return bmusb_req_ep0_handle_feature(usb_ss, ctrl_req, 1);

	case USB_REQ_SET_SEL:
		return bmusb_req_ep0_set_sel(usb_ss, ctrl_req);

	case USB_REQ_SET_ISOCH_DELAY:
		return bmusb_req_ep0_set_isoch_delay(usb_ss, ctrl_req);

	default:
		return bmusb_get_setup_ret(usb_ss, ctrl_req);
	}
}

/**
 * bmusb_ep0_setup_phase - Handling setup USB requests
 * @usb_ss: extended gadget object
 */
static void bmusb_ep0_setup_phase(struct usb_ss_dev *usb_ss)
{
	int result;
	struct usb_ctrlrequest *ctrl_req =
			(struct usb_ctrlrequest *)usb_ss->setup;

	if ((ctrl_req->bRequestType & USB_TYPE_MASK) == USB_TYPE_STANDARD)
		result = bmusb_ep0_standard_request(usb_ss, ctrl_req);
	else
		result = bmusb_get_setup_ret(usb_ss, ctrl_req);

	if (result != 0 && result != USB_GADGET_DELAYED_STATUS) {
		bmusb_dbg(usb_ss->dev, "STALL(00) %d\n", result);

		/* set_stall on ep0 */
		select_ep(usb_ss, 0x00);
		gadget_writel(usb_ss, &usb_ss->regs->ep_cmd,
			EP_CMD__SSTALL__MASK);
		gadget_writel(usb_ss, &usb_ss->regs->ep_cmd,
			EP_CMD__ERDY__MASK | EP_CMD__REQ_CMPL__MASK);
		return;
	}
}

/**
 * bmusb_check_ep_interrupt_proceed - Processes interrupt related to endpoint
 * @usb_ss_ep: extended endpoint object
 */
static int bmusb_check_ep_interrupt_proceed(struct usb_ss_endpoint *usb_ss_ep)
{
	struct usb_ss_dev *usb_ss = usb_ss_ep->usb_ss;
	struct usb_request *request;
	u32 ep_sts_reg;

	select_ep(usb_ss, usb_ss_ep->endpoint.address);
	ep_sts_reg = gadget_readl(usb_ss, &usb_ss->regs->ep_sts);


	if (ep_sts_reg & EP_STS__TRBERR__MASK) {
		gadget_writel(usb_ss,
			&usb_ss->regs->ep_sts, EP_STS__TRBERR__MASK);

		bmusb_dbg(usb_ss->dev, "TRBERR(%02X)\n",
			usb_ss_ep->endpoint.desc->bEndpointAddress);
	}

	if (ep_sts_reg & EP_STS__ISOERR__MASK) {

		gadget_writel(usb_ss,
			&usb_ss->regs->ep_sts, EP_STS__ISOERR__MASK);
		bmusb_dbg(usb_ss->dev, "ISOERR(%02X)\n",
			usb_ss_ep->endpoint.desc->bEndpointAddress);
	}

	if (ep_sts_reg & EP_STS__OUTSMM__MASK) {

		gadget_writel(usb_ss, &usb_ss->regs->ep_sts,
			EP_STS__OUTSMM__MASK);
		bmusb_dbg(usb_ss->dev, "OUTSMM(%02X)\n",
			usb_ss_ep->endpoint.desc->bEndpointAddress);
	}

	if (ep_sts_reg & EP_STS__NRDY__MASK) {

		gadget_writel(usb_ss,
			&usb_ss->regs->ep_sts, EP_STS__NRDY__MASK);
		bmusb_dbg(usb_ss->dev, "NRDY(%02X)\n",
			usb_ss_ep->endpoint.desc->bEndpointAddress);
	}

	if ((ep_sts_reg & EP_STS__IOC__MASK)
			|| (ep_sts_reg & EP_STS__ISP__MASK)) {
		gadget_writel(usb_ss, &usb_ss->regs->ep_sts,
		EP_STS__IOC__MASK | EP_STS__ISP__MASK);

		/* get just completed request */
		request = next_request(&usb_ss_ep->request_list);
		if (!request || request == NULL)
			return 0;
		if (request->dma) {
			usb_gadget_unmap_request(&usb_ss->gadget, request,
				usb_ss_ep->endpoint.desc->bEndpointAddress
				& ENDPOINT_DIR_MASK);
			request->dma = 0;
		}

		request->status = 0;
#if 1
		if (usb_ss_ep->endpoint.address & 0x80) {
			if (request->actual > MAX_TRANSFER_LENGTH)
				request->actual = request->actual -
					MAX_TRANSFER_LENGTH;
			else
				request->actual = request->actual -
					request->actual;
		} else
			request->actual = le32_to_cpu(((u32 *)
						usb_ss_ep->trb_pool)[1])
						& ACTUAL_TRANSFERRED_BYTES_MASK;
#else
		request->actual = le32_to_cpu(((u32 *) usb_ss_ep->trb_pool)[1])
			& ACTUAL_TRANSFERRED_BYTES_MASK;
#endif

		list_del(&request->list);


		usb_ss_ep->hw_pending_flag = 0;
		if (request->complete) {
			spin_unlock(&usb_ss->lock);
			request->complete(&usb_ss_ep->endpoint, request);
			spin_lock(&usb_ss->lock);
		}

		/* handle deferred STALL */
		if (usb_ss_ep->stalled_flag) {
			bmusb_ep_stall_flush(usb_ss_ep);
			return 0;
		}

		/* exit if hardware transfer already started */
		if (usb_ss_ep->hw_pending_flag)
			return 0;
		/* if any request queued run it! */
		if (!list_empty(&usb_ss_ep->request_list))
			bmusb_ep_run_transfer(usb_ss_ep);
	}

	if (ep_sts_reg & EP_STS__DESCMIS__MASK) {

		gadget_writel(usb_ss,
			&usb_ss->regs->ep_sts, EP_STS__DESCMIS__MASK);
		bmusb_dbg(usb_ss->dev, "DESCMIS(%02X)\n",
			usb_ss_ep->endpoint.desc->bEndpointAddress);
	}

	return 0;
}

/**
 * bmusb_check_ep0_interrupt_proceed - Processes interrupt related to endpoint 0
 * @usb_ss: extended gadget object
 * @dir: 1 for IN direction, 0 for OUT direction
 */
static void bmusb_check_ep0_interrupt_proceed(struct usb_ss_dev *usb_ss,
					      int dir)
{
	u32 ep_sts_reg;

	select_ep(usb_ss, 0 | (dir ? USB_DIR_IN : USB_DIR_OUT));
	ep_sts_reg = gadget_readl(usb_ss, &usb_ss->regs->ep_sts);


	if ((ep_sts_reg & EP_STS__SETUP__MASK) && (dir == 0)) {

		bmusb_dbg(usb_ss->dev, "SETUP(%02X)\n", 0x00);

		gadget_writel(usb_ss, &usb_ss->regs->ep_sts,
			EP_STS__SETUP__MASK |
			EP_STS__IOC__MASK | EP_STS__ISP__MASK);

		usb_ss->ep0_data_dir = usb_ss->setup[0] & USB_DIR_IN;
		bmusb_ep0_setup_phase(usb_ss);
		ep_sts_reg &= ~(EP_STS__SETUP__MASK |
			EP_STS__IOC__MASK |
			EP_STS__ISP__MASK);
	}

	if (ep_sts_reg & EP_STS__TRBERR__MASK) {
		gadget_writel(usb_ss,
			&usb_ss->regs->ep_sts, EP_STS__TRBERR__MASK);
	}

	if (ep_sts_reg & EP_STS__DESCMIS__MASK) {

		gadget_writel(usb_ss,
			&usb_ss->regs->ep_sts, EP_STS__DESCMIS__MASK);


		if (dir == 0 && !usb_ss->setup_pending) {
			usb_ss->ep0_data_dir = 0;
			bmusb_ep0_run_transfer(usb_ss,
				usb_ss->setup_dma, 8, 0);
		}
	}

	if ((ep_sts_reg & EP_STS__IOC__MASK)
			|| (ep_sts_reg & EP_STS__ISP__MASK)) {

		gadget_writel(usb_ss,
			&usb_ss->regs->ep_sts, EP_STS__IOC__MASK);
		gadget_writel(usb_ss,
			&usb_ss->regs->ep_cmd, EP_CMD__REQ_CMPL__MASK);

		if (usb_ss->actual_ep0_request) {
			if (usb_ss->actual_ep0_request->dma) {
				usb_gadget_unmap_request(&usb_ss->gadget,
						usb_ss->actual_ep0_request,
						usb_ss->ep0_data_dir);
				usb_ss->actual_ep0_request->dma = 0;
			}

			usb_ss->actual_ep0_request->actual =
				le32_to_cpu((usb_ss->trb_ep0)[1])
				& ACTUAL_TRANSFERRED_BYTES_MASK;

		}

		if (usb_ss->actual_ep0_request
				&& usb_ss->actual_ep0_request->complete) {
			spin_unlock(&usb_ss->lock);
			usb_ss->actual_ep0_request->complete(usb_ss->gadget.ep0,
					usb_ss->actual_ep0_request);
			spin_lock(&usb_ss->lock);
		}
	}
}

/**
 * bmusb_check_usb_interrupt_proceed - Processes interrupt related to device
 * @usb_ss: extended gadget object
 * @usb_ists: bitmap representation of device's reported interrupts
 * (usb_ists register value)
 */
static void bmusb_check_usb_interrupt_proceed(struct usb_ss_dev *usb_ss,
		u32 usb_ists)
{
	int interrupt_bit = ffs(usb_ists) - 1;
	int speed;

	bmusb_dbg(usb_ss->dev, "USB interrupt detected\n");

	switch (interrupt_bit) {
	case USB_ISTS__CON2I__SHIFT:
		/* FS/HS Connection detected */
		bmusb_dbg(usb_ss->dev, "[Interrupt] FS/HS Connection detected\n");
		speed = USB_STS__USBSPEED__READ(
			gadget_readl(usb_ss, &usb_ss->regs->usb_sts));
		if (speed == USB_SPEED_WIRELESS)
			speed = USB_SPEED_SUPER;
		bmusb_dbg(usb_ss->dev, "Setting speed value to: %s (%d)\n",
			usb_speed_string(speed), speed);
		usb_ss->gadget.speed = speed;
		if (usb_ss->in_standby_mode && !usb_ss->is_connected)
			usb_ss_turn_on_ref_clock(usb_ss);
		usb_ss->is_connected = 1;
		usb_gadget_set_state(&usb_ss->gadget, USB_STATE_POWERED);
		bmusb_ep0_config(usb_ss);
		break;

	case USB_ISTS__CONI__SHIFT:
		/* SS Connection detected */
		bmusb_dbg(usb_ss->dev, "[Interrupt] SS Connection detected\n");
		speed = USB_STS__USBSPEED__READ(
				gadget_readl(usb_ss, &usb_ss->regs->usb_sts));
		if (speed == USB_SPEED_WIRELESS)
			speed = USB_SPEED_SUPER;
		bmusb_dbg(usb_ss->dev, "Setting speed value to: %s (%d)\n",
			usb_speed_string(speed), speed);
		usb_ss->gadget.speed = speed;
		if (usb_ss->in_standby_mode && !usb_ss->is_connected)
			usb_ss_turn_on_ref_clock(usb_ss);
		usb_ss->is_connected = 1;
		usb_gadget_set_state(&usb_ss->gadget, USB_STATE_POWERED);
		bmusb_ep0_config(usb_ss);
		break;

	case USB_ISTS__DIS2I__SHIFT:
	case USB_ISTS__DISI__SHIFT:
		/* SS Disconnection detected */
		bmusb_dbg(usb_ss->dev,
			"[Interrupt] Disconnection detected\n");
		if (usb_ss->gadget_driver
			&& usb_ss->gadget_driver->disconnect) {

			spin_unlock(&usb_ss->lock);
			usb_ss->gadget_driver->disconnect(&usb_ss->gadget);
			spin_lock(&usb_ss->lock);
		}
		usb_ss->gadget.speed = USB_SPEED_UNKNOWN;
		usb_gadget_set_state(&usb_ss->gadget, USB_STATE_NOTATTACHED);
		usb_ss->is_connected = 0;
		bmusb_gadget_unconfig(usb_ss);
		break;

	case USB_ISTS__L2ENTI__SHIFT:
		bmusb_dbg(usb_ss->dev,
			 "[Interrupt] Device suspended\n");
		usb_ss->gadget.suspended = 1;
		break;

	case USB_ISTS__L2EXTI__SHIFT:
		bmusb_dbg(usb_ss->dev, "[Interrupt] L2 exit detected\n");
		usb_ss->gadget.suspended = 0;
		/*
		 * Exit from standby mode
		 * on L2 exit (Suspend in HS/FS or SS)
		 */
		usb_ss_turn_on_ref_clock(usb_ss);
		break;
	case USB_ISTS__U3EXTI__SHIFT:
		/*
		 * Exit from standby mode
		 * on U3 exit (Suspend in HS/FS or SS)
		 */
		bmusb_dbg(usb_ss->dev, "[Interrupt] U3 exit detected\n");
		usb_ss_turn_on_ref_clock(usb_ss);
		break;

		/* resets cases */
	case USB_ISTS__UWRESI__SHIFT:
	case USB_ISTS__UHRESI__SHIFT:
	case USB_ISTS__U2RESI__SHIFT:
		/* SS Reset detected */
		bmusb_dbg(usb_ss->dev,
			"[Interrupt] Reset detected\n");
		if (usb_ss->gadget_driver
			&& usb_ss->gadget_driver->reset) {

			spin_unlock(&usb_ss->lock);
			usb_ss->gadget_driver->reset(&usb_ss->gadget);
			spin_lock(&usb_ss->lock);
		}
		bmusb_gadget_unconfig(usb_ss);
		speed = USB_STS__USBSPEED__READ(
				gadget_readl(usb_ss, &usb_ss->regs->usb_sts));
		if (speed == USB_SPEED_WIRELESS)
			speed = USB_SPEED_SUPER;
		usb_gadget_set_state(&usb_ss->gadget, USB_STATE_DEFAULT);
		usb_ss->gadget.speed = speed;
		bmusb_gadget_unconfig(usb_ss);
		bmusb_ep0_config(usb_ss);
		break;
	default:
		break;
	}

	/* Clear interrupt bit */
	gadget_writel(usb_ss, &usb_ss->regs->usb_ists, (1uL << interrupt_bit));
}

#ifdef BM_THREADED_IRQ_HANDLING
static irqreturn_t bmusb_irq_handler(int irq, void *_usb_ss)
{
	struct usb_ss_dev *usb_ss = _usb_ss;

	if (!usb_ss->gadget_driver) {
		bmusb_dbg(usb_ss->dev, "%s: gadget is stop!!\n", __func__);
		return IRQ_HANDLED;
	}

	usb_ss->usb_ien = gadget_readl(usb_ss, &usb_ss->regs->usb_ien);
	usb_ss->ep_ien = gadget_readl(usb_ss, &usb_ss->regs->ep_ien);

	if (!gadget_readl(usb_ss, &usb_ss->regs->usb_ists)
		&& !gadget_readl(usb_ss, &usb_ss->regs->ep_ists)) {
		bmusb_dbg(usb_ss->dev, "--BUBBLE INTERRUPT 0 !!!\n");
		if (gadget_readl(usb_ss, &usb_ss->regs->usb_sts) & 1uL)
			return IRQ_HANDLED;
		return IRQ_NONE;
	}

	gadget_writel(usb_ss, &usb_ss->regs->usb_ien, 0);
	gadget_writel(usb_ss, &usb_ss->regs->ep_ien, 0);

	gadget_readl(usb_ss, &usb_ss->regs->dma_axi_ctrl);
	return IRQ_WAKE_THREAD;
}
#endif

/**
 * bmusb_irq_handler - irq line interrupt handler
 * @irq: interrupt line number
 * @_usb_ss: pointer to extended gadget object
 *
 * Returns IRQ_HANDLED when interrupt raised by USBSS_DEV,
 * IRQ_NONE when interrupt raised by other device connected
 * to the irq line
 */
static irqreturn_t bmusb_irq_handler_thread(int irq, void *_usb_ss)
{
	struct usb_ss_dev *usb_ss = _usb_ss;
	u32 reg;
	enum irqreturn ret = IRQ_NONE;
	unsigned long flags;

	spin_lock_irqsave(&usb_ss->lock, flags);

	if (!usb_ss->gadget_driver) {
		bmusb_dbg(usb_ss->dev, "%s: gadget is stop!!\n", __func__);
		ret = IRQ_HANDLED;
		goto irqend;
	}

	/* check USB device interrupt */
	reg = gadget_readl(usb_ss, &usb_ss->regs->usb_ists);
	if (reg) {
		bmusb_dbg(usb_ss->dev, "usb_ists: %08X\n", reg);
		bmusb_check_usb_interrupt_proceed(usb_ss, reg);
		ret = IRQ_HANDLED;
	}

	/* check endpoint interrupt */
	reg = gadget_readl(usb_ss, &usb_ss->regs->ep_ists);
	if (reg != 0)
		bmusb_dbg(usb_ss->dev, "ep_ists: %08X\n", reg);

	if (reg == 0) {
		if (gadget_readl(usb_ss, &usb_ss->regs->usb_sts) & 1uL)
			ret = IRQ_HANDLED;
		goto irqend;
	}

	/* handle default endpoint OUT */
	if (reg & EP_ISTS__EOUT0__MASK) {
		bmusb_check_ep0_interrupt_proceed(usb_ss, 0);
		ret = IRQ_HANDLED;
	}

	/* handle default endpoint IN */
	if (reg & EP_ISTS__EIN0__MASK) {
		bmusb_check_ep0_interrupt_proceed(usb_ss, 1);
		ret = IRQ_HANDLED;
	}

	/* check if interrupt from non default endpoint, if no exit */
	reg &= ~(EP_ISTS__EOUT0__MASK | EP_ISTS__EIN0__MASK);
	if (!reg)
		goto irqend;

	do {
		unsigned int bit_pos = ffs(reg);
		u32 bit_mask = 1 << (bit_pos - 1);

		bmusb_check_ep_interrupt_proceed(
				usb_ss->eps[CAST_EP_REG_POS_TO_INDEX(bit_pos)]);
		reg &= ~bit_mask;
		ret = IRQ_HANDLED;
	} while (reg);
irqend:

	spin_unlock_irqrestore(&usb_ss->lock, flags);
#ifdef BM_THREADED_IRQ_HANDLING
	local_irq_save(flags);
	gadget_writel(usb_ss, &usb_ss->regs->usb_ien, usb_ss->usb_ien);
	gadget_writel(usb_ss, &usb_ss->regs->ep_ien, usb_ss->ep_ien);
	local_irq_restore(flags);
#endif
	return ret;
}

/**
 * usb_ss_gadget_ep0_enable
 * Function shouldn't be called by gadget driver,
 * endpoint 0 is allways active
 */
static int usb_ss_gadget_ep0_enable(struct usb_ep *ep,
		const struct usb_endpoint_descriptor *desc)
{
	return -EINVAL;
}

/**
 * usb_ss_gadget_ep0_disable
 * Function shouldn't be called by gadget driver,
 * endpoint 0 is allways active
 */
static int usb_ss_gadget_ep0_disable(struct usb_ep *ep)
{
	return -EINVAL;
}

/**
 * usb_ss_gadget_ep0_set_halt
 * @ep: pointer to endpoint zero object
 * @value: 1 for set stall, 0 for clear stall
 *
 * Returns 0
 */
static int usb_ss_gadget_ep0_set_halt(struct usb_ep *ep, int value)
{
	/* TODO */
	return 0;
}

/**
 * usb_ss_gadget_ep0_queue Transfer data on endpoint zero
 * @ep: pointer to endpoint zero object
 * @request: pointer to request object
 * @gfp_flags: gfp flags
 *
 * Returns 0 on success, error code elsewhere
 */
static int usb_ss_gadget_ep0_queue(struct usb_ep *ep,
		struct usb_request *request, gfp_t gfp_flags)
{
	int ret;
	unsigned long flags;
	int erdy_sent = 0;
	/* get extended endpoint */
	struct usb_ss_endpoint *usb_ss_ep =
		to_usb_ss_ep(ep);
	struct usb_ss_dev *usb_ss = usb_ss_ep->usb_ss;


	/* send STATUS stage */
	if (request->length == 0 && request->zero == 0) {
		spin_lock_irqsave(&usb_ss->lock, flags);
		if (!usb_ss->hw_configured_flag) {
			bmusb_gadget_config(usb_ss);
			usb_ss->hw_configured_flag = 1;
			erdy_sent = 1;
		}
		select_ep(usb_ss, 0x00);
		if (!erdy_sent)
			gadget_writel(usb_ss, &usb_ss->regs->ep_cmd,
			EP_CMD__ERDY__MASK | EP_CMD__REQ_CMPL__MASK);
		if (request->complete)
			request->complete(usb_ss->gadget.ep0, request);
		spin_unlock_irqrestore(&usb_ss->lock, flags);
		return 0;
	}

	spin_lock_irqsave(&usb_ss->lock, flags);
	ret = usb_gadget_map_request(&usb_ss->gadget, request,
			usb_ss->ep0_data_dir);
	if (ret) {
		bmusb_err(usb_ss->dev, "failed to map request\n");
		return -EINVAL;
	}

	usb_ss->actual_ep0_request = request;

	bmusb_ep0_run_transfer(usb_ss, request->dma, request->length, 1);

	spin_unlock_irqrestore(&usb_ss->lock, flags);

	return 0;
}

/**
 * bmusb_ep_config Configure hardware endpoint
 * @usb_ss_ep: extended endpoint object
 * @desc: endpoint descriptor
 * @comp_desc: endpoint companion descriptor
 */
static void bmusb_ep_config(struct usb_ss_endpoint *usb_ss_ep, int index)
{
	struct usb_ss_dev *usb_ss = usb_ss_ep->usb_ss;
	u32 ep_cfg = 0;
	u32 max_packet_size = 0;
	u32 bEndpointAddress = (u32)CAST_INDEX_TO_EP_ADDR(index);
	u32 interrupt_mask = 0;


	usb_ss_ep->endpoint.address = bEndpointAddress;
	if (usb_ss_ep->is_iso_flag) {
		ep_cfg = EP_CFG__EPTYPE__WRITE(USB_ENDPOINT_XFER_ISOC);
		interrupt_mask = INTERRUPT_MASK;
	} else {
		ep_cfg = EP_CFG__EPTYPE__WRITE(USB_ENDPOINT_XFER_BULK);
	}

	switch (usb_ss->gadget.speed) {
	case USB_SPEED_UNKNOWN:
		max_packet_size = ENDPOINT_MAX_PACKET_SIZE_0;
		break;

	case USB_SPEED_LOW:
		max_packet_size = ENDPOINT_MAX_PACKET_SIZE_8;
		break;

	case USB_SPEED_FULL:
		max_packet_size = (usb_ss_ep->is_iso_flag ?
			ENDPOINT_MAX_PACKET_SIZE_1023 :
			ENDPOINT_MAX_PACKET_SIZE_64);
		break;

	case USB_SPEED_HIGH:
		max_packet_size = (usb_ss_ep->is_iso_flag ?
			ENDPOINT_MAX_PACKET_SIZE_1024 :
			ENDPOINT_MAX_PACKET_SIZE_512);
		break;

	case USB_SPEED_WIRELESS:
		max_packet_size = ENDPOINT_MAX_PACKET_SIZE_512;
		break;

	case USB_SPEED_SUPER:
	case USB_SPEED_SUPER_PLUS:
		max_packet_size = ENDPOINT_MAX_PACKET_SIZE_1024;
		break;
	}

	ep_cfg |= EP_CFG__MAXPKTSIZE__WRITE(max_packet_size);

	if (usb_ss_ep->is_iso_flag) {
		ep_cfg |= EP_CFG__BUFFERING__WRITE(1);
		ep_cfg |= EP_CFG__MAXBURST__WRITE(0);
	} else {

	ep_cfg |= EP_CFG__BUFFERING__WRITE(3);
	ep_cfg |= EP_CFG__MAXBURST__WRITE(15);
	}

	ep_cfg |= EP_CFG__ENABLE__MASK;

	select_ep(usb_ss, bEndpointAddress);
	gadget_writel(usb_ss, &usb_ss->regs->ep_cfg, ep_cfg);
	gadget_writel(usb_ss, &usb_ss->regs->ep_sts_en,
		EP_STS_EN__TRBERREN__MASK | interrupt_mask);

	/* enable interrupt for selected endpoint */
	ep_cfg = gadget_readl(usb_ss, &usb_ss->regs->ep_ien);
	ep_cfg |= CAST_EP_ADDR_TO_BIT_POS(bEndpointAddress);
	gadget_writel(usb_ss, &usb_ss->regs->ep_ien, ep_cfg);
}

/**
 * usb_ss_gadget_ep_enable Enable endpoint
 * @ep: endpoint object
 * @desc: endpoint descriptor
 *
 * Returns 0 on success, error code elsewhere
 */
static int usb_ss_gadget_ep_enable(struct usb_ep *ep,
		const struct usb_endpoint_descriptor *desc)
{
	struct usb_ss_endpoint *usb_ss_ep;
	struct usb_ss_dev *usb_ss;
	unsigned long flags;
	int ret;

	usb_ss_ep = to_usb_ss_ep(ep);
	usb_ss = usb_ss_ep->usb_ss;

	if (!ep || !desc || desc->bDescriptorType != USB_DT_ENDPOINT) {
		bmusb_err(usb_ss->dev, "usb-ss: invalid parameters\n");
		return -EINVAL;
	}

	if (!desc->wMaxPacketSize) {
		bmusb_err(usb_ss->dev, "usb-ss: missing wMaxPacketSize\n");
		return -EINVAL;
	}

	ret = usb_ss_allocate_trb_pool(usb_ss_ep);
	if (ret)
		return ret;

	//dma_set_coherent_mask(usb_ss->dev, DMA_BIT_MASK(64));

	if (!usb_ss_ep->cpu_addr) {
		usb_ss_ep->cpu_addr = dma_alloc_coherent(usb_ss->dev, 4096,
			&usb_ss_ep->dma_addr, GFP_DMA);

		if (!usb_ss_ep->cpu_addr)
			return -ENOMEM;
	}


	spin_lock_irqsave(&usb_ss->lock, flags);
	ep->enabled = 1;
	usb_ss_ep->disabling = 0;
	usb_ss_ep->hw_pending_flag = 0;
	usb_ss_ep->endpoint.desc = desc;
	spin_unlock_irqrestore(&usb_ss->lock, flags);

	return 0;
}

/**
 * usb_ss_gadget_ep_disable Disable endpoint
 * @ep: endpoint object
 *
 * Returns 0 on success, error code elsewhere
 */
static int usb_ss_gadget_ep_disable(struct usb_ep *ep)
{
	struct usb_ss_endpoint *usb_ss_ep;
	struct usb_ss_dev *usb_ss;
	unsigned long flags;
	int ret = 0;
	struct usb_request *request;

	if (!ep) {
		pr_debug("usb-ss: invalid parameters\n");
		return -EINVAL;
	}

	usb_ss_ep = to_usb_ss_ep(ep);
	usb_ss = usb_ss_ep->usb_ss;

	spin_lock_irqsave(&usb_ss->lock, flags);

	/**
	 * Use disabling flag to prevent re-entry of ep disable
	 * during the unlock period of request giveback.
	 */
	bmusb_dbg(usb_ss->dev,
		  "Disabling endpoint: %s, [%d, %d]\n", ep->name, ep->enabled,
		  usb_ss_ep->disabling);

	if (!ep->enabled || usb_ss_ep->disabling) {
		spin_unlock_irqrestore(&usb_ss->lock, flags);
		return 0;
	}
	usb_ss_ep->disabling = 1;
	select_ep(usb_ss, ep->desc->bEndpointAddress);
	gadget_writel(usb_ss, &usb_ss->regs->ep_cmd,
		EP_CMD__EPRST__MASK);
	while (gadget_readl(usb_ss,
		&usb_ss->regs->ep_cmd) & EP_CMD__EPRST__MASK)
		;

	while (!list_empty(&usb_ss_ep->request_list)) {

		request = next_request(&usb_ss_ep->request_list);
		if (!request || request == NULL)
			return ret;
		if (request->dma) {
			usb_gadget_unmap_request(&usb_ss->gadget, request,
				ep->desc->bEndpointAddress & USB_DIR_IN);
			request->dma = 0;
		}
		request->status = -ESHUTDOWN;
		list_del(&request->list);
		spin_unlock(&usb_ss->lock);
		usb_gadget_giveback_request(ep, request);
		spin_lock(&usb_ss->lock);
	}

	ep->desc = NULL;
	ep->enabled = 0;
	usb_ss_ep->disabling = 0;

	spin_unlock_irqrestore(&usb_ss->lock, flags);

	return ret;
}

/**
 * usb_ss_gadget_ep_alloc_request Allocates request
 * @ep: endpoint object associated with request
 * @gfp_flags: gfp flags
 *
 * Returns allocated request address, NULL on allocation error
 */
static struct usb_request *usb_ss_gadget_ep_alloc_request(struct usb_ep *ep,
		gfp_t gfp_flags)
{
	struct usb_request *request;

	request = kzalloc(sizeof(struct usb_request), gfp_flags);
	if (!request)
		return NULL;

	return request;
}

/**
 * usb_ss_gadget_ep_free_request Free memory occupied by request
 * @ep: endpoint object associated with request
 * @request: request to free memory
 */
static void usb_ss_gadget_ep_free_request(struct usb_ep *ep,
		struct usb_request *request)
{
	kfree(request);
}

/**
 * usb_ss_gadget_ep_queue Transfer data on endpoint
 * @ep: endpoint object
 * @request: request object
 * @gfp_flags: gfp flags
 *
 * Returns 0 on success, error code elsewhere
 */
static int usb_ss_gadget_ep_queue(struct usb_ep *ep,
		struct usb_request *request, gfp_t gfp_flags)
{
	struct usb_ss_endpoint *usb_ss_ep =
		to_usb_ss_ep(ep);
	struct usb_ss_dev *usb_ss = usb_ss_ep->usb_ss;
	unsigned long flags;
	int ret = 0;
	int empty_list = 0;

	spin_lock_irqsave(&usb_ss->lock, flags);
	/**
	 * shall not queue the request while the ep
	 * is disabling.
	 */
	if (!ep->enabled || usb_ss_ep->disabling) {
		spin_unlock_irqrestore(&usb_ss->lock, flags);
		return 0;
	}

	request->actual = 0;
	request->status = -EINPROGRESS;

	if (!request->dma) {
		ret = usb_gadget_map_request(&usb_ss->gadget, request,
				ep->desc->bEndpointAddress & USB_DIR_IN);

		if (ret) {
			spin_unlock_irqrestore(&usb_ss->lock, flags);
			return ret;
		}
	}

	empty_list = list_empty(&usb_ss_ep->request_list);
	list_add_tail(&request->list, &usb_ss_ep->request_list);


	if (!usb_ss->hw_configured_flag) {
		spin_unlock_irqrestore(&usb_ss->lock, flags);
		return 0;
	}

	if (empty_list) {
		if (!usb_ss_ep->stalled_flag)
			bmusb_ep_run_transfer(usb_ss_ep);
	}
	spin_unlock_irqrestore(&usb_ss->lock, flags);

	return ret;
}

/**
 * usb_ss_gadget_ep_dequeue Remove request from transfer queue
 * @ep: endpoint object associated with request
 * @request: request object
 *
 * Returns 0 on success, error code elsewhere
 */
static int usb_ss_gadget_ep_dequeue(struct usb_ep *ep,
		struct usb_request *request)
{
	struct usb_ss_endpoint *usb_ss_ep =
		to_usb_ss_ep(ep);
	struct usb_ss_dev *usb_ss = usb_ss_ep->usb_ss;
	unsigned long flags;

	if (request == NULL || !request)
		return 0;

	spin_lock_irqsave(&usb_ss->lock, flags);
	/**
	 * shall not dequeue the request while the ep
	 * is disabling.
	 */
	if (!ep->enabled || usb_ss_ep->disabling) {
		spin_unlock_irqrestore(&usb_ss->lock, flags);
		return 0;
	}
	bmusb_dbg(usb_ss->dev, "DEQUEUE(%02X) %d\n",
		ep->address, request->length);
	if (request->dma) {
		usb_gadget_unmap_request(&usb_ss->gadget, request,
			ep->address & USB_DIR_IN);
		request->dma = 0;
	}
	request->status = -ECONNRESET;

	if (ep->address)
		list_del(&request->list);

	if (request->complete) {
		spin_unlock(&usb_ss->lock);
		request->complete(ep, request);
		spin_lock(&usb_ss->lock);
	}

	spin_unlock_irqrestore(&usb_ss->lock, flags);
	return 0;
}

/**
 * usb_ss_gadget_ep_set_halt Sets/clears stall on selected endpoint
 * @ep: endpoint object to set/clear stall on
 * @value: 1 for set stall, 0 for clear stall
 *
 * Returns 0 on success, error code elsewhere
 */
static int usb_ss_gadget_ep_set_halt(struct usb_ep *ep, int value)
{
	struct usb_ss_endpoint *usb_ss_ep =
		to_usb_ss_ep(ep);
	struct usb_ss_dev *usb_ss = usb_ss_ep->usb_ss;
	unsigned long flags;

	/* return error when endpoint disabled */
	if (!ep->enabled)
		return -EPERM;

	/* if actual transfer is pending defer setting stall on this endpoint */
	if (usb_ss_ep->hw_pending_flag && value) {
		usb_ss_ep->stalled_flag = 1;
		return 0;
	}

	bmusb_dbg(usb_ss->dev, "HALT(%02X) %d\n", ep->address, value);

	spin_lock_irqsave(&usb_ss->lock, flags);

	select_ep(usb_ss, ep->desc->bEndpointAddress);
	if (value) {
		bmusb_ep_stall_flush(usb_ss_ep);
	} else {
		/*
		 * TODO:
		 * epp->wedgeFlag = 0;
		 */
		usb_ss_ep->wedge_flag = 0;
		gadget_writel(usb_ss, &usb_ss->regs->ep_cmd,
		EP_CMD__CSTALL__MASK | EP_CMD__EPRST__MASK);
		/* wait for EPRST cleared */
		while (gadget_readl(usb_ss,
			&usb_ss->regs->ep_cmd) & EP_CMD__EPRST__MASK)
			;
		usb_ss_ep->stalled_flag = 0;
	}
	usb_ss_ep->hw_pending_flag = 0;

	spin_unlock_irqrestore(&usb_ss->lock, flags);

	return 0;
}

/**
 * usb_ss_gadget_ep_set_wedge Set wedge on selected endpoint
 * @ep: endpoint object
 *
 * Returns 0 on success, error code elsewhere
 */
static int usb_ss_gadget_ep_set_wedge(struct usb_ep *ep)
{
	struct usb_ss_endpoint *usb_ss_ep = to_usb_ss_ep(ep);
	struct usb_ss_dev *usb_ss = usb_ss_ep->usb_ss;

	bmusb_dbg(usb_ss->dev, "WEDGE(%02X)\n", ep->address);
	usb_ss_gadget_ep_set_halt(ep, 1);
	usb_ss_ep->wedge_flag = 1;
	return 0;
}

static const struct usb_ep_ops usb_ss_gadget_ep0_ops = {.enable =
		usb_ss_gadget_ep0_enable, .disable = usb_ss_gadget_ep0_disable,
		.alloc_request = usb_ss_gadget_ep_alloc_request, .free_request =
				usb_ss_gadget_ep_free_request, .queue =
				usb_ss_gadget_ep0_queue, .dequeue =
				usb_ss_gadget_ep_dequeue, .set_halt =
				usb_ss_gadget_ep0_set_halt, .set_wedge =
				usb_ss_gadget_ep_set_wedge, };

static const struct usb_ep_ops usb_ss_gadget_ep_ops = {.enable =
		usb_ss_gadget_ep_enable, .disable = usb_ss_gadget_ep_disable,
		.alloc_request = usb_ss_gadget_ep_alloc_request, .free_request =
				usb_ss_gadget_ep_free_request, .queue =
				usb_ss_gadget_ep_queue, .dequeue =
				usb_ss_gadget_ep_dequeue, .set_halt =
				usb_ss_gadget_ep_set_halt, .set_wedge =
				usb_ss_gadget_ep_set_wedge, };

/**
 * usb_ss_gadget_get_frame Returns number of actual ITP frame
 * @gadget: gadget object
 *
 * Returns number of actual ITP frame
 */
static int usb_ss_gadget_get_frame(struct usb_gadget *gadget)
{
	struct usb_ss_dev *usb_ss = gadget_to_usb_ss(gadget);

	bmusb_dbg(usb_ss->dev, "usb_ss_gadget_get_frame\n");
	return gadget_readl(usb_ss, &usb_ss->regs->usb_iptn);
}

static int usb_ss_gadget_wakeup(struct usb_gadget *gadget)
{
	struct usb_ss_dev *usb_ss = gadget_to_usb_ss(gadget);

	bmusb_dbg(usb_ss->dev, "usb_ss_gadget_wakeup\n");
	return 0;
}

static int usb_ss_gadget_set_selfpowered(struct usb_gadget *gadget,
		int is_selfpowered)
{
	struct usb_ss_dev *usb_ss = gadget_to_usb_ss(gadget);

	bmusb_dbg(usb_ss->dev, "usb_ss_gadget_set_selfpowered: %d\n",
		is_selfpowered);
	return 0;
}

static int usb_ss_gadget_pullup(struct usb_gadget *gadget, int is_on)
{
	struct usb_ss_dev *usb_ss = gadget_to_usb_ss(gadget);
	unsigned long flags = 0;

	spin_lock_irqsave(&usb_ss->lock, flags);
	bmusb_dbg(usb_ss->dev, "usb_ss_gadget_pullup: %d\n", is_on);
	if (is_on != 0) {
		gadget_writel(usb_ss, &usb_ss->regs->usb_conf,
			USB_CONF__DEVEN__MASK);
	} else {
		gadget_writel(usb_ss, &usb_ss->regs->usb_conf,
			USB_CONF__DEVDS__MASK);
	}
	spin_unlock_irqrestore(&usb_ss->lock, flags);
	return 0;
}

/**
 * usb_ss_gadget_udc_start Gadget start
 * @gadget: gadget object
 * @driver: driver which operates on this gadget
 *
 * Returns 0 on success, error code elsewhere
 */
static int usb_ss_gadget_udc_start(struct usb_gadget *gadget,
		struct usb_gadget_driver *driver)
{
	struct usb_ss_dev *usb_ss = gadget_to_usb_ss(gadget);
	struct platform_device *pdev = to_platform_device(usb_ss->dev);
	int irq = platform_get_irq(pdev, 0);
	int irq1 = platform_get_irq(pdev, 1);
	int ret;
	unsigned long flags;

#ifdef BM_THREADED_IRQ_HANDLING
	ret = devm_request_threaded_irq(usb_ss->dev, irq, bmusb_irq_handler,
		bmusb_irq_handler_thread, IRQF_SHARED, DRV_NAME, usb_ss);
#else

	ret = devm_request_irq(usb_ss->dev, irq, bmusb_irq_handler_thread,
		IRQF_SHARED, DRV_NAME, usb_ss);
#endif

	if (ret != 0) {
		bmusb_err(usb_ss->dev, "cannot request irq %d err %d\n", irq,
				ret);
		return -ENODEV;
	}
#ifdef BM_THREADED_IRQ_HANDLING
	ret = devm_request_threaded_irq(usb_ss->dev, irq1, bmusb_irq_handler,
		bmusb_irq_handler_thread, IRQF_SHARED, DRV_NAME, usb_ss);
#else

	ret = devm_request_irq(usb_ss->dev, irq1, bmusb_irq_handler_thread,
		IRQF_SHARED, DRV_NAME, usb_ss);
#endif

	if (ret != 0) {
		bmusb_err(usb_ss->dev, "cannot request irq1 %d err %d\n", irq1,
				ret);
		return -ENODEV;
	}

	bmusb_dbg(usb_ss->dev, "Interrupt request done\n");
	if (usb_ss->gadget_driver) {
		bmusb_err(usb_ss->dev, "%s is already bound to %s\n",
				usb_ss->gadget.name,
				usb_ss->gadget_driver->driver.name);
		return -EBUSY;
	}

	usb_ss->gadget_driver = driver;

	spin_lock_irqsave(&usb_ss->lock, flags);

	/* config new dma mode if support */
	if (usb_ss->new_dma_mode)
		gadget_writel(usb_ss, &usb_ss->regs->new_dma,
			      NEW_DMA_MASK);

	/* configure endpoint 0 hardware */
	bmusb_ep0_config(usb_ss);

	/* enable interrupts for endpoint 0 (in and out) */
	gadget_writel(usb_ss, &usb_ss->regs->ep_ien,
	EP_IEN__EOUTEN0__MASK | EP_IEN__EINEN0__MASK);

	/* enable interrupt for device */
	gadget_writel(usb_ss, &usb_ss->regs->usb_ien,
			USB_IEN__U2RESIEN__MASK | USB_ISTS__DIS2I__MASK
			| USB_IEN__CON2IEN__MASK
			| USB_IEN__UHRESIEN__MASK
			| USB_IEN__UWRESIEN__MASK
			| USB_IEN__DISIEN__MASK
			| USB_IEN__CONIEN__MASK
			| USB_IEN__U3EXTIEN__MASK
		    | USB_IEN__L2ENTIEN__MASK
			| USB_IEN__L2EXTIEN__MASK);
	/*disable USB3 function*/
#if 0
	gadget_writel(usb_ss, &usb_ss->regs->usb_conf,
			USB_CONF__CLK2OFFDS__MASK
			| USB_CONF__USB3DIS__MASK
			| USB_CONF__L1DS__MASK);
#endif

	gadget_writel(usb_ss, &usb_ss->regs->usb_conf,
			USB_CONF__U1DS__MASK
			| USB_CONF__U2DS__MASK
			/*
			 * TODO:
			 * | USB_CONF__L1EN__MASK
			 */
			);

	gadget_writel(usb_ss, &usb_ss->regs->dbg_link1,
		DBG_LINK1__LFPS_MIN_GEN_U1_EXIT_SET__MASK |
		DBG_LINK1__LFPS_MIN_GEN_U1_EXIT__WRITE(0x3C));
	spin_unlock_irqrestore(&usb_ss->lock, flags);

	return 0;
}

/**
 * usb_ss_gadget_udc_stop Stops gadget
 * @gadget: gadget object
 *
 * Returns 0
 */
static int usb_ss_gadget_udc_stop(struct usb_gadget *gadget)
{
	struct usb_ss_dev *usb_ss = gadget_to_usb_ss(gadget);
	struct platform_device *pdev = to_platform_device(usb_ss->dev);
	int irq = platform_get_irq(pdev, 0);
	int irq1 = platform_get_irq(pdev, 1);
	unsigned long flags;
	int i;

	/* free irqs */
	devm_free_irq(usb_ss->dev, irq, usb_ss);
	devm_free_irq(usb_ss->dev, irq1, usb_ss);

	for (i = 0; i < USB_SS_ENDPOINTS_MAX_COUNT; i++) {
		struct usb_ss_endpoint *ss_ep = usb_ss->eps[i];

		if (ss_ep && ss_ep->endpoint.enabled)
			usb_ss_gadget_ep_disable(&ss_ep->endpoint);
	}

	spin_lock_irqsave(&usb_ss->lock, flags);
	usb_ss->gadget_driver = NULL;
	spin_unlock_irqrestore(&usb_ss->lock, flags);

	return 0;
}

#if 0
static struct usb_ep *usb_ss_match_ep(struct usb_gadget *gadget,
		struct usb_endpoint_descriptor *desc,
		struct usb_ss_ep_comp_descriptor *ep_comp)
{
	//struct goku_udc	*dev = to_goku_udc(g);
	struct usb_ep *ep;

	switch (usb_endpoint_type(desc)) {
	case USB_ENDPOINT_XFER_ISOC:
	case USB_ENDPOINT_XFER_BULK:
		if (usb_endpoint_dir_in(desc))
			ep = gadget_find_ep_by_name(gadget, "ep1in");
		else
			ep = gadget_find_ep_by_name(gadget, "ep2out");
		break;
	case USB_ENDPOINT_XFER_INT:
		if (usb_endpoint_dir_in(desc))
			ep = gadget_find_ep_by_name(gadget, "ep3in");
		else
			ep = gadget_find_ep_by_name(gadget, "ep4out");
		break;
	default:
		ep = NULL;
		break;
	}

	if (ep && usb_gadget_ep_match_desc(gadget, ep, desc, ep_comp))
		return ep;

	return NULL;
}
#endif
static const struct usb_gadget_ops usb_ss_gadget_ops = {
	.get_frame = usb_ss_gadget_get_frame,
	.wakeup = usb_ss_gadget_wakeup,
	.set_selfpowered = usb_ss_gadget_set_selfpowered,
	.pullup = usb_ss_gadget_pullup,
	.udc_start = usb_ss_gadget_udc_start,
	.udc_stop = usb_ss_gadget_udc_stop,
	/* .match_ep	= usb_ss_match_ep, */
};

/**
 * usb_ss_init_ep Initializes software endpoints of gadget
 * @gadget: gadget object
 *
 * Returns 0 on success, error code elsewhere
 */
static int usb_ss_init_ep(struct usb_ss_dev *usb_ss)
{
	struct usb_ss_endpoint *usb_ss_ep;
	u32 ep_enabled_reg, iso_ep_reg, bulk_ep_reg;
	int i;
	int ep_reg_pos, ep_dir, ep_number;
	int found_endpoints = 0;

#if IS_ENABLED(CONFIG_USB_BM_DRD)
	ep_enabled_reg = usb_ss->ep_enabled_reg;
	iso_ep_reg = usb_ss->iso_ep_reg;
	bulk_ep_reg = usb_ss->bulk_ep_reg;
#else
	ep_enabled_reg = gadget_readl(usb_ss, &usb_ss->regs->usb_cap3);
	iso_ep_reg = gadget_readl(usb_ss, &usb_ss->regs->usb_cap4);
	bulk_ep_reg = gadget_readl(usb_ss, &usb_ss->regs->usb_cap5);
#endif

	bmusb_dbg(usb_ss->dev, "Initializing non-zero endpoints\n");

	for (i = 0; i < USB_SS_ENDPOINTS_MAX_COUNT; i++) {
		ep_number = (i / 2) + 1;
		ep_dir = i % 2;
		ep_reg_pos = (16 * ep_dir) + ep_number;

		if (!(ep_enabled_reg & (1uL << ep_reg_pos)))
			continue;

		/* create empty endpoint object */
		usb_ss_ep = devm_kzalloc(usb_ss->dev, sizeof(*usb_ss_ep),
		GFP_KERNEL);
		if (!usb_ss_ep)
			return -ENOMEM;

		/* set parent of endpoint object */
		usb_ss_ep->usb_ss = usb_ss;

		/* set index of endpoint in endpoints container */
		usb_ss->eps[found_endpoints++] = usb_ss_ep;

		/* set name of endpoint */
		snprintf(usb_ss_ep->name, sizeof(usb_ss_ep->name), "ep%d%s",
				ep_number, !!ep_dir ? "in" : "out");
		usb_ss_ep->endpoint.name = usb_ss_ep->name;
		bmusb_dbg(usb_ss->dev, "Initializing endpoint: %s\n",
				usb_ss_ep->name);

		usb_ep_set_maxpacket_limit(&usb_ss_ep->endpoint,
		ENDPOINT_MAX_PACKET_LIMIT);
		usb_ss_ep->endpoint.max_streams = ENDPOINT_MAX_STREAMS;
		usb_ss_ep->endpoint.ops = &usb_ss_gadget_ep_ops;
		if (ep_dir)
			usb_ss_ep->endpoint.caps.dir_in = 1;
		else
			usb_ss_ep->endpoint.caps.dir_out = 1;

		/* check endpoint type */
		#if 0
		if (iso_ep_reg & (1uL << ep_reg_pos)) {
			usb_ss_ep->endpoint.caps.type_iso = 1;
			usb_ss_ep->is_iso_flag = 1;
		}
		#endif

		usb_ss_ep->is_iso_flag = 0;

		if (bulk_ep_reg & (1uL << ep_reg_pos)) {
			usb_ss_ep->endpoint.caps.type_bulk = 1;
			usb_ss_ep->endpoint.caps.type_int = 1;
			usb_ss_ep->endpoint.maxburst = 15;
		}

		list_add_tail(&usb_ss_ep->endpoint.ep_list,
				&usb_ss->gadget.ep_list);
		INIT_LIST_HEAD(&usb_ss_ep->request_list);
	}
	usb_ss->ep_nums = found_endpoints;
	return 0;
}

/**
 * usb_ss_init_ep0 Initializes software endpoint 0 of gadget
 * @gadget: gadget object
 *
 * Returns 0 on success, error code elsewhere
 */
static int usb_ss_init_ep0(struct usb_ss_dev *usb_ss)
{
	struct usb_ss_endpoint *ep0;

	bmusb_dbg(usb_ss->dev, "Initializing EP0\n");
	ep0 = devm_kzalloc(usb_ss->dev, sizeof(struct usb_ss_endpoint),
		GFP_KERNEL);

	if (!ep0)
		return -ENOMEM;

	/* fill BM fields */
	ep0->usb_ss = usb_ss;
	sprintf(ep0->name, "ep0");

	/* fill linux fields */
	ep0->endpoint.ops = &usb_ss_gadget_ep0_ops;
	ep0->endpoint.maxburst = 1;
	usb_ep_set_maxpacket_limit(&ep0->endpoint, ENDPOINT0_MAX_PACKET_LIMIT);
	ep0->endpoint.address = 0;
	ep0->endpoint.maxpacket = 64;
	ep0->endpoint.enabled = 1;
	ep0->endpoint.caps.type_control = 1;
	ep0->endpoint.caps.dir_in = 1;
	ep0->endpoint.caps.dir_out = 1;
	ep0->endpoint.name = ep0->name;

	usb_ss->gadget.ep0 = &ep0->endpoint;

	return 0;
}

#if IS_ENABLED(CONFIG_USB_BM_MISC)
/**
 * usb_ss_turn_off_ref_clock
 * Low level function to enter into standby mode
 * @usb_ss: extended gadget object
 */
static void usb_ss_turn_off_ref_clock(struct usb_ss_dev *usb_ss)
{
	u32 usb_pwr_reg = gadget_readl(usb_ss, &usb_ss->regs->usb_pwr);

	usb_pwr_reg |= STB_CLK_SWITCH_EN_MASK;
	gadget_writel(usb_ss, &usb_ss->regs->usb_pwr, usb_pwr_reg);

	while (!(gadget_readl(usb_ss, &usb_ss->regs->usb_pwr)
		& STB_CLK_SWITCH_DONE_MASK))
		udelay(1000);

	usb_ss->in_standby_mode = 1;
}

/**
 * usb_ss_turn_on_ref_clock
 * Low level function to exit from standby mode
 * @usb_ss: extended gadget object
 */
static void usb_ss_turn_on_ref_clock(struct usb_ss_dev *usb_ss)
{
	u32 usb_pwr_reg = gadget_readl(usb_ss, &usb_ss->regs->usb_pwr);

	usb_pwr_reg &= ~STB_CLK_SWITCH_EN_MASK;
	gadget_writel(usb_ss, &usb_ss->regs->usb_pwr, usb_pwr_reg);

	while (!(gadget_readl(usb_ss, &usb_ss->regs->usb_pwr)
		& STB_CLK_SWITCH_DONE_MASK))
		udelay(1000);

	usb_ss->in_standby_mode = 0;
}

/**
 * usb_ss_ensure_clock_on
 * Ensures clock turned on while accessing following registers:
 * EP_CFG, EP_TR_ADDR, EP_CMD, EP_SEL, USB_CONF
 * @usb_ss: extended gadget object
 */
static void usb_ss_ensure_clock_on(struct usb_ss_dev *usb_ss)
{
	if (usb_ss->in_standby_mode)
		usb_ss_turn_on_ref_clock(usb_ss);
}

/**
 * gadget_is_stb_allowed
 * Checks if entering into standby mode is allowed,
 * called from application side
 * @usb_ss: extended gadget object
 *
 * Returns 1 if enter into standby mode is allowed or 0 if not
 */
int gadget_is_stb_allowed(struct usb_ss_dev *usb_ss)
{
	u32 ep_ists_reg = gadget_readl(usb_ss, &usb_ss->regs->ep_sts);
	u32 usb_sts_reg = gadget_readl(usb_ss, &usb_ss->regs->usb_sts);
	int speed;

	/*
	 * Allow to enter into standby mode if
	 * device is not connected and in the
	 * configured state
	 */
	if (!usb_ss->is_connected) {
		if (!(usb_sts_reg & USB_STS__CFGSTS__MASK))
			return 0;
	} else {
		/*
		 * In connected state, allow to enter
		 * into standby mode if DMA is in idle state
		 * and if device is in suspend mode
		 * (U3 in SS or L2 in FS/HS)
		 */
		if (ep_ists_reg & EP_STS__DBUSY__MASK)
			return 0;

		speed = USB_STS__USBSPEED__READ(usb_sts_reg);
		if (speed == USB_SPEED_SUPER) {
			if (USB_STS__LST__READ(usb_sts_reg) != SS_LINK_STATE_U3)
				return 0;
		} else if (speed == USB_SPEED_FULL || speed == USB_SPEED_HIGH) {
			if (USB_STS__LPMST__READ(usb_sts_reg)
				!= FSHS_LPM_STATE_L2)
				return 0;
		}
	}
	return 1;
}

/**
 * gadget_enter_stb_request
 * Attempts to enter to standby mode, called from application side
 * @usb_ss: extended gadget object
 *
 * Returns 0 on success, error code elsewhere
 */
int gadget_enter_stb_request(struct usb_ss_dev *usb_ss)
{
	if (usb_ss->in_standby_mode || !gadget_is_stb_allowed(usb_ss))
		return -EPERM;

	usb_ss_turn_off_ref_clock(usb_ss);
	return 0;
}

/**
 * gadget_exit_stb_request
 * Attempts to exit from standby mode, called from application side
 * @usb_ss: extended gadget object
 *
 * Returns 0 on success, error code elsewhere
 */
int gadget_exit_stb_request(struct usb_ss_dev *usb_ss)
{
	if (!usb_ss->in_standby_mode)
		return -EPERM;

	usb_ss_turn_on_ref_clock(usb_ss);
	return 0;
}

/**
 * gadget_update_stb_state
 * Updates state of standby mode flag
 * @usb_ss: extended gadget object
 */
static void gadget_update_stb_state(struct usb_ss_dev *usb_ss)
{
	u32 usb_pwr_reg;

	usb_pwr_reg = gadget_readl(usb_ss, &usb_ss->regs->usb_pwr);
	usb_ss->in_standby_mode =
		(usb_pwr_reg >> STB_CLK_SWITCH_EN_SHIFT) & 1uL;
}
#endif /* IS_ENABLED(CONFIG_USB_BM_MISC) */

int buffer_empty(struct usb_ep *ep)
{
	struct usb_ss_endpoint *usb_ss_ep =
		to_usb_ss_ep(ep);
	struct usb_ss_dev *usb_ss = usb_ss_ep->usb_ss;

	if (gadget_readl(usb_ss, &usb_ss->regs->ep_sts))
		return 1;
	else
		return 0;
}

static int gadget_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct usb_ss_dev *usb_ss;
	struct resource *res;
	int ret;
	const char *new_dma;

	/* allocate memory for extended gadget object */
	usb_ss = devm_kzalloc(dev, sizeof(*usb_ss), GFP_KERNEL);
	if (!usb_ss)
		return -ENOMEM;

	/* fill extended gadget's required fields */
	usb_ss->dev = dev;
	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	usb_ss->regs = devm_ioremap_resource(dev, res);

	if (IS_ERR(usb_ss->regs))
		return PTR_ERR(usb_ss->regs);

	/* Support new dma mode. */
	ret = device_property_read_string(dev, "dma_mode", &new_dma);
	if (ret < 0) {
		usb_ss->new_dma_mode = 0;
	} else {
		if (!strcmp(new_dma, "new"))
			usb_ss->new_dma_mode = 1;
		else
			usb_ss->new_dma_mode = 0;
	}

	/* fill gadget fields */
	usb_ss->gadget.ops = &usb_ss_gadget_ops;
	usb_ss->gadget.max_speed = USB_SPEED_SUPER;
	usb_ss->gadget.speed = USB_SPEED_UNKNOWN;
	usb_ss->gadget.name = "usb-ss-gadget";
	usb_ss->gadget.sg_supported = 1;
	usb_ss->is_connected = 0;

	/* set device DMA features */
	//ret = dma_set_mask_and_coherent(dev, DMA_BIT_MASK(32));
	//dev->parent->coherent_dma_mask = DMA_BIT_MASK(32);
	dma_set_coherent_mask(dev, DMA_BIT_MASK(64));
	dma_set_mask_and_coherent(dev, DMA_BIT_MASK(64));
	if (!dev->dma_mask) {
		dev->dma_parms = dev->parent->dma_parms;
		dev->dma_mask = dev->parent->dma_mask;
		dma_set_coherent_mask(dev, dev->parent->coherent_dma_mask);
	}

	usb_ss->in_standby_mode = 0;

#if IS_ENABLED(CONFIG_USB_BM_MISC)
	bmusb_dev_misc_register(usb_ss, res->start);
	gadget_update_stb_state(usb_ss);
#endif

#if IS_ENABLED(CONFIG_USB_BM_DRD)
	usb_ss->gadget.is_otg = 1;
	usb_ss->ep_enabled_reg = 0x001F001F;
	usb_ss->iso_ep_reg = 0x00080008;
	usb_ss->bulk_ep_reg = 0x001E001E;
#endif

	/* initialize endpoint container */
	INIT_LIST_HEAD(&usb_ss->gadget.ep_list);
	ret = usb_ss_init_ep0(usb_ss);
	if (ret)
		return -ENOMEM;

	ret = usb_ss_init_ep(usb_ss);
	if (ret)
		return -ENOMEM;

	/* allocate memory for default endpoint TRB */
	usb_ss->trb_ep0 = (u32 *)dma_alloc_coherent(usb_ss->dev, 20,
			&usb_ss->trb_ep0_dma, GFP_DMA);
	if (!usb_ss->trb_ep0)
		return -ENOMEM;

	/* allocate memory for setup packet buffer */
	usb_ss->setup = (u8 *)dma_alloc_coherent(usb_ss->dev, 8,
			&usb_ss->setup_dma,
			GFP_DMA);
	if (!usb_ss->setup)
		return -ENOMEM;

	/* set driver data */
	dev_set_drvdata(dev, usb_ss);

	/* add USB gadget device */
	ret = usb_add_gadget_udc(usb_ss->dev, &usb_ss->gadget);
	if (ret < 0)
		return ret;

	/* if otg_bypass support, override the id to device mode. */
	bmusb_drd_override_id(1);

	return 0;
}

static int gadget_remove(struct platform_device *pdev)
{
	struct usb_ss_dev *usb_ss = dev_get_drvdata(&pdev->dev);

	usb_del_gadget_udc(&usb_ss->gadget);
	return 0;
}

static const struct of_device_id of_gadget_match[] = {
	{.compatible = "bitmain,usb-dev"},
	{},
};
MODULE_DEVICE_TABLE(of, of_gadget_match);

static struct platform_driver bmusb_gadget_driver = {
	.probe = gadget_probe,
	.remove = gadget_remove,
	.driver = {
		.name = "usb-ss",
		.of_match_table = of_match_ptr(of_gadget_match),
	},
};

module_platform_driver(bmusb_gadget_driver);
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Bitmain USB Super Speed device driver");
