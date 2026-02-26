/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
* redeposit startup IO data
*
* Copyright (C) 2015 AllWinnertech Ltd.
* Author: libiao <libiao@allwinnertech>
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License version 2 as
* published by the Free Software Foundation.
*
* This program is distributed "as is" WITHOUT ANY WARRANTY of any
* kind, whether express or implied; without even the implied warranty
* of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*/
#include <linux/platform_device.h>
#include <linux/spinlock.h>
#include <linux/of_address.h>
#include <linux/of_gpio.h>
#include <linux/of_platform.h>

#ifndef __REDEPOSIT_H__
#define __REDEPOSIT_H__

#define RDPST_DRIVER_NAME "redeposit"
#define RDPST_DRIVER_RIVISION "v0.1 2022-05-16 14:28"
#define RDPST_DRIVER_VERSION ""RDPST_DRIVER_NAME" Driver(" RDPST_DRIVER_RIVISION ")"

#ifndef BIO_MAX_PAGES
#define BIO_MAX_PAGES		256
#endif

#define REDEPOSIT_MEM_SIZE(nr)	((nr) * 128 * 1024 * 1024)
#define REDEPOSIT_MAGIC		0x89119800
#define REDEPOSIT_MAP_MAX_SIZE		(BIO_MAX_PAGES * PAGE_SIZE)

#define RDPST_SECT_SIZE		512
#define SECT_TO_PAGE(index)	((index) / (PAGE_SIZE / RDPST_SECT_SIZE))
#define PAGE_TO_SECT(index)	((index) * (PAGE_SIZE / RDPST_SECT_SIZE))

enum head_status {
	H_STATUS_NONE,
	H_STATUS_HANDLE,
	H_STATUS_WAIT_NEXT,
	H_STATUS_ERROR,
	H_STATUS_FINISH
};

enum redeposit_status {
	STATUS_NONE,
	STATUS_BUILD,
	STATUS_MAP,
	STATUS_DATA,
	STATUS_ERROR,
	STATUS_RELEASE,
	STATUS_FINISH
};

enum redeposit_crc {
	CRC_HEAD,
	CRC_MAP,
	CRC_DATA
};

struct redeposit_part {
	u32 start;
	u32 end;
};

#define RO_PART_NUM 15
struct redeposit_head {
	u32 head_crc;
	u32 magic;
	u32 ver;
	u32 map_addr;
	u32 map_num;
	u32 map_size;
	u32 map_crc;
	u32 data_addr;
	u32 data_num;
	u32 data_crc;
	u32 status;
	u32 ro_part_num;
	struct redeposit_part ropart[RO_PART_NUM];
};

struct redeposit_map {
	/*origin addr*/
	u32 log_start_sec;
	/*redeposit area addr*/
	u32 phy_start_sec;
	u32 num_sec;
};

struct request_config {
	/*sync:from block devce request.async: from redeposit reqeust*/
	bool is_async;
	/*request lanch in tasklet*/
	bool is_bkgd;
	/*use in atomic context,it means no use map dma in tasklet/irq*/
	bool is_atomic;
};

struct redeposit_data {
	struct page	*bv_page;
	unsigned int	bv_len;
	unsigned int	bv_offset;
	unsigned int	status;
};

#define SetDataUpdate(data) (data->status = 1)
#define DataUpdate(data) (data->status == 1)

struct redeposit_iostat {
	ktime_t start;
	ktime_t start_build;
	ktime_t start_map;
	ktime_t start_data;
	ktime_t start_hit;
	ktime_t consuming;
	ktime_t time_build;
	ktime_t time_map;
	ktime_t time_data;
	ktime_t time_hit;
	u32 nr_page[8];
};

struct redeposit_info {
#define FLAG_NONE (0)
#define FLAG_READ (1)
#define FLAG_FLUSH (2)
#define FLAG_WRITE (3)
	unsigned long handle_flag;
	enum head_status hstatus;
#define DEV_WAIT_WAKE	0
#define DEV_WAKEUP_DEV	1
#define	DEV_GET_HANDLE	2
	unsigned int status_dev;
	unsigned long aread;
	dma_addr_t phy_addr;
	dma_addr_t mem_size;
	void *buf;
	struct redeposit_map *map_buf;
	u32 map_index;
	struct redeposit_data *data_buf;
	u32 data_index;
	u32 cache_bytes;
	struct redeposit_head *head;
	struct block_device *b_bdev;
	struct redeposit_part re_part;
	/*Cannot guarantee that partition start is aligned with 4K*/
	/*so,we align by zone offset of 0*/
	sector_t off_part_sect;
	u32 status;
	u32 cache_init;
	struct address_space *mapping;
	struct address_space i_data;
	struct platform_device *pdev;

	 struct delayed_work redeposit_flush_work;
	 struct delayed_work redeposit_build_work;
	 struct delayed_work redeposit_irq_work;
	struct delayed_work redeposit_cache_work;
	/*sys node */
	struct device_attribute handle_status;
	struct device_attribute bdev_status;
	struct device_attribute allow_read;
	struct device_attribute debug;
	struct device_attribute record_part;

	struct page **pages;

	struct xarray i_xarr;

	u32 cache_map_index;
	u32 cache_map_off;

	u32 cache_hit;

	wait_queue_head_t       wait;
	wait_queue_head_t       wait_dev;
	u32 write_io;
	/*of node*/
	u32 nr_hz;
	u32 nr_mem;
	u32 nr_blk;
	u32 nr_rd2;
	u32 nr_rd1;
	/*record hit and continue to submit_bio*/
#define NR_MAX_HIT 2048
	u32 nr_hit;
	u32 off_pos;
	u32 do_cache;

	int (*read_flash)(void *flash, void *request, struct request_config *conf);
	void *(*build_request)(void *flash, void *buf, u32 sg_len, u32 addr, u32 nr_byte);
	struct sg_table sgtable;
	u32 sg_len;
	struct bio *bio;
	void *flash;
	u32 max_segment;

	unsigned int build_done;
	struct redeposit_iostat iostat;
	u32 ro_part_num;
#define RDPST_MAX_PATH 35
	u8 ro_path[RO_PART_NUM][RDPST_MAX_PATH];
};

#if IS_ENABLED(CONFIG_SUNXI_REDEPOSIT)
struct redeposit_info *get_redeposit_info(void *flash);

bool redeposit_read_data(struct redeposit_info *data,  struct scatterlist *sg, unsigned int sg_len, unsigned int blk_addr, unsigned int blocks);

bool redeposit_write_data(struct redeposit_info *data,  struct scatterlist *sg, unsigned int sg_len, unsigned int blk_addr, unsigned int blocks);

void redeposit_done(void *info, s32 error);

struct redeposit_info *redeposit_attach_host(void *flash, u32 max_segment,
						void *(*build_request)(void *flash, void *buf, u32 sg_len, u32 addr, u32 nr_byte),
						int (*read_flash)(void *flash, void *request, struct request_config *conf),
						void **async_done);

void write_io_num(struct redeposit_info *info);

void redeposit_wake_up_dev(struct redeposit_info *info, sector_t sect, sector_t blocks);


#else /* IS_ENABLED(CONFIG_SUNXI_REDEPOSIT)*/
static inline struct redeposit_info *__must_check get_redeposit_info(void *flash)
{
	return NULL;
}

static inline bool __must_check redeposit_read_data(struct redeposit_info *data,  struct scatterlist *sg, unsigned int sg_len, unsigned int blk_addr, unsigned int blocks)
{
	return false;
}

static inline bool redeposit_write_data(struct redeposit_info *data,  struct scatterlist *sg, unsigned int sg_len, unsigned int blk_addr, unsigned int blocks)
{
	return false;
}

static inline void redeposit_done(void *info, s32 error)
{
}

static inline struct redeposit_info *redeposit_attach_host(void *flash, u32 max_segment,
						void *(*build_request)(void *flash, void *buf, u32 sg_len, u32 addr, u32 nr_byte),
						int (*read_flash)(void *flash, void *request, struct request_config *conf),
						void **async_done)
{
	return NULL;
}

static inline void write_io_num(struct redeposit_info *info)
{
}

static inline void redeposit_wake_up_dev(struct redeposit_info *info, sector_t sect, sector_t blocks)
{
}

#endif /* IS_ENABLED(CONFIG_SUNXI_REDEPOSIT)*/

#endif /*__REDEPOSIT_H__*/
