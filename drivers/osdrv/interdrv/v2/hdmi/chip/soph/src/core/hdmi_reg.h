#ifndef _HDMI_REG_H_
#define _HDMI_REG_H_

//Design Identification Register
#define DESIGN_ID  0x00000000
#define DESIGN_ID_DESIGN_ID_MASK  0x000000FF //Design ID code fixed by Synopsys that Identifies the instantiated DWC_hdmi_tx controller

//Revision Identification Register
#define REVISION_ID  0x00000004
#define REVISION_ID_REVISION_ID_MASK  0x000000FF //Revision ID code fixed by Synopsys that Identifies the instantiated DWC_hdmi_tx controller

//Product Identification Register 0
#define PRODUCT_ID0  0x00000008
#define PRODUCT_ID0_PRODUCT_ID0_MASK  0x000000FF //This one byte fixed code Identifies Synopsys's product line ("A0h" for DWC_hdmi_tx products)

//Product Identification Register 1
#define PRODUCT_ID1  0x0000000C
#define PRODUCT_ID1_PRODUCT_ID1_TX_MASK  0x00000001 //This bit Identifies Synopsys's DWC_hdmi_tx Controller according to Synopsys product line
#define PRODUCT_ID1_PRODUCT_ID1_RX_MASK  0x00000002 //This bit Identifies Synopsys's DWC_hdmi_rx Controller according to Synopsys product line
#define PRODUCT_ID1_PRODUCT_ID1_HDCP_MASK  0x000000C0 //These bits identify a Synopsys's HDMI Controller with HDCP encryption according to Synopsys product line

//Configuration Identification Register 0
#define CONFIG0_ID  0x00000010
#define CONFIG0_ID_HDCP_MASK  0x00000001 //Indicates if HDCP is present
#define CONFIG0_ID_CEC_MASK  0x00000002 //Indicates if CEC is present
#define CONFIG0_ID_CSC_MASK  0x00000004 //Indicates if Color Space Conversion block is present
#define CONFIG0_ID_HDMI14_MASK  0x00000008 //Indicates if HDMI 1
#define CONFIG0_ID_AUDI2S_MASK  0x00000010 //Indicates if I2S interface is present
#define CONFIG0_ID_AUDSPDIF_MASK  0x00000020 //Indicates if the SPDIF audio interface is present
#define CONFIG0_ID_PREPEN_MASK  0x00000080 //Indicates if it is possible to use internal pixel repetition

//Configuration Identification Register 1
#define CONFIG1_ID  0x00000014
#define CONFIG1_ID_CONFAPB_MASK  0x00000002 //Indicates that configuration interface is APB interface
#define CONFIG1_ID_HDMI20_MASK  0x00000020 //Indicates if HDMI 2
#define CONFIG1_ID_HDCP22_EXT_MASK  0x00000040 //Indicates if HDCP 2.2 External is present
#define CONFIG1_ID_HDCP22_SNPS_MASK  0x00000080 //Indicates if HDCP 2.2 SNPS is present

//Configuration Identification Register 2
#define CONFIG2_ID  0x00000018
#define CONFIG2_ID_PHYTYPE_MASK  0x000000FF //Indicates the type of PHY interface selected: 0x00: Legacy PHY (HDMI TX PHY) 0xF2: PHY GEN2 (HDMI 3D TX PHY) 0xE2: PHY GEN2 (HDMI 3D TX PHY) + HEAC PHY 0xC2: PHY MHL COMBO (MHL+HDMI 2

//Configuration Identification Register 3
#define CONFIG3_ID  0x0000001C
#define CONFIG3_ID_CONFGPAUD_MASK  0x00000001 //Indicates that the audio interface is Generic Parallel Audio (GPAUD)
#define CONFIG3_ID_CONFAHBAUDDMA_MASK  0x00000002 //Indicates that the audio interface is AHB AUD DMA

//Frame Composer Interrupt Status Register 0 (Packet Interrupts)
#define IH_FC_STAT0  0x00000400
#define IH_FC_STAT0_NULL_MASK  0x00000001 //Active after successful transmission of an Null packet
#define IH_FC_STAT0_ACR_MASK  0x00000002 //Active after successful transmission of an Audio Clock Regeneration (N/CTS transmission) packet
#define IH_FC_STAT0_AUDS_MASK  0x00000004 //Active after successful transmission of an Audio Sample packet
#define IH_FC_STAT0_NVBI_MASK  0x00000008 //Active after successful transmission of an NTSC VBI packet
#define IH_FC_STAT0_MAS_MASK  0x00000010 //Active after successful transmission of an MultiStream Audio packet
#define IH_FC_STAT0_HBR_MASK  0x00000020 //Active after successful transmission of an Audio HBR packet
#define IH_FC_STAT0_ACP_MASK  0x00000040 //Active after successful transmission of an Audio Content Protection packet
#define IH_FC_STAT0_AUDI_MASK  0x00000080 //Active after successful transmission of an Audio InfoFrame packet

//Frame Composer Interrupt Status Register 1 (Packet Interrupts)
#define IH_FC_STAT1  0x00000404
#define IH_FC_STAT1_GCP_MASK  0x00000001 //Active after successful transmission of an General Control Packet
#define IH_FC_STAT1_AVI_MASK  0x00000002 //Active after successful transmission of an AVI InfoFrame packet
#define IH_FC_STAT1_AMP_MASK  0x00000004 //Active after successful transmission of an Audio Metadata packet
#define IH_FC_STAT1_SPD_MASK  0x00000008 //Active after successful transmission of an Source Product Descriptor InfoFrame packet
#define IH_FC_STAT1_VSD_MASK  0x00000010 //Active after successful transmission of an Vendor Specific Data InfoFrame packet
#define IH_FC_STAT1_ISCR2_MASK  0x00000020 //Active after successful transmission of an International Standard Recording Code 2 packet
#define IH_FC_STAT1_ISCR1_MASK  0x00000040 //Active after successful transmission of an International Standard Recording Code 1 packet
#define IH_FC_STAT1_GMD_MASK  0x00000080 //Active after successful transmission of an Gamut metadata packet

//Frame Composer Interrupt Status Register 2 (Packet Queue Overflow Interrupts)
#define IH_FC_STAT2  0x00000408
#define IH_FC_STAT2_HIGHPRIORITY_OVERFLOW_MASK  0x00000001 //Frame Composer high priority packet queue descriptor overflow indication
#define IH_FC_STAT2_LOWPRIORITY_OVERFLOW_MASK  0x00000002 //Frame Composer low priority packet queue descriptor overflow indication

//Audio Sampler Interrupt Status Register (FIFO Threshold, Underflow and Overflow Interrupts)
#define IH_AS_STAT0  0x0000040C
#define IH_AS_STAT0_AUD_FIFO_OVERFLOW_MASK  0x00000001 //Audio Sampler audio FIFO full indication
#define IH_AS_STAT0_AUD_FIFO_UNDERFLOW_MASK  0x00000002 //Audio Sampler audio FIFO empty indication
#define IH_AS_STAT0_AUD_FIFO_UNDERFLOW_THR_MASK  0x00000004 //Audio Sampler audio FIFO empty threshold (four samples) indication for the legacy HBR audio interface
#define IH_AS_STAT0_FIFO_OVERRUN_MASK  0x00000008 //Indicates an overrun on the audio FIFO

//PHY Interface Interrupt Status Register (RXSENSE, PLL Lock and HPD Interrupts)
#define IH_PHY_STAT0  0x00000410
#define IH_PHY_STAT0_HPD_MASK  0x00000001 //HDMI Hot Plug Detect indication
#define IH_PHY_STAT0_TX_PHY_LOCK_MASK  0x00000002 //TX PHY PLL lock indication
#define IH_PHY_STAT0_RX_SENSE_0_MASK  0x00000004 //TX PHY RX_SENSE indication for driver 0
#define IH_PHY_STAT0_RX_SENSE_1_MASK  0x00000008 //TX PHY RX_SENSE indication for driver 1
#define IH_PHY_STAT0_RX_SENSE_2_MASK  0x00000010 //TX PHY RX_SENSE indication for driver 2
#define IH_PHY_STAT0_RX_SENSE_3_MASK  0x00000020 //TX PHY RX_SENSE indication for driver 3

//E-DDC I2C Master Interrupt Status Register (Done and Error Interrupts)
#define IH_I2CM_STAT0  0x00000414
#define IH_I2CM_STAT0_I2CMASTERERROR_MASK  0x00000001 //I2C Master error indication
#define IH_I2CM_STAT0_I2CMASTERDONE_MASK  0x00000002 //I2C Master done indication
#define IH_I2CM_STAT0_SCDC_READREQ_MASK  0x00000004 //I2C Master SCDC read request indication

//CEC Interrupt Status Register (Functional Operation Interrupts)
#define IH_CEC_STAT0  0x00000418
#define IH_CEC_STAT0_DONE_MASK  0x00000001 //CEC Done Indication
#define IH_CEC_STAT0_EOM_MASK  0x00000002 //CEC End of Message Indication
#define IH_CEC_STAT0_NACK_MASK  0x00000004 //CEC Not Acknowledge indication
#define IH_CEC_STAT0_ARB_LOST_MASK  0x00000008 //CEC Arbitration Lost indication
#define IH_CEC_STAT0_ERROR_INITIATOR_MASK  0x00000010 //CEC Error Initiator indication
#define IH_CEC_STAT0_ERROR_FOLLOW_MASK  0x00000020 //CEC Error Follow indication
#define IH_CEC_STAT0_WAKEUP_MASK  0x00000040 //CEC Wake-up indication

//Video Packetizer Interrupt Status Register (FIFO Full and Empty Interrupts)
#define IH_VP_STAT0  0x0000041C
#define IH_VP_STAT0_FIFOEMPTYBYP_MASK  0x00000001 //Video Packetizer 8 bit bypass FIFO empty interrupt
#define IH_VP_STAT0_FIFOFULLBYP_MASK  0x00000002 //Video Packetizer 8 bit bypass FIFO full interrupt
#define IH_VP_STAT0_FIFOEMPTYREMAP_MASK  0x00000004 //Video Packetizer pixel YCC 422 re-mapper FIFO empty interrupt
#define IH_VP_STAT0_FIFOFULLREMAP_MASK  0x00000008 //Video Packetizer pixel YCC 422 re-mapper FIFO full interrupt
#define IH_VP_STAT0_FIFOEMPTYPP_MASK  0x00000010 //Video Packetizer pixel packing FIFO empty interrupt
#define IH_VP_STAT0_FIFOFULLPP_MASK  0x00000020 //Video Packetizer pixel packing FIFO full interrupt
#define IH_VP_STAT0_FIFOEMPTYREPET_MASK  0x00000040 //Video Packetizer pixel repeater FIFO empty interrupt
#define IH_VP_STAT0_FIFOFULLREPET_MASK  0x00000080 //Video Packetizer pixel repeater FIFO full interrupt

//PHY GEN2 I2C Master Interrupt Status Register (Done and Error Interrupts)
#define IH_I2CMPHY_STAT0  0x00000420
#define IH_I2CMPHY_STAT0_I2CMPHYERROR_MASK  0x00000001 //I2C Master PHY error indication
#define IH_I2CMPHY_STAT0_I2CMPHYDONE_MASK  0x00000002 //I2C Master PHY done indication

//DMA - not supported in this build
#define IH_AHBDMAAUD_STAT0  0x00000424

//Interruption Handler Decode Assist Register
#define IH_DECODE  0x000005C0
#define IH_DECODE_IH_AHBDMAAUD_STAT0_MASK  0x00000001 //Interruption active at the ih_ahbdmaaud_stat0 register
#define IH_DECODE_IH_CEC_STAT0_MASK  0x00000002 //Interruption active at the ih_cec_stat0 register
#define IH_DECODE_IH_I2CM_STAT0_MASK  0x00000004 //Interruption active at the ih_i2cm_stat0 register
#define IH_DECODE_IH_PHY_MASK  0x00000008 //Interruption active at the ih_phy_stat0 or ih_i2cmphy_stat0 register
#define IH_DECODE_IH_AS_STAT0_MASK  0x00000010 //Interruption active at the ih_as_stat0 register
#define IH_DECODE_IH_FC_STAT2_VP_MASK  0x00000020 //Interruption active at the ih_fc_stat2 or ih_vp_stat0 register
#define IH_DECODE_IH_FC_STAT1_MASK  0x00000040 //Interruption active at the ih_fc_stat1 register
#define IH_DECODE_IH_FC_STAT0_MASK  0x00000080 //Interruption active at the ih_fc_stat0 register

//Frame Composer Interrupt Mute Control Register 0
#define IH_MUTE_FC_STAT0  0x00000600
#define IH_MUTE_FC_STAT0_NULL_MASK  0x00000001 //When set to 1, mutes ih_fc_stat0[0]
#define IH_MUTE_FC_STAT0_ACR_MASK  0x00000002 //When set to 1, mutes ih_fc_stat0[1]
#define IH_MUTE_FC_STAT0_AUDS_MASK  0x00000004 //When set to 1, mutes ih_fc_stat0[2]
#define IH_MUTE_FC_STAT0_NVBI_MASK  0x00000008 //When set to 1, mutes ih_fc_stat0[3]
#define IH_MUTE_FC_STAT0_MAS_MASK  0x00000010 //When set to 1, mutes ih_fc_stat0[4]
#define IH_MUTE_FC_STAT0_HBR_MASK  0x00000020 //When set to 1, mutes ih_fc_stat0[5]
#define IH_MUTE_FC_STAT0_ACP_MASK  0x00000040 //When set to 1, mutes ih_fc_stat0[6]
#define IH_MUTE_FC_STAT0_AUDI_MASK  0x00000080 //When set to 1, mutes ih_fc_stat0[7]

//Frame Composer Interrupt Mute Control Register 1
#define IH_MUTE_FC_STAT1  0x00000604
#define IH_MUTE_FC_STAT1_GCP_MASK  0x00000001 //When set to 1, mutes ih_fc_stat1[0]
#define IH_MUTE_FC_STAT1_AVI_MASK  0x00000002 //When set to 1, mutes ih_fc_stat1[1]
#define IH_MUTE_FC_STAT1_AMP_MASK  0x00000004 //When set to 1, mutes ih_fc_stat1[2]
#define IH_MUTE_FC_STAT1_SPD_MASK  0x00000008 //When set to 1, mutes ih_fc_stat1[3]
#define IH_MUTE_FC_STAT1_VSD_MASK  0x00000010 //When set to 1, mutes ih_fc_stat1[4]
#define IH_MUTE_FC_STAT1_ISCR2_MASK  0x00000020 //When set to 1, mutes ih_fc_stat1[5]
#define IH_MUTE_FC_STAT1_ISCR1_MASK  0x00000040 //When set to 1, mutes ih_fc_stat1[6]
#define IH_MUTE_FC_STAT1_GMD_MASK  0x00000080 //When set to 1, mutes ih_fc_stat1[7]

//Frame Composer Interrupt Mute Control Register 2
#define IH_MUTE_FC_STAT2  0x00000608
#define IH_MUTE_FC_STAT2_HIGHPRIORITY_OVERFLOW_MASK  0x00000001 //When set to 1, mutes ih_fc_stat2[0]
#define IH_MUTE_FC_STAT2_LOWPRIORITY_OVERFLOW_MASK  0x00000002 //When set to 1, mutes ih_fc_stat2[1]

//Audio Sampler Interrupt Mute Control Register
#define IH_MUTE_AS_STAT0  0x0000060C
#define IH_MUTE_AS_STAT0_AUD_FIFO_OVERFLOW_MASK  0x00000001 //When set to 1, mutes ih_as_stat0[0]
#define IH_MUTE_AS_STAT0_AUD_FIFO_UNDERFLOW_MASK  0x00000002 //When set to 1, mutes ih_as_stat0[1]
#define IH_MUTE_AS_STAT0_AUD_FIFO_UNDERFLOW_THR_MASK  0x00000004 //When set to 1, mutes ih_as_stat0[2]
#define IH_MUTE_AS_STAT0_FIFO_OVERRUN_MASK  0x00000008 //When set to 1, mutes ih_as_stat0[3]

//PHY Interface Interrupt Mute Control Register
#define IH_MUTE_PHY_STAT0  0x00000610
#define IH_MUTE_PHY_STAT0_HPD_MASK  0x00000001 //When set to 1, mutes ih_phy_stat0[0]
#define IH_MUTE_PHY_STAT0_TX_PHY_LOCK_MASK  0x00000002 //When set to 1, mutes ih_phy_stat0[1]
#define IH_MUTE_PHY_STAT0_RX_SENSE_0_MASK  0x00000004 //When set to 1, mutes ih_phy_stat0[2]
#define IH_MUTE_PHY_STAT0_RX_SENSE_1_MASK  0x00000008 //When set to 1, mutes ih_phy_stat0[3]
#define IH_MUTE_PHY_STAT0_RX_SENSE_2_MASK  0x00000010 //When set to 1, mutes ih_phy_stat0[4]
#define IH_MUTE_PHY_STAT0_RX_SENSE_3_MASK  0x00000020 //When set to 1, mutes ih_phy_stat0[5]

//E-DDC I2C Master Interrupt Mute Control Register
#define IH_MUTE_I2CM_STAT0  0x00000614
#define IH_MUTE_I2CM_STAT0_I2CMASTERERROR_MASK  0x00000001 //When set to 1, mutes ih_i2cm_stat0[0]
#define IH_MUTE_I2CM_STAT0_I2CMASTERDONE_MASK  0x00000002 //When set to 1, mutes ih_i2cm_stat0[1]
#define IH_MUTE_I2CM_STAT0_SCDC_READREQ_MASK  0x00000004 //When set to 1, mutes ih_i2cm_stat0[2]

//CEC Interrupt Mute Control Register
#define IH_MUTE_CEC_STAT0  0x00000618
#define IH_MUTE_CEC_STAT0_DONE_MASK  0x00000001 //When set to 1, mutes ih_cec_stat0[0]
#define IH_MUTE_CEC_STAT0_EOM_MASK  0x00000002 //When set to 1, mutes ih_cec_stat0[1]
#define IH_MUTE_CEC_STAT0_NACK_MASK  0x00000004 //When set to 1, mutes ih_cec_stat0[2]
#define IH_MUTE_CEC_STAT0_ARB_LOST_MASK  0x00000008 //When set to 1, mutes ih_cec_stat0[3]
#define IH_MUTE_CEC_STAT0_ERROR_INITIATOR_MASK  0x00000010 //When set to 1, mutes ih_cec_stat0[4]
#define IH_MUTE_CEC_STAT0_ERROR_FOLLOW_MASK  0x00000020 //When set to 1, mutes ih_cec_stat0[5]
#define IH_MUTE_CEC_STAT0_WAKEUP_MASK  0x00000040 //When set to 1, mutes ih_cec_stat0[6]

//Video Packetizer Interrupt Mute Control Register
#define IH_MUTE_VP_STAT0  0x0000061C
#define IH_MUTE_VP_STAT0_FIFOEMPTYBYP_MASK  0x00000001 //When set to 1, mutes ih_vp_stat0[0]
#define IH_MUTE_VP_STAT0_FIFOFULLBYP_MASK  0x00000002 //When set to 1, mutes ih_vp_stat0[1]
#define IH_MUTE_VP_STAT0_FIFOEMPTYREMAP_MASK  0x00000004 //When set to 1, mutes ih_vp_stat0[2]
#define IH_MUTE_VP_STAT0_FIFOFULLREMAP_MASK  0x00000008 //When set to 1, mutes ih_vp_stat0[3]
#define IH_MUTE_VP_STAT0_FIFOEMPTYPP_MASK  0x00000010 //When set to 1, mutes ih_vp_stat0[4]
#define IH_MUTE_VP_STAT0_FIFOFULLPP_MASK  0x00000020 //When set to 1, mutes ih_vp_stat0[5]
#define IH_MUTE_VP_STAT0_FIFOEMPTYREPET_MASK  0x00000040 //When set to 1, mutes ih_vp_stat0[6]
#define IH_MUTE_VP_STAT0_FIFOFULLREPET_MASK  0x00000080 //When set to 1, mutes ih_vp_stat0[7]

//PHY GEN2 I2C Master Interrupt Mute Control Register
#define IH_MUTE_I2CMPHY_STAT0  0x00000620
#define IH_MUTE_I2CMPHY_STAT0_I2CMPHYERROR_MASK  0x00000001 //When set to 1, mutes ih_i2cmphy_stat0[0]
#define IH_MUTE_I2CMPHY_STAT0_I2CMPHYDONE_MASK  0x00000002 //When set to 1, mutes ih_i2cmphy_stat0[1]

//AHB Audio DMA Interrupt Mute Control Register
#define IH_MUTE_AHBDMAAUD_STAT0  0x00000624
#define IH_MUTE_AHBDMAAUD_STAT0_INTBUFFEMPTY_MASK  0x00000001 //When set to 1, mutes ih_ahbdmaaud_stat0[0]
#define IH_MUTE_AHBDMAAUD_STAT0_INTBUFFULL_MASK  0x00000002 //en set to 1, mutes ih_ahbdmaaud_stat0[1]
#define IH_MUTE_AHBDMAAUD_STAT0_INTDONE_MASK  0x00000004 //When set to 1, mutes ih_ahbdmaaud_stat0[2]
#define IH_MUTE_AHBDMAAUD_STAT0_INTINTERTRYSPLIT_MASK  0x00000008 //When set to 1, mutes ih_ahbdmaaud_stat0[3]
#define IH_MUTE_AHBDMAAUD_STAT0_INTLOSTOWNERSHIP_MASK  0x00000010 //When set to 1, mutes ih_ahbdmaaud_stat0[4]
#define IH_MUTE_AHBDMAAUD_STAT0_INTERROR_MASK  0x00000020 //When set to 1, mutes ih_ahbdmaaud_stat0[5]
#define IH_MUTE_AHBDMAAUD_STAT0_INTBUFFOVERRUN_MASK  0x00000040 //When set to 1, mutes ih_ahbdmaaud_stat0[6]

//Global Interrupt Mute Control Register
#define IH_MUTE  0x000007FC
#define IH_MUTE_MUTE_ALL_INTERRUPT_MASK  0x00000001 //When set to 1, mutes the main interrupt line (where all interrupts are ORed)
#define IH_MUTE_MUTE_WAKEUP_INTERRUPT_MASK  0x00000002 //When set to 1, mutes the main interrupt output port



//Video Input Mapping and Internal Data Enable Configuration Register
#define TX_INVID0  0x00000800
#define TX_INVID0_VIDEO_MAPPING_MASK  0x0000001F //Video Input mapping (color space/color depth): 0x01: RGB 4:4:4/8 bits 0x03: RGB 4:4:4/10 bits 0x05: RGB 4:4:4/12 bits 0x07: RGB 4:4:4/16 bits 0x09: YCbCr 4:4:4 or 4:2:0/8 bits 0x0B: YCbCr 4:4:4 or 4:2:0/10 bits 0x0D: YCbCr 4:4:4 or 4:2:0/12 bits 0x0F: YCbCr 4:4:4 or 4:2:0/16 bits 0x16: YCbCr 4:2:2/8 bits 0x14: YCbCr 4:2:2/10 bits 0x12: YCbCr 4:2:2/12 bits 0x17: YCbCr 4:4:4 (IPI)/8 bits 0x18: YCbCr 4:4:4 (IPI)/10 bits 0x19: YCbCr 4:4:4 (IPI)/12 bits 0x1A: YCbCr 4:4:4 (IPI)/16 bits 0x1B: YCbCr 4:2:2 (IPI)/12 bits 0x1C: YCbCr 4:2:0 (IPI)/8 bits 0x1D: YCbCr 4:2:0 (IPI)/10 bits 0x1E: YCbCr 4:2:0 (IPI)/12 bits 0x1F: YCbCr 4:2:0 (IPI)/16 bits
#define TX_INVID0_INTERNAL_DE_GENERATOR_MASK  0x00000080 //Internal data enable (DE) generator enable

//Video Input Stuffing Enable Register
#define TX_INSTUFFING  0x00000804
#define TX_INSTUFFING_GYDATA_STUFFING_MASK  0x00000001 //- 0b: When the dataen signal is low, the value in the gydata[15:0] output is the one sampled from the corresponding input data
#define TX_INSTUFFING_RCRDATA_STUFFING_MASK  0x00000002 //- 0b: When the dataen signal is low, the value in the rcrdata[15:0] output is the one sampled from the corresponding input data
#define TX_INSTUFFING_BCBDATA_STUFFING_MASK  0x00000004 //- 0b: When the dataen signal is low, the value in the bcbdata[15:0] output is the one sampled from the corresponding input data

//Video Input gy Data Channel Stuffing Register 0
#define TX_GYDATA0  0x00000808
#define TX_GYDATA0_GYDATA_MASK  0x000000FF //This register defines the value of gydata[7:0] when TX_INSTUFFING[0] (gydata_stuffing) is set to 1b

//Video Input gy Data Channel Stuffing Register 1
#define TX_GYDATA1  0x0000080C
#define TX_GYDATA1_GYDATA_MASK  0x000000FF //This register defines the value of gydata[15:8] when TX_INSTUFFING[0] (gydata_stuffing) is set to 1b

//Video Input rcr Data Channel Stuffing Register 0
#define TX_RCRDATA0  0x00000810
#define TX_RCRDATA0_RCRDATA_MASK  0x000000FF //This register defines the value of rcrydata[7:0] when TX_INSTUFFING[1] (rcrdata_stuffing) is set to 1b

//Video Input rcr Data Channel Stuffing Register 1
#define TX_RCRDATA1  0x00000814
#define TX_RCRDATA1_RCRDATA_MASK  0x000000FF //This register defines the value of rcrydata[15:8] when TX_INSTUFFING[1] (rcrdata_stuffing) is set to 1b

//Video Input bcb Data Channel Stuffing Register 0
#define TX_BCBDATA0  0x00000818
#define TX_BCBDATA0_BCBDATA_MASK  0x000000FF //This register defines the value of bcbdata[7:0] when TX_INSTUFFING[2] (bcbdata_stuffing) is set to 1b

//Video Input bcb Data Channel Stuffing Register 1
#define TX_BCBDATA1  0x0000081C
#define TX_BCBDATA1_BCBDATA_MASK  0x000000FF //This register defines the value of bcbdata[15:8] when TX_INSTUFFING[2] (bcbdata_stuffing) is set to 1b



//Video Packetizer Packing Phase Status Register
#define VP_STATUS  0x00002000
#define VP_STATUS_PACKING_PHASE_MASK  0x0000000F //Read only register that holds the "packing phase" output of the Video Packetizer block

//Video Packetizer Pixel Repetition and Color Depth Register
#define VP_PR_CD  0x00002004
#define VP_PR_CD_DESIRED_PR_FACTOR_MASK  0x0000000F //Desired pixel repetition factor configuration
#define VP_PR_CD_COLOR_DEPTH_MASK  0x000000F0 //The Color depth configuration is described as the following, with the action stated corresponding to color_depth[3:0]: - 0000b: 24 bits per pixel video (8 bits per component)

//Video Packetizer Stuffing and Default Packing Phase Register
#define VP_STUFF  0x00002008
#define VP_STUFF_PR_STUFFING_MASK  0x00000001 //Pixel repeater stuffing control
#define VP_STUFF_PP_STUFFING_MASK  0x00000002 //Pixel packing stuffing control
#define VP_STUFF_YCC422_STUFFING_MASK  0x00000004 //YCC 422 remap stuffing control
#define VP_STUFF_ICX_GOTO_P0_ST_MASK  0x00000008 //Reserved
#define VP_STUFF_IFIX_PP_TO_LAST_MASK  0x00000010 //Reserved
#define VP_STUFF_IDEFAULT_PHASE_MASK  0x00000020 //Controls the default phase packing machine used according to HDMI 1

//Video Packetizer YCC422 Remapping Register
#define VP_REMAP  0x0000200C
#define VP_REMAP_YCC422_SIZE_MASK  0x00000003 //YCC 422 remap input video size ycc422_size[1:0] 00b: YCC 422 16-bit input video (8 bits per component) 01b: YCC 422 20-bit input video (10 bits per component) 10b: YCC 422 24-bit input video (12 bits per component) 11b: Reserved

//Video Packetizer Output, Bypass and Enable Configuration Register
#define VP_CONF  0x00002010
#define VP_CONF_OUTPUT_SELECTOR_MASK  0x00000003 //Video Packetizer output selection output_selector[1:0] 00b: Data from pixel packing block 01b: Data from YCC 422 remap block 10b: Data from 8-bit bypass block 11b: Data from 8-bit bypass block
#define VP_CONF_BYPASS_SELECT_MASK  0x00000004 //bypass_select 0b: Data from pixel repeater block 1b: Data from input of Video Packetizer block
#define VP_CONF_YCC422_EN_MASK  0x00000008 //YCC 422 select enable
#define VP_CONF_PR_EN_MASK  0x00000010 //Pixel repeater enable
#define VP_CONF_PP_EN_MASK  0x00000020 //Pixel packing enable
#define VP_CONF_BYPASS_EN_MASK  0x00000040 //Bypass enable

//Video Packetizer Interrupt Mask Register
#define VP_MASK  0x0000201C
#define VP_MASK_OINTEMPTYBYP_MASK  0x00000001 //Mask bit for Video Packetizer 8-bit bypass FIFO empty
#define VP_MASK_OINTFULLBYP_MASK  0x00000002 //Mask bit for Video Packetizer 8-bit bypass FIFO full
#define VP_MASK_OINTEMPTYREMAP_MASK  0x00000004 //Mask bit for Video Packetizer pixel YCC 422 re-mapper FIFO empty
#define VP_MASK_OINTFULLREMAP_MASK  0x00000008 //Mask bit for Video Packetizer pixel YCC 422 re-mapper FIFO full
#define VP_MASK_OINTEMPTYPP_MASK  0x00000010 //Mask bit for Video Packetizer pixel packing FIFO empty
#define VP_MASK_OINTFULLPP_MASK  0x00000020 //Mask bit for Video Packetizer pixel packing FIFO full
#define VP_MASK_OINTEMPTYREPET_MASK  0x00000040 //Mask bit for Video Packetizer pixel repeater FIFO empty
#define VP_MASK_OINTFULLREPET_MASK  0x00000080 //Mask bit for Video Packetizer pixel repeater FIFO full

//Frame Composer Input Video Configuration and HDCP Keepout Register
#define FC_INVIDCONF  0x00004000
#define FC_INVIDCONF_IN_I_P_MASK  0x00000001 //Input video mode: 1b: Interlaced 0b: Progressive
#define FC_INVIDCONF_R_V_BLANK_IN_OSC_MASK  0x00000002 //Used for CEA861-D modes with fractional Vblank (for example, modes 5, 6, 7, 10, 11, 20, 21, and 22)
#define FC_INVIDCONF_DVI_MODEZ_MASK  0x00000008 //Active low 0b: DVI mode selected 1b: HDMI mode selected
#define FC_INVIDCONF_DE_IN_POLARITY_MASK  0x00000010 //Data enable input polarity 1b: Active high 0b: Active low
#define FC_INVIDCONF_HSYNC_IN_POLARITY_MASK  0x00000020 //Hsync input polarity 1b: Active high 0b: Active low
#define FC_INVIDCONF_VSYNC_IN_POLARITY_MASK  0x00000040 //Vsync input polarity 1b: Active high 0b: Active low
#define FC_INVIDCONF_HDCP_KEEPOUT_MASK  0x00000080 //Start/stop HDCP keepout window generation 1b: Active

//Frame Composer Input Video HActive Pixels Register 0
#define FC_INHACTIV0  0x00004004
#define FC_INHACTIV0_H_IN_ACTIV_MASK  0x000000FF //Input video Horizontal active pixel region width

//Frame Composer Input Video HActive Pixels Register 1
#define FC_INHACTIV1  0x00004008
#define FC_INHACTIV1_H_IN_ACTIV_MASK  0x0000000F //Input video Horizontal active pixel region width
#define FC_INHACTIV1_H_IN_ACTIV_12_MASK  0x00000010 //Input video Horizontal active pixel region width (0
#define FC_INHACTIV1_H_IN_ACTIV_13_MASK  0x00000020 //Input video Horizontal active pixel region width (0

//Frame Composer Input Video HBlank Pixels Register 0
#define FC_INHBLANK0  0x0000400C
#define FC_INHBLANK0_H_IN_BLANK_MASK  0x000000FF //Input video Horizontal blanking pixel region width

//Frame Composer Input Video HBlank Pixels Register 1
#define FC_INHBLANK1  0x00004010
#define FC_INHBLANK1_H_IN_BLANK_MASK  0x00000003 //Input video Horizontal blanking pixel region width this bit field holds bits 9:8 of number of Horizontal blanking pixels
#define FC_INHBLANK1_H_IN_BLANK_12_MASK  0x0000001C //Input video Horizontal blanking pixel region width If configuration parameter DWC_HDMI_TX_14 = True (1), this bit field holds bit 12:10 of number of horizontal blanking pixels

//Frame Composer Input Video VActive Pixels Register 0
#define FC_INVACTIV0  0x00004014
#define FC_INVACTIV0_V_IN_ACTIV_MASK  0x000000FF //Input video Vertical active pixel region width

//Frame Composer Input Video VActive Pixels Register 1
#define FC_INVACTIV1  0x00004018
#define FC_INVACTIV1_V_IN_ACTIV_MASK  0x00000007 //Input video Vertical active pixel region width
#define FC_INVACTIV1_V_IN_ACTIV_12_11_MASK  0x00000018 //Input video Vertical active pixel region width

//Frame Composer Input Video VBlank Pixels Register
#define FC_INVBLANK  0x0000401C
#define FC_INVBLANK_V_IN_BLANK_MASK  0x000000FF //Input video Vertical blanking pixel region width

//Frame Composer Input Video HSync Front Porch Register 0
#define FC_HSYNCINDELAY0  0x00004020
#define FC_HSYNCINDELAY0_H_IN_DELAY_MASK  0x000000FF //Input video Hsync active edge delay

//Frame Composer Input Video HSync Front Porch Register 1
#define FC_HSYNCINDELAY1  0x00004024
#define FC_HSYNCINDELAY1_H_IN_DELAY_MASK  0x00000007 //Input video Horizontal active edge delay
#define FC_HSYNCINDELAY1_H_IN_DELAY_12_MASK  0x00000018 //Input video Horizontal active edge delay

//Frame Composer Input Video HSync Width Register 0
#define FC_HSYNCINWIDTH0  0x00004028
#define FC_HSYNCINWIDTH0_H_IN_WIDTH_MASK  0x000000FF //Input video Hsync active pulse width

//Frame Composer Input Video HSync Width Register 1
#define FC_HSYNCINWIDTH1  0x0000402C
#define FC_HSYNCINWIDTH1_H_IN_WIDTH_MASK  0x00000001 //Input video Hsync active pulse width
#define FC_HSYNCINWIDTH1_H_IN_WIDTH_9_MASK  0x00000002 //Input video Hsync active pulse width

//Frame Composer Input Video VSync Front Porch Register
#define FC_VSYNCINDELAY  0x00004030
#define FC_VSYNCINDELAY_V_IN_DELAY_MASK  0x000000FF //Input video Vsync active edge delay

//Frame Composer Input Video VSync Width Register
#define FC_VSYNCINWIDTH  0x00004034
#define FC_VSYNCINWIDTH_V_IN_WIDTH_MASK  0x0000003F //Input video Vsync active pulse width

//Frame Composer Input Video Refresh Rate Register 0
#define FC_INFREQ0  0x00004038
#define FC_INFREQ0_INFREQ_MASK  0x000000FF //Video refresh rate in Hz*1E3 format

//Frame Composer Input Video Refresh Rate Register 1
#define FC_INFREQ1  0x0000403C
#define FC_INFREQ1_INFREQ_MASK  0x000000FF //Video refresh rate in Hz*1E3 format

//Frame Composer Input Video Refresh Rate Register 2
#define FC_INFREQ2  0x00004040
#define FC_INFREQ2_INFREQ_MASK  0x0000000F //Video refresh rate in Hz*1E3 format

//Frame Composer Control Period Duration Register
#define FC_CTRLDUR  0x00004044
#define FC_CTRLDUR_CTRLPERIODDURATION_MASK  0x000000FF //Configuration of the control period minimum duration (minimum of 12 pixel clock cycles; refer to HDMI 1

//Frame Composer Extended Control Period Duration Register
#define FC_EXCTRLDUR  0x00004048
#define FC_EXCTRLDUR_EXCTRLPERIODDURATION_MASK  0x000000FF //Configuration of the extended control period minimum duration (minimum of 32 pixel clock cycles; refer to HDMI 1

//Frame Composer Extended Control Period Maximum Spacing Register
#define FC_EXCTRLSPAC  0x0000404C
#define FC_EXCTRLSPAC_EXCTRLPERIODSPACING_MASK  0x000000FF //Configuration of the maximum spacing between consecutive extended control periods (maximum of 50ms; refer to the applicable HDMI specification)

//Frame Composer Channel 0 Non-Preamble Data Register
#define FC_CH0PREAM  0x00004050
#define FC_CH0PREAM_CH0_PREAMBLE_FILTER_MASK  0x000000FF //When in control mode, configures 8 bits that fill the channel 0 data lines not used to transmit the preamble (for more clarification, refer to the HDMI 1

//Frame Composer Channel 1 Non-Preamble Data Register
#define FC_CH1PREAM  0x00004054
#define FC_CH1PREAM_CH1_PREAMBLE_FILTER_MASK  0x0000003F //When in control mode, configures 6 bits that fill the channel 1 data lines not used to transmit the preamble (for more clarification, refer to the HDMI 1

//Frame Composer Channel 2 Non-Preamble Data Register
#define FC_CH2PREAM  0x00004058
#define FC_CH2PREAM_CH2_PREAMBLE_FILTER_MASK  0x0000003F //When in control mode, configures 6 bits that fill the channel 2 data lines not used to transmit the preamble (for more clarification, refer to the HDMI 1

//Frame Composer AVI Packet Configuration Register 3
#define FC_AVICONF3  0x0000405C
#define FC_AVICONF3_CN_MASK  0x00000003 //IT content type according to CEA the specification
#define FC_AVICONF3_YQ_MASK  0x0000000C //YCC Quantization range according to the CEA specification

//Frame Composer GCP Packet Configuration Register
#define FC_GCP  0x00004060
#define FC_GCP_CLEAR_AVMUTE_MASK  0x00000001 //Value of "clear_avmute" in the GCP packet
#define FC_GCP_SET_AVMUTE_MASK  0x00000002 //Value of "set_avmute" in the GCP packet Once the AVmute is set, the frame composer schedules the GCP packet with AVmute set in the packet scheduler to be sent once (may only be transmitted between the active edge of VSYNC and 384 pixels following this edge)
#define FC_GCP_DEFAULT_PHASE_MASK  0x00000004 //Value of "default_phase" in the GCP packet

//Frame Composer AVI Packet Configuration Register 0
#define FC_AVICONF0  0x00004064
#define FC_AVICONF0_RGC_YCC_INDICATION_MASK  0x00000003 //Y1,Y0 RGB or YCC indicator
#define FC_AVICONF0_BAR_INFORMATION_MASK  0x0000000C //Bar information data valid
#define FC_AVICONF0_SCAN_INFORMATION_MASK  0x00000030 //Scan information
#define FC_AVICONF0_ACTIVE_FORMAT_PRESENT_MASK  0x00000040 //Active format present
#define FC_AVICONF0_RGC_YCC_INDICATION_2_MASK  0x00000080 //Y2, Bit 2 of rgc_ycc_indication

//Frame Composer AVI Packet Configuration Register 1
#define FC_AVICONF1  0x00004068
#define FC_AVICONF1_ACTIVE_ASPECT_RATIO_MASK  0x0000000F //Active aspect ratio
#define FC_AVICONF1_PICTURE_ASPECT_RATIO_MASK  0x00000030 //Picture aspect ratio
#define FC_AVICONF1_COLORIMETRY_MASK  0x000000C0 //Colorimetry

//Frame Composer AVI Packet Configuration Register 2
#define FC_AVICONF2  0x0000406C
#define FC_AVICONF2_NON_UNIFORM_PICTURE_SCALING_MASK  0x00000003 //Non-uniform picture scaling
#define FC_AVICONF2_QUANTIZATION_RANGE_MASK  0x0000000C //Quantization range
#define FC_AVICONF2_EXTENDED_COLORIMETRY_MASK  0x00000070 //Extended colorimetry
#define FC_AVICONF2_IT_CONTENT_MASK  0x00000080 //IT content

//Frame Composer AVI Packet VIC Register
#define FC_AVIVID  0x00004070
#define FC_AVIVID_FC_AVIVID_MASK  0x0000007F //Configures the AVI InfoFrame Video Identification code
#define FC_AVIVID_FC_AVIVID_7_MASK  0x00000080 //Bit 7 of fc_avivid register

#define FC_AVIETB0  0x4074
#define FC_AVIETB1  0x4078
#define FC_AVISBB0  0x407C
#define FC_AVISBB1  0x4080
#define FC_AVIELB0  0x4084
#define FC_AVIELB1  0x4088
#define FC_AVISRB0  0x408C
#define FC_AVISRB1  0x4090

//Frame Composer AUD Packet Configuration Register 0
#define FC_AUDICONF0  0x00004094
#define FC_AUDICONF0_CT_MASK  0x0000000F //Coding Type
#define FC_AUDICONF0_CC_MASK  0x00000070 //Channel count

//Frame Composer AUD Packet Configuration Register 1
#define FC_AUDICONF1  0x00004098
#define FC_AUDICONF1_SF_MASK  0x00000007 //Sampling frequency
#define FC_AUDICONF1_SS_MASK  0x00000030 //Sampling size

//Frame Composer AUD Packet Configuration Register 2
#define FC_AUDICONF2  0x0000409C
#define FC_AUDICONF2_CA_MASK  0x000000FF //Channel allocation

//Frame Composer AUD Packet Configuration Register 0
#define FC_AUDICONF3  0x000040A0
#define FC_AUDICONF3_LSV_MASK  0x0000000F //Level shift value (for down mixing)
#define FC_AUDICONF3_DM_INH_MASK  0x00000010 //Down mix enable
#define FC_AUDICONF3_LFEPBL_MASK  0x00000060 //LFE playback information LFEPBL1, LFEPBL0 LFE playback level as compared to the other channels

//Frame Composer VSI Packet Data IEEE Register 2
//#define FC_VSDIEEEID2  0x000040A4
#define FC_VSDIEEEID0  0x000040A4
#define FC_VSDIEEEID0_IEEE_MASK  0x000000FF //This register configures the Vendor Specific InfoFrame IEEE registration identifier

//Frame Composer VSI Packet Data Size Register
#define FC_VSDSIZE  0x000040A8
#define FC_VSDSIZE_VSDSIZE_MASK  0x0000001F //Packet size as described in the HDMI Vendor Specific InfoFrame (from the HDMI specification)

//ADD REG******************************************
#define FC_INVBLANK1  0x000040B8

#define FC_VSYNCINDELAY1  0x000040BC
/*************************************************/

//Frame Composer VSI Packet Data IEEE Register 1
#define FC_VSDIEEEID1  0x000040C0
#define FC_VSDIEEEID1_IEEE_MASK  0x000000FF //This register configures the Vendor Specific InfoFrame IEEE registration identifier

//Frame Composer VSI Packet Data IEEE Register 0
//#define FC_VSDIEEEID0  0x000040C4
#define FC_VSDIEEEID2  0x000040C4
#define FC_VSDIEEEID2_IEEE_MASK  0x000000FF //This register configures the Vendor Specific InfoFrame IEEE registration identifier

#define  FC_VSDPAYLOAD0  0x40C8

//Source Product Descriptor
#define FC_SPDVENDORNAME0 0x4128
#define  FC_SPDPRODUCTNAME0 0x4148

//Frame Composer SPD Packet Data Source Product Descriptor Register
#define FC_SPDDEVICEINF  0x00004188
#define FC_SPDDEVICEINF_FC_SPDDEVICEINF_MASK  0x000000FF //Frame Composer SPD Packet Data Source Product Descriptor Register

//Frame Composer Audio Sample Flat and Layout Configuration Register
#define FC_AUDSCONF  0x0000418C
#define FC_AUDSCONF_AUD_PACKET_LAYOUT_MASK  0x00000001 //Set the audio packet layout to be sent in the packet: 1b: layout 1 0b: layout 0 If DWC_HDMI_TX_20 is defined and register field fc_multistream_ctrl
#define FC_AUDSCONF_AUD_PACKET_SAMPFLT_MASK  0x000000F0 //Set the audio packet sample flat value to be sent on the packet

//Frame Composer Audio Sample Flat and Layout Configuration Register
#define FC_AUDSSTAT  0x00004190
#define FC_AUDSSTAT_PACKET_SAMPPRS_MASK  0x0000000F //Shows the data sample present indication of the last Audio sample packet sent by the HDMI TX Controller

//Frame Composer Audio Sample Validity Flag Register
#define FC_AUDSV  0x00004194
#define FC_AUDSV_V0L_MASK  0x00000001 //Set validity bit "V" for Channel 0, Left
#define FC_AUDSV_V1L_MASK  0x00000002 //Set validity bit "V" for Channel 1, Left
#define FC_AUDSV_V2L_MASK  0x00000004 //Set validity bit "V" for Channel 2, Left
#define FC_AUDSV_V3L_MASK  0x00000008 //Set validity bit "V" for Channel 3, Left
#define FC_AUDSV_V0R_MASK  0x00000010 //Set validity bit "V" for Channel 0, Right
#define FC_AUDSV_V1R_MASK  0x00000020 //Set validity bit "V" for Channel 1, Right
#define FC_AUDSV_V2R_MASK  0x00000040 //Set validity bit "V" for Channel 2, Right
#define FC_AUDSV_V3R_MASK  0x00000080 //Set validity bit "V" for Channel 3, Right

//Frame Composer Audio Sample User Flag Register
#define FC_AUDSU  0x00004198
#define FC_AUDSU_U0L_MASK  0x00000001 //Set user bit "U" for Channel 0, Left
#define FC_AUDSU_U1L_MASK  0x00000002 //Set user bit "U" for Channel 1, Left
#define FC_AUDSU_U2L_MASK  0x00000004 //Set user bit "U" for Channel 2, Left
#define FC_AUDSU_U3L_MASK  0x00000008 //Set user bit "U" for Channel 3, Left
#define FC_AUDSU_U0R_MASK  0x00000010 //Set user bit "U" for Channel 0, Right
#define FC_AUDSU_U1R_MASK  0x00000020 //Set user bit "U" for Channel 1, Right
#define FC_AUDSU_U2R_MASK  0x00000040 //Set user bit "U" for Channel 2, Right
#define FC_AUDSU_U3R_MASK  0x00000080 //Set user bit "U" for Channel 3, Right

//Frame Composer Audio Sample Channel Status Configuration Register 0
#define FC_AUDSCHNL0  0x0000419C
#define FC_AUDSCHNL0_OIEC_COPYRIGHT_MASK  0x00000001 //IEC Copyright indication
#define FC_AUDSCHNL0_OIEC_CGMSA_MASK  0x00000030 //CGMS-A

//Frame Composer Audio Sample Channel Status Configuration Register 1
#define FC_AUDSCHNL1  0x000041A0
#define FC_AUDSCHNL1_OIEC_CATEGORYCODE_MASK  0x000000FF //Category code

//Frame Composer Audio Sample Channel Status Configuration Register 2
#define FC_AUDSCHNL2  0x000041A4
#define FC_AUDSCHNL2_OIEC_SOURCENUMBER_MASK  0x0000000F //Source number
#define FC_AUDSCHNL2_OIEC_PCMAUDIOMODE_MASK  0x00000070 //PCM audio mode

//Frame Composer Audio Sample Channel Status Configuration Register 3
#define FC_AUDSCHNL3  0x000041A8
#define FC_AUDSCHNL3_OIEC_CHANNELNUMCR0_MASK  0x0000000F //Channel number for first right sample
#define FC_AUDSCHNL3_OIEC_CHANNELNUMCR1_MASK  0x000000F0 //Channel number for second right sample

//Frame Composer Audio Sample Channel Status Configuration Register 4
#define FC_AUDSCHNL4  0x000041AC
#define FC_AUDSCHNL4_OIEC_CHANNELNUMCR2_MASK  0x0000000F //Channel number for third right sample
#define FC_AUDSCHNL4_OIEC_CHANNELNUMCR3_MASK  0x000000F0 //Channel number for fourth right sample

//Frame Composer Audio Sample Channel Status Configuration Register 5
#define FC_AUDSCHNL5  0x000041B0
#define FC_AUDSCHNL5_OIEC_CHANNELNUMCL0_MASK  0x0000000F //Channel number for first left sample
#define FC_AUDSCHNL5_OIEC_CHANNELNUMCL1_MASK  0x000000F0 //Channel number for second left sample

//Frame Composer Audio Sample Channel Status Configuration Register 6
#define FC_AUDSCHNL6  0x000041B4
#define FC_AUDSCHNL6_OIEC_CHANNELNUMCL2_MASK  0x0000000F //Channel number for third left sample
#define FC_AUDSCHNL6_OIEC_CHANNELNUMCL3_MASK  0x000000F0 //Channel number for fourth left sample

//Frame Composer Audio Sample Channel Status Configuration Register 7
#define FC_AUDSCHNL7  0x000041B8
#define FC_AUDSCHNL7_OIEC_SAMPFREQ_MASK  0x0000000F //Sampling frequency
#define FC_AUDSCHNL7_OIEC_CLKACCURACY_MASK  0x00000030 //Clock accuracy
#define FC_AUDSCHNL7_OIEC_SAMPFREQ_EXT_MASK  0x000000C0 //Sampling frequency (channel status bits 31 and 30)

//Frame Composer Audio Sample Channel Status Configuration Register 8
#define FC_AUDSCHNL8  0x000041BC
#define FC_AUDSCHNL8_OIEC_WORDLENGTH_MASK  0x0000000F //Word length configuration
#define FC_AUDSCHNL8_OIEC_ORIGSAMPFREQ_MASK  0x000000F0 //Original sampling frequency

//Frame Composer Number of High Priority Packets Attended Configuration Register
#define FC_CTRLQHIGH  0x000041CC
#define FC_CTRLQHIGH_ONHIGHATTENDED_MASK  0x0000001F //Configures the number of high priority packets or audio sample packets consecutively attended before checking low priority queue status

//Frame Composer Number of Low Priority Packets Attended Configuration Register
#define FC_CTRLQLOW  0x000041D0
#define FC_CTRLQLOW_ONLOWATTENDED_MASK  0x0000001F //Configures the number of low priority packets or null packets consecutively attended before checking high priority queue status or audio samples availability

//Frame Composer ACP Packet Type Configuration Register 0
#define FC_ACP0  0x000041D4
#define FC_ACP0_ACPTYPE_MASK  0x000000FF //Configures the ACP packet type

//Frame Composer ACP Packet Body Configuration Register 16
#define FC_ACP16  0x00004208
#define FC_ACP16_FC_ACP16_MASK  0x000000FF //Frame Composer ACP Packet Body Configuration Register 16

//Frame Composer ACP Packet Body Configuration Register 15
#define FC_ACP15  0x0000420C
#define FC_ACP15_FC_ACP15_MASK  0x000000FF //Frame Composer ACP Packet Body Configuration Register 15

//Frame Composer ACP Packet Body Configuration Register 14
#define FC_ACP14  0x00004210
#define FC_ACP14_FC_ACP14_MASK  0x000000FF //Frame Composer ACP Packet Body Configuration Register 14

//Frame Composer ACP Packet Body Configuration Register 13
#define FC_ACP13  0x00004214
#define FC_ACP13_FC_ACP13_MASK  0x000000FF //Frame Composer ACP Packet Body Configuration Register 13

//Frame Composer ACP Packet Body Configuration Register 12
#define FC_ACP12  0x00004218
#define FC_ACP12_FC_ACP12_MASK  0x000000FF //Frame Composer ACP Packet Body Configuration Register 12

//Frame Composer ACP Packet Body Configuration Register 11
#define FC_ACP11  0x0000421C
#define FC_ACP11_FC_ACP11_MASK  0x000000FF //Frame Composer ACP Packet Body Configuration Register 11

//Frame Composer ACP Packet Body Configuration Register 10
#define FC_ACP10  0x00004220
#define FC_ACP10_FC_ACP10_MASK  0x000000FF //Frame Composer ACP Packet Body Configuration Register 10

//Frame Composer ACP Packet Body Configuration Register 9
#define FC_ACP9  0x00004224
#define FC_ACP9_FC_ACP9_MASK  0x000000FF //Frame Composer ACP Packet Body Configuration Register 9

//Frame Composer ACP Packet Body Configuration Register 8
#define FC_ACP8  0x00004228
#define FC_ACP8_FC_ACP8_MASK  0x000000FF //Frame Composer ACP Packet Body Configuration Register 8

//Frame Composer ACP Packet Body Configuration Register 7
#define FC_ACP7  0x0000422C
#define FC_ACP7_FC_ACP7_MASK  0x000000FF //Frame Composer ACP Packet Body Configuration Register 7

//Frame Composer ACP Packet Body Configuration Register 6
#define FC_ACP6  0x00004230
#define FC_ACP6_FC_ACP6_MASK  0x000000FF //Frame Composer ACP Packet Body Configuration Register 6

//Frame Composer ACP Packet Body Configuration Register 5
#define FC_ACP5  0x00004234
#define FC_ACP5_FC_ACP5_MASK  0x000000FF //Frame Composer ACP Packet Body Configuration Register 5

//Frame Composer ACP Packet Body Configuration Register 4
#define FC_ACP4  0x00004238
#define FC_ACP4_FC_ACP4_MASK  0x000000FF //Frame Composer ACP Packet Body Configuration Register 4

//Frame Composer ACP Packet Body Configuration Register 3
#define FC_ACP3  0x0000423C
#define FC_ACP3_FC_ACP3_MASK  0x000000FF //Frame Composer ACP Packet Body Configuration Register 3

//Frame Composer ACP Packet Body Configuration Register 2
#define FC_ACP2  0x00004240
#define FC_ACP2_FC_ACP2_MASK  0x000000FF //Frame Composer ACP Packet Body Configuration Register 2

//Frame Composer ACP Packet Body Configuration Register 1
#define FC_ACP1  0x00004244
#define FC_ACP1_FC_ACP1_MASK  0x000000FF //Frame Composer ACP Packet Body Configuration Register 1

//Frame Composer ISRC1 Packet Status, Valid, and Continue Configuration Register
#define FC_ISCR1_0  0x00004248
#define FC_ISCR1_0_ISRC_CONT_MASK  0x00000001 //ISRC1 Indication of packet continuation (ISRC2 will be transmitted)
#define FC_ISCR1_0_ISRC_VALID_MASK  0x00000002 //ISRC1 Valid control signal
#define FC_ISCR1_0_ISRC_STATUS_MASK  0x0000001C //ISRC1 Status signal

//Frame Composer ISRC1 Packet Body Register 16
#define FC_ISCR1_16  0x0000424C
#define FC_ISCR1_16_FC_ISCR1_16_MASK  0x000000FF //Frame Composer ISRC1 Packet Body Register 16; configures ISRC1 packet body of the ISRC1 packet

//Frame Composer ISRC1 Packet Body Register 15
#define FC_ISCR1_15  0x00004250
#define FC_ISCR1_15_FC_ISCR1_15_MASK  0x000000FF //Frame Composer ISRC1 Packet Body Register 15

//Frame Composer ISRC1 Packet Body Register 14
#define FC_ISCR1_14  0x00004254
#define FC_ISCR1_14_FC_ISCR1_14_MASK  0x000000FF //Frame Composer ISRC1 Packet Body Register 14

//Frame Composer ISRC1 Packet Body Register 13
#define FC_ISCR1_13  0x00004258
#define FC_ISCR1_13_FC_ISCR1_13_MASK  0x000000FF //Frame Composer ISRC1 Packet Body Register 13

//Frame Composer ISRC1 Packet Body Register 12
#define FC_ISCR1_12  0x0000425C
#define FC_ISCR1_12_FC_ISCR1_12_MASK  0x000000FF //Frame Composer ISRC1 Packet Body Register 12

//Frame Composer ISRC1 Packet Body Register 11
#define FC_ISCR1_11  0x00004260
#define FC_ISCR1_11_FC_ISCR1_11_MASK  0x000000FF //Frame Composer ISRC1 Packet Body Register 11

//Frame Composer ISRC1 Packet Body Register 10
#define FC_ISCR1_10  0x00004264
#define FC_ISCR1_10_FC_ISCR1_10_MASK  0x000000FF //Frame Composer ISRC1 Packet Body Register 10

//Frame Composer ISRC1 Packet Body Register 9
#define FC_ISCR1_9  0x00004268
#define FC_ISCR1_9_FC_ISCR1_9_MASK  0x000000FF //Frame Composer ISRC1 Packet Body Register 9

//Frame Composer ISRC1 Packet Body Register 8
#define FC_ISCR1_8  0x0000426C
#define FC_ISCR1_8_FC_ISCR1_8_MASK  0x000000FF //Frame Composer ISRC1 Packet Body Register 8

//Frame Composer ISRC1 Packet Body Register 7
#define FC_ISCR1_7  0x00004270
#define FC_ISCR1_7_FC_ISCR1_7_MASK  0x000000FF //Frame Composer ISRC1 Packet Body Register 7

//Frame Composer ISRC1 Packet Body Register 6
#define FC_ISCR1_6  0x00004274
#define FC_ISCR1_6_FC_ISCR1_6_MASK  0x000000FF //Frame Composer ISRC1 Packet Body Register 6

//Frame Composer ISRC1 Packet Body Register 5
#define FC_ISCR1_5  0x00004278
#define FC_ISCR1_5_FC_ISCR1_5_MASK  0x000000FF //Frame Composer ISRC1 Packet Body Register 5

//Frame Composer ISRC1 Packet Body Register 4
#define FC_ISCR1_4  0x0000427C
#define FC_ISCR1_4_FC_ISCR1_4_MASK  0x000000FF //Frame Composer ISRC1 Packet Body Register 4

//Frame Composer ISRC1 Packet Body Register 3
#define FC_ISCR1_3  0x00004280
#define FC_ISCR1_3_FC_ISCR1_3_MASK  0x000000FF //Frame Composer ISRC1 Packet Body Register 3

//Frame Composer ISRC1 Packet Body Register 2
#define FC_ISCR1_2  0x00004284
#define FC_ISCR1_2_FC_ISCR1_2_MASK  0x000000FF //Frame Composer ISRC1 Packet Body Register 2

//Frame Composer ISRC1 Packet Body Register 1
#define FC_ISCR1_1  0x00004288
#define FC_ISCR1_1_FC_ISCR1_1_MASK  0x000000FF //Frame Composer ISRC1 Packet Body Register 1

//Frame Composer ISRC2 Packet Body Register 15
#define FC_ISCR2_15  0x0000428C
#define FC_ISCR2_15_FC_ISCR2_15_MASK  0x000000FF //Frame Composer ISRC2 Packet Body Register 15; configures the ISRC2 packet body of the ISRC2 packet

//Frame Composer ISRC2 Packet Body Register 14
#define FC_ISCR2_14  0x00004290
#define FC_ISCR2_14_FC_ISCR2_14_MASK  0x000000FF //Frame Composer ISRC2 Packet Body Register 14

//Frame Composer ISRC2 Packet Body Register 13
#define FC_ISCR2_13  0x00004294
#define FC_ISCR2_13_FC_ISCR2_13_MASK  0x000000FF //Frame Composer ISRC2 Packet Body Register 13

//Frame Composer ISRC2 Packet Body Register 12
#define FC_ISCR2_12  0x00004298
#define FC_ISCR2_12_FC_ISCR2_12_MASK  0x000000FF //Frame Composer ISRC2 Packet Body Register 12

//Frame Composer ISRC2 Packet Body Register 11
#define FC_ISCR2_11  0x0000429C
#define FC_ISCR2_11_FC_ISCR2_11_MASK  0x000000FF //Frame Composer ISRC2 Packet Body Register 11

//Frame Composer ISRC2 Packet Body Register 10
#define FC_ISCR2_10  0x000042A0
#define FC_ISCR2_10_FC_ISCR2_10_MASK  0x000000FF //Frame Composer ISRC2 Packet Body Register 10

//Frame Composer ISRC2 Packet Body Register 9
#define FC_ISCR2_9  0x000042A4
#define FC_ISCR2_9_FC_ISCR2_9_MASK  0x000000FF //Frame Composer ISRC2 Packet Body Register 9

//Frame Composer ISRC2 Packet Body Register 8
#define FC_ISCR2_8  0x000042A8
#define FC_ISCR2_8_FC_ISCR2_8_MASK  0x000000FF //Frame Composer ISRC2 Packet Body Register 8

//Frame Composer ISRC2 Packet Body Register 7
#define FC_ISCR2_7  0x000042AC
#define FC_ISCR2_7_FC_ISCR2_7_MASK  0x000000FF //Frame Composer ISRC2 Packet Body Register 7

//Frame Composer ISRC2 Packet Body Register 6
#define FC_ISCR2_6  0x000042B0
#define FC_ISCR2_6_FC_ISCR2_6_MASK  0x000000FF //Frame Composer ISRC2 Packet Body Register 6

//Frame Composer ISRC2 Packet Body Register 5
#define FC_ISCR2_5  0x000042B4
#define FC_ISCR2_5_FC_ISCR2_5_MASK  0x000000FF //Frame Composer ISRC2 Packet Body Register 5

//Frame Composer ISRC2 Packet Body Register 4
#define FC_ISCR2_4  0x000042B8
#define FC_ISCR2_4_FC_ISCR2_4_MASK  0x000000FF //Frame Composer ISRC2 Packet Body Register 4

//Frame Composer ISRC2 Packet Body Register 3
#define FC_ISCR2_3  0x000042BC
#define FC_ISCR2_3_FC_ISCR2_3_MASK  0x000000FF //Frame Composer ISRC2 Packet Body Register 3

//Frame Composer ISRC2 Packet Body Register 2
#define FC_ISCR2_2  0x000042C0
#define FC_ISCR2_2_FC_ISCR2_2_MASK  0x000000FF //Frame Composer ISRC2 Packet Body Register 2

//Frame Composer ISRC2 Packet Body Register 1
#define FC_ISCR2_1  0x000042C4
#define FC_ISCR2_1_FC_ISCR2_1_MASK  0x000000FF //Frame Composer ISRC2 Packet Body Register 1

//Frame Composer ISRC2 Packet Body Register 0
#define FC_ISCR2_0  0x000042C8
#define FC_ISCR2_0_FC_ISCR2_0_MASK  0x000000FF //Frame Composer ISRC2 Packet Body Register 0

//Frame Composer Data Island Auto Packet Scheduling Register 0 Configures the Frame Composer RDRB(1)/Manual(0) data island packet insertion for SPD, VSD, ISRC2, ISRC1 and ACP packets
#define FC_DATAUTO0  0x000042CC
#define FC_DATAUTO0_ACP_AUTO_MASK  0x00000001 //Enables ACP automatic packet scheduling
#define FC_DATAUTO0_ISCR1_AUTO_MASK  0x00000002 //Enables ISRC1 automatic packet scheduling
#define FC_DATAUTO0_ISCR2_AUTO_MASK  0x00000004 //Enables ISRC2 automatic packet scheduling
#define FC_DATAUTO0_VSD_AUTO_MASK  0x00000008 //Enables VSD automatic packet scheduling
#define FC_DATAUTO0_SPD_AUTO_MASK  0x00000010 //Enables SPD automatic packet scheduling

//Frame Composer Data Island Auto Packet Scheduling Register 1 Configures the Frame Composer (FC) RDRB frame interpolation for SPD, VSD, ISRC2, ISRC1 and ACP packet insertion on data island when FC is on RDRB mode for the listed packets
#define FC_DATAUTO1  0x000042D0
#define FC_DATAUTO1_AUTO_FRAME_INTERPOLATION_MASK  0x0000000F //Packet frame interpolation for automatic packet scheduling

//Frame Composer Data Island Auto packet scheduling Register 2 Configures the Frame Composer (FC) RDRB line interpolation and number of packets in frame for SPD, VSD, ISRC2, ISRC1 and ACP packet insertion on data island when FC is on RDRB mode for the listed packets
#define FC_DATAUTO2  0x000042D4
#define FC_DATAUTO2_AUTO_LINE_SPACING_MASK  0x0000000F //Packets line spacing, for automatic packet scheduling
#define FC_DATAUTO2_AUTO_FRAME_PACKETS_MASK  0x000000F0 //Packets per frame, for automatic packet scheduling

//Frame Composer Data Island Manual Packet Request Register Requests to the Frame Composer the data island packet insertion for NULL, SPD, VSD, ISRC2, ISRC1 and ACP packets when FC_DATAUTO0 bit is in manual mode for the packet requested
#define FC_DATMAN  0x000042D8
#define FC_DATMAN_ACP_TX_MASK  0x00000001 //ACP packet
#define FC_DATMAN_ISCR1_TX_MASK  0x00000002 //ISRC1 packet
#define FC_DATMAN_ISCR2_TX_MASK  0x00000004 //ISRC2 packet
#define FC_DATMAN_VSD_TX_MASK  0x00000008 //VSD packet
#define FC_DATMAN_SPD_TX_MASK  0x00000010 //SPD packet
#define FC_DATMAN_NULL_TX_MASK  0x00000020 //Null packet

//Frame Composer Data Island Auto Packet Scheduling Register 3 Configures the Frame Composer Automatic(1)/RDRB(0) data island packet insertion for AVI, GCP, AUDI and ACR packets
#define FC_DATAUTO3  0x000042DC
#define FC_DATAUTO3_ACR_AUTO_MASK  0x00000001 //Enables ACR packet insertion
#define FC_DATAUTO3_AUDI_AUTO_MASK  0x00000002 //Enables AUDI packet insertion
#define FC_DATAUTO3_GCP_AUTO_MASK  0x00000004 //Enables GCP packet insertion
#define FC_DATAUTO3_AVI_AUTO_MASK  0x00000008 //Enables AVI packet insertion
#define FC_DATAUTO3_AMP_AUTO_MASK  0x00000010 //Enables AMP packet insertion
#define FC_DATAUTO3_NVBI_AUTO_MASK  0x00000020 //Enables NTSC VBI packet insertion

//Frame Composer Round Robin ACR Packet Insertion Register 0 Configures the Frame Composer (FC) RDRB frame interpolation for ACR packet insertion on data island when FC is on RDRB mode for this packet
#define FC_RDRB0  0x000042E0
#define FC_RDRB0_ACRFRAMEINTERPOLATION_MASK  0x0000000F //ACR Frame interpolation

//Frame Composer Round Robin ACR Packet Insertion Register 1 Configures the Frame Composer (FC) RDRB line interpolation and number of packets in frame for the ACR packet insertion on data island when FC is on RDRB mode this packet
#define FC_RDRB1  0x000042E4
#define FC_RDRB1_ACRPACKETLINESPACING_MASK  0x0000000F //ACR packet line spacing
#define FC_RDRB1_ACRPACKETSINFRAME_MASK  0x000000F0 //ACR packets in frame

//Frame Composer Round Robin AUDI Packet Insertion Register 2 Configures the Frame Composer (FC) RDRB frame interpolation for AUDI packet insertion on data island when FC is on RDRB mode for this packet
#define FC_RDRB2  0x000042E8
#define FC_RDRB2_AUDIFRAMEINTERPOLATION_MASK  0x0000000F //Audio frame interpolation

//Frame Composer Round Robin AUDI Packet Insertion Register 3 Configures the Frame Composer (FC) RDRB line interpolation and number of packets in frame for the AUDI packet insertion on data island when FC is on RDRB mode this packet
#define FC_RDRB3  0x000042EC
#define FC_RDRB3_AUDIPACKETLINESPACING_MASK  0x0000000F //Audio packets line spacing
#define FC_RDRB3_AUDIPACKETSINFRAME_MASK  0x000000F0 //Audio packets per frame

//Frame Composer Round Robin GCP Packet Insertion Register 4 Configures the Frame Composer (FC) RDRB frame interpolation for GCP packet insertion on data island when FC is on RDRB mode for this packet
#define FC_RDRB4  0x000042F0
#define FC_RDRB4_GCPFRAMEINTERPOLATION_MASK  0x0000000F //Frames interpolated between GCP packets

//Frame Composer Round Robin GCP Packet Insertion Register 5 Configures the Frame Composer (FC) RDRB line interpolation and number of packets in frame for the GCP packet insertion on data island when FC is on RDRB mode this packet
#define FC_RDRB5  0x000042F4
#define FC_RDRB5_GCPPACKETLINESPACING_MASK  0x0000000F //GCP packets line spacing
#define FC_RDRB5_GCPPACKETSINFRAME_MASK  0x000000F0 //GCP packets per frame

//Frame Composer Round Robin AVI Packet Insertion Register 6 Configures the Frame Composer (FC) RDRB frame interpolation for AVI packet insertion on data island when FC is on RDRB mode for this packet
#define FC_RDRB6  0x000042F8
#define FC_RDRB6_AVIFRAMEINTERPOLATION_MASK  0x0000000F //Frames interpolated between AVI packets

//Frame Composer Round Robin AVI Packet Insertion Register 7 Configures the Frame Composer (FC) RDRB line interpolation and number of packets in frame for the AVI packet insertion on data island when FC is on RDRB mode this packet
#define FC_RDRB7  0x000042FC
#define FC_RDRB7_AVIPACKETLINESPACING_MASK  0x0000000F //AVI packets line spacing
#define FC_RDRB7_AVIPACKETSINFRAME_MASK  0x000000F0 //AVI packets per frame

//Frame Composer Round Robin AMP Packet Insertion Register 8
#define FC_RDRB8  0x00004300
#define FC_RDRB8_AMPFRAMEINTERPOLATION_MASK  0x0000000F //AMP frame interpolation

//Frame Composer Round Robin AMP Packet Insertion Register 9
#define FC_RDRB9  0x00004304
#define FC_RDRB9_AMPPACKETLINESPACING_MASK  0x0000000F //AMP packets line spacing
#define FC_RDRB9_AMPPACKETSINFRAME_MASK  0x000000F0 //AMP packets per frame

//Frame Composer Round Robin NTSC VBI Packet Insertion Register 10
#define FC_RDRB10  0x00004308
#define FC_RDRB10_NVBIFRAMEINTERPOLATION_MASK  0x0000000F //NTSC VBI frame interpolation

//Frame Composer Round Robin NTSC VBI Packet Insertion Register 11
#define FC_RDRB11  0x0000430C
#define FC_RDRB11_NVBIPACKETLINESPACING_MASK  0x0000000F //NTSC VBI packets line spacing
#define FC_RDRB11_NVBIPACKETSINFRAME_MASK  0x000000F0 //NTSC VBI packets per frame

/***********add*************/
#define FC_RDRB12  0x00004310
#define FC_RDRB13  0x00004314



//Frame Composer Packet Interrupt Mask Register 0
#define FC_MASK0  0x00004348
#define FC_MASK0_NULL_MASK  0x00000001 //Mask bit for FC_INT0
#define FC_MASK0_ACR_MASK  0x00000002 //Mask bit for FC_INT0
#define FC_MASK0_AUDS_MASK  0x00000004 //Mask bit for FC_INT0
#define FC_MASK0_NVBI_MASK  0x00000008 //Mask bit for FC_INT0
#define FC_MASK0_MAS_MASK  0x00000010 //Mask bit for FC_INT0
#define FC_MASK0_HBR_MASK  0x00000020 //Mask bit for FC_INT0
#define FC_MASK0_ACP_MASK  0x00000040 //Mask bit for FC_INT0
#define FC_MASK0_AUDI_MASK  0x00000080 //Mask bit for FC_INT0

//Frame Composer Packet Interrupt Mask Register 1
#define FC_MASK1  0x00004358
#define FC_MASK1_GCP_MASK  0x00000001 //Mask bit for FC_INT1
#define FC_MASK1_AVI_MASK  0x00000002 //Mask bit for FC_INT1
#define FC_MASK1_AMP_MASK  0x00000004 //Mask bit for FC_INT1
#define FC_MASK1_SPD_MASK  0x00000008 //Mask bit for FC_INT1
#define FC_MASK1_VSD_MASK  0x00000010 //Mask bit for FC_INT1
#define FC_MASK1_ISCR2_MASK  0x00000020 //Mask bit for FC_INT1
#define FC_MASK1_ISCR1_MASK  0x00000040 //Mask bit for FC_INT1
#define FC_MASK1_GMD_MASK  0x00000080 //Mask bit for FC_INT1

//Frame Composer High/Low Priority Overflow Interrupt Mask Register 2
#define FC_MASK2  0x00004368
#define FC_MASK2_HIGHPRIORITY_OVERFLOW_MASK  0x00000001 //Mask bit for FC_INT2
#define FC_MASK2_LOWPRIORITY_OVERFLOW_MASK  0x00000002 //Mask bit for FC_INT2

//Frame Composer Pixel Repetition Configuration Register
#define FC_PRCONF  0x00004380
#define FC_PRCONF_OUTPUT_PR_FACTOR_MASK  0x0000000F //Configures the video pixel repetition ratio to be sent on the AVI InfoFrame
#define FC_PRCONF_INCOMING_PR_FACTOR_MASK  0x000000F0 //Configures the input video pixel repetition

//Frame Composer Scrambler Control
#define FC_SCRAMBLER_CTRL  0x00004384
#define FC_SCRAMBLER_CTRL_SCRAMBLER_ON_MASK  0x00000001 //When set (1'b1), this field activates the HDMI 2
#define FC_SCRAMBLER_CTRL_SCRAMBLER_UCP_LINE_MASK  0x00000010 //Debug register

//Frame Composer Multi-Stream Audio Control
#define FC_MULTISTREAM_CTRL  0x00004388
#define FC_MULTISTREAM_CTRL_FC_MAS_PACKET_EN_MASK  0x00000001 //This field, when set (1'b1), activates the HDMI 2

//Frame Composer Packet Transmission Control
#define FC_PACKET_TX_EN  0x0000438C
#define FC_PACKET_TX_EN_ACR_TX_EN_MASK  0x00000001 //ACR packet transmission control 1b: Transmission enabled 0b: Transmission disabled
#define FC_PACKET_TX_EN_GCP_TX_EN_MASK  0x00000002 //GCP transmission control 1b: Transmission enabled 0b: Transmission disabled
#define FC_PACKET_TX_EN_AVI_TX_EN_MASK  0x00000004 //AVI packet transmission control 1b: Transmission enabled 0b: Transmission disabled
#define FC_PACKET_TX_EN_AUDI_TX_EN_MASK  0x00000008 //AUDI packet transmission control 1b: Transmission enabled 0b: Transmission disabled
#define FC_PACKET_TX_EN_AUT_TX_EN_MASK  0x00000010 //ACP, SPD, VSIF, ISRC1, and SRC2 packet transmission control 1b: Transmission enabled 0b: Transmission disabled
#define FC_PACKET_TX_EN_AMP_TX_EN_MASK  0x00000020 //AMP transmission control 1b: Transmission enabled 0b: Transmission disabled
#define FC_PACKET_TX_EN_NVBI_TX_EN_MASK  0x00000040 //NTSC VBI transmission control 1b: Transmission enabled 0b: Transmission disabled

//Frame Composer Active Space Control
#define FC_ACTSPC_HDLR_CFG  0x000043A0
#define FC_ACTSPC_HDLR_CFG_ACTSPC_HDLR_EN_MASK  0x00000001 //Active Space Handler Control 1b: Fixed active space value mode enabled
#define FC_ACTSPC_HDLR_CFG_ACTSPC_HDLR_TGL_MASK  0x00000002 //Active Space handler control 1b: Active space 1 value is different from Active Space 2 value

//Frame Composer Input Video 2D VActive Pixels Register 0
#define FC_INVACT_2D_0  0x000043A4
#define FC_INVACT_2D_0_FC_INVACT_2D_0_MASK  0x000000FF //2D Input video vertical active pixel region width

//Frame Composer Input Video VActive pixels Register 1
#define FC_INVACT_2D_1  0x000043A8
#define FC_INVACT_2D_1_FC_INVACT_2D_1_MASK  0x0000000F //2D Input video vertical active pixel region width

//Frame Composer GMD Packet Status Register Gamut metadata packet status bit information for no_current_gmd, next_gmd_field, gmd_packet_sequence and current_gamut_seq_num
#define FC_GMD_STAT  0x00004400
#define FC_GMD_STAT_IGMDCURRENT_GAMUT_SEQ_NUM_MASK  0x0000000F //Gamut scheduling: Current Gamut packet sequence number
#define FC_GMD_STAT_IGMDPACKET_SEQ_MASK  0x00000030 //Gamut scheduling: Gamut packet sequence
#define FC_GMD_STAT_IGMDDNEXT_FIELD_MASK  0x00000040 //Gamut scheduling: Gamut Next field
#define FC_GMD_STAT_IGMDNO_CRNT_GBD_MASK  0x00000080 //Gamut scheduling: No current gamut data

//Frame Composer GMD Packet Enable Register This register enables Gamut metadata (GMD) packet transmission
#define FC_GMD_EN  0x00004404
#define FC_GMD_EN_GMDENABLETX_MASK  0x00000001 //Gamut Metadata packet transmission enable (1b)

//Frame Composer GMD Packet Update Register This register performs an GMD packet content update according to the configured packet body (FC_GMD_PB0 to FC_GMD_PB27) and packet header (FC_GMD_HB)
#define FC_GMD_UP  0x00004408
#define FC_GMD_UP_GMDUPDATEPACKET_MASK  0x00000001 //Gamut Metadata packet update

//Frame Composer GMD Packet Schedule Configuration Register This register configures the number of GMD packets to be inserted per frame (starting always in the line where the active Vsync appears) and the line spacing between the transmitted GMD packets
#define FC_GMD_CONF  0x0000440C
#define FC_GMD_CONF_GMDPACKETLINESPACING_MASK  0x0000000F //Number of line spacing between the transmitted GMD packets
#define FC_GMD_CONF_GMDPACKETSINFRAME_MASK  0x000000F0 //Number of GMD packets per frame or video field (profile P0)

//Frame Composer GMD Packet Profile and Gamut Sequence Configuration Register This register configures the GMD packet header affected_gamut_seq_num and gmd_profile bits
#define FC_GMD_HB  0x00004410
#define FC_GMD_HB_GMDAFFECTED_GAMUT_SEQ_NUM_MASK  0x0000000F //Affected gamut sequence number
#define FC_GMD_HB_GMDGBD_PROFILE_MASK  0x00000070 //GMD profile bits

#define FC_GMD_PB0  0x4414
#define FC_GMD_PB27 0x4480

//Frame Composer AMP Packet Header Register 1
#define FC_AMP_HB1  0x000044A0
#define FC_AMP_HB1_FC_AMP_HB0_MASK  0x000000FF //Frame Composer AMP Packet Header Register 1

//Frame Composer AMP Packet Header Register 2
#define FC_AMP_HB2  0x000044A4
#define FC_AMP_HB2_FC_AMP_HB1_MASK  0x000000FF //Frame Composer AMP Packet Header Register 2

#define FC_AMP_PB   0x000044A8

//Frame Composer NTSC VBI Packet Header Register 1
#define FC_NVBI_HB1  0x00004520
#define FC_NVBI_HB1_FC_NVBI_HB0_MASK  0x000000FF //Frame Composer NTSC VBI Packet Header Register 1

//Frame Composer NTSC VBI Packet Header Register 2
#define FC_NVBI_HB2  0x00004524
#define FC_NVBI_HB2_FC_NVBI_HB1_MASK  0x000000FF //Frame Composer NTSC VBI Packet Header Register 2

/***********************add*********************************/
#define FC_NVBI_PB  0x00004528
#define FC_DRM_UP   0x0000459C
#define FC_DRM_HB   0x000045A0
#define FC_DRM_PB   0x000045A8


//Frame Composer video/audio Force Enable Register This register allows to force the controller to output audio and video data the values configured in the FC_DBGAUD and FC_DBGTMDS registers
#define FC_DBGFORCE  0x00004800
#define FC_DBGFORCE_FORCEVIDEO_MASK  0x00000001 //Force fixed video output with FC_DBGTMDSx register contents
#define FC_DBGFORCE_FORCEAUDIO_MASK  0x00000010 //Force fixed audio output with FC_DBGAUDxCHx register contents

//Frame Composer Audio Data Channel 0 Register 0 Configures the audio fixed data to be used in channel 0 when in fixed audio selection
#define FC_DBGAUD0CH0  0x00004804
#define FC_DBGAUD0CH0_FC_DBGAUD0CH0_MASK  0x000000FF //Frame Composer Audio Data Channel 0 Register 0

//Frame Composer Audio Data Channel 0 Register 1 Configures the audio fixed data to be used in channel 0 when in fixed audio selection
#define FC_DBGAUD1CH0  0x00004808
#define FC_DBGAUD1CH0_FC_DBGAUD1CH0_MASK  0x000000FF //Frame Composer Audio Data Channel 0 Register 1

//Frame Composer Audio Data Channel 0 Register 2 Configures the audio fixed data to be used in channel 0 when in fixed audio selection
#define FC_DBGAUD2CH0  0x0000480C
#define FC_DBGAUD2CH0_FC_DBGAUD2CH0_MASK  0x000000FF //Frame Composer Audio Data Channel 0 Register 2

//Frame Composer Audio Data Channel 1 Register 0 Configures the audio fixed data to be used in channel 1 when in fixed audio selection
#define FC_DBGAUD0CH1  0x00004810
#define FC_DBGAUD0CH1_FC_DBGAUD0CH1_MASK  0x000000FF //Frame Composer Audio Data Channel 1 Register 0

//Frame Composer Audio Data Channel 1 Register 1 Configures the audio fixed data to be used in channel 1 when in fixed audio selection
#define FC_DBGAUD1CH1  0x00004814
#define FC_DBGAUD1CH1_FC_DBGAUD1CH1_MASK  0x000000FF //Frame Composer Audio Data Channel 1 Register 1

//Frame Composer Audio Data Channel 1 Register 2 Configures the audio fixed data to be used in channel 1 when in fixed audio selection
#define FC_DBGAUD2CH1  0x00004818
#define FC_DBGAUD2CH1_FC_DBGAUD2CH1_MASK  0x000000FF //Frame Composer Audio Data Channel 1 Register 2

//Frame Composer Audio Data Channel 2 Register 0 Configures the audio fixed data to be used in channel 2 when in fixed audio selection
#define FC_DBGAUD0CH2  0x0000481C
#define FC_DBGAUD0CH2_FC_DBGAUD0CH2_MASK  0x000000FF //Frame Composer Audio Data Channel 2 Register 0

//Frame Composer Audio Data Channel 2 Register 1 Configures the audio fixed data to be used in channel 2 when in fixed audio selection
#define FC_DBGAUD1CH2  0x00004820
#define FC_DBGAUD1CH2_FC_DBGAUD1CH2_MASK  0x000000FF //Frame Composer Audio Data Channel 2 Register 1

//Frame Composer Audio Data Channel 2 Register 2 Configures the audio fixed data to be used in channel 2 when in fixed audio selection
#define FC_DBGAUD2CH2  0x00004824
#define FC_DBGAUD2CH2_FC_DBGAUD2CH2_MASK  0x000000FF //Frame Composer Audio Data Channel 2 Register 2

//Frame Composer Audio Data Channel 3 Register 0 Configures the audio fixed data to be used in channel 3 when in fixed audio selection
#define FC_DBGAUD0CH3  0x00004828
#define FC_DBGAUD0CH3_FC_DBGAUD0CH3_MASK  0x000000FF //Frame Composer Audio Data Channel 3 Register 0

//Frame Composer Audio Data Channel 3 Register 1 Configures the audio fixed data to be used in channel 3 when in fixed audio selection
#define FC_DBGAUD1CH3  0x0000482C
#define FC_DBGAUD1CH3_FC_DBGAUD1CH3_MASK  0x000000FF //Frame Composer Audio Data Channel 3 Register 1

//Frame Composer Audio Data Channel 3 Register 2 Configures the audio fixed data to be used in channel 3 when in fixed audio selection
#define FC_DBGAUD2CH3  0x00004830
#define FC_DBGAUD2CH3_FC_DBGAUD2CH3_MASK  0x000000FF //Frame Composer Audio Data Channel 3 Register 2

//Frame Composer Audio Data Channel 4 Register 0 Configures the audio fixed data to be used in channel 4 when in fixed audio selection
#define FC_DBGAUD0CH4  0x00004834
#define FC_DBGAUD0CH4_FC_DBGAUD0CH4_MASK  0x000000FF //Frame Composer Audio Data Channel 4 Register 0

//Frame Composer Audio Data Channel 4 Register 1 Configures the audio fixed data to be used in channel 4 when in fixed audio selection
#define FC_DBGAUD1CH4  0x00004838
#define FC_DBGAUD1CH4_FC_DBGAUD1CH4_MASK  0x000000FF //Frame Composer Audio Data Channel 4 Register 1

//Frame Composer Audio Data Channel 4 Register 2 Configures the audio fixed data to be used in channel 4 when in fixed audio selection
#define FC_DBGAUD2CH4  0x0000483C
#define FC_DBGAUD2CH4_FC_DBGAUD2CH4_MASK  0x000000FF //Frame Composer Audio Data Channel 4 Register 2

//Frame Composer Audio Data Channel 5 Register 0 Configures the audio fixed data to be used in channel 5 when in fixed audio selection
#define FC_DBGAUD0CH5  0x00004840
#define FC_DBGAUD0CH5_FC_DBGAUD0CH5_MASK  0x000000FF //Frame Composer Audio Data Channel 5 Register 0

//Frame Composer Audio Data Channel 5 Register 1 Configures the audio fixed data to be used in channel 5 when in fixed audio selection
#define FC_DBGAUD1CH5  0x00004844
#define FC_DBGAUD1CH5_FC_DBGAUD1CH5_MASK  0x000000FF //Frame Composer Audio Data Channel 5 Register 1

//Frame Composer Audio Data Channel 5 Register 2 Configures the audio fixed data to be used in channel 5 when in fixed audio selection
#define FC_DBGAUD2CH5  0x00004848
#define FC_DBGAUD2CH5_FC_DBGAUD2CH5_MASK  0x000000FF //Frame Composer Audio Data Channel 5 Register 2

//Frame Composer Audio Data Channel 6 Register 0 Configures the audio fixed data to be used in channel 6 when in fixed audio selection
#define FC_DBGAUD0CH6  0x0000484C
#define FC_DBGAUD0CH6_FC_DBGAUD0CH6_MASK  0x000000FF //Frame Composer Audio Data Channel 6 Register 0

//Frame Composer Audio Data Channel 6 Register 1 Configures the audio fixed data to be used in channel 6 when in fixed audio selection
#define FC_DBGAUD1CH6  0x00004850
#define FC_DBGAUD1CH6_FC_DBGAUD1CH6_MASK  0x000000FF //Frame Composer Audio Data Channel 6 Register 1

//Frame Composer Audio Data Channel 6 Register 2 Configures the audio fixed data to be used in channel 6 when in fixed audio selection
#define FC_DBGAUD2CH6  0x00004854
#define FC_DBGAUD2CH6_FC_DBGAUD2CH6_MASK  0x000000FF //Frame Composer Audio Data Channel 6 Register 2

//Frame Composer Audio Data Channel 7 Register 0 Configures the audio fixed data to be used in channel 7 when in fixed audio selection
#define FC_DBGAUD0CH7  0x00004858
#define FC_DBGAUD0CH7_FC_DBGAUD0CH7_MASK  0x000000FF //Frame Composer Audio Data Channel 7 Register 0

//Frame Composer Audio Data Channel 7 Register 1 Configures the audio fixed data to be used in channel 7 when in fixed audio selection
#define FC_DBGAUD1CH7  0x0000485C
#define FC_DBGAUD1CH7_FC_DBGAUD1CH7_MASK  0x000000FF //Frame Composer Audio Data Channel 7 Register 1

//Frame Composer Audio Data Channel 7 Register 2 Configures the audio fixed data to be used in channel 7 when in fixed audio selection
#define FC_DBGAUD2CH7  0x00004860
#define FC_DBGAUD2CH7_FC_DBGAUD2CH7_MASK  0x000000FF //Frame Composer Audio Data Channel 7 Register 2

#define FC_DBGTMDS0  0x4864
#define FC_DBGTMDS1  0x4868
#define FC_DBGTMDS2  0x486C

//PHY Configuration Register This register holds the power down, data enable polarity, and interface control of the HDMI Source PHY control
#define PHY_CONF0  0x0000C000
#define PHY_CONF0_SELDIPIF_MASK  0x00000001 //Select interface control
#define PHY_CONF0_SELDATAENPOL_MASK  0x00000002 //Select data enable polarity
#define PHY_CONF0_ENHPDRXSENSE_MASK  0x00000004 //PHY ENHPDRXSENSE signal
#define PHY_CONF0_TXPWRON_MASK  0x00000008 //PHY TXPWRON signal
#define PHY_CONF0_PDDQ_MASK  0x00000010 //PHY PDDQ signal
#define PHY_CONF0_SVSRET_MASK  0x00000020 //Reserved as "spare" register with no associated functionality
#define PHY_CONF0_SPARES_1_MASK  0x00000040 //Reserved as "spare" register with no associated functionality
#define PHY_CONF0_SPARES_2_MASK  0x00000080 //Reserved as "spare" register with no associated functionality

//PHY Test Interface Register 0 PHY TX mapped test interface (control)
#define PHY_TST0  0x0000C004
#define PHY_TST0_SPARE_0_MASK  0x00000001 //Reserved as "spare" register with no associated functionality
#define PHY_TST0_SPARE_1_MASK  0x0000000E //Reserved as "spare" bit with no associated functionality
#define PHY_TST0_SPARE_3_MASK  0x00000010 //Reserved as "spare" register with no associated functionality
#define PHY_TST0_SPARE_4_MASK  0x00000020 //Reserved as "spare" register with no associated functionality
#define PHY_TST0_SPARE_2_MASK  0x000000C0 //Reserved as "spare" bit with no associated functionality

//PHY Test Interface Register 1 PHY TX mapped text interface (data in)
#define PHY_TST1  0x0000C008
#define PHY_TST1_SPARE_MASK  0x000000FF //Reserved as "spare" register with no associated functionality

//PHY Test Interface Register 2 PHY TX mapped text interface (data out)
#define PHY_TST2  0x0000C00C
#define PHY_TST2_SPARE_MASK  0x000000FF //Reserved as "spare" register with no associated functionality

//PHY RXSENSE, PLL Lock, and HPD Status Register This register contains the following active high packet sent status indications
#define PHY_STAT0  0x0000C010
#define PHY_STAT0_TX_PHY_LOCK_MASK  0x00000001 //Status bit
#define PHY_STAT0_HPD_MASK  0x00000002 //Status bit
#define PHY_STAT0_RX_SENSE_0_MASK  0x00000010 //Status bit
#define PHY_STAT0_RX_SENSE_1_MASK  0x00000020 //Status bit
#define PHY_STAT0_RX_SENSE_2_MASK  0x00000040 //Status bit
#define PHY_STAT0_RX_SENSE_3_MASK  0x00000080 //Status bit

//PHY RXSENSE, PLL Lock, and HPD Interrupt Register This register contains the interrupt indication of the PHY_STAT0 status interrupts
#define PHY_INT0  0x0000C014
#define PHY_INT0_TX_PHY_LOCK_MASK  0x00000001 //Interrupt indication bit
#define PHY_INT0_HPD_MASK  0x00000002 //Interrupt indication bit
#define PHY_INT0_RX_SENSE_0_MASK  0x00000010 //Interrupt indication bit
#define PHY_INT0_RX_SENSE_1_MASK  0x00000020 //Interrupt indication bit
#define PHY_INT0_RX_SENSE_2_MASK  0x00000040 //Interrupt indication bit
#define PHY_INT0_RX_SENSE_3_MASK  0x00000080 //Interrupt indication bit

//PHY RXSENSE, PLL Lock, and HPD Mask Register Mask register for generation of PHY_INT0 interrupts
#define PHY_MASK0  0x0000C018
#define PHY_MASK0_TX_PHY_LOCK_MASK  0x00000001 //Mask bit for PHY_INT0
#define PHY_MASK0_HPD_MASK  0x00000002 //Mask bit for PHY_INT0
#define PHY_MASK0_RX_SENSE_0_MASK  0x00000010 //Mask bit for PHY_INT0
#define PHY_MASK0_RX_SENSE_1_MASK  0x00000020 //Mask bit for PHY_INT0
#define PHY_MASK0_RX_SENSE_2_MASK  0x00000040 //Mask bit for PHY_INT0
#define PHY_MASK0_RX_SENSE_3_MASK  0x00000080 //Mask bit for PHY_INT0

//PHY RXSENSE, PLL Lock, and HPD Polarity Register Polarity register for generation of PHY_INT0 interrupts
#define PHY_POL0  0x0000C01C
#define PHY_POL0_TX_PHY_LOCK_MASK  0x00000001 //Polarity bit for PHY_INT0
#define PHY_POL0_HPD_MASK  0x00000002 //Polarity bit for PHY_INT0
#define PHY_POL0_RX_SENSE_0_MASK  0x00000010 //Polarity bit for PHY_INT0
#define PHY_POL0_RX_SENSE_1_MASK  0x00000020 //Polarity bit for PHY_INT0
#define PHY_POL0_RX_SENSE_2_MASK  0x00000040 //Polarity bit for PHY_INT0
#define PHY_POL0_RX_SENSE_3_MASK  0x00000080 //Polarity bit for PHY_INT0

//PHY I2C Slave Address Configuration Register
#define PHY_I2CM_SLAVE  0x0000C080
#define PHY_I2CM_SLAVE_SLAVEADDR_MASK  0x0000007F //Slave address to be sent during read and write operations

//PHY I2C Address Configuration Register This register writes the address for read and write operations
#define PHY_I2CM_ADDRESS  0x0000C084
#define PHY_I2CM_ADDRESS_ADDRESS_MASK  0x000000FF //Register address for read and write operations

//PHY I2C Data Write Register 1
#define PHY_I2CM_DATAO_1  0x0000C088
#define PHY_I2CM_DATAO_1_DATAO_MASK  0x000000FF //Data MSB (datao[15:8]) to be written on register pointed by phy_i2cm_address [7:0]

//PHY I2C Data Write Register 0
#define PHY_I2CM_DATAO_0  0x0000C08C
#define PHY_I2CM_DATAO_0_DATAO_MASK  0x000000FF //Data LSB (datao[7:0]) to be written on register pointed by phy_i2cm_address [7:0]

//PHY I2C Data Read Register 1
#define PHY_I2CM_DATAI_1  0x0000C090
#define PHY_I2CM_DATAI_1_DATAI_MASK  0x000000FF //Data MSB (datai[15:8]) read from register pointed by phy_i2cm_address[7:0]

//PHY I2C Data Read Register 0
#define PHY_I2CM_DATAI_0  0x0000C094
#define PHY_I2CM_DATAI_0_DATAI_MASK  0x000000FF //Data LSB (datai[7:0]) read from register pointed by phy_i2cm_address[7:0]

//PHY I2C RD/RD_EXT/WR Operation Register This register requests read and write operations from the I2C Master PHY
#define PHY_I2CM_OPERATION  0x0000C098
#define PHY_I2CM_OPERATION_RD_MASK  0x00000001 //Read operation request
#define PHY_I2CM_OPERATION_WR_MASK  0x00000010 //Write operation request

//PHY I2C Done Interrupt Register This register contains and configures I2C master PHY done interrupt
#define PHY_I2CM_INT  0x0000C09C
#define PHY_I2CM_INT_DONE_STATUS_MASK  0x00000001 //Operation done status bit
#define PHY_I2CM_INT_DONE_INTERRUPT_MASK  0x00000002 //Operation done interrupt bit
#define PHY_I2CM_INT_DONE_MASK_MASK  0x00000004 //Done interrupt mask signal
#define PHY_I2CM_INT_DONE_POL_MASK  0x00000008 //Done interrupt polarity configuration

//PHY I2C error Interrupt Register This register contains and configures the I2C master PHY error interrupts
#define PHY_I2CM_CTLINT  0x0000C0A0
#define PHY_I2CM_CTLINT_ARBITRATION_STATUS_MASK  0x00000001 //Arbitration error status bit
#define PHY_I2CM_CTLINT_ARBITRATION_INTERRUPT_MASK  0x00000002 //Arbitration error interrupt bit {arbitration_interrupt = (arbitration_mask==0b) && (arbitration_status==arbitration_pol)} Note: This bit field is read by the sticky bits present on the ih_i2cmphy_stat0 register
#define PHY_I2CM_CTLINT_ARBITRATION_MASK_MASK  0x00000004 //Arbitration error interrupt mask signal
#define PHY_I2CM_CTLINT_ARBITRATION_POL_MASK  0x00000008 //Arbitration error interrupt polarity configuration
#define PHY_I2CM_CTLINT_NACK_STATUS_MASK  0x00000010 //Not acknowledge error status bit
#define PHY_I2CM_CTLINT_NACK_INTERRUPT_MASK  0x00000020 //Not acknowledge error interrupt bit
#define PHY_I2CM_CTLINT_NACK_MASK_MASK  0x00000040 //Not acknowledge error interrupt mask signal
#define PHY_I2CM_CTLINT_NACK_POL_MASK  0x00000080 //Not acknowledge error interrupt polarity configuration

//PHY I2C Speed control Register This register wets the I2C Master PHY to work in either Fast or Standard mode
#define PHY_I2CM_DIV  0x0000C0A4
#define PHY_I2CM_DIV_SPARE_MASK  0x00000007 //Reserved as "spare" register with no associated functionality
#define PHY_I2CM_DIV_FAST_STD_MODE_MASK  0x00000008 //Sets the I2C Master to work in Fast Mode or Standard Mode: 1: Fast Mode 0: Standard Mode

//PHY I2C SW reset control register This register sets the I2C Master PHY software reset
#define PHY_I2CM_SOFTRSTZ  0x0000C0A8
#define PHY_I2CM_SOFTRSTZ_I2C_SOFTRSTZ_MASK  0x00000001 //I2C Master Software Reset

//PHY I2C Slow Speed SCL High Level Control Register 1
#define PHY_I2CM_SS_SCL_HCNT_1_ADDR  0x0000C0AC
#define PHY_I2CM_SS_SCL_HCNT_1_ADDR_I2CMP_SS_SCL_HCNT1_MASK  0x000000FF //PHY I2C Slow Speed SCL High Level Control Register 1

//PHY I2C Slow Speed SCL High Level Control Register 0
#define PHY_I2CM_SS_SCL_HCNT_0_ADDR  0x0000C0B0
#define PHY_I2CM_SS_SCL_HCNT_0_ADDR_I2CMP_SS_SCL_HCNT0_MASK  0x000000FF //PHY I2C Slow Speed SCL High Level Control Register 0

//PHY I2C Slow Speed SCL Low Level Control Register 1
#define PHY_I2CM_SS_SCL_LCNT_1_ADDR  0x0000C0B4
#define PHY_I2CM_SS_SCL_LCNT_1_ADDR_I2CMP_SS_SCL_LCNT1_MASK  0x000000FF //PHY I2C Slow Speed SCL Low Level Control Register 1

//PHY I2C Slow Speed SCL Low Level Control Register 0
#define PHY_I2CM_SS_SCL_LCNT_0_ADDR  0x0000C0B8
#define PHY_I2CM_SS_SCL_LCNT_0_ADDR_I2CMP_SS_SCL_LCNT0_MASK  0x000000FF //PHY I2C Slow Speed SCL Low Level Control Register 0

//PHY I2C Fast Speed SCL High Level Control Register 1
#define PHY_I2CM_FS_SCL_HCNT_1_ADDR  0x0000C0BC
#define PHY_I2CM_FS_SCL_HCNT_1_ADDR_I2CMP_FS_SCL_HCNT1_MASK  0x000000FF //PHY I2C Fast Speed SCL High Level Control Register 1

//PHY I2C Fast Speed SCL High Level Control Register 0
#define PHY_I2CM_FS_SCL_HCNT_0_ADDR  0x0000C0C0
#define PHY_I2CM_FS_SCL_HCNT_0_ADDR_I2CMP_FS_SCL_HCNT0_MASK  0x000000FF //PHY I2C Fast Speed SCL High Level Control Register 0

//PHY I2C Fast Speed SCL Low Level Control Register 1
#define PHY_I2CM_FS_SCL_LCNT_1_ADDR  0x0000C0C4
#define PHY_I2CM_FS_SCL_LCNT_1_ADDR_I2CMP_FS_SCL_LCNT1_MASK  0x000000FF //PHY I2C Fast Speed SCL Low Level Control Register 1

//PHY I2C Fast Speed SCL Low Level Control Register 0
#define PHY_I2CM_FS_SCL_LCNT_0_ADDR  0x0000C0C8
#define PHY_I2CM_FS_SCL_LCNT_0_ADDR_I2CMP_FS_SCL_LCNT0_MASK  0x000000FF //PHY I2C Fast Speed SCL Low Level Control Register 0

//PHY I2C SDA HOLD Control Register
#define PHY_I2CM_SDA_HOLD  0x0000C0CC
#define PHY_I2CM_SDA_HOLD_OSDA_HOLD_MASK  0x000000FF //Defines the number of SFR clock cycles to meet tHD:DAT (300 ns) osda_hold = round_to_high_integer (300 ns / (1/isfrclk_frequency))

//PHY I2C/JTAG I/O Configuration Control Register
#define JTAG_PHY_CONFIG  0x0000C0D0
#define JTAG_PHY_CONFIG_JTAG_TRST_N_MASK  0x00000001 //Configures the JTAG PHY interface output pin JTAG_TRST_N when in internal control mode (iphy_ext_ctrl=1'b0) or ophyext_jtag_trst_n when PHY_EXTERNAL=1
#define JTAG_PHY_CONFIG_I2C_JTAGZ_MASK  0x00000010 //Configures the JTAG PHY interface output pin I2C_JTAGZ to select the PHY configuration interface when in internal control mode (iphy_ext_ctrl=1'b0) or ophyext_jtag_i2c_jtagz when PHY_EXTERNAL=1

//PHY JTAG Clock Control Register
#define JTAG_PHY_TAP_TCK  0x0000C0D4
#define JTAG_PHY_TAP_TCK_JTAG_TCK_MASK  0x00000001 //Configures the JTAG PHY interface pin JTAG_TCK when in internal control mode (iphy_ext_ctrl=1'b0) or ophyext_jtag_tck when PHY_EXTERNAL=1

//PHY JTAG TAP In Control Register
#define JTAG_PHY_TAP_IN  0x0000C0D8
#define JTAG_PHY_TAP_IN_JTAG_TDI_MASK  0x00000001 //Configures the JTAG PHY interface pin JTAG_TDI when in internal control mode (iphy_ext_ctrl=1'b0) or ophyext_jtag_tdi when PHY_EXTERNAL=1
#define JTAG_PHY_TAP_IN_JTAG_TMS_MASK  0x00000010 //Configures the JTAG PHY interface pin JTAG_TMS when in internal control mode (iphy_ext_ctrl=1'b0) or ophyext_jtag_tms when PHY_EXTERNAL=1

//PHY JTAG TAP Out Control Register
#define JTAG_PHY_TAP_OUT  0x0000C0DC
#define JTAG_PHY_TAP_OUT_JTAG_TDO_MASK  0x00000001 //Read JTAG PHY interface input pin JTAG_TDO when in internal control mode (iphy_ext_ctrl=1'b0) or iphyext_jtag_tdo when PHY_EXTERNAL=1
#define JTAG_PHY_TAP_OUT_JTAG_TDO_EN_MASK  0x00000010 //Read JTAG PHY interface input pin JTAG_TDO_EN when in internal control mode (iphy_ext_ctrl=1'b0) or iphyext_jtag_tdo_en when PHY_EXTERNAL=1

//PHY JTAG Address Control Register
#define JTAG_PHY_ADDR  0x0000C0E0
#define JTAG_PHY_ADDR_JTAG_ADDR_MASK  0x000000FF //Configures the JTAG PHY interface pin JTAG_ADDR[7:0] when in internal control mode (iphy_ext_ctrl=1'b0) or iphyext_jtag_addr[7:0] when PHY_EXTERNAL=1


//Audio Clock Regenerator N Value Register 1 For N expected values, refer to the HDMI 1
#define AUD_N1  0x0000C800
#define AUD_N1_AUDN_MASK  0x000000FF //HDMI Audio Clock Regenerator N value

//Audio Clock Regenerator N Value Register 2 For N expected values, refer to the HDMI 1
#define AUD_N2  0x0000C804
#define AUD_N2_AUDN_MASK  0x000000FF //HDMI Audio Clock Regenerator N value

//Audio Clock Regenerator N Value Register 3 For N expected values, refer to the HDMI 1
#define AUD_N3  0x0000C808
#define AUD_N3_AUDN_MASK  0x0000000F //HDMI Audio Clock Regenerator N value
#define AUD_N3_NCTS_ATOMIC_WRITE_MASK  0x00000080 //When set, the new N and CTS values are only used when aud_n1 register is written

//Audio Clock Regenerator CTS Value Register 1 For CTS expected values, refer to the HDMI 1
#define AUD_CTS1  0x0000C80C
#define AUD_CTS1_AUDCTS_MASK  0x000000FF //HDMI Audio Clock Regenerator CTS calculated value

//Audio Clock Regenerator CTS Register 2 For CTS expected values, refer to the HDMI 1
#define AUD_CTS2  0x0000C810
#define AUD_CTS2_AUDCTS_MASK  0x000000FF //HDMI Audio Clock Regenerator CTS calculated value

//Audio Clock Regenerator CTS value Register 3
#define AUD_CTS3  0x0000C814
#define AUD_CTS3_AUDCTS_MASK  0x0000000F //HDMI Audio Clock Regenerator CTS calculated value
#define AUD_CTS3_CTS_MANUAL_MASK  0x00000010 //If the CTS_manual bit equals 0b, this registers contains audCTS[19:0] generated by the Cycle time counter according to the specified timing
#define AUD_CTS3_N_SHIFT_MASK  0x000000E0 //N_shift factor configuration: N_shift | Shift Factor | Action 0 | 1 | This is the N shift factor used for the case that N' ="audN[19:0]"

//Audio Input Clock FS Factor Register
#define AUD_INPUTCLKFS  0x0000C818
#define AUD_INPUTCLKFS_IFSFACTOR_MASK  0x00000007 //Fs factor configuration: ifsfactor[2:0] | Audio Clock | Action 0 | 128xFs | If you select the Bypass SPDIF DRU unit in coreConsultant, the input audio clock (either I2S or SPDIF according to configuration) is used at the audio packetizer to calculate the CTS value and ACR packet insertion rate

#define AUD_CTS_DITHER  0x0000C09C

//AudioDMA Registers
#define AHB_DMA_CONF0               0x0000D800
#define AHB_DMA_START               0x0000D804
#define AHB_DMA_STOP                0x0000D808
#define AHB_DMA_THRSLD              0x0000D80C
#define AHB_DMA_STRADDR_SET0        0x0000D810
#define AHB_DMA_STPADDR_SET0        0x0000D820
#define AHB_DMA_BSTRADDR            0x0000D830
#define AHB_DMA_MBLENGTH0           0x0000D840
#define AHB_DMA_MBLENGTH1           0x0000D844
#define AHB_DMA_MASK                0x0000D850
#define AHB_DMA_CONF1               0x0000D858
#define AHB_DMA_BUFFMASK            0x0000D864
#define AHB_DMA_MASK1               0x0000D86C
#define AHB_DMA_STATUS              0x0000D870
#define AHB_DMA_CONF2               0x0000D874
#define AHB_DMA_STRADDR_SET1        0x0000D880
#define AHB_DMA_STPADDR_SET1        0x0000D890

//Main Controller Synchronous Clock Domain Disable Register
#define MC_CLKDIS  0x00010004
#define MC_CLKDIS_PIXELCLK_DISABLE_MASK  0x00000001 //Pixel clock synchronous disable signal
#define MC_CLKDIS_TMDSCLK_DISABLE_MASK  0x00000002 //TMDS clock synchronous disable signal
#define MC_CLKDIS_PREPCLK_DISABLE_MASK  0x00000004 //Pixel Repetition clock synchronous disable signal
#define MC_CLKDIS_AUDCLK_DISABLE_MASK  0x00000008 //Audio Sampler clock synchronous disable signal
#define MC_CLKDIS_CSCCLK_DISABLE_MASK  0x00000010 //Color Space Converter clock synchronous disable signal
#define MC_CLKDIS_CECCLK_DISABLE_MASK  0x00000020 //CEC Engine clock synchronous disable signal
#define MC_CLKDIS_HDCPCLK_DISABLE_MASK  0x00000040 //HDCP clock synchronous disable signal

//Main Controller Software Reset Register Main controller software reset request per clock domain
#define MC_SWRSTZREQ  0x00010008
#define MC_SWRSTZREQ_PIXELSWRST_REQ_MASK  0x00000001 //Pixel software reset request
#define MC_SWRSTZREQ_TMDSSWRST_REQ_MASK  0x00000002 //TMDS software reset request
#define MC_SWRSTZREQ_PREPSWRST_REQ_MASK  0x00000004 //Pixel Repetition software reset request
#define MC_SWRSTZREQ_II2SSWRST_REQ_MASK  0x00000008 //I2S audio software reset request
#define MC_SWRSTZREQ_ISPDIFSWRST_REQ_MASK  0x00000010 //SPDIF audio software reset request
#define MC_SWRSTZREQ_CECSWRST_REQ_MASK  0x00000040 //CEC software reset request
#define MC_SWRSTZREQ_IGPASWRST_REQ_MASK  0x00000080 //GPAUD interface soft reset request

//Main Controller HDCP Bypass Control Register
#define MC_OPCTRL  0x0001000C
#define MC_OPCTRL_HDCP_BLOCK_BYP_MASK  0x00000001 //Block HDCP bypass mechanism - 1'b0: This is the default value

//Main Controller Feed Through Control Register
#define MC_FLOWCTRL  0x00010010
#define MC_FLOWCTRL_FEED_THROUGH_OFF_MASK  0x00000001 //Video path Feed Through enable bit: - 1b: Color Space Converter is in the video data path

//Main Controller PHY Reset Register
#define MC_PHYRSTZ  0x00010014
#define MC_PHYRSTZ_PHYRSTZ_MASK  0x00000001 //HDMI Source PHY active low reset control for PHY GEN1, active high reset control for PHY GEN2

//Main Controller Clock Present Register
#define MC_LOCKONCLOCK  0x00010018
#define MC_LOCKONCLOCK_CECCLK_MASK  0x00000001 //CEC clock status
#define MC_LOCKONCLOCK_AUDIOSPDIFCLK_MASK  0x00000004 //SPDIF clock status
#define MC_LOCKONCLOCK_I2SCLK_MASK  0x00000008 //I2S clock status
#define MC_LOCKONCLOCK_PREPCLK_MASK  0x00000010 //Pixel Repetition clock status
#define MC_LOCKONCLOCK_TCLK_MASK  0x00000020 //TMDS clock status
#define MC_LOCKONCLOCK_PCLK_MASK  0x00000040 //Pixel clock status
#define MC_LOCKONCLOCK_IGPACLK_MASK  0x00000080 //GPAUD interface clock status

#define MC_LOCKONCLOCK_2  0x00010024
#define MC_SWRSTZREQ_2    0x00010028

//Main Controller Synchronous Clock Domain Disable Register
#define MC_CLKDIS  0x00010004
#define MC_CLKDIS_PIXELCLK_DISABLE_MASK  0x00000001 //Pixel clock synchronous disable signal
#define MC_CLKDIS_TMDSCLK_DISABLE_MASK  0x00000002 //TMDS clock synchronous disable signal
#define MC_CLKDIS_PREPCLK_DISABLE_MASK  0x00000004 //Pixel Repetition clock synchronous disable signal
#define MC_CLKDIS_AUDCLK_DISABLE_MASK  0x00000008 //Audio Sampler clock synchronous disable signal
#define MC_CLKDIS_CSCCLK_DISABLE_MASK  0x00000010 //Color Space Converter clock synchronous disable signal
#define MC_CLKDIS_CECCLK_DISABLE_MASK  0x00000020 //CEC Engine clock synchronous disable signal
#define MC_CLKDIS_HDCPCLK_DISABLE_MASK  0x00000040 //HDCP clock synchronous disable signal

//Main Controller Software Reset Register Main controller software reset request per clock domain
#define MC_SWRSTZREQ  0x00010008
#define MC_SWRSTZREQ_PIXELSWRST_REQ_MASK  0x00000001 //Pixel software reset request
#define MC_SWRSTZREQ_TMDSSWRST_REQ_MASK  0x00000002 //TMDS software reset request
#define MC_SWRSTZREQ_PREPSWRST_REQ_MASK  0x00000004 //Pixel Repetition software reset request
#define MC_SWRSTZREQ_II2SSWRST_REQ_MASK  0x00000008 //I2S audio software reset request
#define MC_SWRSTZREQ_ISPDIFSWRST_REQ_MASK  0x00000010 //SPDIF audio software reset request
#define MC_SWRSTZREQ_CECSWRST_REQ_MASK  0x00000040 //CEC software reset request
#define MC_SWRSTZREQ_IGPASWRST_REQ_MASK  0x00000080 //GPAUD interface soft reset request

//Main Controller HDCP Bypass Control Register
#define MC_OPCTRL  0x0001000C
#define MC_OPCTRL_HDCP_BLOCK_BYP_MASK  0x00000001 //Block HDCP bypass mechanism - 1'b0: This is the default value

//Main Controller Feed Through Control Register
#define MC_FLOWCTRL  0x00010010
#define MC_FLOWCTRL_FEED_THROUGH_OFF_MASK  0x00000001 //Video path Feed Through enable bit: - 1b: Color Space Converter is in the video data path

//Main Controller PHY Reset Register
#define MC_PHYRSTZ  0x00010014
#define MC_PHYRSTZ_PHYRSTZ_MASK  0x00000001 //HDMI Source PHY active low reset control for PHY GEN1, active high reset control for PHY GEN2

//Main Controller Clock Present Register
#define MC_LOCKONCLOCK  0x00010018
#define MC_LOCKONCLOCK_CECCLK_MASK  0x00000001 //CEC clock status
#define MC_LOCKONCLOCK_AUDIOSPDIFCLK_MASK  0x00000004 //SPDIF clock status
#define MC_LOCKONCLOCK_I2SCLK_MASK  0x00000008 //I2S clock status
#define MC_LOCKONCLOCK_PREPCLK_MASK  0x00000010 //Pixel Repetition clock status
#define MC_LOCKONCLOCK_TCLK_MASK  0x00000020 //TMDS clock status
#define MC_LOCKONCLOCK_PCLK_MASK  0x00000040 //Pixel clock status
#define MC_LOCKONCLOCK_IGPACLK_MASK  0x00000080 //GPAUD interface clock status

#define MC_LOCKONCLOCK_2  0x00010024
#define MC_SWRSTZREQ_2    0x00010028

//HDCP Enable and Functional Control Configuration Register 0
#define A_HDCPCFG0  0x00014000
#define A_HDCPCFG0_HDMIDVI_MASK  0x00000001 //Configures the transmitter to operate with a HDMI capable device or with a DVI device
#define A_HDCPCFG0_EN11FEATURE_MASK  0x00000002 //Enable the use of features 1
#define A_HDCPCFG0_RXDETECT_MASK  0x00000004 //Information that a sink device was detected connected to the HDMI port
#define A_HDCPCFG0_AVMUTE_MASK  0x00000008 //This register holds the current AVMUTE state of the DWC_hdmi_tx controller, as expected to be perceived by the connected HDMI/HDCP sink device
#define A_HDCPCFG0_SYNCRICHECK_MASK  0x00000010 //Configures if the Ri check should be done at every 2s even or synchronously to every 128 encrypted frame
#define A_HDCPCFG0_BYPENCRYPTION_MASK  0x00000020 //Bypasses all the data encryption stages
#define A_HDCPCFG0_I2CFASTMODE_MASK  0x00000040 //Enable the I2C fast mode option from the transmitter's side
#define A_HDCPCFG0_ELVENA_MASK  0x00000080 //Enables the Enhanced Link Verification from the transmitter's side

//HDCP Software Reset and Functional Control Configuration Register 1
#define A_HDCPCFG1  0x00014004
#define A_HDCPCFG1_SWRESET_MASK  0x00000001 //Software reset signal, active by writing a zero and auto cleared to 1 in the following cycle
#define A_HDCPCFG1_ENCRYPTIONDISABLE_MASK  0x00000002 //Disable encryption without losing authentication
#define A_HDCPCFG1_PH2UPSHFTENC_MASK  0x00000004 //Enables the encoding of packet header in the tmdsch0 bit[0] with cipher[2] instead of the tmdsch0 bit[2] Note: This bit must always be set to 1 for all PHYs (PHY GEN1, PHY GEN2, and non-Synopsys PHY)
#define A_HDCPCFG1_DISSHA1CHECK_MASK  0x00000008 //Disables the request to the API processor to verify the SHA1 message digest of a received KSV List
#define A_HDCPCFG1_HDCP_LOCK_MASK  0x00000010 //Lock the HDCP bypass and encryption disable mechanisms: - 1'b0: The default 1'b0 value enables you to bypass HDCP through bit 5 (bypencryption) of the A_HDCPCFG0 register or to disable the encryption through bit 1 (encryptiondisable) of A_HDCPCFG1
#define A_HDCPCFG1_SPARE_MASK  0x000000E0 //This is a spare register with no associated functionality

//HDCP Observation Register 0
#define A_HDCPOBS0  0x00014008
#define A_HDCPOBS0_HDCPENGAGED_MASK  0x00000001 //Informs that the current HDMI link has the HDCP protocol fully engaged
#define A_HDCPOBS0_SUBSTATEA_MASK  0x0000000E //Observability register informs in which sub-state the authentication is on
#define A_HDCPOBS0_STATEA_MASK  0x000000F0 //Observability register informs in which state the authentication machine is on

//HDCP Observation Register 1
#define A_HDCPOBS1  0x0001400C
#define A_HDCPOBS1_STATER_MASK  0x0000000F //Observability register informs in which state the revocation machine is on
#define A_HDCPOBS1_STATEOEG_MASK  0x00000070 //Observability register informs in which state the OESS machine is on

//HDCP Observation Register 2
#define A_HDCPOBS2  0x00014010
#define A_HDCPOBS2_STATEEEG_MASK  0x00000007 //Observability register informs in which state the EESS machine is on
#define A_HDCPOBS2_STATEE_MASK  0x00000038 //Observability register informs in which state the cipher machine is on

//HDCP Observation Register 3
#define A_HDCPOBS3  0x00014014
#define A_HDCPOBS3_FAST_REAUTHENTICATION_MASK  0x00000001 //Register read from attached sink device: Bcap(0x40) bit 0
#define A_HDCPOBS3_FEATURES_1_1_MASK  0x00000002 //Register read from attached sink device: Bcap(0x40) bit 1
#define A_HDCPOBS3_HDMI_MODE_MASK  0x00000004 //Register read from attached sink device: Bstatus(0x41) bit 12
#define A_HDCPOBS3_FAST_I2C_MASK  0x00000010 //Register read from attached sink device: Bcap(0x40) bit 4
#define A_HDCPOBS3_KSV_FIFO_READY_MASK  0x00000020 //Register read from attached sink device: Bcap(0x40) bit 5
#define A_HDCPOBS3_REPEATER_MASK  0x00000040 //Register read from attached sink device: Bcap(0x40) bit 6

//HDCP Interrupt Clear Register Write only register, active high and auto cleared, cleans the respective interruption in the interrupt status register
#define A_APIINTCLR  0x00014018
#define A_APIINTCLR_KSVACCESSINT_MASK  0x00000001 //Clears the interruption related to KSV memory access grant for Read-Write access
#define A_APIINTCLR_KSVSHA1CALCINT_MASK  0x00000002 //Clears the interruption related to KSV list update in memory that needs to be SHA1 verified
#define A_APIINTCLR_KEEPOUTERRORINT_MASK  0x00000004 //Clears the interruption related to keep out window error
#define A_APIINTCLR_LOSTARBITRATION_MASK  0x00000008 //Clears the interruption related to I2C arbitration lost
#define A_APIINTCLR_I2CNACK_MASK  0x00000010 //Clears the interruption related to I2C NACK reception
#define A_APIINTCLR_HDCP_FAILED_MASK  0x00000040 //Clears the interruption related to HDCP authentication process failed
#define A_APIINTCLR_HDCP_ENGAGED_MASK  0x00000080 //Clears the interruption related to HDCP authentication process successful

//HDCP Interrupt Status Register Read only register, reports the interruption which caused the activation of the interruption output pin
#define A_APIINTSTAT  0x0001401C
#define A_APIINTSTAT_KSVACCESSINT_MASK  0x00000001 //Notifies that the KSV memory access as been guaranteed for Read-Write access
#define A_APIINTSTAT_KSVSHA1CALCINT_MASK  0x00000002 //Notifies that the HDCP13TCTRL core as updated a KSV list in memory that needs to be SHA1 verified
#define A_APIINTSTAT_KEEPOUTERRORINT_MASK  0x00000004 //Notifies that during the keep out window, the ctlout[3:0] bus was used besides control period
#define A_APIINTSTAT_LOSTARBITRATION_MASK  0x00000008 //Notifies that the I2C lost the arbitration to communicate
#define A_APIINTSTAT_I2CNACK_MASK  0x00000010 //Notifies that the I2C received a NACK from slave device
#define A_APIINTSTAT_KSVSHA1CALCDONEINT_MASK  0x00000002
#define A_APIINTSTAT_HDCP_FAILED_MASK  0x00000040 //Notifies that the HDCP authentication process was failed
#define A_APIINTSTAT_HDCP_ENGAGED_MASK  0x00000080 //Notifies that the HDCP authentication process was successful


//HDCP Interrupt Mask Register The configuration of this register mask a given setup of interruption, disabling them from generating interruption pulses in the interruption output pin
#define A_APIINTMSK  0x00014020
#define A_APIINTMSK_KSVACCESSINT_MASK  0x00000001 //Masks the interruption related to KSV memory access grant for Read-Write access
#define A_APIINTMSK_KSVSHA1CALCINT_MASK  0x00000002 //Masks the interruption related to KSV list update in memory that needs to be SHA1 verified
#define A_APIINTMSK_KEEPOUTERRORINT_MASK  0x00000004 //Masks the interruption related to keep out window error
#define A_APIINTMSK_LOSTARBITRATION_MASK  0x00000008 //Masks the interruption related to I2C arbitration lost
#define A_APIINTMSK_I2CNACK_MASK  0x00000010 //Masks the interruption related to I2C NACK reception
#define A_APIINTMSK_SPARE_MASK  0x00000020 //This is a spare bit and has no associated functionality
#define A_APIINTMSK_HDCP_FAILED_MASK  0x00000040 //Masks the interruption related to HDCP authentication process failed
#define A_APIINTMSK_HDCP_ENGAGED_MASK  0x00000080 //Masks the interruption related to HDCP authentication process successful

//HDCP Video Polarity Configuration Register
#define A_VIDPOLCFG  0x00014024
#define A_VIDPOLCFG_SPARE_1_MASK  0x00000001 //This is a spare bit and has no associated functionality
#define A_VIDPOLCFG_HSYNCPOL_MASK  0x00000002 //Configuration of the video Horizontal synchronism polarity
#define A_VIDPOLCFG_SPARE_2_MASK  0x00000004 //This is a spare bit and has no associated functionality
#define A_VIDPOLCFG_VSYNCPOL_MASK  0x00000008 //Configuration of the video Vertical synchronism polarity
#define A_VIDPOLCFG_DATAENPOL_MASK  0x00000010 //Configuration of the video data enable polarity
#define A_VIDPOLCFG_UNENCRYPTCONF_MASK  0x00000060 //Configuration of the color sent when sending unencrypted video data For a complete table showing the color results (RGB), refer to the "Color Configuration When Sending Unencrypted Video Data" figure in Chapter 2, "Functional Description

//HDCP OESS WOO Configuration Register Pulse width of the encryption enable (CTL3) signal in the HDCP OESS mode
#define A_OESSWCFG  0x00014028
#define A_OESSWCFG_A_OESSWCFG_MASK  0x000000FF //HDCP OESS WOO Configuration Register

//HDCP Core Version Register LSB Design ID number
#define A_COREVERLSB  0x00014050
#define A_COREVERLSB_A_COREVERLSB_MASK  0x000000FF //HDCP Core Version Register LSB

//HDCP Core Version Register MSB Revision ID number
#define A_COREVERMSB  0x00014054
#define A_COREVERMSB_A_COREVERMSB_MASK  0x000000FF //HDCP Core Version Register MSB

//HDCP KSV Memory Control Register The KSVCTRLupd bit is a notification flag
#define A_KSVMEMCTRL  0x00014058
#define A_KSVMEMCTRL_KSVMEMREQUEST_MASK  0x00000001 //Request access to the KSV memory; must be de-asserted after the access is completed by the system
#define A_KSVMEMCTRL_KSVMEMACCESS_MASK  0x00000002 //Notification that the KSV memory access as been guaranteed
#define A_KSVMEMCTRL_KSVCTRLUPD_MASK  0x00000004 //Set to inform that the KSV list in memory has been analyzed and the response to the Message Digest as been updated
#define A_KSVMEMCTRL_SHA1FAIL_MASK  0x00000008 //Notification whether the KSV list message digest is correct
#define A_KSVMEMCTRL_KSVSHA1STATUS_MASK  0x00000010

//HDCP BStatus Register Array
#define HDCP_BSTATUS  0x00014080
#define HDCP_BSTATUS_SIZE  2

//HDCP M0 Register Array
#define HDCP_M0  0x00014088
#define HDCP_M0_SIZE  8

//HDCP KSV Registers
#define HDCP_KSV  0x000140A8
#define HDCP_KSV_SIZE  635

//HDCP SHA-1 VH Registers
#define HDCP_VH  0x00014A94
#define HDCP_VH_SIZE  20

//HDCP Revocation KSV List Size Register 0
#define HDCP_REVOC_SIZE_0  0x00014AE4
#define HDCP_REVOC_SIZE_0_HDCP_REVOC_SIZE_0_MASK  0x000000FF //Register containing the LSB of KSV list size (ksv_list_size[7:0])

//HDCP Revocation KSV List Size Register 1
#define HDCP_REVOC_SIZE_1  0x00014AE8
#define HDCP_REVOC_SIZE_1_HDCP_REVOC_SIZE_1_MASK  0x000000FF //Register containing the MSB of KSV list size (ksv_list_size[15:8])

//HDCP Revocation KSV Registers
#define HDCP_REVOC_LIST  0x00014AEC
#define HDCP_REVOC_LIST_SIZE  5060

//HDCP KSV Status Register 0
#define HDCPREG_BKSV0  0x0001E000
#define HDCPREG_BKSV0_HDCPREG_BKSV0_MASK  0x000000FF //Contains the value of BKSV[7:0]

//HDCP KSV Status Register 1
#define HDCPREG_BKSV1  0x0001E004
#define HDCPREG_BKSV1_HDCPREG_BKSV1_MASK  0x000000FF //Contains the value of BKSV[15:8]

//HDCP KSV Status Register 2
#define HDCPREG_BKSV2  0x0001E008
#define HDCPREG_BKSV2_HDCPREG_BKSV2_MASK  0x000000FF //Contains the value of BKSV[23:16]

//HDCP KSV Status Register 3
#define HDCPREG_BKSV3  0x0001E00C
#define HDCPREG_BKSV3_HDCPREG_BKSV3_MASK  0x000000FF //Contains the value of BKSV[31:24]

//HDCP KSV Status Register 4
#define HDCPREG_BKSV4  0x0001E010
#define HDCPREG_BKSV4_HDCPREG_BKSV4_MASK  0x000000FF //Contains the value of BKSV[39:32]

//HDCP AN Bypass Control Register
#define HDCPREG_ANCONF  0x0001E014
#define HDCPREG_ANCONF_OANBYPASS_MASK  0x00000001 //- When oanbypass=1, the value of AN used in the HDCP engine comes from the hdcpreg_an0 to hdcpreg_an7 registers

//HDCP Forced AN Register 0
#define HDCPREG_AN0  0x0001E018
#define HDCPREG_AN0_HDCPREG_AN0_MASK  0x000000FF //Contains the value of AN[7:0]

//HDCP Forced AN Register 1
#define HDCPREG_AN1  0x0001E01C
#define HDCPREG_AN1_HDCPREG_AN1_MASK  0x000000FF //Contains the value of AN[15:8]

//HDCP forced AN Register 2
#define HDCPREG_AN2  0x0001E020
#define HDCPREG_AN2_HDCPREG_AN2_MASK  0x000000FF //Contains the value of AN[23:16]

//HDCP Forced AN Register 3
#define HDCPREG_AN3  0x0001E024
#define HDCPREG_AN3_HDCPREG_AN3_MASK  0x000000FF //Contains the value of AN[31:24]

//HDCP Forced AN Register 4
#define HDCPREG_AN4  0x0001E028
#define HDCPREG_AN4_HDCPREG_AN4_MASK  0x000000FF //Contains the value of AN[39:32]

//HDCP Forced AN Register 5
#define HDCPREG_AN5  0x0001E02C
#define HDCPREG_AN5_HDCPREG_AN5_MASK  0x000000FF //Contains the value of AN[47:40]

//HDCP Forced AN Register 6
#define HDCPREG_AN6  0x0001E030
#define HDCPREG_AN6_HDCPREG_AN6_MASK  0x000000FF //Contains the value of AN[55:48]

//HDCP Forced AN Register 7
#define HDCPREG_AN7  0x0001E034
#define HDCPREG_AN7_HDCPREG_AN7_MASK  0x000000FF //Contains the value of BKSV[63:56]

#define HDCPREG_RMLCTL    0x0001E038
#define HDCPREG_RMLSTS    0x0001E03C

#define HDCPREG_SEED0    0x0001E040
#define HDCPREG_SEED1    0x0001E044
#define HDCPREG_DPK0     0x0001E048
#define HDCPREG_DPK1     0x0001E04C
#define HDCPREG_DPK2     0x0001E050
#define HDCPREG_DPK3     0x0001E054
#define HDCPREG_DPK4     0x0001E058
#define HDCPREG_DPK5     0x0001E05C
#define HDCPREG_DPK6     0x0001E060

// HDCP register control
#define HDCP22REG_CTRL 0x0001e410
#define HDCP22REG_CTRL_OVR_EN_MASK 2

//I2C DDC Slave address Configuration Register
#define I2CM_SLAVE  0x0001F800
#define I2CM_SLAVE_SLAVEADDR_MASK  0x0000007F //Slave address to be sent during read and write normal operations

//I2C DDC Address Configuration Register
#define I2CM_ADDRESS  0x0001F804
#define I2CM_ADDRESS_ADDRESS_MASK  0x000000FF //Register address for read and write operations

//I2C DDC Data Write Register
#define I2CM_DATAO  0x0001F808
#define I2CM_DATAO_DATAO_MASK  0x000000FF //Data to be written on register pointed by address[7:0]

//I2C DDC Data read Register
#define I2CM_DATAI  0x0001F80C
#define I2CM_DATAI_DATAI_MASK  0x000000FF //Data read from register pointed by address[7:0]

//I2C DDC RD/RD_EXT/WR Operation Register Read and write operation request
#define I2CM_OPERATION  0x0001F810
#define I2CM_OPERATION_RD_MASK  0x00000001 //Single byte read operation request
#define I2CM_OPERATION_RD_EXT_MASK  0x00000002 //After writing 1'b1 to rd_ext bit a extended data read operation is started (E-DDC read operation)
#define I2CM_OPERATION_RD8_MASK  0x00000004 //Sequential read operation request
#define I2CM_OPERATION_RD8_EXT_MASK  0x00000008 //Extended sequential read operation request
#define I2CM_OPERATION_WR_MASK  0x00000010 //Single byte write operation request
#define I2CM_OPERATION_BUSCLEAR_MASK  0x00000020 //Single byte write operation request

//I2C DDC Done Interrupt Register This register configures the I2C master interrupts
#define I2CM_INT  0x0001F814
#define I2CM_INT_DONE_MASK  0x00000004 //Done interrupt mask signal
#define I2CM_INT_READ_REQ_MASK  0x00000040 //Read request interruption mask signal

//I2C DDC error Interrupt Register This register configures the I2C master arbitration lost and not acknowledge error interrupts
#define I2CM_CTLINT  0x0001F818
#define I2CM_CTLINT_ARBITRATION_MASK  0x00000004 //Arbitration error interrupt mask signal
#define I2CM_CTLINT_NACK_MASK  0x00000040 //Not acknowledge error interrupt mask signal

//I2C DDC Speed Control Register This register configures the division relation between master and scl clock
#define I2CM_DIV  0x0001F81C
#define I2CM_DIV_SPARE_MASK  0x00000007 //This bit is a spare register with no associated functionality
#define I2CM_DIV_FAST_STD_MODE_MASK  0x00000008 //Sets the I2C Master to work in Fast Mode or Standard Mode: 1: Fast Mode 0: Standard Mode

//I2C DDC Segment Address Configuration Register This register configures the segment address for extended R/W destination and is used for EDID reading operations, particularly for the Extended Data Read Operation for Enhanced DDC
#define I2CM_SEGADDR  0x0001F820
#define I2CM_SEGADDR_SEG_ADDR_MASK  0x0000007F //I2C DDC Segment Address Configuration Register

//I2C DDC Software Reset Control Register This register resets the I2C master
#define I2CM_SOFTRSTZ  0x0001F824
#define I2CM_SOFTRSTZ_I2C_SOFTRSTZ_MASK  0x00000001 //I2C Master Software Reset

//I2C DDC Segment Pointer Register This register configures the segment pointer for extended RD/WR request
#define I2CM_SEGPTR  0x0001F828
#define I2CM_SEGPTR_SEGPTR_MASK  0x000000FF //I2C DDC Segment Pointer Register

//I2C DDC Slow Speed SCL High Level Control Register 1
#define I2CM_SS_SCL_HCNT_1_ADDR  0x0001F82C
#define I2CM_SS_SCL_HCNT_1_ADDR_I2CMP_SS_SCL_HCNT1_MASK  0x000000FF //I2C DDC Slow Speed SCL High Level Control Register 1

//I2C DDC Slow Speed SCL High Level Control Register 0
#define I2CM_SS_SCL_HCNT_0_ADDR  0x0001F830
#define I2CM_SS_SCL_HCNT_0_ADDR_I2CMP_SS_SCL_HCNT0_MASK  0x000000FF //I2C DDC Slow Speed SCL High Level Control Register 0

//I2C DDC Slow Speed SCL Low Level Control Register 1
#define I2CM_SS_SCL_LCNT_1_ADDR  0x0001F834
#define I2CM_SS_SCL_LCNT_1_ADDR_I2CMP_SS_SCL_LCNT1_MASK  0x000000FF //I2C DDC Slow Speed SCL Low Level Control Register 1

//I2C DDC Slow Speed SCL Low Level Control Register 0
#define I2CM_SS_SCL_LCNT_0_ADDR  0x0001F838
#define I2CM_SS_SCL_LCNT_0_ADDR_I2CMP_SS_SCL_LCNT0_MASK  0x000000FF //I2C DDC Slow Speed SCL Low Level Control Register 0

//I2C DDC Fast Speed SCL High Level Control Register 1
#define I2CM_FS_SCL_HCNT_1_ADDR  0x0001F83C
#define I2CM_FS_SCL_HCNT_1_ADDR_I2CMP_FS_SCL_HCNT1_MASK  0x000000FF //I2C DDC Fast Speed SCL High Level Control Register 1

//I2C DDC Fast Speed SCL High Level Control Register 0
#define I2CM_FS_SCL_HCNT_0_ADDR  0x0001F840
#define I2CM_FS_SCL_HCNT_0_ADDR_I2CMP_FS_SCL_HCNT0_MASK  0x000000FF //I2C DDC Fast Speed SCL High Level Control Register 0

//I2C DDC Fast Speed SCL Low Level Control Register 1
#define I2CM_FS_SCL_LCNT_1_ADDR  0x0001F844
#define I2CM_FS_SCL_LCNT_1_ADDR_I2CMP_FS_SCL_LCNT1_MASK  0x000000FF //I2C DDC Fast Speed SCL Low Level Control Register 1

//I2C DDC Fast Speed SCL Low Level Control Register 0
#define I2CM_FS_SCL_LCNT_0_ADDR  0x0001F848
#define I2CM_FS_SCL_LCNT_0_ADDR_I2CMP_FS_SCL_LCNT0_MASK  0x000000FF //I2C DDC Fast Speed SCL Low Level Control Register 0

//I2C DDC SDA Hold Register
#define I2CM_SDA_HOLD  0x0001F84C
#define I2CM_SDA_HOLD_OSDA_HOLD_MASK  0x000000FF //Defines the number of SFR clock cycles to meet tHD;DAT (300 ns) osda_hold = round_to_high_integer (300 ns / (1 / isfrclk_frequency))

//SCDC Control Register This register configures the SCDC update status read through the I2C master interface
#define I2CM_SCDC_READ_UPDATE  0x0001F850
#define I2CM_SCDC_READ_UPDATE_READ_UPDATE_MASK  0x00000001 //When set to 1'b1, a SCDC Update Read is performed and the read data loaded into registers i2cm_scdc_update0 and i2cm_scdc_update1
#define I2CM_SCDC_READ_UPDATE_READ_REQUEST_EN_MASK  0x00000010 //Read request enabled
#define I2CM_SCDC_READ_UPDATE_UPDTRD_VSYNCPOLL_EN_MASK  0x00000020 //Update read polling enabled

//I2C Master Sequential Read Buffer Register 0
#define I2CM_READ_BUFF0  0x0001F880
#define I2CM_READ_BUFF0_I2CM_READ_BUFF0_MASK  0x000000FF //Byte 0 of a I2C read buffer sequential read (from address i2cm_address)

//I2C Master Sequential Read Buffer Register 1
#define I2CM_READ_BUFF1  0x0001F884
#define I2CM_READ_BUFF1_I2CM_READ_BUFF1_MASK  0x000000FF //Byte 1 of a I2C read buffer sequential read (from address i2cm_address+1)

//I2C Master Sequential Read Buffer Register 2
#define I2CM_READ_BUFF2  0x0001F888
#define I2CM_READ_BUFF2_I2CM_READ_BUFF2_MASK  0x000000FF //Byte 2 of a I2C read buffer sequential read (from address i2cm_address+2)

//I2C Master Sequential Read Buffer Register 3
#define I2CM_READ_BUFF3  0x0001F88C
#define I2CM_READ_BUFF3_I2CM_READ_BUFF3_MASK  0x000000FF //Byte 3 of a I2C read buffer sequential read (from address i2cm_address+3)

//I2C Master Sequential Read Buffer Register 4
#define I2CM_READ_BUFF4  0x0001F890
#define I2CM_READ_BUFF4_I2CM_READ_BUFF4_MASK  0x000000FF //Byte 4 of a I2C read buffer sequential read (from address i2cm_address+4)

//I2C Master Sequential Read Buffer Register 5
#define I2CM_READ_BUFF5  0x0001F894
#define I2CM_READ_BUFF5_I2CM_READ_BUFF5_MASK  0x000000FF //Byte 5 of a I2C read buffer sequential read (from address i2cm_address+5)

//I2C Master Sequential Read Buffer Register 6
#define I2CM_READ_BUFF6  0x0001F898
#define I2CM_READ_BUFF6_I2CM_READ_BUFF6_MASK  0x000000FF //Byte 6 of a I2C read buffer sequential read (from address i2cm_address+6)

//I2C Master Sequential Read Buffer Register 7
#define I2CM_READ_BUFF7  0x0001F89C
#define I2CM_READ_BUFF7_I2CM_READ_BUFF7_MASK  0x000000FF //Byte 7 of a I2C read buffer sequential read (from address i2cm_address+7)

//I2C SCDC Read Update Register 0
#define I2CM_SCDC_UPDATE0  0x0001F8C0
#define I2CM_SCDC_UPDATE0_I2CM_SCDC_UPDATE0_MASK  0x000000FF //Byte 0 of a SCDC I2C update sequential read

//I2C SCDC Read Update Register 1
#define I2CM_SCDC_UPDATE1  0x0001F8C4
#define I2CM_SCDC_UPDATE1_I2CM_SCDC_UPDATE1_MASK  0x000000FF //Byte 1 of a SCDC I2C update sequential read

/*****************************************************************************
 *                                                                           *
 *                      Color Space Converter Registers                      *
 *                                                                           *
 *****************************************************************************/

//Color Space Converter Interpolation and Decimation Configuration Register
#define CSC_CFG  0x00010400
#define CSC_CFG_DECMODE_MASK  0x00000003 //Chroma decimation configuration: decmode[1:0] | Chroma Decimation 00 | decimation disabled 01 | Hd (z) =1 10 | Hd(z)=1/ 4 + 1/2z^(-1 )+1/4 z^(-2) 11 | Hd(z)x2^(11)= -5+12z^(-2) - 22z^(-4)+39z^(-8) +109z^(-10) -204z^(-12)+648z^(-14) + 1024z^(-15) +648z^(-16) -204z^(-18) +109z^(-20)- 65z^(-22) +39z^(-24) -22z^(-26) +12z^(-28)-5z^(-30)
#define CSC_CFG_SPARE_1_MASK  0x0000000C //This is a spare register with no associated functionality
#define CSC_CFG_INTMODE_MASK  0x00000030 //Chroma interpolation configuration: intmode[1:0] | Chroma Interpolation 00 | interpolation disabled 01 | Hu (z) =1 + z^(-1) 10 | Hu(z)=1/ 2 + z^(-11)+1/2 z^(-2) 11 | interpolation disabled
#define CSC_CFG_SPARE_2_MASK  0x00000040 //This is a spare register with no associated functionality
#define CSC_CFG_CSC_LIMIT_MASK  0x00000080 //When set (1'b1), the range limitation values defined in registers csc_mat_uplim and csc_mat_dnlim are applied to the output of the Color Space Conversion matrix

//Color Space Converter Scale and Deep Color Configuration Register
#define CSC_SCALE  0x00010404
#define CSC_SCALE_CSCSCALE_MASK  0x00000003 //Defines the cscscale[1:0] scale factor to apply to all coefficients in Color Space Conversion
#define CSC_SCALE_SPARE_MASK  0x0000000C //The is a spare register with no associated functionality
#define CSC_SCALE_CSC_COLOR_DEPTH_MASK  0x000000F0 //Color space converter color depth configuration: csc_colordepth[3:0] | Action 0000 | 24 bit per pixel video (8 bit per component)

//Color Space Converter Matrix A1 Coefficient Register MSB Notes: - The coefficients used in the CSC matrix use only 15 bits for the internal computations
#define CSC_COEF_A1_MSB  0x00010408
#define CSC_COEF_A1_MSB_CSC_COEF_A1_MSB_MASK  0x000000FF //Color Space Converter Matrix A1 Coefficient Register MSB

//Color Space Converter Matrix A1 Coefficient Register LSB Notes: - The coefficients used in the CSC matrix use only 15 bits for the internal computations
#define CSC_COEF_A1_LSB  0x0001040C
#define CSC_COEF_A1_LSB_CSC_COEF_A1_LSB_MASK  0x000000FF //Color Space Converter Matrix A1 Coefficient Register LSB

//Color Space Converter Matrix A2 Coefficient Register MSB Color Space Conversion A2 coefficient
#define CSC_COEF_A2_MSB  0x00010410
#define CSC_COEF_A2_MSB_CSC_COEF_A2_MSB_MASK  0x000000FF //Color Space Converter Matrix A2 Coefficient Register MSB

//Color Space Converter Matrix A2 Coefficient Register LSB Color Space Conversion A2 coefficient
#define CSC_COEF_A2_LSB  0x00010414
#define CSC_COEF_A2_LSB_CSC_COEF_A2_LSB_MASK  0x000000FF //Color Space Converter Matrix A2 Coefficient Register LSB

//Color Space Converter Matrix A3 Coefficient Register MSB Color Space Conversion A3 coefficient
#define CSC_COEF_A3_MSB  0x00010418
#define CSC_COEF_A3_MSB_CSC_COEF_A3_MSB_MASK  0x000000FF //Color Space Converter Matrix A3 Coefficient Register MSB

//Color Space Converter Matrix A3 Coefficient Register LSB Color Space Conversion A3 coefficient
#define CSC_COEF_A3_LSB  0x0001041C
#define CSC_COEF_A3_LSB_CSC_COEF_A3_LSB_MASK  0x000000FF //Color Space Converter Matrix A3 Coefficient Register LSB

//Color Space Converter Matrix A4 Coefficient Register MSB Color Space Conversion A4 coefficient
#define CSC_COEF_A4_MSB  0x00010420
#define CSC_COEF_A4_MSB_CSC_COEF_A4_MSB_MASK  0x000000FF //Color Space Converter Matrix A4 Coefficient Register MSB

//Color Space Converter Matrix A4 Coefficient Register LSB Color Space Conversion A4 coefficient
#define CSC_COEF_A4_LSB  0x00010424
#define CSC_COEF_A4_LSB_CSC_COEF_A4_LSB_MASK  0x000000FF //Color Space Converter Matrix A4 Coefficient Register LSB

//Color Space Converter Matrix B1 Coefficient Register MSB Color Space Conversion B1 coefficient
#define CSC_COEF_B1_MSB  0x00010428
#define CSC_COEF_B1_MSB_CSC_COEF_B1_MSB_MASK  0x000000FF //Color Space Converter Matrix B1 Coefficient Register MSB

//Color Space Converter Matrix B1 Coefficient Register LSB Color Space Conversion B1 coefficient
#define CSC_COEF_B1_LSB  0x0001042C
#define CSC_COEF_B1_LSB_CSC_COEF_B1_LSB_MASK  0x000000FF //Color Space Converter Matrix B1 Coefficient Register LSB

//Color Space Converter Matrix B2 Coefficient Register MSB Color Space Conversion B2 coefficient
#define CSC_COEF_B2_MSB  0x00010430
#define CSC_COEF_B2_MSB_CSC_COEF_B2_MSB_MASK  0x000000FF //Color Space Converter Matrix B2 Coefficient Register MSB

//Color Space Converter Matrix B2 Coefficient Register LSB Color Space Conversion B2 coefficient
#define CSC_COEF_B2_LSB  0x00010434
#define CSC_COEF_B2_LSB_CSC_COEF_B2_LSB_MASK  0x000000FF //Color Space Converter Matrix B2 Coefficient Register LSB

//Color Space Converter Matrix B3 Coefficient Register MSB Color Space Conversion B3 coefficient
#define CSC_COEF_B3_MSB  0x00010438
#define CSC_COEF_B3_MSB_CSC_COEF_B3_MSB_MASK  0x000000FF //Color Space Converter Matrix B3 Coefficient Register MSB

//Color Space Converter Matrix B3 Coefficient Register LSB Color Space Conversion B3 coefficient
#define CSC_COEF_B3_LSB  0x0001043C
#define CSC_COEF_B3_LSB_CSC_COEF_B3_LSB_MASK  0x000000FF //Color Space Converter Matrix B3 Coefficient Register LSB

//Color Space Converter Matrix B4 Coefficient Register MSB Color Space Conversion B4 coefficient
#define CSC_COEF_B4_MSB  0x00010440
#define CSC_COEF_B4_MSB_CSC_COEF_B4_MSB_MASK  0x000000FF //Color Space Converter Matrix B4 Coefficient Register MSB

//Color Space Converter Matrix B4 Coefficient Register LSB Color Space Conversion B4 coefficient
#define CSC_COEF_B4_LSB  0x00010444
#define CSC_COEF_B4_LSB_CSC_COEF_B4_LSB_MASK  0x000000FF //Color Space Converter Matrix B4 Coefficient Register LSB

//Color Space Converter Matrix C1 Coefficient Register MSB Color Space Conversion C1 coefficient
#define CSC_COEF_C1_MSB  0x00010448
#define CSC_COEF_C1_MSB_CSC_COEF_C1_MSB_MASK  0x000000FF //Color Space Converter Matrix C1 Coefficient Register MSB

//Color Space Converter Matrix C1 Coefficient Register LSB Color Space Conversion C1 coefficient
#define CSC_COEF_C1_LSB  0x0001044C
#define CSC_COEF_C1_LSB_CSC_COEF_C1_LSB_MASK  0x000000FF //Color Space Converter Matrix C1 Coefficient Register LSB

//Color Space Converter Matrix C2 Coefficient Register MSB Color Space Conversion C2 coefficient
#define CSC_COEF_C2_MSB  0x00010450
#define CSC_COEF_C2_MSB_CSC_COEF_C2_MSB_MASK  0x000000FF //Color Space Converter Matrix C2 Coefficient Register MSB

//Color Space Converter Matrix C2 Coefficient Register LSB Color Space Conversion C2 coefficient
#define CSC_COEF_C2_LSB  0x00010454
#define CSC_COEF_C2_LSB_CSC_COEF_C2_LSB_MASK  0x000000FF //Color Space Converter Matrix C2 Coefficient Register LSB

//Color Space Converter Matrix C3 Coefficient Register MSB Color Space Conversion C3 coefficient
#define CSC_COEF_C3_MSB  0x00010458
#define CSC_COEF_C3_MSB_CSC_COEF_C3_MSB_MASK  0x000000FF //Color Space Converter Matrix C3 Coefficient Register MSB

//Color Space Converter Matrix C3 Coefficient Register LSB Color Space Conversion C3 coefficient
#define CSC_COEF_C3_LSB  0x0001045C
#define CSC_COEF_C3_LSB_CSC_COEF_C3_LSB_MASK  0x000000FF //Color Space Converter Matrix C3 Coefficient Register LSB

//Color Space Converter Matrix C4 Coefficient Register MSB Color Space Conversion C4 coefficient
#define CSC_COEF_C4_MSB  0x00010460
#define CSC_COEF_C4_MSB_CSC_COEF_C4_MSB_MASK  0x000000FF //Color Space Converter Matrix C4 Coefficient Register MSB

//Color Space Converter Matrix C4 Coefficient Register LSB Color Space Conversion C4 coefficient
#define CSC_COEF_C4_LSB  0x00010464
#define CSC_COEF_C4_LSB_CSC_COEF_C4_LSB_MASK  0x000000FF //Color Space Converter Matrix C4 Coefficient Register LSB

//Color Space Converter Matrix Output Up Limit Register MSB For more details, refer to the HDMI 1
#define CSC_LIMIT_UP_MSB  0x00010468
#define CSC_LIMIT_UP_MSB_CSC_LIMIT_UP_MSB_MASK  0x000000FF //Color Space Converter Matrix Output Upper Limit Register MSB

//Color Space Converter Matrix output Up Limit Register LSB For more details, refer to the HDMI 1
#define CSC_LIMIT_UP_LSB  0x0001046C
#define CSC_LIMIT_UP_LSB_CSC_LIMIT_UP_LSB_MASK  0x000000FF //Color Space Converter Matrix Output Upper Limit Register LSB

//Color Space Converter Matrix output Down Limit Register MSB For more details, refer to the HDMI 1
#define CSC_LIMIT_DN_MSB  0x00010470
#define CSC_LIMIT_DN_MSB_CSC_LIMIT_DN_MSB_MASK  0x000000FF //Color Space Converter Matrix output Down Limit Register MSB

//Color Space Converter Matrix output Down Limit Register LSB For more details, refer to the HDMI 1
#define CSC_LIMIT_DN_LSB  0x00010474
#define CSC_LIMIT_DN_LSB_CSC_LIMIT_DN_LSB_MASK  0x000000FF //Color Space Converter Matrix Output Down Limit Register LSB


#endif