
/**
 * tee RPC implemention
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
#include <linux/secure/te_protocol.h>
#include <linux/secure/te_rpc.h>
#include <linux/secure/te_sec_storage.h>
#include "te_shmem.h"


unsigned int (*rpc_handle)(unsigned int, unsigned int , unsigned int) = NULL;

unsigned int sst_rpc_handle(
				unsigned int cmd,
				unsigned int args, 
				unsigned int len)
{
	if(rpc_handle){
		return (*rpc_handle)(cmd, args, len);

	}else
		return -1 ;
}
			
int te_rpc_handle(struct smc_param *param)
{
	int ret ;
	struct timespec ts;
	switch (TEE_SMC_RPC_FUNC(param->a0)) {
	case TEE_SMC_RPC_GET_TIME:
		getnstimeofday(&ts);
		param->a1 = ts.tv_sec;             /* seconds */
		param->a2 = ts.tv_nsec / 1000000;  /* milliseconds */
		break;
	case TEE_SMC_RPC_SST_COMMAND:
		ret = sst_rpc_handle(param->a0, param->a1, param->a2);
		param->a0 = ret ;
		break;
	default:
		pr_warn("%s: invalid RPC func 0x%x\n", 
		         __func__, TEE_SMC_RPC_FUNC(param->a0));
		break;
	}
	return 0;
}

int sst_rpc_register(void *handle)
{
	if(handle)
		rpc_handle = handle ;	
	return 0 ;
}

EXPORT_SYMBOL(sst_rpc_register);

int sst_rpc_unregister(void)
{
	rpc_handle = NULL ;	
	return 0 ;
} 

EXPORT_SYMBOL(sst_rpc_unregister);

