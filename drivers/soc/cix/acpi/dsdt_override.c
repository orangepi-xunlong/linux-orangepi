// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright 2024 Cix Technology Group Co., Ltd.
 *
 * Author: Zichar Zhang <zichar.zhang@cixtech.com>
 */

#include <linux/acpi.h>
#include <linux/cma.h>
#include <linux/device.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/efi.h>
#include <linux/libfdt.h>
#include <linux/of_fdt.h>
#include <asm/unaligned.h>

#define AML_NAME_OP                 (u16) 0x08

extern u32 acpi_gbl_dsdt_index;
extern acpi_status
acpi_tb_install_standard_table(acpi_physical_address address,
			       u8 flags,
			       struct acpi_table_header *table,
			       u8 reload, u8 override, u32 *table_index);

static char *acpi_sync_sig_table[] = {
	"GNVA",
	"GNVL",
};

static char *acpi_tb_find_sig_ptr(struct acpi_table_header *table, char *sig)
{
	char *ptr, *cur = (char *)table;

	for (ptr = cur; ptr <= (cur + table->length); ptr++)
		if (ACPI_COMPARE_NAMESEG(ptr, sig)
			&& (*(ptr - 1) == AML_NAME_OP))
			return ptr;

	return NULL;
}

static int acpi_tb_copy_sig_ptr(char *optr, char *nptr)
{
	u8 osize, nsize, size;

	osize = *(optr + 4);
	nsize = *(nptr + 4);
	if (osize != nsize)
		return -EINVAL;

	switch(osize) {
		case 0x0:
			return 0;
		case 0xA:
			size = 1;
			break;
		case 0xB:
			size = 2;
			break;
		case 0xC:
			size = 4;
			break;
		default:
			return -EINVAL;
	}
	memcpy (nptr + 5, optr + 5, size);

	return 0;
}

static int acpi_dsdt_sync(u32 old_idx, u32 new_idx)
{
	struct acpi_table_header *oldt = NULL, *newt = NULL;
	char *sig, *optr, *nptr;
	int i, ret;
	acpi_status status;

	status = acpi_get_table_by_index(old_idx, &oldt);
	if (ACPI_FAILURE(status))
		goto out;

	status = acpi_get_table_by_index(new_idx, &newt);
	if (ACPI_FAILURE(status))
		goto out;

	for (i = 0; i < sizeof(acpi_sync_sig_table)/sizeof(char *); i++) {
		sig = acpi_sync_sig_table[i];
		optr = acpi_tb_find_sig_ptr(oldt, sig);
		if (!optr)
			continue;
		nptr = acpi_tb_find_sig_ptr(newt, sig);
		if (!nptr)
			continue;

		ret = acpi_tb_copy_sig_ptr(optr, nptr);
		if (ret)
			pr_err("dsdt sync sig[%s] error\n", sig);
	}

out:
	if (oldt)
		acpi_put_table(oldt);
	if (newt)
		acpi_put_table(newt);

       return 0;
}

static int __init efi_get_fdt_prop(const void *fdt, int node, const char *pname,
				   const char *rname, void *var, int size)
{
	const void *prop;
	int len;
	u64 val;

	prop = fdt_getprop(fdt, node, pname, &len);
	if (!prop)
		return 1;

	val = (len == 4) ? (u64)be32_to_cpup(prop) : get_unaligned_be64(prop);

	if (size == 8)
		*(u64 *)var = val;
	else
		*(u32 *)var = (val < U32_MAX) ? val : U32_MAX; // saturate

	if (efi_enabled(EFI_DBG))
		pr_info("  %s: 0x%0*llx\n", rname, size * 2, val);

	return 0;
}

static phys_addr_t dsdt_override_fdt_get_addr(void)
{
	const void *fdt = initial_boot_params;
	int node;
	phys_addr_t paddr;

	node = fdt_path_offset(fdt, "/chosen");
	if (node < 0)
		return 0;

	if (efi_get_fdt_prop(fdt, node, "linux,dsdt-override",
				"DSDT Override", &paddr, sizeof(u64)))
		return 0;

	return paddr;
}

static int __init dsdt_override_init(void)
{
	phys_addr_t paddr;
	acpi_status status = AE_OK;
	u32 old_dsdt_index = acpi_gbl_dsdt_index;

	paddr = dsdt_override_fdt_get_addr();
	if (paddr)
		status = acpi_tb_install_standard_table(paddr,
					ACPI_TABLE_ORIGIN_INTERNAL_PHYSICAL,
					NULL, FALSE, FALSE,
					&acpi_gbl_dsdt_index);

	if (status != AE_OK)
		return -1;

	acpi_dsdt_sync(old_dsdt_index, acpi_gbl_dsdt_index);

	return  0;
}

arch_initcall(dsdt_override_init);
