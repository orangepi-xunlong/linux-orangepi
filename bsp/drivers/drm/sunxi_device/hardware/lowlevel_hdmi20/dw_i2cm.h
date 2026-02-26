/* SPDX-License-Identifier: GPL-2.0-or-later */
/*******************************************************************************
 * Allwinner SoCs hdmi2.0 driver.
 *
 * Copyright (C) 2016 Allwinner.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 ******************************************************************************/
#ifndef _DW_I2CM_H
#define _DW_I2CM_H

/**
 * @desc: dw ddc get rate.
 * @return: ddc rate. (*100Hz.)
 */
int dw_i2cm_get_rate(void);
/**
 * @desc: dw i2c set mode and rate
 * @mode: i2c mode. standard or fast mode
 * @rate: i2c rate. (*100Hz)
 * @return: 0 - suaccess
 *         -1 - failed
 */
int dw_i2cm_set_ddc(u32 mode, u32 rate);
/**
 * @desc: dw i2c master send and receive
 * @msgs: send or receive message buffer
 * @num: need send or receive message number
 * @return: actual send or receive message number
 */
int dw_i2cm_xfer(struct i2c_msg *msgs, int num);
/**
 * @desc: dw i2c master retry init
 * @return: 0 - success
 */
int dw_i2cm_re_init(void);
/**
 * @desc: dw i2c master init
 * @return: 0 - success
 */
int dw_i2cm_init(void);

/*****************************************************************************
 *                                                                           *
 *                              E-DDC Registers                              *
 *                                                                           *
 *****************************************************************************/

/**
 * @desc: I2C DDC Slave address Configuration Register
 * @bits: [6:0] Slave address to be sent during read and write normal operations
 */
#define I2CM_SLAVE          0x00007E00
#define I2CM_SLAVE_SLAVEADDR_MASK           0x0000007F

/**
 * @desc: I2C DDC Address Configuration Register
 * @bits: [7:0] Register address for read and write operations
 */
#define I2CM_ADDRESS        0x00007E01
#define I2CM_ADDRESS_ADDRESS_MASK           0x000000FF

/**
 * @desc: I2C DDC Data Write Register
 * @bits: [7:0] Data to be written on register pointed by address[7:0]
 */
#define I2CM_DATAO          0x00007E02
#define I2CM_DATAO_DATAO_MASK               0x000000FF

/**
 * @desc: I2C DDC Data read Register
 * @bits: [7:0] Data read from register pointed by address[7:0]
 */
#define I2CM_DATAI          0x00007E03
#define I2CM_DATAI_DATAI_MASK               0x000000FF

/**
 * @desc: I2C DDC RD/RD_EXT/WR Operation Register Read and write operation request
 * @bits: [0:0] Single byte read operation request
 *        [1:1] After writing 1'b1 to rd_ext bit a extended data read operation is started (E-DDC read operation)
 *        [2:2] Sequential read operation request
 *        [3:3] Extended sequential read operation request
 *        [4:4] Single byte write operation request
 *        [5:5] Bus clear operation request
 */
#define I2CM_OPERATION      0x00007E04
#define I2CM_OPERATION_RD_MASK              0x00000001
#define I2CM_OPERATION_RD_EXT_MASK          0x00000002
#define I2CM_OPERATION_RD8_MASK             0x00000004
#define I2CM_OPERATION_RD8_EXT_MASK         0x00000008
#define I2CM_OPERATION_WR_MASK              0x00000010
#define I2CM_OPERATION_BUS_CLEAR_MASK       0x00000020

/**
 * @desc: I2C DDC Done Interrupt Register
 * This register configures the I2C master interrupts
 * @bits: [2:2] Done interrupt mask signal
 *        [6:6] Read request interruption mask signal
 */
#define I2CM_INT            0x00007E05
#define I2CM_INT_DONE_MASK                  0x00000004
#define I2CM_INT_READ_REQ_MASK              0x00000040

/**
 * @desc: I2C DDC error Interrupt Register
 * This register configures the I2C master arbitration lost and not acknowledge error interrupts
 * @bits: [2:2] Arbitration error interrupt mask signal
 *        [6:6] Not acknowledge error interrupt mask signal
 */
#define I2CM_CTLINT         0x00007E06
#define I2CM_CTLINT_ARBITRATION_MASK        0x00000004
#define I2CM_CTLINT_NACK_MASK               0x00000040

/**
 * @desc: I2C DDC Speed Control Register
 * This register configures the division relation between master and scl clock
 * @bits: [2:0] This bit is a spare register with no associated functionality
 *        [7:7] Sets the I2C Master to work in Fast Mode or Standard Mode
 *          - 0: Standard Mode
 *          - 1: Fast Mode
 */
#define I2CM_DIV            0x00007E07
#define I2CM_DIV_SPARE_MASK                 0x00000007
#define I2CM_DIV_FAST_STD_MODE_MASK         0x00000008

/**
 * @desc: I2C DDC Segment Address Configuration Register
 * This register configures the segment address for extended R/W destination and
 * is used for EDID reading operations, particularly for the Extended Data Read Operation for Enhanced DDC
 * @bits: [6:0] I2C DDC Segment Address Configuration Register
 */
#define I2CM_SEGADDR        0x00007E08
#define I2CM_SEGADDR_SEG_ADDR_MASK          0x0000007F

/**
 * @desc: I2C DDC Software Reset Control Register
 * This register resets the I2C master
 * @bits: [0:0] I2C Master Software Reset
 */
#define I2CM_SOFTRSTZ       0x00007E09
#define I2CM_SOFTRSTZ_I2C_SOFTRSTZ_MASK     0x00000001

/**
 * @desc: I2C DDC Segment Pointer Register
 * This register configures the segment pointer for extended RD/WR request
 * @bits: [7:0] I2C DDC Segment Pointer Register
 */
#define I2CM_SEGPTR         0x00007E0A
#define I2CM_SEGPTR_SEGPTR_MASK             0x000000FF

/**
 * @desc: I2C DDC Slow Speed SCL High Level Control Register 1
 * @bits: [7:0] I2C DDC Slow Speed SCL High Level Control Register 1
 */
#define I2CM_SS_SCL_HCNT_1_ADDR     0x00007E0B
#define I2CM_SS_SCL_HCNT_1_ADDR_I2CMP_SS_SCL_HCNT1_MASK     0x000000FF

/**
 * @desc: I2C DDC Slow Speed SCL High Level Control Register 0
 * @bits: [7:0] I2C DDC Slow Speed SCL High Level Control Register 0
 */
#define I2CM_SS_SCL_HCNT_0_ADDR     0x00007E0C
#define I2CM_SS_SCL_HCNT_0_ADDR_I2CMP_SS_SCL_HCNT0_MASK     0x000000FF

/**
 * @desc: I2C DDC Slow Speed SCL Low Level Control Register 1
 * @bits: [7:0] I2C DDC Slow Speed SCL Low Level Control Register 1
 */
#define I2CM_SS_SCL_LCNT_1_ADDR     0x00007E0D
#define I2CM_SS_SCL_LCNT_1_ADDR_I2CMP_SS_SCL_LCNT1_MASK     0x000000FF

/**
 * @desc: I2C DDC Slow Speed SCL Low Level Control Register 0
 * @bits: [7:0] I2C DDC Slow Speed SCL Low Level Control Register 0
 */
#define I2CM_SS_SCL_LCNT_0_ADDR     0x00007E0E
#define I2CM_SS_SCL_LCNT_0_ADDR_I2CMP_SS_SCL_LCNT0_MASK     0x000000FF

/**
 * @desc: I2C DDC Fast Speed SCL High Level Control Register 1
 * @bits: [7:0] I2C DDC Fast Speed SCL High Level Control Register 1
 */
#define I2CM_FS_SCL_HCNT_1_ADDR     0x00007E0F
#define I2CM_FS_SCL_HCNT_1_ADDR_I2CMP_FS_SCL_HCNT1_MASK     0x000000FF

/**
 * @desc: I2C DDC Fast Speed SCL High Level Control Register 0
 * @bits: [7:0] I2C DDC Fast Speed SCL High Level Control Register 0
 */
#define I2CM_FS_SCL_HCNT_0_ADDR     0x00007E10
#define I2CM_FS_SCL_HCNT_0_ADDR_I2CMP_FS_SCL_HCNT0_MASK     0x000000FF

/**
 * @desc: I2C DDC Fast Speed SCL Low Level Control Register 1
 * @bits: [7:0] I2C DDC Fast Speed SCL Low Level Control Register 1
 */
#define I2CM_FS_SCL_LCNT_1_ADDR     0x00007E11
#define I2CM_FS_SCL_LCNT_1_ADDR_I2CMP_FS_SCL_LCNT1_MASK     0x000000FF

/**
 * @desc: I2C DDC Fast Speed SCL Low Level Control Register 0
 * @bits: [7:0] I2C DDC Fast Speed SCL Low Level Control Register 0
 */
#define I2CM_FS_SCL_LCNT_0_ADDR     0x00007E12
#define I2CM_FS_SCL_LCNT_0_ADDR_I2CMP_FS_SCL_LCNT0_MASK     0x000000FF

/**
 * @desc: I2C DDC SDA Hold Register
 * @bits: [7:0] Defines the number of SFR clock cycles to meet tHD;
 * DAT (300 ns) osda_hold = round_to_high_integer (300 ns / (1 / isfrclk_frequency))
 */
#define I2CM_SDA_HOLD               0x00007E13
#define I2CM_SDA_HOLD_OSDA_HOLD_MASK                        0x000000FF

/**
 * @desc: SCDC Control Register
 * This register configures the SCDC update status read through the I2C master interface
 * @bits: [0:0] SCDC Update Read is performed and the read data loaded into registers i2cm_scdc_update0 and i2cm_scdc_update1
 *        [4:4] Read request enabled
 *        [5:5] Update read polling enabled
 */
#define I2CM_SCDC_READ_UPDATE       0x00007E14
#define I2CM_SCDC_READ_UPDATE_READ_UPDATE_MASK              0x00000001
#define I2CM_SCDC_READ_UPDATE_READ_REQUEST_EN_MASK          0x00000010
#define I2CM_SCDC_READ_UPDATE_UPDTRD_VSYNCPOLL_EN_MASK      0x00000020

/**
 * @desc: I2C Master Sequential Read Buffer Register 0
 * @bits: [7:0] Byte 0 of a I2C read buffer sequential read
 */
#define I2CM_READ_BUFF0             0x00007E20
#define I2CM_READ_BUFF0_I2CM_READ_BUFF0_MASK                0x000000FF

/**
 * @desc: I2C Master Sequential Read Buffer Register 1
 * @bits: [7:0] Byte 1 of a I2C read buffer sequential read
 */
#define I2CM_READ_BUFF1             0x00007E21
#define I2CM_READ_BUFF1_I2CM_READ_BUFF1_MASK                0x000000FF

/**
 * @desc: I2C Master Sequential Read Buffer Register 2
 * @bits: [7:0] Byte 2 of a I2C read buffer sequential read
 */
#define I2CM_READ_BUFF2             0x00007E22
#define I2CM_READ_BUFF2_I2CM_READ_BUFF2_MASK                0x000000FF

/**
 * @desc: I2C Master Sequential Read Buffer Register 3
 * @bits: [7:0] Byte 3 of a I2C read buffer sequential read
 */
#define I2CM_READ_BUFF3             0x00007E23
#define I2CM_READ_BUFF3_I2CM_READ_BUFF3_MASK                0x000000FF

/**
 * @desc: I2C Master Sequential Read Buffer Register 4
 * @bits: [7:0] Byte 4 of a I2C read buffer sequential read
 */
#define I2CM_READ_BUFF4             0x00007E24
#define I2CM_READ_BUFF4_I2CM_READ_BUFF4_MASK                0x000000FF

/**
 * @desc: I2C Master Sequential Read Buffer Register 5
 * @bits: [7:0] Byte 5 of a I2C read buffer sequential read
 */
#define I2CM_READ_BUFF5             0x00007E25
#define I2CM_READ_BUFF5_I2CM_READ_BUFF5_MASK                0x000000FF

/**
 * @desc: I2C Master Sequential Read Buffer Register 6
 * @bits: [7:0] Byte 6 of a I2C read buffer sequential read
 */
#define I2CM_READ_BUFF6             0x00007E26
#define I2CM_READ_BUFF6_I2CM_READ_BUFF6_MASK                0x000000FF

/**
 * @desc: I2C Master Sequential Read Buffer Register 7
 * @bits: [7:0] Byte 7 of a I2C read buffer sequential read
 */
#define I2CM_READ_BUFF7             0x00007E27
#define I2CM_READ_BUFF7_I2CM_READ_BUFF7_MASK                0x000000FF

/**
 * @desc: I2C SCDC Read Update Register 0
 * @bits: [7:0] Byte 0 of a SCDC I2C update sequential read
 */
#define I2CM_SCDC_UPDATE0           0x00007E30
#define I2CM_SCDC_UPDATE0_I2CM_SCDC_UPDATE0_MASK            0x000000FF

/**
 * @desc: I2C SCDC Read Update Register 1
 * @bits: [7:0] Byte 1 of a SCDC I2C update sequential read
 */
#define I2CM_SCDC_UPDATE1           0x00007E31
#define I2CM_SCDC_UPDATE1_I2CM_SCDC_UPDATE1_MASK            0x000000FF

#endif  /* _DW_I2CM_H */