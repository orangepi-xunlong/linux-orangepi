/**
 ******************************************************************************
 *
 * @file bl_main.h
 *
 * Copyright (C) BouffaloLab 2017-2018
 *
 ******************************************************************************
 */

#ifndef _BL_MAIN_H_
#define _BL_MAIN_H_

#include "bl_defs.h"

int bl_cfg80211_init(struct bl_plat *bl_plat, void **platform_data);
void bl_cfg80211_deinit(struct bl_hw *bl_hw);

#endif /* _BL_MAIN_H_ */
