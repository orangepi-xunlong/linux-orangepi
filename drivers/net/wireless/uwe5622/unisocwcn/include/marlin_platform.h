/*
 * Copyright (C) 2016 Spreadtrum Communications Inc.
 *
 * Filename : marlin.h
 * Abstract : This file is a implementation for driver of marlin2
 *
 * Authors	: yufeng.yang
 *
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */
#ifndef __MARLIN_H__
#define __MARLIN_H__

#include <linux/types.h>

#define FALSE								(0)
#define TRUE								(1)

typedef int (*marlin_reset_callback) (void *para);
extern marlin_reset_callback marlin_reset_func;
extern void *marlin_callback_para;
enum marlin_sub_sys {
	MARLIN_BLUETOOTH = 0,
	MARLIN_FM,
	MARLIN_WIFI,
	MARLIN_WIFI_FLUSH,
	MARLIN_SDIO_TX,
	MARLIN_SDIO_RX,
	MARLIN_MDBG,
	MARLIN_GNSS,
	WCN_AUTO,	/* fist GPS, then btwififm */
	MARLIN_ALL,
};

enum wcn_chip_model {
	WCN_CHIP_INVALID,
	WCN_CHIP_MARLIN3,
	WCN_CHIP_MARLIN3L,
	WCN_CHIP_MARLIN3E,
};

enum wcn_chip_id_type {
	WCN_CHIP_ID_INVALID,
	WCN_CHIP_ID_AA,
	WCN_CHIP_ID_AB,
	WCN_CHIP_ID_AC,
	WCN_CHIP_ID_AD,
};

enum wcn_clock_type {
	WCN_CLOCK_TYPE_UNKNOWN,
	WCN_CLOCK_TYPE_TCXO,
	WCN_CLOCK_TYPE_TSX,
};

enum wcn_clock_mode {
	WCN_CLOCK_MODE_UNKNOWN,
	WCN_CLOCK_MODE_XO,
	WCN_CLOCK_MODE_BUFFER,
};

/* number of marlin antennas */
enum marlin_ant_num {
	MARLIN_ANT_NOT_CFG = 0,
	MARLIN_ONE_ANT,
	MARLIN_TWO_ANT,
	MARLIN_THREE_ANT,
	MARLIN_ANT_NUM
};

enum marlin_wake_host_en {
	BT_WAKE_HOST = 0,
	WL_WAKE_HOST
};

unsigned int marlin_get_wcn_chipid(void);
const char *wcn_get_chip_name(void);
enum wcn_chip_model wcn_get_chip_model(void);
enum wcn_chip_id_type wcn_get_chip_type(void);
unsigned long marlin_get_power_state(void);
unsigned char marlin_get_bt_wl_wake_host_en(void);
int marlin_get_wcn_module_vendor(void);
int marlin_get_ant_num(void);
void marlin_power_off(enum marlin_sub_sys subsys);
int marlin_get_power(enum marlin_sub_sys subsys);
int marlin_reset_reg(void);
int start_marlin(u32 subsys);
int stop_marlin(u32 subsys);
void marlin_schedule_download_wq(void);
int open_power_ctl(void);
bool marlin_get_download_status(void);
void marlin_chip_en(bool enable, bool reset);
int marlin_get_module_status(void);
int marlin_get_module_status_changed(void);
int wcn_get_module_status_changed(void);
void wcn_set_module_status_changed(bool status);
int marlin_reset_register_notify(void *callback_func, void *para);
int marlin_reset_unregister_notify(void);
int is_first_power_on(enum marlin_sub_sys subsys);
int cali_ini_need_download(enum marlin_sub_sys subsys);
const char *strno(int subsys);
void mdbg_assert_interface(char *str);
#endif
