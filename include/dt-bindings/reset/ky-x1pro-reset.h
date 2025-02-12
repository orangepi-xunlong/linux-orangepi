// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef __DT_BINDINGS_K1_PRO_RST_H__
#define __DT_BINDINGS_K1_PRO_RST_H__

//000
#define RESET_CPUC0C1       0
#define RESET_CPUC0C2       1
#define RESET_CPUC0C3       2
#define RESET_CPUC0TCM      3
//004
#define RESET_CPUC1APB  4
#define RESET_CPUC1L2C  5
#define RESET_CPUC1PIC  6
#define RESET_CPUC1ACEM 7
#define RESET_CPUC1C0   8
#define RESET_CPUC1C1   9
#define RESET_CPUC1C2   10
#define RESET_CPUC1C3   11
//008
#define RESET_CPU_CCI   12
//100
#define RESET_DMAC      13
#define RESET_GPIO      14
#define RESET_WDT       15
#define RESET_PWM       16
#define RESET_PVTC      17
#define RESET_BMU       18
#define RESET_SYSREG    19
//104
#define RESET_QSPI0     20
#define RESET_QSPI1     21
#define RESET_QSPI2     22
//108
#define RESET_UART0     23
#define RESET_UART1     24
#define RESET_UART2     25
#define RESET_UART3     26
#define RESET_UART4     27
//10C
#define RESET_I2C0      28
#define RESET_I2C1      29
#define RESET_I2C2      30
#define RESET_I2C3      31
#define RESET_I2C4      32
//110
#define RESET_TIMER     33
//114
#define RESET_CAN       34
//118
#define RESET_I2S0      35
#define RESET_I2S1      36
//200
#define RESET_SDIO0     37
#define RESET_EMMC0     38

//304
#define RESET_PCIE0_POWERUP 39
#define RESET_PCIE0_PERST   40
#define RESET_PCIE1_POWERUP 41
#define RESET_PCIE1_PERST   42
#define RESET_PCIE2_POWERUP 43
#define RESET_PCIE2_PERST   44
#define RESET_PCIE3_POWERUP 45
#define RESET_PCIE3_PERST   46
#define RESET_PCIE4_POWERUP 47
#define RESET_PCIE4_PERST   48
#define RESET_PCIE5_POWERUP 49
#define RESET_PCIE5_PERST   50
//308
#define RESET_SATA        51
#define RESET_SATA_PORT0  52
#define RESET_SATA_PORT1  53
#define RESET_SATA_PORT2  54
#define RESET_SATA_PORT3  55
//30C
#define RESET_USB30_VCC   56
#define RESET_USB31_VCC   57
//310
#define RESET_XGMII0      58
#define RESET_XGMII1      59
//314
#define RESET_COMBO_PHY0  60
#define RESET_COMBO_PHY1  61
//400
#define RESET_VPU         62
//500
#define RESET_TRNG        63
#define RESET_PKE         64
#define RESET_HASH        65
#define RESET_SKE_CONFIG  66
#define RESET_SKE_DMA     67
#define RESET_SKE_CORE    68
#define RESET_EFUSE       69

//600
#define RESET_USB20_OTG_HRESETN  70
#define RESET_USB20_OTG_PRSTN    71
#define RESET_USB20_OTG_POR      72
#define RESET_USB20_OTG_PORTRST  73
#define RESET_USB20_HOST_HRESETN 74
#define RESET_USB20_HOST_PRSTN   75
#define RESET_USB20_HOST_POR     76
#define RESET_USB20_HOST_PORTRST 77
#define RESET_USB31CTL_RSTN      78
#define RESET_USB31_PORTRST      79

#define RESET_GMAC_CSR       80
#define RESET_GMAC_DMA       81

//700
#define RESET_DDR0      82
#define RESET_DDR1      83
#define RESET_DDR_PHY0  84
#define RESET_DDR_PHY1  85

#define RESET_DDR_PORT0 86
#define RESET_DDR_PORT1 87
#define RESET_DDR_PORT2 88
#define RESET_DDR_PORT3 89
#define RESET_DDR_PORT4 90
#define RESET_DDR_PORT5 91

//mcu
#define RESET_MCU_CORE  92
#define RESET_MCU_BUS   93
#define RESET_MCU_MAILBOX   94
#define RESET_MCU_SPINLOCK  95

#define RESET_DUMMY    96

#define RESET_NUMBER    97

#endif /* __DT_BINDINGS_K1_PRO_RST_H__ */
