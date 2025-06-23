// SPDX-License-Identifier: GPL-2.0
/*
 * (C) COPYRIGHT 2022-2023 Arm Technology (China) Co., Ltd.
 * ALL RIGHTS RESERVED
 *
 */

#include <linux/of.h>
#include <drm/drm_print.h>
#include "dp_dev.h"
#include "linlondp_io.h"

enum dp_property_id {
    LW_AWCACHE,
    L0_ARCACHE,
    L1_ARCACHE,
    L2_ARCACHE,
    L3_ARCACHE,
    RAXI_AOUTSTDCAPB,
    RAXI_BOUTSTDCAPB,
    RAXI_BEN,
    RAXI_BURSTLEN,
    RAXI_AxQOS,
    RAXI_ORD,
    WAXI_OUTSTDCAPB,
    WAXI_BURSTLEN,
    WAXI_AxQOS,
    WAXI_ORD,
    TBU_DOUTSTDCAPB,
};

enum dp_property_type {
    RANGE_PROP,
    LIST_PROP
};

struct dp_property {
    char *p_name;
    enum dp_property_id p_id;
    enum dp_property_type p_type;
    u32 default_val;
    union {
        struct {
            u32 val_min, val_max;
        };
        struct {
            u32 n_elements;
            const u32 *elements;
        };
    };
};

#define DEFINE_LPU_LAYER_AxCACHE_ATTR(n, id)                \
const struct dp_property n = {                        \
    .p_name = #n,                            \
    .p_id = id,                            \
    .default_val = 0x3,                        \
    .p_type = LIST_PROP,                        \
    .n_elements = 10,                        \
    .elements = (const u32[]) {                    \
        0x0, 0x1, 0x2, 0x3, 0x3, 0x7, 0xA, 0xB, 0xE, 0xF    \
    },                                \
}

static DEFINE_LPU_LAYER_AxCACHE_ATTR(lpu_lw_awcache, LW_AWCACHE);
static DEFINE_LPU_LAYER_AxCACHE_ATTR(lpu_l0_arcache, L0_ARCACHE);
static DEFINE_LPU_LAYER_AxCACHE_ATTR(lpu_l1_arcache, L1_ARCACHE);
static DEFINE_LPU_LAYER_AxCACHE_ATTR(lpu_l2_arcache, L2_ARCACHE);
static DEFINE_LPU_LAYER_AxCACHE_ATTR(lpu_l3_arcache, L3_ARCACHE);

#define DEFINE_LPU_xAXI_CTRL_ATTR_U32_RANGE(n, id, def, min, max)    \
const struct dp_property n = {                        \
    .p_name = #n,                            \
    .p_id = id,                            \
    .default_val = def,                        \
    .p_type = RANGE_PROP,                        \
    .val_min = min,                            \
    .val_max = max,                            \
}

#define DEFINE_LPU_xAXI_CTRL_ATTR_U32_LIST(n, id, def)            \
const struct dp_property n = {                        \
    .p_name = #n,                            \
    .p_id = id,                            \
    .default_val = def,                        \
    .p_type = LIST_PROP,                        \
    .n_elements = 4,                        \
    .elements = (const u32[]) {0x4, 0x8, 0x10, 0x20},        \
}

static DEFINE_LPU_xAXI_CTRL_ATTR_U32_RANGE(lpu_raxi_aoutstdcapb,
                       RAXI_AOUTSTDCAPB, 0x20, 0x4, 0x80);
static DEFINE_LPU_xAXI_CTRL_ATTR_U32_RANGE(lpu_raxi_boutstdcapb,
                       RAXI_BOUTSTDCAPB, 0x20, 0x4, 0x80);
static DEFINE_LPU_xAXI_CTRL_ATTR_U32_RANGE(lpu_raxi_ben, RAXI_BEN, 0, 0, 1);
static DEFINE_LPU_xAXI_CTRL_ATTR_U32_LIST(lpu_raxi_burstlen,
                      RAXI_BURSTLEN, 0x10);
static DEFINE_LPU_xAXI_CTRL_ATTR_U32_RANGE(lpu_raxi_arqos, RAXI_AxQOS,
                       0xF, 0, 0xF);
static DEFINE_LPU_xAXI_CTRL_ATTR_U32_RANGE(lpu_raxi_ord, RAXI_ORD, 0, 0, 1);
static DEFINE_LPU_xAXI_CTRL_ATTR_U32_RANGE(lpu_waxi_outstdcapb, WAXI_OUTSTDCAPB,
                       0x10, 0x1, 0x20);
static DEFINE_LPU_xAXI_CTRL_ATTR_U32_LIST(lpu_waxi_burstlen,
                      WAXI_BURSTLEN, 0x10);
static DEFINE_LPU_xAXI_CTRL_ATTR_U32_RANGE(lpu_waxi_awqos, WAXI_AxQOS,
                       0xF, 0, 0xF);
static DEFINE_LPU_xAXI_CTRL_ATTR_U32_RANGE(lpu_waxi_ord, WAXI_ORD, 0, 0, 1);
static DEFINE_LPU_xAXI_CTRL_ATTR_U32_RANGE(lpu_tbu_doutstdcapb, TBU_DOUTSTDCAPB,
                       8, 1, 16);

static const struct dp_property *dp_lpu_properties[] = {
    &lpu_lw_awcache,
    &lpu_l0_arcache,
    &lpu_l1_arcache,
    &lpu_l2_arcache,
    &lpu_l3_arcache,
    &lpu_raxi_aoutstdcapb,
    &lpu_raxi_boutstdcapb,
    &lpu_raxi_ben,
    &lpu_raxi_burstlen,
    &lpu_raxi_arqos,
    &lpu_raxi_ord,
    &lpu_waxi_outstdcapb,
    &lpu_waxi_burstlen,
    &lpu_waxi_awqos,
    &lpu_waxi_ord,
    &lpu_tbu_doutstdcapb,
    NULL,
};

static void commit_axi_property(const struct dp_property *p,
                u32 val, u32 __iomem *reg)
{
    u32 mask = 0, offset = 0;

    switch (p->p_id) {
    case RAXI_AOUTSTDCAPB:
        val = TO_RAXI_AOUTSTDCAPB(val);
        mask = RAXI_AOUTSTDCAPB_MASK;
        offset = LPU_RAXI_CONTROL;
        break;
    case RAXI_BOUTSTDCAPB:
        val = TO_RAXI_BOUTSTDCAPB(val);
        mask = RAXI_BOUTSTDCAPB_MASK;
        offset = LPU_RAXI_CONTROL;
        break;
    case RAXI_BEN:
        val = TO_RAXI_BEN(val);
        mask = RAXI_BEN_MASK;
        offset = LPU_RAXI_CONTROL;
        break;
    case RAXI_BURSTLEN:
        val = TO_xAXI_BURSTLEN(val);
        mask = xAXI_BURSTLEN_MASK;
        offset = LPU_RAXI_CONTROL;
        break;
    case RAXI_AxQOS:
        val = TO_xAXI_AxQOS(val);
        mask = xAXI_AxQOS_MASK;
        offset = LPU_RAXI_CONTROL;
        break;
    case RAXI_ORD:
        val = TO_xAXI_ORD(val);
        mask = xAXI_ORD_MASK;
        offset = LPU_RAXI_CONTROL;
        break;
    case WAXI_OUTSTDCAPB:
        val = TO_WAXI_OUTSTDCAPB(val);
        mask = WAXI_OUTSTDCAPB_MASK;
        offset = LPU_WAXI_CONTROL;
        break;
    case WAXI_BURSTLEN:
        val = TO_xAXI_BURSTLEN(val);
        mask = xAXI_BURSTLEN_MASK;
        offset = LPU_WAXI_CONTROL;
        break;
    case WAXI_AxQOS:
        val = TO_xAXI_AxQOS(val);
        mask = xAXI_AxQOS_MASK;
        offset = LPU_WAXI_CONTROL;
        break;
    case WAXI_ORD:
        val = TO_xAXI_ORD(val);
        mask = xAXI_ORD_MASK;
        offset = LPU_WAXI_CONTROL;
        break;
    case TBU_DOUTSTDCAPB:
        val = TO_TBU_DOUTSTDCAPB(val);
        mask = TBU_DOUTSTDCAPB_MASK;
        offset = LPU_TBU_CONTROL;
        break;
    default:
        return;
    }
    linlondp_write32_mask(reg, offset, mask, val);
}

static void commit_axi_cache(struct linlondp_pipeline *pipe,
                 const struct dp_property *p, u32 val)
{
    struct linlondp_layer *layer;

    if (p->p_id == LW_AWCACHE)
        layer = pipe->wb_layer;
    else
        layer = pipe->layers[p->p_id - L0_ARCACHE];

    if (!layer)
        return;

    val = Lx_AxCACHE(val);
    linlondp_write32_mask(layer->base.reg, BLK_CONTROL, AxCACHE_MASK, val);
}

int dp_pipeline_config_axi(struct dp_pipeline *dp_pipe)
{
    int i;

    for (i = 0; dp_lpu_properties[i]; i++) {
        const struct dp_property *p = dp_lpu_properties[i];
        u32 val;

        if (of_property_read_u32(dp_pipe->base.of_node,
                     p->p_name, &val))
            continue;
        if (val == p->default_val)
            continue;

        if (p->p_type == RANGE_PROP) {
            if (val < p->val_min || val > p->val_max) {
                DRM_ERROR("%s: property is out of range\n",
                      p->p_name);
                continue;
            }
        } else if (p->p_type == LIST_PROP) {
            int j;

            for (j = 0; j < p->n_elements; j++)
                if (val == p->elements[j])
                    break;

            if (j >= p->n_elements) {
                DRM_ERROR("%s: property is not in the list!\n",
                      p->p_name);
                continue;
            }
        } else {
            WARN_ON(1);
            continue;
        }

        if (p->p_id <= L3_ARCACHE)
            commit_axi_cache(&dp_pipe->base, p, val);
        else
            commit_axi_property(p, val, dp_pipe->lpu_addr);
    }
    return 0;
}
