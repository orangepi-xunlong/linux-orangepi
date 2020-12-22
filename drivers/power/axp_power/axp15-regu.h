#ifndef _LINUX_AXP_REGU_H_
#define _LINUX_AXP_REGU_H_

#include <linux/mfd/axp-mfd.h>

#include "axp-cfg.h"

/*Schematic_name	regulator_ID	virtual_consumer_name	max_voltage 	min_voltage   step
------------------------------------------------------------------------------------------------
*     DCDCA		axp15_dcdca	 reg-23-cs-dcdca	1100/1520mV      600/1120	10/20mV
*     DCDCB		axp15_dcdcb	 reg-23-cs-dcdcb	2550mV           1000mV		50mV
*     DCDCC		axp15_dcdcc	 reg-23-cs-dcdcc	1100/1520mV      600/1120	10/20mV
*     DCDCD		axp15_dcdcd	 reg-23-cs-dcdcd	1500/3300mV      600/1600mV	20/100mV
*     DCDCE		axp15_dcdce	 reg-23-cs-dcdce	3400mV           1100mV		50mV
*     ALDO1		axp15_aldo1	 reg-23-cs-aldo1	3300mV           700mV		100mV
*     ALDO2		axp15_aldo2	 reg-23-cs-aldo2	3300mV           700mV		100mV
*     ALDO3		axp15_aldo3	 reg-23-cs-aldo3	3300mV           700mV		100mV
*     BLDO1		axp15_bldo1	 reg-23-cs-bldo1	1900mV           700mV		100mV
*     BLDO2		axp15_bldo2	 reg-23-cs-bldo2	1900mV           700mV		100mV
*     BLDO3		axp15_bldo3	 reg-23-cs-bldo3	1900mV           700mV		100mV
*     BLDO4		axp15_bldo4	 reg-23-cs-bldo4	1900mV           700mV		100mV
*     CLDO1		axp15_cldo1	 reg-23-cs-cldo1	3300mV           700mV		100mV
*     CLDO2		axp15_cldo2	 reg-23-cs-cldo2	3400mV/4200mV    700mV/3600mV	100mV/200mV
*     CLDO3		axp15_cldo3	 reg-23-cs-cldo3	3300mV           700mV		100mV
*/

/* AXP15 Regulator Registers */
#define AXP15_LDO1		AXP15_ALDO1_VOL
#define AXP15_LDO2	        AXP15_ALDO2_VOL
#define AXP15_LDO3	        AXP15_ALDO3_VOL
#define AXP15_LDO4	        AXP15_BLDO1_VOL
#define AXP15_LDO5	        AXP15_BLDO2_VOL
#define AXP15_LDO6	        AXP15_BLDO3_VOL
#define AXP15_LDO7	        AXP15_BLDO4_VOL
#define AXP15_LDO8	        AXP15_CLDO1_VOL
#define AXP15_LDO9		AXP15_CLDO2_VOL
#define AXP15_LDO10		AXP15_CLDO3_VOL
#define AXP15_DCDC1	        AXP15_DCA_VOL
#define AXP15_DCDC2	        AXP15_DCB_VOL
#define AXP15_DCDC3	        AXP15_DCC_VOL
#define AXP15_DCDC4	        AXP15_DCD_VOL
#define AXP15_DCDC5	        AXP15_DCE_VOL
#define AXP15_SW0	        AXP15_STATUS
                            
#define AXP15_LDO1EN		AXP15_DC_ALDO_EN
#define AXP15_LDO2EN		AXP15_DC_ALDO_EN
#define AXP15_LDO3EN		AXP15_DC_ALDO_EN
#define AXP15_LDO4EN		AXP15_BC_LDO_EN
#define AXP15_LDO5EN		AXP15_BC_LDO_EN
#define AXP15_LDO6EN		AXP15_BC_LDO_EN
#define AXP15_LDO7EN		AXP15_BC_LDO_EN
#define AXP15_LDO8EN		AXP15_BC_LDO_EN
#define AXP15_LDO9EN		AXP15_BC_LDO_EN
#define AXP15_LDO10EN		AXP15_BC_LDO_EN
#define AXP15_DCDC1EN		AXP15_DC_ALDO_EN
#define AXP15_DCDC2EN		AXP15_DC_ALDO_EN
#define AXP15_DCDC3EN		AXP15_DC_ALDO_EN
#define AXP15_DCDC4EN		AXP15_DC_ALDO_EN
#define AXP15_DCDC5EN		AXP15_DC_ALDO_EN
#define AXP15_SW0EN		AXP15_BC_LDO_EN

                            
#define AXP15_BUCKMODE		AXP15_DC_MODE2
#define AXP15_BUCKFREQ		AXP15_DC_FREQ

#define AXP_LDO(_pmic, _id, min, max, step1, vreg, shift, nbits, ereg, ebit, switch_vol, step2)	\
{									\
	.desc	= {							\
		.name	= #_pmic"_LDO" #_id,				\
		.type	= REGULATOR_VOLTAGE,				\
		.id	= _pmic##_ID_LDO##_id,				\
		.n_voltages = (step1) ? ( (switch_vol) ? ((switch_vol - min) / step1 +	\
				(max - switch_vol) / step2  +1):	\
				((max - min) / step1 +1) ): 1,		\
		.owner	= THIS_MODULE,					\
	},								\
	.min_uV		= (min) * 1000,					\
	.max_uV		= (max) * 1000,					\
	.step1_uV	= (step1) * 1000,				\
	.vol_reg	= _pmic##_##vreg,				\
	.vol_shift	= (shift),					\
	.vol_nbits	= (nbits),					\
	.enable_reg	= _pmic##_##ereg,				\
	.enable_bit	= (ebit),					\
	.switch_uV	= (switch_vol)*1000,				\
	.step2_uV	= (step2)*1000,					\
}

#define AXP_BUCK(_pmic, _id, min, max, step1, vreg, shift, nbits, ereg, ebit, switch_vol, step2) \
{									\
	.desc	= {							\
		.name	= #_pmic"_BUCK" #_id,				\
		.type	= REGULATOR_VOLTAGE,				\
		.id	= _pmic##_ID_BUCK##_id,				\
		.n_voltages = (step1) ? ( (switch_vol) ? ((switch_vol - min) / step1 +	\
				(max - switch_vol) / step2  +1):	\
				((max - min) / step1 +1) ): 1,		\
	},								\
	.min_uV		= (min) * 1000,					\
	.max_uV		= (max) * 1000,					\
	.step1_uV	= (step1) * 1000,				\
	.vol_reg	= _pmic##_##vreg,				\
	.vol_shift	= (shift),					\
	.vol_nbits	= (nbits),					\
	.enable_reg	= _pmic##_##ereg,				\
	.enable_bit	= (ebit),					\
	.switch_uV	= (switch_vol)*1000,				\
	.step2_uV	= (step2)*1000,					\
}

#define AXP_DCDC(_pmic, _id, min, max, step1, vreg, shift, nbits, ereg, ebit, switch_vol, step2)	\
{									\
	.desc	= {							\
		.name	= #_pmic"_DCDC" #_id,				\
		.type	= REGULATOR_VOLTAGE,				\
		.id	= _pmic##_ID_DCDC##_id,				\
		.n_voltages = (step1) ? ( (switch_vol) ? ((switch_vol - min) / step1 +	\
				(max - switch_vol) / step2  +1):	\
				((max - min) / step1 +1) ): 1,		\
		.owner	= THIS_MODULE,					\
	},								\
	.min_uV		= (min) * 1000,					\
	.max_uV		= (max) * 1000,					\
	.step1_uV	= (step1) * 1000,				\
	.vol_reg	= _pmic##_##vreg,				\
	.vol_shift	= (shift),					\
	.vol_nbits	= (nbits),					\
	.enable_reg	= _pmic##_##ereg,				\
	.enable_bit	= (ebit),					\
	.switch_uV	= (switch_vol)*1000,				\
	.step2_uV	= (step2)*1000,					\
}

#define AXP_SW(_pmic, _id, min, max, step1, vreg, shift, nbits, ereg, ebit, switch_vol, step2) \
{									\
	.desc	= {							\
		.name	= #_pmic"_SW" #_id,				\
		.type	= REGULATOR_VOLTAGE,				\
		.id	= _pmic##_ID_SW##_id,				\
		.n_voltages = (step1) ? ( (switch_vol) ? ((switch_vol - min) / step1 +	\
				(max - switch_vol) / step2  +1):	\
				((max - min) / step1 +1) ): 1,		\
		.owner	= THIS_MODULE,					\
	},								\
	.min_uV		= (min) * 1000,					\
	.max_uV		= (max) * 1000,					\
	.step1_uV	= (step1) * 1000,				\
	.vol_reg	= _pmic##_##vreg,				\
	.vol_shift	= (shift),					\
	.vol_nbits	= (nbits),					\
	.enable_reg	= _pmic##_##ereg,				\
	.enable_bit	= (ebit),					\
	.switch_uV	= (switch_vol)*1000,				\
	.step2_uV	= (step2)*1000,					\
}

#define AXP_REGU_ATTR(_name)						\
{									\
	.attr = { .name = #_name,.mode = 0644 },			\
	.show =  _name##_show,						\
	.store = _name##_store,						\
}

struct axp_regulator_info {
	struct regulator_desc desc;

	int	min_uV;
	int	max_uV;
	int	step1_uV;
	int	vol_reg;
	int	vol_shift;
	int	vol_nbits;
	int	enable_reg;
	int	enable_bit;
	int	switch_uV;
	int	step2_uV;
};

#endif
