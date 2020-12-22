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
* By      : kevin.z
* Version : v1.0
* Date    : 2011-5-31 14:34
* Descript:
* Update  : date                auther      ver     notes
*********************************************************************************************************
*/
#include "standby_i.h"
#include "axp209_power.h"

static __u32 axp209_volt_data[POWER_VOL_MAX] = {0};

static inline int check_range(struct axp_info *info,__s32 voltage)
{
	if (voltage < info->min_uV || voltage > info->max_uV)
		return -1;

	return 0;
}

static int axp20_ldo4_data[] = {
    1250, 1300, 1400, 1500, 1600, 1700,
    1800, 1900, 2000, 2500, 2700, 2800,
    3000, 3100, 3200, 3300
};

static struct axp_info axp20_info[] = {
	AXP(POWER_VOL_LDO1,	 AXP20LDO1,	AXP20LDO1,	  0, AXP20_LDO1,  0, 0),//ldo1 for rtc
	AXP(POWER_VOL_LDO2,	      1800,      3300,  100, AXP20_LDO2,  4, 4),//ldo2 for analog1
	AXP(POWER_VOL_LDO3,	       700,      3500,   25, AXP20_LDO3,  0, 7),//ldo3 for digital
	AXP(POWER_VOL_LDO4,	      1250,      3300,  100, AXP20_LDO4,  0, 4),//ldo4 for analog2
	AXP(POWER_VOL_DCDC2,       700,      2275,   25, AXP20_BUCK2, 0, 6),//buck2 for core
	AXP(POWER_VOL_DCDC3,       700,      3500,   25, AXP20_BUCK3, 0, 7),//buck3 for memery
};

static inline struct axp_info *find_info(int id)
{
	struct axp_info *ri;
	int i;

	for (i = 0; i < sizeof(axp20_info)/sizeof(struct axp_info); i++) {
		ri = &axp20_info[i];
		if (ri->id == id)
			return ri;
	}
	return (struct axp_info *)(0);
}

/*
*********************************************************************************************************
*                           standby_set_voltage
*
*Description: set voltage for standby;
*
*Arguments  : type      voltage type, defined as "enum power_vol_type_e";
*             voltage   voltage value, based on "mv";
*
*Return     : none;
*
*Notes      :
*
*********************************************************************************************************
*/
static void  axp209_set_voltage(enum power_vol_type_e type, __s32 voltage)
{
	struct axp_info *info = (struct axp_info *)(0);
	__u8 val, mask, reg_val;

	info = find_info(type);
	if (info == (struct axp_info *)(0)) {
		return;
	}

	if (check_range(info, voltage)) {
		return;
	}

	if (type != POWER_VOL_LDO4)
		val = raw_lib_udiv((voltage-info->min_uV+info->step_uV-1), info->step_uV);
	else{
		if(voltage == 1250 ){
			val = 0;
		}
		else{
			val = raw_lib_udiv((voltage-1200+info->step_uV-1), info->step_uV);
			if(val > 16){
				val = val - 6;
			}
			else if(val > 13){
				val = val - 5;
			}
			else if(val > 12){
				val = val - 4;
			}
			else if(val > 8)
				val = 8;
		}
	}

	val <<= info->vol_shift;
	mask = ((1 << info->vol_nbits) - 1)  << info->vol_shift;

	twi_byte_rw(TWI_OP_RD,AXP_ADDR,info->vol_reg, &reg_val);

	if ((reg_val & mask) != val) {
		reg_val = (reg_val & ~mask) | val;
		twi_byte_rw(TWI_OP_WR,AXP_ADDR,info->vol_reg, &reg_val);
	}

	return;
}


/*
*********************************************************************************************************
*                           standby_get_voltage
*
*Description: get voltage for standby;
*
*Arguments  : type  voltage type, defined as "enum power_vol_type_e";
*
*Return     : voltage value, based on "mv";
*
*Notes      :
*
*********************************************************************************************************
*/
static __u32 axp209_get_voltage(enum power_vol_type_e type)
{
	struct axp_info *info = (struct axp_info *)(0);
	__u8 val, mask;

	info = find_info(type);
	if (info == (struct axp_info *)(0)) {
		return -1;
	}

	twi_byte_rw(TWI_OP_RD, AXP_ADDR, info->vol_reg, &val);

	mask = ((1 << info->vol_nbits) - 1)  << info->vol_shift;
	val = (val & mask) >> info->vol_shift;
	if (type != POWER_VOL_LDO4)
		return info->min_uV + info->step_uV * val;
	else
		return axp20_ldo4_data[val];
}

/*
*********************************************************************************************************
*                           axp209_set_volt
*
*Description: axp209 set voltage for standby;
*
*Arguments  : tree_value :     power tree value;
*             volt       :     voltage value, based on "mv";
*
*Return     : success: 0;
*             failed : -1;
*Notes      :
*
*********************************************************************************************************
*/
int axp209_set_volt(unsigned int tree_value, unsigned short volt)
{
	save_mem_status(RESUME0_START | 0X0a);
	switch(tree_value)
	{
	case AXP209_DCDC2:
		if (0 == axp209_volt_data[POWER_VOL_DCDC2])
			axp209_volt_data[POWER_VOL_DCDC2] = axp209_get_voltage(POWER_VOL_DCDC2);
		axp209_set_voltage(POWER_VOL_DCDC2, volt);
		break;
	case AXP209_DCDC3:
		if (0 == axp209_volt_data[POWER_VOL_DCDC3])
			axp209_volt_data[POWER_VOL_DCDC3] = axp209_get_voltage(POWER_VOL_DCDC3);
		axp209_set_voltage(POWER_VOL_DCDC3, volt);
		break;
	case AXP209_LDO1:
		if (0 == axp209_volt_data[POWER_VOL_LDO1])
			axp209_volt_data[POWER_VOL_LDO1] = axp209_get_voltage(POWER_VOL_LDO1);
		axp209_set_voltage(POWER_VOL_LDO1, volt);
		break;
	case AXP209_LDO2:
		if (0 == axp209_volt_data[POWER_VOL_LDO2])
			axp209_volt_data[POWER_VOL_LDO2] = axp209_get_voltage(POWER_VOL_LDO2);
		axp209_set_voltage(POWER_VOL_LDO2, volt);
		break;
	case AXP209_LDO3:
		if (0 == axp209_volt_data[POWER_VOL_LDO3])
			axp209_volt_data[POWER_VOL_LDO3] = axp209_get_voltage(POWER_VOL_LDO3);
		axp209_set_voltage(POWER_VOL_LDO3, volt);
		break;
	case AXP209_LDO4:
		if (0 == axp209_volt_data[POWER_VOL_LDO4])
			axp209_volt_data[POWER_VOL_LDO4] = axp209_get_voltage(POWER_VOL_LDO4);
		axp209_set_voltage(POWER_VOL_LDO4, volt);
		break;
	default:
		return -1;
	}

	return 0;
}

/*
*********************************************************************************************************
*                           axp209_recovery_volt
*
*Description: axp209 recovery voltage for resume;
*
*Arguments  :
*
*Return     : success: 0;
*             failed : -1;
*Notes      :
*
*********************************************************************************************************
*/
int axp209_recovery_volt(void)
{
	int i = 0;

	for (i=0; i<POWER_VOL_MAX; i++) {
		if (0 != axp209_volt_data[i]) {
			axp209_set_voltage(i, axp209_volt_data[i]);
			axp209_volt_data[i] = 0;
		}

	}
	return 0;
}

