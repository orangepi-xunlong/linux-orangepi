// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2021-2021, The Linux Foundation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */
#include "armcb_platform.h"
#include "armcb_camera_io_drv.h"
#include "armcb_register.h"
#include "isp_hw_utils.h"
#include "system_logger.h"

#ifdef LOG_MODULE
#undef LOG_MODULE
#define LOG_MODULE LOG_MODULE_COMMON
#endif

static u32 g_Global_InitDone;

void BSP_Init(void)
{
	u32 regval = 0;

	// Sensor Hardware Power On
	armcb_apb2_write_reg(0x04, 7);

	// ISP Reset
	regval = armcb_apb2_read_reg(0x0);
	regval |= (0x1 << 24);
	armcb_apb2_write_reg(0x0, regval);

	mdelay(1);
	regval &= ~(0x1 << 24);
	armcb_apb2_write_reg(0x0, regval);
	regval = armcb_apb2_read_reg(0x0);
	LOG(LOG_DEBUG, "Readback APB2_REG_BASE[0x83c40000] value[%x]", regval);

	// I2C Initialize init i2c1 bus
	armcb_route_i2c1toslavex(0xA);

	// Connect hw interface0
	armcb_hwchnnel_select(0);

	// SPI
	armcb_spi_set_hwchnl(0); // TODO: add motor driver sometime after

// HDMI Initialize
#ifdef CONFIG_ARENA_FPGA_PLATFORM
	armcb_hdmi_init(RES1080P30FPS, RES1080P60FPS, HDMIOUT_RGB444_8BIT);
#endif
	// Select Source
	regval = armcb_apb2_read_reg(0x0);
	regval |= (0x01 << 1);
	armcb_apb2_write_reg(0x0, regval);

	// Select RGB Order
	regval = armcb_apb2_read_reg(0x0);
	regval |= (0x01 << 25);
	armcb_apb2_write_reg(0x0, regval);
}

/*
 * Function name: armcb_fpga_hwcfg_init()
 * Description:   XCVU440 FPGA board init config for camera.
 * Parameters: void
 * initialize i2c-bus configution
 * initialize isp configution
 * initialize camera sensor configution
 * initialize isp-csi mipi configution
 * initialize interrupt configution
 *
 */
s32 armcb_fpga_hwcfg_init(void)
{
	u32 unDDRRdyFlagAddr = XPAR_REGCTRL16_0_S00_AXI_BASEADDR + 0x20;

	LOG(LOG_DEBUG, "+");
	if (!g_Global_InitDone) {
		BSP_Init();

		LOG(LOG_INFO, "DDR[0x%x] value %d", unDDRRdyFlagAddr,
		    armcb_register_get_int32(unDDRRdyFlagAddr));
		LOG(LOG_INFO, "Wait xcvu440 to be ready...");

		g_Global_InitDone = 1;
	} else {
		LOG(LOG_INFO, "BSP_Init already done!");
	}
	LOG(LOG_DEBUG, "-");
	return 0;
}
