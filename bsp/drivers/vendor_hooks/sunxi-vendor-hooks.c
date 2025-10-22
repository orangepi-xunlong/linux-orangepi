/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * SPDX-License-Identifier: GPL-2.0
 *
 * Allwinner Vendor Hooks
 *
 * Copyright (C) 2022 Allwinner.
 */

#include <linux/version.h>
#include <linux/module.h>
#include <trace/hooks/mm.h>
#include <linux/pagemap.h>
#include <linux/backing-dev.h>
#include <linux/dma-buf.h>
#include <linux/seq_file.h>
#include <trace/hooks/dmabuf.h>
#include <linux/gpio.h>
#include <linux/of_gpio.h>
#include <trace/hooks/sched.h>
#include <linux/cpufreq.h>
#include <linux/usb.h>
#include <linux/hid.h>
#include <trace/hooks/usb.h>
#include <trace/hooks/audio_usboffload.h>
#include <sound/asound.h>
#include <../../sound/usb/card.h>

size_t dma_heap_total;

static void sunxi_show_mem(void *p, unsigned int filter, nodemask_t *nodemask)
{
	printk(KERN_NOTICE "%lu kB dmaheap\n", dma_heap_total / 1024);
}

static void sunxi_meminfo_proc_show(void *p, struct seq_file *m)
{
	seq_printf(m, "DmaHeap:        %8zu kB\n", dma_heap_total / 1024);
}

static void sunxi_dmabuf_heap_flags_validation(void *p, struct dma_heap *heap, size_t len,
	unsigned int fd_flags, unsigned int heap_flags, bool *skip)
{
	dma_heap_total += len;
}

static void sunxi_dma_buf_release(void *p, struct dma_buf *dmabuf)
{
	dma_heap_total -= dmabuf->size;
}

static void sunxi_audio_usb_set_urb_param(void *p, void *arg, bool action)
{
	struct snd_usb_endpoint *ep;
	unsigned int i;

	ep = arg;
	for (i = 0; i < ep->nurbs; i++) {
		struct urb *urb = ep->urb[i].urb;
		urb->transfer_flags = URB_ISO_ASAP | URB_NO_TRANSFER_DMA_MAP;
	}
}

static struct usb_device *udev;
static char *envp[3] = {"SYSTEM=USB_AUDIO", NULL, NULL};

static void usb_audio_event_work(struct work_struct *work)
{
	kobject_uevent_env(&udev->dev.kobj, KOBJ_CHANGE, envp);
}

static DECLARE_WORK(usb_audio_work, usb_audio_event_work);

static int usb_audio_set_dfs_event(struct usb_interface *intf, bool enable)
{
	struct usb_device *dev = interface_to_usbdev(intf);

	if (enable)
		envp[1] = "EVENT=ON";
	else
		envp[1] = "EVENT=OFF";

	udev = dev;
	schedule_work(&usb_audio_work);

	dev_dbg(&udev->dev, "%s usb audio %s, set dfs event %s\n", usb_speed_string(udev->speed),
		enable ? "connect" : "disconnect", enable ? "on" : "off");

	return 0;
}

static void sunxi_audio_usb_connect(void *p, struct usb_interface *intf, struct snd_usb_audio *chip)
{
	usb_audio_set_dfs_event(intf, true);
}

static void sunxi_audio_usb_disconnect(void *p, struct usb_interface *intf)
{
	usb_audio_set_dfs_event(intf, false);
}

static int hid_is_interface(struct usb_host_interface *intf)
{
	return (intf->desc.bInterfaceClass == USB_INTERFACE_CLASS_HID &&
		intf->desc.bInterfaceSubClass == USB_INTERFACE_SUBCLASS_BOOT &&
		intf->desc.bInterfaceProtocol == USB_INTERFACE_PROTOCOL_MOUSE);
}

/* Re-Initialize mouse hid device wakeup to enable */
static int usb_reinit_hid_wakeup(struct usb_device *udev)
{
	unsigned int i;
	int ret = 0;

	if (!udev->parent) {
		/* Roothub don't reinit wakeup */
		return ret;
	}

	for (i = 0; i < udev->descriptor.bNumConfigurations; ++i) {
		struct usb_host_config *cfg = &udev->config[i];
		unsigned int j;
		for (j = 0; j < cfg->desc.bNumInterfaces; ++j) {
			struct usb_interface_cache *cache;
			struct usb_host_interface *intf;
			unsigned int k;

			cache = cfg->intf_cache[j];
			if (cache->num_altsetting == 0)
				continue;

			for (k = 0; k < cache->num_altsetting; ++k) {
				intf = &cache->altsetting[k];
				if (hid_is_interface(intf)) {
					ret = device_init_wakeup(&udev->dev, 1);
					return ret;
				}
			}
		}
	}
	return ret;
}

static void sunxi_usb_new_device_added(void *p, struct usb_device *udev, int *err)
{
	/* *
	 * The usb_new_device initialize non-root-hub device wakeup to disabled.
	 * It's necessary to enable remote wakeup by default for all mouse devices
	 * supporting the boot protocol.
	 */
	*err = usb_reinit_hid_wakeup(udev);
}

/* implement the inode_to_bdi interface of the aw platform */
static struct backing_dev_info *sunxi_inode_to_bdi(struct inode *inode)
{
	struct super_block *sb;

	if (!inode)
		return NULL;

	sb = inode->i_sb;

#ifdef CONFIG_BLOCK
	if (sb_is_blkdev_sb(sb))
		return I_BDEV(inode)->bd_disk->bdi;
#endif

	return sb->s_bdi;
}

static void sunxi_page_cache_forced_ra(void *p, struct readahead_control *ractl, unsigned long req_count, bool *do_forced_ra)
{
	struct backing_dev_info *bdi = sunxi_inode_to_bdi(ractl->mapping->host);
	unsigned long max_pages;
	bool do_rd_ra = ractl->file && (ractl->file->f_mode & FMODE_RANDOM);

	if (!bdi)
		return;

	max_pages = bdi->io_pages;

	/*
	 * Asynchronous read ahead is considered
	 * only when max_pages is greater than or equal to
	 * the quadruple of req_size, which avoid shocks in some special cases.
	 * If one day you find that there is a shock,
	 * you can still set this confficient higher.
	 */
	if ((do_rd_ra) && (*do_forced_ra) && (max_pages * 4 < req_count))
		*do_forced_ra = false;
}

#ifdef CONFIG_AW_RANDOM_INIT
static int irq_count;
static int virq;

static irqreturn_t sunxi_vh_irq_handler(int irq, void *data)
{
	irq_count++;
	if (irq_count == 12500)
		disable_irq_nosync(virq);

	return IRQ_HANDLED;
}

/* Use interrupt of pin to speed up random init */
static void sunxi_random_init(void)
{
	int ret;
	struct device_node *vh_np;
	int vh_gpio;

	irq_count = 0;
	vh_np = of_find_node_by_name(NULL, "vendor_hook");
	if (!vh_np)
		pr_err("%s: of_find_node_by_name failed!\n", __func__);

	vh_gpio = of_get_named_gpio(vh_np, "random_init_pin", 0);
	if (!gpio_is_valid(vh_gpio))
		pr_err("%s: of_get_named_gpio failed!\n", __func__);

	virq = gpio_to_irq(vh_gpio);
	ret = request_irq(virq, sunxi_vh_irq_handler,
			       IRQF_TRIGGER_HIGH, "random_init_irq", NULL);
	if (ret)
		pr_err("%s: random init irq failed!\n", __func__);

	/* disable irq first to void "Unbalanced enable" warning */
	disable_irq_nosync(virq);
	enable_irq(virq);
}
#endif

#ifdef CONFIG_CPU_FREQ_GOV_SCHEDUTIL
#if (LINUX_VERSION_CODE < KERNEL_VERSION(5, 15, 41))
static void sunxi_map_util_freq(void *p, unsigned long util, unsigned long freq,
	unsigned long cap, unsigned long *next_freq)
{
	struct cpufreq_policy *policy = NULL;
	unsigned int cur_freq = 0;
	unsigned long target_freq = 0;

	policy = cpufreq_cpu_get(0);
	if (!policy) {
		pr_err("%s: cpufreq cpu get failed\n", __func__);
		return;
	}
	cpufreq_cpu_put(policy);
	cur_freq = policy->cur;

	target_freq = (freq + (freq >> 2)) * util / cap;
	if (target_freq > (cur_freq * 2))
		target_freq = cur_freq * 2;

	*next_freq = target_freq;
}
#else /* LINUX_VERSION_CODE < KERNEL_VERSION(5, 15, 41) */
static void sunxi_map_util_freq_new(void *p, unsigned long util, unsigned long freq,
	unsigned long cap, unsigned long *next_freq, struct cpufreq_policy *policy,
	bool *need_freq_update)
{
	unsigned int cur_freq = 0;
	unsigned long target_freq = 0;

	cur_freq = policy->cur;
	target_freq = (freq + (freq >> 2)) * util / cap;
	if (target_freq > (cur_freq * 2))
		target_freq = (unsigned long)cur_freq * 2;

	*next_freq = target_freq;
}
#endif /* LINUX_VERSION_CODE < KERNEL_VERSION(5, 15, 41) */
#endif /* CONFIG_CPU_FREQ_GOV_SCHEDUTIL */

static int __init sunxi_vendor_hooks_init(void)
{
	int ret = 0;

#ifdef CONFIG_AW_RANDOM_INIT
	/* speed up random init */
	sunxi_random_init();
#endif

	/* oom meminfo debug */
	ret = register_trace_android_vh_show_mem(sunxi_show_mem, NULL);
	if (ret)
		pr_err("%s: register show_mem failed!\n", __func__);

	/* proc meminfo debug */
	ret = register_trace_android_vh_meminfo_proc_show(sunxi_meminfo_proc_show, NULL);
	if (ret)
		pr_err("%s: register meminfo_proc_show failed!\n", __func__);

	/* dma-buf heap debug for alloc and release */
	dma_heap_total = 0;
	ret = register_trace_android_vh_dmabuf_heap_flags_validation(
		sunxi_dmabuf_heap_flags_validation, NULL);
	if (ret)
		pr_err("%s: register dmabuf_heap_flags_validation failed!\n", __func__);
	ret = register_trace_android_vh_dma_buf_release(sunxi_dma_buf_release, NULL);
	if (ret)
		pr_err("%s: register dma_buf_release failed!\n", __func__);

	/* IO benchmark */
	ret = register_trace_android_vh_page_cache_forced_ra(
		sunxi_page_cache_forced_ra, NULL);
	if (ret)
		pr_err("%s: register sunxi_page_cache_forced_ra failed!\n", __func__);

	/* audio usb */
	ret = register_trace_android_vh_audio_usb_offload_ep_action(
		sunxi_audio_usb_set_urb_param, NULL);
	if (ret)
		pr_err("%s: register sunxi_audio_usb_set_urb_param failed!\n", __func__);

	ret = register_trace_android_vh_audio_usb_offload_connect(sunxi_audio_usb_connect, NULL);
	if (ret)
		pr_err("%s: register sunxi_audio_usb_connect failed!\n", __func__);

	ret = register_trace_android_rvh_audio_usb_offload_disconnect(sunxi_audio_usb_disconnect, NULL);
	if (ret)
		pr_err("%s: register sunxi_audio_usb_disconnect failed!\n", __func__);

	/* usb core */
	ret = register_trace_android_vh_usb_new_device_added(sunxi_usb_new_device_added, NULL);
	if (ret)
		pr_err("%s: register sunxi_usb_new_device_added failed!\n", __func__);

#ifdef CONFIG_CPU_FREQ_GOV_SCHEDUTIL
#if (LINUX_VERSION_CODE < KERNEL_VERSION(5, 15, 41))
	ret = register_trace_android_vh_map_util_freq(sunxi_map_util_freq, NULL);
	if (ret)
		pr_err("%s: register map_util_freq failed!\n", __func__);
#else
	ret = register_trace_android_vh_map_util_freq_new(sunxi_map_util_freq_new, NULL);
	if (ret)
		pr_err("%s: register map_util_freq_new failed!\n", __func__);
#endif
#endif /* #ifdef CONFIG_CPU_FREQ_GOV_SCHEDUTIL */

	return 0;
}

module_init(sunxi_vendor_hooks_init);
MODULE_DESCRIPTION("AW Vendor Hooks register");
MODULE_LICENSE("GPL v2");
MODULE_VERSION("1.0.1");
MODULE_AUTHOR("huangshuosheng<huangshuosheng@allwinnertech.com>");
