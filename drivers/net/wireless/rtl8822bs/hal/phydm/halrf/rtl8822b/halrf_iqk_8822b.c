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

#include "mp_precomp.h"
#if (DM_ODM_SUPPORT_TYPE == ODM_WIN)
	#if RT_PLATFORM==PLATFORM_MACOSX
	#include "phydm_precomp.h"
	#else
	#include "../phydm_precomp.h"
	#endif
#else
#include "../../phydm_precomp.h"
#endif

#if (RTL8822B_SUPPORT == 1)


/*---------------------------Define Local Constant---------------------------*/


void phydm_get_read_counter(struct PHY_DM_STRUCT *p_dm)
{
	u32 counter = 0x0;

	while (1) {
		if ((odm_get_rf_reg(p_dm, RF_PATH_A, 0x8, RFREGOFFSETMASK) == 0xabcde) || (counter > 300))
			break;
		counter++;
		ODM_delay_ms(1);
	};
	odm_set_rf_reg(p_dm, RF_PATH_A, 0x8, RFREGOFFSETMASK, 0x0);
	ODM_RT_TRACE(p_dm, ODM_COMP_CALIBRATION, ODM_DBG_TRACE, ("[IQK]counter = %d\n", counter));
}

/*---------------------------Define Local Constant---------------------------*/


#if !(DM_ODM_SUPPORT_TYPE & ODM_AP)
void do_iqk_8822b(
	void		*p_dm_void,
	u8		delta_thermal_index,
	u8		thermal_value,
	u8		threshold
)
{
	struct PHY_DM_STRUCT	*p_dm = (struct PHY_DM_STRUCT *)p_dm_void;
	struct _IQK_INFORMATION	*p_iqk_info = &p_dm->IQK_info;
	
	p_dm->rf_calibrate_info.thermal_value_iqk = thermal_value;
	halrf_segment_iqk_trigger(p_dm, true, p_iqk_info->segment_iqk);
}
#else
/*Originally p_config->do_iqk is hooked phy_iq_calibrate_8822b, but do_iqk_8822b and phy_iq_calibrate_8822b have different arguments*/
void do_iqk_8822b(
	void		*p_dm_void,
	u8	delta_thermal_index,
	u8	thermal_value,
	u8	threshold
)
{
	struct PHY_DM_STRUCT	*p_dm = (struct PHY_DM_STRUCT *)p_dm_void;
	struct _IQK_INFORMATION	*p_iqk_info = &p_dm->IQK_info;
	boolean		is_recovery = (boolean) delta_thermal_index;

	halrf_segment_iqk_trigger(p_dm, true, p_iqk_info->segment_iqk);
}
#endif

void
_iqk_rf_set_check(
	struct PHY_DM_STRUCT	*p_dm,
	u8		path,
	u16		add,
	u32		data
	)
{
	 u32 i;

	odm_set_rf_reg(p_dm, (enum rf_path)path, add, RFREGOFFSETMASK, data);

	for (i = 0; i < 100; i++) {
		if (odm_get_rf_reg(p_dm, (enum rf_path)path, add, RFREGOFFSETMASK) == data)
			break;
		else {
			ODM_delay_us(10);
			odm_set_rf_reg(p_dm, (enum rf_path)path, add, RFREGOFFSETMASK, data);
		}
	}
}


void
_iqk_rf0xb0_workaround(
	struct PHY_DM_STRUCT	*p_dm
	)
{
	/*add 0xb8 control for the bad phase noise after switching channel*/
	odm_set_rf_reg(p_dm, (enum rf_path)0x0, 0xb8, RFREGOFFSETMASK, 0x00a00);
	odm_set_rf_reg(p_dm, (enum rf_path)0x0, 0xb8, RFREGOFFSETMASK, 0x80a00);
}

void
_iqk_fill_iqk_report_8822b(
	void		*p_dm_void,
	u8			channel
)
{
	struct PHY_DM_STRUCT	*p_dm = (struct PHY_DM_STRUCT *)p_dm_void;
	struct _IQK_INFORMATION	*p_iqk_info = &p_dm->IQK_info;
	u32		tmp1 = 0x0, tmp2 = 0x0, tmp3 = 0x0;
	u8		i;

	for (i = 0; i < SS_8822B; i++) {
		tmp1 = tmp1 + ((p_iqk_info->IQK_fail_report[channel][i][TX_IQK] & 0x1) << i);
		tmp2 = tmp2 + ((p_iqk_info->IQK_fail_report[channel][i][RX_IQK] & 0x1) << (i + 4));
		tmp3 = tmp3 + ((p_iqk_info->RXIQK_fail_code[channel][i] & 0x3) << (i * 2 + 8));
	}
	odm_write_4byte(p_dm, 0x1b00, 0xf8000008);
	odm_set_bb_reg(p_dm, 0x1bf0, 0x0000ffff, tmp1 | tmp2 | tmp3);

	for (i = 0; i < 2; i++)
		odm_write_4byte(p_dm, 0x1be8 + (i * 4), (p_iqk_info->RXIQK_AGC[channel][(i * 2) + 1] << 16) | p_iqk_info->RXIQK_AGC[channel][i * 2]);
}

void
_iqk_fail_count_8822b(
	void		*p_dm_void
)
{
	struct PHY_DM_STRUCT	*p_dm = (struct PHY_DM_STRUCT *)p_dm_void;
	struct _IQK_INFORMATION	*p_iqk_info = &p_dm->IQK_info;
	u8		i;

	p_dm->n_iqk_cnt++;
	if (odm_get_rf_reg(p_dm, RF_PATH_A, 0x1bf0, BIT(16)) == 1)
		p_iqk_info->is_reload = true;
	else
		p_iqk_info->is_reload = false;

	if (!p_iqk_info->is_reload) {
		for (i = 0; i < 8; i++) {
			if (odm_get_bb_reg(p_dm, 0x1bf0, BIT(i)) == 1)
				p_dm->n_iqk_fail_cnt++;
		}
	}
	ODM_RT_TRACE(p_dm, ODM_COMP_CALIBRATION, ODM_DBG_LOUD,
		("[IQK]All/Fail = %d %d\n", p_dm->n_iqk_cnt, p_dm->n_iqk_fail_cnt));
}

void
_iqk_iqk_fail_report_8822b(
	struct PHY_DM_STRUCT	*p_dm
)
{
	u32		tmp1bf0 = 0x0;
	u8		i;

	tmp1bf0 = odm_read_4byte(p_dm, 0x1bf0);

	for (i = 0; i < 4; i++) {
		if (tmp1bf0 & (0x1 << i))
#if !(DM_ODM_SUPPORT_TYPE & ODM_AP)
			ODM_RT_TRACE(p_dm, ODM_COMP_CALIBRATION, ODM_DBG_LOUD, ("[IQK] please check S%d TXIQK\n", i));
#else
			panic_printk("[IQK] please check S%d TXIQK\n", i);
#endif
		if (tmp1bf0 & (0x1 << (i + 12)))
#if !(DM_ODM_SUPPORT_TYPE & ODM_AP)
			ODM_RT_TRACE(p_dm, ODM_COMP_CALIBRATION, ODM_DBG_LOUD, ("[IQK] please check S%d RXIQK\n", i));
#else
			panic_printk("[IQK] please check S%d RXIQK\n", i);
#endif

	}
}


void
_iqk_backup_mac_bb_8822b(
	struct PHY_DM_STRUCT	*p_dm,
	u32		*MAC_backup,
	u32		*BB_backup,
	u32		*backup_mac_reg,
	u32		*backup_bb_reg
)
{
	u32 i;
	for (i = 0; i < MAC_REG_NUM_8822B; i++)
		MAC_backup[i] = odm_read_4byte(p_dm, backup_mac_reg[i]);

	for (i = 0; i < BB_REG_NUM_8822B; i++)
		BB_backup[i] = odm_read_4byte(p_dm, backup_bb_reg[i]);

	/*	ODM_RT_TRACE(p_dm, ODM_COMP_CALIBRATION, ODM_DBG_LOUD, ("[IQK]BackupMacBB Success!!!!\n")); */
}


void
_iqk_backup_rf_8822b(
	struct PHY_DM_STRUCT	*p_dm,
	u32		RF_backup[][2],
	u32		*backup_rf_reg
)
{
	u32 i;

	for (i = 0; i < RF_REG_NUM_8822B; i++) {
		RF_backup[i][RF_PATH_A] = odm_get_rf_reg(p_dm, RF_PATH_A, backup_rf_reg[i], RFREGOFFSETMASK);
		RF_backup[i][RF_PATH_B] = odm_get_rf_reg(p_dm, RF_PATH_B, backup_rf_reg[i], RFREGOFFSETMASK);
	}
	/*	ODM_RT_TRACE(p_dm, ODM_COMP_CALIBRATION, ODM_DBG_LOUD, ("[IQK]BackupRF Success!!!!\n")); */
}


void
_iqk_agc_bnd_int_8822b(
	struct PHY_DM_STRUCT	*p_dm
)
{
	/*initialize RX AGC bnd, it must do after bbreset*/
	odm_write_4byte(p_dm, 0x1b00, 0xf8000008);
	odm_write_4byte(p_dm, 0x1b00, 0xf80a7008);
	odm_write_4byte(p_dm, 0x1b00, 0xf8015008);
	odm_write_4byte(p_dm, 0x1b00, 0xf8000008);
	/*ODM_RT_TRACE(p_dm, ODM_COMP_CALIBRATION, ODM_DBG_TRACE, ("[IQK]init. rx agc bnd\n"));*/
}


void
_iqk_bb_reset_8822b(
	struct PHY_DM_STRUCT	*p_dm
)
{
	boolean		cca_ing = false;
	u32		count = 0;

	odm_set_rf_reg(p_dm, RF_PATH_A, 0x0, RFREGOFFSETMASK, 0x10000);
	odm_set_rf_reg(p_dm, RF_PATH_B, 0x0, RFREGOFFSETMASK, 0x10000);
	/*reset BB report*/
	odm_set_bb_reg(p_dm, 0x8f8, 0x0ff00000, 0x0);

	while (1) {
		odm_write_4byte(p_dm, 0x8fc, 0x0);
		odm_set_bb_reg(p_dm, 0x198c, 0x7, 0x7);
		cca_ing = (boolean) odm_get_bb_reg(p_dm, 0xfa0, BIT(3));

		if (count > 30)
			cca_ing = false;

		if (cca_ing) {
			ODM_delay_ms(1);
			count++;
		} else {
			odm_write_1byte(p_dm, 0x808, 0x0);	/*RX ant off*/
			odm_set_bb_reg(p_dm, 0xa04, BIT(27) | BIT(26) | BIT(25) | BIT(24), 0x0);		/*CCK RX path off*/

			/*BBreset*/
			odm_set_bb_reg(p_dm, 0x0, BIT(16), 0x0);
			odm_set_bb_reg(p_dm, 0x0, BIT(16), 0x1);

			if (odm_get_bb_reg(p_dm, 0x660, BIT(16)))
				odm_write_4byte(p_dm, 0x6b4, 0x89000006);
			/*ODM_RT_TRACE(p_dm, ODM_COMP_CALIBRATION, ODM_DBG_LOUD, ("[IQK]BBreset!!!!\n"));*/
			break;
		}
	}
}

void
_iqk_afe_setting_8822b(
	struct PHY_DM_STRUCT	*p_dm,
	boolean		do_iqk
)
{
	if (do_iqk) {
		odm_write_4byte(p_dm, 0xc60, 0x50000000);
		odm_write_4byte(p_dm, 0xc60, 0x70070040);
		odm_write_4byte(p_dm, 0xe60, 0x50000000);
		odm_write_4byte(p_dm, 0xe60, 0x70070040);
		odm_write_4byte(p_dm, 0xc58, 0xd8000402);
		odm_write_4byte(p_dm, 0xc5c, 0xd1000120);
		odm_write_4byte(p_dm, 0xc6c, 0x00000a15);
		odm_write_4byte(p_dm, 0xe58, 0xd8000402);
		odm_write_4byte(p_dm, 0xe5c, 0xd1000120);
		odm_write_4byte(p_dm, 0xe6c, 0x00000a15);
		_iqk_bb_reset_8822b(p_dm);
		/*		ODM_RT_TRACE(p_dm, ODM_COMP_CALIBRATION, ODM_DBG_LOUD, ("[IQK]AFE setting for IQK mode!!!!\n")); */
	} else {
		odm_write_4byte(p_dm, 0xc60, 0x50000000);
		odm_write_4byte(p_dm, 0xc60, 0x70038040);
		odm_write_4byte(p_dm, 0xe60, 0x50000000);
		odm_write_4byte(p_dm, 0xe60, 0x70038040);
		odm_write_4byte(p_dm, 0xc58, 0xd8020402);
		odm_write_4byte(p_dm, 0xc5c, 0xde000120);
		odm_write_4byte(p_dm, 0xc6c, 0x0000122a);
		odm_write_4byte(p_dm, 0xe58, 0xd8020402);
		odm_write_4byte(p_dm, 0xe5c, 0xde000120);
		odm_write_4byte(p_dm, 0xe6c, 0x0000122a);
		/*		ODM_RT_TRACE(p_dm, ODM_COMP_CALIBRATION, ODM_DBG_LOUD, ("[IQK]AFE setting for Normal mode!!!!\n")); */
	}
	/*0x9a4[31]=0: Select da clock*/
	odm_set_bb_reg(p_dm, 0x9a4, BIT(31), 0x0);
}

void
_iqk_restore_mac_bb_8822b(
	struct PHY_DM_STRUCT		*p_dm,
	u32		*MAC_backup,
	u32		*BB_backup,
	u32		*backup_mac_reg,
	u32		*backup_bb_reg
)
{
	u32 i;

	for (i = 0; i < MAC_REG_NUM_8822B; i++)
		odm_write_4byte(p_dm, backup_mac_reg[i], MAC_backup[i]);
	for (i = 0; i < BB_REG_NUM_8822B; i++)
		odm_write_4byte(p_dm, backup_bb_reg[i], BB_backup[i]);
	/*	ODM_RT_TRACE(p_dm, ODM_COMP_CALIBRATION, ODM_DBG_LOUD, ("[IQK]RestoreMacBB Success!!!!\n")); */
}

void
_iqk_restore_rf_8822b(
	struct PHY_DM_STRUCT			*p_dm,
	u32			*backup_rf_reg,
	u32			RF_backup[][2]
)
{
	u32 i;

	odm_set_rf_reg(p_dm, RF_PATH_A, 0xef, RFREGOFFSETMASK, 0x0);
	odm_set_rf_reg(p_dm, RF_PATH_B, 0xef, RFREGOFFSETMASK, 0x0);
	/*0xdf[4]=0*/
	_iqk_rf_set_check(p_dm, RF_PATH_A, 0xdf, RF_backup[0][RF_PATH_A] & (~BIT(4)));
	_iqk_rf_set_check(p_dm, RF_PATH_B, 0xdf, RF_backup[0][RF_PATH_B] & (~BIT(4)));
	
	/*odm_set_rf_reg(p_dm, RF_PATH_A, 0xdf, RFREGOFFSETMASK, RF_backup[0][RF_PATH_A] & (~BIT(4)));*/
	/*odm_set_rf_reg(p_dm, RF_PATH_B, 0xdf, RFREGOFFSETMASK, RF_backup[0][RF_PATH_B] & (~BIT(4)));*/

	for (i = 1; i < RF_REG_NUM_8822B; i++) {
		odm_set_rf_reg(p_dm, RF_PATH_A, backup_rf_reg[i], RFREGOFFSETMASK, RF_backup[i][RF_PATH_A]);
		odm_set_rf_reg(p_dm, RF_PATH_B, backup_rf_reg[i], RFREGOFFSETMASK, RF_backup[i][RF_PATH_B]);
	}
	/*	ODM_RT_TRACE(p_dm, ODM_COMP_CALIBRATION, ODM_DBG_LOUD, ("[IQK]RestoreRF Success!!!!\n")); */

}


void
_iqk_backup_iqk_8822b(
	struct PHY_DM_STRUCT			*p_dm,
	u8				step,
	u8				path
)
{
	struct _IQK_INFORMATION	*p_iqk_info = &p_dm->IQK_info;
	u8		i, j, k;

	switch (step) {
	case 0:
		p_iqk_info->iqk_channel[1] = p_iqk_info->iqk_channel[0];
		for (i = 0; i < 2; i++) {
			p_iqk_info->LOK_IDAC[1][i] = p_iqk_info->LOK_IDAC[0][i];
			p_iqk_info->RXIQK_AGC[1][i] = p_iqk_info->RXIQK_AGC[0][i];
			p_iqk_info->bypass_iqk[1][i] = p_iqk_info->bypass_iqk[0][i];
			p_iqk_info->RXIQK_fail_code[1][i] = p_iqk_info->RXIQK_fail_code[0][i];
			for (j = 0; j < 2; j++) {
				p_iqk_info->IQK_fail_report[1][i][j] = p_iqk_info->IQK_fail_report[0][i][j];
				for (k = 0; k < 8; k++) {
					p_iqk_info->IQK_CFIR_real[1][i][j][k] = p_iqk_info->IQK_CFIR_real[0][i][j][k];
					p_iqk_info->IQK_CFIR_imag[1][i][j][k] = p_iqk_info->IQK_CFIR_imag[0][i][j][k];
				}
			}
		}

		for (i = 0; i < 4; i++) {
			p_iqk_info->RXIQK_fail_code[0][i] = 0x0;
			p_iqk_info->RXIQK_AGC[0][i] = 0x0;
			for (j = 0; j < 2; j++) {
				p_iqk_info->IQK_fail_report[0][i][j] = true;
				p_iqk_info->gs_retry_count[0][i][j] = 0x0;
			}
			for (j = 0; j < 3; j++)
				p_iqk_info->retry_count[0][i][j] = 0x0;
		}
		/*backup channel*/
		p_iqk_info->iqk_channel[0] = p_iqk_info->rf_reg18;
		break;
	case 1: /*LOK backup*/
			p_iqk_info->LOK_IDAC[0][path] = odm_get_rf_reg(p_dm, (enum rf_path)path, 0x58, RFREGOFFSETMASK);
		break;
	case 2:	/*TXIQK backup*/
	case 3: /*RXIQK backup*/	
		phydm_get_iqk_cfir(p_dm, (step-2), path, false);
		break;
	}
}

void
_iqk_reload_iqk_setting_8822b(
	struct PHY_DM_STRUCT			*p_dm,
	u8				channel,
	u8				reload_idx  /*1: reload TX, 2: reload LO, TX, RX*/
)
{
	struct _IQK_INFORMATION	*p_iqk_info = &p_dm->IQK_info;
	u8 i, path, idx;
	u16 iqk_apply[2] = {0xc94, 0xe94};
	u32 tmp;

	for (path = 0; path < 2; path++) {
		if (reload_idx == 2) {
			/*odm_set_rf_reg(p_dm, (enum rf_path)path, 0xdf, BIT(4), 0x1);*/
			tmp = odm_get_rf_reg(p_dm, (enum rf_path)path, 0xdf, RFREGOFFSETMASK) | BIT(4);
			_iqk_rf_set_check(p_dm, (enum rf_path)path, 0xdf, tmp);
			odm_set_rf_reg(p_dm, (enum rf_path)path, 0x58, RFREGOFFSETMASK, p_iqk_info->LOK_IDAC[channel][path]);
		}

		for (idx = 0; idx < reload_idx; idx++) {
			odm_set_bb_reg(p_dm, 0x1b00, MASKDWORD, 0xf8000008 | path << 1);
			odm_set_bb_reg(p_dm, 0x1b2c, MASKDWORD, 0x7);
			odm_set_bb_reg(p_dm, 0x1b38, MASKDWORD, 0x20000000);
			odm_set_bb_reg(p_dm, 0x1b3c, MASKDWORD, 0x20000000);
			odm_set_bb_reg(p_dm, 0x1bcc, MASKDWORD, 0x00000000);
			if (idx == 0)
				odm_set_bb_reg(p_dm, 0x1b0c, BIT(13) | BIT(12), 0x3);
			else
				odm_set_bb_reg(p_dm, 0x1b0c, BIT(13) | BIT(12), 0x1);
			odm_set_bb_reg(p_dm, 0x1bd4, BIT(20) | BIT(19) | BIT(18) | BIT(17) | BIT(16), 0x10);
			for (i = 0; i < 8; i++) {
				odm_write_4byte(p_dm, 0x1bd8,	((0xc0000000 >> idx) + 0x3) + (i * 4) + (p_iqk_info->IQK_CFIR_real[channel][path][idx][i] << 9));
				odm_write_4byte(p_dm, 0x1bd8, ((0xc0000000 >> idx) + 0x1) + (i * 4) + (p_iqk_info->IQK_CFIR_imag[channel][path][idx][i] << 9));
			}
			if (idx == 0)
				odm_set_bb_reg(p_dm, iqk_apply[path], BIT(0), !(p_iqk_info->IQK_fail_report[channel][path][idx]));
			else
				odm_set_bb_reg(p_dm, iqk_apply[path], BIT(10), !(p_iqk_info->IQK_fail_report[channel][path][idx]));
		}
		odm_set_bb_reg(p_dm, 0x1bd8, MASKDWORD, 0x0);
		odm_set_bb_reg(p_dm, 0x1b0c, BIT(13) | BIT(12), 0x0);
	}
}

boolean
_iqk_reload_iqk_8822b(
	struct PHY_DM_STRUCT			*p_dm,
	boolean			reset
)
{
	struct _IQK_INFORMATION	*p_iqk_info = &p_dm->IQK_info;
	u8 i;
	p_iqk_info->is_reload = false;

	if (reset) {
		for (i = 0; i < 2; i++)
			p_iqk_info->iqk_channel[i] = 0x0;
	} else {
		p_iqk_info->rf_reg18 = odm_get_rf_reg(p_dm, RF_PATH_A, 0x18, RFREGOFFSETMASK);

		for (i = 0; i < 2; i++) {
			if (p_iqk_info->rf_reg18 == p_iqk_info->iqk_channel[i]) {
				_iqk_reload_iqk_setting_8822b(p_dm, i, 2);
				_iqk_fill_iqk_report_8822b(p_dm, i);
				ODM_RT_TRACE(p_dm, ODM_COMP_CALIBRATION, ODM_DBG_LOUD, ("[IQK]reload IQK result before!!!!\n"));
				 p_iqk_info->is_reload = true;
			}
		}
	}
	/*report*/
	odm_set_bb_reg(p_dm, 0x1bf0, BIT(16), (u8) p_iqk_info->is_reload);
	return  p_iqk_info->is_reload;
}


void
_iqk_rfe_setting_8822b(
	struct PHY_DM_STRUCT	*p_dm,
	boolean		ext_pa_on
)
{
	if (ext_pa_on) {
		/*RFE setting*/
		odm_write_4byte(p_dm, 0xcb0, 0x77777777);
		odm_write_4byte(p_dm, 0xcb4, 0x00007777);
		odm_write_4byte(p_dm, 0xcbc, 0x0000083B);
		odm_write_4byte(p_dm, 0xeb0, 0x77777777);
		odm_write_4byte(p_dm, 0xeb4, 0x00007777);
		odm_write_4byte(p_dm, 0xebc, 0x0000083B);
		/*odm_write_4byte(p_dm, 0x1990, 0x00000c30);*/
		ODM_RT_TRACE(p_dm, ODM_COMP_CALIBRATION, ODM_DBG_LOUD, ("[IQK]external PA on!!!!\n"));
	} else {
		/*RFE setting*/
		odm_write_4byte(p_dm, 0xcb0, 0x77777777);
		odm_write_4byte(p_dm, 0xcb4, 0x00007777);
		odm_write_4byte(p_dm, 0xcbc, 0x00000100);
		odm_write_4byte(p_dm, 0xeb0, 0x77777777);
		odm_write_4byte(p_dm, 0xeb4, 0x00007777);
		odm_write_4byte(p_dm, 0xebc, 0x00000100);
		/*odm_write_4byte(p_dm, 0x1990, 0x00000c30);*/
		/*		ODM_RT_TRACE(p_dm, ODM_COMP_CALIBRATION, ODM_DBG_LOUD, ("[IQK]external PA off!!!!\n"));*/
	}
}


void
_iqk_rf_setting_8822b(
	struct PHY_DM_STRUCT	*p_dm
)
{
	u8 path;
	u32 tmp;

	odm_write_4byte(p_dm, 0x1b00, 0xf8000008);
	odm_write_4byte(p_dm, 0x1bb8, 0x00000000);

	for (path = 0; path < 2; path++) {
		/*0xdf:B11 = 1,B4 = 0, B1 = 1*/
		tmp = odm_get_rf_reg(p_dm, (enum rf_path)path, 0xdf, RFREGOFFSETMASK);
		tmp = (tmp & (~BIT(4))) | BIT(1) | BIT(11);
		_iqk_rf_set_check(p_dm, (enum rf_path)path, 0xdf, tmp);
		/*odm_set_rf_reg(p_dm, (enum rf_path)path, 0xdf, RFREGOFFSETMASK, tmp);*/

		/*release 0x56 TXBB*/
		odm_set_rf_reg(p_dm, (enum rf_path)path, 0x65, RFREGOFFSETMASK, 0x09000);

		if (*p_dm->p_band_type == ODM_BAND_5G) {
			odm_set_rf_reg(p_dm, (enum rf_path)path, 0xef, BIT(19), 0x1);
			odm_set_rf_reg(p_dm, (enum rf_path)path, 0x33, RFREGOFFSETMASK, 0x00026);
			odm_set_rf_reg(p_dm, (enum rf_path)path, 0x3e, RFREGOFFSETMASK, 0x00037);
			odm_set_rf_reg(p_dm, (enum rf_path)path, 0x3f, RFREGOFFSETMASK, 0xdefce);
			odm_set_rf_reg(p_dm, (enum rf_path)path, 0xef, BIT(19), 0x0);
		} else {
			odm_set_rf_reg(p_dm, (enum rf_path)path, 0xef, BIT(19), 0x1);
			odm_set_rf_reg(p_dm, (enum rf_path)path, 0x33, RFREGOFFSETMASK, 0x00026);
			odm_set_rf_reg(p_dm, (enum rf_path)path, 0x3e, RFREGOFFSETMASK, 0x00037);
			odm_set_rf_reg(p_dm, (enum rf_path)path, 0x3f, RFREGOFFSETMASK, 0x5efce);
			odm_set_rf_reg(p_dm, (enum rf_path)path, 0xef, BIT(19), 0x0);
		}
	}
}



void
_iqk_configure_macbb_8822b(
	struct PHY_DM_STRUCT		*p_dm
)
{
	/*MACBB register setting*/
	odm_write_1byte(p_dm, 0x522, 0x7f);
	odm_set_bb_reg(p_dm, 0x550, BIT(11) | BIT(3), 0x0);
	odm_set_bb_reg(p_dm, 0x90c, BIT(15), 0x1);			/*0x90c[15]=1: dac_buf reset selection*/
	/*0xc94[0]=1, 0xe94[0]=1: 讓tx從iqk打出來*/
	odm_set_bb_reg(p_dm, 0xc94, BIT(0), 0x1);
	odm_set_bb_reg(p_dm, 0xe94, BIT(0), 0x1);
	odm_set_bb_reg(p_dm, 0xc94, (BIT(11) | BIT(10)), 0x1);
	odm_set_bb_reg(p_dm, 0xe94, (BIT(11) | BIT(10)), 0x1);
	/* 3-wire off*/
	odm_write_4byte(p_dm, 0xc00, 0x00000004);
	odm_write_4byte(p_dm, 0xe00, 0x00000004);
	/*disable PMAC*/
	odm_set_bb_reg(p_dm, 0xb00, BIT(8), 0x0);
	/*	ODM_RT_TRACE(p_dm, ODM_COMP_CALIBRATION, ODM_DBG_LOUD, ("[IQK]Set MACBB setting for IQK!!!!\n"));*/
	/*disable CCK block*/
	odm_set_bb_reg(p_dm, 0x808, BIT(28), 0x0);
	/*disable OFDM CCA*/
	odm_set_bb_reg(p_dm, 0x838, BIT(3) | BIT(2) | BIT(1), 0x7);
}

void
_iqk_lok_setting_8822b(
	struct PHY_DM_STRUCT	*p_dm,

	u8 path
)
{
	odm_write_4byte(p_dm, 0x1b00, 0xf8000008 | path << 1);
	odm_write_4byte(p_dm, 0x1bcc, 0x9);
	odm_write_1byte(p_dm, 0x1b23, 0x00);

	switch (*p_dm->p_band_type) {
	case ODM_BAND_2_4G:
		odm_write_1byte(p_dm, 0x1b2b, 0x00);
		odm_set_rf_reg(p_dm, (enum rf_path)path, 0x56, RFREGOFFSETMASK, 0x50df2);
		odm_set_rf_reg(p_dm, (enum rf_path)path, 0x8f, RFREGOFFSETMASK, 0xadc00);
		/* WE_LUT_TX_LOK*/
		odm_set_rf_reg(p_dm, (enum rf_path)path, 0xef, BIT(4), 0x1);
		odm_set_rf_reg(p_dm, (enum rf_path)path, 0x33, BIT(1) | BIT(0), 0x0);
		break;
	case ODM_BAND_5G:
		odm_write_1byte(p_dm, 0x1b2b, 0x80);
		odm_set_rf_reg(p_dm, (enum rf_path)path, 0x56, RFREGOFFSETMASK, 0x5086c);
		odm_set_rf_reg(p_dm, (enum rf_path)path, 0x8f, RFREGOFFSETMASK, 0xa9c00);
		/* WE_LUT_TX_LOK*/
		odm_set_rf_reg(p_dm, (enum rf_path)path, 0xef, BIT(4), 0x1);
		odm_set_rf_reg(p_dm, (enum rf_path)path, 0x33, BIT(1) | BIT(0), 0x1);
		break;
	}
	/*	ODM_RT_TRACE(p_dm, ODM_COMP_CALIBRATION, ODM_DBG_LOUD, ("[IQK]Set LOK setting!!!!\n"));*/
}


void
_iqk_txk_setting_8822b(
	struct PHY_DM_STRUCT	*p_dm,
	u8 path
)
{
	odm_write_4byte(p_dm, 0x1b00, 0xf8000008 | path << 1);
	odm_write_4byte(p_dm, 0x1bcc, 0x9);
	odm_write_4byte(p_dm, 0x1b20, 0x01440008);

	if (path == 0x0)
		odm_write_4byte(p_dm, 0x1b00, 0xf800000a);
	else
		odm_write_4byte(p_dm, 0x1b00, 0xf8000008);
	odm_write_4byte(p_dm, 0x1bcc, 0x3f);

	switch (*p_dm->p_band_type) {
	case ODM_BAND_2_4G:
		odm_set_rf_reg(p_dm, (enum rf_path)path, 0x56, RFREGOFFSETMASK, 0x50df2);
		odm_set_rf_reg(p_dm, (enum rf_path)path, 0x8f, RFREGOFFSETMASK, 0xadc00);
		odm_write_1byte(p_dm, 0x1b2b, 0x00);
		break;
	case ODM_BAND_5G:
		odm_set_rf_reg(p_dm, (enum rf_path)path, 0x56, RFREGOFFSETMASK, 0x500ef);
		odm_set_rf_reg(p_dm, (enum rf_path)path, 0x8f, RFREGOFFSETMASK, 0xa9c00);
		odm_write_1byte(p_dm, 0x1b2b, 0x80);
		break;
	}
	/*	ODM_RT_TRACE(p_dm, ODM_COMP_CALIBRATION, ODM_DBG_LOUD, ("[IQK]Set TXK setting!!!!\n"));*/

}


void
_iqk_rxk1_setting_8822b(
	struct PHY_DM_STRUCT	*p_dm,
	u8 path
)
{
	odm_write_4byte(p_dm, 0x1b00, 0xf8000008 | path << 1);

	switch (*p_dm->p_band_type) {
	case ODM_BAND_2_4G:
		odm_write_1byte(p_dm, 0x1bcc, 0x9);
		odm_write_1byte(p_dm, 0x1b2b, 0x00);
		odm_write_4byte(p_dm, 0x1b20, 0x01450008);
		odm_write_4byte(p_dm, 0x1b24, 0x01460c88);
		odm_set_rf_reg(p_dm, (enum rf_path)path, 0x56, RFREGOFFSETMASK, 0x510e0);
		odm_set_rf_reg(p_dm, (enum rf_path)path, 0x8f, RFREGOFFSETMASK, 0xacc00);
		break;
	case ODM_BAND_5G:
		odm_write_1byte(p_dm, 0x1bcc, 0x09);
		odm_write_1byte(p_dm, 0x1b2b, 0x80);
		odm_write_4byte(p_dm, 0x1b20, 0x00850008);
		odm_write_4byte(p_dm, 0x1b24, 0x00460048);
		odm_set_rf_reg(p_dm, (enum rf_path)path, 0x56, RFREGOFFSETMASK, 0x510e0);
		odm_set_rf_reg(p_dm, (enum rf_path)path, 0x8f, RFREGOFFSETMASK, 0xadc00);
		break;
	}
	/*ODM_RT_TRACE(p_dm, ODM_COMP_CALIBRATION, ODM_DBG_LOUD, ("[IQK]Set RXK setting!!!!\n"));*/

}


void
_iqk_rxk2_setting_8822b(
	struct PHY_DM_STRUCT	*p_dm,
	u8 path,
	boolean is_gs
)
{
	struct _IQK_INFORMATION	*p_iqk_info = &p_dm->IQK_info;

	odm_write_4byte(p_dm, 0x1b00, 0xf8000008 | path << 1);

	switch (*p_dm->p_band_type) {
	case ODM_BAND_2_4G:
		if (is_gs)
			p_iqk_info->tmp1bcc = 0x12;
		odm_write_1byte(p_dm, 0x1bcc, p_iqk_info->tmp1bcc);
		odm_write_1byte(p_dm, 0x1b2b, 0x00);
		odm_write_4byte(p_dm, 0x1b20, 0x01450008);
		odm_write_4byte(p_dm, 0x1b24, 0x01460848);
		odm_set_rf_reg(p_dm, (enum rf_path)path, 0x56, RFREGOFFSETMASK, 0x510e0);
		odm_set_rf_reg(p_dm, (enum rf_path)path, 0x8f, RFREGOFFSETMASK, 0xa9c00);
		break;
	case ODM_BAND_5G:
		if (is_gs) {
			if (path == RF_PATH_A)
				p_iqk_info->tmp1bcc = 0x12;
			else
				p_iqk_info->tmp1bcc = 0x09;
		}
			odm_write_1byte(p_dm, 0x1bcc, p_iqk_info->tmp1bcc);
		odm_write_1byte(p_dm, 0x1b2b, 0x80);
		odm_write_4byte(p_dm, 0x1b20, 0x00850008);
		odm_write_4byte(p_dm, 0x1b24, 0x00460848);
		odm_set_rf_reg(p_dm, (enum rf_path)path, 0x56, RFREGOFFSETMASK, 0x51060);
		odm_set_rf_reg(p_dm, (enum rf_path)path, 0x8f, RFREGOFFSETMASK, 0xa9c00);
		break;
	}
	/*	ODM_RT_TRACE(p_dm, ODM_COMP_CALIBRATION, ODM_DBG_LOUD, ("[IQK]Set RXK setting!!!!\n"));*/

}


void
halrf_iqk_set_rf0x8(
	struct PHY_DM_STRUCT	*p_dm,
	u8	path
)
{
	u16 c = 0x0;

	while (c < 30000) {
		odm_set_rf_reg(p_dm, (enum rf_path)path, 0xef, RFREGOFFSETMASK, 0x0);
		odm_set_rf_reg(p_dm, (enum rf_path)path, 0x8, RFREGOFFSETMASK, 0x0);
		if (odm_get_rf_reg(p_dm, (enum rf_path)path, 0x8, RFREGOFFSETMASK) == 0x0)
			break;
		c++;
	}
}

void
halrf_iqk_check_if_reload(
	struct PHY_DM_STRUCT	*p_dm
)
{
	struct _IQK_INFORMATION	*p_iqk_info = &p_dm->IQK_info;

	p_iqk_info->is_reload = (boolean)odm_get_bb_reg(p_dm, 0x1bf0, BIT(16));
}


boolean
_iqk_check_cal_8822b(
	struct	PHY_DM_STRUCT	*p_dm,
	u8	path,
	u8	cmd
)
{
	boolean	notready = true, fail = true;
	u32	delay_count = 0x0;

	while (notready) {
		if (odm_get_rf_reg(p_dm, (enum rf_path)path, 0x8, RFREGOFFSETMASK) == 0x12345) {
			if (cmd == 0x0)/*LOK*/
				fail = false;
			else
				fail = (boolean) odm_get_bb_reg(p_dm, 0x1b08, BIT(26));
			notready = false;
		} else {
			ODM_delay_ms(1);
			delay_count++;
		}

		if (delay_count >= 50) {
			fail = true;
			ODM_RT_TRACE(p_dm, ODM_COMP_CALIBRATION, ODM_DBG_LOUD,
				     ("[IQK]IQK timeout!!!\n"));
			break;
		}
	}
	halrf_iqk_set_rf0x8(p_dm, path);
	ODM_RT_TRACE(p_dm, ODM_COMP_CALIBRATION, ODM_DBG_LOUD,
		     ("[IQK]delay count = 0x%x!!!\n", delay_count));
	return fail;
}


boolean
_iqk_rx_iqk_gain_search_fail_8822b(
	struct PHY_DM_STRUCT			*p_dm,
	u8		path,
	u8		step
)
{

	struct _IQK_INFORMATION	*p_iqk_info = &p_dm->IQK_info;
	boolean	fail = true;
	u32	IQK_CMD = 0x0, rf_reg0, tmp, bb_idx;
	u8	IQMUX[4] = {0x9, 0x12, 0x1b, 0x24};
	u8	idx;

	for (idx = 0; idx < 4; idx++)
		if (p_iqk_info->tmp1bcc == IQMUX[idx])
			break;

	odm_write_4byte(p_dm, 0x1b00, 0xf8000008 | path << 1);
	odm_write_4byte(p_dm, 0x1bcc, p_iqk_info->tmp1bcc);

	if (step == RXIQK1)
		ODM_RT_TRACE(p_dm, ODM_COMP_CALIBRATION, ODM_DBG_LOUD, ("[IQK]============ S%d RXIQK GainSearch ============\n", path));

	if (step == RXIQK1)
		IQK_CMD = 0xf8000208 | (1 << (path + 4));
	else
		IQK_CMD = 0xf8000308 | (1 << (path + 4));

	ODM_RT_TRACE(p_dm, ODM_COMP_CALIBRATION, ODM_DBG_TRACE, ("[IQK]S%d GS%d_Trigger = 0x%x\n", path, step, IQK_CMD));

	odm_write_4byte(p_dm, 0x1b00, IQK_CMD);
	odm_write_4byte(p_dm, 0x1b00, IQK_CMD + 0x1);
	ODM_delay_ms(GS_delay_8822B);
	fail = _iqk_check_cal_8822b(p_dm, path, 0x1);

	if (step == RXIQK2) {
		rf_reg0 = odm_get_rf_reg(p_dm, (enum rf_path)path, 0x0, RFREGOFFSETMASK);
	odm_write_4byte(p_dm, 0x1b00, 0xf8000008 | path << 1);
		ODM_RT_TRACE(p_dm, ODM_COMP_CALIBRATION, ODM_DBG_TRACE,
			("[IQK]S%d ==> RF0x0 = 0x%x, tmp1bcc = 0x%x, idx = %d, 0x1b3c = 0x%x\n", path, rf_reg0, p_iqk_info->tmp1bcc, idx, odm_read_4byte(p_dm, 0x1b3c)));
		tmp = (rf_reg0 & 0x1fe0) >> 5;
		p_iqk_info->lna_idx = tmp >> 5;
		bb_idx = tmp & 0x1f;
#if 1
		if (bb_idx == 0x1) {
			if (p_iqk_info->lna_idx != 0x0)
				p_iqk_info->lna_idx--;
			else if (idx != 3)
				idx++;
			else
				p_iqk_info->isbnd = true;
			fail = true;
		} else if (bb_idx == 0xa) {
			if (idx != 0)
				idx--;
			else if (p_iqk_info->lna_idx != 0x7)
				p_iqk_info->lna_idx++;
			else
				p_iqk_info->isbnd = true;
			fail = true;
		} else
			fail = false;

		if (p_iqk_info->isbnd == true)
			fail = false;

		p_iqk_info->tmp1bcc = IQMUX[idx];
#endif

#if 0
		if (bb_idx == 0x1) {
			if (p_iqk_info->lna_idx != 0x0)
				p_iqk_info->lna_idx--;
			fail = true;
		} else if (bb_idx == 0xa) {
			if (p_iqk_info->lna_idx != 0x7)
				p_iqk_info->lna_idx++;
			fail = true;
		} else
			fail = false;
#endif
		if (fail) {
			odm_write_4byte(p_dm, 0x1b00, 0xf8000008 | path << 1);
			odm_write_4byte(p_dm, 0x1b24, (odm_read_4byte(p_dm, 0x1b24) & 0xffffe3ff) | (p_iqk_info->lna_idx << 10));
		}
	}

	return fail;
}

boolean
_lok_one_shot_8822b(
	void		*p_dm_void,
	u8			path
)
{
	struct	PHY_DM_STRUCT	*p_dm = (struct PHY_DM_STRUCT *)p_dm_void;
	struct	_IQK_INFORMATION	*p_iqk_info = &p_dm->IQK_info;
	u8	delay_count = 0;
	boolean	LOK_notready = false;
	u32	LOK_temp = 0;
	u32	IQK_CMD = 0x0;

	ODM_RT_TRACE(p_dm, ODM_COMP_CALIBRATION, ODM_DBG_TRACE, ("[IQK]==========S%d LOK ==========\n", path));
	IQK_CMD = 0xf8000008 | (1 << (4 + path));
	ODM_RT_TRACE(p_dm, ODM_COMP_CALIBRATION, ODM_DBG_TRACE, ("[IQK]LOK_Trigger = 0x%x\n", IQK_CMD));
	odm_write_4byte(p_dm, 0x1b00, IQK_CMD);
	odm_write_4byte(p_dm, 0x1b00, IQK_CMD + 1);
	/*LOK: CMD ID = 0	{0xf8000018, 0xf8000028}*/
	/*LOK: CMD ID = 0	{0xf8000019, 0xf8000029}*/
	ODM_delay_ms(LOK_delay_8822B);
	LOK_notready = _iqk_check_cal_8822b(p_dm, path, 0x0);
	if (!LOK_notready)
		_iqk_backup_iqk_8822b(p_dm, 0x1, path);
	if (ODM_COMP_CALIBRATION) {
		if (!LOK_notready) {
			LOK_temp = odm_get_rf_reg(p_dm, (enum rf_path)path, 0x58, RFREGOFFSETMASK);
			ODM_RT_TRACE(p_dm, ODM_COMP_CALIBRATION, ODM_DBG_TRACE, ("[IQK]0x58 = 0x%x\n", LOK_temp));
		} else
			ODM_RT_TRACE(p_dm, ODM_COMP_CALIBRATION, ODM_DBG_TRACE, ("[IQK]==>S%d LOK Fail!!!\n", path));
	}
	p_iqk_info->LOK_fail[path] = LOK_notready;
	return LOK_notready;
}




boolean
_iqk_one_shot_8822b(
	void		*p_dm_void,
	u8		path,
	u8		idx
)
{
	struct PHY_DM_STRUCT	*p_dm = (struct PHY_DM_STRUCT *)p_dm_void;
	struct _IQK_INFORMATION	*p_iqk_info = &p_dm->IQK_info;
	u8		delay_count = 0;
	boolean		notready = true, fail = true;
	u32		IQK_CMD = 0x0;
	u16		iqk_apply[2]	= {0xc94, 0xe94};

	if (idx == TXIQK)
		ODM_RT_TRACE(p_dm, ODM_COMP_CALIBRATION, ODM_DBG_LOUD, ("[IQK]============ S%d WBTXIQK ============\n", path));
	else if (idx == RXIQK1)
		ODM_RT_TRACE(p_dm, ODM_COMP_CALIBRATION, ODM_DBG_LOUD, ("[IQK]============ S%d WBRXIQK STEP1============\n", path));
	else
		ODM_RT_TRACE(p_dm, ODM_COMP_CALIBRATION, ODM_DBG_LOUD, ("[IQK]============ S%d WBRXIQK STEP2============\n", path));

	if (idx == TXIQK) {
		IQK_CMD = 0xf8000008 | ((*p_dm->p_band_width + 4) << 8) | (1 << (path + 4));
		ODM_RT_TRACE(p_dm, ODM_COMP_CALIBRATION, ODM_DBG_TRACE, ("[IQK]TXK_Trigger = 0x%x\n", IQK_CMD));
		/*{0xf8000418, 0xf800042a} ==> 20 WBTXK (CMD = 4)*/
		/*{0xf8000518, 0xf800052a} ==> 40 WBTXK (CMD = 5)*/
		/*{0xf8000618, 0xf800062a} ==> 80 WBTXK (CMD = 6)*/
	} else if (idx == RXIQK1) {
		if (*p_dm->p_band_width == 2)
			IQK_CMD = 0xf8000808 | (1 << (path + 4));
		else
			IQK_CMD = 0xf8000708 | (1 << (path + 4));
		ODM_RT_TRACE(p_dm, ODM_COMP_CALIBRATION, ODM_DBG_TRACE, ("[IQK]RXK1_Trigger = 0x%x\n", IQK_CMD));
		/*{0xf8000718, 0xf800072a} ==> 20 WBTXK (CMD = 7)*/
		/*{0xf8000718, 0xf800072a} ==> 40 WBTXK (CMD = 7)*/
		/*{0xf8000818, 0xf800082a} ==> 80 WBTXK (CMD = 8)*/
	} else if (idx == RXIQK2) {
		IQK_CMD = 0xf8000008 | ((*p_dm->p_band_width + 9) << 8) | (1 << (path + 4));
		ODM_RT_TRACE(p_dm, ODM_COMP_CALIBRATION, ODM_DBG_TRACE, ("[IQK]RXK2_Trigger = 0x%x\n", IQK_CMD));
		/*{0xf8000918, 0xf800092a} ==> 20 WBRXK (CMD = 9)*/
		/*{0xf8000a18, 0xf8000a2a} ==> 40 WBRXK (CMD = 10)*/
		/*{0xf8000b18, 0xf8000b2a} ==> 80 WBRXK (CMD = 11)*/
		odm_write_4byte(p_dm, 0x1b00, 0xf8000008 | path << 1);
		odm_write_4byte(p_dm, 0x1b24, (odm_read_4byte(p_dm, 0x1b24) & 0xffffe3ff) | ((p_iqk_info->lna_idx & 0x7) << 10));
	}
	odm_write_4byte(p_dm, 0x1b00, IQK_CMD);
	odm_write_4byte(p_dm, 0x1b00, IQK_CMD + 0x1);
	ODM_delay_ms(WBIQK_delay_8822B);
	fail = _iqk_check_cal_8822b(p_dm, path, 0x1);

	if (p_dm->debug_components & ODM_COMP_CALIBRATION) {
		odm_write_4byte(p_dm, 0x1b00, 0xf8000008 | path << 1);
		ODM_RT_TRACE(p_dm, ODM_COMP_CALIBRATION, ODM_DBG_TRACE,
			("[IQK]S%d ==> 0x1b00 = 0x%x, 0x1b08 = 0x%x\n", path, odm_read_4byte(p_dm, 0x1b00), odm_read_4byte(p_dm, 0x1b08)));
		ODM_RT_TRACE(p_dm, ODM_COMP_CALIBRATION, ODM_DBG_TRACE,
			("[IQK]S%d ==> delay_count = 0x%x\n", path, delay_count));
		if (idx != TXIQK)
			ODM_RT_TRACE(p_dm, ODM_COMP_CALIBRATION, ODM_DBG_TRACE,
				("[IQK]S%d ==> RF0x0 = 0x%x, RF0x56 = 0x%x\n", path, odm_get_rf_reg(p_dm, (enum rf_path)path, 0x0, RFREGOFFSETMASK),
				odm_get_rf_reg(p_dm, (enum rf_path)path, 0x56, RFREGOFFSETMASK)));
	}

	odm_write_4byte(p_dm, 0x1b00, 0xf8000008 | path << 1);

	if (idx == TXIQK) {
		if (fail)
			odm_set_bb_reg(p_dm, iqk_apply[path], BIT(0), 0x0);
		else	
			_iqk_backup_iqk_8822b(p_dm, 0x2, path);
	}

	if (idx == RXIQK2) {
		p_iqk_info->RXIQK_AGC[0][path] =
			(u16)(((odm_get_rf_reg(p_dm, (enum rf_path)path, 0x0, RFREGOFFSETMASK) >> 5) & 0xff) |
			      (p_iqk_info->tmp1bcc << 8));

		odm_write_4byte(p_dm, 0x1b38, 0x20000000);

		if (fail)
			odm_set_bb_reg(p_dm, iqk_apply[path], (BIT(11) | BIT(10)), 0x0);
		else
			_iqk_backup_iqk_8822b(p_dm, 0x3, path);
	}

	if (idx == TXIQK)
		p_iqk_info->IQK_fail_report[0][path][TXIQK] = fail;
	else
		p_iqk_info->IQK_fail_report[0][path][RXIQK] = fail;

	return fail;
}


boolean
_iqk_rx_iqk_by_path_8822b(
	void		*p_dm_void,
	u8		path
)
{
	struct PHY_DM_STRUCT	*p_dm = (struct PHY_DM_STRUCT *)p_dm_void;
	struct _IQK_INFORMATION	*p_iqk_info = &p_dm->IQK_info;
	boolean		KFAIL = true, gonext;

#if 1
	switch (p_iqk_info->rxiqk_step) {
	case 1:		/*gain search_RXK1*/
		_iqk_rxk1_setting_8822b(p_dm, path);
		gonext = false;
		while (1) {
			KFAIL = _iqk_rx_iqk_gain_search_fail_8822b(p_dm, path, RXIQK1);
			if (KFAIL && (p_iqk_info->gs_retry_count[0][path][0] < 2))
				p_iqk_info->gs_retry_count[0][path][0]++;
			else if (KFAIL) {
				p_iqk_info->RXIQK_fail_code[0][path] = 0;
				p_iqk_info->rxiqk_step = 5;
				gonext = true;
			} else {
				p_iqk_info->rxiqk_step++;
				gonext = true;
			}
			if (gonext)
				break;
		}
		halrf_iqk_xym_read(p_dm, path, 0x2);
		break;
	case 2:		/*gain search_RXK2*/
		_iqk_rxk2_setting_8822b(p_dm, path, true);
		p_iqk_info->isbnd = false;
		while (1) {
			KFAIL = _iqk_rx_iqk_gain_search_fail_8822b(p_dm, path, RXIQK2);
			if (KFAIL && (p_iqk_info->gs_retry_count[0][path][1] < rxiqk_gs_limit))
				p_iqk_info->gs_retry_count[0][path][1]++;
			else {
				p_iqk_info->rxiqk_step++;
				break;
			}
		}
		halrf_iqk_xym_read(p_dm, path, 0x3);
		break;
	case 3:		/*RXK1*/
		_iqk_rxk1_setting_8822b(p_dm, path);
		gonext = false;
		while (1) {
			KFAIL = _iqk_one_shot_8822b(p_dm, path, RXIQK1);
			if (KFAIL && (p_iqk_info->retry_count[0][path][RXIQK1] < 2))
				p_iqk_info->retry_count[0][path][RXIQK1]++;
			else if (KFAIL) {
				p_iqk_info->RXIQK_fail_code[0][path] = 1;
				p_iqk_info->rxiqk_step = 5;
				gonext = true;
			} else {
				p_iqk_info->rxiqk_step++;
				gonext = true;
			}
			if (gonext)
				break;
		}
		halrf_iqk_xym_read(p_dm, path, 0x4);
		break;
	case 4:		/*RXK2*/
		_iqk_rxk2_setting_8822b(p_dm, path, false);
		gonext = false;
		while (1) {
			KFAIL = _iqk_one_shot_8822b(p_dm, path,	RXIQK2);
			if (KFAIL && (p_iqk_info->retry_count[0][path][RXIQK2] < 2))
				p_iqk_info->retry_count[0][path][RXIQK2]++;
			else if (KFAIL) {
				p_iqk_info->RXIQK_fail_code[0][path] = 2;
				p_iqk_info->rxiqk_step = 5;
				gonext = true;
			} else {
				p_iqk_info->rxiqk_step++;
				gonext = true;
			}
			if (gonext)
				break;
		}
		halrf_iqk_xym_read(p_dm, path, 0x0);
		break;
	}
	return KFAIL;
#endif
}


void
_iqk_iqk_by_path_8822b(
	void		*p_dm_void,
	boolean		segment_iqk
)
{
	struct PHY_DM_STRUCT	*p_dm = (struct PHY_DM_STRUCT *)p_dm_void;
	struct _IQK_INFORMATION	*p_iqk_info = &p_dm->IQK_info;
	boolean		KFAIL = true;
	u8		i, kcount_limit;

	/*	ODM_RT_TRACE(p_dm, ODM_COMP_CALIBRATION, ODM_DBG_TRACE, ("[IQK]iqk_step = 0x%x\n", p_dm->rf_calibrate_info.iqk_step)); */

	if (*p_dm->p_band_width == 2)
		kcount_limit = kcount_limit_80m;
	else
		kcount_limit = kcount_limit_others;

	while (1) {
#if 1
	switch (p_dm->rf_calibrate_info.iqk_step) {
	case 1:		/*S0 LOK*/
#if 1
		_iqk_lok_setting_8822b(p_dm, RF_PATH_A);
		_lok_one_shot_8822b(p_dm, RF_PATH_A);
#endif
		p_dm->rf_calibrate_info.iqk_step++;
		break;
	case 2:		/*S1 LOK*/
#if 1
		_iqk_lok_setting_8822b(p_dm, RF_PATH_B);
		_lok_one_shot_8822b(p_dm, RF_PATH_B);
#endif
		p_dm->rf_calibrate_info.iqk_step++;
		break;
	case 3:		/*S0 TXIQK*/
#if 1
		_iqk_txk_setting_8822b(p_dm, RF_PATH_A);
		KFAIL = _iqk_one_shot_8822b(p_dm, RF_PATH_A, TXIQK);
			p_iqk_info->kcount++;
		ODM_RT_TRACE(p_dm, ODM_COMP_CALIBRATION, ODM_DBG_TRACE, ("[IQK]S0TXK KFail = 0x%x\n", KFAIL));

		if (KFAIL && (p_iqk_info->retry_count[0][RF_PATH_A][TXIQK] < 3))
			p_iqk_info->retry_count[0][RF_PATH_A][TXIQK]++;
		else
#endif
			p_dm->rf_calibrate_info.iqk_step++;
		halrf_iqk_xym_read(p_dm, RF_PATH_A, 0x1);
		break;
	case 4:		/*S1 TXIQK*/
#if 1
		_iqk_txk_setting_8822b(p_dm, RF_PATH_B);
		KFAIL = _iqk_one_shot_8822b(p_dm, RF_PATH_B,	TXIQK);
			p_iqk_info->kcount++;
		ODM_RT_TRACE(p_dm, ODM_COMP_CALIBRATION, ODM_DBG_TRACE, ("[IQK]S1TXK KFail = 0x%x\n", KFAIL));
		if (KFAIL && p_iqk_info->retry_count[0][RF_PATH_B][TXIQK] < 3)
			p_iqk_info->retry_count[0][RF_PATH_B][TXIQK]++;
		else
#endif
			p_dm->rf_calibrate_info.iqk_step++;
		halrf_iqk_xym_read(p_dm, RF_PATH_B, 0x1);
		break;
	case 5:		/*S0 RXIQK*/
			while (1) {
		KFAIL = _iqk_rx_iqk_by_path_8822b(p_dm, RF_PATH_A);
		ODM_RT_TRACE(p_dm, ODM_COMP_CALIBRATION, ODM_DBG_TRACE, ("[IQK]S0RXK KFail = 0x%x\n", KFAIL));
		if (p_iqk_info->rxiqk_step == 5) {
			p_dm->rf_calibrate_info.iqk_step++;
			p_iqk_info->rxiqk_step = 1;
			if (KFAIL)
				ODM_RT_TRACE(p_dm, ODM_COMP_CALIBRATION, ODM_DBG_LOUD,
					("[IQK]S0RXK fail code: %d!!!\n", p_iqk_info->RXIQK_fail_code[0][RF_PATH_A]));
					break;
				}
		}
			p_iqk_info->kcount++;
		break;
	case 6:		/*S1 RXIQK*/
			while (1) {
		KFAIL = _iqk_rx_iqk_by_path_8822b(p_dm, RF_PATH_B);
		ODM_RT_TRACE(p_dm, ODM_COMP_CALIBRATION, ODM_DBG_TRACE, ("[IQK]S1RXK KFail = 0x%x\n", KFAIL));
		if (p_iqk_info->rxiqk_step == 5) {
			p_dm->rf_calibrate_info.iqk_step++;
			p_iqk_info->rxiqk_step = 1;
			if (KFAIL)
				ODM_RT_TRACE(p_dm, ODM_COMP_CALIBRATION, ODM_DBG_LOUD,
					("[IQK]S1RXK fail code: %d!!!\n", p_iqk_info->RXIQK_fail_code[0][RF_PATH_B]));
					break;
				}
		}
			p_iqk_info->kcount++;
		break;
	}

	if (p_dm->rf_calibrate_info.iqk_step == 7) {
		ODM_RT_TRACE(p_dm, ODM_COMP_CALIBRATION, ODM_DBG_TRACE,
			     ("[IQK]==========LOK summary ==========\n"));
		ODM_RT_TRACE(p_dm, ODM_COMP_CALIBRATION, ODM_DBG_LOUD,
			("[IQK]PathA_LOK_notready = %d, PathB_LOK1_notready = %d\n",
			p_iqk_info->LOK_fail[RF_PATH_A], p_iqk_info->LOK_fail[RF_PATH_B]));
		ODM_RT_TRACE(p_dm, ODM_COMP_CALIBRATION, ODM_DBG_TRACE,
			     ("[IQK]==========IQK summary ==========\n"));
		ODM_RT_TRACE(p_dm, ODM_COMP_CALIBRATION, ODM_DBG_LOUD,
			("[IQK]PathA_TXIQK_fail = %d, PathB_TXIQK_fail = %d\n",
			p_iqk_info->IQK_fail_report[0][RF_PATH_A][TXIQK], p_iqk_info->IQK_fail_report[0][RF_PATH_B][TXIQK]));
		ODM_RT_TRACE(p_dm, ODM_COMP_CALIBRATION, ODM_DBG_LOUD,
			("[IQK]PathA_RXIQK_fail = %d, PathB_RXIQK_fail = %d\n",
			p_iqk_info->IQK_fail_report[0][RF_PATH_A][RXIQK], p_iqk_info->IQK_fail_report[0][RF_PATH_B][RXIQK]));
		ODM_RT_TRACE(p_dm, ODM_COMP_CALIBRATION, ODM_DBG_LOUD,
			("[IQK]PathA_TXIQK_retry = %d, PathB_TXIQK_retry = %d\n",
			p_iqk_info->retry_count[0][RF_PATH_A][TXIQK], p_iqk_info->retry_count[0][RF_PATH_B][TXIQK]));
		ODM_RT_TRACE(p_dm, ODM_COMP_CALIBRATION, ODM_DBG_LOUD,
			("[IQK]PathA_RXK1_retry = %d, PathA_RXK2_retry = %d, PathB_RXK1_retry = %d, PathB_RXK2_retry = %d\n",
			p_iqk_info->retry_count[0][RF_PATH_A][RXIQK1], p_iqk_info->retry_count[0][RF_PATH_A][RXIQK2],
			p_iqk_info->retry_count[0][RF_PATH_B][RXIQK1], p_iqk_info->retry_count[0][RF_PATH_B][RXIQK2]));
		ODM_RT_TRACE(p_dm, ODM_COMP_CALIBRATION, ODM_DBG_LOUD,
			("[IQK]PathA_GS1_retry = %d, PathA_GS2_retry = %d, PathB_GS1_retry = %d, PathB_GS2_retry = %d\n",
			p_iqk_info->gs_retry_count[0][RF_PATH_A][RXIQK1], p_iqk_info->gs_retry_count[0][RF_PATH_A][RXIQK2],
			p_iqk_info->gs_retry_count[0][RF_PATH_B][RXIQK1], p_iqk_info->gs_retry_count[0][RF_PATH_B][RXIQK2]));
		for (i = 0; i < 2; i++) {
			odm_write_4byte(p_dm, 0x1b00, 0xf8000008 | i << 1);
			odm_write_4byte(p_dm, 0x1b2c, 0x7);
			odm_write_4byte(p_dm, 0x1bcc, 0x0);
			odm_write_4byte(p_dm, 0x1b38, 0x20000000);
		}
			break;
	}

		if ((segment_iqk == true) && (p_iqk_info->kcount == kcount_limit))
			break;
#endif
}
}

void
_iqk_start_iqk_8822b(
	struct PHY_DM_STRUCT		*p_dm,
	boolean			segment_iqk
)
{
	u32 tmp;

	/*GNT_WL = 1*/
	tmp = odm_get_rf_reg(p_dm, RF_PATH_A, 0x1, RFREGOFFSETMASK);
	tmp = tmp | BIT(5) | BIT(0);
	odm_set_rf_reg(p_dm, RF_PATH_A, 0x1, RFREGOFFSETMASK, tmp);

	tmp = odm_get_rf_reg(p_dm, RF_PATH_B, 0x1, RFREGOFFSETMASK);
	tmp = tmp | BIT(5) | BIT(0);
	odm_set_rf_reg(p_dm, RF_PATH_B, 0x1, RFREGOFFSETMASK, tmp);

	_iqk_iqk_by_path_8822b(p_dm, segment_iqk);


}

void
_iq_calibrate_8822b_init(
	struct PHY_DM_STRUCT		*p_dm
)
{
	struct _IQK_INFORMATION	*p_iqk_info = &p_dm->IQK_info;
	u8	i, j, k, m;
	static boolean firstrun = true;

	if (firstrun) {
		firstrun = false;
		ODM_RT_TRACE(p_dm, ODM_COMP_CALIBRATION, ODM_DBG_LOUD, ("[IQK]=====>PHY_IQCalibrate_8822B_Init\n"));

		for (i = 0; i < SS_8822B; i++) {
			for (j = 0; j < 2; j++) {
				p_iqk_info->LOK_fail[i] = true;
				p_iqk_info->IQK_fail[j][i] = true;
				p_iqk_info->iqc_matrix[j][i] = 0x20000000;
			}
		}

		for (i = 0; i < 2; i++) {
			p_iqk_info->iqk_channel[i] = 0x0;

			for (j = 0; j < SS_8822B; j++) {
				p_iqk_info->LOK_IDAC[i][j] = 0x0;
				p_iqk_info->RXIQK_AGC[i][j] = 0x0;
				p_iqk_info->bypass_iqk[i][j] = 0x0;

				for (k = 0; k < 2; k++) {
					p_iqk_info->IQK_fail_report[i][j][k] = true;
					for (m = 0; m < 8; m++) {
						p_iqk_info->IQK_CFIR_real[i][j][k][m] = 0x0;
						p_iqk_info->IQK_CFIR_imag[i][j][k][m] = 0x0;
					}
				}

				for (k = 0; k < 3; k++)
					p_iqk_info->retry_count[i][j][k] = 0x0;

			}
		}
	}
	/*parameters init.*/
	/*cu_distance (IQK result variation)=111*/
	odm_write_4byte(p_dm, 0x1b10, 0x88011c00);
}


void
_phy_iq_calibrate_8822b(
	struct PHY_DM_STRUCT		*p_dm,
	boolean			reset,
	boolean			segment_iqk
)
{

	u32	MAC_backup[MAC_REG_NUM_8822B], BB_backup[BB_REG_NUM_8822B], RF_backup[RF_REG_NUM_8822B][SS_8822B];
	u32	backup_mac_reg[MAC_REG_NUM_8822B] = {0x520, 0x550};
	u32	backup_bb_reg[BB_REG_NUM_8822B] = {0x808, 0x90c, 0xc00, 0xcb0, 0xcb4, 0xcbc, 0xe00, 0xeb0, 0xeb4, 0xebc, 0x1990, 0x9a4, 0xa04, 0xb00, 0x838};
	u32	backup_rf_reg[RF_REG_NUM_8822B] = {0xdf, 0x8f, 0x65, 0x0, 0x1};
	boolean is_mp = false;

	struct _IQK_INFORMATION	*p_iqk_info = &p_dm->IQK_info;

	if (*(p_dm->p_mp_mode))
		is_mp = true;

	if (!is_mp)
		if (_iqk_reload_iqk_8822b(p_dm, reset))
			return;

	ODM_RT_TRACE(p_dm, ODM_COMP_CALIBRATION, ODM_DBG_TRACE,
		     ("[IQK]==========IQK strat!!!!!==========\n"));

	ODM_RT_TRACE(p_dm, ODM_COMP_CALIBRATION, ODM_DBG_LOUD,
		("[IQK]p_band_type = %s, band_width = %d, ExtPA2G = %d, ext_pa_5g = %d\n", (*p_dm->p_band_type == ODM_BAND_5G) ? "5G" : "2G", *p_dm->p_band_width, p_dm->ext_pa, p_dm->ext_pa_5g));
	ODM_RT_TRACE(p_dm, ODM_COMP_CALIBRATION, ODM_DBG_LOUD,
		("[IQK]Interface = %d, cut_version = %x\n", p_dm->support_interface, p_dm->cut_version));

	p_iqk_info->iqk_times++;
	p_iqk_info->kcount = 0;
	p_dm->rf_calibrate_info.iqk_step = 1;
	p_iqk_info->rxiqk_step = 1;

	_iqk_backup_iqk_8822b(p_dm, 0x0, 0x0);
	_iqk_backup_mac_bb_8822b(p_dm, MAC_backup, BB_backup, backup_mac_reg, backup_bb_reg);
	_iqk_backup_rf_8822b(p_dm, RF_backup, backup_rf_reg);
#if 0
	_iqk_configure_macbb_8822b(p_dm);
	_iqk_afe_setting_8822b(p_dm, true);
	_iqk_rfe_setting_8822b(p_dm, false);
	_iqk_agc_bnd_int_8822b(p_dm);
	_iqk_rf_setting_8822b(p_dm);
#endif

	while (1) {
		_iqk_configure_macbb_8822b(p_dm);
		_iqk_afe_setting_8822b(p_dm, true);
		_iqk_rfe_setting_8822b(p_dm, false);
		_iqk_agc_bnd_int_8822b(p_dm);
		_iqk_rf_setting_8822b(p_dm);
		_iqk_start_iqk_8822b(p_dm, segment_iqk);
		_iqk_afe_setting_8822b(p_dm, false);
		_iqk_restore_mac_bb_8822b(p_dm, MAC_backup, BB_backup, backup_mac_reg, backup_bb_reg);
		_iqk_restore_rf_8822b(p_dm, backup_rf_reg, RF_backup);
		if (p_dm->rf_calibrate_info.iqk_step == 7)
			break;
		p_iqk_info->kcount = 0;
		ODM_RT_TRACE(p_dm, ODM_COMP_CALIBRATION, ODM_DBG_LOUD, ("[IQK]delay 50ms!!!\n"));
		ODM_delay_ms(50);
	};
	if (segment_iqk)
		_iqk_reload_iqk_setting_8822b(p_dm, 0x0, 0x1);
#if 0
	_iqk_afe_setting_8822b(p_dm, false);
	_iqk_restore_mac_bb_8822b(p_dm, MAC_backup, BB_backup, backup_mac_reg, backup_bb_reg);
	_iqk_restore_rf_8822b(p_dm, backup_rf_reg, RF_backup);
#endif
	_iqk_fill_iqk_report_8822b(p_dm, 0);
	_iqk_rf0xb0_workaround(p_dm);
	ODM_RT_TRACE(p_dm, ODM_COMP_CALIBRATION, ODM_DBG_TRACE, ("[IQK]==========IQK end!!!!!==========\n"));
}


void
_phy_iq_calibrate_by_fw_8822b(
	void		*p_dm_void,
	u8		clear,
	u8		segment_iqk
)
{
	struct PHY_DM_STRUCT	*p_dm = (struct PHY_DM_STRUCT *)p_dm_void;
	struct _IQK_INFORMATION	*p_iqk_info = &p_dm->IQK_info;	
	enum hal_status		status = HAL_STATUS_FAILURE;
	
	if (*(p_dm->p_mp_mode))
		clear = 0x1;
//	else if (p_dm->is_linked)
//		segment_iqk = 0x1;

	p_iqk_info->iqk_times++;
	status = odm_iq_calibrate_by_fw(p_dm, clear, segment_iqk);

	if (status == HAL_STATUS_SUCCESS)
		ODM_RT_TRACE(p_dm, ODM_COMP_CALIBRATION, ODM_DBG_LOUD, ("[IQK]FWIQK OK!!!\n"));
	else
		ODM_RT_TRACE(p_dm, ODM_COMP_CALIBRATION, ODM_DBG_LOUD, ("[IQK]FWIQK fail!!!\n"));
}

/*IQK_version:0x2f, NCTL:0x8*/
/*1.disable CCK block and OFDM CCA block while IQKing*/
void
phy_iq_calibrate_8822b(
	void		*p_dm_void,
	boolean		clear,
	boolean		segment_iqk
)
{
	struct PHY_DM_STRUCT	*p_dm = (struct PHY_DM_STRUCT *)p_dm_void;
	struct _hal_rf_				*p_rf = &(p_dm->rf_table);

	if (*(p_dm->p_mp_mode))
		halrf_iqk_hwtx_check(p_dm, true);
	/*FW IQK*/
	if (p_dm->fw_offload_ability & PHYDM_RF_IQK_OFFLOAD) {
		_phy_iq_calibrate_by_fw_8822b(p_dm, clear, (u8)(segment_iqk));
		phydm_get_read_counter(p_dm);
		halrf_iqk_check_if_reload(p_dm);
	} else {
		_iq_calibrate_8822b_init(p_dm);
		_phy_iq_calibrate_8822b(p_dm, clear, segment_iqk);
	}
	_iqk_fail_count_8822b(p_dm);
	if (*(p_dm->p_mp_mode))
		halrf_iqk_hwtx_check(p_dm, false);
#if (DM_ODM_SUPPORT_TYPE & ODM_AP)
	_iqk_iqk_fail_report_8822b(p_dm);
#endif
	halrf_iqk_dbg(p_dm);
}

#endif
