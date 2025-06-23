#ifndef _PRINTK_EXT_H_
#define _PRINTK_EXT_H_

int get_printk_level(void);
int get_sysctl_printk_level(int level);
void printk_level_setup(int level);
void sysctl_printk_level_setup(void);
void plat_log_store_add_time(char *logbuf, u32 logsize, u16 *retlen);
#endif
