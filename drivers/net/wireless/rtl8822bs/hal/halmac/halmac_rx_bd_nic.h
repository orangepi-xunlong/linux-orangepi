/******************************************************************************
 *
 * Copyright(c) 2016 - 2017 Realtek Corporation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of version 2 of the GNU General Public License as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 ******************************************************************************/

#ifndef _HALMAC_RX_BD_NIC_H_
#define _HALMAC_RX_BD_NIC_H_
#if (HALMAC_8814A_SUPPORT || HALMAC_8822B_SUPPORT || HALMAC_8197F_SUPPORT || HALMAC_8821C_SUPPORT || HALMAC_8198F_SUPPORT)

/*TXBD_DW0*/

#define GET_RX_BD_RXFAIL(__pRxBd)    LE_BITS_TO_4BYTE(__pRxBd+0x00, 31, 1)
#define GET_RX_BD_TOTALRXPKTSIZE(__pRxBd)    LE_BITS_TO_4BYTE(__pRxBd+0x00, 16, 13)
#define GET_RX_BD_RXTAG(__pRxBd)    LE_BITS_TO_4BYTE(__pRxBd+0x00, 16, 13)
#define GET_RX_BD_FS(__pRxBd)    LE_BITS_TO_4BYTE(__pRxBd+0x00, 15, 1)
#define GET_RX_BD_LS(__pRxBd)    LE_BITS_TO_4BYTE(__pRxBd+0x00, 14, 1)
#define GET_RX_BD_RXBUFFSIZE(__pRxBd)    LE_BITS_TO_4BYTE(__pRxBd+0x00, 0, 14)

/*TXBD_DW1*/

#define GET_RX_BD_PHYSICAL_ADDR_LOW(__pRxBd)    LE_BITS_TO_4BYTE(__pRxBd+0x04, 0, 32)

/*TXBD_DW2*/

#define GET_RX_BD_PHYSICAL_ADDR_HIGH(__pRxBd)    LE_BITS_TO_4BYTE(__pRxBd+0x08, 0, 32)

#endif


#endif

