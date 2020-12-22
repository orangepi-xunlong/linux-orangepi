#ifndef _SUNXI_IR_TX_H
#define _SUNXI_IR_TX_H

#define IR_TX_BASSADDRESS	(0xf1f02000)

#define IR_TX_GLR		(0x00)
#define IR_TX_MCR		(0x04)
#define IR_TX_CR		(0x08)
#define IR_TX_IDC_H		(0x0c)
#define IR_TX_IDC_L		(0x10)
#define IR_TX_ICR_H		(0x14)
#define IR_TX_ICR_L		(0x18)
#define IR_TX_TELR		(0x20)
#define IR_TX_INT_CR		(0x24)
#define IR_TX_TACR		(0x28)
#define IR_TX_STAR		(0x2c)
#define IR_TX_TR		(0x30)
#define IR_TX_DMA_CR		(0x34)
#define IR_TX_FIFO_DR		(0x80)

#define IR_TX_GL_VALUE		(0x27)
#define IR_TX_MC_VALUE		(0x9e)
#define IR_TX_C_VALUE		((0x5)<<1)
#define IR_TX_IDC_H_VALUE	(0x00)
#define IR_TX_IDC_L_VALUE	(0x50)
#define IR_TX_TEL_VALUE		(0x96-1)
#define IR_TX_INT_C_VALUE	(0x03)
#define IR_TX_STA_VALUE		(0x03)
#define IR_TX_T_VALUE		(0x64)


#define IR_TX_IRQNO		(SUNXI_IRQ_CIR_TX)

#define IR_TX_DEV_NAME		"sunxi-ir-tx"
#define IR_TX_FIFO_SIZE		(128)

#define	IR_RAW_BUF_SIZE		(256)
#define IR_CYCLE_TYPE		(0)			/* 1:cycle 0:non-cycle */
#define CLK_Ts			(1)

enum {
	DEBUG_INIT = 1U << 0,
	DEBUG_CONTROL_INFO = 1U << 1,
	DEBUG_DATA_INFO = 1U << 2,
	DEBUG_SUSPEND = 1U << 3,
	DEBUG_INT = 1U << 4,
};

#endif /* _SUNXI_IR_TX_H */
