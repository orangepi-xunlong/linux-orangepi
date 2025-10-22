// SPDX-License-Identifier: GPL-2.0
/**
 * Copyright 2023 Allwinnertech
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/of_address.h>
#include <linux/of_platform.h>
#include <linux/of_net.h>
#include <linux/uio_driver.h>
#include <linux/clk.h>
#include <linux/kernel.h>
#include <linux/mii.h>
#include <linux/dma-mapping.h>
#include <linux/of_mdio.h>
#include <linux/slab.h>
#include <linux/version.h>
#include "stmmac_ptp.h"
#include "stmmac.h"
#include "hwif.h"

#define DRIVER_NAME	"sunxi_uio"
#define DRIVER_VERSION	"0.0.1"

#define TC_DEFAULT 64
static int tc = TC_DEFAULT;

#define DEFAULT_BUFSIZE	1536
static int buf_sz = DEFAULT_BUFSIZE;

#define STMMAC_RX_COPYBREAK	256

/**
 * sunxi_uio
 * local information for uio module driver
 *
 * @dev:      device pointer
 * @ndev:     network device pointer
 * @name:     uio name
 * @uio:      uio information
 * @map_num:  number of uio memory regions
 */
struct sunxi_uio {
	struct device *dev;
	struct net_device *ndev;
	char name[16];
	struct uio_info uio;
	int map_num;
};

static int sunxi_uio_open(struct uio_info *info, struct inode *inode)
{
	return 0;
}

static int sunxi_uio_release(struct uio_info *info,
				     struct inode *inode)
{
	return 0;
}

static int sunxi_uio_mmap(struct uio_info *info,
				  struct vm_area_struct *vma)
{
	u32 ret, pfn;

	pfn = (info->mem[vma->vm_pgoff].addr) >> PAGE_SHIFT;

	if (vma->vm_pgoff)
		vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);
	else
		vma->vm_page_prot = pgprot_device(vma->vm_page_prot);

	ret = remap_pfn_range(vma, vma->vm_start, pfn,
			      vma->vm_end - vma->vm_start, vma->vm_page_prot);
	if (ret) {
		/* Error Handle */
		pr_err("remap_pfn_range failed");
	}
	return ret;
}

/**
 * sunxi_uio_free_dma_rx_desc_resources - free RX dma desc resources
 * @priv: private structure
 */
static void sunxi_uio_free_dma_rx_desc_resources(struct stmmac_priv *priv)
{
	u32 queue, rx_count = priv->plat->rx_queues_to_use;

	/* Free RX queue resources */
	for (queue = 0; queue < rx_count; queue++) {
		struct stmmac_rx_queue *rx_q = &priv->rx_queue[queue];

		/* Free DMA regions of consistent memory previously allocated */
		if (!priv->extend_desc)
			dma_free_coherent(priv->device, priv->dma_rx_size *
					  sizeof(struct dma_desc),
					  rx_q->dma_rx, rx_q->dma_rx_phy);
		else
			dma_free_coherent(priv->device, priv->dma_rx_size *
					  sizeof(struct dma_extended_desc),
					  rx_q->dma_erx, rx_q->dma_rx_phy);
	}
}

/**
 * sunxi_uio_free_dma_tx_desc_resources - free TX dma desc resources
 * @priv: private structure
 */
static void sunxi_uio_free_dma_tx_desc_resources(struct stmmac_priv *priv)
{
	u32 queue, tx_count = priv->plat->tx_queues_to_use;

	/* Free TX queue resources */
	for (queue = 0; queue < tx_count; queue++) {
		struct stmmac_tx_queue *tx_q = &priv->tx_queue[queue];
		size_t size;
		void *addr;

		if (priv->extend_desc) {
			size = sizeof(struct dma_extended_desc);
			addr = tx_q->dma_etx;
		} else if (tx_q->tbs & STMMAC_TBS_AVAIL) {
			size = sizeof(struct dma_edesc);
			addr = tx_q->dma_entx;
		} else {
			size = sizeof(struct dma_desc);
			addr = tx_q->dma_tx;
		}

		size *= priv->dma_tx_size;

		dma_free_coherent(priv->device, size, addr, tx_q->dma_tx_phy);
	}
}

/**
 * sunxi_uio_alloc_dma_rx_desc_resources - alloc RX resources.
 * @priv: private structure
 * Description: according to which descriptor can be used (extend or basic)
 * this function allocates the resources for TX and RX paths. In case of
 * reception, for example, it pre-allocated the RX socket buffer in order to
 * allow zero-copy mechanism.
 */
static int sunxi_uio_alloc_dma_rx_desc_resources(struct stmmac_priv *priv)
{
	u32 queue, rx_count = priv->plat->rx_queues_to_use;
	int ret = -ENOMEM;

	/* RX queues buffers and DMA */
	for (queue = 0; queue < rx_count; queue++) {
		struct stmmac_rx_queue *rx_q = &priv->rx_queue[queue];

		if (priv->extend_desc) {
			rx_q->dma_erx = dma_alloc_coherent(priv->device,
							   priv->dma_rx_size *
							   sizeof(struct dma_extended_desc),
							   &rx_q->dma_rx_phy,
							   GFP_KERNEL);
			if (!rx_q->dma_erx)
				goto err_dma;
		} else {
			rx_q->dma_rx = dma_alloc_coherent(priv->device,
							  priv->dma_rx_size *
							  sizeof(struct dma_desc),
							  &rx_q->dma_rx_phy,
							  GFP_KERNEL);
			if (!rx_q->dma_rx)
				goto err_dma;
		}
	}

	return 0;

err_dma:
	sunxi_uio_free_dma_rx_desc_resources(priv);

	return ret;
}

/**
 * sunxi_uio_alloc_dma_tx_desc_resources - alloc TX resources.
 * @priv: private structure
 * Description: according to which descriptor can be used (extend or basic)
 * this function allocates the resources for TX and RX paths. In case of
 * reception, for example, it pre-allocated the RX socket buffer in order to
 * allow zero-copy mechanism.
 */
static int sunxi_uio_alloc_dma_tx_desc_resources(struct stmmac_priv *priv)
{
	u32 queue, tx_count = priv->plat->tx_queues_to_use;
	int ret = -ENOMEM;

	/* TX queues buffers and DMA */
	for (queue = 0; queue < tx_count; queue++) {
		struct stmmac_tx_queue *tx_q = &priv->tx_queue[queue];
		size_t size;
		void *addr;

		tx_q->queue_index = queue;
		tx_q->priv_data = priv;

		if (priv->extend_desc)
			size = sizeof(struct dma_extended_desc);
		else if (tx_q->tbs & STMMAC_TBS_AVAIL)
			size = sizeof(struct dma_edesc);
		else
			size = sizeof(struct dma_desc);

		size *= priv->dma_tx_size;

		addr = dma_alloc_coherent(priv->device, size,
					  &tx_q->dma_tx_phy, GFP_KERNEL);
		if (!addr)
			goto err_dma;

		if (priv->extend_desc)
			tx_q->dma_etx = addr;
		else if (tx_q->tbs & STMMAC_TBS_AVAIL)
			tx_q->dma_entx = addr;
		else
			tx_q->dma_tx = addr;
	}

	return 0;

err_dma:
	sunxi_uio_free_dma_tx_desc_resources(priv);
	return ret;
}

/**
 * sunxi_uio_alloc_dma_desc_resources - alloc TX/RX resources.
 * @priv: private structure
 * Description: according to which descriptor can be used (extend or basic)
 * this function allocates the resources for TX and RX paths. In case of
 * reception, for example, it pre-allocated the RX socket buffer in order to
 * allow zero-copy mechanism.
 */
static int sunxi_uio_alloc_dma_desc_resources(struct stmmac_priv *priv)
{
	/* RX Allocation */
	int ret = sunxi_uio_alloc_dma_rx_desc_resources(priv);

	if (ret)
		return ret;

	ret = sunxi_uio_alloc_dma_tx_desc_resources(priv);

	return ret;
}

/**
 * sunxi_uio_free_dma_desc_resources - free dma desc resources
 * @priv: private structure
 */
static void sunxi_uio_free_dma_desc_resources(struct stmmac_priv *priv)
{
	/* Release the DMA RX socket buffers */
	sunxi_uio_free_dma_rx_desc_resources(priv);

	/* Release the DMA TX socket buffers */
	sunxi_uio_free_dma_tx_desc_resources(priv);
}

/**
 * sunxi_uio_init_phy - PHY initialization
 * @dev: net device structure
 * Description: it initializes the driver's PHY state, and attaches the PHY
 * to the mac driver.
 *  Return value:
 *  0 on success
 */
static int sunxi_uio_init_phy(struct net_device *dev)
{
	struct stmmac_priv *priv = netdev_priv(dev);
	struct device_node *node;
	int ret;

	node = priv->plat->phylink_node;

	if (node)
		ret = phylink_of_phy_connect(priv->phylink, node, 0);

	/* Some DT bindings do not set-up the PHY handle. Let's try to
	 * manually parse it
	 */
	if (!node || ret) {
		int addr = priv->plat->phy_addr;
		struct phy_device *phydev;

		phydev = mdiobus_get_phy(priv->mii, addr);
		if (!phydev) {
			netdev_err(priv->dev, "no phy at addr %d\n", addr);
			return -ENODEV;
		}

		ret = phylink_connect_phy(priv->phylink, phydev);
	}

	if (!priv->plat->pmt) {
		struct ethtool_wolinfo wol = { .cmd = ETHTOOL_GWOL };

		phylink_ethtool_get_wol(priv->phylink, &wol);
		device_set_wakeup_capable(priv->device, !!wol.supported);
	}

	return ret;
}

/**
 * sunxi_uio_init_dma_engine - DMA init.
 * @priv: driver private structure
 * Description:
 * It inits the DMA invoking the specific MAC/GMAC callback.
 * Some DMA parameters can be passed from the platform;
 * in case of these are not passed a default is kept for the MAC or GMAC.
 */
static int sunxi_uio_init_dma_engine(struct stmmac_priv *priv)
{
	u32 rx_channels_count = priv->plat->rx_queues_to_use;
	u32 tx_channels_count = priv->plat->tx_queues_to_use;
	u32 dma_csr_ch = max(rx_channels_count, tx_channels_count);
	struct stmmac_rx_queue *rx_q;
	struct stmmac_tx_queue *tx_q;
	u32 chan = 0;
	int atds = 0, ret = 0;

	if (!priv->plat->dma_cfg || !priv->plat->dma_cfg->pbl) {
		dev_err(priv->device, "Invalid DMA configuration\n");
		return -EINVAL;
	}

	if (priv->extend_desc && priv->mode == STMMAC_RING_MODE)
		atds = 1;

	ret = stmmac_reset(priv, priv->ioaddr);
	if (ret) {
		dev_err(priv->device, "Failed to reset the dma\n");
		return ret;
	}

	/* DMA Configuration */
	stmmac_dma_init(priv, priv->ioaddr, priv->plat->dma_cfg, atds);

	if (priv->plat->axi)
		stmmac_axi(priv, priv->ioaddr, priv->plat->axi);

	/* DMA CSR Channel configuration */
	for (chan = 0; chan < dma_csr_ch; chan++)
		stmmac_init_chan(priv, priv->ioaddr, priv->plat->dma_cfg, chan);

	/* DMA RX Channel Configuration */
	for (chan = 0; chan < rx_channels_count; chan++) {
		rx_q = &priv->rx_queue[chan];

		stmmac_init_rx_chan(priv, priv->ioaddr, priv->plat->dma_cfg,
				    rx_q->dma_rx_phy, chan);

		rx_q->rx_tail_addr = rx_q->dma_rx_phy +
				     (priv->dma_rx_size *
				      sizeof(struct dma_desc));
		stmmac_set_rx_tail_ptr(priv, priv->ioaddr,
				       rx_q->rx_tail_addr, chan);
	}

	/* DMA TX Channel Configuration */
	for (chan = 0; chan < tx_channels_count; chan++) {
		tx_q = &priv->tx_queue[chan];

		stmmac_init_tx_chan(priv, priv->ioaddr, priv->plat->dma_cfg,
				    tx_q->dma_tx_phy, chan);

		tx_q->tx_tail_addr = tx_q->dma_tx_phy;
		stmmac_set_tx_tail_ptr(priv, priv->ioaddr,
				       tx_q->tx_tail_addr, chan);
	}

	return ret;
}

static void sunxi_uio_set_rings_length(struct stmmac_priv *priv)
{
	u32 rx_channels_count = priv->plat->rx_queues_to_use;
	u32 tx_channels_count = priv->plat->tx_queues_to_use;
	u32 chan;

	/* set TX ring length */
	for (chan = 0; chan < tx_channels_count; chan++)
		stmmac_set_tx_ring_len(priv, priv->ioaddr,
				       (priv->dma_tx_size - 1), chan);

	/* set RX ring length */
	for (chan = 0; chan < rx_channels_count; chan++)
		stmmac_set_rx_ring_len(priv, priv->ioaddr,
				       (priv->dma_rx_size - 1), chan);
}

/**
 *  sunxi_uio_set_tx_queue_weight - Set TX queue weight
 *  @priv: driver private structure
 *  Description: It is used for setting TX queues weight
 */
static void sunxi_uio_set_tx_queue_weight(struct stmmac_priv *priv)
{
	u32 tx_queues_count = priv->plat->tx_queues_to_use;
	u32 weight, queue;

	for (queue = 0; queue < tx_queues_count; queue++) {
		weight = priv->plat->tx_queues_cfg[queue].weight;
		stmmac_set_mtl_tx_queue_weight(priv, priv->hw, weight, queue);
	}
}

/**
 *  sunxi_uio_configure_cbs - Configure CBS in TX queue
 *  @priv: driver private structure
 *  Description: It is used for configuring CBS in AVB TX queues
 */
static void sunxi_uio_configure_cbs(struct stmmac_priv *priv)
{
	u32 tx_queues_count = priv->plat->tx_queues_to_use;
	u32 mode_to_use, queue;

	/* queue 0 is reserved for legacy traffic */
	for (queue = 1; queue < tx_queues_count; queue++) {
		mode_to_use = priv->plat->tx_queues_cfg[queue].mode_to_use;
		if (mode_to_use == MTL_QUEUE_DCB)
			continue;

		stmmac_config_cbs(priv, priv->hw,
				  priv->plat->tx_queues_cfg[queue].send_slope,
				  priv->plat->tx_queues_cfg[queue].idle_slope,
				  priv->plat->tx_queues_cfg[queue].high_credit,
				  priv->plat->tx_queues_cfg[queue].low_credit,
				  queue);
	}
}

/**
 *  sunxi_uio_rx_queue_dma_chan_map - Map RX queue to RX dma channel
 *  @priv: driver private structure
 *  Description: It is used for mapping RX queues to RX dma channels
 */
static void sunxi_uio_rx_queue_dma_chan_map(struct stmmac_priv *priv)
{
	u32 rx_queues_count = priv->plat->rx_queues_to_use;
	u32 queue, chan;

	for (queue = 0; queue < rx_queues_count; queue++) {
		chan = priv->plat->rx_queues_cfg[queue].chan;
		stmmac_map_mtl_to_dma(priv, priv->hw, queue, chan);
	}
}

/**
 *  sunxi_uio_mac_config_rx_queues_prio - Configure RX Queue priority
 *  @priv: driver private structure
 *  Description: It is used for configuring the RX Queue Priority
 */
static void sunxi_uio_mac_config_rx_queues_prio(struct stmmac_priv *priv)
{
	u32 rx_queues_count = priv->plat->rx_queues_to_use;
	u32 queue, prio;

	for (queue = 0; queue < rx_queues_count; queue++) {
		if (!priv->plat->rx_queues_cfg[queue].use_prio)
			continue;

		prio = priv->plat->rx_queues_cfg[queue].prio;
		stmmac_rx_queue_prio(priv, priv->hw, prio, queue);
	}
}

/**
 *  sunxi_uio_mac_config_tx_queues_prio - Configure TX Queue priority
 *  @priv: driver private structure
 *  Description: It is used for configuring the TX Queue Priority
 */
static void sunxi_uio_mac_config_tx_queues_prio(struct stmmac_priv *priv)
{
	u32 tx_queues_count = priv->plat->tx_queues_to_use;
	u32 queue, prio;

	for (queue = 0; queue < tx_queues_count; queue++) {
		if (!priv->plat->tx_queues_cfg[queue].use_prio)
			continue;

		prio = priv->plat->tx_queues_cfg[queue].prio;
		stmmac_tx_queue_prio(priv, priv->hw, prio, queue);
	}
}

/**
 *  sunxi_uio_mac_config_rx_queues_routing - Configure RX Queue Routing
 *  @priv: driver private structure
 *  Description: It is used for configuring the RX queue routing
 */
static void sunxi_uio_mac_config_rx_queues_routing(struct stmmac_priv *priv)
{
	u32 rx_queues_count = priv->plat->rx_queues_to_use;
	u32 queue;
	u8 packet;

	for (queue = 0; queue < rx_queues_count; queue++) {
		/* no specific packet type routing specified for the queue */
		if (priv->plat->rx_queues_cfg[queue].pkt_route == 0x0)
			continue;

		packet = priv->plat->rx_queues_cfg[queue].pkt_route;
		stmmac_rx_queue_routing(priv, priv->hw, packet, queue);
	}
}

static void sunxi_uio_mac_config_rss(struct stmmac_priv *priv)
{
	if (!priv->dma_cap.rssen || !priv->plat->rss_en) {
		priv->rss.enable = false;
		return;
	}

	if (priv->dev->features & NETIF_F_RXHASH)
		priv->rss.enable = true;
	else
		priv->rss.enable = false;

	stmmac_rss_configure(priv, priv->hw, &priv->rss,
			     priv->plat->rx_queues_to_use);
}

/**
 *  sunxi_uio_mac_enable_rx_queues - Enable MAC rx queues
 *  @priv: driver private structure
 *  Description: It is used for enabling the rx queues in the MAC
 */
static void sunxi_uio_mac_enable_rx_queues(struct stmmac_priv *priv)
{
	u32 rx_queues_count = priv->plat->rx_queues_to_use;
	int queue;
	u8 mode;

	for (queue = 0; queue < rx_queues_count; queue++) {
		mode = priv->plat->rx_queues_cfg[queue].mode_to_use;
		stmmac_rx_queue_enable(priv, priv->hw, mode, queue);
	}
}

/**
 *  sunxi_uio_mtl_configuration - Configure MTL
 *  @priv: driver private structure
 *  Description: It is used for configuring MTL
 */
static void sunxi_uio_mtl_configuration(struct stmmac_priv *priv)
{
	u32 rx_queues_count = priv->plat->rx_queues_to_use;
	u32 tx_queues_count = priv->plat->tx_queues_to_use;

	if (tx_queues_count > 1)
		sunxi_uio_set_tx_queue_weight(priv);

	/* Configure MTL RX algorithms */
	if (rx_queues_count > 1)
		stmmac_prog_mtl_rx_algorithms(priv, priv->hw,
				priv->plat->rx_sched_algorithm);

	/* Configure MTL TX algorithms */
	if (tx_queues_count > 1)
		stmmac_prog_mtl_tx_algorithms(priv, priv->hw,
				priv->plat->tx_sched_algorithm);

	/* Configure CBS in AVB TX queues */
	if (tx_queues_count > 1)
		sunxi_uio_configure_cbs(priv);

	/* Map RX MTL to DMA channels */
	sunxi_uio_rx_queue_dma_chan_map(priv);

	/* Enable MAC RX Queues */
	sunxi_uio_mac_enable_rx_queues(priv);

	/* Set RX priorities */
	if (rx_queues_count > 1)
		sunxi_uio_mac_config_rx_queues_prio(priv);

	/* Set TX priorities */
	if (tx_queues_count > 1)
		sunxi_uio_mac_config_tx_queues_prio(priv);

	/* Set RX routing */
	if (rx_queues_count > 1)
		sunxi_uio_mac_config_rx_queues_routing(priv);

	/* Receive Side Scaling */
	if (rx_queues_count > 1)
		sunxi_uio_mac_config_rss(priv);
}

static void sunxi_uio_safety_feat_configuration(struct stmmac_priv *priv)
{
	if (priv->dma_cap.asp) {
		netdev_info(priv->dev, "Enabling Safety Features\n");
#if (LINUX_VERSION_CODE < KERNEL_VERSION(5, 15, 0))
		stmmac_safety_feat_config(priv, priv->ioaddr, priv->dma_cap.asp);
#else
		stmmac_safety_feat_config(priv, priv->ioaddr, priv->dma_cap.asp,
				priv->plat->safety_feat_cfg);
#endif
	} else {
		netdev_info(priv->dev, "No Safety Features support found\n");
	}
}

/**
 *  sunxi_uio_dma_operation_mode - HW DMA operation mode
 *  @priv: driver private structure
 *  Description: it is used for configuring the DMA operation mode register in
 *  order to program the tx/rx DMA thresholds or Store-And-Forward mode.
 */
static void sunxi_uio_dma_operation_mode(struct stmmac_priv *priv)
{
	u32 rx_channels_count = priv->plat->rx_queues_to_use;
	u32 tx_channels_count = priv->plat->tx_queues_to_use;
	int rxfifosz = priv->plat->rx_fifo_size;
	int txfifosz = priv->plat->tx_fifo_size;
	u32 txmode = 0, rxmode = 0, chan = 0;
	u8 qmode = 0;

	if (rxfifosz == 0)
		rxfifosz = priv->dma_cap.rx_fifo_size;
	if (txfifosz == 0)
		txfifosz = priv->dma_cap.tx_fifo_size;

	/* Adjust for real per queue fifo size */
	rxfifosz /= rx_channels_count;
	txfifosz /= tx_channels_count;

	if (priv->plat->force_thresh_dma_mode) {
		txmode = tc;
		rxmode = tc;
	} else if (priv->plat->force_sf_dma_mode || priv->plat->tx_coe) {
		/* In case of GMAC, SF mode can be enabled
		 * to perform the TX COE in HW. This depends on:
		 * 1) TX COE if actually supported
		 * 2) There is no bugged Jumbo frame support
		 *    that needs to not insert csum in the TDES.
		 */
		txmode = SF_DMA_MODE;
		rxmode = SF_DMA_MODE;
		priv->xstats.threshold = SF_DMA_MODE;
	} else {
		txmode = tc;
		rxmode = SF_DMA_MODE;
	}

	/* configure all channels */
	for (chan = 0; chan < rx_channels_count; chan++) {
		qmode = priv->plat->rx_queues_cfg[chan].mode_to_use;

		stmmac_dma_rx_mode(priv, priv->ioaddr, rxmode, chan,
				   rxfifosz, qmode);
		stmmac_set_dma_bfsize(priv, priv->ioaddr, priv->dma_buf_sz,
				      chan);
	}

	for (chan = 0; chan < tx_channels_count; chan++) {
		qmode = priv->plat->tx_queues_cfg[chan].mode_to_use;

		stmmac_dma_tx_mode(priv, priv->ioaddr, txmode, chan,
				   txfifosz, qmode);
	}
}

/**
 *  sunxi_uio_hw_setup - setup mac in a usable state.
 *  @dev : pointer to the device structure.
 *  @init_ptp: initialize PTP if set
 *  Description:
 *  this is the main function to setup the HW in a usable state because the
 *  dma engine is reset, the core registers are configured (e.g. AXI,
 *  Checksum features, timers). The DMA is ready to start receiving and
 *  transmitting.
 *  Return value:
 *  0 on success and an appropriate (-)ve integer as defined in errno.h
 *  file on failure.
 */
static int sunxi_uio_hw_setup(struct net_device *dev, bool init_ptp)
{
	struct stmmac_priv *priv = netdev_priv(dev);
	int ret;

	/* DMA initialization and SW reset */
	ret = sunxi_uio_init_dma_engine(priv);
	if (ret < 0) {
		netdev_err(priv->dev, "%s: DMA engine initialization failed\n",
			   __func__);
		return ret;
	}

	/* Copy the MAC addr into the HW  */
	stmmac_set_umac_addr(priv, priv->hw, dev->dev_addr, 0);

	/* PS and related bits will be programmed according to the speed */
	if (priv->hw->pcs) {
		int speed = priv->plat->mac_port_sel_speed;

		if (speed == SPEED_10 || speed == SPEED_100 ||
		    speed == SPEED_1000) {
			priv->hw->ps = speed;
		} else {
			dev_warn(priv->device, "invalid port speed\n");
			priv->hw->ps = 0;
		}
	}

	/* Initialize the MAC Core */
	stmmac_core_init(priv, priv->hw, dev);

	/* Initialize MTL*/
	sunxi_uio_mtl_configuration(priv);

	/* Initialize Safety Features */
	sunxi_uio_safety_feat_configuration(priv);

	ret = stmmac_rx_ipc(priv, priv->hw);
	if (!ret) {
		netdev_warn(priv->dev, "RX IPC Checksum Offload disabled\n");
		priv->plat->rx_coe = STMMAC_RX_COE_NONE;
		priv->hw->rx_csum = 0;
	}

	/* Enable the MAC Rx/Tx */
	stmmac_mac_set(priv, priv->ioaddr, true);

	/* Set the HW DMA mode and the COE */
	sunxi_uio_dma_operation_mode(priv);

	if (priv->hw->pcs)
		stmmac_pcs_ctrl_ane(priv, priv->hw, 1, priv->hw->ps, 0);

	/* set TX and RX rings length */
	sunxi_uio_set_rings_length(priv);

	return 0;
}

static int sunxi_uio_set_bfsize(int mtu, int bufsize)
{
	int ret = bufsize;

	if (mtu >= BUF_SIZE_8KiB)
		ret = BUF_SIZE_16KiB;
	else if (mtu >= BUF_SIZE_4KiB)
		ret = BUF_SIZE_8KiB;
	else if (mtu >= BUF_SIZE_2KiB)
		ret = BUF_SIZE_4KiB;
	else if (mtu > DEFAULT_BUFSIZE)
		ret = BUF_SIZE_2KiB;
	else
		ret = DEFAULT_BUFSIZE;

	return ret;
}

/**
 *  sunxi_uio_init - open entry point of the driver
 *  @dev : pointer to the device structure.
 *  Description:
 *  This function is the open entry point of the driver.
 *  Return value:
 *  0 on success and an appropriate (-)ve integer as defined in errno.h
 *  file on failure.
 */
static int sunxi_uio_init(struct net_device *dev)
{
	struct stmmac_priv *priv = netdev_priv(dev);
	int ret, bfsize = 0;

	if (priv->hw->pcs != STMMAC_PCS_TBI &&
	    priv->hw->pcs != STMMAC_PCS_RTBI &&
	    !priv->hw->xpcs) {
		ret = sunxi_uio_init_phy(dev);
		if (ret) {
			netdev_err(priv->dev,
				   "%s: Cannot attach to PHY (error: %d)\n",
				   __func__, ret);
			return ret;
		}
	}

	/* Extra statistics */
	priv->xstats.threshold = tc;

	bfsize = stmmac_set_16kib_bfsize(priv, dev->mtu);
	if (bfsize < 0)
		bfsize = 0;

	if (bfsize < BUF_SIZE_16KiB)
		bfsize = sunxi_uio_set_bfsize(dev->mtu, priv->dma_buf_sz);

	priv->dma_buf_sz = bfsize;
	buf_sz = bfsize;

	priv->rx_copybreak = STMMAC_RX_COPYBREAK;

	if (!priv->dma_tx_size)
		priv->dma_tx_size = DMA_DEFAULT_TX_SIZE;
	if (!priv->dma_rx_size)
		priv->dma_rx_size = DMA_DEFAULT_RX_SIZE;

	ret = sunxi_uio_alloc_dma_desc_resources(priv);
	if (ret < 0) {
		netdev_err(priv->dev, "%s: DMA descriptors allocation failed\n",
			   __func__);
		goto dma_desc_error;
	}

	ret = sunxi_uio_hw_setup(dev, true);
	if (ret < 0) {
		netdev_err(priv->dev, "%s: Hw setup failed\n", __func__);
		goto init_error;
	}

	phylink_start(priv->phylink);
	/* We may have called phylink_speed_down before */
	phylink_speed_up(priv->phylink);

	return 0;

init_error:
	sunxi_uio_free_dma_desc_resources(priv);
dma_desc_error:
	phylink_disconnect_phy(priv->phylink);
	return ret;
}

/**
 *  sunxi_uio_exit - close entry point of the driver
 *  @dev : device pointer.
 *  Description:
 *  This is the stop entry point of the driver.
 */
static int sunxi_uio_exit(struct net_device *dev)
{
	struct stmmac_priv *priv = netdev_priv(dev);

	/* Stop and disconnect the PHY */
	if (dev->phydev) {
		phy_stop(dev->phydev);
		phy_disconnect(dev->phydev);
	}

	/* Release and free the Rx/Tx resources */
	sunxi_uio_free_dma_desc_resources(priv);

	/* Disable the MAC Rx/Tx */
	stmmac_mac_set(priv, priv->ioaddr, false);

	netif_carrier_off(dev);

	return 0;
}

/**
 * sunxi_uio_probe() platform driver probe routine
 * - register uio devices filled with memory maps retrieved
 * from device tree
 */
static int sunxi_uio_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct device_node *np = dev->of_node, *mac_node;
	struct sunxi_uio *chip;
	struct net_device *netdev;
	struct stmmac_priv *priv;
	struct uio_info *uio;
	struct resource *res;
	int err = 0;

	chip = devm_kzalloc(dev, sizeof(struct sunxi_uio),
				 GFP_KERNEL);
	if (!chip)
		return -ENOMEM;

	uio = &chip->uio;
	chip->dev = dev;
	mac_node = of_parse_phandle(np, "sunxi,ethernet", 0);
	if (!mac_node)
		return -ENODEV;

	if (of_device_is_available(mac_node)) {
		netdev = of_find_net_device_by_node(mac_node);
		of_node_put(mac_node);
		if (!netdev)
			return -ENODEV;
	} else {
		of_node_put(mac_node);
		return -EINVAL;
	}

	chip->ndev = netdev;
	rtnl_lock();
	dev_close(netdev);
	rtnl_unlock();

	rtnl_lock();
	err = sunxi_uio_init(netdev);
	if (err) {
		rtnl_unlock();
		dev_err(dev, "Failed to open stmmac resource: %d\n", err);
		return err;
	}
	rtnl_unlock();

	priv = netdev_priv(netdev);
	snprintf(chip->name, sizeof(chip->name), "uio_%s",
		 netdev->name);
	uio->name = chip->name;
	uio->version = DRIVER_VERSION;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res)
		return -ENODEV;

	uio->mem[0].name = "eth_regs";
	uio->mem[0].addr = res->start & PAGE_MASK;
	uio->mem[0].size = PAGE_ALIGN(resource_size(res));
	uio->mem[0].memtype = UIO_MEM_PHYS;

	uio->mem[1].name = "eth_rx_bd";
	uio->mem[1].addr = priv->rx_queue[0].dma_rx_phy;
	uio->mem[1].size = priv->dma_rx_size * sizeof(struct dma_desc);
	uio->mem[1].memtype = UIO_MEM_PHYS;

	uio->mem[2].name = "eth_tx_bd";
	uio->mem[2].addr = priv->tx_queue[0].dma_tx_phy;
	uio->mem[2].size = priv->dma_tx_size * sizeof(struct dma_desc);
	uio->mem[2].memtype = UIO_MEM_PHYS;

	uio->open = sunxi_uio_open;
	uio->release = sunxi_uio_release;
	/* Custom mmap function. */
	uio->mmap = sunxi_uio_mmap;
	uio->priv = chip;

	err = uio_register_device(dev, uio);
	if (err) {
		dev_err(dev, "Failed to register uio device: %d\n", err);
		return err;
	}

	chip->map_num = 3;

	dev_info(dev, "Registered %s uio devices, %d register maps attached\n",
		 chip->name, chip->map_num);

	platform_set_drvdata(pdev, chip);

	return 0;
}

/**
 * sunxi_uio_remove() - UIO platform driver release
 * routine - unregister uio devices
 */
static int sunxi_uio_remove(struct platform_device *pdev)
{
	struct sunxi_uio *chip = platform_get_drvdata(pdev);
	struct net_device *netdev;

	if (!chip)
		return -EINVAL;

	netdev = chip->ndev;

	uio_unregister_device(&chip->uio);

	if (netdev) {
		rtnl_lock();
		sunxi_uio_exit(netdev);
		rtnl_unlock();
	}

	platform_set_drvdata(pdev, NULL);

	if (netdev) {
		rtnl_lock();
		dev_open(netdev, NULL);
		rtnl_unlock();
	}

	return 0;
}

static const struct of_device_id sunxi_uio_of_match[] = {
	{ .compatible	= "allwinner,sunxi-uio", },
	{ }
};

static struct platform_driver sunxi_uio_driver = {
	.driver = {
		.owner		= THIS_MODULE,
		.name		= DRIVER_NAME,
		.of_match_table	= sunxi_uio_of_match,
	},
	.probe	= sunxi_uio_probe,
	.remove	= sunxi_uio_remove,
};
module_platform_driver(sunxi_uio_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("xuminghui <xuminghui@allwinnertech.com>");
MODULE_VERSION(DRIVER_VERSION);
