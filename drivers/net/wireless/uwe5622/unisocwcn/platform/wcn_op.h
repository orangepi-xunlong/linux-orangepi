#ifndef __WCN_OP_H__
#define __WCN_OP_H__

#if defined(CONFIG_STACKPROTECTOR) && defined(CONFIG_STACKPROTECTOR_PER_TASK)
#include <linux/stackprotector.h>
unsigned long __stack_chk_guard __read_mostly;
EXPORT_SYMBOL(__stack_chk_guard);
#endif

int wcn_op_init(void);
void wcn_op_exit(void);

#endif
