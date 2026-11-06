#include <linux/kernel.h>
#include <linux/mm.h>
#include <linux/interrupt.h>
#include <linux/ioport.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/dma-mapping.h>
#include "jpuapi.h"
#include "jpeg_api.h"
#include "jpulog.h"
#include "jpu_helper.h"
#include "jpuapifunc.h"
#include "vb.h"
#include "vbq.h"
#include "bind.h"
#include "ion.h"
#include "datastructure.h"

#ifdef MAX
#undef MAX
#endif
#define MAX(a, b) (((a) > (b)) ? (a) : (b))

#ifdef CLIP3
#undef CLIP3
#endif
#define CLIP3(min, max, x) ((x) < (min) ? (min) : ((x) > (max) ? (max) : (x)))

#define BS_SIZE_ALIGNMENT           4096
#define MIN_BS_SIZE                 8192
#define ENC_SRC_BUF_NUM             1
#define HISTORY_SIZE                10
#define JPEG_HEADER_BUFFER_SIZE     1000

typedef struct {
    int rcmode;
    int first_frame_qs;
    long bitrate;
    int framerate;
    int qfactor_min;
    int qfactor_max;
    int frame_idx;
    int predict_frame_size;

    int last_qfactor;
    int last_frame_predict_size;            // unit: bits
    int bit_error;
    int history_frame_qfactor[HISTORY_SIZE];
    int history_frame_size[HISTORY_SIZE];   // unit: bits

    int qp2bits_coeff;
    int before_change_bitrate;
    int before_change_framerate;
} jpeg_rc_context;

typedef struct {
    jpu_buffer_t        buffer;
    size_t              stream_len;
}jpeg_stream_info;

typedef struct {
    JpgEncHandle        handle;
    jpu_buffer_t        stream_buffer;
    jpu_buffer_t        header_buffer;
    jpeg_stream_info    stream_buffer_ex;//for stream buffer for
    JpgEncOpenParam     open_param;
    JpgEncParamSet      header_param;
    JpgEncOutputInfo    output_info;
    EncConfigParam      *config;
    int                 header_mode;
    int                 core_idx;
    int                 valid_cnt;
    jpeg_rc_context     rc_context;
}JPEG_ENC_HANDLE;

#define MAX_NUM_FRAME 32
typedef struct frame_buffer_pool{
    FrameBuffer buffer[MAX_NUM_FRAME];
    int size[MAX_NUM_FRAME];
    char is_external[MAX_NUM_FRAME];
    vb_blk blk[MAX_NUM_FRAME];
    struct mutex mutex;
}frame_buffer_pool_t;

typedef struct {
    JpgDecHandle        handle;
    jpu_buffer_t        stream_buffer;
    jpu_buffer_t        external_fb;
    FrameBuffer         frame_buffer;
    JpgDecOpenParam     open_param;
    JpgDecOutputInfo    output_info;
    JpgDecInitialInfo   initial_info;
    int                 out_num;
    int                 hor_scale_mode;
    int                 ver_scale_mode;
    int                 core_idx;
    int                 valid_cnt;
    char                is_external_mem;
    char                is_registered_buffer;
    int                 decode_width;
    int                 decode_height;
    int                 max_frame_width;
    int                 max_frame_height;
    int                 min_frame_width;
    int                 min_frame_height;
    int                 timeout;
    Queue               *disp_frame;
    frame_buffer_pool_t frame_buffer_pool;
    char                is_bind_mode;
    int                 channel_index;
    int                 frame_num;
    mmf_bind_dest_s     bind_dst;
    struct vb_jobs_t    jobs;
} JPEG_DEC_HANDLE;

extern int irq_status[MAX_NUM_JPU_CORE];
extern int jpu_core_irq_count[MAX_NUM_JPU_CORE];
extern int jpu_enable_irq(int coreidx);

static int jpeg_calc_start_qfactor(int target_bit, int total_mb)
{
    static int mb_num[9] = {
        0,     200,   700,   1200,
        2000,  4000,  8000,  16000,
        20000
    };

    static int tab_bit[9] = {
        3780,  3570,  3150,  2940,
        2730,  3780,  2100,  1680,
        2100
    };

    static unsigned char qscale2qp[96] = {
        15,  15,  15,  15,  15,  16,  18,  20,  21,  22,  23,  24,
        25,  25,  26,  27,  28,  28,  29,  29,  30,  30,  30,  31,
        31,  32,  32,  33,  33,  33,  34,  34,  34,  34,  35,  35,
        35,  36,  36,  36,  36,  36,  37,  37,  37,  37,  38,  38,
        38,  38,  38,  39,  39,  39,  39,  39,  39,  40,  40,  40,
        40,  41,  41,  41,  41,  41,  41,  41,  42,  42,  42,  42,
        42,  42,  42,  42,  43,  43,  43,  43,  43,  43,  43,  43,
        44,  44,  44,  44,  44,  44,  44,  44,  45,  45,  45,  45,
    };

    int cnt = 0;
    int index;
    int i;

    for (i = 0; i < 8; i++) {
        if (mb_num[i] > total_mb)
            break;
        cnt++;
    }

    index = (total_mb * tab_bit[cnt] - 350) / target_bit;
    index = CLIP3(4, 95, index);

    return qscale2qp[index];
}

static int jpeg_bits_2_qfactor(void *rc_context, int bits)
{
    jpeg_rc_context *pst_rc_handle = rc_context;

    return bits / pst_rc_handle->qp2bits_coeff;
}

static void jpeg_rc_open(void *handle)
{
    JPEG_ENC_HANDLE *pst_handle = handle;
    JpgEncOpenParam *pst_open_param = &pst_handle->open_param;
    jpeg_rc_context *pst_rc_handle = &pst_handle->rc_context;
    int mb_width = JPU_CEIL(16, pst_open_param->picWidth) / 16;
    int mb_height = JPU_CEIL(16, pst_open_param->picHeight) / 16;

    pst_rc_handle->framerate = pst_open_param->framerate;

    pst_rc_handle->before_change_bitrate = pst_open_param->bitrate;
    pst_rc_handle->before_change_framerate = pst_open_param->framerate;
    pst_rc_handle->bitrate =
            MAX(pst_open_param->bitrate * 1000 - pst_rc_handle->framerate * JPEG_HEADER_BUFFER_SIZE
                , pst_rc_handle->framerate * JPEG_HEADER_BUFFER_SIZE);
    pst_rc_handle->qfactor_min = pst_open_param->qfactor_min;
    pst_rc_handle->qfactor_max = pst_open_param->qfactor_max;
    pst_rc_handle->bit_error = 0;
    pst_rc_handle->frame_idx = 0;

    pst_rc_handle->qp2bits_coeff = 1;
    pst_rc_handle->predict_frame_size = pst_rc_handle->bitrate / pst_open_param->framerate;
    pst_rc_handle->first_frame_qs = jpeg_calc_start_qfactor(pst_rc_handle->predict_frame_size, mb_width * mb_height);

    JLOG(INFO,"rc open bitrate:%d, body bitrate:%ld fps:%d, qfactor:[%d, %d]\n"
        , pst_open_param->bitrate, pst_rc_handle->bitrate, pst_rc_handle->framerate
        , pst_rc_handle->qfactor_min, pst_rc_handle->qfactor_max);
}


static void jpeg_rc_reset(void *handle)
{
    JPEG_ENC_HANDLE *pst_handle = handle;
    JpgEncOpenParam *pst_open_param = &pst_handle->open_param;
    jpeg_rc_context *pst_rc_handle = &pst_handle->rc_context;

    pst_rc_handle->framerate = pst_open_param->framerate;
    pst_rc_handle->before_change_bitrate = pst_open_param->bitrate;
    pst_rc_handle->before_change_framerate = pst_open_param->framerate;
    pst_rc_handle->bitrate =
            MAX(pst_open_param->bitrate * 1000 - pst_rc_handle->framerate * JPEG_HEADER_BUFFER_SIZE
                , pst_rc_handle->framerate * JPEG_HEADER_BUFFER_SIZE);

    pst_rc_handle->bit_error = CLIP3(-pst_rc_handle->bitrate / 2,  pst_rc_handle->bitrate / 2, pst_rc_handle->bit_error);
    pst_rc_handle->predict_frame_size = pst_rc_handle->bitrate / pst_open_param->framerate;
}

static int jpeg_rc_estimate_pic_qs(JPEG_ENC_HANDLE *handle)
{
    JPEG_ENC_HANDLE *pst_handle = handle;
    jpeg_rc_context *pst_rc_handle = &pst_handle->rc_context;
    int curr_size = 0, curr_qfactor = 1;
    int smooth_window = pst_rc_handle->framerate;

    do {
        if (pst_rc_handle->frame_idx == 0) {
            curr_qfactor = pst_rc_handle->first_frame_qs;
            break;
        }

        curr_size = pst_rc_handle->predict_frame_size + (pst_rc_handle->bit_error / smooth_window);
        curr_qfactor = CLIP3(pst_rc_handle->qfactor_min
                    , pst_rc_handle->qfactor_max, jpeg_bits_2_qfactor(pst_rc_handle, curr_size));
    } while (0);

    pst_rc_handle->last_qfactor = curr_qfactor;
    JLOG(INFO, "estimate frameidx:%d, framesize:%d, qfactor:%d\n", pst_rc_handle->frame_idx, curr_size, curr_qfactor);
    return curr_qfactor;
}

static int jpeg_rc_update_pic_info(JPEG_ENC_HANDLE *handle, int last_encoded_bits)
{
    JPEG_ENC_HANDLE *pst_handle = handle;
    jpeg_rc_context *pst_rc_handle = &pst_handle->rc_context;
    int history_index = 0;
    int sum_qp = 0, sum_bits = 0;
    int i = 0;

    history_index = (pst_rc_handle->frame_idx % HISTORY_SIZE);
    pst_rc_handle->history_frame_qfactor[history_index] = pst_rc_handle->last_qfactor;
    pst_rc_handle->history_frame_size[history_index] = last_encoded_bits;

    do {
        // first frame
        if (pst_rc_handle->frame_idx == 0) {
            pst_rc_handle->qp2bits_coeff = last_encoded_bits / pst_rc_handle->last_qfactor;
            break;
        }

        for (i = 0; i < HISTORY_SIZE; ++i) {
            sum_qp += pst_rc_handle->history_frame_qfactor[i];
            sum_bits += pst_rc_handle->history_frame_size[i];
        }

        pst_rc_handle->qp2bits_coeff = sum_bits / sum_qp;
    } while (0);

    JLOG(INFO, "update frameidx:%d, encoded bits:%d, coeff:%d\n"
        , pst_rc_handle->frame_idx, last_encoded_bits, pst_rc_handle->qp2bits_coeff);

    pst_rc_handle->bit_error += (pst_rc_handle->predict_frame_size - last_encoded_bits);
    pst_rc_handle->bit_error = CLIP3(-pst_rc_handle->bitrate / 2,  pst_rc_handle->bitrate / 2, pst_rc_handle->bit_error);
    pst_rc_handle->frame_idx++;

    return 0;
}

static int update_bind_mode(JPEG_DEC_HANDLE *pst_handle)
{
    int ret;
    mmf_chn_s src_chn = {0};

    src_chn.mod_id = ID_VDEC;
    src_chn.dev_id = 0;
    src_chn.chn_id = pst_handle->channel_index;
    ret = bind_get_dst(&src_chn, &pst_handle->bind_dst);
    if (ret == 0)
        pst_handle->is_bind_mode = 1;

    return JPG_RET_SUCCESS;
}

static int fill_vbbuffer(JPEG_DEC_HANDLE *pst_handle, FrameBuffer frame_buffer)
{
    vb_blk blk;
    struct vb_s *vb;
    struct video_buffer *vb_buf;
    mmf_chn_s src_chn = {0};

    src_chn.mod_id = ID_VDEC;
    src_chn.dev_id = 0;
    src_chn.chn_id = pst_handle->channel_index;

    blk = vb_phys_addr2handle(frame_buffer.bufY);
    vb = (struct vb_s *)blk;
    vb_buf = &vb->buf;
    atomic_fetch_add(1, &vb->usr_cnt);

    memset(vb_buf, 0, sizeof(struct video_buffer));
    if (pst_handle->ver_scale_mode ||
        pst_handle->hor_scale_mode ||
        pst_handle->open_param.rotation == 90 ||
        pst_handle->open_param.rotation == 270) {
        vb_buf->size.width = pst_handle->output_info.decPicWidth;
        vb_buf->size.height = pst_handle->output_info.decPicHeight;
    } else {
        vb_buf->size.width = pst_handle->initial_info.picWidth;
        vb_buf->size.height = pst_handle->initial_info.picHeight;
    }

    if (frame_buffer.format == FORMAT_400) {
        vb_buf->pixel_format = PIXEL_FORMAT_YUV_400;
    } else if (frame_buffer.format == FORMAT_422) {
        if (pst_handle->open_param.packedFormat == PACKED_FORMAT_422_YUYV) {
            vb_buf->pixel_format = PIXEL_FORMAT_YUYV;
        } else if(pst_handle->open_param.packedFormat == PACKED_FORMAT_422_UYVY) {
            vb_buf->pixel_format = PIXEL_FORMAT_UYVY;
        } else if(pst_handle->open_param.packedFormat == PACKED_FORMAT_422_YVYU) {
            vb_buf->pixel_format = PIXEL_FORMAT_YVYU;
        } else if(pst_handle->open_param.packedFormat == PACKED_FORMAT_422_VYUY) {
            vb_buf->pixel_format = PIXEL_FORMAT_VYUY;
        } else if (pst_handle->open_param.packedFormat == PACKED_FORMAT_NONE){
            if(pst_handle->open_param.chromaInterleave == CBCR_INTERLEAVE) {
                vb_buf->pixel_format = PIXEL_FORMAT_NV16;
            } else if(pst_handle->open_param.chromaInterleave == CRCB_INTERLEAVE) {
                vb_buf->pixel_format = PIXEL_FORMAT_NV61;
            } else
                vb_buf->pixel_format = PIXEL_FORMAT_YUV_PLANAR_422;
        }
    } else if (frame_buffer.format == FORMAT_444) {
        vb_buf->pixel_format = PIXEL_FORMAT_YUV_PLANAR_444;
    } else {//frame_buffer.format == FORMAT_420
        if (pst_handle->open_param.chromaInterleave == CBCR_INTERLEAVE) {
            vb_buf->pixel_format = PIXEL_FORMAT_NV12;
        } else if (pst_handle->open_param.chromaInterleave == CRCB_INTERLEAVE) {
            vb_buf->pixel_format = PIXEL_FORMAT_NV21;
        } else {
            vb_buf->pixel_format = PIXEL_FORMAT_YUV_PLANAR_420;
        }
    }

    vb_buf->stride[0] = frame_buffer.stride;
    vb_buf->phy_addr[0] = frame_buffer.bufY;
    vb_buf->length[0] = frame_buffer.stride * frame_buffer.fbLumaHeight;
    vb_buf->stride[1] = frame_buffer.strideC;
    vb_buf->phy_addr[1] = frame_buffer.bufCb;
    vb_buf->length[1] = frame_buffer.strideC * frame_buffer.fbChromaHeight;
    if (!pst_handle->open_param.chromaInterleave) {
        vb_buf->phy_addr[2] = frame_buffer.bufCr;
        vb_buf->length[2] = frame_buffer.strideC * frame_buffer.fbChromaHeight;
    }

    vb_buf->compress_mode = COMPRESS_MODE_NONE;
    vb_buf->sequence = pst_handle->frame_num;
    vb_buf->frm_num = pst_handle->frame_num;

    vb_done_handler(src_chn, CHN_TYPE_OUT, &pst_handle->jobs, blk);

    return JPG_RET_SUCCESS;
}

static int _insert_frame_buffer(drv_jpg_handle handle, FrameBuffer buffer, int buffer_size, vb_blk blk, char is_external)
{
    int idx;
    JPEG_DEC_HANDLE *pst_handle = handle;

    mutex_lock(&pst_handle->frame_buffer_pool.mutex);

    for (idx=0; idx<MAX_NUM_FRAME; idx++) {
        if (!pst_handle->frame_buffer_pool.size[idx]) {
            memcpy(&pst_handle->frame_buffer_pool.buffer[idx], &buffer, sizeof(FrameBuffer));
            pst_handle->frame_buffer_pool.size[idx] = buffer_size;
            pst_handle->frame_buffer_pool.blk[idx] = blk;
            pst_handle->frame_buffer_pool.is_external[idx] = is_external;
            break;
        }
    }

    mutex_unlock(&pst_handle->frame_buffer_pool.mutex);

    if (idx == MAX_NUM_FRAME) {
        JLOG(ERR, "core:%d, frame buffer pool is full\n", pst_handle->core_idx);
        return -1;
    }

    return idx;
}

static int _find_frame_buffer(drv_jpg_handle handle, PhysicalAddress addr)
{
    int i;
    JPEG_DEC_HANDLE *pst_handle = handle;

    mutex_lock(&pst_handle->frame_buffer_pool.mutex);

    for (i=0; i<MAX_NUM_FRAME; i++) {
        if (pst_handle->frame_buffer_pool.size[i] &&
            pst_handle->frame_buffer_pool.buffer[i].bufY == addr) {
            break;
        }
    }

    mutex_unlock(&pst_handle->frame_buffer_pool.mutex);

    if (i == MAX_NUM_FRAME) {
        JLOG(ERR, "invalid addr:0x%llx\n", addr);
        return -1;
    }

    return i;
}

static int _free_frame_buffer(drv_jpg_handle handle, int frame_idx)
{
    JPEG_DEC_HANDLE *pst_handle = handle;

    if (frame_idx >= MAX_NUM_FRAME) {
        JLOG(ERR, "invalid frame_idx:%d\n", frame_idx);
        return -1;
    }

    mutex_lock(&pst_handle->frame_buffer_pool.mutex);
    if (pst_handle->frame_buffer_pool.size[frame_idx]) {
        vb_release_block(pst_handle->frame_buffer_pool.blk[frame_idx]);
        memset(&pst_handle->frame_buffer_pool.buffer[frame_idx], 0, sizeof(FrameBuffer));
        pst_handle->frame_buffer_pool.size[frame_idx] = 0;
    }

    mutex_unlock(&pst_handle->frame_buffer_pool.mutex);

    return 0;
}

/* initial jpu core */
drv_jpg_handle jpeg_enc_init(void)
{
    JpgRet ret = JPG_RET_SUCCESS;
    JPEG_ENC_HANDLE *pst_handle;

    pst_handle = vzalloc(sizeof(JPEG_ENC_HANDLE));
    if (!pst_handle)
        return NULL;

    ret = JPU_Init();
    if (ret != JPG_RET_SUCCESS && ret != JPG_RET_CALLED_BEFORE) {
        JLOG(ERR, "JPU_Init failed Error code is 0x%x \n", ret);
        vfree(pst_handle);
        return NULL;
    }

    return pst_handle;
}

drv_jpg_handle jpeg_dec_init(void)
{
    JpgRet ret = JPG_RET_SUCCESS;
    JPEG_DEC_HANDLE *pst_handle;

    pst_handle = vzalloc(sizeof(JPEG_DEC_HANDLE));
    if (!pst_handle)
        return NULL;

    ret = JPU_Init();
    if (ret != JPG_RET_SUCCESS && ret != JPG_RET_CALLED_BEFORE) {
        JLOG(ERR, "JPU_Init failed Error code is 0x%x \n", ret);
        vfree(pst_handle);
        return NULL;
    }

    return pst_handle;
}


/* uninitial jpu core */
void jpeg_enc_deinit(drv_jpg_handle handle)
{
    JPEG_ENC_HANDLE *pst_handle = handle;

    if (!pst_handle)
        return ;

    if (JPU_IsInit()) {
        JPU_DeInit();
    }

    vfree(pst_handle);
}

void jpeg_dec_deinit(drv_jpg_handle handle)
{
    JPEG_DEC_HANDLE *pst_handle = handle;

    if (!pst_handle)
        return ;

    if (JPU_IsInit()) {
        JPU_DeInit();
    }

    vfree(pst_handle);
}

static void _calc_slice_height(JpgEncOpenParam* open_param, Uint32 slice_height)
{
    Uint32 width    = open_param->picWidth;
    Uint32 height   = open_param->picHeight;
    Uint32 aligned_buf_height;
    FrameFormat format = open_param->sourceFormat;

    if (open_param->rotation == 90 || open_param->rotation == 270) {
        width  = open_param->picHeight;
        height = open_param->picWidth;
        if (format == FORMAT_422)
            format = FORMAT_440;
        else if (format == FORMAT_440)
            format = FORMAT_422;
    }

    if (format == FORMAT_420 || format == FORMAT_440)
        aligned_buf_height = JPU_CEIL(16, height);
    else
        aligned_buf_height = JPU_CEIL(8, height);

    if (slice_height == 0) {
        if (format == FORMAT_420 || format == FORMAT_440)
            open_param->sliceHeight = aligned_buf_height;
        else
            open_param->sliceHeight = aligned_buf_height;
    }
    else
        open_param->sliceHeight = slice_height;

    if (open_param->sliceHeight != aligned_buf_height) {
        if (format == FORMAT_420 || format == FORMAT_422)
            open_param->restartInterval = (width+15)/16;
        else
            open_param->restartInterval = (width+7)/8;

        if (format == FORMAT_420 || format == FORMAT_440)
            open_param->restartInterval *= (open_param->sliceHeight/16);
        else
            open_param->restartInterval *= (open_param->sliceHeight/8);
        open_param->sliceInstMode = TRUE;
    }

}

int jpeg_enc_open(drv_jpg_handle handle, drv_jpg_config config)
{
    JpgRet ret = JPG_RET_FAILURE;
    JPEG_ENC_HANDLE *pst_handle = handle;

    if(!pst_handle)
        return ret;

    pst_handle->config = vzalloc(sizeof(EncConfigParam));
    if (pst_handle->config == NULL) {
        JLOG(ERR, "no memory for pstEncConfig\n");
        goto ERR_ENC_INIT;
    }

    pst_handle->config->picWidth = config.u.enc.picWidth;
    pst_handle->config->picHeight = config.u.enc.picHeight;
    pst_handle->config->sourceSubsample = config.u.enc.sourceFormat;
    pst_handle->config->packedFormat = config.u.enc.packedFormat;
    pst_handle->config->chromaInterleave = config.u.enc.chromaInterleave;
    pst_handle->config->rotation = config.u.enc.rotAngle * 90;
    pst_handle->config->mirror = config.u.enc.mirDir;
    if(config.u.enc.bitstreamBufSize <= MIN_BS_SIZE) {
        if(config.u.enc.external_bs_addr)
        {
            JLOG(ERR, "bitstream size: %d is smaller the MIN_BS_SIZE: %d\n", config.u.enc.bitstreamBufSize, MIN_BS_SIZE);
            goto ERR_ENC_INIT;
        } else {
            config.u.enc.bitstreamBufSize = STREAM_BUF_SIZE;
        }
    }
    pst_handle->config->bsSize = config.u.enc.bitstreamBufSize;
    if (pst_handle->config->bsSize  % BS_SIZE_ALIGNMENT) {
        JLOG(ERR, "Invalid size of bitstream buffer %u\n", pst_handle->config->bsSize);
        goto ERR_ENC_INIT;
    }

    if(config.u.enc.packedFormat) {
        if (config.u.enc.packedFormat == DRV_PACKED_FORMAT_444 &&
            config.u.enc.sourceFormat != DRV_FORMAT_444) {
            JLOG(ERR, "Invalid operation mode : In case of using packed mode. sourceFormat must be FORMAT_444\n" );
            goto ERR_ENC_INIT;
        }
    }

    if (getJpgEncOpenParamDefault(&pst_handle->open_param, pst_handle->config) == FALSE) {
        JLOG(ERR, "getJpgEncOpenParamDefault falied\n");
        goto ERR_ENC_INIT;
    }

    pst_handle->open_param.bitrate = config.u.enc.bitrate;
    pst_handle->open_param.framerate = config.u.enc.framerate;
    pst_handle->open_param.quality = config.u.enc.quality;
    pst_handle->open_param.qfactor_min = config.u.enc.qmin;
    pst_handle->open_param.qfactor_max = config.u.enc.qmax;
    pst_handle->stream_buffer.size = pst_handle->config->bsSize;

    if(!config.u.enc.external_bs_addr){
        pst_handle->stream_buffer.is_cached = 0;
        if (jdi_allocate_dma_memory(&pst_handle->stream_buffer) < 0) {
            JLOG(ERR, "fail to allocate bitstream buffer\n" );
            goto ERR_ENC_INIT;
        }

        if (jdi_invalidate_cache(&pst_handle->stream_buffer) < 0) {
            JLOG(ERR, "fail to invalidate bitstream buffer\n");
            goto ERR_ENC_INIT;
        }
    } else {
        pst_handle->stream_buffer.phys_addr = config.u.enc.external_bs_addr;
        pst_handle->stream_buffer.virt_addr = (unsigned long)phys_to_virt(config.u.enc.external_bs_addr);
        pst_handle->stream_buffer.base = 0;
        pst_handle->stream_buffer.is_cached = 1;  // external memory must be invalidate/flush
        jdi_insert_external_memory(&pst_handle->stream_buffer);
    }

    pst_handle->header_buffer.size = JPEG_HEADER_BUFFER_SIZE;
    pst_handle->header_buffer.is_cached = 0;
    if (jdi_allocate_dma_memory(&pst_handle->header_buffer) < 0) {
        JLOG(ERR, "fail to allocate jpeg header bitstream buffer\n" );
        goto ERR_ENC_INIT;
    }

    if (jdi_invalidate_cache(&pst_handle->header_buffer) < 0) {
        JLOG(ERR, "fail to invalidate jpeg header bitstream buffer\n");
        goto ERR_ENC_INIT;
    }
    pst_handle->header_param.pParaSet = (BYTE *)pst_handle->header_buffer.virt_addr;
    pst_handle->header_param.size = pst_handle->header_buffer.size;

    pst_handle->open_param.intrEnableBit = ((1<<INT_JPU_DONE) | (1<<INT_JPU_ERROR) | (1<<INT_JPU_BIT_BUF_FULL));
    if (pst_handle->config->sliceInterruptEnable)
        pst_handle->open_param.intrEnableBit |= (1<<INT_JPU_SLICE_DONE);
    pst_handle->open_param.streamEndian          = pst_handle->config->StreamEndian;
    pst_handle->open_param.frameEndian           = pst_handle->config->FrameEndian;
    pst_handle->open_param.bitstreamBuffer       = pst_handle->stream_buffer.phys_addr;
    pst_handle->open_param.bitstreamBufferSize   = pst_handle->stream_buffer.size;
    pst_handle->open_param.pixelJustification    = pst_handle->config->pixelJustification;
    pst_handle->open_param.rotation              = pst_handle->config->rotation;
    pst_handle->open_param.mirror                = pst_handle->config->mirror;

    _calc_slice_height(&pst_handle->open_param, pst_handle->config->sliceHeight);
    if ((ret=JPU_EncOpen((JpgEncHandle *)&pst_handle->handle, &pst_handle->open_param)) != JPG_RET_SUCCESS) {
        JLOG(ERR, "JPU_EncOpen failed Error code is 0x%x \n", ret);
        goto ERR_ENC_INIT;
    }

    pst_handle->header_mode = ENC_HEADER_MODE_NORMAL;
    pst_handle->config->encQualityPercentage = config.u.enc.quality ? config.u.enc.quality : 99;

    JPU_EncGiveCommand(pst_handle->handle, SET_JPG_USE_STUFFING_BYTE_FF, &pst_handle->config->bEnStuffByte);
    if (pst_handle->config->encQualityPercentage > 0)  {
        JPU_EncGiveCommand(pst_handle->handle, SET_JPG_QUALITY_FACTOR, &pst_handle->config->encQualityPercentage);
    }

    JPU_EncGiveCommand(pst_handle->handle, GET_JPG_QUALITY_TABLE, &pst_handle->config->encQualityPercentage);

    pst_handle->output_info.encodeState = -1;

    // start rc
    if (pst_handle->open_param.bitrate != 0) {
        jpeg_rc_open(pst_handle);
    }

    return ret;

ERR_ENC_INIT:

    if (pst_handle->stream_buffer.phys_addr) {
        if(config.u.enc.external_bs_addr)
            jdi_remove_external_memory(&pst_handle->stream_buffer);
        else
            jdi_free_dma_memory(&pst_handle->stream_buffer);
    }

    if (pst_handle->header_buffer.phys_addr)
        jdi_free_dma_memory(&pst_handle->header_buffer);

    if (pst_handle->config)
        vfree(pst_handle->config);

    return ret;
}

int jpeg_dec_open(drv_jpg_handle handle, drv_jpg_config config)
{
    JpgRet ret = JPG_RET_FAILURE;
    JPEG_DEC_HANDLE* pst_handle = handle;

    if (!pst_handle)
        return ret;

    if(config.u.dec.iDataLen < MIN_BS_SIZE)
        pst_handle->stream_buffer.size = STREAM_BUF_SIZE;
    else
        pst_handle->stream_buffer.size = config.u.dec.iDataLen;

    pst_handle->stream_buffer.size = ALIGN(pst_handle->stream_buffer.size, BS_SIZE_ALIGNMENT);
    pst_handle->stream_buffer.is_cached = 0;
    pst_handle->channel_index = config.s32ChnNum;
    pst_handle->out_num           = 1;
    pst_handle->output_info.indexFrameDisplay = -1;
    pst_handle->hor_scale_mode    = config.u.dec.iHorScaleMode;
    pst_handle->ver_scale_mode    = config.u.dec.iVerScaleMode;
    pst_handle->disp_frame = Queue_Create_With_Lock(MAX_NUM_FRAME, sizeof(DRVFRAMEBUF));
    if (pst_handle->disp_frame == NULL)
        return ret;

    if(config.u.dec.external_bs_addr && config.u.dec.external_bs_size)
    {
        memset(&pst_handle->stream_buffer, 0, sizeof(jpu_buffer_t));
        pst_handle->stream_buffer.phys_addr = config.u.dec.external_bs_addr;
        pst_handle->stream_buffer.size = config.u.dec.external_bs_size;
        pst_handle->stream_buffer.virt_addr = (unsigned long)phys_to_virt(config.u.dec.external_bs_addr);
        pst_handle->stream_buffer.base = 0;   // it will not free in dec_close.
        pst_handle->stream_buffer.is_cached = 1;  // external memory must be invalidate/flush
        jdi_insert_external_memory(&pst_handle->stream_buffer);
    } else {
        if (jdi_allocate_dma_memory(&pst_handle->stream_buffer) < 0) {
            JLOG(ERR, "fail to allocate bitstream buffer size:%ld\n", pst_handle->stream_buffer.size);
            goto ERR_DEC_JPU_OPEN;
        }
        if (jdi_invalidate_cache(&pst_handle->stream_buffer) < 0) {
            JLOG(ERR, "fail to invalidate bitstream buffer cache addr:%lx\n", pst_handle->stream_buffer.phys_addr);
            goto ERR_DEC_JPU_OPEN;
        }
    }

    memset(&pst_handle->external_fb, 0, sizeof(jpu_buffer_t));
    if(config.u.dec.external_fb_addr && config.u.dec.external_fb_size)
    {
        pst_handle->external_fb.phys_addr = config.u.dec.external_fb_addr;
        pst_handle->external_fb.size = config.u.dec.external_fb_size;
        pst_handle->external_fb.virt_addr = (unsigned long)phys_to_virt(config.u.dec.external_fb_addr);
        pst_handle->external_fb.base = 0;
    }

    pst_handle->open_param.streamEndian          = JPU_STREAM_ENDIAN;
    pst_handle->open_param.frameEndian           = JPU_FRAME_ENDIAN;
    pst_handle->open_param.bitstreamBuffer       = pst_handle->stream_buffer.phys_addr;
    pst_handle->open_param.bitstreamBufferSize   = pst_handle->stream_buffer.size;
    pst_handle->open_param.pBitStream            = (BYTE *)pst_handle->stream_buffer.virt_addr;
    pst_handle->open_param.chromaInterleave      = config.u.dec.dec_buf.chromaInterleave;
    pst_handle->open_param.packedFormat          = PACKED_FORMAT_NONE;
    pst_handle->open_param.roiEnable             = config.u.dec.roiEnable;
    pst_handle->open_param.roiOffsetX            = config.u.dec.roiOffsetX;
    pst_handle->open_param.roiOffsetY            = config.u.dec.roiOffsetY;
    pst_handle->open_param.roiWidth              = config.u.dec.roiWidth;
    pst_handle->open_param.roiHeight             = config.u.dec.roiHeight;
    pst_handle->open_param.rotation              = config.u.dec.rotAngle;
    pst_handle->open_param.mirror                = config.u.dec.mirDir;
    pst_handle->open_param.outputFormat          = config.u.dec.dec_buf.format;
    pst_handle->open_param.intrEnableBit         = ((1<<INT_JPU_DONE) | (1<<INT_JPU_ERROR) | (1<<INT_JPU_BIT_BUF_EMPTY));

    ret = JPU_DecOpen(&pst_handle->handle, &pst_handle->open_param);
    if( ret != JPG_RET_SUCCESS ) {
        JLOG(ERR, "JPU_DecOpen failed Error code is 0x%x \n", ret );
        goto ERR_DEC_JPU_OPEN;
    }

    base_mod_jobs_init(&pst_handle->jobs, 1, 1, 0);
    return ret;

ERR_DEC_JPU_OPEN:

    if (pst_handle->stream_buffer.phys_addr) {
        if(config.u.dec.external_bs_addr && config.u.dec.external_bs_size)
            jdi_remove_external_memory(&pst_handle->stream_buffer);
        else
            jdi_free_dma_memory(&pst_handle->stream_buffer);
    }

    if (pst_handle->disp_frame)
        Queue_Destroy(pst_handle->disp_frame);

    return ret;
}

int jpeg_dec_set_param(drv_jpg_handle handle, drv_jpg_config config)
{
    JpgRet ret = JPG_RET_FAILURE;
    JPEG_DEC_HANDLE* pst_handle = handle;

    if (!pst_handle)
        return ret;

    pst_handle->max_frame_width   = config.u.dec.max_frame_width;
    pst_handle->max_frame_height  = config.u.dec.max_frame_height;
    pst_handle->min_frame_width   = config.u.dec.min_frame_width;
    pst_handle->min_frame_height  = config.u.dec.min_frame_height;

    pst_handle->hor_scale_mode    = config.u.dec.iHorScaleMode;
    pst_handle->ver_scale_mode    = config.u.dec.iVerScaleMode;
    pst_handle->open_param.chromaInterleave      = config.u.dec.dec_buf.chromaInterleave;
    pst_handle->open_param.outputFormat          = config.u.dec.dec_buf.format;
    pst_handle->open_param.roiEnable             = config.u.dec.roiEnable;
    pst_handle->open_param.roiOffsetX            = config.u.dec.roiOffsetX;
    pst_handle->open_param.roiOffsetY            = config.u.dec.roiOffsetY;
    pst_handle->open_param.roiWidth              = config.u.dec.roiWidth;
    pst_handle->open_param.roiHeight             = config.u.dec.roiHeight;
    pst_handle->open_param.rotation              = config.u.dec.rotAngle;
    pst_handle->open_param.mirror                = config.u.dec.mirDir;

    ret = JPU_DecSetParam(pst_handle->handle, &pst_handle->open_param);
    if( ret != JPG_RET_SUCCESS ) {
        JLOG(ERR, "JPU_DecSetOpenParam failed Error code is 0x%x \n", ret );
        return ret;
    }

    return ret;
}

/* close and free alloced jpu handle */
int jpeg_enc_close(drv_jpg_handle handle)
{
    int ret = JPG_RET_SUCCESS;
    JPEG_ENC_HANDLE *pst_handle = (JPEG_ENC_HANDLE *)handle;

    /* close instance handle */
    if (NULL == pst_handle) {
        JLOG(ERR,"handle = NULL\n");
        return -1;
    }

    /* check handle valid */
    ret = CheckJpgInstValidity((JpgEncHandle)pst_handle->handle);
    if (ret != JPG_RET_SUCCESS) {
        JLOG(INFO, "Invalid handle at cviJpgEncGetInputDataBuf.\n");
        return ret;
    }

    if(pst_handle->header_buffer.base)
        jdi_free_dma_memory(&pst_handle->header_buffer);

    if(pst_handle->stream_buffer.base)
        jdi_free_dma_memory(&pst_handle->stream_buffer);
    else
        jdi_remove_external_memory(&pst_handle->stream_buffer);

    if (JPU_EncClose(pst_handle->handle) == JPG_RET_FRAME_NOT_COMPLETE) {
        JLOG(INFO, "NOT_COMPLETE\n");
    }

    if(pst_handle->config) {
        vfree(pst_handle->config);
        pst_handle->handle = NULL;
    }

    return ret;
}

int jpeg_dec_close(drv_jpg_handle handle)
{
    int i;
    int ret = JPG_RET_SUCCESS;
    JPEG_DEC_HANDLE *pst_handle = handle;

    /* close instance handle */
    if (NULL == pst_handle) {
        JLOG(ERR,"handle = NULL\n");
        return -1;
    }

    ret = CheckJpgInstValidity((JpgDecHandle)pst_handle->handle);
    if (ret != JPG_RET_SUCCESS) {
        JLOG(INFO, "CheckJpgInstValidity fail, return %d\n", ret);
        return ret;
    }

    for (i=0; i<MAX_NUM_FRAME; i++) {
        _free_frame_buffer(handle, i);
    }

    if(pst_handle->stream_buffer.base)
        jdi_free_dma_memory(&pst_handle->stream_buffer);
    else
        jdi_remove_external_memory(&pst_handle->stream_buffer);

    if (JPU_DecClose(pst_handle->handle) == JPG_RET_FRAME_NOT_COMPLETE) {
        JLOG(INFO, "NOT_COMPLETE\n");
    }

    if (pst_handle->disp_frame)
        Queue_Destroy(pst_handle->disp_frame);

    pst_handle->handle = NULL;
    base_mod_jobs_exit(&pst_handle->jobs);

    return ret;
}

/* */
int jpeg_get_caps(drv_jpg_handle handle)
{

    return 0;
}

/* flush data */
int jpeg_flush(drv_jpg_handle handle)
{
    int ret = 0;
    if (NULL == handle)
        return -1;

    return ret;
}

static int _reopen_instance(drv_jpg_handle hadnle, DRVFRAMEBUF *data)
{
    JPEG_ENC_HANDLE *pst_handle = (JPEG_ENC_HANDLE *)data;
    JpgRet ret = JPG_RET_SUCCESS;
    JpgEncOutputInfo   info;

    ret = JPU_EncClose(pst_handle->handle);
    if (ret == JPG_RET_FRAME_NOT_COMPLETE) {
        ret = JPU_EncGetOutputInfo(pst_handle->handle, &info);
        if (ret != JPG_RET_SUCCESS) {
            JLOG(ERR, "JPU_EncGetOutputInfo failed Error code is 0x%x \n", ret);
            return -3;
        }
        ret = JPU_EncClose(pst_handle->handle);
        if (ret != JPG_RET_SUCCESS) {
            JLOG(ERR, "JPU_EncClose failed Error code is 0x%x \n", ret);
            return -3;
        }
    } else if (ret != JPG_RET_SUCCESS) {
        JLOG(ERR, "JPU_EncClose failed Error code is 0x%x \n", ret);
        return -3;
    }

    pst_handle->handle = NULL;
    pst_handle->config->packedFormat = data->packedFormat;
    pst_handle->config->sourceSubsample = data->format;
    pst_handle->config->picWidth = data->width;
    pst_handle->config->picHeight = data->height;

    if (pst_handle->config->packedFormat==PACKED_FORMAT_444 &&
        pst_handle->config->sourceSubsample != FORMAT_444) {
        JLOG(ERR, "In case of using packed mode. sourceFormat must be FORMAT_444\n" );
        return -2;
    }

    _calc_slice_height(&pst_handle->open_param, pst_handle->config->sliceHeight);
    ret=JPU_EncOpen(&pst_handle->handle, &pst_handle->open_param);
    if (ret != JPG_RET_SUCCESS) {
        JLOG(ERR, "JPU_EncOpen failed Error code is 0x%x \n", ret);
        return -3;
    }

    JPU_EncGiveCommand(pst_handle->handle, SET_JPG_USE_STUFFING_BYTE_FF, &pst_handle->config->bEnStuffByte);
    if (pst_handle->config->encQualityPercentage > 0)  {
        JPU_EncGiveCommand(pst_handle->handle, SET_JPG_QUALITY_FACTOR, &pst_handle->config->encQualityPercentage);
    }

    return JPG_RET_SUCCESS;
}

static int _process_stream_buffer_full(drv_jpg_handle handle)
{
    JPEG_ENC_HANDLE *pst_handle = (JPEG_ENC_HANDLE *)handle;
    jpu_buffer_t stream_buffer;
    PhysicalAddress rd_ptr;
    PhysicalAddress wr_ptr;
    int stream_size;
    int ex_addr;

    //allocate new ion
    stream_buffer.size = pst_handle->stream_buffer_ex.stream_len + pst_handle->stream_buffer.size * 2;
    stream_buffer.is_cached = 0;
    if (jdi_allocate_dma_memory(&stream_buffer) < 0) {
        JLOG(ERR, "sopProcessBsFull fail to allocate bitstream buffer\n" );
        return JPG_RET_FAILURE;
    }

    if (jdi_invalidate_cache(&stream_buffer) < 0) {
        JLOG(ERR, "fail to invalidate bitstream buffer cache addr:%lx\n", stream_buffer.phys_addr);
        return JPG_RET_FAILURE;
    }

    JPU_EncGetBitstreamBuffer(pst_handle->handle, &rd_ptr, &wr_ptr, &stream_size);

    if(pst_handle->stream_buffer_ex.stream_len) {
        jdi_read_memory(pst_handle->stream_buffer_ex.buffer.phys_addr,
                        (unsigned char *)stream_buffer.virt_addr,
                        pst_handle->stream_buffer_ex.stream_len,
                        pst_handle->open_param.streamEndian);

        jdi_free_dma_memory(&pst_handle->stream_buffer_ex.buffer);
    }

    JPU_GetExtAddr(pst_handle->core_idx, &ex_addr);
    rd_ptr |= (PhysicalAddress)(ex_addr) << 32;
    jdi_read_memory(rd_ptr,
                    (unsigned char *)(stream_buffer.virt_addr + pst_handle->stream_buffer_ex.stream_len),
                    stream_size,
                    pst_handle->open_param.streamEndian);

    pst_handle->stream_buffer_ex.stream_len += stream_size;
    memcpy(&pst_handle->stream_buffer_ex.buffer, &stream_buffer, sizeof(jpu_buffer_t));

    return JPG_RET_SUCCESS;
}
static int _jpeg_dump_register(int core_idx, int inst_idx)
{
    int i;

    JLOG(ERR, "------------dump core:%d register------------\n", core_idx);
    for (i=0; i<=0x250; i=i+16)
    {
        JLOG(ERR, "0x%04xh: 0x%08lx 0x%08lx 0x%08lx 0x%08lx\n", i,
            jdi_read_register_ext(core_idx, i),
            jdi_read_register_ext(core_idx, i+4),
            jdi_read_register_ext(core_idx, i+8),
            jdi_read_register_ext(core_idx, i+0xc));
    }

    return 0;
}
/* send jpu data to decode or encode */
int jpeg_enc_send_frame(drv_jpg_handle handle, DRVFRAMEBUF *data, int timeout)
{
    int ret = JPG_RET_SUCCESS;
    JpgEncParam enc_param = { 0 };
    FrameBuffer source_buffer;
    int int_reason = 0;
    JPEG_ENC_HANDLE *pst_handle;
    int enc_timeout;

    if (NULL == handle) {
        JLOG(ERR,"handle = NULL\n");
        return -1;
    }

    pst_handle = (JPEG_ENC_HANDLE *)handle;

    enc_timeout = (timeout > 0) ? timeout : JPU_INTERRUPT_TIMEOUT_MS;

    if(((FrameFormat)data->format != pst_handle->open_param.sourceFormat)
        || (data->width != pst_handle->open_param.picWidth)
        || (data->height != pst_handle->open_param.picHeight)) {
        ret = _reopen_instance(pst_handle, data);
        if (ret != JPG_RET_SUCCESS) {
        	JLOG(ERR, "sopRecreateNewInstance fail, return %d\n", ret);
        	return ret;
        }
    }

    if (pst_handle->open_param.bitrate) {
        if((pst_handle->open_param.bitrate != pst_handle->rc_context.before_change_bitrate) ||
            (pst_handle->open_param.framerate != pst_handle->rc_context.before_change_framerate) ) { //bitrate change
            jpeg_rc_reset(pst_handle);
        }
        pst_handle->config->encQualityPercentage = jpeg_rc_estimate_pic_qs(pst_handle);
        JPU_EncGiveCommand(pst_handle->handle, SET_JPG_QUALITY_FACTOR, &pst_handle->config->encQualityPercentage);
    }

    if (pst_handle->header_mode == ENC_HEADER_MODE_NORMAL) {
        pst_handle->header_param.headerMode = ENC_HEADER_MODE_NORMAL;
        pst_handle->header_param.quantMode = JPG_TBL_NORMAL;
        pst_handle->header_param.huffMode  = JPG_TBL_NORMAL;
        pst_handle->header_param.disableAPPMarker = 0;
        pst_handle->header_param.enableSofStuffing = TRUE;
        if (pst_handle->header_param.headerMode == ENC_HEADER_MODE_NORMAL) {
            JPU_EncGiveCommand(pst_handle->handle, ENC_JPG_GET_HEADER, &pst_handle->header_param);
            JLOG(INFO, "JPU_EncGiveCommand[ENC_JPG_GET_HEADER] header size=%d\n", pst_handle->header_param.size);
        }
    }

    source_buffer.bufY = data->vbY.phys_addr;
    source_buffer.bufCb = data->vbCb.phys_addr;
    source_buffer.bufCr = data->vbCr.phys_addr;
    source_buffer.stride = data->strideY;
    source_buffer.strideC = data->strideC;
    source_buffer.format = data->format;
    enc_param.sourceFrame = &source_buffer;

    pst_handle->handle->coreIndex = pst_handle->core_idx = JPU_RequestCore(JPU_INTERRUPT_TIMEOUT_MS);
    if (pst_handle->core_idx < 0)
        return JPG_RET_FAILURE;

    JPU_SetExtAddr(pst_handle->core_idx, source_buffer.bufY >> 32);

    ret = JPU_EncStartOneFrame(pst_handle->handle, &enc_param);
    if( ret != JPG_RET_SUCCESS ) {
        JLOG(ERR, "JPU_EncStartOneFrame failed Error code is 0x%x \n", ret );
        JPU_ReleaseCore(pst_handle->core_idx);
        return ret;
    }

    while(1) {
        int_reason = JPU_WaitInterrupt(pst_handle->handle, enc_timeout);
        if (int_reason == -1) {
            JLOG(ERR, "Error enc: timeout happened,core:%d inst %d, reason:%d, irq_status:%d, irq_cnt:%d\n",
            pst_handle->core_idx, pst_handle->handle->instIndex, int_reason,
            irq_status[pst_handle->core_idx],jpu_core_irq_count[pst_handle->core_idx]);
            _jpeg_dump_register(pst_handle->core_idx, pst_handle->handle->instIndex);
            ret = JPU_EncGetOutputInfo(pst_handle->handle, &pst_handle->output_info);
            JpgLeaveLock();
            JPU_ReleaseCore(pst_handle->core_idx);
            return -2; // ENC_TIMEOUT
        }
        if (int_reason == -2) {
            JLOG(ERR, "Interrupt occurred. but this interrupt is not for my instance enc\n");
            continue;
        }

        if (int_reason & (1<<INT_JPU_DONE) || int_reason & (1<<INT_JPU_SLICE_DONE)) {
            // Do no clear INT_JPU_DONE these will be cleared in JPU_EncGetOutputInfo.
            break;
        }

        if (int_reason & (1<<INT_JPU_BIT_BUF_FULL)) {
            JLOG(ERR, "INT_JPU_BIT_BUF_FULL interrupt issued INSTANCE %d \n", pst_handle->handle->instIndex);
            ret = _process_stream_buffer_full(pst_handle);
            if (ret != JPG_RET_SUCCESS) {
                JPU_SetJpgPendingInstEx(pst_handle->handle, NULL);
                JPU_ReleaseCore(pst_handle->core_idx);
                return ret;
            }

            if(!pst_handle->stream_buffer.base)
                JLOG(WARN, "external bs size: %ld is small, realloc in drv. Notice it, external addr: %lx will be invalid\n", pst_handle->stream_buffer.size, pst_handle->stream_buffer.phys_addr);

            JPU_EncUpdateBitstreamBuffer(pst_handle->handle, 0);
            JPU_ClrStatus(pst_handle->handle, (1<<INT_JPU_BIT_BUF_FULL));
            jpu_enable_irq(pst_handle->core_idx);
            continue;
        }
    }

    ret = JPU_EncGetOutputInfo(pst_handle->handle, &pst_handle->output_info);
    if (ret != JPG_RET_SUCCESS) {
        JPU_ReleaseCore(pst_handle->core_idx);
        JLOG(ERR, "JPU_EncGetOutputInfo failed Error code is 0x%x \n", ret);
        return ret;
    }

    if(pst_handle->stream_buffer_ex.stream_len) {
        jdi_read_memory(pst_handle->stream_buffer.phys_addr,
                    (unsigned char *)(pst_handle->stream_buffer_ex.buffer.virt_addr + pst_handle->stream_buffer_ex.stream_len),
                    pst_handle->output_info.bitstreamSize,
                    pst_handle->open_param.streamEndian);
        pst_handle->stream_buffer_ex.stream_len += pst_handle->output_info.bitstreamSize;
    }

    JPU_EncUpdateBitstreamBuffer(pst_handle->handle, 0);
    pst_handle->valid_cnt++;
    JPU_ReleaseCore(pst_handle->core_idx);

    jpeg_rc_update_pic_info(pst_handle, pst_handle->output_info.bitstreamSize * 8);
    return ret;
}

static int _jpeg_alloc_frame(JPEG_DEC_HANDLE *pst_handle)
{
    int temp;
    BOOL scaler_on = FALSE;
    int rotation_index;
    int decode_width;
    int decode_height;
    int luma_stride;
    int luma_height;
    int luma_size;
    int chroma_stride;
    int chroma_height;
    int chroma_size;
    int frame_size;
    FrameFormat subsample;
    vb_blk blk;

    rotation_index = pst_handle->open_param.rotation / 90;
    if (pst_handle->initial_info.sourceFormat == FORMAT_420 || pst_handle->initial_info.sourceFormat == FORMAT_422)
        decode_width = JPU_CEIL(16, pst_handle->initial_info.picWidth);
    else
        decode_width  = JPU_CEIL(8, pst_handle->initial_info.picWidth);

    if (pst_handle->initial_info.sourceFormat == FORMAT_420 || pst_handle->initial_info.sourceFormat == FORMAT_440)
        decode_height = JPU_CEIL(16, pst_handle->initial_info.picHeight);
    else
        decode_height = JPU_CEIL(8, pst_handle->initial_info.picHeight);

    decode_width >>= pst_handle->hor_scale_mode;
    decode_height >>= pst_handle->ver_scale_mode;
    if (pst_handle->open_param.packedFormat != PACKED_FORMAT_NONE &&
        pst_handle->open_param.packedFormat != PACKED_FORMAT_444) {
        // When packed format, scale-down resolution should be multiple of 2.
        decode_width  = JPU_CEIL(2, decode_width);
    }

    subsample = (pst_handle->open_param.outputFormat == FORMAT_MAX)
                ? pst_handle->initial_info.sourceFormat : pst_handle->open_param.outputFormat;

    temp = decode_width;
    decode_width = (rotation_index == 1 || rotation_index == 3) ?
                    decode_height : decode_width;
    decode_height = (rotation_index == 1 || rotation_index == 3) ?
                    temp : decode_height;
    if(pst_handle->open_param.roiEnable == TRUE) {
        decode_width = pst_handle->initial_info.roiFrameWidth ;
        decode_height = pst_handle->initial_info.roiFrameHeight;
    }

    if ((pst_handle->max_frame_width != 0 && decode_width > pst_handle->max_frame_width) || (pst_handle->max_frame_height != 0 && decode_height > pst_handle->max_frame_height))
    {
        JLOG(ERR, "exceed the max resolution of decoder, limit: %u x %u, input: %u x %u",
                     pst_handle->max_frame_width, pst_handle->max_frame_height, decode_width, decode_height);
        return JPG_RET_INVALID_PARAM;
    }

    if ((pst_handle->min_frame_width != 0 && decode_width < pst_handle->min_frame_width) || (pst_handle->min_frame_height != 0 && decode_height < pst_handle->min_frame_height))
    {
        JLOG(ERR, "deceed the min resolution of decoder, limit: %u x %u, input: %u x %u",
                     pst_handle->min_frame_width, pst_handle->min_frame_height, decode_width, decode_height);
        return JPG_RET_INVALID_PARAM;
    }

    // Check restrictions
    if (rotation_index != 0 || rotation_index != MIRDIR_NONE) {
        if (pst_handle->open_param.outputFormat != FORMAT_MAX
            && pst_handle->open_param.outputFormat != pst_handle->initial_info.sourceFormat) {
            JLOG(ERR, "The rotator cannot work with the format converter together.\n");
            return JPG_RET_FAILURE;
        }
    }

    scaler_on = (BOOL)(pst_handle->hor_scale_mode || pst_handle->ver_scale_mode);

    if (pst_handle->open_param.rotation == 90 || pst_handle->open_param.rotation == 270) {
        if (subsample == FORMAT_422) subsample = FORMAT_440;
        else if (subsample == FORMAT_440) subsample = FORMAT_422;
    }

    GetFrameBufStride(subsample,
                      pst_handle->open_param.chromaInterleave,
                      pst_handle->open_param.packedFormat,
                      scaler_on,
                      decode_width, decode_height,
                      (pst_handle->initial_info.bitDepth + 7)/8,
                      &luma_stride,
                      &luma_height,
                      &chroma_stride,
                      &chroma_height);

    luma_size = luma_stride * luma_height;
    chroma_size = chroma_stride * chroma_height;
    if (pst_handle->open_param.chromaInterleave)
        frame_size = luma_size +chroma_size;
    else
        frame_size = luma_size + 2 * chroma_size;

    pst_handle->frame_buffer.stride     = luma_stride;
    pst_handle->frame_buffer.strideC    = chroma_stride;
    pst_handle->frame_buffer.fbLumaHeight   = luma_height;
    pst_handle->frame_buffer.fbChromaHeight = chroma_height;
    pst_handle->frame_buffer.endian  = pst_handle->open_param.frameEndian;
    pst_handle->frame_buffer.format  = subsample;

    if(pst_handle->external_fb.phys_addr && pst_handle->external_fb.size)
    {
        if(pst_handle->external_fb.size < frame_size)
        {
            JLOG(ERR, "user set external fb size: %ld < calculated size: %d\n", pst_handle->external_fb.size, frame_size);
            return JPG_RET_INVALID_PARAM;
        }

        pst_handle->frame_buffer.bufY = pst_handle->external_fb.phys_addr;
        pst_handle->frame_buffer.bufCb = pst_handle->frame_buffer.bufY + luma_size;
        if (pst_handle->open_param.chromaInterleave == CBCR_SEPARATED)
            pst_handle->frame_buffer.bufCr = pst_handle->frame_buffer.bufCb + chroma_size;

        blk = vb_create_block(pst_handle->external_fb.phys_addr, NULL, VB_STATIC_POOLID, 1);
        if (blk == VB_INVALID_HANDLE)
            return JPG_RET_FAILURE;

        if (_insert_frame_buffer(pst_handle, pst_handle->frame_buffer, pst_handle->external_fb.size, blk, 1) < 0)
            return JPG_RET_FAILURE;
    } else {
        blk = vb_get_block_with_id(VB_STATIC_POOLID, frame_size, ID_VDEC);
        if (blk == VB_INVALID_HANDLE)
            return JPG_RET_FAILURE;

        pst_handle->frame_buffer.bufY = vb_handle2phys_addr(blk);
        pst_handle->frame_buffer.bufCb = pst_handle->frame_buffer.bufY + luma_size;
        if (pst_handle->open_param.chromaInterleave == CBCR_SEPARATED)
            pst_handle->frame_buffer.bufCr = pst_handle->frame_buffer.bufCb + chroma_size;

        base_ion_cache_invalidate(pst_handle->frame_buffer.bufY, NULL, frame_size);
        if (_insert_frame_buffer(pst_handle, pst_handle->frame_buffer, frame_size, blk, 0) < 0) {
            vb_release_block(blk);
            return JPG_RET_FAILURE;
        }
    }

    return JPG_RET_SUCCESS;
}

static int _jpeg_register_framebuffer(JPEG_DEC_HANDLE *pst_handle)
{
    JpgRet ret = JPG_RET_SUCCESS;

    ret = JPU_DecGetInitialInfo(pst_handle->handle, &pst_handle->initial_info);
    if (JPG_RET_BIT_EMPTY == ret) {
        JLOG(INFO, "BITSTREAM EMPTY\n");
        return ret;
    }
    else if (ret != JPG_RET_SUCCESS) {
        JLOG(ERR, "JPU_DecGetInitialInfo failed: 0x%x, inst=%d \n", ret, pst_handle->handle->instIndex);
        return ret;
    }

    if (!pst_handle->frame_buffer.bufY) {
        ret = _jpeg_alloc_frame(pst_handle);
        if (ret != JPG_RET_SUCCESS) {
            JLOG(ERR, "_jpeg_calculate_stride failed:%d\n", ret);
            return ret;
        }
    }

    JPU_SetExtAddr(pst_handle->core_idx, pst_handle->frame_buffer.bufY >> 32);

    // Register frame buffers requested by the decoder.
    ret = JPU_DecRegisterFrameBuffer(pst_handle->handle, &pst_handle->frame_buffer, 1, pst_handle->frame_buffer.stride);
    if (ret != JPG_RET_SUCCESS) {
        JLOG(ERR, "JPU_DecRegisterFrameBuffer failed Error code is 0x%x \n", ret );
        return ret;
    }

    ret = JPU_DecGiveCommand(pst_handle->handle, SET_JPG_SCALE_HOR,  &pst_handle->hor_scale_mode);
    if (ret != JPG_RET_SUCCESS) {
        JLOG(ERR, "JPU_DecGiveCommand[SET_JPG_SCALE_HOR] failed Error code is 0x%x \n", ret );
        return ret;
    }

    ret = JPU_DecGiveCommand(pst_handle->handle, SET_JPG_SCALE_VER,  &pst_handle->ver_scale_mode);
    if (ret != JPG_RET_SUCCESS) {
        JLOG(ERR, "JPU_DecGiveCommand[SET_JPG_SCALE_VER] failed Error code is 0x%x \n", ret);
        return ret;
    }

    return ret;
}

static void _enqueue_disp_frame(JPEG_DEC_HANDLE *pst_handle, FrameBuffer frame_buffer)
{
    DRVFRAMEBUF disp_frame = {0};

    disp_frame.packedFormat = pst_handle->open_param.packedFormat;
    disp_frame.chromaInterleave = pst_handle->open_param.chromaInterleave;
    disp_frame.format = frame_buffer.format;

    disp_frame.vbY.phys_addr = frame_buffer.bufY;
    disp_frame.vbY.virt_addr = phys_to_virt(frame_buffer.bufY);
    disp_frame.vbY.size = frame_buffer.stride * frame_buffer.fbLumaHeight;

    disp_frame.vbCb.phys_addr = frame_buffer.bufCb;
    disp_frame.vbCb.virt_addr = phys_to_virt(frame_buffer.bufCb);
    disp_frame.vbCb.size = frame_buffer.strideC * frame_buffer.fbChromaHeight;

    if (!pst_handle->open_param.chromaInterleave) {
        disp_frame.vbCr.phys_addr = frame_buffer.bufCr;
        disp_frame.vbCr.virt_addr = phys_to_virt(frame_buffer.bufCr);
        disp_frame.vbCr.size = frame_buffer.strideC * frame_buffer.fbChromaHeight;
    }

    if (pst_handle->ver_scale_mode ||
        pst_handle->hor_scale_mode ||
        pst_handle->open_param.rotation == 90 ||
        pst_handle->open_param.rotation == 270) {
        disp_frame.width = pst_handle->output_info.decPicWidth;
        disp_frame.height = pst_handle->output_info.decPicHeight;
    } else {
        disp_frame.width = pst_handle->initial_info.picWidth;
        disp_frame.height = pst_handle->initial_info.picHeight;
    }

    disp_frame.strideY = frame_buffer.stride;
    disp_frame.strideC = frame_buffer.strideC;

    Queue_Enqueue(pst_handle->disp_frame, &disp_frame);

    return ;
}

/* send jpu data to decode or encode */
int jpeg_dec_send_stream(drv_jpg_handle handle, void *data, int length, int timeout)
{
    int ret = JPG_RET_SUCCESS;
    JpgDecParam decParam = { 0 };
    int int_reason = 0;
    JPEG_DEC_HANDLE *pst_handle;
    int dec_timeout;

    if (NULL == handle) {
        JLOG(ERR,"handle = NULL\n");
        return -1;
    }

    pst_handle = (JPEG_DEC_HANDLE *)handle;

    dec_timeout = (timeout > 0) ? timeout : JPU_INTERRUPT_TIMEOUT_MS;

    if (length > pst_handle->stream_buffer.size)
    {
        JLOG(ERR, "jpeg dec bs size: %ld < jpeg data length: %d\n", pst_handle->stream_buffer.size, length);
        return JPG_RET_FAILURE;
    }

    update_bind_mode(pst_handle);

    pst_handle->handle->coreIndex = pst_handle->core_idx = JPU_RequestCore(JPU_INTERRUPT_TIMEOUT_MS);
    if (pst_handle->core_idx < 0)
        return JPG_RET_FAILURE;

    ret = JPU_DecSetRdPtr(pst_handle->handle, pst_handle->stream_buffer.phys_addr, 1);
    if (ret != JPG_RET_SUCCESS) {
        JLOG(ERR, "JPU_DecSetRdPtr ret:%d\n", ret);
        JPU_ReleaseCore(pst_handle->core_idx);
        return ret;
    }

    ret = jdi_write_memory(pst_handle->stream_buffer.phys_addr, data, length, pst_handle->open_param.streamEndian);
    if (ret != length) {
        JLOG(ERR, "jdi_write_memory failed [%d < %d]\n", ret, length);
        JPU_ReleaseCore(pst_handle->core_idx);
        return ret;
    }

    ret = JPU_DecUpdateBitstreamBuffer(pst_handle->handle, length);
    if (ret != JPG_RET_SUCCESS) {
        JLOG(ERR, "JPU_DecUpdateBitstreamBuffer ret:%d\n", ret);
        JPU_ReleaseCore(pst_handle->core_idx);
        return ret;
    }

    ret = JPU_DecUpdateBitstreamBuffer(pst_handle->handle, STREAM_END_SIZE);
    if (ret != JPG_RET_SUCCESS) {
        JLOG(ERR, "JPU_DecUpdateBitstreamBuffer ret:%d\n", ret);
        JPU_ReleaseCore(pst_handle->core_idx);
        return ret;
    }

    ret = _jpeg_register_framebuffer(pst_handle);
    if (ret != JPG_RET_SUCCESS) {
        JLOG(ERR, "_jpeg_register_framebuffer ret:%d\n", ret);
        JPU_ReleaseCore(pst_handle->core_idx);
        return ret;
    }

    ret = JPU_DecStartOneFrame(pst_handle->handle, &decParam);
    if (ret != JPG_RET_SUCCESS && ret != JPG_RET_EOS) {
        if (ret == JPG_RET_BIT_EMPTY) {
            JLOG(ERR, "BITSTREAM NOT ENOUGH.............\n");
            JPU_ReleaseCore(pst_handle->handle->coreIndex);
            return ret;
        }

        JLOG(ERR, "JPU_DecStartOneFrame failed Error code is 0x%x \n", ret );
        return ret;
    }

    while(1) {
        if ((int_reason=JPU_WaitInterrupt(pst_handle->handle, dec_timeout)) == -1) {
            JLOG(ERR, "Error dec: timeout happened,core:%d inst %d, reason:%d, irq_status:%d, irq_cnt:%d\n",
            pst_handle->core_idx, pst_handle->handle->instIndex, int_reason,
            irq_status[pst_handle->core_idx],jpu_core_irq_count[pst_handle->core_idx]);
            _jpeg_dump_register(pst_handle->core_idx, pst_handle->handle->instIndex);
            JPU_SetJpgPendingInstEx(pst_handle->handle, NULL);
            JpgLeaveLock();
            JPU_ReleaseCore(pst_handle->core_idx);
            return JPG_RET_FAILURE;
        }

        if (int_reason == -2) {
            JLOG(ERR, "Interrupt occurred. but this interrupt is not for my instance dec\n");
            continue;
        }

        if (int_reason & ((1<<INT_JPU_DONE) | (1<<INT_JPU_ERROR) | (1<<INT_JPU_SLICE_DONE))) {
            JLOG(INFO, "\tINSTANCE #%d int_reason: %08x\n", pst_handle->handle->instIndex, int_reason);
            ret = JPG_RET_SUCCESS;
            break;
        }

        if (int_reason & (1<<INT_JPU_BIT_BUF_EMPTY)) {
            //need more jpeg data,we need the same coreidx here?
            JPU_ClrStatus(pst_handle->handle, (1<<INT_JPU_BIT_BUF_EMPTY));
            ret = JPG_RET_BIT_EMPTY;
            break;
        }
    }

    ret = JPU_DecGetOutputInfo(pst_handle->handle, &pst_handle->output_info);
    if (ret != JPG_RET_SUCCESS) {
        JLOG(ERR, "JPU_DecGetOutputInfo failed Error code is 0x%x \n", ret );
        JPU_ReleaseCore(pst_handle->core_idx);
        return ret;
    }

    JPU_ReleaseCore(pst_handle->core_idx);
    if (pst_handle->output_info.decodingSuccess == 0) {
        JLOG(ERR, "JPU_DecGetOutputInfo decode fail\n");
        return JPG_RET_FAILURE;
    }

    if (pst_handle->output_info.indexFrameDisplay == -1)
        return JPG_RET_FAILURE;

    if (pst_handle->output_info.indexFrameDisplay >= 0) {
        pst_handle->valid_cnt++;
        pst_handle->frame_num++;
    }

    if (pst_handle->is_bind_mode)
        fill_vbbuffer(pst_handle, pst_handle->frame_buffer);
    else
        _enqueue_disp_frame(pst_handle, pst_handle->frame_buffer);
    memset(&pst_handle->frame_buffer, 0, sizeof(FrameBuffer));

    return ret;
}

int jpeg_enc_get_stream(drv_jpg_handle handle, void *data, unsigned long int *pu64HwTime)
{
    int ret = JPG_RET_SUCCESS;
    JPEG_ENC_HANDLE *pst_handle = (JPEG_ENC_HANDLE *)handle;
    JPG_BUF *pStream = (JPG_BUF *)data;

    if (NULL == handle) {
        JLOG(ERR,"jpgHandle = NULL\n");
        return -1;
    }

    ret = CheckJpgInstValidity(pst_handle->handle);
    if (ret != JPG_RET_SUCCESS) {
        JLOG(INFO, "CheckJpgInstValidity fail, return %d\n", ret);
        return ret;
    }

    if(pst_handle->output_info.encodeState != ENCODE_STATE_FRAME_DONE) {
        JLOG(INFO, "%s not ready, return %d\n", __func__, ret);
        return ret;
    }

    if(pst_handle->valid_cnt > 0)
        pst_handle->valid_cnt--;

    if(pst_handle->header_buffer.phys_addr && pst_handle->header_buffer.size) {
        pStream->phys_addr = pst_handle->header_buffer.phys_addr;
        pStream->virt_addr = (__u8*)pst_handle->header_buffer.virt_addr;
        pStream->size = pst_handle->header_param.size;
        pStream = pStream+1;
    }

    if(!pst_handle->stream_buffer_ex.stream_len) {
        pStream->phys_addr = pst_handle->stream_buffer.phys_addr;
        pStream->virt_addr = (__u8*)pst_handle->stream_buffer.virt_addr;
        pStream->size = pst_handle->output_info.bitstreamSize;
    } else {
        pStream->phys_addr = pst_handle->stream_buffer_ex.buffer.phys_addr;
        pStream->virt_addr = (__u8*)pst_handle->stream_buffer_ex.buffer.virt_addr;
        pStream->size = pst_handle->stream_buffer_ex.stream_len;
    }

    if (pu64HwTime) {
        //*pu64HwTime = pJpgInst->u64EndTime - pJpgInst->u64StartTime;
        *pu64HwTime = 0; //get the cost time
    }

    return ret;
}

int jpeg_dec_get_frame(drv_jpg_handle handle, void *data, unsigned long int *pu64HwTime)
{
    JPEG_DEC_HANDLE *pst_handle = (JPEG_DEC_HANDLE *)handle;
    DRVFRAMEBUF *frame;

    if (NULL == handle) {
        JLOG(ERR,"handle = NULL\n");
        return -1;
    }

    frame = Queue_Dequeue(pst_handle->disp_frame);
    if (frame == NULL)
        return -1;

    memcpy(data, frame, sizeof(DRVFRAMEBUF));
    if(pst_handle->valid_cnt > 0)
        pst_handle->valid_cnt--;

    if (pu64HwTime) {
        //*pu64HwTime = pJpgInst->u64EndTime - pJpgInst->u64StartTime;
        *pu64HwTime = 0; //get the cost time
    }

    return JPG_RET_SUCCESS;
}

/* release stream buffer */
int jpeg_enc_release_stream(drv_jpg_handle handle)
{
    int ret = JPG_RET_SUCCESS;
    JPEG_ENC_HANDLE *pst_handle;

    if (NULL == handle) {
        JLOG(INFO,"jpgHandle = NULL\n");
        return -1;
    }

    pst_handle = (JPEG_ENC_HANDLE *)handle;

    if(pst_handle->stream_buffer_ex.stream_len && pst_handle->stream_buffer_ex.buffer.base) {
        jdi_free_dma_memory(&pst_handle->stream_buffer_ex.buffer);
    }

    pst_handle->stream_buffer_ex.stream_len = 0;

    return ret;
}

int jpeg_dec_release_frame(drv_jpg_handle handle, uint64_t addr)
{
    int ret = JPG_RET_SUCCESS;
    int frame_idx;
    JPEG_DEC_HANDLE *pst_handle = handle;

    if (NULL == handle) {
        JLOG(INFO,"jpgHandle = NULL\n");
        return -1;
    }

    ret = CheckJpgInstValidity((JpgDecHandle)pst_handle->handle);
    if (ret != JPG_RET_SUCCESS) {
        JLOG(INFO, "CheckJpgInstValidity fail, return %d\n", ret);
        return ret;
    }

    frame_idx = _find_frame_buffer(pst_handle, addr);
    if (frame_idx >= 0)
        _free_frame_buffer(pst_handle, frame_idx);

    return ret;
}

int jpeg_reset(void)
{
    return 0;
}

int jpeg_enc_set_quality(drv_jpg_handle handle, void *data)
{
    JPEG_ENC_HANDLE *pst_handle = (JPEG_ENC_HANDLE *)handle;
    JpgQualityTable *qt_table = (JpgQualityTable *)data;

    if(!qt_table) {
        JLOG(ERR,"data is NULL.\n");
        return JPG_RET_INVALID_PARAM;
    }

    if(!pst_handle->config) {
        JLOG(ERR,"pstEncConfig is NULL.\n");
        return JPG_RET_WRONG_CALL_SEQUENCE;
    }

    if(qt_table->quality > 100) {
        JLOG(ERR,"quality %u ,only ranges between 0-100,\n", qt_table->quality);
        return JPG_RET_INVALID_PARAM;
    }

    if (qt_table->quality == 50) {
        JPU_EncGiveCommand(pst_handle->handle, SET_JPG_QUALITY_TABLE, qt_table);
        JLOG(INFO,"set jpeg's qt tab suc.\n");
        return JPG_RET_SUCCESS;
    }

    if (qt_table->quality > 0)  {
        pst_handle->config->encQualityPercentage = qt_table->quality;
        JPU_EncGiveCommand(pst_handle->handle, SET_JPG_QUALITY_FACTOR, &pst_handle->config->encQualityPercentage);
    }

    return JPG_RET_SUCCESS;
}

int jpeg_enc_get_quality(drv_jpg_handle handle, void *data)
{
    JPEG_ENC_HANDLE *pst_handle = (JPEG_ENC_HANDLE *)handle;
    JpgQualityTable *qt_table = (JpgQualityTable *)data;

    if(!pst_handle->config) {
        JLOG(ERR,"pstEncConfig is NULL.\n");
        return JPG_RET_WRONG_CALL_SEQUENCE;
    }

    if(!data) {
        JLOG(ERR,"data is NULL.\n");
        return JPG_RET_INVALID_PARAM;
    }

    qt_table->quality = pst_handle->config->encQualityPercentage;
    JPU_EncGiveCommand(pst_handle->handle, GET_JPG_QUALITY_TABLE, qt_table);

    return JPG_RET_SUCCESS;
}

int jpeg_set_chn_attr(drv_jpg_handle handle, void *arg)
{
    JPEG_ENC_HANDLE *pst_handle = (JPEG_ENC_HANDLE *)handle;
    JpgEncOpenParam *jpu_open_param = &pst_handle->open_param;
    jpeg_chn_attr *pChnAttr = (jpeg_chn_attr *)arg;
    unsigned int numerator = 0;
    unsigned int denominator = 0;

    jpu_open_param->bitrate = pChnAttr->u32BitRate ;

    /*for float fps, such as 27.5 fps, make numerator = 27500, denominator = 1000
     *in middleware: pChnAttr->fr32DstFrameRate = (denominator << 16) | numerator
     */
    denominator = pChnAttr->fr32DstFrameRate >> 16;
    numerator = pChnAttr->fr32DstFrameRate & 0xFFFF;

    if (denominator == 0) {  //integer fps
        jpu_open_param->framerate = numerator;
    } else {
        jpu_open_param->framerate = numerator / denominator;
    }
    return 0;
}

int jpeg_enc_set_mcuinfo(drv_jpg_handle handle, void *data)
{
    int ret = 0;

    return ret;
}

int jpeg_channel_reset(drv_jpg_handle handle, void *data)
{
    int ret = 0;


    return ret;
}

int jpeg_set_user_data(drv_jpg_handle handle, void *data)
{
    int ret = 0;

    return ret;
}

int jpeg_show_channel_info(drv_jpg_handle handle, void *data)
{

    return 0;
}

int jpeg_enc_get_output_count(drv_jpg_handle handle)
{
    int ret = JPG_RET_SUCCESS;
    JPEG_ENC_HANDLE *pst_handle;

    if (NULL == handle) {
        JLOG(ERR,"handle = NULL\n");
        return -1;
    }

    pst_handle = (JPEG_ENC_HANDLE *)handle;

    ret = CheckJpgInstValidity(pst_handle->handle);
    if (ret != JPG_RET_SUCCESS) {
        JLOG(ERR, "CheckJpgInstValidity fail, return %d\n", ret);
        return 0;
    }

    return pst_handle->valid_cnt;

}

int jpeg_dec_get_output_count(drv_jpg_handle handle)
{
    JPEG_DEC_HANDLE *pst_handle;

    if (NULL == handle) {
        JLOG(ERR,"handle = NULL\n");
        return -1;
    }

    pst_handle = (JPEG_DEC_HANDLE *)handle;

    return pst_handle->valid_cnt;
}

int jpeg_set_rc_param(drv_jpg_handle handle, void *data)
{
	int ret = JPG_RET_SUCCESS;
    JPEG_ENC_HANDLE *pst_handle = handle;
    jpeg_rc_context *pst_rc_handle = &pst_handle->rc_context;
	jpeg_enc_rc_param *pst_jpeg_rc_param = (jpeg_enc_rc_param *)data;

    pst_rc_handle->qfactor_min = pst_jpeg_rc_param->min_qfactor;
    pst_rc_handle->qfactor_max = pst_jpeg_rc_param->max_qfactor;
    pst_rc_handle->bitrate = pst_rc_handle->bitrate * pst_jpeg_rc_param->chg_pos / 100;

    JLOG(INFO, "set rc qfactor:[%d, %d], chgpos:%d, bitrate:%ld\n"
        , pst_rc_handle->qfactor_min, pst_rc_handle->qfactor_max, pst_jpeg_rc_param->chg_pos
        , pst_rc_handle->bitrate);
	return ret;
}

typedef struct _JPEG_IOCTL_OP_ {
    int option_num;
    int (*ioctl_func)(drv_jpg_handle handle, void *arg);
} JPEG_IOCTL_OP;

JPEG_IOCTL_OP jpeg_ioctl_option[] = {
    { DRV_JPEG_OP_NONE, NULL },
    { DRV_JPEG_OP_SET_QUALITY, jpeg_enc_set_quality},
    { DRV_JPEG_OP_GET_QUALITY, jpeg_enc_get_quality},
    { DRV_JPEG_OP_SET_CHN_ATTR, jpeg_set_chn_attr},
    { DRV_JPEG_OP_SET_MCUPerECS, jpeg_enc_set_mcuinfo},
    { DRV_JPEG_OP_RESET_CHN, jpeg_channel_reset},
    { DRV_JPEG_OP_SET_USER_DATA, jpeg_set_user_data},
    { DRV_JPEG_OP_SHOW_CHN_INFO, jpeg_show_channel_info},
    { DRV_JPEG_OP_START, NULL},
    { DRV_JPEG_OP_SET_SBM_ENABLE, NULL},
    { DRV_JPEG_OP_WAIT_FRAME_DONE, NULL},
    { DRV_JPEG_OP_SET_QMAP_TABLE, NULL},
    { DRV_JPEG_OP_SET_RC_PARAM,  jpeg_set_rc_param},
};

int jpeg_ioctl(drv_jpg_handle handle, int op, void *arg)
{
    int ret = 0;
    int curr_option;

    JLOG(INFO, "\n");

    if (op <= 0 || op >= DRV_JPEG_OP_MAX) {
        JLOG(ERR, "option = %d\n", op);
        return -1;
    }

    curr_option = (jpeg_ioctl_option[op].option_num & DRV_JPEG_OP_MASK) >> DRV_JPEG_OP_SHIFT;
    if (op != curr_option) {
        JLOG(ERR, "option = %d\n", op);
        return -1;
    }

    ret = jpeg_ioctl_option[op].ioctl_func(handle, arg);

    return ret;
}
