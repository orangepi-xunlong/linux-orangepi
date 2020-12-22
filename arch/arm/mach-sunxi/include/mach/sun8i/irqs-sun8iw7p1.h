/*
 * arch/arm/mach-sunxi/include/mach/sun8i/irqs-sun8iw5p1.h
 *
 * Copyright(c) 2013-2015 Allwinnertech Co., Ltd.
 *      http://www.allwinnertech.com
 *
 * Author: kevin.z.m <kevin@allwinnertech.com>
 *
 * sun8i irq defination header file
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef __IRQS_SUN8I_W7P1_H
#define __IRQS_SUN8I_W7P1_H
#if  (defined CONFIG_FPGA_V4_PLATFORM)|| (defined CONFIG_FPGA_V7_PLATFORM) /* S4 820 */
#define SUNXI_IRQ_NMI                   (SUNXI_GIC_START + 0)  /* 32 */
#define SUNXI_IRQ_UART0                 (SUNXI_GIC_START + 1)  /* 33 */
#define SUNXI_IRQ_UART1                 (SUNXI_GIC_START + 1)  /* 33 */
#define SUNXI_IRQ_TWI0                  (SUNXI_GIC_START + 2)  /* 34 */
#define SUNXI_IRQ_TWI1                  (SUNXI_GIC_START + 2)  /* 34 */
#define SUNXI_IRQ_TWI2                  (SUNXI_GIC_START + 2)  /* 34 */
#define SUNXI_IRQ_CSI_CCI               (SUNXI_GIC_START + 2)  /* 34 */
#define SUNXI_IRQ_EINTA                 (SUNXI_GIC_START + 3)  /* 35 */
#define SUNXI_IRQ_EINTB                 (SUNXI_GIC_START + 3)  /* 35 */
#define SUNXI_IRQ_EINTG                 (SUNXI_GIC_START + 3)  /* 35 */
#define SUNXI_IRQ_EINTL                 (SUNXI_GIC_START + 3)  /* 35 */
#define SUNXI_IRQ_IIS0                  (SUNXI_GIC_START + 4)  /* 36 */
#define SUNXI_IRQ_IIS1                  (SUNXI_GIC_START + 4)  /* 36 */
#define SUNXI_IRQ_CSI                   (SUNXI_GIC_START + 5)  /* 37 */
#define SUNXI_IRQ_TIMER0                (SUNXI_GIC_START + 6)  /* 38 */
#define SUNXI_IRQ_TIMER1                (SUNXI_GIC_START + 6)  /* 38 */
#define SUNXI_IRQ_WATCHDOG              (SUNXI_GIC_START + 6)  /* 38 */
#define SUNXI_IRQ_DMA                   (SUNXI_GIC_START + 7)  /* 39 */
#define SUNXI_IRQ_LCD0                  (SUNXI_GIC_START + 8)  /* 40 */
#define SUNXI_IRQ_RTIMER0               (SUNXI_GIC_START + 9)  /* 41 */
#define SUNXI_IRQ_RTIMER1               (SUNXI_GIC_START + 9)  /* 41 */
#define SUNXI_IRQ_RWATCHDOG             (SUNXI_GIC_START + 9)  /* 41 */
#define SUNXI_IRQ_MBOX                  (SUNXI_GIC_START + 9)  /* 41 */
#define SUNXI_IRQ_HSTMR                 (SUNXI_GIC_START + 10) /* 42 */
#define SUNXI_IRQ_MMC0                  (SUNXI_GIC_START + 11) /* 43 */
#define SUNXI_IRQ_MMC1                  (SUNXI_GIC_START + 11) /* 43 */
#define SUNXI_IRQ_MMC2                  (SUNXI_GIC_START + 11) /* 43 */
#define SUNXI_IRQ_SPI0                  (SUNXI_GIC_START + 12) /* 44 */
#define SUNXI_IRQ_SPI1                  (SUNXI_GIC_START + 12) /* 44 */
#define SUNXI_IRQ_NAND                  (SUNXI_GIC_START + 13) /* 45 */
#define SUNXI_IRQ_RUART                 (SUNXI_GIC_START + 14) /* 46 */
#define SUNXI_IRQ_RTWI                  (SUNXI_GIC_START + 14) /* 46 */
#define SUNXI_IRQ_RALARM0               (SUNXI_GIC_START + 15) /* 47 */
#define SUNXI_IRQ_RALARM1               (SUNXI_GIC_START + 15) /* 47 */
#define SUNXI_IRQ_VE                    (SUNXI_GIC_START + 16) /* 48 */
#define SUNXI_IRQ_USBOTG                (SUNXI_GIC_START + 17) /* 49 */
#define SUNXI_IRQ_USBEHCI0              (SUNXI_GIC_START + 18) /* 50 */
#define SUNXI_IRQ_USBOHCI0              (SUNXI_GIC_START + 19) /* 51 */
#define SUNXI_IRQ_GPU_GP                (SUNXI_GIC_START + 20) /* 52 */
#define SUNXI_IRQ_GPU_GPMMU             (SUNXI_GIC_START + 21) /* 53 */
#define SUNXI_IRQ_GPU_PP0               (SUNXI_GIC_START + 22) /* 54 */
#define SUNXI_IRQ_GPU_PPMMU0            (SUNXI_GIC_START + 23) /* 55 */
#define SUNXI_IRQ_GPU_PMU               (SUNXI_GIC_START + 24) /* 56 */
#define SUNXI_IRQ_GPU_PP1               (SUNXI_GIC_START + 25) /* 57 */
#define SUNXI_IRQ_GPU_PPMMU1            (SUNXI_GIC_START + 26) /* 58 */
#define SUNXI_IRQ_SPDIF                 (SUNXI_GIC_START + 27) /* 59 */
#define SUNXI_IRQ_SS                    (SUNXI_GIC_START + 27) /* 59 */
#define SUNXI_IRQ_R_CIR_RX              (SUNXI_GIC_START + 27) /* 59 */
#define SUNXI_IRQ_LCD1                  (SUNXI_GIC_START + 27) /* 59 */
#define SUNXI_IRQ_RTWD                  (SUNXI_GIC_START + 27) /* 59 */
#define SUNXI_IRQ_SMC                   (SUNXI_GIC_START + 27) /* 59 */
#define SUNXI_IRQ_DE                    (SUNXI_GIC_START + 28) /* 60 */
#define SUNXI_IRQ_TS                    (SUNXI_GIC_START + 29) /* 61 */
#define SUNXI_IRQ_DIT                   (SUNXI_GIC_START + 29) /* 61 */
#define SUNXI_IRQ_GMAC                  (SUNXI_GIC_START + 30) /* 62 */

#define SUNXI_IRQ_PMU                   (SUNXI_GIC_START + 100) /* 132, invalid */

#elif  (defined CONFIG_EVB_PLATFORM)

#define SUNXI_IRQ_UART0                 (SUNXI_GIC_START + 0)  /* 32 */
#define SUNXI_IRQ_UART1                 (SUNXI_GIC_START + 1)  /* 33 */
#define SUNXI_IRQ_UART2                 (SUNXI_GIC_START + 2)  /* 34 */
#define SUNXI_IRQ_UART3                 (SUNXI_GIC_START + 3)  /* 35 */
#define SUNXI_IRQ_TWI0                  (SUNXI_GIC_START + 6)  /* 38 */
#define SUNXI_IRQ_TWI1                  (SUNXI_GIC_START + 7)  /* 39 */
#define SUNXI_IRQ_TWI2                  (SUNXI_GIC_START + 8)  /* 40 */
#define SUNXI_IRQ_EINTA                 (SUNXI_GIC_START + 11)  /* 43 */
#define SUNXI_IRQ_SPDIF                 (SUNXI_GIC_START + 12)  /* 44 */
#define SUNXI_IRQ_IIS0                  (SUNXI_GIC_START + 13)  /* 45 */
#define SUNXI_IRQ_IIS1                  (SUNXI_GIC_START + 14)  /* 46 */
#define SUNXI_IRQ_IIS2                  (SUNXI_GIC_START + 15)  /* 47 */
#define SUNXI_IRQ_EINTG                 (SUNXI_GIC_START + 17)  /* 49 */
#define SUNXI_IRQ_TIMER0                (SUNXI_GIC_START + 18)  /* 50 */
#define SUNXI_IRQ_TIMER1                (SUNXI_GIC_START + 19)  /* 51 */
#define SUNXI_IRQ_WATCHDOG              (SUNXI_GIC_START + 25)  /* 57 */
#define SUNXI_IRQ_AUDIO                 (SUNXI_GIC_START + 29)  /* 61 */
#define SUNXI_IRQ_LRADC                 (SUNXI_GIC_START + 30)  /* 62 */
#define SUNXI_IRQ_THERMAL               (SUNXI_GIC_START + 31)  /* 63 */
#define SUNXI_IRQ_NMI                   (SUNXI_GIC_START + 32)  /* 64 */
#define SUNXI_IRQ_RTIMER0               (SUNXI_GIC_START + 33)  /* 65 */
#define SUNXI_IRQ_RTIMER1               (SUNXI_GIC_START + 34)  /* 66 */
#define SUNXI_IRQ_RWATCHDOG             (SUNXI_GIC_START + 36)  /* 68 */
#define SUNXI_IRQ_R_CIR_RX              (SUNXI_GIC_START + 37)  /* 69 */
#define SUNXI_IRQ_RUART                 (SUNXI_GIC_START + 38)  /* 70 */
#define SUNXI_IRQ_RALARM0               (SUNXI_GIC_START + 40)  /* 72 */
#define SUNXI_IRQ_RALARM1               (SUNXI_GIC_START + 41)  /* 73 */
#define SUNXI_IRQ_RTIMER2               (SUNXI_GIC_START + 42)  /* 74 */
#define SUNXI_IRQ_RTIMER3               (SUNXI_GIC_START + 43)  /* 75 */
#define SUNXI_IRQ_RTWI                  (SUNXI_GIC_START + 44)  /* 76 */
#define SUNXI_IRQ_EINTL                 (SUNXI_GIC_START + 45)  /* 77 */
#define SUNXI_IRQ_RTWD                  (SUNXI_GIC_START + 46)  /* 78 */
#define SUNXI_IRQ_MBOX                  (SUNXI_GIC_START + 49)  /* 81 */
#define SUNXI_IRQ_DMA                   (SUNXI_GIC_START + 50)  /* 82 */
#define SUNXI_IRQ_HSTMR                 (SUNXI_GIC_START + 51)  /* 83 */
#define SUNXI_IRQ_SMC                   (SUNXI_GIC_START + 56)  /* 88 */
#define SUNXI_IRQ_VE                    (SUNXI_GIC_START + 58)  /* 90 */
#define SUNXI_IRQ_MMC0                  (SUNXI_GIC_START + 60)  /* 92 */
#define SUNXI_IRQ_MMC1                  (SUNXI_GIC_START + 61)  /* 93 */
#define SUNXI_IRQ_MMC2                  (SUNXI_GIC_START + 62)  /* 94 */
#define SUNXI_IRQ_SPI0                  (SUNXI_GIC_START + 65)  /* 97 */
#define SUNXI_IRQ_SPI1                  (SUNXI_GIC_START + 66)  /* 98 */
#define SUNXI_IRQ_NAND                  (SUNXI_GIC_START + 70)  /* 102 */
#define SUNXI_IRQ_USBOTG                (SUNXI_GIC_START + 71)  /* 103 */
#define SUNXI_IRQ_USBEHCI0              (SUNXI_GIC_START + 72)  /* 104 */
#define SUNXI_IRQ_USBOHCI0              (SUNXI_GIC_START + 73)  /* 105 */
#define SUNXI_IRQ_USBEHCI1              (SUNXI_GIC_START + 74)  /* 106 */
#define SUNXI_IRQ_USBOHCI1              (SUNXI_GIC_START + 75)  /* 107 */
#define SUNXI_IRQ_USBEHCI2              (SUNXI_GIC_START + 76)  /* 108 */
#define SUNXI_IRQ_USBOHCI2              (SUNXI_GIC_START + 77)  /* 109 */
#define SUNXI_IRQ_USBEHCI3              (SUNXI_GIC_START + 78)  /* 110 */
#define SUNXI_IRQ_USBOHCI3              (SUNXI_GIC_START + 79)  /* 111 */
#define SUNXI_IRQ_SS_TZ                 (SUNXI_GIC_START + 80)  /* 112 SS in TrustZone */
#define SUNXI_IRQ_TS                    (SUNXI_GIC_START + 81)  /* 113 */
#define SUNXI_IRQ_GMAC                  (SUNXI_GIC_START + 82)  /* 114 */
#define SUNXI_IRQ_SCR                   (SUNXI_GIC_START + 83)  /* 115 */
#define SUNXI_IRQ_CSI                   (SUNXI_GIC_START + 84)  /* 116 */
#define SUNXI_IRQ_CSI_CCI               (SUNXI_GIC_START + 85)  /* 117 */
#define SUNXI_IRQ_LCD0                  (SUNXI_GIC_START + 86)  /* 118 */
#define SUNXI_IRQ_LCD1                  (SUNXI_GIC_START + 87)  /* 119 */
#define SUNXI_IRQ_HDMI                  (SUNXI_GIC_START + 88)  /* 120 */
#define SUNXI_IRQ_TVE                   (SUNXI_GIC_START + 92)  /* 124 */
#define SUNXI_IRQ_DIT                   (SUNXI_GIC_START + 93)  /* 125 */
#define SUNXI_IRQ_SS                    (SUNXI_GIC_START + 94)  /* 126 */
#define SUNXI_IRQ_DE                    (SUNXI_GIC_START + 95)  /* 127 */
#define SUNXI_IRQ_GPU_GP                (SUNXI_GIC_START + 97)  /* 129 */
#define SUNXI_IRQ_GPU_GPMMU             (SUNXI_GIC_START + 98)  /* 130 */
#define SUNXI_IRQ_GPU_PP0               (SUNXI_GIC_START + 99)  /* 131 */
#define SUNXI_IRQ_GPU_PPMMU0            (SUNXI_GIC_START + 100)  /* 132 */
#define SUNXI_IRQ_GPU_PMU               (SUNXI_GIC_START + 101)  /* 133 */
#define SUNXI_IRQ_GPU_PP1               (SUNXI_GIC_START + 102)  /* 134 */
#define SUNXI_IRQ_GPU_PPMMU1            (SUNXI_GIC_START + 103)  /* 135 */
#define SUNXI_IRQ_CTI0                  (SUNXI_GIC_START + 108)  /* 140 */
#define SUNXI_IRQ_CTI1                  (SUNXI_GIC_START + 109)  /* 141 */
#define SUNXI_IRQ_CTI2                  (SUNXI_GIC_START + 110)  /* 142 */
#define SUNXI_IRQ_CTI3                  (SUNXI_GIC_START + 111)  /* 143 */
#define SUNXI_IRQ_COMMTX0               (SUNXI_GIC_START + 112)  /* 144 */
#define SUNXI_IRQ_COMMTX1               (SUNXI_GIC_START + 113)  /* 145 */
#define SUNXI_IRQ_COMMTX2               (SUNXI_GIC_START + 114)  /* 146 */
#define SUNXI_IRQ_COMMTX3               (SUNXI_GIC_START + 115)  /* 147 */
#define SUNXI_IRQ_COMMRX0               (SUNXI_GIC_START + 116)  /* 148 */
#define SUNXI_IRQ_COMMRX1               (SUNXI_GIC_START + 117)  /* 149 */
#define SUNXI_IRQ_COMMRX2               (SUNXI_GIC_START + 118)  /* 150 */
#define SUNXI_IRQ_COMMRX3               (SUNXI_GIC_START + 119)  /* 151 */
#define SUNXI_IRQ_PMU0                  (SUNXI_GIC_START + 120)  /* 152 */
#define SUNXI_IRQ_PMU1                  (SUNXI_GIC_START + 121)  /* 153 */
#define SUNXI_IRQ_PMU2                  (SUNXI_GIC_START + 122)  /* 154 */
#define SUNXI_IRQ_PMU3                  (SUNXI_GIC_START + 123)  /* 155 */
#define SUNXI_IRQ_AXIERR                (SUNXI_GIC_START + 124)  /* 156 */

#else

#error "please select a platform"

#endif

#define SUNXI_IRQ_MAX           (SUNXI_GIC_START + 256)

#endif    /* __IRQS_SUN8I_W7P1_H */
