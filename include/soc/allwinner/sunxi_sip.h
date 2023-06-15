/*
 * Copyright (c) 2020, Allwinner Technology, Inc.
 * Author: Fan Qinghua <fanqinghua@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 */
#ifndef __SUNXI_SIP_H
#define __SUNXI_SIP_H

#include <linux/arm-smccc.h>

#define SET_ROOT_WAKEUP_SOURCE(root_irq)  (root_irq)
#define SET_SEC_WAKEUP_SOURCE(root_irq, secondary_irq)  \
	((1 << 30) | ((secondary_irq) << 10) | (root_irq))
#define SET_THIRD_WAKEUP_SOURCE(root_irq, secondary_irq, third_irq)  \
	((2 << 30) | ((third_irq) << 20) ((secondary_irq) << 10) | (root_irq))
#define SET_WAKEUP_TIME_MS(ms)  ((3 << 30) | (ms))

#ifdef CONFIG_ARM
#ifdef CONFIG_ARCH_SUN50I
#define ARM_SVC_BASE (0x80000000)
#else
#define ARM_SVC_BASE \
	ARM_SMCCC_CALL_VAL(ARM_SMCCC_FAST_CALL, ARM_SMCCC_SMC_32, \
			   ARM_SMCCC_OWNER_OEM, 0)
#endif
#endif

#ifdef CONFIG_ARM64
#define ARM_SVC_BASE		(0xc0000000)
#endif

#define SUNXI_BASE			(0x10)
#define CLEAR_WAKEUP_SRC_REQ		(SUNXI_BASE + 0x15)
#define SET_WAKEUP_SRC_REQ		(SUNXI_BASE + 0x16)
#define SET_DEBUG_DRAM_CRC_PARAS_REQ	(SUNXI_BASE + 0x54)
#define SUNXI_CRASHDUMP			(SUNXI_BASE + 0x85)
#define SUNXI_DDRFREQ			(SUNXI_BASE + 0x86)

#define CLEAR_WAKEUP_SRC		(ARM_SVC_BASE + CLEAR_WAKEUP_SRC_REQ)
#define SET_WAKEUP_SRC			(ARM_SVC_BASE + SET_WAKEUP_SRC_REQ)
#define SET_DEBUG_DRAM_CRC_PARAS	(ARM_SVC_BASE + SET_DEBUG_DRAM_CRC_PARAS_REQ)
#define ARM_SVC_SUNXI_CRASHDUMP_START	(ARM_SVC_BASE + SUNXI_CRASHDUMP)
#define ARM_SVC_SUNXI_DDRFREQ	(ARM_SVC_BASE + SUNXI_DDRFREQ)

#if defined(CONFIG_ARM)
static inline int invoke_scp_fn_smc(u32 function_id, u32 arg1, u32 arg2, u32 arg3)
{
	struct arm_smccc_res res;

	arm_smccc_smc(function_id, arg1, arg2, arg3, 0, 0, 0, 0, &res);
	return res.a0;
}
#elif defined(CONFIG_ARM64)
static inline int invoke_scp_fn_smc(u64 function_id, u64 arg1, u64 arg2, u64 arg3)
{
	struct arm_smccc_res res;

	arm_smccc_smc(function_id, arg1, arg2, arg3, 0, 0, 0, 0, &res);
	return res.a0;
}
#endif

#endif
