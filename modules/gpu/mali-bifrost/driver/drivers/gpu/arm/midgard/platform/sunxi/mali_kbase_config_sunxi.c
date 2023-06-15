/*
 * Copyright (C) 2019 Allwinner Technology Limited. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * Author: Albert Yu <yuxyun@allwinnertech.com>
 */

#include <linux/ioport.h>
#include <mali_kbase.h>
#include <mali_kbase_defs.h>
#include <mali_kbase_config.h>
#include "mali_kbase_config_platform.h"

#include "platform.h"
#include <linux/clk.h>
#include <linux/reset.h>
#include <linux/clk-provider.h>
#include <linux/delay.h>

#include <linux/pm_opp.h>
#include <linux/pm_runtime.h>

#if defined(CONFIG_ARCH_SUN50IW9)
#define SMC_REG_BASE 0x3007000
#elif defined(CONFIG_ARCH_SUN50IW12)
#define SMC_REG_BASE 0x04800000
#else
#error "Need configure SMC for GPU"
#endif
#define SMC_GPU_DRM_REG (SMC_REG_BASE + 0x54)

#if defined(CONFIG_ARCH_SUN50IW9)
#define PRCM_REG_BASE 0x07010000
#define GPU_POWEROFF_GATING_REG (PRCM_REG_BASE + 0x0254)
#endif

#define SUNXI_BAK_CLK_RATE 600000000

static struct sunxi_data *sunxi_data;
static struct kbase_device *s_kbdev;
static struct dentry *sunxi_debugfs;

#ifdef CONFIG_DEBUG_FS
static int sunxi_create_debugfs(void);
#endif
void kbase_pm_get_dvfs_metrics(struct kbase_device *kbdev,
			       struct kbasep_pm_metrics *last,
			       struct kbasep_pm_metrics *diff);

static inline void ioremap_regs(void)
{
	struct reg *p_drm;
#if defined(CONFIG_ARCH_SUN50IW9)
	struct reg *p_poweroff_gating;
#endif

	p_drm = &sunxi_data->regs.drm;
	p_drm->phys = SMC_GPU_DRM_REG;
	p_drm->ioaddr = ioremap(p_drm->phys, 4);

#if defined(CONFIG_ARCH_SUN50IW9)
	p_poweroff_gating = &sunxi_data->regs.poweroff_gating;
	p_poweroff_gating->phys = GPU_POWEROFF_GATING_REG;
	p_poweroff_gating->ioaddr = ioremap(p_poweroff_gating->phys, 4);
#endif
}

#if defined(CONFIG_ARCH_SUN50IW9)
static void power_gating(bool gate)
{
	u32 val;

	val = readl(sunxi_data->regs.poweroff_gating.ioaddr);
	if (gate)
		val |= gate;
	else
		val &= 0xfffffffe;
	writel(val, sunxi_data->regs.poweroff_gating.ioaddr);

}
#endif

static inline void iounmap_regs(void)
{
	iounmap(sunxi_data->regs.drm.ioaddr);
#if defined(CONFIG_ARCH_SUN50IW9)
	iounmap(sunxi_data->regs.poweroff_gating.ioaddr);
#endif
}

static void enable_clks_wrap(struct kbase_device *kbdev)
{
	mutex_lock(&sunxi_data->sunxi_lock);
	clk_prepare_enable(kbdev->clocks[1]);
	mutex_unlock(&sunxi_data->sunxi_lock);
}

static void disable_clks_wrap(struct kbase_device *kbdev)
{
	mutex_lock(&sunxi_data->sunxi_lock);
	clk_disable_unprepare(kbdev->clocks[1]);
	mutex_unlock(&sunxi_data->sunxi_lock);
}

int sunxi_chang_freq_safe(struct kbase_device *kbdev, unsigned long *freq, unsigned long u_volt)
{
	int ret = 0;
#if defined(CONFIG_ARCH_SUN50IW9)
	unsigned long s_u_volt = u_volt;
	unsigned long c_u_volt = 0;
	unsigned long c_u_freq = 0;

	c_u_freq = sunxi_data->current_freq;
	c_u_volt = sunxi_data->current_u_volt;

	if (IS_ERR_OR_NULL(sunxi_data->bak_clk)) {
		*freq = c_u_freq;
		return -EINVAL;
	}

	if (*freq <= sunxi_data->bak_freq) {
		s_u_volt = c_u_volt;
		if (c_u_freq < sunxi_data->bak_freq) {
			s_u_volt = sunxi_data->bak_u_volt;
		}
	} else {
		if (*freq <= c_u_freq)
			s_u_volt = c_u_volt;
	}

	if (sunxi_data->independent_power && s_u_volt != c_u_volt) {
#ifdef CONFIG_REGULATOR
		ret = regulator_set_voltage(kbdev->regulators[0], s_u_volt, s_u_volt);
		if (ret < 0) {
			dev_err(kbdev->dev, "set gpu regulators err %d!\n", ret);
			*freq = c_u_freq;
			u_volt = c_u_volt;
			goto ret_safe;
		}
#endif
		c_u_volt = s_u_volt;
	}

	ret = clk_set_parent(kbdev->clocks[1], sunxi_data->bak_clk);
	if (ret < 0) {
		dev_err(kbdev->dev, "set gpu clock err %d!\n", ret);
		*freq = sunxi_data->current_freq;
		u_volt = c_u_volt;
		goto ret_safe;
	}
	ret = clk_set_rate(kbdev->clocks[0], *freq);
	if (ret < 0) {
		*freq = sunxi_data->bak_freq;
		u_volt = c_u_volt;
		goto ret_safe;
	}
	ret = clk_set_parent(kbdev->clocks[1], kbdev->clocks[0]);
	if (ret < 0) {
		clk_set_parent(kbdev->clocks[1], sunxi_data->bak_clk);
		*freq = sunxi_data->bak_freq;
		u_volt = c_u_volt;
		goto ret_safe;
	}
	clk_set_rate(kbdev->clocks[1], *freq);

	if (u_volt != c_u_volt) {
		if (sunxi_data->independent_power) {
#ifdef CONFIG_REGULATOR
			ret = regulator_set_voltage(kbdev->regulators[0], u_volt, u_volt);
			if (ret < 0) {
				dev_err(kbdev->dev, "set gpu 2 regulators err %d!\n", ret);
				ret = 0;
				u_volt = c_u_volt;
			}
#endif
		}
	}

ret_safe:
#ifdef CONFIG_MALI_DEVFREQ
	kbdev->current_voltages[0] = u_volt;
#endif
	dev_dbg(kbdev->dev, "sunxi set gpu form [%ld,%ld] to [%ld,%ld]\n",
			sunxi_data->current_u_volt, sunxi_data->current_freq, u_volt, *freq);

	sunxi_data->current_freq = *freq;
	sunxi_data->current_u_volt = u_volt;

#elif defined(CONFIG_ARCH_SUN50IW12)
	if (sunxi_data->current_freq < *freq) {
		// increase voltage first when up frequency
		if (sunxi_data->independent_power && u_volt != sunxi_data->current_u_volt) {
#ifdef CONFIG_REGULATOR
			ret = regulator_set_voltage(kbdev->regulators[0], u_volt, u_volt);
			if (ret < 0) {
				dev_err(kbdev->dev, "set gpu regulators err %d!\n", ret);
				return ret;
			}
#endif
			sunxi_data->current_u_volt = u_volt;
#ifdef CONFIG_MALI_DEVFREQ
			kbdev->current_voltages[0] = u_volt;
#endif
		}
		// then set frequnecy
		ret = clk_set_rate(kbdev->clocks[0], *freq);
		if (ret < 0) {
			dev_err(kbdev->dev, "set pll_gpu to %ld err %d!\n", *freq, ret);
			return ret;
		}
		ret = clk_set_rate(kbdev->clocks[1], *freq);
		if (ret < 0) {
			dev_err(kbdev->dev, "set gpu core clock to %ld err %d!\n", *freq, ret);
			return ret;
		}
		sunxi_data->current_freq = *freq;
	} else {
		// decrease frequency first when down frequency
		ret = clk_set_rate(kbdev->clocks[0], *freq);
		if (ret < 0) {
			dev_err(kbdev->dev, "set pll_gpu to %ld err %d!\n", *freq, ret);
			return ret;
		}
		ret = clk_set_rate(kbdev->clocks[1], *freq);
		if (ret < 0) {
			dev_err(kbdev->dev, "set gpu core clock to %ld err %d!\n", *freq, ret);
			return ret;
		}
		sunxi_data->current_freq = *freq;
		// decrease voltage
		if (sunxi_data->independent_power && u_volt != sunxi_data->current_u_volt) {
#ifdef CONFIG_REGULATOR
			ret = regulator_set_voltage(kbdev->regulators[0], u_volt, u_volt);
			if (ret < 0) {
				dev_err(kbdev->dev, "set gpu regulators err %d!\n", ret);
				return ret;
			}
#endif
			sunxi_data->current_u_volt = u_volt;
#ifdef CONFIG_MALI_DEVFREQ
			kbdev->current_voltages[0] = u_volt;
#endif
		}
	}
#endif

	return ret;
}

#ifdef CONFIG_MALI_DEVFREQ
int sunxi_dvfs_target(struct kbase_device *kbdev, unsigned long *freq, unsigned long u_volt)
{
	mutex_lock(&sunxi_data->sunxi_lock);
	if (sunxi_data->man_ctrl || !sunxi_data->dvfs_ctrl || sunxi_data->sence_ctrl) {
		*freq = kbdev->current_nominal_freq;
		mutex_unlock(&sunxi_data->sunxi_lock);
		return -ENODEV;
	}

	sunxi_chang_freq_safe(kbdev, freq, u_volt);
	mutex_unlock(&sunxi_data->sunxi_lock);

	return 0;
}
#endif

static int parse_dts_and_fex(struct kbase_device *kbdev, struct sunxi_data *sunxi_data)
{
#ifdef CONFIG_OF
	u32 val;
	int err;
	err = of_property_read_u32(kbdev->dev->of_node, "gpu_idle", &val);
	if (!err)
		sunxi_data->idle_ctrl = val ? true : false;
	err = of_property_read_u32(kbdev->dev->of_node, "independent_power", &val);
	if (!err)
		sunxi_data->independent_power = val ? true : false;
	err = of_property_read_u32(kbdev->dev->of_node, "dvfs_status", &val);
	if (!err)
		sunxi_data->dvfs_ctrl = val ? true : false;
#endif /* CONFIG_OF */

	return 0;
}

static ssize_t scene_ctrl_cmd_show(struct device *dev,
				struct device_attribute *attr,
				char *buf)
{
	return sprintf(buf, "%d\n", sunxi_data->sence_ctrl);
}

static ssize_t scene_ctrl_cmd_store(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count)
{
	int err;
	unsigned long val;
	unsigned long volt;
	enum scene_ctrl_cmd cmd;
#if defined(CONFIG_PM_OPP)
	struct dev_pm_opp *opp;
#endif
	err = kstrtoul(buf, 10, &val);
	if (err) {
		dev_err(dev, "scene_ctrl_cmd_store gets a invalid parameter!\n");
		return count;
	}

	cmd = (enum scene_ctrl_cmd)val;
	switch (cmd) {
	case SCENE_CTRL_NORMAL_MODE:
		sunxi_data->sence_ctrl = 0;
		break;

	case SCENE_CTRL_PERFORMANCE_MODE:
		sunxi_data->sence_ctrl = 1;
		val = sunxi_data->max_freq;
		volt = sunxi_data->max_u_volt;
#if defined(CONFIG_PM_OPP)
		rcu_read_lock();
		opp = dev_pm_opp_find_freq_floor(dev, &val);
		if (!IS_ERR_OR_NULL(opp)) {
			volt = dev_pm_opp_get_voltage(opp);
		} else {
			val = sunxi_data->max_freq;
			volt = sunxi_data->max_u_volt;
		}
		rcu_read_unlock();
#endif
		mutex_lock(&sunxi_data->sunxi_lock);
		sunxi_chang_freq_safe(s_kbdev, &val, volt);
		mutex_unlock(&sunxi_data->sunxi_lock);
#ifdef CONFIG_MALI_DEVFREQ
		s_kbdev->current_nominal_freq = val;
#endif
		break;

	default:
		dev_err(dev, "invalid scene control command %d!\n", cmd);
		return count;
	}

	return count;
}

static DEVICE_ATTR(command, 0660,
		scene_ctrl_cmd_show, scene_ctrl_cmd_store);

static struct attribute *scene_ctrl_attributes[] = {
	&dev_attr_command.attr,
	NULL
};

static struct attribute_group scene_ctrl_attribute_group = {
	.name = "scenectrl",
	.attrs = scene_ctrl_attributes
};

int sunxi_platform_init(struct kbase_device *kbdev)
{
#if defined(CONFIG_PM_OPP)
	struct dev_pm_opp *opp;
#endif

	unsigned long freq = SUNXI_BAK_CLK_RATE;
	unsigned long u_volt = 950000;

	sunxi_data = (struct sunxi_data *)kzalloc(sizeof(struct sunxi_data), GFP_KERNEL);
	if (IS_ERR_OR_NULL(sunxi_data)) {
		dev_err(kbdev->dev, "sunxi init gpu Failed to malloc memory.\n");
		return -ENOMEM;
	}
	sunxi_data->dvfs_ctrl = true;
	sunxi_data->idle_ctrl = true;
	sunxi_data->independent_power = false;
	sunxi_data->power_on = false;
	sunxi_data->clk_on = false;
	parse_dts_and_fex(kbdev, sunxi_data);
#ifdef CONFIG_REGULATOR
	if (IS_ERR_OR_NULL(kbdev->regulators[0])) {
		sunxi_data->independent_power = 0;
	}
#else
	sunxi_data->independent_power = 0;
#endif

#if defined(CONFIG_ARCH_SUN50IW9)
	sunxi_data->bak_freq = SUNXI_BAK_CLK_RATE;
	sunxi_data->bak_u_volt = u_volt;
#endif

#if !defined(CONFIG_PM_OPP)
	clk_set_rate(kbdev->clocks[0], SUNXI_BAK_CLK_RATE);
	clk_set_rate(kbdev->clocks[1], SUNXI_BAK_CLK_RATE);
#else
	freq = ULONG_MAX;
	rcu_read_lock();
	opp = dev_pm_opp_find_freq_floor(kbdev->dev, &freq);
	if (IS_ERR_OR_NULL(opp)) {
		dev_err(kbdev->dev, "sunxi init gpu Failed to get opp (%ld)\n", PTR_ERR(opp));
		freq = SUNXI_BAK_CLK_RATE;
	} else {
		u_volt = dev_pm_opp_get_voltage(opp);
	}
	rcu_read_unlock();
#if defined(CONFIG_ARCH_SUN50IW9)
	if (SUNXI_BAK_CLK_RATE > freq) {
		sunxi_data->bak_freq = SUNXI_BAK_CLK_RATE/2;
		sunxi_data->bak_u_volt = u_volt;
	}
#endif
#ifdef CONFIG_REGULATOR
	if (sunxi_data->independent_power) {
		if (regulator_set_voltage(kbdev->regulators[0], u_volt, u_volt) < 0)
			dev_err(kbdev->dev, "sunxi init set gpu voltage err.\n");
	}
	if (kbdev->regulators[0])
		u_volt = regulator_get_voltage(kbdev->regulators[0]);
#endif
	clk_set_rate(kbdev->clocks[0], freq);
	clk_set_rate(kbdev->clocks[1], freq);
#endif

	sunxi_data->reset = devm_reset_control_get(kbdev->dev, NULL);
	if (IS_ERR_OR_NULL(sunxi_data->reset)) {
		dev_info(kbdev->dev, "sunxi init gpu Failed to get reset ctrl\n");
	}

#if defined(CONFIG_ARCH_SUN50IW9)
	sunxi_data->bus_clk = of_clk_get(kbdev->dev->of_node, 3);
#elif defined(CONFIG_ARCH_SUN50IW12)
	sunxi_data->bus_clk = of_clk_get(kbdev->dev->of_node, 2);
#endif
	if (!IS_ERR_OR_NULL(sunxi_data->bus_clk)) {
		clk_prepare_enable(sunxi_data->bus_clk);
	} else {
		dev_info(kbdev->dev, "sunxi init gpu Failed to get bus_clk \n");
	}

#if defined(CONFIG_ARCH_SUN50IW9)
	sunxi_data->bak_clk = of_clk_get(kbdev->dev->of_node, 2);
	if (!IS_ERR_OR_NULL(sunxi_data->bak_clk)) {
		clk_set_rate(sunxi_data->bak_clk, sunxi_data->bak_freq);
		clk_prepare_enable(sunxi_data->bak_clk);
	} else {
		dev_info(kbdev->dev, "sunxi init gpu Failed to get bakclk \n");
		sunxi_data->bak_u_volt = 0;
		sunxi_data->bak_freq = 0;
	}
#endif

	sunxi_data->max_freq = freq;
	sunxi_data->max_u_volt = u_volt;
	ioremap_regs();
	mutex_init(&sunxi_data->sunxi_lock);

#if defined(CONFIG_ARCH_SUN50IW12)
	// 1860 need use pm_runtime framework to let
	// power domain control poweron or poweroff of gpu
	pm_runtime_enable(kbdev->dev);
	// When use 1860 power domain, you need enable clk and
	// deassert gpu reset in gpu initialization flow.
	// And then power domain will auto control gpu clk and reset,
	// when poweron or poweroff gpu.
	enable_clks_wrap(kbdev);
	reset_control_deassert(sunxi_data->reset);
#endif

#ifdef CONFIG_MALI_DEVFREQ
	kbdev->current_nominal_freq = freq;
	kbdev->current_voltages[0] = u_volt;
#endif
	sunxi_data->current_freq = freq;
	sunxi_data->current_u_volt = u_volt;

	clk_set_parent(kbdev->clocks[1], kbdev->clocks[0]);

#ifdef CONFIG_DEBUG_FS
	sunxi_create_debugfs();
#endif

	if (sysfs_create_group(&kbdev->dev->kobj, &scene_ctrl_attribute_group)) {
		dev_err(kbdev->dev, "sunxi sysfs group creation failed!\n");
	}

	s_kbdev = kbdev;

#if defined(CONFIG_ARCH_SUN50IW9)
	dev_info(kbdev->dev, "[%ldmv-%ldMHz] bak[%ldmv-%ldMHz] inde_power:%d idle:%d dvfs:%d\n",
		u_volt/1000, freq/1000/1000, sunxi_data->bak_u_volt/1000, sunxi_data->bak_freq/1000/1000,
		sunxi_data->independent_power, sunxi_data->idle_ctrl, sunxi_data->dvfs_ctrl);
#elif defined(CONFIG_ARCH_SUN50IW12)
	dev_info(kbdev->dev, "[%ldmv-%ldMHz] inde_power:%d idle:%d dvfs:%d\n",
		u_volt/1000, freq/1000/1000,
		sunxi_data->independent_power, sunxi_data->idle_ctrl, sunxi_data->dvfs_ctrl);
#endif

	return 0;
}

void sunxi_platform_term(struct kbase_device *kbdev)
{
	debugfs_remove_recursive(sunxi_debugfs);
	sysfs_remove_group(&kbdev->dev->kobj, &scene_ctrl_attribute_group);

#if defined(CONFIG_ARCH_SUN50IW12)
	pm_runtime_disable(kbdev->dev);
	disable_clks_wrap(kbdev);
	reset_control_deassert(sunxi_data->reset);
#endif

#if defined(CONFIG_ARCH_SUN50IW9)
	if (!IS_ERR_OR_NULL(sunxi_data->bak_clk))
		clk_disable_unprepare(sunxi_data->bak_clk);
#endif

	iounmap_regs();
	mutex_destroy(&sunxi_data->sunxi_lock);
	kfree(sunxi_data);

	sunxi_data = NULL;
}

struct kbase_platform_funcs_conf sunxi_platform_conf = {
	.platform_init_func = sunxi_platform_init,
	.platform_term_func = sunxi_platform_term,
};

static int sunxi_protected_mode_enable(struct protected_mode_device *pdev)
{
	u32 val;
	val = readl(sunxi_data->regs.drm.ioaddr);
	val |= 1;
	writel(val, sunxi_data->regs.drm.ioaddr);

	return 0;
}

static int sunxi_protected_mode_disable(struct protected_mode_device *pdev)
{
	u32 val;
	val = readl(sunxi_data->regs.drm.ioaddr);
	val &= ~1;
	writel(val, sunxi_data->regs.drm.ioaddr);

	return 0;
}

struct protected_mode_ops sunxi_protected_ops = {
	.protected_mode_enable = sunxi_protected_mode_enable,
	.protected_mode_disable = sunxi_protected_mode_disable
};

static int sunxi_pm_callback_power_on(struct kbase_device *kbdev)
{
#if defined(CONFIG_ARCH_SUN50IW12)
	int error;
#endif

	if (sunxi_data->power_on)
		goto finish;
#if defined(CONFIG_ARCH_SUN50IW9)
#ifdef CONFIG_REGULATOR
	if (sunxi_data->independent_power
		&& !regulator_is_enabled(kbdev->regulators[0])) {
		if (regulator_enable(kbdev->regulators[0])) {
			dev_err(kbdev->dev, "sunxi gpu enable regulator err...\n");
		}
	}
#endif
#elif defined(CONFIG_ARCH_SUN50IW12)
	error = pm_runtime_get_sync(kbdev->dev);
	if (error == 1)
		return 0;
#else
#error "Need enable regulator for GPU"
#endif
	sunxi_data->power_on = true;

finish:
#if defined(CONFIG_ARCH_SUN50IW9)
	if (sunxi_data->clk_on)
		return 1;

	power_gating(false);
	reset_control_deassert(sunxi_data->reset);

	enable_clks_wrap(kbdev);
	sunxi_data->clk_on = true;
#endif

	return 1;
}

static void sunxi_pm_callback_power_off(struct kbase_device *kbdev)
{
#if defined(CONFIG_ARCH_SUN50IW9)
	if (sunxi_data->clk_on) {
		disable_clks_wrap(kbdev);
		reset_control_assert(sunxi_data->reset);
		power_gating(true);
		sunxi_data->clk_on = false;
	} else {
		goto powerdown;
	}
	if (!sunxi_data->idle_ctrl)
		return;
powerdown:
#endif
	if (!sunxi_data->power_on)
		return;
	sunxi_data->power_on = false;
#if defined(CONFIG_ARCH_SUN50IW9)
#ifdef CONFIG_REGULATOR
	if (sunxi_data->independent_power) {
		if (regulator_disable(kbdev->regulators[0])) {
			dev_err(kbdev->dev, "sunxi gpu enable regulator err...\n");
		}
	}
#endif
#elif defined(CONFIG_ARCH_SUN50IW12)
	pm_runtime_mark_last_busy(kbdev->dev);
	pm_runtime_put_autosuspend(kbdev->dev);
#else
#error "Need enable regulator for GPU"
#endif
}

struct kbase_pm_callback_conf sunxi_pm_callbacks = {
	.power_on_callback = sunxi_pm_callback_power_on,
	.power_off_callback = sunxi_pm_callback_power_off,
	.power_suspend_callback  = NULL,
	.power_resume_callback = NULL
};

#ifdef CONFIG_DEBUG_FS
static ssize_t write_write(struct file *filp, const char __user *buf,
	size_t count, loff_t *offp)
{
	int i, err;
	unsigned long val;
	bool semicolon = false;
	bool update_man = false;
	char buffer[50], data[32];
	int head_size, data_size;
	static unsigned long man_freq;
	static unsigned long man_u_volt;

	if (count >= sizeof(buffer))
		goto err_out;

	if (copy_from_user(buffer, buf, count))
		return -EFAULT;

	/* The command format is '<head>:<data>' or '<head>:<data>;' */
	for (i = 0; i < count; i++) {
		if (*(buffer+i) == ':')
			head_size = i;
		if (*(buffer+i) == ';' && head_size) {
			data_size = count - head_size - 3;
			semicolon = true;
			break;
		}
	}

	if (!head_size)
		goto err_out;

	if (!semicolon)
		data_size = count - head_size - 2;

	if (data_size > 32)
		goto err_out;

	memcpy(data, buffer + head_size + 1, data_size);
	data[data_size] = '\0';

	err = kstrtoul(data, 10, &val);
	if (err)
		goto err_out;

	if (!strncmp("frequency", buffer, head_size)) {
		if (val == 0 || val == 1) {
			sunxi_data->man_ctrl = val ? true : false;
		} else {
			man_freq = val * 1000 * 1000;
			update_man = 1;
		}
	} else if (!strncmp("voltage", buffer, head_size)) {
		if (val == 0 || val == 1) {
			sunxi_data->man_ctrl = val ? true : false;
		} else {
			update_man = 1;
			man_u_volt = val * 1000;
		}
	} else if (!strncmp("idle", buffer, head_size)) {
		if (val == 0 || val == 1)
			sunxi_data->idle_ctrl = val ? true : false;
		else
			goto err_out;
	} else if (!strncmp("dvfs", buffer, head_size)) {
		if (val == 0 || val == 1)
			sunxi_data->dvfs_ctrl = val ? true : false;
		else
			goto err_out;
	} else {
		goto err_out;
	}

	if (update_man && sunxi_data->man_ctrl && man_freq != 0 && man_u_volt != 0) {
		dev_info(s_kbdev->dev, "sunxi gpu man set:valtage:%ld freq:%ld!\n", man_u_volt, man_freq);
		mutex_lock(&sunxi_data->sunxi_lock);
		sunxi_chang_freq_safe(s_kbdev, &man_freq, man_u_volt);
		mutex_unlock(&sunxi_data->sunxi_lock);
	}

	return count;

err_out:
	dev_err(s_kbdev->dev, "sunxi gpu invalid parameter:%s!\n", buffer);
	return -EINVAL;
}

static const struct file_operations write_fops = {
	.owner = THIS_MODULE,
	.write = write_write,
};

static int dump_debugfs_show(struct seq_file *s, void *data)
{

	struct kbasep_pm_metrics diff;
	kbase_pm_get_dvfs_metrics(s_kbdev, &sunxi_data->sunxi_last, &diff);
#ifdef CONFIG_REGULATOR
	if (!IS_ERR_OR_NULL(s_kbdev->regulators[0])) {
		int vol = regulator_get_voltage(s_kbdev->regulators[0]);
		seq_printf(s, "voltage:%dmV;\n", vol);
	}
#endif /* CONFIG_REGULATOR */

	seq_printf(s, "idle:%s;\n", sunxi_data->idle_ctrl ? "on" : "off");
	seq_printf(s, "scenectrl:%s;\n",
			sunxi_data->sence_ctrl ? "on" : "off");
	seq_printf(s, "dvfs:%s;\n",
			sunxi_data->dvfs_ctrl ? "on" : "off");
	seq_printf(s, "independent_power:%s;\n",
			sunxi_data->independent_power ? "yes" : "no");
	seq_printf(s, "Frequency:%luMHz;\n", sunxi_data->current_freq/1000/1000);
	seq_printf(s, "Utilisation from last show:%u%%;\n",
		diff.time_busy * 100 / (diff.time_busy + diff.time_idle));

	seq_puts(s, "\n");

	return 0;
}

static int dump_debugfs_open(struct inode *inode, struct file *file)
{
	return single_open(file, dump_debugfs_show, inode->i_private);
}

static const struct file_operations dump_fops = {
	.owner = THIS_MODULE,
	.open = dump_debugfs_open,
	.read  = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

static int sunxi_create_debugfs(void)
{
	struct dentry *node;

	sunxi_debugfs = debugfs_create_dir("sunxi_gpu", NULL);
	if (IS_ERR_OR_NULL(sunxi_debugfs))
		return -ENOMEM;

	node = debugfs_create_file("write", 0644,
				sunxi_debugfs, NULL, &write_fops);
	if (IS_ERR_OR_NULL(node))
		return -ENOMEM;

	node = debugfs_create_file("dump", 0644,
				sunxi_debugfs, NULL, &dump_fops);
	if (IS_ERR_OR_NULL(node))
		return -ENOMEM;

	return 0;
}
#endif /* CONFIG_DEBUG_FS */
