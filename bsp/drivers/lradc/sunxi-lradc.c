/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
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
//#define DEBUG
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
#include <linux/reset.h>

#include <linux/extcon-provider.h>

#if IS_ENABLED(CONFIG_IIO)
#include <linux/iio/iio.h>
#include <linux/iio/machine.h>
#include <linux/iio/driver.h>
#endif

#if IS_ENABLED(CONFIG_PM)
#include <linux/pm.h>
#endif

#define DEFAULT_DEBOUNCE_VALUE 50

#define SUNXI_LRADC_INPUT_DEV_NAME		("sunxi-keyboard")

#define SUNXI_LRADC_KEY_MAX_CNT			(13)
#define JACK_DETECT_VOL				1220

#define SUNXI_LRADC_CTRL			(0x00)
#define SUNXI_LRADC_INTC			(0x04)
#define SUNXI_LRADC_INT_STA			(0x08)
#define SUNXI_LRADC_DATA0			(0x0c)
#define SUNXI_LRADC_DATA1			(0x10)

#define SUNXI_LRADC_FIRST_CONVERT_DLY		(0 << 24)
#define SUNXI_LRADC_CHAN			(0x2)
#define SUNXI_LRADC_CHAN_SELECT			(SUNXI_LRADC_CHAN << 22)
#define SUNXI_LRADC_KEY_MODE			(0)
#define SUNXI_LRADC_KEY_MODE_SELECT		(SUNXI_LRADC_KEY_MODE << 12)
#define SUNXI_LRADC_HOLD_KEY_EN			(0 << 7)
#define SUNXI_LRADC_HOLD_EN			(1 << 6)
#define SUNXI_LRADC_LEVELB_CNT			(0 << 8)
#define SUNXI_LRADC_LEVELB_VOL			(0 << 4)
#define SUNXI_LRADC_LEVELB_VOL_JACK_DETECT			(3 << 4)
#define SUNXI_LRADC_SAMPLE_32HZ			(3 << 2)
#define SUNXI_LRADC_SAMPLE_62HZ			(2 << 2)
#define SUNXI_LRADC_SAMPLE_125HZ		(1 << 2)
#define SUNXI_LRADC_SAMPLE_250HZ		(0 << 2)
#define SUNXI_LRADC_EN				(1 << 0)

#define SUNXI_LRADC_FIRST_CONVERT_DLY_MASK	(0xff << 24)
#define SUNXI_LRADC_KEY_MODE_SELECT_MASK	(0x03 << 12)
#define SUNXI_LRADC_HOLD_KEY_EN_MASK		(0x01 << 7)
#define SUNXI_LRADC_HOLD_EN_MASK		(0x01 << 6)
#define SUNXI_LRADC_LEVELB_CNT_MASK		(0x0f << 8)
#define SUNXI_LRADC_LEVELB_VOL_MASK		(0x03 << 4)
#define SUNXI_LRADC_SAMPLE_250HZ_MASK		(0x03 << 2)
#define SUNXI_LRADC_EN_MASK			(0x01 << 0)

#define SUNXI_LRADC_ADC1_UP_EN			(1 << 12)
#define SUNXI_LRADC_ADC1_DOWN_EN		(1 << 9)
#define SUNXI_LRADC_ADC1_DATA_EN		(1 << 8)

#define SUNXI_LRADC_ADC0_UP_EN			(1 << 4)
#define SUNXI_LRADC_ADC0_DOWN_EN		(1 << 1)
#define SUNXI_LRADC_ADC0_DATA_EN		(1 << 0)

#define SUNXI_LRADC_ADC1_UP_EN_MASK		(0x01 << 12)
#define SUNXI_LRADC_ADC1_DOWN_EN_MASK		(0x01 << 9)
#define SUNXI_LRADC_ADC1_DATA_EN_MASK		(0x01 << 8)

#define SUNXI_LRADC_ADC0_UP_EN_MASK		(0x01 << 4)
#define SUNXI_LRADC_ADC0_DOWN_EN_MASK		(0x01 << 1)
#define SUNXI_LRADC_ADC0_DATA_EN_MASK		(0x01 << 0)

#define SUNXI_LRADC_ADC1_UPPEND			(1 << 12)
#define SUNXI_LRADC_ADC1_DOWNPEND		(1 << 9)
#define SUNXI_LRADC_ADC1_DATAPEND		(1 << 8)

#define SUNXI_LRADC_ADC0_UPPEND			(1 << 4)
#define SUNXI_LRADC_ADC0_DOWNPEND		(1 << 1)
#define SUNXI_LRADC_ADC0_DATAPEND		(1 << 0)

#define SUNXI_LRADC_MODE_0V2
#ifdef	SUNXI_LRADC_MODE_0V2
/*
 * standard of key maping
 * 0.2V mode
 */
#define SUNXI_LRADC_REPORT_KEY_LOW_LIMIT_COUNT	(1)
#endif

#define	SUNXI_LRADC_VENDOR			(0x0001)
#define	SUNXI_LRADC_PRODUCT			(0x0001)
#define	SUNXI_LRADC_VERSION			(0x0100)

#define	SUNXI_LRADC_BITS			(6)
#define SUNXI_LRADC_RESOLUTION			(1 << SUNXI_LRADC_BITS)
#define SUNXI_LRADC_INITIAL_VALUE		(0xff)
#define SUNXI_LRADC_VOL_NUM			SUNXI_LRADC_KEY_MAX_CNT
#define SUNXI_LRADC_KEYMAP_MASK			0x3f
#define SUNXI_LRADC_KEY_MASK			0xff
#define SUNXI_LRADC_KEY_UNIT			8
/*
 * SUNXI_LRADC_MAX_KEYPRESS: The driver can recognize how many keys are pressed at the same time
 * SUNXI_LRADC_MAX_KEYPRESS can be 1, 2, 4;
 */
#define SUNXI_LRADC_MAX_KEYPRESS		2
#define SUNXI_LRADC_KEY_NOCHANGED		0
#define SUNXI_LRADC_KEY_CHANGED_DOWN		1
#define SUNXI_LRADC_KEY_CHANGED_UP		2

static unsigned char keypad_mapindex[SUNXI_LRADC_RESOLUTION];

struct sunxi_lradc_hw_data {
	u32 measure;  /* reference voltage */
	u32 resol;  /* resolution: reference voltage / 64 */
	bool has_clock;  /* flags for the existence of clock and reset resources */
};

static __attribute__((unused)) u32 sunxi_lradc_regs_offset[] = {
	SUNXI_LRADC_CTRL,
	SUNXI_LRADC_INTC,
};

enum jack_pole_sta{
	JACK_PLUG_INIT = 0,
	JACK_PLUG_IN,
	JACK_PLUG_OUT,
};

struct sunxi_lradc {
	struct platform_device	*pdev;
	struct device *dev;
	struct resource *res;
	struct device_node *np;
	struct clk *mclk;
	struct clk *pclk;
	struct reset_control	*rst_clk;
	struct input_dev *input_dev;
	struct sunxi_lradc_hw_data *hw_data;
	spinlock_t lock;  /* syn */
	void __iomem *reg_base;
	u32 scankeycodes[SUNXI_LRADC_KEY_MAX_CNT];
	int irq_num;
	u32 key_val;
	u32 before_code;
	unsigned char compare_later;
	unsigned char compare_before;
	u8 key_code;
	u8 last_key_code;
	char key_name[16];
	u8 key_cnt;
	u32 filter_cnt;
	int key_num;
	int wakeup;
	bool key_debounce;
	bool jack_detect;
	int lradc_chan;
	int debounce_value;
	u32 regs_backup[ARRAY_SIZE(sunxi_lradc_regs_offset)];

	struct extcon_dev *edev;
	struct delayed_work det_scan_work;
	enum jack_pole_sta jack_state;
	enum jack_pole_sta jack_state_old;
};

static const unsigned int lradc_extcon_cable[] = {
	EXTCON_JACK_HEADPHONE,
	EXTCON_NONE,
};

static struct sunxi_lradc_hw_data hw_data_1350 = {
	.measure = 1350,
	.resol = 21,
	.has_clock = true,
};

static struct sunxi_lradc_hw_data hw_data_1350_v100 = {
	.measure = 1350,
	.resol = 21,
	.has_clock = false,
};

static struct sunxi_lradc_hw_data hw_data_1200 = {
	.measure = 1200,
	.resol = 19,
	.has_clock = true,
};

static struct sunxi_lradc_hw_data hw_data_2000 = {
	.measure = 2000,
	.resol = 31,
	.has_clock = true,
};

static struct sunxi_lradc_hw_data hw_data_2000_v100 = {
	.measure = 2000,
	.resol = 31,
	.has_clock = false,
};

/*
 * Translate OpenFirmware node properties into platform_data
 */
static struct of_device_id const sunxi_lradc_of_match[] = {
	{ .compatible = "allwinner,keyboard_1350mv",
		.data = &hw_data_1350 },
	{ .compatible = "allwinner,keyboard_1350mv_v100",
		.data = &hw_data_1350_v100 },
	{ .compatible = "allwinner,keyboard_1200mv",
		.data = &hw_data_1200 },
	{ .compatible = "allwinner,keyboard_2000mv",
		.data = &hw_data_2000 },
	{ .compatible = "allwinner,lradc_2000mv_v100",
		.data = &hw_data_2000_v100 },
	{ },
};
MODULE_DEVICE_TABLE(of, sunxi_lradc_of_match);

static void sunxi_lradc_upirq_control(struct sunxi_lradc *chip, bool irq_en)
{
	u32 reg_val;

	reg_val = readl(chip->reg_base + SUNXI_LRADC_INTC);

	if (irq_en)
		reg_val |= (SUNXI_LRADC_ADC0_UP_EN | SUNXI_LRADC_ADC1_UP_EN);
	else
		reg_val &= ~(SUNXI_LRADC_ADC0_UP_EN | SUNXI_LRADC_ADC1_UP_EN);

	writel(reg_val, chip->reg_base + SUNXI_LRADC_INTC);
}

/*
 * before: last keycode.
 * now: now changed keycode.
 * key: now change key index.
 * report: which key report.
 * return: 0: no changed, 1, key down, 2, key up.
 */
static int sunxi_lradc_key_change(u32 before, u32 now, u32 key, u32 *report)
{
	int i, ret;
	int key_b;
	int keycode = (now >> (SUNXI_LRADC_KEY_UNIT * key)) & SUNXI_LRADC_KEY_MASK;

	if (keycode == 0) {
		for (i = 0; i < SUNXI_LRADC_MAX_KEYPRESS; i++) {
			keycode = (before >> (SUNXI_LRADC_KEY_UNIT * i)) & SUNXI_LRADC_KEY_MASK;
			if (keycode == 0)
				continue;
			ret = sunxi_lradc_key_change(now, before, i, report);
			if (ret == SUNXI_LRADC_KEY_CHANGED_DOWN) {
				*report = keycode;
				return SUNXI_LRADC_KEY_CHANGED_UP;
			}
		}
		return SUNXI_LRADC_KEY_NOCHANGED;
	}

	for (i = 0; i < SUNXI_LRADC_MAX_KEYPRESS; i++) {
		key_b = (before >> (SUNXI_LRADC_KEY_UNIT * i)) & SUNXI_LRADC_KEY_MASK;
		if (key_b == keycode)
			return SUNXI_LRADC_KEY_NOCHANGED;
	}
	*report = keycode;
	return SUNXI_LRADC_KEY_CHANGED_DOWN;
}

static void sunxi_lradc_keydown_event(struct sunxi_lradc *chip)
{
	u32 scancode;
	int i;
	int key;
	int changed;

	chip->compare_later = chip->compare_before;
	scancode = chip->scankeycodes[chip->key_code];
	if (chip->before_code != scancode) {
		for (i = 0; i < SUNXI_LRADC_MAX_KEYPRESS; i++) {
			key = 0;
			changed = sunxi_lradc_key_change(chip->before_code, scancode, i, &key);
			if (changed == SUNXI_LRADC_KEY_CHANGED_DOWN) {
				dev_dbg(chip->dev, "before : %d, scancode : %d, key : %d, down : 1\n",
					chip->before_code, scancode, key);
				input_report_key(chip->input_dev, key, 1);
				input_sync(chip->input_dev);
			} else if (changed == SUNXI_LRADC_KEY_CHANGED_UP) {
				dev_dbg(chip->dev, "before : %d, scancode : %d, key : %d, down : 0\n",
					chip->before_code, scancode, key);
				input_report_key(chip->input_dev, key, 0);
				input_sync(chip->input_dev);
			}
		}
	}
	chip->before_code = scancode;
}

static irqreturn_t sunxi_lradc_irq_handler(int irq, void *dummy)
{
	struct sunxi_lradc *chip = (struct sunxi_lradc *)dummy;
	u32 reg_val;
	u32 key_val;
	int i;
	int key;

	dev_dbg(chip->dev, "Key Interrupt\n");

	reg_val = readl(chip->reg_base + SUNXI_LRADC_INT_STA);
	if (reg_val & SUNXI_LRADC_ADC0_DOWNPEND)
		dev_dbg(chip->dev, "key down\n");

	if (chip->jack_detect) {
		if (chip->lradc_chan == 1) {
			if (reg_val & SUNXI_LRADC_ADC1_DOWNPEND) {
				chip->jack_state = JACK_PLUG_IN;
				schedule_delayed_work(&chip->det_scan_work, msecs_to_jiffies(50));
			} else if (reg_val & SUNXI_LRADC_ADC1_UPPEND) {
				chip->jack_state = JACK_PLUG_OUT;
				schedule_delayed_work(&chip->det_scan_work, msecs_to_jiffies(50));
			}
		} else if (chip->lradc_chan == 0) {
			if (reg_val & SUNXI_LRADC_ADC0_DOWNPEND) {
				chip->jack_state = JACK_PLUG_IN;
				schedule_delayed_work(&chip->det_scan_work, msecs_to_jiffies(50));
			} else if (reg_val & SUNXI_LRADC_ADC0_UPPEND) {
				chip->jack_state = JACK_PLUG_OUT;
				schedule_delayed_work(&chip->det_scan_work, msecs_to_jiffies(50));
			}
		}
	}

	if (reg_val & SUNXI_LRADC_ADC0_DATAPEND) {
		key_val = readl(chip->reg_base + SUNXI_LRADC_DATA0);
		chip->compare_before = key_val & SUNXI_LRADC_KEYMAP_MASK;
		dev_dbg(chip->dev, "vol_in is %d mv\n", chip->compare_before * chip->hw_data->resol);
		if (chip->compare_before >= chip->compare_later - 1
				&& chip->compare_before <= chip->compare_later + 1)
			chip->key_cnt++;
		else
			chip->key_cnt = 0;

		chip->compare_later = chip->compare_before;

		/* when the voltage for filter_cnt consecutive times remains the same,
		 * the condition of reporting the key is met;
		 * it can avoid the problem of voltage mutation caused by unfiltered hardware
		 */
		if (chip->key_cnt >= chip->filter_cnt) {
			dev_dbg(chip->dev, "vol_port is %d mv\n", chip->compare_before * chip->hw_data->resol);
			chip->key_code = keypad_mapindex[key_val & SUNXI_LRADC_KEYMAP_MASK];
			if (chip->key_code != chip->key_num) {
				sunxi_lradc_keydown_event(chip);
			}

			chip->compare_later = 0;
			chip->key_cnt = 0;
			/* enable up irq */
			sunxi_lradc_upirq_control(chip, true);
		}
	}

	if (reg_val & SUNXI_LRADC_ADC0_UPPEND) {
		if (chip->wakeup)
			pm_wakeup_event(chip->input_dev->dev.parent, 0);

		for (i = 0; i < SUNXI_LRADC_MAX_KEYPRESS; i++) {
			key = (chip->before_code >> (SUNXI_LRADC_KEY_UNIT * i)) & SUNXI_LRADC_KEY_MASK;
			if (key > 0) {
				dev_dbg(chip->dev, "report : %d, key : %d\n", chip->before_code, key);
				input_report_key(chip->input_dev, key, 0);
				input_sync(chip->input_dev);
			}
		}
		dev_dbg(chip->dev, "key up\n");

		chip->key_cnt = 0;
		chip->compare_later = 0;
		chip->before_code = 0;
		chip->last_key_code = SUNXI_LRADC_INITIAL_VALUE;
		/* disable up irq */
		sunxi_lradc_upirq_control(chip, false);
	}

	/* Clear interrupt register */
	writel(reg_val, chip->reg_base + SUNXI_LRADC_INT_STA);

	return IRQ_HANDLED;
}

static int sunxi_lradc_key_setup(struct sunxi_lradc *chip)
{
	int i;
	int j = 0;
	u32 val[2] = {0, 0};
	u32 key_vol[SUNXI_LRADC_VOL_NUM];

	if (of_property_read_u32(chip->np, "key_cnt", &chip->key_num)) {
		dev_err(chip->dev, "%s: get key count failed", __func__);
		return -EBUSY;
	}
	dev_dbg(chip->dev, "%s key number = %d.\n", __func__, chip->key_num);
	if (chip->key_num < 1 || chip->key_num >= SUNXI_LRADC_VOL_NUM) {
		dev_err(chip->dev, "incorrect key number.\n");
		return -EINVAL;
	}
	for (i = 0; i < chip->key_num; i++) {
		sprintf(chip->key_name, "key%d", i);
		if (of_property_read_u32_array(chip->np, chip->key_name,
						val, ARRAY_SIZE(val))) {
			dev_err(chip->dev, "%s:get%s err!\n", __func__, chip->key_name);
			return -EBUSY;
		}
		key_vol[i] = val[0];
		chip->scankeycodes[i] = val[1];
		dev_dbg(chip->dev, "%s: key%d vol= %d code= %d\n", __func__, i,
				key_vol[i], chip->scankeycodes[i]);
	}

	/* set the key judgment threshold */
	key_vol[chip->key_num] = chip->hw_data->measure;
	if (chip->key_debounce == 0) {
		for (i = 0; i < chip->key_num; i++)
			key_vol[i] += (key_vol[i+1] - key_vol[i])/2;
		for (i = 0; i < 64; i++) {
			if (i * chip->hw_data->resol > key_vol[j])
				j++;
			keypad_mapindex[i] = j;
		}
	} else {
		for (i = 0; i < 64; i++) {
			if (i * chip->hw_data->resol > key_vol[j] + chip->debounce_value)
				j++;
			if (i * chip->hw_data->resol > key_vol[j] - chip->debounce_value)
				keypad_mapindex[i] = j;
			else
				keypad_mapindex[i] = chip->key_num;
		}
	}

	chip->last_key_code = SUNXI_LRADC_INITIAL_VALUE;

	return 0;
}

static void sunxi_lradc_key_destroy(struct sunxi_lradc *chip)
{
	/* TODO:
	 * If there is an operation that needs to be released later, you can add it directly.
	 */
}

static int sunxi_lradc_resource_get(struct sunxi_lradc *chip)
{
	int err;
	int debounce_value;
	const struct of_device_id *match;

	chip->np = chip->dev->of_node;
	match = of_match_node(sunxi_lradc_of_match, chip->np);
	chip->hw_data = (struct sunxi_lradc_hw_data *)match->data;

	chip->key_debounce = of_property_read_bool(chip->np, "key_debounce");
	if (!chip->key_debounce)
		dev_warn(chip->dev, "get key_debounce failed, please check whether to enable key debounce\n");
	else {
		err = of_property_read_u32(chip->np, "debounce_value", &debounce_value);
		if (err) {
			dev_err(chip->dev, "get debounce_value failed\n");
			debounce_value = DEFAULT_DEBOUNCE_VALUE ;
		}
		chip->debounce_value = debounce_value;
	}

	chip->res = platform_get_resource(chip->pdev, IORESOURCE_MEM, 0);
	if (!chip->res) {
		dev_err(chip->dev, "get resource failed\n");
		return -ENXIO;
	}

	chip->reg_base = devm_ioremap_resource(chip->dev, chip->res);
	if (IS_ERR(chip->reg_base)) {
		dev_err(chip->dev, "get ioremap resource failed\n");
		return PTR_ERR(chip->reg_base);
	}

	chip->irq_num = platform_get_irq(chip->pdev, 0);
	if (chip->irq_num < 0)
		return chip->irq_num;

	if (of_property_read_u32(chip->np, "filter-cnt", &chip->filter_cnt)) {
		dev_warn(chip->dev, "warn: filter_cnt not set, default value is 3\n");
		chip->filter_cnt = 3;
	}

	err = devm_request_irq(chip->dev, chip->irq_num, sunxi_lradc_irq_handler, 0, "sunxikbd", chip);
	if (err) {
		dev_err(chip->dev, "request irq failed\n");
		return err;
	}

	err = sunxi_lradc_key_setup(chip);
	if (err) {
		dev_err(chip->dev, "key init failed\n");
		return err;
	}

	/* If there are clock resources, has_clock is true, apply for clock resources;
	 * otherwise, has_clock is false, you do not need to apply for clock resources.
	 */
	if (chip->hw_data->has_clock) {
		chip->mclk = devm_clk_get(chip->dev, NULL);
		if (IS_ERR(chip->mclk)) {
			dev_err(chip->dev, "get clock failed\n");
			return PTR_ERR(chip->mclk);
		}

		chip->rst_clk = devm_reset_control_get(chip->dev, NULL);
		if (IS_ERR(chip->rst_clk)) {
			dev_err(chip->dev, "reset_control_get failed\n");
			return PTR_ERR(chip->rst_clk);
		}
	}

	chip->wakeup = of_property_read_bool(chip->np, "wakeup-source");
	if (chip->wakeup)
		device_init_wakeup(chip->dev, chip->wakeup);

	chip->jack_detect = of_property_read_bool(chip->np, "jack-detect");
	if (chip->jack_detect) {
		if (of_property_read_u32(chip->np, "jack-detect", &chip->lradc_chan)) {
			dev_dbg(chip->dev, "jack-detect lradc_chan not set, default value is 0\n");
			chip->lradc_chan = 0;
		}
		dev_dbg(chip->dev, "lradc supports jack detection.\n");
	}

	return 0;
}

static void sunxi_lradc_resource_put(struct sunxi_lradc *chip)
{
	if (chip->wakeup)
		device_init_wakeup(chip->dev, 0);

	sunxi_lradc_key_destroy(chip);
}

static int sunxi_lradc_clk_enable(struct sunxi_lradc *chip)
{
	int err;

	err = reset_control_deassert(chip->rst_clk);
	if (err) {
		dev_err(chip->dev, "reset_control_deassert failed!\n");
		return err;
	}

	err = clk_prepare_enable(chip->mclk);
	if (err) {
		dev_err(chip->dev, "clock enable failed\n");
		goto err0;
	}

	return 0;

err0:
	reset_control_assert(chip->rst_clk);
	return err;
}

static void sunxi_lradc_clk_disable(struct sunxi_lradc *chip)
{
	clk_disable_unprepare(chip->mclk);
	reset_control_assert(chip->rst_clk);
}

static int sunxi_lradc_hw_init(struct sunxi_lradc *chip)
{
	int err;
	u32 reg_val;
	u32 key_val;
	unsigned long mask, para;

	if (chip->hw_data->has_clock) {
		err = sunxi_lradc_clk_enable(chip);
		if (err) {
			dev_err(chip->dev, "init lradc clock failed\n");
			return err;
		}
	}

	reg_val = readl(chip->reg_base + SUNXI_LRADC_INTC);
	para = SUNXI_LRADC_ADC0_DOWN_EN | SUNXI_LRADC_ADC0_DATA_EN | \
			SUNXI_LRADC_ADC1_DOWN_EN | SUNXI_LRADC_ADC1_DATA_EN;
	mask = SUNXI_LRADC_ADC0_UP_EN_MASK | SUNXI_LRADC_ADC0_DOWN_EN_MASK | \
			SUNXI_LRADC_ADC0_DATA_EN_MASK | SUNXI_LRADC_ADC1_UP_EN_MASK | \
			SUNXI_LRADC_ADC1_DOWN_EN_MASK | SUNXI_LRADC_ADC1_DATA_EN_MASK;
	reg_val &= ~mask;
	reg_val |= para;
	writel(reg_val, chip->reg_base + SUNXI_LRADC_INTC);

	reg_val = readl(chip->reg_base + SUNXI_LRADC_CTRL);
	para = SUNXI_LRADC_FIRST_CONVERT_DLY | SUNXI_LRADC_LEVELB_VOL | SUNXI_LRADC_KEY_MODE_SELECT
		| SUNXI_LRADC_HOLD_EN | SUNXI_LRADC_CHAN_SELECT | SUNXI_LRADC_LEVELB_CNT
		| SUNXI_LRADC_SAMPLE_250HZ | SUNXI_LRADC_EN;
	mask = SUNXI_LRADC_FIRST_CONVERT_DLY_MASK | SUNXI_LRADC_LEVELB_VOL_MASK | SUNXI_LRADC_KEY_MODE_SELECT_MASK
		| SUNXI_LRADC_HOLD_EN_MASK | SUNXI_LRADC_LEVELB_CNT_MASK
		| SUNXI_LRADC_SAMPLE_250HZ_MASK | SUNXI_LRADC_EN_MASK;
	reg_val &= ~mask;
	reg_val |= para;

	if (chip->jack_detect)
		reg_val |= SUNXI_LRADC_LEVELB_VOL_JACK_DETECT; /* jack detection, set levelB vol */

	writel(reg_val, chip->reg_base + SUNXI_LRADC_CTRL);

	if (chip->jack_detect) {
		key_val = readl(chip->reg_base + SUNXI_LRADC_DATA1);
		key_val &= SUNXI_LRADC_KEYMAP_MASK;
		key_val = key_val * chip->hw_data->resol;
		if (key_val > JACK_DETECT_VOL) {
			chip->jack_state = JACK_PLUG_OUT;
			extcon_set_state(chip->edev, EXTCON_JACK_HEADPHONE, false);
			chip->jack_state_old = chip->jack_state;
		} else {
			chip->jack_state = JACK_PLUG_IN;
			extcon_set_state(chip->edev, EXTCON_JACK_HEADPHONE, true);
			chip->jack_state_old = chip->jack_state;
		}
		dev_dbg(chip->dev, "vol_in is %d mv\n", key_val * chip->hw_data->resol);
	}

	return 0;
}

static void sunxi_lradc_hw_exit(struct sunxi_lradc *chip)
{
	if (chip->hw_data->has_clock)
		sunxi_lradc_clk_disable(chip);

}

static int sunxi_lradc_inputdev_register(struct sunxi_lradc *chip)
{
	int i;
	int err;

	chip->input_dev = devm_input_allocate_device(chip->dev);
	if (!chip->input_dev) {
		dev_err(chip->dev, "input allocate device failed\n");
		return -ENOMEM;
	}

	chip->input_dev->name = SUNXI_LRADC_INPUT_DEV_NAME;
	chip->input_dev->phys = "sunxikbd/input0";
	chip->input_dev->id.bustype = BUS_HOST;
	chip->input_dev->id.vendor = SUNXI_LRADC_VENDOR;
	chip->input_dev->id.product = SUNXI_LRADC_PRODUCT;
	chip->input_dev->id.version = SUNXI_LRADC_VERSION;

#ifdef REPORT_REPEAT_KEY_BY_INPUT_CORE
	chip->input_dev->evbit[0] = BIT_MASK(EV_KEY)|BIT_MASK(EV_REP);
	dev_info(chip->dev, ("support report repeat key value\n");
#else
	chip->input_dev->evbit[0] = BIT_MASK(EV_KEY);
#endif

	for (i = 0; i < SUNXI_LRADC_KEY_MAX_CNT; i++) {
		if (chip->scankeycodes[i] < KEY_MAX)
			set_bit(chip->scankeycodes[i], chip->input_dev->keybit);
	}

	err = input_register_device(chip->input_dev);
	if (err) {
		dev_err(chip->dev, "register input device failed\n");
		return err;
	}

	return 0;
}

static void sunxi_lradc_inputdev_unregister(struct sunxi_lradc *chip)
{
	input_unregister_device(chip->input_dev);
}

#if IS_ENABLED(CONFIG_IIO)
struct sunxi_lradc_iio {
	struct sunxi_lradc *chip;
};

static const struct iio_chan_spec sunxi_lradc_channels[] = {
	{
		.indexed = 1,
		.type = IIO_VOLTAGE,
		.channel = 0,
		.datasheet_name = "LRADC",
		.info_mask_separate = BIT(IIO_CHAN_INFO_RAW),
	},
};

/* default maps used by iio consumer (axp charger driver) */
static struct iio_map sunxi_lradc_default_iio_maps[] = {
	{
		.consumer_dev_name = "axp-charger",
		.consumer_channel = "axp-battery-lradc",
		.adc_channel_label = "LRADC",
	},
	{ }
};

static int sunxi_lradc_read_raw(struct iio_dev *indio_dev,
			struct iio_chan_spec const *chan,
			int *val, int *val2, long mask)
{
	int ret = 0;
	int key_val, id_vol;
	struct sunxi_lradc_iio *info = iio_priv(indio_dev);
	struct sunxi_lradc *chip = info->chip;
	struct sunxi_lradc_hw_data *hw_data = chip->hw_data;

	mutex_lock(&indio_dev->mlock);
	switch (mask) {
	case IIO_CHAN_INFO_RAW:
		key_val = readl(chip->reg_base + SUNXI_LRADC_DATA0) & 0x3f;
		id_vol = key_val * hw_data->resol;
		*val = id_vol;
		break;
	default:
		ret = -EINVAL;
	}
	mutex_unlock(&indio_dev->mlock);

	return ret;
}

static const struct iio_info sunxi_lradc_iio_info = {
	.read_raw = &sunxi_lradc_read_raw,
};

static void sunxi_lradc_remove_iio(void *_data)
{
	struct iio_dev *indio_dev = _data;

	if (IS_ERR_OR_NULL(indio_dev)) {
		pr_err("indio_dev is null\n");
	} else {
		iio_device_unregister(indio_dev);
		iio_map_array_unregister(indio_dev);
	}
}

static int __maybe_unused sunxi_lradc_iio_init(struct platform_device *pdev)
{
	int ret;
	struct iio_dev *indio_dev;
	struct sunxi_lradc_iio *info;
	struct sunxi_lradc *chip = platform_get_drvdata(pdev);

	indio_dev = devm_iio_device_alloc(&pdev->dev, sizeof(*info));
	if (!indio_dev)
		return -ENOMEM;

	info = iio_priv(indio_dev);
	info->chip = chip;

	indio_dev->dev.parent = &pdev->dev;
	indio_dev->name = pdev->name;
	indio_dev->channels = sunxi_lradc_channels;
	indio_dev->num_channels = ARRAY_SIZE(sunxi_lradc_channels);
	indio_dev->info = &sunxi_lradc_iio_info;
	indio_dev->modes = INDIO_DIRECT_MODE;
	ret = iio_map_array_register(indio_dev, sunxi_lradc_default_iio_maps);
	if (ret < 0)
		return ret;

	ret = iio_device_register(indio_dev);
	if (ret < 0) {
		dev_err(&pdev->dev, "unable to register iio device\n");
		goto err_array_unregister;
	}

	ret = devm_add_action(&pdev->dev,
				sunxi_lradc_remove_iio, indio_dev);
	if (ret) {
		dev_err(&pdev->dev, "unable to add iio cleanup action\n");
		goto err_iio_unregister;
	}

	return 0;

err_iio_unregister:
	iio_device_unregister(indio_dev);

err_array_unregister:
	iio_map_array_unregister(indio_dev);

	return ret;
}
#else
static inline int __maybe_unused sunxi_lradc_iio_init(struct platform_device *pdev)
{
	return -ENODEV;
}
#endif

static void sunxi_lradc_jack_det_scan_work(struct work_struct *work)
{
	struct sunxi_lradc *chip = container_of(work, struct sunxi_lradc, det_scan_work.work);

	dev_dbg(chip->dev, "[%s] enter scan work\n", __func__);

	if (chip->jack_state == chip->jack_state_old)
		return;

	if (chip->jack_state == JACK_PLUG_IN) {
		dev_dbg(chip->dev, "jack_detect: lradc%d key down\n", chip->lradc_chan);
		extcon_set_state_sync(chip->edev, EXTCON_JACK_HEADPHONE, true);
	}
	if (chip->jack_state == JACK_PLUG_OUT) {
		dev_dbg(chip->dev, "jack_detect: lradc%d key up\n", chip->lradc_chan);
		extcon_set_state_sync(chip->edev, EXTCON_JACK_HEADPHONE, false);
	}
	chip->jack_state_old = chip->jack_state;
}

static int sunxi_lradc_probe(struct platform_device *pdev)
{
	struct sunxi_lradc *chip;
	int err;

	chip = devm_kzalloc(&pdev->dev, sizeof(*chip), GFP_KERNEL);
	if (ZERO_OR_NULL_PTR(chip))
		return -ENOMEM;

	chip->pdev = pdev;
	chip->dev = &pdev->dev;

	err = sunxi_lradc_resource_get(chip);
	if (err) {
		dev_err(chip->dev, "sunxi lradc get resource failed\n");
		goto err0;
	}

	if (chip->jack_detect) {
		chip->edev = devm_extcon_dev_allocate(chip->dev, lradc_extcon_cable);
		if (IS_ERR(chip->edev)) {
			dev_err(chip->dev, "failed to allocate extcon device\n");
			return -ENOMEM;
		}

		err = devm_extcon_dev_register(chip->dev, chip->edev);
		if (err < 0) {
			dev_err(chip->dev, "failed to register extcon device\n");
			return err;
		}

		INIT_DELAYED_WORK(&chip->det_scan_work, sunxi_lradc_jack_det_scan_work);
		chip->jack_state_old = JACK_PLUG_INIT;
	}

	err = sunxi_lradc_inputdev_register(chip);
	if (err) {
		dev_err(chip->dev, "sunxi lradc inputdev register failed\n");
		goto err1;
	}

	err = sunxi_lradc_hw_init(chip);
	if (err) {
		dev_err(chip->dev, "sunxi lradc hw_init failed\n");
		goto err2;
	}

	platform_set_drvdata(pdev, chip);
	dev_dbg(chip->dev, "sunxi lradc init success\n");
	return 0;

err2:
	sunxi_lradc_inputdev_unregister(chip);
err1:
	sunxi_lradc_resource_put(chip);
err0:
	return err;
}

static int sunxi_lradc_remove(struct platform_device *pdev)
{
	struct sunxi_lradc *chip = platform_get_drvdata(pdev);

	sunxi_lradc_hw_exit(chip);
	sunxi_lradc_inputdev_unregister(chip);
	sunxi_lradc_resource_put(chip);
	return 0;
}

#if IS_ENABLED(CONFIG_PM)
static inline void sunxi_lradc_save_regs(struct sunxi_lradc *chip)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(sunxi_lradc_regs_offset); i++)
		chip->regs_backup[i] = readl(chip->reg_base + sunxi_lradc_regs_offset[i]);
}

static inline void sunxi_lradc_restore_regs(struct sunxi_lradc *chip)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(sunxi_lradc_regs_offset); i++)
		writel(chip->regs_backup[i], chip->reg_base + sunxi_lradc_regs_offset[i]);
}

static int sunxi_lradc_suspend(struct device *dev)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct sunxi_lradc *chip = platform_get_drvdata(pdev);

	dev_dbg(chip->dev, "[%s] enter standby\n", __func__);

	/* Used to determine whether the device can be wakeup, and use this function */
	if (device_may_wakeup(dev)) {
		if (chip->wakeup)
			enable_irq_wake(chip->irq_num);
	} else {
		disable_irq_nosync(chip->irq_num);

		sunxi_lradc_save_regs(chip);

		if (chip->hw_data->has_clock)
			sunxi_lradc_clk_disable(chip);
	}

	return 0;
}

static int sunxi_lradc_resume(struct device *dev)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct sunxi_lradc *chip = platform_get_drvdata(pdev);
	int err;
	u32 key_val;

	dev_dbg(chip->dev, "[%s] return from standby\n", __func__);

	/* Used to determine whether the device can be wakeup, and use this function */
	if (device_may_wakeup(dev)) {
		if (chip->wakeup)
			disable_irq_wake(chip->irq_num);
	} else {
		if (chip->hw_data->has_clock) {
			err = sunxi_lradc_clk_enable(chip);
			if (err) {
				dev_err(chip->dev, "init lradc clock failed\n");
				return err;
			}
		}
		sunxi_lradc_restore_regs(chip);
		enable_irq(chip->irq_num);
	}

	if (chip->jack_detect) {
		key_val = readl(chip->reg_base + SUNXI_LRADC_DATA1);
		key_val &= SUNXI_LRADC_KEYMAP_MASK;
		key_val = key_val * chip->hw_data->resol;
		if (key_val > JACK_DETECT_VOL) {
			chip->jack_state = JACK_PLUG_OUT;
			extcon_set_state(chip->edev, EXTCON_JACK_HEADPHONE, false);
			chip->jack_state_old = chip->jack_state;
		} else {
			chip->jack_state = JACK_PLUG_IN;
			extcon_set_state(chip->edev, EXTCON_JACK_HEADPHONE, true);
			chip->jack_state_old = chip->jack_state;
		}
		dev_dbg(chip->dev, "vol_in is %d mv\n", key_val * chip->hw_data->resol);
	}

	return 0;
}

static const struct dev_pm_ops sunxi_lradc_pm_ops = {
	.suspend = sunxi_lradc_suspend,
	.resume = sunxi_lradc_resume,
};

#define SUNXI_LRADC_PM_OPS (&sunxi_lradc_pm_ops)
#else
#define SUNXI_LRADC_PM_OPS NULL
#endif

static struct platform_driver sunxi_lradc_driver = {
	.probe  = sunxi_lradc_probe,
	.remove = sunxi_lradc_remove,
	.driver = {
		.name   = "sunxi-lradc",
		.owner  = THIS_MODULE,
		.pm	= SUNXI_LRADC_PM_OPS,
		.of_match_table = of_match_ptr(sunxi_lradc_of_match),
	},
};

static int __init sunxi_lradc_init(void)
{
	int ret;

	ret = platform_driver_register(&sunxi_lradc_driver);

	return ret;
}

static void __exit sunxi_lradc_exit(void)
{
	platform_driver_unregister(&sunxi_lradc_driver);
}

subsys_initcall_sync(sunxi_lradc_init);
module_exit(sunxi_lradc_exit);

MODULE_AUTHOR("Qin");
MODULE_AUTHOR("shaosidi <shaosidi@allwinnertech.com>");
MODULE_DESCRIPTION("sunxi lradc driver");
MODULE_LICENSE("GPL");
MODULE_VERSION("2.0.6");
