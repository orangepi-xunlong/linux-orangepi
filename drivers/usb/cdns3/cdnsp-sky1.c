// SPDX-License-Identifier: GPL-2.0
/*
 * cdnsp-sky1 - USB controller driver for CIX's sky1 SoCs
 *
 * Author: Chao Zeng <Chao.Zeng@cixtech.com>
 * Author: Matthew MA <Matthew.Ma@cixtech.com>
 */

#include <linux/acpi.h>
#include <linux/bits.h>
#include <linux/clk.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/io.h>
#include <linux/of_platform.h>
#include <linux/pm_runtime.h>
#include <linux/clk.h>
#include <linux/reset.h>
#include <linux/regmap.h>
#include <linux/mfd/syscon.h>
#include <linux/of_address.h>
#include <linux/device.h>

#include "core.h"
#include "../host/xhci.h"
#include "../host/xhci-plat.h"
#include "cdnsp-sky1.h"

static const char *cix_usb_clk_names[CIX_USB_CLK_NUM] = {
		"sof_clk", "usb_aclk", "lpm_clk", "usb_pclk"
};

struct sky1_src_signal {
	unsigned int offset, bit;
};

static const struct sky1_src_signal sky1_usb_signals[SKY1_USB_S5_NUM] = {
	/* usb config in s5 domain */
	[U3_TYPEC_DRD_ID]	= { USB_MODE_STRAP_S5_DOMAIN, U3_TYPEC_DRD_MODE_STRAP_BIT },
	[U3_TYPEC_HOST0_ID]	= { USB_MODE_STRAP_S5_DOMAIN, U3_TYPEC_HOST0_MODE_STRAP_BIT },
	[U3_TYPEC_HOST1_ID]	= { USB_MODE_STRAP_S5_DOMAIN, U3_TYPEC_HOST1_MODE_STRAP_BIT },
	[U3_TYPEC_HOST2_ID]	= { USB_MODE_STRAP_S5_DOMAIN, U3_TYPEC_HOST2_MODE_STRAP_BIT },
	[U3_TYPEA_CTRL0_ID]	= { USB_MODE_STRAP_S5_DOMAIN, U3_TYPEA_CTRL0_MODE_STRAP_BIT },
	[U3_TYPEA_CTRL1_ID]	= { USB_MODE_STRAP_S5_DOMAIN, U3_TYPEA_CTRL1_MODE_STRAP_BIT},
	[U2_HOST0_ID]		= { USB_MODE_STRAP_S5_DOMAIN, U2_HOST0_MODE_STRAP_BIT },
	[U2_HOST1_ID]		= { USB_MODE_STRAP_S5_DOMAIN, U2_HOST1_MODE_STRAP_BIT },
	[U2_HOST2_ID]		= { USB_MODE_STRAP_S5_DOMAIN, U2_HOST2_MODE_STRAP_BIT },
	[U2_HOST3_ID]		= { USB_MODE_STRAP_S5_DOMAIN, U2_HOST3_MODE_STRAP_BIT },
};

static int sky1_set_mode_by_id(struct device *dev, int mode)
{
	struct cdnsp_sky1 *data = dev_get_drvdata(dev);

	return regmap_update_bits(data->usb_syscon,
			sky1_usb_signals[data->id].offset,
			GENMASK(sky1_usb_signals[data->id].bit+1,
			sky1_usb_signals[data->id].bit),
			mode << sky1_usb_signals[data->id].bit);
}

/**
 * sky1_usb_clk_enable_all() - enable all clocks for usb controller
 * @dev:	Pointer to the device of platform_device
 *
 */

int sky1_usb_clk_enable_all(struct device *dev)
{
	int i, ret;
	struct cdnsp_sky1 *data = dev_get_drvdata(dev);
	struct clk **cix_usb_clks = data->cix_usb_clks;

	dev_info(dev, "---------sky1_usb_clk_enable_all----------\n");

	for (i = 0; i < CIX_USB_CLK_NUM; i++) {
		cix_usb_clks[i] = devm_clk_get(dev, cix_usb_clk_names[i]);
		if (IS_ERR(cix_usb_clks[i])) {
			ret = dev_err_probe(dev, PTR_ERR(cix_usb_clks[i]),
				"could not get %s clock\n", cix_usb_clk_names[i]);
			goto err_usb_clks;
		}

		ret = clk_prepare_enable(cix_usb_clks[i]);
		if (ret) {
			dev_err(dev, "%s enable failed:%d\n", cix_usb_clk_names[i], ret);
			goto err_usb_clks;
		}
	}

	dev_dbg(dev, "enable sky1 USB clock done\n");
	return 0;

err_usb_clks:
	cix_usb_clks[i] = NULL;
	while (--i >= 0) {
		clk_disable_unprepare(cix_usb_clks[i]);
		cix_usb_clks[i] = NULL;
	}

	return ret;
};

/**
 * sky1_usb_clk_disable_all() - disable all clocks for usb controller
 * @dev:	Pointer to the device of platform_device
 *
 */

static void sky1_usb_clk_disable_all(struct device *dev)
{
	int i;
	struct cdnsp_sky1 *data = dev_get_drvdata(dev);
	struct clk **cix_usb_clks = data->cix_usb_clks;

	dev_info(dev, "---------sky1_usb_clk_disable_all----------\n");

	for (i = 0; i < CIX_USB_CLK_NUM; i++) {
		clk_disable_unprepare(cix_usb_clks[i]);
	}
};

/**
 * sky1_usb_clk_enable_resume() - enable the clocks that are turned
 * off while suspend
 * @dev:	Pointer to the device of platform_device
 *
 */

static int sky1_usb_clk_enable_resume(struct device *dev)
{
	int i, ret;
	struct cdnsp_sky1 *data = dev_get_drvdata(dev);
	struct clk **cix_usb_clks = data->cix_usb_clks;

	dev_info(dev, "---------sky1_usb_clk_enable_resume----------\n");

	for (i = 0; i < CIX_USB_CLK_OFF_NUM; i++) {
		ret = clk_prepare_enable(cix_usb_clks[i]);
		if (ret) {
			printk(KERN_ERR"[%s:%d] enable clock:%s error:%d\n", __func__, __LINE__, cix_usb_clk_names[i], ret);
			goto err_usb_clks;
		}
	}
	return 0;

err_usb_clks:
	cix_usb_clks[i] = NULL;
	while (--i >= 0) {
		clk_disable_unprepare(cix_usb_clks[i]);
		cix_usb_clks[i] = NULL;
	}
	return ret;
};

/**
 * sky1_usb_clk_disable_suspend() - disable the clocks which are not
 * needed when suspend
 * @dev:	Pointer to the device of platform_device
 *
 */

static void sky1_usb_clk_disable_suspend(struct device *dev)
{
	int i;
	struct cdnsp_sky1 *data = dev_get_drvdata(dev);
	struct clk **cix_usb_clks = data->cix_usb_clks;
	dev_info(dev, "---------sky1_usb_clk_disable_suspend----------\n");

	for (i = 0; i < CIX_USB_CLK_OFF_NUM; i++) {
		clk_disable_unprepare(cix_usb_clks[i]);
	}
};

#define XECP_PM_PMCSR 0x2240
/* XECP_PM_PMCSR */
#define PS_MASK			GENMASK(1, 0)
#define PS_D0			0
#define PS_D1			1
#define PS_D2			2
#define PS_D3			3
#define PS_PME_En		(1 << 8)

int sky1_handshake(void __iomem *ptr, u32 mask, u32 done, u64 timeout_us)
{
	u32	result;
	int	ret;

	ret = readl_poll_timeout_atomic(ptr, result,
					(result & mask) == done ||
					result == U32_MAX,
					1, timeout_us);
	if (result == U32_MAX)		/* card removed */
		return -ENODEV;

	return ret;
}

static int cdns_sky1_platform_suspend(struct device *dev,
		bool suspend, bool wakeup)
{

	struct cdns *cdns = dev_get_drvdata(dev);
	struct platform_device *xhci_dev = cdns->xhci_device;
	struct usb_hcd  *hcd;
	struct xhci_hcd *xhci;
	struct device *parent = cdns->dev->parent;
	struct cdnsp_sky1 *data = dev_get_drvdata(parent);
	u32 value;
	int ret = 0;
	int count = 5;

	if (cdns->role != USB_ROLE_HOST)
		return 0;

	hcd = dev_get_drvdata(&xhci_dev->dev);
	if (!hcd) {
		dev_dbg(dev, "host controller have not registered\n");
		return 0;
	}
	xhci = hcd_to_xhci(hcd);
	dev_info(dev, "[%s:%d] xhci_dev name:%s, hcd->regs:%llx\n", __func__, __LINE__,
		dev_name(&xhci_dev->dev), (long long unsigned int)hcd->regs);

	if (suspend) {
		/* SW request low power when all usb ports allow to it ??? */
		while (count--) {
			value = readl(hcd->regs + XECP_PM_PMCSR);
			value &= ~PS_MASK;
			value |= PS_D3 | PS_PME_En;
			writel(value, hcd->regs + XECP_PM_PMCSR);
			/* after controller enter D3, disable axi and sof until axi valid flag change to 0.
			 */
			if (sky1_handshake(data->ctst_base, AXI_CLOCK_VALID, 0, 100 * 1000))
				dev_err(dev, "[%s:%d] enter D3 failed,register value:%x\n",
					__func__, __LINE__, readl(data->ctst_base));
			else {
				dev_info(dev, "[%s:%d] enter D3 succeed\n", __func__, __LINE__);
				break;
			}
		}
	} else {
		while (count--) {
			value = readl(hcd->regs + XECP_PM_PMCSR);
			value &= ~PS_MASK;
			value |= PS_D0;
			value &= ~PS_PME_En;
			writel(value, hcd->regs + XECP_PM_PMCSR);
			/* Wait power state back to D0 */
			if (sky1_handshake(hcd->regs + XECP_PM_PMCSR, PS_MASK, 0, 100 * 1000))
				dev_err(dev, "[%s:%d] exit D3 timeout, power state=0x%lx\n",
					__func__, __LINE__, readl(hcd->regs + XECP_PM_PMCSR) & PS_MASK);
			else {
				dev_info(dev, "[%s:%d] exit D3 succeed\n", __func__, __LINE__);
				break;
			}
		}
	}

	return ret;

}

static int cdnsp_sky1_drd_init(struct cdnsp_sky1 *data)
{
	int ret;
	int clk;
	int v0, v1, v2;

	reset_control_assert(data->reset);
	reset_control_assert(data->preset);
	sky1_usb_clk_disable_all(data->dev);
	ret = sky1_usb_clk_enable_all(data->dev);
	if (ret)
		return ret;
	writel(CIX_USB_AXI_WR_CACHE_VALUE, data->axi_base);
	sky1_set_mode_by_id(data->dev, MODE_STRAP_OTG);
	reset_control_deassert(data->preset);
	if (data->u3_disable) {
		dev_info(data->dev, "[%s:%d]disable u3 port\n", __func__, __LINE__);
		writel(D_XEC_CFG_3XPORT_MODE_VALUE, (void *)(data->device_base)
			+ D_XEC_CFG_3XPORT_MODE);
	}
	writel(AXI_HALT, (void *)(data->device_base) + D_XEC_AXI_CAP);
	writel(AXI_HALT, (void *)(data->xhci_base) + D_XEC_AXI_CAP);
	writel(data->axi_bmax_value, (void *)(data->device_base) + D_XEC_AXI_CTRL0);
	writel(data->axi_bmax_value, (void *)(data->xhci_base) + D_XEC_AXI_CTRL0);
	writel((unsigned int)(~(AXI_HALT)), (void *)(data->device_base) + D_XEC_AXI_CAP);
	writel((unsigned int)(~(AXI_HALT)), (void *)(data->xhci_base) + D_XEC_AXI_CAP);

	/* v0 = (int) (250 * data->sof_clk_freq / 1000000000) = (int) ((250/10) *
	 * (data->sof_clk_freq ) / (1000000000/10)) = (int) (25 * clk / 100000000)	250ns
	 * v1 = (int) (100 * data->sof_clk_freq / 1000000) = (int) (clk / 10000)	100us
	 * v2 = (int) (100 * data->sof_clk_freq / 1000) = (int) (clk / 10)		100ms
	 */

	clk = (int)data->sof_clk_freq;
	v0 = (int)(25*clk / 100000000);
	v1 = (int)(clk / 10000);
	v2 = (int)(clk/10);

	writel((unsigned int)((v0 > 1) ? v0 - 1 : 1), (void *)data->device_base
		+ D_XEC_PRE_REG_250NS);
	writel((unsigned int)((v1 / 100 > 1) > 0 ? v1 / 100 - 1 : 1),
		(void *)data->device_base + D_XEC_PRE_REG_1US);
	writel((unsigned int)((v1 / 10 > 1) > 0 ? v1 / 10 - 1 : 1),
		(void *)data->device_base + D_XEC_PRE_REG_10US);
	writel((unsigned int)((v1) > 1 ? v1 - 1 : 1), (void *)data->device_base
		+ D_XEC_PRE_REG_100US);
	writel((unsigned int)((125 * clk / 1000000) > 1 ? (125 * clk / 1000000) : 1),
		(void *)data->device_base + D_XEC_PRE_REG_125US);
	writel((unsigned int)((v2 / 100 > 1) ? v2 / 100 - 1 : 1), (void *)data->device_base
		+ D_XEC_PRE_REG_1MS);
	writel((unsigned int)((v2 / 10) > 1 ? v2 / 10 - 1 : 1), (void *)data->device_base
		+ D_XEC_PRE_REG_10MS);
	writel((unsigned int)(v2 > 1 ? v2 - 1 : 1), (void *)data->device_base
		+ D_XEC_PRE_REG_100MS);
	dev_dbg(data->dev, "[%s:%d]readl:%x, %x ,%x, %x, %x, %x, %x, %x\n", __func__, __LINE__,
		readl((void *)data->device_base + D_XEC_PRE_REG_250NS),
		readl((void *)data->device_base + D_XEC_PRE_REG_1US),
		readl((void *)data->device_base + D_XEC_PRE_REG_10US),
		readl((void *)data->device_base + D_XEC_PRE_REG_100US),
		readl((void *)data->device_base + D_XEC_PRE_REG_125US),
		readl((void *)data->device_base + D_XEC_PRE_REG_1MS),
		readl((void *)data->device_base + D_XEC_PRE_REG_10MS),
		readl((void *)data->device_base + D_XEC_PRE_REG_100MS));

	clk = (int)data->lpm_clk_freq;
	v0 = (int)(25*clk / 100000000);
	v1 = (int)(clk / 10000);
	v2 = (int)(clk/10);

	writel((unsigned int)((v0 > 1) ? v0 - 1 : 1), (void *)data->device_base
		+ D_XEC_LPM_PRE_REG_250NS);
	writel((unsigned int)((v1 / 100 > 1) > 0 ? v1 / 100 - 1 : 1), (void *)data->device_base
		+ D_XEC_LPM_PRE_REG_1US);
	writel((unsigned int)((v1 / 10 > 1) > 0 ? v1 / 10 - 1 : 1), (void *)data->device_base
		+ D_XEC_LPM_PRE_REG_10US);
	writel((unsigned int)((v1) > 1 ? v1 - 1 : 1), (void *)data->device_base
		+ D_XEC_LPM_PRE_REG_100US);
	writel((unsigned int)((125 * clk / 1000000) > 1 ? (125 * clk / 1000000) : 1),
		(void *)data->device_base + D_XEC_LPM_PRE_REG_125US);
	writel((unsigned int)((v2 / 100 > 1) ? v2 / 100 - 1 : 1), (void *)data->device_base
		+ D_XEC_LPM_PRE_REG_1MS);
	writel((unsigned int)((v2 / 10) > 1 ? v2 / 10 - 1 : 1), (void *)data->device_base
		+ D_XEC_LPM_PRE_REG_10MS);
	writel((unsigned int)(v2 > 1 ? v2 - 1 : 1), (void *)data->device_base
		+ D_XEC_LPM_PRE_REG_100MS);

	dev_dbg(data->dev, "[%s:%d]readl:%x, %x ,%x, %x, %x, %x, %x, %x\n", __func__, __LINE__,
		readl((void *)data->device_base + D_XEC_LPM_PRE_REG_250NS),
		readl((void *)data->device_base + D_XEC_LPM_PRE_REG_1US),
		readl((void *)data->device_base + D_XEC_LPM_PRE_REG_10US),
		readl((void *)data->device_base + D_XEC_LPM_PRE_REG_100US),
		readl((void *)data->device_base + D_XEC_LPM_PRE_REG_125US),
		readl((void *)data->device_base + D_XEC_LPM_PRE_REG_1MS),
		readl((void *)data->device_base + D_XEC_LPM_PRE_REG_10MS),
		readl((void *)data->device_base + D_XEC_LPM_PRE_REG_100MS));


	v0 = readl((void *)data->xhci_base + XEC_USBSSP_CHICKEN_BITS_3);
#ifdef CONFIG_ARCH_CIX_FPGA
	v0 &= APB_TIMEOUT_MASK;
	v0 |= APB_TIMEOUT_VALUE_50MS_FREQ_200M;
#endif
	v0 &= ~(CFG_APB_TIMEOUT_PSLVERR_EN | CFG_APB_PSLVERR_EN);
	writel(v0, (void *)data->xhci_base + XEC_USBSSP_CHICKEN_BITS_3);
	if (data->u3_disable) {
		dev_info(data->dev, "[%s:%d]disable u3 port\n", __func__, __LINE__);
		writel(XEC_CFG_3XPORT_MODE_VALUE, (void *)(data->xhci_base)
			+ XEC_CFG_3XPORT_MODE);
	} else if (data->ssp_disable) {
		dev_info(data->dev, "[%s:%d]disable ssp\n", __func__, __LINE__);
		v0 = readl((void *)(data->xhci_base) + XEC_CFG_3XPORT_MODE);
		writel(v0  & CFG_3XPORT_MODE_DIS_SSP, (void *)(data->xhci_base)
			+ XEC_CFG_3XPORT_MODE);
	}

	/* v0 = (int) (250 * data->sof_clk_freq / 1000000000) = (int) ((250/10) *
	 * (data->sof_clk_freq ) / (1000000000/10)) = (int) (25 * clk / 100000000)	250ns
	 * v1 = (int) (100 * data->sof_clk_freq / 1000000) = (int) (clk / 10000)	100us
	 * v2 = (int) (100 * data->sof_clk_freq / 1000) = (int) (clk / 10)		100ms
	 */

	clk = (int)data->sof_clk_freq;
	v0 = (int)(25*clk / 100000000);
	v1 = (int)(clk / 10000);
	v2 = (int)(clk/10);

	writel((unsigned int)((v0 > 1) ? v0 - 1 : 0), (void *)data->xhci_base
		+ XEC_PRE_REG_250NS);
	writel((unsigned int)((v1 / 100 > 1) > 0 ? v1 / 100 - 1 : 0), (void *)data->xhci_base
		+ XEC_PRE_REG_1US);
	writel((unsigned int)((v1 / 10 > 1) > 0 ? v1 / 10 - 1 : 0), (void *)data->xhci_base
		+ XEC_PRE_REG_10US);
	writel((unsigned int)((v1) > 1 ? v1 - 1 : 0), (void *)data->xhci_base
		+ XEC_PRE_REG_100US);
	writel((unsigned int)((125 * clk / 1000000) > 1 ? (125 * clk / 1000000) : 0),
		(void *)data->xhci_base + XEC_PRE_REG_125US);
	writel((unsigned int)((v2 / 100 > 1) ? v2 / 100 - 1 : 0), (void *)data->xhci_base
		+ XEC_PRE_REG_1MS);
	writel((unsigned int)((v2 / 10) > 1 ? v2 / 10 - 1 : 0), (void *)data->xhci_base
		+ XEC_PRE_REG_10MS);
	writel((unsigned int)(v2 > 1 ? v2 - 1 : 0), (void *)data->xhci_base
		+ XEC_PRE_REG_100MS);

	dev_dbg(data->dev, "[%s:%d]readl:%x, %x ,%x, %x, %x, %x, %x, %x\n", __func__, __LINE__,
		readl((void *)data->xhci_base + XEC_PRE_REG_250NS),
		readl((void *)data->xhci_base + XEC_PRE_REG_1US),
		readl((void *)data->xhci_base + XEC_PRE_REG_10US),
		readl((void *)data->xhci_base + XEC_PRE_REG_100US),
		readl((void *)data->xhci_base + XEC_PRE_REG_125US),
		readl((void *)data->xhci_base + XEC_PRE_REG_1MS),
		readl((void *)data->xhci_base + XEC_PRE_REG_10MS),
		readl((void *)data->xhci_base + XEC_PRE_REG_100MS));

	clk = (int)data->lpm_clk_freq;
	v0 = (int)(25*clk / 100000000);
	v1 = (int)(clk / 10000);
	v2 = (int)(clk/10);
	writel((unsigned int)((v0 > 1) ? v0 - 1 : 0), (void *)data->xhci_base
		+ XEC_LPM_PRE_REG_250NS);
	writel((unsigned int)((v1 / 100 > 1) > 0 ? v1 / 100 - 1 : 0), (void *)data->xhci_base
		+ XEC_LPM_PRE_REG_1US);
	writel((unsigned int)((v1 / 10 > 1) > 0 ? v1 / 10 - 1 : 0), (void *)data->xhci_base
		+ XEC_LPM_PRE_REG_10US);
	writel((unsigned int)((v1) > 1 ? v1 - 1 : 0), (void *)data->xhci_base
		+ XEC_LPM_PRE_REG_100US);
	writel((unsigned int)((125 * clk / 1000000) > 1 ? (125 * clk / 1000000) : 0),
		(void *)data->xhci_base + XEC_LPM_PRE_REG_125US);
	writel((unsigned int)((v2 / 100 > 1) ? v2 / 100 - 1 : 0), (void *)data->xhci_base
		+ XEC_LPM_PRE_REG_1MS);
	writel((unsigned int)((v2 / 10) > 1 ? v2 / 10 - 1 : 0), (void *)data->xhci_base
		+ XEC_LPM_PRE_REG_10MS);
	writel((unsigned int)(v2 > 1 ? v2 - 1 : 0), (void *)data->xhci_base
		+ XEC_LPM_PRE_REG_100MS);

	dev_dbg(data->dev, "[%s:%d]readl:%x, %x ,%x, %x, %x, %x, %x, %x\n", __func__, __LINE__,
		readl((void *)data->xhci_base + XEC_LPM_PRE_REG_250NS),
		readl((void *)data->xhci_base + XEC_LPM_PRE_REG_1US),
		readl((void *)data->xhci_base + XEC_LPM_PRE_REG_10US),
		readl((void *)data->xhci_base + XEC_LPM_PRE_REG_100US),
		readl((void *)data->xhci_base + XEC_LPM_PRE_REG_125US),
		readl((void *)data->xhci_base + XEC_LPM_PRE_REG_1MS),
		readl((void *)data->xhci_base + XEC_LPM_PRE_REG_10MS),
		readl((void *)data->xhci_base + XEC_LPM_PRE_REG_100MS));
	reset_control_deassert(data->reset);
	return 0;
}

static int cdns_sky1_platform_reset(struct device *dev)
{
	int ret;
	struct device *parent = dev->parent;
	struct cdnsp_sky1 *data = dev_get_drvdata(parent);

	if (data)
		ret = cdnsp_sky1_drd_init(data);
	return ret;
}

static int cdnsp_sky1_u3_disable(struct cdnsp_sky1 *data)
{
	dev_info(data->dev, "[%s:%d]disable u3 port\n", __func__, __LINE__);
	writel(D_XEC_CFG_3XPORT_MODE_VALUE, (void *)(data->xhci_base)
			+ XEC_CFG_3XPORT_MODE);
	return 0;
}

static int cdns_sky1_platform_u3_disable(struct device *dev)
{
	int ret;
	struct device *parent = dev->parent;
	struct cdnsp_sky1 *data = dev_get_drvdata(parent);

	if (data)
		ret = cdnsp_sky1_u3_disable(data);
	return ret;
}

static void *sky1_of_get_addr_by_name(struct device_node *parent, char *name)
{
	struct device_node *node;
	int index;

	node = of_get_next_child(parent, NULL);
	if (node) {
		index = of_property_match_string(node, "reg-names", name);
		if (index >= 0)
			return of_iomap(node, index);
	}

	return NULL;
}

static void *sky1_acpi_get_addr_by_name(struct device *dev, char *name)
{
	struct fwnode_handle *fwnode;
	struct platform_device *pdev;
	struct device *device;
	struct resource *res;

	fwnode = device_get_next_child_node(dev, NULL);
	if (!fwnode)
		return NULL;

	device = bus_find_device_by_fwnode(&platform_bus_type, fwnode);
	if (!device)
		return NULL;

	pdev = to_platform_device(device);
	if (!pdev)
		return NULL;

	res = platform_get_resource_byname(pdev, IORESOURCE_MEM, name);
	if (!res)
		return NULL;

	return ioremap(res->start, resource_size(res));
}

static void *sky1_get_addr_by_name(struct device *dev, char *name)
{
	if (!ACPI_COMPANION(dev))
		return sky1_of_get_addr_by_name(dev->of_node, name);
	else
		return sky1_acpi_get_addr_by_name(dev, name);
}

static void sky1_put_addr(void __iomem *regs)
{
	if (regs)
		iounmap(regs);
}

static struct of_dev_auxdata cdns_sky1_auxdata[] = {
	{
		.compatible = "cdns,usbssp",
	},
	{},
};

static const struct acpi_device_id cdns_sky1_sub_match[] = {
	{ "CIXH2031", },
	{},
};

static int cdnsp_sky1_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct device_node *node = dev->of_node;
	struct cdnsp_sky1 *data;
	int ret;
	struct cdns3_platform_data *cdns_sky1_pdata;

	data = devm_kzalloc(dev, sizeof(*data), GFP_KERNEL);
	if (!data)
		return -ENOMEM;

	data->axi_base = devm_platform_ioremap_resource_byname(pdev, "axi_property");
	if (IS_ERR(data->axi_base)) {
		dev_err(dev, "can't map IOMEM resource\n");
		return PTR_ERR(data->axi_base);
	}

	data->ctst_base = devm_platform_ioremap_resource_byname(pdev, "controller_status");
	if (IS_ERR(data->ctst_base)) {
		dev_err(dev, "can't map IOMEM resource\n");
		return PTR_ERR(data->ctst_base);
	}

	data->reset = devm_reset_control_get(&pdev->dev, "usb_reset");
	if (IS_ERR(data->reset)) {
		ret = PTR_ERR(data->reset);
		dev_err(dev, "[%s:%d]get reset error:%d\n", __func__, __LINE__, ret);
		return ret;
	}
	data->preset = devm_reset_control_get(&pdev->dev, "usb_preset");
	if (IS_ERR(data->preset)){
		ret = PTR_ERR(data->preset);
		dev_err(dev, "[%s:%d]get reset error:%d\n", __func__, __LINE__, ret);
		return ret;
	}

	platform_set_drvdata(pdev, data);
	data->dev = dev;

	ret = of_alias_get_id(dev->of_node, "usb");
	if (ret == -ENODEV) {
		if (device_property_read_u32(dev, "id", &ret))
			ret = -ENODEV;
	}
	if (ret < 0 || ret > 9) {
		dev_err(dev, "get alias failed.\n");
		return ret;
	}
	data->id = ret;

	data->usb_syscon = device_syscon_regmap_lookup_by_property(&pdev->dev,
							"cix,usb_syscon");

	if (IS_ERR(data->usb_syscon)) {
		dev_err(dev, "Unable to get cix,usb_syscon regmap");
		return PTR_ERR(data->usb_syscon);
	}

	data->u3_disable = device_property_read_bool(dev, "u3-port-disable");
	data->ssp_disable = device_property_read_bool(dev, "ssp-disable");

	if (!device_property_read_u32(dev, "sof_clk_freq", &ret))
		data->sof_clk_freq = ret;
	else
		data->sof_clk_freq = CIX_USB_CLK_32K;

	if (!device_property_read_u32(dev, "lpm_clk_freq", &ret))
		data->lpm_clk_freq = ret;
	else
		data->lpm_clk_freq = CIX_USB_CLK_8M;

	if (!device_property_read_u32(dev, "axi_bmax_value", &ret))
		data->axi_bmax_value = ret;
	else
		data->axi_bmax_value = AXI_BMAX_VALUE_DEFAULT;

	data->xhci_base = sky1_get_addr_by_name(dev, "xhci");
	if (!data->xhci_base)
		return PTR_ERR(data->xhci_base);

	data->device_base = sky1_get_addr_by_name(dev, "dev");
	if (!data->device_base)
		return PTR_ERR(data->device_base);

	ret = cdnsp_sky1_drd_init(data);
	if (ret == -ETIMEDOUT)
		return -EPROBE_DEFER;
	else if (ret)
		return ret;

	data->oc_gpio = devm_gpiod_get_optional(data->dev, "oc", GPIOD_IN);
	if (IS_ERR(data->oc_gpio)) {
		dev_err(data->dev, "[%s:%d] can not get oc_gpio\n", __func__, __LINE__);
		ret = PTR_ERR(data->oc_gpio);
		return ret;
	}
	if (data->oc_gpio) {
		ret = gpiod_direction_input(data->oc_gpio);
		if (ret < 0)
			dev_err(data->dev, "set oc_gpio input failed:%d\n", ret);
	}
	//release by platform_device_release
	cdns_sky1_pdata = kzalloc(sizeof(struct cdns3_platform_data), GFP_KERNEL);
	if (!cdns_sky1_pdata)
		return -ENOMEM;
	cdns_sky1_pdata->platform_reset = cdns_sky1_platform_reset;
	cdns_sky1_pdata->platform_suspend = cdns_sky1_platform_suspend;
	cdns_sky1_pdata->platform_u3_disable = cdns_sky1_platform_u3_disable;
	cdns_sky1_pdata->quirks = CDNS3_DEFAULT_PM_RUNTIME_ALLOW;
	if (!ACPI_COMPANION(dev)) {
		cdns_sky1_auxdata->platform_data = cdns_sky1_pdata;
		ret = of_platform_populate(node, NULL, cdns_sky1_auxdata, dev);
		if (ret) {
			dev_err(dev, "failed to create children: %d\n", ret);
			goto err;
		}
	} else {
		/*
		 * ACPI "populate" all device at once.
		 * Using other mechanism to ensure the driver probe sequence,
		 * like device link.
		 * Just set the platform data here.
		 */
		struct fwnode_handle *child;

		device_for_each_child_node(dev, child) {
			const struct acpi_device_id *id;
			struct device *cdev;

			cdev = bus_find_device_by_fwnode(&platform_bus_type,
						child);
			if (!cdev)
				continue;

			id = acpi_match_device(cdns_sky1_sub_match, cdev);
			if (id && !cdev->platform_data)
				cdev->platform_data = cdns_sky1_pdata;
			else
				goto err;
		}
	}

	device_set_wakeup_capable(dev, true);
	pm_runtime_set_active(dev);
	pm_runtime_enable(dev);
	return 0;
err:
	kfree(cdns_sky1_pdata);
	return ret;
}

static int cdnsp_sky1_remove(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct cdnsp_sky1 *data = dev_get_drvdata(dev);

	pm_runtime_get_sync(dev);
	of_platform_depopulate(dev);
	sky1_put_addr(data->xhci_base);
	sky1_put_addr(data->device_base);
	reset_control_deassert(data->reset);
	reset_control_deassert(data->preset);
	sky1_usb_clk_disable_all(dev);
	platform_set_drvdata(pdev, NULL);

	return 0;
}

#ifdef CONFIG_PM
/* Because the wake-up interrupt and host interrupt are the same interrupt, closing the axi
 * and sof clock will result in the inability to generate port status change interrupt.
 */

static int cdnsp_sky1_resume(struct device *dev)
{
	return 0;
}

static int cdnsp_sky1_suspend(struct device *dev)
{
	return 0;
}

static int cdnsp_sky1_system_suspend(struct device *dev)
{
	sky1_usb_clk_disable_suspend(dev);
	return 0;
}

static int cdnsp_sky1_system_resume(struct device *dev)
{
	int ret = 0;
	struct cdnsp_sky1 *data;

	ret = sky1_usb_clk_enable_resume(dev);
	data = dev_get_drvdata(dev);
	writel(CIX_USB_AXI_WR_CACHE_VALUE, data->axi_base);
	return ret;
}

static int cdnsp_sky1_restore(struct device *dev)
{
	dev_dbg(dev, "at func %s\n", __func__);
	return 0;
}

static int cdnsp_sky1_freeze(struct device *dev)
{
	dev_dbg(dev, "at func %s\n", __func__);
	return 0;
}

static int cdnsp_sky1_thaw(struct device *dev)
{
	dev_dbg(dev, "at func %s\n", __func__);
	return 0;
}
#endif /* CONFIG_PM */

static const struct dev_pm_ops cdnsp_sky1_pm_ops = {
        .suspend = cdnsp_sky1_system_suspend,
        .resume = cdnsp_sky1_system_resume,
        .restore = cdnsp_sky1_restore,
        .freeze = cdnsp_sky1_freeze,
        .thaw = cdnsp_sky1_thaw,
	.runtime_resume = cdnsp_sky1_resume,
	.runtime_suspend = cdnsp_sky1_suspend,
};

static const struct of_device_id cdns_sky1_of_match[] = {
	{ .compatible = "cix,sky1-usbssp", },
	{},
};
MODULE_DEVICE_TABLE(of, cdns_sky1_of_match);

static const struct acpi_device_id cdnsp_sky1_acpi_match[] = {
	{ "CIXH2030" },
	{},
};
MODULE_DEVICE_TABLE(acpi, cdnsp_sky1_acpi_match);

static void cdnsp_sky1_shutdown(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct cdnsp_sky1 *data = dev_get_drvdata(dev);

	if (!device_may_wakeup(dev)) {
		dev_dbg(dev, "at %s, reset controller\n", __func__);
		reset_control_assert(data->reset);
		reset_control_assert(data->preset);
		sky1_usb_clk_disable_all(dev);
	}
}

static struct platform_driver cdnsp_sky1_driver = {
	.probe		= cdnsp_sky1_probe,
	.remove		= cdnsp_sky1_remove,
	.driver		= {
		.name	= "cdnsp-sky1",
		.of_match_table	= cdns_sky1_of_match,
		.acpi_match_table = ACPI_PTR(cdnsp_sky1_acpi_match),
		.pm	= &cdnsp_sky1_pm_ops,
	},
	.shutdown = cdnsp_sky1_shutdown,
};

module_platform_driver(cdnsp_sky1_driver);

MODULE_ALIAS("platform: cdnsp-sky1");
MODULE_AUTHOR("Chao Zeng <Chao.Zeng@cixtech.com>");
MODULE_AUTHOR("Matthew MA <Matthew.Ma@cixtech.com>");
MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("Cdnsp Sky1 Glue Layer");
