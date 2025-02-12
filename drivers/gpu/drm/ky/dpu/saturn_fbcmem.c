// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2023 Ky Co., Ltd.
 *
 */

#include <linux/stddef.h>
#include <linux/export.h>
#include <drm/drm_atomic.h>
#include <drm/drm_framebuffer.h>
#include <drm/drm_mode.h>
#include <drm/drm_print.h>
#include <drm/drm_fourcc.h>
#include <drm/drm_fourcc.h>
#include "../ky_cmdlist.h"
#include "saturn_fbcmem.h"

static int get_fbc_block_size_by_modifier(uint64_t modifier, u32* out_fbc_block_size) {
	int ret_val = 0;
	uint64_t super_block_size = modifier & AFBC_FORMAT_MOD_BLOCK_SIZE_MASK;

	if (super_block_size == AFBC_FORMAT_MOD_BLOCK_SIZE_16x16) {
		*out_fbc_block_size = FBC_BLOCK_SIZE_16x16;
	} else if (super_block_size == AFBC_FORMAT_MOD_BLOCK_SIZE_32x8) {
		*out_fbc_block_size = FBC_BLOCK_SIZE_32x8;
	} else {
		DRM_ERROR("FBC_MEM: failed to get fbc block size info for modifier = 0x%llx\n", super_block_size);
		ret_val = -1;
	}

	return ret_val;
}

static bool is_bpp32_in_fbc_mem_cal(uint32_t drm_4cc_fmt) {
	bool is_taken_as_bpp32 = false;
	const struct drm_format_info *info = NULL;

	info = drm_format_info(drm_4cc_fmt);
	if (info->num_planes == 1 && info->cpp[0] == 4) {
		is_taken_as_bpp32 = true;
	} else {
		switch (drm_4cc_fmt) {
			case DRM_FORMAT_RGB888:   //RDMA_FMT_RGB_888:
			case DRM_FORMAT_BGR888:   //RDMA_FMT_BGR_888:
				is_taken_as_bpp32 = true;
				break;
			default:
				break;
		}
	}

	return is_taken_as_bpp32;
}

static u32 adjust_afbc_layer_mem_size(u8 rdma_work_mode, u32 drm_4cc_fmt, u32 fbc_block_size, bool rot_90_or_270, u32 fbcmem_size) {
	u32 ret_size = fbcmem_size;
	bool is_bpp_32 = is_bpp32_in_fbc_mem_cal(drm_4cc_fmt);

	//if left_right mode fbc_layout 32x8, is_bpp_32 and not rotation 90/270, hw request double mem_size.
	if ((rdma_work_mode == LEFT_RIGHT) && (fbc_block_size == FBC_BLOCK_SIZE_32x8) && is_bpp_32 && (rot_90_or_270 == false)) {
		ret_size *= 2;
	}
	DRM_DEBUG("FBC_MEM: %s, rdma_mode = %u, is_bpp_32 = %d, fbc_block_size = %u, rot_90_or_270 = %d, input = %u, ret = %u\n",
		__func__, rdma_work_mode, is_bpp_32, fbc_block_size, rot_90_or_270, fbcmem_size, ret_size);

	return ret_size;
}

int get_raw_data_plane_rdma_mem_size(u32 drm_4cc_fmt, bool rot_90_or_270, u32 plane_crop_width, u32* output_mem_size) {
	u8 index = 0;
	u32 ret_mem_size = 0;
	u32 data_plane_mem_size[3] = {0}; //max 3 plane, YUV data
	const struct drm_format_info *info = NULL;
	//struct drm_format_name_buf fmt_name = {0};

	//drm_get_format_name(drm_4cc_fmt, &fmt_name);
	if (rot_90_or_270) {
		ret_mem_size = 1024; //dpu hardware request 32K fix size, 32 byte unit has considered
	} else {
		info = drm_format_info(drm_4cc_fmt);
		if (info->cpp[0] == 0) {
			DRM_ERROR("FBC_MEM: not support format %d\n", drm_4cc_fmt);
			return -1;
		}
		if (info->num_planes == 1 && info->is_yuv == false) {
			data_plane_mem_size[0] = plane_crop_width * info->cpp[0];
		} else if (info->num_planes >= 2 && info->is_yuv) {
			data_plane_mem_size[0] = plane_crop_width * info->cpp[0];
			data_plane_mem_size[1] = plane_crop_width * info->cpp[1]/info->hsub;
			if (info->num_planes == 3) {
				data_plane_mem_size[2] = plane_crop_width * info->cpp[2]/info->vsub;
			}
		} else {
			DRM_ERROR("FBC_MEM: not considered drm format %d\n", drm_4cc_fmt);
			return -1;
		}
		for (index = 0; index < info->num_planes; index++) { //max 3 plane, YUV data
			data_plane_mem_size[index] = roundup(data_plane_mem_size[index], 64); //dpu hardware request
			ret_mem_size += data_plane_mem_size[index];
		}
		ret_mem_size = roundup(ret_mem_size, 64);
		ret_mem_size = ret_mem_size/32;
	}
	if (output_mem_size) {
		*output_mem_size = ret_mem_size;
		DRM_DEBUG("FBC_MEM: raw layer, fmt = %d, rot_90_270 = %d, crop_w = %u, cal memsize = %u\n",
		drm_4cc_fmt, rot_90_or_270, plane_crop_width, *output_mem_size);
	}
	return 0;
}
EXPORT_SYMBOL(get_raw_data_plane_rdma_mem_size);

int get_afbc_data_plane_min_rdma_mem_size(u8 rdma_work_mode, u32 drm_4cc_fmt,
		u32 crop_start_x, u32 crop_start_y, u32 crop_width, u32 crop_height,
		u32 fbc_block_size, bool rot_90_or_270, u8 min_lines, u32* output_mem_size) {
	bool is_bpp_32 = false;
	uint32_t ret_mem_size = 0;
	uint32_t crop_start_align = 0;
	uint32_t crop_end_align = 0;
	uint32_t bbox_width = 0;
	uint32_t num_addr_per_line = 0;
	uint32_t num_addr_frac4 = 0;
	uint32_t num_addr_4line = 0;
	uint32_t num_addr_frac8 = 0;
	uint32_t num_addr_8line = 0;
	uint32_t align_num = (fbc_block_size == FBC_BLOCK_SIZE_16x16) ? 16 : 32;
	//struct drm_format_name_buf fmt_name = {0};

	//drm_get_format_name(drm_4cc_fmt, &fmt_name);
	DRM_DEBUG("FBC_MEM: afbc layer, fmt = %d, \
		crop_x = %u, crop_y = %u, crop_w = %u, crop_h = %u, \
		fbc_block_size = %u, rot_90_270 = %u, min_lines = %u, rdma_mode = %u\n",
		drm_4cc_fmt, crop_start_x, crop_start_y, crop_width,
		crop_height, fbc_block_size, rot_90_or_270, min_lines, rdma_work_mode);

	if (rot_90_or_270) {
		crop_start_align = rounddown(crop_start_y, align_num);
		crop_end_align = roundup(crop_start_y + crop_height, align_num) - 1;
	} else {
		crop_start_align = rounddown(crop_start_x, align_num);
		crop_end_align = roundup(crop_start_x + crop_width, align_num) - 1;
	}

	bbox_width = crop_end_align - crop_start_align + 1;
	is_bpp_32 = is_bpp32_in_fbc_mem_cal(drm_4cc_fmt);
	num_addr_per_line = is_bpp_32 ? bbox_width/8 : bbox_width/16;

	if (min_lines == 4) {
		num_addr_frac4 = (num_addr_per_line % 8 == 0) ? num_addr_per_line/8 : (num_addr_per_line/8 + 1);
		num_addr_4line = num_addr_frac4 * 8 * 4;
		ret_mem_size = num_addr_4line;
	} else if (min_lines == 8) {
		num_addr_frac8 = (num_addr_per_line % 16 == 0) ? num_addr_per_line/16 : (num_addr_per_line/16 + 1);
		num_addr_8line = num_addr_frac8 * 16 * 8;
		ret_mem_size = num_addr_8line;
	}

	ret_mem_size = adjust_afbc_layer_mem_size(rdma_work_mode, drm_4cc_fmt, fbc_block_size, rot_90_or_270, ret_mem_size);
	*output_mem_size = ret_mem_size;

	DRM_DEBUG("FBC_MEM: afbc layer, fmt = %d, \
		crop_x = %u, crop_y = %u, crop_w = %u, crop_h = %u, \
		fbc_block_size = %u, rot_90_270 = %u, min_lines = %u, rdma_mode = %u, cal memsize = %u\n",
		drm_4cc_fmt, crop_start_x, crop_start_y, crop_width,
		crop_height, fbc_block_size, rot_90_or_270, min_lines, rdma_work_mode, *output_mem_size);

	return 0;
}
EXPORT_SYMBOL(get_afbc_data_plane_min_rdma_mem_size);

void inline saturn_write_fbcmem_regs(struct drm_plane_state *state, u32 rdma_id,
				     u32 module_base, volatile RDMA_PATH_X_REG *rdma_regs)
{
	struct drm_crtc_state *crtc_state = state->crtc->state;
	struct drm_crtc *crtc = crtc_state->crtc;
	struct ky_drm_private *priv = crtc->dev->dev_private;
	const struct ky_dpu_rdma *rdmas = to_ky_crtc_state(crtc_state)->rdmas;
	u32 size  = rdmas[rdma_id].fbcmem.size;// / FBCMEM_UNIT;
	u32 start = rdmas[rdma_id].fbcmem.start;// / FBCMEM_UNIT;
	bool map   = rdmas[rdma_id].fbcmem.map;

	write_to_cmdlist(priv, RDMA_PATH_X_REG, module_base, FBC_MEM_SIZE, map << 28 | start << 16 | size);

	return;
}

int saturn_cal_layer_fbcmem_size(struct drm_plane *plane, \
				   struct drm_plane_state *state)
{
	int ret = 0;
	//u32 adjust_fbcmem_size = 0;
	struct ky_plane_state *pstate = to_ky_plane_state(state);
	uint64_t modifier = pstate->state.fb->modifier;

	u32 drm_4cc_fmt = pstate->state.fb->format->format;
	unsigned int rotation = pstate->state.rotation;
	bool rot_90_or_270 = (rotation & DRM_MODE_ROTATE_90) || (rotation & DRM_MODE_ROTATE_270);
	u32 crop_x = pstate->state.src_x >> 16; //check
	u32 crop_y = pstate->state.src_y >> 16; //check
	u32 crop_w = pstate->state.src_w >> 16; //check
	u32 crop_h = pstate->state.src_h >> 16; //check
	u32 fbc_block_size = FBC_BLOCK_SIZE_LIMIT;
	struct drm_crtc_state *crtc_state = drm_atomic_get_new_crtc_state(state->state, state->crtc);
	const struct ky_dpu_rdma *rdmas = to_ky_crtc_state(crtc_state)->rdmas;
	u8 rdma_work_mode = rdmas[pstate->rdma_id].mode;
	u8 min_lines = 4; //TODO

	if (modifier == 0) { //raw data layer
		ret = get_raw_data_plane_rdma_mem_size(drm_4cc_fmt, rot_90_or_270, crop_w, &(pstate->fbcmem_size));
	} else { //afbc data layer
		ret = get_fbc_block_size_by_modifier(modifier, &fbc_block_size);
		if (ret < 0) {
			DRM_ERROR("FBC_MEM: failed to get fbc block size info for modifier = 0x%llx\n", modifier);
			return -1;
		}
		if (modifier && drm_4cc_fmt == DRM_FORMAT_YUV420_8BIT) { //video layer
			min_lines = 8; //dpu hardware request 8 line at least
		}
		ret = get_afbc_data_plane_min_rdma_mem_size(rdma_work_mode, drm_4cc_fmt, crop_x, crop_y,
			crop_w, crop_h, fbc_block_size, rot_90_or_270, min_lines, &(pstate->fbcmem_size));
	}
	if (ret < 0) {
		DRM_ERROR("FBC_MEM: faied to get plane mem size for plane: src_x = %u, scr_y = %u, src_h = %u, src_w = %u\n",
				pstate->state.src_x, pstate->state.src_y, pstate->state.src_h, pstate->state.src_w);
	}

	return ret;
}

int saturn_adjust_rdma_fbcmem(struct ky_hw_device *hwdev, \
				     struct ky_dpu_rdma *rdmas)
{
	int ret = -1;
	u8 index = 0;
	u8 sec_fbcmem_index = 0;
	u32 pri_fbcmem_size = 0;
	u32 sec_fbcmem_size = 0;
	u32* fbc_mems_left = NULL;
	u32 cur_rdma_fbcmem_size = 0;
	u8 rdma_nums = hwdev->rdma_nums;
	bool pre_odd_rdma_use_shared_fbc_mem = false;

	struct ky_dpu_fbcmem * fbcmem = NULL;
	for (index = 0; index < rdma_nums; index++) {
		fbcmem = &(rdmas[index].fbcmem);
		DRM_DEBUG("input rdmas[%u/%u]: mode = %d, start = %d, size = %d, map = %d\n",
			index, rdma_nums, rdmas[index].mode, fbcmem->start, fbcmem->size, fbcmem->map);
	}

	fbc_mems_left = kzalloc(sizeof(u32) * (rdma_nums/2), GFP_KERNEL);
	if (fbc_mems_left == NULL) {
		DRM_ERROR("FBC_MEM: No memory left!\n");
		goto free;
	}
	for (index = 0; index < rdma_nums/2; index++) {
		fbc_mems_left[index] = hwdev->fbcmem_sizes[index] / FBCMEM_UNIT;
		DRM_DEBUG("fbcmem_sizes[%u/%u] = %u, total fbc_mems_left = %u\n",
			index, rdma_nums/2, hwdev->fbcmem_sizes[index], fbc_mems_left[index]);
		if (fbc_mems_left[index] == 0) {
			DRM_ERROR("FBC_MEM: error fbcmem_sizes[%d] = %u\n", index, hwdev->fbcmem_sizes[index]);
			goto free;
		}
	}

	for (index = 0; index < rdma_nums; index++) {
		cur_rdma_fbcmem_size = rdmas[index].fbcmem.size;
		if (cur_rdma_fbcmem_size == 0) {
			DRM_DEBUG("return directly for rmda %u fbcmem size is 0\n", index);
			continue;
		}
		pri_fbcmem_size = hwdev->fbcmem_sizes[index/2] / FBCMEM_UNIT;

		if (index % 2 == 0) { //dma_id is even: 0, 2, 4, 6...
			DRM_DEBUG("rdma%u, cur_rdma_fbcmem_size = %u, pri_fbcmem_size = %u\n",
									index, cur_rdma_fbcmem_size, pri_fbcmem_size);
			if (cur_rdma_fbcmem_size > pri_fbcmem_size) {
				DRM_ERROR("FBC_MEM: rdma %d use %d byte, excess the size %d\n",
					index, cur_rdma_fbcmem_size, pri_fbcmem_size);
				goto free;
			}
			if (index > 0) {
				pre_odd_rdma_use_shared_fbc_mem = rdmas[index - 1].fbcmem.map;
				if (pre_odd_rdma_use_shared_fbc_mem) {
					DRM_ERROR("FBC_MEM: both rdma %d %d use fbc memory\n", index, index - 1);
					goto free;
				}
			}
			rdmas[index].fbcmem.start = 0;
			rdmas[index].fbcmem.map = true;
			fbc_mems_left[index/2] -= cur_rdma_fbcmem_size;
			DRM_DEBUG("FBC_MEM: rdma: id = %d, size = %u, start = %u, map = %u, fbc_mems_left[%u] = %d\n",
				index, cur_rdma_fbcmem_size, rdmas[index].fbcmem.start, rdmas[index].fbcmem.map, index/2, fbc_mems_left[index/2]);
		} else { //odd rdma: rdma 1, 3, 5...
			if (cur_rdma_fbcmem_size <= fbc_mems_left[index/2]) { //not share fbc mem
				DRM_DEBUG("rdma%u, cur_rdma_fbcmem_size = %u, fbc_mems_left[%u] = %u\n",
					index, cur_rdma_fbcmem_size, index/2, fbc_mems_left[index/2]);
				rdmas[index].fbcmem.map = false;
				rdmas[index].fbcmem.start = pri_fbcmem_size - fbc_mems_left[index/2];
				rdmas[index].fbcmem.size = fbc_mems_left[index/2];//use all the mem left
				fbc_mems_left[index/2] = 0;
				DRM_DEBUG("FBC_MEM: rdma: id = %d, size = %u, actually size = %u, start = %u, map = %u, fbc_mems_left[%u] = 0\n",
					index, cur_rdma_fbcmem_size, rdmas[index].fbcmem.size, rdmas[index].fbcmem.start, rdmas[index].fbcmem.map, index/2);
			} else { //need share fbc mem
				sec_fbcmem_index = (index/2 + 1)%(rdma_nums/2);
				sec_fbcmem_size = hwdev->fbcmem_sizes[sec_fbcmem_index] / FBCMEM_UNIT;
				DRM_DEBUG("rdma%u, cur_rdma_fbcmem_size = %u, fbc_mems_left[%u] = %d, sec_fbcmem_size = %u\n",
					index, cur_rdma_fbcmem_size, index/2, fbc_mems_left[index/2], sec_fbcmem_size);
				if (cur_rdma_fbcmem_size > fbc_mems_left[index/2] + sec_fbcmem_size) {
					DRM_ERROR("FBC_MEM: rdma %d use %d mem size, but left %d\n",
						index, cur_rdma_fbcmem_size, fbc_mems_left[index/2] + sec_fbcmem_size);
					goto free;
				} else {
					if (index == rdma_nums - 1) { //last rdma id
						if (fbc_mems_left[0] != hwdev->fbcmem_sizes[0] / FBCMEM_UNIT) { //fbc mem0 has used
							DRM_ERROR("FBC_MEM: rdma %d can not use fbc mem 0 for it has been used\n", index);
							goto free;
						} else {
							rdmas[index].fbcmem.size = fbc_mems_left[index/2] + sec_fbcmem_size;
							cur_rdma_fbcmem_size = rdmas[index].fbcmem.size;
						}
					}
					fbc_mems_left[sec_fbcmem_index] -= (cur_rdma_fbcmem_size - fbc_mems_left[index/2]);
					DRM_DEBUG("rmda%u, fbc_mems_left[%u] = %d, fbc_mems_left[%u] = %u\n",
						index, index/2, fbc_mems_left[index/2], sec_fbcmem_index, fbc_mems_left[sec_fbcmem_index]);
					rdmas[index].fbcmem.start = pri_fbcmem_size - fbc_mems_left[index/2];
					rdmas[index].fbcmem.map = true;
					fbc_mems_left[index/2] = 0;

					fbcmem = &(rdmas[index].fbcmem);
					DRM_DEBUG("FBC_MEM: rdma: id = %d, actually size = %u, start = %u, map = %u\n",
						index, fbcmem->size, fbcmem->start, fbcmem->map);
				}
			}
		}
	}
	ret = 0;

free:
	if (fbc_mems_left) {
		kfree(fbc_mems_left);
	}

	return ret;
}

EXPORT_SYMBOL(saturn_adjust_rdma_fbcmem);

