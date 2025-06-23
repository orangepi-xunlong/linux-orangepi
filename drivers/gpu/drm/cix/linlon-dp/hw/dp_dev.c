// SPDX-License-Identifier: GPL-2.0
/*
 * (C) COPYRIGHT 2022-2023 Arm Technology (China) Co., Ltd.
 * ALL RIGHTS RESERVED
 *
 */

#include <drm/drm_blend.h>
#include <drm/drm_print.h>
#include "dp_dev.h"
#include "linlondp_io.h"

static u64 get_lpu_event(struct dp_pipeline *dp_pipeline)
{
    u32 __iomem *reg = dp_pipeline->lpu_addr;
    u32 status, raw_status;
    u64 evts = 0ULL;

    raw_status = linlondp_read32(reg, BLK_IRQ_RAW_STATUS);
    if (raw_status & LPU_IRQ_IBSY)
        evts |= LINLONDP_EVENT_IBSY;
    if (raw_status & LPU_IRQ_EOW)
        evts |= LINLONDP_EVENT_EOW;
    if (raw_status & LPU_IRQ_OVR)
        evts |= LINLONDP_EVENT_OVR;

    if (raw_status & (LPU_IRQ_ERR | LPU_IRQ_IBSY | LPU_IRQ_OVR)) {
        u32 restore = 0, tbu_status;
        /* Check error of LPU status */
        status = linlondp_read32(reg, BLK_STATUS);
        if (status & LPU_STATUS_AXIE) {
            restore |= LPU_STATUS_AXIE;
            evts |= LINLONDP_ERR_AXIE;
        }
        if (status & LPU_STATUS_ACE0) {
            restore |= LPU_STATUS_ACE0;
            evts |= LINLONDP_ERR_ACE0;
        }
        if (status & LPU_STATUS_ACE1) {
            restore |= LPU_STATUS_ACE1;
            evts |= LINLONDP_ERR_ACE1;
        }
        if (status & LPU_STATUS_ACE2) {
            restore |= LPU_STATUS_ACE2;
            evts |= LINLONDP_ERR_ACE2;
        }
        if (status & LPU_STATUS_ACE3) {
            restore |= LPU_STATUS_ACE3;
            evts |= LINLONDP_ERR_ACE3;
        }
        if (status & LPU_STATUS_FEMPTY) {
            restore |= LPU_STATUS_FEMPTY;
            evts |= LINLONDP_EVENT_EMPTY;
        }
        if (status & LPU_STATUS_FFULL) {
            restore |= LPU_STATUS_FFULL;
            evts |= LINLONDP_EVENT_FULL;
        }

        if (restore != 0)
            linlondp_write32_mask(reg, BLK_STATUS, restore, 0);

        restore = 0;
        /* Check errors of TBU status */
        tbu_status = linlondp_read32(reg, LPU_TBU_STATUS);
        if (tbu_status & LPU_TBU_STATUS_TCF) {
            restore |= LPU_TBU_STATUS_TCF;
            evts |= LINLONDP_ERR_TCF;
        }
        if (tbu_status & LPU_TBU_STATUS_TTNG) {
            restore |= LPU_TBU_STATUS_TTNG;
            evts |= LINLONDP_ERR_TTNG;
        }
        if (tbu_status & LPU_TBU_STATUS_TITR) {
            restore |= LPU_TBU_STATUS_TITR;
            evts |= LINLONDP_ERR_TITR;
        }
        if (tbu_status & LPU_TBU_STATUS_TEMR) {
            restore |= LPU_TBU_STATUS_TEMR;
            evts |= LINLONDP_ERR_TEMR;
        }
        if (tbu_status & LPU_TBU_STATUS_TTF) {
            restore |= LPU_TBU_STATUS_TTF;
            evts |= LINLONDP_ERR_TTF;
        }
        if (restore != 0)
            linlondp_write32_mask(reg, LPU_TBU_STATUS, restore, 0);
    }

    linlondp_write32(reg, BLK_IRQ_CLEAR, raw_status);
    return evts;
}

static u64 get_cu_event(struct dp_pipeline *dp_pipeline)
{
    u32 __iomem *reg = dp_pipeline->cu_addr;
    u32 status, raw_status;
    u64 evts = 0ULL;

    raw_status = linlondp_read32(reg, BLK_IRQ_RAW_STATUS);
    if (raw_status & CU_IRQ_OVR)
        evts |= LINLONDP_EVENT_OVR;

    if (raw_status & (CU_IRQ_ERR | CU_IRQ_OVR)) {
        status = linlondp_read32(reg, BLK_STATUS) & 0x7FFFFFFF;
        if (status & CU_STATUS_CPE)
            evts |= LINLONDP_ERR_CPE;
        if (status & CU_STATUS_ZME)
            evts |= LINLONDP_ERR_ZME;
        if (status & CU_STATUS_CFGE)
            evts |= LINLONDP_ERR_CFGE;
        if (status)
            linlondp_write32_mask(reg, BLK_STATUS, status, 0);
    }

    linlondp_write32(reg, BLK_IRQ_CLEAR, raw_status);

    return evts;
}

static u64 get_dou_event(struct dp_pipeline *dp_pipeline)
{
    u32 __iomem *reg = dp_pipeline->dou_addr;
    u32 status, raw_status;
    u64 evts = 0ULL;

    raw_status = linlondp_read32(reg, BLK_IRQ_RAW_STATUS);
    if (raw_status & DOU_IRQ_PL0)
        evts |= LINLONDP_EVENT_VSYNC;
    if (raw_status & DOU_IRQ_UND)
        evts |= LINLONDP_EVENT_URUN;

    if (raw_status & (DOU_IRQ_ERR | DOU_IRQ_UND)) {
        u32 restore  = 0;

        status = linlondp_read32(reg, BLK_STATUS);
        if (status & DOU_STATUS_DRIFTTO) {
            restore |= DOU_STATUS_DRIFTTO;
            evts |= LINLONDP_ERR_DRIFTTO;
        }
        if (status & DOU_STATUS_FRAMETO) {
            restore |= DOU_STATUS_FRAMETO;
            evts |= LINLONDP_ERR_FRAMETO;
        }
        if (status & DOU_STATUS_TETO) {
            restore |= DOU_STATUS_TETO;
            evts |= LINLONDP_ERR_TETO;
        }
        if (status & DOU_STATUS_CSCE) {
            restore |= DOU_STATUS_CSCE;
            evts |= LINLONDP_ERR_CSCE;
        }

        if (restore != 0)
            linlondp_write32_mask(reg, BLK_STATUS, restore, 0);
    }

    linlondp_write32(reg, BLK_IRQ_CLEAR, raw_status);
    return evts;
}

static u64 get_pipeline_event(struct dp_pipeline *dp_pipeline, u32 gcu_status)
{
    u32 evts = 0ULL;

    if (gcu_status & (GLB_IRQ_STATUS_LPU0 | GLB_IRQ_STATUS_LPU1))
        evts |= get_lpu_event(dp_pipeline);

    if (gcu_status & (GLB_IRQ_STATUS_CU0 | GLB_IRQ_STATUS_CU1))
        evts |= get_cu_event(dp_pipeline);

    if (gcu_status & (GLB_IRQ_STATUS_DOU0 | GLB_IRQ_STATUS_DOU1))
        evts |= get_dou_event(dp_pipeline);

    return evts;
}

static irqreturn_t
dp_irq_handler(struct linlondp_dev *mdev, struct linlondp_events *evts)
{
    struct dp_dev *dp = mdev->chip_data;
    u32 status, gcu_status, raw_status;

    gcu_status = linlondp_read32(dp->gcu_addr, GLB_IRQ_STATUS);

    if (gcu_status & GLB_IRQ_STATUS_GCU) {
        raw_status = linlondp_read32(dp->gcu_addr, BLK_IRQ_RAW_STATUS);
        if (raw_status & GCU_IRQ_CVAL0)
            evts->pipes[0] |= LINLONDP_EVENT_FLIP;
        if (raw_status & GCU_IRQ_CVAL1)
            evts->pipes[1] |= LINLONDP_EVENT_FLIP;
        if (raw_status & GCU_IRQ_ERR) {
            status = linlondp_read32(dp->gcu_addr, BLK_STATUS);
            if (status & GCU_STATUS_MERR) {
                evts->global |= LINLONDP_ERR_MERR;
                linlondp_write32_mask(dp->gcu_addr, BLK_STATUS,
                            GCU_STATUS_MERR, 0);
            }
        }

        linlondp_write32(dp->gcu_addr, BLK_IRQ_CLEAR, raw_status);
    }

    if (gcu_status & GLB_IRQ_STATUS_PIPE0)
        evts->pipes[0] |= get_pipeline_event(dp->pipes[0], gcu_status);

    if (gcu_status & GLB_IRQ_STATUS_PIPE1)
        evts->pipes[1] |= get_pipeline_event(dp->pipes[1], gcu_status);

    return IRQ_RETVAL(gcu_status);
}

#define ENABLED_GCU_IRQS    (GCU_IRQ_CVAL0 | GCU_IRQ_CVAL1 | \
                 GCU_IRQ_MODE | GCU_IRQ_ERR)
#define ENABLED_LPU_IRQS    (LPU_IRQ_IBSY | LPU_IRQ_ERR | LPU_IRQ_EOW)
#define ENABLED_CU_IRQS        (CU_IRQ_OVR | CU_IRQ_ERR)
#define ENABLED_DOU_IRQS    (DOU_IRQ_UND | DOU_IRQ_ERR)

static int dp_enable_irq(struct linlondp_dev *mdev)
{
    struct dp_dev *dp = mdev->chip_data;
    struct dp_pipeline *pipe;
    u32 i;

    linlondp_write32_mask(dp->gcu_addr, BLK_IRQ_MASK,
                ENABLED_GCU_IRQS, ENABLED_GCU_IRQS);
    for (i = 0; i < dp->num_pipelines; i++) {
        pipe = dp->pipes[i];
        linlondp_write32_mask(pipe->cu_addr,  BLK_IRQ_MASK,
                    ENABLED_CU_IRQS, ENABLED_CU_IRQS);
        linlondp_write32_mask(pipe->lpu_addr, BLK_IRQ_MASK,
                    ENABLED_LPU_IRQS, ENABLED_LPU_IRQS);
        linlondp_write32_mask(pipe->dou_addr, BLK_IRQ_MASK,
                    ENABLED_DOU_IRQS, ENABLED_DOU_IRQS);
    }
    return 0;
}

static int dp_disable_irq(struct linlondp_dev *mdev)
{
    struct dp_dev *dp = mdev->chip_data;
    struct dp_pipeline *pipe;
    u32 i;

    linlondp_write32_mask(dp->gcu_addr, BLK_IRQ_MASK, ENABLED_GCU_IRQS, 0);
    for (i = 0; i < dp->num_pipelines; i++) {
        pipe = dp->pipes[i];
        linlondp_write32_mask(pipe->cu_addr,  BLK_IRQ_MASK,
                    ENABLED_CU_IRQS, 0);
        linlondp_write32_mask(pipe->lpu_addr, BLK_IRQ_MASK,
                    ENABLED_LPU_IRQS, 0);
        linlondp_write32_mask(pipe->dou_addr, BLK_IRQ_MASK,
                    ENABLED_DOU_IRQS, 0);
    }
    return 0;
}

static void dp_on_off_vblank(struct linlondp_dev *mdev, int master_pipe, bool on)
{
    struct dp_dev *dp = mdev->chip_data;
    struct dp_pipeline *pipe = dp->pipes[master_pipe];

    linlondp_write32_mask(pipe->dou_addr, BLK_IRQ_MASK,
                DOU_IRQ_PL0, on ? DOU_IRQ_PL0 : 0);
}

static int to_dp_opmode(int core_mode)
{
    switch (core_mode) {
    case LINLONDP_MODE_DISP0:
        return DO0_ACTIVE_MODE;
    case LINLONDP_MODE_DISP1:
        return DO1_ACTIVE_MODE;
    case LINLONDP_MODE_DUAL_DISP:
        return DO01_ACTIVE_MODE;
    case LINLONDP_MODE_INACTIVE:
        return INACTIVE_MODE;
    default:
        WARN(1, "Unknown operation mode");
        return INACTIVE_MODE;
    }
}

static int dp_change_opmode(struct linlondp_dev *mdev, int new_mode)
{
    struct dp_dev *dp = mdev->chip_data;
    u32 opmode = to_dp_opmode(new_mode);
    int ret;

    linlondp_write32_mask(dp->gcu_addr, BLK_CONTROL, 0x7, opmode);

    ret = dp_wait_cond(((linlondp_read32(dp->gcu_addr, BLK_CONTROL) & 0x7) == opmode),
               100, 1000, 10000);

    return ret;
}

static void dp_flush(struct linlondp_dev *mdev,
              int master_pipe, u32 active_pipes)
{
    struct dp_dev *dp = mdev->chip_data;
    u32 reg_offset = (master_pipe == 0) ?
             GCU_CONFIG_VALID0 : GCU_CONFIG_VALID1;

    linlondp_write32(dp->gcu_addr, reg_offset, GCU_CONFIG_CVAL);
}

static int dp_reset(struct dp_dev *dp)
{
    u32 __iomem *gcu = dp->gcu_addr;
    int ret;

    linlondp_write32(gcu, BLK_CONTROL, GCU_CONTROL_SRST);

    ret = dp_wait_cond(!(linlondp_read32(gcu, BLK_CONTROL) & GCU_CONTROL_SRST),
               100, 1000, 10000);

    return ret;
}

void dp_read_block_header(u32 __iomem *reg, struct block_header *blk)
{
    int i;

    blk->block_info = linlondp_read32(reg, BLK_BLOCK_INFO);
    if (BLOCK_INFO_BLK_TYPE(blk->block_info) == DP_BLK_TYPE_RESERVED)
        return;

    blk->pipeline_info = linlondp_read32(reg, BLK_PIPELINE_INFO);

    /* get valid input and output ids */
    for (i = 0; i < PIPELINE_INFO_N_VALID_INPUTS(blk->pipeline_info); i++)
        blk->input_ids[i] = linlondp_read32(reg + i, BLK_VALID_INPUT_ID0);
    for (i = 0; i < PIPELINE_INFO_N_OUTPUTS(blk->pipeline_info); i++)
        blk->output_ids[i] = linlondp_read32(reg + i, BLK_OUTPUT_ID0);
}

static void dp_cleanup(struct linlondp_dev *mdev)
{
    struct dp_dev *dp = mdev->chip_data;

    if (!dp)
        return;

    linlondp_coeffs_destroy_manager(dp->it_mgr);
    devm_kfree(mdev->dev, dp);
    mdev->chip_data = NULL;
}

static int dp_enum_resources(struct linlondp_dev *mdev)
{
    struct dp_dev *dp;
    struct linlondp_pipeline *pipe;
    struct block_header blk;
    u32 __iomem *blk_base;
    u32 i, value, offset, coeffs_size;
    int err;

    dp = devm_kzalloc(mdev->dev, sizeof(*dp), GFP_KERNEL);
    if (!dp)
        return -ENOMEM;

    mdev->chip_data = dp;
    dp->mdev = mdev;
    dp->gcu_addr = mdev->reg_base;
    dp->periph_addr = mdev->reg_base + (DP_BLOCK_OFFSET_PERIPH >> 2);

    if (!mdev->enabled_by_gop) {
        err = dp_reset(dp);
        if (err) {
            DRM_ERROR("Fail to reset dp device.\n");
            goto err_cleanup;
        }
    }

    /* probe GCU */
    value = linlondp_read32(dp->gcu_addr, GLB_CORE_INFO);
    dp->num_blocks = value & 0xFF;
    dp->num_pipelines = (value >> 8) & 0x7;

    if (dp->num_pipelines > DP_MAX_PIPELINE) {
        DRM_ERROR("dp supports %d pipelines, but got: %d.\n",
              DP_MAX_PIPELINE, dp->num_pipelines);
        err = -EINVAL;
        goto err_cleanup;
    }

    /* Only the legacy HW has the periph block, the newer merges the periph
     * into GCU
     */
    value = linlondp_read32(dp->periph_addr, BLK_BLOCK_INFO);
    if (BLOCK_INFO_BLK_TYPE(value) != DP_BLK_TYPE_PERIPH)
        dp->periph_addr = NULL;

    if (dp->periph_addr) {
        /* probe PERIPHERAL in legacy HW */
        value = linlondp_read32(dp->periph_addr, PERIPH_CONFIGURATION_ID);

        dp->max_line_size    = value & PERIPH_MAX_LINE_SIZE ? 4096 : 2048;
        dp->max_vsize        = 4096;
        dp->num_rich_layers    = value & PERIPH_NUM_RICH_LAYERS ? 2 : 1;
        dp->supports_dual_link    = !!(value & PERIPH_SPLIT_EN);
        dp->integrates_tbu    = !!(value & PERIPH_TBU_EN);
    } else {
        value = linlondp_read32(dp->gcu_addr, GCU_CONFIGURATION_ID0);
        dp->max_line_size    = GCU_MAX_LINE_SIZE(value);
        dp->max_vsize        = GCU_MAX_NUM_LINES(value);

        value = linlondp_read32(dp->gcu_addr, GCU_CONFIGURATION_ID1);
        dp->num_rich_layers    = GCU_NUM_RICH_LAYERS(value);
        dp->supports_dual_link    = GCU_DISPLAY_SPLIT_EN(value);
        dp->integrates_tbu    = GCU_DISPLAY_TBU_EN(value);
    }

    for (i = 0; i < dp->num_pipelines; i++) {
        pipe = linlondp_pipeline_add(mdev, sizeof(struct dp_pipeline),
                       &dp_pipeline_funcs);
        if (IS_ERR(pipe)) {
            err = PTR_ERR(pipe);
            goto err_cleanup;
        }

        /* DP HW doesn't update shadow registers when display output
         * is turning off, so when we disable all pipeline components
         * together with display output disable by one flush or one
         * operation, the disable operation updated registers will not
         * be flush to or valid in HW, which may leads problem.
         * To workaround this problem, introduce a two phase disable.
         * Phase1: Disabling components with display is on to make sure
         *       the disable can be flushed to HW.
         * Phase2: Only turn-off display output.
         */
        value = LINLONDP_PIPELINE_IMPROCS |
            BIT(LINLONDP_COMPONENT_TIMING_CTRLR);

        pipe->standalone_disabled_comps = value;

        dp->pipes[i] = to_dp_pipeline(pipe);
    }

    coeffs_size = GLB_LT_COEFF_NUM * sizeof(u32);
    dp->it_mgr = linlondp_coeffs_create_manager(coeffs_size);
    dp->ft_mgr = dp->it_mgr;
    dp->it_s_mgr = dp->it_mgr;


    /* loop the register blks and probe.
     * NOTE: dp->num_blocks includes reserved blocks.
     * dp->num_blocks = GCU + valid blocks + reserved blocks
     */
    i = 1; /* exclude GCU */
    offset = DP_BLOCK_SIZE; /* skip GCU */
    while (i < dp->num_blocks) {
        blk_base = mdev->reg_base + (offset >> 2);

        dp_read_block_header(blk_base, &blk);
        if (BLOCK_INFO_BLK_TYPE(blk.block_info) != DP_BLK_TYPE_RESERVED) {
            err = dp_probe_block(dp, &blk, blk_base);
            if (err)
                goto err_cleanup;
        }

        i++;
        offset += DP_BLOCK_SIZE;
    }

    DRM_DEBUG("total %d (out of %d) blocks are found.\n",
          i, dp->num_blocks);

    return 0;

err_cleanup:
    dp_cleanup(mdev);
    return err;
}

#define __HW_ID(__group, __format) \
    ((((__group) & 0x7) << 3) | ((__format) & 0x7))

#define RICH        LINLONDP_FMT_RICH_LAYER
#define SIMPLE        LINLONDP_FMT_SIMPLE_LAYER
#define RICH_SIMPLE    (LINLONDP_FMT_RICH_LAYER | LINLONDP_FMT_SIMPLE_LAYER)
#define RICH_WB        (LINLONDP_FMT_RICH_LAYER | LINLONDP_FMT_WB_LAYER)
#define RICH_SIMPLE_WB    (RICH_SIMPLE | LINLONDP_FMT_WB_LAYER)

#define Rot_0        DRM_MODE_ROTATE_0
#define Flip_H_V    (DRM_MODE_REFLECT_X | DRM_MODE_REFLECT_Y | Rot_0)
#define Rot_ALL_H_V    (DRM_MODE_ROTATE_MASK | Flip_H_V)

#define LYT_NM        BIT(AFBC_FORMAT_MOD_BLOCK_SIZE_16x16)
#define LYT_WB        BIT(AFBC_FORMAT_MOD_BLOCK_SIZE_32x8)
#define LYT_NM_WB    (LYT_NM | LYT_WB)

#define AFB_TH        AFBC(_TILED | _SPARSE)
#define AFB_TH_SC_YTR    AFBC(_TILED | _SC | _SPARSE | _YTR)
#define AFB_TH_SC_YTR_BS AFBC(_TILED | _SC | _SPARSE | _YTR | _SPLIT)

static struct linlondp_format_caps dp_format_caps_table[] = {
    /*   HW_ID    |        fourcc         |   layer_types |   rots    | afbc_layouts | afbc_features */
    /* ABGR_2101010*/
    {__HW_ID(0, 0),    DRM_FORMAT_ARGB2101010,    RICH_SIMPLE_WB,    Flip_H_V,        0, 0},
    {__HW_ID(0, 1),    DRM_FORMAT_ABGR2101010,    RICH_SIMPLE_WB,    Flip_H_V,        0, 0},
    {__HW_ID(0, 1),    DRM_FORMAT_ABGR2101010,    RICH_SIMPLE,    Rot_ALL_H_V,    LYT_NM_WB, AFB_TH_SC_YTR_BS}, /* afbc */
    {__HW_ID(0, 2),    DRM_FORMAT_RGBA1010102,    RICH_SIMPLE_WB,    Flip_H_V,        0, 0},
    {__HW_ID(0, 3),    DRM_FORMAT_BGRA1010102,    RICH_SIMPLE_WB,    Flip_H_V,        0, 0},
    /* ABGR_8888*/
    {__HW_ID(1, 0),    DRM_FORMAT_ARGB8888,    RICH_SIMPLE_WB,    Flip_H_V,        0, 0},
    {__HW_ID(1, 1),    DRM_FORMAT_ABGR8888,    RICH_SIMPLE_WB,    Flip_H_V,        0, 0},
    {__HW_ID(1, 1),    DRM_FORMAT_ABGR8888,    RICH_SIMPLE,    Rot_ALL_H_V,    LYT_NM_WB, AFB_TH_SC_YTR_BS}, /* afbc */
    {__HW_ID(1, 2),    DRM_FORMAT_RGBA8888,    RICH_SIMPLE_WB,    Flip_H_V,        0, 0},
    {__HW_ID(1, 3),    DRM_FORMAT_BGRA8888,    RICH_SIMPLE_WB,    Flip_H_V,        0, 0},
    /* XBGB_8888 */
    {__HW_ID(2, 0),    DRM_FORMAT_XRGB8888,    RICH_SIMPLE_WB,    Flip_H_V,        0, 0},
    {__HW_ID(2, 1),    DRM_FORMAT_XBGR8888,    RICH_SIMPLE_WB,    Flip_H_V,        0, 0},
    {__HW_ID(2, 2),    DRM_FORMAT_RGBX8888,    RICH_SIMPLE_WB,    Flip_H_V,        0, 0},
    {__HW_ID(2, 3),    DRM_FORMAT_BGRX8888,    RICH_SIMPLE_WB,    Flip_H_V,        0, 0},
    /* BGR_888 */ /* none-afbc RGB888 doesn't support rotation and flip */
    {__HW_ID(3, 0),    DRM_FORMAT_RGB888,    RICH_SIMPLE_WB,    Rot_0,            0, 0},
    {__HW_ID(3, 1),    DRM_FORMAT_BGR888,    RICH_SIMPLE_WB,    Rot_0,            0, 0},
    {__HW_ID(3, 1),    DRM_FORMAT_BGR888,    RICH_SIMPLE,    Rot_ALL_H_V,    LYT_NM_WB, AFB_TH_SC_YTR_BS}, /* afbc */
    /* BGR 16bpp */
    {__HW_ID(4, 0),    DRM_FORMAT_RGBA5551,    RICH_SIMPLE,    Flip_H_V,        0, 0},
    {__HW_ID(4, 1),    DRM_FORMAT_ABGR1555,    RICH_SIMPLE,    Flip_H_V,        0, 0},
    {__HW_ID(4, 1),    DRM_FORMAT_ABGR1555,    RICH_SIMPLE,    Rot_ALL_H_V,    LYT_NM_WB, AFB_TH_SC_YTR}, /* afbc */
    {__HW_ID(4, 2),    DRM_FORMAT_RGB565,    RICH_SIMPLE,    Flip_H_V,        0, 0},
    {__HW_ID(4, 3),    DRM_FORMAT_BGR565,    RICH_SIMPLE,    Flip_H_V,        0, 0},
    {__HW_ID(4, 3),    DRM_FORMAT_BGR565,    RICH_SIMPLE,    Rot_ALL_H_V,    LYT_NM_WB, AFB_TH_SC_YTR}, /* afbc */
    /* YUV 444/422/420 8bit  */
    {__HW_ID(5, 1),    DRM_FORMAT_YUYV,    RICH,        Rot_ALL_H_V,    LYT_NM, AFB_TH}, /* afbc */
    {__HW_ID(5, 2),    DRM_FORMAT_YUYV,    RICH,        Flip_H_V,        0, 0},
    {__HW_ID(5, 3),    DRM_FORMAT_UYVY,    RICH,        Flip_H_V,        0, 0},
    {__HW_ID(5, 6),    DRM_FORMAT_NV12,    RICH_WB,        Flip_H_V,        0, 0},
    {__HW_ID(5, 6),    DRM_FORMAT_YUV420_8BIT,    RICH,        Rot_ALL_H_V,    LYT_NM, AFB_TH}, /* afbc */
    {__HW_ID(5, 7),    DRM_FORMAT_YUV420,    RICH,        Flip_H_V,        0, 0},
    /* YUV 10bit*/
    {__HW_ID(6, 6),    DRM_FORMAT_X0L2,    RICH,        Flip_H_V,        0, 0},
    {__HW_ID(6, 7),    DRM_FORMAT_P010,    RICH,        Flip_H_V,        0, 0},
    {__HW_ID(6, 7),    DRM_FORMAT_YUV420_10BIT, RICH,        Rot_ALL_H_V,    LYT_NM, AFB_TH},
};

static bool dp_format_mod_supported(const struct linlondp_format_caps *caps,
                     u32 layer_type, u64 modifier, u32 rot)
{
    uint64_t layout = modifier & AFBC_FORMAT_MOD_BLOCK_SIZE_MASK;

    if ((layout == AFBC_FORMAT_MOD_BLOCK_SIZE_32x8) &&
        drm_rotation_90_or_270(rot)) {
        DRM_DEBUG_ATOMIC("DP doesn't support ROT90 for WB-AFBC.\n");
        return false;
    }

    return true;
}

static void dp_init_fmt_tbl(struct linlondp_dev *mdev)
{
    struct linlondp_format_caps_table *table = &mdev->fmt_tbl;

    table->format_caps = dp_format_caps_table;
    table->format_mod_supported = dp_format_mod_supported;
    table->n_formats = ARRAY_SIZE(dp_format_caps_table);
}

static int dp_connect_iommu(struct linlondp_dev *mdev)
{
    struct dp_dev *dp = mdev->chip_data;
    u32 __iomem *reg = dp->gcu_addr;
    u32 check_bits = (dp->num_pipelines == 2) ?
             GCU_STATUS_TCS0 | GCU_STATUS_TCS1 : GCU_STATUS_TCS0;
    int i, ret;

    if (!dp->integrates_tbu)
    {
        DRM_WARN("Connect mmu without internal TBU!\n");
        return 0;
    }
    linlondp_write32_mask(reg, BLK_CONTROL, 0x7, TBU_CONNECT_MODE);

    ret = dp_wait_cond(has_bits(check_bits, linlondp_read32(reg, BLK_STATUS)),
            100, 1000, 1000);
    if (ret < 0) {
        DRM_ERROR("timed out connecting to TCU!\n");
        linlondp_write32_mask(reg, BLK_CONTROL, 0x7, INACTIVE_MODE);
        return ret;
    }

    for (i = 0; i < dp->num_pipelines; i++)
        linlondp_write32_mask(dp->pipes[i]->lpu_addr, LPU_TBU_CONTROL,
                    LPU_TBU_CTRL_TLBPEN, LPU_TBU_CTRL_TLBPEN);
    return 0;
}

static int dp_disconnect_iommu(struct linlondp_dev *mdev)
{
    struct dp_dev *dp = mdev->chip_data;
    u32 __iomem *reg = dp->gcu_addr;
    u32 check_bits = (dp->num_pipelines == 2) ?
             GCU_STATUS_TCS0 | GCU_STATUS_TCS1 : GCU_STATUS_TCS0;
    int ret;

    if (!dp->integrates_tbu)
    {
        DRM_WARN("Disconnect mmu without internal TBU!\n");
        return 0;
    }

    linlondp_write32_mask(reg, BLK_CONTROL, 0x7, TBU_DISCONNECT_MODE);

    ret = dp_wait_cond(((linlondp_read32(reg, BLK_STATUS) & check_bits) == 0),
            100, 1000, 1000);
    if (ret < 0) {
        DRM_ERROR("timed out disconnecting from TCU!\n");
        linlondp_write32_mask(reg, BLK_CONTROL, 0x7, INACTIVE_MODE);
    }

    return ret;
}


static int dp_init_hw(struct linlondp_dev *mdev)
{
    struct dp_dev *dp = mdev->chip_data;
    int i, err = 0;

    for (i = 0; i < dp->num_pipelines; i++) {
        err = dp_pipeline_config_axi(dp->pipes[i]);
        if (err)
            break;
    }
    return err;
}

static const struct linlondp_dev_funcs dp_chip_funcs = {
    .init_format_table    = dp_init_fmt_tbl,
    .enum_resources        = dp_enum_resources,
    .cleanup        = dp_cleanup,
    .irq_handler        = dp_irq_handler,
    .enable_irq        = dp_enable_irq,
    .disable_irq        = dp_disable_irq,
    .on_off_vblank        = dp_on_off_vblank,
    .change_opmode        = dp_change_opmode,
    .flush            = dp_flush,
    .connect_iommu        = dp_connect_iommu,
    .disconnect_iommu    = dp_disconnect_iommu,
    .init_hw        = dp_init_hw,
    .dump_register        = dp_dump,
    .close_gop        = dp_close_gop,
};

const struct linlondp_dev_funcs *
dp_identify(u32 __iomem *reg_base, struct linlondp_chip_info *chip)
{
    const struct linlondp_dev_funcs *funcs;
    u32 product_id;

    chip->core_id = linlondp_read32(reg_base, GLB_CORE_ID);

    product_id = LINLONDP_CORE_ID_PRODUCT_ID(chip->core_id);

    switch (product_id) {
    case LINLONDP_D8_PRODUCT_ID:
    case LINLONDP_D6_PRODUCT_ID:
    case LINLONDP_D2_PRODUCT_ID:
        funcs = &dp_chip_funcs;
        break;
    default:
        DRM_ERROR("Unsupported product: 0x%x\n", product_id);
        return NULL;
    }

    chip->arch_id    = linlondp_read32(reg_base, GLB_ARCH_ID);
    chip->core_info    = linlondp_read32(reg_base, GLB_CORE_INFO);
    chip->bus_width    = DP_BUS_WIDTH_16_BYTES;

    return funcs;
}
