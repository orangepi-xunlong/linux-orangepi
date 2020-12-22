#ifndef _SUNXI_IR_H
#define _SUNXI_IR_H

#if defined CONFIG_ARCH_SUN8IW1P1
#include <linux/clk/clk-sun8iw1.h>
#define SW_INT_IRQNO_IR0 (SUNXI_IRQ_RCIR)
#define IR0_BASE         (void __iomem *)(0xf1f02000)
#elif defined CONFIG_ARCH_SUN9IW1P1
#include <linux/clk/clk-sun9iw1.h>
#define SW_INT_IRQNO_IR0 (SUNXI_IRQ_R_CIR_RX)
#define IR0_BASE         (void __iomem *)(0xf8002000)
#elif defined CONFIG_ARCH_SUN8IW6P1
#include <linux/clk/clk-sun8iw6.h>
#define SW_INT_IRQNO_IR0 (SUNXI_IRQ_R_CIR_RX)
#define IR0_BASE         (void __iomem *)(0xf1f02000)
#elif defined CONFIG_ARCH_SUN8IW7P1
#include <linux/clk/clk-sun8iw7.h>
#define SW_INT_IRQNO_IR0 (SUNXI_IRQ_R_CIR_RX)
#define IR0_BASE         (void __iomem *)(0xf1f02000)

#endif

#ifdef CONFIG_FPGA_V4_PLATFORM
#define FPGA_SIM_CONFIG                    /* input clk is 24M */
#endif

/* Registers */
#define IR_REG(x)        (x)
#define IR_BASE          IR0_BASE
#define IR_IRQNO         (SW_INT_IRQNO_IR0)

/* CCM register */
#define CCM_BASE         0xf1c20000
/* PIO register */
#define PI_BASE          0xf1c20800

#define IR_CTRL_REG      IR_REG(0x00)     /* IR Control */
#define IR_RXCFG_REG     IR_REG(0x10)     /* Rx Config */
#define IR_RXDAT_REG     IR_REG(0x20)     /* Rx Data */
#define IR_RXINTE_REG    IR_REG(0x2C)     /* Rx Interrupt Enable */
#define IR_RXINTS_REG    IR_REG(0x30)     /* Rx Interrupt Status */
#define IR_SPLCFG_REG    IR_REG(0x34)     /* IR Sample Config */

#define IR_FIFO_SIZE     (64)             /* 64Bytes */

#define IR_CIR_MODE      (0x3<<4)         /* CIR mode enable */
#define IR_ENTIRE_ENABLE (0x3<<0)         /* IR entire enable */
#define IR_SAMPLE_128    (0x1<<0)         /* 3MHz/128 =23437.5Hz (42.7us) */
#define IR_FIFO_32       (((IR_FIFO_SIZE>>1)-1)<<8)
#define IR_IRQ_STATUS    ((0x1<<4)|0x3)

//Bit Definition of IR_RXINTS_REG Register
#define IR_RXINTS_RXOF   (0x1<<0)         /* Rx FIFO Overflow */
#define IR_RXINTS_RXPE   (0x1<<1)         /* Rx Packet End */
#define IR_RXINTS_RXDA   (0x1<<4)         /* Rx FIFO Data Available */

/* Frequency of Sample Clock = 23437.5Hz, Cycle is 42.7us */
/* Pulse of NEC Remote >560us */
#ifdef FPGA_SIM_CONFIG
#define IR_RXFILT_VAL    (((16)&0x3f)<<2) /* Filter Threshold = 8*42.7 = ~341us < 500us */
#define IR_RXIDLE_VAL    (((5)&0xff)<<8)  /* Idle Threshold = (2+1)*128*42.7 = ~16.4ms > 9ms */
#define IR_ACTIVE_T      ((0&0xff)<<16)   /* Active Threshold */
#define IR_ACTIVE_T_C    ((1&0xff)<<23)   /* Active Threshold */

#define IR_L1_MIN        (160)            /* 80*42.7 = ~3.4ms, Lead1(4.5ms) > IR_L1_MIN */
#define IR_L0_MIN        (80)             /* 40*42.7 = ~1.7ms, Lead0(4.5ms) Lead0R(2.25ms)> IR_L0_MIN */
#define IR_PMAX          (52)             /* 26*42.7 = ~1109us ~= 561*2, Pluse < IR_PMAX */
#define IR_DMID          (52)             /* 26*42.7 = ~1109us ~= 561*2, D1 > IR_DMID, D0 =< IR_DMID */
#define IR_DMAX          (106)            /* 53*42.7 = ~2263us ~= 561*4, D < IR_DMAX */

#else
#define IR_RXFILT_VAL    (((8)&0x3f)<<2)  /* Filter Threshold = 8*42.7 = ~341us	< 500us */
#define IR_RXIDLE_VAL    (((2)&0xff)<<8)  /* Idle Threshold = (2+1)*128*42.7 = ~16.4ms > 9ms */
#define IR_ACTIVE_T      ((0&0xff)<<16)   /* Active Threshold */
#define IR_ACTIVE_T_C    ((1&0xff)<<23)   /* Active Threshold */

#define IR_L1_MIN        (80)             /* 80*42.7 = ~3.4ms, Lead1(4.5ms) > IR_L1_MIN */
#define IR_L0_MIN        (40)             /* 40*42.7 = ~1.7ms, Lead0(4.5ms) Lead0R(2.25ms)> IR_L0_MIN */
#define IR_PMAX          (26)             /* 26*42.7 = ~1109us ~= 561*2, Pluse < IR_PMAX */
#define IR_DMID          (26)             /* 26*42.7 = ~1109us ~= 561*2, D1 > IR_DMID, D0 =< IR_DMID */
#define IR_DMAX          (53)             /* 53*42.7 = ~2263us ~= 561*4, D < IR_DMAX */
#endif

#define IR_ERROR_CODE    (0xffffffff)
#define IR_REPEAT_CODE   (0x00000000)
#define DRV_VERSION      "1.00"

enum {
	DEBUG_INIT = 1U << 0,
	DEBUG_INT = 1U << 1,
	DEBUG_DATA_INFO = 1U << 2,
	DEBUG_SUSPEND = 1U << 3,
	DEBUG_ERR = 1U << 4,
};

enum ir_mode {
	CIR_MODE_ENABLE,
	IR_MODULE_ENABLE,
};
enum ir_sample_config {
	IR_SAMPLE_REG_CLEAR,
	IR_CLK_SAMPLE,
	IR_FILTER_TH,
	IR_IDLE_TH,
	IR_ACTIVE_TH,
};
enum ir_irq_config {
	IR_IRQ_STATUS_CLEAR,
	IR_IRQ_ENABLE,
	IR_IRQ_FIFO_SIZE,
};
#endif

