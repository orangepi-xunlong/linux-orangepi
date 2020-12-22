
/**
 * tee RPC implemention
 */

#ifndef __TE_RPC_H__
#define __TE_RPC_H__
#include <mach/sunxi-smc.h>

/* RPC defines */
#define TEE_SMC_RPC_PREFIX_MASK	 0xFFFF0000
#define TEE_SMC_RPC_PREFIX	 0xFAFA0000
#define TEE_SMC_RPC_FUNC_MASK	 0x0000FFFF
#define TEE_SMC_RPC_FUNC(val)	 ((val) & TEE_SMC_RPC_FUNC_MASK)
#define TEE_SMC_IS_RPC_CALL(val) (((val) & TEE_SMC_RPC_PREFIX_MASK) == \
							TEE_SMC_RPC_PREFIX)
#define TEE_SMC_RPC_VAL(func)    ((func) | TEE_SMC_RPC_PREFIX)


/* RPC services command define */
#define TEE_SMC_RPC_GET_TIME     (0x00A0)
#define TEE_SMC_RPC_SST_COMMAND  (0x00B0)

int  te_shmem_init(void);
unsigned int  te_shmem_pa2va(unsigned int paddr);
unsigned int  te_shmem_va2pa(unsigned int vaddr);


int te_rpc_handle(struct smc_param *param);

#endif /* __TE_RPC_H__ */
