// SPDX-License-Identifier: GPL-2.0-only
#include <linux/of.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/rtc.h>
#include <linux/platform_device.h>
#include <linux/pm.h>
#include <linux/pm_wakeirq.h>
#include <linux/of_device.h>
#include <linux/regmap.h>
#include <linux/slab.h>
#include <linux/mfd/ky/ky_pmic.h>

SPM8821_RTC_REG_DESC;

struct ky_rtc {
	int irq;
	struct device *dev;
	struct regmap *regmap;
	struct rtc_device *rtc_dev;
	struct rtc_regdesc *desc;
};

static const struct of_device_id ky_id_table[] = {
	{ .compatible = "pmic,rtc,spm8821", .data = &spm8821_regdesc, },
	{ },
};
MODULE_DEVICE_TABLE(of, ky_id_table);

static irqreturn_t spt_rtc_irq(int irq, void *_pwr)
{
	struct ky_rtc *rtc = (struct ky_rtc *)_pwr;

	rtc_update_irq(rtc->rtc_dev, 1, RTC_IRQF | RTC_AF);

	return IRQ_HANDLED;
}

static int spt_rtc_read_time(struct device *dev, struct rtc_time *tm)
{
	int ret;
	unsigned int v[6], pre_v[6] = {0};
	struct ky_rtc *rtc = dev_get_drvdata(dev);

	ret = regmap_read(rtc->regmap, rtc->desc->cnt_s.reg, &pre_v[0]);
	if (ret) {
		dev_err(rtc->dev, "failed to read second: %d\n", ret);
		return -EINVAL;
	}

	ret = regmap_read(rtc->regmap, rtc->desc->cnt_mi.reg, &pre_v[1]);
	if (ret) {
		dev_err(rtc->dev, "failed to read minute: %d\n", ret);
		return -EINVAL;
	}

	ret = regmap_read(rtc->regmap, rtc->desc->cnt_h.reg, &pre_v[2]);
	if (ret) {
		dev_err(rtc->dev, "failed to read hour: %d\n", ret);
		return -EINVAL;
	}

	ret = regmap_read(rtc->regmap, rtc->desc->cnt_d.reg, &pre_v[3]);
	if (ret) {
		dev_err(rtc->dev, "failed to read day: %d\n", ret);
		return -EINVAL;
	}

	ret = regmap_read(rtc->regmap, rtc->desc->cnt_mo.reg, &pre_v[4]);
	if (ret) {
		dev_err(rtc->dev, "failed to read month: %d\n", ret);
		return -EINVAL;
	}

	ret = regmap_read(rtc->regmap, rtc->desc->cnt_y.reg, &pre_v[5]);
	if (ret) {
		dev_err(rtc->dev, "failed to read year: %d\n", ret);
		return -EINVAL;
	}

	while (1) {
		ret = regmap_read(rtc->regmap, rtc->desc->cnt_s.reg, &v[0]);
		if (ret) {
			dev_err(rtc->dev, "failed to read second: %d\n", ret);
			return -EINVAL;
		}

		ret = regmap_read(rtc->regmap, rtc->desc->cnt_mi.reg, &v[1]);
		if (ret) {
			dev_err(rtc->dev, "failed to read minute: %d\n", ret);
			return -EINVAL;
		}

		ret = regmap_read(rtc->regmap, rtc->desc->cnt_h.reg, &v[2]);
		if (ret) {
			dev_err(rtc->dev, "failed to read hour: %d\n", ret);
			return -EINVAL;
		}

		ret = regmap_read(rtc->regmap, rtc->desc->cnt_d.reg, &v[3]);
		if (ret) {
			dev_err(rtc->dev, "failed to read day: %d\n", ret);
			return -EINVAL;
		}

		ret = regmap_read(rtc->regmap, rtc->desc->cnt_mo.reg, &v[4]);
		if (ret) {
			dev_err(rtc->dev, "failed to read month: %d\n", ret);
			return -EINVAL;
		}

		ret = regmap_read(rtc->regmap, rtc->desc->cnt_y.reg, &v[5]);
		if (ret) {
			dev_err(rtc->dev, "failed to read year: %d\n", ret);
			return -EINVAL;
		}

		if ((pre_v[0] == v[0]) && (pre_v[1] == v[1]) &&
			(pre_v[2] == v[2]) && (pre_v[3] == v[3]) &&
			(pre_v[4] == v[4]) && (pre_v[5] == v[5]))
				break;
		else {
			pre_v[0] = v[0];
			pre_v[1] = v[1];
			pre_v[2] = v[2];
			pre_v[3] = v[3];
			pre_v[4] = v[4];
			pre_v[5] = v[5];
		}
	}

	tm->tm_sec = v[0] & rtc->desc->cnt_s.msk;
	tm->tm_min = v[1] & rtc->desc->cnt_mi.msk;
	tm->tm_hour = v[2] & rtc->desc->cnt_h.msk;
	tm->tm_mday = (v[3] & rtc->desc->cnt_d.msk) + 1;
	tm->tm_mon = (v[4] & rtc->desc->cnt_mo.msk);
	tm->tm_year = (v[5] & rtc->desc->cnt_y.msk) + 100;

	pr_debug("%s:%d, s:%d, min:%d, hour:%d, mday:%d, month:%d, year:%d\n",
			__func__, __LINE__,
			tm->tm_sec,
			tm->tm_min,
			tm->tm_hour,
			tm->tm_mday,
			tm->tm_mon,
			tm->tm_year);

	return 0;
}

static int spt_rtc_set_time(struct device *dev, struct rtc_time *tm)
{
	int ret;
	unsigned int v[6];
	union rtc_ctl_desc rtc_ctl;
	struct ky_rtc *rtc = dev_get_drvdata(dev);

	pr_debug("%s:%d, s:%d, min:%d, hour:%d, mday:%d, month:%d, year:%d\n",
			__func__, __LINE__,
			tm->tm_sec,
			tm->tm_min,
			tm->tm_hour,
			tm->tm_mday,
			tm->tm_mon,
			tm->tm_year);

	ret = regmap_read(rtc->regmap, rtc->desc->rtc_ctl.reg, (unsigned int *)&rtc_ctl.val);
	if (ret) {
		dev_err(rtc->dev, "failed to read rtc ctrl: %d\n", ret);
		return -EINVAL;
	}

	/* disbale rtc first */
	rtc_ctl.bits.rtc_en = 0;

	ret = regmap_write_bits(rtc->regmap, rtc->desc->rtc_ctl.reg,
			0xff, rtc_ctl.val);
	if (ret) {
		dev_err(rtc->dev, "failed to set rtc ctrl register: %d\n", ret);
		return -EINVAL;
	}

	while (1) {
		ret = regmap_write_bits(rtc->regmap, rtc->desc->cnt_s.reg,
				rtc->desc->cnt_s.msk, tm->tm_sec);
		if (ret) {
			dev_err(rtc->dev, "failed to update second: %d\n", ret);
			return -EINVAL;
		}

		ret = regmap_write_bits(rtc->regmap, rtc->desc->cnt_mi.reg,
				rtc->desc->cnt_mi.msk, tm->tm_min);
		if (ret) {
			dev_err(rtc->dev, "failed to update minutes: %d\n", ret);
			return -EINVAL;
		}

		ret = regmap_write_bits(rtc->regmap, rtc->desc->cnt_h.reg,
				rtc->desc->cnt_h.msk, tm->tm_hour);
		if (ret) {
			dev_err(rtc->dev, "failed to update hour: %d\n", ret);
			return -EINVAL;
		}

		ret = regmap_write_bits(rtc->regmap, rtc->desc->cnt_d.reg,
				rtc->desc->cnt_d.msk, tm->tm_mday - 1);
		if (ret) {
			dev_err(rtc->dev, "failed to update day: %d\n", ret);
			return -EINVAL;
		}

		ret = regmap_write_bits(rtc->regmap, rtc->desc->cnt_mo.reg,
				rtc->desc->cnt_mo.msk, tm->tm_mon);
		if (ret) {
			dev_err(rtc->dev, "failed to update month: %d\n", ret);
			return -EINVAL;
		}

		ret = regmap_write_bits(rtc->regmap, rtc->desc->cnt_y.reg,
				rtc->desc->cnt_y.msk, tm->tm_year - 100);
		if (ret) {
			dev_err(rtc->dev, "failed to update month: %d\n", ret);
			return -EINVAL;
		}

		/* read again */
		ret = regmap_read(rtc->regmap, rtc->desc->cnt_s.reg, &v[0]);
		if (ret) {
			dev_err(rtc->dev, "failed to read second: %d\n", ret);
			return -EINVAL;
		}
		v[0] = v[0] & rtc->desc->cnt_s.msk;

		ret = regmap_read(rtc->regmap, rtc->desc->cnt_mi.reg, &v[1]);
		if (ret) {
			dev_err(rtc->dev, "failed to read minute: %d\n", ret);
			return -EINVAL;
		}
		v[1] = v[1] & rtc->desc->cnt_mi.msk;

		ret = regmap_read(rtc->regmap, rtc->desc->cnt_h.reg, &v[2]);
		if (ret) {
			dev_err(rtc->dev, "failed to read hour: %d\n", ret);
			return -EINVAL;
		}
		v[2] = v[2] & rtc->desc->cnt_h.msk;

		ret = regmap_read(rtc->regmap, rtc->desc->cnt_d.reg, &v[3]);
		if (ret) {
			dev_err(rtc->dev, "failed to read day: %d\n", ret);
			return -EINVAL;
		}
		v[3] = v[3] & rtc->desc->cnt_d.msk;

		ret = regmap_read(rtc->regmap, rtc->desc->cnt_mo.reg, &v[4]);
		if (ret) {
			dev_err(rtc->dev, "failed to read month: %d\n", ret);
			return -EINVAL;
		}
		v[4] = v[4] & rtc->desc->cnt_mo.msk;

		ret = regmap_read(rtc->regmap, rtc->desc->cnt_y.reg, &v[5]);
		if (ret) {
			dev_err(rtc->dev, "failed to read year: %d\n", ret);
			return -EINVAL;
		}
		v[5] = v[5] & rtc->desc->cnt_y.msk;

		if ((v[0] == (rtc->desc->cnt_s.msk & tm->tm_sec)) &&
			(v[1] == (rtc->desc->cnt_mi.msk & tm->tm_min)) &&
			(v[2] == (rtc->desc->cnt_h.msk & tm->tm_hour)) &&
			((v[3] + 1) == (rtc->desc->cnt_d.msk & tm->tm_mday)) &&
			(v[4] == (rtc->desc->cnt_mo.msk & tm->tm_mon)) &&
			(v[5] == (rtc->desc->cnt_y.msk & (tm->tm_year - 100))))
			break;
	}

	/* enable rtc last */
	rtc_ctl.bits.rtc_en = 1;

	ret = regmap_write_bits(rtc->regmap, rtc->desc->rtc_ctl.reg,
			0xff, rtc_ctl.val);
	if (ret) {
		dev_err(rtc->dev, "failed to set rtc ctrl register: %d\n", ret);
		return -EINVAL;
	}

	return 0;
}

static int spt_rtc_read_alarm(struct device *dev, struct rtc_wkalrm *alrm)
{
	int ret;
	unsigned int v[6];
	union rtc_ctl_desc rtc_ctl;
	struct ky_rtc *rtc = dev_get_drvdata(dev);

	ret = regmap_read(rtc->regmap, rtc->desc->alarm_s.reg, &v[0]);
	if (ret) {
		dev_err(rtc->dev, "failed to read alarm second: %d\n", ret);
		return -EINVAL;
	}

	ret = regmap_read(rtc->regmap, rtc->desc->alarm_mi.reg, &v[1]);
	if (ret) {
		dev_err(rtc->dev, "failed to read alarm minute: %d\n", ret);
		return -EINVAL;
	}

	ret = regmap_read(rtc->regmap, rtc->desc->alarm_h.reg, &v[2]);
	if (ret) {
		dev_err(rtc->dev, "failed to read alarm hour: %d\n", ret);
		return -EINVAL;
	}

	ret = regmap_read(rtc->regmap, rtc->desc->alarm_d.reg, &v[3]);
	if (ret) {
		dev_err(rtc->dev, "failed to read alarm day: %d\n", ret);
		return -EINVAL;
	}

	ret = regmap_read(rtc->regmap, rtc->desc->alarm_mo.reg, &v[4]);
	if (ret) {
		dev_err(rtc->dev, "failed to read alarm month: %d\n", ret);
		return -EINVAL;
	}

	ret = regmap_read(rtc->regmap, rtc->desc->alarm_y.reg, &v[5]);
	if (ret) {
		dev_err(rtc->dev, "failed to read alarm year: %d\n", ret);
		return -EINVAL;
	}

	/* 2000:1:1:0:0:0 */
	alrm->time.tm_sec = v[0] & rtc->desc->alarm_s.msk;
	alrm->time.tm_min = v[1] & rtc->desc->alarm_mi.msk;
	alrm->time.tm_hour = v[2] & rtc->desc->alarm_h.msk;
	alrm->time.tm_mday = (v[3] & rtc->desc->alarm_d.msk) + 1;
	alrm->time.tm_mon = (v[4] & rtc->desc->alarm_mo.msk);
	alrm->time.tm_year = (v[5] & rtc->desc->alarm_y.msk) + 100;

	pr_debug("%s:%d, s:%d, min:%d, hour:%d, mday:%d, month:%d, year:%d\n",
			__func__, __LINE__,
			alrm->time.tm_sec,
			alrm->time.tm_min,
			alrm->time.tm_hour,
			alrm->time.tm_mday,
			alrm->time.tm_mon,
			alrm->time.tm_year);

	ret = regmap_read(rtc->regmap, rtc->desc->rtc_ctl.reg, (unsigned int *)&rtc_ctl.val);
	if (ret) {
		dev_err(rtc->dev, "failed to read alarm second: %d\n", ret);
		return -EINVAL;
	}

	alrm->enabled = rtc_ctl.bits.alarm_en;

	return 0;
}

static int spt_rtc_set_alarm(struct device *dev, struct rtc_wkalrm *alrm)
{
	int ret;
	union rtc_ctl_desc rtc_ctl;
	struct ky_rtc *rtc = dev_get_drvdata(dev);

	pr_debug("%s:%d, s:%d, min:%d, hour:%d, mday:%d, month:%d, year:%d\n",
			__func__, __LINE__,
			alrm->time.tm_sec,
			alrm->time.tm_min,
			alrm->time.tm_hour,
			alrm->time.tm_mday,
			alrm->time.tm_mon,
			alrm->time.tm_year);

	/* disable the alrm function first */
	ret = regmap_read(rtc->regmap, rtc->desc->rtc_ctl.reg, (unsigned int *)&rtc_ctl.val);
	if (ret) {
		dev_err(rtc->dev, "failed to read rtc ctrl register: %d\n", ret);
		return -EINVAL;
	}

	rtc_ctl.bits.alarm_en = 0;

	ret = regmap_write_bits(rtc->regmap, rtc->desc->rtc_ctl.reg,
			0xff, rtc_ctl.val);
	if (ret) {
		dev_err(rtc->dev, "failed to set rtc ctrl register: %d\n", ret);
		return -EINVAL;
	}

	ret = regmap_write_bits(rtc->regmap, rtc->desc->alarm_s.reg,
			rtc->desc->alarm_s.msk, alrm->time.tm_sec);
	if (ret) {
		dev_err(rtc->dev, "failed to update alrm second: %d\n", ret);
		return -EINVAL;
	}

	ret = regmap_write_bits(rtc->regmap, rtc->desc->alarm_mi.reg,
			rtc->desc->alarm_mi.msk, alrm->time.tm_min);
	if (ret) {
		dev_err(rtc->dev, "failed to update alarm minutes: %d\n", ret);
		return -EINVAL;
	}

	ret = regmap_write_bits(rtc->regmap, rtc->desc->alarm_h.reg,
			rtc->desc->alarm_h.msk, alrm->time.tm_hour);
	if (ret) {
		dev_err(rtc->dev, "failed to update alarm hour: %d\n", ret);
		return -EINVAL;
	}

	ret = regmap_write_bits(rtc->regmap, rtc->desc->alarm_d.reg,
			rtc->desc->alarm_d.msk, alrm->time.tm_mday - 1);
	if (ret) {
		dev_err(rtc->dev, "failed to update alarm day: %d\n", ret);
		return -EINVAL;
	}

	ret = regmap_write_bits(rtc->regmap, rtc->desc->alarm_mo.reg,
			rtc->desc->alarm_mo.msk, alrm->time.tm_mon);
	if (ret) {
		dev_err(rtc->dev, "failed to update alarm month: %d\n", ret);
		return -EINVAL;
	}

	ret = regmap_write_bits(rtc->regmap, rtc->desc->alarm_y.reg,
			rtc->desc->alarm_y.msk, alrm->time.tm_year - 100);
	if (ret) {
		dev_err(rtc->dev, "failed to update month: %d\n", ret);
		return -EINVAL;
	}

	if (alrm->enabled) {
		rtc_ctl.bits.alarm_en = 1;

		ret = regmap_write_bits(rtc->regmap, rtc->desc->rtc_ctl.reg,
				0xff, rtc_ctl.val);
		if (ret) {
			dev_err(rtc->dev, "failed to set rtc ctrl register: %d\n", ret);
			return -EINVAL;
		}
	}

	return 0;
}

static int spt_rtc_alarm_irq_enable(struct device *dev, unsigned int enabled)
{
	int ret;
	union rtc_ctl_desc rtc_ctl;
	struct ky_rtc *rtc = dev_get_drvdata(dev);

	ret = regmap_read(rtc->regmap, rtc->desc->rtc_ctl.reg, (unsigned int *)&rtc_ctl.val);
	if (ret) {
		dev_err(rtc->dev, "failed to read rtc ctrl register: %d\n", ret);
		return -EINVAL;
	}

	if (enabled)
		rtc_ctl.bits.alarm_en = 1;
	else
		rtc_ctl.bits.alarm_en = 0;

	ret = regmap_write_bits(rtc->regmap, rtc->desc->rtc_ctl.reg,
			0xff, rtc_ctl.val);
	if (ret) {
		dev_err(rtc->dev, "failed to set rtc ctrl register: %d\n", ret);
		return -EINVAL;
	}

	return 0;
}

static const struct rtc_class_ops spt_rtc_ops = {
	.read_time = spt_rtc_read_time,
	.set_time = spt_rtc_set_time,
	.read_alarm = spt_rtc_read_alarm,
	.set_alarm = spt_rtc_set_alarm,
	.alarm_irq_enable = spt_rtc_alarm_irq_enable,
};

static int ky_rtc_probe(struct platform_device *pdev)
{
	int ret = 0;
	struct ky_rtc *rtc;
	union rtc_ctl_desc rtc_ctl;
	const struct of_device_id *of_id;
	struct ky_pmic *pmic = dev_get_drvdata(pdev->dev.parent);

	rtc = devm_kzalloc(&pdev->dev, sizeof(struct ky_rtc), GFP_KERNEL);
	if (!rtc)
		return -ENOMEM;

	of_id = of_match_device(ky_id_table, &pdev->dev);
	if (!of_id) {
		pr_err("Unable to match OF ID\n");
		return -ENODEV;
	}

	rtc->regmap = pmic->regmap;
	rtc->dev = &pdev->dev;
	rtc->desc = (struct rtc_regdesc *)of_id->data;
	rtc->irq = platform_get_irq(pdev, 0);
	if (rtc->irq < 0) {
		dev_err(&pdev->dev, "get rtc irq error: %d\n", rtc->irq);
		return -EINVAL;
	}

	dev_set_drvdata(&pdev->dev, rtc);

	ret = devm_request_any_context_irq(&pdev->dev, rtc->irq,
					   spt_rtc_irq,
					   IRQF_TRIGGER_NONE | IRQF_ONESHOT,
					   "rtc@pmic", rtc);
	if (ret < 0) {
		dev_err(&pdev->dev, "Can't register rtc irq: %d\n", ret);
		return -EINVAL;
	}

	dev_pm_set_wake_irq(&pdev->dev, rtc->irq);
	device_init_wakeup(&pdev->dev, 1);

	rtc->rtc_dev = devm_rtc_allocate_device(&pdev->dev);
	if (IS_ERR(rtc->rtc_dev))
		return PTR_ERR(rtc->rtc_dev);

	rtc->rtc_dev->ops = &spt_rtc_ops;
	rtc->rtc_dev->range_min = RTC_TIMESTAMP_BEGIN_2000;
	rtc->rtc_dev->range_max = RTC_TIMESTAMP_END_2063;

	ret = devm_rtc_register_device(rtc->rtc_dev);
	if (ret) {
		dev_err(&pdev->dev, "register rtc device error: %d\n", ret);
		return -EINVAL;
	}

	/* enable the rtc function */
	ret = regmap_read(rtc->regmap, rtc->desc->rtc_ctl.reg, (unsigned int *)&rtc_ctl.val);
	if (ret) {
		dev_err(rtc->dev, "failed to read rtc ctrl register: %d\n", ret);
		return -EINVAL;
	}

	/* internal 32k clk */
	rtc_ctl.bits.rtc_clk_sel = 1;
	/* enable rtc */
	rtc_ctl.bits.rtc_en = 1;
	/* rtc clk out enable */
	rtc_ctl.bits.out_32k_en = 1;
	/* enable external crystal */
	rtc_ctl.bits.crystal_en = 1;

	ret = regmap_update_bits(rtc->regmap, rtc->desc->rtc_ctl.reg,
			0xff, rtc_ctl.val);
	if (ret) {
		dev_err(rtc->dev, "failed to set rtc ctrl register: %d\n", ret);
		return -EINVAL;
	}

	return 0;
}

static struct platform_driver ky_pmic_rtc_driver = {
	.probe		= ky_rtc_probe,
	.driver	= {
		.name		= "ky-pmic-rtc",
		.of_match_table	= ky_id_table,
	},
};

module_platform_driver(ky_pmic_rtc_driver);

MODULE_ALIAS("platform:rtc-spt-pmic");
MODULE_DESCRIPTION("PMIC RTC driver");
MODULE_LICENSE("GPL v2");
