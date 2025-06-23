/* SPDX-License-Identifier: GPL-2.0 */
/*
 * (C) COPYRIGHT 2022-2023 Arm Technology (China) Co., Ltd.
 * ALL RIGHTS RESERVED
 *
 */
#ifndef _LINLONDP_PRODUCT_H_
#define _LINLONDP_PRODUCT_H_

/* Product identification */
#define LINLONDP_CORE_ID(__product, __major, __minor, __status) \
    ((((__product) & 0xFFFF) << 16) | (((__major) & 0xF) << 12) | \
    (((__minor) & 0xF) << 8) | ((__status) & 0xFF))

#define LINLONDP_CORE_ID_PRODUCT_ID(__core_id) ((__u32)(__core_id) >> 16)
#define LINLONDP_CORE_ID_MAJOR(__core_id)      (((__u32)(__core_id) >> 12) & 0xF)
#define LINLONDP_CORE_ID_MINOR(__core_id)      (((__u32)(__core_id) >> 8) & 0xF)
#define LINLONDP_CORE_ID_STATUS(__core_id)     (((__u32)(__core_id)) & 0xFF)

/* Linlon-display product IDs */
#define LINLONDP_D8_PRODUCT_ID    0x0080
#define LINLONDP_D6_PRODUCT_ID    0x0060
#define LINLONDP_D2_PRODUCT_ID    0x0020
union linlondp_config_id {
    struct {
        __u32    max_line_sz:16,
            n_pipelines:2,
            n_scalers:2, /* number of scalers per pipeline */
            n_layers:3, /* number of layers per pipeline */
            n_richs:3, /* number of rich layers per pipeline */
            side_by_side:1, /* if HW works on side_by_side mode */
            reserved_bits:5;
    };
    __u32 value;
};

#endif /* _LINLONDP_PRODUCT_H_ */
