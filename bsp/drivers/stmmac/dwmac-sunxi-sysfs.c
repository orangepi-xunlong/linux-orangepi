/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
* Allwinner DWMAC driver sysfs.
*
* Copyright(c) 2022-2027 Allwinnertech Co., Ltd.
*
*/

#include <sunxi-log.h>
#include <linux/bitrev.h>
#include <linux/completion.h>
#include <linux/crc32.h>
#include <linux/ethtool.h>
#include <linux/ip.h>
#include <linux/phy.h>
#include <linux/udp.h>
#include <net/pkt_cls.h>
#include <net/pkt_sched.h>
#include <net/tcp.h>
#include <net/udp.h>
#include <net/tc_act/tc_gact.h>
#include "stmmac.h"

#include "dwmac-sunxi-sysfs.h"

struct sunxi_dwmac_hdr {
	__be32 version;
	__be64 magic;
	u8 id;
	u32 tx;
	u32 rx;
} __packed;

#define SUNXI_DWMAC_PKT_SIZE	(sizeof(struct ethhdr) + sizeof(struct iphdr) + \
									sizeof(struct sunxi_dwmac_hdr))
#define SUNXI_DWMAC_PKT_MAGIC	0xdeadcafecafedeadULL
#define SUNXI_DWMAC_TIMEOUT		msecs_to_jiffies(2)

struct sunxi_dwmac_packet_attr {
	u32 tx;
	u32 rx;
	unsigned char *src;
	unsigned char *dst;
	u32 ip_src;
	u32 ip_dst;
	int tcp;
	int sport;
	int dport;
	int dont_wait;
	int timeout;
	int size;
	int max_size;
	u8 id;
	u16 queue_mapping;
	u64 timestamp;
};

struct sunxi_dwmac_loop_priv {
	struct sunxi_dwmac_packet_attr *packet;
	struct packet_type pt;
	struct completion comp;
	int ok;
};

struct sunxi_dwmac_calibrate {
	u8 id;
	u32 tx_delay;
	u32 rx_delay;
	u32 window_tx;
	u32 window_rx;
};

/**
 * sunxi_dwmac_parse_read_str - parse the input string for write attri.
 * @str: string to be parsed, eg: "0x00 0x01".
 * @addr: store the phy addr. eg: 0x00.
 * @reg: store the reg addr. eg: 0x01.
 *
 * return 0 if success, otherwise failed.
 */
static int sunxi_dwmac_parse_read_str(char *str, u16 *addr, u16 *reg)
{
	char *ptr = str;
	char *tstr = NULL;
	int ret;

	/**
	 * Skip the leading whitespace, find the true split symbol.
	 * And it must be 'address value'.
	 */
	tstr = strim(str);
	ptr = strchr(tstr, ' ');
	if (!ptr)
		return -EINVAL;

	/**
	 * Replaced split symbol with a %NUL-terminator temporary.
	 * Will be fixed at end.
	 */
	*ptr = '\0';
	ret = kstrtos16(tstr, 16, addr);
	if (ret)
		goto out;

	ret = kstrtos16(skip_spaces(ptr + 1), 16, reg);

out:
	return ret;
}

/**
 * sunxi_dwmac_parse_write_str - parse the input string for compare attri.
 * @str: string to be parsed, eg: "0x00 0x11 0x11".
 * @addr: store the phy addr. eg: 0x00.
 * @reg: store the reg addr. eg: 0x11.
 * @val: store the value. eg: 0x11.
 *
 * return 0 if success, otherwise failed.
 */
static int sunxi_dwmac_parse_write_str(char *str, u16 *addr,
					u16 *reg, u16 *val)
{
	u16 result_addr[3] = { 0 };
	char *ptr = str;
	char *ptr2 = NULL;
	int i, ret;

	for (i = 0; i < ARRAY_SIZE(result_addr); i++) {
		ptr = skip_spaces(ptr);
		ptr2 = strchr(ptr, ' ');
		if (ptr2)
			*ptr2 = '\0';

		ret = kstrtou16(ptr, 16, &result_addr[i]);

		if (!ptr2 || ret)
			break;

		ptr = ptr2 + 1;
	}

	*addr = result_addr[0];
	*reg = result_addr[1];
	*val = result_addr[2];

	return ret;
}

static struct sk_buff *sunxi_dwmac_get_skb(struct stmmac_priv *priv,
							struct sunxi_dwmac_packet_attr *attr)
{
	struct sk_buff *skb = NULL;
	struct udphdr *uhdr = NULL;
	struct tcphdr *thdr = NULL;
	struct sunxi_dwmac_hdr *shdr;
	struct ethhdr *ehdr;
	struct iphdr *ihdr;
	int iplen, size;

	size = attr->size + SUNXI_DWMAC_PKT_SIZE;

	if (attr->tcp)
		size += sizeof(*thdr);
	else
		size += sizeof(*uhdr);

	if (attr->max_size && (attr->max_size > size))
		size = attr->max_size;

	skb = netdev_alloc_skb(priv->dev, size);
	if (!skb)
		return NULL;

	prefetchw(skb->data);

	ehdr = skb_push(skb, ETH_HLEN);
	skb_reset_mac_header(skb);

	skb_set_network_header(skb, skb->len);
	ihdr = skb_put(skb, sizeof(*ihdr));

	skb_set_transport_header(skb, skb->len);
	if (attr->tcp)
		thdr = skb_put(skb, sizeof(*thdr));
	else
		uhdr = skb_put(skb, sizeof(*uhdr));

	eth_zero_addr(ehdr->h_source);
	eth_zero_addr(ehdr->h_dest);
	if (attr->src)
		ether_addr_copy(ehdr->h_source, attr->src);
	if (attr->dst)
		ether_addr_copy(ehdr->h_dest, attr->dst);

	ehdr->h_proto = htons(ETH_P_IP);

	if (attr->tcp) {
		thdr->source = htons(attr->sport);
		thdr->dest = htons(attr->dport);
		thdr->doff = sizeof(*thdr) / 4;
		thdr->check = 0;
	} else {
		uhdr->source = htons(attr->sport);
		uhdr->dest = htons(attr->dport);
		uhdr->len = htons(sizeof(*shdr) + sizeof(*uhdr) + attr->size);
		if (attr->max_size)
			uhdr->len = htons(attr->max_size -
						(sizeof(*ihdr) + sizeof(*ehdr)));
		uhdr->check = 0;
	}

	ihdr->ihl = 5;
	ihdr->ttl = 32;
	ihdr->version = 4;
	if (attr->tcp)
		ihdr->protocol = IPPROTO_TCP;
	else
		ihdr->protocol = IPPROTO_UDP;
	iplen = sizeof(*ihdr) + sizeof(*shdr) + attr->size;
	if (attr->tcp)
		iplen += sizeof(*thdr);
	else
		iplen += sizeof(*uhdr);

	if (attr->max_size)
		iplen = attr->max_size - sizeof(*ehdr);

	ihdr->tot_len = htons(iplen);
	ihdr->frag_off = 0;
	ihdr->saddr = htonl(attr->ip_src);
	ihdr->daddr = htonl(attr->ip_dst);
	ihdr->tos = 0;
	ihdr->id = 0;
	ip_send_check(ihdr);

	shdr = skb_put(skb, sizeof(*shdr));
	shdr->version = 0;
	shdr->magic = cpu_to_be64(SUNXI_DWMAC_PKT_MAGIC);
	shdr->id = attr->id;
	shdr->tx = attr->tx;
	shdr->rx = attr->rx;

	if (attr->size)
		skb_put(skb, attr->size);
	if (attr->max_size && (attr->max_size > skb->len))
		skb_put(skb, attr->max_size - skb->len);

	skb->csum = 0;
	skb->ip_summed = CHECKSUM_PARTIAL;
	if (attr->tcp) {
		thdr->check = ~tcp_v4_check(skb->len, ihdr->saddr, ihdr->daddr, 0);
		skb->csum_start = skb_transport_header(skb) - skb->head;
		skb->csum_offset = offsetof(struct tcphdr, check);
	} else {
		udp4_hwcsum(skb, ihdr->saddr, ihdr->daddr);
	}

	skb->protocol = htons(ETH_P_IP);
	skb->pkt_type = PACKET_HOST;
	skb->dev = priv->dev;

	if (attr->timestamp)
		skb->tstamp = ns_to_ktime(attr->timestamp);

	return skb;
}

static int sunxi_dwmac_loopback_validate(struct sk_buff *skb,
						struct net_device *ndev,
						struct packet_type *pt,
						struct net_device *orig_ndev)
{
	struct sunxi_dwmac_loop_priv *tpriv = pt->af_packet_priv;
	unsigned char *src = tpriv->packet->src;
	unsigned char *dst = tpriv->packet->dst;
	struct sunxi_dwmac_hdr *shdr;
	struct ethhdr *ehdr;
	struct udphdr *uhdr;
	struct tcphdr *thdr;
	struct iphdr *ihdr;

	skb = skb_unshare(skb, GFP_ATOMIC);
	if (!skb)
		goto out;

	if (skb_linearize(skb))
		goto out;
	if (skb_headlen(skb) < (SUNXI_DWMAC_PKT_SIZE - ETH_HLEN))
		goto out;

	ehdr = (struct ethhdr *)skb_mac_header(skb);
	if (dst) {
		if (!ether_addr_equal_unaligned(ehdr->h_dest, dst))
			goto out;
	}
	if (src) {
		if (!ether_addr_equal_unaligned(ehdr->h_source, src))
			goto out;
	}

	ihdr = ip_hdr(skb);

	if (tpriv->packet->tcp) {
		if (ihdr->protocol != IPPROTO_TCP)
			goto out;

		thdr = (struct tcphdr *)((u8 *)ihdr + 4 * ihdr->ihl);
		if (thdr->dest != htons(tpriv->packet->dport))
			goto out;

		shdr = (struct sunxi_dwmac_hdr *)((u8 *)thdr + sizeof(*thdr));
	} else {
		if (ihdr->protocol != IPPROTO_UDP)
			goto out;

		uhdr = (struct udphdr *)((u8 *)ihdr + 4 * ihdr->ihl);
		if (uhdr->dest != htons(tpriv->packet->dport))
			goto out;

		shdr = (struct sunxi_dwmac_hdr *)((u8 *)uhdr + sizeof(*uhdr));
	}

	if (shdr->magic != cpu_to_be64(SUNXI_DWMAC_PKT_MAGIC))
		goto out;
	if (tpriv->packet->id != shdr->id)
		goto out;
	if (tpriv->packet->tx != shdr->tx || tpriv->packet->rx != shdr->rx)
		goto out;

	tpriv->ok = true;
	complete(&tpriv->comp);
out:
	kfree_skb(skb);
	return 0;
}

static int sunxi_dwmac_loopback_run(struct stmmac_priv *priv,
					struct sunxi_dwmac_packet_attr *attr)
{
	struct sunxi_dwmac_loop_priv *tpriv;
	struct sk_buff *skb = NULL;
	int ret = 0;

	tpriv = kzalloc(sizeof(*tpriv), GFP_KERNEL);
	if (!tpriv)
		return -ENOMEM;

	tpriv->ok = false;
	init_completion(&tpriv->comp);

	tpriv->pt.type = htons(ETH_P_IP);
	tpriv->pt.func = sunxi_dwmac_loopback_validate;
	tpriv->pt.dev = priv->dev;
	tpriv->pt.af_packet_priv = tpriv;
	tpriv->packet = attr;

	if (!attr->dont_wait)
		dev_add_pack(&tpriv->pt);

	skb = sunxi_dwmac_get_skb(priv, attr);
	if (!skb) {
		ret = -ENOMEM;
		goto cleanup;
	}

	ret = dev_direct_xmit(skb, attr->queue_mapping);
	if (ret)
		goto cleanup;

	if (attr->dont_wait)
		goto cleanup;

	if (!attr->timeout)
		attr->timeout = SUNXI_DWMAC_TIMEOUT;

	wait_for_completion_timeout(&tpriv->comp, attr->timeout);
	ret = tpriv->ok ? 0 : -ETIMEDOUT;

cleanup:
	if (!attr->dont_wait)
		dev_remove_pack(&tpriv->pt);
	kfree(tpriv);
	return ret;
}

static int sunxi_dwmac_test_delaychain(struct sunxi_dwmac *chip, struct sunxi_dwmac_calibrate *cali)
{
	struct net_device *ndev = dev_get_drvdata(chip->dev);
	struct stmmac_priv *priv = netdev_priv(ndev);
	unsigned char src[ETH_ALEN] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
	unsigned char dst[ETH_ALEN] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
	struct sunxi_dwmac_packet_attr attr = { };

	chip->variant->set_delaychain(chip, SUNXI_DWMAC_DELAYCHAIN_TX, cali->tx_delay);
	chip->variant->set_delaychain(chip, SUNXI_DWMAC_DELAYCHAIN_RX, cali->rx_delay);

	attr.src = src;
	attr.dst = dst;
	attr.tcp = true;
	attr.queue_mapping = 0;
	stmmac_get_systime(priv, priv->ptpaddr, &attr.timestamp);
	attr.id = cali->id;
	attr.tx = cali->tx_delay;
	attr.rx = cali->rx_delay;

	return sunxi_dwmac_loopback_run(priv, &attr);
}

static int sunxi_dwmac_calibrate_scan_window(struct sunxi_dwmac *chip, struct sunxi_dwmac_calibrate *cali)
{
	struct net_device *ndev = dev_get_drvdata(chip->dev);
	struct stmmac_priv *priv = netdev_priv(ndev);
	char *buf, *ptr;
	int tx_sum, rx_sum, count;
	u32 tx, rx;
	int ret = 0;

	buf = devm_kzalloc(chip->dev, PAGE_SIZE, GFP_KERNEL);
	if (!buf)
		return -ENOMEM;

	netif_testing_on(ndev);

	ret = phy_loopback(priv->dev->phydev, true);
	if (ret)
		goto err;

	tx_sum = rx_sum = count = 0;

	for (tx = 0; tx < cali->window_tx; tx++) {
		ptr = buf;
		ptr += scnprintf(ptr, PAGE_SIZE - (ptr - buf), "TX(0x%02x): ", tx);
		for (rx = 0; rx < cali->window_rx; rx++) {
			cali->id++;
			cali->tx_delay = tx;
			cali->rx_delay = rx;
			if (sunxi_dwmac_test_delaychain(chip, cali) < 0) {
				ptr += scnprintf(ptr, PAGE_SIZE - (ptr - buf), "X");
			} else {
				tx_sum += tx;
				rx_sum += rx;
				count++;
				ptr += scnprintf(ptr, PAGE_SIZE - (ptr - buf), "-");
			}
		}
		ptr += scnprintf(ptr, PAGE_SIZE - (ptr - buf), "\n");
		printk(buf);
	}

	if (tx_sum && rx_sum && count) {
		cali->tx_delay = tx_sum / count;
		cali->rx_delay = rx_sum / count;
	} else {
		cali->tx_delay = cali->rx_delay = 0;
	}

	phy_loopback(priv->dev->phydev, false);

err:
	netif_testing_off(ndev);
	devm_kfree(chip->dev, buf);
	return ret;
}

static ssize_t sunxi_dwmac_calibrate_store(struct device *dev,
				 struct device_attribute *attr,
				 const char *buf, size_t count)
{
	struct net_device *ndev = dev_get_drvdata(dev);
	struct stmmac_priv *priv = netdev_priv(ndev);
	struct sunxi_dwmac *chip = priv->plat->bsp_priv;
	struct phy_device *phydev = priv->dev->phydev;
	struct sunxi_dwmac_calibrate *cali;
	u32 old_tx, old_rx;
	int ret;

	if (!ndev || !phydev) {
		sunxi_err(chip->dev, "Not found netdevice or phy\n");
		return -EINVAL;
	}

	if (!netif_carrier_ok(ndev) || !phydev->link) {
		sunxi_err(chip->dev, "Netdevice or phy not link\n");
		return -EINVAL;
	}

	cali = devm_kzalloc(dev, sizeof(*cali), GFP_KERNEL);
	if (!cali)
		return -ENOMEM;

	old_tx = chip->variant->get_delaychain(chip, SUNXI_DWMAC_DELAYCHAIN_TX);
	old_rx = chip->variant->get_delaychain(chip, SUNXI_DWMAC_DELAYCHAIN_RX);

	cali->window_tx = chip->variant->tx_delay_max + 1;
	cali->window_rx = chip->variant->rx_delay_max + 1;

	ret = sunxi_dwmac_calibrate_scan_window(chip, cali);
	if (ret) {
		sunxi_err(dev, "Calibrate scan window tx:%d rx:%d failed\n", cali->window_tx, cali->window_rx);
		goto err;
	}

	if (cali->tx_delay && cali->rx_delay) {
		chip->variant->set_delaychain(chip, SUNXI_DWMAC_DELAYCHAIN_TX, cali->tx_delay);
		chip->variant->set_delaychain(chip, SUNXI_DWMAC_DELAYCHAIN_RX, cali->rx_delay);
		sunxi_info(chip->dev, "Calibrate suitable delay tx:%d rx:%d\n", cali->tx_delay, cali->rx_delay);
	} else {
		chip->variant->set_delaychain(chip, SUNXI_DWMAC_DELAYCHAIN_TX, old_tx);
		chip->variant->set_delaychain(chip, SUNXI_DWMAC_DELAYCHAIN_RX, old_rx);
		sunxi_warn(chip->dev, "Calibrate cannot find suitable delay\n");
	}

err:
	devm_kfree(dev, cali);
	return count;
}

static int sunxi_dwmac_test_ecc_inject(struct stmmac_priv *priv, enum sunxi_dwmac_ecc_fifo_type type, u8 bit)
{
	struct sunxi_dwmac *chip = priv->plat->bsp_priv;
	static const u32 wdata[2] = {0x55555555, 0x55555555};
	u32 rdata[ARRAY_SIZE(wdata)];
	u32 mtl_dbg_ctl, mtl_dpp_ecc_eic;
	u32 val;
	int i, ret = 0;

	mtl_dbg_ctl = readl(priv->ioaddr + MTL_DBG_CTL);
	mtl_dpp_ecc_eic = readl(priv->ioaddr + MTL_DPP_ECC_EIC);

	mtl_dbg_ctl &= ~EIAEE; /* disable ecc error injection on address */
	mtl_dbg_ctl |= DBGMOD | FDBGEN; /* ecc debug mode enable */
	mtl_dpp_ecc_eic &= ~EIM; /* indicate error injection on data */
	mtl_dpp_ecc_eic |= FIELD_PREP(BLEI, 36); /* inject bit location is bit0 and bit36 */

	/* ecc select inject bit */
	switch (bit) {
	case 0:
		mtl_dbg_ctl &= ~EIEE; /* ecc inject error disable */
		break;
	case 1:
		mtl_dbg_ctl &= ~EIEC; /* ecc inject insert 1-bit error */
		mtl_dbg_ctl |= EIEE; /* ecc inject error enable */
		break;
	case 2:
		mtl_dbg_ctl |= EIEC; /* ecc inject insert 2-bit error */
		mtl_dbg_ctl |= EIEE; /* ecc inject error enable */
		break;
	default:
		ret = -EINVAL;
		sunxi_err(chip->dev, "test unsupport ecc inject bit %d\n", bit);
		goto err;
	}

	/* ecc select fifo */
	mtl_dbg_ctl &= ~FIFOSEL;
	switch (type) {
	case SUNXI_DWMAC_ECC_FIFO_TX:
		mtl_dbg_ctl |= FIELD_PREP(FIFOSEL, 0x0);
		break;
	case SUNXI_DWMAC_ECC_FIFO_RX:
		mtl_dbg_ctl |= FIELD_PREP(FIFOSEL, 0x3);
		break;
	default:
		ret = -EINVAL;
		sunxi_err(chip->dev, "test unsupport ecc inject fifo type %d\n", type);
		goto err;
	}

	writel(mtl_dpp_ecc_eic, priv->ioaddr + MTL_DPP_ECC_EIC);
	writel(mtl_dbg_ctl, priv->ioaddr + MTL_DBG_CTL);

	/* write fifo debug data */
	mtl_dbg_ctl &= ~FIFORDEN;
	mtl_dbg_ctl |= FIFOWREN;
	for (i = 0; i < ARRAY_SIZE(wdata); i++) {
		writel(wdata[i], priv->ioaddr + MTL_FIFO_DEBUG_DATA);
		writel(mtl_dbg_ctl, priv->ioaddr + MTL_DBG_CTL);
		ret = readl_poll_timeout_atomic(priv->ioaddr + MTL_DBG_STS, val, !(val & FIFOBUSY), 10, 200000);
		if (ret) {
			sunxi_err(chip->dev, "timeout with ecc debug fifo write busy (%#x)\n", val);
			goto err;
		}
	}

	/* read fifo debug data */
	mtl_dbg_ctl &= ~FIFOWREN;
	mtl_dbg_ctl |= FIFORDEN;
	for (i = 0; i < ARRAY_SIZE(wdata); i++) {
		writel(mtl_dbg_ctl, priv->ioaddr + MTL_DBG_CTL);
		ret = readl_poll_timeout_atomic(priv->ioaddr + MTL_DBG_STS, val, !(val & FIFOBUSY), 10, 200000);
		if (ret) {
			sunxi_err(chip->dev, "test timeout with ecc debug fifo read busy (%#x)\n", val);
			goto err;
		}
		rdata[i] = readl(priv->ioaddr + MTL_FIFO_DEBUG_DATA);
	}

	/* compare data */
	switch (bit) {
	case 0:
	case 1:
		/* for ecc error inject 0/1 bit, read should be same with write */
		for (i = 0; i < ARRAY_SIZE(wdata); i++) {
			if (rdata[i] != wdata[i]) {
				ret = -EINVAL;
				break;
			}
		}
		break;
	case 2:
		/* for ecc error inject 2 bit, read should be different with write */
		for (i = 0; i < ARRAY_SIZE(wdata); i++) {
			if (rdata[i] == wdata[i]) {
				ret = -EINVAL;
				break;
			}
		}
		break;
	}

	for (i = 0; i < ARRAY_SIZE(wdata); i++)
		sunxi_info(chip->dev, "fifo %d write [%#x] -> read [%#x]\n", i, wdata[i], rdata[i]);

err:
	/* ecc debug mode disable */
	mtl_dbg_ctl &= ~(EIEE | EIEC | FIFOWREN | FIFORDEN);
	writel(mtl_dbg_ctl, priv->ioaddr + MTL_DBG_CTL);

	return ret;
}

static ssize_t sunxi_dwmac_ecc_inject_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return scnprintf(buf, PAGE_SIZE,
			"Usage:\n"
			"echo \"[dir] [inject_bit]\" > ecc_inject\n\n"
			"[dir] : 0(tx) 1(rx)\n"
			"[inject_bit] : 0/1/2\n");
}

static ssize_t sunxi_dwmac_ecc_inject_store(struct device *dev,
				 struct device_attribute *attr,
				 const char *buf, size_t count)
{
	struct net_device *ndev = dev_get_drvdata(dev);
	struct stmmac_priv *priv = netdev_priv(ndev);
	struct sunxi_dwmac *chip = priv->plat->bsp_priv;
	struct phy_device *phydev = priv->dev->phydev;
	static const char *dir_str[] = {"tx", "rx"};
	u16 dir, inject_bit;
	u64 ret;

	if (!ndev || !phydev) {
		sunxi_err(chip->dev, "netdevice or phy not found\n");
		return -EINVAL;
	}

	if (!netif_running(ndev)) {
		sunxi_err(chip->dev, "netdevice is not running\n");
		return -EINVAL;
	}

	if (!(chip->variant->flags & SUNXI_DWMAC_MEM_ECC)) {
		sunxi_err(chip->dev, "ecc not support or enabled\n");
		return -EOPNOTSUPP;
	}

	ret = sunxi_dwmac_parse_read_str((char *)buf, &dir, &inject_bit);
	if (ret)
		return ret;

	switch (dir) {
	case 0:
		dir = SUNXI_DWMAC_ECC_FIFO_TX;
		break;
	case 1:
		dir = SUNXI_DWMAC_ECC_FIFO_RX;
		break;
	default:
		sunxi_err(chip->dev, "test unsupport ecc dir %d\n", dir);
		return -EINVAL;
	}

	netif_testing_on(ndev);

	/* ecc inject test */
	ret = sunxi_dwmac_test_ecc_inject(priv, dir, inject_bit);
	if (ret)
		sunxi_info(chip->dev, "test ecc %s inject %d bit : FAILED\n", dir_str[dir], inject_bit);
	else
		sunxi_info(chip->dev, "test ecc %s inject %d bit : PASS\n", dir_str[dir], inject_bit);

	netif_testing_off(ndev);

	return count;
}

static ssize_t sunxi_dwmac_tx_delay_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct net_device *ndev = dev_get_drvdata(dev);
	struct stmmac_priv *priv = netdev_priv(ndev);
	struct sunxi_dwmac *chip = priv->plat->bsp_priv;
	u32 delay = chip->variant->get_delaychain(chip, SUNXI_DWMAC_DELAYCHAIN_TX);

	return scnprintf(buf, PAGE_SIZE,
			"Usage:\n"
			"echo [0~%d] > tx_delay\n\n"
			"now tx_delay: %d\n",
			chip->variant->tx_delay_max, delay);
}

static ssize_t sunxi_dwmac_tx_delay_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	struct net_device *ndev = dev_get_drvdata(dev);
	struct stmmac_priv *priv = netdev_priv(ndev);
	struct sunxi_dwmac *chip = priv->plat->bsp_priv;
	int ret;
	u32 delay;

	if (!netif_running(ndev)) {
		sunxi_err(dev, "Eth is not running\n");
		return count;
	}

	ret = kstrtou32(buf, 0, &delay);
	if (ret)
		return ret;

	if (delay > chip->variant->tx_delay_max) {
		sunxi_err(dev, "Tx_delay exceed max %d\n", chip->variant->tx_delay_max);
		return -EINVAL;
	}

	chip->variant->set_delaychain(chip, SUNXI_DWMAC_DELAYCHAIN_TX, delay);

	return count;
}

static ssize_t sunxi_dwmac_rx_delay_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct net_device *ndev = dev_get_drvdata(dev);
	struct stmmac_priv *priv = netdev_priv(ndev);
	struct sunxi_dwmac *chip = priv->plat->bsp_priv;
	u32 delay = chip->variant->get_delaychain(chip, SUNXI_DWMAC_DELAYCHAIN_RX);

	return scnprintf(buf, PAGE_SIZE,
			"Usage:\n"
			"echo [0~%d] > rx_delay\n\n"
			"now rx_delay: %d\n",
			chip->variant->rx_delay_max, delay);
}

static ssize_t sunxi_dwmac_rx_delay_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	struct net_device *ndev = dev_get_drvdata(dev);
	struct stmmac_priv *priv = netdev_priv(ndev);
	struct sunxi_dwmac *chip = priv->plat->bsp_priv;
	int ret;
	u32 delay;

	if (!netif_running(ndev)) {
		sunxi_err(dev, "Eth is not running\n");
		return count;
	}

	ret = kstrtou32(buf, 0, &delay);
	if (ret)
		return ret;

	if (delay > chip->variant->rx_delay_max) {
		sunxi_err(dev, "Rx_delay exceed max %d\n", chip->variant->rx_delay_max);
		return -EINVAL;
	}

	chip->variant->set_delaychain(chip, SUNXI_DWMAC_DELAYCHAIN_RX, delay);

	return count;
}

static ssize_t sunxi_dwmac_mii_read_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct net_device *ndev = dev_get_drvdata(dev);
	struct stmmac_priv *priv = netdev_priv(ndev);
	struct sunxi_dwmac *chip = priv->plat->bsp_priv;

	chip->mii_reg.value = mdiobus_read(priv->mii, chip->mii_reg.addr, chip->mii_reg.reg);
	return sprintf(buf, "ADDR[0x%02x]:REG[0x%02x] = 0x%04x\n",
				chip->mii_reg.addr, chip->mii_reg.reg, chip->mii_reg.value);
}

static ssize_t sunxi_dwmac_mii_read_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	struct net_device *ndev = dev_get_drvdata(dev);
	struct stmmac_priv *priv = netdev_priv(ndev);
	struct sunxi_dwmac *chip = priv->plat->bsp_priv;
	int ret;
	u16 reg, addr;
	char *ptr;

	ptr = (char *)buf;

	ret = sunxi_dwmac_parse_read_str(ptr, &addr, &reg);
	if (ret)
		return ret;

	chip->mii_reg.addr = addr;
	chip->mii_reg.reg = reg;

	return count;
}

static ssize_t sunxi_dwmac_mii_write_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct net_device *ndev = dev_get_drvdata(dev);
	struct stmmac_priv *priv = netdev_priv(ndev);
	struct sunxi_dwmac *chip = priv->plat->bsp_priv;
	u16 bef_val, aft_val;

	bef_val = mdiobus_read(priv->mii, chip->mii_reg.addr, chip->mii_reg.reg);
	mdiobus_write(priv->mii, chip->mii_reg.addr, chip->mii_reg.reg, chip->mii_reg.value);
	aft_val = mdiobus_read(priv->mii, chip->mii_reg.addr, chip->mii_reg.reg);
	return sprintf(buf, "before ADDR[0x%02x]:REG[0x%02x] = 0x%04x\n"
				"after  ADDR[0x%02x]:REG[0x%02x] = 0x%04x\n",
				chip->mii_reg.addr, chip->mii_reg.reg, bef_val,
				chip->mii_reg.addr, chip->mii_reg.reg, aft_val);
}

static ssize_t sunxi_dwmac_mii_write_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	struct net_device *ndev = dev_get_drvdata(dev);
	struct stmmac_priv *priv = netdev_priv(ndev);
	struct sunxi_dwmac *chip = priv->plat->bsp_priv;
	int ret;
	u16 reg, addr, val;
	char *ptr;

	ptr = (char *)buf;

	ret = sunxi_dwmac_parse_write_str(ptr, &addr, &reg, &val);
	if (ret)
		return ret;

	chip->mii_reg.reg = reg;
	chip->mii_reg.addr = addr;
	chip->mii_reg.value = val;

	return count;
}

static ssize_t sunxi_dwmac_version_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct net_device *ndev = dev_get_drvdata(dev);
	struct stmmac_priv *priv = netdev_priv(ndev);
	struct sunxi_dwmac *chip = priv->plat->bsp_priv;
	u16 ip_tag, ip_vrm;
	ssize_t count = 0;

	if (chip->variant->get_version) {
		chip->variant->get_version(chip, &ip_tag, &ip_vrm);
		count = sprintf(buf, "IP TAG: %x\nIP VRM: %x\n", ip_tag, ip_vrm);
	}

	return count;
}

static struct device_attribute sunxi_dwmac_tool_attr[] = {
	__ATTR(calibrate, 0220, NULL, sunxi_dwmac_calibrate_store),
	__ATTR(rx_delay, 0664, sunxi_dwmac_rx_delay_show, sunxi_dwmac_rx_delay_store),
	__ATTR(tx_delay, 0664, sunxi_dwmac_tx_delay_show, sunxi_dwmac_tx_delay_store),
	__ATTR(mii_read, 0664, sunxi_dwmac_mii_read_show, sunxi_dwmac_mii_read_store),
	__ATTR(mii_write, 0664, sunxi_dwmac_mii_write_show, sunxi_dwmac_mii_write_store),
	__ATTR(ecc_inject, 0664, sunxi_dwmac_ecc_inject_show, sunxi_dwmac_ecc_inject_store),
	__ATTR(version, 0444, sunxi_dwmac_version_show, NULL),
};

void sunxi_dwmac_sysfs_init(struct device *dev)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(sunxi_dwmac_tool_attr); i++)
		device_create_file(dev, &sunxi_dwmac_tool_attr[i]);
}

void sunxi_dwmac_sysfs_exit(struct device *dev)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(sunxi_dwmac_tool_attr); i++)
		device_remove_file(dev, &sunxi_dwmac_tool_attr[i]);
}
