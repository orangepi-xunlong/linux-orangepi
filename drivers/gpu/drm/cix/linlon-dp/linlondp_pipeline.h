/* SPDX-License-Identifier: GPL-2.0 */
/*
 * (C) COPYRIGHT 2022-2023 Arm Technology (China) Co., Ltd.
 * ALL RIGHTS RESERVED
 *
 */
#ifndef _LINLONDP_PIPELINE_H_
#define _LINLONDP_PIPELINE_H_

#include <linux/types.h>
#include <drm/drm_atomic.h>
#include <drm/drm_atomic_helper.h>
#include "linlondp_utils.h"
#include "linlondp_color_mgmt.h"

#define LINLONDP_MAX_PIPELINES 2
#define LINLONDP_PIPELINE_MAX_LAYERS 4
#define LINLONDP_PIPELINE_MAX_SCALERS 2
#define LINLONDP_COMPONENT_N_INPUTS 5

/* pipeline component IDs */
enum {
	LINLONDP_COMPONENT_LAYER0 = 0,
	LINLONDP_COMPONENT_LAYER1 = 1,
	LINLONDP_COMPONENT_LAYER2 = 2,
	LINLONDP_COMPONENT_LAYER3 = 3,
	LINLONDP_COMPONENT_WB_LAYER = 7, /* write back layer */
	LINLONDP_COMPONENT_SCALER0 = 8,
	LINLONDP_COMPONENT_SCALER1 = 9,
	LINLONDP_COMPONENT_SPLITTER = 12,
	LINLONDP_COMPONENT_MERGER = 14,
	LINLONDP_COMPONENT_COMPIZ0 = 16, /* compositor */
	LINLONDP_COMPONENT_COMPIZ1 = 17,
	LINLONDP_COMPONENT_IPS0 = 20, /* post image processor */
	LINLONDP_COMPONENT_IPS1 = 21,
	LINLONDP_COMPONENT_TIMING_CTRLR = 22, /* timing controller */
};

#define LINLONDP_PIPELINE_LAYERS                                           \
	(BIT(LINLONDP_COMPONENT_LAYER0) | BIT(LINLONDP_COMPONENT_LAYER1) | \
	 BIT(LINLONDP_COMPONENT_LAYER2) | BIT(LINLONDP_COMPONENT_LAYER3))

#define LINLONDP_PIPELINE_SCALERS \
	(BIT(LINLONDP_COMPONENT_SCALER0) | BIT(LINLONDP_COMPONENT_SCALER1))

#define LINLONDP_PIPELINE_COMPIZS \
	(BIT(LINLONDP_COMPONENT_COMPIZ0) | BIT(LINLONDP_COMPONENT_COMPIZ1))

#define LINLONDP_PIPELINE_IMPROCS \
	(BIT(LINLONDP_COMPONENT_IPS0) | BIT(LINLONDP_COMPONENT_IPS1))
struct linlondp_component;
struct linlondp_component_state;

/** linlondp_component_funcs - component control functions */
struct linlondp_component_funcs {
	/** @validate: optional,
	 * component may has special requirements or limitations, this function
	 * supply HW the ability to do the further HW specific check.
	 */
	int (*validate)(struct linlondp_component *c,
			struct linlondp_component_state *state);
	/** @update: update is a active update */
	void (*update)(struct linlondp_component *c,
		       struct linlondp_component_state *state);
	/** @disable: disable component */
	void (*disable)(struct linlondp_component *c);
	/** @dump_register: Optional, dump registers to seq_file */
	void (*dump_register)(struct linlondp_component *c,
			      struct seq_file *seq);
};

/**
 * struct linlondp_component
 *
 * struct linlondp_component describe the data flow capabilities for how to link a
 * component into the display pipeline.
 * all specified components are subclass of this structure.
 */
struct linlondp_component {
	/** @obj: treat component as private obj */
	struct drm_private_obj obj;
	/** @pipeline: the linlondp pipeline this component belongs to */
	struct linlondp_pipeline *pipeline;
	/** @name: component name */
	char name[32];
	/**
	 * @reg:
	 * component register base,
	 * which is initialized by chip and used by chip only
	 */
	u32 __iomem *reg;
	/** @id: component id */
	u32 id;
	/**
	 * @hw_id: component hw id,
	 * which is initialized by chip and used by chip only
	 */
	u32 hw_id;

	/**
	 * @max_active_inputs:
	 * @max_active_outputs:
	 *
	 * maximum number of inputs/outputs that can be active at the same time
	 * Note:
	 * the number isn't the bit number of @supported_inputs or
	 * @supported_outputs, but may be less than it, since component may not
	 * support enabling all @supported_inputs/outputs at the same time.
	 */
	u8 max_active_inputs;
	/** @max_active_outputs: maximum number of outputs */
	u8 max_active_outputs;
	/**
	 * @supported_inputs:
	 * @supported_outputs:
	 *
	 * bitmask of BIT(component->id) for the supported inputs/outputs,
	 * describes the possibilities of how a component is linked into a
	 * pipeline.
	 */
	u32 supported_inputs;
	/** @supported_outputs: bitmask of supported output componenet ids */
	u32 supported_outputs;

	/**
	 * @funcs: chip functions to access HW
	 */
	const struct linlondp_component_funcs *funcs;
};

/**
 * struct linlondp_component_output
 *
 * a component has multiple outputs, if want to know where the data
 * comes from, only know the component is not enough, we still need to know
 * its output port
 */
struct linlondp_component_output {
	/** @component: indicate which component the data comes from */
	struct linlondp_component *component;
	/**
	 * @output_port:
	 * the output port of the &linlondp_component_output.component
	 */
	u8 output_port;
};

/**
 * struct linlondp_component_state
 *
 * component_state is the data flow configuration of the component, and it's
 * the superclass of all specific component_state like @linlondp_layer_state,
 * @linlondp_scaler_state
 */
struct linlondp_component_state {
	/** @obj: tracking component_state by drm_atomic_state */
	struct drm_private_state obj;
	/** @component: backpointer to the component */
	struct linlondp_component *component;
	/**
	 * @binding_user:
	 * currently bound user, the user can be @crtc, @plane or @wb_conn,
	 * which is valid decided by @component and @inputs
	 *
	 * -  Layer: its user always is plane.
	 * -  compiz/improc/timing_ctrlr: the user is crtc.
	 * -  wb_layer: wb_conn;
	 * -  scaler: plane when input is layer, wb_conn if input is compiz.
	 */
	union {
		/** @crtc: backpointer for user crtc */
		struct drm_crtc *crtc;
		/** @plane: backpointer for user plane */
		struct drm_plane *plane;
		/** @wb_conn: backpointer for user wb_connector  */
		struct drm_connector *wb_conn;
		void *binding_user;
	};

	/**
	 * @active_inputs:
	 *
	 * active_inputs is bitmask of @inputs index
	 *
	 * -  active_inputs = changed_active_inputs | unchanged_active_inputs
	 * -  affected_inputs = old->active_inputs | new->active_inputs;
	 * -  disabling_inputs = affected_inputs ^ active_inputs;
	 * -  changed_inputs = disabling_inputs | changed_active_inputs;
	 *
	 * NOTE:
	 * changed_inputs doesn't include all active_input but only
	 * @changed_active_inputs, and this bitmask can be used in chip
	 * level for dirty update.
	 */
	u16 active_inputs;
	/** @changed_active_inputs: bitmask of the changed @active_inputs */
	u16 changed_active_inputs;
	/** @affected_inputs: bitmask for affected @inputs */
	u16 affected_inputs;
	/**
	 * @inputs:
	 *
	 * the specific inputs[i] only valid on BIT(i) has been set in
	 * @active_inputs, if not the inputs[i] is undefined.
	 */
	struct linlondp_component_output inputs[LINLONDP_COMPONENT_N_INPUTS];
};

static inline u16
component_disabling_inputs(struct linlondp_component_state *st)
{
	return st->affected_inputs ^ st->active_inputs;
}

static inline u16 component_changed_inputs(struct linlondp_component_state *st)
{
	return component_disabling_inputs(st) | st->changed_active_inputs;
}

#define to_comp(__c) (((__c) == NULL) ? NULL : &((__c)->base))
#define to_cpos(__c) ((struct linlondp_component **)&(__c))

struct linlondp_layer {
	struct linlondp_component base;
	/* accepted h/v input range before rotation */
	struct linlondp_range hsize_in, vsize_in;
	struct linlondp_color_manager color_mgr;
	u32 layer_type; /* RICH, SIMPLE or WB */
	u32 line_sz;
	u32 yuv_line_sz; /* maximum line size for YUV422 and YUV420 */
	u32 supported_rots;
	/* linlondp supports layer split which splits a whole image to two parts
	 * left and right and handle them by two individual layer processors
	 * Note: left/right are always according to the final display rect,
	 * not the source buffer.
	 */
	struct linlondp_layer *right;
	struct linlondp_layer *sbs_slave;
};

struct linlondp_layer_state {
	struct linlondp_component_state base;
	struct linlondp_color_state color_st;
	/* layer specific configuration state */
	u16 hsize, vsize;
	u32 rot;
	u16 afbc_crop_l;
	u16 afbc_crop_r;
	u16 afbc_crop_t;
	u16 afbc_crop_b;
	dma_addr_t addr[3];
};

struct linlondp_scaler {
	struct linlondp_component base;
	struct linlondp_range hsize, vsize;
	u32 max_upscaling;
	u32 max_downscaling;
	u8 scaling_split_overlap; /* split overlap for scaling */
	u8 enh_split_overlap; /* split overlap for image enhancement */
};

struct linlondp_scaler_state {
	struct linlondp_component_state base;
	u16 hsize_in, vsize_in;
	u16 hsize_out, vsize_out;
	u16 total_hsize_in, total_vsize_in;
	u16 total_hsize_out; /* total_xxxx are size before split */
	u16 left_crop, right_crop;
	u8 en_scaling : 1, en_alpha : 1, /* enable alpha processing */
		en_img_enhancement : 1, en_split : 1,
		right_part : 1; /* right part of split image */
};

struct linlondp_compiz {
	struct linlondp_component base;
	struct linlondp_range hsize, vsize;
};

struct linlondp_compiz_input_cfg {
	u16 hsize, vsize;
	u16 hoffset, voffset;
	u8 pixel_blend_mode, layer_alpha;
};

struct linlondp_compiz_state {
	struct linlondp_component_state base;
	/* composition size */
	u16 hsize, vsize;
	struct linlondp_compiz_input_cfg cins[LINLONDP_COMPONENT_N_INPUTS];
};

struct linlondp_merger {
	struct linlondp_component base;
	struct linlondp_range hsize_merged;
	struct linlondp_range vsize_merged;
};

struct linlondp_merger_state {
	struct linlondp_component_state base;
	u16 hsize_merged;
	u16 vsize_merged;
};

struct linlondp_splitter {
	struct linlondp_component base;
	struct linlondp_range hsize, vsize;
};

struct linlondp_splitter_state {
	struct linlondp_component_state base;
	u16 hsize, vsize;
	u16 overlap;
};

struct linlondp_improc {
	struct linlondp_component base;
	u32 supported_color_formats; /* DRM_RGB/YUV444/YUV420*/
	u32 supported_color_depths; /* BIT(8) | BIT(10)*/
	u8 supports_degamma : 1;
	u8 supports_csc : 1;
	u8 supports_gamma : 1;
};

struct linlondp_improc_state {
	struct linlondp_component_state base;
	u8 color_format, color_depth;
	u16 hsize, vsize;
	u32 fgamma_coeffs[LINLONDP_N_GAMMA_COEFFS];
	u32 ctm_coeffs[LINLONDP_N_CTM_COEFFS];
};

/* display timing controller */
struct linlondp_timing_ctrlr {
	struct linlondp_component base;
	u8 supports_dual_link : 1;
};

struct linlondp_timing_ctrlr_state {
	struct linlondp_component_state base;
	/* image merge or not, different rule for hactive setting */
	bool image_merged;
};

/* Why define A separated structure but not use plane_state directly ?
 * 1. Linlondp supports layer_split which means a plane_state can be split and
 *    handled by two layers, one layer only handle half of plane image.
 * 2. Fix up the user properties according to HW's capabilities, like user
 *    set rotation to R180, but HW only supports REFLECT_X+Y. the rot here is
 *    after drm_rotation_simplify()
 */
struct linlondp_data_flow_cfg {
	struct linlondp_component_output input;
	u16 in_x, in_y, in_w, in_h;
	u32 out_x, out_y, out_w, out_h;
	u16 total_in_h, total_in_w;
	u16 total_out_w;
	u16 left_crop, right_crop, overlap;
	u32 rot;
	int blending_zorder;
	u8 pixel_blend_mode, layer_alpha;
	u8 en_scaling : 1, en_img_enhancement : 1, en_split : 1, is_yuv : 1,
		right_part : 1; /* right part of display image if split enabled */
};

struct linlondp_pipeline_funcs {
	/* check if the aclk (main engine clock) can satisfy the clock
	 * requirements of the downscaling that specified by dflow
	 */
	int (*downscaling_clk_check)(struct linlondp_pipeline *pipe,
				     struct drm_display_mode *mode,
				     unsigned long aclk_rate,
				     struct linlondp_data_flow_cfg *dflow);
	/* dump_register: Optional, dump registers to seq_file */
	void (*dump_register)(struct linlondp_pipeline *pipe,
			      struct seq_file *sf);
};

/**
 * struct linlondp_pipeline
 *
 * Represent a complete display pipeline and hold all functional components.
 */
struct linlondp_pipeline {
	/** @obj: link pipeline as private obj of drm_atomic_state */
	struct drm_private_obj obj;
	/** @mdev: the parent linlondp_dev */
	struct linlondp_dev *mdev;
	/** @pxlclk: pixel clock */
	struct clk *pxlclk;
	/** @id: pipeline id */
	int id;
	/** @avail_comps: available components mask of pipeline */
	u32 avail_comps;
	/**
	 * @standalone_disabled_comps:
	 *
	 * When disable the pipeline, some components can not be disabled
	 * together with others, but need a sparated and standalone disable.
	 * The standalone_disabled_comps are the components which need to be
	 * disabled standalone, and this concept also introduce concept of
	 * two phase.
	 * phase 1: for disabling the common components.
	 * phase 2: for disabling the standalong_disabled_comps.
	 */
	u32 standalone_disabled_comps;
	/** @n_layers: the number of layer on @layers */
	int n_layers;
	/** @layers: the pipeline layers */
	struct linlondp_layer *layers[LINLONDP_PIPELINE_MAX_LAYERS];
	/** @n_scalers: the number of scaler on @scalers */
	int n_scalers;
	/** @scalers: the pipeline scalers */
	struct linlondp_scaler *scalers[LINLONDP_PIPELINE_MAX_SCALERS];
	/** @compiz: compositor */
	struct linlondp_compiz *compiz;
	/** @splitter: for split the compiz output to two half data flows */
	struct linlondp_splitter *splitter;
	/** @merger: merger */
	struct linlondp_merger *merger;
	/** @wb_layer: writeback layer */
	struct linlondp_layer *wb_layer;
	/** @improc: post image processor */
	struct linlondp_improc *improc;
	/** @ctrlr: timing controller */
	struct linlondp_timing_ctrlr *ctrlr;
	/** @funcs: chip private pipeline functions */
	const struct linlondp_pipeline_funcs *funcs;

	/** @of_node: pipeline dt node */
	struct device_node *of_node;
	/** @of_output_port: pipeline output port */
	struct device_node *of_output_port;
	/** @of_output_links: output connector device nodes */
	struct device_node *of_output_links[2];

	/** @fwnode_handle: pipeline acpi node(refer to local pipeline node) */
	struct fwnode_handle *fwnode;
	/** @fwnode_output_port: pipeline output port(refer to local port of pipeline) */
	struct fwnode_handle *fwnode_output_port;
	/** @of_output_links: output connector device nodes(refer to remote acpi_device not the remote endpoints) */
	struct fwnode_handle *fwnode_output_links[2];

	/** @dual_link: true if of_output_links[0] and [1] are both valid */
	bool dual_link;
	/** @pixel_per_cycle: pixel per clock cycle */
	u8 pixel_per_cycle, force_pixel_per_cycle;
	/** @en_test_pattern: true if use test pattern */
	bool en_test_pattern;
	/** @en_crc: true if enable crc */
	bool en_crc;
	/** @en_dither: true if enable dither */
	bool en_dither;
};

/**
 * struct linlondp_pipeline_state
 *
 * NOTE:
 * Unlike the pipeline, pipeline_state doesn’t gather any component_state
 * into it. It because all component will be managed by drm_atomic_state.
 */
struct linlondp_pipeline_state {
	/** @obj: tracking pipeline_state by drm_atomic_state */
	struct drm_private_state obj;
	/** @pipe: backpointer to the pipeline */
	struct linlondp_pipeline *pipe;
	/** @crtc: currently bound crtc */
	struct drm_crtc *crtc;
	/**
	 * @active_comps:
	 *
	 * bitmask - BIT(component->id) of active components
	 */
	u32 active_comps;
};

#define to_layer(c) container_of(c, struct linlondp_layer, base)
#define to_compiz(c) container_of(c, struct linlondp_compiz, base)
#define to_scaler(c) container_of(c, struct linlondp_scaler, base)
#define to_splitter(c) container_of(c, struct linlondp_splitter, base)
#define to_merger(c) container_of(c, struct linlondp_merger, base)
#define to_improc(c) container_of(c, struct linlondp_improc, base)
#define to_ctrlr(c) container_of(c, struct linlondp_timing_ctrlr, base)

#define to_layer_st(c) container_of(c, struct linlondp_layer_state, base)
#define to_compiz_st(c) container_of(c, struct linlondp_compiz_state, base)
#define to_scaler_st(c) container_of(c, struct linlondp_scaler_state, base)
#define to_splitter_st(c) container_of(c, struct linlondp_splitter_state, base)
#define to_merger_st(c) container_of(c, struct linlondp_merger_state, base)
#define to_improc_st(c) container_of(c, struct linlondp_improc_state, base)
#define to_ctrlr_st(c) container_of(c, struct linlondp_timing_ctrlr_state, base)

#define priv_to_comp_st(o) container_of(o, struct linlondp_component_state, obj)
#define priv_to_pipe_st(o) container_of(o, struct linlondp_pipeline_state, obj)

/* pipeline APIs */
struct linlondp_pipeline *
linlondp_pipeline_add(struct linlondp_dev *mdev, size_t size,
		      const struct linlondp_pipeline_funcs *funcs);
void linlondp_pipeline_destroy(struct linlondp_dev *mdev,
			       struct linlondp_pipeline *pipe);
struct linlondp_pipeline *
linlondp_pipeline_get_slave(struct linlondp_pipeline *master);
int linlondp_assemble_pipelines(struct linlondp_dev *mdev);
struct linlondp_component *
linlondp_pipeline_get_component(struct linlondp_pipeline *pipe, int id);
struct linlondp_component *
linlondp_pipeline_get_first_component(struct linlondp_pipeline *pipe,
				      u32 comp_mask);

void linlondp_pipeline_dump_register(struct linlondp_pipeline *pipe,
				     struct seq_file *sf);

/* component APIs */
extern __printf(10, 11) struct linlondp_component *linlondp_component_add(
	struct linlondp_pipeline *pipe, size_t comp_sz, u32 id, u32 hw_id,
	const struct linlondp_component_funcs *funcs, u8 max_active_inputs,
	u32 supported_inputs, u8 max_active_outputs, u32 __iomem *reg,
	const char *name_fmt, ...);

void linlondp_component_destroy(struct linlondp_dev *mdev,
				struct linlondp_component *c);

static inline struct linlondp_component *
linlondp_component_pickup_output(struct linlondp_component *c, u32 avail_comps)
{
	u32 avail_inputs = c->supported_outputs & (avail_comps);

	return linlondp_pipeline_get_first_component(c->pipeline, avail_inputs);
}

static inline const char *
linlondp_data_flow_msg(struct linlondp_data_flow_cfg *config)
{
	static char str[128];

	snprintf(str, sizeof(str),
		 "rot: %x src[x/y:%d/%d, w/h:%d/%d] disp[x/y:%d/%d, w/h:%d/%d]",
		 config->rot, config->in_x, config->in_y, config->in_w,
		 config->in_h, config->out_x, config->out_y, config->out_w,
		 config->out_h);

	return str;
}
struct linlondp_plane_state;
struct linlondp_crtc_state;
struct linlondp_crtc;

void pipeline_composition_size(struct linlondp_crtc_state *kcrtc_st,
			       bool side_by_side, u16 *hsize, u16 *vsize,
			       bool is_overlap);

int linlondp_build_layer_data_flow(struct linlondp_layer *layer,
				   struct linlondp_plane_state *kplane_st,
				   struct linlondp_crtc_state *kcrtc_st,
				   struct linlondp_data_flow_cfg *dflow);
int linlondp_build_wb_data_flow(struct linlondp_layer *wb_layer,
				struct drm_connector_state *conn_st,
				struct linlondp_crtc_state *kcrtc_st,
				struct linlondp_data_flow_cfg *dflow);
int linlondp_build_display_data_flow(struct linlondp_crtc *kcrtc,
				     struct linlondp_crtc_state *kcrtc_st);

int linlondp_build_layer_split_data_flow(struct linlondp_layer *left,
					 struct linlondp_plane_state *kplane_st,
					 struct linlondp_crtc_state *kcrtc_st,
					 struct linlondp_data_flow_cfg *dflow);
int linlondp_build_layer_sbs_data_flow(struct linlondp_layer *layer,
				       struct linlondp_plane_state *kplane_st,
				       struct linlondp_crtc_state *kcrtc_st,
				       struct linlondp_data_flow_cfg *dflow);
int linlondp_build_wb_split_data_flow(struct linlondp_layer *wb_layer,
				      struct drm_connector_state *conn_st,
				      struct linlondp_crtc_state *kcrtc_st,
				      struct linlondp_data_flow_cfg *dflow);
int linlondp_build_wb_sbs_data_flow(struct linlondp_crtc *kcrtc,
				    struct drm_connector_state *conn_st,
				    struct linlondp_crtc_state *kcrtc_st,
				    struct linlondp_data_flow_cfg *wb_dflow);

int linlondp_release_unclaimed_resources(struct linlondp_pipeline *pipe,
					 struct linlondp_crtc_state *kcrtc_st);

struct linlondp_pipeline_state *
linlondp_pipeline_get_old_state(struct linlondp_pipeline *pipe,
				struct drm_atomic_state *state);
bool linlondp_pipeline_disable(struct linlondp_pipeline *pipe,
			       struct drm_atomic_state *old_state);
void linlondp_pipeline_update(struct linlondp_pipeline *pipe,
			      struct drm_atomic_state *old_state);

void linlondp_complete_data_flow_cfg(struct linlondp_layer *layer,
				     struct linlondp_data_flow_cfg *dflow,
				     struct drm_framebuffer *fb);

#endif /* _LINLONDP_PIPELINE_H_*/
