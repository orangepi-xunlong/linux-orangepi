// SPDX-License-Identifier: (GPL-2.0+ or MIT)

#ifndef _DT_BINDINGS_CLK_KY_X1PRO_H_
#define _DT_BINDINGS_CLK_KY_X1PRO_H_

#define CLK_DUMMY       0

#define OSC_CLK_24M     1
#define PLL_CLK_SYS     2
#define PLL_CLK_GMAC    3
#define PLL_CLK_I2S     4
#define PLL_DDR         5
#define IN_CLK_32K      6
#define PLL_CLK_400M    7

#define OSC_CLK_24M_DIV60  8
#define OSC_CLK_400K       9
#define OSC_CLK_200K       10
#define PLL_CLK_400M_2MEM  11
#define PLL_CLK_DDR        12
#define PLL_CLK_50M        13
#define PLL_CLK_10M        14
#define CLK_MEM_SYS_DIV    15
#define CLK_MEM_SYS        16
#define CLK_VPU_SYS_DIV    17
#define CLK_VPU_SYS        18
#define CLK_SEC_SYS_DIV    19
#define CLK_SEC_SYS        20
#define CLK_SERDES_SYS_DIV 21
#define CLK_SERDES_SYS     22
#define CLK_USB_SYS_DIV    23
#define CLK_USB_SYS        24
#define CLK_MBUS_SYS_DIV   25
#define CLK_MBUS_SYS       26
#define PLL_CLK_250_50M_DIV 27
#define PLL_CLK_250_50M     28
//sys
#define CLK_SYS_AHB   29
#define CLK_SYS_APB   30
#define CLK_DMAC      31
#define CLK_GPIO      32
#define CLK_WDT       33
#define CLK_PWM       34
#define CLK_BMU       35
#define CLK_SYSREG    36
#define CLK_UART0     37
#define CLK_UART1     38
#define CLK_UART2     39
#define CLK_UART3     40
#define CLK_UART4     41
#define CLK_I2C0      42
#define CLK_I2C1      43
#define CLK_I2C2      44
#define CLK_I2C3      45
#define CLK_I2C4      46
#define CLK_TIMER0    47
#define CLK_TIMER1    48
#define CLK_QSPI0     49
#define CLK_QSPI1     50
#define CLK_QSPI2     51
#define CLK_QSPI0_EN  52
#define CLK_QSPI1_EN  53
#define CLK_QSPI2_EN  54
#define CLK_CAN       55
#define CLK_CAN_EN    56
#define CLK_I2S0_EN   57
#define CLK_I2S1_EN   58
#define CLK_I2S0_MCLK 59
#define CLK_I2S1_MCLK 60
#define CLK_I2S0_BCLK 61
#define CLK_I2S1_BCLK 62
#define CLK_I2S0_MCLK_OUT 63
#define CLK_I2S1_MCLK_OUT 64
//mem
#define CLK_SDIO_DIV  65
#define CLK_EMMC_DIV  66
#define CLK_TMCLK_10M 67
#define CLK_SDIO_SEL  68
#define CLK_EMMC_SEL  69
#define CLK_MEM_HCLK  70
#define CLK_SDIO_HCLK 71
#define CLK_SDIO_ACLK 72
#define CLK_SDIO_TMCLK  73
#define CLK_SDIO_CCLK   74
#define CLK_EMMC_HCLK   75
#define CLK_EMMC_ACLK   76
#define CLK_EMMC_TMCLK  77
#define CLK_EMMC_CCLK   78
#define CLK_MEM_AHB     79
//serdes
#define CLK_SERDES_AXI  80
#define CLK_SERDES_APB  81
#define CLK_SERDES2MBUS 82
#define CLK_PCIE0_EN    83
#define CLK_PCIE1_EN    84
#define CLK_PCIE2_EN    85
#define CLK_PCIE3_EN    86
#define CLK_PCIE4_EN    87
#define CLK_PCIE5_EN    88
#define CLK_SATA_EN     89
#define CLK_SATA0_EN    90
#define CLK_SATA1_EN    91
#define CLK_SATA2_EN    92
#define CLK_SATA3_EN    93
#define CLK_USB31_BUS_CLK_EARLY0  94
#define CLK_USB31_BUS_CLK_EARLY1  95
#define CLK_XGMII_PCLK0      96
#define CLK_XGMII_PCLK1      97
#define CLK_COMBO_PHY_PCLK0  98
#define CLK_COMBO_PHY_PCLK1  99
//vpu
#define CLK_VPU       100
#define CLK_VPU2VBUS  101
//sec
#define CLK_SEC_AHB   102
#define CLK_SEC_SYS_DIV2  103
#define CLK_SEC_APB   104
#define CLK_SEC_TRNG  105
#define CLK_SEC_PKE   106
#define CLK_SEC_HASH  107
#define CLK_SEC_SKE   108
#define CLK_SEC_EFUSE 109
//gmac
#define CLK_USB_AHB       110
#define CLK_USB_AHB_DIV2  111
#define CLK_USB_SYS_DIV2  112
#define CLK_USB20_OTG_EN  113
#define CLK_USB20_HOST_EN 114
#define CLK_USB31_DRD_EN  115

#define CLK_GMAC_CSR            118
#define CLK_USB2VBUS            119
#define CLK_GMAC_PHYCLK_OUT     120
//mbus
#define CLK_DDRC_APB   121
//mcu
#define CLK_MCU_SRC    122
#define CLK_MCU_SYS    123
#define CLK_MCU2MBUS   124
#define CLK_MCU_AHB    125
#define CLK_MCU_APB    126
#define CLK_MBOX       127
#define CLK_SPINLOCK   128

#define CLK_MAX_NO     129
#endif /* _DT_BINDINGS_CLK_KY_X1PRO_H_ */
