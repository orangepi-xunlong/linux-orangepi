#ifndef _AICWF_COMPAT_8800D80_H_
#define _AICWF_COMPAT_8800D80_H_
#include <linux/types.h>
#include "aic_bsp_export.h"

int aicwf_set_rf_config_8800d80(struct rwnx_hw *rwnx_hw, struct mm_set_rf_calib_cfm *cfm);

#endif

