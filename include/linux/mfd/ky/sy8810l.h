#ifndef __SY8810L_H__
#define __SY8810L_H__

enum SY8810L_reg {
	SY8810L_ID_DCDC1,
};

#define KY_SY8810L_MAX_REG	0x2

#define SY8810L_BUCK_VSEL_MASK		0x3f
#define SY8810L_BUCK_EN_MASK		0x80

#define SY8810L_BUCK_CTRL_REG		0x1
#define SY8810L_BUCK_VSEL_REG		0x0

#define SY8810L_REGMAP_CONFIG	\
	static const struct regmap_config sy8810l_regmap_config = {	\
		.reg_bits = 8,	\
		.val_bits = 8,	\
		.max_register = KY_SY8810L_MAX_REG,	\
		.cache_type = REGCACHE_RBTREE,	\
	};

/* regulator configuration */
#define SY8810L_DESC(_id, _match, _supply, _nv, _vr, _vm, _er, _em, _lr)	\
	SPM8XX_DESC_COMMON(_id, _match, _supply, _nv, _vr, _vm, _er, _em, _lr,	\
			&pmic_dcdc_ldo_ops)

#define SY8810L_BUCK_LINER_RANGE					\
static const struct linear_range sy8810l_buck_ranges[] = {		\
        REGULATOR_LINEAR_RANGE(600000, 0x0, 0x5a, 10000),		\
};

#define SY8810L_REGULATOR_DESC		\
static const struct regulator_desc sy8810l_reg[] = {	\
	/* BUCK */		\
	SY8810L_DESC(SY8810L_ID_DCDC1, "EDCDC_REG1", "dcdc1",		\
			91, SY8810L_BUCK_VSEL_REG, SY8810L_BUCK_VSEL_MASK,	\
			SY8810L_BUCK_CTRL_REG, SY8810L_BUCK_EN_MASK,		\
			sy8810l_buck_ranges),	\
};

/* mfd configuration */
#define SY8810L_MFD_CELL	\
	static const struct mfd_cell sy8810l[] = {				\
		{								\
			.name = "ky-regulator@sy8810l",			\
			.of_compatible = "pmic,regulator,sy8810l",		\
		},								\
	};

#define SY8810L_MFD_MATCH_DATA					\
static struct mfd_match_data sy8810l_mfd_match_data = {		\
	.regmap_cfg = &sy8810l_regmap_config,			\
	.mfd_cells = sy8810l,					\
	.nr_cells = ARRAY_SIZE(sy8810l),			\
	.name = "sy8810l",					\
};

#define SY8810L_REGULATOR_MATCH_DATA					\
static struct regulator_match_data sy8810l_regulator_match_data = {	\
	.nr_desc = ARRAY_SIZE(sy8810l_reg),				\
	.desc = sy8810l_reg,						\
	.name = "sy8810l",						\
};

#endif
