// SPDX-License-Identifier: GPL-2.0
/*
 * V4L2 Capture CSI Subdev for Cix sky SOC
 *
 * Copyright 2024 Cix Technology Group Co., Ltd.
 *
 */
#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/errno.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/irq.h>
#include <linux/kernel.h>
#include <linux/memory.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_graph.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/spinlock.h>
#include <linux/pm_domain.h>
#include <linux/iopoll.h>
#include <linux/phy/phy.h>
#include <linux/phy/phy-mipi-dphy.h>
#include "cdns-dphy-rx.h"
#include "csi_common.h"

#define CIX_MIPI_DPHY_HW_DRIVER_NAME "cix-mipi-dphy-hw"
#define HIGH_SPEED_2500M	(2500000000)

static int mipi_dphy_hw_dev_resume(struct dphy_rx *dphy);
static int mipi_dphy_hw_dev_suspend(struct dphy_rx *dphy);

struct phy_config {
	unsigned int offset;
	unsigned int value;
};

struct phy_config dphy_config_2500M[] = {
	{0x020,0x0415},
	{0x208,0xF600},
	{0x308,0xF600},
	{0x708,0xF600},
	{0x808,0xF600},
	{0xb00,0x02d6},
	{0xb0c,0x02aa},
};

static inline u32 mipi_dphy_read(struct cdns_dphy_rx *mipi_dphy, u32 reg)
{
	rmb();
	return readl(mipi_dphy->regs + reg);
}

static inline void mipi_dphy_write(struct cdns_dphy_rx *mipi_dphy, u32 reg, u32 val)
{
	wmb();
	writel(val, mipi_dphy->regs + reg);
}

/* Order of bands is important since the index is the band number. */
static const struct cdns_dphy_rx_band bands[] = {
	{ 80, 100 },
	{ 100, 120 },
	{ 120, 160 },
	{ 160, 200 },
	{ 200, 240 },
	{ 240, 280 },
	{ 280, 320 },
	{ 320, 360 },
	{ 360, 400 },
	{ 400, 480 },
	{ 480, 560 },
	{ 560, 640 },
	{ 640, 720 },
	{ 720, 800 },
	{ 800, 880 },
	{ 880, 1040 },
	{ 1040, 1200 },
	{ 1200, 1350 },
	{ 1350, 1500 },
	{ 1500, 1750 },
	{ 1750, 2000 },
	{ 2000, 2250 },
	{ 2250, 2500 }
};

static struct dph_psm_dynamic_param psm_dynamic_param[] = {
	{ 0, 0, 0, 0x7 },
	{ 0, 0, 0, 0x8 },
	{ 0, 0, 0, 0x9 },
	{ 0, 0, 0, 0xB },
	{ 0, 0, 0, 0xE },
	{ 0, 0, 0, 0x10 },
	{ 0, 0, 0, 0x12 },
	{ 0, 0, 0, 0x14 },
	{ 0, 0, 0, 0x16 },
	{ 0, 0, 0, 0x19 },
	{ 0, 0, 0, 0x1D },
	{ 0, 0, 0, 0x21 },
	{ 0, 0, 0, 0x25 },
	{ 0, 0, 0, 0x29 },
	{ 0, 0, 0, 0x2d },
	{ 0, 0, 0, 0x33 },
	{ 0, 0, 0, 0x3C },
	{ 0, 0, 0, 0x44 },
	{ 0, 0, 0, 0x4B },
	{ 0, 0, 1, 0x54 },
	{ 0, 0, 1, 0x61 },
	{ 0, 0, 1, 0x6E },
	{ 0, 0, 1, 0x7B },
};

static int cdns_dphy_rx_get_band_ctrl(unsigned long hs_clk_rate)
{
	unsigned int rate, i;

	rate = hs_clk_rate / 1000000UL;
	/* Since CSI-2 clock is DDR, the bit rate is twice the clock rate. */
	rate *= 2;

	if (rate < bands[0].min_rate)
		return -EOPNOTSUPP;

	for (i = 0; i < ARRAY_SIZE(bands); i++)
		if (rate < bands[i].max_rate)
			return i;

	return -EOPNOTSUPP;
}

static int mipi_dphy_config(struct dphy_rx *dphy, u32 data_rate)
{
	struct cdns_dphy_rx *hw = dphy->dphy_hw;
	struct device *dev = dphy->dev;
	int i, band_id = 0;
	u8 num_lanes = 0;
	u32 PhyVal = 0;
	int loop;
	int ret;

	if(data_rate < HIGH_SPEED_2500M	) {

		switch (hw->num_lanes)
		{
			case MIPI_4LANES_EN:
				num_lanes = 4;
				break;
			case MIPI_2LANES_EN:
				num_lanes = 2;
				break;
			case MIPI_1LANE_EN:
			default:
				num_lanes = 1;
				break;
		}

		dev_info(dev, "data_rate = 0x%x, num_lanes = %d\n", data_rate, num_lanes);
		/*step 1 psm_clock_freq & ipconfig_cmn config csi RCSU*/

		band_id = cdns_dphy_rx_get_band_ctrl(data_rate);
		if (band_id < 0) {
			dev_info(dev, "unsupport data_rate 0x%x\n",data_rate);
			return band_id;
		}

		/* step 2 */
		PhyVal = mipi_dphy_read(hw, DPHY_CMN_DIG_TBIT2);
		PhyVal &= ~DPHY_CMN_RX_BANDGAP_TIMER_MASK;
		PhyVal |= DPHY_CMN_RX_BANDGAP_TIMER << 1;
		PhyVal |= (ENABLE << CMN_SSM_EN_OFFSET) | (ENABLE << CMN_RX_MODE_EN_OFFSET);

		mipi_dphy_write(hw, DPHY_CMN_DIG_TBIT2, PhyVal);

		/*step3 config DL0_LEFT_RX_DIG_TBIT0 refer mipi data rate*/
		for (int i = 0; i < DPHY_DATA_LANE_NUM_LEFT; i++) {
			/*left DL_RX_DIG_TBIT0*/
			PhyVal = mipi_dphy_read(hw, DL0_LEFT_RX_DIG_TBIT0  + i * 0x100);

			PhyVal &= ~TM_1P5TO2P5G_MODE_EN_MASK;
			PhyVal |= ((psm_dynamic_param[band_id].data_rate_select << TM_1P5TO2P5G_MODE_EN_OFFSET) & TM_1P5TO2P5G_MODE_EN_MASK);
			PhyVal &= ~TM_SETTLE_COUNT_MASK;
			PhyVal |= ((psm_dynamic_param[band_id].hs_settle_counter_value << TM_SETTLE_COUNT_OFFSET) & TM_SETTLE_COUNT_MASK);

			mipi_dphy_write(hw, DL0_LEFT_RX_DIG_TBIT0  + i * 0x100, PhyVal);
			/*RIGHT DL_RX_DIG_TBIT0*/
			PhyVal = mipi_dphy_read(hw, DL0_RIGHT_RX_DIG_TBIT0  + i * 0x100);

			PhyVal &= ~TM_1P5TO2P5G_MODE_EN_MASK;
			PhyVal |= ((psm_dynamic_param[band_id].data_rate_select << TM_1P5TO2P5G_MODE_EN_OFFSET) & TM_1P5TO2P5G_MODE_EN_MASK);
			PhyVal &= ~TM_SETTLE_COUNT_MASK;
			PhyVal |= ((psm_dynamic_param[band_id].hs_settle_counter_value << TM_SETTLE_COUNT_OFFSET) & TM_SETTLE_COUNT_MASK);

			mipi_dphy_write(hw, DL0_RIGHT_RX_DIG_TBIT0  + i * 0x100, PhyVal);
		}

		if (data_rate > 677000000) {
			/*step 4 clock lane config config*/
			for (int i = 0; i < 2; i++) {
				PhyVal = 0xFF;
				mipi_dphy_write(hw, CLK0_RX_ANA_TBIT0 + i * 0x500, PhyVal);

				PhyVal = mipi_dphy_read(hw, CLK0_RX_DIG_TBIT2  + i * 0x500);

				PhyVal &= ~RXDA_FREQ_BAND_STG2_MASK;
				PhyVal |= ((psm_dynamic_param[band_id].rx_stage2_cutoff_freq << RXDA_FREQ_BAND_STG2_OFFSET) & RXDA_FREQ_BAND_STG2_MASK);
				PhyVal &= ~RXDA_FREQ_BAND_STG3_MASK;
				PhyVal |= ((psm_dynamic_param[band_id].rx_stage3_cutoff_freq << RXDA_FREQ_BAND_STG3_OFFSET) & RXDA_FREQ_BAND_STG3_MASK);

				mipi_dphy_write(hw, CLK0_RX_DIG_TBIT2 + i * 0x500, PhyVal);
			}

			/*step 5 data lane config*/
			for (int i = 0; i < num_lanes; i++) {
				PhyVal = 0x34;
				mipi_dphy_write(hw, DL0_LEFT_RX_ANA_TBIT0 + i * 0x100, PhyVal);
				mipi_dphy_write(hw, DL0_RIGHT_RX_ANA_TBIT0 + i * 0x100, PhyVal);

				/*LEFT DL_RX_DIG_TBIT3*/
				PhyVal = mipi_dphy_read(hw, DL0_LEFT_RX_DIG_TBIT3  + i * 0x100);

				PhyVal &= ~TM_PREPAMP_CAL_INIT_WAIT_TIME_MASK;
				PhyVal |= ((0xFF << TM_PREPAMP_CAL_INIT_WAIT_TIME_OFFSET) & TM_PREPAMP_CAL_INIT_WAIT_TIME_MASK);
				mipi_dphy_write(hw, DL0_LEFT_RX_DIG_TBIT3  + i * 0x100, PhyVal);
				/*RIGHT DL_RX_DIG_TBIT3*/
				PhyVal = mipi_dphy_read(hw, DL0_RIGHT_RX_DIG_TBIT3  + i * 0x100);

				PhyVal &= ~TM_PREPAMP_CAL_INIT_WAIT_TIME_MASK;
				PhyVal |= ((0xFF << TM_PREPAMP_CAL_INIT_WAIT_TIME_OFFSET) & TM_PREPAMP_CAL_INIT_WAIT_TIME_MASK);

				mipi_dphy_write(hw, DL0_RIGHT_RX_DIG_TBIT3  + i * 0x100, PhyVal);
				/*LEFT DL_RX_DIG_TBIT5*/
				PhyVal = mipi_dphy_read(hw, DL0_LEFT_RX_DIG_TBIT5  + i * 0x100);

				PhyVal &= ~TM_DCC_COMP_CAL_ITER_WAIT_TIME_MASK;
				PhyVal |= ((0xFF << TM_DCC_COMP_CAL_ITER_WAIT_TIME_OFFSET) & TM_DCC_COMP_CAL_ITER_WAIT_TIME_MASK);

				mipi_dphy_write(hw, DL0_LEFT_RX_DIG_TBIT5  + i * 0x100, PhyVal);
				/*RIGHT DL_RX_DIG_TBIT5*/
				PhyVal = mipi_dphy_read(hw, DL0_RIGHT_RX_DIG_TBIT5  + i * 0x100);

				PhyVal &= ~TM_DCC_COMP_CAL_ITER_WAIT_TIME_MASK;
				PhyVal |= ((0xFF << TM_DCC_COMP_CAL_ITER_WAIT_TIME_OFFSET) & TM_DCC_COMP_CAL_ITER_WAIT_TIME_MASK);

				mipi_dphy_write(hw, DL0_RIGHT_RX_DIG_TBIT5  + i * 0x100, PhyVal);

				/*LEFT DL_RX_DIG_TBIT7*/
				PhyVal = mipi_dphy_read(hw, DL0_LEFT_RX_DIG_TBIT7  + i * 0x100);

				PhyVal &= ~TM_MIXER_COMP_CAL_INIT_WAIT_TIME_MASK;
				PhyVal |= ((0xFF << TM_MIXER_COMP_CAL_INIT_WAIT_TIME_OFFSET) & TM_MIXER_COMP_CAL_INIT_WAIT_TIME_MASK);

				mipi_dphy_write(hw, DL0_LEFT_RX_DIG_TBIT7  + i * 0x100, PhyVal);
				/*RIGHT DL_RX_DIG_TBIT7*/
				PhyVal = mipi_dphy_read(hw, DL0_RIGHT_RX_DIG_TBIT7  + i * 0x100);

				PhyVal &= ~TM_MIXER_COMP_CAL_INIT_WAIT_TIME_MASK;
				PhyVal |= ((0xFF << TM_MIXER_COMP_CAL_INIT_WAIT_TIME_OFFSET) & TM_MIXER_COMP_CAL_INIT_WAIT_TIME_MASK);

				mipi_dphy_write(hw, DL0_RIGHT_RX_DIG_TBIT7  + i * 0x100, PhyVal);
				/*LEFT DL_RX_DIG_TBIT9*/
				PhyVal = mipi_dphy_read(hw, DL0_LEFT_RX_DIG_TBIT9  + i * 0x100);

				PhyVal &= ~TM_POS_SAMP_CAL_INIT_WAIT_TIME_MASK;
				PhyVal |= ((0xFF << TM_POS_SAMP_CAL_INIT_WAIT_TIME_OFFSET) & TM_POS_SAMP_CAL_INIT_WAIT_TIME_MASK);

				mipi_dphy_write(hw, DL0_LEFT_RX_DIG_TBIT9  + i * 0x100, PhyVal);
				/*RIGHT DL_RX_DIG_TBIT9*/
				PhyVal = mipi_dphy_read(hw, DL0_RIGHT_RX_DIG_TBIT9  + i * 0x100);

				PhyVal &= ~TM_POS_SAMP_CAL_INIT_WAIT_TIME_MASK;
				PhyVal |= ((0xFF << TM_POS_SAMP_CAL_INIT_WAIT_TIME_OFFSET) & TM_POS_SAMP_CAL_INIT_WAIT_TIME_MASK);

				mipi_dphy_write(hw, DL0_RIGHT_RX_DIG_TBIT9  + i * 0x100, PhyVal);
				/*LEFT DL_RX_DIG_TBIT12*/
				PhyVal = mipi_dphy_read(hw, DL0_LEFT_RX_DIG_TBIT12  + i * 0x100);

				PhyVal &= ~TM_NEG_SAMP_CAL_INIT_WAIT_TIME_MASK;
				PhyVal |= ((0xFF << TM_NEG_SAMP_CAL_INIT_WAIT_TIME_OFFSET) & TM_NEG_SAMP_CAL_INIT_WAIT_TIME_MASK);

				mipi_dphy_write(hw, DL0_LEFT_RX_DIG_TBIT12  + i * 0x100, PhyVal);
				/*RIGHT DL_RX_DIG_TBIT12*/
				PhyVal = mipi_dphy_read(hw, DL0_RIGHT_RX_DIG_TBIT12  + i * 0x100);

				PhyVal &= ~TM_NEG_SAMP_CAL_INIT_WAIT_TIME_MASK;
				PhyVal |= ((0xFF << TM_NEG_SAMP_CAL_INIT_WAIT_TIME_OFFSET) & TM_NEG_SAMP_CAL_INIT_WAIT_TIME_MASK);

				mipi_dphy_write(hw, DL0_RIGHT_RX_DIG_TBIT12  + i * 0x100, PhyVal);
			}

		}

		PhyVal = (band_id << DPHY_BAND_CFG_LEFT_LANE_OFFSET) |
			(band_id << DPHY_BAND_CFG_RIGHT_LANE_OFFSET);

		mipi_dphy_write(hw, DPHY_PCS_BAND_CFG, PhyVal);

		/* step 7 */
		PhyVal = DPHY_POWER_ISLAND_EN_DATA_VAL;

		mipi_dphy_write(hw, DPHY_POWER_ISLAND_EN_DATA, PhyVal);
		/* step 8 */
		PhyVal = DPHY_POWER_ISLAND_EN_CLK_VAL;
		mipi_dphy_write(hw, DPHY_POWER_ISLAND_EN_CLK, PhyVal);

		udelay(500);

		/*step 9 wait for dphy machine has reached ready state*/
		for (i = 0; i < 10; i++){
			PhyVal = mipi_dphy_read(hw, CMN_DIG_TBIT56);
			if (PhyVal & 0x01) {
				dev_info(dev, "dphy is ready PhyVal = 0x%x\n", PhyVal);
				break;
			}

			udelay(500);
		}

		dev_info(dev, "mipi_dphy_config exit\n");
		return ret;
	} else {

		dev_info(dev, "mipi_dphy lane rate work at 2.5G \n");

		for(loop = 0;loop < ARRAY_SIZE(dphy_config_2500M);loop++)
		{
			mipi_dphy_write(hw,dphy_config_2500M[loop].offset,dphy_config_2500M[loop].value);
		}

		msleep(100);
	}

	return 0;
}

static int csi2_dphy_hw_stream_on(struct dphy_rx *dphy, unsigned int id,unsigned int lane_rate)
{
	struct cdns_dphy_rx *hw = dphy->dphy_hw;
	struct device *dev = dphy->dev;

	mutex_lock(&hw->mutex);
	/*here we need config the full mode or split mode on depend on the id */

	dev_info(dev, "virtual dphy id %d stream on enter\n",id);
	mipi_dphy_config(dphy,lane_rate);
	atomic_inc(&hw->stream_cnt);
	/*low power*/
	mutex_unlock(&hw->mutex);

	return 0;
}

static int csi2_dphy_hw_stream_off(struct dphy_rx *dphy, unsigned int id)
{
	struct cdns_dphy_rx *hw = dphy->dphy_hw;
	struct device *dev = dphy->dev;

	mutex_lock(&hw->mutex);
	/*here we need config the full mode or split mode off depend on the id */

	dev_info(dev, "virtual dphy id %d stream off enter\n",id);
	atomic_dec(&hw->stream_cnt);

	/*low power*/
	mutex_unlock(&hw->mutex);

	return 0;
}

static const struct dphy_hw_drv_data cix_dphy_hw_drv_data = {
	.stream_on = csi2_dphy_hw_stream_on,
	.stream_off = csi2_dphy_hw_stream_off,
	.dphy_hw_resume =  mipi_dphy_hw_dev_resume,
	.dphy_hw_suspend = mipi_dphy_hw_dev_suspend,
};

static int mipi_dphy_hw_parse(struct cdns_dphy_rx *dphy)
{
	struct device *dev = dphy->dev;
	struct device_node *node = dev->of_node;
	int ret;

	/*get the id */
	if (has_acpi_companion(dev)) {
		ret = device_property_read_u8(dev, CIX_MIPI_DPHY_HW_OF_NODE_NAME, &dphy->id);
	} else {
		ret = dphy->id = of_alias_get_id(node, CIX_MIPI_DPHY_HW_OF_NODE_NAME);
	}
	if ((ret < 0) || (dphy->id >= CIX_BRIDGE_MAX_DEVS)) {
		dev_err(dev, "Invalid driver data or device id (%d)\n",
			dphy->id);
		return -EINVAL;
	}

	/*get the clk & reset*/
	dphy->psm_clk = devm_clk_get_optional(dev, "phy_psmclk");
	if (IS_ERR(dphy->psm_clk)) {
		dev_err(dev, "Couldn't get phy_psmclk clock\n");
		return PTR_ERR(dphy->psm_clk);
	}

	dphy->apb_clk = devm_clk_get_optional(dev, "phy_apbclk");
	if (IS_ERR(dphy->apb_clk)) {
		dev_err(dev, "Couldn't get phy_apbclk clock\n");
		return PTR_ERR(dphy->apb_clk);
	}

	dphy->rst_dphy = devm_reset_control_get_optional_shared(dev, "phy_prst");
	if (IS_ERR(dphy->rst_dphy)) {
		dev_err(dev, "failed to get reset phy_prst\n");
		return PTR_ERR(dphy->rst_dphy);
	}

	dphy->phy_cmnrst = devm_reset_control_get_optional_shared(dev, "phy_cmnrst");
	if (IS_ERR(dphy->rst_dphy)) {
		dev_err(dev, "failed to get reset phy_cmnrst\n");
		return PTR_ERR(dphy->rst_dphy);
	}

	return 0;
}

static const struct of_device_id mipi_dphy_hw_of_match[] = {
	{
		.compatible = "cix,cix-mipi-dphy-hw",
		.data = &cix_dphy_hw_drv_data
	},

	{ /* sentinel */ },
};
MODULE_DEVICE_TABLE(of, mipi_dphy_hw_of_match);

static const struct acpi_device_id mipi_dphy_hw_acpi_match[] = {
	{ .id = "CIXH302A", .driver_data = (long unsigned int)&cix_dphy_hw_drv_data },
	{ /* sentinel */ },
};
MODULE_DEVICE_TABLE(acpi, mipi_dphy_hw_acpi_match);

static int mipi_dphy_hw_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct cdns_dphy_rx *dphy;
	int ret;

	dev_info(dev, "mipi-dphy hw probe enter\n");

	dphy = devm_kzalloc(dev, sizeof(*dphy), GFP_KERNEL);
	if (!dphy)
		return -ENOMEM;

	dphy->dev = dev;
	dphy->pdev = pdev;
	dphy->regs = devm_platform_ioremap_resource(pdev, 0);
	if (IS_ERR(dphy->regs))
		return PTR_ERR(dphy->regs);

	dphy->drv_data = device_get_match_data(dev);

	mipi_dphy_hw_parse(dphy);

	mutex_init(&dphy->mutex);

	platform_set_drvdata(pdev, dphy);

	dev_info(dev, "mipi-dphy hw probe exit %s \n",ret == 0 ? "success":"failed");

	return ret;
}

static int mipi_dphy_hw_remove(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;

	dev_info(dev, "mipi-dphy hw remove enter\n");
	return 0;
}

/*for pm interface ,here work at full mode, if work at split mode need more process*/
static int mipi_dphy_hw_dev_suspend(struct dphy_rx *dphy_rx)
{
	struct cdns_dphy_rx *dphy = dphy_rx->dphy_hw;

	if (!dphy->rst_dphy || !dphy->phy_cmnrst
	 || !dphy->psm_clk || !dphy->apb_clk)
		return -EINVAL;

	reset_control_assert(dphy->rst_dphy);
	reset_control_assert(dphy->phy_cmnrst);
	clk_disable_unprepare(dphy->psm_clk);
	clk_disable_unprepare(dphy->apb_clk);

	return 0;
}

static int mipi_dphy_hw_dev_resume(struct dphy_rx *dphy_rx)
{
	struct cdns_dphy_rx *dphy = dphy_rx->dphy_hw;
	int ret;

	if (!dphy->rst_dphy || !dphy->phy_cmnrst
	 || !dphy->psm_clk || !dphy->apb_clk)
		return -EINVAL;

	ret = clk_prepare_enable(dphy->psm_clk);
	if (ret < 0) {
		dev_err(dphy->dev, "%s, enable psm_clk error\n", __func__);
		return ret;
	}

	ret = clk_prepare_enable(dphy->apb_clk);
	if (ret < 0) {
		dev_err(dphy->dev, "%s, enable apb_clk error\n", __func__);
		return ret;
	}

	reset_control_deassert(dphy->rst_dphy);
	reset_control_deassert(dphy->phy_cmnrst);

	return 0;
}

static struct platform_driver mipi_dphy_hw_driver = {
	.driver = {
		.name = CIX_MIPI_DPHY_HW_DRIVER_NAME,
		.of_match_table = mipi_dphy_hw_of_match,
		.acpi_match_table = ACPI_PTR(mipi_dphy_hw_acpi_match),
	},
	.probe = mipi_dphy_hw_probe,
	.remove = mipi_dphy_hw_remove,
};

module_platform_driver(mipi_dphy_hw_driver);
MODULE_AUTHOR("Cix Semiconductor, Inc.");
MODULE_DESCRIPTION("Cix MIPI DPHY RX HWdriver");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:" CIX_MIPI_DPHY_HW_DRIVER_NAME);
