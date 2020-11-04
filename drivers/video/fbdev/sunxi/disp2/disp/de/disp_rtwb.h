/*
 * Allwinner SoCs display driver.
 *
 * Copyright (C) 2016 Allwinner.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */


#ifndef __DISP_RTWB_H__
#define __DISP_RTWB_H__

#include "disp_private.h"

/**
 * @name       :disp_init_rtwb
 * @brief      :register rtwb display device
 * @param[IN]  :para:pointer of hardware resource
 * @return     :0 if success
 */
s32 disp_init_rtwb(struct disp_bsp_init_para *para);

/**
 * @name       :disp_exit_rtwb
 * @brief      :unregister rtwb display device
 * @return     :0 if success
 */
s32 disp_exit_rtwb(void);

/**
 * @name       :disp_rtwb_config
 * @brief      :config rtwb in out param
 * @param[IN]  :mgr:pointer of display manager
 * @param[IN]  :info:pointer of capture info
 * @return     :dmabuf item pointer, NULL if fail
 */
struct dmabuf_item *disp_rtwb_config(struct disp_manager *mgr, struct disp_capture_info2 *info);

/**
 * @name       :disp_rtwb_wait_finish
 * @brief      :wait for rtwb hardware finish
 * @param[IN]  :mgr:pointer of display manger which attach with rtwb device
 * @return     :1 if success
 */
s32 disp_rtwb_wait_finish(struct disp_manager *mgr);

#endif
