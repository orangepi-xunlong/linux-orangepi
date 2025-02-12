/*
 * The confidential and proprietary information contained in this file may
 * only be used by a person authorised under and to the extent permitted
 * by a subsisting licensing agreement from Arm Technology (China) Co., Ltd.
 *
 *            (C) COPYRIGHT 2021-2021 Arm Technology (China) Co., Ltd.
 *                ALL RIGHTS RESERVED
 *
 * This entire notice must be reproduced on all copies of this file
 * and copies of this file may only be made by a person if such person is
 * permitted to do so under the terms of a subsisting license agreement
 * from Arm Technology (China) Co., Ltd.
 * 
 * SPDX-License-Identifier: GPL-2.0-only
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 * 
 */

#ifndef _MVX_FIRMWARE_H_
#define _MVX_FIRMWARE_H_

/****************************************************************************
 * Includes
 ****************************************************************************/

#include <linux/hashtable.h>
#include <linux/kref.h>
#include <linux/list.h>
#include <linux/mutex.h>
#include <linux/seq_file.h>
#include "mvx_if.h"
#include "mvx_buffer.h"

/****************************************************************************
 * Defines
 ****************************************************************************/

#define MVX_FW_HTABLE_BITS      3
#define MVX_FW_QUANT_LEN        64

/****************************************************************************
 * Firmware communication types
 ****************************************************************************/

/**
 * enum mvx_fw_buffer_attr
 */
enum mvx_fw_buffer_attr {
	MVX_FW_BUF_CACHEABLE,
	MVX_FW_BUF_UNCACHEABLE,
	MVX_FW_BUF_ATTR_NUM,
};

/**
 * enum mvx_fw_state - Firmware state.
 */
enum mvx_fw_state {
	MVX_FW_STATE_STOPPED,
	MVX_FW_STATE_RUNNING
};

/**
 * struct mvx_fw_job - Job request.
 * @cores:	Number for cores to use.
 * @frames:	Number of frames to process before job is switched out.
 */
struct mvx_fw_job {
	unsigned int cores;
	unsigned int frames;
};

/**
 * struct mvx_fw_qp_range - QP range.
 */
struct mvx_fw_qp_range {
	int min;
	int max;
};

/**
 * struct mvx_fw_profile_level - Profile and level.
 */
struct mvx_fw_profile_level {
	unsigned int profile;
	unsigned int level;
};

/**
 * struct mvx_fw_tile - Tile size.
 */
struct mvx_fw_tile {
	unsigned int rows;
	unsigned int cols;
};

/**
 * struct mvx_fw_mv - Motion vector search range.
 */
struct mvx_fw_mv {
	unsigned int x;
	unsigned int y;
};

/**
 * struct mvx_fw_bitdepth - Bit depth.
 */
struct mvx_fw_bitdepth {
	unsigned int chroma;
	unsigned int luma;
};

struct mvx_buffer_param_region
{
    uint16_t mbx_left;   /**< X coordinate of the left most macroblock */
    uint16_t mbx_right;  /**< X coordinate of the right most macroblock */
    uint16_t mby_top;    /**< Y coordinate of the top most macroblock */
    uint16_t mby_bottom; /**< Y coordinate of the bottom most macroblock */
    int16_t qp_delta;   /**< QP delta value. This region will be encoded
                         *   with qp = qp_default + qp_delta. */
};

struct mvx_roi_config
{
    unsigned int pic_index;
    unsigned char qp_present;
    unsigned char qp;
    unsigned char roi_present;
    unsigned char num_roi;
    #define MVX_MAX_FRAME_REGIONS 16
    struct mvx_buffer_param_region roi[MVX_MAX_FRAME_REGIONS];
};

struct mvx_buffer_param_rate_control
{
    uint32_t rate_control_mode;
        #define MVX_OPT_RATE_CONTROL_MODE_OFF               (0)
        #define MVX_OPT_RATE_CONTROL_MODE_STANDARD          (1)
        #define MVX_OPT_RATE_CONTROL_MODE_VARIABLE          (2)
        #define MVX_OPT_RATE_CONTROL_MODE_CONSTANT          (3)
        #define MVX_OPT_RATE_CONTROL_MODE_C_VARIABLE        (4)
    uint32_t target_bitrate; /* in bits per second */
    uint32_t maximum_bitrate; /* in bits per second */
};

struct mvx_dsl_frame{
    uint32_t width;
    uint32_t height;
};

struct mvx_dsl_ratio{
    uint32_t hor;
    uint32_t ver;
};

struct mvx_long_term_ref{
    uint32_t mode;
    uint32_t period;
};
/**
 * struct mvx_fw_error - Firmware error message.
 * @error_code:	What kind of error that was reported.
 * @message:	Error message string.
 */
struct mvx_fw_error {
	enum {
		MVX_FW_ERROR_ABORT,
		MVX_FW_ERROR_OUT_OF_MEMORY,
		MVX_FW_ERROR_ASSERT,
		MVX_FW_ERROR_UNSUPPORTED,
		MVX_FW_ERROR_INVALID_BUFFER,
		MVX_FW_ERROR_INVALID_STATE,
		MVX_FW_ERROR_WATCHDOG
	} error_code;
	char message[128];
};

/**
 * struct mvx_fw_flush - Flush firmware buffers.
 * @dir:	Which port to flush.
 */
struct mvx_fw_flush {
	enum mvx_direction dir;
};

/**
 * struct mvx_fw_alloc_param - Allocation parameters.
 * @width:		Width in pixels.
 * @height:		Height in pixels.
 * @afbc_alloc_bytes:	AFBC buffer size.
 * @afbc_width:		AFBC width in superblocks.
 *
 * Dimensions of a decoded frame buffer.
 */
struct mvx_fw_alloc_param {
	unsigned int width;
	unsigned int height;
	unsigned int afbc_alloc_bytes;
	unsigned int afbc_width;
};

/**
 * struct mvx_fw_seq_param - Sequence parameters.
 * @planar.buffers_min:	Minimum number of planar buffers required.
 * @afbc.buffers_min:	Minimum number of AFBC buffers required.
 */
struct mvx_fw_seq_param {
	struct {
		unsigned int buffers_min;
	} planar;
	struct {
		unsigned int buffers_min;
	} afbc;
};

enum mvx_fw_range {
	MVX_FW_RANGE_UNSPECIFIED,
	MVX_FW_RANGE_FULL,
	MVX_FW_RANGE_LIMITED
};

enum mvx_fw_primaries {
	MVX_FW_PRIMARIES_UNSPECIFIED,
	MVX_FW_PRIMARIES_BT709,         /* Rec.ITU-R BT.709 */
	MVX_FW_PRIMARIES_BT470M,        /* Rec.ITU-R BT.470 System M */
	MVX_FW_PRIMARIES_BT601_625,     /* Rec.ITU-R BT.601 625 */
	MVX_FW_PRIMARIES_BT601_525,     /* Rec.ITU-R BT.601 525 */
	MVX_FW_PRIMARIES_GENERIC_FILM,  /* Generic Film */
	MVX_FW_PRIMARIES_BT2020         /* Rec.ITU-R BT.2020 */
};

enum mvx_fw_transfer {
	MVX_FW_TRANSFER_UNSPECIFIED,
	MVX_FW_TRANSFER_LINEAR,         /* Linear transfer characteristics */
	MVX_FW_TRANSFER_SRGB,           /* sRGB or equivalent */
	MVX_FW_TRANSFER_SMPTE170M,      /* SMPTE 170M  */
	MVX_FW_TRANSFER_GAMMA22,        /* Assumed display gamma 2.2 */
	MVX_FW_TRANSFER_GAMMA28,        /* Assumed display gamma 2.8 */
	MVX_FW_TRANSFER_ST2084,         /* SMPTE ST 2084 */
	MVX_FW_TRANSFER_HLG,            /* ARIB STD-B67 hybrid-log-gamma */
	MVX_FW_TRANSFER_SMPTE240M,      /* SMPTE 240M */
	MVX_FW_TRANSFER_XVYCC,          /* IEC 61966-2-4 */
	MVX_FW_TRANSFER_BT1361,         /* Rec.ITU-R BT.1361 extended gamut */
	MVX_FW_TRANSFER_ST428           /* SMPTE ST 428-1 */
};

enum mvx_fw_matrix {
	MVX_FW_MATRIX_UNSPECIFIED,
	MVX_FW_MATRIX_BT709,            /* Rec.ITU-R BT.709 */
	MVX_FW_MATRIX_BT470M,           /* KR=0.30, KB=0.11 */
	MVX_FW_MATRIX_BT601,            /* Rec.ITU-R BT.601 625 */
	MVX_FW_MATRIX_SMPTE240M,        /* SMPTE 240M or equivalent */
	MVX_FW_MATRIX_BT2020,           /* Rec.ITU-R BT.2020 non-const lum */
	MVX_FW_MATRIX_BT2020Constant    /* Rec.ITU-R BT.2020 const lum */
};

struct mvx_fw_primary {
	unsigned int x;
	unsigned int y;
};

/**
 * struct mvx_fw_color_desc - HDR color description.
 */
struct mvx_fw_color_desc {
	unsigned int flags;
	enum mvx_fw_range range;
	enum mvx_fw_primaries primaries;
	enum mvx_fw_transfer transfer;
	enum mvx_fw_matrix matrix;
	struct {
		struct mvx_fw_primary r;
		struct mvx_fw_primary g;
		struct mvx_fw_primary b;
		struct mvx_fw_primary w;
		unsigned int luminance_min;
		unsigned int luminance_max;
	} display;
	struct {
		unsigned int luminance_max;
		unsigned int luminance_average;
	} content;

    uint8_t video_format;
    uint8_t aspect_ratio_idc;
    uint16_t sar_width;
    uint16_t sar_height;
    uint32_t num_units_in_tick;
    uint32_t time_scale;
};

struct mvx_sei_userdata{
    uint8_t flags;
        #define MVX_BUFFER_PARAM_USER_DATA_UNREGISTERED_VALID  (1)
    uint8_t uuid[16];
    char user_data[256 - 35];
    uint8_t user_data_len;
};
/**
 * struct mvx_fw_set_option - Set firmware options.
 */
struct mvx_fw_set_option {
    enum {
        /**
         * Frame rate.
         * Extra data: frame_date.
         */
        MVX_FW_SET_FRAME_RATE,

        /**
         * Bitrate.
         * Extra data: target_bitrate.
         *
         * When target_bitrate is other than zero, rate control
         * in HW is enabled, otherwise rate control is disabled.
         */
        MVX_FW_SET_TARGET_BITRATE,

        /**
         * QP range.
         * Extra data: qp_range.
         *
         * QP range when rate controller is enabled.
         */
        MVX_FW_SET_QP_RANGE,

        /**
         * NALU format.
         * Extra data: nalu_format.
         */
        MVX_FW_SET_NALU_FORMAT,

        /**
         * Defines if stream escaping is enabled.
         * Extra data: stream_escaping.
         */
        MVX_FW_SET_STREAM_ESCAPING,

        /**
         * Defines profile and level for encoder.
         * Extra data: profile_level.
         */
        MVX_FW_SET_PROFILE_LEVEL,

        /**
         * Ignore stream headers.
         * Extra data: ignore_stream_headers.
         */
        MVX_FW_SET_IGNORE_STREAM_HEADERS,

        /**
         * Enable frame reordering for decoder.
         * Extra data: frame_reordering.
         */
        MVX_FW_SET_FRAME_REORDERING,

        /**
         * Suggested internal buffer size.
         * Extra data: intbuf_size.
         */
        MVX_FW_SET_INTBUF_SIZE,

        /**
         * Number of P frames for encoder.
         * Extra data: pb_frames.
         */
        MVX_FW_SET_P_FRAMES,

        /**
         * Number of B frames for encoder.
         * Extra data: pb_frames.
         */
        MVX_FW_SET_B_FRAMES,

        /**
         * GOP type for encoder.
         * Extra data: gop_type.
         */
        MVX_FW_SET_GOP_TYPE,

        /**
         * Intra MB refresh.
         * Extra data: intra_mb_refresh.
         */
        MVX_FW_SET_INTRA_MB_REFRESH,

        /**
         * Constrained intra prediction.
         * Extra data: constr_ipred.
         */
        MVX_FW_SET_CONSTR_IPRED,

        /**
         * Enable entropy synchronization.
         * Extra data: entropy_sync.
         */
        MVX_FW_SET_ENTROPY_SYNC,

        /**
         * Enable temporal motion vector prediction.
         * Extra data: temporal_mvp.
         */
        MVX_FW_SET_TEMPORAL_MVP,

        /**
         * Tiles size.
         * Extra data: tile.
         */
        MVX_FW_SET_TILES,

        /**
         * Minimum luma coding block size.
         * Extra data: min_luma_cb_size.
         */
        MVX_FW_SET_MIN_LUMA_CB_SIZE,

        /**
         * Entropy mode.
         * Extra data: entropy_mode.
         */
        MVX_FW_SET_ENTROPY_MODE,

        /**
         * Suggested number of CTUs in a slice.
         * Extra data: slice_spacing_mb.
         */
        MVX_FW_SET_SLICE_SPACING_MB,

        /**
         * Probability update method.
         * Extra data: vp9_prob_update.
         */
        MVX_FW_SET_VP9_PROB_UPDATE,

        /**
         * Search range for motion vectors.
         * Extra data: mv.
         */
        MVX_FW_SET_MV_SEARCH_RANGE,

        /**
         * Bitdepth.
         * Extra data: bitdepth.
         */
        MVX_FW_SET_BITDEPTH,

        /**
         * Chroma format.
         * Extra data: chroma_format.
         */
        MVX_FW_SET_CHROMA_FORMAT,

        /**
         * RGB to YUV conversion mode.
         * Extra data: rgb_to_yuv_mode.
         */
        MVX_FW_SET_RGB_TO_YUV_MODE,

        /**
         * Maximum bandwidth limit.
         * Extra data: band_limit.
         */
        MVX_FW_SET_BAND_LIMIT,

        /**
         * CABAC initialization table.
         * Extra data: cabac_init_idc.
         */
        MVX_FW_SET_CABAC_INIT_IDC,

        /**
         * QP for I frames when rate control is disabled.
         * Extra data: qp
         */
        MVX_FW_SET_QP_I,

        /**
         * QP for P frames when rate control is disabled.
         * Extra data: qp
         */
        MVX_FW_SET_QP_P,

        /**
         * QP for B frames when rate control is disabled.
         * Extra data: qp
         */
        MVX_FW_SET_QP_B,

        /**
         * JPEG resync interval.
         * Extra data: resync_interval
         */
        MVX_FW_SET_RESYNC_INTERVAL,

        /**
         * JPEG quantization table.
         * Extra data: quant_tbl.
         */
        MVX_FW_SET_QUANT_TABLE,

        /**
         * Set watchdog timeout. 0 to disable.
         */
        MVX_FW_SET_WATCHDOG_TIMEOUT,

        /**
         * QP for encode frame.
         * Extra data: qp
         */
        MVX_FW_SET_QP_REGION,

        /**
         * ROI for encode frame.
         * Extra data: ROI
         */
        MVX_FW_SET_ROI_REGIONS,

        /**
         * Rate Control for encode frame.
         * Extra data: rate control
         */
        MVX_FW_SET_RATE_CONTROL,
        /**
         * Crop left for encode frame.
         * Extra data: crop left
         */
        MVX_FW_SET_CROP_LEFT,
        /**
         * Crop right for encode frame.
         * Extra data: crop right
         */
        MVX_FW_SET_CROP_RIGHT,
        /**
         * Crop top for encode frame.
         * Extra data: crop top
         */
        MVX_FW_SET_CROP_TOP,
        /**
         * Crop bottom for encode frame.
         * Extra data: crop bottom
         */
        MVX_FW_SET_CROP_BOTTOM,

        MVX_FW_SET_COLOUR_DESC,

        MVX_FW_SET_SEI_USERDATA,

        MVX_FW_SET_HRD_BUF_SIZE,

        MVX_FW_SET_DSL_FRAME,

        MVX_FW_SET_LONG_TERM_REF,

        MVX_FW_SET_DSL_MODE,

        MVX_FW_SET_GOP_RESET,

        MVX_FW_SET_INDEX_PROFILING
    } code;

    /**
     * Extra data for an option.
     */
    union {
        unsigned int frame_rate;
        unsigned int target_bitrate;
        struct mvx_fw_qp_range qp_range;
        enum mvx_nalu_format nalu_format;
        bool stream_escaping;
        struct mvx_fw_profile_level profile_level;
        bool ignore_stream_headers;
        bool frame_reordering;
        unsigned int intbuf_size;
        unsigned int pb_frames;
        enum mvx_gop_type gop_type;
        unsigned int intra_mb_refresh;
        bool constr_ipred;
        bool entropy_sync;
        bool temporal_mvp;
        struct mvx_fw_tile tile;
        unsigned int min_luma_cb_size;
        enum mvx_entropy_mode entropy_mode;
        unsigned int slice_spacing_mb;
        enum mvx_vp9_prob_update vp9_prob_update;
        struct mvx_fw_mv mv;
        struct mvx_fw_bitdepth bitdepth;
        unsigned int chroma_format;
        enum mvx_rgb_to_yuv_mode rgb_to_yuv_mode;
        unsigned int band_limit;
        unsigned int cabac_init_idc;
        int qp;
        int resync_interval;
        struct {
            uint8_t *chroma;
            uint8_t *luma;
        } quant_tbl;
        int watchdog_timeout;
        struct mvx_roi_config roi_config;
        struct mvx_buffer_param_rate_control rate_control;
        unsigned int crop_left;
        unsigned int crop_right;
        unsigned int crop_top;
        unsigned int crop_bottom;
        struct mvx_fw_color_desc colour_desc;
        struct mvx_sei_userdata userdata;
        unsigned int nHRDBufsize;
        struct mvx_dsl_frame dsl_frame;
        struct mvx_long_term_ref ltr;
        int dsl_pos_mode;
        int index_profiling;
    };
};
#define MVX_FW_COLOR_DESC_DISPLAY_VALID         0x1
#define MVX_FW_COLOR_DESC_CONTENT_VALID         0x2

/**
 * enum mvx_fw_code - Codes for messages sent between driver and firmware.
 */
enum mvx_fw_code {
	MVX_FW_CODE_ALLOC_PARAM,        /* Driver <- Firmware. */
	MVX_FW_CODE_BUFFER,             /* Driver <-> Firmware. */
	MVX_FW_CODE_ERROR,              /* Driver <- Firmware. */
	MVX_FW_CODE_IDLE,               /* Driver <- Firmware. */
	MVX_FW_CODE_FLUSH,              /* Driver <-> Firmware. */
	MVX_FW_CODE_JOB,                /* Driver -> Firmware. */
	MVX_FW_CODE_PING,               /* Driver -> Firmware. */
	MVX_FW_CODE_PONG,               /* Driver <- Firmware. */
	MVX_FW_CODE_SEQ_PARAM,          /* Driver <- Firmware. */
	MVX_FW_CODE_SET_OPTION,         /* Driver <-> Firmware. */
	MVX_FW_CODE_STATE_CHANGE,       /* Driver <-> Firmware. */
	MVX_FW_CODE_SWITCH_IN,          /* Driver <- Firmware. */
	MVX_FW_CODE_SWITCH_OUT,         /* Driver <-> Firmware. */
	MVX_FW_CODE_IDLE_ACK,           /* Driver -> Firmware. */
	MVX_FW_CODE_EOS,                /* Driver <-> Firmware. */
	MVX_FW_CODE_COLOR_DESC,         /* Driver <- Firmware. */
	MVX_FW_CODE_DUMP,               /* Driver <-> Firmware. */
	MVX_FW_CODE_DEBUG,              /* Driver <-> Firmware. */
	MVX_FW_CODE_BUFFER_GENERAL,     /* Driver <-> Firmware. */
	MVX_FW_CODE_UNKNOWN,            /* Driver <- Firmware. */
	MVX_FW_CODE_DPB_HELD_FRAMES,    /* Driver <- Firmware. */
	MVX_FW_CODE_MAX
};

/**
 * struct mvx_fw_msg - Union of all message types.
 */
struct mvx_fw_msg {
	enum mvx_fw_code code;
	union {
		enum mvx_fw_state state;
		struct mvx_fw_job job;
		struct mvx_fw_error error;
		struct mvx_fw_set_option set_option;
		struct mvx_fw_flush flush;
		struct mvx_fw_alloc_param alloc_param;
		struct mvx_fw_seq_param seq_param;
		struct mvx_fw_color_desc color_desc;
		struct mvx_buffer *buf;
		uint32_t arg;
		bool eos_is_frame;
	};
};

/****************************************************************************
 * Types
 ****************************************************************************/

struct device;
struct mvx_fw_bin;
struct mvx_mmu;
struct mvx_mmu_pages;
struct mvx_session;

/**
 * enum mvx_fw_region - Firmware memory regions.
 */
enum mvx_fw_region {
	MVX_FW_REGION_CORE_0,
	MVX_FW_REGION_CORE_1,
	MVX_FW_REGION_CORE_2,
	MVX_FW_REGION_CORE_3,
	MVX_FW_REGION_CORE_4,
	MVX_FW_REGION_CORE_5,
	MVX_FW_REGION_CORE_6,
	MVX_FW_REGION_CORE_7,
	MVX_FW_REGION_PROTECTED,
	MVX_FW_REGION_FRAMEBUF,
	MVX_FW_REGION_MSG_HOST,
	MVX_FW_REGION_MSG_MVE,
	MVX_FW_REGION_BUF_IN_HOST,
	MVX_FW_REGION_BUF_IN_MVE,
	MVX_FW_REGION_BUF_OUT_HOST,
	MVX_FW_REGION_BUF_OUT_MVE,
	MVX_FW_REGION_RPC,
	MVX_FW_REGION_PRINT_RAM
};

/**
 * struct mvx_fw - Firmware class.
 * @dev:		Pointer to device.
 * @fw_bin:		Pointer to firmware binary.
 * @mmu:		Pointer to MMU object.
 * @session:		Pointer to session.
 * @client_ops:		Client operations.
 * @csession:		Client session this firmware instance is connected to.
 * @text:		Pages allocated for the text segment.
 * @bss:		Pages allocated for the bss segment.
 * @bss_shared:		Pages allocated for the shared bss segment.
 * @dentry:		Debugfs entry for the "fw" directory.
 * @msg_host:		Host message queue.
 * @msg_mve:		MVE message queue.
 * @buf_in_host:	Input buffer queue. Host enqueue filled buffers.
 * @buf_in_mve:		Input buffer queue. MVE return empty buffers.
 * @buf_out_host:	Output buffer queue. Host enqueue empty buffers.
 * @buf_out_mve:	Out buffer queue. MVE return filled buffers.
 * @rpc:		RPC communication area.
 * @ncores:		Number of cores the firmware has been mapped for.
 * @rpc_mem:		Keeps track of RPC allocated memory. Maps MVE virtual
 *                      address to 'struct mvx_mmu_pages' object.
 * @msg_pending:	A subset of the messages that we are waiting for a
 *                      response to.
 * @ops:		Public firmware interface.
 * @ops_priv:		Private firmware interface.
 *
 * There is one firmware instance per active session. The function pointers
 * below are not reentrant and should be protected by the session mutex.
 */
struct mvx_fw {
	struct device *dev;
	const struct mvx_fw_bin *fw_bin;
	struct mvx_mmu *mmu;
	struct mvx_session *session;
	struct mvx_client_ops *client_ops;
	struct mvx_client_session *csession;
	struct mvx_mmu_pages *text;
	struct mvx_mmu_pages *bss;
	struct mvx_mmu_pages *bss_shared;
	struct dentry *dentry;
	void *msg_host;
	void *msg_mve;
	void *buf_in_host;
	void *buf_in_mve;
	void *buf_out_host;
	void *buf_out_mve;
	void *rpc;
	void *fw_print_ram;
	unsigned int ncores;
	DECLARE_HASHTABLE(rpc_mem, MVX_FW_HTABLE_BITS);
	struct mutex rpcmem_mutex;
	unsigned int msg_pending;
	uint32_t latest_used_region_protected_pages;
	uint32_t latest_used_region_outbuf_pages;
	phys_addr_t buf_pa_addr[MVX_FW_REGION_PRINT_RAM+1];
	enum mvx_fw_buffer_attr buf_attr[MVX_FW_REGION_PRINT_RAM+1];

	int (*map_op[MVX_FW_BUF_ATTR_NUM])(struct mvx_fw *fw, void **data, enum mvx_fw_region region);
	void (*unmap_op[MVX_FW_BUF_ATTR_NUM])(struct mvx_fw *fw, void **data, enum mvx_fw_region region);

	struct {
		/**
		 * map_protocol() - MMU map firmware.
		 * @fw:		Pointer to firmware object.
		 */
		int (*map_protocol)(struct mvx_fw *fw);

		/**
		 * unmap_protocol() - MMU unmap firmware.
		 * @fw:		Pointer to firmware object.
		 */
		void (*unmap_protocol)(struct mvx_fw *fw);

		/**
		 * get_region() - Get begin and end address for memory region.
		 * @region:	Which memory region to get addresses for.
		 * @begin:	MVE virtual begin address.
		 * @end:	MVE virtual end address.
		 *
		 * Return: 0 on success, else error code.
		 */
		int (*get_region)(enum mvx_fw_region region,
				  uint32_t *begin,
				  uint32_t *end);

		/**
		 * get_message() - Read message from firmware message queue.
		 * @fw:		Pointer to firmware object.
		 * @msg:	Firmware message.
		 *
		 * Return: 1 if message was received, 0 if no message was
		 *         received, else error code.
		 */
		int (*get_message)(struct mvx_fw *fw,
				   struct mvx_fw_msg *msg);

		/**
		 * put_message() - Write message to firmware message queue.
		 * @fw:		Pointer to firmware object.
		 * @msg:	Firmware message.
		 *
		 * Return: 0 on success, else error code.
		 */
		int (*put_message)(struct mvx_fw *fw,
				   struct mvx_fw_msg *msg);

		/**
		 * handle_rpc() - Handle RPC message.
		 * @fw:		Pointer to firmware object.
		 *
		 * Return: 1 RPC message handled, 0 no RPC message,
		 *         else error code.
		 */
		int (*handle_rpc)(struct mvx_fw *fw);

		/**
		 * handle_fw_ram_print() - Print firmware log from share ram.
		 * @fw:		Pointer to firmware object.
		 *
		 * Return: 1 FW ram log printed, 0 no FW ram log printed,
		 *         else error code.
		 */
		int (*handle_fw_ram_print)(struct mvx_fw *fw);

		/**
		 * print_stat() - Print debug stats to seq-file.
		 * @fw:		Pointer to firmware object.
		 * @ind:	Indentation level.
		 * s:		Pointer to seq-file.
		 *
		 * Return: 0 on success, else error code.
		 */
		int (*print_stat)(struct mvx_fw *fw,
				  int ind,
				  struct seq_file *s);

		/**
		 * print_debug() - Print debug information.
		 * @fw:		Pointer to firmware object.
		 */
		void (*print_debug)(struct mvx_fw *fw);
	} ops;

	struct {
		/**
		 * send_idle_ack() - Send IDLE ACK message.
		 * @fw:		Pointer to firmware object.
		 *
		 * IDLE ACK message will be sent to the firmware if it is
		 * supported by a host protocol, otherwise the call will be
		 * ignored.
		 *
		 * Return: 0 on success, else error code.
		 */
		int (*send_idle_ack)(struct mvx_fw *fw);

		/**
		 * to_mve_profile() - Convert MVX profile to MVE value.
		 * @mvx_profile:	MVX profile.
		 * @mvx_profile:	MVE profile.
		 *
		 * Return: 0 on success, else error code.
		 */
		int (*to_mve_profile)(unsigned int mvx_profile,
				      uint16_t *mve_profile);

		/**
		 * to_mve_level() - Convert MVX level to MVE value.
		 * @mvx_level:	MVX level.
		 * @mvx_level:	MVE level.
		 *
		 * Return: 0 on success, else error code.
		 */
		int (*to_mve_level)(unsigned int mvx_level,
				    uint16_t *mve_level);
	} ops_priv;
};

/****************************************************************************
 * Exported functions
 ****************************************************************************/

/**
 * mvx_fw_factory() - Construct a firmware object.
 * @fw:		Pointer to fw.
 * @fw:_bin	Pointer for firmware binary.
 * @mmu:	Pointer to MMU instance.
 * @session:	Pointer to session.
 * @client_ops:	Pointer to client operations.
 * @csession:	Client session this firmware instance is registered to.
 * @ncores:	Number of cores to configure.
 * @parent:	Debugfs entry for parent debugfs directory entry.
 *
 * Return: 0 on success, else error code.
 */
int mvx_fw_factory(struct mvx_fw *fw,
		   struct mvx_fw_bin *fw_bin,
		   struct mvx_mmu *mmu,
		   struct mvx_session *session,
		   struct mvx_client_ops *client_ops,
		   struct mvx_client_session *csession,
		   unsigned int ncores,
		   struct dentry *parent);

/**
 * mvx_fw_destruct() - Destroy firmware interface instance.
 * @fw:		Pointer to fw.
 */
void mvx_fw_destruct(struct mvx_fw *fw);

#endif /* _MVX_FIRMWARE_H_ */
