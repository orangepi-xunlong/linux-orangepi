// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2023 Ky Co., Ltd.
 *
 */

#ifndef _KY_DPU_H_
#define _KY_DPU_H_

#include <linux/delay.h>
#include <linux/string.h>
#include <linux/platform_device.h>
#include <linux/device.h>
#include <linux/kernel.h>
#include <linux/bug.h>
#include <video/videomode.h>
#include <linux/workqueue.h>
#include <linux/reset.h>
#include <linux/gpio/consumer.h>
#include <linux/gpio/driver.h>
#include <drm/drm_atomic_uapi.h>
#include <drm/drm_print.h>
#include <drm/drm_crtc.h>
#include <drm/drm_fourcc.h>
#include <drm/drm_vblank.h>
#include <drm/drm_writeback.h>
#include <dt-bindings/display/ky-dpu.h>
#include "dpu/saturn_regs/reg_map.h"
#include "ky_drm.h"
#include "ky_lib.h"

#define DPU_INT_UPDATE_DONE	BIT(0)
#define DPU_INT_UNDERRUN	BIT(2)
#define DPU_INT_OVERRUN		BIT(3)
#define DPU_INT_WBDONE		BIT(4)

#define N_SCALER_MAX			5
#define N_DMA_CHANNEL_MAX		12
#define N_DMA_LAYER_MAX			16
#define N_COMPOSER_MAX			4
#define N_COMPOSER_LAYER_MAX		16
#define N_PANEL_MAX			3
#define N_OUTCTRL_MAX			3
#define N_WRITEBACK_MAX			2
#define N_DISPLAY_MAX			5
#define N_LUT3D_MAX			3
#define N_CMDLIST_MAX			14

#define MHZ2HZ	1000000ULL
#define MHZ2KHZ	1000ULL
#define DPU_QOS_REQ			1100000
#define DPU_MIN_QOS_REQ			331070976
#define DPU_MARGIN_QOS_REQ		900000ULL
#define DPU_MAX_QOS_REQ			(3900000ULL - DPU_MARGIN_QOS_REQ)
#define RDMA_INVALID_ID			(~0)
#define SCALER_INVALID_ID		(u8)(~0)

#define DPU_STOP_TIMEOUT		(2000)
#define DPU_CTRL_MAX_TIMING_INTER1	(0xf)

/*
#define DPU_PXCLK_DEFAULT	98000000
#define DPU_MCLK_DEFAULT	307000000
#define DPU_MCLK_MAX		614400000
#define DPU_MCLK_MIN		40960000
#define DPU_AXICLK_DEFAULT	409000000
#define DPU_ESCCLK_DEFAULT	52000000
#define DPU_BITCLK_DEFAULT	624000000
*/

#define DPU_PXCLK_DEFAULT	88000000
// #define DPU_MCLK_DEFAULT	307200000
#define DPU_MCLK_DEFAULT	491520000
#define DPU_MCLK_MAX		614400000
#define DPU_MCLK_MIN		40960000
#define DPU_AXICLK_DEFAULT	409000000
#define DPU_ESCCLK_DEFAULT	51200000
#define DPU_BITCLK_DEFAULT	614400000

#define MAX_SCALER_NUMS		1

struct ky_dpu_scaler {
	u32 rdma_id;
	u32 in_use;
};

enum rdma_mode {
	UP_DOWN = BIT(0),
	LEFT_RIGHT = BIT(1),
};

struct ky_dpu_fbcmem {
	u32 start;
	u32 size;
	bool map;
};

struct ky_dpu_rdma {
	enum rdma_mode mode;
	struct ky_dpu_fbcmem fbcmem;
	bool in_use;
};

struct dpu_mmu_tbl {
	u32 size;
	void *va;
	dma_addr_t pa;
};

struct dpu_clk_context {
	struct clk *pxclk;
	struct clk *mclk;
	struct clk *hclk;
	struct clk *escclk;
	struct clk *bitclk;
	struct clk *hmclk;
};

struct ky_dpu {
	struct device *dev;
	struct drm_crtc crtc;
	struct drm_writeback_connector wb_connector;
	struct dpu_core_ops *core;
	struct workqueue_struct *dpu_underrun_wq;
	struct work_struct work_stop_trace;
	struct work_struct work_update_clk;
	struct work_struct work_update_bw;
	struct dpu_mmu_tbl mmu_tbl;
	int dev_id;
	int type;
	bool enable_dump_reg;
	bool enable_dump_fps;
	bool enable_auto_fc;
	struct timespec64 last_tm;

	bool is_1st_f;
	bool logo_booton;
	struct dpu_clk_context clk_ctx;
	uint64_t new_mclk;		/* new frame mclk */
	uint64_t cur_mclk;		/* current frame mclk */
	unsigned int min_mclk;		/* min_mclk of board panel resolution */
	uint64_t new_bw;
	uint64_t cur_bw;
	struct drm_property *color_matrix_property;
	uint32_t bitclk;
	uint32_t escclk;
	struct reset_control *dsi_reset;
	struct reset_control *mclk_reset;
	struct reset_control *lcd_reset;
	struct reset_control *esc_reset;
	struct reset_control *hdmi_reset;
	struct gpio_desc *enable_gpio;

#ifdef CONFIG_KY_DEBUG
	bool (*is_dpu_running)(struct ky_dpu* dpu);
	struct notifier_block nb;
	bool is_working;
#endif
};

extern struct list_head dpu_core_head;

static inline struct ky_dpu *crtc_to_dpu(struct drm_crtc *crtc)
{
	return crtc ? container_of(crtc, struct ky_dpu, crtc) : NULL;
}

struct dpu_core_ops {
	int (*parse_dt)(struct ky_dpu *dpu, struct device_node *np);
	u32 (*version)(struct ky_dpu *dpu);
	int (*init)(struct ky_dpu *dpu);
	void (*uninit)(struct ky_dpu *dpu);
	void (*run)(struct drm_crtc *crtc,
		    struct drm_crtc_state *old_state);
	void (*stop)(struct ky_dpu *dpu);
	void (*flip)(struct ky_dpu *dpu);
	void (*disable_vsync)(struct ky_dpu *dpu);
	void (*enable_vsync)(struct ky_dpu *dpu);
	u32 (*isr)(struct ky_dpu *dpu);
	int (*modeset)(struct ky_dpu *dpu, struct drm_mode_modeinfo *mode);
	int (*enable_clk)(struct ky_dpu *dpu);
	int (*disable_clk)(struct ky_dpu *dpu);
	int (*cal_layer_fbcmem_size)(struct drm_plane *plane, \
				     struct drm_plane_state *state);
	int (*adjust_rdma_fbcmem)(struct ky_hw_device *hwdev, \
				 struct ky_dpu_rdma *rdmas);
	int (*calc_plane_mclk_bw)(struct drm_plane *plane, \
			struct drm_plane_state *state);
	void (*wb_config)(struct ky_dpu *dpu);
	int (*update_clk)(struct ky_dpu *dpu, uint64_t mclk);
	int (*update_bw)(struct ky_dpu *dpu, uint64_t bw);
};

#define dpu_core_ops_register(entry) \
	disp_ops_register(entry, &dpu_core_head)
#define dpu_core_ops_attach(str) \
	disp_ops_attach(str, &dpu_core_head)

int ky_dpu_run(struct drm_crtc *crtc,
		struct drm_crtc_state *old_state);
int ky_dpu_stop(struct ky_dpu *dpu);
int ky_dpu_wb_config(struct ky_dpu *dpu);

struct ky_plane {
	struct drm_plane plane;
	struct ky_hw_device *hwdev;
	struct drm_property *rdma_id_property;
	struct drm_property *solid_color_property;
	struct drm_property *hdr_coef_property;
	struct drm_property *scale_coef_property;
	u32 hw_pid;
};

struct ky_plane_state {
	struct drm_plane_state state;
	u32 rdma_id;
	u32 solid_color;
	/*
	 * scaler id. As long as its rdma channel is identical with any
	 * one uses the scaler, it's set to the scaler's id, even if it
	 * doesn't really use the scaler.
	 */
	u8 scaler_id;
	/* hw format */
	u8 format;
	bool use_scl;
	bool is_offline; //to indicate rdma is offline
	bool right_image;
	u32 fbcmem_size;
	uint64_t mclk;	//DPU MCLK = MAX(Mclk, Aclk) of all planes
	uint64_t bw;	//BandWidth = SUM(BW_single) * 1.08
	bool is_crop;
	u32 src_crop_x;
	u32 src_crop_y;
	u32 src_crop_w;
	u32 src_crop_h;
	u32 dst_crop_x;
	u32 dst_crop_y;
	u32 dst_crop_w;
	u32 dst_crop_h;
	u32 screen_width;
	u32 screen_height;
	struct dpu_mmu_tbl mmu_tbl;
	struct cmdlist cl;
	uint64_t afbc_effc;
	struct drm_property_blob *hdr_coefs_blob_prop;
	struct drm_property_blob *scale_coefs_blob_prop;
};

struct ky_plane *to_ky_plane(struct drm_plane *plane);

static inline struct
ky_plane_state *to_ky_plane_state(const struct drm_plane_state *state)
{
	return container_of(state, struct ky_plane_state, state);
}

struct ky_crtc_state {
	struct drm_crtc_state base;
	struct ky_dpu_scaler scalers[MAX_SCALER_NUMS];
	struct ky_dpu_rdma *rdmas;
	uint64_t mclk;	/* max of all rdma mclk */
	uint64_t bw;	/* sum of all rdma bw*/
	uint64_t aclk;	/* based on bw */
	uint64_t real_mclk; /* real_mclk = max(mclk, aclk) */
	struct drm_property_blob *color_matrix_blob_prop;
};

#define to_ky_crtc_state(x) container_of(x, struct ky_crtc_state, base)

struct drm_plane *ky_plane_init(struct drm_device *drm,
					struct ky_dpu *dpu);
#endif

