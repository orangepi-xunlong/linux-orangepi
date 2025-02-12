#ifndef __KY_PMIC_H__
#define __KY_PMIC_H__

#include <linux/regulator/machine.h>
#include <linux/regmap.h>

/**
 * this is only used for pm853
 */
struct ky_sub_pmic {
	struct i2c_client *power_page;
	struct regmap *power_regmap;
	unsigned short power_page_addr;
};

struct ky_pmic {
	struct i2c_client		*i2c;
	struct regmap_irq_chip_data	*irq_data;
	struct regmap			*regmap;
	const struct regmap_config	*regmap_cfg;
	const struct regmap_irq_chip	*regmap_irq_chip;
	/**
	 * this is only used for pm853
	 */
	struct ky_sub_pmic *sub;
};

/* pinctrl */
struct pin_func_desc {
	const char *name;
	unsigned char pin_id;
	unsigned char func_reg;
	unsigned char func_mask;
	unsigned char en_val;
	unsigned char ha_sub;
	unsigned char sub_reg;
	unsigned char sub_mask;
	unsigned char sube_val;
};

struct pin_config_desc {
	unsigned int pin_id;
	/* input config desc */
	struct {
		unsigned char reg;
		unsigned char msk;
	} input;

	/* output config desc */
	struct {
		unsigned char reg;
		unsigned char msk;
	} output;

	/* pull-down desc */
	struct {
		unsigned char reg;
		unsigned char msk;
	} pup;

	/* deb */
	struct {
		unsigned char reg;
		unsigned char timemsk;

		struct {
			unsigned char msk;
		} en;
	} deb;

	/* OD */
	struct {
		unsigned char reg;
		unsigned char msk;
	} od;

	struct {
		unsigned char reg;
		unsigned char msk;
	} itype;
};

/* rtc */
union rtc_ctl_desc {
	unsigned int val;
	struct {
		unsigned int crystal_en:1;
		unsigned int out_32k_en:1;
		unsigned int rtc_en:1;
		unsigned int rtc_clk_sel:1;
		unsigned int tick_type:1;
		unsigned int alarm_en:1;
		unsigned int tick_en:1;
		unsigned int reserved:25;
	} bits;
};

struct rtc_regdesc {
	/* seconds */
	struct {
		unsigned char reg;
		unsigned char msk;
	} cnt_s;

	/* mini */
	struct {
		unsigned char reg;
		unsigned char msk;
	} cnt_mi;

	/* hour */
	struct {
		unsigned char reg;
		unsigned char msk;
	} cnt_h;

	/* day */
	struct {
		unsigned char reg;
		unsigned char msk;
	} cnt_d;

	/* mounth */
	struct {
		unsigned char reg;
		unsigned char msk;
	} cnt_mo;

	/* year */
	struct {
		unsigned char reg;
		unsigned char msk;
	} cnt_y;

	struct {
		unsigned char reg;
		unsigned char msk;
	} alarm_s;

	struct {
		unsigned char reg;
		unsigned char msk;
	} alarm_mi;

	struct {
		unsigned char reg;
		unsigned char msk;
	} alarm_h;

	struct {
		unsigned char reg;
		unsigned char msk;
	} alarm_d;

	struct {
		unsigned char reg;
		unsigned char msk;
	} alarm_mo;

	struct {
		unsigned char reg;
		unsigned char msk;
	} alarm_y;

	struct  {
		unsigned char reg;
	} rtc_ctl;
};

/* mfd: match data */
struct mfd_match_data {
	const struct regmap_config *regmap_cfg;
	const struct regmap_irq_chip *regmap_irq_chip;
	const struct mfd_cell *mfd_cells;
	int nr_cells;
	const char *name;
	void *ptr;

	/* shutdown - reboot support */
	struct {
		unsigned char reg;
		unsigned char bit;
	} shutdown;

	struct {
		unsigned int reg;
		unsigned char bit;
	} reboot;

	/* value will be kept in register while reset pmic  */
	struct {
		unsigned int reg;
		unsigned char bit;
	} non_reset;
};

/* regulator: match data */
struct regulator_match_data {
	int nr_desc;
	int sleep_reg_offset;
	const struct regulator_desc *desc;
	const char *name;
};

/* pinctrl: match data */
struct pinctrl_match_data {
	int nr_pin_mux;
	const char **pinmux_funcs;
	int nr_pin_fuc_desc;
	const struct pin_func_desc *pinfunc_desc;
	int nr_pin_conf_desc;
	const struct pin_config_desc *pinconf_desc;
	const char *name;
};

struct adc_match_data {
	int nr_desc;
	const char *name;
	struct iio_chan_spec *iio_desc;
};

/* common regulator defination */
#define SPM8XX_DESC_COMMON(_id, _match, _supply, _nv, _vr, _vm, _er, _em, _lr, _ops)       \
	{								\
		.name		= (_match),				\
		.supply_name	= (_supply),				\
		.of_match	= of_match_ptr(_match),			\
		.regulators_node = of_match_ptr("regulators"),		\
		.ops		= _ops,			\
		.type		= REGULATOR_VOLTAGE,			\
		.id		= (_id),				\
		.n_voltages     = (_nv),				\
		.owner		= THIS_MODULE,				\
		.vsel_reg       = (_vr),				\
		.vsel_mask      = (_vm),				\
		.enable_reg	= (_er),				\
		.enable_mask	= (_em),				\
		.volt_table	= NULL,					\
		.linear_ranges	= (_lr),				\
		.n_linear_ranges	= ARRAY_SIZE(_lr),		\
	}

#define SPM8XX_DESC_SWITCH_COM(_id, _match, _supply, _ereg, _emask,	\
	_enval, _disval, _ops)						\
	{								\
		.name		= (_match),				\
		.supply_name	= (_supply),				\
		.of_match	= of_match_ptr(_match),			\
		.regulators_node = of_match_ptr("regulators"),		\
		.type		= REGULATOR_VOLTAGE,			\
		.id		= (_id),				\
		.enable_reg	= (_ereg),				\
		.enable_mask	= (_emask),				\
		.enable_val     = (_enval),				\
		.disable_val     = (_disval),				\
		.owner		= THIS_MODULE,				\
		.ops		= _ops					\
	}

#define SPM8XX_DESC_PIN_FUNC_COM(_pin_id, _match, _ereg, _emask, 	\
	_enval,	_hsub, _subreg, _submask, _subenval			\
	)								\
	{								\
		.name		= (_match),				\
		.pin_id		= (_pin_id),				\
		.func_reg	= (_ereg),				\
		.func_mask	= (_emask),				\
		.en_val		= (_enval),				\
		.ha_sub		= (_hsub),				\
		.sub_reg	= (_subreg),				\
		.sub_mask	= (_submask),				\
		.sube_val	= (_subenval),				\
	}

#define SPM8XX_DESC_PIN_CONFIG_COM(_pin_id, _ireg, _imsk, _oreg, _omsk,		\
	_pureg, _pumsk, _debreg, _debtmsk, _debemsk, _odreg, _odmsk,		\
	_itypereg, _itypemsk							\
	)							\
	{							\
		.pin_id = (_pin_id),				\
		.input = {					\
			.reg = (_ireg),				\
			.msk = (_imsk),				\
		},						\
		.output = {					\
			.reg = (_oreg),				\
			.msk = (_omsk),				\
		},						\
		.pup = {					\
			.reg = (_pureg),			\
			.msk = (_pumsk),			\
		},						\
		.deb = {					\
			.reg = (_debreg),			\
			.timemsk = (_debtmsk),			\
			.en.msk = (_debemsk)			\
		},						\
		.od = {						\
			.reg = (_odreg),			\
			.msk = (_odmsk),			\
		},						\
		.itype = {					\
			.reg = (_itypereg),			\
			.msk = (_itypemsk),			\
		},						\
	}

#include "spm8821.h"
#include "pm853.h"
#include "sy8810l.h"

#endif /* __KY_PMIC_H__ */
