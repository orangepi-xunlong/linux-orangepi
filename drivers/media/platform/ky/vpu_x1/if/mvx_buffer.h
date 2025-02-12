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

#ifndef _MVX_BUFFER_H_
#define _MVX_BUFFER_H_

/****************************************************************************
 * Includes
 ****************************************************************************/

#include <linux/seq_file.h>
#include <linux/types.h>
#include "mvx_if.h"
#include "mvx_mmu.h"
/****************************************************************************
 * Defines
 ****************************************************************************/

#define MVX_BUFFER_NPLANES    3
#define MVX_ROI_QP_NUMS       10

/****************************************************************************
 * Types
 ****************************************************************************/

struct device;

/**
 * struct mvx_buffer_plane - Plane information.
 * @pages:	MMU pages object.
 * @stride:	Horizontal stride in bytes.
 * @filled:	Number of bytes written to this plane. For a frame buffer this
 *              value should always match the size of the plane.
 * @offset:	Offset in bytes from begin of buffer to first bitstream data.
 * @afbc_width: AFBC width in superblocks.
 */
struct mvx_buffer_plane {
	struct mvx_mmu_pages *pages;
	unsigned int stride;
	unsigned int filled;
	unsigned int offset;
	unsigned int afbc_width;
};

struct mvx_buffer_general_encoder_stats
{
    uint8_t encoder_stats_type;
        #define MVX_ENCODER_STATS_TYPE_FULL       (0x01)
    uint8_t frame_type;         // See MVE_FRAME_TYPE_*
        #define MVX_FRAME_TYPE_I 0
        #define MVX_FRAME_TYPE_P 1
        #define MVX_FRAME_TYPE_B 2
    uint8_t used_as_reference;  // 0=No, 1=Yes
    uint8_t qp;                 // base quantizer used for the frame
                                // HEVC, H.264: 0-51. VP9: 0-63
    uint32_t picture_count;     // display order picture count
    uint16_t num_cols;          // number of columns (each 32 pixels wide)
    uint16_t num_rows;          // number of rows    (each 32 pixels high)
    uint32_t ref_pic_count[2];  // display order picture count of references
                                // unused values are set to zero
};

struct mvx_buffer_general_rows_uncomp_hdr
{
    uint8_t n_cols_minus1; /* number of quad cols in picture minus 1 */
    uint8_t n_rows_minus1; /* number of quad rows in picture minus 1 */
    uint8_t reserved[2];
};

struct mvx_buffer_general_block_configs
{
    uint8_t blk_cfg_type;
        #define MVX_BLOCK_CONFIGS_TYPE_NONE       (0x00)
        #define MVX_BLOCK_CONFIGS_TYPE_ROW_UNCOMP (0xff)
    uint8_t reserved[3];
    union
    {
        struct mvx_buffer_general_rows_uncomp_hdr rows_uncomp;
    } blk_cfgs;
};

struct mvx_buffer_general_hdr
{
    /* For identification of the buffer, this is not changed by the firmware. */
    uint64_t host_handle;

    /* this depends upon the type of the general purpose buffer */
    uint64_t user_data_tag;

    /* pointer to the buffer containing the general purpose data. the format
     * of this data is defined by the configuration in the mve_buffer_general */
    uint32_t buffer_ptr;

    /* size of the buffer pointed to by buffer_ptr */
    uint32_t buffer_size;

    /* selects the type of semantics to use for the general purpose buffer. it
     * tags (or discriminates) the union config member in mve_buffer_general
     */
    uint16_t type;                                        /* Extra data: */
        #define MVX_BUFFER_GENERAL_TYPE_BLOCK_CONFIGS (1) /* block_configs */
        #define MVX_BUFFER_GENERAL_TYPE_ENCODER_STATS (4) /* encoder_stats */

    /* size of the mve_buffer_general config member */
    uint16_t config_size;

    /* pad to force 8-byte alignment */
    uint32_t reserved;
};

struct mvx_buffer_general
{
    struct mvx_buffer_general_hdr header;

    /* used to describe the configuration of the general purpose buffer data
     * pointed to be buffer_ptr
     */
    union
    {
        struct mvx_buffer_general_block_configs block_configs;
        struct mvx_buffer_general_encoder_stats encoder_stats;
    } config;
};


/**
 * struct mvx_buffer - Buffer descriptor.
 * @dev:	Pointer to device.
 * @mmu:	Pointer to MMU.
 * @head:	List head used to add buffer to various queues.
 * @format:	Bitstream or frame format.
 * @dir:	Direction the buffer was allocated for.
 * @user_data:	User data copied from input- to output buffer.
 * @flags:	Buffer flags.
 * @width:	Frame width in pixels.
 * @height:	Frame height in pixels.
 * @crop_left:	Left crop in pixels.
 * @crop_top:	Top crop in pixels.
 * @nplanes:	Number of planes.
 * @planes:	Array or planes.
 */
struct mvx_buffer {
	struct device *dev;
	struct mvx_mmu *mmu;
	struct list_head head;
	enum mvx_format format;
	enum mvx_direction dir;
	uint64_t user_data;
	unsigned int flags;
	unsigned int width;
	unsigned int height;
	unsigned int crop_left;
	unsigned int crop_top;
	unsigned int nplanes;
	struct mvx_buffer_plane planes[MVX_BUFFER_NPLANES];
    struct mvx_buffer_general general;
};

/**
 * struct mvx_buffer - Buffer descriptor.
 * @head:	List head used to add buffer to corrupt buffer queues.
 * @user_data:	User data copied from input- to output buffer.
 */
struct mvx_corrupt_buffer {
	struct list_head head;
	uint64_t user_data;
};

#define MVX_BUFFER_EOS                  0x00000001
#define MVX_BUFFER_EOF                  0x00000002
#define MVX_BUFFER_CORRUPT              0x00000004
#define MVX_BUFFER_REJECTED             0x00000008
#define MVX_BUFFER_DECODE_ONLY          0x00000010
#define MVX_BUFFER_CODEC_CONFIG         0x00000020
#define MVX_BUFFER_AFBC_TILED_HEADERS   0x00000040
#define MVX_BUFFER_AFBC_TILED_BODY      0x00000080
#define MVX_BUFFER_AFBC_32X8_SUPERBLOCK 0x00000100
#define MVX_BUFFER_INTERLACE            0x00000200
#define MVX_BUFFER_END_OF_SUB_FRAME     0x00000400
#define MVX_BUFFER_FRAME_PRESENT        0x00000800


#define MVX_BUFFER_FRAME_FLAG_ROTATION_90   0x00001000  /* Frame is rotated 90 degrees */
#define MVX_BUFFER_FRAME_FLAG_ROTATION_180  0x00002000  /* Frame is rotated 180 degrees */
#define MVX_BUFFER_FRAME_FLAG_ROTATION_270  0x00003000  /* Frame is rotated 270 degrees */
#define MVX_BUFFER_FRAME_FLAG_ROTATION_MASK 0x00003000

#define MVX_BUFFER_FRAME_FLAG_MIRROR_HORI   0x00010000
#define MVX_BUFFER_FRAME_FLAG_MIRROR_VERT   0x00020000
#define MVX_BUFFER_FRAME_FLAG_MIRROR_MASK   0x00030000

#define MVX_BUFFER_FRAME_FLAG_SCALING_2     0x00004000  /* Frame is scaled by half */
#define MVX_BUFFER_FRAME_FLAG_SCALING_4     0x00008000  /* Frame is scaled by quarter */
#define MVX_BUFFER_FRAME_FLAG_SCALING_MASK  0x0000C000

#define MVX_BUFFER_FRAME_FLAG_GENERAL       0x00040000  /* Frame is a general buffer */
#define MVX_BUFFER_FRAME_FLAG_ROI           0x00080000  /* This buffer has a roi region */

#define MVX_BUFFER_FRAME_NEED_REALLOC       0x00100000  /* This buffer needs realloc */

#define MVX_BUFFER_FRAME_FLAG_IFRAME        0x20000000
#define MVX_BUFFER_FRAME_FLAG_PFRAME        0x40000000
#define MVX_BUFFER_FRAME_FLAG_BFRAME        0x80000000

#define MVX_BUFFER_AFBC_BLOCK_SPLIT         0x10000000

#define MVX_BUFFER_FLAG_DISABLE_CACHE_MAINTENANCE 0x01000000 /*disable cache maintenance for buffer */
/****************************************************************************
 * External functions
 ****************************************************************************/

/**
 * mvx_buffer_construct() - Construct the buffer object.
 * @buf:	Pointer to buffer.
 * @dev:	Pointer to device.
 * @mmu:	Pointer to MMU.
 * @dir:	Which direction the buffer was allocated for.
 * @nplanes:	Number of planes.
 * @sgt:	Array with SG tables. Each table contains a list of memory
 *              pages for corresponding plane.
 *
 * Return: 0 on success, else error code.
 */
int mvx_buffer_construct(struct mvx_buffer *buf,
			 struct device *dev,
			 struct mvx_mmu *mmu,
			 enum mvx_direction dir,
			 unsigned int nplanes,
			 struct sg_table **sgt);

/**
 * mvx_buffer_construct() - Destruct the buffer object.
 * @buf:	Pointer to buffer.
 */
void mvx_buffer_destruct(struct mvx_buffer *buf);

/**
 * mvx_buffer_map() - Map the buffer to the MVE virtual address space.
 * @buf:	Pointer to buffer.
 * @begin:	MVE virtual begin address.
 * @end:	MVE virtual end address.
 *
 * Try to MMU map the buffer anywhere between the begin and end addresses.
 *
 * Return: 0 on success, else error code.
 */
int mvx_buffer_map(struct mvx_buffer *buf,
		   mvx_mmu_va begin,
		   mvx_mmu_va end);

/**
 * mvx_buffer_unmap() - Unmap the buffer from the MVE virtual address space.
 * @buf:	Pointer to buffer.
 */
void mvx_buffer_unmap(struct mvx_buffer *buf);

/**
 * mvx_buffer_is_mapped() - Return if buffer has been mapped.
 * @buf:	Pointer to buffer.
 *
 * Return: True if mapped, else false.
 */
bool mvx_buffer_is_mapped(struct mvx_buffer *buf);

/**
 * mvx_buffer_synch() - Synch the data caches.
 * @buf:	Pointer to buffer.
 * @dir:	Data direction.
 *
 * Return: 0 on success, else error code.
 */
int mvx_buffer_synch(struct mvx_buffer *buf,
		     enum dma_data_direction dir);

/**
 * mvx_buffer_clear() - Clear and empty the buffer.
 * @buf:	Pointer to buffer.
 */
void mvx_buffer_clear(struct mvx_buffer *buf);

/**
 * mvx_buffer_filled_set() - Set filled bytes for each plane.
 * @buf:	Pointer to buffer.
 * @plane:	Plane index.
 * @filled:	Number of bytes filled.
 * @offset:	Number of bytes offset.
 *
 * Return: 0 on success, else error code.
 */
int mvx_buffer_filled_set(struct mvx_buffer *buf,
			  unsigned int plane,
			  unsigned int filled,
			  unsigned int offset);

/**
 * mvx_buffer_size() - Get size in bytes for a plane.
 * @buf:	Pointer to buffer.
 * @plane:	Which plane to get size for.
 *
 * Return: Size of plane.
 */
size_t mvx_buffer_size(struct mvx_buffer *buf,
		       unsigned int plane);

/**
 * mvx_buffer_va() - Get VA for a plane.
 * @buf:	Pointer to buffer.
 * @plane:	Plane index.
 *
 * Return: VA address of plane, 0 if unmapped.
 */
mvx_mmu_va mvx_buffer_va(struct mvx_buffer *buf,
			 unsigned int plane);

/**
 * mvx_buffer_frame_dim() - Get frame buffer dimensions.
 * @format:	Bitstream or frame format.
 * @width:	Width in pixels.
 * @height:	Height in pixels.
 * @nplanes:	Number of planes for this format.
 * @stride:	Horizontal stride in bytes.
 * @size:	Size in bytes for each plane.
 *
 * If *nplanes is larger than 0 then the stride is used as input to tell this
 * function which stride that is desired, but it might be modified if the
 * stride is too short or not optimal for the MVE hardware.
 *
 * Return: 0 on success, else error code.
 */
int mvx_buffer_frame_dim(enum mvx_format format,
			 unsigned int width,
			 unsigned int height,
			 uint8_t *nplanes,
			 unsigned int *stride,
			 unsigned int *size);

/**
 * mvx_buffer_frame_set() - Set frame dimensions.
 * @buf:	Pointer to buffer.
 * @format:	Bitstream or frame format.
 * @width:	Width in pixels.
 * @height:	Height in pixels.
 * @stride:	Horizontal stride in bytes.
 * @size:	Size in bytes for each plane.
 * @interlaced:	Defines if the buffer is interlaced.
 *
 * Return: 0 on success, else error code.
 */
int mvx_buffer_frame_set(struct mvx_buffer *buf,
			 enum mvx_format format,
			 unsigned int width,
			 unsigned int height,
			 unsigned int *stride,
			 unsigned int *size,
			 bool interlaced);

/**
 * mvx_buffer_afbc_set() - Set AFBC dimensions.
 * @buf:	Pointer to buffer.
 * @format:	Bitstream or frame format.
 * @width:	Width in pixels.
 * @height:	Height in pixels.
 * @afbc_width:	AFBC width in superblocks.
 * @size:	Size in bytes for AFBC plane.
 * @interlaced:	Defines if the buffer is interlaced.
 *
 * Return: 0 on success, else error code.
 */
int mvx_buffer_afbc_set(struct mvx_buffer *buf,
			enum mvx_format format,
			unsigned int width,
			unsigned int height,
			unsigned int afbc_width,
			unsigned int size,
			bool interlaced);

/**
 * mvx_buffer_show() - Print debug information into seq-file.
 * @buf:	Pointer to buffer.
 * @s:		Seq-file to print to.
 */
void mvx_buffer_show(struct mvx_buffer *buf,
		     struct seq_file *s);

#endif /* _MVX_BUFFER_H_ */
