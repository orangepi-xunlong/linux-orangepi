// SPDX-License-Identifier: GPL-2.0
/* Copyright 2025
 Cix Technology Group Co., Ltd.*/
/**
 * SoC: CIX SKY1 platform
 */
#include <mntn_public_interface.h>
#include "se_crash.h"

#define STRUCT_OUT_U32(p, member) \
	rdr_cleartext_print(fp, &err, #member ": 0x%08x\n", p->member)

typedef union {
	unsigned int value;
	struct {
		unsigned int IPSR : 9; // Interrupt Program Status register (IPSR)
		unsigned int
			EPSR : 18; // Execution Program Status register (EPSR)
		unsigned int
			APSR : 5; // Application Program Status register (APSR)
	} bits;
} psr_t; // Program status register.

typedef struct {
	unsigned int r0; // Register R0
	unsigned int r1; // Register R1
	unsigned int r2; // Register R2
	unsigned int r3; // Register R3
	unsigned int r12; // Register R12
	unsigned int lr; // Link register
	unsigned int pc; // Program counter
	psr_t psr;

	unsigned int cfsr; // CFSR
	unsigned int hfsr; // HFSR
	unsigned int dfsr; // DFSR
	unsigned int bfar; // BFAR
	unsigned int mmfar; // MMFAR
} saved_reg_ex;

struct se_einfo_head {
	struct einfo_offset stack;
	struct einfo_offset reg;
};

struct se_einfo {
	struct se_einfo_head head;
	char data[];
};

int se_wdt_cleartext(const char *log_dir_path, u64 log_addr, u32 log_len)
{
	struct se_einfo *einfo = (void *)log_addr;
	u32 *stack;
	saved_reg_ex *reg;
	struct file *fp;
	bool err;

	if (IS_ERR_OR_NULL(einfo))
		return -1;

	fp = bbox_cleartext_get_filep(log_dir_path, "se_wdt.txt");
	if (IS_ERR_OR_NULL(fp))
		return -1;

	stack = (void *)(einfo->data + einfo->head.stack.offset);
	reg = (void *)(einfo->data + einfo->head.reg.offset);

	for (int i = 0; i < einfo->head.stack.size / sizeof(u32); i += 2) {
		rdr_cleartext_print(fp, &err, "0x%08x, 0x%08x\n", stack[i],
				    stack[i + 1]);
	}

	STRUCT_OUT_U32(reg, r0);
	STRUCT_OUT_U32(reg, r1);
	STRUCT_OUT_U32(reg, r2);
	STRUCT_OUT_U32(reg, r3);
	STRUCT_OUT_U32(reg, r12);
	STRUCT_OUT_U32(reg, lr);
	STRUCT_OUT_U32(reg, pc);
	STRUCT_OUT_U32(reg, psr.bits.APSR);
	STRUCT_OUT_U32(reg, psr.bits.EPSR);
	STRUCT_OUT_U32(reg, psr.bits.IPSR);
	STRUCT_OUT_U32(reg, cfsr);
	STRUCT_OUT_U32(reg, hfsr);
	STRUCT_OUT_U32(reg, dfsr);
	STRUCT_OUT_U32(reg, bfar);
	STRUCT_OUT_U32(reg, mmfar);
	bbox_cleartext_end_filep(fp);
	return 0;
}