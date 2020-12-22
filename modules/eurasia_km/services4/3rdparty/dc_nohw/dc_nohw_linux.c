/*************************************************************************/ /*!
@Title          Dummy 3rd party driver linux specific declarations.
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

/**************************************************************************
 The 3rd party driver is a specification of an API to integrate the
 IMG PowerVR Services driver with 3rd Party display hardware.
 It is NOT a specification for a display controller driver, rather a
 specification to extend the API for a pre-existing driver for the display hardware.

 The 3rd party driver interface provides IMG PowerVR client drivers (e.g. PVR2D)
 with an API abstraction of the system's underlying display hardware, allowing
 the client drivers to indirectly control the display hardware and access its
 associated memory.

 Functions of the API include

  - query primary surface attributes (width, height, stride, pixel format,
      CPU physical and virtual address)
  - swap/flip chain creation and subsequent query of surface attributes
  - asynchronous display surface flipping, taking account of asynchronous
      read (flip) and write (render) operations to the display surface

 Note: having queried surface attributes the client drivers are able to map
 the display memory to any IMG PowerVR Services device by calling
 PVRSRVMapDeviceClassMemory with the display surface handle.

 This code is intended to be an example of how a pre-existing display driver
 may be extended to support the 3rd Party Display interface to
 PowerVR Services - IMG is not providing a display driver implementation
 **************************************************************************/

#include <linux/version.h>

#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,38))
#ifndef AUTOCONF_INCLUDED
#include <linux/config.h>
#endif
#endif

#include <linux/module.h>
#include <linux/pci.h>
#include <linux/slab.h>

#if defined(SUPPORT_DRI_DRM)
#include <drm/drmP.h>
#endif

#if defined(DC_NOHW_DISCONTIG_BUFFERS)
#include <linux/vmalloc.h>
#include <asm/page.h>
#else
#include <asm/dma-mapping.h>
#endif

#include "img_defs.h"
#include "servicesext.h"
#include "kerneldisplay.h"
#include "dc_nohw.h"
#include "pvrmodule.h"

#if defined(SUPPORT_DRI_DRM)
#include "pvr_drm.h"
#endif

#if defined(DC_USE_SET_MEMORY)
	#undef	DC_USE_SET_MEMORY
#endif

#if !defined(DC_NOHW_DISCONTIG_BUFFERS)
	#if defined(__i386__) && (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,26)) && defined(SUPPORT_LINUX_X86_PAT) && defined(SUPPORT_LINUX_X86_WRITECOMBINE)
		#include <asm/cacheflush.h>
		#define	DC_USE_SET_MEMORY
	#endif
#endif	/* defined(DC_NOHW_DISCONTIG_BUFFERS) */

#define DRVNAME "dcnohw"

#if !defined(SUPPORT_DRI_DRM)
MODULE_SUPPORTED_DEVICE(DRVNAME);
#endif

#define unref__ __attribute__ ((unused))

#if defined(DC_NOHW_GET_BUFFER_DIMENSIONS)
static unsigned long width = DC_NOHW_BUFFER_WIDTH;
static unsigned long height = DC_NOHW_BUFFER_HEIGHT;
static unsigned long depth = DC_NOHW_BUFFER_BIT_DEPTH;

module_param(width, ulong, S_IRUGO);
module_param(height, ulong, S_IRUGO);
module_param(depth, ulong, S_IRUGO);

IMG_BOOL GetBufferDimensions(IMG_UINT32 *pui32Width, IMG_UINT32 *pui32Height, PVRSRV_PIXEL_FORMAT *pePixelFormat, IMG_UINT32 *pui32Stride)
{
	if (width == 0 || height == 0 || depth == 0 ||
		depth != dc_nohw_roundup_bit_depth(depth))
	{
		printk(KERN_WARNING DRVNAME ": Illegal module parameters (width %lu, height %lu, depth %lu)\n", width, height, depth);
		return IMG_FALSE;
	}

	*pui32Width = (IMG_UINT32)width;
	*pui32Height = (IMG_UINT32)height;

	switch(depth)
	{
		case 32:
			*pePixelFormat = PVRSRV_PIXEL_FORMAT_ARGB8888;
			break;
		case 16:
			*pePixelFormat = PVRSRV_PIXEL_FORMAT_RGB565;
			break;
		default:
			printk(KERN_WARNING DRVNAME ": Display depth %lu not supported\n", depth);
			*pePixelFormat = PVRSRV_PIXEL_FORMAT_UNKNOWN;
			return IMG_FALSE;
	}
			
	*pui32Stride = dc_nohw_byte_stride(width, depth);

#if defined(DEBUG)
	printk(KERN_INFO DRVNAME " Width: %lu\n", (unsigned long)*pui32Width);
	printk(KERN_INFO DRVNAME " Height: %lu\n", (unsigned long)*pui32Height);
	printk(KERN_INFO DRVNAME " Depth: %lu bits\n", depth);
	printk(KERN_INFO DRVNAME " Stride: %lu bytes\n", (unsigned long)*pui32Stride);
#endif	/* defined(DEBUG) */

	return IMG_TRUE;
}
#endif	/* defined(DC_NOHW_GET_BUFFER_DIMENSIONS) */

/*****************************************************************************
 Function Name:	DC_NOHW_Init
 Description  :	Insert the driver into the kernel.

				__init places the function in a special memory section that
				the kernel frees once the function has been run.  Refer also
				to module_init() macro call below.

*****************************************************************************/
#if defined(SUPPORT_DRI_DRM)
int PVR_DRM_MAKENAME(DISPLAY_CONTROLLER, _Init)(struct drm_device unref__ *dev)
#else
static int __init DC_NOHW_Init(void)
#endif
{
	if(Init() != DC_OK)
	{
		return -ENODEV;
	}

	return 0;
} /*DC_NOHW_Init*/

/*****************************************************************************
 Function Name:	DC_NOHW_Cleanup
 Description  :	Remove the driver from the kernel.

				__exit places the function in a special memory section that
				the kernel frees once the function has been run.  Refer also
				to module_exit() macro call below.

*****************************************************************************/
#if defined(SUPPORT_DRI_DRM)
void PVR_DRM_MAKENAME(DISPLAY_CONTROLLER, _Cleanup)(struct drm_device unref__ *dev)
#else
static void __exit DC_NOHW_Cleanup(void)
#endif
{
	if(Deinit() != DC_OK)
	{
		printk (KERN_INFO DRVNAME ": DC_NOHW_Cleanup: can't deinit device\n");
	}
} /*DC_NOHW_Cleanup*/


void *AllocKernelMem(unsigned long ulSize)
{
	return kmalloc(ulSize, GFP_KERNEL);
}

void FreeKernelMem(void *pvMem)
{
	kfree(pvMem);
}

#if defined(DC_NOHW_DISCONTIG_BUFFERS)

#define RANGE_TO_PAGES(range) (((range) + (PAGE_SIZE - 1)) >> PAGE_SHIFT)
#define	VMALLOC_TO_PAGE_PHYS(vAddr) page_to_phys(vmalloc_to_page(vAddr))

DC_ERROR AllocDiscontigMemory(unsigned long ulSize,
                              DC_HANDLE unref__ *phMemHandle,
                              IMG_CPU_VIRTADDR *pLinAddr,
                              IMG_SYS_PHYADDR **ppPhysAddr)
{
	unsigned long ulPages = RANGE_TO_PAGES(ulSize);
	IMG_SYS_PHYADDR *pPhysAddr;
	unsigned long ulPage;
	IMG_CPU_VIRTADDR LinAddr;

	LinAddr = __vmalloc(ulSize, GFP_KERNEL | __GFP_HIGHMEM, pgprot_noncached(PAGE_KERNEL));
	if (!LinAddr)
	{
		return DC_ERROR_OUT_OF_MEMORY;
	}

	pPhysAddr = kmalloc(ulPages * sizeof(IMG_SYS_PHYADDR), GFP_KERNEL);
	if (!pPhysAddr)
	{
		vfree(LinAddr);
		return DC_ERROR_OUT_OF_MEMORY;
	}

	*pLinAddr = LinAddr;

	for (ulPage = 0; ulPage < ulPages; ulPage++)
	{
		pPhysAddr[ulPage].uiAddr = VMALLOC_TO_PAGE_PHYS(LinAddr);

		LinAddr += PAGE_SIZE;
	}

	*ppPhysAddr = pPhysAddr;

	return DC_OK;
}

void FreeDiscontigMemory(unsigned long ulSize,
                         DC_HANDLE unref__ hMemHandle,
                         IMG_CPU_VIRTADDR LinAddr,
                         IMG_SYS_PHYADDR *pPhysAddr)
{
	kfree(pPhysAddr);

	vfree(LinAddr);
}
#else	/* defined(DC_NOHW_DISCONTIG_BUFFERS) */
DC_ERROR AllocContigMemory(unsigned long ulSize,
                           DC_HANDLE unref__ *phMemHandle,
                           IMG_CPU_VIRTADDR *pLinAddr,
                           IMG_CPU_PHYADDR *pPhysAddr)
{
#if defined(DC_USE_SET_MEMORY)
	void *pvLinAddr;
	unsigned long ulAlignedSize = PAGE_ALIGN(ulSize);
	int iPages = (int)(ulAlignedSize >> PAGE_SHIFT);
	int iError;

	pvLinAddr = kmalloc(ulAlignedSize, GFP_KERNEL);
	iError = set_memory_wc((unsigned long)pvLinAddr, iPages);
	if (iError != 0)
	{
		printk(KERN_ERR DRVNAME ": AllocContigMemory:  set_memory_wc failed (%d)\n", iError);

		return DC_ERROR_OUT_OF_MEMORY;
	}

	pPhysAddr->uiAddr = virt_to_phys(pvLinAddr);
	*pLinAddr = pvLinAddr;

	return DC_OK;
#else	/* DC_USE_SET_MEMORY */
	dma_addr_t dma;
	IMG_VOID *pvLinAddr;

	pvLinAddr = dma_alloc_coherent(NULL, ulSize, &dma, GFP_KERNEL);

	if (pvLinAddr == NULL)
	{
		return DC_ERROR_OUT_OF_MEMORY;
	}

	pPhysAddr->uiAddr = dma;
	*pLinAddr = pvLinAddr;

	return DC_OK;
#endif	/* DC_USE_SET_MEMORY */
}

void FreeContigMemory(unsigned long ulSize,
                      DC_HANDLE unref__ hMemHandle,
                      IMG_CPU_VIRTADDR LinAddr,
                      IMG_CPU_PHYADDR PhysAddr)
{
#if defined(DC_USE_SET_MEMORY)
	unsigned long ulAlignedSize = PAGE_ALIGN(ulSize);
	int iError;
	int iPages = (int)(ulAlignedSize >> PAGE_SHIFT);

	iError = set_memory_wb((unsigned long)LinAddr, iPages);
	if (iError != 0)
	{
		printk(KERN_ERR DRVNAME ": FreeContigMemory:  set_memory_wb failed (%d)\n", iError);
	}
	kfree(LinAddr);
#else	/* DC_USE_SET_MEMORY */
	dma_free_coherent(NULL, ulSize, LinAddr, (dma_addr_t)PhysAddr.uiAddr);
#endif	/* DC_USE_SET_MEMORY */
}
#endif	/* defined(DC_NOHW_DISCONTIG_BUFFERS) */

DC_ERROR OpenPVRServices (DC_HANDLE *phPVRServices)
{
	/* Nothing to do - we have already checked services module insertion */
	*phPVRServices = 0;
	return DC_OK;
}

DC_ERROR ClosePVRServices (DC_HANDLE unref__ hPVRServices)
{
	/* Nothing to do */
	return DC_OK;
}

DC_ERROR GetLibFuncAddr (DC_HANDLE unref__ hExtDrv, char *szFunctionName, PFN_DC_GET_PVRJTABLE *ppfnFuncTable)
{
	if(strcmp("PVRGetDisplayClassJTable", szFunctionName) != 0)
	{
		return DC_ERROR_INVALID_PARAMS;
	}

	/* Nothing to do - should be exported from pvrsrv.ko */
	*ppfnFuncTable = PVRGetDisplayClassJTable;

	return DC_OK;
}

#if !defined(SUPPORT_DRI_DRM)
/*
 These macro calls define the initialisation and removal functions of the
 driver.  Although they are prefixed `module_', they apply when compiling
 statically as well; in both cases they define the function the kernel will
 run to start/stop the driver.
*/
module_init(DC_NOHW_Init);
module_exit(DC_NOHW_Cleanup);
#endif
