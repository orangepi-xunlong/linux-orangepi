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

/*
 * Copyright:
 * ----------------------------------------------------------------------------
 * This confidential and proprietary software may be used only as authorized
 * by a licensing agreement from Arm Technology (China) Co., Ltd.
 *      (C) COPYRIGHT 2021-2021 Arm Technology (China) Co., Ltd.
 * The entire notice above must be reproduced on all authorized copies and
 * copies may only be made to the extent permitted by a licensing agreement
 * from Arm Technology (China) Co., Ltd.
 * ----------------------------------------------------------------------------
 */
#ifndef __FW_INCLUDE__MVE_PROTOCOL_DEF_H__
#define __FW_INCLUDE__MVE_PROTOCOL_DEF_H__

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __KERNEL__
#include <linux/types.h>
#else
#include <stdint.h>
#endif

/*****************************************************************************
 *
 * Communication protocol between the host/driver and the MVE firmware,
 * the 'host interface'.
 *
 * MVE == LINLON Video Engine
 *
 * Protocol version 2.5
 *
 * Note: Message structs may be expanded in the future; the host should
 * use the 'size' of the message to determine how many bytes to
 * read from the message queue, rather than a sizeof(struct).
 *
 ****************************************************************************/

















/*****************************************************************************
 *
 * Virtual memory regions
 *
 * ..._ADDR_BEGIN gives the starting virtual address of the region,
 * and ..._ADDR_END the (non-inclusive) ending address, such that
 * the size of the region is obtained with the subtraction
 * (..._ADDR_END - ..._ADDR_BEGIN).
 *
 ****************************************************************************/

/* Memory region for first firmware instance */
#define MVE_MEM_REGION_FW_INSTANCE0_ADDR_BEGIN (0x00000000u)
#define MVE_MEM_REGION_FW_INSTANCE0_ADDR_END   (0x000FFFFFu + 1)

/*
 * Areas for communication between host and MVE are placed in the interval
 * 0x10079000 - 0x1007FFFF, see special defines further down.
 */

/* PROTECTED virtual memory region */
#define MVE_MEM_REGION_PROTECTED_ADDR_BEGIN    (0x20000000u)
#define MVE_MEM_REGION_PROTECTED_ADDR_END      (0x4FFFFFFFu + 1)

/* FRAMEBUF virtual memory region */
#define MVE_MEM_REGION_FRAMEBUF_ADDR_BEGIN     (0x50000000u)
#define MVE_MEM_REGION_FRAMEBUF_ADDR_END       (0x7FFFFFFFu + 1)

/* Memory regions for other firmware instances */
#define MVE_MEM_REGION_FW_INSTANCE1_ADDR_BEGIN (0x80000000u)
#define MVE_MEM_REGION_FW_INSTANCE1_ADDR_END \
    (MVE_MEM_REGION_FW_INSTANCE1_ADDR_BEGIN + MVE_MEM_REGION_FW_INSTANCE0_ADDR_END)

#define MVE_MEM_REGION_FW_INSTANCE2_ADDR_BEGIN (0x90000000u)
#define MVE_MEM_REGION_FW_INSTANCE2_ADDR_END \
    (MVE_MEM_REGION_FW_INSTANCE2_ADDR_BEGIN + MVE_MEM_REGION_FW_INSTANCE0_ADDR_END)

#define MVE_MEM_REGION_FW_INSTANCE3_ADDR_BEGIN (0xA0000000u)
#define MVE_MEM_REGION_FW_INSTANCE3_ADDR_END \
    (MVE_MEM_REGION_FW_INSTANCE3_ADDR_BEGIN + MVE_MEM_REGION_FW_INSTANCE0_ADDR_END)

#define MVE_MEM_REGION_FW_INSTANCE4_ADDR_BEGIN (0xB0000000u)
#define MVE_MEM_REGION_FW_INSTANCE4_ADDR_END \
    (MVE_MEM_REGION_FW_INSTANCE4_ADDR_BEGIN + MVE_MEM_REGION_FW_INSTANCE0_ADDR_END)

#define MVE_MEM_REGION_FW_INSTANCE5_ADDR_BEGIN (0xC0000000u)
#define MVE_MEM_REGION_FW_INSTANCE5_ADDR_END \
    (MVE_MEM_REGION_FW_INSTANCE5_ADDR_BEGIN + MVE_MEM_REGION_FW_INSTANCE0_ADDR_END)

#define MVE_MEM_REGION_FW_INSTANCE6_ADDR_BEGIN (0xD0000000u)
#define MVE_MEM_REGION_FW_INSTANCE6_ADDR_END \
    (MVE_MEM_REGION_FW_INSTANCE6_ADDR_BEGIN + MVE_MEM_REGION_FW_INSTANCE0_ADDR_END)

#define MVE_MEM_REGION_FW_INSTANCE7_ADDR_BEGIN (0xE0000000u)
#define MVE_MEM_REGION_FW_INSTANCE7_ADDR_END \
    (MVE_MEM_REGION_FW_INSTANCE7_ADDR_BEGIN + MVE_MEM_REGION_FW_INSTANCE0_ADDR_END)

/* 0xF0000000 - 0xFFFFFFFF is used internally in MVE */
















/*****************************************************************************
 *
 * Communication queues between HOST/DRIVER and MVE
 *
 * Address for queue for messages in to MVE,
 * one struct mve_comm_area_host located here
 *
 ****************************************************************************/

#define MVE_COMM_MSG_INQ_ADDR   (0x10079000u)

/* Address for queue for messages out from MVE,
 * one struct mve_comm_area_mve located here
 */
#define MVE_COMM_MSG_OUTQ_ADDR  (0x1007A000u)

/* Address for queue for input buffers in to MVE,
 * one struct mve_comm_area_host located here
 */
#define MVE_COMM_BUF_INQ_ADDR   (0x1007B000u)

/* Address for queue for input buffers returned from MVE,
 * one struct mve_comm_area_mve located here
 */
#define MVE_COMM_BUF_INRQ_ADDR  (0x1007C000u)

/* Address for queue for output buffers in to MVE,
 * one struct mve_comm_area_host located here
 */
#define MVE_COMM_BUF_OUTQ_ADDR  (0x1007D000u)

/* Address for queue for output buffers returned from MVE,
 * one struct mve_comm_area_mve located here
 */
#define MVE_COMM_BUF_OUTRQ_ADDR (0x1007E000u)

/* One struct mve_rpc_communication_area located here */
#define MVE_COMM_RPC_ADDR       (0x1007F000u)

/* Address for ram_print buffer in FW */
#define MVE_FW_PRINT_RAM_ADDR  (0x10100000u)
#define MVE_FW_PRINT_RAM_SIZE  (0x80000u)

/* One page of memory (4 kB) is used for each queue,
 * so maximum 1024 words, but need room for some counters as well,
 * see structs mve_comm_area_mve and mve_comm_area_host below.
 */
#define MVE_COMM_QUEUE_SIZE_IN_WORDS 1020

/* This is the part of the message area that is written by host. */
struct mve_comm_area_host
{
    volatile uint16_t out_rpos;
    volatile uint16_t in_wpos;
    volatile uint32_t reserved[ 3 ];
    /*
     * Queue of messages to MVE, each block of data prefixed with
     * a mve_msg_header
     */
    volatile uint32_t in_data[ MVE_COMM_QUEUE_SIZE_IN_WORDS ];
};

/* This is the part of the message area that is written by MVE. */
struct mve_comm_area_mve
{
    volatile uint16_t out_wpos;
    volatile uint16_t in_rpos;
    volatile uint32_t reserved[ 3 ];
    /*
     * Queue of messages to host, each block of data prefixed with
     * a mve_msg_header
     */
    volatile uint32_t out_data[ MVE_COMM_QUEUE_SIZE_IN_WORDS ];
};

#define MVE_RPC_AREA_SIZE_IN_WORDS 256
#define MVE_RPC_DATA_SIZE_IN_WORDS (MVE_RPC_AREA_SIZE_IN_WORDS - 3)
union mve_rpc_params
{
    volatile uint32_t data[ MVE_RPC_DATA_SIZE_IN_WORDS ];
    struct
    {
        char string[ MVE_RPC_DATA_SIZE_IN_WORDS * 4 ];
    } debug_print;
    struct
    {
        uint32_t size;
        uint32_t max_size;
        uint8_t region; /* Memory region selection */
            #define MVE_MEM_REGION_PROTECTED (0)
            #define MVE_MEM_REGION_OUTBUF    (1)
            #define MVE_MEM_REGION_FRAMEBUF  (MVE_MEM_REGION_OUTBUF)

        /* The newly allocated memory must be placed
         * on (at least) a 2^(log2_alignment) boundary
         */
        uint8_t log2_alignment;
    } mem_alloc;
    struct
    {
        uint32_t ve_pointer;
        uint32_t new_size;
    } mem_resize;
    struct
    {
        uint32_t ve_pointer;
    } mem_free;
};

struct mve_rpc_communication_area
{
    volatile uint32_t state;
        #define MVE_RPC_STATE_FREE   (0)
        #define MVE_RPC_STATE_PARAM  (1)
        #define MVE_RPC_STATE_RETURN (2)
    volatile uint32_t call_id;
        #define MVE_RPC_FUNCTION_DEBUG_PRINTF (1)
        #define MVE_RPC_FUNCTION_MEM_ALLOC    (2)
        #define MVE_RPC_FUNCTION_MEM_RESIZE   (3)
        #define MVE_RPC_FUNCTION_MEM_FREE     (4)
    volatile uint32_t size;
    union mve_rpc_params params;
};

struct mve_fw_ram_print_head_aera
{
    volatile uint32_t rd_cnt;
    volatile uint32_t reserved0[15];

    volatile uint32_t flag;
    volatile uint32_t index;
    volatile uint32_t wr_cnt;
    volatile uint32_t reserved1[13];
};















/*********************************************************************
 *
 *   Message codes
 *
 *********************************************************************/

/* Messages consist of one struct mve_msg_header, possibly followed
 * by extra data.
 */
struct mve_msg_header
{
    uint16_t code;
        /* REQUESTs are messages from the
         * host/driver to the firmware:               Code:     Extra data in message: */
        #define MVE_REQUEST_CODE_GO                   (1001) /* no extra data */
        #define MVE_REQUEST_CODE_STOP                 (1002) /* no extra data */
        #define MVE_REQUEST_CODE_INPUT_FLUSH          (1003) /* no extra data */
        #define MVE_REQUEST_CODE_OUTPUT_FLUSH         (1004) /* no extra data */
        #define MVE_REQUEST_CODE_SWITCH               (1005) /* no extra data */
        #define MVE_REQUEST_CODE_PING                 (1006) /* no extra data */
        #define MVE_REQUEST_CODE_DUMP                 (1008) /* no extra data */
        #define MVE_REQUEST_CODE_JOB                  (1009) /* struct mve_request_job */
        #define MVE_REQUEST_CODE_SET_OPTION           (1010) /* struct mve_request_set_option (variable size) */
        #define MVE_REQUEST_CODE_RELEASE_REF_FRAME    (1011) /* struct mve_request_release_ref_frame */
        #define MVE_REQUEST_CODE_IDLE_ACK             (1012) /* no extra data */
        #define MVE_REQUEST_CODE_DEBUG                (1013) /* level: 0 for disable, refer to fw_log_level */
        /* RESPONSEs are messages from
         * the firmware to the host: */
        #define MVE_RESPONSE_CODE_SWITCHED_IN         (2001) /* struct mve_response_switched_in */
        #define MVE_RESPONSE_CODE_SWITCHED_OUT        (2002) /* struct mve_response_switched_out */
        #define MVE_RESPONSE_CODE_SET_OPTION_CONFIRM  (2003) /* no extra data */
        #define MVE_RESPONSE_CODE_JOB_DEQUEUED        (2004) /* struct mve_response_job_dequeued */
        #define MVE_RESPONSE_CODE_INPUT               (2005) /* no extra data, but buffer placed in buffer queue */
        #define MVE_RESPONSE_CODE_OUTPUT              (2006) /* no extra data, but buffer placed in buffer queue */
        #define MVE_RESPONSE_CODE_INPUT_FLUSHED       (2007) /* no extra data */
        #define MVE_RESPONSE_CODE_OUTPUT_FLUSHED      (2008) /* no extra data */
        #define MVE_RESPONSE_CODE_PONG                (2009) /* no extra data */
        #define MVE_RESPONSE_CODE_ERROR               (2010) /* struct mve_response_error */
        #define MVE_RESPONSE_CODE_STATE_CHANGE        (2011) /* struct mve_response_state_change */
        #define MVE_RESPONSE_CODE_DUMP                (2012) /* no extra data */
        #define MVE_RESPONSE_CODE_IDLE                (2013) /* no extra data */
        #define MVE_RESPONSE_CODE_FRAME_ALLOC_PARAM   (2014) /* struct mve_response_frame_alloc_parameters */
        #define MVE_RESPONSE_CODE_SEQUENCE_PARAMETERS (2015) /* struct mve_response_sequence_parameters */
        #define MVE_RESPONSE_CODE_EVENT               (2016) /* struct mve_response_event (variable size) */
        #define MVE_RESPONSE_CODE_SET_OPTION_FAIL     (2017) /* struct mve_response_set_option_failed */
        #define MVE_RESPONSE_CODE_REF_FRAME_UNUSED    (2018) /* struct mve_response_ref_frame_unused */
        #define MVE_RESPONSE_CODE_DEBUG               (2019) /* no extra data */
        /* BUFFERs are sent from host to firmware,
         * and then return at some time: */
        #define MVE_BUFFER_CODE_FRAME                 (3001) /* struct mve_buffer_frame */
        #define MVE_BUFFER_CODE_BITSTREAM             (3002) /* struct mve_buffer_bitstream */
        #define MVE_BUFFER_CODE_PARAM                 (3003) /* struct mve_buffer_param */
        #define MVE_BUFFER_CODE_GENERAL               (3004) /* struct mve_buffer_general */

    uint16_t size; /* size in bytes of trailing data, 0 if none */
};
















/*********************************************************************
 *
 *   REQUESTs are messages from the host to the firmware
 *
 *   Some of the MVE_REQUEST_CODE_ codes are followed by one of the
 *   structs below.
 *
 *********************************************************************/

struct mve_request_job
{
    uint16_t cores; /* >= 1, number of cores to use, must match request to HW scheduler */
    uint16_t frames; /* number of frames to process, zero means infinite */
    uint32_t flags; /* can be zero */
        #define MVE_JOB_FLAG_DISABLE_BNDMGR    (0x01)
};

struct mve_request_set_option
{
    uint32_t index;
        #define MVE_SET_OPT_INDEX_NALU_FORMAT              (1)  /* see arg, MVE_OPT_NALU_FORMAT_ */
        #define MVE_SET_OPT_INDEX_STREAM_ESCAPING          (2)  /* arg=1 to enable (default), arg=0 to disable */
        #define MVE_SET_OPT_INDEX_PROFILE_LEVEL            (3)  /* data.profile_level */
        #define MVE_SET_OPT_INDEX_HOST_PROTOCOL_PRINTS     (4)  /* arg=1 to enable, arg=0 to disable (default) */
        #define MVE_SET_OPT_INDEX_PROFILING                (5)  /* arg=1 to enable, arg=0 to disable (default) */
        #define MVE_SET_OPT_INDEX_DISABLE_FEATURES         (6)  /* see arg, MVE_OPT_DISABLE_FEATURE_ */
        #define MVE_SET_OPT_INDEX_IGNORE_STREAM_HEADERS    (7)  /* decode, arg=1 to enable,
                                                                 * arg=0 to disable (default) */
        #define MVE_SET_OPT_INDEX_FRAME_REORDERING         (8)  /* decode, arg=1 to enable (default),
                                                                 * arg=0 to disable */
        #define MVE_SET_OPT_INDEX_INTBUF_SIZE              (9)  /* decode, arg = suggested limit of intermediate
                                                                 * buffer allocation */
        #define MVE_SET_OPT_INDEX_ENC_P_FRAMES             (16) /* encode, arg = nPFrames */
        #define MVE_SET_OPT_INDEX_ENC_B_FRAMES             (17) /* encode, arg = number of B frames */
        #define MVE_SET_OPT_INDEX_GOP_TYPE                 (18) /* encode, see arg */
        #define MVE_SET_OPT_INDEX_INTRA_MB_REFRESH         (19) /* encode, arg */
        #define MVE_SET_OPT_INDEX_ENC_CONSTR_IPRED         (20) /* encode, arg = 0 or 1 */
        #define MVE_SET_OPT_INDEX_ENC_ENTROPY_SYNC         (21) /* encode, arg = 0 or 1 */
        #define MVE_SET_OPT_INDEX_ENC_TEMPORAL_MVP         (22) /* encode, arg = 0 or 1 */
        #define MVE_SET_OPT_INDEX_TILES                    (23) /* encode, data.tiles */
        #define MVE_SET_OPT_INDEX_ENC_MIN_LUMA_CB_SIZE     (24) /* HEVC encode, arg = 8 or 16,
                                                                 * for sizes 8x8 or 16x16 */
        #define MVE_SET_OPT_INDEX_ENC_MB_TYPE_ENABLE       (25) /* encode, see arg */
        #define MVE_SET_OPT_INDEX_ENC_MB_TYPE_DISABLE      (26) /* encode, see arg */
        #define MVE_SET_OPT_INDEX_ENC_H264_CABAC           (27) /* encode, arg = 0 or 1, enabled by default */
        #define MVE_SET_OPT_INDEX_ENC_SLICE_SPACING        (28) /* encode, arg = suggested number of
                                                                 * CTUs/macroblocks in a slice */
        #define MVE_SET_OPT_INDEX_ENC_VP9_PROB_UPDATE      (30) /* VP9 encode, see arg */
        #define MVE_SET_OPT_INDEX_RESYNC_INTERVAL          (31) /* JPEG encode, arg = nRestartInterval
                                                                 * = nResynchMarkerSpacing */
        #define MVE_SET_OPT_INDEX_HUFFMAN_TABLE            (32) /* JPEG encode, data.huffman_table */
        #define MVE_SET_OPT_INDEX_QUANT_TABLE              (33) /* JPEG encode, data.quant_table */
        #define MVE_SET_OPT_INDEX_ENC_EXPOSE_REF_FRAMES    (34) /* encode only, disabled by default */
        #define MVE_SET_OPT_INDEX_MBINFO_OUTPUT            (35) /* encode, arg=1 to enable,
                                                                 * arg=0 to disable (default) */
        #define MVE_SET_OPT_INDEX_MV_SEARCH_RANGE          (36) /* encode, data,motion_vector_search_range */
        #define MVE_SET_OPT_INDEX_ENC_STREAM_BITDEPTH      (38) /* encode, data.bitdepth, to set other bitdepth
                                                                 * of encoded stream than of input frames */
        #define MVE_SET_OPT_INDEX_ENC_STREAM_CHROMA_FORMAT (39) /* encode, arg, to set other chroma format of
                                                                 * encoded stream than of input frames */
        #define MVE_SET_OPT_INDEX_ENC_RGB_TO_YUV_MODE      (40) /* encode, arg, select which way RGB is converted
                                                                 * to YUV before encoding */
        #define MVE_SET_OPT_INDEX_ENC_BANDWIDTH_LIMIT      (41) /* encode, arg, the maxium bandwidth limit defined
                                                                 * by host */
        #define MVE_SET_OPT_INDEX_WATCHDOG_TIMEOUT         (42) /* arg=timeout, arg=0 to disable */
        #define MVE_SET_OPT_INDEX_ENC_CABAC_INIT_IDC       (43) /* encode, arg; 0,1,2 for H264; 0,1 for HEVC */
        #define MVE_SET_OPT_INDEX_ENC_ADPTIVE_QUANTISATION (44) /* encode (h264 and hevc) */
        #define MVE_SET_OPT_INDEX_QP_DELTA_I_P             (45)
        #define MVE_SET_OPT_INDEX_QP_DELTA_I_B_REF         (46)
        #define MVE_SET_OPT_INDEX_QP_DELTA_I_B_NONREF      (47)
        #define MVE_SET_OPT_INDEX_CB_QP_OFFSET             (48)
        #define MVE_SET_OPT_INDEX_CR_QP_OFFSET             (49)
        #define MVE_SET_OPT_INDEX_LAMBDA_SCALE             (50) /* encode, data.lambda_scale */
        #define MVE_SET_OPT_INDEX_ENC_MAX_NUM_CORES        (51) /* maximum number of cores */
        #define MVE_SET_OPT_INDEX_ENC_EXTRA_REFS           (52) /* configure number of extra ref buffers */
        #define MVE_SET_OPT_INDEX_QP_DELTA_RAW_I_P         (53)
        #define MVE_SET_OPT_INDEX_QP_DELTA_RAW_I_B_REF     (54)
        #define MVE_SET_OPT_INDEX_QP_DELTA_RAW_I_B_NONREF  (55)
        #define MVE_SET_OPT_INDEX_ENC_FIXED_QP             (56)
        /* ARBITRARY_DOWNSCALE */
        #define MVE_SET_OPT_INDEX_DEC_DOWNSCALE            (57) /* decode, set downscaled width and height */
        #define MVE_SET_OPT_INDEX_FLUSHLESS_REFBANK        (58) /* configure AFBC ref bank for individual buffer
                                                                 * allocation. Forced internally for flushless
                                                                 * resolution change codecs */
        #define MVE_SET_OPT_INDEX_ENC_CROP_RARAM_LEFT      (62)
        #define MVE_SET_OPT_INDEX_ENC_CROP_RARAM_RIGHT     (63)
        #define MVE_SET_OPT_INDEX_ENC_CROP_RARAM_TOP       (64)
        #define MVE_SET_OPT_INDEX_ENC_CROP_RARAM_BOTTOM    (65)
        /* LONG_TERM_REFERENCE */
        #define MVE_SET_OPT_INDEX_ENC_LTR_MODE             (66)
        #define MVE_SET_OPT_INDEX_ENC_LTR_PERIOD           (67)
        #define MVE_SET_OPT_INDEX_DEC_DOWNSCALE_POS_MODE   (69)

    union
    {
        uint32_t arg; /* Most options only need a uint32_t as argument */
            /* For option MVE_SET_OPT_INDEX_NALU_FORMAT, arg should
             * be one of these: */
            #define MVE_OPT_NALU_FORMAT_START_CODES             (1)
            #define MVE_OPT_NALU_FORMAT_ONE_NALU_PER_BUFFER     (2)
            #define MVE_OPT_NALU_FORMAT_ONE_BYTE_LENGTH_FIELD   (4)
            #define MVE_OPT_NALU_FORMAT_TWO_BYTE_LENGTH_FIELD   (8)
            #define MVE_OPT_NALU_FORMAT_FOUR_BYTE_LENGTH_FIELD  (16)
            /* For option MVE_SET_OPT_INDEX_GOP_TYPE, arg should
             * be one of these: */
            #define MVE_OPT_GOP_TYPE_BIDIRECTIONAL              (1)
            #define MVE_OPT_GOP_TYPE_LOW_DELAY                  (2)
            #define MVE_OPT_GOP_TYPE_PYRAMID                    (3)
            /* For option MVE_SET_OPT_INDEX_ENC_VP9_PROB_UPDATE,
             * arg should be one of these: */
            #define MVE_OPT_VP9_PROB_UPDATE_DISABLED            (0)
            #define MVE_OPT_VP9_PROB_UPDATE_IMPLICIT            (1)
            #define MVE_OPT_VP9_PROB_UPDATE_EXPLICIT            (2)
            /* For option MVE_SET_OPT_INDEX_DISABLE_FEATURES, arg
             * should be a bitmask with features to disable: */
            #define MVE_OPT_DISABLE_FEATURE_AFBC_COMP           (0x00000001) /* VDMA AFBC Compression   */
            #define MVE_OPT_DISABLE_FEATURE_REF_CACHE           (0x00000002) /* REF caching             */
            #define MVE_OPT_DISABLE_FEATURE_DEBLOCK             (0x00000004) /* Deblocking              */
            #define MVE_OPT_DISABLE_FEATURE_SAO                 (0x00000008) /* SAO                     */
            #define MVE_OPT_DISABLE_FEATURE_PIC_OUTPUT          (0x00000020) /* Picture Output Removal  */
            #define MVE_OPT_DISABLE_FEATURE_PIPE                (0x00000040) /* Pipe (i.e. parser-only) */
            #define MVE_OPT_DISABLE_FEATURE_SLEEP               (0x00000080) /* Clock gating
                                                                              * (SOC_SYSCTRL.SLEEP bit) */
            #define MVE_OPT_DISABLE_FEATURE_AFBC_LEGACY_REF     (0x00000100) /* Enables tiled AFBC format in
                                                                              * reference buffers. Ignored
                                                                              * for decode AFBC output */
            #define MVE_OPT_DISABLE_FEATURE_REF_PICS            (0x00000400) /* Forces use of static 16x16
                                                                              * reference pics */
            #define MVE_OPT_DISABLE_FEATURE_CHNG_RECT_WA        (0x00000800) /* Disables workaround */
            #define MVE_OPT_DISABLE_FEATURE_REFSZ_LIMIT         (0x00001000) /* Disable REFSZ bw limit */
            /* For options MVE_SET_OPT_INDEX_ENC_MB_TYPE_ENABLE
             * and MVE_SET_OPT_INDEX_ENC_MB_TYPE_DISABLE, arg
             * should be a bitmask of MVE_MBTYPEs: */
            #define MVE_MBTYPE_4x4                              (0x00000001) /* 4x4 inter     */
            #define MVE_MBTYPE_4x8                              (0x00000002) /* 4x8 inter     */
            #define MVE_MBTYPE_8x4                              (0x00000004) /* 8x4 inter     */
            #define MVE_MBTYPE_8x8                              (0x00000008) /* 8x8 inter     */
            #define MVE_MBTYPE_8x16                             (0x00000010) /* 8x16 inter    */
            #define MVE_MBTYPE_16x8                             (0x00000020) /* 16x8 inter    */
            #define MVE_MBTYPE_16x16                            (0x00000040) /* 16x16 inter   */
            #define MVE_MBTYPE_PSKIP                            (0x00000080) /* P Skip inter  */
            #define MVE_MBTYPE_I4x4                             (0x00000100) /* 4x4 intra     */
            #define MVE_MBTYPE_I8x8                             (0x00000200) /* 8x8 intra     */
            #define MVE_MBTYPE_I16x16                           (0x00000400) /* 16x16 intra   */
            #define MVE_MBTYPE_I32x32                           (0x00000800) /* 32x32 intra   */
            #define MVE_MBTYPE_16x32                            (0x00001000) /* 16x32 inter   */
            #define MVE_MBTYPE_32x16                            (0x00002000) /* 32x16 inter   */
            #define MVE_MBTYPE_32x32                            (0x00004000) /* 32x32 inter   */
            /* For option MVE_SET_OPT_INDEX_ENC_RGB_TO_YUV_MODE,
             * arg should be one of these: */
            #define MVE_OPT_RGB_TO_YUV_BT601_STUDIO             (0)
            #define MVE_OPT_RGB_TO_YUV_BT601_FULL               (1)
            #define MVE_OPT_RGB_TO_YUV_BT709_STUDIO             (2)
            #define MVE_OPT_RGB_TO_YUV_BT709_FULL               (3)
            /* For option MVE_SET_OPT_INDEX_ENC_EXPOSE_REF_FRAMES,
             * arg should be one of: */
            #define MVE_OPT_REF_OUTPUT_NONE                      (0) /* No REF output */
            #define MVE_OPT_REF_OUTPUT_USED_FOR_REF              (1) /* Output reference frames */
            #define MVE_OPT_REF_OUTPUT_ALL                       (2) /* Output/reconstruct all frames */
        struct
        {
            uint16_t profile;
                /* AVC/H.264 profiles */
                #define MVE_OPT_PROFILE_H264_BASELINE               (1)
                #define MVE_OPT_PROFILE_H264_MAIN                   (2)
                #define MVE_OPT_PROFILE_H264_HIGH                   (3)
                /* HEVC/H.265 profiles */
                #define MVE_OPT_PROFILE_H265_MAIN                   (1)
                #define MVE_OPT_PROFILE_H265_MAIN_STILL             (2)
                #define MVE_OPT_PROFILE_H265_MAIN_INTRA             (3)
                #define MVE_OPT_PROFILE_H265_MAIN_10                (4)
                /* VC-1 profiles */
                #define MVE_OPT_PROFILE_VC1_SIMPLE                  (1)
                #define MVE_OPT_PROFILE_VC1_MAIN                    (2)
                #define MVE_OPT_PROFILE_VC1_ADVANCED                (3)
                /* VP8 profiles */
                #define MVE_OPT_PROFILE_VP8_MAIN                    (1)
            uint16_t level;
                /* AVC/H.264 levels */
                #define MVE_OPT_LEVEL_H264_1                        (1)
                #define MVE_OPT_LEVEL_H264_1b                       (2)
                #define MVE_OPT_LEVEL_H264_11                       (3)
                #define MVE_OPT_LEVEL_H264_12                       (4)
                #define MVE_OPT_LEVEL_H264_13                       (5)
                #define MVE_OPT_LEVEL_H264_2                        (6)
                #define MVE_OPT_LEVEL_H264_21                       (7)
                #define MVE_OPT_LEVEL_H264_22                       (8)
                #define MVE_OPT_LEVEL_H264_3                        (9)
                #define MVE_OPT_LEVEL_H264_31                       (10)
                #define MVE_OPT_LEVEL_H264_32                       (11)
                #define MVE_OPT_LEVEL_H264_4                        (12)
                #define MVE_OPT_LEVEL_H264_41                       (13)
                #define MVE_OPT_LEVEL_H264_42                       (14)
                #define MVE_OPT_LEVEL_H264_5                        (15)
                #define MVE_OPT_LEVEL_H264_51                       (16)
                #define MVE_OPT_LEVEL_H264_52                       (17)
                #define MVE_OPT_LEVEL_H264_6                        (18)
                #define MVE_OPT_LEVEL_H264_61                       (19)
                #define MVE_OPT_LEVEL_H264_62                       (20)
                #define MVE_OPT_LEVEL_H264_USER_SUPPLIED_BASE       (32)
                /* The value (MVE_OPT_LEVEL_H264_USER_SUPPLIED_BASE + level_idc) encodes a user
                 * supplied level_idc value in the range 0 to 255 inclusive. If the host supplies a level_idc
                 * value by this method then the encoder will encode this level_idc value in the bitstream
                 * without checking the validity of the level_idc value
                 */
                #define MVE_OPT_LEVEL_H264_USER_SUPPLIED_MAX        (MVE_OPT_LEVEL_H264_USER_SUPPLIED_BASE + 255)
                /* HEVC/H.265 levels */
                #define MVE_OPT_LEVEL_H265_MAIN_TIER_1              (1)
                #define MVE_OPT_LEVEL_H265_HIGH_TIER_1              (2)
                #define MVE_OPT_LEVEL_H265_MAIN_TIER_2              (3)
                #define MVE_OPT_LEVEL_H265_HIGH_TIER_2              (4)
                #define MVE_OPT_LEVEL_H265_MAIN_TIER_21             (5)
                #define MVE_OPT_LEVEL_H265_HIGH_TIER_21             (6)
                #define MVE_OPT_LEVEL_H265_MAIN_TIER_3              (7)
                #define MVE_OPT_LEVEL_H265_HIGH_TIER_3              (8)
                #define MVE_OPT_LEVEL_H265_MAIN_TIER_31             (9)
                #define MVE_OPT_LEVEL_H265_HIGH_TIER_31             (10)
                #define MVE_OPT_LEVEL_H265_MAIN_TIER_4              (11)
                #define MVE_OPT_LEVEL_H265_HIGH_TIER_4              (12)
                #define MVE_OPT_LEVEL_H265_MAIN_TIER_41             (13)
                #define MVE_OPT_LEVEL_H265_HIGH_TIER_41             (14)
                #define MVE_OPT_LEVEL_H265_MAIN_TIER_5              (15)
                #define MVE_OPT_LEVEL_H265_HIGH_TIER_5              (16)
                #define MVE_OPT_LEVEL_H265_MAIN_TIER_51             (17)
                #define MVE_OPT_LEVEL_H265_HIGH_TIER_51             (18)
                #define MVE_OPT_LEVEL_H265_MAIN_TIER_52             (19)
                #define MVE_OPT_LEVEL_H265_HIGH_TIER_52             (20)
                #define MVE_OPT_LEVEL_H265_MAIN_TIER_6              (21)
                #define MVE_OPT_LEVEL_H265_HIGH_TIER_6              (22)
                #define MVE_OPT_LEVEL_H265_MAIN_TIER_61             (23)
                #define MVE_OPT_LEVEL_H265_HIGH_TIER_61             (24)
                #define MVE_OPT_LEVEL_H265_MAIN_TIER_62             (25)
                #define MVE_OPT_LEVEL_H265_HIGH_TIER_62             (26)
        } profile_level;
        struct
        {
            int32_t mv_search_range_x;
            int32_t mv_search_range_y;
        } motion_vector_search_range;
        struct
        {
            uint32_t type;
                #define MVE_OPT_HUFFMAN_TABLE_DC_LUMA               (1)
                #define MVE_OPT_HUFFMAN_TABLE_AC_LUMA               (2)
                #define MVE_OPT_HUFFMAN_TABLE_DC_CHROMA             (3)
                #define MVE_OPT_HUFFMAN_TABLE_AC_CHROMA             (4)
            uint8_t number_of_huffman_of_code_length[ 16 ];
            uint8_t table[ 162 ]; /* 12 are used for DC, 162 for AC */
        } huffman_table;
        struct
        {
            uint32_t type;
                #define MVE_OPT_QUANT_TABLE_LUMA                    (1)
                #define MVE_OPT_QUANT_TABLE_CHROMA                  (2)
            uint8_t matrix[ 64 ];
        } quant_table;
        struct
        {
            /* For HEVC, tile_cols must be zero. For VP9, tile_rows
             * and tile_cols must be powers of 2. */
            uint16_t tile_rows;
            uint16_t tile_cols;
        } tiles;
        struct
        {
            uint16_t luma_bitdepth;
            uint16_t chroma_bitdepth;
        } bitdepth;
        struct
        {
            /* Scale factors, and their square roots, for the lambda
             * coefficients used by the encoder, in unsigned Q8 fixed-point
             * format. Default (no scaling) is 1.0 (so 0x0100 in hex).
             */
            uint16_t lambda_scale_i_q8;
            uint16_t lambda_scale_sqrt_i_q8;
            uint16_t lambda_scale_p_q8;
            uint16_t lambda_scale_sqrt_p_q8;
            uint16_t lambda_scale_b_ref_q8;
            uint16_t lambda_scale_sqrt_b_ref_q8;
            uint16_t lambda_scale_b_nonref_q8;
            uint16_t lambda_scale_sqrt_b_nonref_q8;
        } lambda_scale;
        /* ARBITRARY_DOWNSCALE */
        struct
        {
            uint16_t width;
            uint16_t height;
        } downscaled_frame;
        struct
        {
            uint32_t mode;
        } dsl_pos;
    } data;
};

struct mve_request_release_ref_frame
{
    /* Decode: For a frame buffer that MVE has returned
     * marked as _REF_FRAME, the host can send this message
     * to ask the MVE to release the buffer as soon as it is
     * no longer used as reference anymore. (Otherwise, in
     * normal operation, the host would re-enqueue the buffer
     * to the MVE when it has been displayed and can be over-
     * written with a new frame.)
     *
     * Note: When a frame is no longer used as reference depends
     * on the stream being decoded, and there is no way to
     * guarantee a short response time, the response may not
     * come until the end of the stream.
     *
     * Encode: Return this reference buffer to the firmware
     * so it can be reused. This is only useful when the
     * MVE_SET_OPT_INDEX_ENC_EXPOSE_REF_FRAMES is used and reference
     * frames are reported by events (and must be returned).
     */
    uint32_t buffer_address;
};














/*********************************************************************
 *
 *   RESPONSEs are messages from the host to the firmware
 *
 *   Some of the MVE_RESPONSE_CODE_ codes are followed by one of the
 *   structs below.
 *
 *********************************************************************/

/* Sent when firmware has booted.
 */
struct mve_response_switched_in
{
    uint32_t core;
};

/* Sent when last core in a session has switched out.
 */
struct mve_response_switched_out
{
    uint32_t core;
    uint32_t reason;
    uint32_t sub_reason;
};

/* Response confirming state transition after either GO or STOP
 * command from host.
 */
struct mve_response_state_change
{
    uint32_t new_state;
        #define MVE_STATE_STOPPED (0)
        #define MVE_STATE_RUNNING (2)
};

/* Message sent when the all cores in the session have dequeued a
 * job from the firmware job queue.
 */
struct mve_response_job_dequeued
{
    uint32_t valid_job;
};

/* Fatal error message from firmware, if sent then no further
 * operation is possible.
 */
struct mve_response_error
{
    uint32_t error_code;
        #define MVE_ERROR_ABORT          (1)
        #define MVE_ERROR_OUT_OF_MEMORY  (2)
        #define MVE_ERROR_ASSERT         (3)
        #define MVE_ERROR_UNSUPPORTED    (4)
        #define MVE_ERROR_INVALID_BUFFER (6)
        #define MVE_ERROR_INVALID_STATE  (8)
        #define MVE_ERROR_WATCHDOG       (9)

    #define MVE_MAX_ERROR_MESSAGE_SIZE  (128)
    char message[ MVE_MAX_ERROR_MESSAGE_SIZE ];
};

/* When a set-option succeeds, a confirmation message is
 * sent, including the index-code for that particular option.
 */
struct mve_response_set_option_confirm
{
    uint32_t index; /* Same as 'index' in struct mve_request_set_option */
};

/* If a set-option request fails, this message is returned.
 * This is not a fatal error. The set-option had no effect,
 * and the session is still alive.
 * For example, trying to set an option with a too large
 * or small parameter would result in this message.
 * The included text string is meant for development and
 * debugging purposes only.
 * (When a set-option succeeds the set-option-confirm
 * message code is sent instead.)
 */
struct mve_response_set_option_fail
{
    uint32_t index; /* Same as 'index' in struct mve_request_set_option */
    char message[ MVE_MAX_ERROR_MESSAGE_SIZE ];
};

/* Decode only: This message is sent from MVE to the host so that it can
 * allocate large enough output buffers. Output buffers that are to small
 * will be returned to the host marked as 'rejected'.
 */
struct mve_response_frame_alloc_parameters
{
    /* Please note that the below information is a hint
     * for what buffers to allocate, it does not say
     * what actual resolution an output picture has.
     */

    /* To use if allocating PLANAR YUV output buffers: */
    uint16_t planar_alloc_frame_width;
    uint16_t planar_alloc_frame_height;

    /* To use if allocating AFBC output buffers
     * (if interlace, each field needs this size):
     */
    uint32_t afbc_alloc_bytes;

    /* For situations where downscaled AFBC is supported,
     * this number of bytes is needed for the downscaled frame.
     */
    uint32_t afbc_alloc_bytes_downscaled;

    /* When the host allocates an AFBC frame buffer, it should normally set
     * the the afbc_width_in_superblocks to be at least this recommended value.
     * Buffers with smaller values are likely to be returned rejected by the MVE.
     * See also comments above for afbc_alloc_bytes and
     * afbc_alloc_bytes_downscaled, they describe the situations where the
     * different values are used.
     */
    uint16_t afbc_width_in_superblocks;
    uint16_t afbc_width_in_superblocks_downscaled;

    /* For PLANAR YUV output, every plane's address need to be adjusted to get
     * optimal AXI bursts when the pixel data is written, the values below may
     * be used to calculate address offsets.
     */
    uint16_t cropx;
    uint16_t cropy;

    uint32_t mbinfo_alloc_bytes; /* Only for debugging */


    /* downscaled frame width/height for decode */
    /* ARBITRARY_DOWNSCALE */
    uint16_t dsl_frame_width;
    uint16_t dsl_frame_height;
    uint16_t dsl_pos_mode;
    uint8_t  ctu_size;  /* EXPORT_SEQ_INFO */
};

/* Decode only: This message is sent from MVE to the host so that it can
 * allocate suitable output buffers. The needed size of the buffer is sent
 * in a separate message (above).
 * When MVE sends the message below, it enters a waiting-state and will not
 * make any progress until the host sends an output-flush command, upon
 * which MVE will return all output buffers, followed by a message saying
 * that the output has been flushed. Only then should the host start
 * enqueueing new output buffers.
 */
struct mve_response_sequence_parameters
{
    /* Other stream parameters affecting buffer allocation,
     * any change in these values will trigger a flush.
     */
    uint8_t interlace;              /* 0 or 1 */
    uint8_t chroma_format;
        #define MVE_CHROMA_FORMAT_MONO (0x0)
        #define MVE_CHROMA_FORMAT_420  (0x1)
        #define MVE_CHROMA_FORMAT_422  (0x2)
        #define MVE_CHROMA_FORMAT_440  (0x3)
        #define MVE_CHROMA_FORMAT_ARGB (0x4)
    uint8_t bitdepth_luma;          /* 8, 9 or 10 */
    uint8_t bitdepth_chroma;        /* 8, 9 or 10 */
    uint8_t num_buffers_planar;     /* number of planar buffers needed */
    uint8_t num_buffers_afbc;       /* number of AFBC buffers needed, for
                                     * AFBC output more buffers are needed
                                     * (for planar output, the firmware
                                     * will allocate extra memory via RPC)
                                     */
    uint8_t range_mapping_enabled;  /* VC-1 AP specific feature, if enabled
                                     * then AFBC buffers may need special
                                     * filtering before they can be
                                     * displayed correctly. If the host is
                                     * not able to do that, then planar output
                                     * should be used, for which MVE
                                     * automatically performs the filtering.
                                     */
    uint8_t reserved0;
};

struct mve_response_ref_frame_unused
{
    /* Decode only: If requested by the host with the message
     * MVE_REQUEST_CODE_RELEASE_REF_FRAME, the MVE will respond
     * with this message when (if ever) the buffer is no longer
     * used.
     */
    uint32_t unused_buffer_address;
};


/* This message is only for debugging and performance profiling.
 * Is sent by the firmware if the corresponding options is enabled.
 */
struct mve_event_processed
{
    uint8_t  pic_format;
    uint8_t  qp;
    uint8_t  pad0;
    uint8_t  pad1;
    uint32_t parse_start_time;         /* Timestamp, absolute time */
    uint32_t parse_end_time;           /* Timestamp, absolute time */
    uint32_t parse_idle_time;          /* Definition of idle here is waiting for in/out buffers or available RAM */

    uint32_t pipe_start_time;          /* Timestamp */
    uint32_t pipe_end_time;            /* Timestamp, end-start = process time. Idle time while in a frame is
                                        * not measured. */
    uint32_t pipe_idle_time;           /* Always 0 in decode, */

    uint32_t parser_coreid;            /* Core used to parse this frame */
    uint32_t pipe_coreid;              /* Core used to pipe this frame */

    uint32_t bitstream_bits;           /* Number of bitstream bits used for this frame. */

    uint32_t intermediate_buffer_size; /* Size of intermediate (mbinfo/residuals) buffer after this frame was
                                        * parsed. */
    uint32_t total_memory_allocated;   /* after the frame was parsed. Including reference frames. */

    uint32_t bus_read_bytes;           /* bus read bytes */
    uint32_t bus_write_bytes;          /* bus written bytes */

    uint32_t afbc_bytes;               /* afbc data transferred */

    uint32_t slice0_end_time;          /* Timestamp, absolute time */
    uint32_t stream_start_time;        /* Timestamp, absolute stream start time */
    uint32_t stream_open_time;         /* Timestamp, absolute stream open time */
};

/* This message is sent by the firmware if the option
 * MVE_SET_OPT_INDEX_ENC_EXPOSE_REF_FRAMES is enabled
 */
struct mve_event_ref_frame
{
    uint32_t ref_addr;          /* MVE virtual address of AFBC reference frame */
    uint32_t ref_width;         /* Width of display area in luma pixels        */
    uint32_t ref_height;        /* Height of display area in luma pixels       */
    uint32_t ref_mb_width;      /* Width in macroblocks                        */
    uint32_t ref_mb_height;     /* Height in macroblocks                       */
    uint32_t ref_left_crop;     /* Left crop in luma pixels                    */
    uint32_t ref_top_crop;      /* Top crop in luma pixels                     */
    uint32_t ref_frame_size;    /* Total AFBC frame size in bytes              */
    uint32_t ref_display_order; /* Display picture order count                 */
    uint16_t bit_width;         /* Bit width of the YUV either 8 or 10         */
    uint16_t tiled_headers;     /* AFBC format is tiled                        */
    uint64_t user_data_tag;     /* User data tag of corresponding input buffer */
};

/* This message is only for debugging, is sent by the firmware if event tracing
 * is enabled.
 */
struct mve_event_trace_buffers
{
    uint16_t reserved;
    uint8_t num_cores;
    uint8_t rasc_mask;
    #define MVE_MAX_TRACE_BUFFERS 40
    /* this array will contain one buffer per rasc in rasc_mask per num_core */
    struct
    {
        uint32_t rasc_addr; /* rasc address of the buffer */
        uint32_t size;      /* size of the buffer in bytes */
    } buffers[MVE_MAX_TRACE_BUFFERS];
};

/* 'Events' are informative messages, the host is not required to react in
 * any particular way.
 */
struct mve_response_event
{
    uint32_t event_code;
        #define MVE_EVENT_ERROR_STREAM_CORRUPT       (1)  /* message, text string */
        #define MVE_EVENT_ERROR_STREAM_NOT_SUPPORTED (2)  /* message, text string */
        #define MVE_EVENT_PROCESSED                  (3)  /* struct mve_event_processed */
        #define MVE_EVENT_REF_FRAME                  (4)  /* struct mve_event_ref_frame */
        #define MVE_EVENT_TRACE_BUFFERS              (5)  /* struct mve_event_trace_buffers */
    union
    {
        struct mve_event_processed event_processed;
        struct mve_event_ref_frame event_ref_frame;
        struct mve_event_trace_buffers event_trace_buffers;
        char message[ MVE_MAX_ERROR_MESSAGE_SIZE ];
    } event_data;
}__attribute__((packed));
















/*********************************************************************
 *
 *   BUFFERs are sent both ways, from host to firmware and back again
 *
 *   Each MVE_BUFFER_CODE_ code is followed by one of the structs
 *   below.
 *
 *********************************************************************/

/* Flags in mve_buffer_frame::frame_flags:
 *                                     Set by whom?    Meaning:
 *                                     DECODE: ENCODE:
 * MVE_BUFFER_FRAME_FLAG_INTERLACE      host    -      Buffer is interlaced (both top and
 *                                                     bottom fields are allocated)
 * MVE_BUFFER_FRAME_FLAG_BOT_FIRST      fw      -      Bottom field should be displayed
 *                                                     first (only if interlaced)
 * MVE_BUFFER_FRAME_FLAG_TOP_PRESENT    fw      host   Top field present (or full frame if
 *                                                     not interlaced)
 * MVE_BUFFER_FRAME_FLAG_BOT_PRESENT    fw      -      Bottom present (only if interlaced)
 *
 * MVE_BUFFER_FRAME_FLAG_ROTATION_*     host    host   Decode: MVE will rotate the output frame
 *                                                             according to this setting.
 *                                                     Encode: MVE will rotate the input frame
 *                                                             according to this setting before
 *                                                             encoding them.
 * MVE_BUFFER_FRAME_FLAG_SCALING_MASK   host    -      Output pictures should be downscaled
 *
 * MVE_BUFFER_FRAME_FLAG_MIRROR_*       -       host   Input frame should be mirrored before encoding
 *
 * MVE_BUFFER_FRAME_FLAG_REJECTED       fw      -      Buffer was too small, host should re-allocate
 *
 * MVE_BUFFER_FRAME_FLAG_CORRUPT        fw      -      Frame contains visual corruption
 *
 * MVE_BUFFER_FRAME_FLAG_DECODE_ONLY    fw      -      Frame should not be displayed
 *
 * MVE_BUFFER_FRAME_FLAG_REF_FRAME      fw      -      Frame is used by MVE as reference, host must
 *                                                     not change, just re-enqueue when displayed
 * MVE_BUFFER_FRAME_FLAG_EOS            fw      host   This is the last frame in the stream.
 */

/* mve_buffer_frame_planar stores uncompressed YUV pictures.
 *       ________________________________________
 *      |  ^                          |          |  ^
 *      |<-:--visible_frame_width---->|          |  :
 *      |  :                          |          |  :
 *      |  :                          |          |  :
 *      | visible_frame_height        |          | max_frame_height
 *      |  :                          |          |  :
 *      |  :                          |          |  :
 *      |__v__________________________|          |  :
 *      |                                        |  :
 *      |<-------------max_frame_width---------->|  :
 *      |________________________________________|  v
 *
 */
struct mve_buffer_frame_planar
{
    /* Y,Cb,Cr top field */
    uint32_t plane_top[ 3 ];

    /* Y,Cb,Cr bottom field (interlace only) */
    uint32_t plane_bot[ 3 ];

    /* Stride between rows, in bytes */
    int32_t stride[ 3 ];

    /* Size of largest frame allowed to put in this buffer */
    uint16_t max_frame_width;
    uint16_t max_frame_height;
};

/* mve_buffer_frame_afbc stores AFBC compressed content that is also used
 * as the reference frame. Out of loop processing (crop, rotation,
 * range reduction) must be supported by the user of this buffer and
 * the parameters are signaled within the buffer descriptor below.
 *       ________________________________________
 *      |         ^                              |
 *      |       cropy                            |
 *      |         v_____________________________ |
 *      |<-cropx->|  ^                          ||
 *      |         |<-:--visible_frame_width---->||
 *      |         |  :                          ||
 *      |         |  :                          ||
 *      |         | visible_frame_height        ||
 *      |         |  :                          ||
 *      |         |  :                          ||
 *      |         |__v__________________________||
 *      |________________________________________|
 *
 *       <----- superblock_width --------------->
 *              * afbc_width_in_superblocks
 *
 * Note that the sizes and cropping values need not be multiples of 16.
 *
 * For interlaced streams, the values refer to a full frame,
 * while the output is actually separated into fields. Thus for fields,
 * cropy and visible_frame_height should be divided by two.
 *
 * For dual-downscaled AFBC output (not supported for interlace),
 * then the cropx, cropy, visible_frame_width and visible_frame_height
 * should be divided by two for the downscaled plane.
 */
struct mve_buffer_frame_afbc
{
    uint32_t plane[ 2 ];                     /* Addresses for up to two AFBC planes:
                                              * Top and bottom fields for interlace,
                                              * or standard and optional downscaled output. */
    uint32_t alloc_bytes[ 2 ];               /* Size of allocation for each plane */
    uint16_t cropx;                          /* Luma x crop */
    uint16_t cropy;                          /* Luma y crop */
    uint16_t afbc_width_in_superblocks[ 2 ]; /* Width of AFBC frame buffer, in units
                                              * of superblock width (32 or 16).
                                              * If dual-downscaled output is chosen,
                                              * this width can be different for the
                                              * two planes.
                                              * For first plane:
                                              * (cropx + frame_width)
                                              *      <= superblock_width  * afbc_width...
                                              */
    uint32_t afbc_params;                    /* AFBC parameters */
        #define MVE_BUFFER_FRAME_AFBC_TILED_BODY        (0x00000001) /* Output body blocks should be tiled */
        #define MVE_BUFFER_FRAME_AFBC_TILED_HEADER      (0x00000002) /* Output headers should be tiled */
        #define MVE_BUFFER_FRAME_AFBC_32X8_SUPERBLOCK   (0x00000004) /* Super block is 32x8, default is 16x16,
                                                                      * (only supported as input for encode) */
        #define MVE_BUFFER_FRAME_AFBC_DN_FORCE_8BIT     (0x00000008) /* For downscaled AFBC plane: It shall
                                                                      * be 8-bit, even if full-scale is 10-bit */
        #define MVE_BUFFER_FRAME_AFBC_DN_FORCE_420      (0x00000010) /* For downscaled AFBC plane: It shall
                                                                      * be 4:2:0, even if full-scale is 4:2:2 */
        #define MVE_BUFFER_FRAME_AFBC_STRIDE_SET_BY_MVE (0x00000020) /* Decode only: By default, the host should
                                                                        set the afbc_width_in_superblocks. If the
                                                                        value is zero, or if this bit is set, then
                                                                        the MVE sets an appropriate value. */
        #define MVE_BUFFER_FRAME_AFBC_BLOCK_SPLIT       (0x00000040) /* For Superblock layout, block_split mode
                                                                        should be enabled*/
};

/*
 * The FRAME buffer stores the common information for PLANAR and AFBC buffers,
 * and a union of PLANAR and AFBC specific information.
 */
struct mve_buffer_frame
{
    /* For identification of the buffer, this is not changed by
     * the firmware. */
    uint64_t host_handle;

    /* For matching input buffer with output buffers, the firmware
     * copies these values between frame buffers and bitstream buffers. */
    uint64_t user_data_tag;

    /* Frame buffer flags, see commentary above */
    uint32_t frame_flags;
        #define MVE_BUFFER_FRAME_FLAG_INTERLACE          (0x00000001)
        #define MVE_BUFFER_FRAME_FLAG_BOT_FIRST          (0x00000002)
        #define MVE_BUFFER_FRAME_FLAG_TOP_PRESENT        (0x00000004)
        #define MVE_BUFFER_FRAME_FLAG_BOT_PRESENT        (0x00000008)
        #define MVE_BUFFER_FRAME_FLAG_ROTATION_90        (0x00000010)
        #define MVE_BUFFER_FRAME_FLAG_ROTATION_180       (0x00000020)
        #define MVE_BUFFER_FRAME_FLAG_ROTATION_270       (0x00000030)
        #define MVE_BUFFER_FRAME_FLAG_SCALING_MASK       (0x000000C0)
        #define MVE_BUFFER_FRAME_FLAG_MIRROR_HORI        (0x00000100)
        #define MVE_BUFFER_FRAME_FLAG_MIRROR_VERT        (0x00000200)
        #define MVE_BUFFER_FRAME_FLAG_REJECTED           (0x00001000)
        #define MVE_BUFFER_FRAME_FLAG_CORRUPT            (0x00002000)
        #define MVE_BUFFER_FRAME_FLAG_DECODE_ONLY        (0x00004000)
        #define MVE_BUFFER_FRAME_FLAG_REF_FRAME          (0x00008000)
        #define MVE_BUFFER_FRAME_FLAG_EOS                (0x00010000)
        /*ARBITRARY_DOWNSCALE*/
        #define MVE_BUFFER_FRAME_FLAG_SCALING_MASKX      (0xFF000000) //8bit
        #define MVE_BUFFER_FRAME_FLAG_SCALING_MASKY      (0x00FE0000) //7bit

    /* Height (in luma samples) of visible part of frame,
     * may be smaller than allocated frame size. */
    uint16_t visible_frame_height;

    /* Width (in luma samples) of visible part of frame,
     * may be smaller than allocated frame size. */
    uint16_t visible_frame_width;

    /* Color format of buffer */
    uint16_t format;
        /* format bitfield: */
        #define MVE_FORMAT_BF_C  (0) /* 3 bits, chroma subsampling   */
        #define MVE_FORMAT_BF_B  (4) /* 4 bits, max bitdepth minus 8 */
        #define MVE_FORMAT_BF_N  (8) /* 2 bits, number of planes     */
        #define MVE_FORMAT_BF_V (12) /* 2 bits, format variant       */
        #define MVE_FORMAT_BF_A (15) /* 1 bit,  AFBC bit             */
        /* formats: */
        #define MVE_FORMAT_YUV420_AFBC_8  ( (MVE_CHROMA_FORMAT_420  << MVE_FORMAT_BF_C) | \
                                            (              ( 8 - 8) << MVE_FORMAT_BF_B) | \
                                            (                     1 << MVE_FORMAT_BF_A) )

        #define MVE_FORMAT_YUV420_AFBC_10 ( (MVE_CHROMA_FORMAT_420  << MVE_FORMAT_BF_C) | \
                                            (              (10 - 8) << MVE_FORMAT_BF_B) | \
                                            (                     1 << MVE_FORMAT_BF_A) )

        #define MVE_FORMAT_YUV422_AFBC_8  ( (MVE_CHROMA_FORMAT_422  << MVE_FORMAT_BF_C) | \
                                            (              ( 8 - 8) << MVE_FORMAT_BF_B) | \
                                            (                     1 << MVE_FORMAT_BF_A) )

        #define MVE_FORMAT_YUV422_AFBC_10 ( (MVE_CHROMA_FORMAT_422  << MVE_FORMAT_BF_C) | \
                                            (              (10 - 8) << MVE_FORMAT_BF_B) | \
                                            (                     1 << MVE_FORMAT_BF_A) )

        #define MVE_FORMAT_YUV420_I420    ( (MVE_CHROMA_FORMAT_420  << MVE_FORMAT_BF_C) | \
                                            (              ( 8 - 8) << MVE_FORMAT_BF_B) | \
                                            (                     3 << MVE_FORMAT_BF_N) | \
                                            (                     0 << MVE_FORMAT_BF_V) )

        #define MVE_FORMAT_YUV420_NV12    ( (MVE_CHROMA_FORMAT_420  << MVE_FORMAT_BF_C) | \
                                            (              ( 8 - 8) << MVE_FORMAT_BF_B) | \
                                            (                     2 << MVE_FORMAT_BF_N) | \
                                            (                     0 << MVE_FORMAT_BF_V) )

        #define MVE_FORMAT_YUV420_NV21    ( (MVE_CHROMA_FORMAT_420  << MVE_FORMAT_BF_C) | \
                                            (              ( 8 - 8) << MVE_FORMAT_BF_B) | \
                                            (                     2 << MVE_FORMAT_BF_N) | \
                                            (                     1 << MVE_FORMAT_BF_V) )

        #define MVE_FORMAT_YUV420_P010    ( (MVE_CHROMA_FORMAT_420  << MVE_FORMAT_BF_C) | \
                                            (              (16 - 8) << MVE_FORMAT_BF_B) | \
                                            (                     2 << MVE_FORMAT_BF_N) | \
                                            (                     0 << MVE_FORMAT_BF_V) )

        #define MVE_FORMAT_YUV420_Y0L2    ( (MVE_CHROMA_FORMAT_420  << MVE_FORMAT_BF_C) | \
                                            (              (10 - 8) << MVE_FORMAT_BF_B) | \
                                            (                     1 << MVE_FORMAT_BF_N) | \
                                            (                     0 << MVE_FORMAT_BF_V) )

        #define MVE_FORMAT_YUV420_AQB1    ( (MVE_CHROMA_FORMAT_420  << MVE_FORMAT_BF_C) | \
                                            (              (10 - 8) << MVE_FORMAT_BF_B) | \
                                            (                     1 << MVE_FORMAT_BF_N) | \
                                            (                     1 << MVE_FORMAT_BF_V) )

        #define MVE_FORMAT_YUV422_YUY2    ( (MVE_CHROMA_FORMAT_422  << MVE_FORMAT_BF_C) | \
                                            (              ( 8 - 8) << MVE_FORMAT_BF_B) | \
                                            (                     1 << MVE_FORMAT_BF_N) | \
                                            (                     0 << MVE_FORMAT_BF_V) )

        #define MVE_FORMAT_YUV422_UYVY    ( (MVE_CHROMA_FORMAT_422  << MVE_FORMAT_BF_C) | \
                                            (              ( 8 - 8) << MVE_FORMAT_BF_B) | \
                                            (                     1 << MVE_FORMAT_BF_N) | \
                                            (                     1 << MVE_FORMAT_BF_V) )

        #define MVE_FORMAT_YUV422_Y210    ( (MVE_CHROMA_FORMAT_422  << MVE_FORMAT_BF_C) | \
                                            (              (16 - 8) << MVE_FORMAT_BF_B) | \
                                            (                     1 << MVE_FORMAT_BF_N) | \
                                            (                     0 << MVE_FORMAT_BF_V) )

        #define MVE_FORMAT_RGBA_8888      ( (MVE_CHROMA_FORMAT_ARGB << MVE_FORMAT_BF_C) | \
                                            (              ( 8 - 8) << MVE_FORMAT_BF_B) | \
                                            (                     1 << MVE_FORMAT_BF_N) | \
                                            (                     0 << MVE_FORMAT_BF_V) )

        #define MVE_FORMAT_BGRA_8888      ( (MVE_CHROMA_FORMAT_ARGB << MVE_FORMAT_BF_C) | \
                                            (              ( 8 - 8) << MVE_FORMAT_BF_B) | \
                                            (                     1 << MVE_FORMAT_BF_N) | \
                                            (                     1 << MVE_FORMAT_BF_V) )

        #define MVE_FORMAT_ARGB_8888      ( (MVE_CHROMA_FORMAT_ARGB << MVE_FORMAT_BF_C) | \
                                            (              ( 8 - 8) << MVE_FORMAT_BF_B) | \
                                            (                     1 << MVE_FORMAT_BF_N) | \
                                            (                     2 << MVE_FORMAT_BF_V) )

        #define MVE_FORMAT_ABGR_8888      ( (MVE_CHROMA_FORMAT_ARGB << MVE_FORMAT_BF_C) | \
                                            (              ( 8 - 8) << MVE_FORMAT_BF_B) | \
                                            (                     1 << MVE_FORMAT_BF_N) | \
                                            (                     3 << MVE_FORMAT_BF_V) )

        #define MVE_FORMAT_MBINFO         (0x0001) /* only used for debugging */

        #define MVE_FORMAT_UNUSED         (0x0000)

    uint16_t reserved0; /* force 'data' to be 4-byte aligned */

    union
    {
        struct mve_buffer_frame_planar planar;
        struct mve_buffer_frame_afbc   afbc;
    } data;

    uint32_t reserved1; /* force size to be multiple of 8 bytes */
};

/* The bitstream buffer stores a number of bitstream bytes */
struct mve_buffer_bitstream
{
    /* For identification of the buffer, this is not changed by
     * the firmware. */
    uint64_t host_handle;

    /* For matching input buffer with output buffers, the firmware
     * copies these values between frame buffers and bitstream buffers. */
    uint64_t user_data_tag;

    /* BufferFlags */
    uint32_t bitstream_flags;
        #define MVE_BUFFER_BITSTREAM_FLAG_EOS              (0x00000001)
        #define MVE_BUFFER_BITSTREAM_FLAG_ENDOFFRAME       (0x00000010)
        #define MVE_BUFFER_BITSTREAM_FLAG_SYNCFRAME        (0x00000020)
        #define MVE_BUFFER_BITSTREAM_FLAG_CODECCONFIG      (0x00000080)
        #define MVE_BUFFER_BITSTREAM_FLAG_ENDOFSUBFRAME    (0x00000400)

    /* Length of allocated buffer */
    uint32_t bitstream_alloc_bytes;

    /* Byte offset from start to first byte */
    uint32_t bitstream_offset;

    /* Number of bytes in the buffer */
    uint32_t bitstream_filled_len;

    /* Pointer to buffer start */
    uint32_t bitstream_buf_addr;

    /* frame_type. 0:I, 1:P, 2:B, 3:b */
    uint8_t frame_type;

    /* Pad to force 8-byte alignment */
    //uint32_t reserved;
    uint8_t reserved[3];
};

/*
 * Define a region in 16x16 units
 *
 * The region is macroblock positions (x,y) in the range
 * mbx_left <= x < mbx_right
 * mby_top  <= y < mby_bottom
 */
struct mve_buffer_param_region
{
    uint16_t mbx_left;   /* macroblock x left edge   (inclusive) */
    uint16_t mbx_right;  /* macroblock x right edge  (exclusive) */
    uint16_t mby_top;    /* macroblock y top edge    (inclusive) */
    uint16_t mby_bottom; /* macroblock y bottom edge (exclusive) */
    int16_t  qp_delta;   /* QP delta value for this region, this
                          * delta applies to QP values in the ranges:
                          *   H264: 0-51
                          *   HEVC: 0-51
                          *   VP9:  0-255 */
    uint16_t reserved;
};

/* input for encoder,
 * the mve_buffer_param_regions buffer stores the information for FRAME buffers,
 * and the information for regions of interest.
 */
struct mve_buffer_param_regions
{
    uint8_t n_regions;   /* Number of regions */
    uint8_t reserved[ 3 ];
    #define MVE_MAX_FRAME_REGIONS 16
    struct mve_buffer_param_region region[ MVE_MAX_FRAME_REGIONS ];
};

/* the block parameter record specifies the various properties of a quad */
struct mve_block_param_record
{
    uint16_t qp_delta;
        /* Bitset of four 4-bit QP delta values for a quad.
         * For H.264 and HEVC these are qp delta values in the range -8 to +7.
         * For Vp9 these are segment map values in the range 0 to 7.
         */
        #define MVE_BLOCK_PARAM_RECORD_QP_DELTA_TOP_LEFT_16X16     (0)
        #define MVE_BLOCK_PARAM_RECORD_QP_DELTA_TOP_LEFT_16X16_SZ  (4)
        #define MVE_BLOCK_PARAM_RECORD_QP_DELTA_TOP_RIGHT_16X16    (4)
        #define MVE_BLOCK_PARAM_RECORD_QP_DELTA_TOP_RIGHT_16X16_SZ (4)
        #define MVE_BLOCK_PARAM_RECORD_QP_DELTA_BOT_LEFT_16X16     (8)
        #define MVE_BLOCK_PARAM_RECORD_QP_DELTA_BOT_LEFT_16X16_SZ  (4)
        #define MVE_BLOCK_PARAM_RECORD_QP_DELTA_BOT_RIGHT_16X16    (12)
        #define MVE_BLOCK_PARAM_RECORD_QP_DELTA_BOT_RIGHT_16X16_SZ (4)

        #define MVE_BLOCK_PARAM_RECORD_VP9_SEGID_TOP_LEFT_16X16     (0)
        #define MVE_BLOCK_PARAM_RECORD_VP9_SEGID_TOP_LEFT_16X16_SZ  (3)
        #define MVE_BLOCK_PARAM_RECORD_VP9_SEGID_TOP_RIGHT_16X16    (4)
        #define MVE_BLOCK_PARAM_RECORD_VP9_SEGID_TOP_RIGHT_16X16_SZ (3)
        #define MVE_BLOCK_PARAM_RECORD_VP9_SEGID_BOT_LEFT_16X16     (8)
        #define MVE_BLOCK_PARAM_RECORD_VP9_SEGID_BOT_LEFT_16X16_SZ  (3)
        #define MVE_BLOCK_PARAM_RECORD_VP9_SEGID_BOT_RIGHT_16X16    (12)
        #define MVE_BLOCK_PARAM_RECORD_VP9_SEGID_BOT_RIGHT_16X16_SZ (3)

    uint8_t force;
        #define MVE_BLOCK_PARAM_RECORD_FORCE_NONE  (0x00)
        #define MVE_BLOCK_PARAM_RECORD_FORCE_QP    (0x01)
        #define MVE_BLOCK_PARAM_RECORD_FORCE_32X32 (0x02)
        #define MVE_BLOCK_PARAM_RECORD_FORCE_RB    (0x04)

    uint8_t reserved;
};

/* block configuration uncompressed rows header. this configures the size of the
 * uncompressed body. */
struct mve_buffer_general_rows_uncomp_hdr
{
    uint8_t n_cols_minus1; /* number of quad cols in picture minus 1 */
    uint8_t n_rows_minus1; /* number of quad rows in picture minus 1 */
    uint8_t reserved[2];
};

/* block configuration uncompressed rows body. this structure contains an array
 * of block parameter records whose length is (n_cols_minus1 + 1) * (n_rows_minus1 + 1)
 * elements. therefore the allocation of this structure needs to be dynamic and
 * a pointer to the allocated memory should then be assigned to the general
 * purpose buffer data pointer
 */
struct mve_buffer_general_rows_uncomp_body
{
    /* the size of this array is variable and not necessarily equal to 1.
     * therefore the sizeof operator should not be used
     */
    struct mve_block_param_record bpr[1];
};

/* input for encoder, block level configurations.
 * the row based block configurations can be defined in different formats. they
 * are stored in the blk_cfgs union and identified by the blk_cfg_type member.
 * these definitions consist of a header and body pair. the header part contains
 * configuration information for the body. the body part describes the actual
 * layout of the data buffer pointed to by the mve_buffer_general_hdr buffer_ptr.
 */
struct mve_buffer_general_block_configs
{
    uint8_t blk_cfg_type;
        #define MVE_BLOCK_CONFIGS_TYPE_NONE       (0x00)
        #define MVE_BLOCK_CONFIGS_TYPE_ROW_UNCOMP (0xff)
    uint8_t reserved[3];
    union
    {
        struct mve_buffer_general_rows_uncomp_hdr rows_uncomp;
    } blk_cfgs;
};

/* for the mve_buffer_general_encoder_stats buffer type the body data
 * is an array of records of the following format, one record for each
 * 32x32 pixel block of the picture. Values that are marked "per CTU"
 * are only valid in the first 32x32 block of the 64x64 CTU for HEVC or VP9.
 * For H.264 the "per CTU" values are the sum of four 16x16 macroblocks.
 * Each record is 12 bytes and must be 32-bit aligned.
 */
struct mve_block_stats_record_full
{
    uint8_t     intra_count;    // number of 8x8 blocks in the CTU that are intra
    uint8_t     reserved0;      // not used
    uint16_t    bit_estimate;   // bit estimate for the CTU
    uint16_t    luma_mean;      // luminance mean
    uint16_t    luma_cplx;      // luma complexity measure (0=flat)
    int16_t     rmv_x;          // rough x motion vector in pixels
    int16_t     rmv_y;          // rough y motion vector in pixels
};

/* input for encoder, block level statistics
 * this buffer is added to the encoder input frame queue, before the
 * input frame for which statistics are to be gathered. The buffer is
 * filled duing frame analysis and returned in the input buffer return
 * queue before the input buffer is returned.
 */
struct mve_buffer_general_encoder_stats
{
    uint8_t encoder_stats_type;
        #define MVE_ENCODER_STATS_TYPE_FULL       (0x01)
    uint8_t frame_type;         // See MVE_FRAME_TYPE_*
        #define MVE_FRAME_TYPE_I 0
        #define MVE_FRAME_TYPE_P 1
        #define MVE_FRAME_TYPE_B 2
    uint8_t used_as_reference;  // 0=No, 1=Yes
    uint8_t qp;                 // base quantizer used for the frame
                                // HEVC, H.264: 0-51. VP9: 0-63
    uint32_t picture_count;     // display order picture count
    uint16_t num_cols;          // number of columns (each 32 pixels wide)
    uint16_t num_rows;          // number of rows    (each 32 pixels high)
    uint32_t ref_pic_count[2];  // display order picture count of references
                                // unused values are set to zero
};

/* input for encoder */
struct mve_buffer_param_qp
{
    /* QP (quantization parameter) for encode.
     *
     * When used to set fixed QP for encode, with rate control
     * disabled, then the valid ranges are:
     *   H264: 0-51
     *   HEVC: 0-51
     *   VP8:  0-63
     *   VP9:  0-63
     * Note: The QP must be set separately for I, P and B frames.
     *
     * But when this message is used with the regions-feature,
     * then the valid ranges are the internal bitstream ranges:
     *   H264: 0-51
     *   HEVC: 0-51
     *   VP8:  0-127
     *   VP9:  0-255
     */
    int32_t qp;
};

/* output from decoder */
struct mve_buffer_param_display_size
{
    uint16_t display_width;
    uint16_t display_height;
};

/* output from decoder, colour information needed for hdr */
struct mve_buffer_param_colour_description
{
    uint32_t flags;
        #define MVE_BUFFER_PARAM_COLOUR_FLAG_MASTERING_DISPLAY_DATA_VALID  (1)
        #define MVE_BUFFER_PARAM_COLOUR_FLAG_CONTENT_LIGHT_DATA_VALID      (2)

    uint8_t range;                             /* Unspecified=0, Limited=1, Full=2 */
        #define MVE_BUFFER_PARAM_COLOUR_RANGE_UNSPECIFIED  (0)
        #define MVE_BUFFER_PARAM_COLOUR_RANGE_LIMITED      (1)
        #define MVE_BUFFER_PARAM_COLOUR_RANGE_FULL         (2)

    uint8_t colour_primaries;                  /* see hevc spec. E.3.1 */
    uint8_t transfer_characteristics;          /* see hevc spec. E.3.1 */
    uint8_t matrix_coeff;                      /* see hevc spec. E.3.1 */

    uint16_t mastering_display_primaries_x[3]; /* see hevc spec. D.3.27 */
    uint16_t mastering_display_primaries_y[3]; /* see hevc spec. D.3.27 */
    uint16_t mastering_white_point_x;          /* see hevc spec. D.3.27 */
    uint16_t mastering_white_point_y;          /* see hevc spec. D.3.27 */
    uint32_t max_display_mastering_luminance;  /* see hevc spec. D.3.27 */
    uint32_t min_display_mastering_luminance;  /* see hevc spec. D.3.27 */

    uint32_t max_content_light_level;          /* see hevc spec. D.3.35 */
    uint32_t avg_content_light_level;          /* see hevc spec. D.3.35 */

    uint8_t video_format_present_flag;
    uint8_t video_format;
    uint8_t aspect_ratio_info_present_flag;
    uint8_t aspect_ratio_idc;
    uint8_t timing_flag_info_present_flag;
    uint16_t sar_width;
    uint16_t sar_height;
    uint32_t num_units_in_tick;
    uint32_t time_scale;

    uint32_t reserved[2];
};

struct mve_buffer_param_sei_user_data_unregistered
{
    uint8_t flags;
        #define MVE_BUFFER_PARAM_USER_DATA_UNREGISTERED_VALID  (1)
    uint8_t uuid[16];
    char user_data[256 - 35];
    uint8_t user_data_len;

    uint8_t reserved[5];
};

/* output from decoder see hevc spec. D.3.3 */
struct mve_buffer_param_frame_field_info
{
    uint8_t pic_struct;
    uint8_t source_scan_type;
    uint8_t duplicate_flag;
    uint8_t reserved;
};

/* output from decoder, VC-1 specific feature only relevant
 * if using AFBC output
 */
struct mve_buffer_param_range_map
{
    uint8_t luma_map_enabled;
    uint8_t luma_map_value;
    uint8_t chroma_map_enabled;
    uint8_t chroma_map_value;
};

/* input for encoder */
struct mve_buffer_param_rate_control
{
    uint32_t rate_control_mode;
        #define MVE_OPT_RATE_CONTROL_MODE_OFF               (0)
        #define MVE_OPT_RATE_CONTROL_MODE_STANDARD          (1)
        #define MVE_OPT_RATE_CONTROL_MODE_VARIABLE          (2)
        #define MVE_OPT_RATE_CONTROL_MODE_CONSTANT          (3)
        #define MVE_OPT_RATE_CONTROL_MODE_C_VARIABLE        (4)
    uint32_t target_bitrate; /* in bits per second */
    uint32_t maximum_bitrate; /* in bits per second */
};

/* input for encoder */
struct mve_buffer_param_rate_control_qp_range
{
    int32_t  qp_min;
    int32_t  qp_max;
};

/* input for encoder, see hevc spec. D.3.16 */
struct mve_buffer_param_frame_packing
{
    uint32_t flags;
        #define MVE_BUFFER_PARAM_FRAME_PACKING_FLAG_QUINCUNX_SAMPLING       (1)
        #define MVE_BUFFER_PARAM_FRAME_PACKING_FLAG_SPATIAL_FLIPPING        (2)
        #define MVE_BUFFER_PARAM_FRAME_PACKING_FLAG_FRAME0_FLIPPED          (4)
        #define MVE_BUFFER_PARAM_FRAME_PACKING_FLAG_FIELD_VIEWS             (8)
        #define MVE_BUFFER_PARAM_FRAME_PACKING_FLAG_CURRENT_FRAME_IS_FRAME0 (16)

    uint8_t frame_packing_arrangement_type;
    uint8_t content_interpretation_type;

    uint8_t frame0_grid_position_x;
    uint8_t frame0_grid_position_y;
    uint8_t frame1_grid_position_x;
    uint8_t frame1_grid_position_y;

    uint8_t reserved[ 2 ];
};

struct mve_buffer_param_rectangle
{
    uint16_t x_left;   /* pixel x left edge   (inclusive) */
    uint16_t x_right;  /* pixel x right edge  (exclusive) */
    uint16_t y_top;    /* pixel y top edge    (inclusive) */
    uint16_t y_bottom; /* pixel y bottom edge (exclusive) */
};

/* input for encoder,
 * indicate which parts of the source picture has changed.
 * The encoder can (optionally) use this information to
 * reduce memory bandwidth.
 *
 * n_rectangles=0 indicates the source picture is unchanged.
 *
 * This parameter only applies to the picture that immediately
 * follows (and not to subsequent ones).
 */
struct mve_buffer_param_change_rectangles
{
    uint8_t n_rectangles;   /* Number of rectangles */
    uint8_t reserved[3];
    #define MVE_MAX_FRAME_CHANGE_RECTANGLES 2
    struct mve_buffer_param_rectangle rectangles[MVE_MAX_FRAME_CHANGE_RECTANGLES];
};

/* input for VP9 encoder,
 * specify the qp deltas for each segment map index.
 * These are intended to be used with block configs only.
 */
struct mve_buffer_param_vp9_segmap
{
    #define VP9SPEC_MAX_SEGMENTS 8
    int8_t qp_delta[VP9SPEC_MAX_SEGMENTS];     /* Qp delta to use for each segment map region */
    int8_t num_segments;    /* Number of active segments (to set coding probability) */
};

/* Parameters that are sent in the same communication channels
 * as the buffers. A parameter applies to all subsequent buffers.
 * Some types are only valid for decode, and some only for encode.
 */
struct mve_buffer_param
{
    uint32_t type;                                                   /* Extra data: */
        #define MVE_BUFFER_PARAM_TYPE_QP                        (2)  /* qp */
        #define MVE_BUFFER_PARAM_TYPE_REGIONS                   (3)  /* regions */
        #define MVE_BUFFER_PARAM_TYPE_DISPLAY_SIZE              (5)  /* display_size */
        #define MVE_BUFFER_PARAM_TYPE_RANGE_MAP                 (6)  /* range_map */
        #define MVE_BUFFER_PARAM_TYPE_FRAME_RATE                (9)  /* arg, in frames per second, as a
                                                                      * fixed point Q16 value, for example
                                                                      * 0x001e0000 == 30.0 fps */
        #define MVE_BUFFER_PARAM_TYPE_RATE_CONTROL              (10) /* rate_control */
        #define MVE_BUFFER_PARAM_TYPE_QP_I                      (12) /* qp for I frames, when no rate control */
        #define MVE_BUFFER_PARAM_TYPE_QP_P                      (13) /* qp for P frames, when no rate control */
        #define MVE_BUFFER_PARAM_TYPE_QP_B                      (14) /* qp for B frames, when no rate control */
        #define MVE_BUFFER_PARAM_TYPE_COLOUR_DESCRIPTION        (15) /* colour_description */
        #define MVE_BUFFER_PARAM_TYPE_FRAME_PACKING             (16) /* frame_packing */
        #define MVE_BUFFER_PARAM_TYPE_FRAME_FIELD_INFO          (17) /* frame_field_info */
        #define MVE_BUFFER_PARAM_TYPE_GOP_RESET                 (18) /* no extra data */
        #define MVE_BUFFER_PARAM_TYPE_DPB_HELD_FRAMES           (19) /* arg, number of output buffers that are
                                                                      * complete and held by firmware in the
                                                                      * DPB for reordering purposes.
                                                                      * Valid after the next frame is output */
        #define MVE_BUFFER_PARAM_TYPE_CHANGE_RECTANGLES         (20) /* change rectangles */
        #define MVE_BUFFER_PARAM_TYPE_RATE_CONTROL_QP_RANGE     (21) /* rate_control_qp_range */
        #define MVE_BUFFER_PARAM_TYPE_RATE_CONTROL_HRD_BUF_SIZE (23) /* arg */
        #define MVE_BUFFER_PARAM_TYPE_VP9_SEGMAP                (24) /* VP9 segment map settings */
        #define MVE_BUFFER_PARAM_TYPE_RATE_CONTROL_QP_RANGE_I   (25) /* special range for I frames,
                                                                      * rate_control_qp_range */
        #define MVE_BUFFER_PARAM_TYPE_SEI_USER_DATA_UNREGISTERED (26) /* sei user_data_unregistered */

    union
    {
        uint32_t                                        arg; /* some parameters only need a uint32_t as argument */
        struct mve_buffer_param_qp                      qp;
        struct mve_buffer_param_regions                 regions;
        struct mve_buffer_param_display_size            display_size;
        struct mve_buffer_param_range_map               range_map;
        struct mve_buffer_param_rate_control            rate_control;
        struct mve_buffer_param_rate_control_qp_range   rate_control_qp_range;
        struct mve_buffer_param_colour_description      colour_description;
        struct mve_buffer_param_frame_packing           frame_packing;
        struct mve_buffer_param_frame_field_info        frame_field_info;
        struct mve_buffer_param_change_rectangles       change_rectangles;
        struct mve_buffer_param_vp9_segmap              vp9_segmap;
        struct mve_buffer_param_sei_user_data_unregistered user_data_unregistered;
    } data;
};


/* The general purpose buffer header stores the common fields of an
 * mve_buffer_general. it contains the pointer to the data buffer that contains
 * the general purpose data
 */
struct mve_buffer_general_hdr
{
    /* For identification of the buffer, this is not changed by the firmware. */
    uint64_t host_handle;

    /* this depends upon the type of the general purpose buffer */
    uint64_t user_data_tag;

    /* pointer to the buffer containing the general purpose data. the format
     * of this data is defined by the configuration in the mve_buffer_general */
    uint32_t buffer_ptr;

    /* size of the buffer pointed to by buffer_ptr */
    uint32_t buffer_size;

    /* selects the type of semantics to use for the general purpose buffer. it
     * tags (or discriminates) the union config member in mve_buffer_general
     */
    uint16_t type;                                        /* Extra data: */
        #define MVE_BUFFER_GENERAL_TYPE_BLOCK_CONFIGS (1) /* block_configs */
        #define MVE_BUFFER_GENERAL_TYPE_ENCODER_STATS (4) /* encoder_stats */

    /* size of the mve_buffer_general config member */
    uint16_t config_size;

    /* pad to force 8-byte alignment */
    uint32_t reserved;
};

/* The general purpose buffer consists of a header and a configuration. The
 * header contains a pointer to a buffer whose format is described by the
 * configuration. The type of configuration is indicated by the type value in
 * the header. N.B. In use, the size of the config part of this structure is
 * defined in the header and is not necessarily equal to that returned by the
 * sizeof() operator. This allows a more size efficient communication between
 * the host and firmware.
 */
struct mve_buffer_general
{
    struct mve_buffer_general_hdr header;

    /* used to describe the configuration of the general purpose buffer data
     * pointed to be buffer_ptr
     */
    union
    {
        struct mve_buffer_general_block_configs block_configs;
        struct mve_buffer_general_encoder_stats encoder_stats;
    } config;
};

#ifdef __cplusplus
}
#endif

#endif /* __FW_INCLUDE__MVE_PROTOCOL_DEF_H__ */
