/*************************************************************************/ /*!
@File
@Copyright      Copyright (c) Imagination Technologies Ltd. All Rights Reserved
@License        Dual MIT/GPLv2

The contents of this file are subject to the MIT license as set out below.

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

Alternatively, the contents of this file may be used under the terms of
the GNU General Public License Version 2 ("GPL") in which case the provisions
of GPL are applicable instead of those above.

If you wish to allow use of your version of this file only under the terms of
GPL, and not to allow others to use your version of this file under the terms
of the MIT license, indicate your decision by deleting the provisions above
and replace them with the notice and other provisions required by GPL as set
out in the file called "GPL-COPYING" included in this distribution. If you do
not delete the provisions above, a recipient may use your version of this file
under the terms of either the MIT license or GPL.

This License is also included in this distribution in the file called
"MIT-COPYING".

EXCEPT AS OTHERWISE STATED IN A NEGOTIATED AGREEMENT: (A) THE SOFTWARE IS
PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING
BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
PURPOSE AND NONINFRINGEMENT; AND (B) IN NO EVENT SHALL THE AUTHORS OR
COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/ /**************************************************************************/

#ifndef TEE_FW_PREMAP_H
#define TEE_FW_PREMAP_H

#include "rgxlayer.h"
#include "tee_ddk.h"

#define PVR_ASSERT(x)

/* MMU Protection flags */
/* These are specified generically and in a h/w independent way, and
   are interpreted at each level (PC/PD/PT) separately. */
#define PROT_FLAG                           0
#define MMU_PROTFLAGS_INVALID               0x80000000U

/* The following flags should be supplied by the caller: */
#define MMU_PROTFLAGS_READABLE              (1U<<0)
#define MMU_PROTFLAGS_WRITEABLE             (1U<<1)
#define MMU_PROTFLAGS_CACHE_COHERENT        (1U<<2)
#define MMU_PROTFLAGS_CACHED                (1U<<3)

#define MMU_MAX_LEVEL  3

/*!< Memory that only the PM and Meta can access */
#define PMMETA_PROTECT                      (1U << 0)

/* Device specific MMU flags */
#define MMU_PROTFLAGS_DEVICE_OFFSET         16
#define MMU_PROTFLAGS_DEVICE_MASK           0x000F0000UL
#define MMU_PROTFLAGS_DEVICE(n)	\
			(((n) << MMU_PROTFLAGS_DEVICE_OFFSET) & \
			MMU_PROTFLAGS_DEVICE_MASK)

#if (RGX_FEATURE_MMU_VERSION != 4)
#error "Only cores with MMU_VERSION 4 currently supported."
#endif

#define RGX_MMUCTRL_PTE_PROTMASK	(RGX_MMUCTRL_PT_DATA_PM_META_PROTECT_EN | \
		~RGX_MMUCTRL_PT_DATA_AXCACHE_CLRMSK | \
		RGX_MMUCTRL_PT_DATA_ENTRY_PENDING_EN | \
		RGX_MMUCTRL_PT_DATA_PM_SRC_EN | \
		RGX_MMUCTRL_PT_DATA_CC_EN | \
		RGX_MMUCTRL_PT_DATA_READ_ONLY_EN | \
		RGX_MMUCTRL_PT_DATA_VALID_EN)

/*
 * protection bits for MMU_VERSION >= 4
 * MMU4 has no PENDING or PAGE_SIZE fields in PxE
 */
#define RGX_MMU4CTRL_PTE_PROTMASK	(RGX_MMUCTRL_PTE_PROTMASK & ~RGX_MMUCTRL_PT_DATA_ENTRY_PENDING_EN)

#define RGX_MMU4CTRL_PDE_PROTMASK	(RGX_MMUCTRL_PD_DATA_VALID_EN)

#define RGX_MMU4CTRL_PCE_PROTMASK	(RGX_MMUCTRL_PC_DATA_VALID_EN)

/*
	The Memory Management Unit (MMU) performs device virtual to physical
	translation.

	Terminology:
	 - page catalogue, PC (optional, 3 tier MMU)
	 - page directory, PD
	 - page table, PT (can be variable sized)
	 - data page, DP (can be variable sized)
	Note: PD and PC are fixed size and can't be larger than the native
	      physical (CPU) page size
*/

/*!
	All physical allocations and frees are relative to this context, so
	we would get all the allocations of PCs, PDs, and PTs from the same
	RA.

	We have one per MMU context in case we have mixed UMA/LMA devices
	within the same system.
 */
typedef struct _MMU_PHYSMEM_CONTEXT_
{
	/*! Associated MMU_CONTEXT */
	struct _MMU_CONTEXT_ *psMMUContext;
} MMU_PHYSMEM_CONTEXT;

/*!
	Memory descriptor for MMU objects. There can be more than one memory
	descriptor per MMU memory allocation.
 */
typedef struct _MMU_MEMORY_DESC_
{
	/*! Device Physical address of physical backing */
	IMG_DEV_PHYADDR sDevPAddr;
	/*! CPU virtual address of physical backing */
	void *pvCpuVAddr;
	/*! Size of the Memdesc */
	IMG_UINT32 uiSize;
} MMU_MEMORY_DESC;

/*!
	MMU level structure. This is generic and is used
	for all levels (PC, PD, PT).
 */
typedef struct _MMU_LEVEL_INFO_
{
	/*! MemDesc for this level */
	MMU_MEMORY_DESC sMemDesc;

	/*! Array of infos for the next level. Must be last member in structure */
	struct _MMU_LEVEL_INFO_ *apsNextLevel[1];
} MMU_LEVEL_INFO;

/*!
	MMU context structure
 */
typedef struct _MMU_CONTEXT_
{
	/*! For allocation and deallocation of the physical memory where
	    the pagetables live */
	struct _MMU_PHYSMEM_CONTEXT_ *psPhysMemCtx;

	/*! Base level info structure. Must be last member in structure */
	MMU_LEVEL_INFO sBaseLevelInfo;
	/* NO OTHER MEMBERS AFTER THIS STRUCTURE ! */
} MMU_CONTEXT;

/*!
	The level of the MMU
*/
typedef enum
{
	MMU_LEVEL_0 = 0,	/* Level 0 = Page */
	MMU_LEVEL_1,		/* Level 1 = Page Table */
	MMU_LEVEL_2,		/* Level 2 = Page Directory */
	MMU_LEVEL_3,		/* Level 1 = Page Catalog */
	MMU_LEVEL_LAST
} MMU_LEVEL;

/*
	Device Virtual Address Config:
	Incoming Device Virtual Address is deconstructed into up to 4
	fields, where the virtual address is up to 64bits:
	MSB-----------------------------------------------LSB
	| PC Index:   | PD Index:  | PT Index: | DP offset: |
	| d bits      | c bits     | b-v bits  |  a+v bits  |
	-----------------------------------------------------
	where v is the variable page table modifier, e.g.
			v == 0  -> 4KB DP
			v == 2  -> 16KB DP
			v == 4  -> 64KB DP
			v == 6  -> 256KB DP
			v == 8  -> 1MB DP
			v == 10 -> 4MB DP
*/

#define PC_INDEX_MASK                       0xFFC0000000
#define PC_INDEX_SHIFT                      30
#define PD_INDEX_MASK                       0x003FE00000
#define PD_INDEX_SHIFT                      21
/* (v == 0) --> 4KB page table setting */
#define PT_INDEX_MASK                       0x00001FF000
#define PT_INDEX_SHIFT                      12

/*
	P(C/D/T) Entry Config:
	MSB-----------------------------------------------LSB
	| PT Addr:   | variable PT ctrl | protection flags: |
	| bits c+v   | b bits           | a bits            |
	-----------------------------------------------------
	where v is the variable page table modifier and is optional
*/
typedef struct _MMU_PxE_CONFIG_
{
	MMU_LEVEL	ePxLevel;        /*! MMU Level this config describes */
	IMG_UINT8	uiBytesPerEntry; /*! Size of an entry in bytes */

	IMG_UINT64	uiAddrMask;      /*! Physical address mask */
	IMG_UINT8	uiAddrShift;     /*! Physical address shift */
	IMG_UINT8	uiAddrLog2Align; /*! Physical address Log 2 alignment */

	IMG_UINT64	uiVarCtrlMask;   /*! Variable control mask */
	IMG_UINT8	uiVarCtrlShift;  /*! Variable control shift */

	IMG_UINT64	uiProtMask;      /*! Protection flags mask */
	IMG_UINT8	uiProtShift;     /*! Protection flags shift */

	IMG_UINT64	uiValidEnMask;   /*! Entry valid bit mask */
	IMG_UINT8	uiValidEnShift;  /*! Entry valid bit shift */
} MMU_PxE_CONFIG;

/*!
	MMU virtual address split
*/
typedef struct _MMU_DEVVADDR_CONFIG_
{
	/*! Page catalogue index mask */
	IMG_UINT64	uiPCIndexMask;
	/*! Page catalogue index shift */
	IMG_UINT8	uiPCIndexShift;
	/*! Total number of PC entries */
	IMG_UINT32	uiNumEntriesPC;

	/*! Page directory mask */
	IMG_UINT64	uiPDIndexMask;
	/*! Page directory shift */
	IMG_UINT8	uiPDIndexShift;
	/*! Total number of PD entries */
	IMG_UINT32	uiNumEntriesPD;

	/*! Page table mask */
	IMG_UINT64	uiPTIndexMask;
	/*! Page index shift */
	IMG_UINT8	uiPTIndexShift;
	/*! Total number of PT entries */
	IMG_UINT32	uiNumEntriesPT;

	/*! Page offset mask */
	IMG_UINT64	uiPageOffsetMask;
	/*! Page offset shift */
	IMG_UINT8	uiPageOffsetShift;

	/*! First virtual address mappable for this config */
	IMG_UINT64	uiOffsetInBytes;

} MMU_DEVVADDR_CONFIG;

/*************************************************************************/ /*!
@Function       PVRSRVConfigureMMU
@Description    Set up the page tables for the Firmware's context
@Input          psSysData       System's heap details
 */
/*****************************************************************************/
PVRSRV_ERROR PVRSRVConfigureMMU(SYS_DATA *psSysData);

#endif /* #ifdef TEE_FW_PREMAP_H */
