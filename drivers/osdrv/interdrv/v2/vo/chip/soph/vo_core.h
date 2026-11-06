/* SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef __VO_CORE_H__
#define __VO_CORE_H__

#ifdef __cplusplus
	extern "C" {
#endif

#include <linux/cdev.h>
#include <linux/clk.h>
#include <linux/clk-provider.h>
#include <linux/err.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/iommu.h>
#include <linux/kernel.h>
#include <linux/kthread.h>
#include <linux/module.h>
#include <linux/of_platform.h>
#include <linux/platform_device.h>
#include <linux/pm_runtime.h>
#include <linux/pm_qos.h>
#include <linux/slab.h>
#include <linux/spinlock.h>
#include <linux/streamline_annotate.h>
#include <linux/version.h>
#include <linux/delay.h>
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 11, 0))
#include <uapi/linux/sched/types.h>
#endif

#include <vo_common.h>
#include <vo_defines.h>
#include <vo_interfaces.h>

#ifdef __cplusplus
}
#endif

#endif /* __VO_CORE_H__ */
