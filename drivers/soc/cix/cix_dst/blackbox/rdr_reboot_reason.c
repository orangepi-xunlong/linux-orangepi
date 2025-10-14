// SPDX-License-Identifier: GPL-2.0-only
// Copyright 2025 Cix Technology Group Co., Ltd.

#include <linux/soc/cix/dst_reboot_reason.h>
#include <mntn_public_interface.h>
#include <mntn_subtype_exception.h>
#include "rdr_inner.h"
#include "rdr_print.h"

#undef SUBTYPE_DEF
#define SUBTYPE_DEF(typename, subname, value) \
	{ .type = (value), .word = #subname }
#define SUBTYPE_VAR_DEF(typename)                   \
	static struct exception_sub_word SUBTYPE_## \
		typename[] = { typename##_SUB_LIST }

#undef EXEC_TYPE_DEF
#define EXEC_TYPE_DEF(name, value) SUBTYPE_VAR_DEF(name);

EXEC_TYPE_LIST

#undef EXEC_TYPE_DEF
#define EXEC_TYPE_DEF(name, value) \
	{ .type = (value),         \
	  .word = #name,           \
	  .sub = SUBTYPE_##name,   \
	  .sub_num = ARRAY_SIZE(SUBTYPE_##name) },
static struct exception_word rb_map[] = { EXEC_TYPE_LIST };
#define RDR_UNKNOWN_TYPE "UNDEF"

char *rdr_get_exception_type_name(u32 exce_type)
{
	BB_DBG("exce_type = 0x%x\n", exce_type);
	for (int i = 0; (unsigned int)i < ARRAY_SIZE(rb_map); i++) {
		if (rb_map[i].type == exce_type)
			return rb_map[i].word;
	}

	return RDR_UNKNOWN_TYPE;
}

uint32_t rdr_get_exception_type(char *name)
{
	for (int i = 0; (unsigned int)i < ARRAY_SIZE(rb_map); i++) {
		if (!strncmp(rb_map[i].word, name, strlen(rb_map[i].word)))
			return rb_map[i].type;
	}

	return UNKNOWN;
}

char *rdr_get_exception_subtype_name(u32 exce_type, u32 subtype)
{
	struct exception_word *find = NULL;

	BB_DBG("exce_type = 0x%x, subtype = 0x%x\n", exce_type, subtype);
	for (u32 i = 0; i < ARRAY_SIZE(rb_map); i++) {
		if (rb_map[i].type == exce_type) {
			find = &rb_map[i];
			break;
		}
	}

	if (!find)
		return RDR_UNKNOWN_TYPE;

	for (int i = 0; i < find->sub_num; i++) {
		if (find->sub[i].type == subtype)
			return find->sub[i].word;
	}

	return RDR_UNKNOWN_TYPE;
}

uint32_t rdr_get_reboot_type(void)
{
	u32 type = 0;
	bool is_last = rdr_log_save_is_last();

	type = get_reboot_reason(is_last);
	BB_DBG("%d, type = 0x%x\n", is_last, type);
	return type;
}

u32 rdr_get_exec_subtype_value(void)
{
	u32 type = 0;
	bool is_last = rdr_log_save_is_last();

	type = get_sub_reboot_reason(is_last);
	BB_DBG("%d, type = 0x%x\n", is_last, type);
	return type;
}
