// SPDX-License-Identifier: GPL-2.0
/*
 * (C) COPYRIGHT 2022-2023 Arm Technology (China) Co., Ltd.
 * ALL RIGHTS RESERVED
 *
 */
#include <linux/slab.h>
#include "linlon_aeu_hw.h"
#include "linlon_aeu_log.h"

#define AEU_LOG_LINES        128
#define AEU_CHAR_PRE_LINE    128

struct linlon_aeu_log {
    u32 n_lines;
    atomic_t ord;
    char *cstr;
};

static struct linlon_aeu_log aeu_log;

static char *get_aeu_register_name(u32 reg)
{
    char *name[5] = {
        [0] = "AEU_BLOCK_INFO",
        [1] = "AEU_PIPELINE_INFO",
        [2] = "**",
        [3] = "**",
        [4] = "AEU_IRQ_STATUS"
    };

    reg -= AEU_OFFSET;
    if (reg <= 0x10)
        return name[reg >> 2];

    if (reg == 0xB0)
        return "AEU_STATUS";

    return "AEU_CONTROL";
}

static char *get_aes_register_name(u32 reg)
{
    char *aes_name_1[] = {
        "AES_BLOCK_ID",        "AES_IRQ_RAW_STATUS",
        "AES_IRQ_CLEAR",    "AES_IRQ_MASK",
        "AES_IRQ_STATUS",    "AES_COMMAND",
        "AES_STATUS",        "AES_CONTROL",
        "AES_AXI_CONTROL"
    };

    char *aes_name_2[] = {
        "AES_OUT_P0_PTR_LOW",    "AES_OUT_P0_PTR_HIGH",
        "AES_OUT_P1_PTR_LOW",   "AES_OUT_P1_PTR_HIGH",
        "AES_FORMAT",        "AES_IN_HSIZE",
        "AES_IN_VSIZE",        "AES_BOUNDING_BOX_X_START",
        "AES_BOUNDING_BOX_X_END",
        "AES_BOUNDING_BOX_Y_START",
        "AES_BOUNDING_BOX_X_END",
        "AES_IN_P0_PTR_LOW",    "AES_IN_P0_PTR_HIGH",
        "AES_IN_P0_STRIDE",    "AES_IN_WRITE_ORDER",
    };

    char *aes_name_3[] = {
        "AES_IN_P1_PTR_LOW",    "AES_IN_P1_PTR_HIGH",
        "AES_IN_P1_STRIDE"
    };

    char* (*p)[];

    reg -= AEU_AES_OFFSET;
    if (reg < 0x100)
        return (reg) ? "AES_PIPELINE_INFO" : "AES_BLOCK_INFO";

    reg -= 0x100;
    if (reg <= 0x20)
        p = &aes_name_1;
    else if (reg <= 0x78) {
        p = &aes_name_2;
        reg -= 0x40;
    } else {
        p = &aes_name_3;
        reg -= 0xEC;
    }

    return (*p)[reg >> 2];
}

static char *get_ds_register_name(u32 reg)
{
    char *ds_name_1[] = {
        "DS_IRQ_RAW_STATUS",    "DS_IRQ_CLEAR",
        "DS_IRQ_MASK",        "DS_IRQ_STATUS",
        "DS_STATUS"
    };

    char *ds_name_2[] = {
        "DS_CONTROL",        "DS_CONFIG_VALID",
        "DS_PROG_LINE",        "DS_AXI_CONTROL",
        "DS_FORMAT",        "DS_IN_SIZE"
    };
    u32 i;
    static char name[32];
    char *postfix[3] = {
        "PTR_LOW", "PTR_HIGH", "STRIDE"
    };

    reg -= AEU_DS_OFFSET;
    if (reg < 0x8)
        return (reg) ? "DS_PIPELINE_INFO" : "DS_BLOCK_INFO";

    if (reg <= 0xB0)
        return ds_name_1[(reg - 0xA0) >> 2];
    else if (reg <= 0xE4)
        return ds_name_2[(reg - 0xD0) >> 2];

    reg -= 0x100;
    i = reg / 0x10;
    reg = (reg %0x10) >> 2;
    switch (i) {
    case 0:
    case 1:
    case 2:
        snprintf(name, 32, "DS_IN_P%d_%s", i, postfix[reg]);
        break;
    case 3:
    case 4:
        snprintf(name, 32, "DS_OUT_P%d_%s", i - 3, postfix[reg]);
        break;
    }

    return name;
}

static char *get_register_name(u32 reg)
{
    char *(*reg_name[3])(u32) = {
        get_aeu_register_name,
        get_ds_register_name,
        get_aes_register_name
    };

    return reg_name[(reg - AEU_OFFSET) / 0x200](reg);
}

static char *get_line(u32 l)
{
    l %= aeu_log.n_lines;
    return aeu_log.cstr + (l * AEU_CHAR_PRE_LINE);
}

int linlon_aeu_log(bool read, u32 reg, u32 val)
{
    char prefix = (read) ? 'R' : 'W';
    char *line;
    u32 ord;

    if (aeu_log.cstr == NULL)
        return -EINVAL;

    ord = atomic_inc_return(&aeu_log.ord);
    line = get_line(ord - 1);
    snprintf(line, AEU_CHAR_PRE_LINE, "%c ord=[%u] %s=%#x\n",
            prefix, ord, get_register_name(reg), val);
    return 0;
}

static int aeu_logger_show(struct seq_file *sf, void *x)
{
    int i, start;

    if (aeu_log.cstr == NULL)
        return -EINVAL;

    start = atomic_read(&aeu_log.ord);
    for (i = 0 ; i < aeu_log.n_lines; i++) {
        char *line = get_line(start + i);

        if (line[0] == '\0')
            continue;
        seq_printf(sf, line);
    }

    return 0;
}

static int aeu_logger_open(struct inode *inode, struct file *fp)
{
    return single_open(fp, aeu_logger_show, inode->i_private);
}

static const struct file_operations aeu_log_fops = {
    .owner = THIS_MODULE,
    .open = aeu_logger_open,
    .read = seq_read,
    .llseek = seq_lseek,
    .release = single_release,
};

void linlon_aeu_log_init(struct dentry *root)
{
    atomic_set(&aeu_log.ord, 0);

    aeu_log.n_lines = AEU_LOG_LINES;
    aeu_log.cstr = kzalloc(AEU_LOG_LINES * AEU_CHAR_PRE_LINE, GFP_KERNEL);
    if (aeu_log.cstr == NULL) {
        pr_err("failed to initialize aeu logger!\n");
        return;
    }

    debugfs_create_file("aeu_reg_log", S_IROTH | S_IRUSR | S_IRGRP,
                root, NULL, &aeu_log_fops);
}

void linlon_aeu_log_exit(void)
{
    if (aeu_log.cstr) {
        kfree(aeu_log.cstr);
        aeu_log.cstr = NULL;
    }
}
