// SPDX-License-Identifier: GPL-2.0 or MIT
/* Copyright 2018 Marty E. Plummer <hanetzer@startmail.com> */
/* Copyright 2019 Linaro, Ltd, Rob Herring <robh@kernel.org> */
/* Copyright 2023 Collabora ltd. */

#include <asm/delay.h>
#include <linux/clk.h>
#include <linux/mm.h>
#include <linux/platform_device.h>
#include <linux/pm_domain.h>
#include <linux/pm_runtime.h>
#include <linux/regulator/consumer.h>
#include <linux/reset.h>

#include <drm/drm_drv.h>
#include <drm/drm_managed.h>

#include "panthor_devfreq.h"
#include "panthor_device.h"
#include "panthor_fw.h"
#include "panthor_gpu.h"
#include "panthor_mmu.h"
#include "panthor_regs.h"
#include "panthor_sched.h"

static int panthor_clk_init(struct panthor_device *ptdev)
{
	ptdev->clks.core = devm_clk_get(ptdev->base.dev, "gpu_clk_core");
	if (IS_ERR(ptdev->clks.core))
		return dev_err_probe(ptdev->base.dev,
				     PTR_ERR(ptdev->clks.core),
				     "get 'core' clock failed");

	ptdev->clks.stacks = devm_clk_get(ptdev->base.dev, "gpu_clk_stacks");
	if (IS_ERR(ptdev->clks.stacks))
		return dev_err_probe(ptdev->base.dev,
				     PTR_ERR(ptdev->clks.stacks),
				     "get 'stacks' clock failed");

	/* CIX SKY1 needs additional backup clocks */
	ptdev->clks.backup[0] = devm_clk_get_optional(ptdev->base.dev, "gpu_clk_200M");
	if (IS_ERR(ptdev->clks.backup[0]))
		return dev_err_probe(ptdev->base.dev,
						PTR_ERR(ptdev->clks.backup[0]),
						"get 'gpu_clk_200M' clock failed");

	ptdev->clks.backup[1] = devm_clk_get_optional(ptdev->base.dev, "gpu_clk_400M");
	if (IS_ERR(ptdev->clks.backup[1]))
		return dev_err_probe(ptdev->base.dev,
						PTR_ERR(ptdev->clks.backup[1]),
						"get 'gpu_clk_400M' clock failed");

	drm_info(&ptdev->base, "clock rate = %lu\n", clk_get_rate(ptdev->clks.core));
	return 0;
}

static void panthor_pm_domain_fini(struct panthor_device *ptdev)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(ptdev->pm_domain_devs); i++) {
		if (!ptdev->pm_domain_devs[i])
			continue;

		if (ptdev->pm_domain_links[i])
			device_link_del(ptdev->pm_domain_links[i]);

		dev_pm_domain_detach(ptdev->pm_domain_devs[i], true);
	}
}

static int panthor_pm_domain_init(struct panthor_device *ptdev)
{
	int err;
	int i, num_domains;

	if (!has_acpi_companion(ptdev->base.dev)) {
		num_domains = of_count_phandle_with_args(ptdev->base.dev->of_node,
							"power-domains",
							"#power-domain-cells");

		/*
		* Single domain is handled by the core, and, if only a single power
		* the power domain is requested, the property is optional.
		*/
		if (num_domains < 2)
			return 0;

		if (WARN(num_domains > ARRAY_SIZE(ptdev->pm_domain_devs),
				"Too many supplies in compatible structure.\n"))
			return -EINVAL;

		for (i = 0; i < num_domains; i++) {
			ptdev->pm_domain_devs[i] =
				dev_pm_domain_attach_by_id(ptdev->base.dev, i);
			if (IS_ERR_OR_NULL(ptdev->pm_domain_devs[i])) {
				err = PTR_ERR(ptdev->pm_domain_devs[i]) ? : -ENODATA;
				ptdev->pm_domain_devs[i] = NULL;
				dev_err(ptdev->base.dev,
					"failed to get pm-domain %d: %d\n",
					i, err);
				goto err;
			}

			ptdev->pm_domain_links[i] = device_link_add(ptdev->base.dev,
					ptdev->pm_domain_devs[i], DL_FLAG_PM_RUNTIME |
					DL_FLAG_STATELESS | DL_FLAG_RPM_ACTIVE);
			if (!ptdev->pm_domain_links[i]) {
				dev_err(ptdev->pm_domain_devs[i],
					"adding device link failed!\n");
				err = -ENODEV;
				goto err;
			}
		}
	} else {
		ptdev->pm_domain_devs[1]= fwnode_dev_pm_domain_attach_by_name(ptdev->base.dev, "perf");
		if (IS_ERR_OR_NULL(ptdev->pm_domain_devs[1])) {
			err = PTR_ERR(ptdev->pm_domain_devs[1]) ? : -ENODATA;
			ptdev->pm_domain_devs[1] = NULL;
			dev_err(ptdev->base.dev,
				"failed to get acpi perf domain %d\n", err);
			goto err;
		}

		ptdev->pm_domain_links[1] = device_link_add(ptdev->base.dev,
					ptdev->pm_domain_devs[1], DL_FLAG_PM_RUNTIME |
					DL_FLAG_STATELESS | DL_FLAG_RPM_ACTIVE);
		if (!ptdev->pm_domain_links[1]) {
			dev_err(ptdev->base.dev, "Failed to add device_link to gpu perf domain.\n");
			err = -ENODEV;
			goto err;
		}

		struct fwnode_handle *fwnode = fwnode_find_reference(ptdev->base.dev->fwnode, "power-supply", 0);
		if (IS_ERR_OR_NULL(fwnode)) {
			dev_warn(ptdev->base.dev, "Failed to get power-supply property, using single power domain.\n");
			return 0;
		}
		ptdev->pm_domain_devs[0] = bus_find_device_by_fwnode(&platform_bus_type, fwnode);
		pm_runtime_enable(ptdev->pm_domain_devs[0]);
		dev_pm_domain_attach(ptdev->pm_domain_devs[0], true);
		fwnode_handle_put(fwnode);

		ptdev->pm_domain_links[0] = device_link_add(ptdev->base.dev,
					ptdev->pm_domain_devs[0], DL_FLAG_PM_RUNTIME |
					DL_FLAG_STATELESS | DL_FLAG_RPM_ACTIVE);
		if (!ptdev->pm_domain_links[0]) {
			dev_err(ptdev->base.dev, "Failed to add device_link to gpu power domain.\n");
			err = -ENODEV;
			goto err;
		}
	}

	return 0;

err:
	panthor_pm_domain_fini(ptdev);
	return err;
}

static int panthor_resets_init(struct panthor_device *ptdev)
{
	ptdev->gpu_reset = devm_reset_control_get(ptdev->base.dev, "gpu_reset");
	if (IS_ERR(ptdev->gpu_reset)) {
		dev_err(ptdev->base.dev, "failed to get gpu_reset\n");
		return PTR_ERR(ptdev->gpu_reset);
	}

	return 0;
}


void panthor_device_unplug(struct panthor_device *ptdev)
{
	/* This function can be called from two different path: the reset work
	 * and the platform device remove callback. drm_dev_unplug() doesn't
	 * deal with concurrent callers, so we have to protect drm_dev_unplug()
	 * calls with our own lock, and bail out if the device is already
	 * unplugged.
	 */
	mutex_lock(&ptdev->unplug.lock);
	if (drm_dev_is_unplugged(&ptdev->base)) {
		/* Someone beat us, release the lock and wait for the unplug
		 * operation to be reported as done.
		 **/
		mutex_unlock(&ptdev->unplug.lock);
		wait_for_completion(&ptdev->unplug.done);
		return;
	}

	/* Call drm_dev_unplug() so any access to HW blocks happening after
	 * that point get rejected.
	 */
	drm_dev_unplug(&ptdev->base);

	/* We do the rest of the unplug with the unplug lock released,
	 * future callers will wait on ptdev->unplug.done anyway.
	 */
	mutex_unlock(&ptdev->unplug.lock);

	drm_WARN_ON(&ptdev->base, pm_runtime_get_sync(ptdev->base.dev) < 0);

	/* Now, try to cleanly shutdown the GPU before the device resources
	 * get reclaimed.
	 */
	panthor_sched_unplug(ptdev);
	panthor_fw_unplug(ptdev);
	panthor_mmu_unplug(ptdev);
	panthor_gpu_unplug(ptdev);

	pm_runtime_dont_use_autosuspend(ptdev->base.dev);
	pm_runtime_put_sync_suspend(ptdev->base.dev);

	/* If PM is disabled, we need to call the suspend handler manually. */
	if (!IS_ENABLED(CONFIG_PM))
		panthor_device_suspend(ptdev->base.dev);

	panthor_pm_domain_fini(ptdev);

	/* Report the unplug operation as done to unblock concurrent
	 * panthor_device_unplug() callers.
	 */
	complete_all(&ptdev->unplug.done);
}

static void panthor_device_reset_cleanup(struct drm_device *ddev, void *data)
{
	struct panthor_device *ptdev = container_of(ddev, struct panthor_device, base);

	cancel_work_sync(&ptdev->reset.work);
	destroy_workqueue(ptdev->reset.wq);
}

static void panthor_device_reset_work(struct work_struct *work)
{
	struct panthor_device *ptdev = container_of(work, struct panthor_device, reset.work);
	int ret = 0, cookie;

	if (atomic_read(&ptdev->pm.state) != PANTHOR_DEVICE_PM_STATE_ACTIVE) {
		/*
		 * No need for a reset as the device has been (or will be)
		 * powered down
		 */
		atomic_set(&ptdev->reset.pending, 0);
		return;
	}

	if (!drm_dev_enter(&ptdev->base, &cookie))
		return;

	panthor_sched_pre_reset(ptdev);
	panthor_fw_pre_reset(ptdev, true);
	panthor_mmu_pre_reset(ptdev);
	panthor_gpu_soft_reset(ptdev);
	panthor_gpu_l2_power_on(ptdev);
	panthor_mmu_post_reset(ptdev);
	ret = panthor_fw_post_reset(ptdev);
	atomic_set(&ptdev->reset.pending, 0);
	panthor_sched_post_reset(ptdev, ret != 0);
	drm_dev_exit(cookie);

	if (ret) {
		panthor_device_unplug(ptdev);
		drm_err(&ptdev->base, "Failed to boot MCU after reset, making device unusable.");
	}
}

static bool panthor_device_is_initialized(struct panthor_device *ptdev)
{
	return !!ptdev->scheduler;
}

static void panthor_device_free_page(struct drm_device *ddev, void *data)
{
	__free_page(data);
}

int panthor_device_init(struct panthor_device *ptdev)
{
	u32 *dummy_page_virt;
	struct resource *res;
	struct page *p;
	int ret;

	ptdev->coherent = device_get_dma_attr(ptdev->base.dev) == DEV_DMA_COHERENT;

	init_completion(&ptdev->unplug.done);
	ret = drmm_mutex_init(&ptdev->base, &ptdev->unplug.lock);
	if (ret)
		return ret;

	ret = drmm_mutex_init(&ptdev->base, &ptdev->pm.mmio_lock);
	if (ret)
		return ret;

	atomic_set(&ptdev->pm.state, PANTHOR_DEVICE_PM_STATE_SUSPENDED);
	p = alloc_page(GFP_KERNEL | __GFP_ZERO);
	if (!p)
		return -ENOMEM;

	ptdev->pm.dummy_latest_flush = p;
	dummy_page_virt = page_address(p);
	ret = drmm_add_action_or_reset(&ptdev->base, panthor_device_free_page,
				       ptdev->pm.dummy_latest_flush);
	if (ret)
		return ret;

	/*
	 * Set the dummy page holding the latest flush to 1. This will cause the
	 * flush to avoided as we know it isn't necessary if the submission
	 * happens while the dummy page is mapped. Zero cannot be used because
	 * that means 'always flush'.
	 */
	*dummy_page_virt = 1;

	INIT_WORK(&ptdev->reset.work, panthor_device_reset_work);
	ptdev->reset.wq = alloc_ordered_workqueue("panthor-reset-wq", 0);
	if (!ptdev->reset.wq)
		return -ENOMEM;

	ret = drmm_add_action_or_reset(&ptdev->base, panthor_device_reset_cleanup, NULL);
	if (ret)
		return ret;

	ret = panthor_clk_init(ptdev);
	if (ret)
		return ret;

	ret = panthor_devfreq_init(ptdev);
	if (ret)
		return ret;

	ret = panthor_pm_domain_init(ptdev);
	if (ret)
		return ret;

	ret = panthor_resets_init(ptdev);
	if (ret)
		goto err_release_pm_domains;

	ptdev->iomem = devm_platform_get_and_ioremap_resource(to_platform_device(ptdev->base.dev),
							      1, &res);
	if (IS_ERR(ptdev->iomem)) {
		ret = PTR_ERR(ptdev->iomem);
		goto err_release_pm_domains;
	}

	if (of_device_is_compatible(ptdev->base.dev->of_node, "arm,mali-valhall")) {
		ptdev->sky1_rcsu_reg = devm_platform_ioremap_resource(to_platform_device(ptdev->base.dev), 0);
		if (IS_ERR(ptdev->sky1_rcsu_reg)) {
			ret = PTR_ERR(ptdev->sky1_rcsu_reg);
			goto err_release_pm_domains;
		}
	}


	ptdev->phys_addr = res->start;

	ret = devm_pm_runtime_enable(ptdev->base.dev);
	if (ret)
		goto err_release_pm_domains;

	ret = pm_runtime_resume_and_get(ptdev->base.dev);
	if (ret)
		goto err_release_pm_domains;

	/* If PM is disabled, we need to call panthor_device_resume() manually. */
	if (!IS_ENABLED(CONFIG_PM)) {
		ret = panthor_device_resume(ptdev->base.dev);
		if (ret)
			goto err_release_pm_domains;
	}

	ret = panthor_gpu_init(ptdev);
	if (ret)
		goto err_rpm_put;

	ret = panthor_mmu_init(ptdev);
	if (ret)
		goto err_unplug_gpu;

	ret = panthor_fw_init(ptdev);
	if (ret)
		goto err_unplug_mmu;

	ret = panthor_sched_init(ptdev);
	if (ret)
		goto err_unplug_fw;

	/* ~3 frames */
	pm_runtime_set_autosuspend_delay(ptdev->base.dev, 50);
	pm_runtime_use_autosuspend(ptdev->base.dev);

	ret = drm_dev_register(&ptdev->base, 0);
	if (ret)
		goto err_disable_autosuspend;

	pm_runtime_put_autosuspend(ptdev->base.dev);
	return 0;

err_disable_autosuspend:
	pm_runtime_dont_use_autosuspend(ptdev->base.dev);
	panthor_sched_unplug(ptdev);

err_unplug_fw:
	panthor_fw_unplug(ptdev);

err_unplug_mmu:
	panthor_mmu_unplug(ptdev);

err_unplug_gpu:
	panthor_gpu_unplug(ptdev);

err_rpm_put:
	pm_runtime_put_sync_suspend(ptdev->base.dev);

err_release_pm_domains:
	panthor_pm_domain_fini(ptdev);
	return ret;
}

#define PANTHOR_EXCEPTION(id) \
	[DRM_PANTHOR_EXCEPTION_ ## id] = { \
		.name = #id, \
	}

struct panthor_exception_info {
	const char *name;
};

static const struct panthor_exception_info panthor_exception_infos[] = {
	PANTHOR_EXCEPTION(OK),
	PANTHOR_EXCEPTION(TERMINATED),
	PANTHOR_EXCEPTION(KABOOM),
	PANTHOR_EXCEPTION(EUREKA),
	PANTHOR_EXCEPTION(ACTIVE),
	PANTHOR_EXCEPTION(CS_RES_TERM),
	PANTHOR_EXCEPTION(CS_CONFIG_FAULT),
	PANTHOR_EXCEPTION(CS_UNRECOVERABLE),
	PANTHOR_EXCEPTION(CS_ENDPOINT_FAULT),
	PANTHOR_EXCEPTION(CS_BUS_FAULT),
	PANTHOR_EXCEPTION(CS_INSTR_INVALID),
	PANTHOR_EXCEPTION(CS_CALL_STACK_OVERFLOW),
	PANTHOR_EXCEPTION(CS_INHERIT_FAULT),
	PANTHOR_EXCEPTION(INSTR_INVALID_PC),
	PANTHOR_EXCEPTION(INSTR_INVALID_ENC),
	PANTHOR_EXCEPTION(INSTR_BARRIER_FAULT),
	PANTHOR_EXCEPTION(DATA_INVALID_FAULT),
	PANTHOR_EXCEPTION(TILE_RANGE_FAULT),
	PANTHOR_EXCEPTION(ADDR_RANGE_FAULT),
	PANTHOR_EXCEPTION(IMPRECISE_FAULT),
	PANTHOR_EXCEPTION(OOM),
	PANTHOR_EXCEPTION(CSF_FW_INTERNAL_ERROR),
	PANTHOR_EXCEPTION(CSF_RES_EVICTION_TIMEOUT),
	PANTHOR_EXCEPTION(GPU_BUS_FAULT),
	PANTHOR_EXCEPTION(GPU_SHAREABILITY_FAULT),
	PANTHOR_EXCEPTION(SYS_SHAREABILITY_FAULT),
	PANTHOR_EXCEPTION(GPU_CACHEABILITY_FAULT),
	PANTHOR_EXCEPTION(TRANSLATION_FAULT_0),
	PANTHOR_EXCEPTION(TRANSLATION_FAULT_1),
	PANTHOR_EXCEPTION(TRANSLATION_FAULT_2),
	PANTHOR_EXCEPTION(TRANSLATION_FAULT_3),
	PANTHOR_EXCEPTION(TRANSLATION_FAULT_4),
	PANTHOR_EXCEPTION(PERM_FAULT_0),
	PANTHOR_EXCEPTION(PERM_FAULT_1),
	PANTHOR_EXCEPTION(PERM_FAULT_2),
	PANTHOR_EXCEPTION(PERM_FAULT_3),
	PANTHOR_EXCEPTION(ACCESS_FLAG_1),
	PANTHOR_EXCEPTION(ACCESS_FLAG_2),
	PANTHOR_EXCEPTION(ACCESS_FLAG_3),
	PANTHOR_EXCEPTION(ADDR_SIZE_FAULT_IN),
	PANTHOR_EXCEPTION(ADDR_SIZE_FAULT_OUT0),
	PANTHOR_EXCEPTION(ADDR_SIZE_FAULT_OUT1),
	PANTHOR_EXCEPTION(ADDR_SIZE_FAULT_OUT2),
	PANTHOR_EXCEPTION(ADDR_SIZE_FAULT_OUT3),
	PANTHOR_EXCEPTION(MEM_ATTR_FAULT_0),
	PANTHOR_EXCEPTION(MEM_ATTR_FAULT_1),
	PANTHOR_EXCEPTION(MEM_ATTR_FAULT_2),
	PANTHOR_EXCEPTION(MEM_ATTR_FAULT_3),
};

const char *panthor_exception_name(struct panthor_device *ptdev, u32 exception_code)
{
	if (exception_code >= ARRAY_SIZE(panthor_exception_infos) ||
	    !panthor_exception_infos[exception_code].name)
		return "Unknown exception type";

	return panthor_exception_infos[exception_code].name;
}

static vm_fault_t panthor_mmio_vm_fault(struct vm_fault *vmf)
{
	struct vm_area_struct *vma = vmf->vma;
	struct panthor_device *ptdev = vma->vm_private_data;
	u64 offset = (u64)vma->vm_pgoff << PAGE_SHIFT;
	unsigned long pfn;
	pgprot_t pgprot;
	vm_fault_t ret;
	bool active;
	int cookie;

	if (!drm_dev_enter(&ptdev->base, &cookie))
		return VM_FAULT_SIGBUS;

	mutex_lock(&ptdev->pm.mmio_lock);
	active = atomic_read(&ptdev->pm.state) == PANTHOR_DEVICE_PM_STATE_ACTIVE;

	switch (offset) {
	case DRM_PANTHOR_USER_FLUSH_ID_MMIO_OFFSET:
		if (active)
			pfn = __phys_to_pfn(ptdev->phys_addr + CSF_GPU_LATEST_FLUSH_ID);
		else
			pfn = page_to_pfn(ptdev->pm.dummy_latest_flush);
		break;

	default:
		ret = VM_FAULT_SIGBUS;
		goto out_unlock;
	}

	pgprot = vma->vm_page_prot;
	if (active)
		pgprot = pgprot_noncached(pgprot);

	ret = vmf_insert_pfn_prot(vma, vmf->address, pfn, pgprot);

out_unlock:
	mutex_unlock(&ptdev->pm.mmio_lock);
	drm_dev_exit(cookie);
	return ret;
}

static const struct vm_operations_struct panthor_mmio_vm_ops = {
	.fault = panthor_mmio_vm_fault,
};

int panthor_device_mmap_io(struct panthor_device *ptdev, struct vm_area_struct *vma)
{
	u64 offset = (u64)vma->vm_pgoff << PAGE_SHIFT;

	if ((vma->vm_flags & VM_SHARED) == 0)
		return -EINVAL;

	switch (offset) {
	case DRM_PANTHOR_USER_FLUSH_ID_MMIO_OFFSET:
		if (vma->vm_end - vma->vm_start != PAGE_SIZE ||
		    (vma->vm_flags & (VM_WRITE | VM_EXEC)))
			return -EINVAL;
		vm_flags_clear(vma, VM_MAYWRITE);

		break;

	default:
		return -EINVAL;
	}

	/* Defer actual mapping to the fault handler. */
	vma->vm_private_data = ptdev;
	vma->vm_ops = &panthor_mmio_vm_ops;
	vm_flags_set(vma,
		     VM_IO | VM_DONTCOPY | VM_DONTEXPAND |
		     VM_NORESERVE | VM_DONTDUMP | VM_PFNMAP);

	return 0;
}

int panthor_device_resume(struct device *dev)
{
	struct panthor_device *ptdev = dev_get_drvdata(dev);
	int ret, cookie, i;

	if (atomic_read(&ptdev->pm.state) != PANTHOR_DEVICE_PM_STATE_SUSPENDED)
		return -EINVAL;

	atomic_set(&ptdev->pm.state, PANTHOR_DEVICE_PM_STATE_RESUMING);

	ret = clk_prepare_enable(ptdev->clks.core);
	if (ret)
		goto err_set_suspended;

	ret = clk_prepare_enable(ptdev->clks.stacks);
	if (ret)
		goto err_disable_core_clk;

	ret = clk_prepare_enable(ptdev->clks.coregroup);
	if (ret)
		goto err_disable_stacks_clk;

	for (i = 0; i < ARRAY_SIZE(ptdev->clks.backup); i++) {
		ret = clk_prepare_enable(ptdev->clks.backup[i]);
		if (ret)
			goto err_disable_backup_clks;
	}

	/* In case we have reset controls, assert and deassert reset lines */
	reset_control_assert(ptdev->gpu_reset);
	udelay(10);
	reset_control_deassert(ptdev->gpu_reset);

	/* CIX SKY1 have custom devfreq, let's force max for now (XXX: devfreq) */
	dev_pm_genpd_set_performance_state(ptdev->pm_domain_devs[1], 1000);

	ret = panthor_devfreq_resume(ptdev);
	if (ret)
		goto err_disable_coregroup_clk;

	if (panthor_device_is_initialized(ptdev) &&
	    drm_dev_enter(&ptdev->base, &cookie)) {
		panthor_gpu_resume(ptdev);
		panthor_mmu_resume(ptdev);
		ret = panthor_fw_resume(ptdev);
		if (!drm_WARN_ON(&ptdev->base, ret)) {
			panthor_sched_resume(ptdev);
		} else {
			panthor_mmu_suspend(ptdev);
			panthor_gpu_suspend(ptdev);
		}

		drm_dev_exit(cookie);

		if (ret)
			goto err_suspend_devfreq;
	}

	if (atomic_read(&ptdev->reset.pending))
		queue_work(ptdev->reset.wq, &ptdev->reset.work);

	/* Clear all IOMEM mappings pointing to this device after we've
	 * resumed. This way the fake mappings pointing to the dummy pages
	 * are removed and the real iomem mapping will be restored on next
	 * access.
	 */
	mutex_lock(&ptdev->pm.mmio_lock);
	unmap_mapping_range(ptdev->base.anon_inode->i_mapping,
			    DRM_PANTHOR_USER_MMIO_OFFSET, 0, 1);
	atomic_set(&ptdev->pm.state, PANTHOR_DEVICE_PM_STATE_ACTIVE);
	mutex_unlock(&ptdev->pm.mmio_lock);
	return 0;

err_suspend_devfreq:
	panthor_devfreq_suspend(ptdev);

err_disable_backup_clks:
	for (i = 0; i < ARRAY_SIZE(ptdev->clks.backup); i++)
		clk_disable_unprepare(ptdev->clks.backup[i]);

err_disable_coregroup_clk:
	clk_disable_unprepare(ptdev->clks.coregroup);

err_disable_stacks_clk:
	clk_disable_unprepare(ptdev->clks.stacks);

err_disable_core_clk:
	clk_disable_unprepare(ptdev->clks.core);

err_set_suspended:
	atomic_set(&ptdev->pm.state, PANTHOR_DEVICE_PM_STATE_SUSPENDED);
	return ret;
}

int panthor_device_suspend(struct device *dev)
{
	struct panthor_device *ptdev = dev_get_drvdata(dev);
	int ret, cookie, i;

	if (atomic_read(&ptdev->pm.state) != PANTHOR_DEVICE_PM_STATE_ACTIVE)
		return -EINVAL;

	/* Clear all IOMEM mappings pointing to this device before we
	 * shutdown the power-domain and clocks. Failing to do that results
	 * in external aborts when the process accesses the iomem region.
	 * We change the state and call unmap_mapping_range() with the
	 * mmio_lock held to make sure the vm_fault handler won't set up
	 * invalid mappings.
	 */
	mutex_lock(&ptdev->pm.mmio_lock);
	atomic_set(&ptdev->pm.state, PANTHOR_DEVICE_PM_STATE_SUSPENDING);
	unmap_mapping_range(ptdev->base.anon_inode->i_mapping,
			    DRM_PANTHOR_USER_MMIO_OFFSET, 0, 1);
	mutex_unlock(&ptdev->pm.mmio_lock);

	if (panthor_device_is_initialized(ptdev) &&
	    drm_dev_enter(&ptdev->base, &cookie)) {
		cancel_work_sync(&ptdev->reset.work);

		/* We prepare everything as if we were resetting the GPU.
		 * The end of the reset will happen in the resume path though.
		 */
		panthor_sched_suspend(ptdev);
		panthor_fw_suspend(ptdev);
		panthor_mmu_suspend(ptdev);
		panthor_gpu_suspend(ptdev);
		drm_dev_exit(cookie);
	}

	ret = panthor_devfreq_suspend(ptdev);
	if (ret) {
		if (panthor_device_is_initialized(ptdev) &&
		    drm_dev_enter(&ptdev->base, &cookie)) {
			panthor_gpu_resume(ptdev);
			panthor_mmu_resume(ptdev);
			drm_WARN_ON(&ptdev->base, panthor_fw_resume(ptdev));
			panthor_sched_resume(ptdev);
			drm_dev_exit(cookie);
		}

		goto err_set_active;
	}

	for (i = 0; i < ARRAY_SIZE(ptdev->clks.backup); i++) {
		clk_disable_unprepare(ptdev->clks.backup[i]);
	}

	clk_disable_unprepare(ptdev->clks.coregroup);
	clk_disable_unprepare(ptdev->clks.stacks);
	clk_disable_unprepare(ptdev->clks.core);
	atomic_set(&ptdev->pm.state, PANTHOR_DEVICE_PM_STATE_SUSPENDED);
	return 0;

err_set_active:
	/* If something failed and we have to revert back to an
	 * active state, we also need to clear the MMIO userspace
	 * mappings, so any dumb pages that were mapped while we
	 * were trying to suspend gets invalidated.
	 */
	mutex_lock(&ptdev->pm.mmio_lock);
	atomic_set(&ptdev->pm.state, PANTHOR_DEVICE_PM_STATE_ACTIVE);
	unmap_mapping_range(ptdev->base.anon_inode->i_mapping,
			    DRM_PANTHOR_USER_MMIO_OFFSET, 0, 1);
	mutex_unlock(&ptdev->pm.mmio_lock);
	return ret;
}
