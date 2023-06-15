/*
 * Allwinner SoCs display driver.
 *
 * Copyright (C) 2016 Allwinner.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#ifndef DDC_IF_H
#define DDC_IF_H

#include "EP952api.h"

//==================================================================================================
//
// Protected Data Member
//
extern SMBUS_STATUS status;
extern unsigned char DDC_Data[128];	// The DDC Buffer

// EDID status error code
typedef enum {
	// Master
	EDID_STATUS_Success = 0x00,
	EDID_STATUS_Pending,
	EDID_STATUS_NoAct = 0x02,
	EDID_STATUS_TimeOut,
	EDID_STATUS_ArbitrationLoss = 0x04,
	EDID_STATUS_ExtensionOverflow,
	EDID_STATUS_ChecksumError
} EDID_STATUS;

//--------------------------------------------------------------------------------------------------
//
// Downstream HDCP Control Interface
//

extern unsigned char Downstream_Rx_read_BKSV(unsigned char *pBKSV);
extern unsigned char Downstream_Rx_BCAPS(void);
extern void Downstream_Rx_write_AINFO(char ainfo);
extern void Downstream_Rx_write_AN(unsigned char *pAN);
extern void Downstream_Rx_write_AKSV(unsigned char *pAKSV);
extern unsigned char Downstream_Rx_read_RI(unsigned char *pRI);
extern void Downstream_Rx_read_BSTATUS(unsigned char *pBSTATUS);
extern void Downstream_Rx_read_SHA1_HASH(unsigned char *pSHA);
extern unsigned char Downstream_Rx_read_KSV_FIFO(unsigned char *pBKSV,
						 unsigned char Index,
						 unsigned char DevCount);

//--------------------------------------------------------------------------------------------------
//
// Downstream EDID Control Interface
//

extern EDID_STATUS Downstream_Rx_read_EDID(unsigned char *pEDID);
extern EDID_STATUS Downstream_Rx_read_EDID_sunxi(unsigned char *pEDID);

#endif // DDC_IF_H
