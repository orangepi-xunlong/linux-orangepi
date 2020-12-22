/*
 * sound\soc\sunxi\audiocodec\sun8iw5-sndcodec.h
 * (C) Copyright 2010-2016
 * Reuuimlla Technology Co., Ltd. <www.reuuimllatech.com>
 * huangxin <huangxin@reuuimllatech.com>
 *
 * some simple description for this code
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 */
#ifndef _SUN8IW5_CODEC_H
#define _SUN8IW5_CODEC_H
#include <mach/platform.h>
/*Codec Register*/
#define baseaddr                               SUNXI_AUDIO_VBASE
#define CODEC_BASSADDRESS         (0x01c22c00)
#define SUNXI_DA_CTL                			(0x00)
#define SUNXI_DA_FAT0                  			(0x04)
#define SUNXI_DA_FAT1               			(0x08)
#define SUNXI_DA_TXFIFO                         (0x0c)
#define SUNXI_DA_RXFIFO                         (0x10)
#define SUNXI_DA_FCTL                          (0x14)
#define SUNXI_DA_FSTA                          (0x18)
#define SUNXI_DA_INT                           (0x1c)
#define SUNXI_DA_ISTA                          (0x20)
#define SUNXI_DA_CLKD                          (0x24)
#define SUNXI_DA_TXCNT                         (0x28)
#define SUNXI_DA_RXCNT                         (0x2c)
#define SUNXI_DA_TXCHSEL                       (0x30)
#define SUNXI_DA_TXCHMAP                       (0x34)
#define SUNXI_DA_RXCHSEL                       (0x38)
#define SUNXI_DA_RXCHMAP                       (0x3c)
#define SUNXI_CHIP_AUDIO_RST           (0x200)
#define SUNXI_SYSCLK_CTL                       (0x20c)
#define SUNXI_MOD_CLK_ENA                      (0x210)
#define SUNXI_MOD_RST_CTL                      (0x214)
#define SUNXI_SYS_SR_CTRL                      (0x218)
#define SUNXI_SYS_SRC_CLK                      (0x21c)
#define SUNXI_AIF1CLK_CTRL                     (0x240)
#define SUNXI_AIF1_ADCDAT_CTRL         (0x244)
#define SUNXI_AIF1_DACDAT_CTRL         (0x248)
#define SUNXI_AIF1_MXR_SRC                     (0x24c)
#define SUNXI_AIF1_VOL_CTRL1           (0x250)
#define SUNXI_AIF1_VOL_CTRL2           (0x254)
#define SUNXI_AIF1_VOL_CTRL3           (0x258)
#define SUNXI_AIF1_VOL_CTRL4           (0x25c)
#define SUNXI_AIF1_MXR_GAIN                    (0x260)
#define SUNXI_AIF1_RXD_CTRL                    (0x264)
#define SUNXI_AIF2_CLK_CTRL                    (0x280)
#define SUNXI_AIF2_ADCDAT_CTRL         (0x284)
#define SUNXI_AIF2_DACDAT_CTRL         (0x288)
#define SUNXI_AIF2_MXR_SRC                     (0x28c)
#define SUNXI_AIF2_VOL_CTRL1           (0x290)
#define SUNXI_AIF2_VOL_CTRL2           (0x298)
#define SUNXI_AIF2_MXR_GAIN                    (0x2a0)
#define SUNXI_AIF2_RXD_CTRL                    (0x2a4)
#define SUNXI_AIF3_CLK_CTRL                    (0x2c0)
#define SUNXI_AIF3_ADCDAT_CTRL         (0x2c4)
#define SUNXI_AIF3_DACDAT_CTRL         (0x2c8)
#define SUNXI_AIF3_SGP_CTRL                    (0x2cc)
#define SUNXI_AIF3_RXD_CTRL                    (0x2e4)
#define SUNXI_ADC_DIG_CTRL                     (0x300)
#define SUNXI_ADC_VOL_CTRL                     (0x304)
#define SUNXI_ADC_DBG_CTRL                     (0x308)
#define SUNXI_HMIC_CTRL1                       (0x30c)
#define SUNXI_HMIC_CTRL2                       (0x314)
#define SUNXI_HMIC_STS                         (0x318)
#define SUNXI_DAC_DIG_CTRL                     (0x320)
#define SUNXI_DAC_VOL_CTRL                     (0x324)
#define SUNXI_DAC_DBG_CTRL                     (0x32c)
#define SUNXI_DAC_MXR_SRC                      (0x330)
#define SUNXI_DAC_MXR_GAIN                     (0x334)
#define SUNXI_I2SAP_AIF1_DBG           (0x360)
#define SUNXI_AC_ADC_DAPLSTA           (0x400)
#define SUNXI_AC_ADC_DAPRSTA           (0x404)
#define SUNXI_AC_ADC_DAPLCTRL          (0x408)
#define SUNXI_AC_ADC_DAPRCTRL          (0x40c)
#define SUNXI_AC_ADC_DAPLTL                    (0x410)
#define SUNXI_AC_ADC_DAPRTL                    (0x414)
#define SUNXI_AC_ADC_DAPLHAC           (0x418)
#define SUNXI_AC_ADC_DAPLLAC           (0x41c)
#define SUNXI_AC_ADC_DAPRHAC           (0x420)
#define SUNXI_AC_ADC_DAPRLAC           (0x424)
#define SUNXI_AC_ADC_DAPLDT                    (0x428)
#define SUNXI_AC_ADC_DAPLAT                    (0x42c)
#define SUNXI_AC_ADC_DAPRDT                    (0x430)
#define SUNXI_AC_ADC_DAPRAT                    (0x434)
#define SUNXI_AC_ADC_DAPNTH                    (0x438)
#define SUNXI_AC_ADC_DAPLHNAC          (0x43c)
#define SUNXI_AC_ADC_DAPLLNAC          (0x440)
#define SUNXI_AC_ADC_DAPRHNAC          (0x444)
#define SUNXI_AC_ADC_DAPRLNAC          (0x448)
#define SUNXI_AC_DAPHHPFC                      (0x44c)
#define SUNXI_AC_DAPLHPFC                      (0x450)
#define SUNXI_AC_DAPOPT                                (0x454)
#define SUNXI_AC_DAC_DAPCTRL           (0x480)
#define SUNXI_AC_DAC_DAPHHPFC          (0x484)
#define SUNXI_AC_DAC_DAPLHPFC          (0x488)
#define SUNXI_AC_DAC_DAPLHAVC          (0x48c)
#define SUNXI_AC_DAC_DAPLLAVC          (0x490)
#define SUNXI_AC_DAC_DAPRHAVC          (0x494)
#define SUNXI_AC_DAC_DAPRLAVC          (0x498)
#define SUNXI_AC_DAC_DAPHGDEC          (0x49c)
#define SUNXI_AC_DAC_DAPLGDEC          (0x4a0)
#define SUNXI_AC_DAC_DAPHGATC          (0x4a4)
#define SUNXI_AC_DAC_DAPLGATC          (0x4a8)
#define SUNXI_AC_DAC_DAPHETHD          (0x4ac)
#define SUNXI_AC_DAC_DAPLETHD          (0x4b0)
#define SUNXI_AC_DAC_DAPHGKPA          (0x4b4)
#define SUNXI_AC_DAC_DAPLGKPA          (0x4b8)
#define SUNXI_AC_DAC_DAPHGOPA          (0x4bc)
#define SUNXI_AC_DAC_DAPLGOPA          (0x4c0)
#define SUNXI_AC_DAC_DAPOPT                    (0x4c4)
#define SUNXI_AGC_ENA                          (0x4d0)
#define SUNXI_DRC_ENA                          (0x4d4)
#define SUNXI_SRC_BISTCR                       (0x4d8)
#define SUNXI_SRC_BISTST                       (0x4dc)
#define SUNXI_SRC1_CTRL1                       (0x4e0)
#define SUNXI_SRC1_CTROL2                      (0x4e4)
#define SUNXI_SRC1_CTRL3                       (0x4e8)
#define SUNXI_SRC1_CTRL4                       (0x4ec)
#define SUNXI_SRC2_CTRL1                       (0x4f0)
#define SUNXI_SRC2_CTRL2                       (0x4f4)
#define SUNXI_SRC2_CTRL3                       (0x4f8)
#define SUNXI_SRC2_CTRL4                       (0x4fc)
#define SUNXI_ADDA_PR_CFG_REG          (0x600)

/*
*      SUNXI_DA_CTL
*      I2S_AP Control Reg
*      DA_CTL:codecbase+0x00
*/
#define SDO_EN                                 (8)
#define ASS                                            (6)
#define MS                                             (5)
#define PCM                                            (4)
#define LOOP                                   (3)
#define TXEN                                   (2)
#define RXEN                                   (1)
#define GEN                                            (0)

/*
*      SUNXI_DA_FAT0
*      I2S_AP Format Reg
*      DA_FAT0:codecbase+0x04
*/
#define LRCP                                   (7)
#define BCP                                            (6)
#define SR                                             (4)
#define WSS                                            (2)
#define FMT                                            (0)

/*
*      I2S_AP Format Reg1
*      DA_FAT1:codecbase+0x08
*/
#define PCM_SYNC_PERIOD                        (12)
#define PCM_SYNC_OUT                   (11)
#define PCM_OUT_MUTE                   (10)
#define MLS                                            (9)
#define SEXT                                   (8)
#define SI                                             (6)
#define SW                                             (5)
#define SSYNC                                  (4)
#define RX_PDM                                 (2)
#define TX_PDM                                 (0)

/*
*      I2S_AP TX FIFO  register
*      DA_TXFIFO:codecbase+0x0C
*/
#define TX_DATA                                        (0)

/*
*      I2S_AP RX FIFO  register
*      DA_RXFIFO:codecbase+0x10
*/
#define RX_DATA                                        (0)

/*
*      SUNXI_DA_FCTL
*      I2S_AP FIFO     Control register
*      DA_FCTL:codecbase+0x14
*/
#define FIFOSRC                                        (31)
#define FTX                                            (25)
#define FRX                                            (24)
#define TXTL                                   (12)
#define RXTL                                   (4)
#define TXIM                                   (2)
#define RXOM                                   (0)

/*
*      I2S_AP FIFO     Status register
*      DA_FSTA:codecbase+0x18
*/
#define TXE                                            (28)
#define TXE_CNT                                        (16)
#define RXA                                            (8)
#define RXA_CNT                                        (0)

/*
*      SUNXI_DA_INT
*      I2S_AP DMA & Interrupt Control register
*      DA_INT:codecbase+0x1c
*/
#define TX_DRQ                                 (7)
#define TXUI_EN                                        (6)
#define TXOI_EN                                        (5)
#define TXEI_EN                                        (4)
#define RX_DRQ                                 (3)
#define RXUI_EN                                        (2)
#define RXOI_EN                                        (1)
#define RXAI_EN                                        (0)

/*
*      I2S_AP Interrupt Status register
*      DA_ISTA:codecbase+0x20
*/
#define TXU_INT                                        (6)
#define TXO_INT                                        (5)
#define TXE_INT                                        (4)
#define RXU_INT                                        (2)
#define RXO_INT                                        (1)
#define RXA_INT                                        (0)

/*
*      SUNXI_DA_CLKD
*      I2S_AP Clock Divide register
*      DA_CLKD:codecbase+0x24
*/
#define MCLKO_EN                               (7)
#define BCLKDIV                                        (4)
#define MCLKDIV                                        (0)

/*
*      SUNXI_DA_TXCNT
*      I2S_AP TX Counter register
*      DA_TXCNT:codecbase+0x28
*/
#define TX_CNT                                 (0)

/*
*      SUNXI_DA_RXCNT
*      I2S_AP RX Counter register
*      DA_RXCNT:codecbase+0x2c
*/
#define RX_CNT                                 (0)

/*
*      SUNXI_DA_TXCHSEL
*      I2S_AP TX Channel Select register
*      DA_TXCHSEL:codecbase+0x30
*/
#define TX_CHSEL                               (0)

/*
*      SUNXI_DA_TXCHMAP
*      I2S_AP TX Channel Mapping register
*      DA_TXCHMAP:codecbase+0x34
*/
#define TX_CH3_MAP                             (12)
#define TX_CH2_MAP                             (8)
#define TX_CH1_MAP                             (4)
#define TX_CH0_MAP                             (0)

/*
*      SUNXI_DA_RXCHSEL
*      I2S_AP RX Channel Select register
*      DA_RXCHSEL:codecbase+0x38
*/
#define RX_CHSEL                               (0)

/*
*      SUNXI_DA_RXCHMAP
*      I2S_AP RX Channel Mapping register
*      DA_RXCHMAP:codecbase+0x3c
*/
#define RX_CH3_MAP                             (12)
#define RX_CH2_MAP                             (8)
#define RX_CH1_MAP                             (4)
#define RX_CH0_MAP                             (0)

/*
*      Chip Soft Reset register
*      CHIP_AUDIO_RST:codecbase+0x200
*/
// Reserved

/*
*      SUNXI_SYSCLK_CTL
*      System Clock Control Register
*      SYSCLK_CTL:codecbase+0x20c
*/
#define AIF1CLK_ENA                            (11)
#define AIF1CLK_SRC                            (8)
#define AIF2CLK_ENA                            (7)
#define AIF2CLK_SRC                            (4)
#define SYSCLK_ENA                             (3)
#define SYSCLK_SRC                             (0)

/*
*      SUNXI_MOD_CLK_ENA
*      Module Clock Control Register
*      MOD_CLK_ENA:codecbase+0x210
*/
#define MODULE_CLK_EN_CTL              (0)
#define AIF1_MOD_CLK_EN                        (15)
#define AIF2_MOD_CLK_EN                        (14)
#define AIF3_MOD_CLK_EN                        (13)
#define SRC1_MOD_CLK_EN                        (11)
#define SRC2_MOD_CLK_EN                        (10)
#define HPF_AGC_MOD_CLK_EN             (7)
#define HPF_DRC_MOD_CLK_EN             (6)
#define ADC_DIGITAL_MOD_CLK_EN (3)
#define DAC_DIGITAL_MOD_CLK_EN (2)

/*
*      SUNXI_MOD_RST_CTL
*      Module Reset Control Register
*      MOD_RST_CTL:codecbase+0x214
*/
#define MODULE_RST_CTL_BIT             (0)
#define AIF1_MOD_RST_CTL               (15)
#define AIF2_MOD_RST_CTL               (14)
#define AIF3_MOD_RST_CTL               (13)
#define SRC1_MOD_RST_CTL               (11)
#define SRC2_MOD_RST_CTL               (10)
#define HPF_AGC_MOD_RST_CTL            (7)
#define HPF_DRC_MOD_RST_CTL            (6)
#define ADC_DIGITAL_MOD_RST_CTL        (3)
#define DAC_DIGITAL_MOD_RST_CTL        (2)

/*
*      SUNXI_SYS_SR_CTRL
*      System Sample rate & SRC Configuration Register
*      SYS_SR_CTRL:codecbase+0x218
*/
#define AIF1_FS                                        (12)
#define AIF2_FS                                        (8)
#define SRC1_ENA                               (3)
#define SRC1_SRC                               (2)
#define SRC2_ENA                               (1)
#define SRC2_SRC                               (0)

/*
*      System SRC Clock source Select Register
*      SYS_SRC_CLK:codecbase+0x21c
*/
#define SRC_CLK_SLT                            (0)

/*
*      System SRC Clock source Select Register
*      SYS_SRC_CLK_220:codecbase+0x220
*/
#define AIFDVC_FS_SEL                  (0)

/*
*      SUNXI_AIF1CLK_CTRL
*      AIF1 BCLK/LRCK Control Register
*      AIF1CLK_CTRL:codecbase+0x240
*/
#define AIF1_MSTR_MOD                  (15)
#define AIF1_BCLK_INV                  (14)
#define AIF1_LRCK_INV                  (13)
#define AIF1_BCLK_DIV                  (9)
#define AIF1_LRCK_DIV                  (6)
#define AIF1_WORD_SIZ                  (4)
#define AIF1_DATA_FMT                  (2)
#define DSP_MONO_PCM                   (1)
#define AIF1_TDMM_ENA                  (0)

/*
*      SUNXI_AIF1_ADCDAT_CTRL
*      AIF1 ADCDAT Control Register
*      AIF1_ADCDAT_CTRL:codecbase+0x244
*/
#define AIF1_AD0L_ENA                  (15)
#define AIF1_AD0R_ENA                  (14)
#define AIF1_AD1L_ENA                  (13)
#define AIF1_AD1R_ENA                  (12)
#define AIF1_AD0L_SRC                  (10)
#define AIF1_AD0R_SRC                  (8)
#define AIF1_AD1L_SRC                  (6)
#define AIF1_AD1R_SRC                  (4)
#define AIF1_ADCP_ENA                  (3)
#define AIF1_ADUL_ENA                  (2)
#define AIF1_SLOT_SIZ                  (0)

/*
*      SUNXI_AIF1_DACDAT_CTRL
*      AIF1 DACDAT Control Register
*      AIF1_DACDAT_CTRL:codecbase+0x248
*/
#define AIF1_DA0L_ENA                  (15)
#define AIF1_DA0R_ENA                  (14)
#define AIF1_DA1L_ENA                  (13)
#define AIF1_DA1R_ENA                  (12)
#define AIF1_DA0L_SRC                  (10)
#define AIF1_DA0R_SRC                  (8)
#define AIF1_DA1L_SRC                  (6)
#define AIF1_DA1R_SRC                  (4)
#define AIF1_DACP_ENA                  (3)
#define AIF1_DAUL_ENA                  (2)
#define AIF1_LOOP_ENA                  (0)

/*
*      SUNXI_AIF1_MXR_SRC
*      AIF1 Digital Mixer Source Select Register
*      AIF1_MXR_SRC:codecbase+0x24c
*/
#define AIF1_AD0L_MXL_SRC_AIF1DA0L             (15)
#define AIF1_AD0L_MXL_SRC_AIF2DACL             (14)
#define AIF1_AD0L_MXL_SRC_ADCL         (13)
#define AIF1_AD0L_MXL_SRC_AIF2DACR             (12)
#define AIF1_AD0L_MXL_SRC              (12)
#define AIF1_AD0R_MXR_SRC_AIF1DA0R             (11)
#define AIF1_AD0R_MXR_SRC_AIF2DACR             (10)
#define AIF1_AD0R_MXR_SRC_ADCR         (9)
#define AIF1_AD0R_MXR_SRC_AIF2DACL             (8)
#define AIF1_AD0R_MXR_SRC              (8)
#define AIF1_AD1L_MXR_SRC              (6)
#define AIF1_AD1R_MXR_SRC              (2)
#define AIF1_AD1R_MXR_SRC_C            (0)

/*
*      SUNXI_AIF1_VOL_CTRL1
*      AIF1 Volume Control 1 Register
*      AIF1_VOL_CTRL1:codecbase+0x250
*/
#define AIF1_AD0L_VOL                  (8)
#define AIF1_AD0R_VOL                  (0)

/*
*      SUNXI_AIF1_VOL_CTRL2
*      AIF1 Volume Control 2 Register
*      AIF1_VOL_CTRL2:codecbase+0x254
*/
#define AIF1_AD1L_VOL                  (8)
#define AIF1_AD1R_VOL                  (0)

/*
*      SUNXI_AIF1_VOL_CTRL3
*      AIF1 Volume Control 3 Register
*      AIF1_VOL_CTRL3:codecbase+0x258
*/
#define AIF1_DA0L_VOL                  (8)
#define AIF1_DA0R_VOL                  (0)

/*
*      SUNXI_AIF1_VOL_CTRL4
*      AIF1 Volume Control 4 Register
*      AIF1_VOL_CTRL4:codecbase+0x25c
*/
#define AIF1_DA1L_VOL                  (8)
#define AIF1_DA1R_VOL                  (0)

/*
*      SUNXI_AIF1_MXR_GAIN
*      AIF1 Digital Mixer Gain Control Register
*      AIF1_MXR_GAIN:codecbase+0x260
*/
#define AIF1_AD0L_MXR_GAIN             (12)
#define AIF1_AD0R_MXR_GAIN             (8)
#define AIF1_AD1L_MXR_GAIN             (6)
#define AIF1_AD1R_MXR_GAIN             (2)

/*
*      AIF1 Receiver Data Discarding Control Register
*      AIF1_RXD_CTRL:codecbase+0x264
*/
#define AIF1_RXD_CTRL_DISCARD  (8)

/*
*      SUNXI_AIF2_CLK_CTRL
*      AIF2 BCLK/LRCK Control Register
*      AIF2_CLK_CTRL:codecbase+0x280
*/
#define AIF2_MSTR_MOD                  (15)
#define AIF2_BCLK_INV                  (14)
#define AIF2_LRCK_INV                  (13)
#define AIF2_BCLK_DIV                  (9)
#define AIF2_LRCK_DIV                  (6)
#define AIF2_WORD_SIZ                  (4)
#define AIF2_DATA_FMT                  (2)
#define AIF2_MONO_PCM                  (1)

/*
*      SUNXI_AIF2_ADCDAT_CTRL
*      AIF2 ADCDAT Control Register
*      AIF2_ADCDAT_CTRL:codecbase+0x284
*/
#define AIF2_ADCL_EN                   (15)
#define AIF2_ADCR_EN                   (14)
#define AIF2_ADCL_SRC                  (10)
#define AIF2_ADCR_SRC                  (8)
#define AIF2_ADCP_ENA                  (3)
#define AIF2_ADUL_ENA                  (2)
#define AIF2_LOOP_EN                   (0)

/*
*      SUNXI_AIF2_DACDAT_CTRL
*      AIF2 DACDAT Control Register
*      AIF2_DACDAT_CTRL:codecbase+0x288
*/
#define AIF2_DACL_ENA                  (15)
#define AIF2_DACR_ENA                  (14)
#define AIF2_DACL_SRC                  (10)
#define AIF2_DACR_SRC                  (8)
#define AIF2_DACP_ENA                  (3)
#define AIF2_DAUL_ENA                  (2)

/*
*      SUNXI_AIF2_MXR_SRC
*      AIF2 Digital Mixer Source Select Register
*      AIF2_MXR_SRC:codecbase+0x28c
*/
#define AIF2_ADCL_MXR_SRC_AIF1DA0L             (15)
#define AIF2_ADCL_MXR_SRC_AIF1DA1L             (14)
#define AIF2_ADCL_MXR_SRC_AIF2DACR             (13)
#define AIF2_ADCL_MXR_SRC_ADCL         (12)
#define AIF2_ADCL_MXR_SRC              (12)
#define AIF2_ADCR_MXR_SRC_AIF1DA0R             (11)
#define AIF2_ADCR_MXR_SRC_AIF1DA1R             (10)
#define AIF2_ADCR_MXR_SRC_AIF2DACL             (9)
#define AIF2_ADCR_MXR_SRC_ADCR         (8)
#define AIF2_ADCR_MXR_SRC              (8)

/*
*      SUNXI_AIF2_VOL_CTRL1
*      AIF2 Volume Control 1 Register
*      AIF2_VOL_CTRL1:codecbase+0x290
*/
#define AIF2_ADCL_VOL                  (8)
#define AIF2_ADCR_VOL                  (0)

/*
*      SUNXI_AIF2_VOL_CTRL2
*      AIF2 Volume Control 2 Register
*      AIF2_VOL_CTRL2:codecbase+0x298
*/
#define AIF2_DACL_VOL                  (8)
#define AIF2_DACR_VOL                  (0)

/*
*      SUNXI_AIF2_MXR_GAIN
*      AIF2 Digital Mixer Gain Control Register
*      AIF2_MXR_GAIN:codecbase+0x2A0
*/
#define AIF2_ADCL_MXR_GAIN             (12)
#define AIF2_ADCR_MXR_GAIN             (8)

/*
*      AIF2 Receiver Data Discarding Control Register
*      AIF2_RXD_CTRL:codecbase+0x2A4
*/
#define AIF2_RXD_CTRL_DISCARD  (8)

/*
*      SUNXI_AIF3_CLK_CTRL
*      AIF3 BCLK/LRCK Control Register
*      AIF3_CLK_CTRL:codecbase+0x2c0
*/
#define AIF3_BCLK_INV                  (14)
#define AIF3_LRCK_INV                  (13)
#define AIF3_WORD_SIZ                  (4)
#define AIF3_CLOC_SRC                  (0)

/*
*      AIF3 ADCDAT Control Register
*      AIF3_ADCDAT_CTRL:codecbase+0x2c4
*/
#define AIF3_ADCP_ENA                  (3)
#define AIF3_ADUL_ENA                  (2)

/*
*      SUNXI_AIF3_DACDAT_CTRL
*      AIF3 DACDAT Control Register
*      AIF3_DACDAT_CTRL:codecbase+0x2c8
*/
#define AIF3_DACP_ENA                  (3)
#define AIF3_DAUL_ENA                  (2)
#define AIF3_LOOP_ENA                  (0)

/*
*      SUNXI_AIF3_SGP_CTRL
*      AIF3 Signal Path Control Register
*      AIF3_SGP_CTRL:codecbase+0x2cc
*/
#define AIF3_ADC_SRC                   (10)
#define AIF2_DAC_SRC                   (8)
#define AIF3_PINS_TRI                  (7)
#define AIF3_ADCDAT_SRC                        (4)
#define AIF2_ADCDAT_SRC                        (3)
#define AIF2_DACDAT_SRC                        (2)
#define AIF1_ADCDAT_SRC                        (1)
#define AIF1_DACDAT_SRC                        (0)

/*
*      AIF3 Receiver Data Discarding Control Register
*      AIF3_RXD_CTRL:codecbase+0x2e4
*/
#define AIF3_RXD_CTRL_DISCARD  (8)

/*
*      SUNXI_ADC_DIG_CTRL
*      ADC Digital Control Register
*      ADC_DIG_CTRL:codecbase+0x300
*/
#define ENAD                                   (15)
#define ENDM                                   (14)
#define ADFIR32                                        (13)
#define ADOUT_DTS                              (2)
#define ADOUT_DLY                              (1)

/*
*      SUNXI_ADC_VOL_CTRL
*      ADC Volume Control Register
*      ADC_VOL_CTRL:codecbase+0x304
*/
#define ADC_VOL_L                              (8)
#define ADC_VOL_R                              (0)

/*
*      ADC Debug Control Register
*      ADC_DBG_CTRL:codecbase+0x308
*/
#define ADSW                                   (15)
#define ADDA_DIGITAL_ANALOG_DBG        (0)

/*
*      SUNXI_DAC_DIG_CTRL
*      DAC Digital Control Register
*      DAC_DIG_CTRL:codecbase+0x320
*/
#define ENDA                                   (15)
#define ENHPF                                  (14)
#define DAFIR32                                        (13)
#define MODQU                                  (8)

/*
*      SUNXI_DAC_VOL_CTRL
*      DAC Volume Control Register
*      DAC_VOL_CTRL:codecbase+0x324
*/
#define DAC_VOL_L                              (8)
#define DAC_VOL_R                              (0)

/*
*      SUNXI_DAC_DBG_CTRL
*      DAC Debug Control Register
*      DAC_DBG_CTRL:codecbase+0x32c
*/
#define DASW                                   (15)
#define ENDWA_N                                        (14)
#define DAC_MOD_DBG                            (13)
#define DAC_PTN_SEL                            (6)
#define DVC                                            (0)

/*
*      SUNXI_DAC_MXR_SRC
*      DAC Digital Mixer Source Select Register
*      DAC_MXR_SRC:codecbase+0x330
*/
#define DACL_MXR_SRC_AIF1DA0L                  (15)
#define DACL_MXR_SRC_AIF1DA1L                  (14)
#define DACL_MXR_SRC_AIF2DACL                  (13)
#define DACL_MXR_SRC_ADCL                      (12)
#define DACL_MXR_SRC                   (12)

#define DACR_MXR_SRC_AIF1DA0R                  (11)
#define DACR_MXR_SRC_AIF1DA1R                  (10)
#define DACR_MXR_SRC_AIF2ADCR                  (9)
#define DACR_MXR_SRC_ADCR                      (8)
#define DACR_MXR_SRC                   (8)

/*
*      SUNXI_DAC_MXR_GAIN
*      DAC Digital Mixer Gain Control Register
*      DAC_MXR_GAIN:codecbase+0x334
*/
#define DACL_MXR_GAIN                  (12)
#define DACR_MXR_GAIN                  (8)

/*
*      I2SAP AIF1 Debug Register
*      I2SAP_AIF1_DBG:codecbase+0x360
*/
#define SD_OUT_OEB                             (14)
#define DACDAT_IEB                             (13)
#define DACDAT_CUT                             (12)
#define ADCDAT_OEB                             (11)
#define SD_IN_IEB                              (10)
#define SD_IN_CUT                              (9)
#define LRCK_OEB                               (8)
#define LRCK_IEB                               (7)
#define LRCK_CUT                               (6)
#define BCLK_OEB                               (5)
#define BCLK_IEB                               (4)
#define BCLK_CUT                               (3)
#define MCLK_OEB                               (2)
#define MCLK_IEB                               (1)
#define MCLK_CUT                               (0)

/*
*      ADC DAP Left Status Register
*      AC_ADC_DAPLSTA:codecbase+0x400
*/
#define LEFT_AGC_SAT_FLAG              (9)
#define LEFT_AGC_N_THRESH_FLAG (8)
#define LEFT_GAIN_BY_AGC               (0)

/*
*      ADC DAP Right Status Register
*      AC_ADC_DAPRSTA:codecbase+0x404
*/
#define RIGHT_AGC_SAT_FLAG             (9)
#define RIGHT_AGC_N_THRESH_FLAG        (8)
#define RIGHT_GAIN_BY_AGC              (0)

/*
*      ADC DAP Left Channel Control Register
*      AC_ADC_DAPLCTRL:codecbase+0x408
*/
#define LEFT_AGC_EN                            (14)
#define LEFT_HPF_EN                            (13)
#define LEFT_NOISE_DETECT_EN   (12)
#define LEFT_HYSTERESIS_SET            (8)
#define LEFT_NOISE_DEB_TIME            (4)
#define LEFT_SIGNAL_DEB_TIME   (0)

/*
*      ADC DAP Right Channel Control Register
*      AC_ADC_DAPRCTRL:codecbase+0x40C
*/
#define RIGHT_AGC_EN                   (14)
#define RIGHT_HPF_EN                   (13)
#define RIGHT_NOISE_DETECT_EN  (12)
#define RIGHT_HYSTERESIS_SET   (8)
#define RIGHT_NOISE_DEB_TIME   (4)
#define RIGHT_SIGNAL_DEB_TIME  (0)

/*
*      ADC DAP Left Target Level Register
*      AC_ADC_DAPLTL:codecbase+0x410
*/
#define LEFT_CHAN_TAR_LEV_SET  (8)
#define LEFT_CHAN_MAX_GAIN_SET         (0)

/*
*      ADC DAP Right Target Level Register
*      AC_ADC_DAPRTL:codecbase+0x414
*/
#define RIGHT_CHAN_TAR_LEV_SET (8)
#define RIGHT_CHAN_MAX_GAIN_SET        (0)

/*
*      ADC DAP Left High Average Coef Register
*      AC_ADC_DAPLHAC:codecbase+0x418
*/
#define LEFT_CHAN_OUT_H_SIG_AVE_LEV (0)

/*
*      ADC DAP Left Low Average Coef Register
*      AC_ADC_DAPLLAC:codecbase+0x41c
*/
#define LEFT_CHAN_OUT_L_SIG_AVE_LEV (0)

/*
*      ADC DAP Right High Average Coef Register
*      AC_ADC_DAPRHAC:codecbase+0x420
*/
#define RIGHT_CHAN_OUT_H_SIG_AVE_LEV (0)

/*
*      ADC DAP Right Low Average Coef Register
*      AC_ADC_DAPRLAC:codecbase+0x424
*/
#define RIGHT_CHAN_OUT_L_SIG_AVE_LEV (0)

/*
*      ADC DAP Left Decay Time Register
*      AC_ADC_DAPLDT:codecbase+0x428
*/
#define LEFT_DEC_TIME_COEFF_SET        (0)

/*
*      ADC DAP Left Attack Time Register
*      AC_ADC_DAPLAT:codecbase+0x42c
*/
#define LEFT_ATTACK_TIME_COEFF_SET     (0)

/*
*      ADC DAP Right Decay Time Register
*      AC_ADC_DAPRDT:codecbase+0x430
*/
#define RIGHT_DEC_TIME_COEFF_SET       (0)

/*
*      ADC DAP Right Attack Time Register
*      AC_ADC_DAPRAT:codecbase+0x434
*/
#define RIGHT_ATTACK_TIME_COEFF_SET (0)

/*
*      ADC DAP Noise Threshold Register
*      AC_ADC_DAPNTH:codecbase+0x438
*/
#define LEFT_CHAN_NOISE_THRES_SET      (8)
#define RIGHT_CHAN_NOISE_THRES_SET     (0)

/*
*      ADC DAP Left Input Signal High Average Coef Register
*      AC_ADC_DAPLHNAC:codecbase+0x43c
*/
#define LEFT_INPUT_SIGNAL_H_AVE_FILT (0)

/*
*      ADC DAP Left Input Signal Low Average Coef Register
*      AC_ADC_DAPLLNAC:codecbase+0x440
*/
#define LEFT_INPUT_SIGNAL_L_AVE_FILT (0)

/*
*      ADC DAP Right Input Signal High Average Coef Register
*      AC_ADC_DAPRHNAC:codecbase+0x444
*/
#define RIGHT_INPUT_SIGNAL_H_AVE_FILT (0)

/*
*      ADC DAP Right Input Signal Low Average Coef Register
*      AC_ADC_DAPRLNAC:codecbase+0x448
*/
#define RIGHT_INPUT_SIGNAL_L_AVE_FILT (0)

/*
*      ADC DAP High HPF Coef Register
*      AC_DAPHHPFC:codecbase+0x44c
*/
#define HPF_H_COEFFICIENT_SET          (0)

/*
*      ADC DAP Low HPF Coef Register
*      AC_DAPLHPFC:codecbase+0x450
*/
#define HPF_L_COEFFICIENT_SET          (0)

/*
*      ADC DAP Optimum Register
*      AC_DAPOPT:codecbase+0x454
*/
#define LEFT_ENERGY_VAL_SET                    (10)
#define LEFT_CHAN_GAIN_HYSTER_SET              (8)
#define INPUT_SIGNAL_AVE_FILT_COEF_SET (5)
#define AGC_OUTPUT_CHAN_NOISE_STATE            (4)
#define RIGHT_ENERGY_VAL_SET                   (2)
#define RIGHT_CHAN_GAIN_HYSTER_SET             (0)

/*
*      DAC DAP Control Register
*      AC_DAC_DAPCTRL:codecbase+0x480
*/
#define DRC_EN                                         (2)
#define LEFT_CHAN_HPF_EN                       (1)
#define RIGHT_CHAN_HPF_EN                      (0)

/*
*      DAC DAP High HPF Coef Register
*      AC_DAC_DAPHHPFC:codecbase+0x484
*/
#define HPF_H_COEFF_SET                                (0)

/*
*      DAC DAP Low HPF Coef Register
*      AC_DAC_DAPLHPFC:codecbase+0x488
*/
#define HPF_L_COEFF_SET                                (0)

/*
*      DAC DAP left high energy average Coef Register
*      AC_DAC_DAPLHAVC:codecbase+0x48c
*/
#define LEFT_CHAN_H_ENGY_AVC_SET       (0)

/*
*      DAC DAP left Low energy average Coef Register
*      AC_DAC_DAPLLAVC:codecbase+0x490
*/
#define LEFT_CHAN_L_ENGY_AVC_SET       (0)

/*
*      DAC DAP right High energy average Coef Register
*      AC_DAC_DAPRHAVC:codecbase+0x494
*/
#define RIGHT_CHAN_H_ENGY_AVC_SET      (0)

/*
*      DAC DAP right Low energy average Coef Register
*      AC_DAC_DAPRLAVC:codecbase+0x498
*/
#define RIGHT_CHAN_L_ENGY_AVC_SET      (0)

/*
*      DAC DAP high gain decay time Coef Register
*      AC_DAC_DAPHGDEC:codecbase+0x49c
*/
#define GAIN_FILT_H_DEC_TIME_COEF_SET (0)

/*
*      DAC DAP Low gain decay time Coef Register
*      AC_DAC_DAPHGDEC:codecbase+0x4A0
*/
#define GAIN_FILT_L_DEC_TIME_COEF_SET (0)

/*
*      DAC DAP High gain Attack time Coef Register
*      AC_DAC_DAPHGATC:codecbase+0x4A4
*/
#define GAIN_FILT_H_ATC_TIME_COEF_SET (0)

/*
*      DAC DAP Low gain Attack time Coef Register
*      AC_DAC_DAPLGATC:codecbase+0x4A8
*/
#define GAIN_FILT_L_ATC_TIME_COEF_SET (0)

/*
*      DAC DAP High Energy Threshold Register
*      AC_DAC_DAPHETHD:codecbase+0x4AC
*/
#define DRC_ENERGY_H_COMPRESS_THRES_SET (0)

/*
*      DAC DAP Low Energy Threshold Register
*      AC_DAC_DAPLETHD:codecbase+0x4B0
*/
#define DRC_ENERGY_L_COMPRESS_THRES_SET (0)

/*
*      DAC DAP High Energy Threshold Register
*      AC_DAC_DAPHGKPA:codecbase+0x4B4
*/
#define DRC_H_GAIN_K_PARA_SET (0)

/*
*      DAC DAP low Energy Threshold Register
*      AC_DAC_DAPLGKPA:codecbase+0x4B8
*/
#define DRC_L_GAIN_K_PARA_SET (0)

/*
*      DAC DAP high gain offset parameter Register
*      AC_DAC_DAPHGOPA:codecbase+0x4BC
*/
#define DRC_H_GAIN_O_PARA_SET (0)

/*
*      DAC DAP low gain offset parameter Register
*      AC_DAC_DAPLGOPA:codecbase+0x4c0
*/
#define DRC_L_GAIN_O_PARA_SET (0)

/*
*      DAC DAP optimum Register
*      AC_DAC_DAPOPT:codecbase+0x4c4
*/
#define DRC_GAIN_VAL_SET                       (5)
#define HYSTER_GAIN_FILT_DECAY_TIME    (0)

/*
*      AGC Enable Register
*      AGC_ENA:codecbase+0x4D0
*/
#define AIF1_AD0L_AGC_ENA              (15)
#define AIF1_AD0R_AGC_ENA              (14)
#define AIF1_AD1L_AGC_ENA              (13)
#define AIF1_AD1R_AGC_ENA              (12)
#define AIF2_ADCL_AGC_ENA              (11)
#define AIF2_ADCR_AGC_ENA              (10)
#define AIF2_DACL_AGC_ENA              (9)
#define AIF2_DACR_AGC_ENA              (8)
#define ADCL_AGC_ENA                   (7)
#define ADCR_AGC_ENA                   (6)

/*
*      DRC Enable Register
*      DRC_ENA:codecbase+0x4D4
*/
#define AIF1_DAC0_DRC_ENA              (15)
#define AIF1_DAC1_DRC_ENA              (13)
#define AIF2_DAC_DRC_ENA               (11)
#define DAC_DRC_ENA                            (7)


/*
*      SRC bist control Register
*      SRC_BISTCR:codecbase+0x4D8
*/
#define SRC1_2_SRAM_REG_SEL            (13)
#define SRC1_2_SRAM_ADDR_MODE_SEL      (12)
#define SRC1_2_SRAM_WR_DATA_PATTEN     (9)
#define SRC1_2_SRAM_BIST_EN            (8)
#define SRC2_ROM_CHECKSUM_ERR  (7)
#define SRC2_ROM_CHECKXOR_ERR  (6)
#define SRC2_ROM_BIST_BUSY             (5)
#define SRC2_ROM_BIST_EN               (4)
#define SRC1_ROM_CHECKSUM_ERR  (3)
#define SRC1_ROM_CEHCKXOR_ERR  (2)
#define SRC1_ROM_BIST_BUSY             (1)
#define SRC1_ROM_BIST_EN               (0)

/*
*      SRC bist Status Register
*      SRC_BISTST:codecbase+0x4DC
*/
#define SRC2_SRAM_BIST_ERR_STATUS      (15)
#define SRC2_SRAM_BIST_ERR_PATTEN      (12)
#define SRC2_SRAM_BIST_ERR_CYCLES      (10)
#define SRC2_SRAM_BIST_STOP                    (9)
#define SRC2_SRAM_BIST_BUSY                    (8)
#define SRC1_SRAM_BIST_ERR_STATUS      (7)
#define SRC1_SRAM_BIST_ERR_PATTEN      (4)
#define SRC1_SRAM_BIST_ERR_CYCLES      (2)
#define SRC1_SRAM_BIST_STOP                    (1)
#define SRC1_SRAM_BIST_BUSY                    (0)

/*
*      SRC1 control 1 Register
*      SRC1_CTRL1:codecbase+0x4E0
*/
#define SRC1_RATI_ENA                          (15)
#define SRC1_LOCK_STS                          (14)
#define SRC1_FIFO_OVR                          (13)
#define SRC1_FIFO_LEV                          (10)
#define SRC1_RATI_SET                          (0)

/*
*      SRC1 control 2 Register
*      SRC1_CTRL2:codecbase+0x4E4
*/
#define SRC1_RATI_STET                         (0)

/*
*      SRC1 control 3 Register
*      SRC1_CTRL3:codecbase+0x4E8
*/
#define SRC1_FIFO_LEV                          (10)
#define SRC1_RATI_VAL                          (0)

/*
*      SRC1 control 4 Register
*      SRC1_CTRL4:codecbase+0x4EC
*/
#define SRC1_RATI_VAL                          (0)

/*
*      SRC2 control 1 Register
*      SRC2_CTRL1:codecbase+0x4f0
*/
#define SRC2_RATI_EN                           (15)
#define SRC2_LOCK_STS                          (14)
#define SRC2_FIFO_OVR                          (13)
#define SRC2_FIFO_LEV                          (10)
#define SRC2_RATI_SEL                          (0)

/*
*      SRC2 control 2 Register
*      SRC2_CTRL2:codecbase+0x4f4
*/
#define SRC2_RATI_SEL                          (0)

/*
*      SRC2 control 3 Register
*      SRC2_CTRL3:codecbase+0x4f8
*/
#define SRC2_FIFO_LEV                          (10)
#define SRC2_RATI_VAL                          (0)

/*
*      SRC2 control 4 Register
*      SRC2_CTRL4:codecbase+0x4fc
*/
#define SRC2_RATI_VAL                          (0)

#define ADDA_PR_CFG_REG          (SUNXI_R_PRCM_VBASE+0x1c0)
#define HP_VOLC                                          (0x00)
#define LOMIXSC                                          (0x01)
#define ROMIXSC                                          (0x02)
#define DAC_PA_SRC                               (0x03)
#define PHONEIN_GCTRL                    (0x04)
#define LINEIN_GCTRL                     (0x05)
#define MICIN_GCTRL                              (0x06)
#define PAEN_HP_CTRL                     (0x07)
#define PHONEOUT_CTRL                    (0x08)
#define LINEOUT_VOLC                     (0x09)
#define MIC2G_LINEEN_CTRL                (0x0A)
#define MIC1G_MICBIAS_CTRL               (0x0B)
#define LADCMIXSC                                (0x0C)
#define RADCMIXSC                                (0x0D)
#define ADC_AP_EN                                (0x0F)
#define ADDA_APT0                                (0x10)
#define ADDA_APT1                                (0x11)
#define ADDA_APT2                                (0x12)
#define BIAS_AD16_CAL_CTRL               (0x13)
#define BIAS_DA16_CAL_CTRL               (0x14)
#define DA16CALI                                 (0x15)
#define DA16VERIFY                               (0x16)
#define BIASCALI                                 (0x17)
#define BIASVERIFY                               (0x18)

/*
*      apb0 base
*      0x00 HP_VOLC
*/
#define PA_CLK_GC              (7)
#define HPVOL                  (0)

/*
*      apb0 base
*      0x01 LOMIXSC
*/
#define LMIXMUTE                                 (0)
#define LMIXMUTEDACR                     (0)
#define LMIXMUTEDACL                     (1)
#define LMIXMUTELINEINL                          (2)
#define LMIXMUTEPHONEN                   (3)
#define LMIXMUTEPHONEPN                          (4)
#define LMIXMUTEMIC2BOOST                (5)
#define LMIXMUTEMIC1BOOST                (6)

/*
*      apb0 base
*      0x02 ROMIXSC
*/
#define RMIXMUTE                                 (0)
#define RMIXMUTEDACL                     (0)
#define RMIXMUTEDACR                     (1)
#define RMIXMUTELINEINR                          (2)
#define RMIXMUTEPHONEP                   (3)
#define RMIXMUTEPHONEPN                          (4)
#define RMIXMUTEMIC2BOOST                (5)
#define RMIXMUTEMIC1BOOST                (6)

/*
*      apb0 base
*      0x03 DAC_PA_SRC
*/
#define DACAREN                        (7)
#define DACALEN                        (6)
#define RMIXEN                 (5)
#define LMIXEN                 (4)
#define RHPPAMUTE              (3)
#define LHPPAMUTE              (2)
#define RHPIS                  (1)
#define LHPIS                  (0)

/*
*      apb0 base
*      0x04 PHONEIN_GCTRL
*/
#define PHONEPG                        (4)
#define PHONENG                        (0)

/*
*      apb0 base
*      0x05 LINEIN_GCTRL
*/
#define LINEING                        (4)
#define PHONEG                 (0)

/*
*      apb0 base
*      0x06 MICIN_GCTRL
*/
#define MIC1G                  (4)
#define MIC2G                  (0)

/*
*      apb0 base
*      0x07 PAEN_HP_CTRL
*/
#define HPPAEN                  (7)
#define HPCOM_FC                (5)
#define COMPTEN                         (4)
#define PA_ANTI_POP_CTRL (2)
#define LTRNMUTE                (1)
#define RTLNMUTE                (0)

/*
*      apb0 base
*      0x08 PHONEOUT_CTRL
*/
#define PHONEOUTG               (5)
#define PHONEOUT_EN             (4)
#define PHONEOUTS3              (3)
#define PHONEOUTS2              (2)
#define PHONEOUTS1              (1)
#define PHONEOUTS0              (0)

/*
*      apb0 base
*      0x09 LINEOUT_VOLC
*/
#define LINEOUTVOL              (3)
#define PHONEPREG               (0)

/*
*      apb0 base
*      0x0A MIC2G_LINEEN_CTRL
*/
#define MIC2AMPEN               (7)
#define MIC2BOOST               (4)
#define LINEOUTL_EN             (3)
#define LINEOUTR_EN             (2)
#define LINEOUTL_SS             (1)
#define LINEOUTR_SS             (0)

/*
*      apb0 base
*      0x0B MIC1G_MICBIAS_CTRL
*/
#define HMICBIASEN              (7)
#define MMICBIASEN              (6)
#define HMICBIAS_MODE   (5)
#define MIC2_SS                         (4)
#define MIC1AMPEN               (3)
#define MIC1BOOST               (0)

/*
*      apb0 base
*      0x0C LADCMIXSC
*/
#define LADCMIXMUTE                              (0)
#define LADCMIXMUTEMIC1BOOST     (6)
#define LADCMIXMUTEMIC2BOOST     (5)
#define LADCMIXMUTEPHONEPN               (4)
#define LADCMIXMUTEPHONEN                (3)
#define LADCMIXMUTELINEINL               (2)
#define LADCMIXMUTELOUTPUT               (1)
#define LADCMIXMUTEROUTPUT               (0)

/*
*      apb0 base
*      0x0D RADCMIXSC
*/
#define RADCMIXMUTE                      (0)
#define RADCMIXMUTEMIC1BOOST     (6)
#define RADCMIXMUTEMIC2BOOST     (5)
#define RADCMIXMUTEPHONEPN               (4)
#define RADCMIXMUTEPHONEP                (3)
#define RADCMIXMUTELINEINR               (2)
#define RADCMIXMUTEROUTPUT               (1)
#define RADCMIXMUTELOUTPUT               (0)

/*
*      apb0 base
*      0x0E PA_ANTI_POP_CTRL_SLOP
*/
#define PA_ANTI_POP_CTRL_SLOP    (0)

/*
*      apb0 base
*      0x0F ADC_AP_EN
*/
#define ADCREN                  (7)
#define ADCLEN                  (6)
#define ADCG                    (0)

/*
*      apb0 base
*      0x10 ADDA_APT0
*/
#define OPDRV_OPCOM_CUR        (6)
#define OPADC1_BIAS_CUR        (4)
#define OPADC2_BIAS_CUR        (2)
#define OPAAF_BIAS_CUR (0)

/*
*      apb0 base
*      0x11 ADDA_APT1
*/
#define OPMIC_BIAS_CUR (6)
#define OPVR_BIAS_CUR  (4)
#define OPDAC_BIAS_CUR (2)
#define OPMIX_BIAS_CUR (0)

/*
*      apb0 base
*      0x12 ADDA_APT2
*/
#define ZERO_CROSS_EN                  (7)
#define TIMEOUT_ZERO_CROSS_EN  (6)
#define PTDBS                                  (4)
#define PA_SLOPE_SELECT                        (3)
#define USB_BIAS_CUR                   (0)


#define codec_rdreg(reg)         readl((baseaddr+(reg)))
#define codec_wrreg(reg,val)  writel((val),(baseaddr+(reg)))
int codec_wrreg_bits(unsigned short reg, unsigned int  mask,   unsigned int value);

int codec_wr_control(u32 reg, u32 mask, u32 shift, u32 val);


extern int snd_codec_info_volsw(struct snd_kcontrol *kcontrol,
               struct  snd_ctl_elem_info       *uinfo);
extern int snd_codec_get_volsw(struct snd_kcontrol     *kcontrol,
               struct  snd_ctl_elem_value      *ucontrol);
extern int snd_codec_put_volsw(struct  snd_kcontrol    *kcontrol,
       struct  snd_ctl_elem_value      *ucontrol);


int snd_codec_get_volsw_digital(struct snd_kcontrol    *kcontrol,
               struct  snd_ctl_elem_value      *ucontrol);

int snd_codec_put_volsw_digital(struct snd_kcontrol    *kcontrol,
       struct  snd_ctl_elem_value      *ucontrol);

/*
* Convenience kcontrol builders
*/
#define CODEC_SINGLE_VALUE(xreg, xshift, xmax, xinvert)\
               ((unsigned long)&(struct codec_mixer_control)\
               {.reg   =       xreg,   .shift  =       xshift, .rshift =       xshift, .max    =       xmax,\
       .invert =       xinvert})

#define CODEC_SINGLE(xname,    reg,    shift,  max,    invert)\
{      .iface  = SNDRV_CTL_ELEM_IFACE_MIXER, .name = xname,\
       .info   = snd_codec_info_volsw, .get = snd_codec_get_volsw,\
       .put    = snd_codec_put_volsw,\
       .private_value  = CODEC_SINGLE_VALUE(reg, shift, max, invert)}

/*     mixer control*/
struct codec_mixer_control{
       int             min;
       int     max;
       int     where;
       unsigned int mask;
       unsigned int reg;
       unsigned int rreg;
       unsigned int shift;
       unsigned int rshift;
       unsigned int invert;
       unsigned int value;
};


#define CODEC_SINGLE_DIGITAL(xname,    reg,    shift,  max,    invert)\
{      .iface  = SNDRV_CTL_ELEM_IFACE_MIXER, .name = xname,\
       .info   = snd_codec_info_volsw, .get = snd_codec_get_volsw_digital,\
       .put    = snd_codec_put_volsw_digital,\
       .private_value  = CODEC_SINGLE_VALUE(reg, shift, max, invert)}

//
#define    DAC_Digital_Part_Control 0x00
#define    DAC_FIFO_Control        0x04
#define    DAC_FIFO_Status         0x08
#define    DAC_TX_DATA             0x0c
#define    ADC_FIFO_Control        0x10
#define    ADC_FIFO_Status         0x14
#define    ADC_RX_DATA             0x18
#define    DAC_TX_Counter          0x40
#define    ADC_RX_Counter          0x44
#define    DAC_Debug               0x48
#define    ADC_Debug               0x4c

#define    Headphone_Volume_Control                    0x0
#define    Left_Output_Mixer_Source_Control            0x1
#define    Right_Output_Mixer_Source_Control           0x2
#define    DAC_Analog_Enable_and_PA_Source_Control     0x3
#define    Phonein_Stereo_Gain_Control                         0x4
#define    Linein_and_Phone_P_N_Gain_Control           0x5
#define    MIC1_and_MIC2_GAIN_Control                  0x6
#define    PA_Enable_and_HP_Control                    0x7
#define    Phoneout_Control                            0x8
#define    Lineout_Volume_Control                      0x9
#define    Mic2_Boost_and_Lineout_Enable_Control       0xA
#define    Mic1_Boost_and_MICBIAS_Control              0xB
#define    Left_ADC_Mixer_Source_Control               0xC
#define    Right_ADC_Mixer_Source_Control              0xD

#define    PA_UPTIME_CTRL                                   0xE
#define    ADC_ANALIG_PART_ENABLE_REG                    0xF


#define SUNXI_TXCHSEL_CHNUM(v)                         (((v)-1)<<0)
#define SUNXI_RXCHSEL_CHNUM(v)                         (((v)-1)<<0)



struct label {
    const char *name;
    int value;
};

#define LABEL(constant) { #constant, constant }
#define LABEL_END { NULL, -1 }

#endif
