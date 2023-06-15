/*
 * Allwinner DDR Clock driver.
 *
 * Copyright (C) 2020 Allwinner Technology, Inc.
 *	fanqinghua <fanqinghua@allwinnertech.com>
 *
 * Implementation of ddr clock source driver.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/module.h>
#include <linux/clk.h>
#include <linux/clk-provider.h>
#include <linux/io.h>
#include <linux/slab.h>
#include <linux/err.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/regmap.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_platform.h>
#include <soc/allwinner/sunxi_sip.h>

/* phy registers */
#define MX_MSTR                 0x0
#define MX_STAT                 0x4
#define MX_MSTR2                0x28
#define MX_PWRCTL               0x30
#define MX_DFIMISC              0x1B0
#define MX_DFISTAT              0x1BC
#define MX_DBG1                 0x304
#define MX_DBGCAM               0x308
#define MX_SWCTL                0x320
#define MX_SWSTAT               0x324

#define DRAM_CLK_REG			0x800

#define MC_DFS_CONFIG	        0x1000
#define MC_DFS_CMD(x)	        (0x1800 + (x) * 0x10) //x=0~63
#define MC_DFS_WDATA_RDATA(x)	(0x1804 + (x) * 0x10) //x=0~63
#define MC_DFS_DATA_MASK(x)	    (0x1808 + (x) * 0x10) //x=0~63
#define MC_DFS_STATE_RB			0x180C
#define MC_DFS_DIV				0x18E0
#define LAST_CMD				BIT(31)
#define PADHOLD_CLR				BIT(25)
#define PADHOLD_SET				BIT(24)
#define CMD_CCU					BIT(19)
#define ACC_PHY					BIT(18)
#define CMD_WR					BIT(16)

#define DIV_SHIFT			0
#define DIV_WIDTH			5

#define DRIVER_NAME	"DDR Clock Driver"

/*
 * PLL_DDR / (DIV + 1) = DDR_CLK;
 * DDR_PHY_CLK = DDR_CLK * 2;
 * DDR_PHY_IO_CLK = DDR_CLK * 4;
 * So (DIV + 1) = PLL_DDR / (DDR_PHY_CLK / 2);
 * So (DIV + 1) = (PLL_DDR / DDR_PHY_CLK) * 2;
 * The freq in opp table is DDR_PHY_CLK.
 */

struct sunxi_ddrclk {
	struct device *dev;
	void __iomem	*ccmu_base;
	void __iomem	*mctl_base;
	unsigned int	dram_clk;
	unsigned int	dram_div;
	struct clk_hw	hw;
	struct mutex  ddrfreq_lock;
	spinlock_t      lock;
};

#define to_sunxi_ddrclk_hw(_hw) container_of(_hw, struct sunxi_ddrclk, hw)

static inline void mdfs_conf(struct sunxi_ddrclk *ddrclk,
								unsigned int reg,
								unsigned int flag,
								unsigned int data,
								unsigned int dmask,
								unsigned int id)
{
	writel_relaxed(reg | flag, ddrclk->mctl_base + MC_DFS_CMD(id));
	writel_relaxed(data, ddrclk->mctl_base + MC_DFS_WDATA_RDATA(id));
	writel_relaxed(dmask, ddrclk->mctl_base + MC_DFS_DATA_MASK(id));
}

#ifndef SUNXI_DDR_DFS
static inline int set_ddrfreq(struct sunxi_ddrclk *ddrclk, unsigned int freq_id)
{
	int result;

	result = invoke_scp_fn_smc(ARM_SVC_SUNXI_DDRFREQ, freq_id, 0, 0);

	return result;
}
#else
static int set_ddrfreq(struct sunxi_ddrclk *ddrclk, unsigned int freq_id)
{
	unsigned int dram_freq_div = (ddrclk->dram_div >> (freq_id * 8)) & 0x1f;

	unsigned int cmd_id = 0, timeout;
	unsigned long flags;

	spin_lock_irqsave(&ddrclk->lock, flags);
	/*2. 开始预配置DFS命令cache*/
	/*2.1 频率切换前准备*/
	mdfs_conf(ddrclk, MX_DBG1, CMD_WR, (0x1 << 1), 0xFFFFFFFD, cmd_id++);  //配置控制器，不再接收新命令
	mdfs_conf(ddrclk, MX_DBGCAM, 0x0, 0x36000000, 0xC9FFFFFF, cmd_id++); //读控制器，确保读写buffer都是空的(等待)，确保DFI命令全部发完(等待)
	mdfs_conf(ddrclk, MX_PWRCTL, CMD_WR, (0x1 << 5), 0xFFFFFFDF, cmd_id++); //配置控制器，进入刷新模式
	mdfs_conf(ddrclk, MX_STAT, 0x0, 0x3, 0xFFFFFFFC, cmd_id++); //读控制器，确保处于自刷新模式
	mdfs_conf(ddrclk, MX_DBG1, PADHOLD_SET | CMD_WR, 0x0, 0xFFFFFFFF, cmd_id++);  //pad hold打开,注：写时全部bit mask,写无效
	mdfs_conf(ddrclk, MX_DBG1,  CMD_WR, (0x1 << 0), 0xFFFFFFFE, cmd_id++);  //1)配置控制器，关闭控制器数据通道，控制器不再接收发送新的命令;
	mdfs_conf(ddrclk, MX_DBGCAM, 0x0, 0x36000000, 0xC9FFFFFF, cmd_id++); //读控制器，确保读写buffer都是空的(等待)，确保DFI命令全部发完(等待)

	mdfs_conf(ddrclk, MX_SWCTL, CMD_WR, (0x0 << 0), 0xFFFFFFFE, cmd_id++); //开启控制器quasi-dynamic register配置
	mdfs_conf(ddrclk, MX_DFIMISC, CMD_WR, (0x0 << 0), 0xFFFFFFFE, cmd_id++); //配置控制器，确保控制器的状态不会被complete信号复位
	mdfs_conf(ddrclk, MX_DFIMISC, CMD_WR, (0x1 << 5), 0xFFFFFFDF, cmd_id++); //set the dfi_init_start to 1,切换场景
	mdfs_conf(ddrclk, MX_SWCTL, CMD_WR, (0x1 << 0), 0xFFFFFFFE, cmd_id++); //关闭控制器quasi-dynamic register配置
	mdfs_conf(ddrclk, MX_SWSTAT, 0x0, 0x1, 0xFFFFFFFE, cmd_id++); //读控制器，确保quasi-dynamic register配置完成

	mdfs_conf(ddrclk, 0x53 << 2, ACC_PHY | CMD_WR, (0x1 << 3), 0xFFFFFFF7, cmd_id++); //Power down the PLL;
	mdfs_conf(ddrclk, 0x16 << 2, ACC_PHY | CMD_WR, (0x0 << 5), 0xFFFFFFDF, cmd_id++); //Gating the 1x clock of the PHY.
	/*2.2 频率切换*/
	mdfs_conf(ddrclk, dram_freq_div, CMD_CCU, 0x0, 0x0, cmd_id++); //配置ccu寄存器，修改频率
	/*2.3 频率切换后，配置PHY*/
	mdfs_conf(ddrclk, 0x53 << 2, ACC_PHY | CMD_WR, (0x0 << 3), 0xFFFFFFF7, cmd_id++); //Enable the PLL and wait pll lock.
	mdfs_conf(ddrclk, 0x60 << 2, ACC_PHY, 0x4, 0xFFFFFFFB, cmd_id++); //CMD and DQ PLL lock flag. High means PLL is lock.
	mdfs_conf(ddrclk, 0x16 << 2, ACC_PHY | CMD_WR, (0x1 << 5), 0xFFFFFFDF, cmd_id++); //Enable the 1x clock of the PHY.

	/*2.4 频率切换后，配置控制器*/
	mdfs_conf(ddrclk, MX_SWCTL, CMD_WR, (0x0 << 0), 0xFFFFFFFE, cmd_id++); //开启控制器quasi-dynamic register配置
	mdfs_conf(ddrclk, MX_MSTR, CMD_WR, (0x1 << 29), 0xDFFFFFFF, cmd_id++); //打开控制器配置频点的开关
	mdfs_conf(ddrclk, MX_MSTR2, CMD_WR, freq_id << 0, 0xFFFFFFFC, cmd_id++); //控制器从4套预配置频点中选择1个
	mdfs_conf(ddrclk, MX_DFIMISC, CMD_WR, (0x0 << 5), 0xFFFFFFDF, cmd_id++); //set the dfi_init_start to 0，dfi init
	mdfs_conf(ddrclk, MX_SWCTL, CMD_WR, (0x1 << 0), 0xFFFFFFFE, cmd_id++); //关闭控制器quasi-dynamic register配置
	mdfs_conf(ddrclk, MX_SWSTAT, 0x0, (0x1 << 0), 0xFFFFFFFE, cmd_id++); //读控制器，确保quasi-dynamic register配置完成
	mdfs_conf(ddrclk, MX_DFISTAT, 0x0, (0x1 << 0), 0xFFFFFFFE, cmd_id++); //读控制器，确保DFI初始化完成
	mdfs_conf(ddrclk, 0x15 << 2, ACC_PHY | CMD_WR, (freq_id << 2) | (0x1 << 5), 0xFFFFFF03, cmd_id++); //配置PHY，选择预配置好的频点，开启快速切换RX-calibration结果
	mdfs_conf(ddrclk, MX_PWRCTL, PADHOLD_CLR | CMD_WR, 0x0, 0xFFFFFFFF, cmd_id++); //pad hold释放,注：写时全部bit mask,写无效
	mdfs_conf(ddrclk, MX_PWRCTL, CMD_WR, (0x0 << 5), 0xFFFFFFDF, cmd_id++); //1)配置控制器，退出刷新模式,
	mdfs_conf(ddrclk, MX_STAT, 0x0, 0x1, 0xFFFFFFFC, cmd_id++); //读控制器，确保进入normal模式
	mdfs_conf(ddrclk, MX_DBG1, CMD_WR, (0x0 << 0), 0xFFFFFFFC, cmd_id++); //配置控制器，开启接收新命令
	mdfs_conf(ddrclk, MX_STAT, LAST_CMD | CMD_WR, 0x0, 0xFFFFFFFF, cmd_id++); //最后一条命令的标志,注：写时全部bit mask,写无效

	/*3. DFS触发开启*/
	writel_relaxed(readl_relaxed(ddrclk->mctl_base + MC_DFS_CONFIG) | (0x1 << 8) | (0x1 << 2) | 0x1,
					ddrclk->mctl_base + MC_DFS_CONFIG);
	spin_unlock_irqrestore(&ddrclk->lock, flags);
	/* Wait max 200us */
	timeout = 20;
	while (readl_relaxed(ddrclk->mctl_base + MC_DFS_STATE_RB) >> 31 != 0x1) {
		if (!timeout) {
			dev_err(ddrclk->dev, "DRAM DFS timeout!\n");
			return;
		}
		timeout--;
		cpu_relax();
		usleep_range(5, 10);
	}

	return 0;
}
#endif

static unsigned long sunxi_ddr_clk_recalc_rate(struct clk_hw *hw,
					       unsigned long parent_rate)
{
	struct sunxi_ddrclk *ddrclk = to_sunxi_ddrclk_hw(hw);
	unsigned int reg_val, div;
	unsigned long rate = parent_rate;

	reg_val = readl_relaxed(ddrclk->ccmu_base + DRAM_CLK_REG);

	div = (reg_val >> DIV_SHIFT) & GENMASK(DIV_WIDTH - 1, 0);
	rate /= (div + 1);

	return rate << 1;
}

static long sunxi_ddr_clk_round_rate(struct clk_hw *hw,
				     unsigned long target_rate,
				     unsigned long *prate)
{
	struct sunxi_ddrclk *ddrclk = to_sunxi_ddrclk_hw(hw);
	unsigned int dram_div = ddrclk->dram_div;

	if (target_rate <= (*prate << 1) / (((dram_div >> 24) & 0x1f) + 1))
		return (*prate << 1) / (((dram_div >> 24) & 0x1f) + 1);
	else if (target_rate <= (*prate << 1) / (((dram_div >> 16) & 0x1f) + 1))
		return (*prate << 1) / (((dram_div >> 16) & 0x1f) + 1);
	else if (target_rate <= (*prate << 1) / (((dram_div >> 8) & 0x1f) + 1))
		return (*prate << 1) / (((dram_div >> 8) & 0x1f) + 1);
	else
		return (*prate << 1) / ((dram_div & 0x1f) + 1);
}

static int sunxi_ddr_clk_set_rate(struct clk_hw *hw, unsigned long drate,
				  unsigned long prate)
{
	struct sunxi_ddrclk *ddrclk = to_sunxi_ddrclk_hw(hw);
	unsigned int dram_div = ddrclk->dram_div;
	unsigned int freq_id;

	if (drate <= (prate << 1) / (((dram_div >> 24) & 0x1f) + 1))
		freq_id = 3;
	else if (drate <= (prate << 1) / (((dram_div >> 16) & 0x1f) + 1))
		freq_id = 2;
	else if (drate <= (prate << 1) / (((dram_div >> 8) & 0x1f) + 1))
		freq_id = 1;
	else
		freq_id = 0;

	dev_info(ddrclk->dev, "prate:%ld, drate:%ld, freq_id:%d, dram_div:0x%x\n", prate, drate, freq_id, dram_div);
	mutex_lock(&ddrclk->ddrfreq_lock);
	set_ddrfreq(ddrclk, freq_id);
	ddrclk->dram_clk = drate;
	mutex_unlock(&ddrclk->ddrfreq_lock);

	return 0;
}

const struct clk_ops sunxi_ddrclk_ops = {
	.recalc_rate = sunxi_ddr_clk_recalc_rate,
	.round_rate = sunxi_ddr_clk_round_rate,
	.set_rate = sunxi_ddr_clk_set_rate,
};

static const struct of_device_id clk_ddr_of_match[] = {
	{
		.compatible = "allwinner,clock_ddr",
	},
	{ },
};
MODULE_DEVICE_TABLE(of, clk_ddr_of_match);

static int ddr_clock_probe(struct platform_device *pdev)
{
	struct device_node *np = pdev->dev.of_node;
	struct device_node *dram_np;
	struct sunxi_ddrclk *ddrclk;
	struct clk_init_data init;
	struct clk *clk, *parent_clk;
	const char *parent_name;
	int ret = 0;

	if (!np) {
		dev_err(&pdev->dev, "failed to match ddr clock\n");
		return -ENODEV;
	}

	ddrclk = devm_kzalloc(&pdev->dev, sizeof(*ddrclk), GFP_KERNEL);
	if (!ddrclk)
		return -ENOMEM;

	ddrclk->dev = &pdev->dev;
	platform_set_drvdata(pdev, ddrclk);

	mutex_init(&ddrclk->ddrfreq_lock);
	dram_np = of_find_node_by_path("/dram");
	if (!dram_np) {
		dev_err(&pdev->dev, "failed to find dram node\n");
		return -ENODEV;
	}

	ret = of_property_read_u32(dram_np, "dram_para[00]", &ddrclk->dram_clk);
	if (ret) {
		dev_err(&pdev->dev, "failed to find dram_clk\n");
		return -ENODEV;
	}

	ret = of_property_read_u32(dram_np, "dram_para[24]", &ddrclk->dram_div);
	if (ret) {
		dev_err(&pdev->dev, "failed to find dram_div\n");
		return -ENODEV;
	}

	ddrclk->ccmu_base = of_iomap(np, 0);
	if (!ddrclk->ccmu_base) {
		dev_err(&pdev->dev, "map ccmu failed\n");
		return -ENODEV;
	}

	ddrclk->mctl_base = devm_of_iomap(&pdev->dev, np, 1, NULL);
	if (!ddrclk->mctl_base) {
		dev_err(&pdev->dev, "map mctl failed\n");
		return -ENODEV;
	}

	parent_clk = devm_clk_get(&pdev->dev, "pll_ddr");
	if (IS_ERR(parent_clk)) {
		dev_err(&pdev->dev, "clk_get pll_ddr failed\n");
		return -ENODEV;
	}

	parent_name = __clk_get_name(parent_clk);
	if (!parent_name) {
		dev_err(&pdev->dev, "get clk name failed\n");
		return -ENODEV;
	}

	ddrclk->hw.init = &init;
	init.name = "sdram";
	init.ops = &sunxi_ddrclk_ops;
	init.parent_names = &parent_name;
	init.num_parents = 1;
	init.flags |= CLK_SET_RATE_NO_REPARENT | CLK_GET_RATE_NOCACHE;

	clk = devm_clk_register(&pdev->dev, &ddrclk->hw);
	if (IS_ERR(clk)) {
		dev_err(&pdev->dev, "clk_register failed\n");
		return -ENODEV;
	}

	of_clk_add_provider(np, of_clk_src_simple_get, clk);
	return 0;
}

static int ddr_clock_remove(struct platform_device *pdev)
{
	struct device_node *np = pdev->dev.of_node;

	of_clk_del_provider(np);
	return 0;
}

static struct platform_driver ddr_clock_driver = {
	.probe   = ddr_clock_probe,
	.remove  = ddr_clock_remove,
	.driver  = {
		.name  = "sunxi-ddrclock",
		.of_match_table = clk_ddr_of_match,
	},
};

module_platform_driver(ddr_clock_driver);
MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("Allwinner DDR Clock driver");
MODULE_ALIAS("platform:" DRIVER_NAME);
MODULE_AUTHOR("fanqinghua <fanqinghua@allwinnertech.com>");
MODULE_VERSION("1.0.0");
