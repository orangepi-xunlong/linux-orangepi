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
 * Author: liuyuhang<liuyuhang@allwinnertech.com>
 */

#include <sunxi-log.h>
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
#include <backend/gpu/mali_kbase_devfreq.h>
#include <mali_kbase_hwaccess_backend.h>

#if defined(CONFIG_ARCH_SUN55IW3)
#define SMC_REG_BASE 0x03110000
#else
#error "Need configure SMC for GPU"
#endif
#define SMC_GPU_DRM_REG (SMC_REG_BASE + 0x54)

#define SUNXI_BAK_CLK_RATE 648000000

static struct sunxi_data *sunxi_data;
static struct kbase_device *s_kbdev;

void kbase_pm_get_dvfs_metrics(struct kbase_device *kbdev,
			       struct kbasep_pm_metrics *last,
			       struct kbasep_pm_metrics *diff);

static inline void ioremap_regs(void)
{
	struct reg *p_drm;

	p_drm = &sunxi_data->regs.drm;
	p_drm->phys = SMC_GPU_DRM_REG;
	p_drm->ioaddr = ioremap(p_drm->phys, 4);
}

static inline void iounmap_regs(void)
{
	iounmap(sunxi_data->regs.drm.ioaddr);
}

int sunxi_chang_freq_safe(struct kbase_device *kbdev, unsigned long *freq, unsigned long u_volt)
{
	int ret = 0;

#if defined(CONFIG_ARCH_SUN55IW3)
	// increase frequency
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
		/* then set frequency. No need to dynamic change pll-gpu
		 * frequency, otherwise will cause the pll-gpu is not linear
		 * frequency tune.The kbdev->clocks[0] is clk-gpu.
		*/
		ret = clk_set_rate(kbdev->clocks[0], *freq);
		if (ret < 0) {
			dev_err(kbdev->dev, "set gpu core clock to %ld err %d!\n", *freq, ret);
			return ret;
		}
		sunxi_data->current_freq = *freq;
	} else {
		/* decrease frequency first when down frequency, then set
		 * voltage. No need to dynamic change pll-gpu frequency,
		 * otherwise will cause the pll-gpu is not linear frequency
		 * tune. The kbdev->clocks[0] is clk-gpu.
		*/
		ret = clk_set_rate(kbdev->clocks[0], *freq);
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
	/* When pm_callback_resume(system level resume) is called, the sunxi_data->
	 * is_resume_pll_gpu is true, we must set the gpu's freq at max_freq, which
	 * gpu will select pll-gpu as parent clk;
	 */
	if (!sunxi_data->is_resume_pll_gpu && (!sunxi_data->dvfs_ctrl || sunxi_data->sence_ctrl)) {
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
		/* dev_pm_opp_find_freq_floor will call dev_pm_opp_get(),
		 * whilh increment the reference count of OPP.
		 * So, The callers are required to call dev_pm_opp_put()
		 * for the returned OPP after use.
		 */
		opp = dev_pm_opp_find_freq_floor(dev, &val);
		if (!IS_ERR_OR_NULL(opp)) {
			volt = dev_pm_opp_get_voltage(opp);
			dev_pm_opp_put(opp);
		} else {
			val = sunxi_data->max_freq;
			volt = sunxi_data->max_u_volt;
		}
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

static DEVICE_ATTR(scene_ctrl, 0660,
		scene_ctrl_cmd_show, scene_ctrl_cmd_store);

/* read gpu interal registers's value */
static u32 sunxi_kbase_reg_read(struct kbase_device *kbdev, u32 offset)
{
	u32 val;

	val = readl(kbdev->reg + offset);

	return val;
}

/**
 * sunxi_gpu_show - Show callback for the sunxi_gpu_info sysfs entry.
 * @dev:  The device this sysfs file is for.
 * @attr: The attributes of the sysfs file.
 * @buf:  The output buffer to receive the GPU information.
 *
 * This function is called to show the current gpu info(dvfs\voltage\frequency).
 *
 * Return: The number of bytes output to @buf.
 */
static ssize_t sunxi_gpu_info_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	int i;
	ssize_t total = 0;
	size_t max_size = PAGE_SIZE;

	struct kbasep_pm_metrics diff;
	kbase_pm_get_dvfs_metrics(s_kbdev, &sunxi_data->sunxi_last, &diff);
#ifdef CONFIG_REGULATOR
	if (!IS_ERR_OR_NULL(s_kbdev->regulators[0])) {
		int vol = regulator_get_voltage(s_kbdev->regulators[0]);
		total += scnprintf(buf + total, max_size - total, "voltage:%dmV;\n", vol / 1000);
	}
#endif /* CONFIG_REGULATOR */

	total += scnprintf(buf + total, max_size - total, "idle:%s;\n",
			sunxi_data->idle_ctrl ? "on" : "off");
	total += scnprintf(buf + total, max_size - total, "scenectrl:%s;\n",
			sunxi_data->sence_ctrl ? "on" : "off");
	total += scnprintf(buf + total, max_size - total, "dvfs:%s;\n",
			sunxi_data->dvfs_ctrl ? "on" : "off");
	total += scnprintf(buf + total, max_size - total, "independent_power:%s;\n",
			sunxi_data->independent_power ? "yes" : "no");
	total += scnprintf(buf + total, max_size - total, "Frequency:%luMHz;\n",
			sunxi_data->current_freq/1000/1000);
	total += scnprintf(buf + total, max_size - total, "Utilisation from last show:%u%%;\n",
		diff.time_busy * 100 / (diff.time_busy + diff.time_idle));

	total += scnprintf(buf + total, max_size - total, "\nRegister state: \n");
	total += scnprintf(buf + total, max_size - total, "GPU_IRQ_RAWSTAT=0x%08x"
		"	GPU_STATUS=0x%08x\n",
		sunxi_kbase_reg_read(s_kbdev, GPU_CONTROL_REG(GPU_IRQ_RAWSTAT)),
		sunxi_kbase_reg_read(s_kbdev, GPU_CONTROL_REG(GPU_STATUS)));
	total += scnprintf(buf + total, max_size - total, "JOB_IRQ_RAWSTAT=0x%08x"
		"	JOB_IRQ_JS_STATE=0x%08x\n",
		sunxi_kbase_reg_read(s_kbdev, GPU_CONTROL_REG(JOB_IRQ_RAWSTAT)),
		sunxi_kbase_reg_read(s_kbdev, GPU_CONTROL_REG(JOB_IRQ_JS_STATE)));

	for (i = 0; i < 3; i++) {
		total += scnprintf(buf + total, max_size - total, "JS%d_STATUS=0x%08x"
				"	JS%d_HEAD_LO=0x%08x\n",
				i, sunxi_kbase_reg_read(s_kbdev, JOB_SLOT_REG(i, JS_STATUS)),
				i, sunxi_kbase_reg_read(s_kbdev, JOB_SLOT_REG(i, JS_HEAD_LO)));
	}

	total += scnprintf(buf + total, max_size - total, "MMU_IRQ_RAWSTAT=0x%08x"
		"	GPU_FAULTSTATUS=0x%08x\n",
		sunxi_kbase_reg_read(s_kbdev, MMU_REG(MMU_IRQ_RAWSTAT)),
		sunxi_kbase_reg_read(s_kbdev, GPU_CONTROL_REG(GPU_FAULTSTATUS)));
	total += scnprintf(buf + total, max_size - total, "GPU_IRQ_MASK=0x%08x"
		"	JOB_IRQ_MASK=0x%08x	MMU_IRQ_MASK=0x%08x\n",
		sunxi_kbase_reg_read(s_kbdev, GPU_CONTROL_REG(GPU_IRQ_MASK)),
		sunxi_kbase_reg_read(s_kbdev, JOB_CONTROL_REG(JOB_IRQ_MASK)),
		sunxi_kbase_reg_read(s_kbdev, MMU_REG(MMU_IRQ_MASK)));
	total += scnprintf(buf + total, max_size - total, "PWR_OVERRIDE0=0x%08x"
		"	PWR_OVERRIDE1=0x%08x\n",
		sunxi_kbase_reg_read(s_kbdev, GPU_CONTROL_REG(PWR_OVERRIDE0)),
		sunxi_kbase_reg_read(s_kbdev, GPU_CONTROL_REG(PWR_OVERRIDE1)));
	total += scnprintf(buf + total, max_size - total, "SHADER_CONFIG=0x%08x"
		"	L2_MMU_CONFIG=0x%08x\n",
		sunxi_kbase_reg_read(s_kbdev, GPU_CONTROL_REG(SHADER_CONFIG)),
		sunxi_kbase_reg_read(s_kbdev, GPU_CONTROL_REG(L2_MMU_CONFIG)));
	total += scnprintf(buf + total, max_size - total, "TILER_CONFIG=0x%08x"
		"	JM_CONFIG=0x%08x\n",
		sunxi_kbase_reg_read(s_kbdev, GPU_CONTROL_REG(TILER_CONFIG)),
		sunxi_kbase_reg_read(s_kbdev, GPU_CONTROL_REG(JM_CONFIG)));

	return total;
}
static DEVICE_ATTR_RO(sunxi_gpu_info);

/**
 * sunxi_gpu_freq_show - Show callback for the sunxi_gpu_freq sysfs entry.
 * @dev:  The device this sysfs file is for.
 * @attr: The attributes of the sysfs file.
 * @buf:  The output buffer to receive the GPU information.
 *
 * This function is called to show the current gpu frequency.
 *
 * Return: The number of bytes output to @buf.
 */
static ssize_t sunxi_gpu_freq_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	ssize_t total = 0;
	size_t max_size = PAGE_SIZE;
	struct kbasep_pm_metrics diff;

	kbase_pm_get_dvfs_metrics(s_kbdev, &sunxi_data->sunxi_last, &diff);
	total += scnprintf(buf + total, max_size - total, "Frequency:%luMHz;\n",
			sunxi_data->current_freq/1000/1000);
	total += scnprintf(buf + total, max_size - total, "Utilisation from last show:%u%%;\n",
		diff.time_busy * 100 / (diff.time_busy + diff.time_idle));

	return total;
}

/**
 * sunxi_gpu_freq_store - Store callback for the sunxi_gpu_freq sysfs file.
 * @dev:   The device with sysfs file is for
 * @attr:  The attributes of the sysfs file
 * @buf:   The value written to the sysfs file
 * @count: The number of bytes written to the sysfs file
 *
 * This function is called when the sunxi_gpu_info/freq sysfs file is written to.
 * It checks the data written, and if valid updates the gpu dvfs frequency variable
 *
 * Return: @count if the function succeeded. An error code on failure.
 */
static ssize_t sunxi_gpu_freq_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	int ret;
	unsigned long freq_MHz;
	struct kbase_device *kbdev;
	static unsigned long man_freq; /* in Hz */
	static unsigned long man_u_volt; /* in uV */
#if defined(CONFIG_PM_OPP)
	struct dev_pm_opp *opp;
#endif

	kbdev = s_kbdev;
	if (!kbdev)
		return -ENODEV;

	ret = kstrtoul(buf, 0, &freq_MHz);
	if (ret || freq_MHz <= 0) {
		sunxi_err(kbdev->dev, "Couldn't process sunxi_gpu_freq write operation.\n"
				"Use format <freq_MHz>\n");
		return -EINVAL;
	}

	man_freq = freq_MHz * 1000 * 1000;
	man_u_volt = sunxi_data->max_u_volt;

	/* If gpu use independent power-supply, driver can change gpu's voltage */
	if (sunxi_data->independent_power) {
#if defined(CONFIG_PM_OPP)
		opp = dev_pm_opp_find_freq_floor(dev, &man_freq);
		if (!IS_ERR_OR_NULL(opp)) {
			man_u_volt = dev_pm_opp_get_voltage(opp);
			dev_pm_opp_put(opp);
		}
#endif
	} else {
		man_u_volt = sunxi_data->current_u_volt;
	}

	/* If gpu doesn't use independent_power, sunxi_chang_freq_safe will not
	 * change the gpu's voltage; we don't care man_u_volt value.
	 */
	mutex_lock(&sunxi_data->sunxi_lock);
	sunxi_chang_freq_safe(kbdev, &man_freq, man_u_volt);
	mutex_unlock(&sunxi_data->sunxi_lock);

	/* If you manually change the GPU frequency, dvfs will be disabled */
	sunxi_data->dvfs_ctrl = 0;

	return count;
}
static DEVICE_ATTR_RW(sunxi_gpu_freq);

/**
 * sunxi_gpu_volt_show - Show callback for the sunxi_gpu_volt sysfs entry.
 * @dev:  The device this sysfs file is for.
 * @attr: The attributes of the sysfs file.
 * @buf:  The output buffer to receive the GPU information.
 *
 * This function is called to show the current gpu voltage.
 *
 * Return: The number of bytes output to @buf.
 */
static ssize_t sunxi_gpu_volt_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%ldmv\n", sunxi_data->current_u_volt / 1000);
}

/**
 * sunxi_gpu_volt_store - Store callback for the sunxi_gpu_volt sysfs file.
 * @dev:   The device with sysfs file is for
 * @attr:  The attributes of the sysfs file
 * @buf:   The value written to the sysfs file
 * @count: The number of bytes written to the sysfs file
 *
 * This function is called when the sunxi_gpu_info/freq sysfs file is written to.
 * It checks the data written, and if valid updates the gpu dvfs frequency variable
 *
 * Return: @count if the function succeeded. An error code on failure.
 */
static ssize_t sunxi_gpu_volt_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	int ret;
	unsigned long volt_mV;
	struct kbase_device *kbdev;
	static unsigned long man_u_volt; /* in uV */

	kbdev = s_kbdev;
	if (!kbdev)
		return -ENODEV;

	if (!sunxi_data->independent_power) {
		sunxi_err(kbdev->dev, "GPU not support change voltage!!!\n");
		return -EPERM;
	}

	ret = kstrtoul(buf, 0, &volt_mV);
	if (ret || volt_mV <= 0) {
		sunxi_err(kbdev->dev, "Couldn't process sunxi_gpu_volt write operation.\n"
				"Use format <volt_mV>\n");
		return -EINVAL;
	}

	man_u_volt = volt_mV * 1000;

#ifdef CONFIG_REGULATOR
	mutex_lock(&sunxi_data->sunxi_lock);
	ret = regulator_set_voltage(kbdev->regulators[0], man_u_volt, man_u_volt);
	if (ret < 0) {
		sunxi_err(kbdev->dev, "sunxi_gpu_volt set gpu voltage err %d!\n", ret);
		mutex_unlock(&sunxi_data->sunxi_lock);
		return ret;
	}
	mutex_unlock(&sunxi_data->sunxi_lock);
#endif

	sunxi_data->current_u_volt = man_u_volt;
#ifdef CONFIG_MALI_DEVFREQ
	kbdev->current_voltages[0] = man_u_volt;
#endif
	/* If you manually change the GPU voltage, dvfs will be disabled */
	sunxi_data->dvfs_ctrl = 0;

	return count;
}
static DEVICE_ATTR_RW(sunxi_gpu_volt);

/**
 * sunxi_gpu_dvfs_show - Show callback for the sunxi_gpu_dvfs sysfs entry.
 * @dev:  The device this sysfs file is for.
 * @attr: The attributes of the sysfs file.
 * @buf:  The output buffer to receive the GPU information.
 *
 * This function is called to show the current gpu dvfs_ctrl status.
 *
 * Return: The number of bytes output to @buf.
 */
static ssize_t sunxi_gpu_dvfs_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", sunxi_data->dvfs_ctrl);
}

/**
 * sunxi_gpu_dvfs_store - Store callback for the sunxi_gpu_dvfs sysfs file.
 * @dev:   The device with sysfs file is for
 * @attr:  The attributes of the sysfs file
 * @buf:   The value written to the sysfs file
 * @count: The number of bytes written to the sysfs file
 *
 * This function is called when the sunxi_gpu_info/freq sysfs file is written to.
 * It checks the data written, and if valid updates the gpu dvfs_ctrl variable
 *
 * Return: @count if the function succeeded. An error code on failure.
 */
static ssize_t sunxi_gpu_dvfs_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	int ret;
	int dvfs_ctrl;
	struct kbase_device *kbdev;

	kbdev = s_kbdev;
	if (!kbdev)
		return -ENODEV;

	ret = kstrtoint(buf, 0, &dvfs_ctrl);
	if (ret || dvfs_ctrl < 0) {
		sunxi_err(kbdev->dev, "Couldn't process sunxi_gpu_dvfs write operation.\n"
				"Use format <bool>\n");
		return -EINVAL;
	}

	sunxi_data->dvfs_ctrl = dvfs_ctrl ? true : false;

	return count;
}
static DEVICE_ATTR_RW(sunxi_gpu_dvfs);

#if defined(CONFIG_PM_OPP)
/**
 * gpu_opp_ops_show - Show callback for the gpu_opp_ops sysfs entry.
 * @dev:  The device this sysfs file is for.
 * @attr: The attributes of the sysfs file.
 * @buf:  The output buffer to receive the GPU opp_table information.
 *
 * This function is called to show the current gpu opp-tables.
 * eg:
 * opp0 is 696000000Hz---920mV
 * opp1 is 600000000Hz---920mV
 * opp2 is 400000000Hz---920mV
 * opp3 is 300000000Hz---920mV
 * opp4 is 200000000Hz---920mV
 * opp5 is 150000000Hz---920mV
 *
 * Return: The number of bytes output to @buf.
 */
static ssize_t gpu_opp_ops_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct kbase_device *kbdev;
	int count;
	int i = 0;
	unsigned long freq;
	unsigned long volt;
	struct dev_pm_opp *opp;
	ssize_t total = 0;
	size_t max_size = PAGE_SIZE;

	kbdev = s_kbdev;
	if (!kbdev)
		return -ENODEV;

	count = dev_pm_opp_get_opp_count(kbdev->dev);
	if (count <= 0)
		return sprintf(buf, "Warning: The gpu opp-table is empty!\n");

	for (i = 0, freq = ULONG_MAX; i < count; i++, freq--) {
		opp = dev_pm_opp_find_freq_floor(kbdev->dev, &freq);
		if (IS_ERR(opp))
			break;

		if (sunxi_data->independent_power)
			volt = dev_pm_opp_get_voltage(opp);
		else
			volt = sunxi_data->current_u_volt;

		dev_pm_opp_put(opp);
		total += scnprintf(buf + total, max_size - total,
			"opp%d is %luHz---%lumV\n",
			i, freq, volt / 1000);
	}

	return total;
}

/**
 * gpu_opp_ops_store - Store callback for the gpu_opp_ops sysfs file.
 * @dev:   The device with sysfs file is for
 * @attr:  The attributes of the sysfs file
 * @buf:   The value written to the sysfs file
 * @count: The number of bytes written to the sysfs file
 *
 * This function is called when the sunxi_gpu/gpu_opp_ops sysfs file is written to.
 * It checks the data written, and if valid updates the gpu opp-table
 *
 * eg:
 * echo disable > gpu_opp_ops; --> Terminate dvfs and remove opp-table.
 *
 * echo freq0_MHz volt0_mv freq1_MHz volt2_mV ... freqn_MH voltn_mV > gpu_opp_ops
 * --> Init n pairs opps. Up to 6 pairs opp are support.
 *
 * echo enable > gpu_opp_ops;  --> Init dvfs with new opp-table.
 *
 * Return: @count if the function succeeded. An error code on failure.
 */
static ssize_t gpu_opp_ops_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	struct kbase_device *kbdev;
	struct dev_pm_opp *opp;
	int items;
	uint32_t i;
	uint32_t *freq_MHz;
	uint32_t *volt_mV;
	unsigned long freq; /* in Hz */
	unsigned long voltage; /* in uV */
	char command[8] = { 0x00 };
	int ret = 0;

	kbdev = s_kbdev;
	if (!kbdev)
		return -ENODEV;

	items = sscanf(buf, "%s", command);
	sunxi_info(kbdev->dev, "command is %s\n", command);

	if (sysfs_streq(command, "enable") && !kbdev->devfreq) {
		kbase_backend_devfreq_init(kbdev);
		sunxi_data->dvfs_ctrl = true;
		return count;
	}

	if (sysfs_streq(command, "disable")) {
		/* mali devfreq has been terminated */
		if (!kbdev->devfreq) {
			sunxi_info(kbdev->dev, "mali devfreq has been terminated\n");
			return count;
		}

		if (sunxi_data->power_on) {
			sunxi_err(kbdev->dev, "mali driver is running with dvfs\n"
				"If you want to change gpu's opp-tables, you "
				"should keep gpu free!\n");
			return -EPERM;
		}

		/*
		 * Terminate mali_kbase driver dvfs.
		 * Free OPP table entries created from static DT entries
		 */
		kbase_backend_devfreq_term(kbdev);
		dev_pm_opp_of_remove_table(kbdev->dev);
		sunxi_data->dvfs_ctrl = false;
		sunxi_info(kbdev->dev, "Term mali devfreq and rm opp_table\n");
		return count;
	}

	/*
	 * The next implementation is update gpu opp-tables, if the gpu's
	 * devfreq is already exists, updating the gpu opp-tables is not allowed.
	 */
	if (kbdev->devfreq) {
		sunxi_err(kbdev->dev, "mali driver devfreq is already exists\n"
			"If you want to change gpu's opp-tables, you should\n"
			"echo disable >gpu_opp_ops\n");
		return -EPERM;
	}

	freq_MHz = kmalloc(sizeof(uint32_t) * 6, GFP_KERNEL);
	volt_mV = kmalloc(sizeof(uint32_t) * 6, GFP_KERNEL);
	if (!freq_MHz || !volt_mV)
		return -ENOMEM;

	items = sscanf(buf, "%u %u %u %u %u %u %u %u %u %u %u %u",
		&freq_MHz[0], &volt_mV[0], &freq_MHz[1], &volt_mV[1],
		&freq_MHz[2], &volt_mV[2], &freq_MHz[3], &volt_mV[3],
		&freq_MHz[4], &volt_mV[4], &freq_MHz[5], &volt_mV[5]);

	if (items % 2 != 0) {
		sunxi_err(kbdev->dev, "Couldn't process gpu_opp_ops write operation.\n"
			"Use format <freq0_MHz> <volt1_mV> <freq1_MHz> <volt2_mV>\n"
			"Up to 6 groups can be set up\n");
		ret = -EINVAL;
		goto free_input_table;
	}

	if (dev_pm_opp_get_opp_count(kbdev->dev) > 0) {
		/*
		 * Prevent generating duplicate OPPs, free OPPs created
		 * using the dynamically added entries(dev_pm_opp_add).
		 */
		dev_pm_opp_remove_all_dynamic(kbdev->dev);
	}

	for (i = 0; i < items / 2; i++) {
		freq = freq_MHz[i] * 1000 * 1000;
		voltage = sunxi_data->current_u_volt;

		/* If gpu use independent power, we cloud updates new voltage */
		if (sunxi_data->independent_power)
			voltage = volt_mV[i] * 1000;

		ret = dev_pm_opp_add(kbdev->dev, freq, voltage);
		if (ret) {
			sunxi_err(kbdev->dev, "add opp failed, freq is %ldHz,"
				"voltage is %lduV\n", freq, voltage);
			goto free_input_table;
		}
	}

	/* Update the pll-gpu rate */
	freq = ULONG_MAX;
	opp = dev_pm_opp_find_freq_floor(kbdev->dev, &freq);
	if (IS_ERR(opp))
		goto free_input_table;

	sunxi_data->max_freq = freq;

	if (sunxi_data->independent_power)
		sunxi_data->max_u_volt = dev_pm_opp_get_voltage(opp);
	dev_pm_opp_put(opp);

	clk_set_rate(sunxi_data->pll_gpu, freq);

free_input_table:
	kfree(freq_MHz);
	kfree(volt_mV);

	if (ret)
		return ret;

	return count;
}
static DEVICE_ATTR_RW(gpu_opp_ops);
#endif

static struct attribute *sunxi_gpu_attributes[] = {
	&dev_attr_sunxi_gpu_info.attr,
	&dev_attr_scene_ctrl.attr,
	&dev_attr_sunxi_gpu_freq.attr,
	&dev_attr_sunxi_gpu_volt.attr,
	&dev_attr_sunxi_gpu_dvfs.attr,
#if defined(CONFIG_PM_OPP)
	&dev_attr_gpu_opp_ops.attr,
#endif
	NULL
};

static struct attribute_group sunxi_gpu_attribute_group = {
	.name = "sunxi_gpu",
	.attrs = sunxi_gpu_attributes,
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
	sunxi_data->is_resume_pll_gpu = false;
	parse_dts_and_fex(kbdev, sunxi_data);
#ifdef CONFIG_REGULATOR
	if (IS_ERR_OR_NULL(kbdev->regulators[0])) {
		sunxi_data->independent_power = 0;
	}
#else
	sunxi_data->independent_power = 0;
#endif

	sunxi_data->pll_gpu = of_clk_get(kbdev->dev->of_node, 2);
	if (IS_ERR_OR_NULL(sunxi_data->pll_gpu)) {
		/* pll-gpu is one of the clk-gpu parent clks, which is
		 * defined in ccu-sun55iw3.c.
		 * So we just need to set the frequency of the pll-gpu,
		 * no need to enable pll-gpu. When driver want clk-gpu
		 * to run at the Maximum frequency(defined in opp_table),
		 * the clk-gpu will select pll-gpu as parent clk, and
		 * prepare & enable it.
		 */
		//clk_prepare_enable(sunxi_data->pll_gpu);
		sunxi_info(kbdev->dev, "sunxi init gpu Failed to get pll_gpu\n");
	}

#if !defined(CONFIG_PM_OPP)
	clk_set_rate(kbdev->clocks[0], SUNXI_BAK_CLK_RATE);
	clk_set_rate(sunxi_data->pll_gpu, SUNXI_BAK_CLK_RATE);
#else
	freq = ULONG_MAX;
	opp = dev_pm_opp_find_freq_floor(kbdev->dev, &freq);
	if (IS_ERR_OR_NULL(opp)) {
		dev_err(kbdev->dev, "sunxi init gpu Failed to get opp (%ld)\n", PTR_ERR(opp));
		freq = SUNXI_BAK_CLK_RATE;
	} else {
		u_volt = dev_pm_opp_get_voltage(opp);

		/* dev_pm_opp_find_freq_floor will call dev_pm_opp_get(),
		 * whilh increment the reference count of OPP.
		 * So, The callers are required to call dev_pm_opp_put()
		 * for the returned OPP after use.
		 */
		dev_pm_opp_put(opp);
	}

#ifdef CONFIG_REGULATOR
	if (sunxi_data->independent_power) {
		if (regulator_set_voltage(kbdev->regulators[0], u_volt, u_volt) < 0)
			dev_err(kbdev->dev, "sunxi init set gpu voltage err.\n");
	}
	if (kbdev->regulators[0])
		u_volt = regulator_get_voltage(kbdev->regulators[0]);
#endif
	clk_set_rate(kbdev->clocks[0], freq);
	clk_set_rate(sunxi_data->pll_gpu, freq);
#endif

	sunxi_data->reset = devm_reset_control_get(kbdev->dev, NULL);
	if (IS_ERR_OR_NULL(sunxi_data->reset)) {
		dev_info(kbdev->dev, "sunxi init gpu Failed to get reset ctrl\n");
	}

	sunxi_data->max_freq = freq;
	sunxi_data->max_u_volt = u_volt;
	ioremap_regs();
	mutex_init(&sunxi_data->sunxi_lock);

#if defined(CONFIG_ARCH_SUN55IW3)
	/* GPU driver need use pm_runtime framework to let power domain
	 * control poweron or poweroff of gpu.
	 * So during the previous initialization phase, the GPU driver
	 * will call function kbase_device_runtime_init().
	 *
	 * When GPU using power domain, you need enable clk and
	 * deassert gpu reset in gpu initialization flow.
	 * And then power domain will auto control gpu clk and
	 * reset, when poweron or poweroff gpu.
	 *
	 * In previous kbase_device_pm_init() phase, the pll-gpu and gpu
	 * clk have been initialized. So only need to deassert gpu reset.
	 */
	reset_control_deassert(sunxi_data->reset);
#endif

#ifdef CONFIG_MALI_DEVFREQ
	kbdev->current_nominal_freq = freq;
	kbdev->current_voltages[0] = u_volt;
#endif
	sunxi_data->current_freq = freq;
	sunxi_data->current_u_volt = u_volt;

	if (sysfs_create_group(&kbdev->dev->kobj, &sunxi_gpu_attribute_group)) {
		dev_err(kbdev->dev, "sunxi sysfs group creation failed!\n");
	}

	s_kbdev = kbdev;

#if defined(CONFIG_ARCH_SUN55IW3)
	dev_info(kbdev->dev, "[%ldmv-%ldMHz] inde_power:%d idle:%d dvfs:%d\n",
		u_volt/1000, freq/1000/1000,
		sunxi_data->independent_power, sunxi_data->idle_ctrl, sunxi_data->dvfs_ctrl);
#endif

	return 0;
}

void sunxi_platform_term(struct kbase_device *kbdev)
{
	sysfs_remove_group(&kbdev->dev->kobj, &sunxi_gpu_attribute_group);

#if defined(CONFIG_ARCH_SUN55IW3)
	reset_control_deassert(sunxi_data->reset);
#endif

	iounmap_regs();
	mutex_destroy(&sunxi_data->sunxi_lock);
	kfree(sunxi_data);

	sunxi_data = NULL;
}

struct kbase_platform_funcs_conf sunxi_platform_conf = {
	.platform_init_func = sunxi_platform_init,
	.platform_term_func = sunxi_platform_term,
	.platform_late_init_func = NULL,
	.platform_late_term_func = NULL,
#if !MALI_USE_CSF
	.platform_handler_context_init_func = NULL,
	.platform_handler_context_term_func = NULL,
	.platform_handler_atom_submit_func = NULL,
	.platform_handler_atom_complete_func = NULL
#endif
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

/* NOTE: kbdev->nr_clocks is 2, kbdev->clocks[0] is clk-gpu,
 * kbdev->clocks[1] is bus-gpu;
 *
 */
static void enable_gpu_power_control(struct kbase_device *kbdev)
{
	unsigned int i;
	dev_dbg(kbdev->dev, "%s\n", __func__);
/*
 * If GPU use independent_power, user need to care regulators
 */
#if defined(CONFIG_REGULATOR)
	if (sunxi_data->independent_power) {
		for (i = 0; i < kbdev->nr_regulators; i++) {
			if (WARN_ON(kbdev->regulators[i] == NULL))
				;
			else if (!regulator_is_enabled(kbdev->regulators[i]))
				WARN_ON(regulator_enable(kbdev->regulators[i]));
		}
	}
#endif

	/* After waking up from pm_callback_suspend(system level suspend), the GPU
	 * hardware defaults to selecting pll-gpu as the parent clock. The software
	 * needs to do corresponding synchronization configuration, otherwise gpu
	 * cannot be awakened.
	 */
	if (sunxi_data->is_resume_pll_gpu) {
		clk_set_parent(kbdev->clocks[0], sunxi_data->pll_gpu);
		kbase_devfreq_force_freq(kbdev, sunxi_data->max_freq);
		sunxi_data->is_resume_pll_gpu = false;
	}

	for (i = 0; i < kbdev->nr_clocks; i++) {
		if (WARN_ON(kbdev->clocks[i] == NULL))
			;
		else if (!__clk_is_enabled(kbdev->clocks[i]))
			WARN_ON(clk_prepare_enable(kbdev->clocks[i]));
	}
}

static void disable_gpu_power_control(struct kbase_device *kbdev)
{
	unsigned int i;
	dev_dbg(kbdev->dev, "%s\n", __func__);

	for (i = 0; i < kbdev->nr_clocks; i++) {
		if (WARN_ON(kbdev->clocks[i] == NULL))
			;
		else if (__clk_is_enabled(kbdev->clocks[i])) {
			clk_disable_unprepare(kbdev->clocks[i]);
			WARN_ON(__clk_is_enabled(kbdev->clocks[i]));
		}
	}

/*
 * If GPU use independent_power, user need to care regulators
 */
#if defined(CONFIG_REGULATOR)
	if (sunxi_data->independent_power) {
		for (i = 0; i < kbdev->nr_regulators; i++) {
			if (WARN_ON(kbdev->regulators[i] == NULL))
				;
			else if (regulator_is_enabled(kbdev->regulators[i]))
				WARN_ON(regulator_disable(kbdev->regulators[i]));
		}
	}
#endif

}

static int sunxi_pm_callback_power_on(struct kbase_device *kbdev)
{
	int ret = 1; /* Assume GPU has been powered off */
	int error;

	dev_dbg(kbdev->dev, "%s %pK\n", __func__, (void *)kbdev->dev->pm_domain);

#ifdef KBASE_PM_RUNTIME
	error = pm_runtime_get_sync(kbdev->dev);
	if (error == 1) {
		/*
		 * Let core know that the chip has not been
		 * powered off, so we can save on re-initialization.
		 */
		ret = 0;
	}
	dev_dbg(kbdev->dev, "pm_runtime_get_sync returned %d\n", error);
#else
	enable_gpu_power_control(kbdev);
#endif /* KBASE_PM_RUNTIME */

	sunxi_data->power_on = true;
	return ret;
}

static void sunxi_pm_callback_power_off(struct kbase_device *kbdev)
{
	dev_dbg(kbdev->dev, "%s\n", __func__);

#ifdef KBASE_PM_RUNTIME
	pm_runtime_mark_last_busy(kbdev->dev);
	pm_runtime_put_autosuspend(kbdev->dev);
#else
	/* Power down the GPU immediately as runtime PM is disabled */
	disable_gpu_power_control(kbdev);
#endif

	sunxi_data->power_on = false;
}

#ifdef KBASE_PM_RUNTIME
static int kbase_device_runtime_init(struct kbase_device *kbdev)
{
	int ret = 0;

	dev_dbg(kbdev->dev, "%s\n", __func__);

	pm_runtime_set_autosuspend_delay(kbdev->dev, AUTO_SUSPEND_DELAY);
	pm_runtime_use_autosuspend(kbdev->dev);

	pm_runtime_set_active(kbdev->dev);
	pm_runtime_enable(kbdev->dev);

	if (!pm_runtime_enabled(kbdev->dev)) {
		dev_warn(kbdev->dev, "pm_runtime not enabled");
		ret = -EINVAL;
	}

	return ret;
}

static void kbase_device_runtime_disable(struct kbase_device *kbdev)
{
	dev_dbg(kbdev->dev, "%s\n", __func__);
	pm_runtime_disable(kbdev->dev);
}
#endif /* KBASE_PM_RUNTIME */

static int pm_callback_runtime_on(struct kbase_device *kbdev)
{
	dev_dbg(kbdev->dev, "%s\n", __func__);

#if !MALI_USE_CSF
	enable_gpu_power_control(kbdev);
#endif
	return 0;
}

static void pm_callback_runtime_off(struct kbase_device *kbdev)
{
	dev_dbg(kbdev->dev, "%s\n", __func__);

#if !MALI_USE_CSF
	disable_gpu_power_control(kbdev);
#endif
}

static void pm_callback_resume(struct kbase_device *kbdev)
{
	int ret;

	sunxi_data->is_resume_pll_gpu = true;
	ret = pm_callback_runtime_on(kbdev);

	WARN_ON(ret);
}

static void pm_callback_suspend(struct kbase_device *kbdev)
{
	pm_callback_runtime_off(kbdev);
}

struct kbase_pm_callback_conf sunxi_pm_callbacks = {
	.power_on_callback = sunxi_pm_callback_power_on,
	.power_off_callback = sunxi_pm_callback_power_off,
	.power_suspend_callback = pm_callback_suspend,
	.power_resume_callback = pm_callback_resume,
#ifdef KBASE_PM_RUNTIME
	.power_runtime_init_callback = kbase_device_runtime_init,
	.power_runtime_term_callback = kbase_device_runtime_disable,
	.power_runtime_on_callback = pm_callback_runtime_on,
	.power_runtime_off_callback = pm_callback_runtime_off,
#else				/* KBASE_PM_RUNTIME */
	.power_runtime_init_callback = NULL,
	.power_runtime_term_callback = NULL,
	.power_runtime_on_callback = NULL,
	.power_runtime_off_callback = NULL,
#endif				/* KBASE_PM_RUNTIME */
};
