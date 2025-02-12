// SPDX-License-Identifier: GPL-2.0
/*
 * vdev.c - video divece functions
 *
 * Copyright(C) 2023 KY Micro Limited
 */
//#include <soc/spm/plat.h>
#include <media/v4l2-dev.h>
#include <media/media-entity.h>
#include <media/media-device.h>
#include <media/v4l2-subdev.h>
#include <media/videobuf2-v4l2.h>
#include <media/v4l2-ioctl.h>
#include <linux/media-bus-format.h>
#include <linux/compat.h>
#include <media/x1/x1_media_bus_format.h>
#define CAM_MODULE_TAG CAM_MDL_VI
#include <cam_dbg.h>
#include "vdev.h"
#include "mlink.h"
#include "ky_videobuf2.h"
#include "subdev.h"

#define SAINT_CHECK()	({ \
		int r = 0; \
		do { \
			if (NULL == remote_pad) { \
				cam_err("%s(%s) no remote entity linked with this devnode. ", __func__, sc_vnode->name); \
				r = -ENODEV; \
				break; \
			} \
			remote_me = remote_pad->entity; \
			if (NULL == remote_me) { \
				cam_err("%s(%s) remote_pad did not have entity associated with it. ", __func__, sc_vnode->name); \
				BUG_ON(1); \
			} \
			if (!is_subdev(remote_me)) { \
				cam_err("%s remote entity must be a v4l2 subdevice! ", __func__); \
				r = -1; \
				break; \
			} \
			remote_sd = container_of(remote_me, struct v4l2_subdev, entity); \
		} while (0); \
		r; \
	})

static struct {
	__u32 pixelformat;
	__u32 mbus_fmtcode;
	__u8 num_planes;
	__u32 pixel_width_align;
	__u32 pixel_height_align;
	__u32 plane_bytes_align[VIDEO_MAX_PLANES];
	struct {
		__u32 num;
		__u32 den;
	} plane_bpp[VIDEO_MAX_PLANES];
	struct {
		__u32 num;
		__u32 den;
	} height_subsampling[VIDEO_MAX_PLANES];
} spm_camera_formats_table[] = {
	/* bayer raw8 */
	{
		.pixelformat = V4L2_PIX_FMT_KYGB8P,
		.mbus_fmtcode = MEDIA_BUS_FMT_SRGB8_KYPACK_1X8,
		.num_planes = 1,
		.pixel_width_align = 16,
		.plane_bytes_align = {
			[0] = 16,
		},
		.plane_bpp = {
			[0] = {
				.num = 8,
				.den = 1,
			},
		},
		.height_subsampling = {
			[0] = {
				.num = 1,
				.den = 1,
			},
		},
	},
	{
		.pixelformat = V4L2_PIX_FMT_SBGGR8,
		.mbus_fmtcode = MEDIA_BUS_FMT_SBGGR8_1X8,
		.num_planes = 1,
		.pixel_width_align = 16,
		.plane_bytes_align = {
			[0] = 16,
		},
		.plane_bpp = {
			[0] = {
				.num = 8,
				.den = 1,
			},
		},
		.height_subsampling = {
			[0] = {
				.num = 1,
				.den = 1,
			},
		},
	},
	{
		.pixelformat = V4L2_PIX_FMT_SGBRG8,
		.mbus_fmtcode = MEDIA_BUS_FMT_SGBRG8_1X8,
		.num_planes = 1,
		.pixel_width_align = 16,
		.plane_bytes_align = {
			[0] = 16,
		},
		.plane_bpp = {
			[0] = {
				.num = 8,
				.den = 1,
			},
		},
		.height_subsampling = {
			[0] = {
				.num = 1,
				.den = 1,
			},
		},
	},
	{
		.pixelformat = V4L2_PIX_FMT_SGRBG8,
		.mbus_fmtcode = MEDIA_BUS_FMT_SGRBG8_1X8,
		.num_planes = 1,
		.pixel_width_align = 16,
		.plane_bytes_align = {
			[0] = 16,
		},
		.plane_bpp = {
			[0] = {
				.num = 8,
				.den = 1,
			},
		},
		.height_subsampling = {
			[0] = {
				.num = 1,
				.den = 1,
			},
		},
	},
	{
		.pixelformat = V4L2_PIX_FMT_SRGGB8,
		.mbus_fmtcode = MEDIA_BUS_FMT_SRGGB8_1X8,
		.num_planes = 1,
		.pixel_width_align = 16,
		.plane_bytes_align = {
			[0] = 16,
		},
		.plane_bpp = {
			[0] = {
				.num = 8,
				.den = 1,
			},
		},
		.height_subsampling = {
			[0] = {
				.num = 1,
				.den = 1,
			},
		},
	},
	/* bayer raw10 */
	{
		.pixelformat = V4L2_PIX_FMT_KYGB10P,
		.mbus_fmtcode = MEDIA_BUS_FMT_SRGB10_KYPACK_1X10,
		.num_planes = 1,
		.pixel_width_align = 12,
		.plane_bytes_align = {
			[0] = 16,
		},
		.plane_bpp = {
			[0] = {
				.num = 128,
				.den = 12,
			},
		},
		.height_subsampling = {
			[0] = {
				.num = 1,
				.den = 1,
			},
		},
	},
	{
		.pixelformat = V4L2_PIX_FMT_SBGGR10P,
		.mbus_fmtcode = MEDIA_BUS_FMT_SBGGR10_1X10,
		.num_planes = 1,
		.pixel_width_align = 12,
		.plane_bytes_align = {
			[0] = 16,
		},
		.plane_bpp = {
			[0] = {
				.num = 128,
				.den = 12,
			},
		},
		.height_subsampling = {
			[0] = {
				.num = 1,
				.den = 1,
			},
		},
	},
	{
		.pixelformat = V4L2_PIX_FMT_SGBRG10P,
		.mbus_fmtcode = MEDIA_BUS_FMT_SGBRG10_1X10,
		.num_planes = 1,
		.pixel_width_align = 12,
		.plane_bytes_align = {
			[0] = 16,
		},
		.plane_bpp = {
			[0] = {
				.num = 128,
				.den = 12,
			},
		},
		.height_subsampling = {
			[0] = {
				.num = 1,
				.den = 1,
			},
		},
	},
	{
		.pixelformat = V4L2_PIX_FMT_SGRBG10P,
		.mbus_fmtcode = MEDIA_BUS_FMT_SGRBG10_1X10,
		.num_planes = 1,
		.pixel_width_align = 12,
		.plane_bytes_align = {
			[0] = 16,
		},
		.plane_bpp = {
			[0] = {
				.num = 128,
				.den = 12,
			},
		},
		.height_subsampling = {
			[0] = {
				.num = 1,
				.den = 1,
			},
		},
	},
	{
		.pixelformat = V4L2_PIX_FMT_SRGGB10P,
		.mbus_fmtcode = MEDIA_BUS_FMT_SRGGB10_1X10,
		.num_planes = 1,
		.pixel_width_align = 12,
		.plane_bytes_align = {
			[0] = 16,
		},
		.plane_bpp = {
			[0] = {
				.num = 128,
				.den = 12,
			},
		},
		.height_subsampling = {
			[0] = {
				.num = 1,
				.den = 1,
			},
		},
	},
	/* bayer raw12 */
	{
		.pixelformat = V4L2_PIX_FMT_KYGB12P,
		.mbus_fmtcode = MEDIA_BUS_FMT_SRGB12_KYPACK_1X12,
		.num_planes = 1,
		.pixel_width_align = 10,
		.plane_bytes_align = {
			[0] = 16,
		},
		.plane_bpp = {
			[0] = {
				.num = 128,
				.den = 10,
			},
		},
		.height_subsampling = {
			[0] = {
				.num = 1,
				.den = 1,
			},
		},
	},
	{
		.pixelformat = V4L2_PIX_FMT_SBGGR12P,
		.mbus_fmtcode = MEDIA_BUS_FMT_SBGGR12_1X12,
		.num_planes = 1,
		.pixel_width_align = 10,
		.plane_bytes_align = {
			[0] = 16,
		},
		.plane_bpp = {
			[0] = {
				.num = 128,
				.den = 10,
			},
		},
		.height_subsampling = {
			[0] = {
				.num = 1,
				.den = 1,
			},
		},
	},
	{
		.pixelformat = V4L2_PIX_FMT_SGBRG12P,
		.mbus_fmtcode = MEDIA_BUS_FMT_SGBRG12_1X12,
		.num_planes = 1,
		.pixel_width_align = 10,
		.plane_bytes_align = {
			[0] = 16,
		},
		.plane_bpp = {
			[0] = {
				.num = 128,
				.den = 10,
			},
		},
		.height_subsampling = {
			[0] = {
				.num = 1,
				.den = 1,
			},
		},
	},
	{
		.pixelformat = V4L2_PIX_FMT_SGRBG12P,
		.mbus_fmtcode = MEDIA_BUS_FMT_SGRBG12_1X12,
		.num_planes = 1,
		.pixel_width_align = 10,
		.plane_bytes_align = {
			[0] = 16,
		},
		.plane_bpp = {
			[0] = {
				.num = 128,
				.den = 10,
			},
		},
		.height_subsampling = {
			[0] = {
				.num = 1,
				.den = 1,
			},
		},
	},
	{
		.pixelformat = V4L2_PIX_FMT_SRGGB12P,
		.mbus_fmtcode = MEDIA_BUS_FMT_SRGGB12_1X12,
		.num_planes = 1,
		.pixel_width_align = 10,
		.plane_bytes_align = {
			[0] = 16,
		},
		.plane_bpp = {
			[0] = {
				.num = 128,
				.den = 10,
			},
		},
		.height_subsampling = {
			[0] = {
				.num = 1,
				.den = 1,
			},
		},
	},
	/* bayer raw14 */
	{
		.pixelformat = V4L2_PIX_FMT_KYGB14P,
		.mbus_fmtcode = MEDIA_BUS_FMT_SRGB14_KYPACK_1X14,
		.num_planes = 1,
		.pixel_width_align = 8,
		.plane_bytes_align = {
			[0] = 16,
		},
		.plane_bpp = {
			[0] = {
				.num = 128,
				.den = 8,
			},
		},
		.height_subsampling = {
			[0] = {
				.num = 1,
				.den = 1,
			},
		},
	},
	{
		.pixelformat = V4L2_PIX_FMT_SBGGR14P,
		.mbus_fmtcode = MEDIA_BUS_FMT_SBGGR14_1X14,
		.num_planes = 1,
		.pixel_width_align = 8,
		.plane_bytes_align = {
			[0] = 16,
		},
		.plane_bpp = {
			[0] = {
				.num = 128,
				.den = 8,
			},
		},
		.height_subsampling = {
			[0] = {
				.num = 1,
				.den = 1,
			},
		},
	},
	{
		.pixelformat = V4L2_PIX_FMT_SGBRG14P,
		.mbus_fmtcode = MEDIA_BUS_FMT_SGBRG14_1X14,
		.num_planes = 1,
		.pixel_width_align = 8,
		.plane_bytes_align = {
			[0] = 16,
		},
		.plane_bpp = {
			[0] = {
				.num = 128,
				.den = 8,
			},
		},
		.height_subsampling = {
			[0] = {
				.num = 1,
				.den = 1,
			},
		},
	},
	{
		.pixelformat = V4L2_PIX_FMT_SGRBG14P,
		.mbus_fmtcode = MEDIA_BUS_FMT_SGRBG14_1X14,
		.num_planes = 1,
		.pixel_width_align = 8,
		.plane_bytes_align = {
			[0] = 16,
		},
		.plane_bpp = {
			[0] = {
				.num = 128,
				.den = 8,
			},
		},
		.height_subsampling = {
			[0] = {
				.num = 1,
				.den = 1,
			},
		},
	},
	{
		.pixelformat = V4L2_PIX_FMT_SRGGB14P,
		.mbus_fmtcode = MEDIA_BUS_FMT_SRGGB14_1X14,
		.num_planes = 1,
		.pixel_width_align = 8,
		.plane_bytes_align = {
			[0] = 16,
		},
		.plane_bpp = {
			[0] = {
				.num = 128,
				.den = 8,
			},
		},
		.height_subsampling = {
			[0] = {
				.num = 1,
				.den = 1,
			},
		},
	},
	/* rgb */
	{
		.pixelformat = V4L2_PIX_FMT_RGB565,
		.mbus_fmtcode = MEDIA_BUS_FMT_RGB565_1X16,
		.num_planes = 1,
		.pixel_width_align = 2,
		.plane_bytes_align = {
			[0] = 1,
		},
		.plane_bpp = {
			[0] = {
				.num = 16,
				.den = 1,
			},
		},
		.height_subsampling = {
			[0] = {
				.num = 1,
				.den = 1,
			},
		},
	},
	{
		.pixelformat = V4L2_PIX_FMT_RGB24,
		.mbus_fmtcode = MEDIA_BUS_FMT_RGB888_1X24,
		.num_planes = 1,
		.pixel_width_align = 2,
		.plane_bytes_align = {
			[0] = 1,
		},
		.plane_bpp = {
			[0] = {
				.num = 24,
				.den = 1,
			},
		},
		.height_subsampling = {
			[0] = {
				.num = 1,
				.den = 1,
			},
		},
	},
	/* yuv */
	{
		.pixelformat = V4L2_PIX_FMT_NV12,
		.mbus_fmtcode = MEDIA_BUS_FMT_YUYV8_1_5X8,
		.num_planes = 2,
		.pixel_width_align = 2,
		.plane_bytes_align = {
			[0] = 1,
			[1] = 1,
		},
		.plane_bpp = {
			[0] = {
				.num = 8,
				.den = 1,
			},
			[1] = {
				.num = 8,
				.den = 1,
			},
		},
		.height_subsampling = {
			[0] = {
				.num = 1,
				.den = 1,
			},
			[1] = {
				.num = 1,
				.den = 2,
			},
		},
	},
	{
		.pixelformat = V4L2_PIX_FMT_NV12_AFBC,
		.mbus_fmtcode = MEDIA_BUS_FMT_YUYV8_1_5X8_AFBC,
		.num_planes = 2,
		.pixel_width_align = 32,
		.pixel_height_align = 4,
	},
	{
		.pixelformat = V4L2_PIX_FMT_NV21,
		.mbus_fmtcode = MEDIA_BUS_FMT_YVYU8_1_5X8,
		.num_planes = 2,
		.pixel_width_align = 2,
		.plane_bytes_align = {
			[0] = 1,
			[1] = 1,
		},
		.plane_bpp = {
			[0] = {
				.num = 8,
				.den = 1,
			},
			[1] = {
				.num = 8,
				.den = 1,
			},
		},
		.height_subsampling = {
			[0] = {
				.num = 1,
				.den = 1,
			},
			[1] = {
				.num = 1,
				.den = 2,
			},
		},
	},
	/* YUYV YUV422 */
	{
		.pixelformat = V4L2_PIX_FMT_YUYV,
		.mbus_fmtcode = MEDIA_BUS_FMT_YUYV8_1X16,
		.num_planes = 1,
		.pixel_width_align = 2,
		.plane_bytes_align = {
			[0] = 1,
		},
		.plane_bpp = {
			[0] = {
				.num = 16,
				.den = 1,
			},
		},
		.height_subsampling = {
			[0] = {
				.num = 1,
				.den = 1,
			},
		},
	},
	/* YVYU YUV422 */
	{
		.pixelformat = V4L2_PIX_FMT_YVYU,
		.mbus_fmtcode = MEDIA_BUS_FMT_YVYU8_1X16,
		.num_planes = 1,
		.pixel_width_align = 2,
		.plane_bytes_align = {
			[0] = 1,
		},
		.plane_bpp = {
			[0] = {
				.num = 16,
				.den = 1,
			},
		},
		.height_subsampling = {
			[0] = {
				.num = 1,
				.den = 1,
			},
		},
	},
	/* Y210 */
	{
		.pixelformat = V4L2_PIX_FMT_Y210,
		.mbus_fmtcode = MEDIA_BUS_FMT_YUYV10_1X20,
		.num_planes = 1,
		.pixel_width_align = 2,
		.plane_bytes_align = {
			[0] = 1,
		},
		.plane_bpp = {
			[0] = {
				.num = 64,
				.den = 2,
			},
		},
		.height_subsampling = {
			[0] = {
				.num = 1,
				.den = 1,
			},
		},
	},
	/* P210 */
	{
		.pixelformat = V4L2_PIX_FMT_P210,
		.mbus_fmtcode = MEDIA_BUS_FMT_YUYV10_2X10,
		.num_planes = 2,
		.pixel_width_align = 2,
		.plane_bytes_align = {
			[0] = 1,
			[1] = 1,
		},
		.plane_bpp = {
			[0] = {
				.num = 32,
				.den = 2,
			},
			[1] = {
				.num = 32,
				.den = 2,
			},
		},
		.height_subsampling = {
			[0] = {
				.num = 1,
				.den = 1,
			},
			[1] = {
				.num = 1,
				.den = 1,
			},
		},
	},
	/* P010 */
	{
		.pixelformat = V4L2_PIX_FMT_P010,
		.mbus_fmtcode = MEDIA_BUS_FMT_YUYV10_1_5X10,
		.num_planes = 2,
		.pixel_width_align = 2,
		.plane_bytes_align = {
			[0] = 1,
			[1] = 1,
		},
		.plane_bpp = {
			[0] = {
				.num = 32,
				.den = 2,
			},
			[1] = {
				.num = 32,
				.den = 2,
			},
		},
		.height_subsampling = {
			[0] = {
				.num = 1,
				.den = 1,
			},
			[1] = {
				.num = 1,
				.den = 2,
			},
		},
	},
	/* D010 layer1 */
	{
		.pixelformat = V4L2_PIX_FMT_D010_1,
		.mbus_fmtcode = MEDIA_BUS_FMT_YUYV10_1_5X10_D1,
		.num_planes = 2,
		.pixel_width_align = 32,
		.pixel_height_align = 16,
		.plane_bytes_align = {
			[0] = 1,
			[1] = 1,
		},
		.plane_bpp = {
			[0] = {
				.num = 10,
				.den = 1,
			},
			[1] = {
				.num = 10,
				.den = 1,
			},
		},
		.height_subsampling = {
			[0] = {
				.num = 1,
				.den = 1,
			},
			[1] = {
				.num = 1,
				.den = 2,
			},
		},
	},
	/* D010 layer2 */
	{
		.pixelformat = V4L2_PIX_FMT_D010_2,
		.mbus_fmtcode = MEDIA_BUS_FMT_YUYV10_1_5X10_D2,
		.num_planes = 2,
		.pixel_width_align = 16,
		.pixel_height_align = 8,
		.plane_bytes_align = {
			[0] = 1,
			[1] = 1,
		},
		.plane_bpp = {
			[0] = {
				.num = 10,
				.den = 1,
			},
			[1] = {
				.num = 10,
				.den = 1,
			},
		},
		.height_subsampling = {
			[0] = {
				.num = 1,
				.den = 1,
			},
			[1] = {
				.num = 1,
				.den = 2,
			},
		},
	},
	/* D010 layer3 */
	{
		.pixelformat = V4L2_PIX_FMT_D010_3,
		.mbus_fmtcode = MEDIA_BUS_FMT_YUYV10_1_5X10_D3,
		.num_planes = 2,
		.pixel_width_align = 8,
		.pixel_height_align = 4,
		.plane_bytes_align = {
			[0] = 1,
			[1] = 1,
		},
		.plane_bpp = {
			[0] = {
				.num = 10,
				.den = 1,
			},
			[1] = {
				.num = 10,
				.den = 1,
			},
		},
		.height_subsampling = {
			[0] = {
				.num = 1,
				.den = 1,
			},
			[1] = {
				.num = 1,
				.den = 2,
			},
		},
	},
	/* D010 layer4 */
	{
		.pixelformat = V4L2_PIX_FMT_D010_4,
		.mbus_fmtcode = MEDIA_BUS_FMT_YUYV10_1_5X10_D4,
		.num_planes = 2,
		.pixel_width_align = 4,
		.pixel_height_align = 2,
		.plane_bytes_align = {
			[0] = 1,
			[1] = 1,
		},
		.plane_bpp = {
			[0] = {
				.num = 10,
				.den = 1,
			},
			[1] = {
				.num = 10,
				.den = 1,
			},
		},
		.height_subsampling = {
			[0] = {
				.num = 1,
				.den = 1,
			},
			[1] = {
				.num = 1,
				.den = 2,
			},
		},
	},
	/* D210 layer1 */
	{
		.pixelformat = V4L2_PIX_FMT_D210_1,
		.mbus_fmtcode = MEDIA_BUS_FMT_YVYU10_1_5X10_D1,
		.num_planes = 2,
		.pixel_width_align = 32,
		.plane_bytes_align = {
			[0] = 1,
			[1] = 1,
		},
		.plane_bpp = {
			[0] = {
				.num = 10,
				.den = 1,
			},
			[1] = {
				.num = 10,
				.den = 1,
			},
		},
		.height_subsampling = {
			[0] = {
				.num = 1,
				.den = 1,
			},
			[1] = {
				.num = 1,
				.den = 2,
			},
		},
	},
	/* D210 layer2 */
	{
		.pixelformat = V4L2_PIX_FMT_D210_2,
		.mbus_fmtcode = MEDIA_BUS_FMT_YVYU10_1_5X10_D2,
		.num_planes = 2,
		.pixel_width_align = 16,
		.plane_bytes_align = {
			[0] = 1,
			[1] = 1,
		},
		.plane_bpp = {
			[0] = {
				.num = 10,
				.den = 1,
			},
			[1] = {
				.num = 10,
				.den = 1,
			},
		},
		.height_subsampling = {
			[0] = {
				.num = 1,
				.den = 1,
			},
			[1] = {
				.num = 1,
				.den = 2,
			},
		},
	},
	/* D210 layer3 */
	{
		.pixelformat = V4L2_PIX_FMT_D210_3,
		.mbus_fmtcode = MEDIA_BUS_FMT_YVYU10_1_5X10_D3,
		.num_planes = 2,
		.pixel_width_align = 8,
		.plane_bytes_align = {
			[0] = 1,
			[1] = 1,
		},
		.plane_bpp = {
			[0] = {
				.num = 10,
				.den = 1,
			},
			[1] = {
				.num = 10,
				.den = 1,
			},
		},
		.height_subsampling = {
			[0] = {
				.num = 1,
				.den = 1,
			},
			[1] = {
				.num = 1,
				.den = 2,
			},
		},
	},
	/* D210 layer4 */
	{
		.pixelformat = V4L2_PIX_FMT_D210_4,
		.mbus_fmtcode = MEDIA_BUS_FMT_YVYU10_1_5X10_D4,
		.num_planes = 2,
		.pixel_width_align = 4,
		.plane_bytes_align = {
			[0] = 1,
			[1] = 1,
		},
		.plane_bpp = {
			[0] = {
				.num = 10,
				.den = 1,
			},
			[1] = {
				.num = 10,
				.den = 1,
			},
		},
		.height_subsampling = {
			[0] = {
				.num = 1,
				.den = 1,
			},
			[1] = {
				.num = 1,
				.den = 2,
			},
		},
	},
};

static int spm_vdev_lookup_formats_table(struct v4l2_format *f)
{
	struct v4l2_pix_format_mplane *pix_fmt = &f->fmt.pix_mp;
	int loop = 0;

	for (loop = 0; loop < ARRAY_SIZE(spm_camera_formats_table); loop++) {
		if (spm_camera_formats_table[loop].pixelformat == pix_fmt->pixelformat)
			break;
	}
	if (loop >= ARRAY_SIZE(spm_camera_formats_table))
		return -1;

	return 0;
}

void spm_vdev_fill_subdev_format(struct v4l2_format *f, struct v4l2_subdev_format *sub_f)
{
	int loop = 0;

	for (loop = 0; loop < ARRAY_SIZE(spm_camera_formats_table); loop++) {
		if (f->fmt.pix_mp.pixelformat == spm_camera_formats_table[loop].pixelformat) {
			sub_f->format.code = spm_camera_formats_table[loop].mbus_fmtcode;
			sub_f->format.width = f->fmt.pix_mp.width;
			sub_f->format.height = f->fmt.pix_mp.height;
			sub_f->format.field = f->fmt.pix_mp.field;
			sub_f->format.colorspace = f->fmt.pix_mp.colorspace;
			sub_f->format.ycbcr_enc = f->fmt.pix_mp.ycbcr_enc;
			sub_f->format.quantization = f->fmt.pix_mp.quantization;
			sub_f->format.xfer_func = f->fmt.pix_mp.xfer_func;
			break;
		}
	}
}

void spm_vdev_fill_v4l2_format(struct v4l2_subdev_format *sub_f, struct v4l2_format *f)
{
	int loop = 0, plane = 0;
	unsigned int width = 0, height = 0, stride = 0;
	struct v4l2_plane_pix_format *plane_fmt = NULL;

	for (loop = 0; loop < ARRAY_SIZE(spm_camera_formats_table); loop++) {
		if (sub_f->format.code == spm_camera_formats_table[loop].mbus_fmtcode) {
			f->fmt.pix_mp.pixelformat = spm_camera_formats_table[loop].pixelformat;
			f->fmt.pix_mp.width = sub_f->format.width;
			f->fmt.pix_mp.height = height = sub_f->format.height;
			f->fmt.pix_mp.field = sub_f->format.field;
			f->fmt.pix_mp.colorspace = sub_f->format.colorspace;
			f->fmt.pix_mp.ycbcr_enc = sub_f->format.ycbcr_enc;
			f->fmt.pix_mp.quantization = sub_f->format.quantization;
			f->fmt.pix_mp.xfer_func = sub_f->format.xfer_func;
			width = CAM_ALIGN(sub_f->format.width, spm_camera_formats_table[loop].pixel_width_align);
			if (0 == spm_camera_formats_table[loop].pixel_height_align)
				spm_camera_formats_table[loop].pixel_height_align = 1;
			height = CAM_ALIGN(sub_f->format.height, spm_camera_formats_table[loop].pixel_height_align);
			cam_dbg("%s width=%u, width_align=%u",__func__ ,width, spm_camera_formats_table[loop].pixel_width_align);
			f->fmt.pix_mp.num_planes = spm_camera_formats_table[loop].num_planes;
			for (plane = 0; plane < f->fmt.pix_mp.num_planes; plane++) {
				plane_fmt = &f->fmt.pix_mp.plane_fmt[plane];
				if (V4L2_PIX_FMT_NV12_AFBC == spm_camera_formats_table[loop].pixelformat) {
					//height = CAM_ALIGN(sub_f->format.height, 4);
					plane_fmt->bytesperline = 0x1000;
					if (0 == plane) {
						//(ceil(width / 32) * ceil(height / 4)) * 8
						plane_fmt->sizeimage = (width * height) >> 4;
					} else {
						//(ceil(width / 32) * ceil(height / 4)) * 192
						plane_fmt->sizeimage = (width * height * 3) >> 1;
					}
				} else {
					stride = CAM_ALIGN((width * spm_camera_formats_table[loop].plane_bpp[plane].num) / (spm_camera_formats_table[loop].plane_bpp[plane].den * 8),
							    spm_camera_formats_table[loop].plane_bytes_align[plane]);
					plane_fmt->sizeimage =
						height * stride * spm_camera_formats_table[loop].height_subsampling[plane].num / spm_camera_formats_table[loop].height_subsampling[plane].den;
					plane_fmt->bytesperline = stride;
				}
				cam_dbg("plane%d stride=%u", plane, stride);
			}
			break;
		}
	}
}

static int spm_vdev_queue_setup(struct vb2_queue *q,
				unsigned int *num_buffers,
				unsigned int *num_planes,
				unsigned int sizes[],
				struct device *alloc_devs[])
{
	struct spm_camera_vnode *sc_vnode = container_of(q, struct spm_camera_vnode, buf_queue);
	int loop = 0;

	if (num_buffers && num_planes) {
		*num_planes = sc_vnode->cur_fmt.fmt.pix_mp.num_planes;
		cam_dbg("%s num_buffers=%d num_planes=%d ", __func__, *num_buffers, *num_planes);
		for (loop = 0; loop < *num_planes; loop++) {
			sizes[loop] = sc_vnode->cur_fmt.fmt.pix_mp.plane_fmt[loop].sizeimage;
			cam_dbg("plane%d size=%u ", loop, sizes[loop]);
		}
	} else {
		cam_err("%s NULL num_buffers or num_planes ", __func__);
		return -EINVAL;
	}

	return 0;
}

static void spm_vdev_wait_prepare(struct vb2_queue *q)
{
	//going to wait sleep, release all locks that may block any vb2 buf/stream functions
	struct spm_camera_vnode *sc_vnode = container_of(q, struct spm_camera_vnode, buf_queue);
	mutex_unlock(&sc_vnode->mlock);
}

static void spm_vdev_wait_finish(struct vb2_queue *q)
{
	//wakeup from wait sleep, reacquire all locks
	struct spm_camera_vnode *sc_vnode = container_of(q, struct spm_camera_vnode, buf_queue);
	mutex_lock(&sc_vnode->mlock);
}

static int spm_vdev_buf_init(struct vb2_buffer *vb)
{
	struct spm_camera_vbuffer *sc_vb = to_camera_vbuffer(vb);

	INIT_LIST_HEAD(&sc_vb->list_entry);
	sc_vb->reset_flag = 0;
	return 0;
}

static int spm_vdev_buf_prepare(struct vb2_buffer *vb)
{
	struct spm_camera_vbuffer *sc_vb = to_camera_vbuffer(vb);
	struct spm_camera_vnode *sc_vnode = container_of(vb->vb2_queue, struct spm_camera_vnode, buf_queue);

	sc_vb->flags = 0;
	sc_vb->timestamp_eof = 0;
	memset(sc_vb->reserved, 0, SC_BUF_RESERVED_DATA_LEN);
	//sc_vb->vb2_v4l2_buf.flags &= ~V4L2_BUF_FLAG_IGNOR;
	sc_vb->vb2_v4l2_buf.flags = 0;
	sc_vb->sc_vnode = sc_vnode;
	blocking_notifier_call_chain(&sc_vnode->notify_chain, KY_VNODE_NOTIFY_BUF_PREPARE, sc_vb);
	return 0;
}

static void spm_vdev_buf_finish(struct vb2_buffer *vb)
{
}

static void spm_vdev_buf_cleanup(struct vb2_buffer *vb)
{

}

static int spm_vdev_start_streaming(struct vb2_queue *q, unsigned int count)
{
	struct spm_camera_vnode *sc_vnode = container_of(q, struct spm_camera_vnode, buf_queue);
	struct media_entity *me = &sc_vnode->vnode.entity, *remote_me = NULL;
	struct media_pad *remote_pad = media_entity_remote_pad(&me->pads[0]);
	struct v4l2_subdev *remote_sd = NULL;
	struct spm_camera_subdev *remote_sc_subdev = NULL;
	struct v4l2_subdev_format sd_fmt;
	int ret = 0;

	cam_dbg("%s(%s)", __func__, sc_vnode->name);
	sc_vnode->total_frm = 0;
	sc_vnode->sw_err_frm = 0;
	sc_vnode->hw_err_frm = 0;
	sc_vnode->ok_frm = 0;
	ret = blocking_notifier_call_chain(&sc_vnode->notify_chain, KY_VNODE_NOTIFY_STREAM_ON, sc_vnode);
	if (ret) {
		cam_err("%s notifer_call_chain failed. ", __func__);
	}

	ret = SAINT_CHECK();
	if (ret)
		return ret;
	remote_sc_subdev = v4l2_subdev_to_sc_subdev(remote_sd);
	/* use link_validata to notify vnode stream on */
	sd_fmt.which = 1;	//1 means stream on
	sd_fmt.pad = remote_pad->index;
	ret = v4l2_subdev_call(remote_sd, pad, link_validate, NULL, &sd_fmt, &sd_fmt);
	if (ret) {
		cam_err("%s stream on from remote sd(%s) on pad(%d) failed.",
			__func__, remote_sc_subdev->name, remote_pad->index);
	}

	return ret;
}

static void spm_vdev_stop_streaming(struct vb2_queue *q)
{
	struct spm_camera_vnode *sc_vnode = container_of(q, struct spm_camera_vnode, buf_queue);
	struct media_entity *me = &sc_vnode->vnode.entity, *remote_me = NULL;
	struct media_pad *remote_pad = media_entity_remote_pad(&me->pads[0]);
	struct v4l2_subdev *remote_sd = NULL;
	struct spm_camera_subdev *remote_sc_subdev = NULL;
	struct v4l2_subdev_format sd_fmt;
	int ret = 0;

	cam_dbg("%s(%s) enter", __func__, sc_vnode->name);
	ret = blocking_notifier_call_chain(&sc_vnode->notify_chain, KY_VNODE_NOTIFY_STREAM_OFF, sc_vnode);
	if (ret) {
		cam_err("%s notifer_call_chain failed. ", __func__);
	}

	ret = SAINT_CHECK();
	if (ret)
		return;
	remote_sc_subdev = v4l2_subdev_to_sc_subdev(remote_sd);
	/* use link_validata to notify vnode stream off */
	sd_fmt.which = 0;	//0 means stream off
	sd_fmt.pad = remote_pad->index;
	ret = v4l2_subdev_call(remote_sd, pad, link_validate, NULL, &sd_fmt, &sd_fmt);
	if (ret) {
		cam_err("%s stream off from remote sd(%s) on pad(%d) failed.",
			__func__, remote_sc_subdev->name, remote_pad->index);
	}
	cam_not("%s total_frm(%u) sw_err_frm(%u) hw_err_frm(%u) ok_frm(%u)",
		sc_vnode->name, sc_vnode->total_frm, sc_vnode->sw_err_frm, sc_vnode->hw_err_frm, sc_vnode->ok_frm);
	cam_dbg("%s(%s) leave", __func__, sc_vnode->name);
}

static void spm_vdev_buf_queue(struct vb2_buffer *vb)
{
	unsigned long flags = 0;
	struct spm_camera_vbuffer *sc_vb = to_camera_vbuffer(vb);
	struct vb2_queue *buf_queue = vb->vb2_queue;
	struct spm_camera_vnode *sc_vnode = container_of(buf_queue, struct spm_camera_vnode, buf_queue);
	unsigned int v4l2_buf_flags = sc_vnode->v4l2_buf_flags[vb->index];

	if (v4l2_buf_flags & V4l2_BUF_FLAG_FORCE_SHADOW
		&& (sc_vnode->idx == 12 || sc_vnode->idx == 13)) {
		sc_vb->flags |= SC_BUF_FLAG_FORCE_SHADOW;
	}
	sc_vb->timestamp_qbuf = ktime_get_boottime_ns();
	spin_lock_irqsave(&sc_vnode->slock, flags);
	atomic_inc(&sc_vnode->queued_buf_cnt);
	list_add_tail(&sc_vb->list_entry, &sc_vnode->queued_list);
	spin_unlock_irqrestore(&sc_vnode->slock, flags);
}

static struct vb2_ops spm_camera_vb2_ops = {
	.queue_setup = spm_vdev_queue_setup,
	.wait_prepare = spm_vdev_wait_prepare,
	.wait_finish = spm_vdev_wait_finish,
	.buf_init = spm_vdev_buf_init,
	.buf_prepare = spm_vdev_buf_prepare,
	.buf_finish = spm_vdev_buf_finish,
	.buf_cleanup = spm_vdev_buf_cleanup,
	.start_streaming = spm_vdev_start_streaming,
	.stop_streaming = spm_vdev_stop_streaming,
	.buf_queue = spm_vdev_buf_queue,
};

static void spm_vdev_cancel_all_buffers(struct spm_camera_vnode *sc_vnode)
{
	unsigned long flags = 0;
	struct spm_camera_vbuffer *pos = NULL, *n = NULL;
	struct vb2_buffer *vb2_buf = NULL;

	spin_lock_irqsave(&sc_vnode->slock, flags);
	list_for_each_entry_safe(pos, n, &sc_vnode->queued_list, list_entry) {
		vb2_buf = &(pos->vb2_v4l2_buf.vb2_buf);
		vb2_buffer_done(vb2_buf, VB2_BUF_STATE_ERROR);
		list_del_init(&pos->list_entry);
		atomic_dec(&sc_vnode->queued_buf_cnt);
	}
	list_for_each_entry_safe(pos, n, &sc_vnode->busy_list, list_entry) {
		vb2_buf = &(pos->vb2_v4l2_buf.vb2_buf);
		vb2_buffer_done(vb2_buf, VB2_BUF_STATE_ERROR);
		list_del_init(&pos->list_entry);
		atomic_dec(&sc_vnode->busy_buf_cnt);
	}
	if (sc_vnode->sc_vb && !(sc_vnode->sc_vb->flags & SC_BUF_FLAG_RSVD_Z1)) {
		vb2_buf = &(sc_vnode->sc_vb->vb2_v4l2_buf.vb2_buf);
		vb2_buffer_done(vb2_buf, VB2_BUF_STATE_DONE);
		sc_vnode->sc_vb = NULL;
	}
	spin_unlock_irqrestore(&sc_vnode->slock, flags);
}

static int spm_vdev_vidioc_reqbufs(struct file *file, void *fh, struct v4l2_requestbuffers *b)
{
	struct video_device *vnode = video_devdata(file);
	struct spm_camera_vnode *sc_vnode = container_of(vnode, struct spm_camera_vnode, vnode);
	int ret = 0;

	mutex_lock(&sc_vnode->mlock);
	ret = vb2_reqbufs(&sc_vnode->buf_queue, b);
	mutex_unlock(&sc_vnode->mlock);
	return ret;
}

static int spm_vdev_vidioc_querybuf(struct file *file, void *fh, struct v4l2_buffer *b)
{
	struct video_device *vnode = video_devdata(file);
	struct spm_camera_vnode *sc_vnode = container_of(vnode, struct spm_camera_vnode, vnode);
	int ret = 0;

	mutex_lock(&sc_vnode->mlock);
	ret = vb2_querybuf(&sc_vnode->buf_queue, b);
	mutex_unlock(&sc_vnode->mlock);
	return ret;
}

static int spm_vdev_vidioc_qbuf(struct file *file, void *fh, struct v4l2_buffer *b)
{
	struct video_device *vnode = video_devdata(file);
	struct spm_camera_vnode *sc_vnode = container_of(vnode, struct spm_camera_vnode, vnode);
	int ret = 0;
	unsigned int i = 0;

	sc_vnode->v4l2_buf_flags[b->index] = b->flags;
	for (i = 0; i < b->length; i++) {
		sc_vnode->planes_offset[b->index][i] = b->m.planes[i].data_offset;
	}
	mutex_lock(&sc_vnode->mlock);
	ret = vb2_qbuf(&sc_vnode->buf_queue, vnode->v4l2_dev->mdev, b);
	if (ret == 0) {
		blocking_notifier_call_chain(&sc_vnode->notify_chain, KY_VNODE_NOTIFY_BUF_QUEUED, sc_vnode);
	}
	mutex_unlock(&sc_vnode->mlock);
	return ret;
}

static int spm_vdev_vidioc_expbuf(struct file *file, void *fh, struct v4l2_exportbuffer *e)
{
	struct video_device *vnode = video_devdata(file);
	struct spm_camera_vnode *sc_vnode = container_of(vnode, struct spm_camera_vnode, vnode);
	int ret = 0;

	mutex_lock(&sc_vnode->mlock);
#ifndef MODULE
	ret = vb2_expbuf(&sc_vnode->buf_queue, e);
#endif
	mutex_unlock(&sc_vnode->mlock);
	return ret;
}

static int spm_vdev_vidioc_dqbuf(struct file *file, void *fh, struct v4l2_buffer *b)
{
	struct video_device *vnode = video_devdata(file);
	struct spm_camera_vnode *sc_vnode = container_of(vnode, struct spm_camera_vnode, vnode);
	int ret = 0;

	mutex_lock(&sc_vnode->mlock);
	ret = vb2_dqbuf(&sc_vnode->buf_queue, b, file->f_flags & O_NONBLOCK);
	mutex_unlock(&sc_vnode->mlock);
	return ret;
}

static int spm_vdev_vidioc_create_bufs(struct file *file, void *fh, struct v4l2_create_buffers *b)
{
	struct video_device *vnode = video_devdata(file);
	struct spm_camera_vnode *sc_vnode = container_of(vnode, struct spm_camera_vnode, vnode);
	int ret = 0;

	mutex_lock(&sc_vnode->mlock);
#ifndef MODULE
	ret = vb2_create_bufs(&sc_vnode->buf_queue, b);
#endif
	mutex_unlock(&sc_vnode->mlock);
	return ret;
}

static int spm_vdev_vidioc_prepare_buf(struct file *file, void *fh, struct v4l2_buffer *b)
{
	struct video_device *vnode = video_devdata(file);
	struct spm_camera_vnode *sc_vnode = container_of(vnode, struct spm_camera_vnode, vnode);
	int ret = 0;

	mutex_lock(&sc_vnode->mlock);
	ret = vb2_prepare_buf(&sc_vnode->buf_queue, vnode->v4l2_dev->mdev, b);
	mutex_unlock(&sc_vnode->mlock);
	return ret;
}

static int spm_vdev_vidioc_streamon(struct file *file, void *fn, enum v4l2_buf_type i)
{
	int ret = 0;
	struct video_device *vnode = video_devdata(file);
	struct spm_camera_vnode *sc_vnode = container_of(vnode, struct spm_camera_vnode, vnode);
	mutex_lock(&sc_vnode->mlock);
	ret = vb2_streamon(&sc_vnode->buf_queue, i);
	mutex_unlock(&sc_vnode->mlock);
	return ret;
}

static int spm_vdev_vidioc_streamoff(struct file *file, void *fn, enum v4l2_buf_type i)
{
	int ret = 0;
	struct video_device *vnode = video_devdata(file);
	struct spm_camera_vnode *sc_vnode = container_of(vnode, struct spm_camera_vnode, vnode);
	unsigned long flags = 0;

	cam_dbg("%s(%s) enter", __func__, sc_vnode->name);
	cam_dbg("%s(%s) queued_buf_cnt=%d busy_buf_cnt=%d.", __func__, sc_vnode->name,
		atomic_read(&sc_vnode->queued_buf_cnt),
		atomic_read(&sc_vnode->busy_buf_cnt));
	spin_lock_irqsave(&sc_vnode->waitq_head.lock, flags);
	wait_event_interruptible_locked_irq(sc_vnode->waitq_head, !sc_vnode->in_tasklet);
	sc_vnode->in_streamoff = 1;
	spin_unlock_irqrestore(&sc_vnode->waitq_head.lock, flags);
	cam_dbg("%s tasklet clean", sc_vnode->name);
	mutex_lock(&sc_vnode->mlock);
	cam_dbg("%s cancel all buffers", sc_vnode->name);
	spm_vdev_cancel_all_buffers(sc_vnode);
	cam_dbg("%s streamoff", sc_vnode->name);
	ret = vb2_streamoff(&sc_vnode->buf_queue, i);
	mutex_unlock(&sc_vnode->mlock);
	spin_lock_irqsave(&sc_vnode->waitq_head.lock, flags);
	sc_vnode->in_streamoff = 0;
	spin_unlock_irqrestore(&sc_vnode->waitq_head.lock, flags);
	cam_dbg("%s(%s) leave", __func__, sc_vnode->name);
	return ret;
}

static int spm_vdev_vidioc_querycap(struct file *file, void *fh, struct v4l2_capability *cap)
{
	strlcpy(cap->driver, "kyisp", 16);
	cap->capabilities = V4L2_CAP_DEVICE_CAPS | V4L2_CAP_STREAMING | V4L2_CAP_VIDEO_CAPTURE_MPLANE;
	cap->device_caps = V4L2_CAP_STREAMING | V4L2_CAP_VIDEO_CAPTURE_MPLANE;
	return 0;
}

/*
 * static int spm_vdev_vidioc_enum_fmt_vid_cap_mplane(struct file *file, void *fh, struct v4l2_fmtdesc *f)
 * {
 *     return 0;
 * }
 */

static int spm_vdev_vidioc_g_fmt_vid_cap_mplane(struct file *file, void *fh, struct v4l2_format *f)
{
	struct video_device *vnode = video_devdata(file);
	struct spm_camera_vnode *sc_vnode = container_of(vnode, struct spm_camera_vnode, vnode);
	struct media_entity *me = &sc_vnode->vnode.entity, *remote_me = NULL;
	struct media_pad *remote_pad = media_entity_remote_pad(&me->pads[0]);
	struct v4l2_subdev *remote_sd = NULL;
	int ret = 0;

	ret = SAINT_CHECK();
	if (ret)
		return ret;
	cam_dbg("get format fourcc code[0x%08x] (%dx%d)",
		sc_vnode->cur_fmt.fmt.pix_mp.pixelformat,
		sc_vnode->cur_fmt.fmt.pix_mp.width,
		sc_vnode->cur_fmt.fmt.pix_mp.height);
	*f = sc_vnode->cur_fmt;
	return 0;
}

static int __spm_vdev_vidioc_s_fmt_vid_cap_mplane(struct file *file, void *fh, struct v4l2_format *f)
{
	struct video_device *vnode = video_devdata(file);
	struct spm_camera_vnode *sc_vnode = container_of(vnode, struct spm_camera_vnode, vnode);
	struct media_entity *me = &sc_vnode->vnode.entity, *remote_me = NULL;
	struct media_pad *remote_pad = media_entity_remote_pad(&me->pads[0]);
	struct v4l2_subdev *remote_sd = NULL;
	struct spm_camera_subdev *remote_sc_subdev = NULL;
	struct vb2_queue *vb2_queue = &sc_vnode->buf_queue;
	int ret = 0;
	struct v4l2_subdev_format pad_format = { 0 }, tmp_format = { 0 };

	cam_dbg("set format fourcc code[0x%08x] (%dx%d)",
		f->fmt.pix_mp.pixelformat, f->fmt.pix_mp.width, f->fmt.pix_mp.height);
	if (vb2_is_streaming(vb2_queue)) {
		cam_err("%s set format not allowed while streaming on.", __func__);
		return -EBUSY;
	}
	ret = spm_vdev_lookup_formats_table(f);
	if (ret) {
		cam_err("%s failed to lookup formats table fourcc code[0x%08x]",
			__func__, f->fmt.pix_mp.pixelformat);
		return ret;
	}
	ret = SAINT_CHECK();
	if (ret)
		return ret;
	remote_sc_subdev = v4l2_subdev_to_sc_subdev(remote_sd);
	pad_format.which = V4L2_SUBDEV_FORMAT_ACTIVE;
	pad_format.pad = remote_pad->index;
	if (sc_vnode->direction == KY_VNODE_DIR_OUT) {
		spm_vdev_fill_subdev_format(f, &pad_format);
		ret = v4l2_subdev_call(remote_sd, pad, set_fmt, NULL, &pad_format);
		if (ret) {
			cam_err("%s set format failed on remote subdev(%s).", __func__,
				remote_sc_subdev->name);
			return ret;
		}
	} else {
		ret = v4l2_subdev_call(remote_sd, pad, get_fmt, NULL, &pad_format);
		if (ret) {
			cam_err("%s get format failed on remote subdev(%s).", __func__,
				remote_sc_subdev->name);
			return ret;
		}
		spm_vdev_fill_subdev_format(f, &tmp_format);
		if (tmp_format.format.width != pad_format.format.width
		    || tmp_format.format.height != pad_format.format.height
		    || tmp_format.format.code != pad_format.format.code) {
			cam_err("%s remote subdev(%s) didn't support this format.",
				__func__, remote_sc_subdev->name);
			return -EINVAL;
		}
		ret = v4l2_subdev_call(remote_sd, pad, set_fmt, NULL, &pad_format);
		if (ret) {
			cam_err("%s set format failed on remote subdev(%s).", __func__, remote_sc_subdev->name);
			return ret;
		}
	}
	spm_vdev_fill_v4l2_format(&pad_format, f);
	sc_vnode->cur_fmt = *f;

	return 0;
}

static int spm_vdev_vidioc_s_fmt_vid_cap_mplane(struct file *file, void *fh, struct v4l2_format *f)
{
	struct video_device *vnode = video_devdata(file);
	struct spm_camera_vnode *sc_vnode = container_of(vnode, struct spm_camera_vnode, vnode);
	int ret = 0;

	mutex_lock(&sc_vnode->mlock);
	ret = __spm_vdev_vidioc_s_fmt_vid_cap_mplane(file, fh, f);
	mutex_unlock(&sc_vnode->mlock);
	return ret;
}

static int spm_vdev_vidioc_try_fmt_vid_cap_mplane(struct file *file, void *fh, struct v4l2_format *f)
{
	return 0;
}

static void spm_vdev_debug_dump(struct spm_camera_vnode *sc_vnode, int reason)
{
	__u64 now = 0;
	unsigned long flags = 0, t = 0;
	struct spm_camera_vbuffer *pos = NULL;

	now = ktime_get_boottime_ns();
	cam_not("%s(a[%u]) queued_buf_cnt=%d busy_buf_cnt=%d reason=%d", __func__,
		sc_vnode->idx, atomic_read(&sc_vnode->queued_buf_cnt),
		atomic_read(&sc_vnode->busy_buf_cnt), reason);
	spin_lock_irqsave(&sc_vnode->slock, flags);
	list_for_each_entry(pos, &sc_vnode->queued_list, list_entry) {
		t = (unsigned long)(now - pos->timestamp_qbuf);
		t /= 1000;
		cam_not("a[%u] queued_list buf_index=%u t=%lu reason=%d", sc_vnode->idx,
			pos->vb2_v4l2_buf.vb2_buf.index, t, reason);
	}
	list_for_each_entry(pos, &sc_vnode->busy_list, list_entry) {
		t = (unsigned long)(now - pos->timestamp_qbuf);
		t /= 1000;
		cam_not("a[%u] busy_list buf_index=%u t=%lu reason=%d", sc_vnode->idx,
			pos->vb2_v4l2_buf.vb2_buf.index, t, reason);
	}
	spin_unlock_irqrestore(&sc_vnode->slock, flags);
}

static long spm_vdev_vidioc_default(struct file *file,
				    void *fh,
				    bool valid_prio, unsigned int cmd, void *arg)
{
	struct video_device *vnode = video_devdata(file);
	struct spm_camera_vnode *sc_vnode = container_of(vnode, struct spm_camera_vnode, vnode);
	struct v4l2_vi_entity_info *entity_info = NULL;
	struct spm_camera_pipeline *sc_pipeline = NULL;
	struct media_pipeline *pipe = media_entity_pipeline(&vnode->entity);
	int *slice_mode = NULL, *is_z1 = NULL;
	struct v4l2_vi_slice_info *slice_info = NULL;
	struct v4l2_vi_debug_dump *debug_dump = NULL;
	int ret = 0;

	switch (cmd) {
	case VIDIOC_G_ENTITY_INFO:
		entity_info = (struct v4l2_vi_entity_info *)arg;
		entity_info->id = media_entity_id(&sc_vnode->vnode.entity);
		strlcpy(entity_info->name, sc_vnode->name, KY_VI_ENTITY_NAME_LEN);
		break;
	case VIDIOC_G_SLICE_MODE:
		BUG_ON(!pipe);
		sc_pipeline = media_pipeline_to_sc_pipeline(pipe);
		BUG_ON(!sc_pipeline);
		slice_mode = (int *)arg;
		*slice_mode = sc_pipeline->is_slice_mode;
		cam_dbg("VIDIOC_G_SLICE_MODE slice_mode(%d) %s", *slice_mode, sc_vnode->name);
		break;
	case VIDIOC_QUERY_SLICE_READY:
		cam_not("query slice info ready");
		BUG_ON(!pipe);
		sc_pipeline = media_pipeline_to_sc_pipeline(pipe);
		BUG_ON(!sc_pipeline);
		if (sc_pipeline->state <= PIPELINE_ST_STOPPING) {
			return -2;
		}
		slice_info = (struct v4l2_vi_slice_info *)arg;
		ret = wait_event_interruptible_timeout(sc_pipeline->slice_waitq,
							atomic_read(&sc_pipeline->slice_info_update),
							msecs_to_jiffies(slice_info->timeout));
		if (sc_pipeline->state <= PIPELINE_ST_STOPPING) {
			return -2;
		}
		if (ret == 0) {
			cam_err("%s wait isp slice info notify timeout(%u)", __func__, slice_info->timeout);
			return -1;
		} else if (ret < 0) {
			cam_err("%s wait isp slice info notify interrupted by user app", __func__);
			return -1;
		}
		atomic_set(&sc_pipeline->slice_info_update, 0);
		blocking_notifier_call_chain(&sc_pipeline->blocking_notify_chain,
					     PIPELINE_ACTION_SLICE_READY, sc_pipeline);
		slice_info->slice_id = sc_pipeline->slice_id;
		slice_info->total_slice_cnt = sc_pipeline->total_slice_cnt;
		break;
	case VIDIOC_S_SLICE_DONE:
		BUG_ON(!pipe);
		sc_pipeline = media_pipeline_to_sc_pipeline(pipe);
		BUG_ON(!sc_pipeline);
		atomic_set(&sc_pipeline->slice_info_update, 0);
		sc_pipeline->slice_result = *((int *)arg);
		complete_all(&sc_pipeline->slice_done);
		break;
	case VIDIOC_CPU_Z1:
		is_z1 = (int *)arg;
		*is_z1 = 0;
		break;
	case VIDIOC_DEBUG_DUMP:
		debug_dump = (struct v4l2_vi_debug_dump *)arg;
		spm_vdev_debug_dump(sc_vnode, debug_dump->reason);
		break;
	default:
		cam_warn("unknown ioctl cmd(%d).", cmd);
		return -ENOIOCTLCMD;
	}

	return 0;
}

static struct v4l2_ioctl_ops spm_camera_v4l2_ioctl_ops = {
	/* VIDIOC_QUERYCAP handler */
	.vidioc_querycap = spm_vdev_vidioc_querycap,
	/* VIDIOC_ENUM_FMT handlers */
	/* .vidioc_enum_fmt_vid_cap = spm_vdev_vidioc_enum_fmt_vid_cap_mplane, */
	/* VIDIOC_G_FMT handlers */
	.vidioc_g_fmt_vid_cap_mplane = spm_vdev_vidioc_g_fmt_vid_cap_mplane,
	/* VIDIOC_S_FMT handlers */
	.vidioc_s_fmt_vid_cap_mplane = spm_vdev_vidioc_s_fmt_vid_cap_mplane,
	/* VIDIOC_TRY_FMT handlers */
	.vidioc_try_fmt_vid_cap_mplane = spm_vdev_vidioc_try_fmt_vid_cap_mplane,
	/* Buffer handlers */
	.vidioc_reqbufs = spm_vdev_vidioc_reqbufs,
	.vidioc_querybuf = spm_vdev_vidioc_querybuf,
	.vidioc_qbuf = spm_vdev_vidioc_qbuf,
	.vidioc_expbuf = spm_vdev_vidioc_expbuf,
	.vidioc_dqbuf = spm_vdev_vidioc_dqbuf,
	.vidioc_create_bufs = spm_vdev_vidioc_create_bufs,
	.vidioc_prepare_buf = spm_vdev_vidioc_prepare_buf,
	.vidioc_streamon = spm_vdev_vidioc_streamon,
	.vidioc_streamoff = spm_vdev_vidioc_streamoff,
	.vidioc_default = spm_vdev_vidioc_default,
};

static int spm_vdev_open(struct file *file)
{
	struct video_device *vnode = video_devdata(file);
	struct spm_camera_vnode *sc_vnode = container_of(vnode, struct spm_camera_vnode, vnode);

	if (atomic_inc_return(&sc_vnode->ref_cnt) != 1) {
		cam_err("vnode(%s - %s) was already openned.", sc_vnode->name,
			video_device_node_name(vnode));
		atomic_dec(&sc_vnode->ref_cnt);
		return -EBUSY;
	}

	cam_dbg("open vnode(%s - %s).", sc_vnode->name, video_device_node_name(vnode));
	return 0;
}

static void __spm_vdev_close(struct spm_camera_vnode *sc_vnode)
{
	unsigned long flags = 0;

	cam_dbg("%s(%s) enter", __func__, sc_vnode->name);
	cam_dbg("%s(%s) queued_buf_cnt=%d busy_buf_cnt=%d.", __func__, sc_vnode->name,
		atomic_read(&sc_vnode->queued_buf_cnt),
		atomic_read(&sc_vnode->busy_buf_cnt));
	spin_lock_irqsave(&sc_vnode->waitq_head.lock, flags);
	sc_vnode->in_streamoff = 1;
	wait_event_interruptible_locked_irq(sc_vnode->waitq_head, !sc_vnode->in_tasklet);
	spin_unlock_irqrestore(&sc_vnode->waitq_head.lock, flags);
	cam_dbg("%s tasklet clean", sc_vnode->name);
	mutex_lock(&sc_vnode->mlock);
	cam_dbg("%s cancel all buffers", sc_vnode->name);
	spm_vdev_cancel_all_buffers(sc_vnode);
	cam_dbg("%s queue release", sc_vnode->name);
	vb2_queue_release(&sc_vnode->buf_queue);
	sc_vnode->buf_queue.owner = NULL;
	mutex_unlock(&sc_vnode->mlock);
	spin_lock_irqsave(&sc_vnode->waitq_head.lock, flags);
	sc_vnode->in_streamoff = 0;
	spin_unlock_irqrestore(&sc_vnode->waitq_head.lock, flags);
	cam_dbg("%s(%s) leave", __func__, sc_vnode->name);
}

static int spm_vdev_close(struct file *file)
{
	struct video_device *vnode = video_devdata(file);
	struct spm_camera_vnode *sc_vnode = container_of(vnode, struct spm_camera_vnode, vnode);

	if (atomic_dec_and_test(&sc_vnode->ref_cnt)) {
		__spm_vdev_close(sc_vnode);
	}

	return v4l2_fh_release(file);
}

static __poll_t spm_vdev_poll(struct file *file, struct poll_table_struct *wait)
{
	__poll_t ret;
	struct video_device *vnode = video_devdata(file);
	struct spm_camera_vnode *sc_vnode = container_of(vnode, struct spm_camera_vnode, vnode);

	ret = vb2_poll(&sc_vnode->buf_queue, file, wait);

	return ret;
}

//#ifdef CONFIG_COMPAT
#if 0

static int alloc_userspace(unsigned int size, u32 aux_space, void __user **new_p64)
{
	*new_p64 = compat_alloc_user_space(size + aux_space);
	if (!*new_p64)
		return -ENOMEM;
	if (clear_user(*new_p64, size))
		return -EFAULT;
	return 0;
}

long spm_vdev_compat_ioctl32(struct file *file, unsigned int cmd, unsigned long arg)
{
	void __user *p32 = compat_ptr(arg);
	void __user *new_p64 = NULL;
	//void __user *aux_buf;
	//u32 aux_space;
	long err = 0;
	const size_t ioc_size = _IOC_SIZE(cmd);
	//size_t ioc_size64 = 0;

	//if (_IOC_TYPE(cmd) == 'V') {
	//	switch (_IOC_NR(cmd)) {
	//		//int r
	//		case _IOC_NR(VIDIOC_G_SLICE_MODE):
	//		case _IOC_NR(VIDIOC_CPU_Z1):
	//		//int w
	//		case _IOC_NR(VIDIOC_PUT_PIPELINE):
	//		case _IOC_NR(VIDIOC_RESET_PIPELINE):
	//		ioc_size64 = sizeof(int);
	//		break;
	//		//unsigned int
	//		case _IOC_NR(VIDIOC_G_PIPE_STATUS):
	//		ioc_size64 = sizeof(int);
	//		break;
	//		case _IOC_NR(VIDIOC_S_PORT_CFG):
	//		ioc_size64 = sizeof(struct v4l2_vi_port_cfg);
	//		break;
	//		case _IOC_NR(VIDIOC_DBG_REG_WRITE):
	//		case _IOC_NR(VIDIOC_DBG_REG_READ):
	//		ioc_size64 = sizeof(struct v4l2_vi_dbg_reg);
	//		break;
	//		case _IOC_NR(VIDIOC_CFG_INPUT_INTF):
	//		ioc_size64 = sizeof(struct v4l2_vi_input_interface);
	//		break;
	//		case _IOC_NR(VIDIOC_SET_SELECTION):
	//		ioc_size64 = sizeof(struct v4l2_vi_selection);
	//		break;
	//		case _IOC_NR(VIDIOC_QUERY_SLICE_READY):
	//		ioc_size64 = sizeof(struct v4l2_vi_slice_info);
	//		break;
	//		case _IOC_NR(VIDIOC_S_BANDWIDTH):
	//		ioc_size64 = sizeof(struct v4l2_vi_bandwidth_info);
	//		break;
	//		case _IOC_NR(VIDIOC_G_ENTITY_INFO):
	//		ioc_size64 = sizeof(struct v4l2_vi_entity_info);
	//		break;
	//	}
	//	cam_dbg("%s cmd_nr=%d ioc_size32=%u ioc_size64=%u",__func__,  _IOC_NR(cmd), ioc_size, ioc_size64);
	//}
	if (_IOC_DIR(cmd) != _IOC_NONE) {
		err = alloc_userspace(ioc_size, 0, &new_p64);
		if (err) {
			cam_err("%s alloc userspace failed err=%l cmd=%d ioc_size=%u", __func__, err, _IOC_NR(cmd), ioc_size);
			return err;
		}
		if ((_IOC_DIR(cmd) & _IOC_WRITE)) {
			err = copy_in_user(new_p64, p32, ioc_size);
			if (err) {
				cam_err("%s copy in user 1 failed err=%l cmd=%d ioc_size=%u", __func__, err, _IOC_NR(cmd), ioc_size);
				return err;
			}
		}
	}

	err = video_ioctl2(file, cmd, (unsigned long)new_p64);
	if (err) {
		return err;
	}

	if ((_IOC_DIR(cmd) & _IOC_READ)) {
		err = copy_in_user(p32, new_p64, ioc_size);
		if (err) {
			cam_err("%s copy in user 2 failed err=%l cmd=%d ioc_size=%u", __func__, err, _IOC_NR(cmd), ioc_size);
			return err;
		}
	}
	//switch (cmd) {
	//	//int r
	//	case VIDIOC_G_SLICE_MODE:
	//	case VIDIOC_CPU_Z1:
	//	//int w 
	//	case VIDIOC_PUT_PIPELINE:
	//	case VIDIOC_RESET_PIPELINE:
	//	err = alloc_userspace(sizeof(int), 0, &new_p64);
	//	if (!err && assign_in_user((int __user *)new_p64,
	//				   (compat_int_t __user *)p32))
	//		err = -EFAULT;
	//	break;
	//	//unsigned int
	//	case VIDIOC_G_PIPE_STATUS:
	//	err = alloc_userspace(sizeof(unsigned int), 0, &new_p64);
	//	if (!err && assign_in_user((unsigned int __user *)new_p64,
	//				   (compat_uint_t __user *)p32))
	//		err = -EFAULT;
	//	break;
	//	case VIDIOC_S_PORT_CFG:
	//	err = alloc_userspace(sizeof(struct v4l2_vi_port_cfg), 0, &new_p64);
	//	if (!err) {
	//		err = -EFAULT;
	//		break;
	//	}
	//	break;
	//	case VIDIOC_DBG_REG_WRITE:
	//	case VIDIOC_DBG_REG_READ:
	//	break;
	//	case VIDIOC_CFG_INPUT_INTF:
	//	break;
	//	case VIDIOC_SET_SELECTION:
	//	break;
	//	case VIDIOC_QUERY_SLICE_READY:
	//	break;
	//	case VIDIOC_S_BANDWIDTH:
	//	break;
	//	case VIDIOC_G_ENTITY_INFO:
	//	break;
		
	//}	
	//if (err)
	//	return err;
	return 0;
}
#endif

static struct v4l2_file_operations spm_camera_file_operations = {
	.owner = THIS_MODULE,
	.poll = spm_vdev_poll,
	.unlocked_ioctl = video_ioctl2,
	.open = spm_vdev_open,
	.release = spm_vdev_close,
//#ifdef CONFIG_COMPAT
#if 0
	.compat_ioctl32 = spm_vdev_compat_ioctl32,
#endif
};

static void spm_vdev_release(struct video_device *vdev)
{
	struct spm_camera_vnode *sc_vnode = container_of(vdev, struct spm_camera_vnode, vnode);

	cam_dbg("%s(%s %s) enter.", __func__, sc_vnode->name, video_device_node_name(&sc_vnode->vnode));
	media_entity_cleanup(&sc_vnode->vnode.entity);
	mutex_destroy(&sc_vnode->mlock);
}

#ifdef MODULE
void media_gobj_destroy(struct media_gobj *gobj)
{
	/* Do nothing if the object is not linked. */
	if (gobj->mdev == NULL)
		return;

	gobj->mdev->topology_version++;

	/* Remove the object from mdev list */
	list_del(&gobj->list);

	gobj->mdev = NULL;
}

static void __media_entity_remove_link(struct media_entity *entity,
				       struct media_link *link)
{
	struct media_link *rlink, *tmp;
	struct media_entity *remote;

	if (link->source->entity == entity)
		remote = link->sink->entity;
	else
		remote = link->source->entity;

	list_for_each_entry_safe(rlink, tmp, &remote->links, list) {
		if (rlink != link->reverse)
			continue;

		if (link->source->entity == entity)
			remote->num_backlinks--;

		/* Remove the remote link */
		list_del(&rlink->list);
		media_gobj_destroy(&rlink->graph_obj);
		kfree(rlink);

		if (--remote->num_links == 0)
			break;
	}
	list_del(&link->list);
	media_gobj_destroy(&link->graph_obj);
	kfree(link);
}

void __spm_media_entity_remove_links(struct media_entity *entity)
{
	struct media_link *link, *tmp;

	list_for_each_entry_safe(link, tmp, &entity->links, list)
	    __media_entity_remove_link(entity, link);

	entity->num_links = 0;
	entity->num_backlinks = 0;
}

EXPORT_SYMBOL_GPL(__spm_media_entity_remove_links);

void spm_media_entity_remove_links(struct media_entity *entity)
{
	struct media_device *mdev = entity->graph_obj.mdev;

	/* Do nothing if the entity is not registered. */
	if (mdev == NULL)
		return;

	mutex_lock(&mdev->graph_mutex);
	__spm_media_entity_remove_links(entity);
	mutex_unlock(&mdev->graph_mutex);
}
#endif
static void spm_vdev_block_release(struct spm_camera_block *b)
{
	struct spm_camera_vnode *sc_vnode = container_of(b, struct spm_camera_vnode, sc_block);

	cam_dbg("%s(%s %s) enter.", __func__, sc_vnode->name, video_device_node_name(&sc_vnode->vnode));
	vb2_queue_release(&sc_vnode->buf_queue);
#ifdef MODULE
	spm_media_entity_remove_links(&sc_vnode->vnode.entity);
#else
	media_entity_remove_links(&sc_vnode->vnode.entity);
#endif
	video_unregister_device(&sc_vnode->vnode);
}

static struct spm_camera_block_ops sc_block_ops = {
	.release = spm_vdev_block_release,
};

static int spm_vdev_link_validate(struct media_link *link)
{
	struct media_entity *me = link->sink->entity;
	struct spm_camera_vnode *sc_vnode = media_entity_to_sc_vnode(me);
	struct spm_camera_pipeline *sc_pipeline = NULL;
	struct media_pipeline *mpipe = media_entity_pipeline(me);
	int ret = 0;

	if (!sc_vnode) {
		return -1;
	}
	if (!mpipe) {
		return -2;
	}

	sc_pipeline = media_pipeline_to_sc_pipeline(mpipe);
	ret = blocking_notifier_chain_register(&sc_pipeline->blocking_notify_chain, &sc_vnode->pipeline_notify_block);
	if (ret) {
		return ret;
	}
	return 0;
}

static struct media_entity_operations spm_camera_media_entity_ops = {
	.link_validate = spm_vdev_link_validate,
};

static int spm_vdev_pipeline_notify_fn(struct notifier_block *nb,
				       unsigned long action, void *data)
{
	struct entity_usrdata *entity_usrdata = NULL;
	struct spm_camera_vnode *sc_vnode = container_of(nb, struct spm_camera_vnode, pipeline_notify_block);
	switch (action) {
	case PIPELINE_ACTION_SET_ENTITY_USRDATA:
		entity_usrdata = (struct entity_usrdata *)data;
		cam_dbg("%s(%s) set entity usrdata(%d:%d)", __func__, sc_vnode->name,
			media_entity_id(&sc_vnode->vnode.entity),
			entity_usrdata->entity_id);
		if (media_entity_id(&sc_vnode->vnode.entity) == entity_usrdata->entity_id) {
			sc_vnode->usr_data = entity_usrdata->usr_data;
			return NOTIFY_STOP;
		}
		break;
	case PIPELINE_ACTION_GET_ENTITY_USRDATA:
		entity_usrdata = (struct entity_usrdata *)data;
		cam_dbg("%s(%s) get entity usrdata(%d:%d)", __func__, sc_vnode->name,
			media_entity_id(&sc_vnode->vnode.entity),
			entity_usrdata->entity_id);
		if (media_entity_id(&sc_vnode->vnode.entity) == entity_usrdata->entity_id) {
			entity_usrdata->usr_data = sc_vnode->usr_data;
			return NOTIFY_STOP;
		}
		break;
	default:
		return NOTIFY_DONE;
	}
	return NOTIFY_DONE;
}

struct spm_camera_vnode *spm_vdev_create_vnode(const char *name,
					       int direction,
					       unsigned int idx,
					       struct v4l2_device *v4l2_dev,
					       struct device *alloc_dev,
					       unsigned int min_buffers_needed)
{
	int ret = 0;
	struct spm_camera_vnode *sc_vnode = NULL;

	if (NULL == name || NULL == v4l2_dev || NULL == alloc_dev) {
		cam_err("%s invalid arguments.", __func__);
		return NULL;
	}
	if (direction < KY_VNODE_DIR_IN || direction > KY_VNODE_DIR_OUT) {
		cam_err("%s invalid direction.", __func__);
		return NULL;
	}
	sc_vnode = devm_kzalloc(alloc_dev, sizeof(*sc_vnode), GFP_KERNEL);
	if (NULL == sc_vnode) {
		cam_err("%s failed to alloc mem for spm_camera_vnode(%s).", __func__, name);
		return NULL;
	}

	INIT_LIST_HEAD(&sc_vnode->queued_list);
	INIT_LIST_HEAD(&sc_vnode->busy_list);
	atomic_set(&sc_vnode->queued_buf_cnt, 0);
	atomic_set(&sc_vnode->busy_buf_cnt, 0);
	spin_lock_init(&sc_vnode->slock);
	mutex_init(&sc_vnode->mlock);
	init_waitqueue_head(&sc_vnode->waitq_head);
	sc_vnode->direction = direction;
	if (direction == KY_VNODE_DIR_IN)
		sc_vnode->pad.flags = MEDIA_PAD_FL_SOURCE;
	else
		sc_vnode->pad.flags = MEDIA_PAD_FL_SINK;
	sc_vnode->idx = idx;
	BLOCKING_INIT_NOTIFIER_HEAD(&sc_vnode->notify_chain);
	sc_vnode->buf_queue.timestamp_flags =
	    V4L2_BUF_FLAG_TIMESTAMP_MONOTONIC | V4L2_BUF_FLAG_TSTAMP_SRC_SOE;
	sc_vnode->buf_queue.buf_struct_size = sizeof(struct spm_camera_vbuffer);
	sc_vnode->buf_queue.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
	sc_vnode->buf_queue.io_modes = KY_VB2_IO_MODE;
	sc_vnode->buf_queue.ops = &spm_camera_vb2_ops;
	sc_vnode->buf_queue.mem_ops = spm_vb2_mem_ops;
	sc_vnode->buf_queue.min_buffers_needed = min_buffers_needed;
	sc_vnode->buf_queue.dev = alloc_dev;
	ret = vb2_queue_init(&sc_vnode->buf_queue);
	if (ret) {
		cam_err("%s vb2_queue_init failed for spm_camera_vnode(%s).", __func__, name);
		goto queue_init_fail;
	}

	strlcpy(sc_vnode->vnode.name, name, 32);
	strlcpy(sc_vnode->name, name, KY_VI_ENTITY_NAME_LEN);
	sc_vnode->vnode.queue = &sc_vnode->buf_queue;
	sc_vnode->vnode.fops = &spm_camera_file_operations;
	sc_vnode->vnode.ioctl_ops = &spm_camera_v4l2_ioctl_ops;
	sc_vnode->vnode.release = spm_vdev_release;
	sc_vnode->vnode.entity.ops = &spm_camera_media_entity_ops;
	sc_vnode->vnode.device_caps = V4L2_CAP_STREAMING | V4L2_CAP_VIDEO_CAPTURE_MPLANE;
	ret = media_entity_pads_init(&sc_vnode->vnode.entity, 1, &sc_vnode->pad);
	if (ret) {
		cam_err("%s media entity pads init failed for spm_camera_vnode(%s).",
			__func__, name);
		goto entity_pads_init_fail;
	}
	spm_camera_block_init(&sc_vnode->sc_block, &sc_block_ops);
	sc_vnode->vnode.v4l2_dev = v4l2_dev;
	ret = __video_register_device(&sc_vnode->vnode, VFL_TYPE_VIDEO, -1, 1, THIS_MODULE);
	if (ret) {
		cam_err("%s video dev register failed for spm_camera_vnode(%s).",
			__func__, name);
		goto vdev_register_fail;
	}
	sc_vnode->vnode.entity.name = video_device_node_name(&sc_vnode->vnode);
	sc_vnode->pipeline_notify_block.notifier_call = spm_vdev_pipeline_notify_fn;
	sc_vnode->pipeline_notify_block.priority = SC_PIPE_NOTIFY_PRIO_NORMAL;

	cam_dbg("create vnode(%s - %s) successfully.", name,
		video_device_node_name(&sc_vnode->vnode));
	return sc_vnode;
vdev_register_fail:
entity_pads_init_fail:
	vb2_queue_release(&sc_vnode->buf_queue);
queue_init_fail:
	devm_kfree(alloc_dev, sc_vnode);
	return NULL;
}

int __spm_vdev_dq_idle_vbuffer(struct spm_camera_vnode *sc_vnode, struct spm_camera_vbuffer **sc_vb)
{
	*sc_vb = list_first_entry_or_null(&sc_vnode->queued_list, struct spm_camera_vbuffer, list_entry);
	if (NULL == *sc_vb)
		return -1;
	list_del_init(&(*sc_vb)->list_entry);
	atomic_dec(&sc_vnode->queued_buf_cnt);
	return 0;
}

int __spm_vdev_q_idle_vbuffer(struct spm_camera_vnode *sc_vnode, struct spm_camera_vbuffer *sc_vb)
{
	list_add_tail(&sc_vb->list_entry, &sc_vnode->queued_list);
	atomic_inc(&sc_vnode->queued_buf_cnt);
	return 0;
}

int spm_vdev_dq_idle_vbuffer(struct spm_camera_vnode *sc_vnode, struct spm_camera_vbuffer **sc_vb)
{
	unsigned long flags = 0;
	int ret = 0;

	spin_lock_irqsave(&sc_vnode->slock, flags);
	ret = __spm_vdev_dq_idle_vbuffer(sc_vnode, sc_vb);
	spin_unlock_irqrestore(&sc_vnode->slock, flags);
	return ret;
}

int spm_vdev_q_idle_vbuffer(struct spm_camera_vnode *sc_vnode, struct spm_camera_vbuffer *sc_vb)
{
	unsigned long flags = 0;
	int ret = 0;

	spin_lock_irqsave(&sc_vnode->slock, flags);
	ret = __spm_vdev_q_idle_vbuffer(sc_vnode, sc_vb);
	spin_unlock_irqrestore(&sc_vnode->slock, flags);

	return ret;
}

int spm_vdev_pick_idle_vbuffer(struct spm_camera_vnode *sc_vnode, struct spm_camera_vbuffer **sc_vb)
{
	unsigned long flags = 0;

	spin_lock_irqsave(&sc_vnode->slock, flags);
	*sc_vb = list_first_entry_or_null(&sc_vnode->queued_list, struct spm_camera_vbuffer, list_entry);
	spin_unlock_irqrestore(&sc_vnode->slock, flags);
	if (NULL == *sc_vb) {
		return -1;
	}
	return 0;
}

int __spm_vdev_pick_idle_vbuffer(struct spm_camera_vnode *sc_vnode, struct spm_camera_vbuffer **sc_vb)
{
	*sc_vb = list_first_entry_or_null(&sc_vnode->queued_list, struct spm_camera_vbuffer, list_entry);
	if (NULL == *sc_vb) {
		return -1;
	}
	return 0;
}

int __spm_vdev_dq_busy_vbuffer(struct spm_camera_vnode *sc_vnode, struct spm_camera_vbuffer **sc_vb)
{
	*sc_vb = list_first_entry_or_null(&sc_vnode->busy_list, struct spm_camera_vbuffer, list_entry);
	if (NULL == *sc_vb)
		return -1;
	list_del_init(&(*sc_vb)->list_entry);
	atomic_dec(&sc_vnode->busy_buf_cnt);
	return 0;
}

/*
int __spm_vdev_dq_busy_vbuffer_by_paddr(struct spm_camera_vnode *sc_vnode, int plane_id, unsigned long plane_paddr, struct spm_camera_vbuffer **sc_vb)
{
	struct spm_camera_vbuffer *pos = NULL, *n = NULL;
	struct vb2_buffer *vb = NULL;
	unsigned long paddr = 0;

	list_for_each_entry_safe(pos, n, &sc_vnode->busy_list, list_entry) {
		vb = &pos->vb2_v4l2_buf.vb2_buf;
		paddr = spm_vb2_buf_paddr(vb, plane_id);
		if (paddr == plane_paddr) {
			*sc_vb = pos;
			list_del_init(&pos->list_entry);
			atomic_dec(&sc_vnode->busy_buf_cnt);
			return 0;
		}
	}

	return -1;
}
*/
int __spm_vdev_q_busy_vbuffer(struct spm_camera_vnode *sc_vnode, struct spm_camera_vbuffer *sc_vb)
{
	list_add_tail(&sc_vb->list_entry, &sc_vnode->busy_list);
	atomic_inc(&sc_vnode->busy_buf_cnt);
	return 0;
}

int spm_vdev_dq_busy_vbuffer(struct spm_camera_vnode *sc_vnode, struct spm_camera_vbuffer **sc_vb)
{
	unsigned long flags = 0;
	int ret = 0;

	spin_lock_irqsave(&sc_vnode->slock, flags);
	ret = __spm_vdev_dq_busy_vbuffer(sc_vnode, sc_vb);
	spin_unlock_irqrestore(&sc_vnode->slock, flags);
	return ret;
}

int spm_vdev_pick_busy_vbuffer(struct spm_camera_vnode *sc_vnode, struct spm_camera_vbuffer **sc_vb)
{
	unsigned long flags = 0;

	spin_lock_irqsave(&sc_vnode->slock, flags);
	*sc_vb = list_first_entry_or_null(&sc_vnode->busy_list, struct spm_camera_vbuffer, list_entry);
	spin_unlock_irqrestore(&sc_vnode->slock, flags);
	if (NULL == *sc_vb)
		return -1;

	return 0;
}

int __spm_vdev_pick_busy_vbuffer(struct spm_camera_vnode *sc_vnode, struct spm_camera_vbuffer **sc_vb)
{
	*sc_vb = list_first_entry_or_null(&sc_vnode->busy_list, struct spm_camera_vbuffer, list_entry);
	if (NULL == *sc_vb)
		return -1;

	return 0;
}
/*
int spm_vdev_dq_busy_vbuffer_by_paddr(struct spm_camera_vnode *sc_vnode, int plane_id, unsigned long plane_paddr, struct spm_camera_vbuffer **sc_vb)
{
	unsigned long flags = 0;
	int ret = 0;

	spin_lock_irqsave(&sc_vnode->slock, flags);
	ret = __spm_vdev_dq_busy_vbuffer_by_paddr(sc_vnode, plane_id, plane_paddr, sc_vb);
	spin_unlock_irqrestore(&sc_vnode->slock, flags);
	return ret;
}
*/
int spm_vdev_q_busy_vbuffer(struct spm_camera_vnode *sc_vnode, struct spm_camera_vbuffer *sc_vb)
{
	unsigned long flags = 0;
	int ret = 0;

	spin_lock_irqsave(&sc_vnode->slock, flags);
	ret = __spm_vdev_q_busy_vbuffer(sc_vnode, sc_vb);
	spin_unlock_irqrestore(&sc_vnode->slock, flags);
	return ret;
}

/*
int spm_vdev_busy_list_cnt(struct spm_camera_vnode *sc_vnode)
{
	return atomic_read(&sc_vnode->busy_buf_cnt);
}
*/
int spm_vdev_export_camera_vbuffer(struct spm_camera_vbuffer *sc_vb, int with_error)
{
	struct vb2_buffer *vb = &sc_vb->vb2_v4l2_buf.vb2_buf;
	if (with_error)
		vb2_buffer_done(vb, VB2_BUF_STATE_ERROR);
	else
		vb2_buffer_done(vb, VB2_BUF_STATE_DONE);
	return 0;
}

struct spm_camera_vnode *spm_vdev_remote_vnode(struct media_pad *pad)
{
	struct media_pad *remote_pad = media_entity_remote_pad(pad);
	struct media_entity *me = NULL;

	if (!remote_pad)
		return NULL;
	me = remote_pad->entity;
	if (is_subdev(me))
		return NULL;
	return (struct spm_camera_vnode *)me;
}

int spm_vdev_register_vnode_notify(struct spm_camera_vnode *sc_vnode,
				   struct notifier_block *notifier_block)
{
	return blocking_notifier_chain_register(&sc_vnode->notify_chain, notifier_block);
}

int spm_vdev_unregister_vnode_notify(struct spm_camera_vnode *sc_vnode,
				     struct notifier_block *notifier_block)
{
	return blocking_notifier_chain_unregister(&sc_vnode->notify_chain, notifier_block);
}

int __spm_vdev_busy_list_empty(struct spm_camera_vnode *sc_vnode)
{
	return list_empty(&sc_vnode->busy_list);
}

int spm_vdev_busy_list_empty(struct spm_camera_vnode *sc_vnode)
{
	unsigned long flags = 0;
	int ret = 0;

	spin_lock_irqsave(&sc_vnode->slock, flags);
	ret = __spm_vdev_busy_list_empty(sc_vnode);
	spin_unlock_irqrestore(&sc_vnode->slock, flags);
	return ret;
}

int __spm_vdev_idle_list_empty(struct spm_camera_vnode *sc_vnode)
{
	return list_empty(&sc_vnode->queued_list);
}

int spm_vdev_idle_list_empty(struct spm_camera_vnode *sc_vnode)
{
	unsigned long flags = 0;
	int ret = 0;

	spin_lock_irqsave(&sc_vnode->slock, flags);
	ret = __spm_vdev_idle_list_empty(sc_vnode);
	spin_unlock_irqrestore(&sc_vnode->slock, flags);
	return ret;
}

int spm_vdev_is_raw_vnode(struct spm_camera_vnode *sc_vnode)
{
	__u32 pixelformat = sc_vnode->cur_fmt.fmt.pix_mp.pixelformat;

	switch (pixelformat) {
	case V4L2_PIX_FMT_SBGGR8:
	case V4L2_PIX_FMT_SGBRG8:
	case V4L2_PIX_FMT_SGRBG8:
	case V4L2_PIX_FMT_SRGGB8:
	case V4L2_PIX_FMT_SBGGR10:
	case V4L2_PIX_FMT_SGBRG10:
	case V4L2_PIX_FMT_SGRBG10:
	case V4L2_PIX_FMT_SRGGB10:
	case V4L2_PIX_FMT_SBGGR10P:
	case V4L2_PIX_FMT_SGBRG10P:
	case V4L2_PIX_FMT_SGRBG10P:
	case V4L2_PIX_FMT_SRGGB10P:
	case V4L2_PIX_FMT_SBGGR10ALAW8:
	case V4L2_PIX_FMT_SGBRG10ALAW8:
	case V4L2_PIX_FMT_SGRBG10ALAW8:
	case V4L2_PIX_FMT_SRGGB10ALAW8:
	case V4L2_PIX_FMT_SBGGR10DPCM8:
	case V4L2_PIX_FMT_SGBRG10DPCM8:
	case V4L2_PIX_FMT_SGRBG10DPCM8:
	case V4L2_PIX_FMT_SRGGB10DPCM8:
	case V4L2_PIX_FMT_SBGGR12:
	case V4L2_PIX_FMT_SGBRG12:
	case V4L2_PIX_FMT_SGRBG12:
	case V4L2_PIX_FMT_SRGGB12:
	case V4L2_PIX_FMT_SBGGR12P:
	case V4L2_PIX_FMT_SGBRG12P:
	case V4L2_PIX_FMT_SGRBG12P:
	case V4L2_PIX_FMT_SRGGB12P:
	case V4L2_PIX_FMT_SBGGR14P:
	case V4L2_PIX_FMT_SGBRG14P:
	case V4L2_PIX_FMT_SGRBG14P:
	case V4L2_PIX_FMT_SRGGB14P:
	case V4L2_PIX_FMT_SBGGR16:
	case V4L2_PIX_FMT_SGBRG16:
	case V4L2_PIX_FMT_SGRBG16:
	case V4L2_PIX_FMT_SRGGB16:
	case V4L2_PIX_FMT_KYGB8P:
	case V4L2_PIX_FMT_KYGB10P:
	case V4L2_PIX_FMT_KYGB12P:
	case V4L2_PIX_FMT_KYGB14P:
		return 1;
		break;
	default:
		return 0;
	}
	return 0;
}
