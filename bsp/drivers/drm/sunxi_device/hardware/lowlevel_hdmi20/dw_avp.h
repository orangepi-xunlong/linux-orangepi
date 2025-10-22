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
#ifndef _DW_AVP_H_
#define _DW_AVP_H_

/*******************************************************************************
 * dw audio config function
 ******************************************************************************/
/**
 * @desc: dw audio set audio param data
 * @data: point to param data buffer
 * @return: 0 - success
 *         -1 - failed
 */
int dw_audio_set_info(void *data);
/**
 * @desc: dw audio init
 * @return: 0 - success
 *         -1 - failed
 */
int dw_audio_init(void);
/**
 * @desc: dw audio open
 * @return: 0 - success
 *         -1 - failed
 */
int dw_audio_on(void);
/*******************************************************************************
 * dw video config function
 ******************************************************************************/
/**
 * @desc: dw video set dtd timing info
 * @dtd: point to need set dtd
 * @rate: current dtd timing frame rate
 * @return: 0 - success
 *         -1 - failed
 */
int dw_video_filling_timing(dw_dtd_t *dtd, u32 rate);
/**
 * @desc: dw video get software pixel clock
 * @return: pixel clock. unit: KHZ
 */
u32 dw_video_get_pixel_clk(void);
/**
 * @desc: dw video get dtd timing cea vic code
 * @return: cea vic code
 */
u32 dw_video_get_cea_vic(void);
/**
 * @desc: dw video get hdmi vic by cea vic
 * @return: -1 - failed
 *       other - hdmi spec define hdmi_vic
 */
int dw_video_cea_to_hdmi_vic(int cea_vic);
/**
 * @desc: dw video select send hdmi14 vsif
 * @format: vsif video format
 * @hdmi_vic: hdmi vic code
 * @return: 0 - success
 */
int dw_video_use_hdmi14_vsif(u8 format, u8 data_pb5);
/**
 * @desc: dw video select send hdmi20 vsif
 * @return: 0 - success
 */
int dw_video_use_hdmi20_vsif(void);
/**
 * @desc: dw video set video format and vic code
 * @type: video format type
 * @code: hdmi or vic code
 */
int dw_video_set_vic_format(enum dw_video_format_e type, u32 code);
/**
 * @desc: dw video color space convert coefficients. now not use
 * @video: point to video dev
 */
void dw_video_csc_update_coefficients(struct dw_video_s *video);
/**
 * @desc: dw video software update color format
 * @format: color format
 * @return: 0 - success
 */
int dw_video_update_color_format(dw_color_format_t format);
/**
 * @desc: dw video software update color depth
 * @bits: color depth
 * @return: 0 - success
 */
int dw_video_update_color_depth(u8 bits);
/**
 * @desc: dw video software update color metry
 * @metry: color metry
 * @ext_metry: extrenal color metry
 * @return: 0 - success
 */
int dw_video_update_color_metry(u8 metry, u8 ext_metry);
/**
 * @desc: dw video software update color eotf
 * @hdr: hdr mode flag
 * @eotf: set eotf value
 * @return: 0 - success
 */
int dw_video_update_hdr_eotf(u8 eotf);
/**
 * @desc: dw video software update tmds mode
 * @mode: tmds mode. hdmi or dvi
 * @return: 0 - success
 */
int dw_video_update_tmds_mode(u8 mode);
/**
 * @desc: dw video software update color range
 * @range: color range
 * @return: 0 - success
 */
int dw_video_update_range(u8 range);
/**
 * @desc: dw video software update color scan
 * @format: color scan
 * @return: 0 - success
 */
int dw_video_update_scaninfo(u8 scan);
/**
 * @desc: dw video software update picture ratio
 * @ratio: picture ratio
 * @return: 0 - success
 */
int dw_video_update_ratio(u8 ratio);
/**
 * @desc: dw video print disp info
 * @return 0 - success
 */
int dw_video_dump_disp_info(void);
/*******************************************************************************
 * dw audio video path config function
 ******************************************************************************/
/**
 * @desc: dw avp dump
 * @buf: point to dump buffer
 * @return: dump info size. Unit: byte
 */
ssize_t dw_avp_dump(char *buf);
/**
 * @desc: dw avp set send avmute singal
 * @enable: 0 - disable avmute
 *          1 - enable avmute
 * @return: 0 - success
 *         -1 - failed
 */
int dw_avp_set_mute(u8 enable);
/**
 * @desc: dw avp config send scramble
 * @return: 0 - success
 *         -1 - failed
 */
int dw_avp_config_scramble(void);
/**
 * @desc: dw avp config flow
 * @return: 0 - success
 *         -1 - failed
 */
int dw_avp_config(void);

/*****************************************************************************
 *                                                                           *
 *                           Audio Sample Registers                          *
 *                                                                           *
 *****************************************************************************/

/**
 * @desc: Audio I2S Software FIFO Reset, Select, and Enable Control Register 0
 * This register configures the I2S input enable that indicates which input I2S channels have valid data
 * @bits: [3:0] I2S data enable
 *          - bit[0]: I2Sdata[0] enable
 *          - bit[1]: I2Sdata[1] enable
 *          - bit[2]: I2Sdata[2] enable
 *          - bit[3]: I2Sdata[3] enable
 *        [4:4] This field is a "spare" bit with no associated functionality
 *        [5:5] I2S select
 *          - 1b: Selects I2S Audio Interface
 *          - 0b: Selects the second (SPDIF/GPA) interface, in configurations with more that one audio interface (DOUBLE/GDOUBLE)
 *        [6:6] This field is a "spare" bit with no associated functionality
 *        [7:7] Audio FIFOs software reset
 *          - 1b: Resets all audio FIFOs Reading from this register always returns 0b
 *          - 0b: no action taken
 */
#define AUD_CONF0           0x00003100
#define AUD_CONF0_I2S_IN_EN_MASK          0x0000000F
#define AUD_CONF0_SPARE_1_MASK            0x00000010
#define AUD_CONF0_I2S_SELECT_MASK         0x00000020
#define AUD_CONF0_SPARE_2_MASK            0x00000040
#define AUD_CONF0_SW_AUDIO_FIFO_RST_MASK  0x00000080

/**
 * @desc: Audio I2S Width and Mode Configuration Register 1
 * This register configures the I2S mode and data width of the input data
 * @bits: [4:0] I2S input data width
 *          - 00000b-01111b: Not used
 *          - 10000b: 16 bit data samples at input
 *          - 10001b: 17 bit data samples at input
 *          - 10010b: 18 bit data samples at input
 *          - 10011b: 19 bit data samples at input
 *          - 10100b: 20 bit data samples at input
 *          - 10101b: 21 bit data samples at input
 *          - 10110b: 22 bit data samples at input
 *          - 10111b: 23 bit data samples at input
 *          - 11000b: 24 bit data samples at input
 *          - 11001b-11111b: Not Used
 *        [7:5] I2S input data mode
 *          - 000b: Standard I2S mode
 *          - 001b: Right-justified I2S mode
 *          - 010b: Left-justified I2S mode
 *          - 011b: Burst 1 mode
 *          - 100b: Burst 2 mode
 */
#define AUD_CONF1           0x00003101
#define AUD_CONF1_I2S_WIDTH_MASK      0x0000001F
#define AUD_CONF1_I2S_MODE_MASK       0x000000E0

/**
 * @desc: I2S FIFO status and interrupts
 * @bits: [2:2] FIFO full mask
 *        [3:3] FIFO empty mask
 */
#define AUD_INT         0x00003102
#define AUD_INT_FIFO_FULL_MASK_MASK   0x00000004
#define AUD_INT_FIFO_EMPTY_MASK_MASK  0x00000008

/**
 * @desc: Audio I2S NLPCM and HBR configuration Register 2
 * This register configures the I2S Audio Data mapping
 * @bits: [0:0] I2S HBR Mode Enable
 *        [1:1] I2S NLPCM Mode Enable
 *        [2:2]
 *          - 1: the insertion of the PCUV bits on the incoming audio stream
 *          - 0: the incoming audio stream must contain the PCUV bits, mapped according to databook
 */
#define AUD_CONF2       0x00003103
#define AUD_CONF2_HBR_MASK          0x00000001
#define AUD_CONF2_NLPCM_MASK        0x00000002
#define AUD_CONF2_INSERT_PCUV_MASK  0x00000004

/**
 * @desc: I2S Mask Interrupt Register
 * This register masks the interrupts present in the I2S module
 * @bits: [4:4] FIFO overrun mask
 */
#define AUD_INT1  0x00003104
#define AUD_INT1_FIFO_OVERRUN_MASK_MASK  0x00000010

/*****************************************************************************
 *                                                                           *
 *                         Audio Packetizer Registers                        *
 *                                                                           *
 *****************************************************************************/

/**
 * @desc: Audio Clock Regenerator N Value Register 1
 * For N expected values, refer to the HDMI 1.4b specification
 * @bits: [7:0] HDMI Audio Clock Regenerator N value
 */
#define AUD_N1          0x00003200
#define AUD_N1_AUDN_MASK                0x000000FF

/**
 * @desc: Audio Clock Regenerator N Value Register 2
 * For N expected values, refer to the HDMI 1.4b specification
 * @bits: [7:0] HDMI Audio Clock Regenerator N value
 */
#define AUD_N2          0x00003201
#define AUD_N2_AUDN_MASK                0x000000FF

/**
 * @desc: Audio Clock Regenerator N Value Register 3
 * For N expected values, refer to the HDMI 1.4b specification
 * @bits: [3:0] HDMI Audio Clock Regenerator N value
 *        [7:7] When set, the new N and CTS values are only used when aud_n1 register is written
 */
#define AUD_N3          0x00003202
#define AUD_N3_AUDN_MASK               0x0000000F
#define AUD_N3_NCTS_ATOMIC_WRITE_MASK  0x00000080

/**
 * @desc: Audio Clock Regenerator CTS Value Register 1
 * For CTS expected values, refer to the HDMI 1.4b specification
 * @bits: [7:0] HDMI Audio Clock Regenerator CTS calculated value
 */
#define AUD_CTS1        0x00003203
#define AUD_CTS1_AUDCTS_MASK  0x000000FF

/**
 * @desc: Audio Clock Regenerator CTS Value Register 2
 * For CTS expected values, refer to the HDMI 1.4b specification
 * @bits: [7:0] HDMI Audio Clock Regenerator CTS calculated value
 */
#define AUD_CTS2        0x00003204
#define AUD_CTS2_AUDCTS_MASK  0x000000FF

/**
 * @desc: Audio Clock Regenerator CTS value Register 3
 * For CTS expected values, refer to the HDMI 1.4b specification
 * @bits: [3:0] HDMI Audio Clock Regenerator CTS calculated value
 *        [4:4] If the CTS_manual bit equals 0b, this registers contains audCTS[19:0]
 *              generated by the Cycle time counter according to the specified timing
 *        [7:5] N_shift factor configuration: N_shift | Shift Factor | Action 0 | 1 |
 *              This is the N shift factor used for the case that N' ="audN[19:0]"
 */
#define AUD_CTS3        0x00003205
#define AUD_CTS3_AUDCTS_MASK      0x0000000F
#define AUD_CTS3_CTS_MANUAL_MASK  0x00000010
#define AUD_CTS3_N_SHIFT_MASK     0x000000E0

/**
 * @desc: Audio Input Clock FS Factor Register
 * @bits: [2:0] Fs factor configuration
 *          - 0: 128xFs, For more detailed description refer to the databook
 *          - 1: 256xFs
 *          - 2: 512xFs
 *          - 3: Reserved
 *          - 4: 64xFs
 *          - Other: 128xFs
 */
#define AUD_INPUTCLKFS  0x00003206
#define AUD_INPUTCLKFS_IFSFACTOR_MASK  0x00000007

/**
 * @desc: Audio CTS Dither Register
 * @bits: [3:0] Dither divisor
 *        [7:4] Dither dividend
 */
#define AUD_CTS_DITHER  0x00003207

/*****************************************************************************
 *                                                                           *
 *                        Audio Sample SPDIF Registers                       *
 *                                                                           *
 *****************************************************************************/

/**
 * @desc: Audio SPDIF Software FIFO Reset Control Register 0.
 * This register allows the system processor to reset audio FIFOs upon underflow/overflow error detection.
 * @bit: [6:0] This field is a "spare" bit with no associated functionality
 *       [7:7] Audio FIFOs software reset Writing.
 *          - 0: no action taken Writing
 *          - 1: Resets all audio FIFOs Reading from this register always returns 0b
 */
#define AUD_SPDIF0  0x00003300
#define AUD_SPDIF0_SPARE_MASK              0x0000007F
#define AUD_SPDIF0_SW_AUDIO_FIFO_RST_MASK  0x00000080

/**
 * @desc: Audio SPDIF NLPCM and Width Configuration Register 1
 * This register configures the SPDIF data width.
 * @bit: [4:0] SPDIF input data width
 *          - 00000b-01111b: Not used.
 *          - 10000b: 16-bit data samples at input.
 *          - 10001b: 17-bit data samples at input.
 *          - 10010b: 18-bit data samples at input.
 *          - 10011b: 19-bit data samples at input.
 *          - 10100b: 20-bit data samples at input.
 *          - 10101b: 21-bit data samples at input.
 *          - 10110b: 22-bit data samples at input.
 *          - 10111b: 23-bit data samples at input.
 *          - 11000b: 24-bit data samples at input.
 *          - 11001b-11111b: Not Used.
 *       [5:5] This field is a "spare" bit with no associated functionality
 *       [6:6] When set to 1'b1, this bit field indicates that the input stream
 *              has a High Bit Rate (HBR) to be transmitted in HDMI HBR packets
 *       [7:7] Select Non-Linear (1b) / Linear (0b) PCM mode
 */
#define AUD_SPDIF1  0x00003301
#define AUD_SPDIF1_SPDIF_WIDTH_MASK     0x0000001F
#define AUD_SPDIF1_SPARE_MASK           0x00000020
#define AUD_SPDIF1_SPDIF_HBR_MODE_MASK  0x00000040
#define AUD_SPDIF1_SETNLPCM_MASK        0x00000080

/**
 * @desc: Audio SPDIF FIFO Empty/Full Mask Register
 * @bits: [2:2] SPDIF FIFO empty mask
 *        [3:3] SPDIF FIFO full mask
 */
#define AUD_SPDIFINT  0x00003302
#define AUD_SPDIFINT_SPDIF_FIFO_FULL_MASK_MASK   0x00000004
#define AUD_SPDIFINT_SPDIF_FIFO_EMPTY_MASK_MASK  0x00000008

/**
 * @desc: Audio SPDIF Mask Interrupt Register 1.
 * This register masks interrupts present in the SPDIF module
 * @bits: [4:4] FIFO overrun mask
*/
#define AUD_SPDIFINT1   0x00003303
#define AUD_SPDIFINT1_FIFO_OVERRUN_MASK_MASK  0x00000010

/**
 * @desc: Audio SPDIF Enable Configuration Register 2
 * This register configures the SPDIF input enable that indicates which input SPDIF channels have valid data
 * @bits: [3:0]
 *          - bit[0]: ispdifdata[0] enable
 *          - bit[1]: ispdifdata[1] enable
 *          - bit[2]: ispdifdata[2] enable
 *          - bit[3]: ispdifdata[3] enable
 */
#define AUD_SPDIF2      0x00003304
#define AUD_SPDIF2_IN_EN_MASK          0x0000000F

/*****************************************************************************
 *                                                                           *
 *                          Video Sampler Registers                          *
 *                                                                           *
 *****************************************************************************/

/**
 * @desc: Video Input Mapping and Internal Data Enable Configuration Register
 * @bits: [4:0] Video Input mapping (color space/color depth)
 *          - 0x01: RGB 4:4:4/8 bits
 *          - 0x03: RGB 4:4:4/10 bits
 *          - 0x05: RGB 4:4:4/12 bits
 *          - 0x07: RGB 4:4:4/16 bits
 *          - 0x09: YCbCr 4:4:4 or 4:2:0/8 bits
 *          - 0x0B: YCbCr 4:4:4 or 4:2:0/10 bits
 *          - 0x0D: YCbCr 4:4:4 or 4:2:0/12 bits
 *          - 0x0F: YCbCr 4:4:4 or 4:2:0/16 bits
 *          - 0x16: YCbCr 4:2:2/8 bits
 *          - 0x14: YCbCr 4:2:2/10 bits
 *          - 0x12: YCbCr 4:2:2/12 bits
 *          - 0x17: YCbCr 4:4:4 (IPI)/8 bits
 *          - 0x18: YCbCr 4:4:4 (IPI)/10 bits
 *          - 0x19: YCbCr 4:4:4 (IPI)/12 bits
 *          - 0x1A: YCbCr 4:4:4 (IPI)/16 bits
 *          - 0x1B: YCbCr 4:2:2 (IPI)/12 bits
 *          - 0x1C: YCbCr 4:2:0 (IPI)/8 bits
 *          - 0x1D: YCbCr 4:2:0 (IPI)/10 bits
 *          - 0x1E: YCbCr 4:2:0 (IPI)/12 bits
 *          - 0x1F: YCbCr 4:2:0 (IPI)/16 bits
 *        [7:7] Internal data enable (DE) generator enable
 */
#define TX_INVID0               0x00000200
#define TX_INVID0_VIDEO_MAPPING_MASK                0x0000001F
#define TX_INVID0_INTERNAL_DE_GENERATOR_MASK        0x00000080

/**
 * @desc: Video Input Stuffing Enable Register
 * @bits: [0:0]
 *          - 0: When the dataen signal is low, the value in the gydata[15:0]
 *              output is the one sampled from the corresponding input data
 *        [1:1]
 *          - 0: When the dataen signal is low, the value in the rcrdata[15:0]
 *              output is the one sampled from the corresponding input data
 *        [2:2]
 *          - 0: When the dataen signal is low, the value in the bcbdata[15:0]
 *              output is the one sampled from the corresponding input data
 */
#define TX_INSTUFFING           0x00000201
#define TX_INSTUFFING_GYDATA_STUFFING_MASK          0x00000001
#define TX_INSTUFFING_RCRDATA_STUFFING_MASK         0x00000002
#define TX_INSTUFFING_BCBDATA_STUFFING_MASK         0x00000004

/**
 * @desc: Video Input gy Data Channel Stuffing Register 0
 * @bits: [7:0] the value of gydata[7:0] when TX_INSTUFFING[0] (gydata_stuffing) is set to 1b
 */
#define TX_GYDATA0              0x00000202
#define TX_GYDATA0_GYDATA_MASK                      0x000000FF

/**
 * @desc: Video Input gy Data Channel Stuffing Register 2
 * @bits: [7:0] the value of gydata[15:8 when TX_INSTUFFING[0] (gydata_stuffing) is set to 1b
 */
#define TX_GYDATA1              0x00000203
#define TX_GYDATA1_GYDATA_MASK                      0x000000FF

/**
 * @desc: Video Input rcr Data Channel Stuffing Register 0
 * @bits: [7:0] the value of rcrydata[7:0] when TX_INSTUFFING[1] (rcrdata_stuffing) is set to 1b
 */
#define TX_RCRDATA0             0x00000204
#define TX_RCRDATA0_RCRDATA_MASK                    0x000000FF

/**
 * @desc: Video Input rcr Data Channel Stuffing Register 1
 * @bits: [7:0] the value of rcrydata[15:8] when TX_INSTUFFING[1] (rcrdata_stuffing) is set to 1b
 */
#define TX_RCRDATA1             0x00000205
#define TX_RCRDATA1_RCRDATA_MASK                    0x000000FF

/**
 * @desc: Video Input bcb Data Channel Stuffing Register 0
 * @bits: [7:0] the value of bcbdata[7:0] when TX_INSTUFFING[2] (bcbdata_stuffing) is set to 1b
 */
#define TX_BCBDATA0             0x00000206
#define TX_BCBDATA0_BCBDATA_MASK                    0x000000FF

/**
 * @desc: Video Input bcb Data Channel Stuffing Register 1
 * @bits: [7:0] the value of bcbdata[15:8] when TX_INSTUFFING[2] (bcbdata_stuffing) is set to 1b
 */
#define TX_BCBDATA1             0x00000207
#define TX_BCBDATA1_BCBDATA_MASK                    0x000000FF

/*****************************************************************************
 *                                                                           *
 *                         Video Packetizer Registers                        *
 *                                                                           *
 *****************************************************************************/

/**
 * @desc: Video Packetizer Packing Phase Status Register
 * @bits: [3:0] Read only register that holds the "packing phase" output of the Video Packetizer block
 */
#define VP_STATUS               0x00000800
#define VP_STATUS_PACKING_PHASE_MASK                0x0000000F

/**
 * @desc: Video Packetizer Pixel Repetition and Color Depth Register
 * @bits: [3:0] Desired pixel repetition factor configuration
 *          - 0000b: No Pixel repetition
 *          - 0001b: Pixel sent two times
 *          - 0010b: Pixel sent three times
 *          - 0011b: Pixel sent four times
 *          - 0100b: Pixel sent five times
 *          - 0101b: Pixel sent six times
 *          - 0110b: Pixel sent seven times
 *          - 0111b: Pixel sent eight times
 *          - 1000b: Pixel sent nine times
 *          - 1001b: Pixel sent ten times
 *        [7:4] The Color depth configuration is described as the following, with the action stated corresponding to color_depth[3:0]
 *          - 0000b: 24 bits per pixel video (8 bits per component)
 *          - 0001b-0011b: Not used
 *          - 0100b: 24 bits per pixel video (8 bits per component)
 *          - 0101b: 30 bits per pixel video (10 bits per component)
 *          - 0110b: 36 bits per pixel video (12 bits per component)
 *          - 0111b: 48 bits per pixel video (16 bits per component)
 *          - Other: Not used
 */
#define VP_PR_CD                0x00000801
#define VP_PR_CD_DESIRED_PR_FACTOR_MASK             0x0000000F
#define VP_PR_CD_COLOR_DEPTH_MASK                   0x000000F0

/**
 * @desc: Video Packetizer Stuffing and Default Packing Phase Register
 * @bits: [0:0] Pixel repeater stuffing control
 *        [1:1] Pixel packing stuffing control
 *        [2:2] YCC 422 remap stuffing control
 *        [3:3] Reserved, Controls packing maching strategy
 *        [4:4] Reserved, Controls packing maching strategy
 *        [5:5] Controls the default phase packing machine
 */
#define VP_STUFF                0x00000802
#define VP_STUFF_PR_STUFFING_MASK                   0x00000001
#define VP_STUFF_PP_STUFFING_MASK                   0x00000002
#define VP_STUFF_YCC422_STUFFING_MASK               0x00000004
#define VP_STUFF_ICX_GOTO_P0_ST_MASK                0x00000008
#define VP_STUFF_IFIX_PP_TO_LAST_MASK               0x00000010
#define VP_STUFF_IDEFAULT_PHASE_MASK                0x00000020

/**
 * @desc: Video Packetizer YCC422 Remapping Register
 * @bits: [1:0] YCC 422 remap input video size ycc422_size[1:0]
 *          - 00b: YCC 422 16-bit input video (8 bits per component)
 *          - 01b: YCC 422 20-bit input video (10 bits per component)
 *          - 10b: YCC 422 24-bit input video (12 bits per component)
 *          - 11b: Reserved
 */
#define VP_REMAP                0x00000803
#define VP_REMAP_YCC422_SIZE_MASK                   0x00000003

/**
 * @desc: Video Packetizer Output, Bypass and Enable Configuration Register
 * @bits: [1:0] Video Packetizer output selection output_selector[1:0]
 *          - 00b: Data from pixel packing block
 *          - 01b: Data from YCC 422 remap block
 *          - 10b: Data from 8-bit bypass block
 *          - 11b: Data from 8-bit bypass block
 *        [2:2] bypass_select
 *          - 0b: Data from pixel repeater block
 *          - 1b: Data from input of Video Packetizer block
 *        [3:3] YCC 422 select enable
 *        [4:4] Pixel repeater enable
 *        [5:5] Pixel packing enable
 *        [6:6] Bypass enable
 */
#define VP_CONF                 0x00000804
#define VP_CONF_OUTPUT_SELECTOR_MASK                0x00000003
#define VP_CONF_BYPASS_SELECT_MASK                  0x00000004
#define VP_CONF_YCC422_EN_MASK                      0x00000008
#define VP_CONF_PR_EN_MASK                          0x00000010
#define VP_CONF_PP_EN_MASK                          0x00000020
#define VP_CONF_BYPASS_EN_MASK                      0x00000040

/**
 * @desc: Video Packetizer Interrupt Mask Register
 * @bits: [0:0] Mask bit for Video Packetizer 8-bit bypass FIFO empty
 *        [1:1] Mask bit for Video Packetizer 8-bit bypass FIFO full
 *        [2:2] Mask bit for Video Packetizer pixel YCC 422 re-mapper FIFO empty
 *        [3:3] Mask bit for Video Packetizer pixel YCC 422 re-mapper FIFO full
 *        [4:4] Mask bit for Video Packetizer pixel packing FIFO empty
 *        [5:5] Mask bit for Video Packetizer pixel packing FIFO full
 *        [6:6] Mask bit for Video Packetizer pixel repeater FIFO empty
 *        [7:7] Mask bit for Video Packetizer pixel repeater FIFO full
 */
#define VP_MASK                 0x00000807
#define VP_MASK_OINTEMPTYBYP_MASK                   0x00000001
#define VP_MASK_OINTFULLBYP_MASK                    0x00000002
#define VP_MASK_OINTEMPTYREMAP_MASK                 0x00000004
#define VP_MASK_OINTFULLREMAP_MASK                  0x00000008
#define VP_MASK_OINTEMPTYPP_MASK                    0x00000010
#define VP_MASK_OINTFULLPP_MASK                     0x00000020
#define VP_MASK_OINTEMPTYREPET_MASK                 0x00000040
#define VP_MASK_OINTFULLREPET_MASK                  0x00000080

/*****************************************************************************
 *                                                                           *
 *                      Color Space Converter Registers                      *
 *                                                                           *
 *****************************************************************************/

/**
 * @desc: Color Space Converter Interpolation and Decimation Configuration Register
 * @bits: [1:0] Chroma decimation configuration: decmode[1:0] | Chroma Decimation
 *          - 00: decimation disabled
 *          - 01: Hd (z) =1
 *          - 10: Hd(z)=1/ 4 + 1/2z^(-1 )+1/4 z^(-2)
 *          - 11: Hd(z)x2^(11)= -5+12z^(-2) - 22z^(-4)+39z^(-8) +109z^(-10) -204z^(-12)+
 *              648z^(-14) + 1024z^(-15) +648z^(-16) -204z^(-18) +109z^(-20)- 65z^(-22)+
 *              39z^(-24) -22z^(-26) +12z^(-28)-5z^(-30)
 *        [3:2] This is a spare register with no associated functionality
 *        [5:4] Chroma interpolation configuration: intmode[1:0] | Chroma Interpolation
 *          - 00: interpolation disabled
 *          - 01: Hu (z) =1 + z^(-1)
 *          - 10: Hu(z)=1/ 2 + z^(-11)+1/2 z^(-2)
 *          - 11: interpolation disabled
 *        [6:6] This is a spare register with no associated functionality
 *        [7:7] When set (1'b1), the range limitation values defined in registers
 *              csc_mat_uplim and csc_mat_dnlim are applied to the output of the Color Space Conversion matrix
 */
#define CSC_CFG                 0x00004100
#define CSC_CFG_DECMODE_MASK                        0x00000003
#define CSC_CFG_SPARE_1_MASK                        0x0000000C
#define CSC_CFG_INTMODE_MASK                        0x00000030
#define CSC_CFG_SPARE_2_MASK                        0x00000040
#define CSC_CFG_CSC_LIMIT_MASK                      0x00000080

/**
 * @desc: Color Space Converter Scale and Deep Color Configuration Register
 * @bits: [1:0] Defines the cscscale[1:0] scale factor to apply to all coefficients in Color Space Conversion
 *        [3:2] The is a spare register with no associated functionality
 *        [7:4] Color space converter color depth configuration: csc_colordepth[3:0]
 *          - 0000: 24 bit per pixel video (8 bit per component)
 *          - 00001-0011: Not used
 *          - 0100: 24 bit per pixel video (8 bit per component)
 *          - 0101: 30 bit per pixel video (10 bit per component)
 *          - 0110: 36 bit per pixel video (12 bit per component)
 *          - 0111: 48 bit per pixel video (16 bit per component)
 */
#define CSC_SCALE               0x00004101
#define CSC_SCALE_CSCSCALE_MASK                     0x00000003
#define CSC_SCALE_SPARE_MASK                        0x0000000C
#define CSC_SCALE_CSC_COLOR_DEPTH_MASK              0x000000F0

/**
 * @desc: Color Space Converter Matrix A1 Coefficient Register MSB
 * Notes: - The coefficients used in the CSC matrix use only 15 bits for the internal computations
 * @bits: [7:0] Color Space Converter Matrix A1 Coefficient Register MSB
 */
#define CSC_COEF_A1_MSB         0x00004102
#define CSC_COEF_A1_MSB_CSC_COEF_A1_MSB_MASK        0x000000FF

/**
 * @desc: Color Space Converter Matrix A1 Coefficient Register LSB
 * Notes: - The coefficients used in the CSC matrix use only 15 bits for the internal computations
 * @bits: [7:0] Color Space Converter Matrix A1 Coefficient Register LSB
 */
#define CSC_COEF_A1_LSB         0x00004103
#define CSC_COEF_A1_LSB_CSC_COEF_A1_LSB_MASK        0x000000FF

/**
 * @desc: Color Space Converter Matrix A2 Coefficient Register MSB
 * Notes: - The coefficients used in the CSC matrix use only 15 bits for the internal computations
 * @bits: [7:0] Color Space Converter Matrix A2 Coefficient Register MSB
 */
#define CSC_COEF_A2_MSB         0x00004104
#define CSC_COEF_A2_MSB_CSC_COEF_A2_MSB_MASK        0x000000FF

/**
 * @desc: Color Space Converter Matrix A2 Coefficient Register LSB
 * Notes: - The coefficients used in the CSC matrix use only 15 bits for the internal computations
 * @bits: [7:0] Color Space Converter Matrix A2 Coefficient Register LSB
 */
#define CSC_COEF_A2_LSB         0x00004105
#define CSC_COEF_A2_LSB_CSC_COEF_A2_LSB_MASK        0x000000FF

/**
 * @desc: Color Space Converter Matrix A3 Coefficient Register MSB
 * Notes: - The coefficients used in the CSC matrix use only 15 bits for the internal computations
 * @bits: [7:0] Color Space Converter Matrix A3 Coefficient Register MSB
 */
#define CSC_COEF_A3_MSB         0x00004106
#define CSC_COEF_A3_MSB_CSC_COEF_A3_MSB_MASK        0x000000FF

/**
 * @desc: Color Space Converter Matrix A3 Coefficient Register LSB
 * Notes: - The coefficients used in the CSC matrix use only 15 bits for the internal computations
 * @bits: [7:0] Color Space Converter Matrix A3 Coefficient Register LSB
 */
#define CSC_COEF_A3_LSB         0x00004107
#define CSC_COEF_A3_LSB_CSC_COEF_A3_LSB_MASK        0x000000FF

/**
 * @desc: Color Space Converter Matrix A4 Coefficient Register MSB
 * Notes: - The coefficients used in the CSC matrix use only 15 bits for the internal computations
 * @bits: [7:0] Color Space Converter Matrix A4 Coefficient Register MSB
 */
#define CSC_COEF_A4_MSB         0x00004108
#define CSC_COEF_A4_MSB_CSC_COEF_A4_MSB_MASK        0x000000FF

/**
 * @desc: Color Space Converter Matrix A4 Coefficient Register LSB
 * Notes: - The coefficients used in the CSC matrix use only 15 bits for the internal computations
 * @bits: [7:0] Color Space Converter Matrix A4 Coefficient Register LSB
 */
#define CSC_COEF_A4_LSB         0x00004109
#define CSC_COEF_A4_LSB_CSC_COEF_A4_LSB_MASK        0x000000FF

/**
 * @desc: Color Space Converter Matrix B1 Coefficient Register MSB
 * Notes: - The coefficients used in the CSC matrix use only 15 bits for the internal computations
 * @bits: [7:0] Color Space Converter Matrix B1 Coefficient Register MSB
 */
#define CSC_COEF_B1_MSB         0x0000410A
#define CSC_COEF_B1_MSB_CSC_COEF_B1_MSB_MASK        0x000000FF

/**
 * @desc: Color Space Converter Matrix B1 Coefficient Register LSB
 * Notes: - The coefficients used in the CSC matrix use only 15 bits for the internal computations
 * @bits: [7:0] Color Space Converter Matrix B1 Coefficient Register LSB
 */
#define CSC_COEF_B1_LSB         0x0000410B
#define CSC_COEF_B1_LSB_CSC_COEF_B1_LSB_MASK        0x000000FF

/**
 * @desc: Color Space Converter Matrix B2 Coefficient Register MSB
 * Notes: - The coefficients used in the CSC matrix use only 15 bits for the internal computations
 * @bits: [7:0] Color Space Converter Matrix B2 Coefficient Register MSB
 */
#define CSC_COEF_B2_MSB         0x0000410C
#define CSC_COEF_B2_MSB_CSC_COEF_B2_MSB_MASK        0x000000FF

/**
 * @desc: Color Space Converter Matrix B2 Coefficient Register LSB
 * Notes: - The coefficients used in the CSC matrix use only 15 bits for the internal computations
 * @bits: [7:0] Color Space Converter Matrix B2 Coefficient Register LSB
 */
#define CSC_COEF_B2_LSB         0x0000410D
#define CSC_COEF_B2_LSB_CSC_COEF_B2_LSB_MASK        0x000000FF

/**
 * @desc: Color Space Converter Matrix B3 Coefficient Register MSB
 * Notes: - The coefficients used in the CSC matrix use only 15 bits for the internal computations
 * @bits: [7:0] Color Space Converter Matrix B3 Coefficient Register MSB
 */
#define CSC_COEF_B3_MSB         0x0000410E
#define CSC_COEF_B3_MSB_CSC_COEF_B3_MSB_MASK        0x000000FF

/**
 * @desc: Color Space Converter Matrix B3 Coefficient Register LSB
 * Notes: - The coefficients used in the CSC matrix use only 15 bits for the internal computations
 * @bits: [7:0] Color Space Converter Matrix B3 Coefficient Register LSB
 */
#define CSC_COEF_B3_LSB         0x0000410F
#define CSC_COEF_B3_LSB_CSC_COEF_B3_LSB_MASK        0x000000FF

/**
 * @desc: Color Space Converter Matrix B4 Coefficient Register MSB
 * Notes: - The coefficients used in the CSC matrix use only 15 bits for the internal computations
 * @bits: [7:0] Color Space Converter Matrix B4 Coefficient Register MSB
 */
#define CSC_COEF_B4_MSB         0x00004110
#define CSC_COEF_B4_MSB_CSC_COEF_B4_MSB_MASK        0x000000FF

/**
 * @desc: Color Space Converter Matrix B4 Coefficient Register LSB
 * Notes: - The coefficients used in the CSC matrix use only 15 bits for the internal computations
 * @bits: [7:0] Color Space Converter Matrix B4 Coefficient Register LSB
 */
#define CSC_COEF_B4_LSB         0x00004111
#define CSC_COEF_B4_LSB_CSC_COEF_B4_LSB_MASK        0x000000FF

/**
 * @desc: Color Space Converter Matrix C1 Coefficient Register MSB
 * Notes: - The coefficients used in the CSC matrix use only 15 bits for the internal computations
 * @bits: [7:0] Color Space Converter Matrix C1 Coefficient Register MSB
 */
#define CSC_COEF_C1_MSB         0x00004112
#define CSC_COEF_C1_MSB_CSC_COEF_C1_MSB_MASK        0x000000FF

/**
 * @desc: Color Space Converter Matrix C1 Coefficient Register LSB
 * Notes: - The coefficients used in the CSC matrix use only 15 bits for the internal computations
 * @bits: [7:0] Color Space Converter Matrix C1 Coefficient Register LSB
 */
#define CSC_COEF_C1_LSB         0x00004113
#define CSC_COEF_C1_LSB_CSC_COEF_C1_LSB_MASK        0x000000FF

/**
 * @desc: Color Space Converter Matrix C2 Coefficient Register MSB
 * Notes: - The coefficients used in the CSC matrix use only 15 bits for the internal computations
 * @bits: [7:0] Color Space Converter Matrix C2 Coefficient Register MSB
 */
#define CSC_COEF_C2_MSB         0x00004114
#define CSC_COEF_C2_MSB_CSC_COEF_C2_MSB_MASK        0x000000FF

/**
 * @desc: Color Space Converter Matrix C2 Coefficient Register LSB
 * Notes: - The coefficients used in the CSC matrix use only 15 bits for the internal computations
 * @bits: [7:0] Color Space Converter Matrix C2 Coefficient Register LSB
 */
#define CSC_COEF_C2_LSB         0x00004115
#define CSC_COEF_C2_LSB_CSC_COEF_C2_LSB_MASK        0x000000FF

/**
 * @desc: Color Space Converter Matrix C3 Coefficient Register MSB
 * Notes: - The coefficients used in the CSC matrix use only 15 bits for the internal computations
 * @bits: [7:0] Color Space Converter Matrix C3 Coefficient Register MSB
 */
#define CSC_COEF_C3_MSB         0x00004116
#define CSC_COEF_C3_MSB_CSC_COEF_C3_MSB_MASK        0x000000FF

/**
 * @desc: Color Space Converter Matrix C3 Coefficient Register LSB
 * Notes: - The coefficients used in the CSC matrix use only 15 bits for the internal computations
 * @bits: [7:0] Color Space Converter Matrix C3 Coefficient Register LSB
 */
#define CSC_COEF_C3_LSB         0x00004117
#define CSC_COEF_C3_LSB_CSC_COEF_C3_LSB_MASK        0x000000FF

/**
 * @desc: Color Space Converter Matrix C4 Coefficient Register MSB
 * Notes: - The coefficients used in the CSC matrix use only 15 bits for the internal computations
 * @bits: [7:0] Color Space Converter Matrix C4 Coefficient Register MSB
 */
#define CSC_COEF_C4_MSB         0x00004118
#define CSC_COEF_C4_MSB_CSC_COEF_C4_MSB_MASK        0x000000FF

/**
 * @desc: Color Space Converter Matrix C4 Coefficient Register LSB
 * Notes: - The coefficients used in the CSC matrix use only 15 bits for the internal computations
 * @bits: [7:0] Color Space Converter Matrix C3 Coefficient Register LSB
 */
#define CSC_COEF_C4_LSB         0x00004119
#define CSC_COEF_C4_LSB_CSC_COEF_C4_LSB_MASK        0x000000FF

/**
 * @desc: Color Space Converter Matrix Output Up Limit Register MSB
 * @bits: [7:0] Color Space Converter Matrix Output Upper Limit Register MSB
 */
#define CSC_LIMIT_UP_MSB        0x0000411A
#define CSC_LIMIT_UP_MSB_CSC_LIMIT_UP_MSB_MASK      0x000000FF

/**
 * @desc: Color Space Converter Matrix Output Up Limit Register LSB
 * @bits: [7:0] Color Space Converter Matrix Output Upper Limit Register LSB
 */
#define CSC_LIMIT_UP_LSB        0x0000411B
#define CSC_LIMIT_UP_LSB_CSC_LIMIT_UP_LSB_MASK      0x000000FF

/**
 * @desc: Color Space Converter Matrix output Down Limit Register MSB
 * @bits: [7:0] Color Space Converter Matrix output Down Limit Register MSB
 */
#define CSC_LIMIT_DN_MSB        0x0000411C
#define CSC_LIMIT_DN_MSB_CSC_LIMIT_DN_MSB_MASK      0x000000FF

/**
 * @desc: Color Space Converter Matrix output Down Limit Register LSB
 * @bits: [7:0] Color Space Converter Matrix output Down Limit Register LSB
 */
#define CSC_LIMIT_DN_LSB        0x0000411D
#define CSC_LIMIT_DN_LSB_CSC_LIMIT_DN_LSB_MASK      0x000000FF

#endif  /* _DW_AVP_H_ */
