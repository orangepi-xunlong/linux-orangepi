/*
 * drivers/usb/sunxi_usb/hcd/core/sunxi_hcd_core.c
 * (C) Copyright 2010-2015
 * Allwinner Technology Co., Ltd. <www.allwinnertech.com>
 * javen, 2010-12-20, create this file
 *
 * usb host controller driver
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/init.h>
#include <linux/list.h>
#include <linux/kobject.h>
#include <linux/platform_device.h>
#include <linux/io.h>

#include  "../include/sunxi_hcd_config.h"
#include  "../include/sunxi_hcd_core.h"
#include  "../include/sunxi_hcd_dma.h"

/* for high speed test mode; see USB 2.0 spec 7.1.20 */
static const u8 sunxi_hcd_test_packet[53] = {
	/* implicit SYNC then DATA0 to start */

	/* JKJKJKJK x9 */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	/* JJKKJJKK x8 */
	0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa,
	/* JJJJKKKK x8 */
	0xee, 0xee, 0xee, 0xee, 0xee, 0xee, 0xee, 0xee,
	/* JJJJJJJKKKKKKK x8 */
	0xfe, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	/* JJJJJJJK x8 */
	0x7f, 0xbf, 0xdf, 0xef, 0xf7, 0xfb, 0xfd,
	/* JKKKKKKK x10, JK */
	0xfc, 0x7e, 0xbf, 0xdf, 0xef, 0xf7, 0xfb, 0xfd, 0x7e

	/* implicit CRC16 then EOP to end */
};

/*
* Interrupt Service Routine to record USB "global" interrupts.
* Since these do not happen often and signify things of
* paramount importance, it seems OK to check them individually;
* the order of the tests is specified in the manual
*
* @param sunxi_hcd instance pointer
* @param int_usb register contents
* @param devctl
* @param power
*/
#define STAGE0_MASK ((1 << USBC_BP_INTUSB_RESUME) \
	| (1 << USBC_BP_INTUSB_SESSION_REQ) \
	| (1 << USBC_BP_INTUSB_VBUS_ERROR) \
	| (1 << USBC_BP_INTUSB_CONNECT) \
	| (1 << USBC_BP_INTUSB_RESET) \
	| (1 << USBC_BP_INTUSB_SOF))

void sunxi_hcd_write_fifo(struct sunxi_hcd_hw_ep *hw_ep, u16 len, const u8 *src)
{
	void __iomem *fifo = hw_ep->fifo;
	__u32 old_ep_index = 0;

	prefetch((u8 *)src);

	DMSG_DBG_HCD("sunxi_hcd_write_fifo: %cX ep%d fifo %p count %d buf %p\n",
			'T', hw_ep->epnum, fifo, len, src);

	old_ep_index = USBC_GetActiveEp(hw_ep->sunxi_hcd->sunxi_hcd_io->usb_bsp_hdle);
	USBC_SelectActiveEp(hw_ep->sunxi_hcd->sunxi_hcd_io->usb_bsp_hdle, hw_ep->epnum);

	/* we can't assume unaligned reads work */
	if (likely((0x01 & (unsigned long) src) == 0)) {
		u16	index = 0;

		/* best case is 32bit-aligned source address */
		if ((0x02 & (unsigned long) src) == 0) {
			if (len >= 4) {
				sunxi_hcd_writesl(fifo, src + index, len >> 2);
				index += len & ~0x03;
			}

			if (len & 0x02) {
				USBC_Writew(*(u16 *)&src[index], fifo);
				index += 2;
			}
		} else {
			if (len >= 2) {
				sunxi_hcd_writesw(fifo, src + index, len >> 1);
				index += len & ~0x01;
			}
		}

		if (len & 0x01) {
			USBC_Writeb(src[index], fifo);
		}
	} else  {
		/* byte aligned */
		sunxi_hcd_writesb(fifo, src, len);
	}

	USBC_SelectActiveEp(hw_ep->sunxi_hcd->sunxi_hcd_io->usb_bsp_hdle, old_ep_index);
}
EXPORT_SYMBOL(sunxi_hcd_write_fifo);

/* Unload an endpoint's FIFO */
void sunxi_hcd_read_fifo(struct sunxi_hcd_hw_ep *hw_ep, u16 len, u8 *dst)
{
	void __iomem *fifo = hw_ep->fifo;
	__u32 old_ep_index = 0;

	DMSG_DBG_HCD("sunxi_hcd_read_fifo: %cX ep%d fifo %p count %d buf %p\n",
		'R', hw_ep->epnum, fifo, len, dst);

	old_ep_index = USBC_GetActiveEp(hw_ep->sunxi_hcd->sunxi_hcd_io->usb_bsp_hdle);
	USBC_SelectActiveEp(hw_ep->sunxi_hcd->sunxi_hcd_io->usb_bsp_hdle, hw_ep->epnum);

	/* we can't assume unaligned writes work */
	if (likely((0x01 & (unsigned long) dst) == 0)) {
		u16	index = 0;

		/* best case is 32bit-aligned destination address */
		if ((0x02 & (unsigned long) dst) == 0) {
			if (len >= 4) {
				sunxi_hcd_readsl(fifo, dst, len >> 2);
				index = len & ~0x03;
			}
			if (len & 0x02) {
				*(u16 *)&dst[index] = USBC_Readw(fifo);
				index += 2;
			}
		} else {
			if (len >= 2) {
				sunxi_hcd_readsw(fifo, dst, len >> 1);
				index = len & ~0x01;
			}
		}

		if (len & 0x01) {
			dst[index] = USBC_Readb(fifo);
		}
	} else  {
		/* byte aligned */
		sunxi_hcd_readsb(fifo, dst, len);
	}

	USBC_SelectActiveEp(hw_ep->sunxi_hcd->sunxi_hcd_io->usb_bsp_hdle, old_ep_index);
}
EXPORT_SYMBOL(sunxi_hcd_read_fifo);

void sunxi_hcd_load_testpacket(struct sunxi_hcd *sunxi_hcd)
{
	void __iomem *usbc_base = sunxi_hcd->endpoints[0].regs;

	sunxi_hcd_ep_select(sunxi_hcd->mregs, 0);

	sunxi_hcd_write_fifo(sunxi_hcd->control_ep, sizeof(sunxi_hcd_test_packet), sunxi_hcd_test_packet);

	USBC_Writew(USBC_BP_CSR0_H_TxPkRdy, USBC_REG_CSR0(usbc_base));

	USBC_Host_DisablePing(sunxi_hcd->sunxi_hcd_io->usb_bsp_hdle);
}
EXPORT_SYMBOL(sunxi_hcd_load_testpacket);

/* Program the to start (enable interrupts, dma, etc.). */
void sunxi_hcd_start(struct sunxi_hcd *sunxi_hcd)
{
	void __iomem    *usbc_base  = NULL;
	u8              devctl      = 0;

	/* check argment */
	if (sunxi_hcd == NULL) {
		DMSG_PANIC("ERR: invalid argment\n");
		return ;
	}

	sunxi_hcd->is_active = 0;

	if (!sunxi_hcd->enable) {
		DMSG_INFO("wrn: hcd is not enable, need not start hcd\n");
		return;
	}

	/* initialize parameter */
	usbc_base = sunxi_hcd->mregs;

	DMSG_DBG_HCD("sunxi_hcd_start: devctl = 0x%x, epmask = 0x%x, 0x%x\n",
		USBC_Readb(USBC_REG_DEVCTL(usbc_base)), sunxi_hcd->epmask, usbc_base);

#if defined (CONFIG_ARCH_SUN8IW5) || defined (CONFIG_ARCH_SUN8IW9)
{
	__u32 reg_val = 0;
	reg_val = USBC_Readl(usbc_base + USBPHYC_REG_o_PHYCTL);
	reg_val &= ~(0x01 << 1);
	USBC_Writel(reg_val, (usbc_base + USBPHYC_REG_o_PHYCTL));
}
#endif

	/*  Set INT enable registers, enable interrupts */
	USBC_Writew(sunxi_hcd->epmask, USBC_REG_INTTxE(usbc_base));
	USBC_Writew((sunxi_hcd->epmask & 0xfe), USBC_REG_INTRxE(usbc_base));
	USBC_Writeb(0xff, USBC_REG_INTUSBE(usbc_base));

	USBC_Writeb(0x00, USBC_REG_TMCTL(usbc_base));

	/* put into basic highspeed mode and start session */
	USBC_Writeb((1 << USBC_BP_POWER_H_HIGH_SPEED_EN), USBC_REG_PCTL(usbc_base));

	devctl = USBC_Readb(USBC_REG_DEVCTL(usbc_base));
	devctl &= ~(1 << USBC_BP_DEVCTL_SESSION);
	USBC_Writeb(devctl, USBC_REG_DEVCTL(usbc_base));

	USBC_SelectBus(sunxi_hcd->sunxi_hcd_io->usb_bsp_hdle, USBC_IO_TYPE_PIO, 0, 0);

	/* assume ID pin is hard-wired to ground */
	sunxi_hcd_platform_enable(sunxi_hcd);
}
EXPORT_SYMBOL(sunxi_hcd_start);

void sunxi_hcd_generic_disable(struct sunxi_hcd *sunxi_hcd)
{
	void __iomem    *usbc_base  = sunxi_hcd->mregs;

	/* disable interrupts */
	USBC_Writeb(0x00, USBC_REG_INTUSBE(usbc_base));
	USBC_Writew(0x00, USBC_REG_INTTxE(usbc_base));
	USBC_Writew(0x00, USBC_REG_INTRxE(usbc_base));

	/* off */
	USBC_Writew(0x00, USBC_REG_DEVCTL(usbc_base));

	/*  flush pending interrupts */
	USBC_Writeb(0xff, USBC_REG_INTUSB(usbc_base));
	USBC_Writew(0x3f, USBC_REG_INTTx(usbc_base));
	USBC_Writew(0x3f, USBC_REG_INTRx(usbc_base));
}
EXPORT_SYMBOL(sunxi_hcd_generic_disable);

/*
 *    Make the stop (disable interrupts, etc.);
 * reversible by sunxi_hcd_start called on gadget driver unregister
 * with controller locked, irqs blocked
 * acts as a NOP unless some role activated the hardware
 *
 */
void sunxi_hcd_stop(struct sunxi_hcd *sunxi_hcd)
{
	if (!sunxi_hcd->enable) {
		DMSG_INFO("wrn: hcd is not enable, need not stop hcd\n");
		return;
	}

	/* stop IRQs, timers, ... */
	sunxi_hcd_platform_disable(sunxi_hcd);
	sunxi_hcd_generic_disable(sunxi_hcd);

	DMSG_INFO("sunxi_hcd_stop: sunxi_hcd disabled\n");

	/* FIXME
	 *  - mark host and/or peripheral drivers unusable/inactive
	 *  - disable DMA (and enable it in sunxi_hcd Start)
	 *  - make sure we can sunxi_hcd_start() after sunxi_hcd_stop(); with
	 *    OTG mode, gadget driver module rmmod/modprobe cycles that
	 *  - ...
	 */
	sunxi_hcd_platform_try_idle(sunxi_hcd, 0);
}
EXPORT_SYMBOL(sunxi_hcd_stop);

void sunxi_hcd_platform_try_idle(struct sunxi_hcd *sunxi_hcd, unsigned long timeout)
{
}
EXPORT_SYMBOL(sunxi_hcd_platform_try_idle);

void sunxi_hcd_platform_enable(struct sunxi_hcd *sunxi_hcd)
{
}
EXPORT_SYMBOL(sunxi_hcd_platform_enable);

void sunxi_hcd_platform_disable(struct sunxi_hcd *sunxi_hcd)
{
}
EXPORT_SYMBOL(sunxi_hcd_platform_disable);

int sunxi_hcd_platform_set_mode(struct sunxi_hcd *sunxi_hcd, u8 sunxi_hcd_mode)
{
	DMSG_PANIC("ERR: sunxi_hcd_platform_set_mode not support\n");
	return 0;
}
EXPORT_SYMBOL(sunxi_hcd_platform_set_mode);

int sunxi_hcd_platform_init(struct sunxi_hcd *sunxi_hcd)
{
	USBC_EnhanceSignal(sunxi_hcd->sunxi_hcd_io->usb_bsp_hdle);
	USBC_EnableDpDmPullUp(sunxi_hcd->sunxi_hcd_io->usb_bsp_hdle);
	USBC_EnableIdPullUp(sunxi_hcd->sunxi_hcd_io->usb_bsp_hdle);
	USBC_ForceId(sunxi_hcd->sunxi_hcd_io->usb_bsp_hdle, USBC_ID_TYPE_HOST);

	return 0;
}
EXPORT_SYMBOL(sunxi_hcd_platform_init);

int sunxi_hcd_platform_suspend(struct sunxi_hcd *sunxi_hcd)
{
	DMSG_PANIC("ERR: sunxi_hcd_platform_suspend not support\n");
	return 0;
}
EXPORT_SYMBOL(sunxi_hcd_platform_suspend);

int sunxi_hcd_platform_resume(struct sunxi_hcd *sunxi_hcd)
{
	DMSG_PANIC("ERR: sunxi_hcd_platform_resume not support\n");

	return 0;
}
EXPORT_SYMBOL(sunxi_hcd_platform_resume);

int sunxi_hcd_platform_exit(struct sunxi_hcd *sunxi_hcd)
{
	USBC_DisableDpDmPullUp(sunxi_hcd->sunxi_hcd_io->usb_bsp_hdle);
	USBC_DisableIdPullUp(sunxi_hcd->sunxi_hcd_io->usb_bsp_hdle);
	USBC_ForceId(sunxi_hcd->sunxi_hcd_io->usb_bsp_hdle, USBC_ID_TYPE_DISABLE);

	return 0;
}
EXPORT_SYMBOL(sunxi_hcd_platform_exit);

/* "modprobe ... use_dma=0" etc */
#if 0
void sunxi_hcd_dma_completion(struct sunxi_hcd *sunxi_hcd, u8 epnum, u8 transmit)
{
	u8	devctl = USBC_Readb(USBC_REG_DEVCTL(sunxi_hcd->mregs));
	unsigned long   flags       = 0;

	spin_lock_irqsave(&sunxi_hcd->lock, flags);

	/* called with controller lock already held */

	if (!epnum) {
		DMSG_PANIC("ERR: sunxi_hcd_dma_completion, not support ep0\n");
	} else {
		/* endpoints 1..15 */
		if (transmit) {
			if (devctl & (1 << USBC_BP_DEVCTL_HOST_MODE)) {
				if (is_host_capable()) {
					sunxi_hcd_host_tx(sunxi_hcd, epnum);
				}
			}
		} else {
			/* receive */
			if (devctl & (1 << USBC_BP_DEVCTL_HOST_MODE)) {
				if (is_host_capable()) {
					sunxi_hcd_host_rx(sunxi_hcd, epnum);
				}
			}
		}
	}

	spin_unlock_irqrestore(&sunxi_hcd->lock, flags);
}
EXPORT_SYMBOL(sunxi_hcd_dma_completion);
#endif

void sunxi_hcd_soft_disconnect(struct sunxi_hcd *sunxi_hcd)
{
	DMSG_INFO("-------sunxi_hcd%d_soft_disconnect---------\n", sunxi_hcd->usbc_no);

	usb_hcd_resume_root_hub(sunxi_hcd_to_hcd(sunxi_hcd));
	sunxi_hcd_root_disconnect(sunxi_hcd);
}

static irqreturn_t sunxi_hcd_stage0_irq(struct sunxi_hcd *sunxi_hcd, u8 int_usb, u8 devctl, u8 power)
{
	irqreturn_t handled = IRQ_NONE;
	void __iomem *usbc_base = sunxi_hcd->mregs;

	DMSG_DBG_HCD("sunxi_hcd_stage0_irq: Power=%02x, DevCtl=%02x, int_usb=0x%x\n", power, devctl, int_usb);

	if (int_usb & (1 << USBC_BP_INTUSB_SOF)) {
		//DMSG_INFO("\n\n------------IRQ SOF-------------\n\n");

		USBC_INT_ClearMiscPending(sunxi_hcd->sunxi_hcd_io->usb_bsp_hdle, (1 << USBC_BP_INTUSB_SOF));
		handled = IRQ_HANDLED;

		USBC_INT_DisableUsbMiscUint(sunxi_hcd->sunxi_hcd_io->usb_bsp_hdle, (1 << USBC_BP_INTUSB_SOF));
	}

	/* in host mode, the peripheral may issue remote wakeup.
	 * in peripheral mode, the host may resume the link.
	 * spurious RESUME irqs happen too, paired with SUSPEND.
	 */
	if (int_usb & (1 << USBC_BP_INTUSB_RESUME)) {
		DMSG_INFO("\n------------IRQ RESUME-------------\n\n");

		USBC_INT_ClearMiscPending(sunxi_hcd->sunxi_hcd_io->usb_bsp_hdle, (1 << USBC_BP_INTUSB_RESUME));

		handled = IRQ_HANDLED;

		if (devctl & (1 << USBC_BP_DEVCTL_HOST_MODE)) {
			if (power & (1 << USBC_BP_POWER_H_SUSPEND)) {
				/* spurious */
				sunxi_hcd->int_usb &= ~(1 << USBC_BP_INTUSBE_EN_SUSPEND);

				DMSG_INFO("sunxi_hcd_stage0_irq, Spurious SUSPENDM\n");
				//break;
			}

			power &= ~(1 << USBC_BP_POWER_H_SUSPEND);
			power |= (1 << USBC_BP_POWER_H_RESUME);
			USBC_Writeb(power, USBC_REG_PCTL(usbc_base));

			sunxi_hcd->port1_status |= (USB_PORT_STAT_C_SUSPEND << 16) | SUNXI_HCD_PORT_STAT_RESUME;
			sunxi_hcd->rh_timer = jiffies + msecs_to_jiffies(20);
			sunxi_hcd->is_active = 1;
			usb_hcd_resume_root_hub(sunxi_hcd_to_hcd(sunxi_hcd));
		} else {
			usb_hcd_resume_root_hub(sunxi_hcd_to_hcd(sunxi_hcd));
		}
	}

	/* see manual for the order of the tests */
	if (int_usb & (1 << USBC_BP_INTUSB_SESSION_REQ)) {
		DMSG_INFO("\n------------IRQ SESSION_REQ-------------\n\n");

		USBC_INT_ClearMiscPending(sunxi_hcd->sunxi_hcd_io->usb_bsp_hdle, (1 << USBC_BP_INTUSB_SESSION_REQ));
		handled = IRQ_HANDLED;

		USBC_INT_DisableUsbMiscUint(sunxi_hcd->sunxi_hcd_io->usb_bsp_hdle, (1 << USBC_BP_INTUSBE_EN_SESSION_REQ));
		sunxi_hcd->session_req_flag = 1;

		sunxi_hcd->ep0_stage = SUNXI_HCD_EP0_START;
	}

	if (int_usb & (1 << USBC_BP_INTUSB_VBUS_ERROR)) {
		int	ignore = 0;

		DMSG_INFO("\n------------IRQ VBUS_ERROR-------------\n\n");

		USBC_INT_ClearMiscPending(sunxi_hcd->sunxi_hcd_io->usb_bsp_hdle, (1 << USBC_BP_INTUSB_VBUS_ERROR));
		handled = IRQ_HANDLED;

		/* recovery is dicey once we've gotten past the
		 * initial stages of enumeration, but if VBUS
		 * stayed ok at the other end of the link, and
		 * another reset is due (at least for high speed,
		 * to redo the chirp etc), it might work OK...
		 */
		if (sunxi_hcd->vbuserr_retry) {
			sunxi_hcd->vbuserr_retry--;
			ignore  = 1;

			devctl |= (1 << USBC_BP_DEVCTL_SESSION);
			USBC_Writeb(devctl, USBC_REG_DEVCTL(usbc_base));
		} else {
			sunxi_hcd->port1_status |= (1 << USB_PORT_FEAT_OVER_CURRENT)
				| (1 << USB_PORT_FEAT_C_OVER_CURRENT);
		}

		/* go through A_WAIT_VFALL then start a new session */
		if (!ignore) {
			USBC_INT_DisableUsbMiscUint(sunxi_hcd->sunxi_hcd_io->usb_bsp_hdle, (1 << USBC_BP_INTUSB_VBUS_ERROR));
			sunxi_hcd->vbus_error_flag = 1;
			sunxi_hcd->ep0_stage = SUNXI_HCD_EP0_START;
		}
	}

	if (int_usb & (1 << USBC_BP_INTUSB_CONNECT)) {
		struct usb_hcd *hcd = sunxi_hcd_to_hcd(sunxi_hcd);

		DMSG_INFO("\n------------IRQ CONNECT-------------\n\n");

{
		int old_ep_index = 0;
		old_ep_index = USBC_GetActiveEp(sunxi_hcd->sunxi_hcd_io->usb_bsp_hdle);
		USBC_SelectActiveEp(sunxi_hcd->sunxi_hcd_io->usb_bsp_hdle, 0);
		USBC_Host_DisablePing(sunxi_hcd->sunxi_hcd_io->usb_bsp_hdle);
		USBC_SelectActiveEp(sunxi_hcd->sunxi_hcd_io->usb_bsp_hdle, old_ep_index);
}
		USBC_INT_ClearMiscPending(sunxi_hcd->sunxi_hcd_io->usb_bsp_hdle, (1 << USBC_BP_INTUSB_CONNECT));
		handled = IRQ_HANDLED;

		sunxi_hcd->is_active = 1;
		sunxi_hcd->is_connected = 1;
		//set_bit(HCD_FLAG_SAW_IRQ, &hcd->flags);

		sunxi_hcd->ep0_stage = SUNXI_HCD_EP0_START;

		sunxi_hcd->port1_status &= ~(USB_PORT_STAT_LOW_SPEED
			|USB_PORT_STAT_HIGH_SPEED
			|USB_PORT_STAT_ENABLE
			);
		sunxi_hcd->port1_status |= USB_PORT_STAT_CONNECTION
			|(USB_PORT_STAT_C_CONNECTION << 16);

		/* high vs full speed is just a guess until after reset */
		if (devctl & (1 << USBC_BP_DEVCTL_LS_DEV)) {
			sunxi_hcd->port1_status |= USB_PORT_STAT_LOW_SPEED;
		}

		if (hcd->status_urb) {
			usb_hcd_poll_rh_status(hcd);
		} else {
			usb_hcd_resume_root_hub(hcd);
		}

		SUNXI_HCD_HST_MODE(sunxi_hcd);
	}

	/* mentor saves a bit: bus reset and babble share the same irq.
	 * only host sees babble; only peripheral sees bus reset.
	 */
	if (int_usb & (1 << USBC_BP_INTUSB_RESET)) {
		DMSG_INFO("\n------------IRQ Reset or Babble-------------\n\n");

		USBC_INT_ClearMiscPending(sunxi_hcd->sunxi_hcd_io->usb_bsp_hdle, (1 << USBC_BP_INTUSB_RESET));
		handled = IRQ_HANDLED;
		USBC_INT_DisableUsbMiscUint(sunxi_hcd->sunxi_hcd_io->usb_bsp_hdle, (1 << USBC_BP_INTUSB_RESET));
		sunxi_hcd->reset_flag = 1;
		//sunxi_hcd->is_connected = 0;

		//treat babble as disconnect
		USBC_Host_SetFunctionAddress_Deafult(sunxi_hcd->sunxi_hcd_io->usb_bsp_hdle, USBC_EP_TYPE_TX, 0);
		{
			u32 i = 1;
			for( i = 1 ; i <= 5; i++) {
				USBC_Host_SetFunctionAddress_Deafult(sunxi_hcd->sunxi_hcd_io->usb_bsp_hdle, USBC_EP_TYPE_TX, i);
				USBC_Host_SetFunctionAddress_Deafult(sunxi_hcd->sunxi_hcd_io->usb_bsp_hdle, USBC_EP_TYPE_RX, i);
			}
		}

		/* clear all the irq status of the plugout devie, now has no hub, so can clear all irqs */
		USBC_INT_ClearMiscPendingAll(sunxi_hcd->sunxi_hcd_io->usb_bsp_hdle);
		USBC_INT_ClearEpPendingAll(sunxi_hcd->sunxi_hcd_io->usb_bsp_hdle, USBC_EP_TYPE_TX);
		USBC_INT_ClearEpPendingAll(sunxi_hcd->sunxi_hcd_io->usb_bsp_hdle, USBC_EP_TYPE_RX);

	}

	schedule_work(&sunxi_hcd->irq_work);
	return handled;
}

/*
 *    Interrupt Service Routine to record USB "global" interrupts.
 * Since these do not happen often and signify things of
 * paramount importance, it seems OK to check them individually;
 * the order of the tests is specified in the manual
 *
 */
static irqreturn_t sunxi_hcd_stage2_irq(struct sunxi_hcd *sunxi_hcd,
						u8 int_usb,
						u8 devctl,
						u8 power)
{
	irqreturn_t handled = IRQ_NONE;

	if ((int_usb & (1 << USBC_BP_INTUSB_DISCONNECT)) && !sunxi_hcd->ignore_disconnect) {
		DMSG_INFO("\n------------IRQ DISCONNECT-------------\n\n");

		USBC_INT_ClearMiscPending(sunxi_hcd->sunxi_hcd_io->usb_bsp_hdle, (1 << USBC_BP_INTUSB_DISCONNECT));

		handled = IRQ_HANDLED;

		usb_hcd_resume_root_hub(sunxi_hcd_to_hcd(sunxi_hcd));
		sunxi_hcd_root_disconnect(sunxi_hcd);

		sunxi_hcd->is_connected = 0;

		schedule_work(&sunxi_hcd->irq_work);
	}

	if (int_usb & (1 << USBC_BP_INTUSB_SUSPEND)) {
		DMSG_INFO("\n------------IRQ SUSPEND-------------\n\n");

		USBC_INT_ClearMiscPending(sunxi_hcd->sunxi_hcd_io->usb_bsp_hdle, (1 << USBC_BP_INTUSB_SUSPEND));

		handled = IRQ_HANDLED;

		/* "should not happen" */
		sunxi_hcd->is_active = 0;

		schedule_work(&sunxi_hcd->irq_work);
	}

	return handled;
}


/*
 *    handle all the irqs defined by the sunxi_hcd core. for now we expect:  other
 * irq sources //(phy, dma, etc) will be handled first, sunxi_hcd->int_* values
 * will be assigned, and the irq will already have been acked.
 *
 * called in irq context with spinlock held, irqs blocked
 *
 */
static irqreturn_t sunxi_hcd_interrupt(struct sunxi_hcd *sunxi_hcd)
{
	irqreturn_t     retval      = IRQ_NONE;
	u8              devctl      = 0;
	u8              power       = 0;
	int             ep_num      = 0;
	u32             reg         = 0;
	void __iomem    *usbc_base  = NULL;

	/* check argment */
	if (sunxi_hcd == NULL) {
		DMSG_PANIC("ERR: invalid argment\n");
		return IRQ_NONE;
	}

	/* initialize parameter */
	usbc_base   = sunxi_hcd->mregs;

	devctl = USBC_Readb(USBC_REG_DEVCTL(usbc_base));
	power  = USBC_Readb(USBC_REG_PCTL(usbc_base));

	DMSG_DBG_HCD("irq: (0x%x, 0x%x, 0x%x)\n", sunxi_hcd->int_usb, sunxi_hcd->int_tx, sunxi_hcd->int_rx);

	/* the core can interrupt us for multiple reasons; docs have
	 * a generic interrupt flowchart to follow
	 */
	if (sunxi_hcd->int_usb & STAGE0_MASK) {
		retval |= sunxi_hcd_stage0_irq(sunxi_hcd, sunxi_hcd->int_usb, devctl, power);
	}

	/* "stage 1" is handling endpoint irqs */

	/* handle endpoint 0 first */
	if (sunxi_hcd->int_tx & 1) {
		USBC_INT_ClearEpPending(sunxi_hcd->sunxi_hcd_io->usb_bsp_hdle, USBC_EP_TYPE_TX, 0);

		retval |= sunxi_hcd_h_ep0_irq(sunxi_hcd);
	}

	/* RX on endpoints 1-15 */
	reg = sunxi_hcd->int_rx >> 1;
	ep_num = 1;
	while (reg) {
		if (reg & 1) {
			USBC_INT_ClearEpPending(sunxi_hcd->sunxi_hcd_io->usb_bsp_hdle,
				USBC_EP_TYPE_RX,
				ep_num);

			/* sunxi_hcd_ep_select(sunxi_hcd->mregs, ep_num); */
			/* REVISIT just retval = ep->rx_irq(...) */
			retval = IRQ_HANDLED;
			if (devctl & (1 << USBC_BP_DEVCTL_HOST_MODE)) {
				if (is_host_capable()) {
					sunxi_hcd_host_rx(sunxi_hcd, ep_num);
				}
			}
		}

		reg >>= 1;
		ep_num++;
	}

	/* TX on endpoints 1-15 */
	reg = sunxi_hcd->int_tx >> 1;
	ep_num = 1;
	while (reg) {
		if (reg & 1) {
			USBC_INT_ClearEpPending(sunxi_hcd->sunxi_hcd_io->usb_bsp_hdle,
				USBC_EP_TYPE_TX,
				ep_num);

			/* sunxi_hcd_ep_select(sunxi_hcd->mregs, ep_num); */
			/* REVISIT just retval |= ep->tx_irq(...) */
			retval = IRQ_HANDLED;
			if (devctl & (1 << USBC_BP_DEVCTL_HOST_MODE)) {
				if (is_host_capable()) {
					sunxi_hcd_host_tx(sunxi_hcd, ep_num);
				}
			}
		}

		reg >>= 1;
		ep_num++;
	}

	/* finish handling "global" interrupts after handling fifos */
	if (sunxi_hcd->int_usb) {
		retval |= sunxi_hcd_stage2_irq(sunxi_hcd, sunxi_hcd->int_usb, devctl, power);
	}

	return retval;
}

static void clear_all_irq(struct sunxi_hcd *sunxi_hcd)
{
	USBC_INT_ClearEpPendingAll(sunxi_hcd->sunxi_hcd_io->usb_bsp_hdle, USBC_EP_TYPE_TX);
	USBC_INT_ClearEpPendingAll(sunxi_hcd->sunxi_hcd_io->usb_bsp_hdle, USBC_EP_TYPE_RX);
	USBC_INT_ClearMiscPendingAll(sunxi_hcd->sunxi_hcd_io->usb_bsp_hdle);
}

irqreturn_t generic_interrupt(int irq, void *__hci)
{
	unsigned long   flags       = 0;
	irqreturn_t     retval      = IRQ_NONE;
	struct sunxi_hcd *sunxi_hcd = NULL;
	void __iomem    *usbc_base  = NULL;

	/* check argment */
	if (__hci == NULL) {
		DMSG_PANIC("ERR: invalid argment\n");
		return IRQ_NONE;
	}

	/* initialize parameter */
	sunxi_hcd   = (struct sunxi_hcd *)__hci;
	usbc_base   = sunxi_hcd->mregs;

	/* host role must be active */
	if (!sunxi_hcd->enable) {
		DMSG_PANIC("ERR: usb generic_interrupt, host is not enable\n");
		clear_all_irq(sunxi_hcd);
		return IRQ_NONE;
	}

	spin_lock_irqsave(&sunxi_hcd->lock, flags);

	sunxi_hcd->int_usb = USBC_Readb(USBC_REG_INTUSB(usbc_base));
	sunxi_hcd->int_tx  = USBC_Readw(USBC_REG_INTTx(usbc_base));
	sunxi_hcd->int_rx  = USBC_Readw(USBC_REG_INTRx(usbc_base));

	if (sunxi_hcd->int_usb || sunxi_hcd->int_tx || sunxi_hcd->int_rx) {
		retval = sunxi_hcd_interrupt(sunxi_hcd);
	}

	spin_unlock_irqrestore(&sunxi_hcd->lock, flags);

	/* REVISIT we sometimes get spurious IRQs on g_ep0
	 * not clear why...
	 */
	if (retval != IRQ_HANDLED) {
		DMSG_INFO("spurious?, int_usb=0x%x, int_tx=0x%x, int_rx=0x%x\n",
		USBC_Readb(USBC_REG_INTUSB(usbc_base)),
		USBC_Readw(USBC_REG_INTTx(usbc_base)),
		USBC_Readw(USBC_REG_INTRx(usbc_base)));
	}

	return IRQ_HANDLED;
}
EXPORT_SYMBOL(generic_interrupt);

