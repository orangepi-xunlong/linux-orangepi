// SPDX-License-Identifier: GPL-2.0
/*
 * sunxi's rproc log trace driver
 *
 * Copyright (C) 2022 Allwinnertech - All Rights Reserved
 *
 * Author: shihongfu <shihongfu@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/firmware.h>
#include <linux/uaccess.h>
#include <linux/list.h>
#include <linux/slab.h>
#include <linux/vmalloc.h>
#include <linux/string.h>
#include <linux/jiffies.h>
#include <asm/cacheflush.h>
#include <linux/delay.h>

#include <linux/debugfs.h>
#include <linux/remoteproc.h>

#include "remoteproc_internal.h"

#ifdef CONFIG_ARM
#include <linux/dma-direction.h>
#include <../arch/arm/mm/dma.h>
#endif

#define SUNXI_RPROC_TRACE_VERSION "1.0.4"

//#define SUNXI_RPROC_TRACE_DEBUG

#define MAX_BYTES_IN_ONE_LOG_LINE 255

/* mem layout: | user0(reader) area | user1(writer) area | data |*/

#define AREA_ALIGN_SIZE		(64)

#define READER_AREA_SIZE	(AREA_ALIGN_SIZE)
#define READER_AREA_OFFSET	(0)

#define WRITER_AREA_SIZE	(AREA_ALIGN_SIZE)
#define WRITER_AREA_OFFSET	(READER_AREA_SIZE)

#define LOG_BUF_OFFSET		(WRITER_AREA_OFFSET + WRITER_AREA_SIZE)
#define LOG_BUF_SIZE		(sizeof(amp_log_buffer) - LOG_BUF_OFFSET)

#pragma pack(1)
struct area_info_t {
	u64 pos;
	u32 area_size;
	u32 size;
	u32 overrun;
};
#pragma pack()

#define READER_AREA_PTR(base)	((struct area_info_t *)((char *)(base) + READER_AREA_OFFSET))
#define WRITER_AREA_PTR(base)	((struct area_info_t *)((char *)(base) + WRITER_AREA_OFFSET))
#define LOG_BUF_PTR(base)	((char *)(base) + LOG_BUF_OFFSET)

static inline void cpy_to_cachemem(void *dst, const void *src, size_t size)
{
	memcpy(dst, src, size);
#ifdef CONFIG_ARM64
	dcache_clean_inval_poc((unsigned long)dst, (unsigned long)(dst + size));
#endif
#ifdef CONFIG_ARM
	dmac_flush_range(dst, dst + size);
#endif
}

static inline void cpy_from_cachemem(void *dst, const void *src, size_t size)
{
#ifdef CONFIG_ARM64
	dcache_inval_poc((unsigned long)src, (unsigned long)(src + size));
#endif
#ifdef CONFIG_ARM
	dmac_unmap_area(src, size, DMA_FROM_DEVICE);
#endif
	memcpy(dst, src, size);
}

static inline unsigned long
ringbuffer_get_enter(const struct area_info_t *shared_writer, struct area_info_t *writer,
		     const struct area_info_t *shared_reader, struct area_info_t *reader)
{
	cpy_from_cachemem(writer, shared_writer, sizeof(*writer));
	cpy_from_cachemem(reader, shared_reader, sizeof(*reader));
	return 0;
}

static inline void
ringbuffer_get_exit(struct area_info_t *shared_reader, const struct area_info_t *reader,
		    unsigned long flags)
{
	cpy_to_cachemem(shared_reader, reader, sizeof(*shared_reader));
}

#define update_space(_pos_, _max_size_, _ret_) \
	do { \
		if ((_ret_) < 0) { \
			pr_err("[%s:%u]append msg failed! ret: %d", \
			       __func__, __LINE__, (int)(_ret_)); \
			break; \
		} else if ((_ret_) > (_max_size_)) { \
			pr_err("[%s:%u]append msg overflow! %d > %lu", \
			       __func__, __LINE__, (int)(_ret_), (unsigned long)(_max_size_)); \
			BUG_ON(1); \
			break; \
		} else { \
			(_pos_) += (_ret_); \
			(_max_size_) -= (_ret_); \
		} \
	} while (0)

static inline void read_ringbuffer(const void *ring, size_t ring_size, uint64_t pos,
				   void *data, size_t data_size)
{
#if BITS_PER_LONG == 64
	size_t start = pos % ring_size;
#endif
	size_t end = 0;
	size_t off;

#if BITS_PER_LONG == 32
	uint32_t remainder = 0;
	size_t start;
	div_u64_rem(pos, ring_size, &remainder);
	start = remainder;
#endif

	end = start + data_size;

	if (ring_size < data_size)
		return;

	if (end > ring_size) {
		off = ring_size - start;
		cpy_from_cachemem(data, ring + start, off);
		cpy_from_cachemem(data + off, ring, data_size - off);
	} else {
		cpy_from_cachemem(data, ring + start, data_size);
	}
}

static inline int ringbuffer_get(void *rb, int rb_size, void *_data, size_t max_size)
{
	unsigned long flags;
	struct area_info_t _reader;
	struct area_info_t _writer;
	struct area_info_t * const reader = &_reader;
	struct area_info_t * const writer = &_writer;
	const char *log_buf = LOG_BUF_PTR(rb);
	size_t readable, read = 0;
	char *data = _data;
	int ret;

	if (!rb || !data || max_size <= 0)
		return -EINVAL;

	flags = ringbuffer_get_enter(WRITER_AREA_PTR(rb), writer, READER_AREA_PTR(rb), reader);

	if (!reader->area_size) {
		reader->area_size = READER_AREA_SIZE;
		reader->pos = 0;
		if (max_size > 1) {
			ret = scnprintf(&data[read], max_size - 1, "info: reader init\r\n");
			update_space(read, max_size, ret);
		}
	}

	if (!writer->size) {
		if (max_size > 1) {
			ret = scnprintf(&data[read], max_size - 1, "err: writer not init\r\n");
			update_space(read, max_size, ret);
		}
		goto out;
	}

	if ((rb_size - LOG_BUF_OFFSET) != writer->size && max_size > 1) {
		ret = scnprintf(&data[read], max_size - 1, "err: (rb_size(%d) - %d) != %u\r\n",
				rb_size, LOG_BUF_OFFSET, writer->size);
		update_space(read, max_size, ret);
	}

	// using uint64_t, not considering numerical overflow issues
	if (reader->pos > writer->pos) {
		if (max_size > 1) {
			ret = scnprintf(&data[read], max_size - 1,
					"err: reader->pos(%llu) > writer->pos(%llu)\r\n",
					reader->pos, writer->pos);
			update_space(read, max_size, ret);
		}
		reader->pos = writer->pos;
	}

	// using size_t, not considering numerical overflow issues
	readable = writer->pos - reader->pos;

	if (readable == 0) {
		goto out;
	} else if (readable > writer->size) {
		if (max_size > 1) {
			ret = scnprintf(&data[read], max_size - 1,
					"warn: overrun, lost %llu bytes of log\r\n",
					(unsigned long long)(readable - writer->size));
			update_space(read, max_size, ret);
		}
		reader->pos += readable - writer->size;
		readable = writer->size;
	}

	if (max_size > readable)
		max_size = readable;

	read_ringbuffer(log_buf, writer->size, reader->pos, &data[read], max_size);
	read += max_size;
	reader->pos += max_size;

out:
	ringbuffer_get_exit(READER_AREA_PTR(rb), reader, flags);
	return read;
}

#ifdef SUNXI_RPROC_TRACE_DEBUG
static inline void cachemem_dump(const char *mem, int size)
{
	int i, j, cnt, zero = 0;
	unsigned char tmp[64 + 2];

	pr_info("--------\n");
	tmp[64] = 0;
	for (i = 0; i < size; i += 64) {
		cpy_from_cachemem(tmp, &mem[i], 64);
		cnt = 0;
		for (j = 0; j < 64; j++)
			cnt += tmp[j];
		if (cnt == 0) {
			if (zero == 0)
				pr_info("%5d: all zero\n", i);
			zero++;
			continue;
		} else {
			if (zero > 1)
				pr_info("%5d: all zero end\n", i - 64);
			zero = 0;
		}
		for (j = 0; j < 64; j++) {
			if (tmp[j] < 0x20 || tmp[j] > 0x7e)
				tmp[j] = '.';
		}
		pr_info("%5d:|%s|\n", i, tmp);
	}
	pr_info("--------\n");
}
#endif

static inline void ringbuffer_info(void *trace_mem, int trace_mem_len)
{
	struct area_info_t reader, writer;
#ifdef SUNXI_RPROC_TRACE_DEBUG
	const char *log_buf = LOG_BUF_PTR(trace_mem);
#endif

	cpy_from_cachemem(&writer, WRITER_AREA_PTR(trace_mem), sizeof(writer));
	cpy_from_cachemem(&reader, READER_AREA_PTR(trace_mem), sizeof(reader));

	pr_info("trace_mem:     %px\n", trace_mem);
	pr_info("trace_mem_len: %d\n", trace_mem_len);
	pr_info("r->pos:        %llu\n", reader.pos);
	pr_info("r->area_size:  %u\n", reader.area_size);
	pr_info("w->pos:        %llu\n", writer.pos);
	pr_info("w->size:       %u\n", writer.size);
	pr_info("w->area_size:  %u\n", writer.area_size);
	pr_info("w->overrun:    %u\n", writer.overrun);

#ifdef SUNXI_RPROC_TRACE_DEBUG
	cachemem_dump(log_buf, writer.size);
#endif
}

ssize_t sunxi_rproc_trace_read(void *from, int buf_len, char *to, size_t count)
{
	ssize_t len = 0;
	int ret;

#ifdef SUNXI_RPROC_TRACE_DEBUG
	ringbuffer_info(from, buf_len);
#endif

	while (count > 0) {
		ret = ringbuffer_get(from, buf_len, &to[len], count);
		if (ret <= 0)
			break;

		count -= ret;
		len += ret;
	}

	return len;
}

ssize_t sunxi_rproc_trace_read_to_user(void *from, int buf_len, char __user *userbuf,
				       size_t count, loff_t *ppos)
{
	char tmp_buf[128];
	ssize_t read, len = 0;
	int ret;

#ifdef SUNXI_RPROC_TRACE_DEBUG
	ringbuffer_info(from, buf_len);
#endif

	*ppos = 0;

	while (count > 0) {
		read = count > sizeof(tmp_buf) ? sizeof(tmp_buf) : count;
		ret = ringbuffer_get(from, buf_len, tmp_buf, read);
		if (ret <= 0)
			break;

		if (copy_to_user(userbuf + len, tmp_buf, ret)) {
			pr_warn("remoteproc trace fifo copy_to_user failed.\n");
			break;
		}
		count -= ret;
		len += ret;
		*ppos += len;
	}

	return len;
}

static int find_newline_char(const char *buf, int buf_len)
{
	int i;

	for (i = 0; i < buf_len; i++) {
		if (buf[i] == '\n')
			return i;
	}

	return -1;
}

int sunxi_rproc_trace_dump(void *trace_mem, int trace_mem_len)
{
	char tmp_buf[MAX_BYTES_IN_ONE_LOG_LINE + 1];
	int read = 0, off;

	ringbuffer_info(trace_mem, trace_mem_len);

	while (1) {
		off = read;
		if (off >= MAX_BYTES_IN_ONE_LOG_LINE) {
			// buffer full, print & clean up
			pr_emerg("%s\n", tmp_buf);
			off = 0;
		}
		read = MAX_BYTES_IN_ONE_LOG_LINE - off;
		read = ringbuffer_get(trace_mem, trace_mem_len, &tmp_buf[off], read);
		if (read <= 0)
			break;

		read += off;
		tmp_buf[read] = '\0';

		while (1) {
			off = find_newline_char(tmp_buf, read);
			if (off < 0)
				break;

			// print a line of logs
			tmp_buf[off] = '\0';
			pr_emerg("%s\n", tmp_buf);

			if (read > (off + 1)) {
				read -= off + 1;
				memmove(tmp_buf, &tmp_buf[off + 1], read);
			} else {
				read = 0;
			}
		}
	}
	if (off > 0) {
		// print remaining logs
		tmp_buf[off] = '\0';
		pr_emerg("%s\n", tmp_buf);
	}

	return 0;
}

static ssize_t rproc_aw_trace_read(struct file *filp, char __user *userbuf,
				size_t count, loff_t *ppos)
{
	struct rproc_debug_trace *data = filp->private_data;
	struct rproc_mem_entry *trace = &data->trace_mem;
	struct device *dev = data->rproc->dev.parent;

	void *va = rproc_da_to_va(data->rproc, trace->da, trace->len, NULL);
	if (!va) {
		dev_err(dev, "failed to get trace mem's va, da: 0x%08x, len: %zu\n", trace->da, trace->len);
		return 0;
	}

	return sunxi_rproc_trace_read_to_user(va, trace->len, userbuf, count, ppos);
}

static const struct file_operations aw_trace_file_ops = {
	.read = rproc_aw_trace_read,
	.open = simple_open,
	.llseek	= generic_file_llseek,
};

struct dentry *sunxi_rproc_create_aw_trace_file(const char *name, struct rproc *rproc,
				       struct rproc_debug_trace *trace)
{
	struct dentry *tfile;

	tfile = debugfs_create_file(name, 0400, rproc->dbg_dir, trace,
				    &aw_trace_file_ops);
	if (!tfile) {
		dev_err(rproc->dev.parent, "failed to create debugfs trace entry\n");
		return NULL;
	}

	return tfile;
}

void sunxi_rproc_remove_aw_trace_file(struct dentry *tfile)
{
	debugfs_remove(tfile);
}

MODULE_DESCRIPTION("SUNXI Remote Log Trace Helper");
MODULE_AUTHOR("shihongfu <shihongfu@allwinnertech.com>");
MODULE_LICENSE("GPL v2");
MODULE_VERSION(SUNXI_RPROC_TRACE_VERSION);
