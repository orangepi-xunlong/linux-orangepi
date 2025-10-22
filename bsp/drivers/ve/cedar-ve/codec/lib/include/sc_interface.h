/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
* Copyright (c) 2008-2016 Allwinner Technology Co. Ltd.
* All rights reserved.
*
* File : typedef.h
* Description :
* History :
*   Author  : xyliu <xyliu@allwinnertech.com>
*   Date    : 2016/04/13
*   Comment :
*
*
*/


#ifndef SC_INTERFACE_H
#define SC_INTERFACE_H

#ifdef __cplusplus
extern "C"
{
#endif

#ifdef CONFIG_AW_VIDEO_KERNEL_ENC
#include <linux/types.h>
#else
#include <stddef.h>
#include <stdint.h>
#endif

struct ScMemOpsS {
	int (*open)(void);
	int  (*open2)(void *, void *);
	void (*close)(void);

	int (*total_size)(void);

	void *(*palloc)(int /*size*/, void *, void *);

	void *(*palloc_no_cache)(int /*size*/, void *, void *);

	void  (*pfree)(void */*mem*/, void *, void *);

	void (*flush_cache)(void  */*mem*/, int /*size*/);

	void *(*ve_get_phyaddr)(void  */*viraddr*/);

	void *(*ve_get_viraddr)(void  */*phyaddr*/);

	void *(*cpu_get_phyaddr)(void  */*viraddr*/);

	void *(*cpu_get_viraddr)(void  */*phyaddr*/);

	int (*mem_set)(void  */*s*/, int /*c*/, size_t /*n*/);

	int (*mem_cpy)(void  */*dest*/, void  */*src*/, size_t /*n*/);

	int (*mem_read)(void  */*dest */, void  */*src*/, size_t /*n*/);

	int (*mem_write)(void  */*dest*/, void  */*src*/, size_t /*n*/);

	int (*setup)(void);

	int (*shutdown)(void);

	//*the interface just for secureAlloc.c, not used by other function
	void *(*palloc_secure)(int /*size*/, void*, void*);

	unsigned int (*get_ve_addr_offset)(void);

	int (*get_debug_info)(char*, int);

	int (*get_vir_by_fd)(int/*dmafd*/, void */*phy*/);
	int (*get_phy_by_fd)(int/*dmafd*/, void */*phy*/);
	int (*free_phy_by_fd)(int/*dmafd*/, unsigned long /*phy*/);
	int (*get_fd_by_vir)(void  */*viraddr*/);
};

static inline int CdcMemOpen(struct ScMemOpsS *memops)
{
	return memops->open();
}

//* close the memory adapter.
static inline void CdcMemClose(struct ScMemOpsS *memops)
{
	memops->close();
}

static inline int CdcMemTotalSize(struct ScMemOpsS *memops)
{
	return memops->total_size();
}

static inline void *CdcMemPalloc(struct ScMemOpsS *memops, int nSize, void *veOps, void *pVeopsSelf)
{
	return memops->palloc(nSize, veOps, pVeopsSelf);
}

static inline void CdcMemPfree(struct ScMemOpsS *memops, void *pMem, void *veOps, void *pVeopsSelf)
{
	memops->pfree(pMem, veOps, pVeopsSelf);
}

static inline void CdcMemFlushCache(struct ScMemOpsS *memops, void *pMem, int nSize)
{
	memops->flush_cache(pMem, nSize);
}

static inline void *CdcMemGetPhysicAddress(struct ScMemOpsS *memops, void *pVirtualAddress)
{
	return memops->ve_get_phyaddr(pVirtualAddress);
}

static inline void *CdcMemGetVirtualAddress(struct ScMemOpsS *memops, void *pPhysicAddress)
{
	return memops->ve_get_viraddr(pPhysicAddress);
}

static inline void CdcMemSet(struct ScMemOpsS *memops, void *pMem, int nValue, int nSize)
{
   memops->mem_set(pMem, nValue, nSize);
}

static inline void CdcMemCopy(struct ScMemOpsS *memops, void *pMemDst, void *pMemSrc, int nSize)
{
	memops->mem_cpy(pMemDst, pMemSrc, nSize);
}

static inline int CdcMemRead(struct ScMemOpsS *memops, void *pMemDst, void *pMemSrc, int nSize)
{
	memops->mem_read(pMemDst, pMemSrc, nSize);
	return 0;
}

static inline int CdcMemWrite(struct ScMemOpsS *memops, void *pMemDst,  void *pMemSrc, int nSize)
{
	(void)memops; /*not use memops */
	memops->mem_write(pMemDst, pMemSrc, nSize);
	return 0;
}

static inline void *CdcMemGetPhysicAddressCpu(struct ScMemOpsS *memops, void *virt)
{
	return memops->cpu_get_phyaddr(virt);
}

static inline void *CdcMemGetVirtualAddressCpu(struct ScMemOpsS *memops, void *phy)
{
	return memops->cpu_get_viraddr(phy);
}

static inline int CdcMemSetup(struct ScMemOpsS *memops)
{
	return memops->setup();
}

static inline int CdcMemShutdown(struct ScMemOpsS *memops)
{
	return memops->shutdown();
}

static inline void *CdcMemPallocSecure(struct ScMemOpsS *memops, int nSize,
											  void *veOps, void *pVeopsSelf)
{
	return memops->palloc_secure(nSize, veOps, pVeopsSelf);
}

static inline unsigned int CdcMemPallocGetVeAddrOffset(struct ScMemOpsS *memops)
{
	return memops->get_ve_addr_offset();
}

#ifdef __cplusplus
}
#endif

#endif

