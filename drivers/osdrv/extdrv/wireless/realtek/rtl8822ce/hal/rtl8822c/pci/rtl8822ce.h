/******************************************************************************
 *
 * Copyright(c) 2015 - 2017 Realtek Corporation.
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
 *****************************************************************************/
#ifndef _RTL8822CE_H_
#define _RTL8822CE_H_

#include <drv_types.h>		/* PADAPTER */

#ifdef CONFIG_PCI_TX_POLLING
#define TX_BD_NUM_8822CE	256
#else
#define TX_BD_NUM_8822CE	128
#endif
#define RX_BD_NUM_8822CE	PCI_MAX_RX_COUNT /* TODO */

#ifdef CONFIG_CONCURRENT_MODE
#define TX_BD_NUM_BEQ_8822CE	(TX_BD_NUM_8822CE << 1)
#else
#define TX_BD_NUM_BEQ_8822CE	TX_BD_NUM_8822CE
#endif /* CONFIG_CONCURRENT_MODE */

#define TX_BD_NUM_8822CE_BCN	2
#define TX_BD_NUM_8822CE_CMD	128

#define RTL8822CE_SEG_NUM       1 /* 0:2 seg, 1: 4 seg, 2: 8 seg */

#ifndef MAX_RECVBUF_SZ
	#ifndef CONFIG_MINIMAL_MEMORY_USAGE
		#define MAX_RECVBUF_SZ (32768)
	#else
		#define MAX_RECVBUF_SZ (4000)
	#endif
#endif /* !MAX_RECVBUF_SZ */

#define TX_BUFFER_SEG_NUM	1 /* 0:2 seg, 1: 4 seg, 2: 8 seg. */

#define MAX_RECVBUF_SZ_8822C	24576	/* 24k */

/* TX BD */
#ifdef CONFIG_64BIT_DMA
#define SET_TXBUFFER_DESC_LEN_WITH_OFFSET(__pTxDesc, __Offset, __Value) \
	SET_BITS_TO_LE_4BYTE(__pTxDesc+(__Offset*16), 0, 16, __Value)
#define SET_TXBUFFER_DESC_AMSDU_WITH_OFFSET(__pTxDesc, __Offset, __Value) \
	SET_BITS_TO_LE_4BYTE(__pTxDesc+(__Offset*16), 31, 1, __Value)
#define SET_TXBUFFER_DESC_ADD_LOW_WITH_OFFSET(__pTxDesc, __Offset, __Value) \
	SET_BITS_TO_LE_4BYTE(__pTxDesc+(__Offset*16)+4, 0, 32, __Value)
#define SET_TXBUFFER_DESC_ADD_HIGH_WITH_OFFSET(__pTxDesc, __Offset, __Value) \
	SET_BITS_TO_LE_4BYTE(__pTxDesc+(__Offset*16)+8, 0, 32, __Value)
#else
#define SET_TXBUFFER_DESC_LEN_WITH_OFFSET(__pTxDesc, __Offset, __Value) \
	SET_BITS_TO_LE_4BYTE(__pTxDesc+(__Offset*8), 0, 16, __Value)
#define SET_TXBUFFER_DESC_AMSDU_WITH_OFFSET(__pTxDesc, __Offset, __Value) \
	SET_BITS_TO_LE_4BYTE(__pTxDesc+(__Offset*8), 31, 1, __Value)
#define SET_TXBUFFER_DESC_ADD_LOW_WITH_OFFSET(__pTxDesc, __Offset, __Value) \
	SET_BITS_TO_LE_4BYTE(__pTxDesc+(__Offset*8)+4, 0, 32, __Value)
#define SET_TXBUFFER_DESC_ADD_HIGH_WITH_OFFSET(__pTxDesc, __Offset, __Value) 0
#endif

/* RX BD */
#define SET_RX_BD_PHYSICAL_ADDR_LOW(__pRxBd, __Value) \
	SET_BITS_TO_LE_4BYTE(__pRxBd + 0x04, 0, 32, __Value)
#ifdef CONFIG_64BIT_DMA
#define SET_RX_BD_PHYSICAL_ADDR_HIGH(__pRxBd, __Value) \
	SET_BITS_TO_LE_4BYTE(__pRxBd + 0x08, 0, 32, __Value)
#else
#define SET_RX_BD_PHYSICAL_ADDR_HIGH(__pRxBd, __Value) 0
#endif
#define SET_RX_BD_RXBUFFSIZE(__pRxBd, __Value) \
	SET_BITS_TO_LE_4BYTE(__pRxBd + 0x00, 0, 14, __Value)
#define SET_RX_BD_LS(__pRxBd, __Value) \
	SET_BITS_TO_LE_4BYTE(__pRxBd + 0x00, 14, 1, __Value)
#define SET_RX_BD_FS(__pRxBd, __Value) \
	SET_BITS_TO_LE_4BYTE(__pRxBd + 0x00, 15, 1, __Value)
#define SET_RX_BD_TOTALRXPKTSIZE(__pRxBd, __Value) \
	SET_BITS_TO_LE_4BYTE(__pRxBd + 0x00, 16, 13, __Value)

/* rtl8822ce_halinit.c */
u32 rtl8822ce_init(PADAPTER);
u32 rtl8822ce_deinit(PADAPTER padapter);
void rtl8822ce_init_default_value(PADAPTER);

/* rtl8822ce_halmac.c */
int rtl8822ce_halmac_init_adapter(PADAPTER);

/* rtl8822ce_io.c */

/* rtl8822ce_led.c */
void rtl8822ce_initswleds(PADAPTER);
void rtl8822ce_deinitswleds(PADAPTER);

/* rtl8822cs_xmit.c */
#define OFFSET_SZ 0

s32 rtl8822ce_init_xmit_priv(PADAPTER);
void rtl8822ce_free_xmit_priv(PADAPTER);
struct xmit_buf *rtl8822ce_dequeue_xmitbuf(struct rtw_tx_ring *);
void rtl8822ce_fill_fake_txdesc(PADAPTER, u8 *pDesc, u32 BufferLen,
				u8 IsPsPoll, u8 IsBTQosNull, u8 bDataFrame);
int rtl8822ce_init_txbd_ring(PADAPTER, unsigned int q_idx,
			     unsigned int entries);
void rtl8822ce_free_txbd_ring(PADAPTER, unsigned int prio);

void rtl8822ce_tx_isr(PADAPTER, int prio);

#ifdef CONFIG_PCI_TX_POLLING
void rtl8822ce_tx_ring_poll(PADAPTER Adapter, int prio);
#endif

s32 rtl8822ce_mgnt_xmit(PADAPTER, struct xmit_frame *);
s32 rtl8822ce_hal_xmit(PADAPTER, struct xmit_frame *);
#ifdef CONFIG_RTW_MGMT_QUEUE
s32 rtl8822ce_hal_mgmt_xmitframe_enqueue(PADAPTER, struct xmit_frame *);
#endif
s32 rtl8822ce_hal_xmitframe_enqueue(PADAPTER, struct xmit_frame *);

#ifdef CONFIG_XMIT_THREAD_MODE
	s32 rtl8822ce_xmit_buf_handler(PADAPTER);
#endif
u32 InitMAC_TRXBD_8822CE(PADAPTER adapter);
void rtl8822ce_reset_bd(_adapter *padapter);

void rtl8822ce_xmitframe_resume(PADAPTER);
void rtl8822ce_hci_flush(PADAPTER Adapter, u32 queue);

/* rtl8822cs_recv.c */
s32 rtl8822ce_init_recv_priv(PADAPTER);
void rtl8822ce_free_recv_priv(PADAPTER);
int rtl8822ce_init_rxbd_ring(PADAPTER);
void rtl8822ce_free_rxbd_ring(PADAPTER);

/* rtl8822cs_ops.c */

#endif /* _RTL8822CE_H_ */
