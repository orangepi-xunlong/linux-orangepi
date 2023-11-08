/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Rockchip Image Enhancement Processor (IEP) driver
 *
 * Copyright (C) 2020 Alex Bee <knaerzche@gmail.com>
 *
 */
#ifndef __IEP_H__
#define __IEP_H__

#include <linux/platform_device.h>
#include <media/videobuf2-v4l2.h>
#include <media/v4l2-ctrls.h>
#include <media/v4l2-device.h>

#define IEP_NAME "rockchip-iep"

/* Hardware limits */
#define IEP_MIN_WIDTH 320U
#define IEP_MAX_WIDTH 1920U

#define IEP_MIN_HEIGHT 240U
#define IEP_MAX_HEIGHT 1088U

/* Hardware defaults */
#define IEP_DEFAULT_WIDTH 320U
#define IEP_DEFAULT_HEIGHT 240U

//ns
#define IEP_TIMEOUT 250000

struct iep_fmt {
	u32 fourcc;
	u8 depth;
	u8 uv_factor;
	u8 color_swap;
	u8 hw_format;
};

struct iep_frm_fmt {
	struct iep_fmt *hw_fmt;
	struct v4l2_pix_format pix;

	unsigned int y_stride;
	unsigned int uv_stride;
};

struct iep_ctx {
	struct v4l2_fh fh;
	struct rockchip_iep *iep;

	struct iep_frm_fmt src_fmt;
	struct iep_frm_fmt dst_fmt;

	struct vb2_v4l2_buffer	*prev_src_buf;
	struct vb2_v4l2_buffer	*dst0_buf;
	struct vb2_v4l2_buffer	*dst1_buf;

	u32 dst_sequence;
	u32 src_sequence;

	/* bff = bottom field first */
	bool field_order_bff;
	bool field_bff;

	unsigned int dst_buffs_done;

	bool fmt_changed;
	bool job_abort;
};

struct rockchip_iep {
	struct v4l2_device v4l2_dev;
	struct v4l2_m2m_dev *m2m_dev;
	struct video_device vfd;

	struct device *dev;

	void __iomem *regs;

	struct clk *axi_clk;
	struct clk *ahb_clk;

	/* vfd lock */
	struct mutex mutex;
};

static inline void iep_write(struct rockchip_iep *iep, u32 reg, u32 value)
{
	writel(value, iep->regs + reg);
};

static inline u32 iep_read(struct rockchip_iep *iep, u32 reg)
{
	return readl(iep->regs + reg);
};

static inline void iep_shadow_mod(struct rockchip_iep *iep, u32 reg,
				  u32 shadow_reg, u32 mask, u32 val)
{
	u32 temp = iep_read(iep, shadow_reg) & ~(mask);

	temp |= val & mask;
	iep_write(iep, reg, temp);
};

static inline void iep_mod(struct rockchip_iep *iep, u32 reg, u32 mask, u32 val)
{
	iep_shadow_mod(iep, reg, reg, mask, val);
};

#endif
