/*
 * Based on drivers/input/keyboard/sunxi-keyboard.c
 *
 * Copyright (C) 2015 Allwinnertech Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include <linux/module.h>
#include <linux/init.h>
#include <linux/input.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/interrupt.h>
#include <linux/keyboard.h>
#include <linux/ioport.h>
#include <asm/irq.h>
#include <asm/io.h>
#include <linux/timer.h>
#include <linux/clk.h>
#include <linux/irq.h>
#include <linux/of_platform.h>
#include <linux/of_irq.h>
#include <linux/of_address.h>

#if defined(CONFIG_PM)
#include <linux/pm.h>
#endif
#include "sunxi-keyboard.h"

static unsigned char keypad_mapindex[64] = {
	0, 0, 0, 0, 0, 0, 0, 0, 0,		/* key 1, 0-8 */
	1, 1, 1, 1, 1,				/* key 2, 9-13 */
	2, 2, 2, 2, 2, 2,			/* key 3, 14-19 */
	3, 3, 3, 3, 3, 3,			/* key 4, 20-25 */
	4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,	/* key 5, 26-36 */
	5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,	/* key 6, 37-39 */
	6, 6, 6, 6, 6, 6, 6, 6, 6,		/* key 7, 40-49 */
	7, 7, 7, 7, 7, 7, 7			/* key 8, 50-63 */
};
#define VOL_NUM KEY_MAX_CNT

struct sunxi_key_data {
	struct platform_device	*pdev;
	struct clk *mclk;
	struct clk *pclk;
	struct input_dev *input_dev;
	void __iomem *reg_base;
	u32 scankeycodes[KEY_MAX_CNT];
	int irq_num;
	u32 key_val;
	unsigned char compare_later;
	unsigned char compare_before;
	u8 key_code;
	char key_name[16];
	u8 key_cnt;
	int wakeup;
};

static struct sunxi_adc_disc disc_1350 = {
	.measure = 1350,
	.resol = 21,
};
static struct sunxi_adc_disc disc_1200 = {
	.measure = 1200,
	.resol = 19,
};
static struct sunxi_adc_disc disc_2000 = {
	.measure = 2000,
	.resol = 31,
};

#ifdef CONFIG_OF
/*
 * Translate OpenFirmware node properties into platform_data
 */
static struct of_device_id const sunxi_keyboard_of_match[] = {
	{ .compatible = "allwinner,keyboard_1350mv",
		.data = &disc_1350 },
	{ .compatible = "allwinner,keyboard_1200mv",
		.data = &disc_1200 },
	{ .compatible = "allwinner,keyboard_2000mv",
		.data = &disc_2000 },
	{ },
};
MODULE_DEVICE_TABLE(of, sunxi_keyboard_of_match);
#else /* !CONFIG_OF */
#endif

static void sunxi_keyboard_ctrl_set(void __iomem *reg_base,
				enum key_mode key_mode, u32 para)
{
	u32 ctrl_reg = 0;

	if (para != 0)
		ctrl_reg = readl(reg_base + LRADC_CTRL);
	if (CONCERT_DLY_SET & key_mode)
		ctrl_reg |= (FIRST_CONCERT_DLY & para);
	if (ADC_CHAN_SET & key_mode)
		ctrl_reg |= (ADC_CHAN_SELECT & para);
	if (KEY_MODE_SET & key_mode)
		ctrl_reg |= (KEY_MODE_SELECT & para);
	if (LRADC_HOLD_SET & key_mode)
		ctrl_reg |= (LRADC_HOLD_EN & para);
	if (LEVELB_VOL_SET & key_mode) {
		ctrl_reg |= (LEVELB_VOL & para);
#if defined(CONFIG_ARCH_SUN8IW18)
		ctrl_reg &= ~(u32)(3 << 4);
#endif
	}
	if (LRADC_SAMPLE_SET & key_mode)
		ctrl_reg |= (LRADC_SAMPLE_250HZ & para);
	if (LRADC_EN_SET & key_mode)
		ctrl_reg |= (LRADC_EN & para);

	writel(ctrl_reg, reg_base + LRADC_CTRL);
}

static void sunxi_keyboard_int_set(void __iomem *reg_base,
					enum int_mode int_mode, u32 para)
{
	u32 ctrl_reg = 0;

	if (para != 0)
		ctrl_reg = readl(reg_base + LRADC_INTC);

	if (ADC0_DOWN_INT_SET & int_mode)
		ctrl_reg |= (LRADC_ADC0_DOWN_EN & para);
	if (ADC0_UP_INT_SET & int_mode)
		ctrl_reg |= (LRADC_ADC0_UP_EN & para);
	if (ADC0_DATA_INT_SET & int_mode)
		ctrl_reg |= (LRADC_ADC0_DATA_EN & para);

	writel(ctrl_reg, reg_base + LRADC_INTC);
}

static u32 sunxi_keyboard_read_ints(void __iomem *reg_base)
{
	u32 reg_val;

	reg_val  = readl(reg_base + LRADC_INT_STA);

	return reg_val;
}

static void sunxi_keyboard_clr_ints(void __iomem *reg_base, u32 reg_val)
{
	writel(reg_val, reg_base + LRADC_INT_STA);
}

static u32 sunxi_keyboard_read_data(void __iomem *reg_base)
{
	u32 reg_val;

	reg_val = readl(reg_base + LRADC_DATA0);

	return reg_val;
}

#ifdef CONFIG_PM
static int sunxi_keyboard_suspend(struct device *dev)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct sunxi_key_data *key_data = platform_get_drvdata(pdev);

	pr_debug("[%s] enter standby\n", __func__);

	if (device_may_wakeup(dev)) {
		if (key_data->wakeup)
			enable_irq_wake(key_data->irq_num);
	} else {
		disable_irq_nosync(key_data->irq_num);

		sunxi_keyboard_ctrl_set(key_data->reg_base, 0, 0);

		if (IS_ERR_OR_NULL(key_data->mclk))
			pr_warn("%s apb1_keyadc mclk handle is invalid!\n",
					__func__);
		else
			clk_disable_unprepare(key_data->mclk);
	}

	return 0;
}

static int sunxi_keyboard_resume(struct device *dev)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct sunxi_key_data *key_data = platform_get_drvdata(pdev);
	unsigned long mode, para;

	pr_debug("[%s] return from standby\n", __func__);

	if (device_may_wakeup(dev)) {
		if (key_data->wakeup)
			disable_irq_wake(key_data->irq_num);
	} else {
		if (IS_ERR_OR_NULL(key_data->mclk))
			pr_warn("%s apb1_keyadc mclk handle is invalid!\n",
					__func__);
		else
			clk_prepare_enable(key_data->mclk);

		mode = ADC0_DOWN_INT_SET | ADC0_UP_INT_SET | ADC0_DATA_INT_SET;
		para = LRADC_ADC0_DOWN_EN | LRADC_ADC0_UP_EN
			| LRADC_ADC0_DATA_EN;
		sunxi_keyboard_int_set(key_data->reg_base, mode, para);
		mode = CONCERT_DLY_SET | ADC_CHAN_SET | KEY_MODE_SET
			| LRADC_HOLD_SET | LEVELB_VOL_SET
			| LRADC_SAMPLE_SET | LRADC_EN_SET;
		para = FIRST_CONCERT_DLY | LEVELB_VOL|KEY_MODE_SELECT
			| LRADC_HOLD_EN	| ADC_CHAN_SELECT
			| LRADC_SAMPLE_250HZ|LRADC_EN;
		sunxi_keyboard_ctrl_set(key_data->reg_base, mode, para);

		enable_irq(key_data->irq_num);
	}

	return 0;
}
#endif

static irqreturn_t sunxi_isr_key(int irq, void *dummy)
{
	struct sunxi_key_data *key_data = (struct sunxi_key_data *)dummy;
	u32 reg_val = 0;
	u32 key_val = 0;

	pr_debug("Key Interrupt\n");
	reg_val = sunxi_keyboard_read_ints(key_data->reg_base);
	if (reg_val & LRADC_ADC0_DOWNPEND)
		pr_debug("key down\n");

	if (reg_val & LRADC_ADC0_DATAPEND) {
		key_data->key_cnt++;
		key_val = sunxi_keyboard_read_data(key_data->reg_base);
		key_data->compare_before = key_val&0x3f;
		if (key_data->compare_before == key_data->compare_later) {
			key_data->compare_later = key_data->compare_before;
			key_data->key_code = keypad_mapindex[key_val&0x3f];
			input_report_key(key_data->input_dev,
				key_data->scankeycodes[key_data->key_code], 1);
			input_sync(key_data->input_dev);
			key_data->key_cnt = 0;
		}
		if (key_data->key_cnt == 2) {
			key_data->compare_later = key_data->compare_before;
			key_data->key_cnt = 0;
		}
	}

	if (reg_val & LRADC_ADC0_UPPEND) {
		if (key_data->wakeup)
			pm_wakeup_event(key_data->input_dev->dev.parent, 0);
		pr_debug("report data:%8d key_val:%8d\n",
				key_data->scankeycodes[key_data->key_code],
				key_val);
		input_report_key(key_data->input_dev,
				key_data->scankeycodes[key_data->key_code], 0);
		input_sync(key_data->input_dev);
		pr_debug("key up\n");
		key_data->key_cnt = 0;
	}

	sunxi_keyboard_clr_ints(key_data->reg_base, reg_val);

	return IRQ_HANDLED;
}

static int sunxi_keyboard_startup(struct sunxi_key_data *key_data,
				struct platform_device *pdev)
{
	struct device_node *np = NULL;
	int ret = 0;

	np = pdev->dev.of_node;
	if (!of_device_is_available(np)) {
		pr_err("%s: sunxi keyboard is disable\n", __func__);
		return -EPERM;
	}
	key_data->reg_base = of_iomap(np, 0);
	if (key_data->reg_base == 0) {
		pr_err("%s:Failed to ioremap() io memory region.\n", __func__);
		ret = -EBUSY;
	} else
		pr_debug("key base: %p !\n", key_data->reg_base);

	key_data->irq_num = irq_of_parse_and_map(np, 0);
	if (key_data->irq_num == 0) {
		pr_err("%s:Failed to map irq.\n", __func__);
		ret = -EBUSY;
	} else
		pr_debug("ir irq num: %d !\n", key_data->irq_num);

	/* some IC will use clock gating while others HW use 24MHZ, So just try
	 * to get the clock, if it doesn't exist, give warning instead of error
	 */
	key_data->mclk = of_clk_get(np, 0);
	if (IS_ERR_OR_NULL(key_data->mclk)) {
		pr_err("%s: keyboard has no clk.\n", __func__);
	} else{
		if (clk_prepare_enable(key_data->mclk)) {
			pr_err("%s enable apb1_keyadc clock failed!\n",
							__func__);
			return -EINVAL;
		}
	}
	return ret;
}

static int sunxikbd_key_init(struct sunxi_key_data *key_data,
			struct platform_device *pdev)
{
	struct device_node *np = NULL;
	const struct of_device_id *match;
	struct sunxi_adc_disc *disc;
	int i, j = 0;
	u32 val[2] = {0, 0};
	u32 key_num = 0;
	u32 key_vol[VOL_NUM];

	np = pdev->dev.of_node;
	match = of_match_node(sunxi_keyboard_of_match, np);
	disc = (struct sunxi_adc_disc *)match->data;
	if (of_property_read_u32(np, "key_cnt", &key_num)) {
		pr_err("%s: get key count failed", __func__);
		return -EBUSY;
	}
	pr_debug("%s key number = %d.\n", __func__, key_num);
	if (key_num < 1 || key_num > VOL_NUM) {
		pr_err("incorrect key number.\n");
		return -1;
	}
	for (i = 0; i < key_num; i++) {
		sprintf(key_data->key_name, "key%d", i);
		if (of_property_read_u32_array(np, key_data->key_name,
						val, ARRAY_SIZE(val))) {
			pr_err("%s:get%s err!\n", __func__, key_data->key_name);
			return -EBUSY;
		}
		key_vol[i] = val[0];
		key_data->scankeycodes[i] = val[1];
		pr_debug("%s: key%d vol= %d code= %d\n", __func__, i,
				key_vol[i], key_data->scankeycodes[i]);
	}
	key_vol[key_num] = disc->measure;
	for (i = 0; i < key_num; i++)
		key_vol[i] += (key_vol[i+1] - key_vol[i])/2;

	for (i = 0; i < 64; i++) {
		if (i * disc->resol > key_vol[j])
			j++;
		keypad_mapindex[i] = j;
	}

	key_data->wakeup = of_property_read_bool(np, "wakeup-source");
	device_init_wakeup(&pdev->dev, key_data->wakeup);

	return 0;
}

static int sunxi_keyboard_probe(struct platform_device *pdev)
{
	static struct input_dev *sunxikbd_dev;
	struct sunxi_key_data *key_data;
	unsigned long mode, para;
	int i;
	int err = 0;

	key_data = kzalloc(sizeof(*key_data), GFP_KERNEL);
	if (IS_ERR_OR_NULL(key_data)) {
		pr_err("key_data: not enough memory for key data\n");
		return -ENOMEM;
	}

	pr_debug("sunxikbd_init\n");
	if (pdev->dev.of_node) {
		/* get dt and sysconfig */
		err = sunxi_keyboard_startup(key_data, pdev);
	} else {
		pr_err("sunxi keyboard device tree err!\n");
		return -EBUSY;
	}

	if (err < 0)
		goto fail1;

	if (sunxikbd_key_init(key_data, pdev)) {
		err = -EFAULT;
		goto fail1;
	}

	sunxikbd_dev = input_allocate_device();
	if (!sunxikbd_dev) {
		pr_err("sunxikbd: not enough memory for input device\n");
		err = -ENOMEM;
		goto fail1;
	}

	sunxikbd_dev->name = INPUT_DEV_NAME;
	sunxikbd_dev->phys = "sunxikbd/input0";
	sunxikbd_dev->id.bustype = BUS_HOST;
	sunxikbd_dev->id.vendor = 0x0001;
	sunxikbd_dev->id.product = 0x0001;
	sunxikbd_dev->id.version = 0x0100;

#ifdef REPORT_REPEAT_KEY_BY_INPUT_CORE
	sunxikbd_dev->evbit[0] = BIT_MASK(EV_KEY)|BIT_MASK(EV_REP);
	pr_info("support report repeat key value.\n");
#else
	sunxikbd_dev->evbit[0] = BIT_MASK(EV_KEY);
#endif

	for (i = 0; i < KEY_MAX_CNT; i++) {
		if (key_data->scankeycodes[i] <  KEY_MAX)
			set_bit(key_data->scankeycodes[i], sunxikbd_dev->keybit);
	}
	key_data->input_dev = sunxikbd_dev;
	platform_set_drvdata(pdev, key_data);
#ifdef ONE_CHANNEL
	mode = ADC0_DOWN_INT_SET | ADC0_UP_INT_SET | ADC0_DATA_INT_SET;
	para = LRADC_ADC0_DOWN_EN | LRADC_ADC0_UP_EN | LRADC_ADC0_DATA_EN;
	sunxi_keyboard_int_set(key_data->reg_base, mode, para);
	mode = CONCERT_DLY_SET | ADC_CHAN_SET | KEY_MODE_SET
		| LRADC_HOLD_SET | LEVELB_VOL_SET
		| LRADC_SAMPLE_SET | LRADC_EN_SET;
	para = FIRST_CONCERT_DLY|LEVELB_VOL|KEY_MODE_SELECT
		|LRADC_HOLD_EN|ADC_CHAN_SELECT
		|LRADC_SAMPLE_250HZ|LRADC_EN;
	sunxi_keyboard_ctrl_set(key_data->reg_base, mode, para);
#else
#endif
	if (request_irq(key_data->irq_num, sunxi_isr_key, 0,
					"sunxikbd", key_data)) {
		err = -EBUSY;
		pr_err("request irq failure.\n");
		goto fail2;
	}
	err = input_register_device(key_data->input_dev);
	if (err)
		goto fail3;

	pr_debug("sunxikbd_init end\n");
	return 0;

fail3:
	free_irq(key_data->irq_num, NULL);
fail2:
	input_free_device(key_data->input_dev);
fail1:
	kfree(key_data);
	pr_err("sunxikbd_init failed.\n");

	return err;
}

static int sunxi_keyboard_remove(struct platform_device *pdev)
{
	struct sunxi_key_data *key_data = platform_get_drvdata(pdev);

	free_irq(key_data->irq_num, key_data);
	input_unregister_device(key_data->input_dev);
	device_init_wakeup(&pdev->dev, 0);
	kfree(key_data);
	return 0;
}

#ifdef CONFIG_PM
static const struct dev_pm_ops sunxi_keyboard_pm_ops = {
	.suspend = sunxi_keyboard_suspend,
	.resume = sunxi_keyboard_resume,
};

#define SUNXI_KEYBOARD_PM_OPS (&sunxi_keyboard_pm_ops)
#endif

static struct platform_driver sunxi_keyboard_driver = {
	.probe  = sunxi_keyboard_probe,
	.remove = sunxi_keyboard_remove,
	.driver = {
		.name   = "sunxi-keyboard",
		.owner  = THIS_MODULE,
#ifdef CONFIG_PM
		.pm	= SUNXI_KEYBOARD_PM_OPS,
#endif
		.of_match_table = of_match_ptr(sunxi_keyboard_of_match),
	},
};
module_platform_driver(sunxi_keyboard_driver);

MODULE_AUTHOR(" Qin");
MODULE_DESCRIPTION("sunxi-keyboard driver");
MODULE_LICENSE("GPL");
