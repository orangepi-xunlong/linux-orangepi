#ifndef _MCTL_STANDBY_H
#define _MCTL_STANDBY_H

#if defined(CONFIG_ARCH_SUN8IW11)
#include "sun8iw11p1/mctl_para-sun8iw11.h"
#elif defined(CONFIG_ARCH_SUN8IW10)
#include "sun8iw10p1/mctl_para-sun8iw10.h"
#else
#include "mctl_para-default.h"			/*the default dram para*/
#endif

extern unsigned int dram_power_save_process(void);
extern unsigned int dram_power_up_process(__dram_para_t *para);
extern void dram_enable_all_master(void);
extern void dram_disable_all_master(void);
#endif
