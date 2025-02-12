#ifndef __SMP8821_H__
#define __SMP8821_H__

enum SPM8821_reg {
	SPM8821_ID_DCDC1,
	SPM8821_ID_DCDC2,
	SPM8821_ID_DCDC3,
	SPM8821_ID_DCDC4,
	SPM8821_ID_DCDC5,
	SPM8821_ID_DCDC6,
	SPM8821_ID_LDO1,
	SPM8821_ID_LDO2,
	SPM8821_ID_LDO3,
	SPM8821_ID_LDO4,
	SPM8821_ID_LDO5,
	SPM8821_ID_LDO6,
	SPM8821_ID_LDO7,
	SPM8821_ID_LDO8,
	SPM8821_ID_LDO9,
	SPM8821_ID_LDO10,
	SPM8821_ID_LDO11,
	SPM8821_ID_SWITCH1,
};

#define KY_SPM8821_MAX_REG	0xB0

#define SPM8821_VERSION_ID_REG		0xa1

#define SPM8821_BUCK_VSEL_MASK		0xff
#define SMP8821_BUCK_EN_MASK		0x1

#define SPM8821_BUCK1_CTRL_REG		0x47
#define SPM8821_BUCK2_CTRL_REG		0x4a
#define SPM8821_BUCK3_CTRL_REG		0x4d
#define SPM8821_BUCK4_CTRL_REG		0x50
#define SPM8821_BUCK5_CTRL_REG		0x53
#define SPM8821_BUCK6_CTRL_REG		0x56

#define SPM8821_BUCK1_VSEL_REG		0x48
#define SPM8821_BUCK2_VSEL_REG		0x4b
#define SPM8821_BUCK3_VSEL_REG		0x4e
#define SPM8821_BUCK4_VSEL_REG		0x51
#define SPM8821_BUCK5_VSEL_REG		0x54
#define SPM8821_BUCK6_VSEL_REG		0x57

#define SPM8821_ALDO1_CTRL_REG		0x5b
#define SPM8821_ALDO2_CTRL_REG		0x5e
#define SPM8821_ALDO3_CTRL_REG		0x61
#define SPM8821_ALDO4_CTRL_REG		0x64

#define SPM8821_ALDO1_VOLT_REG		0x5c
#define SPM8821_ALDO2_VOLT_REG		0x5f
#define SPM8821_ALDO3_VOLT_REG		0x62
#define SPM8821_ALDO4_VOLT_REG		0x65

#define SPM8821_ALDO_EN_MASK		0x1
#define SPM8821_ALDO_VSEL_MASK		0x7f

#define SPM8821_DLDO1_CTRL_REG		0x67
#define SPM8821_DLDO2_CTRL_REG		0x6a
#define SPM8821_DLDO3_CTRL_REG		0x6d
#define SPM8821_DLDO4_CTRL_REG		0x70
#define SPM8821_DLDO5_CTRL_REG		0x73
#define SPM8821_DLDO6_CTRL_REG		0x76
#define SPM8821_DLDO7_CTRL_REG		0x79

#define SPM8821_DLDO1_VOLT_REG		0x68
#define SPM8821_DLDO2_VOLT_REG		0x6b
#define SPM8821_DLDO3_VOLT_REG		0x6e
#define SPM8821_DLDO4_VOLT_REG		0x71
#define SPM8821_DLDO5_VOLT_REG		0x74
#define SPM8821_DLDO6_VOLT_REG		0x77
#define SPM8821_DLDO7_VOLT_REG		0x7a

#define SPM8821_DLDO_EN_MASK		0x1
#define SPM8821_DLDO_VSEL_MASK		0x7f

#define SPM8821_SWITCH_CTRL_REG		0x59
#define SPM8821_SWTICH_EN_MASK		0x1

#define SPM8821_PWR_CTRL2		0x7e
#define SPM8821_SW_SHUTDOWN_BIT_MSK	0x4
#define SPM8821_SW_RESET_BIT_MSK	0x2

#define SPM8821_NON_RESET_REG		0xAB
#define SPM8821_RESTART_CFG_BIT_MSK	0x7

#define SPM8821_SLEEP_REG_OFFSET	0x1

#define SPM8821_ADC_AUTO_REG		0x22
#define SPM8821_ADC_AUTO_BIT_MSK	0x7f

#define SPM8821_ADC_CTRL_REG		0x1e
#define SPM8821_ADC_CTRL_BIT_MSK	0x3
#define SPM8821_ADC_CTRL_EN_BIT_OFFSET	0x0
#define SPM8821_ADC_CTRL_GO_BIT_OFFSET	0x1

#define SPM8821_ADC_CFG1_REG		0x20

#define SPM8821_ADC_CFG1_ADC_CHOP_EN_BIT_OFFSET		0x6
#define SPM8821_ADC_CFG1_ADC_CHOP_EN_BIT_MSK		0x40

#define SPM8821_ADC_CFG1_ADC_CHNNL_SEL_BIT_OFFSET	0x3
#define SPM8821_ADC_CFG1_ADC_CHNNL_SEL_BIT_MSK		0x38

#define SPM8821_ADC_CFG2_REG				0x21
#define SPM8821_ADC_CFG2_REF_SEL_BIT_OFFSET		0x0
#define SPM8821_ADC_CFG2_REF_SEL_BIT_MSK		0x3
#define SPM8821_ADC_CFG2_3V3_REF			0x2

#define SPM8821_ADC_CFG2_7_DEB_NUM			0x7
#define SPM8821_ADC_CFG2_DEB_NUM_BIT_MSK		0x70
#define SPM8821_ADC_CFG2_DEB_NUM_BIT_OFFSET		0x4

#define SPM8821_ADC_EXTERNAL_CHANNEL_OFFSET		2

#define SPM8821_ADCIN0_RES_H_REG			0x2a
#define SPM8821_ADCIN0_RES_L_REG			0x2b
#define SPM8821_ADCIN0_REG_L_BIT_MSK			0xf0

#define SPM8821_REGMAP_CONFIG	\
	static const struct regmap_config spm8821_regmap_config = {	\
		.reg_bits = 8,	\
		.val_bits = 8,	\
		.max_register = KY_SPM8821_MAX_REG,	\
		.cache_type = REGCACHE_RBTREE,	\
	};

/* regulator configuration */
#define SPM8821_DESC(_id, _match, _supply, _nv, _vr, _vm, _er, _em, _lr)	\
	SPM8XX_DESC_COMMON(_id, _match, _supply, _nv, _vr, _vm, _er, _em, _lr,	\
			&pmic_dcdc_ldo_ops)

#define SPM8821_DESC_SWITCH(_id, _match, _supply, _ereg, _emask)	\
	SPM8XX_DESC_SWITCH_COM(_id, _match, _supply, _ereg, _emask,	\
	0, 0, &pmic_switch_ops)


#define SPM8821_BUCK_LINER_RANGE					\
static const struct linear_range spm8821_buck_ranges[] = {		\
        REGULATOR_LINEAR_RANGE(500000, 0x0, 0xaa, 5000),		\
        REGULATOR_LINEAR_RANGE(1375000, 0xab, 0xff, 25000),		\
};


#define SPM8821_LDO_LINER_RANGE						\
static const struct linear_range spm8821_ldo_ranges[] = {		\
        REGULATOR_LINEAR_RANGE(500000, 0xb, 0x7f, 25000),		\
};

#define SPM8821_REGULATOR_DESC		\
static const struct regulator_desc spm8821_reg[] = {	\
	/* BUCK */		\
	SPM8821_DESC(SPM8821_ID_DCDC1, "DCDC_REG1", "vcc_sys",		\
			255, SPM8821_BUCK1_VSEL_REG, SPM8821_BUCK_VSEL_MASK,	\
			SPM8821_BUCK1_CTRL_REG, SMP8821_BUCK_EN_MASK,		\
			spm8821_buck_ranges),	\
	\
	SPM8821_DESC(SPM8821_ID_DCDC2, "DCDC_REG2", "vcc_sys",		\
			255, SPM8821_BUCK2_VSEL_REG, SPM8821_BUCK_VSEL_MASK,	\
			SPM8821_BUCK2_CTRL_REG, SMP8821_BUCK_EN_MASK,	\
			spm8821_buck_ranges),	\
	\
	SPM8821_DESC(SPM8821_ID_DCDC3, "DCDC_REG3", "vcc_sys",		\
			255, SPM8821_BUCK3_VSEL_REG, SPM8821_BUCK_VSEL_MASK,	\
			SPM8821_BUCK3_CTRL_REG, SMP8821_BUCK_EN_MASK,	\
			spm8821_buck_ranges),	\
	\
	SPM8821_DESC(SPM8821_ID_DCDC4, "DCDC_REG4", "vcc_sys",	\
			255, SPM8821_BUCK4_VSEL_REG, SPM8821_BUCK_VSEL_MASK,	\
			SPM8821_BUCK4_CTRL_REG, SMP8821_BUCK_EN_MASK,	\
			spm8821_buck_ranges),	\
	\
	SPM8821_DESC(SPM8821_ID_DCDC5, "DCDC_REG5", "vcc_sys",		\
			255, SPM8821_BUCK5_VSEL_REG, SPM8821_BUCK_VSEL_MASK,	\
			SPM8821_BUCK5_CTRL_REG, SMP8821_BUCK_EN_MASK,	\
			spm8821_buck_ranges),	\
	\
	SPM8821_DESC(SPM8821_ID_DCDC6, "DCDC_REG6", "vcc_sys",	\
			255, SPM8821_BUCK6_VSEL_REG, SPM8821_BUCK_VSEL_MASK,	\
			SPM8821_BUCK6_CTRL_REG, SMP8821_BUCK_EN_MASK,	\
			spm8821_buck_ranges),	\
	/* ALDO */	\
	SPM8821_DESC(SPM8821_ID_LDO1, "LDO_REG1", "vcc_sys",	\
			128, SPM8821_ALDO1_VOLT_REG, SPM8821_ALDO_VSEL_MASK,	\
			SPM8821_ALDO1_CTRL_REG, SPM8821_ALDO_EN_MASK, spm8821_ldo_ranges),	\
	\
	SPM8821_DESC(SPM8821_ID_LDO2, "LDO_REG2", "vcc_sys",	\
			128, SPM8821_ALDO2_VOLT_REG, SPM8821_ALDO_VSEL_MASK,	\
			SPM8821_ALDO2_CTRL_REG, SPM8821_ALDO_EN_MASK, spm8821_ldo_ranges),	\
	\
	SPM8821_DESC(SPM8821_ID_LDO3, "LDO_REG3", "vcc_sys",	\
			128, SPM8821_ALDO3_VOLT_REG, SPM8821_ALDO_VSEL_MASK,	\
			SPM8821_ALDO3_CTRL_REG, SPM8821_ALDO_EN_MASK, spm8821_ldo_ranges),	\
	\
	SPM8821_DESC(SPM8821_ID_LDO4, "LDO_REG4", "vcc_sys",	\
			128, SPM8821_ALDO4_VOLT_REG, SPM8821_ALDO_VSEL_MASK,	\
			SPM8821_ALDO4_CTRL_REG, SPM8821_ALDO_EN_MASK, spm8821_ldo_ranges),	\
	\
	/* DLDO */	\
	SPM8821_DESC(SPM8821_ID_LDO5, "LDO_REG5", "dcdc5",		\
			128, SPM8821_DLDO1_VOLT_REG, SPM8821_DLDO_VSEL_MASK,	\
			SPM8821_DLDO1_CTRL_REG, SPM8821_DLDO_EN_MASK, spm8821_ldo_ranges),	\
	\
	SPM8821_DESC(SPM8821_ID_LDO6, "LDO_REG6", "dcdc5",	\
			128, SPM8821_DLDO2_VOLT_REG, SPM8821_DLDO_VSEL_MASK,	\
			SPM8821_DLDO2_CTRL_REG, SPM8821_DLDO_EN_MASK, spm8821_ldo_ranges),	\
	\
	SPM8821_DESC(SPM8821_ID_LDO7, "LDO_REG7", "dcdc5",		\
			128, SPM8821_DLDO3_VOLT_REG, SPM8821_DLDO_VSEL_MASK,	\
			SPM8821_DLDO3_CTRL_REG, SPM8821_DLDO_EN_MASK, spm8821_ldo_ranges),	\
	\
	SPM8821_DESC(SPM8821_ID_LDO8, "LDO_REG8", "dcdc5",		\
			128, SPM8821_DLDO4_VOLT_REG, SPM8821_DLDO_VSEL_MASK,	\
			SPM8821_DLDO4_CTRL_REG, SPM8821_DLDO_EN_MASK, spm8821_ldo_ranges),	\
	\
	SPM8821_DESC(SPM8821_ID_LDO9, "LDO_REG9", "dcdc5",		\
			128, SPM8821_DLDO5_VOLT_REG, SPM8821_DLDO_VSEL_MASK,	\
			SPM8821_DLDO5_CTRL_REG, SPM8821_DLDO_EN_MASK, spm8821_ldo_ranges),	\
	\
	SPM8821_DESC(SPM8821_ID_LDO10, "LDO_REG10", "dcdc5",		\
			128, SPM8821_DLDO6_VOLT_REG, SPM8821_DLDO_VSEL_MASK,	\
			SPM8821_DLDO6_CTRL_REG, SPM8821_DLDO_EN_MASK, spm8821_ldo_ranges),	\
	\
	SPM8821_DESC(SPM8821_ID_LDO11, "LDO_REG11", "dcdc5",		\
			128, SPM8821_DLDO7_VOLT_REG, SPM8821_DLDO_VSEL_MASK,	\
			SPM8821_DLDO7_CTRL_REG, SPM8821_DLDO_EN_MASK, spm8821_ldo_ranges),	\
	\
	/* PWR SWITCH */	\
	SPM8821_DESC_SWITCH(SPM8821_ID_SWITCH1, "SWITCH_REG1", "vcc_sys", SPM8821_SWITCH_CTRL_REG, SPM8821_SWTICH_EN_MASK),		\
};

#define SPM8821_ADC_IIO_DESC						\
static struct iio_chan_spec spm8821_iio_desc[] = {		\
	{								\
		.indexed = 1,						\
		.type = IIO_VOLTAGE,					\
		.channel = 0,						\
		.info_mask_separate = BIT(IIO_CHAN_INFO_RAW) | BIT(IIO_CHAN_INFO_SCALE),		\
	},								\
	{								\
		.indexed = 1,						\
		.type = IIO_VOLTAGE,					\
		.channel = 1,						\
		.info_mask_separate = BIT(IIO_CHAN_INFO_RAW) | BIT(IIO_CHAN_INFO_SCALE),		\
	},								\
	{								\
		.indexed = 1,						\
		.type = IIO_VOLTAGE,					\
		.channel = 2,						\
		.info_mask_separate = BIT(IIO_CHAN_INFO_RAW) | BIT(IIO_CHAN_INFO_SCALE),		\
	},								\
	{								\
		.indexed = 1,						\
		.type = IIO_VOLTAGE,					\
		.channel = 3,						\
		.info_mask_separate = BIT(IIO_CHAN_INFO_RAW) | BIT(IIO_CHAN_INFO_SCALE),		\
	},								\
	{								\
		.indexed = 1,						\
		.type = IIO_VOLTAGE,					\
		.channel = 4,						\
		.info_mask_separate = BIT(IIO_CHAN_INFO_RAW) | BIT(IIO_CHAN_INFO_SCALE),		\
	},								\
	{								\
		.indexed = 1,						\
		.type = IIO_VOLTAGE,					\
		.channel = 5,						\
		.info_mask_separate = BIT(IIO_CHAN_INFO_RAW) | BIT(IIO_CHAN_INFO_SCALE),		\
	},								\
};

/* gpio set */
#define SPM8821_PINMUX_DESC		\
const char* spm8821_pinmux_functions[] = {	\
	"gpioin", "gpioout", "exten", "pwrctrl",	\
	"sleep", "nreset", "adcin"			\
};

#define SPM8821_PINFUNC_DESC	\
static const struct pin_func_desc spm8821_pinfunc_desc[] = {	\
	/* PIN0 gpioin */				\
	SPM8XX_DESC_PIN_FUNC_COM(0, "gpioin", 0x8, 0x3, 0, 0, 0, 0, 0),				\
	/* PIN0 gpioout*/					\
	SPM8XX_DESC_PIN_FUNC_COM(0, "gpioout", 0x8, 0x3, 1, 0, 0, 0, 0),			\
	/* PIN0 exten */					\
	SPM8XX_DESC_PIN_FUNC_COM(0, "exten", 0x8, 0x3, 0x3, 1, 0xa, 0x7, 0x0),			\
	/* PIN0 pwrctrl */					\
	SPM8XX_DESC_PIN_FUNC_COM(0, "pwrctrl", 0x8, 0x3, 0x3, 1, 0xa, 0x7, 0x1),		\
	/* PIN0 sleep */					\
	SPM8XX_DESC_PIN_FUNC_COM(0, "sleep", 0x8, 0x3, 0x3, 1, 0xa, 0x7, 0x2),			\
	/* PIN0 nreset */					\
	SPM8XX_DESC_PIN_FUNC_COM(0, "nreset", 0x8, 0x3, 0x3, 1, 0xa, 0x7, 0x3),		\
	/* PIN0 adcin */					\
	SPM8XX_DESC_PIN_FUNC_COM(0, "adcin", 0x8, 0x3, 0x3, 1, 0xa, 0x7, 0x4),			\
	/* PIN1 gpioin */				\
	SPM8XX_DESC_PIN_FUNC_COM(1, "gpioin", 0x8, 0xc, 0, 0, 0, 0, 0),				\
	/* PIN1 gpioout*/					\
	SPM8XX_DESC_PIN_FUNC_COM(1, "gpioout", 0x8, 0xc, 1, 0, 0, 0, 0),			\
	/* PIN1 exten */					\
	SPM8XX_DESC_PIN_FUNC_COM(1, "exten", 0x8, 0xc, 0x3, 1, 0xa, 0x38, 0x0),		\
	/* PIN1 pwrctrl */					\
	SPM8XX_DESC_PIN_FUNC_COM(1, "pwrctrl", 0x8, 0xc, 0x3, 1, 0xa, 0x38, 0x1),		\
	/* PIN1 sleep */					\
	SPM8XX_DESC_PIN_FUNC_COM(1, "sleep", 0x8, 0xc, 0x3, 1, 0xa, 0x38, 0x2),		\
	/* PIN1 nreset */					\
	SPM8XX_DESC_PIN_FUNC_COM(1, "nreset", 0x8, 0xc, 0x3, 1, 0xa, 0x38, 0x3),		\
	/* PIN1 adcin */					\
	SPM8XX_DESC_PIN_FUNC_COM(1, "adcin", 0x8, 0xc, 0x3, 1, 0xa, 0x38, 0x4),		\
	/* PIN2 gpioin */				\
	SPM8XX_DESC_PIN_FUNC_COM(2, "gpioin", 0x8, 0x30, 0, 0, 0, 0, 0),			\
	/* PIN2 gpioout*/					\
	SPM8XX_DESC_PIN_FUNC_COM(2, "gpioout", 0x8, 0x30, 1, 0, 0, 0, 0),			\
	/* PIN2 exten */					\
	SPM8XX_DESC_PIN_FUNC_COM(2, "exten", 0x8, 0x30, 0x3, 1, 0xb, 0x7, 0x0),		\
	/* PIN2 pwrctrl */					\
	SPM8XX_DESC_PIN_FUNC_COM(2, "pwrctrl", 0x8, 0x30, 0x3, 1, 0xb, 0x7, 0x1),		\
	/* PIN2 sleep */					\
	SPM8XX_DESC_PIN_FUNC_COM(2, "sleep", 0x8, 0x30, 0x3, 1, 0xb, 0x7, 0x2),		\
	/* PIN2 nreset */					\
	SPM8XX_DESC_PIN_FUNC_COM(2, "nreset", 0x8, 0x30, 0x3, 1, 0xb, 0x7, 0x3),		\
	/* PIN2 adcin */					\
	SPM8XX_DESC_PIN_FUNC_COM(2, "adcin", 0x8, 0x30, 0x3, 1, 0xb, 0x7, 0x4),		\
	/* PIN3 gpioin */				\
	SPM8XX_DESC_PIN_FUNC_COM(3, "gpioin", 0x9, 0x3, 0, 0, 0, 0, 0),			\
	/* PIN3 gpioout*/					\
	SPM8XX_DESC_PIN_FUNC_COM(3, "gpioout", 0x9, 0x3, 1, 0, 0, 0, 0),			\
	/* PIN3 exten */					\
	SPM8XX_DESC_PIN_FUNC_COM(3, "exten", 0x9, 0x3, 0x3, 1, 0xb, 0x38, 0x0),		\
	/* PIN3 pwrctrl */					\
	SPM8XX_DESC_PIN_FUNC_COM(3, "pwrctrl", 0x9, 0x3, 0x3, 1, 0xb, 0x38, 0x1),		\
	/* PIN3 sleep */					\
	SPM8XX_DESC_PIN_FUNC_COM(3, "sleep", 0x9, 0x3, 0x3, 1, 0xb, 0x38, 0x2),		\
	/* PIN3 nreset */					\
	SPM8XX_DESC_PIN_FUNC_COM(3, "nreset", 0x9, 0x3, 0x3, 1, 0xb, 0x38, 0x3),		\
	/* PIN3 adcin */					\
	SPM8XX_DESC_PIN_FUNC_COM(3, "adcin", 0x9, 0x3, 0x3, 1, 0xb, 0x38, 0x4),		\
	/* PIN4 gpioin */				\
	SPM8XX_DESC_PIN_FUNC_COM(4, "gpioin", 0x9, 0xc, 0, 0, 0, 0, 0),			\
	/* PIN4 gpioout*/					\
	SPM8XX_DESC_PIN_FUNC_COM(4, "gpioout", 0x9, 0xc, 1, 0, 0, 0, 0),			\
	/* PIN4 exten */					\
	SPM8XX_DESC_PIN_FUNC_COM(4, "exten", 0x9, 0xc, 0x3, 1, 0xc, 0x7, 0x0),		\
	/* PIN4 pwrctrl */					\
	SPM8XX_DESC_PIN_FUNC_COM(4, "pwrctrl", 0x9, 0xc, 0x3, 1, 0xc, 0x7, 0x1),		\
	/* PIN4 sleep */					\
	SPM8XX_DESC_PIN_FUNC_COM(4, "sleep", 0x9, 0xc, 0x3, 1, 0xc, 0x7, 0x2),		\
	/* PIN4 nreset */					\
	SPM8XX_DESC_PIN_FUNC_COM(4, "nreset", 0x9, 0xc, 0x3, 1, 0xc, 0x7, 0x3),		\
	/* PIN4 adcin */					\
	SPM8XX_DESC_PIN_FUNC_COM(4, "adcin", 0x9, 0xc, 0x3, 1, 0xc, 0x7, 0x4),		\
	/* PIN5 gpioin */				\
	SPM8XX_DESC_PIN_FUNC_COM(5, "gpioin", 0x9, 0x30, 0, 0, 0, 0, 0),			\
	/* PIN5 gpioout*/					\
	SPM8XX_DESC_PIN_FUNC_COM(5, "gpioout", 0x9, 0x30, 1, 0, 0, 0, 0),			\
	/* PIN5 exten */					\
	SPM8XX_DESC_PIN_FUNC_COM(5, "exten", 0x9, 0x30, 0x3, 1, 0xc, 0x38, 0x0),		\
	/* PIN5 pwrctrl */					\
	SPM8XX_DESC_PIN_FUNC_COM(5, "pwrctrl", 0x9, 0x30, 0x3, 1, 0xc, 0x38, 0x1),		\
	/* PIN5 sleep */					\
	SPM8XX_DESC_PIN_FUNC_COM(5, "sleep", 0x9, 0x30, 0x3, 1, 0xc, 0x38, 0x2),		\
	/* PIN5 nreset */					\
	SPM8XX_DESC_PIN_FUNC_COM(5, "nreset", 0x9, 0x30, 0x3, 1, 0xc, 0x38, 0x3),		\
	/* PIN5 adcin */					\
	SPM8XX_DESC_PIN_FUNC_COM(5, "adcin", 0x9, 0x30, 0x3, 1, 0xc, 0x38, 0x4),		\
};

#define SPM8821_PIN_CINFIG_DESC				\
static const struct pin_config_desc spm8821_pinconfig_desc[] = \
{												\
	SPM8XX_DESC_PIN_CONFIG_COM(0, 0x0, 0x1, 0x1, 0x1, 0x2, 0x3, 0x4, 0xc0, 0x1, 0x5, 0x1, 0x6, 0x3),		\
	SPM8XX_DESC_PIN_CONFIG_COM(1, 0x0, 0x2, 0x1, 0x2, 0x2, 0xC, 0x4, 0xc0, 0x2, 0x5, 0x2, 0x6, 0xC),		\
	SPM8XX_DESC_PIN_CONFIG_COM(2, 0x0, 0x4, 0x1, 0x4, 0x2, 0x30, 0x4, 0xc0, 0x4, 0x5, 0x4, 0x6, 0x30),		\
	SPM8XX_DESC_PIN_CONFIG_COM(3, 0x0, 0x8, 0x1, 0x8, 0x3, 0x3, 0x4, 0xc0, 0x8, 0x5, 0x8, 0x7, 0x3),		\
	SPM8XX_DESC_PIN_CONFIG_COM(4, 0x0, 0x10, 0x1, 0x10, 0x3, 0xc, 0x4, 0xc0, 0x10, 0x5, 0x10, 0x7, 0xc),		\
	SPM8XX_DESC_PIN_CONFIG_COM(5, 0x0, 0x20, 0x1, 0x20, 0x3, 0x30, 0x4, 0xc0, 0x20, 0x5, 0x20, 0x7, 0x30),		\
};

/* irq description */
enum IRQ_line {
	/* reg: 0x91 */
	SPM8821_E_GPI0,
	SPM8821_E_GPI1,
	SPM8821_E_GPI2,
	SPM8821_E_GPI3,
	SPM8821_E_GPI4,
	SPM8821_E_GPI5,

	/* reg: 0x92 */
	SPM8821_E_ADC_TEMP,
	SPM8821_E_ADC_EOC,
	SPM8821_E_ADC_EOS,
	SPM8821_E_WDT_TO,
	SPM8821_E_ALARM,
	SPM8821_E_TICK,

	/* reg: 0x93 */
	SPM8821_E_LDO_OV,
	SPM8821_E_LDO_UV,
	SPM8821_E_LDO_SC,
	SPM8821_E_SW_SC,
	SPM8821_E_TEMP_WARN,
	SPM8821_E_TEMP_SEVERE,
	SPM8821_E_TEMP_CRIT,

	/* reg: 0x94 */
	SPM8821_E_BUCK1_OV,
	SPM8821_E_BUCK2_OV,
	SPM8821_E_BUCK3_OV,
	SPM8821_E_BUCK4_OV,
	SPM8821_E_BUCK5_OV,
	SPM8821_E_BUCK6_OV,

	/* reg: 0x95 */
	SPM8821_E_BUCK1_UV,
	SPM8821_E_BUCK2_UV,
	SPM8821_E_BUCK3_UV,
	SPM8821_E_BUCK4_UV,
	SPM8821_E_BUCK5_UV,
	SPM8821_E_BUCK6_UV,

	/* reg: 0x96 */
	SPM8821_E_BUCK1_SC,
	SPM8821_E_BUCK2_SC,
	SPM8821_E_BUCK3_SC,
	SPM8821_E_BUCK4_SC,
	SPM8821_E_BUCK5_SC,
	SPM8821_E_BUCK6_SC,

	/* reg: 0x97 */
	SPM8821_E_PWRON_RINTR,
	SPM8821_E_PWRON_FINTR,
	SPM8821_E_PWRON_SINTR,
	SPM8821_E_PWRON_LINTR,
	SPM8821_E_PWRON_SDINTR,
	SPM8821_E_VSYS_OV,
};

#define SPM8821_E_GPI0_MSK	BIT(0)
#define SPM8821_E_GPI1_MSK	BIT(1)
#define SPM8821_E_GPI2_MSK	BIT(2)
#define SPM8821_E_GPI3_MSK	BIT(3)
#define SPM8821_E_GPI4_MSK	BIT(4)
#define SPM8821_E_GPI5_MSK	BIT(5)

#define SPM8821_E_ADC_TEMP_MSK	BIT(0)
#define SPM8821_E_ADC_EOC_MSK	BIT(1)
#define SPM8821_E_ADC_EOS_MSK	BIT(2)
#define SPM8821_E_WDT_TO_MSK	BIT(3)
#define SPM8821_E_ALARM_MSK	BIT(4)
#define SPM8821_E_TICK_MSK	BIT(5)

#define SPM8821_E_LDO_OV_MSK	BIT(0)
#define SPM8821_E_LDO_UV_MSK	BIT(1)
#define SPM8821_E_LDO_SC_MSK	BIT(2)
#define SPM8821_E_SW_SC_MSK	BIT(3)
#define SPM8821_E_TEMP_WARN_MSK	BIT(4)
#define SPM8821_E_TEMP_SEVERE_MSK	BIT(5)
#define SPM8821_E_TEMP_CRIT_MSK		BIT(6)

#define SPM8821_E_BUCK1_OV_MSK	BIT(0)
#define SPM8821_E_BUCK2_OV_MSK	BIT(1)
#define SPM8821_E_BUCK3_OV_MSK	BIT(2)
#define SPM8821_E_BUCK4_OV_MSK	BIT(3)
#define SPM8821_E_BUCK5_OV_MSK	BIT(4)
#define SPM8821_E_BUCK6_OV_MSK	BIT(5)

#define SPM8821_E_BUCK1_UV_MSK	BIT(0)
#define SPM8821_E_BUCK2_UV_MSK	BIT(1)
#define SPM8821_E_BUCK3_UV_MSK	BIT(2)
#define SPM8821_E_BUCK4_UV_MSK	BIT(3)
#define SPM8821_E_BUCK5_UV_MSK	BIT(4)
#define SPM8821_E_BUCK6_UV_MSK	BIT(5)

#define SPM8821_E_BUCK1_SC_MSK	BIT(0)
#define SPM8821_E_BUCK2_SC_MSK	BIT(1)
#define SPM8821_E_BUCK3_SC_MSK	BIT(2)
#define SPM8821_E_BUCK4_SC_MSK	BIT(3)
#define SPM8821_E_BUCK5_SC_MSK	BIT(4)
#define SPM8821_E_BUCK6_SC_MSK	BIT(5)

#define SPM8821_E_PWRON_RINTR_MSK	BIT(0)
#define SPM8821_E_PWRON_FINTR_MSK	BIT(1)
#define SPM8821_E_PWRON_SINTR_MSK	BIT(2)
#define SPM8821_E_PWRON_LINTR_MSK	BIT(3)
#define SPM8821_E_PWRON_SDINTR_MSK	BIT(4)
#define SPM8821_E_VSYS_OV_MSK		BIT(5)

#define SPM8821_E_STATUS_REG_BASE	0x91
#define SPM8821_E_EN_REG_BASE		0x98

#define SPM8821_IRQS_DESC				\
static const struct regmap_irq spm8821_irqs[] = {	\
	[SPM8821_E_GPI0] = {				\
		.mask = SPM8821_E_GPI0_MSK,		\
		.reg_offset = 0,			\
	},						\
							\
	[SPM8821_E_GPI1] = {				\
		.mask = SPM8821_E_GPI1_MSK,		\
		.reg_offset = 0,			\
	},						\
							\
	[SPM8821_E_GPI2] = {				\
		.mask = SPM8821_E_GPI2_MSK,		\
		.reg_offset = 0,			\
	},						\
							\
	[SPM8821_E_GPI3] = {				\
		.mask = SPM8821_E_GPI3_MSK,		\
		.reg_offset = 0,			\
	},						\
							\
	[SPM8821_E_GPI4] = {				\
		.mask = SPM8821_E_GPI4_MSK,		\
		.reg_offset = 0,			\
	},						\
							\
	[SPM8821_E_GPI5] = {				\
		.mask = SPM8821_E_GPI5_MSK,		\
		.reg_offset = 0,			\
	},						\
							\
	[SPM8821_E_ADC_TEMP] = {			\
		.mask = SPM8821_E_ADC_TEMP_MSK,		\
		.reg_offset = 1,			\
	},						\
							\
	[SPM8821_E_ADC_EOC] = {				\
		.mask = SPM8821_E_ADC_EOC_MSK,		\
		.reg_offset = 1,			\
	},						\
							\
	[SPM8821_E_ADC_EOS] = {				\
		.mask = SPM8821_E_ADC_EOS_MSK,		\
		.reg_offset = 1,			\
	},						\
							\
	[SPM8821_E_WDT_TO] = {				\
		.mask = SPM8821_E_WDT_TO_MSK,		\
		.reg_offset = 1,			\
	},						\
							\
	[SPM8821_E_ALARM] = {				\
		.mask = SPM8821_E_ALARM_MSK,		\
		.reg_offset = 1,			\
	},						\
							\
	[SPM8821_E_TICK] = {				\
		.mask = SPM8821_E_TICK_MSK,		\
		.reg_offset = 1,			\
	},						\
							\
	[SPM8821_E_LDO_OV] = {				\
		.mask = SPM8821_E_LDO_OV_MSK,		\
		.reg_offset = 2,			\
	},						\
							\
	[SPM8821_E_LDO_UV] = {				\
		.mask = SPM8821_E_LDO_UV_MSK,		\
		.reg_offset = 2,			\
	},						\
							\
	[SPM8821_E_LDO_SC] = {				\
		.mask = SPM8821_E_LDO_SC_MSK,		\
		.reg_offset = 2,			\
	},						\
							\
	[SPM8821_E_SW_SC] = {				\
		.mask = SPM8821_E_SW_SC_MSK,		\
		.reg_offset = 2,			\
	},						\
							\
	[SPM8821_E_TEMP_WARN] = {			\
		.mask = SPM8821_E_TEMP_WARN_MSK,	\
		.reg_offset = 2,			\
	},						\
							\
	[SPM8821_E_TEMP_SEVERE] = {			\
		.mask = SPM8821_E_TEMP_SEVERE_MSK,	\
		.reg_offset = 2,			\
	},						\
							\
	[SPM8821_E_TEMP_CRIT] = {			\
		.mask = SPM8821_E_TEMP_CRIT_MSK,	\
		.reg_offset = 2,			\
	},						\
							\
	[SPM8821_E_BUCK1_OV] = {			\
		.mask = SPM8821_E_BUCK1_OV_MSK,		\
		.reg_offset = 3,			\
	},						\
							\
	[SPM8821_E_BUCK2_OV] = {			\
		.mask = SPM8821_E_BUCK2_OV_MSK,		\
		.reg_offset = 3,			\
	},						\
							\
	[SPM8821_E_BUCK3_OV] = {			\
		.mask = SPM8821_E_BUCK3_OV_MSK,		\
		.reg_offset = 3,			\
	},						\
							\
	[SPM8821_E_BUCK4_OV] = {			\
		.mask = SPM8821_E_BUCK4_OV_MSK,		\
		.reg_offset = 3,			\
	},						\
							\
	[SPM8821_E_BUCK5_OV] = {			\
		.mask = SPM8821_E_BUCK5_OV_MSK,		\
		.reg_offset = 3,			\
	},						\
							\
	[SPM8821_E_BUCK6_OV] = {			\
		.mask = SPM8821_E_BUCK6_OV_MSK,		\
		.reg_offset = 3,			\
	},						\
							\
	[SPM8821_E_BUCK1_UV] = {			\
		.mask = SPM8821_E_BUCK1_UV_MSK,		\
		.reg_offset = 4,			\
	},						\
							\
	[SPM8821_E_BUCK2_UV] = {			\
		.mask = SPM8821_E_BUCK2_UV_MSK,		\
		.reg_offset = 4,			\
	},						\
							\
	[SPM8821_E_BUCK3_UV] = {			\
		.mask = SPM8821_E_BUCK3_UV_MSK,		\
		.reg_offset = 4,			\
	},						\
							\
	[SPM8821_E_BUCK4_UV] = {			\
		.mask = SPM8821_E_BUCK4_UV_MSK,		\
		.reg_offset = 4,			\
	},						\
							\
	[SPM8821_E_BUCK5_UV] = {			\
		.mask = SPM8821_E_BUCK5_UV_MSK,		\
		.reg_offset = 4,			\
	},						\
							\
	[SPM8821_E_BUCK6_UV] = {			\
		.mask = SPM8821_E_BUCK6_UV_MSK,		\
		.reg_offset = 4,			\
	},						\
							\
	[SPM8821_E_BUCK1_SC] = {			\
		.mask = SPM8821_E_BUCK1_SC_MSK,		\
		.reg_offset = 5,			\
	},						\
							\
	[SPM8821_E_BUCK2_SC] = {			\
		.mask = SPM8821_E_BUCK2_SC_MSK,		\
		.reg_offset = 5,			\
	},						\
							\
	[SPM8821_E_BUCK3_SC] = {			\
		.mask = SPM8821_E_BUCK3_SC_MSK,		\
		.reg_offset = 5,			\
	},						\
							\
	[SPM8821_E_BUCK4_SC] = {			\
		.mask = SPM8821_E_BUCK4_SC_MSK,		\
		.reg_offset = 5,			\
	},						\
							\
	[SPM8821_E_BUCK5_SC] = {			\
		.mask = SPM8821_E_BUCK5_SC_MSK,		\
		.reg_offset = 5,			\
	},						\
							\
	[SPM8821_E_BUCK6_SC] = {			\
		.mask = SPM8821_E_BUCK6_SC_MSK,		\
		.reg_offset = 5,			\
	},						\
							\
	[SPM8821_E_PWRON_RINTR] = {			\
		.mask = SPM8821_E_PWRON_RINTR_MSK,	\
		.reg_offset = 6,			\
	},						\
							\
	[SPM8821_E_PWRON_FINTR] = {			\
		.mask = SPM8821_E_PWRON_FINTR_MSK,	\
		.reg_offset = 6,			\
	},						\
							\
	[SPM8821_E_PWRON_SINTR] = {			\
		.mask = SPM8821_E_PWRON_SINTR_MSK,	\
		.reg_offset = 6,			\
	},						\
							\
	[SPM8821_E_PWRON_LINTR] = {			\
		.mask = SPM8821_E_PWRON_LINTR_MSK,	\
		.reg_offset = 6,			\
	},						\
							\
	[SPM8821_E_PWRON_SDINTR] = {			\
		.mask = SPM8821_E_PWRON_SDINTR_MSK,	\
		.reg_offset = 6,			\
	},						\
							\
	[SPM8821_E_VSYS_OV] = {				\
		.mask = SPM8821_E_VSYS_OV_MSK,		\
		.reg_offset = 6,			\
	},						\
};


#define SPM8821_IRQ_CHIP_DESC				\
static const struct regmap_irq_chip spm8821_irq_chip = {	\
	.name = "spm8821",					\
	.irqs = spm8821_irqs,					\
	.num_irqs = ARRAY_SIZE(spm8821_irqs),			\
	.num_regs = 7,						\
	.status_base = SPM8821_E_STATUS_REG_BASE,		\
	.mask_base = SPM8821_E_EN_REG_BASE,			\
	.unmask_base = SPM8821_E_EN_REG_BASE,			\
	.ack_base = SPM8821_E_STATUS_REG_BASE,			\
	.init_ack_masked = true,				\
	.mask_unmask_non_inverted = true,			\
};

/* power-key */
#define SPM8821_POWER_KEY_RESOURCES_DESC			\
static const struct resource spm8821_pwrkey_resources[] = {	\
	DEFINE_RES_IRQ(SPM8821_E_PWRON_RINTR),			\
	DEFINE_RES_IRQ(SPM8821_E_PWRON_FINTR),			\
	DEFINE_RES_IRQ(SPM8821_E_PWRON_SINTR),			\
	DEFINE_RES_IRQ(SPM8821_E_PWRON_LINTR),			\
};

/* rtc desc */
#define SPM8821_RTC_RESOURCES_DESC				\
static const struct resource spm8821_rtc_resources[] = {	\
	DEFINE_RES_IRQ(SPM8821_E_ALARM),			\
};

/* adc desc */
#define SPM8821_ADC_RESOURCES_DESC				\
static const struct resource spm8821_adc_resources[] = {	\
	DEFINE_RES_IRQ(SPM8821_E_ADC_EOC),			\
};

#define SPM8821_RTC_REG_DESC					\
static const struct rtc_regdesc spm8821_regdesc = {		\
	.cnt_s = {						\
		.reg = 0xd,					\
		.msk = 0x3f,					\
	},							\
								\
	.cnt_mi = {						\
		.reg = 0xe,					\
		.msk = 0x3f,					\
	},							\
								\
	.cnt_h = {						\
		.reg = 0xf,					\
		.msk = 0x1f,					\
	},							\
								\
	.cnt_d = {						\
		.reg = 0x10,					\
		.msk = 0x1f,					\
	},							\
								\
	.cnt_mo = {						\
		.reg = 0x11,					\
		.msk = 0xf,					\
	},							\
								\
	.cnt_y = {						\
		.reg = 0x12,					\
		.msk = 0x3f,					\
	},							\
								\
	.alarm_s = {						\
		.reg = 0x13,					\
		.msk = 0x3f,					\
	},							\
								\
	.alarm_mi = {						\
		.reg = 0x14,					\
		.msk = 0x3f,					\
	},							\
								\
	.alarm_h = {						\
		.reg = 0x15,					\
		.msk = 0x1f,					\
	},							\
								\
	.alarm_d = {						\
		.reg = 0x16,					\
		.msk = 0x1f,					\
	},							\
								\
	.alarm_mo = {						\
		.reg = 0x17,					\
		.msk = 0xf,					\
	},							\
								\
	.alarm_y = {						\
		.reg = 0x18,					\
		.msk = 0x3f,					\
	},							\
								\
	.rtc_ctl = {						\
		.reg = 0x1d,					\
	},							\
}; 

/* mfd configuration */
#define SPM8821_MFD_CELL	\
	static const struct mfd_cell spm8821[] = {				\
		{								\
			.name = "ky-regulator@spm8821",			\
			.of_compatible = "pmic,regulator,spm8821",		\
		},								\
		{ 								\
			.name = "ky-pinctrl@spm8821",			\
			.of_compatible = "pmic,pinctrl,spm8821",		\
		},								\
		{								\
			.name = "ky-pwrkey@spm8821",			\
			.of_compatible = "pmic,pwrkey,spm8821",			\
			.num_resources = ARRAY_SIZE(spm8821_pwrkey_resources),	\
			.resources = &spm8821_pwrkey_resources[0],		\
		},								\
		{								\
			.name = "ky-rtc@spm8821",				\
			.of_compatible = "pmic,rtc,spm8821",			\
			.num_resources = ARRAY_SIZE(spm8821_rtc_resources),	\
			.resources = &spm8821_rtc_resources[0],			\
		},								\
		{								\
			.name = "ky-adc@spm8821",				\
			.of_compatible = "pmic,adc,spm8821",			\
			.num_resources = ARRAY_SIZE(spm8821_adc_resources),	\
			.resources = &spm8821_adc_resources[0],			\
		},								\
	};

#define SPM8821_MFD_MATCH_DATA					\
static struct mfd_match_data spm8821_mfd_match_data = {		\
	.regmap_cfg = &spm8821_regmap_config,			\
	.regmap_irq_chip = &spm8821_irq_chip,			\
	.mfd_cells = spm8821,					\
	.nr_cells = ARRAY_SIZE(spm8821),			\
	.name = "spm8821",					\
	.shutdown = {						\
		.reg = SPM8821_PWR_CTRL2,			\
		.bit = SPM8821_SW_SHUTDOWN_BIT_MSK,		\
	},							\
	.reboot = {						\
		.reg = SPM8821_PWR_CTRL2,			\
		.bit = SPM8821_SW_RESET_BIT_MSK,		\
	},							\
	.non_reset = {						\
		.reg = SPM8821_NON_RESET_REG,			\
		.bit = SPM8821_RESTART_CFG_BIT_MSK,		\
	},							\
};

#define SPM8821_PINCTRL_MATCH_DATA				\
static struct pinctrl_match_data spm8821_pinctrl_match_data = {	\
	.nr_pin_mux = ARRAY_SIZE(spm8821_pinmux_functions),	\
	.pinmux_funcs = spm8821_pinmux_functions,		\
	.nr_pin_fuc_desc = ARRAY_SIZE(spm8821_pinfunc_desc),	\
	.pinfunc_desc = spm8821_pinfunc_desc,			\
	.nr_pin_conf_desc = ARRAY_SIZE(spm8821_pinconfig_desc),	\
	.pinconf_desc = spm8821_pinconfig_desc,			\
	.name = "spm8821",					\
};

#define SPM8821_REGULATOR_MATCH_DATA					\
static struct regulator_match_data spm8821_regulator_match_data = {	\
	.nr_desc = ARRAY_SIZE(spm8821_reg),				\
	.desc = spm8821_reg,						\
	.name = "spm8821",						\
	.sleep_reg_offset = SPM8821_SLEEP_REG_OFFSET,			\
};

#define SPM8821_ADC_MATCH_DATA						\
static struct adc_match_data spm8821_adc_match_data = {			\
	.iio_desc = spm8821_iio_desc,					\
	.nr_desc = ARRAY_SIZE(spm8821_iio_desc),			\
	.name = "spm8821",						\
};

#endif /* __SPM8821_H__ */
