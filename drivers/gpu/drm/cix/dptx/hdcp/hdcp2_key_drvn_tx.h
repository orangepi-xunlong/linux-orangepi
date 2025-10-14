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
#ifndef HDCP2_KEY_DRVN_TX__
#define HDCP2_KEY_DRVN_TX__

#include "cix_hdcp.h"
//------------------------------------------------------------------------------
//  Module Typedefs
//------------------------------------------------------------------------------
typedef enum {
	aes_tx_sel_disabled,
	aes_tx_sel_normal,
	aes_tx_sel_key_derivation,
	aes_tx_sel_ekh,
} aes_tx_select_t;

//------------------------------------------------------------------------------
//  Functional Interface
//------------------------------------------------------------------------------
bool hdcp2_key_drvn_tx_init(struct cix_hdcp *hdcp, void *kdata);
bool hdcp2_key_drvn_tx_reset(struct cix_hdcp *hdcp);
bool hdcp2_key_drvn_tx_src_select(struct cix_hdcp *hdcp, aes_tx_select_t sel);
bool hdcp2_key_drvn_tx_load(struct cix_hdcp *hdcp, void *kdata);
bool hdcp2_key_drvn_tx_ctr_reset(struct cix_hdcp *hdcp);
bool hdcp2_key_drvn_tx_ctr_inc(struct cix_hdcp *hdcp);
bool hdcp2_key_drvn_tx_kd_calc(struct cix_hdcp *hdcp, void *kdata);
bool hdcp2_key_drvn_tx_kd_generate(struct cix_hdcp *hdcp, void *kdata);
bool hdcp2_key_drvn_tx_dkey2_calc(struct cix_hdcp *hdcp, void *kdata);

#endif // HDCP2_KEY_DRVN_TX__
