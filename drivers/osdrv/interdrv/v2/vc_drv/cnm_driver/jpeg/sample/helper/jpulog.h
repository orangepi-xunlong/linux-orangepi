//-----------------------------------------------------------------------------
// COPYRIGHT (C) 2020   CHIPS&MEDIA INC. ALL RIGHTS RESERVED
//
// This file is distributed under BSD 3 clause and LGPL2.1 (dual license)
// SPDX License Identifier: BSD-3-Clause
// SPDX License Identifier: LGPL-2.1-only
//
// The entire notice above must be reproduced on all authorized copies.
//
// Description  :
//-----------------------------------------------------------------------------
#ifndef _LOG_H_
#define _LOG_H_

extern unsigned int jpu_log_level;

enum {NONE=0, ERR=0x1, WARN=0x2, TRACE=0x4, INFO=0x8, MAX_LOG_LEVEL};

#define JLOG(level, fmt, ...) \
    do { \
        if (level & jpu_log_level) { \
            if (level & ERR) \
                pr_err("[ERR] %s:%d: " fmt, __func__, __LINE__, ##__VA_ARGS__); \
            else if (level & WARN) \
                pr_warn("[WARN] %s:%d: " fmt, __func__, __LINE__, ##__VA_ARGS__); \
            else if (level & TRACE) \
                pr_notice("[TRACE] %s:%d: " fmt, __func__, __LINE__, ##__VA_ARGS__); \
            else if (level & INFO) \
                pr_info("[INFO] %s:%d: " fmt, __func__, __LINE__, ##__VA_ARGS__); \
        } \
    } while (0)


#endif //#ifndef _LOG_H_
