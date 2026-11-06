/* Copyright (C) 2023 MacroSilicon Technology Co., Ltd.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * usb_hal_thread.c -- Framebuffer driver for MacroSilicon chip 913x and 912x
 */

#include <linux/slab.h>
#include <linux/usb.h>
#include <linux/kfifo.h>

#include "usb_hal_interface.h"
#include "usb_hal_dev.h"
#include "usb_hal_event.h"
#include "usb_hal_thread.h"
#include "hal_adaptor.h"

struct usb_hal_api_context
{
	struct completion done;
	int status;
};

static void usb_hal_api_blocking_completion(struct urb *urb)
{
	struct usb_hal_api_context *ctx = urb->context;

	ctx->status = urb->status;
	complete(&ctx->done);
}

static int usb_hal_start_wait_urb(struct urb *urb, int timeout, int *actual_length)
{
	struct usb_hal_api_context ctx;
	unsigned long expire;
	int retval;

	init_completion(&ctx.done);
	urb->context = &ctx;
	urb->actual_length = 0;
	retval = usb_submit_urb(urb, GFP_NOIO);
	if (unlikely(retval))
		goto out;

	expire = timeout ? msecs_to_jiffies(timeout) : MAX_SCHEDULE_TIMEOUT;
	if (!wait_for_completion_timeout(&ctx.done, expire))
	{
		usb_kill_urb(urb);
		retval = (ctx.status == -ENOENT ? -ETIMEDOUT : ctx.status);
	}
	else
		retval = ctx.status;
out:
	if (actual_length)
		*actual_length = urb->actual_length;

	return retval;
}

static int usb_hal_dev_send_frame(struct usb_hal_dev *usb_dev, struct urb *data_urb, unsigned char *zero_msg, int ep)
{
	int real_ret, ret, snd_len;
	struct usb_device *udev = usb_dev->udev;

	real_ret = 0;
	usb_dev->stat.send_total++;
	mutex_lock(&usb_dev->usb_buf.mutex);

	usb_fill_bulk_urb(data_urb, udev, usb_sndbulkpipe(udev, ep), usb_dev->usb_buf.buf, usb_dev->usb_buf.len,
			  usb_hal_api_blocking_completion, NULL);

	if ((USB_HAL_BUF_TYPE_USB == usb_dev->usb_buf.type) || (USB_HAL_BUF_TYPE_DMA == usb_dev->usb_buf.type))
	{
		data_urb->transfer_dma = usb_dev->usb_buf.dma_addr;
		data_urb->transfer_flags |= URB_NO_TRANSFER_DMA_MAP;
	}
	else if (USB_HAL_BUF_TYPE_VMALLOC == usb_dev->usb_buf.type)
	{
		data_urb->num_sgs = usb_dev->usb_buf.sgt->orig_nents;
		data_urb->sg = usb_dev->usb_buf.sgt->sgl;
	}

	ret = usb_hal_start_wait_urb(data_urb, 2000, &snd_len);
	if (ret)
	{
		dev_err(&udev->dev, "wait urb failed!\n ret = %d\n", ret);
		real_ret = ret;
	}
	else
	{
		usb_dev->stat.send_success++;
	}

	ret = usb_bulk_msg(udev, usb_sndbulkpipe(udev, ep), zero_msg, 0, &snd_len, 2000);
	if (ret)
	{
		dev_err(&udev->dev, "send zero msg failed! ret=%d\n", ret);
	}

	usb_dev->frame_index = ((0 == usb_dev->frame_index) ? 1 : 0);
	usb_dev->hal_dev->funcs->trigger_frame(usb_dev->udev, usb_dev->frame_index, 100);

	mutex_unlock(&usb_dev->usb_buf.mutex);

	return real_ret;
}

static void usb_hal_dev_do_enable(struct usb_hal_dev *usb_dev, struct usb_hal_event *event)
{
	unsigned char frame_index = 0;
	int ret;
	const struct msdisp_hal_dev *hal_dev = usb_dev->hal_dev;
	struct usb_device *udev = usb_dev->udev;
	struct usb_hal *hal = usb_dev->hal;

	ret = hal_dev->funcs->event_proc(usb_dev->udev, event, hal->chip_id, hal->port_type, hal->sdram_type);
	if (ret)
	{
		dev_err(&udev->dev, "hal dev enable event proc failed! ret=%d\n", ret);
	}

	ret = hal_dev->funcs->current_frame_index(udev, &frame_index);
	if (!ret)
	{
		usb_dev->frame_index = frame_index;
		dev_info(&udev->dev, "current index:%d\n", frame_index);
	}
	else
	{
		dev_info(&udev->dev, "get frame index failed! rtn = %d\n", ret);
		usb_dev->frame_index = 0;
	}

	usb_dev->state = USB_HAL_DEV_STATE_ENABLED;
	usb_dev->first_buf_send = 0;
}

static void usb_hal_dev_do_disable(struct usb_hal_dev *usb_dev, struct usb_hal_event *event)
{
	int ret;
	const struct msdisp_hal_dev *hal_dev = usb_dev->hal_dev;
	struct usb_device *udev = usb_dev->udev;
	struct usb_hal *hal = usb_dev->hal;

	ret = hal_dev->funcs->event_proc(usb_dev->udev, event, hal->chip_id, hal->port_type, hal->sdram_type);
	if (ret)
	{
		dev_err(&udev->dev, "hal dev disable event proc failed! ret=%d\n", ret);
	}

	usb_dev->state = USB_HAL_DEV_STATE_DISABLED;
}

static void usb_hal_dev_do_update(struct usb_hal_dev *usb_dev, struct urb *data_urb, unsigned char *zero_msg, int ep, struct usb_hal_event *event)
{
	int ret;
	const struct msdisp_hal_dev *hal_dev = usb_dev->hal_dev;
	struct usb_device *udev = usb_dev->udev;

	usb_dev->stat.update_event++;
	usb_dev->wait_send_cnt = 0;
	usb_dev->usb_buf.len = event->para.update.len;
	ret = usb_hal_dev_send_frame(usb_dev, data_urb, zero_msg, ep);
	if (ret)
	{
		goto out;
	}

	if (0 == usb_dev->first_buf_send)
	{
		struct usb_hal *hal = usb_dev->hal;
		ret = hal_dev->funcs->set_video_enable(udev, 1);
		if (ret)
		{
			dev_err(&udev->dev, "start video failed! rtn = %d\n", ret);
		}

		ret = hal_dev->funcs->set_screen_enable(udev, 1, hal->chip_id, hal->port_type, hal->sdram_type);
		if (ret)
		{
			dev_err(&udev->dev, "stop screen failed! rtn = %d\n", ret);
		}

		usb_dev->first_buf_send = 1;
		dev_info(&udev->dev, "start video success!\n");
	}
out:
	return;
}

void usb_hal_dev_state_unknown(struct usb_hal_dev *usb_dev, struct usb_hal_event *event)
{
	if (USB_HAL_EVENT_TYPE_ENABLE == event->base.type)
	{

		dev_info(&usb_dev->udev->dev, "event:%x width:%d height:%d\n", event->base.type, event->para.enable.width,
			 event->para.enable.height);
	}

	/* in unknown state, only process enable event*/
	if (event->base.type != USB_HAL_EVENT_TYPE_ENABLE)
	{
		return;
	}

	usb_hal_dev_do_enable(usb_dev, event);
}

void usb_hal_dev_state_enable(struct usb_hal_dev *usb_dev, struct urb *data_urb, unsigned char *zero_msg, int ep, struct usb_hal_event *event)
{
	if (USB_HAL_EVENT_TYPE_DISABLE == event->base.type)
	{
		usb_hal_dev_do_disable(usb_dev, event);
		return;
	}

	if (USB_HAL_EVENT_TYPE_UPDATE == event->base.type)
	{
		usb_hal_dev_do_update(usb_dev, data_urb, zero_msg, ep, event);
	}
}

void usb_hal_state_machine(struct usb_hal_dev *usb_dev, struct urb *data_urb, unsigned char *zero_msg, int ep, struct kfifo *fifo)
{
	int len, ret;
	struct usb_hal_event event;

	ret = down_timeout(&usb_dev->sema, msecs_to_jiffies(USB_HAL_BUF_WAIT_TIME));
	// if usb will be suspend, not process, until usb resume
	if (MS9132_USB_BUS_STATUS_SUSPEND == usb_dev->bus_status)
	{
		return;
	}
	// event received, proc event
	if (!ret)
	{
		while ((len = kfifo_out(fifo, &event, sizeof(event)) != 0))
		{
			switch (usb_dev->state)
			{
			case USB_HAL_DEV_STATE_UNKNOWN:
			case USB_HAL_DEV_STATE_DISABLED:
				usb_hal_dev_state_unknown(usb_dev, &event);
				break;
			case USB_HAL_DEV_STATE_ENABLED:
				usb_hal_dev_state_enable(usb_dev, data_urb, zero_msg, ep, &event);
				break;
			}
		}
		return;
	}

	// in enable state, must send frame to usb chip periodly.
	if ((USB_HAL_DEV_STATE_ENABLED == usb_dev->state) && (usb_dev->first_buf_send))
	{
		usb_dev->wait_send_cnt++;
		if (usb_dev->wait_send_cnt < (USB_HAL_BUF_TIMEOUT / USB_HAL_BUF_WAIT_TIME))
		{
			return;
		}

		usb_dev->stat.period_send++;
		usb_dev->wait_send_cnt = 0;
		(void)usb_hal_dev_send_frame(usb_dev, data_urb, zero_msg, ep);
	}
}

int usb_hal_state_machine_entry(void *data)
{
	struct usb_hal *usb_hal = (struct usb_hal *)data;
	struct usb_hal_dev *usb_dev = (struct usb_hal_dev *)usb_hal->private;
	struct urb *data_urb;
	int ep;
	unsigned char *zero_msg;
	struct kfifo *fifo;

	dev_info(&usb_dev->udev->dev, "state machine proc enter!\n");

	fifo = usb_dev->fifo;

	zero_msg = kmalloc(8, GFP_KERNEL);
	if (!zero_msg)
	{
		return -ENOMEM;
	}

	data_urb = usb_alloc_urb(0, GFP_KERNEL);
	if (!data_urb)
	{
		kfree(zero_msg);
		return -ENOMEM;
	}

	ep = usb_dev->hal_dev->funcs->get_transfer_bulk_ep();
	/* wait for drm enable */
	while (usb_dev->thread_run_flag)
	{
		usb_hal_state_machine(usb_dev, data_urb, zero_msg, ep, fifo);
	}

	usb_free_urb(data_urb);
	kfree(zero_msg);

	return 0;
}

void usb_hal_stop_thread(struct usb_hal_dev *usb_dev)
{
	usb_dev->thread_run_flag = 0;
}