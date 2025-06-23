/*
 * SPDX-License-Identifier: GPL-2.0+
 *
 * Add CIX SKY1 SoC Version driver
 *
 */

#include <linux/iommu.h>
#include <linux/kernel.h>
#include <linux/pci.h>
#include <linux/platform_device.h>
#include <linux/types.h>
#include <linux/arm-smccc.h>

#include <../drivers/iommu/arm/arm-smmu-v3/arm-smmu-v3.h>

#ifdef CIX_GOP_RESOURCE_QUIRK
#define CIX_SIP_SMMU_GOP_CTRL	0xc200000c
#define SMMU_RESET_BEFORE	0x0
#define SMMU_RESET_AFTER	0x1
#define MMHUB_BASE_STRT		0x0b1b0000
#define SYSHUB_BASE_STRT	0x0b0e0000
#define PCIEUB_BASE_STRT	0x0b010000
#endif


bool param_efifb_enable = false;
static struct page *pcie_page;

static int __init parse_gop(char *arg)
{
	if (!arg)
		return -EINVAL;

	if (strcmp(arg, "off") == 0)
		param_efifb_enable = false;
	else if (strcmp(arg, "on") == 0)
		param_efifb_enable = true;
	else
		return -EINVAL;
	return 0;
}
early_param("efifb_enable", parse_gop);

static void smmu_pcie_quirks(struct device *dev)
{
	struct arm_smmu_master *master;
	struct iommu_domain *domain;
	struct page *page = pcie_page;
	unsigned long vaddr = 0x0;
	int ret;
	phys_addr_t paddr;

	if (!dev)
		return;

	if (!dev_is_pci(dev))
		return;

	if (!page) {
		dev_err(dev, "pcie page NULL\n");
		return;
	}

	/*
	 * iommu domain not ready in iommu_get_domain_for_dev() interface, so
	 * get iommu domain from arm_smmu_master, which set as dev iommu priv.
	 */
	master = dev_iommu_priv_get(dev);
	if (!master || !master->domain)
		goto error;

	domain = &master->domain->domain;

	paddr = iommu_iova_to_phys(domain, vaddr);
	if (paddr) /* already mapped */
		return;

	ret = iommu_map(domain,
			vaddr, __pfn_to_phys(page_to_pfn(page)),
			PAGE_SIZE,
			IOMMU_READ | IOMMU_WRITE);
	if (ret) {
		dev_err(dev, "iommu map fail, ret[%d]", ret);
		goto error;
	}

	dev_info(dev, "smmu pcie quirks dev[%s] done\n", dev_name(dev));

	return;
error:
	dev_err(dev, "smmu pcie quirks dev[%s] fail\n", dev_name(dev));

	return;
}

static int smmu_attach_notify(struct notifier_block *nb, unsigned long val,
						      void *dev)
{
	switch (val) {
	case SMMU_DEV_ATTACH:
		smmu_pcie_quirks(dev);
		break;
	case SMMU_DEV_DETACH:
		break;
	default:
		break;
	}

	return 0;
}

static struct notifier_block smmu_attach_nb = {
	.notifier_call = smmu_attach_notify,
};

void cix_pcie_io_space_init(void)
{
	/*
	 * Default IO space is limited to IO_SPACE_LIMIT, which not enough
	 * for current usage. Expecially in acpi case. So extend it here.
	 */
	ioport_resource.end = -1;
}

int cix_pcie_quirks_init(void)
{
	pcie_page = alloc_pages(GFP_KERNEL, 0);
	if (!pcie_page) {
		pr_err("alloc page page error");
		return -ENOMEM;
	}

	return register_smmu_attach_notifier(&smmu_attach_nb);
}

#ifdef CIX_GOP_RESOURCE_QUIRK
static void smmu_mmhub_reset_before_quirks(struct device *dev)
{
	struct resource *res;
	struct arm_smccc_res smccc_res;

	res = platform_get_resource(to_platform_device(dev), IORESOURCE_MEM, 0);

	if (res->start != MMHUB_BASE_STRT)
		return;

	arm_smccc_smc(CIX_SIP_SMMU_GOP_CTRL,
                      SMMU_RESET_BEFORE, 0, 0, 0, 0,
                      0, 0, &smccc_res);
	return;
}

static void smmu_mmhub_reset_after_quirks(struct device *dev)
{
	struct resource *res;
	struct arm_smccc_res smccc_res;

	res = platform_get_resource(to_platform_device(dev), IORESOURCE_MEM, 0);

	if (res->start != MMHUB_BASE_STRT)
		return;

	arm_smccc_smc(CIX_SIP_SMMU_GOP_CTRL,
                      SMMU_RESET_AFTER, 0, 0, 0, 0,
                      0, 0, &smccc_res);

	return;

}

static int smmu_reset_notify(struct notifier_block *nb, unsigned long val,
						      void *dev)
{
	switch (val) {
	case SMMU_EN_BEFORE:
		smmu_mmhub_reset_before_quirks(dev);
		break;
	case SMMU_EN_AFTER:
		smmu_mmhub_reset_after_quirks(dev);
		break;
	default:
		break;
	}

	return 0;
}

static struct notifier_block smmu_reset_nb = {
	.notifier_call = smmu_reset_notify,
};

int cix_mmhub_quirks_init(void)
{
	if (!param_efifb_enable) {
		return register_smmu_probe_notifier(&smmu_reset_nb);
	}

	return 0;
}
#endif

static int cix_plat_init(void)
{
	cix_pcie_io_space_init();
	cix_pcie_quirks_init();

#ifdef CIX_GOP_RESOURCE_QUIRK
	cix_mmhub_quirks_init();
#endif

	return 0;
}
arch_initcall(cix_plat_init);
