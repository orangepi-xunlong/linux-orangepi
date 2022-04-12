/**
 ****************************************************************************************
 *
 * @file bl_strs.h
 *
 * @brief Miscellaneous debug strings
 *
 * Copyright (C) BouffaloLab 2017-2018
 *
 ****************************************************************************************
 */

#ifndef _BL_STRS_H_
#define _BL_STRS_H_

#include "lmac_msg.h"

#define BL_ID2STR(tag) (((MSG_T(tag) < ARRAY_SIZE(bl_id2str)) &&        \
                           (bl_id2str[MSG_T(tag)]) &&          \
                           ((bl_id2str[MSG_T(tag)])[MSG_I(tag)])) ?   \
                          (bl_id2str[MSG_T(tag)])[MSG_I(tag)] : "unknown")

extern const char *const *bl_id2str[TASK_LAST_EMB + 1];

#endif /* _BL_STRS_H_ */
