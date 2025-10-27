// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) 2025 Collabora Ltd.
 *
 * A driver to manage all the different functionalities exposed by Rockchip's
 * PWMv4 hardware.
 *
 * This driver is chiefly focused on guaranteeing non-concurrent operation
 * between the different device functions, as well as setting the clocks.
 * It registers the device function platform devices, e.g. PWM output or
 * PWM capture.
 *
 * Authors:
 *     Nicolas Frattaroli <nicolas.frattaroli@collabora.com>
 */

#include <linux/array_size.h>
#include <linux/clk.h>
#include <linux/clk-provider.h>
#include <linux/mfd/core.h>
#include <linux/mfd/rockchip-mfpwm.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/overflow.h>
#include <linux/platform_device.h>
#include <linux/spinlock.h>

/**
 * struct rockchip_mfpwm - private mfpwm driver instance state struct
 * @pdev: pointer to this instance's &struct platform_device
 * @base: pointer to the memory mapped registers of this device
 * @pwm_clk: pointer to the PLL clock the PWM signal may be derived from
 * @osc_clk: pointer to the fixed crystal the PWM signal may be derived from
 * @rc_clk: pointer to the RC oscillator the PWM signal may be derived from
 * @chosen_clk: a clk-mux of pwm_clk, osc_clk and rc_clk
 * @pclk: pointer to the APB bus clock needed for mmio register access
 * @active_func: pointer to the currently active device function, or %NULL if no
 *               device function is currently actively using any of the shared
 *               resources. May only be checked/modified with @state_lock held.
 * @acquire_cnt: number of times @active_func has currently mfpwm_acquire()'d
 *               it. Must only be checked or modified while holding @state_lock.
 * @state_lock: this lock is held while either the active device function, the
 *              enable register, or the chosen clock is being changed.
 * @irq: the IRQ number of this device
 */
struct rockchip_mfpwm {
	struct platform_device *pdev;
	void __iomem *base;
	struct clk *pwm_clk;
	struct clk *osc_clk;
	struct clk *rc_clk;
	struct clk *chosen_clk;
	struct clk *pclk;
	struct rockchip_mfpwm_func *active_func;
	unsigned int acquire_cnt;
	spinlock_t state_lock;
	int irq;
};

static atomic_t subdev_id = ATOMIC_INIT(0);

static inline struct rockchip_mfpwm *to_rockchip_mfpwm(struct platform_device *pdev)
{
	return platform_get_drvdata(pdev);
}

static int mfpwm_check_pwmf(const struct rockchip_mfpwm_func *pwmf,
			    const char *fname)
{
	struct device *dev = &pwmf->parent->pdev->dev;

	if (IS_ERR_OR_NULL(pwmf)) {
		dev_warn(dev, "called %s with an erroneous handle, no effect\n",
			 fname);
		return -EINVAL;
	}

	if (IS_ERR_OR_NULL(pwmf->parent)) {
		dev_warn(dev, "called %s with an erroneous mfpwm_func parent, no effect\n",
			 fname);
		return -EINVAL;
	}

	return 0;
}

__attribute__((nonnull))
static int mfpwm_do_acquire(struct rockchip_mfpwm_func *pwmf)
{
	struct rockchip_mfpwm *mfpwm = pwmf->parent;
	unsigned int cnt;

	if (mfpwm->active_func && pwmf->id != mfpwm->active_func->id)
		return -EBUSY;

	if (!mfpwm->active_func)
		mfpwm->active_func = pwmf;

	if (!check_add_overflow(mfpwm->acquire_cnt, 1, &cnt)) {
		mfpwm->acquire_cnt = cnt;
	} else {
		dev_warn(&mfpwm->pdev->dev, "prevented acquire counter overflow in %s\n",
			 __func__);
		return -EOVERFLOW;
	}

	dev_dbg(&mfpwm->pdev->dev, "%d acquired mfpwm, acquires now at %u\n",
		pwmf->id, mfpwm->acquire_cnt);

	return clk_enable(mfpwm->pclk);
}

int mfpwm_acquire(struct rockchip_mfpwm_func *pwmf)
{
	struct rockchip_mfpwm *mfpwm;
	unsigned long flags;
	int ret = 0;

	ret = mfpwm_check_pwmf(pwmf, "mfpwm_acquire");
	if (ret)
		return ret;

	mfpwm = pwmf->parent;
	dev_dbg(&mfpwm->pdev->dev, "%d is attempting to acquire\n", pwmf->id);

	if (!spin_trylock_irqsave(&mfpwm->state_lock, flags))
		return -EBUSY;

	ret = mfpwm_do_acquire(pwmf);

	spin_unlock_irqrestore(&mfpwm->state_lock, flags);

	return ret;
}
EXPORT_SYMBOL_NS_GPL(mfpwm_acquire, "ROCKCHIP_MFPWM");

__attribute__((nonnull))
static void mfpwm_do_release(const struct rockchip_mfpwm_func *pwmf)
{
	struct rockchip_mfpwm *mfpwm = pwmf->parent;

	if (!mfpwm->active_func)
		return;

	if (mfpwm->active_func->id != pwmf->id)
		return;

	/*
	 * No need to check_sub_overflow here, !mfpwm->active_func above catches
	 * this type of problem already.
	 */
	mfpwm->acquire_cnt--;

	if (!mfpwm->acquire_cnt)
		mfpwm->active_func = NULL;

	clk_disable(mfpwm->pclk);
}

void mfpwm_release(const struct rockchip_mfpwm_func *pwmf)
{
	struct rockchip_mfpwm *mfpwm;
	unsigned long flags;

	if (mfpwm_check_pwmf(pwmf, "mfpwm_release"))
		return;

	mfpwm = pwmf->parent;

	spin_lock_irqsave(&mfpwm->state_lock, flags);
	mfpwm_do_release(pwmf);
	dev_dbg(&mfpwm->pdev->dev, "%d released mfpwm, acquires now at %u\n",
		pwmf->id, mfpwm->acquire_cnt);
	spin_unlock_irqrestore(&mfpwm->state_lock, flags);
}
EXPORT_SYMBOL_NS_GPL(mfpwm_release, "ROCKCHIP_MFPWM");

/**
 * mfpwm_register_subdev - register a single mfpwm_func
 * @mfpwm: pointer to the parent &struct rockchip_mfpwm
 * @name: sub-device name string
 *
 * Allocate a single &struct mfpwm_func, fill its members with appropriate data,
 * and register a new mfd cell.
 *
 * Returns: 0 on success, negative errno on error
 */
static int mfpwm_register_subdev(struct rockchip_mfpwm *mfpwm,
				 const char *name)
{
	struct rockchip_mfpwm_func *func;
	struct mfd_cell cell = {};

	func = devm_kzalloc(&mfpwm->pdev->dev, sizeof(*func), GFP_KERNEL);
	if (IS_ERR(func))
		return PTR_ERR(func);
	func->irq = mfpwm->irq;
	func->parent = mfpwm;
	func->id = atomic_inc_return(&subdev_id);
	func->base = mfpwm->base;
	func->core = mfpwm->chosen_clk;
	cell.name = name;
	cell.platform_data = func;
	cell.pdata_size = sizeof(*func);
	// cell.ignore_resource_conflicts = true;
	// cell.resources = mfpwm->pdev->resource;
	// cell.num_resources = mfpwm->pdev->num_resources;

	return devm_mfd_add_devices(&mfpwm->pdev->dev, func->id, &cell, 1, NULL,
				    0, NULL);
}

static int mfpwm_register_subdevs(struct rockchip_mfpwm *mfpwm)
{
	int ret;

	ret = mfpwm_register_subdev(mfpwm, "pwm-rockchip-v4");
	if (ret)
		return ret;

	ret = mfpwm_register_subdev(mfpwm, "rockchip-pwm-capture");
	if (ret)
		return ret;

	return 0;
}

static int rockchip_mfpwm_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct rockchip_mfpwm *mfpwm;
	char *clk_mux_name;
	const char *mux_p_names[3];
	int ret = 0;

	mfpwm = devm_kzalloc(&pdev->dev, sizeof(*mfpwm), GFP_KERNEL);
	if (IS_ERR(mfpwm))
		return PTR_ERR(mfpwm);

	mfpwm->pdev = pdev;

	spin_lock_init(&mfpwm->state_lock);

	mfpwm->base = devm_platform_ioremap_resource(pdev, 0);
	if (IS_ERR(mfpwm->base))
		return dev_err_probe(dev, PTR_ERR(mfpwm->base),
				     "failed to ioremap address\n");

	mfpwm->pclk = devm_clk_get_prepared(dev, "pclk");
	if (IS_ERR(mfpwm->pclk))
		return dev_err_probe(dev, PTR_ERR(mfpwm->pclk),
				     "couldn't get and prepare 'pclk' clock\n");

	mfpwm->irq = platform_get_irq(pdev, 0);
	if (mfpwm->irq < 0)
		return dev_err_probe(dev, mfpwm->irq, "couldn't get irq 0\n");

	mfpwm->pwm_clk = devm_clk_get_prepared(dev, "pwm");
	if (IS_ERR(mfpwm->pwm_clk))
		return dev_err_probe(dev, PTR_ERR(mfpwm->pwm_clk),
				     "couldn't get and prepare 'pwm' clock\n");

	mfpwm->osc_clk = devm_clk_get_prepared(dev, "osc");
	if (IS_ERR(mfpwm->osc_clk))
		return dev_err_probe(dev, PTR_ERR(mfpwm->osc_clk),
				     "couldn't get and prepare 'osc' clock\n");

	mfpwm->rc_clk = devm_clk_get_prepared(dev, "rc");
	if (IS_ERR(mfpwm->rc_clk))
		return dev_err_probe(dev, PTR_ERR(mfpwm->rc_clk),
				     "couldn't get and prepare 'rc' clock\n");

	clk_mux_name = devm_kasprintf(dev, GFP_KERNEL, "%s_chosen", dev_name(dev));
	if (!clk_mux_name)
		return -ENOMEM;

	mux_p_names[0] = __clk_get_name(mfpwm->pwm_clk);
	mux_p_names[1] = __clk_get_name(mfpwm->osc_clk);
	mux_p_names[2] = __clk_get_name(mfpwm->rc_clk);
	mfpwm->chosen_clk = clk_register_mux(dev, clk_mux_name, mux_p_names,
					     ARRAY_SIZE(mux_p_names),
					     CLK_SET_RATE_PARENT,
					     mfpwm->base + PWMV4_REG_CLK_CTRL,
					     PWMV4_CLK_SRC_SHIFT, PWMV4_CLK_SRC_WIDTH,
					     CLK_MUX_HIWORD_MASK, NULL);
	ret = clk_prepare(mfpwm->chosen_clk);
	if (ret) {
		dev_err(dev, "failed to prepare PWM clock mux: %pe\n",
			ERR_PTR(ret));
		return ret;
	}

	platform_set_drvdata(pdev, mfpwm);

	ret = mfpwm_register_subdevs(mfpwm);
	if (ret) {
		dev_err(dev, "failed to register sub-devices: %pe\n",
			ERR_PTR(ret));
		return ret;
	}

	return ret;
}

static void rockchip_mfpwm_remove(struct platform_device *pdev)
{
	struct rockchip_mfpwm *mfpwm = to_rockchip_mfpwm(pdev);
	unsigned long flags;

	spin_lock_irqsave(&mfpwm->state_lock, flags);

	if (mfpwm->chosen_clk) {
		clk_unprepare(mfpwm->chosen_clk);
		clk_unregister_mux(mfpwm->chosen_clk);
	}

	spin_unlock_irqrestore(&mfpwm->state_lock, flags);
}

static const struct of_device_id rockchip_mfpwm_of_match[] = {
	{
		.compatible = "rockchip,rk3576-pwm",
	},
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, rockchip_mfpwm_of_match);

static struct platform_driver rockchip_mfpwm_driver = {
	.driver = {
		.name = KBUILD_MODNAME,
		.of_match_table = rockchip_mfpwm_of_match,
	},
	.probe = rockchip_mfpwm_probe,
	.remove = rockchip_mfpwm_remove,
};
module_platform_driver(rockchip_mfpwm_driver);

MODULE_AUTHOR("Nicolas Frattaroli <nicolas.frattaroli@collabora.com>");
MODULE_DESCRIPTION("Rockchip MFPWM Driver");
MODULE_LICENSE("GPL");
