#include <linux/usb/gadget.h>
#include <linux/list.h>

#include "debug.h"
#include "io.h"
#include "g_regs_map.h"

void display_ep_desc(struct usb_ss_dev *usb_ss, struct usb_ep *usb_endpoint)
{
	cviusb_dbg(usb_ss->dev, "======== EP & DESC DUMP ======\n");

	cviusb_dbg(usb_ss->dev, "&usb_ep: %p\n", usb_endpoint);
	cviusb_dbg(usb_ss->dev, " usb_ep->address: 0x%02X\n",
			usb_endpoint->address);

	cviusb_dbg(usb_ss->dev, " usb_ep->desc: %p\n", usb_endpoint->desc);

	if (usb_endpoint->desc) {
		cviusb_dbg(usb_ss->dev, "  usb_ep->desc->bLength: %d\n",
				usb_endpoint->desc->bLength);
		cviusb_dbg(usb_ss->dev,
				"  usb_ep->desc->bDescriptorType: 0x%02X\n",
				usb_endpoint->desc->bDescriptorType);
		cviusb_dbg(usb_ss->dev,
				"  usb_ep->desc->bEndpointAddress: 0x%02X\n",
				usb_endpoint->desc->bEndpointAddress);
		cviusb_dbg(usb_ss->dev, "  usb_ep->desc->bmAttributes: 0x%02X\n",
				usb_endpoint->desc->bmAttributes);
		cviusb_dbg(usb_ss->dev, "  usb_ep->desc->wMaxPacketSize: %d\n",
				usb_endpoint->desc->wMaxPacketSize);
		cviusb_dbg(usb_ss->dev, "  usb_ep->desc->bInterval: %d\n",
				usb_endpoint->desc->bInterval);
		cviusb_dbg(usb_ss->dev, "  usb_ep->desc->bRefresh: %d\n",
				usb_endpoint->desc->bRefresh);
		cviusb_dbg(usb_ss->dev, "  usb_ep->desc->bSynchAddress: 0x%02X\n",
				usb_endpoint->desc->bSynchAddress);
	}

	cviusb_dbg(usb_ss->dev, "==============================\n");
}

void print_all_ep(struct usb_ss_dev *usb_ss)
{
	int i;

	for (i = 0; i < USB_SS_ENDPOINTS_MAX_COUNT; i++) {
		cviusb_dbg(usb_ss->dev, "/// Dumping %s\n",
				usb_ss->eps[i]->endpoint.name);
		display_ep_desc(usb_ss, &usb_ss->eps[i]->endpoint);
	}
}

void usb_ss_dump_regs(struct usb_ss_dev *usb_ss)
{
	cviusb_dbg(usb_ss->dev, ": =========== REGISTER DUMP ===========\n");

	cviusb_dbg(usb_ss->dev, ": Global configuration register (1):  0x%08X\n",
			cviusb_readl(&usb_ss->regs->usb_conf));
	cviusb_dbg(usb_ss->dev, ": Global status register:  0x%08X\n",
			cviusb_readl(&usb_ss->regs->usb_sts));
	cviusb_dbg(usb_ss->dev, ": Global command register (1):  0x%08X\n",
			cviusb_readl(&usb_ss->regs->usb_cmd));
	cviusb_dbg(usb_ss->dev, ": ITP/SOF number register:  0x%08X\n",
			cviusb_readl(&usb_ss->regs->usb_iptn));
	cviusb_dbg(usb_ss->dev, ": Global command register (2):  0x%08X\n",
			cviusb_readl(&usb_ss->regs->usb_lpm));
	cviusb_dbg(usb_ss->dev, ": Interrupt enable register:  0x%08X\n",
			cviusb_readl(&usb_ss->regs->usb_ien));
	cviusb_dbg(usb_ss->dev, ": Interrupt status register:  0x%08X\n",
			cviusb_readl(&usb_ss->regs->usb_ists));
	cviusb_dbg(usb_ss->dev, ": Endpoint select register:  0x%08X\n",
			cviusb_readl(&usb_ss->regs->ep_sel));
	cviusb_dbg(usb_ss->dev,
			": Endpoint transfer ring address register:  0x%08X\n",
			cviusb_readl(&usb_ss->regs->ep_traddr));
	cviusb_dbg(usb_ss->dev, ": Endpoint configuration register:  0x%08X\n",
			cviusb_readl(&usb_ss->regs->ep_cfg));
	cviusb_dbg(usb_ss->dev, ": Endpoint command register:  0x%08X\n",
			cviusb_readl(&usb_ss->regs->ep_cmd));
	cviusb_dbg(usb_ss->dev, ": Endpoint status register (1):  0x%08X\n",
			cviusb_readl(&usb_ss->regs->ep_sts));
	cviusb_dbg(usb_ss->dev, ": Endpoint status register (2):  0x%08X\n",
			cviusb_readl(&usb_ss->regs->ep_sts_sid));
	cviusb_dbg(usb_ss->dev, ": Endpoint status enable register:  0x%08X\n",
			cviusb_readl(&usb_ss->regs->ep_sts_en));
	cviusb_dbg(usb_ss->dev, ": Doorbell register:  0x%08X\n",
			cviusb_readl(&usb_ss->regs->drbl));
	cviusb_dbg(usb_ss->dev,
			": Endpoint interrupt enable register:  0x%08X\n",
			cviusb_readl(&usb_ss->regs->ep_ien));
	cviusb_dbg(usb_ss->dev,
			": Endpoint interrupt status register:  0x%08X\n",
			cviusb_readl(&usb_ss->regs->ep_ists));
	cviusb_dbg(usb_ss->dev,
			": Global power configuration register:  0x%08X\n",
			cviusb_readl(&usb_ss->regs->usb_pwr));
	cviusb_dbg(usb_ss->dev, ": Global configuration register (2):  0x%08X\n",
			cviusb_readl(&usb_ss->regs->usb_conf2));
	cviusb_dbg(usb_ss->dev, ": Capability register (1):  0x%08X\n",
			cviusb_readl(&usb_ss->regs->usb_cap1));
	cviusb_dbg(usb_ss->dev, ": Capability register (2):  0x%08X\n",
			cviusb_readl(&usb_ss->regs->usb_cap2));
	cviusb_dbg(usb_ss->dev, ": Capability register (3):  0x%08X\n",
			cviusb_readl(&usb_ss->regs->usb_cap3));
	cviusb_dbg(usb_ss->dev, ": Capability register (4):  0x%08X\n",
			cviusb_readl(&usb_ss->regs->usb_cap4));
	cviusb_dbg(usb_ss->dev, ": Capability register (5):  0x%08X\n",
			cviusb_readl(&usb_ss->regs->usb_cap5));
	cviusb_dbg(usb_ss->dev, ": Custom packet register (1):  0x%08X\n",
			cviusb_readl(&usb_ss->regs->usb_cpkt1));
	cviusb_dbg(usb_ss->dev, ": Custom packet register (2):  0x%08X\n",
			cviusb_readl(&usb_ss->regs->usb_cpkt2));
	cviusb_dbg(usb_ss->dev, ": Custom packet register (3):  0x%08X\n",
			cviusb_readl(&usb_ss->regs->usb_cpkt3));

	cviusb_dbg(usb_ss->dev,
			": ===========================================\n");
}

void usb_ss_dump_reg(struct usb_ss_dev *usb_ss, uint32_t __iomem *reg)
{
	cviusb_dbg(usb_ss->dev, "Address: %p, value: 0x%08X\n", reg,
			cviusb_readl(reg));
}

void usb_ss_dbg_dump_lnx_usb_ep(struct device *dev, struct usb_ep *ep, int tab)
{
	if (!!tab) {
		cviusb_dbg(dev, "\tAddress: %p\n", ep);
		cviusb_dbg(dev, "\n");
		cviusb_dbg(dev, "\t ep->driver_data: 0x%08lX (void*)\n",
				(uintptr_t)ep->driver_data);
		cviusb_dbg(dev, "\t ep->name: %s (const char*)\n", ep->name);
		cviusb_dbg(dev, "\t ep->ops: %p (const struct usb_ep_ops*)\n",
				ep->ops);
		cviusb_dbg(dev, "\t&ep->ep_list: %p (struct list_head)\n",
				&ep->ep_list);
		cviusb_dbg(dev, "\t ep->maxpacket: %d (unsigned:16)\n",
				ep->maxpacket);
		cviusb_dbg(dev, "\t ep->maxpacket_limit: %d (unsigned:16)\n",
				ep->maxpacket_limit);
		cviusb_dbg(dev, "\t ep->max_streams: %d (unsigned:16)\n",
				ep->max_streams);
		cviusb_dbg(dev, "\t ep->mult: %d (unsigned:2)\n", ep->mult);
		cviusb_dbg(dev, "\t ep->maxburst: %d (unsigned:5)\n",
				ep->maxburst);
		cviusb_dbg(dev, "\t ep->address: 0x%02X (u8)\n", ep->address);
		cviusb_dbg(dev,
				"\t ep->desc: %p (const struct usb_endpoint_descriptor*)\n",
				ep->desc);
		cviusb_dbg(dev,
				"\t ep->comp_desc: %p (const struct usb_ss_ep_comp_descriptor*)\n",
				ep->comp_desc);
		cviusb_dbg(dev, "\t Caps:\n");
		cviusb_dbg(dev, "\t\t ep->caps.type_control: %d\n",
				ep->caps.type_control);
		cviusb_dbg(dev, "\t\t ep->caps.type_iso: %d\n",
				ep->caps.type_iso);
		cviusb_dbg(dev, "\t\t ep->caps.type_bulk: %d\n",
				ep->caps.type_bulk);
		cviusb_dbg(dev, "\t\t ep->caps.type_int: %d\n",
				ep->caps.type_int);
		cviusb_dbg(dev, "\t\t ep->caps.dir_in: %d\n", ep->caps.dir_in);
		cviusb_dbg(dev, "\t\t ep->caps.dir_out: %d\n", ep->caps.dir_out);
		cviusb_dbg(dev, "\n");
		cviusb_dbg(dev,
				"// ============================================== //\n");
	} else {
		cviusb_dbg(dev,
				"// =============== DUMP: struct usb_ep ========== //\n");
		cviusb_dbg(dev, "Address: %p\n", ep);
		cviusb_dbg(dev, "\n");
		cviusb_dbg(dev, " ep->driver_data: 0x%08lX (void*)\n",
				(uintptr_t)ep->driver_data);
		cviusb_dbg(dev, " ep->name: %s (const char*)\n", ep->name);
		cviusb_dbg(dev, " ep->ops: %p (const struct usb_ep_ops*)\n",
				ep->ops);
		cviusb_dbg(dev, "&ep->ep_list: %p (struct list_head)\n",
				&ep->ep_list);
		cviusb_dbg(dev, " ep->maxpacket: %d (unsigned:16)\n",
				ep->maxpacket);
		cviusb_dbg(dev, " ep->maxpacket_limit: %d (unsigned:16)\n",
				ep->maxpacket_limit);
		cviusb_dbg(dev, " ep->max_streams: %d (unsigned:16)\n",
				ep->max_streams);
		cviusb_dbg(dev, " ep->mult: %d (unsigned:2)\n", ep->mult);
		cviusb_dbg(dev, " ep->maxburst: %d (unsigned:5)\n",
				ep->maxburst);
		cviusb_dbg(dev, " ep->address: 0x%02X (u8)\n", ep->address);
		cviusb_dbg(dev,
				" ep->desc: %p (const struct usb_endpoint_descriptor*)\n",
				ep->desc);
		cviusb_dbg(dev,
				" ep->comp_desc: %p (const struct usb_ss_ep_comp_descriptor*)\n",
				ep->comp_desc);
		cviusb_dbg(dev, " Caps:\n");
		cviusb_dbg(dev, "\t ep->caps.type_control: %d\n",
				ep->caps.type_control);
		cviusb_dbg(dev, "\t ep->caps.type_iso: %d\n", ep->caps.type_iso);
		cviusb_dbg(dev, "\t ep->caps.type_bulk: %d\n",
				ep->caps.type_bulk);
		cviusb_dbg(dev, "\t ep->caps.type_int: %d\n", ep->caps.type_int);
		cviusb_dbg(dev, "\t ep->caps.dir_in: %d\n", ep->caps.dir_in);
		cviusb_dbg(dev, "\t ep->caps.dir_out: %d\n", ep->caps.dir_out);
		cviusb_dbg(dev, "\n");
		cviusb_dbg(dev,
				"// ============================================== //\n");
	}
}

void usb_ss_dbg_dump_lnx_usb_gadget(struct usb_gadget *gadget)
{
	struct usb_ep *end_point;
	struct list_head *element;

	cviusb_dbg(&gadget->dev,
			"// ============ DUMP: struct usb_gadget ========= //\n");
	cviusb_dbg(&gadget->dev, "Address: %p\n", gadget);
	cviusb_dbg(&gadget->dev, "\n");
	cviusb_dbg(&gadget->dev, "&gadget->work: %p (struct work_struct)\n",
			&gadget->work);
	cviusb_dbg(&gadget->dev, " gadget->udc: %p (struct usb_udc*)\n",
			gadget->udc);
	cviusb_dbg(&gadget->dev,
			" gadget->ops: %p (const struct usb_gadget_ops*)\n",
			gadget->ops);
	cviusb_dbg(&gadget->dev, " gadget->ep0: %p (struct usb_ep*)\n",
			gadget->ep0);
	cviusb_dbg(&gadget->dev, "&gadget->ep_list: %p (struct list_head)\n",
			&gadget->ep_list);

	list_for_each(element, &gadget->ep_list)
	{
		end_point = list_entry(element, struct usb_ep, ep_list);
		if (end_point)
			usb_ss_dbg_dump_lnx_usb_ep(&gadget->dev, end_point, 1);
	}

	cviusb_dbg(&gadget->dev, " gadget->speed: %d (enum usb_device_speed)\n",
			gadget->speed);
	cviusb_dbg(&gadget->dev,
			" gadget->max_speed: %d (enum usb_device_speed)\n",
			gadget->max_speed);
	cviusb_dbg(&gadget->dev, " gadget->state: %d (enum usb_device_state)\n",
			gadget->state);
	cviusb_dbg(&gadget->dev, " gadget->name: %s (const char*)\n",
			gadget->name);
	cviusb_dbg(&gadget->dev, "gadget->dev: %p (struct device)\n",
			&gadget->dev);
	cviusb_dbg(&gadget->dev, " gadget->out_epnum: %d (unsigned)\n",
			gadget->out_epnum);
	cviusb_dbg(&gadget->dev, " gadget->in_epnum: %d (unsigned)\n",
			gadget->in_epnum);
	cviusb_dbg(&gadget->dev, " gadget->sg_supported: %d (unsigned:1)\n",
			gadget->sg_supported);
	cviusb_dbg(&gadget->dev, " gadget->is_otg: %d (unsigned:1)\n",
			gadget->is_otg);
	cviusb_dbg(&gadget->dev, " gadget->is_a_peripheral: %d (unsigned:1)\n",
			gadget->is_a_peripheral);
	cviusb_dbg(&gadget->dev, " gadget->b_hnp_enable: %d (unsigned:1)\n",
			gadget->b_hnp_enable);
	cviusb_dbg(&gadget->dev, " gadget->a_hnp_support: %d (unsigned:1)\n",
			gadget->a_hnp_support);
	cviusb_dbg(&gadget->dev, " gadget->a_alt_hnp_support: %d (unsigned:1)\n",
			gadget->a_alt_hnp_support);
	cviusb_dbg(&gadget->dev,
			" gadget->quirk_ep_out_aligned_size: %d (unsigned:1)\n",
			gadget->quirk_ep_out_aligned_size);
	cviusb_dbg(&gadget->dev, " gadget->is_selfpowered: %d (unsigned:1)\n",
			gadget->is_selfpowered);
	cviusb_dbg(&gadget->dev, "\n");
	cviusb_dbg(&gadget->dev,
			"// ============================================== //\n");
}

void usb_ss_dbg_dump_lnx_usb_request(struct usb_ss_dev *usb_ss,
		struct usb_request *usb_req)
{
	cviusb_dbg(usb_ss->dev,
			"// ============ DUMP: struct usb_request ======== //\n");
	cviusb_dbg(usb_ss->dev, "Address: %p\n", usb_req);
	cviusb_dbg(usb_ss->dev, "\n");
	cviusb_dbg(usb_ss->dev, " usb_req->buf: 0x%08lX (void*)\n",
			(uintptr_t)usb_req->buf);
	cviusb_dbg(usb_ss->dev, " usb_req->length: %d (unsigned)\n",
			usb_req->length);
	cviusb_dbg(usb_ss->dev, " usb_req->dma: 0x%08llX (dma_addr_t)\n",
			(unsigned long long int)usb_req->dma);
	cviusb_dbg(usb_ss->dev, " usb_req->sg: %p (struct scatterlist*)\n",
			usb_req->sg);
	cviusb_dbg(usb_ss->dev, " usb_req->num_sgs: %d (unsigned)\n",
			usb_req->num_sgs);
	cviusb_dbg(usb_ss->dev, " usb_req->num_mapped_sgs: %d (unsigned)\n",
			usb_req->num_mapped_sgs);
	cviusb_dbg(usb_ss->dev, " usb_req->stream_id: %d (unsigned:16)\n",
			usb_req->stream_id);
	cviusb_dbg(usb_ss->dev, " usb_req->no_interrupt: %d (unsigned:1)\n",
			usb_req->no_interrupt);
	cviusb_dbg(usb_ss->dev, " usb_req->zero: %d (unsigned:1)\n",
			usb_req->zero);
	cviusb_dbg(usb_ss->dev, " usb_req->short_not_ok: %d (unsigned:1)\n",
			usb_req->short_not_ok);
	cviusb_dbg(usb_ss->dev,
			" usb_req->complete: %p ((void)(f_ptr*)(struct usb_ep *ep, struct usb_request *req))\n",
			usb_req->complete);
	cviusb_dbg(usb_ss->dev, " usb_req->context: 0x%08lX (void*)\n",
			(uintptr_t)usb_req->context);
	cviusb_dbg(usb_ss->dev, "&usb_req->list: %p (struct list_head)\n",
			&usb_req->list);
	cviusb_dbg(usb_ss->dev, " usb_req->status: %d (int)\n", usb_req->status);
	cviusb_dbg(usb_ss->dev, " usb_req->actual: %d (unsigned)\n",
			usb_req->actual);
	cviusb_dbg(usb_ss->dev, "\n");
	cviusb_dbg(usb_ss->dev,
			"// ============================================== //\n");
}

void usb_ss_dbg_dump_cviusb_usb_ss_dev(struct usb_ss_dev *usb_ss)
{
	cviusb_dbg(usb_ss->dev,
			"// ============ DUMP: struct usb_ss_dev ========= //\n");
	cviusb_dbg(usb_ss->dev, "Address: %p\n", usb_ss);
	cviusb_dbg(usb_ss->dev, "\n");
	cviusb_dbg(usb_ss->dev, " usb_ss->dev: %p (struct device*)\n",
			usb_ss->dev);
	cviusb_dbg(usb_ss->dev, " usb_ss->regs: 0x%08lX (void*)\n",
			(uintptr_t)usb_ss->regs);
	cviusb_dbg(usb_ss->dev, "&usb_ss->gadget: %p (struct usb_gadget)\n",
			&usb_ss->gadget);
	cviusb_dbg(usb_ss->dev,
			" usb_ss->gadget_driver: %p (struct usb_gadget_driver*)\n",
			usb_ss->gadget_driver);
	cviusb_dbg(usb_ss->dev, " usb_ss->eps: %p (struct usb_ss_ep* a[32])\n",
			usb_ss->eps);
	cviusb_dbg(usb_ss->dev, "\n");
	cviusb_dbg(usb_ss->dev,
			"// ============================================== //\n");
}

void usb_ss_dbg_dump_cviusb_usb_ss_ep(struct usb_ss_dev *usb_ss,
		struct usb_ss_endpoint *usb_ss_ep)
{
	cviusb_dbg(usb_ss->dev,
			"// ============ DUMP: struct usb_ss_ep ========== //\n");
	cviusb_dbg(usb_ss->dev, "Address: %p\n", usb_ss_ep);
	cviusb_dbg(usb_ss->dev, "\n");
	cviusb_dbg(usb_ss->dev, " usb_ss_ep->usb_ss: %p (struct usb_ss_dev*)\n",
			usb_ss_ep->usb_ss);
	cviusb_dbg(usb_ss->dev, " usb_ss_ep->name: %s (char*)\n",
			usb_ss_ep->name);
	cviusb_dbg(usb_ss->dev, "&usb_ss_ep->endpoint: %p (struct usb_ep)\n",
			&usb_ss_ep->endpoint);
	cviusb_dbg(usb_ss->dev,
			"&usb_ss_ep->request_list: %p (struct list_head)\n",
			&usb_ss_ep->request_list);
	cviusb_dbg(usb_ss->dev, "\n");
	cviusb_dbg(usb_ss->dev,
			"// ============================================== //\n");
}

void usb_ss_dbg_dump_cviusb_usb_ss_trb(struct usb_ss_dev *usb_ss,
		struct usb_ss_trb *usb_trb)
{
	cviusb_dbg(usb_ss->dev,
			"// ============ DUMP: struct usb_ss_ep ========== //\n");
	cviusb_dbg(usb_ss->dev, "Address: %p\n", usb_trb);
	cviusb_dbg(usb_ss->dev, "\n");
	cviusb_dbg(usb_ss->dev, " usb_trb->offset8: 0x%08X (u32)\n",
			usb_trb->offset8);
	cviusb_dbg(usb_ss->dev, " usb_trb->offset4: 0x%08X (u32)\n",
			usb_trb->offset4);
	cviusb_dbg(usb_ss->dev, " usb_trb->offset0: 0x%08X (u32)\n",
			usb_trb->offset0);
	cviusb_dbg(usb_ss->dev, "\n");
	cviusb_dbg(usb_ss->dev,
			"// ============================================== //\n");
}
