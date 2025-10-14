// SPDX-License-Identifier: GPL-2.0
/*
 * (C) COPYRIGHT 2022-2023 Arm Technology (China) Co., Ltd.
 * ALL RIGHTS RESERVED
 *
 */
#include <linux/slab.h>
#include "linlon_aeu_hw.h"
#include "linlon_aeu_io.h"
#include "linlon_aeu_reg_dump.h"

/* NOTICE: no spinlock is provided for protecting reg file,
 * The caller should care about that.
 * Recommend to use same locker which is used for protecting
 * register access.
 */
struct linlon_aeu_reg_file {
    void __iomem *base;
    uint reg_count;
    u32 *reg_bef;
    u32 *reg_aft;
};

static u32 reg_addr_array[] = {
    AEU_BLOCK_INFO, AEU_PIPELINE_INFO, AEU_IRQ_STATUS, AEU_STATUS,
    AEU_CONTROL, AEU_AES_BLOCK_INFO, AEU_AES_PIPELINE_INFO,
    AEU_AES_BLOCK_ID, AEU_AES_IRQ_RAW_STATUS, AEU_AES_IRQ_MASK,
    AEU_AES_IRQ_STATUS, AEU_AES_COMMAND, AEU_AES_STATUS, AEU_AES_CONTROL,
    AEU_AES_AXI_CONTROL, AEU_AES_OUT_P0_PTR_LOW, AEU_AES_OUT_P0_PTR_HIGH,
    AEU_AES_OUT_P1_PTR_LOW, AEU_AES_OUT_P1_PTR_HIGH, AEU_AES_FORMAT,
    AEU_AES_IN_HSIZE, AEU_AES_IN_VSIZE, AEU_AES_BBOX_X_START,
    AEU_AES_BBOX_X_END, AEU_AES_BBOX_Y_START, AEU_AES_BBOX_Y_END,
    AEU_AES_IN_P0_PTR_LOW, AEU_AES_IN_P0_PTR_HIGH, AEU_AES_IN_P0_STRIDE,
    AEU_AES_IN_WRITE_ORDER, AEU_AES_IN_P1_PTR_LOW, AEU_AES_IN_P1_PTR_HIGH,
    AEU_AES_IN_P1_STRIDE, AEU_DS_BLOCK_INFO, AEU_DS_PIPELINE_INFO,
    AEU_DS_IRQ_RAW_STATUS, AEU_DS_IRQ_MASK, AEU_DS_IRQ_STATUS,
    AEU_DS_CONTROL, AEU_DS_CONFIG_VALID, AEU_DS_PROG_LINE,
    AEU_DS_AXI_CONTROL, AEU_DS_FORMAT, AEU_DS_IN_SIZE,
    AEU_DS_IN_P0_PTR_LOW, AEU_DS_IN_P0_PTR_HIGH, AEU_DS_IN_P0_STRIDE,
    AEU_DS_IN_P1_PTR_LOW, AEU_DS_IN_P1_PTR_HIGH, AEU_DS_IN_P1_STRIDE,
    AEU_DS_IN_P2_PTR_LOW, AEU_DS_IN_P2_PTR_HIGH, AEU_DS_OUT_P0_PTR_LOW,
    AEU_DS_OUT_P0_PTR_HIGH, AEU_DS_OUT_P0_STRIDE, AEU_DS_OUT_P1_PTR_LOW,
    AEU_DS_OUT_P1_PTR_HIGH, AEU_DS_OUT_P1_STRIDE
};

#define REG_COUNT    (sizeof(reg_addr_array)/sizeof(u32))

struct linlon_aeu_reg_file *linlon_aeu_init_reg_file(void __iomem *b)
{
    struct linlon_aeu_reg_file *reg_f;

    reg_f = kzalloc(sizeof(*reg_f), GFP_KERNEL);
    if (!reg_f)
        return NULL;

    reg_f->base = b;
    reg_f->reg_count = REG_COUNT;

    reg_f->reg_bef = kzalloc(sizeof(u32) * REG_COUNT, GFP_KERNEL);
    if (!reg_f->reg_bef)
        goto err_reg_bef;

    reg_f->reg_aft = kzalloc(sizeof(u32) * REG_COUNT, GFP_KERNEL);
    if (!reg_f->reg_aft) {
        kfree(reg_f->reg_bef);
        goto err_reg_bef;
    }

    return reg_f;

err_reg_bef:
    kfree(reg_f);
    return NULL;
}

void linlon_aeu_free_reg_file(struct linlon_aeu_reg_file *reg_f)
{
    if (reg_f) {
        kfree(reg_f->reg_bef);
        kfree(reg_f->reg_aft);
        kfree(reg_f);
    }
}

int linlon_aeu_reg_dump(struct linlon_aeu_reg_file *reg_f, u32 table)
{
    u32 *reg_t;
    u32 i;

    if (!reg_f)
        return -EINVAL;

    switch (table) {
    case REG_FILE_CURR:
        return 0;
    case REG_FILE_BEF:
        reg_t = reg_f->reg_bef;
        break;
    case REG_FILE_AFT:
        reg_t = reg_f->reg_aft;
        break;
    default:
        return -ENOENT;
    }

    for (i = 0; i < reg_f->reg_count; i++)
        reg_t[i] = linlon_aeu_read(reg_f->base, reg_addr_array[i]);

    return 0;
}

int linlon_aeu_g_reg(struct linlon_aeu_reg_file *reg_f, u32 table, u32 idx, u32 *v)
{

    if (!reg_f || idx >= reg_f->reg_count)
        return -EINVAL;

    if (!v)
        return -ENOMEM;

    switch (table) {
    case REG_FILE_CURR:
        idx = reg_addr_array[idx];
        *v = linlon_aeu_read(reg_f->base, idx);
        break;
    case REG_FILE_BEF:
        *v = reg_f->reg_bef[idx];
        break;
    case REG_FILE_AFT:
        *v = reg_f->reg_aft[idx];
        break;
    default:
        return -ENOENT;
    }

    return 0;
}
