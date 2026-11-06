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
#if defined(_WIN32) || defined(__WIN32__) || defined(_WIN64) || defined(WIN32) || defined(__MINGW32__)
#include <windows.h>
#endif
#include <linux/printk.h>
#include <drv_file.h>
#include "../jpuapi/jpuconfig.h"
#include "jpulog.h"


unsigned int jpu_log_level = ERR;
module_param(jpu_log_level, uint, 0644);

