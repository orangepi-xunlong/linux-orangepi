/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * AVB Header
 *
 * This Synopsys avb software and associated documentation
 * (hereinafter the "Software") is an unsupported proprietary work of
 * Synopsys, Inc. unless otherwise expressly agreed to in writing between
 * Synopsys and you. The Software IS NOT an item of Licensed Software or a
 * Licensed Product under any End User Software License Agreement or
 * Agreement for Licensed Products with Synopsys or any supplement thereto.
 * Synopsys is a registered trademark of Synopsys, Inc. Other names included
 * in the SOFTWARE may be the trademarks of their respective owners.
 *
 * The contents of this file are dual-licensed; you may select either version 2
 * of the GNU General Public License ("GPL") or the MIT license ("MIT").
 *
 * Copyright (c) 2017 Synopsys, Inc. and/or its affiliates.
 *
 * THIS SOFTWARE IS PROVIDED "AS IS"  WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING, BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE, AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT, OR OTHERWISE
 * ARISING FROM, OUT OF, OR IN CONNECTION WITH THE SOFTWARE THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

#ifndef __LINUX_AVB_H__
#define __LINUX_AVB_H__

#include <sunxi-tsn.h>

#define AVB_TEST_HEADER_MAGIC		0xdeadbeef
#define AVB_VIDEO_XRES			640
#define AVB_VIDEO_YRES			480

struct avb_test_header {
	u32 id;
	u32 size;
	u8 payload[0];
} __packed;

static inline size_t avb_hdr_size(void)
{
	return sizeof(struct avb_test_header);
}

static inline void avb_assemble_header(struct avtpdu_header *header,
		size_t bytes)
{
	struct avb_test_header *dh;

	dh = (struct avb_test_header *)&header->data;
	dh->id = AVB_TEST_HEADER_MAGIC;
	dh->size = bytes;
}

static inline int avb_validate_header(struct avtpdu_header *header)
{
	struct avb_test_header *dh;

	dh = (struct avb_test_header *)&header->data;
	if (dh->id != AVB_TEST_HEADER_MAGIC)
		return -EINVAL;
	return 0;
}

static inline void *avb_get_payload_data(struct avtpdu_header *header)
{
	struct avb_test_header *dh;

	dh = (struct avb_test_header *)&header->data;
	return &dh->payload;
}

#endif /* __LINUX_AVB_H__ */
