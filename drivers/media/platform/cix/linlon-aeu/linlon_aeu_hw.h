// SPDX-License-Identifier: GPL-2.0
/*
 * (C) COPYRIGHT 2022-2023 Arm Technology (China) Co., Ltd.
 * ALL RIGHTS RESERVED
 *
 */

#ifndef _LINLON_AEU_HW_H_
#define _LINLON_AEU_HW_H_

#include <media/v4l2-device.h>
#include <media/v4l2-mem2mem.h>
#include <linux/interrupt.h>
#include <linux/of.h>
#include <linux/of_device.h>

#define ALIGN_UP(x, align_to)   (((x) + ((align_to)-1)) & ~((align_to)-1))

#define AEU_OFFSET    0x08000
#define AEU_DS_OFFSET    0x08200
#define AEU_AES_OFFSET    0x08400
#define AEU_TRUSTED    0x18000

#define _AES_REG(offset)    (AEU_AES_OFFSET + offset)
#define _DS_REG(offset)        (AEU_DS_OFFSET + offset)
#define _TRUSTED_REG(offset)    (AEU_TRUSTED + offset)

#define _BLK_INFO(base)        (base + 0x000)
#define _PPL_INFO(base)        (base + 0x004)
#define _IRQ_STATUS(base)    (base + 0x010)
#define _STATUS(base)        (base + 0x0B0)
#define _CONTROL(base)        (base + 0x0D0)

#define AEU_BLOCK_INFO        _BLK_INFO(AEU_OFFSET)
#define AEU_PIPELINE_INFO    _PPL_INFO(AEU_OFFSET)
#define AEU_IRQ_STATUS        _IRQ_STATUS(AEU_OFFSET)
#define AEU_STATUS        _STATUS(AEU_OFFSET)
#define AEU_CONTROL        _CONTROL(AEU_OFFSET)

#define AEU_IRQ_DS        (1 << 0)
#define AEU_IRQ_AES        (1 << 1)

#define AEU_CTRL_SRST        (1 << 16)
#define AEU_CTRL_PM        (1 << 20)

#define AEU_AES_BLOCK_INFO    _BLK_INFO(AEU_AES_OFFSET)
#define AEU_AES_PIPELINE_INFO    _PPL_INFO(AEU_AES_OFFSET)
#define AEU_AES_BLOCK_ID    _AES_REG(0x100)
#define AEU_AES_IRQ_RAW_STATUS    _AES_REG(0x104)
#define AEU_AES_IRQ_CLEAR    _AES_REG(0x108)
#define AEU_AES_IRQ_MASK    _AES_REG(0x10C)
#define AEU_AES_IRQ_STATUS    _AES_REG(0x110)
#define AEU_AES_COMMAND        _AES_REG(0x114)
#define AEU_AES_STATUS        _AES_REG(0x118)
#define AEU_AES_CONTROL        _AES_REG(0x11C)
#define AEU_AES_AXI_CONTROL    _AES_REG(0x120)
#define AEU_AES_OUT_P0_PTR_LOW    _AES_REG(0x140)
#define AEU_AES_OUT_P0_PTR_HIGH    _AES_REG(0x144)
#define AEU_AES_OUT_P1_PTR_LOW    _AES_REG(0x148)
#define AEU_AES_OUT_P1_PTR_HIGH    _AES_REG(0x14C)
#define AEU_AES_FORMAT        _AES_REG(0x150)
#define AEU_AES_IN_HSIZE    _AES_REG(0x154)
#define AEU_AES_IN_VSIZE    _AES_REG(0x158)
#define AEU_AES_BBOX_X_START    _AES_REG(0x15C)
#define AEU_AES_BBOX_X_END    _AES_REG(0x160)
#define AEU_AES_BBOX_Y_START    _AES_REG(0x164)
#define AEU_AES_BBOX_Y_END    _AES_REG(0x168)
#define AEU_AES_IN_P0_PTR_LOW    _AES_REG(0x16C)
#define AEU_AES_IN_P0_PTR_HIGH    _AES_REG(0x170)
#define AEU_AES_IN_P0_STRIDE    _AES_REG(0x174)
#define AEU_AES_IN_WRITE_ORDER    _AES_REG(0x178)
#define AEU_AES_IN_P1_PTR_LOW    _AES_REG(0x1EC)
#define AEU_AES_IN_P1_PTR_HIGH    _AES_REG(0x1F0)
#define AEU_AES_IN_P1_STRIDE    _AES_REG(0x1F4)

#define AES_IRQ_EOW        (1 << 0)
#define AES_IRQ_CFGS        (1 << 1)
#define AES_IRQ_ERR        (1 << 2)
#define AES_IRQ_TERR        (1 << 3)

#define AES_CTRL_EN        (1 << 0)
#define AES_CMD_DS        (1 << 0)

#define AES_AWQOS_BIT        0
#define AES_AWCACHE_BIT        4
#define AES_AWQOS_MASK        0xF
#define AES_AWCACHE_MASK    0xF

#define AEU_DS_BLOCK_INFO    _BLK_INFO(AEU_DS_OFFSET)
#define AEU_DS_PIPELINE_INFO    _PPL_INFO(AEU_DS_OFFSET)
#define AEU_DS_IRQ_RAW_STATUS    _DS_REG(0x0A0)
#define AEU_DS_IRQ_CLEAR    _DS_REG(0x0A4)
#define AEU_DS_IRQ_MASK        _DS_REG(0x0A8)
#define AEU_DS_IRQ_STATUS    _DS_REG(0x0AC)
#define AEU_DS_STATUS        _DS_REG(0x0B0)
#define AEU_DS_CONTROL        _DS_REG(0x0D0)
#define AEU_DS_CONFIG_VALID    _DS_REG(0x0D4)
#define AEU_DS_PROG_LINE    _DS_REG(0x0D8)
#define AEU_DS_AXI_CONTROL    _DS_REG(0x0DC)
#define AEU_DS_FORMAT        _DS_REG(0x0E0)
#define AEU_DS_IN_SIZE        _DS_REG(0x0E4)
#define AEU_DS_IN_P0_PTR_LOW    _DS_REG(0x100)
#define AEU_DS_IN_P0_PTR_HIGH    _DS_REG(0x104)
#define AEU_DS_IN_P0_STRIDE    _DS_REG(0x108)
#define AEU_DS_IN_P1_PTR_LOW    _DS_REG(0x110)
#define AEU_DS_IN_P1_PTR_HIGH    _DS_REG(0x114)
#define AEU_DS_IN_P1_STRIDE    _DS_REG(0x118)
#define AEU_DS_IN_P2_PTR_LOW    _DS_REG(0x120)
#define AEU_DS_IN_P2_PTR_HIGH    _DS_REG(0x124)
#define AEU_DS_OUT_P0_PTR_LOW    _DS_REG(0x130)
#define AEU_DS_OUT_P0_PTR_HIGH    _DS_REG(0x134)
#define AEU_DS_OUT_P0_STRIDE    _DS_REG(0x138)
#define AEU_DS_OUT_P1_PTR_LOW    _DS_REG(0x140)
#define AEU_DS_OUT_P1_PTR_HIGH    _DS_REG(0x144)
#define AEU_DS_OUT_P1_STRIDE    _DS_REG(0x148)

#define DS_IRQ_ERR        (1 << 2)
#define DS_IRQ_CVAL        (1 << 8)
#define DS_IRQ_PL        (1 << 9)

#define DS_OUTSTDCAPB_BIT    0
#define DS_ARCACHE_BIT        12
#define DS_BURSTLEN_BIT        16
#define DS_ARQOS_BIT        24
#define DS_ORD_BIT        31

#define DS_OUTSTDCAPB_MASK    0x1F
#define DS_ARCACHE_MASK        0xF
#define DS_BURSTLEN_MASK    0x3F
#define DS_ARQOS_MASK        0xF
#define DS_ORD_MASK        0x1

#define DS_CTRL_EN        (1 << 0)
#define DS_CTRL_TH        (1 << 1)
#define DS_CTRL_CVAL        (1 << 0)

#define DS_INPUT_SIZE(h, v)    (((h) & 0x1FFF) | (((v) & 0x1FFF) << 16))

enum aeu_hw_ds_format {
    ds_argb_2101010 = 0,
    ds_abgr_2101010,
    ds_rgba_1010102,
    ds_bgra_1010102,

    ds_argb_8888 = 8,
    ds_abgr_8888,
    ds_rgba_8888,
    ds_bgra_8888,

    ds_xrgb_8888 = 16,
    ds_xbgr_8888,
    ds_rgbx_8888,
    ds_bgrx_8888,

    ds_rgba_5551 = 32,
    ds_abgr_1555,
    ds_rgb_565,
    ds_bgr_565,

    ds_yuv_422_p2_8 = 41,
    ds_vyuv_422_p1_8,
    ds_yvyu_422_p1_8,

    ds_yuv_420_p2_8 = 46,
    ds_yuv_420_p3_8,

    ds_yuv_420_p2_10 = 55
};

enum aeu_hw_aes_format {
    aes_rgb_565 = 0,
    aes_rgba_5551,
    aes_rgba_1010102,
    aes_yuv_420_p2_10,
    aes_rgb_888,
    aes_rgba_8888,
    aes_rgba_4444,
    aes_r_8,
    aes_rg_88,
    aes_yuv_420_p2_8,
    aes_yuyv = 11,
    aes_y210 = 14,
};

struct linlon_aeu_hw_info {
    u32 min_width, min_height, max_width, max_height;
    u32 raddr_align;    /* read address alignment */
    u32 waddr_align;    /* write address alignment for afbc 1.0/1.1 */
    u32 waddr_align_afbc12;    /* write address alignment for afbc 1.2 */
};

/* buffer type */
#define AEU_HW_INPUT_BUF    0
#define AEU_HW_OUTPUT_BUF    1

/* AFBC flags (features) */
#define LINLON_AEU_HW_AFBC_YT    (1 << 0)
#define LINLON_AEU_HW_AFBC_BS    (1 << 1)
#define LINLON_AEU_HW_AFBC_SP    (1 << 2)
#define LINLON_AEU_HW_AFBC_SBA_16x16    (0 << 8)
#define LINLON_AEU_HW_AFBC_SBA_32x8    (1 << 8)
#define LINLON_AEU_HW_AFBC_TH    (1 << 10)
#define LINLON_AEU_HW_AFBC_SC    (1 << 11)

#define LINLON_AEU_HW_PLANES    3
struct linlon_aeu_hw_buf_fmt {
    u32 buf_h, buf_w;
    u32 nplanes;
    u32 stride[LINLON_AEU_HW_PLANES];
    u32 size[LINLON_AEU_HW_PLANES];
    u32 buf_type; /* input or output buffer */
    union {
        enum aeu_hw_ds_format input_format;
        enum aeu_hw_aes_format output_format;
    };
    /* For input format, it contains AFBC constraints.
     * For output format, it is modified by format modifiers.
     */
    u32 afbc_fmt_flags;
};

struct linlon_aeu_hw_buf_addr {
    dma_addr_t    p0_addr;
    dma_addr_t    p1_addr;
    dma_addr_t    p2_addr;

    int        p0_stride;
    int        p1_stride;
};

typedef struct linlon_aeu_hw_ctx linlon_aeu_hw_ctx_t;
struct linlon_aeu_hw_device;

struct linlon_aeu_hw_device *
linlon_aeu_hw_init(void __iomem *r, struct device *dev, struct fwnode_handle *fwnode,
         struct dentry *parent, struct linlon_aeu_hw_info *hw_info);
void linlon_aeu_hw_exit(struct linlon_aeu_hw_device *hw_dev);
void linlon_aeu_hw_release(struct linlon_aeu_hw_device *hw_dev);

irqreturn_t linlon_aeu_hw_irq_handler(int irq, void *data);

linlon_aeu_hw_ctx_t *
linlon_aeu_hw_init_ctx(struct linlon_aeu_hw_device *hw_dev);
void linlon_aeu_hw_free_ctx(linlon_aeu_hw_ctx_t *hw_ctx);
void linlon_aeu_hw_set_buffer_fmt(linlon_aeu_hw_ctx_t *hw_ctx,
    struct linlon_aeu_hw_buf_fmt *in_fmt,
    struct linlon_aeu_hw_buf_fmt *out_fmt);
int linlon_aeu_hw_ctx_commit(linlon_aeu_hw_ctx_t *hw_ctx);
void linlon_aeu_hw_set_buf_addr(linlon_aeu_hw_ctx_t *hw_ctx,
    struct linlon_aeu_hw_buf_addr *addr, u32 type);
void linlon_aeu_hw_connect_m2m_device(struct linlon_aeu_hw_device *hw_dev,
        struct v4l2_m2m_dev *m2mdev,
        linlon_aeu_hw_ctx_t* (*cb)(struct v4l2_m2m_dev *));
struct v4l2_m2m_dev *
linlon_aeu_hw_get_m2m_device(struct linlon_aeu_hw_device *hw_dev);
enum aeu_hw_aes_format linlon_aeu_hw_convert_fmt(enum aeu_hw_ds_format ifmt);
u16 linlon_aeu_hw_pix_fmt_planes(enum aeu_hw_ds_format ifmt);
u32 linlon_aeu_hw_plane_stride(struct linlon_aeu_hw_buf_fmt *bf, u32 n);
bool linlon_aeu_hw_pix_fmt_native(enum aeu_hw_ds_format ifmt);
u32 linlon_aeu_hw_plane_size(struct linlon_aeu_hw_buf_fmt *bf, u32 n);
u32 linlon_aeu_hw_g_reg(linlon_aeu_hw_ctx_t *hw_ctx, u32 table, u32 reg);
bool linlon_aeu_hw_job_done(struct linlon_aeu_hw_ctx *hw_ctx);
void linlon_aeu_hw_prepare(struct linlon_aeu_hw_device *hw_dev);
void linlon_aeu_hw_active(struct linlon_aeu_hw_device *hw_dev);
void linlon_aeu_hw_deactive(struct linlon_aeu_hw_device *hw_dev);
int linlon_aeu_soft_reset(struct linlon_aeu_hw_device *hw_dev);
void linlon_aeu_hw_protected_mode(struct linlon_aeu_hw_ctx *hw_ctx, u32 enable);
void linlon_aeu_hw_clear_ctrl(struct linlon_aeu_hw_device *hw_dev);
#endif
