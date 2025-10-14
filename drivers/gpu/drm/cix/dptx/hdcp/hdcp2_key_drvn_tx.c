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
#include <linux/stddef.h>
#include <linux/string.h>

#include "cix_hdcp.h"
#include "cix_hdcp_ioctl_cmd.h"
#include "hdcp2_hw_tx.h"
#include "hdcp2_key_drvn_tx.h"

#define HDCP2_DKEY_SZ 16
#define HDCP2_KD_SZ 32
#define HDCP2_H_SZ 32
#define HDCP2_H_DATA_SZ \
	14 //  H-data is (rtx || RxCaps || TxCaps) this is 14-bytes
#define HDCP2_RTX_SZ 8
#define HDCP2_RN_SZ 8
#define HDCP2_TX_CAPS_SZ 3
#define HDCP2_RX_CAPS_SZ 3

//------------------------------------------------------------------------------
//  Function:   hdcp2_key_drvn_tx_init
//              Initialize key derivation module
//
//  Parameters:
//      hdcp2_hndl - hdcp handle
//
//  Returns:
//      true=OK / false=fail
//------------------------------------------------------------------------------
bool hdcp2_key_drvn_tx_init(struct cix_hdcp *hdcp, void *kdata)
{
	hdcp2_kd_t *tx_data = kdata;

	memset(tx_data->dkey0, 0, HDCP2_DKEY_SZ);
	memset(tx_data->dkey1, 0, HDCP2_DKEY_SZ);
	memset(tx_data->kd, 0, HDCP2_KD_SZ);

	return true;
}

//------------------------------------------------------------------------------
//  Function:   hdcp2_key_drvn_tx_reset
//              Reset key derivation hardware
//
//  Parameters:
//      hdcp - hdcp handle
//
//  Returns:
//      true=OK / false=fail
//------------------------------------------------------------------------------
bool hdcp2_key_drvn_tx_reset(struct cix_hdcp *hdcp)
{
	hdcp2_hw_tx_stream_cipher_write(hdcp, false);
	hdcp2_hw_tx_hardware_keys(hdcp, false);
	hdcp2_hw_tx_aes_ctr_reset(hdcp);

	return true;
}

//------------------------------------------------------------------------------
//  Function:   hdcp2_key_drvn_tx_src_select
//              Select source for key derivation hardware
//
//  Parameters:
//      hdcp - hdcp handle
//      sel - clock input source
//
//  Returns:
//      true=OK / false=fail
//------------------------------------------------------------------------------
bool hdcp2_key_drvn_tx_src_select(struct cix_hdcp *hdcp, aes_tx_select_t sel)
{
	switch (sel) {
	case aes_tx_sel_disabled:
		hdcp2_hw_tx_aes_input_write(hdcp, 0);
		break;
	case aes_tx_sel_normal:
		hdcp2_hw_tx_aes_input_write(hdcp, 1);
		break;
	case aes_tx_sel_key_derivation:
		hdcp2_hw_tx_aes_input_write(hdcp, 2);
		break;
	case aes_tx_sel_ekh:
		hdcp2_hw_tx_aes_input_write(hdcp, 4);
		break;
	default:
		break;
	}

	return true;
}

//------------------------------------------------------------------------------
//  Function:   hdcp2_key_drvn_tx_load
//              load rrx, rtx, and rn
//
//  Parameters:
//      hdcp - hdcp handle
//
//  Returns:
//      true=OK / false=fail
//------------------------------------------------------------------------------
bool hdcp2_key_drvn_tx_load(struct cix_hdcp *hdcp, void *kdata)
{
	hdcp2_kd_t *tx_data = kdata;

	hdcp2_hw_tx_rrx_write(hdcp, tx_data->rrx);
	hdcp2_hw_tx_rtx_write(hdcp, tx_data->rtx);
	hdcp2_hw_tx_rn_clear(hdcp);

	return true;
}

//------------------------------------------------------------------------------
//  Function:   hdcp2_key_drvn_tx_ctr_reset
//              Reset derivation counter
//
//  Parameters:
//      hdcp - hdcp handle
//
//  Returns:
//      true=OK / false=fail
//------------------------------------------------------------------------------
bool hdcp2_key_drvn_tx_ctr_reset(struct cix_hdcp *hdcp)
{
	hdcp2_hw_tx_aes_ctr_reset(hdcp);

	return true;
}

//------------------------------------------------------------------------------
//  Function:   hdcp2_key_drvn_tx_ctr_inc
//              Increment key derivation counter
//
//  Parameters:
//      hdcp - hdcp handle
//
//  Returns:
//      true=OK / false=fail
//------------------------------------------------------------------------------
bool hdcp2_key_drvn_tx_ctr_inc(struct cix_hdcp *hdcp)
{
	hdcp2_hw_tx_aes_ctr_inc(hdcp);

	return true;
}

//------------------------------------------------------------------------------
//  Function:   hdcp2_key_drvn_tx_kd_calc
//              generate dkey0
//              generate dkey1
//              populate kd with dkey0 || dkey1
//
//  Parameters:
//      hdcp - hdcp handle
//
//  Returns:
//      true=OK / false=fail
//------------------------------------------------------------------------------
bool hdcp2_key_drvn_tx_kd_calc(struct cix_hdcp *hdcp, void *kdata)
{
	hdcp2_kd_t *tx_data = kdata;

	//  generate dkey0
	hdcp2_hw_tx_km_write(
		hdcp, tx_data->km); //  write km  (triggers AES execution)
	hdcp2_hw_tx_dkey_read(hdcp, tx_data->dkey0);

	//  generate dkey1
	hdcp2_key_drvn_tx_ctr_inc(hdcp); //  increment aes counter
	hdcp2_hw_tx_km_write(
		hdcp, tx_data->km); //  write km  (triggers AES execution)
	hdcp2_hw_tx_dkey_read(hdcp, tx_data->dkey1);

	//  assemble kd
	memset(tx_data->kd, 0, HDCP2_KD_SZ); //  clear kd
	memcpy(&tx_data->kd[0], tx_data->dkey0, HDCP2_DKEY_SZ);
	memcpy(&tx_data->kd[16], tx_data->dkey1, HDCP2_DKEY_SZ);

	return true;
}

bool hdcp2_key_drvn_tx_kd_generate(struct cix_hdcp *hdcp, void *kdata)
{
	//------------------------------------------------------------------------------
	//  Perform key derivation to generate 256bit kd.
	//------------------------------------------------------------------------------
	hdcp2_key_drvn_tx_init(hdcp, kdata); //  initialize key derivation unit
	hdcp2_key_drvn_tx_reset(hdcp); //  reset the the engine
	hdcp2_key_drvn_tx_src_select(
		hdcp,
		aes_tx_sel_key_derivation); //  select source for key derivation
	hdcp2_key_drvn_tx_load(
		hdcp, kdata); //  load up values required for kd calculation
	hdcp2_key_drvn_tx_kd_calc(hdcp, kdata); //  generate kd

	return true;
}

//------------------------------------------------------------------------------
//  Function:   hdcp2_key_drvn_tx_dkey2_calc
//
//  This function generates dkey2.
//  When this function is called, rn should be a random number.
//  When dkey0 and dkey1 are calculated, rn is initialized to zero.
//
//  Parameters:
//      hdcp - hdcp handle
//
//  Returns:
//      true=OK / false=fail
//------------------------------------------------------------------------------
bool hdcp2_key_drvn_tx_dkey2_calc(struct cix_hdcp *hdcp, void *kdata)
{
	hdcp2_dkey2_t *tx_data = kdata;

	//  update rn, increment aes counter
	hdcp2_hw_tx_rn_write(hdcp, tx_data->rn); //  write rn (currently random)
	hdcp2_key_drvn_tx_ctr_inc(hdcp); //  bump the counter by one

	//  generate dkey2
	hdcp2_hw_tx_km_write(
		hdcp, tx_data->km); //  write km  (triggers AES execution)
	hdcp2_hw_tx_dkey_read(
		hdcp,
		tx_data->dkey2); //  copy hardware registers into HDCP 2.x transmitter data structure

	return true;
}
