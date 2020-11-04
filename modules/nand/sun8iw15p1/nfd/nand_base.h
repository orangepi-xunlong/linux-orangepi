/*
 * nand_base.h for  SUNXI NAND .
 *
 * Copyright (C) 2016 Allwinner.
 *
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */
#ifndef _NAND_BASE_H_
#define _NAND_BASE_H_

#include "nand_blk.h"
#include "nand_dev.h"

/*****************************************************************************/
extern int nand_type;

extern struct nand_blk_ops mytr;
extern struct _nand_info *p_nand_info;

extern int init_blklayer(void);
extern int init_blklayer_for_dragonboard(void);
extern void exit_blklayer(void);
extern void set_cache_level(struct _nand_info *nand_info,
			    unsigned short cache_level);
extern void set_capacity_level(struct _nand_info *nand_info,
			       unsigned short capacity_level);
extern __u32 nand_wait_rb_mode(void);
extern __u32 nand_wait_dma_mode(void);
extern void do_nand_interrupt(unsigned int no);
extern void tdo_nand_interrupt(unsigned int no);

extern void print_nftl_zone(void *zone);
extern int NAND_get_storagetype(void);
extern int NAND_Get_Dragonboard_Flag(void);
extern int nand_thread(void *arg);
extern int NAND_CheckBoot(void);
extern int TNAND_CheckBoot(void);
extern int tlc_nand_thread(void *arg);


extern int tnftl_struct_init(void);
extern uint32 tflush_write_cache(void);
extern int tnftl_exit(void);

int test_mbr(uchar *data);
extern int NAND_Print_DBG(const char *fmt, ...);
extern __u32 NAND_GetMaxChannelCnt(void);

extern int nand_clean_zone_table(void *p);
extern int nand_find_zone_table(void *p);
#define BLK_ERR_MSG_ON

#endif

