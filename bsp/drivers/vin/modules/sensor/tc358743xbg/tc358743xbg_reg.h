/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * A V4L2 driver for tc358743 HDMI to MIPI.
 *
 * Copyright (c) 2017 by Allwinnertech Co., Ltd.  http://www.allwinnertech.com
 *
 * Authors: Li Huiyu <lihuiyu@allwinnertrch>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#ifndef __TC358743XBG_REGS_H_
#define __TC358743XBG_REGS_H_

#include <linux/init.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/i2c.h>
#include <linux/delay.h>
#include <linux/videodev2.h>
#include <linux/clk.h>
#include <media/v4l2-device.h>
#include <media/v4l2-mediabus.h>
#include <linux/io.h>

#include "../camera.h"
#include "../sensor_helper.h"

#define TC_FLIP_EN	1

struct tc_reg {
	unsigned short addr;
	unsigned int data;
	unsigned char data_len;
};

int tc_write8(struct v4l2_subdev *sd, unsigned short addr, unsigned char data);
int tc_write16(struct v4l2_subdev *sd, unsigned short addr, unsigned int data);
int tc_write32(struct v4l2_subdev *sd, unsigned short addr, unsigned int data);

int tc_reg_read(struct v4l2_subdev *sd, unsigned short addr,
		int *data, unsigned char data_len, unsigned char dir);
unsigned char tc_read8(struct v4l2_subdev *sd, unsigned short addr);
unsigned short tc_read16(struct v4l2_subdev *sd, unsigned short addr);
unsigned int tc_read32(struct v4l2_subdev *sd, unsigned short addr);

void tc_write8_mask(struct v4l2_subdev *sd, u16 reg, u8 mask, u8 val);
void tc_write16_mask(struct v4l2_subdev *sd, u16 reg, u16 mask, u16 val);
void tc_write32_mask(struct v4l2_subdev *sd, u16 reg, u32 mask, u32 val);
#endif

//global  16-bit
#define CHIPID        0x0000
#define SENSOR_NAME "tc358743xbg_mipi"

#define SYSCTL                 0x0002
#define SYSCTL_SLEEP_MASK      (1 << 0)
/*This bit is set to force CEC logic and its configuration registers to reset*/
#define SYSCTL_HDMIRST_MASK    (1 << 8)
/*This bit is set to force CEC logic and its configuration registers to reset*/
#define SYSCTL_CTXRST_MASK     (1 << 9)
/*This bit is set to force CEC logic and its configuration registers to reset*/
#define SYSCTL_CECRST_MASK     (1 << 10)
/*This bit is set to force IR logic to reset state only,
the configuration registers are not affected*/
#define SYSCTL_IRRST_MASK      (1 << 11)

#define CONFCTL                0x0004
/*Video TX Buffer Enable*/
#define CONFCTL_VBUFEN_MASK    (1 << 0)
/*Audio TX Buffer Enable*/
#define CONFCTL_ABUFEN_MASK    (1 << 1)
/*I2C slave index increment
1'b0: I2C address index does not increment on every data byte transfer
1'b1: I2C address index increments on every databyte transfer*/
#define CONFCTL_AUTOINDEX_MASK (1 << 2)
/*Audio Output option
2'b00: Audio output to CSI2-TX i/f
2'b01: Reserved
2'b10: Audio output to I2S i/f (valid for 2 channel only)
2'b11: Audio output to TDM i/f
Note: When HDMI source output more than 2 Audio channels, automatic
select TDM i/f if AudOutSel[4]=1.*/
#define CONFCTL_AUDOUTSEL_MASK (3 << 3)
/*InfoFrame Data enable
1'b0: Do not send InfoFrame data out to CSI2
1'b1: Send InfoFrame data out to CSI2*/
#define CONFCTL_INFRMEN_MASK   (1 << 5)
/*YCbCr Video Output Format select
2'b00: Select YCbCr444 data format, Data Type 0x24 is used
2'b01: Select YCbCr422 24bpp data format,
Data Type is specified in register PacketID3[VPID2]
2'b10: Internal Generated output pattern, e.g. ColorBar, Data Type is
programmed in register field PacketID3[VPID2], independent of data format
2'b11: Select YCbCr422 16bpp (HDMI YCbCr422 24bppdata format,
discard last 4 data bits), Data Type = 0x1E as specified in CSI-2 spec
Note: RGB data, this field has to be set to 2'b00
*/
#define CONFCTL_YCBCRFMT_MASK  (3 << 6)
/*I2S/TDM Data Delay Option
1'b0: No delay
1'b1: Delay by 1 clock*/
#define CONFCTL_I2SDLYOPT_MASK (1 << 8)
/*Audio Channel Number Selection Mode
1'b0: Auto detect by HW
1'b1: Select by AudChNum register bits*/
#define CONFCTL_AUDCHSEL_MASK  (1 << 9)
/*Audio Channel Output Channels
2'b00: Enable 8 Audio channels
2'b01: Enable 6 Audio channels
2'b10: Enable 4 Audio channels
2'b11: Enable 2 Audio channels*/
#define CONFCTL_AUDCHNUM_MASK  (3 << 10)
/*Audio Bit Clock Option
1'b0: I2S/TDM clock are free running
1'b1: I2S/TDM clock stops when Mute active*/
#define CONFCTL_ACLKOPT_MASK   (1 << 12)
/*To be work with SysCtl.SLEEP*/
#define CONFCTL_PWRISO_MASK    (1 << 15)

#define FIFOCTL       0x0006
/*This field determines video delay from FiFo level, when reaches to this level
FiFo controller asserts FiFoRdy for CSI2-TX controller to start output*/
#define FIFOLEVEL_MASK (0x1ff << 0)

#define AWCNT         0x0008
#define VWCNT         0x000a
#define VWORDCNT_MASK (0xffff << 0)

#define PACKETID1               0x000c
/*Note: For interlace mode only, this ID is for Bottom video field*/
#define PACKETID1_VPID0_MASK    (0xff << 0)
/*Note: For interlace mode only, this ID is for Bottom video field*/
#define PACKETID1_VPID1_MASK    (0xff << 8)

#define PACKETID2                       0x000e
/*CSI InfoFrame Packet ID*/
#define PACKETID2_APID_MASK     (0xff << 0)
/*CSI Audio Packet ID*/
#define PACKETID2_IFPID_MASK    (0xff << 8)

#define PACKETID3     0x0010
/*CSI Video Packet ID
Note: Use when YCbCrFmt[1:0] = 2'b01 or YCbCrFmt[1:0] = 2'b10*/
#define PACKETID3_VPID_MASK     (0xff << 0)

#define FCCTL         0x0012

/*bit10:0 amute,csi,sys,cec_e,cec_t, cec_r, ir_e, ir_d*/
#define INTSTATUS     0x0014
#define IR_DINT_MASK	(1 << 0)
#define IR_EINT_MASK	(1 << 1)
#define CEC_RINT_MASK  (1 << 2)
#define CEC_TINT_MASK  (1 << 3)
#define CEC_EINT_MASK  (1 << 4)
#define SYS_INT_MASK     (1 << 5)
#define CSI_INT_MASK     (1 << 8)
#define HDMI_INT_MASK     (1 << 9)
#define AMUTE_INT_MASK     (1 << 10)


#define INTMASK                               0x0016
#define MASK_AMUTE_INT                        0x0400
#define MASK_HDMI_INT                         0x0200
#define MASK_CSI_INT                          0x0100
#define MASK_SYS_INT                          0x0020
#define MASK_CEC_EINT                         0x0010
#define MASK_CEC_TINT                         0x0008
#define MASK_CEC_RINT                         0x0004
#define MASK_IR_EINT                          0x0002
#define MASK_IR_DINT                          0x0001


#define INTFLAG       0x0018
#define INTSYSSTATUS  0x001a

#define PLLCTL0                   0x0020
/*Feedback divider setting Division ratio = (FBD8...0) + 1*/
#define PLLCTL0_PLL_FBD_MASK      (0xff << 0)
/*Input divider setting, Division ratio = (PRD3...0) + 1*/
#define PLLCTL0_PLL_PRD           (0xff << 8)

#define PLLCTL1                   0x0022
#define PLLCTL1_PLL_EN_MASK       (1 << 0)
#define PLLCTL1_PLL_RESETB_MASK   (1 << 1)
#define PLLCTL1_PLL_CKEN_MASK     (1 << 4)
#define PLLCTL1_PLL_BYPCKEN_MASK  (1 << 5)
#define PLLCTL1_PLL_LBWS_MASK     (3 << 8)
#define PLLCTL1_PLL_FRS_MASK      (3 << 10)

#define CECHCLK       0x0028
#define CECLCLK       0x002a
#define IRHCLK        0x002c
#define IRLCLK        0x002e
#define LCHMIN        0x0034
#define LCHMAX        0x0036
#define LCLMIN        0x0038
#define LCLMAX        0x003a
#define BHHMIN        0x003c
#define BHHMAX        0x003e
#define BHLMIN        0x0040
#define BHLMAX        0x0042
#define BLHMIN        0x0044
#define BLHMAX        0x0046
#define BLLMIN        0x0048
#define BLLMAX        0x004a
#define EndHMIN       0x004c
#define EndHMAX       0x004e
#define RCLMIN        0x0050
#define RCLMAX        0x0052
#define IRCTL         0x0058
#define IRRDATA       0x005a


/*CSI2-TX PHY/PPI 32-bit*/
#define CLW_DPHYCONTTX 0x0100
#define D0W_DPHYCONTTX 0x0104
#define D1W_DPHYCONTTX 0x0108
#define D2W_DPHYCONTTX 0x010c
#define D3W_DPHYCONTTX 0x0110

#define CLW_CNTRL      0x0140
/*Force Lane Disable for Clock Lane.
1'b1: Force Lane Disable
1'b0: Bypass Lane Enable from PPI Layer enable.*/
#define CLW_LANEDISABLE_MASK       (1 << 0)

#define D0W_CNTRL      0x0144
/*Force Lane Disable for Data Lane 0.
1'b1: Force Lane Disable
1'b0: Bypass Lane Enable from PPI Layer enable.*/
#define D0W_LANEDISABLE_MASK       (1 << 0)

#define D1W_CNTRL      0x0148
/*Force Lane Disable for Data Lane 1.
1'b1: Force Lane Disable
1'b0: Bypass Lane Enable from PPI Layer enable.*/
#define D1W_LANEDISABLE_MASK       (1 << 0)

#define D2W_CNTRL      0x014c
/*Force Lane Disable for Data Lane 2.
1'b1: Force Lane Disable
1'b0: Bypass Lane Enable from PPI Layer enable.*/
#define D2W_LANEDISABLE_MASK       (1 << 0)

#define D3W_CNTRL      0x0150
/*Force Lane Disable for Data Lane 3.
1'b1: Force Lane Disable
1'b0: Bypass Lane Enable from PPI Layer enable.*/
#define D3W_LANEDISABLE_MASK       (1 << 0)

#define STARTCNTRL     0x0204
#define STATUS         0x0208

#define LINEINITCNT      0x0210
#define LINEINITCNT_MASK (0xff << 0)

#define LPTXTIMECNT       0x0214
/*SYSLPTX Timing Generation Counter
The counter generates a timing signal for the period of LPTX.
This counter is counted using the HSByteClk (the Main Bus clock),
and the value of (setting + 1) * HSByteClk Period becomes the period LPTX.
Be sure to set the counter to a value greater than 50 ns.*/
#define LPTXTIMECNT_MASK  (0x07ff << 0)

#define TCLK_HEADERCNT 0x0218
#define TCLK_PREPARECNT_MASK (0x7f << 0)
#define TCLK_ZEROCNT_MASK (0xff << 8)

#define TCLK_TRAILCNT  0x021c
#define TCLKTRAILCNT_MASK (0xff << 0)

#define THS_HEADERCNT  0x0220
#define THS_PREPARECNT_MASK (0x7f << 0)
#define THS_ZEROCNT_MASK (0x7f << 8)

#define TWAKEUP           0x0224
#define TWAKEUPCNTMASK    (0xffff << 0)

#define TCLK_POSTCNT      0x0228
#define TCLK_POSTCNT_MASK (0x7ff << 0)

#define THS_TRAILCNT      0x022c
#define THS_TRAILCN_MASK  (0xf << 0)

#define HSTXVREGCNT       0x0230
#define HSTXVREGCNT_MASK  (0xffff << 0)

#define HSTXVREGEN          0x0234
#define CLM_HSTXVREGEN_MASK (1 << 0)
#define D0M_HSTXVREGEN_MASK (1 << 1)
#define D1M_HSTXVREGEN_MASK (1 << 2)
#define D2M_HSTXVREGEN_MASK (1 << 3)
#define D3M_HSTXVREGEN_MASK (1 << 4)

#define TXOPTIONCNTRL       0x0238
#define CONTCLKMODE_MASK    (1 << 0)


/* TX CTRL 32-bits*/
#define CSI_CONTROL        0x040c
#define CSI_STATUS         0x0410
#define CSI_INT            0x0414
#define CSI_INT_ENA        0x0418
#define CSI_ERR            0x044c
#define CSI_ERR_INTENA     0x0450
#define CSI_ERR_HALT       0x0454

#define CSI_CONFW          0x0500
/*DATA Field
When location DATA[n] is set to '1', the corresponding bit at AddrReg[n] will be
cleared or set depending on MODE bits described above.
Multiples bits can be set simultaneously*/
#define CSI_CONFW_DATA_MASK    (0xffff << 0)
/*Address Field
0x03: CSI_Control Register, please refer to 6.4.1, register 0x040C
0x06: CSI_INT_ENA Register
0x14: CSI_ERR_INTENA Register
0x15: CSI_ERR_ HALT Register
Others: Reserved*/
#define CSI_CONFW_ADRESS_MASK    (0x1f << 24)
/*Set or Clear AddrReg (register specified in Addressfield) Bits
3'b101: Set Register Bits in AddReg as indicated inDATA field
3'b110: Clear Register Bits in AddReg as indicated in DATA field
Others: Reserved */
#define CSI_CONFW_MODE_MASK    (0x7 << 29)

#define CSI_RESET          0x0504
#define CSI_INT_CLR        0x050c

#define CSI_START          0x0518
#define CSI_STRT_MASK      (1 << 0)


/*HDMIRX 8-bit*/
#define HDMI_INT0                      0x8500
#define MASK_I_KEY                    0x80
#define MASK_I_MISC                  0x02
#define MASK_I_PHYERR           0x01

#define HDMI_INT1                       0x8501
#define MASK_I_GBD                     0x80
#define MASK_I_HDCP                  0x40
#define MASK_I_ERR                     0x20
#define MASK_I_AUD                     0x10
#define MASK_I_CBIT                    0x08
#define MASK_I_PACKET             0x04
#define MASK_I_CLK                      0x02
#define MASK_I_SYS                       0x01

/* ACR_CTS, ACR_N, DVI, HDMI, NOPMBDET, DPMBDET, TMDS, DDC*/
#define SYS_INT                               0x8502
#define MASK_I_ACR_CTS            0x80
#define MASK_I_ACRN                   0x40
#define MASK_I_DVI                       0x20
#define MASK_I_HDMI                   0x10
#define MASK_I_NOPMBDET      0x08
#define MASK_I_DPMBDET         0x04
#define MASK_I_TMDS                   0x02
#define MASK_I_DDC                       0x01


#define CLK_INT           0x8503
#define MASK_I_OUT_H_CHG                      0x40
#define MASK_I_IN_DE_CHG                      0x20
#define MASK_I_IN_HV_CHG                      0x10
#define MASK_I_DC_CHG                         0x08
#define MASK_I_PXCLK_CHG                      0x04
#define MASK_I_PHYCLK_CHG                     0x02
#define MASK_I_TMDSCLK_CHG                    0x01

#define PACKET_INT        0x8504
#define MASK_I_PK_AVI     (1 << 0)
#define MASK_I_PK_AUD     (1 << 1)
#define MASK_I_PK_MS      (1 << 2)
#define MASK_I_PK_SPD     (1 << 3)
#define MASK_I_PK_VS      (1 << 4)
#define MASK_I_PK_ACP     (1 << 5)
#define MASK_I_PK_ISRC    (1 << 6)
#define MASK_I_PK_ISRC2   (1 << 7)

#define CBIT_INT          0x8505
#define MASK_I_AF_LOCK                        0x80
#define MASK_I_AF_UNLOCK                      0x40
#define MASK_I_AU_DST                         0x20
#define MASK_I_AU_DSD                         0x10
#define MASK_I_AU_HBR                         0x08
#define MASK_I_CBIT_NLPCM                     0x04
#define MASK_I_CBIT_FS                        0x02
#define CBIT_MASK_I_CBIT                      0x01


#define AUDIO_INT         0x8506
#define ERR_INT           0x8507
#define HDCP_INT          0x8508
#define GBD_INT           0x8509

#define MISC_INT          0x850b
#define MASK_I_AS_LAYOUT                      0x10
#define MASK_I_NO_SPD                         0x08
#define MASK_I_NO_VS                          0x03
#define MASK_I_SYNC_CHG                       0x02
#define MASK_I_AUDIO_MUTE                     0x01


#define KEY_INT           0x850f

/*int masks: ACR_CTS, ACR_N, DVI, HDMI, NOPMBDET, DPMBDET, TMDS, DDC*/
#define SYS_INTM          0x8512
#define CLK_INTM          0x8513

/*packet interrupt mask: ISRC2,ISRC,ACP,VS,SPD,MS,AUD,AVI*/
#define PACKET_INTM       0x8514

/*interrupt mask: Audio clock frequency lock detection, Audio clock frequency unlock detection
 DST/DSD/HBR Audio packet receive detection, LPCM<->NLPCM, FS update
*/
#define CBIT_INTM         0x8515
#define MASK_M_AF_LOCK                        0x80
#define MASK_M_AF_UNLOCK                      0x40
#define MASK_M_AU_DST                         0x20
#define MASK_M_AU_DSD                         0x10
#define MASK_M_AU_HBR                         0x08
#define MASK_M_CBIT_NLPCM                     0x04
#define MASK_M_CBIT_FS                        0x02
#define MASK_M_CBIT                           0x01

/*int mask: Buffer Over flow, Buffer Nearly Over(Threshold 2), Buffer Nearly Over(Threshold 1)
Buffer CENTER, Buffer Nearly Under (Threshold 1), Buffer Nearly Under (Threshold 2), Buffer Under flow, Buffer initialization operation completed
*/
#define AUDIO_INTM        0x8516
#define ERR_INTM          0x8517
#define HDCP_INTM         0x8518
#define GBD_INTM          0x8519

#define MISC_INTM         0x851b
/*audio Layout Bit change detection interrupt mask*/
#define MASK_M_AS_LAYOUT                      0x10
/*SPD_Info packet receive cutoff detection interrupt mask*/
#define MASK_M_NO_SPD                         0x08
/*VS_Info packet receive cutoff detection interrupt mask */
#define MASK_M_NO_VS                          0x03
/*Video sync signal state change detection interrupt mask */
#define MASK_M_SYNC_CHG                       0x02
/*audio Layout Bit change detection interrupt mask */
#define MASK_M_AUDIO_MUTE                     0x01


#define KEY_INTM          0x851f

#define SYS_STATUS        0x8520
#define MASK_S_SYNC                           0x80
#define MASK_S_AVMUTE                         0x40
#define MASK_S_HDCP                           0x20
#define MASK_S_HDMI                           0x10
#define MASK_S_PHY_SCDT                       0x08
#define MASK_S_PHY_PLL                        0x04
#define MASK_S_TMDS                           0x02
#define MASK_S_DDC5V                          0x01

#define VI_STATUS0        0x8521

#define VI_STATUS1        0x8522
#define MASK_S_V_GBD                          0x08
#define MASK_S_DEEPCOLOR                      0x0c
#define MASK_S_V_422                          0x02
#define MASK_S_V_INTERLACE                    0x01


#define AU_STATUS0        0x8523
#define MASK_S_A_SAMPLE 	0x01

#define AU_STATUS1        0x8524

#define VI_STATUS2        0x8525
#define CLK_STATUS        0x8526
#define PHYERR_STATUS     0x8527
#define VI_STATUS3        0x8528

#define PHY_CTL0          0x8531
/*PHY power ON/OFF control mode
0: HOST manual setting
1: DDC5V detection operation (reaction time depends on
address 0x8543[1:0] setting)*/
#define PHY_CTL           (1 << 0)
/*PHY System clock frequency setting
0: 27 or 26MHz 1: 42MHz*/
#define PHY_SYSCLK_IND    (1 << 1)

#define PHY_EN            0x8534
/*PHY general SUSPEND control
0: Suspend 1: Normal Operation */
#define ENABLE_PHY (1 << 0)

#define PHY_RST           0x8535
#define MASK_RESET_CTRL                       0x01

#define PHY_PLL           0x8538
#define PHY_CDR           0x853a
#define SYS_FREQ0         0x8540
#define SYS_FREQ1         0x8541

#define DDC_CTL           0x8543
/*DDC5V_active detect delay setting
To prevent chattering in the DDC5V input rising detection area,
DDC5V is judged as avtime after a specified time from the rise
detect point in time.
00: 0msec, 01: 50msec, 10: 100msec, 11: 200msec,*/
#define DDC5V_MODE        (0x3 << 0)
/*Selection of response method for DDC access from send side
0: DDC is active only while HotPLUG is being output
1: DDC is active when initialization completion INIT_END,
0x854A[0], is asserted */
#define DDC_ACTION        (1 << 2)
/*DDC_ACK output terminal polarity selection
0: H active output
1: L active output*/
#define DDC_ACK_POL       (1 << 3)

#define HPD_CTL           0x8544
#define MASK_HPD_CTL0                         0x10
#define MASK_HPD_OUT0                         0x01

#define ANA_CTL           0x8545
#define MASK_APPL_PCSX_NORMAL                 0x30
#define MASK_ANALOG_ON                        0x01


#define AVM_CTL           0x8546
#define SOFT_RST          0x8547
#define INIT_END          0x854a
#define HDMI_DVI          0x8550
#define HDCP_MODE         0x8560
#define AUTO_CLR        (1 << 1)
#define LINE_REKEY     (1 << 4)
#define MODE_RST_TN (1 << 5)

#define HDCP_CMD          0x8561
#define VI_MODE           0x8570
#define VOUT_SET2         0x8573
/*RGB888 to YUV422 Conversion Mode Setting
00: Through mode (RGB888 Output)
01: Enable RGB888 to YUV422 Conversion (Fixed Coloroutput)
10: Reserved
11: Enable  RGB888  to  YUV422  Conversion  (Use  Multiplication
Coeff set in registers 0x85b0~0x85c1)*/
#define VOUTCOLORMODE_MASK (3 << 0)
#define VOUT_422FIL_MASK (7 << 4)
/*0: 444 fixed output
1: 422 fixed output
[Note]Valid only when 0x8574[3] = 1*/
#define VOUT_SEL422_MASK (1 << 7)

#define VOUT_SET3         0x8574
#define VI_REP            0x8576
#define DC_MODE           0x8577

#define VI_MUTE       0x857f
#define VI_BLACK_MASK          (1 << 0)
#define VI_MUTE_MASK            (1 << 4)
#define AUTOMUTE0_MASK   (1 << 6)
#define AUTOMUTE1_MASK   (1 << 7)

#define DE_WIDTH_H_LO                         0x8582 /* Not in REF_01 */
#define DE_WIDTH_H_HI                         0x8583 /* Not in REF_01 */
#define DE_WIDTH_V_LO                         0x8588 /* Not in REF_01 */
#define DE_WIDTH_V_HI                         0x8589 /* Not in REF_01 */
#define H_SIZE_LO                             0x858A /* Not in REF_01 */
#define H_SIZE_HI                             0x858B /* Not in REF_01 */
#define V_SIZE_LO                             0x858C /* Not in REF_01 */
#define V_SIZE_HI                             0x858D /* Not in REF_01 */
#define FV_CNT_LO                             0x85A1 /* Not in REF_01 */
#define FV_CNT_HI                             0x85A2 /* Not in REF_01 */

#define FH_MIN0                               0x85AA /* Not in REF_01 */
#define FH_MIN1                               0x85AB /* Not in REF_01 */
#define FH_MAX0                               0x85AC /* Not in REF_01 */
#define FH_MAX1                               0x85AD /* Not in REF_01 */

#define HV_RST                                0x85AF /* Not in REF_01 */
#define MASK_H_PI_RST                         0x20
#define MASK_V_PI_RST                         0x10

#define EDID_MODE         0x85c7
/*EDID access response mode selection
00: DDC line direct connection EEPROM mode
(Absolutely no response to EDID access from DDC line)
01: Internal EDID-RAM & DDC2B mode
(No response to 0x60slave =>Returns NACK)
1x: Internal EDID-RAM & E-DDC mode*/
#define MASK_EDID_MODE    (0x3 << 0)
/*EDID dedicated terminal I2C speed selection
000: 100KHz
001: 400KHz*/
#define MASK_EDID_SPEED   (0x7 << 4)

#define EDID_SLV          0x85c8
#define EDID_OFF          0x85c9
#define EDID_LEN1         0x85ca
#define EDID_LEN2         0x85cb
#define EDID_CMD          0x85cc
#define FORCE_MUTE        0x8600
#define CMD_AUD           0x8601
#define AUTO_CMD0         0x8602
#define AUTO_CMD1         0x8603
#define AUTO_CMD2         0x8604
#define BUFINIT_START     0x8606
#define FS_MUTE           0x8607
#define MUTE_MODE         0x8608
#define FS_IMODE          0x8620

#define FS_SET            0x8621
#define MASK_FS                               0x0f

#define CBIT_BYTE0        0x8622
#define CBIT_BYTE1        0x8623
#define CBIT_BYTE2        0x8624
#define CBIT_BYTE3        0x8625
#define CBIT_BYTE4        0x8626
#define CBIT_BYTE5        0x8627
#define LOCKDET_REF0      0x8630
#define LOCKDET_REF1      0x8631
#define LOCKDET_REF2      0x8632
#define ACR_MODE          0x8640
#define ACR_MDF0          0x8641
#define ACR_MDF1          0x8642
#define SDO_MODE0         0x8651
#define SDO_MODE1         0x8652

#define NCO_F0_MOD        0x8670
/*NCO standard frequency setting for Audio PLL
00: For REFCLK = 42MHz
01: For REFCLK = 27MHz
1x: Register setting value uses 28bit setting for 48KHz series
use, for 44.1KHz series use. (address 0x8671~78*/
#define MASK_NCO_F0_MOD   (0x03 << 0)

#define APIN_ENO          0x8690
#define TYP_VS_SET        0x8701
#define TYP_AVI_SET       0x8702
#define TYP_SPD_SET       0x8703
#define TYP_AUD_SET       0x8704
#define TYP_MS_SET        0x8705
#define TYP_ACP_SET       0x8706
#define TYP_ISRC1_SET     0x8707
#define TYP_ISRC2_SET     0x8708
#define PK_INT_MODE       0x8709
#define PK_AUTO_CLR       0x870a
#define NO_PK_LIMIT       0x870b
#define NO_PK_CLR         0x870c
#define ERR_PK_LIMIT      0x870d
#define NO_PK_LIMIT2      0x870e
#define VS_IEEE_SEL       0x870f
#define PK_AVI_0HEAD                          0x8710
#define PK_AVI_1HEAD                          0x8711
#define PK_AVI_2HEAD                          0x8712
#define PK_AVI_0BYTE                          0x8713
#define PK_AVI_1BYTE                          0x8714
#define PK_AVI_2BYTE                          0x8715
#define PK_AVI_3BYTE                          0x8716
#define PK_AVI_4BYTE                          0x8717
#define PK_AVI_5BYTE                          0x8718
#define PK_AVI_6BYTE                          0x8719
#define PK_AVI_7BYTE                          0x871A
#define PK_AVI_8BYTE                          0x871B
#define PK_AVI_9BYTE                          0x871C
#define PK_AVI_10BYTE                         0x871D
#define PK_AVI_11BYTE                         0x871E
#define PK_AVI_12BYTE                         0x871F
#define PK_AVI_13BYTE                         0x8720
#define PK_AVI_14BYTE                         0x8721
#define PK_AVI_15BYTE                         0x8722
#define PK_AVI_16BYTE                         0x8723

#define BKSV                                  0x8800

#define BCAPS                                 0x8840
#define MASK_HDMI_RSVD                        0x80
#define MASK_REPEATER                         0x40
#define MASK_READY                            0x20
#define MASK_FASTI2C                          0x10
#define MASK_1_1_FEA                          0x02
#define MASK_FAST_REAU                        0x01

#define BSTATUS1                              0x8842
#define MASK_MAX_EXCED                        0x08

#define EDID_RAM                              0x8C00
#define NO_GDB_LIMIT                          0x9007
