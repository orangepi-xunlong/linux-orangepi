
/*
 ******************************************************************************
 *
 * bsp_common.h
 *
 * Hawkview ISP - bsp_common.h module
 *
 * Copyright (c) 2015 by Allwinnertech Co., Ltd.  http://www.allwinnertech.com
 *
 * Version		  Author         Date		    Description
 *
 *   3.0		  Yang Feng   	2015/12/02	ISP Tuning Tools Support
 *
 ******************************************************************************
 */

#ifndef __BSP_COMMON__H__
#define __BSP_COMMON__H__

#include <linux/videodev2.h>
#include <media/v4l2-mediabus.h>

enum bus_pixeltype {
	BUS_FMT_RGB565,
	BUS_FMT_RGB888,
	BUS_FMT_Y_U_V,
	BUS_FMT_YY_YUYV,
	BUS_FMT_YY_YVYU,
	BUS_FMT_YY_UYVY,
	BUS_FMT_YY_VYUY,
	BUS_FMT_YUYV,
	BUS_FMT_YVYU,
	BUS_FMT_UYVY,
	BUS_FMT_VYUY,
	BUS_FMT_SBGGR,
	BUS_FMT_SGBRG,
	BUS_FMT_SGRBG,
	BUS_FMT_SRGGB,
};

enum pixel_fmt_type {
	RGB565,
	RGB888,
	PRGB888,
	YUV422_INTLVD,
	YUV422_PL,
	YUV422_SPL,
	YUV422_MB,
	YUV420_PL,
	YUV420_SPL,
	YUV420_MB,
	BAYER_RGB,
};

enum bit_width {
	W_1BIT,
	W_2BIT,
	W_4BIT,
	W_6BIT,
	W_8BIT,
	W_10BIT,
	W_12BIT,
	W_14BIT,
	W_16BIT,
	W_20BIT,
	W_24BIT,
	W_32BIT,
};

extern enum bus_pixeltype find_bus_type(enum v4l2_mbus_pixelcode code);
extern enum bit_width find_bus_width(enum v4l2_mbus_pixelcode code);
extern enum bit_width find_bus_precision(enum v4l2_mbus_pixelcode code);
extern enum pixel_fmt_type find_pixel_fmt_type(unsigned int pix_fmt);

#endif /*__BSP_COMMON__H__*/
