// SPDX-License-Identifier: GPL-2.0
/*
 * (C) COPYRIGHT 2022-2023 Arm Technology (China) Co., Ltd.
 * ALL RIGHTS RESERVED
 *
 */
#ifndef _LINLON_AEU_REG_DUMP_H_
#define _LINLON_AEU_REG_DUMP_H_

/* current registers */
#define REG_FILE_CURR    0
/* just before compressing is enabled */
#define REG_FILE_BEF    1
/* the coppressing is done, or error is detected */
#define REG_FILE_AFT    2

struct linlon_aeu_reg_file;

#ifdef CONFIG_VIDEO_ADV_DEBUG
struct linlon_aeu_reg_file *linlon_aeu_init_reg_file(void __iomem *b);
void linlon_aeu_free_reg_file(struct linlon_aeu_reg_file *reg_f);
int linlon_aeu_reg_dump(struct linlon_aeu_reg_file *reg_f, u32 table);
int linlon_aeu_g_reg(struct linlon_aeu_reg_file *reg_f, u32 table, u32 idx, u32 *v);
#else
static inline struct linlon_aeu_reg_file *linlon_aeu_init_reg_file(void __iomem *b)
{
    return NULL;
}

static inline void linlon_aeu_free_reg_file(struct linlon_aeu_reg_file *reg_f)
{
}

static inline int linlon_aeu_reg_dump(struct linlon_aeu_reg_file *reg_f, u32 table)
{
    return -EINVAL;
}

static inline
int linlon_aeu_g_reg(struct linlon_aeu_reg_file *reg_f, u32 table, u32 idx, u32 *v)
{
    return -EINVAL;
}
#endif /* CONFIG_VIDEO_ADV_DEBUG */

#endif /* !_LINLON_AEU_REG_DUMP_H_ */
