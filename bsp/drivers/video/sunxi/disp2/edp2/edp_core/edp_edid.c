/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * edid function of edp driver
 *
 * Copyright (c) 2007-2022 Allwinnertech Co., Ltd.
 * Author: huangyongxing <huangyongxing@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include "edp_edid.h"
#include "../edp_configs.h"
#include "../lowlevel/edp_lowlevel.h"
#include <linux/errno.h>
#include <linux/mm.h>
#include <linux/slab.h>

static const char edid_header[] = {
	0x00, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00
};

/*edp_hal_xxx means xxx function is from lowlevel*/

static s32 edid_block_check_header(char *edid)
{
	s32 i, score = 0;

	for (i = 0; i < sizeof(edid_header); i++)
		if (edid[i] == edid_header[i])
			score++;

	return score;
}

static s32 edid_block_ext_check_tag(char *edid)
{
	return edid[0] == CEA_EXT ? 1 : 0;
}

static s32 edid_block_compute_checksum(char *edid)
{
	u8 i, checksum = 0;

	for (i = 0; i < EDID_LENGTH; i++)
		checksum += edid[i];

	return checksum % 256; /* CEA-861 Spec */
}

static s32 edid_block_valid(char *edid)
{
	s32 ret;

	ret = edid_block_check_header(edid);
	if (ret != 8) {
		EDP_ERR("Error:edid header invalid\n");
		return RET_FAIL;
	}

	ret = edid_block_compute_checksum(edid);
	if (ret != 0) {
		EDP_ERR("Error:edid checksum failed, checksum:%d\n", ret);
		return RET_FAIL;
	}

	return RET_OK;
}

static s32 edid_block_ext_valid(char *edid)
{
	s32 ret;

	ret = edid_block_ext_check_tag(edid);
	if (ret <= 0) {
		EDP_ERR("Error:edid extend block is not support CEA\n");
		return RET_FAIL;
	}

	ret = edid_block_compute_checksum(edid);
	if (ret != 0) {
		EDP_ERR("Error:edid  checksum failed\n");
		return RET_FAIL;
	}

	return RET_OK;
}

static struct edid *edid_block_data(struct edid *edid, s32 index)
{
	return edid + index;
}

/*
 * Search EDID for CEA extension block.
 */
static u8 *edid_find_extension(struct edid *edid, s32 ext_id)
{
	u8 *edid_ext = NULL;
	s32 i;

	/* No EDID or EDID extensions */
	if (edid == NULL || edid->extensions == 0)
		return NULL;

	/* Find CEA extension */
	for (i = 0; i < edid->extensions; i++) {
		edid_ext = (u8 *)edid + EDID_LENGTH * (i + 1);
		if (edid_ext[0] == ext_id)
			break;
	}

	if (i == edid->extensions)
		return NULL;

	return edid_ext;
}



static s32 edp_edid_block_cnt(struct edid *edid)
{
	u32 block_cnt;

	block_cnt = edid->extensions + 1;

	return block_cnt;
}

struct edid *edp_edid_get(u32 sel)
{
	struct edid *edid, *edid_ext, *new;
	s32 ret;
	u32 block_cnt;
	s32 i;

	edid = kmalloc(EDID_LENGTH, GFP_KERNEL);
	if (!edid)
		return NULL;

	ret = edp_hal_read_edid(sel, edid);
	if (ret < 0) {
		EDP_ERR("EDID: edp read edid block0 failed\n");
		goto fail;
	}
	ret = edid_block_valid((char *)edid);
	if (ret < 0) {
		EDP_ERR("edid block0 is invalid\n");
		goto fail;
	}

	block_cnt = edp_edid_block_cnt(edid);
	if (block_cnt > 1) {
		new = krealloc(edid, block_cnt * EDID_LENGTH, GFP_KERNEL);
		if (!new) {
			EDP_ERR("alloc buffer for edid block1 fail\n");
			goto fail;
		}

		edid = new;

		ret = edp_hal_read_edid_ext(sel, block_cnt, edid);
		if (ret < 0) {
			EDP_ERR("EDID: edp read edid extend block failed\n");
			goto fail;
		}

		for (i = 1; i < block_cnt; i++) {
			edid_ext = edid_block_data(edid, i);
			ret = edid_block_ext_valid((char *)edid_ext);
			if (ret < 0) {
				EDP_ERR("edid block ext is invalid\n");
				goto fail;
			}
		}
	}

	print_hex_dump_debug("suxni_edid_data:", DUMP_PREFIX_OFFSET, 16, 1,\
			     edid, block_cnt * sizeof(struct edid), false);
	return edid;

fail:
	kfree(edid);
	return NULL;
}

s32 edp_edid_put(struct edid *edid)
{
	u32 block_cnt;

	if (edid == NULL) {
		EDP_WRN("EDID: edid is already null\n");
		return 0;
	}

	block_cnt = edp_edid_block_cnt(edid);

	memset(edid, 0, block_cnt * EDID_LENGTH);

	kfree(edid);

	return 0;
}

u8 *edp_edid_displayid_extension(struct edid *edid)
{
	return edid_find_extension(edid, DISPLAYID_EXT);
}

u8 *edp_edid_cea_extension(struct edid *edid)
{
	/* Look for a top level CEA extension block */
	return edid_find_extension(edid, CEA_EXT);
}

s32 edp_edid_cea_revision(u8 *cea)
{
	return cea[1];
}

s32 edp_edid_cea_tag(u8 *cea)
{
	return cea[0];
}

s32 edp_edid_cea_db_offsets(u8 *cea, s32 *start, s32 *end)
{
	/* DisplayID CTA extension blocks and top-level CEA EDID
	 * block header definitions differ in the following bytes:
	 *   1) Byte 2 of the header specifies length differently,
	 *   2) Byte 3 is only present in the CEA top level block.
	 *
	 * The different definitions for byte 2 follow.
	 *
	 * DisplayID CTA extension block defines byte 2 as:
	 *   Number of payload bytes
	 *
	 * CEA EDID block defines byte 2 as:
	 *   Byte number (decimal) within this block where the 18-byte
	 *   DTDs begin. If no non-DTD data is present in this extension
	 *   block, the value should be set to 04h (the byte after next).
	 *   If set to 00h, there are no DTDs present in this block and
	 *   no non-DTD data.
	 */
	if (cea[0] == DATA_BLOCK_CTA) {
		*start = 3;
		*end = *start + cea[2];
	} else if (cea[0] == CEA_EXT) {
		/* Data block offset in CEA extension block */
		*start = 4;
		*end = cea[2];
		if (*end == 0)
			*end = 127;
		if (*end < 4 || *end > 127)
			return -ERANGE;
	} else {
		return -EOPNOTSUPP;
	}

	return 0;
}
