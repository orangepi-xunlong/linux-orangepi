/*
*********************************************************************************************************
*                                                    LINUX-KERNEL
*                                        AllWinner Linux Platform Develop Kits
*                                                   Kernel Module
*
*                                    (c) Copyright 2006-2011, kevin.z China
*                                             All Rights Reserved
*
* File    : standby_power.h
* By      : kevin.z
* Version : v1.0
* Date    : 2011-5-31 14:34
* Descript:
* Update  : date                auther      ver     notes
*********************************************************************************************************
*/
#ifndef __STANDBY_POWER_H__
#define __STANDBY_POWER_H__

#include "standby_cfg.h"

extern int standby_set_power(unsigned int pmu_id, unsigned int bitmap,
	unsigned int (* power_tree)[VCC_MAX_INDEX], unsigned short volt);

extern int standby_recovery_power(unsigned int pmu_id);
#endif  /* __STANDBY_POWER_H__ */


