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
//#include <stdint.h>                                                             //  Standard int types
//#include <stdbool.h>                                                            //  Bool
#include <linux/types.h>
#include <linux/stddef.h>

#include "cix_hdcp.h"
#include "cix_hdcp_ioctl_cmd.h"
#include "hdcp2_hw_tx.h"
#include "hdcp2_key_drvn_tx.h"

static const uint8_t test_lc128[] = { 0x93U, 0xceU, 0x5aU, 0x56U, 0xa0U, 0xa1U,
				      0xf4U, 0xf7U, 0x3cU, 0x65U, 0x8aU, 0x1bU,
				      0xd2U, 0xaeU, 0xf0U, 0xf7U };

static const uint8_t product_lc128[] = { 0x93U, 0xceU, 0x5aU, 0x56U, 0xa0U, 0xa1U,
				      0xf4U, 0xf7U, 0x3cU, 0x65U, 0x8aU, 0x1bU,
				      0xd2U, 0xaeU, 0xf0U, 0xf7U };
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
// Local functions
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
//  Function:   hdcp2_cipher_enable
//              Enable HDCP cipher once authentication is complete
//
//  Parameters:
//      sm - state machine pointer
//
//  Returns:
//      None
//------------------------------------------------------------------------------
void hdcp2_cipher_enable(struct cix_hdcp *hdcp, void *kdata)
{
	hdcp2_cipher_data_t *tx_data = kdata;

	hdcp2_hw_tx_enable_write(hdcp, false); //  disable HDCP transmit
	hdcp2_key_drvn_tx_ctr_reset(hdcp); //  reset counter
	hdcp2_key_drvn_tx_src_select(
		hdcp, aes_tx_sel_normal); //  set AES to stream cipher
	hdcp2_hw_tx_enable_write(hdcp, true); //  enable HDCP transmit

	//hdcp2_hw_tx_lc128_write(hdcp, test_lc128);	//  re-load the lc128 value
	hdcp2_hw_tx_lc128_write(hdcp,
				product_lc128); //  re-load the lc128 value

	hdcp2_hw_tx_riv_write(hdcp, tx_data->riv); //  write riv
	hdcp2_hw_tx_ks_write(hdcp, tx_data->ks); //  write ks
	hdcp2_hw_tx_stream_cipher_write(hdcp, true); //  enable stream cipher
}

//------------------------------------------------------------------------------
//  Function:   hdcp2_cipher_disable
//              Disable HDCP cipher once authentication is complete
//
//  Parameters:
//      sm - state machine pointer
//
//  Returns:
//      None
//------------------------------------------------------------------------------
void hdcp2_cipher_disable(struct cix_hdcp *hdcp)
{
	hdcp2_hw_tx_enable_write(
		hdcp,
		false); //Disable HDCP to reset the internal state machines to the default state.
	hdcp2_hw_tx_mode_write(hdcp, 2); //  Select HDCP 2.x mode
	hdcp2_hw_tx_repeater_write(
		hdcp,
		false); //  Enable (0x01) or disable (0x00) repeater mode as required
	hdcp2_hw_tx_stream_cipher_write(hdcp,
					false); //  Disable the stream cipher
	hdcp2_hw_tx_aes_input_write(hdcp,
				    1); //  AES input select set to cipher mode
	hdcp2_hw_tx_aes_ctr_reset(hdcp); //  Issue a reset to the AES counter
	hdcp2_hw_tx_enable_write(hdcp, true); //  Enable HDCP
}

//------------------------------------------------------------------------------
//  Function:   hdcp2_tx_state_init
//              Initialize HDCP transmit authentication state machine.
//
//  Parameters:
//      sm - state machine pointer
//      u_data - user data
//
//  Returns:
//      none
//------------------------------------------------------------------------------
void hdcp2_tx_state_init(struct cix_hdcp *hdcp)
{
	hdcp2_hw_tx_enable_write(
		hdcp,
		false); //Disable HDCP to reset the internal state machines to the default state.
	hdcp2_hw_tx_mode_write(hdcp, 2); //  Select HDCP 2.x mode
	hdcp2_hw_tx_repeater_write(
		hdcp,
		false); //  Enable (0x01) or disable (0x00) repeater mode as required
	hdcp2_hw_tx_stream_cipher_write(hdcp,
					false); //  Disable the stream cipher
	hdcp2_hw_tx_aes_input_write(hdcp,
				    1); //  AES input select set to cipher mode
	hdcp2_hw_tx_aes_ctr_reset(hdcp); //  Issue a reset to the AES counter
	hdcp2_hw_tx_enable_write(hdcp, true); //  Enable HDCP
	//hdcp2_hw_tx_lc128_write(hdcp, test_lc128);	//  Set the lc128 value
	hdcp2_hw_tx_lc128_write(hdcp,
				product_lc128); //  re-load the lc128 value
}
