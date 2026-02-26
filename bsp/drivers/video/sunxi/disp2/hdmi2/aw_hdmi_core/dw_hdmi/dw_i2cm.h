/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * Allwinner SoCs hdmi2.0 driver.
 *
 * Copyright (C) 2016 Allwinner.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#ifndef _DW_I2CM_H
#define _DW_I2CM_H

#include "dw_dev.h"

#ifdef __FPGA_PLAT__
/* SDL: 100K */
#define I2C_SFR_CLK 2500
#define I2CDDC_TIMEOUT 100
#define I2C_MIN_FS_SCL_HIGH_TIME   61
#define I2C_MIN_FS_SCL_LOW_TIME    132
#define I2C_MIN_SS_SCL_HIGH_TIME   4592
#define I2C_MIN_SS_SCL_LOW_TIME    5102
#else
/* SDL: 50K */
#define I2C_SFR_CLK 2400
#define I2CDDC_TIMEOUT 300
#define I2C_MIN_FS_SCL_HIGH_TIME   600/* 61 63 75 */
#define I2C_MIN_FS_SCL_LOW_TIME    1300/* 132 137 163 */
#define I2C_MIN_SS_SCL_HIGH_TIME   9184 /* 5333 4000 4737 5625 */
#define I2C_MIN_SS_SCL_LOW_TIME    10204 /* 4700 5263 6250 */
#endif
/*****************************************************************************
 *                                                                           *
 *                              E-DDC Registers                        *
 *                                                                           *
 *****************************************************************************/

/* I2C DDC Slave address Configuration Register */
#define I2CM_SLAVE  0x0001F800
#define I2CM_SLAVE_SLAVEADDR_MASK  0x0000007F /* Slave address to be sent during read and write normal operations */

/* I2C DDC Address Configuration Register */
#define I2CM_ADDRESS  0x0001F804
#define I2CM_ADDRESS_ADDRESS_MASK  0x000000FF /* Register address for read and write operations */

/* I2C DDC Data Write Register */
#define I2CM_DATAO  0x0001F808
#define I2CM_DATAO_DATAO_MASK  0x000000FF /* Data to be written on register pointed by address[7:0] */

/* I2C DDC Data read Register */
#define I2CM_DATAI  0x0001F80C
#define I2CM_DATAI_DATAI_MASK  0x000000FF /* Data read from register pointed by address[7:0] */

/* I2C DDC RD/RD_EXT/WR Operation Register Read and write operation request */
#define I2CM_OPERATION  0x0001F810
#define I2CM_OPERATION_RD_MASK  0x00000001 /* Single byte read operation request */
#define I2CM_OPERATION_RD_EXT_MASK  0x00000002 /* After writing 1'b1 to rd_ext bit a extended data read operation is started (E-DDC read operation) */
#define I2CM_OPERATION_RD8_MASK  0x00000004 /* Sequential read operation request */
#define I2CM_OPERATION_RD8_EXT_MASK  0x00000008 /* Extended sequential read operation request */
#define I2CM_OPERATION_WR_MASK  0x00000010 /* Single byte write operation request */

/* I2C DDC Done Interrupt Register This register configures the I2C master interrupts */
#define I2CM_INT  0x0001F814
#define I2CM_INT_DONE_MASK  0x00000004 /* Done interrupt mask signal */
#define I2CM_INT_READ_REQ_MASK  0x00000040 /* Read request interruption mask signal */

/* I2C DDC error Interrupt Register This register configures the I2C master arbitration lost and not acknowledge error interrupts */
#define I2CM_CTLINT  0x0001F818
#define I2CM_CTLINT_ARBITRATION_MASK  0x00000004 /* Arbitration error interrupt mask signal */
#define I2CM_CTLINT_NACK_MASK  0x00000040 /* Not acknowledge error interrupt mask signal */

/* I2C DDC Speed Control Register This register configures the division relation between master and scl clock */
#define I2CM_DIV  0x0001F81C
#define I2CM_DIV_SPARE_MASK  0x00000007 /* This bit is a spare register with no associated functionality */
#define I2CM_DIV_FAST_STD_MODE_MASK  0x00000008 /* Sets the I2C Master to work in Fast Mode or Standard Mode: 1: Fast Mode 0: Standard Mode */

/* I2C DDC Segment Address Configuration Register This register configures the segment address for extended R/W destination and is used for EDID reading operations, particularly for the Extended Data Read Operation for Enhanced DDC */
#define I2CM_SEGADDR  0x0001F820
#define I2CM_SEGADDR_SEG_ADDR_MASK  0x0000007F /* I2C DDC Segment Address Configuration Register */

/* I2C DDC Software Reset Control Register This register resets the I2C master */
#define I2CM_SOFTRSTZ  0x0001F824
#define I2CM_SOFTRSTZ_I2C_SOFTRSTZ_MASK  0x00000001 /* I2C Master Software Reset */

/* I2C DDC Segment Pointer Register This register configures the segment pointer for extended RD/WR request */
#define I2CM_SEGPTR  0x0001F828
#define I2CM_SEGPTR_SEGPTR_MASK  0x000000FF /* I2C DDC Segment Pointer Register */

/* I2C DDC Slow Speed SCL High Level Control Register 1 */
#define I2CM_SS_SCL_HCNT_1_ADDR  0x0001F82C
#define I2CM_SS_SCL_HCNT_1_ADDR_I2CMP_SS_SCL_HCNT1_MASK  0x000000FF /* I2C DDC Slow Speed SCL High Level Control Register 1 */

/* I2C DDC Slow Speed SCL High Level Control Register 0 */
#define I2CM_SS_SCL_HCNT_0_ADDR  0x0001F830
#define I2CM_SS_SCL_HCNT_0_ADDR_I2CMP_SS_SCL_HCNT0_MASK  0x000000FF /* I2C DDC Slow Speed SCL High Level Control Register 0 */

/* I2C DDC Slow Speed SCL Low Level Control Register 1 */
#define I2CM_SS_SCL_LCNT_1_ADDR  0x0001F834
#define I2CM_SS_SCL_LCNT_1_ADDR_I2CMP_SS_SCL_LCNT1_MASK  0x000000FF /* I2C DDC Slow Speed SCL Low Level Control Register 1 */

/* I2C DDC Slow Speed SCL Low Level Control Register 0 */
#define I2CM_SS_SCL_LCNT_0_ADDR  0x0001F838
#define I2CM_SS_SCL_LCNT_0_ADDR_I2CMP_SS_SCL_LCNT0_MASK  0x000000FF /* I2C DDC Slow Speed SCL Low Level Control Register 0 */

/* I2C DDC Fast Speed SCL High Level Control Register 1 */
#define I2CM_FS_SCL_HCNT_1_ADDR  0x0001F83C
#define I2CM_FS_SCL_HCNT_1_ADDR_I2CMP_FS_SCL_HCNT1_MASK  0x000000FF /* I2C DDC Fast Speed SCL High Level Control Register 1 */

/* I2C DDC Fast Speed SCL High Level Control Register 0 */
#define I2CM_FS_SCL_HCNT_0_ADDR  0x0001F840
#define I2CM_FS_SCL_HCNT_0_ADDR_I2CMP_FS_SCL_HCNT0_MASK  0x000000FF /* I2C DDC Fast Speed SCL High Level Control Register 0 */

/* I2C DDC Fast Speed SCL Low Level Control Register 1 */
#define I2CM_FS_SCL_LCNT_1_ADDR  0x0001F844
#define I2CM_FS_SCL_LCNT_1_ADDR_I2CMP_FS_SCL_LCNT1_MASK  0x000000FF /* I2C DDC Fast Speed SCL Low Level Control Register 1 */

/* I2C DDC Fast Speed SCL Low Level Control Register 0 */
#define I2CM_FS_SCL_LCNT_0_ADDR  0x0001F848
#define I2CM_FS_SCL_LCNT_0_ADDR_I2CMP_FS_SCL_LCNT0_MASK  0x000000FF /* I2C DDC Fast Speed SCL Low Level Control Register 0 */

/* I2C DDC SDA Hold Register */
#define I2CM_SDA_HOLD  0x0001F84C
#define I2CM_SDA_HOLD_OSDA_HOLD_MASK  0x000000FF /* Defines the number of SFR clock cycles to meet tHD;DAT (300 ns) osda_hold = round_to_high_integer (300 ns / (1 / isfrclk_frequency)) */

/* SCDC Control Register This register configures the SCDC update status read through the I2C master interface */
#define I2CM_SCDC_READ_UPDATE  0x0001F850
#define I2CM_SCDC_READ_UPDATE_READ_UPDATE_MASK  0x00000001 /* When set to 1'b1, a SCDC Update Read is performed and the read data loaded into registers i2cm_scdc_update0 and i2cm_scdc_update1 */
#define I2CM_SCDC_READ_UPDATE_READ_REQUEST_EN_MASK  0x00000010 /* Read request enabled */
#define I2CM_SCDC_READ_UPDATE_UPDTRD_VSYNCPOLL_EN_MASK  0x00000020 /* Update read polling enabled */

/* I2C Master Sequential Read Buffer Register 0 */
#define I2CM_READ_BUFF0  0x0001F880
#define I2CM_READ_BUFF0_I2CM_READ_BUFF0_MASK  0x000000FF /* Byte 0 of a I2C read buffer sequential read (from address i2cm_address) */

/* I2C Master Sequential Read Buffer Register 1 */
#define I2CM_READ_BUFF1  0x0001F884
#define I2CM_READ_BUFF1_I2CM_READ_BUFF1_MASK  0x000000FF /* Byte 1 of a I2C read buffer sequential read (from address i2cm_address+1) */

/* I2C Master Sequential Read Buffer Register 2 */
#define I2CM_READ_BUFF2  0x0001F888
#define I2CM_READ_BUFF2_I2CM_READ_BUFF2_MASK  0x000000FF /* Byte 2 of a I2C read buffer sequential read (from address i2cm_address+2) */

/* I2C Master Sequential Read Buffer Register 3 */
#define I2CM_READ_BUFF3  0x0001F88C
#define I2CM_READ_BUFF3_I2CM_READ_BUFF3_MASK  0x000000FF /* Byte 3 of a I2C read buffer sequential read (from address i2cm_address+3) */

/* I2C Master Sequential Read Buffer Register 4 */
#define I2CM_READ_BUFF4  0x0001F890
#define I2CM_READ_BUFF4_I2CM_READ_BUFF4_MASK  0x000000FF /* Byte 4 of a I2C read buffer sequential read (from address i2cm_address+4) */

/* I2C Master Sequential Read Buffer Register 5 */
#define I2CM_READ_BUFF5  0x0001F894
#define I2CM_READ_BUFF5_I2CM_READ_BUFF5_MASK  0x000000FF /* Byte 5 of a I2C read buffer sequential read (from address i2cm_address+5) */

/* I2C Master Sequential Read Buffer Register 6 */
#define I2CM_READ_BUFF6  0x0001F898
#define I2CM_READ_BUFF6_I2CM_READ_BUFF6_MASK  0x000000FF /* Byte 6 of a I2C read buffer sequential read (from address i2cm_address+6) */

/* I2C Master Sequential Read Buffer Register 7 */
#define I2CM_READ_BUFF7  0x0001F89C
#define I2CM_READ_BUFF7_I2CM_READ_BUFF7_MASK  0x000000FF /* Byte 7 of a I2C read buffer sequential read (from address i2cm_address+7) */

/* I2C SCDC Read Update Register 0 */
#define I2CM_SCDC_UPDATE0  0x0001F8C0
#define I2CM_SCDC_UPDATE0_I2CM_SCDC_UPDATE0_MASK  0x000000FF /* Byte 0 of a SCDC I2C update sequential read */

/* I2C SCDC Read Update Register 1 */
#define I2CM_SCDC_UPDATE1  0x0001F8C4
#define I2CM_SCDC_UPDATE1_I2CM_SCDC_UPDATE1_MASK  0x000000FF /* Byte 1 of a SCDC I2C update sequential read */


/** I2C clock configuration
 *
 * @param sfrClock external clock supplied to controller
 * @param value of standard speed low time counter
 * @param value of standard speed high time counter
 * @param value of fast speed low time counter
 * @param value of fast speed high time counter
 */
void dw_i2cm_ddc_clk_config(dw_hdmi_dev_t *dev, u16 sfrClock, u16 ss_low_ckl,
			u16 ss_high_ckl, u16 fs_low_ckl, u16 fs_high_ckl);

/** Set the speed mode (standard/fast mode)
 *
 * @param fast mode selection, 0 standard - 1 fast
 */
void ctrl_i2cm_ddc_fast_mode(dw_hdmi_dev_t *dev, u8 fast);


/** Enable disable interrupts.
 *
 * @param mask to enable or disable the masking (u32 baseAddr, true to mask,
 * ie true to stop seeing the interrupt).
 */
void dw_i2cm_ddc_interrupt_mask(dw_hdmi_dev_t *dev, u8 mask);

/** Read from extended addresses, E-DDC.
 *
 * @param i2cAddr i2c device address to read data
 * @param addr base address of the module registers
 * @param segment segment to read from
 * @param pointer in segment to read
 * @param value pointer to data read
 * @returns 0 if ok and error in other cases
 */
int dw_i2cm_ddc_read(dw_hdmi_dev_t *dev, u8 i2cAddr, u8 segment, u8 pointer,
						u8 addr, u8 len, u8 *data);

/** Write from extended addresses, E-DDC.
 *
 * @param i2cAddr i2c device address to read data
 * @param addr base address of the module registers
 * @param len lenght to write
 * @param data pointer to data write
 * @returns 0 if ok and error in other cases
 */
int dw_i2cm_ddc_write(dw_hdmi_dev_t *dev, u8 i2cAddr, u8 addr, u8 len, u8 *data);

#endif  /* _DW_I2CM_H */
