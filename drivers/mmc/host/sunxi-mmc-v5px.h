#ifdef CONFIG_ARCH_SUN8IW10P1

#ifndef __SUNXI_MMC_SUN8IW10P1_2_H__
#define __SUNXI_MMC_SUN8IW10P1_2_H__


void sunxi_mmc_init_priv_v5px(struct sunxi_mmc_host *host,
			       struct platform_device *pdev, int phy_index);
int sunxi_mmc_oclk_onoff(struct sunxi_mmc_host *host, u32 oclk_en);

#endif

#endif
