#include <linux/arm_sdei.h>
#include <asm/cacheflush.h>
#include <linux/soc/cix/rdr_platform.h>
#include "dst_print.h"

static int tfa_trace_dump(void *dump_addr, unsigned int size)
{
	/*invalid cache*/
	dcache_inval_poc((unsigned long)dump_addr,
			 (unsigned long)((char *)dump_addr + size));

	return 0;
}

static int __init dst_tfa_trace_init(void)
{
	unsigned char *virt_addr;
	uintptr_t phy_addr;
	u32 size;
	u64 os_size = 0;

	DST_PRINT_START();
	if (get_module_dump_mem_addr(MODU_TFA, &virt_addr)) {
		DST_PRINT_ERR("%s, get module memory failed. \n", __func__);
		return 0;
	}
	if (get_module_dump_mem_size(MODU_TFA, &size)) {
		DST_PRINT_ERR("%s, get module memory size failed. \n",
			      __func__);
		return 0;
	}

	phy_addr = (vmalloc_to_pfn(virt_addr) << PAGE_SHIFT) +
		   ((u64)virt_addr & ((1 << PAGE_SHIFT) - 1));
	DST_PRINT_PN("%s, phys memory address=0x%lx, size=0x%x\n", __func__,
		     phy_addr, size);

	if (sdei_api_event_set_info(size, SDEI_EVENT_SET_TFA_TRACE_MEMORY,
				    (u64)phy_addr)) {
		DST_PRINT_ERR("%s, set sdei tfa trace memory failed. \n",
			      __func__);
		return 0;
	}
	register_module_dump_mem_func(tfa_trace_dump, "tfa", MODU_TFA);

#ifdef CONFIG_PLAT_KERNELDUMP
	/*set tfa flush cache size*/
	os_size = memblock_end_of_DRAM() - memstart_addr;
	if (sdei_api_event_set_info(0, SDEI_EVENT_SET_OS_MEM_SIZE, os_size)) {
		DST_PRINT_ERR("%s, set os memory failed. \n", __func__);
	}
#endif

	DST_PRINT_END();

	return 0;
}

late_initcall(dst_tfa_trace_init);
