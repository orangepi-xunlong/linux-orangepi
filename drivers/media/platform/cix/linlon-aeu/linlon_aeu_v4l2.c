// SPDX-License-Identifier: GPL-2.0
/*
 * (C) COPYRIGHT 2022-2023 Arm Technology (China) Co., Ltd.
 * ALL RIGHTS RESERVED
 *
 */

#include <linux/slab.h>
#include <linux/pm.h>
#include <linux/pm_runtime.h>
#include <uapi/drm/drm_fourcc.h>
#include <media/videobuf2-memops.h>
#include <media/videobuf2-dma-sg.h>
#include "linlon_aeu_dev.h"
#include "linlon_aeu_hw.h"
#include "linlon_aeu_reg_dump.h"
#include "linlon_aeu_log.h"

#define IRQ_NAME    "AEU"

struct linlon_aeu_pix_fmt_desc {
    u32 fmt_fourcc;
    enum aeu_hw_ds_format hw_fmt;
    u32 afbc_flag;
};

static const struct linlon_aeu_pix_fmt_desc fmt_table[] = {
    { DRM_FORMAT_ARGB2101010, ds_argb_2101010, LINLON_AEU_HW_AFBC_YT },
    { DRM_FORMAT_ABGR2101010, ds_abgr_2101010, LINLON_AEU_HW_AFBC_YT },
    { DRM_FORMAT_RGBA1010102, ds_rgba_1010102, LINLON_AEU_HW_AFBC_YT },
    { DRM_FORMAT_BGRA1010102, ds_bgra_1010102, LINLON_AEU_HW_AFBC_YT },

    { DRM_FORMAT_ARGB8888, ds_argb_8888, LINLON_AEU_HW_AFBC_YT },
    { DRM_FORMAT_ABGR8888, ds_abgr_8888, LINLON_AEU_HW_AFBC_YT },
    { DRM_FORMAT_RGBA8888, ds_rgba_8888, LINLON_AEU_HW_AFBC_YT },
    { DRM_FORMAT_BGRA8888, ds_bgra_8888, LINLON_AEU_HW_AFBC_YT },

    { DRM_FORMAT_XRGB8888, ds_xrgb_8888, LINLON_AEU_HW_AFBC_YT },
    { DRM_FORMAT_XBGR8888, ds_xbgr_8888, LINLON_AEU_HW_AFBC_YT },
    { DRM_FORMAT_RGBX8888, ds_rgbx_8888, LINLON_AEU_HW_AFBC_YT },
    { DRM_FORMAT_BGRX8888, ds_bgrx_8888, LINLON_AEU_HW_AFBC_YT },

    { DRM_FORMAT_RGBA5551, ds_rgba_5551, LINLON_AEU_HW_AFBC_YT },
    { DRM_FORMAT_ABGR1555, ds_abgr_1555, LINLON_AEU_HW_AFBC_YT },

    { DRM_FORMAT_RGB565, ds_rgb_565, LINLON_AEU_HW_AFBC_YT },
    { DRM_FORMAT_BGR565, ds_bgr_565, LINLON_AEU_HW_AFBC_YT },

    { DRM_FORMAT_YUYV, ds_vyuv_422_p1_8, 0 },
    { DRM_FORMAT_UYVY, ds_yvyu_422_p1_8, 0 },

    { DRM_FORMAT_NV12, ds_yuv_420_p2_8, 0 },
    { DRM_FORMAT_YUV420, ds_yuv_420_p3_8, 0 },
    { DRM_FORMAT_P010, ds_yuv_420_p2_10, 0 }
};

#define FMT_TABLE_SIZE    sizeof(fmt_table)/sizeof(struct linlon_aeu_pix_fmt_desc)

static enum aeu_hw_ds_format get_hw_fmt_with_flag(u32 fourcc, u32 *flg)
{
    u32 i;

    for (i = 0; i < FMT_TABLE_SIZE; i++)
        if (fourcc == fmt_table[i].fmt_fourcc) {
            if (flg)
                *flg = fmt_table[i].afbc_flag;
            return fmt_table[i].hw_fmt;
        }
    return 0xFFFFFFFF;
}

#define get_hw_fmt(f)    get_hw_fmt_with_flag(f, NULL)

struct linlon_aeu_v4l2_buffer {
    struct v4l2_m2m_buffer    v4l2_buf;
    struct linlon_aeu_hw_buf_fmt    hw_fmt;
    /* clone from vb2.queue->memops->cookies */
    struct sg_table sg_tbl_bk[LINLON_AEU_HW_PLANES];
};

typedef struct {
    struct v4l2_fh        fh;
    struct linlon_aeu_device    *adev;
    struct v4l2_ctrl_handler    hdl;
    u32    running: 1,
        pm_enabled: 1,
        reserved: 30;
    wait_queue_head_t    idle;
    /* protect access to the buffer status */
    spinlock_t        buf_lock;
    /* how many buffers per transaction */
    u32    trans_num;

    struct linlon_aeu_hw_buf_fmt curr_buf_fmt[2];

    linlon_aeu_hw_ctx_t    *hw_ctx;

} aeu_ctx_t;

static inline struct linlon_aeu_v4l2_buffer *
to_aeu_v4l2_buf(void *b)
{
    struct v4l2_m2m_buffer *v4l2_mb =
        container_of(b, struct v4l2_m2m_buffer, vb);

    return container_of(v4l2_mb, struct linlon_aeu_v4l2_buffer,
                v4l2_buf);
}

/* buffer queue implementation */
static int aeu_queue_setup(struct vb2_queue *q,
               unsigned int *nbuf,
               unsigned int *nplanes, unsigned int sizes[],
               struct device *alloc_devs[])
{
    aeu_ctx_t *ctx = vb2_get_drv_priv(q);
    u32 type = (V4L2_TYPE_IS_OUTPUT(q->type)) ?
            AEU_HW_INPUT_BUF : AEU_HW_OUTPUT_BUF;
    u32 max_planes = (type == AEU_HW_INPUT_BUF) ? 3 : 1;
    struct linlon_aeu_hw_buf_fmt *bf = &ctx->curr_buf_fmt[type];

    if (*nbuf > ctx->trans_num)
        *nbuf = ctx->trans_num;

    for (*nplanes = 0; *nplanes < max_planes; (*nplanes)++) {
        sizes[*nplanes] = linlon_aeu_hw_plane_size(bf, *nplanes);
        if (sizes[*nplanes] == 0)
            break;
    }

    if (*nplanes == 0) {
        dev_err(ctx->adev->dev, "%s: buffer queue setup error!\n",
            __func__);
        return -EINVAL;
    }

    return 0;
}

static int aeu_buf_prepare(struct vb2_buffer *buf)
{
    struct vb2_v4l2_buffer *vbuf = to_vb2_v4l2_buffer(buf);
    aeu_ctx_t *ctx = vb2_get_drv_priv(buf->vb2_queue);
    u32 i;

    if (V4L2_TYPE_IS_OUTPUT(buf->vb2_queue->type)) {
        if (vbuf->field == V4L2_FIELD_ANY)
            vbuf->field = V4L2_FIELD_NONE;
        if (vbuf->field != V4L2_FIELD_NONE) {
            dev_err(ctx->adev->dev, "%s:field isn't supported\n",
                    __func__);
            return -EINVAL;
        }
    }

    for (i = 0; i < buf->num_planes; i++)
        vb2_set_plane_payload(buf, i, vb2_plane_size(buf, i));

    return 0;
}

static void aeu_buf_queue(struct vb2_buffer *buf)
{
    aeu_ctx_t *ctx = vb2_get_drv_priv(buf->vb2_queue);
    struct vb2_v4l2_buffer *vbuf = to_vb2_v4l2_buffer(buf);
    struct linlon_aeu_v4l2_buffer *aeu_buf = to_aeu_v4l2_buf(buf);
    u32 type = AEU_HW_OUTPUT_BUF;

    v4l2_m2m_buf_queue(ctx->fh.m2m_ctx, vbuf);

    if (V4L2_TYPE_IS_OUTPUT(buf->vb2_queue->type))
        type = AEU_HW_INPUT_BUF;

    aeu_buf->hw_fmt = ctx->curr_buf_fmt[type];
}

static int aeu_start_streaming(struct vb2_queue *q, unsigned int count)
{
    aeu_ctx_t *ctx = vb2_get_drv_priv(q);

    pm_runtime_get_sync(ctx->adev->dev);
    dev_dbg(ctx->adev->dev, "%s:start\n", __func__);
    return 0;
}

static void aeu_stop_streaming(struct vb2_queue *q)
{
    aeu_ctx_t *ctx = vb2_get_drv_priv(q);
    struct vb2_v4l2_buffer *vbuf;
    unsigned long flags;

    do {
        if (V4L2_TYPE_IS_OUTPUT(q->type))
            vbuf = v4l2_m2m_src_buf_remove(ctx->fh.m2m_ctx);
        else
            vbuf = v4l2_m2m_dst_buf_remove(ctx->fh.m2m_ctx);

        if (vbuf) {
            spin_lock_irqsave(&ctx->buf_lock, flags);
            vb2_buffer_done(&vbuf->vb2_buf, VB2_BUF_STATE_ERROR);
            spin_unlock_irqrestore(&ctx->buf_lock, flags);
        }

    } while (vbuf != NULL);
    pm_runtime_put(ctx->adev->dev);
    dev_dbg(ctx->adev->dev, "%s:stop\n", __func__);
}

static void aeu_buf_finish_and_cleanup(struct vb2_buffer *buf)
{
    struct linlon_aeu_v4l2_buffer *aeu_buf = to_aeu_v4l2_buf(buf);
    aeu_ctx_t *ctx = vb2_get_drv_priv(buf->vb2_queue);
    u32 i;
    enum dma_data_direction dir;

    dir = DMA_BIDIRECTIONAL;
    for (i = 0; i < LINLON_AEU_HW_PLANES; i++) {
        if (!aeu_buf->sg_tbl_bk[i].sgl)
            continue;
        dma_unmap_sg_attrs(ctx->adev->dev, aeu_buf->sg_tbl_bk[i].sgl,
                    aeu_buf->sg_tbl_bk[i].nents, dir,
                    DMA_ATTR_SKIP_CPU_SYNC);
        sg_free_table(&aeu_buf->sg_tbl_bk[i]);
    }
}

static const struct vb2_ops aeu_qops = {
    .queue_setup        = aeu_queue_setup,
    .buf_prepare        = aeu_buf_prepare,
    .buf_queue        = aeu_buf_queue,
    .start_streaming    = aeu_start_streaming,
    .buf_finish        = aeu_buf_finish_and_cleanup,
    .buf_cleanup        = aeu_buf_finish_and_cleanup,
    .stop_streaming        = aeu_stop_streaming,
    .wait_prepare        = vb2_ops_wait_prepare,
    .wait_finish        = vb2_ops_wait_finish,
};

static int init_vb2_queue(struct vb2_queue *q, unsigned int type, void *priv)
{
    int ret;
    aeu_ctx_t *ctx = priv;

    q->type = type;
    q->io_modes = VB2_MMAP | VB2_USERPTR | VB2_DMABUF;
    q->drv_priv = ctx;
    q->buf_struct_size = sizeof(struct linlon_aeu_v4l2_buffer);
    q->ops = &aeu_qops;
    q->dev = ctx->adev->dev;
    q->bidirectional = 1;

    q->mem_ops = &vb2_dma_sg_memops;
    q->timestamp_flags = V4L2_BUF_FLAG_TIMESTAMP_COPY;
    q->lock = &ctx->adev->aeu_mutex;

    ret = vb2_queue_init(q);
    if (ret) {
        struct device *dev = ctx->adev->dev;
        dev_err(dev, "%s: init queue (tyep: %u) error!\n",
            __func__, type);
    }
    return ret;
}

static int
aeu_queue_init(void *priv, struct vb2_queue *s_vq, struct vb2_queue *d_vq)
{
    if (init_vb2_queue(s_vq, V4L2_BUF_TYPE_VIDEO_OUTPUT, priv))
        return -1;
    return init_vb2_queue(d_vq, V4L2_BUF_TYPE_VIDEO_CAPTURE, priv);
}

/* Define user contrl ID */
#define V4L2_CID_TRANS_NUM_BUFS    (V4L2_CID_USER_BASE + 0x1000)
#define V4L2_CID_AFBC_MODIFIERS    (V4L2_CID_USER_BASE + 0x1001)
#define V4L2_CID_PROTECTED_MODE    (V4L2_CID_USER_BASE + 0x1002)

static bool modifiers_and(u64 m, u64 f)
{
    m &= 0xffffffffffffffull;
    f &= 0xffffffffffffffull;

    return (m & f)? true : false;
}

static bool modifiers_layout_is(u64 m, u64 f)
{
    m &= AFBC_FORMAT_MOD_BLOCK_SIZE_MASK;

    return (m == f)? true : false;
}

static int aeu_s_ctrl(struct v4l2_ctrl *ctrl)
{
    aeu_ctx_t *actx = container_of(ctrl->handler, aeu_ctx_t, hdl);
    u64 modifiers;

    switch (ctrl->id) {
    case V4L2_CID_TRANS_NUM_BUFS:
        actx->trans_num = ctrl->val;
        break;
    case V4L2_CID_AFBC_MODIFIERS:
        modifiers = (u64)*ctrl->p_new.p_s64;
        if ((modifiers >> 56) != DRM_FORMAT_MOD_VENDOR_ARM) {
            dev_err(actx->adev->dev, "wrong modifiers\n");
            return -EINVAL;
        }

        if (modifiers_layout_is(modifiers, AFBC_FORMAT_MOD_BLOCK_SIZE_32x8)) {
            dev_err(actx->adev->dev,
                "Wide body layout is not supported!\n");
            return -EINVAL;
        } else {
            actx->curr_buf_fmt[AEU_HW_OUTPUT_BUF].afbc_fmt_flags &=
                ~LINLON_AEU_HW_AFBC_SBA_32x8;
        }

        if (modifiers_and(modifiers, AFBC_FORMAT_MOD_SPLIT)) {
            dev_err(actx->adev->dev,
                "Block split is not supported!\n");
            return -EINVAL;
        }

        if (modifiers_and(modifiers, AFBC_FORMAT_MOD_TILED))
            actx->curr_buf_fmt[AEU_HW_OUTPUT_BUF].afbc_fmt_flags |=
                LINLON_AEU_HW_AFBC_TH | LINLON_AEU_HW_AFBC_SC;

        if (modifiers_and(modifiers, AFBC_FORMAT_MOD_SPARSE))
            actx->curr_buf_fmt[AEU_HW_OUTPUT_BUF].afbc_fmt_flags |=
                LINLON_AEU_HW_AFBC_SP;
        break;
    case V4L2_CID_PROTECTED_MODE:
        if (ctrl->val)
            dev_dbg(actx->adev->dev,
                "AEU is running in protected mode!");
        actx->pm_enabled = !!ctrl->val;
        break;
    default:
        dev_warn(actx->adev->dev, "Unsupported ctrl (%u)\n", ctrl->id);
        return -EINVAL;
    }
    return 0;
}

static const struct v4l2_ctrl_ops aeu_ctrl_ops = {
    .s_ctrl            = aeu_s_ctrl,
};

static const struct v4l2_ctrl_config aeu_ctrl_trans_buf_num = {
    .ops = &aeu_ctrl_ops,
    .id = V4L2_CID_TRANS_NUM_BUFS,
    .name = "Number of buffers Per Transaction",
    .type = V4L2_CTRL_TYPE_INTEGER,
    .def = 1,
    .min = 1,
    .max = VIDEO_MAX_FRAME,
    .step = 1,
};

static const struct v4l2_ctrl_config aeu_ctrl_protected_mode = {
    .ops = &aeu_ctrl_ops,
    .id = V4L2_CID_PROTECTED_MODE,
    .name = "Protected Mode",
    .type = V4L2_CTRL_TYPE_INTEGER,
    .def = 0,
    .min = 0,
    .max = 0xffffffff,
    .step = 1,
};

static const struct v4l2_ctrl_config aeu_ctrl_afbc_modifiers = {
    .ops = &aeu_ctrl_ops,
    .id = V4L2_CID_AFBC_MODIFIERS,
    .name = "AFBC flags (modifiers)",
    .type = V4L2_CTRL_TYPE_INTEGER64,
    .def = fourcc_mod_code(ARM, 0),
    .min = fourcc_mod_code(ARM, 0),
    .max = fourcc_mod_code(ARM, 0xfff),
    .step = 1,
};

/* file operation implementation */
static aeu_ctx_t *file2ctx(struct file *f)
{
    return container_of(f->private_data, aeu_ctx_t, fh);
}

static int aeu_open(struct file *file)
{
    struct linlon_aeu_device *adev = video_drvdata(file);
    int ret = 0;
    aeu_ctx_t *ctx = NULL;

    if (mutex_lock_interruptible(&adev->aeu_mutex))
        return -ERESTARTSYS;

    ctx = kzalloc(sizeof(aeu_ctx_t), GFP_KERNEL);
    if (!ctx) {
        ret = -ENOMEM;
        goto exit_aeu_open;
    }

    spin_lock_init(&ctx->buf_lock);

    ctx->hw_ctx = linlon_aeu_hw_init_ctx(adev->hw_dev);
    if (!ctx->hw_ctx) {
        ret = -ENOMEM;
        goto exit_aeu_ctx;
    }

    init_waitqueue_head(&ctx->idle);
    ctx->adev = adev;
    v4l2_fh_init(&ctx->fh, &adev->vdev);
    file->private_data = &ctx->fh;

    v4l2_ctrl_handler_init(&ctx->hdl, 3);
    v4l2_ctrl_new_custom(&ctx->hdl, &aeu_ctrl_trans_buf_num, NULL);
    v4l2_ctrl_new_custom(&ctx->hdl, &aeu_ctrl_afbc_modifiers, NULL);
    v4l2_ctrl_new_custom(&ctx->hdl, &aeu_ctrl_protected_mode, NULL);
    if (ctx->hdl.error) {
        ret = ctx->hdl.error;
        goto exit_aeu_hw_ctx;
    }
    ctx->fh.ctrl_handler = &ctx->hdl;
    v4l2_ctrl_handler_setup(&ctx->hdl);

    ctx->fh.m2m_ctx = v4l2_m2m_ctx_init(adev->m2mdev, ctx, aeu_queue_init);
    if (IS_ERR(ctx->fh.m2m_ctx)) {
        ret = PTR_ERR(ctx->fh.m2m_ctx);
        goto exit_aeu_hw_ctx;
    }

    v4l2_fh_add(&ctx->fh);
    mutex_unlock(&adev->aeu_mutex);
    return 0;

exit_aeu_hw_ctx:
    v4l2_ctrl_handler_free(&ctx->hdl);
    v4l2_fh_exit(&ctx->fh);
    linlon_aeu_hw_free_ctx(ctx->hw_ctx);
exit_aeu_ctx:
    kfree(ctx);
exit_aeu_open:
    mutex_unlock(&adev->aeu_mutex);
    return ret;
}

static int aeu_release(struct file *file)
{
    struct linlon_aeu_device *adev = video_drvdata(file);
    aeu_ctx_t *ctx = file2ctx(file);

    v4l2_fh_del(&ctx->fh);
    v4l2_fh_exit(&ctx->fh);
    v4l2_ctrl_handler_free(&ctx->hdl);
    linlon_aeu_hw_free_ctx(ctx->hw_ctx);
    mutex_lock(&adev->aeu_mutex);
    v4l2_m2m_ctx_release(ctx->fh.m2m_ctx);
    mutex_unlock(&adev->aeu_mutex);
    kfree(ctx);

    return 0;
}

static const struct v4l2_file_operations linlon_aeu_fops = {
    .owner        = THIS_MODULE,
    .unlocked_ioctl    = video_ioctl2,
    .open        = aeu_open,
    .release    = aeu_release,
    .poll        = v4l2_m2m_fop_poll,
    .mmap        = v4l2_m2m_fop_mmap,
};

/* Implement aeu v4l2 ioctl operations */
static int aeu_drv_cap(struct file *file, void *priv,
               struct v4l2_capability *cap)
{
    strncpy(cap->driver, AEU_NAME, sizeof(cap->driver) - 1);
    strncpy(cap->card, AEU_NAME, sizeof(cap->card) - 1);
    snprintf(cap->bus_info, sizeof(cap->bus_info),
         "platform:%s", AEU_NAME);
    cap->device_caps = V4L2_CAP_VIDEO_M2M | V4L2_CAP_STREAMING;
    cap->capabilities = cap->device_caps | V4L2_CAP_DEVICE_CAPS;
    return 0;
}

static void align_size(struct linlon_aeu_device *adev, u32 *h, u32 *w)
{
    if (adev->hw_info.min_width > *w)
        *w = adev->hw_info.min_width;
    if (adev->hw_info.max_width < *w)
        *w = adev->hw_info.max_width;

    if (adev->hw_info.min_height > *h)
        *h = adev->hw_info.min_height;
    if (adev->hw_info.max_height < *h)
        *h = adev->hw_info.max_height;
}

static int enum_fmt(struct v4l2_fmtdesc *f, u32 type)
{
    u32 i;
    u16 np;

    for (i = 0; i < FMT_TABLE_SIZE; i++)
        if (f->pixelformat == fmt_table[i].fmt_fourcc) {
            f->index = i;
            break;
        }

    if (i == FMT_TABLE_SIZE)
        return -EINVAL;

    np = linlon_aeu_hw_pix_fmt_planes(fmt_table[i].hw_fmt);
    if (type == AEU_HW_INPUT_BUF) {
        f->type = (np > 1) ? V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE :
                V4L2_BUF_TYPE_VIDEO_OUTPUT;
        f->flags = 0;
    } else {
        f->type = (np > 1) ? V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE :
                V4L2_BUF_TYPE_VIDEO_CAPTURE;
        f->flags = V4L2_FMT_FLAG_COMPRESSED;
        if (!linlon_aeu_hw_pix_fmt_native(fmt_table[i].hw_fmt))
            f->flags |= V4L2_FMT_FLAG_EMULATED;
    }

    for (i = 0; i < 4; i++)
        f->reserved[i] = 0;
    return 0;
}

static int aeu_enum_fmt_vid_cap(struct file *file, void *p,
            struct v4l2_fmtdesc *f)
{
    return enum_fmt(f, AEU_HW_OUTPUT_BUF);
}

static int aeu_enum_fmt_vid_out(struct file *file, void *p,
            struct v4l2_fmtdesc *f)
{
    return enum_fmt(f, AEU_HW_INPUT_BUF);
}

static int aeu_try_format(aeu_ctx_t *ctx, struct v4l2_format *f)
{
    struct linlon_aeu_device *adev = ctx->adev;
    struct v4l2_pix_format    *pix = &f->fmt.pix;
    struct v4l2_pix_format_mplane *pix_mp = &f->fmt.pix_mp;
    u16 np = 1;

    if (f->type != V4L2_BUF_TYPE_VIDEO_CAPTURE &&
        f->type != V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE &&
        f->type != V4L2_BUF_TYPE_VIDEO_OUTPUT &&
        f->type != V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE)
        return -1;

    if (V4L2_TYPE_IS_OUTPUT(f->type)) {
        np = linlon_aeu_hw_pix_fmt_planes(get_hw_fmt(pix->pixelformat));
        if (np == 0xFFFF)
            return -1;
    }

    pix->field = V4L2_FIELD_NONE;
    if (!V4L2_TYPE_IS_MULTIPLANAR(f->type)) {
        if (np > 1) {
            f->type = (f->type == V4L2_BUF_TYPE_VIDEO_OUTPUT) ?
                    V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE :
                    V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;

            return aeu_try_format(ctx, f);
        }
        align_size(adev, &pix->height, &pix->width);
    } else {
        if (np == 1) {
            f->type = (f->type == V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE) ?
                    V4L2_BUF_TYPE_VIDEO_OUTPUT :
                    V4L2_BUF_TYPE_VIDEO_CAPTURE;
            return aeu_try_format(ctx, f);
        }
        align_size(adev, &pix_mp->height, &pix_mp->width);
        pix_mp->num_planes = np;
    }

    return 0;
}

static int linlon_aeu_set_fmt(aeu_ctx_t *ctx, struct v4l2_format *f)
{
    int ret = aeu_try_format(ctx, f);
    int type = AEU_HW_OUTPUT_BUF;
    struct v4l2_pix_format *fmt = &f->fmt.pix;
    enum aeu_hw_ds_format in_fmt;
    struct vb2_queue *vq = v4l2_m2m_get_vq(ctx->fh.m2m_ctx, f->type);
    u32 flag;

    if (ret)
        return ret;

    if (!vq)
        return -EINVAL;

    if (vb2_is_busy(vq)) {
        dev_err(ctx->adev->dev, "%s: queue busy\n", __func__);
        return -EBUSY;
    }

    if (V4L2_TYPE_IS_OUTPUT(f->type)) {
        type = AEU_HW_INPUT_BUF;
        vq->type = f->type;
        vq->is_multiplanar = V4L2_TYPE_IS_MULTIPLANAR(vq->type);
    }

    memset(&ctx->curr_buf_fmt[type], 0,
                sizeof(struct linlon_aeu_hw_buf_fmt));

    ctx->curr_buf_fmt[type].buf_type = type;
    ctx->curr_buf_fmt[type].buf_h = fmt->height;
    ctx->curr_buf_fmt[type].buf_w = fmt->width;
    in_fmt = get_hw_fmt_with_flag(fmt->pixelformat, &flag);
    if (in_fmt == 0xFFFFFFFF)
        return -EINVAL;

    if (type == AEU_HW_OUTPUT_BUF) {
        ctx->curr_buf_fmt[type].output_format =
                    linlon_aeu_hw_convert_fmt(in_fmt);
        ctx->curr_buf_fmt[type].nplanes = 1;
        ctx->curr_buf_fmt[type].size[0] = fmt->sizeimage;
    } else {
        u32 align = ctx->adev->hw_info.raddr_align;

        ctx->curr_buf_fmt[type].input_format = in_fmt;
        ctx->curr_buf_fmt[type].afbc_fmt_flags = flag;
        if (f->type == V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE) {
            struct v4l2_pix_format_mplane *fmt_mp = &f->fmt.pix_mp;
            uint i;

            ctx->curr_buf_fmt[type].nplanes = fmt_mp->num_planes;
            for(i = 0; i < fmt_mp->num_planes; i++) {
                ctx->curr_buf_fmt[type].stride[i] =
                    fmt_mp->plane_fmt[i].bytesperline;
                ctx->curr_buf_fmt[type].size[i] =
                    fmt_mp->plane_fmt[i].sizeimage;
                if (ctx->curr_buf_fmt[type].stride[i] % align)
                    return -EINVAL;
            }
        } else {
            ctx->curr_buf_fmt[type].nplanes = 1;
            ctx->curr_buf_fmt[type].size[0] = fmt->sizeimage;
            if (fmt->bytesperline % align)
                return -EINVAL;
            ctx->curr_buf_fmt[type].stride[0] = fmt->bytesperline;
        }
    }

    return 0;
}

static int aeu_try_fmt_vid_cap(struct file *file, void *p,
            struct v4l2_format *f)
{
    aeu_ctx_t *ctx = file2ctx(file);
    return aeu_try_format(ctx, f);
}

static int aeu_try_fmt_vid_out(struct file *file, void *p,
            struct v4l2_format *f)
{
    aeu_ctx_t *ctx = file2ctx(file);
    return aeu_try_format(ctx, f);
}

static int aeu_g_fmt_vid_cap(struct file *file, void *p,
            struct v4l2_format *f)
{
    return 0;
}

static int aeu_g_fmt_vid_out(struct file *file, void *p,
            struct v4l2_format *f)
{
    return 0;
}

static int aeu_s_fmt_vid_cap(struct file *file, void *p,
            struct v4l2_format *f)
{
    aeu_ctx_t *ctx = file2ctx(file);

    return linlon_aeu_set_fmt(ctx, f);
}

static int aeu_s_fmt_vid_out(struct file *file, void *p,
            struct v4l2_format *f)
{
    aeu_ctx_t *ctx = file2ctx(file);

    return linlon_aeu_set_fmt(ctx, f);
}

#ifdef CONFIG_VIDEO_ADV_DEBUG
static int aeu_g_reg(struct file *file, void *fh,
        struct v4l2_dbg_register *reg)
{
    u32 reg_idx = reg->reg & 0xFFFF;
    u32 table = (reg->reg >> 16) & 0xFFFF;
    aeu_ctx_t *ctx = file2ctx(file);

    reg->val = linlon_aeu_hw_g_reg(ctx->hw_ctx, table, reg_idx);
    return 0;
}
#endif

static const struct v4l2_ioctl_ops linlon_aeu_ioctl_ops = {
    .vidioc_querycap        = aeu_drv_cap,

    .vidioc_enum_fmt_vid_cap    = aeu_enum_fmt_vid_cap,
    .vidioc_g_fmt_vid_cap        = aeu_g_fmt_vid_cap,
    .vidioc_g_fmt_vid_cap_mplane    = aeu_g_fmt_vid_cap,
    .vidioc_try_fmt_vid_cap        = aeu_try_fmt_vid_cap,
    .vidioc_s_fmt_vid_cap        = aeu_s_fmt_vid_cap,
    .vidioc_s_fmt_vid_cap_mplane    = aeu_s_fmt_vid_cap,

    .vidioc_enum_fmt_vid_out    = aeu_enum_fmt_vid_out,
    .vidioc_g_fmt_vid_out        = aeu_g_fmt_vid_out,
    .vidioc_g_fmt_vid_out_mplane    = aeu_g_fmt_vid_out,
    .vidioc_try_fmt_vid_out        = aeu_try_fmt_vid_out,
    .vidioc_s_fmt_vid_out        = aeu_s_fmt_vid_out,
    .vidioc_s_fmt_vid_out_mplane    = aeu_s_fmt_vid_out,

    .vidioc_reqbufs        = v4l2_m2m_ioctl_reqbufs,
    .vidioc_querybuf    = v4l2_m2m_ioctl_querybuf,
    .vidioc_qbuf        = v4l2_m2m_ioctl_qbuf,
    .vidioc_dqbuf        = v4l2_m2m_ioctl_dqbuf,
    .vidioc_create_bufs    = v4l2_m2m_ioctl_create_bufs,
    .vidioc_expbuf        = v4l2_m2m_ioctl_expbuf,

    .vidioc_streamon    = v4l2_m2m_ioctl_streamon,
    .vidioc_streamoff    = v4l2_m2m_ioctl_streamoff,

    .vidioc_subscribe_event = v4l2_ctrl_subscribe_event,
    .vidioc_unsubscribe_event = v4l2_event_unsubscribe,

#ifdef CONFIG_VIDEO_ADV_DEBUG
    .vidioc_g_register    = aeu_g_reg,
#endif
};

/* m2m ops functions */
static
dma_addr_t get_aeu_buffer_plane_addr(aeu_ctx_t *ctx,
                     struct linlon_aeu_v4l2_buffer *buf,
                     u32 p)
{
    struct sg_table *dma_sgt;
    dma_addr_t    addr = (dma_addr_t)0;
    bool is_buf_output = false;

    /* AFBC buffer's offset is alway 0 */
    dma_addr_t offsets = (dma_addr_t)0;
    dma_sgt = vb2_plane_cookie(&buf->v4l2_buf.vb.vb2_buf, p);
    if (buf->v4l2_buf.vb.vb2_buf.vb2_queue->is_output) {
        is_buf_output = true;
        /* single plane buffer offset is always 0 */
        if (buf->v4l2_buf.vb.vb2_buf.num_planes > 1)
            offsets = (dma_addr_t)buf->v4l2_buf.vb.vb2_buf.planes[p].data_offset;
    }

    if (dma_sgt == NULL)
        return addr;

    if (ctx->adev->iommu) {
        struct sg_table *new = &buf->sg_tbl_bk[p];
        struct scatterlist *s, *d;
        u32 i;

        if (new->sgl) {
            sg_free_table(new);
            WARN_ON(1);
        }

        if (sg_alloc_table(new, dma_sgt->orig_nents, GFP_KERNEL))
            return addr;

        d = new->sgl;
        for_each_sg(dma_sgt->sgl, s, dma_sgt->orig_nents, i) {
#ifdef CONFIG_DMABUF_DEBUG
            s->page_link ^= ~0xffUL;
#endif
            sg_set_page(d, sg_page(s), s->length, s->offset);
            d = sg_next(d);
        }

        if (!dma_map_sg_attrs(ctx->adev->dev, new->sgl, new->nents,
                DMA_BIDIRECTIONAL,
                DMA_ATTR_SKIP_CPU_SYNC)) {
            dev_err(ctx->adev->dev, "dma_map_sg failed \n");
            sg_free_table(new);
            return addr;
        }
        addr = sg_dma_address(new->sgl);
    } else {
        if (dma_sgt->nents != 1)
            dev_err(ctx->adev->dev,
                "%s: physical address should be continuous!\n",
                __func__);
        addr = sg_dma_address(dma_sgt->sgl);
    }
    return addr + offsets;
}

static u32 linlon_aeu_afbc_body_offset(aeu_ctx_t *ctx,
        struct linlon_aeu_v4l2_buffer *buf)
{
    bool is_wide_block = buf->hw_fmt.afbc_fmt_flags &
                LINLON_AEU_HW_AFBC_SBA_32x8;
    u32 align_w, align_h, align_hd, nblocks;
    u32 block_size = 256; /* a super block is 16x16 or 32x8 = 256 */

    if (buf->hw_fmt.afbc_fmt_flags & LINLON_AEU_HW_AFBC_TH) {
        align_hd = 4096;
        align_w = 8;
        align_h = 8;
    } else {
        align_hd = 1024;
        align_w = 1;
        align_h = 1;
    }

    if (is_wide_block) {
        align_w <<= 5;
        align_h <<= 3;
    } else {
        align_w <<= 4;
        align_h <<= 4;
    }

    nblocks = ALIGN_UP(buf->hw_fmt.buf_w, align_w) *
          ALIGN_UP(buf->hw_fmt.buf_h, align_h) / block_size;

    return ALIGN_UP(nblocks << 4, align_hd);
}

static void aeu_m2m_device_run(void *priv)
{
    aeu_ctx_t *ctx = priv;
    struct linlon_aeu_v4l2_buffer *src_buf, *dst_buf;
    struct linlon_aeu_hw_buf_addr buf_addr;

    src_buf = to_aeu_v4l2_buf(v4l2_m2m_next_src_buf(ctx->fh.m2m_ctx));
    dst_buf = to_aeu_v4l2_buf(v4l2_m2m_next_dst_buf(ctx->fh.m2m_ctx));

    linlon_aeu_hw_set_buffer_fmt(ctx->hw_ctx, &src_buf->hw_fmt,
                    &dst_buf->hw_fmt);

    buf_addr.p0_addr = get_aeu_buffer_plane_addr(ctx, src_buf, 0);
    buf_addr.p1_addr = get_aeu_buffer_plane_addr(ctx, src_buf, 1);
    buf_addr.p2_addr = get_aeu_buffer_plane_addr(ctx, src_buf, 2);

    buf_addr.p0_stride = linlon_aeu_hw_plane_stride(&src_buf->hw_fmt, 0);
    buf_addr.p1_stride = linlon_aeu_hw_plane_stride(&src_buf->hw_fmt, 1);
    linlon_aeu_hw_set_buf_addr(ctx->hw_ctx, &buf_addr, AEU_HW_INPUT_BUF);

    buf_addr.p0_addr = get_aeu_buffer_plane_addr(ctx, dst_buf, 0);
    buf_addr.p1_addr = get_aeu_buffer_plane_addr(ctx, dst_buf, 1);
    if (buf_addr.p1_addr == (dma_addr_t)0)
        buf_addr.p1_addr = buf_addr.p0_addr +
                linlon_aeu_afbc_body_offset(ctx, dst_buf);

    linlon_aeu_hw_set_buf_addr(ctx->hw_ctx, &buf_addr, AEU_HW_OUTPUT_BUF);

    if (ctx->adev->status == AEU_PAUSED)
        return;

    linlon_aeu_hw_protected_mode(ctx->hw_ctx, ctx->pm_enabled);
    ctx->running = 1;
    if (linlon_aeu_hw_ctx_commit(ctx->hw_ctx)) {
        dev_err(ctx->adev->dev, "%s: hw commit error!\n", __func__);
        ctx->running = 0;
        v4l2_m2m_job_finish(ctx->adev->m2mdev, ctx->fh.m2m_ctx);
        linlon_aeu_hw_clear_ctrl(ctx->adev->hw_dev);
        wake_up(&ctx->idle);
    }
}

static int aeu_m2m_job_ready(void *priv)
{
    aeu_ctx_t *ctx = priv;

    if ((v4l2_m2m_num_src_bufs_ready(ctx->fh.m2m_ctx) < ctx->trans_num) ||
        (v4l2_m2m_num_dst_bufs_ready(ctx->fh.m2m_ctx) < ctx->trans_num))
        return 0;
    return 1;
}

static void aeu_m2m_job_abort(void *priv)
{
}

static const struct v4l2_m2m_ops linlon_aeu_m2m_ops = {
    .device_run    = aeu_m2m_device_run,
    .job_ready    = aeu_m2m_job_ready,
    .job_abort    = aeu_m2m_job_abort,
};

irqreturn_t linlon_aeu_irq_thread_handler(int irq, void *data)
{
    struct linlon_aeu_hw_device *hw_dev = data;
    struct v4l2_m2m_dev *m2mdev =
        linlon_aeu_hw_get_m2m_device(hw_dev);
    aeu_ctx_t *ctx = v4l2_m2m_get_curr_priv(m2mdev);
    struct vb2_v4l2_buffer *s_buf, *d_buf;
    unsigned long flags;
    u32 buf_flag = VB2_BUF_STATE_DONE;

    if (ctx == NULL) {
        pr_err("%s: context error!\n", __func__);
        return IRQ_HANDLED;
    }

    s_buf = v4l2_m2m_src_buf_remove(ctx->fh.m2m_ctx);
    d_buf = v4l2_m2m_dst_buf_remove(ctx->fh.m2m_ctx);

    if (linlon_aeu_hw_job_done(ctx->hw_ctx) == false)
        buf_flag = VB2_BUF_STATE_ERROR;

    spin_lock_irqsave(&ctx->buf_lock, flags);
    vb2_buffer_done(&s_buf->vb2_buf, buf_flag);
    vb2_buffer_done(&d_buf->vb2_buf, buf_flag);
    spin_unlock_irqrestore(&ctx->buf_lock, flags);

    ctx->running = 0;
    wake_up(&ctx->idle);

    v4l2_m2m_job_finish(ctx->adev->m2mdev, ctx->fh.m2m_ctx);

    return IRQ_HANDLED;
}

void linlon_aeu_paused(struct linlon_aeu_device *adev)
{
    aeu_ctx_t *ctx = v4l2_m2m_get_curr_priv(adev->m2mdev);

    WARN_ON(adev->status != AEU_PAUSED);

    if (ctx && ctx->running)
        wait_event(ctx->idle, ctx->running == 0);
}

void linlon_aeu_resume(struct linlon_aeu_device *adev)
{
    aeu_ctx_t *ctx = v4l2_m2m_get_curr_priv(adev->m2mdev);

    if (linlon_aeu_soft_reset(adev->hw_dev))
        dev_err(adev->dev, "%s: reset HW error!\n", __func__);

    adev->status = AEU_ACTIVE;

    if (ctx)
        v4l2_m2m_try_schedule(ctx->fh.m2m_ctx);
}

static linlon_aeu_hw_ctx_t *get_curr_hw_ctx(struct v4l2_m2m_dev *m2mdev)
{
    aeu_ctx_t *ctx = v4l2_m2m_get_curr_priv(m2mdev);
    if (ctx == NULL)
        return NULL;
    return ctx->hw_ctx;
}

int linlon_aeu_device_init(struct linlon_aeu_device *adev,
             struct platform_device *pdev, struct dentry *parent)
{
    int ret, irq;

    ret = v4l2_device_register(&pdev->dev, &adev->v4l2_dev);
    if (ret)
        return ret;

    irq = platform_get_irq(pdev, 0);

    if (irq < 0) {
        dev_err(&pdev->dev, "%s: failed to get irq number\n",
            __func__);
        return irq;
    }

    ret = devm_request_threaded_irq(&pdev->dev, irq,
                    linlon_aeu_hw_irq_handler,
                    linlon_aeu_irq_thread_handler,
                    IRQF_SHARED, AEU_NAME, adev->hw_dev);
    if (ret < 0) {
        dev_err(&pdev->dev, "%s: failed to request aeu irq\n",
            __func__);
        return ret;
    }

    snprintf(adev->vdev.name, sizeof(adev->vdev.name), "%s", AEU_NAME);
    adev->vdev.vfl_dir    = VFL_DIR_M2M;
    adev->vdev.release    = video_device_release_empty;
    adev->vdev.v4l2_dev    = &adev->v4l2_dev;
    adev->vdev.fops        = &linlon_aeu_fops;
    adev->vdev.ioctl_ops    = &linlon_aeu_ioctl_ops;
    adev->vdev.device_caps    = V4L2_CAP_VIDEO_M2M | V4L2_CAP_STREAMING;
    adev->vdev.minor    = -1;

    adev->dev = &pdev->dev;
    adev->dev->dma_parms = &adev->dma_parms;
    dma_set_max_seg_size(adev->dev, ~0u);

    mutex_init(&adev->aeu_mutex);
    adev->vdev.lock = &adev->aeu_mutex;
    video_set_drvdata(&adev->vdev, adev);

    adev->m2mdev = v4l2_m2m_init(&linlon_aeu_m2m_ops);
    if (IS_ERR(adev->m2mdev)) {
        dev_err(&pdev->dev, "%s: failed to init m2m device\n",
            __func__);
        ret = PTR_ERR(adev->m2mdev);
    }
    linlon_aeu_hw_connect_m2m_device(adev->hw_dev, adev->m2mdev,
                       get_curr_hw_ctx);

    adev->dbg_folder = parent;
    if (parent)
        linlon_aeu_log_init(adev->dbg_folder);

    ret = video_register_device(&adev->vdev, VFL_TYPE_VIDEO, 0);
    if (ret) {
        dev_err(&pdev->dev, "%s: video device register error\n",
            __func__);
        return ret;
    }
    dev_info(&pdev->dev, "%s: Device registered as /dev/video%d\n",
         __func__, adev->vdev.num);

    return ret;
}

int linlon_aeu_device_destroy(struct linlon_aeu_device *adev)
{
    v4l2_m2m_release(adev->m2mdev);
    video_unregister_device(&adev->vdev);
    v4l2_device_unregister(&adev->v4l2_dev);

    if (adev->dbg_folder) {
        linlon_aeu_log_exit();
        debugfs_remove_recursive(adev->dbg_folder);
        adev->dbg_folder = NULL;
    }

    return 0;
}
