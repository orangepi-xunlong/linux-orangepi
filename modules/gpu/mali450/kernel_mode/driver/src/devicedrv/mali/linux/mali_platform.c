/**
 * Copyright (C) 2015-2016 Allwinner Technology Limited. All rights reserved.
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

#include <linux/aw/platform.h>
#include <linux/aw/config.h>

extern unsigned long totalram_pages;

static void mali_gpu_utilization_callback(struct mali_gpu_utilization_data *data);

static void aw_dvfs_queue_work(struct work_struct *work);

#ifdef CONFIG_SUNXI_GPU_COOLING
extern int gpu_thermal_cool_unregister(void);
#endif /* CONFIG_SUNXI_GPU_COOLING */

static struct mali_gpu_device_data mali_gpu_data = {
	.control_interval = 500, /* 500ms */
	.utilization_callback = mali_gpu_utilization_callback,
};

#ifndef CONFIG_MALI_DT
/* For Mali450, the number of maximum PP core is 8 */
static struct resource mali_gpu_resources[] = {
	MALI_GPU_RESOURCES_MALI450_MPX_PMU(GPU_PBASE,
					IRQ_GPU_GP,
					IRQ_GPU_GPMMU,
					IRQ_GPU_PP0,
					IRQ_GPU_PPMMU0
					#if MALI_PP_CORES_NUM == 2
					,
					IRQ_GPU_PP1,
					IRQ_GPU_PPMMU1
					#endif
					#if MALI_PP_CORES_NUM == 3
					,
					IRQ_GPU_PP2,
					IRQ_GPU_PPMMU2
					#endif
					#if MALI_PP_CORES_NUM == 4
					,
					IRQ_GPU_PP3,
					IRQ_GPU_PPMMU3
					#endif
					#if MALI_PP_CORES_NUM == 5
					,
					IRQ_GPU_PP4,
					IRQ_GPU_PPMMU4
					#endif
					#if MALI_PP_CORES_NUM == 6
					,
					IRQ_GPU_PP5,
					IRQ_GPU_PPMMU5
					#endif
					#if MALI_PP_CORES_NUM == 7
					,
					IRQ_GPU_PP6,
					IRQ_GPU_PPMMU6
					#endif
					#if MALI_PP_CORES_NUM == 8
					,
					IRQ_GPU_PP7,
					IRQ_GPU_PPMMU7
					#endif
					)
};

static struct platform_device mali_gpu_device = {
	.name = MALI_GPU_NAME_UTGARD,
	.id = 0,
};
#endif /* CONFIG_MALI_DT */

/**
 ****************************************************************
 * Function   : get_gpu_clk
 * Description: Get gpu related clocks.
 * Input      : None.
 * Output     : None.
 ****************************************************************
 */
static void get_gpu_clk(void)
{
	int i;

	for (i = 0; i < sizeof(aw_private.pm.clk)/sizeof(aw_private.pm.clk[0]); i++) {
#ifdef CONFIG_MALI_DT
		aw_private.pm.clk[i].clk_handle = of_clk_get(aw_private.np_gpu, i);
#else
		aw_private.pm.clk[i].clk_handle = clk_get(NULL, aw_private.pm.clk[i].clk_id);
#endif /* CONFIG_MALI_DT */
		if (IS_ERR_OR_NULL(aw_private.pm.clk[i].clk_handle)) {
			MALI_PRINT_ERROR(("Failed to get gpu %s clock!\n", aw_private.pm.clk[i].clk_name));
			BUG();
		}
	}
}

/**
 ****************************************************************
 * Function   : put_gpu_clk
 * Description: Put gpu related clocks.
 * Input      : None.
 * Output     : None.
 ****************************************************************
 */
static void put_gpu_clk(void)
{
	int i;

	for (i = 0; i < sizeof(aw_private.pm.clk)/sizeof(aw_private.pm.clk[0]); i++)
		clk_put(aw_private.pm.clk[i].clk_handle);
}

/**
 ****************************************************************
 * Function   : enable_gpu_clk
 * Description: Enable gpu related clocks.
 * Input      : None.
 * Output     : None.
 ****************************************************************
 */
void enable_gpu_clk(void)
{
	int i;

	for (i = 0; i < sizeof(aw_private.pm.clk)/sizeof(aw_private.pm.clk[0]); i++) {
		if (!aw_private.pm.clk[i].clk_status) {
			if (clk_prepare_enable(aw_private.pm.clk[i].clk_handle)) {
				MALI_PRINT_ERROR(("Failed to enable %s clock!\n", aw_private.pm.clk[i].clk_name));
				BUG();

				return;
			}
			aw_private.pm.clk[i].clk_status = 1;
		}
	}
}

/**
 ****************************************************************
 * Function   : disable_gpu_clk
 * Description: Disable gpu related clocks
 * Input      : None.
 * Output     : None.
 ****************************************************************
 */
void disable_gpu_clk(void)
{
	int i;

	for (i = sizeof(aw_private.pm.clk)/sizeof(aw_private.pm.clk[0])-1; i >= 0; i--) {
		if (aw_private.pm.clk[i].clk_status) {
			clk_disable_unprepare(aw_private.pm.clk[i].clk_handle);
			aw_private.pm.clk[i].clk_status = 0;
		}
	}
}

/**
 ****************************************************************
 * Function   : set_freq
 * Description: Set the frequency of GPU related clocks.
 * Input      : freq, the frequency value to be set in MHz.
 * Output     : None.
 ****************************************************************
 */
static void set_freq(int freq)
{
	int i;

	if (freq <= 0 || freq == aw_private.pm.clk[0].freq)
		return;

	for (i = 0; i < sizeof(aw_private.pm.clk)/sizeof(aw_private.pm.clk[0]); i++) {
		if (clk_set_rate(aw_private.pm.clk[i].clk_handle, freq*1000*1000)) {
			MALI_PRINT_ERROR(("Failed to set the frequency of gpu %s clock: Current frequency is %ld MHz, the frequency to be is %d MHz\n",
								aw_private.pm.clk[i].clk_name, clk_get_rate(aw_private.pm.clk[i].clk_handle)/(1000*1000), freq));
			return;
		} else {
			aw_private.pm.clk[i].freq = clk_get_rate(aw_private.pm.clk[i].clk_handle)/(1000*1000);
		}
	}

	MALI_PRINT(("Set gpu frequency to %d MHz\n", freq));
}

/**
 ****************************************************************
 * Function   : set_freq_wrap
 * Description: Set the frequency of gpu, Mali GPU will pause dur-
 *		ing setting its frequency and resume after the freq-
 *		uency is changed.
 * Input      : freq, the frequency to be set in MHz.
 * Output     : None.
 ****************************************************************
 */
void set_freq_wrap(int freq)
{
	if (freq <= 0 || freq == aw_private.pm.clk[0].freq)
		return;

	BUG_ON(NULL == &aw_private.pm.dvfs_lock);

	if (&aw_private.pm.dvfs_lock) {
		mutex_lock(&aw_private.pm.dvfs_lock);
		mali_dev_pause();
		set_freq(freq);
		mali_dev_resume();
		mutex_unlock(&aw_private.pm.dvfs_lock);
	}
}

/**
 ****************************************************************
 * Function   : enable_gpu_power
 * Description: Enable GPU power.
 * Input      : None.
 * Output     : None.
 ****************************************************************
 */
static void enable_gpu_power(void)
{
	if (IS_ERR_OR_NULL(aw_private.pm.regulator))
		return;
	
	if (regulator_enable(aw_private.pm.regulator)) {
		MALI_PRINT_ERROR(("Failed to enable gpu power!\n"));

	} else {
		/*
		 * If DVM is disabled, the voltage may be unstable after enable_gpu_power finished,
		 * so we need to delay 2 ms for power stability.
		 */
		if (!aw_private.pm.dvm && aw_private.pm.independent_pow)
			mdelay(2);

		MALI_PRINT(("Enable gpu power successfully.\n"));
	}
}

/**
 ****************************************************************
 * Function   : disable_gpu_power
 * Description: Disable GPU power.
 * Input      : None.
 * Output     : None.
 ****************************************************************
 */
static void disable_gpu_power(void)
{
	if (IS_ERR_OR_NULL(aw_private.pm.regulator))
		return;

	if (regulator_disable(aw_private.pm.regulator))
		MALI_PRINT_ERROR(("Failed to disable gpu power!\n"));
	else
		MALI_PRINT(("Disable gpu power successfully.\n"));
}

/**
 ****************************************************************
 * Function   : set_voltage
 * Description: Set the voltage of gpu.
 * Input      : vol, the voltage to be set in mV.
 * Output     : None.
 ****************************************************************
 */
void set_voltage(int vol)
{
	if (vol <= 0 || IS_ERR_OR_NULL(aw_private.pm.regulator))
		return;

	if (vol == aw_private.pm.current_vol)
		return;

	if (!aw_private.pm.dvm) {
		mutex_lock(&aw_private.pm.dvfs_lock);
		mali_dev_pause();
	}

	if (!IS_ERR_OR_NULL(aw_private.pm.regulator)) {
		if (regulator_set_voltage(aw_private.pm.regulator, vol*1000, vol*1000) != 0) {
			MALI_PRINT_ERROR(("Failed to set gpu voltage!\n"));
		} else {
			aw_private.pm.current_vol = regulator_get_voltage(aw_private.pm.regulator)/1000;
			MALI_PRINT(("Set gpu voltage to %dmV.\n", aw_private.pm.current_vol));
		}
	}

	if (!aw_private.pm.dvm) {
		mali_dev_resume();
		mutex_unlock(&aw_private.pm.dvfs_lock);
	}
}

/**
 ****************************************************************
 * Function   : dvfs_change
 * Description: Change the voltage and frequency of GPU
 * Input      : level, the level to be set.
 * Output     : None.
 ****************************************************************
 */
void dvfs_change(u8 level)
{
	if (level == aw_private.pm.current_level)
		return;

	if (aw_private.pm.vf_table[level].freq < aw_private.pm.clk[0].freq) {
		set_freq_wrap(aw_private.pm.vf_table[level].freq);
		set_voltage(aw_private.pm.vf_table[level].vol);
	} else {
		set_voltage(aw_private.pm.vf_table[level].vol);
		set_freq_wrap(aw_private.pm.vf_table[level].freq);
	}

	aw_private.pm.current_level = level;
}

/**
 ****************************************************************
 * Function   : aw_suspend
 * Description: Called during suspend.
 * Input      : None.
 * Output     : None.
 ****************************************************************
 */
void aw_suspend(void)
{
	disable_gpu_clk();
	disable_gpu_power();
}

/**
 ****************************************************************
 * Function   : aw_resume
 * Description: Called during resume.
 * Input      : None.
 * Output     : None.
 ****************************************************************
 */
void aw_resume(void)
{
	enable_gpu_power();
	enable_gpu_clk();
}

void revise_current_level(void)
{
	int i;
	aw_private.pm.current_level = -1;
	for (i = 0; i <= aw_private.pm.max_level; i++) {
		if (aw_private.pm.clk[0].freq == aw_private.pm.vf_table[i].freq) {
			if (IS_ERR_OR_NULL(aw_private.pm.regulator) || aw_private.pm.current_vol == aw_private.pm.vf_table[i].vol) {
				aw_private.pm.current_level = i;
				break;
			}
		}
	}
}

#ifdef CONFIG_SUNXI_GPU_COOLING
/**
 ****************************************************************
 * Function   : gpu_thermal_cool
 * Description: Called by thermal cooling system to restrict the
 *		maximum frequency of GPU.
 * Input      : freq, the maximum frequency to be restricted in MHz.
 * Output     : Zero.
 ****************************************************************
 */
int gpu_thermal_cool(int freq /* MHz */)
{
	int i;

	if (aw_private.tempctrl.temp_ctrl_status) {
		aw_private.pm.cool_freq = freq;

		if (freq > 0) {
			for (i = aw_private.pm.max_level; i >= 0; i--) {
				if (aw_private.pm.vf_table[i].freq <= freq) {
					aw_private.pm.max_available_level = i;
					break;
				}
			}
		} else {
			aw_private.pm.max_available_level = aw_private.pm.max_level;
		}

		if (!aw_private.pm.dvfs_status) {
			if (freq > 0) {
				set_freq_wrap(freq);
				revise_current_level();
				return 0;
			} else {
				dvfs_change(aw_private.pm.begin_level);
			}
		}

		if (aw_private.pm.current_level > aw_private.pm.max_available_level)
			dvfs_change(aw_private.pm.max_available_level);
	}

	return 0;
}
EXPORT_SYMBOL(gpu_thermal_cool);
#endif /* CONFIG_SUNXI_GPU_COOLING */

/**
 ****************************************************************
 * Function   : get_u32_from_fex
 * Description: Get a u32 parameter from sys_config.fex.
 * Input      : main_key, second_key.
 * Output     : value or -1.
 ****************************************************************
 */
static int get_u32_from_fex(char *main_key, char *second_key)
{
	u32 value;
#if (LINUX_VERSION_CODE < KERNEL_VERSION(3, 10, 0))
	script_item_u val;
	script_item_value_type_e type = script_get_item(main_key, second_key, &val);

	if (SCIRPT_ITEM_VALUE_TYPE_INT != type)
		goto err_out;
	value = val.val;
#else
	if (of_property_read_u32(aw_private.np_gpu, second_key, &value))
		goto err_out;
#endif

	return value;

err_out:
	MALI_PRINT(("Failed to get [%s, %s] in sys_config.fex!\n", main_key, second_key));
	return -1;
}

/**
 ****************************************************************
 * Function   : get_str_from_fex
 * Description: Get a string parameter from sys_config.fex.
 * Input      : main_key, second_key.
 * Output     : p_str or NULL.
 ****************************************************************
 */
static char *get_str_from_fex(char *main_key, char *second_key)
{
	char *p_str;
#ifdef CONFIG_MALI_DT
	const char *str;
	if (of_property_read_string(aw_private.np_gpu, second_key, &str))
		goto err_out;
	p_str = (char *)str;
#else /* CONFIG_MALI_DT */
	script_item_u str;
	script_item_value_type_e type = script_get_item(main_key, second_key, &str);

	if (SCIRPT_ITEM_VALUE_TYPE_STR != type)
		goto err_out;
	p_str = str.str;
#endif /* CONFIG_MALI_DT */

	return p_str;

err_out:
	MALI_PRINT(("Failed to get [%s, %s] in sys_config.fex!\n", main_key, second_key));
	return NULL;
}

/**
 ****************************************************************
 * Function   : parse_sysconfig_fex
 * Description: Parse GPU parameters from sys_config.fex.
 * Input      : None.
 * Output     : None.
 ****************************************************************
 */
static void parse_sysconfig_fex(void)
{
	char *mainkey = "gpu_mali450_0";
	char second_key[11] = {0};
	int val, i;

	aw_private.pm.regulator_id = get_str_from_fex(mainkey, "regulator_id");

	val = get_u32_from_fex(mainkey, "dvfs_status");
	if (val == 0 || val == 1)
		aw_private.pm.dvfs_status = val;

	val = get_u32_from_fex(mainkey, "temp_ctrl_status");
	if (val == 0 || val == 1)
		aw_private.tempctrl.temp_ctrl_status = val;

	val = get_u32_from_fex(mainkey, "scene_ctrl_status");
	if (val == 0 || val == 1)
		aw_private.pm.scene_ctrl_status = val;

	val = get_u32_from_fex(mainkey, "max_level");
	if (val >= 0)
		aw_private.pm.max_level = val;

	val = get_u32_from_fex(mainkey, "begin_level");
	if (val >= 0 && val <= aw_private.pm.max_level)
		aw_private.pm.begin_level = val;

	for (i = 0; i <= aw_private.pm.max_level; i++) {
		sprintf(second_key, "lv%d_freq", i);
		val = get_u32_from_fex(mainkey, second_key);
		if (val > 0)
			aw_private.pm.vf_table[i].freq = val;

		sprintf(second_key, "lv%d_volt", i);
		val = get_u32_from_fex(mainkey, second_key);
		if (val > 0)
			aw_private.pm.vf_table[i].vol = val;
	}
}

/**
 ****************************************************************
 * Function   : aw_init
 * Description: Init the clocks of gpu.
 * Input      : device, a platform_device pointer.
 * Output     : None.
 ****************************************************************
 */
static void aw_init(struct platform_device *device)
{
	int i;
	struct platform_device *pdev = device;

#ifdef CONFIG_MALI_DT
	aw_private.np_gpu = of_find_compatible_node(NULL, NULL, "arm,mali-450");
#endif /* CONFIG_MALI_DT */

	pdev->dev.coherent_dma_mask = DMA_BIT_MASK(32);
	pdev->dev.dma_mask = &pdev->dev.coherent_dma_mask;

	mali_gpu_data.shared_mem_size = totalram_pages * PAGE_SIZE; /* B */

	parse_sysconfig_fex();

	if (NULL != aw_private.pm.regulator_id) {
		aw_private.pm.regulator = regulator_get(NULL, aw_private.pm.regulator_id);
		if (IS_ERR_OR_NULL(aw_private.pm.regulator))
			MALI_PRINT(("Failed to get regulator!\n"));
	} else {
		MALI_PRINT(("There is no regulator id for GPU.\n"));
	}

	aw_private.pm.current_level = aw_private.pm.begin_level;
	aw_private.pm.max_available_level = aw_private.pm.max_level;

	if (IS_ERR_OR_NULL(aw_private.pm.regulator)) {
		for (i = 0; i <= aw_private.pm.max_level; i++) {
			aw_private.pm.current_vol = -1;
			aw_private.pm.vf_table[i].vol = aw_private.pm.current_vol;
		}
	} else {
		if (aw_private.pm.independent_pow) {
			if (regulator_set_voltage(aw_private.pm.regulator,
								aw_private.pm.vf_table[aw_private.pm.max_level].vol*1000,
								aw_private.pm.vf_table[aw_private.pm.max_level].vol*1000) != 0) {
				MALI_PRINT_ERROR(("Failed to set gpu voltage!\n"));
			} else {
				aw_private.pm.current_vol = regulator_get_voltage(aw_private.pm.regulator)/1000;
				MALI_PRINT(("Set gpu voltage to %dmV.\n", aw_private.pm.current_vol));
			}
		} else {
			for (i = 0; i <= aw_private.pm.max_level; i++) {
				aw_private.pm.current_vol = regulator_get_voltage(aw_private.pm.regulator)/1000;
				aw_private.pm.vf_table[i].vol = aw_private.pm.current_vol;
			}
		}
	}

	enable_gpu_power();

	get_gpu_clk();

	set_freq(aw_private.pm.vf_table[aw_private.pm.begin_level].freq);

	enable_gpu_clk();

	mutex_init(&aw_private.pm.dvfs_lock);

	spin_lock_init(&aw_private.utilization.lock);

	INIT_WORK(&aw_private.pm.dvfs_work, aw_dvfs_queue_work);
}

/**
 ****************************************************************
 * Function   : aw_deinit
 * Description: Init the clocks of gpu.
 * Input      : device, a platform_device pointer.
 * Output     : None.
 ****************************************************************
 */
static void aw_deinit(struct platform_device *device)
{
	disable_gpu_clk();

	put_gpu_clk();

	disable_gpu_power();

	if (!IS_ERR_OR_NULL(aw_private.pm.regulator))
		regulator_put(aw_private.pm.regulator);

#ifdef CONFIG_SUNXI_GPU_COOLING
	gpu_thermal_cool_unregister();
#endif /* CONFIG_SUNXI_GPU_COOLING */
}

/**
 ****************************************************************
 * Function   : mali_platform_device_init/
 *		mali_platform_device_register
 * Description: Init the essential data of gpu.
 * Input      : device, a platform_device pointer/None.
 * Output     : Zero or error number.
 ****************************************************************
 */
#ifdef CONFIG_MALI_DT
int mali_platform_device_init(struct platform_device *device)
#else /* CONFIG_MALI_DT */
int mali_platform_device_register(void)
#endif /* CONFIG_MALI_DT */
{
	int err = 0;
	struct platform_device *pdev;

#ifdef CONFIG_MALI_DT
	pdev = device;
#else /* CONFIG_MALI_DT */
	pdev = &mali_gpu_device;
#endif /* CONFIG_MALI_DT */

	aw_init(pdev);

#ifndef CONFIG_MALI_DT
	err = platform_device_add_resources(&mali_gpu_device,
					mali_gpu_resources,
					sizeof(mali_gpu_resources) / sizeof(mali_gpu_resources[0]));
	if (err) {
		MALI_PRINT_ERROR(("platform_device_add_resources failed!\n"));
		goto out;
	}

	err = platform_device_register(&mali_gpu_device);
	if (err) {
		MALI_PRINT_ERROR(("platform_device_register failed!\n"));
		goto out;
	}
#endif /* CONFIG_MALI_DT */

	err = platform_device_add_data(pdev, &mali_gpu_data, sizeof(mali_gpu_data));
	if (err) {
		MALI_PRINT_ERROR(("platform_device_add_data failed!\n"));
		goto out;
	}

#ifdef CONFIG_PM_RUNTIME
#ifdef CONFIG_DEVFREQ_DRAM_FREQ_WITH_SOFT_NOTIFY
	pm_runtime_set_autosuspend_delay(&(pdev->dev), 1000);
#else /* CONFIG_DEVFREQ_DRAM_FREQ_WITH_SOFT_NOTIFY */
	pm_runtime_set_autosuspend_delay(&(pdev->dev), 2);
#endif /* CONFIG_DEVFREQ_DRAM_FREQ_WITH_SOFT_NOTIFY */
	pm_runtime_use_autosuspend(&(pdev->dev));
	pm_runtime_enable(&(pdev->dev));
#endif /* CONFIG_PM_RUNTIME */

	MALI_PRINT(("Mali GPU initialization finished.\n"));

out:
	return err;
}

/**
 ****************************************************************
 * Function   : mali_platform_device_deinit/
 *		mali_platform_device_unregister
 * Description: Remove the resource gpu used, and it is called
 *		when mali driver is removed.
 * Input      : device, a platform_device pointer/None.
 * Output     : Zero or error number.
 ****************************************************************
 */
#ifdef CONFIG_MALI_DT
int mali_platform_device_deinit(struct platform_device *device)
#else /* CONFIG_MALI_DT */
int mali_platform_device_unregister(void)
#endif /* CONFIG_MALI_DT */
{
	int err = 0;
	struct platform_device *pdev;

#ifdef CONFIG_MALI_DT
	pdev = device;
#else /* CONFIG_MALI_DT */
	pdev = &mali_gpu_device;
#endif /* CONFIG_MALI_DT */

#ifndef CONFIG_MALI_DT
	err = platform_device_register(&mali_gpu_device);
	if (err)
		MALI_PRINT_ERROR(("platform_device_register failed!\n"));
#endif /* CONFIG_MALI_DT */

	aw_deinit(pdev);

	return err;
}

/**
 ****************************************************************
 * Function   : aw_dvfs_queue_work
 * Description: Determine whether need to change GPU frequency.
 * Input      : work, a work_struct pointer.
 * Output     : None.
 ****************************************************************
 */
static void aw_dvfs_queue_work(struct work_struct *work)
{
	int lower_freq;
	int high_util = 192; /* 75% */

	if (aw_private.pm.current_level == -1) {
		dvfs_change(aw_private.pm.max_available_level);
		return;
	}

	/* Determine whether need to increase frequency */
	if (aw_private.utilization.data.utilization_gpu > high_util) {
		if (aw_private.pm.current_level < aw_private.pm.max_available_level) {
			dvfs_change(aw_private.pm.current_level + 1);
		}
		return;
	}

	/* Determine whether need to decrease frequency */
	if (aw_private.pm.current_level > 0) {
		lower_freq = aw_private.pm.vf_table[aw_private.pm.current_level - 1].freq;

		if (lower_freq * high_util >= aw_private.pm.vf_table[aw_private.pm.current_level].freq
					* aw_private.utilization.data.utilization_gpu)
			dvfs_change(aw_private.pm.current_level - 1);
	}
}

/**
 ****************************************************************
 * Function   : mali_gpu_utilization_callback
 * Description: Get Mali GPU utilization and start a queue work.
 * Input      : data, a mali_gpu_utilization_data pointer, which
 *		contains GPU GP and PP utilization.
 * Output     : None.
 ****************************************************************
 */
static void mali_gpu_utilization_callback(struct mali_gpu_utilization_data *data)
{
	unsigned long flags;

	if (aw_private.pm.dvfs_status && !aw_private.pm.scene_ctrl_cmd) {
		spin_lock_irqsave(&aw_private.utilization.lock, flags);
		aw_private.utilization.data = *data;
		spin_unlock_irqrestore(&aw_private.utilization.lock, flags);

		schedule_work(&aw_private.pm.dvfs_work);
	}
}
