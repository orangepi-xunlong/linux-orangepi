// SPDX-License-Identifier: GPL-2.0+
/*
 * Module-based hack to drive MPAM functionality
 *
 * NOTICE: This circumvents existing infrastructure to discover and enable CPU
 * features and attempts to contain everything within a loadable module. This is
 * *not* the right way to do things, but it is one way to start testing MPAM on
 * real hardware.
 *
 * Copyright (C) 2022 Arm Ltd.
 */

#define DEBUG

#define pr_fmt(fmt) "MPAM_arch: " fmt

#include <linux/bits.h>
#include <linux/io.h>
#include <linux/kernel.h>
#include <linux/list.h>
#include <linux/lockdep.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/spinlock.h>

#include <asm/sysreg.h>

#include "mpam_arch.h"
#include "mpam_arch_internal.h"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Valentin Schneider <valentin.schneider@arm.com>");

#define FIELD_SET(reg, field, val) (reg = (reg & ~field) | FIELD_PREP(field, val))

struct msc_part_kobj {
	struct mpam_msc *msc;
	unsigned int partid;
	struct kobject kobj;
};

struct mpam_msc {
	struct platform_device *pdev;

	void __iomem *base;
	spinlock_t lock;

	unsigned int partid_count;
	unsigned int cpbm_nbits;
	unsigned int cmax_nbits;
	unsigned int cmax_shift;

	int has_ris;
	union {
		struct {
			bool has_cpor;
			bool has_ccap;
		};
		u8 part_feats;
	};

	struct kobject ko_root;
	struct kobject ko_part_dir;
	struct msc_part_kobj *ko_parts;
};

static __read_mostly unsigned int mpam_partid_count = UINT_MAX;

/* The MPAM0_EL1.PARTID_D in use by a given CPU */
static DEFINE_PER_CPU(unsigned int, mpam_local_partid) = MPAM_PARTID_DEFAULT;

unsigned int mpam_get_partid_count(void)
{
	/*
	 * XXX: this should check the driver has probed all matching devices
	 * first
	 */
	return mpam_partid_count;
}
EXPORT_SYMBOL_GPL(mpam_get_partid_count);

static void mpam_set_el0_partid(unsigned int inst_id, unsigned int data_id)
{
	u64 reg;

	cant_migrate();

	reg = read_sysreg_s(SYS_MPAM0_EL1);

	FIELD_SET(reg, MPAM0_EL1_PARTID_I, inst_id);
	FIELD_SET(reg, MPAM0_EL1_PARTID_D, data_id);

	write_sysreg_s(reg, SYS_MPAM0_EL1);
	/*
	 * Note: if the scope is limited to userspace, we'll get an EL switch
	 * before getting back to US which will be our context synchronization
	 * event, so this won't be necessary.
	 */
	isb();
}

/*
 * Write the PARTID to use on the local CPU.
 */
void mpam_write_partid(unsigned int partid)
{
	WARN_ON_ONCE(preemptible());
	WARN_ON_ONCE(partid >= mpam_partid_count);

	if (partid == this_cpu_read(mpam_local_partid))
		return;

	this_cpu_write(mpam_local_partid, partid);
	mpam_set_el0_partid(partid, partid);
}
EXPORT_SYMBOL_GPL(mpam_write_partid);

static void mpam_msc_sel_partid(struct mpam_msc *msc, unsigned int id)
{
	u32 reg;

	lockdep_assert_held(&msc->lock);

	reg = readl_relaxed(msc->base + MPAMCFG_PART_SEL);

	FIELD_SET(reg, MPAMCFG_PART_SEL_PARTID_SEL, id);
	if (msc->has_ris)
		FIELD_SET(reg, MPAMCFG_PART_SEL_RIS, 0);

	writel_relaxed(reg, msc->base + MPAMCFG_PART_SEL);
	pr_debug("PART_SEL: 0x%x\n", reg);
}

static unsigned int mpam_msc_get_partid_max(struct mpam_msc *msc)
{
	lockdep_assert_held(&msc->lock);

	return FIELD_GET(MPAMF_IDR_PARTID_MAX, readq_relaxed(msc->base + MPAMF_IDR));
}

static void mpam_msc_set_cpbm(struct mpam_msc *msc,
			      unsigned int id,
			      const unsigned long *bitmap)
{
	void __iomem *addr = msc->base + MPAMCFG_CPBM_n;
	unsigned int bit = 0, n = 0;
	u32 acc = 0;

	lockdep_assert_held(&msc->lock);

	mpam_msc_sel_partid(msc, id);

	/* Single write every reg boundary */
	while (n++ < BITS_TO_U32(msc->cpbm_nbits)) {
		for_each_set_bit(bit, bitmap, min_t(unsigned int,
						    (n * BITS_PER_TYPE(u32)),
						     msc->cpbm_nbits))
			acc |= 1 << bit % BITS_PER_TYPE(u32);

		writel_relaxed(acc, addr);
		pr_debug("CPBM: 0x%x @%px\n", acc, addr);
		addr += sizeof(acc);
		bit = n*BITS_PER_TYPE(u32);
		acc = 0;
	}
}

static void mpam_msc_get_cpbm(struct mpam_msc *msc,
			      unsigned int id,
			      unsigned long *bitmap)
{
	void __iomem *addr = msc->base + MPAMCFG_CPBM_n;
	size_t regsize = BITS_PER_TYPE(u32);
	unsigned int bit;
	int n;

	lockdep_assert_held(&msc->lock);

	mpam_msc_sel_partid(msc, id);

	for (n = 0; (n * regsize) < msc->cpbm_nbits; n++) {
		unsigned long tmp = readl_relaxed(addr);

		for_each_set_bit(bit, &tmp, min(regsize, msc->cpbm_nbits - (n * regsize)))
			bitmap_set(bitmap, bit + (n * regsize), 1);

		addr += regsize;
	}
}

static u16 mpam_msc_get_cmax(struct mpam_msc *msc, unsigned int id)
{
	u32 reg;
	u16 res;

	lockdep_assert_held(&msc->lock);

	mpam_msc_sel_partid(msc, id);

	reg = readl_relaxed(msc->base + MPAMCFG_CMAX);
	res = FIELD_GET(MPAMCFG_CMAX_CMAX, reg);
	return res << msc->cmax_shift;
}

static void mpam_msc_set_cmax(struct mpam_msc *msc, unsigned int id, u16 val)
{
	lockdep_assert_held(&msc->lock);

	mpam_msc_sel_partid(msc, id);
	writel_relaxed(FIELD_PREP(MPAMCFG_CMAX_CMAX, val >> msc->cmax_shift),
		       msc->base + MPAMCFG_CMAX);
}

struct mpam_validation_masks {
	cpumask_var_t visited_cpus;
	cpumask_var_t supported_cpus;
	spinlock_t lock;
};

static void mpam_validate_cpu(void *info)
{
	struct mpam_validation_masks *masks = (struct mpam_validation_masks *)info;
	unsigned int partid_count;
	bool valid = true;

	if (!FIELD_GET(ID_AA64PFR0_MPAM, read_sysreg_s(SYS_ID_AA64PFR0_EL1))) {
		valid = false;
		goto out;
	}

	if (!FIELD_GET(MPAM1_EL1_MPAMEN, read_sysreg_s(SYS_MPAM1_EL1))) {
		valid = false;
		goto out;
	}

	partid_count = FIELD_GET(MPAMIDR_EL1_PARTID_MAX, read_sysreg_s(SYS_MPAMIDR_EL1)) + 1;

	spin_lock(&masks->lock);
	mpam_partid_count = min(partid_count, mpam_partid_count);
	spin_unlock(&masks->lock);
out:

	cpumask_set_cpu(smp_processor_id(), masks->visited_cpus);
	if (valid)
		cpumask_set_cpu(smp_processor_id(), masks->supported_cpus);
}

/*
 * Does the system support MPAM, and if so is it actually usable?
 */
static int mpam_validate_sys(void)
{
	struct mpam_validation_masks masks;
	int ret = 0;

	if (!zalloc_cpumask_var(&masks.visited_cpus, GFP_KERNEL))
		return -ENOMEM;

	if (!zalloc_cpumask_var(&masks.supported_cpus, GFP_KERNEL)) {
		ret = -ENOMEM;
		goto out_free_visited;
	}
	spin_lock_init(&masks.lock);

	on_each_cpu_cond_mask(NULL, mpam_validate_cpu, &masks, true, cpu_present_mask);

	if (!cpumask_equal(masks.visited_cpus, cpu_present_mask)) {
		pr_warn("Could not check all CPUs for MPAM settings (visited %*pbl)\n",
			cpumask_pr_args(masks.visited_cpus));
		ret = -ENODATA;
		goto out;
	}

	if (!cpumask_equal(masks.visited_cpus, masks.supported_cpus)) {
		pr_warn("MPAM only supported on CPUs [%*pbl]\n",
			cpumask_pr_args(masks.supported_cpus));
		ret = -EOPNOTSUPP;
	}
out:
	free_cpumask_var(masks.supported_cpus);
out_free_visited:
	free_cpumask_var(masks.visited_cpus);

	return ret;
}

static ssize_t mpam_msc_cpbm_show(struct kobject *kobj, struct kobj_attribute *attr,
				  char *buf)
{
	struct msc_part_kobj *mpk = container_of(kobj, struct msc_part_kobj, kobj);
	unsigned long *bitmap;
	unsigned long flags;
	size_t size;

	bitmap = bitmap_zalloc(mpk->msc->cpbm_nbits, GFP_KERNEL);
	if (!bitmap)
		return -ENOMEM;

	spin_lock_irqsave(&mpk->msc->lock, flags);
	mpam_msc_get_cpbm(mpk->msc, mpk->partid, bitmap);
	spin_unlock_irqrestore(&mpk->msc->lock, flags);

	size = bitmap_print_to_pagebuf(true, buf, bitmap, mpk->msc->cpbm_nbits);

	bitmap_free(bitmap);
	return size;
}

static ssize_t mpam_msc_cpbm_store(struct kobject *kobj, struct kobj_attribute *attr,
				    const char *buf, size_t size)
{
	struct msc_part_kobj *mpk = container_of(kobj, struct msc_part_kobj, kobj);
	unsigned long *bitmap;
	unsigned long flags;
	int ret;

	bitmap = bitmap_zalloc(mpk->msc->cpbm_nbits, GFP_KERNEL);
	if (!bitmap)
		return -ENOMEM;

	ret = bitmap_parselist(buf, bitmap, mpk->msc->cpbm_nbits);
	if (ret)
		goto out_free;

	spin_lock_irqsave(&mpk->msc->lock, flags);
	mpam_msc_set_cpbm(mpk->msc, mpk->partid, bitmap);
	spin_unlock_irqrestore(&mpk->msc->lock, flags);
out_free:
	bitmap_free(bitmap);
	return ret ?: size;
}

static ssize_t mpam_msc_cmax_show(struct kobject *kobj, struct kobj_attribute *attr,
				  char *buf)
{
	struct msc_part_kobj *mpk = container_of(kobj, struct msc_part_kobj, kobj);
	unsigned long flags;
	u16 val;

	spin_lock_irqsave(&mpk->msc->lock, flags);
	val = mpam_msc_get_cmax(mpk->msc, mpk->partid);
	spin_unlock_irqrestore(&mpk->msc->lock, flags);

	return sprintf(buf, "0x%04x\n", val);
}

static ssize_t mpam_msc_cmax_store(struct kobject *kobj, struct kobj_attribute *attr,
				    const char *buf, size_t size)
{
	struct msc_part_kobj *mpk = container_of(kobj, struct msc_part_kobj, kobj);
	unsigned long flags;
	u16 val;
	int ret;

	ret = kstrtou16(buf, 0, &val);
	if (ret)
		return ret;

	spin_lock_irqsave(&mpk->msc->lock, flags);
	mpam_msc_set_cmax(mpk->msc, mpk->partid, val);
	spin_unlock_irqrestore(&mpk->msc->lock, flags);

	return size;
}

static struct kobj_attribute mpam_msc_cpbm_attr =
	__ATTR(cpbm, 0644, mpam_msc_cpbm_show, mpam_msc_cpbm_store);

static struct kobj_attribute mpam_msc_cmax_attr =
	__ATTR(cmax, 0644, mpam_msc_cmax_show, mpam_msc_cmax_store);

static struct attribute *mpam_msc_ctrl_attrs[] = {
	&mpam_msc_cpbm_attr.attr,
	&mpam_msc_cmax_attr.attr,
	NULL,
};

static umode_t mpam_msc_ctrl_attr_visible(struct kobject *kobj,
				     struct attribute *attr,
				     int n)
{
	struct msc_part_kobj *mpk;

	mpk = container_of(kobj, struct msc_part_kobj, kobj);

	if (attr == &mpam_msc_cpbm_attr.attr &&
	    mpk->msc->has_cpor)
		goto visible;

	if (attr == &mpam_msc_cmax_attr.attr &&
	    mpk->msc->has_ccap)
		goto visible;

	return 0;

visible:
	return attr->mode;
}

static struct attribute_group mpam_msc_ctrl_attr_group = {
	.attrs = mpam_msc_ctrl_attrs,
	.is_visible = mpam_msc_ctrl_attr_visible,
};


static ssize_t mpam_msc_cpbm_nbits_show(struct kobject *kobj, struct kobj_attribute *attr,
				       char *buf)
{
	struct mpam_msc *msc = container_of(kobj, struct mpam_msc, ko_root);

	return sprintf(buf, "%u\n", msc->cpbm_nbits);
}

static ssize_t mpam_msc_cmax_nbits_show(struct kobject *kobj, struct kobj_attribute *attr,
				       char *buf)
{
	struct mpam_msc *msc = container_of(kobj, struct mpam_msc, ko_root);

	return sprintf(buf, "%u\n", msc->cmax_nbits);
}

static struct kobj_attribute mpam_msc_cpbm_nbits_attr =
	__ATTR(cpbm_nbits, 0444, mpam_msc_cpbm_nbits_show, NULL);
static struct kobj_attribute mpam_msc_cmax_nbits_attr =
	__ATTR(cmax_nbits, 0444, mpam_msc_cmax_nbits_show, NULL);

static struct attribute *mpam_msc_info_attrs[] = {
	&mpam_msc_cpbm_nbits_attr.attr,
	&mpam_msc_cmax_nbits_attr.attr,
	NULL,
};

static umode_t mpam_msc_info_attr_visible(struct kobject *kobj,
					  struct attribute *attr,
					  int n)
{
	struct mpam_msc *msc = container_of(kobj, struct mpam_msc, ko_root);

	if (attr == &mpam_msc_cpbm_nbits_attr.attr &&
	    msc->has_cpor)
		goto visible;

	if (attr == &mpam_msc_cmax_nbits_attr.attr &&
	    msc->has_ccap)
		goto visible;

	return 0;

visible:
	return attr->mode;
}

static struct attribute_group mpam_msc_info_attr_group = {
	.attrs = mpam_msc_info_attrs,
	.is_visible = mpam_msc_info_attr_visible,
};

static struct kobj_type mpam_kobj_ktype = {
	.sysfs_ops	= &kobj_sysfs_ops,
};

/*
 * msc-foo/
 *   mpam/
 *     cpbm_nbits
 *     partitions/
 *       0/cpbm
 *       1/cpbm
 *       ...
 */
static int mpam_msc_create_sysfs(struct mpam_msc *msc)
{
	unsigned int partid_count = min(mpam_partid_count, msc->partid_count);
	unsigned int part, tmp;
	int ret;

	kobject_init(&msc->ko_root, &mpam_kobj_ktype);
	ret = kobject_add(&msc->ko_root, &msc->pdev->dev.kobj, "mpam");
	if (ret)
		goto err_root;

	kobject_init(&msc->ko_part_dir, &mpam_kobj_ktype);
	ret = kobject_add(&msc->ko_part_dir, &msc->ko_root, "partitions");
	if (ret)
		goto err_part_dir;

	msc->ko_parts = devm_kzalloc(&msc->pdev->dev,
				     sizeof(*msc->ko_parts) * partid_count,
				     GFP_KERNEL);
	if (!msc->ko_parts) {
		ret = -ENOMEM;
		goto err_part_dir;
	}

	ret = sysfs_create_group(&msc->ko_root, &mpam_msc_info_attr_group);
	if (ret)
		goto err_info_grp;

	for (part = 0; part < partid_count; part++) {
		kobject_init(&msc->ko_parts[part].kobj, &mpam_kobj_ktype);
		msc->ko_parts[part].msc = msc;
		msc->ko_parts[part].partid = part;

		ret = kobject_add(&msc->ko_parts[part].kobj, &msc->ko_part_dir, "%d", part);
		if (ret)
			goto err_parts_add;
	}

	for (part = 0; part < partid_count; part++) {
		ret = sysfs_create_group(&msc->ko_parts[part].kobj, &mpam_msc_ctrl_attr_group);
		if (ret)
			goto err_parts_grp;
	}
	return 0;

err_parts_grp:
	for (tmp = 0; tmp < part; tmp++)
		sysfs_remove_group(&msc->ko_parts[part].kobj, &mpam_msc_ctrl_attr_group);
	part = partid_count - 1;

err_parts_add:
	for (tmp = 0; tmp < part; tmp++)
		kobject_put(&msc->ko_parts[tmp].kobj);

	sysfs_remove_group(&msc->ko_root, &mpam_msc_info_attr_group);

err_info_grp:
	devm_kfree(&msc->pdev->dev, msc->ko_parts);
err_part_dir:
	kobject_put(&msc->ko_part_dir);
err_root:
	kobject_put(&msc->ko_root);
	return ret;
}

static void mpam_msc_cmax_shift_set(struct mpam_msc *msc)
{
	u16 val;
	/*
	 * Note: The TRM says the implemented bits are the most significant ones,
	 * but the model doesn't seem to agree with it...
	 * Handle that in the background, dropping a warning case needed
	 */
	lockdep_assert_held(&msc->lock);

	if (!(msc->cmax_nbits < 16))
		return;
	/*
	 * Unimplemented bits within the field are RAZ/WI
	 * At this point the MPAM_CMAX.CMAX will not be adjusted with the shift
	 * so this operates on an unmodified reg content.
	 * Also, the default value for CMAX will be set further down the init
	 * so there is no need for reset here.
	 */
	mpam_msc_set_cmax(msc, MPAM_PARTID_DEFAULT, GENMASK(15, 0));
	val = mpam_msc_get_cmax(msc, MPAM_PARTID_DEFAULT);

	if (val & GENMASK(15 - msc->cmax_nbits, 0)) {
		msc->cmax_shift = 16 - msc->cmax_nbits;
		pr_warn("MPAM_CMAX: implemented bits are the least-significant ones!");
	}
}

static int mpam_msc_initialize(struct mpam_msc *msc)
{
	static unsigned long *bitmap;
	int partid;
	u64 reg;

	/*
	 * We're using helpers that expect the lock to be held, but we're
	 * setting things up and there is no interface yet, so nothing can
	 * race with us. Make lockdep happy, and save ourselves from a couple
	 * of lock/unlock.
	 */
	spin_acquire(&msc->lock.dep_map, 0, 0, _THIS_IP_);

	reg = readq_relaxed(msc->base + MPAMF_IDR);

	msc->has_cpor = FIELD_GET(MPAMF_IDR_HAS_CPOR_PART, reg);
	msc->has_ccap = FIELD_GET(MPAMF_IDR_HAS_CCAP_PART, reg);
	pr_err("%s %d msc->has_cpor = 0x%x msc->has_ccap = 0x%x msc->part_feats = 0x%x\n", __func__, __LINE__, msc->has_cpor, msc->has_ccap, msc->part_feats);
	/* Detect more features here */

	if (!msc->part_feats) {
		pr_err("MSC does not support any recognized partitionning feature\n");
		return -EOPNOTSUPP;
	}

	/* Check for features that aren't supported, disable those we can */
	if (FIELD_GET(MPAMF_IDR_HAS_PRI_PART, reg)) {
		pr_err("Priority partitionning present but not supported\n");
		return -EOPNOTSUPP;
	}

	msc->has_ris = FIELD_GET(MPAMF_IDR_HAS_RIS, reg);
	if (msc->has_ris)
		pr_warn("RIS present but not supported, only instance 0 will be used\n");

	/* Error interrupts aren't handled */
	reg = readl_relaxed(msc->base + MPAMF_ECR);
	FIELD_SET(reg, MPAMF_ECR_INTEN, 0);
	writel_relaxed(reg, msc->base + MPAMF_ECR);

	msc->partid_count = mpam_msc_get_partid_max(msc) + 1;
	pr_debug("%d partitions supported\n", msc->partid_count);
	if (msc->partid_count > mpam_partid_count)
		pr_debug("System limited to %d partitions\n", mpam_partid_count);

	reg = readl_relaxed(msc->base + MPAMF_CPOR_IDR);
	msc->cpbm_nbits = FIELD_GET(MPAMF_CPOR_IDR_CPBM_WD, reg);
	pr_debug("%d portions supported\n", msc->cpbm_nbits);

	reg = readl_relaxed(msc->base + MPAMF_CCAP_IDR);
	msc->cmax_nbits = FIELD_GET(MPAMF_CCAP_IDR_CMAX_WD, reg);
	mpam_msc_cmax_shift_set(msc);

	bitmap = bitmap_alloc(mpam_partid_count, GFP_KERNEL);
	if (!bitmap)
		return -ENOMEM;

	/*
	 * Make all partitions have a sane default setting. The reference manual
	 * "suggests" sane defaults, be paranoid.
	 */
	bitmap_fill(bitmap, mpam_partid_count);
	for (partid = 0; partid < mpam_partid_count; partid++) {
		mpam_msc_set_cpbm(msc, partid, bitmap);
		mpam_msc_set_cmax(msc, partid,
				  GENMASK(15, 15 - (msc->cmax_nbits -1)));
	}
	bitmap_free(bitmap);

	spin_release(&msc->lock.dep_map, _THIS_IP_);

	return mpam_msc_create_sysfs(msc);
}

static int mpam_probe(struct platform_device *pdev)
{
	struct mpam_msc *msc;
	struct resource *mem;
	void __iomem *base;
	int ret;

	msc = devm_kzalloc(&pdev->dev, sizeof(*msc), GFP_KERNEL);
	if (!msc)
		return -ENOMEM;

	msc->pdev = pdev;

	base = devm_platform_get_and_ioremap_resource(pdev, 0, &mem);
	if (IS_ERR(base)) {
		devm_kfree(&pdev->dev, msc);
		return PTR_ERR(base);
	}

	msc->base = base;
	spin_lock_init(&msc->lock);
	platform_set_drvdata(pdev, msc);

	ret = mpam_msc_initialize(msc);

	return ret;
}

static const struct of_device_id of_mpam_match[] = {
	{
		.compatible = "arm,mpam-msc"
	},
	{ /* end */ },
};

static struct platform_driver mpam_arch_driver = {
	.probe = mpam_probe,
	.driver = {
	       .name = "mpam",
	       .of_match_table = of_mpam_match
	},
};

static int __init mpam_arch_driver_init(void)
{
	int ret;

	/* Does the system support MPAM at all? */
	ret = mpam_validate_sys();
	if (ret)
		return -EOPNOTSUPP;

	return platform_driver_register(&mpam_arch_driver);
}

module_init(mpam_arch_driver_init);
