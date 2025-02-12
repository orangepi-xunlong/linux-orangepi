
/******************************************************************************
 *
 * Copyright(c) 2007 - 2022 Realtek Corporation.
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
#ifndef __RTW_CSI_H_
#define __RTW_CSI_H_

#ifdef CONFIG_RTW_CSI_NETLINK
int rtw_csi_nl_init(struct dvobj_priv *dvobj);
void rtw_csi_nl_exit(struct dvobj_priv *dvobj);
#endif

#ifdef CONFIG_CSI_TIMER_POLLING
void rtw_csi_poll_init(struct dvobj_priv *dvobj);
void rtw_csi_poll_timer_cancel(struct dvobj_priv *dvobj);
#endif

#endif /*__RTW_CSI_H_*/
