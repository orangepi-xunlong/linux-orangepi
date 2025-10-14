// SPDX-License-Identifier: GPL-2.0
/*
 * (C) COPYRIGHT 2022-2023 Arm Technology (China) Co., Ltd.
 * ALL RIGHTS RESERVED
 *
 */
#include <linux/io.h>
#include <linux/iommu.h>
#include <linux/of_device.h>
#include <linux/of_graph.h>
#include <linux/of_reserved_mem.h>
#include <linux/platform_device.h>
#include <linux/pm_runtime.h>
#include <linux/dma-mapping.h>
#ifdef CONFIG_DEBUG_FS
#include <linux/debugfs.h>
#include <linux/seq_file.h>
#endif

#include <drm/drm_print.h>

#include "linlondp_dev.h"

#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 0, 0)
struct fwnode_handle *
fwnode_graph_get_remote_node(const struct fwnode_handle *fwnode, u32 port_id,
			     u32 endpoint_id)
{
	struct fwnode_handle *endpoint = NULL;

	while ((endpoint = fwnode_graph_get_next_endpoint(fwnode, endpoint))) {
		struct fwnode_endpoint fwnode_ep;
		struct fwnode_handle *remote;
		int ret;

		ret = fwnode_graph_parse_endpoint(endpoint, &fwnode_ep);
		if (ret < 0)
			continue;

		if (fwnode_ep.port != port_id || fwnode_ep.id != endpoint_id)
			continue;

		remote = fwnode_graph_get_remote_port_parent(endpoint);
		if (!remote)
			return NULL;

		return fwnode_device_is_available(remote) ? remote : NULL;
	}

	return NULL;
}
#endif

static int linlondp_register_show(struct seq_file *sf, void *x)
{
	struct linlondp_dev *mdev = sf->private;
	int i;

	seq_puts(sf, "\n====== Linlondp register dump =========\n");

	pm_runtime_get_sync(mdev->dev);

	if (mdev->funcs->dump_register)
		mdev->funcs->dump_register(mdev, sf);

	for (i = 0; i < mdev->n_pipelines; i++)
		linlondp_pipeline_dump_register(mdev->pipelines[i], sf);

	pm_runtime_put(mdev->dev);

	return 0;
}

DEFINE_SHOW_ATTRIBUTE(linlondp_register);

#ifdef CONFIG_DEBUG_FS
static void linlondp_debugfs_init(struct linlondp_dev *mdev)
{
	char dir_name[20];

	if (!debugfs_initialized())
		return;

	sprintf(dir_name, "linlondp%d", mdev->id);

	mdev->debugfs_root = debugfs_create_dir(dir_name, NULL);
	debugfs_create_file("register", 0444, mdev->debugfs_root, mdev,
			    &linlondp_register_fops);
	debugfs_create_x16("err_verbosity", 0664, mdev->debugfs_root,
			   &mdev->err_verbosity);
}
#endif

static ssize_t core_id_show(struct device *dev, struct device_attribute *attr,
			    char *buf)
{
	struct linlondp_dev *mdev = dev_to_mdev(dev);

	return sysfs_emit(buf, "0x%08x\n", mdev->chip.core_id);
}
static DEVICE_ATTR_RO(core_id);

static ssize_t config_id_show(struct device *dev, struct device_attribute *attr,
			      char *buf)
{
	struct linlondp_dev *mdev = dev_to_mdev(dev);
	struct linlondp_pipeline *pipe = mdev->pipelines[0];
	union linlondp_config_id config_id;
	int i;

	memset(&config_id, 0, sizeof(config_id));

	config_id.max_line_sz = pipe->layers[0]->hsize_in.end;
	config_id.side_by_side = mdev->side_by_side;
	config_id.n_pipelines = mdev->n_pipelines;
	config_id.n_scalers = pipe->n_scalers;
	config_id.n_layers = pipe->n_layers;
	config_id.n_richs = 0;
	for (i = 0; i < pipe->n_layers; i++) {
		if (pipe->layers[i]->layer_type == LINLONDP_FMT_RICH_LAYER)
			config_id.n_richs++;
	}
	return sysfs_emit(buf, "0x%08x\n", config_id.value);
}
static DEVICE_ATTR_RO(config_id);

static ssize_t aclk_hz_show(struct device *dev, struct device_attribute *attr,
			    char *buf)
{
	struct linlondp_dev *mdev = dev_to_mdev(dev);

	return sysfs_emit(buf, "%lu\n", mdev->aclk_freq);
}
static DEVICE_ATTR_RO(aclk_hz);

static ssize_t aclk_freq_fixed_store(struct device *dev,
				     struct device_attribute *attr,
				     const char *buf, size_t count)
{
	struct linlondp_dev *mdev = dev_to_mdev(dev);
	long val;
	int err;

	err = kstrtol(buf, 10, &val);
	if (err)
		return err;

	if (val > 800000000) {
		pr_err("specified frequency %lu exceeds the upper limit\n",
		       val);
		return -EINVAL;
	}

	mdev->aclk_freq_fixed = val;
	pr_info("specify aclk frequency: %lu\n", val);

	return count;
}

static ssize_t aclk_freq_fixed_show(struct device *dev,
				    struct device_attribute *attr, char *buf)
{
	struct linlondp_dev *mdev = dev_to_mdev(dev);

	return sysfs_emit(buf, "%lu\n", mdev->aclk_freq_fixed);
}

static DEVICE_ATTR_RW(aclk_freq_fixed);

static ssize_t smart_aclk_freq_store(struct device *dev,
				     struct device_attribute *attr,
				     const char *buf, size_t count)
{
	struct linlondp_dev *mdev = dev_to_mdev(dev);
	long val;
	int err;

	err = kstrtol(buf, 10, &val);
	if (err)
		return err;

	mdev->smart_aclk_freq = !!val;

	pr_info("set smart_aclk_freq %lu\n", val);

	return count;
}

static ssize_t smart_aclk_freq_show(struct device *dev,
				    struct device_attribute *attr, char *buf)
{
	struct linlondp_dev *mdev = dev_to_mdev(dev);

	return sysfs_emit(buf, "%d\n", mdev->smart_aclk_freq ? 1 : 0);
}

static DEVICE_ATTR_RW(smart_aclk_freq);

static ssize_t test_pattern_store(struct device *dev,
				  struct device_attribute *attr,
				  const char *buf, size_t count)
{
	struct linlondp_dev *mdev = dev_to_mdev(dev);
	struct linlondp_pipeline *pipe0 = mdev->pipelines[0];
	struct linlondp_pipeline *pipe1 = mdev->pipelines[1];
	long val;
	int err;

	err = kstrtol(buf, 10, &val);
	if (err)
		return err;

	pr_info("%s, set pattern value: 0x%lx\n", __func__, val);

	pipe0->en_test_pattern = !!(val & 0x1);
	pipe1->en_test_pattern = !!((val >> 0x1) & 0x1);

	return count;
}

static ssize_t test_pattern_show(struct device *dev,
				 struct device_attribute *attr, char *buf)
{
	long val;
	struct linlondp_dev *mdev = dev_to_mdev(dev);
	struct linlondp_pipeline *pipe0 = mdev->pipelines[0];
	struct linlondp_pipeline *pipe1 = mdev->pipelines[1];

	if (pipe0->en_test_pattern)
		val = 0x1;
	else
		val = 0x0;

	if (pipe1->en_test_pattern)
		val |= 0x2;
	else
		val &= 0x1;

	return sysfs_emit(buf, "0x%lx\n", val);
}

static DEVICE_ATTR_RW(test_pattern);

static ssize_t crc_enable_store(struct device *dev,
				struct device_attribute *attr, const char *buf,
				size_t count)
{
	struct linlondp_dev *mdev = dev_to_mdev(dev);
	struct linlondp_pipeline *pipe0 = mdev->pipelines[0];
	struct linlondp_pipeline *pipe1 = mdev->pipelines[1];
	long val;
	int err;

	err = kstrtol(buf, 10, &val);
	if (err)
		return err;

	pr_info("%s, set crc_enable value: 0x%lx\n", __func__, val);

	pipe0->en_crc = !!(val & 0x1);
	pipe1->en_crc = !!((val >> 0x1) & 0x1);

	return count;
}

static ssize_t crc_enable_show(struct device *dev,
			       struct device_attribute *attr, char *buf)
{
	long val;
	struct linlondp_dev *mdev = dev_to_mdev(dev);
	struct linlondp_pipeline *pipe0 = mdev->pipelines[0];
	struct linlondp_pipeline *pipe1 = mdev->pipelines[1];

	if (pipe0->en_crc)
		val = 0x1;
	else
		val = 0x0;

	if (pipe1->en_crc)
		val |= 0x2;
	else
		val &= 0x1;

	return sysfs_emit(buf, "0x%lx\n", val);
}

static DEVICE_ATTR_RW(crc_enable);

static ssize_t dither_enable_store(struct device *dev,
				   struct device_attribute *attr,
				   const char *buf, size_t count)
{
	struct linlondp_dev *mdev = dev_to_mdev(dev);
	struct linlondp_pipeline *pipe0 = mdev->pipelines[0];
	struct linlondp_pipeline *pipe1 = mdev->pipelines[1];
	long val;
	int err;

	err = kstrtol(buf, 10, &val);
	if (err)
		return err;

	pr_info("%s, set dither_enable value: 0x%lx\n", __func__, val);

	pipe0->en_dither = !!(val & 0x1);
	pipe1->en_dither = !!((val >> 0x1) & 0x1);

	return count;
}

static ssize_t dither_enable_show(struct device *dev,
				  struct device_attribute *attr, char *buf)
{
	long val;
	struct linlondp_dev *mdev = dev_to_mdev(dev);
	struct linlondp_pipeline *pipe0 = mdev->pipelines[0];
	struct linlondp_pipeline *pipe1 = mdev->pipelines[1];

	if (pipe0->en_dither)
		val = 0x1;
	else
		val = 0x0;

	if (pipe1->en_dither)
		val |= 0x2;
	else
		val &= 0x1;

	return sysfs_emit(buf, "0x%lx\n", val);
}

static DEVICE_ATTR_RW(dither_enable);

static struct attribute *linlondp_sysfs_entries[] = {
	&dev_attr_core_id.attr,
	&dev_attr_config_id.attr,
	&dev_attr_aclk_hz.attr,
	&dev_attr_aclk_freq_fixed.attr,
	&dev_attr_smart_aclk_freq.attr,
	&dev_attr_test_pattern.attr,
	&dev_attr_crc_enable.attr,
	&dev_attr_dither_enable.attr,
	NULL,
};

static struct attribute_group linlondp_sysfs_attr_group = {
	.attrs = linlondp_sysfs_entries,
};

static int linlondp_parse_pipe_dt(struct linlondp_pipeline *pipe)
{
	struct device_node *np = pipe->of_node;
	struct clk *clk;
#if !IS_ENABLED(CONFIG_DRM_LINLONDP_CLOCK_FIXED)
	clk = of_clk_get_by_name(np, "pxclk");
	if (IS_ERR(clk)) {
		DRM_ERROR("get pxclk for pipeline %d failed!\n", pipe->id);
		return PTR_ERR(clk);
	}
	pipe->pxlclk = clk;
#endif
	/* enum ports */
	pipe->of_output_links[0] =
		of_graph_get_remote_node(np, LINLONDP_OF_PORT_OUTPUT, 0);
	pipe->of_output_links[1] =
		of_graph_get_remote_node(np, LINLONDP_OF_PORT_OUTPUT, 1);
	pipe->of_output_port =
		of_graph_get_port_by_id(np, LINLONDP_OF_PORT_OUTPUT);

	pipe->dual_link = pipe->of_output_links[0] && pipe->of_output_links[1];

	pipe->pixel_per_cycle = 1;
	if (of_property_read_u8(np, "pixel_per_cycle",
				&pipe->force_pixel_per_cycle))
		pipe->force_pixel_per_cycle = 0;

	/* enable dither by default */
	pipe->en_dither = 1;

	return 0;
}

static int linlondp_parse_dt(struct device *dev, struct linlondp_dev *mdev)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct device_node *child, *np = dev->of_node;
	struct linlondp_pipeline *pipe;
	u32 pipe_id = U32_MAX;
	int ret = -1;

	mdev->irq = platform_get_irq(pdev, 0);
	if (mdev->irq < 0) {
		DRM_ERROR("could not get IRQ number.\n");
		return mdev->irq;
	}

	mdev->ip_rst = devm_reset_control_get(dev, "ip_reset");
	if (IS_ERR(mdev->ip_rst)) {
		dev_err(dev, "failed to get reset ip_reset\n");
		return PTR_ERR(mdev->ip_rst);
	}

	mdev->rcsu_rst = devm_reset_control_get(dev, "rcsu_reset");
	if (IS_ERR(mdev->rcsu_rst)) {
		dev_err(dev, "failed to get reset rcsu_reset\n");
		return PTR_ERR(mdev->rcsu_rst);
	}

	/* Get the optional framebuffer memory resource */
	ret = of_reserved_mem_device_init(dev);
	if (ret && ret != -ENODEV)
		return ret;

	for_each_available_child_of_node(np, child) {
		if (of_node_name_eq(child, "pipeline")) {
			of_property_read_u32(child, "reg", &pipe_id);
			if (pipe_id >= mdev->n_pipelines) {
				DRM_WARN(
					"Skip the redundant DT node: pipeline-%u.\n",
					pipe_id);
				continue;
			}
			mdev->pipelines[pipe_id]->of_node = of_node_get(child);
		}
	}

	for (pipe_id = 0; pipe_id < mdev->n_pipelines; pipe_id++) {
		pipe = mdev->pipelines[pipe_id];

		if (!pipe->of_node) {
			DRM_ERROR("Pipeline-%d doesn't have a DT node.\n",
				  pipe->id);
			return -EINVAL;
		}
		ret = linlondp_parse_pipe_dt(pipe);
		if (ret)
			return ret;
	}

	mdev->side_by_side = !of_property_read_u32(np, "side_by_side_master",
						   &mdev->side_by_side_master);

	ret = of_property_read_u32(np, "aclk_freq_fixed",
				   (u32 *)&mdev->aclk_freq_fixed);
	if (ret)
		mdev->aclk_freq_fixed = 0;

	if (of_find_property(np, "smart_aclk_freq", &ret))
		mdev->smart_aclk_freq = true;
	else
		mdev->smart_aclk_freq = false;

	ret = of_property_read_u32(np, "device-id", (u32 *)&mdev->id);
	if (ret)
		mdev->id = 0;

	return 0;
}

static struct fwnode_handle *
fwnode_graph_get_port_by_id(struct fwnode_handle *parent, u32 id)
{
	struct fwnode_handle *node, *port;

	node = fwnode_get_named_child_node(parent, "ports");
	if (node)
		parent = node;

	fwnode_for_each_child_node(parent, port) {
		u32 port_id = 0;

		if (is_acpi_data_node(port)) {
			if (strncmp(port->ops->get_name(port), "port", 4))
				continue;
		} else {
			continue;
		}

		fwnode_property_read_u32(port, "reg", &port_id);
		if (id == port_id)
			break;
	}

	fwnode_handle_put(node);

	return port;
}

static int linlondp_parse_pipe_acpi(struct linlondp_pipeline *pipe)
{
    struct fwnode_handle *np = pipe->fwnode;
#if !IS_ENABLED(CONFIG_DRM_LINLONDP_CLOCK_FIXED)
    struct clk *clk;
    clk = devm_clk_get(to_acpi_data_node(pipe->fwnode)->parent->dev,
		       pipe->fwnode->ops->get_name(pipe->fwnode));
    if (IS_ERR(clk)) {
        DRM_ERROR("get pxclk for pipeline %d failed!\n", pipe->id);
        return PTR_ERR(clk);
    }
    pipe->pxlclk = clk;
#endif
	/* enum ports */
	pipe->fwnode_output_links[0] =
		fwnode_graph_get_remote_node(np, LINLONDP_ACPI_PORT_OUTPUT, 0);

	pipe->fwnode_output_links[1] =
		fwnode_graph_get_remote_node(np, LINLONDP_ACPI_PORT_OUTPUT, 1);

	pipe->fwnode_output_port =
		fwnode_graph_get_port_by_id(np, LINLONDP_ACPI_PORT_OUTPUT);

	pipe->dual_link = pipe->fwnode_output_links[0] &&
			  pipe->fwnode_output_links[1];

	pipe->pixel_per_cycle = 1;
	if (device_property_read_u8(np->ops->get_parent(np)->dev,
				    "pixel_per_cycle",
				    &pipe->force_pixel_per_cycle))
		pipe->force_pixel_per_cycle = 0;

	return 0;
}

static int linlondp_parse_acpi(struct device *dev, struct linlondp_dev *mdev)
{
    struct platform_device *pdev = to_platform_device(dev);
    struct fwnode_handle *child, *np = dev->fwnode;

    struct linlondp_pipeline *pipe;
    u32 pipe_id = U32_MAX;
    const char *tmp_name = NULL;
    int ret = -1;

    mdev->irq  = platform_get_irq(pdev, 0);
    if (mdev->irq < 0) {
        DRM_ERROR("could not get IRQ number.\n");
        return mdev->irq;
    }

    ret = 0;
    fwnode_for_each_child_node(np, child) {
        tmp_name = child->ops->get_name(child);
        pr_info("linlondp_parse_acpi node.name=%s\n", tmp_name);

        if (strncmp(tmp_name, "pipeline", 8))
            continue;

        fwnode_property_read_u32(child, "reg", &pipe_id);
        pr_info("linlondp_parse_acpi pipeId=%d, n_pipelines=%d\n", pipe_id, mdev->n_pipelines);
        if (pipe_id >= mdev->n_pipelines) {
            DRM_WARN("Skip the redundant ACPI node: pipeline-%u.\n",
                    pipe_id);
            continue;
        }
        mdev->pipelines[pipe_id]->fwnode = child;
    }

    for (pipe_id = 0; pipe_id < mdev->n_pipelines; pipe_id++) {
        pipe = mdev->pipelines[pipe_id];

        if (!pipe->fwnode) {
            DRM_ERROR("Pipeline-%d doesn't have a ACPI node.\n",
                  pipe->id);
            return -EINVAL;
        }
        ret = linlondp_parse_pipe_acpi(pipe);
        if (ret)
            return ret;
    }

    ret = device_property_read_u64(dev, "aclk_freq_fixed", (u64 *)&mdev->aclk_freq_fixed);
    if (ret)
        mdev->aclk_freq_fixed = 0;

    mdev->side_by_side = !device_property_read_u32(dev, "side_by_side_master",
                           &mdev->side_by_side_master);

    ret = device_property_read_u32(dev, "aclk_freq_fixed", (u32 *)&mdev->aclk_freq_fixed);
    if (ret)
        mdev->aclk_freq_fixed = 0;

    if (device_property_present(dev, "smart_aclk_freq"))
        mdev->smart_aclk_freq = true;
    else
        mdev->smart_aclk_freq = false;

    ret = device_property_read_u32(dev, "device-id", (u32 *)&mdev->id);
    if (ret)
        mdev->id = 0;

    return 0;
}

static int linlondp_gop_get(void)
{
	struct arm_smccc_res res;
	bool enabled_by_gop = 0;

	arm_smccc_smc(CIX_SIP_DP_GOP_CTRL, SKY1_SIP_DP_GOP_GET, 0, 0, 0, 0, 0,
		      0, &res);

	if (res.a0 & DPU_GOP_MASK)
		enabled_by_gop = true;
	else
		enabled_by_gop = false;

	return enabled_by_gop;
}

static void linlondp_gop_set(void)
{
	struct arm_smccc_res res;
	int dpu_gop_bit = 1;

	arm_smccc_smc(CIX_SIP_DP_GOP_CTRL, SKY1_SIP_DP_GOP_SET,
		      dpu_gop_bit << DPU_GOP_SHIFT, 0, 0, 0, 0, 0, &res);
}

struct linlondp_dev *linlondp_dev_create(struct device *dev)
{
	struct platform_device *pdev = to_platform_device(dev);
	linlondp_identify_func linlondp_identify;
	struct linlondp_dev *mdev;
	u32 is_insmod = 0;
	int err = 0;

	linlondp_identify = device_get_match_data(dev);

	if (!linlondp_identify)
		return ERR_PTR(-ENODEV);

	mdev = devm_kzalloc(dev, sizeof(*mdev), GFP_KERNEL);
	if (!mdev)
		return ERR_PTR(-ENOMEM);

	mutex_init(&mdev->lock);

	mdev->dev = dev;

    err = device_property_read_u32(dev, "enabled_by_gop",
                               (u32 *)&mdev->enabled_by_gop);
    if (err)
        mdev->enabled_by_gop = 0;

	is_insmod = linlondp_gop_get();
	if (is_insmod)
		mdev->enabled_by_gop = 0;

	mdev->reg_base = devm_platform_ioremap_resource(pdev, 0);
	if (IS_ERR(mdev->reg_base)) {
		DRM_ERROR("Map register space failed.\n");
		err = PTR_ERR(mdev->reg_base);
		mdev->reg_base = NULL;
		goto err_cleanup;
	}
#if !IS_ENABLED(CONFIG_DRM_LINLONDP_CLOCK_FIXED)
	mdev->aclk = devm_clk_get(dev, "aclk");
	if (IS_ERR(mdev->aclk)) {
		DRM_ERROR("Get engine clk failed.\n");
		err = PTR_ERR(mdev->aclk);
		mdev->aclk = NULL;
		goto err_cleanup;
	}

	clk_prepare_enable(mdev->aclk);
#endif
	pm_runtime_get_sync(mdev->dev);
	mdev->funcs = linlondp_identify(mdev->reg_base, &mdev->chip);
	if (!mdev->funcs) {
		DRM_ERROR("Failed to identify the HW.\n");
		err = -ENODEV;
		goto disable_clk;
	}

	DRM_INFO("Found ARMCHINA Linlon-D%x version r%dp%d\n",
		 LINLONDP_CORE_ID_PRODUCT_ID(mdev->chip.core_id),
		 LINLONDP_CORE_ID_MAJOR(mdev->chip.core_id),
		 LINLONDP_CORE_ID_MINOR(mdev->chip.core_id));

	mdev->funcs->init_format_table(mdev);

	err = mdev->funcs->enum_resources(mdev);
	if (err) {
		DRM_ERROR("enumerate display resource failed.\n");
		goto disable_clk;
	}

	if (has_acpi_companion(dev))
		err = linlondp_parse_acpi(dev, mdev);
	else
		err = linlondp_parse_dt(dev, mdev);

	if (err) {
		DRM_ERROR("parse device tree failed.\n");
		goto disable_clk;
	}

	err = linlondp_assemble_pipelines(mdev);
	if (err) {
		DRM_ERROR("assemble display pipelines failed.\n");
		goto disable_clk;
	}

	if (mdev->funcs->init_hw) {
		err = mdev->funcs->init_hw(mdev);
		if (err) {
			DRM_ERROR("Initialize hardware failed!\n");
			goto disable_clk;
		}
	}

	dma_set_max_seg_size(dev, U32_MAX);

	mdev->iommu = iommu_get_domain_for_dev(mdev->dev);
	if (!mdev->iommu)
		DRM_INFO("continue without IOMMU support!\n");
#if !IS_ENABLED(CONFIG_DRM_LINLONDP_CLOCK_FIXED)
		//clk_disable_unprepare(mdev->aclk);
#endif
	err = sysfs_create_group(&dev->kobj, &linlondp_sysfs_attr_group);
	if (err) {
		DRM_ERROR("create sysfs group failed.\n");
		goto err_cleanup;
	}

	mdev->err_verbosity = LINLONDP_DEV_PRINT_ERR_EVENTS;

#ifdef CONFIG_DEBUG_FS
	linlondp_debugfs_init(mdev);
#endif

	err = dma_set_mask_and_coherent(mdev->dev, DMA_BIT_MASK(40));
	if (err) {
		DRM_ERROR("dma_set_mask_and_coherent failed.\n");
		goto err_cleanup;
	}

	pm_runtime_put(mdev->dev);

	return mdev;

disable_clk:
#if !IS_ENABLED(CONFIG_DRM_LINLONDP_CLOCK_FIXED)
	clk_disable_unprepare(mdev->aclk);
#endif
err_cleanup:
	pm_runtime_put(mdev->dev);
	linlondp_dev_destroy(mdev);
	return ERR_PTR(err);
}

void linlondp_dev_destroy(struct linlondp_dev *mdev)
{
	struct device *dev = mdev->dev;
	const struct linlondp_dev_funcs *funcs = mdev->funcs;
	int i;

	sysfs_remove_group(&dev->kobj, &linlondp_sysfs_attr_group);

#ifdef CONFIG_DEBUG_FS
	debugfs_remove_recursive(mdev->debugfs_root);
#endif
#if !IS_ENABLED(CONFIG_DRM_LINLONDP_CLOCK_FIXED)
	if (mdev->aclk)
		clk_prepare_enable(mdev->aclk);
#endif
	for (i = 0; i < mdev->n_pipelines; i++) {
		linlondp_pipeline_destroy(mdev, mdev->pipelines[i]);
		mdev->pipelines[i] = NULL;
	}

	mdev->n_pipelines = 0;

	of_reserved_mem_device_release(dev);

	if (funcs && funcs->cleanup)
		funcs->cleanup(mdev);

	if (mdev->reg_base) {
		devm_iounmap(dev, mdev->reg_base);
		mdev->reg_base = NULL;
	}
#if !IS_ENABLED(CONFIG_DRM_LINLONDP_CLOCK_FIXED)
	if (mdev->aclk) {
		clk_disable_unprepare(mdev->aclk);
		devm_clk_put(dev, mdev->aclk);
		mdev->aclk = NULL;
	}
#endif
	devm_kfree(dev, mdev);
}

int linlondp_dev_resume(struct linlondp_dev *mdev)
{
	int err = 0;

	dev_info(mdev->dev, "%s\n", __func__);

#if !IS_ENABLED(CONFIG_DRM_LINLONDP_CLOCK_FIXED)
	err = clk_prepare_enable(mdev->aclk);
	if (err)
		dev_err(mdev->dev, "%s, failed to enable aclk\n", __func__);
#endif
	mdev->funcs->enable_irq(mdev);

	if (mdev->iommu && mdev->funcs->connect_iommu)
		if (mdev->funcs->connect_iommu(mdev))
			DRM_ERROR("connect iommu failed.\n");

	if (mdev->funcs->init_hw) {
		err = mdev->funcs->init_hw(mdev);
		if (err)
			DRM_ERROR("Initialize hardware failed!\n");
	}

	return 0;
}

int linlondp_dev_suspend(struct linlondp_dev *mdev)
{
	dev_info(mdev->dev, "%s\n", __func__);

	if (mdev->iommu && mdev->funcs->disconnect_iommu)
		if (mdev->funcs->disconnect_iommu(mdev))
			DRM_ERROR("disconnect iommu failed.\n");

	mdev->funcs->disable_irq(mdev);
#if !IS_ENABLED(CONFIG_DRM_LINLONDP_CLOCK_FIXED)
	clk_disable_unprepare(mdev->aclk);
#endif
	return 0;
}
