// SPDX-License-Identifier: GPL-2.0-only
/*
 * Ky Generic power domain support.
 *
 * Copyright (c) 2023 KY, Co. Ltd.
 */

#include <linux/io.h>
#include <linux/iopoll.h>
#include <linux/err.h>
#include <linux/mutex.h>
#include <linux/pm_clock.h>
#include <linux/pm_domain.h>
#include <linux/of_address.h>
#include <linux/of_clk.h>
#include <linux/of_platform.h>
#include <linux/clk.h>
#include <linux/regmap.h>
#include <linux/pm_qos.h>
#include <linux/mfd/syscon.h>
#include <linux/spinlock_types.h>
#include <linux/regulator/consumer.h>
#include <dt-bindings/pmu/x1_pmu.h>
#include <linux/syscore_ops.h>
#include "atomic_qos.h"

#define MAX_REGMAP		5
#define MAX_REGULATOR_PER_DOMAIN	5

#define MPMU_REGMAP_INDEX	0
#define APMU_REGMAP_INDEX	1

#define APMU_POWER_STATUS_REG	0xf0
#define MPMU_APCR_PER_REG	0x1098
#define MPMU_AWUCRM_REG		0x104c

#define APMU_AUDIO_CLK_RES_CTRL	0x14c
#define AP_POWER_CTRL_AUDIO_AUTH_OFFSET	28
#define FORCE_AUDIO_POWER_ON_OFFSET	13

/* wakeup set */
/* pmic */
#define WAKEUP_SOURCE_WAKEUP_7	7
/* gpio */
#define WAKEUP_SOURCE_WAKEUP_2	2

/* usb & others */
#define WAKEUP_SOURCE_WAKEUP_5	5
static bool pmu_support_wakeup5 = false;

#define PM_QOS_BLOCK_C1		0x0 /* core wfi */
#define PM_QOS_BLOCK_C2		0x2 /* core power off */
#define PM_QOS_BLOCK_M2		0x6 /* core l2 off */
#define PM_QOS_BLOCK_AXI        0x7 /* d1p */
#define PM_QOS_BLOCK_DDR        12 /* d1 */
#define PM_QOS_BLOCK_UDR_VCTCXO 13 /* d2 */
#define PM_QOS_BLOCK_UDR        14 /* d2pp */
#define PM_QOS_BLOCK_DEFAULT_VALUE	15

#define PM_QOS_AXISDD_OFFSET	31
#define PM_QOS_DDRCORSD_OFFSET	27
#define PM_QOS_APBSD_OFFSET	26
#define PM_QOS_VCTCXOSD_OFFSET	19
#define PM_QOS_STBYEN_OFFSET	13
#define PM_QOS_PE_VOTE_AP_SLPEN_OFFSET	3

#define DEV_PM_QOS_CLK_GATE		1
#define DEV_PM_QOS_REGULATOR_GATE	2
#define DEV_PM_QOS_PM_DOMAIN_GATE	4
#define DEV_PM_QOS_DEFAULT		7

struct ky_pm_domain_param {
	int reg_pwr_ctrl;
	int pm_qos;
	int bit_hw_mode;
	int bit_sleep2;
	int bit_sleep1;
	int bit_isolation;
	int bit_auto_pwr_on;
	int bit_hw_pwr_stat;
	int bit_pwr_stat;
	int use_hw;
};

struct per_device_qos {
	struct notifier_block notifier;
	struct list_head qos_node;
	struct dev_pm_qos_request req;
	int level;
	struct device *dev;
	struct regulator *rgr[MAX_REGULATOR_PER_DOMAIN];
	int rgr_count;
	/**
	 * manageing the cpuidle-qos, should be per-device
	 */
	struct atomic_freq_qos_request qos;

	bool handle_clk;
	bool handle_regulator;
	bool handle_pm_domain;
	bool handle_cpuidle_qos;
};

struct ky_pm_domain {
	struct generic_pm_domain genpd;
	int pm_index;
	struct device *gdev;
	int rgr_count;
	struct regulator *rgr[MAX_REGULATOR_PER_DOMAIN];
	/**
	 * manageing the cpuidle-qos
	 */
	struct ky_pm_domain_param param;

	/**
	 * manageing the device-drivers power qos
	 */
	struct list_head qos_head;
};

struct ky_pmu {
	struct device *dev;
	int number_domains;
	struct genpd_onecell_data genpd_data;
	struct regmap *regmap[MAX_REGMAP];
	struct ky_pm_domain **domains;
	/**
	 * manageing the cpuidle-qos
	 */
	struct notifier_block notifier;
};

static DEFINE_SPINLOCK(ky_apcr_qos_lock);

static unsigned int g_acpr_per;
static struct ky_pmu *gpmu;

static struct atomic_freq_constraints afreq_constraints;

static const struct of_device_id ky_regmap_dt_match[] = {
	{ .compatible = "ky,ky-mpmu", },
	{ .compatible = "ky,ky-apmu", },
};

static int ky_pd_power_off(struct generic_pm_domain *domain)
{
	unsigned int val;
	int loop, ret;
	struct per_device_qos *pos;
	struct ky_pm_domain *spd = container_of(domain, struct ky_pm_domain, genpd);

	if (spd->param.reg_pwr_ctrl == 0)
		return 0;

	/**
	 * if all the devices in this power domain don't want the pm-domain driver taker over
	 * the power-domian' on/off, return directly.
	 */
	list_for_each_entry(pos, &spd->qos_head, qos_node) {
		if (!pos->handle_pm_domain)
			return 0;
	}

	/**
	 * as long as there is one device don't want to on/off this power-domain, just return
	 */
	list_for_each_entry(pos, &spd->qos_head, qos_node) {
		if ((pos->level & DEV_PM_QOS_PM_DOMAIN_GATE) == 0)
			return 0;
	}

	if (!spd->param.use_hw) {
		/* this is the sw type */
		regmap_read(gpmu->regmap[APMU_REGMAP_INDEX], spd->param.reg_pwr_ctrl, &val);
		val &= ~(1 << spd->param.bit_isolation);
		regmap_write(gpmu->regmap[APMU_REGMAP_INDEX], spd->param.reg_pwr_ctrl, val);

		usleep_range(10, 15);

		/* mcu power off */
		regmap_read(gpmu->regmap[APMU_REGMAP_INDEX], spd->param.reg_pwr_ctrl, &val);
		val &= ~((1 << spd->param.bit_sleep1) | (1 << spd->param.bit_sleep2));
		regmap_write(gpmu->regmap[APMU_REGMAP_INDEX], spd->param.reg_pwr_ctrl, val);

		usleep_range(10, 15);

		for (loop = 10000; loop >= 0; --loop) {
			regmap_read(gpmu->regmap[APMU_REGMAP_INDEX], APMU_POWER_STATUS_REG, &val);
			if ((val & (1 << spd->param.bit_pwr_stat)) == 0)
				break;
			usleep_range(4, 6);
		}
	} else {
		/* LCD */
		regmap_read(gpmu->regmap[APMU_REGMAP_INDEX], spd->param.reg_pwr_ctrl, &val);
		val &= ~(1 << spd->param.bit_auto_pwr_on);
		val &= ~(1 << spd->param.bit_hw_mode);
		regmap_write(gpmu->regmap[APMU_REGMAP_INDEX], spd->param.reg_pwr_ctrl, val);

		usleep_range(10, 30);

		for (loop = 10000; loop >= 0; --loop) {
			regmap_read(gpmu->regmap[APMU_REGMAP_INDEX], APMU_POWER_STATUS_REG, &val);
			if ((val & (1 << spd->param.bit_hw_pwr_stat)) == 0)
				break;
			usleep_range(4, 6);
		}
	}

	if (loop < 0) {
		pr_err("power-off domain: %d, error\n", spd->pm_index);
		return -EBUSY;
	}

	/* enable the supply */
	for (loop = 0; loop < spd->rgr_count; ++loop) {
		ret = regulator_disable(spd->rgr[loop]);
		if (ret < 0) {
			pr_err("%s: regulator disable failed\n", __func__);
			return ret;
		}
	}

	return 0;
}

static int ky_pd_power_on(struct generic_pm_domain *domain)
{
	int loop, ret;
	unsigned int val;
	struct per_device_qos *pos;
	struct ky_pm_domain *spd = container_of(domain, struct ky_pm_domain, genpd);

	if (spd->param.reg_pwr_ctrl == 0)
		return 0;

	/**
	 * if all the devices in this power domain don't want the pm-domain driver taker over
	 * the power-domian' on/off, return directly.
	 * */
	list_for_each_entry(pos, &spd->qos_head, qos_node) {
		if (!pos->handle_pm_domain)
			return 0;
	}

	/**
	 * as long as there is one device don't want to on/off this power-domain, just return
	 */
	list_for_each_entry(pos, &spd->qos_head, qos_node) {
		if ((pos->level & DEV_PM_QOS_PM_DOMAIN_GATE) == 0)
			return 0;
	}

	/* enable the supply */
	for (loop = 0; loop < spd->rgr_count; ++loop) {
		ret = regulator_enable(spd->rgr[loop]);
		if (ret < 0) {
			pr_err("%s: regulator disable failed\n", __func__);
			return ret;
		}
	}

	regmap_read(gpmu->regmap[APMU_REGMAP_INDEX], APMU_POWER_STATUS_REG, &val);
	if (val & (1 << spd->param.bit_pwr_stat)) {
		if (spd->pm_index == X1_PMU_LCD_PWR_DOMAIN)
			return 0;

		if (!spd->param.use_hw) {
			/* this is the sw type */
			regmap_read(gpmu->regmap[APMU_REGMAP_INDEX], spd->param.reg_pwr_ctrl, &val);
			val &= ~(1 << spd->param.bit_isolation);
			regmap_write(gpmu->regmap[APMU_REGMAP_INDEX], spd->param.reg_pwr_ctrl, val);

			usleep_range(10, 15);

			/* mcu power off */
			regmap_read(gpmu->regmap[APMU_REGMAP_INDEX], spd->param.reg_pwr_ctrl, &val);
			val &= ~((1 << spd->param.bit_sleep1) | (1 << spd->param.bit_sleep2));
			regmap_write(gpmu->regmap[APMU_REGMAP_INDEX], spd->param.reg_pwr_ctrl, val);

			usleep_range(10, 15);

			for (loop = 10000; loop >= 0; --loop) {
				regmap_read(gpmu->regmap[APMU_REGMAP_INDEX], APMU_POWER_STATUS_REG, &val);
				if ((val & (1 << spd->param.bit_pwr_stat)) == 0)
					break;
				usleep_range(4, 6);
			}
		} else {
			/* LCD */
			regmap_read(gpmu->regmap[APMU_REGMAP_INDEX], spd->param.reg_pwr_ctrl, &val);
			val &= ~(1 << spd->param.bit_auto_pwr_on);
			val &= ~(1 << spd->param.bit_hw_mode);
			regmap_write(gpmu->regmap[APMU_REGMAP_INDEX], spd->param.reg_pwr_ctrl, val);

			usleep_range(10, 30);

			for (loop = 10000; loop >= 0; --loop) {
				regmap_read(gpmu->regmap[APMU_REGMAP_INDEX], APMU_POWER_STATUS_REG, &val);
				if ((val & (1 << spd->param.bit_hw_pwr_stat)) == 0)
					break;
				usleep_range(4, 6);
			}
		}

		if (loop < 0) {
			pr_err("power-off domain: %d, error\n", spd->pm_index);
			return -EBUSY;
		}
	}

	if (!spd->param.use_hw) {
		/* mcu power on */
		regmap_read(gpmu->regmap[APMU_REGMAP_INDEX], spd->param.reg_pwr_ctrl, &val);
		val |= (1 << spd->param.bit_sleep1);
		regmap_write(gpmu->regmap[APMU_REGMAP_INDEX], spd->param.reg_pwr_ctrl, val);

		usleep_range(20, 25);

		regmap_read(gpmu->regmap[APMU_REGMAP_INDEX], spd->param.reg_pwr_ctrl, &val);
		val |= (1 << spd->param.bit_sleep2) | (1 << spd->param.bit_sleep1);
		regmap_write(gpmu->regmap[APMU_REGMAP_INDEX], spd->param.reg_pwr_ctrl, val);

		usleep_range(20, 25);

		/* disable isolation */
		regmap_read(gpmu->regmap[APMU_REGMAP_INDEX], spd->param.reg_pwr_ctrl, &val);
		val |= (1 << spd->param.bit_isolation);
		regmap_write(gpmu->regmap[APMU_REGMAP_INDEX], spd->param.reg_pwr_ctrl, val);

		usleep_range(10, 15);

		for (loop = 10000; loop >= 0; --loop) {
			regmap_read(gpmu->regmap[APMU_REGMAP_INDEX], APMU_POWER_STATUS_REG, &val);
			if (val & (1 << spd->param.bit_pwr_stat))
				break;
			usleep_range(4, 6);
		}
	} else {
		/* LCD */
		regmap_read(gpmu->regmap[APMU_REGMAP_INDEX], spd->param.reg_pwr_ctrl, &val);
		val |= (1 << spd->param.bit_auto_pwr_on);
		val |= (1 << spd->param.bit_hw_mode);
		regmap_write(gpmu->regmap[APMU_REGMAP_INDEX], spd->param.reg_pwr_ctrl, val);

		usleep_range(290, 310);

		for (loop = 10000; loop >= 0; --loop) {
			regmap_read(gpmu->regmap[APMU_REGMAP_INDEX], APMU_POWER_STATUS_REG, &val);
			if (val & (1 << spd->param.bit_hw_pwr_stat))
				break;
			usleep_range(4, 6);
		}
	}

	if (loop < 0) {
		pr_err("power-on domain: %d, error\n", spd->pm_index);
		return -EBUSY;
	}

	return 0;
}

static int ky_handle_level_notfier_call(struct notifier_block *nb, unsigned long action, void *data)
{
	struct per_device_qos *per_qos = container_of(nb, struct per_device_qos, notifier);

	per_qos->level = action;

	return 0;
}

static int ky_pd_attach_dev(struct generic_pm_domain *genpd, struct device *dev)
{
	int err, i = 0, count;
	struct clk *clk;
	struct per_device_qos *per_qos, *pos;
	const char *strings[MAX_REGULATOR_PER_DOMAIN];
	struct ky_pm_domain *spd = container_of(genpd, struct ky_pm_domain, genpd);

	/**
	 * per-device qos set
	 * this feature enable the device drivers to dynamically modify the power
	 * module taken over by PM domain driver
	 */
	per_qos = (struct per_device_qos *)devm_kzalloc(dev, sizeof(struct per_device_qos), GFP_KERNEL);
	if (!per_qos) {
		pr_err(" allocate per device qos error\n");
		return -ENOMEM;
	}

	per_qos->dev = dev;
	INIT_LIST_HEAD(&per_qos->qos_node);
	list_add(&per_qos->qos_node, &spd->qos_head);
	per_qos->notifier.notifier_call = ky_handle_level_notfier_call;

	dev_pm_qos_add_notifier(dev, &per_qos->notifier, DEV_PM_QOS_MAX_FREQUENCY);

	dev_pm_qos_add_request(dev, &per_qos->req, DEV_PM_QOS_MAX_FREQUENCY, DEV_PM_QOS_DEFAULT);

	if (!of_property_read_bool(dev->of_node, "clk,pm-runtime,no-sleep")) {
		err = pm_clk_create(dev);
		if (err) {
			 dev_err(dev, "pm_clk_create failed %d\n", err);
			 return err;
		}

		while ((clk = of_clk_get(dev->of_node, i++)) && !IS_ERR(clk)) {
			err = pm_clk_add_clk(dev, clk);
			if (err) {
				 dev_err(dev, "pm_clk_add_clk failed %d\n", err);
				 clk_put(clk);
				 pm_clk_destroy(dev);
				 return err;
			}
		}

		per_qos->handle_clk = true;
	}

	/* parse the regulator */
	if (!of_property_read_bool(dev->of_node, "regulator,pm-runtime,no-sleep")) {
		count = of_property_count_strings(dev->of_node, "vin-supply-names");
		if (count < 0)
			pr_debug("no vin-suppuly-names found\n");
		else {
			err = of_property_read_string_array(dev->of_node, "vin-supply-names",
				strings, count);
			if (err < 0) {
				pr_info("read string array vin-supplu-names error\n");
				return err;
			}

			for (i = 0; i < count; ++i) {
				per_qos->rgr[i] = devm_regulator_get(dev, strings[i]);
				if (IS_ERR(per_qos->rgr[i])) {
					pr_err("regulator supply %s, get failed\n", strings[i]);
					return PTR_ERR(per_qos->rgr[i]);
				}
			}

			per_qos->rgr_count = count;
		}

		per_qos->handle_regulator = true;
	}

	/* dealing with the cpuidle-qos */
	if (of_property_read_bool(dev->of_node, "cpuidle,pm-runtime,sleep")) {
		atomic_freq_qos_add_request(&afreq_constraints, &per_qos->qos, FREQ_QOS_MAX, PM_QOS_BLOCK_DEFAULT_VALUE);
		per_qos->handle_cpuidle_qos = true;
	}

	if (!of_property_read_bool(dev->of_node, "pwr-domain,pm-runtime,no-sleep"))
		per_qos->handle_pm_domain = true;

	list_for_each_entry(pos, &spd->qos_head, qos_node) {
		if (per_qos->handle_pm_domain != pos->handle_pm_domain) {
			pr_err("all the devices in this power domain must has the same 'pwr-domain,pm-runtime,no-sleep' perporty\n");
			return -EINVAL;
		}
	}

	return 0;
}

static void ky_pd_detach_dev(struct generic_pm_domain *genpd, struct device *dev)
{
	struct per_device_qos *pos;
	struct ky_pm_domain *spd = container_of(genpd, struct ky_pm_domain, genpd);

	list_for_each_entry(pos, &spd->qos_head, qos_node) {
		if (pos->dev == dev)
			break;
	}

	if (pos->handle_clk)
		pm_clk_destroy(dev);

	if (pos->handle_regulator) {
		while (--pos->rgr_count >= 0)
			devm_regulator_put(pos->rgr[pos->rgr_count]);
	}

	if (pos->handle_pm_domain) {
		if (pos->qos.qos)
			atomic_freq_qos_remove_request(&pos->qos);
	}

	dev_pm_qos_remove_request(&pos->req);
	dev_pm_qos_remove_notifier(dev, &pos->notifier, DEV_PM_QOS_MAX_FREQUENCY);
	list_del(&pos->qos_node);
	devm_kfree(dev, pos);
}

static int ky_cpuidle_qos_notfier_call(struct notifier_block *nb, unsigned long action, void *data)
{
	unsigned int apcr_per;
	unsigned int apcr_clear = 0, apcr_set = (1 << PM_QOS_PE_VOTE_AP_SLPEN_OFFSET);

	spin_lock(&ky_apcr_qos_lock);

	switch (action) {
		case PM_QOS_BLOCK_C1:
		case PM_QOS_BLOCK_C2:
		case PM_QOS_BLOCK_M2:
		case PM_QOS_BLOCK_AXI:
			apcr_clear |= (1 << PM_QOS_AXISDD_OFFSET);
			apcr_clear |= (1 << PM_QOS_DDRCORSD_OFFSET);
			apcr_clear |= (1 << PM_QOS_APBSD_OFFSET);
			apcr_clear |= (1 << PM_QOS_VCTCXOSD_OFFSET);
			apcr_clear |= (1 << PM_QOS_STBYEN_OFFSET);
			break;
		case PM_QOS_BLOCK_DDR:
			apcr_set |= (1 << PM_QOS_AXISDD_OFFSET);
			apcr_clear |= (1 << PM_QOS_DDRCORSD_OFFSET);
			apcr_clear |= (1 << PM_QOS_APBSD_OFFSET);
			apcr_clear |= (1 << PM_QOS_VCTCXOSD_OFFSET);
			apcr_clear |= (1 << PM_QOS_STBYEN_OFFSET);
			break;
		case PM_QOS_BLOCK_UDR:
			apcr_clear |= (1 << PM_QOS_STBYEN_OFFSET);
			apcr_set |= (1 << PM_QOS_AXISDD_OFFSET);
			apcr_set |= (1 << PM_QOS_DDRCORSD_OFFSET);
			apcr_set |= (1 << PM_QOS_APBSD_OFFSET);
			apcr_set |= (1 << PM_QOS_VCTCXOSD_OFFSET);
			break;
		case PM_QOS_BLOCK_DEFAULT_VALUE:
			apcr_set |= (1 << PM_QOS_AXISDD_OFFSET);
			apcr_set |= (1 << PM_QOS_DDRCORSD_OFFSET);
			apcr_set |= (1 << PM_QOS_APBSD_OFFSET);
			apcr_set |= (1 << PM_QOS_VCTCXOSD_OFFSET);
			apcr_set |= (1 << PM_QOS_STBYEN_OFFSET);
			break;
		default:
			pr_warn("Invalidate pm qos value\n");
			spin_unlock(&ky_apcr_qos_lock);
	}

	regmap_read(gpmu->regmap[MPMU_REGMAP_INDEX], MPMU_APCR_PER_REG, &apcr_per);
	apcr_per &= ~(apcr_clear);
	apcr_per |= apcr_set;
	regmap_write(gpmu->regmap[MPMU_REGMAP_INDEX], MPMU_APCR_PER_REG, apcr_per);

	regmap_read(gpmu->regmap[MPMU_REGMAP_INDEX], MPMU_APCR_PER_REG, &apcr_per);

	spin_unlock(&ky_apcr_qos_lock);

	return 0;
}

static int ky_genpd_stop(struct device *dev)
{
	int loop, ret;
	struct per_device_qos *pos;
	struct generic_pm_domain *pd = pd_to_genpd(dev->pm_domain);
	struct ky_pm_domain *spd = container_of(pd, struct ky_pm_domain, genpd);

	list_for_each_entry(pos, &spd->qos_head, qos_node) {
		if (pos->dev == dev)
			break;
	}

	/* disable the clk */
	if ((pos->level & DEV_PM_QOS_CLK_GATE) && pos->handle_clk)
		pm_clk_suspend(dev);

	/* dealing with the pm_qos */
	if (pos->handle_cpuidle_qos)
		atomic_freq_qos_update_request(&pos->qos, PM_QOS_BLOCK_DEFAULT_VALUE);

	if (pos->handle_regulator && (pos->level & DEV_PM_QOS_REGULATOR_GATE)) {
		for (loop = 0; loop < pos->rgr_count; ++loop) {
			ret = regulator_disable(pos->rgr[loop]);
			if (ret < 0) {
				pr_err("%s: regulator disable failed\n", __func__);
				return ret;
			}
		}
	}

	return 0;
}

static int ky_genpd_start(struct device *dev)
{
	int loop, ret;
	struct per_device_qos *pos;
	struct generic_pm_domain *pd = pd_to_genpd(dev->pm_domain);
	struct ky_pm_domain *spd = container_of(pd, struct ky_pm_domain, genpd);

	list_for_each_entry(pos, &spd->qos_head, qos_node) {
		if (pos->dev == dev)
			break;
	}

	if (pos->handle_regulator && (pos->level & DEV_PM_QOS_REGULATOR_GATE)) {
		for (loop = 0; loop < pos->rgr_count; ++loop) {
			ret = regulator_enable(pos->rgr[loop]);
			if (ret < 0) {
				pr_err("%s: regulator disable failed\n", __func__);
				return ret;
			}
		}
	}

	/* dealing with the pm_qos */
	if (pos->handle_cpuidle_qos)
		atomic_freq_qos_update_request(&pos->qos, spd->param.pm_qos);

	if ((pos->level & DEV_PM_QOS_CLK_GATE) && pos->handle_clk)
		pm_clk_resume(dev);

	return 0;
}

static int ky_get_pm_domain_parameters(struct device_node *node, struct ky_pm_domain *pd)
{
	int err;

	err = of_property_read_u32(node, "pm_qos", &pd->param.pm_qos);
	if (err) {
		pr_err("%s:%d, failed to retrive the domain pm_qos\n",
				__func__, __LINE__);
		return -EINVAL;
	}

	err = of_property_read_u32(node, "reg_pwr_ctrl", &pd->param.reg_pwr_ctrl);
	err |= of_property_read_u32(node, "bit_hw_mode", &pd->param.bit_hw_mode);
	err |= of_property_read_u32(node, "bit_sleep2", &pd->param.bit_sleep2);
	err |= of_property_read_u32(node, "bit_sleep1", &pd->param.bit_sleep1);
	err |= of_property_read_u32(node, "bit_isolation", &pd->param.bit_isolation);
	err |= of_property_read_u32(node, "bit_auto_pwr_on", &pd->param.bit_auto_pwr_on);
	err |= of_property_read_u32(node, "bit_hw_pwr_stat", &pd->param.bit_hw_pwr_stat);
	err |= of_property_read_u32(node, "bit_pwr_stat", &pd->param.bit_pwr_stat);
	err |= of_property_read_u32(node, "use_hw", &pd->param.use_hw);

	if (err)
		pr_debug("get pm domain parameter failed\n");

	return 0;
}

static int ky_pm_add_one_domain(struct ky_pmu *pmu, struct device_node *node)
{
	int err;
	int id, count, i;
	struct ky_pm_domain *pd;
	const char *strings[MAX_REGULATOR_PER_DOMAIN];

	err = of_property_read_u32(node, "reg", &id);
	if (err) {
		pr_err("%s:%d, failed to retrive the domain id\n",
				__func__, __LINE__);
		return -EINVAL;
	}

	if (id >= pmu->number_domains) {
		pr_err("%pOFn: invalid domain id %d\n", node, id);
		return -EINVAL;
	}

	pd = (struct ky_pm_domain *)devm_kzalloc(pmu->dev, sizeof(struct ky_pm_domain), GFP_KERNEL);
	if (!pd)
		return -ENOMEM;

	pd->pm_index = id;

	/* we will add all the notifiers to this device */
	pd->gdev = pmu->dev;

	err = ky_get_pm_domain_parameters(node, pd);
	if (err)
		return -EINVAL;

	/* get the power supply of the power-domain */
	count = of_property_count_strings(node, "vin-supply-names");
	if (count < 0)
		pr_debug("no vin-suppuly-names found\n");
	else {
		err = of_property_read_string_array(node, "vin-supply-names",
			strings, count);
		if (err < 0) {
			pr_info("read string array vin-supplu-names error\n");
			return err;
		}

		for (i = 0; i < count; ++i) {
			pd->rgr[i] = regulator_get(NULL, strings[i]);
			if (IS_ERR(pd->rgr[i])) {
				pr_err("regulator supply %s, get failed\n", strings[i]);
				return PTR_ERR(pd->rgr[i]);
			}
		}

		pd->rgr_count = count;
	}

	INIT_LIST_HEAD(&pd->qos_head);

	pd->genpd.name = kbasename(node->full_name);
	pd->genpd.power_off = ky_pd_power_off;
	pd->genpd.power_on = ky_pd_power_on;
	pd->genpd.attach_dev = ky_pd_attach_dev;
	pd->genpd.detach_dev = ky_pd_detach_dev;

	pd->genpd.dev_ops.stop = ky_genpd_stop;
	pd->genpd.dev_ops.start = ky_genpd_start;

	/* audio power-domain is power-on by default */
	if (id == X1_PMU_AUD_PWR_DOMAIN) {
		/* set this flag has nothing affect, just do not want the framework disable
		 * the clk & power in noirq callback when system suspend.
		 * */
		pd->genpd.flags |= GENPD_FLAG_ACTIVE_WAKEUP;
		pm_genpd_init(&pd->genpd, NULL, false);
	} else
		pm_genpd_init(&pd->genpd, NULL, true);

	pmu->domains[id] = pd;

	return 0;
}

static void ky_pm_remove_one_domain(struct ky_pm_domain *pd)
{
	int ret;

	ret = pm_genpd_remove(&pd->genpd);
	if (ret < 0) {
		pr_err("failed to remove domain '%s' : %d\n", pd->genpd.name, ret);
	}

	/* devm will free our memory */
}

static int ky_pm_add_subdomain(struct ky_pmu *pmu, struct device_node *parent)
{
	struct device_node *np;
	struct generic_pm_domain *child_domain, *parent_domain;
	int err, idx;

	for_each_child_of_node(parent, np) {
		err = of_property_read_u32(parent, "reg", &idx);
		if (err) {
			pr_err("%pOFn: failed to retrive domain id (reg): %d\n",
					parent, err);
			goto err_out;
		}

		parent_domain = &pmu->domains[idx]->genpd;

		err = ky_pm_add_one_domain(pmu, np);
		if (err) {
			pr_err("failed to handle node %pOFn: %d\n", np, err);
			goto err_out;
		}

		err = of_property_read_u32(np, "reg", &idx);
		if (err) {
			pr_err("%pOFn: failed to retrive domain id (reg): %d\n",
					parent, err);
			goto err_out;
		}

		child_domain = &pmu->domains[idx]->genpd;

		err = pm_genpd_add_subdomain(parent_domain, child_domain);
		if (err) {
			pr_err("%s failed to add subdomain %s: %d\n",
					parent_domain->name, child_domain->name, err);
			goto err_out;
		} else {
			pr_info("%s add subdomain: %s\n",
					parent_domain->name, child_domain->name);
		}

		ky_pm_add_subdomain(pmu, np);
	}

	return 0;

err_out:
	of_node_put(np);
	return err;
}

static void ky_pm_domain_cleanup(struct ky_pmu *pmu)
{
	struct ky_pm_domain *pd;
	int i;

	for (i = 0; i < pmu->number_domains; i++) {
		pd = pmu->domains[i];
		if (pd)
			ky_pm_remove_one_domain(pd);
	}

	/* devm will free our memory */
}

#ifdef CONFIG_PM_SLEEP
static int acpr_per_suspend(void)
{
	unsigned int apcr_per;
	unsigned int apcr_clear = 0, apcr_set = (1 << PM_QOS_PE_VOTE_AP_SLPEN_OFFSET);

	spin_lock(&ky_apcr_qos_lock);

	apcr_set |= (1 << PM_QOS_AXISDD_OFFSET);
	apcr_set |= (1 << PM_QOS_DDRCORSD_OFFSET);
	apcr_set |= (1 << PM_QOS_APBSD_OFFSET);
	apcr_set |= (1 << PM_QOS_VCTCXOSD_OFFSET);
	apcr_set |= (1 << PM_QOS_STBYEN_OFFSET);

	regmap_read(gpmu->regmap[MPMU_REGMAP_INDEX], MPMU_APCR_PER_REG, &apcr_per);
	g_acpr_per = apcr_per;
	apcr_per &= ~(apcr_clear);
	apcr_per |= apcr_set;
	regmap_write(gpmu->regmap[MPMU_REGMAP_INDEX], MPMU_APCR_PER_REG, apcr_per);

	spin_unlock(&ky_apcr_qos_lock);

	/* enable pmic wakeup */
	regmap_read(gpmu->regmap[MPMU_REGMAP_INDEX], MPMU_AWUCRM_REG, &apcr_per);
	apcr_per |= (1 << WAKEUP_SOURCE_WAKEUP_7);
	regmap_write(gpmu->regmap[MPMU_REGMAP_INDEX], MPMU_AWUCRM_REG, apcr_per);

	/* enable pinctrl edge detect wakeup */
	regmap_read(gpmu->regmap[MPMU_REGMAP_INDEX], MPMU_AWUCRM_REG, &apcr_per);
	apcr_per |= (1 << WAKEUP_SOURCE_WAKEUP_2);
	regmap_write(gpmu->regmap[MPMU_REGMAP_INDEX], MPMU_AWUCRM_REG, apcr_per);

	/* enable usb/rcpu/ap2audio */
	if (pmu_support_wakeup5) {
		regmap_read(gpmu->regmap[MPMU_REGMAP_INDEX], MPMU_AWUCRM_REG, &apcr_per);
		apcr_per |= (1 << WAKEUP_SOURCE_WAKEUP_5);
		regmap_write(gpmu->regmap[MPMU_REGMAP_INDEX], MPMU_AWUCRM_REG, apcr_per);
	}

	return 0;
}

static void acpr_per_resume(void)
{
	unsigned int apcr_per;

	spin_lock(&ky_apcr_qos_lock);

	regmap_write(gpmu->regmap[MPMU_REGMAP_INDEX], MPMU_APCR_PER_REG, g_acpr_per);

	spin_unlock(&ky_apcr_qos_lock);

	/* disable pmic wakeup */
	regmap_read(gpmu->regmap[MPMU_REGMAP_INDEX], MPMU_AWUCRM_REG, &apcr_per);
	apcr_per &= ~(1 << WAKEUP_SOURCE_WAKEUP_7);
	regmap_write(gpmu->regmap[MPMU_REGMAP_INDEX], MPMU_AWUCRM_REG, apcr_per);
}

static struct syscore_ops acpr_per_syscore_ops = {
	.suspend = acpr_per_suspend,
	.resume = acpr_per_resume,
};
#endif

static int ky_pm_domain_probe(struct platform_device *pdev)
{
	int err = 0, i;
	struct device *dev = &pdev->dev;
	struct device_node *node;
	struct device_node *np = dev->of_node;
	struct ky_pmu *pmu = NULL;

	pmu = (struct ky_pmu *)devm_kzalloc(dev, sizeof(struct ky_pmu), GFP_KERNEL);
	if (pmu == NULL) {
		pr_err("%s:%d, err\n", __func__, __LINE__);
		return -ENOMEM;
	}

	pmu->dev = dev;

	for (i = 0; i < sizeof(ky_regmap_dt_match) / sizeof(ky_regmap_dt_match[0]); ++i) {
		pmu->regmap[i] = syscon_regmap_lookup_by_compatible(ky_regmap_dt_match[i].compatible);
		if (IS_ERR(pmu->regmap[i])) {
			pr_err("%s:%d err\n", __func__, __LINE__);
			return PTR_ERR(pmu->regmap[i]);
		}
	}

	pmu_support_wakeup5 = of_property_read_bool(np, "pmu_wakeup5");

	/* get number power domains */
	err = of_property_read_u32(np, "domains", &pmu->number_domains);
	if (err) {
		pr_err("%s:%d, failed to retrive the number of domains\n",
				__func__, __LINE__);
		return -EINVAL;
	}

	pmu->domains = devm_kzalloc(dev, sizeof(struct ky_pm_domain *) * pmu->number_domains,
			GFP_KERNEL);
	if (!pmu->domains) {
		pr_err("%s:%d, err\n", __func__, __LINE__);
		return -ENOMEM;
	}

	err = -ENODEV;

	for_each_available_child_of_node(np, node) {
		err = ky_pm_add_one_domain(pmu, node);
		if (err) {
			pr_err("%s:%d, failed to handle node %pOFn: %d\n", __func__, __LINE__,
					node, err);
			of_node_put(node);
			goto err_out;
		}

		err = ky_pm_add_subdomain(pmu, node);
		if (err) {
			pr_err("%s:%d, failed to handle subdomain node %pOFn: %d\n",
					__func__, __LINE__, node, err);
			of_node_put(node);
			goto err_out;
		}
	}

	if(err) {
		pr_err("no power domains defined\n");
		goto err_out;
	}

	pmu->genpd_data.domains = (struct generic_pm_domain **)pmu->domains;
	pmu->genpd_data.num_domains = pmu->number_domains;

	err = of_genpd_add_provider_onecell(np, &pmu->genpd_data);
	if (err) {
		pr_err("failed to add provider: %d\n", err);
		goto err_out;
	}

	/**
	 * dealing with the cpuidle qos
	 */
	pmu->notifier.notifier_call = ky_cpuidle_qos_notfier_call;
	atomic_freq_constraints_init(&afreq_constraints);
	atomic_freq_qos_add_notifier(&afreq_constraints, FREQ_QOS_MAX, &pmu->notifier);

	gpmu = pmu;

#ifdef CONFIG_PM_SLEEP
	register_syscore_ops(&acpr_per_syscore_ops);
#endif
	return 0;

err_out:
	ky_pm_domain_cleanup(pmu);
	return err;
}

static const struct of_device_id ky_pm_domain_dt_match[] = {
	{ .compatible = "ky,power-controller", },
	{ },
};

static struct platform_driver ky_pm_domain_driver = {
	.probe = ky_pm_domain_probe,
	.driver = {
		.name   = "ky-pm-domain",
		.of_match_table = ky_pm_domain_dt_match,
	},
};

static int __init ky_pm_domain_drv_register(void)
{
	return platform_driver_register(&ky_pm_domain_driver);
}
core_initcall(ky_pm_domain_drv_register);
