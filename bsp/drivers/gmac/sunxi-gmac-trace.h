/* SPDX-License-Identifier: GPL-2.0+ */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */

/* This must be outside ifdef _SUNXI_GMAC_TRACE_H */
#undef TRACE_SYSTEM
#define TRACE_SYSTEM gmac

#if !defined(_SUNXI_GMAC_TRACE_H_) || defined(TRACE_HEADER_MULTI_READ)
#define _SUNXI_GMAC_TRACE_H_

#include <linux/tracepoint.h>

TRACE_EVENT(sunxi_gmac_skb_dump,
	TP_PROTO(struct sk_buff *skb, char *ndev_name, bool is_tx),
	TP_ARGS(skb, ndev_name, is_tx),

	TP_STRUCT__entry(
		__string(devname, ndev_name)
		__string(data, skb->data)
		__field(unsigned int, headlen)
		__field(unsigned int, len)
		__field(__u8, nr_frags)
		__field(bool, is_tx)
	),

	TP_fast_assign(
		__assign_str(devname, ndev_name);
		__assign_str(data, skb->data);
		__entry->headlen = skb_headlen(skb); /* Linear data len */
		__entry->len = skb->len; /* Linear and non-linear len */
		__entry->nr_frags = skb_shinfo(skb)->nr_frags; /* Non-linear fragments count */
		__entry->is_tx = is_tx;
	),

	TP_printk(
		"%s: netdevice: %s, linear_len: %d, total_len: %d, non-linear_fragments: %d, skb_data: %s",
		__entry->is_tx ? "TX packet" : "RX packet", __get_str(devname),  __entry->headlen, __entry->len,
		__entry->nr_frags, __print_hex_dump(KERN_INFO, DUMP_PREFIX_NONE, 16, 1, __get_str(data),
		64, true))
);

TRACE_EVENT(sunxi_gmac_tx_desc,
	TP_PROTO(int tx_clean, int tx_dirty, dma_addr_t dma_addr, char *ndev_name),
	TP_ARGS(tx_clean, tx_dirty, dma_addr, ndev_name),

	TP_STRUCT__entry(
		__field(int, tx_clean)
		__field(int, tx_dirty)
		__field(dma_addr_t, desc_dma)
		__string(devname, ndev_name)
	),

	TP_fast_assign(
		__entry->tx_clean = tx_clean;
		__entry->tx_dirty = tx_dirty;
		__entry->desc_dma = dma_addr,
		__assign_str(devname, ndev_name);
	),

	TP_printk(
		"netdevice: %s, tx_clean:%d tx_dirty:%d skb_phys:(%pad)",
		__get_str(devname), __entry->tx_clean, __entry->tx_dirty,
		&__entry->desc_dma)
);

TRACE_EVENT(sunxi_gmac_rx_desc,
	TP_PROTO(int rx_clean, int rx_dirty, dma_addr_t dma_addr, char *ndev_name),
	TP_ARGS(rx_clean, rx_dirty, dma_addr, ndev_name),

	TP_STRUCT__entry(
		__field(int, rx_clean)
		__field(int, rx_dirty)
		__field(dma_addr_t, desc_dma)
		__string(devname, ndev_name)
	),

	TP_fast_assign(
		__entry->rx_clean = rx_clean;
		__entry->rx_dirty = rx_dirty;
		__entry->desc_dma = dma_addr,
		__assign_str(devname, ndev_name);
	),

	TP_printk(
		"netdevice: %s, rx_clean:%d rx_dirty:%d skb_phys:(%pad)",
		__get_str(devname), __entry->rx_clean, __entry->rx_dirty,
		&__entry->desc_dma)
);

#endif /* _SUNXI_GMAC_TRACE_H_ */

/* This must be outside ifdef _SUNXI_GMAC_TRACE_H */
#undef TRACE_INCLUDE_PATH
#define TRACE_INCLUDE_PATH .
#undef TRACE_INCLUDE_FILE
#define TRACE_INCLUDE_FILE sunxi-gmac-trace
#include <trace/define_trace.h>
