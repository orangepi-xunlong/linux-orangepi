/* SPDX-License-Identifier: GPL-2.0 */
/*
 * TODO: V4L2-Userspace
 *
 */
#ifndef __ARMCB_PLATFORM_H__
#define __ARMCB_PLATFORM_H__
#include <linux/delay.h>

/* Definitions for peripheral AXI_CDMA_0 */
#define XPAR_AXI_CDMA_0_BASEADDR 0x8E200000
#define XPAR_AXI_CDMA_0_HIGHADDR 0x8E20FFFF

/* Definitions for peripheral REGCTRL16_0 */
#define XPAR_REGCTRL16_0_S00_AXI_BASEADDR 0x43C00000

// Resolution
enum {
	RES720P30FPS = 0x01,
	RES720P60FPS,
	RES1080P30FPS,
	RES1080P60FPS,
	RES1080I50FPS,
	RES1080P50FPS,
	RES960P30FPS,
	RES_TOTAL_SIZE,
};

s32 armcb_fpga_hwcfg_init(void);
#endif
