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
//#include <stdint.h>
//#include <stdbool.h>
#include <linux/types.h>
#include <linux/stddef.h>
#include <linux/string.h>
#include <linux/io.h>

#include "cix_hdcp.h"
#include "dptx_reg.h"
#include "dptx_aux_reg.h"
#include "hdcp2_hw_tx.h"

//------------------------------------------------------------------------------
//  Function:   reg_read_offset
//  register read with offset
//
//  Parameters:
//  base   - 32-bit device base address
//  offset - 32-bit register offset
//
//  Returns:
//  32-bit data read
//------------------------------------------------------------------------------
static inline uint32_t reg_read_offset(void *base, uint32_t offset)
{
	return readl(base + offset);
}

//------------------------------------------------------------------------------
//  Function:   reg_write_offset
//  register write with offset
//
//  Parameters:
//  base   - 32-bit device base address
//  offset - 32-bit register offset
//  data   - 32-bit data write
//
//  Returns:
//  none
//------------------------------------------------------------------------------
static inline void reg_write_offset(void *base, uint32_t offset, uint32_t data)
{
	writel(data, base + offset);
}

//------------------------------------------------------------------------------
//  Function:   hdcp2_hw_tx_enable_write
//  Enable / disable HDCP 2.x
//
//  Registers:
//  0x400   RW  HDCP_ENABLE
//
//  Parameters:
//  hdnl   - hdcp device handle
//  enable - true=enable / false=disable
//
//  Returns:
//  register value
//------------------------------------------------------------------------------
uint32_t hdcp2_hw_tx_interrupt_state_read(struct cix_hdcp *hdcp)
{
	return reg_read_offset(hdcp->base_addr, TR_DPTX_INTERRUPT_STATE);
}

//------------------------------------------------------------------------------
//  Function:   hdcp2_hw_tx_enable_write
//  Enable / disable HDCP 2.x
//
//  Registers:
//  0x400   RW  HDCP_ENABLE
//
//  Parameters:
//  hdnl   - hdcp device handle
//  enable - true=enable / false=disable
//
//  Returns:
//  None
//------------------------------------------------------------------------------
void hdcp2_hw_tx_enable_write(struct cix_hdcp *hdcp, bool enable)
{
	reg_write_offset(hdcp->base_addr, TR_DPTX_HDCP_ENABLE,
			 (enable ? 1ul : 0ul));
}

//------------------------------------------------------------------------------
//  Function:   hdcp2_hw_tx_enable_read
//  Read HDCP state
//
//  Registers:
//  0x400   RW  HDCP_ENABLE
//
//  Parameters:
//  hdnl - hdcp device handle
//
//  Returns:
//  Register read value
//------------------------------------------------------------------------------
uint32_t hdcp2_hw_tx_enable_read(struct cix_hdcp *hdcp)
{
	return reg_read_offset(hdcp->base_addr, TR_DPTX_HDCP_ENABLE);
}

//------------------------------------------------------------------------------
//  Function:   hdcp2_hw_tx_mode_write
//  Set specified HDCP mode.
//
//  Registers:
//  0x0404  RW  TR_DPTX_HDCP_MODE
//
//  Parameters:
//  hdnl - hdcp device handle
//  val  - 1 = hdcp 1.x / 2 = hdcp 2.x
//
//  Returns:
//  None
//------------------------------------------------------------------------------
void hdcp2_hw_tx_mode_write(struct cix_hdcp *hdcp, uint32_t val)
{
	reg_write_offset(hdcp->base_addr, TR_DPTX_HDCP_MODE, val);
}

//------------------------------------------------------------------------------
//  Function:   hdcp2_hw_tx_mode_read
//  Read HDCP mode
//
//  Registers:
//  0x0404  RW  TR_DPTX_HDCP_MODE
//
//  Parameters:
//  hdnl - hdcp device handle
//
//  Returns:
//  Register read value
//------------------------------------------------------------------------------
uint32_t hdcp2_hw_tx_mode_read(struct cix_hdcp *hdcp)
{
	return reg_read_offset(hdcp->base_addr, TR_DPTX_HDCP_MODE);
}

//------------------------------------------------------------------------------
//  Function:   hdcp2_hw_tx_hardware_keys
//  Enable / disable hardware keys
//
//  Registers:
//  0x42C   RW  TR_DPTX_HDCP_CIPHER_CONTROL
//
//  Parameters:
//  hdnl   - hdcp device handle
//  enable - true=enable / false=disable
//
//  Returns:
//  None
//------------------------------------------------------------------------------
void hdcp2_hw_tx_hardware_keys(struct cix_hdcp *hdcp, bool enable)
{
	reg_write_offset(hdcp->base_addr, TR_DPTX_HDCP_CIPHER_CONTROL,
			 (enable ? 1ul : 0ul));
}

//------------------------------------------------------------------------------
//  Function:   hdcp2_hw_tx_km_write
//  Write pseudo random number km
//
//  Registers:
//  TR_DPTX_HDCP_KS_31_0                WO  Km bits 31:0
//  TR_DPTX_HDCP_KS_63_32               WO  Km bits 63:32
//  TR_DPTX_HDCP_KM_31_0                WO  Km bits 95:64
//  TR_DPTX_HDCP_KM_63_32               WO  Km bits 127:96
//
//  Parameters:
//  hdnl - hdcp device handle
//  km -  pointer to 16-byte random km value
//
//  Note:   ks and km are both fed to seed the AES engine.
//          This means that the write is to the same registers.
//
//  Returns:
//  true=OK / false=fail
//------------------------------------------------------------------------------
bool hdcp2_hw_tx_km_write(struct cix_hdcp *hdcp, uint8_t *km)
{
	uint32_t wr_val;

	wr_val = km[3] << 24 | km[2] << 16 | km[1] << 8 | km[0];
	reg_write_offset(hdcp->base_addr, TR_DPTX_HDCP_KS_31_0,
			 wr_val); //  write bits 31:0

	wr_val = km[7] << 24 | km[6] << 16 | km[5] << 8 | km[4];
	reg_write_offset(hdcp->base_addr, TR_DPTX_HDCP_KS_63_32,
			 wr_val); //  write bits 63:32

	wr_val = km[11] << 24 | km[10] << 16 | km[9] << 8 | km[8];
	reg_write_offset(hdcp->base_addr, TR_DPTX_HDCP_KM_31_0,
			 wr_val); //  write bits 95:64

	wr_val = km[15] << 24 | km[14] << 16 | km[13] << 8 | km[12];
	reg_write_offset(hdcp->base_addr, TR_DPTX_HDCP_KM_63_32,
			 wr_val); //  write bits 127:96

	udelay(50);

	return true;
}

//------------------------------------------------------------------------------
//  Function:   hdcp2_hw_tx_km_read
//  Read pseudo random number km
//  The km registers are write only. This function always fails and displays
//  a critical message that km cannot be read.
//
//  Registers:
//  TR_DPTX_HDCP_KS_0                   WO  Km bits 31:0
//  TR_DPTX_HDCP_KS_1                   WO  Km bits 63:32
//  TR_DPTX_HDCP_KM_31_0                WO  Km bits 95:64
//  TR_DPTX_HDCP_KM_63_32               WO  Km bits 127:96
//
//  Parameters:
//  hdnl - hdcp device handle
//  km -  pointer to 16-byte random km value
//
//  Returns:
//  true=OK / false=fail
//------------------------------------------------------------------------------
bool hdcp2_hw_tx_km_read(struct cix_hdcp *hdcp, uint8_t *km)
{
	return false;
}

//------------------------------------------------------------------------------
//  Function:   hdcp2_hw_tx_ks_write
//  Write session key ks
//
//  Parameters:
//  hdnl - hdcp device handle
//  ks   -  pointer to 16-byte session key value
//
//  Note:   ks and km are both fed to seed the AES engine.
//          This means that the write is to the same registers.
//
//  Returns:
//  true=OK / false=fail
//------------------------------------------------------------------------------
bool hdcp2_hw_tx_ks_write(struct cix_hdcp *hdcp, uint8_t *ks)
{
	return hdcp2_hw_tx_km_write(hdcp, ks);
}

//------------------------------------------------------------------------------
//  Function:   hdcp2_hw_tx_ks_read
//  Read session key
//
//  The ks registers are write only. This function always fails and displays
//  a critical message that ks cannot be read.
//
//  Parameters:
//  hdnl - hdcp device handle
//  ks   -  pointer to 16-byte session key value
//
//  Returns:
//  true=OK / false=fail
//------------------------------------------------------------------------------
bool hdcp2_hw_tx_ks_read(struct cix_hdcp *hdcp, uint8_t *km)
{
	return false;
}

//------------------------------------------------------------------------------
//  Function:   hdcp2_hw_tx_rtx_write
//  Write pseudo random number rtx
//
//  Note: rtx and riv are the seed for the AES128 engine. They are used for
//  different modes of AES operation. When the AES engine is in CTR mode,
//  then the seed is loaded with rtx. When the AES engine is in stream mode,
//  then the seed is loaded with riv.
//
//  Registers:
//  0x0418  RW  HDCP_AN_31_0
//  0x041C  RW  HDCP_AN_63_32
//
//  Parameters:
//  hdnl - hdcp device handle
//  rtx  - pointer to 8-byte random rtx value
//
//  Returns:
//  true=OK / false=fail
//------------------------------------------------------------------------------
bool hdcp2_hw_tx_rtx_write(struct cix_hdcp *hdcp, uint8_t *rtx)
{
	uint32_t wr_val;

	wr_val = rtx[3] << 24 | rtx[2] << 16 | rtx[1] << 8 | rtx[0];
	reg_write_offset(hdcp->base_addr, TR_DPTX_HDCP_AN_31_0, wr_val);

	wr_val = rtx[7] << 24 | rtx[6] << 16 | rtx[5] << 8 | rtx[4];
	reg_write_offset(hdcp->base_addr, TR_DPTX_HDCP_AN_63_32, wr_val);

	return true;
}

//------------------------------------------------------------------------------
//  Function:   hdcp2_hw_tx_rtx_read
//  Read pseudo random number rtx
//
//  Note: rtx and riv are the seed for the AES128 engine. They are used for
//  different modes of AES operation. When the AES engine is in CTR mode,
//  then the seed is loaded with rtx. When the AES engine is in stream mode,
//  then the seed is loaded with riv.
//
//  Registers:
//  0x0418  RW  HDCP_AN_31_0 / HDCP_RTX_31_0
//  0x041C  RW  HDCP_AN_63_32 / HDCP_RTX_63_32
//
//  Parameters:
//  hdnl  - hdcp device handle
//  rtx   - pointer to 8-byte random rtx value
//
//  Returns:
//  true=OK / false=fail
//------------------------------------------------------------------------------
bool hdcp2_hw_tx_rtx_read(struct cix_hdcp *hdcp, uint8_t *rtx)
{
	uint32_t rd_val;

	rd_val = reg_read_offset(hdcp->base_addr,
				 TR_DPTX_HDCP_AN_31_0); //  read lower
	rtx[3] = (rd_val >> 24) & 0xffU;
	rtx[2] = (rd_val >> 16) & 0xffU;
	rtx[1] = (rd_val >> 8) & 0xffU;
	rtx[0] = (rd_val)&0xffU;

	rd_val = reg_read_offset(hdcp->base_addr,
				 TR_DPTX_HDCP_AN_63_32); //  read upper
	rtx[7] = (rd_val >> 24) & 0xffU;
	rtx[6] = (rd_val >> 16) & 0xffU;
	rtx[5] = (rd_val >> 8) & 0xffU;
	rtx[4] = (rd_val)&0xffU;

	return true;
}

//------------------------------------------------------------------------------
//  Function:   hdcp2_hw_tx_riv_write
//  Write pseudo random number riv
//
//  Note: rtx and riv are the seed for the AES128 engine. They are used for
//  different modes of AES operation. When the AES engine is in CTR mode,
//  then the seed is loaded with rtx. When the AES engine is in stream mode,
//  then the seed is loaded with riv.
//
//  Registers:
//  0x0418  RW  HDCP_AN_31_0
//  0x041C  RW  HDCP_AN_63_32
//
//  Parameters:
//  hdnl - hdcp device handle
//  riv  - pointer to 8-byte random riv value
//
//  Returns:
//  true=OK / false=fail
//------------------------------------------------------------------------------
bool hdcp2_hw_tx_riv_write(struct cix_hdcp *hdcp, uint8_t *riv)
{
	uint32_t wr_val;

	wr_val = riv[3] << 24 | riv[2] << 16 | riv[1] << 8 | riv[0];
	reg_write_offset(hdcp->base_addr, TR_DPTX_HDCP_AN_31_0, wr_val);

	wr_val = riv[7] << 24 | riv[6] << 16 | riv[5] << 8 | riv[4];
	reg_write_offset(hdcp->base_addr, TR_DPTX_HDCP_AN_63_32, wr_val);

	return true;
}

//------------------------------------------------------------------------------
//  Function:   hdcp2_hw_tx_riv_read
//  Read pseudo random number riv
//
//  Note: rtx and riv are the seed for the AES128 engine. They are used for
//  different modes of AES operation. When the AES engine is in CTR mode,
//  then the seed is loaded with rtx. When the AES engine is in stream mode,
//  then the seed is loaded with riv.
//
//  Registers:
//  0x0418  RW  HDCP_AN_31_0 / HDCP_RTX_31_0
//  0x041C  RW  HDCP_AN_63_32 / HDCP_RTX_63_32
//
//  Parameters:
//  hdnl  - hdcp device handle
//  riv   - pointer to 8-byte random riv value
//
//  Returns:
//  true=OK / false=fail
//------------------------------------------------------------------------------
bool hdcp2_hw_tx_riv_read(struct cix_hdcp *hdcp, uint8_t *riv)
{
	uint32_t rd_val;

	rd_val = reg_read_offset(hdcp->base_addr,
				 TR_DPTX_HDCP_AN_31_0); //  read lower
	riv[3] = (rd_val >> 24) & 0xffU;
	riv[2] = (rd_val >> 16) & 0xffU;
	riv[1] = (rd_val >> 8) & 0xffU;
	riv[0] = (rd_val)&0xffU;

	rd_val = reg_read_offset(hdcp->base_addr,
				 TR_DPTX_HDCP_AN_63_32); //  read upper
	riv[7] = (rd_val >> 24) & 0xffU;
	riv[6] = (rd_val >> 16) & 0xffU;
	riv[5] = (rd_val >> 8) & 0xffU;
	riv[4] = (rd_val)&0xffU;

	return true;
}

//------------------------------------------------------------------------------
//  Function:   hdcp2_hw_tx_rrx_write
//  Write pseudo random number rrx
//
//  Registers:
//  0x0430  RW  TR_DPTX_HDCP_BKSV_31_0 / HDCP_RRX_31_0
//  0x0434  RW  TR_DPTX_HDCP_BKSV_63_32 / HDCP_RRX_63_32
//
//  Parameters:
//  hdnl - hdcp device handle
//  rrx  - pointer to 8-byte random rrx value
//
//  Returns:
//  true=OK / false=fail
//------------------------------------------------------------------------------
bool hdcp2_hw_tx_rrx_write(struct cix_hdcp *hdcp, uint8_t *rrx)
{
	uint32_t wr_val;

	wr_val = rrx[3] << 24 | rrx[2] << 16 | rrx[1] << 8 | rrx[0];
	reg_write_offset(hdcp->base_addr, TR_DPTX_HDCP_BKSV_31_0, wr_val);

	wr_val = rrx[7] << 24 | rrx[6] << 16 | rrx[5] << 8 | rrx[4];
	reg_write_offset(hdcp->base_addr, TR_DPTX_HDCP_BKSV_63_32, wr_val);

	return true;
}

//------------------------------------------------------------------------------
//  Function:   hdcp2_hw_tx_rrx_read
//  Read pseudo random number rrx
//
//  Registers:
//  0x0430  RW  TR_DPTX_HDCP_BKSV_31_0 / HDCP_RRX_31_0
//  0x0434  RW  TR_DPTX_HDCP_BKSV_63_32 / HDCP_RRX_63_32
//
//  Parameters:
//  hdnl - hdcp device handle
//  rrx  - pointer to 8-byte random rrx value
//
//  Returns:
//  true=OK / false=fail
//------------------------------------------------------------------------------
bool hdcp2_hw_tx_rrx_read(struct cix_hdcp *hdcp, uint8_t *rrx)
{
	uint32_t rd_val;

	rd_val = reg_read_offset(hdcp->base_addr,
				 TR_DPTX_HDCP_BKSV_31_0); //  read lower
	rrx[3] = (rd_val >> 24) & 0xffU;
	rrx[2] = (rd_val >> 16) & 0xffU;
	rrx[1] = (rd_val >> 8) & 0xffU;
	rrx[0] = (rd_val)&0xffU;

	rd_val = reg_read_offset(hdcp->base_addr,
				 TR_DPTX_HDCP_BKSV_63_32); //  read upper
	rrx[7] = (rd_val >> 24) & 0xffU;
	rrx[6] = (rd_val >> 16) & 0xffU;
	rrx[5] = (rd_val >> 8) & 0xffU;
	rrx[4] = (rd_val)&0xffU;

	return true;
}

//------------------------------------------------------------------------------
//  Function:   hdcp2_hw_tx_dkey_read
//  Read dkey from AES engine.
//
//  Parameters:
//  hdnl - hdcp device handle
//  dkey - pointer to 8-byte dkey buffer
//
//  Returns:
//  true=OK / false=fail
//------------------------------------------------------------------------------
bool hdcp2_hw_tx_dkey_read(struct cix_hdcp *hdcp, uint8_t *dkey)
{
	uint32_t rd_val;

	rd_val = reg_read_offset(hdcp->base_addr,
				 TR_DPTX_HDCP_M0_31_0); //  bits 31:0
	dkey[3] = (rd_val >> 24) & 0xffU;
	dkey[2] = (rd_val >> 16) & 0xffU;
	dkey[1] = (rd_val >> 8) & 0xffU;
	dkey[0] = (rd_val)&0xffU;

	rd_val = reg_read_offset(hdcp->base_addr,
				 TR_DPTX_HDCP_M0_63_32); //  bits 63:32
	dkey[7] = (rd_val >> 24) & 0xffU;
	dkey[6] = (rd_val >> 16) & 0xffU;
	dkey[5] = (rd_val >> 8) & 0xffU;
	dkey[4] = (rd_val)&0xffU;

	rd_val = reg_read_offset(hdcp->base_addr,
				 TR_DPTX_HDCP_AKSV_31_0); //  bits 95:64
	dkey[11] = (rd_val >> 24) & 0xffU;
	dkey[10] = (rd_val >> 16) & 0xffU;
	dkey[9] = (rd_val >> 8) & 0xffU;
	dkey[8] = (rd_val)&0xffU;

	rd_val = reg_read_offset(hdcp->base_addr,
				 TR_DPTX_HDCP_AKSV_63_32); //  bits 127:96
	dkey[15] = (rd_val >> 24) & 0xffU;
	dkey[14] = (rd_val >> 16) & 0xffU;
	dkey[13] = (rd_val >> 8) & 0xffU;
	dkey[12] = (rd_val)&0xffU;

	return true;
}

//------------------------------------------------------------------------------
//  Function:   hdcp2_hw_tx_lc128_write
//              Write system constant lc128
//
//  Registers:
//  0x0440U WO  TR_DPTX_HDCP_LC128_31_0     Global constant, lc128 bits 31:0
//  0x0444U WO  TR_DPTX_HDCP_LC128_63_32    Global constant, lc128 bits 63:32
//  0x0448U WO  TR_DPTX_HDCP_LC128_95_64    Global constant, lc128 bits 95:64
//  0x044CU WO  TR_DPTX_HDCP_LC128_127_96   Global constant, lc128 bits 127:64
//
//  Parameters:
//  hdnl  - hdcp device handle
//  lc128 - pointer to 8-byte lc128 value
//
//  Returns:
//  true=OK / false=fail
//
//------------------------------------------------------------------------------
bool hdcp2_hw_tx_lc128_write(struct cix_hdcp *hdcp, const uint8_t *lc128)
{
	uint32_t wr_val;

	wr_val = lc128[3] << 24 | lc128[2] << 16 | lc128[1] << 8 | lc128[0];
	reg_write_offset(hdcp->base_addr, TR_DPTX_HDCP_LC128_31_0,
			 wr_val); //  write bits 31:0

	wr_val = lc128[7] << 24 | lc128[6] << 16 | lc128[5] << 8 | lc128[4];
	reg_write_offset(hdcp->base_addr, TR_DPTX_HDCP_LC128_63_32,
			 wr_val); //  write bits 63:32

	wr_val = lc128[11] << 24 | lc128[10] << 16 | lc128[9] << 8 | lc128[8];
	reg_write_offset(hdcp->base_addr, TR_DPTX_HDCP_LC128_95_64,
			 wr_val); //  write bits 95:64

	wr_val = lc128[15] << 24 | lc128[14] << 16 | lc128[13] << 8 | lc128[12];
	reg_write_offset(hdcp->base_addr, TR_DPTX_HDCP_LC128_127_96,
			 wr_val); //  write bits 127:96

	return true;
}

//------------------------------------------------------------------------------
//  Function:   hdcp2_hw_tx_repeater_write
//  Enable / Disable repeater
//
//  Registers:
//  0x0450  RW  HDCP_REPEATER
//
//  Parameters:
//  hdnl   - hdcp device handle
//  enable - enable / disable
//
//  Returns:
//  None
//------------------------------------------------------------------------------
void hdcp2_hw_tx_repeater_write(struct cix_hdcp *hdcp, bool enable)
{
	reg_write_offset(hdcp->base_addr, TR_DPTX_HDCP_REPEATER,
			 (enable) ? 1ul : 0ul);
}

//------------------------------------------------------------------------------
//  Function:   hdcp2_hw_tx_repeater_read
//  Read HDCP 2.x repeater status
//
//  Registers:
//  0x0450  RW  HDCP_REPEATER
//
//  Parameters:
//  hdnl   - hdcp device handle
//  enable - enable / disable
//
//  Returns:
//  None
//------------------------------------------------------------------------------
uint32_t hdcp2_hw_tx_repeater_read(struct cix_hdcp *hdcp, bool enable)
{
	return reg_read_offset(hdcp->base_addr, TR_DPTX_HDCP_REPEATER);
}

//------------------------------------------------------------------------------
//  Function:   hdcp2_hw_tx_stream_cipher_write
//              Enable / Disable stream cipher
//
//  Registers:
//  0x0454  RW  HDCP_STREAM_CIPHER_ENABLE
//
//  Parameters:
//  hdnl   - hdcp device handle
//  enable - enable / disable
//
//  Returns:
//  None
//------------------------------------------------------------------------------
void hdcp2_hw_tx_stream_cipher_write(struct cix_hdcp *hdcp, bool enable)
{
	reg_write_offset(hdcp->base_addr, TR_DPTX_HDCP_STREAM_CIPHER_ENABLE,
			 (enable) ? 1ul : 0ul);
}

//------------------------------------------------------------------------------
//  Function:   hdcp2_hw_tx_stream_cipher_read
//  Read / return stream cipher
//
//  Registers:
//  0x0454  RW  HDCP_STREAM_CIPHER_ENABLE
//
//  Parameters:
//      hdnl - hdcp device handle
//
//  Returns:
//      Stream cipher status
//
//  TR_DPTX_HDCP2_STREAM_CYPHER_ENABLE WR  0       Stream cipher enable
//------------------------------------------------------------------------------
uint32_t hdcp2_hw_tx_stream_cipher_read(struct cix_hdcp *hdcp)
{
	return reg_read_offset(hdcp->base_addr,
			       TR_DPTX_HDCP_STREAM_CIPHER_ENABLE);
}

//------------------------------------------------------------------------------
//  Function:   hdcp2_hw_tx_aes_input_write
//  Select AES input source
//
//  Registers:
//  0x0460  RW  HDCP_AES_INPUT_SELECT
//
//  Parameters:
//  hdnl     - hdcp device handle
//  input_id - AES channel to select
//
//  Returns:
//  None
//------------------------------------------------------------------------------
void hdcp2_hw_tx_aes_input_write(struct cix_hdcp *hdcp, uint32_t input_id)
{
	reg_write_offset(hdcp->base_addr, TR_DPTX_HDCP_AES_INPUT_SELECT,
			 (input_id & 0x07U));
}

//------------------------------------------------------------------------------
//  Function:   hdcp2_hw_tx_aes_input_read
//  Read / Return AES input select
//
//  Registers:
//  0x0460  RW  HDCP_AES_INPUT_SELECT
//
//  Parameters:
//  hdnl - hdcp device handle
//
//  Returns:
//  Selected AES channel
//------------------------------------------------------------------------------
uint32_t hdcp2_hw_tx_aes_input_read(struct cix_hdcp *hdcp)
{
	return reg_read_offset(hdcp->base_addr, TR_DPTX_HDCP_AES_INPUT_SELECT) &
	       0x07u;
}

//------------------------------------------------------------------------------
//  Function:   hdcp2_hw_tx_aes_ctr_disable_write
//  Enable / Disable AES ctr
//
//  0x0464  RW  HDCP_AES_COUNTER_DISABLE
//
//  Parameters:
//  hdnl   - hdcp device handle
//  enable - enable / disable flag
//
//  Returns:
//  None
//------------------------------------------------------------------------------
void hdcp2_hw_tx_aes_ctr_disable_write(struct cix_hdcp *hdcp, bool enable)
{
	reg_write_offset(hdcp->base_addr, TR_DPTX_HDCP_AES_COUNTER_DISABLE,
			 (enable) ? 1ul : 0ul);
}

//------------------------------------------------------------------------------
//  Function:   hdcp2_hw_tx_aes_ctr_disable_read
//  Read AES ctr status
//
//  0x0464  RW  HDCP_AES_COUNTER_DISABLE
//
//  Parameters:
//  hdnl - hdcp device handle
//
//  Returns:
//  enable / disable flag
//------------------------------------------------------------------------------
uint32_t hdcp2_hw_tx_aes_ctr_disable_read(struct cix_hdcp *hdcp)
{
	return reg_read_offset(hdcp->base_addr,
			       TR_DPTX_HDCP_AES_COUNTER_DISABLE);
}

//------------------------------------------------------------------------------
//  Function:   hdcp2_hw_tx_aes_ctr_inc
//  Bump AES ctr
//
//  0x0468  WO  HDCP_AES_COUNTER_ADVANCE
//
//  Parameters:
//  hdnl - hdcp device handle
//
//  Returns:
//  None
//------------------------------------------------------------------------------
void hdcp2_hw_tx_aes_ctr_inc(struct cix_hdcp *hdcp)
{
	reg_write_offset(hdcp->base_addr, TR_DPTX_HDCP_AES_COUNTER_ADVANCE, 1);
}

//------------------------------------------------------------------------------
//  Function:   hdcp2_hw_tx_encryption_ctl_write
//  Write encryption control
//
//  0x046C  RW  TR_DPTX_HDCP_ECF_31_0   HDCP Encryption Control Field, bits 31:0
//  0x0470  RW  TR_DPTX_HDCP_ECF_63_32  HDCP Encryption Control Field, bits 63:32
//
//  Parameters:
//  hdnl - hdcp device handle
//  ctl  - pointer to 8-byte ctl value
//
//  Returns:
//  true=OK / false=fail
//------------------------------------------------------------------------------
bool hdcp2_hw_tx_encryption_ctl_write(struct cix_hdcp *hdcp, uint8_t *ctl)
{
	uint32_t wr_val;

	wr_val = ctl[3] << 24 | ctl[2] << 16 | ctl[1] << 8 | ctl[0];
	reg_write_offset(hdcp->base_addr, TR_DPTX_HDCP_ECF_31_0, wr_val);

	wr_val = ctl[7] << 24 | ctl[6] << 16 | ctl[5] << 8 | ctl[4];
	reg_write_offset(hdcp->base_addr, TR_DPTX_HDCP_ECF_63_32, wr_val);

	return true;
}

//------------------------------------------------------------------------------
//  Function:   hdcp2_hw_tx_encryption_ctl_read
//  Read encryption ctl value
//
//  0x046C  RW  TR_DPTX_HDCP_ECF_31_0   HDCP Encryption Control Field, bits 31:0
//  0x0470  RW  TR_DPTX_HDCP_ECF_63_32  HDCP Encryption Control Field, bits 63:32
//
//  Parameters:
//  hdnl - hdcp device handle
//  ctl  - pointer to 8-byte encryption ctl value
//
//  Returns:
//  true=OK / false=fail
//------------------------------------------------------------------------------
bool hdcp2_hw_tx_encryption_ctl_read(struct cix_hdcp *hdcp, uint8_t *ctl)
{
	uint32_t rd_val;

	rd_val = reg_read_offset(hdcp->base_addr,
				 TR_DPTX_HDCP_ECF_31_0); //  read lower
	ctl[3] = (rd_val >> 24) & 0xffU;
	ctl[2] = (rd_val >> 16) & 0xffU;
	ctl[1] = (rd_val >> 8) & 0xffU;
	ctl[0] = (rd_val)&0xffU;

	rd_val = reg_read_offset(hdcp->base_addr,
				 TR_DPTX_HDCP_ECF_63_32); //  read upper
	ctl[7] = (rd_val >> 24) & 0xffU;
	ctl[6] = (rd_val >> 16) & 0xffU;
	ctl[5] = (rd_val >> 8) & 0xffU;
	ctl[4] = (rd_val)&0xffU;

	return true;
}

//------------------------------------------------------------------------------
//  Function:   hdcp2_hw_tx_aes_ctr_reset_write
//  AES ctr reset
//
//  0x0474  WO  HDCP_AES_COUNTER_RESET
//
//  Parameters:
//  hdnl - hdcp device handle
//
//  Returns:
//  None
//------------------------------------------------------------------------------
void hdcp2_hw_tx_aes_ctr_reset(struct cix_hdcp *hdcp)
{
	reg_write_offset(hdcp->base_addr, TR_DPTX_HDCP_AES_COUNTER_RESET, 1u);
}

//------------------------------------------------------------------------------
//  Function:   hdcp2_hw_tx_rn_clear
//              Write Rn
//
//  Registers:
//  0x0478  RW  HDCP_RN_31_0    Rn bits 31:0
//  0x047C  RW  HDCP_RN_63_32   Rn bits 63:32
//
//  Parameters:
//  hdnl - hdcp device handle
//
//  Returns:
//  true=OK / false=fail
//------------------------------------------------------------------------------
bool hdcp2_hw_tx_rn_clear(struct cix_hdcp *hdcp)
{
	reg_write_offset(hdcp->base_addr, TR_DPTX_HDCP_RN_31_0, 0ul);
	reg_write_offset(hdcp->base_addr, TR_DPTX_HDCP_RN_63_32, 0ul);

	return true;
}

//------------------------------------------------------------------------------
//  Function:   hdcp2_hw_tx_rn_write
//              Write Rn
//
//  Registers:
//  0x0478  RW  HDCP_RN_31_0    Rn bits 31:0
//  0x047C  RW  HDCP_RN_63_32   Rn bits 63:32
//
//  Parameters:
//  hdnl - hdcp device handle
//  rn   - pointer to 8-byte rn value
//
//  Returns:
//  true=OK / false=fail
//------------------------------------------------------------------------------
bool hdcp2_hw_tx_rn_write(struct cix_hdcp *hdcp, uint8_t *rn)
{
	uint32_t wr_val;

	wr_val = rn[3] << 24 | rn[2] << 16 | rn[1] << 8 | rn[0];
	reg_write_offset(hdcp->base_addr, TR_DPTX_HDCP_RN_31_0, wr_val);

	wr_val = rn[7] << 24 | rn[6] << 16 | rn[5] << 8 | rn[4];
	reg_write_offset(hdcp->base_addr, TR_DPTX_HDCP_RN_63_32, wr_val);

	return true;
}

//------------------------------------------------------------------------------
//  Function:   hdcp2_hw_tx_rn_read
//  Read Rn
//
//  Registers:
//  0x0478  RW  HDCP_RN_31_0    Rn bits 31:0
//  0x047C  RW  HDCP_RN_63_32   Rn bits 63:32
//
//  Parameters:
//  hdnl - hdcp device handle
//  ctl  - pointer to 8-byte ctl value
//
//  Returns:
//  true=OK / false=fail
//------------------------------------------------------------------------------
bool hdcp2_hw_tx_rn_read(struct cix_hdcp *hdcp, uint8_t *ctl)
{
	uint32_t rd_val;

	rd_val = reg_read_offset(hdcp->base_addr,
				 TR_DPTX_HDCP_RN_31_0); //  read lower
	ctl[3] = (rd_val >> 24) & 0xffU;
	ctl[2] = (rd_val >> 16) & 0xffU;
	ctl[1] = (rd_val >> 8) & 0xffU;
	ctl[0] = (rd_val)&0xffU;

	rd_val = reg_read_offset(hdcp->base_addr,
				 TR_DPTX_HDCP_RN_63_32); //  read upper
	ctl[7] = (rd_val >> 24) & 0xffU;
	ctl[6] = (rd_val >> 16) & 0xffU;
	ctl[5] = (rd_val >> 8) & 0xffU;
	ctl[4] = (rd_val)&0xffU;

	return true;
}
