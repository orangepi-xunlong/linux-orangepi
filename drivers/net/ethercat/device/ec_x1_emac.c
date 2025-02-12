// SPDX-License-Identifier: GPL-2.0
/*
 * ky x1 emac driver
 *
 * Copyright (c) 2023, ky Corporation.
 *
 */

#include <linux/bitops.h>
#include <linux/clk.h>
#include <linux/crc32.h>
#include <linux/delay.h>
#include <linux/dma-mapping.h>
#include <linux/etherdevice.h>
#include <linux/dma-direct.h>
#include <linux/in.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/iopoll.h>
#include <linux/ip.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_gpio.h>
#include <linux/of_net.h>
#include <linux/of_mdio.h>
#include <linux/of_irq.h>
#include <linux/phy.h>
#include <linux/phylink.h>
#include <linux/platform_device.h>
#include <linux/pm_runtime.h>
#include <linux/pm.h>
#include <linux/tcp.h>
#include <linux/timer.h>
#include <linux/types.h>
#include <linux/udp.h>
#include <linux/workqueue.h>
#include <linux/reset.h>

#include "ec_x1_emac.h"

#define DRIVER_NAME				"x1_ec_emac"

/* x1 PMUap base */
#define PMUA_BASE_REG		0xd4282800

#define TUNING_CMD_LEN				50
#define CLK_PHASE_CNT				256
#define CLK_PHASE_REVERT			180

#define TXCLK_PHASE_DEFAULT			0
#define RXCLK_PHASE_DEFAULT			0

#define TX_PHASE				1
#define RX_PHASE				0

#define DEFAULT_TX_THRESHOLD			(192)
#define DEFAULT_RX_THRESHOLD			(12)
#define DEFAULT_TX_RING_NUM			(128)
#define DEFAULT_RX_RING_NUM			(128)
#define DEFAULT_DMA_BURST_LEN			(1)
#define HASH_TABLE_SIZE				(64)

#define EMAC_DMA_REG_CNT			16
#define EMAC_MAC_REG_CNT			124
#define EMAC_REG_SPACE_SIZE			((EMAC_DMA_REG_CNT + \
						 EMAC_MAC_REG_CNT) * 4)

enum clk_tuning_way {
	/* fpga clk tuning register */
	CLK_TUNING_BY_REG,
	/* zebu/evb rgmii delayline register */
	CLK_TUNING_BY_DLINE,
	/* evb rmii only revert tx/rx clock for clk tuning */
	CLK_TUNING_BY_CLK_REVERT,
	CLK_TUNING_MAX,
};

static int emac_open(struct net_device *ndev);
static int emac_close(struct net_device *ndev);
static int emac_start_xmit(struct sk_buff *skb, struct net_device *ndev);
static void emac_clean_tx_desc_ring(struct emac_priv *priv);
static void emac_clean_rx_desc_ring(struct emac_priv *priv);
static void emac_configure_tx(struct emac_priv *priv);
static void emac_configure_rx(struct emac_priv *priv);
static int emac_tx_mem_map(struct emac_priv *priv, struct sk_buff *skb, u32 max_tx_len,	u32 frag_num);
static int emac_tx_clean_desc(struct emac_priv *priv);
static int emac_rx_clean_desc(struct emac_priv *priv, int budget);
static void emac_alloc_rx_desc_buffers(struct emac_priv *priv);
static int emac_phy_connect(struct net_device *dev);
static int emac_sw_init(struct emac_priv *priv);

static bool emac_is_rmii(struct emac_priv *priv)
{
	return priv->phy_interface == PHY_INTERFACE_MODE_RMII;
}

static void emac_enable_axi_single_id_mode(struct emac_priv *priv, int en)
{
	u32 val;

	val = readl(priv->ctrl_reg);
	if (en)
		val |= AXI_SINGLE_ID;
	else
		val &= ~AXI_SINGLE_ID;
	writel(val, priv->ctrl_reg);
}

static void emac_phy_interface_config(struct emac_priv *priv)
{
	u32 val;

	val = readl(priv->ctrl_reg);
	if (emac_is_rmii(priv)) {
		val &= ~PHY_INTF_RGMII;
		if (priv->ref_clk_frm_soc)
			val |= REF_CLK_SEL;
		else
			val &= ~REF_CLK_SEL;
	} else {
		val |= PHY_INTF_RGMII;
		if (priv->ref_clk_frm_soc)
			val |= RGMII_TX_CLK_SEL;
	}
	writel(val, priv->ctrl_reg);
}

/* Name		emac_reset_hw
 * Arguments	priv : pointer to hardware data structure
 * Return	Status: 0 - Success;  non-zero - Fail
 * Description	TBDL
 */
static int emac_reset_hw(struct emac_priv *priv)
{
	/* disable all the interrupts */
	emac_wr(priv, MAC_INTERRUPT_ENABLE, 0x0000);
	emac_wr(priv, DMA_INTERRUPT_ENABLE, 0x0000);

	/* disable transmit and receive units */
	emac_wr(priv, MAC_RECEIVE_CONTROL, 0x0000);
	emac_wr(priv, MAC_TRANSMIT_CONTROL, 0x0000);

	/* stop the DMA */
	emac_wr(priv, DMA_CONTROL, 0x0000);

	/* reset mac, statistic counters */
	emac_wr(priv, MAC_GLOBAL_CONTROL, 0x0018);

	emac_wr(priv, MAC_GLOBAL_CONTROL, 0x0000);
	return 0;
}

/* Name		emac_init_hw
 * Arguments	pstHWData	: pointer to hardware data structure
 * Return	Status: 0 - Success;  non-zero - Fail
 * Description	TBDL
 * Assumes that the controller has previously been reset
 * and is in apost-reset uninitialized state.
 * Initializes the receive address registers,
 * multicast table, and VLAN filter table.
 * Calls routines to setup link
 * configuration and flow control settings.
 * Clears all on-chip counters. Leaves
 * the transmit and receive units disabled and uninitialized.
 */
static int emac_init_hw(struct emac_priv *priv)
{
	u32 val = 0;

	emac_enable_axi_single_id_mode(priv, 1);

	/* MAC Init
	 * disable transmit and receive units
	 */
	emac_wr(priv, MAC_RECEIVE_CONTROL, 0x0000);
	emac_wr(priv, MAC_TRANSMIT_CONTROL, 0x0000);

	/* enable mac address 1 filtering */
	emac_wr(priv, MAC_ADDRESS_CONTROL, MREGBIT_MAC_ADDRESS1_ENABLE);

	/* zero initialize the multicast hash table */
	emac_wr(priv, MAC_MULTICAST_HASH_TABLE1, 0x0);
	emac_wr(priv, MAC_MULTICAST_HASH_TABLE2, 0x0);
	emac_wr(priv, MAC_MULTICAST_HASH_TABLE3, 0x0);
	emac_wr(priv, MAC_MULTICAST_HASH_TABLE4, 0x0);

	emac_wr(priv, MAC_TRANSMIT_FIFO_ALMOST_FULL, 0x1f8);

	emac_wr(priv, MAC_TRANSMIT_PACKET_START_THRESHOLD, priv->tx_threshold);

	emac_wr(priv, MAC_RECEIVE_PACKET_START_THRESHOLD, priv->rx_threshold);

	/* set emac rx mitigation frame count */
	val = EMAC_RX_FRAMES & MREGBIT_RECEIVE_IRQ_FRAME_COUNTER_MSK;

	/* set emac rx mitigation timeout */
	val |= (EMAC_RX_COAL_TIMEOUT << MREGBIT_RECEIVE_IRQ_TIMEOUT_COUNTER_OFST) &
		MREGBIT_RECEIVE_IRQ_TIMEOUT_COUNTER_MSK;

	/* disable emac rx irq mitigation */
	val &= ~MRGEBIT_RECEIVE_IRQ_MITIGATION_ENABLE;

	emac_wr(priv, DMA_RECEIVE_IRQ_MITIGATION_CTRL, val);

	/* reset dma */
	emac_wr(priv, DMA_CONTROL, 0x0000);

	emac_wr(priv, DMA_CONFIGURATION, 0x01);
	usleep_range(9000, 10000);
	emac_wr(priv, DMA_CONFIGURATION, 0x00);
	usleep_range(9000, 10000);

	val = 0;
	val |= MREGBIT_STRICT_BURST;
	val |= MREGBIT_DMA_64BIT_MODE;

	if (priv->dma_burst_len)
		val |= 1 << priv->dma_burst_len;
	else
		val |= MREGBIT_BURST_1WORD;

	emac_wr(priv, DMA_CONFIGURATION, val);

	return 0;
}

static int emac_set_mac_addr(struct emac_priv *priv, const unsigned char *addr)
{
	emac_wr(priv, MAC_ADDRESS1_HIGH, ((addr[1] << 8) | addr[0]));
	emac_wr(priv, MAC_ADDRESS1_MED, ((addr[3] << 8) | addr[2]));
	emac_wr(priv, MAC_ADDRESS1_LOW, ((addr[5] << 8) | addr[4]));

	return 0;
}

static void emac_dma_start_transmit(struct emac_priv *priv)
{
	emac_wr(priv, DMA_TRANSMIT_POLL_DEMAND, 0xFF);
}

static inline u32 emac_tx_avail(struct emac_priv *priv)
{
	struct emac_desc_ring *tx_ring = &priv->tx_ring;
	u32 avail;

	if (tx_ring->tail > tx_ring->head)
		avail = tx_ring->tail - tx_ring->head - 1;
	else
		avail = tx_ring->total_cnt - tx_ring->head + tx_ring->tail - 1;

	return avail;
}

/* Name		emac_sw_init
 * Arguments	priv	: pointer to driver private data structure
 * Return	Status: 0 - Success;  non-zero - Fail
 * Description	Reads PCI space configuration information and
 *		initializes the variables with
 *		their default values
 */
static int emac_sw_init(struct emac_priv *priv)
{
	priv->dma_buf_sz = EMAC_DEFAULT_BUFSIZE;

	priv->tx_ring.total_cnt = priv->tx_ring_num;
	priv->rx_ring.total_cnt = priv->rx_ring_num;
	return 0;
}

/* Name		emac_configure_tx
 * Arguments	priv : pointer to driver private data structure
 * Return	none
 * Description	Configures the transmit unit of the device
 */
static void emac_configure_tx(struct emac_priv *priv)
{
	u32 val;

	/* set the transmit base address */
	val = (u32)(priv->tx_ring.desc_dma_addr);

	emac_wr(priv, DMA_TRANSMIT_BASE_ADDRESS, val);

	/* Tx Inter Packet Gap value and enable the transmit */
	val = emac_rd(priv, MAC_TRANSMIT_CONTROL);
	val &= (~MREGBIT_IFG_LEN);
	val |= MREGBIT_TRANSMIT_ENABLE;
	val |= MREGBIT_TRANSMIT_AUTO_RETRY;
	emac_wr(priv, MAC_TRANSMIT_CONTROL, val);

	emac_wr(priv, DMA_TRANSMIT_AUTO_POLL_COUNTER, 0x00);

	/* start tx dma */
	val = emac_rd(priv, DMA_CONTROL);
	val |= MREGBIT_START_STOP_TRANSMIT_DMA;
	emac_wr(priv, DMA_CONTROL, val);
}

/* Name		emac_configure_rx
 * Arguments	priv : pointer to driver private data structure
 * Return	none
 * Description	Configures the receive unit of the device
 */
static void emac_configure_rx(struct emac_priv *priv)
{
	u32 val;

	/* set the receive base address */
	val = (u32)(priv->rx_ring.desc_dma_addr);
	emac_wr(priv, DMA_RECEIVE_BASE_ADDRESS, val);

	/* enable the receive */
	val = emac_rd(priv, MAC_RECEIVE_CONTROL);
	val |= MREGBIT_RECEIVE_ENABLE;
	val |= MREGBIT_STORE_FORWARD;
	emac_wr(priv, MAC_RECEIVE_CONTROL, val);

	/* start rx dma */
	val = emac_rd(priv, DMA_CONTROL);
	val |= MREGBIT_START_STOP_RECEIVE_DMA;
	emac_wr(priv, DMA_CONTROL, val);
}

/* Name		emac_free_tx_buf
 * Arguments	priv : pointer to driver private data structure
 * 		i: ring idx
 * Return	0 - Success;
 * Description	Freeing the TX buffer data.
 */
static int emac_free_tx_buf(struct emac_priv *priv, int i)
{
	struct emac_desc_ring *tx_ring;
	struct emac_tx_desc_buffer *tx_buf;
	struct desc_buf *buf;
	int j;

	tx_ring = &priv->tx_ring;
	tx_buf = &tx_ring->tx_desc_buf[i];

	for (j = 0; j < 2; j++) {
		buf = &tx_buf->buf[j];
		if (buf->dma_addr) {
			if (buf->map_as_page)
				dma_unmap_page(&priv->pdev->dev, buf->dma_addr,
					       buf->dma_len, DMA_TO_DEVICE);
			else
				dma_unmap_single(&priv->pdev->dev, buf->dma_addr,
						 buf->dma_len, DMA_TO_DEVICE);

			buf->dma_addr = 0;
			buf->map_as_page = false;
			buf->buff_addr = NULL;
		}
	}
	return 0;
}

/* Name		emac_clean_tx_desc_ring
 * Arguments	priv : pointer to driver private data structure
 * Return	none
 * Description	Freeing the TX resources allocated earlier.
 */
static void emac_clean_tx_desc_ring(struct emac_priv *priv)
{
	struct emac_desc_ring *tx_ring = &priv->tx_ring;
	u32 i;

	/* Free all the Tx ring sk_buffs */
	for (i = 0; i < tx_ring->total_cnt; i++)
		emac_free_tx_buf(priv, i);

	tx_ring->head = 0;
	tx_ring->tail = 0;
}

/* Name		emac_clean_rx_desc_ring
 * Arguments	priv : pointer to driver private data structure
 * Return	none
 * Description	Freeing the RX resources allocated earlier.
 */
static void emac_clean_rx_desc_ring(struct emac_priv *priv)
{
	struct emac_desc_ring *rx_ring;
	struct emac_desc_buffer *rx_buf;
	u32 i;

	rx_ring = &priv->rx_ring;

	/* Free all the Rx ring sk_buffs */
	for (i = 0; i < rx_ring->total_cnt; i++) {
		rx_buf = &rx_ring->desc_buf[i];
		if (rx_buf->skb) {
			dma_unmap_single(&priv->pdev->dev, rx_buf->dma_addr,
					 rx_buf->dma_len, DMA_FROM_DEVICE);

			dev_kfree_skb(rx_buf->skb);
			rx_buf->skb = NULL;
		}
	}

	rx_ring->tail = 0;
	rx_ring->head = 0;
}

/* Name		emac_up
 * Arguments	priv : pointer to driver private data structure
 * Return	Status: 0 - Success;  non-zero - Fail
 * Description	This function is called from emac_open and
 *		performs the things when net interface is about to up.
 *		It configues the Tx and Rx unit of the device and
 *		registers interrupt handler.
 *		It also starts one watchdog timer to monitor
 *		the net interface link status.
 */
static int emac_up(struct emac_priv *priv)
{
	struct net_device *ndev = priv->ndev;
	int ret;

	ret = emac_phy_connect(ndev);
	if (ret) {
		pr_err("%s  phy_connet failed\n", __func__);
		goto err;
	}
	/* init hardware */
	emac_init_hw(priv);

	emac_set_mac_addr(priv, ndev->dev_addr);
	/* configure transmit unit */
	emac_configure_tx(priv);
	/* configure rx unit */
	emac_configure_rx(priv);

	/* allocate buffers for receive descriptors */
	emac_alloc_rx_desc_buffers(priv);

	if (ndev->phydev)
		phy_start(ndev->phydev);

	emac_wr(priv, MAC_INTERRUPT_ENABLE, 0x0000);

	return 0;
err:
	return ret;
}

/* Name		emac_down
 * Arguments	priv : pointer to driver private data structure
 * Return	Status: 0 - Success;  non-zero - Fail
 * Description	This function is called from emac_close and
 *		performs the things when net interface is about to down.
 *		It frees the irq, removes the various timers.
 *		It sets the net interface off and
 *		resets the hardware. Cleans the Tx and Rx
 *		ring descriptor.
 */
static int emac_down(struct emac_priv *priv)
{
	struct net_device *ndev = priv->ndev;

	/* Stop and disconnect the PHY */
	if (ndev->phydev) {
		phy_stop(ndev->phydev);
		phy_disconnect(ndev->phydev);
	}

	ecdev_set_link(priv->ecdev, 0);
	priv->link = false;
	priv->duplex = DUPLEX_UNKNOWN;
	priv->speed = SPEED_UNKNOWN;

	emac_reset_hw(priv);

	return 0;
}

/* Name		emac_alloc_tx_resources
 * Arguments	priv : pointer to driver private data structure
 * Return	Status: 0 - Success;  non-zero - Fail
 * Description	Allocates TX resources and getting virtual & physical address.
 */
static int emac_alloc_tx_resources(struct emac_priv *priv)
{
	struct emac_desc_ring *tx_ring = &priv->tx_ring;
	struct platform_device *pdev  = priv->pdev;
	u32 size;

	size = sizeof(struct emac_tx_desc_buffer) * tx_ring->total_cnt;

	/* allocate memory */
	tx_ring->tx_desc_buf = kzalloc(size, GFP_KERNEL);
	if (!tx_ring->tx_desc_buf) {
		pr_err("Memory allocation failed for the Transmit descriptor buffer\n");
		return -ENOMEM;
	}

	memset(tx_ring->tx_desc_buf, 0, size);

	tx_ring->total_size = tx_ring->total_cnt * sizeof(struct emac_tx_desc);

	EMAC_ROUNDUP(tx_ring->total_size, 1024);

	tx_ring->desc_addr = dma_alloc_coherent(&pdev->dev,
							tx_ring->total_size,
							&tx_ring->desc_dma_addr,
							GFP_KERNEL);
	if (!tx_ring->desc_addr) {
		pr_err("Memory allocation failed for the Transmit descriptor ring\n");
		kfree(tx_ring->tx_desc_buf);
		return -ENOMEM;
	}

	memset(tx_ring->desc_addr, 0, tx_ring->total_size);

	tx_ring->head = 0;
	tx_ring->tail = 0;

	return 0;
}

/* Name		emac_alloc_rx_resources
 * Arguments	priv	: pointer to driver private data structure
 * Return	Status: 0 - Success;  non-zero - Fail
 * Description	Allocates RX resources and getting virtual & physical address.
 */
static int emac_alloc_rx_resources(struct emac_priv *priv)
{
	struct emac_desc_ring *rx_ring = &priv->rx_ring;
	struct platform_device *pdev  = priv->pdev;
	u32 buf_len;

	buf_len = sizeof(struct emac_desc_buffer) * rx_ring->total_cnt;

	rx_ring->desc_buf = kzalloc(buf_len, GFP_KERNEL);
	if (!rx_ring->desc_buf) {
		pr_err("Memory allocation failed for the Receive descriptor buffer\n");
		return -ENOMEM;
	}

	memset(rx_ring->desc_buf, 0, buf_len);

	/* round up to nearest 4K */
	rx_ring->total_size = rx_ring->total_cnt * sizeof(struct emac_rx_desc);

	EMAC_ROUNDUP(rx_ring->total_size, 1024);

	rx_ring->desc_addr = dma_alloc_coherent(&pdev->dev,
							rx_ring->total_size,
							&rx_ring->desc_dma_addr,
							GFP_KERNEL);
	if (!rx_ring->desc_addr) {
		pr_err("Memory allocation failed for the Receive descriptor ring\n");
		kfree(rx_ring->desc_buf);
		return -ENOMEM;
	}

	memset(rx_ring->desc_addr, 0, rx_ring->total_size);

	rx_ring->head = 0;
	rx_ring->tail = 0;

	return 0;
}

/* Name		emac_free_tx_resources
 * Arguments	priv : pointer to driver private data structure
 * Return	none
 * Description	Frees the Tx resources allocated
 */
static void emac_free_tx_resources(struct emac_priv *priv)
{
	emac_clean_tx_desc_ring(priv);
	kfree(priv->tx_ring.tx_desc_buf);
	priv->tx_ring.tx_desc_buf = NULL;
	dma_free_coherent(&priv->pdev->dev, priv->tx_ring.total_size,
				priv->tx_ring.desc_addr,
				priv->tx_ring.desc_dma_addr);
	priv->tx_ring.desc_addr = NULL;
}

/* Name		emac_free_rx_resources
 * Arguments	priv : pointer to driver private data structure
 * Return	none
 * Description	Frees the Rx resources allocated
 */
static void emac_free_rx_resources(struct emac_priv *priv)
{
	emac_clean_rx_desc_ring(priv);
	kfree(priv->rx_ring.desc_buf);
	priv->rx_ring.desc_buf = NULL;
	dma_free_coherent(&priv->pdev->dev, priv->rx_ring.total_size,
				priv->rx_ring.desc_addr,
				priv->rx_ring.desc_dma_addr);
	priv->rx_ring.desc_addr = NULL;
}

/* Name		emac_open
 * Arguments	pstNetdev : pointer to net_device structure
 * Return	Status: 0 - Success;  non-zero - Fail
 * Description	This function is called when net interface is made up.
 *		Setting up Tx and Rx
 *		resources and making the interface up.
 */
static int emac_open(struct net_device *ndev)
{
	struct emac_priv *priv = netdev_priv(ndev);
	int ret;

	ret = emac_alloc_tx_resources(priv);
	if (ret) {
		pr_err("Error in setting up the Tx resources\n");
		goto emac_alloc_tx_resource_fail;
	}

	ret = emac_alloc_rx_resources(priv);
	if (ret) {
		pr_err("Error in setting up the Rx resources\n");
		goto emac_alloc_rx_resource_fail;
	}

	ret = emac_up(priv);
	if (ret) {
		pr_err("Error in making the net intrface up\n");
		goto emac_up_fail;
	}
	return 0;

emac_up_fail:
	emac_free_rx_resources(priv);
emac_alloc_rx_resource_fail:
	emac_free_tx_resources(priv);
emac_alloc_tx_resource_fail:
	return ret;
}

/* Name		emac_close
 * Arguments	pstNetdev : pointer to net_device structure
 * Return	Status: 0 - Success;  non-zero - Fail
 * Description	This function is called when net interface is made down.
 *		It calls the appropriate functions to
 *		free Tx and Rx resources.
 */
static int emac_close(struct net_device *ndev)
{
	struct emac_priv *priv = netdev_priv(ndev);

	emac_down(priv);
	emac_free_tx_resources(priv);
	emac_free_rx_resources(priv);

	return 0;
}

/* Name		emac_tx_clean_desc
 * Arguments	priv : pointer to driver private data structure
 * Return	1: Cleaned; 0:Failed
 * Description
 */
static int emac_tx_clean_desc(struct emac_priv *priv)
{
	struct emac_desc_ring *tx_ring;
	struct emac_tx_desc_buffer *tx_buf;
	struct emac_tx_desc *tx_desc;

	u32 i;

	tx_ring = &priv->tx_ring;
	i = tx_ring->tail;

	while (i != tx_ring->head) {
		tx_desc = &((struct emac_tx_desc *)tx_ring->desc_addr)[i];

		/* if desc still own by dma, so we quit it */
		if (tx_desc->OWN)
			break;

		tx_buf = &tx_ring->tx_desc_buf[i];

		emac_free_tx_buf(priv, i);
		memset(tx_desc, 0, sizeof(struct emac_tx_desc));

		if (++i == tx_ring->total_cnt)
			i = 0;
	}

	tx_ring->tail = i;

	return 0;
}

static int emac_rx_frame_status(struct emac_priv *priv, struct emac_rx_desc *dsc)
{
	/* if last descritpor isn't set, so we drop it*/
	if (!dsc->LastDescriptor) {
		netdev_dbg(priv->ndev, "rx LD bit isn't set, drop it.\n");
		return frame_discard;
	}

	/*
	 * A Frame that is less than 64-bytes (from DA thru the FCS field)
	 * is considered as Runt Frame.
	 * Most of the Runt Frames happen because of collisions.
	 */
	if (dsc->ApplicationStatus & EMAC_RX_FRAME_RUNT) {
		netdev_dbg(priv->ndev, "rx frame less than 64.\n");
		return frame_discard;
	}

	/*
	 * When the frame fails the CRC check,
	 * the frame is assumed to have the CRC error
	 */
	if (dsc->ApplicationStatus & EMAC_RX_FRAME_CRC_ERR) {
		netdev_dbg(priv->ndev, "rx frame crc error\n");
		return frame_discard;
	}

	/*
	 * When the length of the frame exceeds
	 * the Programmed Max Frame Length
	 */
	if (dsc->ApplicationStatus & EMAC_RX_FRAME_MAX_LEN_ERR) {
		netdev_dbg(priv->ndev, "rx frame too long\n");
		return frame_discard;
	}

	/*
	 * frame reception is truncated at that point and
	 * frame is considered to have Jabber Error
	 */
	if (dsc->ApplicationStatus & EMAC_RX_FRAME_JABBER_ERR) {
		netdev_dbg(priv->ndev, "rx frame has been truncated\n");
		return frame_discard;
	}

	/* this bit is only for 802.3 Type Frames */
	if (dsc->ApplicationStatus & EMAC_RX_FRAME_LENGTH_ERR) {
		netdev_dbg(priv->ndev, "rx frame length err for 802.3\n");
		return frame_discard;
	}

	if (dsc->FramePacketLength <= ETHERNET_FCS_SIZE ||
	    dsc->FramePacketLength > priv->dma_buf_sz) {
		netdev_dbg(priv->ndev, "rx frame len too small or too long\n");
		return frame_discard;
	}
	return frame_ok;
}

/* Name		emac_rx_clean_desc
 * Arguments	priv : pointer to driver private data structure
 * Return	1: Cleaned; 0:Failed
 * Description
 */
static int emac_rx_clean_desc(struct emac_priv *priv, int budget)
{
	struct emac_desc_ring *rx_ring;
	struct emac_desc_buffer *rx_buf;
	struct emac_rx_desc *rx_desc;
	struct sk_buff *skb = NULL;
	int status;
	u32 receive_packet = 0;
	u32 i;
	u32 skb_len;

	rx_ring = &priv->rx_ring;

	i = rx_ring->tail;

	while (budget--) {
		/* get rx desc */
		rx_desc = &((struct emac_rx_desc *)rx_ring->desc_addr)[i];

		/* if rx_desc still owned by DMA, so we need to wait */
		if (rx_desc->OWN)
			break;

		rx_buf = &rx_ring->desc_buf[i];
		if (!rx_buf->skb)
			break;

		receive_packet++;

		dma_unmap_single(&priv->pdev->dev, rx_buf->dma_addr,
				 rx_buf->dma_len, DMA_FROM_DEVICE);

		status = emac_rx_frame_status(priv, rx_desc);
		if (unlikely(status == frame_discard)) {
			dev_kfree_skb_irq(rx_buf->skb);
			rx_buf->skb = NULL;
		} else {
			skb = rx_buf->skb;
			skb_len = rx_desc->FramePacketLength - ETHERNET_FCS_SIZE;
			ecdev_receive(priv->ecdev,skb->data,skb_len);
			dev_kfree_skb_irq(rx_buf->skb);
			rx_buf->skb = NULL;	
		}

		if (++i == rx_ring->total_cnt)
			i = 0;
	}

	rx_ring->tail = i;
	emac_alloc_rx_desc_buffers(priv);
	return receive_packet;
}

/* Name		emac_alloc_rx_desc_buffers
 * Arguments	priv : pointer to driver private data structure
 * Return	1: Cleaned; 0:Failed
 * Description
 */
static void emac_alloc_rx_desc_buffers(struct emac_priv *priv)
{
	struct net_device *ndev = priv->ndev;
	struct emac_desc_ring *rx_ring = &priv->rx_ring;
	struct emac_desc_buffer *rx_buf;
	struct sk_buff *skb;
	struct emac_rx_desc *rx_desc;
	u32 i;

	i = rx_ring->head;
	rx_buf = &rx_ring->desc_buf[i];

	while (!rx_buf->skb) {
		skb = netdev_alloc_skb_ip_align(ndev, priv->dma_buf_sz);
		if (!skb) {
			pr_err("sk_buff allocation failed\n");
			break;
		}

		skb->dev = ndev;

		rx_buf->skb = skb;
		rx_buf->dma_len = priv->dma_buf_sz;
		rx_buf->dma_addr = dma_map_single(&priv->pdev->dev,
						  skb->data,
						  priv->dma_buf_sz,
						  DMA_FROM_DEVICE);
		if (dma_mapping_error(&priv->pdev->dev, rx_buf->dma_addr)) {
			netdev_err(ndev, "dma mapping_error\n");
			goto dma_map_err;
		}

		rx_desc = &((struct emac_rx_desc *)rx_ring->desc_addr)[i];

		memset(rx_desc, 0, sizeof(struct emac_rx_desc));

		rx_desc->BufferAddr1 = rx_buf->dma_addr;
		rx_desc->BufferSize1 = rx_buf->dma_len;

		rx_desc->FirstDescriptor = 0;
		rx_desc->LastDescriptor = 0;
		if (++i == rx_ring->total_cnt) {
			rx_desc->EndRing = 1;
			i = 0;
		}
		dma_wmb();
		rx_desc->OWN = 1;

		rx_buf = &rx_ring->desc_buf[i];
	}
	rx_ring->head = i;
	return;

dma_map_err:
	dev_kfree_skb_any(skb);
	rx_buf->skb = NULL;
	return;
}


/* Name		emac_tx_mem_map
 * Arguments	priv : pointer to driver private data structure
 *		pstSkb : pointer to sk_buff structure passed by upper layer
 *		max_tx_len : max data len per descriptor
 *		frag_num : number of fragments in the packet
 * Return	number of descriptors needed for transmitting packet
 * Description
 */
static int emac_tx_mem_map(struct emac_priv *priv, struct sk_buff *skb,
			   u32 max_tx_len, u32 frag_num)
{
	struct emac_desc_ring *tx_ring;
	struct emac_tx_desc_buffer *tx_buf;
	struct emac_tx_desc *tx_desc;
	u32 skb_linear_len = skb_headlen(skb);
	u32 len, i, f, first, buf_idx = 0;
	phys_addr_t addr;

	tx_ring = &priv->tx_ring;

	i = tx_ring->head;
	first = i;

	if (++i == tx_ring->total_cnt)
		i = 0;

	/* if the data is fragmented */
	for (f = 0; f < frag_num; f++) {
		const skb_frag_t *frag = &skb_shinfo(skb)->frags[f];

		len = skb_frag_size(frag);

		buf_idx = (f + 1) % 2;

		/* first frag fill into second buffer of first descriptor */
		if (f == 0) {
			tx_buf = &tx_ring->tx_desc_buf[first];
			tx_desc = &((struct emac_tx_desc *)tx_ring->desc_addr)[first];
		} else {
			/* from second frags to more frags,
			 * we only get new descriptor when it frag num is odd.
			 */
			if (!buf_idx) {
				tx_buf = &tx_ring->tx_desc_buf[i];
				tx_desc = &((struct emac_tx_desc *)tx_ring->desc_addr)[i];
			}
		}
		tx_buf->buf[buf_idx].dma_len = len;

		addr = skb_frag_dma_map(&priv->pdev->dev, frag, 0,
				       skb_frag_size(frag),
				       DMA_TO_DEVICE);

		if (dma_mapping_error(&priv->pdev->dev, addr)) {
			netdev_err(priv->ndev, "%s dma map page:%d error \n",
					   __func__, f);
			goto dma_map_err;
		}
		tx_buf->buf[buf_idx].dma_addr = addr;

		tx_buf->buf[buf_idx].map_as_page = true;

		/*
		 * every desc has two buffer for packet
		 */

		if (buf_idx) {
			tx_desc->BufferAddr2 = addr;
			tx_desc->BufferSize2 = len;
		} else {
			tx_desc->BufferAddr1 = addr;
			tx_desc->BufferSize1 = len;

			if (++i == tx_ring->total_cnt) {
				tx_desc->EndRing = 1;
				i = 0;
			}
		}
		/*
		 * if frag num equal 1, we don't set tx_desc except buffer addr & size
		 */
		if (f > 0) {
			if (f == (frag_num - 1)) {
				tx_desc->LastSegment = 1;
				tx_buf->skb = skb;
			}

			tx_desc->OWN = 1;
		}
	}

	/* fill out first descriptor for skb linear data */
	tx_buf = &tx_ring->tx_desc_buf[first];

	tx_buf->buf[0].dma_len = skb_linear_len;

	addr = dma_map_single(&priv->pdev->dev, skb->data,
			      skb_linear_len, DMA_TO_DEVICE);
	if (dma_mapping_error(&priv->pdev->dev, addr)) {
		netdev_err(priv->ndev, "%s dma mapping_error\n", __func__);
		goto dma_map_err;
	}

	tx_buf->buf[0].dma_addr = addr;

	tx_buf->buf[0].buff_addr = skb->data;
	tx_buf->buf[0].map_as_page = false;

	/* fill tx descriptor */
	tx_desc = &((struct emac_tx_desc *)tx_ring->desc_addr)[first];
	tx_desc->BufferAddr1 = addr;
	tx_desc->BufferSize1 = skb_linear_len;
	tx_desc->FirstSegment = 1;

	/* if last desc for ring, need to end ring flag */
	if (first == (tx_ring->total_cnt - 1)) {
		tx_desc->EndRing = 1;
	}
	/*
	 * if frag num more than 1, that means data need another desc
	 * so current descriptor isn't last piece of packet data.
	 */
	tx_desc->LastSegment = frag_num > 1 ? 0 : 1;

	/* only last descriptor had skb pointer */
	if (tx_desc->LastSegment)
		tx_buf->skb = skb;

	tx_desc->OWN = 1;

	dma_wmb();

	emac_dma_start_transmit(priv);

	/* update tx ring head */
	tx_ring->head = i;

	return 0;
dma_map_err:
	return 0;
}

/* Name		emac_start_xmit
 * Arguments	pstSkb : pointer to sk_buff structure passed by upper layer
 *		pstNetdev : pointer to net_device structure
 * Return	Status: 0 - Success;  non-zero - Fail
 * Description	This function is called by upper layer to
 *		handover the Tx packet to the driver
 *		for sending it to the device.
 *		Currently this is doing nothing but
 *		simply to simulate the tx packet handling.
 */
static int emac_start_xmit(struct sk_buff *skb, struct net_device *ndev)
{
	struct emac_priv *priv = netdev_priv(ndev);
	int nfrags = skb_shinfo(skb)->nr_frags;

	if (unlikely(emac_tx_avail(priv) < nfrags + 1))
		return NETDEV_TX_BUSY;

	emac_tx_mem_map(priv, skb, MAX_DATA_LEN_TX_DES, nfrags);

	return NETDEV_TX_OK;
}

static int clk_phase_rgmii_set(struct emac_priv *priv, bool is_tx)
{
	u32 val;

	switch (priv->clk_tuning_way) {
	case CLK_TUNING_BY_REG:
		val = readl(priv->ctrl_reg);
		if (is_tx) {
			val &= ~RGMII_TX_PHASE_MASK;
			val |= (priv->tx_clk_phase & 0x7) << RGMII_TX_PHASE_OFFSET;
		} else {
			val &= ~RGMII_RX_PHASE_MASK;
			val |= (priv->rx_clk_phase & 0x7) << RGMII_RX_PHASE_OFFSET;
		}
		writel(val, priv->ctrl_reg);
		break;
	case CLK_TUNING_BY_DLINE:
		val = readl(priv->dline_reg);
		if (is_tx) {
			val &= ~EMAC_TX_DLINE_CODE_MASK;
			val |= priv->tx_clk_phase << EMAC_TX_DLINE_CODE_OFFSET;
			val |= EMAC_TX_DLINE_EN;
		} else {
			val &= ~EMAC_RX_DLINE_CODE_MASK;
			val |= priv->rx_clk_phase << EMAC_RX_DLINE_CODE_OFFSET;
			val |= EMAC_RX_DLINE_EN;
		}
		writel(val, priv->dline_reg);
		break;
	default:
		pr_err("wrong clk tuning way:%d !!\n", priv->clk_tuning_way);
		return -1;
	}
	pr_debug("%s tx phase:%d rx phase:%d\n",
		__func__, priv->tx_clk_phase, priv->rx_clk_phase);
	return 0;
}

static int clk_phase_rmii_set(struct emac_priv *priv, bool is_tx)
{
	u32 val;

	switch (priv->clk_tuning_way) {
	case CLK_TUNING_BY_REG:
		val = readl(priv->ctrl_reg);
		if (is_tx) {
			val &= ~RMII_TX_PHASE_MASK;
			val |= (priv->tx_clk_phase & 0x7) << RMII_TX_PHASE_OFFSET;
		} else {
			val &= ~RMII_RX_PHASE_MASK;
			val |= (priv->rx_clk_phase & 0x7) << RMII_RX_PHASE_OFFSET;
		}
		writel(val, priv->ctrl_reg);
		break;
	case CLK_TUNING_BY_CLK_REVERT:
		val = readl(priv->ctrl_reg);
		if (is_tx) {
			if (priv->tx_clk_phase == CLK_PHASE_REVERT)
				val |= RMII_TX_CLK_SEL;
			else
				val &= ~RMII_TX_CLK_SEL;
		} else {
			if (priv->rx_clk_phase == CLK_PHASE_REVERT)
				val |= RMII_RX_CLK_SEL;
			else
				val &= ~RMII_RX_CLK_SEL;
		}
		writel(val, priv->ctrl_reg);
		break;
	default:
		pr_err("wrong clk tuning way:%d !!\n", priv->clk_tuning_way);
		return -1;
	}
	pr_debug("%s tx phase:%d rx phase:%d\n",
		__func__, priv->tx_clk_phase, priv->rx_clk_phase);
	return 0;
}

static int clk_phase_set(struct emac_priv *priv, bool is_tx)
{
	if (priv->clk_tuning_enable) {
		if (emac_is_rmii(priv)) {
			clk_phase_rmii_set(priv, is_tx);
		} else {
			clk_phase_rgmii_set(priv, is_tx);
		}
	}

	return 0;
}

static int emac_mii_reset(struct mii_bus *bus)
{
	struct emac_priv *priv = bus->priv;
	struct device *dev = &priv->pdev->dev;
	int rst_gpio, ldo_gpio;
	int active_state;
	u32 delays[3] = {0};

	if (dev->of_node) {
		struct device_node *np = dev->of_node;

		if (!np)
			return 0;

		ldo_gpio = of_get_named_gpio(np, "emac,ldo-gpio", 0);
		if (ldo_gpio >= 0) {
			if (gpio_request(ldo_gpio, "mdio-ldo"))
				return 0;

			gpio_direction_output(ldo_gpio, 1);
		}

		rst_gpio = of_get_named_gpio(np, "emac,reset-gpio", 0);
		if (rst_gpio < 0)
			return 0;

		active_state = of_property_read_bool(np,
						     "emac,reset-active-low");
		of_property_read_u32_array(np,
					   "emac,reset-delays-us", delays, 3);

		if (gpio_request(rst_gpio, "mdio-reset"))
			return 0;

		gpio_direction_output(rst_gpio,
		                      active_state ? 1 : 0);
		if (delays[0])
		        msleep(DIV_ROUND_UP(delays[0], 1000));

		gpio_set_value(rst_gpio, active_state ? 0 : 1);
		if (delays[1])
		        msleep(DIV_ROUND_UP(delays[1], 1000));

		gpio_set_value(rst_gpio, active_state ? 1 : 0);
		if (delays[2])
		        msleep(DIV_ROUND_UP(delays[2], 1000));
        }
	return 0;
}

static int emac_mii_read(struct mii_bus *bus, int phy_addr, int regnum)
{
	struct emac_priv *priv = bus->priv;
	u32 cmd = 0;
	u32 val;

	cmd |= phy_addr & 0x1F;
	cmd |= (regnum & 0x1F) << 5;
	cmd |= MREGBIT_START_MDIO_TRANS | MREGBIT_MDIO_READ_WRITE;

	emac_wr(priv, MAC_MDIO_DATA, 0x0);
	emac_wr(priv, MAC_MDIO_CONTROL, cmd);

	if (readl_poll_timeout(priv->iobase + MAC_MDIO_CONTROL,
			       val, !((val >> 15) & 0x1), 100, 10000))
		return -EBUSY;

	val = emac_rd(priv, MAC_MDIO_DATA);
	return val;
}

static int emac_mii_write(struct mii_bus *bus, int phy_addr, int regnum,
			    u16 value)
{
	struct emac_priv *priv = bus->priv;
	u32 cmd = 0;
	u32 val;

	emac_wr(priv, MAC_MDIO_DATA, value);

	cmd |= phy_addr & 0x1F;
	cmd |= (regnum & 0x1F) << 5;
	cmd |= MREGBIT_START_MDIO_TRANS;

	emac_wr(priv, MAC_MDIO_CONTROL, cmd);

	if (readl_poll_timeout(priv->iobase + MAC_MDIO_CONTROL,
			       val, !((val >> 15) & 0x1), 100, 10000))
		return -EBUSY;

	return 0;
}

static void emac_adjust_link(struct net_device *dev)
{
	struct phy_device *phydev = dev->phydev;
	struct emac_priv *priv = netdev_priv(dev);
	bool link_changed = false;
	u32 ctrl;

	if (!phydev)
		return;

	if (phydev->link) {
		ctrl = emac_rd(priv, MAC_GLOBAL_CONTROL);

		/* Now we make sure that we can be in full duplex mode
		 * If not, we operate in half-duplex mode.
		 */
		if (phydev->duplex != priv->duplex) {
			link_changed = true;

			if (!phydev->duplex)
				ctrl &= ~MREGBIT_FULL_DUPLEX_MODE;
			else
				ctrl |= MREGBIT_FULL_DUPLEX_MODE;
			priv->duplex = phydev->duplex;
		}

		if (phydev->speed != priv->speed) {
			link_changed = true;

			ctrl &= ~MREGBIT_SPEED;

			switch (phydev->speed) {
			case SPEED_1000:
				ctrl |= MREGBIT_SPEED_1000M;
				break;
			case SPEED_100:
				ctrl |= MREGBIT_SPEED_100M;
				break;
			case SPEED_10:
				ctrl |= MREGBIT_SPEED_10M;
				break;
			default:
				pr_err("broken speed: %d\n", phydev->speed);
				phydev->speed = SPEED_UNKNOWN;
				break;
			}
			if (phydev->speed != SPEED_UNKNOWN) {
				priv->speed = phydev->speed;
			}
		}

		emac_wr(priv, MAC_GLOBAL_CONTROL, ctrl);

		if (!priv->link) {
			priv->link = true;
			link_changed = true;
		}
	} else if (priv->link) {
		priv->link = false;
		link_changed = true;
		priv->duplex = DUPLEX_UNKNOWN;
		priv->speed = SPEED_UNKNOWN;
	}

	if (link_changed) {
		phy_print_status(phydev);
		if (priv->ecdev) {
			ecdev_set_link(priv->ecdev, priv->link ? 1 : 0);
		}
	}
}

static int emac_phy_connect(struct net_device *dev)
{
	struct phy_device *phydev;
	struct device_node *np;
	struct emac_priv *priv = netdev_priv(dev);

	np = of_parse_phandle(priv->pdev->dev.of_node, "phy-handle", 0);
	if (!np && of_phy_is_fixed_link(priv->pdev->dev.of_node))
		np = of_node_get(priv->pdev->dev.of_node);
	if (!np)
		return -ENODEV;

	of_get_phy_mode(np, &priv->phy_interface);
	pr_info("priv phy_interface = %d\n", priv->phy_interface);

	emac_phy_interface_config(priv);

	phydev = of_phy_connect(dev, np,
				&emac_adjust_link, 0, priv->phy_interface);
	if (IS_ERR_OR_NULL(phydev)) {
		pr_err("Could not attach to PHY\n");
		if (!phydev)
			return -ENODEV;
		return PTR_ERR(phydev);
	}

	pr_info("%s:  %s: attached to PHY (UID 0x%x)"
			" Link = %d\n", __func__,
			dev->name, phydev->phy_id, phydev->link);

	/* Indicate that the MAC is responsible for PHY PM */
	phydev->mac_managed_pm = true;
	dev->phydev = phydev;

	clk_phase_set(priv, TX_PHASE);
	clk_phase_set(priv, RX_PHASE);
	return 0;
}

static int emac_mdio_init(struct emac_priv *priv)
{
	struct device_node *mii_np;
	struct device *dev = &priv->pdev->dev;
	int ret;

	mii_np = of_get_child_by_name(dev->of_node, "mdio-bus");
	if (!mii_np) {
		if (of_phy_is_fixed_link(dev->of_node)) {
			if ((of_phy_register_fixed_link(dev->of_node) < 0)) {
				return -ENODEV;
			}
			dev_dbg(dev, "find fixed link\n");
			return 0;
		}

		dev_err(dev, "no %s child node found", "mdio-bus");
		return -ENODEV;
	}

	if (!of_device_is_available(mii_np)) {
		ret = -ENODEV;
		goto err_put_node;
	}

	priv->mii = devm_mdiobus_alloc(dev);
	if (!priv->mii) {
		ret = -ENOMEM;
		goto err_put_node;
	}
	priv->mii->priv = priv;
	priv->mii->name = "emac mii";
	priv->mii->reset = emac_mii_reset;
	priv->mii->read = emac_mii_read;
	priv->mii->write = emac_mii_write;
	snprintf(priv->mii->id, MII_BUS_ID_SIZE, "%s",
			priv->pdev->name);
	priv->mii->parent = dev;
	priv->mii->phy_mask = 0xffffffff;
	ret = of_mdiobus_register(priv->mii, mii_np);
	if (ret) {
		dev_err(dev, "Failed to register mdio bus.\n");
		goto err_put_node;
	}

	priv->phy = phy_find_first(priv->mii);
	if (!priv->phy) {
		dev_err(dev, "no PHY found\n");
		return -ENODEV;
	}

err_put_node:
	of_node_put(mii_np);
	return ret;
}

static int emac_mdio_deinit(struct emac_priv *priv)
{
	if (!priv->mii)
		return 0;

	mdiobus_unregister(priv->mii);
	return 0;
}

static const struct net_device_ops emac_netdev_ops = {
	.ndo_open               = emac_open,
	.ndo_stop               = emac_close,
	.ndo_start_xmit         = emac_start_xmit,
};

static int emac_config_dt(struct platform_device *pdev, struct emac_priv *priv)
{
	struct device_node *np = pdev->dev.of_node;
	struct resource *res;
	u8 mac_addr[ETH_ALEN] = {0};
	u32 tx_phase, rx_phase;
	u32 ctrl_reg;
	int ret;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	priv->iobase = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(priv->iobase)) {
		dev_err(&pdev->dev, "failed to io remap res reg 0\n");
		return -ENOMEM;
	}

	if (of_property_read_u32(np, "x1,apmu-base-reg", &priv->apmu_base)) {
		priv->apmu_base = PMUA_BASE_REG;
	}

	if (of_property_read_u32(np, "ctrl-reg", &ctrl_reg)) {
		dev_err(&pdev->dev, "cannot find ctrl register in device tree\n");
		return -EINVAL;
	}

	priv->ctrl_reg = ioremap(priv->apmu_base + ctrl_reg, 4);

	if (of_property_read_u32(np, "tx-threshold",
				 &priv->tx_threshold)) {
		priv->tx_threshold = DEFAULT_TX_THRESHOLD;
		dev_dbg(&pdev->dev, "%s tx_threshold using default value:%d \n",
			__func__, priv->tx_threshold);
	}

	if (of_property_read_u32(np, "rx-threshold",
				 &priv->rx_threshold)) {
		priv->rx_threshold = DEFAULT_RX_THRESHOLD;
		dev_dbg(&pdev->dev, "%s rx_threshold using default value:%d \n",
			__func__, priv->rx_threshold);
	}

	if (of_property_read_u32(np, "tx-ring-num",
				 &priv->tx_ring_num)) {
		priv->tx_ring_num = DEFAULT_TX_RING_NUM;
		dev_dbg(&pdev->dev, "%s tx_ring_num using default value:%d \n",
			__func__, priv->tx_ring_num);
	}

	if (of_property_read_u32(np, "rx-ring-num",
				 &priv->rx_ring_num)) {
		priv->rx_ring_num = DEFAULT_RX_RING_NUM;
		dev_dbg(&pdev->dev, "%s rx_ring_num using default value:%d \n",
			__func__, priv->rx_ring_num);
	}

	if (of_property_read_u32(np, "dma-burst-len",
				 &priv->dma_burst_len)) {
		priv->dma_burst_len = DEFAULT_DMA_BURST_LEN;
		dev_dbg(&pdev->dev, "%s dma_burst_len using default value:%d \n",
			__func__, priv->dma_burst_len);
	} else {
		if (priv->dma_burst_len <= 0 || priv->dma_burst_len > 7) {
			dev_err(&pdev->dev, "%s burst len illegal, use default vallue:%d\n",
				__func__, DEFAULT_DMA_BURST_LEN);
			priv->dma_burst_len = DEFAULT_DMA_BURST_LEN;
		}
	}

	if (of_property_read_bool(np, "ref-clock-from-phy")) {
		priv->ref_clk_frm_soc = 0;
		dev_dbg(&pdev->dev, "%s ref clock from external phy \n", __func__);
	} else
		priv->ref_clk_frm_soc = 1;


	ret = of_get_mac_address(np, mac_addr);
	if (ret) {
		if (ret == -EPROBE_DEFER)
			return ret;

		dev_info(&pdev->dev, "Using random mac address\n");
		eth_hw_addr_random(priv->ndev);
	} else {
		eth_hw_addr_set(priv->ndev, mac_addr);
	}

	dev_dbg(&pdev->dev, "%s tx-threshold:%d rx_therhold:%d tx_ring_num:%d rx_ring_num:%d dma-bur_len:%d\n",
		__func__, priv->tx_threshold, priv->rx_threshold, priv->tx_ring_num,
		priv->rx_ring_num, priv->dma_burst_len);

	priv->clk_tuning_enable = of_property_read_bool(np, "clk-tuning-enable");
	if (priv->clk_tuning_enable) {
		if (of_property_read_bool(np, "clk-tuning-by-reg"))
			priv->clk_tuning_way = CLK_TUNING_BY_REG;
		else if (of_property_read_bool(np, "clk-tuning-by-clk-revert"))
			priv->clk_tuning_way = CLK_TUNING_BY_CLK_REVERT;
		else if (of_property_read_bool(np, "clk-tuning-by-delayline")) {
			priv->clk_tuning_way = CLK_TUNING_BY_DLINE;
			if (of_property_read_u32(np, "dline-reg", &ctrl_reg)) {
				dev_err(&pdev->dev, "cannot find delayline register in device tree\n");
				return -EINVAL;
			}
			priv->dline_reg = ioremap(priv->apmu_base + ctrl_reg, 4);
		} else
			priv->clk_tuning_way = CLK_TUNING_BY_REG;

		if (of_property_read_u32(np, "tx-phase", &tx_phase))
			priv->tx_clk_phase = TXCLK_PHASE_DEFAULT;
		else
			priv->tx_clk_phase = tx_phase;

		if (of_property_read_u32(np, "rx-phase", &rx_phase))
			priv->rx_clk_phase = RXCLK_PHASE_DEFAULT;
		else
			priv->rx_clk_phase = rx_phase;
	}

	return 0;
}

void emac_ec_poll(struct net_device *ndev)
{
	struct emac_priv *priv = netdev_priv(ndev);

	emac_rx_clean_desc(priv, 100);
	emac_tx_clean_desc(priv);
}

static int emac_probe(struct platform_device *pdev)
{
	struct emac_priv *priv;
	struct net_device *ndev = NULL;
	int ret;

	ndev = alloc_etherdev(sizeof(struct emac_priv));
	if (!ndev)
		return -ENOMEM;

	ndev->hw_features = NETIF_F_SG;
	ndev->features |= ndev->hw_features;
	priv = netdev_priv(ndev);
	priv->ndev = ndev;
	priv->pdev = pdev;
	platform_set_drvdata(pdev, priv);

	ret = emac_config_dt(pdev, priv);
	if (ret < 0) {
		dev_err(&pdev->dev, "failed to config dt\n");
		goto err_netdev;
	}

	ndev->watchdog_timeo = 5 * HZ;
	ndev->base_addr = (unsigned long)priv->iobase;
	ndev->netdev_ops = &emac_netdev_ops;

	priv->mac_clk = devm_clk_get(&pdev->dev, "emac-clk");
	if (IS_ERR(priv->mac_clk)) {
		dev_err(&pdev->dev, "emac clock not found.\n");
		ret = PTR_ERR(priv->mac_clk);
		goto err_netdev;
	}

	ret = clk_prepare_enable(priv->mac_clk);
	if (ret < 0) {
		dev_err(&pdev->dev, "failed to enable emac clock: %d\n",
			ret);
		goto err_netdev;
	}

	if (priv->ref_clk_frm_soc) {
		priv->phy_clk = devm_clk_get(&pdev->dev, "phy-clk");
		if (IS_ERR(priv->phy_clk)) {
			dev_err(&pdev->dev, "phy clock not found.\n");
			ret = PTR_ERR(priv->phy_clk);
			goto mac_clk_disable;
		}

		ret = clk_prepare_enable(priv->phy_clk);
		if (ret < 0) {
			dev_err(&pdev->dev, "failed to enable phy clock: %d\n",
				ret);
			goto mac_clk_disable;
		}
	}

	priv->reset = devm_reset_control_get_optional(&pdev->dev, NULL);
	if (IS_ERR(priv->reset)) {
		dev_err(&pdev->dev, "Failed to get emac's resets\n");
		goto mac_clk_disable;
	}

	reset_control_deassert(priv->reset);

	emac_sw_init(priv);

	ret = emac_mdio_init(priv);
	if (ret) {
		dev_err(&pdev->dev, "failed to init mdio.\n");
		goto reset_assert;
	}

	SET_NETDEV_DEV(ndev, &pdev->dev);

	priv->ecdev = ecdev_offer(priv->ndev, emac_ec_poll, THIS_MODULE);
	if(!priv->ecdev) {
		dev_err(&pdev->dev, "failed to offer EtherCAT device\n");
		goto err_mdio_deinit;
	} else {
		pr_info("success to offer EtherCAT device\n");
	}

	dma_set_mask_and_coherent(&pdev->dev, DMA_BIT_MASK(32));

	ret = ecdev_open(priv->ecdev);
	if (ret) {
		ecdev_withdraw(priv->ecdev);
		goto err_mdio_deinit;
	}
	return 0;

err_mdio_deinit:
	emac_mdio_deinit(priv);
reset_assert:
	reset_control_assert(priv->reset);
mac_clk_disable:
	clk_disable_unprepare(priv->mac_clk);
err_netdev:
	free_netdev(ndev);
    dev_info(&pdev->dev, "emac_probe failed ret = %d.\n", ret);
	return ret;
}

static int emac_remove(struct platform_device *pdev)
{
	struct emac_priv *priv = platform_get_drvdata(pdev);
	ecdev_close(priv->ecdev);
	ecdev_withdraw(priv->ecdev);

	emac_reset_hw(priv);
	free_netdev(priv->ndev);
	emac_mdio_deinit(priv);
	reset_control_assert(priv->reset);
	clk_disable_unprepare(priv->mac_clk);
	if (priv->ref_clk_frm_soc)
		clk_disable_unprepare(priv->phy_clk);
	return 0;
}

static void emac_shutdown(struct platform_device *pdev)
{
}

static const struct of_device_id emac_of_match[] = {
	{ .compatible = "ky,x1-ec-emac" },
	{ },
};
MODULE_DEVICE_TABLE(of, emac_of_match);

static struct platform_driver ec_emac_driver = {
	.probe = emac_probe,
	.remove = emac_remove,
	.shutdown = emac_shutdown,
	.driver = {
		.name = DRIVER_NAME,
		.of_match_table = of_match_ptr(emac_of_match),
#ifdef CONFIG_KY_PARALLEL_BOOTING
		.probe_type = PROBE_PREFER_ASYNCHRONOUS,
#endif
	},
};

module_platform_driver(ec_emac_driver); 

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Ethernet driver with EtherCAT support for Ky x1 Emac");
MODULE_ALIAS("platform:ky_eth");
