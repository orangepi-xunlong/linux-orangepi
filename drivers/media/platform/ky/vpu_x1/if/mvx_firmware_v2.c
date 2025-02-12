/*
 * The confidential and proprietary information contained in this file may
 * only be used by a person authorised under and to the extent permitted
 * by a subsisting licensing agreement from Arm Technology (China) Co., Ltd.
 *
 *            (C) COPYRIGHT 2021-2021 Arm Technology (China) Co., Ltd.
 *                ALL RIGHTS RESERVED
 *
 * This entire notice must be reproduced on all copies of this file
 * and copies of this file may only be made by a person if such person is
 * permitted to do so under the terms of a subsisting license agreement
 * from Arm Technology (China) Co., Ltd.
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 */

/****************************************************************************
 * Includes
 ****************************************************************************/
#include <linux/dma-buf.h>
#include <linux/dma-mapping.h>
#include <linux/string.h>
#include "fw_v2/mve_protocol_def.h"
#include "mvx_firmware_cache.h"
#include "mvx_firmware_priv.h"
#include "mvx_log_group.h"
#include "mvx_log_ram.h"
#include "mvx_mmu.h"
#include "mvx_secure.h"
#include "mvx_seq.h"
#include "mvx_session.h"
#include "mvx_v4l2_session.h"

/****************************************************************************
 * Static functions
 ****************************************************************************/

/**
 * is_afbc() - Detect if format is AFBC.
 * @format:		Color format.
 *
 * Return: True if AFBC, else false.
 */
static bool is_afbc(unsigned int format)
{
	return (format & (1 << MVE_FORMAT_BF_A)) != 0;
}

/**
 * log_message() - Log a message.
 * @session:		Pointer to session.
 * @channel:		The type of the firmware interface message;
 *			message, input buffer, output buffer or RPC
 * @direction:		The type of the firmware interface message;
 *			host->firmware or firware->host.
 * @msg_header:		The header of the message.
 * @data:		Pointer to the message data.
 */
static void log_message(struct mvx_session *session,
			enum mvx_log_fwif_channel channel,
			enum mvx_log_fwif_direction direction,
			struct mve_msg_header *msg_header,
			void *data)
{
	struct mvx_log_header header;
	struct mvx_log_fwif fwif;
	struct iovec vec[4];
	struct timespec64 timespec;

	ktime_get_real_ts64(&timespec);

	header.magic = MVX_LOG_MAGIC;
	header.length = sizeof(fwif) + sizeof(*msg_header) + msg_header->size;
	header.type = MVX_LOG_TYPE_FWIF;
	header.severity = MVX_LOG_INFO;
	header.timestamp.sec = timespec.tv_sec;
	header.timestamp.nsec = timespec.tv_nsec;

	fwif.version_major = 2;
	fwif.version_minor = 0;
	fwif.channel = channel;
	fwif.direction = direction;
	fwif.session = (uintptr_t)session;

	vec[0].iov_base = &header;
	vec[0].iov_len = sizeof(header);

	vec[1].iov_base = &fwif;
	vec[1].iov_len = sizeof(fwif);

	vec[2].iov_base = msg_header;
	vec[2].iov_len = sizeof(*msg_header);

	vec[3].iov_base = data;
	vec[3].iov_len = msg_header->size;

	MVX_LOG_DATA(&mvx_log_fwif_if, MVX_LOG_INFO, vec, 4);
}

/**
 * log_rpc() - Log a RPC message.
 * @session:		Pointer to session.
 * @direction:		The type of the firmware interface message;
 *			host->firmware or firware->host.
 * @rpc:		RPC message.
 */
static void log_rpc(struct mvx_session *session,
		    enum mvx_log_fwif_direction direction,
		    struct mve_rpc_communication_area *rpc)
{
	struct mvx_log_header header;
	struct mvx_log_fwif fwif;
	size_t rpc_size;
	struct iovec vec[3];
	struct timespec64 timespec;

	rpc_size = offsetof(typeof(*rpc), params) + rpc->size;

	if (rpc_size > sizeof(*rpc))
		MVX_LOG_PRINT(&mvx_log_if, MVX_LOG_WARNING,
			      "RPC message size is too large. size=%u.",
			      rpc->size);

	ktime_get_real_ts64(&timespec);

	header.magic = MVX_LOG_MAGIC;
	header.length = sizeof(fwif) + rpc_size;
	header.type = MVX_LOG_TYPE_FWIF;
	header.severity = MVX_LOG_INFO;
	header.timestamp.sec = timespec.tv_sec;
	header.timestamp.nsec = timespec.tv_nsec;

	fwif.version_major = 2;
	fwif.version_minor = 0;
	fwif.channel = MVX_LOG_FWIF_CHANNEL_RPC;
	fwif.direction = direction;
	fwif.session = (uintptr_t)session;

	vec[0].iov_base = &header;
	vec[0].iov_len = sizeof(header);

	vec[1].iov_base = &fwif;
	vec[1].iov_len = sizeof(fwif);

	vec[2].iov_base = rpc;
	vec[2].iov_len = rpc_size;

	MVX_LOG_DATA(&mvx_log_fwif_if, MVX_LOG_INFO, vec, 3);
}

static int get_stride90(enum mvx_format format,
		      uint8_t *nplanes,
		      unsigned int stride[MVX_BUFFER_NPLANES][2])
{
	switch (format) {
	case MVX_FORMAT_YUV420_I420:
		*nplanes = 3;
		stride[0][0] = 2;
		stride[0][1] = 2;
		stride[1][0] = 1;
		stride[1][1] = 1;
		stride[2][0] = 1;
		stride[2][1] = 1;
		break;
	case MVX_FORMAT_YUV420_NV12:
	case MVX_FORMAT_YUV420_NV21:
		*nplanes = 2;
		stride[0][0] = 2;
		stride[0][1] = 2;
		stride[1][0] = 2;
		stride[1][1] = 1;
		stride[2][0] = 0;
		stride[2][1] = 0;
		break;
	case MVX_FORMAT_YUV420_P010:
		*nplanes = 2;
		stride[0][0] = 4;
		stride[0][1] = 2;
		stride[1][0] = 4;
		stride[1][1] = 1;
		stride[2][0] = 0;
		stride[2][1] = 0;
		break;
	case MVX_FORMAT_YUV420_Y0L2:
	case MVX_FORMAT_YUV420_AQB1:
		*nplanes = 1;
		stride[0][0] = 8;
		stride[0][1] = 1;
		stride[1][0] = 0;
		stride[1][1] = 0;
		stride[2][0] = 0;
		stride[2][1] = 0;
		break;
	case MVX_FORMAT_YUV422_YUY2:
	case MVX_FORMAT_YUV422_UYVY:
		*nplanes = 1;
		stride[0][0] = 4;
		stride[0][1] = 2;
		stride[1][0] = 0;
		stride[1][1] = 0;
		stride[2][0] = 0;
		stride[2][1] = 0;
		break;
	case MVX_FORMAT_YUV422_Y210:
	case MVX_FORMAT_RGBA_8888:
	case MVX_FORMAT_BGRA_8888:
	case MVX_FORMAT_ARGB_8888:
	case MVX_FORMAT_ABGR_8888:
		*nplanes = 1;
		stride[0][0] = 8;
		stride[0][1] = 2;
		stride[1][0] = 0;
		stride[1][1] = 0;
		stride[2][0] = 0;
		stride[2][1] = 0;
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

/**
 * read32n() - Read a number of bytes from 'src' to 'dst'.
 * @src:	Pointer to circular buffer of source data.
 * @offset:	Current offset in the circular buffer.
 * @dst:	Pointer to destination buffer.
 * @size:	Size in bytes.
 *
 * Return: New offset in the circular buffer.
 */
static unsigned int read32n(volatile uint32_t *src,
			    unsigned int offset,
			    uint32_t *dst,
			    size_t size)
{
	for (; size >= sizeof(uint32_t); size -= sizeof(uint32_t)) {
		*dst++ = src[offset];
		offset = (offset + 1) % MVE_COMM_QUEUE_SIZE_IN_WORDS;
	}

	if (size != 0) {
		memcpy(dst, (void *)&src[offset], size);
		offset = (offset + 1) % MVE_COMM_QUEUE_SIZE_IN_WORDS;
	}

	return offset;
}

/**
 * read_message() - Read message from firmware message queue.
 * @fw:		Pointer to firmware object.
 * @host:	Host communication area.
 * @mve:	MVE communication area.
 * @code:	Pointer to where the message code shall be placed.
 * @data:	Pointer to where message data shall be placed.
 * @size:	Input: the size of the data. Output: The size of the message.
 * @channel:	Firmware interface message type to log.
 *
 * Return: 1 if a message was read, 0 if no message was read, else error code.
 */
static int read_message(struct mvx_fw *fw,
			struct mve_comm_area_host *host,
			struct mve_comm_area_mve *mve,
			unsigned int *code,
			void *data,
			size_t *size,
			enum mvx_log_fwif_channel channel,
			enum mvx_fw_buffer_attr mve_buf_attr,
			enum mvx_fw_buffer_attr host_buf_attr)
{
	struct mve_msg_header header;
	unsigned int rpos;
	ssize_t capacity;

	if (mve_buf_attr == MVX_FW_BUF_CACHEABLE) {
		dma_sync_single_for_cpu(fw->dev,
			phys_cpu2vpu(virt_to_phys(mve)),
			MVE_PAGE_SIZE, DMA_FROM_DEVICE);
	}

	rpos = host->out_rpos;

	/* Calculate how much data that is available in the buffer. */
	capacity = mve->out_wpos - rpos;
	if (capacity < 0)
		capacity += MVE_COMM_QUEUE_SIZE_IN_WORDS;

	if (capacity <= 0)
		return 0;
        /*
         * It's possible below sequence occurs due to process out-of-order execution:
         * 1. read msg_header first and host_area->out_rpos++;
         * 2. firmware writes msg_header and updates mve_area->out_wpos;
         * 3. compare host_area->out_rpos and mve_area->out_wpos, since the latter
         *    is updated in step 2, the available is not 0;
         * 4. parse the msg_header got in step 1 and it's invalid because it should
         *    be read after update in step 2.
         * Add barrier here to ensure msg_header is always read after firmware  updates.
         */
        smp_rmb();

	/* Read the header. */
	rpos = read32n(mve->out_data, rpos, (uint32_t *)&header,
		       sizeof(header));

	if (!((header.code >= 1001 && header.code <= 1013) || (header.code >= 2001 && header.code <= 2019) || (header.code >= 3001 && header.code <= 3004))) {
		MVX_LOG_PRINT(&mvx_log_if, MVX_LOG_WARNING, "read_message header. header.size=%d, rpos=%d, code=%d, capacity=%zu", header.size, host->out_rpos, header.code, capacity);
		return 0;
	}

	/* Make sure there is enough space for both header and message. */
	capacity -= DIV_ROUND_UP(sizeof(header) + header.size,
				 sizeof(uint32_t));
	if (capacity < 0) {
		MVX_LOG_PRINT(&mvx_log_if, MVX_LOG_WARNING,
			      "Firmware v2 msg larger than capacity. code=%u, size=%u, wpos=%u, rpos=%u.",
			      header.code, header.size, mve->out_wpos,
			      host->out_rpos);
		return -EFAULT;
	}

	if (header.size > *size) {
		MVX_LOG_PRINT(&mvx_log_if, MVX_LOG_WARNING,
			      "Firmware v2 message does not fit in buffer. code=%u, msg_size=%u, size=%zu.",
			      header.code, header.size, *size);
		return -ENOMEM;
	}

	/* Read message body. */
	rpos = read32n(mve->out_data, rpos, data, header.size);
	host->out_rpos = rpos;

	/*
	 * Make sure the read pointer has been written before the cache is
	 * flushed.
	 */
	if (host_buf_attr == MVX_FW_BUF_CACHEABLE) {
		wmb();
		dma_sync_single_for_device(fw->dev,
				   phys_cpu2vpu(virt_to_phys(&host->out_rpos)),
				   sizeof(host->out_rpos), DMA_TO_DEVICE);
	}

	*code = header.code;
	*size = header.size;

	/* Log firmware message. */
	MVX_LOG_EXECUTE(&mvx_log_fwif_if, MVX_LOG_INFO,
			log_message(fw->session, channel,
				    MVX_LOG_FWIF_DIRECTION_FIRMWARE_TO_HOST,
				    &header, data));

	return 1;
}

/**
 * write32n() - Write a number of bytes to 'dst' from 'src'.
 * @dst:	Pointer to circular buffer of destination data.
 * @offset:	Current offset in the circular buffer.
 * @src:	Pointer to source buffer.
 * @size:	Size in bytes.
 *
 * Return: New offset in the circular buffer.
 */
static unsigned int write32n(volatile uint32_t *dst,
			     unsigned int offset,
			     uint32_t *src,
			     size_t size)
{
	for (; size >= sizeof(uint32_t); size -= sizeof(uint32_t)) {
		dst[offset] = *src++;
		offset = (offset + 1) % MVE_COMM_QUEUE_SIZE_IN_WORDS;
	}

	if (size != 0) {
		memcpy((void *)&dst[offset], src, size);
		offset = (offset + 1) % MVE_COMM_QUEUE_SIZE_IN_WORDS;
	}

	return offset;
}

/**
 * write_message() - Write message to firmware message queue.
 * @fw:		Pointer to firmware object.
 * @host:	Host communication area.
 * @mve:	MVE communication area.
 * @code:	Message code.
 * @data:	Pointer to message data. May be NULL if size if 0.
 * @size:	Size in bytes of data.
 * @channel:	Firmware interface message type to log.
 *
 * Return: 0 on success, else error code.
 */
static int write_message(struct mvx_fw *fw,
			 struct mve_comm_area_host *host,
			 struct mve_comm_area_mve *mve,
			 unsigned int code,
			 void *data,
			 size_t size,
			 enum mvx_log_fwif_channel channel,
			 enum mvx_fw_buffer_attr mve_buf_attr,
			 enum mvx_fw_buffer_attr host_buf_attr)
{
	struct mve_msg_header header = { .code = code, .size = size };
	ssize_t capacity;
	unsigned int wpos;

	if (mve_buf_attr == MVX_FW_BUF_CACHEABLE) {
		dma_sync_single_for_cpu(fw->dev,
				phys_cpu2vpu(virt_to_phys(&mve->in_rpos)),
				sizeof(mve->in_rpos), DMA_FROM_DEVICE);
	}

	wpos = host->in_wpos;

	/* Calculate how much space that is available in the buffer. */
	capacity = mve->in_rpos - wpos;
	if (capacity <= 0)
		capacity += MVE_COMM_QUEUE_SIZE_IN_WORDS;

	/* Make sure there is enough space for both header and message. */
	capacity -= DIV_ROUND_UP(sizeof(header) + size, sizeof(uint32_t));
	if (capacity <= 0)
		return -ENOMEM;

	/* Write header. */
	wpos = write32n(host->in_data, wpos, (uint32_t *)&header,
			sizeof(header));

	/* Write message. */
	wpos = write32n(host->in_data, wpos, data, size);

	/*
	 * Make sure all message data has been written before the cache is
	 * flushed.
	 */
	if (host_buf_attr == MVX_FW_BUF_CACHEABLE) {
		wmb();
		dma_sync_single_for_device(fw->dev,
				   phys_cpu2vpu(virt_to_phys(host)),
				   MVE_PAGE_SIZE, DMA_TO_DEVICE);
	}

	host->in_wpos = wpos;

	/*
	 * Make sure the write pointer has been written before the cache is
	 * flushed.
	 */
	if (host_buf_attr == MVX_FW_BUF_CACHEABLE) {
		wmb();
		dma_sync_single_for_device(fw->dev,
				   phys_cpu2vpu(virt_to_phys(&host->in_wpos)),
				   sizeof(host->in_wpos), DMA_TO_DEVICE);
	}

	/* Log firmware message. */
	MVX_LOG_EXECUTE(&mvx_log_fwif_if, MVX_LOG_INFO,
			log_message(fw->session, channel,
				    MVX_LOG_FWIF_DIRECTION_HOST_TO_FIRMWARE,
				    &header, data));

	return 0;
}

static int get_region_v2(enum mvx_fw_region region,
			 uint32_t *begin,
			 uint32_t *end)
{
	switch (region) {
	case MVX_FW_REGION_CORE_0:
		*begin = MVE_MEM_REGION_FW_INSTANCE0_ADDR_BEGIN;
		*end = MVE_MEM_REGION_FW_INSTANCE0_ADDR_END;
		break;
	case MVX_FW_REGION_CORE_1:
		*begin = MVE_MEM_REGION_FW_INSTANCE1_ADDR_BEGIN;
		*end = MVE_MEM_REGION_FW_INSTANCE1_ADDR_END;
		break;
	case MVX_FW_REGION_CORE_2:
		*begin = MVE_MEM_REGION_FW_INSTANCE2_ADDR_BEGIN;
		*end = MVE_MEM_REGION_FW_INSTANCE2_ADDR_END;
		break;
	case MVX_FW_REGION_CORE_3:
		*begin = MVE_MEM_REGION_FW_INSTANCE3_ADDR_BEGIN;
		*end = MVE_MEM_REGION_FW_INSTANCE3_ADDR_END;
		break;
	case MVX_FW_REGION_CORE_4:
		*begin = MVE_MEM_REGION_FW_INSTANCE4_ADDR_BEGIN;
		*end = MVE_MEM_REGION_FW_INSTANCE4_ADDR_END;
		break;
	case MVX_FW_REGION_CORE_5:
		*begin = MVE_MEM_REGION_FW_INSTANCE5_ADDR_BEGIN;
		*end = MVE_MEM_REGION_FW_INSTANCE5_ADDR_END;
		break;
	case MVX_FW_REGION_CORE_6:
		*begin = MVE_MEM_REGION_FW_INSTANCE6_ADDR_BEGIN;
		*end = MVE_MEM_REGION_FW_INSTANCE6_ADDR_END;
		break;
	case MVX_FW_REGION_CORE_7:
		*begin = MVE_MEM_REGION_FW_INSTANCE7_ADDR_BEGIN;
		*end = MVE_MEM_REGION_FW_INSTANCE7_ADDR_END;
		break;
	case MVX_FW_REGION_PROTECTED:
		*begin = MVE_MEM_REGION_PROTECTED_ADDR_BEGIN;
		*end = MVE_MEM_REGION_PROTECTED_ADDR_END;
		break;
	case MVX_FW_REGION_FRAMEBUF:
		*begin = MVE_MEM_REGION_FRAMEBUF_ADDR_BEGIN;
		*end = MVE_MEM_REGION_FRAMEBUF_ADDR_END;
		break;
	case MVX_FW_REGION_MSG_HOST:
		*begin = MVE_COMM_MSG_INQ_ADDR;
		*end = MVE_COMM_MSG_INQ_ADDR + MVE_PAGE_SIZE;
		break;
	case MVX_FW_REGION_MSG_MVE:
		*begin = MVE_COMM_MSG_OUTQ_ADDR;
		*end = MVE_COMM_MSG_OUTQ_ADDR + MVE_PAGE_SIZE;
		break;
	case MVX_FW_REGION_BUF_IN_HOST:
		*begin = MVE_COMM_BUF_INQ_ADDR;
		*end = MVE_COMM_BUF_INQ_ADDR + MVE_PAGE_SIZE;
		break;
	case MVX_FW_REGION_BUF_IN_MVE:
		*begin = MVE_COMM_BUF_INRQ_ADDR;
		*end = MVE_COMM_BUF_INRQ_ADDR + MVE_PAGE_SIZE;
		break;
	case MVX_FW_REGION_BUF_OUT_HOST:
		*begin = MVE_COMM_BUF_OUTQ_ADDR;
		*end = MVE_COMM_BUF_OUTQ_ADDR + MVE_PAGE_SIZE;
		break;
	case MVX_FW_REGION_BUF_OUT_MVE:
		*begin = MVE_COMM_BUF_OUTRQ_ADDR;
		*end = MVE_COMM_BUF_OUTRQ_ADDR + MVE_PAGE_SIZE;
		break;
	case MVX_FW_REGION_RPC:
		*begin = MVE_COMM_RPC_ADDR;
		*end = MVE_COMM_RPC_ADDR + MVE_PAGE_SIZE;
		break;
	case MVX_FW_REGION_PRINT_RAM:
		*begin = MVE_FW_PRINT_RAM_ADDR;
		*end = MVE_FW_PRINT_RAM_ADDR + MVE_FW_PRINT_RAM_SIZE;
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static void convert_buffer_general(struct mvx_fw *fw,
				 enum mvx_direction dir,
				 struct mvx_fw_msg *msg,
				 struct mve_buffer_general *g) {
    struct mvx_buffer *buf = (struct mvx_buffer *)g->header.host_handle;

	if (g->header.host_handle == MVX_FW_CODE_EOS)
		return;

	WARN_ON(buf->dir != dir);

	msg->code = MVX_FW_CODE_BUFFER_GENERAL;
	msg->buf = buf;
}


static void convert_buffer_frame(struct mvx_fw *fw,
				 enum mvx_direction dir,
				 struct mvx_fw_msg *msg,
				 struct mve_buffer_frame *f)
{
	struct mvx_buffer *buf = (struct mvx_buffer *)f->host_handle;

	if (f->host_handle == MVX_FW_CODE_EOS)
		return;

	WARN_ON(buf->dir != dir);

	msg->code = MVX_FW_CODE_BUFFER;
	msg->buf = buf;

	if (dir == MVX_DIR_OUTPUT) {
		unsigned int i;

		buf->width = f->visible_frame_width;
		buf->height = f->visible_frame_height;
		if (buf->width == 0 || buf->height == 0 ||
                (f->frame_flags & (MVE_BUFFER_FRAME_FLAG_TOP_PRESENT | MVE_BUFFER_FRAME_FLAG_BOT_PRESENT)) == 0)
			for (i = 0; i < buf->nplanes; i++)
				(void)mvx_buffer_filled_set(buf, i, 0, 0);

		if (is_afbc(f->format) != false) {
			struct mve_buffer_frame_afbc *afbc = &f->data.afbc;

			buf->crop_left = afbc->cropx;
			buf->crop_top = afbc->cropy;
		}
	}

	buf->user_data = f->user_data_tag;
	buf->flags = 0;
	if (f->frame_flags & MVE_BUFFER_FRAME_FLAG_EOS)
		buf->flags |= MVX_BUFFER_EOS;

	if (f->frame_flags & MVE_BUFFER_FRAME_FLAG_REJECTED)
		buf->flags |= MVX_BUFFER_REJECTED;

	if (f->frame_flags & MVE_BUFFER_FRAME_FLAG_CORRUPT)
		buf->flags |= MVX_BUFFER_CORRUPT;

	if (f->frame_flags & MVE_BUFFER_FRAME_FLAG_DECODE_ONLY)
		buf->flags |= MVX_BUFFER_DECODE_ONLY;

	if (f->frame_flags & (MVE_BUFFER_FRAME_FLAG_TOP_PRESENT | MVE_BUFFER_FRAME_FLAG_BOT_PRESENT)) {
		buf->flags |= MVX_BUFFER_FRAME_PRESENT;
    }

	if (is_afbc(f->format) != false) {
		struct mve_buffer_frame_afbc *afbc = &f->data.afbc;

		if (afbc->afbc_params & MVE_BUFFER_FRAME_AFBC_TILED_HEADER)
			buf->flags |= MVX_BUFFER_AFBC_TILED_HEADERS;

		if (afbc->afbc_params & MVE_BUFFER_FRAME_AFBC_TILED_BODY)
			buf->flags |= MVX_BUFFER_AFBC_TILED_BODY;

		if (afbc->afbc_params & MVE_BUFFER_FRAME_AFBC_32X8_SUPERBLOCK)
			buf->flags |= MVX_BUFFER_AFBC_32X8_SUPERBLOCK;
	}
}

static void convert_buffer_bitstream(struct mvx_fw *fw,
				     enum mvx_direction dir,
				     struct mvx_fw_msg *msg,
				     struct mve_buffer_bitstream *b)
{
	struct mvx_buffer *buf = (struct mvx_buffer *)b->host_handle;

	if (b->host_handle == MVX_FW_CODE_EOS)
		return;

	WARN_ON(buf->dir != dir);

	msg->code = MVX_FW_CODE_BUFFER;
	msg->buf = buf;

	if (dir == MVX_DIR_OUTPUT)
		mvx_buffer_filled_set(buf, 0, b->bitstream_filled_len,
				      b->bitstream_offset);

	buf->user_data = b->user_data_tag;
	buf->flags = 0;
    if (dir == MVX_DIR_INPUT) {
        struct mvx_corrupt_buffer *corrupt_buf;
        struct mvx_corrupt_buffer *tmp;
        list_for_each_entry_safe(corrupt_buf, tmp, &fw->session->buffer_corrupt_queue, head) {
            if (corrupt_buf->user_data == buf->user_data) {
                list_del(&corrupt_buf->head);
                buf->flags |= MVX_BUFFER_CORRUPT;
                vfree(corrupt_buf);
            }
        }
    }
	if (b->bitstream_flags & MVE_BUFFER_BITSTREAM_FLAG_EOS)
		buf->flags |= MVX_BUFFER_EOS;

        if (b->bitstream_flags & MVE_BUFFER_BITSTREAM_FLAG_ENDOFFRAME)
                buf->flags |= MVX_BUFFER_EOF;

        if (b->bitstream_flags & MVE_BUFFER_BITSTREAM_FLAG_CODECCONFIG)
                buf->flags |= MVX_BUFFER_CODEC_CONFIG;

        if (b->frame_type == 0) {
            buf->flags |= MVX_BUFFER_FRAME_FLAG_IFRAME;
        } else if (b->frame_type == 1) {
            buf->flags |= MVX_BUFFER_FRAME_FLAG_PFRAME;
        } else if (b->frame_type == 2) {
            buf->flags |= MVX_BUFFER_FRAME_FLAG_BFRAME;
        }
}

static int convert_buffer_param(struct mvx_fw *fw,
				struct mvx_fw_msg *msg,
				struct mve_buffer_param *p)
{
	switch (p->type) {
	case MVE_BUFFER_PARAM_TYPE_COLOUR_DESCRIPTION: {
		struct mve_buffer_param_colour_description *c =
			&p->data.colour_description;
		struct mvx_fw_color_desc *d = &msg->color_desc;

		d->flags = 0;

		switch (c->range) {
		case MVE_BUFFER_PARAM_COLOUR_RANGE_UNSPECIFIED:
			d->range = MVX_FW_RANGE_UNSPECIFIED;
			break;
		case MVE_BUFFER_PARAM_COLOUR_RANGE_LIMITED:
			d->range = MVX_FW_RANGE_LIMITED;
			break;
		case MVE_BUFFER_PARAM_COLOUR_RANGE_FULL:
			d->range = MVX_FW_RANGE_FULL;
			break;
		default:
			MVX_LOG_PRINT(&mvx_log_if, MVX_LOG_WARNING,
				      "Unknown fw buffer param color desc range. range=%u.",
				      c->range);
			return -EINVAL;
		}

		/* Color primaries according to HEVC E.3.1. */
		switch (c->colour_primaries) {
		case 1:
			d->primaries = MVX_FW_PRIMARIES_BT709;
			break;
		case 4:
			d->primaries = MVX_FW_PRIMARIES_BT470M;
			break;
		case 5:
			d->primaries = MVX_FW_PRIMARIES_BT601_625;
			break;
		case 6:
		case 7:
			d->primaries = MVX_FW_PRIMARIES_BT601_525;
			break;
		case 8:
			d->primaries = MVX_FW_PRIMARIES_GENERIC_FILM;
			break;
		case 9:
			d->primaries = MVX_FW_PRIMARIES_BT2020;
			break;
		default:
			d->primaries = MVX_FW_PRIMARIES_UNSPECIFIED;
			break;
		}

		/* Transfer characteristics according to HEVC E.3.1. */
		switch (c->transfer_characteristics) {
		case 12:
		        d->transfer = MVX_FW_TRANSFER_BT1361;
		        break;
		case 1:
			d->transfer = MVX_FW_TRANSFER_SMPTE170M;
			break;
		case 4:
			d->transfer = MVX_FW_TRANSFER_GAMMA22;
			break;
		case 5:
			d->transfer = MVX_FW_TRANSFER_GAMMA28;
			break;
		case 6:
		case 14:
		case 15:
			d->transfer = MVX_FW_TRANSFER_SMPTE170M;
			break;
		case 7:
			d->transfer = MVX_FW_TRANSFER_SMPTE240M;
			break;
		case 8:
			d->transfer = MVX_FW_TRANSFER_LINEAR;
			break;
		case 9:
		case 10:
			d->transfer = MVX_FW_TRANSFER_UNSPECIFIED;
			break;
		case 18:
			d->transfer = MVX_FW_TRANSFER_HLG;
			break;
		case 11:
			d->transfer = MVX_FW_TRANSFER_XVYCC;
			break;
		case 13:
			d->transfer = MVX_FW_TRANSFER_SRGB;
			break;
		case 16:
			d->transfer = MVX_FW_TRANSFER_ST2084;
			break;
		case 17:
			d->transfer = MVX_FW_TRANSFER_ST428;
			break;
		default:
			d->transfer = MVX_FW_TRANSFER_UNSPECIFIED;
			break;
		}

		/* Matrix coefficient according to HEVC E.3.1. */
		switch (c->matrix_coeff) {
		case 1:
			d->matrix = MVX_FW_MATRIX_BT709;
			break;
		case 4:
			d->matrix = MVX_FW_MATRIX_BT470M;
			break;
		case 5:
		case 6:
			d->matrix = MVX_FW_MATRIX_BT601;
			break;
		case 7:
			d->matrix = MVX_FW_MATRIX_SMPTE240M;
			break;
		case 9:
			d->matrix = MVX_FW_MATRIX_BT2020;
			break;
		case 10:
			d->matrix = MVX_FW_MATRIX_BT2020Constant;
			break;
		default:
			d->matrix = MVX_FW_MATRIX_UNSPECIFIED;
			break;
		}

		if (c->flags &
		    MVE_BUFFER_PARAM_COLOUR_FLAG_MASTERING_DISPLAY_DATA_VALID) {
			d->flags |= MVX_FW_COLOR_DESC_DISPLAY_VALID;

			d->display.r.x = c->mastering_display_primaries_x[0];
			d->display.r.y = c->mastering_display_primaries_y[0];
			d->display.g.x = c->mastering_display_primaries_x[1];
			d->display.g.y = c->mastering_display_primaries_y[1];
			d->display.b.x = c->mastering_display_primaries_x[2];
			d->display.b.y = c->mastering_display_primaries_y[2];
			d->display.w.x = c->mastering_white_point_x;
			d->display.w.y = c->mastering_white_point_y;

			d->display.luminance_min =
				c->min_display_mastering_luminance;
			d->display.luminance_max =
				c->max_display_mastering_luminance;
		}

		if (c->flags &
		    MVE_BUFFER_PARAM_COLOUR_FLAG_CONTENT_LIGHT_DATA_VALID) {
			d->flags |= MVX_FW_COLOR_DESC_CONTENT_VALID;

			d->content.luminance_max = c->max_content_light_level;
			d->content.luminance_average =
				c->avg_content_light_level;
		}

		msg->code = MVX_FW_CODE_COLOR_DESC;
		break;
	}
	case MVE_BUFFER_PARAM_TYPE_DPB_HELD_FRAMES: {
		msg->arg = p->data.arg;
		msg->code = MVX_FW_CODE_DPB_HELD_FRAMES;
		break;
	}

	default:
		MVX_LOG_PRINT(&mvx_log_if, MVX_LOG_INFO,
			      "Default buffer param. type=%d", p->type);
		break;
	}

	return 1;
}

static int get_buffer(struct mvx_fw *fw,
		      struct mve_comm_area_host *host,
		      struct mve_comm_area_mve *mve,
		      enum mvx_direction dir,
		      struct mvx_fw_msg *msg,
		      enum mvx_log_fwif_channel channel,
		      enum mvx_fw_buffer_attr mve_buf_attr,
		      enum mvx_fw_buffer_attr host_buf_attr)
{
	unsigned int code;
	union {
		struct mve_buffer_frame frame;
		struct mve_buffer_bitstream bitstream;
		struct mve_buffer_param param;
        struct mve_buffer_general general;
	} fw_msg;
	size_t size = sizeof(fw_msg);
	int ret;
	struct mvx_session *session = fw->session;
	struct mvx_v4l2_session *vsession =
		           container_of(session, struct mvx_v4l2_session, session);

	ret = read_message(fw, host, mve, &code, &fw_msg, &size, channel, mve_buf_attr, host_buf_attr);
	if (ret <= 0)
		return ret;

	if (vsession->port[dir].q_set == false) {
		MVX_SESSION_WARN(session, "vb2 queue is released. dir=%d, code=%d.", dir, code);
		return 0;
	}

	switch (code) {
	case MVE_BUFFER_CODE_FRAME:
		convert_buffer_frame(fw, dir, msg, &fw_msg.frame);
		break;
	case MVE_BUFFER_CODE_BITSTREAM:
		convert_buffer_bitstream(fw, dir, msg, &fw_msg.bitstream);
		break;
	case MVE_BUFFER_CODE_PARAM:
		convert_buffer_param(fw, msg, &fw_msg.param);
		break;
    case MVE_BUFFER_CODE_GENERAL:
        convert_buffer_general(fw, dir, msg, &fw_msg.general);
        break;
	default:
		MVX_LOG_PRINT(&mvx_log_if, MVX_LOG_WARNING,
			      "Unknown fw buffer code. code=%u.", code);
		break;
	}

	return 1;
}

static int get_message_v2(struct mvx_fw *fw,
			  struct mvx_fw_msg *msg)
{
	unsigned int code;
	union {
		struct mve_request_job job;
		struct mve_response_state_change state_change;
		struct mve_response_error error;
		struct mve_response_frame_alloc_parameters alloc_param;
		struct mve_response_sequence_parameters seq_param;
		struct mve_response_set_option_fail set_option_fail;
		struct mve_buffer_param buffer_param;
		struct mve_response_event event;
	} fw_msg;
	size_t size = sizeof(fw_msg);
	int ret;
	struct mvx_session *session = fw->session;

	ret = read_message(fw, fw->msg_host, fw->msg_mve, &code, &fw_msg,
			   &size, MVX_LOG_FWIF_CHANNEL_MESSAGE, fw->buf_attr[MVX_FW_REGION_MSG_MVE], fw->buf_attr[MVX_FW_REGION_MSG_HOST]);
	if (ret <= 0)
		return ret;

	msg->code = MVX_FW_CODE_MAX;

	switch (code) {
	case MVE_RESPONSE_CODE_SWITCHED_IN:
		msg->code = MVX_FW_CODE_SWITCH_IN;
		break;
	case MVE_RESPONSE_CODE_SWITCHED_OUT:
		msg->code = MVX_FW_CODE_SWITCH_OUT;
		break;
	case MVE_RESPONSE_CODE_SET_OPTION_CONFIRM:
		msg->code = MVX_FW_CODE_SET_OPTION;
		fw->msg_pending--;
		break;
	case MVE_RESPONSE_CODE_SET_OPTION_FAIL: {
		MVX_LOG_PRINT(&mvx_log_if, MVX_LOG_WARNING,
			      "Firmware set option failed. index=%u, msg=%s.",
			      fw_msg.set_option_fail.index,
			      fw_msg.set_option_fail.message);
		msg->code = MVX_FW_CODE_SET_OPTION;
		fw->msg_pending--;
		break;
	}
	case MVE_RESPONSE_CODE_JOB_DEQUEUED:
		msg->code = MVX_FW_CODE_JOB;
		break;
	case MVE_RESPONSE_CODE_INPUT:
		ret = get_buffer(fw, fw->buf_in_host, fw->buf_in_mve,
				 MVX_DIR_INPUT, msg,
				 MVX_LOG_FWIF_CHANNEL_INPUT_BUFFER, fw->buf_attr[MVX_FW_REGION_BUF_IN_MVE], fw->buf_attr[MVX_FW_REGION_BUF_IN_HOST]);
		break;
	case MVE_RESPONSE_CODE_OUTPUT:
		ret = get_buffer(fw, fw->buf_out_host, fw->buf_out_mve,
				 MVX_DIR_OUTPUT, msg,
				 MVX_LOG_FWIF_CHANNEL_OUTPUT_BUFFER, fw->buf_attr[MVX_FW_REGION_BUF_OUT_MVE], fw->buf_attr[MVX_FW_REGION_BUF_OUT_HOST]);
		break;
	case MVE_BUFFER_CODE_PARAM:
		ret = convert_buffer_param(fw, msg, &fw_msg.buffer_param);
		break;
	case MVE_RESPONSE_CODE_INPUT_FLUSHED:
		msg->code = MVX_FW_CODE_FLUSH;
		msg->flush.dir = MVX_DIR_INPUT;
		fw->msg_pending--;
		break;
	case MVE_RESPONSE_CODE_OUTPUT_FLUSHED:
		msg->code = MVX_FW_CODE_FLUSH;
		msg->flush.dir = MVX_DIR_OUTPUT;
		fw->msg_pending--;
		break;
	case MVE_RESPONSE_CODE_PONG:
		msg->code = MVX_FW_CODE_PONG;
		break;
	case MVE_RESPONSE_CODE_ERROR: {
		msg->code = MVX_FW_CODE_ERROR;

		switch (fw_msg.error.error_code) {
		case MVE_ERROR_ABORT:
			msg->error.error_code = MVX_FW_ERROR_ABORT;
			break;
		case MVE_ERROR_OUT_OF_MEMORY:
			msg->error.error_code = MVX_FW_ERROR_OUT_OF_MEMORY;
			break;
		case MVE_ERROR_ASSERT:
			msg->error.error_code = MVX_FW_ERROR_ASSERT;
			break;
		case MVE_ERROR_UNSUPPORTED:
			msg->error.error_code = MVX_FW_ERROR_UNSUPPORTED;
			break;
		case MVE_ERROR_INVALID_BUFFER:
			msg->error.error_code = MVX_FW_ERROR_INVALID_BUFFER;
			break;
		case MVE_ERROR_INVALID_STATE:
			msg->error.error_code = MVX_FW_ERROR_INVALID_STATE;
			break;
		case MVE_ERROR_WATCHDOG:
			msg->error.error_code = MVX_FW_ERROR_WATCHDOG;
			break;
		default:
			MVX_LOG_PRINT(&mvx_log_if, MVX_LOG_WARNING,
				      "Unsupported fw error code. code=%u.",
				      fw_msg.error.error_code);
			break;
		}

		strlcpy(msg->error.message, fw_msg.error.message,
			min(sizeof(msg->error.message),
			    sizeof(fw_msg.error.message)));

		break;
	}
	case MVE_RESPONSE_CODE_STATE_CHANGE: {
		msg->code = MVX_FW_CODE_STATE_CHANGE;

		if (fw_msg.state_change.new_state == MVE_STATE_STOPPED)
			msg->state = MVX_FW_STATE_STOPPED;
		else
			msg->state = MVX_FW_STATE_RUNNING;

		fw->msg_pending--;
		break;
	}
	case MVE_RESPONSE_CODE_DUMP:
		msg->code = MVX_FW_CODE_DUMP;
		fw->msg_pending--;
		break;
	case MVE_RESPONSE_CODE_DEBUG:
		msg->code = MVX_FW_CODE_DEBUG;
		fw->msg_pending--;
		break;
	case MVE_RESPONSE_CODE_IDLE:
		msg->code = MVX_FW_CODE_IDLE;
		break;
	case MVE_RESPONSE_CODE_FRAME_ALLOC_PARAM:
		msg->code = MVX_FW_CODE_ALLOC_PARAM;
		msg->alloc_param.width =
			fw_msg.alloc_param.planar_alloc_frame_width;
		msg->alloc_param.height =
			fw_msg.alloc_param.planar_alloc_frame_height;
		msg->alloc_param.afbc_alloc_bytes =
			fw_msg.alloc_param.afbc_alloc_bytes;
		msg->alloc_param.afbc_width =
			fw_msg.alloc_param.afbc_width_in_superblocks;

		break;
	case MVE_RESPONSE_CODE_SEQUENCE_PARAMETERS:
		msg->code = MVX_FW_CODE_SEQ_PARAM;
		msg->seq_param.planar.buffers_min =
			fw_msg.seq_param.num_buffers_planar;
		msg->seq_param.afbc.buffers_min =
			fw_msg.seq_param.num_buffers_afbc;
		session->port[MVX_DIR_OUTPUT].interlaced = fw_msg.seq_param.interlace;
		break;
	case MVE_RESPONSE_CODE_EVENT:
        if (MVE_EVENT_ERROR_STREAM_CORRUPT == fw_msg.event.event_code) {
            int ret = 0;
            bool is_find = false;
            struct mvx_corrupt_buffer *buf_corrupt;
            uint64_t user_data = 0;
            char* tmp;
            char* err_msg = fw_msg.event.event_data.message;
            char* err_msg_sep = vmalloc(sizeof(char)*MVE_MAX_ERROR_MESSAGE_SIZE);
            memcpy(err_msg_sep,err_msg,MVE_MAX_ERROR_MESSAGE_SIZE);
            tmp = strsep(&err_msg_sep, ",");
            while (tmp != NULL) {
                ret = sscanf(tmp,"UDT[%llx]",&user_data);
                if (ret == 1) {
                    is_find = true;
                    break;
                }
                tmp = strsep(&err_msg_sep, ",");
            }
            vfree(err_msg_sep);
            if (!is_find) {
               MVX_LOG_PRINT(&mvx_log_if, MVX_LOG_ERROR, "EVENT code=%d %s.but not found UDT ret:%d",fw_msg.event.event_code,fw_msg.event.event_data.message,ret);
            } else {
                buf_corrupt = vmalloc(sizeof(struct mvx_corrupt_buffer));
                buf_corrupt->user_data = user_data;
                list_add(&buf_corrupt->head, &fw->session->buffer_corrupt_queue);
            }
        }
        if (MVE_EVENT_ERROR_STREAM_NOT_SUPPORTED == fw_msg.event.event_code) {
            MVX_LOG_PRINT(&mvx_log_if, MVX_LOG_WARNING, "event STREAM_NOT_SUPPORTED. code=%d. %s.",fw_msg.event.event_code,fw_msg.event.event_data.message);
            wake_up(&session->waitq);
            session->event(session, MVX_SESSION_EVENT_ERROR, (void *)(-EINVAL));
        }
		MVX_LOG_PRINT(&mvx_log_if, MVX_LOG_INFO,
				      "EVENT code=%d. %s",
				      fw_msg.event.event_code,
				      (MVE_EVENT_ERROR_STREAM_CORRUPT == fw_msg.event.event_code
				      	|| MVE_EVENT_ERROR_STREAM_NOT_SUPPORTED == fw_msg.event.event_code) ? fw_msg.event.event_data.message : "");
        if (fw_msg.event.event_code == MVE_EVENT_PROCESSED) {
            session->bus_read_bytes_total += fw_msg.event.event_data.event_processed.bus_read_bytes;
            session->bus_write_bytes_total += fw_msg.event.event_data.event_processed.bus_write_bytes;
            if (fw_msg.event.event_data.event_processed.pic_format == 0) {
                session->frame_id++;
            }
        }
		break;
	case MVE_RESPONSE_CODE_REF_FRAME_UNUSED:
		break;
	default:
		MVX_LOG_PRINT(&mvx_log_if, MVX_LOG_WARNING,
			      "Unknown fw message code. code=%u, size=%u.",
			      code, size);
		msg->code = MVX_FW_CODE_UNKNOWN;
		break;
	}

	return ret;
}

static int put_buffer_general(struct mvx_fw *fw,
			    struct mve_comm_area_host *host,
			    struct mve_comm_area_mve *mve,
			    struct mvx_fw_msg *msg,
			    enum mvx_log_fwif_channel channel,
			    enum mvx_fw_buffer_attr mve_buf_attr,
			    enum mvx_fw_buffer_attr host_buf_attr)
{
    int ret;
    struct mve_buffer_general g = { 0 };
    struct mvx_buffer *buf = msg->buf;
    g.header.host_handle = (ptrdiff_t)buf;
    g.header.user_data_tag = buf->user_data;
    g.header.buffer_ptr = mvx_buffer_va(buf, 0);
    g.header.buffer_size = buf->general.header.buffer_size;
    g.header.config_size = buf->general.header.config_size;
    g.header.type = buf->general.header.type;

    memcpy(&g.config, &buf->general.config, sizeof(buf->general.config));
    ret = write_message(fw, host, mve, MVE_BUFFER_CODE_GENERAL, &g,
			    sizeof(g), channel, mve_buf_attr, host_buf_attr);

	return ret;
}

static int put_buffer_frame(struct mvx_fw *fw,
			    struct mve_comm_area_host *host,
			    struct mve_comm_area_mve *mve,
			    struct mvx_fw_msg *msg,
			    enum mvx_log_fwif_channel channel,
			    enum mvx_fw_buffer_attr mve_buf_attr,
			    enum mvx_fw_buffer_attr host_buf_attr)
{
	struct mve_buffer_frame f = { 0 };
	struct mvx_buffer *buf = msg->buf;
    struct mvx_session *session = fw->session;
	int ret;
	int stride_shift = 0, stride = 0;
	unsigned int strideRot[MVX_BUFFER_NPLANES];
	int max_height;
	uint32_t scaling_shift = 0;
	uint32_t rotation = (buf->flags & MVX_BUFFER_FRAME_FLAG_ROTATION_MASK) >> 12;
	scaling_shift = (buf->flags & MVX_BUFFER_FRAME_FLAG_SCALING_MASK) >> 14;
	f.host_handle = (ptrdiff_t)buf;
	f.user_data_tag = buf->user_data;

	if (buf->dir == MVX_DIR_INPUT) {
		f.visible_frame_width = buf->width;
		f.visible_frame_height = buf->height;

		if (buf->flags & MVX_BUFFER_EOS)
			f.frame_flags |= MVE_BUFFER_FRAME_FLAG_EOS;

		if (buf->planes[0].filled != 0)
			f.frame_flags |= MVE_BUFFER_FRAME_FLAG_TOP_PRESENT;
	}

    if (buf->dir == MVX_DIR_OUTPUT && (session->dsl_ratio.hor != 1 || session->dsl_ratio.ver != 1)) {
        f.frame_flags |= ((session->dsl_ratio.hor - 1) << 24 | (session->dsl_ratio.ver - 1) << 17);
    }
	if (buf->flags & MVX_BUFFER_INTERLACE)
		f.frame_flags |= MVE_BUFFER_FRAME_FLAG_INTERLACE;

	f.frame_flags |= (buf->flags & MVX_BUFFER_FRAME_FLAG_ROTATION_MASK) >> 8;
	f.frame_flags |= (buf->flags & MVX_BUFFER_FRAME_FLAG_MIRROR_MASK) >> 8;
	f.frame_flags |= (buf->flags & MVX_BUFFER_FRAME_FLAG_SCALING_MASK) >> 8;
	if (buf->dir == MVX_DIR_OUTPUT && (rotation == 1 || rotation == 3)) {
		uint8_t nplanes = 0;
		unsigned int stride90[MVX_BUFFER_NPLANES][2];
		int i;
		get_stride90(buf->format, &nplanes, stride90);
		for (i = 0; i < buf->nplanes; i++) {
			const unsigned int stride_align = 1;
			unsigned int tmp = DIV_ROUND_UP(buf->height * stride90[i][0], 2);
			strideRot[i] = round_up(tmp, stride_align);
		}
	}

	switch (buf->format) {
	case MVX_FORMAT_YUV420_AFBC_8:
		f.format = MVE_FORMAT_YUV420_AFBC_8;
		break;
	case MVX_FORMAT_YUV420_AFBC_10:
		f.format = MVE_FORMAT_YUV420_AFBC_10;
		break;
	case MVX_FORMAT_YUV422_AFBC_8:
		f.format = MVE_FORMAT_YUV422_AFBC_8;
		break;
	case MVX_FORMAT_YUV422_AFBC_10:
		f.format = MVE_FORMAT_YUV422_AFBC_10;
		break;
	case MVX_FORMAT_YUV420_I420:
		f.format = MVE_FORMAT_YUV420_I420;
		break;
	case MVX_FORMAT_YUV420_NV12:
		f.format = MVE_FORMAT_YUV420_NV12;
		break;
	case MVX_FORMAT_YUV420_NV21:
		f.format = MVE_FORMAT_YUV420_NV21;
		break;
	case MVX_FORMAT_YUV420_P010:
		f.format = MVE_FORMAT_YUV420_P010;
		break;
	case MVX_FORMAT_YUV420_Y0L2:
		f.format = MVE_FORMAT_YUV420_Y0L2;
		break;
	case MVX_FORMAT_YUV420_AQB1:
		f.format = MVE_FORMAT_YUV420_AQB1;
		break;
	case MVX_FORMAT_YUV422_YUY2:
		f.format = MVE_FORMAT_YUV422_YUY2;
		break;
	case MVX_FORMAT_YUV422_UYVY:
		f.format = MVE_FORMAT_YUV422_UYVY;
		break;
	case MVX_FORMAT_YUV422_Y210:
		f.format = MVE_FORMAT_YUV422_Y210;
		break;
	case MVX_FORMAT_RGBA_8888:
		f.format = MVE_FORMAT_RGBA_8888;
		break;
	case MVX_FORMAT_BGRA_8888:
		f.format = MVE_FORMAT_BGRA_8888;
		break;
	case MVX_FORMAT_ARGB_8888:
		f.format = MVE_FORMAT_ARGB_8888;
		break;
	case MVX_FORMAT_ABGR_8888:
		f.format = MVE_FORMAT_ABGR_8888;
		break;
	default:
		MVX_LOG_PRINT(&mvx_log_if, MVX_LOG_WARNING,
			      "Unsupported frame format. format=%u.",
			      buf->format);
		return -EINVAL;
	}

	if (is_afbc(f.format) == false) {
		struct mve_buffer_frame_planar *planar = &f.data.planar;
		int i;

		if (f.frame_flags & MVE_BUFFER_FRAME_FLAG_INTERLACE){
			max_height = buf->width;
			stride_shift = 1;
			max_height >>= 1;
		}
#if 0
		for (i = 0; i < buf->nplanes; i++) {
			struct mvx_buffer_plane *plane = &buf->planes[i];

			if (plane->stride > 0)
				planar->plane_top[i] = mvx_buffer_va(buf, i);

			planar->stride[i] = plane->stride;
			planar->plane_bot[i] = 0;

			if (f.frame_flags & MVE_BUFFER_FRAME_FLAG_INTERLACE)

				planar->plane_bot[i] = planar->plane_top[i] +
						       DIV_ROUND_UP(
					plane->filled, 2);
		}
#else
		for (i = 0; i < buf->nplanes; i++) {
			struct mvx_buffer_plane *plane = &buf->planes[i];

			if (plane->stride > 0) {
				planar->plane_top[i] = mvx_buffer_va(buf, i);
			}

			if (f.frame_flags & MVE_BUFFER_FRAME_FLAG_INTERLACE) {
				// interlace mode
				stride = plane->stride;
				//stride_shift = 1;
				if (stride_shift) {
					stride = round_up(stride, 2) << stride_shift;
				}
				planar->stride[i] = stride;
				planar->plane_bot[i] = planar->plane_top[i] +
									(round_up(stride, 2) >> stride_shift);
                if (buf->dir == MVX_DIR_OUTPUT && (rotation == 1 || rotation == 3)) {
                    planar->stride[i] = strideRot[i];
                }
			} else {
				// frame mode
                if (buf->dir == MVX_DIR_OUTPUT && (rotation == 1 || rotation == 3)) {
                    planar->stride[i] = strideRot[i];
                } else {
                    planar->stride[i] = plane->stride;
                }
                planar->plane_bot[i] = 0;

			}
		}

#endif
		if (buf->dir == MVX_DIR_OUTPUT && (rotation == 1 || rotation == 3)) {
            planar->max_frame_width = buf->height;
            planar->max_frame_height = buf->width;
        } else {
            planar->max_frame_width = buf->width;
            planar->max_frame_height = buf->height;
        }
	} else {
		struct mve_buffer_frame_afbc *afbc = &f.data.afbc;

		afbc->afbc_width_in_superblocks[0] = buf->planes[0].afbc_width;
		afbc->plane[0] = mvx_buffer_va(buf, 0);

		if (f.frame_flags & MVE_BUFFER_FRAME_FLAG_INTERLACE) {
			afbc->alloc_bytes[0] =
				ALIGN((buf->planes[0].filled / 2), 32);
			afbc->alloc_bytes[1] =
				buf->planes[0].filled - afbc->alloc_bytes[0];
			afbc->plane[1] =
				afbc->plane[0] + afbc->alloc_bytes[0];
			afbc->afbc_width_in_superblocks[1] =
				afbc->afbc_width_in_superblocks[0];
		} else {
			afbc->alloc_bytes[0] = buf->planes[0].filled;
		}

		afbc->afbc_params = 0;
		if (buf->flags & MVX_BUFFER_AFBC_TILED_HEADERS)
			afbc->afbc_params |= MVE_BUFFER_FRAME_AFBC_TILED_HEADER;

		if (buf->flags & MVX_BUFFER_AFBC_TILED_BODY)
			afbc->afbc_params |= MVE_BUFFER_FRAME_AFBC_TILED_BODY;

		if (buf->flags & MVX_BUFFER_AFBC_32X8_SUPERBLOCK)
			afbc->afbc_params |=
				MVE_BUFFER_FRAME_AFBC_32X8_SUPERBLOCK;
		if (buf->flags & MVX_BUFFER_AFBC_BLOCK_SPLIT)
			afbc->afbc_params |=
				MVE_BUFFER_FRAME_AFBC_BLOCK_SPLIT;
	}

	ret = write_message(fw, host, mve, MVE_BUFFER_CODE_FRAME,
			    &f, sizeof(f), channel, mve_buf_attr, host_buf_attr);

	return ret;
}

static int put_buffer_bitstream(struct mvx_fw *fw,
				struct mve_comm_area_host *host,
				struct mve_comm_area_mve *mve,
				struct mvx_fw_msg *msg,
				enum mvx_log_fwif_channel channel,
				enum mvx_fw_buffer_attr mve_buf_attr,
				enum mvx_fw_buffer_attr host_buf_attr)
{
	struct mve_buffer_bitstream b = { 0 };
	struct mvx_buffer *buf = msg->buf;
	int ret;

	if (buf->dir == MVX_DIR_INPUT)
		b.bitstream_filled_len = buf->planes[0].filled;

	b.host_handle = (ptrdiff_t)buf;
	b.user_data_tag = buf->user_data;
	b.bitstream_alloc_bytes = mvx_buffer_size(buf, 0);
	b.bitstream_buf_addr = mvx_buffer_va(buf, 0);

	if (buf->flags & MVX_BUFFER_EOS)
		b.bitstream_flags |= MVE_BUFFER_BITSTREAM_FLAG_EOS;

	if (buf->flags & MVX_BUFFER_EOF)
		b.bitstream_flags |= MVE_BUFFER_BITSTREAM_FLAG_ENDOFFRAME;

	if (buf->flags & MVX_BUFFER_END_OF_SUB_FRAME) {
		b.bitstream_flags |= MVE_BUFFER_BITSTREAM_FLAG_ENDOFSUBFRAME;
	}
	if (buf->flags & MVX_BUFFER_CODEC_CONFIG) {
		b.bitstream_flags |= MVE_BUFFER_BITSTREAM_FLAG_CODECCONFIG;
		b.bitstream_flags |= MVE_BUFFER_BITSTREAM_FLAG_ENDOFSUBFRAME;
	}

	ret = write_message(fw, host, mve, MVE_BUFFER_CODE_BITSTREAM, &b,
			    sizeof(b), channel, mve_buf_attr, host_buf_attr);

	return ret;
}

static int to_mve_nalu_format(enum mvx_nalu_format fmt,
			      int *mve_val)
{
	switch (fmt) {
	case MVX_NALU_FORMAT_START_CODES:
		*mve_val = MVE_OPT_NALU_FORMAT_START_CODES;
		break;
	case MVX_NALU_FORMAT_ONE_NALU_PER_BUFFER:
		*mve_val = MVE_OPT_NALU_FORMAT_ONE_NALU_PER_BUFFER;
		break;
	case MVX_NALU_FORMAT_ONE_BYTE_LENGTH_FIELD:
		*mve_val = MVE_OPT_NALU_FORMAT_ONE_BYTE_LENGTH_FIELD;
		break;
	case MVX_NALU_FORMAT_TWO_BYTE_LENGTH_FIELD:
		*mve_val = MVE_OPT_NALU_FORMAT_TWO_BYTE_LENGTH_FIELD;
		break;
    case MVX_NALU_FORMAT_FOUR_BYTE_LENGTH_FIELD:
        *mve_val = MVE_OPT_NALU_FORMAT_FOUR_BYTE_LENGTH_FIELD;
        break;
	default:
		return -EINVAL;
	}

	return 0;
}

int mvx_fw_to_mve_profile_v2(unsigned int mvx_profile,
			     uint16_t *mve_profile)
{
	switch (mvx_profile) {
	case MVX_PROFILE_H264_BASELINE:
		*mve_profile = MVE_OPT_PROFILE_H264_BASELINE;
		break;
	case MVX_PROFILE_H264_MAIN:
		*mve_profile = MVE_OPT_PROFILE_H264_MAIN;
		break;
	case MVX_PROFILE_H264_HIGH:
		*mve_profile = MVE_OPT_PROFILE_H264_HIGH;
		break;
	case MVX_PROFILE_H265_MAIN:
		*mve_profile = MVE_OPT_PROFILE_H265_MAIN;
		break;
	case MVX_PROFILE_H265_MAIN_STILL:
		*mve_profile = MVE_OPT_PROFILE_H265_MAIN_STILL;
		break;
	case MVX_PROFILE_H265_MAIN_INTRA:
		*mve_profile = MVE_OPT_PROFILE_H265_MAIN_INTRA;
		break;
	case MVX_PROFILE_VC1_SIMPLE:
		*mve_profile = MVE_OPT_PROFILE_VC1_SIMPLE;
		break;
	case MVX_PROFILE_VC1_MAIN:
		*mve_profile = MVE_OPT_PROFILE_VC1_MAIN;
		break;
	case MVX_PROFILE_VC1_ADVANCED:
		*mve_profile = MVE_OPT_PROFILE_VC1_ADVANCED;
		break;
	case MVX_PROFILE_VP8_MAIN:
		*mve_profile = MVE_OPT_PROFILE_VP8_MAIN;
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

int mvx_fw_to_mve_level_v2(unsigned int mvx_level,
			   uint16_t *mve_level)
{
	switch (mvx_level) {
	case MVX_LEVEL_NONE:
		*mve_level = 0;
		break;
	case MVX_LEVEL_H264_1:
		*mve_level = MVE_OPT_LEVEL_H264_1;
		break;
	case MVX_LEVEL_H264_1b:
		*mve_level = MVE_OPT_LEVEL_H264_1b;
		break;
	case MVX_LEVEL_H264_11:
		*mve_level = MVE_OPT_LEVEL_H264_11;
		break;
	case MVX_LEVEL_H264_12:
		*mve_level = MVE_OPT_LEVEL_H264_12;
		break;
	case MVX_LEVEL_H264_13:
		*mve_level = MVE_OPT_LEVEL_H264_13;
		break;
	case MVX_LEVEL_H264_2:
		*mve_level = MVE_OPT_LEVEL_H264_2;
		break;
	case MVX_LEVEL_H264_21:
		*mve_level = MVE_OPT_LEVEL_H264_21;
		break;
	case MVX_LEVEL_H264_22:
		*mve_level = MVE_OPT_LEVEL_H264_22;
		break;
	case MVX_LEVEL_H264_3:
		*mve_level = MVE_OPT_LEVEL_H264_3;
		break;
	case MVX_LEVEL_H264_31:
		*mve_level = MVE_OPT_LEVEL_H264_31;
		break;
	case MVX_LEVEL_H264_32:
		*mve_level = MVE_OPT_LEVEL_H264_32;
		break;
	case MVX_LEVEL_H264_4:
		*mve_level = MVE_OPT_LEVEL_H264_4;
		break;
	case MVX_LEVEL_H264_41:
		*mve_level = MVE_OPT_LEVEL_H264_41;
		break;
	case MVX_LEVEL_H264_42:
		*mve_level = MVE_OPT_LEVEL_H264_42;
		break;
	case MVX_LEVEL_H264_5:
		*mve_level = MVE_OPT_LEVEL_H264_5;
		break;
	case MVX_LEVEL_H264_51:
		*mve_level = MVE_OPT_LEVEL_H264_51;
		break;

	/**
	 * Levels supported by the HW but not by V4L2 controls API.
	 *
	 * case MVX_LEVEL_H264_52:
	 * mve_level = MVE_OPT_LEVEL_H264_52;
	 *      break;
	 * case MVX_LEVEL_H264_6:
	 * mve_level = MVE_OPT_LEVEL_H264_6;
	 *      break;
	 * case MVX_LEVEL_H264_61:
	 * mve_level = MVE_OPT_LEVEL_H264_61;
	 *      break;
	 * case MVX_LEVEL_H264_62:
	 * mve_level = MVE_OPT_LEVEL_H264_62;
	 *      break;
	 */
	case MVX_LEVEL_H265_MAIN_1:
		*mve_level = MVE_OPT_LEVEL_H265_MAIN_TIER_1;
		break;
	case MVX_LEVEL_H265_HIGH_1:
		*mve_level = MVE_OPT_LEVEL_H265_HIGH_TIER_1;
		break;
	case MVX_LEVEL_H265_MAIN_2:
		*mve_level = MVE_OPT_LEVEL_H265_MAIN_TIER_2;
		break;
	case MVX_LEVEL_H265_HIGH_2:
		*mve_level = MVE_OPT_LEVEL_H265_HIGH_TIER_2;
		break;
	case MVX_LEVEL_H265_MAIN_21:
		*mve_level = MVE_OPT_LEVEL_H265_MAIN_TIER_21;
		break;
	case MVX_LEVEL_H265_HIGH_21:
		*mve_level = MVE_OPT_LEVEL_H265_HIGH_TIER_21;
		break;
	case MVX_LEVEL_H265_MAIN_3:
		*mve_level = MVE_OPT_LEVEL_H265_MAIN_TIER_3;
		break;
	case MVX_LEVEL_H265_HIGH_3:
		*mve_level = MVE_OPT_LEVEL_H265_HIGH_TIER_3;
		break;
	case MVX_LEVEL_H265_MAIN_31:
		*mve_level = MVE_OPT_LEVEL_H265_MAIN_TIER_31;
		break;
	case MVX_LEVEL_H265_HIGH_31:
		*mve_level = MVE_OPT_LEVEL_H265_HIGH_TIER_31;
		break;
	case MVX_LEVEL_H265_MAIN_4:
		*mve_level = MVE_OPT_LEVEL_H265_MAIN_TIER_4;
		break;
	case MVX_LEVEL_H265_HIGH_4:
		*mve_level = MVE_OPT_LEVEL_H265_HIGH_TIER_4;
		break;
	case MVX_LEVEL_H265_MAIN_41:
		*mve_level = MVE_OPT_LEVEL_H265_MAIN_TIER_41;
		break;
	case MVX_LEVEL_H265_HIGH_41:
		*mve_level = MVE_OPT_LEVEL_H265_HIGH_TIER_41;
		break;
	case MVX_LEVEL_H265_MAIN_5:
		*mve_level = MVE_OPT_LEVEL_H265_MAIN_TIER_5;
		break;
	case MVX_LEVEL_H265_HIGH_5:
		*mve_level = MVE_OPT_LEVEL_H265_HIGH_TIER_5;
		break;
	case MVX_LEVEL_H265_MAIN_51:
		*mve_level = MVE_OPT_LEVEL_H265_MAIN_TIER_51;
		break;
	case MVX_LEVEL_H265_HIGH_51:
		*mve_level = MVE_OPT_LEVEL_H265_HIGH_TIER_51;
		break;
	case MVX_LEVEL_H265_MAIN_52:
		*mve_level = MVE_OPT_LEVEL_H265_MAIN_TIER_52;
		break;
	case MVX_LEVEL_H265_HIGH_52:
		*mve_level = MVE_OPT_LEVEL_H265_HIGH_TIER_52;
		break;
	case MVX_LEVEL_H265_MAIN_6:
		*mve_level = MVE_OPT_LEVEL_H265_MAIN_TIER_6;
		break;
	case MVX_LEVEL_H265_HIGH_6:
		*mve_level = MVE_OPT_LEVEL_H265_HIGH_TIER_6;
		break;
	case MVX_LEVEL_H265_MAIN_61:
		*mve_level = MVE_OPT_LEVEL_H265_MAIN_TIER_61;
		break;
	case MVX_LEVEL_H265_HIGH_61:
		*mve_level = MVE_OPT_LEVEL_H265_HIGH_TIER_61;
		break;
	case MVX_LEVEL_H265_MAIN_62:
		*mve_level = MVE_OPT_LEVEL_H265_MAIN_TIER_62;
		break;
	case MVX_LEVEL_H265_HIGH_62:
		*mve_level = MVE_OPT_LEVEL_H265_HIGH_TIER_62;
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static int to_mve_gop_type(enum mvx_gop_type gop,
			   unsigned int *mve_arg)
{
	switch (gop) {
	case MVX_GOP_TYPE_BIDIRECTIONAL:
		*mve_arg = MVE_OPT_GOP_TYPE_BIDIRECTIONAL;
		break;
	case MVX_GOP_TYPE_LOW_DELAY:
		*mve_arg = MVE_OPT_GOP_TYPE_LOW_DELAY;
		break;
	case MVX_GOP_TYPE_PYRAMID:
		*mve_arg = MVE_OPT_GOP_TYPE_PYRAMID;
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static int to_mve_h264_cabac(enum mvx_entropy_mode entropy_mode,
			     unsigned int *mve_arg)
{
	switch (entropy_mode) {
	case MVX_ENTROPY_MODE_CABAC:
		*mve_arg = 1;
		break;
	case MVX_ENTROPY_MODE_CAVLC:
		*mve_arg = 0;
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static int to_mve_vp9_prob_update(enum mvx_vp9_prob_update prob_update,
				  unsigned int *mve_arg)
{
	switch (prob_update) {
	case MVX_VP9_PROB_UPDATE_DISABLED:
		*mve_arg = MVE_OPT_VP9_PROB_UPDATE_DISABLED;
		break;
	case MVX_VP9_PROB_UPDATE_IMPLICIT:
		*mve_arg = MVE_OPT_VP9_PROB_UPDATE_IMPLICIT;
		break;
	case MVX_VP9_PROB_UPDATE_EXPLICIT:
		*mve_arg = MVE_OPT_VP9_PROB_UPDATE_EXPLICIT;
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static int to_mve_rgb_to_yuv(enum mvx_rgb_to_yuv_mode mode,
			     unsigned int *mve_arg)
{
	switch (mode) {
	case MVX_RGB_TO_YUV_MODE_BT601_STUDIO:
		*mve_arg = MVE_OPT_RGB_TO_YUV_BT601_STUDIO;
		break;
	case MVX_RGB_TO_YUV_MODE_BT601_FULL:
		*mve_arg = MVE_OPT_RGB_TO_YUV_BT601_FULL;
		break;
	case MVX_RGB_TO_YUV_MODE_BT709_STUDIO:
		*mve_arg = MVE_OPT_RGB_TO_YUV_BT709_STUDIO;
		break;
	case MVX_RGB_TO_YUV_MODE_BT709_FULL:
		*mve_arg = MVE_OPT_RGB_TO_YUV_BT709_FULL;
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static int put_fw_opt(struct mvx_fw *fw,
		      struct mve_request_set_option *opt,
		      size_t size)
{
	int ret;

	ret = write_message(fw, fw->msg_host, fw->msg_mve,
			    MVE_REQUEST_CODE_SET_OPTION,
			    opt, offsetof(typeof(*opt), data) + size,
			    MVX_LOG_FWIF_CHANNEL_MESSAGE, fw->buf_attr[MVX_FW_REGION_MSG_MVE], fw->buf_attr[MVX_FW_REGION_MSG_HOST]);

	if (ret == 0)
		fw->msg_pending++;

	return ret;
}

static int put_fw_buf_param(struct mvx_fw *fw,
			    struct mve_buffer_param *param,
			    size_t size)
{
	return write_message(fw, fw->buf_in_host, fw->buf_in_mve,
			     MVE_BUFFER_CODE_PARAM,
			     param, offsetof(typeof(*param), data) + size,
			     MVX_LOG_FWIF_CHANNEL_MESSAGE, fw->buf_attr[MVX_FW_REGION_BUF_IN_MVE], fw->buf_attr[MVX_FW_REGION_BUF_IN_HOST]);
}

static int put_message_v2(struct mvx_fw *fw,
			  struct mvx_fw_msg *msg)
{
	int ret = 0;

	switch (msg->code) {
	case MVX_FW_CODE_STATE_CHANGE: {
		unsigned int code = msg->state == MVX_FW_STATE_STOPPED ?
				    MVE_REQUEST_CODE_STOP :
				    MVE_REQUEST_CODE_GO;

		ret = write_message(fw, fw->msg_host, fw->msg_mve,
				    code, NULL, 0,
				    MVX_LOG_FWIF_CHANNEL_MESSAGE, fw->buf_attr[MVX_FW_REGION_MSG_MVE], fw->buf_attr[MVX_FW_REGION_MSG_HOST]);
		if (ret == 0)
			fw->msg_pending++;

		break;
	}
	case MVX_FW_CODE_JOB: {
		struct mve_request_job job;

		job.cores = msg->job.cores;
		job.frames = msg->job.frames;
		job.flags = 0;

		ret = write_message(fw, fw->msg_host, fw->msg_mve,
				    MVE_REQUEST_CODE_JOB, &job, sizeof(job),
				    MVX_LOG_FWIF_CHANNEL_MESSAGE, fw->buf_attr[MVX_FW_REGION_MSG_MVE], fw->buf_attr[MVX_FW_REGION_MSG_HOST]);
		break;
	}
	case MVX_FW_CODE_SWITCH_OUT: {
		ret = write_message(fw, fw->msg_host, fw->msg_mve,
				    MVE_REQUEST_CODE_SWITCH, NULL, 0,
				    MVX_LOG_FWIF_CHANNEL_MESSAGE, fw->buf_attr[MVX_FW_REGION_MSG_MVE], fw->buf_attr[MVX_FW_REGION_MSG_HOST]);
		break;
	}
	case MVX_FW_CODE_PING: {
		ret = write_message(fw, fw->msg_host, fw->msg_mve,
				    MVE_REQUEST_CODE_PING, NULL, 0,
				    MVX_LOG_FWIF_CHANNEL_MESSAGE, fw->buf_attr[MVX_FW_REGION_MSG_MVE], fw->buf_attr[MVX_FW_REGION_MSG_HOST]);
		break;
	}
	case MVX_FW_CODE_SET_OPTION: {
		switch (msg->set_option.code) {
		case MVX_FW_SET_FRAME_RATE: {
			struct mve_buffer_param param;

			param.type = MVE_BUFFER_PARAM_TYPE_FRAME_RATE;
			param.data.arg = msg->set_option.frame_rate;
			ret = put_fw_buf_param(fw, &param,
					       sizeof(param.data.arg));
			break;
		}
		case MVX_FW_SET_TARGET_BITRATE: {
			struct mve_buffer_param param;

			param.type = MVE_BUFFER_PARAM_TYPE_RATE_CONTROL;
			if (msg->set_option.target_bitrate == 0) {
				param.data.rate_control.rate_control_mode =
					MVE_OPT_RATE_CONTROL_MODE_OFF;
				param.data.rate_control.target_bitrate = 0;
			} else {
				param.data.rate_control.rate_control_mode =
					MVE_OPT_RATE_CONTROL_MODE_STANDARD;
				param.data.rate_control.target_bitrate =
					msg->set_option.target_bitrate;
			}

			ret = put_fw_buf_param(fw, &param,
					       sizeof(param.data.rate_control));
			break;
		}
        case MVX_FW_SET_RATE_CONTROL: {
            struct mve_buffer_param param;

            param.type = MVE_BUFFER_PARAM_TYPE_RATE_CONTROL;
            if (msg->set_option.rate_control.target_bitrate == 0) {
                param.data.rate_control.rate_control_mode =
                    MVE_OPT_RATE_CONTROL_MODE_OFF;
                param.data.rate_control.target_bitrate = 0;
            } else {
                param.data.rate_control.rate_control_mode =
                    msg->set_option.rate_control.rate_control_mode;
                param.data.rate_control.target_bitrate =
                    msg->set_option.rate_control.target_bitrate;
                if (msg->set_option.rate_control.rate_control_mode == MVX_OPT_RATE_CONTROL_MODE_C_VARIABLE) {
                param.data.rate_control.maximum_bitrate =
                    msg->set_option.rate_control.maximum_bitrate;
                }
            }
            ret = put_fw_buf_param(fw, &param,
                           sizeof(param.data.rate_control));
            break;

        }
        case MVX_FW_SET_CROP_LEFT: {
            struct mve_request_set_option opt;

            opt.index = MVE_SET_OPT_INDEX_ENC_CROP_RARAM_LEFT;
            opt.data.arg = msg->set_option.crop_left;

            ret = put_fw_opt(fw, &opt, sizeof(opt.data.arg));
            break;
        }
        case MVX_FW_SET_CROP_RIGHT: {
            struct mve_request_set_option opt;

            opt.index = MVE_SET_OPT_INDEX_ENC_CROP_RARAM_RIGHT;
            opt.data.arg = msg->set_option.crop_right;

            ret = put_fw_opt(fw, &opt, sizeof(opt.data.arg));
            break;
        }
        case MVX_FW_SET_CROP_TOP: {
            struct mve_request_set_option opt;

            opt.index = MVE_SET_OPT_INDEX_ENC_CROP_RARAM_TOP;
            opt.data.arg = msg->set_option.crop_top;

            ret = put_fw_opt(fw, &opt, sizeof(opt.data.arg));

            break;
        }
        case MVX_FW_SET_CROP_BOTTOM: {
            struct mve_request_set_option opt;

            opt.index = MVE_SET_OPT_INDEX_ENC_CROP_RARAM_BOTTOM;
            opt.data.arg = msg->set_option.crop_bottom;

            ret = put_fw_opt(fw, &opt, sizeof(opt.data.arg));

            break;
        }
        case MVX_FW_SET_INDEX_PROFILING: {
            struct mve_request_set_option opt;

            opt.index = MVE_SET_OPT_INDEX_PROFILING;
            opt.data.arg = msg->set_option.index_profiling;

            ret = put_fw_opt(fw, &opt, sizeof(opt.data.arg));

            break;
        }
        case MVX_FW_SET_HRD_BUF_SIZE: {
            struct mve_buffer_param param;

            param.type = MVE_BUFFER_PARAM_TYPE_RATE_CONTROL_HRD_BUF_SIZE;
            param.data.arg = msg->set_option.nHRDBufsize;
            ret = put_fw_buf_param(fw, &param,
                           sizeof(param.data.arg));
            break;
        }
        case MVX_FW_SET_COLOUR_DESC: {
            struct mve_buffer_param param;

            param.type = MVE_BUFFER_PARAM_TYPE_COLOUR_DESCRIPTION;
            param.data.colour_description.flags = msg->set_option.colour_desc.flags;
            switch (msg->set_option.colour_desc.range)
            {
                case MVX_FW_RANGE_UNSPECIFIED:
                    param.data.colour_description.range = MVE_BUFFER_PARAM_COLOUR_RANGE_UNSPECIFIED;
                    break;
                case MVX_FW_RANGE_LIMITED:
                    param.data.colour_description.range = MVE_BUFFER_PARAM_COLOUR_RANGE_LIMITED;
                    break;
                case MVX_FW_RANGE_FULL:
                    param.data.colour_description.range = MVE_BUFFER_PARAM_COLOUR_RANGE_FULL;
                    break;
                default:
                    MVX_LOG_PRINT(&mvx_log_if, MVX_LOG_WARNING,
                              "Unknown fw buffer param color desc range. range=%u.",
                              msg->set_option.colour_desc.range);
                    return -EINVAL;
            }
            /* Color primaries according to HEVC E.3.1. */
    		switch (msg->set_option.colour_desc.primaries) {
        		case MVX_FW_PRIMARIES_BT709:
        			param.data.colour_description.colour_primaries = 1;
        			break;
        		case MVX_FW_PRIMARIES_BT470M:
        			param.data.colour_description.colour_primaries = 4;
        			break;
        		case MVX_FW_PRIMARIES_BT601_625:
        			param.data.colour_description.colour_primaries = 5;
        			break;
        		case MVX_FW_PRIMARIES_BT601_525:
        			param.data.colour_description.colour_primaries = 6;
        			break;
        		case MVX_FW_PRIMARIES_GENERIC_FILM:
        			param.data.colour_description.colour_primaries = 8;
        			break;
        		case MVX_FW_PRIMARIES_BT2020:
        			param.data.colour_description.colour_primaries = 9;
        			break;
        		default:
        			param.data.colour_description.colour_primaries = 2;
        			break;
            }
            /* Transfer characteristics according to HEVC E.3.1. */
    		switch (msg->set_option.colour_desc.transfer) {
        		case MVX_FW_TRANSFER_BT1361:
        			param.data.colour_description.transfer_characteristics = 12;
        			break;
        		case MVX_FW_TRANSFER_GAMMA22:
        			param.data.colour_description.transfer_characteristics = 4;
        			break;
        		case MVX_FW_TRANSFER_GAMMA28:
        			param.data.colour_description.transfer_characteristics = 5;
        			break;
        		case MVX_FW_TRANSFER_SMPTE170M:
        			param.data.colour_description.transfer_characteristics = 6;
        			break;
        		case MVX_FW_TRANSFER_SMPTE240M:
        			param.data.colour_description.transfer_characteristics = 7;
        			break;
        		case MVX_FW_TRANSFER_LINEAR:
        			param.data.colour_description.transfer_characteristics = 8;
        			break;
        		case MVX_FW_TRANSFER_HLG:
        			param.data.colour_description.transfer_characteristics = 18;
        			break;
        		case MVX_FW_TRANSFER_XVYCC:
        			param.data.colour_description.transfer_characteristics = 11;
        			break;
        		case MVX_FW_TRANSFER_SRGB:
        			param.data.colour_description.transfer_characteristics = 13;
        			break;
        		case MVX_FW_TRANSFER_ST2084:
        			param.data.colour_description.transfer_characteristics = 16;
        			break;
        		case MVX_FW_TRANSFER_ST428:
        			param.data.colour_description.transfer_characteristics = 17;
        			break;
        		default:
        			param.data.colour_description.transfer_characteristics = 2;
        			break;
    		}
            /* Matrix coefficient according to HEVC E.3.1. */
            switch (msg->set_option.colour_desc.matrix) {
                case MVX_FW_MATRIX_BT709:
                    param.data.colour_description.matrix_coeff = 1;
                    break;
                case MVX_FW_MATRIX_BT470M:
                    param.data.colour_description.matrix_coeff = 4;
                    break;
                case MVX_FW_MATRIX_BT601:
                    param.data.colour_description.matrix_coeff = 6;
                    break;
                case MVX_FW_MATRIX_SMPTE240M:
                    param.data.colour_description.matrix_coeff = 7;
                    break;
                case MVX_FW_MATRIX_BT2020:
                    param.data.colour_description.matrix_coeff = 9;
                    break;
                case MVX_FW_MATRIX_BT2020Constant:
                    param.data.colour_description.matrix_coeff = 10;
                    break;
                default:
                    param.data.colour_description.matrix_coeff = 2;
                    break;
            }
            param.data.colour_description.sar_height = msg->set_option.colour_desc.sar_height;
            param.data.colour_description.sar_width = msg->set_option.colour_desc.sar_width;
            if (msg->set_option.colour_desc.aspect_ratio_idc != 0) {
                param.data.colour_description.aspect_ratio_idc = msg->set_option.colour_desc.aspect_ratio_idc;
                param.data.colour_description.aspect_ratio_info_present_flag = 1;
            }
            if (msg->set_option.colour_desc.video_format != 0) {
                param.data.colour_description.video_format = msg->set_option.colour_desc.video_format;
                param.data.colour_description.video_format_present_flag = 1;
            }
            if (msg->set_option.colour_desc.time_scale != 0 || msg->set_option.colour_desc.num_units_in_tick != 0) {
                param.data.colour_description.time_scale = msg->set_option.colour_desc.time_scale;
                param.data.colour_description.num_units_in_tick = msg->set_option.colour_desc.num_units_in_tick;
                param.data.colour_description.timing_flag_info_present_flag = 1;
            }
            if (msg->set_option.colour_desc.flags & MVX_FW_COLOR_DESC_CONTENT_VALID) {
                param.data.colour_description.avg_content_light_level =
                        msg->set_option.colour_desc.content.luminance_average;
                param.data.colour_description.max_content_light_level =
                        msg->set_option.colour_desc.content.luminance_max;
            }
            if (msg->set_option.colour_desc.flags & MVX_FW_COLOR_DESC_DISPLAY_VALID) {
                param.data.colour_description.mastering_display_primaries_x[0] =
                        msg->set_option.colour_desc.display.r.x;
                param.data.colour_description.mastering_display_primaries_x[1] =
                        msg->set_option.colour_desc.display.g.x;
                param.data.colour_description.mastering_display_primaries_x[2] =
                        msg->set_option.colour_desc.display.b.x;
                param.data.colour_description.mastering_display_primaries_y[0] =
                        msg->set_option.colour_desc.display.r.y;
                param.data.colour_description.mastering_display_primaries_y[1] =
                        msg->set_option.colour_desc.display.g.y;
                param.data.colour_description.mastering_display_primaries_y[2] =
                        msg->set_option.colour_desc.display.b.y;
                param.data.colour_description.mastering_white_point_x =
                        msg->set_option.colour_desc.display.w.x;
                param.data.colour_description.mastering_white_point_y =
                        msg->set_option.colour_desc.display.w.y;
                param.data.colour_description.max_display_mastering_luminance =
                        msg->set_option.colour_desc.display.luminance_min;
                param.data.colour_description.min_display_mastering_luminance =
                        msg->set_option.colour_desc.display.luminance_max;
            }

            ret = put_fw_buf_param(fw, &param,
                       sizeof(param.data.colour_description));
            break;
        }
        case MVX_FW_SET_SEI_USERDATA: {
            struct mve_buffer_param param;
            param.type = MVE_BUFFER_PARAM_TYPE_SEI_USER_DATA_UNREGISTERED;
            param.data.user_data_unregistered.user_data_len = msg->set_option.userdata.user_data_len;
            param.data.user_data_unregistered.flags = msg->set_option.userdata.flags;
            memcpy(&param.data.user_data_unregistered.uuid, &msg->set_option.userdata.uuid,
                    sizeof(param.data.user_data_unregistered.uuid));
            memcpy(&param.data.user_data_unregistered.user_data, &msg->set_option.userdata.user_data,
                    sizeof(param.data.user_data_unregistered.user_data));
            ret = put_fw_buf_param(fw, &param,
                       sizeof(param.data.user_data_unregistered));
            break;
        }
		case MVX_FW_SET_NALU_FORMAT: {
			struct mve_request_set_option opt;

			opt.index = MVE_SET_OPT_INDEX_NALU_FORMAT;
			ret = to_mve_nalu_format(msg->set_option.nalu_format,
						 &opt.data.arg);

			if (ret == 0)
				ret = put_fw_opt(fw, &opt,
						 sizeof(opt.data.arg));

			break;
		}
		case MVX_FW_SET_STREAM_ESCAPING: {
			struct mve_request_set_option opt;

			opt.index = MVE_SET_OPT_INDEX_STREAM_ESCAPING;
			opt.data.arg = msg->set_option.stream_escaping ? 1 : 0;

			ret = put_fw_opt(fw, &opt, sizeof(opt.data.arg));
			break;
		}
		case MVX_FW_SET_PROFILE_LEVEL: {
			struct mve_request_set_option opt;

			opt.index = MVE_SET_OPT_INDEX_PROFILE_LEVEL;
			ret = fw->ops_priv.to_mve_profile(
				msg->set_option.profile_level.profile,
				&opt.data.profile_level.profile);
			if (ret != 0)
				return ret;

			ret = fw->ops_priv.to_mve_level(
				msg->set_option.profile_level.level,
				&opt.data.profile_level.level);
			if (ret != 0)
				return ret;

			ret = put_fw_opt(
				fw, &opt,
				sizeof(opt.data.profile_level));

			break;
		}
		case MVX_FW_SET_IGNORE_STREAM_HEADERS: {
			struct mve_request_set_option opt;

			opt.index = MVE_SET_OPT_INDEX_IGNORE_STREAM_HEADERS;
			opt.data.arg =
				msg->set_option.ignore_stream_headers ? 1 : 0;
			ret = put_fw_opt(fw, &opt, sizeof(opt.data.arg));
			break;
		}
		case MVX_FW_SET_FRAME_REORDERING: {
			struct mve_request_set_option opt;

			opt.index = MVE_SET_OPT_INDEX_FRAME_REORDERING;
			opt.data.arg = msg->set_option.frame_reordering ? 1 : 0;
			ret = put_fw_opt(fw, &opt, sizeof(opt.data.arg));
			break;
		}
		case MVX_FW_SET_INTBUF_SIZE: {
			struct mve_request_set_option opt;

			opt.index = MVE_SET_OPT_INDEX_INTBUF_SIZE;
			opt.data.arg = msg->set_option.intbuf_size;
			ret = put_fw_opt(fw, &opt, sizeof(opt.data.arg));
			break;
		}
		case MVX_FW_SET_P_FRAMES: {
			struct mve_request_set_option opt;

			opt.index = MVE_SET_OPT_INDEX_ENC_P_FRAMES;
			opt.data.arg = msg->set_option.pb_frames;
			ret = put_fw_opt(fw, &opt, sizeof(opt.data.arg));
			break;
		}
		case MVX_FW_SET_B_FRAMES: {
			struct mve_request_set_option opt;

			opt.index = MVE_SET_OPT_INDEX_ENC_B_FRAMES;
			opt.data.arg = msg->set_option.pb_frames;
			ret = put_fw_opt(fw, &opt, sizeof(opt.data.arg));
			break;
		}
		case MVX_FW_SET_GOP_TYPE: {
			struct mve_request_set_option opt;

			opt.index = MVE_SET_OPT_INDEX_GOP_TYPE;
			ret = to_mve_gop_type(msg->set_option.gop_type,
					      &opt.data.arg);
			if (ret == 0)
				ret = put_fw_opt(fw, &opt,
						 sizeof(opt.data.arg));

			break;
		}
		case MVX_FW_SET_INTRA_MB_REFRESH: {
			struct mve_request_set_option opt;

			opt.index = MVE_SET_OPT_INDEX_INTRA_MB_REFRESH;
			opt.data.arg = msg->set_option.intra_mb_refresh;
			ret = put_fw_opt(fw, &opt, sizeof(opt.data.arg));
			break;
		}
		case MVX_FW_SET_CONSTR_IPRED: {
			struct mve_request_set_option opt;

			opt.index = MVE_SET_OPT_INDEX_ENC_CONSTR_IPRED;
			opt.data.arg = msg->set_option.constr_ipred ? 1 : 0;
			ret = put_fw_opt(fw, &opt, sizeof(opt.data.arg));
			break;
		}
		case MVX_FW_SET_ENTROPY_SYNC: {
			struct mve_request_set_option opt;

			opt.index = MVE_SET_OPT_INDEX_ENC_ENTROPY_SYNC;
			opt.data.arg = msg->set_option.entropy_sync ? 1 : 0;
			ret = put_fw_opt(fw, &opt, sizeof(opt.data.arg));
			break;
		}
		case MVX_FW_SET_TEMPORAL_MVP: {
			struct mve_request_set_option opt;

			opt.index = MVE_SET_OPT_INDEX_ENC_TEMPORAL_MVP;
			opt.data.arg = msg->set_option.temporal_mvp ? 1 : 0;
			ret = put_fw_opt(fw, &opt, sizeof(opt.data.arg));
			break;
		}
		case MVX_FW_SET_TILES: {
			struct mve_request_set_option opt;

			opt.index = MVE_SET_OPT_INDEX_TILES;
			opt.data.tiles.tile_rows = msg->set_option.tile.rows;
			opt.data.tiles.tile_cols = msg->set_option.tile.cols;
			ret = put_fw_opt(fw, &opt, sizeof(opt.data.tiles));
			break;
		}
		case MVX_FW_SET_MIN_LUMA_CB_SIZE: {
			struct mve_request_set_option opt;

			opt.index = MVE_SET_OPT_INDEX_ENC_MIN_LUMA_CB_SIZE;
			opt.data.arg = msg->set_option.min_luma_cb_size;
			ret = put_fw_opt(fw, &opt, sizeof(opt.data.arg));
			break;
		}
		case MVX_FW_SET_QP_RANGE: {
			struct mve_buffer_param param;

			param.type =
				MVE_BUFFER_PARAM_TYPE_RATE_CONTROL_QP_RANGE;
			param.data.rate_control_qp_range.qp_min =
				msg->set_option.qp_range.min;
			param.data.rate_control_qp_range.qp_max =
				msg->set_option.qp_range.max;
			ret = put_fw_buf_param(
				fw, &param,
				sizeof(param.data.rate_control_qp_range));
			break;
		}
		case MVX_FW_SET_ENTROPY_MODE: {
			struct mve_request_set_option opt;

			opt.index = MVE_SET_OPT_INDEX_ENC_H264_CABAC;
			ret = to_mve_h264_cabac(msg->set_option.entropy_mode,
						&opt.data.arg);
			if (ret == 0)
				ret = put_fw_opt(fw, &opt,
						 sizeof(opt.data.arg));

			break;
		}
		case MVX_FW_SET_SLICE_SPACING_MB: {
			struct mve_request_set_option opt;

			opt.index = MVE_SET_OPT_INDEX_ENC_SLICE_SPACING;
			opt.data.arg = msg->set_option.slice_spacing_mb;
			ret = put_fw_opt(fw, &opt, sizeof(opt.data.arg));
			break;
		}
		case MVX_FW_SET_VP9_PROB_UPDATE: {
			struct mve_request_set_option opt;

			opt.index = MVE_SET_OPT_INDEX_ENC_VP9_PROB_UPDATE;
			ret = to_mve_vp9_prob_update(
				msg->set_option.vp9_prob_update,
				&opt.data.arg);
			if (ret == 0)
				ret = put_fw_opt(fw, &opt,
						 sizeof(opt.data.arg));

			break;
		}
		case MVX_FW_SET_MV_SEARCH_RANGE: {
			struct mve_request_set_option opt;

			opt.index = MVE_SET_OPT_INDEX_MV_SEARCH_RANGE;
			opt.data.motion_vector_search_range.mv_search_range_x =
				msg->set_option.mv.x;
			opt.data.motion_vector_search_range.mv_search_range_y =
				msg->set_option.mv.y;
			ret = put_fw_opt(
				fw, &opt,
				sizeof(opt.data.motion_vector_search_range));
			break;
		}
		case MVX_FW_SET_BITDEPTH: {
			struct mve_request_set_option opt;

			opt.index = MVE_SET_OPT_INDEX_ENC_STREAM_BITDEPTH;
			opt.data.bitdepth.luma_bitdepth =
				msg->set_option.bitdepth.luma;
			opt.data.bitdepth.chroma_bitdepth =
				msg->set_option.bitdepth.chroma;
			ret = put_fw_opt(fw, &opt, sizeof(opt.data.bitdepth));
			break;
		}
		case MVX_FW_SET_CHROMA_FORMAT: {
			struct mve_request_set_option opt;

			opt.index = MVE_SET_OPT_INDEX_ENC_STREAM_CHROMA_FORMAT;
			opt.data.arg = msg->set_option.chroma_format;
			ret = put_fw_opt(fw, &opt, sizeof(opt.data.arg));
			break;
		}
		case MVX_FW_SET_RGB_TO_YUV_MODE: {
			struct mve_request_set_option opt;

			opt.index = MVE_SET_OPT_INDEX_ENC_RGB_TO_YUV_MODE;
			ret = to_mve_rgb_to_yuv(msg->set_option.rgb_to_yuv_mode,
						&opt.data.arg);
			if (ret == 0)
				ret = put_fw_opt(fw, &opt,
						 sizeof(opt.data.arg));

			break;
		}
		case MVX_FW_SET_BAND_LIMIT: {
			struct mve_request_set_option opt;

			opt.index = MVE_SET_OPT_INDEX_ENC_BANDWIDTH_LIMIT;
			opt.data.arg = msg->set_option.band_limit;
			ret = put_fw_opt(fw, &opt, sizeof(opt.data.arg));
			break;
		}
		case MVX_FW_SET_CABAC_INIT_IDC: {
			struct mve_request_set_option opt;

			opt.index = MVE_SET_OPT_INDEX_ENC_CABAC_INIT_IDC;
			opt.data.arg = msg->set_option.cabac_init_idc;
			ret = put_fw_opt(fw, &opt, sizeof(opt.data.arg));
			break;
		}
		case MVX_FW_SET_QP_I: {
			struct mve_buffer_param param;

			param.type = MVE_BUFFER_PARAM_TYPE_QP_I;
			param.data.qp.qp = msg->set_option.qp;
			ret = put_fw_buf_param(fw, &param,
					       sizeof(param.data.qp));
			break;
		}
		case MVX_FW_SET_QP_P: {
			struct mve_buffer_param param;

			param.type = MVE_BUFFER_PARAM_TYPE_QP_P;
			param.data.qp.qp = msg->set_option.qp;
			ret = put_fw_buf_param(fw, &param,
					       sizeof(param.data.qp));
			break;
		}
		case MVX_FW_SET_QP_B: {
			struct mve_buffer_param param;

			param.type = MVE_BUFFER_PARAM_TYPE_QP_B;
			param.data.qp.qp = msg->set_option.qp;
			ret = put_fw_buf_param(fw, &param,
					       sizeof(param.data.qp));
			break;
		}
		case MVX_FW_SET_GOP_RESET:{
			struct mve_buffer_param param;
			param.type = MVE_BUFFER_PARAM_TYPE_GOP_RESET;
			ret = put_fw_buf_param(fw, &param, sizeof(param.data.arg));
			break;
		}
		case MVX_FW_SET_RESYNC_INTERVAL: {
			struct mve_request_set_option opt;

			opt.index = MVE_SET_OPT_INDEX_RESYNC_INTERVAL;
			opt.data.arg = msg->set_option.resync_interval;
			ret = put_fw_opt(fw, &opt, sizeof(opt.data.arg));
			break;
		}
		case MVX_FW_SET_QUANT_TABLE: {
			struct mve_request_set_option opt;

			opt.index = MVE_SET_OPT_INDEX_QUANT_TABLE;

			opt.data.quant_table.type = MVE_OPT_QUANT_TABLE_LUMA;
			memcpy(opt.data.quant_table.matrix,
			       msg->set_option.quant_tbl.luma,
			       sizeof(opt.data.quant_table.matrix));
			ret = put_fw_opt(fw, &opt,
					 sizeof(opt.data.quant_table));
			if (ret != 0)
				break;

			opt.data.quant_table.type = MVE_OPT_QUANT_TABLE_CHROMA;
			memcpy(opt.data.quant_table.matrix,
			       msg->set_option.quant_tbl.chroma,
			       sizeof(opt.data.quant_table.matrix));
			ret = put_fw_opt(fw, &opt,
					 sizeof(opt.data.quant_table));
			break;
		}
		case MVX_FW_SET_WATCHDOG_TIMEOUT: {
			struct mve_request_set_option opt;

			opt.index = MVE_SET_OPT_INDEX_WATCHDOG_TIMEOUT;
			opt.data.arg = msg->set_option.watchdog_timeout;

			ret = put_fw_opt(fw, &opt, sizeof(opt.data.arg));
			break;
		}
        case MVX_FW_SET_ROI_REGIONS: {
            struct mve_buffer_param param;
            int i = 0;
			param.type = MVE_BUFFER_PARAM_TYPE_REGIONS;
			param.data.regions.n_regions = msg->set_option.roi_config.num_roi;
            for (;i < msg->set_option.roi_config.num_roi; i++) {
                param.data.regions.region[i].mbx_left = msg->set_option.roi_config.roi[i].mbx_left;
                param.data.regions.region[i].mbx_right = msg->set_option.roi_config.roi[i].mbx_right;
                param.data.regions.region[i].mby_top = msg->set_option.roi_config.roi[i].mby_top;
                param.data.regions.region[i].mby_bottom = msg->set_option.roi_config.roi[i].mby_bottom;
                param.data.regions.region[i].qp_delta = msg->set_option.roi_config.roi[i].qp_delta;
            }
            ret = put_fw_buf_param(fw, &param,
                       sizeof(param.data.regions));
            break;
        }
        case MVX_FW_SET_QP_REGION: {
            struct mve_buffer_param param;

			param.type = MVE_BUFFER_PARAM_TYPE_QP;
			param.data.qp.qp = msg->set_option.qp;
			ret = put_fw_buf_param(fw, &param,
					       sizeof(param.data.qp));
            break;
        }
        case MVX_FW_SET_DSL_FRAME: {
            struct mve_request_set_option opt;
            opt.index = MVE_SET_OPT_INDEX_DEC_DOWNSCALE;
            opt.data.downscaled_frame.width = msg->set_option.dsl_frame.width;
            opt.data.downscaled_frame.height = msg->set_option.dsl_frame.height;
            ret = put_fw_opt(fw, &opt, sizeof(opt.index) + sizeof(opt.data.downscaled_frame));
            break;
        }
        case MVX_FW_SET_LONG_TERM_REF: {
            struct mve_request_set_option opt;
            if (msg->set_option.ltr.mode >= 1 && msg->set_option.ltr.mode <= 8) {
                opt.index = MVE_SET_OPT_INDEX_ENC_LTR_MODE;
                opt.data.arg = msg->set_option.ltr.mode;
                ret = put_fw_opt(fw, &opt, sizeof(opt.data.arg));
            }
            if (msg->set_option.ltr.period >= 2 && msg->set_option.ltr.period <= 254) {
                opt.index = MVE_SET_OPT_INDEX_ENC_LTR_PERIOD;
                opt.data.arg = msg->set_option.ltr.period;
                ret = put_fw_opt(fw, &opt, sizeof(opt.data.arg));
            }
            break;
        }
        case MVX_FW_SET_DSL_MODE: {
            struct mve_request_set_option opt;
            opt.index = MVE_SET_OPT_INDEX_DEC_DOWNSCALE_POS_MODE;
            opt.data.dsl_pos.mode =  msg->set_option.dsl_pos_mode;
            ret = put_fw_opt(fw, &opt, sizeof(opt.index) + sizeof(opt.data.dsl_pos));
            break;
        }
		default:
			ret = -EINVAL;
		}

		break;
	}
	case MVX_FW_CODE_FLUSH: {
		switch (msg->flush.dir) {
		case MVX_DIR_INPUT:
			ret = write_message(fw, fw->msg_host, fw->msg_mve,
					    MVE_REQUEST_CODE_INPUT_FLUSH, NULL,
					    0, MVX_LOG_FWIF_CHANNEL_MESSAGE, fw->buf_attr[MVX_FW_REGION_MSG_MVE], fw->buf_attr[MVX_FW_REGION_MSG_HOST]);
			break;
		case MVX_DIR_OUTPUT:
			ret = write_message(fw, fw->msg_host, fw->msg_mve,
					    MVE_REQUEST_CODE_OUTPUT_FLUSH, NULL,
					    0, MVX_LOG_FWIF_CHANNEL_MESSAGE, fw->buf_attr[MVX_FW_REGION_MSG_MVE], fw->buf_attr[MVX_FW_REGION_MSG_HOST]);
			break;
		default:
			MVX_LOG_PRINT(&mvx_log_if, MVX_LOG_WARNING,
				      "Invalid flush direction. dir=%d.",
				      msg->flush.dir);
			return -EINVAL;
		}

		if (ret == 0)
			fw->msg_pending++;

		break;
	}
	case MVX_FW_CODE_BUFFER: {
		struct mve_comm_area_host *host;
		struct mve_comm_area_mve *mve;
		enum mvx_log_fwif_channel channel;
		enum mvx_fw_buffer_attr mve_buf_attr;
		enum mvx_fw_buffer_attr host_buf_attr;

		if (msg->buf->dir == MVX_DIR_INPUT) {
			host = fw->buf_in_host;
			mve = fw->buf_in_mve;
			mve_buf_attr = fw->buf_attr[MVX_FW_REGION_BUF_IN_MVE];
			host_buf_attr = fw->buf_attr[MVX_FW_REGION_BUF_IN_HOST];
			channel = MVX_LOG_FWIF_CHANNEL_INPUT_BUFFER;
		} else {
			host = fw->buf_out_host;
			mve = fw->buf_out_mve;
			mve_buf_attr = fw->buf_attr[MVX_FW_REGION_BUF_OUT_MVE];
			host_buf_attr = fw->buf_attr[MVX_FW_REGION_BUF_OUT_HOST];
			channel = MVX_LOG_FWIF_CHANNEL_OUTPUT_BUFFER;
		}

		if (mvx_is_frame(msg->buf->format))
            if ((msg->buf->flags & MVX_BUFFER_FRAME_FLAG_GENERAL) == MVX_BUFFER_FRAME_FLAG_GENERAL) {
                ret = put_buffer_general(fw, host, mve, msg, channel, mve_buf_attr, host_buf_attr);
            } else {
                ret = put_buffer_frame(fw, host, mve, msg, channel, mve_buf_attr, host_buf_attr);
            }
		else
			ret = put_buffer_bitstream(fw, host, mve, msg, channel, mve_buf_attr, host_buf_attr);

		break;
	}
	case MVX_FW_CODE_IDLE_ACK: {
		if (fw->ops_priv.send_idle_ack != NULL)
			ret = fw->ops_priv.send_idle_ack(fw);

		break;
	}
	case MVX_FW_CODE_EOS: {
		struct mve_comm_area_host *host;
		struct mve_comm_area_mve *mve;
		enum mvx_log_fwif_channel channel;

		/* The message is on the MVX_DIR_INPUT side. */
		host = fw->buf_in_host;
		mve = fw->buf_in_mve;
		channel = MVX_LOG_FWIF_CHANNEL_INPUT_BUFFER;

		if (msg->eos_is_frame != false) {
			struct mve_buffer_frame f = {
				.host_handle = MVX_FW_CODE_EOS,
				.frame_flags = MVE_BUFFER_FRAME_FLAG_EOS,
				.format      = MVE_FORMAT_YUV420_NV12
			};

			ret = write_message(fw, host, mve,
					    MVE_BUFFER_CODE_FRAME,
					    &f, sizeof(f), channel, fw->buf_attr[MVX_FW_REGION_BUF_IN_MVE], fw->buf_attr[MVX_FW_REGION_BUF_IN_HOST]);
		} else {
			struct mve_buffer_bitstream b = {
				.host_handle        = MVX_FW_CODE_EOS,
				.bitstream_buf_addr =
					MVE_MEM_REGION_PROTECTED_ADDR_BEGIN,
				.bitstream_flags    =
					MVE_BUFFER_BITSTREAM_FLAG_EOS
			};

			ret = write_message(fw, host, mve,
					    MVE_BUFFER_CODE_BITSTREAM, &b,
					    sizeof(b), channel, fw->buf_attr[MVX_FW_REGION_BUF_IN_MVE], fw->buf_attr[MVX_FW_REGION_BUF_IN_HOST]);
		}

		break;
	}
	case MVX_FW_CODE_DUMP: {
		ret = write_message(fw, fw->msg_host, fw->msg_mve,
				    MVE_REQUEST_CODE_DUMP, NULL,
				    0, MVX_LOG_FWIF_CHANNEL_MESSAGE, fw->buf_attr[MVX_FW_REGION_MSG_MVE], fw->buf_attr[MVX_FW_REGION_MSG_HOST]);
		fw->msg_pending++;
		break;
	}
	case MVX_FW_CODE_DEBUG: {
		ret = write_message(fw, fw->msg_host, fw->msg_mve,
				    MVE_REQUEST_CODE_DEBUG, &msg->arg,
				    sizeof(msg->arg), MVX_LOG_FWIF_CHANNEL_MESSAGE, fw->buf_attr[MVX_FW_REGION_MSG_MVE], fw->buf_attr[MVX_FW_REGION_MSG_HOST]);
		fw->msg_pending++;
		break;
	}
	default: {
		ret = -EINVAL;
		break;
	}
	}

	if (ret != 0)
		MVX_LOG_PRINT(&mvx_log_if, MVX_LOG_WARNING,
			      "Firmware put message failed. ret=%d.", ret);

	return ret;
}

/**
 * find_pages() - Find a page allocate in the map.
 * @fw:		Pointer to firmware object.
 * @va:		MVE virtual address.
 *
 * Return: Pointer to pages, NULL if not found.
 */
static struct mvx_mmu_pages *find_pages(struct mvx_fw *fw,
					mvx_mmu_va va)
{
	struct mvx_mmu_pages *pages;

	hash_for_each_possible(fw->rpc_mem, pages, node, va) {
		if (pages->va == va)
			return pages;
	}

	return NULL;
}

static void rpc_mem_alloc(struct mvx_fw *fw,
			  struct mve_rpc_communication_area *rpc_area)
{
	union mve_rpc_params *p = &rpc_area->params;
	enum mvx_fw_region region;
	struct mvx_mmu_pages *pages;
	size_t npages;
	size_t max_pages;
	mvx_mmu_va va = 0;
	mvx_mmu_va va0,va_next;
	mvx_mmu_va end;
	int ret;
	uint8_t log2_alignment;
	uint32_t alignment_pages;
	uint32_t alignment_bytes;
	uint32_t total_used_pages = 0;

	if (IS_ENABLED(CONFIG_DEBUG_FS)) {
		ret = mutex_lock_interruptible(&fw->rpcmem_mutex);
		if (ret != 0) {
			MVX_LOG_PRINT(&mvx_log_if, MVX_LOG_ERROR,
				      "Cannot protect RPC alloc list.");
			goto out;
		}
	}

	switch (p->mem_alloc.region) {
	case MVE_MEM_REGION_PROTECTED:
		region = MVX_FW_REGION_PROTECTED;
		total_used_pages = fw->latest_used_region_protected_pages;
		break;
	case MVE_MEM_REGION_OUTBUF:
		region = MVX_FW_REGION_FRAMEBUF;
		total_used_pages = fw->latest_used_region_outbuf_pages;
		break;
	default:
		MVX_LOG_PRINT(&mvx_log_if, MVX_LOG_WARNING,
			      "Unsupported RPC mem alloc region. region=%u.",
			      p->mem_alloc.region);
		goto unlock_mutex;
	}

	ret = fw->ops.get_region(region, &va, &end);
	if (ret != 0)
		goto unlock_mutex;

	va0 = va;

	npages = DIV_ROUND_UP(p->mem_alloc.size, MVE_PAGE_SIZE);
	max_pages = DIV_ROUND_UP(p->mem_alloc.max_size, MVE_PAGE_SIZE);

	if (fw->fw_bin->securevideo != false) {
		struct dma_buf *dmabuf;

		dmabuf = mvx_secure_mem_alloc(fw->fw_bin->secure.secure,
					      p->mem_alloc.size);
		if (IS_ERR(dmabuf))
			goto unlock_mutex;

		pages = mvx_mmu_alloc_pages_dma_buf(fw->dev, dmabuf, max_pages);
		if (IS_ERR(pages)) {
			dma_buf_put(dmabuf);
			goto unlock_mutex;
		}
	} else {
		pages = mvx_mmu_alloc_pages(fw->dev, npages, max_pages);
		if (IS_ERR(pages))
			goto unlock_mutex;
	}

	va += (total_used_pages << MVE_PAGE_SHIFT);
	log2_alignment = p->mem_alloc.log2_alignment <= MVE_PAGE_SHIFT ? MVE_PAGE_SHIFT : p->mem_alloc.log2_alignment;
	alignment_bytes = 1 << log2_alignment;
	alignment_pages = alignment_bytes >> MVE_PAGE_SHIFT;
	ret = -EINVAL;
	while (va < end) {
		va = (va + alignment_bytes - 1) & ~(alignment_bytes - 1);
		ret = mvx_mmu_map_pages(fw->mmu, va, pages, MVX_ATTR_SHARED_RW,
					MVX_ACCESS_READ_WRITE);
		if (ret == 0){
			va_next = va + MVE_PAGE_SIZE * pages->capacity;
			total_used_pages = (va_next - va0) >> MVE_PAGE_SHIFT;
			break;
		}

		//va += 16 * 1024 * 1024; /* 16MB */
		va += alignment_bytes;
	}

	if (ret != 0) {
		MVX_LOG_PRINT(&mvx_log_if, MVX_LOG_WARNING,
			      "Failed to find memory region for RPC alloc.");
		mvx_mmu_free_pages(pages);
		va = 0;
		goto unlock_mutex;
	}

	switch (p->mem_alloc.region) {
	case MVE_MEM_REGION_PROTECTED:
		fw->latest_used_region_protected_pages = total_used_pages;
		break;
	case MVE_MEM_REGION_OUTBUF:
		fw->latest_used_region_outbuf_pages = total_used_pages;
		break;
	default:
		break;
	}

	hash_add(fw->rpc_mem, &pages->node, pages->va);

	MVX_LOG_PRINT(&mvx_log_if, MVX_LOG_INFO,
		      "RPC alloc memory. size=%u, max_size=%u, region=%u, npages=%zu, va=0x%x.",
		      p->mem_alloc.size, p->mem_alloc.max_size,
		      p->mem_alloc.region, npages, va);

unlock_mutex:
	if (IS_ENABLED(CONFIG_DEBUG_FS))
		mutex_unlock(&fw->rpcmem_mutex);

out:
	rpc_area->size = sizeof(uint32_t);
	p->data[0] = va;
}

static void rpc_mem_resize(struct mvx_fw *fw,
			   struct mve_rpc_communication_area *rpc_area)
{
	union mve_rpc_params *p = &rpc_area->params;
	struct mvx_mmu_pages *pages;
	mvx_mmu_va va = 0;
	int ret;

	if (IS_ENABLED(CONFIG_DEBUG_FS)) {
		ret = mutex_lock_interruptible(&fw->rpcmem_mutex);
		if (ret != 0) {
			MVX_LOG_PRINT(&mvx_log_if, MVX_LOG_ERROR,
				      "Cannot protect RPC alloc list.");
			goto out;
		}
	}

	pages = find_pages(fw, p->mem_resize.ve_pointer);
	if (pages != 0) {
		size_t size;
		size_t npages;
		int ret;

		if (fw->fw_bin->securevideo != false) {
			size = mvx_mmu_size_pages(pages);

			/* The size of RPC memory is only increased. */
			if (size < p->mem_resize.new_size) {
				struct dma_buf *dmabuf;

				size = p->mem_resize.new_size - size;

				/* Allocate a new secure DMA buffer. */
				dmabuf = mvx_secure_mem_alloc(
					fw->fw_bin->secure.secure, size);
				if (IS_ERR(dmabuf))
					goto unlock_mutex;

				ret = mvx_mmu_pages_append_dma_buf(
					pages, dmabuf);
				if (ret != 0) {
					dma_buf_put(dmabuf);
					goto unlock_mutex;
				}
			}
		} else {
			/* Resize the allocated pages. */
			npages = DIV_ROUND_UP(p->mem_resize.new_size,
					      MVE_PAGE_SIZE);
			ret = mvx_mmu_resize_pages(pages, npages);
			if (ret != 0) {
				MVX_LOG_PRINT(&mvx_log_if, MVX_LOG_WARNING,
					      "Failed to resize RPC mapped pages. ret=%d.",
					      ret);
				goto unlock_mutex;
			}
		}

		va = pages->va;
	} else {
		MVX_LOG_PRINT(&mvx_log_if, MVX_LOG_WARNING,
			      "Could not find pages for RPC resize. va=0x%x.",
			      p->mem_resize.ve_pointer);
	}

	fw->client_ops->flush_mmu(fw->csession);

	MVX_LOG_PRINT(&mvx_log_if, MVX_LOG_INFO,
		      "RPC resize memory. va=0x%x, new_size=%u.",
		      p->mem_resize.ve_pointer, p->mem_resize.new_size);

unlock_mutex:
	if (IS_ENABLED(CONFIG_DEBUG_FS))
		mutex_unlock(&fw->rpcmem_mutex);

out:
	rpc_area->size = sizeof(uint32_t);
	p->data[0] = va;
}

static void rpc_mem_free(struct mvx_fw *fw,
			 struct mve_rpc_communication_area *rpc_area)
{
	union mve_rpc_params *p = &rpc_area->params;
	struct mvx_mmu_pages *pages;
	int ret;

	if (IS_ENABLED(CONFIG_DEBUG_FS)) {
		ret = mutex_lock_interruptible(&fw->rpcmem_mutex);
		if (ret != 0) {
			MVX_LOG_PRINT(&mvx_log_if, MVX_LOG_ERROR,
				      "Cannot protect RPC alloc list.");
			return;
		}
	}

	pages = find_pages(fw, p->mem_free.ve_pointer);
	if (pages != NULL) {
		hash_del(&pages->node);
		if(MVE_MEM_REGION_PROTECTED_ADDR_BEGIN <= p->mem_free.ve_pointer && p->mem_free.ve_pointer < MVE_MEM_REGION_PROTECTED_ADDR_END){
			fw->latest_used_region_protected_pages = fw->latest_used_region_protected_pages > pages->capacity ? fw->latest_used_region_protected_pages - pages->capacity : 0;
		} else if(MVE_MEM_REGION_FRAMEBUF_ADDR_BEGIN <= p->mem_free.ve_pointer && p->mem_free.ve_pointer < MVE_MEM_REGION_FRAMEBUF_ADDR_END){
			fw->latest_used_region_outbuf_pages = fw->latest_used_region_outbuf_pages > pages->capacity ? fw->latest_used_region_outbuf_pages - pages->capacity : 0;
		}
		mvx_mmu_free_pages(pages);
	} else {
		MVX_LOG_PRINT(&mvx_log_if, MVX_LOG_WARNING,
			      "Could not find pages for RPC free. va=0x%x.",
			      p->mem_free.ve_pointer);
	}

	fw->client_ops->flush_mmu(fw->csession);

	MVX_LOG_PRINT(&mvx_log_if, MVX_LOG_INFO,
		      "RPC free memory. va=0x%x.", p->mem_free.ve_pointer);

	rpc_area->size = 0;
	if (IS_ENABLED(CONFIG_DEBUG_FS))
		mutex_unlock(&fw->rpcmem_mutex);
}

/**
 * rstrip() - Remove trailing chars from string.
 * @s:		String to be stripped.
 * @t:		String containing chars to be stripped.
 *
 * Return: Pointer to stripped string.
 */
static char *rstrip(char *str,
		    char *trim)
{
	size_t l = strlen(str);

	while (l-- > 0) {
		char *t;

		for (t = trim; *t != '\0'; t++)
			if (str[l] == *t) {
				str[l] = '\0';
				break;
			}

		if (*t == '\0')
			break;
	}

	return str;
}

static int handle_rpc_v2(struct mvx_fw *fw)
{
	struct mve_rpc_communication_area *rpc_area = fw->rpc;
	int ret = 0;

	if (fw->buf_attr[MVX_FW_REGION_RPC] == MVX_FW_BUF_CACHEABLE) {
		dma_sync_single_for_cpu(fw->dev,
				phys_cpu2vpu((virt_to_phys(rpc_area))), sizeof(*rpc_area),
				DMA_FROM_DEVICE);
	}

	if (rpc_area->state == MVE_RPC_STATE_PARAM) {
		ret = 1;

		/* Log RPC request. */
		MVX_LOG_EXECUTE(&mvx_log_fwif_if, MVX_LOG_INFO,
				log_rpc(fw->session,
					MVX_LOG_FWIF_DIRECTION_FIRMWARE_TO_HOST,
					rpc_area));

		switch (rpc_area->call_id) {
		case MVE_RPC_FUNCTION_DEBUG_PRINTF: {
			MVX_LOG_PRINT(
				&mvx_log_if, MVX_LOG_INFO,
				"RPC_PRINT=%s",
				rstrip(rpc_area->params.debug_print.string,
				       "\n\r"));
			break;
		}
		case MVE_RPC_FUNCTION_MEM_ALLOC: {
			rpc_mem_alloc(fw, rpc_area);
			break;
		}
		case MVE_RPC_FUNCTION_MEM_RESIZE: {
			rpc_mem_resize(fw, rpc_area);
			break;
		}
		case MVE_RPC_FUNCTION_MEM_FREE: {
			rpc_mem_free(fw, rpc_area);
			break;
		}
		default:
			MVX_LOG_PRINT(&mvx_log_if, MVX_LOG_INFO,
				      "Unsupported RPC request. call_id=%u.",
				      rpc_area->call_id);
			ret = -EINVAL;
			break;
		}

		/*
		 * Make sure the whole RPC message body has been written before
		 * the RPC message area is returned to the firmware.
		 */
		wmb();
		rpc_area->state = MVE_RPC_STATE_RETURN;

		/* Make sure state is written before memory is flushed. */
		if (fw->buf_attr[MVX_FW_REGION_RPC] == MVX_FW_BUF_CACHEABLE) {
			wmb();
			dma_sync_single_for_device(
				fw->dev,
				phys_cpu2vpu(virt_to_phys(rpc_area)), sizeof(*rpc_area),
				DMA_TO_DEVICE);
		}

		/* Log RPC response. */
		MVX_LOG_EXECUTE(&mvx_log_fwif_if, MVX_LOG_INFO,
				log_rpc(fw->session,
					MVX_LOG_FWIF_DIRECTION_HOST_TO_FIRMWARE,
					rpc_area));

		fw->client_ops->send_irq(fw->csession);
	}

	return ret;
}

#ifdef MVX_FW_DEBUG_ENABLE
#define RAM_PRINTBUF_SIZE MVE_FW_PRINT_RAM_SIZE
#define RAM_PRINT_MAX_LEN (128)
#define RAM_PRINT_BUF_CNT ((RAM_PRINTBUF_SIZE / RAM_PRINT_MAX_LEN) - 1)
#define RAM_PRINT_FLAG (0x11223356)
static int handle_fw_ram_print_v2(struct mvx_fw *fw)
{
	struct mve_fw_ram_print_head_aera *rpt_area = fw->fw_print_ram;
	int ret = 0;
	uint32_t wr_cnt;
	uint32_t rd_cnt = 0;
	uint32_t cnt;
	uint32_t rd_idx;
	char *print_buf = NULL;

	dma_sync_single_for_cpu(fw->dev,
				phys_cpu2vpu(virt_to_phys(rpt_area)), sizeof(*rpt_area),
				DMA_FROM_DEVICE);

	wr_cnt = rpt_area->wr_cnt;
	rd_cnt = rpt_area->rd_cnt;
	cnt = (rd_cnt <= wr_cnt) ? wr_cnt - rd_cnt : wr_cnt - rd_cnt + (uint32_t)~0u;

	if(RAM_PRINT_FLAG == rpt_area->flag && RAM_PRINT_BUF_CNT > rpt_area->index && cnt){
		//printk("RPT:flag=%x, idx=%u, wr_cnt=%u, rd_cnt=%u.\n", rpt_area->flag, rpt_area->index, wr_cnt, rd_cnt);

		while(cnt--){
			rd_idx = rd_cnt % RAM_PRINT_BUF_CNT;
			print_buf = (fw->fw_print_ram + RAM_PRINT_MAX_LEN ) + rd_idx * RAM_PRINT_MAX_LEN;
			MVX_LOG_PRINT(&mvx_log_if, MVX_LOG_WARNING, "FW-%u: %s\n", rd_cnt, print_buf);
			rd_cnt++;
		}

		rpt_area->rd_cnt = rd_cnt;
		/* Make sure rpt_area->rd_cnt is written before memory is flushed. */
		wmb();
		dma_sync_single_for_device(
			fw->dev,
			phys_cpu2vpu(virt_to_phys(&rpt_area->rd_cnt)), sizeof(rpt_area->rd_cnt),
			DMA_TO_DEVICE);

		ret = 1;
	}

	return ret;
}
#endif

static void unmap_msq(struct mvx_fw *fw,
		      void **data,
		      enum mvx_fw_region region)
{
	int ret;
	mvx_mmu_va begin;
	mvx_mmu_va end;

	if (*data == NULL)
		return;

	ret = fw->ops.get_region(region, &begin, &end);
	if (ret == 0)
		mvx_mmu_unmap_va(fw->mmu, begin, MVE_PAGE_SIZE);

	mvx_mmu_free_page(fw->dev, phys_cpu2vpu(virt_to_phys(*data)));

	*data = NULL;
}

static int map_msq(struct mvx_fw *fw,
		   void **data,
		   enum mvx_fw_region region)
{
	phys_addr_t page;
	mvx_mmu_va begin;
	mvx_mmu_va end;
	int ret;

	/* Get virtual address where the message queue is to be mapped. */
	ret = fw->ops.get_region(region, &begin, &end);
	if (ret != 0)
		return ret;

	/* Allocate page and store Linux logical address in 'data'. */
	page = mvx_mmu_alloc_page(fw->dev);
	if (page == 0)
		return -ENOMEM;

	/* Memory map region. */
	ret = mvx_mmu_map_pa(fw->mmu, begin, page, MVE_PAGE_SIZE,
			     MVX_ATTR_SHARED_RW, MVX_ACCESS_READ_WRITE);
	if (ret != 0) {
		mvx_mmu_free_page(fw->dev, page);
		return ret;
	}

	*data = phys_to_virt(phys_vpu2cpu(page));

	return 0;
}

static int map_msq_uncache(struct mvx_fw *fw,
		void **data,
		enum mvx_fw_region region)
{
	phys_addr_t page;
	mvx_mmu_va begin;
	mvx_mmu_va end;
	void* vir_addr = NULL;
	int ret;

	/* Get virtual address where the message queue is to be mapped. */
	ret = fw->ops.get_region(region, &begin, &end);
	if (ret != 0)
		return ret;

	/* Allocate page and store Linux logical address in 'data'. */
	page = mvx_mmu_dma_alloc_coherent(fw->dev, &vir_addr);
	if (page == 0) {
		return -ENOMEM;
	}

	/* Memory map region. */
	ret = mvx_mmu_map_pa(fw->mmu, begin, page, MVE_PAGE_SIZE,
			MVX_ATTR_SHARED_RW, MVX_ACCESS_READ_WRITE);
	if (ret != 0) {
		mvx_mmu_dma_free_coherent(fw->dev, page, vir_addr);
		return ret;
	}

	*data = vir_addr;
	fw->buf_pa_addr[region] = page;

	return 0;
}

static void unmap_msq_uncache(struct mvx_fw *fw,
		void **data,
		enum mvx_fw_region region)
{
	int ret;
	mvx_mmu_va begin;
	mvx_mmu_va end;

	if (*data == NULL)
		return;

	ret = fw->ops.get_region(region, &begin, &end);
	if (ret == 0) {
		mvx_mmu_unmap_va(fw->mmu, begin, MVE_PAGE_SIZE);
	}

	mvx_mmu_dma_free_coherent(fw->dev, fw->buf_pa_addr[region], *data);

	*data = NULL;
	fw->buf_pa_addr[region] = 0;
}

#ifdef MVX_FW_DEBUG_ENABLE
static void unmap_fw_print_ram(struct mvx_fw *fw,
		      void **data,
		      enum mvx_fw_region region)
{
	int ret;
	mvx_mmu_va begin;
	mvx_mmu_va end;

	if (*data == NULL)
		return;

	ret = fw->ops.get_region(region, &begin, &end);
	if (ret == 0)
		mvx_mmu_unmap_va(fw->mmu, begin, MVE_FW_PRINT_RAM_SIZE);

	mvx_mmu_free_contiguous_pages(fw->dev, phys_cpu2vpu(virt_to_phys(*data)), MVE_FW_PRINT_RAM_SIZE >> PAGE_SHIFT);

	*data = NULL;
}

static int map_fw_print_ram(struct mvx_fw *fw,
		   void **data,
		   enum mvx_fw_region region)
{
	phys_addr_t page;
	mvx_mmu_va begin;
	mvx_mmu_va end;
	int ret;

	/* Get virtual address where the message queue is to be mapped. */
	ret = fw->ops.get_region(region, &begin, &end);
	if (ret != 0)
		return ret;

	/* Allocate pages and store Linux logical address in 'data'. */
	page = mvx_mmu_alloc_contiguous_pages(fw->dev, MVE_FW_PRINT_RAM_SIZE >> PAGE_SHIFT);
	if (page == 0)
		return -ENOMEM;

	/* Memory map region. */
	ret = mvx_mmu_map_pa(fw->mmu, begin, page, MVE_FW_PRINT_RAM_SIZE,
			     MVX_ATTR_SHARED_RW, MVX_ACCESS_READ_WRITE);
	if (ret != 0) {
		mvx_mmu_free_contiguous_pages(fw->dev, page, MVE_FW_PRINT_RAM_SIZE >> PAGE_SHIFT);
		return ret;
	}

	*data = phys_to_virt(phys_vpu2cpu(page));

	return 0;
}
#endif

static void unmap_protocol_v2(struct mvx_fw *fw)
{
	struct mvx_mmu_pages *pages;
	struct hlist_node *tmp;
	int bkt;

	fw->unmap_op[fw->buf_attr[MVX_FW_REGION_MSG_HOST]](fw, &fw->msg_host, MVX_FW_REGION_MSG_HOST);
	fw->unmap_op[fw->buf_attr[MVX_FW_REGION_MSG_MVE]](fw, &fw->msg_mve, MVX_FW_REGION_MSG_MVE);
	fw->unmap_op[fw->buf_attr[MVX_FW_REGION_BUF_IN_HOST]](fw, &fw->buf_in_host, MVX_FW_REGION_BUF_IN_HOST);
	fw->unmap_op[fw->buf_attr[MVX_FW_REGION_BUF_IN_MVE]](fw, &fw->buf_in_mve, MVX_FW_REGION_BUF_IN_MVE);
	fw->unmap_op[fw->buf_attr[MVX_FW_REGION_BUF_OUT_HOST]](fw, &fw->buf_out_host, MVX_FW_REGION_BUF_OUT_HOST);
	fw->unmap_op[fw->buf_attr[MVX_FW_REGION_BUF_OUT_MVE]](fw, &fw->buf_out_mve, MVX_FW_REGION_BUF_OUT_MVE);
	fw->unmap_op[fw->buf_attr[MVX_FW_REGION_RPC]](fw, &fw->rpc, MVX_FW_REGION_RPC);

#ifdef MVX_FW_DEBUG_ENABLE
	unmap_fw_print_ram(fw, &fw->fw_print_ram, MVX_FW_REGION_PRINT_RAM);
#endif
	fw->latest_used_region_protected_pages = 0;
	fw->latest_used_region_outbuf_pages = 0;

	hash_for_each_safe(fw->rpc_mem, bkt, tmp, pages, node) {
		hash_del(&pages->node);
		mvx_mmu_free_pages(pages);
	}
}

static int map_protocol_v2(struct mvx_fw *fw)
{
	int ret;

	ret = fw->map_op[fw->buf_attr[MVX_FW_REGION_MSG_HOST]](fw, &fw->msg_host, MVX_FW_REGION_MSG_HOST);
	if (ret != 0)
		goto unmap_fw;

	ret = fw->map_op[fw->buf_attr[MVX_FW_REGION_MSG_MVE]](fw, &fw->msg_mve, MVX_FW_REGION_MSG_MVE);
	if (ret != 0)
		goto unmap_fw;

	ret = fw->map_op[fw->buf_attr[MVX_FW_REGION_BUF_IN_HOST]](fw, &fw->buf_in_host, MVX_FW_REGION_BUF_IN_HOST);
	if (ret != 0)
		goto unmap_fw;

	ret = fw->map_op[fw->buf_attr[MVX_FW_REGION_BUF_IN_MVE]](fw, &fw->buf_in_mve, MVX_FW_REGION_BUF_IN_MVE);
	if (ret != 0)
		goto unmap_fw;

	ret = fw->map_op[fw->buf_attr[MVX_FW_REGION_BUF_OUT_HOST]](fw, &fw->buf_out_host, MVX_FW_REGION_BUF_OUT_HOST);
	if (ret != 0)
		goto unmap_fw;

	ret = fw->map_op[fw->buf_attr[MVX_FW_REGION_BUF_OUT_MVE]](fw, &fw->buf_out_mve, MVX_FW_REGION_BUF_OUT_MVE);
	if (ret != 0)
		goto unmap_fw;

	ret = fw->map_op[fw->buf_attr[MVX_FW_REGION_RPC]](fw, &fw->rpc, MVX_FW_REGION_RPC);
	if (ret != 0)
		goto unmap_fw;

#ifdef MVX_FW_DEBUG_ENABLE
	ret = map_fw_print_ram(fw, &fw->fw_print_ram, MVX_FW_REGION_PRINT_RAM);
	if (ret != 0)
		goto unmap_fw;
#endif

	return 0;

unmap_fw:
	unmap_protocol_v2(fw);

	return ret;
}

static void print_pair(char *name_in,
		       char *name_out,
		       struct device *device,
		       struct mve_comm_area_host *host,
		       struct mve_comm_area_mve *mve,
		       int ind,
		       struct seq_file *s,
		       enum mvx_fw_buffer_attr mve_buf_attr)
{
	if (mve_buf_attr == MVX_FW_BUF_CACHEABLE) {
		dma_sync_single_for_cpu(device, phys_cpu2vpu(virt_to_phys(mve)),
				MVE_PAGE_SIZE, DMA_FROM_DEVICE);
	}
	mvx_seq_printf(s, name_in, ind, "wr=%10d, rd=%10d, avail=%10d\n",
		       host->in_wpos, mve->in_rpos,
		       (uint16_t)(host->in_wpos - mve->in_rpos));
	mvx_seq_printf(s, name_out, ind, "wr=%10d, rd=%10d, avail=%10d\n",
		       mve->out_wpos, host->out_rpos,
		       (uint16_t)(mve->out_wpos - host->out_rpos));
}

static int print_stat_v2(struct mvx_fw *fw,
			 int ind,
			 struct seq_file *s)
{
	print_pair("Msg host->mve", "Msg host<-mve",
		   fw->dev, fw->msg_host, fw->msg_mve,
		   ind, s, fw->buf_attr[MVX_FW_REGION_MSG_MVE]);
	print_pair("Inbuf host->mve", "Inbuf host<-mve",
		   fw->dev, fw->buf_in_host, fw->buf_in_mve,
		   ind, s, fw->buf_attr[MVX_FW_REGION_BUF_IN_MVE]);
	print_pair("Outbuf host->mve", "Outbuf host<-mve",
		   fw->dev, fw->buf_out_host, fw->buf_out_mve,
		   ind, s, fw->buf_attr[MVX_FW_REGION_BUF_OUT_MVE]);

	return 0;
}

static ssize_t get_capacity(int rpos,
			    int wpos)
{
	ssize_t capacity;

	capacity = wpos - rpos;
	if (capacity < 0)
		capacity += MVE_COMM_QUEUE_SIZE_IN_WORDS;

	return capacity * sizeof(uint32_t);
}

static void print_debug_v2(struct mvx_fw *fw)
{
	struct mve_comm_area_host *msg_host = fw->msg_host;
	struct mve_comm_area_mve *msg_mve = fw->msg_mve;
	unsigned int rpos, wpos;
	ssize_t capacity;
	struct mve_msg_header header;
	struct mve_rpc_communication_area *rpc_area = fw->rpc;
	union mve_rpc_params *p = &rpc_area->params;

	if (fw->buf_attr[MVX_FW_REGION_MSG_MVE] == MVX_FW_BUF_CACHEABLE) {
		dma_sync_single_for_cpu(fw->dev, phys_cpu2vpu(virt_to_phys(msg_mve)),
				MVE_PAGE_SIZE, DMA_FROM_DEVICE);
	}

	MVX_LOG_PRINT_SESSION(&mvx_log_session_if, MVX_LOG_WARNING, fw->session,
			      "Dump message queue. msg={host={out_rpos=%u, in_wpos=%u}, mve={out_wpos=%u, in_rpos=%u}}",
			      msg_host->out_rpos, msg_host->in_wpos,
			      msg_mve->out_wpos, msg_mve->in_rpos);

	MVX_LOG_PRINT_SESSION(&mvx_log_session_if, MVX_LOG_WARNING, fw->session,
			      "Dump fw rpc. state=%u, call_id=%u, size=%u, data=%u, debug_print=%s",
			      rpc_area->state, rpc_area->call_id, rpc_area->size, p->data, p->debug_print.string);

	rpos = msg_host->out_rpos;
	wpos = msg_mve->out_wpos;

	while ((capacity = get_capacity(rpos, wpos)) >= sizeof(header)) {
		unsigned int pos;

		pos = read32n(msg_mve->out_data, rpos, (uint32_t *)&header,
			      sizeof(header));

		MVX_LOG_PRINT_SESSION(&mvx_log_session_if, MVX_LOG_WARNING,
				      fw->session,
				      "queue={rpos=%u, wpos=%u, capacity=%u}, msg={code=%u, size=%u}",
				      rpos, wpos, capacity,
				      header.code, header.size);

		capacity = get_capacity(pos, wpos);
		if (header.size > capacity) {
			MVX_LOG_PRINT_SESSION(
				&mvx_log_session_if, MVX_LOG_WARNING,
				fw->session,
				"Size is larger than capacity. capacity=%zd, size=%u.",
				capacity, header.size);
			return;
		}

		rpos = (pos + DIV_ROUND_UP(header.size, sizeof(uint32_t))) %
		       MVE_COMM_QUEUE_SIZE_IN_WORDS;
	}
}

int mvx_fw_send_idle_ack_v2(struct mvx_fw *fw)
{
	int ret = 0;

	ret = write_message(fw, fw->msg_host, fw->msg_mve,
			    MVE_REQUEST_CODE_IDLE_ACK,
			    NULL, 0,
			    MVX_LOG_FWIF_CHANNEL_MESSAGE, fw->buf_attr[MVX_FW_REGION_MSG_MVE], fw->buf_attr[MVX_FW_REGION_MSG_HOST]);

	return ret;
}

/****************************************************************************
 * Exported functions
 ****************************************************************************/

int mvx_fw_construct_v2(struct mvx_fw *fw,
			struct mvx_fw_bin *fw_bin,
			struct mvx_mmu *mmu,
			struct mvx_session *session,
			struct mvx_client_ops *client_ops,
			struct mvx_client_session *csession,
			unsigned int ncores,
			unsigned char major,
			unsigned char minor)
{
	int ret;

	ret = mvx_fw_construct(fw, fw_bin, mmu, session, client_ops, csession,
			       ncores);
	if (ret != 0)
		return ret;

	fw->ops.map_protocol = map_protocol_v2;
	fw->ops.unmap_protocol = unmap_protocol_v2;
	fw->ops.get_region = get_region_v2;
	fw->ops.get_message = get_message_v2;
	fw->ops.put_message = put_message_v2;
	fw->ops.handle_rpc = handle_rpc_v2;
#ifdef MVX_FW_DEBUG_ENABLE
	fw->ops.handle_fw_ram_print = handle_fw_ram_print_v2;
#endif
	fw->ops.print_stat = print_stat_v2;
	fw->ops.print_debug = print_debug_v2;
	fw->ops_priv.send_idle_ack = NULL;
	fw->ops_priv.to_mve_profile = mvx_fw_to_mve_profile_v2;
	fw->ops_priv.to_mve_level = mvx_fw_to_mve_level_v2;

	fw->buf_attr[MVX_FW_REGION_MSG_HOST] = MVX_FW_BUF_CACHEABLE;
	fw->buf_attr[MVX_FW_REGION_MSG_MVE] = MVX_FW_BUF_UNCACHEABLE;
	fw->buf_attr[MVX_FW_REGION_BUF_IN_HOST] = MVX_FW_BUF_CACHEABLE;
	fw->buf_attr[MVX_FW_REGION_BUF_IN_MVE] = MVX_FW_BUF_CACHEABLE;
	fw->buf_attr[MVX_FW_REGION_BUF_OUT_HOST] = MVX_FW_BUF_CACHEABLE;
	fw->buf_attr[MVX_FW_REGION_BUF_OUT_MVE] = MVX_FW_BUF_CACHEABLE;
	fw->buf_attr[MVX_FW_REGION_RPC] = MVX_FW_BUF_CACHEABLE;

	fw->map_op[MVX_FW_BUF_CACHEABLE] = map_msq;
	fw->map_op[MVX_FW_BUF_UNCACHEABLE] = map_msq_uncache;
	fw->unmap_op[MVX_FW_BUF_CACHEABLE] = unmap_msq;
	fw->unmap_op[MVX_FW_BUF_UNCACHEABLE] = unmap_msq_uncache;

	if (major == 2 && minor >= 4)
		fw->ops_priv.send_idle_ack = mvx_fw_send_idle_ack_v2;

	return 0;
}
