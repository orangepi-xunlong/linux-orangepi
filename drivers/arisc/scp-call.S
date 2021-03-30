/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * Copyright (C) 2015 Allwinner Tech Limited
 *
 * Author: sunny <sunny@allwinnertech.com>
 */

#include <linux/linkage.h>

#ifdef CONFIG_ARM
/* int invoke_scp_fn_smc(u32 function_id, u32 arg0, u32 arg1, u32 arg2) */
#include <asm/opcodes-sec.h>
ENTRY(invoke_scp_fn_smc)
	__SMC(0)
	bx	lr
ENDPROC(invoke_scp_fn_smc)
#endif
#ifdef CONFIG_ARM64
/* int invoke_scp_fn_smc(u64 function_id, u64 arg0, u64 arg1, u64 arg2) */
ENTRY(invoke_scp_fn_smc)
	smc	#0
	ret
ENDPROC(invoke_scp_fn_smc)
#endif
