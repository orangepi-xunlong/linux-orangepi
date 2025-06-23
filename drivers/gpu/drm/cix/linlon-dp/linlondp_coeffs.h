// SPDX-License-Identifier: GPL-2.0
/*
 * (C) COPYRIGHT 2022-2023 Arm Technology (China) Co., Ltd.
 * ALL RIGHTS RESERVED
 *
 */

#ifndef _LINLONDP_COEFFS_H_
#define _LINLONDP_COEFFS_H_

#include <linux/refcount.h>

/* Linlon display HWs have kinds of coefficient tables for various purposes,
 * like gamma/degamma. ususally these tables are shared by multiple HW component
 * and limited number.
 * The linlondp_coeffs_table/manager are imported for describing and managing
 * these tables for table reuse and racing.
 */
struct linlondp_coeffs_table {
    struct linlondp_coeffs_manager *mgr;
    refcount_t refcount;
    bool needs_update;
    u32 hw_id;
    void *coeffs;
    u32 __iomem *reg;
    void (*update)(struct linlondp_coeffs_table *table);
};

struct linlondp_coeffs_manager {
    struct mutex mutex; /* for tables accessing */
    u32 n_tables;
    u32 coeffs_sz;
    struct linlondp_coeffs_table *tables[8];
};

static inline struct linlondp_coeffs_table *
linlondp_coeffs_get(struct linlondp_coeffs_table *table)
{
    if (table)
        refcount_inc(&table->refcount);

    return table;
}

static inline void __linlondp_coeffs_put(struct linlondp_coeffs_table *table)
{
    if (table)
        refcount_dec(&table->refcount);
}

#define linlondp_coeffs_put(table) \
do { \
    __linlondp_coeffs_put(table); \
    (table) = NULL; \
} while (0)

static inline void linlondp_coeffs_update(struct linlondp_coeffs_table *table)
{
    if (!table || !table->needs_update)
        return;

    table->update(table);
    table->needs_update = false;
}

struct linlondp_coeffs_manager *linlondp_coeffs_create_manager(u32 coeffs_sz);
void linlondp_coeffs_destroy_manager(struct linlondp_coeffs_manager *mgr);

int linlondp_coeffs_add(struct linlondp_coeffs_manager *mgr,
        u32 hw_id, u32 __iomem *reg,
        void (*update)(struct linlondp_coeffs_table *table));
struct linlondp_coeffs_table *
linlondp_coeffs_request(struct linlondp_coeffs_manager *mgr, void *coeffs);

#endif /*_LINLONDP_COEFFS_H_*/
