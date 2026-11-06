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
#ifndef _PHL_TEST_MP_WATCHDOG_H_
#define _PHL_TEST_MP_WATCHDOG_H_

#define MP_WATCHDOG_PERIOD 2000

#ifdef CONFIG_PHL_TEST_MP

void rtw_phl_mp_watchdog_init(struct mp_context *mp_ctx);
void rtw_phl_mp_watchdog_deinit(struct mp_context *mp_ctx);
void rtw_phl_mp_watchdog_start(struct mp_context *mp_ctx);
void rtw_phl_mp_watchdog_stop(struct mp_context *mp_ctx);

#endif /* CONFIG_PHL_TEST_MP */
#endif /*_PHL_TEST_MP_WATCHDOG_H_*/

