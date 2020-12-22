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
#ifndef __AXP209_POWER_H__
#define __AXP209_POWER_H__

#include "standby_cfg.h"

enum power_vol_type_e{
    POWER_VOL_DCDC2 = 0,
    POWER_VOL_DCDC3,
    POWER_VOL_LDO1,
    POWER_VOL_LDO2,
    POWER_VOL_LDO3,
    POWER_VOL_LDO4,
    POWER_VOL_MAX,
};

#define AXP_ADDR        (0x34)
#define AXP_IICBUS      (0)
#define AXP20_LDO1      (0x00)
#define AXP20_LDO2      (0x28)
#define AXP20_LDO3      (0x29)
#define AXP20_LDO4      (0x28)
#define AXP20_BUCK2     (0x23)
#define AXP20_BUCK3     (0x27)

#define AXP20_PEK    	(0x36)

#define AXP20_IRQEN1    (0x40)
#define AXP20_IRQEN2    (0x41)
#define AXP20_IRQEN3    (0x42)
#define AXP20_IRQEN4    (0x43)
#define AXP20_IRQEN5    (0x44)

#define AXP20_IRQ1      (0x48)
#define AXP20_IRQ2      (0x49)
#define AXP20_IRQ3      (0x4A)
#define AXP20_IRQ4      (0x4B)
#define AXP20_IRQ5      (0x4C)

#define AXP20LDO1       1300


#define AXP(_id, min, max, step, vreg, shift, nbits)    \
{                               \
    .id = _id,                  \
    .min_uV        = (min),     \
    .max_uV        = (max),     \
    .step_uV    = (step),       \
    .vol_reg    =  (vreg),      \
    .vol_shift    = (shift),    \
    .vol_nbits    = (nbits),    \
}

struct axp_info {
    enum    power_vol_type_e id;
    int     min_uV;
    int     max_uV;
    int     step_uV;
    int     vol_reg;
    int     vol_shift;
    int     vol_nbits;
};

extern int axp209_set_volt(unsigned int tree_value, unsigned short volt);
extern int axp209_recovery_volt(void);

#endif  /* __AXP209_POWER_H__ */


