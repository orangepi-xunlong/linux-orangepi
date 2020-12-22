
#ifndef __MULTI_IR_H__
#define __MULTI_IR_H__

#include <linux/ioctl.h>

#define MULTI_IR_DEV_NAME	"sunxi-multi-ir"

/* ioctl cmd define */
#define MULTI_IR_IOC_MAGIC	'M'
/* return the max mapping table count support, if error, return -1 */
#define MULTI_IR_IOC_REQ_MAP	_IOR(MULTI_IR_IOC_MAGIC, 1, int *)
#define MULTI_IR_IOC_SET_MAP	_IOW(MULTI_IR_IOC_MAGIC, 2, void *)
#define MULTI_IR_IOC_CLR_MAP	_IOW(MULTI_IR_IOC_MAGIC, 3, int)

#define MAX_NAME_LEN	(32)
#define KEYCODE_MIN		(0)
#define KEYCODE_MAX		(255)
#define KEYCODE_CNT		(KEYCODE_MAX-KEYCODE_MIN+1)

/*
 A mapping table is mapping from customer ir key layout file(*.kl)
 to default layout file.

 For example:
	in coustomer kl file  -->  key 28  POWER
	in default kl file    -->  key 57  POWER

	then, the mapping will be like that: mapping_table.value[57] = 28
*/
struct mapping_table_t {
	int identity;				/* means ir address */
	int powerkey;				/* powerkey to wakeup system */
	int value[KEYCODE_CNT];		/* convert from coustomer keycode to
                               		default keycode */
};

#endif /* __MULTI_IR_H__ */
