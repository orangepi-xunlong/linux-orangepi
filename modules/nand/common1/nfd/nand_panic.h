/* SPDX-License-Identifier: GPL-2.0 */

#ifndef __NAND_PANIC_H__
#define __NAND_PANIC_H__

#include "nand_blk.h"

#define PANIC_NAND_VERSION "0.4.0"

extern dma_addr_t panic_dma_handle;
extern void *panic_buf_handle;
extern void *panic_map_buff;

void panic_mark_enable(void);
int is_panic_enable(void);
int is_on_panic(void);
int panic_init_part(const char *name, unsigned int off, unsigned int size);
int panic_read(struct _nftl_blk *nftl_blk, uint32 start_sector,
		uint32 sector_num, uchar *buf);
int panic_write(struct _nftl_blk *nftl_blk, uint32 start_sector,
		uint32 sector_num, uchar *buf);
int critical_dev_nand_read(const char *dev, size_t sec_off,
		size_t sec_cnt, char *buf);
int critical_dev_nand_write(const char *dev, size_t sec_off,
		size_t sec_cnt, char *buf);

#if IS_ENABLED(CONFIG_PSTORE_BLK)
dma_addr_t nand_panic_dma_map(__u32 rw, void *buf, __u32 len);
void nand_panic_dma_unmap(__u32 rw, dma_addr_t dma_handle, __u32 len);
int nand_panic_init(struct _nftl_blk *nftl_blk);
int nand_support_panic(void);
#else
static dma_addr_t __maybe_unused nand_panic_dma_map(__u32 rw, void *buf, __u32 len)
{
	return -1;
}

static void __maybe_unused nand_panic_dma_unmap(__u32 rw, dma_addr_t dma_handle, __u32 len)
{
	return;
}

static int __maybe_unused nand_support_panic(void)
{
	return -1;
}

static int __maybe_unused nand_panic_init(struct _nftl_blk *nftl_blk)
{
	return -1;
}
#endif

#endif
