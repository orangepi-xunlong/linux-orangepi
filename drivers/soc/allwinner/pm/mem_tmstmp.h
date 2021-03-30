/*
 * Copyright (c) 2011-2020 yanggq.young@allwinnertech.com
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published by
 * the Free Software Foundation.
 */
#ifndef __MEM_TMSTMP_H__
#define __MEM_TMSTMP_H__

typedef struct __MEM_TMSTMP_REG {
	/*  offset:0x00 */
	volatile __u32 Ctl;
	volatile __u32 reserved0[7];
	/*  offset:0x20 */
	volatile __u32 Cluster0CtrlReg1;

} __mem_tmstmp_reg_t;

/* for super standby; */
__s32 mem_tmstmp_save(__mem_tmstmp_reg_t *ptmstmp_state);
__s32 mem_tmstmp_restore(__mem_tmstmp_reg_t *ptmstmp_state);

#endif
