/*
 * Copyright (C) Cvitek Co., Ltd. 2022-2022. All rights reserved.
 *
 * File Name: cv186x_pinmux.h
 *
 * Description: Cvitek cv186x pinmux header
 */
#ifndef __CV186X_PINMUX_H__
#define __CV186X_PINMUX_H__

#include "./cv186x/cv186x_reg_G1_PINMUX_REG.h"
#include "./cv186x/cv186x_reg_G5_PINMUX_REG.h"
#include "./cv186x/cv186x_reg_G6_PINMUX_REG.h"
#include "./cv186x/cv186x_reg_G7_PINMUX_REG.h"
#include "./cv186x/cv186x_reg_G8_PINMUX_REG.h"
#include "./cv186x/cv186x_reg_G9_PINMUX_REG.h"
#include "./cv186x/cv186x_reg_G11_PINMUX_REG.h"
#include "./cv186x/cv186x_reg_G12_PINMUX_REG.h"
#include "./cv186x/cv186x_reg_PHY_PINMUX_REG.h"
#include "./cv186x/cv186x_reg_CORE_IP_MUX.h"
#include "./cv186x/cv186x_reg_RTC_IP_MUX.h"
#include "./cv186x/cv186x_reg_PHY_IP_MUX.h"
#include "./cv186x/cv186x_reg_CORE_IP_MUX_EXTRA.h"
#include "./cv186x/cv186x_reg_RTC_IP_MUX_EXTRA.h"
#include "./cv186x/cv186x_reg_PHY_IP_MUX_EXTRA.h"
#include "./cv186x/cv186x_reg_PINMUX_VALUE.h"

/* pinmux register offset */
#define G1_PINMUX_REG_REG_BASE 0x28104100
#define G5_PINMUX_REG_REG_BASE 0x28104500
#define G6_PINMUX_REG_REG_BASE 0x28104600
#define G7_PINMUX_REG_REG_BASE 0x05027000
#define G8_PINMUX_REG_REG_BASE 0x28104800
#define G9_PINMUX_REG_REG_BASE 0x28104900
#define G11_PINMUX_REG_REG_BASE 0x28104b00
#define G12_PINMUX_REG_REG_BASE 0x28104c00
#define PHY_PINMUX_REG_REG_BASE 0x28104e00

/* ipmux register offset */
#define CORE_IP_MUX_REG_GRP_BASE 0x28105000
#define RTC_IP_MUX_REG_GRP_BASE 0x28105300
#define PHY_IP_MUX_REG_GRP_BASE 0x28105600

#define GROUP_PINMUX_REG_SIZE 0x100
#define CORE_IP_MUX_REG_SIZE (CORE_IP_MUX_REG_MS_GROUP8 + 4)
#define RTC_IP_MUX_REG_SIZE (RTC_IP_MUX_REG_GRP_SEL_CAM_MCLK6 + 4)
#define PHY_IP_MUX_REG_SIZE (PHY_IP_MUX_REG_GRP_SEL_VO1_D6 + 4)

#define PAD_NON_PHY 0
#define PAD_PHY 4
#define PAD_NON_RTC 0
#define PAD_RTC 7
#define PIN_PULL_DOWN 0x0
#define PIN_PULL_UP 0x1
#define PINMUX_INVALID_SEL 0xffff

static inline void mmio_clrsetbits_32(uintptr_t addr,
				      uint32_t clear,
				      uint32_t set)
{
	void __iomem *tpreg;

	tpreg = ioremap(addr, 0x4);
	if (IS_ERR(tpreg)) {
		pr_err("ioremap %p failed\n", (void *)addr);
		return;
	}

	iowrite32((ioread32(tpreg) & ~clear) | set, tpreg);

	iounmap(tpreg);
}
typedef enum {
    G1 = 1,
    G5 = 5,
    G6 = 6,
    G7 = 7,
    G8 = 8,
    G9 = 9,
    G11 = 11,
    G12 = 12,
    PHY = 14,
} group_pin_t;

#define IP_MUX(FUNC_NAME, CORE_RTC_PHY, VALUE) \
({ \
    if (CORE_RTC_PHY##_IP_MUX_REG_GRP_##FUNC_NAME##_SEL != PINMUX_INVALID_SEL) \
        mmio_clrsetbits_32(CORE_RTC_PHY##_IP_MUX_REG_GRP_BASE + \
                       CORE_RTC_PHY##_IP_MUX_REG_GRP_##FUNC_NAME##_SEL, \
                       CORE_RTC_PHY##_IP_MUX_REG_GRP_SEL_##FUNC_NAME##_MASK, \
                       VALUE << CORE_RTC_PHY##_IP_MUX_REG_GRP_SEL_##FUNC_NAME##_OFFSET); \
})

#define PIN_MUX(PIN_NAME, GROUP, VALUE) \
({ \
    if (GROUP##_PINMUX_REG_REG_##PIN_NAME != PINMUX_INVALID_SEL) \
        mmio_clrsetbits_32(GROUP##_PINMUX_REG_REG_BASE + \
                        GROUP##_PINMUX_REG_REG_##PIN_NAME, \
                       GROUP##_PINMUX_REG_REG_##PIN_NAME##_PIN_SEL_EN_MASK, \
                       VALUE << GROUP##_PINMUX_REG_REG_##PIN_NAME##_PIN_SEL_EN_OFFSET); \
})

#define PINMUX_CONFIG(PIN_NAME, FUNC_NAME, GROUP) \
({ \
    if (GROUP == PHY) { \
        IP_MUX(FUNC_NAME, PHY, PAD_PHY); \
    } else if (GROUP == G7) { \
        IP_MUX(FUNC_NAME, RTC, PAD_RTC); \
    } else { \
        IP_MUX(FUNC_NAME, PHY, PAD_NON_PHY); \
        IP_MUX(FUNC_NAME, RTC, PAD_NON_RTC); \
        IP_MUX(FUNC_NAME, CORE, GROUP); \
    } \
    PIN_MUX(PIN_NAME, GROUP, PINMUX_VAL__##PIN_NAME##__##FUNC_NAME); \
})

/*
bit0: Pull Enable for xxx 
bit1: Pull Selector for xxx (0:pull down; 1:pull up)
*/
#define PIN_PULL_CONFIG_1(PIN_NAME, GROUP, UP_DOWN) \
({ \
    if (UP_DOWN) \
        mmio_clrsetbits_32(GROUP##_PINMUX_REG_REG_BASE + \
                       GROUP##_PINMUX_REG_REG_##PIN_NAME, \
                       GROUP##_PINMUX_REG_REG_##PIN_NAME##_P_EN_MASK | \
                       GROUP##_PINMUX_REG_REG_##PIN_NAME##_PU_SEL_MASK, \
                       (0x1 << GROUP##_PINMUX_REG_REG_##PIN_NAME##_P_EN_OFFSET) | \
                       (0x1 << GROUP##_PINMUX_REG_REG_##PIN_NAME##_PU_SEL_OFFSET) ); \
    else \
        mmio_clrsetbits_32(GROUP##_PINMUX_REG_REG_BASE + \
                       GROUP##_PINMUX_REG_REG_##PIN_NAME, \
                       GROUP##_PINMUX_REG_REG_##PIN_NAME##_P_EN_MASK | \
                       GROUP##_PINMUX_REG_REG_##PIN_NAME##_PU_SEL_MASK, \
                       (0x1 << GROUP##_PINMUX_REG_REG_##PIN_NAME##_P_EN_OFFSET) | \
                       (0x0 << GROUP##_PINMUX_REG_REG_##PIN_NAME##_PU_SEL_OFFSET) ); \
})

/*
bit2: Pull Up Enable for xxx 
bit3: Pull Down Enable for xxx 
*/
#define PIN_PULL_CONFIG_2(PIN_NAME, GROUP, UP_DOWN) \
({ \
    if (UP_DOWN) \
        mmio_clrsetbits_32(GROUP##_PINMUX_REG_REG_BASE + \
                       GROUP##_PINMUX_REG_REG_##PIN_NAME, \
                       GROUP##_PINMUX_REG_REG_##PIN_NAME##_PD_EN_MASK | \
                       GROUP##_PINMUX_REG_REG_##PIN_NAME##_PU_EN_MASK, \
                       0x1 << GROUP##_PINMUX_REG_REG_##PIN_NAME##_PU_EN_OFFSET); \
    else \
        mmio_clrsetbits_32(GROUP##_PINMUX_REG_REG_BASE + \
                       GROUP##_PINMUX_REG_REG_##PIN_NAME, \
                       GROUP##_PINMUX_REG_REG_##PIN_NAME##_PD_EN_MASK | \
                       GROUP##_PINMUX_REG_REG_##PIN_NAME##_PU_EN_MASK, \
                       0x1 << GROUP##_PINMUX_REG_REG_##PIN_NAME##_PD_EN_OFFSET); \
})

#endif /* __CV186X_PINMUX_H__ */
