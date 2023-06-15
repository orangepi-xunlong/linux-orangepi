/*
 * Copyright © 2015-2016, Shuge
 *		Author: Sugar <shugeLinux@gmail.com>
 *
 * This file is provided under a dual BSD/GPL license.  When using or
 * redistributing this file, you may do so under either license.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.
 *
 */
//#define DEBUG
#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/errno.h>
#include <linux/unistd.h>
#include <linux/interrupt.h>
#include <linux/init.h>
#include <linux/delay.h>
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

#include <asm/io.h>
#include <asm/irq.h>

#include <linux/mfd/acx00-mfd.h>
#include <linux/sunxi-sid.h>

#define EXTEPHY_CTRL0 0x0014
#define EXTEPHY_CTRL1 0x0016

#define EPHY_CTRL 0x6000
#define EPHY_SID 0x8004

#define EPHY_ACX00 1
#define EPHY_AC300 2

#define WAIT_MAX_COUNT 200


struct ephy_res {
	struct phy_device *ac300;
	struct acx00 *acx;
	atomic_t ephy_en;
};

static struct ephy_res acx00_ephy;
static struct ephy_res ac300_ephy;
u8 ephy_type;


int ephy_is_enable(void)
{
	if (ephy_type == EPHY_ACX00)
		return atomic_read(&acx00_ephy.ephy_en);
	else if (ephy_type == EPHY_AC300)
		return atomic_read(&ac300_ephy.ephy_en);
	return 0;
}
EXPORT_SYMBOL_GPL(ephy_is_enable);

/**
 * @name	ephy_read_sid
 * @brief		read ephy sid from efuse
 * @param[IN]	none
 * @param[OUT]	p_ephy_cali: ephy calibration value
 * @return	return 0 if success,-value if fail
 */
static __attribute__((unused)) s32 ephy_read_sid(u16 *p_ephy_cali)
{
	s32 ret = 0;
	u8 buf[6];

	if (!p_ephy_cali) {
		pr_info("%s's pointer type args are NULL!\n", __func__);
		return -1;
	}
	ret = sunxi_efuse_readn(EFUSE_OEM_NAME, buf, 6);
	if (ret != 0) {
		pr_info("sunxi_efuse_readn failed:%d\n", ret);
		return ret;
	}
	*p_ephy_cali = buf[0] + (buf[1] << 8);

	return ret;
}

#if 0
static int ephy_reset(struct phy_device *phydev)
{
	int bmcr;

	/* Software Reset PHY */
	bmcr = phy_read(phydev, MII_BMCR);
	if (bmcr < 0)
		return bmcr;

	bmcr |= BMCR_RESET;
	bmcr = phy_write(phydev, MII_BMCR, bmcr);
	if (bmcr < 0)
		return bmcr;

	do {
		bmcr = phy_read(phydev, MII_BMCR);
		if (bmcr < 0)
			return bmcr;
	} while (bmcr & BMCR_RESET);

	return 0;
}
#endif

static void disable_intelligent_ieee(struct phy_device *phydev)
{
	unsigned int value;

	phy_write(phydev, 0x1f, 0x0100);	/* switch to page 1 */
	value = phy_read(phydev, 0x17);		/* read address 0 0x17 register */
	value &= ~(1 << 3);			/* reg 0x17 bit 3, set 0 to disable IEEE */
	phy_write(phydev, 0x17, value);
	phy_write(phydev, 0x1f, 0x0000);	/* switch to page 0 */
}

static void disable_802_3az_ieee(struct phy_device *phydev)
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

static void ephy_config_default(struct phy_device *phydev)
{
	phy_write(phydev, 0x1f, 0x0100);	/* Switch to Page 1 */
	phy_write(phydev, 0x12, 0x4824);	/* Disable APS */

	phy_write(phydev, 0x1f, 0x0200);	/* Switch to Page 2 */
	phy_write(phydev, 0x18, 0x0000);	/* PHYAFE TRX optimization */

	phy_write(phydev, 0x1f, 0x0600);	/* Switch to Page 6 */
	phy_write(phydev, 0x14, 0x708b);	/* PHYAFE TX optimization */
	phy_write(phydev, 0x13, 0xF000);	/* PHYAFE RX optimization */
	phy_write(phydev, 0x15, 0x1530);

	phy_write(phydev, 0x1f, 0x0800);	/* Switch to Page 6 */
	phy_write(phydev, 0x18, 0x00bc);	/* PHYAFE TRX optimization */
}

static void __maybe_unused ephy_config_fixed(struct phy_device *phydev)
{
	phy_write(phydev, 0x1f, 0x0100);	/*switch to Page 1 */
	phy_write(phydev, 0x12, 0x4824);	/*Disable APS */

	phy_write(phydev, 0x1f, 0x0200);	/*switch to Page 2 */
	phy_write(phydev, 0x18, 0x0000);	/*PHYAFE TRX optimization */

	phy_write(phydev, 0x1f, 0x0600);	/*switch to Page 6 */
	phy_write(phydev, 0x14, 0x7809);	/*PHYAFE TX optimization */
	phy_write(phydev, 0x13, 0xf000);	/*PHYAFE RX optimization */
	phy_write(phydev, 0x10, 0x5523);
	phy_write(phydev, 0x15, 0x3533);

	phy_write(phydev, 0x1f, 0x0800);	/*switch to Page 8 */
	phy_write(phydev, 0x1d, 0x0844);	/*disable auto offset */
	phy_write(phydev, 0x18, 0x00bc);	/*PHYAFE TRX optimization */
}

static void __maybe_unused ephy_config_cali(struct phy_device *phydev,
					    u16 ephy_cali)
{
	int value;
	value = phy_read(phydev, 0x06);
	value &= ~(0x0F << 12);
	value |= (0x0F & (0x03 + ephy_cali)) << 12;
	phy_write(phydev, 0x06, value);

	return;
}

static int ephy_config_init(struct phy_device *phydev)
{
	int value;
#if defined(CONFIG_ARCH_SUN50IW9)
	if (ephy_type == EPHY_AC300) {
		int ret;
		u16 ephy_cali = 0;
		ret = ephy_read_sid(&ephy_cali);
		if (ret) {
			pr_err("ephy cali efuse read fail!\n");
			return -1;
		}
		ephy_config_cali(ac300_ephy.ac300, ephy_cali);
		/*
		 * BIT9: the flag of calibration value
		 * 0: Normal
		 * 1: Low level of calibration value
		 */
		if (ephy_cali & BIT(9)) {
			pr_debug("ac300:ephy cali efuse read: fixed!\n");
			ephy_config_fixed(phydev);
		} else {
			pr_debug("ac300:ephy cali efuse read: default!\n");
			ephy_config_default(phydev);
		}
	} else {
		pr_debug("ac200:ephy cali efuse read: default!\n");
		ephy_config_default(phydev);
	}
#else
	ephy_config_default(phydev);
#endif

#if 0
	/* Disable Auto Power Saving mode */
	phy_write(phydev, 0x1f, 0x0100);	/* Switch to Page 1 */
	value = phy_read(phydev, 0x17);
	value &= ~BIT(13);
	phy_write(phydev, 0x17, value);
#endif
	disable_intelligent_ieee(phydev);	/* Disable Intelligent IEEE */
	disable_802_3az_ieee(phydev);		/* Disable 802.3az IEEE */
	phy_write(phydev, 0x1f, 0x0000);	/* Switch to Page 0 */

#ifdef CONFIG_MFD_ACX00
	if (ephy_type == EPHY_ACX00) {
		value = acx00_reg_read(acx00_ephy.acx, EPHY_CTRL);
		if (phydev->interface == PHY_INTERFACE_MODE_RMII)
			value |= (1 << 11);
		else
			value &= (~(1 << 11));
		acx00_reg_write(acx00_ephy.acx, EPHY_CTRL, value | (1 << 11));
	} else
#endif
	if (ephy_type == EPHY_AC300) {
		value = phy_read(ac300_ephy.ac300, 0x06);
		if (phydev->interface == PHY_INTERFACE_MODE_RMII)
			value |= (1 << 11);
		else
			value &= (~(1 << 11));
		phy_write(ac300_ephy.ac300, 0x06, value);
	}

#if defined(CONFIG_ARCH_SUN50IW6)
	value = phy_read(phydev, 0x13);
	value |= 1 << 12;
	phy_write(phydev, 0x13, value);
#endif

	return 0;
}

static int ephy_probe(struct phy_device *phydev)
{
	return 0;
}

#if 0
static int ephy_ack_interrupt(struct phy_device *phydev)
{
	int err = phy_read(phydev, IP101A_G_IRQ_CONF_STATUS);

	if (err < 0)
		return err;

	return 0;
}
#endif



static struct phy_driver sunxi_phy_driver = {
	.phy_id		= 0x00441400,
	.name		= "ephy",
	.phy_id_mask	= 0x0ffffff0,
#if 0
	.features	= PHY_BASIC_FEATURES | SUPPORTED_Pause |
			  SUPPORTED_Asym_Pause,
	.flags		= PHY_HAS_INTERRUPT,
	.ack_interrupt	= ephy_ack_interrupt,
#endif
	.config_init	= &ephy_config_init,
	.config_aneg	= &genphy_config_aneg,
	.read_status	= &genphy_read_status,
	.suspend	= genphy_suspend,
	.resume		= genphy_resume,
	.probe		= ephy_probe,
};

static void ac300_ephy_enable(struct ephy_res *priv)
{
	/* release reset */
	phy_write(priv->ac300, 0x00, 0x1f83);
	/* clk gating */
	phy_write(priv->ac300, 0x00, 0x1fb7);

	phy_write(priv->ac300, 0x05, 0xa81f);
	phy_write(priv->ac300, 0x06, 0);

	msleep(1000);  /* FIXME: fix some board compatible issues. */

	atomic_set(&ac300_ephy.ephy_en, 1);
}

static void ac300_ephy_disable(struct ephy_res *priv)
{
//	phy_write(priv->ac300, 0x00, 0x1f40);
//	phy_write(priv->ac300, 0x05, 0xa800);
//	phy_write(priv->ac300, 0x06, 0x01);
}

static int ac300_ephy_probe(struct phy_device *phydev)
{
	ac300_ephy.ac300 = phydev;

	atomic_set(&ac300_ephy.ephy_en, 0);
	ac300_ephy_enable(&ac300_ephy);

	ephy_type = EPHY_AC300;

	return 0;
}

static int ac300_ephy_suspend(struct phy_device *phydev)
{
	ac300_ephy_disable(&ac300_ephy);

	return 0;
}

static int ac300_ephy_resume(struct phy_device *phydev)
{
	ac300_ephy_enable(&ac300_ephy);

	return 0;
}

static struct phy_driver ac300_ephy_driver = {
	.phy_id		= 0xc0000000,
	.name		= "ac300",
	.phy_id_mask	= 0xffffffff,
	.suspend	= ac300_ephy_suspend,
	.resume		= ac300_ephy_resume,
	.probe		= ac300_ephy_probe,
};

static void acx00_ephy_enble(struct ephy_res *priv)
{
#ifdef CONFIG_MFD_ACX00
	int value;
	unsigned char i = 0;
#if defined(CONFIG_ARCH_SUN50IW6) || defined(CONFIG_ARCH_SUN50IW9)
	u16 ephy_cali = 0;
#endif

	if (!acx00_enable()) {
		for (i = 0; i < WAIT_MAX_COUNT; i++) {
			msleep(10);
			if (acx00_enable())
				break;
		}
		if (i == WAIT_MAX_COUNT) {
			pr_err("acx00 is no enable, and acx00_ephy_enble is fail\n");
			return;
		}
	}

	value = acx00_reg_read(priv->acx, EXTEPHY_CTRL0);
	value |= 0x03;
	acx00_reg_write(priv->acx, EXTEPHY_CTRL0, value);
	value = acx00_reg_read(priv->acx, EXTEPHY_CTRL1);
#if defined(CONFIG_ARCH_SUN50IW9)
	/* disable link led to avoid conflict with twi2 */
	value |= 0x09;
#else
	value |= 0x0f;
#endif
	acx00_reg_write(priv->acx, EXTEPHY_CTRL1, value);
	acx00_reg_write(priv->acx, EPHY_CTRL, 0x06);

	/*for ephy */
	value = acx00_reg_read(priv->acx, EPHY_CTRL);
	value &= ~(0xf << 12);

#if defined(CONFIG_ARCH_SUN50IW6) || defined(CONFIG_ARCH_SUN50IW9)
	ephy_read_sid(&ephy_cali);
	value |= (0x0F & (0x03 + ephy_cali)) << 12;
#else
	value |= (0x0F & (0x03 + acx00_reg_read(priv->acx, EPHY_SID))) << 12;
#endif

	acx00_reg_write(priv->acx, EPHY_CTRL, value);

	atomic_set(&acx00_ephy.ephy_en, 1);
#endif
}

static void acx00_ephy_disable(struct ephy_res *priv)
{
	int value;

	/* reset ephy */
	value = acx00_reg_read(priv->acx, EXTEPHY_CTRL0);
	value &= ~0x01;
	acx00_reg_write(priv->acx, EXTEPHY_CTRL0, value);

	/* shutdown ephy */
	value = acx00_reg_read(priv->acx, EPHY_CTRL);
	value |= 0x01;
	acx00_reg_write(priv->acx, EPHY_CTRL, value);

	atomic_set(&priv->ephy_en, 0);
}

static const struct platform_device_id acx00_ephy_id[] = {
	{ "acx-ephy", 0},
	{ },
};
MODULE_DEVICE_TABLE(platform, acx00_ephy_id);

static int acx00_ephy_probe(struct platform_device *pdev)
{
	struct acx00 *ax = dev_get_drvdata(pdev->dev.parent);

	if (!ax)
		return -ENODEV;

	acx00_ephy.acx = ax;
	ephy_type = EPHY_ACX00;
	platform_set_drvdata(pdev, &acx00_ephy);

	atomic_set(&acx00_ephy.ephy_en, 0);
	acx00_ephy_enble(&acx00_ephy);

	return 0;
}

static int acx00_ephy_remove(struct platform_device *pdev)
{
	acx00_ephy_disable(&acx00_ephy);

	return 0;
}

static int acx00_ephy_suspend(struct device *dev)
{
	acx00_ephy_disable(&acx00_ephy);

	return 0;
}

static int acx00_ephy_resume(struct device *dev)
{
	acx00_ephy_enble(&acx00_ephy);

	return 0;
}

/* Suspend hook structures */
static const struct dev_pm_ops acx00_ephy_pm_ops = {
	.suspend = acx00_ephy_suspend,
	.resume = acx00_ephy_resume,
};

static struct platform_driver acx00_ephy_driver = {
	.driver = {
		.name = "acx-ephy",
		.owner = THIS_MODULE,
		.pm = &acx00_ephy_pm_ops,
	},
	.probe = acx00_ephy_probe,
	.remove = acx00_ephy_remove,
	.id_table = acx00_ephy_id,
};

static int ephy_init(void)
{
	int ret = 0;

	ret = platform_driver_register(&acx00_ephy_driver);
	if (ret)
		return ret;

	ret = phy_driver_register(&ac300_ephy_driver, THIS_MODULE);
	if (ret)
		goto ac300_ephy_error;

	ret = phy_driver_register(&sunxi_phy_driver, THIS_MODULE);
	if (ret)
		goto ephy_driver_error;

	return ret;

ephy_driver_error:
	phy_driver_unregister(&ac300_ephy_driver);

ac300_ephy_error:
	platform_driver_unregister(&acx00_ephy_driver);

	return ret;
}

static void ephy_exit(void)
{
	phy_driver_unregister(&sunxi_phy_driver);
	phy_driver_unregister(&ac300_ephy_driver);
	platform_driver_unregister(&acx00_ephy_driver);
}

module_init(ephy_init);
module_exit(ephy_exit);

static struct mdio_device_id __maybe_unused ephy_tbl[] = {
	{ 0x00441400, 0x0ffffff0 },
	{ }
};

MODULE_DEVICE_TABLE(mdio, ephy_tbl);

MODULE_DESCRIPTION("Allwinner EPHY drivers");
MODULE_AUTHOR("Sugar <shugeLinux@gmail.com>");
MODULE_VERSION("1.0.0");
MODULE_LICENSE("GPL");
