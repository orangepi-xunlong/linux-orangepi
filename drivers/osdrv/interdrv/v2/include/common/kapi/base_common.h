#ifndef __BASE_COMMON_H__
#define __BASE_COMMON_H__

#ifdef __cplusplus
	extern "C" {
#endif

#include <linux/common.h>

const uint8_t *sys_get_modname(mod_id_e id);

u32 get_diff_in_us(struct timespec64 t1, struct timespec64 t2);

u32 get_pm_buf_mode(void);

#ifdef __cplusplus
}
#endif

#endif /* __BASE_CB_H__ */
