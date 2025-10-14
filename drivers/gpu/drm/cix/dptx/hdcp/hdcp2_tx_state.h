//------------------------------------------------------------------------------
//  COPYRIGHT (c) 2018-2022 TRILINEAR TECHNOLOGIES, INC.
//  CONFIDENTIAL AND PROPRIETARY
//
//  THE SOURCE CODE CONTAINED HEREIN IS PROVIDED ON AN "AS IS" BASIS.
//  TRILINEAR TECHNOLOGIES, INC. DISCLAIMS ANY AND ALL WARRANTIES,
//  WHETHER EXPRESS, IMPLIED, OR STATUTORY, INCLUDING ANY IMPLIED
//  WARRANTIES OF MERCHANTABILITY OR OF FITNESS FOR A PARTICULAR PURPOSE.
//  IN NO EVENT SHALL TRILINEAR TECHNOLOGIES, INC. BE LIABLE FOR ANY
//  INCIDENTAL, PUNITIVE, OR CONSEQUENTIAL DAMAGES OF ANY KIND WHATSOEVER
//  ARISING FROM THE USE OF THIS SOURCE CODE.
//
//  THIS DISCLAIMER OF WARRANTY EXTENDS TO THE USER OF THIS SOURCE CODE
//  AND USER'S CUSTOMERS, EMPLOYEES, AGENTS, TRANSFEREES, SUCCESSORS,
//  AND ASSIGNS.
//
//  THIS IS NOT A GRANT OF PATENT RIGHTS
//------------------------------------------------------------------------------
#ifndef HDCP2_TX_STATE__
#define HDCP2_TX_STATE__

#include "cix_hdcp.h"

//------------------------------------------------------------------------------
//  Functional Interface
//------------------------------------------------------------------------------
void hdcp2_cipher_enable(struct cix_hdcp *hdcp, void *kdata);
void hdcp2_cipher_disable(struct cix_hdcp *hdcp);
void hdcp2_tx_state_init(struct cix_hdcp *hdcp);

#endif // HDCP2_TX_STATE__
