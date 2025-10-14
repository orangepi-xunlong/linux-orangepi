// SPDX-License-Identifier: (GPL-2.0-only OR BSD-3-Clause)
/*
 * Copyright 2024 Cix Technology Group Co., Ltd.
 */

#ifndef __CIX_COMMON_H__
#define __CIX_COMMON_H__

#include <linux/clk.h>

#define EXCEPT_MAX_HDR_SIZE		0x400
#define CIX_STACK_DUMP_SIZE		32

void cix_get_registers(struct snd_sof_dev *sdev,
		       struct sof_ipc_dsp_oops_xtensa *xoops,
		       struct sof_ipc_panic_info *panic_info,
		       u32 *stack, size_t stack_words);

void cix_dump(struct snd_sof_dev *sdev, u32 flags);

struct cix_clocks {
	struct clk_bulk_data *dsp_clks;
	int num_dsp_clks;
};

int cix_parse_clocks(struct snd_sof_dev *sdev, struct cix_clocks *clks);
int cix_enable_clocks(struct snd_sof_dev *sdev, struct cix_clocks *clks);
void cix_disable_clocks(struct snd_sof_dev *sdev, struct cix_clocks *clks);

#endif
