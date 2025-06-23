// SPDX-License-Identifier: GPL-2.0
/*
 * display port hdcp driver for the cix ec
 *
 * Copyright 2024 Cix Technology Group Co., Ltd.
 */

#ifndef HDCP2_TX_TMR__
#define HDCP2_TX_TMR__

#include "cix_hdcp.h"

//------------------------------------------------------------------------------
//  Functional Interface
//------------------------------------------------------------------------------
bool hdcp2_tx_tmr_start(struct cix_hdcp *hdcp, uint32_t ms);
void hdcp2_tx_tmr_stop(struct cix_hdcp *hdcp);

#endif // HDCP2_TX_TMR__
