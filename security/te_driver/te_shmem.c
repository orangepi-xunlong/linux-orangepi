
/**
 * secure and non-secure share memory manager
 */

#include <linux/atomic.h>
#include <linux/uaccess.h>
#include <linux/fs.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/printk.h>
#include <linux/ioctl.h>
#include <linux/sched.h>
#include <linux/mm.h>
#include <linux/pagemap.h>
#include <linux/slab.h>
#include <asm/cacheflush.h>
#include <asm/firmware.h>
#include <linux/secure/te_protocol.h>
#include <linux/smp.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/pagemap.h>
#include <linux/init.h>
#include <linux/io.h>
#include <linux/export.h>
#include <linux/time.h>
#include <mach/sunxi-smc.h>
#include <linux/dma-mapping.h>
#include "te_shmem.h"

static struct te_shmem_pool shmem_pool;

u32 te_generic_smc(u32 arg0, u32 arg1, u32 arg2, u32 arg3);

int te_shmem_init(void)
{
	u32  vaddr;
	u32  paddr;
	u32  size;
	
	/* allocate share memory */
	size  = TEE_SHMEM_SIZE;
	vaddr=	__get_free_pages(GFP_KERNEL, get_order(size));
	if (vaddr == 0) {
		pr_err("%s: allocate shmem fail\n", __func__);
		return -ENOMEM;
	}
	paddr = virt_to_phys(vaddr);
	
	/* initialize shmem pool */
	shmem_pool.size  = size;
	shmem_pool.vaddr = vaddr;
	shmem_pool.paddr = paddr;
	
	/* register to secure world */
	te_generic_smc(TEE_SMC_CONFIG_SHMEM, paddr, size, 0);
	
	return 0;
}

static bool te_shmem_is_valid_paddr(u32 paddr)
{
	if ((paddr >= shmem_pool.paddr) && 
	    (paddr <  (shmem_pool.paddr + shmem_pool.size))) {
		return 1;
	}
	return 0;
}

static bool te_shmem_is_valid_vaddr(u32 vaddr)
{
	if ((vaddr >= shmem_pool.vaddr) && 
	    (vaddr <  (shmem_pool.vaddr + shmem_pool.size))) {
		return 1;
	}
	return 0;
}

u32 te_shmem_pa2va(u32 paddr)
{
	u32   offset;
	u32   vaddr;
	
	if (!te_shmem_is_valid_paddr(paddr)) {
		pr_err("%s: invalid paddr=%x\n", __func__, paddr);
		return 0;
	}
	offset = paddr - shmem_pool.paddr;
	vaddr  = shmem_pool.vaddr + offset;
	
	return vaddr;
}
EXPORT_SYMBOL(te_shmem_pa2va);
u32 te_shmem_va2pa(u32 vaddr)
{
	u32   offset;
	u32   paddr;
	
	if (!te_shmem_is_valid_vaddr(vaddr)) {
		pr_err("%s: invalid vaddr=%x\n", __func__, vaddr);
		return 0;
	}
	offset = vaddr - shmem_pool.vaddr;
	paddr  = shmem_pool.paddr + offset;
	
	return paddr;
}
