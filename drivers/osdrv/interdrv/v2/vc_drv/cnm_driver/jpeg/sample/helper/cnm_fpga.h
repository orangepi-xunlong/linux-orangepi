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
#ifndef CNM_FPGA_H_INCLUDED
#define CNM_FPGA_H_INCLUDED

#include "jputypes.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef struct {
    Uint32  aclk;
    Uint32  cclk;
    BOOL    reset;
    Uint32  aclk_div;
    Uint32  cclk_div;
} TestDevConfig;

extern BOOL CNM_InitTestDev(
    TestDevConfig   config
    );

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* CNM_FPGA_H_INCLUDED */
