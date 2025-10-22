/*************************************************************************/ /*!
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

/* Services headers */
#include <powervr/imgpixfmts.h>
#include <powervr/buffer_attribs.h>
#include "dc_osfuncs.h"
#include "dc_example.h"
#include "img_defs.h"
#include "kerneldisplay.h"

/*
	Enable to track contexts and buffers
*/
/* #define DCEX_DEBUG 1 */

/*
	Enable to get more debug. Only supported on UMA
*/
/* #define DCEX_VERBOSE 1*/

#if defined(DCEX_DEBUG)
	#define DCEX_DEBUG_PRINT(fmt, ...) \
		DC_OSDebugPrintf(DBGLVL_WARNING, fmt, __VA_ARGS__)
#else
	#define DCEX_DEBUG_PRINT(fmt, ...)
#endif

/*
	The number of inflight commands this display driver can handle
*/
#define MAX_COMMANDS_INFLIGHT 2

#ifdef NO_HARDWARE
#define MAX_PIPES 1
#else
#define MAX_PIPES 8
#endif

static IMG_UINT32 ui32ByteStride;
static IMG_UINT32 ePixelFormat;

static IMG_BOOL CheckBufferDimensions(void)
{
	const DC_EXAMPLE_MODULE_PARAMETERS *psModuleParams;
	IMG_UINT32 ui32BytesPP = 0;

	psModuleParams = DCExampleGetModuleParameters();
	if (NULL == psModuleParams)
	{
		DC_OSDebugPrintf(DBGLVL_ERROR, ": Cannot fetch module parameters\n");
		return IMG_FALSE;
	}

	if (psModuleParams->ui32Width == 0 ||
	    psModuleParams->ui32Height == 0 ||
	    psModuleParams->ui32Depth == 0)
	{
		DC_OSDebugPrintf(
			DBGLVL_WARNING,
			": Illegal module parameters (width %u, height %u, depth %u)\n",
			psModuleParams->ui32Width,
			psModuleParams->ui32Height,
			psModuleParams->ui32Depth);

		return IMG_FALSE;
	}

	switch (psModuleParams->ui32Depth)
	{
		case 32:
			switch (psModuleParams->ui32Format)
			{
				case 0:
					ePixelFormat = IMG_PIXFMT_B8G8R8A8_UNORM;
					break;
				case 1:
					ePixelFormat = IMG_PIXFMT_B10G10R10A2_UNORM;
					break;
				default:
					ePixelFormat = IMG_PIXFMT_UNKNOWN;
					break;
			}

			ui32BytesPP = 4;
			break;
		case 16:
			switch (psModuleParams->ui32Format)
			{
				case 0:
					ePixelFormat = IMG_PIXFMT_B5G6R5_UNORM;
					break;
				default:
					ePixelFormat = IMG_PIXFMT_UNKNOWN;
					break;
			}

			ui32BytesPP = 2;
			break;
		default:
			DC_OSDebugPrintf(DBGLVL_WARNING, ": Display depth %lu not supported\n", psModuleParams->ui32Depth);

			ePixelFormat = IMG_PIXFMT_UNKNOWN;
			break;
	}

	if (ePixelFormat == IMG_PIXFMT_UNKNOWN)
	{
		DC_OSDebugPrintf(DBGLVL_WARNING,
						 ": Display format %lu not supported for depth %lu\n",
						 psModuleParams->ui32Format, psModuleParams->ui32Depth);
		return IMG_FALSE;
	}

	if ((psModuleParams->ui32MemLayout == 1 && psModuleParams->ui32FBCFormat == 0) ||
	    (psModuleParams->ui32MemLayout != 1 && psModuleParams->ui32FBCFormat != 0))
	{
		DC_OSDebugPrintf(DBGLVL_ERROR,
				 ": Invalid memory layout/FBC mode combination (memory layout %u, FBC format %u)\n",
				 psModuleParams->ui32MemLayout,
				 psModuleParams->ui32FBCFormat);

		return IMG_FALSE;
	}

	ui32ByteStride = psModuleParams->ui32Width * ui32BytesPP;

	DC_OSDebugPrintf(DBGLVL_INFO, " Width: %u pixels\n", psModuleParams->ui32Width);
	DC_OSDebugPrintf(DBGLVL_INFO, " Height: %u pixels\n", psModuleParams->ui32Height);
	DC_OSDebugPrintf(DBGLVL_INFO, " Stride: %u bytes\n", ui32ByteStride);
	DC_OSDebugPrintf(DBGLVL_INFO, " Depth: %u bits\n", psModuleParams->ui32Depth);
	DC_OSDebugPrintf(DBGLVL_INFO, " Format: %u\n", psModuleParams->ui32Format);
	DC_OSDebugPrintf(DBGLVL_INFO, " Memory layout: %u\n", psModuleParams->ui32MemLayout);
	DC_OSDebugPrintf(DBGLVL_INFO, " FBC format: %u\n", psModuleParams->ui32FBCFormat);
	DC_OSDebugPrintf(DBGLVL_INFO, " X-Dpi: %u\n", psModuleParams->ui32XDpi);
	DC_OSDebugPrintf(DBGLVL_INFO, " Y-Dpi: %u\n", psModuleParams->ui32YDpi);

	return IMG_TRUE;
}


typedef struct _DCEX_DEVICE_
{
	IMG_HANDLE		hPVRServicesConnection;
	IMG_HANDLE		hPVRServicesDevice;
	DC_SERVICES_FUNCS	sPVRServicesFuncs;

	IMG_HANDLE		hSrvHandle;
#if defined(LMA)
	PHYS_HEAP		*psPhysHeap;
	IMG_CPU_PHYADDR		sDispStartAddr;
	IMG_UINT64		uiDispMemSize;
	IMG_UINT32		ui32BufferSize;
	IMG_UINT32		ui32BufferCount;
	IMG_UINT32		ui32BufferUseMask;
#endif
	DLLIST_NODE		sListNode;      /* List of all device nodes */
	void			*hBufListLock;  /* Lock for accessing sBufListNode */
	DLLIST_NODE		sBufListNode;   /* List of Buffers for this device */
	/* Per-device context information */
	IMG_HANDLE		hConfigData[MAX_COMMANDS_INFLIGHT];
	IMG_UINT32		ui32Head;
	IMG_UINT32		ui32Tail;
	IMG_BOOL		b_DisplayContextActive;
} DCEX_DEVICE;

typedef enum
{
	DCEX_BUFFER_SOURCE_ALLOC,
	DCEX_BUFFER_SOURCE_IMPORT,
} DCEX_BUFFER_SOURCE;

typedef struct _DCEX_BUFFER_
{
	ATOMIC_T		i32RefCount;	/* Only required for system buffer */
	IMG_BOOL		bIsSysBuffer;	/* Set for system buffer */
	IMG_UINT32		ePixFormat;
	IMG_UINT32		ui32Width;
	IMG_UINT32		ui32Height;
	IMG_UINT32		ui32ByteStride;
	IMG_UINT32		ui32Size;
	IMG_UINT32		ui32PageCount;
	IMG_HANDLE		hImport;
	IMG_DEV_PHYADDR		*pasDevPAddr;
	DCEX_DEVICE		*psDevice;
#if defined(LMA)
	IMG_UINT64		uiAllocHandle;
#else
	void			*pvAllocHandle;
#endif
	DCEX_BUFFER_SOURCE eSource;
	DLLIST_NODE		sListNode;     /* List of all buffers one per device */
} DCEX_BUFFER;

static void *g_hDevDataListLock;

static DLLIST_NODE g_sDeviceDataListHead;

static INLINE void DCExampleConfigPush(DCEX_DEVICE *psDevice,
                                       IMG_HANDLE hConfigData)
{
	IMG_UINT32 ui32Head;

	DC_ASSERT(psDevice != NULL);
	ui32Head = psDevice->ui32Head;
	psDevice->hConfigData[ui32Head] = hConfigData;
	ui32Head++;

	if (ui32Head >= MAX_COMMANDS_INFLIGHT)
	{
		ui32Head = 0;
	}

	psDevice->ui32Head = ui32Head;
}

static INLINE IMG_HANDLE DCExampleConfigPop(DCEX_DEVICE *psDevice)
{
	IMG_UINT32 ui32Tail;
	IMG_HANDLE hConfigData;

	DC_ASSERT(psDevice != NULL);
	ui32Tail = psDevice->ui32Tail;
	hConfigData = psDevice->hConfigData[ui32Tail];
	ui32Tail++;

	if (ui32Tail >= MAX_COMMANDS_INFLIGHT)
	{
		ui32Tail = 0;
	}
	psDevice->ui32Tail = ui32Tail;

	return hConfigData;
}

static INLINE IMG_BOOL DCExampleConfigIsEmpty(DCEX_DEVICE *psDevice)
{
	DC_ASSERT(psDevice != NULL);
	return ((psDevice->ui32Tail == psDevice->ui32Head) ? IMG_TRUE : IMG_FALSE);
}

#if defined(LMA)
/*
	Simple unit size allocator
*/
static
IMG_UINT64 _DCExampleAllocLMABuffer(DCEX_DEVICE *psDevice)
{
	IMG_UINT32 i;
	IMG_UINT64 pvRet = 0;

	for (i = 0; i < psDevice->ui32BufferCount; i++)
	{
		if ((psDevice->ui32BufferUseMask & (1UL << i)) == 0)
		{
			pvRet = psDevice->sDispStartAddr.uiAddr +
					(i * psDevice->ui32BufferSize);
			psDevice->ui32BufferUseMask |= (1UL << i);
			break;
		}
	}

	return pvRet;
}

static
void _DCExampleFreeLMABuffer(DCEX_DEVICE *psDevice, IMG_UINT64 uiAddr)
{
	IMG_UINT64 ui64Offset;

	DC_ASSERT(uiAddr >= psDevice->sDispStartAddr.uiAddr);

	DCEX_DEBUG_PRINT("Freeing %p, Size %x, UseMask %x", (void *)uiAddr,
	                  psDevice->ui32BufferSize, psDevice->ui32BufferUseMask);

	ui64Offset = uiAddr - psDevice->sDispStartAddr.uiAddr;
	ui64Offset = DC_OSDiv64(ui64Offset, psDevice->ui32BufferSize);
	DC_ASSERT(ui64Offset <= psDevice->ui32BufferCount);
	psDevice->ui32BufferUseMask &= ~(1UL << ui64Offset);
}
#endif /* LMA */

static
void DCExampleGetInfo(IMG_HANDLE hDeviceData,
						  DC_DISPLAY_INFO *psDisplayInfo)
{
	PVR_UNREFERENCED_PARAMETER(hDeviceData);

	/*
		Copy our device name
	*/
	DC_OSStringNCopy(psDisplayInfo->szDisplayName, DRVNAME " 1", DC_NAME_SIZE);

	/*
		Report what our minimum and maximum display period is.
	*/
	psDisplayInfo->ui32MinDisplayPeriod	= 0;
	psDisplayInfo->ui32MaxDisplayPeriod	= 1;
	psDisplayInfo->ui32MaxPipes			= MAX_PIPES;
	psDisplayInfo->bUnlatchedSupported	= IMG_FALSE;
}

static
PVRSRV_ERROR DCExamplePanelQueryCount(IMG_HANDLE hDeviceData,
										 IMG_UINT32 *pui32NumPanels)
{
	PVR_UNREFERENCED_PARAMETER(hDeviceData);
	/*
		If you know the panel count at compile time just hardcode it, if it's
		dynamic you should probe it here
	*/
	*pui32NumPanels = 1;

	return PVRSRV_OK;
}

static
PVRSRV_ERROR DCExamplePanelQuery(IMG_HANDLE hDeviceData,
									IMG_UINT32 ui32PanelsArraySize,
									IMG_UINT32 *pui32NumPanels,
									PVRSRV_PANEL_INFO *psPanelInfo)
{
	const DC_EXAMPLE_MODULE_PARAMETERS *psModuleParams = DCExampleGetModuleParameters();

	PVR_UNREFERENCED_PARAMETER(hDeviceData);

	if (NULL == psModuleParams)
	{
		DC_OSDebugPrintf(DBGLVL_ERROR, ": Cannot fetch module parameters\n");
		return PVRSRV_ERROR_NO_DEVICEDATA_FOUND;
	}

	/*
		If we have hotplug displays then there is a chance a display could
		have been removed so return the number of panels we have queried
	*/
	*pui32NumPanels = 1;

	/*
		Either hard code the values here or probe each panel here. If a new
		panel has been hotplugged then ignore it as we've not been given
		room to store its data
	*/
	psPanelInfo[0].sSurfaceInfo.sFormat.ePixFormat = ePixelFormat;
	psPanelInfo[0].sSurfaceInfo.sDims.ui32Width = psModuleParams->ui32Width;
	psPanelInfo[0].sSurfaceInfo.sDims.ui32Height = psModuleParams->ui32Height;

	psPanelInfo[0].ui32RefreshRate = psModuleParams->ui32RefreshRate;
	psPanelInfo[0].ui32XDpi = psModuleParams->ui32XDpi;
	psPanelInfo[0].ui32YDpi = psModuleParams->ui32YDpi;

	switch (psModuleParams->ui32MemLayout)
	{
		case 0:
			psPanelInfo[0].sSurfaceInfo.sFormat.eMemLayout = PVRSRV_SURFACE_MEMLAYOUT_STRIDED;
			break;
		case 1:
			psPanelInfo[0].sSurfaceInfo.sFormat.eMemLayout = PVRSRV_SURFACE_MEMLAYOUT_FBC;
			break;
		default:
			DC_OSDebugPrintf(DBGLVL_ERROR, ": Unknown memory layout %ld\n", psModuleParams->ui32MemLayout);

			return PVRSRV_ERROR_INVALID_PARAMS;
	}

	switch (psModuleParams->ui32FBCFormat)
	{
		case 0:
			psPanelInfo[0].sSurfaceInfo.sFormat.u.sFBCLayout.eFBCompressionMode = IMG_FB_COMPRESSION_NONE;
			break;
		case 1:
			psPanelInfo[0].sSurfaceInfo.sFormat.u.sFBCLayout.eFBCompressionMode = IMG_FB_COMPRESSION_DIRECT_8x8;
			break;
		case 2:
			psPanelInfo[0].sSurfaceInfo.sFormat.u.sFBCLayout.eFBCompressionMode = IMG_FB_COMPRESSION_DIRECT_16x4;
			break;
		case 5:
			psPanelInfo[0].sSurfaceInfo.sFormat.u.sFBCLayout.eFBCompressionMode = IMG_FB_COMPRESSION_DIRECT_32x2;
			break;
		case 6:
			psPanelInfo[0].sSurfaceInfo.sFormat.u.sFBCLayout.eFBCompressionMode = IMG_FB_COMPRESSION_DIRECT_LOSSY50_8x8;
			break;
		case 7:
			psPanelInfo[0].sSurfaceInfo.sFormat.u.sFBCLayout.eFBCompressionMode = IMG_FB_COMPRESSION_DIRECT_LOSSY50_16x4;
			break;
		case 8:
			psPanelInfo[0].sSurfaceInfo.sFormat.u.sFBCLayout.eFBCompressionMode = IMG_FB_COMPRESSION_DIRECT_LOSSY50_32x2;
			break;
		case 9:
			psPanelInfo[0].sSurfaceInfo.sFormat.u.sFBCLayout.eFBCompressionMode = IMG_FB_COMPRESSION_DIRECT_LOSSY25_8x8;
			break;
		case 10:
			psPanelInfo[0].sSurfaceInfo.sFormat.u.sFBCLayout.eFBCompressionMode = IMG_FB_COMPRESSION_DIRECT_LOSSY25_16x4;
			break;
		case 11:
			psPanelInfo[0].sSurfaceInfo.sFormat.u.sFBCLayout.eFBCompressionMode = IMG_FB_COMPRESSION_DIRECT_LOSSY25_32x2;
			break;
		case 12:
			psPanelInfo[0].sSurfaceInfo.sFormat.u.sFBCLayout.eFBCompressionMode = IMG_FB_COMPRESSION_DIRECT_LOSSY75_8x8;
			break;
		case 13:
			psPanelInfo[0].sSurfaceInfo.sFormat.u.sFBCLayout.eFBCompressionMode = IMG_FB_COMPRESSION_DIRECT_LOSSY75_16x4;
			break;
		case 14:
			psPanelInfo[0].sSurfaceInfo.sFormat.u.sFBCLayout.eFBCompressionMode = IMG_FB_COMPRESSION_DIRECT_LOSSY75_32x2;
			break;
		case 15:
			psPanelInfo[0].sSurfaceInfo.sFormat.u.sFBCLayout.eFBCompressionMode = IMG_FB_COMPRESSION_DIRECT_LOSSY37_8x8;
			break;
		case 16:
			psPanelInfo[0].sSurfaceInfo.sFormat.u.sFBCLayout.eFBCompressionMode = IMG_FB_COMPRESSION_DIRECT_LOSSY37_16x4;
			break;
		case 17:
			psPanelInfo[0].sSurfaceInfo.sFormat.u.sFBCLayout.eFBCompressionMode = IMG_FB_COMPRESSION_DIRECT_LOSSY37_32x2;
			break;
		default:
			DC_OSDebugPrintf(DBGLVL_ERROR, ": Unknown FBC format %ld\n", psModuleParams->ui32FBCFormat);

			return PVRSRV_ERROR_INVALID_PARAMS;
	}

	DC_ASSERT((psPanelInfo[0].sSurfaceInfo.sFormat.eMemLayout == PVRSRV_SURFACE_MEMLAYOUT_FBC &&
		   psPanelInfo[0].sSurfaceInfo.sFormat.u.sFBCLayout.eFBCompressionMode != IMG_FB_COMPRESSION_NONE) ||
		  (psPanelInfo[0].sSurfaceInfo.sFormat.eMemLayout != PVRSRV_SURFACE_MEMLAYOUT_FBC &&
		   psPanelInfo[0].sSurfaceInfo.sFormat.u.sFBCLayout.eFBCompressionMode == IMG_FB_COMPRESSION_NONE));

	return PVRSRV_OK;
}

static
PVRSRV_ERROR DCExampleFormatQuery(IMG_HANDLE hDeviceData,
									IMG_UINT32 ui32NumFormats,
									PVRSRV_SURFACE_FORMAT *pasFormat,
									IMG_UINT32 *pui32Supported)
{
	IMG_UINT32 i;

	PVR_UNREFERENCED_PARAMETER(hDeviceData);

	for (i = 0; i < ui32NumFormats; i++)
	{
		pui32Supported[i] = 0;

		/*
			If the display controller has multiple display pipes (DMA engines)
			each one should be checked to see if it supports the specified
			format.
		*/
		if ((pasFormat[i].ePixFormat == IMG_PIXFMT_B10G10R10A2_UNORM) ||
		    (pasFormat[i].ePixFormat == IMG_PIXFMT_B8G8R8A8_UNORM) ||
		    (pasFormat[i].ePixFormat == IMG_PIXFMT_B5G6R5_UNORM))
		{
			pui32Supported[i]++;
		}
	}

	return PVRSRV_OK;
}

static
PVRSRV_ERROR DCExampleDimQuery(IMG_HANDLE hDeviceData,
								 IMG_UINT32 ui32NumDims,
								 PVRSRV_SURFACE_DIMS *psDim,
								 IMG_UINT32 *pui32Supported)
{
	IMG_UINT32 i;
	const DC_EXAMPLE_MODULE_PARAMETERS *psModuleParams = DCExampleGetModuleParameters();

	PVR_UNREFERENCED_PARAMETER(hDeviceData);

	if (NULL == psModuleParams)
	{
		DC_OSDebugPrintf(DBGLVL_ERROR, ": Cannot fetch module parameters\n");
		return PVRSRV_ERROR_NO_DEVICEDATA_FOUND;
	}

	for (i = 0; i < ui32NumDims; i++)
	{
		pui32Supported[i] = 0;

		/*
			If the display controller has multiple display pipes (DMA engines)
			each one should be checked to see if it supports the specified
			dimension.
		*/
		if ((psDim[i].ui32Width == psModuleParams->ui32Width)
		&&  (psDim[i].ui32Height == psModuleParams->ui32Height))
		{
			pui32Supported[i]++;
		}
	}

	return PVRSRV_OK;
}

static
PVRSRV_ERROR DCExampleBufferSystemAcquire(IMG_HANDLE hDeviceData,
											IMG_DEVMEM_LOG2ALIGN_T *puiLog2PageSize,
											IMG_UINT32 *pui32PageCount,
											IMG_UINT32 *pui32ByteStride,
											IMG_HANDLE *phSystemBuffer)
{
	DCEX_BUFFER *psBuffer;
	DCEX_DEVICE *psDevice = (DCEX_DEVICE *)hDeviceData;
	DLLIST_NODE *psNode, *psNext;

	/*
		This function is optional. It provides a method for services
		to acquire a display buffer which it didn't setup but was created
		by the OS (e.g. Linux frame buffer).
		If the OS should trigger a mode change then it's not allowed to free
		the previous buffer until services has released it via BufferSystemRelease
	*/

	/*
		Protect the access to the system buffer list
	 */
	DC_OSMutexLock(psDevice->hBufListLock);

	/*
		Take a reference to the system buffer 1st to make sure it isn't freed
	*/
	dllist_foreach_node(&psDevice->sBufListNode, psNode, psNext)
	{
		psBuffer = IMG_CONTAINER_OF(psNode, DCEX_BUFFER, sListNode);

		if ((psBuffer->psDevice == psDevice) && psBuffer->bIsSysBuffer)
		{

			DCEX_DEBUG_PRINT("%s: psBuffer %p Ref Count %d\n", __func__,
			                 psBuffer, DC_OSAtomicRead(&psBuffer->i32RefCount));

			DC_OSAtomicIncrement(&psBuffer->i32RefCount);

			*puiLog2PageSize = DC_OSGetPageShift();
			*pui32PageCount = psBuffer->ui32Size >> DC_OSGetPageShift();
			*pui32ByteStride = psBuffer->ui32ByteStride;
			*phSystemBuffer = psBuffer;

			DC_OSMutexUnlock(psDevice->hBufListLock);
			return PVRSRV_OK;
		}
	}

	DC_OSMutexUnlock(psDevice->hBufListLock);
	return PVRSRV_ERROR_INVALID_PARAMS;
}

static
void DCExampleBufferSystemRelease(IMG_HANDLE hSystemBuffer)
{
	DCEX_BUFFER *psBuffer = hSystemBuffer;

	/*
		Acquire the buffer mutex to prevent alteration while we are
		releasing the system buffer.
	*/
	DC_OSMutexLock(psBuffer->psDevice->hBufListLock);

	if (!psBuffer->bIsSysBuffer)
	{
		DC_OSDebugPrintf(DBGLVL_ERROR, "%s: Unexpected non system-buffer found\n",
		                __func__);
		DC_OSMutexUnlock(psBuffer->psDevice->hBufListLock);
		return;
	}

	DC_OSAtomicDecrement(&psBuffer->i32RefCount);

	/*
		If the system buffer has changed and we've just dropped the last
		refcount then free the buffer
	*/
	if (!dllist_node_is_in_list(&psBuffer->sListNode) && (DC_OSAtomicRead(&psBuffer->i32RefCount) == 0))
	{
		/* Free the buffer and it's memory (if the memory was allocated) */
		DCEX_DEBUG_PRINT("%s: Dropping %p", __func__, psBuffer);
	}

	/*
		Release buffer mutex now that we've released it.
	*/
	DC_OSMutexUnlock(psBuffer->psDevice->hBufListLock);
}

static
PVRSRV_ERROR DCExampleContextCreate(IMG_HANDLE hDeviceData,
									  IMG_HANDLE *hDisplayContext)
{
	DCEX_DEVICE *psDevice = hDeviceData;

	if (psDevice == NULL)
	{
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	/*
		The creation of a display context is a software concept and
		it's an "agreement" between the services client and the DC driver
		as to what this means (if anything)
	*/
	if (psDevice->b_DisplayContextActive != IMG_TRUE)
	{
		*hDisplayContext = hDeviceData;

		psDevice->b_DisplayContextActive = IMG_TRUE;
		DCEX_DEBUG_PRINT("Create context (%p)\n", *hDisplayContext);
		return PVRSRV_OK;
	}

	return PVRSRV_ERROR_RESOURCE_UNAVAILABLE;
}

static
PVRSRV_ERROR DCExampleContextConfigureCheck(IMG_HANDLE hDisplayContext,
											  IMG_UINT32 ui32PipeCount,
											  PVRSRV_SURFACE_CONFIG_INFO *pasSurfAttrib,
											  IMG_HANDLE *ahBuffers)
{
	PVRSRV_ERROR eError = PVRSRV_OK;
	IMG_UINT32 i;
	const DC_EXAMPLE_MODULE_PARAMETERS *psModuleParams = DCExampleGetModuleParameters();

	PVR_UNREFERENCED_PARAMETER(hDisplayContext);

	if (NULL == psModuleParams)
	{
		DC_OSDebugPrintf(DBGLVL_ERROR, ": Cannot fetch module parameters\n");
		return PVRSRV_ERROR_NO_DEVICEDATA_FOUND;
	}

	/*
		The display engine might have a limit on the number of discretely
		configurable pipes. In such a case an error should be returned.
	*/
	if (ui32PipeCount > MAX_PIPES)
	{
		eError = PVRSRV_ERROR_DC_TOO_MANY_PIPES;
		goto fail_max_pipes;
	}

	/*
		This optional function allows the display driver to check if the display
		configuration passed in is valid.
		It's possible that due to HW constraints that although the client has
		honoured the DimQuery and FormatQuery results the configuration it
		has requested is still not possible (e.g. there isn't enough space in
		the display controller's MMU, or due restrictions on display pipes.
	*/

	for (i = 0; i < ui32PipeCount; i++)
	{
		DCEX_BUFFER *psBuffer = ahBuffers[i];

		if (pasSurfAttrib[i].sCrop.sDims.ui32Width  != psModuleParams->ui32Width  ||
		    pasSurfAttrib[i].sCrop.sDims.ui32Height != psModuleParams->ui32Height ||
		    pasSurfAttrib[i].sCrop.i32XOffset != 0 ||
		    pasSurfAttrib[i].sCrop.i32YOffset != 0)
		{
			eError = PVRSRV_ERROR_DC_INVALID_CROP_RECT;
			break;
		}

		if (pasSurfAttrib[i].sDisplay.sDims.ui32Width !=
			pasSurfAttrib[i].sCrop.sDims.ui32Width ||
			pasSurfAttrib[i].sDisplay.sDims.ui32Height !=
			pasSurfAttrib[i].sCrop.sDims.ui32Height ||
			pasSurfAttrib[i].sDisplay.i32XOffset !=
			pasSurfAttrib[i].sCrop.i32XOffset ||
			pasSurfAttrib[i].sDisplay.i32YOffset !=
			pasSurfAttrib[i].sCrop.i32YOffset)
		{
			eError = PVRSRV_ERROR_DC_INVALID_DISPLAY_RECT;
			break;
		}

		if (psBuffer->ui32Width != psModuleParams->ui32Width
		||  psBuffer->ui32Height != psModuleParams->ui32Height)
		{
			eError = PVRSRV_ERROR_DC_INVALID_BUFFER_DIMS;
			break;
		}
	}

fail_max_pipes:
	return eError;
}

static
void DCExampleContextConfigure(IMG_HANDLE hDisplayContext,
									 IMG_UINT32 ui32PipeCount,
									 PVRSRV_SURFACE_CONFIG_INFO *pasSurfAttrib,
									 IMG_HANDLE *ahBuffers,
									 IMG_UINT32 ui32DisplayPeriod,
									 IMG_HANDLE hConfigData)
{
	DCEX_DEVICE *psDeviceData = hDisplayContext;
	IMG_UINT32 i;

	/*
		As we have no HW and thus no VSync IRQ we just activate the
		new config here
	*/
	for (i = 0; i < ui32PipeCount; i++)
	{
#if defined(DCEX_DEBUG)
		DCEX_BUFFER *psBuffer = ahBuffers[i];
		/*
			There is no checking to be done here as we can't fail,
			any checking should have been DCExampleContextConfigureCheck.
		*/

		/*
			If the display controller supports scaling it should set it up
			here.
		*/

		/*
			Setup the DMA from the display buffer
		*/

		/*
			Save the config data as we need to pass it back once this
			configuration gets retired
		*/
		DCEX_DEBUG_PRINT("Display buffer (%p), Ref Count %d\n", psBuffer,
		                 DC_OSAtomicRead(&psBuffer->i32RefCount));
#endif /* DCEX_DEBUG */
	}

	/*
		As we have no HW and thus no VSync IRQ we just retire the
		previous config as soon as we get a new one
	*/
	if (DCExampleConfigIsEmpty(psDeviceData) == IMG_FALSE)
	{
		/* Retire the current config */
		psDeviceData->sPVRServicesFuncs.pfnDCDisplayConfigurationRetired(DCExampleConfigPop(psDeviceData));
	}

	if (ui32PipeCount != 0)
	{
		/* Save our new config data */
		DCExampleConfigPush(psDeviceData, hConfigData);
	}
	else
	{
		/*
			When the client requests the display context to be destroyed
			services will issue a "NULL" flip to us so we can retire
			the current configuration.

			We need to pop the current (and last) configuration off the our
			stack and retire it which we're already done above as it's our
			default behaviour to retire the previous config immediately when we
			get a new one. We also need to "retire" the NULL flip so the DC core
			knows that we've done whatever we need to do so it can start to
			tear down the display context

			In real devices we could have a number of configurations in flight,
			all these configurations _and the NULL flip_ need to be processed
			in order with the NULL flip being completed when it's active rather
			then when it will be retired (as it will never be retired)

			At this point there is nothing that the display is being asked to
			display by services and it's the DC driver implementation decision
			as to what it should then do. Typically, for systems that have a
			system surface the DC would switch back to displaying that.
		*/
		DCEX_DEBUG_PRINT("Display flushed (%p)\n", hDisplayContext);

		psDeviceData->sPVRServicesFuncs.pfnDCDisplayConfigurationRetired(hConfigData);
	}
}

static
void DCExampleContextDestroy(IMG_HANDLE hDisplayContext)
{
	DCEX_DEVICE *psDevice = hDisplayContext;

	DC_ASSERT(DCExampleConfigIsEmpty(psDevice) == IMG_TRUE);

	/*
		Counterpart to ContextCreate. Any buffers created/imported on
		this display context will have been freed before this call so
		all the display driver needs to do is release any resources
		allocated at ContextCreate time.
	*/
	psDevice->b_DisplayContextActive = IMG_FALSE;
	DCEX_DEBUG_PRINT("Destroy display context (%p)\n", hDisplayContext);
}

static
PVRSRV_ERROR DCExampleBufferAlloc(IMG_HANDLE hDisplayContext,
									DC_BUFFER_CREATE_INFO *psCreateInfo,
									IMG_DEVMEM_LOG2ALIGN_T *puiLog2PageSize,
									IMG_UINT32 *pui32PageCount,
									IMG_UINT32 *pui32ByteStride,
									IMG_HANDLE *phBuffer)
{
	DCEX_BUFFER *psBuffer;
	PVRSRV_SURFACE_INFO *psSurfInfo = &psCreateInfo->sSurface;
	PVRSRV_ERROR eError;
	DCEX_DEVICE *psDevice = hDisplayContext;

	/*
		Allocate the buffer control structure
	*/

	psBuffer = DC_OSCallocMem(sizeof(DCEX_BUFFER));

	if (psBuffer == NULL)
	{
		eError = PVRSRV_ERROR_OUT_OF_MEMORY;
		goto fail_bufferalloc;
	}

	dllist_init(&psBuffer->sListNode);

	/* Common setup */
	psBuffer->eSource = DCEX_BUFFER_SOURCE_ALLOC;
	DC_OSAtomicWrite(&psBuffer->i32RefCount, 1);
	psBuffer->ePixFormat = psSurfInfo->sFormat.ePixFormat;
	psBuffer->bIsSysBuffer = IMG_FALSE;

	/*
		Store the display buffer size, that is the pixel width and height that
		will be displayed which might be less than the pixel width and height
		of the buffer
	*/
	psBuffer->ui32Width = psSurfInfo->sDims.ui32Width;
	psBuffer->ui32Height = psSurfInfo->sDims.ui32Height;

	switch (psSurfInfo->sFormat.eMemLayout)
	{
		case PVRSRV_SURFACE_MEMLAYOUT_STRIDED:
			/*
				As we're been asked to allocate this buffer we decide what it's
				stride should be.
			*/
			psBuffer->ui32ByteStride = psSurfInfo->sDims.ui32Width * psCreateInfo->ui32BPP;
			psBuffer->ui32Size = psBuffer->ui32Height * psBuffer->ui32ByteStride;
			break;
		case PVRSRV_SURFACE_MEMLAYOUT_FBC:
			psBuffer->ui32ByteStride = psCreateInfo->sFBC.ui32FBCStride * psCreateInfo->ui32BPP;
			psBuffer->ui32Size = psCreateInfo->sFBC.ui32Size;

			/*
				Here we should program the FBC registers in the display
				controller according to the information we have in
				psSurfInfo->sFBC
			*/
			break;

		default:
			eError = PVRSRV_ERROR_NOT_SUPPORTED;
			goto fail_memlayout;
			break;
	}

	psBuffer->psDevice = psDevice;

#if defined(LMA)
	/*
		Check that the buffer size calculated is compatible with what was allocated during initialisation.
	*/
	if (psBuffer->ui32Size > psDevice->ui32BufferSize)
	{
		DC_OSDebugPrintf(DBGLVL_ERROR, "%s: Buffer size of 0x%0x is too large for allocation of 0x%0x!\n",
		                 __func__, psBuffer->ui32Size, psDevice->ui32BufferSize);
		eError = PVRSRV_ERROR_OUT_OF_MEMORY;
		goto fail_buffersize;
	}
	else if (psBuffer->ui32Size != psDevice->ui32BufferSize)
	{
		DCEX_DEBUG_PRINT("%s: Buffer size of 0x%0x is not same as allocated 0x%0x\n",
		                 __func__, psBuffer->ui32Size, psDevice->ui32BufferSize);
		psBuffer->ui32Size = psDevice->ui32BufferSize;
	}

	/*
		Allocate display addressable memory. We only need physical addresses
		at this stage.

		Note: This could be deferred until the 1st map or acquire call.
	*/
	psBuffer->uiAllocHandle = _DCExampleAllocLMABuffer(psDevice);
	if (psBuffer->uiAllocHandle == 0)
	{
		eError = PVRSRV_ERROR_OUT_OF_MEMORY;
		goto fail_buffermemalloc;
	}
#else
	psBuffer->pvAllocHandle = DCExampleVirtualAllocUncached(psBuffer->ui32Size);
	if (psBuffer->pvAllocHandle == NULL)
	{
		eError = PVRSRV_ERROR_OUT_OF_MEMORY;
		goto fail_buffermemalloc;
	}
#endif
	*pui32ByteStride = psBuffer->ui32ByteStride;
	*puiLog2PageSize = DC_OSGetPageShift();
	*pui32PageCount = DC_OS_BYTES_TO_PAGES(psBuffer->ui32Size);
	*phBuffer = psBuffer;

	DCEX_DEBUG_PRINT("Allocate buffer (%p)\n", psBuffer);

	DC_OSMutexLock(psDevice->hBufListLock);
	dllist_add_to_tail(&psDevice->sBufListNode, &psBuffer->sListNode);
	DC_OSMutexUnlock(psDevice->hBufListLock);

	return PVRSRV_OK;

#if defined(LMA)
fail_buffersize:
#endif
fail_memlayout:
fail_buffermemalloc:
	DC_OSFreeMem(psBuffer);

fail_bufferalloc:
	return eError;
}

#if !defined(LMA)
static
PVRSRV_ERROR DCExampleBufferImport(IMG_HANDLE hDisplayContext,
									 IMG_UINT32 ui32NumPlanes,
									 IMG_HANDLE **paphImport,
									 DC_BUFFER_IMPORT_INFO *psSurfAttrib,
									 IMG_HANDLE *phBuffer)
{
	/*
		This it optional and should only be provided if the display controller
		can access "general" memory (e.g. the memory doesn't have to contiguous)
	*/
	const DC_EXAMPLE_MODULE_PARAMETERS *psModuleParams = DCExampleGetModuleParameters();
	DCEX_BUFFER *psBuffer;
	DCEX_DEVICE *psDevice = hDisplayContext;

	if (NULL == psModuleParams)
	{
		DC_OSDebugPrintf(DBGLVL_ERROR, ": Cannot fetch module parameters\n");
		return PVRSRV_ERROR_NO_DEVICEDATA_FOUND;
	}

	/*
		Check to see if our display hardware supports this buffer
	*/
	if ((psSurfAttrib->ePixFormat != IMG_PIXFMT_B8G8R8A8_UNORM) ||
		(psSurfAttrib->ui32Width[0] != psModuleParams->ui32Width) ||
		(psSurfAttrib->ui32Height[0] != psModuleParams->ui32Height) ||
		(psSurfAttrib->ui32ByteStride[0] != ui32ByteStride))
	{
		return PVRSRV_ERROR_UNSUPPORTED_PIXEL_FORMAT;
	}

	psBuffer = DC_OSCallocMem(sizeof(DCEX_BUFFER));

	if (psBuffer == NULL)
	{
		return PVRSRV_ERROR_OUT_OF_MEMORY;
	}

	psBuffer->eSource = DCEX_BUFFER_SOURCE_IMPORT;
	psBuffer->ui32Width = psSurfAttrib->ui32Width[0];
	DC_OSAtomicWrite(&psBuffer->i32RefCount, 1);
	psBuffer->ePixFormat = psSurfAttrib->ePixFormat;
	psBuffer->ui32ByteStride = psSurfAttrib->ui32ByteStride[0];
	psBuffer->ui32Width = psSurfAttrib->ui32Width[0];
	psBuffer->ui32Height = psSurfAttrib->ui32Height[0];
	psBuffer->psDevice = psDevice;

	/*
		If the display controller supports mapping "general" memory, but has
		limitations (e.g. if it doesn't have full range addressing) these
		should be checked here by calling DCImportBufferAcquire. In this case
		it lock down the physical address of the buffer at this strange rather
		then being able to defer it to map time.
	*/
	psBuffer->hImport = paphImport[0];

	*phBuffer = psBuffer;
	DCEX_DEBUG_PRINT("Import buffer (%p)\n", psBuffer);
	return PVRSRV_OK;
}
#endif

static
PVRSRV_ERROR DCExampleBufferAcquire(IMG_HANDLE hBuffer,
									IMG_DEV_PHYADDR *pasDevPAddr,
									void **ppvLinAddr)
{
	DCEX_BUFFER *psBuffer = hBuffer;
	PVRSRV_ERROR eError;
#if defined(LMA)
	IMG_UINT32 i;
	unsigned long ulPages = DC_OS_BYTES_TO_PAGES(psBuffer->ui32Size);
	PHYS_HEAP *psPhysHeap = psBuffer->psDevice->psPhysHeap;
	IMG_CPU_PHYADDR sCpuPAddr;
#endif
	/*
		If we didn't allocate the display memory at buffer alloc time
		we would have to do it here.
	*/

	/*
		Fill in the array of addresses we were passed
	*/
#if defined(LMA)
	sCpuPAddr.uiAddr = psBuffer->uiAllocHandle;
	DCEX_DEBUG_PRINT("Acquire buffer (%p) memory, npages = %d, Start = %p, size = 0x%x\n",
	                 psBuffer, ulPages, sCpuPAddr.uiAddr, psBuffer->ui32Size);
	for (i = 0; i < ulPages; i++)
	{
		psBuffer->psDevice->sPVRServicesFuncs.pfnPhysHeapCpuPAddrToDevPAddr(
				psPhysHeap,
				1,
				&pasDevPAddr[i],
				&sCpuPAddr);

		/* Only display first and last mappings */
		if ((i == 0) || i == (ulPages - 1))
		{
			DCEX_DEBUG_PRINT("Acquire buffer (%p) page %d, DevPAddr %p, CPUPAddr %p\n",
			                 psBuffer, i, pasDevPAddr[i].uiAddr, sCpuPAddr.uiAddr);
		}
		sCpuPAddr.uiAddr += DC_OSGetPageSize();
	}
	*ppvLinAddr = NULL;
	eError      = PVRSRV_OK;
#else
	eError = DCExampleGetDevPAddrs(psBuffer->pvAllocHandle, pasDevPAddr, 0, psBuffer->ui32Size);
	if (eError == PVRSRV_OK)
	{
		eError = DCExampleGetLinAddr(psBuffer->pvAllocHandle, ppvLinAddr);
	}
#endif

	DCEX_DEBUG_PRINT("Acquire buffer (%p) memory\n", psBuffer);
	return eError;
}

static
void DCExampleBufferRelease(IMG_HANDLE hBuffer)
{
#if defined(DCEX_DEBUG)
	DCEX_BUFFER *psBuffer = hBuffer;
#endif
	/*
		We could release the display memory here (assuming it wasn't
		still mapped into the display controller).

		As the buffer hasn't been freed the contents must be preserved, i.e.
		in the next call to Acquire different physical pages can be returned,
		but they must have the same contents as the old pages had at Release
		time.
	*/
	DCEX_DEBUG_PRINT("Release buffer (%p) memory, Ref Count = %d\n",
	                 psBuffer, DC_OSAtomicRead(&psBuffer->i32RefCount));
}

static
void DCExampleBufferFree(IMG_HANDLE hBuffer)
{
	DCEX_BUFFER *psBuffer = hBuffer;

	DCEX_DEBUG_PRINT("Free buffer (%p)\n", psBuffer);
	if (psBuffer->eSource == DCEX_BUFFER_SOURCE_ALLOC)
	{
#if defined(LMA)
		DCEX_DEBUG_PRINT("Free buffer memory (%llu/[0x%llx])\n",
		                 psBuffer->uiAllocHandle, psBuffer->uiAllocHandle);
		_DCExampleFreeLMABuffer(psBuffer->psDevice, psBuffer->uiAllocHandle);
#else
		DCEX_DEBUG_PRINT("Free buffer memory (%p)\n", psBuffer->pvAllocHandle);
		DCExampleVirtualFree(psBuffer->pvAllocHandle);
#endif
	}

	DC_OSMutexLock(psBuffer->psDevice->hBufListLock);
	dllist_remove_node(&psBuffer->sListNode);
	DC_OSMutexUnlock(psBuffer->psDevice->hBufListLock);
	DC_OSFreeMem(psBuffer);
}

static
PVRSRV_ERROR DCExampleBufferMap(IMG_HANDLE hBuffer)
{
	DCEX_BUFFER *psBuffer = hBuffer;
	IMG_UINT32 ui32PageCount;
	PVRSRV_ERROR eError;
#if defined(DCEX_VERBOSE)
	IMG_UINT32 i;
#endif
	/*
		If the display controller needs memory to be mapped into it
		(e.g. it has an MMU) and didn't do it in the alloc and import then it
		should provide this function.
	*/

	if (psBuffer->hImport)
	{
		IMG_DEV_PHYADDR *pasDevPAddr;
		/*
			In the case of an import buffer we didn't allocate the buffer and
			so need to ask for it's pages
		*/
		eError = psBuffer->psDevice->sPVRServicesFuncs.pfnDCImportBufferAcquire(
					psBuffer->hImport,
					DC_OSGetPageShift(),
					&ui32PageCount,
					&pasDevPAddr);

		if (eError != PVRSRV_OK)
		{
			goto fail_import;
		}
#if defined(DCEX_VERBOSE)
		for (i = 0; i < ui32PageCount; i++)
		{
			DCEX_DEBUG_PRINT(": DCExampleBufferMap: DCExample map address 0x%016llx\n", pasDevPAddr[i].uiAddr);
		}
#endif
		psBuffer->pasDevPAddr = pasDevPAddr;
		psBuffer->ui32PageCount = ui32PageCount;
	}
#if defined(DCEX_VERBOSE)
	else
	{
		unsigned long ulPages = DC_OS_BYTES_TO_PAGES(psBuffer->ui32Size);
		IMG_DEV_PHYADDR sDevPAddr;

		for (i = 0; i < ulPages; i++)
		{
			eError = DCExampleGetDevPAddrs(psBuffer->pvAllocHandle, &sDevPAddr, i, DC_OSGetPageSize());
			if (PVRSRV_OK == eError)
			{
				DC_OSDebugPrintf(DBGLVL_INFO, ": DCExampleBufferMap: DCExample map address 0x%016llx\n", sDevPAddr.uiAddr);
			}
		}
	}
#endif
	DCEX_DEBUG_PRINT("Map buffer (%p) into display\n", psBuffer);
	return PVRSRV_OK;

fail_import:
	return eError;
}

static
void DCExampleBufferUnmap(IMG_HANDLE hBuffer)
{
	DCEX_BUFFER *psBuffer = hBuffer;
#if defined(DCEX_VERBOSE)
	IMG_UINT32 i;
#endif
	/*
		If the display controller provided buffer map then it must provide
		this function
	*/

	/*
		Unmap the memory from the display controller's MMU
	*/
	if (psBuffer->hImport)
	{
#if defined(DCEX_VERBOSE)
		for (i = 0; i < psBuffer->ui32PageCount; i++)
		{
			DC_OSDebugPrintf(DBGLVL_INFO, ": DCExampleBufferUnmap: DCExample unmap address 0x%016llx\n", psBuffer->pasDevPAddr[i].uiAddr);
		}
#endif
		/*
			As this was an imported buffer we need to release it
		*/
		psBuffer->psDevice->sPVRServicesFuncs.pfnDCImportBufferRelease(
				psBuffer->hImport,
				psBuffer->pasDevPAddr);
	}
#if defined(DCEX_VERBOSE)
	else
	{
		unsigned long ulPages = DC_OS_BYTES_TO_PAGES(psBuffer->ui32Size);
		IMG_DEV_PHYADDR sDevPAddr;

		for (i = 0; i < ulPages; i++)
		{
			PVRSRV_ERROR eError = DCExampleGetDevPAddrs(psBuffer->pvAllocHandle, &sDevPAddr, i, DC_OSGetPageSize());
			if (PVRSRV_OK == eError)
			{
				DC_OSDebugPrintf(DBGLVL_INFO, ": DCExampleBufferUnmap: DCExample unmap address 0x%016llx\n", sDevPAddr.uiAddr);
			}
		}
	}
#endif
	DCEX_DEBUG_PRINT("Unmap buffer (%p) from display\n", psBuffer);
}

/*
	In this example driver we provide (almost) the full range of functions
*/

static DC_DEVICE_FUNCTIONS sDCFunctions;

/*
    Release all driver data allocated during DCExampleInit()
*/
static void DCExampleReleaseDriverData(const IMG_BOOL bFromDeInit)
{
	DLLIST_NODE *psNode, *psNext;
	DCEX_DEVICE *psDeviceData;
	DCEX_BUFFER *psBuffer;

	dllist_foreach_node(&g_sDeviceDataListHead, psNode, psNext)
	{
		DLLIST_NODE *psBufNode, *psBufNext;

		psDeviceData = IMG_CONTAINER_OF(psNode, DCEX_DEVICE, sListNode);

		if (bFromDeInit)
		{
			/* Unregister the device before freeing */
			psDeviceData->sPVRServicesFuncs.pfnDCUnregisterDevice(psDeviceData->hSrvHandle);
		}

		/* Release all Heaps */
#if defined(LMA)
		psDeviceData->sPVRServicesFuncs.pfnPhysHeapRelease(psDeviceData->psPhysHeap);
#endif

		/* Close all connection handles */
		DC_OSPVRServicesConnectionClose(psDeviceData->hPVRServicesConnection);

		/* Release all associated buffers for this device */
		DC_OSMutexLock(psDeviceData->hBufListLock);
		dllist_foreach_node(&psDeviceData->sBufListNode, psBufNode, psBufNext)
		{
			psBuffer = IMG_CONTAINER_OF(psBufNode, DCEX_BUFFER, sListNode);

			/* Free all LMA / Virtual Buffer allocations */
#if defined(LMA)
			_DCExampleFreeLMABuffer(psBuffer->psDevice, psBuffer->uiAllocHandle);
#else
			DCExampleVirtualFree(psBuffer->pvAllocHandle);
#endif
			dllist_remove_node(psBufNode);

			DC_OSFreeMem(psBuffer);
		}
		DC_OSMutexUnlock(psDeviceData->hBufListLock);

		/* Release all OS memory */
		DC_OSMutexLock(g_hDevDataListLock);

		dllist_remove_node(psNode);

		DC_OSMutexUnlock(g_hDevDataListLock);

		DC_ASSERT(dllist_is_empty(&psDeviceData->sBufListNode));

		DC_OSMutexDestroy(psDeviceData->hBufListLock);
		DC_OSFreeMem(psDeviceData);
	}

	DC_ASSERT(dllist_is_empty(&g_sDeviceDataListHead));
}

#if defined(INTEGRITY_OS)
static PVRSRV_ERROR DCExampleAcquireKernelMappingData(IMG_HANDLE hBuffer, IMG_HANDLE *phMapping, void **ppPhysAddr)
{
	DCEX_BUFFER *psBuffer = (DCEX_BUFFER *)hBuffer;
	if (!psBuffer)
	{
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	return DCExampleOSAcquireKernelMappingData(psBuffer->pvAllocHandle, phMapping, ppPhysAddr);
}
#endif

/*
	functions exported by kernel services for use by 3rd party kernel display
	class device driver
*/
PVRSRV_ERROR DCExampleInit(IMG_UINT32 *puiNumDevices)
{
	DCEX_BUFFER *psBuffer;
	DCEX_DEVICE *psDeviceData;
	PVRSRV_ERROR eError;
#if defined(LMA)
	PPVRSRV_DEVICE_NODE psDevNode;
	IMG_UINT64 ui64BufferCount;
#endif
	const DC_EXAMPLE_MODULE_PARAMETERS *psModuleParams;
	IMG_UINT32 uiCurDev;
	IMG_UINT32 uiNumDevicesConfigured;

	DC_OSSetDrvName(DRVNAME);
	dllist_init(&g_sDeviceDataListHead);

	eError = DC_OSMutexCreate(&g_hDevDataListLock);
	if (PVRSRV_OK != eError)
	{
		DC_OSDebugPrintf(DBGLVL_ERROR, " - %s: Failed to create g_hDevDataListLock (%d)\n", __func__, eError);
		goto fail_nolocks;
	}

	/* Check the module params and setup global buffer size state */
	if (!CheckBufferDimensions())
	{
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	psModuleParams = DCExampleGetModuleParameters();
	if (NULL == psModuleParams)
	{
		DC_OSDebugPrintf(DBGLVL_ERROR, ": Cannot fetch module parameters\n");
		return PVRSRV_ERROR_NO_DEVICEDATA_FOUND;
	}

	/* Get the number of physically registered devices in the system. */
	uiNumDevicesConfigured = psModuleParams->ui32NumDevices;

	/* Validate the number-of-devices passed in as a driver parameter */
	if (uiNumDevicesConfigured > PVRSRV_MAX_DEVICES)
	{
		DC_OSDebugPrintf(DBGLVL_ERROR,
		                 "num_devices '%u': exceeds PVRSRV_MAX_DEVICES '%u'\n",
		                 uiNumDevicesConfigured, PVRSRV_MAX_DEVICES);
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	DCEX_DEBUG_PRINT("%s: Num Registered = %u\n", __func__, uiNumDevicesConfigured);

	for (uiCurDev = 0; uiCurDev < uiNumDevicesConfigured; uiCurDev++)
	{

		/*
			If the display controller hasn't already been initialised elsewhere
			in the system then it should be initialised here.

			Create the private data structure (psDeviceData) and store all the
			device specific data we will need later (e.g. pointer to mapped
			registered)
			This device specific private data will be passed into our callbacks
			so we don't need global data and can have more than one display
			controller driven by the same driver (we would just create an
			"instance" of a device
			by registering the same callbacks with different private data)
		*/

		DCEX_DEBUG_PRINT("%s: Initialising device %u\n", __func__, uiCurDev);

		psDeviceData = DC_OSCallocMem(sizeof(DCEX_DEVICE));
		if (psDeviceData == NULL)
		{
			eError = PVRSRV_ERROR_OUT_OF_MEMORY;
			goto fail_devicealloc;
		}

		dllist_init(&psDeviceData->sListNode);
		dllist_init(&psDeviceData->sBufListNode);

		eError = DC_OSMutexCreate(&psDeviceData->hBufListLock);
		if (PVRSRV_OK != eError)
		{
			DC_OSDebugPrintf(DBGLVL_ERROR, " - %s: Failed to create hBufListLock (%d)\n",  __func__, eError);
			goto fail_servicesconnectionclose;
		}

		eError = DC_OSPVRServicesConnectionOpen(&psDeviceData->hPVRServicesConnection);
		if (eError != PVRSRV_OK)
		{
			DC_OSDebugPrintf(DBGLVL_ERROR, " - %s: Failed to open connection to PVR Services (%d)\n", __func__, eError);
			goto fail_servicesconnectionclose;
		}

		eError = DC_OSPVRServicesSetupFuncs(psDeviceData->hPVRServicesConnection, &psDeviceData->sPVRServicesFuncs);
		if (eError != PVRSRV_OK)
		{
			DC_OSDebugPrintf(DBGLVL_ERROR, " - %s: Failed to setup PVR Services function table (%d)\n", __func__, eError);
			goto fail_servicesconnectionclose;
		}

#if defined(LMA)
		psDevNode = psDeviceData->sPVRServicesFuncs.pfnGetDeviceInstance(uiCurDev);
		PVR_GOTO_IF_INVALID_PARAM(psDevNode != NULL, eError, fail_devicealloc);
		/*
			If the display is using card memory then we need to know
			where that memory is so we have to acquire the heap we want
			to use (a carveout of the card memory) so we can get it's address
		*/
		eError = psDeviceData->sPVRServicesFuncs.pfnPhysHeapAcquireByID(
					PVRSRV_PHYS_HEAP_DISPLAY,
					psDevNode,
					&psDeviceData->psPhysHeap);

		if (eError != PVRSRV_OK)
		{
			goto fail_heapacquire;
		}

		/* Verify we're operating on a LMA heap */
		DC_ASSERT(psDeviceData->sPVRServicesFuncs.pfnPhysHeapGetType(psDeviceData->psPhysHeap) == PHYS_HEAP_TYPE_LMA);

		eError = psDeviceData->sPVRServicesFuncs.pfnPhysHeapGetCpuPAddr(
					psDeviceData->psPhysHeap,
					&psDeviceData->sDispStartAddr);

		DC_ASSERT(eError == PVRSRV_OK);

		eError = psDeviceData->sPVRServicesFuncs.pfnPhysHeapGetSize(
					psDeviceData->psPhysHeap,
					&psDeviceData->uiDispMemSize);

		DC_ASSERT(eError == PVRSRV_OK);
#endif
		/*
			If the display driver has a system surface create the buffer structure
			that describes it here.

			Note:
			All this data and the buffer should be queried from the OS, but in this
			example we don't have the ability to fetch this data from an OS driver,
			so we create the data.
		*/
		psBuffer = DC_OSCallocMem(sizeof(DCEX_BUFFER));

		if (psBuffer == NULL)
		{
			eError = PVRSRV_ERROR_OUT_OF_MEMORY;
			goto fail_bufferalloc;
		}

		dllist_init(&psBuffer->sListNode);

		DC_OSAtomicWrite(&psBuffer->i32RefCount, 1);
		psBuffer->ePixFormat = ePixelFormat;
		psBuffer->ui32Width = psModuleParams->ui32Width;
		psBuffer->ui32Height = psModuleParams->ui32Height;
		psBuffer->ui32ByteStride = ui32ByteStride;
		psBuffer->ui32Size = psBuffer->ui32Height * psBuffer->ui32ByteStride;
		psBuffer->ui32Size += (psBuffer->ui32Size >> 4); /* Speculative extra 6.25% for FBC overhead. */
		psBuffer->ui32Size = (psBuffer->ui32Size + DC_OSGetPageSize() - 1) & DC_OSGetPageMask();
		psBuffer->psDevice = psDeviceData;
		psBuffer->bIsSysBuffer = IMG_TRUE;

#if defined(LMA)
		/* Simple allocator, assume all buffers are going to be the same size. */
		ui64BufferCount = psDeviceData->uiDispMemSize;
		ui64BufferCount = DC_OSDiv64(ui64BufferCount, psBuffer->ui32Size);

		psDeviceData->ui32BufferCount = (IMG_UINT32) ui64BufferCount;
		DC_ASSERT((IMG_UINT32) ui64BufferCount == psDeviceData->ui32BufferCount);

		if (psDeviceData->ui32BufferCount > 32)
		{
			psDeviceData->ui32BufferCount = 32;
		}

		psDeviceData->ui32BufferSize = psBuffer->ui32Size;
		psDeviceData->ui32BufferUseMask = 0;

		DC_OSDebugPrintf(DBGLVL_INFO, " Buffers: %u (%u bytes each)\n",
		                 psDeviceData->ui32BufferCount, psDeviceData->ui32BufferSize);

		psBuffer->uiAllocHandle = _DCExampleAllocLMABuffer(psDeviceData);
		if (psBuffer->uiAllocHandle == 0)
		{
			eError = PVRSRV_ERROR_OUT_OF_MEMORY;
			goto fail_buffermemalloc;
		}
		DCEX_DEBUG_PRINT("Allocate system buffer handle = %p\n", psBuffer->uiAllocHandle);
#else
		psBuffer->pvAllocHandle = DCExampleVirtualAllocUncached(psBuffer->ui32Size);
		if (psBuffer->pvAllocHandle == NULL)
		{
			eError = PVRSRV_ERROR_OUT_OF_MEMORY;
			goto fail_buffermemalloc;
		}
#endif
		DCEX_DEBUG_PRINT("Allocate system buffer = %p\n", psBuffer);

		/* Initialise DC Function Table */
		DC_OSMemSet(&sDCFunctions, 0, sizeof(sDCFunctions));

		sDCFunctions.pfnGetInfo					= DCExampleGetInfo;
		sDCFunctions.pfnPanelQueryCount			= DCExamplePanelQueryCount;
		sDCFunctions.pfnPanelQuery				= DCExamplePanelQuery;
		sDCFunctions.pfnFormatQuery				= DCExampleFormatQuery;
		sDCFunctions.pfnDimQuery				= DCExampleDimQuery;
		sDCFunctions.pfnSetBlank				= NULL;
		sDCFunctions.pfnSetVSyncReporting		= NULL;
		sDCFunctions.pfnLastVSyncQuery			= NULL;
		sDCFunctions.pfnContextCreate			= DCExampleContextCreate;
		sDCFunctions.pfnContextDestroy			= DCExampleContextDestroy;
		sDCFunctions.pfnContextConfigure		= DCExampleContextConfigure;
		sDCFunctions.pfnContextConfigureCheck	= DCExampleContextConfigureCheck;
		sDCFunctions.pfnBufferAlloc				= DCExampleBufferAlloc;
		sDCFunctions.pfnBufferAcquire			= DCExampleBufferAcquire;
		sDCFunctions.pfnBufferRelease			= DCExampleBufferRelease;
		sDCFunctions.pfnBufferFree				= DCExampleBufferFree;
#if !defined(LMA)
		sDCFunctions.pfnBufferImport			= DCExampleBufferImport;
#endif
		sDCFunctions.pfnBufferMap				= DCExampleBufferMap;
		sDCFunctions.pfnBufferUnmap				= DCExampleBufferUnmap;
		sDCFunctions.pfnBufferSystemAcquire		= DCExampleBufferSystemAcquire;
		sDCFunctions.pfnBufferSystemRelease		= DCExampleBufferSystemRelease;
#if defined(INTEGRITY_OS)
		sDCFunctions.pfnAcquireKernelMappingData = DCExampleAcquireKernelMappingData;
#endif

		/*
			Register our DC driver with services
		*/
		eError = psDeviceData->sPVRServicesFuncs.pfnDCRegisterDevice(
					&sDCFunctions,
					MAX_COMMANDS_INFLIGHT,
					psDeviceData,
					&psDeviceData->hSrvHandle);

		/*
		    We may fail to register if we've had more instances enabled than
		    there are physical devices available. If this is not the first
		    device we should allow access to all of the devices currently
		    registered.
		 */
		if (eError != PVRSRV_OK)
		{
			DC_OSDebugPrintf(DBGLVL_WARNING,
			                 "pfnDCRegisterDevice failed (%d). %u devices registered\n",
			                  eError, uiCurDev);
			if (uiCurDev > 0)
			{
				/* Free psSystemBuffer and psDeviceData */
#if defined(LMA)
				_DCExampleFreeLMABuffer(psBuffer->psDevice,
				                        psBuffer->uiAllocHandle);
				psDeviceData->sPVRServicesFuncs.pfnPhysHeapRelease(psDeviceData->psPhysHeap);
#else
				DCExampleVirtualFree(psBuffer->pvAllocHandle);
#endif
				DC_OSFreeMem(psBuffer);
				DC_OSPVRServicesConnectionClose(psDeviceData->hPVRServicesConnection);
				DC_OSFreeMem(psDeviceData);
				/* Allow access to successfully discovered devices */
				if (puiNumDevices)
				{
					*puiNumDevices = uiCurDev;
				}
				return PVRSRV_OK;
			}
			else
			{
				goto fail_register;
			}
		}

		/* Save the device data somewhere we can retrieve it */
		DC_OSMutexLock(g_hDevDataListLock);
		dllist_add_to_tail(&g_sDeviceDataListHead, &psDeviceData->sListNode);
		DC_OSMutexUnlock(g_hDevDataListLock);

		/* Store the system buffer on the device hook */
		DC_OSMutexLock(psDeviceData->hBufListLock);
		dllist_add_to_tail(&psDeviceData->sBufListNode, &psBuffer->sListNode);
		DC_OSMutexUnlock(psDeviceData->hBufListLock);
	}	/* End of loop over # devices to create */

	if (puiNumDevices)
	{
		*puiNumDevices = uiNumDevicesConfigured;
	}
	return PVRSRV_OK;

fail_register:
	/*
	 * Warning: Here we are aborting the initialisation and we may have
	 * partially allocated some data. On entry psDeviceData refers to the
	 * most recent 'in-progress' device allocation as does 'psBuffer' refer
	 * to the in-progress buffer allocation.
	 * We need to free these first, and then clean up any already established
	 * contents which will be hanging from the DLLIST_NODEs
	 */
	if (!dllist_node_is_in_list(&psBuffer->sListNode))
	{
#if defined(LMA)
		_DCExampleFreeLMABuffer(psBuffer->psDevice, psBuffer->uiAllocHandle);
#else
		DCExampleVirtualFree(psBuffer->pvAllocHandle);
#endif
	}

fail_buffermemalloc:
	/* Free all buffer allocations */
	if (!dllist_node_is_in_list(&psBuffer->sListNode))
	{
		DC_OSFreeMem(psBuffer);
	}

fail_bufferalloc:
	/* Release all Heaps */
#if defined(LMA)
	if (!dllist_node_is_in_list(&psDeviceData->sListNode))
	{
		psDeviceData->sPVRServicesFuncs.pfnPhysHeapRelease(psDeviceData->psPhysHeap);
	}
fail_heapacquire:
#endif

	/* Close all connection handles */
	if (!dllist_node_is_in_list(&psDeviceData->sListNode))
	{
		DC_OSPVRServicesConnectionClose(psDeviceData->hPVRServicesConnection);
	}

fail_servicesconnectionclose:
	/* Release all OS memory */
	if (!dllist_node_is_in_list(&psDeviceData->sListNode))
	{
		DC_OSFreeMem(psDeviceData);
	}

	/*
	 * Now we must clean up any previously completed allocations. These will,
	 * if present, be on the DLLIST_NODE lists for g_sDeviceDataListHead and
	 * the incorporated sBufListNode entries within.
	 */
	DCExampleReleaseDriverData(IMG_FALSE);

fail_devicealloc:
	DC_OSMutexDestroy(g_hDevDataListLock);

fail_nolocks:
	return eError;
}

void DCExampleDeinit(void)
{

	DCExampleReleaseDriverData(IMG_TRUE);

	/* Free the global list locks. */
	DC_OSMutexDestroy(g_hDevDataListLock);
}
