/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * Allwinner SoCs hdmi2.0 driver.
 *
 * Copyright (C) 2016 Allwinner.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */
#ifndef _DW_HDCP22_H
#define _DW_HDCP22_H

#include <linux/workqueue.h>

int dw_esm_tx_initial(unsigned long esm_hpi_base,
			      u32 code_base,
			      unsigned long vir_code_base,
			      u32 code_size,
			      u32 data_base,
			      unsigned long vir_data_base,
			      u32 data_size);

void dw_esm_tx_exit(void);
int dw_esm_open(void);
void dw_esm_close(void);
void dw_esm_disable(void);
void dw_hdcp22_esm_start_auth(void);
int dw_hdcp22_status_check_and_handle(void);
ssize_t dw_hdcp22_esm_dump(char *buf);
#endif /* _DW_HDCP22_H */
