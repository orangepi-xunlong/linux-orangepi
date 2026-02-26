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
#ifndef _DW_MC_H_
#define _DW_MC_H_

typedef enum {
	DW_MC_SWRST_PIXEL   = 0,
	DW_MC_SWRST_TMDS    = 1,
	DW_MC_SWRST_PREP    = 2,
	DW_MC_SWRST_I2S     = 3,
	DW_MC_SWRST_SPDIF   = 4,
	DW_MC_SWRST_CEC     = 5,
	DW_MC_SWRST_IGPA    = 6,
	DW_MC_SWRST_PHY     = 7,
	DW_MC_SWRST_HEACPHY = 8,
	DW_MC_SWRST_ADMA    = 9,
} dw_mc_swrst_t;

typedef enum {
	DW_MC_CLK_LOCK_CEC   = 0,
	DW_MC_CLK_LOCK_RES   = 1,
	DW_MC_CLK_LOCK_SPDIF = 2,
	DW_MC_CLK_LOCK_I2S   = 3,
	DW_MC_CLK_LOCK_PREP  = 4,
	DW_MC_CLK_LOCK_TMDS  = 5,
	DW_MC_CLK_LOCK_PIXEL = 6,
	DW_MC_CLK_LOCK_IGPA  = 7,
} dw_mc_lock_e;

typedef enum {
	DW_MC_CLK_PIXEL = 0,
	DW_MC_CLK_TMDS  = 1,
	DW_MC_CLK_PREP  = 2,
	DW_MC_CLK_AUDIO = 3,
	DW_MC_CLK_CSC   = 4,
	DW_MC_CLK_CEC   = 5,
	DW_MC_CLK_HDCP  = 6,
} dw_mc_clk_e;

typedef enum {
	DW_MC_IRQ_FC0 = 1,
	DW_MC_IRQ_FC1,
	DW_MC_IRQ_FC2,
	DW_MC_IRQ_AS,
	DW_MC_IRQ_PHY,
	DW_MC_IRQ_I2CM,
	DW_MC_IRQ_CEC,
	DW_MC_IRQ_VP,
	DW_MC_IRQ_PHYI2C,
	DW_MC_IRQ_AUDIO_DMA,
} dw_mc_irq_e;

/**
 * @desc: dw main control software reset
 * @type: need reset type
 * @state: reset signal.
 * 	if phy reset, when PHY_GEN1 signal is 0. when PHY_GEN2 signal is 1
 *  other reset signal is 0.
 */
int dw_mc_sw_reset(dw_mc_swrst_t type, u8 state);
/**
 * @desc: main control set module clock
 * @type: main control module index
 * @state: 0 - disable module clock
 *         1 - enable module clock
 */
int dw_mc_set_clk(dw_mc_clk_e type, u8 state);
/**
 * @desc: main control set module clock
 * @type: main control module index
 * @return: 0 - disable module clock
 *          1 - enable module clock
 */
u8 dw_mc_get_clk(dw_mc_clk_e type);
/**
 * @desc: main control get clock lock status
 * @type: main control module index
 * @return: 0 - clock unlock
 *          1 - clock lock
 */
u8 dw_mc_get_lock(dw_mc_lock_e type);

/**
 * @desc: main control all clock enable
 */
void dw_mc_clk_all_enable(void);

/**
 * @desc: main control all clock disable
 */
void dw_mc_clk_all_disable(void);

int dw_mc_irq_mute(dw_mc_irq_e irq_source);

u8 dw_mc_irq_get_state(dw_mc_irq_e irq);

int dw_mc_irq_clear_state(dw_mc_irq_e irq, u8 state);

/**
 * @desc: main control set irq unmute by source
 */
int dw_mc_irq_unmute(dw_mc_irq_e irq_source);
/**
 * @desc: main control set main irq
 * @state: 1 - enable main irq
 *         0 - disable main irq
 */
void dw_mc_set_main_irq(u8 state);
/**
 * @desc: main control mask all irq
 */
void dw_mc_irq_mask_all(void);

/*****************************************************************************
 *                                                                           *
 *                            Interrupt Registers                            *
 *                                                                           *
 *****************************************************************************/

/**
 * @desc: Frame Composer Interrupt Status Register 0 (Packet Interrupts)
 * @bits: [0:0] Active after successful transmission of an Null packet
 *        [1:1] Active after successful transmission of an Audio Clock Regeneration (N/CTS transmission) packet
 *        [2:2] Active after successful transmission of an Audio Sample packet
 *        [3:3] Active after successful transmission of an NTSC VBI packet
 *        [4:4] Active after successful transmission of an MultiStream Audio packet
 *        [5:5] Active after successful transmission of an Audio HBR packet
 *        [6:6] Active after successful transmission of an Audio Content Protection packet
 *        [7:7] Active after successful transmission of an Audio InfoFrame packet
 */
#define IH_FC_STAT0         0x00000100
#define IH_FC_STAT0_NULL_MASK               0x00000001
#define IH_FC_STAT0_ACR_MASK                0x00000002
#define IH_FC_STAT0_AUDS_MASK               0x00000004
#define IH_FC_STAT0_NVBI_MASK               0x00000008
#define IH_FC_STAT0_MAS_MASK                0x00000010
#define IH_FC_STAT0_HBR_MASK                0x00000020
#define IH_FC_STAT0_ACP_MASK                0x00000040
#define IH_FC_STAT0_AUDI_MASK               0x00000080

/**
 * @desc: Frame Composer Interrupt Status Register 1 (Packet Interrupts)
 * @bits: [0:0] Active after successful transmission of an General Control Packet
 *        [1:1] Active after successful transmission of an AVI InfoFrame packet
 *        [2:2] Active after successful transmission of an Audio Metadata packet
 *        [3:3] Active after successful transmission of an Source Product Descriptor InfoFrame packet
 *        [4:4] Active after successful transmission of an Vendor Specific Data InfoFrame packet
 *        [5:5] Active after successful transmission of an International Standard Recording Code 2 packet
 *        [6:6] Active after successful transmission of an International Standard Recording Code 1 packet
 *        [7:7] Active after successful transmission of an Gamut metadata packet
 */
#define IH_FC_STAT1         0x00000101
#define IH_FC_STAT1_GCP_MASK                0x00000001
#define IH_FC_STAT1_AVI_MASK                0x00000002
#define IH_FC_STAT1_AMP_MASK                0x00000004
#define IH_FC_STAT1_SPD_MASK                0x00000008
#define IH_FC_STAT1_VSD_MASK                0x00000010
#define IH_FC_STAT1_ISCR2_MASK              0x00000020
#define IH_FC_STAT1_ISCR1_MASK              0x00000040
#define IH_FC_STAT1_GMD_MASK                0x00000080

/**
 * @desc: Frame Composer Interrupt Status Register 2 (Packet Queue Overflow Interrupts)
 * @bits: [0:0] Frame Composer high priority packet queue descriptor overflow indication
 *        [1:1] Frame Composer low priority packet queue descriptor overflow indication
 */
#define IH_FC_STAT2         0x00000102
#define IH_FC_STAT2_HIGHPRIORITY_OVERFLOW_MASK  0x00000001
#define IH_FC_STAT2_LOWPRIORITY_OVERFLOW_MASK   0x00000002

/**
 * @desc: Audio Sampler Interrupt Status Register (FIFO Threshold, Underflow and Overflow Interrupts)
 * @bits: [0:0] Audio Sampler audio FIFO full indication
 *        [1:1] Audio Sampler audio FIFO empty indication
 *        [2:2] Audio Sampler audio FIFO empty threshold (four samples) indication for the legacy HBR audio interface
 *        [3:3] Indicates an overrun on the audio FIFO
 *        [4:4] Indicates an underrun on the audio FIFO
 */
#define IH_AS_STAT0         0x00000103
#define IH_AS_STAT0_AUD_FIFO_OVERFLOW_MASK          0x00000001
#define IH_AS_STAT0_AUD_FIFO_UNDERFLOW_MASK         0x00000002
#define IH_AS_STAT0_AUD_FIFO_UNDERFLOW_THR_MASK     0x00000004
#define IH_AS_STAT0_FIFO_OVERRUN_MASK               0x00000008
#define IH_AS_STAT0_FIFO_UNDERRUN_MASK              0x00000010

/**
 * @desc: PHY Interface Interrupt Status Register (RXSENSE, PLL Lock and HPD Interrupts)
 * @bits: [0:0] HDMI Hot Plug Detect indication
 *        [1:1] TX PHY PLL lock indication
 *        [2:2] TX PHY RX_SENSE indication for driver 0
 *        [3:3] TX PHY RX_SENSE indication for driver 1
 *        [4:4] TX PHY RX_SENSE indication for driver 2
 *        [5:5] TX PHY RX_SENSE indication for driver 3
 */
#define IH_PHY_STAT0        0x00000104
#define IH_PHY_STAT0_HPD_MASK                       0x00000001
#define IH_PHY_STAT0_TX_PHY_LOCK_MASK               0x00000002
#define IH_PHY_STAT0_RX_SENSE_0_MASK                0x00000004
#define IH_PHY_STAT0_RX_SENSE_1_MASK                0x00000008
#define IH_PHY_STAT0_RX_SENSE_2_MASK                0x00000010
#define IH_PHY_STAT0_RX_SENSE_3_MASK                0x00000020

/**
 * @desc: E-DDC I2C Master Interrupt Status Register (Done and Error Interrupts)
 * @bits: [0:0] I2C Master error indication
 *        [1:1] I2C Master done indication
 *        [2:2] I2C Master SCDC read request indication
 */
#define IH_I2CM_STAT0       0x00000105
#define IH_I2CM_STAT0_I2CMASTERERROR_MASK           0x00000001
#define IH_I2CM_STAT0_I2CMASTERDONE_MASK            0x00000002
#define IH_I2CM_STAT0_SCDC_READREQ_MASK             0x00000004

/**
 * @desc: CEC Interrupt Status Register (Functional Operation Interrupts)
 * @bits: [0:0] CEC Done Indication
 *        [1:1] CEC End of Message Indication
 *        [2:2] CEC Not Acknowledge indication
 *        [3:3] CEC Arbitration Lost indication
 *        [4:4] CEC Error Initiator indication
 *        [5:5] CEC Error Follow indication
 *        [6:6] CEC Wake-up indication
 */
#define IH_CEC_STAT0        0x00000106
#define IH_CEC_STAT0_DONE_MASK                      0x00000001
#define IH_CEC_STAT0_EOM_MASK                       0x00000002
#define IH_CEC_STAT0_NACK_MASK                      0x00000004
#define IH_CEC_STAT0_ARB_LOST_MASK                  0x00000008
#define IH_CEC_STAT0_ERROR_INITIATOR_MASK           0x00000010
#define IH_CEC_STAT0_ERROR_FOLLOW_MASK              0x00000020
#define IH_CEC_STAT0_WAKEUP_MASK                    0x00000040

/**
 * @desc: Video Packetizer Interrupt Status Register (FIFO Full and Empty Interrupts)
 * @bits: [0:0] Video Packetizer 8 bit bypass FIFO empty interrupt
 *        [1:1] Video Packetizer 8 bit bypass FIFO full interrupt
 *        [2:2] Video Packetizer pixel YCC 422 re-mapper FIFO empty interrupt
 *        [3:3] Video Packetizer pixel YCC 422 re-mapper FIFO full interrupt
 *        [4:4] Video Packetizer pixel packing FIFO empty interrupt
 *        [5:5] Video Packetizer pixel packing FIFO full interrupt
 *        [6:6] Video Packetizer pixel repeater FIFO empty interrupt
 *        [7:7] Video Packetizer pixel repeater FIFO full interrupt
 */
#define IH_VP_STAT0         0x00000107
#define IH_VP_STAT0_FIFOEMPTYBYP_MASK               0x00000001
#define IH_VP_STAT0_FIFOFULLBYP_MASK                0x00000002
#define IH_VP_STAT0_FIFOEMPTYREMAP_MASK             0x00000004
#define IH_VP_STAT0_FIFOFULLREMAP_MASK              0x00000008
#define IH_VP_STAT0_FIFOEMPTYPP_MASK                0x00000010
#define IH_VP_STAT0_FIFOFULLPP_MASK                 0x00000020
#define IH_VP_STAT0_FIFOEMPTYREPET_MASK             0x00000040
#define IH_VP_STAT0_FIFOFULLREPET_MASK              0x00000080

/**
 * @desc: PHY GEN2 I2C Master Interrupt Status Register (Done and Error Interrupts)
 * @bits: [0:0] I2C Master PHY error indication
 *        [1:1] I2C Master PHY done indication
 */
#define IH_I2CMPHY_STAT0    0x00000108
#define IH_I2CMPHY_STAT0_I2CMPHYERROR_MASK          0x00000001
#define IH_I2CMPHY_STAT0_I2CMPHYDONE_MASK           0x00000002

/**
 * @desc: DMA - not supported in this build
 */
#define IH_AHBDMAAUD_STAT0  0x00000109

/**
 * @desc: Interruption Handler Decode Assist Register
 * @bits: [0:0] Interruption active at the ih_ahbdmaaud_stat0 register
 *        [1:1] Interruption active at the ih_cec_stat0 register
 *        [2:2] Interruption active at the ih_i2cm_stat0 register
 *        [3:3] Interruption active at the ih_phy_stat0 or ih_i2cmphy_stat0 register
 *        [4:4] Interruption active at the ih_as_stat0 register
 *        [5:5] Interruption active at the ih_fc_stat2 or ih_vp_stat0 register
 *        [6:6] Interruption active at the ih_fc_stat1 register
 *        [7:7] Interruption active at the ih_fc_stat0 register
 */
#define IH_DECODE           0x00000170
#define IH_DECODE_IH_AHBDMAAUD_STAT0_MASK           0x00000001
#define IH_DECODE_IH_CEC_STAT0_MASK                 0x00000002
#define IH_DECODE_IH_I2CM_STAT0_MASK                0x00000004
#define IH_DECODE_IH_PHY_MASK                       0x00000008
#define IH_DECODE_IH_AS_STAT0_MASK                  0x00000010
#define IH_DECODE_IH_FC_STAT2_VP_MASK               0x00000020
#define IH_DECODE_IH_FC_STAT1_MASK                  0x00000040
#define IH_DECODE_IH_FC_STAT0_MASK                  0x00000080

/**
 * @desc: Frame Composer Interrupt Mute Control Register 0
 * @bits: [0:0] When set to 1, mutes ih_fc_stat0[0]
 *        [1:1] When set to 1, mutes ih_fc_stat0[1]
 *        [2:2] When set to 1, mutes ih_fc_stat0[2]
 *        [3:3] When set to 1, mutes ih_fc_stat0[3]
 *        [4:4] When set to 1, mutes ih_fc_stat0[4]
 *        [5:5] When set to 1, mutes ih_fc_stat0[5]
 *        [6:6] When set to 1, mutes ih_fc_stat0[6]
 *        [7:7] When set to 1, mutes ih_fc_stat0[7]
 */
#define IH_MUTE_FC_STAT0        0x00000180
#define IH_MUTE_FC_STAT0_NULL_MASK                  0x00000001
#define IH_MUTE_FC_STAT0_ACR_MASK                   0x00000002
#define IH_MUTE_FC_STAT0_AUDS_MASK                  0x00000004
#define IH_MUTE_FC_STAT0_NVBI_MASK                  0x00000008
#define IH_MUTE_FC_STAT0_MAS_MASK                   0x00000010
#define IH_MUTE_FC_STAT0_HBR_MASK                   0x00000020
#define IH_MUTE_FC_STAT0_ACP_MASK                   0x00000040
#define IH_MUTE_FC_STAT0_AUDI_MASK                  0x00000080

/**
 * @desc: Frame Composer Interrupt Mute Control Register 1
 * @bits: [0:0] When set to 1, mutes ih_fc_stat1[0]
 *        [1:1] When set to 1, mutes ih_fc_stat1[1]
 *        [2:2] When set to 1, mutes ih_fc_stat1[2]
 *        [3:3] When set to 1, mutes ih_fc_stat1[3]
 *        [4:4] When set to 1, mutes ih_fc_stat1[4]
 *        [5:5] When set to 1, mutes ih_fc_stat1[5]
 *        [6:6] When set to 1, mutes ih_fc_stat1[6]
 *        [7:7] When set to 1, mutes ih_fc_stat1[7]
 */
#define IH_MUTE_FC_STAT1        0x00000181
#define IH_MUTE_FC_STAT1_GCP_MASK                   0x00000001
#define IH_MUTE_FC_STAT1_AVI_MASK                   0x00000002
#define IH_MUTE_FC_STAT1_AMP_MASK                   0x00000004
#define IH_MUTE_FC_STAT1_SPD_MASK                   0x00000008
#define IH_MUTE_FC_STAT1_VSD_MASK                   0x00000010
#define IH_MUTE_FC_STAT1_ISCR2_MASK                 0x00000020
#define IH_MUTE_FC_STAT1_ISCR1_MASK                 0x00000040
#define IH_MUTE_FC_STAT1_GMD_MASK                   0x00000080

/**
 * @desc: Frame Composer Interrupt Mute Control Register 2
 * @bits: [0:0] When set to 1, mutes ih_fc_stat2[0]
 *        [1:1] When set to 1, mutes ih_fc_stat2[1]
 */
#define IH_MUTE_FC_STAT2        0x00000182
#define IH_MUTE_FC_STAT2_HIGHPRIORITY_OVERFLOW_MASK 0x00000001
#define IH_MUTE_FC_STAT2_LOWPRIORITY_OVERFLOW_MASK  0x00000002

/**
 * @desc: Audio Sampler Interrupt Mute Control Register
 * @bits: [0:0] When set to 1, mutes ih_as_stat0[0]
 *        [1:1] When set to 1, mutes ih_as_stat0[1]
 *        [2:2] When set to 1, mutes ih_as_stat0[2]
 *        [3:3] When set to 1, mutes ih_as_stat0[3]
 */
#define IH_MUTE_AS_STAT0        0x00000183
#define IH_MUTE_AS_STAT0_AUD_FIFO_OVERFLOW_MASK         0x00000001
#define IH_MUTE_AS_STAT0_AUD_FIFO_UNDERFLOW_MASK        0x00000002
#define IH_MUTE_AS_STAT0_AUD_FIFO_UNDERFLOW_THR_MASK    0x00000004
#define IH_MUTE_AS_STAT0_FIFO_OVERRUN_MASK              0x00000008

/**
 * @desc: PHY Interface Interrupt Mute Control Register
 * @bits: [0:0] When set to 1, mutes ih_phy_stat0[0]
 *        [1:1] When set to 1, mutes ih_phy_stat0[1]
 *        [2:2] When set to 1, mutes ih_phy_stat0[2]
 *        [3:3] When set to 1, mutes ih_phy_stat0[3]
 *        [4:4] When set to 1, mutes ih_phy_stat0[4]
 *        [5:5] When set to 1, mutes ih_phy_stat0[5]
 */
#define IH_MUTE_PHY_STAT0       0x00000184
#define IH_MUTE_PHY_STAT0_HPD_MASK                      0x00000001
#define IH_MUTE_PHY_STAT0_TX_PHY_LOCK_MASK              0x00000002
#define IH_MUTE_PHY_STAT0_RX_SENSE_0_MASK               0x00000004
#define IH_MUTE_PHY_STAT0_RX_SENSE_1_MASK               0x00000008
#define IH_MUTE_PHY_STAT0_RX_SENSE_2_MASK               0x00000010
#define IH_MUTE_PHY_STAT0_RX_SENSE_3_MASK               0x00000020

/**
 * @desc: E-DDC I2C Master Interrupt Mute Control Register
 * @bits: [0:0] When set to 1, mutes ih_i2cm_stat0[0]
 *        [1:1] When set to 1, mutes ih_i2cm_stat0[1]
 *        [2:2] When set to 1, mutes ih_i2cm_stat0[2]
 */
#define IH_MUTE_I2CM_STAT0      0x00000185
#define IH_MUTE_I2CM_STAT0_I2CMASTERERROR_MASK          0x00000001
#define IH_MUTE_I2CM_STAT0_I2CMASTERDONE_MASK           0x00000002
#define IH_MUTE_I2CM_STAT0_SCDC_READREQ_MASK            0x00000004

/**
 * @desc: CEC Interrupt Mute Control Register
 * @bits: [0:0] When set to 1, mutes ih_cec_stat0[0]
 *        [1:1] When set to 1, mutes ih_cec_stat0[1]
 *        [2:2] When set to 1, mutes ih_cec_stat0[2]
 *        [3:3] When set to 1, mutes ih_cec_stat0[3]
 *        [4:4] When set to 1, mutes ih_cec_stat0[4]
 *        [5:5] When set to 1, mutes ih_cec_stat0[5]
 *        [6:6] When set to 1, mutes ih_cec_stat0[6]
 */
#define IH_MUTE_CEC_STAT0       0x00000186
#define IH_MUTE_CEC_STAT0_DONE_MASK                     0x00000001
#define IH_MUTE_CEC_STAT0_EOM_MASK                      0x00000002
#define IH_MUTE_CEC_STAT0_NACK_MASK                     0x00000004
#define IH_MUTE_CEC_STAT0_ARB_LOST_MASK                 0x00000008
#define IH_MUTE_CEC_STAT0_ERROR_INITIATOR_MASK          0x00000010
#define IH_MUTE_CEC_STAT0_ERROR_FOLLOW_MASK             0x00000020
#define IH_MUTE_CEC_STAT0_WAKEUP_MASK                   0x00000040

/**
 * @desc: Video Packetizer Interrupt Mute Control Register
 * @bits: [0:0] When set to 1, mutes ih_vp_stat0[0]
 *        [1:1] When set to 1, mutes ih_vp_stat0[1]
 *        [2:2] When set to 1, mutes ih_vp_stat0[2]
 *        [3:3] When set to 1, mutes ih_vp_stat0[3]
 *        [4:4] When set to 1, mutes ih_vp_stat0[4]
 *        [5:5] When set to 1, mutes ih_vp_stat0[5]
 *        [6:6] When set to 1, mutes ih_vp_stat0[6]
 *        [7:7] When set to 1, mutes ih_vp_stat0[7]
 */
#define IH_MUTE_VP_STAT0        0x00000187
#define IH_MUTE_VP_STAT0_FIFOEMPTYBYP_MASK              0x00000001
#define IH_MUTE_VP_STAT0_FIFOFULLBYP_MASK               0x00000002
#define IH_MUTE_VP_STAT0_FIFOEMPTYREMAP_MASK            0x00000004
#define IH_MUTE_VP_STAT0_FIFOFULLREMAP_MASK             0x00000008
#define IH_MUTE_VP_STAT0_FIFOEMPTYPP_MASK               0x00000010
#define IH_MUTE_VP_STAT0_FIFOFULLPP_MASK                0x00000020
#define IH_MUTE_VP_STAT0_FIFOEMPTYREPET_MASK            0x00000040
#define IH_MUTE_VP_STAT0_FIFOFULLREPET_MASK             0x00000080

/**
 * @desc: PHY GEN2 I2C Master Interrupt Mute Control Register
 * @bits: [0:0] When set to 1, mutes ih_i2cmphy_stat0[0]
 *        [1:1] When set to 1, mutes ih_i2cmphy_stat0[1]
 */
#define IH_MUTE_I2CMPHY_STAT0   0x00000188
#define IH_MUTE_I2CMPHY_STAT0_I2CMPHYERROR_MASK         0x00000001
#define IH_MUTE_I2CMPHY_STAT0_I2CMPHYDONE_MASK          0x00000002

/**
 * @desc: AHB Audio DMA Interrupt Mute Control Register
 * @bits: [0:0] When set to 1, mutes ih_ahbdmaaud_stat0[0]
 *        [1:1] When set to 1, mutes ih_ahbdmaaud_stat0[1]
 *        [2:2] When set to 1, mutes ih_ahbdmaaud_stat0[2]
 *        [3:3] When set to 1, mutes ih_ahbdmaaud_stat0[3]
 *        [4:4] When set to 1, mutes ih_ahbdmaaud_stat0[4]
 *        [5:5] When set to 1, mutes ih_ahbdmaaud_stat0[5]
 *        [6:6] When set to 1, mutes ih_ahbdmaaud_stat0[6]
 */
#define IH_MUTE_AHBDMAAUD_STAT0     0x00000189
#define IH_MUTE_AHBDMAAUD_STAT0_INTBUFFEMPTY_MASK       0x00000001
#define IH_MUTE_AHBDMAAUD_STAT0_INTBUFFULL_MASK         0x00000002
#define IH_MUTE_AHBDMAAUD_STAT0_INTDONE_MASK            0x00000004
#define IH_MUTE_AHBDMAAUD_STAT0_INTINTERTRYSPLIT_MASK   0x00000008
#define IH_MUTE_AHBDMAAUD_STAT0_INTLOSTOWNERSHIP_MASK   0x00000010
#define IH_MUTE_AHBDMAAUD_STAT0_INTERROR_MASK           0x00000020
#define IH_MUTE_AHBDMAAUD_STAT0_INTBUFFOVERRUN_MASK     0x00000040

/**
 * @desc: Global Interrupt Mute Control Register
 * @bits: [0:0] When set to 1, mutes the main interrupt line (where all interrupts are ORed)
 *        [1:1] When set to 1, mutes the main interrupt output port
 */
#define IH_MUTE                     0x000001FF
#define IH_MUTE_MUTE_ALL_INTERRUPT_MASK                 0x00000001
#define IH_MUTE_MUTE_WAKEUP_INTERRUPT_MASK              0x00000002

/*****************************************************************************
 *                                                                           *
 *                         Main Controller Registers                         *
 *                                                                           *
 *****************************************************************************/

/**
 * @desc: Main Controller Synchronous Clock Domain Disable Register
 * @bits: [0:0] Pixel clock synchronous disable signal
 *        [1:1] TMDS clock synchronous disable signal
 *        [2:2] Pixel Repetition clock synchronous disable signal
 *        [3:3] Audio Sampler clock synchronous disable signal
 *        [4:4] Color Space Converter clock synchronous disable signal
 *        [5:5] CEC Engine clock synchronous disable signal
 *        [6:6] HDCP clock synchronous disable signal
 */
#define MC_CLKDIS           0x00004001
#define MC_CLKDIS_PIXELCLK_DISABLE_MASK     0x00000001
#define MC_CLKDIS_TMDSCLK_DISABLE_MASK      0x00000002
#define MC_CLKDIS_PREPCLK_DISABLE_MASK      0x00000004
#define MC_CLKDIS_AUDCLK_DISABLE_MASK       0x00000008
#define MC_CLKDIS_CSCCLK_DISABLE_MASK       0x00000010
#define MC_CLKDIS_CECCLK_DISABLE_MASK       0x00000020
#define MC_CLKDIS_HDCPCLK_DISABLE_MASK      0x00000040

/**
 * @desc: Main Controller Software Reset Register
 * Main controller software reset request per clock domain
 * write 0 is request reset
 * @bits: [0:0] Pixel software reset request
 *        [1:1] TMDS software reset request
 *        [2:2] Pixel Repetition software reset request
 *        [3:3] I2S audio software reset request
 *        [4:4] SPDIF audio software reset request
 *        [6:6] CEC software reset request
 *        [7:7] GPAUD interface soft reset request
 */
#define MC_SWRSTZREQ        0x00004002
#define MC_SWRSTZREQ_PIXELSWRST_REQ_MASK        0x00000001
#define MC_SWRSTZREQ_TMDSSWRST_REQ_MASK         0x00000002
#define MC_SWRSTZREQ_PREPSWRST_REQ_MASK         0x00000004
#define MC_SWRSTZREQ_II2SSWRST_REQ_MASK         0x00000008
#define MC_SWRSTZREQ_ISPDIFSWRST_REQ_MASK       0x00000010
#define MC_SWRSTZREQ_CECSWRST_REQ_MASK          0x00000040
#define MC_SWRSTZREQ_IGPASWRST_REQ_MASK         0x00000080

/**
 * @desc: Main Controller HDCP Bypass Control Register
 * @bits: [0:0] Block HDCP bypass mechanism - 1'b0: This is the default value
 */
#define MC_OPCTRL           0x00004003
#define MC_OPCTRL_HDCP_BLOCK_BYP_MASK           0x00000001

/**
 * @desc: Main Controller Feed Through Control Register
 * @bits: [0:0] Video path Feed Through enable bit
 *          - 1: Color Space Converter is in the video data path
 *          - 0: Color Space Converter is bypassed
 */
#define MC_FLOWCTRL         0x00004004
#define MC_FLOWCTRL_FEED_THROUGH_OFF_MASK       0x00000001

/**
 * @desc: Main Controller PHY Reset Register
 * @bits: [0:0] HDMI Source PHY active low reset control for PHY GEN1, active high reset control for PHY GEN2
 */
#define MC_PHYRSTZ          0x00004005
#define MC_PHYRSTZ_PHYRSTZ_MASK                 0x00000001

/**
 * @desc: Main Controller Clock Present Register
 * @bits: [0:0] CEC clock status
 *        [2:2] SPDIF clock status
 *        [3:3] I2S clock status
 *        [4:4] Pixel Repetition clock status
 *        [5:5] TMDS clock status
 *        [6:6] Pixel clock status
 *        [7:7] GPAUD interface clock status
 */
#define MC_LOCKONCLOCK          0x00004006
#define MC_LOCKONCLOCK_CECCLK_MASK         0x00000001
#define MC_LOCKONCLOCK_SPDIFCLK_MASK       0x00000004
#define MC_LOCKONCLOCK_I2SCLK_MASK         0x00000008
#define MC_LOCKONCLOCK_PREPCLK_MASK        0x00000010
#define MC_LOCKONCLOCK_TCLK_MASK           0x00000020
#define MC_LOCKONCLOCK_PCLK_MASK           0x00000040
#define MC_LOCKONCLOCK_IGPACLK_MASK        0x00000080

#define MC_HEACPHYRSTZ          0x00004007
#define MC_HEACPHYRSTZ_MASK                0x00000001

#define MC_LOCKONCLOCK2         0x00004009
#define MC_LOCKONCLOCK_AHBDMA_MASK         0x00000001

#define MC_AHBDMARSTZ           0x0000400A
#define MC_AHBDMARSTZ_MASK                 0x00000001

#endif  /* _DW_MC_H_ */
