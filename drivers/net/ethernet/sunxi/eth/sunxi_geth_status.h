#ifndef __SUNXI_GETH_STATUS_H__
#define __SUNXI_GETH_STATUS_H__

enum rx_frame_status { /* IPC status */
	good_frame = 0,
	discard_frame = 1,
	csum_none = 2,
	llc_snap = 4,
};

enum tx_dma_irq_status {
	tx_hard_error = 1,
	tx_hard_error_bump_tc = 2,
	handle_tx_rx = 3,
};

struct geth_extra_stats {

	/* Transmit errors */
	unsigned long tx_underflow;
	unsigned long tx_carrier;
	unsigned long tx_losscarrier;
	unsigned long vlan_tag;
	unsigned long tx_deferred;
	unsigned long tx_vlan;
	unsigned long tx_jabber;
	unsigned long tx_frame_flushed;
	unsigned long tx_payload_error;
	unsigned long tx_ip_header_error;

	/* Receive errors */
	unsigned long rx_desc;
	unsigned long sa_filter_fail;
	unsigned long overflow_error;
	unsigned long ipc_csum_error;
	unsigned long rx_collision;
	unsigned long rx_crc;
	unsigned long dribbling_bit;
	unsigned long rx_length;
	unsigned long rx_mii;
	unsigned long rx_multicast;
	unsigned long rx_gmac_overflow;
	unsigned long rx_watchdog;
	unsigned long da_rx_filter_fail;
	unsigned long sa_rx_filter_fail;
	unsigned long rx_missed_cntr;
	unsigned long rx_overflow_cntr;
	unsigned long rx_vlan;

	/* Tx/Rx IRQ errors */
	unsigned long tx_undeflow_irq;
	unsigned long tx_process_stopped_irq;
	unsigned long tx_jabber_irq;
	unsigned long rx_overflow_irq;
	unsigned long rx_buf_unav_irq;
	unsigned long rx_process_stopped_irq;
	unsigned long rx_watchdog_irq;
	unsigned long tx_early_irq;
	unsigned long fatal_bus_error_irq;

	/* Extra info */
	unsigned long threshold;
	unsigned long tx_pkt_n;
	unsigned long rx_pkt_n;
	unsigned long poll_n;
	unsigned long sched_timer_n;
	unsigned long normal_irq_n;
};

#endif
