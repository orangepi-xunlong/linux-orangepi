
#ifndef __EINK_BUFFER_MANAGER_H__
#define __EINK_BUFFER_MANAGER_H__

#include "disp_eink.h"

s32 ring_buffer_manager_init();
s32 ring_buffer_manager_exit();
bool is_ring_queue_full();
bool is_ring_queue_empty();
u8* get_current_image();
u8* get_last_image();
s32 queue_image(u32 mode,  struct area_info update_area);
s32 dequeue_image();

#endif
