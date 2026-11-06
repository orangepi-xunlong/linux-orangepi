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

/*@===========================================================
 * include files
 *============================================================
 */
#include "mp_precomp.h"
#include "phydm_precomp.h"

u64 _sqrt(u64 x)
{
	u64 i = 0;
	u64 j = (x >> 1) + 1;

	while (i <= j) {
		u64 mid = (i + j) >> 1;

		u64 sq = mid * mid;

		if (sq == x)
			return mid;
		else if (sq < x)
			i = mid + 1;
		else
			j = mid - 1;
	}

	return j;
}

u32 halrf_get_psd_data(
	struct dm_struct *dm,
	u32 point)
{
	struct _hal_rf_ *rf = &(dm->rf_table);
	struct _halrf_psd_data *psd = &(rf->halrf_psd_data);
	u32 psd_val = 0, psd_reg, psd_report, psd_point, psd_start, i, delay_time = 0;

#if (DEV_BUS_TYPE == RT_USB_INTERFACE) || (DEV_BUS_TYPE == RT_SDIO_INTERFACE)
	if (dm->support_interface == ODM_ITRF_USB || dm->support_interface == ODM_ITRF_SDIO) {
		if (psd->average == 0)
			delay_time = 100;
		else
			delay_time = 0;
	}
#endif
#if (DEV_BUS_TYPE == RT_PCI_INTERFACE)
	if (dm->support_interface == ODM_ITRF_PCIE) {
		if (psd->average == 0)
			delay_time = 1000;
		else
			delay_time = 100;
	}
#endif

	if (dm->support_ic_type & (ODM_RTL8812 | ODM_RTL8821 | ODM_RTL8814A | ODM_RTL8822B | ODM_RTL8821C)) {
		psd_reg = R_0x910;
		psd_report = R_0xf44;
	} else {
		psd_reg = R_0x808;
		psd_report = R_0x8b4;
	}

	if (dm->support_ic_type & ODM_RTL8710B) {
		psd_point = 0xeffffc00;
		psd_start = 0x10000000;
	} else {
		psd_point = 0xffbffc00;
		psd_start = 0x00400000;
	}

	psd_val = odm_get_bb_reg(dm, psd_reg, MASKDWORD);

	psd_val &= psd_point;
	psd_val |= point;

	odm_set_bb_reg(dm, psd_reg, MASKDWORD, psd_val);

	psd_val |= psd_start;

	odm_set_bb_reg(dm, psd_reg, MASKDWORD, psd_val);

	for (i = 0; i < delay_time; i++)
		ODM_delay_us(1);

	psd_val = odm_get_bb_reg(dm, psd_report, MASKDWORD);

	if (dm->support_ic_type & (ODM_RTL8821C | ODM_RTL8710B)) {
		psd_val &= MASKL3BYTES;
		psd_val = psd_val / 32;
	} else {
		psd_val &= MASKLWORD;
	}

	return psd_val;
}

void halrf_psd(
	struct dm_struct *dm,
	u32 point,
	u32 start_point,
	u32 stop_point,
	u32 average)
{
	struct _hal_rf_ *rf = &(dm->rf_table);
	struct _halrf_psd_data *psd = &(rf->halrf_psd_data);

	u32 i = 0, j = 0, k = 0;
	u32 psd_reg, avg_org, point_temp, average_tmp, mode;
	u64 data_tatal = 0, data_temp[64] = {0};

	psd->buf_size = 256;

	mode = average >> 16;
	
	if (mode == 2)
		average_tmp = 1;
	else
		average_tmp = average & 0xffff;

	if (dm->support_ic_type & (ODM_RTL8812 | ODM_RTL8821 | ODM_RTL8814A | ODM_RTL8822B | ODM_RTL8821C))
		psd_reg = R_0x910;
	else
		psd_reg = R_0x808;

#if 0
	dbg_print("[PSD]point=%d, start_point=%d, stop_point=%d, average=%d, average_tmp=%d, buf_size=%d\n",
		point, start_point, stop_point, average, average_tmp, psd->buf_size);
#endif
#if (DM_ODM_SUPPORT_TYPE & ODM_IOT)
#ifdef CONFIG_MP_INCLUDED
	for (i = 0; i < psd->buf_size; i++)
		psd->psd_data[i] = 0;
#endif
#else
	for (i = 0; i < psd->buf_size; i++)
		psd->psd_data[i] = 0;
#endif

	if (dm->support_ic_type & ODM_RTL8710B)
		avg_org = odm_get_bb_reg(dm, psd_reg, 0x30000);
	else
		avg_org = odm_get_bb_reg(dm, psd_reg, 0x3000);

	if (mode == 1) {
		if (dm->support_ic_type & ODM_RTL8710B)
			odm_set_bb_reg(dm, psd_reg, 0x30000, 0x1);
		else
			odm_set_bb_reg(dm, psd_reg, 0x3000, 0x1);
	}

#if 0
	if (avg_temp == 0)
		avg = 1;
	else if (avg_temp == 1)
		avg = 8;
	else if (avg_temp == 2)
		avg = 16;
	else if (avg_temp == 3)
		avg = 32;
#endif

	i = start_point;
	while (i < stop_point) {
		data_tatal = 0;

		if (i >= point)
			point_temp = i - point;
		else
			point_temp = i;

		for (k = 0; k < average_tmp; k++) {
			data_temp[k] = halrf_get_psd_data(dm, point_temp);
			data_tatal = data_tatal + (data_temp[k] * data_temp[k]);

#if 0
			if ((k % 20) == 0)
				dbg_print("\n ");

			dbg_print("0x%x ", data_temp[k]);
#endif
		}
#if 0
		/*dbg_print("\n");*/
#endif

#if (DM_ODM_SUPPORT_TYPE & ODM_IOT)
#ifdef CONFIG_MP_INCLUDED
		data_tatal = phydm_division64((data_tatal * 100), average_tmp);
		psd->psd_data[j] = (u32)_sqrt(data_tatal);
#endif
#else
		data_tatal = phydm_division64((data_tatal * 100), average_tmp);
		psd->psd_data[j] = (u32)_sqrt(data_tatal);
#endif

		i++;
		j++;
	}

#if 0
	for (i = 0; i < psd->buf_size; i++) {
		if ((i % 20) == 0)
			dbg_print("\n ");

		dbg_print("0x%x ", psd->psd_data[i]);
	}
	dbg_print("\n\n");
#endif

	if (dm->support_ic_type & ODM_RTL8710B)
		odm_set_bb_reg(dm, psd_reg, 0x30000, avg_org);
	else
		odm_set_bb_reg(dm, psd_reg, 0x3000, avg_org);
}

void backup_bb_register(struct dm_struct *dm, u32 *bb_backup, u32 *backup_bb_reg, u32 counter)
{
	u32 i ;

	for (i = 0; i < counter; i++)
		bb_backup[i] = odm_get_bb_reg(dm, backup_bb_reg[i], MASKDWORD);
}

void restore_bb_register(struct dm_struct *dm, u32 *bb_backup, u32 *backup_bb_reg, u32 counter)
{
	u32 i ;

	for (i = 0; i < counter; i++)
		odm_set_bb_reg(dm, backup_bb_reg[i], MASKDWORD, bb_backup[i]);
}



void _halrf_psd_iqk_init(struct dm_struct *dm)
{
	odm_set_bb_reg(dm, 0x1b04, MASKDWORD, 0x0);
	odm_set_bb_reg(dm, 0x1b08, MASKDWORD, 0x80);
	odm_set_bb_reg(dm, 0x1b0c, 0xc00, 0x3);
	odm_set_bb_reg(dm, 0x1b14, MASKDWORD, 0x0);
	odm_set_bb_reg(dm, 0x1b18, BIT(0), 0x1);

	if (dm->support_ic_type & ODM_RTL8197G)
		odm_set_bb_reg(dm, 0x1b20, MASKDWORD, 0x00040008);
	if (dm->support_ic_type & ODM_RTL8198F)
		odm_set_bb_reg(dm, 0x1b20, MASKDWORD, 0x00000000);

	if (dm->support_ic_type & (ODM_RTL8197G | ODM_RTL8198F)) {
		odm_set_bb_reg(dm, 0x1b24, MASKDWORD, 0x00030000);
		odm_set_bb_reg(dm, 0x1b28, MASKDWORD, 0x00000000);
		odm_set_bb_reg(dm, 0x1b2c, MASKDWORD, 0x00180018);
		odm_set_bb_reg(dm, 0x1b30, MASKDWORD, 0x20000000);
		/*odm_set_bb_reg(dm, 0x1b38, MASKDWORD, 0x20000000);*/
		/*odm_set_bb_reg(dm, 0x1b3c, MASKDWORD, 0x20000000);*/
	}

	odm_set_bb_reg(dm, 0x1b1c, 0xfff, 0xd21);
	odm_set_bb_reg(dm, 0x1b1c, 0xfff00000, 0x821);
	odm_set_bb_reg(dm, 0x1b28, MASKDWORD, 0x0);
	odm_set_bb_reg(dm, 0x1bcc, 0x3f, 0x3f);	
}

void _halrf_iqk_psd_init_8723f(void *dm_void,	 boolean onoff)
{
	struct dm_struct *dm = (struct dm_struct *)dm_void;
	u8 s;

	s = (u8)odm_get_bb_reg(dm, 0x1884, BIT(20));

	if (onoff) {
		/*01_8723F_AFE_ON_BB_settings.txt*/
		odm_set_bb_reg(dm, 0x1c38, MASKDWORD, 0x0);
		odm_set_bb_reg(dm, R_0x1830, BIT(30), 0x0);
		odm_set_bb_reg(dm, R_0x1860, 0xF0000000, 0xf);
		odm_set_bb_reg(dm, R_0x1860, 0x0FFFF000, 0x0041);
		odm_set_bb_reg(dm, 0x09f0, 0x0000FFFF, 0xbbbb);
		odm_set_bb_reg(dm, 0x1d40, BIT(3), 0x1);
		odm_set_bb_reg(dm, 0x1d40, 0x00000007, 0x3);
		odm_set_bb_reg(dm, 0x09b4, 0x00000700, 0x3);
		odm_set_bb_reg(dm, 0x09b4, 0x00003800, 0x3);
		odm_set_bb_reg(dm, 0x09b4, 0x0001C000, 0x3);
		odm_set_bb_reg(dm, 0x09b4, 0x000E0000, 0x3);
		odm_set_bb_reg(dm, R_0x1c20, BIT(5), 0x1);
		odm_set_bb_reg(dm, R_0x1e24, BIT(31), 0x0);
		odm_set_bb_reg(dm, R_0x1e28, 0x0000000F, 0x1);
		odm_set_bb_reg(dm, R_0x824, 0x000F0000, 0x1);
		odm_set_bb_reg(dm, R_0x1cd0, 0xF0000000, 0x7);
		odm_set_bb_reg(dm, R_0x2a24, BIT(13), 0x1);
		odm_set_bb_reg(dm, R_0x1c68, BIT(24), 0x1);
		odm_set_bb_reg(dm, R_0x1864, BIT(31), 0x1);
		odm_set_bb_reg(dm, R_0x180c, BIT(27), 0x1);
		odm_set_bb_reg(dm, R_0x180c, BIT(30), 0x1);
		odm_set_bb_reg(dm, R_0x1e24, BIT(17), 0x1);
		odm_set_bb_reg(dm, R_0x1880, BIT(21), 0x0);
		odm_set_bb_reg(dm, R_0x1c38, MASKDWORD, 0xffffffff);
		/*02_IQK_Preset.txt*/
		//odm_set_rf_reg(dm, RF_PATH_A, 0x05, BIT(0), 0x0);
		//odm_set_rf_reg(dm, RF_PATH_B, 0x05, BIT(0), 0x0);
		odm_set_bb_reg(dm, R_0x1b08, MASKDWORD, 0x00000080);
		//odm_set_bb_reg(dm, R_0x1bd8, MASKDWORD, 0x00000002);
		//switch path  10 od 0x1b38 0x1/0x3 [1:0]
		if (s == 0)
			odm_set_bb_reg(dm, R_0x1b00, MASKDWORD, 0x00000008);
		else
			odm_set_bb_reg(dm, R_0x1b00, MASKDWORD, 0x0000000a);

		odm_set_bb_reg(dm, R_0x1b18, MASKDWORD, 0x40010101);
		odm_set_bb_reg(dm, R_0x1b14, MASKDWORD, 0x40010100);
		//odm_set_bb_reg(dm, R_0x1b1c, MASKDWORD, 0xA2103C00);
		odm_set_bb_reg(dm, R_0x1b0c, 0x00000C00, 0x2);
		odm_set_bb_reg(dm, R_0x1bcc, 0x0000003F, 0x3f);	
		//DbgPrint("[PSD][8723F]iqkpsd init!\n");
	} else {
		/*10_IQK_Reg_PSD_Restore.txt*/
		//odm_set_bb_reg(dm, R_0x1b1c, MASKDWORD, 0xA2103C00);
		odm_set_bb_reg(dm, R_0x1b08, MASKDWORD, 0x00000000);
		odm_set_bb_reg(dm, R_0x1b38, BIT(0), 0x0);
		odm_set_bb_reg(dm, R_0x1bcc, 0x0000003F, 0x0);	
		//odm_set_rf_reg(dm, RF_PATH_A, 0x05, BIT(0), 0x1);
		//odm_set_rf_reg(dm, RF_PATH_B, 0x05, BIT(0), 0x1);
		/*11_8723F_restore_AFE_BB_settings.txt*/
		odm_set_bb_reg(dm, 0x1c38, MASKDWORD, 0x0);
		odm_set_bb_reg(dm, R_0x1830, BIT(30), 0x1);
		odm_set_bb_reg(dm, R_0x1e24, BIT(31), 0x1);
		odm_set_bb_reg(dm, R_0x2a24, BIT(13), 0x0);
		odm_set_bb_reg(dm, R_0x1c68, BIT(24), 0x0);
		odm_set_bb_reg(dm, R_0x1864, BIT(31), 0x0);
		odm_set_bb_reg(dm, R_0x180c, BIT(27), 0x0);
		odm_set_bb_reg(dm, R_0x180c, BIT(30), 0x0);
		odm_set_bb_reg(dm, R_0x1880, BIT(21), 0x0);
		odm_set_bb_reg(dm, R_0x1c38, MASKDWORD, 0xffa1005e);
		//DbgPrint("[PSD][8723F]iqkpsd resotre!\n");
	}
}

void _halrf_iqk_psd_init_8730a(void *dm_void, boolean onoff)
{
	struct dm_struct *dm = (struct dm_struct *)dm_void;

	if (onoff) {
		/*01_D2_AFE_ON_BB_settings.txt*/
		odm_set_bb_reg(dm, 0x1c38, MASKDWORD, 0x0);
		odm_set_bb_reg(dm, R_0x1810, MASKDWORD, 0x10001600);
		odm_set_bb_reg(dm, R_0x1814, MASKDWORD, 0x5bba5020);
		odm_set_bb_reg(dm, R_0x1818, MASKDWORD, 0x0001d429);
		odm_set_bb_reg(dm, R_0x181c, MASKDWORD, 0x00000140);
		odm_set_bb_reg(dm, R_0x1830, BIT(30), 0x0);
		odm_set_bb_reg(dm, R_0x1860, 0xFF000000, 0x0F);
		odm_set_bb_reg(dm, R_0x1860, 0x00FFF000, 0x001);
		odm_set_bb_reg(dm, 0x09f0, 0x0000FFFF, 0x5555);
		odm_set_bb_reg(dm, 0x1d40, BIT(3), 0x1);
		odm_set_bb_reg(dm, 0x1d40, 0x00000007, 0x4);

		odm_set_bb_reg(dm, 0x09b4, 0x00000700, 0x5);
		odm_set_bb_reg(dm, 0x09b4, 0x00003800, 0x5);
		odm_set_bb_reg(dm, 0x09b4, 0x0001C000, 0x5);
		odm_set_bb_reg(dm, 0x09b4, 0x000E0000, 0x5);
		odm_set_bb_reg(dm, R_0x1c20, BIT(5), 0x1);

		odm_set_bb_reg(dm, R_0x1e24, BIT(31), 0x0);
		odm_set_bb_reg(dm, R_0x1e28, 0x0000000F, 0x1);
		odm_set_bb_reg(dm, R_0x824, 0x000F0000, 0x1);
		odm_set_bb_reg(dm, R_0x1cd0, 0xF0000000, 0x7);
		odm_set_bb_reg(dm, R_0x2a24, BIT(13), 0x1);
		odm_set_bb_reg(dm, R_0x1c68, BIT(24), 0x1);
		odm_set_bb_reg(dm, R_0x1864, BIT(31), 0x1);
		odm_set_bb_reg(dm, R_0x180c, BIT(27), 0x1);
		odm_set_bb_reg(dm, R_0x180c, BIT(30), 0x1);
		odm_set_bb_reg(dm, R_0x1e24, BIT(17), 0x1);
		odm_set_bb_reg(dm, R_0x180c, BIT(31), 0x1);
		odm_set_bb_reg(dm, R_0x1880, BIT(21), 0x0);
		odm_set_bb_reg(dm, R_0x1c38, MASKDWORD, 0xffffffff);
		/*02_1_backup_and_nctl_rst_on.txt*/
		odm_set_bb_reg(dm, R_0x1b80, MASKDWORD, 0x00000006);
		/*02_IQK_Preset.txt*/
		odm_set_bb_reg(dm, R_0x1b08, MASKDWORD, 0x00000080);
		/*04 init*/
		odm_set_bb_reg(dm, R_0x1b00, MASKDWORD, 0x00000008);
		odm_set_bb_reg(dm, R_0x1b18, MASKDWORD, 0x40010101);
		odm_set_bb_reg(dm, R_0x1b14, MASKDWORD, 0x40010100);
		odm_set_bb_reg(dm, R_0x1b0c, 0x00000c00, 0x3);
		odm_set_bb_reg(dm, R_0x1bcc, 0x0000003f, 0x3f);
		//DbgPrint("[PSD][8730A]iqkpsd init!\n");
	} else {
		/*10_IQK_Reg_PSD_Restore.txt*/
		odm_set_bb_reg(dm, R_0x1b80, MASKDWORD, 0x00000002);
		odm_set_bb_reg(dm, R_0x1b08, MASKDWORD, 0x00000000);
		//10 or1 0xEE 0x0 [19]
		//10 or1 0x0 0x3 [19:16]
		//10 or1 0x5 0x1 [0]
		/*11_restore_AFE_BB_settings.txt*/
		odm_set_bb_reg(dm, 0x1c38, MASKDWORD, 0x0);
		odm_set_bb_reg(dm, R_0x1830, BIT(30), 0x1);
		odm_set_bb_reg(dm, R_0x1e24, BIT(31), 0x1);
		odm_set_bb_reg(dm, R_0x2a24, BIT(13), 0x0);
		odm_set_bb_reg(dm, R_0x1c68, BIT(24), 0x0);
		odm_set_bb_reg(dm, R_0x1864, BIT(31), 0x0);
		odm_set_bb_reg(dm, R_0x180c, BIT(27), 0x0);
		odm_set_bb_reg(dm, R_0x180c, BIT(30), 0x0);
		odm_set_bb_reg(dm, R_0x1880, BIT(21), 0x0);
		//DbgPrint("[PSD][8730A]iqkpsd resotre!\n");
	}
}

void _halrf_psd_iqk_init_8814c(struct dm_struct *dm)
{
	odm_set_bb_reg(dm, 0x1b04, MASKDWORD, 0x0);
	odm_set_bb_reg(dm, 0x1b08, MASKDWORD, 0x80);
	odm_set_bb_reg(dm, 0x1b0c, 0xc00, 0x3);
	odm_set_bb_reg(dm, 0x1b14, MASKDWORD, 0x0);
	odm_set_bb_reg(dm, 0x1b18, BIT(0), 0x1);
	odm_set_bb_reg(dm, 0x1b28, MASKDWORD, 0x0);
	odm_set_bb_reg(dm, 0x1b30, MASKDWORD, 0x40000000);
	odm_set_bb_reg(dm, 0x1bcc, 0x3f, 0x3f);
}

void _halrf_psd_iqk_init_8735b(struct dm_struct *dm)
{
	odm_set_bb_reg(dm, R_0x1b08, MASKDWORD, 0x00000080);
	odm_set_bb_reg(dm, R_0x1b00, MASKDWORD, 0x00000008);
	odm_set_bb_reg(dm, R_0x1b18, MASKDWORD, 0x40010101);
	odm_set_bb_reg(dm, R_0x1b14, MASKDWORD, 0x40010100);
	odm_set_bb_reg(dm, R_0x1b0c, 0x00000c00, 0x3);
	odm_set_bb_reg(dm, R_0x1bcc, 0x0000003f, 0x3f);
}

void _halrf_psd_iqk_init_8822e(struct dm_struct *dm)
{
	/*02_8822E_BB_for_IQK*/
	odm_set_bb_reg(dm, R_0x1e24, 0x00020000, 0x1);
	odm_set_bb_reg(dm, R_0x1cd0, 0x10000000, 0x1);
	odm_set_bb_reg(dm, R_0x1cd0, 0x20000000, 0x1);
	odm_set_bb_reg(dm, R_0x1cd0, 0x40000000, 0x1);
	odm_set_bb_reg(dm, R_0x1cd0, 0x80000000, 0x0);
	odm_set_bb_reg(dm, R_0x1c68, 0x0f000000, 0xf);
	odm_set_bb_reg(dm, R_0x1864, 0x80000000, 0x1);
	odm_set_bb_reg(dm, R_0x4164, 0x80000000, 0x1);
	odm_set_bb_reg(dm, R_0x180c, 0x08000000, 0x1);
	odm_set_bb_reg(dm, R_0x410c, 0x08000000, 0x1);
	odm_set_bb_reg(dm, R_0x186c, 0x00000080, 0x1);
	odm_set_bb_reg(dm, R_0x416c, 0x00000080, 0x1);
	odm_set_bb_reg(dm, R_0x180c, 0x00000003, 0x0);
	odm_set_bb_reg(dm, R_0x410c, 0x00000003, 0x0);
	odm_set_bb_reg(dm, R_0x1a00, 0x00000003, 0x2);
	odm_set_bb_reg(dm, R_0x1b08, MASKDWORD, 0x00000080);

	/*03_8822E_AFE_for_IQK*/
	odm_set_bb_reg(dm, R_0x1c38, MASKDWORD, 0x00000000);
	odm_set_bb_reg(dm, R_0x1c38, MASKDWORD, 0xffffffff);
#if 0
	odm_set_bb_reg(dm, R_0x1830, MASKDWORD, 0x700f0001);
	odm_set_bb_reg(dm, R_0x1830, MASKDWORD, 0x700f0001);
	odm_set_bb_reg(dm, R_0x1830, MASKDWORD, 0x701f0001);
	odm_set_bb_reg(dm, R_0x1830, MASKDWORD, 0x702f0001);
	odm_set_bb_reg(dm, R_0x1830, MASKDWORD, 0x703f0001);
	odm_set_bb_reg(dm, R_0x1830, MASKDWORD, 0x704f0001);
	odm_set_bb_reg(dm, R_0x1830, MASKDWORD, 0x705f0001);
	odm_set_bb_reg(dm, R_0x1830, MASKDWORD, 0x706f0001);
	odm_set_bb_reg(dm, R_0x1830, MASKDWORD, 0x707f0001);
	odm_set_bb_reg(dm, R_0x1830, MASKDWORD, 0x708f0001);
	odm_set_bb_reg(dm, R_0x1830, MASKDWORD, 0x709f0001);
	odm_set_bb_reg(dm, R_0x1830, MASKDWORD, 0x70af0001);
	odm_set_bb_reg(dm, R_0x1830, MASKDWORD, 0x70bf0001);
	odm_set_bb_reg(dm, R_0x1830, MASKDWORD, 0x70cf0001);
	odm_set_bb_reg(dm, R_0x1830, MASKDWORD, 0x70df0001);
	odm_set_bb_reg(dm, R_0x1830, MASKDWORD, 0x70ef0001);
	odm_set_bb_reg(dm, R_0x1830, MASKDWORD, 0x70ff0001);
	odm_set_bb_reg(dm, R_0x1830, MASKDWORD, 0x70ff0001);
	odm_set_bb_reg(dm, R_0x4130, MASKDWORD, 0x700f0001);
	odm_set_bb_reg(dm, R_0x4130, MASKDWORD, 0x700f0001);
	odm_set_bb_reg(dm, R_0x4130, MASKDWORD, 0x701f0001);
	odm_set_bb_reg(dm, R_0x4130, MASKDWORD, 0x702f0001);
	odm_set_bb_reg(dm, R_0x4130, MASKDWORD, 0x703f0001);
	odm_set_bb_reg(dm, R_0x4130, MASKDWORD, 0x704f0001);
	odm_set_bb_reg(dm, R_0x4130, MASKDWORD, 0x705f0001);
	odm_set_bb_reg(dm, R_0x4130, MASKDWORD, 0x706f0001);
	odm_set_bb_reg(dm, R_0x4130, MASKDWORD, 0x707f0001);
	odm_set_bb_reg(dm, R_0x4130, MASKDWORD, 0x708f0001);
	odm_set_bb_reg(dm, R_0x4130, MASKDWORD, 0x709f0001);
	odm_set_bb_reg(dm, R_0x4130, MASKDWORD, 0x70af0001);
	odm_set_bb_reg(dm, R_0x4130, MASKDWORD, 0x70bf0001);
	odm_set_bb_reg(dm, R_0x4130, MASKDWORD, 0x70cf0001);
	odm_set_bb_reg(dm, R_0x4130, MASKDWORD, 0x70df0001);
	odm_set_bb_reg(dm, R_0x4130, MASKDWORD, 0x70ef0001);
	odm_set_bb_reg(dm, R_0x4130, MASKDWORD, 0x70ff0001);
	odm_set_bb_reg(dm, R_0x4130, MASKDWORD, 0x70ff0001);
#endif

	/*RX_IQK_PSD_8822E_S0_20220211*/
	odm_set_bb_reg(dm, R_0x1b04, MASKDWORD, 0x00000000);
	odm_set_bb_reg(dm, R_0x1b08, MASKDWORD, 0x00000080);
	odm_set_bb_reg(dm, R_0x1b0c, 0x00000c00, 0x3);
	odm_set_bb_reg(dm, R_0x1b14, MASKDWORD, 0x00000000);
	odm_set_bb_reg(dm, R_0x1b18, MASKDWORD, 0x00000001);
	odm_set_bb_reg(dm, R_0x1b1c, MASKDWORD, 0x821e3d21);
	odm_set_bb_reg(dm, R_0x1b20, 0x20000000, 0x0);
	odm_set_bb_reg(dm, R_0x1b28, MASKDWORD, 0x00000000);
	odm_set_bb_reg(dm, R_0x1bcc, 0x0000003f, 0x3f);
}

void _halrf_psd_iqk_reload_8822e(struct dm_struct *dm)
{
	/*10_8822E_AFE_for_IQK_restore*/
	odm_set_bb_reg(dm, R_0x1c38, MASKDWORD, 0x00000000);
	odm_set_bb_reg(dm, R_0x1c38, MASKDWORD, 0xffa1005e);
#if 0
	odm_set_bb_reg(dm, R_0x1830, MASKDWORD, 0x700b8041);
	odm_set_bb_reg(dm, R_0x1830, MASKDWORD, 0x70144041);
	odm_set_bb_reg(dm, R_0x1830, MASKDWORD, 0x70244041);
	odm_set_bb_reg(dm, R_0x1830, MASKDWORD, 0x70344041);
	odm_set_bb_reg(dm, R_0x1830, MASKDWORD, 0x70444041);
	odm_set_bb_reg(dm, R_0x1830, MASKDWORD, 0x705b8041);
	odm_set_bb_reg(dm, R_0x1830, MASKDWORD, 0x70644041);
	odm_set_bb_reg(dm, R_0x1830, MASKDWORD, 0x707b8041);
	odm_set_bb_reg(dm, R_0x1830, MASKDWORD, 0x708b8041);
	odm_set_bb_reg(dm, R_0x1830, MASKDWORD, 0x709b8041);
	odm_set_bb_reg(dm, R_0x1830, MASKDWORD, 0x70ab8041);
	odm_set_bb_reg(dm, R_0x1830, MASKDWORD, 0x70bb8041);
	odm_set_bb_reg(dm, R_0x1830, MASKDWORD, 0x70cb8041);
	odm_set_bb_reg(dm, R_0x1830, MASKDWORD, 0x70db8041);
	odm_set_bb_reg(dm, R_0x1830, MASKDWORD, 0x70eb8041);
	odm_set_bb_reg(dm, R_0x1830, MASKDWORD, 0x70fb8041);
	odm_set_bb_reg(dm, R_0x4130, MASKDWORD, 0x700b8041);
	odm_set_bb_reg(dm, R_0x4130, MASKDWORD, 0x70144041);
	odm_set_bb_reg(dm, R_0x4130, MASKDWORD, 0x70244041);
	odm_set_bb_reg(dm, R_0x4130, MASKDWORD, 0x70344041);
	odm_set_bb_reg(dm, R_0x4130, MASKDWORD, 0x70444041);
	odm_set_bb_reg(dm, R_0x4130, MASKDWORD, 0x705b8041);
	odm_set_bb_reg(dm, R_0x4130, MASKDWORD, 0x70644041);
	odm_set_bb_reg(dm, R_0x4130, MASKDWORD, 0x707b8041);
	odm_set_bb_reg(dm, R_0x4130, MASKDWORD, 0x708b8041);
	odm_set_bb_reg(dm, R_0x4130, MASKDWORD, 0x709b8041);
	odm_set_bb_reg(dm, R_0x4130, MASKDWORD, 0x70ab8041);
	odm_set_bb_reg(dm, R_0x4130, MASKDWORD, 0x70bb8041);
	odm_set_bb_reg(dm, R_0x4130, MASKDWORD, 0x70cb8041);
	odm_set_bb_reg(dm, R_0x4130, MASKDWORD, 0x70db8041);
	odm_set_bb_reg(dm, R_0x4130, MASKDWORD, 0x70eb8041);
	odm_set_bb_reg(dm, R_0x4130, MASKDWORD, 0x70fb8041);
#endif

	/*11_8822E_BB_for_IQK_restore*/
	/*odm_set_bb_reg(dm, R_0x1b00, 0x00000006, 0x0);*/
	odm_set_bb_reg(dm, R_0x1b08, MASKDWORD, 0x00000000);
	odm_set_bb_reg(dm, R_0x1c68, 0x0f000000, 0x0);
	odm_set_bb_reg(dm, R_0x1d0c, 0x00010000, 0x1);
	odm_set_bb_reg(dm, R_0x1d0c, 0x00010000, 0x0);
	odm_set_bb_reg(dm, R_0x1d0c, 0x00010000, 0x1);
	odm_set_bb_reg(dm, R_0x1864, 0x80000000, 0x0);
	odm_set_bb_reg(dm, R_0x4164, 0x80000000, 0x0);
	odm_set_bb_reg(dm, R_0x180c, 0x08000000, 0x0);
	odm_set_bb_reg(dm, R_0x410c, 0x08000000, 0x0);
	odm_set_bb_reg(dm, R_0x186c, 0x00000080, 0x0);
	odm_set_bb_reg(dm, R_0x416c, 0x00000080, 0x0);
	odm_set_bb_reg(dm, R_0x180c, 0x00000003, 0x3);
	odm_set_bb_reg(dm, R_0x410c, 0x00000003, 0x3);
	odm_set_bb_reg(dm, R_0x1a00, 0x00000003, 0x0);
}



#if (RTL8814C_SUPPORT == 1)
void _halrf_iqk_psd_enterpsd_8814c(struct dm_struct *dm)
{
	struct _hal_rf_ *rf = &(dm->rf_table);
	struct _halrf_psd_data *psd = &(rf->halrf_psd_data);

	u32 backup_bb_reg_8814c[18] = {0x1e24, 0x1cd0, 0x1b08, 0x1d58, 0x1834,
					0x4134, 0x5234, 0x5334, 0x180c, 0x410c,
					0x520c, 0x530c, 0x186c, 0x416c, 0x526c,
					0x536c, 0x1a00, 0x1c38};

	backup_bb_register(dm, psd->bb_backup, backup_bb_reg_8814c, 18);

	odm_set_bb_reg(dm, 0x1e24, 0x00020000, 0x1);
	odm_set_bb_reg(dm, 0x1cd0, 0x10000000, 0x1);
	odm_set_bb_reg(dm, 0x1cd0, 0x20000000, 0x1);
	odm_set_bb_reg(dm, 0x1cd0, 0x40000000, 0x1);
	odm_set_bb_reg(dm, 0x1cd0, 0x80000000, 0x0);
	odm_set_bb_reg(dm, 0x1b08, 0xffffffff, 0x00000080);
	odm_set_bb_reg(dm, 0x1d58, 0x00000ff8, 0x1ff);
	odm_set_bb_reg(dm, 0x1834, 0x00008000, 0x1);
	odm_set_bb_reg(dm, 0x4134, 0x00008000, 0x1);
	odm_set_bb_reg(dm, 0x5234, 0x00008000, 0x1);
	odm_set_bb_reg(dm, 0x5334, 0x00008000, 0x1);
	odm_set_bb_reg(dm, 0x180c, 0x08000000, 0x1);
	odm_set_bb_reg(dm, 0x410c, 0x08000000, 0x1);
	odm_set_bb_reg(dm, 0x520c, 0x08000000, 0x1);
	odm_set_bb_reg(dm, 0x530c, 0x08000000, 0x1);
	odm_set_bb_reg(dm, 0x186c, 0x00000080, 0x1);
	odm_set_bb_reg(dm, 0x416c, 0x00000080, 0x1);
	odm_set_bb_reg(dm, 0x526c, 0x00000080, 0x1);
	odm_set_bb_reg(dm, 0x536c, 0x00000080, 0x1);
	odm_set_bb_reg(dm, 0x180c, 0x00000003, 0x0);
	odm_set_bb_reg(dm, 0x410c, 0x00000003, 0x0);
	odm_set_bb_reg(dm, 0x520c, 0x00000003, 0x0);
	odm_set_bb_reg(dm, 0x530c, 0x00000003, 0x0);
	odm_set_bb_reg(dm, 0x1a00, 0x00000003, 0x2);
	odm_set_bb_reg(dm, 0x1c38, 0xffffffff, 0xffffffff);
	odm_set_bb_reg(dm, 0x1830, 0xffffffff, 0x700f0001);
	odm_set_bb_reg(dm, 0x1830, 0xffffffff, 0x700f0001);
	odm_set_bb_reg(dm, 0x1830, 0xffffffff, 0x701f0001);
	odm_set_bb_reg(dm, 0x1830, 0xffffffff, 0x702f0001);
	odm_set_bb_reg(dm, 0x1830, 0xffffffff, 0x703f0001);
	odm_set_bb_reg(dm, 0x1830, 0xffffffff, 0x704f0001);
	odm_set_bb_reg(dm, 0x1830, 0xffffffff, 0x705f0001);
	odm_set_bb_reg(dm, 0x1830, 0xffffffff, 0x706f0001);
	odm_set_bb_reg(dm, 0x1830, 0xffffffff, 0x707f0001);
	odm_set_bb_reg(dm, 0x1830, 0xffffffff, 0x708f0001);
	odm_set_bb_reg(dm, 0x1830, 0xffffffff, 0x709f0001);
	odm_set_bb_reg(dm, 0x1830, 0xffffffff, 0x70af0001);
	odm_set_bb_reg(dm, 0x1830, 0xffffffff, 0x70bf0001);
	odm_set_bb_reg(dm, 0x1830, 0xffffffff, 0x70cf0001);
	odm_set_bb_reg(dm, 0x1830, 0xffffffff, 0x70df0001);
	odm_set_bb_reg(dm, 0x1830, 0xffffffff, 0x70ef0001);
	odm_set_bb_reg(dm, 0x1830, 0xffffffff, 0x70ff0001);
	odm_set_bb_reg(dm, 0x1830, 0xffffffff, 0x70ff0001);
	odm_set_bb_reg(dm, 0x4130, 0xffffffff, 0x700f0001);
	odm_set_bb_reg(dm, 0x4130, 0xffffffff, 0x700f0001);
	odm_set_bb_reg(dm, 0x4130, 0xffffffff, 0x701f0001);
	odm_set_bb_reg(dm, 0x4130, 0xffffffff, 0x702f0001);
	odm_set_bb_reg(dm, 0x4130, 0xffffffff, 0x703f0001);
	odm_set_bb_reg(dm, 0x4130, 0xffffffff, 0x704f0001);
	odm_set_bb_reg(dm, 0x4130, 0xffffffff, 0x705f0001);
	odm_set_bb_reg(dm, 0x4130, 0xffffffff, 0x706f0001);
	odm_set_bb_reg(dm, 0x4130, 0xffffffff, 0x707f0001);
	odm_set_bb_reg(dm, 0x4130, 0xffffffff, 0x708f0001);
	odm_set_bb_reg(dm, 0x4130, 0xffffffff, 0x709f0001);
	odm_set_bb_reg(dm, 0x4130, 0xffffffff, 0x70af0001);
	odm_set_bb_reg(dm, 0x4130, 0xffffffff, 0x70bf0001);
	odm_set_bb_reg(dm, 0x4130, 0xffffffff, 0x70cf0001);
	odm_set_bb_reg(dm, 0x4130, 0xffffffff, 0x70df0001);
	odm_set_bb_reg(dm, 0x4130, 0xffffffff, 0x70ef0001);
	odm_set_bb_reg(dm, 0x4130, 0xffffffff, 0x70ff0001);
	odm_set_bb_reg(dm, 0x4130, 0xffffffff, 0x70ff0001);
	odm_set_bb_reg(dm, 0x5230, 0xffffffff, 0x700f0001);
	odm_set_bb_reg(dm, 0x5230, 0xffffffff, 0x700f0001);
	odm_set_bb_reg(dm, 0x5230, 0xffffffff, 0x701f0001);
	odm_set_bb_reg(dm, 0x5230, 0xffffffff, 0x702f0001);
	odm_set_bb_reg(dm, 0x5230, 0xffffffff, 0x703f0001);
	odm_set_bb_reg(dm, 0x5230, 0xffffffff, 0x704f0001);
	odm_set_bb_reg(dm, 0x5230, 0xffffffff, 0x705f0001);
	odm_set_bb_reg(dm, 0x5230, 0xffffffff, 0x706f0001);
	odm_set_bb_reg(dm, 0x5230, 0xffffffff, 0x707f0001);
	odm_set_bb_reg(dm, 0x5230, 0xffffffff, 0x708f0001);
	odm_set_bb_reg(dm, 0x5230, 0xffffffff, 0x709f0001);
	odm_set_bb_reg(dm, 0x5230, 0xffffffff, 0x70af0001);
	odm_set_bb_reg(dm, 0x5230, 0xffffffff, 0x70bf0001);
	odm_set_bb_reg(dm, 0x5230, 0xffffffff, 0x70cf0001);
	odm_set_bb_reg(dm, 0x5230, 0xffffffff, 0x70df0001);
	odm_set_bb_reg(dm, 0x5230, 0xffffffff, 0x70ef0001);
	odm_set_bb_reg(dm, 0x5230, 0xffffffff, 0x70ff0001);
	odm_set_bb_reg(dm, 0x5230, 0xffffffff, 0x70ff0001);
	odm_set_bb_reg(dm, 0x5330, 0xffffffff, 0x700f0001);
	odm_set_bb_reg(dm, 0x5330, 0xffffffff, 0x700f0001);
	odm_set_bb_reg(dm, 0x5330, 0xffffffff, 0x701f0001);
	odm_set_bb_reg(dm, 0x5330, 0xffffffff, 0x702f0001);
	odm_set_bb_reg(dm, 0x5330, 0xffffffff, 0x703f0001);
	odm_set_bb_reg(dm, 0x5330, 0xffffffff, 0x704f0001);
	odm_set_bb_reg(dm, 0x5330, 0xffffffff, 0x705f0001);
	odm_set_bb_reg(dm, 0x5330, 0xffffffff, 0x706f0001);
	odm_set_bb_reg(dm, 0x5330, 0xffffffff, 0x707f0001);
	odm_set_bb_reg(dm, 0x5330, 0xffffffff, 0x708f0001);
	odm_set_bb_reg(dm, 0x5330, 0xffffffff, 0x709f0001);
	odm_set_bb_reg(dm, 0x5330, 0xffffffff, 0x70af0001);
	odm_set_bb_reg(dm, 0x5330, 0xffffffff, 0x70bf0001);
	odm_set_bb_reg(dm, 0x5330, 0xffffffff, 0x70cf0001);
	odm_set_bb_reg(dm, 0x5330, 0xffffffff, 0x70df0001);
	odm_set_bb_reg(dm, 0x5330, 0xffffffff, 0x70ef0001);
	odm_set_bb_reg(dm, 0x5330, 0xffffffff, 0x70ff0001);
	odm_set_bb_reg(dm, 0x5330, 0xffffffff, 0x70ff0001);
	odm_set_bb_reg(dm, 0x1b1c, 0xffffffff, 0x821a3d21);
	odm_set_bb_reg(dm, 0x1b18, 0x00000001, 0x00000001);	
}
#endif

#if (RTL8814C_SUPPORT == 1)
void _halrf_iqk_psd_reload_8814c(struct dm_struct *dm)
{
	struct _hal_rf_ *rf = &(dm->rf_table);
	struct _halrf_psd_data *psd = &(rf->halrf_psd_data);

	u32 backup_bb_reg_8814c[18] = {0x1e24, 0x1cd0, 0x1b08, 0x1d58, 0x1834,
					0x4134, 0x5234, 0x5334, 0x180c, 0x410c,
					0x520c, 0x530c, 0x186c, 0x416c, 0x526c,
					0x536c, 0x1a00, 0x1c38};

	odm_set_bb_reg(dm, 0x1b08, 0xffffffff, 0x00000000);
	odm_set_bb_reg(dm, 0x1c38, 0xffffffff, 0xffa1005e);
	odm_set_bb_reg(dm, 0x1830, 0xffffffff, 0x700b8041);
	odm_set_bb_reg(dm, 0x1830, 0xffffffff, 0x70144041);
	odm_set_bb_reg(dm, 0x1830, 0xffffffff, 0x70244041);
	odm_set_bb_reg(dm, 0x1830, 0xffffffff, 0x70344041);
	odm_set_bb_reg(dm, 0x1830, 0xffffffff, 0x70444041);
	odm_set_bb_reg(dm, 0x1830, 0xffffffff, 0x705b8041);
	odm_set_bb_reg(dm, 0x1830, 0xffffffff, 0x70644041);
	odm_set_bb_reg(dm, 0x1830, 0xffffffff, 0x707b8041);
	odm_set_bb_reg(dm, 0x1830, 0xffffffff, 0x708b8041);
	odm_set_bb_reg(dm, 0x1830, 0xffffffff, 0x709b8041);
	odm_set_bb_reg(dm, 0x1830, 0xffffffff, 0x70ab8041);
	odm_set_bb_reg(dm, 0x1830, 0xffffffff, 0x70bb8041);
	odm_set_bb_reg(dm, 0x1830, 0xffffffff, 0x70cb8041);
	odm_set_bb_reg(dm, 0x1830, 0xffffffff, 0x70db8041);
	odm_set_bb_reg(dm, 0x1830, 0xffffffff, 0x70eb8041);
	odm_set_bb_reg(dm, 0x1830, 0xffffffff, 0x70fb8041);
	odm_set_bb_reg(dm, 0x4130, 0xffffffff, 0x700b8041);
	odm_set_bb_reg(dm, 0x4130, 0xffffffff, 0x70144041);
	odm_set_bb_reg(dm, 0x4130, 0xffffffff, 0x70244041);
	odm_set_bb_reg(dm, 0x4130, 0xffffffff, 0x70344041);
	odm_set_bb_reg(dm, 0x4130, 0xffffffff, 0x70444041);
	odm_set_bb_reg(dm, 0x4130, 0xffffffff, 0x705b8041);
	odm_set_bb_reg(dm, 0x4130, 0xffffffff, 0x70644041);
	odm_set_bb_reg(dm, 0x4130, 0xffffffff, 0x707b8041);
	odm_set_bb_reg(dm, 0x4130, 0xffffffff, 0x708b8041);
	odm_set_bb_reg(dm, 0x4130, 0xffffffff, 0x709b8041);
	odm_set_bb_reg(dm, 0x4130, 0xffffffff, 0x70ab8041);
	odm_set_bb_reg(dm, 0x4130, 0xffffffff, 0x70bb8041);
	odm_set_bb_reg(dm, 0x4130, 0xffffffff, 0x70cb8041);
	odm_set_bb_reg(dm, 0x4130, 0xffffffff, 0x70db8041);
	odm_set_bb_reg(dm, 0x4130, 0xffffffff, 0x70eb8041);
	odm_set_bb_reg(dm, 0x4130, 0xffffffff, 0x70fb8041);
	odm_set_bb_reg(dm, 0x5230, 0xffffffff, 0x700b8041);
	odm_set_bb_reg(dm, 0x5230, 0xffffffff, 0x70144041);
	odm_set_bb_reg(dm, 0x5230, 0xffffffff, 0x70244041);
	odm_set_bb_reg(dm, 0x5230, 0xffffffff, 0x70344041);
	odm_set_bb_reg(dm, 0x5230, 0xffffffff, 0x70444041);
	odm_set_bb_reg(dm, 0x5230, 0xffffffff, 0x705b8041);
	odm_set_bb_reg(dm, 0x5230, 0xffffffff, 0x70644041);
	odm_set_bb_reg(dm, 0x5230, 0xffffffff, 0x707b8041);
	odm_set_bb_reg(dm, 0x5230, 0xffffffff, 0x708b8041);
	odm_set_bb_reg(dm, 0x5230, 0xffffffff, 0x709b8041);
	odm_set_bb_reg(dm, 0x5230, 0xffffffff, 0x70ab8041);
	odm_set_bb_reg(dm, 0x5230, 0xffffffff, 0x70bb8041);
	odm_set_bb_reg(dm, 0x5230, 0xffffffff, 0x70cb8041);
	odm_set_bb_reg(dm, 0x5230, 0xffffffff, 0x70db8041);
	odm_set_bb_reg(dm, 0x5230, 0xffffffff, 0x70eb8041);
	odm_set_bb_reg(dm, 0x5230, 0xffffffff, 0x70fb8041);
	odm_set_bb_reg(dm, 0x5330, 0xffffffff, 0x700b8041);
	odm_set_bb_reg(dm, 0x5330, 0xffffffff, 0x70144041);
	odm_set_bb_reg(dm, 0x5330, 0xffffffff, 0x70244041);
	odm_set_bb_reg(dm, 0x5330, 0xffffffff, 0x70344041);
	odm_set_bb_reg(dm, 0x5330, 0xffffffff, 0x70444041);
	odm_set_bb_reg(dm, 0x5330, 0xffffffff, 0x705b8041);
	odm_set_bb_reg(dm, 0x5330, 0xffffffff, 0x70644041);
	odm_set_bb_reg(dm, 0x5330, 0xffffffff, 0x707b8041);
	odm_set_bb_reg(dm, 0x5330, 0xffffffff, 0x708b8041);
	odm_set_bb_reg(dm, 0x5330, 0xffffffff, 0x709b8041);
	odm_set_bb_reg(dm, 0x5330, 0xffffffff, 0x70ab8041);
	odm_set_bb_reg(dm, 0x5330, 0xffffffff, 0x70bb8041);
	odm_set_bb_reg(dm, 0x5330, 0xffffffff, 0x70cb8041);
	odm_set_bb_reg(dm, 0x5330, 0xffffffff, 0x70db8041);
	odm_set_bb_reg(dm, 0x5330, 0xffffffff, 0x70eb8041);
	odm_set_bb_reg(dm, 0x5330, 0xffffffff, 0x70fb8041);
	odm_set_bb_reg(dm, 0x1d58, 0x00000ff8, 0x000);
	odm_set_bb_reg(dm, 0x1834, 0x00008000, 0x0);
	odm_set_bb_reg(dm, 0x4134, 0x00008000, 0x0);
	odm_set_bb_reg(dm, 0x5234, 0x00008000, 0x0);
	odm_set_bb_reg(dm, 0x5334, 0x00008000, 0x0);
	odm_set_bb_reg(dm, 0x180c, 0x08000000, 0x0);
	odm_set_bb_reg(dm, 0x410c, 0x08000000, 0x0);
	odm_set_bb_reg(dm, 0x520c, 0x08000000, 0x0);
	odm_set_bb_reg(dm, 0x530c, 0x08000000, 0x0);
	odm_set_bb_reg(dm, 0x186c, 0x00000080, 0x0);
	odm_set_bb_reg(dm, 0x416c, 0x00000080, 0x0);
	odm_set_bb_reg(dm, 0x526c, 0x00000080, 0x0);
	odm_set_bb_reg(dm, 0x536c, 0x00000080, 0x0);
	odm_set_bb_reg(dm, 0x180c, 0x00000003, 0x3);
	odm_set_bb_reg(dm, 0x410c, 0x00000003, 0x3);
	odm_set_bb_reg(dm, 0x520c, 0x00000003, 0x3);
	odm_set_bb_reg(dm, 0x530c, 0x00000003, 0x3);
	odm_set_bb_reg(dm, 0x1a00, 0x00000003, 0x0);

	restore_bb_register(dm, psd->bb_backup, backup_bb_reg_8814c, 18);
}
#endif

u64 halrf_get_iqk_psd_data(void *dm_void, u32 point)
{
	struct dm_struct *dm = (struct dm_struct *)dm_void;
	struct _hal_rf_ *rf = &(dm->rf_table);
	struct _halrf_psd_data *psd = &(rf->halrf_psd_data);
	u64 psd_val, psd_val1, psd_val2;
	u32 i, delay_time = 0;

#if (DEV_BUS_TYPE == RT_USB_INTERFACE) || (DEV_BUS_TYPE == RT_SDIO_INTERFACE)
	if (dm->support_interface == ODM_ITRF_USB || dm->support_interface == ODM_ITRF_SDIO) {
		if (dm->support_ic_type & (ODM_RTL8822C | ODM_RTL8723F | ODM_RTL8814C | ODM_RTL8822E))
			delay_time = 2000;
		else
			delay_time = 0;
	}
#endif
#if (DEV_BUS_TYPE == RT_PCI_INTERFACE)
	if (dm->support_interface == ODM_ITRF_PCIE) {
		if (dm->support_ic_type & (ODM_RTL8822C | ODM_RTL8814C | ODM_RTL8822E))
			delay_time = 1000;
		else
			delay_time = 150;
	}
#endif

	if (dm->support_ic_type & ODM_RTL8735B)
		delay_time = 1000;

	if (dm->support_ic_type & ODM_RTL8730A)
		delay_time = 1000;

	odm_set_bb_reg(dm, R_0x1b2c, 0x0fff0000, (point & 0xfff));

	odm_set_bb_reg(dm, R_0x1b34, BIT(0), 0x1);

	odm_set_bb_reg(dm, R_0x1b34, BIT(0), 0x0);

	for (i = 0; i < delay_time; i++)
		ODM_delay_us(1);

	if (dm->support_ic_type & (ODM_RTL8197G | ODM_RTL8198F)) {
		if (dm->support_ic_type & ODM_RTL8197G)
			odm_set_bb_reg(dm, R_0x1bd4, MASKDWORD, 0x001a0001);
		else
			odm_set_bb_reg(dm, R_0x1bd4, MASKDWORD, 0x00250001);

		psd_val1 = odm_get_bb_reg(dm, R_0x1bfc, MASKDWORD);

		psd_val1 = (psd_val1 & 0x001f0000) >> 16;

		if (dm->support_ic_type & ODM_RTL8197G)
			odm_set_bb_reg(dm, R_0x1bd4, MASKDWORD, 0x001b0001);
		else
			odm_set_bb_reg(dm, R_0x1bd4, MASKDWORD, 0x002e0001);

		psd_val2 = odm_get_bb_reg(dm, R_0x1bfc, MASKDWORD);

		psd_val = (psd_val1 << 27) + (psd_val2 >> 5);
	} else if (dm->support_ic_type & ODM_RTL8723F) {
		odm_set_bb_reg(dm, R_0x1bd4, MASKDWORD, 0x00210001);
		psd_val1 = odm_get_bb_reg(dm, R_0x1bfc, MASKDWORD);
		psd_val1 = (psd_val1 & 0x00FF0000) >> 16;
		odm_set_bb_reg(dm, R_0x1bd4, MASKDWORD, 0x00220001);
		psd_val2 = odm_get_bb_reg(dm, R_0x1bfc, MASKDWORD);
		//psd_val = (psd_val1 << 27) + (psd_val2 >> 5);
		psd_val = (psd_val1 << 32) + psd_val2;
	} else if (dm->support_ic_type & ODM_RTL8730A) {
		odm_set_bb_reg(dm, R_0x1bd4, MASKDWORD, 0x00210001);
		psd_val1 = odm_get_bb_reg(dm, R_0x1bfc, MASKDWORD);
		psd_val1 = (psd_val1 & 0x00FF0000) >> 16;
		odm_set_bb_reg(dm, R_0x1bd4, MASKDWORD, 0x00220001);
		psd_val2 = odm_get_bb_reg(dm, R_0x1bfc, MASKDWORD);
		//psd_val = (psd_val1 << 27) + (psd_val2 >> 5);
		psd_val = (psd_val1 << 32) + psd_val2;
	} else if (dm->support_ic_type & ODM_RTL8735B) {
		odm_set_bb_reg(dm, R_0x1bd4, MASKDWORD, 0x00210001);
		psd_val1 = odm_get_bb_reg(dm, R_0x1bfc, 0x00ff0000);
		odm_set_bb_reg(dm, R_0x1bd4, MASKDWORD, 0x00220001);
		psd_val2 = odm_get_bb_reg(dm, R_0x1bfc, MASKDWORD);
		//psd_val = (psd_val1 << 24) + (psd_val2 >> 8);
		psd_val = (psd_val1 << 32) + psd_val2;
	} else {
		odm_set_bb_reg(dm, R_0x1bd4, MASKDWORD, 0x00250001);
		psd_val1 = odm_get_bb_reg(dm, R_0x1bfc, 0x07ff0000);
		odm_set_bb_reg(dm, R_0x1bd4, MASKDWORD, 0x002e0001);
		psd_val2 = odm_get_bb_reg(dm, R_0x1bfc, MASKDWORD);
		psd_val = (psd_val1 << 21) + (psd_val2 >> 11);
	}

	return psd_val;
}

void halrf_iqk_psd(
	struct dm_struct *dm,
	u32 point,
	u32 start_point,
	u32 stop_point,
	u32 average)
{
	struct _hal_rf_ *rf = &(dm->rf_table);
	struct _halrf_psd_data *psd = &(rf->halrf_psd_data);

	u32 i = 0, j = 0, k = 0;
	u32 psd_reg, avg_org, point_temp, average_tmp = 32, mode, reg_tmp = 5;
	u64 data_tatal = 0, data_temp[64] = {0};
	s32 s_point_tmp;

#if (RTL8735B_SUPPORT == 1 || RTL8730A_SUPPORT == 1)
	psd->buf_size = 320;
#else
	psd->buf_size = 256;
#endif

	mode = average >> 16;

	if (mode == 2) {
		if (dm->support_ic_type & (ODM_RTL8822C | ODM_RTL8723F | ODM_RTL8814C |
					ODM_RTL8735B | ODM_RTL8730A | ODM_RTL8822E))
			average_tmp = 1; //HW average
		else {
			reg_tmp = odm_get_bb_reg(dm, R_0x1b1c, 0x000e0000);
			if (reg_tmp == 0)
				average_tmp = 1;
			else if (reg_tmp == 3)
				average_tmp = 8;
			else if (reg_tmp == 4)
				average_tmp = 16;
			else if (reg_tmp == 5)
				average_tmp = 32;
			odm_set_bb_reg(dm, R_0x1b1c, 0x000e0000, 0x0);
		}
	} else {
		reg_tmp = odm_get_bb_reg(dm, R_0x1b1c, 0x000e0000);
		if (reg_tmp == 0)
			average_tmp = 1;
		else if (reg_tmp == 3)
			average_tmp = 8;
		else if (reg_tmp == 4)
			average_tmp = 16;
		else if (reg_tmp == 5)
			average_tmp = 32;
		if (!((dm->support_ic_type & ODM_RTL8723F)&&(dm->support_ic_type & ODM_RTL8730A)))
			odm_set_bb_reg(dm, R_0x1b1c, 0x000e0000, 0x0);
	}

#if 0
	DbgPrint("[PSD]point=%d, start_point=%d, stop_point=%d, average=0x%x, average_tmp=%d, buf_size=%d, mode=%d\n",
				point, start_point, stop_point, average, average_tmp, psd->buf_size, mode);
#endif

#if (DM_ODM_SUPPORT_TYPE & ODM_IOT)
#ifdef CONFIG_MP_INCLUDED
	for (i = 0; i < psd->buf_size; i++)
		psd->psd_data[i] = 0;
#endif
#else
	for (i = 0; i < psd->buf_size; i++)
		psd->psd_data[i] = 0;
#endif


	i = start_point;

	if ((dm->support_ic_type & ODM_RTL8723F)||(dm->support_ic_type & ODM_RTL8730A)) {
		while (i < stop_point) {
			data_tatal = 0;

			if (i >= point)
				point_temp = i - point;
			else
				point_temp = i + 0xB00;
			//-640:0xD80,640:0x280,0x280+0xB00 =0xD80
				//point_temp = i + 0xC00;
			//-512:0xE00,512:0x200,0x200+0xC00 = 0xE00
			/*for (k = 0; k < average_tmp; k++) {
				data_temp[k] = halrf_get_iqk_psd_data(dm, point_temp);
				data_tatal = data_tatal + data_temp[k];
			}*/

			//data_tatal = phydm_division64((data_tatal * 10), average_tmp);
			
			data_tatal = halrf_get_iqk_psd_data(dm, point_temp);
#if (DM_ODM_SUPPORT_TYPE & ODM_IOT)
#ifdef CONFIG_MP_INCLUDED

			if (dm->support_ic_type & ODM_RTL8730A)
				psd->psd_data_64[j] = data_tatal;
			else
				psd->psd_data[j] = (u32)data_tatal;
#endif
#else
			
			psd->psd_data[j] = (u32)data_tatal;
#endif
			i++;
			j++;
		}
	} else {
		while (i < stop_point) {
			data_tatal = 0;

			if (i >= point)
				point_temp = i - point;
			else
			{
				if (dm->support_ic_type &
					(ODM_RTL8814B | ODM_RTL8814C | ODM_RTL8735B |
					ODM_RTL8822E))
				{
					/*s_point_tmp = i - point - 1;*/
					s_point_tmp = i - point;
					point_temp = s_point_tmp & 0xfff;
				}
				else
					point_temp = i;
			}

			for (k = 0; k < average_tmp; k++) {
				data_temp[k] = halrf_get_iqk_psd_data(dm, point_temp);
				/*data_tatal = data_tatal + (data_temp[k] * data_temp[k]);*/
				data_tatal = data_tatal + data_temp[k];

#if 0
				if ((k % 20) == 0)
					DbgPrint("\n ");

				DbgPrint("0x%x ", data_temp[k]);
#endif
			}

			data_tatal = phydm_division64((data_tatal * 10), average_tmp);
#if (DM_ODM_SUPPORT_TYPE & ODM_IOT)
#ifdef CONFIG_MP_INCLUDED

			if (dm->support_ic_type & ODM_RTL8735B)
				psd->psd_data_64[j] = data_tatal;
			else
				psd->psd_data[j] = (u32)data_tatal;
#endif
#else
			psd->psd_data[j] = (u32)data_tatal;
#endif

			i++;
			j++;
		}

		if (dm->support_ic_type & (ODM_RTL8814B | ODM_RTL8198F | ODM_RTL8197G))
			odm_set_bb_reg(dm, R_0x1b1c, 0x000e0000, reg_tmp);
	} 
	
#if 0
	DbgPrint("\n [iqk psd]psd result:\n");

	for (i = 0; i < psd->buf_size; i++) {
		if ((i % 20) == 0)
		DbgPrint("\n ");

		DbgPrint("0x%x ", psd->psd_data[i]);
	}
	DbgPrint("\n\n");
#endif
}


u32
halrf_psd_init(void *dm_void)
{
	enum rt_status ret_status = RT_STATUS_SUCCESS;
	struct dm_struct *dm = (struct dm_struct *)dm_void;
	struct _hal_rf_ *rf = &(dm->rf_table);
	struct _halrf_psd_data *psd = &(rf->halrf_psd_data);
	u32 bb_backup[29];

#if (RTL8814B_SUPPORT == 1 || RTL8822C_SUPPORT == 1 || RTL8812F_SUPPORT == 1 || \
	RTL8198F_SUPPORT == 1 || RTL8197G_SUPPORT == 1)
	u32 backup_bb_reg[12] = {0x1b04, 0x1b08, 0x1b0c, 0x1b14, 0x1b18,
				0x1b1c, 0x1b28, 0x1bcc, 0x1b2c, 0x1b34,
				0x1bd4, 0x1bfc};
#endif

#if (RTL8723F_SUPPORT == 1)
	u32 backup_bb_reg_8723f[11] = {0x09f0, 0x09b4, 0x1c38, 0x1860, 0x1cd0,
				 	0x824, 0x2a24, 0x1d40, 0x1c20, 0x1880,
				 	0x180c};
#endif

#if (RTL8730A_SUPPORT == 1)
	u32 backup_bb_reg_8730a[29] = {0x1b08, 0x1e24, 0x1e28, 0x0824, 0x1cd0,
					0x2a24, 0x1c68, 0x1864, 0x180c, 0x1c38,
					0x1810, 0x1814, 0x1830, 0x1860, 0x09f0,
					0x1d40, 0x09b4, 0x1c20, 0x1b80, 0x1b00,
					0x1b18,	0x1b14, 0x1b1c, 0x1b0c, 0x1bcc,
					0x1b2c,	0x1b34, 0x1bd4, 0x1bfc};
#endif

#if (RTL8814C_SUPPORT == 1)
	u32 backup_bb_reg_8814c[18] = {0x1e24, 0x1cd0, 0x1b08, 0x1d58, 0x1834,
					0x4134, 0x5234, 0x5334, 0x180c, 0x410c,
					0x520c, 0x530c, 0x186c, 0x416c, 0x526c,
					0x536c, 0x1a00, 0x1c38};
#endif

#if (RTL8735B_SUPPORT == 1)
	u32 backup_bb_reg_8735b[29] = {0x1b08, 0x1e24, 0x1e28, 0x0824, 0x1cd0,
					0x2a24, 0x1c68, 0x1864, 0x180c, 0x1c38,
					0x1810, 0x1814, 0x1830, 0x1860, 0x09f0,
					0x1d40, 0x09b4, 0x1c20, 0x1b80, 0x1b00,
					0x1b18,	0x1b14, 0x1b1c, 0x1b0c, 0x1bcc,
					0x1b2c,	0x1b34, 0x1bd4, 0x1bfc};
#endif

#if (RTL8822E_SUPPORT == 1)
	u32 backup_bb_reg_8822e[24] = {0x1e24, 0x1cd0, 0x1c68, 0x1864, 0x4164,
					0x180c, 0x410c, 0x186c, 0x416c, 0x1a00,
					0x1b08, 0x1c38, 0x1b04, 0x1b0c, 0x1b14,
					0x1b18, 0x1b1c, 0x1b20, 0x1b28, 0x1bcc,
					0x1b2c,	0x1b34, 0x1bd4, 0x1bfc};
#endif


	if (psd->psd_progress) {
		ret_status = RT_STATUS_PENDING;
	} else {
		psd->psd_progress = 1;
#if (RTL8723F_SUPPORT == 1)
		if (dm->support_ic_type & ODM_RTL8723F) {
			backup_bb_register(dm, bb_backup, backup_bb_reg_8723f, 11);
			_halrf_iqk_psd_init_8723f(dm, true);
			halrf_iqk_psd(dm, psd->point, psd->start_point, psd->stop_point, psd->average);
			_halrf_iqk_psd_init_8723f(dm, false);
			restore_bb_register(dm, bb_backup, backup_bb_reg_8723f, 11);
		}
#endif

#if (RTL8730A_SUPPORT == 1)
		if (dm->support_ic_type & ODM_RTL8730A) {
			backup_bb_register(dm, bb_backup, backup_bb_reg_8730a, 29);
			_halrf_iqk_psd_init_8730a(dm, true);
			halrf_iqk_psd(dm, psd->point, psd->start_point, psd->stop_point, psd->average);
			_halrf_iqk_psd_init_8730a(dm, false);
			restore_bb_register(dm, bb_backup, backup_bb_reg_8730a, 29);
		}
#endif

#if (RTL8814C_SUPPORT == 1)
		if (dm->support_ic_type & ODM_RTL8814C) {
			backup_bb_register(dm, bb_backup, backup_bb_reg_8814c, 18);
			_halrf_psd_iqk_init_8814c(dm);
			halrf_iqk_psd(dm, psd->point, psd->start_point, psd->stop_point, psd->average);
			restore_bb_register(dm, bb_backup, backup_bb_reg_8814c, 18);
		}
#endif

#if (RTL8735B_SUPPORT == 1)
		if (dm->support_ic_type & ODM_RTL8735B) {
			backup_bb_register(dm, bb_backup, backup_bb_reg_8735b, 29);
			_halrf_psd_iqk_init_8735b(dm);
			halrf_iqk_psd(dm, psd->point, psd->start_point, psd->stop_point, psd->average);
			restore_bb_register(dm, bb_backup, backup_bb_reg_8735b, 29);
		} 
#endif

#if (RTL8822E_SUPPORT == 1)
		if (dm->support_ic_type & ODM_RTL8822E) {
			backup_bb_register(dm, bb_backup, backup_bb_reg_8822e, 24);
			_halrf_psd_iqk_init_8822e(dm);
			halrf_iqk_psd(dm, psd->point, psd->start_point, psd->stop_point, psd->average);
			_halrf_psd_iqk_reload_8822e(dm);
			restore_bb_register(dm, bb_backup, backup_bb_reg_8822e, 24);
		}
#endif

#if (RTL8822C_SUPPORT == 1 || RTL8812F_SUPPORT == 1 || RTL8814B_SUPPORT == 1 || \
	RTL8198F_SUPPORT == 1 || RTL8197G_SUPPORT == 1)
		if (dm->support_ic_type & 
			(ODM_RTL8822C | ODM_RTL8812F | ODM_RTL8814B | ODM_RTL8198F | ODM_RTL8197G)) {
			backup_bb_register(dm, bb_backup, backup_bb_reg, 12);
			_halrf_psd_iqk_init(dm);
			halrf_iqk_psd(dm, psd->point, psd->start_point, psd->stop_point, psd->average);
			restore_bb_register(dm, bb_backup, backup_bb_reg, 12);
		}
#endif

		if (!(dm->support_ic_type & 
			(ODM_RTL8822C | ODM_RTL8812F | ODM_RTL8814B | ODM_RTL8198F | ODM_RTL8197G |
			ODM_RTL8735B | ODM_RTL8730A | ODM_RTL8814C | ODM_RTL8723F | ODM_RTL8822E))) {
			halrf_psd(dm, psd->point, psd->start_point, psd->stop_point, psd->average);
		}
		psd->psd_progress = 0;
	}
	return ret_status;
}

u32
halrf_psd_query(
	void *dm_void,
	u32 *outbuf,
	u32 buf_size)
{
	enum rt_status ret_status = RT_STATUS_SUCCESS;
	struct dm_struct *dm = (struct dm_struct *)dm_void;
	struct _hal_rf_ *rf = &(dm->rf_table);
	struct _halrf_psd_data *psd = &(rf->halrf_psd_data);

	if (psd->psd_progress) {
		ret_status = RT_STATUS_PENDING;
	} else {

#if (DM_ODM_SUPPORT_TYPE & ODM_IOT)
#ifdef CONFIG_MP_INCLUDED
		if (dm->support_ic_type & (ODM_RTL8730A| ODM_RTL8735B)) {
			odm_move_memory(dm, outbuf, psd->psd_data_64,
					sizeof(u64) * buf_size);
		} else {
			odm_move_memory(dm, outbuf, psd->psd_data,
					sizeof(u32) * buf_size);
		}
#endif
#else
		odm_move_memory(dm, outbuf, psd->psd_data,
				sizeof(u32) * buf_size);
#endif
	}

	return ret_status;
}

u32
halrf_psd_init_query(
	void *dm_void,
	u32 *outbuf,
	u32 point,
	u32 start_point,
	u32 stop_point,
	u32 average,
	u32 buf_size)
{
	enum rt_status ret_status = RT_STATUS_SUCCESS;
	struct dm_struct *dm = (struct dm_struct *)dm_void;
	struct _hal_rf_ *rf = &(dm->rf_table);
	struct _halrf_psd_data *psd = &(rf->halrf_psd_data);

	psd->point = point;
	psd->start_point = start_point;
	psd->stop_point = stop_point;
	psd->average = average;

	if (psd->psd_progress) {
		ret_status = RT_STATUS_PENDING;
	} else {
		psd->psd_progress = 1;

#if (DM_ODM_SUPPORT_TYPE & ODM_IOT)
#ifdef CONFIG_MP_INCLUDED
		halrf_psd(dm, psd->point, psd->start_point, psd->stop_point, psd->average);
		odm_move_memory(dm, outbuf, psd->psd_data, 0x400);
#endif
#else
		halrf_psd(dm, psd->point, psd->start_point, psd->stop_point, psd->average);
		odm_move_memory(dm, outbuf, psd->psd_data, 0x400);
#endif
		psd->psd_progress = 0;
	}

	return ret_status;
}

void halrf_iqk_psd_init(void *dm_void)
{
	struct dm_struct *dm = (struct dm_struct *)dm_void;
	
#if (RTL8814C_SUPPORT == 1)
	if (dm->support_ic_type & (ODM_RTL8814C)) {
		_halrf_iqk_psd_enterpsd_8814c(dm);
		_halrf_psd_iqk_init_8814c(dm);
	}
#endif		
}

u32 halrf_iqk_psd_result(void *dm_void, u32 ch_freq, u32 spur_freq)
{
	struct dm_struct *dm = (struct dm_struct *)dm_void;
	
#if (RTL8814C_SUPPORT == 1)
	if (dm->support_ic_type & (ODM_RTL8814C)) {
		s32 point;
		point = ((32 * spur_freq) - (32 * ch_freq)) / 5;
		return (u32)halrf_get_iqk_psd_data(dm, (u32)(point & 0xfff));
	}
#endif
	return 0;
}

void halrf_iqk_psd_reload(void *dm_void)
{
	struct dm_struct *dm = (struct dm_struct *)dm_void;

#if (RTL8814C_SUPPORT == 1)
	if (dm->support_ic_type & (ODM_RTL8814C))
		_halrf_iqk_psd_reload_8814c(dm);
#endif
}
