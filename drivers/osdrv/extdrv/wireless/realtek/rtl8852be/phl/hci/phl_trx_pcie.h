/******************************************************************************
 *
 * Copyright(c) 2019 Realtek Corporation.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of version 2 of the GNU General Public License as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 *****************************************************************************/
#ifndef _PHL_TRX_PCIE_H_
#define _PHL_TRX_PCIE_H_

#define WP_DELAY_THRES_MS 1000
#define WD_PAGE_SIZE 128

enum dump_list_type {
	TYPE_WD_PAGE = 0,
	TYPE_PHL_RING = 1,
	TYPE_RING_STS = 2,
	TYPE_H2C_PKT = 3,
	TYPE_MAX = 0xFF
};

#define _CMD_LIST_CPUID 1
#define _CMD_CPUID_OFFSET 10
#define _CMD_MAX_CPUID_VAL 20

#define _CMD_DUMP_WP_PTR 1
#define _CMD_RESET_WP_PTR 2
#define _CMD_MAX_RESET_WP_VAL 30

#define _CMD_DUMP_WD_INFO 1
#define _CMD_LIST_WP_INFO 2
#define _CMD_MAX_WD_VAL 40
#define _CMD_SHOW_WP_OFFSET 10000
#define _CMD_WP_OFFSET 10

struct rtw_rx_buf_ring {
	struct rtw_rx_buf *rx_buf;
	_os_list idle_rxbuf_list;
	_os_list busy_rxbuf_list;
	u16 idle_rxbuf_cnt;
	u16 busy_rxbuf_cnt;
	_os_lock idle_rxbuf_lock;
	_os_lock busy_rxbuf_lock;
#ifdef CONFIG_DYNAMIC_RX_BUF
	_os_list empty_rxbuf_list;
	_os_lock empty_rxbuf_lock;
	u16 empty_rxbuf_cnt;
#endif
};

struct rtw_wp_tag {
	u8 *ptr;
};

struct rtw_h2c_work {
	struct rtw_h2c_pkt *cmd;
	struct rtw_h2c_pkt *data;
	struct rtw_h2c_pkt *ldata;
	struct rtw_h2c_pkt **cmd_ring;
	struct rtw_h2c_pkt **data_ring;
	struct rtw_h2c_pkt **ldata_ring;
	_os_lock       	lock;
	u16		cmd_cnt;
	u16		cmd_idx;
	u16		data_cnt;
	u16		data_idx;
	u16		ldata_cnt;
	u16		ldata_idx;
};

#ifdef RTW_WD_PAGE_USE_SHMEM_POOL
struct rtw_shmem_pool {
	u8 *vir_addr;
	u32 phy_addr_l;
	u32 phy_addr_h;
	u32 buf_len;
	void *os_rsvd[1];
};
#endif

struct rtw_wd_page_ring {
	struct rtw_wd_page *wd_page;
	struct rtw_wd_page *wd_work;
	struct rtw_wd_page **wd_work_ring;
#ifdef RTW_WD_PAGE_USE_SHMEM_POOL
	struct rtw_shmem_pool wd_page_shmem_pool;
#endif
 	_os_list	idle_wd_page_list;
  	_os_list	busy_wd_page_list;
	_os_list	pending_wd_page_list;
	_os_lock	idle_lock;
 	_os_lock	busy_lock;
	_os_lock	pending_lock;
	_os_lock	work_lock;
	_os_lock	wp_tag_lock;
	u16 		idle_wd_page_cnt;
	u16 		busy_wd_page_cnt;
	u16 		pending_wd_page_cnt;
	u16		wd_work_cnt;
	u16		wd_work_idx;
	struct rtw_h2c_work h2c_work;
	struct rtw_wp_tag wp_tag[WP_MAX_SEQ_NUMBER];
	u16 wp_seq;
	u16 cur_hw_res;
};

/* struct hana_temp{ */
/* 	struct wp_tag wp_tag[DMA_CHANNEL_ENTRY][4096]; */
/* 	struct wp_tag wp_tag_hq_b0[4096]; */
/* 	struct wp_tag wp_tag_mq_b0[4096]; */
/* 	struct wp_tag wp_tag_mq_no_ps_b0[4096]; */
/* 	struct wp_tag wp_tag_hq_b1[4096]; */
/* 	struct wp_tag wp_tag_mq_b1[4096]; */
/* 	struct wp_tag wp_tag_mq_no_ps_b1[4096]; */
/* }; */

enum rtw_phl_status phl_hook_trx_ops_pci(struct phl_info_t *phl_info);

enum rtw_phl_status phl_cmd_set_l2_leave(struct phl_info_t *phl_info);
#endif	/* _PHL_TRX_PCIE_H_ */
