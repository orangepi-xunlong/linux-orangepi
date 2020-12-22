
/**
 * secure and non-secure share memory manager
 */

#ifndef __TE_SHMEM_H_
#define __TE_SHMEM_H_

#define TEE_SHMEM_SIZE  (8192)

struct te_shmem_pool {
	size_t  size;
	u32     vaddr;
	u32     paddr;
};

int  te_shmem_init(void);
u32  te_shmem_pa2va(u32 paddr);
u32  te_shmem_va2pa(u32 vaddr);

#endif /* __TE_SHMEM_H_ */
