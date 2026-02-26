/* SPDX-License-Identifier: GPL-2.0-or-later */
#define pr_fmt(x) KBUILD_MODNAME ": " x "\n"

#include "axp2101_charger.h"

/* #define DONOT_Correction */
/* #define POLL_READ */
#define SOC_RISE_INTERVAL (30)
#define POLL_INTERVAL     (1 * HZ)

/* AXP210X BAT SETTING*/
#define AXP210X_MASK_WDT    (0x1 << 3)
#define AXP210X_MASK_OT     (0x1 << 2)
#define AXP210X_MASK_NEWSOC (0x1 << 1)
#define AXP210X_MASK_LOWSOC (0x1 << 0)

#define AXP210X_MODE_RSTGAUGE (0x1 << 3)
#define AXP210X_MODE_RSTMCU   (0x1 << 2)
#define AXP210X_MODE_POR      (0x1 << 4)
#define AXP210X_MODE_SLEEP    (0x1 << 0)

#define AXP210X_CFG_ENWDT       (0x1 << 5)
#define AXP210X_CFG_UPDATE_MARK (0x1 << 4)
#define AXP210X_CFG_BROMUP      (0x1 << 0)

#define AXP210X_MASK_VBUS_STATE (BIT(5))
#define AXP210X_MASK_BAT_PRST_STATE (BIT(3))

#define AXP210X_VBAT_MAX        (8000)
#define AXP210X_VBAT_MIN        (2000)
#define AXP210X_SOC_MAX         (100)
#define AXP210X_SOC_MIN         (0)

#define AXP210X_CHARGING_TRI  (0)
#define AXP210X_CHARGING_PRE  (1)
#define AXP210X_CHARGING_CC   (2)
#define AXP210X_CHARGING_CV   (3)
#define AXP210X_CHARGING_DONE (4)
#define AXP210X_CHARGING_NCHG (5)
#define AXP210X_MAX_PARAM     (512)

int axp2101_init_chip(void *data);

struct axp2101_bat_power {
	char                      *name;
	struct device             *dev;
	unsigned int                    batnum;
	struct axp_config_info     dts_info;
	struct axp210x_reg_t       regcache;
	struct axp210x_model_data  data;
	struct task_struct        *poll_read;
	int                        version;
	struct regmap             *regmap;
	struct power_supply       *bat_supply;
	struct delayed_work        bat_supply_mon;
	struct delayed_work        bat_chk;
	struct axp210x_state       stat;

};

static enum power_supply_property axp2101_bat_props[] = {
	POWER_SUPPLY_PROP_PRESENT,
	POWER_SUPPLY_PROP_VOLTAGE_NOW,
	POWER_SUPPLY_PROP_CONSTANT_CHARGE_CURRENT,
	POWER_SUPPLY_PROP_ENERGY_FULL_DESIGN,
	POWER_SUPPLY_PROP_STATUS,
	POWER_SUPPLY_PROP_CAPACITY_ALERT_MIN,
	POWER_SUPPLY_PROP_TEMP,
	POWER_SUPPLY_PROP_TEMP_AMBIENT,
	POWER_SUPPLY_PROP_CAPACITY,
	POWER_SUPPLY_PROP_TEMP_ALERT_MIN,
	POWER_SUPPLY_PROP_TEMP_ALERT_MAX,
	POWER_SUPPLY_PROP_TEMP_AMBIENT_ALERT_MIN,
	POWER_SUPPLY_PROP_TEMP_AMBIENT_ALERT_MAX,
	POWER_SUPPLY_PROP_TIME_TO_EMPTY_NOW,
	POWER_SUPPLY_PROP_TIME_TO_FULL_NOW,
	POWER_SUPPLY_PROP_MANUFACTURER,
	POWER_SUPPLY_PROP_CAPACITY_LEVEL,
};

static unsigned char axp2101_model[] = {
	0x01, 0xF5, 0x00, 0x00, 0xFB, 0x00, 0x00, 0xFB, 0x00, 0x1E, 0x32, 0x01,
	0x14, 0x04, 0xD8, 0x04, 0x74, 0xFD, 0x58, 0x0B, 0xB3, 0x10, 0x3F, 0xFB,
	0xC8, 0x00, 0xBE, 0x03, 0x4E, 0x06, 0x3F, 0x06, 0x02, 0x0A, 0xD3, 0x0F,
	0x74, 0x0F, 0x31, 0x09, 0xE5, 0x0E, 0xB9, 0x0E, 0xC0, 0x04, 0xBE, 0x04,
	0xBB, 0x09, 0xB4, 0x0E, 0xA0, 0x0E, 0x92, 0x09, 0x79, 0x0E, 0x4C, 0x0E,
	0x27, 0x03, 0xFC, 0x03, 0xD5, 0x08, 0xBC, 0x0D, 0x9C, 0x0D, 0x55, 0x06,
	0xB8, 0x2E, 0x24, 0x2E, 0x2E, 0x24, 0x2E, 0x24, 0xC5, 0x98, 0x7E, 0x66,
	0x4E, 0x44, 0x38, 0x1A, 0x12, 0x0A, 0xF6, 0x00, 0x00, 0xF6, 0x00, 0xF6,
	0x00, 0xFB, 0x00, 0x00, 0xFB, 0x00, 0x00, 0xFB, 0x00, 0x00, 0xF6, 0x00,
	0x00, 0xF6, 0x00, 0xF6, 0x00, 0xFB, 0x00, 0x00, 0xFB, 0x00, 0x00, 0xFB,
	0x00, 0x00, 0xF6, 0x00, 0x00, 0xF6, 0x00, 0xF6,
};

static struct axp210x_model_data axp2101_model_data = {
	.model = axp2101_model,
	.model_size = ARRAY_SIZE(axp2101_model),
};

static int axp2101_get_vbat_vol(struct power_supply *ps,
			     union power_supply_propval *val)
{
	struct axp2101_bat_power *bat_power = power_supply_get_drvdata(ps);
	struct regmap *regmap = bat_power->regmap;
	unsigned int data[2];
	uint16_t vtemp[3], tempv;
	int ret = 0;
	unsigned int i;

	for (i = 0; i < 3; i++) {
		ret = regmap_bulk_read(regmap, AXP2101_VBAT_H, data, 2);
		if (ret < 0)
			return ret;

		vtemp[i] = (((data[0] & GENMASK(5, 0)) << 0x08) | (data[1]));
	}
	if (vtemp[0] > vtemp[1]) {
		tempv = vtemp[0];
		vtemp[0] = vtemp[1];
		vtemp[1] = tempv;
	}
	if (vtemp[1] > vtemp[2]) {
		tempv = vtemp[1];
		vtemp[1] = vtemp[2];
		vtemp[2] = tempv;
	}
	if (vtemp[0] > vtemp[1]) {
		tempv = vtemp[0];
		vtemp[0] = vtemp[1];
		vtemp[1] = tempv;
	}
	/*incase vtemp[1] exceed AXP210X_VBAT_MAX */
	if ((vtemp[1] > AXP210X_VBAT_MAX) || (vtemp[1] < AXP210X_VBAT_MIN)) {
		val->intval = bat_power->regcache.vbat;
		return 0;
	}
	bat_power->regcache.vbat = vtemp[1];
	val->intval = vtemp[1];
	return 0;
}

static int axp2101_icchg_get(struct power_supply *ps,
				union power_supply_propval *val)
{
	struct axp2101_bat_power *bat_power = power_supply_get_drvdata(ps);
	struct regmap *regmap = bat_power->regmap;
	unsigned int data;
	int ret = 0;

	ret = regmap_read(regmap, AXP2101_ICC_CFG, &data);
	if (ret < 0)
		return ret;

	if (data <= 8)
		data = 25 * data;
	else
		data = 200 + 100 * (data - 8);

	val->intval = data;

	return 0;
}

static inline int axp_vts_to_temp(int data,
		const struct axp_config_info *axp_config)
{
	int temp;

	if (!axp_config->pmu_bat_temp_enable)
		return 300;
	else if (data < axp_config->pmu_bat_temp_para16)
		return 800;
	else if (data <= axp_config->pmu_bat_temp_para15) {
		temp = 700 + (axp_config->pmu_bat_temp_para15-data)*100/
		(axp_config->pmu_bat_temp_para15-axp_config->pmu_bat_temp_para16);
	} else if (data <= axp_config->pmu_bat_temp_para14) {
		temp = 600 + (axp_config->pmu_bat_temp_para14-data)*100/
		(axp_config->pmu_bat_temp_para14-axp_config->pmu_bat_temp_para15);
	} else if (data <= axp_config->pmu_bat_temp_para13) {
		temp = 550 + (axp_config->pmu_bat_temp_para13-data)*50/
		(axp_config->pmu_bat_temp_para13-axp_config->pmu_bat_temp_para14);
	} else if (data <= axp_config->pmu_bat_temp_para12) {
		temp = 500 + (axp_config->pmu_bat_temp_para12-data)*50/
		(axp_config->pmu_bat_temp_para12-axp_config->pmu_bat_temp_para13);
	} else if (data <= axp_config->pmu_bat_temp_para11) {
		temp = 450 + (axp_config->pmu_bat_temp_para11-data)*50/
		(axp_config->pmu_bat_temp_para11-axp_config->pmu_bat_temp_para12);
	} else if (data <= axp_config->pmu_bat_temp_para10) {
		temp = 400 + (axp_config->pmu_bat_temp_para10-data)*50/
		(axp_config->pmu_bat_temp_para10-axp_config->pmu_bat_temp_para11);
	} else if (data <= axp_config->pmu_bat_temp_para9) {
		temp = 300 + (axp_config->pmu_bat_temp_para9-data)*100/
		(axp_config->pmu_bat_temp_para9-axp_config->pmu_bat_temp_para10);
	} else if (data <= axp_config->pmu_bat_temp_para8) {
		temp = 200 + (axp_config->pmu_bat_temp_para8-data)*100/
		(axp_config->pmu_bat_temp_para8-axp_config->pmu_bat_temp_para9);
	} else if (data <= axp_config->pmu_bat_temp_para7) {
		temp = 100 + (axp_config->pmu_bat_temp_para7-data)*100/
		(axp_config->pmu_bat_temp_para7-axp_config->pmu_bat_temp_para8);
	} else if (data <= axp_config->pmu_bat_temp_para6) {
		temp = 50 + (axp_config->pmu_bat_temp_para6-data)*50/
		(axp_config->pmu_bat_temp_para6-axp_config->pmu_bat_temp_para7);
	} else if (data <= axp_config->pmu_bat_temp_para5) {
		temp = 0 + (axp_config->pmu_bat_temp_para5-data)*50/
		(axp_config->pmu_bat_temp_para5-axp_config->pmu_bat_temp_para6);
	} else if (data <= axp_config->pmu_bat_temp_para4) {
		temp = -50 + (axp_config->pmu_bat_temp_para4-data)*50/
		(axp_config->pmu_bat_temp_para4-axp_config->pmu_bat_temp_para5);
	} else if (data <= axp_config->pmu_bat_temp_para3) {
		temp = -100 + (axp_config->pmu_bat_temp_para3-data)*50/
		(axp_config->pmu_bat_temp_para3-axp_config->pmu_bat_temp_para4);
	} else if (data <= axp_config->pmu_bat_temp_para2) {
		temp = -150 + (axp_config->pmu_bat_temp_para2-data)*50/
		(axp_config->pmu_bat_temp_para2-axp_config->pmu_bat_temp_para3);
	} else if (data <= axp_config->pmu_bat_temp_para1) {
		temp = -250 + (axp_config->pmu_bat_temp_para1-data)*100/
		(axp_config->pmu_bat_temp_para1-axp_config->pmu_bat_temp_para2);
	} else
		temp = -250;
	return temp;
}

/* read battery temperature */
static int axp2101_get_temp(struct power_supply *ps,
			     union power_supply_propval *val)
{
	struct axp2101_bat_power *bat_power = power_supply_get_drvdata(ps);
	struct regmap *regmap = bat_power->regmap;
	struct axp_config_info *axp_config = &bat_power->dts_info;

	unsigned int data[2];
	uint16_t temp;
	int ret = 0, tmp = 0;

	ret = regmap_bulk_read(regmap, AXP2101_TS_H, data, 2);
	if (ret < 0)
		return ret;
	temp = (((data[0] & GENMASK(5, 0)) << 0x08) | (data[1]));
	tmp = temp * 500 / 1000;
	val->intval = axp_vts_to_temp(tmp, axp_config);

	return 0;
}

/* read ic die temperature */
static int axp2101_get_temp_ambient(struct power_supply *ps,
				union power_supply_propval *val)
{
	struct axp2101_bat_power *bat_power = power_supply_get_drvdata(ps);
	struct regmap *regmap = bat_power->regmap;

	uint8_t data[2];
	uint16_t temp;
	int ret = 0, tmp = 0;

	ret = regmap_bulk_read(regmap, AXP2101_TDIE_H, data, 2);
	if (ret < 0)
		return ret;
	temp = (((data[0] & GENMASK(5, 0)) << 0x08) | (data[1]));
	tmp = 22 + (7274 - temp) / 20;
	val->intval = tmp;

	return 0;
}

static int axp2101_param_in_ram(void *data)
{
	struct axp2101_bat_power *bat_power = data;
	struct regmap *regmap = bat_power->regmap;
	unsigned int reg_value, ret;

	ret = regmap_read(regmap, AXP2101_CONFIG, &reg_value);
	if (reg_value & AXP210X_CFG_UPDATE_MARK)
		return true;
	else
		return false;
}

static int axp2101_get_soc(struct power_supply *ps,
			    union power_supply_propval *val)
{
	struct axp2101_bat_power *bat_power = power_supply_get_drvdata(ps);
	struct regmap *regmap = bat_power->regmap;
	unsigned int data[2];
#ifdef DONOT_Correction
	static long int lasttime;
	struct timeval sysday;
#endif

	int ret = 0;

	if (!bat_power->stat.bat_stat)
		return 0;

	if (!axp2101_param_in_ram(bat_power)) {
		pr_warn("the gauge have been reset, need to reload param\n");
		axp2101_init_chip(bat_power);
		/* 500ms can read the soc */
		msleep(500);
	}

	ret = regmap_read(regmap, AXP2101_SOC, &data[0]);

	if (ret < 0)
		return ret;
	if (data[0] > AXP210X_SOC_MAX)
		data[0] = AXP210X_SOC_MAX;
	else if (data[0] < AXP210X_SOC_MIN)
		data[0] = AXP210X_SOC_MIN;
#ifdef DONOT_Correction
	ret = regmap_read(regmap, AXP2101_COMM_STAT0, &data);
	if (ret < 0)
		return ret;

	if (!!(data & AXP210X_MASK_VBUS_STATE)) {
		do_gettimeofday(&sysday);
		pr_info("systime = %ld, lastime = %ld \r\n", sysday.tv_sec, lasttime);
		if (lasttime == 0) {
			lasttime = sysday.tv_sec;
		}
		if (data[0] > bat_power->regcache.soc) {
			if (bat_power->regcache.soc < 92) {
				if (sysday.tv_sec - lasttime > (SOC_RISE_INTERVAL)) {
					bat_power->regcache.soc++;
					val->intval = bat_power->regcache.soc;
					lasttime = sysday.tv_sec;
//					pr_info("no corrention socreal = %d, socnow = %d \r\n",
//					data[0], bat_power->regcache.soc);
				}
			} else if (bat_power->regcache.soc >= 92) {
				if (sysday.tv_sec - lasttime > (SOC_RISE_INTERVAL * (bat_power->regcache.soc - 92))) {
					bat_power->regcache.soc++;
					val->intval = bat_power->regcache.soc;
					lasttime = sysday.tv_sec;
//					pr_info("no corrention socreg = %d, socnow = %d \r\n",
//					data[0], bat_power->regcache.soc);
				}
			}
		} else if (data[0] < bat_power->regcache.soc) {
			bat_power->regcache.soc--;
			val->intval = bat_power->regcache.soc;
		}

	} else {
		if (data[0] < bat_power->regcache.soc) {
			bat_power->regcache.soc--;
			val->intval = bat_power->regcache.soc;
			lasttime = 0;
		}
	}
#else
	bat_power->regcache.soc = data[0];
	val->intval = data[0];
#endif
	return 0;
}

static int axp2101_get_time2empty(struct power_supply *ps,
				   union power_supply_propval *val)
{
	struct axp2101_bat_power *bat_power = power_supply_get_drvdata(ps);
	struct regmap *regmap = bat_power->regmap;
	unsigned int data[2];
	uint16_t ttemp[3], tempt;
	int ret = 0;
	unsigned int i;

	for (i = 0; i < 3; i++) {
		ret = regmap_bulk_read(regmap, AXP2101_TIME2EMPTY_H, data, 2);
		if (ret < 0)
			return ret;
		ttemp[i] = ((data[0] << 0x08) | (data[1]));
	}
	if (ttemp[0] > ttemp[1]) {
		tempt = ttemp[0];
		ttemp[0] = ttemp[1];
		ttemp[1] = tempt;
	}
	if (ttemp[1] > ttemp[2]) {
		tempt = ttemp[1];
		ttemp[1] = ttemp[2];
		ttemp[2] = tempt;
	}
	if (ttemp[0] > ttemp[1]) {
		tempt = ttemp[0];
		ttemp[0] = ttemp[1];
		ttemp[1] = tempt;
	}
	bat_power->regcache.t2e = ttemp[1];
	val->intval = ttemp[1];
	return 0;
}

static int axp2101_get_bat_present(struct power_supply *ps,
				  union power_supply_propval *val)
{
	struct axp2101_bat_power *bat_power = power_supply_get_drvdata(ps);
	struct regmap *regmap = bat_power->regmap;
	int ret = 0;
	unsigned int reg_value;

	ret = regmap_read(regmap, AXP2101_COMM_STAT0, &reg_value);
	if (ret < 0)
		return ret;

	val->intval = !!(reg_value & AXP210X_MASK_BAT_PRST_STATE);

	return ret;
}

static int axp2101_get_time2full(struct power_supply *ps,
				  union power_supply_propval *val)
{
	struct axp2101_bat_power *bat_power = power_supply_get_drvdata(ps);
	struct regmap *regmap = bat_power->regmap;
	unsigned int data[2];
	uint16_t ttemp[3], tempt;
	int ret = 0;
	unsigned int i;

	for (i = 0; i < 3; i++) {
		ret = regmap_bulk_read(regmap, AXP2101_TIME2FULL_H, data, 2);
		if (ret < 0)
			return ret;
		ttemp[i] = ((data[0] << 0x08) | (data[1]));
	}
	if (ttemp[0] > ttemp[1]) {
		tempt = ttemp[0];
		ttemp[0] = ttemp[1];
		ttemp[1] = tempt;
	}
	if (ttemp[1] > ttemp[2]) {
		tempt = ttemp[1];
		ttemp[1] = ttemp[2];
		ttemp[2] = tempt;
	}
	if (ttemp[0] > ttemp[1]) {
		tempt = ttemp[0];
		ttemp[0] = ttemp[1];
		ttemp[1] = tempt;
	}
	bat_power->regcache.t2f = ttemp[1];
	val->intval = ttemp[1];
	return 0;
}

static int axp2101_get_lowsocth(struct power_supply *ps,
				 union power_supply_propval *val)
{
	struct axp2101_bat_power *bat_power = power_supply_get_drvdata(ps);
	struct regmap *regmap = bat_power->regmap;
	unsigned int data;
	int ret = 0;

	ret = regmap_read(regmap, AXP2101_GAUGE_THLD, &data);
	if (ret < 0)
		return ret;

	val->intval = (data >> 4) + 5;

	return 0;
}


static int axp2101_set_lowsocth(struct regmap *regmap, int v)
{
	unsigned int data;
	int ret = 0;

	data = v;

	if (data > 20 || data < 5)
		return -EINVAL;

	data = (data - 5);
	ret = regmap_update_bits(regmap, AXP2101_GAUGE_THLD, GENMASK(7, 4),
				 data);
	if (ret < 0)
		return ret;

	return 0;
}

static int axp2101_icchg_set(void *data, int mA)
{
	struct axp2101_bat_power *bat_power = data;
	struct regmap *regmap = bat_power->regmap;
	int ret = 0;

	if (mA <= 200)
		mA = mA / 25;
	else
		mA = (mA - 200) / 100 + 8;

	/* bit 4:0 is the ctrl bit */
	ret = regmap_update_bits(regmap, AXP2101_ICC_CFG, 0x1f, mA);
	if (ret < 0)
		return ret;

	return 0;
}


static int axp2101_reset_gauge(struct regmap *regmap)
{
	int ret = 0;
	unsigned int data;

	ret = regmap_read(regmap, AXP2101_RESET_CFG, &data);
	if (ret < 0)
		return ret;

	data |= AXP210X_MODE_RSTMCU;
	ret = regmap_write(regmap, AXP2101_RESET_CFG, data);
	if (ret < 0)
		return ret;

	data &= ~AXP210X_MODE_RSTMCU;
	ret = regmap_write(regmap, AXP2101_RESET_CFG, data);
	if (ret < 0)
		return ret;

	return 0;
}

static int axp2101_reset_mcu(struct regmap *regmap)
{
	int ret = 0;
	unsigned int data;

	ret = regmap_read(regmap, AXP2101_RESET_CFG, &data);
	if (ret < 0)
		return ret;
	data |= AXP210X_MODE_RSTMCU;
	ret = regmap_write(regmap, AXP2101_RESET_CFG, data);
	if (ret < 0)
		return ret;
	data &= ~AXP210X_MODE_RSTMCU;
	ret = regmap_write(regmap, AXP2101_RESET_CFG, data);
	if (ret < 0)
		return ret;

	return 0;
}

int axp2101_model_update(void *data)
{
	struct axp2101_bat_power *bat_power = data;
	struct regmap *regmap = bat_power->regmap;
	int ret = 0;
	unsigned int reg_value;
	unsigned int *para;
	unsigned int i;

	para = devm_kzalloc(bat_power->dev, bat_power->data.model_size * sizeof(int), GFP_KERNEL);
	if (!para) {
		pr_err("bat para memory allocation failed!\n");
		return -ENOMEM;
	}

	/*
	 * when write parameters, first close charger
	 */
	ret = regmap_read(regmap, AXP2101_MODULE_EN, &reg_value);
	if (ret < 0)
		goto UPDATE_ERR;
	reg_value &= ~BIT(1);
	ret = regmap_write(regmap, AXP2101_MODULE_EN, reg_value);
	if (ret < 0)
		goto UPDATE_ERR;
	msleep(1000);

	/* reset_mcu */
	ret = axp2101_reset_mcu(regmap);
	if (ret < 0)
		goto UPDATE_ERR;

	/* reset and open brom */
	ret = regmap_read(regmap, AXP2101_CONFIG, &reg_value);
	if (ret < 0)
		goto UPDATE_ERR;
	reg_value &= ~AXP210X_CFG_BROMUP;
	ret = regmap_write(regmap, AXP2101_CONFIG, reg_value);
	if (ret < 0)
		goto UPDATE_ERR;
	reg_value |= AXP210X_CFG_BROMUP;
	ret = regmap_write(regmap, AXP2101_CONFIG, reg_value);
	if (ret < 0)
		goto UPDATE_ERR;

	/* down load battery parameters */
	for (i = 0; i < bat_power->data.model_size; i++) {
		ret = regmap_write(regmap, AXP2101_BROM, bat_power->data.model[i]);
	}
	if (ret < 0)
		goto UPDATE_ERR;

	/* reset and open brom */
	ret = regmap_read(regmap, AXP2101_CONFIG, &reg_value);
	if (ret < 0)
		goto UPDATE_ERR;
	reg_value &= ~AXP210X_CFG_BROMUP;
	ret = regmap_write(regmap, AXP2101_CONFIG, reg_value);
	if (ret < 0)
		goto UPDATE_ERR;
	reg_value |= AXP210X_CFG_BROMUP;
	ret = regmap_write(regmap, AXP2101_CONFIG, reg_value);
	if (ret < 0)
		goto UPDATE_ERR;

	/* check battery parameters is ok ? */
	for (i = 0; i < bat_power->data.model_size; i++) {
		ret = regmap_read(regmap, AXP2101_BROM, &para[i]);
		if (para[i] != bat_power->data.model[i]) {
			axp210x_warn(
				"model [%d] para reading %02x != write %02x\n",
				i, para[i], bat_power->data.model[i]);
			ret = -EINVAL;
			//	goto UPDATE_ERR;
		}
	}
	if (ret < 0)
		goto UPDATE_ERR;

	/* close brom and set battery update flag */
	ret = regmap_read(regmap, AXP2101_CONFIG, &reg_value);
	if (ret < 0)
		goto UPDATE_ERR;
	reg_value &= ~AXP210X_CFG_BROMUP;
	reg_value |= AXP210X_CFG_UPDATE_MARK;
	ret = regmap_write(regmap, AXP2101_CONFIG, reg_value);
	ret = regmap_read(regmap, AXP2101_CONFIG, &reg_value);
	if (ret < 0)
		goto UPDATE_ERR;

	/* reset_mcu */
	ret = axp2101_reset_mcu(regmap);
	if (ret < 0)
		goto UPDATE_ERR;

	return 0;

UPDATE_ERR:
	ret = regmap_read(regmap, AXP2101_CONFIG, &reg_value);
	reg_value &= ~AXP210X_CFG_BROMUP;
	ret = regmap_write(regmap, AXP2101_CONFIG, reg_value);
	axp2101_reset_mcu(regmap);
	return ret;
}

static bool axp2101_model_update_check(struct regmap *regmap)
{
	int ret = 0;
	unsigned int data;
	ret = regmap_read(regmap, AXP2101_CONFIG, &data);
	if (ret < 0)
		goto CHECK_ERR;
	if ((data & AXP210X_CFG_UPDATE_MARK) == 0)
		goto CHECK_ERR;

	return true;

CHECK_ERR:

	ret = regmap_read(regmap, AXP2101_CONFIG, &data);
	data &= ~AXP210X_CFG_BROMUP;
	ret = regmap_write(regmap, AXP2101_CONFIG, data);
	axp2101_reset_mcu(regmap);
	return false;
}

static int axp2101_reg_update(void *data)
{
	struct axp2101_bat_power *bat_power = data;
	struct regmap *regmap = bat_power->regmap;
	int ret = 0;
	unsigned int reg_value[2];

	reg_value[0] = 0x10;
	ret = regmap_write(regmap, AXP2101_TS_CFG, reg_value[0]);
	if (ret < 0)
		return ret;

	ret = regmap_read(regmap, AXP2101_CONFIG, &reg_value[0]); // 0x03,
	if (ret < 0)
		return ret;
	bat_power->regcache.config.byte = reg_value[0];

	ret = regmap_bulk_read(regmap, AXP2101_VBAT_H, reg_value, 2);
	if (ret < 0)
		return ret;
	bat_power->regcache.vbat = ((reg_value[0] & GENMASK(5, 0)) << 8) + reg_value[1];

	ret = regmap_bulk_read(regmap, AXP2101_TDIE_H, reg_value, 2);
	bat_power->regcache.temp = (reg_value[0] << 8) + reg_value[1];
	if (ret < 0)
		return ret;

	ret = regmap_read(regmap, AXP2101_SOC, &reg_value[0]);
	bat_power->regcache.soc = reg_value[0];
	if (ret < 0)
		return ret;
	ret = regmap_bulk_read(regmap, AXP2101_TIME2EMPTY_H, reg_value, 2);
	if (ret < 0)
		return ret;
	bat_power->regcache.t2e = (reg_value[0] << 8) + reg_value[1];
	ret = regmap_bulk_read(regmap, AXP2101_TIME2FULL_H, reg_value, 2);
	if (ret < 0)
		return ret;
	bat_power->regcache.t2f = (reg_value[0] << 8) + reg_value[1];

	ret = regmap_read(regmap, AXP2101_GAUGE_THLD, &reg_value[0]);
	bat_power->regcache.lowsocth = (reg_value[0] >> 4) + 5;
	if (ret < 0)
		return ret;

	/* set led not bright power on first */
	regmap_update_bits(bat_power->regmap, AXP2101_CHGLED_CFG, BIT(0), 0);
	return 0;
}

static int axp2101_get_bat_status(struct power_supply *psy,
				  union power_supply_propval *val)
{
	struct axp2101_bat_power *bat_power = power_supply_get_drvdata(psy);
	struct regmap *regmap = bat_power->regmap;
	unsigned int data[2];
	int ret;

	/* some bug cause can't get battery out */
	if (!bat_power->stat.bat_stat) {
		val->intval = POWER_SUPPLY_STATUS_NOT_CHARGING;
		return 0;
	}

	ret = regmap_read(regmap, AXP2101_COMM_STAT1, &data[0]);
	if (ret) {
		dev_dbg(&psy->dev, "error read AXP210X_COM_STAT1\n");
		return ret;
	}
	/* chg_stat = bit[2:0] */
	switch (data[0] & 0x07) {
	case AXP210X_CHARGING_NCHG:
		if (((data[0] & 0x60) >> 5) != 0x1) {
			val->intval = POWER_SUPPLY_STATUS_DISCHARGING;
		} else {
			val->intval = POWER_SUPPLY_STATUS_CHARGING;
		}
		break;
	case AXP210X_CHARGING_TRI:
		val->intval = POWER_SUPPLY_STATUS_DISCHARGING;
		/* BIT[6:5] bat_curr_dir */
		/* if (((data[0] >> 5) & 0x03) == 2) { */
			/* val->intval = POWER_SUPPLY_STATUS_DISCHARGING; */
		/* } */
		break;
	case AXP210X_CHARGING_PRE:
	case AXP210X_CHARGING_CC:
	case AXP210X_CHARGING_CV:
		val->intval = POWER_SUPPLY_STATUS_CHARGING;
		break;
	case AXP210X_CHARGING_DONE:
		if (bat_power->stat.bat_full)
			val->intval = POWER_SUPPLY_STATUS_FULL;
		else
			val->intval = POWER_SUPPLY_STATUS_CHARGING;
		break;
	default:
		val->intval = POWER_SUPPLY_STATUS_UNKNOWN;
		break;
	}

	return 0;
}

static int axp2101_bat_get_property(struct power_supply *psy,
				enum power_supply_property psp,
				union power_supply_propval *val)
{
	int ret = 0;
	struct axp2101_bat_power *bat_power = power_supply_get_drvdata(psy);

	switch (psp) {
	case POWER_SUPPLY_PROP_CAPACITY_LEVEL: // customer modify
		if (!bat_power->stat.bat_stat)
			val->intval = POWER_SUPPLY_CAPACITY_LEVEL_UNKNOWN;

		ret = axp2101_get_soc(psy, val);

		bat_power->regcache.soc = val->intval;

		if (bat_power->regcache.soc == 100)
			val->intval = POWER_SUPPLY_CAPACITY_LEVEL_FULL;
		else if (bat_power->regcache.soc > 80)
			val->intval = POWER_SUPPLY_CAPACITY_LEVEL_HIGH;
		else if (bat_power->regcache.soc > bat_power->dts_info.pmu_battery_warning_level1)
			val->intval = POWER_SUPPLY_CAPACITY_LEVEL_NORMAL;
		else if (bat_power->regcache.soc > bat_power->dts_info.pmu_battery_warning_level2)
			val->intval = POWER_SUPPLY_CAPACITY_LEVEL_LOW;
		else if (bat_power->regcache.soc >= 0)
			val->intval = POWER_SUPPLY_CAPACITY_LEVEL_CRITICAL;
		else
			val->intval = POWER_SUPPLY_CAPACITY_LEVEL_UNKNOWN;
		break;
	case POWER_SUPPLY_PROP_STATUS:
		ret = axp2101_get_bat_status(psy, val);
		break;
	case POWER_SUPPLY_PROP_PRESENT:
		ret = axp2101_get_bat_present(psy, val);
		bat_power->stat.bat_stat = val->intval;
		break;
	case POWER_SUPPLY_PROP_VOLTAGE_NOW:
		ret = axp2101_get_vbat_vol(psy, val);
		val->intval = val->intval * 1000; 											//unit uV;
		break;
	case POWER_SUPPLY_PROP_CONSTANT_CHARGE_CURRENT:
		ret = axp2101_icchg_get(psy, val);
		break;
	case POWER_SUPPLY_PROP_ENERGY_FULL_DESIGN:
		val->intval = bat_power->dts_info.pmu_battery_cap;
		break;
	case POWER_SUPPLY_PROP_CAPACITY:
		if (READ_ONCE(bat_power->stat.bat_stat) &&
		    READ_ONCE(bat_power->stat.bat_read))
			ret = axp2101_get_soc(psy, val); // unit %;
		else
			val->intval = -1;
		break;
	case POWER_SUPPLY_PROP_CAPACITY_ALERT_MIN:
		ret = axp2101_get_lowsocth(psy, val);						//unit %;
		break;
	case POWER_SUPPLY_PROP_TEMP:
		ret = axp2101_get_temp(psy, val);								//unit degree celsius
		break;
	case POWER_SUPPLY_PROP_TEMP_AMBIENT:
		ret = axp2101_get_temp_ambient(psy, val);
		break;
	case POWER_SUPPLY_PROP_TEMP_ALERT_MIN:
		ret = bat_power->dts_info.pmu_bat_shutdown_ltf;
		val->intval = axp_vts_to_temp(ret, &bat_power->dts_info);
		if (!bat_power->dts_info.pmu_bat_temp_enable)
			val->intval = -200000;
		break;
	case POWER_SUPPLY_PROP_TEMP_ALERT_MAX:
		ret = bat_power->dts_info.pmu_bat_shutdown_htf;
		val->intval = axp_vts_to_temp(ret, &bat_power->dts_info);
		if (!bat_power->dts_info.pmu_bat_temp_enable)
			val->intval = 200000;
		break;
	case POWER_SUPPLY_PROP_TEMP_AMBIENT_ALERT_MIN:
		ret = bat_power->dts_info.pmu_bat_charge_ltf;
		val->intval = axp_vts_to_temp(ret, &bat_power->dts_info);
		if (!bat_power->dts_info.pmu_bat_temp_enable)
			val->intval = -200000;
		break;
	case POWER_SUPPLY_PROP_TEMP_AMBIENT_ALERT_MAX:
		ret = bat_power->dts_info.pmu_bat_charge_htf;
		val->intval = axp_vts_to_temp(ret, &bat_power->dts_info);
		if (!bat_power->dts_info.pmu_bat_temp_enable)
			val->intval = 200000;
		break;
	case POWER_SUPPLY_PROP_TIME_TO_EMPTY_NOW:
		ret = axp2101_get_time2empty(psy, val);
		val->intval = val->intval * 60;												//unit second
		break;
	case POWER_SUPPLY_PROP_TIME_TO_FULL_NOW:
		ret = axp2101_get_time2full(psy, val);
		val->intval = val->intval * 60;												//unit second
		break;
	case POWER_SUPPLY_PROP_MANUFACTURER:
		val->strval = AXP210X_MANUFACTURER;
		break;
	default:
		return -EINVAL;
	}
	return ret;
}

static int axp2101_bat_set_property(struct power_supply *psy,
				enum power_supply_property psp,
				const union power_supply_propval *val)
{
	struct axp2101_bat_power *bat_power = power_supply_get_drvdata(psy);
	struct regmap *regmap = bat_power->regmap;
	int ret = 0;

	switch (psp) {
	case POWER_SUPPLY_PROP_CAPACITY_ALERT_MIN:
		ret = axp2101_set_lowsocth(regmap, val->intval);
		break;
	case POWER_SUPPLY_PROP_CONSTANT_CHARGE_CURRENT:
		ret = axp2101_icchg_set(bat_power, val->intval);
		break;
	default:
		return -EINVAL;
	}

	pm_runtime_put_sync(bat_power->dev);

	return ret;
}

static int axp210x_writeable(struct power_supply *psy,
			     enum power_supply_property psp)
{
	int ret = 0;

	switch (psp) {
	case POWER_SUPPLY_PROP_CAPACITY_ALERT_MIN:
		ret = 1;
		break;
	case POWER_SUPPLY_PROP_CONSTANT_CHARGE_CURRENT:
		ret = 1;
		break;
	default:
		return 0;
	}

	return ret;
}

static const struct power_supply_desc axp2101_bat_desc = {
	.name = "battery",
	.type = POWER_SUPPLY_TYPE_BATTERY,
	.get_property = axp2101_bat_get_property,
	.set_property = axp2101_bat_set_property,
	.properties = axp2101_bat_props,
	.num_properties = ARRAY_SIZE(axp2101_bat_props),
	.property_is_writeable = axp210x_writeable,
};

void axp2101_teardown_battery(void *data)
{
	struct axp2101_bat_power *bat_power = data;
	if (bat_power->bat_supply)
		power_supply_unregister(bat_power->bat_supply);

}

int axp2101_init_chip(void *data)
{
	struct axp2101_bat_power *bat_power = data;
	struct axp_config_info *axp_config = &bat_power->dts_info;
	struct regmap *regmap = bat_power->regmap;
	int ret = 0;
	int reg_value;

	if (bat_power == NULL) {
		axp210x_err("axp2101_bat_power is invalid!\n");
		return -ENODEV;
	}

	ret = axp2101_reg_update(bat_power);
	if (ret < 0) {
		axp210x_err("axp210x reg update, i2c communication err!\n");
		return ret;
	}

	ret = regmap_read(regmap, AXP2101_COMM_STAT0, &reg_value);
	if (ret < 0) {
		axp210x_err("axp210x read battery prst err\n");
		return ret;
	}

	reg_value = !!(reg_value & AXP210X_MASK_BAT_PRST_STATE);

	if (reg_value && !axp2101_model_update_check(regmap)) {
		bat_power->stat.bat_init = 0;
		ret = axp2101_model_update(bat_power);
		if (ret < 0) {
			axp210x_err("axp210x model update fail!\n");
			return ret;
		}
	} else {
		bat_power->stat.bat_read = true;
	}

	axp210x_debug("axp210x model update ok\n");

	/* after 500ms can read soc */
	ret = axp2101_reg_update(bat_power);
	if (ret < 0) {
		axp210x_err("axp210x reg update, i2c communication err!\n");
		return ret;
	}

	if (axp_config->pmu_bat_temp_enable) {
		/* set disable dbg */
		regmap_update_bits(bat_power->regmap, AXP2101_VBAT_H, GENMASK(7, 6), 0);
		regmap_update_bits(bat_power->regmap, AXP2101_TS_H, BIT(6), 0);
		/* enable ts cfg*/
		regmap_update_bits(bat_power->regmap, AXP2101_TS_CFG, BIT(4), 0);
		regmap_update_bits(bat_power->regmap, AXP2101_TS_CFG, GENMASK(3, 2), AXP2101_TS_SRC_EN);
		/* set ts curr */
		regmap_read(bat_power->regmap, AXP2101_TS_CFG, &reg_value);
		reg_value &= 0xFC;
		if (axp_config->pmu_bat_ts_current < 40)
			reg_value |= 0x00;
		else if (axp_config->pmu_bat_ts_current < 50)
			reg_value |= 0x01;
		else if (axp_config->pmu_bat_ts_current < 60)
			reg_value |= 0x02;
		else
			reg_value |= 0x03;
		regmap_write(bat_power->regmap, AXP2101_TS_CFG, reg_value);
		/* set ntc settings */
		if (axp_config->pmu_bat_charge_ltf) {
			if (axp_config->pmu_bat_charge_ltf < axp_config->pmu_bat_charge_htf)
				axp_config->pmu_bat_charge_ltf = axp_config->pmu_bat_charge_htf;

			reg_value = axp_config->pmu_bat_charge_ltf / 32;
			regmap_write(bat_power->regmap, AXP2101_VLTF_CHG, reg_value);
		}

		if (axp_config->pmu_bat_charge_htf) {
			if (axp_config->pmu_bat_charge_htf > 510)
				axp_config->pmu_bat_charge_htf = 510;
			reg_value = axp_config->pmu_bat_charge_htf / 2;
			regmap_write(bat_power->regmap, AXP2101_VHTF_CHG, reg_value);
		}
		/* set work vol */
		if (axp_config->pmu_bat_shutdown_ltf) {
			if (axp_config->pmu_bat_shutdown_ltf < axp_config->pmu_bat_charge_ltf)
				axp_config->pmu_bat_shutdown_ltf = axp_config->pmu_bat_charge_ltf;

			reg_value = axp_config->pmu_bat_shutdown_ltf / 32;
			regmap_write(bat_power->regmap, AXP2101_VLTF_WORK, reg_value);
		}

		if (axp_config->pmu_bat_shutdown_htf) {
			if (axp_config->pmu_bat_shutdown_htf > axp_config->pmu_bat_charge_htf)
				axp_config->pmu_bat_shutdown_htf = axp_config->pmu_bat_charge_htf;
			reg_value = axp_config->pmu_bat_shutdown_htf / 2;
			regmap_write(bat_power->regmap, AXP2101_VHTF_WORK, reg_value);
		}
		/* set jeita enable */
		if (axp_config->pmu_jetia_en) {
			regmap_update_bits(bat_power->regmap, AXP2101_JEITA_CFG, BIT(0), 1);
			regmap_update_bits(bat_power->regmap, AXP2101_JEITA_CV_CFG, BIT(2), 0);
			regmap_update_bits(bat_power->regmap, AXP2101_JEITA_CV_CFG, BIT(3), 0);
		} else {
			regmap_update_bits(bat_power->regmap, AXP2101_JEITA_CFG, BIT(0), 0);
		}

		/* set jeita cool vol */
		if (axp_config->pmu_jetia_cool) {
			if (axp_config->pmu_jetia_cool < axp_config->pmu_jetia_warm)
				axp_config->pmu_jetia_cool = axp_config->pmu_jetia_warm;

			reg_value = axp_config->pmu_jetia_cool / 16;
			regmap_write(bat_power->regmap, AXP2101_JEITA_COOL, reg_value);
		}

		/* set jeita warm vol */
		if (axp_config->pmu_jetia_warm) {
			if (axp_config->pmu_jetia_warm > 2040)
				axp_config->pmu_jetia_warm = 2040;

			reg_value = axp_config->pmu_jetia_warm/8;
			regmap_write(bat_power->regmap, AXP2101_JEITA_WARM, reg_value);
		}

		/* set jeita config */
		regmap_read(bat_power->regmap, AXP2101_JEITA_CV_CFG, &reg_value);
		reg_value &= 0x0F;
		if (axp_config->pmu_jwarm_ifall)
			reg_value |= axp_config->pmu_jwarm_ifall << 6;

		if (axp_config->pmu_jcool_ifall)
			reg_value |= axp_config->pmu_jcool_ifall << 4;

		regmap_write(bat_power->regmap, AXP2101_JEITA_CV_CFG, reg_value);
	} else {
		regmap_update_bits(bat_power->regmap, AXP2101_TS_CFG, BIT(4), BIT(4));
	}

	return ret;
}

static irqreturn_t axp2101_irq_handler_thread(int irq, void *data)
{
	int ret = 0;
	struct irq_desc *id = irq_to_desc(irq);
	struct axp2101_bat_power *bat_power = data;

	union power_supply_propval val;
	pr_debug("%s: enter interrupt %d\n", __func__, irq);

	power_supply_changed(bat_power->bat_supply);
	switch (id->irq_data.hwirq) {
	case axp2101_IRQ_CHGDN:
		pr_debug("interrupt:charger done");
		break;
	case axp2101_IRQ_CHGST:
		pr_debug("interrutp:charger start");
		break;
	case axp2101_IRQ_BINSERT:
		pr_debug("interrupt:battery insert");
		break;
	case axp2101_IRQ_BREMOV:
		pr_debug("interrupt:battery remove");
		break;
	default:
		pr_debug("interrupt:others");
		break;
	}

	ret = axp2101_get_soc(bat_power->bat_supply, &val);
	if (ret != 0)
		printk(KERN_ALERT "%s: soc update fail!\n", __FUNCTION__);
	ret = axp2101_get_vbat_vol(bat_power->bat_supply, &val);
	if (ret != 0)
		printk(KERN_ALERT "%s: vbat update fail!\n", __FUNCTION__);

	ret = axp2101_get_bat_status(bat_power->bat_supply, &val);
	if (ret != 0)
		printk(KERN_ALERT "%s: temprature update fail!\n", __FUNCTION__);

	ret = axp2101_get_time2empty(bat_power->bat_supply, &val);
	if (ret != 0)
		printk(KERN_ALERT "%s: time2empty update fail!\n", __FUNCTION__);

	ret = axp2101_get_time2full(bat_power->bat_supply, &val);
	if (ret != 0)
		printk(KERN_ALERT "%s: time2full update fail!\n", __FUNCTION__);

	return IRQ_HANDLED;
}

#if ((defined DONOT_Correction) || (defined POLL_READ))
static void timer_handler(unsigned long arg, void *data)
{
	int ret = 0;
	struct axp2101_bat_power *bat_power = data;
	union power_supply_propval val;
	printk("%s: timer_handler work!\n", __FUNCTION__);

	ret = axp2101_get_soc(bat_power->bat_supply, &val);
	if (ret != 0)
		printk(KERN_ALERT "%s: soc update fail!\n", __FUNCTION__);
	ret = axp2101_get_vbat_vol(bat_power->bat_supply, val);
	if (ret != 0)
		printk(KERN_ALERT "%s: vbat update fail!\n", __FUNCTION__);

	ret = axp2101_get_temp(bat_power->bat_supply, &val);
	if (ret != 0)
		printk(KERN_ALERT "%s: temprature update fail!\n", __FUNCTION__);

	printk("%s: soc[%d] vbat[%d] \n", __func__, bat_power->regcache.soc,
	       bat_power->regcache.vbat);
}

static int thread_dosomthing(void *data)
{
	set_freezable();

	while (!kthread_should_stop()) {
		schedule_timeout_interruptible(POLL_INTERVAL);
		try_to_freeze();
		timer_handler(0, data);
	}

	return 0;
}
#endif

enum axp2101_virq_index {
	AXP2101_VIRQ_BAT_IN,
	AXP2101_VIRQ_BAT_OUT,
	AXP2101_VIRQ_CHARGING,
	AXP2101_VIRQ_CHARGE_OVER,
	AXP2101_VIRQ_LOW_WARNING1,
	AXP2101_VIRQ_LOW_WARNING2,
	AXP2101_VIRQ_BAT_UNTEMP_WORK,
	AXP2101_VIRQ_BAT_OVTEMP_WORK,
	AXP2101_VIRQ_BAT_UNTEMP_CHG,
	AXP2101_VIRQ_BAT_OVTEMP_CHG,

	AXP2101_VIRQ_MAX_VIRQ,
};

static struct axp_interrupts axp_bat_irq[] = {
	[AXP2101_VIRQ_BAT_IN] = { "bat in", axp2101_irq_handler_thread },
	[AXP2101_VIRQ_BAT_OUT] = { "bat out", axp2101_irq_handler_thread },
	[AXP2101_VIRQ_CHARGING] = { "charging", axp2101_irq_handler_thread },
	[AXP2101_VIRQ_CHARGE_OVER] = { "charge over",
				       axp2101_irq_handler_thread },
	[AXP2101_VIRQ_LOW_WARNING1] = { "low warning1",
					axp2101_irq_handler_thread },
	[AXP2101_VIRQ_LOW_WARNING2] = { "low warning2",
					axp2101_irq_handler_thread },
	[AXP2101_VIRQ_BAT_UNTEMP_WORK] = { "bat untemp work",
					   axp2101_irq_handler_thread },
	[AXP2101_VIRQ_BAT_OVTEMP_WORK] = { "bat ovtemp work",
					   axp2101_irq_handler_thread },
	[AXP2101_VIRQ_BAT_UNTEMP_CHG] = { "bat untemp chg",
					  axp2101_irq_handler_thread },
	[AXP2101_VIRQ_BAT_OVTEMP_CHG] = { "bat ovtemp chg",
					  axp2101_irq_handler_thread },
};

static void axp2101_charger_sysconfig(void *data)
{
	struct axp2101_bat_power *bat_power = data;
	struct regmap *regmap = bat_power->regmap;
	struct axp_config_info *dinfo = &bat_power->dts_info;
	unsigned int value;

	if (dinfo->pmu_chg_ic_temp)
		regmap_update_bits(regmap, AXP2101_TS_CFG, 0x0a, 0x0a);
	else
		regmap_update_bits(regmap, AXP2101_TS_CFG, 0x0a, 0x00);

	/* enable tdie adc channel */
	regmap_update_bits(regmap, AXP2101_ADC_CH_EN0, BIT(4), BIT(4));

	/* set charger voltage limit */
	/* 4600 is change to 2101b */
	if (dinfo->pmu_init_chgvol < 4100) {
		regmap_update_bits(regmap, AXP2101_CHG_V_CFG, 0x07, 0x01);
	} else if (dinfo->pmu_init_chgvol < 4200) {
		regmap_update_bits(regmap, AXP2101_CHG_V_CFG, 0x07, 0x02);
	} else if (dinfo->pmu_init_chgvol < 4350) {
		regmap_update_bits(regmap, AXP2101_CHG_V_CFG, 0x07, 0x03);
	} else if (dinfo->pmu_init_chgvol < 4400) {
		regmap_update_bits(regmap, AXP2101_CHG_V_CFG, 0x07, 0x04);
	} else if (dinfo->pmu_init_chgvol < 4600) {
		regmap_update_bits(regmap, AXP2101_CHG_V_CFG, 0x07, 0x05);
	} else {
		regmap_update_bits(regmap, AXP2101_CHG_V_CFG, 0x07, 0x00);
	}
	/* set button battery charge termination voltage to 2.9v */
	regmap_update_bits(regmap, AXP2101_BTN_CHG_CFG, 0x07, 0x03);
	/* set chglend func 0x69 */
	regmap_update_bits(regmap, AXP2101_CHGLED_CFG, 0x06,
			   dinfo->pmu_chgled_type << 1);
	/* set gauge_thld */
	value = clamp_val(dinfo->pmu_battery_warning_level1 - 5, 0, 15) << 4;
	value |= clamp_val(dinfo->pmu_battery_warning_level2, 0, 15);
	regmap_write(regmap, AXP2101_GAUGE_THLD, value);
}

int axp2101_bat_dt_parse(struct device_node *node,
			 struct axp_config_info *axp_config)
{
	if (!of_device_is_available(node)) {
		pr_err("%s: failed\n", __func__);
		return -1;
	}

	AXP_OF_PROP_READ(pmu_battery_rdc,              BATRDC);
	AXP_OF_PROP_READ(pmu_battery_cap,                4000);
	AXP_OF_PROP_READ(pmu_batdeten,                      1);
	AXP_OF_PROP_READ(pmu_chg_ic_temp,                   0);
	AXP_OF_PROP_READ(pmu_runtime_chgcur, INTCHGCUR / 1000);
	AXP_OF_PROP_READ(pmu_suspend_chgcur,             1200);
	AXP_OF_PROP_READ(pmu_shutdown_chgcur,            1200);
	AXP_OF_PROP_READ(pmu_init_chgvol,    INTCHGVOL / 1000);
	AXP_OF_PROP_READ(pmu_init_chgend_rate,  INTCHGENDRATE);
	AXP_OF_PROP_READ(pmu_init_chg_enabled,              1);
	AXP_OF_PROP_READ(pmu_init_bc_en,                    0);
	AXP_OF_PROP_READ(pmu_init_adc_freq,        INTADCFREQ);
	AXP_OF_PROP_READ(pmu_init_adcts_freq,     INTADCFREQC);
	AXP_OF_PROP_READ(pmu_init_chg_pretime,  INTCHGPRETIME);
	AXP_OF_PROP_READ(pmu_init_chg_csttime,  INTCHGCSTTIME);
	AXP_OF_PROP_READ(pmu_batt_cap_correct,              1);
	AXP_OF_PROP_READ(pmu_chg_end_on_en,                 0);
	AXP_OF_PROP_READ(ocv_coulumb_100,                   0);
	AXP_OF_PROP_READ(pmu_bat_para1,               OCVREG0);
	AXP_OF_PROP_READ(pmu_bat_para2,               OCVREG1);
	AXP_OF_PROP_READ(pmu_bat_para3,               OCVREG2);
	AXP_OF_PROP_READ(pmu_bat_para4,               OCVREG3);
	AXP_OF_PROP_READ(pmu_bat_para5,               OCVREG4);
	AXP_OF_PROP_READ(pmu_bat_para6,               OCVREG5);
	AXP_OF_PROP_READ(pmu_bat_para7,               OCVREG6);
	AXP_OF_PROP_READ(pmu_bat_para8,               OCVREG7);
	AXP_OF_PROP_READ(pmu_bat_para9,               OCVREG8);
	AXP_OF_PROP_READ(pmu_bat_para10,              OCVREG9);
	AXP_OF_PROP_READ(pmu_bat_para11,              OCVREGA);
	AXP_OF_PROP_READ(pmu_bat_para12,              OCVREGB);
	AXP_OF_PROP_READ(pmu_bat_para13,              OCVREGC);
	AXP_OF_PROP_READ(pmu_bat_para14,              OCVREGD);
	AXP_OF_PROP_READ(pmu_bat_para15,              OCVREGE);
	AXP_OF_PROP_READ(pmu_bat_para16,              OCVREGF);
	AXP_OF_PROP_READ(pmu_bat_para17,             OCVREG10);
	AXP_OF_PROP_READ(pmu_bat_para18,             OCVREG11);
	AXP_OF_PROP_READ(pmu_bat_para19,             OCVREG12);
	AXP_OF_PROP_READ(pmu_bat_para20,             OCVREG13);
	AXP_OF_PROP_READ(pmu_bat_para21,             OCVREG14);
	AXP_OF_PROP_READ(pmu_bat_para22,             OCVREG15);
	AXP_OF_PROP_READ(pmu_bat_para23,             OCVREG16);
	AXP_OF_PROP_READ(pmu_bat_para24,             OCVREG17);
	AXP_OF_PROP_READ(pmu_bat_para25,             OCVREG18);
	AXP_OF_PROP_READ(pmu_bat_para26,             OCVREG19);
	AXP_OF_PROP_READ(pmu_bat_para27,             OCVREG1A);
	AXP_OF_PROP_READ(pmu_bat_para28,             OCVREG1B);
	AXP_OF_PROP_READ(pmu_bat_para29,             OCVREG1C);
	AXP_OF_PROP_READ(pmu_bat_para30,             OCVREG1D);
	AXP_OF_PROP_READ(pmu_bat_para31,             OCVREG1E);
	AXP_OF_PROP_READ(pmu_bat_para32,             OCVREG1F);
	AXP_OF_PROP_READ(pmu_pwroff_vol,                 3300);
	AXP_OF_PROP_READ(pmu_pwron_vol,                  2900);
	AXP_OF_PROP_READ(pmu_battery_warning_level1,       15);
	AXP_OF_PROP_READ(pmu_battery_warning_level2,        0);
	AXP_OF_PROP_READ(pmu_restvol_adjust_time,          30);
	AXP_OF_PROP_READ(pmu_ocv_cou_adjust_time,          60);
	AXP_OF_PROP_READ(pmu_chgled_func,                   0);
	AXP_OF_PROP_READ(pmu_chgled_type,                   0);
	AXP_OF_PROP_READ(pmu_bat_temp_enable,               0);
	AXP_OF_PROP_READ(pmu_bat_ts_current,               50);
	AXP_OF_PROP_READ(pmu_bat_charge_ltf,             0xA5);
	AXP_OF_PROP_READ(pmu_bat_charge_htf,             0x1F);
	AXP_OF_PROP_READ(pmu_bat_shutdown_ltf,           0xFC);
	AXP_OF_PROP_READ(pmu_bat_shutdown_htf,           0x16);
	AXP_OF_PROP_READ(pmu_jetia_en,                      0);
	AXP_OF_PROP_READ(pmu_jetia_cool,                  880);
	AXP_OF_PROP_READ(pmu_jetia_warm,                  240);
	AXP_OF_PROP_READ(pmu_jcool_ifall,                   1);
	AXP_OF_PROP_READ(pmu_jwarm_ifall,                   0);
	AXP_OF_PROP_READ(pmu_bat_temp_para1,                0);
	AXP_OF_PROP_READ(pmu_bat_temp_para2,                0);
	AXP_OF_PROP_READ(pmu_bat_temp_para3,                0);
	AXP_OF_PROP_READ(pmu_bat_temp_para4,                0);
	AXP_OF_PROP_READ(pmu_bat_temp_para5,                0);
	AXP_OF_PROP_READ(pmu_bat_temp_para6,                0);
	AXP_OF_PROP_READ(pmu_bat_temp_para7,                0);
	AXP_OF_PROP_READ(pmu_bat_temp_para8,                0);
	AXP_OF_PROP_READ(pmu_bat_temp_para9,                0);
	AXP_OF_PROP_READ(pmu_bat_temp_para10,               0);
	AXP_OF_PROP_READ(pmu_bat_temp_para11,               0);
	AXP_OF_PROP_READ(pmu_bat_temp_para12,               0);
	AXP_OF_PROP_READ(pmu_bat_temp_para13,               0);
	AXP_OF_PROP_READ(pmu_bat_temp_para14,               0);
	AXP_OF_PROP_READ(pmu_bat_temp_para15,               0);
	AXP_OF_PROP_READ(pmu_bat_temp_para16,               0);
	AXP_OF_PROP_READ(pmu_bat_unused,                    0);
	AXP_OF_PROP_READ(power_start,                       0);
	AXP_OF_PROP_READ(pmu_ocv_en,                        1);
	AXP_OF_PROP_READ(pmu_cou_en,                        1);
	AXP_OF_PROP_READ(pmu_update_min_time,   UPDATEMINTIME);

	axp_config->wakeup_bat_in =
		of_property_read_bool(node, "wakeup_bat_in");
	axp_config->wakeup_bat_out =
		of_property_read_bool(node, "wakeup_bat_out");
	axp_config->wakeup_bat_charging =
		of_property_read_bool(node, "wakeup_bat_charging");
	axp_config->wakeup_bat_charge_over =
		of_property_read_bool(node, "wakeup_bat_charge_over");
	axp_config->wakeup_low_warning1 =
		of_property_read_bool(node, "wakeup_low_warning1");
	axp_config->wakeup_low_warning2 =
		of_property_read_bool(node, "wakeup_low_warning2");
	axp_config->wakeup_bat_untemp_work =
		of_property_read_bool(node, "wakeup_bat_untemp_work");
	axp_config->wakeup_bat_ovtemp_work =
		of_property_read_bool(node, "wakeup_bat_ovtemp_work");
	axp_config->wakeup_untemp_chg =
		of_property_read_bool(node, "wakeup_bat_untemp_chg");
	axp_config->wakeup_ovtemp_chg =
		of_property_read_bool(node, "wakeup_bat_ovtemp_chg");
	/* axp_config_obj = axp_config; */
	return 0;
}

static void axp2101_bat_parse_device_tree(void *data)
{
	struct axp2101_bat_power *bat_power = data;
	int ret;
	uint32_t prop = 0;
	struct axp_config_info *cfg;

	/* set input current limit */
	if (!bat_power->dev->of_node) {
		pr_info("can not find device tree\n");
		return;
	}

	cfg = &bat_power->dts_info;
	ret = axp2101_bat_dt_parse(bat_power->dev->of_node, cfg);
	if (ret) {
		pr_info("can not parse device tree err\n");
		return;
	}

	/* old sysconfig parse */
	axp2101_charger_sysconfig(bat_power);

	/* prechg default change to 100mA */
	if (!of_property_read_u32(bat_power->dev->of_node, "pmu_pre_chg", &prop)) {
		if (prop > 200)
			prop = 200;
		regmap_update_bits(bat_power->regmap, AXP2101_IPRECHG_CFG, 0x0f,
				   prop / 25);
	} else {
		regmap_update_bits(bat_power->regmap, AXP2101_IPRECHG_CFG, 0x0f, 0x04);
	}

	/* Termination default current limit to 50mA */
	if (!of_property_read_u32(bat_power->dev->of_node, "pmu_iterm_limit", &prop)) {
		if (prop) {
			if (prop > 200)
				prop = 200;
			regmap_update_bits(bat_power->regmap, AXP2101_ITERM_CFG, 0x0f,
					   prop / 25);
			regmap_update_bits(bat_power->regmap, AXP2101_ITERM_CFG,
					   BIT(4), BIT(4));
		} else {
			regmap_write(bat_power->regmap, AXP2101_ITERM_CFG, 0x00);
		}
	} else {
		regmap_update_bits(bat_power->regmap, AXP2101_ITERM_CFG, 0x0f, 0x02);
	}

	if (!of_property_read_u32(bat_power->dev->of_node, "pmu_chled_enable",
				  &prop) &&
	    !prop) {
		regmap_update_bits(bat_power->regmap, AXP2101_CHGLED_CFG, 0x01, 0x00);
	}

	prop = bat_power->dts_info.pmu_runtime_chgcur;
	if (!of_property_read_u32(bat_power->dev->of_node, "icc_cfg", &prop) || true) {
		prop = clamp_val(prop, 0, 2000);
		/* step is 25mA, and then 100mA step */
		if (prop <= 200)
			prop /= 25;
		else
			prop = 8 + (prop - 200) / 100;

		regmap_update_bits(bat_power->regmap, AXP2101_ICC_CFG, GENMASK(4, 0), prop);
	}

	if (!of_property_read_u32(bat_power->dev->of_node, "pmu_bat_det", &prop)) {
		regmap_write(bat_power->regmap, AXP2101_BAT_DET, (unsigned int)prop);
	}

	if (of_property_read_bool(bat_power->dev->of_node, "pmu_btn_chg_en")) {
		regmap_update_bits(bat_power->regmap, AXP2101_MODULE_EN, BIT(2),
				   BIT(2));
	} else {
		regmap_update_bits(bat_power->regmap, AXP2101_MODULE_EN, BIT(2), 0);
	}

	if (!of_property_read_u32(bat_power->dev->of_node, "pmu_btn_chg_cfg", &prop)) {
		prop = (prop - 2600) / 100;
		regmap_write(bat_power->regmap, AXP2101_BTN_CHG_CFG,
			     (unsigned int)(prop & 0x07));
	}

	if (of_property_read_bool(bat_power->dev->of_node, "pmu_batfet_ocp_en"))
		regmap_update_bits(bat_power->regmap, AXP2101_BATFET_CTRL, BIT(1), BIT(1));
	else
		regmap_update_bits(bat_power->regmap, AXP2101_BATFET_CTRL, BIT(1), 0);
}

static void battery_set_full(int *rs)
{
	static ktime_t l_time;

	if (ktime_ms_delta(ktime_get(), l_time) > MSEC_PER_SEC) {
		WRITE_ONCE(*rs, true);
	}

	l_time = ktime_get();
}

static void battery_chk_online_v1(struct work_struct *work)
{
	int ret;
	unsigned int data[2] = {0};
	static ktime_t s_chg;
	struct axp2101_bat_power *bat_power =
		container_of(work, typeof(*bat_power), bat_chk.work);
	struct regmap *regmap = bat_power->regmap;

	if (bat_power->stat.bat_init == 0) {

		bat_power->stat.bat_init = 1;

		schedule_delayed_work(&bat_power->bat_chk, msecs_to_jiffies(500));

		return;

	} else if (bat_power->stat.bat_init == 1) {

		bat_power->stat.bat_init = 2;

		ret = regmap_read(regmap, AXP2101_COMM_STAT0, &data[0]);
		if (data[0] & BIT(5)) {
			/* reset_mcu */
			ret = axp2101_reset_mcu(regmap);
			if (ret < 0)
				goto err_read;

			msleep(10);
		}

		WRITE_ONCE(bat_power->stat.bat_read, true);

		ret = regmap_read(regmap, AXP2101_MODULE_EN, &data[0]);
		if (ret < 0)
			goto err_read;

		data[0] |= BIT(1);
		ret = regmap_write(regmap, AXP2101_MODULE_EN, data[0]);
		if (ret < 0)
			goto err_read;

		schedule_delayed_work(&bat_power->bat_chk, msecs_to_jiffies(500));

		return;
	}

	/*
	 * check_full of batt, because bug of axp2101b, full flag can generate
	 * in batt plugged out
	 */
	ret = regmap_read(regmap, AXP2101_COMM_STAT1, &data[0]);
	if (ret < 0) {
		pr_info("read AXP210X_REG_VBAT error\n");
		goto err_read;
	}
	/* chg_stat is full with bit[2:0] is b100 */
	if ((data[0] & 0x07) == 0x04) {
		battery_set_full(&bat_power->stat.bat_full);
	} else {
		WRITE_ONCE(bat_power->stat.bat_full, false);
	}

	ret = regmap_read(regmap, AXP2101_COMM_STAT0, &data[0]);
	if (ret < 0) {
		pr_info("read AXP210X_COMM_STAT0 error\n");
		goto err_read;
	}

	if ((!(data[0] & BIT(3))) && (bat_power->stat.bat_stat != 0)) {
		pr_debug("bat_stat change to none\n");
		bat_power->stat.bat_stat = 0;
		WRITE_ONCE(bat_power->stat.bat_read, false);
		axp2101_reset_gauge(regmap);
		s_chg = ktime_get();
		regmap_update_bits(bat_power->regmap, AXP2101_CHGLED_CFG, BIT(0), 0);
		power_supply_changed(bat_power->bat_supply);
	}
	if ((data[0] & BIT(3)) && (bat_power->stat.bat_stat != 1)) {
		pr_debug("bat_stat change to exist\n");
		bat_power->stat.bat_stat = 1;
		axp2101_reset_gauge(regmap);
		s_chg = ktime_get();
		regmap_update_bits(bat_power->regmap, AXP2101_CHGLED_CFG, BIT(0), BIT(0));
		power_supply_changed(bat_power->bat_supply);
	}

	if (bat_power->stat.bat_stat &&
	    ktime_ms_delta(ktime_get(), s_chg) > MSEC_PER_SEC) {
		WRITE_ONCE(bat_power->stat.bat_read, true);
	}

err_read:
	schedule_delayed_work(&bat_power->bat_chk, msecs_to_jiffies(500));
}

static void battery_chk_online(struct work_struct *work)
{
	int ret;
	static int cnt;
	static int rst[3] = { 1, 1, 1 };
	unsigned int data[2] = {0};
	static ktime_t s_chg;
	struct axp2101_bat_power *bat_power =
		container_of(work, typeof(*bat_power), bat_chk.work);
	struct regmap *regmap = bat_power->regmap;

	if (bat_power->stat.bat_init == 0) {

		bat_power->stat.bat_init = 1;

		schedule_delayed_work(&bat_power->bat_chk, msecs_to_jiffies(500));

		return;

	} else if (bat_power->stat.bat_init == 1) {

		bat_power->stat.bat_init = 2;

		ret = regmap_read(regmap, AXP2101_COMM_STAT0, &data[0]);
		if (data[0] & BIT(5)) {
			/* reset_mcu */
			ret = axp2101_reset_mcu(regmap);
			if (ret < 0)
				goto err_read;

			msleep(10);
		}

		WRITE_ONCE(bat_power->stat.bat_read, true);

		ret = regmap_read(regmap, AXP2101_MODULE_EN, &data[0]);
		if (ret < 0)
			goto err_read;

		data[0] |= BIT(1);
		ret = regmap_write(regmap, AXP2101_MODULE_EN, data[0]);
		if (ret < 0)
			goto err_read;

		schedule_delayed_work(&bat_power->bat_chk, msecs_to_jiffies(500));

		return;
	}

	/*
	 * check_full of batt, because bug of axp2101b, full flag can generate
	 * in batt plugged out
	 */
	ret = regmap_read(regmap, AXP2101_COMM_STAT1, &data[0]);
	if (ret < 0) {
		pr_info("read AXP210X_REG_VBAT error\n");
		goto err_read;
	}
	/* chg_stat is full with bit[2:0] is b100 */
	if ((data[0] & 0x07) == 0x04) {
		battery_set_full(&bat_power->stat.bat_full);
	} else {
		WRITE_ONCE(bat_power->stat.bat_full, false);
	}

	ret = regmap_bulk_read(regmap, AXP2101_VBAT_H, data, 2);
	if (ret < 0) {
		pr_info("read AXP210X_REG_VBAT error\n");
		goto err_read;
	}

	rst[cnt] = (((data[0] & GENMASK(5, 0)) << 0x08) | (data[1]));
	if (rst[cnt] < AXP210X_VBAT_MIN)
		rst[cnt] = 0;
	cnt = ++cnt == 3 ? 0 : cnt;

	/* there's one zero to indicate battery disconnect */
	if (!rst[0] || !rst[1] || !rst[2]) {
		if (bat_power->stat.bat_stat != 0) {
			pr_debug("bat_stat change to none\n");
			bat_power->stat.bat_stat = 0;
			WRITE_ONCE(bat_power->stat.bat_read, false);
			axp2101_reset_gauge(regmap);
			s_chg = ktime_get();
			regmap_update_bits(bat_power->regmap, AXP2101_CHGLED_CFG, BIT(0), 0);
			power_supply_changed(bat_power->bat_supply);
		}
	} else {
		if (bat_power->stat.bat_stat != 1) {
			pr_debug("bat_stat change to exist\n");
			bat_power->stat.bat_stat = 1;
			axp2101_reset_gauge(regmap);
			s_chg = ktime_get();
			regmap_update_bits(bat_power->regmap, AXP2101_CHGLED_CFG, BIT(0), BIT(0));
			power_supply_changed(bat_power->bat_supply);
		}
	}

	if (bat_power->stat.bat_stat &&
	    ktime_ms_delta(ktime_get(), s_chg) > MSEC_PER_SEC) {
		WRITE_ONCE(bat_power->stat.bat_read, true);
	}

err_read:
	schedule_delayed_work(&bat_power->bat_chk, msecs_to_jiffies(500));
}

static void axp2101_power_monitor(struct work_struct *work)
{
	struct axp2101_bat_power *bat_power =
		container_of(work, typeof(*bat_power), bat_supply_mon.work);
	power_supply_changed(bat_power->bat_supply);

	if (bat_power->stat.bat_stat) {
		if (bat_power->regcache.soc == 100) {
			regmap_update_bits(bat_power->regmap, AXP2101_MODULE_EN, BIT(1), 0);
		} else {
			regmap_update_bits(bat_power->regmap, AXP2101_MODULE_EN, BIT(1), BIT(1));
		}
	}

	schedule_delayed_work(&bat_power->bat_supply_mon, msecs_to_jiffies(10 * 1000));
}

/**
 * axp2101_get_param - get battery config from dts
 *
 * is not get battery config parameter from dts,
 * then it use the default config.
 */
static int axp2101_get_param(void *data)
{
	struct axp2101_bat_power *bat_power = data;
	struct device_node *n_para, *r_para;
	const char *pparam;
	int cnt;
	void *pres;

	pres = devm_kmalloc(bat_power->dev, AXP210X_MAX_PARAM, GFP_KERNEL);
	if (!pres)
		return -ENOMEM;

	n_para = of_parse_phandle(bat_power->dev->of_node, "param", 0);
	if (!n_para)
		goto e_n_para;

	if (of_property_read_string(n_para, "select", &pparam))
		goto e_para;

	r_para = of_get_child_by_name(n_para, pparam);
	if (!r_para)
		goto e_para;

	cnt = of_property_read_variable_u8_array(r_para, "parameter", pres, 1,
						 AXP210X_MAX_PARAM);
	if (cnt <= 0)
		goto e_n_parameter;

	bat_power->data.model = pres;
	bat_power->data.model_size = cnt;

	of_node_put(r_para);
	of_node_put(n_para);

	return 0;

e_n_parameter:
	of_node_put(r_para);
e_para:
	of_node_put(n_para);
e_n_para:
	devm_kfree(bat_power->dev, pres);

	return -ENODATA;
}

static ssize_t charger_show(struct device *dev, struct device_attribute *attr,
			    char *buf)
{
	struct power_supply *ps = dev_get_drvdata(dev);
	struct axp2101_bat_power *bat_power = power_supply_get_drvdata(ps);

	return sprintf(buf, "%d\n", !bat_power->stat.charger_disable);
}

static ssize_t charger_store(struct device *dev, struct device_attribute *attr,
			     const char *buf, size_t count)
{
	struct power_supply *ps = dev_get_drvdata(dev);
	struct axp2101_bat_power *bat_power = power_supply_get_drvdata(ps);
	long val = 0;
	ssize_t rc;

	rc = kstrtol(buf, 0, &val);
	if (rc)
		return rc;

	if (val) {
		/* enable */
		bat_power->stat.charger_disable = 0;
		axp2101_icchg_set(bat_power, bat_power->dts_info.pmu_runtime_chgcur);
	} else {
		/* disable */
		bat_power->stat.charger_disable = 1;
		axp2101_icchg_set(bat_power, 0);
	}

	return count;
}

DEVICE_ATTR_RW(charger);

static int axp2101_chip_id(void *data)
{
	struct axp2101_bat_power *bat_power = data;
	struct regmap *regmap = bat_power->regmap;
	unsigned int reg_value;
	int ret;

	ret = regmap_read(regmap, AXP2101_CHIP_ID, &reg_value);
	if (ret < 0) {
		pr_info("read AXP2101_CHIP_ID error\n");
		return -ENODEV;
	}

	reg_value &= 0xcf;
	if (reg_value == 0x47 || reg_value == 0x4a) {
		pr_info("read chipid = %x\n", reg_value);
		if (reg_value == 0x4a)
			bat_power->version = 1;
		else
			bat_power->version = 0;

		return 0;
	}

	return -ENODEV;
}

static int axp2101_battery_probe(struct platform_device *pdev)
{
	int ret = 0;
	int i = 0, irq;
	struct axp2101_bat_power *bat_power;
	struct power_supply_config psy_cfg = {};
	struct axp20x_dev *axp_dev = dev_get_drvdata(pdev->dev.parent);
	struct device_node *node = pdev->dev.of_node;

	if (!of_device_is_available(node)) {
		pr_err("axp2101-battery device is not configed\n");
		return -ENODEV;
	}

	if (!axp_dev->irq) {
		pr_err("can not register axp2101-charger without irq\n");
		return -EINVAL;
	}

	bat_power = devm_kzalloc(&pdev->dev, sizeof(*bat_power), GFP_KERNEL);
	if (bat_power == NULL) {
		axp210x_err("axp2101_bat_power alloc failed\n");
		ret = -ENOMEM;
		goto err;
	}

	bat_power->name = "battery";
	bat_power->dev = &pdev->dev;
	bat_power->data = axp2101_model_data;
	bat_power->regmap = axp_dev->regmap;
	bat_power->stat.bat_stat = true;

	bat_power->stat.bat_init = 2;

	/* for device tree parse */
	axp2101_bat_parse_device_tree(bat_power);
	/* get param frome device tree */
	axp2101_get_param(bat_power);

	ret = axp2101_chip_id(bat_power);
	if (ret < 0)
		goto err;

	ret = axp2101_init_chip(bat_power);
	if (ret < 0) {
		axp210x_err("axp210x init chip fail!\n");
		ret = -ENODEV;
		goto err;
	}

	psy_cfg.of_node = pdev->dev.of_node;
	psy_cfg.drv_data = bat_power;

	bat_power->bat_supply = devm_power_supply_register(bat_power->dev,
		&axp2101_bat_desc, &psy_cfg);

	if (ret < 0) {
		axp210x_err("axp210x register battery dev fail!\n");
		goto err;
	}

	device_create_file(&bat_power->bat_supply->dev, &dev_attr_charger);

	if (bat_power->version == 1)
		INIT_DELAYED_WORK(&bat_power->bat_chk, battery_chk_online_v1);
	else
		INIT_DELAYED_WORK(&bat_power->bat_chk, battery_chk_online);

	INIT_DELAYED_WORK(&bat_power->bat_supply_mon, axp2101_power_monitor);
#if ((defined DONOT_Correction) || (defined POLL_READ))
	bat_power->poll_read = kthread_run(thread_dosomthing, bat_power, "axp2101");

#else
	printk("%s: L%d\n", __func__, __LINE__);
	for (i = 0; i < ARRAY_SIZE(axp_bat_irq); i++) {
		printk("ARRAY_SIZE(axp_bat_irq); i++:%d\n", i);
		irq = platform_get_irq_byname(pdev, axp_bat_irq[i].name);
		if (irq < 0)
			continue;

		irq = regmap_irq_get_virq(axp_dev->regmap_irqc, irq);
		if (irq < 0) {
			dev_err(&pdev->dev, "can not get irq\n");
			return irq;
		}
		/* we use this variable to suspend irq */
		axp_bat_irq[i].irq = irq;
		ret = devm_request_any_context_irq(&pdev->dev, irq,
						   axp_bat_irq[i].isr, 0,
						   axp_bat_irq[i].name, bat_power);
		if (ret < 0) {
			dev_err(&pdev->dev, "failed to request %s IRQ %d: %d\n",
				axp_bat_irq[i].name, irq, ret);
			return ret;
		} else {
			ret = 0;
		}

		dev_dbg(&pdev->dev, "Requested %s IRQ %d: %d\n",
			axp_bat_irq[i].name, irq, ret);
	}
#endif
	platform_set_drvdata(pdev, bat_power);

	schedule_delayed_work(&bat_power->bat_chk, msecs_to_jiffies(500));
	schedule_delayed_work(&bat_power->bat_supply_mon, msecs_to_jiffies(10 * 1000));

	return ret;

#if ((defined DONOT_Correction) || (defined POLL_READ))
	kthread_stop(bat_power->poll_read);
#endif

err:
	axp210x_err("%s,probe fail, ret = %d\n", __func__, ret);

	return ret;
}

static int axp2101_battery_remove(struct platform_device *pdev)
{
	struct axp2101_bat_power *bat_power = platform_get_drvdata(pdev);
	axp210x_alway("==============AXP2101 unegister==============\n");
#if ((defined DONOT_Correction) || (defined POLL_READ))
	kthread_stop(bat_power->poll_read);
#endif
	axp2101_teardown_battery(bat_power);
	axp210x_debug("axp210x teardown battery dev\n");

	return 0;
}

static inline void axp2101_irq_set(unsigned int irq, bool enable)
{
	if (enable)
		enable_irq(irq);
	else
		disable_irq(irq);
}

static void axp2101_virq_dts_set(void *data, bool enable)
{
	struct axp2101_bat_power *bat_power = data;
	struct axp_config_info *dts_info = &bat_power->dts_info;

	if (!dts_info->wakeup_bat_in)
		axp2101_irq_set(axp_bat_irq[AXP2101_VIRQ_BAT_IN].irq,
				enable);
	if (!dts_info->wakeup_bat_out)
		axp2101_irq_set(axp_bat_irq[AXP2101_VIRQ_BAT_OUT].irq,
				enable);
	if (!dts_info->wakeup_bat_charging)
		axp2101_irq_set(axp_bat_irq[AXP2101_VIRQ_CHARGING].irq,
				enable);
	if (!dts_info->wakeup_bat_charge_over)
		axp2101_irq_set(axp_bat_irq[AXP2101_VIRQ_CHARGE_OVER].irq,
				enable);
	if (!dts_info->wakeup_low_warning1)
		axp2101_irq_set(axp_bat_irq[AXP2101_VIRQ_LOW_WARNING1].irq,
				enable);
	if (!dts_info->wakeup_low_warning2)
		axp2101_irq_set(axp_bat_irq[AXP2101_VIRQ_LOW_WARNING2].irq,
				enable);
	if (!dts_info->wakeup_bat_untemp_work)
		axp2101_irq_set(
			axp_bat_irq[AXP2101_VIRQ_BAT_UNTEMP_WORK].irq,
			enable);
	if (!dts_info->wakeup_bat_ovtemp_work)
		axp2101_irq_set(
			axp_bat_irq[AXP2101_VIRQ_BAT_OVTEMP_WORK].irq,
			enable);
	if (!dts_info->wakeup_untemp_chg)
		axp2101_irq_set(
			axp_bat_irq[AXP2101_VIRQ_BAT_UNTEMP_CHG].irq,
			enable);
	if (!dts_info->wakeup_ovtemp_chg)
		axp2101_irq_set(
			axp_bat_irq[AXP2101_VIRQ_BAT_OVTEMP_CHG].irq,
			enable);
}

static void axp2101_bat_shutdown(struct platform_device *p)
{
	struct axp2101_bat_power *bat_power = platform_get_drvdata(p);

	/*
	 * for reduce shutdown current
	 */
	regmap_write(bat_power->regmap, AXP2101_VBAT_H, 0);

	cancel_delayed_work_sync(&bat_power->bat_chk);
	cancel_delayed_work_sync(&bat_power->bat_supply_mon);

	axp2101_icchg_set(bat_power, bat_power->dts_info.pmu_shutdown_chgcur);
}

static int axp2101_bat_suspend(struct platform_device *p, pm_message_t state)
{
	struct axp2101_bat_power *bat_power = platform_get_drvdata(p);

	if (!bat_power->stat.charger_disable)
		axp2101_icchg_set(bat_power, bat_power->dts_info.pmu_suspend_chgcur);

	cancel_delayed_work_sync(&bat_power->bat_chk);
	cancel_delayed_work_sync(&bat_power->bat_supply_mon);

	axp2101_virq_dts_set(bat_power, false);
	return 0;
}

static int axp2101_bat_resume(struct platform_device *p)
{
	struct axp2101_bat_power *bat_power = platform_get_drvdata(p);

	if (!bat_power->stat.charger_disable)
		axp2101_icchg_set(bat_power, bat_power->dts_info.pmu_runtime_chgcur);
	else
		axp2101_icchg_set(bat_power, 0);

	schedule_delayed_work(&bat_power->bat_chk, 0);
	schedule_delayed_work(&bat_power->bat_supply_mon, 0);

	axp2101_virq_dts_set(bat_power, true);
	return 0;
}

static const struct of_device_id axp2101_bat_power_match[] = {
	{
		.compatible = "x-powers,axp2101-bat-power-supply",
		.data = (void *)AXP2101_ID,
	}, {/* sentinel */}
};
MODULE_DEVICE_TABLE(of, axp2101_bat_power_match);

static struct platform_driver axp2101_bat_power_driver = {
	.driver = {
		.name = "axp2101-bat-power-supply",
		.of_match_table = axp2101_bat_power_match,
	},
	.probe = axp2101_battery_probe,
	.remove = axp2101_battery_remove,
	.shutdown = axp2101_bat_shutdown,
	.suspend = axp2101_bat_suspend,
	.resume = axp2101_bat_resume,
};

module_platform_driver(axp2101_bat_power_driver);

MODULE_AUTHOR("wangxiaoliang <wangxiaoliang@x-powers.com>");
MODULE_DESCRIPTION("axp210x i2c driver");
MODULE_LICENSE("GPL");
MODULE_VERSION("1.0.0");
