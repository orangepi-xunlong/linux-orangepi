/*************************************************************************/ /*!
@Title          NOHW display driver display-specific functions
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

#if defined(__linux__)
#include <linux/string.h>
#else
#include <string.h>
#endif

/* IMG services headers */
#include "img_defs.h"
#include "servicesext.h"
#include "kerneldisplay.h"

#include "dc_nohw.h"

#define DISPLAY_DEVICE_NAME "DC_NOHW"

#define DC_NOHW_COMMAND_COUNT		1

/* top level 'hook ptr' */
static void *gpvAnchor = 0;
static PFN_DC_GET_PVRJTABLE pfnGetPVRJTable = 0;




/*
	Kernel services is a kernel module and must be loaded first.
	The display controller driver is also a kernel module and must be loaded after the pvr services module.
	The display controller driver should be able to retrieve the
	address of the services PVRGetDisplayClassJTable from (the already loaded)
	kernel services module.
*/

/* returns anchor pointer */
static DC_NOHW_DEVINFO * GetAnchorPtr(void)
{
	return (DC_NOHW_DEVINFO *)gpvAnchor;
}

/* sets anchor pointer */
static void SetAnchorPtr(DC_NOHW_DEVINFO *psDevInfo)
{
	gpvAnchor = (void *)psDevInfo;
}

#if !defined(DC_NOHW_DISCONTIG_BUFFERS) && !defined(USE_BASE_VIDEO_FRAMEBUFFER)
IMG_SYS_PHYADDR CpuPAddrToSysPAddr(IMG_CPU_PHYADDR cpu_paddr)
{
	IMG_SYS_PHYADDR sys_paddr;

	/* This would only be an inequality if the CPU's MMU did not point to sys address 0,
	   ie. multi CPU system */
	sys_paddr.uiAddr = cpu_paddr.uiAddr;
	return sys_paddr;
}

IMG_CPU_PHYADDR SysPAddrToCpuPAddr(IMG_SYS_PHYADDR sys_paddr)
{
	IMG_CPU_PHYADDR cpu_paddr;

	/* This would only be an inequality if the CPU's MMU did not point to sys address 0,
	   ie. multi CPU system */
	cpu_paddr.uiAddr = sys_paddr.uiAddr;
	return cpu_paddr;
}
#endif


static PVRSRV_ERROR OpenDCDevice(IMG_UINT32 ui32DeviceID,
                                 IMG_HANDLE *phDevice,
                                 PVRSRV_SYNC_DATA* psSystemBufferSyncData)
{
	DC_NOHW_DEVINFO *psDevInfo;
	PVR_UNREFERENCED_PARAMETER(ui32DeviceID);

	psDevInfo = GetAnchorPtr();

#if defined (ENABLE_DISPLAY_MODE_TRACKING)
	if (Shadow_Desktop_Resolution(psDevInfo) != DC_OK)
	{
		return (PVRSRV_ERROR_NOT_SUPPORTED);
	}
#endif

	/* store the system surface sync data */
	psDevInfo->asBackBuffers[0].psSyncData = psSystemBufferSyncData;

	/* return handle to the devinfo */
	*phDevice = (IMG_HANDLE)psDevInfo;

#if defined(USE_BASE_VIDEO_FRAMEBUFFER)
	return (SetupDevInfo(psDevInfo));
#else
	return (PVRSRV_OK);
#endif
}


static PVRSRV_ERROR CloseDCDevice(IMG_HANDLE hDevice)
{
	UNREFERENCED_PARAMETER(hDevice);

#if defined(USE_BASE_VIDEO_FRAMEBUFFER)
	FreeBackBuffers(GetAnchorPtr());
#endif

	return (PVRSRV_OK);
}


static PVRSRV_ERROR EnumDCFormats(IMG_HANDLE		hDevice,
                                  IMG_UINT32		*pui32NumFormats,
                                  DISPLAY_FORMAT	*psFormat)
{
	DC_NOHW_DEVINFO	*psDevInfo;

	if(!hDevice || !pui32NumFormats)
	{
		return (PVRSRV_ERROR_INVALID_PARAMS);
	}

	psDevInfo = (DC_NOHW_DEVINFO *)hDevice;

	*pui32NumFormats = (IMG_UINT32)psDevInfo->ulNumFormats;

	if(psFormat != IMG_NULL)
	{
		unsigned long i;

		for(i=0; i<psDevInfo->ulNumFormats; i++)
		{
			psFormat[i] = psDevInfo->asDisplayFormatList[i];
		}
	}

	return (PVRSRV_OK);
}


static PVRSRV_ERROR EnumDCDims(IMG_HANDLE		hDevice,
                               DISPLAY_FORMAT	*psFormat,
                               IMG_UINT32		*pui32NumDims,
                               DISPLAY_DIMS		*psDim)
{
	DC_NOHW_DEVINFO	*psDevInfo;

	if(!hDevice || !psFormat || !pui32NumDims)
	{
		return (PVRSRV_ERROR_INVALID_PARAMS);
	}

	psDevInfo = (DC_NOHW_DEVINFO *)hDevice;

	*pui32NumDims = (IMG_UINT32)psDevInfo->ulNumDims;

	/* given psFormat return the available Dims */

	if(psDim != IMG_NULL)
	{
		unsigned long i;

		for(i=0; i<psDevInfo->ulNumDims; i++)
		{
			psDim[i] = psDevInfo->asDisplayDimList[i];
		}
	}

	return (PVRSRV_OK);
}


static PVRSRV_ERROR GetDCSystemBuffer(IMG_HANDLE hDevice, IMG_HANDLE *phBuffer)
{
	DC_NOHW_DEVINFO	*psDevInfo;

	if(!hDevice || !phBuffer)
	{
		return (PVRSRV_ERROR_INVALID_PARAMS);
	}

	psDevInfo = (DC_NOHW_DEVINFO *)hDevice;

	*phBuffer = (IMG_HANDLE)&psDevInfo->asBackBuffers[0];

	return (PVRSRV_OK);
}


static PVRSRV_ERROR GetDCInfo(IMG_HANDLE hDevice, DISPLAY_INFO *psDCInfo)
{
	DC_NOHW_DEVINFO	*psDevInfo;

	if(!hDevice || !psDCInfo)
	{
		return (PVRSRV_ERROR_INVALID_PARAMS);
	}

	psDevInfo = (DC_NOHW_DEVINFO *)hDevice;

	*psDCInfo = psDevInfo->sDisplayInfo;

	return (PVRSRV_OK);
}


static PVRSRV_ERROR GetDCBufferAddr(IMG_HANDLE         hDevice,
                                    IMG_HANDLE         hBuffer,
                                    IMG_SYS_PHYADDR **ppsSysAddr,
                                    IMG_UINT32        *pui32ByteSize,
                                    IMG_VOID         **ppvCpuVAddr,
                                    IMG_HANDLE        *phOSMapInfo,
                                    IMG_BOOL          *pbIsContiguous,
                                    IMG_UINT32		  *pui32TilingStride)
{
	DC_NOHW_DEVINFO	*psDevInfo;
	DC_NOHW_BUFFER	*psBuffer;

	if(!hDevice || !hBuffer || !ppsSysAddr || !pui32ByteSize)
	{
		return (PVRSRV_ERROR_INVALID_PARAMS);
	}

	psDevInfo = (DC_NOHW_DEVINFO *)hDevice;

	psBuffer = (DC_NOHW_BUFFER*)hBuffer;

	*ppvCpuVAddr = psBuffer->sCPUVAddr;

	*pui32ByteSize = (IMG_UINT32)(psDevInfo->asDisplayDimList[0].ui32Height * psDevInfo->asDisplayDimList[0].ui32ByteStride);
	*phOSMapInfo = IMG_NULL;

#if defined(DC_NOHW_DISCONTIG_BUFFERS)
	*ppsSysAddr = psBuffer->psSysAddr;
	*pbIsContiguous = IMG_FALSE;
#else
	*ppsSysAddr = &psBuffer->sSysAddr;
	*pbIsContiguous = IMG_TRUE;
#endif

#if defined(SUPPORT_MEMORY_TILING)
	{
		IMG_UINT32 ui32Stride = psDevInfo->asDisplayDimList[0].ui32ByteStride;
		IMG_UINT32 ui32NumBits = 0, ui32StrideTopBit, n;

		// How many bits for x?
		for(n = 0; n < 32; n++)
		{
			if(ui32Stride & (1<<n))
			{
				ui32NumBits = n+1;
			}
		}

		// clamp to the minimum..
		if(ui32NumBits < 10)
		{
			ui32NumBits = 10;
		}

		// Subtract one to make this a range limit..
		ui32StrideTopBit = ui32NumBits - 1;

		// Subtract 9 to prepare it for the HW..
		ui32StrideTopBit -= 9;

		*pui32TilingStride = ui32StrideTopBit;
	}
#else
	UNREFERENCED_PARAMETER(pui32TilingStride);
#endif /* defined(SUPPORT_MEMORY_TILING) */

	return (PVRSRV_OK);
}

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
	DC_NOHW_DEVINFO	*psDevInfo;
	DC_NOHW_SWAPCHAIN *psSwapChain;
	DC_NOHW_BUFFER *psBuffer;
	IMG_UINT32 i;

	UNREFERENCED_PARAMETER(ui32OEMFlags);
	UNREFERENCED_PARAMETER(pui32SwapChainID);

	/* check parameters */
	if(!hDevice
	|| !psDstSurfAttrib
	|| !psSrcSurfAttrib
	|| !ppsSyncData
	|| !phSwapChain)
	{
		return (PVRSRV_ERROR_INVALID_PARAMS);
	}

	psDevInfo = (DC_NOHW_DEVINFO*)hDevice;

	/* the dc_nohw only supports a single swapchain */
	if(psDevInfo->psSwapChain)
	{
		return (PVRSRV_ERROR_FLIP_CHAIN_EXISTS);
	}
	
	/* create a swapchain structure */
	psSwapChain = (DC_NOHW_SWAPCHAIN*)AllocKernelMem(sizeof(DC_NOHW_SWAPCHAIN));
	if(!psSwapChain)
	{
		return (PVRSRV_ERROR_OUT_OF_MEMORY);
	}
	
	memset(psSwapChain, 0, sizeof(DC_NOHW_SWAPCHAIN));

	if (ui32BufferCount)
	{
	
		/* check the buffer count */
		if(ui32BufferCount > DC_NOHW_MAX_BACKBUFFERS)
		{
			return (PVRSRV_ERROR_TOOMANYBUFFERS);
		}
	
		/*
			verify the DST/SRC attributes
			- SRC/DST must match the current display mode config
		*/
		if(psDstSurfAttrib->pixelformat != psDevInfo->sSysFormat.pixelformat
		|| psDstSurfAttrib->sDims.ui32ByteStride != psDevInfo->sSysDims.ui32ByteStride
		|| psDstSurfAttrib->sDims.ui32Width != psDevInfo->sSysDims.ui32Width
		|| psDstSurfAttrib->sDims.ui32Height != psDevInfo->sSysDims.ui32Height)
		{
			/* DST doesn't match the current mode */
			return (PVRSRV_ERROR_INVALID_PARAMS);
		}
	
		if(psDstSurfAttrib->pixelformat != psSrcSurfAttrib->pixelformat
		|| psDstSurfAttrib->sDims.ui32ByteStride != psSrcSurfAttrib->sDims.ui32ByteStride
		|| psDstSurfAttrib->sDims.ui32Width != psSrcSurfAttrib->sDims.ui32Width
		|| psDstSurfAttrib->sDims.ui32Height != psSrcSurfAttrib->sDims.ui32Height)
		{
			/* DST doesn't match the SRC */
			return (PVRSRV_ERROR_INVALID_PARAMS);
		}
	
		/* INTEGRATION_POINT: check the flags */
		UNREFERENCED_PARAMETER(ui32Flags);
	
	
	
		psBuffer = (DC_NOHW_BUFFER*)AllocKernelMem(sizeof(DC_NOHW_BUFFER) * ui32BufferCount);
		if(!psBuffer)
		{
			FreeKernelMem(psSwapChain);
			return (PVRSRV_ERROR_OUT_OF_MEMORY);
		}
	
		/* initialise allocations */
		memset(psBuffer, 0, sizeof(DC_NOHW_BUFFER) * ui32BufferCount);
	
		psSwapChain->ulBufferCount = (unsigned long)ui32BufferCount;
		psSwapChain->psBuffer = psBuffer;
	
		/* link the buffers */
		for(i=0; i<ui32BufferCount-1; i++)
		{
			psBuffer[i].psNext = &psBuffer[i+1];
		}
		/* and link last to first */
		psBuffer[i].psNext = &psBuffer[0];
	
		/* populate the buffers */
		for(i=0; i<ui32BufferCount; i++)
		{
			psBuffer[i].psSyncData = ppsSyncData[i];
#if defined(DC_NOHW_DISCONTIG_BUFFERS)
			psBuffer[i].psSysAddr = psDevInfo->asBackBuffers[i].psSysAddr;
#else
			psBuffer[i].sSysAddr = psDevInfo->asBackBuffers[i].sSysAddr;
#endif
			psBuffer[i].sDevVAddr = psDevInfo->asBackBuffers[i].sDevVAddr;
			psBuffer[i].sCPUVAddr = psDevInfo->asBackBuffers[i].sCPUVAddr;
			psBuffer[i].hSwapChain = (DC_HANDLE)psSwapChain;
		}
	}
	else
	{
		psSwapChain->psBuffer = NULL;
	}

	/* mark swapchain's existence */
	psDevInfo->psSwapChain = psSwapChain;

	/* return swapchain handle */
	*phSwapChain = (IMG_HANDLE)psSwapChain;

	/* INTEGRATION_POINT: enable Vsync ISR */

	return (PVRSRV_OK);
}


static PVRSRV_ERROR DestroyDCSwapChain(IMG_HANDLE hDevice,
                                       IMG_HANDLE hSwapChain)
{
	DC_NOHW_DEVINFO	*psDevInfo;
	DC_NOHW_SWAPCHAIN *psSwapChain;

	/* check parameters */
	if(!hDevice
	|| !hSwapChain)
	{
		return (PVRSRV_ERROR_INVALID_PARAMS);
	}

	psDevInfo = (DC_NOHW_DEVINFO*)hDevice;
	psSwapChain = (DC_NOHW_SWAPCHAIN*)hSwapChain;

	/* free resources */
	if (psSwapChain->psBuffer)
	{
		FreeKernelMem(psSwapChain->psBuffer);
	}
	FreeKernelMem(psSwapChain);

	/* mark swapchain as not existing */
	psDevInfo->psSwapChain = 0;

	/* INTEGRATION_POINT: disable Vsync ISR */

	return (PVRSRV_OK);
}


static PVRSRV_ERROR SetDCDstRect(IMG_HANDLE	hDevice,
                                 IMG_HANDLE	hSwapChain,
                                 IMG_RECT	*psRect)
{
	UNREFERENCED_PARAMETER(hDevice);
	UNREFERENCED_PARAMETER(hSwapChain);
	UNREFERENCED_PARAMETER(psRect);

	return (PVRSRV_ERROR_NOT_SUPPORTED);
}


static PVRSRV_ERROR SetDCSrcRect(IMG_HANDLE	hDevice,
                                 IMG_HANDLE	hSwapChain,
                                 IMG_RECT	*psRect)
{
	UNREFERENCED_PARAMETER(hDevice);
	UNREFERENCED_PARAMETER(hSwapChain);
	UNREFERENCED_PARAMETER(psRect);

	return (PVRSRV_ERROR_NOT_SUPPORTED);
}


static PVRSRV_ERROR SetDCDstColourKey(IMG_HANDLE	hDevice,
                                      IMG_HANDLE	hSwapChain,
                                      IMG_UINT32	ui32CKColour)
{
	UNREFERENCED_PARAMETER(hDevice);
	UNREFERENCED_PARAMETER(hSwapChain);
	UNREFERENCED_PARAMETER(ui32CKColour);

	return (PVRSRV_ERROR_NOT_SUPPORTED);
}


static PVRSRV_ERROR SetDCSrcColourKey(IMG_HANDLE	hDevice,
                                      IMG_HANDLE	hSwapChain,
                                      IMG_UINT32	ui32CKColour)
{
	UNREFERENCED_PARAMETER(hDevice);
	UNREFERENCED_PARAMETER(hSwapChain);
	UNREFERENCED_PARAMETER(ui32CKColour);

	return (PVRSRV_ERROR_NOT_SUPPORTED);
}


static PVRSRV_ERROR GetDCBuffers(IMG_HANDLE hDevice,
                                 IMG_HANDLE hSwapChain,
                                 IMG_UINT32 *pui32BufferCount,
                                 IMG_HANDLE *phBuffer)
{
/*	DC_NOHW_DEVINFO	*psDevInfo; */
	DC_NOHW_SWAPCHAIN *psSwapChain;
	unsigned long i;

	/* check parameters */
	if(!hDevice
	|| !hSwapChain
	|| !pui32BufferCount
	|| !phBuffer)
	{
		return (PVRSRV_ERROR_INVALID_PARAMS);
	}

/*	psDevInfo = (DC_NOHW_DEVINFO*)hDevice; */
	psSwapChain = (DC_NOHW_SWAPCHAIN*)hSwapChain;

	/* return the buffer count */
	*pui32BufferCount = (IMG_UINT32)psSwapChain->ulBufferCount;

	/* return the buffers */
	for(i=0; i<psSwapChain->ulBufferCount; i++)
	{
		phBuffer[i] = (IMG_HANDLE)&psSwapChain->psBuffer[i];
	}

	return (PVRSRV_OK);
}


static PVRSRV_ERROR SwapToDCBuffer(IMG_HANDLE	hDevice,
                                   IMG_HANDLE	hBuffer,
                                   IMG_UINT32	ui32SwapInterval,
                                   IMG_HANDLE	hPrivateTag,
                                   IMG_UINT32	ui32ClipRectCount,
                                   IMG_RECT		*psClipRect)
{
	UNREFERENCED_PARAMETER(ui32SwapInterval);
	UNREFERENCED_PARAMETER(hPrivateTag);
	UNREFERENCED_PARAMETER(psClipRect);

	if(!hDevice
	|| !hBuffer
	|| (ui32ClipRectCount != 0))
	{
		return (PVRSRV_ERROR_INVALID_PARAMS);
	}

	/* nothing to do for no hw */
	return (PVRSRV_OK);
}


static DC_ERROR Flip(DC_NOHW_DEVINFO	*psDevInfo,
                     DC_NOHW_BUFFER		*psBuffer)
{
	UNREFERENCED_PARAMETER(psBuffer);
	/* check parameters */
	if(!psDevInfo)
	{
		return (DC_ERROR_INVALID_PARAMS);
	}
	/* to be implemented */

	return (DC_OK);
}


static IMG_BOOL ProcessFlip(IMG_HANDLE	hCmdCookie,
                            IMG_UINT32	ui32DataSize,
                            IMG_VOID	*pvData)
{
	DC_ERROR eError;
	DISPLAYCLASS_FLIP_COMMAND *psFlipCmd;
	DC_NOHW_DEVINFO	*psDevInfo;
	DC_NOHW_BUFFER	*psBuffer;
	
	UNREFERENCED_PARAMETER(ui32DataSize);

	/* check parameters */
	if(!hCmdCookie)
	{
		return (IMG_FALSE);
	}

	/* validate data packet */
	psFlipCmd = (DISPLAYCLASS_FLIP_COMMAND*)pvData;
	/* Under android, this may be a DISPLAYCLASS_FLIP_COMMAND2, but the structs
	 * are compatable for everything used by dc_nohw so it makes no difference */
	if (psFlipCmd == IMG_NULL)
	{
		return (IMG_FALSE);
	}

	/* setup some useful pointers */
	psDevInfo = (DC_NOHW_DEVINFO*)psFlipCmd->hExtDevice;

	psBuffer = (DC_NOHW_BUFFER*)psFlipCmd->hExtBuffer;

	/* flip the display */
	eError = Flip(psDevInfo, psBuffer);
	if(eError != DC_OK)
	{
		return (IMG_FALSE);
	}

	/* call command complete Callback */
	psDevInfo->sPVRJTable.pfnPVRSRVCmdComplete(hCmdCookie, IMG_FALSE);

	return (IMG_TRUE);
}


DC_ERROR Init(void)
{
	DC_NOHW_DEVINFO *psDevInfo;
	DC_ERROR         eError;
	unsigned long    ulBBuf;
	unsigned long    ulNBBuf;
	/*
		- connect to services
		- register with services
		- allocate and setup private data structure
	*/


	/*
		in kernel driver, data structures must be anchored to something for subsequent retrieval
		this may be a single global pointer or TLS or something else - up to you
		call API to retrieve this ptr
	*/

	/*
		get the anchor pointer
	*/
	psDevInfo = GetAnchorPtr();

	if (psDevInfo == 0)
	{
		PFN_CMD_PROC  pfnCmdProcList[DC_NOHW_COMMAND_COUNT];
		IMG_UINT32    aui32SyncCountList[DC_NOHW_COMMAND_COUNT][2];

		/* allocate device info. structure */
		psDevInfo = (DC_NOHW_DEVINFO *)AllocKernelMem(sizeof(*psDevInfo));

		if(!psDevInfo)
		{
			eError = DC_ERROR_OUT_OF_MEMORY;/* failure */
			goto ExitError;
		}

		/* initialise allocation */
		memset(psDevInfo, 0, sizeof(*psDevInfo));

		/* set the top-level anchor */
		SetAnchorPtr((void*)psDevInfo);

		/* set ref count */
		psDevInfo->ulRefCount = 0UL;

		if(OpenPVRServices(&psDevInfo->hPVRServices) != DC_OK)
		{
			eError = DC_ERROR_INIT_FAILURE;
			goto ExitFreeDevInfo;
		}
		if(GetLibFuncAddr (psDevInfo->hPVRServices, "PVRGetDisplayClassJTable", &pfnGetPVRJTable) != DC_OK)
		{
			eError = DC_ERROR_INIT_FAILURE;
			goto ExitCloseServices;
		}

		/* got the kernel services function table */
		if((*pfnGetPVRJTable)(&psDevInfo->sPVRJTable) == IMG_FALSE)
		{
			eError = DC_ERROR_INIT_FAILURE;
			goto ExitCloseServices;
		}

		/*
			Setup the devinfo
		*/
		psDevInfo->psSwapChain = 0;
		psDevInfo->sDisplayInfo.ui32MinSwapInterval = 0UL;
		psDevInfo->sDisplayInfo.ui32MaxSwapInterval = 1UL;
		psDevInfo->sDisplayInfo.ui32MaxSwapChains = 1UL;
		psDevInfo->sDisplayInfo.ui32MaxSwapChainBuffers = DC_NOHW_MAX_BACKBUFFERS;
		strncpy(psDevInfo->sDisplayInfo.szDisplayName, DISPLAY_DEVICE_NAME, MAX_DISPLAY_NAME_SIZE);

		psDevInfo->ulNumFormats = 1UL;

		psDevInfo->ulNumDims = 1UL;

#if defined(DC_NOHW_GET_BUFFER_DIMENSIONS)
		if (!GetBufferDimensions(&psDevInfo->asDisplayDimList[0].ui32Width,
			&psDevInfo->asDisplayDimList[0].ui32Height,
			&psDevInfo->asDisplayFormatList[0].pixelformat,
			&psDevInfo->asDisplayDimList[0].ui32ByteStride))
		{
			eError = DC_ERROR_INIT_FAILURE;
			goto ExitCloseServices;
		}
#else	/* defined(DC_NOHW_GET_BUFFER_DIMENSIONS) */
	#if defined (ENABLE_DISPLAY_MODE_TRACKING)
		// Set sizes to zero to force re-alloc on display mode change.
		psDevInfo->asDisplayFormatList[0].pixelformat = DC_NOHW_BUFFER_PIXEL_FORMAT;
		psDevInfo->asDisplayDimList[0].ui32Width =  0;
		psDevInfo->asDisplayDimList[0].ui32Height =  0;
		psDevInfo->asDisplayDimList[0].ui32ByteStride = 0;
	#else
		psDevInfo->asDisplayFormatList[0].pixelformat = DC_NOHW_BUFFER_PIXEL_FORMAT;
		psDevInfo->asDisplayDimList[0].ui32Width =  DC_NOHW_BUFFER_WIDTH;
		psDevInfo->asDisplayDimList[0].ui32Height =  DC_NOHW_BUFFER_HEIGHT;
		psDevInfo->asDisplayDimList[0].ui32ByteStride = DC_NOHW_BUFFER_BYTE_STRIDE;
	#endif
#endif	/* defined(DC_NOHW_GET_BUFFER_DIMENSIONS) */

		psDevInfo->sSysFormat = psDevInfo->asDisplayFormatList[0];
		psDevInfo->sSysDims.ui32Width = psDevInfo->asDisplayDimList[0].ui32Width;
		psDevInfo->sSysDims.ui32Height = psDevInfo->asDisplayDimList[0].ui32Height;
		psDevInfo->sSysDims.ui32ByteStride = psDevInfo->asDisplayDimList[0].ui32ByteStride;
		psDevInfo->ui32BufferSize = psDevInfo->sSysDims.ui32Height * psDevInfo->sSysDims.ui32ByteStride;


		/* setup swapchain details */
		for(ulBBuf=0; ulBBuf<DC_NOHW_MAX_BACKBUFFERS; ulBBuf++)
		{
#if defined(USE_BASE_VIDEO_FRAMEBUFFER) || defined (ENABLE_DISPLAY_MODE_TRACKING)
			psDevInfo->asBackBuffers[ulBBuf].sSysAddr.uiAddr = IMG_NULL;
			psDevInfo->asBackBuffers[ulBBuf].sCPUVAddr = IMG_NULL;
#else
#if defined(DC_NOHW_DISCONTIG_BUFFERS)
			if (AllocDiscontigMemory(psDevInfo->ui32BufferSize,
								  &psDevInfo->asBackBuffers[ulBBuf].hMemChunk,
								  &psDevInfo->asBackBuffers[ulBBuf].sCPUVAddr,
								  &psDevInfo->asBackBuffers[ulBBuf].psSysAddr) != DC_OK)
			{
				eError = DC_ERROR_INIT_FAILURE;
				goto ExitFreeMem;
			}
#else
			IMG_CPU_PHYADDR		sBufferCPUPAddr;

			if (AllocContigMemory(psDevInfo->ui32BufferSize,
								  &psDevInfo->asBackBuffers[ulBBuf].hMemChunk,
								  &psDevInfo->asBackBuffers[ulBBuf].sCPUVAddr,
								  &sBufferCPUPAddr) != DC_OK)
			{
				eError = DC_ERROR_INIT_FAILURE;
				goto ExitFreeMem;
			}

			psDevInfo->asBackBuffers[ulBBuf].sSysAddr =  CpuPAddrToSysPAddr(sBufferCPUPAddr);
#endif
#endif /* #if defined(USE_BASE_VIDEO_FRAMEBUFFER) */
			/* sDevVAddr not meaningful for nohw */
			psDevInfo->asBackBuffers[ulBBuf].sDevVAddr.uiAddr = 0UL;
			psDevInfo->asBackBuffers[ulBBuf].hSwapChain = 0;
			psDevInfo->asBackBuffers[ulBBuf].psSyncData = 0;
			psDevInfo->asBackBuffers[ulBBuf].psNext = 0;
		}

		/*
			setup the DC Jtable so SRVKM can call into this driver
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
		psDevInfo->sDCJTable.pfnSetDCState = IMG_NULL;

		/* register device with services and retrieve device index */
		if(psDevInfo->sPVRJTable.pfnPVRSRVRegisterDCDevice (&psDevInfo->sDCJTable,
															&psDevInfo->uiDeviceID ) != PVRSRV_OK)
		{
			eError = DC_ERROR_DEVICE_REGISTER_FAILED;
			goto ExitFreeMem;
		}

		/*
			setup private command processing function table
		*/
		pfnCmdProcList[DC_FLIP_COMMAND] = ProcessFlip;

		/*
			and associated sync count(s)
		*/
		aui32SyncCountList[DC_FLIP_COMMAND][0] = 0UL;/* no writes */
		aui32SyncCountList[DC_FLIP_COMMAND][1] = 2UL;/* 2 reads: To / From */

		/*
			register private command processing functions with
			the Command Queue Manager and setup the general
			command complete function in the devinfo
		*/
		if (psDevInfo->sPVRJTable.pfnPVRSRVRegisterCmdProcList(psDevInfo->uiDeviceID,
															   &pfnCmdProcList[0],
															   aui32SyncCountList,
															   DC_NOHW_COMMAND_COUNT) != PVRSRV_OK)
		{
			eError = DC_ERROR_CANT_REGISTER_CALLBACK;
			goto ExitRemoveDevice;
		}
	}

	/* increment the ref count */
	psDevInfo->ulRefCount++;

	/* return success */
	return (DC_OK);

ExitRemoveDevice:
	(IMG_VOID) psDevInfo->sPVRJTable.pfnPVRSRVRemoveDCDevice(psDevInfo->uiDeviceID);

ExitFreeMem:
	ulNBBuf = ulBBuf;
	for(ulBBuf=0; ulBBuf<ulNBBuf; ulBBuf++)
	{
#if defined(DC_NOHW_DISCONTIG_BUFFERS)
		FreeDiscontigMemory(psDevInfo->ui32BufferSize,
			 psDevInfo->asBackBuffers[ulBBuf].hMemChunk,
			 psDevInfo->asBackBuffers[ulBBuf].sCPUVAddr,
			 psDevInfo->asBackBuffers[ulBBuf].psSysAddr);
#else
#if !defined(USE_BASE_VIDEO_FRAMEBUFFER)

		FreeContigMemory(psDevInfo->ui32BufferSize,
			 psDevInfo->asBackBuffers[ulBBuf].hMemChunk,
			 psDevInfo->asBackBuffers[ulBBuf].sCPUVAddr,
			 SysPAddrToCpuPAddr(psDevInfo->asBackBuffers[ulBBuf].sSysAddr));


#endif /* #if defined(USE_BASE_VIDEO_FRAMEBUFFER) */
#endif /* #if defined(DC_NOHW_DISCONTIG_BUFFERS) */
	}

ExitCloseServices:
	(void)ClosePVRServices(psDevInfo->hPVRServices);

ExitFreeDevInfo:
	FreeKernelMem(psDevInfo);
	SetAnchorPtr(0);

ExitError:
	return eError;
}



/*
 *	Deinit
 */
DC_ERROR Deinit(void)
{
	DC_NOHW_DEVINFO *psDevInfo, *psDevFirst;
#if !defined(USE_BASE_VIDEO_FRAMEBUFFER)
	unsigned long i;
#endif

	psDevFirst = GetAnchorPtr();
	psDevInfo = psDevFirst;

	/* check DevInfo has been setup */
	if (psDevInfo == 0)
	{
		return (DC_ERROR_GENERIC);/* failure */
	}

	/* decrement ref count */
	psDevInfo->ulRefCount--;

	if (psDevInfo->ulRefCount == 0UL)
	{
		/* all references gone - de-init device information */
		PVRSRV_DC_DISP2SRV_KMJTABLE	*psJTable = &psDevInfo->sPVRJTable;

		/* Remove display class device from kernel services device register */
		if (psJTable->pfnPVRSRVRemoveDCDevice((IMG_UINT32)psDevInfo->uiDeviceID) != PVRSRV_OK)
		{
			return (DC_ERROR_GENERIC);/* failure */
		}

		if (psDevInfo->sPVRJTable.pfnPVRSRVRemoveCmdProcList(psDevInfo->uiDeviceID,
															 DC_NOHW_COMMAND_COUNT) != PVRSRV_OK)
		{
			return (DC_ERROR_GENERIC);/* failure */
		}

		if (ClosePVRServices(psDevInfo->hPVRServices) != DC_OK)
		{
			psDevInfo->hPVRServices = 0;
			return (DC_ERROR_GENERIC);/* failure */
		}

#if !defined(USE_BASE_VIDEO_FRAMEBUFFER)
		for(i=0; i<DC_NOHW_MAX_BACKBUFFERS; i++)
		{
			if (psDevInfo->asBackBuffers[i].sCPUVAddr)
			{
				#if defined(DC_NOHW_DISCONTIG_BUFFERS)
				FreeDiscontigMemory(psDevInfo->ui32BufferSize,
							 psDevInfo->asBackBuffers[i].hMemChunk,
							 psDevInfo->asBackBuffers[i].sCPUVAddr,
							 psDevInfo->asBackBuffers[i].psSysAddr);
				#else

				FreeContigMemory(psDevInfo->ui32BufferSize,
							 psDevInfo->asBackBuffers[i].hMemChunk,
							 psDevInfo->asBackBuffers[i].sCPUVAddr,
							 SysPAddrToCpuPAddr(psDevInfo->asBackBuffers[i].sSysAddr));
				#endif
			}
		}
#endif /* #if !defined(USE_BASE_VIDEO_FRAMEBUFFER) */

		/* de-allocate data structure */
		FreeKernelMem(psDevInfo);
	}

#if defined (ENABLE_DISPLAY_MODE_TRACKING)
	CloseMiniport();
#endif
	/* clear the top-level anchor */
	SetAnchorPtr(0);

	/* return success */
	return (DC_OK);
}

/******************************************************************************
 End of file (dc_nohw_displayclass.c)
******************************************************************************/
