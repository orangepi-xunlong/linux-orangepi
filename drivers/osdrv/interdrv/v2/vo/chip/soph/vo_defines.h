#ifndef __VO_DEFINES_H__
#define __VO_DEFINES_H__

#ifdef __cplusplus
	extern "C" {
#endif

#include <linux/cdev.h>
#include <linux/interrupt.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>

#include <linux/vo_uapi.h>
#include <vo_disp.h>

struct vo_core {
	u8 core_id;
	int irq_num;
	struct vo_dv_timings dv_timings;
	struct vo_rect sink_rect;
	struct vo_rect compose_out;
	struct vo_rect crop_rect;
	enum vo_disp_intf disp_interface;
	bool bgcolor_enable;
	u8 align;
	bool disp_online;
	u32 frame_number;
	atomic_t disp_streamon;
};

struct vo_core_dev {
	// private data
	struct device *dev;
	struct class *vo_class;
	struct cdev cdev;
	dev_t cdev_id;
	struct clk *clk_vo[6];
	struct clk *clk_lvds[2];
	//vo_mac, disp, dsi_mac, dsi_phy, oenc
	void __iomem *reg_base[10];
	struct vo_core vo_core[VO_MAX_DEV_NUM];
};

#ifdef __cplusplus
}
#endif

#endif /* __VO_DEFINES_H__ */
