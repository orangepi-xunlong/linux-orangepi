/*
 * Broadcom Dongle Host Driver (DHD), Linux-specific network interface.
 * Basically selected code segments from usb-cdc.c and usb-rndis.c
 *
 * Copyright (C) 2024 Synaptics Incorporated. All rights reserved.
 *
 * This software is licensed to you under the terms of the
 * GNU General Public License version 2 (the "GPL") with Broadcom special exception.
 *
 * INFORMATION CONTAINED IN THIS DOCUMENT IS PROVIDED "AS-IS," AND SYNAPTICS
 * EXPRESSLY DISCLAIMS ALL EXPRESS AND IMPLIED WARRANTIES, INCLUDING ANY
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE,
 * AND ANY WARRANTIES OF NON-INFRINGEMENT OF ANY INTELLECTUAL PROPERTY RIGHTS.
 * IN NO EVENT SHALL SYNAPTICS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, PUNITIVE, OR CONSEQUENTIAL DAMAGES ARISING OUT OF OR IN CONNECTION
 * WITH THE USE OF THE INFORMATION CONTAINED IN THIS DOCUMENT, HOWEVER CAUSED
 * AND BASED ON ANY THEORY OF LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * NEGLIGENCE OR OTHER TORTIOUS ACTION, AND EVEN IF SYNAPTICS WAS ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE. IF A TRIBUNAL OF COMPETENT JURISDICTION
 * DOES NOT PERMIT THE DISCLAIMER OF DIRECT DAMAGES OR ANY OTHER DAMAGES,
 * SYNAPTICS' TOTAL CUMULATIVE LIABILITY TO ANY PARTY SHALL NOT
 * EXCEED ONE HUNDRED U.S. DOLLARS
 *
 * Copyright (C) 2024, Broadcom.
 *
 *      Unless you and Broadcom execute a separate written software license
 * agreement governing use of this software, this software is licensed to you
 * under the terms of the GNU General Public License version 2 (the "GPL"),
 * available at http://www.broadcom.com/licenses/GPLv2.php, with the
 * following added to such license:
 *
 *      As a special exception, the copyright holders of this software give you
 * permission to link this software with independent modules, and to copy and
 * distribute the resulting executable under terms of your choice, provided that
 * you also meet, for each linked independent module, the terms and conditions of
 * the license of that module.  An independent module is a module which is not
 * derived from this software.  The special exception does not apply to any
 * modifications of the software.
 *
 *
 * <<Broadcom-WL-IPTag/Open:>>
 *
 * $Id$
 */

#ifndef __DHD_CSI_H__
#define __DHD_CSI_H__

#ifdef CSI_SUPPORT

/* Different FW using different CSI data formats, and here
 * the  DHD driver will directly convert input DATA to the
 * common header format(latest full set version CSI header
 * format which contains much more items), and then it will
 * be converted to corresponding version CSI header which
 * applicaiton required.
 *
 *        FW                   DHD-Driver                 Application
 * dhd_cfr_header_rev_0 \
 * dhd_cfr_header_rev_1  \                              / syna_csi_header_v1
 * dhd_cfr_header_rev_2   --> syna_csi_common_header -->  syna_csi_header_v2
 * dhd_cfr_header_rev_3  /                              \      ...
 *     ...           /
 */

/*** definition for structure and API between APP and driver */
enum eSyna_csi_data_mode_type {
	SYNA_CSI_DATA_MODE_NONE = 0,
	/* upper layer will fetch data by sysfs */
	SYNA_CSI_DATA_MODE_SYSFS,
	/* upper layer will fetch data by UDP socket */
	SYNA_CSI_DATA_MODE_UDP_SOCKET,
	SYNA_CSI_DATA_MODE_LAST
};

enum syna_csi_format_type {
	SYNA_CSI_FORMAT_UNKNOWN = 0,
	SYNA_CSI_FORMAT_Q8,  /* (9,9,5) Q8 floating point
	                      * BCM4358/4359/4375x
	                      */
	SYNA_CSI_FORMAT_Q11, /* (12,12,6) Q11 floating point
	                      * BCM4360/4366
	                      */
	SYNA_CSI_FORMAT_Q12, /* (13,13,0) Q12 fixed point
	                      */
	SYNA_CSI_FORMAT_Q13, /* (14,14,0) Q13 fixed point
	                      * BCM4339/4345x/43013x
	                      */
	SYNA_CSI_FORMAT_LAST
};

/* (9,9,5) Q8 floating point data format */
#define _Q8_REAL_MA(t)         (0x000ff &((t)>>14))
#define _Q8_REAL_SI(t)         (0x00001 &((t)>>22))
#define _Q8_IMAGINARY_MA(t)    (0x000ff &((t)>> 5))
#define _Q8_IMAGINARY_SI(t)    (0x00001 &((t)>>13))
#define SYNA_CSI_DATA_Q8_REAL(t) \
(((0 < _Q8_REAL_SI(t)) \
	? (-1) \
	:  (1)) \
	* ((int32)_Q8_REAL_MA(t)) \
)

#define SYNA_CSI_DATA_Q8_IMAGINARY(t) \
(((0 < _Q8_IMAGINARY_SI(t)) \
	? (-1) \
	:  (1)) \
	* ((int32)_Q8_IMAGINARY_MA(t)) \
)

#define SYNA_CSI_DATA_Q8_EXP(t) \
(((int8)(0xf8 \
	& ((t) << 3))) \
	>> 3 \
)

/* (12,12,6) Q11 floating point format */
#define _Q11_REAL_MA(t)         (0x007ff &((t)>>18))
#define _Q11_REAL_SI(t)         (0x00001 &((t)>>29))
#define _Q11_IMAGINARY_MA(t)    (0x007ff &((t)>> 6))
#define _Q11_IMAGINARY_SI(t)    (0x00001 &((t)>>17))
#define SYNA_CSI_DATA_Q11_REAL(t) \
(((0 < _Q11_REAL_SI(t)) \
	? (-1) \
	: (1)) \
	* ((int32)_Q11_REAL_MA(t)) \
)

#define SYNA_CSI_DATA_Q11_IMAGINARY(t) \
(((0 < _Q11_IMAGINARY_SI(t)) \
	? (-1) \
	: (1)) \
	* ((int32)_Q11_IMAGINARY_MA(t)) \
)

#define SYNA_CSI_DATA_Q11_EXP(t) \
(((int8)(0xfc \
	& ((t) << 2))) \
	>> 2 \
)

/* (13,13,0) Q12 fixed point data format */
#define SYNA_CSI_DATA_Q12_REAL(t) \
(((int32)(0xfffc0000 \
	& ((t) << 6))) \
	>> (13 + 6) \
)

#define SYNA_CSI_DATA_Q12_IMAGINARY(t) \
(((int32)(0xfffc0000 \
	& ((t) << 19))) \
	>> (0 + 19) \
)

/* (14,14,0) Q13 fixed point data format */
#define SYNA_CSI_DATA_Q13_REAL(t) \
(((int32)(0xfffc0000 \
	& ((t) << 4))) \
	>> (14 + 4) \
)

#define SYNA_CSI_DATA_Q13_IMAGINARY(t) \
(((int32)(0xfffc0000 \
	& ((t) << 18))) \
	>> (0 + 18) \
)

/* 8bits */
enum syna_csi_legacy_error_flag {
	/* bit 0 ~ bit 7 are for error information */
	SYNA_CSI_LEGACY_FLAG_ERROR_MASK = 0x007F,
	SYNA_CSI_LEGACY_FLAG_ERROR_CHECKSUM = (1 << 0),
	SYNA_CSI_LEGACY_FLAG_ERROR_GENERIC = (1 << 1),
	SYNA_CSI_LEGACY_FLAG_ERROR_NO_ACK = (1 << 2),
	SYNA_CSI_LEGACY_FLAG_ERROR_READ = (1 << 3),
	SYNA_CSI_LEGACY_FLAG_ERROR_PS = (1 << 4),
	/* subcarrier real position along FFT index: (total N=2*M subcarries)
	 * -M, -(M-1), -(M-2), -(M-3),...,-1, 0, 1, 2,...M-1, M
	 *
	 *  Data Output order with positive part first(*default*):
	 * 0, 1, 2,...M-1, M,	-M, -(M-1), -(M-2), -(M-3),...,-1,
	 *
	 *  Data Output order with negative part first:
	 * -M, -(M-1), -(M-2), -(M-3),...,-1, 0, 1, 2,...M-1, M
	 */
	SYNA_CSI_LEGACY_FLAG_FFT_NEGATIVE_FIRST = (1 << 7),
	SYNA_CSI_LEGACY_FLAG_ERROR_LAST
};

/* 16 bits */
enum syna_csi_error_flag {
	SYNA_CSI_FLAG_ERROR_NONE = 0x0,
	SYNA_CSI_FLAG_ERROR_FAILURE = 1,
	SYNA_CSI_FLAG_ERROR_CHECKSUM = 2,
	SYNA_CSI_FLAG_ERROR_GENERIC = 3,
	SYNA_CSI_FLAG_ERROR_OTHER = 4,

	SYNA_CSI_FLAG_ERROR_WRONG_CHANNEL = 10,
	SYNA_CSI_FLAG_ERROR_WRONG_MODE = 11,

	SYNA_CSI_FLAG_ERROR_TX_FAIL = 20,
	SYNA_CSI_FLAG_ERROR_TX_NO_ACK = 21,

	SYNA_CSI_FLAG_ERROR_DATA_FEEDBACK_MISS = 30,
	SYNA_CSI_FLAG_ERROR_TX_FEEDBACK_MISS = 31,
	SYNA_CSI_FLAG_ERROR_POWERSAVING = 32,

	/* active mode suppress */
	SYNA_CSI_FLAG_ERROR_SUPPRESS = 100,
	SYNA_CSI_FLAG_ERROR_SUPP_PMQ = 101,
	SYNA_CSI_FLAG_ERROR_SUPP_FLUSH = 102,
	SYNA_CSI_FLAG_ERROR_SUPP_FRAG = 103,
	SYNA_CSI_FLAG_ERROR_SUPP_TBTT = 104,
	SYNA_CSI_FLAG_ERROR_SUPP_BADCH = 105,
	SYNA_CSI_FLAG_ERROR_SUPP_EXPTIME = 106,
	SYNA_CSI_FLAG_ERROR_SUPP_UF = 107,
	SYNA_CSI_FLAG_ERROR_SUPP_ABS = 108,
	SYNA_CSI_FLAG_ERROR_SUPP_PPS = 109,
	SYNA_CSI_FLAG_ERROR_SUPP_NOKEY = 110,
	SYNA_CSI_FLAG_ERROR_SUPP_DMA_XFER = 111,
	SYNA_CSI_FLAG_ERROR_SUPP_TWT = 112,

	SYNA_CSI_FLAG_ERROR_LAST
};

/* 16 bits */
enum syna_csi_feature_flag {
	SYNA_CSI_FLAG_FEATURE_NONE = 0x0,
	/* subcarrier real position along FFT index: (total N=2*M subcarries)
	 * -M, -(M-1), -(M-2), -(M-3),...,-1, 0, 1, 2,...M-1, M
	 *
	 *  Data Output order with positive part first(*default*):
	 * 0, 1, 2,...M-1, M,   -M, -(M-1), -(M-2), -(M-3),...,-1,
	 *
	 *  Data Output order with negative part first:
	 * -M, -(M-1), -(M-2), -(M-3),...,-1, 0, 1, 2,...M-1, M
	 */
	SYNA_CSI_FLAG_FEATURE_FFT_NEGATIVE_FIRST = (0x1 << 0),
	SYNA_CSI_FLAG_FEATURE_LAST
};

enum syna_csi_dot11_mode {
	SYNA_CSI_DOT11_UNKNOW,
	SYNA_CSI_DOT11_B,      /* 11b, CCK */
	SYNA_CSI_DOT11_G,      /* 11g, legacy OFDM */
	SYNA_CSI_DOT11_A,      /* 11a, legacy OFDM */
	SYNA_CSI_DOT11_HT,     /* 11n, HT */
	SYNA_CSI_DOT11_VHT,    /* 11ac, VHT */
	SYNA_CSI_DOT11_HE,     /* 11ax, HE */
	SYNA_CSI_DOT11_EHT,    /* 11be, EHT */
	SYNA_CSI_DOT11_LAST
};

enum syna_csi_band_type {
	SYNA_CSI_BAND_UNKNOWN = 0,
	SYNA_CSI_BAND_2G,
	SYNA_CSI_BAND_5G,
	SYNA_CSI_BAND_6G,
	SYNA_CSI_BAND_60G,
	SYNA_CSI_BAND_LAST
};

enum syna_csi_sideband_type {
	SYNA_CSI_SB_NONE = 0x0,
	SYNA_CSI_SB_LLL  = (0x1 << 0),
	SYNA_CSI_SB_LLU  = (0x1 << 1),
	SYNA_CSI_SB_LUL  = (0x1 << 2),
	SYNA_CSI_SB_LUU  = (0x1 << 3),
	SYNA_CSI_SB_ULL  = (0x1 << 4),
	SYNA_CSI_SB_ULU  = (0x1 << 5),
	SYNA_CSI_SB_UUL  = (0x1 << 6),
	SYNA_CSI_SB_UUU  = (0x1 << 7),
	SYNA_CSI_SB_FULL = (0x1 << 16)
};

/* 32bits */
enum syna_csi_rate_type {
	SYNA_CSI_RATE_VALUE_MASK = 0x000000FF,
	SYNA_CSI_RATE_MODE_MASK  = 0xFF000000,
	SYNA_CSI_RATE_LEGACY     = 0x01000000, /* value part show as 500K unit,
	                                        * for example,
	                                        * 0x0100000C => 0x0C => 12 => 6Mbps
	                                        */
	SYNA_CSI_RATE_OFDM       = 0x02000000, /* value part show as 500K unit,
	                                        * for example,
	                                        * 0x02000030 => 0x30 => 48 => 24Mbps
	                                        */
	SYNA_CSI_RATE_HT_MCS     = 0x03000000, /* value part shown as HT MCS
	                                        * index, for example,
	                                        * 0x03000007 => HT 0x07 => HT mcs 7
	                                        */
	SYNA_CSI_RATE_VHT_MCS    = 0x04000000, /* value part shown as VHT MCS
	                                        * index, for example,
	                                        * 0x04000007 => VHT 0x07 => VHT mcs 7
	                                        */
	SYNA_CSI_RATE_HE_MCS     = 0x05000000, /* value part shown as HE MCS
	                                        * index, for example,
	                                        * 0x05000007 => HE 0x07 => HE mcs 7
	                                        */
	SYNA_CSI_RATE_EHT_MCS    = 0x06000000, /* value part shown as EHT MCS
	                                        * index, for example,
	                                        * 0x06000007 => EHT 0x07 => EHT mcs 7
	                                        */
	SYNA_CSI_RATE_FLAG_MASK  = 0x00FFFF00,
	SYNA_CSI_RATE_LDPC       = 0x00000100,
	SYNA_CSI_RATE_STBC       = 0x00000200,
	SYNA_CSI_RATE_TXBF       = 0x00000400,
	SYNA_CSI_RATE_DCM        = 0x00000800,
	SYNA_CSI_RATE_GI_SHIFT   = 14,
	SYNA_CSI_RATE_GI_MASK    = 0x00003000,
	SYNA_CSI_RATE_GI_1       = 0x00001000,
	SYNA_CSI_RATE_GI_2       = 0x00002000,
	SYNA_CSI_RATE_GI_3       = 0x00003000
};

enum syna_csi_bandwidth_type {
	SYNA_CSI_BW_UNKNOWN = 0x0,
	SYNA_CSI_BW_5MHz    = (0x1 << 0),
	SYNA_CSI_BW_10MHz   = (0x1 << 1),
	SYNA_CSI_BW_20MHz   = (0x1 << 2),
	SYNA_CSI_BW_40MHz   = (0x1 << 3),
	SYNA_CSI_BW_80MHz   = (0x1 << 4),
	SYNA_CSI_BW_160MHz  = (0x1 << 5),
	SYNA_CSI_BW_320MHz  = (0x1 << 6)
};

/* 'SYNA' -> 'S'53, 'Y'59, 'N'4E, 'A'41 */
#define CONST_SYNA_CSI_MAGIC_FLAG               0x414E5953

/*** CSI API common header structure (8 bytes alignment for 64bit system) */
/* Version 1 'syna_csi_header' (8 bytes alignment for 64bit system) */
#define CONST_SYNA_CSI_HEADER_VERSION_V1    0x01
struct syna_csi_header_v1 {
	/* byte 0 ~ 3 */
	uint32  magic_flag;       /* flag for protecting and detecting header */

	/* byte 4 ~ 7 */
	uint8  version;           /* header version of this structure */
	uint8  format_type;       /* data format type of current CSI packet,
	                           * enumerated in 'syna_csi_format_type'
	                           */
	uint8  fc;                /* packet frame control type of current
	                           * CSI data for further checking if it
	                           * is correct expected result
	                           */
	uint8  flags;             /* flag may use for extra emendation
	                           * which enumerated as 'syna_csi_flag'
	                           */

	/* byte 8 ~ 19 */
	uint8  client_ea[6];      /* client MAC address */
	uint8  bsscfg_ea[6];      /* BSSCFG address of current interface */

	/* byte 20 ~ 23 */
	uint8  band;              /* enumerated in 'syna_csi_band_type' */
	uint8  bandwidth;         /* enumerated in 'syna_csi_bandwidth_type' */
	uint16 channel;           /* channel number of corresponding band */

	/* byte 24 ~ 31 */
	uint64 report_tsf;        /* current CSI DATA RX timestamp */
	/* byte 32 ~ 35 */

	uint8  num_txstream;      /* peer side TX spatial steams */
	uint8  num_rxchain;       /* number of RX side chain/antenna */
	uint16 num_subcarrier;    /* Number of subcarrier */

	/* byte 36 ~ 39 */
	int8   rssi;              /* average RSSI, and goto check the
	                           * 'rssi_ant' if this 'rssi' is zero
	                           */
	int8   noise;             /* average noise */
	int16  global_id;         /* CSI frame global ID */

	/* byte 40 ~ 47 */
	int8   rssi_ant[8];       /* RSSI of each RX chain/antenna
	                           * (Depends on solution, and some
	                           * chipsets may only provide 'rssi'
	                           * rather than 'rssi_ant')
	                           */

	/* byte 48 ~ 55 */
	uint64 padding_future;    /* reserved */

	/* byte 56 ~ 63 */
	uint16 data_length;       /* the bytes of 'data[]' */
	uint16 remain_length;     /* remain length of not received data */
	uint16 copied_length;     /* the length has been copied to user */
	uint16 data_checksum;     /* 'data[]' checksum, 0 - not used */

	/* byte 64 ~ End */
	uint32 data[0];           /* variable length according to the
	                           * 'data_length', but the minimal
	                           * valid data length will be
	                           * decided by the 'num_txchains'
	                           * 'num_rxchain' 'num_subcarrier'.
	                           * For example, the index quantity
	                           * of 'uint32' in the 11N 80MHz can
	                           * be got via:
	                           * 256 SubCarriers * 2 TXStream * 2 RXChain
	                           * Note: 'data_length' is bytes quantity,
	                           * and the 'data[]' array index range should
	                           * be 0 ~ (data_length/4 - 1)
	                           */
};

#define SYNA_CSI_PKT_V1_TOTAL_LEN(ptr) \
(sizeof(struct syna_csi_header_v1) \
	+ (((struct syna_csi_header_v1 *)(ptr))->data_length) \
)

/* Version 2 'syna_csi_header' (8 bytes alignment for 64bit system) */
#define CONST_SYNA_CSI_HEADER_VERSION_V2    0x02
struct syna_csi_header_v2 {
	/* byte 0 ~ 3 */
	uint32 magic_flag;        /* flag for protecting and detecting header */
	/* byte 4 ~ 7 */
	uint8  version;           /* header version of this structure */
	uint8  format_type;       /* data format type of current CSI packet,
	                           * enumerated in 'syna_csi_format_type'
	                           */
	uint16 frame_control;     /* packet frame control type of current
	                           * CSI data for further checking if it
	                           * is the expected result
	                           */

	/* byte 8 ~ 11 */
	uint32 global_id;         /* CSI frame global ID */
	/* byte 12 ~ 15 */
	uint16 header_length;     /* total header length of 'syna_csi_header',
	                           * and this offset can be used to find the
	                           * right data[] offset among different
	                           * version header structure.
	                           */
	uint8  trigger_mode;      /* active/passive working mode in the
	                           *'eSyna_csi_trigger_mode'
	                           */
	uint8  dot11_mode;        /* 802.11 RX mode in 'syna_csi_dot11_mode' */

	/* byte 16 ~ 23 */
	uint64 report_tsf;        /* current CSI DATA RX/generated timestamp */

	/* byte 24 ~ 27 */
	uint16 flag_error;        /* error flag may use for extra emendation
	                           * which enumerated as 'syna_csi_error_flag'
	                           */
	uint16 flag_feature;      /* feature flag in 'syna_csi_feature_flag' */
	/* byte 28 ~ 39 */
	uint8  peer_mac[6];       /* peer side transmitor MAC address */
	uint8  local_mac[6];      /* local side receiver MAC address */

	/* byte 40 ~ 43 */
	uint8  qty_txstream;      /* peer side TX spatial steams */
	uint8  qty_rxchain;       /* quantity of RX side chain/antenna */
	uint16 qty_subcarrier;    /* quantity of sub-carriers */

	/* byte 44 ~ 45 */
	uint16 chipset_flag;      /* chipset flag */
	/* byte 46 ~ 51 */
	uint8  band;              /* enumerated in 'syna_csi_band_type' */
	uint8  side_band;         /* side band in 'syna_csi_sideband_type' */
	uint16 bandwidth;         /* PHY RX bandwidth which enumerated in
	                           *'syna_csi_bandwidth_type'
	                           */
	uint16 channel;           /* channel number of corresponding band */
	/* byte 52 ~ 55 */
	uint32 rx_rate;           /* rate information as 'syna_csi_rate_type' */

	/* byte 56 ~ 57 */
	uint8  rx_bandwidth;      /* frame bandwidth which enumerated in
	                           * 'syna_csi_bandwidth_type'
	                           */
	uint8  rxchain_idx_mask;  /* bit index based RX chain/antenna mask,
	                           * 1 - core 0(bit0), 2 - core 1(bit1)
	                           * 3 - core 0(bit0) + core 1(bit1)
	                           */
	/* byte 58 ~ 59 */
	int8   rssi;              /* average RSSI (dBm), and goto check the
	                           * 'rssi_ant' if this 'rssi' is zero
	                           */
	int8   noise;             /* average noise (dBm) */

	/* byte 60 ~ 63 */
	int8   rssi_ant[4];       /* RSSI of each RX chain/antenna
	                           * (Depends on solution, and some
	                           * chipsets may only provide 'rssi'
	                           * rather than 'rssi_ant')
	                           */

	/* byte 64 ~ 75 */
	uint8  padding[12];       /* reserved for reuse */

	/* byte 76 ~ 79 */
	uint16 data_length;       /* the bytes of 'data[]' */
	uint16 data_checksum;     /* 'data[]' checksum, 0 - not used */

	/* byte 80 ~ End */
	uint32 data[0];           /* variable length according to the
	                           * 'data_length', but the minimal
	                           * valid data length will be
	                           * decided by the 'num_txchains'
	                           * 'num_rxchain' 'num_subcarrier'.
	                           * For example, the index quantity
	                           * of 'uint32' in the 11N 80MHz can
	                           * be got via:
	                           * 256 SubCarriers * 2 TXStream * 2 RXChain
	                           * Note: 'data_length' is bytes quantity,
	                           * and the 'data[]' array index range should
	                           * be 0 ~ (data_length/4 - 1)
	                           * It's better to use 'header_length' to
	                           * count the 'data[]' offset to exclude
	                           * different version header structure's
	                           * difference.
	                           */
};

#define SYNA_CSI_PKT_V2_TOTAL_LEN(ptr) \
(sizeof(struct syna_csi_header_v2) \
	+ (((struct syna_csi_header_v2 *)(ptr))->data_length) \
)

/* common header for transfering */
typedef struct syna_csi_header_v2                  syna_csi_common_header;
#define CONST_SYNA_CSI_COMMON_HEADER_VERSION       CONST_SYNA_CSI_HEADER_VERSION_V2
#define SYNA_CSI_COMMON_PKT_TOTAL_LEN(ptr)         SYNA_CSI_PKT_V2_TOTAL_LEN(ptr)

/* Output V1 CSI Header to upper layer by default for compatibility */
#define CONST_CSI_HEADER_VERSION_OUTPUT_DEFAULT    CONST_SYNA_CSI_HEADER_VERSION_V1

/* Maxinum csi file dump size */
#define CONST_CSI_SYS_FILE_SIZE_MAX		(1024 * 32)
/* maximun csi packet number stored here: 30Hz * 120 seconds * STA */
#define CONST_CSI_MAX_STA_QTY			16
#define CONST_CSI_QUEUE_MAX_SIZE		((1024 * 4) * CONST_CSI_MAX_STA_QTY)

/*** definition for structure and functions between driver and FW */
/* revision 0 'dhd_cfr_header' (can only access by byte due to not aligned!) */
#define CONST_CFR_HEADER_REVISION_0	0 /* cfr header version V0 */
struct dhd_cfr_header_rev_0 {
	uint8 status:4;     /* bit3-0 */
	uint8 version:4;    /* bit7-4 */
	/* Peer MAC address */
	uint8 peer_macaddr[6];
	/* Number of Space Time Streams */
	uint8 sts;
	/* Number of RX chain */
	uint8 num_rx;
	/* Number of subcarrier */
	uint16 num_carrier;
	/* Length of the CSI dump */
	uint32 cfr_dump_length;
	/* remain unsend CSI data length */
	uint32 remain_length;
	/* RSSI */
	int8 rssi;
	uint32 data[0];
} __attribute__ ((packed));

/* revision 1 'dhd_cfr_header' (can only access by byte due to not aligned!) */
#define CONST_CFR_HEADER_REVISION_1	1 /* cfr header version V1 */
struct dhd_cfr_header_rev_1 {
	uint8 status:4;     /* bit3-0 */
	uint8 version:4;    /* bit7-4 */
	/* Peer MAC address */
	uint8 peer_macaddr[6];
	/* Number of Space Time Streams */
	uint8 sts;
	/* Number of RX chain */
	uint8 num_rx;
	/* Number of subcarrier */
	uint16 num_carrier;
	/* Length of the CSI dump */
	uint32 cfr_dump_length;
	/* remain unsend CSI data length */
	uint32 remain_length;
	/* RSSI */
	int8 rssi;

	/* Chip id. 1 for BCM43456/8 */
	uint8 chip_id;
	/* Frame control field */
	uint8 fc;
	/* Time stamp when CFR capture is taken,
	 * in microseconds since the epoch
	 */
	uint64 cfr_timestamp;

	uint32 data[0];
} __attribute__ ((packed));

/* revision 2 'dhd_cfr_header' (8 bytes alignment for 64bit system) */
#define CONST_CFR_HEADER_REVISION_2	2 /* cfr header version V2 */
struct dhd_cfr_header_rev_2 {
	/* byte 0 ~ 7 */
	uint8  status_compat:4;   /* bit3-0, current CSI data status: 0->good */
	uint8  version_compat:4;  /* bit7-4, duplicate but compatible part */
	uint8  padding_magic;
	uint16 magic_flag;        /* flag for protecting and detecting header */

	uint8  version;           /* header version of this structure */
	uint8  format_type;       /* data format type of current CSI packet,
	                           * enumerated in 'syna_csi_format_type'
	                           */
	uint8  fc;                /* packet frame control type of current
	                           * CSI data for further checking if it
	                           * is correct expected result
	                           */
	uint8  flags;             /* flag may use for extra emendation
	                           * which enumerated as 'syna_csi_flag'
	                           */

	/* byte 8 ~ 19 */
	uint8  client_ea[6];      /* client MAC address */
	uint8  bsscfg_ea[6];      /* BSSCFG address of current interface */

	/* byte 20 ~ 23 */
	uint8  band;              /* enumerated in 'syna_csi_band_type' */
	uint8  bandwidth;         /* enumerated in 'syna_csi_bandwidth_type' */
	uint16 channel;           /* channel number of corresponding band */

	/* byte 24 ~ 31 */
	uint64 report_tsf;        /* current CSI DATA RX timestamp */

	/* byte 32 ~ 35 */
	uint8  num_txstream;      /* peer side TX spatial steams */
	uint8  num_rxchain;       /* number of RX side chain/antenna */
	uint16 num_subcarrier;    /* Number of subcarrier */

	/* byte 36 ~ 39 */
	int8   rssi;              /* average RSSI, and goto check the
	                           * 'rssi_ant' if this 'rssi' is zero
	                           */
	int8   noise;             /* average noise */
	int16  global_id;         /* CSI frame global ID */

	/* byte 40 ~ 47 */
	int8   rssi_ant[8];       /* RSSI of each RX chain/antenna
	                           * (Depends on solution, and some
	                           * chipsets may only provide 'rssi'
	                           * rather than 'rssi_ant')
	                           */

	/* byte 48 ~ 55 */
	uint64 padding_future;    /* reserved */

	/* byte 56 ~ 63 */
	uint16 data_length;       /* the bytes of 'data[]' */
	uint16 remain_length;     /* the remain unsent length of 'data[]' */
	uint16 padding_length;    /* reserved */
	uint16 data_checksum;     /* 'data[]' checksum, 0 - not used */

	/* byte 64 ~ End */
	uint32 data[0];           /* variable according to 'data_length', but
	                           * the minial valid data length will be
	                           * decided by 'num_txchains', 'num_rxchain'
	                           * 'num_subcarrier'. For example, the
	                           * quantity of uint32(4 bytes) in the
	                           * 11N 80MHz can be got via:
	                           * 256 SubCarriers * 2 TXStream * 2 RXChain
	                           */
};

/* revision 3 'dhd_cfr_header' (8 bytes alignment for 64bit system) */
#define CONST_CFR_HEADER_REVISION_3	3 /* cfr header version V3 */
struct dhd_cfr_header_rev_3 {
	/* byte 0 ~ 3 */
	uint8  status_compat:4;   /* bit3-0, current CSI data status: 0->good */
	uint8  version_compat:4;  /* bit7-4, duplicate but compatible part */
	uint8  padding_magic;     /* reserved for compatibility */
	uint16 magic_flag;        /* flag for protecting and detecting header */
	/* byte 4 ~ 7 */
	uint8  version;           /* header version of this structure */
	uint8  format_type;       /* data format type of current CSI packet,
	                           * enumerated in 'syna_csi_format_type'
	                           */
	uint16 frame_control;     /* packet frame control type of current
	                           * CSI data for further checking if it
	                           * is the expected result
	                           */

	/* byte 8 ~ 11 */
	uint32 global_id;         /* CSI frame global ID */
	/* byte 12 ~ 15 */
	uint16 header_length;     /* total header length of 'syna_csi_header',
	                           * and this offset can be used to find the
	                           * right data[] offset among different
	                           * version header structure.
	                           */
	uint8  trigger_mode;      /* active/passive working mode in the
	                           *'eSyna_csi_trigger_mode'
	                           */
	uint8  dot11_mode;        /* 802.11 RX mode in 'syna_csi_dot11_mode' */

	/* byte 16 ~ 23 */
	uint64 report_tsf;        /* current CSI DATA RX/generated timestamp */

	/* byte 24 ~ 27 */
	uint16 flag_error;        /* error flag may use for extra emendation
	                           * which enumerated as 'syna_csi_error_flag'
	                           */
	uint16 flag_feature;      /* feature flag in 'syna_csi_feature_flag' */
	/* byte 28 ~ 39 */
	uint8  peer_mac[6];       /* peer side transmitor MAC address */
	uint8  local_mac[6];      /* local side receiver MAC address */

	/* byte 40 ~ 43 */
	uint8  qty_txstream;      /* peer side TX spatial steams */
	uint8  qty_rxchain;       /* quantity of RX side chain/antenna */
	uint16 qty_subcarrier;    /* quantity of sub-carriers */

	/* byte 44 ~ 45 */
	uint16 chipset_flag;      /* chipset flag */
	/* byte 46 ~ 51 */
	uint8  band;              /* enumerated in 'syna_csi_band_type' */
	uint8  side_band;         /* side band in 'syna_csi_sideband_type' */
	uint16 bandwidth;         /* PHY RX bandwidth which enumerated in
	                           *'syna_csi_bandwidth_type'
	                           */
	uint16 channel;           /* channel number of corresponding band */
	/* byte 52 ~ 55 */
	uint32 rx_rate;           /* rate information as 'syna_csi_rate_type' */

	/* byte 56 ~ 57 */
	uint8  rx_bandwidth;      /* frame bandwidth which enumerated in
	                           * 'syna_csi_bandwidth_type'
	                           */
	uint8  rxchain_idx_mask;  /* bit index based RX chain/antenna mask,
	                           * 1 - core 0(bit0), 2 - core 1(bit1)
	                           * 3 - core 0(bit0) + core 1(bit1)
	                           */
	/* byte 58 ~ 59 */
	int8   rssi;              /* average RSSI (dBm), and goto check the
	                           * 'rssi_ant' if this 'rssi' is zero
	                           */
	int8   noise;             /* average noise (dBm) */

	/* byte 60 ~ 63 */
	int8   rssi_ant[4];       /* RSSI of each RX chain/antenna
	                           * (Depends on solution, and some
	                           * chipsets may only provide 'rssi'
	                           * rather than 'rssi_ant')
	                           */

	/* byte 64 ~ 71 */
	uint64 padding_future;    /* reserved for reuse */

	/* byte 72 ~ 79 */
	uint16 padding_data;      /* reserved for compatibility */
	uint16 remain_length;     /* remain length of not received data */
	uint16 data_length;       /* the bytes of 'data[]' */
	uint16 data_checksum;     /* 'data[]' checksum, 0 - not used */

	/* byte 80 ~ End */
	uint32 data[0];           /* variable length according to the
	                           * 'data_length', but the minimal
	                           * valid data length will be
	                           * decided by the 'num_txchains'
	                           * 'num_rxchain' 'num_subcarrier'.
	                           * For example, the index quantity
	                           * of 'uint32' in the 11N 80MHz can
	                           * be got via:
	                           * 256 SubCarriers * 2 TXStream * 2 RXChain
	                           * Note: 'data_length' is bytes quantity,
	                           * and the 'data[]' array index range should
	                           * be 0 ~ (data_length/4 - 1)
	                           * It's better to use 'header_length' to
	                           * count the 'data[]' offset to exclude
	                           * different version header structure's
	                           * difference.
	                           */
};

/* union structure for decoding the version */
union dhd_cfr_header {
	struct dhd_cfr_header_rev_0  header_rev_0;
	struct dhd_cfr_header_rev_1  header_rev_1;
	struct dhd_cfr_header_rev_2  header_rev_2;
	struct dhd_cfr_header_rev_3  header_rev_3;
};
#define CSI_CFG_PKT_LENGTH(pHeader) \
(\
	(CONST_CFR_HEADER_REVISION_0 == pHeader->header_rev_0.version) \
	?(sizeof(struct dhd_cfr_header_rev_0) + ((pHeader)->header_rev_0.cfr_dump_length)) \
	:(CONST_CFR_HEADER_REVISION_1 == pHeader->header_rev_0.version) \
	?(sizeof(struct dhd_cfr_header_rev_1) + ((pHeader)->header_rev_1.cfr_dump_length)) \
	:(CONST_CFR_HEADER_REVISION_2 == pHeader->header_rev_0.version) \
	?(sizeof(struct dhd_cfr_header_rev_2) + ((pHeader)->header_rev_2.data_length)) \
	:(CONST_CFR_HEADER_REVISION_3 == pHeader->header_rev_0.version) \
	?(sizeof(struct dhd_cfr_header_rev_3) + ((pHeader)->header_rev_3.data_length)) \
	:(0) \
)
#define CSI_CFR_MAXIMUM_LENGTH(data_len) \
	(sizeof(union dhd_cfr_header) + data_len)

/* BW80 2x2 => 4bytes * 256subcarries * 2txstream * 2rxchain */
#define CONST_CSI_DATA_BYTES_MAX          (CSI_CFR_MAXIMUM_LENGTH(256 *2 *2 *4))

#define CONST_SYNA_SOCKET_HEADER_BYTES    (14 + 20 + 8)

struct csi_cfr_node {
	struct list_head  list;
	void   *pNode; /* recording current node pointer
	                * especially for UDP data manner
	                */

	uint32  total_size;
	uint16  ifidx;
	uint16  send_manner;

	/* for packet reference */
	uint16  length_data_total;
	uint16  length_data_received;
	uint16  length_data_remain;
	uint16  length_data_copied;
	uint32  global_id;
	 int16  convert_pull;
	uint8   peer_mac[ETHER_ADDR_LEN];
	uint8   padding[4];

	/* layout for skb:
	 *   alignment_padding(6)
	 *   header_ethernet(14)
	 *   header_ip(20)
	 *   header_udp(8)
	 *     csi_header(80)
	 *     csi_data(N)
	 */
	uint8  network_padding[CONST_SYNA_SOCKET_HEADER_BYTES];
	/* should make this be 8 bytes alignment */
	syna_csi_common_header  *pHeader_csi;
};

#define CONST_SYNA_CSI_CFR_NODE_LEN          sizeof(struct csi_cfr_node)  /* 112 */

#define CONST_SYNA_CSI_CFR_NODE_LEN_WITH_COMMON_HEADER(ptr, data_length) \
(\
	CONST_SYNA_CSI_CFR_NODE_LEN + data_length \
)

#define SYNA_CSI_CFR_NODE_FREE_LEN(ptr) \
(\
	((struct csi_cfr_node *)(ptr))->total_size \
	- CONST_SYNA_CSI_CFR_NODE_LEN \
)

extern int dhd_csi_version(dhd_pub_t *dhdp, char *pBuf, uint length, bool is_set);
extern int dhd_csi_config(dhd_pub_t *dhdp, char *buf, uint length, bool is_set);

/* Function: change the upper layer fetching data manner
 * Input:  uint  is_set        indicate it's set or get action
 *         uint  type          the expected fetch type(ignore when get)
 *         uint  param         extra param when set(UDP socket port)
 * Output: <  0        error encounter
 *         == 0        successful
 *         >  0        corresponding 'fetch type' when get
 */
extern int dhd_csi_data_fetch_type_access(uint is_set, uint type, uint param);

/* Function: parse the CSI event and convert to 'dhd_csi_header' format,
 *           and queue the item into the common 'csi_cfr_queue'
 * Input:  dhd_pub_t    *dhdp     context for operation
 *         char         *buf      buffer for storing csi data
 *         uint          count    total bytes available in 'buf'
 * Output: <  0        error encounter
 *         == 0        successful but no csi data in queue
 *         >  0        bytes write into 'buf'
 */
extern int dhd_csi_data_queue_polling(dhd_pub_t *dhdp, char *buf, uint count);

/* Function: push the skb packet into the CSI raw skb queue for later
 *           processing since DPC context maybe atomic and can't do
 *           'schedule'/interruptible work like IOVAR or printf
 * Input:  dhd_pub_t  *dhdp       context for operation
 *         int        *ifidx      interface index
 *         void       *pktbuf     raw skb pointer
 * Output: <  0        error encounter
 *         == 0        successful
 * Note:   the skb will be internally processed and freed if enqueue successfully
 */
extern int dhd_csi_event_enqueue(dhd_pub_t  *dhdp, int ifidx, void *pktbuf);

/* Function: Initialize the CSI module
 * Input:  dhd_pub_t      *dhdp          context for operation
 * Output: <  0        error encounter
 *         == 0        successful
 */
extern int dhd_csi_init(dhd_pub_t *dhdp);

/* Function: Deinitialize the CSI module
 * Input:  dhd_pub_t      *dhdp          context for operation
 * Output: <  0        error encounter
 *         == 0        successful
 */
extern int dhd_csi_deinit(dhd_pub_t *dhdp);

#endif /* CSI_SUPPORT */

#endif /* __DHD_CSI_H__ */
