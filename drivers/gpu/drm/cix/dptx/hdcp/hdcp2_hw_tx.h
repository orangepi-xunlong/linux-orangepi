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
#ifndef HDCP2_HW_TX__
#define HDCP2_HW_TX__

#include "cix_hdcp.h"
//------------------------------------------------------------------------------
//  This section is Trilinear display port I/O
//------------------------------------------------------------------------------
uint32_t hdcp2_hw_tx_interrupt_state_read(struct cix_hdcp *hdcp);

void hdcp2_hw_tx_enable_write(struct cix_hdcp *hdcp, bool enable);
uint32_t hdcp2_hw_tx_enable_read(struct cix_hdcp *hdcp);
void hdcp2_hw_tx_mode_write(struct cix_hdcp *hdcp, uint32_t val);
uint32_t hdcp2_hw_tx_mode_read(struct cix_hdcp *hdcp);

void hdcp2_hw_tx_hardware_keys(struct cix_hdcp *hdcp, bool enable);

bool hdcp2_hw_tx_km_write(struct cix_hdcp *hdcp, uint8_t *km);
bool hdcp2_hw_tx_km_read(struct cix_hdcp *hdcp, uint8_t *km);
bool hdcp2_hw_tx_ks_write(struct cix_hdcp *hdcp, uint8_t *ks);
bool hdcp2_hw_tx_ks_read(struct cix_hdcp *hdcp, uint8_t *ks);
bool hdcp2_hw_tx_rtx_write(struct cix_hdcp *hdcp, uint8_t *rtx);
bool hdcp2_hw_tx_rtx_read(struct cix_hdcp *hdcp, uint8_t *rtx);
bool hdcp2_hw_tx_riv_write(struct cix_hdcp *hdcp, uint8_t *rtx);
bool hdcp2_hw_tx_riv_read(struct cix_hdcp *hdcp, uint8_t *rtx);
bool hdcp2_hw_tx_rrx_write(struct cix_hdcp *hdcp, uint8_t *rrx);
bool hdcp2_hw_tx_rrx_read(struct cix_hdcp *hdcp, uint8_t *rrx);
bool hdcp2_hw_tx_dkey_read(struct cix_hdcp *hdcp, uint8_t *dkey);
bool hdcp2_hw_tx_lc128_write(struct cix_hdcp *hdcp, const uint8_t *lc128);
void hdcp2_hw_tx_repeater_write(struct cix_hdcp *hdcp, bool enable);
uint32_t hdcp2_hw_tx_repeater_read(struct cix_hdcp *hdcp, bool enable);
void hdcp2_hw_tx_stream_cipher_write(struct cix_hdcp *hdcp, bool enable);
uint32_t hdcp2_hw_tx_stream_cipher_read(struct cix_hdcp *hdcp);
void hdcp2_hw_tx_aes_input_write(struct cix_hdcp *hdcp, uint32_t input_id);
uint32_t hdcp2_hw_tx_aes_input_read(struct cix_hdcp *hdcp);
void hdcp2_hw_tx_aes_ctr_disable_write(struct cix_hdcp *hdcp, bool enable);
uint32_t hdcp2_hw_tx_aes_ctr_disable_read(struct cix_hdcp *hdcp);
void hdcp2_hw_tx_aes_ctr_inc(struct cix_hdcp *hdcp);
bool hdcp2_hw_tx_encryption_ctl_write(struct cix_hdcp *hdcp, uint8_t *ctl);
bool hdcp2_hw_tx_encryption_ctl_read(struct cix_hdcp *hdcp, uint8_t *ctl);
void hdcp2_hw_tx_aes_ctr_reset(struct cix_hdcp *hdcp);
bool hdcp2_hw_tx_rn_clear(struct cix_hdcp *hdcp);
bool hdcp2_hw_tx_rn_write(struct cix_hdcp *hdcp, uint8_t *ctl);
bool hdcp2_hw_tx_rn_read(struct cix_hdcp *hdcp, uint8_t *ctl);
#endif
