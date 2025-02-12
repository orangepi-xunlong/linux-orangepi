// SPDX-License-Identifier: GPL-2.0-only

#include <linux/of.h>
#include <linux/errno.h>
#include <linux/init.h>
#include <linux/input.h>
#include <linux/interrupt.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/pm_wakeirq.h>
#include <linux/notifier.h>
#include <linux/spinlock.h>
#include <linux/suspend.h>
#include <linux/mfd/ky/ky_pmic.h>

static int report_event, fall_triggered;
static struct notifier_block   pm_notify;
static spinlock_t pm_lock;

static irqreturn_t pwrkey_fall_irq(int irq, void *_pwr)
{
	unsigned long flags;
	struct input_dev *pwr = _pwr;

	spin_lock_irqsave(&pm_lock, flags);
	if (report_event) {
		input_report_key(pwr, KEY_POWER, 1);
		input_sync(pwr);
		fall_triggered = 1;
	}

	pm_wakeup_event(pwr->dev.parent, 0);

	spin_unlock_irqrestore(&pm_lock, flags);

	return IRQ_HANDLED;
}

static irqreturn_t pwrkey_rise_irq(int irq, void *_pwr)
{
	unsigned long flags;
	struct input_dev *pwr = _pwr;

	spin_lock_irqsave(&pm_lock, flags);
	/* report key up if key down has been reported */
	if (fall_triggered) {
		input_report_key(pwr, KEY_POWER, 0);
		input_sync(pwr);
		fall_triggered = 0;
	}

	pm_wakeup_event(pwr->dev.parent, 0);

	spin_unlock_irqrestore(&pm_lock, flags);

	return IRQ_HANDLED;
}

static irqreturn_t pwrkey_skey_irq(int irq, void *_pwr)
{
	/* do nothing by now */
	return IRQ_HANDLED;
}

static irqreturn_t pwrkey_lkey_irq(int irq, void *_pwr)
{
	/* do nothing by now */
	return IRQ_HANDLED;
}

static int pwrk_pm_notify(struct notifier_block *notify_block,
			unsigned long mode, void *unused)
{
	unsigned long flags;

	spin_lock_irqsave(&pm_lock, flags);

	switch (mode) {
	case PM_SUSPEND_PREPARE:
		/* don't report power-key when enter suspend */
		report_event = 0;
		break;

	case PM_POST_SUSPEND:
		/* restore report power-key */
		report_event = 1;
		break;
	}

	spin_unlock_irqrestore(&pm_lock, flags);

	return 0;
}

static int ky_pwrkey_probe(struct platform_device *pdev)
{
	int err;
	struct input_dev *pwr;
	int rise_irq, fall_irq, s_key_irq, l_key_irq;

	report_event = 1;
	fall_triggered = 0;

	pwr = devm_input_allocate_device(&pdev->dev);
	if (!pwr) {
		dev_err(&pdev->dev, "Can't allocate power button\n");
		return -ENOMEM;
	}

	pwr->name = "ky pwrkey";
	pwr->phys = "ky_pwrkey/input0";
	pwr->id.bustype = BUS_HOST;
	input_set_capability(pwr, EV_KEY, KEY_POWER);

	rise_irq = platform_get_irq(pdev, 0);
	if (rise_irq < 0)
		return rise_irq;

	fall_irq = platform_get_irq(pdev, 1);
	if (fall_irq < 0)
		return fall_irq;

	s_key_irq = platform_get_irq(pdev, 2);
	if (s_key_irq < 0)
		return s_key_irq;

	l_key_irq = platform_get_irq(pdev, 3);
	if (l_key_irq < 0)
		return l_key_irq;

	err = devm_request_any_context_irq(&pwr->dev, rise_irq,
					   pwrkey_rise_irq,
					   IRQF_TRIGGER_NONE | IRQF_ONESHOT,
					   "ky_pwrkey_rise", pwr);
	if (err < 0) {
		dev_err(&pdev->dev, "Can't register rise irq: %d\n", err);
		return err;
	}

	err = devm_request_any_context_irq(&pwr->dev, fall_irq,
					   pwrkey_fall_irq,
					   IRQF_TRIGGER_NONE | IRQF_ONESHOT,
					   "ky_pwrkey_fall", pwr);
	if (err < 0) {
		dev_err(&pdev->dev, "Can't register fall irq: %d\n", err);
		return err;
	}

 	err = devm_request_any_context_irq(&pwr->dev, s_key_irq,
 					   pwrkey_skey_irq,
 					   IRQF_TRIGGER_NONE | IRQF_ONESHOT,
 					   "ky_pwrkey_skey", pwr);
 	if (err < 0) {
 		dev_err(&pdev->dev, "Can't register skey irq: %d\n", err);
 		return err;
 	}
 
 	err = devm_request_any_context_irq(&pwr->dev, l_key_irq,
 					   pwrkey_lkey_irq,
 					   IRQF_TRIGGER_NONE | IRQF_ONESHOT,
 					   "ky_pwrkey_lkey", pwr);
 	if (err < 0) {
 		dev_err(&pdev->dev, "Can't register lkey irq: %d\n", err);
 		return err;
 	}
 
	err = input_register_device(pwr);
	if (err) {
		dev_err(&pdev->dev, "Can't register power button: %d\n", err);
		return err;
	}

	platform_set_drvdata(pdev, pwr);
	dev_pm_set_wake_irq(&pdev->dev, fall_irq);
	device_init_wakeup(&pdev->dev, true);

	spin_lock_init(&pm_lock);

	pm_notify.notifier_call = pwrk_pm_notify;
	err = register_pm_notifier(&pm_notify);
	if (err) {
		dev_err(&pdev->dev, "Register pm notifier failed\n");
		return err;
	}

	return 0;
}

static const struct of_device_id ky_pwr_key_id_table[] = {
	{ .compatible = "pmic,pwrkey,spm8821", },
	{ }
};
MODULE_DEVICE_TABLE(of, ky_pwr_key_id_table);

static struct platform_driver ky_pwrkey_driver = {
	.probe = ky_pwrkey_probe,
	.driver = {
		.name = "ky-pmic-pwrkey",
		.of_match_table = of_match_ptr(ky_pwr_key_id_table),
	},
};
module_platform_driver(ky_pwrkey_driver);

MODULE_DESCRIPTION("KY Power Key driver");
MODULE_LICENSE("GPL v2");
