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
#ifndef _HALRF_DACK_H_
#define _HALRF_DACK_H_

/*@--------------------------Define Parameters-------------------------------*/

enum halrf_dack_dz{
	DZ_ADDCK_TIMEOUT	= BIT(0),
	DZ_DADCK_TIMEOUT	= BIT(1),
	DZ_MSBK_TIMEOUT	= BIT(2),
};
/*@-----------------------End Define Parameters-----------------------*/

struct halrf_dack_rpt {
	u8 msbk_d[2][2][16];
	u8 dadck_d[2][2];		/*path/IQ*/
	u16 addck_d[2][2];	/*path/IQ*/
	u16 biask_d[2][2];		/*path/IQ*/
	u8 addck_timeout_s0: 1;
	u8 addck_timeout_s1: 1;
	u8 dadck_timeout_s0: 1;
	u8 dadck_timeout_s1: 1;
	u8 msbk_timeout_s0: 1;
	u8 msbk_timeout_s1: 1;
	u8 dack_fail: 1;
	u8 rsvd: 1;
};

struct halrf_dack_info {
	bool dack_done;
	u8 msbk_d[2][2][16];
	u8 dadck_d[2][2];		/*path/IQ*/
	u16 addck_d[2][2];	/*path/IQ*/
	u16 biask_d[2][2];		/*path/IQ*/
	u32 dack_cnt;
	u32 dack_time;
	bool addck_timeout[2];
	bool dadck_timeout[2];
	bool msbk_timeout[2];
	bool dack_fail;
	struct halrf_dack_rpt dack_rpt;
};

#endif