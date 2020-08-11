/* disp_edp.h
 *
 * Copyright (c) 2007-2017 Allwinnertech Co., Ltd.
 * Author: zhengxiaobin <zhengxiaobin@allwinnertech.com>
 *
 * register edp
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/
#ifndef _DISP_EDP_H
#define _DISP_EDP_H

#include "disp_private.h"
#include "disp_display.h"


s32 disp_init_edp(struct disp_bsp_init_para *para);

/**
 * @name       :disp_edp_get_cur_line
 * @brief      :get current line
 * @param[IN]  :p_edp
 * @return     :current line.negtive if fail
 */
int disp_edp_get_cur_line(struct disp_device *p_edp);

#endif /*End of file*/
