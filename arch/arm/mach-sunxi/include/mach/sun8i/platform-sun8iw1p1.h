/*
 * arch/arm/mach-sunxi/include/mach/sun8i/platform-sun8iw1p1.h
 *
 * Copyright(c) 2013-2015 Allwinnertech Co., Ltd.
 *      http://www.allwinnertech.com
 *
 * Author: liugang <liugang@allwinnertech.com>
 *
 * sun8i platform header file
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef __PLATFORM_SUN8I_W1P1_H
#define __PLATFORM_SUN8I_W1P1_H

/*
 * Memory definitions
 */
#define SUNXI_IO_PBASE                                 0x01c00000
#define SUNXI_IO_SIZE                                  0x00400000 /* 4MB(Max) */
#define SUNXI_SRAM_A1_PBASE                            0x00000000
#define SUNXI_SRAM_A1_SIZE                             0x00008000
#define SUNXI_SRAM_A2_PBASE                            0x00040000
#define SUNXI_SRAM_A2_SIZE                             0x00014000
#define SUNXI_SRAM_D_PBASE                             0x00010000
#define SUNXI_SRAM_D_SIZE                              0x00001000
#define SUNXI_SRAM_B_PBASE                             0x00020000 /* Secure, 64KB */
#define SUNXI_SRAM_B_SIZE                              0x00010000
#define SUNXI_SDRAM_PBASE                              0x40000000
#define SUNXI_BROM_PBASE                               0xffff0000
#define SUNXI_BROM_SIZE                                0x00008000 /* 32KB*/
#define SUNXI_MTC_ACC                                  0x00080000

/*
 *  device physical addresses
 */
#define SUNXI_SRAMCTRL_PBASE                           0x01c00000
#define SUNXI_DMA_PBASE                                0x01c02000
#define SUNXI_NANDFLASHC0_PBASE                        0x01c03000
#define SUNXI_TS_PBASE                                 0x01c04000
#define SUNXI_NANDFLASHC1_PBASE                        0x01c05000
#define SUNXI_LCD0_PBASE                               0x01c0c000
#define SUNXI_LCD1_PBASE                               0x01c0d000
#define SUNXI_VE_PBASE                                 0x01c0e000
#define SUNXI_SDMMC0_PBASE                             0x01c0f000
#define SUNXI_SDMMC1_PBASE                             0x01c10000
#define SUNXI_SDMMC2_PBASE                             0x01c11000
#define SUNXI_SDMMC3_PBASE                             0x01c12000
#define SUNXI_SS_PBASE                                 0x01c15000
#define SUNXI_HDMI_PBASE                               0x01c16000
#define SUNXI_MSGBOX_PBASE                             0x01c17000
#define SUNXI_SPINLOCK_PBASE                           0x01c18000
#define SUNXI_USB_OTG_PBASE                            0x01c19000
#define SUNXI_USB_EHCI0_PBASE                          0x01c1a000
#define SUNXI_USB_OHCI0_PBASE                          0x01c1a000
#define SUNXI_USB_EHCI1_PBASE                          0x01c1b000
#define SUNXI_USB_OHCI1_PBASE                          0x01c1b000
#define SUNXI_USB_OHCI2_PBASE                          0x01c1c000
#define SUNXI_TZASC_PBASE                              0x01c1e000
#define SUNXI_CCM_PBASE                                0x01c20000
#define SUNXI_PIO_PBASE                                0x01c20800
#define SUNXI_TIMER_PBASE                              0x01c20c00
#define SUNXI_SPDIF_PBASE                              0x01c21000
#define SUNXI_PWM_PBASE                                0x01c21400
#define SUNXI_DAUDIO0_PBASE                            0x01c22000
#define SUNXI_DAUDIO1_PBASE                            0x01c22400
#define SUNXI_LRADC01_PBASE                            0x01c22800
#define SUNXI_AUDIOCODEC_PBASE                         0x01c22C00
#define SUNXI_TZPC_PBASE                               0x01c23400
#define SUNXI_SID_PBASE                                0x01c23800
#define SUNXI_SJTAG_PBASE                              0x01c23c00
#define SUNXI_TP_PBASE                                 0x01c25000
#define SUNXI_DMIC_PBASE                               0x01c25400
#define SUNXI_UART0_PBASE                              0x01c28000 /* UART 0 */
#define SUNXI_UART1_PBASE                              0x01c28400 /* UART 1 */
#define SUNXI_UART2_PBASE                              0x01c28800 /* UART 2 */
#define SUNXI_UART3_PBASE                              0x01c28c00 /* UART 3 */
#define SUNXI_UART4_PBASE                              0x01c29000 /* UART 4 */
#define SUNXI_UART5_PBASE                              0x01c29400 /* UART 5 */
#define SUNXI_TWI0_PBASE                               0x01c2ac00
#define SUNXI_TWI1_PBASE                               0x01c2b000
#define SUNXI_TWI2_PBASE                               0x01c2b400
#define SUNXI_TWI3_PBASE                               0x01c2b800
#define SUNXI_GMAC_PBASE                               0x01c30000
#define SUNXI_GPU_PBASE                                0x01c40000
#define SUNXI_HSTMR_PBASE                              0x01c60000
#define SUNXI_DRAMCOM_PBASE                            0x01c62000
#define SUNXI_DRAMCTL0_PBASE                           0x01c63000
#define SUNXI_DRAMCTL1_PBASE                           0x01c64000
#define SUNXI_DRAMPHY0_PBASE                           0x01c65000
#define SUNXI_DRAMPHY1_PBASE                           0x01c66000
#define SUNXI_SPI0_PBASE                               0x01c68000
#define SUNXI_SPI1_PBASE                               0x01c69000
#define SUNXI_SPI2_PBASE                               0x01c6a000
#define SUNXI_SPI3_PBASE                               0x01c6b000
#define SUNXI_SCU_PBASE                                0x01c80000
#define SUNXI_GIC_DIST_PBASE                           0x01c81000
#define SUNXI_GIC_CPU_PBASE                            0x01c82000
#define SUNXI_MIPI_DSI0_PBASE                          0x01ca0000
#define SUNXI_MIPI_DSI0_PHY_PBASE                      0x01ca1000
#define SUNXI_CSI0_PBASE                               0x01cb0000
#define SUNXI_MIPI_CSI0_PBASE                          0x01cb1000
#define SUNXI_MIPI_CSI0_PHY_PBASE                      0x01cb2000
#define SUNXI_CSI1_PBASE                               0x01cb3000
#define SUNXI_ISP_PBASE                                0x01cb8000
#define SUNXI_ISP_MEM_PBASE                            0x01cc0000
#define SUNXI_SRAM_C_PBASE                             0x01d00000
#define SUNXI_DE_FE0_PBASE                             0x01e00000
#define SUNXI_DE_FE1_PBASE                             0x01e20000
#define SUNXI_DE_BE1_PBASE                             0x01e40000
#define SUNXI_DRC1_PBASE                               0x01e50000
#define SUNXI_DE_BE0_PBASE                             0x01e60000
#define SUNXI_DRC0_PBASE                               0x01e70000
#define SUNXI_MP_PBASE                                 0x01e80000
#define SUNXI_DEU1_PBASE                               0x01ea0000
#define SUNXI_DEU0_PBASE                               0x01eb0000
#define SUNXI_PS_PBASE                                 0x01ef0000
#define SUNXI_RTC_PBASE                                0x01f00000
#define SUNXI_R_TIMER_PBASE                            0x01f00800
#define SUNXI_R_INTC_PBASE                             0x01f00c00
#define SUNXI_R_WDOG_PBASE                             0x01f01000
#define SUNXI_R_PRCM_PBASE                             0x01f01400
#define SUNXI_R_CPUCFG_PBASE                           0x01f01c00
#define SUNXI_R_CIR_PBASE                              0x01f02000
#define SUNXI_R_TWI_PBASE                              0x01f02400
#define SUNXI_R_UART_PBASE                             0x01f02800 /* R_UART */
#define SUNXI_R_PIO_PBASE                              0x01f02c00 /* for r-pio */
#define SUNXI_R_ONE_WIRE_PBASE                         0x01f03000
#define SUNXI_R_P2WI_PBASE                             0x01f03400
#define SUNXI_CDM_PBASE                                0x3f500000 /*coresight debug module*/
#define SUNXI_TSGEN_RO_PBASE                           0x3f506000
#define SUNXI_TSGEN_CTRL_PBASE                         0x3f507000

#define SUNXI_GIC_DIST_PBASE                           0x01c81000
#define SUNXI_GIC_CPU_PBASE                            0x01c82000
#define SUNXI_TIMER_G_PBASE                            0x01c80200 /* CPU global timer, not used */
#define SUNXI_TIMER_P_PBASE                            0x01c80600 /* CPU private timer, not used */

/*
 * Peripheral addresses
 */
#define SUNXI_RTC_REG                                  SUNXI_RTC_BASE
#define SUNXI_RPIO_BASE                                SUNXI_R_PIO_BASE

/*
 *  device virtual addresses
 */
#define SUNXI_IO_VBASE                                 0xf1c00000
#define SUNXI_SRAM_A1_VBASE                            0xf0000000
#define SUNXI_SRAM_A2_VBASE                            0xf0040000
#define SUNXI_SRAM_D_VBASE                             0xf0010000
#define SUNXI_SRAM_B_VBASE                             0xf0020000 /* Secure, 64KB */
#define SUNXI_BROM_VBASE                               0xf1000000

#define SUNXI_SRAMCTRL_VBASE                           0xf1c00000
#define SUNXI_DMA_VBASE                                0xf1c02000
#define SUNXI_NANDFLASHC0_VBASE                        0xf1c03000
#define SUNXI_TS_VBASE                                 0xf1c04000
#define SUNXI_NANDFLASHC1_VBASE                        0xf1c05000
#define SUNXI_LCD0_VBASE                               0xf1c0c000
#define SUNXI_LCD1_VBASE                               0xf1c0d000
#define SUNXI_VE_VBASE                                 0xf1c0e000
#define SUNXI_SDMMC0_VBASE                             0xf1c0f000
#define SUNXI_SDMMC1_VBASE                             0xf1c10000
#define SUNXI_SDMMC2_VBASE                             0xf1c11000
#define SUNXI_SDMMC3_VBASE                             0xf1c12000
#define SUNXI_SS_VBASE                                 0xf1c15000
#define SUNXI_HDMI_VBASE                               0xf1c16000
#define SUNXI_MSGBOX_VBASE                             0xf1c17000
#define SUNXI_SPINLOCK_VBASE                           0xf1c18000
#define SUNXI_USB_OTG_VBASE                            0xf1c19000
#define SUNXI_USB_EHCI0_VBASE                          0xf1c1a000
#define SUNXI_USB_OHCI0_VBASE                          0xf1c1a000
#define SUNXI_USB_EHCI1_VBASE                          0xf1c1b000
#define SUNXI_USB_OHCI1_VBASE                          0xf1c1b000
#define SUNXI_USB_OHCI2_VBASE                          0xf1c1c000
#define SUNXI_TZASC_VBASE                              0xf1c1e000
#define SUNXI_CCM_VBASE                                0xf1c20000
#define SUNXI_PIO_VBASE                                0xf1c20800
#define SUNXI_TIMER_VBASE                              0xf1c20c00
#define SUNXI_SPDIF_VBASE                              0xf1c21000
#define SUNXI_PWM_VBASE                                0xf1c21400
#define SUNXI_DAUDIO0_VBASE                            0xf1c22000
#define SUNXI_DAUDIO1_VBASE                            0xf1c22400
#define SUNXI_LRADC01_VBASE                            0xf1c22800
#define SUNXI_AUDIOCODEC_VBASE                         0xf1c22C00
#define SUNXI_TZPC_VBASE                               0xf1c23400
#define SUNXI_SID_VBASE                                0xf1c23800
#define SUNXI_SJTAG_VBASE                              0xf1c23c00
#define SUNXI_TP_VBASE                                 0xf1c25000
#define SUNXI_DMIC_VBASE                               0xf1c25400
#define SUNXI_UART0_VBASE                              0xf1c28000 /* UART 0 */
#define SUNXI_UART1_VBASE                              0xf1c28400 /* UART 1 */
#define SUNXI_UART2_VBASE                              0xf1c28800 /* UART 2 */
#define SUNXI_UART3_VBASE                              0xf1c28c00 /* UART 3 */
#define SUNXI_UART4_VBASE                              0xf1c29000 /* UART 4 */
#define SUNXI_UART5_VBASE                              0xf1c29400 /* UART 5 */
#define SUNXI_TWI0_VBASE                               0xf1c2ac00
#define SUNXI_TWI1_VBASE                               0xf1c2b000
#define SUNXI_TWI2_VBASE                               0xf1c2b400
#define SUNXI_TWI3_VBASE                               0xf1c2b800
#define SUNXI_GMAC_VBASE                               0xf1c30000
#define SUNXI_GPU_VBASE                                0xf1c40000
#define SUNXI_HSTMR_VBASE                              0xf1c60000
#define SUNXI_DRAMCOM_VBASE                            0xf1c62000
#define SUNXI_DRAMCTL0_VBASE                           0xf1c63000
#define SUNXI_DRAMCTL1_VBASE                           0xf1c64000
#define SUNXI_DRAMPHY0_VBASE                           0xf1c65000
#define SUNXI_DRAMPHY1_VBASE                           0xf1c66000
#define SUNXI_SPI0_VBASE                               0xf1c68000
#define SUNXI_SPI1_VBASE                               0xf1c69000
#define SUNXI_SPI2_VBASE                               0xf1c6a000
#define SUNXI_SPI3_VBASE                               0xf1c6b000
#define SUNXI_SCU_VBASE                                0xf1c80000
#define SUNXI_GIC_DIST_VBASE                           0xf1c81000
#define SUNXI_GIC_CPU_VBASE                            0xf1c82000
#define SUNXI_MIPI_DSI0_VBASE                          0xf1ca0000
#define SUNXI_MIPI_DSI0_PHY_VBASE                      0xf1ca1000
#define SUNXI_CSI0_VBASE                               0xf1cb0000
#define SUNXI_MIPI_CSI0_VBASE                          0xf1cb1000
#define SUNXI_MIPI_CSI0_PHY_VBASE                      0xf1cb2000
#define SUNXI_CSI1_VBASE                               0xf1cb3000
#define SUNXI_ISP_VBASE                                0xf1cb8000
#define SUNXI_ISP_MEM_VBASE                            0xf1cc0000
#define SUNXI_SRAM_C_VBASE                             0xf1d00000
#define SUNXI_DE_FE0_VBASE                             0xf1e00000
#define SUNXI_DE_FE1_VBASE                             0xf1e20000
#define SUNXI_DE_BE1_VBASE                             0xf1e40000
#define SUNXI_DRC1_VBASE                               0xf1e50000
#define SUNXI_DE_BE0_VBASE                             0xf1e60000
#define SUNXI_DRC0_VBASE                               0xf1e70000
#define SUNXI_MP_VBASE                                 0xf1e80000
#define SUNXI_DEU1_VBASE                               0xf1ea0000
#define SUNXI_DEU0_VBASE                               0xf1eb0000
#define SUNXI_PS_VBASE                                 0xf1ef0000
#define SUNXI_RTC_VBASE                                0xf1f00000
#define SUNXI_R_TIMER_VBASE                            0xf1f00800
#define SUNXI_R_INTC_VBASE                             0xf1f00c00
#define SUNXI_R_WDOG_VBASE                             0xf1f01000
#define SUNXI_R_PRCM_VBASE                             0xf1f01400
#define SUNXI_R_CPUCFG_VBASE                           0xf1f01c00
#define SUNXI_R_CIR_VBASE                              0xf1f02000
#define SUNXI_R_TWI_VBASE                              0xf1f02400
#define SUNXI_R_UART_VBASE                             0xf1f02800 /* R_UART */
#define SUNXI_R_PIO_VBASE                              0xf1f02c00 /* for r-pio */
#define SUNXI_R_ONE_WIRE_VBASE                         0xf1f03000
#define SUNXI_R_P2WI_VBASE                             0xf1f03400

/*
 * Timer registers
 */
#define SUNXI_TMR_IRQ_EN_REG                           0x0000
#define SUNXI_TMR_IRQ_STA_REG                          0x0004
#define SUNXI_TMR0_CTRL_REG                            0x0010
#define SUNXI_TMR0_INTV_VALUE_REG                      0x0014
#define SUNXI_TMR0_CUR_VALUE_REG                       0x0018

#define SUNXI_AVS_CNT_CTL_REG                          0x0080
#define SUNXI_AVS_CNT0_REG                             0x0084
#define SUNXI_AVS_CNT1_REG                             0x0088
#define SUNXI_AVS_CNT_DIV_REG                          0x008c

#define SUNXI_WDOG1_IRQ_EN_REG                         0xa0
#define SUNXI_WDOG1_IRQ_STA_REG                        0xa4
#define SUNXI_WDOG1_CTRL_REG                           0xb0
#define SUNXI_WDOG1_CFG_REG                            0xb4
#define SUNXI_WDOG1_MODE_REG                           0xb8

/* r-watchdog0 reg offset define */
#define R_WDOG_IRQ_EN_REG                              0x0
#define R_WDOG_IRQ_STA_REG                             0x4
#define R_WDOG_CTRL_REG                                0x10
#define R_WDOG_CFG_REG                                 0x14
#define R_WDOG_MODE_REG                                0x18

/*
 * cpucfg
 */
#define SUNXI_CPUCFG_P_REG0                            0x01a4
#define SUNXI_CPUCFG_P_REG1                            0x01a8
#define CPUX_RESET_CTL(x)                             (0x40 + (x) * 0x40)
#define CPUX_CONTROL(x)                               (0x44 + (x) * 0x40)
#define CPUX_STATUS(x)                                (0x48 + (x) * 0x40)
#define SUNXI_CPUCFG_GENCTL                            0x0184
#define SUNXI_CPUCFG_DBGCTL0                           0x01e0
#define SUNXI_CPUCFG_DBGCTL1                           0x01e4

/*
 * prcm
 */
#define SUNXI_CPU_PWROFF_REG                           0x100
/* cpu0 has no clmap register! */
#define SUNXI_CPUX_PWR_CLAMP(x)                        (0x140 + (x)*0x04)
#define SUNXI_CPUX_PWR_CLAMP_STATUS(x)                 (0x64 + (x)*0x40)

/*
 * uart
 */
#define SUNXI_UART_RBR              0x00 /* Receive Buffer Register */
#define SUNXI_UART_THR              0x00 /* Transmit Holding Register */
#define SUNXI_UART_DLL              0x00 /* Divisor Latch Low Register */
#define SUNXI_UART_DLH              0x04 /* Diviso Latch High Register */
#define SUNXI_UART_IER              0x04 /* Interrupt Enable Register */
#define SUNXI_UART_IIR              0x08 /* Interrrupt Identity Register */
#define SUNXI_UART_FCR              0x08 /* FIFO Control Register */
#define SUNXI_UART_LCR              0x0c /* Line Control Register */
#define SUNXI_UART_MCR              0x10 /* Modem Control Register */
#define SUNXI_UART_LSR              0x14 /* Line Status Register */
#define SUNXI_UART_MSR              0x18 /* Modem Status Register */
#define SUNXI_UART_SCH              0x1c /* Scratch Register */
#define SUNXI_UART_USR              0x7c /* Status Register */
#define SUNXI_UART_TFL              0x80 /* Transmit FIFO Level */
#define SUNXI_UART_RFL              0x84 /* RFL */
#define SUNXI_UART_HALT             0xa4 /* Halt TX Register */

#define UART_USR                   (SUNXI_UART_USR >> 2)
#define UART_HALT                  (SUNXI_UART_HALT >> 2)
#define UART_SCH                   (SUNXI_UART_SCH >> 2)
#define UART_FORCE_CFG             (1 << 1)
#define UART_FORCE_UPDATE          (1 << 2)

#define SUNXI_UART_LOG(fmt, args...) do {} while(0)

#define SUNXI_R_UART_LOG(fmt, args...)                                          \
do {                                                                            \
	aw_printk((u32)SUNXI_R_UART_BASE, "[%s]"fmt"\n", __FUNCTION__, ##args); \
}while(0)

#endif /* __PLATFORM_SUN8I_W1P1_H */
