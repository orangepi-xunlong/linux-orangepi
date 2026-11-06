#ifndef __RGN_CORE_H__
#define __RGN_CORE_H__

#ifdef __cplusplus
	extern "C" {
#endif

#include <linux/miscdevice.h>
#include <linux/clk.h>
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
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 11, 0))
#include <uapi/linux/sched/types.h>
#endif

#include <rgn_common.h>
#include <rgn_defines.h>
#include <rgn_interfaces.h>

#ifdef __cplusplus
}
#endif

#endif /* __RGN_CORE_H__ */
