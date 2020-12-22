/*************************************************************************/ /*!
@Title          OMAP common display driver components
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
 The 3rd party driver is a specification of an API to integrate the IMG POWERVR
 Services driver with 3rd Party display hardware.  It is NOT a specification for
 a display controller driver, rather a specification to extend the API for a
 pre-existing driver for the display hardware.

 The 3rd party driver interface provides IMG POWERVR client drivers (e.g. PVR2D)
 with an API abstraction of the system's underlying display hardware, allowing
 the client drivers to indirectly control the display hardware and access its
 associated memory.
 
 Functions of the API include
 - query primary surface attributes (width, height, stride, pixel format, CPU
     physical and virtual address)
 - swap/flip chain creation and subsequent query of surface attributes
 - asynchronous display surface flipping, taking account of asynchronous read
 (flip) and write (render) operations to the display surface

 Note: having queried surface attributes the client drivers are able to map the
 display memory to any IMG POWERVR Services device by calling
 PVRSRVMapDeviceClassMemory with the display surface handle.

 This code is intended to be an example of how a pre-existing display driver may
 be extended to support the 3rd Party Display interface to POWERVR Services
 - IMG is not providing a display driver implementation.
 **************************************************************************/

/*
 * OMAP Linux 3rd party display driver.
 * This is based on the Generic PVR Linux Framebuffer 3rd party display
 * driver, with OMAP specific extensions to support flipping.
 */

#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/console.h>
#include <linux/fb.h>
#include <linux/module.h>
#include <linux/string.h>
#include <linux/notifier.h>

/* IMG services headers */
#include "img_defs.h"
#include "servicesext.h"
#include "kerneldisplay.h"
#include "omaplfb.h"

#if defined(CONFIG_DSSCOMP)
#if defined(CONFIG_ION_OMAP)
extern struct ion_device *omap_ion_device;
#else /* defined(CONFIG_ION_OMAP) */
#error CONFIG_DSSCOMP support requires CONFIG_ION_OMAP
#endif /* defined(CONFIG_ION_OMAP) */
#if defined(CONFIG_DRM_OMAP_DMM_TILER)
#include <../drivers/staging/omapdrm/omap_dmm_tiler.h>
#include <../drivers/video/omap2/dsscomp/tiler-utils.h>
#elif defined(CONFIG_TI_TILER)
#include <mach/tiler.h>
#else /* defined(CONFIG_DRM_OMAP_DMM_TILER) */
#error CONFIG_DSSCOMP support requires either \
       CONFIG_DRM_OMAP_DMM_TILER or CONFIG_TI_TILER
#endif /* defined(CONFIG_DRM_OMAP_DMM_TILER) */
#include <video/dsscomp.h>
#include <plat/dsscomp.h>
#endif /* defined(CONFIG_DSSCOMP) */

#define OMAPLFB_COMMAND_COUNT		1

#define	OMAPLFB_VSYNC_SETTLE_COUNT	5

#define	OMAPLFB_MAX_NUM_DEVICES		FB_MAX
#if (OMAPLFB_MAX_NUM_DEVICES > FB_MAX)
#error "OMAPLFB_MAX_NUM_DEVICES must not be greater than FB_MAX"
#endif

static OMAPLFB_DEVINFO *gapsDevInfo[OMAPLFB_MAX_NUM_DEVICES];

/* Top level 'hook ptr' */
static PFN_DC_GET_PVRJTABLE gpfnGetPVRJTable = NULL;

#if !defined(CONFIG_DSSCOMP)
/* Round x up to a multiple of y */
static inline unsigned long RoundUpToMultiple(unsigned long x, unsigned long y)
{
	unsigned long div = x / y;
	unsigned long rem = x % y;

	return (div + ((rem == 0) ? 0 : 1)) * y;
}

/* Greatest common divisor of x and y */
static unsigned long GCD(unsigned long x, unsigned long y)
{
	while (y != 0)
	{
		unsigned long r = x % y;
		x = y;
		y = r;
	}

	return x;
}

/* Least common multiple of x and y */
static unsigned long LCM(unsigned long x, unsigned long y)
{
	unsigned long gcd = GCD(x, y);

	return (gcd == 0) ? 0 : ((x / gcd) * y);
}
#endif

unsigned OMAPLFBMaxFBDevIDPlusOne(void)
{
	return OMAPLFB_MAX_NUM_DEVICES;
}

/* Returns DevInfo pointer for a given device */
OMAPLFB_DEVINFO *OMAPLFBGetDevInfoPtr(unsigned uiFBDevID)
{
	WARN_ON(uiFBDevID >= OMAPLFBMaxFBDevIDPlusOne());

	if (uiFBDevID >= OMAPLFB_MAX_NUM_DEVICES)
	{
		return NULL;
	}

	return gapsDevInfo[uiFBDevID];
}

/* Sets the DevInfo pointer for a given device */
static inline void OMAPLFBSetDevInfoPtr(unsigned uiFBDevID, OMAPLFB_DEVINFO *psDevInfo)
{
	WARN_ON(uiFBDevID >= OMAPLFB_MAX_NUM_DEVICES);

	if (uiFBDevID < OMAPLFB_MAX_NUM_DEVICES)
	{
		gapsDevInfo[uiFBDevID] = psDevInfo;
	}
}

static inline OMAPLFB_BOOL SwapChainHasChanged(OMAPLFB_DEVINFO *psDevInfo, OMAPLFB_SWAPCHAIN *psSwapChain)
{
	return (psDevInfo->psSwapChain != psSwapChain) ||
		(psDevInfo->uiSwapChainID != psSwapChain->uiSwapChainID);
}

/* Don't wait for vertical sync */
static inline OMAPLFB_BOOL DontWaitForVSync(OMAPLFB_DEVINFO *psDevInfo)
{
	OMAPLFB_BOOL bDontWait;

	bDontWait = OMAPLFBAtomicBoolRead(&psDevInfo->sBlanked) ||
			OMAPLFBAtomicBoolRead(&psDevInfo->sFlushCommands);

#if defined(CONFIG_HAS_EARLYSUSPEND)
	bDontWait = bDontWait || OMAPLFBAtomicBoolRead(&psDevInfo->sEarlySuspendFlag);
#endif
#if defined(SUPPORT_DRI_DRM)
	bDontWait = bDontWait || OMAPLFBAtomicBoolRead(&psDevInfo->sLeaveVT);
#endif
	return bDontWait;
}

/*
 * SetDCState
 * Called from services.
 */
static IMG_VOID SetDCState(IMG_HANDLE hDevice, IMG_UINT32 ui32State)
{
	OMAPLFB_DEVINFO *psDevInfo = (OMAPLFB_DEVINFO *)hDevice;

	switch (ui32State)
	{
		case DC_STATE_FLUSH_COMMANDS:
			/* Flush out any 'real' operation waiting for another flip.
			 * In flush state we won't pass any 'real' operations along
			 * to dsscomp_gralloc_queue(); we'll just CmdComplete them
			 * immediately.
			 */
			OMAPLFBFlip(psDevInfo, &psDevInfo->sSystemBuffer);
			OMAPLFBAtomicBoolSet(&psDevInfo->sFlushCommands, OMAPLFB_TRUE);
			break;
		case DC_STATE_NO_FLUSH_COMMANDS:
			OMAPLFBAtomicBoolSet(&psDevInfo->sFlushCommands, OMAPLFB_FALSE);
			break;
		default:
			break;
	}
}

/*
 * OpenDCDevice
 * Called from services.
 */
static PVRSRV_ERROR OpenDCDevice(IMG_UINT32 uiPVRDevID,
                                 IMG_HANDLE *phDevice,
                                 PVRSRV_SYNC_DATA* psSystemBufferSyncData)
{
	OMAPLFB_DEVINFO *psDevInfo;
	OMAPLFB_ERROR eError;
	unsigned uiMaxFBDevIDPlusOne;
	unsigned i;

	if (!try_module_get(THIS_MODULE))
	{
		return PVRSRV_ERROR_UNABLE_TO_OPEN_DC_DEVICE;
	}

	uiMaxFBDevIDPlusOne = OMAPLFBMaxFBDevIDPlusOne();

	for (i = 0; i < uiMaxFBDevIDPlusOne; i++)
	{
		psDevInfo = OMAPLFBGetDevInfoPtr(i);
		if (psDevInfo != NULL && psDevInfo->uiPVRDevID == uiPVRDevID)
		{
			break;
		}
	}
	if (i == uiMaxFBDevIDPlusOne)
	{
		DEBUG_PRINTK((KERN_WARNING DRIVER_PREFIX
			": %s: PVR Device %u not found\n", __FUNCTION__, uiPVRDevID));
		eError = PVRSRV_ERROR_INVALID_DEVICE;
		goto ErrorModulePut;
	}

	/* store the system surface sync data */
	psDevInfo->sSystemBuffer.psSyncData = psSystemBufferSyncData;
	
	eError = OMAPLFBUnblankDisplay(psDevInfo);
	if (eError != OMAPLFB_OK)
	{
		DEBUG_PRINTK((KERN_WARNING DRIVER_PREFIX
			": %s: Device %u: OMAPLFBUnblankDisplay failed (%d)\n", __FUNCTION__, psDevInfo->uiFBDevID, eError));
		eError = PVRSRV_ERROR_UNBLANK_DISPLAY_FAILED;
		goto ErrorModulePut;
	}

	/* return handle to the devinfo */
	*phDevice = (IMG_HANDLE)psDevInfo;
	
	return PVRSRV_OK;

ErrorModulePut:
	module_put(THIS_MODULE);

	return eError;
}

/*
 * CloseDCDevice
 * Called from services.
 */
static PVRSRV_ERROR CloseDCDevice(IMG_HANDLE hDevice)
{
#if defined(SUPPORT_DRI_DRM)
	OMAPLFB_DEVINFO *psDevInfo = (OMAPLFB_DEVINFO *)hDevice;

	OMAPLFBAtomicBoolSet(&psDevInfo->sLeaveVT, OMAPLFB_FALSE);
	(void) OMAPLFBUnblankDisplay(psDevInfo);
#else
	UNREFERENCED_PARAMETER(hDevice);
#endif
	module_put(THIS_MODULE);

	return PVRSRV_OK;
}

/*
 * EnumDCFormats
 * Called from services.
 */
static PVRSRV_ERROR EnumDCFormats(IMG_HANDLE hDevice,
                                  IMG_UINT32 *pui32NumFormats,
                                  DISPLAY_FORMAT *psFormat)
{
	OMAPLFB_DEVINFO	*psDevInfo;
	
	if(!hDevice || !pui32NumFormats)
	{
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	psDevInfo = (OMAPLFB_DEVINFO*)hDevice;
	
	*pui32NumFormats = 1;
	
	if(psFormat)
	{
		psFormat[0] = psDevInfo->sDisplayFormat;
	}

	return PVRSRV_OK;
}

/*
 * EnumDCDims
 * Called from services.
 */
static PVRSRV_ERROR EnumDCDims(IMG_HANDLE hDevice, 
                               DISPLAY_FORMAT *psFormat,
                               IMG_UINT32 *pui32NumDims,
                               DISPLAY_DIMS *psDim)
{
	OMAPLFB_DEVINFO	*psDevInfo;

	if(!hDevice || !psFormat || !pui32NumDims)
	{
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	psDevInfo = (OMAPLFB_DEVINFO*)hDevice;

	*pui32NumDims = 1;

	/* No need to look at psFormat; there is only one */
	if(psDim)
	{
		psDim[0] = psDevInfo->sDisplayDim;
	}
	
	return PVRSRV_OK;
}


/*
 * GetDCSystemBuffer
 * Called from services.
 */
static PVRSRV_ERROR GetDCSystemBuffer(IMG_HANDLE hDevice, IMG_HANDLE *phBuffer)
{
	OMAPLFB_DEVINFO	*psDevInfo;
	
	if(!hDevice || !phBuffer)
	{
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	psDevInfo = (OMAPLFB_DEVINFO*)hDevice;

	*phBuffer = (IMG_HANDLE)&psDevInfo->sSystemBuffer;

	return PVRSRV_OK;
}


/*
 * GetDCInfo
 * Called from services.
 */
static PVRSRV_ERROR GetDCInfo(IMG_HANDLE hDevice, DISPLAY_INFO *psDCInfo)
{
	OMAPLFB_DEVINFO	*psDevInfo;
	
	if(!hDevice || !psDCInfo)
	{
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	psDevInfo = (OMAPLFB_DEVINFO*)hDevice;

	*psDCInfo = psDevInfo->sDisplayInfo;

	return PVRSRV_OK;
}

/*
 * GetDCBufferAddr
 * Called from services.
 */
static PVRSRV_ERROR GetDCBufferAddr(IMG_HANDLE        hDevice,
                                    IMG_HANDLE        hBuffer, 
                                    IMG_SYS_PHYADDR   **ppsSysAddr,
                                    IMG_UINT32        *pui32ByteSize,
                                    IMG_VOID          **ppvCpuVAddr,
                                    IMG_HANDLE        *phOSMapInfo,
                                    IMG_BOOL          *pbIsContiguous,
	                                IMG_UINT32		  *pui32TilingStride)
{
	OMAPLFB_DEVINFO	*psDevInfo;
	OMAPLFB_BUFFER *psSystemBuffer;

	UNREFERENCED_PARAMETER(pui32TilingStride);

	if(!hDevice)
	{
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	if(!hBuffer)
	{
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	if (!ppsSysAddr)
	{
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	if (!pui32ByteSize)
	{
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	psDevInfo = (OMAPLFB_DEVINFO*)hDevice;

	psSystemBuffer = (OMAPLFB_BUFFER *)hBuffer;

	*ppsSysAddr = &psSystemBuffer->sSysAddr;

	*pui32ByteSize = (IMG_UINT32)psDevInfo->sFBInfo.ulBufferSize;

	if (ppvCpuVAddr)
	{
#if defined(CONFIG_DSSCOMP)
		*ppvCpuVAddr = psDevInfo->sFBInfo.bIs2D ? NULL : psSystemBuffer->sCPUVAddr;
#else
		*ppvCpuVAddr = psSystemBuffer->sCPUVAddr;
#endif
	}

	if (phOSMapInfo)
	{
		*phOSMapInfo = (IMG_HANDLE)0;
	}

	if (pbIsContiguous)
	{
#if defined(CONFIG_DSSCOMP)
		*pbIsContiguous = !psDevInfo->sFBInfo.bIs2D;
#else
		*pbIsContiguous = IMG_TRUE;
#endif
	}

#if defined(CONFIG_DSSCOMP)
	if (psDevInfo->sFBInfo.bIs2D)
	{
		int i = (psSystemBuffer->sSysAddr.uiAddr - psDevInfo->sFBInfo.psPageList->uiAddr) >> PAGE_SHIFT;
		*ppsSysAddr = psDevInfo->sFBInfo.psPageList + psDevInfo->sFBInfo.ulHeight * i;
	}
#endif

	return PVRSRV_OK;
}

/*
 * CreateDCSwapChain
 * Called from services.
 */
static PVRSRV_ERROR CreateDCSwapChain(IMG_HANDLE hDevice,
                                      IMG_UINT32 ui32Flags,
                                      DISPLAY_SURF_ATTRIBUTES *psDstSurfAttrib,
                                      DISPLAY_SURF_ATTRIBUTES *psSrcSurfAttrib,
                                      IMG_UINT32 ui32BufferCount,
                                      PVRSRV_SYNC_DATA **ppsSyncData,
                                      IMG_UINT32 ui32OEMFlags,
                                      IMG_HANDLE *phSwapChain,
                                      IMG_UINT32 *pui32SwapChainID)
{
	OMAPLFB_SWAPCHAIN *psSwapChain;
	OMAPLFB_DEVINFO	*psDevInfo;
	PVRSRV_ERROR eError;
	IMG_UINT32 i;

	UNREFERENCED_PARAMETER(ui32OEMFlags);
	UNREFERENCED_PARAMETER(ui32Flags);

	/* Check parameters */
	if(!hDevice
	|| !psDstSurfAttrib
	|| !psSrcSurfAttrib
	|| !ppsSyncData
	|| !phSwapChain)
	{
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	psDevInfo = (OMAPLFB_DEVINFO*)hDevice;
	
	/* Do we support swap chains? */
	if (psDevInfo->sDisplayInfo.ui32MaxSwapChains == 0)
	{
		return PVRSRV_ERROR_NOT_SUPPORTED;
	}

	OMAPLFBCreateSwapChainLock(psDevInfo);

	/* The driver only supports a single swapchain */
	if(psDevInfo->psSwapChain != NULL)
	{
		eError = PVRSRV_ERROR_FLIP_CHAIN_EXISTS;
		goto ExitUnLock;
	}

	/* create a swapchain structure */
	psSwapChain = (OMAPLFB_SWAPCHAIN*)OMAPLFBAllocKernelMem(sizeof(OMAPLFB_SWAPCHAIN));
	if(!psSwapChain)
	{
		eError = PVRSRV_ERROR_OUT_OF_MEMORY;
		goto ExitUnLock;
	}

	/* If services asks for a 0-length swap chain, it's probably Android.
	 *
	 * This will use only non-display memory posting via PVRSRVSwapToDCBuffers2(),
	 * and we can skip some useless sanity checking.
	 */
	if(ui32BufferCount > 0)
	{
		IMG_UINT32 ui32BuffersToSkip;

		/* Check the buffer count */
		if(ui32BufferCount > psDevInfo->sDisplayInfo.ui32MaxSwapChainBuffers)
		{
			eError = PVRSRV_ERROR_TOOMANYBUFFERS;
			goto ErrorFreeSwapChain;
		}
	
		if ((psDevInfo->sFBInfo.ulRoundedBufferSize * (unsigned long)ui32BufferCount) > psDevInfo->sFBInfo.ulFBSize)
		{
			eError = PVRSRV_ERROR_TOOMANYBUFFERS;
			goto ErrorFreeSwapChain;
		}

		/*
		 * We will allocate the swap chain buffers at the back of the frame
		 * buffer area.  This preserves the front portion, which may be being
		 * used by other Linux Framebuffer based applications.
		 */
		ui32BuffersToSkip = psDevInfo->sDisplayInfo.ui32MaxSwapChainBuffers - ui32BufferCount;

		/* 
		 *	Verify the DST/SRC attributes,
		 *	SRC/DST must match the current display mode config
		 */
		if(psDstSurfAttrib->pixelformat != psDevInfo->sDisplayFormat.pixelformat
		|| psDstSurfAttrib->sDims.ui32ByteStride != psDevInfo->sDisplayDim.ui32ByteStride
		|| psDstSurfAttrib->sDims.ui32Width != psDevInfo->sDisplayDim.ui32Width
		|| psDstSurfAttrib->sDims.ui32Height != psDevInfo->sDisplayDim.ui32Height)
		{
			/* DST doesn't match the current mode */
			eError = PVRSRV_ERROR_INVALID_PARAMS;
			goto ErrorFreeSwapChain;
		}

		if(psDstSurfAttrib->pixelformat != psSrcSurfAttrib->pixelformat
		|| psDstSurfAttrib->sDims.ui32ByteStride != psSrcSurfAttrib->sDims.ui32ByteStride
		|| psDstSurfAttrib->sDims.ui32Width != psSrcSurfAttrib->sDims.ui32Width
		|| psDstSurfAttrib->sDims.ui32Height != psSrcSurfAttrib->sDims.ui32Height)
		{
			/* DST doesn't match the SRC */
			eError = PVRSRV_ERROR_INVALID_PARAMS;
			goto ErrorFreeSwapChain;
		}

		psSwapChain->psBuffer = (OMAPLFB_BUFFER*)OMAPLFBAllocKernelMem(sizeof(OMAPLFB_BUFFER) * ui32BufferCount);
		if(!psSwapChain->psBuffer)
		{
			eError = PVRSRV_ERROR_OUT_OF_MEMORY;
			goto ErrorFreeSwapChain;
		}

		/* Link the buffers */
		for(i = 0; i < ui32BufferCount - 1; i++)
		{
			psSwapChain->psBuffer[i].psNext = &psSwapChain->psBuffer[i + 1];
		}

		/* and link last to first */
		psSwapChain->psBuffer[i].psNext = &psSwapChain->psBuffer[0];

		/* Configure the swapchain buffers */
		for(i = 0; i < ui32BufferCount; i++)
		{
			IMG_UINT32 ui32SwapBuffer = i + ui32BuffersToSkip;
			IMG_UINT32 ui32BufferOffset = ui32SwapBuffer * (IMG_UINT32)psDevInfo->sFBInfo.ulRoundedBufferSize;

#if defined(CONFIG_DSSCOMP)
			if (psDevInfo->sFBInfo.bIs2D)
			{
				ui32BufferOffset = 0;
			}
#endif /* defined(CONFIG_DSSCOMP) */

			psSwapChain->psBuffer[i].psSyncData = ppsSyncData[i];

			psSwapChain->psBuffer[i].sSysAddr.uiAddr = psDevInfo->sFBInfo.sSysAddr.uiAddr + ui32BufferOffset;
			psSwapChain->psBuffer[i].sCPUVAddr = psDevInfo->sFBInfo.sCPUVAddr + ui32BufferOffset;
			psSwapChain->psBuffer[i].ulYOffset = ui32BufferOffset / psDevInfo->sFBInfo.ulByteStride;
			psSwapChain->psBuffer[i].psDevInfo = psDevInfo;

#if defined(CONFIG_DSSCOMP)
			if (psDevInfo->sFBInfo.bIs2D)
			{
				psSwapChain->psBuffer[i].sSysAddr.uiAddr += ui32SwapBuffer *
					ALIGN((IMG_UINT32)psDevInfo->sFBInfo.ulWidth * psDevInfo->sFBInfo.uiBytesPerPixel, PAGE_SIZE);
			}
#endif /* defined(CONFIG_DSSCOMP) */

			OMAPLFBInitBufferForSwap(&psSwapChain->psBuffer[i]);
		}
	}
	else
	{
		psSwapChain->psBuffer = NULL;
	}

#if defined(PVR_OMAPFB3_UPDATE_MODE)
	if (!OMAPLFBSetUpdateMode(psDevInfo, PVR_OMAPFB3_UPDATE_MODE))
	{
		printk(KERN_WARNING DRIVER_PREFIX ": %s: Device %u: Couldn't set frame buffer update mode %d\n", __FUNCTION__, psDevInfo->uiFBDevID, PVR_OMAPFB3_UPDATE_MODE);
	}
#endif /* defined(PVR_OMAPFB3_UPDATE_MODE) */

	psSwapChain->ulBufferCount = (unsigned long)ui32BufferCount;
	psSwapChain->bNotVSynced = OMAPLFB_TRUE;
	psSwapChain->uiFBDevID = psDevInfo->uiFBDevID;

	if (OMAPLFBCreateSwapQueue(psSwapChain) != OMAPLFB_OK)
	{ 
		printk(KERN_WARNING DRIVER_PREFIX ": %s: Device %u: Failed to create workqueue\n", __FUNCTION__, psDevInfo->uiFBDevID);
		eError = PVRSRV_ERROR_UNABLE_TO_INSTALL_ISR;
		goto ErrorFreeBuffers;
	}

	if (OMAPLFBEnableLFBEventNotification(psDevInfo)!= OMAPLFB_OK)
	{
		eError = PVRSRV_ERROR_UNABLE_TO_ENABLE_EVENT;
		printk(KERN_WARNING DRIVER_PREFIX ": %s: Device %u: Couldn't enable framebuffer event notification\n", __FUNCTION__, psDevInfo->uiFBDevID);
		goto ErrorDestroySwapQueue;
	}

	psDevInfo->uiSwapChainID++;
	if (psDevInfo->uiSwapChainID == 0)
	{
		psDevInfo->uiSwapChainID++;
	}

	psSwapChain->uiSwapChainID = psDevInfo->uiSwapChainID;

	psDevInfo->psSwapChain = psSwapChain;

	*pui32SwapChainID = psDevInfo->uiSwapChainID;

	*phSwapChain = (IMG_HANDLE)psSwapChain;

	eError = PVRSRV_OK;
	goto ExitUnLock;

ErrorDestroySwapQueue:
	OMAPLFBDestroySwapQueue(psSwapChain);
ErrorFreeBuffers:
	if(psSwapChain->psBuffer)
	{
		OMAPLFBFreeKernelMem(psSwapChain->psBuffer);
	}
ErrorFreeSwapChain:
	OMAPLFBFreeKernelMem(psSwapChain);
ExitUnLock:
	OMAPLFBCreateSwapChainUnLock(psDevInfo);
	return eError;
}

/*
 * DestroyDCSwapChain
 * Called from services.
 */
static PVRSRV_ERROR DestroyDCSwapChain(IMG_HANDLE hDevice,
	IMG_HANDLE hSwapChain)
{
	OMAPLFB_DEVINFO	*psDevInfo;
	OMAPLFB_SWAPCHAIN *psSwapChain;
	OMAPLFB_ERROR eError;

	/* Check parameters */
	if(!hDevice || !hSwapChain)
	{
		return PVRSRV_ERROR_INVALID_PARAMS;
	}
	
	psDevInfo = (OMAPLFB_DEVINFO*)hDevice;
	psSwapChain = (OMAPLFB_SWAPCHAIN*)hSwapChain;

	OMAPLFBCreateSwapChainLock(psDevInfo);

	if (SwapChainHasChanged(psDevInfo, psSwapChain))
	{
		printk(KERN_WARNING DRIVER_PREFIX
			": %s: Device %u: Swap chain mismatch\n", __FUNCTION__, psDevInfo->uiFBDevID);

		eError = PVRSRV_ERROR_INVALID_PARAMS;
		goto ExitUnLock;
	}

	/* The swap queue is flushed before being destroyed */
	OMAPLFBDestroySwapQueue(psSwapChain);

	eError = OMAPLFBDisableLFBEventNotification(psDevInfo);
	if (eError != OMAPLFB_OK)
	{
		printk(KERN_WARNING DRIVER_PREFIX ": %s: Device %u: Couldn't disable framebuffer event notification\n", __FUNCTION__, psDevInfo->uiFBDevID);
	}

	/* Free resources */
	if (psSwapChain->psBuffer)
	{
		OMAPLFBFreeKernelMem(psSwapChain->psBuffer);
	}
	OMAPLFBFreeKernelMem(psSwapChain);

	psDevInfo->psSwapChain = NULL;

	OMAPLFBFlip(psDevInfo, &psDevInfo->sSystemBuffer);
	(void) OMAPLFBCheckModeAndSync(psDevInfo);

	eError = PVRSRV_OK;

ExitUnLock:
	OMAPLFBCreateSwapChainUnLock(psDevInfo);

	return eError;
}

/*
 * SetDCDstRect
 * Called from services.
 */
static PVRSRV_ERROR SetDCDstRect(IMG_HANDLE hDevice,
	IMG_HANDLE hSwapChain,
	IMG_RECT *psRect)
{
	UNREFERENCED_PARAMETER(hDevice);
	UNREFERENCED_PARAMETER(hSwapChain);
	UNREFERENCED_PARAMETER(psRect);

	/* Only full display swapchains on this device */
	
	return PVRSRV_ERROR_NOT_SUPPORTED;
}

/*
 * SetDCSrcRect
 * Called from services.
 */
static PVRSRV_ERROR SetDCSrcRect(IMG_HANDLE hDevice,
                                 IMG_HANDLE hSwapChain,
                                 IMG_RECT *psRect)
{
	UNREFERENCED_PARAMETER(hDevice);
	UNREFERENCED_PARAMETER(hSwapChain);
	UNREFERENCED_PARAMETER(psRect);

	/* Only full display swapchains on this device */

	return PVRSRV_ERROR_NOT_SUPPORTED;
}

/*
 * SetDCDstColourKey
 * Called from services.
 */
static PVRSRV_ERROR SetDCDstColourKey(IMG_HANDLE hDevice,
                                      IMG_HANDLE hSwapChain,
                                      IMG_UINT32 ui32CKColour)
{
	UNREFERENCED_PARAMETER(hDevice);
	UNREFERENCED_PARAMETER(hSwapChain);
	UNREFERENCED_PARAMETER(ui32CKColour);

	/* Don't support DST CK on this device */

	return PVRSRV_ERROR_NOT_SUPPORTED;
}

/*
 * SetDCSrcColourKey
 * Called from services.
 */
static PVRSRV_ERROR SetDCSrcColourKey(IMG_HANDLE hDevice,
                                      IMG_HANDLE hSwapChain,
                                      IMG_UINT32 ui32CKColour)
{
	UNREFERENCED_PARAMETER(hDevice);
	UNREFERENCED_PARAMETER(hSwapChain);
	UNREFERENCED_PARAMETER(ui32CKColour);

	/* Don't support SRC CK on this device */

	return PVRSRV_ERROR_NOT_SUPPORTED;
}

/*
 * GetDCBuffers
 * Called from services.
 */
static PVRSRV_ERROR GetDCBuffers(IMG_HANDLE hDevice,
                                 IMG_HANDLE hSwapChain,
                                 IMG_UINT32 *pui32BufferCount,
                                 IMG_HANDLE *phBuffer)
{
	OMAPLFB_DEVINFO   *psDevInfo;
	OMAPLFB_SWAPCHAIN *psSwapChain;
	PVRSRV_ERROR eError;
	unsigned i;
	
	/* Check parameters */
	if(!hDevice 
	|| !hSwapChain
	|| !pui32BufferCount
	|| !phBuffer)
	{
		return PVRSRV_ERROR_INVALID_PARAMS;
	}
	
	psDevInfo = (OMAPLFB_DEVINFO*)hDevice;
	psSwapChain = (OMAPLFB_SWAPCHAIN*)hSwapChain;

	OMAPLFBCreateSwapChainLock(psDevInfo);

	if (SwapChainHasChanged(psDevInfo, psSwapChain))
	{
		printk(KERN_WARNING DRIVER_PREFIX
			": %s: Device %u: Swap chain mismatch\n", __FUNCTION__, psDevInfo->uiFBDevID);

		eError = PVRSRV_ERROR_INVALID_PARAMS;
		goto Exit;
	}
	
	/* Return the buffer count */
	*pui32BufferCount = (IMG_UINT32)psSwapChain->ulBufferCount;
	
	/* Return the buffers */
	for(i=0; i<psSwapChain->ulBufferCount; i++)
	{
		phBuffer[i] = (IMG_HANDLE)&psSwapChain->psBuffer[i];
	}
	
	eError = PVRSRV_OK;

Exit:
	OMAPLFBCreateSwapChainUnLock(psDevInfo);

	return eError;
}

/*
 * SwapToDCBuffer
 * Called from services.
 */
static PVRSRV_ERROR SwapToDCBuffer(IMG_HANDLE hDevice,
                                   IMG_HANDLE hBuffer,
                                   IMG_UINT32 ui32SwapInterval,
                                   IMG_HANDLE hPrivateTag,
                                   IMG_UINT32 ui32ClipRectCount,
                                   IMG_RECT *psClipRect)
{
	UNREFERENCED_PARAMETER(hDevice);
	UNREFERENCED_PARAMETER(hBuffer);
	UNREFERENCED_PARAMETER(ui32SwapInterval);
	UNREFERENCED_PARAMETER(hPrivateTag);
	UNREFERENCED_PARAMETER(ui32ClipRectCount);
	UNREFERENCED_PARAMETER(psClipRect);
	
	/* * Nothing to do since Services common code does the work */

	return PVRSRV_OK;
}

/*
 * Called after the screen has unblanked, or after any other occasion
 * when we didn't wait for vsync, but now need to. Not doing this after
 * unblank leads to screen jitter on some screens.
 * Returns true if the screen has been deemed to have settled.
 */
static OMAPLFB_BOOL WaitForVSyncSettle(OMAPLFB_DEVINFO *psDevInfo)
{
		unsigned i;
		for(i = 0; i < OMAPLFB_VSYNC_SETTLE_COUNT; i++)
		{
			if (DontWaitForVSync(psDevInfo) || !OMAPLFBWaitForVSync(psDevInfo))
			{
				return OMAPLFB_FALSE;
			}
		}

		return OMAPLFB_TRUE;
}

/*
 * Swap handler.
 * Called from the swap chain work queue handler.
 * There is no need to take the swap chain creation lock in here, or use
 * some other method of stopping the swap chain from being destroyed.
 * This is because the swap chain creation lock is taken when queueing work,
 * and the work queue is flushed before the swap chain is destroyed.
 */
void OMAPLFBSwapHandler(OMAPLFB_BUFFER *psBuffer)
{
	OMAPLFB_DEVINFO *psDevInfo = psBuffer->psDevInfo;
	OMAPLFB_SWAPCHAIN *psSwapChain = psDevInfo->psSwapChain;
	OMAPLFB_BOOL bPreviouslyNotVSynced;

#if defined(SUPPORT_DRI_DRM)
	if (!OMAPLFBAtomicBoolRead(&psDevInfo->sLeaveVT))
#endif
	{
		OMAPLFBFlip(psDevInfo, psBuffer);
	}

	bPreviouslyNotVSynced = psSwapChain->bNotVSynced;
	psSwapChain->bNotVSynced = OMAPLFB_TRUE;


	if (!DontWaitForVSync(psDevInfo))
	{
		OMAPLFB_UPDATE_MODE eMode = OMAPLFBGetUpdateMode(psDevInfo);
		int iBlankEvents = OMAPLFBAtomicIntRead(&psDevInfo->sBlankEvents);

		switch(eMode)
		{
			case OMAPLFB_UPDATE_MODE_AUTO:
			case OMAPLFB_UPDATE_MODE_VSYNC:
				psSwapChain->bNotVSynced = OMAPLFB_FALSE;

				if (bPreviouslyNotVSynced || psSwapChain->iBlankEvents != iBlankEvents)
				{
					psSwapChain->iBlankEvents = iBlankEvents;
					psSwapChain->bNotVSynced = !WaitForVSyncSettle(psDevInfo);
				} else if (psBuffer->ulSwapInterval != 0)
				{
					psSwapChain->bNotVSynced = !OMAPLFBWaitForVSync(psDevInfo);
				}
				break;
#if defined(PVR_OMAPFB3_MANUAL_UPDATE_SYNC_IN_SWAP)
			case OMAPLFB_UPDATE_MODE_MANUAL:
				if (psBuffer->ulSwapInterval != 0)
				{
					(void) OMAPLFBManualSync(psDevInfo);
				}
				break;
#endif
			default:
				break;
		}
	}

	psDevInfo->sPVRJTable.pfnPVRSRVCmdComplete((IMG_HANDLE)psBuffer->hCmdComplete, IMG_TRUE);
}

/* Triggered by PVRSRVSwapToDCBuffer */
static IMG_BOOL ProcessFlipV1(IMG_HANDLE hCmdCookie,
							  OMAPLFB_DEVINFO *psDevInfo,
							  OMAPLFB_SWAPCHAIN *psSwapChain,
							  OMAPLFB_BUFFER *psBuffer,
							  unsigned long ulSwapInterval)
{
	OMAPLFBCreateSwapChainLock(psDevInfo);

	/* The swap chain has been destroyed */
	if (SwapChainHasChanged(psDevInfo, psSwapChain))
	{
		DEBUG_PRINTK((KERN_WARNING DRIVER_PREFIX
			": %s: Device %u (PVR Device ID %u): The swap chain has been destroyed\n",
			__FUNCTION__, psDevInfo->uiFBDevID, psDevInfo->uiPVRDevID));
	}
	else
	{
		psBuffer->hCmdComplete = (OMAPLFB_HANDLE)hCmdCookie;
		psBuffer->ulSwapInterval = ulSwapInterval;
#if defined(CONFIG_DSSCOMP)
		if (is_tiler_addr(psBuffer->sSysAddr.uiAddr))
		{
			int res;
			IMG_UINT32 w = psBuffer->psDevInfo->sDisplayDim.ui32Width;
			IMG_UINT32 h = psBuffer->psDevInfo->sDisplayDim.ui32Height;
			struct dsscomp_setup_dispc_data comp = {
				.num_mgrs = 1,
				.mgrs[0].alpha_blending = 1,
				.num_ovls = 1,
				.ovls[0].cfg =
				{
					.width = w,
					.win.w = w,
					.crop.w = w,
					.height = h,
					.win.h = h,
					.crop.h = h,
					.stride = psBuffer->psDevInfo->sDisplayDim.ui32ByteStride,
					.color_mode = OMAP_DSS_COLOR_ARGB32,
					.enabled = 1,
					.global_alpha = 255,
				},
				.mode = DSSCOMP_SETUP_DISPLAY,
			};
			struct tiler_pa_info *pas[1] = { NULL };
			comp.ovls[0].ba = (u32) psBuffer->sSysAddr.uiAddr;
			res = dsscomp_gralloc_queue(&comp, pas, true,
								  (void *) psDevInfo->sPVRJTable.pfnPVRSRVCmdComplete,
								  (void *) psBuffer->hCmdComplete);
			if (res != 0)
			{
				DEBUG_PRINTK((KERN_WARNING DRIVER_PREFIX ": %s: Device %u: dsscomp_gralloc_queue failed (Y Offset: %lu, Error: %d)\n", __FUNCTION__, psDevInfo->uiFBDevID, psBuffer->ulYOffset, res));
			}
		}
		else
#endif /* defined(CONFIG_DSSCOMP) */
		{
			OMAPLFBQueueBufferForSwap(psSwapChain, psBuffer);
		}
	}

	OMAPLFBCreateSwapChainUnLock(psDevInfo);

	return IMG_TRUE;
}

#if defined(CONFIG_DSSCOMP)

/* Triggered by PVRSRVSwapToDCBuffer2 */
static IMG_BOOL ProcessFlipV2(IMG_HANDLE hCmdCookie,
							  OMAPLFB_DEVINFO *psDevInfo,
							  PDC_MEM_INFO *ppsMemInfos,
							  IMG_UINT32 ui32NumMemInfos,
							  struct dsscomp_setup_dispc_data *psDssData,
							  IMG_UINT32 ui32DssDataLength)
{
	struct tiler_pa_info *apsTilerPAs[5];
	IMG_UINT32 i, k;
	struct
	{
		IMG_UINTPTR_T uiAddr;
		IMG_UINTPTR_T uiUVAddr;
		struct tiler_pa_info *psTilerInfo;
	}
	asMemInfo[5] = {};
	int res;

	if(!psDssData)
	{
		if(ui32NumMemInfos == 1)
		{
			OMAPLFB_BUFFER sBuffer;
			IMG_CPU_PHYADDR phyAddr;

			psDevInfo->sPVRJTable.pfnPVRSRVDCMemInfoGetCpuPAddr(ppsMemInfos[0], 0, &phyAddr);

			/* Fake up an OMAPLFB_BUFFER */
			sBuffer.psNext				= NULL;
			sBuffer.psDevInfo			= psDevInfo;
			sBuffer.ulYOffset			= 0;
			sBuffer.sSysAddr.uiAddr		= phyAddr.uiAddr;
			sBuffer.sCPUVAddr			= 0;
			sBuffer.psSyncData			= NULL;
			sBuffer.hCmdComplete		= (OMAPLFB_HANDLE)hCmdCookie;
			sBuffer.ulSwapInterval		= 0;

			/* If we got a meminfo but no private data, assume the 'null' HWC
			 * backend is in use, and emulate a swapchain-less ProcessFlipV1.
			 */
			OMAPLFBFlip(psDevInfo, &sBuffer);

			/* FIXME: Why do this? Shouldn't we use the hCmdCookie correctly,
			 * like ProcessFlipV1 does?
			 */
			psDevInfo->sPVRJTable.pfnPVRSRVCmdComplete(hCmdCookie, IMG_TRUE);
		}
		else
		{
			printk(KERN_WARNING DRIVER_PREFIX
				   ": %s: Device %u: WARNING: psDispcData was NULL. "
				   "The HWC probably has a bug. Silently ignoring.",
				   __FUNCTION__, psDevInfo->uiFBDevID);
		}

		psDevInfo->sPVRJTable.pfnPVRSRVCmdComplete(hCmdCookie, IMG_TRUE);
		return IMG_TRUE;
	}

	if(ui32DssDataLength != sizeof(*psDssData))
	{
		WARN(1, "invalid size of private data (%d vs %d)",
			 ui32DssDataLength, sizeof(*psDssData));
		return IMG_FALSE;
	}

	if(psDssData->num_ovls == 0 || ui32NumMemInfos == 0)
	{
		WARN(1, "must have at least one layer");
		return IMG_FALSE;
	}

	if(DontWaitForVSync(psDevInfo))
	{
		psDevInfo->sPVRJTable.pfnPVRSRVCmdComplete(hCmdCookie, IMG_TRUE);
		return IMG_TRUE;
	}

	for(i = k = 0; i < ui32NumMemInfos && k < ARRAY_SIZE(asMemInfo); i++, k++)
	{
		struct tiler_pa_info *psTilerInfo;
		IMG_CPU_VIRTADDR virtAddr;
		IMG_CPU_PHYADDR phyAddr;
		IMG_UINT32 ui32NumPages;
		IMG_SIZE_T uByteSize;
		int j;

		psDevInfo->sPVRJTable.pfnPVRSRVDCMemInfoGetByteSize(ppsMemInfos[i], &uByteSize);
		ui32NumPages = (uByteSize + PAGE_SIZE - 1) >> PAGE_SHIFT;

		psDevInfo->sPVRJTable.pfnPVRSRVDCMemInfoGetCpuPAddr(ppsMemInfos[i], 0, &phyAddr);

		/* TILER buffers do not need meminfos */
		if(is_tiler_addr((u32)phyAddr.uiAddr))
		{
#ifdef CONFIG_DRM_OMAP_DMM_TILER
			enum tiler_fmt fmt;
#endif
			asMemInfo[k].uiAddr = phyAddr.uiAddr;
#ifdef CONFIG_DRM_OMAP_DMM_TILER
			if(tiler_get_fmt((u32)phyAddr.uiAddr, &fmt) && fmt == TILFMT_8BIT)
#else
			if(tiler_fmt((u32)phyAddr.uiAddr) == TILFMT_8BIT)
#endif
			{
				psDevInfo->sPVRJTable.pfnPVRSRVDCMemInfoGetCpuPAddr(ppsMemInfos[i], (uByteSize * 2) / 3, &phyAddr);
				asMemInfo[k].uiUVAddr = phyAddr.uiAddr;
			}
			continue;
		}

		/* normal gralloc layer */
		psTilerInfo = kzalloc(sizeof(*psTilerInfo), GFP_KERNEL);
		if(!psTilerInfo)
		{
			continue;
		}

		psTilerInfo->mem = kzalloc(sizeof(*psTilerInfo->mem) * ui32NumPages, GFP_KERNEL);
		if(!psTilerInfo->mem)
		{
			kfree(psTilerInfo);
			continue;
		}

		psTilerInfo->num_pg = ui32NumPages;
		psTilerInfo->memtype = TILER_MEM_USING;
		for(j = 0; j < ui32NumPages; j++)
		{
			psDevInfo->sPVRJTable.pfnPVRSRVDCMemInfoGetCpuPAddr(ppsMemInfos[i], j << PAGE_SHIFT, &phyAddr);
			psTilerInfo->mem[j] = (u32)phyAddr.uiAddr;
		}

		/* need base address for in-page offset */
		psDevInfo->sPVRJTable.pfnPVRSRVDCMemInfoGetCpuVAddr(ppsMemInfos[i], &virtAddr);
		asMemInfo[k].uiAddr = (IMG_UINTPTR_T) virtAddr;
		asMemInfo[k].psTilerInfo = psTilerInfo;
	}

	for(i = 0; i < psDssData->num_ovls; i++)
	{
		unsigned int ix;
		apsTilerPAs[i] = NULL;

		/* only supporting Post2, cloned and fbmem layers */
		if (psDssData->ovls[i].addressing != OMAP_DSS_BUFADDR_LAYER_IX &&
		    psDssData->ovls[i].addressing != OMAP_DSS_BUFADDR_OVL_IX &&
		    psDssData->ovls[i].addressing != OMAP_DSS_BUFADDR_FB)
		{
			psDssData->ovls[i].cfg.enabled = false;
		}

		if (psDssData->ovls[i].addressing != OMAP_DSS_BUFADDR_LAYER_IX)
		{
			continue;
		}

		/* Post2 layers */
		ix = psDssData->ovls[i].ba;
		if (ix >= k)
		{
			WARN(1, "Invalid Post2 layer (%u)", ix);
			psDssData->ovls[i].cfg.enabled = false;
			continue;
		}

		psDssData->ovls[i].addressing = OMAP_DSS_BUFADDR_DIRECT;
		psDssData->ovls[i].ba = (u32) asMemInfo[ix].uiAddr;
		psDssData->ovls[i].uv = (u32) asMemInfo[ix].uiUVAddr;
		apsTilerPAs[i] = asMemInfo[ix].psTilerInfo;
	}

	res = dsscomp_gralloc_queue(psDssData, apsTilerPAs, false,
						  (void *)psDevInfo->sPVRJTable.pfnPVRSRVCmdComplete,
						  (void *)hCmdCookie);
	if (res != 0)
	{
		DEBUG_PRINTK((KERN_WARNING DRIVER_PREFIX ": %s: Device %u: dsscomp_gralloc_queue failed (%d)\n", __FUNCTION__, psDevInfo->uiFBDevID, res));
	}

	for(i = 0; i < k; i++)
	{
		tiler_pa_free(apsTilerPAs[i]);
	}

	return IMG_TRUE;
}

#endif /* defined(CONFIG_DSSCOMP) */

/* Command processing flip handler function.  Called from services. */
static IMG_BOOL ProcessFlip(IMG_HANDLE  hCmdCookie,
                            IMG_UINT32  ui32DataSize,
                            IMG_VOID   *pvData)
{
	DISPLAYCLASS_FLIP_COMMAND *psFlipCmd;
	OMAPLFB_DEVINFO *psDevInfo;

	/* Check parameters  */
	if(!hCmdCookie || !pvData)
	{
		return IMG_FALSE;
	}

	/* Validate data packet  */
	psFlipCmd = (DISPLAYCLASS_FLIP_COMMAND*)pvData;

	if (psFlipCmd == IMG_NULL)
	{
		return IMG_FALSE;
	}

	psDevInfo = (OMAPLFB_DEVINFO*)psFlipCmd->hExtDevice;

	if(psFlipCmd->hExtBuffer)
	{
		return ProcessFlipV1(hCmdCookie,
							 psDevInfo,
							 psFlipCmd->hExtSwapChain,
							 psFlipCmd->hExtBuffer,
							 psFlipCmd->ui32SwapInterval);
	}
	else
	{
#if defined(CONFIG_DSSCOMP)
		DISPLAYCLASS_FLIP_COMMAND2 *psFlipCmd2;
		psFlipCmd2 = (DISPLAYCLASS_FLIP_COMMAND2 *)pvData;
		return ProcessFlipV2(hCmdCookie,
							 psDevInfo,
							 psFlipCmd2->ppsMemInfos,
							 psFlipCmd2->ui32NumMemInfos,
							 psFlipCmd2->pvPrivData,
							 psFlipCmd2->ui32PrivDataLength);
#else
		BUG();
#endif
	}
}

/*!
******************************************************************************

 @Function	OMAPLFBInitFBDev
 
 @Description specifies devices in the systems memory map
 
 @Input    psSysData - sys data

 @Return   OMAPLFB_ERROR  :

******************************************************************************/
static OMAPLFB_ERROR OMAPLFBInitFBDev(OMAPLFB_DEVINFO *psDevInfo)
{
	struct fb_info *psLINFBInfo;
	struct module *psLINFBOwner;
	OMAPLFB_FBINFO *psPVRFBInfo = &psDevInfo->sFBInfo;
	OMAPLFB_ERROR eError = OMAPLFB_ERROR_GENERIC;
	unsigned uiFBDevID = psDevInfo->uiFBDevID;

	OMAPLFB_CONSOLE_LOCK();

	psLINFBInfo = registered_fb[uiFBDevID];
	if (psLINFBInfo == NULL)
	{
		eError = OMAPLFB_ERROR_INVALID_DEVICE;
		goto ErrorRelSem;
	}

	psLINFBOwner = psLINFBInfo->fbops->owner;
	if (!try_module_get(psLINFBOwner))
	{
		printk(KERN_INFO DRIVER_PREFIX
			": %s: Device %u: Couldn't get framebuffer module\n", __FUNCTION__, uiFBDevID);

		goto ErrorRelSem;
	}

	if (psLINFBInfo->fbops->fb_open != NULL)
	{
		int res;

		res = psLINFBInfo->fbops->fb_open(psLINFBInfo, 0);
		if (res != 0)
		{
			printk(KERN_INFO DRIVER_PREFIX
				" %s: Device %u: Couldn't open framebuffer(%d)\n", __FUNCTION__, uiFBDevID, res);

			goto ErrorModPut;
		}
	}

	psDevInfo->psLINFBInfo = psLINFBInfo;

	psPVRFBInfo->ulWidth = psLINFBInfo->var.xres;
	psPVRFBInfo->ulHeight = psLINFBInfo->var.yres;

	if (psPVRFBInfo->ulWidth == 0 || psPVRFBInfo->ulHeight == 0)
	{
		eError = OMAPLFB_ERROR_INVALID_DEVICE;
		goto ErrorFBRel;
	}

#if !defined(CONFIG_DSSCOMP)
	psPVRFBInfo->ulFBSize = (psLINFBInfo->screen_size) != 0 ?
					psLINFBInfo->screen_size :
					psLINFBInfo->fix.smem_len;

	/*
	 * Try and filter out invalid FB info structures (a problem
	 * seen on some OMAP3 systems).
	 */
	if (psPVRFBInfo->ulFBSize == 0 || psLINFBInfo->fix.line_length == 0)
	{
		eError = OMAPLFB_ERROR_INVALID_DEVICE;
		goto ErrorFBRel;
	}

	DEBUG_PRINTK((KERN_INFO DRIVER_PREFIX
			": Device %u: Framebuffer size: %lu\n",
			psDevInfo->uiFBDevID, psPVRFBInfo->ulFBSize));
	DEBUG_PRINTK((KERN_INFO DRIVER_PREFIX
			": Device %u: Framebuffer virtual width: %u\n",
			psDevInfo->uiFBDevID, psLINFBInfo->var.xres_virtual));
	DEBUG_PRINTK((KERN_INFO DRIVER_PREFIX
			": Device %u: Framebuffer virtual height: %u\n",
			psDevInfo->uiFBDevID, psLINFBInfo->var.yres_virtual));
#endif
	DEBUG_PRINTK((KERN_INFO DRIVER_PREFIX
			": Device %u: Framebuffer width: %lu\n",
			psDevInfo->uiFBDevID, psPVRFBInfo->ulWidth));
	DEBUG_PRINTK((KERN_INFO DRIVER_PREFIX
			": Device %u: Framebuffer height: %lu\n",
			psDevInfo->uiFBDevID, psPVRFBInfo->ulHeight));

#if defined(CONFIG_DSSCOMP)
	{
#if defined(SUPPORT_PVRSRV_GET_DC_SYSTEM_BUFFER)
		/*
		 * Assume we need 3 swap buffers, and a separate system
		 * buffer.
		 */
		int n = 4;
#else
		/*
		 * Assume we need just 3 swap buffers, and no separate
		 * system buffer.
		 */
		int n = 3;
#endif
		int res;
		int i, x, y, w;
		ion_phys_addr_t phys;
		size_t size;
		struct tiler_view_t view;

		struct omap_ion_tiler_alloc_data sAllocData =
		{
			/* TILER will align width to 128-bytes */
			/* however, SGX must have full page width */
			.w = ALIGN(psPVRFBInfo->ulWidth, PAGE_SIZE / (psLINFBInfo->var.bits_per_pixel / 8)),
			.h = psPVRFBInfo->ulHeight,
			.fmt = psLINFBInfo->var.bits_per_pixel == 16 ? TILER_PIXEL_FMT_16BIT : TILER_PIXEL_FMT_32BIT,
			.flags = 0,
		};

		printk(KERN_DEBUG DRIVER_PREFIX
			   " %s: Device %u: Requesting %d TILER 2D framebuffers\n",
			   __FUNCTION__, uiFBDevID, n);

		sAllocData.w *= n;

		psPVRFBInfo->uiBytesPerPixel = psLINFBInfo->var.bits_per_pixel >> 3;
		psPVRFBInfo->bIs2D = OMAPLFB_TRUE;

		res = omap_ion_tiler_alloc(psDevInfo->psIONClient, &sAllocData);
		psPVRFBInfo->psIONHandle = sAllocData.handle;
		if (res < 0)
		{
			printk(KERN_ERR DRIVER_PREFIX
				   " %s: Device %u: Could not allocate 2D framebuffer(%d)\n",
				   __FUNCTION__, uiFBDevID, res);
			goto ErrorFBRel;
		}

		res = ion_phys(psDevInfo->psIONClient, sAllocData.handle, &phys, &size);
		if (res < 0)
		{
			printk(KERN_ERR DRIVER_PREFIX
				   " %s: Device %u: Could not get 2D framebufferphysical address (%d)\n",
				   __FUNCTION__, uiFBDevID, res);
			goto ErrorFBRel;
		}

		psPVRFBInfo->sSysAddr.uiAddr = phys;
		psPVRFBInfo->sCPUVAddr = 0;

		psPVRFBInfo->ulByteStride = PAGE_ALIGN(psPVRFBInfo->ulWidth * psPVRFBInfo->uiBytesPerPixel);
		w = psPVRFBInfo->ulByteStride >> PAGE_SHIFT;
 
		psPVRFBInfo->ulFBSize = sAllocData.h * n * psPVRFBInfo->ulByteStride;
		psPVRFBInfo->psPageList = kzalloc(w * n * psPVRFBInfo->ulHeight * sizeof(*psPVRFBInfo->psPageList), GFP_KERNEL);
		if (!psPVRFBInfo->psPageList)
		{
			printk(KERN_WARNING DRIVER_PREFIX ": %s: Device %u: Could not allocate page list\n", __FUNCTION__, psDevInfo->uiFBDevID);
			ion_free(psDevInfo->psIONClient, sAllocData.handle);
			goto ErrorFBRel;
		}

		tilview_create(&view, phys, psDevInfo->sFBInfo.ulWidth, psDevInfo->sFBInfo.ulHeight);
		for(i = 0; i < n; i++)
		{
			for(y = 0; y < psDevInfo->sFBInfo.ulHeight; y++)
			{
				for(x = 0; x < w; x++)
				{
					psPVRFBInfo->psPageList[i * psDevInfo->sFBInfo.ulHeight * w + y * w + x].uiAddr =
					phys + view.v_inc * y + ((x + i * w) << PAGE_SHIFT);
				}
			}
		}
	}
#else /* defined(CONFIG_DSSCOMP) */
	/* System Surface */
	psPVRFBInfo->sSysAddr.uiAddr = psLINFBInfo->fix.smem_start;
	psPVRFBInfo->sCPUVAddr = psLINFBInfo->screen_base;

	psPVRFBInfo->ulByteStride =  psLINFBInfo->fix.line_length;
#endif /* defined(CONFIG_DSSCOMP) */

	DEBUG_PRINTK((KERN_INFO DRIVER_PREFIX
			": Device %u: Framebuffer physical address: 0x%x\n",
			psDevInfo->uiFBDevID, psPVRFBInfo->sSysAddr.uiAddr));

	if (psPVRFBInfo->sCPUVAddr != NULL)
	{
		DEBUG_PRINTK((KERN_INFO DRIVER_PREFIX
			": Device %u: Framebuffer virtual address: %p\n",
			psDevInfo->uiFBDevID, psPVRFBInfo->sCPUVAddr));
	}

	DEBUG_PRINTK((KERN_INFO DRIVER_PREFIX
			": Device %u: Framebuffer stride: %lu\n",
			psDevInfo->uiFBDevID, psPVRFBInfo->ulByteStride));

	/* Additional implementation specific information */
	OMAPLFBPrintInfo(psDevInfo);

	psPVRFBInfo->ulBufferSize = psPVRFBInfo->ulHeight * psPVRFBInfo->ulByteStride;

#if defined(CONFIG_DSSCOMP)
	psPVRFBInfo->ulRoundedBufferSize = psPVRFBInfo->ulBufferSize;
#else
	{
		unsigned long ulLCM;
		ulLCM = LCM(psPVRFBInfo->ulByteStride, OMAPLFB_PAGE_SIZE);

		DEBUG_PRINTK((KERN_INFO DRIVER_PREFIX
			": Device %u: LCM of stride and page size: %lu\n",
			psDevInfo->uiFBDevID, ulLCM));

		/* Round the buffer size up to a multiple of the number of pages
		 * and the byte stride.
		 * This is used internally, to ensure buffers start on page
		 * boundaries, for the benefit of PVR Services.
		 */
		psPVRFBInfo->ulRoundedBufferSize = RoundUpToMultiple(psPVRFBInfo->ulBufferSize, ulLCM);
	}
#endif
	if(psLINFBInfo->var.bits_per_pixel == 16)
	{
		if((psLINFBInfo->var.red.length == 5) &&
			(psLINFBInfo->var.green.length == 6) && 
			(psLINFBInfo->var.blue.length == 5) && 
			(psLINFBInfo->var.red.offset == 11) &&
			(psLINFBInfo->var.green.offset == 5) && 
			(psLINFBInfo->var.blue.offset == 0) && 
			(psLINFBInfo->var.red.msb_right == 0))
		{
			psPVRFBInfo->ePixelFormat = PVRSRV_PIXEL_FORMAT_RGB565;
		}
		else
		{
			printk(KERN_INFO DRIVER_PREFIX ": %s: Device %u: Unknown FB format\n", __FUNCTION__, uiFBDevID);
		}
	}
	else if(psLINFBInfo->var.bits_per_pixel == 32)
	{
		if((psLINFBInfo->var.red.length == 8) &&
			(psLINFBInfo->var.green.length == 8) && 
			(psLINFBInfo->var.blue.length == 8) && 
			(psLINFBInfo->var.red.offset == 16) &&
			(psLINFBInfo->var.green.offset == 8) && 
			(psLINFBInfo->var.blue.offset == 0) && 
			(psLINFBInfo->var.red.msb_right == 0))
		{
			psPVRFBInfo->ePixelFormat = PVRSRV_PIXEL_FORMAT_ARGB8888;
		}
		else
		{
			printk(KERN_INFO DRIVER_PREFIX ": %s: Device %u: Unknown FB format\n", __FUNCTION__, uiFBDevID);
		}
	}	
	else
	{
		printk(KERN_INFO DRIVER_PREFIX ": %s: Device %u: Unknown FB format\n", __FUNCTION__, uiFBDevID);
	}

	psDevInfo->sFBInfo.ulPhysicalWidthmm =
		((int)psLINFBInfo->var.width  > 0) ? psLINFBInfo->var.width  : 90;

	psDevInfo->sFBInfo.ulPhysicalHeightmm =
		((int)psLINFBInfo->var.height > 0) ? psLINFBInfo->var.height : 54;

	/* System Surface */
	psDevInfo->sFBInfo.sSysAddr.uiAddr = psPVRFBInfo->sSysAddr.uiAddr;
	psDevInfo->sFBInfo.sCPUVAddr = psPVRFBInfo->sCPUVAddr;

	eError = OMAPLFB_OK;
	goto ErrorRelSem;

ErrorFBRel:
	if (psLINFBInfo->fbops->fb_release != NULL) 
	{
		(void) psLINFBInfo->fbops->fb_release(psLINFBInfo, 0);
	}
ErrorModPut:
	module_put(psLINFBOwner);
ErrorRelSem:
	OMAPLFB_CONSOLE_UNLOCK();

	return eError;
}

static void OMAPLFBDeInitFBDev(OMAPLFB_DEVINFO *psDevInfo)
{
	struct fb_info *psLINFBInfo = psDevInfo->psLINFBInfo;
	struct module *psLINFBOwner;

	OMAPLFB_CONSOLE_LOCK();

#if defined(CONFIG_DSSCOMP)
	{
		OMAPLFB_FBINFO *psPVRFBInfo = &psDevInfo->sFBInfo;
		kfree(psPVRFBInfo->psPageList);
		if (psPVRFBInfo->psIONHandle)
		{
			ion_free(psDevInfo->psIONClient, psPVRFBInfo->psIONHandle);
		}
	}
#endif /* defined(CONFIG_DSSCOMP) */

	psLINFBOwner = psLINFBInfo->fbops->owner;

	if (psLINFBInfo->fbops->fb_release != NULL) 
	{
		(void) psLINFBInfo->fbops->fb_release(psLINFBInfo, 0);
	}

	module_put(psLINFBOwner);

	OMAPLFB_CONSOLE_UNLOCK();
}

static OMAPLFB_DEVINFO *OMAPLFBInitDev(unsigned uiFBDevID)
{
	PFN_CMD_PROC	 	pfnCmdProcList[OMAPLFB_COMMAND_COUNT];
	IMG_UINT32		aui32SyncCountList[OMAPLFB_COMMAND_COUNT][2];
	OMAPLFB_DEVINFO		*psDevInfo = NULL;

	/* Allocate device info. structure */
	psDevInfo = (OMAPLFB_DEVINFO *)OMAPLFBAllocKernelMem(sizeof(OMAPLFB_DEVINFO));

	if(psDevInfo == NULL)
	{
		printk(KERN_ERR DRIVER_PREFIX
			": %s: Device %u: Couldn't allocate device information structure\n", __FUNCTION__, uiFBDevID);

		goto ErrorExit;
	}

	/* Any fields not set will be zero */
	memset(psDevInfo, 0, sizeof(OMAPLFB_DEVINFO));

	psDevInfo->uiFBDevID = uiFBDevID;

	/* Get the kernel services function table */
	if(!(*gpfnGetPVRJTable)(&psDevInfo->sPVRJTable))
	{
		goto ErrorFreeDevInfo;
	}

#if defined(CONFIG_ION_OMAP)
	psDevInfo->psIONClient =
		ion_client_create(omap_ion_device,
#if defined(SUPPORT_OLD_ION_API)
						  1 << ION_HEAP_TYPE_CARVEOUT |
						  1 << OMAP_ION_HEAP_TYPE_TILER,
#endif
						  "dc_omapfb3_linux");
	if (IS_ERR_OR_NULL(psDevInfo->psIONClient))
	{
		printk(KERN_ERR DRIVER_PREFIX
			": %s: Device %u: Failed to create ion client\n", __FUNCTION__, uiFBDevID);

		goto ErrorFreeDevInfo;
	}
#endif /* defined(CONFIG_ION_OMAP) */

	/* Save private fbdev information structure in the dev. info. */
	if(OMAPLFBInitFBDev(psDevInfo) != OMAPLFB_OK)
	{
		/*
		 * Leave it to OMAPLFBInitFBDev to print an error message, if
		 * required.  The function may have failed because
		 * there is no Linux framebuffer device corresponding
		 * to the device ID.
		 */
		goto ErrorIonClientDestroy;
	}

	psDevInfo->sDisplayInfo.ui32MaxSwapChainBuffers = (IMG_UINT32)(psDevInfo->sFBInfo.ulFBSize / psDevInfo->sFBInfo.ulRoundedBufferSize);
	if (psDevInfo->sDisplayInfo.ui32MaxSwapChainBuffers != 0)
	{
		psDevInfo->sDisplayInfo.ui32MaxSwapChains = 1;
		psDevInfo->sDisplayInfo.ui32MaxSwapInterval = 1;
#if defined(CONFIG_DSSCOMP)
		psDevInfo->sDisplayInfo.ui32MinSwapInterval = 1;
#endif
	}

	psDevInfo->sDisplayInfo.ui32PhysicalWidthmm = psDevInfo->sFBInfo.ulPhysicalWidthmm;
	psDevInfo->sDisplayInfo.ui32PhysicalHeightmm = psDevInfo->sFBInfo.ulPhysicalHeightmm;

	strncpy(psDevInfo->sDisplayInfo.szDisplayName, DISPLAY_DEVICE_NAME, MAX_DISPLAY_NAME_SIZE);

	psDevInfo->sDisplayFormat.pixelformat = psDevInfo->sFBInfo.ePixelFormat;
	psDevInfo->sDisplayDim.ui32Width      = (IMG_UINT32)psDevInfo->sFBInfo.ulWidth;
	psDevInfo->sDisplayDim.ui32Height     = (IMG_UINT32)psDevInfo->sFBInfo.ulHeight;
	psDevInfo->sDisplayDim.ui32ByteStride = (IMG_UINT32)psDevInfo->sFBInfo.ulByteStride;

	DEBUG_PRINTK((KERN_INFO DRIVER_PREFIX
		": Device %u: Maximum number of swap chain buffers: %u\n",
		psDevInfo->uiFBDevID, psDevInfo->sDisplayInfo.ui32MaxSwapChainBuffers));

	/* Setup system buffer */
	psDevInfo->sSystemBuffer.sSysAddr = psDevInfo->sFBInfo.sSysAddr;
	psDevInfo->sSystemBuffer.sCPUVAddr = psDevInfo->sFBInfo.sCPUVAddr;
	psDevInfo->sSystemBuffer.psDevInfo = psDevInfo;

	OMAPLFBInitBufferForSwap(&psDevInfo->sSystemBuffer);

#if defined(CONFIG_DSSCOMP) && defined(SUPPORT_PVRSRV_GET_DC_SYSTEM_BUFFER)
	OMAPLFBFlip(psDevInfo, &psDevInfo->sSystemBuffer);
#endif

	/*
		Setup the DC Jtable so SRVKM can call into this driver
	*/
	psDevInfo->sDCJTable.ui32TableSize = sizeof(PVRSRV_DC_SRV2DISP_KMJTABLE);
	psDevInfo->sDCJTable.pfnOpenDCDevice = OpenDCDevice;
	psDevInfo->sDCJTable.pfnCloseDCDevice = CloseDCDevice;
	psDevInfo->sDCJTable.pfnEnumDCFormats = EnumDCFormats;
	psDevInfo->sDCJTable.pfnEnumDCDims = EnumDCDims;
	psDevInfo->sDCJTable.pfnGetDCSystemBuffer = GetDCSystemBuffer;
	psDevInfo->sDCJTable.pfnGetDCInfo = GetDCInfo;
	psDevInfo->sDCJTable.pfnGetBufferAddr = GetDCBufferAddr;
	psDevInfo->sDCJTable.pfnCreateDCSwapChain = CreateDCSwapChain;
	psDevInfo->sDCJTable.pfnDestroyDCSwapChain = DestroyDCSwapChain;
	psDevInfo->sDCJTable.pfnSetDCDstRect = SetDCDstRect;
	psDevInfo->sDCJTable.pfnSetDCSrcRect = SetDCSrcRect;
	psDevInfo->sDCJTable.pfnSetDCDstColourKey = SetDCDstColourKey;
	psDevInfo->sDCJTable.pfnSetDCSrcColourKey = SetDCSrcColourKey;
	psDevInfo->sDCJTable.pfnGetDCBuffers = GetDCBuffers;
	psDevInfo->sDCJTable.pfnSwapToDCBuffer = SwapToDCBuffer;
	psDevInfo->sDCJTable.pfnSetDCState = SetDCState;

	/* Register device with services and retrieve device index */
	if(psDevInfo->sPVRJTable.pfnPVRSRVRegisterDCDevice(
		&psDevInfo->sDCJTable,
		&psDevInfo->uiPVRDevID) != PVRSRV_OK)
	{
		printk(KERN_ERR DRIVER_PREFIX
			": %s: Device %u: PVR Services device registration failed\n", __FUNCTION__, uiFBDevID);

		goto ErrorDeInitFBDev;
	}
	DEBUG_PRINTK((KERN_INFO DRIVER_PREFIX
		": Device %u: PVR Device ID: %u\n",
		psDevInfo->uiFBDevID, psDevInfo->uiPVRDevID));
	
	/* Setup private command processing function table ... */
	pfnCmdProcList[DC_FLIP_COMMAND] = ProcessFlip;

	/* ... and associated sync count(s) */
	aui32SyncCountList[DC_FLIP_COMMAND][0] = 0; /* writes */
	aui32SyncCountList[DC_FLIP_COMMAND][1] = 10; /* reads */

	/*
		Register private command processing functions with
		the Command Queue Manager and setup the general
		command complete function in the devinfo.
	*/
	if (psDevInfo->sPVRJTable.pfnPVRSRVRegisterCmdProcList(psDevInfo->uiPVRDevID,
															&pfnCmdProcList[0],
															aui32SyncCountList,
															OMAPLFB_COMMAND_COUNT) != PVRSRV_OK)
	{
		printk(KERN_ERR DRIVER_PREFIX
			": %s: Device %u: Couldn't register command processing functions with PVR Services\n", __FUNCTION__, uiFBDevID);
		goto ErrorUnregisterDevice;
	}

	OMAPLFBCreateSwapChainLockInit(psDevInfo);

	OMAPLFBAtomicBoolInit(&psDevInfo->sBlanked, OMAPLFB_FALSE);
	OMAPLFBAtomicIntInit(&psDevInfo->sBlankEvents, 0);
	OMAPLFBAtomicBoolInit(&psDevInfo->sFlushCommands, OMAPLFB_FALSE);
#if defined(CONFIG_HAS_EARLYSUSPEND)
	OMAPLFBAtomicBoolInit(&psDevInfo->sEarlySuspendFlag, OMAPLFB_FALSE);
#endif
#if defined(SUPPORT_DRI_DRM)
	OMAPLFBAtomicBoolInit(&psDevInfo->sLeaveVT, OMAPLFB_FALSE);
#endif

	return psDevInfo;

ErrorUnregisterDevice:
	(void)psDevInfo->sPVRJTable.pfnPVRSRVRemoveDCDevice(psDevInfo->uiPVRDevID);
ErrorDeInitFBDev:
	OMAPLFBDeInitFBDev(psDevInfo);
ErrorIonClientDestroy:
#if defined(CONFIG_ION_OMAP)
	ion_client_destroy(psDevInfo->psIONClient);
#endif /* defined(CONFIG_ION_OMAP) */
ErrorFreeDevInfo:
	OMAPLFBFreeKernelMem(psDevInfo);
ErrorExit:
	return NULL;
}

OMAPLFB_ERROR OMAPLFBInit(void)
{
	unsigned uiMaxFBDevIDPlusOne = OMAPLFBMaxFBDevIDPlusOne();
	unsigned i;
	unsigned uiDevicesFound = 0;

	if(OMAPLFBGetLibFuncAddr ("PVRGetDisplayClassJTable", &gpfnGetPVRJTable) != OMAPLFB_OK)
	{
		return OMAPLFB_ERROR_INIT_FAILURE;
	}

	/*
	 * We search for frame buffer devices backwards, as the last device
	 * registered with PVR Services will be the first device enumerated
	 * by PVR Services.
	 */
	for(i = uiMaxFBDevIDPlusOne; i-- != 0;)
	{
		OMAPLFB_DEVINFO *psDevInfo = OMAPLFBInitDev(i);

		if (psDevInfo != NULL)
		{
			/* Set the top-level anchor */
			OMAPLFBSetDevInfoPtr(psDevInfo->uiFBDevID, psDevInfo);
			uiDevicesFound++;
		}
	}

	return (uiDevicesFound != 0) ? OMAPLFB_OK : OMAPLFB_ERROR_INIT_FAILURE;
}

/*
 *	OMAPLFBDeInitDev
 *	DeInitialises one device
 */
static OMAPLFB_BOOL OMAPLFBDeInitDev(OMAPLFB_DEVINFO *psDevInfo)
{
	PVRSRV_DC_DISP2SRV_KMJTABLE *psPVRJTable = &psDevInfo->sPVRJTable;

	if (psPVRJTable->pfnPVRSRVRemoveCmdProcList (psDevInfo->uiPVRDevID, OMAPLFB_COMMAND_COUNT) != PVRSRV_OK)
	{
		printk(KERN_ERR DRIVER_PREFIX
			": %s: Device %u: PVR Device %u: Couldn't unregister command processing functions\n", __FUNCTION__, psDevInfo->uiFBDevID, psDevInfo->uiPVRDevID);
		return OMAPLFB_FALSE;
	}

	/*
	 * Remove display class device from kernel services device
	 * register.
	 */
	if (psPVRJTable->pfnPVRSRVRemoveDCDevice(psDevInfo->uiPVRDevID) != PVRSRV_OK)
	{
		printk(KERN_ERR DRIVER_PREFIX
			": %s: Device %u: PVR Device %u: Couldn't remove device from PVR Services\n", __FUNCTION__, psDevInfo->uiFBDevID, psDevInfo->uiPVRDevID);
		return OMAPLFB_FALSE;
	}

#if defined(CONFIG_DSSCOMP)
	/* Disable the overlay, as we will be freeing the display buffers */
	psDevInfo->sSystemBuffer.sSysAddr.uiAddr = 0;
	OMAPLFBFlip(psDevInfo, &psDevInfo->sSystemBuffer);
#endif /* defined(CONFIG_DSSCOMP) */

	OMAPLFBCreateSwapChainLockDeInit(psDevInfo);

	OMAPLFBAtomicBoolDeInit(&psDevInfo->sBlanked);
	OMAPLFBAtomicIntDeInit(&psDevInfo->sBlankEvents);
	OMAPLFBAtomicBoolDeInit(&psDevInfo->sFlushCommands);
#if defined(CONFIG_HAS_EARLYSUSPEND)
	OMAPLFBAtomicBoolDeInit(&psDevInfo->sEarlySuspendFlag);
#endif
#if defined(SUPPORT_DRI_DRM)
	OMAPLFBAtomicBoolDeInit(&psDevInfo->sLeaveVT);
#endif

	OMAPLFBDeInitFBDev(psDevInfo);

#if defined(CONFIG_ION_OMAP)
	ion_client_destroy(psDevInfo->psIONClient);
#endif

	OMAPLFBSetDevInfoPtr(psDevInfo->uiFBDevID, NULL);

	/* De-allocate data structure */
	OMAPLFBFreeKernelMem(psDevInfo);

	return OMAPLFB_TRUE;
}

/*
 *	OMAPLFBDeInit
 *	Deinitialises the display class device component of the FBDev
 */
OMAPLFB_ERROR OMAPLFBDeInit(void)
{
	unsigned uiMaxFBDevIDPlusOne = OMAPLFBMaxFBDevIDPlusOne();
	unsigned i;
	OMAPLFB_BOOL bError = OMAPLFB_FALSE;

	for(i = 0; i < uiMaxFBDevIDPlusOne; i++)
	{
		OMAPLFB_DEVINFO *psDevInfo = OMAPLFBGetDevInfoPtr(i);

		if (psDevInfo != NULL)
		{
			bError |= !OMAPLFBDeInitDev(psDevInfo);
		}
	}

	return (bError) ? OMAPLFB_ERROR_INIT_FAILURE : OMAPLFB_OK;
}

/******************************************************************************
 End of file (omaplfb_displayclass.c)
******************************************************************************/

