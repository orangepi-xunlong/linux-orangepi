/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
* Allwinner ephy driver.
*
* Copyright(c) 2022-2027 Allwinnertech Co., Ltd.
*
* This file is licensed under the terms of the GNU General Public
* License version 2.  This program is licensed "as is" without any
* warranty of any kind, whether express or implied.
*/

/* #define DEBUG */
#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/errno.h>
#include <linux/unistd.h>
#include <linux/interrupt.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/of.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/skbuff.h>
#include <linux/spinlock.h>
#include <linux/mm.h>
#include <linux/module.h>
#include <linux/mii.h>
#include <linux/ethtool.h>
#include <linux/phy.h>
#include <linux/platform_device.h>
#include <sunxi-sid.h>
#include <asm/io.h>
#include <asm/irq.h>

#define AC300_EPHY		"ac300-ephy"
#define AC300_DEV		"ac300"

#define EPHY_CALI_BASE		0

#if IS_ENABLED(CONFIG_ARCH_SUN300IW1P1)
#define EPHY_CALI_BIT		BIT(21)
#define CALI_FLAG_BIT		BIT(20)
#define EPHY_BGS_MASK		0x000f0000
#define EPHY_BGS_OFFSET		16
#else
#define EPHY_CALI_BIT		BIT(29)
#define EPHY_BGS_MASK		0x0f000000
#define EPHY_BGS_OFFSET		24
#endif

/*
 * Ephy diagram test
 * This macro will cause all cpu stuck
 * Use it carefully
 */
/* #define EPHY_100M_ED_TEST */

struct ephy_res {
	struct phy_device *ac300;
	spinlock_t lock;
	atomic_t ephy_en;
	bool leds_active_low;
};

static struct ephy_res ac300_ephy;

static int sunxi_ephy_read_sid(u32 *buf)
{
	int ret;

	if (!buf)
		return -EINVAL;

	ret = sunxi_efuse_readn(EFUSE_FTCP_NAME, buf, 4);
	if (ret)
		return ret;

	return 0;
}

void sunxi_ephy_config_new_init(struct phy_device *phydev)
{
	phy_write(phydev, 0x1f, 0x0100);	/* switch to Page 1 */
	phy_write(phydev, 0x12, 0x4824);	/* Disable APS */

	phy_write(phydev, 0x1f, 0x0200);	/* switch to Page 2 */
	phy_write(phydev, 0x18, 0x0000);	/* PHYAFE TRX optimization */

	phy_write(phydev, 0x1f, 0x0600);	/* switch to Page 6 */
	phy_write(phydev, 0x14, 0x7809);	/* PHYAFE TX optimization */
	phy_write(phydev, 0x13, 0xf000);	/* PHYAFE RX optimization */
	phy_write(phydev, 0x10, 0x5523);
	phy_write(phydev, 0x15, 0x3533);

	phy_write(phydev, 0x1f, 0x0800);	/* switch to Page 8 */
	phy_write(phydev, 0x1d, 0x0844);	/* disable auto offset */
	phy_write(phydev, 0x18, 0x00bc);	/* PHYAFE TRX optimization */

	phy_write(phydev, 0x1f, 0x0000);	/* switch to Page 0 */
}

void sunxi_ephy_config_old_init(struct phy_device *phydev)
{
	phy_write(phydev, 0x1f, 0x0100);	/* switch to Page 1 */
	phy_write(phydev, 0x12, 0x4824);	/* Disable APS */

	phy_write(phydev, 0x1f, 0x0200);	/* switch to Page 2 */
	phy_write(phydev, 0x18, 0x0000);	/* PHYAFE TRX optimization */

	phy_write(phydev, 0x1f, 0x0600);	/* switch to Page 6 */
	phy_write(phydev, 0x14, 0x780b);	/* PHYAFE TX optimization */
	phy_write(phydev, 0x13, 0xf000);	/* PHYAFE RX optimization */
	phy_write(phydev, 0x15, 0x1530);
	phy_write(phydev, 0x1f, 0x0800);	/* switch to Page 8 */
	phy_write(phydev, 0x18, 0x00bc);	/* PHYAFE TRX optimization */

	phy_write(phydev, 0x1f, 0x0000);	/* switch to Page 0 */
}

void sunxi_ephy_config_cali(struct phy_device *phydev, u32 ephy_cali)
{
	int value, bgs_adjust;

	/* Adjust BGS value of 0x06 reg */
	value = phy_read(phydev, 0x06);
	value &= ~(0x0F << 12);
	bgs_adjust = (ephy_cali & EPHY_BGS_MASK) >> EPHY_BGS_OFFSET;
	value |= (0xF & (EPHY_CALI_BASE + bgs_adjust)) << 12;
	phy_write(phydev, 0x06, value);
}

void sunxi_ephy_disable_intelligent_ieee(struct phy_device *phydev)
{
	unsigned int value;

	phy_write(phydev, 0x1f, 0x0100);	/* switch to page 1 */
	value = phy_read(phydev, 0x17);		/* read address 0 0x17 register */
	value &= ~(1 << 3);			/* reg 0x17 bit 3, set 0 to disable IEEE */
	phy_write(phydev, 0x17, value);
	phy_write(phydev, 0x1f, 0x0000);	/* switch to page 0 */
}

void sunxi_ephy_disable_802_3az_ieee(struct phy_device *phydev)
{
	unsigned int value;

	phy_write(phydev, 0xd, 0x7);
	phy_write(phydev, 0xe, 0x3c);
	phy_write(phydev, 0xd, 0x1 << 14 | 0x7);
	value = phy_read(phydev, 0xe);
	value &= ~(0x1 << 1);
	phy_write(phydev, 0xd, 0x7);
	phy_write(phydev, 0xe, 0x3c);
	phy_write(phydev, 0xd, 0x1 << 14 | 0x7);
	phy_write(phydev, 0xe, value);

	phy_write(phydev, 0x1f, 0x0200);	/* switch to page 2 */
	phy_write(phydev, 0x18, 0x0000);
}

#ifdef EPHY_100M_ED_TEST
static void ephy_debug_test(void *info)
{
	while (1)
		;
}
#endif

static int ephy_config_init(struct phy_device *phydev)
{
	int value;
	int ret;
	u32 ephy_cali = 0;

	ret = sunxi_ephy_read_sid(&ephy_cali);
	if (ret) {
		pr_err("ephy cali efuse read fail, use default 0\n");
	}

#ifdef CALI_FLAG_BIT
	if (ephy_cali & CALI_FLAG_BIT) {
#endif
		sunxi_ephy_config_cali(ac300_ephy.ac300, ephy_cali);

		/*
		 * EPHY_CALI_BIT: the flag of calibration value
		 * 0: Normal
		 * 1: Low level of calibration value
		 */
		if (ephy_cali & EPHY_CALI_BIT) {
			pr_debug("Low level ephy, use new init\n");
			sunxi_ephy_config_new_init(phydev);
		} else {
			pr_debug("Normal ephy, use old init\n");
			sunxi_ephy_config_old_init(phydev);
		}

#ifdef CALI_FLAG_BIT
	} else {
		pr_err("this phy is not Calibrated\n");
		sunxi_ephy_config_old_init(phydev);
	}
#endif

	sunxi_ephy_disable_intelligent_ieee(phydev);	/* Disable Intelligent IEEE */
	sunxi_ephy_disable_802_3az_ieee(phydev);	/* Disable 802.3az IEEE */
	phy_write(phydev, 0x1f, 0x0000);		/* Switch to Page 0 */

#ifdef EPHY_100M_ED_TEST
	phy_write(phydev, 0x1f, 0x0000);		/* Switch to Page 0 */
	phy_write(phydev, 0x00, 0x2100);		/* Force 100M Mode */
	phy_write(phydev, 0x1f, 0x0000);		/* Switch to Page 0 */
	phy_write(phydev, 0x13, 0x0100);		/* Force TX output@TXP/TXN */
	on_each_cpu(ephy_debug_test, NULL, 1);		/* Stuck all cpu for ephy eye diagram test */
#endif

	value = phy_read(ac300_ephy.ac300, 0x06);
	if (phydev->interface == PHY_INTERFACE_MODE_RMII)
		value |= (1 << 11);
	else
		value &= (~(1 << 11));

	phy_write(ac300_ephy.ac300, 0x06, value);

	return 0;
}

static int ephy_probe(struct phy_device *phydev)
{
	return 0;
}

static int ephy_suspend(struct phy_device *phydev)
{
	return genphy_suspend(phydev);
}

static int ephy_resume(struct phy_device *phydev)
{
	return genphy_resume(phydev);
}

static void ac300_enable(struct phy_device *phydev)
{
	/* release reset */
	phy_write(phydev, 0x00, 0x1f40); /* reset ephy */
	phy_write(phydev, 0x00, 0x1f43); /* de-reset ephy */

	/* clk gating */
	phy_write(phydev, 0x00, 0x1fb7);

	/* io enable */
	phy_write(phydev, 0x05, 0xa81f);

	mdelay(10);
	phy_write(phydev, 0x06, 0x0811);

	mdelay(10);
	if (ac300_ephy.leds_active_low)
		phy_write(phydev, 0x06, 0x0812);
	else
		phy_write(phydev, 0x06, 0x0810);
}

static void ac300_disable(struct phy_device *phydev)
{
	phy_write(phydev, 0x00, 0x1f40);
	phy_write(phydev, 0x05, 0xa800);

	phy_write(phydev, 0x06, 0x01);
}

static int ac300_suspend(struct phy_device *phydev)
{
	ac300_disable(phydev);
	return 0;
}

static int ac300_resume(struct phy_device *phydev)
{
	return 0;
}

static int ac300_probe(struct phy_device *phydev)
{
	struct device *dev = &phydev->mdio.dev;

	/* save ac300 message */
	ac300_ephy.ac300 = phydev;
	ac300_ephy.leds_active_low = of_property_read_bool(dev->of_node, "allwinner,leds-active-low");

	ac300_enable(phydev);

	return 0;
}

static int ac300_init(struct phy_device *phydev)
{
	/* ac300 enable */
	ac300_enable(phydev);

	/*  FIXME: delay may be required after AC300 reset*/
	msleep(50);

	return 0;
}

static struct phy_driver ac300_driver[] = {
{
	.phy_id		= 0xc0000000,
	.name		= AC300_DEV,
	.phy_id_mask	= 0xffffffff,
	.config_init	= ac300_init,
	.suspend	= ac300_suspend,
	.resume		= ac300_resume,
	.probe		= ac300_probe,
},
{	.phy_id		= 0x00441400,
	.name		= AC300_EPHY,
	.phy_id_mask	= 0x0ffffff0,
	.config_init	= ephy_config_init,
	.config_aneg	= &genphy_config_aneg,
	.read_status	= &genphy_read_status,
	.suspend	= ephy_suspend,
	.resume		= ephy_resume,
	.probe		= ephy_probe,

} };

module_phy_driver(ac300_driver);

static struct mdio_device_id __maybe_unused ac300_tbl[] = {
	{ 0xc0000000, 0x0fffffff },
	{ 0x00441400, 0x0ffffff0 },
	{ }
};

MODULE_DEVICE_TABLE(mdio, ac300_tbl);
MODULE_DESCRIPTION("Allwinner phy drivers");
MODULE_AUTHOR("xuminghui <xuminghui@allwinnertech.com>");
MODULE_LICENSE("GPL");
MODULE_VERSION("1.1.2");
