#ifndef __HW_BREAKPOINT_UNTIL_H
#define __HW_BREAKPOINT_UNTIL_H

#include <linux/vmalloc.h>

typedef struct iophys_info {
	struct list_head list;
	struct vm_struct area;
	u64 virt_addr;
} iophys_info;

void process_cmd_string(char *pBuf, int *pArgc, char *pArgv[]);
/*iophy to virt func*/
iophys_info *get_iophys_info(u64 addr);
void free_iophys_info(iophys_info *info);

#endif
