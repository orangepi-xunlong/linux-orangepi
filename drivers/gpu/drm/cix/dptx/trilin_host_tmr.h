// SPDX-License-Identifier: GPL-2.0
//------------------------------------------------------------------------------
//	Trilinear Technologies DisplayPort DRM Driver
//	Copyright (C) 2023 Trilinear Technologies
//
//	This program is free software: you can redistribute it and/or modify
//	it under the terms of the GNU General Public License as published by
//	the Free Software Foundation, version 2.
//
//	This program is distributed in the hope that it will be useful, but
//	WITHOUT ANY WARRANTY; without even the implied warranty of
//	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
//	General Public License for more details.
//
//	You should have received a copy of the GNU General Public License
//	along with this program. If not, see <http://www.gnu.org/licenses/>.
//------------------------------------------------------------------------------

#ifndef _TRILIN_TMR_H_
#define _TRILIN_TMR_H_

//------------------------------------------------------------------------------
// Driver structures
//------------------------------------------------------------------------------
struct trilin_dp;
struct trilin_dpsub;

//------------------------------------------------------------------------------
// Device Timer Register Format:
// bit 31   = Timer enable
// bit 30   = Auto-reload
// bit 29   = Interrupt enable
// bit 19:0 = Timer value (20-bits)
//------------------------------------------------------------------------------
#define DEV_TMR_FLAG_ENABLE             (1ul<<31)
#define DEV_TMR_FLAG_AUTORELOAD         (1ul<<30)
#define DEV_TMR_FLAG_INTERRUPT          (1ul<<29)
#define DEV_TMR_MAX_CT                  ((1ul<<20)-1ul)
#define DEV_TMR_FLAGS_ALL               (DEV_TMR_FLAG_ENABLE | DEV_TMR_FLAG_AUTORELOAD | DEV_TMR_FLAG_INTERRUPT)

//------------------------------------------------------------------------------
//  Public interface functions
//------------------------------------------------------------------------------
void trilin_host_tmr_init	(struct trilin_dp *dp);
u32  trilin_host_tmr_get_value	(struct trilin_dp *dp);

void trilin_host_tmr_set_ct	(struct trilin_dp *dp, u32 tmr_ct);
void trilin_host_tmr_set_us	(struct trilin_dp *dp, u32 tmr_us);
void trilin_host_tmr_set_ms	(struct trilin_dp *dp, u32 tmr_ms);

void trilin_host_tmr_enable	(struct trilin_dp *dp, bool enable);
void trilin_host_tmr_interrupt	(struct trilin_dp *dp, bool enable);
void trilin_host_tmr_autoreload	(struct trilin_dp *dp, bool enable);

void trilin_host_tmr_wait_ct	(struct trilin_dp *dp, u32 ct);
void trilin_host_tmr_wait_us	(struct trilin_dp *dp, u32 us);
void trilin_host_tmr_wait_ms	(struct trilin_dp *dp, u32 ms);

#endif
