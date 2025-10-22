/******************************************************************************
 *
 * Copyright(c) 2019 - 2020 Realtek Corporation.
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
#ifndef _PHL_TRX_DEF_SDIO_H_
#define _PHL_TRX_DEF_SDIO_H_

#define MAX_BUS_RX_AGG		20

struct sdio_rx_pkt {
	u8 *wd;
	u8 wd_len;
	u8 *pkt;
	u16 pkt_len;
	struct rtw_r_meta_data meta;
};

struct rtw_rx_buf {
	_os_list list;
	_os_mutex mutex;
	_os_atomic ref;				/* reference count */

	u8 *buffer;
	u32 buf_len;			/* buffer size */
	u32 used_len;			/* total valid data size */
	u8 *ptr;			/* start address of data */
	u32 len;			/* data length */
	struct sdio_rx_pkt pkt[MAX_BUS_RX_AGG];
	u8 agg_cnt;			/* bus aggregation nubmer */
	u8 agg_start;			/* agg start index */
	u8 *next_ptr;			/* next ptr to process if there are */
					/* more data to be procesed */
					/* NULL for no more data */
};

#endif	/* _PHL_TRX_DEF_SDIO_H_ */
