/******************************************************************************
 *
 * Copyright(c) 2007 - 2017 Realtek Corporation.
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

#ifndef __HALRF_PSD_H__
#define __HALRF_PSD_H__


struct _halrf_psd_data {
	u32 point;
	u32 start_point;
	u32 stop_point;
	u32 average;
	u32 buf_size;
	u32 psd_progress;
	u32 bb_backup[30];
#if (DM_ODM_SUPPORT_TYPE & ODM_IOT)
#ifdef CONFIG_MP_INCLUDED
	u64 psd_data_64[320];
	u32 psd_data[320];
#endif
#else
#if (RTL8735B_SUPPORT == 1 || RTL8730A_SUPPORT == 1)
	u32 psd_data[320];
#else
	u32 psd_data[256];
#endif
#endif
};

u32
halrf_psd_init(
	void *dm_void);

void
_halrf_iqk_psd_init_8723f(
	void *dm_void,
	boolean onoff);
void
_halrf_iqk_psd_init_8730a(
	void *dm_void,
	boolean onoff);
u64
halrf_get_iqk_psd_data(
	void *dm_void,
	u32 point);

u32
halrf_psd_query(
	void *dm_void,
	u32 *outbuf,
	u32 buf_size);

u32
halrf_psd_init_query(
	void *dm_void,
	u32 *outbuf,
	u32 point,
	u32 start_point,
	u32 stop_point,
	u32 average,
	u32 buf_size);

void halrf_iqk_psd_init(void *dm_void);

u32 halrf_iqk_psd_result(void *dm_void, u32 ch_freq, u32 spur_freq);

void halrf_iqk_psd_reload(void *dm_void);


#endif /*#__HALRF_PSD_H__*/
