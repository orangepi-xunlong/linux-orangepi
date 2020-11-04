/*
 * Allwinner SoCs display driver.
 *
 * Copyright (C) 2016 Allwinner.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */


#ifndef __EINK_BUFFER_MANAGER_H__
#define __EINK_BUFFER_MANAGER_H__

#include "disp_eink.h"

s32 ring_buffer_manager_init(void);
s32 ring_buffer_manager_exit(void);
bool is_ring_queue_full(void);
bool is_ring_queue_empty(void);
u8 *get_current_image(void);
u8 *get_last_image(void);
s32 queue_image(u32 mode, struct area_info update_area);
s32 dequeue_image(void);

extern void *disp_malloc(u32 num_bytes, void *phy_addr);
extern void disp_free(void *virt_addr, void *phy_addr, u32 num_bytes);
extern struct format_manager *disp_get_format_manager(unsigned int id);

#endif
