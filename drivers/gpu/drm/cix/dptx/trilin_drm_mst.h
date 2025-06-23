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

#ifndef __TRILIN_DP_MST_H__
#define __TRILIN_DP_MST_H__

int trilin_drm_mst_encoder_init(struct trilin_dp *dp, int conn_id);
void trilin_drm_mst_encoder_cleanup(struct trilin_dp *dp);
int trilin_dp_set_mst_mgr_state(struct trilin_dp *dp, bool state);
void trilin_dp_mst_display_hpd_irq(struct trilin_dp *dp);
int trilin_dp_mst_suspend(struct trilin_dp *dp);
int trilin_dp_mst_resume(struct trilin_dp *dp);

#endif /* __TRILIN_DP_MST_H__ */
