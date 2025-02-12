// SPDX-License-Identifier: GPL-2.0
/*
 * Ky x1 emac ptp driver
 *
 * Copyright (c) 2023, ky Corporation.
 *
 */

#include <linux/bitops.h>
#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/dma-mapping.h>
#include <linux/etherdevice.h>
#include <linux/ethtool.h>
#include <linux/in.h>
#include <linux/io.h>
#include <linux/ip.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_gpio.h>
#include <linux/of_net.h>
#include <linux/of_mdio.h>
#include <linux/of_irq.h>
#include <linux/phy.h>
#include <linux/platform_device.h>
#include <linux/pm_qos.h>
#include <linux/ptp_clock_kernel.h>
#include <linux/timer.h>
#include <linux/time64.h>
#include <linux/types.h>
#include <linux/workqueue.h>
#include "x1-emac.h"

/* for ptp event message , udp port is 319 */
#define DEFAULT_UDP_PORT		(0x13F)

/* ptp ethernet type */
#define DEFAULT_ETH_TYPE		(0x88F7)


#define EMAC_SYSTIM_OVERFLOW_PERIOD	(HZ * 60 * 60 * 4)

#define to_emacpriv(_ptp) container_of(_ptp, struct emac_priv, ptp_clock_ops)

#define INCVALUE_100MHZ		10
#define INCVALUE_SHIFT_100MHZ	17
#define INCPERIOD_100MHZ	1

void emac_hw_timestamp_config(struct emac_priv *priv, u32 enable, u8 rx_ptp_type, u32 ptp_msg_id)
{
	void __iomem *ioaddr = priv->iobase;
	u32 val;

	if (enable) {
		/*
		 * enable tx/rx timestamp and config rx ptp type
		 */
		val = TX_TIMESTAMP_EN | RX_TIMESTAMP_EN;
		val |= (rx_ptp_type << RX_PTP_PKT_TYPE_OFST) & RX_PTP_PKT_TYPE_MSK;
		writel(val, ioaddr + PTP_1588_CTRL);

		/* config ptp message id */
		writel(ptp_msg_id, ioaddr + PTP_MSG_ID);

		/* config ptp ethernet type */
		writel(DEFAULT_ETH_TYPE, ioaddr + PTP_ETH_TYPE);

		/* config ptp udp port */
		writel(DEFAULT_UDP_PORT, ioaddr + PTP_UDP_PORT);

	} else
		writel(0, ioaddr + PTP_1588_CTRL);
}

u32 emac_hw_config_systime_increment(struct emac_priv *priv, u32 ptp_clock,
                                        u32 adj_clock)
{
	void __iomem *ioaddr = priv->iobase;
	u32 incr_val;
	u32 incr_period;
	u32 val;
	u32 period = 0, def_period = 0;
	/*
	 *  set system time counter resolution as ns
	 *  if ptp clock is 50Mhz,  20ns per clock cycle,
	 *  so increment value should be 20,
	 *  increment period should be 1m
	 */
	if (ptp_clock == adj_clock) {
		incr_val = INCVALUE_100MHZ << INCVALUE_SHIFT_100MHZ;
		incr_period = INCPERIOD_100MHZ;
	} else {
		def_period = div_u64(1000000000ULL, ptp_clock);
		period = div_u64(1000000000ULL, adj_clock);
		if (def_period == period)
			return 0;

		incr_period = 1;
		incr_val = (def_period * def_period)/ period;
	}

	val = (incr_val | (incr_period << INCR_PERIOD_OFST));
	writel(val, ioaddr + PTP_INRC_ATTR);

	return 0;
}

u64 emac_hw_get_systime(struct emac_priv *priv)
{
	void __iomem *ioaddr = priv->iobase;
	u64 systimel, systimeh;
	u64 systim;

	/* update system time adjust low register */
	systimel = readl(ioaddr + SYS_TIME_GET_LOW);
	systimeh = readl(ioaddr + SYS_TIME_GET_HI);
	/* perform system time adjust */
	systim = (systimeh << 32) | systimel;

	return systim;
}

u64 emac_hw_get_phc_time(struct emac_priv *priv)
{
	unsigned long flags;
	u64 cycles, ns;

	spin_lock_irqsave(&priv->ptp_lock, flags);
	/* first read system time low register */
	cycles = emac_hw_get_systime(priv);
	ns = timecounter_cyc2time(&priv->tc, cycles);

	spin_unlock_irqrestore(&priv->ptp_lock, flags);

	return ns;
}

u64 emac_hw_get_tx_timestamp(struct emac_priv *priv)
{
	void __iomem *ioaddr = priv->iobase;
	unsigned long flags;
	u64 systimel, systimeh;
	u64 systim;
	u64 ns;

	/* first read system time low register */
	systimel = readl(ioaddr + TX_TIMESTAMP_LOW);
	systimeh = readl(ioaddr + TX_TIMESTAMP_HI);
	systim = (systimeh << 32) | systimel;

	spin_lock_irqsave(&priv->ptp_lock, flags);

	ns = timecounter_cyc2time(&priv->tc, systim);
	spin_unlock_irqrestore(&priv->ptp_lock, flags);
	return ns;
}

u64 emac_hw_get_rx_timestamp(struct emac_priv *priv)
{
	void __iomem *ioaddr = priv->iobase;
	unsigned long flags;
	u64 systimel, systimeh;
	u64 systim;
	u64 ns;

	/* first read system time low register */
	systimel = readl(ioaddr + RX_TIMESTAMP_LOW);
	systimeh = readl(ioaddr + RX_TIMESTAMP_HI);
	systim = (systimeh << 32) | systimel;

	spin_lock_irqsave(&priv->ptp_lock, flags);

	ns = timecounter_cyc2time(&priv->tc, systim);
	spin_unlock_irqrestore(&priv->ptp_lock, flags);
	return ns;
}

/**
 * emac_cyclecounter_read - read raw cycle counter (used by time counter)
 * @cc: cyclecounter structure
 **/
static u64 emac_cyclecounter_read(const struct cyclecounter *cc)
{
	struct emac_priv *priv = container_of(cc, struct emac_priv, cc);

	return emac_hw_get_systime(priv);
}
	/* according to spec , set system time upper register and adjust time */
int emac_hw_init_systime(struct emac_priv *priv, u64 set_ns)
{
	unsigned long flags;

	spin_lock_irqsave(&priv->ptp_lock, flags);

	timecounter_init(&priv->tc, &priv->cc, set_ns);

	spin_unlock_irqrestore(&priv->ptp_lock, flags);

	return 0;
}

struct emac_hw_ptp emac_hwptp = {
	.config_hw_tstamping = emac_hw_timestamp_config,
	.config_systime_increment = emac_hw_config_systime_increment,
	.init_systime = emac_hw_init_systime,
	.get_phc_time = emac_hw_get_phc_time,
	.get_tx_timestamp = emac_hw_get_tx_timestamp,
	.get_rx_timestamp = emac_hw_get_rx_timestamp,
};

/**
 * emac_adjust_freq
 *
 * @ptp: pointer to ptp_clock_info structure
 * @ppb: desired period change in parts ber billion
 *
 * Description: this function will adjust the frequency of hardware clock.
 */
static int emac_adjust_freq(struct ptp_clock_info *ptp, s32 ppb)
{
	struct emac_priv *priv = to_emacpriv(ptp);
	void __iomem *ioaddr = priv->iobase;
	unsigned long flags;
	u32 addend, incvalue;
	int neg_adj = 0;
	u64 adj;

	if ((ppb > ptp->max_adj) || (ppb <= -1000000000))
		return -EINVAL;
	if (ppb < 0) {
		neg_adj = 1;
		ppb = -ppb;
	}

	spin_lock_irqsave(&priv->ptp_lock, flags);

	incvalue = INCVALUE_100MHZ << INCVALUE_SHIFT_100MHZ;
	adj = incvalue;
	adj *= ppb;
	adj = div_u64(adj, 1000000000);
	/*
	 * ppb = (Fnew - F0)/F0
	 * diff = F0 * ppb
	 */

	addend = neg_adj ? (incvalue - adj) : (incvalue + adj);
	pr_debug("emac_adjust_freq: new inc_val=%d ppb=%d\n", addend, ppb);
	addend = (addend | (INCPERIOD_100MHZ << INCR_PERIOD_OFST));
	writel(addend, ioaddr + PTP_INRC_ATTR);

	spin_unlock_irqrestore(&priv->ptp_lock, flags);

	return 0;
}

static int emac_adjust_fine(struct ptp_clock_info *ptp, long scaled_ppm)
{
	s32 ppb;

	ppb = scaled_ppm_to_ppb(scaled_ppm);
	return emac_adjust_freq(ptp, ppb);
}

/**
 * emac_adjust_time
 *
 * @ptp: pointer to ptp_clock_info structure
 * @delta: desired change in nanoseconds
 *
 * Description: this function will shift/adjust the hardware clock time.
 */
static int emac_adjust_time(struct ptp_clock_info *ptp, s64 delta)
{
	struct emac_priv *priv = to_emacpriv(ptp);
	unsigned long flags;

	spin_lock_irqsave(&priv->ptp_lock, flags);

	timecounter_adjtime(&priv->tc, delta);

	spin_unlock_irqrestore(&priv->ptp_lock, flags);

	return 0;
}

/**
 * emac_get_time
 *
 * @ptp: pointer to ptp_clock_info structure
 * @ts: pointer to hold time/result
 *
 * Description: this function will read the current time from the
 * hardware clock and store it in @ts.
 */
static int emac_phc_get_time(struct ptp_clock_info *ptp, struct timespec64 *ts)
{
	struct emac_priv *priv = to_emacpriv(ptp);
	unsigned long flags;
	u64 cycles, ns;

	spin_lock_irqsave(&priv->ptp_lock, flags);

	cycles = emac_hw_get_systime(priv);
	ns = timecounter_cyc2time(&priv->tc, cycles);

	spin_unlock_irqrestore(&priv->ptp_lock, flags);

	*ts = ns_to_timespec64(ns);

	return 0;
}

/**
 * emac_set_time
 *
 * @ptp: pointer to ptp_clock_info structure
 * @ts: time value to set
 *
 * Description: this function will set the current time on the
 * hardware clock.
 */
static int emac_phc_set_time(struct ptp_clock_info *ptp,
			   const struct timespec64 *ts)
{
	struct emac_priv *priv = to_emacpriv(ptp);
	unsigned long flags;
	u64 ns;

	ns = timespec64_to_ns(ts);

	spin_lock_irqsave(&priv->ptp_lock, flags);

	timecounter_init(&priv->tc, &priv->cc, ns);

	spin_unlock_irqrestore(&priv->ptp_lock, flags);

	return 0;
}

static void emac_systim_overflow_work(struct work_struct *work)
{
	struct emac_priv *priv = container_of(work, struct emac_priv,
						     systim_overflow_work.work);
	struct timespec64 ts;
	u64 ns;
	ns = timecounter_read(&priv->tc);
	ts = ns_to_timespec64(ns);
	pr_debug("SYSTIM overflow check at %lld.%09lu\n",
	      (long long) ts.tv_sec, ts.tv_nsec);
	schedule_delayed_work(&priv->systim_overflow_work,
			     EMAC_SYSTIM_OVERFLOW_PERIOD);
}
/* structure describing a PTP hardware clock */
static struct ptp_clock_info emac_ptp_clock_ops = {
	.owner = THIS_MODULE,
	.name = "emac_ptp_clock",
	.max_adj = 1000000000,
	.n_alarm = 0,
	.n_ext_ts = 0,
	.n_per_out = 0,
	.n_pins = 0,
	.pps = 0,
	.adjfine = emac_adjust_fine,
	.adjtime = emac_adjust_time,
	.gettime64 = emac_phc_get_time,
	.settime64 = emac_phc_set_time,
};

/**
 * emac_ptp_register
 * @priv: driver private structure
 * Description: this function will register the ptp clock driver
 * to kernel. It also does some house keeping work.
 */
void emac_ptp_register(struct emac_priv *priv)
{
	unsigned long flags;
	priv->cc.read = emac_cyclecounter_read;
	priv->cc.mask = CYCLECOUNTER_MASK(64);
	priv->cc.mult = 1;
	priv->cc.shift = INCVALUE_SHIFT_100MHZ;
	spin_lock_init(&priv->ptp_lock);
	priv->ptp_clock_ops = emac_ptp_clock_ops;

	INIT_DELAYED_WORK(&priv->systim_overflow_work,
			  emac_systim_overflow_work);
	schedule_delayed_work(&priv->systim_overflow_work,
			      EMAC_SYSTIM_OVERFLOW_PERIOD);
	priv->ptp_clock = ptp_clock_register(&priv->ptp_clock_ops,
					     NULL);
	if (IS_ERR(priv->ptp_clock)) {
		netdev_err(priv->ndev, "ptp_clock_register failed\n");
		priv->ptp_clock = NULL;
	} else if (priv->ptp_clock)
		netdev_info(priv->ndev, "registered PTP clock\n");
	else
		netdev_info(priv->ndev, "PTP_1588_CLOCK maybe not enabled\n");

	spin_lock_irqsave(&priv->ptp_lock, flags);
	timecounter_init(&priv->tc, &priv->cc,
			 ktime_to_ns(ktime_get_real()));
	spin_unlock_irqrestore(&priv->ptp_lock, flags);
	priv->hwptp = &emac_hwptp;
}

/**
 * emac_ptp_unregister
 * @priv: driver private structure
 * Description: this function will remove/unregister the ptp clock driver
 * from the kernel.
 */
void emac_ptp_unregister(struct emac_priv *priv)
{
	cancel_delayed_work_sync(&priv->systim_overflow_work);
	if (priv->ptp_clock) {
		ptp_clock_unregister(priv->ptp_clock);
		priv->ptp_clock = NULL;
		pr_debug("Removed PTP HW clock successfully on %s\n",
			 priv->ndev->name);
	}
	priv->hwptp = NULL;
}
