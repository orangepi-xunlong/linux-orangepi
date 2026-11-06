//-----------------------------------------------------------------------------
// COPYRIGHT (C) 2020   CHIPS&MEDIA INC. ALL RIGHTS RESERVED
//
// This file is distributed under BSD 3 clause and LGPL2.1 (dual license)
// SPDX License Identifier: BSD-3-Clause
// SPDX License Identifier: LGPL-2.1-only
//
// The entire notice above must be reproduced on all authorized copies.
//
// Description  :
//-----------------------------------------------------------------------------

#ifndef VPUAPI_UTIL_H_INCLUDED
#define VPUAPI_UTIL_H_INCLUDED

#include "vpuapi.h"

// COD_STD
enum {
    AVC_DEC  = 0,
    VC1_DEC  = 1,
    MP2_DEC  = 2,
    MP4_DEC  = 3,
    DV3_DEC  = 3,
    RV_DEC   = 4,
    AVS_DEC  = 5,
    VPX_DEC  = 7,
    MAX_DEC  = 7,
    AVC_ENC  = 8,
    MP4_ENC  = 11,
    MAX_CODECS,
};

// new COD_STD since WAVE series
enum {
    W_HEVC_DEC  = 0x00,
    W_HEVC_ENC  = 0x01,
    W_AVC_DEC   = 0x02,
    W_AVC_ENC   = 0x03,
    W_VP9_DEC   = 0x16,
    W_AVS2_DEC  = 0x18,
    W_AV1_DEC   = 0x1A,
    W_AV1_ENC    = 0x1B,
    STD_UNKNOWN = 0xFF
};

// AUX_COD_STD
enum {
    MP4_AUX_MPEG4 = 0,
    MP4_AUX_DIVX3 = 1
};

enum {
    VPX_AUX_THO = 0,
    VPX_AUX_VP6 = 1,
    VPX_AUX_VP8 = 2,
    VPX_AUX_NUM
};

enum {
    AVC_AUX_AVC = 0,
    AVC_AUX_MVC = 1
};

// BIT_RUN command
enum {
    DEC_SEQ_INIT         = 1,
    ENC_SEQ_INIT         = 1,
    DEC_SEQ_END          = 2,
    ENC_SEQ_END          = 2,
    PIC_RUN              = 3,
    SET_FRAME_BUF        = 4,
    ENCODE_HEADER        = 5,
    ENC_PARA_SET         = 6,
    DEC_PARA_SET         = 7,
    DEC_BUF_FLUSH        = 8,
    ENC_CHANGE_PARAMETER = 9,
    VPU_SLEEP            = 10,
    VPU_WAKE             = 11,
    ENC_ROI_INIT         = 12,
    FIRMWARE_GET         = 0xf
};

enum {
    SRC_BUFFER_EMPTY        = 0,    //!< source buffer doesn't allocated.
    SRC_BUFFER_ALLOCATED    = 1,    //!< source buffer has been allocated.
    SRC_BUFFER_SRC_LOADED   = 2,    //!< source buffer has been allocated.
    SRC_BUFFER_USE_ENCODE   = 3     //!< source buffer was sent to VPU. but it was not used for encoding.
};

enum {
    FramebufCacheNone,
    FramebufCacheMaverickI,
    FramebufCacheMaverickII,
};

#ifndef MAX
#define MAX(a, b)       (((a) > (b)) ? (a) : (b))
#endif

#ifndef MIN
#define MIN(a,b)        (((a) < (b)) ? (a) : (b))
#endif

#define HEVC_MAX_SUB_LAYER_ID   6
#define AVS2_MAX_SUB_LAYER_ID   7
#define DECODE_ALL_TEMPORAL_LAYERS  -1
#define DECODE_ALL_SPATIAL_LAYERS   -1

//#define API_DEBUG
#ifdef API_DEBUG
#ifdef _MSC_VER
#define APIDPRINT(_fmt, ...)            printf(_fmt, __VA_ARGS__)
#else
#define APIDPRINT(_fmt, ...)            printf(_fmt, ##__VA_ARGS__)
#endif
#else
#define APIDPRINT(_fmt, ...)
#endif

extern Uint32 __VPU_BUSY_TIMEOUT;
extern VpuAttr g_VpuCoreAttributes[MAX_NUM_VPU_CORE];

/**
 * PRODUCT: CODA9/WAVE5/WAVE6
 */
typedef struct {
    union {
        struct {
            int             useBitEnable;
            int             useIpEnable;
            int             useDbkYEnable;
            int             useDbkCEnable;
            int             useOvlEnable;
            int             useBtpEnable;
            PhysicalAddress bufBitUse;
            PhysicalAddress bufIpAcDcUse;
            PhysicalAddress bufDbkYUse;
            PhysicalAddress bufDbkCUse;
            PhysicalAddress bufOvlUse;
            PhysicalAddress bufBtpUse;
        } coda9;
        struct {
            //Decoder
            int             useBitEnable;
            int             useIpEnable;
            int             useLfRowEnable;
            //Encoder
            int             useEncRdoEnable;
            int             useEncLfEnable;

        } wave;
    } u;
    int             bufSize;
    PhysicalAddress bufBase;
} SecAxiInfo;

typedef struct _tag_FramebufferIndex {
    Int16 tiledIndex;
    Int16 linearIndex;
} FramebufferIndex;

typedef struct
{
    osal_file_t     fpPicDispInfoLogfile;
    osal_file_t     fpPicTypeLogfile;
    osal_file_t     fpSeqDispInfoLogfile;
    osal_file_t     fpUserDataLogfile;
    osal_file_t     fpSeqUserDataLogfile;

    // encoder report file
    osal_file_t     fpEncSliceBndInfo;
    osal_file_t     fpEncQpInfo;
    osal_file_t     fpEncmvInfo;
    osal_file_t     fpEncsliceInfo;

    // Report Information
    BOOL            reportOpened;
    Int32           decIndex;
    vpu_buffer_t    vb_rpt;
    BOOL            userDataEnable;
    BOOL            userDataReportMode;

    Int32           profile;
    Int32           level;
} vpu_rpt_info_t;

typedef struct {
    DecOpenParam    openParam;
    DecInitialInfo  initialInfo;
    DecInitialInfo  newSeqInfo;     /* Temporal new sequence information */
    PhysicalAddress streamWrPtr;
    PhysicalAddress streamRdPtr;
    int             streamEndflag;
    int             frameDisplayFlag;
    int             clearDisplayIndexes;
    int             setDisplayIndexes;
    PhysicalAddress streamRdPtrRegAddr;
    PhysicalAddress streamWrPtrRegAddr;
    PhysicalAddress streamBufStartAddr;
    PhysicalAddress streamBufEndAddr;
    PhysicalAddress frameDisplayFlagRegAddr;
    PhysicalAddress currentPC;
    PhysicalAddress busyFlagAddr;
    int             streamBufSize;
    FrameBuffer     frameBufPool[MAX_REG_FRAME];
    vpu_buffer_t    vbFrame;
    vpu_buffer_t    vbWTL;
    vpu_buffer_t    vbPPU;
    vpu_buffer_t    vbMV[MAX_REG_FRAME];
    vpu_buffer_t    vbFbcYTbl[MAX_REG_FRAME];
    vpu_buffer_t    vbFbcCTbl[MAX_REG_FRAME];
    vpu_buffer_t    vbDefCdf;
    vpu_buffer_t    vbSegMap;
    int             frameAllocExt;
    int             ppuAllocExt;
    int             numFrameBuffers;
    int             numFbsForDecoding;                  /*!<< number of framebuffers used in decoding */
    int             numFbsForWTL;                       /*!<< number of linear framebuffer for displaying when DecInfo::wtlEnable is set to 1 */
    int             stride;
    int             frameBufferHeight;
    int             rotationEnable;
    int             mirrorEnable;
    int             deringEnable;
    MirrorDirection mirrorDirection;
    int             rotationAngle;
    FrameBuffer     rotatorOutput;
    int             rotatorStride;
    int             rotatorOutputValid;
    int             initialInfoObtained;
    int             vc1BframeDisplayValid;
    int             mapType;
    int             tiled2LinearEnable;
    int             tiled2LinearMode;
    int             wtlEnable;
    int             wtlMode;
    FrameBufferFormat   wtlFormat;                      /*!<< default value: FORMAT_420 8bit */
    SecAxiInfo          secAxiInfo;
    MaverickCacheConfig cacheConfig;
    int seqInitEscape;

    // Report Information
    PhysicalAddress userDataBufAddr;
    vpu_buffer_t    vbUserData;

    Uint32          userDataEnable;                    /* User Data Enable Flag
                                                          CODA9xx: TRUE or FALSE
                                                          WAVExxx: Refer to H265_USERDATA_FLAG_xxx values in vpuapi.h */
    int             userDataBufSize;
    int             userDataReportMode;                // User Data report mode (0 : interrupt mode, 1 interrupt disable mode)


    LowDelayInfo    lowDelayInfo;
    int             frameStartPos;
    int             frameEndPos;
    int             frameDelay;
    vpu_buffer_t    vbSlice;            // AVC, VP8 only
    vpu_buffer_t    vbWork;
    vpu_buffer_t    vbTemp;
    vpu_buffer_t    vbReport;
    vpu_buffer_t    vbTask;
    DecOutputInfo   decOutInfo[MAX_GDI_IDX];
    TiledMapConfig  mapCfg;
    int             reorderEnable;
    Uint32          avcErrorConcealMode;
    DRAMConfig      dramCfg;            //coda960 only
    int             thumbnailMode;
    int             seqChangeMask;      // WAVE410
    TemporalIdMode  tempIdSelectMode;
    Int32           targetTempId;
    Int32           targetSpatialId;    // AV1 only
    Int32           instanceQueueCount;
    Int32           reportQueueCount;
    BOOL            instanceQueueFull;
    BOOL            reportQueueEmpty;
    Uint32          firstCycleCheck;
    Uint32          cyclePerTick;
    Uint32          productCode;
    Uint32          vlcBufSize;
    Uint32          paramBufSize;
    vpu_rpt_info_t  rpt_info;
    Uint32          frameBufFlag;
} DecInfo;


typedef struct {
    EncOpenParam        openParam;
    EncInitialInfo      initialInfo;
    PhysicalAddress     streamRdPtr;
    PhysicalAddress     streamWrPtr;
    int                 streamEndflag;
    PhysicalAddress     streamRdPtrRegAddr;
    PhysicalAddress     streamWrPtrRegAddr;
    PhysicalAddress     streamBufStartAddr;
    PhysicalAddress     streamBufEndAddr;
    PhysicalAddress     currentPC;
    PhysicalAddress     busyFlagAddr;
    int                 streamBufSize;
    int                 linear2TiledEnable;
    int                 linear2TiledMode;    // coda980 only
    TiledMapType        mapType;
    int                 userMapEnable;
    FrameBuffer         frameBufPool[MAX_REG_FRAME];
    vpu_buffer_t        vbFrame;
    vpu_buffer_t        vbPPU;
    int                 frameAllocExt;
    int                 ppuAllocExt;
    vpu_buffer_t        vbSubSampFrame;         /*!<< CODA960 only */
    vpu_buffer_t        vbMvcSubSampFrame;      /*!<< CODA960 only */
    int                 numFrameBuffers;
    int                 stride;
    int                 frameBufferHeight;
    int                 rotationEnable;
    int                 mirrorEnable;
    MirrorDirection     mirrorDirection;
    int                 rotationAngle;
    int                 initialInfoObtained;
    int                 ringBufferEnable;
    SecAxiInfo          secAxiInfo;
    MaverickCacheConfig cacheConfig;
    EncSubFrameSyncConfig subFrameSyncConfig;   /*!<< CODA980 & WAVE320 */
    int                 ActivePPSIdx;           /*!<< CODA980 */
    int                 frameIdx;               /*!<< CODA980 */
    int                 fieldDone;              /*!<< CODA980 */
    int                 lineBufIntEn;
    vpu_buffer_t        vbWork;
    vpu_buffer_t        vbScratch;

    vpu_buffer_t        vbTemp;                     //!< Temp buffer (WAVE encoder )
    vpu_buffer_t        vbMV[MAX_REG_FRAME];        //!< colMV buffer (WAVE encoder)
    vpu_buffer_t        vbFbcYTbl[MAX_REG_FRAME];   //!< FBC Luma table buffer (WAVE encoder)
    vpu_buffer_t        vbFbcCTbl[MAX_REG_FRAME];   //!< FBC Chroma table buffer (WAVE encoder)
    vpu_buffer_t        vbSubSamBuf[MAX_REG_FRAME]; //!< Sub-sampled buffer for ME (WAVE encoder)
    vpu_buffer_t        vbTask;
    vpu_buffer_t        vbDefCdf;

    TiledMapConfig      mapCfg;
    DRAMConfig          dramCfg;                /*!<< CODA960 */

    Uint32 prefixSeiNalEnable;
    Uint32 prefixSeiDataSize;
    PhysicalAddress prefixSeiNalAddr;
    Uint32 suffixSeiNalEnable;
    Uint32 suffixSeiDataSize;
    PhysicalAddress suffixSeiNalAddr;
    Int32   errorReasonCode;
    Uint64          curPTS;             /**! Current timestamp in 90KHz */
    Uint64          ptsMap[32];         /**! PTS mapped with source frame index */
    Uint32          instanceQueueCount;
    Uint32          reportQueueCount;
    BOOL            instanceQueueFull;
    BOOL            reportQueueEmpty;
    Uint32          encWrPtrSel;
    Uint32          firstCycleCheck;
    Uint32          cyclePerTick;
    Uint32          productCode;
    Uint32          vlcBufSize;
    Uint32          paramBufSize;
    Int32           ringBufferWrapEnable;
    Uint32          streamEndian;
    Uint32          sourceEndian;
    Uint32          customMapEndian;
    Uint32          encWidth;
    Uint32          encHeight;

    BOOL addrRemapEn;
    AddrRemap addrRemap;
} EncInfo;

typedef struct CodecInst {
    Int32   inUse;
    Int32   instIndex;
    Int32   coreIdx;
    Int32   codecMode;
    Int32   codecModeAux;
    Int32   productId;
    Int32   loggingEnable;
    Uint32  isDecoder;
    union {
        EncInfo encInfo;
        DecInfo decInfo;
    }* CodecInfo;
} CodecInst;

/**
* @brief This structure is used for reporting bandwidth (only for WAVE5).
*/
typedef struct {
    Uint32 bwMode;         /* 0: total bandwidth, 1: select burst length bandwidth*/
    Uint32 burstLengthIdx;
    Uint32 prpBwRead;
    Uint32 prpBwWrite;
    Uint32 fbdYRead;
    Uint32 fbcYWrite;
    Uint32 fbdCRead;
    Uint32 fbcCWrite;
    Uint32 priBwRead;
    Uint32 priBwWrite;
    Uint32 secBwRead;
    Uint32 secBwWrite;
    Uint32 procBwRead;
    Uint32 procBwWrite;
    Uint32 bwbBwWrite;
} VPUBWData;


/*******************************************************************************
 * H.265 USER DATA(VUI and SEI)                                                *
 *******************************************************************************/
#define H265_MAX_DPB_SIZE                 17
#define H265_MAX_NUM_SUB_LAYER            8
#define H265_MAX_NUM_ST_RPS               64
#define H265_MAX_CPB_CNT                  32
#define H265_MAX_NUM_VERTICAL_FILTERS     6
#define H265_MAX_NUM_HORIZONTAL_FILTERS   4
#define H265_MAX_TAP_LENGTH               32
#define H265_MAX_NUM_KNEE_POINT           1000

#define H265_MAX_NUM_TONE_VALUE		      1024
#define H265_MAX_NUM_FILM_GRAIN_COMPONENT 3
#define H265_MAX_NUM_INTENSITY_INTERVALS  256
#define H265_MAX_NUM_MODEL_VALUES		  5

#define H265_MAX_LUT_NUM_VAL			  4
#define H265_MAX_LUT_NUM_VAL_MINUS1       33
#define H265_MAX_COLOUR_REMAP_COEFFS	  4
typedef struct
{
    Uint32   offset;
    Uint32   size;
} user_data_entry_t;

typedef struct
{
    Int16   left;
    Int16   right;
    Int16   top;
    Int16   bottom;
} win_t;

typedef struct
{
    Uint32 nal_hrd_param_present_flag                 : 1; // [    0]
    Uint32 vcl_hrd_param_present_flag                 : 1; // [    1]
    Uint32 sub_pic_hrd_params_present_flag            : 1; // [    2]
    Uint32 tick_divisor_minus2                        : 8; // [10: 3]
    Uint32 du_cpb_removal_delay_inc_length_minus1     : 5; // [15:11]
    Uint32 sub_pic_cpb_params_in_pic_timing_sei_flag  : 1; // [   16]
    Uint32 dpb_output_delay_du_length_minus1          : 5; // [21:17]
    Uint32 bit_rate_scale                             : 4; // [25:22]
    Uint32 cpb_size_scale                             : 4; // [29:26]
    Uint32 reserved_0                                 : 2; // [31:30]

    Uint32 initial_cpb_removal_delay_length_minus1    : 10; // [ 9: 0]
    Uint32 cpb_removal_delay_length_minus1            :  5; // [14:10]
    Uint32 dpb_output_delay_length_minus1             :  5; // [19:15]
    Uint32 reserved_1                                 : 12; // [31:20]

    Uint32 fixed_pic_rate_gen_flag[H265_MAX_NUM_SUB_LAYER];
    Uint32 fixed_pic_rate_within_cvs_flag[H265_MAX_NUM_SUB_LAYER];
    Uint32 low_delay_hrd_flag[H265_MAX_NUM_SUB_LAYER];
    Int32  cpb_cnt_minus1[H265_MAX_NUM_SUB_LAYER];
    Int32  elemental_duration_in_tc_minus1[H265_MAX_NUM_SUB_LAYER];

    Uint32 nal_bit_rate_value_minus1[H265_MAX_NUM_SUB_LAYER][H265_MAX_CPB_CNT];
    Uint32 nal_cpb_size_value_minus1[H265_MAX_NUM_SUB_LAYER][H265_MAX_CPB_CNT];
    Uint32 nal_cpb_size_du_value_minus1[H265_MAX_NUM_SUB_LAYER];
    Uint32 nal_bit_rate_du_value_minus1[H265_MAX_NUM_SUB_LAYER];
    Uint32 nal_cbr_flag[H265_MAX_NUM_SUB_LAYER][H265_MAX_CPB_CNT];

    Uint32 vcl_bit_rate_value_minus1[H265_MAX_NUM_SUB_LAYER][H265_MAX_CPB_CNT];
    Uint32 vcl_cpb_size_value_minus1[H265_MAX_NUM_SUB_LAYER][H265_MAX_CPB_CNT];
    Uint32 vcl_cpb_size_du_value_minus1[H265_MAX_NUM_SUB_LAYER];
    Uint32 vcl_bit_rate_du_value_minus1[H265_MAX_NUM_SUB_LAYER];
    Uint32 vcl_cbr_flag[H265_MAX_NUM_SUB_LAYER][H265_MAX_CPB_CNT];
} h265_hrd_param_t;

typedef struct
{
    Uint32 aspect_ratio_info_present_flag             :  1; // [    0]
    Uint32 aspect_ratio_idc                           :  8; // [ 8: 1]
    Uint32 reserved_0                                 : 23; // [31: 9]

    Uint32 sar_width                                  : 16; // [15: 0]
    Uint32 sar_height                                 : 16; // [31:16]

    Uint32 overscan_info_present_flag                 :  1; // [    0]
    Uint32 overscan_appropriate_flag                  :  1; // [    1]
    Uint32 video_signal_type_present_flag             :  1; // [    2]
    Uint32 video_format                               :  3; // [ 5: 3]
    Uint32 video_full_range_flag                      :  1; // [    6]
    Uint32 colour_description_present_flag            :  1; // [    7]
    Uint32 colour_primaries                           :  8; // [15: 8]
    Uint32 transfer_characteristics                   :  8; // [23:16]
    Uint32 matrix_coefficients                        :  8; // [31:24]

    Uint32 chroma_loc_info_present_flag               :  1; // [    0]
    Int32  chroma_sample_loc_type_top_field           :  8; // [ 8: 1]
    Int32  chroma_sample_loc_type_bottom_field        :  8; // [16: 9]
    Uint32 neutral_chroma_indication_flag             :  1; // [   17]
    Uint32 field_seq_flag                             :  1; // [   18]
    Uint32 frame_field_info_present_flag              :  1; // [   19]
    Uint32 default_display_window_flag                :  1; // [   20]
    Uint32 vui_timing_info_present_flag               :  1; // [   21]
    Uint32 vui_poc_proportional_to_timing_flag        :  1; // [   22]
    Uint32 vui_hrd_parameters_present_flag            :  1; // [   23]
    Uint32 bitstream_restriction_flag                 :  1; // [   24]
    Uint32 tiles_fixed_structure_flag                 :  1; // [   25]
    Uint32 motion_vectors_over_pic_boundaries_flag    :  1; // [   26]
    Uint32 restricted_ref_pic_lists_flag              :  1; // [   27]
    Uint32 reserved_1                                 :  4; // [31:28]

    Uint32 vui_num_units_in_tick                       : 32; // [31: 0]
    Uint32 vui_time_scale                              : 32; // [31: 0]

    Uint32 min_spatial_segmentation_idc               : 12; // [11: 0]
    Uint32 max_bytes_per_pic_denom                    :  5; // [16:12]
    Uint32 max_bits_per_mincu_denom                   :  5; // [21:17]
    Uint32 log2_max_mv_length_horizontal              :  5; // [26:22]
    Uint32 log2_max_mv_length_vertical                :  5; // [31:27]

    Int32 vui_num_ticks_poc_diff_one_minus1           : 32; // [31: 0]

    win_t   def_disp_win;
    h265_hrd_param_t hrd_param;
} h265_vui_param_t;

typedef struct
{
    Uint32 display_primaries_x[3];
    Uint32 display_primaries_y[3];

    Uint32 white_point_x                   : 16;
    Uint32 white_point_y                   : 16;

    Uint32 max_display_mastering_luminance : 32;
    Uint32 min_display_mastering_luminance : 32;
} h265_mastering_display_colour_volume_t;

typedef struct
{
    Uint32 ver_chroma_filter_idc               : 8; // [ 7: 0]
    Uint32 hor_chroma_filter_idc               : 8; // [15: 8]
    Uint32 ver_filtering_field_processing_flag : 1; // [   16]
    Uint32 target_format_idc                   : 2; // [18:17]
    Uint32 num_vertical_filters                : 3; // [21:19]
    Uint32 num_horizontal_filters              : 3; // [24:22]
    Uint32 reserved_0                          : 7; // [31:25]

    Uint16 ver_tap_length_minus1[H265_MAX_NUM_VERTICAL_FILTERS];
    Uint16 hor_tap_length_minus1[H265_MAX_NUM_HORIZONTAL_FILTERS];

    Int32 ver_filter_coeff[H265_MAX_NUM_VERTICAL_FILTERS][H265_MAX_TAP_LENGTH];
    Int32 hor_filter_coeff[H265_MAX_NUM_HORIZONTAL_FILTERS][H265_MAX_TAP_LENGTH];
} h265_chroma_resampling_filter_hint_t;

typedef struct
{
    Uint32 knee_function_id                         : 32; // [31: 0]

    Uint32 knee_function_cancel_flag                :  1; // [    0]
    Uint32 knee_function_persistence_flag           :  1; // [    1]
    Uint32 reserved_0                               : 30; // [31: 2]

    Uint32 input_d_range                            : 32; // [31: 0]
    Uint32 input_disp_luminance                     : 32; // [31: 0]
    Uint32 output_d_range                           : 32; // [31: 0]
    Uint32 output_disp_luminance                    : 32; // [31: 0]
    Uint32 num_knee_points_minus1                   : 32; // [31: 0]

    Uint16 input_knee_point[H265_MAX_NUM_KNEE_POINT];
    Uint16 output_knee_point[H265_MAX_NUM_KNEE_POINT];
} h265_knee_function_info_t;

typedef struct
{
    Uint32 max_content_light_level                : 16; // [15: 0]
    Uint32 max_pic_average_light_level            : 16; // [31:16]
} h265_content_light_level_info_t;

typedef struct
{
    Uint32    colour_remap_id                                 : 32; // [31: 0]

    Uint32    colour_remap_cancel_flag                        :  1; // [    0]
    Uint32    colour_remap_persistence_flag                   :  1; // [    1]
    Uint32    colour_remap_video_signal_info_present_flag     :  1; // [    2]
    Uint32    colour_remap_full_range_flag                    :  1; // [    3]
    Uint32    colour_remap_primaries                          :  8; // [11: 4]
    Uint32    colour_remap_transfer_function                  :  8; // [19:12]
    Uint32    colour_remap_matrix_coefficients                :  8; // [27:20]
    Uint32    reserved_0                                      :  4; // [31:28]

    Uint32    colour_remap_input_bit_depth                    : 16; // [15: 0]
    Uint32    colour_remap_output_bit_depth                   : 16; // [31:16]

    Uint16     pre_lut_num_val_minus1[H265_MAX_LUT_NUM_VAL];

    Uint16    pre_lut_coded_value[H265_MAX_LUT_NUM_VAL][H265_MAX_LUT_NUM_VAL_MINUS1];

    Uint16    pre_lut_target_value[H265_MAX_LUT_NUM_VAL][H265_MAX_LUT_NUM_VAL_MINUS1];

    Uint32    colour_remap_matrix_present_flag                : 1; // [    0]
    Uint32    log2_matrix_denom                               : 4; // [ 4: 1]
    Uint32    reserved_4                                      : 27; // [31: 5]

    Uint16     colour_remap_coeffs[H265_MAX_COLOUR_REMAP_COEFFS][H265_MAX_COLOUR_REMAP_COEFFS];

    Uint16     post_lut_num_val_minus1[H265_MAX_LUT_NUM_VAL];

    Uint16    post_lut_coded_value[H265_MAX_LUT_NUM_VAL][H265_MAX_LUT_NUM_VAL_MINUS1];

    Uint16    post_lut_target_value[H265_MAX_LUT_NUM_VAL][H265_MAX_LUT_NUM_VAL_MINUS1];
} h265_colour_remapping_info_t;

typedef struct
{
    Uint32 film_grain_characteristics_cancel_flag              :  1; // [    0]
    Uint32 film_grain_model_id                                 :  2; // [ 2: 1]
    Uint32 separate_colour_description_present_flag            :  1; // [    3]
    Uint32 film_grain_bit_depth_luma_minus8                    :  3; // [ 6: 4]
    Uint32 film_grain_bit_depth_chroma_minus8                  :  3; // [ 9: 7]
    Uint32 film_grain_full_range_flag                          :  1; // [   10]
    Uint32 reserved_0                                          : 21; // [31:11]

    Uint32 film_grain_colour_primaries                         :  8; // [ 7: 0]
    Uint32 film_grain_transfer_characteristics                 :  8; // [15: 8]
    Uint32 film_grain_matrix_coeffs                            :  8; // [23:16]
    Uint32 blending_mode_id                                    :  2; // [25:24]
    Uint32 log2_scale_factor                                   :  4; // [29:26]
    Uint32 film_grain_characteristics_persistence_flag         :  1; // [   30]
    Uint32 reserved_1                                          :  1; // [   31]

    Uint32  comp_model_present_flag[H265_MAX_NUM_FILM_GRAIN_COMPONENT];

    Uint32  num_intensity_intervals_minus1[H265_MAX_NUM_FILM_GRAIN_COMPONENT];

    Uint32  num_model_values_minus1[H265_MAX_NUM_FILM_GRAIN_COMPONENT];

    Uint8  intensity_interval_lower_bound[H265_MAX_NUM_FILM_GRAIN_COMPONENT][H265_MAX_NUM_INTENSITY_INTERVALS];
    Uint8  intensity_interval_upper_bound[H265_MAX_NUM_FILM_GRAIN_COMPONENT][H265_MAX_NUM_INTENSITY_INTERVALS];

    Uint32 comp_model_value[H265_MAX_NUM_FILM_GRAIN_COMPONENT][H265_MAX_NUM_INTENSITY_INTERVALS][H265_MAX_NUM_MODEL_VALUES];
} h265_film_grain_characteristics_t;

typedef struct
{
    Uint32 tone_map_id                                        : 32; // [31: 0]

    Uint32 tone_map_cancel_flag                                :  1; // [    0]
    Uint32 tone_map_persistence_flag                           :  1; // [    1]
    Uint32 coded_data_bit_depth                                :  8; // [ 9: 2]
    Uint32 target_bit_depth                                    :  8; // [17:10]
    Uint32 tone_map_model_id                                   :  4; // [21:18]
    Uint32 reserved_0                                          : 10; // [31:22]

    Uint32 min_value                                          : 32; // [31: 0]
    Uint32 max_value                                          : 32; // [31: 0]
    Uint32 sigmoid_midpoint                                   : 32; // [31: 0]
    Uint32 sigmoid_width                                      : 32; // [31: 0]

    Uint16 start_of_coded_interval[H265_MAX_NUM_TONE_VALUE];

    Uint32 num_pivots                                         :32; // [31: 0]

    Uint16 coded_pivot_value[H265_MAX_NUM_TONE_VALUE];
    Uint16 target_pivot_value[H265_MAX_NUM_TONE_VALUE];

    Uint32 camera_iso_speed_idc                       :  8; // [ 7: 0]
    Uint32 exposure_index_idc                         :  8; // [15: 8]
    Uint32 exposure_compensation_value_sign_flag      :  1; // [   16]
    Uint32 reserved_1                                 : 15; // [31:17]

    Uint32 camera_iso_speed_value                     : 32; // [31: 0]

    Uint32 exposure_index_value                       : 32; // [31: 0]

    Uint32 exposure_compensation_value_numerator      : 16; // [15: 0]
    Uint32 exposure_compensation_value_denom_idc      : 16; // [31:16]

    Uint32 ref_screen_luminance_white                 : 32; // [31: 0]
    Uint32 extended_range_white_level                 : 32; // [31: 0]

    Uint32 nominal_black_level_code_value             : 16; // [15: 0]
    Uint32 nominal_white_level_code_value             : 16; // [31:16]

    Uint32 extended_white_level_code_value            : 16; // [15: 0]
    Uint32 reserved_2                                 : 16; // [31:16]
} h265_tone_mapping_info_t;

typedef struct
{
    Uint32 pic_struct                               :  4; // [ 3: 0]
    Uint32 source_scan_type                         :  2; // [ 5: 4]
    Uint32 duplicate_flag                           :  1; // [    6]
    Uint32 reserved_0                               : 25; // [31: 7]
} h265_sei_pic_timing_t;


/*******************************************************************************
 * H.264 USER DATA(VUI and SEI)                                                *
 *******************************************************************************/
typedef struct
{
    Int32 cpb_cnt                             : 16; // [15: 0]
    Uint32 bit_rate_scale                     :  8; // [23:16]
    Uint32 cbp_size_scale                     :  8; // [31:24]

    Int32 initial_cpb_removal_delay_length   : 16; // [15: 0]
    Int32 cpb_removal_delay_length           : 16; // [31:16]

    Int32 dpb_output_delay_length            : 16; // [15: 0]
    Int32 time_offset_length                 : 16; // [31:16]

    Uint32 bit_rate_value[32];

    Uint32 cpb_size_value[32];

    Uint8 cbr_flag[32];
} avc_hrd_param_t;

typedef struct
{
    Uint32 aspect_ratio_info_present_flag          :  1; // [    0]
    Uint32 aspect_ratio_idc                        :  8; // [ 8: 1]
    Uint32 reserved_0                              : 23; // [31: 9]

    Uint32 sar_width                               : 16; // [15: 0]
    Uint32 sar_height                              : 16; // [31:15]

    Uint32 overscan_info_present_flag              :  1; // [    0]
    Uint32 overscan_appropriate_flag               :  1; // [    1]
    Uint32 video_signal_type_present_flag          :  1; // [    2]
    Uint32 video_format                            :  3; // [ 5: 3]
    Uint32 video_full_range_flag                   :  1; // [    6]
    Uint32 colour_description_present_flag         :  1; // [    7]
    Uint32 colour_primaries                        :  8; // [15: 8]
    Uint32 transfer_characteristics                :  8; // [23:16]
    Uint32 matrix_coefficients                     :  8; // [31:24]

    Uint32 chroma_loc_info_present_flag            :  1; // [    0]
    Int32 chroma_sample_loc_type_top_field        :  8; // [ 8: 1]
    Int32 chroma_sample_loc_type_bottom_field     :  8; // [16: 9]
    Uint32 vui_timing_info_present_flag            :  1; // [   17]
    Uint32 reserved_1                              : 14; // [31:18]

    Uint32 vui_num_units_in_tick                   : 32;

    Uint32 vui_time_scale                          : 32;

    Uint32 vui_fixed_frame_rate_flag               :  1; // [    0]
    Uint32 vcl_hrd_parameters_present_flag         :  1; // [    1]
    Uint32 nal_hrd_parameters_present_flag         :  1; // [    2]
    Uint32 low_delay_hrd_flag                      :  1; // [    3]
    Uint32 pic_struct_present_flag                 :  1; // [    4]
    Uint32 bitstream_restriction_flag              :  1; // [    5]
    Uint32 motion_vectors_over_pic_boundaries_flag :  1; // [    6]
    Int32 max_bytes_per_pic_denom                 :  8; // [14: 7]
    Int32 max_bits_per_mincu_denom                :  8; // [22:15]
    Uint32 reserved_2                              :  9; // [31:23]

    Int32 log2_max_mv_length_horizontal           :  8; // [ 7: 0]
    Int32 log2_max_mv_length_vertical             :  8; // [15: 8]
    Int32 max_num_reorder_frames                  :  8; // [23:16]
    Int32 max_dec_frame_buffering                 :  8; // [31:24]

    avc_hrd_param_t vcl_hrd;
    avc_hrd_param_t nal_hrd;
} avc_vui_info_t;

typedef struct
{
    Uint32 cpb_removal_delay                         : 32;

    Uint32 dpb_output_delay                          : 32;

    Uint32 pic_struct                                :  4; // [ 3: 0]
    Uint32 num_clock_ts                              : 16; // [19: 4]
    Uint32 resreved_0                                : 11; // [31:20]
} avc_sei_pic_timing_t;

#define AVC_MAX_NUM_FILM_GRAIN_COMPONENT   3
#define AVC_MAX_NUM_INTENSITY_INTERVALS    256
#define AVC_MAX_NUM_MODEL_VALUES           5
#define AVC_MAX_NUM_TONE_VALUE             1024

typedef struct
{
    Uint32 film_grain_characteristics_cancel_flag         :  1; // [    0]
    Uint32 film_grain_model_id                            :  2; // [ 2: 1]
    Uint32 separate_colour_description_present_flag       :  1; // [    3]
    Uint32 reserved_0                                     : 28; // [31: 4]

    Uint32 film_grain_bit_depth_luma_minus8               :  3; // [ 2: 0]
    Uint32 film_grain_bit_depth_chroma_minus8             :  3; // [ 5: 3]
    Uint32 film_grain_full_range_flag                     :  1; // [    6]
    Uint32 film_grain_colour_primaries                    :  8; // [14: 7]
    Uint32 film_grain_transfer_characteristics            :  8; // [22:15]
    Uint32 film_grain_matrix_coeffs                       :  8; // [30:23]
    Uint32 reserved_1                                     :  1; // [   31]

    Uint32 blending_mode_id                               :  2; // [ 1: 0]
    Uint32 log2_scale_factor                              :  4; // [ 5: 2]
    Uint32 film_grain_characteristics_persistence_flag    :  1; // [    6]
    Uint32 reserved_2                                     : 25; // [31: 7]

    Uint32 comp_model_present_flag[AVC_MAX_NUM_FILM_GRAIN_COMPONENT];

    Uint32 num_intensity_intervals_minus1[AVC_MAX_NUM_FILM_GRAIN_COMPONENT];

    Uint32 num_model_values_minus1[AVC_MAX_NUM_FILM_GRAIN_COMPONENT];

    Uint8 intensity_interval_lower_bound[AVC_MAX_NUM_FILM_GRAIN_COMPONENT][AVC_MAX_NUM_INTENSITY_INTERVALS];
    Uint8 intensity_interval_upper_bound[AVC_MAX_NUM_FILM_GRAIN_COMPONENT][AVC_MAX_NUM_INTENSITY_INTERVALS];

    Uint32 comp_model_value[AVC_MAX_NUM_FILM_GRAIN_COMPONENT][AVC_MAX_NUM_INTENSITY_INTERVALS][AVC_MAX_NUM_MODEL_VALUES];
} avc_sei_film_grain_t;

typedef struct
{
    Uint32 tone_map_id                                : 32;

    Uint32 tone_map_cancel_flag                      :  1; // [    0]
    Uint32 reserved_0                                : 31; // [31: 1]

    Uint32 tone_map_repetition_period                 : 32;

    Uint32 coded_data_bit_depth                       :  8; // [ 7: 0]
    Uint32 target_bit_depth                           :  8; // [15: 8]
    Uint32 tone_map_model_id                          :  8; // [23:16]
    Uint32 reserved_1                                 :  8; // [31:24]

    Uint32 min_value                                  : 32;
    Uint32 max_value                                  : 32;
    Uint32 sigmoid_midpoint                           : 32;
    Uint32 sigmoid_width                              : 32;
    Uint16 start_of_coded_interval[AVC_MAX_NUM_TONE_VALUE]; // [1 << target_bit_depth] // 10bits

    Uint32 num_pivots                                 : 16; // [15: 0], [(1 << coded_data_bit_depth)?1][(1 << target_bit_depth)-1] // 10bits
    Uint32 reserved_2                                 : 16; // [31:16]

    Uint16 coded_pivot_value[AVC_MAX_NUM_TONE_VALUE];
    Uint16 target_pivot_value[AVC_MAX_NUM_TONE_VALUE];

    Uint32 camera_iso_speed_idc                       :  8; // [ 7: 0]
    Uint32 reserved_3                                 : 24; // [31: 8]

    Uint32 camera_iso_speed_value                     : 32;

    Uint32 exposure_index_idc                         :  8; // [ 7: 0]
    Uint32 reserved_4                                 : 24; // [31: 8]

    Uint32 exposure_index_value                       : 32;

    Uint32 exposure_compensation_value_sign_flag     :  1; // [    0]
    Uint32 reserved_5                                : 31; // [31: 1]

    Uint32 exposure_compensation_value_numerator     : 16; // [15: 0]
    Uint32 exposure_compensation_value_denom_idc     : 16; // [31:16]

    Uint32 ref_screen_luminance_white                : 32;
    Uint32 extended_range_white_level                : 32;

    Uint32 nominal_black_level_code_value            : 16; // [15: 0]
    Uint32 nominal_white_level_code_value            : 16; // [31:16]

    Uint32 extended_white_level_code_value           : 16; // [15: 0]
    Uint32 reserved_6                                : 16; // [31:16]
} avc_sei_tone_mapping_info_t;

typedef struct
{
    Uint32 colour_remap_id                                 : 32;

    Uint32 colour_cancel_flag                              :  1; // [    0]
    Uint32 colour_remap_repetition_period                  : 16; // [16: 1]
    Uint32 colour_remap_video_signal_info_present_flag     :  1; // [   17]
    Uint32 colour_remap_full_range_flag                    :  1; // [   18]
    Uint32 reserved_0                                      : 13; // [31:19]

    Uint32 colour_remap_primaries                          :  8; // [ 7: 0]
    Uint32 colour_remap_transfer_function                  :  8; // [15: 8]
    Uint32 colour_remap_matrix_coefficients                :  8; // [23:16]
    Uint32 reserved_1                                      :  8; // [31:24]

    Uint32 colour_remap_input_bit_depth                    :  8; // [ 7: 0]
    Uint32 colour_remap_output_bit_depth                   :  8; // [15: 8]
    Uint32 reserved_2                                      : 16; // [31:16]

    Uint32 pre_lut_num_val_minus1[3];
    Uint32 pre_lut_coded_value[3][33];
    Uint32 pre_lut_target_value[3][33];

    Uint32 colour_remap_matrix_present_flag                :  1; // [    0]
    Uint32 log2_matrix_denom                               :  4; // [ 4: 1]
    Uint32 reserved_3                                      : 27; // [31: 5]

    Uint32 colour_remap_coeffs[3][3];

    Uint32 post_lut_num_val_minus1[3];
    Uint32 post_lut_coded_value[3][33];
    Uint32 post_lut_target_value[3][33];
} avc_sei_colour_remap_info_t;


#ifdef __cplusplus
extern "C" {
#endif

RetCode InitCodecInstancePool(Uint32 coreIdx);
RetCode GetCodecInstance(Uint32 coreIdx, CodecInst ** ppInst);
void    FreeCodecInstance(CodecInst * pCodecInst);

int     DecBitstreamBufEmpty(DecInfo * pDecInfo);
RetCode SetParaSet(DecHandle handle, int paraSetType, DecParamSet * para);
void    DecSetHostParaAddr(Uint32 coreIdx, PhysicalAddress baseAddr, PhysicalAddress paraAddr);

RetCode CheckInstanceValidity(CodecInst* pCodecInst);
Int32   ConfigSecAXICoda9(Uint32 coreIdx, Uint32 productId, Int32 codecMode, SecAxiInfo* sa, Uint32 width, Uint32 height, Uint32 profile);
RetCode UpdateFrameBufferAddr(TiledMapType mapType, FrameBuffer* fbArr, Uint32 numOfFrameBuffers, Uint32 sizeLuma, Uint32 sizeChroma);
RetCode AllocateTiledFrameBufferGdiV1(TiledMapType mapType, PhysicalAddress tiledBaseAddr, FrameBuffer* fbArr, Uint32 numOfFrameBuffers, Uint32 sizeLuma, Uint32 sizeChroma, DRAMConfig* pDramCfg);
RetCode AllocateTiledFrameBufferGdiV2(TiledMapType mapType, FrameBuffer* fbArr, Uint32 numOfFrameBuffers, Uint32 sizeLuma, Uint32 sizeChroma);

RetCode EnterLock(Uint32 coreIdx);
RetCode LeaveLock(Uint32 coreIdx);
RetCode SetClockGate(Uint32 coreIdx, Uint32 on);

RetCode EnterDispFlagLock(Uint32 coreIdx);
RetCode LeaveDispFlagLock(Uint32 coreIdx);

void       SetPendingInst(Uint32 coreIdx, CodecInst *inst);
void       ClearPendingInst(Uint32 coreIdx);
CodecInst* GetPendingInst(Uint32 coreIdx);
int        GetPendingInstIdx(Uint32 coreIdx);

Int32 MaverickCache2Config(MaverickCacheConfig* pCache, BOOL decoder, BOOL interleave, Uint32 bypass, Uint32 burst, Uint32 merge, TiledMapType mapType, Uint32 wayshape);
int   SetTiledMapType(Uint32 coreIdx, TiledMapConfig *pMapCfg, int mapType, int stride, int interleave, DRAMConfig *dramCfg);
int   GetXY2AXIAddr(TiledMapConfig *pMapCfg, int ycbcr, int posY, int posX, int stride, FrameBuffer *fb);
int   GetLowDelayOutput(CodecInst *pCodecInst, DecOutputInfo *lowDelayOutput);
Int32 CalcStride(Int32 productId, Uint32 width, Uint32 height, FrameBufferFormat format, BOOL cbcrInterleave, TiledMapType mapType, BOOL isVP9);
Int32 CalcLumaSize(CodecInst* inst, Int32 productId, Int32 stride, Int32 height, FrameBufferFormat format, BOOL cbcrIntl, TiledMapType mapType, DRAMConfig* pDramCfg);
Int32 CalcChromaSize(CodecInst* inst, Int32 productId, Int32 stride, Int32 height, FrameBufferFormat format, BOOL cbcrIntl, TiledMapType mapType, DRAMConfig* pDramCfg);

//for GDI 1.0
void            SetTiledFrameBase(Uint32 coreIdx, PhysicalAddress baseAddr);
PhysicalAddress GetTiledFrameBase(Uint32 coreIdx, FrameBuffer *frame, int num);

RetCode CheckEncInstanceValidity(EncHandle handle);
RetCode EncParaSet(EncHandle handle, int paraSetType);
RetCode SetSliceMode(EncHandle handle, EncSliceMode *pSliceMode);
RetCode SetHecMode(EncHandle handle, int mode);
void    EncSetHostParaAddr(Uint32 coreIdx, PhysicalAddress baseAddr, PhysicalAddress paraAddr);
int     LevelCalculation(int MbNumX, int MbNumY, int frameRateInfo, int interlaceFlag, int BitRate, int SliceNum);
/* timescale: 1/90000 */
Uint64  GetTimestamp(EncHandle handle);
RetCode SetEncCropInfo(Int32 codecMode, VpuRect* param, int rotMode, int srcWidth, int srcHeight);






#if defined(SUPPORT_SW_UART) || defined(SUPPORT_SW_UART_V2)
int SwUartHandler(void *context);
int  create_sw_uart_thread(unsigned long coreIdx, unsigned long productId);
void destroy_sw_uart_thread(unsigned long coreIdx);
#endif
#ifdef __cplusplus
}
#endif

#endif // endif VPUAPI_UTIL_H_INCLUDED

