/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright 2025 Cix Technology Group Co., Ltd.
 *
 */
#ifndef __max96724_H__
#define __max96724_H__

#include "gmsl-link.h"

int z_max96724_start_streaming(struct device *dev, struct gmsl_link_ctx *g_ctx);
int z_max96724_stop_streaming(struct device *dev, struct gmsl_link_ctx *g_ctx);

int z_max96724_check_link_status(struct device *dev, int link);
int z_max96724_monopolize_link(struct device *dev, int link);
int z_max96724_enable_link(struct device *dev, int link);
int z_max96724_restore_link(struct device *dev);
int z_max96724_check_link_status(struct device *dev, int link);
int z_max96724_set_link_bandwidth(struct device *dev, int link, int gbps);
int z_max96724_get_ox5b_config(struct device *dev, int link);
int z_max96724_lock_link(struct device *dev);
int z_max96724_unlock_link(struct device *dev);

int z_max96724_reverse_contrl_link_enable_all_map_port(struct device *dev);
int z_max96724_reverse_contrl_link_disbale(struct device *dev, int port_a, int port_b, int port_c, int port_d);
int z_max96724_reverse_contrl_set_map_port(struct device *dev, int port);

/** @} */

#endif  /* __max96724_H__ */
