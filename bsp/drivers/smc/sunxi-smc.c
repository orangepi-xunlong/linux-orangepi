/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * Based on arch/arm64/kernel/chipid-sunxi.c
 *
 * Copyright (C) 2015 Allwinnertech Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
//#define DEBUG
#include <linux/io.h>
#include <linux/kernel.h>
#include <sunxi-smc.h>
#include <sunxi-sid.h>
#include <linux/tee.h>
#include <linux/tee_drv.h>
#include <linux/version.h>
#include <linux/arm-smccc.h>
#include <linux/bitops.h>
#include <linux/slab.h>
#include <linux/module.h>
#include <linux/printk.h>
#include <linux/kernel.h>
#include <linux/device.h>
#include <linux/vmalloc.h>
#include <linux/uaccess.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <asm/cacheflush.h>
#include <sunxi-log.h>

struct smc_tee_secure_key_addr_group {
	uint32_t out_tee;
	uint32_t in_tee;
	uint32_t private;
};

#ifndef OPTEE_SMC_STD_CALL_VAL
#define OPTEE_SMC_STD_CALL_VAL(func_num) \
	ARM_SMCCC_CALL_VAL(ARM_SMCCC_STD_CALL, ARM_SMCCC_SMC_32, \
			   ARM_SMCCC_OWNER_TRUSTED_OS, (func_num))
#endif

#ifndef OPTEE_SMC_FAST_CALL_VAL
#define OPTEE_SMC_FAST_CALL_VAL(func_num) \
	ARM_SMCCC_CALL_VAL(ARM_SMCCC_FAST_CALL, ARM_SMCCC_SMC_32, \
			   ARM_SMCCC_OWNER_TRUSTED_OS, (func_num))
#endif
/*
 * Function specified by SMC Calling convention.
 */
#define OPTEE_SMC_FUNCID_READ_REG  (17 | sunxi_smc_call_offset())
#define OPTEE_SMC_READ_REG \
	OPTEE_SMC_FAST_CALL_VAL(OPTEE_SMC_FUNCID_READ_REG)


#define OPTEE_SMC_FUNCID_WRITE_REG  (18 | sunxi_smc_call_offset())
#define OPTEE_SMC_WRITE_REG \
	OPTEE_SMC_FAST_CALL_VAL(OPTEE_SMC_FUNCID_WRITE_REG)

#define OPTEE_SMC_FUNCID_COPY_ARISC_PARAS  (19 | sunxi_smc_call_offset())
#define OPTEE_SMC_COPY_ARISC_PARAS \
	OPTEE_SMC_FAST_CALL_VAL(OPTEE_SMC_FUNCID_COPY_ARISC_PARAS)
#define ARM_SVC_COPY_ARISC_PARAS    OPTEE_SMC_COPY_ARISC_PARAS

#define OPTEE_SMC_FUNCID_GET_TEEADDR_PARAS  (20 | sunxi_smc_call_offset())
#define OPTEE_SMC_GET_TEEADDR_PARAS \
	OPTEE_SMC_FAST_CALL_VAL(OPTEE_SMC_FUNCID_GET_TEEADDR_PARAS)
#define ARM_SVC_GET_TEEADDR_PARAS    OPTEE_SMC_GET_TEEADDR_PARAS

#define OPTEE_SMC_FUNCID_EFUSE  (21 | sunxi_smc_call_offset())
#define OPTEE_SMC_EFUSE_OP \
	OPTEE_SMC_FAST_CALL_VAL(OPTEE_SMC_FUNCID_EFUSE)
#define EFUSE_OP_WR 1
#define EFUSE_OP_RD 0

#define OPTEE_SMC_FUNCIC_SUNXI_HASH (23 | sunxi_smc_call_offset())
#define OPTEE_SMC_SUNXI_HASH_OP \
		OPTEE_SMC_FAST_CALL_VAL(OPTEE_SMC_FUNCIC_SUNXI_HASH)

#define OPTEE_SMC_FUNCID_GET_DRMINFO	(15 | sunxi_smc_call_offset())
#define OPTEE_SMC_GET_DRM_INFO \
	OPTEE_SMC_FAST_CALL_VAL(OPTEE_SMC_FUNCID_GET_DRMINFO)

#define OPTEE_SMC_FUNCIC_SUNXI_SETUP_MIPS (24 | sunxi_smc_call_offset())
#define OPTEE_SMC_SUNXI_SETUP_MIPS \
		OPTEE_SMC_FAST_CALL_VAL(OPTEE_SMC_FUNCIC_SUNXI_SETUP_MIPS)

#define OPTEE_SMC_FUNCID_SUNXI_INFORM_FDT (25 | sunxi_smc_call_offset())
#define OPTEE_SMC_SUNXI_INFORM_FDT \
		OPTEE_SMC_FAST_CALL_VAL(OPTEE_SMC_FUNCID_SUNXI_INFORM_FDT)

#define OPTEE_SMC_FUNCID_CRASHDUMP (26 | sunxi_smc_call_offset())
#define OPTEE_SMC_CRASHDUMP \
	OPTEE_SMC_FAST_CALL_VAL(OPTEE_SMC_FUNCID_CRASHDUMP)

#if defined(CONFIG_ARM64)

/* cmd to call ATF service */
#define ARM_SVC_EFUSE_PROBE_SECURE_ENABLE (0xc000fe03)
#define ARM_SVC_READ_SEC_REG (0xC000ff05)
#define ARM_SVC_WRITE_SEC_REG (0xC000ff06)
#define ARM_SVC_EFUSE_READ_AARCH64 (0xc000fe00)
#define ARM_SVC_EFUSE_WRITE_AARCH64 (0xc000fe01)
#define ARM_SVC_EFUSE_READ_EXTRA_AARCH64 (0xc000ff07)
#define ARM_SVC_EFUSE_READ ARM_SVC_EFUSE_READ_AARCH64
#define ARM_SVC_EFUSE_WRITE ARM_SVC_EFUSE_WRITE_AARCH64

#elif defined(CONFIG_ARCH_SUN50I) && defined(CONFIG_ARM)

#define ARM_SVC_EFUSE_PROBE_SECURE_ENABLE (0x8000fe03)
#define ARM_SVC_READ_SEC_REG (0x8000ff05)
#define ARM_SVC_WRITE_SEC_REG (0x8000ff06)
#define ARM_SVC_EFUSE_READ_AARCH32 (0x8000fe00)
#define ARM_SVC_EFUSE_WRITE_AARCH32 (0x8000fe01)
#define ARM_SVC_EFUSE_READ ARM_SVC_EFUSE_READ_AARCH32
#define ARM_SVC_EFUSE_WRITE ARM_SVC_EFUSE_WRITE_AARCH32

#else

/* cmd to call TEE service */
#define ARM_SVC_READ_SEC_REG OPTEE_SMC_READ_REG
#define ARM_SVC_WRITE_SEC_REG OPTEE_SMC_WRITE_REG
#define ARM_SVC_EFUSE_READ OPTEE_SMC_EFUSE_OP
#define ARM_SVC_EFUSE_WRITE OPTEE_SMC_EFUSE_OP

#endif  /* CONFIG_ARM64 */

#define ROUND(a, b) (((a) + (b)-1) & ~((b)-1))

/* interface for smc */
u32 sunxi_smc_readl(phys_addr_t addr)
{
#if defined(CONFIG_ARM64) || defined(CONFIG_TEE)
	struct arm_smccc_res res;
	arm_smccc_smc(ARM_SVC_READ_SEC_REG, addr, 0, 0, 0, 0, 0, 0, &res);
	return res.a0;
#else
	void __iomem *vaddr = ioremap(addr, 4);
	u32 ret;
	ret = readl(vaddr);
	iounmap(vaddr);
	return ret;
#endif
}
EXPORT_SYMBOL(sunxi_smc_readl);

int sunxi_smc_writel(u32 value, phys_addr_t addr)
{
#if defined(CONFIG_ARM64) || defined(CONFIG_TEE)
	struct arm_smccc_res res;
	arm_smccc_smc(ARM_SVC_WRITE_SEC_REG, addr, value, 0, 0, 0, 0, 0, &res);
	return res.a0;
#else
	void __iomem *vaddr = ioremap(addr, 4);
	writel(value, vaddr);
	iounmap(vaddr);
	return 0;
#endif
}
EXPORT_SYMBOL(sunxi_smc_writel);

#if defined(CONFIG_ARM64)
/* Currently 'ARM_SVC_EFUSE_READ_EXTRA_AARCH64' only supports ARM64 in atf. */
int sunxi_smc_read_extra(const char *key, void *val, unsigned long len)
{
	char *key_linear;
	void *val_linear;
	unsigned long key_pa;
	unsigned long val_pa;
	struct arm_smccc_res res;
	int ret;

	/* Prepare linear buffer for 'key' */
	key_linear = kzalloc(strlen(key) + 1, GFP_KERNEL);
	if (!key_linear) {
		sunxi_err(NULL, "Fail to kmalloc 0x%lx bytes for 'key_linear'\n", (unsigned long)(strlen(key) + 1));
		return -ENOMEM;
	}
	strcpy(key_linear, key);
	key_pa = virt_to_phys((volatile void *)key_linear);

	/* Prepare linear buffer for 'val' */
	val_linear = kzalloc(len, GFP_KERNEL);
	if (!val_linear) {
		sunxi_err(NULL, "Fail to kmalloc 0x%lx bytes for 'val_linear'\n", len);
		kfree(key_linear);
		return -ENOMEM;
	}
	val_pa = virt_to_phys((volatile void *)val_linear);

	sunxi_debug(NULL, "arm_smccc_smc(ARM_SVC_EFUSE_READ_EXTRA_AARCH64): key='%s', len=0x%lx\n", key, len);
	arm_smccc_smc(ARM_SVC_EFUSE_READ_EXTRA_AARCH64, key_pa, val_pa, len, 0, 0, 0, 0, &res);
	sunxi_debug(NULL, "arm_smccc_smc(ARM_SVC_EFUSE_READ_EXTRA_AARCH64): return 0x%lx\n", res.a0);
	ret = res.a0;
	if (ret < 0)
		sunxi_err(NULL, "Fail in smc call\n");
	else
		memcpy(val, val_linear, len);

	kfree(val_linear);
	kfree(key_linear);
	return ret;
}
EXPORT_SYMBOL_GPL(sunxi_smc_read_extra);
#endif  /* CONFIG_ARM64 */

#if defined(CONFIG_TEE) && defined(CONFIG_ARM)
static int optee_ctx_match(struct tee_ioctl_version_data *ver, const void *data)
{
	if (ver->impl_id == TEE_IMPL_ID_OPTEE)
		return 1;
	else
		return 0;
}

static inline int arm_svc_tee_efuse_op(phys_addr_t key_buf, phys_addr_t read_buf,
				       int efuse_op)
{
	struct arm_smccc_res res;
	sunxi_efuse_key_info_t *efuse_info = NULL;
	sunxi_efuse_key_info_t *key_info =
		(sunxi_efuse_key_info_t *)phys_to_virt(key_buf);
	uint64_t *key_data = NULL;
	struct tee_context *tee_ctx;
	struct tee_shm *smc_data_shm = NULL;
	u8 *smc_data_shm_va = NULL;
	phys_addr_t key_data_pa;
	phys_addr_t efuse_info_pa;
	int ret = -1;
	/*prepare ctx for calling stander tee drv api*/
	tee_ctx = tee_client_open_context(NULL, optee_ctx_match, NULL, NULL);
	if (IS_ERR(tee_ctx)) {
		pr_err("tee_ctx:%d\n", IS_ERR(tee_ctx));
		ret = PTR_ERR(tee_ctx);
		goto out_ctx;
	}

	smc_data_shm =
		tee_shm_alloc(tee_ctx, 4096, TEE_SHM_MAPPED | TEE_SHM_DMA_BUF);
	if (IS_ERR(smc_data_shm)) {
		pr_err("failed with:%lx\n", PTR_ERR(smc_data_shm));
		ret = PTR_ERR(tee_ctx);
		goto out_shm;
	}

	smc_data_shm_va = tee_shm_get_va(smc_data_shm, 0);
	if (IS_ERR(smc_data_shm_va)) {
		pr_err("Failed to get virtual address for shared memory\n");
		ret = PTR_ERR(tee_ctx);
		goto out_shm;
	}

	tee_shm_va2pa(smc_data_shm, smc_data_shm_va, &key_data_pa);
	efuse_info = (sunxi_efuse_key_info_t *)((void *)smc_data_shm_va +
						sizeof(struct tee_shm));
	memset(&res, 0, sizeof(res));
	/*copy data of sunxi_efuse_key_info_t to shm memcpy*/
	memset(efuse_info, 0, sizeof(sunxi_efuse_key_info_t));
	memcpy(efuse_info, key_info, ROUND(sizeof(sunxi_efuse_key_info_t), 8));
	key_data = (uint64_t *)((char *)efuse_info +
				ROUND(sizeof(sunxi_efuse_key_info_t), 8));
	tee_shm_va2pa(smc_data_shm, key_data, &key_data_pa);
	efuse_info->key_data = (uint64_t)key_data_pa;
	/*copy data of writed to shm memcpy*/
	if (efuse_op == EFUSE_OP_WR) {
		memcpy((void *)phys_to_virt(efuse_info->key_data),
		       (void *)phys_to_virt(key_info->key_data), key_info->len);
	}

	/*different kernel version has different interface about cache*/
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 15, 0)
	flush_kernel_vmap_range(
		(void *)smc_data_shm_va,
		(unsigned long)((char *)smc_data_shm_va + 4096));
#else
	__flush_dcache_area((void *)smc_data_shm_va, 4096);
#endif

	memset(&res, 0, sizeof(res));
	tee_shm_va2pa(smc_data_shm, (void *)efuse_info, &efuse_info_pa);

	arm_smccc_smc(OPTEE_SMC_EFUSE_OP, efuse_op,
		      (unsigned long)efuse_info_pa, 0, 0, 0, 0, 0, &res);

	if (res.a0 < 0) {
		pr_err("fail with write efuse:%ld\n", res.a0);
		ret = -ENODEV;
		goto out_shm;
	}

	/*different kernel version has different interface about cache*/
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 15, 0)
	invalidate_kernel_vmap_range(
		(void *)smc_data_shm_va,
		(unsigned long)((char *)smc_data_shm_va + 4096));
#else
	__inval_cache_range(smc_data_shm_va, smc_data_shm_va + 4096);
#endif
	if (efuse_op == EFUSE_OP_RD) {
		memset(phys_to_virt(read_buf), 0, sizeof(sunxi_efuse_key_info_t));
		memcpy(phys_to_virt(read_buf),
		       phys_to_virt(efuse_info->key_data), key_info->len);
	}
out_shm:
	tee_shm_free(smc_data_shm);
out_ctx:
	tee_client_close_context(tee_ctx);
	ret = key_info->len;

	return ret;
}
#endif

int arm_svc_efuse_write(phys_addr_t key_buf)
{
#if defined(CONFIG_ARM64)
	struct arm_smccc_res res;
	arm_smccc_smc(ARM_SVC_EFUSE_WRITE, (unsigned long)key_buf, 0, 0, 0, 0,
		      0, 0, &res);
	return res.a0;
#elif defined(CONFIG_ARCH_SUN50I) && defined(CONFIG_ARM)
	struct arm_smccc_res res;
	arm_smccc_smc(ARM_SVC_EFUSE_WRITE, (unsigned long)key_buf, 0, 0, 0, 0,
		      0, 0, &res);
	return res.a0;
#elif defined(CONFIG_TEE)
	return arm_svc_tee_efuse_op(key_buf, (phys_addr_t)NULL, EFUSE_OP_WR);
#else
	return -1;
#endif
}
EXPORT_SYMBOL_GPL(arm_svc_efuse_write);

int arm_svc_efuse_read(phys_addr_t key_buf, phys_addr_t read_buf)
{
#if defined(CONFIG_ARM64)
	struct arm_smccc_res res;
	arm_smccc_smc(ARM_SVC_EFUSE_READ, (unsigned long)key_buf,
		      (unsigned long)read_buf, 0, 0, 0, 0, 0, &res);
	return res.a0;
#elif defined(CONFIG_ARCH_SUN50I) && defined(CONFIG_ARM)
	struct arm_smccc_res res;
	arm_smccc_smc(ARM_SVC_EFUSE_READ, (unsigned long)key_buf,
		      (unsigned long)read_buf, 0, 0, 0, 0, 0, &res);
	return res.a0;
#elif defined(CONFIG_TEE)
	return arm_svc_tee_efuse_op(key_buf, read_buf, EFUSE_OP_RD);
#else
	return -1;
#endif
}
EXPORT_SYMBOL_GPL(arm_svc_efuse_read);

int sunxi_smc_copy_arisc_paras(phys_addr_t dest, phys_addr_t src, u32 len)
{
	struct arm_smccc_res res;
	arm_smccc_smc(ARM_SVC_COPY_ARISC_PARAS, dest, src, len, 0, 0, 0, 0, &res);
	return res.a0;
}

phys_addr_t sunxi_smc_get_teeaddr_paras(phys_addr_t resumeaddr)
{
	struct arm_smccc_res res;

	arm_smccc_smc(ARM_SVC_GET_TEEADDR_PARAS,
		resumeaddr, 0, 0, 0, 0, 0, 0, &res);
	return res.a0;
}
/* optee smc */
#define ARM_SMCCC_SMC_32		0
#define ARM_SMCCC_SMC_64		1
#define ARM_SMCCC_CALL_CONV_SHIFT	30

#define ARM_SMCCC_OWNER_MASK		0x3F
#define ARM_SMCCC_OWNER_SHIFT		24

#define ARM_SMCCC_FUNC_MASK		0xFFFF
#define ARM_SMCCC_OWNER_TRUSTED_OS	50

#define ARM_SMCCC_IS_FAST_CALL(smc_val)	\
	((smc_val) & (ARM_SMCCC_FAST_CALL << ARM_SMCCC_TYPE_SHIFT))
#define ARM_SMCCC_IS_64(smc_val) \
	((smc_val) & (ARM_SMCCC_SMC_64 << ARM_SMCCC_CALL_CONV_SHIFT))
#define ARM_SMCCC_FUNC_NUM(smc_val)	((smc_val) & ARM_SMCCC_FUNC_MASK)
#define ARM_SMCCC_OWNER_NUM(smc_val) \
	(((smc_val) >> ARM_SMCCC_OWNER_SHIFT) & ARM_SMCCC_OWNER_MASK)

#define ARM_SMCCC_CALL_VAL(type, calling_convention, owner, func_num) \
	(((type) << ARM_SMCCC_TYPE_SHIFT) | \
	((calling_convention) << ARM_SMCCC_CALL_CONV_SHIFT) | \
	(((owner) & ARM_SMCCC_OWNER_MASK) << ARM_SMCCC_OWNER_SHIFT) | \
	((func_num) & ARM_SMCCC_FUNC_MASK))

#define OPTEE_SMC_FAST_CALL_VAL(func_num) \
	ARM_SMCCC_CALL_VAL(ARM_SMCCC_FAST_CALL, ARM_SMCCC_SMC_32, \
			   ARM_SMCCC_OWNER_TRUSTED_OS, (func_num))

/*
 * optee3.7 should be like this: OPTEE_SMC_FUNCID_CRYPT  (16 | sunxi_smc_call_offset())
 * optee2,5 should be like this: OPTEE_SMC_FUNCID_CRYPT  (16)
 * the format of other system calls is the same.
 */
#define OPTEE_SMC_FUNCID_CRYPT  (16 | sunxi_smc_call_offset())
#define OPTEE_SMC_CRYPT \
	OPTEE_SMC_FAST_CALL_VAL(OPTEE_SMC_FUNCID_CRYPT)

#define TEESMC_RSSK_DECRYPT      5
int sunxi_smc_refresh_hdcp(void)
{
	struct arm_smccc_res res;
	arm_smccc_smc(OPTEE_SMC_CRYPT, TEESMC_RSSK_DECRYPT, 0, 0, 0, 0, 0, 0, &res);
	return res.a0;
}
EXPORT_SYMBOL(sunxi_smc_refresh_hdcp);

#define OPTEE_MSG_FUNCID_GET_OS_REVISION	0x0001
#define OPTEE_SMC_CALL_GET_OS_REVISION \
	OPTEE_SMC_FAST_CALL_VAL(OPTEE_MSG_FUNCID_GET_OS_REVISION)

int smc_tee_get_os_revision(uint32_t *major, uint32_t *minor)
{
	struct arm_smccc_res param = { 0 };

	arm_smccc_smc(OPTEE_SMC_CALL_GET_OS_REVISION, 0, 0, 0, 0, 0, 0, 0,
		      &param);
	*major = param.a0;
	*minor = param.a1;
	return 0;
}

int sunxi_smc_call_offset(void)
{
	uint32_t major, minor;

	smc_tee_get_os_revision(&major, &minor);
	if ((major > 3) || ((major == 3) && (minor >= 5))) {
		return SUNXI_OPTEE_SMC_OFFSET;
	} else {
		return 0;
	}
}

int  optee_probe_drm_configure(unsigned long *drm_base,
	size_t *drm_size, unsigned long  *tee_base)
{
	struct arm_smccc_res res;

	arm_smccc_smc(OPTEE_SMC_GET_DRM_INFO, 0, 0, 0, 0, 0, 0, 0, &res);
	if (res.a0  != 0) {
		pr_err("drm config service not available: %X", (uint32_t)res.a0);
		return -EINVAL;
	}

	*drm_base = res.a1;
	*drm_size = res.a2;
	*tee_base = res.a3;

	pr_debug("drm_base=0x%x\n", (uint32_t)*drm_base);
	pr_debug("drm_size=0x%x\n", (uint32_t)*drm_size);
	pr_debug("tee_base=0x%x\n", (uint32_t)*tee_base);

	return 0;
}
EXPORT_SYMBOL(optee_probe_drm_configure);

int sunxi_optee_call_crashdump(void)
{
	struct arm_smccc_res res = { 0 };

	arm_smccc_smc(OPTEE_SMC_CRASHDUMP, 0, 0, 0, 0, 0, 0, 0, &res);
	if (res.a0  != 0) {
		pr_err("optee call crashdump error: %X", (uint32_t)res.a0);
		return -EINVAL;
	}

	return 0;
}
EXPORT_SYMBOL(sunxi_optee_call_crashdump);

#define TEESMC_PROBE_SHM_BASE    2
#define TEESMC_HUK_ENCRYPT       8
#define TEESMC_HUK_DECRYPT       9

static struct smc_tee_secure_key_addr_group *tee_addr;
int smc_tee_secure_key_encrypt(char *out_buf, char *in_buf, int len, enum secure_key_type key)
{
	struct arm_smccc_res param = { 0 };
	int align_len = 0;
	//align_len = ALIGN(len, CACHE_LINE_SIZE);
	align_len = ALIGN(len, 64);
	//align_len = len;

	if (out_buf == NULL || in_buf == NULL) {
		pr_err("smc enc parameter error\n");
		return -EINVAL;
	}

	if (len > 4096) {
		pr_err("smc enc len error\n");
		return -EINVAL;
	}

	arm_smccc_smc(OPTEE_SMC_CRYPT, TEESMC_PROBE_SHM_BASE, 0, 0, 0, 0, 0, 0, &param);
	if (param.a0 != 0) {
		return param.a0;
		pr_err("smc tee probe share memory base failed\n");
	}

	tee_addr = (struct smc_tee_secure_key_addr_group *)phys_to_virt(
	param.a1);
	tee_addr->in_tee = param.a1 + 0x100;
	tee_addr->out_tee = param.a1 + 0x100 + 4096;

	memset((void *)phys_to_virt(tee_addr->in_tee), 0x0, align_len);
	memcpy((void *)phys_to_virt(tee_addr->in_tee), in_buf, len);

	flush_cache_all();

	memset(&param, 0, sizeof(param));
	//pr_info("==========encrypt start=============\n");
	switch (key) {
	case CE_KEY_SELECT_HUK:
		arm_smccc_smc(OPTEE_SMC_CRYPT, TEESMC_HUK_ENCRYPT,
			virt_to_phys((tee_addr)),
			align_len, 0, 0, 0, 0, &param);
		break;
	default:
		pr_err("this secure key not support\n");
		return -EINVAL;
	}
	//pr_info("==========encrypt end=============\n");
	if (param.a0 != 0) {
		pr_err("smc tee encrypt with huk failed with: %ld\n", param.a0);
		return param.a0;
	}

	memcpy(out_buf, (void *)phys_to_virt(tee_addr->out_tee), align_len);

	return 0;
}
EXPORT_SYMBOL(smc_tee_secure_key_encrypt);

int smc_tee_secure_key_decrypt(char *out_buf, char *in_buf, int len, enum secure_key_type key)
{
	struct arm_smccc_res param = { 0 };

	arm_smccc_smc(OPTEE_SMC_CRYPT, TEESMC_PROBE_SHM_BASE, 0, 0, 0, 0, 0, 0,
		&param);
	if (param.a0 != 0) {
		pr_err("smc tee probe share memory base failed\n ");
		return param.a0;
	}

	if (out_buf == NULL || in_buf == NULL) {
		pr_err("smc enc parameter error\n");
		return -EINVAL;
	}

	if (len > 4096) {
		pr_err("smc enc len error\n");
		return -EINVAL;
	}

	tee_addr = (struct smc_tee_secure_key_addr_group *)phys_to_virt(
	param.a1);
	tee_addr->in_tee = param.a1 + 0x100;
	tee_addr->out_tee = param.a1 + 0x100 + 4096;
	memcpy((void *)phys_to_virt(tee_addr->in_tee), in_buf, len);

	flush_cache_all();

	memset(&param, 0, sizeof(param));

	//pr_info("==========decrypt start=============\n");
	switch (key) {
	case CE_KEY_SELECT_HUK:
		arm_smccc_smc(OPTEE_SMC_CRYPT, TEESMC_HUK_DECRYPT,
			virt_to_phys(tee_addr), len, 0,
			0, 0, 0, &param);
		break;
	default:
		pr_err("this secure key not support\n");
		return -EINVAL;
	}
	//pr_info("==========decrypt end=============\n");
	if (param.a0 != 0) {
		pr_err("smc tee decrypt with huk failed with: %ld", param.a0);
		return param.a0;
	}

	memcpy(out_buf, (void *)phys_to_virt(tee_addr->out_tee), len);
	return 0;
}
EXPORT_SYMBOL(smc_tee_secure_key_decrypt);

static int __init sunxi_smc_init(void)
{
	return 0;
}

static void __exit sunxi_smc_exit(void)
{
}

module_init(sunxi_smc_init);
module_exit(sunxi_smc_exit);
MODULE_LICENSE("GPL v2");
MODULE_VERSION("1.0.2");
MODULE_AUTHOR("weidonghui<weidonghui@allwinnertech.com>");
MODULE_DESCRIPTION("sunxi smc.");
