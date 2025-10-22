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
#ifndef _DW_PHY_H
#define _DW_PHY_H

/**
 * @desc: dw phy get hpd state
 * @return: 1 - hpd plugin
 *          0 - hpd plugout
 */
u8 dw_phy_get_hpd(void);
/**
 * @desc: dw phy config svsret
 */
void dw_phy_config_svsret(void);
/**
 * @desc: dw phy set extern phy power
 * @state: 1 - enable extern phy power
 *         0 - disable extern phy power
 */
void dw_phy_set_power(u8 state);
/**
 * @desc: dw phy config interface mode by global params
 * @return: 0 - config success
 *         -1 - config failed
 */
int dw_phy_config_interface(void);
/**
 * @desc: dw phy set standby mode
 * @return: 0 - success
 *         -1 - failed
 */
int dw_phy_standby(void);
/**
 * @desc: dw phy wait mpll lock state
 * @return: 1 - mpll lock
 *          0 - mpll unlock
 */
u8 dw_phy_wait_lock(void);
/**
 * @desc: dw phy write
 * @addr: write address
 * @data: write value
 * @return: 0 - write success
 *         -1 - write failed
 */
int dw_phy_write(u8 addr, u16 data);
/**
 * @desc: dw phy read
 * @addr: read address
 * @value: point to read value
 * @return: 0 - read successs
 *         -1 - read failed
 */
int dw_phy_read(u8 addr, u16 *value);
/**
 * @desc: dw phy initial. not-use
 * @return: 0 - success
 *         -1 - failed
 */
int dw_phy_initialize(void);
/**
 * @desc: dw phy init. useing
 * @return: 0 - success
 *         -1 - failed
 */
int dw_phy_init(void);
/**
 * @desc: dw phy info dump
 * @buf: point to info buffer
 * @return: dump info byte
 */
ssize_t dw_phy_dump(char *buf);

/*****************************************************************************
 *                                                                           *
 *                        PHY Configuration Registers                        *
 *                                                                           *
 *****************************************************************************/

/**
 * @desc: PHY Configuration Register
 * This register holds the power down, data enable polarity,
 * and interface control of the HDMI Source PHY control
 * @bits: [0:0] Select interface control
 *        [1:1] Select data enable polarity
 *        [2:2] PHY ENHPDRXSENSE signal
 *        [3:3] PHY TXPWRON signal
 *        [4:4] PHY PDDQ signal
 *        [5:5] Reserved as "spare" register with no associated functionality
 *        [6:6] Reserved as "spare" register with no associated functionality
 *        [7:7] Reserved as "spare" register with no associated functionality
 */
#define PHY_CONF0               0x00003000
#define PHY_CONF0_SELDIPIF_MASK                 0x00000001
#define PHY_CONF0_SELDATAENPOL_MASK             0x00000002
#define PHY_CONF0_ENHPDRXSENSE_MASK             0x00000004
#define PHY_CONF0_TXPWRON_MASK                  0x00000008
#define PHY_CONF0_PDDQ_MASK                     0x00000010
#define PHY_CONF0_SVSRET_MASK                   0x00000020
#define PHY_CONF0_SPARES_1_MASK                 0x00000040
#define PHY_CONF0_SPARES_2_MASK                 0x00000080

/**
 * @desc: PHY Test Interface Register 0 PHY TX mapped test interface (control)
 * @bits: [0:0] Reserved as "spare" register with no associated functionality
 *        [3:1] Reserved as "spare" bit with no associated functionality
 *        [4:4] Reserved as "spare" register with no associated functionality
 *        [5:5] Reserved as "spare" register with no associated functionality
 *        [7:6] Reserved as "spare" bit with no associated functionality
 */
#define PHY_TST0                0x00003001
#define PHY_TST0_SPARE_0_MASK                   0x00000001
#define PHY_TST0_SPARE_1_MASK                   0x0000000E
#define PHY_TST0_SPARE_3_MASK                   0x00000010
#define PHY_TST0_SPARE_4_MASK                   0x00000020
#define PHY_TST0_SPARE_2_MASK                   0x000000C0

/**
 * @desc: PHY Test Interface Register 1 PHY TX mapped text interface (data in)
 * @bits: [7:0] Reserved as "spare" register with no associated functionality
 */
#define PHY_TST1                0x00003002
#define PHY_TST1_SPARE_MASK                     0x000000FF

/**
 * @desc: PHY Test Interface Register 2 PHY TX mapped text interface (data out)
 * @bits: [7:0] Reserved as "spare" register with no associated functionality
 */
#define PHY_TST2                0x00003003
#define PHY_TST2_SPARE_MASK                     0x000000FF

/**
 * @desc: PHY RXSENSE, PLL Lock, and HPD Status Register
 * This register contains the following active high packet sent status indications
 * @bits: [0:0] Tx PHY PLL Lock indication
 *        [1:1] HDMI Hot Plug Detect indication
 *        [4:4] Tx PHY RXSENSE indication for TMDS Channel 0
 *        [5:5] Tx PHY RXSENSE indication for TMDS Channel 1
 *        [6:6] Tx PHY RXSENSE indication for TMDS Channel 2
 *        [7:7] Tx PHY RXSENSE indication for TMDS Channel 3
 */
#define PHY_STAT0               0x00003004
#define PHY_STAT0_TX_PHY_LOCK_MASK                  0x00000001
#define PHY_STAT0_HPD_MASK                          0x00000002
#define PHY_STAT0_RX_SENSE_0_MASK                   0x00000010
#define PHY_STAT0_RX_SENSE_1_MASK                   0x00000020
#define PHY_STAT0_RX_SENSE_2_MASK                   0x00000040
#define PHY_STAT0_RX_SENSE_3_MASK                   0x00000080
#define PHY_STAT0_RX_SENSE_ALL_MASK					0x000000F0

/**
 * @desc: PHY RXSENSE, PLL Lock, and HPD Interrupt Register
 * This register contains the interrupt indication of the PHY_STAT0 status interrupts
 * @bits: [0:0] Tx PHY PLL Lock indication interrupt
 *        [1:1] HDMI Hot Plug Detect indication interrupt
 *        [4:4] Tx PHY RXSENSE indication interrupt for TMDS Channel 0
 *        [5:5] Tx PHY RXSENSE indication interrupt for TMDS Channel 1
 *        [6:6] Tx PHY RXSENSE indication interrupt for TMDS Channel 2
 *        [7:7] Tx PHY RXSENSE indication interrupt for TMDS Channel 3
 */
#define PHY_INT0                0x00003005
#define PHY_INT0_TX_PHY_LOCK_MASK                   0x00000001
#define PHY_INT0_HPD_MASK                           0x00000002
#define PHY_INT0_RX_SENSE_0_MASK                    0x00000010
#define PHY_INT0_RX_SENSE_1_MASK                    0x00000020
#define PHY_INT0_RX_SENSE_2_MASK                    0x00000040
#define PHY_INT0_RX_SENSE_3_MASK                    0x00000080

/**
 * @desc: PHY RXSENSE, PLL Lock, and HPD Mask Register Mask register
 * for generation of PHY_INT0 interrupts
 * @bits: [0:0] Tx PHY PLL Lock indication interrupt mask
 *        [1:1] HDMI Hot Plug Detect indication interrupt mask
 *        [4:4] Tx PHY RXSENSE indication interrupt mask for TMDS Channel 0
 *        [5:5] Tx PHY RXSENSE indication interrupt mask for TMDS Channel 1
 *        [6:6] Tx PHY RXSENSE indication interrupt mask for TMDS Channel 2
 *        [7:7] Tx PHY RXSENSE indication interrupt mask for TMDS Channel 3
 */
#define PHY_MASK0               0x00003006
#define PHY_MASK0_TX_PHY_LOCK_MASK                  0x00000001
#define PHY_MASK0_HPD_MASK                          0x00000002
#define PHY_MASK0_RX_SENSE_0_MASK                   0x00000010
#define PHY_MASK0_RX_SENSE_1_MASK                   0x00000020
#define PHY_MASK0_RX_SENSE_2_MASK                   0x00000040
#define PHY_MASK0_RX_SENSE_3_MASK                   0x00000080

/**
 * @desc: PHY RXSENSE, PLL Lock, and HPD Polarity Register Polarity register
 * for generation of PHY_INT0 interrupts
 * @bits: [0:0] Polarity bit for Tx PHY PLL Lock indication interrupt
 *        [1:1] Polarity bit for HDMI Hot Plug Detect indication interrupt
 *        [4:4] Polarity bit for Tx PHY RXSENSE indication interrupt for TMDS Channel 0
 *        [5:5] Polarity bit for Tx PHY RXSENSE indication interrupt for TMDS Channel 1
 *        [6:6] Polarity bit for Tx PHY RXSENSE indication interrupt for TMDS Channel 2
 *        [7:7] Polarity bit for Tx PHY RXSENSE indication interrupt for TMDS Channel 3
 */
#define PHY_POL0                0x00003007
#define PHY_POL0_TX_PHY_LOCK_MASK                   0x00000001
#define PHY_POL0_HPD_MASK                           0x00000002
#define PHY_POL0_RX_SENSE_0_MASK                    0x00000010
#define PHY_POL0_RX_SENSE_1_MASK                    0x00000020
#define PHY_POL0_RX_SENSE_2_MASK                    0x00000040
#define PHY_POL0_RX_SENSE_3_MASK                    0x00000080

/**
 * @desc: PHY I2C Slave Address Configuration Register
 * @bits: [6:0] Slave address to be sent during read and write operations
 */
#define PHY_I2CM_SLAVE          0x00003020
#define PHY_I2CM_SLAVE_SLAVEADDR_MASK               0x0000007F

/**
 * @desc: PHY I2C Address Configuration Register
 * This register writes the address for read and write operations
 * @bits: [7:0] Register address for read and write operations
 */
#define PHY_I2CM_ADDRESS        0x00003021
#define PHY_I2CM_ADDRESS_ADDRESS_MASK               0x000000FF

/**
 * @desc: PHY I2C Data Write Register 1
 * @bits: [7:0] Data MSB (datao[15:8]) to be written on register pointed by phy_i2cm_address [7:0]
 */
#define PHY_I2CM_DATAO_1        0x00003022
#define PHY_I2CM_DATAO_1_DATAO_MASK                 0x000000FF

/**
 * @desc: PHY I2C Data Write Register 0
 * @bits: [7:0] Data LSB (datao[7:0]) to be written on register pointed by phy_i2cm_address [7:0]
 */
#define PHY_I2CM_DATAO_0        0x00003023
#define PHY_I2CM_DATAO_0_DATAO_MASK                 0x000000FF

/**
 * @desc: PHY I2C Data Read Register 1
 * @bits: [7:0] Data MSB (datai[15:8]) read from register pointed by phy_i2cm_address[7:0]
 */
#define PHY_I2CM_DATAI_1        0x00003024
#define PHY_I2CM_DATAI_1_DATAI_MASK                 0x000000FF

/**
 * @desc: PHY I2C Data Read Register 0
 * @bits: Data LSB (datai[7:0]) read from register pointed by phy_i2cm_address[7:0]
 */
#define PHY_I2CM_DATAI_0        0x00003025
#define PHY_I2CM_DATAI_0_DATAI_MASK                 0x000000FF

/**
 * @desc: PHY I2C RD/RD_EXT/WR Operation Register
 * This register requests read and write operations from the I2C Master PHY
 * @bits: [0:0] Read operation request
 *        [4:4] Write operation request
 */
#define PHY_I2CM_OPERATION      0x00003026
#define PHY_I2CM_OPERATION_RD_MASK                  0x00000001
#define PHY_I2CM_OPERATION_WR_MASK                  0x00000010

/**
 * @desc: PHY I2C Done Interrupt Register
 * This register contains and configures I2C master PHY done interrupt
 * @bits: [0:0] Operation done status bit
 *        [1:1] Operation done interrupt bit
 *        [2:2] Done interrupt mask signal
 *        [3:3] Done interrupt polarity configuration
 */
#define PHY_I2CM_INT            0x00003027
#define PHY_I2CM_INT_DONE_STATUS_MASK               0x00000001
#define PHY_I2CM_INT_DONE_INTERRUPT_MASK            0x00000002
#define PHY_I2CM_INT_DONE_MASK_MASK                 0x00000004
#define PHY_I2CM_INT_DONE_POL_MASK                  0x00000008

/**
 * @desc: PHY I2C error Interrupt Register
 * This register contains and configures the I2C master PHY error interrupts
 * @bits: [0:0] Arbitration error status bit
 *        [1:1] Arbitration error interrupt bit
 *        [2:2] Arbitration error interrupt mask signa
 *        [3:3] Arbitration error interrupt polarity configuratio
 *        [4:4] Not acknowledge error status bit
 *        [5:5] Not acknowledge error interrupt bit
 *        [6:6] Not acknowledge error interrupt mask signal
 *        [7:7] Not acknowledge error interrupt polarity configuration
 */
#define PHY_I2CM_CTLINT         0x00003028
#define PHY_I2CM_CTLINT_ARBITRATION_STATUS_MASK     0x00000001
#define PHY_I2CM_CTLINT_ARBITRATION_INTERRUPT_MASK  0x00000002
#define PHY_I2CM_CTLINT_ARBITRATION_MASK_MASK       0x00000004
#define PHY_I2CM_CTLINT_ARBITRATION_POL_MASK        0x00000008
#define PHY_I2CM_CTLINT_NACK_STATUS_MASK            0x00000010
#define PHY_I2CM_CTLINT_NACK_INTERRUPT_MASK         0x00000020
#define PHY_I2CM_CTLINT_NACK_MASK_MASK              0x00000040
#define PHY_I2CM_CTLINT_NACK_POL_MASK               0x00000080

/**
 * @desc: PHY I2C Speed control Register
 * This register wets the I2C Master PHY to work in either Fast or Standard mode
 * @bits: [2:0] Reserved as "spare" register with no associated functionality
 *        [3:3] Sets the I2C Master to work in Fast Mode or Standard Mode
 *          - 1: Fast Mode
 *          - 0: Standard Mode
 */
#define PHY_I2CM_DIV            0x00003029
#define PHY_I2CM_DIV_SPARE_MASK                     0x00000007
#define PHY_I2CM_DIV_FAST_STD_MODE_MASK             0x00000008

/**
 * @desc: PHY I2C SW reset control register
 * This register sets the I2C Master PHY software reset
 * @bits: [0:0] I2C Master Software Reset
 */
#define PHY_I2CM_SOFTRSTZ       0x0000302A
#define PHY_I2CM_SOFTRSTZ_I2C_SOFTRSTZ_MASK         0x00000001

/**
 * @desc: PHY I2C Slow Speed SCL High Level Control Register 1
 * @bits: [7:0] PHY I2C Slow Speed SCL High Level Control Register 1
 */
#define PHY_I2CM_SS_SCL_HCNT_1_ADDR     0x0000302B
#define PHY_I2CM_SS_SCL_HCNT_1_ADDR_I2CMP_SS_SCL_HCNT1_MASK     0x000000FF

/**
 * @desc: PHY I2C Slow Speed SCL High Level Control Register 0
 * @bits: [7:0] PHY I2C Slow Speed SCL High Level Control Register 0
 */
#define PHY_I2CM_SS_SCL_HCNT_0_ADDR     0x0000302C
#define PHY_I2CM_SS_SCL_HCNT_0_ADDR_I2CMP_SS_SCL_HCNT0_MASK     0x000000FF

/**
 * @desc: PHY I2C Slow Speed SCL Low Level Control Register 1
 * @bits: [7:0] PHY I2C Slow Speed SCL Low Level Control Register 1
 */
#define PHY_I2CM_SS_SCL_LCNT_1_ADDR     0x0000302D
#define PHY_I2CM_SS_SCL_LCNT_1_ADDR_I2CMP_SS_SCL_LCNT1_MASK     0x000000FF

/**
 * @desc: PHY I2C Slow Speed SCL Low Level Control Register 0
 * @bits: [7:0] PHY I2C Slow Speed SCL Low Level Control Register 0
 */
#define PHY_I2CM_SS_SCL_LCNT_0_ADDR     0x0000302E
#define PHY_I2CM_SS_SCL_LCNT_0_ADDR_I2CMP_SS_SCL_LCNT0_MASK     0x000000FF

/**
 * @desc: PHY I2C Fast Speed SCL High Level Control Register 1
 * @bits: [7:0] PHY I2C Fast Speed SCL High Level Control Register 1
 */
#define PHY_I2CM_FS_SCL_HCNT_1_ADDR     0x0000302F
#define PHY_I2CM_FS_SCL_HCNT_1_ADDR_I2CMP_FS_SCL_HCNT1_MASK     0x000000FF

/**
 * @desc: PHY I2C Fast Speed SCL High Level Control Register 0
 * @bits: [7:0] PHY I2C Fast Speed SCL High Level Control Register 0
 */
#define PHY_I2CM_FS_SCL_HCNT_0_ADDR     0x00003030
#define PHY_I2CM_FS_SCL_HCNT_0_ADDR_I2CMP_FS_SCL_HCNT0_MASK     0x000000FF

/**
 * @desc: PHY I2C Fast Speed SCL Low Level Control Register 1
 * @bits: [7:0] PHY I2C Fast Speed SCL Low Level Control Register 1
 */
#define PHY_I2CM_FS_SCL_LCNT_1_ADDR     0x00003031
#define PHY_I2CM_FS_SCL_LCNT_1_ADDR_I2CMP_FS_SCL_LCNT1_MASK     0x000000FF

/**
 * @desc: PHY I2C Fast Speed SCL Low Level Control Register 0
 * @bits: [7:0] PHY I2C Fast Speed SCL Low Level Control Register 0
 */
#define PHY_I2CM_FS_SCL_LCNT_0_ADDR     0x00003032
#define PHY_I2CM_FS_SCL_LCNT_0_ADDR_I2CMP_FS_SCL_LCNT0_MASK     0x000000FF
/**
 * @desc: PHY I2C SDA HOLD Control Register
 * @bits: [7:0] Defines the number of SFR clock cycles to meet tHD
 * DAT (300 ns) osda_hold = round_to_high_integer (300 ns / (1/isfrclk_frequency))
 */
#define PHY_I2CM_SDA_HOLD               0x00003033
#define PHY_I2CM_SDA_HOLD_OSDA_HOLD_MASK                        0x000000FF

/**
 * @desc: PHY I2C/JTAG I/O Configuration Control Register
 * @bits: [0:0] Configures the JTAG PHY interface output pin JTAG_TRST_N
 *        [4:4] Configures the JTAG PHY interface output pin I2C_JTAGZ to select
 *              the PHY configuration interface
 */
#define JTAG_PHY_CONFIG         0x00003034
#define JTAG_PHY_CONFIG_JTAG_TRST_N_MASK        0x00000001
#define JTAG_PHY_CONFIG_I2C_JTAGZ_MASK          0x00000010

/**
 * @desc: PHY JTAG Clock Control Register
 * @bits: [0:0] Configures the JTAG PHY interface pin JTAG_TCK
 */
#define JTAG_PHY_TAP_TCK        0x00003035
#define JTAG_PHY_TAP_TCK_JTAG_TCK_MASK          0x00000001

/**
 * @desc: PHY JTAG TAP In Control Register
 * @bits: [0:0] Configures the JTAG PHY interface pin JTAG_TDI
 *        [4:4] Configures the JTAG PHY interface pin JTAG_TMS
 */
#define JTAG_PHY_TAP_IN         0x00003036
#define JTAG_PHY_TAP_IN_JTAG_TDI_MASK           0x00000001
#define JTAG_PHY_TAP_IN_JTAG_TMS_MASK           0x00000010

/**
 * @desc: PHY JTAG TAP Out Control Register
 * @bits: [0:0] Read JTAG PHY interface input pin JTAG_TDO
 *        [4:4] Read JTAG PHY interface input pin JTAG_TDO_EN
 */
#define JTAG_PHY_TAP_OUT        0x00003037
#define JTAG_PHY_TAP_OUT_JTAG_TDO_MASK          0x00000001
#define JTAG_PHY_TAP_OUT_JTAG_TDO_EN_MASK       0x00000010

/**
 * @desc: PHY JTAG Address Control Register
 * @bits: [7:0] Configures the JTAG PHY interface pin JTAG_ADDR[7:0]
 */
#define JTAG_PHY_ADDR           0x00003038
#define JTAG_PHY_ADDR_JTAG_ADDR_MASK            0x000000FF

#endif  /* _DW_PHY_H_ */
