// SPDX-License-Identifier: GPL-2.0
/*
 * simple driver for PWM (Pulse Width Modulator) controller
 *
 */

#include <linux/bitfield.h>
#include <linux/bitops.h>
#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/err.h>
#include <linux/io.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/pwm.h>
#include <linux/screen_info.h>
#include <linux/slab.h>
#include <linux/acpi.h>
#include <linux/reset.h>
#include <linux/pm_runtime.h>
#include <linux/pm.h>
#include <linux/pinctrl/consumer.h>

/*pwm registers */
#define TCTL            0x0024 /*Control register for PWM*/
#define PWMH_RW		0x0028 /*Contains PWM_HIGH_TIME value to be loaded to Timer*/
#define PWMPRD_RW	0x002C /*Contains PWM_PERIOD value to be loaded to Timer*/

/*timer function mode */
#define PWM_MODE      0x3 /*PWM mode*/

/*TCTL register */
#define ENABLE        (1 << 17) /*Enable PWM (This bit should be set finally)*/
#define ENPWM	      (1 << 14) /*Enable PWM output*/

struct pwm_sky1_chip {
	struct clk	*pclk;
	struct clk	*tclk;
	void __iomem	*mmio_base;
	struct reset_control *func_reset;
	struct pwm_chip	chip;
};

#define to_pwm_sky1_chip(chip)	container_of(chip, struct pwm_sky1_chip, chip)

static int pwm_sky1_clk_prepare_enable(struct pwm_sky1_chip *sky1)
{
	int ret;

	ret = clk_prepare_enable(sky1->pclk);
	if (ret)
		return ret;

	ret = clk_prepare_enable(sky1->tclk);
	if (ret) {
		clk_disable_unprepare(sky1->pclk);
		return ret;
	}

	return 0;
}

static void pwm_sky1_clk_disable_unprepare(struct pwm_sky1_chip *sky1)
{
	clk_disable_unprepare(sky1->pclk);
	clk_disable_unprepare(sky1->tclk);
}

static int pwm_sky1_get_state(struct pwm_chip *chip,
			       struct pwm_device *pwm, struct pwm_state *state)
{
	struct pwm_sky1_chip *sky1 = to_pwm_sky1_chip(chip);
	u32 val, period;
	u64 tmp, clkrate;
	int ret;

	ret = pwm_sky1_clk_prepare_enable(sky1);
	if (ret < 0)
		return ret;

	val = readl(sky1->mmio_base + TCTL);

	if (val & (ENABLE|ENPWM))
		state->enabled = true;
	else
		state->enabled = false;

	clkrate = clk_get_rate(sky1->tclk);
	period = readl(sky1->mmio_base + PWMPRD_RW);
	tmp = NSEC_PER_SEC * (u64)period;
	state->period = DIV_ROUND_UP_ULL(tmp, clkrate);

	val = readl(sky1->mmio_base + PWMH_RW);
	tmp = NSEC_PER_SEC * (u64)val;
	state->duty_cycle = DIV_ROUND_UP_ULL(tmp, clkrate);

	pwm_sky1_clk_disable_unprepare(sky1);

	return 0;
}

static int pwm_sky1_apply(struct pwm_chip *chip, struct pwm_device *pwm,
			   const struct pwm_state *state)
{
	unsigned long period_cycles, duty_cycles, c, clkrate;
	struct pwm_sky1_chip *sky1 = to_pwm_sky1_chip(chip);
	struct pwm_state cstate;
	u32 cr;
	int ret;
	pwm_get_state(pwm, &cstate);

	ret = pwm_sky1_clk_prepare_enable(sky1);
	if (ret)
		return ret;

	cr = readl(sky1->mmio_base + TCTL);
	/* Confirm configure timer to pwm module */
	cr |= PWM_MODE;
	writel(cr, sky1->mmio_base + TCTL);

	clkrate = clk_get_rate(sky1->tclk);
	c = clkrate * state->period;
	do_div(c, NSEC_PER_SEC);
	period_cycles = c;

	c = clkrate * state->duty_cycle;
	do_div(c, NSEC_PER_SEC);
	duty_cycles = c;

	writel(duty_cycles, sky1->mmio_base + PWMH_RW);
	writel(period_cycles, sky1->mmio_base + PWMPRD_RW);

	if (state->enabled)
		cr |= (ENPWM|ENABLE);

	writel(cr, sky1->mmio_base + TCTL);

	if (!state->enabled) {
		/* disable pwm for configure */
		cr &= ~(ENPWM|ENABLE);
		writel(cr, sky1->mmio_base + TCTL);
		pwm_sky1_clk_disable_unprepare(sky1);
	}

	return 0;
}

static const struct pwm_ops pwm_sky1_ops = {
	.apply = pwm_sky1_apply,
	.get_state = pwm_sky1_get_state,
	.owner = THIS_MODULE,
};

/**
 * sky1_pwm_suspend - Suspend method for the I2C driver
 * @dev:	Address of the pwm device structure
 *
 * This function disables the pwm device and changes
 * the driver state to "suspend"
 *
 * Return:	0 on sucess and error value on error
 *
 */
static int __maybe_unused sky1_pwm_suspend(struct device *dev)
{
	struct pwm_state cstate;
	struct pwm_sky1_chip *sky1 = dev_get_drvdata(dev);
	struct pwm_device *pwm = &(sky1->chip.pwms[0]);

	pwm_get_state(pwm, &cstate);

	if (cstate.enabled)
		pwm_sky1_clk_disable_unprepare(sky1);

	pinctrl_pm_select_sleep_state(dev);

	return 0;
}

/**
 * sky1_pwm_resume - Resume method for the pwm driver
 * @dev:	Address of the pwm device structure
 *
 * This function changes the driver state to "ready"
 *
 * Return: 0 on success and error vlaue on error
 */

static int __maybe_unused sky1_pwm_resume(struct device *dev)
{
	int ret;
	struct pwm_state cstate;
	struct pwm_sky1_chip *sky1 = dev_get_drvdata(dev);
	struct pwm_device *pwm = &(sky1->chip.pwms[0]);
	pwm_get_state(pwm, &cstate);

	if (cstate.enabled) {
		ret = pwm_sky1_clk_prepare_enable(sky1);
		if (ret) {
			dev_err(dev, "Cannot enable clock.\n");
			return ret;
		}
	}

	pinctrl_select_default_state(dev);

	return 0;
}

static const struct dev_pm_ops sky1_pwm_dev_ops = {
	SET_SYSTEM_SLEEP_PM_OPS(sky1_pwm_suspend, sky1_pwm_resume)
};

static const struct of_device_id pwm_sky1_dt_ids[] = {
	{ .compatible = "cix,sky1-pwm", },
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, pwm_sky1_dt_ids);

static const struct acpi_device_id pwm_sky1_acpi_match[] = {
	{ "CIXH2011", 0 },
	{ },
};
MODULE_DEVICE_TABLE(acpi, pwm_sky1_acpi_match);

static int pwm_sky1_probe(struct platform_device *pdev)
{
	struct pwm_sky1_chip *sky1;
	int ret;
	u32 pwmcr;

	sky1 = devm_kzalloc(&pdev->dev, sizeof(*sky1), GFP_KERNEL);
	if (sky1 == NULL)
		return -ENOMEM;

	platform_set_drvdata(pdev, sky1);
	sky1->pclk = devm_clk_get(&pdev->dev, "fch_pwm_apb_clk");
	if (IS_ERR(sky1->pclk)) {
		int ret = PTR_ERR(sky1->pclk);

		if (ret != -EPROBE_DEFER)
			dev_err(&pdev->dev,
				"getting clock failed with %d\n",
				ret);
		return ret;
	}

	sky1->tclk = devm_clk_get(&pdev->dev, "fch_pwm_func_clk");
	if (IS_ERR(sky1->tclk)) {
		int ret = PTR_ERR(sky1->tclk);

		if (ret != -EPROBE_DEFER)
			dev_err(&pdev->dev,
				"getting clock failed with %d\n",
				ret);
		return ret;
	}

	sky1->func_reset = devm_reset_control_get_optional_shared(&pdev->dev, "func_reset");
	if (IS_ERR(sky1->func_reset)) {
		dev_err(&pdev->dev, "[%s:%d]get reset error\n", __func__, __LINE__);
		ret = PTR_ERR(sky1->func_reset);
		goto err_clk_dis;
	}

	sky1->chip.ops = &pwm_sky1_ops;
	sky1->chip.dev = &pdev->dev;
	sky1->chip.base = -1;
	sky1->chip.npwm = 1;

	sky1->chip.of_xlate = of_pwm_xlate_with_flags;
	sky1->chip.of_pwm_n_cells = 2;

	sky1->mmio_base = devm_platform_ioremap_resource(pdev, 0);
	if (IS_ERR(sky1->mmio_base))
		return PTR_ERR(sky1->mmio_base);

	ret = pwm_sky1_clk_prepare_enable(sky1);
	if (ret)
		return ret;

	pwmcr = readl(sky1->mmio_base + TCTL);
	if (screen_info.lfb_linelength && (pwmcr & (PWM_MODE|ENABLE|ENPWM)))
		return pwmchip_add(&sky1->chip); /* already init in uefi */

	/* reset pwm */
	if (!screen_info.lfb_linelength)
		reset_control_reset(sky1->func_reset);

	/* Configure timer to pwm module */
	writel(PWM_MODE, sky1->mmio_base + TCTL);

	/* keep clks on if pwm is running */
	if (!(pwmcr & (ENABLE|ENPWM)))
		pwm_sky1_clk_disable_unprepare(sky1);

	return pwmchip_add(&sky1->chip);

err_clk_dis:
	pwm_sky1_clk_disable_unprepare(sky1);
	return ret;
}

static int pwm_sky1_remove(struct platform_device *pdev)
{
	struct pwm_sky1_chip *sky1;

	sky1 = platform_get_drvdata(pdev);
	pwmchip_remove(&sky1->chip);

	return 0;
}

static struct platform_driver sky1_pwm_driver = {
	.driver = {
		.name = "pwm-sky1",
		.of_match_table = pwm_sky1_dt_ids,
		.acpi_match_table = ACPI_PTR(pwm_sky1_acpi_match),
		.pm = &sky1_pwm_dev_ops,
	},
	.probe = pwm_sky1_probe,
	.remove = pwm_sky1_remove,
};
module_platform_driver(sky1_pwm_driver);

MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("Jerry Zhu <jerry.zhu@cixtech.com>");
