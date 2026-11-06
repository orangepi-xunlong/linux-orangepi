// SPDX-License-Identifier: GPL-2.0-or-later

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/platform_device.h>
#include <linux/io.h>
#include <linux/of.h>
#include <linux/of_platform.h>
#include <linux/delay.h>
#include <linux/clk.h>
#include <linux/clk-provider.h>
#include "cv1835_i2s_subsys.h"

struct cvi_i2s_subsys_dev *dev;
void __iomem *master_reg;
u32 current_freq;
void __iomem *subsys_reg;

typedef struct _audio_power_ctrl_ {
    	struct clk *aud_clk[6];
	struct clk *aud_src;
	struct clk *aud_dw;
} aud_power_ctrl;

static const char *const aud_clk_name[8] = {
        "clk_sdma_aud0", "clk_sdma_aud1", "clk_sdma_aud2", "clk_sdma_aud3",
        "clk_sdma_aud4", "clk_sdma_aud5", "clk_audsrc", "clk_aud_dw"
};

static aud_power_ctrl aud_pwm_ctrl = {0};

int aud_register_clk(void)
{
	int ret;
	int i;

	for (i = 0; i < 6; i++) {
		aud_pwm_ctrl.aud_clk[i] = devm_clk_get(dev->dev, aud_clk_name[i]);
		if (IS_ERR(aud_pwm_ctrl.aud_clk[i])) {
			ret = PTR_ERR(aud_pwm_ctrl.aud_clk[i]);
			dev_err(dev->dev, "failed to retrieve aud %s clock",  aud_clk_name[i]);
			return ret;
		}
	}

	aud_pwm_ctrl.aud_src = devm_clk_get(dev->dev, aud_clk_name[6]);
	if (IS_ERR(aud_pwm_ctrl.aud_src)) {
		ret = PTR_ERR( aud_pwm_ctrl.aud_src);
		dev_err(dev->dev, "failed to retrieve aud %s clock",  aud_clk_name[6]);
		return ret;
	}

	aud_pwm_ctrl.aud_dw = devm_clk_get(dev->dev, aud_clk_name[7]);
	if (IS_ERR(aud_pwm_ctrl.aud_dw)) {
		ret = PTR_ERR( aud_pwm_ctrl.aud_dw);
		dev_err(dev->dev, "failed to retrieve aud %s clock",  aud_clk_name[7]);
		return ret;
	}

	return 0;
}

u32 i2s_subsys_query_master(void)
{
	return dev->master_id;
}

bool i2s_master_is_on(void)
{
	return readl(master_reg + I2S_ENABLE_REG);
}

void i2s_set_master_clk(u32 clk_ctrl1)
{
	if (i2s_master_is_on()) {
		if (clk_ctrl1 != readl(master_reg + I2S_CLK_CTRL1_REG))
			dev_err(dev->dev, "Master I2S is already enabled, cannot set with different bclk\n");
		return;
	}

	writel(clk_ctrl1, master_reg + I2S_CLK_CTRL1_REG);
}

void i2s_set_master_frame_setting(u32 frame_format)
{
	if (i2s_master_is_on()) {
		if (frame_format != readl(master_reg + I2S_FRAME_SETTING_REG))
			dev_err(dev->dev, "Master I2S is already enabled, cannot set with different format\n");
		return;
	}

	writel(frame_format, master_reg + I2S_FRAME_SETTING_REG);
}

void i2s_master_clk_switch_on(bool on)
{
	u32 clk_ctrl0 = readl(master_reg + I2S_CLK_CTRL0_REG);

	if (i2s_master_is_on())
		return;

	if (on) {
		writel(clk_ctrl0 | 0x140, master_reg + I2S_CLK_CTRL0_REG);
		writel(0x1, master_reg + I2S_LCRK_MASTER_REG);
	} else {
		writel(0x0, master_reg + I2S_LCRK_MASTER_REG);
		writel(clk_ctrl0 & 0x0bf, master_reg + I2S_CLK_CTRL0_REG);
	}

}

void cv1835_set_mclk(u32 freq)
{
	struct clk *clk_a0pll;
	struct clk *clk_sdma_aud0;
	struct clk *clk_sdma_aud1;
	struct clk *clk_sdma_aud2;
	struct clk *clk_sdma_aud3;
	struct clk *clk_sdma_aud4;
	struct clk *clk_sdma_aud5;
#ifdef CONFIG_ARCH_CV183X_ASIC
	void __iomem *gp_reg3 = ioremap(0x0300008c, 4);
	u32 chip_id = readl(gp_reg3);
#endif

	if (current_freq != freq)
		current_freq = freq;
	else
		return;

	clk_a0pll = devm_clk_get(dev->dev, "clk_a0pll");
	if (IS_ERR(clk_a0pll)) {
		dev_err(dev->dev, "Get clk_a0pll failed\n");
		return;
	}

	clk_sdma_aud0 = devm_clk_get(dev->dev, "clk_sdma_aud0");
	if (IS_ERR(clk_sdma_aud0)) {
		dev_err(dev->dev, "Get clk_sdma_aud0 failed\n");
		return;
	}

	clk_sdma_aud1 = devm_clk_get(dev->dev, "clk_sdma_aud1");
	if (IS_ERR(clk_sdma_aud1)) {
		dev_err(dev->dev, "Get clk_sdma_aud1 failed\n");
		return;
	}

	clk_sdma_aud2 = devm_clk_get(dev->dev, "clk_sdma_aud2");
	if (IS_ERR(clk_sdma_aud2)) {
		dev_err(dev->dev, "Get clk_sdma_aud2 failed\n");
		return;
	}

	clk_sdma_aud3 = devm_clk_get(dev->dev, "clk_sdma_aud3");
	if (IS_ERR(clk_sdma_aud3)) {
		dev_err(dev->dev, "Get clk_sdma_aud3 failed\n");
		return;
	}

	clk_sdma_aud4 = devm_clk_get(dev->dev, "clk_sdma_aud4");
	if (IS_ERR(clk_sdma_aud4)) {
		dev_err(dev->dev, "Get clk_sdma_aud2 failed\n");
		return;
	}

	clk_sdma_aud5 = devm_clk_get(dev->dev, "clk_sdma_aud5");
	if (IS_ERR(clk_sdma_aud5)) {
		dev_err(dev->dev, "Get clk_sdma_aud3 failed\n");
		return;
	}

	switch (freq) {
	case CVI_16384_MHZ:
#ifdef CONFIG_ARCH_CV183X_ASIC
		if (chip_id != 0x1838) {
			dev_info(dev->dev, "Set clk_a0pll to 406425600\n");
			clk_set_rate(clk_a0pll, 406425600);
		}
#endif
		dev_info(dev->dev, "Set clk_sdma_aud0~5 to 16384000\n");
		clk_set_rate(clk_sdma_aud0, 16384000);
		clk_set_rate(clk_sdma_aud1, 16384000);
		clk_set_rate(clk_sdma_aud2, 16384000);
		clk_set_rate(clk_sdma_aud3, 16384000);
		clk_set_rate(clk_sdma_aud4, 16384000);
		clk_set_rate(clk_sdma_aud5, 16384000);
		break;
	case CVI_22579_MHZ:
#ifdef CONFIG_ARCH_CV183X_ASIC
		if (chip_id != 0x1838) {
			dev_info(dev->dev, "Set clk_a0pll to 406425600\n");
			clk_set_rate(clk_a0pll, 406425600);
		}
#endif
		dev_info(dev->dev, "Set clk_sdma_aud0~5 to 22579200\n");
		clk_set_rate(clk_sdma_aud0, 22579200);
		clk_set_rate(clk_sdma_aud1, 22579200);
		clk_set_rate(clk_sdma_aud2, 22579200);
		clk_set_rate(clk_sdma_aud3, 22579200);
		clk_set_rate(clk_sdma_aud4, 22579200);
		clk_set_rate(clk_sdma_aud5, 22579200);
		break;
	case CVI_24576_MHZ:
#ifdef CONFIG_ARCH_CV183X_ASIC
		if (chip_id != 0x1838) {
			dev_info(dev->dev, "Set clk_a0pll to 417792000\n");
			clk_set_rate(clk_a0pll, 417792000);
		}
#endif
		dev_info(dev->dev, "Set clk_sdma_aud0~5 to 24576000\n");
		clk_set_rate(clk_sdma_aud0, 24576000);
		clk_set_rate(clk_sdma_aud1, 24576000);
		clk_set_rate(clk_sdma_aud2, 24576000);
		clk_set_rate(clk_sdma_aud3, 24576000);
		clk_set_rate(clk_sdma_aud4, 24576000);
		clk_set_rate(clk_sdma_aud5, 24576000);
		break;
	default:
		dev_info(dev->dev, "Unrecognised freq\n");
		break;
	}

#ifdef CONFIG_ARCH_CV183X_ASIC
	iounmap(gp_reg3);
#endif
}

void dwi2s_set_mclk(u32 dwi2s_mode, u32 slave_source, u32 ctl0, u32 ctl1)
{
	if (subsys_reg) {
		writel(dwi2s_mode, subsys_reg + DW_I2S_MODE_REG);
		writel(slave_source, subsys_reg + DW_I2S_SLAVEMODE_SOURCE);
		writel(ctl0, subsys_reg + DW_I2S_CLK_CTRL0);
		writel(ctl1, subsys_reg + DW_I2S_CLK_CTRL1);
	}
}

void dwi2s_get_subsys(void)
{
	if (subsys_reg) {
		pr_info("[%s][subsys:%p]dwi2s_mode:0x%x, dwi2s_slacemode_source:0x%x, clk_ctrl0:0x%x, clk_ctrl1:%x\n",
			__func__, subsys_reg, readl(subsys_reg + DW_I2S_MODE_REG),
			readl(subsys_reg + DW_I2S_SLAVEMODE_SOURCE),
			readl(subsys_reg + DW_I2S_CLK_CTRL0), readl(subsys_reg + DW_I2S_CLK_CTRL1));
	}
}

void aud_clk_enable(void){
	int i;

	for(i = 0; i < 6; i++){
		if (!__clk_is_enabled(aud_pwm_ctrl.aud_clk[i]))
        		clk_prepare_enable(aud_pwm_ctrl.aud_clk[i]);
	}

	if (!__clk_is_enabled(aud_pwm_ctrl.aud_src))
        		clk_prepare_enable(aud_pwm_ctrl.aud_src);

	if (!__clk_is_enabled(aud_pwm_ctrl.aud_dw))
        		clk_prepare_enable(aud_pwm_ctrl.aud_dw);
}

void aud_clk_disable(void){
	int i;

	for(i = 0; i < 6; i++){
		if (__clk_is_enabled(aud_pwm_ctrl.aud_clk[i]))
        		clk_disable_unprepare(aud_pwm_ctrl.aud_clk[i]);
	}

	if (__clk_is_enabled(aud_pwm_ctrl.aud_src))
        		clk_disable_unprepare(aud_pwm_ctrl.aud_src);

	if (__clk_is_enabled(aud_pwm_ctrl.aud_dw))
        		clk_disable_unprepare(aud_pwm_ctrl.aud_dw);
}

static int i2s_subsys_probe(struct platform_device *pdev)
{
	struct resource *res;
	struct clk *i2sclk;
	const char *clk_id;
	u32 audio_clk;

	dev = devm_kzalloc(&pdev->dev, sizeof(*dev), GFP_KERNEL);
	if (!dev)
		return -ENOMEM;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	dev->subsys_base = devm_ioremap_resource(&pdev->dev, res);
	dev_dbg(&pdev->dev, "I2S get i2s_subsys_base=0x%p\n", dev->subsys_base);
	if (IS_ERR(dev->subsys_base))
		return PTR_ERR(dev->subsys_base);
	dev->dev = &pdev->dev;

#if defined(CONFIG_SND_SOC_CV1835_CONCURRENT_I2S)
	device_property_read_u32(&pdev->dev, "master_base", &dev->master_base);
	dev_dbg(dev->dev, "master base is 0x%x\n", dev->master_base);

	switch (dev->master_base) {
	case 0x29120000:
		dev->master_id = 1;
		break;
	case 0x29130000:
		dev->master_id = 2;
		break;
	default:
		dev_err(dev->dev, "master base is not correct!! plz set I2S1 or I2S2\n");
	}
#endif

	subsys_reg = dev->subsys_base;
	master_reg = ioremap(dev->master_base, 0x100);
	if (!master_reg)
		dev_err(dev->dev, "%s FAILED!!\n", __func__);

	clk_id = "i2sclk";
	i2sclk = devm_clk_get(&pdev->dev, clk_id);
	if (IS_ERR(i2sclk))
		return PTR_ERR(i2sclk);

	audio_clk = clk_get_rate(i2sclk);
	pr_info("get audio clk=%d\n", audio_clk);

	if (aud_register_clk()) {
            dev_err(dev->dev, "aud clock init failed\n");
        }
	cv1835_set_mclk(audio_clk);
	aud_clk_enable();
	aud_clk_disable();

#if defined(CONFIG_ARCH_CV186X)
#if defined(CONFIG_SND_SOC_CV1835PDM)
	writel(0x543260, dev->subsys_base + SCLK_IN_SEL);
	writel(0x543260, dev->subsys_base + FS_IN_SEL);
	writel(0x543260, dev->subsys_base + SDI_IN_SEL);
	writel(0x543210, dev->subsys_base + SDO_OUT_SEL);
	writel(0x0000, dev->subsys_base + MULTI_SYNC);
	writel(0x0202, dev->subsys_base + BCLK_OEN_SEL);//pdm_to_i2s1
	writel(0x0001, dev->subsys_base + AUDIO_PDM_CTRL);//select i2s1 io
#else
	writel(0x543210, dev->subsys_base + SCLK_IN_SEL);
	writel(0x543210, dev->subsys_base + FS_IN_SEL);
	writel(0x543210, dev->subsys_base + SDI_IN_SEL);
	writel(0x543210, dev->subsys_base + SDO_OUT_SEL);
	writel(0x0000, dev->subsys_base + MULTI_SYNC);
	writel(0x0000, dev->subsys_base + BCLK_OEN_SEL);

	//master
	writel(DWI2S_MODE_REG_VAL, dev->subsys_base + DW_I2S_MODE_REG);
	//need mclk 0x1c0
	writel(DWI2S_CLK_CTRL0_VAL, dev->subsys_base + DW_I2S_CLK_CTRL0);
#endif

#else

#if defined(CONFIG_SND_SOC_CV1835_CONCURRENT_I2S)
	writel(0x7114, dev->subsys_base + SCLK_IN_SEL);
	writel(0x7114, dev->subsys_base + FS_IN_SEL);
	writel(0x7554, dev->subsys_base + SDI_IN_SEL);
	writel(0x7664, dev->subsys_base + SDO_OUT_SEL);
	writel(0x0000, dev->subsys_base + MULTI_SYNC);
	writel(0x0000, dev->subsys_base + BCLK_OEN_SEL);

#elif defined(CONFIG_SND_SOC_CV1835PDM)
	writel(0x7614, dev->subsys_base + SCLK_IN_SEL);
	writel(0x7214, dev->subsys_base + FS_IN_SEL);
	writel(0x7614, dev->subsys_base + SDI_IN_SEL);
	writel(0x7604, dev->subsys_base + SDO_OUT_SEL);
	writel(0x0000, dev->subsys_base + MULTI_SYNC);
#if defined(CONFIG_ARCH_CV182X)
	writel(0x0404, dev->subsys_base + BCLK_OEN_SEL);
#else
	writel(0x0202, dev->subsys_base + BCLK_OEN_SEL);
#endif
	writel(0x0002, dev->subsys_base + AUDIO_PDM_CTRL);

#else
	/* normal operation, use I2S1 as TX and RX */
	writel(0x7654, dev->subsys_base + SCLK_IN_SEL);
	writel(0x7654, dev->subsys_base + FS_IN_SEL);
	writel(0x7654, dev->subsys_base + SDI_IN_SEL);
	writel(0x7654, dev->subsys_base + SDO_OUT_SEL);
	writel(0x0000, dev->subsys_base + MULTI_SYNC);
	writel(0x0000, dev->subsys_base + BCLK_OEN_SEL);
#endif//CONFIG_SND_SOC_CV1835_CONCURRENT_I2S
#endif//CONFIG_ARCH_CV186X

	return 0;
}

#define CV182X_DAC_RESET	0xF7FFFFFF
#define CV182X_ADC_RESET	0xDFFFFFFF
#define CV182XA_DAC_RESET	0xF7FFFFFF
#define CV182XA_ADC_RESET	0xDFFFFFFF

#define CV186X_DAC0_RESET	0xFFFBFFFF
#define CV186X_DAC1_RESET	0xFFDFFFFF
#define CV186X_ADC0_RESET	0xFFEFFFFF
#define CV186X_ADC1_RESET	0xFF7FFFFF

/* while cv182x codecs transfer CIC between 64 and 128, need to reset codec first */
void cv182x_reset_dac(void)
{
#ifndef CONFIG_ARCH_CV186X
	void __iomem *reset_reg = ioremap(0x03003008, 4);

	writel((readl(reset_reg) & CV182X_DAC_RESET), reset_reg);
	writel((readl(reset_reg) | ~CV182X_DAC_RESET), reset_reg);
	iounmap(reset_reg);
#else
//add a2 dac
#endif
}

void cv182x_reset_adc(void)
{
#ifndef CONFIG_ARCH_CV186X
	void __iomem *reset_reg = ioremap(0x03003008, 4);

	writel((readl(reset_reg) & CV182X_ADC_RESET), reset_reg);
	writel((readl(reset_reg) | ~CV182X_ADC_RESET), reset_reg);
	iounmap(reset_reg);
#else
//add a2 adc
#endif
}

void cv182xa_reset_dac(void)
{
#ifndef CONFIG_ARCH_CV186X
	void __iomem *reset_reg = ioremap(0x03003008, 4);

	writel((readl(reset_reg) & CV182XA_DAC_RESET), reset_reg);
	writel((readl(reset_reg) | ~CV182XA_DAC_RESET), reset_reg);
	iounmap(reset_reg);
#else

	void __iomem *reset_reg = ioremap(0x28103004, 4);

	writel((readl(reset_reg) & CV186X_DAC0_RESET), reset_reg);
	writel((readl(reset_reg) | ~CV186X_DAC0_RESET), reset_reg);

	writel((readl(reset_reg) & CV186X_DAC1_RESET), reset_reg);
	writel((readl(reset_reg) | ~CV186X_DAC1_RESET), reset_reg);

	iounmap(reset_reg);

#endif
}

void cv182xa_reset_adc(void)
{
#ifndef CONFIG_ARCH_CV186X
	void __iomem *reset_reg = ioremap(0x03003008, 4);

	writel((readl(reset_reg) & CV182XA_ADC_RESET), reset_reg);
	writel((readl(reset_reg) | ~CV182XA_ADC_RESET), reset_reg);
	iounmap(reset_reg);

#else
	void __iomem *reset_reg = ioremap(0x28103004, 4);

	writel((readl(reset_reg) & CV186X_ADC0_RESET), reset_reg);
	writel((readl(reset_reg) | ~CV186X_ADC0_RESET), reset_reg);

	writel((readl(reset_reg) & CV186X_ADC1_RESET), reset_reg);
	writel((readl(reset_reg) | ~CV186X_ADC1_RESET), reset_reg);
	iounmap(reset_reg);
#endif
}

static const struct of_device_id i2s_subsys_id_match[] = {
	{
		.compatible = "cvitek,i2s_tdm_subsys",
	},
	{},
};

#ifdef CONFIG_PM_SLEEP
static int i2s_subsys_suspend_late(struct device *t_dev)
{
	if (!dev->reg_ctx) {
		dev->reg_ctx = devm_kzalloc(dev->dev, sizeof(struct subsys_reg_context), GFP_KERNEL);
		if (!dev->reg_ctx)
			return -ENOMEM;
	}

	dev->reg_ctx->sclk_in_sel = readl(dev->subsys_base + SCLK_IN_SEL);
	dev->reg_ctx->fs_in_sel = readl(dev->subsys_base + FS_IN_SEL);
	dev->reg_ctx->sdi_in_sel = readl(dev->subsys_base + SDI_IN_SEL);
	dev->reg_ctx->sdo_out_sel = readl(dev->subsys_base + SDO_OUT_SEL);
	dev->reg_ctx->multi_sync = readl(dev->subsys_base + MULTI_SYNC);
	dev->reg_ctx->bclk_oen_sel = readl(dev->subsys_base + BCLK_OEN_SEL);
	dev->reg_ctx->pdm_ctrl = readl(dev->subsys_base + AUDIO_PDM_CTRL);
	return 0;
}

static int i2s_subsys_resume_early(struct device *t_dev)
{
	writel(dev->reg_ctx->sclk_in_sel, dev->subsys_base + SCLK_IN_SEL);
	writel(dev->reg_ctx->fs_in_sel, dev->subsys_base + FS_IN_SEL);
	writel(dev->reg_ctx->sdi_in_sel, dev->subsys_base + SDI_IN_SEL);
	writel(dev->reg_ctx->sdo_out_sel, dev->subsys_base + SDO_OUT_SEL);
	writel(dev->reg_ctx->multi_sync, dev->subsys_base + MULTI_SYNC);
	writel(dev->reg_ctx->bclk_oen_sel, dev->subsys_base + BCLK_OEN_SEL);
	writel(dev->reg_ctx->pdm_ctrl, dev->subsys_base + AUDIO_PDM_CTRL);

	return 0;
}
#else
#define i2s_subsys_suspend_late	NULL
#define i2s_subsys_resume_early	NULL
#endif /* CONFIG_PM_SLEEP */

static const struct dev_pm_ops i2s_subsys_pm_ops = {
	SET_LATE_SYSTEM_SLEEP_PM_OPS(i2s_subsys_suspend_late, i2s_subsys_resume_early)
};

static struct platform_driver i2s_subsys_driver = {
	.driver = {
		.name = "cvitek-i2s-subsys",
		.owner = THIS_MODULE,
		.pm = &i2s_subsys_pm_ops,
		.of_match_table = of_match_ptr(i2s_subsys_id_match),
	},
	.probe = i2s_subsys_probe,
};

static int __init i2s_subsys_init(void)
{
	return platform_driver_register(&i2s_subsys_driver);
}

arch_initcall(i2s_subsys_init);
