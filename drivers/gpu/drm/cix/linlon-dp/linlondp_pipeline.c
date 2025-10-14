// SPDX-License-Identifier: GPL-2.0
/*
 * (C) COPYRIGHT 2022-2023 Arm Technology (China) Co., Ltd.
 * ALL RIGHTS RESERVED
 *
 */
#include <linux/of.h>

#include <drm/drm_print.h>

#include "linlondp_dev.h"
#include "linlondp_pipeline.h"

/** linlondp_pipeline_add - Add a pipeline to &linlondp_dev */
struct linlondp_pipeline *
linlondp_pipeline_add(struct linlondp_dev *mdev, size_t size,
		      const struct linlondp_pipeline_funcs *funcs)
{
	struct linlondp_pipeline *pipe;

	if (mdev->n_pipelines + 1 > LINLONDP_MAX_PIPELINES) {
		DRM_ERROR("Exceed max support %d pipelines.\n",
			  LINLONDP_MAX_PIPELINES);
		return ERR_PTR(-ENOSPC);
	}

	if (size < sizeof(*pipe)) {
		DRM_ERROR("Request pipeline size too small.\n");
		return ERR_PTR(-EINVAL);
	}

	pipe = devm_kzalloc(mdev->dev, size, GFP_KERNEL);
	if (!pipe)
		return ERR_PTR(-ENOMEM);

	pipe->mdev = mdev;
	pipe->id = mdev->n_pipelines;
	pipe->funcs = funcs;

	mdev->pipelines[mdev->n_pipelines] = pipe;
	mdev->n_pipelines++;

	return pipe;
}

void linlondp_pipeline_destroy(struct linlondp_dev *mdev,
			       struct linlondp_pipeline *pipe)
{
	struct linlondp_component *c;
	int i;
	unsigned long avail_comps = pipe->avail_comps;

	for_each_set_bit(i, &avail_comps, 32) {
		c = linlondp_pipeline_get_component(pipe, i);
		linlondp_component_destroy(mdev, c);
	}
#if !IS_ENABLED(CONFIG_DRM_LINLONDP_CLOCK_FIXED)
	clk_put(pipe->pxlclk);
#endif
	if (!has_acpi_companion(mdev->dev)) {
		of_node_put(pipe->of_output_links[0]);
		of_node_put(pipe->of_output_links[1]);
		of_node_put(pipe->of_output_port);
		of_node_put(pipe->of_node);
	} else {
		fwnode_handle_put(pipe->fwnode_output_links[0]);
		fwnode_handle_put(pipe->fwnode_output_links[1]);
		fwnode_handle_put(pipe->fwnode_output_port);
		fwnode_handle_put(pipe->fwnode);
	}

	devm_kfree(mdev->dev, pipe);
}

static struct linlondp_component **
linlondp_pipeline_get_component_pos(struct linlondp_pipeline *pipe, int id)
{
	struct linlondp_dev *mdev = pipe->mdev;
	struct linlondp_pipeline *temp = NULL;
	struct linlondp_component **pos = NULL;

	switch (id) {
	case LINLONDP_COMPONENT_LAYER0:
	case LINLONDP_COMPONENT_LAYER1:
	case LINLONDP_COMPONENT_LAYER2:
	case LINLONDP_COMPONENT_LAYER3:
		pos = to_cpos(pipe->layers[id - LINLONDP_COMPONENT_LAYER0]);
		break;
	case LINLONDP_COMPONENT_WB_LAYER:
		pos = to_cpos(pipe->wb_layer);
		break;
	case LINLONDP_COMPONENT_COMPIZ0:
	case LINLONDP_COMPONENT_COMPIZ1:
		temp = mdev->pipelines[id - LINLONDP_COMPONENT_COMPIZ0];
		if (!temp) {
			DRM_ERROR("compiz-%d doesn't exist.\n", id);
			return NULL;
		}
		pos = to_cpos(temp->compiz);
		break;
	case LINLONDP_COMPONENT_SCALER0:
	case LINLONDP_COMPONENT_SCALER1:
		pos = to_cpos(pipe->scalers[id - LINLONDP_COMPONENT_SCALER0]);
		break;
	case LINLONDP_COMPONENT_SPLITTER:
		pos = to_cpos(pipe->splitter);
		break;
	case LINLONDP_COMPONENT_MERGER:
		pos = to_cpos(pipe->merger);
		break;
	case LINLONDP_COMPONENT_IPS0:
	case LINLONDP_COMPONENT_IPS1:
		temp = mdev->pipelines[id - LINLONDP_COMPONENT_IPS0];
		if (!temp) {
			DRM_ERROR("ips-%d doesn't exist.\n", id);
			return NULL;
		}
		pos = to_cpos(temp->improc);
		break;
	case LINLONDP_COMPONENT_TIMING_CTRLR:
		pos = to_cpos(pipe->ctrlr);
		break;
	default:
		pos = NULL;
		DRM_ERROR("Unknown pipeline resource ID: %d.\n", id);
		break;
	}

	return pos;
}

struct linlondp_component *
linlondp_pipeline_get_component(struct linlondp_pipeline *pipe, int id)
{
	struct linlondp_component **pos = NULL;
	struct linlondp_component *c = NULL;

	pos = linlondp_pipeline_get_component_pos(pipe, id);
	if (pos)
		c = *pos;

	return c;
}

struct linlondp_component *
linlondp_pipeline_get_first_component(struct linlondp_pipeline *pipe,
				      u32 comp_mask)
{
	struct linlondp_component *c = NULL;
	unsigned long comp_mask_local = (unsigned long)comp_mask;
	int id;

	id = find_first_bit(&comp_mask_local, 32);
	if (id < 32)
		c = linlondp_pipeline_get_component(pipe, id);

	return c;
}

static struct linlondp_component *
linlondp_component_pickup_input(struct linlondp_component *c, u32 avail_comps)
{
	u32 avail_inputs = c->supported_inputs & (avail_comps);

	return linlondp_pipeline_get_first_component(c->pipeline, avail_inputs);
}

/** linlondp_component_add - Add a component to &linlondp_pipeline */
struct linlondp_component *
linlondp_component_add(struct linlondp_pipeline *pipe, size_t comp_sz, u32 id,
		       u32 hw_id, const struct linlondp_component_funcs *funcs,
		       u8 max_active_inputs, u32 supported_inputs,
		       u8 max_active_outputs, u32 __iomem *reg,
		       const char *name_fmt, ...)
{
	struct linlondp_component **pos;
	struct linlondp_component *c;
	int idx, *num = NULL;

	if (max_active_inputs > LINLONDP_COMPONENT_N_INPUTS) {
		WARN(1, "please large LINLONDP_COMPONENT_N_INPUTS to %d.\n",
		     max_active_inputs);
		return ERR_PTR(-ENOSPC);
	}

	pos = linlondp_pipeline_get_component_pos(pipe, id);
	if (!pos || (*pos))
		return ERR_PTR(-EINVAL);

	if (has_bit(id, LINLONDP_PIPELINE_LAYERS)) {
		idx = id - LINLONDP_COMPONENT_LAYER0;
		num = &pipe->n_layers;
		if (idx != pipe->n_layers) {
			DRM_ERROR("please add Layer by id sequence.\n");
			return ERR_PTR(-EINVAL);
		}
	} else if (has_bit(id, LINLONDP_PIPELINE_SCALERS)) {
		idx = id - LINLONDP_COMPONENT_SCALER0;
		num = &pipe->n_scalers;
		if (idx != pipe->n_scalers) {
			DRM_ERROR("please add Scaler by id sequence.\n");
			return ERR_PTR(-EINVAL);
		}
	}

	c = devm_kzalloc(pipe->mdev->dev, comp_sz, GFP_KERNEL);
	if (!c)
		return ERR_PTR(-ENOMEM);

	c->id = id;
	c->hw_id = hw_id;
	c->reg = reg;
	c->pipeline = pipe;
	c->max_active_inputs = max_active_inputs;
	c->max_active_outputs = max_active_outputs;
	c->supported_inputs = supported_inputs;
	c->funcs = funcs;

	if (name_fmt) {
		va_list args;

		va_start(args, name_fmt);
		vsnprintf(c->name, sizeof(c->name), name_fmt, args);
		va_end(args);
	}

	if (num)
		*num = *num + 1;

	pipe->avail_comps |= BIT(c->id);
	*pos = c;

	return c;
}

void linlondp_component_destroy(struct linlondp_dev *mdev,
				struct linlondp_component *c)
{
	devm_kfree(mdev->dev, c);
}

static void linlondp_component_dump(struct linlondp_component *c)
{
	if (!c)
		return;

	DRM_DEBUG("    %s: ID %d-0x%08lx.\n", c->name, c->id, BIT(c->id));
	DRM_DEBUG("        max_active_inputs:%d, supported_inputs: 0x%08x.\n",
		  c->max_active_inputs, c->supported_inputs);
	DRM_DEBUG("        max_active_outputs:%d, supported_outputs: 0x%08x.\n",
		  c->max_active_outputs, c->supported_outputs);
}

static void linlondp_pipeline_dump(struct linlondp_pipeline *pipe)
{
	struct linlondp_component *c;
	int id;
	unsigned long avail_comps = pipe->avail_comps;

	DRM_INFO(
		"Pipeline-%d: n_layers: %d, n_scalers: %d, output: %s  %d PPC\n",
		pipe->id, pipe->n_layers, pipe->n_scalers,
		pipe->dual_link ? "dual-link" : "single-link",
		pipe->pixel_per_cycle);

	if (pipe->fwnode && is_acpi_data_node(pipe->fwnode)) {
		DRM_INFO("    pipe->fwnode: %s.\n",
			 pipe->fwnode->ops->get_name(pipe->fwnode) ?: "none");

		if (pipe->fwnode_output_port &&
		    is_acpi_data_node(pipe->fwnode_output_port)) {
			DRM_INFO(
				"    pipe->fwnode_output_port: %s.\n",
				pipe->fwnode_output_port->ops->get_name(pipe->fwnode_output_port) ?:
					"none");
		}

		if (pipe->fwnode_output_links[0] &&
		    is_acpi_device_node(pipe->fwnode_output_links[0])) {
			DRM_INFO(
				"    output_link[0]: %s.\n",
				pipe->fwnode_output_links[0]->ops->get_name(pipe->fwnode_output_links[0]) ?:
					"none");
		}

		if (pipe->fwnode_output_links[1] &&
		    is_acpi_device_node(pipe->fwnode_output_links[1])) {
			DRM_INFO(
				"    output_link[1]: %s.\n",
				pipe->fwnode_output_links[1]->ops->get_name(pipe->fwnode_output_links[1]) ?:
					"none");
		}
	} else {
		DRM_INFO("    output_link[0]: %s.\n",
			 pipe->of_output_links[0] ?
				 pipe->of_output_links[0]->full_name :
				 "none");
		DRM_INFO("    output_link[1]: %s.\n",
			 pipe->of_output_links[1] ?
				 pipe->of_output_links[1]->full_name :
				 "none");
	}

	for_each_set_bit(id, &avail_comps, 32) {
		c = linlondp_pipeline_get_component(pipe, id);

		linlondp_component_dump(c);
	}
}

static void linlondp_component_verify_inputs(struct linlondp_component *c)
{
	struct linlondp_pipeline *pipe = c->pipeline;
	struct linlondp_component *input;
	int id;
	unsigned long supported_inputs = c->supported_inputs;

	for_each_set_bit(id, &supported_inputs, 32) {
		input = linlondp_pipeline_get_component(pipe, id);
		if (!input) {
			c->supported_inputs &= ~(BIT(id));
			DRM_WARN(
				"Can not find input(ID-%d) for component: %s.\n",
				id, c->name);
			continue;
		}

		input->supported_outputs |= BIT(c->id);
	}
}

static struct linlondp_layer *
linlondp_get_layer_split_right_layer(struct linlondp_pipeline *pipe,
				     struct linlondp_layer *left)
{
	int index = left->base.id - LINLONDP_COMPONENT_LAYER0;
	int i;

	for (i = index + 1; i < pipe->n_layers; i++)
		if (left->layer_type == pipe->layers[i]->layer_type)
			return pipe->layers[i];
	return NULL;
}

static void linlondp_pipeline_assemble(struct linlondp_pipeline *pipe)
{
	struct linlondp_component *c;
	struct linlondp_layer *layer;
	int i, id;
	unsigned long avail_comps = pipe->avail_comps;

	for_each_set_bit(id, &avail_comps, 32) {
		c = linlondp_pipeline_get_component(pipe, id);
		linlondp_component_verify_inputs(c);
	}
	/* calculate right layer for the layer split */
	for (i = 0; i < pipe->n_layers; i++) {
		layer = pipe->layers[i];

		layer->right =
			linlondp_get_layer_split_right_layer(pipe, layer);
	}

	if (pipe->dual_link && !pipe->ctrlr->supports_dual_link) {
		pipe->dual_link = false;
		DRM_WARN(
			"PIPE-%d doesn't support dual-link, ignore DT dual-link configuration.\n",
			pipe->id);
	}
}

/* if pipeline_A accept another pipeline_B's component as input, treat
 * pipeline_B as slave of pipeline_A.
 */
struct linlondp_pipeline *
linlondp_pipeline_get_slave(struct linlondp_pipeline *master)
{
	struct linlondp_dev *mdev = master->mdev;
	struct linlondp_component *comp, *slave;
	u32 avail_inputs;

	/* on SBS, slave pipeline merge to master via image processor */
	if (mdev->side_by_side) {
		comp = &master->improc->base;
		avail_inputs = LINLONDP_PIPELINE_IMPROCS;
	} else {
		comp = &master->compiz->base;
		avail_inputs = LINLONDP_PIPELINE_COMPIZS;
	}

	slave = linlondp_component_pickup_input(comp, avail_inputs);
	return slave ? slave->pipeline : NULL;
}

static int linlondp_assemble_side_by_side(struct linlondp_dev *mdev)
{
	struct linlondp_pipeline *master, *slave;
	int i;

	if (!mdev->side_by_side)
		return 0;

	if (mdev->side_by_side_master >= mdev->n_pipelines) {
		DRM_ERROR("DT configured side by side master-%d is invalid.\n",
			  mdev->side_by_side_master);
		return -EINVAL;
	}

	master = mdev->pipelines[mdev->side_by_side_master];
	slave = linlondp_pipeline_get_slave(master);
	if (!slave || slave->n_layers != master->n_layers) {
		DRM_ERROR("Current HW doesn't support side by side.\n");
		return -EINVAL;
	}

	//SBS can work with image merge mode
	/*
	 * if (!master->dual_link) {
	 * DRM_DEBUG_ATOMIC("SBS can not work without dual link.\n");
	 * return -EINVAL;
	 * }
	 */
	for (i = 0; i < master->n_layers; i++)
		master->layers[i]->sbs_slave = slave->layers[i];

	return 0;
}

int linlondp_assemble_pipelines(struct linlondp_dev *mdev)
{
	struct linlondp_pipeline *pipe;
	int i;

	for (i = 0; i < mdev->n_pipelines; i++) {
		pipe = mdev->pipelines[i];

		linlondp_pipeline_assemble(pipe);
		linlondp_pipeline_dump(pipe);
	}

	return linlondp_assemble_side_by_side(mdev);
}

void linlondp_pipeline_dump_register(struct linlondp_pipeline *pipe,
				     struct seq_file *sf)
{
	struct linlondp_component *c;
	u32 id;
	unsigned long avail_comps;

	seq_printf(sf, "\n======== Pipeline-%d ==========\n", pipe->id);

	if (pipe->funcs && pipe->funcs->dump_register)
		pipe->funcs->dump_register(pipe, sf);

	avail_comps = pipe->avail_comps;
	for_each_set_bit(id, &avail_comps, 32) {
		c = linlondp_pipeline_get_component(pipe, id);

		seq_printf(sf, "\n------%s------\n", c->name);
		if (c->funcs->dump_register)
			c->funcs->dump_register(c, sf);
	}
}
