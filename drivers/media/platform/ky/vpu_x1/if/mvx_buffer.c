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

#include <linux/kernel.h>
#include "mvx_buffer.h"
#include "mvx_seq.h"
#include "mvx_log_group.h"

/****************************************************************************
 * Defines
 ****************************************************************************/

/**
 * Each 2x2 pixel square is subsampled. How many samples that are taken depends
 * on the color format, but typically the luma channel (Y) gets 4 samples and
 * the luma channels (UV) get 2 or 4 samples.
 */
#define SUBSAMPLE_PIXELS 2

/****************************************************************************
 * Static functions
 ****************************************************************************/

/**
 * get_stride() - Get 3 plane stride for 2x2 pixels square.
 * @format:	MVX frame format.
 * @stride:	[plane 0, plane 1, plane 2][x, y] stride.
 *
 * Calculate the stride in bytes for each plane for a subsampled (2x2) pixels
 * square.
 *
 * Return: 0 on success, else error code.
 */
static int get_stride(enum mvx_format format,
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

static int map_plane(struct mvx_buffer *buf,
		     mvx_mmu_va begin,
		     mvx_mmu_va end,
		     unsigned int plane)
{
	while (begin < end) {
		struct mvx_buffer_plane *p = &buf->planes[plane];
		int ret;

		ret = mvx_mmu_map_pages(buf->mmu, begin, p->pages,
					MVX_ATTR_SHARED_RW,
					MVX_ACCESS_READ_WRITE);
		if (ret == 0) {
			MVX_LOG_PRINT(&mvx_log_if, MVX_LOG_INFO,
				      "Memory map buffer. buf=%p, plane=%u, va=0x%x, size=%zu.",
				      buf, plane, p->pages->va,
				      mvx_buffer_size(buf, plane));
			return 0;
		}

		if (ret != -EAGAIN)
			return ret;

		begin += 1 * 1024 * 1024; /* 1 MB. */
	}

	return -ENOMEM;
}

/****************************************************************************
 * External functions
 ****************************************************************************/

void mvx_buffer_show(struct mvx_buffer *buf,
		     struct seq_file *s)
{
	int i;
	int ind = 0;

	mvx_seq_printf(s, "mvx_buffer", ind, "%p\n", buf);

	ind++;
	mvx_seq_printf(s, "format", ind, "0x%x\n", buf->format);
	mvx_seq_printf(s, "dir", ind, "%u\n", buf->dir);
	mvx_seq_printf(s, "flags", ind, "0x%0x\n", buf->flags);
	mvx_seq_printf(s, "width", ind, "%u\n", buf->width);
	mvx_seq_printf(s, "height", ind, "%u\n", buf->height);
	mvx_seq_printf(s, "nplanes", ind, "%u\n", buf->nplanes);
	mvx_seq_printf(s, "planes", ind, "\n");
	ind++;
	for (i = 0; i < buf->nplanes; ++i) {
		char tag[10];
		struct mvx_buffer_plane *plane = &buf->planes[i];

		scnprintf(tag, sizeof(tag), "#%d", i);
		mvx_seq_printf(s, tag, ind,
			       "va: 0x%08x, size: %10zu, stride: %5u, filled: %10u\n",
			       mvx_buffer_va(buf, i),
			       mvx_buffer_size(buf, i),
			       plane->stride,
			       plane->filled);
	}

	ind--;
}

int mvx_buffer_construct(struct mvx_buffer *buf,
			 struct device *dev,
			 struct mvx_mmu *mmu,
			 enum mvx_direction dir,
			 unsigned int nplanes,
			 struct sg_table **sgt)
{
	int i;

	if (nplanes > MVX_BUFFER_NPLANES) {
		MVX_LOG_PRINT(&mvx_log_if, MVX_LOG_WARNING,
			      "Failed to construct buffer. Too many planes. nplanes=%u.",
			      nplanes);
		return -EINVAL;
	}

	memset(buf, 0, sizeof(*buf));

	buf->dev = dev;
	buf->mmu = mmu;
	buf->dir = dir;
	buf->nplanes = nplanes;

	for (i = 0; i < buf->nplanes; ++i) {
		struct mvx_buffer_plane *plane = &buf->planes[i];

		if (sgt[i] == NULL)
			break;

		plane->pages = mvx_mmu_alloc_pages_sg(dev, sgt[i], 0);
		if (IS_ERR(plane->pages))
			goto free_pages;
	}

	return 0;

free_pages:
	while (i--)
		mvx_mmu_free_pages(buf->planes[i].pages);

	return -ENOMEM;
}

void mvx_buffer_destruct(struct mvx_buffer *buf)
{
	int i;

	mvx_buffer_unmap(buf);

	for (i = 0; i < buf->nplanes; i++)
		if (buf->planes[i].pages != NULL)
			mvx_mmu_free_pages(buf->planes[i].pages);
}

int mvx_buffer_map(struct mvx_buffer *buf,
		   mvx_mmu_va begin,
		   mvx_mmu_va end)
{
	int i;
	int ret = 0;

	for (i = 0; i < buf->nplanes; i++) {
		struct mvx_buffer_plane *plane = &buf->planes[i];

		if (plane->pages != NULL) {
			ret = map_plane(buf, begin, end, i);
			if (ret != 0) {
				mvx_buffer_unmap(buf);
				break;
			}
		}
	}

	return ret;
}

void mvx_buffer_unmap(struct mvx_buffer *buf)
{
	int i;

	for (i = 0; i < buf->nplanes; i++) {
		struct mvx_buffer_plane *plane = &buf->planes[i];

		if ((plane->pages != NULL) && (plane->pages->va != 0))
			mvx_mmu_unmap_pages(plane->pages);
	}
}

bool mvx_buffer_is_mapped(struct mvx_buffer *buf)
{
	return (buf->planes[0].pages != NULL) &&
	       (buf->planes[0].pages->va != 0);
}

int mvx_buffer_synch(struct mvx_buffer *buf,
		     enum dma_data_direction dir)
{
	int i;
	int ret;
	int page_count = 0;
	for (i = 0; i < buf->nplanes; i++) {
        struct mvx_buffer_plane *plane = &buf->planes[i];
        /*calculate page offset of plane, follow as 'mvx_buffer_va' */
        int page_off = (plane->offset + plane->pages->offset) >> PAGE_SHIFT;
        /*calculate page end of plane if filled is valid */
        int page_off_end = plane->pages->count;
        if (plane->filled) {
            page_off_end = (plane->offset + plane->pages->offset + plane->filled + PAGE_SIZE -1 ) >> PAGE_SHIFT;
        }
        page_count = page_off_end - page_off;
        if (page_count + page_off > plane->pages->count) {
            page_count = plane->pages->count - page_off;
        }
        MVX_LOG_PRINT(&mvx_log_if, MVX_LOG_INFO,
            "plane [%d] sync pages: %p, dir: %d, page offset: %d, pages %d",
            i,
            plane->pages->pages[0],
            buf->dir,
            page_off, page_count);
        if (plane->pages != NULL) {
			ret = mvx_mmu_synch_pages(plane->pages, dir, page_off, page_count);
			if (ret != 0)
				return ret;
		}
	}

	return 0;
}

void mvx_buffer_clear(struct mvx_buffer *buf)
{
	unsigned int i;

	buf->flags = 0;

	for (i = 0; i < buf->nplanes; i++)
		buf->planes[i].filled = 0;
}

int mvx_buffer_filled_set(struct mvx_buffer *buf,
			  unsigned int plane,
			  unsigned int filled,
			  unsigned int offset)
{
	struct mvx_buffer_plane *p = &buf->planes[plane];
	size_t size = mvx_buffer_size(buf, plane);

	if (plane > buf->nplanes)
		return -EINVAL;

	if (size < (filled + offset)) {
		MVX_LOG_PRINT(&mvx_log_if, MVX_LOG_WARNING,
			      "Buffer plane too small. plane=%d, size=%zu, filled=%u, offset=%u.",
			      plane, size, filled, offset);
        buf->flags |= MVX_BUFFER_FRAME_NEED_REALLOC;
		return -ENOMEM;
	}

	p->filled = filled;
	p->offset = offset;

	return 0;
}

size_t mvx_buffer_size(struct mvx_buffer *buf,
		       unsigned int plane)
{
	struct mvx_buffer_plane *p = &buf->planes[plane];

	if (plane >= buf->nplanes || p->pages == NULL)
		return 0;

	return mvx_mmu_size_pages(p->pages);
}

mvx_mmu_va mvx_buffer_va(struct mvx_buffer *buf,
			 unsigned int plane)
{
	struct mvx_buffer_plane *p = &buf->planes[plane];

	if (plane >= buf->nplanes || p->pages == NULL)
		return 0;

	return p->pages->va + p->pages->offset + p->offset;
}

int mvx_buffer_frame_dim(enum mvx_format format,
			 unsigned int width,
			 unsigned int height,
			 uint8_t *nplanes,
			 unsigned int *stride,
			 unsigned int *size)
{
	unsigned int s[MVX_BUFFER_NPLANES][2];
	unsigned int __nplanes = *nplanes;
	int i;
	int ret;

	ret = get_stride(format, nplanes, s);
	if (ret != 0)
		return ret;

	for (i = 0; i < *nplanes; i++) {
		const unsigned int stride_align = 1;
		unsigned int tmp = DIV_ROUND_UP(width * s[i][0],
						SUBSAMPLE_PIXELS);
		/* Use optimal stride if no special stride was requested. */
		if (i >= __nplanes || stride[i] == 0)
			stride[i] = round_up(tmp, stride_align);
		/* Else make sure to round up to minimum stride. */
		else
			stride[i] = max(stride[i], tmp);

		size[i] = DIV_ROUND_UP(height * s[i][1],
				       SUBSAMPLE_PIXELS ) * stride[i];
	}
    /* a workaround patch for nv12/nv21/p010 odd height/width output*/
    if (*nplanes == 2 && (width % 2 != 0 || height % 2 != 0)) {
        unsigned int tmp = DIV_ROUND_UP(width, SUBSAMPLE_PIXELS) * s[1][0];
        stride[1] = max(stride[1], tmp);
        size[1] = DIV_ROUND_UP(height * s[1][1],
				       SUBSAMPLE_PIXELS ) * stride[1];
    }

    for (i = *nplanes; i < MVX_BUFFER_NPLANES; i++) {
        size[i] = 0;
    }
	return 0;
}

int mvx_buffer_frame_set(struct mvx_buffer *buf,
			 enum mvx_format format,
			 unsigned int width,
			 unsigned int height,
			 unsigned int *stride,
			 unsigned int *size,
			 bool interlaced)
{
	int i;

	buf->format = format;
	buf->width = width;
	buf->height = height;

	for (i = 0; i < buf->nplanes; i++) {
		struct mvx_buffer_plane *plane = &buf->planes[i];

		plane->stride = stride[i];

		if (buf->dir == MVX_DIR_OUTPUT) {
			int ret;

			ret = mvx_buffer_filled_set(buf, i, size[i], plane->offset);
			if (ret != 0)
				return ret;
		}

		/* Verify that plane has correct length. */
		if (plane->filled > 0 && plane->filled != size[i]) {
			MVX_LOG_PRINT(&mvx_log_if, MVX_LOG_INFO,
				      "Buffer filled length does not match plane size. plane=%i, filled=%zu, size=%u.",
				      i, plane->filled, size[i]);
			//return -ENOMEM;
		}

		/* Verify that there is no buffer overflow. */
		if ((plane->filled + plane->offset) > mvx_buffer_size(buf, i)) {
			MVX_LOG_PRINT(&mvx_log_if, MVX_LOG_WARNING,
				      "Buffer plane size is too small. plane=%i, size=%zu, size=%u, filled=%u, offset=%u",
				      i, size[i], mvx_buffer_size(buf, i), plane->filled, plane->offset);
			return -ENOMEM;
		}
	}

	if (interlaced != false){
		buf->flags |= MVX_BUFFER_INTERLACE;
	} else {
		buf->flags &= ~MVX_BUFFER_INTERLACE;
	}
	return 0;
}

int mvx_buffer_afbc_set(struct mvx_buffer *buf,
			enum mvx_format format,
			unsigned int width,
			unsigned int height,
			unsigned int afbc_width,
			unsigned int size,
			bool interlaced)
{
	int ret;

	buf->format = format;
	buf->width = width;
	buf->height = height;
	buf->planes[0].afbc_width = afbc_width;

	if (buf->dir == MVX_DIR_INPUT) {
		buf->crop_left = 0;
		buf->crop_top = 0;
	}

	if (buf->dir == MVX_DIR_OUTPUT) {
		ret = mvx_buffer_filled_set(buf, 0, size, buf->planes[0].offset);
		if (ret != 0)
			return ret;
	}

	if (size > mvx_buffer_size(buf, 0)) {
		MVX_LOG_PRINT(&mvx_log_if, MVX_LOG_WARNING,
			      "AFBC buffer too small. buf_size=%zu, size=%u.",
			      size, mvx_buffer_size(buf, 0));
		return -ENOMEM;
	}

	if (interlaced != false)
		buf->flags |= MVX_BUFFER_INTERLACE;

	return 0;
}
