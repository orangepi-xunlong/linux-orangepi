#include <linux/kernel.h>
#include <linux/err.h>
#include <linux/init.h>
#include <linux/regmap.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/regulator/driver.h>
#include <linux/regulator/machine.h>
#include <linux/module.h>
#include <linux/power/axp_depend.h>
#if defined(CONFIG_SUNXI_ARISC)
#include <linux/arisc/arisc.h>
#endif
#include "axp-core.h"
#include "axp-regulator.h"

#if defined(CONFIG_AW_AXPDUMMY) && defined(CONFIG_SUNXI_ARISC)
struct ldo_type_name_mapping {
	enum power_voltage_type ldo_type;
	char *ldo_name;
};

static struct ldo_type_name_mapping mapping_list[] = {
	{DUMMY_REGULATOR1, "axpdummy_ldo1"},
	{DUMMY_REGULATOR2, "axpdummy_ldo2"},
	{DUMMY_REGULATOR3, "axpdummy_ldo3"},
	{DUMMY_REGULATOR4, "axpdummy_ldo4"},
	{DUMMY_REGULATOR5, "axpdummy_ldo5"},
	{DUMMY_REGULATOR6, "axpdummy_ldo6"},
};

static enum power_voltage_type get_ldo_type_by_name(const char *name)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(mapping_list); i++) {
		if (!strcmp(mapping_list[i].ldo_name, name))
			break;
	}

	return mapping_list[i].ldo_type;
};
#endif

static struct axp_consumer_supply *consumer_supply_count;

static inline struct axp_dev *to_axp_dev(struct regulator_dev *rdev)
{
	struct device *dev;
	struct axp_dev *adev;

	dev = rdev_get_dev(rdev)->parent->parent;
	adev = dev_get_drvdata(dev);

	return adev;
}

static inline s32 check_range(struct axp_regulator_info *info,
							s32 min_uv, s32 max_uv)
{
	if (min_uv < info->min_uv || min_uv > info->max_uv)
		return -EINVAL;

	return 0;
}

/* AXP common operations */
static s32 axp_set_voltage(struct regulator_dev *rdev,
				s32 min_uv, s32 max_uv, unsigned *selector)
{
	struct axp_regulator_info *info = rdev_get_drvdata(rdev);
	struct axp_regmap *regmap = info->regmap;
	u8 val, mask;
	s32 ret = -1;
	int i, flag = 0;
	struct axp_dev *cur_axp_dev;

	for (i = 0; i < AXP_ONLINE_SUM; i++) {
		cur_axp_dev = get_pmu_cur_dev(i);
		if (cur_axp_dev == NULL)
			continue;

		flag++;
		if (cur_axp_dev->is_dummy) {
#if defined(CONFIG_AW_AXPDUMMY) && defined(CONFIG_SUNXI_ARISC)
			return arisc_pmu_set_voltage(
				get_ldo_type_by_name(rdev->constraints->name),
				min_uv / 1000);
#endif
			return 0;
		} else {
			continue;
		}
	}

	BUG_ON(flag == 0);

	if (check_range(info, min_uv, max_uv)) {
		pr_err("invalid voltage range (%d, %d) uV\n", min_uv, max_uv);
		return -EINVAL;
	}

	if ((info->switch_uv != 0) && (info->step2_uv != 0) &&
		(info->new_level_uv != 0) && (min_uv > info->switch_uv)) {
		val = (info->switch_uv - info->min_uv + info->step1_uv - 1)
				/ info->step1_uv;
		if (min_uv <= info->new_level_uv) {
			val += 1;
		} else {
			val += (min_uv - info->new_level_uv) / info->step2_uv;
			val += 1;
		}
		mask = ((1 << info->vol_nbits) - 1)  << info->vol_shift;
	} else if ((info->switch_uv != 0) && (info->step2_uv != 0)
				&& (min_uv > info->switch_uv)
				&& (info->new_level_uv == 0)) {
		val = (info->switch_uv - info->min_uv + info->step1_uv - 1)
				/ info->step1_uv;
		val += (min_uv - info->switch_uv) / info->step2_uv;
		mask = ((1 << info->vol_nbits) - 1)  << info->vol_shift;
	} else {
		val = (min_uv - info->min_uv + info->step1_uv - 1)
				/ info->step1_uv;
		val <<= info->vol_shift;
		mask = ((1 << info->vol_nbits) - 1)  << info->vol_shift;
	}

	ret = axp_regmap_update(regmap, info->vol_reg, val, mask);
	if (ret)
		return ret;

	if (0 != info->dvm_enable_reg) {
		ret = axp_regmap_read(regmap, info->dvm_enable_reg, &val);
		if (ret) {
			pr_err("read dvm enable reg failed!\n");
			return ret;
		}

		if (val & (0x1<<info->dvm_enable_bit)) {
			if (!info->dvm_finish_flag) {
				udelay(1000);
			} else {
				/* wait dvm status update */
				udelay(100);
				do {
					ret = axp_regmap_read(regmap,
						info->vol_reg, &val);
					if (ret) {
						pr_err("read dvm failed!\n");
						break;
					}
				} while (!(val & (0x1<<7)));
			}
		}
	}

	AXP_DEBUG(AXP_REGU, info->pmu_num, "set %s voltage: %duV\n",
				rdev->constraints->name, min_uv);

	return ret;
}

static s32 axp_get_voltage(struct regulator_dev *rdev)
{
	struct axp_regulator_info *info = rdev_get_drvdata(rdev);
	struct axp_regmap *regmap = info->regmap;
	u8 val, mask;
	s32 ret, switch_val, vol;
	int i, flag = 0;
	struct axp_dev *cur_axp_dev;

	for (i = 0; i < AXP_ONLINE_SUM; i++) {
		cur_axp_dev = get_pmu_cur_dev(i);
		if (cur_axp_dev == NULL)
			continue;

		flag++;
		if (cur_axp_dev->is_dummy) {
#if defined(CONFIG_AW_AXPDUMMY) && defined(CONFIG_SUNXI_ARISC)
			return arisc_pmu_get_voltage(
			get_ldo_type_by_name(rdev->constraints->name)) * 1000;
#endif
			return INT_MAX;
		} else {
			continue;
		}
	}

	BUG_ON(flag == 0);

	ret = axp_regmap_read(regmap, info->vol_reg, &val);
	if (ret)
		return ret;

	mask = ((1 << info->vol_nbits) - 1) << info->vol_shift;
	if (info->step1_uv != 0)
		switch_val = ((info->switch_uv - info->min_uv
			+ info->step1_uv - 1) / info->step1_uv);
	else
		switch_val = 0;

	val = (val & mask) >> info->vol_shift;

	if ((info->switch_uv != 0) && (info->step2_uv != 0) &&
		(val > switch_val) && (info->new_level_uv != 0)) {
		val -= switch_val;
		vol = info->new_level_uv + info->step2_uv * val;
	} else if ((info->switch_uv != 0)
			&& (info->step2_uv != 0)
			&& (val > switch_val)
			&& (info->new_level_uv == 0)) {
		val -= switch_val;
		vol = info->switch_uv + info->step2_uv * val;
	} else {
		vol = info->min_uv + info->step1_uv * val;
	}

	if (vol > info->max_uv)
		vol = info->max_uv;

	return vol;
}

static s32 axp_enable(struct regulator_dev *rdev)
{
	struct axp_regulator_info *info = rdev_get_drvdata(rdev);
	struct axp_regmap *regmap = info->regmap;
	int i, flag = 0;
	struct axp_dev *cur_axp_dev;

	AXP_DEBUG(AXP_REGU, info->pmu_num, "enable %s\n",
				rdev->constraints->name);

	for (i = 0; i < AXP_ONLINE_SUM; i++) {
		cur_axp_dev = get_pmu_cur_dev(i);
		if (cur_axp_dev == NULL)
			continue;

		flag++;
		if (cur_axp_dev->is_dummy) {
#if defined(CONFIG_AW_AXPDUMMY) && defined(CONFIG_SUNXI_ARISC)
			return arisc_pmu_set_voltage_state(
			get_ldo_type_by_name(rdev->constraints->name), 1);
#endif
			return 0;
		} else {
			continue;
		}
	}

	BUG_ON(flag == 0);

	return axp_regmap_update(regmap, rdev->desc->enable_reg,
			info->enable_val, rdev->desc->enable_mask);
}

static s32 axp_disable(struct regulator_dev *rdev)
{
	struct axp_regulator_info *info = rdev_get_drvdata(rdev);
	struct axp_regmap *regmap = info->regmap;
	int i, flag = 0;
	struct axp_dev *cur_axp_dev;

	AXP_DEBUG(AXP_REGU, info->pmu_num, "disable %s\n",
				rdev->constraints->name);

	for (i = 0; i < AXP_ONLINE_SUM; i++) {
		cur_axp_dev = get_pmu_cur_dev(i);
		if (cur_axp_dev == NULL)
			continue;

		flag++;
		if (cur_axp_dev->is_dummy) {
#if defined(CONFIG_AW_AXPDUMMY) && defined(CONFIG_SUNXI_ARISC)
			return arisc_pmu_set_voltage_state(
			get_ldo_type_by_name(rdev->constraints->name), 0);
#endif
			return 0;
		} else {
			continue;
		}
	}

	BUG_ON(flag == 0);

	return axp_regmap_update(regmap, rdev->desc->enable_reg,
			info->disable_val, rdev->desc->enable_mask);
}

static s32 axp_is_enabled(struct regulator_dev *rdev)
{
	struct axp_regulator_info *info = rdev_get_drvdata(rdev);
	struct axp_regmap *regmap = info->regmap;
	u8 reg_val;
	s32 ret;
	int i, flag = 0;
	struct axp_dev *cur_axp_dev;

	for (i = 0; i < AXP_ONLINE_SUM; i++) {
		cur_axp_dev = get_pmu_cur_dev(i);
		if (cur_axp_dev == NULL)
			continue;

		flag++;
		if (cur_axp_dev->is_dummy) {
#if defined(CONFIG_AW_AXPDUMMY) && defined(CONFIG_SUNXI_ARISC)
			return arisc_pmu_get_voltage_state(
			get_ldo_type_by_name(rdev->constraints->name));
#endif
			return INT_MAX;
		} else {
			continue;
		}
	}

	BUG_ON(flag == 0);

	/* rtc is always enabled, can't disable by software */
	if (strstr(rdev->constraints->name, "rtc"))
		return 1;

	ret = axp_regmap_read(regmap, rdev->desc->enable_reg, &reg_val);
	if (ret)
		return -1;

	if ((reg_val & rdev->desc->enable_mask) == info->enable_val)
		return 1;
	else
		return 0;
}

static s32 axp_list_voltage(struct regulator_dev *rdev, unsigned selector)
{
	struct axp_regulator_info *info = rdev_get_drvdata(rdev);
	s32 ret;
	int i, flag = 0;
	struct axp_dev *cur_axp_dev;

	for (i = 0; i < AXP_ONLINE_SUM; i++) {
		cur_axp_dev = get_pmu_cur_dev(i);
		if (cur_axp_dev == NULL)
			continue;

		flag++;
		if (cur_axp_dev->is_dummy)
			return 0;
		else
			continue;
	}

	BUG_ON(flag == 0);

	ret = info->min_uv + info->step1_uv * selector;
	if ((info->switch_uv != 0) && (info->step2_uv != 0) &&
		(ret > info->switch_uv) && (info->new_level_uv != 0)) {
		selector -= ((info->switch_uv-info->min_uv)/info->step1_uv);
		ret = info->new_level_uv + info->step2_uv * selector;
	} else if ((info->switch_uv != 0) && (info->step2_uv != 0)
				&& (ret > info->switch_uv)
				&& (info->new_level_uv == 0)) {
		selector -= ((info->switch_uv-info->min_uv)/info->step1_uv);
		ret = info->switch_uv + info->step2_uv * selector;
	}

	if (ret > info->max_uv)
		return -EINVAL;

	return ret;
}

static s32 axp_enable_time_regulator(struct regulator_dev *rdev)
{
	struct axp_regulator_info *info = rdev_get_drvdata(rdev);
	int i, flag = 0;
	struct axp_dev *cur_axp_dev;

	for (i = 0; i < AXP_ONLINE_SUM; i++) {
		cur_axp_dev = get_pmu_cur_dev(i);
		if (cur_axp_dev == NULL)
			continue;

		flag++;
		if (cur_axp_dev->is_dummy)
			return 0;
		else
			continue;
	}

	BUG_ON(flag == 0);

	/* Per-regulator power on delay from spec */
	if (40 > info->desc.id)
		return 400;
	else
		return 1200;
}

static int axp_set_voltage_sel(struct regulator_dev *rdev, unsigned selector)
{
	struct axp_regulator_info *info = rdev_get_drvdata(rdev);
	struct axp_regmap *regmap = info->regmap;
	u8 mask;

	AXP_DEBUG(AXP_REGU, info->pmu_num, "set %s voltage selector: %u\n",
				rdev->constraints->name, selector);

	selector <<= info->vol_shift;
	mask = ((1 << info->vol_nbits) - 1)  << info->vol_shift;

	return axp_regmap_update(regmap, info->vol_reg, selector, mask);
}

static int axp_get_voltage_sel(struct regulator_dev *rdev)
{
	struct axp_regulator_info *info = rdev_get_drvdata(rdev);
	struct axp_regmap *regmap = info->regmap;
	int ret;
	u8 val, mask;
	mask = ((1 << info->vol_nbits) - 1)  << info->vol_shift;

	ret = axp_regmap_read(regmap, info->vol_reg, &val);
	if (ret != 0)
		return ret;

	val &= mask;
	val >>= info->vol_shift;

	return val;
}

static int axp_list_voltage_sel(struct regulator_dev *rdev, unsigned index)
{
	struct axp_regulator_info *info = rdev_get_drvdata(rdev);
	int ret = -EINVAL;

	if (info->vtable && (index < rdev->desc->n_voltages))
		ret = info->vtable[index] * 1000;

	return ret;
}

static struct regulator_ops axp_ops = {
	.set_voltage         = axp_set_voltage,
	.get_voltage         = axp_get_voltage,
	.list_voltage        = axp_list_voltage,
	.enable              = axp_enable,
	.disable             = axp_disable,
	.is_enabled          = axp_is_enabled,
	.enable_time         = axp_enable_time_regulator,
	.set_suspend_enable  = axp_enable,
	.set_suspend_disable = axp_disable,
};

static struct regulator_ops axp_sel_ops = {
	.set_voltage_sel     = axp_set_voltage_sel,
	.get_voltage_sel     = axp_get_voltage_sel,
	.list_voltage        = axp_list_voltage_sel,
	.enable              = axp_enable,
	.disable             = axp_disable,
	.is_enabled          = axp_is_enabled,
	.enable_time         = axp_enable_time_regulator,
	.set_suspend_enable  = axp_enable,
	.set_suspend_disable = axp_disable,
};

struct regulator_dev *axp_regulator_register(struct device *dev,
					struct axp_regmap *regmap,
					struct regulator_init_data *init_data,
					struct axp_regulator_info *info)
{
	struct regulator_config config = { };
	struct regulator_dev *rdev;

	config.dev = dev;
	config.init_data = init_data;
	info->regmap = regmap;
	info->desc.ops = &axp_ops;
	config.driver_data = info;

	rdev = regulator_register(&info->desc, &config);

	return rdev;
}
EXPORT_SYMBOL(axp_regulator_register);

struct regulator_dev *axp_regulator_sel_register(struct device *dev,
					struct axp_regmap *regmap,
					struct regulator_init_data *init_data,
					struct axp_regulator_info *info)
{
	struct regulator_config config = { };
	struct regulator_dev *rdev;

	config.dev = dev;
	config.init_data = init_data;
	info->regmap = regmap;
	info->desc.ops = &axp_sel_ops;
	config.driver_data = info;

	rdev = regulator_register(&info->desc, &config);

	return rdev;
}
EXPORT_SYMBOL(axp_regulator_sel_register);

void axp_regulator_unregister(struct regulator_dev *rdev)
{
	regulator_unregister(rdev);
}
EXPORT_SYMBOL(axp_regulator_unregister);

static s32 regu_device_tree_do_parse(struct device_node *node,
				struct regulator_init_data *axp_init_data,
				s32 (*get_dep_cb)(const char *))
{
	u32 ldo_count = 0, ldo_index = 0;
	char name[32] = {0};
	s32 num = 0, supply_num = 0, i = 0, j = 0, var = 0;
	struct axp_consumer_supply consumer_supply[20];
	const char *regulator_string = NULL;
	struct regulator_consumer_supply *regu_consumer_supply = NULL;

	if (of_property_read_u32(node, "regulator_count", &ldo_count)) {
		pr_err("%s: axp regu get regulator_count failed", __func__);
		return -ENOENT;
	}

	for (ldo_index = 1; ldo_index <= ldo_count; ldo_index++) {
		sprintf(name, "regulator%d", ldo_index);
		if (of_property_read_string(node,
			(const char *)&name, &regulator_string)) {
			pr_err("node %s get failed!\n", name);
			continue;
		}

		num = sscanf(regulator_string,
		"%s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s",
		consumer_supply[0].supply, consumer_supply[1].supply,
		consumer_supply[2].supply, consumer_supply[3].supply,
		consumer_supply[4].supply, consumer_supply[5].supply,
		consumer_supply[6].supply, consumer_supply[7].supply,
		consumer_supply[8].supply, consumer_supply[9].supply,
		consumer_supply[10].supply, consumer_supply[11].supply,
		consumer_supply[12].supply, consumer_supply[13].supply,
		consumer_supply[14].supply, consumer_supply[15].supply,
		consumer_supply[16].supply, consumer_supply[17].supply,
		consumer_supply[18].supply, consumer_supply[19].supply);

		if (num <= -1) {
			pr_err("parse ldo%d from sysconfig failed\n",
				ldo_index);
			return -1;
		} else {
			if (strcmp(consumer_supply[1].supply, "none")) {
				var = simple_strtoul(
						consumer_supply[1].supply,
						NULL, 10);
				if (var > (ldo_index-1))
					pr_err("supply rely set err\n");
				else
					(*(axp_init_data+(ldo_index-1))).supply_regulator =
					((*(axp_init_data+(var-1))).consumer_supplies)->supply;
			}

			supply_num = num-1;
			(*(axp_init_data+(ldo_index-1))).num_consumer_supplies = supply_num;

			consumer_supply_count = kzalloc(
				sizeof(struct axp_consumer_supply)*supply_num,
				GFP_KERNEL);
			if (!consumer_supply_count) {
				pr_err("%s: request \"\
					\"consumer_supply_count failed\n",
					__func__);
				return -1;
			}

			regu_consumer_supply = kzalloc(
				sizeof(struct regulator_consumer_supply)*supply_num,
				GFP_KERNEL);
			if (!regu_consumer_supply) {
				pr_err("%s: request \"\
					\"regu_consumer_supply failed\n",
					__func__);
				kfree(consumer_supply_count);
				return -1;
			}

			for (i = 0; i < supply_num; i++) {
				if (0 != i)
					j = i + 1;
				else
					j = i;

				strcpy((char *)(consumer_supply_count+i),
					consumer_supply[j].supply);
				(regu_consumer_supply+i)->supply =
					(const char *)(
					(struct axp_consumer_supply *)
					(consumer_supply_count+i)->supply);

				{
					int ret = 0, sys_id_index = 0;

					sys_id_index = axp_check_sys_id(
					(consumer_supply_count+i)->supply);
					if (0 <= sys_id_index) {
						ret = get_ldo_dependence(
						(const char *)
						&(consumer_supply[0].supply),
						sys_id_index, get_dep_cb);
						if (ret < 0)
							pr_err("sys_id %s set dependence failed.\n",
							(consumer_supply_count
							+i)->supply);
					}
				}
			}
			(*(axp_init_data+(ldo_index-1))).consumer_supplies =
					regu_consumer_supply;
		}
	}
	return 0;
}

s32 axp_regulator_dt_parse(struct device_node *node,
				struct regulator_init_data *axp_init_data,
				s32 (*get_dep_cb)(const char *))
{
	s32 ret;

	if (!of_device_is_available(node)) {
		pr_err("%s: failed\n", __func__);
		return -1;
	}

	ret = regu_device_tree_do_parse(node, axp_init_data, get_dep_cb);

	return ret;
}
EXPORT_SYMBOL(axp_regulator_dt_parse);

int axp_get_ldo_count(struct device_node *node, u32 *ldo_count)
{
	if (!of_device_is_available(node)) {
		pr_err("%s: axp regu is disable", __func__);
		return -EPERM;
	}

	if (of_property_read_u32(node, "regulator_count", ldo_count)) {
		pr_err("%s: axp regu get regulator_count failed", __func__);
		return -ENOENT;
	}

	return 0;
}

int axp_mem_regu_init(struct device_node *node,
				axp_mem_data_t *r_list, u32 ldo_count)
{
	u32 ldo_index = 0;
	char name[32] = {0};
	s32 num = 0;
	struct axp_consumer_supply consumer_supply[20];
	const char *regulator_string = NULL;

	for (ldo_index = 1; ldo_index <= ldo_count; ldo_index++) {
		sprintf(name, "regulator%d", ldo_index);
		if (of_property_read_string(node, (const char *)&name,
					&regulator_string)) {
			pr_err("node %s get failed!\n",
				  name);
			continue;
		}

		num = sscanf(regulator_string,
		"%s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s",
		consumer_supply[0].supply, consumer_supply[1].supply,
		consumer_supply[2].supply, consumer_supply[3].supply,
		consumer_supply[4].supply, consumer_supply[5].supply,
		consumer_supply[6].supply, consumer_supply[7].supply,
		consumer_supply[8].supply, consumer_supply[9].supply,
		consumer_supply[10].supply, consumer_supply[11].supply,
		consumer_supply[12].supply, consumer_supply[13].supply,
		consumer_supply[14].supply, consumer_supply[15].supply,
		consumer_supply[16].supply, consumer_supply[17].supply,
		consumer_supply[18].supply, consumer_supply[19].supply);

		if (num <= -1) {
			pr_err("get ldo%d from sysconfig failed\n", ldo_index);
			return -1;
		} else {
			strcpy((char *)((r_list + ldo_index - 1)->id_name),
						consumer_supply[0].supply);
		}
	}

	return 0;
}
