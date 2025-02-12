// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Device Tree support for Ky SoCs
 *
 * Copyright (c) 2023 Ky Inc.
 */

#include <linux/clk-provider.h>
#include <linux/kernel.h>
#include <linux/reset-controller.h>
#include <linux/spinlock.h>

#define CLK_TYPE_GATE           (0)
#define CLK_TYPE_DIVIDER        (1)
#define CLK_TYPE_MUX            (2)
#define CLK_TYPE_FIXED_FACTOR   (3)

/**
 * struct ccu_clk_data - clk structure
 * @clk_index:  clock index
 * @clk_type:  0-gate, 1-divider, 2-mux, 3-fixed_factor
 * @name: the clock name
 * @parent_name: the name of the parent clock
 * @parent_names: the mult-parents clock name list
 * @num_parents: the number of mult-parents clock
 * @reg:  register address
 * @shift:  shift
 * @width:  width
 * @flags:  hardware-specific flags
 * @factor_mult:  fixed mult of fixed-factor clock
 * @factor_div:  fixed div of fixed-factor clock
 *
 */
struct ccu_clk_data{
    u32 clk_index;
    u8 clk_type;
    const char *name;
    const char *parent_name;
    const char * const *parent_names;
    u8 num_parents;
    u32 reg;
    u8 shift;
    u8 width;
    unsigned long flags;
    u8 factor_mult;
    u8 factor_div;
};
struct ccu_init_data{
    u32 reg;
    u32 value;
};

//registers address
//PLLs
#define CPU0_PLL_CTRL_REG000          (0x0000)
#define CPU0_PLL_CONFIG_REG004        (0x0004)
#define CPU1_PLL_CTRL_REG008          (0x0008)
#define CPU1_PLL_CONFIG_REG00C        (0x000C)
#define DFS_PLL_CTRL_REG010           (0x0010)
#define DFS_PLL_CONFIG_REG014         (0x0014)
#define DDR_PLL_CTRL_REG010           (0x0018)
#define DDR_PLL_CONFIG_REG01C         (0x001C)
#define DDR_PLL_CONFIG_FRAC_REG020    (0x0020)
#define SYS_PLL_CTRL_REG024           (0x0024)
#define SYS_PLL_CONFIG_REG028         (0x0028)
#define GMAC_PLL_CTRL_REG02C          (0x002C)
#define GMAC_PLL_CONFIG_REG030        (0x0030)
#define AUDIO_PLL_CTRL_REG034         (0x0034)
#define AUDIO_PLL_CONFIG_REG038       (0x0038)
#define AUDIO_PLL_CONFIG_FRAC_REG03C  (0x003C)

//SUB system
#define CPU_SUB_CPU0_CTRL_REG100      (0x0100)
#define CPU_SUB_CPU0_EN_REG104        (0x0104)
#define CPU_SUB_CPU1_CTRL_REG108      (0x0108)
#define CPU_SUB_CPU1_EN_REG10C        (0x010C)
#define CPU_SUB_CTRL_REG110           (0x0110)
#define CPU_SUB_EN_REG114             (0x0114)

#define SYS_SUB_CTRL_REG200       (0x0200)
#define SYS_SUB_EN_REG204         (0x0204)
#define SYS_SUB_QSPI_CTRL_REG208  (0x0208)
#define SYS_SUB_QSPI_EN_REG20C    (0x020c)
#define SYS_SUB_UART_EN_REG214    (0x0214)
#define SYS_SUB_I2C_EN_REG21C     (0x021c)
#define SYS_SUB_TIMER_EN_REG224   (0x0224)
#define SYS_SUB_CAN_CTRL_REG228   (0x0228)
#define SYS_SUB_CAN_EN_REG22C     (0x022c)
#define SYS_SUB_I2S_CTRL_REG230   (0x0230)
#define SYS_SUB_I2S_EN_REG234     (0x0234)

#define MEM_SUB_CTRL_REG300       (0x0300)
#define MEM_SUB_EN_REG304         (0x0304)

#define SERDES_SUB_CTRL_REG400        (0x0400)
#define SERDES_SUB_EN_REG404          (0x0404)
#define SERDES_SUB_PCIE_EN_REG40C     (0x040C)
#define SERDES_SUB_SATA_EN_REG414     (0x0414)
#define SERDES_SUB_USB_EN_REG41C      (0x041C)
#define SERDES_SUB_XGMII_EN_REG424    (0x0424)
#define SERDES_SUB_PHY_EN_REG42C      (0x042C)

#define VPU_SUB_CTRL_REG500           (0x0500)
#define VPU_SUB_EN_REG504             (0x0504)

#define SEC_SUB_CTRL_REG600           (0x0600)
#define SEC_SUB_EN_REG604             (0x0604)

#define USB_GMAC_SUB_CTRL_REG700  (0x0700)
#define USB_GMAC_SUB_EN_REG704    (0x0704)

#define MBUS_DDR_SUB_CTRL_REG800  (0x0800)
#define MBUS_DDR_SUB_EN_REG804    (0x0804)

//MCU
#define MCU_SUB_CTRL_REG000    (0x0000)
#define MCU_SUB_EN_REG004      (0x0004)


