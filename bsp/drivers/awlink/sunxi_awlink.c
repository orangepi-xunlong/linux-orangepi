/*
 *
 * Copyright (C) [2023] [Allwinner]
 * Alternatively, provided that this notice is retained in full, this
 * software may be distributed under the terms of the GNU General
 * Public License ("GPL") version 2, in which case the provisions of the
 * GPL apply INSTEAD OF those given above.
 *
 * The provided data structures and external interfaces from this code
 * are not restricted to be used by modules with a GPL compatible license.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * This driver is specifically developed for the T527 device.
 */

#include <linux/netdevice.h>
#include <linux/can.h>
#include <linux/can/dev.h>
#include <linux/can/error.h>
#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/reset.h>
#include <linux/interrupt.h>
#include <linux/gpio.h>
#include <linux/pinctrl/consumer.h>
#include <linux/init.h>
#include <linux/io.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/platform_device.h>
#include <linux/of_irq.h>
#include <linux/version.h>
#include "sunxi_awlink_asm.h"
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 1, 0)
#include <linux/ethtool.h>
#include <linux/reset.h>

struct sunxi_awlink_quirks {
	bool has_reset;
};
#endif

#define DRV_NAME "sunxi-awlink"
#define DRV_VER		"V1.6"
#define COMMIT_ID	"157"

struct sunxi_awlink_priv {
	struct can_priv awlink;
	void __iomem *base;
	void __iomem *iomem_t;
	raw_spinlock_t cmdreg_lock;	/* lock for concurrent cmd register writes */
	bool is_suspend;
	raw_spinlock_t lock;
	int num;
	int awlink_pin;
	int tx_trigger;
	int tx_complete;
	int irq_max;
	bool printk_flag;
#if LINUX_VERSION_CODE == KERNEL_VERSION(6, 1, 0)
	struct reset_control *reset;
#endif
};

static const struct can_bittiming_const sunxi_awlink_bittiming_const = {
	.name = DRV_NAME,
	.tseg1_min = 1,
	.tseg1_max = 16,
	.tseg2_min = 2,
	.tseg2_max = 8,
	.sjw_max = 4,
	.brp_min = 2,
	.brp_max = 1024,
	.brp_inc = 1,
};

static ssize_t sunxi_awlink_status_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct net_device *ndev = NULL;
	struct sunxi_awlink_priv *priv = NULL;

	if (dev == NULL) {
		pr_err("Argment is invalid\n");
		return 0;
	}

	ndev = dev_get_drvdata(dev);
	if (ndev == NULL) {
		pr_err("Net device is null\n");
		return 0;
	}

	priv = netdev_priv(ndev);
	if (priv == NULL) {
		pr_err("sunxi_awlink_priv is null\n");
		return 0;
	}

	pr_info("tx_trigger = %d , tx_complete = %d, priv->irq_max = %d\n",\
				priv->tx_trigger, priv->tx_complete, priv->irq_max);
	return 0;
}

static ssize_t sunxi_awlink_status_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	return count;
}

static DEVICE_ATTR(check_status, 0664, sunxi_awlink_status_show, sunxi_awlink_status_store);

static void sunxi_awlink_write_cmdreg(struct sunxi_awlink_priv *priv, u8 val)
{
	unsigned long flags;

	raw_spin_lock_irqsave(&priv->cmdreg_lock, flags);
	awlink_asm_write_cmdreg(val, priv->base, priv->iomem_t);
	raw_spin_unlock_irqrestore(&priv->cmdreg_lock, flags);
	wmb();
	rmb();
}

static int set_normal_mode(struct net_device *dev)
{
	struct sunxi_awlink_priv *priv = netdev_priv(dev);
	int retry = SUNXI_MODE_MAX_RETRIES;
	u32 mod_reg_val = 0;

	do {
		mod_reg_val = readl(priv->base + SUNXI_REG_MSEL_ADDR);
		mod_reg_val &= ~SUNXI_MSEL_RESET_MODE;
		writel(mod_reg_val, priv->base + SUNXI_REG_MSEL_ADDR);
	} while (retry-- && (mod_reg_val & SUNXI_MSEL_RESET_MODE));

	if (readl(priv->base + SUNXI_REG_MSEL_ADDR) & SUNXI_MSEL_RESET_MODE) {
		netdev_err(dev, "setting controller into normal mode failed!\n");
		return -ETIMEDOUT;
	}

	return 0;
}

static int set_reset_mode(struct net_device *dev)
{
	struct sunxi_awlink_priv *priv = netdev_priv(dev);
	int retry = SUNXI_MODE_MAX_RETRIES;
	u32 mod_reg_val = 0;

	do {
		mod_reg_val = readl(priv->base + SUNXI_REG_MSEL_ADDR);
		mod_reg_val |= SUNXI_MSEL_RESET_MODE;
		writel(mod_reg_val, priv->base + SUNXI_REG_MSEL_ADDR);
	} while (retry-- && !(mod_reg_val & SUNXI_MSEL_RESET_MODE));

	if (!(readl(priv->base + SUNXI_REG_MSEL_ADDR) &	SUNXI_MSEL_RESET_MODE)) {
		netdev_err(dev, "setting controller into reset mode failed!\n");
		return -ETIMEDOUT;
	}

	return 0;
}

/* bittiming is called in reset_mode only */
static int sunxi_awlink_set_bittiming(struct net_device *dev)
{
	struct sunxi_awlink_priv *priv = netdev_priv(dev);
	struct can_bittiming *bt = &priv->awlink.bittiming;
	u32 cfg;

	cfg = ((bt->brp - 1) & 0x3FF) |
	     (((bt->sjw - 1) & 0x3) << 14) |
	     (((bt->prop_seg + bt->phase_seg1 - 1) & 0xf) << 16) |
	     (((bt->phase_seg2 - 1) & 0x7) << 20);

	awlink_asm_set_bittiming(priv->base, priv->awlink.ctrlmode, &cfg, priv->iomem_t);

	netdev_dbg(dev, "setting BITTIMING=0x%08x\n", cfg);

	return 0;
}

static int sunxi_awlink_get_berr_counter(const struct net_device *dev,
				     struct can_berr_counter *bec)
{
	struct sunxi_awlink_priv *priv = netdev_priv(dev);
	awlink_asm_fun1(priv->num);
	awlink_asm_fun2(priv->num);
	awlink_asm_clean_transfer_err(priv->base, &bec->txerr, &bec->rxerr);
	return 0;
}

static int sunxi_awlink_start(struct net_device *dev)
{
	struct sunxi_awlink_priv *priv = netdev_priv(dev);
	int err;

	/* we need to enter the reset mode */
	err = set_reset_mode(dev);
	if (err) {
		netdev_err(dev, "could not enter reset mode\n");
		return err;
	}

	awlink_asm_start(priv->base, priv->awlink.ctrlmode, priv->num);

	err = sunxi_awlink_set_bittiming(dev);
	if (err)
		return err;

	/* we are ready to enter the normal mode */
	err = set_normal_mode(dev);
	if (err) {
		netdev_err(dev, "could not enter normal mode\n");
		return err;
	}

	priv->awlink.state = CAN_STATE_ERROR_ACTIVE;

	return 0;
}

static int sunxi_awlink_stop(struct net_device *dev)
{
	struct sunxi_awlink_priv *priv = netdev_priv(dev);
	int err;

	priv->awlink.state = CAN_STATE_STOPPED;
	/* we need to enter reset mode */
	err = set_reset_mode(dev);
	if (err) {
		netdev_err(dev, "could not enter reset mode\n");
		return err;
	}

	/* disable all interrupts */
	writel(0, priv->base + SUNXI_REG_INTEN_ADDR);

	return 0;
}

static int sunxi_awlink_set_mode(struct net_device *dev, enum can_mode mode)
{
	int err;

	switch (mode) {
	case CAN_MODE_START:
		err = sunxi_awlink_start(dev);
		if (err) {
			netdev_err(dev, "starting AWLINK controller failed!\n");
			return err;
		}
		if (netif_queue_stopped(dev))
			netif_wake_queue(dev);
		break;

	default:
		return -EOPNOTSUPP;
	}
	return 0;
}

/* transmit a AWLINK message
 * message layout in the sk_buff should be like this:
 * xx xx xx xx         ff         ll 00 11 22 33 44 55 66 77
 * [ awlink_id ] [flags] [len] [awlink data (up to 8 bytes]
 */
#define TIME_OUT_MAX (200)
static netdev_tx_t sunxi_awlink_start_xmit(struct sk_buff *skb, struct net_device *dev)
{
	struct sunxi_awlink_priv *priv = netdev_priv(dev);
	struct can_frame *cf = (struct can_frame *)skb->data;
	u8 dlc;
	u32 dreg, msg_flag_n, status;
	canid_t id;
	int time_out = 0;
	do {
		status = readl(priv->base + SUNXI_REG_STA_ADDR);
		if ((!(status & SUNXI_STA_TRANS_BUSY)) && ((status & SUNXI_STA_TBUF_RDY)))
			break;
		udelay(50);
		if (++time_out > TIME_OUT_MAX) {
			if (priv->printk_flag == false) {
				netdev_warn(dev, "AWLINK tx time out, STA_TRANS_BUSY = %d, STA_TBUF_RDY = %d\n",\
					(int)(status & SUNXI_STA_TRANS_BUSY), (int)(status & SUNXI_STA_TBUF_RDY));
				netdev_warn(dev, "tx_trigger = %d , tx_complete = %d\n",\
					priv->tx_trigger, priv->tx_complete);
				priv->printk_flag = true;
			}
			return NETDEV_TX_BUSY;
		}

	} while (1);

	priv->printk_flag = false;
#if LINUX_VERSION_CODE == KERNEL_VERSION(6, 1, 0)
	if (can_dev_dropped_skb(dev, skb))
		return NETDEV_TX_OK;
#else
	if (can_dropped_invalid_skb(dev, skb))
		return NETDEV_TX_OK;
#endif

	netif_stop_queue(dev);

	id = cf->can_id;
	dlc = cf->can_dlc;
	msg_flag_n = dlc;

	awlink_asm_start_xmit(priv->base, priv->iomem_t, id, &msg_flag_n, &dreg, cf->data);

#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 15, 0)
	can_put_echo_skb(skb, dev, 0);
#else
	can_put_echo_skb(skb, dev, 0, 0);
#endif

	wmb();

	if (priv->awlink.ctrlmode & CAN_CTRLMODE_LOOPBACK)
		sunxi_awlink_write_cmdreg(priv, SUNXI_CMD_SELF_RCV_REQ);
	else
		sunxi_awlink_write_cmdreg(priv, SUNXI_CMD_TRANS_REQ);

	priv->tx_trigger++;

	return NETDEV_TX_OK;
}

static void sunxi_awlink_rx(struct net_device *dev)
{
	struct sunxi_awlink_priv *priv = netdev_priv(dev);
	struct net_device_stats *stats = &dev->stats;
	struct can_frame *cf;
	struct sk_buff *skb;
	u8 fi = 0;
	u8 dlc = 0;
	u32 dreg = 0;
	canid_t id = 0;
	int i;

	/* create zero'ed AWLINK frame buffer */
	skb = alloc_can_skb(dev, &cf);
	if (!skb)
		return;

	awlink_asm_rx(priv->base, &fi, &dreg, &id);

#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 15, 0)
	cf->can_dlc = get_can_dlc(fi & 0x0F);
	dlc = cf->can_dlc;
#else
	cf->len = can_cc_dlc2len(fi & 0x0F);
	dlc = cf->len;
#endif


	/* remote frame ? */
	if (fi & SUNXI_MSG_RTR_FLAG)
		id |= CAN_RTR_FLAG;
	else {
		for (i = 0; i < dlc; i++)
			cf->data[i] = readl(priv->base + dreg + i * 4);
		stats->rx_bytes += dlc;
	}
	stats->rx_packets++;

	cf->can_id = id;

	wmb();

	sunxi_awlink_write_cmdreg(priv, SUNXI_CMD_RELEASE_RBUF);

	netif_rx(skb);

}

static int sunxi_awlink_err(struct net_device *dev, u8 isrc, u8 status)
{
	struct sunxi_awlink_priv *priv = netdev_priv(dev);
	struct net_device_stats *stats = &dev->stats;
	struct can_frame *cf;
	struct sk_buff *skb;
	enum can_state state = priv->awlink.state;
	enum can_state rx_state, tx_state;
	u16 rxerr, txerr;
	u32 ecc, alc;

	/* we don't skip if alloc fails because we want the stats anyhow */
	skb = alloc_can_err_skb(dev, &cf);

	awlink_asm_clean_transfer_err(priv->base, &txerr, &rxerr);

#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 1, 0)
	if (skb) {
		cf->data[6] = txerr;
		cf->data[7] = rxerr;
	}
#endif

	if (isrc & SUNXI_INT_DATA_OR) {
		/* data overrun interrupt */
		netdev_dbg(dev, "data overrun interrupt\n");
		if (likely(skb)) {
			cf->can_id |= CAN_ERR_CRTL;
			cf->data[1] = CAN_ERR_CRTL_RX_OVERFLOW;
		}
		stats->rx_over_errors++;
		stats->rx_errors++;

		/* reset the AWLINK IP by entering reset mode
		 * ignoring timeout error
		 */
		set_reset_mode(dev);
		set_normal_mode(dev);

		/* clear bit */
		sunxi_awlink_write_cmdreg(priv, SUNXI_CMD_CLEAR_OR_FLAG);
	}
	if (isrc & SUNXI_INT_ERR_WRN) {
		/* error warning interrupt */
		netdev_dbg(dev, "error warning interrupt\n");

		if (status & SUNXI_STA_BUS_OFF)
			state = CAN_STATE_BUS_OFF;
		else if (status & SUNXI_STA_ERR_STA)
			state = CAN_STATE_ERROR_WARNING;
		else
			state = CAN_STATE_ERROR_ACTIVE;
	}

#if LINUX_VERSION_CODE == KERNEL_VERSION(6, 1, 0)
	if (skb && state != CAN_STATE_BUS_OFF) {
		cf->can_id |= CAN_ERR_CNT;
		cf->data[6] = txerr;
		cf->data[7] = rxerr;
	}
#endif

	if (isrc & SUNXI_INT_BUS_ERR) {
		/* bus error interrupt */
		netdev_dbg(dev, "bus error interrupt\n");
		priv->awlink.can_stats.bus_error++;
		stats->rx_errors++;

		if (likely(skb)) {
			ecc = readl(priv->base + SUNXI_REG_STA_ADDR);

			cf->can_id |= CAN_ERR_PROT | CAN_ERR_BUSERROR;

			switch (ecc & SUNXI_STA_MASK_ERR) {
			case SUNXI_STA_BIT_ERR:
				cf->data[2] |= CAN_ERR_PROT_BIT;
				break;
			case SUNXI_STA_FORM_ERR:
				cf->data[2] |= CAN_ERR_PROT_FORM;
				break;
			case SUNXI_STA_STUFF_ERR:
				cf->data[2] |= CAN_ERR_PROT_STUFF;
				break;
			default:
				cf->data[3] = (ecc & SUNXI_STA_ERR_SEG_CODE)
					       >> 16;
				break;
			}
			/* error occurred during transmission? */
			if ((ecc & SUNXI_STA_ERR_DIR) == 0)
				cf->data[2] |= CAN_ERR_PROT_TX;
		}
	}
	if (isrc & SUNXI_INT_ERR_PASSIVE) {
		/* error passive interrupt */
		netdev_dbg(dev, "error passive interrupt\n");
		if (state == CAN_STATE_ERROR_PASSIVE)
			state = CAN_STATE_ERROR_WARNING;
		else
			state = CAN_STATE_ERROR_PASSIVE;
	}
	if (isrc & SUNXI_INT_ARB_LOST) {
		/* arbitration lost interrupt */
		netdev_dbg(dev, "arbitration lost interrupt\n");
		alc = readl(priv->base + SUNXI_REG_STA_ADDR);
		priv->awlink.can_stats.arbitration_lost++;
		stats->tx_errors++;
		if (likely(skb)) {
			cf->can_id |= CAN_ERR_LOSTARB;
			cf->data[0] = (alc >> 8) & 0x1f;
		}
	}

	if (state != priv->awlink.state) {
		tx_state = txerr >= rxerr ? state : 0;
		rx_state = txerr <= rxerr ? state : 0;

		if (likely(skb))
			can_change_state(dev, cf, tx_state, rx_state);
		else
			priv->awlink.state = state;
		if (state == CAN_STATE_BUS_OFF)
			can_bus_off(dev);
	}

	if (likely(skb)) {
		stats->rx_packets++;
		stats->rx_bytes += cf->can_dlc;
		netif_rx(skb);
	} else {
		return -ENOMEM;
	}

	return 0;
}

static irqreturn_t sunxi_awlink_interrupt(int irq, void *dev_id)
{
	struct net_device *dev = (struct net_device *)dev_id;
	struct sunxi_awlink_priv *priv = netdev_priv(dev);
	struct net_device_stats *stats = &dev->stats;
	u8 isrc, status, isrc_write;
	int n = 0;

	while ((isrc = readl(priv->base + SUNXI_REG_INT_ADDR)) &&
			(n < SUNXI_AWLINK_MAX_IRQ)) {
		n++;
		status = readl(priv->base + SUNXI_REG_STA_ADDR);

		if (isrc & SUNXI_INT_TBUF_VLD) {
			/* transmission complete interrupt */
			writel(SUNXI_INT_TBUF_VLD, priv->base + SUNXI_REG_INT_ADDR);
			readl(priv->base + SUNXI_REG_INT_ADDR);

			priv->tx_complete++;
			stats->tx_packets++;

#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 15, 0)
			stats->tx_bytes += readl(priv->base + SUNXI_REG_RBUF_RBACK_START_ADDR) & 0xf;
			can_get_echo_skb(dev, 0);
#else
			stats->tx_bytes += can_get_echo_skb(dev, 0, NULL);
#endif

			netif_wake_queue(dev);

		}
		if (isrc & SUNXI_INT_RBUF_VLD) {
			while (status & SUNXI_STA_RBUF_RDY) {
				/* RX buffer is not empty */
				sunxi_awlink_rx(dev);
				status = readl(priv->base + SUNXI_REG_STA_ADDR);
			}
			writel(SUNXI_INT_RBUF_VLD, priv->base + SUNXI_REG_INT_ADDR);
			readl(priv->base + SUNXI_REG_INT_ADDR);
		}
		if (isrc &
		    (SUNXI_INT_DATA_OR | SUNXI_INT_ERR_WRN | SUNXI_INT_BUS_ERR |
		     SUNXI_INT_ERR_PASSIVE | SUNXI_INT_ARB_LOST)) {
			/* error interrupt */
			if (sunxi_awlink_err(dev, isrc, status))
				netdev_err(dev, "can't allocate buffer - clearing pending interrupts\n");

			isrc_write = (SUNXI_INT_DATA_OR | SUNXI_INT_ERR_WRN | SUNXI_INT_BUS_ERR | SUNXI_INT_ERR_PASSIVE | SUNXI_INT_ARB_LOST);
			writel(isrc_write, priv->base + SUNXI_REG_INT_ADDR);
			readl(priv->base + SUNXI_REG_INT_ADDR);
		}

		if (isrc & SUNXI_INT_WAKEUP) {
			writel(SUNXI_INT_WAKEUP, priv->base + SUNXI_REG_INT_ADDR);
			readl(priv->base + SUNXI_REG_INT_ADDR);
			netdev_warn(dev, "wakeup interrupt\n");
		}

	}
	if (n >= SUNXI_AWLINK_MAX_IRQ)
		netdev_dbg(dev, "%d messages handled in ISR", n);

	if (priv->irq_max < n)
		priv->irq_max = n;

	return (n) ? IRQ_HANDLED : IRQ_NONE;
}

static int sunxi_awlink_open(struct net_device *dev)
{
	struct sunxi_awlink_priv *priv = netdev_priv(dev);
	int err;

	/* clear cnt */
	priv->printk_flag = false;
	priv->tx_complete = 0;
	priv->tx_trigger = 0;
	priv->irq_max = 0;

	/* common open */
	err = open_candev(dev);
	if (err)
		return err;

	/* register interrupt handler */
	err = request_irq(dev->irq, sunxi_awlink_interrupt, 0, dev->name, dev);
	if (err) {
		netdev_err(dev, "request_irq err: %d\n", err);
		goto exit_irq;
	}

#if LINUX_VERSION_CODE == KERNEL_VERSION(6, 1, 0)
	err = reset_control_deassert(priv->reset);
	if (err) {
		netdev_err(dev, "could not deassert AWLINK reset\n");
		goto exit_awlink_start;
	}
#endif

	awlink_asm_fun0(priv->num, priv->awlink_pin);
	awlink_asm_fun1(priv->num);
	awlink_asm_fun2(priv->num);

	err = sunxi_awlink_start(dev);
	if (err) {
		netdev_err(dev, "could not start AWLINK peripheral\n");
		goto exit_awlink_start;
	}

	netif_start_queue(dev);

	return 0;

exit_awlink_start:
	free_irq(dev->irq, dev);
exit_irq:
	close_candev(dev);
	return err;
}

static int sunxi_awlink_close(struct net_device *dev)
{
	netif_stop_queue(dev);
	sunxi_awlink_stop(dev);
	free_irq(dev->irq, dev);
#if LINUX_VERSION_CODE == KERNEL_VERSION(6, 1, 0)
	reset_control_assert(priv->reset);
#endif

	close_candev(dev);

	return 0;
}

static const struct net_device_ops sunxi_awlink_netdev_ops = {
	.ndo_open = sunxi_awlink_open,
	.ndo_stop = sunxi_awlink_close,
	.ndo_start_xmit = sunxi_awlink_start_xmit,
};

#if LINUX_VERSION_CODE == KERNEL_VERSION(6, 1, 0)
static const struct ethtool_ops sunxi_awlink_ethtool_ops = {
	.get_ts_info = ethtool_op_get_ts_info,
};

static const struct sunxi_awlink_quirks sunxi_awlink_quirks_a10 = {
	.has_reset = false,
};

static const struct sunxi_awlink_quirks sunxi_awlink_quirks_r40 = {
	.has_reset = true,
};
#endif

static const struct of_device_id sunxi_awlink_of_match[] = {
	{.compatible = "allwinner,t527-awlink"},
	{},
};

MODULE_DEVICE_TABLE(of, sunxi_awlink_of_match);

static int sunxi_awlink_remove(struct platform_device *pdev)
{
	struct net_device *dev = platform_get_drvdata(pdev);
	struct sunxi_awlink_priv *priv = netdev_priv(dev);
	iounmap(priv->base);
	unregister_netdev(dev);
	free_candev(dev);
	return 0;
}

static int sunxi_awlink_probe(struct platform_device *pdev)
{
	void __iomem *addr;
	int err, irq;
	struct net_device *dev;
	struct sunxi_awlink_priv *priv;
	struct device_node *node;
	int len;
	struct property *pp;
	int num = 0, awlink_pin = 0;
	char *dev_name = "awlink%d";

	dev_info(&pdev->dev, "------awlink probe\n");
#if LINUX_VERSION_CODE == KERNEL_VERSION(6, 1, 0)
	struct reset_control *reset = NULL;
	const struct sunxi_awlink_quirks *quirks;

	quirks = of_device_get_match_data(&pdev->dev);
	if (!quirks) {
		dev_err(&pdev->dev, "failed to determine the quirks to use\n");
		err = -ENODEV;
		goto exit;
	}

	if (quirks->has_reset) {
		reset = devm_reset_control_get_exclusive(&pdev->dev, NULL);
		if (IS_ERR(reset)) {
			dev_err(&pdev->dev, "unable to request reset\n");
			err = PTR_ERR(reset);
			goto exit;
		}
	}
#endif

	node = pdev->dev.of_node;

	pp = of_find_property(node, "id", &len);
	if (pp == NULL)
		num = 0;
	else
		num = of_read_number(pp->value, len / 4);

	pp = of_find_property(node, "awlink-pin", &len);
	if (pp == NULL)
		awlink_pin = 0;
	else
		awlink_pin = of_read_number(pp->value, len / 4);

	irq = awlink_asm_probe(node, num);
	if (irq < 0) {
		err = -ENODEV;
		dev_err(&pdev->dev, "could not get irq\n");
		panic("awlink error\n");
		goto exit;
	}

	addr = awlink_asm_fun5(num);
	if (IS_ERR(addr)) {
		err = -EBUSY;
		dev_err(&pdev->dev,
			"could not get addr for AWLINK device\n");
		goto exit;
	}

	dev = alloc_candev(sizeof(struct sunxi_awlink_priv), 1);
	if (!dev) {
		dev_err(&pdev->dev,
			"could not allocate memory for AWLINK device\n");
		err = -ENOMEM;
		goto exit;
	}

	strcpy(dev->name, dev_name);

	dev->netdev_ops = &sunxi_awlink_netdev_ops;
	dev->irq = irq;
	dev->flags |= IFF_ECHO;

	priv = netdev_priv(dev);
	priv->awlink.clock.freq = 24000000;
	priv->awlink.bittiming_const = &sunxi_awlink_bittiming_const;
	priv->awlink.do_set_mode = sunxi_awlink_set_mode;
	priv->awlink.do_get_berr_counter = sunxi_awlink_get_berr_counter;
	priv->awlink.ctrlmode_supported = CAN_CTRLMODE_BERR_REPORTING |
									CAN_CTRLMODE_LISTENONLY |
									CAN_CTRLMODE_LOOPBACK |
									CAN_CTRLMODE_3_SAMPLES;
	priv->base = addr;

#if LINUX_VERSION_CODE == KERNEL_VERSION(6, 1, 0)
	priv->reset = reset;
	dev->ethtool_ops = &sunxi_awlink_ethtool_ops;
#endif

	priv->num = num;
	priv->awlink_pin = awlink_pin;

	raw_spin_lock_init(&priv->cmdreg_lock);
	raw_spin_lock_init(&priv->lock);

	platform_set_drvdata(pdev, dev);
	SET_NETDEV_DEV(dev, &pdev->dev);

	err = register_candev(dev);
	if (err) {
		dev_err(&pdev->dev, "registering %s failed (err=%d)\n",
			DRV_NAME, err);
		goto exit_free;
	}

	device_create_file(&pdev->dev, &dev_attr_check_status);

	dev_err(&pdev->dev, "awlink driver probe ok ...\n");
	return 0;

exit_free:
	free_candev(dev);
exit:
	return err;
}

#ifdef CONFIG_PM

static int awlink_suspend(struct device *dev)
{
	struct net_device *ndev = dev_get_drvdata(dev);
	struct sunxi_awlink_priv *priv = netdev_priv(ndev);

	if (!ndev || !netif_running(ndev))
		return 0;

	raw_spin_lock(&priv->lock);
	priv->is_suspend = true;
	raw_spin_unlock(&priv->lock);

	sunxi_awlink_close(ndev);

	return 0;
}

static int awlink_resume(struct device *dev)
{
	struct net_device *ndev = dev_get_drvdata(dev);
	struct sunxi_awlink_priv *priv = netdev_priv(ndev);

	if (!ndev || !netif_running(ndev))
		return 0;

	raw_spin_lock(&priv->lock);
	priv->is_suspend = false;
	raw_spin_unlock(&priv->lock);

	sunxi_awlink_open(ndev);

	return 0;
}

static const struct dev_pm_ops awlink_pm_ops = {
	.suspend = awlink_suspend,
	.resume = awlink_resume,
};
#else
static const struct dev_pm_ops awlink_pm_ops;
#endif /* CONFIG_PM */
static struct platform_driver sunxi_awlink_driver = {
	.driver = {
		.name = DRV_NAME,
		.pm = &awlink_pm_ops,
		.of_match_table = sunxi_awlink_of_match,
	},
	.probe = sunxi_awlink_probe,
	.remove = sunxi_awlink_remove,
};

static int sunxi_awlink_driver_initcall(void)
{
	return platform_driver_register(&sunxi_awlink_driver);
}
late_initcall(sunxi_awlink_driver_initcall);

static void __exit sunxi_awlink_driver_exit(void)
{
	platform_driver_unregister(&sunxi_awlink_driver);
}
__exitcall(sunxi_awlink_driver_exit);

MODULE_AUTHOR("wujiayi <wujiayi@allwinnertech.com>");
MODULE_LICENSE("Dual BSD/GPL");
MODULE_DESCRIPTION("AWLINK driver for Allwinner SoCs");
