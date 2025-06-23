// SPDX-License-Identifier: GPL-2.0
//------------------------------------------------------------------------------
//	Trilinear Technologies DisplayPort DRM Driver
//	Copyright (C) 2023 Trilinear Technologies
//
//	This program is free software: you can redistribute it and/or modify
//	it under the terms of the GNU General Public License as published by
//	the Free Software Foundation, version 2.
//
//	This program is distributed in the hope that it will be useful, but
//	WITHOUT ANY WARRANTY; without even the implied warranty of
//	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
//	General Public License for more details.
//
//	You should have received a copy of the GNU General Public License
//	along with this program. If not, see <http://www.gnu.org/licenses/>.
//------------------------------------------------------------------------------

#ifndef _TRILIN_DPTX_REG_H_
#define _TRILIN_DPTX_REG_H_

//------------------------------------------------------------------------------
//   link configuration fields
//------------------------------------------------------------------------------
#define TRILIN_DPTX_LINK_BW_SET					0x000ul
#define TRILIN_DPTX_LANE_COUNT_SET                              0x004ul
#define TRILIN_DPTX_ENHANCED_FRAMING_ENABLE                     0x008ul
#define TRILIN_DPTX_TRAINING_PATTERN_SET                        0x00cul
#define TRILIN_DPTX_LINK_QUAL_PATTERN_SET                       0x010ul
#define TRILIN_DPTX_DISABLE_SCRAMBLING                          0x014ul
#define TRILIN_DPTX_DOWNSPREAD_CONTROL                          0x018ul
#define TRILIN_DPTX_EDP_CAPABILITY_CONFIG                       0x01cul
#define TRILIN_DPTX_HBR2_SCRAMBLER_RESET                        0x020ul
#define TRILIN_DPTX_DISPLAYPORT_VERSION                         0x024ul
#define TRILIN_DPTX_ALPM_POWER_SET                              0x028ul
#define TRILIN_DPTX_LANE_REMAP                                  0x02cul
#define TRILIN_DPTX_CUSTOM_80BIT_PATTERN_31_0                   0x030ul
#define TRILIN_DPTX_CUSTOM_80BIT_PATTERN_63_32                  0x034ul
#define TRILIN_DPTX_CUSTOM_80BIT_PATTERN_79_64                  0x038ul

//------------------------------------------------------------------------------
//   core enables
//------------------------------------------------------------------------------
#define TRILIN_DPTX_TRANSMITTER_ENABLE                          0x080ul
#define TRILIN_DPTX_SOFT_RESET                                  0x090ul
#define TRILIN_DPTX_SOURCE_ENABLE                               0x094ul
#define TRILIN_DPTX_FEC_ENABLE                                  0x098ul

//------------------------------------------------------------------------------
//   misc control registers
//------------------------------------------------------------------------------
#define TRILIN_DPTX_FORCE_SCRAMBLER_RESET                       0x0c0ul
#define TRILIN_DPTX_CORE_FEATURES                               0x0f8ul
#define TRILIN_DPTX_CORE_REVISION                               0x0fcul

//------------------------------------------------------------------------------
//   AUX channel
//------------------------------------------------------------------------------
#define TRILIN_DPTX_AUX_COMMAND                                 0x100ul
#define TRILIN_DPTX_AUX_WRITE_FIFO                              0x104ul
#define TRILIN_DPTX_AUX_ADDRESS                                 0x108ul
#define TRILIN_DPTX_AUX_CLOCK_DIVIDER                           0x10cul
#define TRILIN_DPTX_AUX_REPLY_TIMEOUT_INTERVAL                  0x110ul
#define TRILIN_DPTX_HPD_INPUT_STATE                             0x128ul
#define TRILIN_DPTX_INTERRUPT_STATE                             0x130ul
#define TRILIN_DPTX_AUX_REPLY_DATA                              0x134ul
#define TRILIN_DPTX_AUX_REPLY_CODE                              0x138ul
#define TRILIN_DPTX_AUX_REPLY_COUNT                             0x13cul
#define TRILIN_DPTX_INTERRUPT_CAUSE                             0x140ul
#define TRILIN_DPTX_INTERRUPT_MASK                              0x144ul
#define TRILIN_DPTX_AUX_REPLY_DATA_COUNT                        0x148ul
#define TRILIN_DPTX_AUX_STATUS                                  0x14cul
#define TRILIN_DPTX_AUX_REPLY_CLOCK_WIDTH                       0x150ul
#define TRILIN_DPTX_AUX_PHY_WAKE_ACK_DETECTED                   0x154ul
#define TRILIN_DPTX_HOST_TIMER                                  0x158ul
#define TRILIN_DPTX_MST_TIMER                                   0x15cul

//------------------------------------------------------------------------------
//   PHY status
//------------------------------------------------------------------------------
#define TRILIN_DPTX_PHY_STATUS                                  0x280ul

//------------------------------------------------------------------------------
//   HDCP control
//------------------------------------------------------------------------------
#define TRILIN_DPTX_HDCP_ENABLE                                 0x400ul
#define TRILIN_DPTX_HDCP_MODE                                   0x404ul
#define TRILIN_DPTX_HDCP_KS_31_0                                0x408ul
#define TRILIN_DPTX_HDCP_KS_63_32                               0x40cul
#define TRILIN_DPTX_HDCP_KM_31_0                                0x410ul
#define TRILIN_DPTX_HDCP_KM_63_32                               0x414ul
#define TRILIN_DPTX_HDCP_AN_31_0                                0x418ul
#define TRILIN_DPTX_HDCP_RTX_31_0                               0x418ul
#define TRILIN_DPTX_HDCP_AN_63_32                               0x41cul
#define TRILIN_DPTX_HDCP_RTX_63_32                              0x41cul
#define TRILIN_DPTX_HDCP_RESERVED_420                           0x420ul
#define TRILIN_DPTX_HDCP_AUTH_IN_PROGRESS                       0x424ul
#define TRILIN_DPTX_HDCP_R0_STATUS                              0x428ul
#define TRILIN_DPTX_HDCP_CIPHER_CONTROL                         0x42cul
#define TRILIN_DPTX_HDCP_BKSV_31_0                              0x430ul
#define TRILIN_DPTX_HDCP_RRX_31_0                               0x430ul
#define TRILIN_DPTX_HDCP_BKSV_63_32                             0x434ul
#define TRILIN_DPTX_HDCP_RRX_63_32                              0x434ul
#define TRILIN_DPTX_HDCP_AKSV_31_0                              0x438ul
#define TRILIN_DPTX_HDCP_AKSV_63_32                             0x43cul
#define TRILIN_DPTX_HDCP_LC128_31_0                             0x440ul
#define TRILIN_DPTX_HDCP_LC128_63_32                            0x444ul
#define TRILIN_DPTX_HDCP_LC128_95_64                            0x448ul
#define TRILIN_DPTX_HDCP_LC128_127_96                           0x44cul
#define TRILIN_DPTX_HDCP_REPEATER                               0x450ul
#define TRILIN_DPTX_HDCP_STREAM_CIPHER_ENABLE                   0x454ul
#define TRILIN_DPTX_HDCP_M0_31_0                                0x458ul
#define TRILIN_DPTX_HDCP_M0_63_32                               0x45cul
#define TRILIN_DPTX_HDCP_AES_INPUT_SELECT                       0x460ul
#define TRILIN_DPTX_HDCP_AES_COUNTER_DISABLE                    0x464ul
#define TRILIN_DPTX_HDCP_AES_COUNTER_ADVANCE                    0x468ul
#define TRILIN_DPTX_HDCP_ECF_31_0                               0x46cul
#define TRILIN_DPTX_HDCP_ECF_63_32                              0x470ul
#define TRILIN_DPTX_HDCP_AES_COUNTER_RESET                      0x474ul
#define TRILIN_DPTX_HDCP_RN_31_0                                0x478ul
#define TRILIN_DPTX_HDCP_RN_63_32                               0x47cul
#define TRILIN_DPTX_HDCP_RNG_CIPHER_STORE_AN                    0x480ul
#define TRILIN_DPTX_HDCP_RNG_CIPHER_AN_31_0                     0x484ul
#define TRILIN_DPTX_HDCP_RNG_CIPHER_AN_63_32                    0x488ul
#define TRILIN_DPTX_HDCP_HOST_TIMER                             0x48cul
#define TRILIN_DPTX_HDCP_ENCRYPTION_STATUS                      0x490ul
#define TRILIN_DPTX_HDCP_RESERVED_494                           0x494ul
#define TRILIN_DPTX_HDCP_CONTENT_TYPE_SELECT_31_0               0x498ul
#define TRILIN_DPTX_HDCP_CONTENT_TYPE_SELECT_63_32              0x49cul

//------------------------------------------------------------------------------
//   MST control
//------------------------------------------------------------------------------
#define TRILIN_DPTX_MST_ENABLE                                  0x500ul
#define TRILIN_DPTX_MST_PID_TABLE_INDEX                         0x504ul
#define TRILIN_DPTX_MST_PID_TABLE_ENTRY                         0x508ul
#define TRILIN_DPTX_SST_SOURCE_SELECT                           0x50cul
#define TRILIN_DPTX_MST_ALLOCATION_TRIGGER                      0x510ul
#define TRILIN_DPTX_MST_PID_TABLE_SELECT                        0x514ul
#define TRILIN_DPTX_MST_ACTIVE_PAYLOAD_TABLE                    0x518ul
#define TRILIN_DPTX_MST_ACTIVE                                  0x520ul
#define TRILIN_DPTX_MST_LINK_FRAME_COUNT                        0x524ul
#define TRILIN_DPTX_MSO_CONFIG                                  0x528ul

//------------------------------------------------------------------------------
//   Main stream control, virtual source 0
//------------------------------------------------------------------------------
#define TRILIN_DPTX_VIDEO_STREAM_ENABLE                         0x800ul
#define TRILIN_DPTX_SECONDARY_STREAM_ENABLE                     0x804ul
#define TRILIN_DPTX_SRC0_SECONDARY_DATA_WINDOW                  0x808ul
#define TRILIN_DPTX_SRC0_INPUT_STATUS                           0x80cul
#define TRILIN_DPTX_SRC0_DATA_CONTROL                           0x810ul
#define TRILIN_DPTX_SRC0_MAIN_STREAM_OVERRIDE                   0x814ul
#define TRILIN_DPTX_SRC0_MAIN_STREAM_HTOTAL                     0x820ul
#define TRILIN_DPTX_SRC0_MAIN_STREAM_VTOTAL                     0x824ul
#define TRILIN_DPTX_SRC0_MAIN_STREAM_POLARITY                   0x828ul
#define TRILIN_DPTX_SRC0_MAIN_STREAM_HSWIDTH                    0x82cul
#define TRILIN_DPTX_SRC0_MAIN_STREAM_VSWIDTH                    0x830ul
#define TRILIN_DPTX_SRC0_MAIN_STREAM_HRES                       0x834ul
#define TRILIN_DPTX_SRC0_MAIN_STREAM_VRES                       0x838ul
#define TRILIN_DPTX_SRC0_MAIN_STREAM_HSTART                     0x83cul
#define TRILIN_DPTX_SRC0_MAIN_STREAM_VSTART                     0x840ul
#define TRILIN_DPTX_SRC0_MAIN_STREAM_MISC0                      0x844ul
#define TRILIN_DPTX_SRC0_MAIN_STREAM_MISC1                      0x848ul
#define TRILIN_DPTX_SRC0_MVID                                   0x84cul
#define TRILIN_DPTX_SRC0_TU_CONFIG                              0x850ul
#define TRILIN_DPTX_SRC0_NVID                                   0x854ul
#define TRILIN_DPTX_SRC0_USER_PIXEL_COUNT                       0x858ul
#define TRILIN_DPTX_SRC0_USER_DATA_COUNT                        0x85cul
#define TRILIN_DPTX_SRC0_MAIN_STREAM_INTERLACED                 0x860ul
#define TRILIN_DPTX_SRC0_USER_SYNC_POLARITY                     0x864ul
#define TRILIN_DPTX_SRC0_USER_CONTROL                           0x868ul
#define TRILIN_DPTX_SRC0_USER_FIFO_STATUS                       0x86cul
#define TRILIN_DPTX_SRC0_FRAMING_STATUS                         0x870ul

//------------------------------------------------------------------------------
//   Secondary channel, source 0
//------------------------------------------------------------------------------
#define TRILIN_DPTX_SEC0_AUDIO_ENABLE                           0x900ul
#define TRILIN_DPTX_SEC0_INPUT_SELECT                           0x904ul
#define TRILIN_DPTX_SEC0_CHANNEL_COUNT                          0x908ul
#define TRILIN_DPTX_SEC0_INFOFRAME_ENABLE                       0x910ul
#define TRILIN_DPTX_SEC0_INFOFRAME_RATE                         0x914ul
#define TRILIN_DPTX_SEC0_MAUD                                   0x918ul
#define TRILIN_DPTX_SEC0_NAUD                                   0x91cul
#define TRILIN_DPTX_SEC0_AUDIO_CLOCK_MODE                       0x920ul
#define TRILIN_DPTX_SEC0_3D_VSC_DATA                            0x924ul
#define TRILIN_DPTX_SEC0_AUDIO_FIFO                             0x928ul
#define TRILIN_DPTX_SEC0_AUDIO_FIFO_LAST                        0x92cul
#define TRILIN_DPTX_SEC0_AUDIO_FIFO_READY                       0x930ul
#define TRILIN_DPTX_SEC0_INFOFRAME_SELECT                       0x934ul
#define TRILIN_DPTX_SEC0_INFOFRAME_DATA                         0x938ul
#define TRILIN_DPTX_SEC0_TIMESTAMP_INTERVAL                     0x93cul
#define TRILIN_DPTX_SEC0_CS_SOURCE_FORMAT                       0x940ul
#define TRILIN_DPTX_SEC0_CS_CATEGORY_CODE                       0x944ul
#define TRILIN_DPTX_SEC0_CS_LENGTH_ORIG_FREQ                    0x948ul
#define TRILIN_DPTX_SEC0_CS_FREQ_CLOCK_ACCURACY                 0x94cul
#define TRILIN_DPTX_SEC0_CS_COPYRIGHT                           0x950ul
#define TRILIN_DPTX_SEC0_AUDIO_CHANNEL_MAP                      0x954ul
#define TRILIN_DPTX_SEC0_AUDIO_FIFO_OVERFLOW                    0x958ul
#define TRILIN_DPTX_SEC0_PACKET_COUNT                           0x95cul
#define TRILIN_DPTX_SEC0_CHANNEL_USER_DATA                      0x960ul
#define TRILIN_DPTX_SEC0_DATA_PACKET_ID                         0x964ul
#define TRILIN_DPTX_SEC0_ADAPTIVE_SYNC_ENABLE                   0x968ul

//------------------------------------------------------------------------------
//   GTC
//------------------------------------------------------------------------------
#define TRILIN_DPTX_GTC_COUNT_CONFIG                            0x980ul
#define TRILIN_DPTX_GTC_COMMAND_EDGE                            0x984ul
#define TRILIN_DPTX_GTC_AUX_FRAME_SYNC                          0x988ul

//------------------------------------------------------------------------------
//   eDP CRC, virtual source 0
//------------------------------------------------------------------------------
#define TRILIN_DPTX_SRC0_EDP_CRC_ENABLE                         0x990ul
#define TRILIN_DPTX_SRC0_EDP_CRC_RED                            0x994ul
#define TRILIN_DPTX_SRC0_EDP_CRC_GREEN                          0x998ul
#define TRILIN_DPTX_SRC0_EDP_CRC_BLUE                           0x99cul

//------------------------------------------------------------------------------
//   PSR/PSR2
//------------------------------------------------------------------------------
#define TRILIN_DPTX_SRC0_PSR_3D_ENABLE                          0x9a0ul
#define TRILIN_DPTX_SRC0_PSR_CONFIG                             0x9a4ul
#define TRILIN_DPTX_SRC0_PSR_STATE                              0x9a8ul
#define TRILIN_DPTX_SRC0_PSR_INTERNAL_STATE                     0x9acul
#define TRILIN_DPTX_SRC0_PSR2_UPDATE_TOP                        0x9b0ul
#define TRILIN_DPTX_SRC0_PSR2_UPDATE_BOTTOM                     0x9b4ul
#define TRILIN_DPTX_SRC0_PSR2_UPDATE_LEFT                       0x9b8ul
#define TRILIN_DPTX_SRC0_PSR2_UPDATE_WIDTH                      0x9bcul

//------------------------------------------------------------------------------
//   Direct SDP interface
//------------------------------------------------------------------------------
#define TRILIN_DPTX_SDP_LANE_SELECT                             0x9e0ul
#define TRILIN_DPTX_SDP_BUFFER_INDEX                            0x9e4ul
#define TRILIN_DPTX_SDP_BUFFER_DATA_COUNT                       0x9e8ul
#define TRILIN_DPTX_SDP_BUFFER_DATA                             0x9ecul
#define TRILIN_DPTX_SDP_BUFFER_DONE                             0x9f0ul
#define TRILIN_DPTX_SDP_BUFFER_BUSY                             0x9f4ul
#define TRILIN_DPTX_SDP_BUFFER_ENABLE                           0x9f8ul

//------------------------------------------------------------------------------
//   MST source 1
//------------------------------------------------------------------------------
#define TRILIN_DPTX_SRC1_VIDEO_STREAM_ENABLE                    0xa00ul
#define TRILIN_DPTX_SRC1_SECONDARY_STREAM_ENABLE                0xa04ul
#define TRILIN_DPTX_SRC1_SECONDARY_DATA_WINDOW                  0xa08ul
#define TRILIN_DPTX_SRC1_INPUT_STATUS                           0xa0cul
#define TRILIN_DPTX_SRC1_DATA_CONTROL                           0xa10ul
#define TRILIN_DPTX_SRC1_MAIN_STREAM_OVERRIDE                   0xa14ul
#define TRILIN_DPTX_SRC1_MAIN_STREAM_HTOTAL                     0xa20ul
#define TRILIN_DPTX_SRC1_MAIN_STREAM_VTOTAL                     0xa24ul
#define TRILIN_DPTX_SRC1_MAIN_STREAM_POLARITY                   0xa28ul
#define TRILIN_DPTX_SRC1_MAIN_STREAM_HSWIDTH                    0xa2cul
#define TRILIN_DPTX_SRC1_MAIN_STREAM_VSWIDTH                    0xa30ul
#define TRILIN_DPTX_SRC1_MAIN_STREAM_HRES                       0xa34ul
#define TRILIN_DPTX_SRC1_MAIN_STREAM_VRES                       0xa38ul
#define TRILIN_DPTX_SRC1_MAIN_STREAM_HSTART                     0xa3cul
#define TRILIN_DPTX_SRC1_MAIN_STREAM_VSTART                     0xa40ul
#define TRILIN_DPTX_SRC1_MAIN_STREAM_MISC0                      0xa44ul
#define TRILIN_DPTX_SRC1_MAIN_STREAM_MISC1                      0xa48ul
#define TRILIN_DPTX_SRC1_MVID                                   0xa4cul
#define TRILIN_DPTX_SRC1_TU_CONFIG                              0xa50ul
#define TRILIN_DPTX_SRC1_NVID                                   0xa54ul
#define TRILIN_DPTX_SRC1_USER_PIXEL_COUNT                       0xa58ul
#define TRILIN_DPTX_SRC1_USER_DATA_COUNT                        0xa5cul
#define TRILIN_DPTX_SRC1_MAIN_STREAM_INTERLACED                 0xa60ul
#define TRILIN_DPTX_SRC1_USER_SYNC_POLARITY                     0xa64ul
#define TRILIN_DPTX_SRC1_USER_CONTROL                           0xa68ul
#define TRILIN_DPTX_SRC1_USER_FIFO_STATUS                       0xa6cul
#define TRILIN_DPTX_SRC1_FRAMING_STATUS                         0xa70ul

//------------------------------------------------------------------------------
//   Secondary channel, source 1
//------------------------------------------------------------------------------
#define TRILIN_DPTX_SEC1_AUDIO_ENABLE                           0xb00ul
#define TRILIN_DPTX_SEC1_INPUT_SELECT                           0xb04ul
#define TRILIN_DPTX_SEC1_CHANNEL_COUNT                          0xb08ul
#define TRILIN_DPTX_SEC1_INFOFRAME_ENABLE                       0xb10ul
#define TRILIN_DPTX_SEC1_INFOFRAME_RATE                         0xb14ul
#define TRILIN_DPTX_SEC1_MAUD                                   0xb18ul
#define TRILIN_DPTX_SEC1_NAUD                                   0xb1cul
#define TRILIN_DPTX_SEC1_AUDIO_CLOCK_MODE                       0xb20ul
#define TRILIN_DPTX_SEC1_3D_VSC_DATA                            0xb24ul
#define TRILIN_DPTX_SEC1_AUDIO_FIFO                             0xb28ul
#define TRILIN_DPTX_SEC1_AUDIO_FIFO_LAST                        0xb2cul
#define TRILIN_DPTX_SEC1_AUDIO_FIFO_READY                       0xb30ul
#define TRILIN_DPTX_SEC1_INFOFRAME_SELECT                       0xb34ul
#define TRILIN_DPTX_SEC1_INFOFRAME_DATA                         0xb38ul
#define TRILIN_DPTX_SEC1_TIMESTAMP_INTERVAL                     0xb3cul
#define TRILIN_DPTX_SEC1_CS_SOURCE_FORMAT                       0xb40ul
#define TRILIN_DPTX_SEC1_CS_CATEGORY_CODE                       0xb44ul
#define TRILIN_DPTX_SEC1_CS_LENGTH_ORIG_FREQ                    0xb48ul
#define TRILIN_DPTX_SEC1_CS_FREQ_CLOCK_ACCURACY                 0xb4cul
#define TRILIN_DPTX_SEC1_CS_COPYRIGHT                           0xb50ul
#define TRILIN_DPTX_SEC1_AUDIO_CHANNEL_MAP                      0xb54ul
#define TRILIN_DPTX_SEC1_AUDIO_FIFO_OVERFLOW                    0xb58ul
#define TRILIN_DPTX_SEC1_PACKET_COUNT                           0xb5cul
#define TRILIN_DPTX_SEC1_GTC_AUX_FRAME_SYNC                     0xb5eul
#define TRILIN_DPTX_SEC1_CHANNEL_USER_DATA                      0xb60ul
#define TRILIN_DPTX_SEC1_DATA_PACKET_ID                         0xb64ul
#define TRILIN_DPTX_SEC1_ADAPTIVE_SYNC_ENABLE                   0xb68ul

//------------------------------------------------------------------------------
//   eDP CRC, virtual source 1
//------------------------------------------------------------------------------
#define TRILIN_DPTX_SRC1_EDP_CRC_ENABLE                         0xb90ul
#define TRILIN_DPTX_SRC1_EDP_CRC_RED                            0xb94ul
#define TRILIN_DPTX_SRC1_EDP_CRC_GREEN                          0xb98ul
#define TRILIN_DPTX_SRC1_EDP_CRC_BLUE                           0xb9cul

//------------------------------------------------------------------------------
//   PSR/PSR2
//------------------------------------------------------------------------------
#define TRILIN_DPTX_SRC1_PSR_3D_ENABLE                          0xba0ul
#define TRILIN_DPTX_SRC1_PSR_CONFIG                             0xba4ul
#define TRILIN_DPTX_SRC1_PSR_STATE                              0xba8ul
#define TRILIN_DPTX_SRC1_PSR_INTERNAL_STATE                     0xbacul
#define TRILIN_DPTX_SRC1_PSR2_UPDATE_TOP                        0xbb0ul
#define TRILIN_DPTX_SRC1_PSR2_UPDATE_BOTTOM                     0xbb4ul
#define TRILIN_DPTX_SRC1_PSR2_UPDATE_LEFT                       0xbb8ul
#define TRILIN_DPTX_SRC1_PSR2_UPDATE_WIDTH                      0xbbcul

#define TRILIN_DPTX_SOURCE_OFFSET (TRILIN_DPTX_SRC1_VIDEO_STREAM_ENABLE \
			- TRILIN_DPTX_VIDEO_STREAM_ENABLE)


#endif	// _TRILIN_DPTX_REG_H_
