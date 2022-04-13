/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Allwinner Deinterlace driver
 *
 * Copyright (C) 2020 Jernej Skrabec <jernej.skrabec@siol.net>
 */

#ifndef _SUN8I_DEINTERLACE_H_
#define _SUN8I_DEINTERLACE_H_

#include <media/v4l2-device.h>
#include <media/v4l2-mem2mem.h>
#include <media/videobuf2-v4l2.h>
#include <media/videobuf2-dma-contig.h>

#include <linux/platform_device.h>

#define DEINTERLACE_NAME		"sun50i-di"

#define DEINTERLACE_CTRL			0x00
#define DEINTERLACE_CTRL_START				BIT(0)
#define DEINTERLACE_CTRL_IOMMU_EN			BIT(16)
#define DEINTERLACE_CTRL_RESET				BIT(31)

#define DEINTERLACE_INT_CTRL			0x04
#define DEINTERLACE_INT_EN				BIT(0)

#define DEINTERLACE_STATUS			0x08
#define DEINTERLACE_STATUS_FINISHED			BIT(0)
#define DEINTERLACE_STATUS_BUSY				BIT(8)

#define DEINTERLACE_SIZE			0x10
#define DEINTERLACE_SIZE_WIDTH(w) \
	(((w) - 1) & 0x7ff)
#define DEINTERLACE_SIZE_HEIGHT(h) \
	((((h) - 1) & 0x7ff) << 16)

#define DEINTERLACE_FORMAT			0x14
#define DEINTERLACE_FORMAT_YUV420P			0
#define DEINTERLACE_FORMAT_YUV420SP			1
#define DEINTERLACE_FORMAT_YUV422P			2
#define DEINTERLACE_FORMAT_YUV422SP			3

#define DEINTERLACE_POLAR			0x18
#define DEINTERLACE_POLAR_FIELD(x)			((x) & 1)

/* all pitch registers accept 16-bit values */
#define DEINTERLACE_IN_PITCH0			0x20
#define DEINTERLACE_IN_PITCH1			0x24
#define DEINTERLACE_IN_PITCH2			0x28
#define DEINTERLACE_OUT_PITCH0			0x30
#define DEINTERLACE_OUT_PITCH1			0x34
#define DEINTERLACE_OUT_PITCH2			0x38
#define DEINTERLACE_FLAG_PITCH			0x40
#define DEINTERLACE_IN0_ADDR0			0x50
#define DEINTERLACE_IN0_ADDR1			0x54
#define DEINTERLACE_IN0_ADDR2			0x58
#define DEINTERLACE_IN0_ADDRH			0x5c
#define DEINTERLACE_IN1_ADDR0			0x60
#define DEINTERLACE_IN1_ADDR1			0x64
#define DEINTERLACE_IN1_ADDR2			0x68
#define DEINTERLACE_IN1_ADDRH			0x6c
#define DEINTERLACE_IN2_ADDR0			0x70
#define DEINTERLACE_IN2_ADDR1			0x74
#define DEINTERLACE_IN2_ADDR2			0x78
#define DEINTERLACE_IN2_ADDRH			0x7c
#define DEINTERLACE_IN3_ADDR0			0x80
#define DEINTERLACE_IN3_ADDR1			0x84
#define DEINTERLACE_IN3_ADDR2			0x88
#define DEINTERLACE_IN3_ADDRH			0x8c
#define DEINTERLACE_OUT_ADDR0			0x90
#define DEINTERLACE_OUT_ADDR1			0x94
#define DEINTERLACE_OUT_ADDR2			0x98
#define DEINTERLACE_OUT_ADDRH			0x9c
#define DEINTERLACE_IN_FLAG_ADDR		0xa0
#define DEINTERLACE_OUT_FLAG_ADDR		0xa4
#define DEINTERLACE_FLAG_ADDRH			0xa8

#define DEINTERLACE_ADDRH0(x)				((x) & 0xff)
#define DEINTERLACE_ADDRH1(x)				(((x) & 0xff) << 8)
#define DEINTERLACE_ADDRH2(x)				(((x) & 0xff) << 16)

#define DEINTERLACE_MODE			0xb0
#define DEINTERLACE_MODE_DEINT_LUMA			BIT(0)
#define DEINTERLACE_MODE_MOTION_EN			BIT(4)
#define DEINTERLACE_MODE_INTP_EN			BIT(5)
#define DEINTERLACE_MODE_AUTO_UPD_MODE(x)		(((x) & 3) << 12)
#define DEINTERLACE_MODE_DEINT_CHROMA			BIT(16)
#define DEINTERLACE_MODE_FIELD_MODE			BIT(31)

#define DEINTERLACE_MD_PARAM0			0xb4
#define DEINTERLACE_MD_PARAM0_MIN_LUMA_TH(x)		((x) & 0xff)
#define DEINTERLACE_MD_PARAM0_MAX_LUMA_TH(x)		(((x) & 0xff) << 8)
#define DEINTERLACE_MD_PARAM0_AVG_LUMA_SHIFT(x)		(((x) & 0xf) << 16)
#define DEINTERLACE_MD_PARAM0_TH_SHIFT(x)		(((x) & 0xf) << 24)

#define DEINTERLACE_MD_PARAM1			0xb8
#define DEINTERLACE_MD_PARAM1_MOV_FAC_NONEDGE(x)	(((x) & 0x3) << 28)

#define DEINTERLACE_MD_PARAM2			0xbc
#define DEINTERLACE_MD_PARAM2_CHROMA_SPATIAL_TH(x)	(((x) & 0xff) << 8)
#define DEINTERLACE_MD_PARAM2_CHROMA_DIFF_TH(x)		(((x) & 0xff) << 16)
#define DEINTERLACE_MD_PARAM2_PIX_STATIC_TH(x)		(((x) & 0x3) << 28)

#define DEINTERLACE_INTP_PARAM0			0xc0
#define DEINTERLACE_INTP_PARAM0_ANGLE_LIMIT(x)		((x) & 0x1f)
#define DEINTERLACE_INTP_PARAM0_ANGLE_CONST_TH(x)	(((x) & 7) << 8)
#define DEINTERLACE_INTP_PARAM0_LUMA_CUR_FAC_MODE(x)	(((x) & 7) << 16)
#define DEINTERLACE_INTP_PARAM0_LUMA_CUR_FAC_MODE_MSK	(7 << 16)
#define DEINTERLACE_INTP_PARAM0_CHROMA_CUR_FAC_MODE(x)	(((x) & 7) << 20)
#define DEINTERLACE_INTP_PARAM0_CHROMA_CUR_FAC_MODE_MSK	(7 << 20)

#define DEINTERLACE_MD_CH_PARAM			0xc4
#define DEINTERLACE_MD_CH_PARAM_BLEND_MODE(x)		((x) & 0xf)
#define DEINTERLACE_MD_CH_PARAM_FONT_PRO_EN		BIT(8)
#define DEINTERLACE_MD_CH_PARAM_FONT_PRO_TH(x)		(((x) & 0xff) << 16)
#define DEINTERLACE_MD_CH_PARAM_FONT_PRO_FAC(x)		(((x) & 0x1f) << 24)

#define DEINTERLACE_INTP_PARAM1			0xc8
#define DEINTERLACE_INTP_PARAM1_A(x)			((x) & 7)
#define DEINTERLACE_INTP_PARAM1_EN			BIT(3)
#define DEINTERLACE_INTP_PARAM1_C(x)			(((x) & 0xf) << 4)
#define DEINTERLACE_INTP_PARAM1_CMAX(x)			(((x) & 0xff) << 8)
#define DEINTERLACE_INTP_PARAM1_MAXRAT(x)		(((x) & 3) << 16)

#define DEINTERLACE_OUT_PATH			0x200

#define DEINTERLACE_MIN_WIDTH	2U
#define DEINTERLACE_MIN_HEIGHT	2U
#define DEINTERLACE_MAX_WIDTH	2048U
#define DEINTERLACE_MAX_HEIGHT	1100U

struct deinterlace_ctx {
	struct v4l2_fh		fh;
	struct deinterlace_dev	*dev;

	struct v4l2_pix_format	src_fmt;
	struct v4l2_pix_format	dst_fmt;

	void			*flag1_buf;
	dma_addr_t		flag1_buf_dma;

	void			*flag2_buf;
	dma_addr_t		flag2_buf_dma;

	struct vb2_v4l2_buffer	*prev[2];

	unsigned int		first_field;
	unsigned int		field;

	int			aborting;
};

struct deinterlace_dev {
	struct v4l2_device	v4l2_dev;
	struct video_device	vfd;
	struct device		*dev;
	struct v4l2_m2m_dev	*m2m_dev;

	/* Device file mutex */
	struct mutex		dev_mutex;

	void __iomem		*base;

	struct clk		*bus_clk;
	struct clk		*mod_clk;
	struct clk		*ram_clk;

	struct reset_control	*rstc;
};

#endif
