/*
 * arch/arm/mach-sunxi/include/mach/sun9i/platform-sun9iw1p1.h
 *
 * Copyright(c) 2013-2015 Allwinnertech Co., Ltd.
 *      http://www.allwinnertech.com
 *
 * Author: liugang <liugang@allwinnertech.com>
 *
 * sun9i platform header file
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef __PLATFORM_SUN9I_W1P1_H
#define __PLATFORM_SUN9I_W1P1_H

/*
 * memory definitions
 */
#define SUNXI_BROM0_N_PBASE                       0x00000000
#define SUNXI_BROM0_N_SIZE                        0x00008000
#define SUNXI_BROM1_S_PBASE                       0x00000000
#define SUNXI_BROM1_S_SIZE                        0x00010000
#define SUNXI_SRAM_A1_PBASE                       0x00010000
#define SUNXI_SRAM_A1_SIZE                        0x0000a000
#define SUNXI_SRAM_A2_PBASE                       0x08100000
#define SUNXI_SRAM_A2_SIZE                        0x00028000
#define SUNXI_SRAM_B_PBASE                        0x00020000
#define SUNXI_SRAM_B_SIZE                         0x00040000
#define SUNXI_SRAM_C_PBASE                        0x01d00000
#define SUNXI_SRAM_C_SIZE                         0x00100000
/* memory not need iomap */
#define SUNXI_DRAM_PBASE                          0x20000000 /* 0x20000000 ~ 0x220000000(size: 8G) */
#define SUNXI_GPU_PBASE                           0x02000000
#define SUNXI_GPU_SIZE                            0x01000000

/*
 * device definitions
 */
#define SUNXI_CORE_DEBUG_PBASE                    0x01400000
#define SUNXI_CORE_DEBUG_SIZE                     0x00020000
#define SUNXI_TSGEN_RO_PBASE                      0x01506000
#define SUNXI_TSGEN_RO_SIZE                       0x00001000
#define SUNXI_TSGEN_CTRL_PBASE                    0x01607000
#define SUNXI_TSGEN_CTRL_SIZE                     0x00001000
#define SUNXI_R_CPUCFG_PBASE                      0x01700000
#define SUNXI_TIMESTAMP_STA_PBASE                 0x01710000
#define SUNXI_TIMESTAMP_CTRL_PBASE                0x01720000
#define SUNXI_R_CPUCFG_SIZE                       0x00000400
/* ahb0 device */
#define SUNXI_SS_PBASE                            0x01c02000
#define SUNXI_NFC0_PBASE                          0x01c03000
#define SUNXI_NFC1_PBASE                          0x01c04000
#define SUNXI_TS_PBASE                            0x01c06000
#define SUNXI_GPU_CTRL_PBASE                      0x01c08000
#define SUNXI_GTBUS_PBASE                         0x01c09000
#define SUNXI_SATA_PBASE                          0x01c0a000
#define SUNXI_SID_PBASE                           0x01c0e000
#define SUNXI_SDMMC0_PBASE                        0x01c0f000
#define SUNXI_SDMMC1_PBASE                        0x01c10000
#define SUNXI_SDMMC2_PBASE                        0x01c11000
#define SUNXI_SDMMC3_PBASE                        0x01c12000
#define SUNXI_SPI0_PBASE                          0x01c1a000
#define SUNXI_SPI1_PBASE                          0x01c1b000
#define SUNXI_SPI2_PBASE                          0x01c1c000
#define SUNXI_SPI3_PBASE                          0x01c1d000
#define SUNXI_MIPI_HSI_PBASE                      0x01c1f000
#define SUNXI_GIC_PBASE                           0x01c40000
#define SUNXI_GIC_DIST_PBASE                      0x01c41000
#define SUNXI_GIC_CPU_PBASE                       0x01c42000
#define SUNXI_DRAMCOM_PBASE                       0x01c62000
#define SUNXI_DRAMCTRL0_PBASE                     0x01c63000
#define SUNXI_DRAMCTRL1_PBASE                     0x01c64000
#define SUNXI_DRAMPHY0_PBASE                      0x01c65000
#define SUNXI_DRAMPHY1_PBASE                      0x01c66000
#define SUNXI_CCI400_PBASE                        0x01c90000
/* ahb1 device */
#define SUNXI_SYS_CTRL_PBASE                      0x00800000
#define SUNXI_HSTMR0_3_PBASE                      0x00801000
#define SUNXI_DMA_PBASE                           0x00802000
#define SUNXI_MSGBOX_PBASE                        0x00803000
#define SUNXI_SPINLOCK_PBASE                      0x00804000
#define SUNXI_TZASC_PBASE                         0x00805000
#define SUNXI_USB_HCI0_PBASE                      0x00a00000
#define SUNXI_USB_HCI1_PBASE                      0x00a01000
#define SUNXI_USB_HCI2_PBASE                      0x00a02000
#define SUNXI_USB_CTRL_PBASE                      0x00a08000
#define SUNXI_GMAC_PBASE                          0x00830000
/* ahb2 device */
#define SUNXI_DE_SYS_PBASE                        0x03000000
#define SUNXI_DISP_SYS_PBASE                      0x03010000
#define SUNXI_FE0_PBASE                           0x03100000
#define SUNXI_FE1_PBASE                           0x03140000
#define SUNXI_FE2_PBASE                           0x03180000
#define SUNXI_BE0_PBASE                           0x03200000
#define SUNXI_BE1_PBASE                           0x03240000
#define SUNXI_BE2_PBASE                           0x03280000
#define SUNXI_DEU0_PBASE                          0x03300000
#define SUNXI_DEU1_PBASE                          0x03340000
#define SUNXI_DRC0_PBASE                          0x03400000
#define SUNXI_DRC1_PBASE                          0x03440000
#define SUNXI_CSI_PBASE                           0x03800000
#define SUNXI_CSI0_PBASE                          0x03800000
#define SUNXI_MIPI_CSI0_PBASE                     0x03801000
#define SUNXI_MIPI_CSI0_PHY_PBASE                 0x03802000
#define SUNXI_CSI0_CCI_PBASE                      0x03803000
#define SUNXI_ISP_PBASE                           0x03808000
#define SUNXI_CSI1_PBASE                          0x03900000
#define SUNXI_CSI1_CCI_PBASE                      0x03903000
#define SUNXI_FD_IO_PBASE                         0x03a00000
#define SUNXI_FD_MEM_PBASE                        0x03a10000
#define SUNXI_VE_PBASE                            0x03a40000
#define SUNXI_LCD0_PBASE                          0x03c00000
#define SUNXI_LCD1_PBASE                          0x03c10000
#define SUNXI_LCD2_PBASE                          0x03c20000
#define SUNXI_MP_PBASE                            0x03f00000
/* apb0 device */
#define SUNXI_CCM_PLL_PBASE                       0x06000000
#define SUNXI_CCM_MOD_PBASE                       0x06000400
#define SUNXI_PIO_PBASE                           0x06000800
#define SUNXI_TIMER_PBASE                         0x06000c00
#define SUNXI_SPDIF_PBASE                         0x06001000
#define SUNXI_PWM_PBASE                           0x06001400
#define SUNXI_LRADC01_PBASE                       0x06001800
#define SUNXI_AC97_PBASE                          0x06001c00
#define SUNXI_DAUDIO0_PBASE                       0x06002000
#define SUNXI_DAUDIO1_PBASE                       0x06002400
#define SUNXI_TWD_PBASE                           0x06003000
#define SUNXI_TZPC_PBASE                          0x06003400
#define SUNXI_SJTAG_PBASE                         0x06003c00
#define SUNXI_CIR_TX_PBASE                        0x06004400
#define SUNXI_GPADC_PBASE                         0x06004c00
/* apb1 device */
#define SUNXI_UART0_PBASE                         0x07000000
#define SUNXI_UART1_PBASE                         0x07000400
#define SUNXI_UART2_PBASE                         0x07000800
#define SUNXI_UART3_PBASE                         0x07000c00
#define SUNXI_UART4_PBASE                         0x07001000
#define SUNXI_UART5_PBASE                         0x07001400
#define SUNXI_TWI0_PBASE                          0x07002800
#define SUNXI_TWI1_PBASE                          0x07002c00
#define SUNXI_TWI2_PBASE                          0x07003000
#define SUNXI_TWI3_PBASE                          0x07003400
#define SUNXI_TWI4_PBASE                          0x07003800
/* cpus ahb/apb */
#define SUNXI_R_TIMER_PBASE                       0x08000800
#define SUNXI_R_NVIC_PBASE                        0x08000c00
#define SUNXI_R_WDOG_PBASE                        0x08001000
#define SUNXI_R_PRCM_PBASE                        0x08001400
#define SUNXI_R_CNT_PBASE                         0x08001c00
#define SUNXI_R_CIR_RX_PBASE                      0x08002000
#define SUNXI_R_TWI0_PBASE                        0x08002400
#define SUNXI_R_UART_PBASE                        0x08002800
#define SUNXI_R_PIO_PBASE                         0x08002c00
#define SUNXI_R_ONE_WIRE_PBASE                    0x08003000
#define SUNXI_R_RSB_PBASE                         0x08003400
#define SUNXI_R_TWI1_PBASE                        0x08003800
#define SUNXI_R_PS2_0_PBASE                       0x08004000
#define SUNXI_R_PS2_1_PBASE                       0x08004400
#define SUNXI_R_DAUDIO_0_PBASE                    0x08006000
#define SUNXI_R_DAUDIO_1_PBASE                    0x08007000
#define SUNXI_R_DMA_PBASE                         0x08008000

/*
 * virt address definitions
 */
#define SUNXI_BROM0_N_VBASE                       IO_ADDRESS(SUNXI_BROM0_N_PBASE        )
#define SUNXI_BROM1_S_VBASE                       IO_ADDRESS(SUNXI_BROM1_S_PBASE        )
#define SUNXI_SRAM_A1_VBASE                       IO_ADDRESS(SUNXI_SRAM_A1_PBASE        )
#define SUNXI_SRAM_A2_VBASE                       IO_ADDRESS(SUNXI_SRAM_A2_PBASE        )
#define SUNXI_SRAM_B_VBASE                        IO_ADDRESS(SUNXI_SRAM_B_PBASE         )
#define SUNXI_SRAM_C_VBASE                        IO_ADDRESS(SUNXI_SRAM_C_PBASE         )
#define SUNXI_CORE_DEBUG_VBASE                    IO_ADDRESS(SUNXI_CORE_DEBUG_PBASE     )
#define SUNXI_TSGEN_RO_VBASE                      IO_ADDRESS(SUNXI_TSGEN_RO_PBASE       )
#define SUNXI_TSGEN_CTRL_VBASE                    IO_ADDRESS(SUNXI_TSGEN_CTRL_PBASE     )
#define SUNXI_R_CPUCFG_VBASE                      IO_ADDRESS(SUNXI_R_CPUCFG_PBASE       )
#define SUNXI_SS_VBASE                            IO_ADDRESS(SUNXI_SS_PBASE             )
#define SUNXI_NFC0_VBASE                          IO_ADDRESS(SUNXI_NFC0_PBASE           )
#define SUNXI_NFC1_VBASE                          IO_ADDRESS(SUNXI_NFC1_PBASE           )
#define SUNXI_TS_VBASE                            IO_ADDRESS(SUNXI_TS_PBASE             )
#define SUNXI_GPU_CTRL_VBASE                      IO_ADDRESS(SUNXI_GPU_CTRL_PBASE       )
#define SUNXI_GTBUS_VBASE                         IO_ADDRESS(SUNXI_GTBUS_PBASE          )
#define SUNXI_SATA_VBASE                          IO_ADDRESS(SUNXI_SATA_PBASE           )
#define SUNXI_SID_VBASE                           IO_ADDRESS(SUNXI_SID_PBASE            )
#define SUNXI_VE_VBASE                            IO_ADDRESS(SUNXI_VE_PBASE             )
#define SUNXI_SDMMC0_VBASE                        IO_ADDRESS(SUNXI_SDMMC0_PBASE         )
#define SUNXI_SDMMC1_VBASE                        IO_ADDRESS(SUNXI_SDMMC1_PBASE         )
#define SUNXI_SDMMC2_VBASE                        IO_ADDRESS(SUNXI_SDMMC2_PBASE         )
#define SUNXI_SDMMC3_VBASE                        IO_ADDRESS(SUNXI_SDMMC3_PBASE         )
#define SUNXI_SPI0_VBASE                          IO_ADDRESS(SUNXI_SPI0_PBASE           )
#define SUNXI_SPI1_VBASE                          IO_ADDRESS(SUNXI_SPI1_PBASE           )
#define SUNXI_SPI2_VBASE                          IO_ADDRESS(SUNXI_SPI2_PBASE           )
#define SUNXI_SPI3_VBASE                          IO_ADDRESS(SUNXI_SPI3_PBASE           )
#define SUNXI_MIPI_HSI_VBASE                      IO_ADDRESS(SUNXI_MIPI_HSI_PBASE       )
#define SUNXI_GIC_VBASE                           IO_ADDRESS(SUNXI_GIC_PBASE            )
#define SUNXI_GIC_DIST_VBASE                      IO_ADDRESS(SUNXI_GIC_DIST_PBASE       )
#define SUNXI_GIC_CPU_VBASE                       IO_ADDRESS(SUNXI_GIC_CPU_PBASE        )
#define SUNXI_DRAMCOM_VBASE                       IO_ADDRESS(SUNXI_DRAMCOM_PBASE        )
#define SUNXI_DRAMCTRL0_VBASE                     IO_ADDRESS(SUNXI_DRAMCTRL0_PBASE      )
#define SUNXI_DRAMCTRL1_VBASE                     IO_ADDRESS(SUNXI_DRAMCTRL1_PBASE      )
#define SUNXI_DRAMPHY0_VBASE                      IO_ADDRESS(SUNXI_DRAMPHY0_PBASE       )
#define SUNXI_DRAMPHY1_VBASE                      IO_ADDRESS(SUNXI_DRAMPHY1_PBASE       )
#define SUNXI_CCI400_VBASE                        IO_ADDRESS(SUNXI_CCI400_PBASE         )
#define SUNXI_SYS_CTRL_VBASE                      IO_ADDRESS(SUNXI_SYS_CTRL_PBASE       )
#define SUNXI_HSTMR0_3_VBASE                      IO_ADDRESS(SUNXI_HSTMR0_3_PBASE       )
#define SUNXI_DMA_VBASE                           IO_ADDRESS(SUNXI_DMA_PBASE            )
#define SUNXI_MSGBOX_VBASE                        IO_ADDRESS(SUNXI_MSG_BOX_PBASE        )
#define SUNXI_SPINLOCK_VBASE                      IO_ADDRESS(SUNXI_SPINLOCK_PBASE       )
#define SUNXI_TZASC_VBASE                         IO_ADDRESS(SUNXI_TZASC_PBASE          )
#define SUNXI_USB_HCI0_VBASE                      IO_ADDRESS(SUNXI_USB_HCI0_PBASE       )
#define SUNXI_USB_HCI1_VBASE                      IO_ADDRESS(SUNXI_USB_HCI1_PBASE       )
#define SUNXI_USB_HCI2_VBASE                      IO_ADDRESS(SUNXI_USB_HCI2_PBASE       )
#define SUNXI_USB_CTRL_VBASE                      IO_ADDRESS(SUNXI_USB_CTRL_PBASE       )
#define SUNXI_GMAC_VBASE                          IO_ADDRESS(SUNXI_GMAC_PBASE           )
#define SUNXI_DE_SYS_VBASE                        IO_ADDRESS(SUNXI_DE_SYS_PBASE         )
#define SUNXI_DISP_SYS_VBASE                      IO_ADDRESS(SUNXI_DISP_SYS_PBASE       )
#define SUNXI_FE0_VBASE                           IO_ADDRESS(SUNXI_FE0_PBASE            )
#define SUNXI_FE1_VBASE                           IO_ADDRESS(SUNXI_FE1_PBASE            )
#define SUNXI_FE2_VBASE                           IO_ADDRESS(SUNXI_FE2_PBASE            )
#define SUNXI_BE0_VBASE                           IO_ADDRESS(SUNXI_BE0_PBASE            )
#define SUNXI_BE1_VBASE                           IO_ADDRESS(SUNXI_BE1_PBASE            )
#define SUNXI_BE2_VBASE                           IO_ADDRESS(SUNXI_BE2_PBASE            )
#define SUNXI_DEU0_VBASE                          IO_ADDRESS(SUNXI_DEU0_PBASE           )
#define SUNXI_DEU1_VBASE                          IO_ADDRESS(SUNXI_DEU1_PBASE           )
#define SUNXI_DRC0_VBASE                          IO_ADDRESS(SUNXI_DRC0_PBASE           )
#define SUNXI_DRC1_VBASE                          IO_ADDRESS(SUNXI_DRC1_PBASE           )
#define SUNXI_CSI_VBASE                           IO_ADDRESS(SUNXI_CSI_PBASE            )
#define SUNXI_CSI0_VBASE                          IO_ADDRESS(SUNXI_CSI0_PBASE           )
#define SUNXI_MIPI_CSI0_VBASE                     IO_ADDRESS(SUNXI_MIPI_CSI0_PBASE      )
#define SUNXI_MIPI_CSI0_PHY_VBASE                 IO_ADDRESS(SUNXI_MIPI_CSI0_PHY_PBASE  )
#define SUNXI_CSI0_CCI_VBASE                      IO_ADDRESS(SUNXI_CSI0_CCI_PBASE       )
#define SUNXI_ISP_VBASE                           IO_ADDRESS(SUNXI_ISP_PBASE            )
#define SUNXI_CSI1_VBASE                          IO_ADDRESS(SUNXI_CSI1_PBASE           )
#define SUNXI_CSI1_CCI_VBASE                      IO_ADDRESS(SUNXI_CSI1_CCI_PBASE       )
#define SUNXI_FD_IO_VBASE                         IO_ADDRESS(SUNXI_FD_IO_PBASE          )
#define SUNXI_FD_MEM_VBASE                        IO_ADDRESS(SUNXI_FD_MEM_PBASE         )
#define SUNXI_LCD0_VBASE                          IO_ADDRESS(SUNXI_LCD0_PBASE           )
#define SUNXI_LCD1_VBASE                          IO_ADDRESS(SUNXI_LCD1_PBASE           )
#define SUNXI_LCD2_VBASE                          IO_ADDRESS(SUNXI_LCD2_PBASE           )
#define SUNXI_MP_VBASE                            IO_ADDRESS(SUNXI_MP_PBASE             )
#define SUNXI_CCM_PLL_VBASE                       IO_ADDRESS(SUNXI_CCM_PLL_PBASE        )
#define SUNXI_CCM_MOD_VBASE                       IO_ADDRESS(SUNXI_CCM_MOD_PBASE        )
#define SUNXI_PIO_VBASE                           IO_ADDRESS(SUNXI_PIO_PBASE            )
#define SUNXI_TIMER_VBASE                         IO_ADDRESS(SUNXI_TIMER_PBASE          )
#define SUNXI_SPDIF_VBASE                         IO_ADDRESS(SUNXI_SPDIF_PBASE          )
#define SUNXI_PWM_VBASE                           IO_ADDRESS(SUNXI_PWM_PBASE           )
#define SUNXI_LRADC01_VBASE                       IO_ADDRESS(SUNXI_LRADC01_PBASE        )
#define SUNXI_AC97_VBASE                          IO_ADDRESS(SUNXI_AC97_PBASE           )
#define SUNXI_DAUDIO0_VBASE                       IO_ADDRESS(SUNXI_DAUDIO0_PBASE        )
#define SUNXI_DAUDIO1_VBASE                       IO_ADDRESS(SUNXI_DAUDIO1_PBASE        )
#define SUNXI_TWD_VBASE                           IO_ADDRESS(SUNXI_TWD_PBASE            )
#define SUNXI_TZPC_VBASE                          IO_ADDRESS(SUNXI_TZPC_PBASE           )
#define SUNXI_SID_VBASE                           IO_ADDRESS(SUNXI_SID_PBASE            )
#define SUNXI_SJTAG_VBASE                         IO_ADDRESS(SUNXI_SJTAG_PBASE          )
#define SUNXI_CIR_TX_VBASE                        IO_ADDRESS(SUNXI_CIR_TX_PBASE         )
#define SUNXI_GPADC_VBASE                         IO_ADDRESS(SUNXI_GPADC_PBASE          )
#define SUNXI_UART0_VBASE                         IO_ADDRESS(SUNXI_UART0_PBASE          )
#define SUNXI_UART1_VBASE                         IO_ADDRESS(SUNXI_UART1_PBASE          )
#define SUNXI_UART2_VBASE                         IO_ADDRESS(SUNXI_UART2_PBASE          )
#define SUNXI_UART3_VBASE                         IO_ADDRESS(SUNXI_UART3_PBASE          )
#define SUNXI_UART4_VBASE                         IO_ADDRESS(SUNXI_UART4_PBASE          )
#define SUNXI_UART5_VBASE                         IO_ADDRESS(SUNXI_UART5_PBASE          )
#define SUNXI_TWI0_VBASE                          IO_ADDRESS(SUNXI_TWI0_PBASE           )
#define SUNXI_TWI1_VBASE                          IO_ADDRESS(SUNXI_TWI1_PBASE           )
#define SUNXI_TWI2_VBASE                          IO_ADDRESS(SUNXI_TWI2_PBASE           )
#define SUNXI_TWI3_VBASE                          IO_ADDRESS(SUNXI_TWI3_PBASE           )
#define SUNXI_TWI4_VBASE                          IO_ADDRESS(SUNXI_TWI4_PBASE           )
#define SUNXI_R_TIMER_VBASE                       IO_ADDRESS(SUNXI_R_TIMER_PBASE        )
#define SUNXI_R_NVIC_VBASE                        IO_ADDRESS(SUNXI_R_NVIC_PBASE         )
#define SUNXI_R_WDOG_VBASE                        IO_ADDRESS(SUNXI_R_WDOG_PBASE         )
#define SUNXI_R_PRCM_VBASE                        IO_ADDRESS(SUNXI_R_PRCM_PBASE         )
#define SUNXI_R_CNT_VBASE                         IO_ADDRESS(SUNXI_R_CNT_PBASE          )
#define SUNXI_R_CIR_RX_VBASE                      IO_ADDRESS(SUNXI_R_CIR_RX_PBASE       )
#define SUNXI_R_TWI0_VBASE                        IO_ADDRESS(SUNXI_R_TWI0_PBASE         )
#define SUNXI_R_UART_VBASE                        IO_ADDRESS(SUNXI_R_UART_PBASE         )
#define SUNXI_R_PIO_VBASE                         IO_ADDRESS(SUNXI_R_PIO_PBASE          )
#define SUNXI_R_ONE_WIRE_VBASE                    IO_ADDRESS(SUNXI_R_ONE_WIRE_PBASE     )
#define SUNXI_R_RSB_VBASE                         IO_ADDRESS(SUNXI_R_RSB_PBASE          )
#define SUNXI_R_TWI1_VBASE                        IO_ADDRESS(SUNXI_R_TWI1_PBASE         )
#define SUNXI_R_PS2_0_VBASE                       IO_ADDRESS(SUNXI_R_PS2_0_PBASE        )
#define SUNXI_R_PS2_1_VBASE                       IO_ADDRESS(SUNXI_R_PS2_1_PBASE        )
#define SUNXI_R_DAUDIO_0_VBASE                    IO_ADDRESS(SUNXI_R_DAUDIO_0_PBASE     )
#define SUNXI_R_DAUDIO_1_VBASE                    IO_ADDRESS(SUNXI_R_DAUDIO_1_PBASE     )
#define SUNXI_R_DMA_VBASE                         IO_ADDRESS(SUNXI_R_DMA_PBASE          )

/*
 * io range definitions for iomap
 */
#define SUNXI_IO_CORE_DEBUG_PBASE                 0x01400000
#define SUNXI_IO_CORE_DEBUG_SIZE                  0x00020000
#define SUNXI_IO_TSGEN_RO_PBASE                   0x01506000
#define SUNXI_IO_TSGEN_RO_SIZE                    0x00001000
#define SUNXI_IO_TSGEN_CTRL_PBASE                 0x01607000
#define SUNXI_IO_TSGEN_CTRL_SIZE                  0x00001000
#define SUNXI_IO_R_CPUCFG_PBASE                   0x01700000
#define SUNXI_IO_R_CPUCFG_SIZE                    0x00010000
#define SUNXI_IO_TIMESTAMP_PBASE                  0x01710000
#define SUNXI_IO_TIMESTAMP_SIZE                   0x00020000
#define SUNXI_IO_AHB0_PBASE                       0x01c00000 /* start: 0x01c02000  */
#define SUNXI_IO_AHB0_SIZE                        0x000a0000 /* end:   0x01c90000+ */
#define SUNXI_IO_AHB1_PBASE                       0x00800000 /* start: 0x00800000  */
#define SUNXI_IO_AHB1_SIZE                        0x00300000 /* end:   0x00afffff+ */
#define SUNXI_IO_AHB2_PBASE                       0x03000000 /* start: 0x03000000  */
#define SUNXI_IO_AHB2_SIZE                        0x01000000 /* end:   0x03f00000+ */
#define SUNXI_IO_APB0_PBASE                       0x06000000 /* start: 0x06000000  */
#define SUNXI_IO_APB0_SIZE                        0x00010000 /* end:   0x06004c00+ */
#define SUNXI_IO_APB1_PBASE                       0x07000000 /* start: 0x07000000  */
#define SUNXI_IO_APB1_SIZE                        0x00010000 /* end:   0x07003800+ */
#define SUNXI_IO_CPUS_PBASE                       0x08000800 /* start: 0x08000800  */
#define SUNXI_IO_CPUS_SIZE                        0x00010000 /* end:   0x08008000+ */
#define SUNXI_IO_CORE_DEBUG_VBASE                 IO_ADDRESS(SUNXI_IO_CORE_DEBUG_PBASE  )
#define SUNXI_IO_TSGEN_RO_VBASE                   IO_ADDRESS(SUNXI_IO_TSGEN_RO_PBASE    )
#define SUNXI_IO_TSGEN_CTRL_VBASE                 IO_ADDRESS(SUNXI_IO_TSGEN_CTRL_PBASE  )
#define SUNXI_IO_TIMESTAMP_VBASE                  IO_ADDRESS(SUNXI_IO_TIMESTAMP_PBASE   )
#define SUNXI_IO_R_CPUCFG_VBASE                   IO_ADDRESS(SUNXI_IO_R_CPUCFG_PBASE    )
#define SUNXI_IO_AHB0_VBASE                       IO_ADDRESS(SUNXI_IO_AHB0_PBASE        )
#define SUNXI_IO_AHB1_VBASE                       IO_ADDRESS(SUNXI_IO_AHB1_PBASE        )
#define SUNXI_IO_AHB2_VBASE                       IO_ADDRESS(SUNXI_IO_AHB2_PBASE        )
#define SUNXI_IO_APB0_VBASE                       IO_ADDRESS(SUNXI_IO_APB0_PBASE        )
#define SUNXI_IO_APB1_VBASE                       IO_ADDRESS(SUNXI_IO_APB1_PBASE        )
#define SUNXI_IO_CPUS_VBASE                       IO_ADDRESS(SUNXI_IO_CPUS_PBASE        )

/*
 * timer registers
 */
#define SUNXI_TMR_IRQ_EN_REG                      0x0000
#define SUNXI_TMR_IRQ_STA_REG                     0x0004
#define SUNXI_TMR0_CTRL_REG                       0x0010
#define SUNXI_TMR0_INTV_VALUE_REG                 0x0014
#define SUNXI_TMR0_CUR_VALUE_REG                  0x0018

#define SUNXI_AVS_CNT_CTL_REG                     0x0080
#define SUNXI_AVS_CNT0_REG                        0x0084
#define SUNXI_AVS_CNT1_REG                        0x0088
#define SUNXI_AVS_CNT_DIV_REG                     0x008c

#define SUNXI_WDOG1_IRQ_EN_REG                    0xa0
#define SUNXI_WDOG1_IRQ_STA_REG                   0xa4
#define SUNXI_WDOG1_CTRL_REG                      0xb0
#define SUNXI_WDOG1_CFG_REG                       0xb4
#define SUNXI_WDOG1_MODE_REG                      0xb8

/* r-watchdog0 reg offset define */
#define R_WDOG_IRQ_EN_REG                         0x0
#define R_WDOG_IRQ_STA_REG                        0x4
#define R_WDOG_CTRL_REG                           0x10
#define R_WDOG_CFG_REG                            0x14
#define R_WDOG_MODE_REG                           0x18

/*
 * cpucfg
 */
#define C0_CTRL_REG0                              0x0000
#define C0_CTRL_REG1                              0x0004
#define C0_ADB400_PWRDNREQN_REG                   0x0008
#define C0_EMA_REG                                0x000c
#define C1_CTRL_REG0                              0x0010
#define C1_CTRL_REG1                              0x0014
#define C1_ADB400_PWRDNREQN_REG                   0x0018
#define C1_EMA_REG                                0x001c
#define DBG_REG0                                  0x0020
#define CNT_FORCE_SYNC_REG                        0x0024
#define GENER_CTRL_REG0                           0x0028
#define GENER_CTRL_REG1                           0x002c
#define C0_CPU_STATUS                             0x0030
#define C1_CPU_STATUS                             0x0034
#define DBG_REG1                                  0x0038
#define C0_RST_CTRL                               0x0080
#define C1_RST_CTRL                               0x0084
#define GIC_RST_CTRL                              0x0088
#define SUNXI_CLUSTER_CPU_STATUS(cluster)         (0x30 + (cluster)*0x4)
#define SUNXI_CPU_RST_CTRL(cluster)               (0x80 + (cluster)*0x4)
#define SUNXI_CLUSTER_CTRL0(cluster)              (0x00 + (cluster)*0x10)
#define SUNXI_CLUSTER_CTRL1(cluster)              (0x04 + (cluster)*0x10)

/*
 * r-prcm
 */
#define CPUS_RST_CTRL                             0x0000
#define C0_CPU_RST_CTRL                           0x0004
#define C1_CPU_RST_CTRL                           0x0008
#define CLUSTER_RST_CTRL                          0x000c
#define CPUS_CLK_CFG_REG                          0x0010
#define APBS_CLK_DIV_REG                          0x001c
#define APBS_GATING_REG                           0x0028
#define PLL_CTRL_REG0                             0x0040
#define PLL_CTRL_REG1                             0x0044
#define R_ONE_WIRE_CLK_REG                        0x0050
#define R_CIR_RX_CLK_REG                          0x0054
#define R_DAUDIO_CLK_REG                          0x0058
#define R_IIS_CLK_REG                             0x005c
#define APBS_MODULE_RST_REG                       0x00b0
#define C0_CPU_PWROFF_GATING                      0x0100
#define C1_CPU_PWROFF_GATING                      0x0104
#define VDD_SYS_PWROFF_GATING                     0x0110
#define GPU_PWROFF_GATING                         0x0118
#define VDD_SYS_PWR_RST                           0x0120
#define C0_CPU1_PWR_SWITCH                        0x0144
#define C0_CPU2_PWR_SWITCH                        0x0148
#define C0_CPU3_PWR_SWITCH                        0x014c
#define C1_CPU1_PWR_SWITCH                        0x0154
#define C1_CPU2_PWR_SWITCH                        0x0158
#define C1_CPU3_PWR_SWITCH                        0x015c
#define SUP_STAN_FLAG_REG                         0x0160
#define PRIVATE_REG0                              0x0164
#define PRIVATE_REG1                              0x0168
#define CPU_TEST_REG                              0x0170
#define RAM_CFG_REG                               0x0180
#define RAM_TEST_REG                              0x0190
#define NMI_INIT_CTRL_REG                         0x01a0
#define NMI_IRQ_EN_REG                            0x01a4
#define NMI_IRQ_STATUS                            0x01a8
#define R_PLL_AUDIO_CFG_REG                       0x01c0
#define R_PLL_AUDIO_BIAS_REG                      0x01c4
#define R_PLL_AUDIO_PAT_CFG_REG                   0x01c8
#define AUDIO_PLL_CTRL_REG                        0x01cc
#define R_PIO_HOLD_CTRL_REG                       0x01f0
#define SUNXI_CPU_PWR_CLAMP(cluster, cpu)         (0x140 + (cluster*4 + cpu)*0x04)
#define SUNXI_CPU_PWR_CLAMP_STATUS(cluster, cpu)  (0x64  + (cluster*4 + cpu)*0x40)
#define SUNXI_CLUSTER_PWROFF_GATING(cluster)      (0x100 + (cluster)*0x04)
#define SUNXI_CLUSTER_PWRON_RESET(cluster)        (0x04  + (cluster)*0x04)

/*
 * uart
 */
#define SUNXI_UART_RBR                            0x00 /* Receive Buffer Register */
#define SUNXI_UART_THR                            0x00 /* Transmit Holding Register */
#define SUNXI_UART_DLL                            0x00 /* Divisor Latch Low Register */
#define SUNXI_UART_DLH                            0x04 /* Diviso Latch High Register */
#define SUNXI_UART_IER                            0x04 /* Interrupt Enable Register */
#define SUNXI_UART_IIR                            0x08 /* Interrrupt Identity Register */
#define SUNXI_UART_FCR                            0x08 /* FIFO Control Register */
#define SUNXI_UART_LCR                            0x0c /* Line Control Register */
#define SUNXI_UART_MCR                            0x10 /* Modem Control Register */
#define SUNXI_UART_LSR                            0x14 /* Line Status Register */
#define SUNXI_UART_MSR                            0x18 /* Modem Status Register */
#define SUNXI_UART_SCH                            0x1c /* Scratch Register */
#define SUNXI_UART_USR                            0x7c /* Status Register */
#define SUNXI_UART_TFL                            0x80 /* Transmit FIFO Level */
#define SUNXI_UART_RFL                            0x84 /* RFL */
#define SUNXI_UART_HALT                           0xa4 /* Halt TX Register */

#define UART_USR                                  (SUNXI_UART_USR >> 2)
#define UART_HALT                                 (SUNXI_UART_HALT >> 2)
#define UART_SCH                                  (SUNXI_UART_SCH >> 2)
#define UART_FORCE_CFG                            (1 << 1)
#define UART_FORCE_UPDATE                         (1 << 2)

#define SUNXI_R_UART_LOG(fmt, args...)            \
do {                                              \
	printk("[%s]"fmt"\n", __func__, ##args);  \
}while(0)

#endif /* __PLATFORM_SUN9I_W1P1_H */
