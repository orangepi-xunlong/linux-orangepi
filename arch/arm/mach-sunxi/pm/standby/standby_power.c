/*
*********************************************************************************************************
*                                                    LINUX-KERNEL
*                                        AllWinner Linux Platform Develop Kits
*                                                   Kernel Module
*
*                                    (c) Copyright 2006-2011, kevin.z China
*                                             All Rights Reserved
*
* File    : standby_power.c
* By      : Ming Li
* Version : v1.0
* Date    : 2014-11-03 14:34
* Descript:
* Update  : date                auther      ver     notes
*********************************************************************************************************
*/
#include "standby_i.h"
#include "standby_power.h"

/*
*********************************************************************************************************
*                           standby_set_power
*
*Description: set voltage for standby;
*
*Arguments  : pmu_id     :     pmu's id
*             bitmap     :     power bitmap
*             power_tree :     power tree point;
*             volt       :     voltage value, based on "mv";
*
*Return     : success: 0;
*             failed : -1;
*Notes      :
*
*********************************************************************************************************
*/
int standby_set_power(unsigned int pmu_id, unsigned int bitmap,
	unsigned int (* power_tree)[VCC_MAX_INDEX], unsigned short volt)
{
	unsigned int pmux_id = 0, i = 0, ret = -1;
	unsigned int tree_value = 0;

	tree_value = (*(power_tree))[bitmap];
	if (0 == tree_value) {
		printk("power tree %d value is 0\n", bitmap);
		return -1;
	}

	save_mem_status(RESUME0_START | 0X09);

	for (i = 0; i<4; i++) {
		pmux_id  = (pmu_id >> (8 * i)) & 0xFF;
		if (0 != pmux_id) {
			switch(pmux_id)
			{
			case AXP_19X_ID:
				break;
			case AXP_209_ID:
				ret = axp209_set_volt(tree_value, volt);
				break;
			case AXP_22X_ID:
				break;
			case AXP_806_ID:
				break;
			case AXP_808_ID:
				break;
			case AXP_809_ID:
				break;
			case AXP_803_ID:
				break;
			case AXP_813_ID:
				break;
			default:
				return -1;
			}
		}
	}
	return 0;
}

/*
*********************************************************************************************************
*                           standby_recovery_power
*
*Description: set voltage for standby;
*
*Arguments  : pmu_id     :     pmu's id
*             bitmap     :     power bitmap
*             power_tree :     power tree point;
*             volt       :     voltage value, based on "mv";
*
*Return     : success: 0;
*             failed : -1;
*Notes      :
*
*********************************************************************************************************
*/
int standby_recovery_power(unsigned int pmu_id)
{
	unsigned int pmux_id = 0, i = 0, ret = -1;
	unsigned int tree_value = 0;

	for (i = 0; i<4; i++) {
		pmux_id  = (pmu_id >> (8 * i)) & 0xFF;
		if (0 != pmux_id) {
			switch(pmux_id)
			{
			case AXP_19X_ID:
				break;
			case AXP_209_ID:
				ret = axp209_recovery_volt();
				break;
			case AXP_22X_ID:
				break;
			case AXP_806_ID:
				break;
			case AXP_808_ID:
				break;
			case AXP_809_ID:
				break;
			case AXP_803_ID:
				break;
			case AXP_813_ID:
				break;
			default:
				return -1;
			}
		}
	}
	return 0;
}


