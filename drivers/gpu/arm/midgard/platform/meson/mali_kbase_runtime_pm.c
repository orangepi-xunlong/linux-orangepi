/*
 *
 * (C) COPYRIGHT 2015, 2017 ARM Limited. All rights reserved.
 *
 * This program is free software and is provided to you under the terms of the
 * GNU General Public License version 2 as published by the Free Software
 * Foundation, and any use by you of this program is subject to the terms
 * of such GNU licence.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you can access it online at
 * http://www.gnu.org/licenses/gpl-2.0.html.
 *
 * SPDX-License-Identifier: GPL-2.0
 *
 */

#include <mali_kbase.h>
#include <mali_kbase_defs.h>
#include <linux/pm_runtime.h>
#include <linux/reset.h>
#include "mali_kbase_config_platform.h"
#include <backend/gpu/mali_kbase_device_internal.h>
#include <asm/delay.h>

static int first = 1;

/* Manually Power On Cores and L2 Cache */
static int gpu_power_on(struct kbase_device *kbdev, uint32_t  mask)
{
	/* Clear all interrupts */
	kbase_reg_write(kbdev, GPU_CONTROL_REG(GPU_IRQ_CLEAR), GPU_IRQ_REG_ALL);

	/* Power On all Tiler Cores */
	kbase_reg_write(kbdev, GPU_CONTROL_REG(TILER_PWRON_LO), 0xffffffff);
	kbase_reg_write(kbdev, GPU_CONTROL_REG(TILER_PWRON_HI), 0xffffffff);

	/* Power On all L2 cache */
	kbase_reg_write(kbdev, GPU_CONTROL_REG(L2_PWRON_LO), 0xffffffff);
	kbase_reg_write(kbdev, GPU_CONTROL_REG(L2_PWRON_HI), 0xffffffff);

	/* Power On all Shader Cores */
	kbase_reg_write(kbdev, GPU_CONTROL_REG(SHADER_PWRON_LO), 0x3);
	kbase_reg_write(kbdev, GPU_CONTROL_REG(SHADER_PWRON_HI), 0x0);

	/* Wait for Power On to complete */
	udelay(10);
	while (!kbase_reg_read(kbdev, GPU_CONTROL_REG(GPU_IRQ_RAWSTAT)))
		udelay(10);

	/* Clear all interrupts */
	kbase_reg_write(kbdev, GPU_CONTROL_REG(GPU_IRQ_CLEAR), GPU_IRQ_REG_ALL);

	return 0;
}

static int pm_soft_reset(struct kbase_device *kbdev)
{
	struct reset_control *rstc;

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 2, 0)
	rstc = of_reset_control_array_get(kbdev->dev->of_node, false, false, true);
#else
	rstc = of_reset_control_array_get(kbdev->dev->of_node, false, false);
#endif
	if (!IS_ERR(rstc)) {
		reset_control_assert(rstc);
		udelay(10);
		reset_control_deassert(rstc);
		udelay(10);

		reset_control_put(rstc);

		/* Override Power Management Settings */
		kbase_reg_write(kbdev, GPU_CONTROL_REG(PWR_KEY),
				0x2968A819);
		kbase_reg_write(kbdev, GPU_CONTROL_REG(PWR_OVERRIDE1),
				0xfff | (0x20 << 16));
	} else
		return PTR_ERR(rstc);

	return 0;
}

static int pm_callback_power_on(struct kbase_device *kbdev)
{
	int ret = 1; /* Assume GPU has been powered off */
	int error;

	dev_dbg(kbdev->dev, "pm_callback_power_on %p\n",
			(void *)kbdev->dev->pm_domain);

	if (first) {
		pm_soft_reset(kbdev);

		gpu_power_on(kbdev, 7);

		first = 0;
	}

	error = pm_runtime_get_sync(kbdev->dev);
	if (error == 1) {
		/*
		 * Let core know that the chip has not been
		 * powered off, so we can save on re-initialization.
		 */
		ret = 0;
	}

	dev_dbg(kbdev->dev, "pm_runtime_get_sync returned %d\n", error);

	return ret;
}

static void pm_callback_power_off(struct kbase_device *kbdev)
{
	dev_dbg(kbdev->dev, "pm_callback_power_off\n");

	pm_runtime_mark_last_busy(kbdev->dev);
	pm_runtime_put_autosuspend(kbdev->dev);
}

#ifdef KBASE_PM_RUNTIME
static int kbase_device_runtime_init(struct kbase_device *kbdev)
{
	int ret = 0;

	dev_dbg(kbdev->dev, "kbase_device_runtime_init\n");

	pm_runtime_set_autosuspend_delay(kbdev->dev, AUTO_SUSPEND_DELAY);
	pm_runtime_use_autosuspend(kbdev->dev);

	pm_runtime_set_active(kbdev->dev);
	pm_runtime_enable(kbdev->dev);

	if (!pm_runtime_enabled(kbdev->dev)) {
		dev_warn(kbdev->dev, "pm_runtime not enabled");
		ret = -ENOSYS;
	}

	return ret;
}

static void kbase_device_runtime_disable(struct kbase_device *kbdev)
{
	dev_dbg(kbdev->dev, "kbase_device_runtime_disable\n");

	pm_runtime_dont_use_autosuspend(kbdev->dev);
	pm_runtime_disable(kbdev->dev);
}
#endif

static int pm_clk_enable(struct kbase_device *kbdev)
{
	dev_dbg(kbdev->dev, "pm_clk_enable\n");

	if (!kbdev->clock)
		return 0;

	return clk_enable(kbdev->clock);
}

static void pm_clk_disable(struct kbase_device *kbdev)
{
	dev_dbg(kbdev->dev, "pm_clk_disable\n");

	if (!kbdev->clock)
		return;

	clk_disable(kbdev->clock);
}

static int pm_callback_runtime_on(struct kbase_device *kbdev)
{
	int ret;

	dev_dbg(kbdev->dev, "pm_callback_runtime_on\n");

	ret = pm_clk_enable(kbdev);
	if (ret) {
		dev_err(kbdev->dev, "failed to enable clk: %d\n", ret);
		return ret;
	}

	return 0;
}

static void pm_callback_runtime_off(struct kbase_device *kbdev)
{
	dev_dbg(kbdev->dev, "pm_callback_runtime_off\n");

	pm_clk_disable(kbdev);
}

static void pm_callback_resume(struct kbase_device *kbdev)
{
	int ret = pm_callback_runtime_on(kbdev);

	WARN_ON(ret);
}

static void pm_callback_suspend(struct kbase_device *kbdev)
{
	pm_callback_runtime_off(kbdev);
}

struct kbase_pm_callback_conf pm_callbacks = {
	.power_on_callback = pm_callback_power_on,
	.power_off_callback = pm_callback_power_off,
	.power_suspend_callback = pm_callback_suspend,
	.power_resume_callback = pm_callback_resume,
	.soft_reset_callback = pm_soft_reset,
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


