//  SPDX-License-Identifier: GPL-2.0
//  Copyright 2024 Cix Technology Group Co., Ltd.

#ifndef __TRILIN_DP_MST_H__
#define __TRILIN_DP_MST_H__

int trilin_drm_mst_encoder_init(struct trilin_dp *dp, int conn_id);
void trilin_drm_mst_encoder_cleanup(struct trilin_dp *dp);
int trilin_dp_set_mst_mgr_state(struct trilin_dp *dp, bool state);
void trilin_dp_mst_display_hpd_irq(struct trilin_dp *dp);
int trilin_dp_mst_suspend(struct trilin_dp *dp);
int trilin_dp_mst_resume(struct trilin_dp *dp);

#endif /* __TRILIN_DP_MST_H__ */
