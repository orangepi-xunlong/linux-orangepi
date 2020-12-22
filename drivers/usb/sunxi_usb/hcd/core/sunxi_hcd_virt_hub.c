/*
 * drivers/usb/sunxi_usb/hcd/core/sunxi_hcd_virt_hub.c
 * (C) Copyright 2010-2015
 * Allwinner Technology Co., Ltd. <www.allwinnertech.com>
 * javen, 2010-12-20, create this file
 *
 * usb host contoller driver. virtual hub.
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
#include <linux/errno.h>
#include <linux/init.h>
#include <linux/time.h>
#include <linux/timer.h>

#include <asm/unaligned.h>

#include  "../include/sunxi_hcd_config.h"
#include  "../include/sunxi_hcd_core.h"
#include  "../include/sunxi_hcd_virt_hub.h"

/*
 * only suspend USB port
 * @sw_hcd: usb contoller
 *
 */
void sunxi_hcd_port_suspend_ex(struct sunxi_hcd *sunxi_hcd)
{
	/* if peripheral connect, suspend the device */
	if (sunxi_hcd->is_active) {
		/* suspend usb port */
		USBC_Host_SuspendPort(sunxi_hcd->sunxi_hcd_io->usb_bsp_hdle);

		//mdelay(1000);
	}
}

/*
 * only resume USB port
 * @sw_hcd: usb contoller
 *
 */
void sunxi_hcd_port_resume_ex(struct sunxi_hcd *sunxi_hcd)
{
	/* resume port */
	USBC_Host_RusumePort(sunxi_hcd->sunxi_hcd_io->usb_bsp_hdle);
	//mdelay(500);
	USBC_Host_ClearRusumePortFlag(sunxi_hcd->sunxi_hcd_io->usb_bsp_hdle);
}

/*
 * only reset USB port
 * @sw_hcd: usb contoller
 *
 */
void sunxi_hcd_port_reset_ex(struct sunxi_hcd *sunxi_hcd)
{
	/* resume port */
	sunxi_hcd_port_resume_ex(sunxi_hcd);

	/* reset port */
	USBC_Host_ResetPort(sunxi_hcd->sunxi_hcd_io->usb_bsp_hdle);
	//mdelay(50);
	USBC_Host_ClearResetPortFlag(sunxi_hcd->sunxi_hcd_io->usb_bsp_hdle);
	//mdelay(500);
}

/*
 * suspend USB port
 * @sw_hcd:     usb contoller
 * @do_suspend: flag. is suspend USB port or not?
 *
 */
static void sunxi_hcd_port_suspend(struct sunxi_hcd *sunxi_hcd, bool do_suspend)
{
	u8 power = 0;
	void __iomem *usbc_base = sunxi_hcd->mregs;

	if (!is_host_active(sunxi_hcd) || !sunxi_hcd->enable) {
		DMSG_PANIC("ERR: usb host is not active\n");
		return;
	}

	sunxi_hcd->is_suspend = do_suspend;
	/* NOTE:  this doesn't necessarily put PHY into low power mode,
	 * turning off its clock; that's a function of PHY integration and
	 * sunxi_hcd_POWER_ENSUSPEND.  PHY may need a clock (sigh) to detect
	 * SE0 changing to connect (J) or wakeup (K) states.
	 */
	power = USBC_Readb(USBC_REG_PCTL(usbc_base));
	if (do_suspend) {
		int retries = 10000;

		DMSG_INFO("[sunxi_hcd]: suspend port.\n");

		power &= ~(1 << USBC_BP_POWER_H_RESUME);
		power |= (1 << USBC_BP_POWER_H_SUSPEND);
		USBC_Writeb(power, USBC_REG_PCTL(usbc_base));

		/* Needed for OPT A tests */
		power = USBC_Readb(USBC_REG_PCTL(usbc_base));
		while (power & (1 << USBC_BP_POWER_H_SUSPEND)) {
			power = USBC_Readb(USBC_REG_PCTL(usbc_base));
			if (retries-- < 1)
				break;
		}

		DMSG_DBG_HCD("DBG: Root port suspended, power %02x\n", power);

		sunxi_hcd->port1_status |= USB_PORT_STAT_SUSPEND;
	} else if (power & (1 << USBC_BP_POWER_H_SUSPEND)) {
		DMSG_INFO("[sunxi_hcd]: suspend portend, resume port.\n");

		power &= ~(1 << USBC_BP_POWER_H_SUSPEND);
		power |= (1 << USBC_BP_POWER_H_RESUME);
		USBC_Writeb(power, USBC_REG_PCTL(usbc_base));

		DMSG_DBG_HCD("DBG: Root port resuming, power %02x\n", power);

		/* later, GetPortStatus will stop RESUME signaling */
		sunxi_hcd->port1_status |= SUNXI_HCD_PORT_STAT_RESUME;
		sunxi_hcd->rh_timer = jiffies + msecs_to_jiffies(20);
	} else {
		DMSG_PANIC("WRN: sunxi_hcd_port_suspend nothing to do\n");
	}
}

/*
 * reset USB port
 * @sw_hcd:     usb contoller
 * @do_reset:   flag. is reset USB port or not?
 *
 */
static void sunxi_hcd_port_reset(struct sunxi_hcd *sunxi_hcd, bool do_reset)
{
	u8 power = 0;
	void __iomem *usbc_base = sunxi_hcd->mregs;

	if (!is_host_active(sunxi_hcd) || !sunxi_hcd->enable) {
		DMSG_PANIC("ERR: sunxi_hcd_port_reset, usb host is not active, do_reset=%d\n", do_reset);
		/* clean status */
		if (!do_reset) {
			DMSG_PANIC("sunxi_hcd_port_reset clean status\n");
			sunxi_hcd->port1_status &= ~USB_PORT_STAT_RESET;
			sunxi_hcd->port1_status |= USB_PORT_STAT_ENABLE
				| (USB_PORT_STAT_C_RESET << 16)
				| (USB_PORT_STAT_C_ENABLE << 16);
			usb_hcd_poll_rh_status(sunxi_hcd_to_hcd(sunxi_hcd));
		}
		return;
	}

	sunxi_hcd->is_reset = do_reset;

	/* NOTE:  caller guarantees it will turn off the reset when
	 * the appropriate amount of time has passed
	 */
	power = USBC_Readb(USBC_REG_PCTL(usbc_base));
	if (do_reset) {
		DMSG_INFO("[sunxi_hcd]: reset port. \n");

		/*
		 * If RESUME is set, we must make sure it stays minimum 20 ms.
		 * Then we must clear RESUME and wait a bit to let sunxi_hcd start
		 * generating SOFs. If we don't do this, OPT HS A 6.8 tests
		 * fail with "Error! Did not receive an SOF before suspend
		 * detected".
		 */
		if (power & (1 << USBC_BP_POWER_H_RESUME)) {
			while (time_before(jiffies, sunxi_hcd->rh_timer)) {
				msleep(1);
			}

			power &= ~(1 << USBC_BP_POWER_H_RESUME);
			USBC_Writeb(power, USBC_REG_PCTL(usbc_base));
			udelay(10);
		}

		//sunxi_hcd->ignore_disconnect = true;
		power &= 0xf0;
		power |= (1 << USBC_BP_POWER_H_RESET);
		USBC_Writeb(power, USBC_REG_PCTL(usbc_base));

		sunxi_hcd->port1_status |= USB_PORT_STAT_RESET;
		sunxi_hcd->port1_status &= ~USB_PORT_STAT_ENABLE;
		sunxi_hcd->rh_timer = jiffies + msecs_to_jiffies(50);

		USBC_Host_SetFunctionAddress_Deafult(sunxi_hcd->sunxi_hcd_io->usb_bsp_hdle, USBC_EP_TYPE_TX, 0);

		//set address ep0
		{
			__u32 i = 1;
			__u8 old_ep_index = 0;

			old_ep_index = USBC_GetActiveEp(sunxi_hcd->sunxi_hcd_io->usb_bsp_hdle);

			USBC_SelectActiveEp(sunxi_hcd->sunxi_hcd_io->usb_bsp_hdle, 0);
			USBC_Host_SetFunctionAddress_Deafult(sunxi_hcd->sunxi_hcd_io->usb_bsp_hdle, USBC_EP_TYPE_TX, 0);

			for( i = 1 ; i <= 5; i++) {
				USBC_SelectActiveEp(sunxi_hcd->sunxi_hcd_io->usb_bsp_hdle, i);
				USBC_Host_SetFunctionAddress_Deafult(sunxi_hcd->sunxi_hcd_io->usb_bsp_hdle, USBC_EP_TYPE_TX, i);
				USBC_Host_SetFunctionAddress_Deafult(sunxi_hcd->sunxi_hcd_io->usb_bsp_hdle, USBC_EP_TYPE_RX, i);
			}

			USBC_SelectActiveEp(sunxi_hcd->sunxi_hcd_io->usb_bsp_hdle, old_ep_index);
		}
	} else {
		DMSG_INFO("[sunxi_hcd]: reset port stopped.\n");

		UsbPhyEndReset(0);

		power &= ~(1 << USBC_BP_POWER_H_RESET);
		USBC_Writeb(power, USBC_REG_PCTL(usbc_base));

		sunxi_hcd->ignore_disconnect = false;

		power = USBC_Readb(USBC_REG_PCTL(usbc_base));
		if (power & (1 << USBC_BP_POWER_H_HIGH_SPEED_FLAG)) {
			DMSG_DBG_HCD("high-speed device connected\n");
			sunxi_hcd->port1_status |= USB_PORT_STAT_HIGH_SPEED;
		}

		sunxi_hcd->port1_status &= ~USB_PORT_STAT_RESET;
		sunxi_hcd->port1_status |= USB_PORT_STAT_ENABLE
			| (USB_PORT_STAT_C_RESET << 16)
			| (USB_PORT_STAT_C_ENABLE << 16);
		usb_hcd_poll_rh_status(sunxi_hcd_to_hcd(sunxi_hcd));

		sunxi_hcd->vbuserr_retry = VBUSERR_RETRY_COUNT;
	}
}

/*
 * disconnect usb connection
 * @sw_hcd:     usb contoller
 *
 */
void sunxi_hcd_root_disconnect(struct sunxi_hcd *sunxi_hcd)
{
	/* clean pctrl reg, when disconnect happened during reseting or suspending */
	if ((sunxi_hcd->port1_status & USB_PORT_STAT_SUSPEND)
		|| (sunxi_hcd->port1_status & USB_PORT_STAT_RESET)
		|| (sunxi_hcd->port1_status & SUNXI_HCD_PORT_STAT_RESUME)) {
		u8 power = 0;
		void __iomem *usbc_base = sunxi_hcd->mregs;

		DMSG_INFO("[sunxi_hcd]: clean pctrl reg, when disconnect happen during reseting or suspending,"
			"status =0x%x\n", sunxi_hcd->port1_status);

		power = USBC_Readb(USBC_REG_PCTL(usbc_base));
		power &= ~(1 << USBC_BP_POWER_H_SUSPEND);
		power &= ~(1 << USBC_BP_POWER_H_RESUME);
		power &= ~(1 << USBC_BP_POWER_H_RESET);
		USBC_Writeb(power, USBC_REG_PCTL(usbc_base));
	}
	sunxi_hcd->port1_status = (1 << USB_PORT_FEAT_POWER)
		| (1 << USB_PORT_FEAT_C_CONNECTION);

	usb_hcd_poll_rh_status(sunxi_hcd_to_hcd(sunxi_hcd));
	sunxi_hcd->is_active = 0;
}
EXPORT_SYMBOL(sunxi_hcd_root_disconnect);

/*
 * Caller may or may not hold sunxi_hcd->lock
 *
 */
int sunxi_hcd_hub_status_data(struct usb_hcd *hcd, char *buf)
{
	struct sunxi_hcd *sunxi_hcd = hcd_to_sunxi_hcd(hcd);
	int retval = 0;

	/* called in_irq() via usb_hcd_poll_rh_status() */
	if (sunxi_hcd->port1_status & 0xffff0000) {
		*buf = 0x02;
		retval = 1;
	}

	return retval;
}
EXPORT_SYMBOL(sunxi_hcd_hub_status_data);

int sunxi_hcd_hub_control(struct usb_hcd	*hcd,
					u16 typeReq,
					u16 wValue,
					u16 wIndex,
					char *buf,
					u16 wLength)
{
	struct sunxi_hcd *sunxi_hcd = hcd_to_sunxi_hcd(hcd);
	u32 temp = 0;
	int retval = 0;
	unsigned long flags = 0;
	void __iomem *usbc_base = sunxi_hcd->mregs;
	int set_vbus_flag = -1;

	if (hcd == NULL) {
		DMSG_PANIC("ERR: invalid argment\n");
		return -ENODEV;
	}

	/*
	if (!is_host_active(sunxi_hcd) || !sunxi_hcd->enable) {
		DMSG_PANIC("ERR: sunxi_hcd_hub_control, usb host is not active\n");
		return -ENOTCONN;
	}
	*/

	spin_lock_irqsave(&sunxi_hcd->lock, flags);

	if (unlikely(!test_bit(HCD_FLAG_HW_ACCESSIBLE, &hcd->flags))) {
		spin_unlock_irqrestore(&sunxi_hcd->lock, flags);
		return -ENODEV;
	}

	DMSG_DBG_HCD("sunxi_hcd_hub_control: typeReq = %x, wValue = 0x%x, wIndex = 0x%x\n",
		typeReq, wValue, wIndex);

	/* hub features:  always zero, setting is a NOP
	 * port features: reported, sometimes updated when host is active
	 * no indicators
	 */
	switch (typeReq) {
	case ClearHubFeature:
	case SetHubFeature:
		switch (wValue) {
		case C_HUB_OVER_CURRENT:
		case C_HUB_LOCAL_POWER:
			break;
		default:
			goto error;
		}
		break;
	case ClearPortFeature:
		if ((wIndex & 0xff) != 1) {
			goto error;
		}

		switch (wValue) {
		case USB_PORT_FEAT_ENABLE:
			break;
		case USB_PORT_FEAT_SUSPEND:
			sunxi_hcd_port_suspend(sunxi_hcd, false);
			break;
		case USB_PORT_FEAT_POWER:
			/* fixme */
			//sunxi_hcd_set_vbus(sunxi_hcd, 0);
			set_vbus_flag = USB_SET_VBUS_OFF;
			break;
		case USB_PORT_FEAT_C_CONNECTION:
		case USB_PORT_FEAT_C_ENABLE:
		case USB_PORT_FEAT_C_OVER_CURRENT:
		case USB_PORT_FEAT_C_RESET:
		case USB_PORT_FEAT_C_SUSPEND:
			break;
		default:
			goto error;
		}

		DMSG_DBG_HCD("DBG: clear feature %d\n", wValue);
		sunxi_hcd->port1_status &= ~(1 << wValue);
		break;
	case GetHubDescriptor:
		{
			struct usb_hub_descriptor *desc = (void *)buf;

			desc->bDescLength = 9;
			desc->bDescriptorType = 0x29;
			desc->bNbrPorts = 1;
			desc->wHubCharacteristics = cpu_to_le16(
				0x0001	/* per-port power switching */
				| 0x0010	/* no overcurrent reporting */
				);
			desc->bPwrOn2PwrGood = 5;	/* msec/2 */
			desc->bHubContrCurrent = 0;

			/* workaround bogus struct definition */
			desc->u.hs.DeviceRemovable[0] = 0x02;	/* port 1 */
			desc->u.hs.DeviceRemovable[1] = 0xff;
		}
		break;
	case GetHubStatus:
		temp = 0;
		*(__le32 *) buf = cpu_to_le32(temp);
		break;
	case GetPortStatus:
		{
			if (wIndex != 1) {
				DMSG_PANIC("ERR: GetPortStatus parameter wIndex is not 1.\n");
				goto error;
			}

			/* finish RESET signaling? */
			if ((sunxi_hcd->port1_status & USB_PORT_STAT_RESET)
				&& time_after_eq(jiffies, sunxi_hcd->rh_timer)) {
				sunxi_hcd_port_reset(sunxi_hcd, false);
			}

			/* finish RESUME signaling? */
			if ((sunxi_hcd->port1_status & SUNXI_HCD_PORT_STAT_RESUME)
				&& time_after_eq(jiffies, sunxi_hcd->rh_timer)) {
				u8 power = 0;

				if (!sunxi_hcd->enable) {
					power = USBC_Readb(USBC_REG_PCTL(usbc_base));
					power &= ~(1 << USBC_BP_POWER_H_RESUME);
					USBC_Writeb(power, USBC_REG_PCTL(usbc_base));
				}

				DMSG_DBG_HCD("DBG: root port resume stopped, power %02x\n", power);

				/* ISSUE:  DaVinci (RTL 1.300) disconnects after
				 * resume of high speed peripherals (but not full
				 * speed ones).
				 */
				sunxi_hcd->is_active = 1;
				sunxi_hcd->port1_status &= ~(USB_PORT_STAT_SUSPEND
					| SUNXI_HCD_PORT_STAT_RESUME);
				sunxi_hcd->port1_status |= USB_PORT_STAT_C_SUSPEND << 16;

				usb_hcd_poll_rh_status(sunxi_hcd_to_hcd(sunxi_hcd));
			}

			put_unaligned(cpu_to_le32(sunxi_hcd->port1_status
				& ~SUNXI_HCD_PORT_STAT_RESUME),
				(__le32 *) buf);

			/* port change status is more interesting */
			DMSG_DBG_HCD("DBG: port status %08x\n", sunxi_hcd->port1_status);
		}
		break;
	case SetPortFeature:
		{
			if ((wIndex & 0xff) != 1) {
				goto error;
			}

			switch (wValue) {
			case USB_PORT_FEAT_POWER:
				/* NOTE: this controller has a strange state machine
	 		 	 * that involves "requesting sessions" according to
	 			 * magic side effects from incompletely-described
	 			 * rules about startup...
	 			 *
	 			 * This call is what really starts the host mode; be
	 			 * very careful about side effects if you reorder any
	 			 * initialization logic, e.g. for OTG, or change any
	 			 * logic relating to VBUS power-up.
				 */
				sunxi_hcd_start(sunxi_hcd);
				set_vbus_flag = USB_SET_VBUS_ON;
				break;
			case USB_PORT_FEAT_RESET:
				sunxi_hcd_port_reset(sunxi_hcd, true);
				break;
			case USB_PORT_FEAT_SUSPEND:
				sunxi_hcd_port_suspend(sunxi_hcd, true);
				break;
			case USB_PORT_FEAT_TEST:
				{
					if (unlikely(is_host_active(sunxi_hcd))) {
						DMSG_PANIC("ERR: usb host is not active\n");
						goto error;
					}

					wIndex >>= 8;
					switch (wIndex) {
					case 1:
						DMSG_DBG_HCD("TEST_J\n");
						temp =  1 << USBC_BP_TMCTL_TEST_J;
						break;
					case 2:
						DMSG_DBG_HCD("TEST_K\n");
						temp = 1 << USBC_BP_TMCTL_TEST_K;
						break;
					case 3:
						DMSG_DBG_HCD("TEST_SE0_NAK\n");
						temp = 1 << USBC_BP_TMCTL_TEST_SE0_NAK;
						break;
					case 4:
						DMSG_DBG_HCD("TEST_PACKET\n");
						temp = 1 << USBC_BP_TMCTL_TEST_PACKET;
						sunxi_hcd_load_testpacket(sunxi_hcd);
						break;
					case 5:
						DMSG_DBG_HCD("TEST_FORCE_ENABLE\n");
						temp = (1 << USBC_BP_TMCTL_FORCE_HOST)
							| (1 << USBC_BP_TMCTL_FORCE_HS);

						USBC_REG_set_bit_b(USBC_BP_DEVCTL_SESSION, USBC_REG_DEVCTL(usbc_base));
						break;
					case 6:
						DMSG_DBG_HCD("TEST_FIFO_ACCESS\n");
						temp = 1 << USBC_BP_TMCTL_FIFO_ACCESS;
						break;
					default:
						DMSG_PANIC("ERR: unkown SetPortFeature USB_PORT_FEAT_TEST wIndex(%d)\n", wIndex);
						goto error;
					}

					USBC_Writeb(temp, USBC_REG_TMCTL(usbc_base));
				}
				break;
			default:
				DMSG_PANIC("ERR: unkown SetPortFeature wValue(%d)\n", wValue);
				goto error;
			}

			DMSG_DBG_HCD("DBG: set feature %d\n", wValue);
			sunxi_hcd->port1_status |= 1 << wValue;
		}
		break;
	default:
error:
		DMSG_PANIC("ERR: protocol stall on error\n");

		/* "protocol stall" on error */
		retval = -EPIPE;
	}

	spin_unlock_irqrestore(&sunxi_hcd->lock, flags);

	if (set_vbus_flag == USB_SET_VBUS_OFF) {
		sunxi_hcd_set_vbus(sunxi_hcd, set_vbus_flag);
	} else if (set_vbus_flag == USB_SET_VBUS_ON) {
		if (sunxi_hcd->enable && !sunxi_hcd->init_controller) {
			sunxi_hcd_set_vbus(sunxi_hcd, set_vbus_flag);
		}
	}

	return retval;
}
EXPORT_SYMBOL(sunxi_hcd_hub_control);

