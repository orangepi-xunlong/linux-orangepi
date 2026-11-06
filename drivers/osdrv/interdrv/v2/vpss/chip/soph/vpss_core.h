#ifndef _VPSS_CORE_H_
#define _VPSS_CORE_H_

#include <linux/kernel.h>
#include <linux/interrupt.h>
#include <linux/version.h>
#include <linux/pm_qos.h>
#include <linux/miscdevice.h>

#include <linux/iommu.h>
#include "base_ctx.h"
#include <linux/defines.h>
#include <linux/vpss_uapi.h>

#include "scaler.h"
#include "vpss_common.h"

#include <vpss_cb.h>

#define DEVICE_FROM_DTS 1

#define VIP_MAX_PLANES 3

#define INTR_IMG_IN (1 << 0)
#define INTR_SC     (1 << 1)


#if 0
#define VIP_CLK_RATIO_MASK(CLK_NAME) VIP_SYS_REG_CK_COEF_##CLK_NAME##_MASK
#define VIP_CLK_RATIO_OFFSET(CLK_NAME) VIP_SYS_REG_CK_COEF_##CLK_NAME##_OFFSET
#define VIP_CLK_RATIO_CONFIG(CLK_NAME, RATIO) \
	vip_sys_reg_write_mask(VIP_SYS_REG_CK_COEF_##CLK_NAME, \
		VIP_CLK_RATIO_MASK(CLK_NAME), \
		RATIO << VIP_CLK_RATIO_OFFSET(CLK_NAME))
#endif

enum vpss_dev_type {
	VPSS_DEV_V0 = 0,
	VPSS_DEV_V1,
	VPSS_DEV_V2,
	VPSS_DEV_V3,
	VPSS_DEV_T0,
	VPSS_DEV_T1,
	VPSS_DEV_T2,
	VPSS_DEV_T3,
	VPSS_DEV_D0,
	VPSS_DEV_D1,
	VPSS_DEV_MAX,
};

enum vip_state {
	VIP_IDLE,
	VIP_RUNNING,
	VIP_END,
	VIP_ONLINE,
};

struct vpss_core {
	enum vpss_dev_type vpss_type;
	unsigned int irq_num;
	atomic_t state;
	spinlock_t vpss_lock;
	struct clk *clk_src;
	struct clk *clk_apb;
	struct clk *clk_vpss;
	struct timespec64 ts_start;
	struct timespec64 ts_end;
	u32 hw_duration;
	u32 hw_duration_total;
	u32 duty_ratio;
	u32 duty_ratio_long;
	u8 intr_status;
	u8 tile_mode;
	u8 map_chn;
	u8 is_online;
	u32 checksum;
	u32 start_cnt;
	u32 int_cnt;
	void *job;
};

struct vpss_device {
	struct miscdevice miscdev;
	spinlock_t lock;
	struct vpss_core vpss_cores[VPSS_MAX];
};

unsigned long vpss_dmabuf_fd_to_paddr(int dmabuf_fd);

#endif /* _VPSS_CORE_H_ */
