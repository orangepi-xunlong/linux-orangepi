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

#include "isp_hw_ops.h"
#include "armcb_camera_io_drv.h"
#include "armcb_isp_driver.h"
#include "armcb_platform.h"
#include "armcb_register.h"
#include "system_dma.h"
#include "system_logger.h"

#include "cix_vi_hw.h"

#ifdef LOG_MODULE
#undef LOG_MODULE
#define LOG_MODULE LOG_MODULE_COMMON
#endif

#define SOF_TIMEOUT (2000)

static unsigned char is_stream_on_list_init;
static unsigned char is_stream_off_list_init;
static unsigned char is_stream_powerdown_list_init;
static unsigned int hw_apply_entry_cnt;
static unsigned int hw_apply_cam_flag;
static struct list_head streamon_list;
static struct list_head streamoff_list;
static struct list_head powerdown_list;

static unsigned int count_bits(unsigned int n)
{
	unsigned int count = 0;

	while (n) {
		count += n & 1;
		n >>= 1;
	}
	return count;
}

/// Apply I2C cmd
static int armcb_i2c_apply(struct cmd_buf *cmd, void *clinet)
{
	int i = 0;
	int ret = 0;
	struct cmd_i2c_setting *i2c_settings = NULL;

	if (!cmd || !clinet || 0 == cmd->cmd_cnt) {
		LOG(LOG_ERR, "input parameters is invalid, %p %p %d", cmd,
		    clinet, cmd->cmd_cnt);
		return -EINVAL;
	}

	i2c_settings = cmd->settings.i2c;

	for (i = 0; i < cmd->cmd_cnt; i++) {
		if (i2c_settings[i].reg_addr_type == DRV_ADDR_TYPE_BYTE ||
		    i2c_settings[i].reg_addr_type == DRV_ADDR_TYPE_WORD ||
		    i2c_settings[i].reg_addr_type ==
			    DRV_ADDR_TYPE_WORD_REVERSE) {
		} else {
			LOG(LOG_ERR, "Input register address type not support");
			break;
		}

		if (i2c_settings[i].direct == DRV_DIRECTION_WRITE) {
			if (i2c_settings[i].reg_data_type ==
			    DRV_DATA_TYPE_BYTE) {
				i2c_settings[i].val &= 0xFF;
				/* The linux i2c driver will adjust i2c data reverse, so we don't need
				 * to deal with it
				 */
			} else if (i2c_settings[i].reg_data_type ==
					   DRV_DATA_TYPE_WORD ||
				   i2c_settings[i].reg_data_type ==
					   DRV_DATA_TYPE_WORD_REVERSE) {
				i2c_settings[i].val &= 0xFFFF;
			} else {
				LOG(LOG_ERR,
				    "Input register data type not support");
				ret = -EINVAL;
				break;
			}
			ret = armcb_i2c_register_write(
				(struct i2c_client *)clinet, &i2c_settings[i]);
			if (ret < 0) {
				LOG(LOG_ERR,
				    "I2C Write index[%d] reg_addr:0x%x reg_data:0x%x failed, ret = %d",
				    i, i2c_settings[i].reg_addr,
				    i2c_settings[i].val, ret);
				break;
			}

			LOG(LOG_DEBUG,
			    "I2C Write index[%d]  slave_addr 0x%x ,reg_addr:0x%x reg_data:0x%x "
			    "ch:%d delay_us %d ",
			    i, i2c_settings[i].slave_addr,
			    i2c_settings[i].reg_addr, i2c_settings[i].val,
			    i2c_settings[i].channel, i2c_settings[i].delay_us);
		} else if (i2c_settings[i].direct == DRV_DIRECTION_READ) {
			ret = armcb_i2c_register_read(
				(struct i2c_client *)clinet, &i2c_settings[i]);
			if (ret < 0 && CMD_TYPE_PROBE != cmd->cmd_type) {
				LOG(LOG_ERR,
				    " I2C Read failed index[%d slave_addr 0x%x reg_addr:0x%x "
				    "reg_data:0x%x ch:%d ",
				    i, i2c_settings[i].slave_addr,
				    i2c_settings[i].reg_addr,
				    i2c_settings[i].val,
				    i2c_settings[i].channel);
				break;
			}

			if (i2c_settings[i].reg_data_type ==
			    DRV_DATA_TYPE_BYTE) {
				i2c_settings[i].val &= 0xFF;
			} else if (i2c_settings[i].reg_data_type ==
					   DRV_DATA_TYPE_WORD ||
				   i2c_settings[i].reg_data_type ==
					   DRV_DATA_TYPE_WORD_REVERSE) {
				i2c_settings[i].val &= 0xFFFF;
			} else {
				LOG(LOG_ERR,
				    "Input register data type not support");
				break;
			}

			LOG(LOG_DEBUG,
			    "[RegDebug] I2C Read index[%d]  slave_addr 0x%x ,reg_addr:0x%x "
			    "reg_data:0x%x ch:%d delay_us %d ",
			    i, i2c_settings[i].slave_addr,
			    i2c_settings[i].reg_addr, i2c_settings[i].val,
			    i2c_settings[i].channel, i2c_settings[i].delay_us);
		} else {
			LOG(LOG_ERR, "Unsupported direct");
		}

		if (i2c_settings[i].delay_us > 0)
			usleep_range(i2c_settings[i].delay_us,
				     i2c_settings[i].delay_us + 50);
	}

	return ret;
}

/// Apply SPI cmd
static int armcb_spi_apply(struct cmd_buf *cmd, void *clinet)
{
	int i = 0;
	int ret = 0;
	struct cmd_spi_setting *spi_settings = NULL;

	if (WARN_ON(!cmd) || WARN_ON(!clinet) || WARN_ON(cmd->cmd_cnt == 0)) {
		LOG(LOG_ERR, "input parameters is invalid ");
		return -EINVAL;
	}

	spi_settings = cmd->settings.spi;

	for (i = 0; i < cmd->cmd_cnt; i++) {
		if (spi_settings[i].reg_addr_type == DRV_ADDR_TYPE_BYTE) {
			spi_settings[i].reg_addr &= 0xFF;
		} else {
			LOG(LOG_ERR,
			    "Input register address type %u not support",
			    spi_settings[i].reg_addr_type);
			ret = -EINVAL;
			break;
		}

		if (spi_settings[i].direct == DRV_DIRECTION_WRITE) {
			if (spi_settings[i].reg_data_type ==
			    DRV_DATA_TYPE_WORD) {
				spi_settings[i].val &= 0xFFFF;
			} else {
				LOG(LOG_ERR,
				    "Input register data type %u not support",
				    spi_settings[i].reg_data_type);
				ret = -EINVAL;
				break;
			}
			ret = armcb_spi_register_write(
				(struct spi_device *)clinet, &spi_settings[i]);
			if (ret < 0) {
				LOG(LOG_ERR,
				    "SPI Write index[%d] reg_addr:0x%x reg_data:0x%x failed, ret = %d",
				    i, spi_settings[i].reg_addr,
				    spi_settings[i].val, ret);
				break;
			}

			LOG(LOG_DEBUG,
			    "SPI Write index[%d] reg_addr:0x%x reg_data:0x%x ch:%d delay_us %d",
			    i, spi_settings[i].reg_addr, spi_settings[i].val,
			    spi_settings[i].channel, spi_settings[i].delay_us);
		} else if (spi_settings[i].direct == DRV_DIRECTION_READ) {
			ret = armcb_spi_register_read(
				(struct spi_device *)clinet, &spi_settings[i]);
			if (ret < 0) {
				LOG(LOG_ERR,
				    " SPI Read index[%d reg_addr:0x%x reg_data:0x%x failed",
				    i, spi_settings[i].reg_addr,
				    spi_settings[i].val);
				break;
			}

			if (spi_settings[i].reg_data_type ==
			    DRV_DATA_TYPE_WORD) {
				spi_settings[i].val &= 0xFF;
			} else {
				LOG(LOG_ERR,
				    "Input register data type %u not support",
				    spi_settings[i].reg_data_type);
				break;
			}

			LOG(LOG_DEBUG,
			    "[RegDebug] SPI Read index[%d] reg_addr:0x%x reg_data:0x%x ch:%d "
			    "delay_us %d ",
			    i, spi_settings[i].reg_addr, spi_settings[i].val,
			    spi_settings[i].channel, spi_settings[i].delay_us);
		} else {
			LOG(LOG_ERR, "Unsupported direct");
		}

		if (spi_settings[i].delay_us > 0)
			usleep_range(spi_settings[i].delay_us,
				     spi_settings[i].delay_us + 50);
	}

	return ret;
}

/// Apply AHB cmd
static int armcb_ahb_apply(struct cmd_buf *cmd, void *clinet)
{
	int i = 0;
	int ret = 0;
	struct cmd_ahb_setting *ahb_settings = NULL;
	unsigned int delay_us = 0;
	unsigned int expected_val = 0;

	if (!cmd || !clinet || !cmd->cmd_cnt) {
		LOG(LOG_ERR, "input parameters is invalid, %p %p %d", cmd,
		    clinet, cmd->cmd_cnt);
		return -EINVAL;
	}

	ahb_settings = cmd->settings.ahb;

	for (i = 0; i < cmd->cmd_cnt; i++) {
		if (ahb_settings[i].direct == DRV_DIRECTION_WRITE) {
#ifdef CONFIG_ARENA_FPGA_PLATFORM
			if (ahb_settings[i].reg_addr >=
			    XPAR_AXI_CDMA_0_BASEADDR)
				CDMA_Write_Int32(
					ahb_settings[i].reg_addr -
						XPAR_AXI_CDMA_0_BASEADDR,
					ahb_settings[i].val);
			else if (ahb_settings[i].reg_addr >= ISP_GDC_REG_BASE)
				armcb_isp_write_reg2(ahb_settings[i].reg_addr -
							     ISP_GDC_REG_BASE,
						     ahb_settings[i].val);
			else if (ahb_settings[i].reg_addr >= VDMA_REG_BASE)
				VDMA_Write_Int32(VDMA_REG_BASE,
						 ahb_settings[i].reg_addr -
							 VDMA_REG_BASE,
						 ahb_settings[i].val);
#endif
#ifdef ARMCB_CAM_AHB
			if ((ahb_settings[i].reg_addr >=
					AHB_CSIRCSU0_REG_BASE) &&
				(ahb_settings[i].reg_addr <=
					AHB_CSIRCSU0_REG_END)) {
				cix_ahb_csircsu_write_reg(
					ahb_settings[i].reg_addr,
					ahb_settings[i].val);
			} else if ((ahb_settings[i].reg_addr >=
				    AHB_CSI0_REG_BASE) &&
				   (ahb_settings[i].reg_addr <=
				    AHB_CSI1_REG_END)) {
				cix_ahb_csi_write_reg(ahb_settings[i].reg_addr,
						      ahb_settings[i].val);
			} else if ((ahb_settings[i].reg_addr >=
				    AHB_DPHY0_REG_BASE) &&
				   (ahb_settings[i].reg_addr <=
				    AHB_DPHY0_REG_END)) {
				cix_ahb_dphy_write_reg(ahb_settings[i].reg_addr,
						       ahb_settings[i].val);
			} else if ((ahb_settings[i].reg_addr >=
				    AHB_CSIDMA0_REG_BASE) &&
				   (ahb_settings[i].reg_addr <=
				    AHB_CSIDMA1_REG_END)) {
				cix_ahb_csidma_write_reg(
					ahb_settings[i].reg_addr,
					ahb_settings[i].val);
			} else if ((ahb_settings[i].reg_addr >=
				    AHB_CSIRCSU1_REG_BASE) &&
				   (ahb_settings[i].reg_addr <=
				    AHB_CSIRCSU1_REG_END)) {
				cix_ahb_csircsu_write_reg(
					ahb_settings[i].reg_addr,
					ahb_settings[i].val);
			} else if ((ahb_settings[i].reg_addr >=
				    AHB_CSI2_REG_BASE) &&
				   (ahb_settings[i].reg_addr <=
				    AHB_CSI3_REG_END)) {
				cix_ahb_csi_write_reg(ahb_settings[i].reg_addr,
						      ahb_settings[i].val);
			} else if ((ahb_settings[i].reg_addr >=
				    AHB_DPHY1_REG_BASE) &&
				   (ahb_settings[i].reg_addr <=
				    AHB_DPHY1_REG_END)) {
				cix_ahb_dphy_write_reg(ahb_settings[i].reg_addr,
						       ahb_settings[i].val);
			} else if ((ahb_settings[i].reg_addr >=
				    AHB_CSIDMA2_REG_BASE) &&
				   (ahb_settings[i].reg_addr <=
				    AHB_CSIDMA3_REG_END)) {
				cix_ahb_csidma_write_reg(
					ahb_settings[i].reg_addr,
					ahb_settings[i].val);
			} else if ((ahb_settings[i].reg_addr >=
				    AHB_PMCTRL_RES_REG_BASE) &&
				   (ahb_settings[i].reg_addr <=
				    AHB_PMCTRL_RES_REG_END)) {
				armcb_ahb_pmctrl_res_write_reg(
					ahb_settings[i].reg_addr -
						AHB_PMCTRL_RES_REG_BASE,
					ahb_settings[i].val);
			} else if ((ahb_settings[i].reg_addr >=
				    DPHY_POWER_BASE) &&
				   (ahb_settings[i].reg_addr <=
				    DPHY_POWER_END)) {
				cix_enable_dphy_clk(ahb_settings[i].reg_addr,
						    ahb_settings[i].val);
			} else if ((ahb_settings[i].reg_addr >=
				    CSI_POWER_BASE) &&
				   (ahb_settings[i].reg_addr <=
				    CSI_POWER_END)) {
				cix_set_csi_clk_rate(ahb_settings[i].reg_addr,
						     ahb_settings[i].val);
			} else if ((ahb_settings[i].reg_addr >=
				    CSIDMA_POWER_BASE) &&
				   (ahb_settings[i].reg_addr <=
				    CSIDMA_POWER_END)) {
				cix_enable_csidma_clk(ahb_settings[i].reg_addr,
						      ahb_settings[i].val);
			} else if (ahb_settings[i].reg_addr >=
				   ISP_GDC_REG_BASE) {
				armcb_isp_write_reg2(ahb_settings[i].reg_addr -
							     ISP_GDC_REG_BASE,
						     ahb_settings[i].val);
			}
#else
			if (ahb_settings[i].reg_addr >= APB2_REG_BASE)
				armcb_apb2_write_reg(ahb_settings[i].reg_addr -
							     APB2_REG_BASE,
						     ahb_settings[i].val);
#endif
			else
				armcb_isp_write_reg(ahb_settings[i].reg_addr -
							    ISP_REG_BASE,
						    ahb_settings[i].val);

			if (ahb_settings[i].delay_us > 0)
				usleep_range(ahb_settings[i].delay_us,
					     ahb_settings[i].delay_us + 50);
		} else if (ahb_settings[i].direct == DRV_DIRECTION_READ) {
#ifdef CONFIG_ARENA_FPGA_PLATFORM
			if (ahb_settings[i].reg_addr >=
			    XPAR_AXI_CDMA_0_BASEADDR)
				ahb_settings[i].val = CDMA_Read_Int32(
					ahb_settings[i].reg_addr -
					XPAR_AXI_CDMA_0_BASEADDR);
			else if (ahb_settings[i].reg_addr >= ISP_GDC_REG_BASE)
				ahb_settings[i].val = armcb_isp_read_reg2(
					ahb_settings[i].reg_addr -
					ISP_GDC_REG_BASE);
			else if (ahb_settings[i].reg_addr >= VDMA_REG_BASE)
				ahb_settings[i].val = VDMA_Read_Int32(
					VDMA_REG_BASE,
					ahb_settings[i].reg_addr -
						VDMA_REG_BASE);
#endif
#ifdef ARMCB_CAM_AHB
			if ((ahb_settings[i].reg_addr >=
					AHB_CSIRCSU0_REG_BASE) &&
				(ahb_settings[i].reg_addr <=
					AHB_CSIRCSU0_REG_END)) {
				ahb_settings[i].val = cix_ahb_csircsu_read_reg(
				ahb_settings[i].reg_addr);
			} else if ((ahb_settings[i].reg_addr >=
				    AHB_CSI0_REG_BASE) &&
				   (ahb_settings[i].reg_addr <=
				    AHB_CSI1_REG_END)) {
				ahb_settings[i].val = cix_ahb_csi_read_reg(
					ahb_settings[i].reg_addr);
			} else if ((ahb_settings[i].reg_addr >=
				    AHB_DPHY0_REG_BASE) &&
				   (ahb_settings[i].reg_addr <=
				    AHB_DPHY0_REG_END)) {
				ahb_settings[i].val = cix_ahb_dphy_read_reg(
					ahb_settings[i].reg_addr);
			} else if ((ahb_settings[i].reg_addr >=
				    AHB_CSIDMA0_REG_BASE) &&
				   (ahb_settings[i].reg_addr <=
				    AHB_CSIDMA1_REG_END)) {
				ahb_settings[i].val = cix_ahb_csidma_read_reg(
					ahb_settings[i].reg_addr);
			} else if ((ahb_settings[i].reg_addr >=
				    AHB_CSIRCSU1_REG_BASE) &&
				   (ahb_settings[i].reg_addr <=
				    AHB_CSIRCSU1_REG_END)) {
				ahb_settings[i].val = cix_ahb_csircsu_read_reg(
					ahb_settings[i].reg_addr);
			} else if ((ahb_settings[i].reg_addr >=
				    AHB_CSI2_REG_BASE) &&
				   (ahb_settings[i].reg_addr <=
				    AHB_CSI3_REG_END)) {
				ahb_settings[i].val = cix_ahb_csi_read_reg(
					ahb_settings[i].reg_addr);
			} else if ((ahb_settings[i].reg_addr >=
				    AHB_DPHY1_REG_BASE) &&
				   (ahb_settings[i].reg_addr <=
				    AHB_DPHY1_REG_END)) {
				ahb_settings[i].val = cix_ahb_dphy_read_reg(
					ahb_settings[i].reg_addr);
			} else if ((ahb_settings[i].reg_addr >=
				    AHB_CSIDMA2_REG_BASE) &&
				   (ahb_settings[i].reg_addr <=
				    AHB_CSIDMA3_REG_END)) {
				ahb_settings[i].val = cix_ahb_csidma_read_reg(
					ahb_settings[i].reg_addr);
			} else if ((ahb_settings[i].reg_addr >=
				    AHB_PMCTRL_RES_REG_BASE) &&
				   (ahb_settings[i].reg_addr <=
				    AHB_PMCTRL_RES_REG_END)) {
				ahb_settings[i].val =
					armcb_ahb_pmctrl_res_read_reg(
						ahb_settings[i].reg_addr -
						AHB_PMCTRL_RES_REG_BASE);
			} else if (ahb_settings[i].reg_addr >=
				   ISP_GDC_REG_BASE) {
				ahb_settings[i].val = armcb_isp_read_reg2(
					ahb_settings[i].reg_addr -
					ISP_GDC_REG_BASE);
			}
#else
			if (ahb_settings[i].reg_addr >= APB2_REG_BASE)
				ahb_settings[i].val = armcb_apb2_read_reg(
					ahb_settings[i].reg_addr -
					APB2_REG_BASE);
#endif
			else
				ahb_settings[i].val = armcb_isp_read_reg(
					ahb_settings[i].reg_addr -
					ISP_REG_BASE);

			if (ahb_settings[i].delay_us > 0)
				usleep_range(ahb_settings[i].delay_us,
					     ahb_settings[i].delay_us + 50);
		} else if (ahb_settings[i].direct == DRV_DIRECTION_READ_POLL) {
			expected_val = ahb_settings[i].val;
			while (1) {
#ifdef CONFIG_ARENA_FPGA_PLATFORM
				if (ahb_settings[i].reg_addr >=
				    XPAR_AXI_CDMA_0_BASEADDR)
					ahb_settings[i].val = CDMA_Read_Int32(
						ahb_settings[i].reg_addr -
						XPAR_AXI_CDMA_0_BASEADDR);
				else if (ahb_settings[i].reg_addr >=
					 VDMA_REG_BASE)
					ahb_settings[i].val = VDMA_Read_Int32(
						VDMA_REG_BASE,
						ahb_settings[i].reg_addr -
							VDMA_REG_BASE);
#endif
#ifdef ARMCB_CAM_AHB
				if ((ahb_settings[i].reg_addr >=
						AHB_CSIRCSU0_REG_BASE) &&
					(ahb_settings[i].reg_addr <
						AHB_CSIRCSU0_REG_END)) {
					ahb_settings[i]
						.val = cix_ahb_csircsu_read_reg(
						ahb_settings[i].reg_addr);
				} else if ((ahb_settings[i].reg_addr >=
					    AHB_CSI0_REG_BASE) &&
					   (ahb_settings[i].reg_addr <
					    AHB_CSI1_REG_END)) {
					ahb_settings[i]
						.val = cix_ahb_csi_read_reg(
						ahb_settings[i].reg_addr);
				} else if ((ahb_settings[i].reg_addr >=
					    AHB_DPHY0_REG_BASE) &&
					   (ahb_settings[i].reg_addr <
					    AHB_DPHY0_REG_END)) {
					ahb_settings[i]
						.val = cix_ahb_dphy_read_reg(
						ahb_settings[i].reg_addr);
				} else if ((ahb_settings[i].reg_addr >=
					    AHB_CSIDMA0_REG_BASE) &&
					   (ahb_settings[i].reg_addr <
					    AHB_CSIDMA1_REG_END)) {
					ahb_settings[i]
						.val = cix_ahb_csidma_read_reg(
						ahb_settings[i].reg_addr);
				} else if ((ahb_settings[i].reg_addr >=
					    AHB_CSIRCSU1_REG_BASE) &&
					   (ahb_settings[i].reg_addr <
					    AHB_CSIRCSU1_REG_END)) {
					ahb_settings[i]
						.val = cix_ahb_csircsu_read_reg(
						ahb_settings[i].reg_addr);
				} else if ((ahb_settings[i].reg_addr >=
					    AHB_CSI2_REG_BASE) &&
					   (ahb_settings[i].reg_addr <
					    AHB_CSI3_REG_END)) {
					ahb_settings[i]
						.val = cix_ahb_csi_read_reg(
						ahb_settings[i].reg_addr);
				} else if ((ahb_settings[i].reg_addr >=
					    AHB_DPHY1_REG_BASE) &&
					   (ahb_settings[i].reg_addr <
					    AHB_DPHY1_REG_END)) {
					ahb_settings[i]
						.val = cix_ahb_dphy_read_reg(
						ahb_settings[i].reg_addr);
				} else if ((ahb_settings[i].reg_addr >=
					    AHB_CSIDMA2_REG_BASE) &&
					   (ahb_settings[i].reg_addr <
					    AHB_CSIDMA3_REG_END)) {
					ahb_settings[i]
						.val = cix_ahb_csidma_read_reg(
						ahb_settings[i].reg_addr);
				} else if ((ahb_settings[i].reg_addr >=
					    AHB_PMCTRL_RES_REG_BASE) &&
					   (ahb_settings[i].reg_addr <
					    AHB_PMCTRL_RES_REG_END)) {
					ahb_settings[i].val =
						armcb_ahb_pmctrl_res_read_reg(
							ahb_settings[i].reg_addr -
							AHB_PMCTRL_RES_REG_BASE);
				}
#else
				if (ahb_settings[i].reg_addr >= APB2_REG_BASE)
					ahb_settings[i]
						.val = armcb_apb2_read_reg(
						ahb_settings[i].reg_addr -
						APB2_REG_BASE);
#endif
				else
					ahb_settings[i].val = armcb_isp_read_reg(
						ahb_settings[i].reg_addr -
						ISP_REG_BASE);

				if (expected_val == ahb_settings[i].val)
					break;
				else
					LOG(LOG_DEBUG,
					    "Wait 0x%x, expected 0x%x, now 0x%x !",
					    ahb_settings[i].reg_addr,
					    expected_val, ahb_settings[i].val);

				if (delay_us >= ahb_settings[i].delay_us) {
					LOG(LOG_WARN,
					    "Wait 0x%x timeout, expected 0x%x, final 0x%x !",
					    ahb_settings[i].reg_addr,
					    expected_val, ahb_settings[i].val);
					break;
				}

				usleep_range(2000, 2000 + 50);
				delay_us += 2000;
			}
		} else {
			LOG(LOG_ERR, "Unsupported direct");
		}
	}

	return ret;
}

/// Apply AHB Power cmd
static int armcb_ahb_power_apply(struct cmd_buf *cmd, void *clinet)
{
	int i = 0;
	int ret = 0;
	struct cmd_ahb_power_setting *ahb_power_settings = NULL;
	u32 tmp = 0;

	if (!cmd || !clinet || !cmd->cmd_cnt) {
		LOG(LOG_ERR, "input parameters is invalid %p %p %u", cmd,
		    clinet, cmd->cmd_cnt);
		return -EINVAL;
	}

	ahb_power_settings = cmd->settings.ahb_power;

	for (i = 0; i < cmd->cmd_cnt; i++) {
		if (ahb_power_settings[i].reg_addr >= APB2_REG_BASE &&
		    ahb_power_settings[i].reg_addr < XPAR_AXI_CDMA_0_BASEADDR) {
			tmp = armcb_apb2_read_reg(
				ahb_power_settings[i].reg_addr - APB2_REG_BASE);
			if (ahb_power_settings[i].bit_val == 0)
				tmp &= (~ahb_power_settings[i].bit_mask);
			else
				tmp |= ahb_power_settings[i].bit_mask;
			armcb_apb2_write_reg(ahb_power_settings[i].reg_addr -
						     APB2_REG_BASE,
					     tmp);

			if (ahb_power_settings[i].delay_us > 0)
				usleep_range(ahb_power_settings[i].delay_us,
					     ahb_power_settings[i].delay_us +
						     50);
		} else {
			LOG(LOG_ERR, "invaild address 0x%x",
			    ahb_power_settings[i].reg_addr);
			return -EINVAL;
		}
	}

	return ret;
}

/// Process ISP HW cmd
int armcb_isp_hw_apply_list(enum cmd_type type)
{
	int ret = 0;
	struct isp_hw_cmd_buf *hw_cmd_buf = NULL;
	struct isp_hw_cmd_buf *hw_cmd_buf_next = NULL;
	struct list_head *cmd_list = NULL;

	switch (type) {
	case CMD_TYPE_STREAMON:
		hw_apply_entry_cnt--;
		if (is_stream_on_list_init && !hw_apply_entry_cnt) {
			cmd_list = &streamon_list;
			is_stream_on_list_init = 0;
			hw_apply_cam_flag = 0;
		}
		break;
	case CMD_TYPE_STREAMOFF:
		if (is_stream_off_list_init) {
			cmd_list = &streamoff_list;
			is_stream_off_list_init = 0;
		}
		break;
	case CMD_TYPE_POWERDOWN:
		if (is_stream_powerdown_list_init) {
			cmd_list = &powerdown_list;
			is_stream_powerdown_list_init = 0;
		}
		break;
	default:
		break;
	}

	if (cmd_list) {
		list_for_each_entry_safe(hw_cmd_buf, hw_cmd_buf_next, cmd_list,
					  list) {
			if (hw_cmd_buf && hw_cmd_buf->cmd) {
				LOG_RATELIMITED(
					LOG_DEBUG,
					"cam %u, cmd_type %u, order %u, bus %u, dev %u, size %u, addr %p",
					hw_cmd_buf->cmd->static_info.camId,
					hw_cmd_buf->cmd->cmd_type,
					hw_cmd_buf->cmd->order,
					hw_cmd_buf->cmd->static_info.bus,
					hw_cmd_buf->cmd->static_info.dev,
					hw_cmd_buf->cmd->static_info.buf_size,
					hw_cmd_buf);

				switch (hw_cmd_buf->cmd->static_info.bus) {
				case HW_BUS_I2C:
#ifndef QEMU_ON_VEXPRESS
					ret = armcb_i2c_apply(
						hw_cmd_buf->cmd,
						hw_cmd_buf->client);
#endif
					break;
				case HW_BUS_SPI:
#ifndef QEMU_ON_VEXPRESS
					ret = armcb_spi_apply(
						hw_cmd_buf->cmd,
						hw_cmd_buf->client);
#endif
					break;
				case HW_BUS_AHB:
#ifndef QEMU_ON_VEXPRESS
					ret = armcb_ahb_apply(
						hw_cmd_buf->cmd,
						hw_cmd_buf->client);
#endif
					break;
				case HW_BUS_AHB_POWER:
#ifndef QEMU_ON_VEXPRESS
					ret = armcb_ahb_power_apply(
						hw_cmd_buf->cmd,
						hw_cmd_buf->client);
#endif
					break;
				case HW_BUS_DMA_REG:
				case HW_BUS_XDMA_REG:
				case HW_BUS_DMA_SRAM:
				case HW_BUS_XDMA_SRAM:
					hw_cmd_buf->cmd->cmd_cnt =
						MCFB_TRIG_MAX;
#ifndef QEMU_ON_VEXPRESS
					ret = armcb_ahb_apply(
						hw_cmd_buf->cmd,
						hw_cmd_buf->client);
#endif
					break;
				default:
					LOG(LOG_WARN, "not support hw bus %d!",
					    hw_cmd_buf->cmd->static_info.bus);
					ret = -EINVAL;
					break;
				}

				list_del(&hw_cmd_buf->list);

				kfree(hw_cmd_buf->cmd);
				hw_cmd_buf->cmd = NULL;
				kfree(hw_cmd_buf);
				hw_cmd_buf = NULL;
			}
		}
	}

	return ret;
}

/// Process ISP HW cmd
int armcb_isp_hw_apply(struct cmd_buf *cmd, void *client)
{
	int ret = 0;
	struct isp_hw_cmd_buf *p_isp_hw_cmd_buf = NULL;

	if (!cmd) {
		LOG(LOG_ERR, "input parameters is invalid.");
		return -EINVAL;
	}

	LOG(LOG_DEBUG,
	    "info: camId %u, dev %u, bus %u, triggerCond %u, maxCmdCnt %u, maxBufNum "
	    "%u, sName:%s, cmd:%p",
	    cmd->static_info.camId, cmd->static_info.dev, cmd->static_info.bus,
	    cmd->static_info.trigger_cond, cmd->static_info.max_cmd_cnt,
	    cmd->static_info.max_buf_num, cmd->static_info.sName, cmd);

	LOG(LOG_DEBUG,
	    "cmdType %u, applyFrameId %u, effectFrameId %u, cmdCnt %u",
	    cmd->cmd_type, cmd->apply_frame_id, cmd->effect_frame_id,
	    cmd->cmd_cnt);

	if (cmd->cmd_type == CMD_TYPE_STREAMON ||
	    cmd->cmd_type == CMD_TYPE_POST_STREAMON ||
	    cmd->cmd_type == CMD_TYPE_STREAMOFF ||
	    cmd->cmd_type == CMD_TYPE_POWERDOWN) {
		p_isp_hw_cmd_buf = kmalloc(
			sizeof(struct isp_hw_cmd_buf), GFP_KERNEL);
		if (p_isp_hw_cmd_buf == NULL) {
			LOG(LOG_ERR, "failed to kmalloc memory!");
			return -ENOMEM;
		}

		p_isp_hw_cmd_buf->cmd = kmalloc(
			cmd->static_info.buf_size, GFP_KERNEL);
		if (p_isp_hw_cmd_buf->cmd == NULL) {
			LOG(LOG_ERR, "failed to kmalloc memory!");
			return -ENOMEM;
		}

		memcpy(p_isp_hw_cmd_buf->cmd, cmd, cmd->static_info.buf_size);

		p_isp_hw_cmd_buf->client = client;

		LOG(LOG_DEBUG,
		    "cam %u, cmd_type %u, cnt %u, order %u, bus %u, dev %u, size %u, addr "
		    "%p",
		    p_isp_hw_cmd_buf->cmd->static_info.camId,
		    p_isp_hw_cmd_buf->cmd->cmd_type,
		    p_isp_hw_cmd_buf->cmd->cmd_cnt,
		    p_isp_hw_cmd_buf->cmd->order,
		    p_isp_hw_cmd_buf->cmd->static_info.bus,
		    p_isp_hw_cmd_buf->cmd->static_info.dev,
		    p_isp_hw_cmd_buf->cmd->static_info.buf_size,
		    p_isp_hw_cmd_buf);

		/// already sort in userspace
		switch (cmd->cmd_type) {
		case CMD_TYPE_STREAMON:
		case CMD_TYPE_POST_STREAMON:
			if (cmd->static_info.dev == DRV_DEV_ISP) {
				hw_apply_cam_flag |=
					1 << p_isp_hw_cmd_buf->cmd->static_info
						     .camId;
				hw_apply_entry_cnt =
					count_bits(hw_apply_cam_flag);
			}
			if (is_stream_on_list_init == 0) {
				INIT_LIST_HEAD(&streamon_list);
				is_stream_on_list_init = 1;
			}
			list_add_tail(&p_isp_hw_cmd_buf->list, &streamon_list);
			break;
		case CMD_TYPE_STREAMOFF:
			if (is_stream_off_list_init == 0) {
				INIT_LIST_HEAD(&streamoff_list);
				is_stream_off_list_init = 1;
			}
			list_add_tail(&p_isp_hw_cmd_buf->list, &streamoff_list);
			break;
		case CMD_TYPE_POWERDOWN:
			if (is_stream_powerdown_list_init == 0) {
				INIT_LIST_HEAD(&powerdown_list);
				is_stream_powerdown_list_init = 1;
			}
			list_add_tail(&p_isp_hw_cmd_buf->list, &powerdown_list);
			break;
		default:
			break;
		}

		return ret;
	}

	switch (cmd->static_info.bus) {
	case HW_BUS_I2C:
#ifndef QEMU_ON_VEXPRESS
		ret = armcb_i2c_apply(cmd, client);
#endif
		break;
	case HW_BUS_SPI:
#ifndef QEMU_ON_VEXPRESS
		ret = armcb_spi_apply(cmd, client);
#endif
		break;
	case HW_BUS_AHB:
#ifndef QEMU_ON_VEXPRESS
		ret = armcb_ahb_apply(cmd, client);
#endif
		break;
	case HW_BUS_AHB_POWER:
#ifndef QEMU_ON_VEXPRESS
		ret = armcb_ahb_power_apply(cmd, client);
#endif
		break;
	case HW_BUS_DMA_REG:
	case HW_BUS_XDMA_REG:
	case HW_BUS_DMA_SRAM:
	case HW_BUS_XDMA_SRAM:
		cmd->cmd_cnt = MCFB_TRIG_MAX;
#ifndef QEMU_ON_VEXPRESS
		ret = armcb_ahb_apply(cmd, client);
#endif
		break;
	default:
		LOG(LOG_WARN, "not support hw bus %d!", cmd->static_info.bus);
		ret = -EINVAL;
		break;
	}

	if (ret < 0 && cmd->cmd_type != CMD_TYPE_PROBE) {
		LOG(LOG_ERR, "hw bus:%d type:%d action failed ret:%d! ",
		    cmd->static_info.bus, cmd->cmd_type, ret);
	}

	return ret;
}

/// system bus(AHB) performance test
int armcb_sys_bus_test(struct perf_bus_params *puser_bus_params)
{
	int ret = 0;
	int i = 0;
	s64 cost_time = 0;
	ktime_t ktime = 0;
	struct perf_bus_params bus_params = { 0 };
	struct perf_bus_params *pkel_bus_params = &bus_params;

	if (WARN_ON(!puser_bus_params)) {
		LOG(LOG_ERR, "input parameters is invalid ");
		return -EINVAL;
	}

	ret = copy_from_user(pkel_bus_params, puser_bus_params,
			     sizeof(bus_params));
	if (ret)
		return -EFAULT;

	/// read test
	ktime = ktime_get();
	for (i = 0; i < pkel_bus_params->test_times; ++i) {
		pkel_bus_params->reg_val = armcb_isp_read_reg(
			pkel_bus_params->reg_addr - ISP_REG_BASE);
	}
	ktime = ktime_sub(ktime_get(), ktime);
	cost_time = ktime_to_us(ktime);
	if (cost_time > 0)
		pkel_bus_params->read_cost = (unsigned int)cost_time;

	/// write test
	ktime = ktime_get();
	for (i = 0; i < pkel_bus_params->test_times; ++i) {
		armcb_isp_write_reg(pkel_bus_params->reg_addr - ISP_REG_BASE,
				    pkel_bus_params->reg_val);
	}
	ktime = ktime_sub(ktime_get(), ktime);
	cost_time = ktime_to_us(ktime);
	if (cost_time > 0)
		pkel_bus_params->write_cost = (unsigned int)cost_time;

	ret = copy_to_user(puser_bus_params, pkel_bus_params,
			   sizeof(bus_params));
	if (ret)
		return -EFAULT;

	return ret;
}
