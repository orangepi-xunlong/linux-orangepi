// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright 2023 Linaro
 */

#include <linux/module.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/pm_opp.h>
#include <linux/acpi.h>

struct dpc_reg {
	u8 descriptor;
	u16 length;
	u8 space_id;
	u8 bit_width;
	u8 bit_offset;
	u8 access_width;
	u64 address;
} __packed;

struct dpc_register_resource {
	acpi_object_type type;
	void __iomem *sys_mem_vaddr;
	union {
		struct dpc_reg reg;
		u64 int_value;
	} buffer;
};

struct acpi_dev_perf_level {
	u64 level; /* abstract unit */
	u64 power; /* milli Watts */
	u64 latency; /* micro Seconds */
};

enum dpc_regs {
	NOMINAL_PERF,
	NOMINAL_FREQ,
	DESIRED_PERF,
	DELIVERED_PERF,
	SUSTAINED_PERF,
	POWER_DOMAIN_INDEX,
	MAX_DPC_REGS
};

struct acpi_dpc_ctxt {
	u64 num_entries;
	u64 revision;
	unsigned int num_perf_levels;
	struct acpi_dev_perf_level *perf_levels;
	struct dpc_register_resource regs[MAX_DPC_REGS];
};

struct acpi_dev_pd_ctxt {
	u64 perf_level; /* Aggreegrate performance level */
	u64 domain_id; /* Index for the domain */
	struct list_head
		ctrl_list; /* List of acpi_dev_pstates_ctrl in this domain  */
	struct list_head list; /* List of acpi_dev_pd_ctxt */
};

struct acpi_dev_pstates_ctrl {
	struct device *dev;
	struct acpi_dpc_ctxt dpc;
	u64 nominal_freq;
	u64 nominal_perf;
	u64 sustained_perf;
	u64 perf_to_freq_factor;
	struct list_head list; /* List of acpi_dev_pstates_ctrl */
	struct list_head
		list_domain; /* List of acpi_dev_pstates_ctrl in same performance domain */
	u64 perf_level;
	struct acpi_dev_pd_ctxt *domain;
};

/* Evaluates to True if reg is a NULL register descriptor */
#define IS_NULL_REG(reg) ((reg)->space_id ==  ACPI_ADR_SPACE_SYSTEM_MEMORY && \
				(reg)->address == 0 &&			\
				(reg)->bit_width == 0 &&		\
				(reg)->bit_offset == 0 &&		\
				(reg)->access_width == 0)

static LIST_HEAD(dev_perf_ctrl_list);
static DEFINE_MUTEX(dev_perf_ctrl_list_mutex);

static LIST_HEAD(dev_perf_domain_list);
static DEFINE_MUTEX(dev_perf_domain_list_mutex);

static struct acpi_dev_pd_ctxt *find_perf_domain_ctxt(unsigned int domain_id)
{
	struct acpi_dev_pd_ctxt *ctxt;

	mutex_lock(&dev_perf_domain_list_mutex);

	list_for_each_entry(ctxt, &dev_perf_domain_list, list) {
		if (ctxt->domain_id == domain_id) {
			mutex_unlock(&dev_perf_domain_list_mutex);
			return ctxt;
		}
	}

	mutex_unlock(&dev_perf_domain_list_mutex);

	/* No perf ctxt found yet, allocate a new one */
	ctxt = kzalloc(sizeof(struct acpi_dev_pd_ctxt), GFP_KERNEL);

	/* Initialize */
	INIT_LIST_HEAD(&ctxt->list);
	INIT_LIST_HEAD(&ctxt->ctrl_list);

	ctxt->domain_id = domain_id;
	ctxt->perf_level = 0;

	/* Add to perf domain ctxt list */
	mutex_lock(&dev_perf_domain_list_mutex);
	list_add_tail(&ctxt->list, &dev_perf_domain_list);
	mutex_unlock(&dev_perf_domain_list_mutex);

	return ctxt;
}

static struct acpi_dev_pstates_ctrl *find_perf_ctrl_desc(struct device *dev)
{
	struct acpi_dev_pstates_ctrl *ctrl;

	mutex_lock(&dev_perf_ctrl_list_mutex);

	list_for_each_entry(ctrl, &dev_perf_ctrl_list, list) {
		if (!strcmp(dev_name(dev), dev_name(ctrl->dev))) {
			/* Matched on device name */
			mutex_unlock(&dev_perf_ctrl_list_mutex);
			return ctrl;
		}
	}

	mutex_unlock(&dev_perf_ctrl_list_mutex);

	return NULL;
}

// TODO: refactor arm64 ffh implementation into a seperate file
static int dpc_ffh_read(struct dpc_reg *reg, u64 *val)
{
	struct arm_smccc_res res;

	// ARM64 FFH Spec. for DPC - if top 32 bits are set, it is an SMC call
	if (reg->address >> 32 & 0xffffffff) {
		arm_smccc_smc(reg->address & 0xffffffff, 0, 0, 0, 0, 0, 0, 0,
			      &res);
		*val = res.a0;
		return 0;
	}

	return -EFAULT;
}

static int dpc_ffh_write(struct dpc_reg *reg, u64 val)
{
	struct arm_smccc_res res;

	// ARM64 FFH Spec. for DPC - if top 32 bits are set, it is an SMC call
	if (reg->address >> 32 & 0xffffffff) {
		arm_smccc_smc(reg->address & 0xffffffff, val, 0, 0, 0, 0, 0, 0,
			      &res);
		return 0;
	}

	return -EFAULT;
}

static int dpc_read(struct dpc_register_resource *reg_res, u64 *val)
{
	void __iomem *vaddr = NULL;
	struct dpc_reg *reg = &reg_res->buffer.reg;
	int ret = 0;

	if (reg_res->type == ACPI_TYPE_INTEGER) {
		*val = reg_res->buffer.int_value;
		return 0;
	}

	*val = 0;

	if (reg->space_id == ACPI_ADR_SPACE_FIXED_HARDWARE)
		return dpc_ffh_read(reg, val);
	else if (reg->space_id == ACPI_ADR_SPACE_SYSTEM_MEMORY)
		vaddr = reg_res->sys_mem_vaddr;
	else {
		pr_debug("DPC Register resource type (%d) not yet supported \n",
				reg_res->type);
		return -ENODEV;
	}

	switch (reg->bit_width) {
	case 8:
		*val = readb_relaxed(vaddr);
		break;
	case 16:
		*val = readw_relaxed(vaddr);
		break;
	case 32:
		*val = readl_relaxed(vaddr);
		break;
	case 64:
		*val = readq_relaxed(vaddr);
		break;
	default:
		pr_debug("Error: Cannot write %u bit width to DPC\n",
				reg->bit_width);
		ret = -EFAULT;
		break;
	}

	return ret;
}

static int dpc_write(struct dpc_register_resource *reg_res, u64 val)
{
	void __iomem *vaddr = NULL;
	struct dpc_reg *reg = &reg_res->buffer.reg;
	int ret = 0;

	if (reg->space_id == ACPI_ADR_SPACE_FIXED_HARDWARE)
		return dpc_ffh_write(reg, val);
	else if (reg->space_id == ACPI_ADR_SPACE_SYSTEM_MEMORY)
		vaddr = reg_res->sys_mem_vaddr;
	else {
		pr_debug("DPC Register resource type (%d) not yet supported \n",
				reg_res->type);
		return -ENODEV;
	}

	switch (reg->bit_width) {
	case 8:
		writeb_relaxed(val, vaddr);
		break;
	case 16:
		writew_relaxed(val, vaddr);
		break;
	case 32:
		writel_relaxed(val, vaddr);
		break;
	case 64:
		writeq_relaxed(val, vaddr);
		break;
	default:
		pr_debug("Error: Cannot write %u bit width to DPC\n",
			 reg->bit_width);
		ret = -EFAULT;
		break;
	}

	return ret;
}

// Derive delivered performance rate for device from delivered performance state
u64 acpi_dev_perf_current_rate(struct device *dev)
{
	struct acpi_dev_pstates_ctrl *ctrl;
	u64 perf_level;
	int ret;

	ctrl = find_perf_ctrl_desc(dev);

	ret = dpc_read(&ctrl->dpc.regs[DELIVERED_PERF], &perf_level);
	if (ret < 0)
		return 0;

	return perf_level * ctrl->perf_to_freq_factor;
}

int acpi_dev_perf_set_state(struct device *dev, unsigned long target_rate)
{
	struct acpi_dev_pstates_ctrl *ctrl;
	unsigned int state;
	int ret;

	ctrl = find_perf_ctrl_desc(dev);
	if (ctrl == NULL) {
		pr_debug("Failed to find perf control for device %s \n",
			 dev_name(dev));
		return -ENODEV;
	}

	/* Convert freq rate to state */
	state = target_rate / ctrl->perf_to_freq_factor;

	/* we can't go higher than guaranteed perf */
	if (state > ctrl->sustained_perf)
		state = ctrl->sustained_perf;

	if (state == ctrl->domain->perf_level)
		return 0;

	ret = dpc_write(&(ctrl->dpc.regs[DESIRED_PERF]), state);

	ctrl->domain->perf_level = state;
	ctrl->perf_level = state;

	return ret;
}

static int acpi_xtract_performance_levels(struct device *dev,
					  unsigned int *num_levels,
					  struct acpi_dev_perf_level **levels)
{
	unsigned int ret = 0;
	int i;
	acpi_status status;
	struct acpi_buffer buffer = { ACPI_ALLOCATE_BUFFER, NULL };
	struct acpi_buffer state = { 0, NULL };
	struct acpi_buffer format = { sizeof("NNN"), "NNN" };
	union acpi_object *dps = NULL;

	status = acpi_evaluate_object_typed(ACPI_HANDLE(dev), "_DPS", NULL,
					    &buffer, ACPI_TYPE_PACKAGE);

	if (ACPI_FAILURE(status))
		return -ENODEV;

	dps = buffer.pointer;

	*levels = devm_kzalloc(
		dev, dps->package.count * sizeof(struct acpi_dev_perf_level),
		GFP_KERNEL);

	state.length = sizeof(struct acpi_dev_perf_level);

	for (i = 0; i < dps->package.count; i++) {
		state.pointer = &(*levels)[i];

		status = acpi_extract_package(&(dps->package.elements[i]),
					      &format, &state);

		if (ACPI_FAILURE(status)) {
			ret = -EFAULT;
			kfree(*levels);
			goto out;
		}
	}

	*num_levels = dps->package.count;

out:
	kfree(buffer.pointer);
	return ret;
}

static int acpi_parse_dpc_resource(union acpi_object *obj,
				   struct dpc_register_resource *reg)
{
	struct dpc_reg *gas_t;

	switch (obj->type) {
	case ACPI_TYPE_INTEGER:
		reg->type = ACPI_TYPE_INTEGER;
		reg->buffer.int_value = obj->integer.value;
		return 0;
	case ACPI_TYPE_BUFFER:
		reg->type = ACPI_TYPE_BUFFER;
		memcpy(&reg->buffer.reg, obj->buffer.pointer,
		       sizeof(struct dpc_reg));
		gas_t = &reg->buffer.reg;

		if (gas_t->space_id == ACPI_ADR_SPACE_SYSTEM_MEMORY) {
			if (gas_t->address) {
				void __iomem *addr;

				addr = ioremap(gas_t->address, gas_t->bit_width/8);
				if (!addr)
					return -EINVAL;
				reg->sys_mem_vaddr = addr;
			}
		}

		return 0;
	default:
		return -EINVAL;
	}
}

int acpi_dev_perf_attach(struct device *dev)
{
	struct acpi_dev_pstates_ctrl *ctrl;
	acpi_handle handle = ACPI_HANDLE(dev);
	struct acpi_buffer output = { ACPI_ALLOCATE_BUFFER, NULL };
	struct dpc_register_resource *nominal_reg, *sustained_reg,
		*domain_index_reg, *nom_freq_reg = NULL;
	union acpi_object *out_obj, *dpc_obj;
	unsigned int i, num_ent;
	acpi_status status;
	int ret = -ENODATA, res_index = 0;
	u64 domain_index = 0;

	if (!acpi_has_method(handle, "_DPC") || !acpi_has_method(handle, "_DPS"))
		return 0;

	/* Parse ACPI _DPC object for this device. */
	status = acpi_evaluate_object_typed(handle, "_DPC", NULL, &output,
			ACPI_TYPE_PACKAGE);

	if (ACPI_FAILURE(status)) {
		ret = -ENODEV;
		goto out_buf_free;
	}

	ctrl = devm_kzalloc(dev, sizeof(struct acpi_dev_pstates_ctrl),
			    GFP_KERNEL);

	/* Initialize */
	ctrl->dev = dev;
	ctrl->domain = NULL;
	INIT_LIST_HEAD(&ctrl->list_domain);
	INIT_LIST_HEAD(&ctrl->list);

	out_obj = (union acpi_object *)output.pointer;

	/* First entry is NumEntries. */
	dpc_obj = &out_obj->package.elements[res_index++];
	if (dpc_obj->type == ACPI_TYPE_INTEGER) {
		num_ent = dpc_obj->integer.value;
		if (num_ent <= 1) {
			pr_debug(
				"Unexpected _DPC NumEntries value (%d) for device:%s\n",
				num_ent, dev_name(dev));
			goto out_free;
		}
		ctrl->dpc.num_entries = num_ent;
	} else {
		pr_debug(
			"Unexpected _DPC NumEntries entry type (%d) for device:%s\n",
			dpc_obj->type, dev_name(dev));
		goto out_free;
	}

	/* Second entry should be revision. */
	dpc_obj = &out_obj->package.elements[res_index++];
	if (dpc_obj->type == ACPI_TYPE_INTEGER) {
		ctrl->dpc.revision = dpc_obj->integer.value;
	} else {
		pr_debug(
			"Unexpected _DPC Revision entry type (%d) for Device:%s\n",
			dpc_obj->type, dev_name(dev));
		goto out_free;
	}

	ret = acpi_xtract_performance_levels(dev, &ctrl->dpc.num_perf_levels,
					     &ctrl->dpc.perf_levels);
	if (ret < 0)
		goto out_free;

	/* Iterate over rest of the resources from _DPC */
	for (i = 0; res_index < num_ent; i++, res_index++) {
		ret = acpi_parse_dpc_resource(
			&out_obj->package.elements[res_index],
			&ctrl->dpc.regs[i]);
		if (ret < 0)
			goto out_free;
	}

	/* Initialize the remaining dpc register as unsupported. */
	for (; i < MAX_DPC_REGS; i++) {
		ctrl->dpc.regs[i].type = ACPI_TYPE_INTEGER;
		ctrl->dpc.regs[i].buffer.int_value = 0;
	}

	nominal_reg = &ctrl->dpc.regs[NOMINAL_PERF];
	sustained_reg = &ctrl->dpc.regs[SUSTAINED_PERF];
	domain_index_reg = &ctrl->dpc.regs[POWER_DOMAIN_INDEX];
	nom_freq_reg = &ctrl->dpc.regs[NOMINAL_FREQ];

	ret = dpc_read(domain_index_reg, &domain_index);
	if (ret < 0)
		goto out_free;

	if (domain_index > 0) {
		ctrl->domain = find_perf_domain_ctxt(domain_index);
		if (ctrl->domain == NULL) {
			pr_debug(
				"Cannot locate performance domain index (%lld) for device %s\n",
				domain_index, dev_name(dev));
			goto out_free;
		}

		/* Add perf ctrl object to performance domain descriptor */
		mutex_lock(&dev_perf_domain_list_mutex);
		list_add_tail(&ctrl->list_domain, &ctrl->domain->ctrl_list);
		mutex_unlock(&dev_perf_domain_list_mutex);
	}

	ret = dpc_read(nom_freq_reg, &ctrl->nominal_freq);
	if (ret < 0)
		goto out_free;

	ret = dpc_read(nominal_reg, &ctrl->nominal_perf);
	if (ret < 0)
		goto out_free;

	if (nom_freq_reg->type != ACPI_TYPE_BUFFER ||
			IS_NULL_REG(&nom_freq_reg->buffer.reg)) {
		ctrl->sustained_perf = ctrl->nominal_perf;
	} else {
		ret = dpc_read(sustained_reg, &ctrl->sustained_perf);
		if (ret < 0)
			goto out_free;
	}

	/* Multiplication factor for converting performance level to frequency */
	ctrl->perf_to_freq_factor =
		(ctrl->nominal_freq * 1000) / ctrl->nominal_perf;

	/* Add device OPPs */
	for (i = 0; i < ctrl->dpc.num_perf_levels; i++) {
		ret = dev_pm_opp_add(dev,
				     ctrl->dpc.perf_levels[i].level *
					     ctrl->perf_to_freq_factor,
				     ctrl->dpc.perf_levels[i].power);
	}

	/* Add perf ctrl object to end of list */
	mutex_lock(&dev_perf_ctrl_list_mutex);
	list_add_tail(&ctrl->list, &dev_perf_ctrl_list);
	mutex_unlock(&dev_perf_ctrl_list_mutex);

	kfree(output.pointer);

	return 0;

out_free:
	kfree(ctrl);

out_buf_free:
	kfree(output.pointer);

	return ret;
}

int acpi_dev_perf_detach(struct device *dev)
{
	// TODO: free device performance control descriptor
	dev_pm_opp_remove_table(dev);
	return 0;
}
