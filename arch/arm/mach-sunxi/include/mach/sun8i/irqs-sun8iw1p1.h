/*
 * arch/arm/mach-sunxi/include/mach/sun8i/irqs-sun8iw1p1.h
 *
 * Copyright(c) 2013-2015 Allwinnertech Co., Ltd.
 *      http://www.allwinnertech.com
 *
 * Author: liugang <liugang@allwinnertech.com>
 *
 * sun8i irq defination header file
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef __IRQS_SUN8I_W1P1_H
#define __IRQS_SUN8I_W1P1_H

#define SUNXI_IRQ_UART_DEBUG           (SUNXI_GIC_START + 0)   /* UART0       */
#define SUNXI_IRQ_UART0                (SUNXI_GIC_START + 0)   /* UART0       */
#define SUNXI_IRQ_UART1                (SUNXI_GIC_START + 1)   /* UART1       */
#define SUNXI_IRQ_UART2                (SUNXI_GIC_START + 2)   /* UART2       */
#define SUNXI_IRQ_UART3                (SUNXI_GIC_START + 3)   /* UART3       */
#define SUNXI_IRQ_UART4                (SUNXI_GIC_START + 4)   /* UART3       */
#define SUNXI_IRQ_UART5                (SUNXI_GIC_START + 5)   /* UART3       */
#define SUNXI_IRQ_TWI0                 (SUNXI_GIC_START + 6)   /* TWI0        */
#define SUNXI_IRQ_TWI1                 (SUNXI_GIC_START + 7)   /* TWI1        */
#define SUNXI_IRQ_TWI2                 (SUNXI_GIC_START + 8)   /* TWI0        */
#define SUNXI_IRQ_TWI3                 (SUNXI_GIC_START + 9)   /* TWI1        */

#define SUNXI_IRQ_EINTA                (SUNXI_GIC_START + 11)  /* EINTA       */
#define SUNXI_IRQ_SPDIF                (SUNXI_GIC_START + 12)  /* SPDIF       */
#define SUNXI_IRQ_DAUDIO0              (SUNXI_GIC_START + 13)  /* DAUDIO0     */
#define SUNXI_IRQ_DAUDIO1              (SUNXI_GIC_START + 14)  /* DAUDIO1     */
#define SUNXI_IRQ_EINTB                (SUNXI_GIC_START + 15)  /* EINTB       */
#define SUNXI_IRQ_EINTE                (SUNXI_GIC_START + 16)  /* EINTE       */
#define SUNXI_IRQ_EINTG                (SUNXI_GIC_START + 17)  /* EINTG       */
#define SUNXI_IRQ_TIMER0               (SUNXI_GIC_START + 18)  /* Timer0      */
#define SUNXI_IRQ_TIMER1               (SUNXI_GIC_START + 19)  /* Timer1      */
#define SUNXI_IRQ_TIMER2               (SUNXI_GIC_START + 20)  /* Timer2      */
#define SUNXI_IRQ_TIMER3               (SUNXI_GIC_START + 21)  /* Timer3      */
#define SUNXI_IRQ_TIMER4               (SUNXI_GIC_START + 22)  /* Timer4      */
#define SUNXI_IRQ_TIMER5               (SUNXI_GIC_START + 23)  /* Timer5      */
#define SUNXI_IRQ_WATCHDOG4            (SUNXI_GIC_START + 24)  /* WATCHDOG4   */
#define SUNXI_IRQ_WATCHDOG1            (SUNXI_GIC_START + 25)  /* WATCHDOG1   */
#define SUNXI_IRQ_WATCHDOG2            (SUNXI_GIC_START + 26)  /* WATCHDOG2   */
#define SUNXI_IRQ_WATCHDOG3            (SUNXI_GIC_START + 27)  /* WATCHDOG3   */
#define SUNXI_IRQ_TOUCHPANEL           (SUNXI_GIC_START + 28)  /* TOUCH PANEL */
#define SUNXI_IRQ_CODEC                (SUNXI_GIC_START + 29)  /* AUDIO CEDEC */
#define SUNXI_IRQ_LRADC                (SUNXI_GIC_START + 30)  /* LRADC       */
#define SUNXI_IRQ_MTCACC               (SUNXI_GIC_START + 31)  /* MTCACC      */
#define SUNXI_IRQ_NMI                  (SUNXI_GIC_START + 32)  /* NMI         */
#define SUNXI_IRQ_RTIMER0              (SUNXI_GIC_START + 33)  /* R_TIMER 0   */
#define SUNXI_IRQ_RTIMER1              (SUNXI_GIC_START + 34)  /* R_TIMER 1   */

#define SUNXI_IRQ_RWATCHDOG            (SUNXI_GIC_START + 36)  /* R_WATCHDO   */
#define SUNXI_IRQ_RCIR                 (SUNXI_GIC_START + 37)  /* R_CIR       */
#define SUNXI_IRQ_RUART                (SUNXI_GIC_START + 38)  /* R_UART      */
#define SUNXI_IRQ_RP2TWI               (SUNXI_GIC_START + 39)  /* R_P2TWI     */
#define SUNXI_IRQ_RALARM0              (SUNXI_GIC_START + 40)  /* R_RLARM 0   */
#define SUNXI_IRQ_RALARM1              (SUNXI_GIC_START + 41)  /* R_RLARM 1   */

#define SUNXI_IRQ_R_1WIRE              (SUNXI_GIC_START + 43)  /* R_ONE_WIRE  */
#define SUNXI_IRQ_RTWI                 (SUNXI_GIC_START + 44)  /* R_TWI       */
#define SUNXI_IRQ_EINTL                (SUNXI_GIC_START + 45)  /* R_EINTL     */
#define SUNXI_IRQ_EINTM                (SUNXI_GIC_START + 46)  /* R_EINTM     */

#define SUNXI_IRQ_SPINLOCK             (SUNXI_GIC_START + 48)  /* SPINLOCK    */
#define SUNXI_IRQ_MBOX                 (SUNXI_GIC_START + 49)  /* M-BOX       */
#define SUNXI_IRQ_DMA                  (SUNXI_GIC_START + 50)  /* DMA         */
#define SUNXI_IRQ_HSTIMER0             (SUNXI_GIC_START + 51)  /* HSTIMER0    */
#define SUNXI_IRQ_HSTIMER1             (SUNXI_GIC_START + 52)  /* HSTIMER1    */
#define SUNXI_IRQ_HSTIMER2             (SUNXI_GIC_START + 53)  /* HSTIMER2    */
#define SUNXI_IRQ_HSTIMER3             (SUNXI_GIC_START + 54)  /* HSTIMER3    */

#define SUNXI_IRQ_TZASC                (SUNXI_GIC_START + 56)  /* TZASC       */

#define SUNXI_IRQ_VE                   (SUNXI_GIC_START + 58)  /* VE          */
#define SUNXI_IRQ_DIGMIC               (SUNXI_GIC_START + 59)  /* DIG_MIC     */
#define SUNXI_IRQ_MMC0                 (SUNXI_GIC_START + 60)  /* MMC0        */
#define SUNXI_IRQ_MMC1                 (SUNXI_GIC_START + 61)  /* MMC1        */
#define SUNXI_IRQ_MMC2                 (SUNXI_GIC_START + 62)  /* MMC2        */
#define SUNXI_IRQ_MMC3                 (SUNXI_GIC_START + 63)  /* MMC3        */

#define SUNXI_IRQ_SPI0                 (SUNXI_GIC_START + 65)  /* SPI0        */
#define SUNXI_IRQ_SPI1                 (SUNXI_GIC_START + 66)  /* SPI1        */
#define SUNXI_IRQ_SPI2                 (SUNXI_GIC_START + 67)  /* SPI2        */
#define SUNXI_IRQ_SPI3                 (SUNXI_GIC_START + 68)  /* SPI3        */
#define SUNXI_IRQ_NAND1                (SUNXI_GIC_START + 69)  /* NAND1       */
#define SUNXI_IRQ_NAND0                (SUNXI_GIC_START + 70)  /* NAND0       */

#define SUNXI_IRQ_USB_OTG              (SUNXI_GIC_START + 71)  /* USB_OTG     */
#define SUNXI_IRQ_USB_EHCI0            (SUNXI_GIC_START + 72)  /* USB_EHCI0   */
#define SUNXI_IRQ_USB_OHCI0            (SUNXI_GIC_START + 73)  /* USB_OHCI0   */
#define SUNXI_IRQ_USB_EHCI1            (SUNXI_GIC_START + 74)  /* USB_EHCI1   */
#define SUNXI_IRQ_USB_OHCI1            (SUNXI_GIC_START + 75)  /* USB_OHCI1   */

#define SUNXI_IRQ_USB_OHCI2            (SUNXI_GIC_START + 77)  /* USB_OHCI2   */

#define SUNXI_IRQ_SS                   (SUNXI_GIC_START + 80)  /* SS          */
#define SUNXI_IRQ_TS                   (SUNXI_GIC_START + 81)  /* TS          */
#define SUNXI_IRQ_GMAC                 (SUNXI_GIC_START + 82)  /* GMAC        */
#define SUNXI_IRQ_MP                   (SUNXI_GIC_START + 83)  /* MP          */
#define SUNXI_IRQ_CSI0                 (SUNXI_GIC_START + 84)  /* CSI0        */
#define SUNXI_IRQ_CSI1                 (SUNXI_GIC_START + 85)  /* CSI1        */
#define SUNXI_IRQ_LCD0                 (SUNXI_GIC_START + 86)  /* LCD0        */
#define SUNXI_IRQ_LCD1                 (SUNXI_GIC_START + 87)  /* LCD1        */
#define SUNXI_IRQ_HDMI                 (SUNXI_GIC_START + 88)  /* HDMI        */
#define SUNXI_IRQ_MIPIDSI              (SUNXI_GIC_START + 89)  /* MIPI DSI    */
#define SUNXI_IRQ_MIPICSI              (SUNXI_GIC_START + 90)  /* MIPI CSI    */
#define SUNXI_IRQ_DRC01                (SUNXI_GIC_START + 91)  /* DRC 0/1     */
#define SUNXI_IRQ_DEU01                (SUNXI_GIC_START + 92)  /* DEU 0/1     */
#define SUNXI_IRQ_DEFE0                (SUNXI_GIC_START + 93)  /* DE_FE0      */
#define SUNXI_IRQ_DEFE1                (SUNXI_GIC_START + 94)  /* DE_FE1      */
#define SUNXI_IRQ_DEBE0                (SUNXI_GIC_START + 95)  /* DE_BE0      */
#define SUNXI_IRQ_DEBE1                (SUNXI_GIC_START + 96)  /* DE_BE1      */
#define SUNXI_IRQ_GPU                  (SUNXI_GIC_START + 97)  /* GPU         */

#define SUNXI_IRQ_CTI0                 (SUNXI_GIC_START + 108) /* CTI0        */
#define SUNXI_IRQ_CTI1                 (SUNXI_GIC_START + 109) /* CTI1        */
#define SUNXI_IRQ_CTI2                 (SUNXI_GIC_START + 110) /* CTI2        */
#define SUNXI_IRQ_CTI3                 (SUNXI_GIC_START + 111) /* CTI3        */
#define SUNXI_IRQ_COMMTX0              (SUNXI_GIC_START + 112) /* COMMTX0     */
#define SUNXI_IRQ_COMMTX1              (SUNXI_GIC_START + 113) /* COMMTX1     */
#define SUNXI_IRQ_COMMTX2              (SUNXI_GIC_START + 114) /* COMMTX2     */
#define SUNXI_IRQ_COMMTX3              (SUNXI_GIC_START + 115) /* COMMTX3     */
#define SUNXI_IRQ_COMMRX0              (SUNXI_GIC_START + 116) /* COMMRX0     */
#define SUNXI_IRQ_COMMRX1              (SUNXI_GIC_START + 117) /* COMMRX1     */
#define SUNXI_IRQ_COMMRX2              (SUNXI_GIC_START + 118) /* COMMRX2     */
#define SUNXI_IRQ_COMMRX3              (SUNXI_GIC_START + 119) /* COMMRX3     */
#define SUNXI_IRQ_PMU0                 (SUNXI_GIC_START + 120) /* PMU0        */
#define SUNXI_IRQ_PMU1                 (SUNXI_GIC_START + 121) /* PMU1        */
#define SUNXI_IRQ_PMU2                 (SUNXI_GIC_START + 122) /* PMU2        */
#define SUNXI_IRQ_PMU3                 (SUNXI_GIC_START + 123) /* PMU3        */
#define SUNXI_IRQ_AXI_ERROR            (SUNXI_GIC_START + 124) /* AXI_ERROR   */

#define SUNXI_IRQ_MAX                  (SUNXI_GIC_START + 128)

#endif    /* __IRQS_SUN8I_W1P1_H */
