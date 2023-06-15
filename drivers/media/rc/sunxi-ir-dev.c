/*
 * Copyright (c) 2007-2020 allwinnertech Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */
//#define DEBUG
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/gpio.h>
#include <linux/slab.h>
#include <linux/clk.h>
#include <linux/of_gpio.h>
#include <linux/platform_device.h>
#include <linux/regulator/consumer.h>
#include <linux/irq.h>
#include <linux/of_platform.h>
#include <linux/of_irq.h>
#include <linux/of_address.h>
#include <media/rc-core.h>
//#include <linux/arisc/arisc.h>
#include <linux/delay.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include "sunxi-ir-rx.h"
#include "rc-core-priv.h"

static u32 is_receiving;
static u32 threshold_low = RC5_UNIT + RC5_UNIT / 2;
static u32 threshold_high = 2 * RC5_UNIT + RC5_UNIT / 2;
static bool pluse_pre;
static char protocol;
static bool boot_code;

struct sunxi_ir_rx_data *ir_rx_data;
static struct ir_raw_event rawir;
static void __iomem *vaddr;

static inline u32 ir_get_data(void __iomem *reg_base)
{
	return readl(reg_base + IR_RXDAT_REG);
}

static inline u32 ir_get_intsta(void __iomem *reg_base)
{
	return readl(reg_base + IR_RXINTS_REG);
}

static inline void ir_clr_intsta(void __iomem *reg_base, u32 bitmap)
{
	u32 reg_val;

	reg_val = readl(reg_base + IR_RXINTS_REG);
	reg_val &= ~0xff;
	reg_val |= bitmap & 0xff;
	writel(reg_val, reg_base + IR_RXINTS_REG);
}

#ifdef CONFIG_OF
/* Translate OpenFirmware node properties into platform_data */
static struct of_device_id const sunxi_ir_recv_of_match[] = {
	{ .compatible = "allwinner,s_cir", },
	{ .compatible = "allwinner,ir", },
	{ },
};
MODULE_DEVICE_TABLE(of, sunxi_ir_recv_of_match);
#else /* !CONFIG_OF */
#endif

static void sunxi_ir_recv(u32 reg_data, struct sunxi_ir_rx_data *ir_rx_data)
{
	bool pluse_now = 0;
	u32 ir_duration = 0;

	pluse_now = reg_data >> 7; /* get the polarity */
	ir_duration = reg_data & 0x7f; /* get duration, number of clocks */

	if (pluse_pre == pluse_now) {
		/* the signal sunperposition */
		rawir.duration += ir_duration;
		pr_debug("raw: polar=%d; dur=%d\n", pluse_now, ir_duration);
	} else {
		if (is_receiving) {
			rawir.duration *= IR_SIMPLE_UNIT;
			pr_debug("pusle :polar=%d, dur: %u ns\n",
						rawir.pulse, rawir.duration);
			if (boot_code == 0) {
				boot_code = 1;
				if (eq_margin(rawir.duration, NEC_BOOT_CODE,
							NEC_UNIT*2)) {
					protocol = NEC;
					ir_raw_event_store(ir_rx_data->rcdev, &rawir);
				} else {
					protocol = RC5;
					ir_raw_event_store(ir_rx_data->rcdev, &rawir);
				}
			} else {
				if (((rawir.duration > threshold_low) &&
					(rawir.duration < threshold_high)) &&
						(protocol == RC5)) {
					rawir.duration = rawir.duration / 2;
					ir_raw_event_store(ir_rx_data->rcdev, &rawir);
					//ir_raw_event_store(ir_rx_data->rcdev, &rawir);
				} else
					ir_raw_event_store(ir_rx_data->rcdev, &rawir);
			}
			rawir.pulse = pluse_now;
			rawir.duration = ir_duration;
			pr_debug("raw: polar=%d; dur=%d\n",
						pluse_now, ir_duration);
		} else {
				/* get the first pluse signal */
				rawir.pulse = pluse_now;
				rawir.duration = ir_duration;
				is_receiving = 1;
				pr_debug("get frist pulse,add head !!\n");
				pr_debug("raw: polar=%d; dur=%d\n", pluse_now,
						ir_duration);
		}
		pluse_pre = pluse_now;
	}
}

static irqreturn_t sunxi_ir_recv_irq(int irq, void *dev_id)
{
	struct sunxi_ir_rx_data *ir_rx_data = (struct sunxi_ir_rx_data *)dev_id;
	u32 intsta, dcnt;
	u32 i = 0;
	u32 reg_data;

	pr_debug("IR RX IRQ Serve\n");

	intsta = ir_get_intsta(ir_rx_data->reg_base);
	ir_clr_intsta(ir_rx_data->reg_base, intsta);

	/* get the count of signal */
	dcnt =	(intsta>>8) & 0x7f;
	pr_debug("receive cnt :%d\n", dcnt);
	/* Read FIFO and fill the raw event */
	for (i = 0; i < dcnt; i++) {
		/* get the data from fifo */
		reg_data = ir_get_data(ir_rx_data->reg_base);
		/* Byte in FIFO format YXXXXXXX(B)
		 * Y:polarity(0:low level, 1:high level)
		 * X:Number of clocks
		 */
		sunxi_ir_recv(reg_data, ir_rx_data);
	}

	if (intsta & IR_RXINTS_RXPE) {
		/* The last pulse can not call ir_raw_event_store() since miss
		 * invert level in above, manu call
		 */
		if (rawir.duration) {
			rawir.duration *= IR_SIMPLE_UNIT;
			pr_debug("pusle :polar=%d, dur: %u ns\n",
						rawir.pulse, rawir.duration);
			ir_raw_event_store(ir_rx_data->rcdev, &rawir);
		}
		pr_debug("handle raw data.\n");
		/* handle ther decoder theread */
		ir_raw_event_handle(ir_rx_data->rcdev);
		is_receiving = 0;
		boot_code = 0;
		pluse_pre = false;

		if (ir_rx_data->wakeup)
			pm_wakeup_event(ir_rx_data->rcdev->input_dev->dev.parent, 0);
	}

	if (intsta & IR_RXINTS_RXOF) {
		/* FIFO Overflow */
		pr_err("ir_rx_irq_service: Rx FIFO Overflow!!\n");
		is_receiving = 0;
		boot_code = 0;
		pluse_pre = false;
	}

	return IRQ_HANDLED;
}

static void ir_mode_set(void __iomem *reg_base, enum ir_mode set_mode)
{
	u32 ctrl_reg = 0;

	switch (set_mode) {
	case CIR_MODE_ENABLE:
		ctrl_reg = readl(reg_base + IR_CTRL_REG);
		ctrl_reg |= IR_CIR_MODE;
		break;
	case IR_MODULE_ENABLE:
		ctrl_reg = readl(reg_base + IR_CTRL_REG);
		ctrl_reg |= IR_ENTIRE_ENABLE;
		break;
	case IR_BOTH_PULSE_MODE:
		ctrl_reg = readl(reg_base + IR_CTRL_REG);
		ctrl_reg |= IR_BOTH_PULSE;
		break;
	case IR_LOW_PULSE_MODE:
		ctrl_reg = readl(reg_base + IR_CTRL_REG);
		ctrl_reg |= IR_LOW_PULSE;
		break;
	case IR_HIGH_PULSE_MODE:
		ctrl_reg = readl(reg_base + IR_CTRL_REG);
		ctrl_reg |= IR_HIGH_PULSE;
		break;
	default:
		pr_err("ir_mode_set error!!\n");
		return;
	}
	writel(ctrl_reg, reg_base + IR_CTRL_REG);
}

static void ir_sample_config(void __iomem *reg_base,
					enum ir_sample_config set_sample)
{
	u32 sample_reg = 0;

	sample_reg = readl(reg_base + IR_SPLCFG_REG);

	switch (set_sample) {
	case IR_SAMPLE_REG_CLEAR:
		sample_reg = 0;
		break;
	case IR_CLK_SAMPLE:
		sample_reg |= IR_SAMPLE_DEV;
		break;
	case IR_FILTER_TH_NEC:
		sample_reg |= IR_RXFILT_VAL;
		break;
	case IR_FILTER_TH_RC5:
		sample_reg |= IR_RXFILT_VAL_RC5;
		break;
	case IR_IDLE_TH:
		sample_reg |= IR_RXIDLE_VAL;
		break;
	case IR_ACTIVE_TH:
		sample_reg |= IR_ACTIVE_T;
		sample_reg |= IR_ACTIVE_T_C;
		break;
	case IR_ACTIVE_TH_SAMPLE:
		sample_reg |= IR_ACTIVE_T_SAMPLE;
		sample_reg &= ~IR_ACTIVE_T_C;
		break;
	default:
		return;
	}
	writel(sample_reg, reg_base + IR_SPLCFG_REG);
}

static void ir_signal_invert(void __iomem *reg_base)
{
	u32 reg_val;

	reg_val = 0x1 << 2;
	writel(reg_val, reg_base + IR_RXCFG_REG);
}

static void ir_irq_config(void __iomem *reg_base, enum ir_irq_config set_irq)
{
	u32 irq_reg = 0;

	switch (set_irq) {
	case IR_IRQ_STATUS_CLEAR:
		writel(0xef, reg_base + IR_RXINTS_REG);
		return;
	case IR_IRQ_ENABLE:
		irq_reg = readl(reg_base + IR_RXINTE_REG);
		irq_reg |= IR_IRQ_STATUS;
		break;
	case IR_IRQ_FIFO_SIZE:
		irq_reg = readl(reg_base + IR_RXINTE_REG);
		irq_reg |= IR_FIFO_20;
		break;
	default:
		return;
	}
	writel(irq_reg, reg_base + IR_RXINTE_REG);
}

static void ir_reg_cfg(void __iomem *reg_base)
{
	/* Enable IR Mode */
	ir_mode_set(reg_base, CIR_MODE_ENABLE);
	/* Config IR Smaple Register */
	ir_sample_config(reg_base, IR_SAMPLE_REG_CLEAR);
	ir_sample_config(reg_base, IR_CLK_SAMPLE);
	ir_sample_config(reg_base, IR_IDLE_TH); /* Set Idle Threshold */

	/* rc5 Set Active Threshold */
	ir_sample_config(reg_base, IR_ACTIVE_TH_SAMPLE);
	ir_sample_config(reg_base, IR_FILTER_TH_NEC); /* Set Filter Threshold */
	ir_signal_invert(reg_base);
	/* Clear All Rx Interrupt Status */
	ir_irq_config(reg_base, IR_IRQ_STATUS_CLEAR);
	/* Set Rx Interrupt Enable */
	ir_irq_config(reg_base, IR_IRQ_ENABLE);

	/* Rx FIFO Threshold = FIFOsz/2; */
	ir_irq_config(reg_base, IR_IRQ_FIFO_SIZE);
	/* for NEC decode which start with high level in the header so should
	 * use IR_HIGH_PULSE_MODE mode, but some ICs don't support this function
	 * therefor use IR_BOTH_PULSE_MODE mode as default
	 */
	ir_mode_set(reg_base, IR_HIGH_PULSE_MODE);
	/* Enable IR Module */
	ir_mode_set(reg_base, IR_MODULE_ENABLE);
}

static int ir_clk_cfg(struct sunxi_ir_rx_data *ir_rx_data)
{

	unsigned long rate = 0;
	int ret = 0;

	ret = reset_control_deassert(ir_rx_data->reset);
	if (ret) {
		pr_err("deassert ir rx rst failed!\n");
		return ret;
	}

	rate = clk_get_rate(ir_rx_data->bclk);
	pr_debug("%s: get ir bus clk rate %dHZ\n", __func__, (__u32)rate);

	rate = clk_get_rate(ir_rx_data->pclk);
	pr_debug("%s: get ir parent clk rate %dHZ\n", __func__, (__u32)rate);

	ret = clk_set_parent(ir_rx_data->mclk, ir_rx_data->pclk);
	if (ret) {
		pr_err("%s: set ir_clk parent failed!\n", __func__);
		return ret;
	}

	ret = clk_set_rate(ir_rx_data->mclk, IR_CLK);
	if (ret) {
		pr_err("set ir clock freq to %d failed!\n", IR_CLK);
		return ret;
	}

	rate = clk_get_rate(ir_rx_data->mclk);
	pr_debug("%s: get ir_clk rate %dHZ\n", __func__, (__u32)rate);

	ret = clk_prepare_enable(ir_rx_data->bclk);
	if (ret) {
		pr_err("try to enable bus clk failed!\n");
		goto assert_reset;
	}

	ret = clk_prepare_enable(ir_rx_data->mclk);
	if (ret) {
		pr_err("try to enable ir_clk failed!\n");
		goto assert_reset;
	}

	return 0;

assert_reset:
	reset_control_assert(ir_rx_data->reset);

	return ret;
}

#ifdef CONFIG_PM
static void ir_clk_uncfg(struct sunxi_ir_rx_data *ir_rx_data)
{

	if (IS_ERR_OR_NULL(ir_rx_data->mclk))
		pr_err("ir_clk handle is invalid, just return!\n");
	else {
		clk_disable_unprepare(ir_rx_data->mclk);
	}

	if (IS_ERR_OR_NULL(ir_rx_data->bclk))
		pr_err("bus clk handle is invalid, just return!\n");
	else {
		clk_disable_unprepare(ir_rx_data->bclk);
	}

	reset_control_assert(ir_rx_data->reset);
/*
	if (IS_ERR_OR_NULL(ir_rx_data->pclk))
		pr_err("ir_clk_source handle is invalid, just return!\n");
	else {
		clk_disable_unprepare(ir_rx_data->pclk);
	}
*/
}
#endif

static int ir_select_gpio_state(struct pinctrl *pctrl, char *name)
{
	int ret = 0;
	struct pinctrl_state *pctrl_state = NULL;

	pctrl_state = pinctrl_lookup_state(pctrl, name);
	if (IS_ERR(pctrl_state)) {
		pr_err("IR pinctrl_lookup_state(%s) failed! return %p \n",
				name, pctrl_state);
		return -1;
	}

	ret = pinctrl_select_state(pctrl, pctrl_state);
	if (ret) {
		pr_err("IR pinctrl_select_state(%s) failed! return %d \n",
				name, ret);
		return ret;
	}

	return 0;
}

static int ir_request_gpio(struct sunxi_ir_rx_data *ir_rx_data)
{
	ir_rx_data->pctrl = devm_pinctrl_get(&ir_rx_data->pdev->dev);
	if (IS_ERR(ir_rx_data->pctrl)) {
		pr_err("IR devm_pinctrl_get() failed!\n");
		return -1;
	}

	return ir_select_gpio_state(ir_rx_data->pctrl, PINCTRL_STATE_DEFAULT);
}

static int ir_setup(struct sunxi_ir_rx_data *ir_rx_data)
{
	int ret = 0;

	ret = ir_request_gpio(ir_rx_data);
	if (ret) {
		pr_err("ir request i2c gpio failed!\n");
		return -1;
	}

	ret = ir_clk_cfg(ir_rx_data);
	if (ret) {
		pr_err("ir rx clk configure failed!\n");
		return ret;
	}

	ir_reg_cfg(ir_rx_data->reg_base);

	return 0;
}

#ifdef CONFIG_ANDROID
/*export node: /proc/sunxi_ir_protocol*/
static ssize_t sunxi_ir_protocol_read(struct file *file,
			char __user *buf, size_t size, loff_t *ppos)
{
	if (size != sizeof(unsigned int))
		return -1;

	if (copy_to_user((void __user *)buf, &ir_rx_data->ir_protocols, size))
		return -1;

	return size;
}

static const struct file_operations sunxi_ir_proc_fops = {
	.read		= sunxi_ir_protocol_read,
};

static	struct proc_dir_entry *ir_protocol_dir;
static bool sunxi_get_ir_protocol(void)
{
	ir_protocol_dir = proc_create(
		(const char *)"sunxi_ir_protocol",
		(umode_t)0400, NULL, &sunxi_ir_proc_fops);

	if (!ir_protocol_dir)
		return true;

	return false;
}
#endif

static int sunxi_ir_startup(struct platform_device *pdev,
				struct sunxi_ir_rx_data *ir_rx_data)
{
	struct device_node *np = NULL;
	struct device *dev = NULL;
	__maybe_unused char ir_supply[16] = {0};
	__maybe_unused const char *name = NULL;
	int ret = 0;
#ifdef CONFIG_ANDROID
	int i = 0;
	char addr_name[MAX_ADDR_NUM];
#endif
	np = pdev->dev.of_node;
	dev = &pdev->dev;

	ir_rx_data->reg_base = of_iomap(np, 0);
	if (ir_rx_data->reg_base == NULL) {
		pr_err("%s:Failed to ioremap() io memory region.\n", __func__);
		return -EBUSY;
	}
	pr_debug("ir base: %p !\n", ir_rx_data->reg_base);

	vaddr = ioremap(RTC_REG, 1);
	if (!vaddr) {
		pr_err("ioremap fail\n");
		ret = -EBUSY;
		goto out_iounmap1;
	}

	ir_rx_data->irq_num = irq_of_parse_and_map(np, 0);
	if (ir_rx_data->irq_num == 0) {
		pr_err("%s:Failed to map irq.\n", __func__);
		ret = -EBUSY;
		goto out_iounmap2;
	}
	pr_debug("ir irq num: %d !\n", ir_rx_data->irq_num);

	ir_rx_data->reset = devm_reset_control_get(&pdev->dev, NULL);
	if (IS_ERR(ir_rx_data->reset)) {
		pr_err("Failed to get reset handle!\n");
		ret = PTR_ERR(ir_rx_data->reset);
		goto out_dispose_mapping;
	}

	ir_rx_data->bclk = devm_clk_get(&pdev->dev, "bus");
	if (IS_ERR_OR_NULL(ir_rx_data->bclk)) {
		pr_err("%s:Failed to get bus clk.\n", __func__);
		ret = -EBUSY;
		goto out_dispose_mapping;
	}

	ir_rx_data->pclk = devm_clk_get(&pdev->dev, "pclk");
	if (IS_ERR_OR_NULL(ir_rx_data->pclk)) {
		pr_err("%s:Failed to get parent clk.\n", __func__);
		ret = -EBUSY;
		goto out_dispose_mapping;
	}

	ir_rx_data->mclk = devm_clk_get(&pdev->dev, "mclk");
	if (IS_ERR_OR_NULL(ir_rx_data->mclk)) {
		pr_err("%s:Failed to get ir rx clk.\n", __func__);
		ret = -EBUSY;
		goto out_dispose_mapping;
	}

	if (of_property_read_u32(np, "ir_protocol_used",
				&ir_rx_data->ir_protocols)) {
		pr_err("%s: get ir protocol failed", __func__);
		ir_rx_data->ir_protocols = 0x0;
	}
#ifdef CONFIG_ANDROID
/*get iraddr,powerkey,... from sysconfig, CHECK later*/
if (ir_rx_data->ir_protocols == NEC) {
	for (i = 0; i < MAX_ADDR_NUM; i++) {
		sprintf(addr_name, "ir_addr_code%d", i);
		if (of_property_read_u32(np, (const char *)&addr_name,
					&ir_rx_data->ir_addr[i])) {
			break;
		}
	}

	ir_rx_data->ir_addr_cnt = i;
	for (i = 0; i < ir_rx_data->ir_addr_cnt; i++) {
		sprintf(addr_name, "ir_power_key_code%d", i);
		if (of_property_read_u32(np, (const char *)&addr_name,
					&ir_rx_data->ir_powerkey[i])) {
			break;
		}
	}
} else if (ir_rx_data->ir_protocols ==  RC5) {
	for (i = 0; i < MAX_ADDR_NUM; i++) {
		sprintf(addr_name, "rc5_ir_addr_code%d", i);
		if (of_property_read_u32(np, (const char *)&addr_name,
					&ir_rx_data->ir_addr[i])) {
			break;
		}
	}

	ir_rx_data->ir_addr_cnt = i;
	for (i = 0; i < ir_rx_data->ir_addr_cnt; i++) {
		sprintf(addr_name, "rc5_ir_power_key_code%d", i);
		if (of_property_read_u32(np, (const char *)&addr_name,
					&ir_rx_data->ir_powerkey[i])) {
			break;
		}
	}
}
#endif

#ifdef CONFIG_SUNXI_REGULATOR_DT
	pr_debug("%s: cir try dt way to get regulator\n", __func__);
	snprintf(ir_supply, sizeof(ir_supply), "ir%d", pdev->id);
	ir_rx_data->suply = regulator_get(dev, ir_supply);
	if (IS_ERR(ir_rx_data->suply)) {
		pr_err("%s: cir get supply err\n", __func__);
		ir_rx_data->suply = NULL;
	}
#else
	if (of_property_read_u32(np, "supply_vol", &ir_rx_data->suply_vol))
		pr_debug("%s: get cir supply_vol failed", __func__);

	if (of_property_read_string(np, "supply", &name)) {
		pr_debug("%s: cir have no power supply\n", __func__);
		ir_rx_data->suply = NULL;
	} else {
		ir_rx_data->suply = regulator_get(NULL, name);
		if (IS_ERR(ir_rx_data->suply)) {
			pr_err("%s: cir get supply err\n", __func__);
			ir_rx_data->suply = NULL;
		}
	}
#endif

#ifdef CONFIG_ANDROID
//#define IR_TX_DMA_CR		(0x34)
//#define IR_TX_DMA_CR		(0x34)
	if (sunxi_get_ir_protocol())
		pr_err("%s: get_ir_protocol failed.\n", __func__);
#endif

	ir_rx_data->wakeup = of_property_read_bool(np, "wakeup-source");
	if (ir_rx_data->wakeup)
		device_init_wakeup(&pdev->dev, ir_rx_data->wakeup);

	return 0;

out_dispose_mapping:
	irq_dispose_mapping(ir_rx_data->irq_num);
out_iounmap2:
	iounmap(vaddr);
out_iounmap1:
	iounmap(ir_rx_data->reg_base);

	return ret;
}

static int sunxi_ir_recv_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct rc_dev *sunxi_rcdev;
	int ret;
	static char const ir_rx_dev_name[] = "s_cir_rx";

	pr_debug("sunxi-ir probe start !\n");
	ir_rx_data = devm_kzalloc(dev, sizeof(*ir_rx_data), GFP_KERNEL);
	if (IS_ERR_OR_NULL(ir_rx_data)) {
		pr_err("ir_rx_data: not enough memory for ir data\n");
		return -ENOMEM;
	}

	if (pdev->dev.of_node) {
		pdev->id = of_alias_get_id(pdev->dev.of_node, "ir");
		if (pdev->id < 0) {
			pr_err("sunxi ir failed to get alias id\n");
			return -EINVAL;
		}

		/* initialize hardware resource */
		ret = sunxi_ir_startup(pdev, ir_rx_data);
		if (ret) {
			pr_err("initialize hardware resource failed!\n");
			goto err_startup;
		}
	} else {
		pr_err("sunxi ir device tree err!\n");
		ret = -EBUSY;
		goto err_startup;
	}
	pr_debug("sunxi-ir initialize hardware res success!\n");

	sunxi_rcdev = devm_rc_allocate_device(dev, RC_DRIVER_IR_RAW);
	if (!sunxi_rcdev) {
		pr_err("rc dev allocate fail !\n");
		ret = -ENOMEM;
		goto err_startup;
	}
	pr_debug("sunxi-ir allocate rc device success!\n");

	/* initialize sunxi_rcdev */
	sunxi_rcdev->priv = ir_rx_data;
	sunxi_rcdev->device_name = SUNXI_IR_DEVICE_NAME;
	sunxi_rcdev->input_phys = SUNXI_IR_DEVICE_NAME "/input0";
	sunxi_rcdev->input_id.bustype = BUS_HOST;
	sunxi_rcdev->input_id.vendor = 0x0001;
	sunxi_rcdev->input_id.product = 0x0001;
	sunxi_rcdev->input_id.version = 0x0100;
	//sunxi_rcdev->input_dev->dev.init_name = &ir_rx_dev_name[0];

	sunxi_rcdev->dev.parent = &pdev->dev;
	sunxi_rcdev->driver_type = RC_DRIVER_IR_RAW;
	sunxi_rcdev->driver_name = SUNXI_IR_DRIVER_NAME;

	if (ir_rx_data->ir_protocols == NEC)
		sunxi_rcdev->allowed_protocols = (u64)RC_PROTO_BIT_NEC;
	if (ir_rx_data->ir_protocols == RC5)
		sunxi_rcdev->allowed_protocols = (u64)RC_PROTO_BIT_RC5;
	if (ir_rx_data->ir_protocols == RC5ANDNEC)
		sunxi_rcdev->allowed_protocols = (u64)RC_PROTO_BIT_RC5 |
			(u64)RC_PROTO_BIT_NEC;
	sunxi_rcdev->enabled_protocols = sunxi_rcdev->allowed_protocols;

	sunxi_rcdev->map_name = RC_MAP_SUNXI;
#ifdef CONFIG_SUNXI_KEYMAPPING_SUPPORT
	init_sunxi_ir_map_ext(ir_rx_data->ir_addr, ir_rx_data->ir_addr_cnt);
#else
	init_sunxi_ir_map();
#endif

	ret = devm_rc_register_device(dev, sunxi_rcdev);
	if (ret) {
		dev_err(&pdev->dev, "failed to register rc device\n");
		goto err_register_rc_device;
	}
	pr_debug("sunxi-ir register rc device success!\n");

	ir_rx_data->rcdev = sunxi_rcdev;
	ir_rx_data->pdev = pdev;

	if (ir_rx_data->suply) {
		ret = regulator_set_voltage(ir_rx_data->suply, ir_rx_data->suply_vol,
				ir_rx_data->suply_vol);
		if (ret)
			pr_err("ir rx set regulator voltage failed!\n");

		ret = regulator_enable(ir_rx_data->suply);
		if (ret)
			pr_err("ir rx regulator enable failed!\n");
	}

	ret = ir_setup(ir_rx_data);
	if (ret) {
		pr_err("%s: ir_setup failed.\n", __func__);
		ret = -EIO;
		goto err_register_rc_device;
	}
	pr_debug("sunxi-ir hardware setup success!\n");

	platform_set_drvdata(pdev, ir_rx_data);
	ret = devm_request_irq(dev, ir_rx_data->irq_num, sunxi_ir_recv_irq, 0,
				"RemoteIR_RX", ir_rx_data);
	if (ret) {
		pr_err("%s: request irq fail.\n", __func__);
		ret = -EBUSY;
		goto err_request_irq;
	}

	pr_debug("ir probe end!\n");

	return 0;

err_request_irq:
	ir_clk_uncfg(ir_rx_data);
	if (ir_rx_data->suply) {
		regulator_disable(ir_rx_data->suply);
		regulator_put(ir_rx_data->suply);
	}

err_register_rc_device:
	exit_sunxi_ir_map();

err_startup:
	return ret;
}

static int sunxi_ir_recv_remove(struct platform_device *pdev)
{
	struct sunxi_ir_rx_data *ir_rx_data = platform_get_drvdata(pdev);

#ifdef CONFIG_ANDROID
	 proc_remove(ir_protocol_dir);
#endif
	ir_clk_uncfg(ir_rx_data);

	if (ir_rx_data->suply) {
		regulator_disable(ir_rx_data->suply);
		regulator_put(ir_rx_data->suply);
	}

	exit_sunxi_ir_map();

	return 0;
}

#ifdef CONFIG_PM

static int ir_resume_setup(struct sunxi_ir_rx_data *ir_data)
{
	int ret;

	if (IS_ERR_OR_NULL(ir_data->pctrl)) {
		pr_err("ir pin config error\n");
		return -1;
	}

	ir_select_gpio_state(ir_data->pctrl, PINCTRL_STATE_DEFAULT);

	ret = ir_clk_cfg(ir_rx_data);
	if (ret) {
		pr_err("ir rx clk configure failed!\n");
		return ret;
	}

	ir_reg_cfg(ir_data->reg_base);
	return 0;
}

static int sunxi_ir_recv_suspend(struct device *dev)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct sunxi_ir_rx_data *ir_rx_data = platform_get_drvdata(pdev);
	int ret = 0;

	pr_debug("enter: sunxi_ir_rx_suspend.\n");

	if (device_may_wakeup(dev)) {
		if (ir_rx_data->wakeup)
			enable_irq_wake(ir_rx_data->irq_num);
		dev_dbg(dev, "enter: sunxi_ir_rx_suspend enable irq wakeup.\n");
	} else {
		disable_irq_nosync(ir_rx_data->irq_num);
		ir_clk_uncfg(ir_rx_data);

		ret = ir_select_gpio_state(ir_rx_data->pctrl, PINCTRL_STATE_SLEEP);
		if (ret) {
			pr_err("%s: ir rx suspend failed\n", __func__);
			return ret;
		}
	}

	return 0;
}

static int sunxi_ir_recv_resume(struct device *dev)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct sunxi_ir_rx_data *ir_rx_data = platform_get_drvdata(pdev);
	int ret = 0;
	u32 rtc_val, scancode;
	u8 not_address, command, not_command;

	rtc_val = readl(vaddr);
	writel(0, vaddr);
	pr_debug("enter: sunxi_ir_rx_resume.\n");

	if (device_may_wakeup(dev)) {
		if (ir_rx_data->wakeup)
			disable_irq_wake(ir_rx_data->irq_num);

		if (rtc_val) {
			not_address = (u8)((rtc_val >> 16) & 0xff);
			command	    = (u8)((rtc_val >>  8) & 0xff);
			not_command = (u8)((rtc_val >>  0) & 0xff);
			scancode = not_command << 8 | command << 16 | not_address;
			rc_keydown(ir_rx_data->rcdev, RC_PROTO_NEC, scancode, 0);
			rc_keyup(ir_rx_data->rcdev);
		}

		dev_dbg(dev, "enter: sunxi_ir_rx_suspend disable irq wakeup.\n");
	} else {
		enable_irq(ir_rx_data->irq_num);

		ret = ir_resume_setup(ir_rx_data);
		if (ret) {
			pr_err("%s: ir rx resume failed\n", __func__);
			return ret;
		}
	}

	return 0;
}

static const struct dev_pm_ops sunxi_ir_recv_pm_ops = {
	.suspend        = sunxi_ir_recv_suspend,
	.resume         = sunxi_ir_recv_resume,
};
#endif

static struct platform_driver sunxi_ir_recv_driver = {
	.probe  = sunxi_ir_recv_probe,
	.remove = sunxi_ir_recv_remove,
	.driver = {
		.name   = SUNXI_IR_DRIVER_NAME,
		.owner  = THIS_MODULE,
		.of_match_table = of_match_ptr(sunxi_ir_recv_of_match),
#ifdef CONFIG_PM
		.pm	= &sunxi_ir_recv_pm_ops,
#endif
	},
};
module_platform_driver(sunxi_ir_recv_driver);
MODULE_DESCRIPTION("SUNXI IR Receiver driver");
MODULE_AUTHOR("QIn");
MODULE_LICENSE("GPL v2");
