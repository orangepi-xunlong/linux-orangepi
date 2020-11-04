/*
 * Allwinner SoCs display driver.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

/*******************************************************************************
 *  All Winner Tech, All Right Reserved. 2014-2016 Copyright (c)
 *
 *  File name   :  display engine 3.0 dns basic function definition
 *
 *  History     :  2016-3-3 zzz  v0.1  Initial version
 ******************************************************************************/

#include "de_dns_type.h"
#include "de_rtmx.h"
#include "de_enhance.h"
#include "de_vep_table.h"

enum {
	DNS_DNS_REG_BLK = 0,
	DNS_IQA_REG_BLK,
	DNS_REG_BLK_NUM,
};

struct de_dns_private {
	struct de_reg_mem_info reg_mem_info;
	u32 reg_blk_num;
	struct de_reg_block reg_blks[DNS_REG_BLK_NUM];
	struct de_dns_para dns_para;

	void (*set_blk_dirty)(struct de_dns_private *priv,
		u32 blk_id, u32 dirty);
};

static struct de_dns_private dns_priv[DE_NUM][VI_CHN_NUM];


static inline struct dns_reg *get_dns_reg(struct de_dns_private *priv)
{
	return (struct dns_reg *)(priv->reg_blks[DNS_DNS_REG_BLK].vir_addr);
}

static inline struct iqa_reg *get_iqa_reg(struct de_dns_private *priv)
{
	return (struct iqa_reg *)(priv->reg_blks[DNS_IQA_REG_BLK].vir_addr);
}

static void dns_set_block_dirty(
	struct de_dns_private *priv, u32 blk_id, u32 dirty)
{
	priv->reg_blks[blk_id].dirty = dirty;
}

static void dns_set_rcq_head_dirty(
	struct de_dns_private *priv, u32 blk_id, u32 dirty)
{
	if (priv->reg_blks[blk_id].rcq_hd) {
		priv->reg_blks[blk_id].rcq_hd->dirty.dwval = dirty;
	} else {
		DE_WRN("rcq_head is null ! blk_id=%d\n", blk_id);
	}
}

s32 de_dns_init(u32 disp, u32 chn, uintptr_t reg_base,
	u8 __iomem **phy_addr, u8 **vir_addr, u32 *size)
{
	struct de_dns_private *priv = &dns_priv[disp][chn];
	struct de_reg_mem_info *reg_mem_info = &(priv->reg_mem_info);
	struct de_reg_block *reg_blk;
	uintptr_t base;
	u32 phy_chn;
	u32 rcq_used = de_feat_is_using_rcq(disp);

	phy_chn = de_feat_get_phy_chn_id(disp, chn);
	base = reg_base + DE_CHN_OFFSET(phy_chn) + CHN_DNS_OFFSET;

	reg_mem_info->phy_addr = *phy_addr;
	reg_mem_info->vir_addr = *vir_addr;
	reg_mem_info->size = DE_DNS_REG_MEM_SIZE;

	priv->reg_blk_num = DNS_REG_BLK_NUM;

	reg_blk = &(priv->reg_blks[DNS_DNS_REG_BLK]);
	reg_blk->phy_addr = reg_mem_info->phy_addr;
	reg_blk->vir_addr = reg_mem_info->vir_addr;
	reg_blk->size = sizeof(struct dns_reg);
	reg_blk->reg_addr = (u8 __iomem *)base;

	reg_blk = &(priv->reg_blks[DNS_IQA_REG_BLK]);
	reg_blk->phy_addr = reg_mem_info->phy_addr + 0x100;
	reg_blk->vir_addr = reg_mem_info->vir_addr + 0x100;
	reg_blk->size = sizeof(struct iqa_reg);
	reg_blk->reg_addr = (u8 __iomem *)(base + 0x100);

	*phy_addr += DE_DNS_REG_MEM_SIZE;
	*vir_addr += DE_DNS_REG_MEM_SIZE;
	*size -= DE_DNS_REG_MEM_SIZE;

	if (rcq_used)
		priv->set_blk_dirty = dns_set_rcq_head_dirty;
	else
		priv->set_blk_dirty = dns_set_block_dirty;

	return 0;
}

s32 de_dns_exit(u32 disp, u32 chn)
{
	return 0;
}

s32 de_dns_get_reg_blocks(u32 disp, u32 chn,
	struct de_reg_block **blks, u32 *blk_num)
{
	struct de_dns_private *priv = &(dns_priv[disp][chn]);
	u32 i, num;

	if (blks == NULL) {
		*blk_num = priv->reg_blk_num;
		return 0;
	}

	if (*blk_num >= priv->reg_blk_num) {
		num = priv->reg_blk_num;
	} else {
		num = *blk_num;
		DE_WRN("should not happen\n");
	}
	for (i = 0; i < num; ++i)
		blks[i] = priv->reg_blks + i;

	*blk_num = i;
	return 0;
}

s32 de_dns_enable(u32 disp, u32 chn, u32 en)
{
	struct de_dns_private *priv = &dns_priv[disp][chn];
	struct dns_reg *reg = get_dns_reg(priv);

	reg->ctrl.bits.en = en;
	priv->set_blk_dirty(priv, DNS_DNS_REG_BLK, 1);
	return 0;
}

s32 de_dns_set_size(u32 disp, u32 chn, u32 width, u32 height)
{
	struct de_dns_private *priv = &dns_priv[disp][chn];
	struct dns_reg *reg = get_dns_reg(priv);

	reg->size.dwval = (height - 1) << 16 | (width - 1);
	priv->dns_para.winsz_sel = (width > 2048) ? 1 : 0;
	reg->ctrl.bits.winsz_sel = priv->dns_para.winsz_sel;
	priv->set_blk_dirty(priv, DNS_DNS_REG_BLK, 1);
	return 0;
}

s32 de_dns_set_window(u32 disp, u32 chn,
			    u32 win_enable, struct de_rect_o window)
{
	/* save win en para */
	struct de_dns_private *priv = &dns_priv[disp][chn];
	struct dns_reg *reg = get_dns_reg(priv);

	priv->dns_para.win_en = win_enable;

	reg->ctrl.bits.win_en = win_enable;
	if (win_enable) {
		reg->win0.dwval = window.y << 16 | window.x;
		reg->win1.dwval =
			  (window.y + window.h - 1) << 16 |
			  (window.x + window.w - 1);
	}
	priv->set_blk_dirty(priv, DNS_DNS_REG_BLK, 1);
	return 0;
}

s32 de_dns_init_para(u32 disp, u32 chn)
{
	struct de_dns_private *priv = &dns_priv[disp][chn];
	struct dns_reg *reg = get_dns_reg(priv);
	struct iqa_reg *iqa = get_iqa_reg(priv);

	s32 SIG = 1;
	s32 SIG2 = 10;
	s32 SIG3 = 255;
	s32 DIR_RSIG_GAIN = 100;
	s32 DIR_THRLOW = 64;
	s32 DIR_THRHIGH = 192;
	s32 DIR_CEN = 128;
	s32 DIR_CEN_RAD = (DIR_THRHIGH - DIR_THRLOW + 1) / 2;
	s32 RSIG_CEN = SIG2 + DIR_RSIG_GAIN * (DIR_CEN - DIR_THRLOW) / 16;
	s32 DIR_RSIG_GAIN2;
	RSIG_CEN = min(SIG3, RSIG_CEN);
	DIR_RSIG_GAIN2 = (SIG3 - RSIG_CEN + DIR_CEN_RAD / 2) / DIR_CEN_RAD;

	reg->lpara0.dwval = DIR_RSIG_GAIN2 << 24 |
				SIG3 << 16 | SIG2 << 8 | SIG;
	reg->lpara1.dwval = DIR_THRHIGH << 24 |
				DIR_THRLOW << 16 |
				DIR_RSIG_GAIN << 8 |
				DIR_CEN;
	reg->lpara2.dwval = 0x77 << 24 |
				0 << 20 |
				0 << 16 |
				0 << 8 | 1;
	reg->lpara3.dwval = RSIG_CEN;
	priv->set_blk_dirty(priv, DNS_DNS_REG_BLK, 1);

	iqa->blk_dt_para.dwval = 0xc810;
	priv->set_blk_dirty(priv, DNS_IQA_REG_BLK, 1);
	return 0;
}

s32 de_dns_read_iqa_regs(u32 disp, u32 chn,
			   u32 regs[18])
{
	/* Read regs */
	struct de_dns_private *priv = &dns_priv[disp][chn];

	memcpy((u8 *)regs,
		(u8 *)priv->reg_blks[DNS_IQA_REG_BLK].reg_addr,
		sizeof(u32) * 18);

	return 0;
}

s32 de_dns_info2para(u32 disp, u32 chn,
			   u32 fmt, u32 dev_type,
			   struct __dns_config_data *para,
			   u32 bypass)
{
	struct de_dns_private *priv = &dns_priv[disp][chn];
	struct dns_reg *reg = get_dns_reg(priv);

	u32 en;
	/* parameters */
	en = ((para->level == 0) || (bypass != 0)) ? 0 : 1;
	para->mod_en = en; /* return enable info */

	if (en == 0 || fmt == 1 || para->inw > 4096 || para->inw < 32) {
		/*
		* if level=0, module will be disabled,
		* no need to set para register
		* if rgb fmt, dont support
		*/
		priv->dns_para.en = 0;
	} else {
		/* save info, and apply in tasklet */
		priv->dns_para.en = 1;
		priv->dns_para.autolvl = para->level;
		priv->dns_para.xst = para->croprect.x & 0x7;
		priv->dns_para.yst = para->croprect.y & 0x7;
		priv->dns_para.w = para->inw;
		priv->dns_para.h = para->inh;
	}
	reg->ctrl.bits.en = priv->dns_para.en;
	priv->set_blk_dirty(priv, DNS_DNS_REG_BLK, 1);

	return 0;
}

s32 de_dns_para_apply(u32 disp, u32 chn,
		struct de_dns_para *para)
{
	struct de_dns_private *priv = &dns_priv[disp][chn];
	struct dns_reg *reg = get_dns_reg(priv);

	reg->lpara0.dwval = para->dir_rsig_gain2 << 24 |
			para->sig3 << 16 |
			para->sig2 << 8 | para->sig;
	reg->lpara1.dwval = para->dir_thrhigh << 24 |
			para->dir_thrlow << 16 |
			para->dir_rsig_gain << 8 |
			para->dir_center;
	reg->lpara2.dwval = 0x77 << 24 |
			para->xst << 20 |
			para->yst << 16 |
			para->bgain << 8 | 1;
	reg->lpara3.dwval = para->rsig_cen;

	priv->set_blk_dirty(priv, DNS_DNS_REG_BLK, 1);
	return 0;
}

s32 cal_blks(s32 w, s32 h, s32 DB_XST, s32 DB_YST)
{
	s32 blk_num;
	s32 yres = (h + DB_YST - 8) % 8;
	s32 yend = (8 - DB_YST) + ((h + DB_YST - 8) / 8) * 8;
	s32 xres = (w + DB_XST - 8) % 8;
	s32 xend = (8 - DB_XST) + ((w + DB_XST - 8) / 8) * 8;

	if (yres == 0)
		yend = yend - 8;

	if (xres == 0)
		xend = xend - 8;

	blk_num = ((yend - DB_YST) / 8)*((xend - DB_XST) / 8);

	return blk_num;
}

void dns_para_adapt(struct de_dns_para *para, s32 regs[18])
{
	s32 auto_lvl = para->autolvl;
	s32 auto_level_ratio;
	s32 sum, avg;
	s32 blkm, blkn, blkr, blks;
	s32 SIG, SIG2, SIG3;
	s32 DIR_RSIG_GAIN, DIR_RSIG_GAIN2, DIR_CEN;
	s32 DIR_THRLOW;
	s32 RSIG_CEN;
	s32 BGAIN;
	s32 w, h, num;
	s32 dir_cen_rad;

	w = para->w;
	h = para->h;

	if (w >= 32 && h >= 32) {
		/* float AUTO_LV_R[11] = {0.2f, 0.4f, 0.7f, 0.8f, 0.9f, */
		/*    1.0f, 1.1f, 1.2f, 1.3f, 1.4f, 1.5f}; */
		s32 AUTO_LV_R[11] = {2, 4, 7, 8, 9, 10, 11, 12, 13, 14, 15};

		para->dir_center = 128;
		para->dir_rsig_gain = 100;
		para->sig3 = 255;
		para->dir_thrlow = 64;
		para->dir_thrhigh = 192;
		dir_cen_rad = (para->dir_thrhigh - para->dir_thrlow + 1) / 2;
		SIG3 = para->sig3;

		num = w * h;
		sum = regs[1];
		avg = sum / num;

		blks = regs[16];
		blkn = regs[17];
		blkn = (blkn == 0) ? 1 : blkn;
		blkm = blks * 100 / blkn;
		blkr = blkn * 100 / cal_blks(w, h, para->xst, para->yst);

		/* set default para for different size */
		if (abs(w - 1280) < 192 ||
			 abs(w - 1920) < 288 ||
			 abs(w - 3840) < 576) {
			SIG = 0;
			SIG2 = 25; /* 10 */
		}	else if ((w == 720 || w == 704 || w == 640) &&
			(h == 576 || h == 480)) {
			SIG = 1;
			SIG2 = 75; /* 10 */
		} else if (w >= 480) {
			SIG = 1;
			SIG2 = 30;
		}	else {
			SIG = 2;
			SIG2 = 40;
		}

		DIR_THRLOW = para->dir_thrlow;
		DIR_RSIG_GAIN = para->dir_rsig_gain;
		DIR_CEN = para->dir_center;
		RSIG_CEN = SIG2 + DIR_RSIG_GAIN * (DIR_CEN - DIR_THRLOW) / 16;
		RSIG_CEN = min(SIG3, RSIG_CEN);
		DIR_RSIG_GAIN2 = (SIG3 - RSIG_CEN + dir_cen_rad / 2) /
				dir_cen_rad;

		if (avg < 35) {
			SIG2 = max(0, min(255, SIG2 * 3 / 2));
			DIR_RSIG_GAIN += 20;
			DIR_CEN = max(0, min(255, DIR_CEN - 32));
		} else if (avg >= 35 && avg < 65) {
			/* remain */
		} else if (avg >= 65 && avg <= 140) {
			if (w >= (1280)) {
				SIG = max(0, SIG - 1);
				SIG2 = max(0, min(255, SIG2 * 2 / 3));
			} else {
				SIG2 = max(0, min(255, SIG2 * 3 / 4));
			}

			DIR_RSIG_GAIN  = max(0, DIR_RSIG_GAIN - 20);
			DIR_CEN  = max(0, min(255, DIR_CEN + 32));
		}

		if (w <= 2048) {
			if (blkm >= 20)
				DIR_RSIG_GAIN = min(255,
					DIR_RSIG_GAIN * (blkm - 20) / 20);
			if (blkr >= 5) {
				SIG2 = SIG2 + (blkr - 5) * 8;
				SIG2 = max(15, min(30, SIG2));
				if (blkm < 30)
					SIG2 = SIG2 * (blkm + 15) / 30;
				else if (blkm < 60)
					SIG2 = SIG2 * (blkm + 15) / 30;
				else
					SIG2 = SIG2 * 2;
				SIG2 = min(100, SIG2);
			}
#if 0
			if (blkr < 5)
				BGAIN = 10 + 5 * (blkr + 1) / 2;
			else if (blkr < 10)
				BGAIN = 22 + 9 * (blkr - 5) / 2;
			else if (blkr < 15)
				BGAIN = 43 + 3 * (blkr - 10);
			else if (blkr < 30)
				BGAIN = 58 + 6 * (blkr - 15) / 5;
			else if (blkr < 60)
				BGAIN = 76 + 1 * (blkr - 30) / 3;
			else
				BGAIN = 85;
#else
			if (blkr < 5)
				BGAIN = 10 + (16 - 10) * blkr / (10 - 5);
			else if (blkr < 10)
				BGAIN = 16 +
					(32 - 16) * (blkr - 10) / (15 - 10);
			else if (blkr < 15)
				BGAIN = 32 +
					(36 - 32) * (blkr - 15) / (30 - 15);
			else if (blkr < 30)
				BGAIN = 36 +
					(40 - 36) * (blkr - 30) / (60 - 30);
			else if (blkr < 60)
				BGAIN = 40;
			else
				BGAIN = 50;
#endif
		} else {
			BGAIN = 0x10;
			/* default low gain in 4k size : 0x14 is original */
		}
		/* for level control */
		auto_lvl = max(0, min(auto_lvl, 10));
		auto_level_ratio = AUTO_LV_R[auto_lvl];

		SIG2 = min(255, (5 + SIG2 * auto_level_ratio)/10);
		BGAIN = min(127, (5 + BGAIN * auto_level_ratio)/10);
		DIR_RSIG_GAIN = min(255,
			(5 + DIR_RSIG_GAIN * auto_level_ratio)/10);
		para->bgain = BGAIN;
		para->sig = SIG;
		para->sig2 = SIG2;
		para->dir_rsig_gain = DIR_RSIG_GAIN;
		para->dir_rsig_gain2 = DIR_RSIG_GAIN2;
		para->rsig_cen = RSIG_CEN;

		__inf("dns set sig/sig2/bgain: %d, %d, %d\n", SIG, SIG2, BGAIN);

		if (SIG2 == 0) {
			SIG2 = 1;
			__wrn("%s: SIG2==0!\n", __func__);
		}

	}
}

s32 de_dns_tasklet(u32 disp, u32 chn,
			u32 frame_cnt)
{
	struct de_dns_private *priv = &dns_priv[disp][chn];
	struct de_dns_para *para = &(priv->dns_para);

	if (para->en) {
		u32 regs_read[18] = {0};
		__inf("dns tasklet start\n");
		de_dns_read_iqa_regs(disp, chn, regs_read);
		dns_para_adapt(para, regs_read);
		de_dns_para_apply(disp, chn, para);
		__inf("dns tasklet end\n");
	}
	return 0;
}
