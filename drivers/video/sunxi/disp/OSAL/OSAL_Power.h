#ifndef  __OSAL_POWER_H__
#define  __OSAL_POWER_H__

#include "OSAL.h"
#include <linux/regulator/consumer.h>

#ifndef CONFIG_AW_AXP
#define  __OSAL_POWER_MASK__
#endif

int OSAL_Power_Enable(char *name);

int OSAL_Power_Disable(char *name);

#endif
