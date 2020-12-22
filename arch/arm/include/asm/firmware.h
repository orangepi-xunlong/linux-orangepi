/*
 * Copyright (C) 2012 Samsung Electronics.
 * Kyungmin Park <kyungmin.park@samsung.com>
 * Tomasz Figa <t.figa@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __ASM_ARM_FIRMWARE_H
#define __ASM_ARM_FIRMWARE_H

#include <linux/bug.h>


/*
 * struct firmware_ops
 *
 * A structure to specify available firmware operations.
 *
 * A filled up structure can be registered with register_firmware_ops().
 */
struct firmware_ops {
	unsigned int (*read_reg)(void __iomem *reg);
	unsigned int (*write_reg)(u32 value, void __iomem *reg);
	unsigned int (*send_command)(u32 arg0, u32 arg1, u32 arg2, u32 arg3);
	unsigned int (*load_arisc)(void *image, u32 image_size, void *para, u32 para_size, u32 para_offset);
	unsigned int (*set_secondary_entry)(void *entry);
	unsigned int (*suspend)(void);
	unsigned int (*suspend_prepare)(void);
	unsigned int (*set_standby_status)(u32 arg0, u32 arg1, u32 arg2, u32 arg3);
	unsigned int (*get_cp15_status)(void *addr);
	unsigned int (*resume_hdcp_key)(void);
};

/* Global pointer for current firmware_ops structure, can't be NULL. */
extern const struct firmware_ops *firmware_ops;

/*
 * call_firmware_op(op, ...)
 *
 * Checks if firmware operation is present and calls it,
 * otherwise returns -ENOSYS
 */
#define call_firmware_op(op, ...)					\
	((firmware_ops->op) ? firmware_ops->op(__VA_ARGS__) : (-ENOSYS))

/*
 * register_firmware_ops(ops)
 *
 * A function to register platform firmware_ops struct.
 */
static inline void register_firmware_ops(const struct firmware_ops *ops)
{
	BUG_ON(!ops);

	firmware_ops = ops;
}

struct sunxi_sst_hdcp{
	u8			key[SZ_4K];
	size_t		act_len ;
};
#endif
