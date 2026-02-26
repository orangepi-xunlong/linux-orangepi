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
#ifndef _DW_FRAME_COMPOSER_H
#define _DW_FRAME_COMPOSER_H

typedef struct fc_spd_info {
    const u8 *vName;
    u8 vLength;
    const u8 *pName;
    u8 pLength;
    u8 code;
} fc_spd_info_t;

typedef struct channel_count {
    unsigned char channel_allocation;
    unsigned char channel_count;
} channel_count_t;

typedef union iec {
    u32 frequency;
    u8 sample_size;
} iec_t;

typedef struct iec_sampling_freq {
    iec_t iec;
    u8 value;
} iec_params_t;

/**
 * @desc: set audio mute state
*/
void fc_audio_mute(struct dw_hdmi_dev_s *dev);

/**
 * @desc: get audio current mute state
*/
u8 dw_fc_audio_get_mute(void);

/**
 * @desc: set audio infoframe config
*/
void dw_fc_audio_packet_config(struct dw_audio_s *audio);

/**
 * @desc: set audio config
 */
void dw_fc_audio_sample_config(struct dw_audio_s *audio);
/**
 * @desc: dw video get hactive value
 * @return: hardware hactive value
 */
u32 dw_fc_video_get_hactive(void);
/**
 * @desc: dw video get vactive value
 * @return: hardware vactive value
 */
u32 dw_fc_video_get_vactive(void);
/**
 * @desc: dw fc video set force output
 * @bits: 1 - enable force video output
 *        0 - disable force video output
 * @value: video output rgb value. R || G || B
*/
void dw_fc_video_force_value(u8 bit, u32 value);

/**
 * @desc: set video hdcp keepout
*/
void dw_fc_video_set_hdcp_keepout(u8 bit);

/**
 * @desc: set video config
*/
int dw_fc_video_config(struct dw_video_s *video);

/**
 * @desc: get video current mode
 * @return: 1 - hdmi mode
 *          0 - dvi mode
*/
u8 dw_fc_video_get_tmds_mode(void);

/**
 * @desc: video config scramble
 * @state: 1 - enable scramble
 *         0 - disable scramble
*/
void dw_fc_video_set_scramble(u8 state);

/**
 * @desc: video get current scramble state
 * @return: 1 - scramble is enable
 *          0 - scramble is disable
*/
u8 dw_fc_video_get_scramble(void);

u8 dw_fc_video_get_hsync_polarity(void);

u8 dw_fc_video_get_vsync_polarity(void);

/**
 * @desc: packets configure is the same as infoframe configure
*/
int dw_infoframe_packet(void);

/**
 * @desc: packet config set avmute
*/
void dw_gcp_set_avmute(u8 enable);

/**
 * @desc: get current avmute state
*/
u8 dw_gcp_get_avmute(void);

int dw_drm_packet_filling_data(dw_fc_drm_pb_t *data);

void dw_drm_packet_clear(dw_fc_drm_pb_t *pb);

u8 dw_avi_get_rgb_ycc(void);

u8 dw_avi_get_video_code(void);

u32 dw_fc_audio_get_sample_freq(void);

u8 dw_fc_audio_get_word_length(void);

void dw_fc_audio_set_mute(u8 state);

int dw_fc_iteration_process(void);

/*****************************************************************************
 *                                                                           *
 *                          Frame Composer Registers                         *
 *                                                                           *
 *****************************************************************************/

/**
 * @desc: Frame Composer Input Video Configuration and HDCP Keepout Register
 * @bits: [0:0] Input video mode
 *          - 1: Interlaced
 *          - 0: Progressive
 *        [1:1] Used for CEA861-D modes with fractional Vblank (for example, modes 5, 6, 7, 10, 11, 20, 21, and 22)
 *        [3:3] TMDS mode
 *          - 0: DVI mode selected
 *          - 1: HDMI mode selected
 *        [4:4] Data enable input polarity
 *          - 1: Active high
 *          - 0: Active low
 *        [5:5] Hsync input polarity
 *          - 1: Active high
 *          - 0: Active low
 *        [6:6] Vsync input polarity
 *          - 1: Active high
 *          - 0: Active low
 *        [7:7] Start/stop HDCP keepout window generation
 *          - 1: Start
 *          - 0: Stop
 */
#define FC_INVIDCONF            0x00001000
#define FC_INVIDCONF_IN_I_P_MASK                0x00000001
#define FC_INVIDCONF_R_V_BLANK_IN_OSC_MASK      0x00000002
#define FC_INVIDCONF_DVI_MODEZ_MASK             0x00000008
#define FC_INVIDCONF_DE_IN_POLARITY_MASK        0x00000010
#define FC_INVIDCONF_HSYNC_IN_POLARITY_MASK     0x00000020
#define FC_INVIDCONF_VSYNC_IN_POLARITY_MASK     0x00000040
#define FC_INVIDCONF_HDCP_KEEPOUT_MASK          0x00000080

/**
 * @desc: Frame Composer Input Video HActive Pixels Register 0
 * @bits: [7:0] Input video Horizontal active pixel region width
 */
#define FC_INHACTIV0            0x00001001
#define FC_INHACTIV0_H_IN_ACTIV_MASK            0x000000FF

/**
 * @desc: Frame Composer Input Video HActive Pixels Register 1
 * @bits: [3:0] Input video Horizontal active pixel region width
 *        [4:4] Input video Horizontal active pixel region width (0...8191)
 *        [5:5] Input video Horizontal active pixel region width (0...16383)
 */
#define FC_INHACTIV1            0x00001002
#define FC_INHACTIV1_H_IN_ACTIV_MASK            0x0000000F
#define FC_INHACTIV1_H_IN_ACTIV_12_MASK         0x00000010
#define FC_INHACTIV1_H_IN_ACTIV_13_MASK         0x00000020

/**
 * @desc: Frame Composer Input Video HBlank Pixels Register 0
 * @bits: [7:0] Input video Horizontal blanking pixel region width
 */
#define FC_INHBLANK0            0x00001003
#define FC_INHBLANK0_H_IN_BLANK_MASK            0x000000FF

/**
 * @desc: Frame Composer Input Video HBlank Pixels Register 1
 * @bits: [1:0] Input video Horizontal blanking pixel region width
 *              this bit field holds bits 9:8 of number of Horizontal blanking pixels
 *        [4:2] Input video Horizontal blanking pixel region width
 *              If configuration parameter DWC_HDMI_TX_14 = True (1),
 *              this bit field holds bit 12:10 of number of horizontal blanking pixels
 */
#define FC_INHBLANK1            0x00001004
#define FC_INHBLANK1_H_IN_BLANK_MASK            0x00000003
#define FC_INHBLANK1_H_IN_BLANK_12_MASK         0x0000001C

/**
 * @desc: Frame Composer Input Video VActive Pixels Register 0
 * @bits: [7:0] Input video Vertical active pixel region width
 */
#define FC_INVACTIV0            0x00001005
#define FC_INVACTIV0_V_IN_ACTIV_MASK            0x000000FF

/**
 * @desc: Frame Composer Input Video VActive Pixels Register 1
 * @bits: [2:0] Input video Vertical active pixel region width
 *        [4:3] Input video Vertical active pixel region width
 */
#define FC_INVACTIV1            0x00001006
#define FC_INVACTIV1_V_IN_ACTIV_MASK            0x00000007
#define FC_INVACTIV1_V_IN_ACTIV_12_11_MASK      0x00000018
#define FC_INVACTIV1_V_IN_ACTIV_HIGHT_MASK      0x0000001F

/**
 * @desc: Frame Composer Input Video VBlank Pixels Register
 * @bits: [7:0] Input video Vertical blanking pixel region width
 *  */
#define FC_INVBLANK             0x00001007
#define FC_INVBLANK_V_IN_BLANK_MASK             0x000000FF

/**
 * @desc: Frame Composer Input Video HSync Front Porch Register 0
 * @bits: [7:0] Input video Hsync active edge delay
 */
#define FC_HSYNCINDELAY0        0x00001008
#define FC_HSYNCINDELAY0_H_IN_DELAY_MASK        0x000000FF

/**
 * @desc: Frame Composer Input Video HSync Front Porch Register 1
 * @bits: [2:0] Input video Horizontal active edge delay
 *        [4:3] Input video Horizontal active edge delay
 */
#define FC_HSYNCINDELAY1        0x00001009
#define FC_HSYNCINDELAY1_H_IN_DELAY_MASK        0x00000007
#define FC_HSYNCINDELAY1_H_IN_DELAY_12_MASK     0x00000018

/**
 * @desc: Frame Composer Input Video HSync Width Register 0
 * @bits: [7:0] Input video Hsync active pulse width
 */
#define FC_HSYNCINWIDTH0        0x0000100A
#define FC_HSYNCINWIDTH0_H_IN_WIDTH_MASK        0x000000FF

/**
 * @desc: Frame Composer Input Video HSync Width Register 1
 * @bits: [0:0] Input video Hsync active pulse width
 *        [1:1] Input video Hsync active pulse width
 */
#define FC_HSYNCINWIDTH1        0x0000100B
#define FC_HSYNCINWIDTH1_H_IN_WIDTH_MASK        0x00000001
#define FC_HSYNCINWIDTH1_H_IN_WIDTH_9_MASK      0x00000002

/**
 * @desc: Frame Composer Input Video VSync Front Porch Register
 * @bits: [7:0] Input video Vsync active edge delay
 */
#define FC_VSYNCINDELAY         0x0000100C
#define FC_VSYNCINDELAY_V_IN_DELAY_MASK         0x000000FF

/**
 * @desc: Frame Composer Input Video VSync Width Register
 * @bits: [5:0] Input video Vsync active pulse width
 */
#define FC_VSYNCINWIDTH         0x0000100D
#define FC_VSYNCINWIDTH_V_IN_WIDTH_MASK         0x0000003F

/**
 * @desc: Frame Composer Input Video Refresh Rate Register 0
 * @bits: [7:0] Video refresh rate in Hz*1E3 format
 */
#define FC_INFREQ0              0x0000100E
#define FC_INFREQ0_INFREQ_MASK                  0x000000FF

/**
 * @desc: Frame Composer Input Video Refresh Rate Register 1
 * @bits: [7:0] Video refresh rate in Hz*1E3 format
 */
#define FC_INFREQ1              0x0000100F
#define FC_INFREQ1_INFREQ_MASK                  0x000000FF

/**
 * @desc: Frame Composer Input Video Refresh Rate Register 2
 * @bits: [3:0] Video refresh rate in Hz*1E3 format
 */
#define FC_INFREQ2              0x00001010
#define FC_INFREQ2_INFREQ_MASK                  0x0000000F

/**
 * @desc: Frame Composer Control Period Duration Register
 * @bits: [7:0] Configuration of the control period minimum duration
 */
#define FC_CTRLDUR              0x00001011
#define FC_CTRLDUR_CTRLPERIODDURATION_MASK      0x000000FF

/**
 * @desc: Frame Composer Extended Control Period Duration Register
 * @bits: [7:0] Configuration of the extended control period minimum duration
 */
#define FC_EXCTRLDUR            0x00001012
#define FC_EXCTRLDUR_EXCTRLPERIODDURATION_MASK  0x000000FF

/**
 * @desc: Frame Composer Extended Control Period Maximum Spacing Register
 * @bits: [7:0] Configuration of the maximum spacing between consecutive extended control periods
 */
#define FC_EXCTRLSPAC           0x00001013
#define FC_EXCTRLSPAC_EXCTRLPERIODSPACING_MASK  0x000000FF

/**
 * @desc: Frame Composer Channel 0 Non-Preamble Data Register
 * @bits: [7:0] When in control mode, configures 8 bits that fill the channel 0
 *              data lines not used to transmit the preamble
 */
#define FC_CH0PREAM             0x00001014
#define FC_CH0PREAM_CH0_PREAMBLE_FILTER_MASK    0x000000FF

/**
 * @desc: Frame Composer Channel 1 Non-Preamble Data Register
 * @bits: [5:0] When in control mode, configures 6 bits that fill the channel 1
 *              data lines not used to transmit the preamble
 */
#define FC_CH1PREAM             0x00001015
#define FC_CH1PREAM_CH1_PREAMBLE_FILTER_MASK    0x0000003F

/**
 * @desc: Frame Composer Channel 2 Non-Preamble Data Register
 * @bits: [5:0] When in control mode, configures 6 bits that fill the channel 2
 *              data lines not used to transmit the preamble
 */
#define FC_CH2PREAM             0x00001016
#define FC_CH2PREAM_CH2_PREAMBLE_FILTER_MASK    0x0000003F

/**
 * @desc: Frame Composer AVI Packet Configuration Register 3
 * @bits: [1:0] IT content type according to CEA the specification
 *        [3:2] YCC Quantization range according to the CEA specification
 */
#define FC_AVICONF3             0x00001017
#define FC_AVICONF3_CN_MASK                     0x00000003
#define FC_AVICONF3_YQ_MASK                     0x0000000C

/**
 * @desc: Frame Composer GCP Packet Configuration Register
 * @bits: [0:0] Value of "clear_avmute" in the GCP packet
 *        [1:1] Value of "set_avmute" in the GCP packet Once the AVmute is set,
 *              the frame composer schedules the GCP packet with AVmute set in the packet scheduler to be sent once
 *        [2:2] Value of "default_phase" in the GCP packet
 */
#define FC_GCP                  0x00001018
#define FC_GCP_CLEAR_AVMUTE_MASK                0x00000001
#define FC_GCP_SET_AVMUTE_MASK                  0x00000002
#define FC_GCP_DEFAULT_PHASE_MASK               0x00000004

/**
 * @desc: Frame Composer AVI Packet Configuration Register 0
 * @bits: [1:0] Y1,Y0 RGB or YCC indicator
 *        [3:2] Bar information data valid
 *        [5:4] Scan information
 *        [6:6] Active format present
 *        [7:7] Y2, Bit 2 of rgc_ycc_indication
 */
#define FC_AVICONF0             0x00001019
#define FC_AVICONF0_RGC_YCC_INDICATION_MASK     0x00000003
#define FC_AVICONF0_BAR_INFORMATION_MASK        0x0000000C
#define FC_AVICONF0_SCAN_INFORMATION_MASK       0x00000030
#define FC_AVICONF0_ACTIVE_FORMAT_PRESENT_MASK  0x00000040
#define FC_AVICONF0_RGC_YCC_INDICATION_2_MASK   0x00000080

/**
 * @desc: Frame Composer AVI Packet Configuration Register 1
 * @bits: [3:0] Active aspect ratio
 *        [5:4] Picture aspect ratio
 *        [7:6] Colorimetry
 */
#define FC_AVICONF1             0x0000101A
#define FC_AVICONF1_ACTIVE_ASPECT_RATIO_MASK    0x0000000F
#define FC_AVICONF1_PICTURE_ASPECT_RATIO_MASK   0x00000030
#define FC_AVICONF1_COLORIMETRY_MASK            0x000000C0

/**
 * @desc: Frame Composer AVI Packet Configuration Register 2
 * @bits: [1:0] Non-uniform picture scaling
 *        [3:2] Quantization range
 *        [6:4] Extended colorimetry
 *        [7:7] IT content
 */
#define FC_AVICONF2             0x0000101B
#define FC_AVICONF2_NON_UNIFORM_PICTURE_SCALING_MASK    0x00000003
#define FC_AVICONF2_QUANTIZATION_RANGE_MASK             0x0000000C
#define FC_AVICONF2_EXTENDED_COLORIMETRY_MASK           0x00000070
#define FC_AVICONF2_IT_CONTENT_MASK                     0x00000080

/**
 * @desc: Frame Composer AVI Packet VIC Register
 * @bits: [6:0] Configures the AVI InfoFrame Video Identification code
 *        [7:7] Bit 7 of fc_avivid register
 */
#define FC_AVIVID               0x0000101C
#define FC_AVIVID_FC_AVIVID_MASK                0x0000007F
#define FC_AVIVID_FC_AVIVID_7_MASK              0x00000080

/**
 * @desc: Frame Composer AVI Packet End of Top Bar Register
 * @bits: [7:0] Defines the AVI Infoframe End of Top Bar Value
*/
#define FC_AVIETB0              0x0000101D
#define FC_AVIETB_MAKSK                         0x000000FF
#define FC_AVIETB1              0x0000101F
#define FC_AVISBB0              0x0000101E
#define FC_AVISBB1              0x00001020
#define FC_AVIELB0              0x00001021
#define FC_AVIELB1              0x00001022
#define FC_AVISRB0              0x00001023
#define FC_AVISRB1              0x00001024

/**
 * @desc: Frame Composer AUD Packet Configuration Register 0
 * @bits: [3:0] Coding Type
 *        [6:4] Channel count
 */
#define FC_AUDICONF0            0x00001025
#define FC_AUDICONF0_CT_MASK                    0x0000000F
#define FC_AUDICONF0_CC_MASK                    0x00000070

/**
 * @desc: Frame Composer AUD Packet Configuration Register 1
 * @bits: [2:0] Sampling frequency
 *        [5:4] Sampling size
 */
#define FC_AUDICONF1            0x00001026
#define FC_AUDICONF1_SF_MASK                    0x00000007
#define FC_AUDICONF1_SS_MASK                    0x00000030

/**
 * @desc: Frame Composer AUD Packet Configuration Register 2
 * @bits: [7:0] Channel allocation
 */
#define FC_AUDICONF2            0x00001027
#define FC_AUDICONF2_CA_MASK                    0x000000FF

/**
 * @desc: Frame Composer AUD Packet Configuration Register 0
 * @bits: [3:0] Level shift value (for down mixing)
 *        [4:4] Down mix enable
 *        [6:5] LFE playback information LFEPBL1, LFEPBL0 LFE playback level as compared to the other channels
 */
#define FC_AUDICONF3            0x00001028
#define FC_AUDICONF3_LSV_MASK                   0x0000000F
#define FC_AUDICONF3_DM_INH_MASK                0x00000010
#define FC_AUDICONF3_LFEPBL_MASK                0x00000060

/**
 * @desc: Frame Composer VSI Packet Data IEEE Register 0
 * @bits: [7:0] Configures the Vendor Specific InfoFrame IEEE registration identifier
 */
#define FC_VSDIEEEID0           0x00001029
#define FC_VSDIEEEID0_IEEE_MASK                 0x000000FF

/**
 * @desc: Frame Composer VSI Packet Data Size Register
 * @bits: [7:0] Packet size as described in the HDMI Vendor Specific InfoFrame
 */
#define FC_VSDSIZE              0x0000102A
#define FC_VSDSIZE_VSDSIZE_MASK                 0x0000001F

/**
 * @desc: Frame Composer VSI Packet Data IEEE Register 1
 * @bits: [7:0] Configures the Vendor Specific InfoFrame IEEE registration identifier
 */
#define FC_VSDIEEEID1           0x00001030
#define FC_VSDIEEEID1_IEEE_MASK                 0x000000FF

/**
 * @desc: Frame Composer VSI Packet Data IEEE Register 2
 * @bits: [7:0] Configures the Vendor Specific InfoFrame IEEE registration identifier
 */
#define FC_VSDIEEEID2           0x00001031
#define FC_VSDIEEEID2_IEEE_MASK                 0x000000FF

/**
 * @desc: Frame Composer VSI Packet Data Payload Register Array
 * @bits: [7:0] Configures the Vendor Specific infoframe 24 bytes specific payload
*/
#define FC_VSDPAYLOAD0          0x00001032
#define FC_VSDPAYLOAD_MASK                      0x000000FF

/**
 * @desc: Frame Composer SPD Packet Data Vendor Name Register Array
 * @bits: [7:0] Configures the Source Product Descriptior infoFrame 8 bytes Vendor name
*/
#define FC_SPDVENDORNAME0       0x0000104A
#define FC_SPDVENDORNAME_MASK                   0x000000FF

/**
 * @desc: Frame Composer SPD Packet Data Product Name Register Array
 * @bits: [7:0] Configures the Source Product Descriptior infoFrame 16 bytes Product name
*/
#define FC_SPDPRODUCTNAME0      0x00001052
#define FC_SPDPRODUCTNAME_MASK                  0x000000FF

/**
 * @desc: Frame Composer SPD Packet Data Source Product Descriptor Register
 * @bits: [7:0] Frame Composer SPD Packet Data Source Product Descriptor Register
 */
#define FC_SPDDEVICEINF         0x00001062
#define FC_SPDDEVICEINF_FC_SPDDEVICEINF_MASK    0x000000FF

/**
 * @desc: Frame Composer Audio Sample Flat and Layout Configuration Register
 * @bits: [0:0] Set the audio packet layout to be sent in the packet
 *          - 1: layout 1
 *          - 0: layout 0
 *        [7:4] Set the audio packet sample flat value to be sent on the packet
 */
#define FC_AUDSCONF             0x00001063
#define FC_AUDSCONF_AUD_PACKET_LAYOUT_MASK      0x00000001
#define FC_AUDSCONF_AUD_PACKET_SAMPFLT_MASK     0x000000F0

/**
 * @desc: Frame Composer Audio Sample Flat and Layout Configuration Register
 * @bits: [3:0] Shows the data sample present indication of the last Audio sample packet sent by the HDMI TX Controller
 */
#define FC_AUDSSTAT             0x00001064
#define FC_AUDSSTAT_PACKET_SAMPPRS_MASK         0x0000000F

/**
 * @desc: Frame Composer Audio Sample Validity Flag Register
 * @bits: [0:0] Set validity bit "V" for Channel 0, Left
 *        [1:1] Set validity bit "V" for Channel 1, Left
 *        [2:2] Set validity bit "V" for Channel 2, Left
 *        [3:3] Set validity bit "V" for Channel 3, Left
 *        [4:4] Set validity bit "V" for Channel 0, Right
 *        [5:5] Set validity bit "V" for Channel 1, Right
 *        [6:6] Set validity bit "V" for Channel 2, Right
 *        [7:7] Set validity bit "V" for Channel 3, Right
 */
#define FC_AUDSV                0x00001065
#define FC_AUDSV_V0L_MASK                       0x00000001
#define FC_AUDSV_V1L_MASK                       0x00000002
#define FC_AUDSV_V2L_MASK                       0x00000004
#define FC_AUDSV_V3L_MASK                       0x00000008
#define FC_AUDSV_V0R_MASK                       0x00000010
#define FC_AUDSV_V1R_MASK                       0x00000020
#define FC_AUDSV_V2R_MASK                       0x00000040
#define FC_AUDSV_V3R_MASK                       0x00000080

/**
 * @desc: Frame Composer Audio Sample User Flag Register
 * @bits: [0:0] Set user bit "U" for Channel 0, Left
 *        [1:1] Set user bit "U" for Channel 1, Left
 *        [2:2] Set user bit "U" for Channel 2, Left
 *        [3:3] Set user bit "U" for Channel 3, Left
 *        [4:4] Set user bit "U" for Channel 0, Right
 *        [5:5] Set user bit "U" for Channel 1, Right
 *        [6:6] Set user bit "U" for Channel 2, Right
 *        [7:7] Set user bit "U" for Channel 3, Right
 */
#define FC_AUDSU                0x00001066
#define FC_AUDSU_U0L_MASK                       0x00000001
#define FC_AUDSU_U1L_MASK                       0x00000002
#define FC_AUDSU_U2L_MASK                       0x00000004
#define FC_AUDSU_U3L_MASK                       0x00000008
#define FC_AUDSU_U0R_MASK                       0x00000010
#define FC_AUDSU_U1R_MASK                       0x00000020
#define FC_AUDSU_U2R_MASK                       0x00000040
#define FC_AUDSU_U3R_MASK                       0x00000080

/**
 * @desc: Frame Composer Audio Sample Channel Status Configuration Register 0
 * @bits: [0:0] IEC Copyright indication
 *        [5:4] CGMS-A
 */
#define FC_AUDSCHNL0            0x00001067
#define FC_AUDSCHNL0_OIEC_COPYRIGHT_MASK        0x00000001
#define FC_AUDSCHNL0_OIEC_CGMSA_MASK            0x00000030

/**
 * @desc: Frame Composer Audio Sample Channel Status Configuration Register 1
 * @bits: [7:0] Category code
 */
#define FC_AUDSCHNL1            0x00001068
#define FC_AUDSCHNL1_OIEC_CATEGORYCODE_MASK     0x000000FF

/**
 * @desc: Frame Composer Audio Sample Channel Status Configuration Register 2
 * @bits: [3:0] Source number
 *        [6:4] PCM audio mode
 */
#define FC_AUDSCHNL2            0x00001069
#define FC_AUDSCHNL2_OIEC_SOURCENUMBER_MASK     0x0000000F
#define FC_AUDSCHNL2_OIEC_PCMAUDIOMODE_MASK     0x00000070

/**
 * @desc: Frame Composer Audio Sample Channel Status Configuration Register 3
 * @bits: [3:0] Channel number for first right sample
 *        [7:4] Channel number for second right sample
 */
#define FC_AUDSCHNL3            0x0000106A
#define FC_AUDSCHNL3_OIEC_CHANNELNUMCR0_MASK    0x0000000F
#define FC_AUDSCHNL3_OIEC_CHANNELNUMCR1_MASK    0x000000F0

/**
 * @desc: Frame Composer Audio Sample Channel Status Configuration Register 4
 * @bits: [3:0] Channel number for third right sample
 *        [7:4] Channel number for fourth right sample
 */
#define FC_AUDSCHNL4            0x0000106B
#define FC_AUDSCHNL4_OIEC_CHANNELNUMCR2_MASK    0x0000000F
#define FC_AUDSCHNL4_OIEC_CHANNELNUMCR3_MASK    0x000000F0

/**
 * @desc: Frame Composer Audio Sample Channel Status Configuration Register 5
 * @bits: [3:0] Channel number for first left sample
 *        [7:4] Channel number for second left sample
 */
#define FC_AUDSCHNL5            0x0000106C
#define FC_AUDSCHNL5_OIEC_CHANNELNUMCL0_MASK    0x0000000F
#define FC_AUDSCHNL5_OIEC_CHANNELNUMCL1_MASK    0x000000F0

/**
 * @desc: Frame Composer Audio Sample Channel Status Configuration Register 6
 * @bits: [3:0] Channel number for third left sample
 *        [7:4] Channel number for fourth left sample
 */
#define FC_AUDSCHNL6            0x0000106D
#define FC_AUDSCHNL6_OIEC_CHANNELNUMCL2_MASK    0x0000000F
#define FC_AUDSCHNL6_OIEC_CHANNELNUMCL3_MASK    0x000000F0

/**
 * @desc: Frame Composer Audio Sample Channel Status Configuration Register 7
 * @bits: [3:0] Sampling frequency
 *        [5:4] Clock accuracy
 *        [7:6] Sampling frequency (channel status bits 31 and 30)
 */
#define FC_AUDSCHNL7            0x0000106E
#define FC_AUDSCHNL7_OIEC_SAMPFREQ_MASK         0x0000000F
#define FC_AUDSCHNL7_OIEC_CLKACCURACY_MASK      0x00000030
#define FC_AUDSCHNL7_OIEC_SAMPFREQ_EXT_MASK     0x000000C0

/**
 * @desc: Frame Composer Audio Sample Channel Status Configuration Register 8
 * @bits: [3:0] Word length configuration
 *        [7:4] Original sampling frequency
 */
#define FC_AUDSCHNL8            0x0000106F
#define FC_AUDSCHNL8_OIEC_WORDLENGTH_MASK       0x0000000F
#define FC_AUDSCHNL8_OIEC_ORIGSAMPFREQ_MASK     0x000000F0

/**
 * @desc: Frame Composer Number of High Priority Packets Attended Configuration Register
 * @bits: [4:0] Configures the number of high priority packets or audio sample
 *              packets consecutively attended before checking low priority queue status
 */
#define FC_CTRLQHIGH            0x00001073
#define FC_CTRLQHIGH_ONHIGHATTENDED_MASK        0x0000001F

/**
 * @desc: Frame Composer Number of Low Priority Packets Attended Configuration Register
 * @bits: [4:0] Configures the number of low priority packets or null packets
 *              consecutively attended before checking high priority queue status or audio samples availability
 */
#define FC_CTRLQLOW             0x00001074
#define FC_CTRLQLOW_ONLOWATTENDED_MASK          0x0000001F

/**
 * @desc: Frame Composer ACP Packet Type Configuration Register 0
 * @bits: [7:0] Configures the ACP packet type
 */
#define FC_ACP0                 0x00001075
#define FC_ACP0_ACPTYPE_MASK                    0x000000FF

/**
 * @desc: Frame Composer ACP Packet Body Configuration Register 16
 * @bits: [7:0] Frame Composer ACP Packet Body Configuration Register 16
 */
#define FC_ACP16                0x00001082
#define FC_ACP16_FC_ACP16_MASK                  0x000000FF

/**
 * @desc: Frame Composer ACP Packet Body Configuration Register 15
 * @bits: [7:0] Frame Composer ACP Packet Body Configuration Register 15
 */
#define FC_ACP15                0x00001083
#define FC_ACP15_FC_ACP15_MASK                  0x000000FF

/**
 * @desc: Frame Composer ACP Packet Body Configuration Register 14
 * @bits: [7:0] Frame Composer ACP Packet Body Configuration Register 14
 */
#define FC_ACP14                0x00001084
#define FC_ACP14_FC_ACP14_MASK                  0x000000FF

/**
 * @desc: Frame Composer ACP Packet Body Configuration Register 13
 * @bits: [7:0] Frame Composer ACP Packet Body Configuration Register 13
 */
#define FC_ACP13                0x00001085
#define FC_ACP13_FC_ACP13_MASK                  0x000000FF

/**
 * @desc: Frame Composer ACP Packet Body Configuration Register 12
 * @bits: [7:0] Frame Composer ACP Packet Body Configuration Register 12
 */
#define FC_ACP12                0x00001086
#define FC_ACP12_FC_ACP12_MASK                  0x000000FF

/**
 * @desc: Frame Composer ACP Packet Body Configuration Register 11
 * @bits: [7:0] Frame Composer ACP Packet Body Configuration Register 11
 */
#define FC_ACP11                0x00001087
#define FC_ACP11_FC_ACP11_MASK                  0x000000FF

/**
 * @desc: Frame Composer ACP Packet Body Configuration Register 10
 * @bits: [7:0] Frame Composer ACP Packet Body Configuration Register 10
 */
#define FC_ACP10                0x00001088
#define FC_ACP10_FC_ACP10_MASK                  0x000000FF

/**
 * @desc: Frame Composer ACP Packet Body Configuration Register 9
 * @bits: [7:0] Frame Composer ACP Packet Body Configuration Register 9
 */
#define FC_ACP9                 0x00001089
#define FC_ACP9_FC_ACP9_MASK                    0x000000FF

/**
 * @desc: Frame Composer ACP Packet Body Configuration Register 8
 * @bits: [7:0] Frame Composer ACP Packet Body Configuration Register 8
 */
#define FC_ACP8                 0x0000108A
#define FC_ACP8_FC_ACP8_MASK                    0x000000FF

/**
 * @desc: Frame Composer ACP Packet Body Configuration Register 7
 * @bits: [7:0] Frame Composer ACP Packet Body Configuration Register 7
 */
#define FC_ACP7                 0x0000108B
#define FC_ACP7_FC_ACP7_MASK                    0x000000FF

/**
 * @desc: Frame Composer ACP Packet Body Configuration Register 6
 * @bits: [7:0] Frame Composer ACP Packet Body Configuration Register 6
 */
#define FC_ACP6                 0x0000108C
#define FC_ACP6_FC_ACP6_MASK                    0x000000FF

/**
 * @desc: Frame Composer ACP Packet Body Configuration Register 5
 * @bits: [7:0] Frame Composer ACP Packet Body Configuration Register 5
 */
#define FC_ACP5                 0x0000108D
#define FC_ACP5_FC_ACP5_MASK                    0x000000FF

/**
 * @desc: Frame Composer ACP Packet Body Configuration Register 4
 * @bits: [7:0] Frame Composer ACP Packet Body Configuration Register 4
 */
#define FC_ACP4                 0x0000108E
#define FC_ACP4_FC_ACP4_MASK                    0x000000FF

/**
 * @desc: Frame Composer ACP Packet Body Configuration Register 3
 * @bits: [7:0] Frame Composer ACP Packet Body Configuration Register 3
 */
#define FC_ACP3                 0x0000108F
#define FC_ACP3_FC_ACP3_MASK                    0x000000FF

/**
 * @desc: Frame Composer ACP Packet Body Configuration Register 2
 * @bits: [7:0] Frame Composer ACP Packet Body Configuration Register 2
 */
#define FC_ACP2                 0x00001090
#define FC_ACP2_FC_ACP2_MASK                    0x000000FF

/**
 * @desc: Frame Composer ACP Packet Body Configuration Register 1
 * @bits: [7:0] Frame Composer ACP Packet Body Configuration Register 1
 */
#define FC_ACP1                 0x00001091
#define FC_ACP1_FC_ACP1_MASK                    0x000000FF

/**
 * @desc: Frame Composer ISRC1 Packet Status, Valid, and Continue Configuration Register
 * @bits: [0:0] ISRC1 Indication of packet continuation (ISRC2 will be transmitted)
 *        [1:1] ISRC1 Valid control signal
 *        [4:2] ISRC1 Status signal
 */
#define FC_ISCR1_0              0x00001092
#define FC_ISCR1_0_ISRC_CONT_MASK               0x00000001
#define FC_ISCR1_0_ISRC_VALID_MASK              0x00000002
#define FC_ISCR1_0_ISRC_STATUS_MASK             0x0000001C

/**
 * @desc: Frame Composer ISRC1 Packet Body Register 16
 * @bits: [7:0] Frame Composer ISRC1 Packet Body Register 16
 */
#define FC_ISCR1_16             0x00001093
#define FC_ISCR1_16_FC_ISCR1_16_MASK            0x000000FF

/**
 * @desc: Frame Composer ISRC1 Packet Body Register 15
 * @bits: [7:0] Frame Composer ISRC1 Packet Body Register 15
 */
#define FC_ISCR1_15             0x00001094
#define FC_ISCR1_15_FC_ISCR1_15_MASK            0x000000FF

/**
 * @desc: Frame Composer ISRC1 Packet Body Register 14
 * @bits: [7:0] Frame Composer ISRC1 Packet Body Register 14
 */
#define FC_ISCR1_14             0x00001095
#define FC_ISCR1_14_FC_ISCR1_14_MASK            0x000000FF

/**
 * @desc: Frame Composer ISRC1 Packet Body Register 13
 * @bits: [7:0] Frame Composer ISRC1 Packet Body Register 13
 */
#define FC_ISCR1_13             0x00001096
#define FC_ISCR1_13_FC_ISCR1_13_MASK            0x000000FF

/**
 * @desc: Frame Composer ISRC1 Packet Body Register 12
 * @bits: [7:0] Frame Composer ISRC1 Packet Body Register 12
 */
#define FC_ISCR1_12             0x00001097
#define FC_ISCR1_12_FC_ISCR1_12_MASK            0x000000FF

/**
 * @desc: Frame Composer ISRC1 Packet Body Register 11
 * @bits: [7:0] Frame Composer ISRC1 Packet Body Register 11
 */
#define FC_ISCR1_11             0x00001098
#define FC_ISCR1_11_FC_ISCR1_11_MASK            0x000000FF

/**
 * @desc: Frame Composer ISRC1 Packet Body Register 10
 * @bits: [7:0] Frame Composer ISRC1 Packet Body Register 10
 */
#define FC_ISCR1_10             0x00001099
#define FC_ISCR1_10_FC_ISCR1_10_MASK            0x000000FF

/**
 * @desc: Frame Composer ISRC1 Packet Body Register 9
 * @bits: [7:0] Frame Composer ISRC1 Packet Body Register 9
 */
#define FC_ISCR1_9              0x0000109A
#define FC_ISCR1_9_FC_ISCR1_9_MASK              0x000000FF

/**
 * @desc: Frame Composer ISRC1 Packet Body Register 8
 * @bits: [7:0] Frame Composer ISRC1 Packet Body Register 8
 */
#define FC_ISCR1_8              0x0000109B
#define FC_ISCR1_8_FC_ISCR1_8_MASK              0x000000FF

/**
 * @desc: Frame Composer ISRC1 Packet Body Register 7
 * @bits: [7:0] Frame Composer ISRC1 Packet Body Register 7
 */
#define FC_ISCR1_7              0x0000109C
#define FC_ISCR1_7_FC_ISCR1_7_MASK              0x000000FF

/**
 * @desc: Frame Composer ISRC1 Packet Body Register 6
 * @bits: [7:0] Frame Composer ISRC1 Packet Body Register 6
 */
#define FC_ISCR1_6              0x0000109D
#define FC_ISCR1_6_FC_ISCR1_6_MASK              0x000000FF

/**
 * @desc: Frame Composer ISRC1 Packet Body Register 5
 * @bits: [7:0] Frame Composer ISRC1 Packet Body Register 5
 */
#define FC_ISCR1_5              0x0000109E
#define FC_ISCR1_5_FC_ISCR1_5_MASK              0x000000FF

/**
 * @desc: Frame Composer ISRC1 Packet Body Register 4
 * @bits: [7:0] Frame Composer ISRC1 Packet Body Register 4
 */
#define FC_ISCR1_4              0x0000109F
#define FC_ISCR1_4_FC_ISCR1_4_MASK              0x000000FF

/**
 * @desc: Frame Composer ISRC1 Packet Body Register 3
 * @bits: [7:0] Frame Composer ISRC1 Packet Body Register 3
 */
#define FC_ISCR1_3              0x000010A0
#define FC_ISCR1_3_FC_ISCR1_3_MASK              0x000000FF

/**
 * @desc: Frame Composer ISRC1 Packet Body Register 2
 * @bits: [7:0] Frame Composer ISRC1 Packet Body Register 2
 */
#define FC_ISCR1_2              0x000010A1
#define FC_ISCR1_2_FC_ISCR1_2_MASK              0x000000FF

/**
 * @desc: Frame Composer ISRC1 Packet Body Register 1
 * @bits: [7:0] Frame Composer ISRC1 Packet Body Register 1
 */
#define FC_ISCR1_1              0x000010A2
#define FC_ISCR1_1_FC_ISCR1_1_MASK              0x000000FF

/**
 * @desc: Frame Composer ISRC2 Packet Body Register 15
 * @bits: [7:0] Frame Composer ISRC2 Packet Body Register 15
 */
#define FC_ISCR2_15             0x000010A3
#define FC_ISCR2_15_FC_ISCR2_15_MASK            0x000000FF

/**
 * @desc: Frame Composer ISRC2 Packet Body Register 14
 * @bits: [7:0] Frame Composer ISRC2 Packet Body Register 14
 */
#define FC_ISCR2_14             0x000010A4
#define FC_ISCR2_14_FC_ISCR2_14_MASK            0x000000FF

/**
 * @desc: Frame Composer ISRC2 Packet Body Register 13
 * @bits: [7:0] Frame Composer ISRC2 Packet Body Register 13
 */
#define FC_ISCR2_13             0x000010A5
#define FC_ISCR2_13_FC_ISCR2_13_MASK            0x000000FF

/**
 * @desc: Frame Composer ISRC2 Packet Body Register 12
 * @bits: [7:0] Frame Composer ISRC2 Packet Body Register 12
 */
#define FC_ISCR2_12             0x000010A6
#define FC_ISCR2_12_FC_ISCR2_12_MASK            0x000000FF

/**
 * @desc: Frame Composer ISRC2 Packet Body Register 11
 * @bits: [7:0] Frame Composer ISRC2 Packet Body Register 11
 */
#define FC_ISCR2_11             0x000010A7
#define FC_ISCR2_11_FC_ISCR2_11_MASK            0x000000FF

/**
 * @desc: Frame Composer ISRC2 Packet Body Register 10
 * @bits: [7:0] Frame Composer ISRC2 Packet Body Register 10
 */
#define FC_ISCR2_10             0x000010A8
#define FC_ISCR2_10_FC_ISCR2_10_MASK            0x000000FF

/**
 * @desc: Frame Composer ISRC2 Packet Body Register 9
 * @bits: [7:0] Frame Composer ISRC2 Packet Body Register 9
 */
#define FC_ISCR2_9              0x000010A9
#define FC_ISCR2_9_FC_ISCR2_9_MASK              0x000000FF

/**
 * @desc: Frame Composer ISRC2 Packet Body Register 8
 * @bits: [7:0] Frame Composer ISRC2 Packet Body Register 8
 */
#define FC_ISCR2_8              0x000010AA
#define FC_ISCR2_8_FC_ISCR2_8_MASK              0x000000FF

/**
 * @desc: Frame Composer ISRC2 Packet Body Register 7
 * @bits: [7:0] Frame Composer ISRC2 Packet Body Register 7
 */
#define FC_ISCR2_7              0x000010AB
#define FC_ISCR2_7_FC_ISCR2_7_MASK              0x000000FF

/**
 * @desc: Frame Composer ISRC2 Packet Body Register 6
 * @bits: [7:0] Frame Composer ISRC2 Packet Body Register 6
 */
#define FC_ISCR2_6              0x000010AC
#define FC_ISCR2_6_FC_ISCR2_6_MASK              0x000000FF

/**
 * @desc: Frame Composer ISRC2 Packet Body Register 5
 * @bits: [7:0] Frame Composer ISRC2 Packet Body Register 5
 */
#define FC_ISCR2_5              0x000010AD
#define FC_ISCR2_5_FC_ISCR2_5_MASK              0x000000FF

/**
 * @desc: Frame Composer ISRC2 Packet Body Register 4
 * @bits: [7:0] Frame Composer ISRC2 Packet Body Register 4
 */
#define FC_ISCR2_4              0x000010AE
#define FC_ISCR2_4_FC_ISCR2_4_MASK              0x000000FF

/**
 * @desc: Frame Composer ISRC2 Packet Body Register 3
 * @bits: [7:0] Frame Composer ISRC2 Packet Body Register 3
 */
#define FC_ISCR2_3              0x000010AF
#define FC_ISCR2_3_FC_ISCR2_3_MASK              0x000000FF

/**
 * @desc: Frame Composer ISRC2 Packet Body Register 2
 * @bits: [7:0] Frame Composer ISRC2 Packet Body Register 2
 */
#define FC_ISCR2_2              0x000010B0
#define FC_ISCR2_2_FC_ISCR2_2_MASK              0x000000FF

/**
 * @desc: Frame Composer ISRC2 Packet Body Register 1
 * @bits: [7:0] Frame Composer ISRC2 Packet Body Register 1
 */
#define FC_ISCR2_1              0x000010B1
#define FC_ISCR2_1_FC_ISCR2_1_MASK              0x000000FF

/**
 * @desc: Frame Composer ISRC2 Packet Body Register 0
 * @bits: [7:0] Frame Composer ISRC2 Packet Body Register 0
 */
#define FC_ISCR2_0              0x000010B2
#define FC_ISCR2_0_FC_ISCR2_0_MASK              0x000000FF

/**
 * @desc: Frame Composer Data Island Auto Packet Scheduling Register 0
 * Configures the Frame Composer RDRB(1)/Manual(0) data island packet insertion
 * for SPD, VSD, ISRC2, ISRC1 and ACP packets
 * @bits: [0:0] Enables ACP automatic packet scheduling
 *        [1:1] Enables ISRC1 automatic packet scheduling
 *        [2:2] Enables ISRC2 automatic packet scheduling
 *        [3:3] Enables VSD automatic packet scheduling
 *        [4:4] Enables SPD automatic packet scheduling
 */
#define FC_DATAUTO0             0x000010B3
#define FC_DATAUTO0_ACP_AUTO_MASK               0x00000001
#define FC_DATAUTO0_ISCR1_AUTO_MASK             0x00000002
#define FC_DATAUTO0_ISCR2_AUTO_MASK             0x00000004
#define FC_DATAUTO0_VSD_AUTO_MASK               0x00000008
#define FC_DATAUTO0_SPD_AUTO_MASK               0x00000010

/**
 * @desc: Frame Composer Data Island Auto Packet Scheduling Register 1
 * Configures the Frame Composer (FC) RDRB frame interpolation for SPD, VSD,
 * ISRC2, ISRC1 and ACP packet insertion on data island when FC is on RDRB mode for the listed packets
 * @bits: [3:0] Packet frame interpolation for automatic packet scheduling
 */
#define FC_DATAUTO1             0x000010B4
#define FC_DATAUTO1_AUTO_FRAME_INTERPOLATION_MASK   0x0000000F

/**
 * @desc: Frame Composer Data Island Auto packet scheduling Register 2
 * Configures the Frame Composer (FC) RDRB line interpolation and number of
 * packets in frame for SPD, VSD, ISRC2, ISRC1 and ACP packet insertion on
 * data island when FC is on RDRB mode for the listed packets
 * @bits: [3:0] Packets line spacing, for automatic packet scheduling
 *        [7:4] Packets per frame, for automatic packet scheduling
 */
#define FC_DATAUTO2             0x000010B5
#define FC_DATAUTO2_AUTO_LINE_SPACING_MASK      0x0000000F
#define FC_DATAUTO2_AUTO_FRAME_PACKETS_MASK     0x000000F0

/**
 * @desc: Frame Composer Data Island Manual Packet Request Register
 * Requests to the Frame Composer the data island packet insertion for NULL, SPD,
 * VSD, ISRC2, ISRC1 and ACP packets when FC_DATAUTO0 bit is in manual mode for the packet requested
 * @bits: [0:0] ACP packet
 *        [1:1] ISRC1 packet
 *        [2:2] ISRC2 packet
 *        [3:3] VSD packet
 *        [4:4] SPD packet
 *        [5:5] Null packet
 */
#define FC_DATMAN               0x000010B6
#define FC_DATMAN_ACP_TX_MASK                   0x00000001
#define FC_DATMAN_ISCR1_TX_MASK                 0x00000002
#define FC_DATMAN_ISCR2_TX_MASK                 0x00000004
#define FC_DATMAN_VSD_TX_MASK                   0x00000008
#define FC_DATMAN_SPD_TX_MASK                   0x00000010
#define FC_DATMAN_NULL_TX_MASK                  0x00000020

/**
 * @desc: Frame Composer Data Island Auto Packet Scheduling Register 3
 * Configures the Frame Composer Automatic(1)/RDRB(0) data island packet
 * insertion for AVI, GCP, AUDI and ACR packets
 * @bits: [0:0] Enables ACR packet insertion
 *        [1:1] Enables AUDI packet insertion
 *        [2:2] Enables GCP packet insertion
 *        [3:3] Enables AVI packet insertion
 *        [4:4] Enables AMP packet insertion
 *        [5:5] Enables NTSC VBI packet insertion
 */
#define FC_DATAUTO3             0x000010B7
#define FC_DATAUTO3_ACR_AUTO_MASK               0x00000001
#define FC_DATAUTO3_AUDI_AUTO_MASK              0x00000002
#define FC_DATAUTO3_GCP_AUTO_MASK               0x00000004
#define FC_DATAUTO3_AVI_AUTO_MASK               0x00000008
#define FC_DATAUTO3_AMP_AUTO_MASK               0x00000010
#define FC_DATAUTO3_NVBI_AUTO_MASK              0x00000020

/**
 * @desc: Frame Composer Round Robin ACR Packet Insertion Register 0
 * Configures the Frame Composer (FC) RDRB frame interpolation for ACR packet
 * insertion on data island when FC is on RDRB mode for this packet
 * @bits: [3:0] ACR Frame interpolation
 */
#define FC_RDRB0                0x000010B8
#define FC_RDRB0_ACRFRAMEINTERPOLATION_MASK     0x0000000F

/**
 * @desc: Frame Composer Round Robin ACR Packet Insertion Register 1
 * Configures the Frame Composer (FC) RDRB line interpolation and number of packets
 * in frame for the ACR packet insertion on data island when FC is on RDRB mode this packet
 * @bits: [3:0] ACR packet line spacing
 *        [7:4] ACR packets in frame
 */
#define FC_RDRB1                0x000010B9
#define FC_RDRB1_ACRPACKETLINESPACING_MASK      0x0000000F
#define FC_RDRB1_ACRPACKETSINFRAME_MASK         0x000000F0

/**
 * @desc: Frame Composer Round Robin AUDI Packet Insertion Register 2
 * Configures the Frame Composer (FC) RDRB frame interpolation for AUDI packet
 * insertion on data island when FC is on RDRB mode for this packet
 * @bits: [3:0] Audio frame interpolation
 */
#define FC_RDRB2                0x000010BA
#define FC_RDRB2_AUDIFRAMEINTERPOLATION_MASK    0x0000000F

/**
 * @desc: Frame Composer Round Robin AUDI Packet Insertion Register 3
 * Configures the Frame Composer (FC) RDRB line interpolation and number of packets
 * in frame for the AUDI packet insertion on data island when FC is on RDRB mode this packet
 * @bits: [3:0] Audio packets line spacing
 *        [7:4] Audio packets per frame
 */
#define FC_RDRB3                0x000010BB
#define FC_RDRB3_AUDIPACKETLINESPACING_MASK     0x0000000F
#define FC_RDRB3_AUDIPACKETSINFRAME_MASK        0x000000F0

/**
 * @desc: Frame Composer Round Robin GCP Packet Insertion Register 4
 * Configures the Frame Composer (FC) RDRB frame interpolation for GCP packet
 * insertion on data island when FC is on RDRB mode for this packet
 * @bits: [3:0] Frames interpolated between GCP packets
 */
#define FC_RDRB4                0x000010BC
#define FC_RDRB4_GCPFRAMEINTERPOLATION_MASK     0x0000000F

/**
 * @desc: Frame Composer Round Robin GCP Packet Insertion Register 5
 * Configures the Frame Composer (FC) RDRB line interpolation and number of
 * packets in frame for the GCP packet insertion on data island when FC is on RDRB mode this packet
 * @bits: [3:0] GCP packets line spacing
 *        [7:4] GCP packets per frame
 */
#define FC_RDRB5                0x000010BD
#define FC_RDRB5_GCPPACKETLINESPACING_MASK      0x0000000F
#define FC_RDRB5_GCPPACKETSINFRAME_MASK         0x000000F0

/**
 * @desc: Frame Composer Round Robin AVI Packet Insertion Register 6
 * Configures the Frame Composer (FC) RDRB frame interpolation for AVI packet
 * insertion on data island when FC is on RDRB mode for this packet
 * @bits: [3:0] Frames interpolated between AVI packets
 */
#define FC_RDRB6                0x000010BE
#define FC_RDRB6_AVIFRAMEINTERPOLATION_MASK     0x0000000F

/**
 * @desc: Frame Composer Round Robin AVI Packet Insertion Register 7
 * Configures the Frame Composer (FC) RDRB line interpolation and number of packets
 * in frame for the AVI packet insertion on data island when FC is on RDRB mode this packet
 * @bits: [3:0] AVI packets line spacing
 *        [7:4] AVI packets per frame
 */
#define FC_RDRB7                0x000010BF
#define FC_RDRB7_AVIPACKETLINESPACING_MASK      0x0000000F
#define FC_RDRB7_AVIPACKETSINFRAME_MASK         0x000000F0

/**
 * @desc: Frame Composer Round Robin AMP Packet Insertion Register 8
 * @bits: [3:0] AMP frame interpolation
 */
#define FC_RDRB8                0x000010C0
#define FC_RDRB8_AMPFRAMEINTERPOLATION_MASK     0x0000000F

/**
 * @desc: Frame Composer Round Robin AMP Packet Insertion Register 9
 * @bits: [3:0] AMP packets line spacing
 *        [7:4] AMP packets per frame
 */
#define FC_RDRB9                0x000010C1
#define FC_RDRB9_AMPPACKETLINESPACING_MASK      0x0000000F
#define FC_RDRB9_AMPPACKETSINFRAME_MASK         0x000000F0

/**
 * @desc: Frame Composer Round Robin NTSC VBI Packet Insertion Register 10
 * @bits: [3:0] NTSC VBI frame interpolation
 */
#define FC_RDRB10               0x000010C2
#define FC_RDRB10_NVBIFRAMEINTERPOLATION_MASK   0x0000000F

/**
 * @desc: Frame Composer Round Robin NTSC VBI Packet Insertion Register 11
 * @bits: [3:0] NTSC VBI packets line spacing
 *        [7:4] NTSC VBI packets per frame
 */
#define FC_RDRB11               0x000010C3
#define FC_RDRB11_NVBIPACKETLINESPACING_MASK    0x0000000F
#define FC_RDRB11_NVBIPACKETSINFRAME_MASK       0x000000F0

/**
 * @desc: Frame Composer Packet Interrupt Mask Register 0
 * @bits: [0:0] Mask bit for FC_INT0
 *        [1:1] Mask bit for FC_INT0
 *        [2:2] Mask bit for FC_INT0
 *        [3:3] Mask bit for FC_INT0
 *        [4:4] Mask bit for FC_INT0
 *        [5:5] Mask bit for FC_INT0
 *        [6:6] Mask bit for FC_INT0
 *        [7:7] Mask bit for FC_INT0
 */
#define FC_MASK0                0x000010D2
#define FC_MASK0_NULL_MASK                      0x00000001
#define FC_MASK0_ACR_MASK                       0x00000002
#define FC_MASK0_AUDS_MASK                      0x00000004
#define FC_MASK0_NVBI_MASK                      0x00000008
#define FC_MASK0_MAS_MASK                       0x00000010
#define FC_MASK0_HBR_MASK                       0x00000020
#define FC_MASK0_ACP_MASK                       0x00000040
#define FC_MASK0_AUDI_MASK                      0x00000080

/**
 * @desc: Frame Composer Packet Interrupt Mask Register 1
 * @bits: [0:0] Mask bit for FC_INT1
 *        [1:1] Mask bit for FC_INT1
 *        [2:2] Mask bit for FC_INT1
 *        [3:3] Mask bit for FC_INT1
 *        [4:4] Mask bit for FC_INT1
 *        [5:5] Mask bit for FC_INT1
 *        [6:6] Mask bit for FC_INT1
 *        [7:7] Mask bit for FC_INT1
 */
#define FC_MASK1                0x000010D6
#define FC_MASK1_GCP_MASK                       0x00000001
#define FC_MASK1_AVI_MASK                       0x00000002
#define FC_MASK1_AMP_MASK                       0x00000004
#define FC_MASK1_SPD_MASK                       0x00000008
#define FC_MASK1_VSD_MASK                       0x00000010
#define FC_MASK1_ISCR2_MASK                     0x00000020
#define FC_MASK1_ISCR1_MASK                     0x00000040
#define FC_MASK1_GMD_MASK                       0x00000080

/**
 * @desc: Frame Composer High/Low Priority Overflow Interrupt Mask Register 2
 * @bits: [0:0] Mask bit for FC_INT2
 *        [1:1] Mask bit for FC_INT2
 */
#define FC_MASK2                0x000010DA
#define FC_MASK2_HIGHPRIORITY_OVERFLOW_MASK     0x00000001
#define FC_MASK2_LOWPRIORITY_OVERFLOW_MASK      0x00000002

/**
 * @desc: Frame Composer Pixel Repetition Configuration Register
 * @bits: [3:0] Configures the video pixel repetition ratio to be sent on the AVI InfoFrame
 *        [7:4] Configures the input video pixel repetition
 *          - 0000: Not used
 *          - 0001: No Pixel repetition (pixel sent only once)
 *          - 0010: Pixel sent two times
 *          - 0011: Pixel sent three times
 *          - 0100: Pixel sent four times
 *          - 0101: Pixel sent five times
 *          - 0110: Pixel sent six times
 *          - 0111: Pixel sent seven times
 *          - 1000: Pixel sent eight times
 *          - 1001: Pixel sent nine times
 *          - 1010: Pixel sent ten times
 *          - other: Not used
 */
#define FC_PRCONF               0x000010E0
#define FC_PRCONF_OUTPUT_PR_FACTOR_MASK         0x0000000F
#define FC_PRCONF_INCOMING_PR_FACTOR_MASK       0x000000F0

/**
 * @desc: Frame Composer Scrambler Control
 * @birs: [0:0] When set (1'b1), this field activates the scrambler feature
 *        [4:4] Debug register
 */
#define FC_SCRAMBLER_CTRL       0x000010E1
#define FC_SCRAMBLER_CTRL_SCRAMBLER_ON_MASK         0x00000001
#define FC_SCRAMBLER_CTRL_SCRAMBLER_UCP_LINE_MASK   0x00000010

/**
 * @desc: Frame Composer Multi-Stream Audio Control
 * @bits: [0:0] when set (1'b1), this field activates the Multi-Stream support
 */
#define FC_MULTISTREAM_CTRL     0x000010E2
#define FC_MULTISTREAM_CTRL_FC_MAS_PACKET_EN_MASK   0x00000001

/**
 * @desc: Frame Composer Packet Transmission Control
 * @bits: [0:0] ACR packet transmission control
 *          - 1: Transmission enabled
 *          - 0: Transmission disabled
 *        [1:1] GCP transmission control
 *          - 1: Transmission enabled
 *          - 0: Transmission disabled
 *        [2:2] AVI packet transmission control
 *          - 1: Transmission enabled
 *          - 0: Transmission disabled
 *        [3:3] AUDI packet transmission control
 *          - 1: Transmission enabled
 *          - 0: Transmission disabled
 *        [4:4] ACP, SPD, VSIF, ISRC1, and SRC2 packet transmission control
 *          - 1: Transmission enabled
 *          - 0: Transmission disabled
 *        [5:5] AMP transmission control
 *          - 1: Transmission enabled
 *          - 0: Transmission disabled
 *        [6:6] NTSC VBI transmission control
 *          - 1: Transmission enabled
 *          - 0: Transmission disabled
 *        [7:7] DRM transmission control
 */
#define FC_PACKET_TX_EN         0x000010E3
#define FC_PACKET_TX_EN_ACR_TX_EN_MASK              0x00000001
#define FC_PACKET_TX_EN_GCP_TX_EN_MASK              0x00000002
#define FC_PACKET_TX_EN_AVI_TX_EN_MASK              0x00000004
#define FC_PACKET_TX_EN_AUDI_TX_EN_MASK             0x00000008
#define FC_PACKET_TX_EN_AUT_TX_EN_MASK              0x00000010
#define FC_PACKET_TX_EN_AMP_TX_EN_MASK              0x00000020
#define FC_PACKET_TX_EN_NVBI_TX_EN_MASK             0x00000040
#define FC_PACKET_TX_EN_DRM_TX_EN_MASK              0x00000080

/**
 * @desc: Frame Composer Active Space Control
 * @bits: [0:0] Active Space Handler Control 1b: Fixed active space value mode enabled
 *        [1:1] Active Space handler control 1b: Active space 1 value is different from Active Space 2 value
 */
#define FC_ACTSPC_HDLR_CFG      0x000010E8
#define FC_ACTSPC_HDLR_CFG_ACTSPC_HDLR_EN_MASK      0x00000001
#define FC_ACTSPC_HDLR_CFG_ACTSPC_HDLR_TGL_MASK     0x00000002

/**
 * @desc: Frame Composer Input Video 2D VActive Pixels Register 0
 * @bits: [7:0] 2D Input video vertical active pixel region width
 */
#define FC_INVACT_2D_0          0x000010E9
#define FC_INVACT_2D_0_FC_INVACT_2D_0_MASK          0x000000FF

/**
 * @desc: Frame Composer Input Video VActive pixels Register 1
 * @bits: [3:0] 2D Input video vertical active pixel region width
 */
#define FC_INVACT_2D_1          0x000010EA
#define FC_INVACT_2D_1_FC_INVACT_2D_1_MASK          0x0000000F

/**
 * @desc: Frame Composer GMD Packet Status Register
 * Gamut metadata packet status bit information for no_current_gmd,
 * next_gmd_field, gmd_packet_sequence and current_gamut_seq_num
 * @bits: [3:0] Gamut scheduling: Current Gamut packet sequence number
 *        [5:4] Gamut scheduling: Gamut packet sequence
 *        [6:6] Gamut scheduling: Gamut Next field
 *        [7:7] Gamut scheduling: No current gamut data
 */
#define FC_GMD_STAT             0x00001100
#define FC_GMD_STAT_IGMDCURRENT_GAMUT_SEQ_NUM_MASK  0x0000000F
#define FC_GMD_STAT_IGMDPACKET_SEQ_MASK             0x00000030
#define FC_GMD_STAT_IGMDDNEXT_FIELD_MASK            0x00000040
#define FC_GMD_STAT_IGMDNO_CRNT_GBD_MASK            0x00000080

/**
 * @desc: Frame Composer GMD Packet Enable Register
 * This register enables Gamut metadata (GMD) packet transmission
 * @bits: [0:0] Gamut Metadata packet transmission enable (1b)
 */
#define FC_GMD_EN               0x00001101
#define FC_GMD_EN_GMDENABLETX_MASK                  0x00000001

/**
 * @desc: Frame Composer GMD Packet Update Register
 * This register performs an GMD packet content update according to the
 * configured packet body (FC_GMD_PB0 to FC_GMD_PB27) and packet header (FC_GMD_HB)
 * @bits: [0:0] Gamut Metadata packet update
 */
#define FC_GMD_UP               0x00001102
#define FC_GMD_UP_GMDUPDATEPACKET_MASK              0x00000001

/**
 * @desc: Frame Composer GMD Packet Schedule Configuration Register
 * This register configures the number of GMD packets to be inserted per frame
 * (starting always in the line where the active Vsync appears) and the line
 * spacing between the transmitted GMD packets
 * @bits: [3:0] Number of line spacing between the transmitted GMD packets
 *        [7:4] Number of GMD packets per frame or video field (profile P0)
 */
#define FC_GMD_CONF             0x00001103
#define FC_GMD_CONF_GMDPACKETLINESPACING_MASK       0x0000000F
#define FC_GMD_CONF_GMDPACKETSINFRAME_MASK          0x000000F0

/**
 * @desc: Frame Composer GMD Packet Profile and Gamut Sequence Configuration Register
 * This register configures the GMD packet header affected_gamut_seq_num and gmd_profile bits
 * @bits: [3:0] Affected gamut sequence number
 *        [6:4] GMD profile bits
 */
#define FC_GMD_HB               0x00001104
#define FC_GMD_HB_GMDAFFECTED_GAMUT_SEQ_NUM_MASK    0x0000000F
#define FC_GMD_HB_GMDGBD_PROFILE_MASK               0x00000070

/**
 * @desc: Frame Composer GMD Packet Body Register Array
 * @bits: [7:0] Frame Composer GMD Packet Body Register Array
*/
#define FC_GMD_PB0              0x00001105
#define FC_GMD_PB27             0x00001120

/**
 * @desc: Frame Composer AMP Packet Header Register 1
 * @bits: [7:0] Frame Composer AMP Packet Header Register 1
 */
#define FC_AMP_HB1              0x00001128
#define FC_AMP_HB1_FC_AMP_HB0_MASK                  0x000000FF

/**
 * @desc: Frame Composer AMP Packet Header Register 2
 * @bits: [7:0] Frame Composer AMP Packet Header Register 2
 */
#define FC_AMP_HB2              0x00001189
#define FC_AMP_HB2_FC_AMP_HB1_MASK                  0x000000FF

/**
 * @desc: Frame Composer AMP Packet Body Register Array
 * @bits: [7:0] Frame Composer AMP Packet Body Register Array
 */
#define FC_AMP_PB               0x0000112A

/**
 * @desc: Frame Composer NTSC VBI Packet Header Register 1
 * @bits: [7:0] Frame Composer NTSC VBI Packet Header Register 1
 */
#define FC_NVBI_HB1             0x00001148
#define FC_NVBI_HB1_FC_NVBI_HB0_MASK                0x000000FF

/**
 * @desc: Frame Composer NTSC VBI Packet Header Register 2
 * @bits: [7:0] Frame Composer NTSC VBI Packet Header Register 2
 */
#define FC_NVBI_HB2             0x00001149
#define FC_NVBI_HB2_FC_NVBI_HB1_MASK                0x000000FF

/**
 * @desc: Frame Composer DRM Packet Update Register
 * @bits: [0:0] DRM packet update
 */
#define FC_DRM_UP               0x00001167
#define FC_DRM_UP_DRMPACKETUPDATE_MASK              0x00000001

/**
 * @desc: Frame Composer DRM Packet Header Rigister 0
 * @bits: [7:0] Frame Composer DRM Packet Header Rigister 0
 */
#define FC_DRM_HB0              0x00001168

/**
 * @desc: Frame Composer DRM Packet Header Rigister 1
 * @bits: [7:0] Frame Composer DRM Packet Header Rigister 1
 */
#define FC_DRM_HB1              0x00001169
#define FC_DRM_UP_FC_DRM_HB_MASK                    0x000000FF

/**
 * @desc: Frame Composer DRM Packet Body Register Arry
 * @bits: [7:0] Frame Composer DRM Packet Body Register Arry
 */
#define FC_DRM_PB0              0x0000116A
#define FC_DRM_PB1              0x0000116B
#define FC_DRM_PB2              0x0000116C
#define FC_DRM_PB3              0x0000116D
#define FC_DRM_PB4              0x0000116E
#define FC_DRM_PB5              0x0000116F
#define FC_DRM_PB6              0x00001170
#define FC_DRM_PB7              0x00001171
#define FC_DRM_PB8              0x00001172
#define FC_DRM_PB9              0x00001173
#define FC_DRM_PB10             0x00001174
#define FC_DRM_PB11             0x00001175
#define FC_DRM_PB12             0x00001176
#define FC_DRM_PB13             0x00001177
#define FC_DRM_PB14             0x00001178
#define FC_DRM_PB15             0x00001179
#define FC_DRM_PB16             0x0000117A
#define FC_DRM_PB17             0x0000117B
#define FC_DRM_PB18             0x0000117C
#define FC_DRM_PB19             0x0000117D
#define FC_DRM_PB20             0x0000117E
#define FC_DRM_PB21             0x0000117F
#define FC_DRM_PB22             0x00001180
#define FC_DRM_PB23             0x00001181
#define FC_DRM_PB24             0x00001182
#define FC_DRM_PB25             0x00001183
#define FC_DRM_PB26             0x00001184

/**
 * @desc: Frame Composer video/audio Force Enable Register
 * This register allows to force the controller to output audio and video data
 * the values configured in the FC_DBGAUD and FC_DBGTMDS registers
 * @bits: [0:0] Force fixed video output with FC_DBGTMDSx register contents
 *        [4:4] Force fixed audio output with FC_DBGAUDxCHx register contents
 */
#define FC_DBGFORCE             0x00001200
#define FC_DBGFORCE_FORCEVIDEO_MASK                 0x00000001
#define FC_DBGFORCE_FORCEAUDIO_MASK                 0x00000010

/**
 * @desc: Frame Composer Audio Data Channel 0 Register 0
 * Configures the audio fixed data to be used in channel 0 when in fixed audio selection
 * @bits: [7:0] Frame Composer Audio Data Channel 0 Register 0
 */
#define FC_DBGAUD0CH0           0x00001201
#define FC_DBGAUD0CH0_FC_DBGAUD0CH0_MASK            0x000000FF

/**
 * @desc: Frame Composer Audio Data Channel 0 Register 1
 * Configures the audio fixed data to be used in channel 0 when in fixed audio selection
 * @bits: [7:0] Frame Composer Audio Data Channel 0 Register 1
 */
#define FC_DBGAUD1CH0           0x00001202
#define FC_DBGAUD1CH0_FC_DBGAUD1CH0_MASK            0x000000FF

/**
 * @desc: Frame Composer Audio Data Channel 0 Register 2
 * Configures the audio fixed data to be used in channel 0 when in fixed audio selection
 * @bits: [7:0] Frame Composer Audio Data Channel 0 Register 2
 */
#define FC_DBGAUD2CH0           0x00001203
#define FC_DBGAUD2CH0_FC_DBGAUD2CH0_MASK            0x000000FF

/**
 * @desc: Frame Composer Audio Data Channel 1 Register 0
 * Configures the audio fixed data to be used in channel 1 when in fixed audio selection
 * @bits: [7:0] Frame Composer Audio Data Channel 1 Register 0
 */
#define FC_DBGAUD0CH1           0x00001204
#define FC_DBGAUD0CH1_FC_DBGAUD0CH1_MASK            0x000000FF

/**
 * @desc: Frame Composer Audio Data Channel 1 Register 1
 * Configures the audio fixed data to be used in channel 1 when in fixed audio selection
 * @bits: [7:0] Frame Composer Audio Data Channel 1 Register 1
 */
#define FC_DBGAUD1CH1           0x00001205
#define FC_DBGAUD1CH1_FC_DBGAUD1CH1_MASK            0x000000FF

/**
 * @desc: Frame Composer Audio Data Channel 1 Register 2
 * Configures the audio fixed data to be used in channel 1 when in fixed audio selection
 * @bits: [7:0] Frame Composer Audio Data Channel 1 Register 2
 */
#define FC_DBGAUD2CH1           0x00001206
#define FC_DBGAUD2CH1_FC_DBGAUD2CH1_MASK            0x000000FF

/**
 * @desc: Frame Composer Audio Data Channel 2 Register 0
 * Configures the audio fixed data to be used in channel 2 when in fixed audio selection
 * @bits: [7:0] Frame Composer Audio Data Channel 2 Register 0
 */
#define FC_DBGAUD0CH2           0x00001207
#define FC_DBGAUD0CH2_FC_DBGAUD0CH2_MASK            0x000000FF

/**
 * @desc: Frame Composer Audio Data Channel 2 Register 1
 * Configures the audio fixed data to be used in channel 2 when in fixed audio selection
 * @bits: [7:0] Frame Composer Audio Data Channel 2 Register 1
 */
#define FC_DBGAUD1CH2           0x00001208
#define FC_DBGAUD1CH2_FC_DBGAUD1CH2_MASK            0x000000FF

/**
 * @desc: Frame Composer Audio Data Channel 2 Register 2
 * Configures the audio fixed data to be used in channel 2 when in fixed audio selection
 * @bits: [7:0] Frame Composer Audio Data Channel 2 Register 2
 */
#define FC_DBGAUD2CH2           0x00001209
#define FC_DBGAUD2CH2_FC_DBGAUD2CH2_MASK            0x000000FF

/**
 * @desc: Frame Composer Audio Data Channel 3 Register 0
 * Configures the audio fixed data to be used in channel 3 when in fixed audio selection
 * @bits: [7:0] Frame Composer Audio Data Channel 3 Register 0
 */
#define FC_DBGAUD0CH3           0x0000120A
#define FC_DBGAUD0CH3_FC_DBGAUD0CH3_MASK            0x000000FF

/**
 * @desc: Frame Composer Audio Data Channel 3 Register 1
 * Configures the audio fixed data to be used in channel 3 when in fixed audio selection
 * @bits: Frame Composer Audio Data Channel 3 Register 1
 */
#define FC_DBGAUD1CH3           0x0000120B
#define FC_DBGAUD1CH3_FC_DBGAUD1CH3_MASK            0x000000FF

/**
 * @desc: Frame Composer Audio Data Channel 3 Register 2
 * Configures the audio fixed data to be used in channel 3 when in fixed audio selection
 * @bits: Frame Composer Audio Data Channel 3 Register 2
 */
#define FC_DBGAUD2CH3           0x0000120C
#define FC_DBGAUD2CH3_FC_DBGAUD2CH3_MASK            0x000000FF

/**
 * @desc: Frame Composer Audio Data Channel 4 Register 0
 * Configures the audio fixed data to be used in channel 4 when in fixed audio selection
 * @bits: [7:0] Frame Composer Audio Data Channel 4 Register 0
 */
#define FC_DBGAUD0CH4           0x0000120D
#define FC_DBGAUD0CH4_FC_DBGAUD0CH4_MASK            0x000000FF

/**
 * @desc: Frame Composer Audio Data Channel 4 Register 1
 * Configures the audio fixed data to be used in channel 4 when in fixed audio selection
 * @bits: [7:0] Frame Composer Audio Data Channel 4 Register 1
 */
#define FC_DBGAUD1CH4           0x0000120E
#define FC_DBGAUD1CH4_FC_DBGAUD1CH4_MASK            0x000000FF

/**
 * @desc: Frame Composer Audio Data Channel 4 Register 2
 * Configures the audio fixed data to be used in channel 4 when in fixed audio selection
 * @bits: [7:0] Frame Composer Audio Data Channel 4 Register 2
 */
#define FC_DBGAUD2CH4           0x0000120F
#define FC_DBGAUD2CH4_FC_DBGAUD2CH4_MASK            0x000000FF

/**
 * @desc: Frame Composer Audio Data Channel 5 Register 0
 * Configures the audio fixed data to be used in channel 5 when in fixed audio selection
 * @bits: [7:0] Frame Composer Audio Data Channel 5 Register 0
 */
#define FC_DBGAUD0CH5           0x00001210
#define FC_DBGAUD0CH5_FC_DBGAUD0CH5_MASK            0x000000FF

/**
 * @desc: Frame Composer Audio Data Channel 5 Register 1
 * Configures the audio fixed data to be used in channel 5 when in fixed audio selection
 * @bits: [7:0] Frame Composer Audio Data Channel 5 Register 1
 */
#define FC_DBGAUD1CH5           0x00001211
#define FC_DBGAUD1CH5_FC_DBGAUD1CH5_MASK            0x000000FF

/**
 * @desc: Frame Composer Audio Data Channel 5 Register 2
 * Configures the audio fixed data to be used in channel 5 when in fixed audio selection
 * @bits: [7:0] Frame Composer Audio Data Channel 5 Register 2
 */
#define FC_DBGAUD2CH5           0x00001212
#define FC_DBGAUD2CH5_FC_DBGAUD2CH5_MASK            0x000000FF

/**
 * @desc: Frame Composer Audio Data Channel 6 Register 0
 * Configures the audio fixed data to be used in channel 6 when in fixed audio selection
 * @bits: [7:0] Frame Composer Audio Data Channel 6 Register 0
 */
#define FC_DBGAUD0CH6           0x00001213
#define FC_DBGAUD0CH6_FC_DBGAUD0CH6_MASK            0x000000FF

/**
 * @desc: Frame Composer Audio Data Channel 6 Register 1
 * Configures the audio fixed data to be used in channel 6 when in fixed audio selection
 * @bits: [7:0] Frame Composer Audio Data Channel 6 Register 1
 */
#define FC_DBGAUD1CH6           0x00001214
#define FC_DBGAUD1CH6_FC_DBGAUD1CH6_MASK            0x000000FF

/**
 * @desc: Frame Composer Audio Data Channel 6 Register 2
 * Configures the audio fixed data to be used in channel 6 when in fixed audio selection
 * @bits: [7:0] Frame Composer Audio Data Channel 6 Register 2
 */
#define FC_DBGAUD2CH6           0x00001215
#define FC_DBGAUD2CH6_FC_DBGAUD2CH6_MASK            0x000000FF

/**
 * @desc: Frame Composer Audio Data Channel 7 Register 0
 * Configures the audio fixed data to be used in channel 7 when in fixed audio selection
 * @bits: [7:0] Frame Composer Audio Data Channel 7 Register 0
 */
#define FC_DBGAUD0CH7           0x00001216
#define FC_DBGAUD0CH7_FC_DBGAUD0CH7_MASK            0x000000FF

/**
 * @desc: Frame Composer Audio Data Channel 7 Register 1
 * Configures the audio fixed data to be used in channel 7 when in fixed audio selection
 * @bits: [7:0] Frame Composer Audio Data Channel 7 Register 1
 */
#define FC_DBGAUD1CH7           0x00001217
#define FC_DBGAUD1CH7_FC_DBGAUD1CH7_MASK            0x000000FF

/**
 * @desc: Frame Composer Audio Data Channel 7 Register 2
 * Configures the audio fixed data to be used in channel 7 when in fixed audio selection
 * @bits: [7:0] Frame Composer Audio Data Channel 7 Register 2
 */
#define FC_DBGAUD2CH7           0x00001218
#define FC_DBGAUD2CH7_FC_DBGAUD2CH7_MASK            0x000000FF

/**
 * @desc: Frame Composer TMDS Data Channel 0 Register
 * Configures the video fixed data ti be used in TMDS channel 0 when fixed video selection
 * @bits: [7:0] Frame Composer TMDS Data Channel 0 Register
*/
#define FC_DBGTMDS0             0x00001219
/**
 * @desc: Frame Composer TMDS Data Channel 1 Register
 * Configures the video fixed data ti be used in TMDS channel 1 when fixed video selection
 * @bits: [7:0] Frame Composer TMDS Data Channel 1 Register
*/
#define FC_DBGTMDS1             0x0000121A
/**
 * @desc: Frame Composer TMDS Data Channel 2 Register
 * Configures the video fixed data ti be used in TMDS channel 2 when fixed video selection
 * @bits: [7:0] Frame Composer TMDS Data Channel 2 Register
*/
#define FC_DBGTMDS2             0x0000121B

#endif /* _DW_FRAME_COMPOSER_H */
