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
#ifndef _RTL8852BS_HAL_H_
#define _RTL8852BS_HAL_H_
/* rtl8852bs_halinit.c */
void init_hal_spec_8852bs(struct rtw_phl_com_t *phl_com,
					struct hal_info_t *hal);
enum rtw_hal_status hal_get_efuse_8852bs(struct rtw_phl_com_t *phl_com,
					 struct hal_info_t *hal);

enum rtw_hal_status hal_init_8852bs(struct rtw_phl_com_t *phl_com,
				    struct hal_info_t *hal);
void hal_deinit_8852bs(struct rtw_phl_com_t *phl_com,
		       struct hal_info_t *hal);

enum rtw_hal_status hal_start_8852bs(struct rtw_phl_com_t *phl_com,
				     struct hal_info_t *hal);
enum rtw_hal_status hal_stop_8852bs(struct rtw_phl_com_t *phl_com,
				    struct hal_info_t *hal);
#ifdef CONFIG_WOWLAN
enum rtw_hal_status hal_wow_init_8852bs(struct rtw_phl_com_t *phl_com, struct hal_info_t *hal, struct rtw_phl_stainfo_t *sta);
enum rtw_hal_status hal_wow_deinit_8852bs(struct rtw_phl_com_t *phl_com, struct hal_info_t *hal, struct rtw_phl_stainfo_t *sta);
#endif /* CONFIG_WOWLAN */

enum rtw_hal_status hal_mp_init_8852bs(struct rtw_phl_com_t *phl_com, struct hal_info_t *hal);
enum rtw_hal_status hal_mp_deinit_8852bs(struct rtw_phl_com_t *phl_com, struct hal_info_t *hal);
bool hal_mp_path_chk_8852bs(struct rtw_phl_com_t *phl_com, u8 ant_tx, u8 cur_phy);


u32 hal_hci_cfg_8852bs(struct rtw_phl_com_t *phl_com,
		struct hal_info_t *hal, struct rtw_ic_info *ic_info);
void init_default_value_8852bs(struct hal_info_t *hal);
void init_int_default_value_8852bs(struct hal_info_t *hal, enum rtw_hal_int_set_opt opt);

void sd_int_hdl_8852bs(void *h);

void hal_enable_int_8852bs(struct hal_info_t *hal);
void hal_disable_int_8852bs(struct hal_info_t *hal);
void hal_config_int_8852bs(struct hal_info_t *hal, enum rtw_phl_config_int int_mode);
bool hal_recognize_int_8852bs(struct hal_info_t *hal);
bool hal_recognize_halt_c2h_int_8852bs(struct hal_info_t *hal);
void hal_clear_interrupt_8852bs(struct hal_info_t *hal);
u32 hal_int_hdler_8852bs(struct hal_info_t *hal);


#endif /* _RTL8852BS_HAL_H_ */
