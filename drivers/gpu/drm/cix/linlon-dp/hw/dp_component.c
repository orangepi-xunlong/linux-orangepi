// SPDX-License-Identifier: GPL-2.0
/*
 * (C) COPYRIGHT 2022-2023 Arm Technology (China) Co., Ltd.
 * ALL RIGHTS RESERVED
 *
 */
#include "dp_dev.h"
#include "linlondp_kms.h"
#include "linlondp_io.h"
#include "linlondp_framebuffer.h"
#include "linlondp_color_mgmt.h"

static void get_resources_id(u32 hw_id, u32 *pipe_id, u32 *comp_id)
{
	u32 id = BLOCK_INFO_BLK_ID(hw_id);
	u32 pipe = id;

	switch (BLOCK_INFO_BLK_TYPE(hw_id)) {
	case DP_BLK_TYPE_LPU_WB_LAYER:
		id = LINLONDP_COMPONENT_WB_LAYER;
		break;
	case DP_BLK_TYPE_CU_SPLITTER:
		id = LINLONDP_COMPONENT_SPLITTER;
		break;
	case DP_BLK_TYPE_CU_SCALER:
		pipe = id / DP_PIPELINE_MAX_SCALERS;
		id %= DP_PIPELINE_MAX_SCALERS;
		id += LINLONDP_COMPONENT_SCALER0;
		break;
	case DP_BLK_TYPE_CU:
		id += LINLONDP_COMPONENT_COMPIZ0;
		break;
	case DP_BLK_TYPE_LPU_LAYER:
		pipe = id / DP_PIPELINE_MAX_LAYERS;
		id %= DP_PIPELINE_MAX_LAYERS;
		id += LINLONDP_COMPONENT_LAYER0;
		break;
	case DP_BLK_TYPE_DOU_IPS:
		id += LINLONDP_COMPONENT_IPS0;
		break;
	case DP_BLK_TYPE_CU_MERGER:
		id = LINLONDP_COMPONENT_MERGER;
		break;
	case DP_BLK_TYPE_DOU:
		id = LINLONDP_COMPONENT_TIMING_CTRLR;
		break;
	default:
		id = 0xFFFFFFFF;
	}

	if (comp_id)
		*comp_id = id;

	if (pipe_id)
		*pipe_id = pipe;
}

static u32 get_valid_inputs(struct block_header *blk)
{
	u32 valid_inputs = 0, comp_id;
	int i;

	for (i = 0; i < PIPELINE_INFO_N_VALID_INPUTS(blk->pipeline_info); i++) {
		get_resources_id(blk->input_ids[i], NULL, &comp_id);
		if (comp_id == 0xFFFFFFFF)
			continue;
		valid_inputs |= BIT(comp_id);
	}

	return valid_inputs;
}

static void get_values_from_reg(void __iomem *reg, u32 offset, u32 count,
				u32 *val)
{
	u32 i, addr;

	for (i = 0; i < count; i++) {
		addr = offset + (i << 2);
		/* 0xA4 is WO register */
		if (addr != 0xA4)
			val[i] = linlondp_read32(reg, addr);
		else
			val[i] = 0xDEADDEAD;
	}
}

static void dump_block_header(struct seq_file *sf, void __iomem *reg)
{
	struct block_header hdr;
	u32 i, n_input, n_output;

	dp_read_block_header(reg, &hdr);
	seq_printf(sf, "BLOCK_INFO:\t\t0x%X\n", hdr.block_info);
	seq_printf(sf, "PIPELINE_INFO:\t\t0x%X\n", hdr.pipeline_info);

	n_output = PIPELINE_INFO_N_OUTPUTS(hdr.pipeline_info);
	n_input = PIPELINE_INFO_N_VALID_INPUTS(hdr.pipeline_info);

	for (i = 0; i < n_input; i++)
		seq_printf(sf, "VALID_INPUT_ID%u:\t0x%X\n", i,
			   hdr.input_ids[i]);

	for (i = 0; i < n_output; i++)
		seq_printf(sf, "OUTPUT_ID%u:\t\t0x%X\n", i, hdr.output_ids[i]);
}

/* On DP, we are using the global line size. From DP, every component have
 * a line size register to indicate the fifo size.
 */
static u32 __get_blk_line_size(struct dp_dev *dp, u32 __iomem *reg,
			       u32 max_default)
{
	if (!dp->periph_addr)
		max_default = linlondp_read32(reg, BLK_MAX_LINE_SIZE);

	return max_default;
}

static u32 get_blk_line_size(struct dp_dev *dp, u32 __iomem *reg)
{
	return __get_blk_line_size(dp, reg, dp->max_line_size);
}

static u32 to_rot_ctrl(u32 rot)
{
	u32 lr_ctrl = 0;

	switch (rot & DRM_MODE_ROTATE_MASK) {
	case DRM_MODE_ROTATE_0:
		lr_ctrl |= L_ROT(L_ROT_R0);
		break;
	case DRM_MODE_ROTATE_90:
		lr_ctrl |= L_ROT(L_ROT_R90);
		break;
	case DRM_MODE_ROTATE_180:
		lr_ctrl |= L_ROT(L_ROT_R180);
		break;
	case DRM_MODE_ROTATE_270:
		lr_ctrl |= L_ROT(L_ROT_R270);
		break;
	}

	if (rot & DRM_MODE_REFLECT_X)
		lr_ctrl |= L_HFLIP;
	if (rot & DRM_MODE_REFLECT_Y)
		lr_ctrl |= L_VFLIP;

	return lr_ctrl;
}

static u32 to_ad_ctrl(u64 modifier)
{
	u32 afbc_ctrl = AD_AEN;

	if (!modifier)
		return 0;

	if ((modifier & AFBC_FORMAT_MOD_BLOCK_SIZE_MASK) ==
	    AFBC_FORMAT_MOD_BLOCK_SIZE_32x8)
		afbc_ctrl |= AD_WB;

	if (modifier & AFBC_FORMAT_MOD_YTR)
		afbc_ctrl |= AD_YT;
	if (modifier & AFBC_FORMAT_MOD_SPLIT)
		afbc_ctrl |= AD_BS;
	if (modifier & AFBC_FORMAT_MOD_TILED)
		afbc_ctrl |= AD_TH;

	return afbc_ctrl;
}

static inline u32 to_dp_input_id(struct linlondp_component_state *st, int idx)
{
	struct linlondp_component_output *input = &st->inputs[idx];

	/* if input is not active, set hw input_id(0) to disable it */
	if (has_bit(idx, st->active_inputs))
		return input->component->hw_id + input->output_port;
	else
		return 0;
}

static void dp_layer_update_fb(struct linlondp_component *c,
			       struct linlondp_fb *kfb, dma_addr_t *addr)
{
	struct drm_framebuffer *fb = &kfb->base;
	const struct drm_format_info *info = fb->format;
	u32 __iomem *reg = c->reg;
	int block_h;

	if (info->num_planes > 2)
		linlondp_write64(reg, BLK_P2_PTR_LOW, addr[2]);

	if (info->num_planes > 1) {
		block_h = drm_format_info_block_height(info, 1);
		linlondp_write32(reg, BLK_P1_STRIDE, fb->pitches[1] * block_h);
		linlondp_write64(reg, BLK_P1_PTR_LOW, addr[1]);
	}

	block_h = drm_format_info_block_height(info, 0);
	linlondp_write32(reg, BLK_P0_STRIDE, fb->pitches[0] * block_h);
	linlondp_write64(reg, BLK_P0_PTR_LOW, addr[0]);
	linlondp_write32(reg, LAYER_FMT, kfb->format_caps->hw_id);
}

static void dp_layer_disable(struct linlondp_component *c)
{
	linlondp_write32_mask(c->reg, BLK_CONTROL, L_EN, 0);
}

static u32 dp_layer_update_color(struct drm_plane_state *st, u32 __iomem *reg,
				 struct linlondp_color_state *color_st,
				 u32 *mask)
{
	struct linlondp_plane_state *kplane_st = to_kplane_st(st);
	struct linlondp_coeffs_table *igamma = color_st->igamma;
	struct linlondp_coeffs_table *fgamma = color_st->fgamma;
	u32 ctrl = 0, v = 0;

	if (!kplane_st->color_mgmt_changed)
		return 0;

	*mask |= L_IT | L_R2R | L_FT;

	if (igamma) {
		linlondp_coeffs_update(igamma);
		ctrl |= L_IT;
		v = L_ITSEL(igamma->hw_id);
	}

	if (kplane_st->ctm) {
		u32 ctm_coeffs[LINLONDP_N_CTM_COEFFS];

		drm_ctm_to_coeffs(kplane_st->ctm, ctm_coeffs);
		linlondp_write_group(reg, LAYER_RGB_RGB_COEFF0,
				     ARRAY_SIZE(ctm_coeffs), ctm_coeffs);
		ctrl |= L_R2R; /* enable RGB2RGB conversion */
	}

	if (fgamma) {
		linlondp_coeffs_update(fgamma);
		ctrl |= L_FT;
		v |= L_FTSEL(fgamma->hw_id);
	}

	linlondp_write32(reg, LAYER_LT_COEFFTAB, v);
	return ctrl;
}

static void dp_layer_update(struct linlondp_component *c,
			    struct linlondp_component_state *state)
{
	struct linlondp_layer_state *st = to_layer_st(state);
	struct drm_plane_state *plane_st = state->plane->state;
	struct drm_framebuffer *fb = plane_st->fb;
	struct linlondp_fb *kfb = to_kfb(fb);
	struct dp_dev *dp = c->pipeline->mdev->chip_data;

	u32 __iomem *reg = c->reg;
	u32 ctrl_mask = L_EN | L_ROT(L_ROT_R270) | L_HFLIP | L_VFLIP | L_TBU_EN;
	u32 ctrl = L_EN | to_rot_ctrl(st->rot);

	dp_layer_update_fb(c, kfb, st->addr);

	linlondp_write32(reg, AD_CONTROL, to_ad_ctrl(fb->modifier));
	if (fb->modifier) {
		u64 addr;

		linlondp_write32(reg, LAYER_AD_H_CROP,
				 HV_CROP(st->afbc_crop_l, st->afbc_crop_r));
		linlondp_write32(reg, LAYER_AD_V_CROP,
				 HV_CROP(st->afbc_crop_t, st->afbc_crop_b));
		/* afbc 1.2 wants payload, afbc 1.0/1.1 wants end_addr */
		if (fb->modifier & AFBC_FORMAT_MOD_TILED)
			addr = st->addr[0] + kfb->offset_payload;
		else
			addr = st->addr[0] + kfb->afbc_size - 1;

		linlondp_write32(reg, BLK_P1_PTR_LOW, lower_32_bits(addr));
		linlondp_write32(reg, BLK_P1_PTR_HIGH, upper_32_bits(addr));
	}

	if (fb->format->is_yuv) {
		u32 upsampling = 0;

		switch (kfb->format_caps->fourcc) {
		case DRM_FORMAT_YUYV:
			upsampling = fb->modifier ? LR_CHI422_BILINEAR :
						    LR_CHI422_REPLICATION;
			break;
		case DRM_FORMAT_UYVY:
			upsampling = LR_CHI422_REPLICATION;
			break;
		case DRM_FORMAT_NV12:
		case DRM_FORMAT_YUV420_8BIT:
		case DRM_FORMAT_YUV420_10BIT:
		case DRM_FORMAT_YUV420:
		case DRM_FORMAT_P010:
			/* these fmt support MPGE/JPEG both, here perfer JPEG*/
			upsampling = LR_CHI420_JPEG;
			break;
		case DRM_FORMAT_X0L2:
			upsampling = LR_CHI420_JPEG;
			break;
		default:
			break;
		}

		linlondp_write32(reg, LAYER_R_CONTROL, upsampling);
		linlondp_write_group(
			reg, LAYER_YUV_RGB_COEFF0, LINLONDP_N_YUV2RGB_COEFFS,
			linlondp_select_yuv2rgb_coeffs(plane_st->color_encoding,
						       plane_st->color_range));
	}

	linlondp_write32(reg, BLK_IN_SIZE, HV_SIZE(st->hsize, st->vsize));
	linlondp_write32(reg, LAYER_PALPHA, 0xffffffff);

	ctrl |= dp_layer_update_color(plane_st, reg, &st->color_st, &ctrl_mask);

	if (kfb->is_va && dp->integrates_tbu)
		ctrl |= L_TBU_EN;
	linlondp_write32_mask(reg, BLK_CONTROL, ctrl_mask, ctrl);
}

static void dp_layer_dump(struct linlondp_component *c, struct seq_file *sf)
{
	u32 v[15], i;
	bool rich, rgb2rgb;
	char *prefix;

	get_values_from_reg(c->reg, LAYER_INFO, 1, &v[14]);
	if (v[14] & 0x1) {
		rich = true;
		prefix = "LR_";
	} else {
		rich = false;
		prefix = "LS_";
	}

	rgb2rgb = !!(v[14] & L_INFO_CM);

	dump_block_header(sf, c->reg);

	seq_printf(sf, "%sLAYER_INFO:\t\t0x%X\n", prefix, v[14]);

	get_values_from_reg(c->reg, 0xD0, 1, v);
	seq_printf(sf, "%sCONTROL:\t\t0x%X\n", prefix, v[0]);
	if (rich) {
		get_values_from_reg(c->reg, 0xD4, 1, v);
		seq_printf(sf, "LR_RICH_CONTROL:\t0x%X\n", v[0]);
	}
	get_values_from_reg(c->reg, 0xD8, 4, v);
	seq_printf(sf, "%sFORMAT:\t\t0x%X\n", prefix, v[0]);
	seq_printf(sf, "%sIT_COEFFTAB:\t\t0x%X\n", prefix, v[1]);
	seq_printf(sf, "%sIN_SIZE:\t\t0x%X\n", prefix, v[2]);
	seq_printf(sf, "%sPALPHA:\t\t0x%X\n", prefix, v[3]);

	get_values_from_reg(c->reg, 0x100, 3, v);
	seq_printf(sf, "%sP0_PTR_LOW:\t\t0x%X\n", prefix, v[0]);
	seq_printf(sf, "%sP0_PTR_HIGH:\t\t0x%X\n", prefix, v[1]);
	seq_printf(sf, "%sP0_STRIDE:\t\t0x%X\n", prefix, v[2]);

	get_values_from_reg(c->reg, 0x110, 2, v);
	seq_printf(sf, "%sP1_PTR_LOW:\t\t0x%X\n", prefix, v[0]);
	seq_printf(sf, "%sP1_PTR_HIGH:\t\t0x%X\n", prefix, v[1]);
	if (rich) {
		get_values_from_reg(c->reg, 0x118, 1, v);
		seq_printf(sf, "LR_P1_STRIDE:\t\t0x%X\n", v[0]);

		get_values_from_reg(c->reg, 0x120, 2, v);
		seq_printf(sf, "LR_P2_PTR_LOW:\t\t0x%X\n", v[0]);
		seq_printf(sf, "LR_P2_PTR_HIGH:\t\t0x%X\n", v[1]);

		get_values_from_reg(c->reg, 0x130, 12, v);
		for (i = 0; i < 12; i++)
			seq_printf(sf, "LR_YUV_RGB_COEFF%u:\t0x%X\n", i, v[i]);
	}

	if (rgb2rgb) {
		get_values_from_reg(c->reg, LAYER_RGB_RGB_COEFF0, 12, v);
		for (i = 0; i < 12; i++)
			seq_printf(sf, "LS_RGB_RGB_COEFF%u:\t0x%X\n", i, v[i]);
	}

	get_values_from_reg(c->reg, 0x160, 3, v);
	seq_printf(sf, "%sAD_CONTROL:\t\t0x%X\n", prefix, v[0]);
	seq_printf(sf, "%sAD_H_CROP:\t\t0x%X\n", prefix, v[1]);
	seq_printf(sf, "%sAD_V_CROP:\t\t0x%X\n", prefix, v[2]);
}

static int dp_layer_validate(struct linlondp_component *c,
			     struct linlondp_component_state *state)
{
	struct linlondp_layer_state *st = to_layer_st(state);
	struct linlondp_layer *layer = to_layer(c);
	struct drm_plane_state *plane_st;
	struct drm_framebuffer *fb;
	u32 fourcc, line_sz, max_line_sz;

	plane_st =
		drm_atomic_get_new_plane_state(state->obj.state, state->plane);
	fb = plane_st->fb;
	fourcc = fb->format->format;

	if (drm_rotation_90_or_270(st->rot))
		line_sz = st->vsize - st->afbc_crop_t - st->afbc_crop_b;
	else
		line_sz = st->hsize - st->afbc_crop_l - st->afbc_crop_r;

	if (fb->modifier) {
		if ((fb->modifier & AFBC_FORMAT_MOD_BLOCK_SIZE_MASK) ==
		    AFBC_FORMAT_MOD_BLOCK_SIZE_32x8)
			max_line_sz = layer->line_sz;
		else
			max_line_sz = layer->line_sz / 2;

		if (line_sz > max_line_sz) {
			DRM_DEBUG_ATOMIC(
				"afbc request line_sz: %d exceed the max afbc line_sz: %d.\n",
				line_sz, max_line_sz);
			return -EINVAL;
		}
	}

	if (fourcc == DRM_FORMAT_YUV420_10BIT && line_sz > 2046 &&
	    (st->afbc_crop_l % 4)) {
		DRM_DEBUG_ATOMIC(
			"YUV420_10BIT input_hsize: %d exceed the max size 2046.\n",
			line_sz);
		return -EINVAL;
	}

	if (fourcc == DRM_FORMAT_X0L2 && line_sz > 2046 && (st->addr[0] % 16)) {
		DRM_DEBUG_ATOMIC(
			"X0L2 input_hsize: %d exceed the max size 2046.\n",
			line_sz);
		return -EINVAL;
	}

	return 0;
}

static const struct linlondp_component_funcs dp_layer_funcs = {
	.validate = dp_layer_validate,
	.update = dp_layer_update,
	.disable = dp_layer_disable,
	.dump_register = dp_layer_dump,
};

static int dp_layer_init(struct dp_dev *dp, struct block_header *blk,
			 u32 __iomem *reg)
{
	struct linlondp_component *c;
	struct linlondp_layer *layer;
	u32 pipe_id, layer_id, layer_info;

	get_resources_id(blk->block_info, &pipe_id, &layer_id);
	c = linlondp_component_add(&dp->pipes[pipe_id]->base, sizeof(*layer),
				   layer_id,
				   BLOCK_INFO_INPUT_ID(blk->block_info),
				   &dp_layer_funcs, 0, get_valid_inputs(blk), 1,
				   reg, "LPU%d_LAYER%d", pipe_id, layer_id);
	if (IS_ERR(c)) {
		DRM_ERROR("Failed to add layer component\n");
		return PTR_ERR(c);
	}

	layer = to_layer(c);
	layer_info = linlondp_read32(reg, LAYER_INFO);

	if (layer_info & L_INFO_RF) {
		layer->layer_type = LINLONDP_FMT_RICH_LAYER;
		layer->color_mgr.igamma_mgr = dp->it_mgr;
	} else {
		layer->layer_type = LINLONDP_FMT_SIMPLE_LAYER;
		layer->color_mgr.igamma_mgr = dp->it_s_mgr;
	}

	if (layer_info & L_INFO_CM) {
		layer->color_mgr.has_ctm = true;
		layer->color_mgr.fgamma_mgr = dp->ft_mgr;
	}

	if (!dp->periph_addr) {
		/* DP or newer product */
		layer->line_sz = linlondp_read32(reg, BLK_MAX_LINE_SIZE);
		layer->yuv_line_sz = L_INFO_YUV_MAX_LINESZ(layer_info);
	} else if (dp->max_line_size > 2048) {
		/* DP 4K */
		layer->line_sz = dp->max_line_size;
		layer->yuv_line_sz = layer->line_sz / 2;
	} else {
		/* DP 2K */
		if (layer->layer_type == LINLONDP_FMT_RICH_LAYER) {
			/* rich layer is 4K configuration */
			layer->line_sz = dp->max_line_size * 2;
			layer->yuv_line_sz = layer->line_sz / 2;
		} else {
			layer->line_sz = dp->max_line_size;
			layer->yuv_line_sz = 0;
		}
	}

	set_range(&layer->hsize_in, 4, layer->line_sz);

	set_range(&layer->vsize_in, 4, dp->max_vsize);

	linlondp_write32(reg, LAYER_PALPHA, DP_PALPHA_DEF_MAP);

	layer->supported_rots = DRM_MODE_ROTATE_MASK | DRM_MODE_REFLECT_MASK;

	return 0;
}

static void dp_wb_layer_update(struct linlondp_component *c,
			       struct linlondp_component_state *state)
{
	struct linlondp_layer_state *st = to_layer_st(state);
	struct drm_connector_state *conn_st = state->wb_conn->state;
	struct linlondp_fb *kfb = to_kfb(conn_st->writeback_job->fb);
	struct dp_dev *dp = c->pipeline->mdev->chip_data;
	u32 ctrl = L_EN | LW_OFM, mask = L_EN | LW_OFM | LW_TBU_EN;
	u32 __iomem *reg = c->reg;

	dp_layer_update_fb(c, kfb, st->addr);

	if (kfb->base.format->is_yuv) {
		struct linlondp_wb_connector_state *kc_state =
			to_kconn_st(conn_st);
		linlondp_write_group(
			reg, LAYER_WR_RGB_YUV_COEFF0, LINLONDP_N_RGB2YUV_COEFFS,
			linlondp_select_rgb2yuv_coeffs(kc_state->color_encoding,
						       kc_state->color_range));
	}

	if (kfb->is_va && dp->integrates_tbu)
		ctrl |= LW_TBU_EN;

	linlondp_write32(reg, BLK_IN_SIZE, HV_SIZE(st->hsize, st->vsize));
	linlondp_write32(reg, BLK_INPUT_ID0, to_dp_input_id(state, 0));
	linlondp_write32_mask(reg, BLK_CONTROL, mask, ctrl);
}

static void dp_wb_layer_dump(struct linlondp_component *c, struct seq_file *sf)
{
	u32 v[12], i;

	dump_block_header(sf, c->reg);

	get_values_from_reg(c->reg, 0x80, 1, v);
	seq_printf(sf, "LW_INPUT_ID0:\t\t0x%X\n", v[0]);

	get_values_from_reg(c->reg, 0xD0, 3, v);
	seq_printf(sf, "LW_CONTROL:\t\t0x%X\n", v[0]);
	seq_printf(sf, "LW_PROG_LINE:\t\t0x%X\n", v[1]);
	seq_printf(sf, "LW_FORMAT:\t\t0x%X\n", v[2]);

	get_values_from_reg(c->reg, 0xE0, 1, v);
	seq_printf(sf, "LW_IN_SIZE:\t\t0x%X\n", v[0]);

	for (i = 0; i < 2; i++) {
		get_values_from_reg(c->reg, 0x100 + i * 0x10, 3, v);
		seq_printf(sf, "LW_P%u_PTR_LOW:\t\t0x%X\n", i, v[0]);
		seq_printf(sf, "LW_P%u_PTR_HIGH:\t\t0x%X\n", i, v[1]);
		seq_printf(sf, "LW_P%u_STRIDE:\t\t0x%X\n", i, v[2]);
	}

	get_values_from_reg(c->reg, 0x130, 12, v);
	for (i = 0; i < 12; i++)
		seq_printf(sf, "LW_RGB_YUV_COEFF%u:\t0x%X\n", i, v[i]);
}

static void dp_wb_layer_disable(struct linlondp_component *c)
{
	linlondp_write32(c->reg, BLK_INPUT_ID0, 0);
	linlondp_write32_mask(c->reg, BLK_CONTROL, L_EN, 0);
}

static const struct linlondp_component_funcs dp_wb_layer_funcs = {
	.update = dp_wb_layer_update,
	.disable = dp_wb_layer_disable,
	.dump_register = dp_wb_layer_dump,
};

static int dp_wb_layer_init(struct dp_dev *dp, struct block_header *blk,
			    u32 __iomem *reg)
{
	struct linlondp_component *c;
	struct linlondp_layer *wb_layer;
	u32 pipe_id, layer_id;

	get_resources_id(blk->block_info, &pipe_id, &layer_id);

	c = linlondp_component_add(&dp->pipes[pipe_id]->base, sizeof(*wb_layer),
				   layer_id,
				   BLOCK_INFO_INPUT_ID(blk->block_info),
				   &dp_wb_layer_funcs, 1, get_valid_inputs(blk),
				   0, reg, "LPU%d_LAYER_WR", pipe_id);
	if (IS_ERR(c)) {
		DRM_ERROR("Failed to add wb_layer component\n");
		return PTR_ERR(c);
	}

	wb_layer = to_layer(c);
	wb_layer->layer_type = LINLONDP_FMT_WB_LAYER;
	wb_layer->line_sz = get_blk_line_size(dp, reg);
	wb_layer->yuv_line_sz = wb_layer->line_sz;

	set_range(&wb_layer->hsize_in, 64, wb_layer->line_sz);
	set_range(&wb_layer->vsize_in, 64, dp->max_vsize);

	return 0;
}

static void dp_component_disable(struct linlondp_component *c)
{
	u32 __iomem *reg = c->reg;
	u32 i;

	linlondp_write32(reg, BLK_CONTROL, 0);

	for (i = 0; i < c->max_active_inputs; i++) {
		linlondp_write32(reg, BLK_INPUT_ID0 + (i << 2), 0);

		/* Besides clearing the input ID to zero, DP compiz also has
		 * input enable bit in CU_INPUTx_CONTROL which need to be
		 * cleared.
		 */
		if (has_bit(c->id, LINLONDP_PIPELINE_COMPIZS))
			linlondp_write32(reg,
					 CU_INPUT0_CONTROL +
						 i * CU_PER_INPUT_REGS * 4,
					 CU_INPUT_CTRL_ALPHA(0xFF));
	}
}

static void compiz_enable_input(u32 __iomem *id_reg, u32 __iomem *cfg_reg,
				u32 input_hw_id,
				struct linlondp_compiz_input_cfg *cin)
{
	u32 ctrl = CU_INPUT_CTRL_EN;
	u8 blend = cin->pixel_blend_mode;

	if (blend == DRM_MODE_BLEND_PIXEL_NONE)
		ctrl |= CU_INPUT_CTRL_PAD;
	else if (blend == DRM_MODE_BLEND_PREMULTI)
		ctrl |= CU_INPUT_CTRL_PMUL;

	ctrl |= CU_INPUT_CTRL_ALPHA(cin->layer_alpha);

	linlondp_write32(id_reg, BLK_INPUT_ID0, input_hw_id);

	linlondp_write32(cfg_reg, CU_INPUT0_SIZE,
			 HV_SIZE(cin->hsize, cin->vsize));
	linlondp_write32(cfg_reg, CU_INPUT0_OFFSET,
			 HV_OFFSET(cin->hoffset, cin->voffset));
	linlondp_write32(cfg_reg, CU_INPUT0_CONTROL, ctrl);
}

static void dp_compiz_update(struct linlondp_component *c,
			     struct linlondp_component_state *state)
{
	struct linlondp_compiz_state *st = to_compiz_st(state);
	u32 __iomem *reg = c->reg;
	u32 __iomem *id_reg, *cfg_reg;
	u32 index;

	for (index = 0; index < state->component->max_active_inputs; index++) {
		if (has_bit(index, component_changed_inputs(state))) {
			id_reg = reg + index;
			cfg_reg = reg + index * CU_PER_INPUT_REGS;
			if (state->active_inputs & BIT(index)) {
				compiz_enable_input(id_reg, cfg_reg,
							to_dp_input_id(state, index),
							&st->cins[index]);
			} else {
				linlondp_write32(id_reg, BLK_INPUT_ID0, 0);
				linlondp_write32(cfg_reg, CU_INPUT0_CONTROL, 0);
			}
		}
	}

	linlondp_write32(reg, BLK_SIZE, HV_SIZE(st->hsize, st->vsize));
}

static void dp_compiz_dump(struct linlondp_component *c, struct seq_file *sf)
{
	u32 v[8], i;

	dump_block_header(sf, c->reg);

	get_values_from_reg(c->reg, 0x80, 5, v);
	for (i = 0; i < 5; i++)
		seq_printf(sf, "CU_INPUT_ID%u:\t\t0x%X\n", i, v[i]);

	get_values_from_reg(c->reg, 0xA0, 5, v);
	seq_printf(sf, "CU_IRQ_RAW_STATUS:\t0x%X\n", v[0]);
	seq_printf(sf, "CU_IRQ_CLEAR:\t\t0x%X\n", v[1]);
	seq_printf(sf, "CU_IRQ_MASK:\t\t0x%X\n", v[2]);
	seq_printf(sf, "CU_IRQ_STATUS:\t\t0x%X\n", v[3]);
	seq_printf(sf, "CU_STATUS:\t\t0x%X\n", v[4]);

	get_values_from_reg(c->reg, 0xD0, 2, v);
	seq_printf(sf, "CU_CONTROL:\t\t0x%X\n", v[0]);
	seq_printf(sf, "CU_SIZE:\t\t0x%X\n", v[1]);

	get_values_from_reg(c->reg, 0xDC, 1, v);
	seq_printf(sf, "CU_BG_COLOR:\t\t0x%X\n", v[0]);

	for (i = 0, v[4] = 0xE0; i < 5; i++, v[4] += 0x10) {
		get_values_from_reg(c->reg, v[4], 3, v);
		seq_printf(sf, "CU_INPUT%u_SIZE:\t\t0x%X\n", i, v[0]);
		seq_printf(sf, "CU_INPUT%u_OFFSET:\t0x%X\n", i, v[1]);
		seq_printf(sf, "CU_INPUT%u_CONTROL:\t0x%X\n", i, v[2]);
	}

	get_values_from_reg(c->reg, 0x130, 2, v);
	seq_printf(sf, "CU_USER_LOW:\t\t0x%X\n", v[0]);
	seq_printf(sf, "CU_USER_HIGH:\t\t0x%X\n", v[1]);
}

static const struct linlondp_component_funcs dp_compiz_funcs = {
	.update = dp_compiz_update,
	.disable = dp_component_disable,
	.dump_register = dp_compiz_dump,
};

static int dp_compiz_init(struct dp_dev *dp, struct block_header *blk,
			  u32 __iomem *reg)
{
	struct linlondp_component *c;
	struct linlondp_compiz *compiz;
	u32 pipe_id, comp_id;

	get_resources_id(blk->block_info, &pipe_id, &comp_id);

	c = linlondp_component_add(&dp->pipes[pipe_id]->base, sizeof(*compiz),
				   comp_id,
				   BLOCK_INFO_INPUT_ID(blk->block_info),
				   &dp_compiz_funcs, CU_NUM_INPUT_IDS,
				   get_valid_inputs(blk), CU_NUM_OUTPUT_IDS,
				   reg, "CU%d", pipe_id);
	if (IS_ERR(c))
		return PTR_ERR(c);

	compiz = to_compiz(c);

	set_range(&compiz->hsize, 64, get_blk_line_size(dp, reg));
	set_range(&compiz->vsize, 64, dp->max_vsize);

	return 0;
}

static void dp_scaler_update_filter_lut(u32 __iomem *reg, u32 hsize_in,
					u32 vsize_in, u32 hsize_out,
					u32 vsize_out)
{
	u32 val = 0;

	if (hsize_in <= hsize_out)
		val |= 0x62;
	else if (hsize_in <= (hsize_out + hsize_out / 2))
		val |= 0x63;
	else if (hsize_in <= hsize_out * 2)
		val |= 0x64;
	else if (hsize_in <= hsize_out * 2 + (hsize_out * 3) / 4)
		val |= 0x65;
	else
		val |= 0x66;

	if (vsize_in <= vsize_out)
		val |= SC_VTSEL(0x6A);
	else if (vsize_in <= (vsize_out + vsize_out / 2))
		val |= SC_VTSEL(0x6B);
	else if (vsize_in <= vsize_out * 2)
		val |= SC_VTSEL(0x6C);
	else if (vsize_in <= vsize_out * 2 + vsize_out * 3 / 4)
		val |= SC_VTSEL(0x6D);
	else
		val |= SC_VTSEL(0x6E);

	linlondp_write32(reg, SC_COEFFTAB, val);
}

static void dp_scaler_update(struct linlondp_component *c,
			     struct linlondp_component_state *state)
{
	struct linlondp_scaler_state *st = to_scaler_st(state);
	u32 __iomem *reg = c->reg;
	u32 init_ph, delta_ph, ctrl;

	dp_scaler_update_filter_lut(reg, st->hsize_in, st->vsize_in,
				    st->hsize_out, st->vsize_out);

	linlondp_write32(reg, BLK_IN_SIZE, HV_SIZE(st->hsize_in, st->vsize_in));

	//right_part flag means crop done by initphase
	if ((st->right_part) && (st->hsize_in != st->hsize_out)) {
		if (st->en_img_enhancement) {
			// 1 pixel for image enhancement
			linlondp_write32(
				reg, SC_OUT_SIZE,
				HV_SIZE(st->hsize_out - st->left_crop + 1,
					st->vsize_out));
			linlondp_write32(reg, SC_H_CROP,
					 HV_CROP(1, st->right_crop));
		} else {
			linlondp_write32(reg, SC_OUT_SIZE,
					 HV_SIZE(st->hsize_out - st->left_crop,
						 st->vsize_out));
			linlondp_write32(reg, SC_H_CROP,
					 HV_CROP(0, st->right_crop));
		}
	} else {
		linlondp_write32(reg, SC_OUT_SIZE,
				 HV_SIZE(st->hsize_out, st->vsize_out));
		linlondp_write32(reg, SC_H_CROP,
				 HV_CROP(st->left_crop, st->right_crop));
	}
	/*
	 *linlondp_write32(reg, SC_OUT_SIZE, HV_SIZE(st->hsize_out, st->vsize_out));
	 *linlondp_write32(reg, SC_H_CROP, HV_CROP(st->left_crop, st->right_crop));
	 */
	/* for right part, HW only sample the valid pixel which means the pixels
	 * in left_crop will be jumpped, and the first sample pixel is:
	 *
	 * dst_a = st->total_hsize_out - st->hsize_out + st->left_crop + 0.5;
	 *
	 * Then the corresponding texel in src is:
	 *
	 * h_delta_phase = st->total_hsize_in / st->total_hsize_out;
	 * src_a = dst_A * h_delta_phase;
	 *
	 * and h_init_phase is src_a deduct the real source start src_S;
	 *
	 * src_S = st->total_hsize_in - st->hsize_in;
	 * h_init_phase = src_a - src_S;
	 *
	 * And HW precision for the initial/delta_phase is 16:16 fixed point,
	 * the following is the simplified formula
	 */
	if (st->right_part) {
		u32 dst_a = st->total_hsize_out - st->hsize_out + st->left_crop;

		if (st->en_img_enhancement)
			dst_a -= 1;

		init_ph = ((st->total_hsize_in * (2 * dst_a + 1) -
			    2 * st->total_hsize_out *
				    (st->total_hsize_in - st->hsize_in))
			   << 15) /
			  st->total_hsize_out;
	} else {
		init_ph = (st->total_hsize_in << 15) / st->total_hsize_out;
	}

	linlondp_write32(reg, SC_H_INIT_PH, init_ph);

	delta_ph = (st->total_hsize_in << 16) / st->total_hsize_out;
	linlondp_write32(reg, SC_H_DELTA_PH, delta_ph);

	init_ph = (st->total_vsize_in << 15) / st->vsize_out;
	linlondp_write32(reg, SC_V_INIT_PH, init_ph);

	delta_ph = (st->total_vsize_in << 16) / st->vsize_out;
	linlondp_write32(reg, SC_V_DELTA_PH, delta_ph);

	ctrl = 0;
	ctrl |= st->en_scaling ? SC_CTRL_SCL : 0;
	ctrl |= st->en_alpha ? SC_CTRL_AP : 0;
	ctrl |= st->en_img_enhancement ? SC_CTRL_IENH : 0;
	/* If we use the hardware splitter we shouldn't set SC_CTRL_LS */
	if (st->en_split &&
	    state->inputs[0].component->id != LINLONDP_COMPONENT_SPLITTER)
		ctrl |= SC_CTRL_LS;

	linlondp_write32(reg, BLK_CONTROL, ctrl);
	linlondp_write32(reg, BLK_INPUT_ID0, to_dp_input_id(state, 0));
}

static void dp_scaler_dump(struct linlondp_component *c, struct seq_file *sf)
{
	u32 v[10];

	dump_block_header(sf, c->reg);

	get_values_from_reg(c->reg, 0x80, 1, v);
	seq_printf(sf, "SC_INPUT_ID0:\t\t0x%X\n", v[0]);

	get_values_from_reg(c->reg, 0xD0, 1, v);
	seq_printf(sf, "SC_CONTROL:\t\t0x%X\n", v[0]);

	get_values_from_reg(c->reg, 0xDC, 9, v);
	seq_printf(sf, "SC_COEFFTAB:\t\t0x%X\n", v[0]);
	seq_printf(sf, "SC_IN_SIZE:\t\t0x%X\n", v[1]);
	seq_printf(sf, "SC_OUT_SIZE:\t\t0x%X\n", v[2]);
	seq_printf(sf, "SC_H_CROP:\t\t0x%X\n", v[3]);
	seq_printf(sf, "SC_V_CROP:\t\t0x%X\n", v[4]);
	seq_printf(sf, "SC_H_INIT_PH:\t\t0x%X\n", v[5]);
	seq_printf(sf, "SC_H_DELTA_PH:\t\t0x%X\n", v[6]);
	seq_printf(sf, "SC_V_INIT_PH:\t\t0x%X\n", v[7]);
	seq_printf(sf, "SC_V_DELTA_PH:\t\t0x%X\n", v[8]);

	get_values_from_reg(c->reg, 0x130, 10, v);
	seq_printf(sf, "SC_ENH_LIMITS:\t\t0x%X\n", v[0]);
	seq_printf(sf, "SC_ENH_COEFF0:\t\t0x%X\n", v[1]);
	seq_printf(sf, "SC_ENH_COEFF1:\t\t0x%X\n", v[2]);
	seq_printf(sf, "SC_ENH_COEFF2:\t\t0x%X\n", v[3]);
	seq_printf(sf, "SC_ENH_COEFF3:\t\t0x%X\n", v[4]);
	seq_printf(sf, "SC_ENH_COEFF4:\t\t0x%X\n", v[5]);
	seq_printf(sf, "SC_ENH_COEFF5:\t\t0x%X\n", v[6]);
	seq_printf(sf, "SC_ENH_COEFF6:\t\t0x%X\n", v[7]);
	seq_printf(sf, "SC_ENH_COEFF7:\t\t0x%X\n", v[8]);
	seq_printf(sf, "SC_ENH_COEFF8:\t\t0x%X\n", v[9]);
}

static const struct linlondp_component_funcs dp_scaler_funcs = {
	.update = dp_scaler_update,
	.disable = dp_component_disable,
	.dump_register = dp_scaler_dump,
};

static int dp_scaler_init(struct dp_dev *dp, struct block_header *blk,
			  u32 __iomem *reg)
{
	struct linlondp_component *c;
	struct linlondp_scaler *scaler;
	u32 pipe_id, comp_id;

	get_resources_id(blk->block_info, &pipe_id, &comp_id);

	c = linlondp_component_add(&dp->pipes[pipe_id]->base, sizeof(*scaler),
				   comp_id,
				   BLOCK_INFO_INPUT_ID(blk->block_info),
				   &dp_scaler_funcs, 1, get_valid_inputs(blk),
				   1, reg, "CU%d_SCALER%d", pipe_id,
				   BLOCK_INFO_BLK_ID(blk->block_info));

	if (IS_ERR(c)) {
		DRM_ERROR("Failed to initialize scaler");
		return PTR_ERR(c);
	}

	scaler = to_scaler(c);
	set_range(&scaler->hsize, 4, __get_blk_line_size(dp, reg, 2048));
	set_range(&scaler->vsize, 4, 4096);
	scaler->max_downscaling = 6;
	scaler->max_upscaling = 64;
	scaler->scaling_split_overlap = 8;
	scaler->enh_split_overlap = 1;

	linlondp_write32(c->reg, BLK_CONTROL, 0);

	return 0;
}

static int dp_downscaling_clk_check(struct linlondp_pipeline *pipe,
				    struct drm_display_mode *mode,
				    unsigned long aclk_rate,
				    struct linlondp_data_flow_cfg *dflow)
{
	u32 h_in = dflow->in_w;
	u32 v_in = dflow->in_h;
	u32 v_out = dflow->out_h;
	u64 fraction, denominator;

	/* DP downscaling must satisfy the following equation
	 *
	 *   ACLK                   h_in * v_in
	 * ------- >= ---------------------------------------------
	 *  PXLCLK     (h_total - (1 + 2 * v_in / v_out)) * v_out
	 *
	 * In only horizontal downscaling situation, the right side should be
	 * multiplied by (h_total - 3) / (h_active - 3), then equation becomes
	 *
	 *   ACLK          h_in
	 * ------- >= ----------------
	 *  PXLCLK     (h_active - 3)
	 *
	 * To avoid precision lost the equation 1 will be convert to:
	 *
	 *   ACLK             h_in * v_in
	 * ------- >= -----------------------------------
	 *  PXLCLK     (h_total -1 ) * v_out -  2 * v_in
	 */
	if (v_in == v_out) {
		fraction = h_in;
		denominator = mode->hdisplay - 3;
	} else {
		fraction = h_in * v_in;
		denominator = (mode->htotal - 1) * v_out - 2 * v_in;
	}

	return aclk_rate * denominator >= mode->crtc_clock * 1000 * fraction ?
		       0 :
		       -EINVAL;
}

static void dp_splitter_update(struct linlondp_component *c,
			       struct linlondp_component_state *state)
{
	struct linlondp_splitter_state *st = to_splitter_st(state);
	u32 __iomem *reg = c->reg;

	linlondp_write32(reg, BLK_INPUT_ID0, to_dp_input_id(state, 0));
	linlondp_write32(reg, BLK_SIZE, HV_SIZE(st->hsize, st->vsize));
	linlondp_write32(reg, SP_OVERLAP_SIZE, st->overlap & 0x1FFF);
	linlondp_write32(reg, BLK_CONTROL, BLK_CTRL_EN);
}

static void dp_splitter_dump(struct linlondp_component *c, struct seq_file *sf)
{
	u32 v[3];

	dump_block_header(sf, c->reg);

	get_values_from_reg(c->reg, BLK_INPUT_ID0, 1, v);
	seq_printf(sf, "SP_INPUT_ID0:\t\t0x%X\n", v[0]);

	get_values_from_reg(c->reg, BLK_CONTROL, 3, v);
	seq_printf(sf, "SP_CONTROL:\t\t0x%X\n", v[0]);
	seq_printf(sf, "SP_SIZE:\t\t0x%X\n", v[1]);
	seq_printf(sf, "SP_OVERLAP_SIZE:\t0x%X\n", v[2]);
}

static const struct linlondp_component_funcs dp_splitter_funcs = {
	.update = dp_splitter_update,
	.disable = dp_component_disable,
	.dump_register = dp_splitter_dump,
};

static int dp_splitter_init(struct dp_dev *dp, struct block_header *blk,
			    u32 __iomem *reg)
{
	struct linlondp_component *c;
	struct linlondp_splitter *splitter;
	u32 pipe_id, comp_id;

	get_resources_id(blk->block_info, &pipe_id, &comp_id);

	c = linlondp_component_add(&dp->pipes[pipe_id]->base, sizeof(*splitter),
				   comp_id,
				   BLOCK_INFO_INPUT_ID(blk->block_info),
				   &dp_splitter_funcs, 1, get_valid_inputs(blk),
				   2, reg, "CU%d_SPLITTER", pipe_id);

	if (IS_ERR(c)) {
		DRM_ERROR("Failed to initialize splitter");
		return -1;
	}

	splitter = to_splitter(c);

	set_range(&splitter->hsize, 4, get_blk_line_size(dp, reg));
	set_range(&splitter->vsize, 4, dp->max_vsize);

	return 0;
}

static void dp_merger_update(struct linlondp_component *c,
			     struct linlondp_component_state *state)
{
	struct linlondp_merger_state *st = to_merger_st(state);
	u32 __iomem *reg = c->reg;
	u32 index;
	u32 input_id;

	for (index = 0; index < state->component->max_active_inputs; index++) {
		if (has_bit(index, component_changed_inputs(state))) {
			input_id = to_dp_input_id(state, index);
			linlondp_write32(reg, MG_INPUT_ID0 + index * 4, input_id);
		}
	}

	linlondp_write32(reg, MG_SIZE,
			 HV_SIZE(st->hsize_merged, st->vsize_merged));
	linlondp_write32(reg, BLK_CONTROL, BLK_CTRL_EN);
}

static void dp_merger_dump(struct linlondp_component *c, struct seq_file *sf)
{
	u32 v;

	dump_block_header(sf, c->reg);

	get_values_from_reg(c->reg, MG_INPUT_ID0, 1, &v);
	seq_printf(sf, "MG_INPUT_ID0:\t\t0x%X\n", v);

	get_values_from_reg(c->reg, MG_INPUT_ID1, 1, &v);
	seq_printf(sf, "MG_INPUT_ID1:\t\t0x%X\n", v);

	get_values_from_reg(c->reg, BLK_CONTROL, 1, &v);
	seq_printf(sf, "MG_CONTROL:\t\t0x%X\n", v);

	get_values_from_reg(c->reg, MG_SIZE, 1, &v);
	seq_printf(sf, "MG_SIZE:\t\t0x%X\n", v);
}

static const struct linlondp_component_funcs dp_merger_funcs = {
	.update = dp_merger_update,
	.disable = dp_component_disable,
	.dump_register = dp_merger_dump,
};

static int dp_merger_init(struct dp_dev *dp, struct block_header *blk,
			  u32 __iomem *reg)
{
	struct linlondp_component *c;
	struct linlondp_merger *merger;
	u32 pipe_id, comp_id;

	get_resources_id(blk->block_info, &pipe_id, &comp_id);

	c = linlondp_component_add(&dp->pipes[pipe_id]->base, sizeof(*merger),
				   comp_id,
				   BLOCK_INFO_INPUT_ID(blk->block_info),
				   &dp_merger_funcs, MG_NUM_INPUTS_IDS,
				   get_valid_inputs(blk), MG_NUM_OUTPUTS_IDS,
				   reg, "CU%d_MERGER", pipe_id);

	if (IS_ERR(c)) {
		DRM_ERROR("Failed to initialize merger.\n");
		return PTR_ERR(c);
	}

	merger = to_merger(c);

	set_range(&merger->hsize_merged, 4, __get_blk_line_size(dp, reg, 4032));
	set_range(&merger->vsize_merged, 4, 4096);

	return 0;
}

static void dp_improc_update(struct linlondp_component *c,
			     struct linlondp_component_state *state)
{
	struct drm_crtc_state *crtc_st = state->crtc->state;
	struct linlondp_improc_state *st = to_improc_st(state);
	struct dp_pipeline *pipe = to_dp_pipeline(c->pipeline);
	u32 __iomem *reg = c->reg;
	u32 index, mask = 0, ctrl = 0;
	u32 input_id;

	for (index = 0; index < state->component->max_active_inputs; index++) {
		if (has_bit(index, component_changed_inputs(state))) {
			input_id = to_dp_input_id(state, index);
			linlondp_write32(reg, BLK_INPUT_ID0 + index * 4, input_id);
		}
	}

	linlondp_write32(reg, BLK_SIZE, HV_SIZE(st->hsize, st->vsize));
	linlondp_write32(reg, IPS_DEPTH, st->color_depth);

	if (crtc_st->color_mgmt_changed) {
		mask |= IPS_CTRL_FT | IPS_CTRL_IT | IPS_CTRL_RGB;

		if (crtc_st->degamma_lut) {
			ctrl |= IPS_CTRL_IT; /* enable degamma */
		}

		if (crtc_st->gamma_lut) {
			linlondp_write_group(pipe->dou_ft_coeff_addr, FT_COEFF0,
					     LINLONDP_N_GAMMA_COEFFS,
					     st->fgamma_coeffs);
			ctrl |= IPS_CTRL_FT; /* enable gamma */
		}

		if (crtc_st->ctm) {
			linlondp_write_group(reg, IPS_RGB_RGB_COEFF0,
					     LINLONDP_N_CTM_COEFFS,
					     st->ctm_coeffs);
			ctrl |= IPS_CTRL_RGB; /* enable gamut */
		}
	}

	mask |= IPS_CTRL_IM | IPS_CTRL_SBS | IPS_CTRL_YUV | IPS_CTRL_CHD422 |
		IPS_CTRL_CHD420 | IPS_CTRL_DITH;

	/* config color format */
	if (st->color_format == DRM_COLOR_FORMAT_YCBCR420)
		ctrl |= IPS_CTRL_YUV | IPS_CTRL_CHD422 | IPS_CTRL_CHD420;
	else if (st->color_format == DRM_COLOR_FORMAT_YCBCR422)
		ctrl |= IPS_CTRL_YUV | IPS_CTRL_CHD422;
	else if (st->color_format == DRM_COLOR_FORMAT_YCBCR444)
		ctrl |= IPS_CTRL_YUV;

	/* config dither */
	if (pipe->base.en_dither)
		ctrl |= IPS_CTRL_DITH;

	//Set a default RGB to YUV(601 limited) matrix
	//It should be set according to spec from backend
	if (ctrl & IPS_CTRL_YUV) {
		linlondp_write_group(reg, IPS_RGB_YUV_COEFF0,
				     LINLONDP_N_RGB2YUV_COEFFS,
				     linlondp_select_rgb2yuv_coeffs(
					     DRM_COLOR_YCBCR_BT601,
					     DRM_COLOR_YCBCR_LIMITED_RANGE));
	}
	/* slave input enabled, means side by side */
	if (has_bit(1, state->active_inputs)) {
		ctrl |= IPS_CTRL_SBS;

		//SBS without dual link, image merge should be enabled
		if (c->pipeline->dual_link == FALSE)
			ctrl |= IPS_CTRL_IM;
	}
	linlondp_write32_mask(reg, BLK_CONTROL, mask, ctrl);
}

static void dp_improc_dump(struct linlondp_component *c, struct seq_file *sf)
{
	u32 v[12], i;

	dump_block_header(sf, c->reg);

	get_values_from_reg(c->reg, 0x80, 2, v);
	seq_printf(sf, "IPS_INPUT_ID0:\t\t0x%X\n", v[0]);
	seq_printf(sf, "IPS_INPUT_ID1:\t\t0x%X\n", v[1]);

	get_values_from_reg(c->reg, 0xC0, 1, v);
	seq_printf(sf, "IPS_INFO:\t\t0x%X\n", v[0]);

	get_values_from_reg(c->reg, 0xD0, 3, v);
	seq_printf(sf, "IPS_CONTROL:\t\t0x%X\n", v[0]);
	seq_printf(sf, "IPS_SIZE:\t\t0x%X\n", v[1]);
	seq_printf(sf, "IPS_DEPTH:\t\t0x%X\n", v[2]);

	get_values_from_reg(c->reg, 0x130, 12, v);
	for (i = 0; i < 12; i++)
		seq_printf(sf, "IPS_RGB_RGB_COEFF%u:\t0x%X\n", i, v[i]);

	get_values_from_reg(c->reg, 0x170, 12, v);
	for (i = 0; i < 12; i++)
		seq_printf(sf, "IPS_RGB_YUV_COEFF%u:\t0x%X\n", i, v[i]);
}

static const struct linlondp_component_funcs dp_improc_funcs = {
	.update = dp_improc_update,
	.disable = dp_component_disable,
	.dump_register = dp_improc_dump,
};

static int dp_improc_init(struct dp_dev *dp, struct block_header *blk,
			  u32 __iomem *reg)
{
	struct linlondp_component *c;
	struct linlondp_improc *improc;
	u32 pipe_id, comp_id, value;

	get_resources_id(blk->block_info, &pipe_id, &comp_id);

	c = linlondp_component_add(&dp->pipes[pipe_id]->base, sizeof(*improc),
				   comp_id,
				   BLOCK_INFO_INPUT_ID(blk->block_info),
				   &dp_improc_funcs, IPS_NUM_INPUT_IDS,
				   get_valid_inputs(blk), IPS_NUM_OUTPUT_IDS,
				   reg, "DOU%d_IPS", pipe_id);
	if (IS_ERR(c)) {
		DRM_ERROR("Failed to add improc component\n");
		return PTR_ERR(c);
	}

	improc = to_improc(c);
	improc->supported_color_depths = BIT(8) | BIT(10);
	improc->supported_color_formats = DRM_COLOR_FORMAT_RGB444 |
					  DRM_COLOR_FORMAT_YCBCR444 |
					  DRM_COLOR_FORMAT_YCBCR422;
	value = linlondp_read32(reg, BLK_INFO);
	if (value & IPS_INFO_CHD420)
		improc->supported_color_formats |= DRM_COLOR_FORMAT_YCBCR420;

	improc->supports_csc = true;
	improc->supports_gamma = true;

	return 0;
}

static void dp_timing_ctrlr_disable(struct linlondp_component *c)
{
	linlondp_write32_mask(c->reg, BLK_CONTROL, BS_CTRL_EN, 0);
}

static void dp_timing_ctrlr_update(struct linlondp_component *c,
				   struct linlondp_component_state *state)
{
	struct drm_crtc_state *crtc_st = state->crtc->state;
	struct linlondp_crtc_state *kcrtc_st = to_kcrtc_st(crtc_st);
	struct drm_display_mode *mode = &crtc_st->adjusted_mode;
	struct linlondp_timing_ctrlr_state *st = to_ctrlr_st(state);
	struct dp_pipeline *pipe = to_dp_pipeline(c->pipeline);
	u32 __iomem *reg = c->reg;
	u32 hactive, hfront_porch, hback_porch, hsync_len;
	u32 vactive, vfront_porch, vback_porch, vsync_len;
	u32 value;

	hactive = mode->crtc_hdisplay;
	hfront_porch = mode->crtc_hsync_start - mode->crtc_hdisplay;
	hsync_len = mode->crtc_hsync_end - mode->crtc_hsync_start;
	hback_porch = mode->crtc_htotal - mode->crtc_hsync_end;

	vactive = mode->crtc_vdisplay;
	vfront_porch = mode->crtc_vsync_start - mode->crtc_vdisplay;
	vsync_len = mode->crtc_vsync_end - mode->crtc_vsync_start;
	vback_porch = mode->crtc_vtotal - mode->crtc_vsync_end;

	/*
	 * Special setting for hactive with im and ppc function
	 * hactive has already been fixed up by linlondp_crtc_mode_fixup
	 * drm_display_mode is counted in pixel clock domain
	 */
	if (st->image_merged) {
		//FIXME, check 4 ppc later once hardware spec ready
		if (c->pipeline->pixel_per_cycle == 1)
			hactive = hactive / 2;
	} else {
		if (c->pipeline->pixel_per_cycle != 1)
			hactive = hactive * 2;
	}

	linlondp_write32(reg, BS_ACTIVESIZE, HV_SIZE(hactive, vactive));
	linlondp_write32(reg, BS_HINTERVALS,
			 BS_H_INTVALS(hfront_porch, hback_porch));
	linlondp_write32(reg, BS_VINTERVALS,
			 BS_V_INTVALS(vfront_porch, vback_porch));

	value = BS_SYNC_VSW(vsync_len) | BS_SYNC_HSW(hsync_len);
	value |= mode->flags & DRM_MODE_FLAG_PVSYNC ? BS_SYNC_VSP : 0;
	value |= mode->flags & DRM_MODE_FLAG_PHSYNC ? BS_SYNC_HSP : 0;
	linlondp_write32(reg, BS_SYNC, value);

	linlondp_write32(reg, BS_PROG_LINE, DP_DEFAULT_PREPRETCH_LINE - 1);
	linlondp_write32(reg, BS_PREFETCH_LINE, DP_DEFAULT_PREPRETCH_LINE);

	/* configure bs control register */
	value = BS_CTRL_EN;

	if (pipe->base.en_test_pattern)
		value |= BS_CTRL_TM;

	if (pipe->base.en_crc)
		value |= BS_CTRL_CRC;

	if (c->pipeline->dual_link) {
		linlondp_write32(reg, BS_DRIFT_TO, hfront_porch + 16);
		value |= BS_CTRL_DL;
	}

	value |= is_writeback_only(crtc_st) ? BS_CTRL_VD : BS_CTRL_VM;

	value |= kcrtc_st->en_protected_mode ? BS_CTRL_PM : 0;

	if (c->pipeline->pixel_per_cycle == 2)
		value |= BS_CTRL_PPC(1);
	else
		value |= BS_CTRL_PPC(0);

	linlondp_write32(reg, BLK_CONTROL, value);
}

static void dp_timing_ctrlr_dump(struct linlondp_component *c,
				 struct seq_file *sf)
{
	u32 v[8], i;

	dump_block_header(sf, c->reg);

	get_values_from_reg(c->reg, 0xC0, 1, v);
	seq_printf(sf, "BS_INFO:\t\t0x%X\n", v[0]);

	get_values_from_reg(c->reg, 0xD0, 8, v);
	seq_printf(sf, "BS_CONTROL:\t\t0x%X\n", v[0]);
	seq_printf(sf, "BS_PROG_LINE:\t\t0x%X\n", v[1]);
	seq_printf(sf, "BS_PREFETCH_LINE:\t0x%X\n", v[2]);
	seq_printf(sf, "BS_BG_COLOR:\t\t0x%X\n", v[3]);
	seq_printf(sf, "BS_ACTIVESIZE:\t\t0x%X\n", v[4]);
	seq_printf(sf, "BS_HINTERVALS:\t\t0x%X\n", v[5]);
	seq_printf(sf, "BS_VINTERVALS:\t\t0x%X\n", v[6]);
	seq_printf(sf, "BS_SYNC:\t\t0x%X\n", v[7]);

	get_values_from_reg(c->reg, 0x100, 3, v);
	seq_printf(sf, "BS_DRIFT_TO:\t\t0x%X\n", v[0]);
	seq_printf(sf, "BS_FRAME_TO:\t\t0x%X\n", v[1]);
	seq_printf(sf, "BS_TE_TO:\t\t0x%X\n", v[2]);

	get_values_from_reg(c->reg, 0x110, 3, v);
	for (i = 0; i < 3; i++)
		seq_printf(sf, "BS_T%u_INTERVAL:\t\t0x%X\n", i, v[i]);

	get_values_from_reg(c->reg, 0x120, 5, v);
	for (i = 0; i < 2; i++) {
		seq_printf(sf, "BS_CRC%u_LOW:\t\t0x%X\n", i, v[i << 1]);
		seq_printf(sf, "BS_CRC%u_HIGH:\t\t0x%X\n", i, v[(i << 1) + 1]);
	}
	seq_printf(sf, "BS_USER:\t\t0x%X\n", v[4]);
}

static const struct linlondp_component_funcs dp_timing_ctrlr_funcs = {
	.update = dp_timing_ctrlr_update,
	.disable = dp_timing_ctrlr_disable,
	.dump_register = dp_timing_ctrlr_dump,
};

static int dp_timing_ctrlr_init(struct dp_dev *dp, struct block_header *blk,
				u32 __iomem *reg)
{
	struct linlondp_component *c;
	struct linlondp_timing_ctrlr *ctrlr;
	u32 pipe_id, comp_id;

	get_resources_id(blk->block_info, &pipe_id, &comp_id);

	c = linlondp_component_add(&dp->pipes[pipe_id]->base, sizeof(*ctrlr),
				   LINLONDP_COMPONENT_TIMING_CTRLR,
				   BLOCK_INFO_INPUT_ID(blk->block_info),
				   &dp_timing_ctrlr_funcs, 1,
				   BIT(LINLONDP_COMPONENT_IPS0 + pipe_id),
				   BS_NUM_OUTPUT_IDS, reg, "DOU%d_BS", pipe_id);
	if (IS_ERR(c)) {
		DRM_ERROR("Failed to add display_ctrl component\n");
		return PTR_ERR(c);
	}

	ctrlr = to_ctrlr(c);

	ctrlr->supports_dual_link = dp->supports_dual_link;

	return 0;
}

static void dp_gamma_update(struct linlondp_coeffs_table *table)
{
	linlondp_write_group(table->reg, GLB_LT_COEFF_DATA, GLB_LT_COEFF_NUM,
			     table->coeffs);
}

static void dp_gamma_table_init(struct dp_dev *d71, struct block_header *blk,
				u32 __iomem *reg)
{
	struct linlondp_coeffs_manager *mgr = NULL;
	int hw_id = BLOCK_INFO_TYPE_ID(blk->block_info);

	switch (hw_id) {
	case 0x50:
		mgr = d71->it_s_mgr;
		break;
	case 0x52:
		mgr = d71->ft_mgr;
		break;
	default:
		mgr = d71->it_mgr;
		break;
	}

	linlondp_coeffs_add(mgr, hw_id, reg, dp_gamma_update);
}

int dp_probe_block(struct dp_dev *dp, struct block_header *blk,
		   u32 __iomem *reg)
{
	struct dp_pipeline *pipe;
	int blk_id = BLOCK_INFO_BLK_ID(blk->block_info);

	int err = 0;

	switch (BLOCK_INFO_BLK_TYPE(blk->block_info)) {
	case DP_BLK_TYPE_GCU:
		break;

	case DP_BLK_TYPE_LPU:
		pipe = dp->pipes[blk_id];
		pipe->lpu_addr = reg;
		break;

	case DP_BLK_TYPE_LPU_LAYER:
		err = dp_layer_init(dp, blk, reg);
		break;

	case DP_BLK_TYPE_LPU_WB_LAYER:
		err = dp_wb_layer_init(dp, blk, reg);
		break;

	case DP_BLK_TYPE_CU:
		pipe = dp->pipes[blk_id];
		pipe->cu_addr = reg;
		err = dp_compiz_init(dp, blk, reg);
		break;

	case DP_BLK_TYPE_CU_SCALER:
		err = dp_scaler_init(dp, blk, reg);
		break;

	case DP_BLK_TYPE_CU_SPLITTER:
		err = dp_splitter_init(dp, blk, reg);
		break;

	case DP_BLK_TYPE_CU_MERGER:
		err = dp_merger_init(dp, blk, reg);
		break;

	case DP_BLK_TYPE_DOU:
		pipe = dp->pipes[blk_id];
		pipe->dou_addr = reg;
		break;

	case DP_BLK_TYPE_DOU_IPS:
		err = dp_improc_init(dp, blk, reg);
		break;

	case DP_BLK_TYPE_DOU_FT_COEFF:
		pipe = dp->pipes[blk_id];
		pipe->dou_ft_coeff_addr = reg;
		break;

	case DP_BLK_TYPE_DOU_BS:
		err = dp_timing_ctrlr_init(dp, blk, reg);
		break;

	case DP_BLK_TYPE_GLB_LT_COEFF:
		dp_gamma_table_init(dp, blk, reg);
		break;

	case DP_BLK_TYPE_GLB_SCL_COEFF:
		dp->glb_scl_coeff_addr[blk_id] = reg;
		break;

	default:
		DRM_ERROR("Unknown block (block_info: 0x%x) is found\n",
			  blk->block_info);
		err = -EINVAL;
		break;
	}

	return err;
}

static void dp_gcu_dump(struct dp_dev *dp, struct seq_file *sf)
{
	u32 v[5];

	seq_puts(sf, "\n------ GCU ------\n");

	get_values_from_reg(dp->gcu_addr, 0, 3, v);
	seq_printf(sf, "GLB_ARCH_ID:\t\t0x%X\n", v[0]);
	seq_printf(sf, "GLB_CORE_ID:\t\t0x%X\n", v[1]);
	seq_printf(sf, "GLB_CORE_INFO:\t\t0x%X\n", v[2]);

	get_values_from_reg(dp->gcu_addr, 0x10, 1, v);
	seq_printf(sf, "GLB_IRQ_STATUS:\t\t0x%X\n", v[0]);

	get_values_from_reg(dp->gcu_addr, 0xA0, 5, v);
	seq_printf(sf, "GCU_IRQ_RAW_STATUS:\t0x%X\n", v[0]);
	seq_printf(sf, "GCU_IRQ_CLEAR:\t\t0x%X\n", v[1]);
	seq_printf(sf, "GCU_IRQ_MASK:\t\t0x%X\n", v[2]);
	seq_printf(sf, "GCU_IRQ_STATUS:\t\t0x%X\n", v[3]);
	seq_printf(sf, "GCU_STATUS:\t\t0x%X\n", v[4]);

	get_values_from_reg(dp->gcu_addr, 0xD0, 3, v);
	seq_printf(sf, "GCU_CONTROL:\t\t0x%X\n", v[0]);
	seq_printf(sf, "GCU_CONFIG_VALID0:\t0x%X\n", v[1]);
	seq_printf(sf, "GCU_CONFIG_VALID1:\t0x%X\n", v[2]);
}

static void dp_lpu_dump(struct dp_pipeline *pipe, struct seq_file *sf)
{
	u32 v[6];

	seq_printf(sf, "\n------ LPU%d ------\n", pipe->base.id);

	dump_block_header(sf, pipe->lpu_addr);

	get_values_from_reg(pipe->lpu_addr, 0xA0, 6, v);
	seq_printf(sf, "LPU_IRQ_RAW_STATUS:\t0x%X\n", v[0]);
	seq_printf(sf, "LPU_IRQ_CLEAR:\t\t0x%X\n", v[1]);
	seq_printf(sf, "LPU_IRQ_MASK:\t\t0x%X\n", v[2]);
	seq_printf(sf, "LPU_IRQ_STATUS:\t\t0x%X\n", v[3]);
	seq_printf(sf, "LPU_STATUS:\t\t0x%X\n", v[4]);
	seq_printf(sf, "LPU_TBU_STATUS:\t\t0x%X\n", v[5]);

	get_values_from_reg(pipe->lpu_addr, 0xC0, 1, v);
	seq_printf(sf, "LPU_INFO:\t\t0x%X\n", v[0]);

	get_values_from_reg(pipe->lpu_addr, 0xD0, 3, v);
	seq_printf(sf, "LPU_RAXI_CONTROL:\t0x%X\n", v[0]);
	seq_printf(sf, "LPU_WAXI_CONTROL:\t0x%X\n", v[1]);
	seq_printf(sf, "LPU_TBU_CONTROL:\t0x%X\n", v[2]);
}

static void dp_dou_dump(struct dp_pipeline *pipe, struct seq_file *sf)
{
	u32 v[5];

	seq_printf(sf, "\n------ DOU%d ------\n", pipe->base.id);

	dump_block_header(sf, pipe->dou_addr);

	get_values_from_reg(pipe->dou_addr, 0xA0, 5, v);
	seq_printf(sf, "DOU_IRQ_RAW_STATUS:\t0x%X\n", v[0]);
	seq_printf(sf, "DOU_IRQ_CLEAR:\t\t0x%X\n", v[1]);
	seq_printf(sf, "DOU_IRQ_MASK:\t\t0x%X\n", v[2]);
	seq_printf(sf, "DOU_IRQ_STATUS:\t\t0x%X\n", v[3]);
	seq_printf(sf, "DOU_STATUS:\t\t0x%X\n", v[4]);
}

static void dp_pipeline_dump(struct linlondp_pipeline *pipe,
			     struct seq_file *sf)
{
	struct dp_pipeline *dp_pipe = to_dp_pipeline(pipe);

	dp_lpu_dump(dp_pipe, sf);
	dp_dou_dump(dp_pipe, sf);
}

const struct linlondp_pipeline_funcs dp_pipeline_funcs = {
	.downscaling_clk_check = dp_downscaling_clk_check,
	.dump_register = dp_pipeline_dump,
};

void dp_dump(struct linlondp_dev *mdev, struct seq_file *sf)
{
	struct dp_dev *dp = mdev->chip_data;

	dp_gcu_dump(dp, sf);
}

static void dp_wait_flip_done(struct linlondp_dev *mdev)
{
	struct dp_dev *dp = mdev->chip_data;
	int ret;

	ret = dp_wait_cond(((linlondp_read32(dp->gcu_addr, GCU_CONFIG_VALID0) &
			     0x1) == 0x0),
			   50, 1000, 10000);
	if (ret)
		dev_err(mdev->dev, "%s timeout\n", __func__);
}

void dp_close_gop(struct linlondp_dev *mdev)
{
	u32 __iomem *base = mdev->reg_base;

	dev_info(mdev->dev, "%s enter\n", __func__);

	/* disable needs two phase, firstly do first phase */
	linlondp_write32(base, LPU0_LAYER0 + BLK_CONTROL,
			 0); // disbale lpu0_layer0
	linlondp_write32(base, LPU0_LAYER2 + BLK_CONTROL,
			 0); // disable lpu0_layer2
	linlondp_write32(base, CU0 + CU_INPUT_ID0, 0); // clear cu inputid0
	linlondp_write32(base, CU0 + CU_INPUT_ID1, 0); // clear cu inputid1
	linlondp_write32(base, CU0_SCALER0 + BLK_CONTROL,
			 0); // disable cu0_sclaer0
	linlondp_write32(base, CU0_SCALER1 + BLK_CONTROL,
			 0); // disable cu0_scaler1
	linlondp_write32(base, CU0_MERGER + BLK_CONTROL,
			 0); // disable cu0_merger
	linlondp_write32(base, GCU_CONFIG_VALID0, 1); // config valid

	dp_wait_flip_done(mdev);

	/* do second phase */
	linlondp_write32(base, DOU0_IPS + IPS_INPUT_ID0,
			 0); // clear dou0_ips inputid0
	linlondp_write32(base, DOU0_IPS + BLK_CONTROL, 0); // disable dou0_ips
	linlondp_write32(base, DOU0_BS + BLK_CONTROL, 0); // disable dou0_bs
	linlondp_write32(base, GCU_CONFIG_VALID0, 1); //config valid

	dp_wait_flip_done(mdev);
}

bool dp_gop_mode_changed(struct linlondp_dev *mdev, struct drm_display_mode *mode)
{
	u32 __iomem *base = mdev->reg_base;
	u32 hv, hporch, vporch;
	u32 hactive = mode->crtc_hdisplay;
	u32 hfront_porch = mode->crtc_hsync_start - mode->crtc_hdisplay;
	u32 hback_porch = mode->crtc_htotal - mode->crtc_hsync_end;
	u32 vactive = mode->crtc_vdisplay;
	u32 vfront_porch = mode->crtc_vsync_start - mode->crtc_vdisplay;
	u32 vback_porch = mode->crtc_vtotal - mode->crtc_vsync_end;

	dev_dbg(mdev->dev, "%s enter\n", __func__);

	hv = linlondp_read32(base, DOU0_BS + BS_ACTIVESIZE);
	hporch = linlondp_read32(base, DOU0_BS + BS_HINTERVALS);
	vporch = linlondp_read32(base, DOU0_BS + BS_VINTERVALS);

	if (hv != HV_SIZE(hactive, vactive)
		|| hporch != BS_H_INTVALS(hfront_porch, hback_porch)
		|| vporch != BS_V_INTVALS(vfront_porch, vback_porch)) {
		return true;
	}
	return false;
}
