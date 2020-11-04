/*
 * Allwinner SoCs display driver.
 *
 * Copyright (C) 2017 Allwinner.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#ifndef _DE_BLD_H_
#define _DE_BLD_H_

#include <linux/types.h>
#include "de_top.h"

enum de_blend_mode {
	/*
	* pixel color = sc * sa * cfs + dc * da * cfd
	* pixel alpha = sa * afs + da * afd
	* sc = source color
	* sa = source alpha
	* dc = destination color
	* da = destination alpha
	* cfs = source color factor for blend function
	* cfd = destination color factor for blend function
	* afs = source alpha factor for blend function
	* afd = destination alpha factor for blend function
	*/
	DE_BLD_MODE_CLEAR   = 0x00, /* cfs/afs: 0     cfd/afd: 0    */
	DE_BLD_MODE_SRC     = 0x01, /* cfs/afs: 1     cfd/afd: 0    */
	DE_BLD_MODE_DST     = 0x02, /* cfs/afs: 0     cfd/afd: 1    */
	DE_BLD_MODE_SRCOVER = 0x03, /* cfs/afs: 1     cfd/afd: 1-sa */
	DE_BLD_MODE_DSTOVER = 0x04, /* cfs/afs: 1-da  cfd/afd: 1    */
	DE_BLD_MODE_SRCIN   = 0x05, /* cfs/afs: da    cfd/afd: 0    */
	DE_BLD_MODE_DSTIN   = 0x06, /* cfs/afs: 0     cfd/afd: sa   */
	DE_BLD_MODE_SRCOUT  = 0x07, /* cfs/afs: 1-da  cfd/afd: 0    */
	DE_BLD_MODE_DSTOUT  = 0x08, /* cfs/afs: 0     cfd/afd: 1-sa */
	DE_BLD_MODE_SRCATOP = 0x09, /* cfs/afs: da    cfd/afd: 1-sa */
	DE_BLD_MODE_DSTATOP = 0x0a, /* cfs/afs: 1-da  cfd/afd: sa   */
	DE_BLD_MODE_XOR     = 0x0b, /* cfs/afs: 1-da  cfd/afd: 1-sa */
};

struct de_bld_pipe_info {
	u8 enable;
	u8 fcolor_en;
	u8 chn;
	u8 premul_ctl;
};

s32 de_bld_set_bg_color(u32 disp, u32 color);
s32 de_bld_set_pipe_fcolor(u32 disp, u32 pipe, u32 color);
s32 de_bld_set_out_size(u32 disp, u32 width, u32 height);
s32 de_bld_set_fmt_space(u32 disp, u32 fmt_space);
s32 de_bld_set_out_scan_mode(u32 disp, u32 interlace);
s32 de_bld_set_pipe_ctl(u32 disp,
	struct de_bld_pipe_info *pipe_info, u32 pipe_num);
s32 de_bld_set_pipe_attr(u32 disp, u32 chn, u32 pipe);
s32 de_bld_set_blend_mode(u32 disp, u32 ibld_id,
	enum de_blend_mode mode);
s32 de_bld_disable(u32 disp);

s32 de_bld_init(u32 disp, u8 __iomem *de_reg_base);
s32 de_bld_exit(u32 disp);
s32 de_bld_get_reg_blocks(u32 disp,
	struct de_reg_block **blks, u32 *blk_num);

s32 de_bld_get_out_size(u32 disp, u32 *p_width, u32 *p_height);

#endif /* #ifndef _DE_BLD_H_ */
