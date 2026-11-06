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

#ifndef NEW_MAP_CONV_H
#define NEW_MAP_CONV_H

//#define MAP_CONV_CLEAR_MEMORY
//#define MAPI_TEST_ENABLE_DEBUG_MSG


#define MAPI_DISPLAY_TIMEOUT            (5*1000)


#define MAPI_TEST_MAX_INPUT_PATH            5
#define MAPI_TEST_MAX_OUTPUT_PATH           2
#define MAPI_TEST_MAX_NUM_INPUT_BUF         20
#define MAPI_TEST_MAX_NUM_OUTPUT_BUF        20

#define INPUT_PATH_POST_FIX             "input_data.bin"
#define REF_PATH_POST_FIX               "ref_data.bin"

#define FBC_DATA_PRE_PATH               "fbc_result/"

#define MIN_WIDTH               128
#define MIN_HEIGHT              32
#ifdef SUPPORT_MAPI_8K
#define MAX_WIDTH               7680
#define MAX_HEIGHT              4320
#else
#define MAX_WIDTH               4096
#define MAX_HEIGHT              2160
#endif
#define MIN_SCL_WIDTH           128
#define MIN_SCL_HEIGHT          32
#define MAX_SCL_WIDTH           4096
#define MAX_SCL_HEIGHT          2160
#define VPU_ALIGN16_DOWN(_x)   (((_x)/16)*16)

#define MAPI_FPGA_CODE                  0x4d415049
#define PRODUCT_ID_MAPI(x)              (x == MAPI_FPGA_CODE)



typedef enum {
    MAPI_TEST_INPUT_ERR = 0,
    MAPI_INPUT_DISP,
    MAPI_INPUT_CF10,
    MAPI_INPUT_NOT_SUPPORT,
    MAPI_INPUT_RDMA = 4,
}InputCtrlMode;

typedef enum {
    MAPI_OUTPUT_ERR = 0,
    MAPI_OUTPUT_DISP,
    MAPI_OUTPUT_RESERVED1,
    MAPI_OUTPUT_RESERVED2,
    MAPI_OUTPUT_WDMA,
    MAPI_OUTPUT_DISP_WDMA,
    MAPI_OUTPUT_RESERVED = 0xff,
}OutputCtrlMode;

#define IS_LINEAR_INPUT(x)          (x == MAPI_INPUT_RDMA || x == MAPI_INPUT_DISP)
#define IS_COMPRESSED_INPUT(x)      (x == MAPI_INPUT_CF10)

typedef struct {
    Int32       bit_depth;
    BOOL        is_msb;
    BOOL        is_3p4b;
    BOOL        is_nv21;
    BOOL        is_packed;
    BOOL        is_1plane;
    BOOL        is_aligned;
    MAPIFormat  mapi_fmt;
    EndianMode  endian;
} MapiFormatProperty;

/**
 * @brief   from vpiapi.h::FrameBufferFormat
 */
 typedef enum {
    MAPI_FORMAT_ERR           = -1,
    MAPI_FORMAT_420           = 0 ,    /* 8bit */
    MAPI_FORMAT_422           = 1 ,    /* 8bit */
    MAPI_FORMAT_444           = 2 ,    /* 8bit */
    MAPI_FORMAT_RGB           = 3 ,

    MAPI_FORMAT_420_P10_16BIT_MSB = 5, /* lsb |000000xx|xxxxxxxx | msb */
    MAPI_FORMAT_420_P10_16BIT_LSB = 6,    /* lsb |xxxxxxx |xx000000 | msb */
    MAPI_FORMAT_420_P10_32BIT_MSB = 7,    /* lsb |00xxxxxxxxxxxxxxxxxxxxxxxxxxx| msb */
    MAPI_FORMAT_420_P10_32BIT_LSB = 8,    /* lsb |xxxxxxxxxxxxxxxxxxxxxxxxxxx00| msb */

    /* 4:2:2 packed format */
    /* Little Endian Perspective     */
    /*     | addr 0  | addr 1  |     */
    MAPI_FORMAT_422_P10_16BIT_MSB = 9,    /* lsb |000000xx |xxxxxxxx | msb */
    MAPI_FORMAT_422_P10_16BIT_LSB = 10,    /* lsb |xxxxxxxx |xx000000 | msb */
    MAPI_FORMAT_422_P10_32BIT_MSB = 11,    /* lsb |00xxxxxxxxxxxxxxxxxxxxxxxxxxx| msb */
    MAPI_FORMAT_422_P10_32BIT_LSB = 12,    /* lsb |xxxxxxxxxxxxxxxxxxxxxxxxxxx00| msb */

    MAPI_FORMAT_444_P10_16BIT_MSB = 13,
    MAPI_FORMAT_444_P10_16BIT_LSB = 14,
    MAPI_FORMAT_444_P10_32BIT_MSB = 15,
    MAPI_FORMAT_444_P10_32BIT_LSB = 16,

    /* RGB */
    MAPI_FORMAT_RGB_P10_1P2B_MSB = 20,
    MAPI_FORMAT_RGB_P10_1P2B_LSB = 21,
    MAPI_FORMAT_RGB_P10_3P4B_MSB = 22,
    MAPI_FORMAT_RGB_P10_3P4B_LSB = 23,

    MAPI_FORMAT_RGB_PACKED = 24,
    MAPI_FORMAT_RGB_P10_1P2B_MSB_PACKED = 25,
    MAPI_FORMAT_RGB_P10_1P2B_LSB_PACKED = 26,

    /* 1PLANE */
    MAPI_FORMAT_422_P8_8BIT_1PLANE = 30,
    MAPI_FORMAT_422_P10_16BIT_MSB_1PLANE = 31,
    MAPI_FORMAT_422_P10_16BIT_LSB_1PLANE = 32,
    MAPI_FORMAT_422_P10_32BIT_MSB_1PLANE = 33,
    MAPI_FORMAT_422_P10_32BIT_LSB_1PLANE = 34,

    MAPI_FORMAT_444_P8_8BIT_1PLANE = 35,
    MAPI_FORMAT_444_P10_16BIT_MSB_1PLANE = 36,
    MAPI_FORMAT_444_P10_16BIT_LSB_1PLANE = 37,
    MAPI_FORMAT_444_P10_32BIT_MSB_1PLANE = 38,
    MAPI_FORMAT_444_P10_32BIT_LSB_1PLANE = 39,

    MAPI_FORMAT_444_P8_8BIT_1PLANE_PACKED = 40,
    MAPI_FORMAT_444_P10_16BIT_MSB_1PLANE_PACKED = 41,
    MAPI_FORMAT_444_P10_16BIT_LSB_1PLANE_PACKED = 42,

    MAPI_FORMAT_MAX,
} FrameMapConvBufferFormat;

 typedef enum {
     MAP_COMP_FORMAT_ERR            = -1,
     MAP_COMPRESSED_FRAME_MAP       = 0,
     MAP_COMPRESSED_FRAME_LUMA_TABLE,
     MAP_COMPRESSED_FRAME_CHROMA_TABLE,
     MAP_COMP_FORMAT_MAX,
 } FrameMapConvCompressedType;

 typedef struct TestMAPIAPICtrl_struct {
     MAPIInputPath              mapi_input_path;
     MAPIOutputPath             mapi_output_path[MAPI_TEST_MAX_OUTPUT_PATH];
     MAPIInputRDMA              mapi_input_rdma;
     MAPIInputDIS               mapi_input_display;
     MAPIOutputDIS              mapi_output_display;

     MAPIInputCF10              mapi_input_cf10;
     MAPIInputFGN               mapi_input_fgn;

     MAPIOutputWDMA             mapi_output_wdma;

     MAPIOutputScaler           mapi_output_scaler[MAPI_TEST_MAX_OUTPUT_PATH];
 } TestMAPIAPICtrl;

 typedef struct FrameSizeInfo_struct {
     Uint32  luma_width;
     Uint32  luma_height;
     Uint32  chroma_width;
     Uint32  chroma_height;
     Uint32  luma_size;
     Uint32  chroma_size;
     Int32   frame_size;
     Uint32  luma_stride;
     Uint32  chroma_stride;
     Uint32  aligned_luma_size;
     Uint32  aligned_chroma_size;
     Uint32  aligned_frame_size;
 } FrameSizeInfo;

 typedef struct StrideInfo_struct {
     Uint32  luma_stride;
     Uint32  chroma_stride;
 } StrideInfo;

 //AV1 FGN
#define FGN_DATA_PRE_PATH               "fbc_result/"
#define FGN_PIC_PARAM_DATA_NUM          93
#define FGN_PIC_PARAM_POST_PATH         "lf_film_grain_pic_param.txt"
#define FGN_MEM_POST_PATH               "lf_film_grain_mem_128.bin"
#define FGN_MEM_SIZE                    13056 // 816x16 byte
#define FGN_POINT_Y_DATA_NUM            28
#define FGN_POINT_CB_DATA_NUM           20
#define FGN_POINT_CR_DATA_NUM           20

 //FGN INFO
#define FGN_INFO_APPLY_GRAIN                    9
#define FGN_INFO_GRAIN_SEED                     10
#define FGN_INFO_CHROMA_SCALING_FROM_LUMA       11
#define FGN_INFO_OVERLAP_FLAG                   12
#define FGN_INFO_CLIP_TO_RESTRICTED_RANGE       13
#define FGN_INFO_MATRIX_COEFFICIENTS            14
#define FGN_INFO_SCALING_SHIFT_MINUS8           15

#define FGN_INFO_CB_MULT                        16
#define FGN_INFO_CB_LUMA_MULT                   17

#define FGN_INFO_CB_OFFSET                      18
#define FGN_INFO_CR_MULT                        19
#define FGN_INFO_CR_LUMA_MULT                   20

#define FGN_INFO_CR_OFFSET                      21
#define FGN_INFO_NUM_Y_POINTS                   22
#define FGN_INFO_NUM_CB_POINTS                  23
#define FGN_INFO_NUM_CR_POINTS                  24

#define FGN_INFO_POINT_Y_VALUE                  25

#define FGN_INFO_POINT_CB_VALUE                 53

#define FGN_INFO_POINT_CR_VALUE                 73

 typedef struct FgnInputData_struct {
     Int32 fgn_pic_param_data[FGN_PIC_PARAM_DATA_NUM];
     vpu_buffer_t fgn_pic_param_buf;
     vpu_buffer_t fgn_mem_buf;
 } FgnInputData;


 typedef struct TestMapConvConfig_struct {
     char                       inputPath[MAX_FILE_PATH];
     char                       outputPath[MAPI_TEST_MAX_OUTPUT_PATH][MAX_FILE_PATH];
     char                       fbc_dir[MAX_FILE_PATH];
     char                       ref_path[MAPI_TEST_MAX_OUTPUT_PATH][MAX_FILE_PATH];
     osal_file_t                fp_input[MAPI_TEST_MAX_INPUT_PATH];
     osal_file_t                fp_output[MAPI_TEST_MAX_OUTPUT_PATH];
     osal_file_t                fp_ref[MAPI_TEST_MAX_OUTPUT_PATH];
     InputCtrlMode              input_ctrl_mode;
     OutputCtrlMode             output_ctrl_mode;
     MapiFormatProperty         input_format_property;
     MapiFormatProperty         output_format_property[MAPI_TEST_MAX_OUTPUT_PATH];
     char                       fbc_chroma_data_path[MAX_FILE_PATH];
     char                       fbc_luma_table_path[MAX_FILE_PATH];
     char                       fbc_chroma_table_path[MAX_FILE_PATH];
     Int32                      forceOutNum;
     Int32                      bitFormat;
     BitStreamMode              bitstreamMode;
     Uint32                     width;
     Uint32                     height;
     Uint32                     frame_idx;
     Int32                      input_fb_num;
     Int32                      output_fb_num;
     FrameMapConvBufferFormat   input_buf_format;
     FrameMapConvBufferFormat   output_buf_format[MAPI_TEST_MAX_OUTPUT_PATH];
     MAPIMode                   out_mode[MAPI_TEST_MAX_OUTPUT_PATH];
     FrameBuffer                in_frame_buffer[MAPI_TEST_MAX_NUM_INPUT_BUF];
     FrameBuffer                in_compressed_fb[MAPI_TEST_MAX_INPUT_PATH];
     FrameBuffer                out_frame_buffer[MAPI_TEST_MAX_OUTPUT_PATH][MAPI_TEST_MAX_NUM_OUTPUT_BUF];
     vpu_buffer_t               in_vpu_buf[MAPI_TEST_MAX_NUM_INPUT_BUF];
     vpu_buffer_t               out_vpu_buf[MAPI_TEST_MAX_OUTPUT_PATH][MAPI_TEST_MAX_NUM_OUTPUT_BUF];
     TestMAPIAPICtrl            mapi_ctrl;
     Int32                      coreIdx;
     ProductId                  productId;
     BOOL                       enableCrop;
     BOOL                       in_nv21;
     BOOL                       out_nv21[MAPI_TEST_MAX_OUTPUT_PATH];
     BOOL                       isMSB;
     BOOL                       is3P4B;
     EndianMode                 inputEndian;
     EndianMode                 outputEndian;
     BOOL                       enable_fgn;
     FgnInputData               fgn_input_data;
     Int32                      compareType;
     Int32                      bilinear_mode;
     Int32                      scaler_mode;
     Uint32                     scl_width[MAPI_TEST_MAX_OUTPUT_PATH];
     Uint32                     scl_height[MAPI_TEST_MAX_OUTPUT_PATH];
     Uint32                     input_display_2port;
     Uint32                     output_display_2port;
     Uint32                     is_offset_tb_64;
     Uint32                     is_monochrome;
     Uint32                     cf10_10b_to_8b;
 } TestMapConvConfig;

#endif /* NEW_MAP_CONV_H */

