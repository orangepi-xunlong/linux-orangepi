//------------------------------------------------------------------------------
//  COPYRIGHT (c) 2018-2022 TRILINEAR TECHNOLOGIES, INC.
//  CONFIDENTIAL AND PROPRIETARY
//
//  THE SOURCE CODE CONTAINED HEREIN IS PROVIDED ON AN "AS IS" BASIS.
//  TRILINEAR TECHNOLOGIES, INC. DISCLAIMS ANY AND ALL WARRANTIES,
//  WHETHER EXPRESS, IMPLIED, OR STATUTORY, INCLUDING ANY IMPLIED
//  WARRANTIES OF MERCHANTABILITY OR OF FITNESS FOR A PARTICULAR PURPOSE.
//  IN NO EVENT SHALL TRILINEAR TECHNOLOGIES, INC. BE LIABLE FOR ANY
//  INCIDENTAL, PUNITIVE, OR CONSEQUENTIAL DAMAGES OF ANY KIND WHATSOEVER
//  ARISING FROM THE USE OF THIS SOURCE CODE.
//
//  THIS DISCLAIMER OF WARRANTY EXTENDS TO THE USER OF THIS SOURCE CODE
//  AND USER'S CUSTOMERS, EMPLOYEES, AGENTS, TRANSFEREES, SUCCESSORS,
//  AND ASSIGNS.
//
//  THIS IS NOT A GRANT OF PATENT RIGHTS
//------------------------------------------------------------------------------
#ifndef __DPTX_REG__
#define __DPTX_REG__

//------------------------------------------------------------------------------
//   link configuration fields
//------------------------------------------------------------------------------
#define TR_DPTX_LINK_BW_SET 0x000ul
#define TR_DPTX_LANE_COUNT_SET 0x004ul
#define TR_DPTX_ENHANCED_FRAMING_ENABLE 0x008ul
#define TR_DPTX_TRAINING_PATTERN_SET 0x00cul
#define TR_DPTX_LINK_QUAL_PATTERN_SET 0x010ul
#define TR_DPTX_DISABLE_SCRAMBLING 0x014ul
#define TR_DPTX_DOWNSPREAD_CONTROL 0x018ul
#define TR_DPTX_EDP_CAPABILITY_CONFIG 0x01cul
#define TR_DPTX_HBR2_SCRAMBLER_RESET 0x020ul
#define TR_DPTX_DISPLAYPORT_VERSION 0x024ul
#define TR_DPTX_ALPM_POWER_SET 0x028ul
#define TR_DPTX_LANE_REMAP 0x02cul
#define TR_DPTX_CUSTOM_80BIT_PATTERN_31_0 0x030ul
#define TR_DPTX_CUSTOM_80BIT_PATTERN_63_32 0x034ul
#define TR_DPTX_CUSTOM_80BIT_PATTERN_79_64 0x038ul

//------------------------------------------------------------------------------
//   core enables
//------------------------------------------------------------------------------
#define TR_DPTX_TRANSMITTER_ENABLE 0x080ul
#define TR_DPTX_SOFT_RESET 0x090ul
#define TR_DPTX_SOURCE_ENABLE 0x094ul
#define TR_DPTX_FEC_ENABLE 0x098ul

//------------------------------------------------------------------------------
//   misc control registers
//------------------------------------------------------------------------------
#define TR_DPTX_FORCE_SCRAMBLER_RESET 0x0c0ul
#define TR_DPTX_CORE_FEATURES 0x0f8ul
#define TR_DPTX_CORE_REVISION 0x0fcul

//------------------------------------------------------------------------------
//   AUX channel
//------------------------------------------------------------------------------
#define TR_DPTX_AUX_COMMAND 0x100ul
#define TR_DPTX_AUX_WRITE_FIFO 0x104ul
#define TR_DPTX_AUX_ADDRESS 0x108ul
#define TR_DPTX_AUX_CLOCK_DIVIDER 0x10cul
#define TR_DPTX_AUX_REPLY_TIMEOUT_INTERVAL 0x110ul
#define TR_DPTX_HPD_INPUT_STATE 0x128ul
#define TR_DPTX_INTERRUPT_STATE 0x130ul
#define TR_DPTX_AUX_REPLY_DATA 0x134ul
#define TR_DPTX_AUX_REPLY_CODE 0x138ul
#define TR_DPTX_AUX_REPLY_COUNT 0x13cul
#define TR_DPTX_INTERRUPT_CAUSE 0x140ul
#define TR_DPTX_INTERRUPT_MASK 0x144ul
#define TR_DPTX_AUX_REPLY_DATA_COUNT 0x148ul
#define TR_DPTX_AUX_STATUS 0x14cul
#define TR_DPTX_AUX_REPLY_CLOCK_WIDTH 0x150ul
#define TR_DPTX_AUX_PHY_WAKE_ACK_DETECTED 0x154ul
#define TR_DPTX_HOST_TIMER 0x158ul
#define TR_DPTX_MST_TIMER 0x15cul

//------------------------------------------------------------------------------
//   PHY status
//------------------------------------------------------------------------------
#define TR_DPTX_PHY_STATUS 0x280ul

//------------------------------------------------------------------------------
//   HDCP control
//------------------------------------------------------------------------------
#define TR_DPTX_HDCP_ENABLE 0x400ul
#define TR_DPTX_HDCP_MODE 0x404ul
#define TR_DPTX_HDCP_KS_31_0 0x408ul
#define TR_DPTX_HDCP_KS_63_32 0x40cul
#define TR_DPTX_HDCP_KM_31_0 0x410ul
#define TR_DPTX_HDCP_KM_63_32 0x414ul
#define TR_DPTX_HDCP_AN_31_0 0x418ul
#define TR_DPTX_HDCP_RTX_31_0 0x418ul
#define TR_DPTX_HDCP_AN_63_32 0x41cul
#define TR_DPTX_HDCP_RTX_63_32 0x41cul
#define TR_DPTX_HDCP_RESERVED_420 0x420ul
#define TR_DPTX_HDCP_AUTH_IN_PROGRESS 0x424ul
#define TR_DPTX_HDCP_R0_STATUS 0x428ul
#define TR_DPTX_HDCP_CIPHER_CONTROL 0x42cul
#define TR_DPTX_HDCP_BKSV_31_0 0x430ul
#define TR_DPTX_HDCP_RRX_31_0 0x430ul
#define TR_DPTX_HDCP_BKSV_63_32 0x434ul
#define TR_DPTX_HDCP_RRX_63_32 0x434ul
#define TR_DPTX_HDCP_AKSV_31_0 0x438ul
#define TR_DPTX_HDCP_AKSV_63_32 0x43cul
#define TR_DPTX_HDCP_LC128_31_0 0x440ul
#define TR_DPTX_HDCP_LC128_63_32 0x444ul
#define TR_DPTX_HDCP_LC128_95_64 0x448ul
#define TR_DPTX_HDCP_LC128_127_96 0x44cul
#define TR_DPTX_HDCP_REPEATER 0x450ul
#define TR_DPTX_HDCP_STREAM_CIPHER_ENABLE 0x454ul
#define TR_DPTX_HDCP_M0_31_0 0x458ul
#define TR_DPTX_HDCP_M0_63_32 0x45cul
#define TR_DPTX_HDCP_AES_INPUT_SELECT 0x460ul
#define TR_DPTX_HDCP_AES_COUNTER_DISABLE 0x464ul
#define TR_DPTX_HDCP_AES_COUNTER_ADVANCE 0x468ul
#define TR_DPTX_HDCP_ECF_31_0 0x46cul
#define TR_DPTX_HDCP_ECF_63_32 0x470ul
#define TR_DPTX_HDCP_AES_COUNTER_RESET 0x474ul
#define TR_DPTX_HDCP_RN_31_0 0x478ul
#define TR_DPTX_HDCP_RN_63_32 0x47cul
#define TR_DPTX_HDCP_RNG_CIPHER_STORE_AN 0x480ul
#define TR_DPTX_HDCP_RNG_CIPHER_AN_31_0 0x484ul
#define TR_DPTX_HDCP_RNG_CIPHER_AN_63_32 0x488ul
#define TR_DPTX_HDCP_HOST_TIMER 0x48cul
#define TR_DPTX_HDCP_ENCRYPTION_STATUS 0x490ul
#define TR_DPTX_HDCP_RESERVED_494 0x494ul
#define TR_DPTX_HDCP_CONTENT_TYPE_SELECT_31_0 0x498ul
#define TR_DPTX_HDCP_CONTENT_TYPE_SELECT_63_32 0x49cul

//------------------------------------------------------------------------------
//   MST control
//------------------------------------------------------------------------------
#define TR_DPTX_MST_ENABLE 0x500ul
#define TR_DPTX_MST_PID_TABLE_INDEX 0x504ul
#define TR_DPTX_MST_PID_TABLE_ENTRY 0x508ul
#define TR_DPTX_SST_SOURCE_SELECT 0x50cul
#define TR_DPTX_MST_ALLOCATION_TRIGGER 0x510ul
#define TR_DPTX_MST_PID_TABLE_SELECT 0x514ul
#define TR_DPTX_MST_ACTIVE_PAYLOAD_TABLE 0x518ul
#define TR_DPTX_MST_ACTIVE 0x520ul
#define TR_DPTX_MST_LINK_FRAME_COUNT 0x524ul
#define TR_DPTX_MSO_CONFIG 0x528ul

//------------------------------------------------------------------------------
//   Main stream control, virtual source 0
//------------------------------------------------------------------------------
#define TR_DPTX_VIDEO_STREAM_ENABLE 0x800ul
#define TR_DPTX_SECONDARY_STREAM_ENABLE 0x804ul
#define TR_DPTX_SRC0_SECONDARY_DATA_WINDOW 0x808ul
#define TR_DPTX_SRC0_INPUT_STATUS 0x80cul
#define TR_DPTX_SRC0_DATA_CONTROL 0x810ul
#define TR_DPTX_SRC0_MAIN_STREAM_OVERRIDE 0x814ul
#define TR_DPTX_SRC0_MAIN_STREAM_HTOTAL 0x820ul
#define TR_DPTX_SRC0_MAIN_STREAM_VTOTAL 0x824ul
#define TR_DPTX_SRC0_MAIN_STREAM_POLARITY 0x828ul
#define TR_DPTX_SRC0_MAIN_STREAM_HSWIDTH 0x82cul
#define TR_DPTX_SRC0_MAIN_STREAM_VSWIDTH 0x830ul
#define TR_DPTX_SRC0_MAIN_STREAM_HRES 0x834ul
#define TR_DPTX_SRC0_MAIN_STREAM_VRES 0x838ul
#define TR_DPTX_SRC0_MAIN_STREAM_HSTART 0x83cul
#define TR_DPTX_SRC0_MAIN_STREAM_VSTART 0x840ul
#define TR_DPTX_SRC0_MAIN_STREAM_MISC0 0x844ul
#define TR_DPTX_SRC0_MAIN_STREAM_MISC1 0x848ul
#define TR_DPTX_SRC0_MVID 0x84cul
#define TR_DPTX_SRC0_TU_CONFIG 0x850ul
#define TR_DPTX_SRC0_NVID 0x854ul
#define TR_DPTX_SRC0_USER_PIXEL_COUNT 0x858ul
#define TR_DPTX_SRC0_USER_DATA_COUNT 0x85cul
#define TR_DPTX_SRC0_MAIN_STREAM_INTERLACED 0x860ul
#define TR_DPTX_SRC0_USER_SYNC_POLARITY 0x864ul
#define TR_DPTX_SRC0_USER_CONTROL 0x868ul
#define TR_DPTX_SRC0_USER_FIFO_STATUS 0x86cul
#define TR_DPTX_SRC0_FRAMING_STATUS 0x870ul

//------------------------------------------------------------------------------
//   Secondary channel, source 0
//------------------------------------------------------------------------------
#define TR_DPTX_SEC0_AUDIO_ENABLE 0x900ul
#define TR_DPTX_SEC0_INPUT_SELECT 0x904ul
#define TR_DPTX_SEC0_CHANNEL_COUNT 0x908ul
#define TR_DPTX_SEC0_INFOFRAME_ENABLE 0x910ul
#define TR_DPTX_SEC0_INFOFRAME_RATE 0x914ul
#define TR_DPTX_SEC0_MAUD 0x918ul
#define TR_DPTX_SEC0_NAUD 0x91cul
#define TR_DPTX_SEC0_AUDIO_CLOCK_MODE 0x920ul
#define TR_DPTX_SEC0_3D_VSC_DATA 0x924ul
#define TR_DPTX_SEC0_AUDIO_FIFO 0x928ul
#define TR_DPTX_SEC0_AUDIO_FIFO_LAST 0x92cul
#define TR_DPTX_SEC0_AUDIO_FIFO_READY 0x930ul
#define TR_DPTX_SEC0_INFOFRAME_SELECT 0x934ul
#define TR_DPTX_SEC0_INFOFRAME_DATA 0x938ul
#define TR_DPTX_SEC0_TIMESTAMP_INTERVAL 0x93cul
#define TR_DPTX_SEC0_CS_SOURCE_FORMAT 0x940ul
#define TR_DPTX_SEC0_CS_CATEGORY_CODE 0x944ul
#define TR_DPTX_SEC0_CS_LENGTH_ORIG_FREQ 0x948ul
#define TR_DPTX_SEC0_CS_FREQ_CLOCK_ACCURACY 0x94cul
#define TR_DPTX_SEC0_CS_COPYRIGHT 0x950ul
#define TR_DPTX_SEC0_AUDIO_CHANNEL_MAP 0x954ul
#define TR_DPTX_SEC0_AUDIO_FIFO_OVERFLOW 0x958ul
#define TR_DPTX_SEC0_PACKET_COUNT 0x95cul
#define TR_DPTX_SEC0_CHANNEL_USER_DATA 0x960ul
#define TR_DPTX_SEC0_DATA_PACKET_ID 0x964ul
#define TR_DPTX_SEC0_ADAPTIVE_SYNC_ENABLE 0x968ul

//------------------------------------------------------------------------------
//   GTC
//------------------------------------------------------------------------------
#define TR_DPTX_GTC_COUNT_CONFIG 0x980ul
#define TR_DPTX_GTC_COMMAND_EDGE 0x984ul
#define TR_DPTX_GTC_AUX_FRAME_SYNC 0x988ul

//------------------------------------------------------------------------------
//   eDP CRC, virtual source 0
//------------------------------------------------------------------------------
#define TR_DPTX_SRC0_EDP_CRC_ENABLE 0x990ul
#define TR_DPTX_SRC0_EDP_CRC_RED 0x994ul
#define TR_DPTX_SRC0_EDP_CRC_GREEN 0x998ul
#define TR_DPTX_SRC0_EDP_CRC_BLUE 0x99cul

//------------------------------------------------------------------------------
//   PSR/PSR2
//------------------------------------------------------------------------------
#define TR_DPTX_SRC0_PSR_3D_ENABLE 0x9a0ul
#define TR_DPTX_SRC0_PSR_CONFIG 0x9a4ul
#define TR_DPTX_SRC0_PSR_STATE 0x9a8ul
#define TR_DPTX_SRC0_PSR_INTERNAL_STATE 0x9acul
#define TR_DPTX_SRC0_PSR2_UPDATE_TOP 0x9b0ul
#define TR_DPTX_SRC0_PSR2_UPDATE_BOTTOM 0x9b4ul
#define TR_DPTX_SRC0_PSR2_UPDATE_LEFT 0x9b8ul
#define TR_DPTX_SRC0_PSR2_UPDATE_WIDTH 0x9bcul

//------------------------------------------------------------------------------
//   Direct SDP interface
//------------------------------------------------------------------------------
#define TR_DPTX_SDP_LANE_SELECT 0x9e0ul
#define TR_DPTX_SDP_BUFFER_INDEX 0x9e4ul
#define TR_DPTX_SDP_BUFFER_DATA_COUNT 0x9e8ul
#define TR_DPTX_SDP_BUFFER_DATA 0x9ecul
#define TR_DPTX_SDP_BUFFER_DONE 0x9f0ul
#define TR_DPTX_SDP_BUFFER_BUSY 0x9f4ul
#define TR_DPTX_SDP_BUFFER_ENABLE 0x9f8ul

//------------------------------------------------------------------------------
//   MST source 1
//------------------------------------------------------------------------------
#define TR_DPTX_SRC1_SECONDARY_DATA_WINDOW 0xa08ul
#define TR_DPTX_SRC1_INPUT_STATUS 0xa0cul
#define TR_DPTX_SRC1_DATA_CONTROL 0xa10ul
#define TR_DPTX_SRC1_MAIN_STREAM_OVERRIDE 0xa14ul
#define TR_DPTX_SRC1_MAIN_STREAM_HTOTAL 0xa20ul
#define TR_DPTX_SRC1_MAIN_STREAM_VTOTAL 0xa24ul
#define TR_DPTX_SRC1_MAIN_STREAM_POLARITY 0xa28ul
#define TR_DPTX_SRC1_MAIN_STREAM_HSWIDTH 0xa2cul
#define TR_DPTX_SRC1_MAIN_STREAM_VSWIDTH 0xa30ul
#define TR_DPTX_SRC1_MAIN_STREAM_HRES 0xa34ul
#define TR_DPTX_SRC1_MAIN_STREAM_VRES 0xa38ul
#define TR_DPTX_SRC1_MAIN_STREAM_HSTART 0xa3cul
#define TR_DPTX_SRC1_MAIN_STREAM_VSTART 0xa40ul
#define TR_DPTX_SRC1_MAIN_STREAM_MISC0 0xa44ul
#define TR_DPTX_SRC1_MAIN_STREAM_MISC1 0xa48ul
#define TR_DPTX_SRC1_MVID 0xa4cul
#define TR_DPTX_SRC1_TU_CONFIG 0xa50ul
#define TR_DPTX_SRC1_NVID 0xa54ul
#define TR_DPTX_SRC1_USER_PIXEL_COUNT 0xa58ul
#define TR_DPTX_SRC1_USER_DATA_COUNT 0xa5cul
#define TR_DPTX_SRC1_MAIN_STREAM_INTERLACED 0xa60ul
#define TR_DPTX_SRC1_USER_SYNC_POLARITY 0xa64ul
#define TR_DPTX_SRC1_USER_CONTROL 0xa68ul
#define TR_DPTX_SRC1_USER_FIFO_STATUS 0xa6cul
#define TR_DPTX_SRC1_FRAMING_STATUS 0xa70ul

//------------------------------------------------------------------------------
//   Secondary channel, source 1
//------------------------------------------------------------------------------
#define TR_DPTX_SEC1_AUDIO_ENABLE 0xb00ul
#define TR_DPTX_SEC1_INPUT_SELECT 0xb04ul
#define TR_DPTX_SEC1_CHANNEL_COUNT 0xb08ul
#define TR_DPTX_SEC1_INFOFRAME_ENABLE 0xb10ul
#define TR_DPTX_SEC1_INFOFRAME_RATE 0xb14ul
#define TR_DPTX_SEC1_MAUD 0xb18ul
#define TR_DPTX_SEC1_NAUD 0xb1cul
#define TR_DPTX_SEC1_AUDIO_CLOCK_MODE 0xb20ul
#define TR_DPTX_SEC1_3D_VSC_DATA 0xb24ul
#define TR_DPTX_SEC1_AUDIO_FIFO 0xb28ul
#define TR_DPTX_SEC1_AUDIO_FIFO_LAST 0xb2cul
#define TR_DPTX_SEC1_AUDIO_FIFO_READY 0xb30ul
#define TR_DPTX_SEC1_INFOFRAME_SELECT 0xb34ul
#define TR_DPTX_SEC1_INFOFRAME_DATA 0xb38ul
#define TR_DPTX_SEC1_TIMESTAMP_INTERVAL 0xb3cul
#define TR_DPTX_SEC1_CS_SOURCE_FORMAT 0xb40ul
#define TR_DPTX_SEC1_CS_CATEGORY_CODE 0xb44ul
#define TR_DPTX_SEC1_CS_LENGTH_ORIG_FREQ 0xb48ul
#define TR_DPTX_SEC1_CS_FREQ_CLOCK_ACCURACY 0xb4cul
#define TR_DPTX_SEC1_CS_COPYRIGHT 0xb50ul
#define TR_DPTX_SEC1_AUDIO_CHANNEL_MAP 0xb54ul
#define TR_DPTX_SEC1_AUDIO_FIFO_OVERFLOW 0xb58ul
#define TR_DPTX_SEC1_PACKET_COUNT 0xb5cul
#define TR_DPTX_SEC1_GTC_AUX_FRAME_SYNC 0xb5eul
#define TR_DPTX_SEC1_CHANNEL_USER_DATA 0xb60ul
#define TR_DPTX_SEC1_DATA_PACKET_ID 0xb64ul
#define TR_DPTX_SEC1_ADAPTIVE_SYNC_ENABLE 0xb68ul

//------------------------------------------------------------------------------
//   eDP CRC, virtual source 1
//------------------------------------------------------------------------------
#define TR_DPTX_SRC1_EDP_CRC_ENABLE 0xb90ul
#define TR_DPTX_SRC1_EDP_CRC_RED 0xb94ul
#define TR_DPTX_SRC1_EDP_CRC_GREEN 0xb98ul
#define TR_DPTX_SRC1_EDP_CRC_BLUE 0xb9cul

//------------------------------------------------------------------------------
//   PSR/PSR2
//------------------------------------------------------------------------------
#define TR_DPTX_SRC1_PSR_3D_ENABLE 0xba0ul
#define TR_DPTX_SRC1_PSR_CONFIG 0xba4ul
#define TR_DPTX_SRC1_PSR_STATE 0xba8ul
#define TR_DPTX_SRC1_PSR_INTERNAL_STATE 0xbacul
#define TR_DPTX_SRC1_PSR2_UPDATE_TOP 0xbb0ul
#define TR_DPTX_SRC1_PSR2_UPDATE_BOTTOM 0xbb4ul
#define TR_DPTX_SRC1_PSR2_UPDATE_LEFT 0xbb8ul
#define TR_DPTX_SRC1_PSR2_UPDATE_WIDTH 0xbbcul

//------------------------------------------------------------------------------
//   MST source 2
//------------------------------------------------------------------------------
#define TR_DPTX_SRC2_SECONDARY_DATA_WINDOW 0xc08ul
#define TR_DPTX_SRC2_INPUT_STATUS 0xc0cul
#define TR_DPTX_SRC2_DATA_CONTROL 0xc10ul
#define TR_DPTX_SRC2_MAIN_STREAM_OVERRIDE 0xc14ul
#define TR_DPTX_SRC2_MAIN_STREAM_HTOTAL 0xc20ul
#define TR_DPTX_SRC2_MAIN_STREAM_VTOTAL 0xc24ul
#define TR_DPTX_SRC2_MAIN_STREAM_POLARITY 0xc28ul
#define TR_DPTX_SRC2_MAIN_STREAM_HSWIDTH 0xc2cul
#define TR_DPTX_SRC2_MAIN_STREAM_VSWIDTH 0xc30ul
#define TR_DPTX_SRC2_MAIN_STREAM_HRES 0xc34ul
#define TR_DPTX_SRC2_MAIN_STREAM_VRES 0xc38ul
#define TR_DPTX_SRC2_MAIN_STREAM_HSTART 0xc3cul
#define TR_DPTX_SRC2_MAIN_STREAM_VSTART 0xc40ul
#define TR_DPTX_SRC2_MAIN_STREAM_MISC0 0xc44ul
#define TR_DPTX_SRC2_MAIN_STREAM_MISC1 0xc48ul
#define TR_DPTX_SRC2_MVID 0xc4cul
#define TR_DPTX_SRC2_TU_CONFIG 0xc50ul
#define TR_DPTX_SRC2_NVID 0xc54ul
#define TR_DPTX_SRC2_USER_PIXEL_COUNT 0xc58ul
#define TR_DPTX_SRC2_USER_DATA_COUNT 0xc5cul
#define TR_DPTX_SRC2_MAIN_STREAM_INTERLACED 0xc60ul
#define TR_DPTX_SRC2_USER_SYNC_POLARITY 0xc64ul
#define TR_DPTX_SRC2_USER_CONTROL 0xc68ul
#define TR_DPTX_SRC2_USER_FIFO_STATUS 0xc6cul
#define TR_DPTX_SRC2_FRAMING_STATUS 0xc70ul

//------------------------------------------------------------------------------
//   Secondary channel, source 2
//------------------------------------------------------------------------------
#define TR_DPTX_SEC2_AUDIO_ENABLE 0xd00ul
#define TR_DPTX_SEC2_INPUT_SELECT 0xd04ul
#define TR_DPTX_SEC2_CHANNEL_COUNT 0xd08ul
#define TR_DPTX_SEC2_INFOFRAME_ENABLE 0xd10ul
#define TR_DPTX_SEC2_INFOFRAME_RATE 0xd14ul
#define TR_DPTX_SEC2_MAUD 0xd18ul
#define TR_DPTX_SEC2_NAUD 0xd1cul
#define TR_DPTX_SEC2_AUDIO_CLOCK_MODE 0xd20ul
#define TR_DPTX_SEC2_3D_VSC_DATA 0xd24ul
#define TR_DPTX_SEC2_AUDIO_FIFO 0xd28ul
#define TR_DPTX_SEC2_AUDIO_FIFO_LAST 0xd2cul
#define TR_DPTX_SEC2_AUDIO_FIFO_READY 0xd30ul
#define TR_DPTX_SEC2_INFOFRAME_SELECT 0xd34ul
#define TR_DPTX_SEC2_INFOFRAME_DATA 0xd38ul
#define TR_DPTX_SEC2_TIMESTAMP_INTERVAL 0xd3cul
#define TR_DPTX_SEC2_CS_SOURCE_FORMAT 0xd40ul
#define TR_DPTX_SEC2_CS_CATEGORY_CODE 0xd44ul
#define TR_DPTX_SEC2_CS_LENGTH_ORIG_FREQ 0xd48ul
#define TR_DPTX_SEC2_CS_FREQ_CLOCK_ACCURACY 0xd4cul
#define TR_DPTX_SEC2_CS_COPYRIGHT 0xd50ul
#define TR_DPTX_SEC2_AUDIO_CHANNEL_MAP 0xd54ul
#define TR_DPTX_SEC2_AUDIO_FIFO_OVERFLOW 0xd58ul
#define TR_DPTX_SEC2_PACKET_COUNT 0xd5cul
#define TR_DPTX_SEC2_GTC_AUX_FRAME_SYNC 0xd5eul
#define TR_DPTX_SEC2_CHANNEL_USER_DATA 0xd60ul
#define TR_DPTX_SEC2_DATA_PACKET_ID 0xd64ul
#define TR_DPTX_SEC2_ADAPTIVE_SYNC_ENABLE 0xd68ul

//------------------------------------------------------------------------------
//   eDP CRC, virtual source 2
//------------------------------------------------------------------------------
#define TR_DPTX_SRC2_EDP_CRC_ENABLE 0xd90ul
#define TR_DPTX_SRC2_EDP_CRC_RED 0xd94ul
#define TR_DPTX_SRC2_EDP_CRC_GREEN 0xd98ul
#define TR_DPTX_SRC2_EDP_CRC_BLUE 0xd9cul

//------------------------------------------------------------------------------
//   PSR/PSR2
//------------------------------------------------------------------------------
#define TR_DPTX_SRC2_PSR_3D_ENABLE 0xda0ul
#define TR_DPTX_SRC2_PSR_CONFIG 0xda4ul
#define TR_DPTX_SRC2_PSR_STATE 0xda8ul
#define TR_DPTX_SRC2_PSR_INTERNAL_STATE 0xdacul
#define TR_DPTX_SRC2_PSR2_UPDATE_TOP 0xdb0ul
#define TR_DPTX_SRC2_PSR2_UPDATE_BOTTOM 0xdb4ul
#define TR_DPTX_SRC2_PSR2_UPDATE_LEFT 0xdb8ul
#define TR_DPTX_SRC2_PSR2_UPDATE_WIDTH 0xdbcul

//------------------------------------------------------------------------------
//   MST source 3
//------------------------------------------------------------------------------
#define TR_DPTX_SRC3_SECONDARY_DATA_WINDOW 0xe08ul
#define TR_DPTX_SRC3_INPUT_STATUS 0xe0cul
#define TR_DPTX_SRC3_DATA_CONTROL 0xe10ul
#define TR_DPTX_SRC3_MAIN_STREAM_OVERRIDE 0xe14ul
#define TR_DPTX_SRC3_MAIN_STREAM_HTOTAL 0xe20ul
#define TR_DPTX_SRC3_MAIN_STREAM_VTOTAL 0xe24ul
#define TR_DPTX_SRC3_MAIN_STREAM_POLARITY 0xe28ul
#define TR_DPTX_SRC3_MAIN_STREAM_HSWIDTH 0xe2cul
#define TR_DPTX_SRC3_MAIN_STREAM_VSWIDTH 0xe30ul
#define TR_DPTX_SRC3_MAIN_STREAM_HRES 0xe34ul
#define TR_DPTX_SRC3_MAIN_STREAM_VRES 0xe38ul
#define TR_DPTX_SRC3_MAIN_STREAM_HSTART 0xe3cul
#define TR_DPTX_SRC3_MAIN_STREAM_VSTART 0xe40ul
#define TR_DPTX_SRC3_MAIN_STREAM_MISC0 0xe44ul
#define TR_DPTX_SRC3_MAIN_STREAM_MISC1 0xe48ul
#define TR_DPTX_SRC3_MVID 0xe4cul
#define TR_DPTX_SRC3_TU_CONFIG 0xe50ul
#define TR_DPTX_SRC3_NVID 0xe54ul
#define TR_DPTX_SRC3_USER_PIXEL_COUNT 0xe58ul
#define TR_DPTX_SRC3_USER_DATA_COUNT 0xe5cul
#define TR_DPTX_SRC3_MAIN_STREAM_INTERLACED 0xe60ul
#define TR_DPTX_SRC3_USER_SYNC_POLARITY 0xe64ul
#define TR_DPTX_SRC3_USER_CONTROL 0xe68ul
#define TR_DPTX_SRC3_USER_FIFO_STATUS 0xe6cul
#define TR_DPTX_SRC3_FRAMING_STATUS 0xe70ul

//------------------------------------------------------------------------------
//   Secondary channel, source 3
//------------------------------------------------------------------------------
#define TR_DPTX_SEC3_AUDIO_ENABLE 0xf00ul
#define TR_DPTX_SEC3_INPUT_SELECT 0xf04ul
#define TR_DPTX_SEC3_CHANNEL_COUNT 0xf08ul
#define TR_DPTX_SEC3_INFOFRAME_ENABLE 0xf10ul
#define TR_DPTX_SEC3_INFOFRAME_RATE 0xf14ul
#define TR_DPTX_SEC3_MAUD 0xf18ul
#define TR_DPTX_SEC3_NAUD 0xf1cul
#define TR_DPTX_SEC3_AUDIO_CLOCK_MODE 0xf20ul
#define TR_DPTX_SEC3_3D_VSC_DATA 0xf24ul
#define TR_DPTX_SEC3_AUDIO_FIFO 0xf28ul
#define TR_DPTX_SEC3_AUDIO_FIFO_LAST 0xf2cul
#define TR_DPTX_SEC3_AUDIO_FIFO_READY 0xf30ul
#define TR_DPTX_SEC3_INFOFRAME_SELECT 0xf34ul
#define TR_DPTX_SEC3_INFOFRAME_DATA 0xf38ul
#define TR_DPTX_SEC3_TIMESTAMP_INTERVAL 0xf3cul
#define TR_DPTX_SEC3_CS_SOURCE_FORMAT 0xf40ul
#define TR_DPTX_SEC3_CS_CATEGORY_CODE 0xf44ul
#define TR_DPTX_SEC3_CS_LENGTH_ORIG_FREQ 0xf48ul
#define TR_DPTX_SEC3_CS_FREQ_CLOCK_ACCURACY 0xf4cul
#define TR_DPTX_SEC3_CS_COPYRIGHT 0xf50ul
#define TR_DPTX_SEC3_AUDIO_CHANNEL_MAP 0xf54ul
#define TR_DPTX_SEC3_AUDIO_FIFO_OVERFLOW 0xf58ul
#define TR_DPTX_SEC3_PACKET_COUNT 0xf5cul
#define TR_DPTX_SEC3_GTC_AUX_FRAME_SYNC 0xf5eul
#define TR_DPTX_SEC3_CHANNEL_USER_DATA 0xf60ul
#define TR_DPTX_SEC3_DATA_PACKET_ID 0xf64ul
#define TR_DPTX_SEC3_ADAPTIVE_SYNC_ENABLE 0xf68ul

//------------------------------------------------------------------------------
//   eDP CRC, virtual source 3
//------------------------------------------------------------------------------
#define TR_DPTX_SRC3_EDP_CRC_ENABLE 0xf90ul
#define TR_DPTX_SRC3_EDP_CRC_RED 0xf94ul
#define TR_DPTX_SRC3_EDP_CRC_GREEN 0xf98ul
#define TR_DPTX_SRC3_EDP_CRC_BLUE 0xf9cul

//------------------------------------------------------------------------------
//   PSR/PSR2
//------------------------------------------------------------------------------
#define TR_DPTX_SRC3_PSR_3D_ENABLE 0xfa0ul
#define TR_DPTX_SRC3_PSR_CONFIG 0xfa4ul
#define TR_DPTX_SRC3_PSR_STATE 0xfa8ul
#define TR_DPTX_SRC3_PSR_INTERNAL_STATE 0xfacul
#define TR_DPTX_SRC3_PSR2_UPDATE_TOP 0xfb0ul
#define TR_DPTX_SRC3_PSR2_UPDATE_BOTTOM 0xfb4ul
#define TR_DPTX_SRC3_PSR2_UPDATE_LEFT 0xfb8ul
#define TR_DPTX_SRC3_PSR2_UPDATE_WIDTH 0xfbcul
#endif
