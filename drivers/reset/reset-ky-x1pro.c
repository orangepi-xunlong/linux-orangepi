// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Device Tree support for Ky SoCs
 *
 * Copyright (c) 2023 Ky Inc.
 */
#include <linux/mfd/syscon.h>
#include <linux/mod_devicetable.h>
#include <linux/of_device.h>
#include <linux/of_address.h>
#include <linux/platform_device.h>
#include <linux/reset-controller.h>
#include <linux/io.h>
#include <dt-bindings/reset/ky-x1pro-reset.h>
#include <linux/clk-provider.h>
#include <linux/atomic.h>
#include <linux/spinlock.h>

#define LOG_INFO(fmt, arg...)    pr_info("[K1-RESET][%s][%d]:" fmt "\n", __func__, __LINE__, ##arg)

#define CPU0_SW_RESET000   0x000
#define CPU1_SW_RESET004   0x004
#define CPUSUB_SW_RESET008 0x008
#define SYS_SW_RESET100    0x100
#define QSPI_SW_RESET104   0x104
#define UART_SW_RESET108   0x108
#define I2C_SW_RESET10C    0x10C
#define TIMER_SW_RESET110  0x110
#define CAN_SW_RESET114    0x114
#define I2S_SW_RESET118    0x118
#define MEM_SW_RESET200    0x200
#define SERDES_SW_RESET300 0x300
#define PCIE_SW_RESET304   0x304
#define SATA_SW_RESET308   0x308
#define USB_SW_RESET30C    0x30C
#define XGMII_SW_RESET310  0x310
#define PHY_SW_RESET314    0x314
#define VPU_SW_RESET400    0x400
#define SEC_SW_RESET500    0x500
#define USB_GMAC_SW_RESET600  0x600
#define DDR_MBUS_SW_RESET700  0x700
//mcu regs
#define MCU_SW_RESET008  0x008

enum x1pro_reset_gsr_id{
    RESET_CPU_SUB_GSR       = 0,
    RESET_SYS_SUB_GSR       = 1,
    RESET_MEM_SUB_GSR       = 2,
    RESET_SERDES_SUB_GSR    = 3,
    RESET_VPU_SUB_GSR       = 4,
    RESET_SEC_SUB_GSR       = 5,
    RESET_USB_GMAC_SUB_GSR  = 6,
    RESET_DDR_SUB_GSR       = 7,
    RESET_MCU_SUB_GSR       = 8,
    RESET_GSR_NUMBER        = 9,
};

struct x1pro_reset_signal {
    u32 offset;
    u32 bit;
    int gsr_id;
};

struct x1pro_reset_gsr_signal {
    u32    offset;
    u32    bit_gsr;
    u32    bit_bus;
    atomic_t deassert_count;
};

struct x1pro_reset_variant {
    const struct x1pro_reset_signal *signals;
    struct x1pro_reset_gsr_signal *gsr_signals;
    u32 signals_num;
    struct reset_control_ops ops;
};

struct x1pro_reset {
    spinlock_t    lock;
    struct reset_controller_dev rcdev;
    void __iomem *reg_base;
    void __iomem *mcu_reg_base;
    const struct x1pro_reset_signal *signals;
    struct x1pro_reset_gsr_signal *gsr_signals;
};

struct x1pro_reset x1pro_reset_controller;

static struct x1pro_reset_gsr_signal
    x1pro_reset_gsr_signals[RESET_GSR_NUMBER] = {
    [RESET_CPU_SUB_GSR]      = { CPUSUB_SW_RESET008, BIT(0), BIT(2) },
    [RESET_SYS_SUB_GSR]      = { SYS_SW_RESET100, BIT(0) },
    [RESET_MEM_SUB_GSR]      = { MEM_SW_RESET200, BIT(0), BIT(5) },
    [RESET_SERDES_SUB_GSR]   = { SERDES_SW_RESET300, BIT(0) },
    [RESET_VPU_SUB_GSR]      = { VPU_SW_RESET400, BIT(0), BIT(2) },
    [RESET_SEC_SUB_GSR]      = { SEC_SW_RESET500, BIT(0) },
    [RESET_USB_GMAC_SUB_GSR] = { USB_GMAC_SW_RESET600, BIT(0), BIT(13) },
    [RESET_DDR_SUB_GSR]      = { DDR_MBUS_SW_RESET700, BIT(0) },
    [RESET_MCU_SUB_GSR]      = { MCU_SW_RESET008, BIT(0), BIT(12) },
};


static const struct x1pro_reset_signal
    x1pro_reset_signals[RESET_NUMBER] = {
    [RESET_CPUC0C1]     = { CPU0_SW_RESET000, BIT(5), RESET_CPU_SUB_GSR },
    [RESET_CPUC0C2]     = { CPU0_SW_RESET000, BIT(6), RESET_CPU_SUB_GSR },
    [RESET_CPUC0C3]     = { CPU0_SW_RESET000, BIT(7), RESET_CPU_SUB_GSR },
    [RESET_CPUC0TCM]    = { CPU0_SW_RESET000, BIT(8), RESET_CPU_SUB_GSR },
    [RESET_CPUC1APB]    = { CPU1_SW_RESET004, BIT(0), RESET_CPU_SUB_GSR },
    [RESET_CPUC1L2C]    = { CPU1_SW_RESET004, BIT(1), RESET_CPU_SUB_GSR },
    [RESET_CPUC1PIC]    = { CPU1_SW_RESET004, BIT(2), RESET_CPU_SUB_GSR },
    [RESET_CPUC1ACEM]   = { CPU1_SW_RESET004, BIT(3), RESET_CPU_SUB_GSR },
    [RESET_CPUC1C0]     = { CPU1_SW_RESET004, BIT(4), RESET_CPU_SUB_GSR },
    [RESET_CPUC1C1]     = { CPU1_SW_RESET004, BIT(5), RESET_CPU_SUB_GSR },
    [RESET_CPUC1C2]     = { CPU1_SW_RESET004, BIT(6), RESET_CPU_SUB_GSR },
    [RESET_CPUC1C3]     = { CPU1_SW_RESET004, BIT(7), RESET_CPU_SUB_GSR },
    [RESET_CPU_CCI]     = { CPUSUB_SW_RESET008, BIT(1), RESET_CPU_SUB_GSR },
    [RESET_DMAC]        = { SYS_SW_RESET100, BIT(3), RESET_SYS_SUB_GSR },
    [RESET_GPIO]        = { SYS_SW_RESET100, BIT(4), RESET_SYS_SUB_GSR },
    [RESET_WDT]         = { SYS_SW_RESET100, BIT(5), RESET_SYS_SUB_GSR },
    [RESET_PWM]         = { SYS_SW_RESET100, BIT(6), RESET_SYS_SUB_GSR },
    [RESET_PVTC]        = { SYS_SW_RESET100, BIT(7), RESET_SYS_SUB_GSR },
    [RESET_BMU]         = { SYS_SW_RESET100, BIT(8), RESET_SYS_SUB_GSR },
    [RESET_SYSREG]      = { SYS_SW_RESET100, BIT(9), RESET_SYS_SUB_GSR },
    [RESET_QSPI0]       = { QSPI_SW_RESET104, BIT(0), RESET_SYS_SUB_GSR },
    [RESET_QSPI1]       = { QSPI_SW_RESET104, BIT(1), RESET_SYS_SUB_GSR },
    [RESET_QSPI2]       = { QSPI_SW_RESET104, BIT(2), RESET_SYS_SUB_GSR },
    [RESET_UART0]       = { UART_SW_RESET108, BIT(0), RESET_SYS_SUB_GSR },
    [RESET_UART1]       = { UART_SW_RESET108, BIT(1), RESET_SYS_SUB_GSR },
    [RESET_UART2]       = { UART_SW_RESET108, BIT(2), RESET_SYS_SUB_GSR },
    [RESET_UART3]       = { UART_SW_RESET108, BIT(3), RESET_SYS_SUB_GSR },
    [RESET_UART4]       = { UART_SW_RESET108, BIT(4), RESET_SYS_SUB_GSR },
    [RESET_I2C0]        = { I2C_SW_RESET10C, BIT(0), RESET_SYS_SUB_GSR },
    [RESET_I2C1]        = { I2C_SW_RESET10C, BIT(1), RESET_SYS_SUB_GSR },
    [RESET_I2C2]        = { I2C_SW_RESET10C, BIT(2), RESET_SYS_SUB_GSR },
    [RESET_I2C3]        = { I2C_SW_RESET10C, BIT(3), RESET_SYS_SUB_GSR },
    [RESET_I2C4]        = { I2C_SW_RESET10C, BIT(4), RESET_SYS_SUB_GSR },
    [RESET_TIMER]       = { TIMER_SW_RESET110, BIT(0), RESET_SYS_SUB_GSR },
    [RESET_CAN]         = { CAN_SW_RESET114, BIT(0), RESET_SYS_SUB_GSR },
    [RESET_I2S0]        = { I2S_SW_RESET118, BIT(0), RESET_SYS_SUB_GSR },
    [RESET_I2S1]        = { I2S_SW_RESET118, BIT(1), RESET_SYS_SUB_GSR },
    [RESET_SDIO0]       = { MEM_SW_RESET200, BIT(1), RESET_MEM_SUB_GSR },
    [RESET_EMMC0]       = { MEM_SW_RESET200, BIT(3), RESET_MEM_SUB_GSR },
    [RESET_PCIE0_POWERUP]   = { PCIE_SW_RESET304, BIT(0), RESET_SERDES_SUB_GSR },
    [RESET_PCIE0_PERST]     = { PCIE_SW_RESET304, BIT(1), RESET_SERDES_SUB_GSR },
    [RESET_PCIE1_POWERUP]   = { PCIE_SW_RESET304, BIT(2), RESET_SERDES_SUB_GSR },
    [RESET_PCIE1_PERST]     = { PCIE_SW_RESET304, BIT(3), RESET_SERDES_SUB_GSR },
    [RESET_PCIE2_POWERUP]   = { PCIE_SW_RESET304, BIT(4), RESET_SERDES_SUB_GSR },
    [RESET_PCIE2_PERST]     = { PCIE_SW_RESET304, BIT(5), RESET_SERDES_SUB_GSR },
    [RESET_PCIE3_POWERUP]   = { PCIE_SW_RESET304, BIT(6), RESET_SERDES_SUB_GSR },
    [RESET_PCIE3_PERST]     = { PCIE_SW_RESET304, BIT(7), RESET_SERDES_SUB_GSR },
    [RESET_PCIE4_POWERUP]   = { PCIE_SW_RESET304, BIT(8), RESET_SERDES_SUB_GSR },
    [RESET_PCIE4_PERST]     = { PCIE_SW_RESET304, BIT(9), RESET_SERDES_SUB_GSR },
    [RESET_PCIE5_POWERUP]   = { PCIE_SW_RESET304, BIT(10), RESET_SERDES_SUB_GSR },
    [RESET_PCIE5_PERST]     = { PCIE_SW_RESET304, BIT(11), RESET_SERDES_SUB_GSR },
    [RESET_SATA]        = { SATA_SW_RESET308, BIT(0), RESET_SERDES_SUB_GSR },
    [RESET_SATA_PORT0]  = { SATA_SW_RESET308, BIT(1), RESET_SERDES_SUB_GSR },
    [RESET_SATA_PORT1]  = { SATA_SW_RESET308, BIT(2), RESET_SERDES_SUB_GSR },
    [RESET_SATA_PORT2]  = { SATA_SW_RESET308, BIT(3), RESET_SERDES_SUB_GSR },
    [RESET_SATA_PORT3]  = { SATA_SW_RESET308, BIT(4), RESET_SERDES_SUB_GSR },
    [RESET_USB30_VCC]   = { USB_SW_RESET30C, BIT(0), RESET_SERDES_SUB_GSR },
    [RESET_USB31_VCC]   = { USB_SW_RESET30C, BIT(1), RESET_SERDES_SUB_GSR },
    [RESET_XGMII0]      = { XGMII_SW_RESET310, BIT(0), RESET_SERDES_SUB_GSR },
    [RESET_XGMII1]      = { XGMII_SW_RESET310, BIT(1), RESET_SERDES_SUB_GSR },
    [RESET_COMBO_PHY0]  = { PHY_SW_RESET314, BIT(0), RESET_SERDES_SUB_GSR },
    [RESET_COMBO_PHY1]  = { PHY_SW_RESET314, BIT(1), RESET_SERDES_SUB_GSR },
    [RESET_VPU]         = { VPU_SW_RESET400, BIT(1), RESET_VPU_SUB_GSR },
    [RESET_TRNG]        = { SEC_SW_RESET500, BIT(1), RESET_SEC_SUB_GSR },
    [RESET_PKE]         = { SEC_SW_RESET500, BIT(2), RESET_SEC_SUB_GSR },
    [RESET_HASH]        = { SEC_SW_RESET500, BIT(3), RESET_SEC_SUB_GSR },
    [RESET_SKE_CONFIG]  = { SEC_SW_RESET500, BIT(4), RESET_SEC_SUB_GSR },
    [RESET_SKE_DMA]     = { SEC_SW_RESET500, BIT(5), RESET_SEC_SUB_GSR },
    [RESET_SKE_CORE]    = { SEC_SW_RESET500, BIT(6), RESET_SEC_SUB_GSR },
    [RESET_EFUSE]       = { SEC_SW_RESET500, BIT(7), RESET_SEC_SUB_GSR },
    [RESET_USB20_OTG_HRESETN]     = { USB_GMAC_SW_RESET600, BIT(1), RESET_USB_GMAC_SUB_GSR },
    [RESET_USB20_OTG_PRSTN]       = { USB_GMAC_SW_RESET600, BIT(2), RESET_USB_GMAC_SUB_GSR },
    [RESET_USB20_OTG_POR]         = { USB_GMAC_SW_RESET600, BIT(3), RESET_USB_GMAC_SUB_GSR },
    [RESET_USB20_OTG_PORTRST]     = { USB_GMAC_SW_RESET600, BIT(4), RESET_USB_GMAC_SUB_GSR },
    [RESET_USB20_HOST_HRESETN]    = { USB_GMAC_SW_RESET600, BIT(5), RESET_USB_GMAC_SUB_GSR },
    [RESET_USB20_HOST_PRSTN]      = { USB_GMAC_SW_RESET600, BIT(6), RESET_USB_GMAC_SUB_GSR },
    [RESET_USB20_HOST_POR]        = { USB_GMAC_SW_RESET600, BIT(7), RESET_USB_GMAC_SUB_GSR },
    [RESET_USB20_HOST_PORTRST]    = { USB_GMAC_SW_RESET600, BIT(8), RESET_USB_GMAC_SUB_GSR },
    [RESET_USB31CTL_RSTN]         = { USB_GMAC_SW_RESET600, BIT(9), RESET_USB_GMAC_SUB_GSR },
    [RESET_USB31_PORTRST]         = { USB_GMAC_SW_RESET600, BIT(10), RESET_USB_GMAC_SUB_GSR },
    [RESET_GMAC_CSR]              = { USB_GMAC_SW_RESET600, BIT(11), RESET_USB_GMAC_SUB_GSR },
    [RESET_GMAC_DMA]              = { USB_GMAC_SW_RESET600, BIT(12), RESET_USB_GMAC_SUB_GSR },
    [RESET_DDR0]          = { DDR_MBUS_SW_RESET700, BIT(1), RESET_DDR_SUB_GSR },
    [RESET_DDR1]          = { DDR_MBUS_SW_RESET700, BIT(2), RESET_DDR_SUB_GSR },
    [RESET_DDR_PHY0]      = { DDR_MBUS_SW_RESET700, BIT(3), RESET_DDR_SUB_GSR },
    [RESET_DDR_PHY1]      = { DDR_MBUS_SW_RESET700, BIT(4), RESET_DDR_SUB_GSR },
    [RESET_DDR_PORT0]     = { DDR_MBUS_SW_RESET700, BIT(5), RESET_DDR_SUB_GSR },
    [RESET_DDR_PORT1]     = { DDR_MBUS_SW_RESET700, BIT(6), RESET_DDR_SUB_GSR },
    [RESET_DDR_PORT2]     = { DDR_MBUS_SW_RESET700, BIT(7), RESET_DDR_SUB_GSR },
    [RESET_DDR_PORT3]     = { DDR_MBUS_SW_RESET700, BIT(8), RESET_DDR_SUB_GSR },
    [RESET_DDR_PORT4]     = { DDR_MBUS_SW_RESET700, BIT(9), RESET_DDR_SUB_GSR },
    [RESET_DDR_PORT5]     = { DDR_MBUS_SW_RESET700, BIT(10), RESET_DDR_SUB_GSR },
    //mcu
    [RESET_MCU_CORE]      = { MCU_SW_RESET008, BIT(1), RESET_MCU_SUB_GSR },
    [RESET_MCU_BUS]       = { MCU_SW_RESET008, BIT(2), RESET_MCU_SUB_GSR },
    [RESET_MCU_MAILBOX]   = { MCU_SW_RESET008, BIT(13), RESET_MCU_SUB_GSR },
    [RESET_MCU_SPINLOCK]  = { MCU_SW_RESET008, BIT(14), RESET_MCU_SUB_GSR },
    [RESET_DUMMY]         = {},
};

static struct x1pro_reset *to_x1pro_reset(
    struct reset_controller_dev *rcdev)
{
    return container_of(rcdev, struct x1pro_reset, rcdev);
}
static u32 x1pro_reset_read(struct x1pro_reset *reset,
    u32 id, bool is_gsr)
{
    u32 value = 0;
    u32 offset = 0;
    if(is_gsr)
        offset = reset->gsr_signals[id].offset;
    else
        offset = reset->signals[id].offset;
    if(id >= RESET_MCU_CORE || (is_gsr && id == RESET_MCU_SUB_GSR)){
        value = readl(reset->mcu_reg_base + offset);
    } else {
        value = readl(reset->reg_base + offset);
    }
    return value;
}

static void x1pro_reset_write(struct x1pro_reset *reset, u32 value,
    u32 id, bool is_gsr)
{
    u32 offset = 0;
    if(is_gsr)
        offset = reset->gsr_signals[id].offset;
    else
        offset = reset->signals[id].offset;
    if(id >= RESET_MCU_CORE || (is_gsr && id == RESET_MCU_SUB_GSR)){
        writel(value, reset->mcu_reg_base + offset);
    } else {
        writel(value, reset->reg_base + offset);
    }
}


static void x1pro_reset_set(struct reset_controller_dev *rcdev,
    u32 id, bool assert, bool is_gsr)
{
    u32 value;
    struct x1pro_reset *reset = to_x1pro_reset(rcdev);
    struct x1pro_reset_gsr_signal *gsr_signal;
    LOG_INFO("assert = %d, id = %d ", assert, id);

    if(is_gsr)
    {
        value = x1pro_reset_read(reset, id, is_gsr);
        gsr_signal = &reset->gsr_signals[id];

        if(assert == true) {
            if(gsr_signal->bit_bus != 0)
                value &= ~gsr_signal->bit_bus;
        } else {
            value |= gsr_signal->bit_gsr;
        }
        x1pro_reset_write(reset, value, id, is_gsr);

        value = x1pro_reset_read(reset, id, is_gsr);

        if(assert == true) {
            value &= ~gsr_signal->bit_gsr;

        } else {
            if(gsr_signal->bit_bus != 0)
                value |= gsr_signal->bit_bus;
        }
        x1pro_reset_write(reset, value, id, is_gsr);

    }
    else
    {
        value = x1pro_reset_read(reset, id, is_gsr);
        if(assert == true) {
            value &= ~reset->signals[id].bit;

        } else {
            value |= reset->signals[id].bit;
        }
        x1pro_reset_write(reset, value, id, is_gsr);
    }
}
static int x1pro_reset_subsys(struct reset_controller_dev *rcdev,
	unsigned long id, bool assert)
{
    int gsr_id;
    atomic_t * deassert_count;
    struct x1pro_reset *reset = to_x1pro_reset(rcdev);
    gsr_id = reset->signals[id].gsr_id;

    if(gsr_id < 0 || gsr_id >= RESET_GSR_NUMBER)
        return 0;
    deassert_count = &reset->gsr_signals[gsr_id].deassert_count;
    if(assert == true) {
        if (WARN_ON(atomic_read(deassert_count) == 0))
            return -EINVAL;
        if (atomic_dec_return(deassert_count) != 0)
            return 0;
    } else {
        if (atomic_inc_return(deassert_count) != 1)
            return 0;
    }
    x1pro_reset_set(rcdev, gsr_id, assert, true);
    return 0;
}
static int x1pro_reset_update(struct reset_controller_dev *rcdev,
    unsigned long id, bool assert)
{
    unsigned long flags;
    struct x1pro_reset *reset = to_x1pro_reset(rcdev);

    if(id < 0 || id >= RESET_DUMMY)
        return 0;

    spin_lock_irqsave(&reset->lock, flags);
    if(assert == true){
        x1pro_reset_set(rcdev, id, assert, false);
        x1pro_reset_subsys(rcdev, id, true);
    }
    else{
        x1pro_reset_subsys(rcdev, id, false);
        x1pro_reset_set(rcdev, id, assert, false);
    }
    spin_unlock_irqrestore(&reset->lock, flags);
    return 0;
}

static int x1pro_reset_assert(struct reset_controller_dev *rcdev,
    unsigned long id)
{
    return x1pro_reset_update(rcdev, id, true);
}

static int x1pro_reset_deassert(struct reset_controller_dev *rcdev, unsigned long id)
{
    return x1pro_reset_update(rcdev, id, false);
}

static const struct x1pro_reset_variant x1pro_reset_data = {
    .signals = x1pro_reset_signals,
    .signals_num = ARRAY_SIZE(x1pro_reset_signals),
    .gsr_signals = x1pro_reset_gsr_signals,
    .ops = {
        .assert   = x1pro_reset_assert,
        .deassert = x1pro_reset_deassert,
    },
};

static void x1pro_reset_init(struct device_node *np)
{
    struct x1pro_reset *reset = &x1pro_reset_controller;
    LOG_INFO("init reset");
    reset->reg_base = of_iomap(np, 0);
    reset->mcu_reg_base = of_iomap(np, 1);
    if (!(reset->reg_base &&  reset->mcu_reg_base)) {
        pr_err("%s: could not map reset region\n", __func__);
        return;
    }
    spin_lock_init(&reset->lock);
    reset->signals = x1pro_reset_data.signals;
    reset->gsr_signals = x1pro_reset_data.gsr_signals;
    reset->rcdev.owner     = THIS_MODULE;
    reset->rcdev.nr_resets = x1pro_reset_data.signals_num;
    reset->rcdev.ops       = &x1pro_reset_data.ops;
    reset->rcdev.of_node   = np;
    LOG_INFO("register");
    reset_controller_register(&reset->rcdev);
}

CLK_OF_DECLARE(k1_reset, "ky,x1pro-reset", x1pro_reset_init);

