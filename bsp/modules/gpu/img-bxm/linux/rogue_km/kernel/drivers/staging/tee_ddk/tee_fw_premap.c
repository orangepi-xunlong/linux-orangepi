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

#include "tee_fw_premap.h"
#include "rgxmmudefs_km.h"

#include <linux/slab.h>
#include <linux/compiler_types.h>
#include <asm/io.h>

static const MMU_PxE_CONFIG sRGXMMUPCEConfig = {
	.ePxLevel			= MMU_LEVEL_3,
	.uiBytesPerEntry	= 4,
	.uiAddrMask			= 0xfffffff0,
	.uiAddrShift		= 4,
	.uiAddrLog2Align	= 12,
	.uiProtMask			= RGX_MMU4CTRL_PCE_PROTMASK,
	.uiProtShift		= 0,
	.uiValidEnMask		= RGX_MMUCTRL_PC_DATA_VALID_EN,
	.uiValidEnShift		= RGX_MMUCTRL_PC_DATA_VALID_SHIFT
};

#if (PAGE_SHIFT == 12)
/*
 *  Configuration for heaps with 4kB Data-Page size
 *
 * Bit  39      30   29   21   20   12   11        0
 *      |         \ /       \ /       \ /           \
 *      |....PCE...|...PDE...|...PTE...|....VAddr...|
 */
static const MMU_PxE_CONFIG sRGXMMUPDEConfig = {
	.ePxLevel			= MMU_LEVEL_2,
	.uiBytesPerEntry	= 8,
	.uiAddrMask			= IMG_UINT64_C(0xfffffffff0),
	.uiAddrShift		= 12,
	.uiAddrLog2Align	= 12,
	.uiProtMask			= RGX_MMU4CTRL_PDE_PROTMASK,
	.uiProtShift		= 0,
	.uiValidEnMask		= RGX_MMUCTRL_PD_DATA_VALID_EN,
	.uiValidEnShift		= RGX_MMUCTRL_PD_DATA_VALID_SHIFT
};

static const MMU_PxE_CONFIG sRGXMMUPTEConfig = {
	.ePxLevel			= MMU_LEVEL_1,
	.uiBytesPerEntry	= 8,
	.uiAddrMask			= IMG_UINT64_C(0xfffffff000),
	.uiAddrShift		= 12,
	.uiAddrLog2Align	= 12,
	.uiProtMask			= RGX_MMU4CTRL_PTE_PROTMASK,
	.uiProtShift		= 0,
	.uiValidEnMask		= RGX_MMUCTRL_PT_DATA_VALID_EN,
	.uiValidEnShift		= RGX_MMUCTRL_PT_DATA_VALID_SHIFT
};

static const MMU_DEVVADDR_CONFIG sRGXMMUDevVAddrConfig = {
	.uiPCIndexMask		= ~RGX_MMUCTRL_VADDR_PC_INDEX_CLRMSK,
	.uiPCIndexShift		= RGX_MMUCTRL_VADDR_PC_INDEX_SHIFT,
	.uiNumEntriesPC		= 1024,

	.uiPDIndexMask		= ~RGX_MMUCTRL_VADDR_PD_INDEX_CLRMSK,
	.uiPDIndexShift		= RGX_MMUCTRL_VADDR_PD_INDEX_SHIFT,
	.uiNumEntriesPD		= 512,

	.uiPTIndexMask		= ~RGX_MMUCTRL_VADDR_PT_INDEX_CLRMSK,
	.uiPTIndexShift		= RGX_MMUCTRL_VADDR_PT_INDEX_SHIFT,
	.uiNumEntriesPT		= 512,

	.uiPageOffsetMask	= IMG_UINT64_C(0x0000000fff),
	.uiPageOffsetShift	= 0,
	.uiOffsetInBytes	= 0
};
#elif (PAGE_SHIFT == 14)
	/*
	 *  Configuration for heaps with 16kB Data-Page size
	 *
	 * Bit  39      30   29   21   20 14   13          0
	 *      |         \ /       \ /     \ /             \
	 *      |....PCE...|...PDE...|..PTE..|.....VAddr....|
	 */
static const MMU_PxE_CONFIG sRGXMMUPDEConfig = {
	.ePxLevel			= MMU_LEVEL_2,
	.uiBytesPerEntry	= 8,
	.uiAddrMask			= IMG_UINT64_C(0xfffffffff0),
	.uiAddrShift		= 10,
	.uiAddrLog2Align	= 10,
	.uiProtMask			= RGX_MMU4CTRL_PDE_PROTMASK,
	.uiProtShift		= 0,
	.uiValidEnMask		= RGX_MMUCTRL_PD_DATA_VALID_EN,
	.uiValidEnShift		= RGX_MMUCTRL_PD_DATA_VALID_SHIFT
};

static const MMU_PxE_CONFIG sRGXMMUPTEConfig = {
	.ePxLevel			= MMU_LEVEL_1,
	.uiBytesPerEntry	= 8,
	.uiAddrMask			= IMG_UINT64_C(0xffffffc000),
	.uiAddrShift		= 14,
	.uiAddrLog2Align	= 14,
	.uiProtMask			= RGX_MMU4CTRL_PTE_PROTMASK,
	.uiProtShift		= 0,
	.uiValidEnMask		= RGX_MMUCTRL_PT_DATA_VALID_EN,
	.uiValidEnShift		= RGX_MMUCTRL_PT_DATA_VALID_SHIFT
};

static const MMU_DEVVADDR_CONFIG sRGXMMUDevVAddrConfig = {
	.uiPCIndexMask		= ~RGX_MMUCTRL_VADDR_PC_INDEX_CLRMSK,
	.uiPCIndexShift		= RGX_MMUCTRL_VADDR_PC_INDEX_SHIFT,
	.uiNumEntriesPC		= 1024,

	.uiPDIndexMask		= ~RGX_MMUCTRL_VADDR_PD_INDEX_CLRMSK,
	.uiPDIndexShift		= RGX_MMUCTRL_VADDR_PD_INDEX_SHIFT,
	.uiNumEntriesPD		= 512,

	.uiPTIndexMask		= IMG_UINT64_C(0x00001fc000),
	.uiPTIndexShift		= 14,
	.uiNumEntriesPT		= 128,

	.uiPageOffsetMask	= IMG_UINT64_C(0x0000003fff),
	.uiPageOffsetShift	= 0,
	.uiOffsetInBytes	= 0
};
#elif (PAGE_SHIFT == 16)
	/*
	 *  Configuration for heaps with 64kB Data-Page size
	 *
	 * Bit  39      30   29   21  20 16 15             0
	 *      |         \ /       \ /   \ /               \
	 *      |....PCE...|...PDE...|.PTE.|.....VAddr......|
	 */
static const MMU_PxE_CONFIG sRGXMMUPDEConfig = {
	.ePxLevel			= MMU_LEVEL_2,
	.uiBytesPerEntry	= 8,
	.uiAddrMask			= IMG_UINT64_C(0xfffffffff0),
	.uiAddrShift		= 8,
	.uiAddrLog2Align	= 8,
	.uiProtMask			= RGX_MMU4CTRL_PDE_PROTMASK,
	.uiProtShift		= 0,
	.uiValidEnMask		= RGX_MMUCTRL_PD_DATA_VALID_EN,
	.uiValidEnShift		= RGX_MMUCTRL_PD_DATA_VALID_SHIFT
};

static const MMU_PxE_CONFIG sRGXMMUPTEConfig = {
	.ePxLevel			= MMU_LEVEL_1,
	.uiBytesPerEntry	= 8,
	.uiAddrMask			= IMG_UINT64_C(0xffffff0000),
	.uiAddrShift		= 16,
	.uiAddrLog2Align	= 16,
	.uiProtMask			= RGX_MMU4CTRL_PTE_PROTMASK,
	.uiProtShift		= 0,
	.uiValidEnMask		= RGX_MMUCTRL_PT_DATA_VALID_EN,
	.uiValidEnShift		= RGX_MMUCTRL_PT_DATA_VALID_SHIFT
};

static const MMU_DEVVADDR_CONFIG sRGXMMUDevVAddrConfig = {
	.uiPCIndexMask		= ~RGX_MMUCTRL_VADDR_PC_INDEX_CLRMSK,
	.uiPCIndexShift		= RGX_MMUCTRL_VADDR_PC_INDEX_SHIFT,
	.uiNumEntriesPC		= 1024,

	.uiPDIndexMask		= ~RGX_MMUCTRL_VADDR_PD_INDEX_CLRMSK,
	.uiPDIndexShift		= RGX_MMUCTRL_VADDR_PD_INDEX_SHIFT,
	.uiNumEntriesPD		= 512,

	.uiPTIndexMask		= IMG_UINT64_C(0x00001f0000),
	.uiPTIndexShift		= 16,
	.uiNumEntriesPT		= 32,

	.uiPageOffsetMask	= IMG_UINT64_C(0x000000ffff),
	.uiPageOffsetShift	= 0,
	.uiOffsetInBytes	= 0
};
#elif (PAGE_SHIFT == 18)
	/*
	 *  Configuration for heaps with 256kB Data-Page size
	 *
	 * Bit  39      30   29   21 20 18 17              0
	 *      |         \ /       \|   |/                 \
	 *      |....PCE...|...PDE...|PTE|.......VAddr......|
	 */
static const MMU_PxE_CONFIG sRGXMMUPDEConfig = {
	.ePxLevel			= MMU_LEVEL_2,
	.uiBytesPerEntry	= 8,
	.uiAddrMask			= IMG_UINT64_C(0xfffffffff0),
	.uiAddrShift		= 6,
	.uiAddrLog2Align	= 6,
	.uiProtMask			= RGX_MMU4CTRL_PDE_PROTMASK,
	.uiProtShift		= 0,
	.uiValidEnMask		= RGX_MMUCTRL_PD_DATA_VALID_EN,
	.uiValidEnShift		= RGX_MMUCTRL_PD_DATA_VALID_SHIFT
};

static const MMU_PxE_CONFIG sRGXMMUPTEConfig = {
	.ePxLevel			= MMU_LEVEL_1,
	.uiBytesPerEntry	= 8,
	.uiAddrMask			= IMG_UINT64_C(0xfffffc0000),
	.uiAddrShift		= 18,
	.uiAddrLog2Align	= 18,
	.uiProtMask			= RGX_MMU4CTRL_PTE_PROTMASK,
	.uiProtShift		= 0,
	.uiValidEnMask		= RGX_MMUCTRL_PT_DATA_VALID_EN,
	.uiValidEnShift		= RGX_MMUCTRL_PT_DATA_VALID_SHIFT
};

static const MMU_DEVVADDR_CONFIG sRGXMMUDevVAddrConfig = {
	.uiPCIndexMask		= ~RGX_MMUCTRL_VADDR_PC_INDEX_CLRMSK,
	.uiPCIndexShift		= RGX_MMUCTRL_VADDR_PC_INDEX_SHIFT,
	.uiNumEntriesPC		= 1024,

	.uiPDIndexMask		= ~RGX_MMUCTRL_VADDR_PD_INDEX_CLRMSK,
	.uiPDIndexShift		= RGX_MMUCTRL_VADDR_PD_INDEX_SHIFT,
	.uiNumEntriesPD		= 512,

	.uiPTIndexMask		= IMG_UINT64_C(0x00001c0000),
	.uiPTIndexShift		= 18,
	.uiNumEntriesPT		= 8,

	.uiPageOffsetMask	= IMG_UINT64_C(0x000003ffff),
	.uiPageOffsetShift	= 0,
	.uiOffsetInBytes	= 0
};
#elif (PAGE_SHIFT == 20)
	/*
	 *  Configuration for heaps with 1MB Data-Page size
	 *
	 * Bit  39      30   29   21 20  19               0
	 *      |         \ /       \ | /                  \
	 *      |....PCE...|...PDE...|.|........VAddr.......|
	 */
static const MMU_PxE_CONFIG sRGXMMUPDEConfig = {
	.ePxLevel			= MMU_LEVEL_2,
	.uiBytesPerEntry	= 8,
	.uiAddrMask			= IMG_UINT64_C(0xfffffffff0),
	.uiAddrShift		= 6,
	.uiAddrLog2Align	= 6,
	.uiProtMask			= RGX_MMU4CTRL_PDE_PROTMASK,
	.uiProtShift		= 0,
	.uiValidEnMask		= RGX_MMUCTRL_PD_DATA_VALID_EN,
	.uiValidEnShift		= RGX_MMUCTRL_PD_DATA_VALID_SHIFT
};

static const MMU_PxE_CONFIG sRGXMMUPTEConfig = {
	.ePxLevel			= MMU_LEVEL_1,
	.uiBytesPerEntry	= 8,
	.uiAddrMask			= IMG_UINT64_C(0xfffff00000),
	.uiAddrShift		= 20,
	.uiAddrLog2Align	= 20,
	.uiProtMask			= RGX_MMU4CTRL_PTE_PROTMASK,
	.uiProtShift		= 0,
	.uiValidEnMask		= RGX_MMUCTRL_PT_DATA_VALID_EN,
	.uiValidEnShift		= RGX_MMUCTRL_PT_DATA_VALID_SHIFT
};

static const MMU_DEVVADDR_CONFIG sRGXMMUDevVAddrConfig = {
	.uiPCIndexMask		= ~RGX_MMUCTRL_VADDR_PC_INDEX_CLRMSK,
	.uiPCIndexShift		= RGX_MMUCTRL_VADDR_PC_INDEX_SHIFT,
	.uiNumEntriesPC		= 1024,

	.uiPDIndexMask		= ~RGX_MMUCTRL_VADDR_PD_INDEX_CLRMSK,
	.uiPDIndexShift		= RGX_MMUCTRL_VADDR_PD_INDEX_SHIFT,
	.uiNumEntriesPD		= 512,

	.uiPTIndexMask		= IMG_UINT64_C(0x0000100000),
	.uiPTIndexShift		= 20,
	.uiNumEntriesPT		= 2,

	.uiPageOffsetMask	= IMG_UINT64_C(0x00000fffff),
	.uiPageOffsetShift	= 0,
	.uiOffsetInBytes	= 0
};
#elif (PAGE_SHIFT == 21)
	/*
	 *  Configuration for heaps with 2MB Data-Page size
	 *
	 * Bit  39      30   29   21   20                0
	 *      |         \ /       \ /                   \
	 *      |....PCE...|...PDE...|.........VAddr.......|
	 */
static const MMU_PxE_CONFIG sRGXMMUPDEConfig = {
	.ePxLevel			= MMU_LEVEL_2,
	.uiBytesPerEntry	= 8,
	.uiAddrMask			= IMG_UINT64_C(0xfffffffff0),
	.uiAddrShift		= 6,
	.uiAddrLog2Align	= 6,
	.uiProtMask			= RGX_MMU4CTRL_PDE_PROTMASK,
	.uiProtShift		= 0,
	.uiValidEnMask		= RGX_MMUCTRL_PD_DATA_VALID_EN,
	.uiValidEnShift		= RGX_MMUCTRL_PD_DATA_VALID_SHIFT
};

static const MMU_PxE_CONFIG sRGXMMUPTEConfig = {
	.ePxLevel			= MMU_LEVEL_1,
	.uiBytesPerEntry	= 8,
	.uiAddrMask			= IMG_UINT64_C(0xffffe00000),
	.uiAddrShift		= 21,
	.uiAddrLog2Align	= 21,
	.uiProtMask			= RGX_MMU4CTRL_PTE_PROTMASK,
	.uiProtShift		= 0,
	.uiValidEnMask		= RGX_MMUCTRL_PT_DATA_VALID_EN,
	.uiValidEnShift		= RGX_MMUCTRL_PT_DATA_VALID_SHIFT
};

static const MMU_DEVVADDR_CONFIG sRGXMMUDevVAddrConfig = {
	.uiPCIndexMask		= ~RGX_MMUCTRL_VADDR_PC_INDEX_CLRMSK,
	.uiPCIndexShift		= RGX_MMUCTRL_VADDR_PC_INDEX_SHIFT,
	.uiNumEntriesPC		= 1024,

	.uiPDIndexMask		= ~RGX_MMUCTRL_VADDR_PD_INDEX_CLRMSK,
	.uiPDIndexShift		= RGX_MMUCTRL_VADDR_PD_INDEX_SHIFT,
	.uiNumEntriesPD		= 512,

	.uiPTIndexMask		= IMG_UINT64_C(0x0000000000),
	.uiPTIndexShift		= 21,
	.uiNumEntriesPT		= 1,

	.uiPageOffsetMask	= IMG_UINT64_C(0x00001fffff),
	.uiPageOffsetShift	= 0,
	.uiOffsetInBytes	= 0
};
#else
#error "PAGE SIZE NOT SUPPORTED"
#endif
/*************************************************************************/ /*!
@Function       RGXMMU4DerivePCEProt4
@Description    calculate the PCE protection flags based on a 4 byte entry
 */ /**************************************************************************/
static IMG_UINT32 RGXMMU4DerivePCEProt4(IMG_UINT32 uiProtFlags)
{
	return (uiProtFlags & MMU_PROTFLAGS_INVALID)?0:RGX_MMUCTRL_PC_DATA_VALID_EN;
}

/*************************************************************************/ /*!
@Function       RGXMMU4DerivePCEProt8
@Description    calculate the PCE protection flags based on an 8 byte entry
 */ /**************************************************************************/
static IMG_UINT64 RGXMMU4DerivePCEProt8(IMG_UINT32 uiProtFlags)
{
	(void)uiProtFlags;
	RGXErrorLog(NULL, "8-byte PCE not supported on this device");
	return 0;
}

/*************************************************************************/ /*!
@Function       RGXMMU4DerivePDEProt4
@Description    derive the PDE protection flags based on a 4 byte entry
 */ /**************************************************************************/
static IMG_UINT32 RGXMMU4DerivePDEProt4(IMG_UINT32 uiProtFlags)
{
	(void)uiProtFlags;
	RGXErrorLog(NULL, "4-byte PDE not supported on this device");
	return 0;
}

/*************************************************************************/ /*!
@Function       RGXMMU4DerivePDEProt8
@Description    derive the PDE protection flags based on an 8 byte entry

@Return         PVRSRV_ERROR
*/ /**************************************************************************/
static IMG_UINT64 RGXMMU4DerivePDEProt8(IMG_UINT32 uiProtFlags)
{
	IMG_UINT64 ret_value = 0; /* 0 means invalid */

	if (!(uiProtFlags & MMU_PROTFLAGS_INVALID)) /* if not invalid */
	{
		/*  page size in range config registers. Bits in PD entries are reserved */
		ret_value = RGX_MMUCTRL_PD_DATA_VALID_EN;
	}
	return ret_value;
}


/*************************************************************************/ /*!
@Function       RGXMMU4DerivePTEProt4
@Description    calculate the PTE protection flags based on a 4 byte entry
 */ /**************************************************************************/
static IMG_UINT32 RGXMMU4DerivePTEProt4(IMG_UINT32 uiProtFlags)
{
	(void)uiProtFlags;
	RGXErrorLog(NULL, "4-byte PTE not supported on this device");
	return 0;
}

/*************************************************************************/ /*!
@Function       RGXMMU4DerivePTEProt8
@Description    calculate the PTE protection flags based on an 8 byte entry
 */ /**************************************************************************/
static IMG_UINT64 RGXMMU4DerivePTEProt8(IMG_UINT32 uiProtFlags)
{
	IMG_UINT64 ui64MMUFlags=0;

	if (((MMU_PROTFLAGS_READABLE|MMU_PROTFLAGS_WRITEABLE) & uiProtFlags) ==
	    (MMU_PROTFLAGS_READABLE|MMU_PROTFLAGS_WRITEABLE))
	{
		/* read/write */
	}
	else if (MMU_PROTFLAGS_READABLE & uiProtFlags)
	{
		/* read only */
		ui64MMUFlags |= RGX_MMUCTRL_PT_DATA_READ_ONLY_EN;
	}
	else if (MMU_PROTFLAGS_WRITEABLE & uiProtFlags)
	{
		/* write only */
		RGXErrorLog(NULL, "RGXMMU4DerivePTEProt8: write-only is not possible on this device");
	}
	else if ((MMU_PROTFLAGS_INVALID & uiProtFlags) == 0)
	{
		RGXErrorLog(NULL, "RGXMMU4DerivePTEProt8: neither read nor write specified...");
	}

	/* cache coherency */
	if (MMU_PROTFLAGS_CACHE_COHERENT & uiProtFlags)
	{
		ui64MMUFlags |= RGX_MMUCTRL_PT_DATA_CC_EN;
	}

	if ((uiProtFlags & MMU_PROTFLAGS_INVALID) == 0)
	{
		ui64MMUFlags |= RGX_MMUCTRL_PT_DATA_VALID_EN;
	}

	if (MMU_PROTFLAGS_DEVICE(PMMETA_PROTECT) & uiProtFlags)
	{
		ui64MMUFlags |= RGX_MMUCTRL_PT_DATA_PM_META_PROTECT_EN;
	}

	/**
	 * Always enable caching on the fabric level cache irrespective of type of
	 * cache coherent interconnect and memory cache attributes.
	 * This needs to be updated, if selective caching policy needs to be
	 * implemented based on cache attributes requested by caller and based on
	 * cache coherent interconnect.
	 */
	ui64MMUFlags |= RGX_MMUCTRL_PT_DATA_AXCACHE_WBRWALLOC;

	return ui64MMUFlags;
}

static PVRSRV_ERROR _CarveoutAllocator(IMG_UINT64 ui64PageTableHeapGpuBase,
                                       IMG_UINT64 ui64PageTableHeapSize,
                                       IMG_UINT32 uiBytes,
                                       IMG_UINT32 uiAlignment,
                                       IMG_UINT64 *pui64PhysAddr)
{
	static IMG_UINT64 ui64MemCursor = 0;

	if (ui64MemCursor == 0)
	{
		ui64MemCursor = ui64PageTableHeapGpuBase;
	}

	*pui64PhysAddr = PVR_ALIGN(ui64MemCursor, (IMG_UINT64) uiAlignment);

	ui64MemCursor = *pui64PhysAddr + uiBytes;

	if (ui64MemCursor > (ui64PageTableHeapGpuBase + ui64PageTableHeapSize))
	{
		*pui64PhysAddr = 0;
		return PVRSRV_ERROR_OUT_OF_MEMORY;
	}
	else
	{
		return PVRSRV_OK;
	}
}

/*************************************************************************/ /*!
@Function       _MMU_PhysMemAlloc
@Description    Allocates physical memory for MMU objects
@Input          psPhysMemCtx    Physmem context to do the allocation from
@Output         psMemDesc       Allocation description
@Input          uiBytes         Size of the allocation in bytes
@Input          uiAlignment     Alignment requirement of this allocation
@Return         PVRSRV_OK if allocation was successful
 */
/*****************************************************************************/
static PVRSRV_ERROR _MMU_PhysMemAlloc(SYS_DATA *psSysData,
                                      MMU_PHYSMEM_CONTEXT *psPhysMemCtx,
                                      MMU_MEMORY_DESC *psMemDesc,
                                      IMG_UINT32 uiBytes,
                                      IMG_UINT32 uiAlignment)
{
	PVRSRV_ERROR eError;
	IMG_UINT64 ui64PhysAddr;
	IMG_UINT64 ui64CpuGpuPAOffset = (psSysData->ui64FwPageTableHeapCpuBase > psSysData->ui64FwPageTableHeapGpuBase) ?
	                                (psSysData->ui64FwPageTableHeapCpuBase - psSysData->ui64FwPageTableHeapGpuBase) :
	                                (psSysData->ui64FwPageTableHeapGpuBase - psSysData->ui64FwPageTableHeapCpuBase);

	PVR_ASSERT(psConfig->uiBytesPerEntry != 0);
	eError = _CarveoutAllocator(psSysData->ui64FwPageTableHeapGpuBase,
	                            psSysData->ui64FwPageTableHeapSize,
	                            uiBytes, uiAlignment, &ui64PhysAddr);

	if (eError != PVRSRV_OK)
	{
		return eError;
	}

	psMemDesc->pvCpuVAddr = (void __iomem *)ioremap(ui64PhysAddr +
	                                        ui64CpuGpuPAOffset, uiBytes);
	psMemDesc->sDevPAddr.uiAddr = (IMG_UINT64) ui64PhysAddr;
	psMemDesc->uiSize = uiBytes;

	return PVRSRV_OK;
}

/*************************************************************************/ /*!
@Function       _PxMemAlloc
@Description    Allocates physical memory for MMU objects
@Input          psSysData       System's heap details
@Input          psMMUContext    MMU context
@Input          uiNumEntries    Number of entries to allocate
@Input          psConfig        MMU Px config
@Input          eMMULevel       MMU level that allocation is for
@Output         psMemDesc       Description of allocation
@Return         PVRSRV_OK if allocation was successful
 */
/*****************************************************************************/
static PVRSRV_ERROR _PxMemAlloc(SYS_DATA *psSysData,
                                MMU_CONTEXT *psMMUContext,
                                IMG_UINT32 uiNumEntries,
                                const MMU_PxE_CONFIG *psConfig,
                                MMU_LEVEL eMMULevel,
                                MMU_MEMORY_DESC *psMemDesc,
                                IMG_UINT32 uiLog2Align)
{
	IMG_UINT32 uiBytes;
	IMG_UINT32 uiAlign;
	PVRSRV_ERROR eError;

	PVR_ASSERT(psConfig->uiBytesPerEntry != 0);

	uiBytes = uiNumEntries * psConfig->uiBytesPerEntry;
	/* We need here the alignment of the previous level because that is the entry for we generate here */
	uiAlign = 1 << uiLog2Align;

	/*
	 * If the hardware specifies an alignment requirement for a page table then
	 * it also requires that all memory up to the next aligned address is
	 * zeroed.
	 *
	 * Failing to do this can result in uninitialised data outside of the actual
	 * page table range being read by the MMU and treated as valid, e.g. the
	 * pending flag.
	 *
	 * Typically this will affect 1MiB, 2MiB PT pages which have a size of 16
	 * and 8 bytes respectively but an alignment requirement of 64 bytes each.
	 */
	uiBytes = PVR_ALIGN(uiBytes, uiAlign);

	/* allocate the object */
	eError = _MMU_PhysMemAlloc(psSysData,
	                           psMMUContext->psPhysMemCtx,
	                           psMemDesc,
	                           uiBytes,
	                           uiAlign);
	if (eError != PVRSRV_OK)
	{
		goto e0;
	}

	return PVRSRV_OK;
e0:
	return eError;
}

/*************************************************************************/ /*!
@Function       _SetupPxE
@Description    Sets up an entry of an MMU object to point to the
                provided address
@Input          psMMUContext    MMU context to operate on
@Input          uiIndex         Index into the MMU object to setup
@Input          psConfig        MMU Px config
@Input          eMMULevel       Level of MMU object
@Input          psDevPAddr      Address to setup the MMU object to point to
@Return         PVRSRV_OK if the setup was successful
 */
/*****************************************************************************/
static PVRSRV_ERROR _SetupPxE(MMU_LEVEL_INFO *psLevel,
                              IMG_UINT32 uiIndex,
                              const MMU_PxE_CONFIG *psConfig,
                              MMU_LEVEL eMMULevel,
                              const IMG_DEV_PHYADDR *psDevPAddr)
{
	MMU_MEMORY_DESC *psMemDesc = &psLevel->sMemDesc;

	IMG_UINT32 (*pfnDerivePxEProt4)(IMG_UINT32);
	IMG_UINT64 (*pfnDerivePxEProt8)(IMG_UINT32);

	switch (eMMULevel)
	{
		case MMU_LEVEL_3:
			pfnDerivePxEProt4 = RGXMMU4DerivePCEProt4;
			pfnDerivePxEProt8 = RGXMMU4DerivePCEProt8;
			break;

		case MMU_LEVEL_2:
			pfnDerivePxEProt4 = RGXMMU4DerivePDEProt4;
			pfnDerivePxEProt8 = RGXMMU4DerivePDEProt8;
			break;

		case MMU_LEVEL_1:
			pfnDerivePxEProt4 = RGXMMU4DerivePTEProt4;
			pfnDerivePxEProt8 = RGXMMU4DerivePTEProt8;
			break;

		default:
			RGXErrorLog(NULL, "%s: invalid MMU level", __func__);
			return PVRSRV_ERROR_INVALID_PARAMS;
	}

	/* How big is a PxE in bytes? */
	/* Filling the actual Px entry with an address */
	switch (psConfig->uiBytesPerEntry)
	{
		case 4:
		{
			IMG_UINT32 *pui32Px;
			IMG_UINT64 ui64PxE64;

			pui32Px = psMemDesc->pvCpuVAddr; /* Give the virtual base address of Px */

			ui64PxE64 = psDevPAddr->uiAddr       /* Calculate the offset to that base */
					>> psConfig->uiAddrLog2Align /* Shift away the unnecessary bits of the address */
					<< psConfig->uiAddrShift     /* Shift back to fit address in the Px entry */
					& psConfig->uiAddrMask;      /* Delete unused higher bits */

			ui64PxE64 |= (IMG_UINT64)pfnDerivePxEProt4(PROT_FLAG);
			/* assert that the result fits into 32 bits before writing
			   it into the 32-bit array with a cast */
			PVR_ASSERT(ui64PxE64 == (ui64PxE64 & 0xffffffffU));

			/* We should never invalidate an invalid page */
			if (PROT_FLAG & MMU_PROTFLAGS_INVALID)
			{
				PVR_ASSERT(pui32Px[uiIndex] != ui64PxE64);
			}
			pui32Px[uiIndex] = (IMG_UINT32) ui64PxE64;
			break;
		}
		case 8:
		{
			IMG_UINT64 *pui64Px = psMemDesc->pvCpuVAddr; /* Give the virtual base address of Px */

			pui64Px[uiIndex] = psDevPAddr->uiAddr /* Calculate the offset to that base */
					>> psConfig->uiAddrLog2Align  /* Shift away the unnecessary bits of the address */
					<< psConfig->uiAddrShift      /* Shift back to fit address in the Px entry */
					& psConfig->uiAddrMask;       /* Delete unused higher bits */
			pui64Px[uiIndex] |= pfnDerivePxEProt8(PROT_FLAG);
			break;
		}
		default:
			RGXErrorLog(NULL, "%s: PxE size not supported (%d) for level %d",
					__func__, psConfig->uiBytesPerEntry, eMMULevel);

			return PVRSRV_ERROR_MMU_CONFIG_IS_WRONG;
	}

	return PVRSRV_OK;
}

/*************************************************************************/ /*!
@Function       _MMU_GetLevelData
@Description    Get all the level data and calculate the indices for the
                specified address range
@Input          sDevVAddrStart          Start device virtual address
@Input          sDevVAddrEnd            End device virtual address
@Input          auiStartArray           Array of start indices (one for each level)
@Input          auiEndArray             Array of end indices (one for each level)
@Input          auiEntriesPerPx         Array of number of entries for the Px
                                        (one for each level)
@Input          aeMMULevel              Array of MMU levels (one for each level)
@Input          ui32Log2PageSize        Page size of the data pages
 */
/*****************************************************************************/
static void _MMU_GetLevelData(IMG_DEV_VIRTADDR sDevVAddrStart,
                              IMG_DEV_VIRTADDR sDevVAddrEnd,
                              IMG_UINT32 auiStartArray[],
                              IMG_UINT32 auiEndArray[],
                              IMG_UINT32 auiEntriesPerPx[],
                              MMU_LEVEL aeMMULevel[],
                              IMG_UINT32 ui32Log2PageSize)
{
	IMG_UINT32 i = 0;
	IMG_UINT64 ret;

	/* PC */
	{
		auiStartArray[i] = ((sDevVAddrStart.uiAddr & PC_INDEX_MASK) >> PC_INDEX_SHIFT);
		/* round up */
		sDevVAddrEnd.uiAddr--;
		ret = ((sDevVAddrEnd.uiAddr & PC_INDEX_MASK) >> PC_INDEX_SHIFT);
		auiEndArray[i] = ++ret;
		sDevVAddrEnd.uiAddr++;
		auiEntriesPerPx[i] = (IMG_UINT32) ((PC_INDEX_MASK >> PC_INDEX_SHIFT) + 1);
		aeMMULevel[i] = MMU_LEVEL_3;
		i++;
	}

	/* PD */
	{
		auiStartArray[i] = ((sDevVAddrStart.uiAddr & PD_INDEX_MASK) >> PD_INDEX_SHIFT);
		/* round up */
		sDevVAddrEnd.uiAddr--;
		ret = ((sDevVAddrEnd.uiAddr & PD_INDEX_MASK) >> PD_INDEX_SHIFT);
		auiEndArray[i] = ++ret;
		sDevVAddrEnd.uiAddr++;
		auiEntriesPerPx[i] = (IMG_UINT32) ((PD_INDEX_MASK >> PD_INDEX_SHIFT) + 1);
		aeMMULevel[i] = MMU_LEVEL_2;
		i++;
	}

	/* PT */
	{
		IMG_UINT32 ui32PtIndexMask = ((PT_INDEX_MASK >> ui32Log2PageSize) << ui32Log2PageSize);
		auiStartArray[i] = ((sDevVAddrStart.uiAddr & ui32PtIndexMask) >> ui32Log2PageSize);
		/* round up */
		sDevVAddrEnd.uiAddr--;
		ret = ((sDevVAddrEnd.uiAddr & ui32PtIndexMask) >> ui32Log2PageSize);
		auiEndArray[i] = ++ret;
		sDevVAddrEnd.uiAddr++;
		auiEntriesPerPx[i] = (IMG_UINT32) ((ui32PtIndexMask >> ui32Log2PageSize) + 1);
		aeMMULevel[i] = MMU_LEVEL_1;
	}
}

/*************************************************************************/ /*!
@Function       _MMU_AllocLevel
@Description    Recursively allocates the specified range of Px entries.
                The values for auiStartArray should be by used for
                the first call into each level and the values in auiEndArray
                should only be used in the last call for each level.
                In order to determine if this is the first/last call we pass
                in bFirst and bLast.
                When one level calls down to the next only if bFirst/bLast is set
                and it's the first/last iteration of the loop at its level will
                bFirst/bLast set for the next recursion.
                This means that each iteration has the knowledge of the previous
                level which is required.
@Input          psSysData               System's heap details
@Input          psMMUContext            MMU context to operate on
@Input          psLevel                 Level info on which to free the
                                        specified range
@Input          auiStartArray           Array of start indices (one for each level)
@Input          auiEndArray             Array of end indices (one for each level)
@Input          auiEntriesPerPxArray    Array of number of entries for the Px
                                        (one for each level)
@Input          aeMMULevel              Array of MMU levels (one for each level)
@Input          pui32CurrentLevel       Pointer to a variable which is set to our
                                        current level
@Input          uiStartIndex            Start index of the range to free
@Input          uiEndIndex              End index of the range to free
@Input          bFirst                  This is the first call for this level
@Input          bLast                   This is the last call for this level
@Input          ui32Log2PageSize        Page size of the data pages
@Return         PVRSRV_OK if the setup was successful
 */
/*****************************************************************************/
static PVRSRV_ERROR _MMU_AllocLevel(SYS_DATA *psSysData,
                                    MMU_CONTEXT *psMMUContext,
                                    MMU_LEVEL_INFO *psLevel,
                                    IMG_UINT32 auiStartArray[],
                                    IMG_UINT32 auiEndArray[],
                                    IMG_UINT32 auiEntriesPerPxArray[],
                                    MMU_LEVEL aeMMULevel[],
                                    IMG_UINT32 *pui32CurrentLevel,
                                    IMG_UINT32 uiStartIndex,
                                    IMG_UINT32 uiEndIndex,
                                    bool bFirst,
                                    bool bLast,
                                    IMG_UINT32 ui32Log2PageSize)
{
	IMG_UINT32 uiThisLevel = *pui32CurrentLevel; /* Starting with 0 */
	MMU_PxE_CONFIG psConfig; /* The table config for the current level */
	PVRSRV_ERROR eError;
	IMG_UINT32 i;

	/* Parameter check */
	PVR_ASSERT(*pui32CurrentLevel < MMU_MAX_LEVEL);

	/* P(C/D/T) Entry Config */
	if (aeMMULevel[uiThisLevel] == MMU_LEVEL_3)
	{
		psConfig = sRGXMMUPCEConfig;
	}
	else if (aeMMULevel[uiThisLevel] == MMU_LEVEL_2)
	{
		psConfig = sRGXMMUPDEConfig;
	}
	else
	{
		psConfig = sRGXMMUPTEConfig;
	}

	/* Go from uiStartIndex to uiEndIndex through the Px */
	for (i = uiStartIndex;i < uiEndIndex;i++)
	{
		/* Only try an allocation if this is not the last level */
		/*Because a PT allocation is already done while setting the entry in PD */
		if (aeMMULevel[uiThisLevel] != MMU_LEVEL_1)
		{
			IMG_UINT32 uiNextStartIndex;
			IMG_UINT32 uiNextEndIndex;
			bool bNextFirst;
			bool bNextLast;

			/* If there is already a next Px level existing, do not allocate it */
			if (!psLevel->apsNextLevel[i])
			{
				MMU_LEVEL_INFO *psNextLevel;
				IMG_UINT32 ui32AllocSize;
				IMG_UINT32 uiNextEntries;

				/* Allocate and setup the next level */
				uiNextEntries = auiEntriesPerPxArray[uiThisLevel + 1];
				ui32AllocSize = sizeof(MMU_LEVEL_INFO);
				if (aeMMULevel[uiThisLevel + 1] != MMU_LEVEL_1)
				{
					ui32AllocSize += sizeof(MMU_LEVEL_INFO *) * (uiNextEntries - 1);
				}

				psNextLevel = kzalloc(ui32AllocSize, GFP_KERNEL);
				if (psNextLevel == NULL)
				{
					goto e0;
				}

				/* Hook in this level for next time */
				psLevel->apsNextLevel[i] = psNextLevel;

				/* Allocate Px memory for a sub level*/
				eError = _PxMemAlloc(psSysData,
				                     psMMUContext, uiNextEntries, &psConfig,
				                     aeMMULevel[uiThisLevel + 1],
				                     &psNextLevel->sMemDesc,
				                     psConfig.uiAddrLog2Align);
				if (eError != PVRSRV_OK)
				{
					goto e0;
				}

				/* Wire up the entry */
				eError = _SetupPxE(psLevel,
				                   i,
				                   &psConfig,
				                   aeMMULevel[uiThisLevel],
				                   &psNextLevel->sMemDesc.sDevPAddr);

				if (eError != PVRSRV_OK)
				{
					goto e0;
				}
			}

			/* If we're crossing a Px then the start index changes */
			if (bFirst && (i == uiStartIndex))
			{
				uiNextStartIndex = auiStartArray[uiThisLevel + 1];
				bNextFirst = true;
			}
			else
			{
				uiNextStartIndex = 0;
				bNextFirst = false;
			}

			/* If we're crossing a Px then the end index changes */
			if (bLast && (i == (uiEndIndex - 1)))
			{
				uiNextEndIndex = auiEndArray[uiThisLevel + 1];
				bNextLast = true;
			}
			else
			{
				uiNextEndIndex = auiEntriesPerPxArray[uiThisLevel + 1];
				bNextLast = false;
			}

			/* Recurse into the next level */
			(*pui32CurrentLevel)++;
			eError = _MMU_AllocLevel(psSysData,
			                         psMMUContext,
			                         psLevel->apsNextLevel[i],
			                         auiStartArray,
			                         auiEndArray,
			                         auiEntriesPerPxArray,
			                         aeMMULevel,
			                         pui32CurrentLevel,
			                         uiNextStartIndex,
			                         uiNextEndIndex,
			                         bNextFirst,
			                         bNextLast,
			                         ui32Log2PageSize);
			(*pui32CurrentLevel)--;
			if (eError != PVRSRV_OK)
			{
				goto e0;
			}
		}
	}
	return PVRSRV_OK;

e0:
	return eError;
}

static IMG_UINT32 _CalcPCEIdx(IMG_DEV_VIRTADDR sDevVAddr,
                              const MMU_DEVVADDR_CONFIG *psDevVAddrConfig,
                              IMG_BOOL bRoundUp)
{
	IMG_DEV_VIRTADDR sTmpDevVAddr;
	IMG_UINT32 ui32RetVal;

	sTmpDevVAddr = sDevVAddr;

	if (bRoundUp)
	{
		sTmpDevVAddr.uiAddr--;
	}
	ui32RetVal = (IMG_UINT32) ((sTmpDevVAddr.uiAddr & psDevVAddrConfig->uiPCIndexMask)
			>> psDevVAddrConfig->uiPCIndexShift);

	if (bRoundUp)
	{
		ui32RetVal++;
	}

	return ui32RetVal;
}

static IMG_UINT32 _CalcPDEIdx(IMG_DEV_VIRTADDR sDevVAddr,
                              const MMU_DEVVADDR_CONFIG *psDevVAddrConfig,
                              IMG_BOOL bRoundUp)
{
	IMG_DEV_VIRTADDR sTmpDevVAddr;
	IMG_UINT32 ui32RetVal;

	sTmpDevVAddr = sDevVAddr;

	if (bRoundUp)
	{
		sTmpDevVAddr.uiAddr--;
	}
	ui32RetVal = (IMG_UINT32) ((sTmpDevVAddr.uiAddr & psDevVAddrConfig->uiPDIndexMask)
			>> psDevVAddrConfig->uiPDIndexShift);

	if (bRoundUp)
	{
		ui32RetVal++;
	}

	return ui32RetVal;
}

static IMG_UINT32 _CalcPTEIdx(IMG_DEV_VIRTADDR sDevVAddr,
                              const MMU_DEVVADDR_CONFIG *psDevVAddrConfig,
                              IMG_BOOL bRoundUp)
{
	IMG_DEV_VIRTADDR sTmpDevVAddr;
	IMG_UINT32 ui32RetVal;

	sTmpDevVAddr = sDevVAddr;
	sTmpDevVAddr.uiAddr -= psDevVAddrConfig->uiOffsetInBytes;
	if (bRoundUp)
	{
		sTmpDevVAddr.uiAddr--;
	}
	ui32RetVal = (IMG_UINT32) ((sTmpDevVAddr.uiAddr & psDevVAddrConfig->uiPTIndexMask)
			>> psDevVAddrConfig->uiPTIndexShift);

	if (bRoundUp)
	{
		ui32RetVal++;
	}

	return ui32RetVal;
}
static INLINE void _MMU_GetPTInfo(MMU_CONTEXT                *psMMUContext,
                                  IMG_DEV_VIRTADDR            sDevVAddr,
                                  const MMU_DEVVADDR_CONFIG  *psDevVAddrConfig,
                                  MMU_LEVEL_INFO           **ppsLevel,
                                  IMG_UINT32                 *pui32PTEIndex)
{
	MMU_LEVEL_INFO *psLocalLevel = NULL;
	MMU_LEVEL eMMULevel = sRGXMMUPCEConfig.ePxLevel;
	IMG_UINT32 uiPCEIndex;
	IMG_UINT32 uiPDEIndex;

	if ((eMMULevel <= MMU_LEVEL_0) || (eMMULevel >= MMU_LEVEL_LAST))
	{
		RGXErrorLog(NULL, "%s: Invalid MMU level", __func__);
	}

	for (; eMMULevel > MMU_LEVEL_0; eMMULevel--)
	{
		if (eMMULevel == MMU_LEVEL_3)
		{
			/* find the page directory containing the PCE */
			uiPCEIndex = _CalcPCEIdx(sDevVAddr, psDevVAddrConfig,
			                         false);
			psLocalLevel = psMMUContext->sBaseLevelInfo.apsNextLevel[uiPCEIndex];
		}

		if (eMMULevel == MMU_LEVEL_2)
		{
			/* find the page table containing the PDE */
			uiPDEIndex = _CalcPDEIdx(sDevVAddr, psDevVAddrConfig,
			                         false);
			if (psLocalLevel != NULL)
			{
				psLocalLevel = psLocalLevel->apsNextLevel[uiPDEIndex];
			}
			else
			{
				psLocalLevel =
						psMMUContext->sBaseLevelInfo.apsNextLevel[uiPDEIndex];
			}
		}

		if (eMMULevel == MMU_LEVEL_1)
		{
			/* find PTE index into page table */
			*pui32PTEIndex = _CalcPTEIdx(sDevVAddr, psDevVAddrConfig,
			                             false);
			if (psLocalLevel == NULL)
			{
				psLocalLevel = &psMMUContext->sBaseLevelInfo;
			}
		}
	}
	*ppsLevel = psLocalLevel;
}

static PVRSRV_ERROR MMU_MapRange(IMG_UINT64 ui64BasePA,
                                 IMG_DEV_VIRTADDR sDevVAddrBase,
                                 IMG_DEVMEM_SIZE_T uiSizeBytes,
                                 MMU_CONTEXT *psMMUContext,
                                 IMG_UINT32 uiLog2HeapPageSize)
{
	const MMU_PxE_CONFIG *psConfig = &sRGXMMUPTEConfig;

	IMG_UINT32 i, uiChunkStart, uiLastPTEIndex, uiNumEntriesToWrite;
	IMG_UINT32 ui32PagesDone=0, uiPTEIndex=0;

	IMG_UINT8 uiAddrLog2Align, uiAddrShift;
	IMG_UINT64 uiAddrMask, uiProtFlags;
	IMG_UINT32 uiBytesPerEntry;

	IMG_UINT64* pui64LevelBase;
	IMG_UINT32* pui32LevelBase;
	MMU_LEVEL_INFO *psLevel = NULL;

	IMG_UINT32 uiNumPages = uiSizeBytes >> uiLog2HeapPageSize;
	IMG_UINT32 uiPageSize = (1 << uiLog2HeapPageSize);

	PVR_ASSERT (psMMUContext != NULL);
	PVR_ASSERT((IMG_DEVMEM_OFFSET_T)uiNumPages << uiLog2HeapPageSize == uiSizeBytes);

	uiAddrLog2Align = psConfig->uiAddrLog2Align;
	uiAddrShift = psConfig->uiAddrShift;
	uiAddrMask = psConfig->uiAddrMask;
	uiBytesPerEntry = psConfig->uiBytesPerEntry;
	uiProtFlags = RGX_MMUCTRL_PT_DATA_PM_META_PROTECT_EN |
	              ~RGX_MMUCTRL_PT_DATA_AXCACHE_CLRMSK |
	              RGX_MMUCTRL_PT_DATA_VALID_EN;

	do
	{
		_MMU_GetPTInfo(psMMUContext, sDevVAddrBase, &sRGXMMUDevVAddrConfig,
		               &psLevel, &uiPTEIndex);

		pui64LevelBase = (IMG_UINT64*)psLevel->sMemDesc.pvCpuVAddr;
		pui32LevelBase = (IMG_UINT32*)psLevel->sMemDesc.pvCpuVAddr;

		uiLastPTEIndex = MIN(uiPTEIndex + uiNumPages - ui32PagesDone, sRGXMMUDevVAddrConfig.uiNumEntriesPT);
		uiNumEntriesToWrite = uiLastPTEIndex - uiPTEIndex;

		for (uiChunkStart = 0; uiChunkStart < uiNumEntriesToWrite; uiChunkStart += 32)
		{
			IMG_UINT32 uiNumPagesInBlock = MIN(uiNumEntriesToWrite - uiChunkStart, 32);

			for (i=0; i<uiNumPagesInBlock; i++)
			{
				IMG_UINT32 ui32Index = uiPTEIndex + uiChunkStart + i;
				IMG_UINT64 ui64Page = ui64BasePA + uiPageSize * ui32Index;
				IMG_UINT64 uiEntryValue = (((ui64Page >> uiAddrLog2Align) << uiAddrShift) & uiAddrMask) | uiProtFlags;

				if (uiBytesPerEntry == 8)
				{
					pui64LevelBase[ui32Index] = uiEntryValue;
				}
				else if (uiBytesPerEntry == 4)
				{
					pui32LevelBase[ui32Index] = (IMG_UINT32) uiEntryValue;
				}
			}
		}

		sDevVAddrBase.uiAddr += uiNumEntriesToWrite * uiPageSize;
		ui64BasePA += uiNumEntriesToWrite * uiPageSize;
		ui32PagesDone += uiNumEntriesToWrite;

	} while (ui32PagesDone < uiNumPages);

	return PVRSRV_OK;
}

PVRSRV_ERROR PVRSRVConfigureMMU(SYS_DATA *psSysData)
{
	PVRSRV_ERROR eError = PVRSRV_OK;
	IMG_UINT32 auiStartArray[MMU_MAX_LEVEL];
	IMG_UINT32 auiEndArray[MMU_MAX_LEVEL];
	IMG_UINT32 auiEntriesPerPx[MMU_MAX_LEVEL];
	MMU_LEVEL aeMMULevel[MMU_MAX_LEVEL];
	IMG_UINT32 ui32CurrentLevel = 0;
	IMG_UINT32 ui32Log2PageSize = PAGE_SHIFT;
	IMG_DEV_VIRTADDR sDevVAddrStart;
	IMG_DEV_VIRTADDR sDevVAddrEnd;
	IMG_UINT32 ui32DriverID;
	MMU_CONTEXT *psMMUContext;
	const IMG_UINT32 ui32ContextSize = sizeof(MMU_CONTEXT) +
			(sRGXMMUDevVAddrConfig.uiNumEntriesPC - 1) * sizeof(MMU_LEVEL_INFO*);

	sDevVAddrStart.uiAddr = FWHEAP_GPU_VA;
	sDevVAddrEnd.uiAddr  = FWHEAP_GPU_VA + RGX_NUM_DRIVERS_SUPPORTED*psSysData->ui64FwTotalHeapSize;

	psMMUContext = kzalloc(ui32ContextSize, GFP_KERNEL);
	if (psMMUContext == NULL)
	{
		eError = PVRSRV_ERROR_OUT_OF_MEMORY;
		goto err;
	}

	_MMU_GetLevelData(sDevVAddrStart, sDevVAddrEnd,
	                  auiStartArray, auiEndArray,
	                  auiEntriesPerPx, aeMMULevel, ui32Log2PageSize);

	/* Allocate the top MMU level */
	eError = _PxMemAlloc(psSysData,
	                     psMMUContext,
	                     sRGXMMUDevVAddrConfig.uiNumEntriesPC,
	                     &sRGXMMUPCEConfig,
	                     sRGXMMUPCEConfig.ePxLevel,
	                     &psMMUContext->sBaseLevelInfo.sMemDesc,
	                     ui32Log2PageSize);
	if (eError != PVRSRV_OK)
	{
		goto err;
	}

	eError = _MMU_AllocLevel(psSysData, psMMUContext,
	                   &psMMUContext->sBaseLevelInfo,
	                   auiStartArray, auiEndArray, auiEntriesPerPx,
	                   aeMMULevel, &ui32CurrentLevel,
	                   auiStartArray[0], auiEndArray[0],
	                   true, true, ui32Log2PageSize);
	if (eError != PVRSRV_OK)
	{
		goto err;
	}

	/* Populate page tables for the Firmware heaps of all supported Guests */
	for (ui32DriverID=0; ui32DriverID < RGX_NUM_DRIVERS_SUPPORTED; ui32DriverID++)
	{
		IMG_UINT64 ui64BasePA;

		sDevVAddrStart.uiAddr = FWHEAP_GPU_VA + ui32DriverID*psSysData->ui64FwTotalHeapSize;
		ui64BasePA = psSysData->ui64FwHeapGpuBase +
		             ui32DriverID*psSysData->ui64FwHeapStride +
		             sDevVAddrStart.uiAddr - FWHEAP_GPU_VA -
		             ui32DriverID*psSysData->ui64FwTotalHeapSize;

		MMU_MapRange(ui64BasePA, sDevVAddrStart, psSysData->ui64FwTotalHeapSize,
		             psMMUContext, ui32Log2PageSize);
	}

#if defined(FW_CUSTOM_REGION0_GPU_VA)
	sDevVAddrStart.uiAddr = FW_CUSTOM_REGION0_GPU_VA;
	sDevVAddrEnd.uiAddr  = FW_CUSTOM_REGION0_GPU_VA + FW_CUSTOM_REGION0_SIZE;

	_MMU_GetLevelData(sDevVAddrStart, sDevVAddrEnd,
	                  auiStartArray, auiEndArray,
	                  auiEntriesPerPx, aeMMULevel, ui32Log2PageSize);

	eError = _MMU_AllocLevel(psSysData, psMMUContext,
	                   &psMMUContext->sBaseLevelInfo,
	                   auiStartArray, auiEndArray, auiEntriesPerPx,
	                   aeMMULevel, &ui32CurrentLevel,
	                   auiStartArray[0], auiEndArray[0],
	                   true, true, ui32Log2PageSize);
	if (eError != PVRSRV_OK)
	{
		goto err;
	}

	MMU_MapRange(FW_CUSTOM_REGION0_GPU_PA, sDevVAddrStart,
	             FW_CUSTOM_REGION0_SIZE, psMMUContext, ui32Log2PageSize);
#endif

#if defined(FW_CUSTOM_REGION1_GPU_VA)
	sDevVAddrStart.uiAddr = FW_CUSTOM_REGION1_GPU_VA;
	sDevVAddrEnd.uiAddr  = FW_CUSTOM_REGION1_GPU_VA + FW_CUSTOM_REGION1_SIZE;

	_MMU_GetLevelData(sDevVAddrStart, sDevVAddrEnd,
	                  auiStartArray, auiEndArray,
	                  auiEntriesPerPx, aeMMULevel, ui32Log2PageSize);

	eError = _MMU_AllocLevel(psSysData, psMMUContext,
	                   &psMMUContext->sBaseLevelInfo,
	                   auiStartArray, auiEndArray, auiEntriesPerPx,
	                   aeMMULevel, &ui32CurrentLevel,
	                   auiStartArray[0], auiEndArray[0],
	                   true, true, ui32Log2PageSize);
	if (eError != PVRSRV_OK)
	{
		goto err;
	}

	MMU_MapRange(FW_CUSTOM_REGION1_GPU_PA, sDevVAddrStart,
	             FW_CUSTOM_REGION1_SIZE, psMMUContext, ui32Log2PageSize);
#endif

err:
	kfree(psMMUContext);

	return PVRSRV_OK;
}
