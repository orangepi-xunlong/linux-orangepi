/******************************************************************************
 *
 * Copyright(c) 2013 - 2017 Realtek Corporation.
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
#ifndef __RTW_ODM_H__
#define __RTW_ODM_H__

#include <drv_types.h>
#include "../hal/phydm/phydm_types.h"
/*
* This file provides utilities/wrappers for rtw driver to use ODM
*/
typedef enum _HAL_PHYDM_OPS {
	HAL_PHYDM_DIS_ALL_FUNC,
	HAL_PHYDM_FUNC_SET,
	HAL_PHYDM_FUNC_CLR,
	HAL_PHYDM_ABILITY_BK,
	HAL_PHYDM_ABILITY_RESTORE,
	HAL_PHYDM_ABILITY_SET,
	HAL_PHYDM_ABILITY_GET,
} HAL_PHYDM_OPS;


#define DYNAMIC_FUNC_DISABLE		(0x0)
	u32 rtw_phydm_ability_ops(_adapter *adapter, HAL_PHYDM_OPS ops, u32 ability);

#define rtw_phydm_func_disable_all(adapter)	\
		rtw_phydm_ability_ops(adapter, HAL_PHYDM_DIS_ALL_FUNC, 0)

#ifdef CONFIG_RTW_ACS
#define rtw_phydm_func_for_offchannel(adapter) \
		do { \
			rtw_phydm_ability_ops(adapter, HAL_PHYDM_DIS_ALL_FUNC, 0); \
			if (rtw_cfg_adaptivity_needed(adapter)) \
				rtw_phydm_ability_ops(adapter, HAL_PHYDM_FUNC_SET, ODM_BB_ADAPTIVITY); \
			if (IS_ACS_ENABLE(adapter))\
				rtw_phydm_ability_ops(adapter, HAL_PHYDM_FUNC_SET, ODM_BB_ENV_MONITOR); \
		} while (0)
#else
#define rtw_phydm_func_for_offchannel(adapter) \
		do { \
			rtw_phydm_ability_ops(adapter, HAL_PHYDM_DIS_ALL_FUNC, 0); \
			if (rtw_cfg_adaptivity_needed(adapter)) \
				rtw_phydm_ability_ops(adapter, HAL_PHYDM_FUNC_SET, ODM_BB_ADAPTIVITY); \
		} while (0)
#endif

#define rtw_phydm_func_clr(adapter, ability)	\
		rtw_phydm_ability_ops(adapter, HAL_PHYDM_FUNC_CLR, ability)

#define rtw_phydm_ability_backup(adapter)	\
		rtw_phydm_ability_ops(adapter, HAL_PHYDM_ABILITY_BK, 0)

#define rtw_phydm_ability_restore(adapter)	\
		rtw_phydm_ability_ops(adapter, HAL_PHYDM_ABILITY_RESTORE, 0)


static inline u32 rtw_phydm_ability_get(_adapter *adapter)
{
	return rtw_phydm_ability_ops(adapter, HAL_PHYDM_ABILITY_GET, 0);
}


void rtw_odm_init_ic_type(_adapter *adapter);

void rtw_edcca_hal_update(struct dvobj_priv *dvobj);
void rtw_odm_adaptivity_ver_msg(void *sel, _adapter *adapter);
void rtw_odm_adaptivity_parm_msg(void *sel, _adapter *adapter);
void rtw_odm_adaptivity_parm_set(_adapter *adapter, s8 th_l2h_ini, s8 th_edcca_hl_diff);
void rtw_odm_get_perpkt_rssi(void *sel, _adapter *adapter);
void rtw_odm_acquirespinlock(_adapter *adapter,	enum rt_spinlock_type type);
void rtw_odm_releasespinlock(_adapter *adapter,	enum rt_spinlock_type type);

struct dm_struct;
s16 rtw_odm_get_tx_power_mbm(struct dm_struct *dm, u8 rfpath, u8 rate, u8 bw, u8 cch);

#if CONFIG_TXPWR_LIMIT
enum txpwr_lmt_reg_exc_match {
	TXPWR_LMT_REG_EXC_MATCH_NONE = 0,
	TXPWR_LMT_REG_EXC_MATCH_COUNTRY,
	TXPWR_LMT_REG_EXC_MATCH_DOMAIN,
};

enum txpwr_lmt_reg_exc_match rtw_txpwr_hal_lmt_reg_exc_search(struct dvobj_priv* dvobj, const char *country, u8 domain, const char **reg_name);
bool rtw_txpwr_hal_lmt_reg_search(struct dvobj_priv* dvobj, enum band_type band, const char *name);
void rtw_txpwr_hal_set_current_lmt_regs_by_name(struct dvobj_priv* dvobj, char *names_of_band[], int names_len_of_band[]);
void rtw_txpwr_hal_get_current_lmt_regs_name(struct dvobj_priv* dvobj, char *names_of_band[], int names_len_of_band[]);
#endif /* CONFIG_TXPWR_LIMIT */

#ifdef CONFIG_DFS_MASTER
bool rtw_dfs_hal_region_supported(struct dvobj_priv* dvobj, enum rtw_dfs_regd domain);
void rtw_dfs_hal_update_region(struct dvobj_priv *dvobj, u8 band_idx, enum rtw_dfs_regd domain);
void rtw_dfs_hal_radar_detect_disable(struct dvobj_priv *dvobj, u8 band_idx);
void rtw_dfs_hal_radar_detect_enable(struct dvobj_priv *dvobj, u8 band_idx, bool cac, u32 rd_freq_hi, u32 rd_freq_lo);
void rtw_dfs_hal_set_cac_status(struct dvobj_priv *dvobj, u8 band_idx, bool cac);
void rtw_dfs_hal_radar_detect_result_polling(struct dvobj_priv *dvobj, u8 band_idx, u8 *radar_cch, u8 *radar_bw);
u8 rtw_dfs_hal_radar_detect_polling_int_ms(struct dvobj_priv *dvobj);
#endif /* CONFIG_DFS_MASTER */

void rtw_odm_parse_rx_phy_status_chinfo(union recv_frame *rframe, u8 *phys);

#if defined(CONFIG_RTL8822C) && defined(CONFIG_LPS_PG)
void odm_lps_pg_debug_8822c(void *dm_void);
#endif

#endif /* __RTW_ODM_H__ */
