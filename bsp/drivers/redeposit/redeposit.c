// SPDX-License-Identifier: GPL-2.0-or-later
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */

#include <linux/module.h>
#include <linux/device.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/io.h>
#include <linux/list_sort.h>
#include <linux/of_address.h>
#include <linux/of_device.h>
#include <linux/blkdev.h>
#include <linux/backing-dev.h>
#include <linux/genhd.h>
#include <linux/xarray.h>
#include <linux/swap.h>
#include <linux/gfp.h>
#include <linux/random.h>
#include <linux/irq.h>
#include <linux/crc32.h>

#include "redeposit.h"
static struct redeposit_info *info;

static inline void *redeposit_get_data(struct redeposit_info *info, u32 index)
{
	return &(info->data_buf[index]);
}

static void *redeposit_get_map(struct redeposit_info *info, u32 index)
{
	struct redeposit_map *map = (struct redeposit_map *)((struct redeposit_head *)(page_to_virt(info->pages[0])) + 1);

	return (map + index);
}

static void redeposit_set_data(struct redeposit_info *info, u32 index, void *buf, u32 len)
{
	struct redeposit_data *data;
	u32 nr_sect = len / RDPST_SECT_SIZE, i;
	u8 *data_buf;

	BUG_ON(len % RDPST_SECT_SIZE);

	for (i = 0; i < nr_sect; i++) {
		data = redeposit_get_data(info, index);
		index++;
		data_buf = (u8 *)page_to_virt(data->bv_page) + data->bv_offset;
		dev_dbg(&info->pdev->dev, "%s:index:%ld, len:%ld, buf:%px, data_buf:%px, page:%px\n",
			__func__, index, len, buf, data_buf, data->bv_page);
		memcpy(data_buf, ((u8 *)buf + i * RDPST_SECT_SIZE), data->bv_len);
	}
	info->data_index += nr_sect;
}

static void *redeposit_get_head(struct redeposit_info *info)
{
	return ((struct redeposit_head *)(page_to_virt(info->pages[0])));
}

static struct page *redeposit_get_page(struct redeposit_info *info, u32 index)
{
	u32 off_pages, off_page;
	struct page *page;

	off_pages = index / BIO_MAX_PAGES;
	off_page = index % BIO_MAX_PAGES;
	page = (struct page *)(info->pages[off_pages] + off_page);

	return page;
}

static void redeposit_build_data(struct redeposit_info *info)
{
	struct redeposit_data *data;
	struct page *page;
	u32 i, j, nr_page, nr_map_page;

	/*Only enough accommodation has been applied for the number of redeposit_data
	 * (data->mem_size - REDEPOSIT_MAP_MAX_SIZE) / RDPST_SECT_SIZE;
	 * */
	nr_page = (info->mem_size - REDEPOSIT_MAP_MAX_SIZE) / PAGE_SIZE;
	nr_map_page = info->head->map_size / PAGE_SIZE;
	BUG_ON((PAGE_SIZE % RDPST_SECT_SIZE) || (info->head->map_size % PAGE_SIZE));

	for (i = 0; i < nr_page; i++) {
		page = (struct page *)redeposit_get_page(info, (i + nr_map_page));
		dev_dbg(&info->pdev->dev, "%s:i:%ld, nr_page:%ld, nr_map:%ld\n", __func__, i, nr_page, nr_map_page);
		for (j = 0; j < PAGE_TO_SECT(1); j++) {
			data = redeposit_get_data(info, i * PAGE_TO_SECT(1) + j);
			data->bv_page = page;
			data->bv_len = RDPST_SECT_SIZE;
			data->bv_offset = j * RDPST_SECT_SIZE;
			data->status = 0;
		}
	}
}

static bool is_redeposit_addr(struct redeposit_info *info, sector_t sect, sector_t blocks)
{
	struct redeposit_head *head = (struct redeposit_head *)redeposit_get_head(info);
	u32 is_ro = 0;
	int i;

	for (i = 0; !is_ro && i < head->ro_part_num; i++) {
		is_ro = (!!((head->ropart[i].start <= sect) && (sect + blocks <= head->ropart[i].end)));
	}

	return is_ro;
}

static inline sector_t daddr_remap(struct redeposit_info *data, u32 index)
{
	return index << 3;
}

static inline pgoff_t laddr_to_lindex(struct redeposit_info *data, u32 laddr)
{
	return (laddr >> 3);
}

static inline bool is_over_cap(struct redeposit_info *info, unsigned int blocks)
{
	u32 map_size = sizeof(struct redeposit_map) * (info->map_index + 1) + sizeof(struct redeposit_head);
	u32 data_size = (info->data_index + blocks) * RDPST_SECT_SIZE;
	map_size = (map_size + PAGE_SIZE - 1) / PAGE_SIZE * PAGE_SIZE;

	return ((map_size > REDEPOSIT_MAP_MAX_SIZE) || ((data_size + REDEPOSIT_MAP_MAX_SIZE) > info->mem_size));
}

static inline bool is_redeposit_nrblk(struct redeposit_info *info, unsigned int blocks)
{
	return (!!(blocks <= (8 * 8 * info->nr_blk)));
}

static inline bool is_handle_read(struct redeposit_info *info)
{
	return (info->handle_flag == FLAG_READ);
}

static inline bool is_handle_write(struct redeposit_info *info)
{
	return (info->handle_flag == FLAG_WRITE);
}

static inline bool is_handle_flush(struct redeposit_info *info)
{
	return (info->handle_flag == FLAG_FLUSH);
}

static inline void set_handle(struct redeposit_info *info, unsigned int flag)
{
	info->handle_flag = flag;
}

struct redeposit_info *get_redeposit_info(void *flash)
{
	return ((info && info->flash == flash) ? info : NULL);
}
EXPORT_SYMBOL_GPL(get_redeposit_info);

void end_redeposit_bio_write(struct bio *bio)
{
	bio_put(bio);
}

static u32 redeposit_head_crc(struct redeposit_info *info)
{
	struct page *page = redeposit_get_page(info, 0);
	struct redeposit_head *head = page_to_virt(page);
	u32 crc;

	crc = crc32(REDEPOSIT_MAGIC, (&head->magic), (PAGE_SIZE - sizeof(head->head_crc)));

	return crc;
}

static u32 redeposit_map_crc(struct redeposit_info *info)
{
	int i;
	u32 crc = REDEPOSIT_MAGIC;
	struct redeposit_map *map;

	for (i = 0; i < info->map_index; i++) {
		map = redeposit_get_map(info, i);
		crc = crc32(crc, map, sizeof(struct redeposit_map));
	}

	return crc;
}

static u32 redeposit_data_crc(struct redeposit_info *info)
{
	struct redeposit_data *data;
	u32 crc = REDEPOSIT_MAGIC;
	u8 *buf;
	int i;

	for (i = 0; i < info->data_index; i++) {
		data = redeposit_get_data(info, i);
		buf = (u8 *)page_to_virt(data->bv_page) + data->bv_offset;
		crc = crc32(crc, buf, data->bv_len);
		dev_dbg(&info->pdev->dev, "i:%ld crc:%ld\n", i, crc);
	}

	return crc;
}

static u32 redeposit_crc(struct redeposit_info *info, enum redeposit_crc crc_type)
{
	u32 crc;

	if (crc_type == CRC_HEAD) {
		crc = redeposit_head_crc(info);
	} else if (crc_type == CRC_MAP) {
		crc = redeposit_map_crc(info);
	} else
		crc = redeposit_data_crc(info);

	return crc;
}

static void redeposit_release_info(struct redeposit_info *info)
{
	u32 nr_bios, nr_page, i;

	nr_bios = (info->mem_size / PAGE_SIZE + BIO_MAX_PAGES - 1) / BIO_MAX_PAGES;

	for (i = 0; i < nr_bios ; i++) {
		nr_page = (i == nr_bios -1) ? ((info->mem_size / PAGE_SIZE - 1) % BIO_MAX_PAGES + 1):(BIO_MAX_PAGES);
		if (info->pages[i]) {
			free_pages((unsigned long)(page_to_virt(info->pages[i])), get_order(nr_page * PAGE_SIZE));
			continue;
		}
		break;
	}
	vfree(info->data_buf);
	sg_free_table(&info->sgtable);
	kfree(info->pages);
	info->flash = NULL;
	info->status = STATUS_RELEASE;
	dev_info(&info->pdev->dev, "release info\n");
}

static bool redeposit_build_part(struct redeposit_info *info)
{
	struct block_device *ro_bdev[RO_PART_NUM];
	struct redeposit_head *head = (struct redeposit_head *)redeposit_get_head(info);
	int i = 0;

	head->ro_part_num = 0;
	info->b_bdev = NULL;
	for (i = 0; i < info->ro_part_num; i++) {
		ro_bdev[i] = blkdev_get_by_path(info->ro_path[i], 0, info);

		if (IS_ERR(ro_bdev[i])) {
			dev_err(&info->pdev->dev, "get ro ropart failed:ropart:%s,%ld\n",
				info->ro_path[i], ro_bdev[i]);
				goto get_ropart_failed;
		}

		head->ropart[i].start = get_start_sect(ro_bdev[i]);
		head->ropart[i].end = bdev_nr_sectors(ro_bdev[i]) + get_start_sect(ro_bdev[i]);
		dev_info(&info->pdev->dev, "ropart:%s, start:%ld, end:%ld\n",
			 info->ro_path[i], head->ropart[i].start, head->ropart[i].end);

		if (i == 0) {
			/*have to get the disk for flush all data*/
			info->b_bdev = blkdev_get_by_dev(disk_devt(ro_bdev[i]->bd_disk), FMODE_READ | FMODE_WRITE, info);
			if (IS_ERR(info->b_bdev)) {
				dev_err(&info->pdev->dev, "get dev failed:%ld, %lx\n",
					info->b_bdev, disk_devt(ro_bdev[i]->bd_disk));
				goto get_dev_failed;
			}
			dev_info(&info->pdev->dev, "get disk successfully:%px, %lx\n", info->b_bdev, disk_devt(ro_bdev[i]->bd_disk));

		}
		head->ro_part_num++;
		blkdev_put(ro_bdev[i], 0);
	}

	return true;

get_ropart_failed:
	if (!IS_ERR(info->b_bdev)) {
		blkdev_put(info->b_bdev, FMODE_READ | FMODE_WRITE);
		info->b_bdev = NULL;
	}
get_dev_failed:
	blkdev_put(ro_bdev[0], 0);
	return false;
}

static void redeposit_flush_data(struct redeposit_info *info, enum head_status status)
{
	struct page *page;
	int nr_pages, all_pages, nr_map_page, nr_map_off, nr_page_off;
	int nr_bios;
	int i, j, k, err;
	u32 head_crc, map_crc, data_crc, tmp_head_crc, tmp_map_crc, tmp_data_crc;
	sector_t blk_addr;
	struct redeposit_map *map;
	struct redeposit_data *data;
	struct bio *bio;
	u32 data_sec_num = 0;

	err = redeposit_build_part(info);
	if (unlikely(!err)) {
		dev_err(&info->pdev->dev, "build part failed:%ld\n", info->b_bdev);
		goto build_part_err;
	}

	blk_addr = get_start_sect(info->b_bdev);

	/*sync write redeposit head+map+data to redeposit partition*/
	/*fill the head of redeposit_info*/
	info->head->data_addr = get_start_sect(info->b_bdev) + (sizeof(struct redeposit_head) +
			sizeof(struct redeposit_map) * info->map_index + PAGE_SIZE - 1) / PAGE_SIZE;
	info->head->data_num = info->data_index;
	info->head->map_num = info->map_index;
	info->head->status = H_STATUS_HANDLE;
	info->head->magic = REDEPOSIT_MAGIC;
	map_crc = info->head->map_crc = redeposit_crc(info, CRC_MAP);
	data_crc = info->head->data_crc = redeposit_crc(info, CRC_DATA);
	head_crc = info->head->head_crc = redeposit_crc(info, CRC_HEAD);
	nr_map_off = info->head->map_size / PAGE_SIZE;
	nr_map_page = (sizeof(struct redeposit_head) + sizeof(struct redeposit_map) * info->map_index + PAGE_SIZE - 1) / PAGE_SIZE;

	all_pages = (sizeof(struct redeposit_head) + sizeof(struct redeposit_map) * info->map_index + PAGE_SIZE - 1) / PAGE_SIZE + SECT_TO_PAGE(info->data_index + PAGE_TO_SECT(1) - 1);

	nr_bios = (all_pages + BIO_MAX_PAGES -1) / BIO_MAX_PAGES;

	if (status == H_STATUS_WAIT_NEXT)
		goto submit_end;

	dev_info(&info->pdev->dev, "F:dindex:%ld, mindex:%ld, all_pages:%ld, nr_bios:%ld\n",
		 info->data_index, info->map_index, all_pages, nr_bios);
	dev_info(&info->pdev->dev, "no:%ld, start:%ld,p:%ld\n",
		 info->b_bdev->bd_partno, info->re_part.start, get_start_sect(info->b_bdev));

	for (i = 0; i < nr_bios; i++) {
		blk_addr = PAGE_TO_SECT(i * BIO_MAX_PAGES) + info->re_part.start;
		nr_pages = ((i == (nr_bios - 1)) ? ((all_pages - 1) % BIO_MAX_PAGES + 1) : BIO_MAX_PAGES);
		bio = bio_alloc(GFP_KERNEL, nr_pages);
		if (bio == NULL)
			goto alloc_bio_failed;
		bio_set_dev(bio, info->b_bdev);
		bio->bi_iter.bi_sector = blk_addr;
		bio->bi_end_io = end_redeposit_bio_write;
		bio->bi_private = info;
		bio_set_op_attrs(bio, REQ_OP_WRITE, REQ_SYNC);
		for (j = 0; j < nr_pages; j++) {
			nr_page_off = i * BIO_MAX_PAGES + j;
			nr_page_off = ((nr_page_off >= nr_map_page) ? (nr_page_off + nr_map_off - nr_map_page) : nr_page_off);
			page = redeposit_get_page(info, nr_page_off);
			dev_dbg(&info->pdev->dev, "F:i:%ld, j:%ld, nr_pages:%ld, page:%px, blk_addr:%ld\n", i, j, nr_pages, page, blk_addr);
			bio_add_page(bio, page, PAGE_SIZE, 0);
		}
		err = submit_bio_wait(bio);
		bio_put(bio);
		if (err) {
			status = H_STATUS_ERROR;
			goto submit_end;
		}
	}

	/*modify map_size then remap data_buf*/
	info->head->map_size = nr_map_page * PAGE_SIZE;
	redeposit_build_data(info);

	/*reread redeposit partition data to calculate crc*/
	for (i = 0; i < nr_bios; i++) {
		blk_addr = PAGE_TO_SECT(i * BIO_MAX_PAGES) + info->re_part.start;
		nr_pages = ((i == (nr_bios - 1)) ? ((all_pages - 1) % BIO_MAX_PAGES + 1) : BIO_MAX_PAGES);
		bio = bio_alloc(GFP_KERNEL, nr_pages);
		if (bio == NULL)
			goto alloc_bio_failed;
		bio_set_dev(bio, info->b_bdev);
		bio->bi_iter.bi_sector = blk_addr;
		bio->bi_end_io = end_redeposit_bio_write;
		bio->bi_private = info;
		bio_set_op_attrs(bio, REQ_OP_READ, 0);
		for (j = 0; j < nr_pages; j++) {
			nr_page_off = i * BIO_MAX_PAGES + j;
			page = redeposit_get_page(info, nr_page_off);

			/*we have to sure all page reset to 0 and reset databuf*/
			memset(page_to_virt(page), 0, PAGE_SIZE);

			dev_dbg(&info->pdev->dev, "F:i:%ld, j:%ld, nr_pages:%ld, page:%px, blk_addr:%ld\n", i, j, nr_pages, page, blk_addr);
			bio_add_page(bio, page, PAGE_SIZE, 0);
		}
		err = submit_bio_wait(bio);
		bio_put(bio);
		if (err) {
			status = H_STATUS_ERROR;
			goto submit_end;
		}
	}

	tmp_head_crc = redeposit_crc(info, CRC_HEAD);
	dev_info(&info->pdev->dev, "head_crc orgin:%lu, flash:%lu, cal:%lu\n", head_crc, info->head->head_crc, tmp_head_crc);
	tmp_map_crc = redeposit_crc(info, CRC_MAP);
	dev_info(&info->pdev->dev, "map_crc orgin:%lu, flash:%lu, cal:%lu\n", map_crc, info->head->map_crc, tmp_map_crc);
	tmp_data_crc = redeposit_crc(info, CRC_DATA);
	dev_info(&info->pdev->dev, "data_crc orgin:%lu, flash:%lu, cal:%lu\n", data_crc, info->head->data_crc, tmp_data_crc);

	/*reread original data to calculate crc, compare with redeposit data crc*/
	for (i = 0; i < info->map_index; i++) {
		map = redeposit_get_map(info, i);
		all_pages = SECT_TO_PAGE((map->num_sec + PAGE_TO_SECT(1) - 1));
		nr_bios = (all_pages + BIO_MAX_PAGES -1) / BIO_MAX_PAGES;
		for (k = 0; k < nr_bios; k++) {
			nr_pages = ((k == (nr_bios - 1)) ? ((all_pages - 1) % BIO_MAX_PAGES + 1) : BIO_MAX_PAGES);

			dev_dbg(&info->pdev->dev, "nr_pages:%ld, num_sec:%ld, i:%ld, index:%ld, data_num:%ld\n", nr_pages, map->num_sec, i, info->map_index, data_sec_num);
			bio = bio_alloc(GFP_KERNEL, nr_pages);
			if (bio == NULL)
				goto alloc_bio_failed;
			bio_set_dev(bio, info->b_bdev);
			bio->bi_iter.bi_sector = (map->log_start_sec + k * PAGE_TO_SECT(BIO_MAX_PAGES));
			bio->bi_private = info;
			bio_set_op_attrs(bio, REQ_OP_READ, 0);

			for (j = 0; j < PAGE_TO_SECT(nr_pages); j++) {
				data = redeposit_get_data(info, data_sec_num);
				data_sec_num++;
				memset(((u8 *)page_to_virt(data->bv_page) + data->bv_offset), 0, data->bv_len);
				bio_add_page(bio, data->bv_page, data->bv_len, data->bv_offset);
			}
			err = submit_bio_wait(bio);
			bio_put(bio);
			if (err) {
				status = H_STATUS_ERROR;
				goto submit_end;
			}
		}
	}

	if (((head_crc != info->head->head_crc) || (head_crc != tmp_head_crc))
	    || ((map_crc != info->head->map_crc) || (map_crc != tmp_map_crc))
	    || ((data_crc != info->head->data_crc) || (data_crc != tmp_data_crc))) {
		dev_err(&info->pdev->dev, "head_crc orgin:%lu, flash:%lu, cal:%lu\n", head_crc, info->head->head_crc, tmp_head_crc);
		dev_err(&info->pdev->dev, "map_crc orgin:%lu, flash:%lu, cal:%lu\n", map_crc, info->head->map_crc, tmp_map_crc);
		dev_err(&info->pdev->dev, "data_crc orgin:%lu, flash:%lu, cal:%lu\n", data_crc, info->head->data_crc, tmp_data_crc);
		status = H_STATUS_ERROR;
		goto submit_end;
	}

	tmp_data_crc = redeposit_crc(info, CRC_DATA);
	if (data_crc != tmp_data_crc) {
		dev_err(&info->pdev->dev, "check memory stablity, data crc (orgin:%lu != cal:%lu)\n", data_crc, tmp_data_crc);
	}
	dev_info(&info->pdev->dev, "data crc orgin:%lu cal:%lu\n", data_crc, tmp_data_crc);

submit_end:
	/*write through redeposit head*/
	info->head->status = status;
	info->head->head_crc = redeposit_crc(info, CRC_HEAD);
	page = redeposit_get_page(info, 0);
	bio = bio_alloc(GFP_KERNEL, 1);
	if (bio == NULL) {
		dev_err(&info->pdev->dev, "alloc bio failed, flush redeposit head failed\n");
		goto alloc_bio_failed;
	}
	bio_set_dev(bio, info->b_bdev);
	bio->bi_iter.bi_sector = info->re_part.start;
	bio->bi_end_io = end_redeposit_bio_write;
	bio->bi_private = info;
	bio_set_op_attrs(bio, REQ_OP_WRITE, REQ_SYNC);
	bio_add_page(bio, page, PAGE_SIZE, 0);

	submit_bio_wait(bio);

	bio_put(bio);
alloc_bio_failed:
	blkdev_put(info->b_bdev, FMODE_READ | FMODE_WRITE);
build_part_err:
	redeposit_release_info(info);
	return;
}

static void redeposit_flush_handle(struct work_struct *work)
{
	struct redeposit_info *info = container_of(work, struct redeposit_info, redeposit_flush_work.work);

	redeposit_flush_data(info, info->hstatus);
}

static struct redeposit_data  *redeposit_find_get_data(struct xarray *i_xarr, pgoff_t offset)
{
	struct redeposit_data *data;
	XA_STATE(xas, i_xarr, offset);

	rcu_read_lock();
repeat:
	xas_reset(&xas);
	data = xas_load(&xas);
	if (xas_retry(&xas, data))
		goto repeat;

	if (!data || xa_is_value(data))
		goto out;


out:
	rcu_read_unlock();

	return data;

}
static int redeposit_add_data_to_cache(struct redeposit_data *data, struct xarray *i_xarr, pgoff_t offset)
{
	void *old;
	XA_STATE(xas, i_xarr, offset);

	do {
		xas_lock_irq(&xas);
		old = xas_load(&xas);
		if (old && !xa_is_value(old))
			xas_set_err(&xas, -EEXIST);
		xas_store(&xas, data);
		if (xas_error(&xas))
			goto unlock;

unlock:
		xas_unlock_irq(&xas);

	} while (xas_nomem(&xas, (__GFP_NOFAIL | __GFP_NORETRY | GFP_NOFS)));

	if (xas_error(&xas))
		goto error;

	return 0;

error:
	return xas_error(&xas);
}

/*record start blk, data,write to cache,write back when init ok*/
bool redeposit_write_data(struct redeposit_info *info, struct scatterlist *sg, unsigned int sg_len, unsigned int blk_addr, unsigned int blocks)
{
	struct redeposit_map *map;
	u8 *sg_buf = NULL;
	unsigned int sg_buf_len = 0;
	int i = 0;

	if (!info || !is_handle_write(info) || !info->build_done) {
		return false;
	}

	if (!info->build_done)
		pr_debug("redeposit:%ld--%ld", blk_addr, blocks);

	if (!is_redeposit_nrblk(info, blocks) || !is_redeposit_addr(info, blk_addr, blocks) || is_over_cap(info, blocks)) {
		return false;
	}
	map = redeposit_get_map(info, info->map_index);
	info->map_index++;
	map->log_start_sec = blk_addr;
	map->phy_start_sec = info->data_index;
	map->num_sec = blocks;

	for (i = 0; i < sg_len; i++) {
		sg_buf = sg_virt(&sg[i]);
		sg_buf_len = sg[i].length;
		dev_dbg(&info->pdev->dev, "W:dindex:%ld, mindex:%ld, num_sec:%ld, sg_buf:%px\n",
			info->data_index, info->map_index, map->num_sec, sg_buf);
		redeposit_set_data(info, info->data_index, sg_buf, sg_buf_len);
	}
	dev_dbg(&info->pdev->dev, "W:dindex:%ld, mindex:%ld, num_sec:%ld, psSec:%ld, blocks:%ld\n",
		info->data_index, info->map_index, map->num_sec, map->phy_start_sec, blocks);

	return true;
}
EXPORT_SYMBOL_GPL(redeposit_write_data);


void end_redeposit_bio_map(struct bio *bio)
{
	struct redeposit_info *info = bio->bi_private;
	struct redeposit_head *head;
	struct redeposit_map *map;
	struct page *page;
	pgoff_t index, end_index, j, page_index;
	u32 nr_map_pages, nr_update_page = 0;
	struct bio_vec *bv;
	struct bvec_iter_all iter_all;
	int i, k = 0;
	struct redeposit_data *data;

	info->iostat.start_map = ktime_get();
	if (bio->bi_status) {
		bio->bi_status = 0;
		info->status = STATUS_ERROR;
		set_handle(info, FLAG_NONE);
		dev_err(&info->pdev->dev, "bi_status:%ld\n", bio->bi_status);
		goto end_bio;
	}
	bio_for_each_segment_all(bv, bio, iter_all) {
		info->cache_bytes += bv->bv_len;
	}

	if (info->cache_bytes < REDEPOSIT_MAP_MAX_SIZE)
		goto end_bio;

	/*firstly, wake up build_cache to handle info*/
	info->status = STATUS_MAP;
	head = info->head;
	info->map_index = head->map_num;
	nr_map_pages = (sizeof(struct redeposit_head) + sizeof(struct redeposit_map) * info->map_index + PAGE_SIZE - 1) / PAGE_SIZE;
	info->head->map_size = nr_map_pages * PAGE_SIZE;

	dev_info(&info->pdev->dev, "%s--%d map_index:%ld, nr_map_pages:%ld, map_size:%ld!!\n", __func__, __LINE__, info->map_index, nr_map_pages, info->head->map_size);

	/*caculate the num of uptodated page*/
	nr_update_page = REDEPOSIT_MAP_MAX_SIZE / PAGE_SIZE - nr_map_pages;
	page_index = nr_map_pages;

	dev_dbg(&info->pdev->dev, "%s:map_off:%ld, map_index:%ld\n", __func__, info->cache_map_off, info->cache_map_index);
	/*build cache tree according to redeposit_map*/
	for (i = 0; i < info->map_index; i++) {
		map = redeposit_get_map(info, i);
		index =  map->log_start_sec;
		end_index = map->log_start_sec + map->num_sec;
		dev_dbg(&info->pdev->dev, "%s:lsect:%ld, esect:%ld, num_sect:%ld\n", __func__, map->log_start_sec, map->log_start_sec + map->num_sec, map->num_sec);
		dev_dbg(&info->pdev->dev, "%s:index:%ld, end_index:%ld, page_index:%ld, nr_map:%ld\n", __func__, index, end_index, page_index, nr_map_pages);

		for (j = index; j < end_index; j++) {
			if (k == 0) {
				page = redeposit_get_page(info, page_index++);
				/*of course, need to set the page to Uptodate, which page have been read*/
				if (nr_update_page) {
					SetPageUptodate(page);
					nr_update_page--;
				} else
					goto end_bio;
			}
			dev_dbg(&info->pdev->dev, "%s:i:%ld, j:%ld, page:%px\n", __func__, i, j, page);
			data = redeposit_get_data(info, info->data_index++);
			data->bv_page = page;
			data->bv_len = RDPST_SECT_SIZE;
			data->bv_offset = k * RDPST_SECT_SIZE;
			SetDataUpdate(data);
			redeposit_add_data_to_cache(data, &info->i_xarr, j);
			k = (k + 1) % PAGE_TO_SECT(1);
			info->cache_map_off++;
		}
		info->cache_map_off = 0;
		info->cache_map_index++;
	}

end_bio:
	bio_put(bio);
	wake_up(&info->wait);
	dev_info(&info->pdev->dev, "%s:map_off:%ld, map_index:%ld\n", __func__, info->cache_map_off, info->cache_map_index);
	info->iostat.time_map += ktime_sub(ktime_get(), info->iostat.start_map);
}

static void end_redeposit_bio_data(struct bio *bio)
{
	struct redeposit_info *info = bio->bi_private;
	struct page *page;
	struct bio_vec *bv;
	struct bvec_iter_all iter_all;
	struct redeposit_data *data;
	int k, index, off;
	struct redeposit_map *map;

	info->iostat.start_data = ktime_get();
	if (bio->bi_status) {
		bio->bi_status = 0;
		info->status = STATUS_ERROR;
		set_handle(info, FLAG_NONE);
		dev_err(&info->pdev->dev, "bi_status:%ld\n", bio->bi_status);
		goto end_bio;
	}
	index = bio->bi_iter.bi_sector;
	/*have to use lock to ensure*/
	bio_for_each_segment_all(bv, bio, iter_all) {
		page = bv->bv_page;
		off = bv->bv_offset / RDPST_SECT_SIZE;
		BUG_ON(off);
		/* PG_error was set if any post_read step failed */
		if (bio->bi_status || PageError(page)) {
			ClearPageUptodate(page);
			/* will re-read again later */
			ClearPageError(page);
			BUG_ON(1);
		} else {
			for (k = off; k < PAGE_TO_SECT(1); k++) {
				map = redeposit_get_map(info, info->cache_map_index);
				data = redeposit_get_data(info, info->data_index++);
				data->bv_page = page;
				data->bv_len = RDPST_SECT_SIZE;
				data->bv_offset = k * RDPST_SECT_SIZE;
				SetDataUpdate(data);
				redeposit_add_data_to_cache(data, &info->i_xarr, map->log_start_sec + info->cache_map_off);
				dev_dbg(&info->pdev->dev, "%s:cache_index:%ld\n", __func__, (map->log_start_sec + info->cache_map_off));
				info->cache_map_off++;
				if (info->cache_map_off >= map->num_sec) {
					info->cache_map_index++;
					info->cache_map_off = 0;
				}
			}
			SetPageUptodate(page);
		}
	}

	info->status = STATUS_NONE;

end_bio:
	bio_put(bio);
	wake_up(&info->wait);
	info->iostat.time_data += ktime_sub(ktime_get(), info->iostat.start_data);
}

void end_redeposit_get_handle(struct bio *bio)
{
	struct redeposit_info *info = bio->bi_private;
	struct redeposit_head *head = (struct redeposit_head *)redeposit_get_head(info);
	u32 crc;

	if (bio->bi_status) {
		bio->bi_status = 0;
		info->status = STATUS_ERROR;
		set_handle(info, FLAG_NONE);
		dev_err(&info->pdev->dev, "bi_status:%ld\n", bio->bi_status);
		goto end_bio;
	}

	dev_info(&info->pdev->dev, "head status:%ld\n", head->status);
	if (head->status == H_STATUS_FINISH) {
		crc = redeposit_crc(info, CRC_HEAD);
		if (crc == info->head->head_crc && info->head->magic == REDEPOSIT_MAGIC) {
			set_handle(info, FLAG_READ);
		} else {
			set_handle(info, FLAG_WRITE);
			dev_err(&info->pdev->dev, "head crc (orgin:%lu != cal:%lu)\n", crc, info->head->head_crc);
		}
	} else if (head->status == H_STATUS_WAIT_NEXT) {
		set_handle(info, FLAG_WRITE);
	} else {
		set_handle(info, FLAG_NONE);
	}

end_bio:
	bio_put(bio);
	info->status_dev = DEV_GET_HANDLE;
	wake_up(&info->wait_dev);
}

void redeposit_done(void *info, s32 error)
{
	struct redeposit_info *info_done = info;
	if (!info_done)
		return;

	info_done->bio->bi_status = error;
	queue_delayed_work(system_wq, &info_done->redeposit_irq_work, 0);
}
EXPORT_SYMBOL_GPL(redeposit_done);

struct redeposit_info *redeposit_attach_host(void *flash, u32 max_segment,
						void *(*build_request)(void *flash, void *buf, u32 sg_len, u32 addr, u32 nr_byte),
						int (*read_flash)(void *flash, void *request, struct request_config *conf),
						void **async_done)
{
	if (!info)
		return NULL;

	info->read_flash = read_flash;
	info->build_request = build_request;
	info->max_segment = max_segment;
	info->flash = flash;
	*async_done = redeposit_done;

	return info;
}
EXPORT_SYMBOL_GPL(redeposit_attach_host);

#define PAGE_PHYS_MERG(page1, page2) ((page_to_phys(page1)+PAGE_SIZE) == page_to_phys(page2))

static bool redeposit_read_flash(struct redeposit_info *info, u32 addr, u32 nr_byte,
		void (*end_io)(struct bio *bio))
{
	int all_pages;
	struct page *page, *prev;
	u32 i, nr_sg;
	struct scatterlist *sg;
	void *request = NULL;
	struct request_config conf = {
		.is_async = true,
		.is_bkgd = false,
		.is_atomic = true,
	};

	if (!info)
		return false;

	all_pages = (nr_byte + PAGE_SIZE - 1) / PAGE_SIZE;
	info->sg_len = all_pages;
	info->bio = bio_alloc(GFP_KERNEL, all_pages);
	if (info->bio == NULL)
		return false;
	//bio_set_dev(info->bio, info->b_bdev);
	info->bio->bi_iter.bi_sector = info->re_part.start;
	info->bio->bi_end_io = end_io;
	info->bio->bi_private = info;
	info->bio->bi_status = 0;
	bio_set_op_attrs(info->bio, REQ_OP_READ, REQ_RAHEAD);

	page = redeposit_get_page(info, SECT_TO_PAGE(addr));
	sg = (info->sgtable.sgl);
	sg->length = 0;
	nr_sg = 1;
	for (i = 0; i < all_pages; i++) {
		prev = page;
		page = redeposit_get_page(info, i + SECT_TO_PAGE(addr));
		bio_add_page(info->bio, page, PAGE_SIZE, 0);

		//pr_err("LB:merge:%ld, len:%ld, max:%ld\n", PAGE_PHYS_MERG(prev, page), sg->length, info->max_segment);
		if (sg->length == 0) {
			sg_set_page(sg, page, PAGE_SIZE, 0);
		} else if (PAGE_PHYS_MERG(prev, page) && (sg->length + PAGE_SIZE) <= info->max_segment) {
			sg->length += PAGE_SIZE;
		} else {
			sg = sg_next(sg);
			sg->length = 0;
			sg_set_page(sg, page, PAGE_SIZE, 0);
			nr_sg++;
		}
		sg_unmark_end(sg);
		//pr_err("LB:i:%ld, j:%ld, len:%ld, prev:%ld, page:%ld\n", i, j, sg->length, page_to_phys(prev), page_to_phys(page));
	}
	info->sg_len = nr_sg;
	sg_mark_end(sg);

	request = info->build_request(info->flash, info->sgtable.sgl, info->sg_len, addr + info->re_part.start, nr_byte);

	if (unlikely(request == NULL))
		return false;

	info->read_flash(info->flash, request, &conf);

	return true;
}

static bool redeposit_reads_flash(struct redeposit_info *info, u32 addr, u32 nr_byte,
		void (*end_io)(struct bio *bio))
{
	int nr_bios, all_pages;
	u32 i, nr_pages;
	u32 blk_addr;
	int ret = true;

	all_pages = (nr_byte + PAGE_SIZE - 1) / PAGE_SIZE;
	nr_bios = (all_pages + BIO_MAX_PAGES - 1) / BIO_MAX_PAGES;

	for (i = 0; i < nr_bios; i++) {
		info->status = STATUS_DATA;
		blk_addr = addr + PAGE_TO_SECT(i * BIO_MAX_PAGES);
		nr_pages = ((i == nr_bios - 1) ? ((all_pages - 1) % BIO_MAX_PAGES + 1) : BIO_MAX_PAGES);
		ret = redeposit_read_flash(info, blk_addr, nr_pages * PAGE_SIZE, end_io);
		wait_event(info->wait, (info->status == STATUS_NONE || unlikely(info->status == STATUS_ERROR)));
		if (unlikely((!ret || info->status == STATUS_ERROR))) {
			info->status = STATUS_ERROR;
			set_handle(info, FLAG_NONE);
			break;
		}
	}

	return ret;
}

static void redeposit_irq_handle(struct work_struct *work)
{
	struct bio *bio;
	struct redeposit_info *info = container_of(work, struct redeposit_info, redeposit_irq_work.work);

	bio = info->bio;

	/*will put bio in bi_end_io*/
	bio->bi_end_io(bio);
}

static void redeposit_cache_handle(struct work_struct *work)
{
	struct redeposit_info *info = container_of(work, struct redeposit_info, redeposit_cache_work.work);
	int nr_map_pages, all_pages;
	struct redeposit_head *head;
	int odd, ret;

	head = info->head;
	nr_map_pages = (sizeof(struct redeposit_head) + sizeof(struct redeposit_map) * info->map_index + PAGE_SIZE - 1) / PAGE_SIZE;
	//??
	all_pages = nr_map_pages + SECT_TO_PAGE(head->data_num + PAGE_TO_SECT(1) - 1);
	if (PAGE_TO_SECT(all_pages) - info->off_pos < info->nr_rd2 * NR_MAX_HIT) {
		odd = (PAGE_TO_SECT(all_pages) - info->off_pos) * RDPST_SECT_SIZE;
	} else {
		odd = info->nr_rd2;
		odd = odd * NR_MAX_HIT * RDPST_SECT_SIZE;
	}
	dev_info(&info->pdev->dev, "odd:%ld, pos:%ld, max:%ld, all:%ld, rd2:%ld\n", odd, info->off_pos, NR_MAX_HIT, all_pages, info->nr_rd2);
	ret = redeposit_reads_flash(info, info->off_pos, odd, end_redeposit_bio_data);
	if (likely(ret)) {
		info->off_pos += odd / RDPST_SECT_SIZE;
		info->do_cache = 0;
	}
}

static bool redeposit_build_cache_from_flash(struct redeposit_info *info)
{
	int all_pages, nr_map_pages;
	struct redeposit_head *head;
	int ret;

	info->iostat.time_build = ktime_get();
	info->status = STATUS_BUILD;
	info->head = (struct redeposit_head *)redeposit_get_head(info);
	info->off_pos = 0;
	dev_info(&info->pdev->dev, "%s--%d start redeposit map build!!\n", __func__, __LINE__);
	ret = redeposit_read_flash(info, 0, REDEPOSIT_MAP_MAX_SIZE, end_redeposit_bio_map);
	if (unlikely(!ret)) {
		info->status = STATUS_ERROR;
		set_handle(info, FLAG_NONE);
		dev_err(&info->pdev->dev, "read map failed%s--%ld\n", __func__, __LINE__);
		goto build_cache_failed;
	}

	wait_event(info->wait, ((info->status == STATUS_MAP) || (info->status == STATUS_ERROR)));

	if (unlikely(info->status == STATUS_ERROR)) {
		set_handle(info, FLAG_NONE);
		dev_err(&info->pdev->dev, "read map failed%s--%ld\n", __func__, __LINE__);
		goto build_cache_failed;
	}

	dev_dbg(&info->pdev->dev, "%s--%d finish redeposit map build!!\n", __func__, __LINE__);
	/*assume Read back head correctly*/
	head = info->head;
	nr_map_pages = (sizeof(struct redeposit_head) + sizeof(struct redeposit_map) * info->map_index + PAGE_SIZE - 1) / PAGE_SIZE;
	all_pages = nr_map_pages + SECT_TO_PAGE(head->data_num + PAGE_TO_SECT(1) - 1);

	/*This can be considered abnormal in normal use*/
	if (all_pages * PAGE_SIZE <= REDEPOSIT_MAP_MAX_SIZE) {
		dev_info(&info->pdev->dev, "%s-%d-B:%ld, %ld, %ld!!\n", __func__, __LINE__,
			 nr_map_pages, all_pages, REDEPOSIT_MAP_MAX_SIZE);
		goto build_cache;
	}

	dev_dbg(&info->pdev->dev, "%s--%d redeposit data build!!\n", __func__, __LINE__);
	ret = redeposit_reads_flash(info, REDEPOSIT_MAP_MAX_SIZE / RDPST_SECT_SIZE, (info->nr_rd1 * NR_MAX_HIT * RDPST_SECT_SIZE), end_redeposit_bio_data);
	if (unlikely(!ret)) {
		info->status = STATUS_ERROR;
		set_handle(info, FLAG_NONE);
		dev_err(&info->pdev->dev, "read data failed %s--%ld\n", __func__, __LINE__);
		goto build_cache_failed;
	}
	info->off_pos = REDEPOSIT_MAP_MAX_SIZE / RDPST_SECT_SIZE + info->nr_rd1 * NR_MAX_HIT;

	info->cache_init = 1;

build_cache:
	dev_dbg(&info->pdev->dev, "%s--%d finish redeposit data build!!\n", __func__, __LINE__);

	info->iostat.time_build = ktime_sub(ktime_get(), info->iostat.time_build);
	return true;

build_cache_failed:
	return false;
}

bool redeposit_read_data(struct redeposit_info *info, struct scatterlist *sg, unsigned int sg_len, unsigned int blk_addr, unsigned int blocks)
{
	unsigned int nr_blk = 0;
	unsigned char *data_buf = NULL, *buf = NULL;
	unsigned int i, j, sg_buf_len, data_buf_len;
	struct redeposit_data *data;
	int nr_map_pages, all_pages;
	struct redeposit_head *head;

	if (!info || !is_handle_read(info)) {
		return false;
	}

	info->iostat.start_hit = ktime_get();
	if (!is_redeposit_addr(info, blk_addr, blocks)) {
		return false;
	}

	head = info->head;
	dev_dbg(&info->pdev->dev, "R-start:blk_addr:%ld, blocks:%ld, %px\n", blk_addr, blocks, sg_virt(&sg[0]));
	/*only consider hit cache from start to end_page_index */
	for (i = blk_addr; i < (blk_addr + blocks); i++) {
		data = redeposit_find_get_data(&info->i_xarr, i);
		if (!data || !DataUpdate(data))
			break;
		nr_blk++;
	}

	if (nr_blk == blocks) {
		j = 0;
		sg_buf_len = 0;
		for (i = blk_addr; i < (blk_addr + blocks); i++) {
			data = redeposit_find_get_data(&info->i_xarr, i);
			if (!data || !DataUpdate(data))
				break;

			data_buf = (u8 *)page_to_virt(data->bv_page) + data->bv_offset;
			data_buf_len = data->bv_len;
next_sg:
			if (!sg_buf_len) {
				buf = sg_virt(&sg[j]);
				dev_dbg(&info->pdev->dev, "Rsg:&sg[j]:%px, page:%px, buf:%px\n", &sg[j], sg_page((&sg[j])), buf);
				sg_buf_len = sg[j].length;
				j++;
				if (j > sg_len)
					break;
			}

			if (data_buf_len > sg_buf_len) {
				memcpy(buf, data_buf, sg_buf_len);
				data_buf_len -= sg_buf_len;
				data_buf += sg_buf_len;
				goto next_sg;
			} else {
				memcpy(buf, data_buf, data_buf_len);
				sg_buf_len -= data_buf_len;
				buf += data_buf_len;
			}

		}
		info->cache_hit += nr_blk;

		if (info->cache_init) {
			nr_map_pages = (sizeof(struct redeposit_head) + sizeof(struct redeposit_map) * info->map_index + PAGE_SIZE - 1) / PAGE_SIZE;
			all_pages = nr_map_pages + SECT_TO_PAGE(head->data_num + PAGE_TO_SECT(1) - 1);
			if (info->aread && (SECT_TO_PAGE(info->off_pos) < all_pages) && !info->do_cache) {
				info->do_cache = 1;
				queue_delayed_work(system_wq, &info->redeposit_cache_work, 0);
			}
		}

		info->iostat.time_hit += ktime_sub(ktime_get(), info->iostat.start_hit);
	}

	if (likely(is_handle_read(info)))
		return (!!(nr_blk == blocks));
	else {
		/*startup finish or error happen*/
		redeposit_release_info(info);
		return false;
	}
}
EXPORT_SYMBOL_GPL(redeposit_read_data);

static ssize_t handle_status_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	int ret;
	struct platform_device *pdev = to_platform_device(dev);
	struct redeposit_info *info = platform_get_drvdata(pdev);

	ret =
	    snprintf(buf, PAGE_SIZE,
		     "redeposit handle_status : %ld, hit:%ld, time:start:%ld, build:%ld, map:%ld, data:%ld, hit:%ld\n", info->handle_flag, info->cache_hit, info->iostat.start, info->iostat.time_build, info->iostat.time_map, info->iostat.time_data, info->iostat.time_hit);
	return ret;
}
static ssize_t handle_status_store(struct device *dev,
				  struct device_attribute *attr,
				  const char *buf, size_t count)
{
	int ret;
	struct platform_device *pdev = to_platform_device(dev);
	struct redeposit_info *info = platform_get_drvdata(pdev);
	unsigned int old_handle = info->handle_flag;

	ret = kstrtoul(buf, 0, &info->handle_flag);
	if (ret) {
		ret = -EINVAL;
		return ret;
	}

	if (is_handle_flush(info) && info->status != STATUS_ERROR) {
		if (old_handle != FLAG_READ && info->status != STATUS_RELEASE) {
			info->hstatus = (old_handle == FLAG_WRITE) ? H_STATUS_FINISH : H_STATUS_WAIT_NEXT;
			queue_delayed_work(system_wq, &info->redeposit_flush_work, 0);
		}
		set_handle(info, FLAG_NONE);
	}
	dev_info(&info->pdev->dev, "old_handle:%ld, handle:%ld, status:%ld\n", old_handle, info->handle_flag, info->status);

	ret = count;
	return ret;
}

static ssize_t bdev_status_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	int ret;
	struct platform_device *pdev = to_platform_device(dev);
	struct redeposit_info *info = platform_get_drvdata(pdev);

	ret = snprintf(buf, PAGE_SIZE, "status_dev:%ld %ld\n", info->status_dev, info->write_io);

	return ret;
}

static ssize_t allow_read_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	int ret;
	struct platform_device *pdev = to_platform_device(dev);
	struct redeposit_info *info = platform_get_drvdata(pdev);

	ret = snprintf(buf, PAGE_SIZE, "allow_read:%ld\n", info->aread);

	return ret;
}

void redeposit_wake_up_dev(struct redeposit_info *info, sector_t sect, sector_t blocks)
{
	if (!info)
		return;
	if (info->status_dev == DEV_WAIT_WAKE) {
		info->status_dev = DEV_WAKEUP_DEV;
		wake_up(&info->wait_dev);
	}
}
EXPORT_SYMBOL_GPL(redeposit_wake_up_dev);

static ssize_t allow_read_store(struct device *dev,
				  struct device_attribute *attr,
				  const char *buf, size_t count)
{
	int ret;
	struct platform_device *pdev = to_platform_device(dev);
	struct redeposit_info *info = platform_get_drvdata(pdev);

	ret = kstrtoul(buf, 0, &info->aread);
	if (ret) {
		ret = -EINVAL;
		return ret;
	}
	ret = count;
	return ret;
}

static ssize_t debug_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	int ret = 0;
	//struct platform_device *pdev = to_platform_device(dev);
	//struct redeposit_info *info = platform_get_drvdata(pdev);

	//ret = snprintf(buf, PAGE_SIZE, "allow_read:%ld\n", info->aread);

	return ret;
}

static ssize_t debug_store(struct device *dev,
				  struct device_attribute *attr,
				  const char *buf, size_t count)
{
	int ret;
	struct platform_device *pdev = to_platform_device(dev);
	struct redeposit_info *info = platform_get_drvdata(pdev);
	unsigned long dmod;
	struct redeposit_map *map;
	unsigned int i;

	ret = kstrtoul(buf, 0, &dmod);
	if (ret) {
		ret = -EINVAL;
		return ret;
	}

	if (dmod > 254) {
		for (i = 0; i < info->map_index; i++) {
			map = redeposit_get_map(info, i);
			if (map->log_start_sec <= dmod && (dmod <= map->log_start_sec+map->num_sec)) {
				dev_err(&info->pdev->dev, "addr:%ld, num:%ld, i:%ld\n", map->log_start_sec, map->num_sec, i);
			}
		}
	} else {
		for (i = 0; i < info->map_index; i++) {
			map = redeposit_get_map(info, i);
			dev_err(&info->pdev->dev, "addr:%ld, num:%ld, i:%ld\n", map->log_start_sec, map->num_sec, i);
		}
	}

	ret = count;
	return ret;
}

static ssize_t record_part_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	int ret = 0;

	return ret;
}

static ssize_t record_part_store(struct device *dev,
				  struct device_attribute *attr,
				  const char *buf, size_t count)
{
	int ret, i = 0;
	unsigned long type;
	struct platform_device *pdev = to_platform_device(dev);
	struct redeposit_info *info = platform_get_drvdata(pdev);
	const char *p = NULL;
	char str[RDPST_MAX_PATH] = {'\0'};

	if (!info || likely(is_handle_read(info))) {
		return count;
	}

	p = buf;

	while (*p++ != ',' && i < RDPST_MAX_PATH)
		i++;

	if (i >= RDPST_MAX_PATH) {
		ret = -EINVAL;
		dev_err(&info->pdev->dev, "record_store--err:%s---%ld\n", __func__, __LINE__);
		goto ret_err;
	}
	memcpy(str, buf, i);
	ret = kstrtoul(str, 0, &type);

	if (ret) {
		ret = -EINVAL;
		dev_err(&info->pdev->dev, "record_store--err:%s---%ld\n", __func__, __LINE__);
		goto ret_err;
	}

	if (type == 1) {
		if (info->ro_part_num >= RO_PART_NUM) {
			dev_err(&info->pdev->dev, "record_part ro_part_num:%ld >= RO_PART_NUM:%ld", info->ro_part_num, RO_PART_NUM);
			ret = -EINVAL;
			goto ret_err;
		}
		memcpy(info->ro_path[info->ro_part_num++], p, RDPST_MAX_PATH);
	}

	dev_err(&info->pdev->dev, "record part:%s %d\n", p, type);
	ret = count;
ret_err:
	return ret;
}

static int redeposit_create_sys_fs(struct redeposit_info *info, struct platform_device *pdev)
{
	int ret;

	info->handle_status.show = handle_status_show;
	info->handle_status.store = handle_status_store;
	sysfs_attr_init(&(info->handle_status.attr));
	info->handle_status.attr.name = "handle_status";
	info->handle_status.attr.mode = S_IRUGO | S_IWUSR;
	ret = device_create_file(&pdev->dev, &info->handle_status);

	info->bdev_status.show = bdev_status_show;
	info->bdev_status.store = NULL;
	sysfs_attr_init(&(info->bdev_status.attr));
	info->bdev_status.attr.name = "bdev_status";
	info->bdev_status.attr.mode = S_IRUGO;
	ret = device_create_file(&pdev->dev, &info->bdev_status);

	info->allow_read.show = allow_read_show;
	info->allow_read.store = allow_read_store;
	sysfs_attr_init(&(info->allow_read.attr));
	info->allow_read.attr.name = "allow_read";
	info->allow_read.attr.mode = S_IRUGO | S_IWUSR;
	ret = device_create_file(&pdev->dev, &info->allow_read);

	info->debug.show = debug_show;
	info->debug.store = debug_store;
	sysfs_attr_init(&(info->debug.attr));
	info->debug.attr.name = "debug";
	info->debug.attr.mode = S_IRUGO | S_IWUSR;
	ret = device_create_file(&pdev->dev, &info->debug);

	info->record_part.show = record_part_show;
	info->record_part.store = record_part_store;
	sysfs_attr_init(&(info->record_part.attr));
	info->record_part.attr.name = "record_part";
	info->record_part.attr.mode = S_IRUGO | S_IWUSR;
	ret = device_create_file(&pdev->dev, &info->record_part);
	return ret;
}

static void mmc_remove_sys_fs(struct redeposit_info *info, struct platform_device *pdev)
{
	device_remove_file(&pdev->dev, &info->handle_status);
	device_remove_file(&pdev->dev, &info->bdev_status);
	device_remove_file(&pdev->dev, &info->allow_read);
	device_remove_file(&pdev->dev, &info->debug);
}

static void redeposit_build_handle(struct work_struct *work)
{
	struct redeposit_info *info = container_of(work, struct redeposit_info, redeposit_build_work.work);
	struct platform_device *pdev = info->pdev;
	int ret;

	info->iostat.start = ktime_get();

	dev_info(&pdev->dev, "wait event to get disk\n");
	wait_event(info->wait_dev, (info->status_dev == DEV_WAKEUP_DEV));
	dev_info(&pdev->dev, "wake up to get disk\n");

	ret = redeposit_read_flash(info, 0, PAGE_SIZE, end_redeposit_get_handle);
	if (unlikely(!ret) || info->status == STATUS_ERROR) {
		set_handle(info, FLAG_NONE);
		goto give_up_build;
	}

	redeposit_create_sys_fs(info, pdev);

	wait_event(info->wait_dev, (info->status_dev == DEV_GET_HANDLE));

	if (likely(is_handle_read(info))) {
		ret = redeposit_build_cache_from_flash(info);
		if (unlikely(!ret)) {
			set_handle(info, FLAG_NONE);
			goto give_up_build;
		}
	} else if (is_handle_write(info) || info->handle_flag == FLAG_NONE) {
		info->head->map_size = REDEPOSIT_MAP_MAX_SIZE;
		redeposit_build_data(info);
	}

	info->iostat.start = ktime_sub(ktime_get(), info->iostat.start);
	info->build_done = 1;

	return;

give_up_build:
	/*release redeposit info*/
	redeposit_release_info(info);
	return;
}

void write_io_num(struct redeposit_info *info)
{
	if (info->handle_flag != FLAG_FLUSH) {
		info->write_io++;
	}
}
EXPORT_SYMBOL_GPL(write_io_num);

static void redeposit_set_part(struct redeposit_info *info, struct device_node *np)
{
	of_property_read_u32_array(np, "rdpst_start", (u32 *)(&info->re_part.start), 1);
	of_property_read_u32_array(np, "rdpst_end", (u32 *)(&info->re_part.end), 1);

	dev_info(&info->pdev->dev, "re:%ld-%ld\n", info->re_part.start, info->re_part.end);
}

static void redeposit_get_conf(struct redeposit_info *info, struct device_node *np)
{
	if (of_property_read_u32(np, "nr-hz", &(info->nr_hz)) < 0) {
		info->nr_hz = 1;
	}
	if (of_property_read_u32(np, "nr-mem", &(info->nr_mem)) < 0) {
		info->nr_mem = 1;
	}
	if (of_property_read_u32(np, "nr-blk", &(info->nr_blk)) < 0) {
		info->nr_blk = 1;
	}
	if (of_property_read_u32(np, "nr-rd2", &(info->nr_rd2)) < 0) {
		info->nr_rd2 = 1;
	}
	if (of_property_read_u32(np, "nr-rd1", &(info->nr_rd1)) < 0) {
		info->nr_rd1 = 3;
	}

	info->mem_size = REDEPOSIT_MEM_SIZE(info->nr_mem);
}

static int redeposit_probe(struct platform_device *pdev)
{
	u32 nr_bios, nr_page, nr_data_buf;
	struct device_node *np = NULL;
	int i, ret;

	np = pdev->dev.of_node;

	dev_info(&pdev->dev, "%s\n", RDPST_DRIVER_VERSION);
	BUG_ON((REDEPOSIT_MAP_MAX_SIZE % PAGE_SIZE));

	info = kzalloc(sizeof(struct redeposit_info), GFP_KERNEL);
	if (unlikely(!info)) {
		dev_err(&pdev->dev, "redeposit alloc info failed\n");
		goto alloc_info_failed;
	}

	info->aread = 1;
	info->status_dev = DEV_WAIT_WAKE;
	platform_set_drvdata(pdev, info);
	info->pdev = pdev;
	set_handle(info, FLAG_NONE);

	redeposit_get_conf(info, np);

	redeposit_set_part(info, np);

	nr_bios = (info->mem_size / PAGE_SIZE + BIO_MAX_PAGES - 1) / BIO_MAX_PAGES;
	info->pages = kmalloc(sizeof(struct page *) * nr_bios, GFP_KERNEL);
	dev_info(&pdev->dev, "mem region nr_bios:%ld size:%d\n", nr_bios, nr_bios * sizeof(struct page *));
	if (unlikely(!info->pages)) {
		dev_err(&pdev->dev, "can not kmalloc mem region nr_bios:%ld size:%d\n",
			nr_bios, nr_bios * sizeof(struct page *));
		goto free_info;
	}

	BUG_ON((info->mem_size - REDEPOSIT_MAP_MAX_SIZE) % PAGE_SIZE);
	nr_data_buf = (info->mem_size - REDEPOSIT_MAP_MAX_SIZE) / RDPST_SECT_SIZE;
	info->data_buf = vmalloc(sizeof(struct redeposit_data) * nr_data_buf);
	if (unlikely(!info->data_buf)) {
		dev_err(&pdev->dev, "can not kmalloc mem region nr_data_buf:%ld size:%d\n",
			nr_data_buf, nr_data_buf * sizeof(struct redeposit_data *));
		goto free_pages;
	}

	ret = sg_alloc_table(&info->sgtable, PAGE_SIZE / 4, GFP_KERNEL);
	if (unlikely(ret)) {
		goto  free_data_buf;
	}

	init_waitqueue_head(&info->wait);
	init_waitqueue_head(&info->wait_dev);
	INIT_DELAYED_WORK(&info->redeposit_build_work, redeposit_build_handle);
	INIT_DELAYED_WORK(&info->redeposit_irq_work, redeposit_irq_handle);
	INIT_DELAYED_WORK(&info->redeposit_cache_work, redeposit_cache_handle);
	INIT_DELAYED_WORK(&info->redeposit_flush_work, redeposit_flush_handle);

	nr_bios = (info->mem_size / PAGE_SIZE + BIO_MAX_PAGES - 1) / BIO_MAX_PAGES;
	for (i = 0; i < nr_bios ; i++) {
		nr_page = (i == nr_bios -1) ? ((info->mem_size / PAGE_SIZE - 1) % BIO_MAX_PAGES + 1):(BIO_MAX_PAGES);
		info->pages[i] = alloc_pages(GFP_KERNEL, get_order(nr_page * PAGE_SIZE));
		if (unlikely(!info->pages[i])) {
			dev_err(&pdev->dev, "i%ld, nr_bios:%ld,nr_page:%ld, MAX_PAGES:%ld, order:%ld\n",
				i, nr_bios, nr_page, BIO_MAX_PAGES, get_order(nr_page * PAGE_SIZE));
			goto  free_data_pages;
		}
	}

	/*layout:/head/ + /map_buf/ + /data_buf/*/
	info->head = redeposit_get_head(info);
	memset(info->head, 0, sizeof(struct redeposit_head));
	info->head->magic = REDEPOSIT_MAGIC;

	/*we may be try to delay wake up the build work*/
	queue_delayed_work(system_wq, &info->redeposit_build_work, HZ/2 * info->nr_hz);

	return 0;

free_data_pages:
	for (i = 0; i < nr_bios ; i++) {
		nr_page = (i == nr_bios -1) ? ((info->mem_size / PAGE_SIZE - 1) % BIO_MAX_PAGES + 1):(BIO_MAX_PAGES);
		if (info->pages[i]) {
			free_pages((unsigned long)(info->pages[i]), get_order(nr_page * PAGE_SIZE));
			continue;
		}
		break;
	}

	sg_free_table(&info->sgtable);
free_data_buf:
	vfree(info->data_buf);
free_pages:
	kfree(info->pages);
free_info:
	kfree(info);
alloc_info_failed:
	/*only no mem will cause it probe failed*/
	return -ENOMEM;
}

static int redeposit_remove(struct platform_device *pdev)
{
	struct redeposit_info *info = platform_get_drvdata(pdev);

	cancel_delayed_work_sync(&info->redeposit_build_work);
	cancel_delayed_work_sync(&info->redeposit_irq_work);
	cancel_delayed_work_sync(&info->redeposit_cache_work);
	cancel_delayed_work_sync(&info->redeposit_flush_work);

	wait_event(info->wait, (info->status == STATUS_RELEASE));

	mmc_remove_sys_fs(info, pdev);

	kfree(info);

	return 0;
}

static const struct of_device_id redeposit_of_match[] = {
	{.compatible = "redeposit",},
	{ /* sentinel */ }
};

static struct platform_driver redeposit_driver = {
	.driver = {
		   .name = "redeposit",
		   .of_match_table = of_match_ptr(redeposit_of_match),
		   },
	.probe = redeposit_probe,
	.remove = redeposit_remove,
};

module_platform_driver(redeposit_driver);

MODULE_DESCRIPTION("Allwinner's redeposit startup io data");
MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("libiao <libiao@allwinnertech.com>");
MODULE_ALIAS("platform:redeposit");
MODULE_VERSION("1.0.0");
