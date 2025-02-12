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
#include <linux/clk-provider.h>
#include <linux/atomic.h>
#include <linux/spinlock.h>

#define LOG_INFO(fmt, arg...)    pr_info("[K1-RESET][%s][%d]:" fmt "\n", __func__, __LINE__, ##arg)

#define PCIE_SW_RESET300   0x300

//FIXME
enum x1matrix_reset_gsr_id{
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

struct x1matrix_reset_signal {
    u32 offset;
    u32 bit;
    int gsr_id;
};

struct x1matrix_reset_gsr_signal {
    u32    offset;
    u32    bit_gsr;
    u32    bit_bus;
    atomic_t deassert_count;
};

struct x1matrix_reset_variant {
    const struct x1matrix_reset_signal *signals;
    struct x1matrix_reset_gsr_signal *gsr_signals;
    u32 signals_num;
    struct reset_control_ops ops;
};

struct x1matrix_reset {
    spinlock_t    lock;
    struct reset_controller_dev rcdev;
    void __iomem *reg_base;
    void __iomem *mcu_reg_base;
    const struct x1matrix_reset_signal *signals;
    struct x1matrix_reset_gsr_signal *gsr_signals;
};

struct x1matrix_reset x1matrix_reset_controller;

static struct x1matrix_reset_gsr_signal
    x1matrix_reset_gsr_signals[RESET_GSR_NUMBER] = {
};

//FIXME
static const struct x1matrix_reset_signal
    x1matrix_reset_signals[64] = {
    [0]   = { PCIE_SW_RESET300, BIT(1), RESET_SERDES_SUB_GSR },
    [1]   = { PCIE_SW_RESET300, BIT(2), RESET_SERDES_SUB_GSR },
    [2]   = { PCIE_SW_RESET300, BIT(3), RESET_SERDES_SUB_GSR },
};

static struct x1matrix_reset *to_x1matrix_reset(
    struct reset_controller_dev *rcdev)
{
    return container_of(rcdev, struct x1matrix_reset, rcdev);
}
static u32 x1matrix_reset_read(struct x1matrix_reset *reset,
    u32 id, bool is_gsr)
{
    u32 value = 0;
    u32 offset = 0;
    if(is_gsr)
        offset = reset->gsr_signals[id].offset;
    else
        offset = reset->signals[id].offset;
    if(id >= /*RESET_MCU_CORE*/92 || (is_gsr && id == RESET_MCU_SUB_GSR)){ //FIXME
        value = readl(reset->mcu_reg_base + offset);
    } else {
        value = readl(reset->reg_base + offset);
    }
    return value;
}

static void x1matrix_reset_write(struct x1matrix_reset *reset, u32 value,
    u32 id, bool is_gsr)
{
    u32 offset = 0;
    if(is_gsr)
        offset = reset->gsr_signals[id].offset;
    else
        offset = reset->signals[id].offset;
    if(id >= /*RESET_MCU_CORE*/92 || (is_gsr && id == RESET_MCU_SUB_GSR)){ //FIXME
        writel(value, reset->mcu_reg_base + offset);
    } else {
        writel(value, reset->reg_base + offset);
    }
}


static void x1matrix_reset_set(struct reset_controller_dev *rcdev,
    u32 id, bool assert, bool is_gsr)
{
    u32 value;
    struct x1matrix_reset *reset = to_x1matrix_reset(rcdev);
    struct x1matrix_reset_gsr_signal *gsr_signal;
    LOG_INFO("assert = %d, id = %d ", assert, id);

    if(is_gsr)
    {
        value = x1matrix_reset_read(reset, id, is_gsr);
        gsr_signal = &reset->gsr_signals[id];

        if(assert == true) {
            if(gsr_signal->bit_bus != 0)
                value &= ~gsr_signal->bit_bus;
        } else {
            value |= gsr_signal->bit_gsr;
        }
        x1matrix_reset_write(reset, value, id, is_gsr);

        value = x1matrix_reset_read(reset, id, is_gsr);

        if(assert == true) {
            value &= ~gsr_signal->bit_gsr;

        } else {
            if(gsr_signal->bit_bus != 0)
                value |= gsr_signal->bit_bus;
        }
        x1matrix_reset_write(reset, value, id, is_gsr);

    }
    else
    {
        value = x1matrix_reset_read(reset, id, is_gsr);
        if(assert == true) {
            value &= ~reset->signals[id].bit;

        } else {
            value |= reset->signals[id].bit;
        }
        x1matrix_reset_write(reset, value, id, is_gsr);
    }
}
static int x1matrix_reset_subsys(struct reset_controller_dev *rcdev,
	unsigned long id, bool assert)
{
    int gsr_id;
    atomic_t * deassert_count;
    struct x1matrix_reset *reset = to_x1matrix_reset(rcdev);
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
    x1matrix_reset_set(rcdev, gsr_id, assert, true);
    return 0;
}
static int x1matrix_reset_update(struct reset_controller_dev *rcdev,
    unsigned long id, bool assert)
{
    unsigned long flags;
    struct x1matrix_reset *reset = to_x1matrix_reset(rcdev);

    if(id < 0 || id >= /*RESET_DUMMY*/96)
        return 0;

    spin_lock_irqsave(&reset->lock, flags);
    if(assert == true){
        x1matrix_reset_set(rcdev, id, assert, false);
        x1matrix_reset_subsys(rcdev, id, true);
    }
    else{
        x1matrix_reset_subsys(rcdev, id, false);
        x1matrix_reset_set(rcdev, id, assert, false);
    }
    spin_unlock_irqrestore(&reset->lock, flags);
    return 0;
}

static int x1matrix_reset_assert(struct reset_controller_dev *rcdev,
    unsigned long id)
{
    return x1matrix_reset_update(rcdev, id, true);
}

static int x1matrix_reset_deassert(struct reset_controller_dev *rcdev, unsigned long id)
{
    return x1matrix_reset_update(rcdev, id, false);
}

static const struct x1matrix_reset_variant x1matrix_reset_data = {
    .signals = x1matrix_reset_signals,
    .signals_num = ARRAY_SIZE(x1matrix_reset_signals),
    .gsr_signals = x1matrix_reset_gsr_signals,
    .ops = {
        .assert   = x1matrix_reset_assert,
        .deassert = x1matrix_reset_deassert,
    },
};

static void x1matrix_reset_init(struct device_node *np)
{
    struct x1matrix_reset *reset = &x1matrix_reset_controller;
    LOG_INFO("init reset");
    reset->reg_base = of_iomap(np, 0);
    reset->mcu_reg_base = of_iomap(np, 1);
    if (!(reset->reg_base &&  reset->mcu_reg_base)) {
        pr_err("%s: could not map reset region\n", __func__);
        return;
    }
    spin_lock_init(&reset->lock);
    reset->signals = x1matrix_reset_data.signals;
    reset->gsr_signals = x1matrix_reset_data.gsr_signals;
    reset->rcdev.owner     = THIS_MODULE;
    reset->rcdev.nr_resets = x1matrix_reset_data.signals_num;
    reset->rcdev.ops       = &x1matrix_reset_data.ops;
    reset->rcdev.of_node   = np;
    LOG_INFO("register");
    reset_controller_register(&reset->rcdev);
}

CLK_OF_DECLARE(k1_reset, "ky,x1matrix-reset", x1matrix_reset_init);

