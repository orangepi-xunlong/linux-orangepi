/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * Allwinner EMAC Fast Ethernet driver for Linux.
 *
 * Copyright 2012-2013 Stefan Roese <sr@denx.de>
 * Copyright 2013 Maxime Ripard <maxime.ripard@free-electrons.com>
 *
 * Based on the Linux driver provided by Allwinner:
 * Copyright (C) 1997  Sten Wang
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2. This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

/* #define DEBUG */
#include <linux/clk.h>
#include <linux/etherdevice.h>
#include <linux/ethtool.h>
#include <linux/gpio.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/mii.h>
#include <linux/module.h>
#include <linux/netdevice.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>
#include <linux/of_mdio.h>
#include <linux/of_net.h>
#include <linux/of_platform.h>
#include <linux/platform_device.h>
#include <linux/phy.h>
#include <linux/of_gpio.h>
#include <linux/io.h>
#include <linux/regulator/consumer.h>
#include <linux/soc/sunxi/sunxi_sram.h>
#include <linux/reset.h>

#define DRV_NAME		"sun4i-emac"
#define DRV_VERSION		"1.0.3"

#define EMAC_MAX_FRAME_LEN	0x0600

#define EMAC_POWER_CHAN_NUM	3
#define EMAC_POWER_CHAR_LENGTH	20

#define EMAC_CTL_REG		(0x00)
#define EMAC_CTL_RESET			(1 << 0)
#define EMAC_CTL_TX_EN			(1 << 1)
#define EMAC_CTL_RX_EN			(1 << 2)
#define EMAC_TX_MODE_REG	(0x04)
#define EMAC_TX_MODE_ABORTED_FRAME_EN	(1 << 0)
#define EMAC_TX_MODE_DMA_EN		(1 << 1)
#define EMAC_TX_FLOW_REG	(0x08)
#define EMAC_TX_CTL0_REG	(0x0c)
#define EMAC_TX_CTL1_REG	(0x10)
#define EMAC_TX_INS_REG		(0x14)
#define EMAC_TX_PL0_REG		(0x18)
#define EMAC_TX_PL1_REG		(0x1c)
#define EMAC_TX_STA_REG		(0x20)
#define EMAC_TX_IO_DATA_REG	(0x24)
#define EMAC_TX_IO_DATA1_REG	(0x28)
#define EMAC_TX_TSVL0_REG	(0x2c)
#define EMAC_TX_TSVH0_REG	(0x30)
#define EMAC_TX_TSVL1_REG	(0x34)
#define EMAC_TX_TSVH1_REG	(0x38)
#define EMAC_RX_CTL_REG		(0x3c)
#define EMAC_RX_CTL_AUTO_DRQ_EN		(1 << 1)
#define EMAC_RX_CTL_DMA_EN		(1 << 2)
#define EMAC_RX_CTL_PASS_ALL_EN		(1 << 4)
#define EMAC_RX_CTL_PASS_CTL_EN		(1 << 5)
#define EMAC_RX_CTL_PASS_CRC_ERR_EN	(1 << 6)
#define EMAC_RX_CTL_PASS_LEN_ERR_EN	(1 << 7)
#define EMAC_RX_CTL_PASS_LEN_OOR_EN	(1 << 8)
#define EMAC_RX_CTL_ACCEPT_UNICAST_EN	(1 << 16)
#define EMAC_RX_CTL_DA_FILTER_EN	(1 << 17)
#define EMAC_RX_CTL_ACCEPT_MULTICAST_EN	(1 << 20)
#define EMAC_RX_CTL_HASH_FILTER_EN	(1 << 21)
#define EMAC_RX_CTL_ACCEPT_BROADCAST_EN	(1 << 22)
#define EMAC_RX_CTL_SA_FILTER_EN	(1 << 24)
#define EMAC_RX_CTL_SA_FILTER_INVERT_EN	(1 << 25)
#define EMAC_RX_HASH0_REG	(0x40)
#define EMAC_RX_HASH1_REG	(0x44)
#define EMAC_RX_STA_REG		(0x48)
#define EMAC_RX_IO_DATA_REG	(0x4c)
#define EMAC_RX_IO_DATA_LEN(x)		(x & 0xffff)
#define EMAC_RX_IO_DATA_STATUS(x)	((x >> 16) & 0xffff)
#define EMAC_RX_IO_DATA_STATUS_CRC_ERR	(1 << 4)
#define EMAC_RX_IO_DATA_STATUS_LEN_ERR	(3 << 5)
#define EMAC_RX_IO_DATA_STATUS_OK	(1 << 7)
#define EMAC_RX_FBC_REG		(0x50)
#define EMAC_INT_CTL_REG	(0x54)
#define EMAC_INT_STA_REG	(0x58)
#define EMAC_MAC_CTL0_REG	(0x5c)
#define EMAC_MAC_CTL0_RX_FLOW_CTL_EN	(1 << 2)
#define EMAC_MAC_CTL0_TX_FLOW_CTL_EN	(1 << 3)
#define EMAC_MAC_CTL0_SOFT_RESET	(1 << 15)
#define EMAC_MAC_CTL1_REG	(0x60)
#define EMAC_MAC_CTL1_DUPLEX_EN		(1 << 0)
#define EMAC_MAC_CTL1_LEN_CHECK_EN	(1 << 1)
#define EMAC_MAC_CTL1_HUGE_FRAME_EN	(1 << 2)
#define EMAC_MAC_CTL1_DELAYED_CRC_EN	(1 << 3)
#define EMAC_MAC_CTL1_CRC_EN		(1 << 4)
#define EMAC_MAC_CTL1_PAD_EN		(1 << 5)
#define EMAC_MAC_CTL1_PAD_CRC_EN	(1 << 6)
#define EMAC_MAC_CTL1_AD_SHORT_FRAME_EN	(1 << 7)
#define EMAC_MAC_CTL1_BACKOFF_DIS	(1 << 12)
#define EMAC_MAC_IPGT_REG	(0x64)
#define EMAC_MAC_IPGT_HALF_DUPLEX	(0x12)
#define EMAC_MAC_IPGT_FULL_DUPLEX	(0x15)
#define EMAC_MAC_IPGR_REG	(0x68)
#define EMAC_MAC_IPGR_IPG1		(0x0c)
#define EMAC_MAC_IPGR_IPG2		(0x12)
#define EMAC_MAC_CLRT_REG	(0x6c)
#define EMAC_MAC_CLRT_COLLISION_WINDOW	(0x37)
#define EMAC_MAC_CLRT_RM		(0x0f)
#define EMAC_MAC_MAXF_REG	(0x70)
#define EMAC_MAC_SUPP_REG	(0x74)
#define EMAC_MAC_TEST_REG	(0x78)
#define EMAC_MAC_MCFG_REG	(0x7c)
#define EMAC_MAC_A0_REG		(0x98)
#define EMAC_MAC_A1_REG		(0x9c)
#define EMAC_MAC_A2_REG		(0xa0)
#define EMAC_SAFX_L_REG0	(0xa4)
#define EMAC_SAFX_H_REG0	(0xa8)
#define EMAC_SAFX_L_REG1	(0xac)
#define EMAC_SAFX_H_REG1	(0xb0)
#define EMAC_SAFX_L_REG2	(0xb4)
#define EMAC_SAFX_H_REG2	(0xb8)
#define EMAC_SAFX_L_REG3	(0xbc)
#define EMAC_SAFX_H_REG3	(0xc0)

#define EMAC_PHY_DUPLEX		(1 << 8)

#define EMAC_EEPROM_MAGIC	(0x444d394b)
#define EMAC_UNDOCUMENTED_MAGIC	(0x0143414d)

#define EMAC_SRAM_REMAP_MASK	(0xffffffef)

/********************************************************/
#define EMAC_DEFAULT_MSG_ENABLE 0x0000
static int debug = -1;     /* defaults above */;
module_param(debug, int, 0);
MODULE_PARM_DESC(debug, "debug message flags");

/* Transmit timeout, default 5 seconds. */
static int watchdog = 5000;
module_param(watchdog, int, 0400);
MODULE_PARM_DESC(watchdog, "transmit timeout in milliseconds");

/* EMAC register address locking.
 *
 * The EMAC uses an address register to control where data written
 * to the data register goes. This means that the address register
 * must be preserved over interrupts or similar calls.
 *
 * During interrupt and other critical calls, a spinlock is used to
 * protect the system, but the calls themselves save the address
 * in the address register in case they are interrupting another
 * access to the device.
 *
 * For general accesses a lock is provided so that calls which are
 * allowed to sleep are serialised so that the address register does
 * not need to be saved. This lock also serves to serialise access
 * to the EEPROM and PHY access registers which are shared between
 * these two devices.
 */

/* The driver supports the original EMACE, and now the two newer
 * devices, EMACA and EMACB.
 */

struct emac_board_info {
	struct clk		*clk;
	struct device		*dev;
	struct platform_device	*pdev;
	raw_spinlock_t		lock;
	void __iomem		*membase;
	void __iomem		*sram_remap;
	u32			msg_enable;
	struct net_device	*ndev;
	struct sk_buff		*skb_last;
	u16			tx_fifo_stat;

	int			emacrx_completed_flag;

	struct napi_struct	napi;

	struct phy_device	*phy_dev;
	struct device_node	*phy_node;
	unsigned int		link;
	unsigned int		speed;
	unsigned int		duplex;

	phy_interface_t		phy_interface;
	struct reset_control	*reset;

	struct regulator	*emac_supply[EMAC_POWER_CHAN_NUM];
	u32 emac_supply_vol[EMAC_POWER_CHAN_NUM];
	int phyrst;
	u8  rst_active_low;
};

struct emac_power {
	unsigned int vol;
	const char *name;
};

static void emac_phy_rst(struct emac_board_info *priv)
{
	/* If configure gpio to reset the phy device, we should reset it. */
	if (gpio_is_valid(priv->phyrst)) {
		msleep(20);
		gpio_direction_output(priv->phyrst, !priv->rst_active_low);
		msleep(20);
	}

}

static int emac_power_on(struct emac_board_info *priv)
{
	/* Nothing to do */
	return 0;
}

static void emac_power_off(struct emac_board_info *priv)
{
	/* Nothing to do */
}

static void emac_power_get(struct emac_board_info *priv)
{
	/* Nothing to do */
}

static void emac_power_put(struct emac_board_info *priv)
{
	/* Nothing to do */
}

static void emac_update_speed(struct net_device *dev)
{
	struct emac_board_info *db = netdev_priv(dev);
	unsigned int reg_val;

	/* set EMAC SPEED, depend on PHY  */
	reg_val = readl(db->membase + EMAC_MAC_SUPP_REG);
	reg_val &= ~(0x1 << 8);
	if (db->speed == SPEED_100)
		reg_val |= 1 << 8;
	writel(reg_val, db->membase + EMAC_MAC_SUPP_REG);
}

static void emac_update_duplex(struct net_device *dev)
{
	struct emac_board_info *db = netdev_priv(dev);
	unsigned int reg_val;

	/* set duplex depend on phy */
	reg_val = readl(db->membase + EMAC_MAC_CTL1_REG);
	reg_val &= ~EMAC_MAC_CTL1_DUPLEX_EN;
	if (db->duplex)
		reg_val |= EMAC_MAC_CTL1_DUPLEX_EN;
	writel(reg_val, db->membase + EMAC_MAC_CTL1_REG);
}

static void emac_handle_link_change(struct net_device *dev)
{
	struct emac_board_info *db = netdev_priv(dev);
	struct phy_device *phydev = db->phy_dev;
	unsigned long flags;
	int status_change = 0;

	if (phydev->link) {
		if (db->speed != phydev->speed) {
			raw_spin_lock_irqsave(&db->lock, flags);
			db->speed = phydev->speed;
			emac_update_speed(dev);
			raw_spin_unlock_irqrestore(&db->lock, flags);
			status_change = 1;
		}

		if (db->duplex != phydev->duplex) {
			raw_spin_lock_irqsave(&db->lock, flags);
			db->duplex = phydev->duplex;
			emac_update_duplex(dev);
			raw_spin_unlock_irqrestore(&db->lock, flags);
			status_change = 1;
		}
	}

	if (phydev->link != db->link) {
		if (!phydev->link) {
			db->speed = 0;
			db->duplex = -1;
		}
		db->link = phydev->link;

		status_change = 1;
	}

	if (status_change)
		phy_print_status(phydev);
}

static int emac_mdio_probe(struct net_device *dev)
{
	struct emac_board_info *db = netdev_priv(dev);

	/* to-do: PHY interrupts are currently not supported */

	/* attach the mac to the phy */
	db->phy_dev = of_phy_connect(db->ndev, db->phy_node,
				     &emac_handle_link_change, 0,
				     db->phy_interface);
	if (!db->phy_dev) {
		netdev_err(db->ndev, "could not find the PHY\n");
		return -ENODEV;
	}

	/* mask with MAC supported features */
	phy_set_max_speed(db->phy_dev, SPEED_100);

	db->link = 0;
	db->speed = 0;
	db->duplex = -1;

	return 0;
}

static void emac_mdio_remove(struct net_device *dev)
{
	struct emac_board_info *db = netdev_priv(dev);

	phy_disconnect(db->phy_dev);
	db->phy_dev = NULL;
}

static void emac_reset(struct emac_board_info *db)
{
	dev_dbg(db->dev, "resetting device\n");

	/* RESET device */
	writel(0, db->membase + EMAC_CTL_REG);
	udelay(200);
	writel(EMAC_CTL_RESET, db->membase + EMAC_CTL_REG);
	udelay(200);
}

static void emac_outblk_32bit(void __iomem *reg, void *data, int count)
{
	writesl(reg, data, round_up(count, 4) / 4);
}

static void emac_inblk_32bit(void __iomem *reg, void *data, int count)
{
	readsl(reg, data, round_up(count, 4) / 4);
}

/* ethtool ops */
static void emac_get_drvinfo(struct net_device *dev,
			      struct ethtool_drvinfo *info)
{
	strlcpy(info->driver, DRV_NAME, sizeof(info->driver));
	strlcpy(info->bus_info, dev_name(&dev->dev), sizeof(info->bus_info));
}

static u32 emac_get_msglevel(struct net_device *dev)
{
	struct emac_board_info *db = netdev_priv(dev);

	return db->msg_enable;
}

static void emac_set_msglevel(struct net_device *dev, u32 value)
{
	struct emac_board_info *db = netdev_priv(dev);

	db->msg_enable = value;
}

static const struct ethtool_ops emac_ethtool_ops = {
	.get_drvinfo	= emac_get_drvinfo,
	.get_link	= ethtool_op_get_link,
	.get_link_ksettings = phy_ethtool_get_link_ksettings,
	.set_link_ksettings = phy_ethtool_set_link_ksettings,
	.get_msglevel	= emac_get_msglevel,
	.set_msglevel	= emac_set_msglevel,
};

static unsigned int emac_setup(struct net_device *ndev)
{
	struct emac_board_info *db = netdev_priv(ndev);
	unsigned int reg_val;

	/* set up TX */
	reg_val = readl(db->membase + EMAC_TX_MODE_REG);

	writel(reg_val | EMAC_TX_MODE_ABORTED_FRAME_EN,
		db->membase + EMAC_TX_MODE_REG);

	/* set MAC */
	/* set MAC CTL0 */
	reg_val = readl(db->membase + EMAC_MAC_CTL0_REG);
	writel(reg_val | EMAC_MAC_CTL0_RX_FLOW_CTL_EN |
		EMAC_MAC_CTL0_TX_FLOW_CTL_EN,
		db->membase + EMAC_MAC_CTL0_REG);

	/* set MAC CTL1 */
	reg_val = readl(db->membase + EMAC_MAC_CTL1_REG);
	reg_val |= EMAC_MAC_CTL1_LEN_CHECK_EN;
	reg_val |= EMAC_MAC_CTL1_CRC_EN;
	reg_val |= EMAC_MAC_CTL1_PAD_EN;
	writel(reg_val, db->membase + EMAC_MAC_CTL1_REG);

	/* set up IPGT */
	writel(EMAC_MAC_IPGT_FULL_DUPLEX, db->membase + EMAC_MAC_IPGT_REG);

	/* set up IPGR */
	writel((EMAC_MAC_IPGR_IPG1 << 8) | EMAC_MAC_IPGR_IPG2,
		db->membase + EMAC_MAC_IPGR_REG);

	/* set up Collison window */
	writel((EMAC_MAC_CLRT_COLLISION_WINDOW << 8) | EMAC_MAC_CLRT_RM,
		db->membase + EMAC_MAC_CLRT_REG);

	/* set up Max Frame Length */
	writel(EMAC_MAX_FRAME_LEN,
		db->membase + EMAC_MAC_MAXF_REG);

	return 0;
}

static void emac_set_rx_mode(struct net_device *ndev)
{
	struct emac_board_info *db = netdev_priv(ndev);
	unsigned int reg_val;

	/* set up RX */
	reg_val = readl(db->membase + EMAC_RX_CTL_REG);

	if (ndev->flags & IFF_PROMISC)
		reg_val |= EMAC_RX_CTL_PASS_ALL_EN;
	else
		reg_val &= ~EMAC_RX_CTL_PASS_ALL_EN;

	writel(reg_val | EMAC_RX_CTL_PASS_LEN_OOR_EN |
		EMAC_RX_CTL_ACCEPT_UNICAST_EN | EMAC_RX_CTL_DA_FILTER_EN |
		EMAC_RX_CTL_ACCEPT_MULTICAST_EN |
		EMAC_RX_CTL_ACCEPT_BROADCAST_EN,
		db->membase + EMAC_RX_CTL_REG);
}

static unsigned int emac_powerup(struct net_device *ndev)
{
	struct emac_board_info *db = netdev_priv(ndev);
	unsigned int reg_val;

	/* initial EMAC */
	/* flush RX FIFO */
	reg_val = readl(db->membase + EMAC_RX_CTL_REG);
	reg_val |= 0x8;
	writel(reg_val, db->membase + EMAC_RX_CTL_REG);
	udelay(1);

	/* initial MAC */
	/* soft reset MAC */
	reg_val = readl(db->membase + EMAC_MAC_CTL0_REG);
	reg_val &= ~EMAC_MAC_CTL0_SOFT_RESET;
	writel(reg_val, db->membase + EMAC_MAC_CTL0_REG);

	/* set MII clock */
	reg_val = readl(db->membase + EMAC_MAC_MCFG_REG);
	reg_val &= (~(0xf << 2));
	reg_val |= (0xD << 2);
	writel(reg_val, db->membase + EMAC_MAC_MCFG_REG);

	/* clear RX counter */
	writel(0x0, db->membase + EMAC_RX_FBC_REG);

	/* disable all interrupt and clear interrupt status */
	writel(0, db->membase + EMAC_INT_CTL_REG);
	reg_val = readl(db->membase + EMAC_INT_STA_REG);
	writel(reg_val, db->membase + EMAC_INT_STA_REG);

	udelay(1);

	/* set up EMAC */
	emac_setup(ndev);

	/* set mac_address to chip */
	writel(ndev->dev_addr[0] << 16 | ndev->dev_addr[1] << 8 | ndev->
	       dev_addr[2], db->membase + EMAC_MAC_A1_REG);
	writel(ndev->dev_addr[3] << 16 | ndev->dev_addr[4] << 8 | ndev->
	       dev_addr[5], db->membase + EMAC_MAC_A0_REG);

	mdelay(1);

	return 0;
}

static int emac_set_mac_address(struct net_device *dev, void *p)
{
	struct sockaddr *addr = p;
	struct emac_board_info *db = netdev_priv(dev);

	if (netif_running(dev))
		return -EBUSY;

	memcpy(dev->dev_addr, addr->sa_data, ETH_ALEN);

	writel(dev->dev_addr[0] << 16 | dev->dev_addr[1] << 8 | dev->
	       dev_addr[2], db->membase + EMAC_MAC_A1_REG);
	writel(dev->dev_addr[3] << 16 | dev->dev_addr[4] << 8 | dev->
	       dev_addr[5], db->membase + EMAC_MAC_A0_REG);

	return 0;
}

/* Initialize emac board */
static void emac_init_device(struct net_device *dev)
{
	struct emac_board_info *db = netdev_priv(dev);
	unsigned long flags;
	unsigned int reg_val;

	raw_spin_lock_irqsave(&db->lock, flags);

	emac_update_speed(dev);
	emac_update_duplex(dev);

	/* enable RX/TX */
	reg_val = readl(db->membase + EMAC_CTL_REG);
	writel(reg_val | EMAC_CTL_RESET | EMAC_CTL_TX_EN | EMAC_CTL_RX_EN,
		db->membase + EMAC_CTL_REG);

	/* enable RX/TX0/RX Hlevel interrup */
	reg_val = readl(db->membase + EMAC_INT_CTL_REG);
	reg_val |= (0xf << 0) | (0x01 << 8);
	writel(reg_val, db->membase + EMAC_INT_CTL_REG);

	raw_spin_unlock_irqrestore(&db->lock, flags);
}

/* Our watchdog timed out. Called by the networking layer */
static void emac_timeout(struct net_device *dev, unsigned int txqueue)
{
	struct emac_board_info *db = netdev_priv(dev);
	unsigned long flags;

	if (netif_msg_timer(db))
		dev_err(db->dev, "tx time out.\n");

	/* Save previous register address */
	raw_spin_lock_irqsave(&db->lock, flags);

	netif_stop_queue(dev);
	emac_reset(db);
	emac_init_device(dev);
	/* We can accept TX packets again */
	netif_trans_update(dev);
	netif_wake_queue(dev);

	/* Restore previous register address */
	raw_spin_unlock_irqrestore(&db->lock, flags);
}

/* Hardware start transmission.
 * Send a packet to media from the upper layer.
 */
static netdev_tx_t emac_start_xmit(struct sk_buff *skb, struct net_device *dev)
{
	struct emac_board_info *db = netdev_priv(dev);
	unsigned long channel;
	unsigned long flags;

	channel = db->tx_fifo_stat & 3;
	if (channel == 3)
		return 1;

	channel = (channel == 1 ? 1 : 0);

	raw_spin_lock_irqsave(&db->lock, flags);

	writel(channel, db->membase + EMAC_TX_INS_REG);

	emac_outblk_32bit(db->membase + EMAC_TX_IO_DATA_REG,
			skb->data, skb->len);
	dev->stats.tx_bytes += skb->len;

	db->tx_fifo_stat |= 1 << channel;
	/* TX control: First packet immediately send, second packet queue */
	if (channel == 0) {
		/* set TX len */
		writel(skb->len, db->membase + EMAC_TX_PL0_REG);
		/* start translate from fifo to phy */
		writel(readl(db->membase + EMAC_TX_CTL0_REG) | 1,
		       db->membase + EMAC_TX_CTL0_REG);

		/* save the time stamp */
		netif_trans_update(dev);
	} else if (channel == 1) {
		/* set TX len */
		writel(skb->len, db->membase + EMAC_TX_PL1_REG);
		/* start translate from fifo to phy */
		writel(readl(db->membase + EMAC_TX_CTL1_REG) | 1,
		       db->membase + EMAC_TX_CTL1_REG);

		/* save the time stamp */
		netif_trans_update(dev);
	}

	if ((db->tx_fifo_stat & 3) == 3) {
		/* Second packet */
		netif_stop_queue(dev);
	}

	raw_spin_unlock_irqrestore(&db->lock, flags);

	/* free this SKB */
	dev_consume_skb_any(skb);

	return NETDEV_TX_OK;
}

/* EMAC interrupt handler
 * receive the packet to upper layer, free the transmitted packet
 */
static void emac_tx_done(struct net_device *dev, struct emac_board_info *db,
			  unsigned int tx_status)
{
	/* One packet sent complete */
	db->tx_fifo_stat &= ~(tx_status & 3);
	if (3 == (tx_status & 3))
		dev->stats.tx_packets += 2;
	else
		dev->stats.tx_packets++;

	if (netif_msg_tx_done(db))
		dev_dbg(db->dev, "tx done, NSR %02x\n", tx_status);

	netif_wake_queue(dev);
}

/* Received a packet and pass to upper layer
 */
static int emac_rx(struct net_device *dev, int limit)
{
	struct emac_board_info *db = netdev_priv(dev);
	struct sk_buff *skb;
	u8 *rdptr;
	bool good_packet;
	static int rxlen_last;
	unsigned int reg_val;
	u32 rxhdr, rxstatus, rxcount, rxlen, work_done = 0;

	/* Check packet ready or not */
	while (work_done < limit) {
		/* race warning: the first packet might arrive with
		 * the interrupts disabled, but the second will fix
		 * it
		 */
		rxcount = readl(db->membase + EMAC_RX_FBC_REG);
		work_done++;

		if (netif_msg_rx_status(db))
			dev_dbg(db->dev, "RXCount: %x\n", rxcount);

		if ((db->skb_last != NULL) && (rxlen_last > 0)) {
			dev->stats.rx_bytes += rxlen_last;

			/* Pass to upper layer */
			db->skb_last->protocol = eth_type_trans(db->skb_last,
								dev);
			netif_rx(db->skb_last);
			dev->stats.rx_packets++;
			db->skb_last = NULL;
			rxlen_last = 0;

			reg_val = readl(db->membase + EMAC_RX_CTL_REG);
			reg_val &= ~EMAC_RX_CTL_DMA_EN;
			writel(reg_val, db->membase + EMAC_RX_CTL_REG);
		}

		if (!rxcount) {
			db->emacrx_completed_flag = 1;
			reg_val = readl(db->membase + EMAC_INT_CTL_REG);
			reg_val |= (0xf << 0) | (0x01 << 8);
			writel(reg_val, db->membase + EMAC_INT_CTL_REG);

			/* had one stuck? */
			rxcount = readl(db->membase + EMAC_RX_FBC_REG);
			if (!rxcount)
				return work_done;
		}

		reg_val = readl(db->membase + EMAC_RX_IO_DATA_REG);
		if (netif_msg_rx_status(db))
			dev_dbg(db->dev, "receive header: %x\n", reg_val);
		if (reg_val != EMAC_UNDOCUMENTED_MAGIC) {
			/* disable RX */
			reg_val = readl(db->membase + EMAC_CTL_REG);
			writel(reg_val & ~EMAC_CTL_RX_EN,
			       db->membase + EMAC_CTL_REG);

			/* Flush RX FIFO */
			reg_val = readl(db->membase + EMAC_RX_CTL_REG);
			writel(reg_val | (1 << 3),
			       db->membase + EMAC_RX_CTL_REG);

			do {
				reg_val = readl(db->membase + EMAC_RX_CTL_REG);
			} while (reg_val & (1 << 3));

			/* enable RX */
			reg_val = readl(db->membase + EMAC_CTL_REG);
			writel(reg_val | EMAC_CTL_RX_EN,
			       db->membase + EMAC_CTL_REG);
			reg_val = readl(db->membase + EMAC_INT_CTL_REG);
			reg_val |= (0xf << 0) | (0x01 << 8);
			writel(reg_val, db->membase + EMAC_INT_CTL_REG);

			db->emacrx_completed_flag = 1;

			return work_done;
		}

		/* A packet ready now  & Get status/length */
		good_packet = true;

		emac_inblk_32bit(db->membase + EMAC_RX_IO_DATA_REG,
				&rxhdr, sizeof(rxhdr));

		if (netif_msg_rx_status(db))
			dev_dbg(db->dev, "rxhdr: %x\n", *((int *)(&rxhdr)));

		rxlen = EMAC_RX_IO_DATA_LEN(rxhdr);
		rxstatus = EMAC_RX_IO_DATA_STATUS(rxhdr);

		if (netif_msg_rx_status(db))
			dev_dbg(db->dev, "RX: status %02x, length %04x\n",
				rxstatus, rxlen);

		/* Packet Status check */
		if (rxlen < 0x40) {
			good_packet = false;
			if (netif_msg_rx_err(db))
				dev_dbg(db->dev, "RX: Bad Packet (runt)\n");
		}

		if (unlikely(!(rxstatus & EMAC_RX_IO_DATA_STATUS_OK))) {
			good_packet = false;

			if (rxstatus & EMAC_RX_IO_DATA_STATUS_CRC_ERR) {
				if (netif_msg_rx_err(db))
					dev_dbg(db->dev, "crc error\n");
				dev->stats.rx_crc_errors++;
			}

			if (rxstatus & EMAC_RX_IO_DATA_STATUS_LEN_ERR) {
				if (netif_msg_rx_err(db))
					dev_dbg(db->dev, "length error\n");
				dev->stats.rx_length_errors++;
			}
		}

		/* Move data from EMAC */
		if (good_packet) {
			skb = netdev_alloc_skb(dev, rxlen + 4);
			if (!skb)
				continue;
			skb_reserve(skb, 2);
			rdptr = (u8 *) skb_put(skb, rxlen - 4);

			/* Read received packet from RX SRAM */
			if (netif_msg_rx_status(db))
				dev_dbg(db->dev, "RxLen %x\n", rxlen);

			emac_inblk_32bit(db->membase + EMAC_RX_IO_DATA_REG,
					rdptr, rxlen);
			dev->stats.rx_bytes += rxlen;

			/* Pass to upper layer */
			skb->protocol = eth_type_trans(skb, dev);
			napi_gro_receive(&db->napi, skb);
			dev->stats.rx_packets++;
		}
	}

	return work_done;
}

static void emac_irq_disable(struct emac_board_info *db)
{
	/* Disable all interrupts */
	writel(0, db->membase + EMAC_INT_CTL_REG);
}

static void emac_irq_enable(struct emac_board_info *db)
{
	u32 reg_val;

	if (db->emacrx_completed_flag == 1) {
		reg_val = readl(db->membase + EMAC_INT_CTL_REG);
		reg_val |= (0xf << 0) | (0x01 << 8);
		writel(reg_val, db->membase + EMAC_INT_CTL_REG);
	}
}

static irqreturn_t emac_interrupt(int irq, void *dev_id)
{
	struct net_device *dev = dev_id;
	struct emac_board_info *db = netdev_priv(dev);
	int int_status;
	unsigned long flags;

	/* A real interrupt coming */

	/* holders of db->lock must always block IRQs */
	raw_spin_lock_irqsave(&db->lock, flags);

	/* Got EMAC interrupt status */
	/* Got ISR */
	int_status = readl(db->membase + EMAC_INT_STA_REG);
	/* Clear ISR status */
	writel(int_status, db->membase + EMAC_INT_STA_REG);

	if (netif_msg_intr(db))
		dev_dbg(db->dev, "emac interrupt %02x\n", int_status);

	/* Received the coming packet */
	if ((int_status & 0x100) && (db->emacrx_completed_flag == 1)) {
		/* carrier lost */
		db->emacrx_completed_flag = 0;
		if (likely(napi_schedule_prep(&db->napi))) {
			emac_irq_disable(db);
			__napi_schedule(&db->napi);
		}
	}

	/* Transmit Interrupt check */
	if (int_status & (0x01 | 0x02))
		emac_tx_done(dev, db, int_status);

	if (int_status & (0x04 | 0x08))
		netdev_info(dev, " ab : %x\n", int_status);

	raw_spin_unlock_irqrestore(&db->lock, flags);

	return IRQ_HANDLED;
}

#ifdef CONFIG_NET_POLL_CONTROLLER
/*
 * Used by netconsole
 */
static void emac_poll_controller(struct net_device *dev)
{
	disable_irq(dev->irq);
	emac_interrupt(dev->irq, dev);
	enable_irq(dev->irq);
}
#endif

/*  Open the interface.
 *  The interface is opened whenever "ifconfig" actives it.
 */
static int emac_open(struct net_device *dev)
{
	struct emac_board_info *db = netdev_priv(dev);
	int ret;
	u32 val;

	/* Remap sram A3 A4 to emac */
	val = readl(db->sram_remap);
	writel(val | ~EMAC_SRAM_REMAP_MASK, db->sram_remap);


	if (netif_msg_ifup(db))
		dev_dbg(db->dev, "enabling %s\n", dev->name);

	if (request_irq(dev->irq, &emac_interrupt, 0, dev->name, dev))
		return -EAGAIN;

	ret = emac_power_on(db);
	if (ret)
		goto out;

	emac_phy_rst(db);

	ret = clk_prepare_enable(db->clk);
	if (ret) {
		dev_err(db->dev, "Error couldn't enable clock (%d)\n", ret);
		goto power_out;
	}

	/* Initialize EMAC board */
	emac_powerup(dev);
	emac_reset(db);
	emac_init_device(dev);

	ret = emac_mdio_probe(dev);
	if (ret < 0) {
		free_irq(dev->irq, dev);
		netdev_err(dev, "cannot probe MDIO bus\n");
		goto clk_out;
	}

	phy_start(db->phy_dev);
	napi_enable(&db->napi);
	netif_start_queue(dev);

	return 0;

clk_out:
	clk_disable_unprepare(db->clk);
power_out:
	emac_power_off(db);
out:
	free_irq(dev->irq, dev);

	return ret;
}

static void emac_shutdown(struct net_device *dev)
{
	unsigned int reg_val;
	struct emac_board_info *db = netdev_priv(dev);
	u32 val;

	/* Disable all interrupt */
	writel(0, db->membase + EMAC_INT_CTL_REG);

	/* clear interupt status */
	reg_val = readl(db->membase + EMAC_INT_STA_REG);
	writel(reg_val, db->membase + EMAC_INT_STA_REG);

	/* Disable RX/TX */
	reg_val = readl(db->membase + EMAC_CTL_REG);
	reg_val &= ~(EMAC_CTL_TX_EN | EMAC_CTL_RX_EN | EMAC_CTL_RESET);
	writel(reg_val, db->membase + EMAC_CTL_REG);

	/* Unremap sram A3 A4 */
	val = readl(db->sram_remap);
	writel(val & EMAC_SRAM_REMAP_MASK, db->sram_remap);
}

/* Stop the interface.
 * The interface is stopped when it is brought.
 */
static int emac_stop(struct net_device *ndev)
{
	struct emac_board_info *db = netdev_priv(ndev);

	if (netif_msg_ifdown(db))
		dev_dbg(db->dev, "shutting down %s\n", ndev->name);

	netif_stop_queue(ndev);
	napi_disable(&db->napi);
	netif_carrier_off(ndev);

	phy_stop(db->phy_dev);

	emac_mdio_remove(ndev);

	emac_shutdown(ndev);

	free_irq(ndev->irq, ndev);

	clk_disable_unprepare(db->clk);

	emac_power_off(db);

	return 0;
}

static int emac_poll(struct napi_struct *napi, int budget)
{
	struct emac_board_info *db = container_of(napi, struct emac_board_info, napi);
	int work_done;

	work_done = emac_rx(db->ndev, budget);

	if (work_done < budget) {
		napi_complete(napi);
		emac_irq_enable(db);
	}

	return work_done;
}

static const struct net_device_ops emac_netdev_ops = {
	.ndo_open		= emac_open,
	.ndo_stop		= emac_stop,
	.ndo_start_xmit		= emac_start_xmit,
	.ndo_tx_timeout		= emac_timeout,
	.ndo_set_rx_mode	= emac_set_rx_mode,
	.ndo_do_ioctl		= phy_do_ioctl_running,
	.ndo_validate_addr	= eth_validate_addr,
	.ndo_set_mac_address	= emac_set_mac_address,
#ifdef CONFIG_NET_POLL_CONTROLLER
	.ndo_poll_controller	= emac_poll_controller,
#endif
};

/* Search EMAC board, allocate space and register it
 */
static int emac_probe(struct platform_device *pdev)
{
	struct device_node *np = pdev->dev.of_node;
	struct emac_board_info *db;
	struct net_device *ndev;
	int ret = 0;
	const char *mac_addr;
	enum of_gpio_flags flag;

	ndev = alloc_etherdev(sizeof(*db));
	if (!ndev) {
		dev_err(&pdev->dev, "could not allocate device.\n");
		return -ENOMEM;
	}

	SET_NETDEV_DEV(ndev, &pdev->dev);

	db = netdev_priv(ndev);
	memset(db, 0, sizeof(*db));

	db->dev = &pdev->dev;
	db->ndev = ndev;
	db->pdev = pdev;
	db->msg_enable = netif_msg_init(debug, EMAC_DEFAULT_MSG_ENABLE);

	raw_spin_lock_init(&db->lock);

	db->reset = devm_reset_control_get(&pdev->dev, NULL);
	if (IS_ERR(db->reset)) {
		dev_err(&pdev->dev, "Error couldn't get reset\n");
		goto out_reset;
	}

	ret = reset_control_reset(db->reset);
	if (ret) {
		dev_err(&pdev->dev, "Error reset control error\n");
		goto out_reset;
	}

	db->membase = of_iomap(np, 0);
	if (!db->membase) {
		dev_err(&pdev->dev, "failed to remap registers\n");
		ret = -ENOMEM;
		goto out;
	}

	/* fill in parameters for net-dev structure */
	ndev->base_addr = (unsigned long)db->membase;
	ndev->irq = irq_of_parse_and_map(np, 0);
	if (ndev->irq == -ENXIO) {
		netdev_err(ndev, "No irq resource\n");
		ret = ndev->irq;
		goto out;
	}


	db->sram_remap = of_iomap(np, 1);
	if (!db->sram_remap) {
		dev_err(&pdev->dev, "failed to reamp sram remap registers\n");
		ret = -ENOMEM;
		goto out_iounmap;
	}

	db->clk = devm_clk_get(&pdev->dev, NULL);
	if (IS_ERR(db->clk))
		goto out_release_sram;


	db->phy_node = of_parse_phandle(np, "phy-handle", 0);
	if (!db->phy_node) {
		dev_err(&pdev->dev, "no associated PHY\n");
		ret = -ENODEV;
		goto out_clkput;
	}

	db->phyrst = of_get_named_gpio_flags(np, "phy-rst", 0, &flag);
	db->rst_active_low = (flag == OF_GPIO_ACTIVE_LOW) ? 1 : 0;

	if (gpio_is_valid(db->phyrst)) {
		if (gpio_request(db->phyrst, "phy-rst") < 0) {
			pr_err("gmac gpio request fail!\n");
			ret = -EINVAL;
			goto out_clkput;
		}
	}

	/* config power regulator */
	emac_power_get(db);

	/* Read MAC-address from DT */
	mac_addr = of_get_mac_address(np);
	if (!IS_ERR(mac_addr))
		ether_addr_copy(ndev->dev_addr, mac_addr);

	/* Check if the MAC address is valid, if not get a random one */
	if (!is_valid_ether_addr(ndev->dev_addr)) {
		eth_hw_addr_random(ndev);
		dev_warn(&pdev->dev, "using random MAC address %pM\n",
			 ndev->dev_addr);
	}

	db->emacrx_completed_flag = 1;

	ndev->netdev_ops = &emac_netdev_ops;
	ndev->watchdog_timeo = msecs_to_jiffies(watchdog);
	ndev->ethtool_ops = &emac_ethtool_ops;

	platform_set_drvdata(pdev, ndev);

	/* Carrier starts down, phylib will bring it up */
	netif_carrier_off(ndev);

	netif_napi_add(ndev, &db->napi, emac_poll, NAPI_POLL_WEIGHT);

	ret = register_netdev(ndev);
	if (ret) {
		dev_err(&pdev->dev, "Registering netdev failed!\n");
		ret = -ENODEV;

		if (gpio_is_valid(db->phyrst))
			gpio_free(db->phyrst);

		goto out_powerput;
	}

	dev_info(&pdev->dev, "%s: at %p, IRQ %d MAC: %pM\n",
		 ndev->name, db->membase, ndev->irq, ndev->dev_addr);

	return 0;

out_powerput:
	netif_napi_del(&db->napi);
	emac_power_put(db);
out_clkput:
	clk_put(db->clk);
out_release_sram:
	iounmap(db->sram_remap);
out_iounmap:
	iounmap(db->membase);
out:
	dev_err(db->dev, "not found (%d).\n", ret);
	reset_control_assert(db->reset);
out_reset:
	free_netdev(ndev);

	return ret;
}

static int emac_remove(struct platform_device *pdev)
{
	struct net_device *ndev = platform_get_drvdata(pdev);
	struct emac_board_info *db = netdev_priv(ndev);

	unregister_netdev(ndev);
	netif_napi_del(&db->napi);

	iounmap(db->sram_remap);

	iounmap(db->membase);

	if (gpio_is_valid(db->phyrst))
		gpio_free(db->phyrst);
	clk_put(db->clk);
	free_netdev(ndev);

	dev_dbg(&pdev->dev, "released and freed device\n");
	return 0;
}

static int emac_suspend(struct platform_device *dev, pm_message_t state)
{
	struct net_device *ndev = platform_get_drvdata(dev);

	netif_carrier_off(ndev);
	netif_device_detach(ndev);
	emac_shutdown(ndev);

	return 0;
}

static int emac_resume(struct platform_device *dev)
{
	struct net_device *ndev = platform_get_drvdata(dev);
	struct emac_board_info *db = netdev_priv(ndev);

	emac_reset(db);
	emac_init_device(ndev);
	netif_device_attach(ndev);

	return 0;
}

static const struct of_device_id emac_of_match[] = {
	{.compatible = "allwinner,sunxi-emac",},
	{},
};

MODULE_DEVICE_TABLE(of, emac_of_match);

static struct platform_driver emac_driver = {
	.driver = {
		.name = "sun4i-emac",
		.of_match_table = emac_of_match,
	},
	.probe = emac_probe,
	.remove = emac_remove,
	.suspend = emac_suspend,
	.resume = emac_resume,
};

module_platform_driver(emac_driver);

MODULE_AUTHOR("Stefan Roese <sr@denx.de>");
MODULE_AUTHOR("Maxime Ripard <maxime.ripard@free-electrons.com>");
MODULE_AUTHOR("xuminghui <xuminghui@allwinnertech.com");
MODULE_DESCRIPTION("Allwinner emac network driver");
MODULE_LICENSE("GPL");
MODULE_VERSION(DRV_VERSION);
