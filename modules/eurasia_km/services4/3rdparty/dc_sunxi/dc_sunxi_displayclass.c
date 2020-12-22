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

#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/console.h>
#include <linux/fb.h>
#include <linux/module.h>
#include <linux/string.h>
#include <linux/notifier.h>


#if defined(SUPPORT_DRI_DRM)
#include <drm/drmP.h>
#endif

/* IMG services headers */
#include "img_defs.h"
#include "servicesext.h"
#include "kerneldisplay.h"
#include "linux/drv_display.h"

#if defined(SUPPORT_DRI_DRM)
#include "pvr_drm.h"
#else
#include "pvrmodule.h"
#endif

#define DC_SUNXI_COMMAND_COUNT		    1

#define	DC_SUNXI_VSYNC_SETTLE_COUNT	    5

#define	DC_SUNXI_MAX_NUM_DEVICES		1

//#define DC_SUNXI_DISPC_GRALLOC_QUEUE_IN_V1_PATH

#if (DC_SUNXI_MAX_NUM_DEVICES > FB_MAX)
#error DC_SUNXI_MAX_NUM_DEVICES must not be greater than FB_MAX
#endif

/* DEBUG only printk */
#ifdef DEBUG
#define	DEBUG_PRINTK(x) printk x
#else
#define	DEBUG_PRINTK(x)
#endif

#define DISPLAY_DEVICE_NAME		"SUNXI Linux Display Driver"
#define	DRVNAME					"dc_sunxi"
#define	DEVNAME					DRVNAME
#define	DRIVER_PREFIX			DRVNAME

#ifndef UNREFERENCED_PARAMETER
#define	UNREFERENCED_PARAMETER(param) (param) = (param)
#endif

#if !defined(SUPPORT_DRI_DRM)
MODULE_SUPPORTED_DEVICE(DEVNAME);
#endif

/* DC_SUNXI buffer structure */
typedef struct DC_SUNXI_BUFFER_TAG
{
	struct DC_SUNXI_BUFFER_TAG	*psNext;
	struct DC_SUNXI_DEVINFO_TAG	*psDevInfo;

	struct work_struct			sWork;

	/* Position of this buffer in the virtual framebuffer */
	unsigned long				ulYOffset;

	/* IMG structures used, to minimise API function code */
	/* replace with own structures where necessary */
	IMG_SYS_PHYADDR				sSysAddr;
	IMG_CPU_VIRTADDR			sCPUVAddr;
	PVRSRV_SYNC_DATA			*psSyncData;

	void						*hCmdComplete;
	unsigned long    			ulSwapInterval;

} DC_SUNXI_BUFFER;

/* DC_SUNXI swapchain structure */
typedef struct DC_SUNXI_SWAPCHAIN_TAG
{
	/* Swap chain ID */
	unsigned int				uiSwapChainID;

	/* number of buffers in swapchain */
	unsigned long				ulBufferCount;

	/* list of buffers in the swapchain */
	DC_SUNXI_BUFFER				*psBuffer;

	/* Swap chain work queue */
	struct workqueue_struct		*psWorkQueue;

	/*
	 * Set if we didn't manage to wait for VSync on last swap,
	 * or if we think we need to wait for VSync on the next flip.
	 * The flag helps to avoid jitter when the screen is
	 * unblanked, by forcing an extended wait for VSync before
	 * attempting the next flip.
	 */
	bool						bNotVSynced;

	/* Framebuffer Device ID for messages (e.g. printk) */
	unsigned int				uiFBDevID;

} DC_SUNXI_SWAPCHAIN;

typedef struct DC_SUNXI_FBINFO_TAG
{
	unsigned long				ulFBSize;
	unsigned long				ulBufferSize;
	unsigned long				ulRoundedBufferSize;
	unsigned long				ulWidth;
	unsigned long				ulHeight;
	unsigned long				ulByteStride;
	unsigned long				ulPhysicalWidthmm;
	unsigned long				ulPhysicalHeightmm;

	/* IMG structures used, to minimise API function code */
	/* replace with own structures where necessary */
	IMG_SYS_PHYADDR				sSysAddr;
	IMG_CPU_VIRTADDR			sCPUVAddr;

	/* pixelformat of system/primary surface */
	PVRSRV_PIXEL_FORMAT			ePixelFormat;

} DC_SUNXI_FBINFO;

/* kernel device information structure */
typedef struct DC_SUNXI_DEVINFO_TAG
{
	/* Framebuffer Device ID */
	unsigned int				uiFBDevID;

	/* PVR Device ID */
	unsigned int				uiPVRDevID;

	/* Swapchain create/destroy mutex */
	struct mutex				sCreateSwapChainMutex;

	/* system surface info */
	DC_SUNXI_BUFFER				sSystemBuffer;

	/* jump table into PVR services */
	PVRSRV_DC_DISP2SRV_KMJTABLE	sPVRJTable;

	/* jump table into DC */
	PVRSRV_DC_SRV2DISP_KMJTABLE	sDCJTable;

	/* fb info structure */
	DC_SUNXI_FBINFO				sFBInfo;

	/* Only one swapchain supported by this device so hang it here */
	DC_SUNXI_SWAPCHAIN			*psSwapChain;

	/* Swap chain ID */
	unsigned int				uiSwapChainID;

	/* True if PVR Services is flushing its command queues */
	atomic_t					sFlushCommands;

	/* pointer to linux frame buffer information structure */
	struct fb_info				*psLINFBInfo;

	/* IMG structures used, to minimise API function code */
	/* replace with own structures where necessary */

	DISPLAY_INFO				sDisplayInfo;

	/* Display format */
	DISPLAY_FORMAT				sDisplayFormat;

	/* Display dimensions */
	DISPLAY_DIMS				sDisplayDim;

	/* Number of blank/unblank events */
	atomic_t					sBlankEvents;

}  DC_SUNXI_DEVINFO;

typedef enum _DC_SUNXI_ERROR_
{
	DC_SUNXI_OK						= 0,
	DC_SUNXI_ERROR_GENERIC			= 1,
	DC_SUNXI_ERROR_INIT_FAILURE		= 2,
	DC_SUNXI_ERROR_INVALID_DEVICE	= 3,

} DC_SUNXI_ERROR;

static DC_SUNXI_DEVINFO *gapsDevInfo[DC_SUNXI_MAX_NUM_DEVICES];

/* Top level 'hook ptr' */
static PFN_DC_GET_PVRJTABLE gpfnGetPVRJTable;

/* Don't wait for vertical sync */
static inline bool DontWaitForVSync(DC_SUNXI_DEVINFO *psDevInfo)
{
	return atomic_read(&psDevInfo->sFlushCommands);
}

/*
 * Called after the screen has unblanked, or after any other occasion
 * when we didn't wait for vsync, but now need to. Not doing this after
 * unblank leads to screen jitter on some screens.
 * Returns true if the screen has been deemed to have settled.
 */
static bool WaitForVSyncSettle(DC_SUNXI_DEVINFO *psDevInfo)
{
	unsigned int i;
	for(i = 0; i < DC_SUNXI_VSYNC_SETTLE_COUNT; i++)
	{
		if (DontWaitForVSync(psDevInfo))
		{
			return false;
		}
	}

	return true;
}

/* Flip display to given buffer */
static void DC_SUNXIFlip(DC_SUNXI_DEVINFO *psDevInfo, DC_SUNXI_BUFFER *psBuffer)
{
	struct fb_var_screeninfo sFBVar;
	unsigned long ulYResVirtual;
	int res;

	console_lock();

	sFBVar = psDevInfo->psLINFBInfo->var;

	sFBVar.xoffset = 0;
	sFBVar.yoffset = psBuffer->ulYOffset;

	ulYResVirtual = psBuffer->ulYOffset + sFBVar.yres;

	res = fb_pan_display(psDevInfo->psLINFBInfo, &sFBVar);
	if (res != 0)
	{
		printk(KERN_ERR DRIVER_PREFIX ": %s: Device %u: fb_pan_display failed (Y Offset: %lu, Error: %d)\n", __FUNCTION__, psDevInfo->uiFBDevID, psBuffer->ulYOffset, res);
	}

	console_unlock();
}

/*
 * Swap handler.
 * Called from the swap chain work queue handler.
 * There is no need to take the swap chain creation lock in here, or use
 * some other method of stopping the swap chain from being destroyed.
 * This is because the swap chain creation lock is taken when queueing work,
 * and the work queue is flushed before the swap chain is destroyed.
 */
static void DC_SUNXISwapHandler(DC_SUNXI_BUFFER *psBuffer)
{
	DC_SUNXI_DEVINFO *psDevInfo = psBuffer->psDevInfo;
	DC_SUNXI_SWAPCHAIN *psSwapChain = psDevInfo->psSwapChain;
	bool bPreviouslyNotVSynced;

	DC_SUNXIFlip(psDevInfo, psBuffer);

	bPreviouslyNotVSynced = psSwapChain->bNotVSynced;
	psSwapChain->bNotVSynced = true;

	if (!DontWaitForVSync(psDevInfo))
	{
		psSwapChain->bNotVSynced = false;

		if (bPreviouslyNotVSynced)
		{
			psSwapChain->bNotVSynced = !WaitForVSyncSettle(psDevInfo);
		}
		else if (psBuffer->ulSwapInterval != 0)
		{
			psSwapChain->bNotVSynced = IMG_FALSE;
		}
	}
	
	psDevInfo->sPVRJTable.pfnPVRSRVCmdComplete((IMG_HANDLE)psBuffer->hCmdComplete, IMG_TRUE);
}

/* Process an item on a swap chain work queue */
static void WorkQueueHandler(struct work_struct *psWork)
{
	DC_SUNXI_BUFFER *psBuffer = container_of(psWork, DC_SUNXI_BUFFER, sWork);

	DC_SUNXISwapHandler(psBuffer);
}

/* Create a swap chain work queue */
static DC_SUNXI_ERROR DC_SUNXICreateSwapQueue(DC_SUNXI_SWAPCHAIN *psSwapChain)
{
	/*
	 * Calling alloc_ordered_workqueue with the WQ_FREEZABLE and
	 * WQ_MEM_RECLAIM flags set, (currently) has the same effect as
	 * calling create_freezable_workqueue. None of the other WQ
	 * flags are valid. Setting WQ_MEM_RECLAIM should allow the
	 * workqueue to continue to service the swap chain in low memory
	 * conditions, preventing the driver from holding on to
	 * resources longer than it needs to.
	 */
	psSwapChain->psWorkQueue = alloc_ordered_workqueue(DEVNAME, WQ_FREEZABLE | WQ_MEM_RECLAIM);
	if (psSwapChain->psWorkQueue == NULL)
	{
		printk(KERN_ERR DRIVER_PREFIX ": %s: Device %u: Couldn't create workqueue\n", __FUNCTION__, psSwapChain->uiFBDevID);
		
		return (DC_SUNXI_ERROR_INIT_FAILURE);
	}

	return (DC_SUNXI_OK);
}

/* Prepare buffer for insertion into a swap chain work queue */
static void DC_SUNXIInitBufferForSwap(DC_SUNXI_BUFFER *psBuffer)
{
	INIT_WORK(&psBuffer->sWork, WorkQueueHandler);
}

/* Destroy a swap chain work queue */
static void DC_SUNXIDestroySwapQueue(DC_SUNXI_SWAPCHAIN *psSwapChain)
{
	destroy_workqueue(psSwapChain->psWorkQueue);
}

/* Unblank the screen */
static DC_SUNXI_ERROR DC_SUNXIUnblankDisplay(DC_SUNXI_DEVINFO *psDevInfo)
{
	int res;

	console_lock();
	res = fb_blank(psDevInfo->psLINFBInfo, 0);
	console_unlock();

	if (res != 0 && res != -EINVAL)
	{
		printk(KERN_ERR DRIVER_PREFIX
		": %s: Device %u: fb_blank failed (%d)\n", __FUNCTION__, psDevInfo->uiFBDevID, res);
		return (DC_SUNXI_ERROR_GENERIC);
	}

	return (DC_SUNXI_OK);
}

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

static unsigned DC_SUNXIMaxFBDevIDPlusOne(void)
{
	return DC_SUNXI_MAX_NUM_DEVICES;
}

/* Returns DevInfo pointer for a given device */
static DC_SUNXI_DEVINFO *DC_SUNXIGetDevInfoPtr(unsigned uiFBDevID)
{
	WARN_ON(uiFBDevID >= DC_SUNXIMaxFBDevIDPlusOne());

	if (uiFBDevID >= DC_SUNXI_MAX_NUM_DEVICES)
	{
		return NULL;
	}

	return gapsDevInfo[uiFBDevID];
}

/* Sets the DevInfo pointer for a given device */
static inline void DC_SUNXISetDevInfoPtr(unsigned uiFBDevID, DC_SUNXI_DEVINFO *psDevInfo)
{
	WARN_ON(uiFBDevID >= DC_SUNXI_MAX_NUM_DEVICES);

	if (uiFBDevID < DC_SUNXI_MAX_NUM_DEVICES)
	{
		gapsDevInfo[uiFBDevID] = psDevInfo;
	}
}

static inline bool SwapChainHasChanged(DC_SUNXI_DEVINFO *psDevInfo, DC_SUNXI_SWAPCHAIN *psSwapChain)
{
	return (psDevInfo->psSwapChain != psSwapChain) ||
		   (psDevInfo->uiSwapChainID != psSwapChain->uiSwapChainID);
}

extern int dispc_gralloc_queue(setup_dispc_data_t *psDispcData, int ui32DispcDataLength, void (*cb_fn)(void *, int), void *cb_arg);

static void
QueueBufferImmediate(DC_SUNXI_DEVINFO *psDevInfo, IMG_SYS_PHYADDR sSysAddr,
					 void (*cb_fn)(void *, int), void *cb_arg)
{
	setup_dispc_data_t sDispcData =
	{
		.post2_layers				= 1,
		.primary_display_layer_num	= 1,
		.layer_info =
		{
			[0] =
			{
				.mode				= DISP_LAYER_WORK_MODE_NORMAL,
				.src_win =
				{
					.width			= psDevInfo->sFBInfo.ulWidth,
					.height			= psDevInfo->sFBInfo.ulHeight
				},
				.scn_win =
				{
					.width			= psDevInfo->sFBInfo.ulWidth,
					.height			= psDevInfo->sFBInfo.ulHeight
				},
				.alpha_en			= 1,
				.alpha_val			= 0xff,
				.fb =
				{
					.mode			= DISP_MOD_INTERLEAVED,
					.format			= DISP_FORMAT_ARGB8888,
					.seq			= DISP_SEQ_ARGB,
					.pre_multiply	= 1,
					.size			=
					{
						.width		= psDevInfo->sFBInfo.ulWidth,
						.height		= psDevInfo->sFBInfo.ulHeight
					},
					.addr[0]		= sSysAddr.uiAddr,
				},
			},
		},
	};

	dispc_gralloc_queue(&sDispcData, sizeof(setup_dispc_data_t), cb_fn, cb_arg);
}

static void dispc_proxy_cmdcomplete(void * cookie, int i)
{
	/* Workaround for bug in sunxi kernel, where it uses the latest cb_fn
	 * with older cb_arg (they should be used as a pair).
	 */
	if (cookie == (void *)0xdeadbeef)
		return;

	/* XXX: assumes that there is only one display */
	gapsDevInfo[0]->sPVRJTable.pfnPVRSRVCmdComplete(cookie, i);
}

/*
 * SetDCState
 * Called from services.
 */
static IMG_VOID SetDCState(IMG_HANDLE hDevice, IMG_UINT32 ui32State)
{
	DC_SUNXI_DEVINFO *psDevInfo = (DC_SUNXI_DEVINFO *)hDevice;

	switch (ui32State)
	{
		case DC_STATE_FLUSH_COMMANDS:
			/* Flush out any 'real' operation waiting for another flip.
			 * In flush state we won't pass any 'real' operations along
			 * to dispc_gralloc_queue(); we'll just CmdComplete them
			 * immediately.
			 */
			QueueBufferImmediate(psDevInfo, psDevInfo->sSystemBuffer.sSysAddr,
								 dispc_proxy_cmdcomplete, (void *)0xdeadbeef);
			atomic_set(&psDevInfo->sFlushCommands, true);
			break;
		case DC_STATE_NO_FLUSH_COMMANDS:
			atomic_set(&psDevInfo->sFlushCommands, false);
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
	DC_SUNXI_DEVINFO *psDevInfo;
	DC_SUNXI_ERROR eError;
	unsigned int i, uiMaxFBDevIDPlusOne;

	if (!try_module_get(THIS_MODULE))
	{
		return PVRSRV_ERROR_UNABLE_TO_OPEN_DC_DEVICE;
	}

	uiMaxFBDevIDPlusOne = DC_SUNXIMaxFBDevIDPlusOne();

	for (i = 0; i < uiMaxFBDevIDPlusOne; i++)
	{
		psDevInfo = DC_SUNXIGetDevInfoPtr(i);
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

	eError = DC_SUNXIUnblankDisplay(psDevInfo);
	if (eError != DC_SUNXI_OK)
	{
		DEBUG_PRINTK((KERN_WARNING DRIVER_PREFIX
					  ": %s: Device %u: DC_SUNXIUnblankDisplay failed (%d)\n", __FUNCTION__, psDevInfo->uiFBDevID, eError));
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
	UNREFERENCED_PARAMETER(hDevice);

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
	DC_SUNXI_DEVINFO *psDevInfo;

	if(!hDevice || !pui32NumFormats)
	{
		return PVRSRV_ERROR_INVALID_PARAMS;
	}
	
	psDevInfo = (DC_SUNXI_DEVINFO*)hDevice;

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
	DC_SUNXI_DEVINFO *psDevInfo;

	if(!hDevice || !psFormat || !pui32NumDims)
	{
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	psDevInfo = (DC_SUNXI_DEVINFO*)hDevice;

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
	DC_SUNXI_DEVINFO *psDevInfo;

	if(!hDevice || !phBuffer)
	{
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	psDevInfo = (DC_SUNXI_DEVINFO*)hDevice;

	*phBuffer = (IMG_HANDLE)&psDevInfo->sSystemBuffer;

	return PVRSRV_OK;
}


/*
 * GetDCInfo
 * Called from services.
 */
static PVRSRV_ERROR GetDCInfo(IMG_HANDLE hDevice, DISPLAY_INFO *psDCInfo)
{
	DC_SUNXI_DEVINFO *psDevInfo;

	if(!hDevice || !psDCInfo)
	{
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	psDevInfo = (DC_SUNXI_DEVINFO*)hDevice;

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
	DC_SUNXI_DEVINFO *psDevInfo;
	DC_SUNXI_BUFFER *psSystemBuffer;

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

	psDevInfo = (DC_SUNXI_DEVINFO*)hDevice;

	psSystemBuffer = (DC_SUNXI_BUFFER *)hBuffer;

	*ppsSysAddr = &psSystemBuffer->sSysAddr;

	*pui32ByteSize = (IMG_UINT32)psDevInfo->sFBInfo.ulBufferSize;

	if (ppvCpuVAddr)
	{
		*ppvCpuVAddr = psSystemBuffer->sCPUVAddr;
	}

	if (phOSMapInfo)
	{
		*phOSMapInfo = (IMG_HANDLE)0;
	}

	if (pbIsContiguous)
	{
		*pbIsContiguous = IMG_TRUE;
	}

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
	DC_SUNXI_SWAPCHAIN *psSwapChain;
	DC_SUNXI_DEVINFO *psDevInfo;
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

	psDevInfo = (DC_SUNXI_DEVINFO*)hDevice;

	/* Do we support swap chains? */
	if (psDevInfo->sDisplayInfo.ui32MaxSwapChains == 0)
	{
		return PVRSRV_ERROR_NOT_SUPPORTED;
	}

	mutex_lock(&psDevInfo->sCreateSwapChainMutex);

	/* The driver only supports a single swapchain */
	if(psDevInfo->psSwapChain != NULL)
	{
		eError = PVRSRV_ERROR_FLIP_CHAIN_EXISTS;
		goto ExitUnLock;
	}

	/* create a swapchain structure */
	psSwapChain = (DC_SUNXI_SWAPCHAIN*)kmalloc(sizeof(DC_SUNXI_SWAPCHAIN), GFP_KERNEL);
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

		psSwapChain->psBuffer = (DC_SUNXI_BUFFER*)kmalloc(sizeof(DC_SUNXI_BUFFER) * ui32BufferCount, GFP_KERNEL);
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

			psSwapChain->psBuffer[i].psSyncData = ppsSyncData[i];

			psSwapChain->psBuffer[i].sSysAddr.uiAddr = psDevInfo->sFBInfo.sSysAddr.uiAddr + ui32BufferOffset;
			psSwapChain->psBuffer[i].sCPUVAddr = psDevInfo->sFBInfo.sCPUVAddr + ui32BufferOffset;
			psSwapChain->psBuffer[i].ulYOffset = ui32BufferOffset / psDevInfo->sFBInfo.ulByteStride;
			psSwapChain->psBuffer[i].psDevInfo = psDevInfo;

			DC_SUNXIInitBufferForSwap(&psSwapChain->psBuffer[i]);
		}
	}
	else
	{
		psSwapChain->psBuffer = NULL;
	}

	psSwapChain->ulBufferCount = (unsigned long)ui32BufferCount;
	psSwapChain->bNotVSynced = true;
	psSwapChain->uiFBDevID = psDevInfo->uiFBDevID;

	if (DC_SUNXICreateSwapQueue(psSwapChain) != DC_SUNXI_OK)
	{ 
		printk(KERN_WARNING DRIVER_PREFIX ": %s: Device %u: Failed to create workqueue\n", __FUNCTION__, psDevInfo->uiFBDevID);
		eError = PVRSRV_ERROR_UNABLE_TO_INSTALL_ISR;
		goto ErrorFreeBuffers;
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
	
ErrorFreeBuffers:
	if(psSwapChain->psBuffer)
	{
		kfree(psSwapChain->psBuffer);
	}
ErrorFreeSwapChain:
	kfree(psSwapChain);
ExitUnLock:
	mutex_unlock(&psDevInfo->sCreateSwapChainMutex);
	return eError;
}

/*
 * DestroyDCSwapChain
 * Called from services.
 */
static PVRSRV_ERROR DestroyDCSwapChain(IMG_HANDLE hDevice,
									   IMG_HANDLE hSwapChain)
{
	DC_SUNXI_DEVINFO *psDevInfo;
	DC_SUNXI_SWAPCHAIN *psSwapChain;
	DC_SUNXI_ERROR eError;

	/* Check parameters */
	if(!hDevice || !hSwapChain)
	{
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	psDevInfo = (DC_SUNXI_DEVINFO*)hDevice;
	psSwapChain = (DC_SUNXI_SWAPCHAIN*)hSwapChain;

	mutex_lock(&psDevInfo->sCreateSwapChainMutex);

	if (SwapChainHasChanged(psDevInfo, psSwapChain))
	{
		printk(KERN_WARNING DRIVER_PREFIX
			   ": %s: Device %u: Swap chain mismatch\n", __FUNCTION__, psDevInfo->uiFBDevID);
		
		eError = PVRSRV_ERROR_INVALID_PARAMS;
		goto ExitUnLock;
	}

	/* The swap queue is flushed before being destroyed */
	DC_SUNXIDestroySwapQueue(psSwapChain);

	/* Free resources */
	if (psSwapChain->psBuffer)
	{
		kfree(psSwapChain->psBuffer);
	}
	kfree(psSwapChain);

	psDevInfo->psSwapChain = NULL;

	DC_SUNXIFlip(psDevInfo, &psDevInfo->sSystemBuffer);

	eError = PVRSRV_OK;

ExitUnLock:
	mutex_unlock(&psDevInfo->sCreateSwapChainMutex);
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
	DC_SUNXI_DEVINFO *psDevInfo;
	DC_SUNXI_SWAPCHAIN *psSwapChain;
	PVRSRV_ERROR eError;
	unsigned int i;

	/* Check parameters */
	if(!hDevice 
		|| !hSwapChain
		|| !pui32BufferCount
		|| !phBuffer)
	{
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	psDevInfo = (DC_SUNXI_DEVINFO*)hDevice;
	psSwapChain = (DC_SUNXI_SWAPCHAIN*)hSwapChain;

	mutex_lock(&psDevInfo->sCreateSwapChainMutex);

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
	mutex_unlock(&psDevInfo->sCreateSwapChainMutex);
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

/* Triggered by PVRSRVSwapToDCBuffer */
static IMG_BOOL ProcessFlipV1(IMG_HANDLE hCmdCookie,
							  DC_SUNXI_DEVINFO *psDevInfo,
							  DC_SUNXI_SWAPCHAIN *psSwapChain,
							  DC_SUNXI_BUFFER *psBuffer,
							  unsigned long ulSwapInterval)
{
	mutex_lock(&psDevInfo->sCreateSwapChainMutex);
	
	/* The swap chain has been destroyed */
	if (SwapChainHasChanged(psDevInfo, psSwapChain))
	{
		DEBUG_PRINTK((KERN_WARNING DRIVER_PREFIX
					  ": %s: Device %u (PVR Device ID %u): The swap chain has been destroyed\n",
					  __FUNCTION__, psDevInfo->uiFBDevID, psDevInfo->uiPVRDevID));
	}
	else
	{
#if defined(DC_SUNXI_DISPC_GRALLOC_QUEUE_IN_V1_PATH)
		/* Not enabled by default yet until we have a way to CmdComplete
		 * after vblank rather than after next programming!
		 */
		QueueBufferImmediate(psDevInfo, psBuffer->sSysAddr,
							 dispc_proxy_cmdcomplete, (void *)0xdeadbeef);

		/* When dispc_gralloc_queue() can call back after programming,
		 * this can be removed.
		 */
		psDevInfo->sPVRJTable.pfnPVRSRVCmdComplete(hCmdCookie, IMG_TRUE);
#else /* DC_SUNXI_DISPC_GRALLOC_QUEUE_IN_V1_PATH */
		int res;

		psBuffer->hCmdComplete = hCmdCookie;
		psBuffer->ulSwapInterval = ulSwapInterval;

		res = queue_work(psSwapChain->psWorkQueue, &psBuffer->sWork);
		if (res == 0)
		{
			printk(KERN_WARNING DRIVER_PREFIX
				   ": %s: Device %u: Buffer already on work queue\n",
				   __FUNCTION__, psSwapChain->uiFBDevID);
		}
#endif /* DC_SUNXI_DISPC_GRALLOC_QUEUE_IN_V1_PATH */
	}

	mutex_unlock(&psDevInfo->sCreateSwapChainMutex);
	return IMG_TRUE;
}

static IMG_BOOL ProcessFlipV2(IMG_HANDLE hCmdCookie,
							  DC_SUNXI_DEVINFO *psDevInfo,
							  PDC_MEM_INFO *ppsMemInfos,
							  IMG_UINT32 ui32NumMemInfos,
							  setup_dispc_data_t *psDispcData,
							  IMG_UINT32 ui32DispcDataLength)
{
	int i;

	if(!psDispcData)
	{
		if(ui32NumMemInfos == 1)
		{
			IMG_CPU_PHYADDR phyAddr;
			IMG_SYS_PHYADDR sSysAddr;

			psDevInfo->sPVRJTable.pfnPVRSRVDCMemInfoGetCpuPAddr(ppsMemInfos[0], 0, &phyAddr);
			sSysAddr.uiAddr = phyAddr.uiAddr;

			/* If we got a meminfo but no private data, assume the 'null' HWC
			 * backend is in use, and emulate a swapchain-less ProcessFlipV1.
			 */
			QueueBufferImmediate(psDevInfo, sSysAddr,
								 dispc_proxy_cmdcomplete, hCmdCookie);
		}
		else
		{
			printk(KERN_WARNING DRIVER_PREFIX
				   ": %s: Device %u: WARNING: psDispcData was NULL. "
				   "The HWC probably has a bug. Silently ignoring.",
				   __FUNCTION__, psDevInfo->uiFBDevID);
			gapsDevInfo[0]->sPVRJTable.pfnPVRSRVCmdComplete(hCmdCookie, IMG_TRUE);
		}

		return IMG_TRUE;
	}

	if(ui32DispcDataLength != sizeof(setup_dispc_data_t))
	{
		printk(KERN_WARNING DRIVER_PREFIX
			   ": %s: Device %u: WARNING: Unexpected private data size, %u vs %u.",
			   __FUNCTION__, psDevInfo->uiFBDevID, ui32DispcDataLength,
			   sizeof(setup_dispc_data_t));
		gapsDevInfo[0]->sPVRJTable.pfnPVRSRVCmdComplete(hCmdCookie, IMG_TRUE);
		return IMG_TRUE;
	}

	if(DontWaitForVSync(psDevInfo))
	{
		gapsDevInfo[0]->sPVRJTable.pfnPVRSRVCmdComplete(hCmdCookie, IMG_TRUE);
		return IMG_TRUE;
	}

	/* Maximum of 8 layer_infos. Meminfo array is dynamically sized */
	for(i = 0; i < ui32NumMemInfos && i < 8; i++)
	{
		IMG_CPU_PHYADDR phyAddr;

		psDevInfo->sPVRJTable.pfnPVRSRVDCMemInfoGetCpuPAddr(ppsMemInfos[i], 0, &phyAddr);

		psDispcData->layer_info[i].fb.addr[0] = phyAddr.uiAddr;
	}

	dispc_gralloc_queue(psDispcData, ui32DispcDataLength, dispc_proxy_cmdcomplete, (void *)hCmdCookie);

	return IMG_TRUE;
}

/* Command processing flip handler function.  Called from services. */
static IMG_BOOL ProcessFlip(IMG_HANDLE  hCmdCookie,
							IMG_UINT32  ui32DataSize,
							IMG_VOID   *pvData)
{
	DISPLAYCLASS_FLIP_COMMAND *psFlipCmd;
	DC_SUNXI_DEVINFO *psDevInfo;

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

	psDevInfo = (DC_SUNXI_DEVINFO*)psFlipCmd->hExtDevice;

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
		DISPLAYCLASS_FLIP_COMMAND2 *psFlipCmd2;
		psFlipCmd2 = (DISPLAYCLASS_FLIP_COMMAND2 *)pvData;
		return ProcessFlipV2(hCmdCookie,
							 psDevInfo,
							 psFlipCmd2->ppsMemInfos,
							 psFlipCmd2->ui32NumMemInfos,
							 psFlipCmd2->pvPrivData,
							 psFlipCmd2->ui32PrivDataLength);
	}
}

static DC_SUNXI_ERROR DC_SUNXIInitFBDev(DC_SUNXI_DEVINFO *psDevInfo)
{
	struct fb_info *psLINFBInfo;
	struct module *psLINFBOwner;
	DC_SUNXI_FBINFO *psPVRFBInfo = &psDevInfo->sFBInfo;
	DC_SUNXI_ERROR eError = DC_SUNXI_ERROR_GENERIC;
	unsigned int uiFBDevID = psDevInfo->uiFBDevID;

	console_lock();

	psLINFBInfo = registered_fb[uiFBDevID];
	if (psLINFBInfo == NULL)
	{
		eError = DC_SUNXI_ERROR_INVALID_DEVICE;
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
		eError = DC_SUNXI_ERROR_INVALID_DEVICE;
		goto ErrorFBRel;
	}

	psPVRFBInfo->ulFBSize = (psLINFBInfo->screen_size) != 0 ?
							psLINFBInfo->screen_size : psLINFBInfo->fix.smem_len;

	/*
	 * Try and filter out invalid FB info structures (a problem
	 * seen on some systems).
	 */
	if (psPVRFBInfo->ulFBSize == 0 || psLINFBInfo->fix.line_length == 0)
	{
		eError = DC_SUNXI_ERROR_INVALID_DEVICE;
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
	DEBUG_PRINTK((KERN_INFO DRIVER_PREFIX
				  ": Device %u: Framebuffer width: %lu\n",
				  psDevInfo->uiFBDevID, psPVRFBInfo->ulWidth));
	DEBUG_PRINTK((KERN_INFO DRIVER_PREFIX
				  ": Device %u: Framebuffer height: %lu\n",
				  psDevInfo->uiFBDevID, psPVRFBInfo->ulHeight));

	/* System Surface */
	psPVRFBInfo->sSysAddr.uiAddr = psLINFBInfo->fix.smem_start;
	psPVRFBInfo->sCPUVAddr = psLINFBInfo->screen_base;

	psPVRFBInfo->ulByteStride =  psLINFBInfo->fix.line_length;

	printk(KERN_WARNING 
		   "#####: Device %u: Framebuffer physical address: 0x%x\n",
		   psDevInfo->uiFBDevID, psPVRFBInfo->sSysAddr.uiAddr);

	if (psPVRFBInfo->sCPUVAddr != NULL)
	{
		DEBUG_PRINTK((KERN_WARNING DRIVER_PREFIX
		": Device %u: Framebuffer virtual address: %p\n",
		psDevInfo->uiFBDevID, psPVRFBInfo->sCPUVAddr));
	}

	DEBUG_PRINTK((KERN_WARNING DRIVER_PREFIX
				  ": Device %u: Framebuffer stride: %lu\n",
				  psDevInfo->uiFBDevID, psPVRFBInfo->ulByteStride));

	psPVRFBInfo->ulBufferSize = psPVRFBInfo->ulHeight * psPVRFBInfo->ulByteStride;

	{
		unsigned long ulLCM;
		ulLCM = LCM(psPVRFBInfo->ulByteStride, PAGE_SIZE);

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

	eError = DC_SUNXI_OK;
	goto ErrorRelSem;

ErrorFBRel:
	if (psLINFBInfo->fbops->fb_release != NULL) 
	{
		(void) psLINFBInfo->fbops->fb_release(psLINFBInfo, 0);
	}
ErrorModPut:
	module_put(psLINFBOwner);
ErrorRelSem:
	console_unlock();

	return eError;
}

static void DC_SUNXIDeInitFBDev(DC_SUNXI_DEVINFO *psDevInfo)
{
	struct fb_info *psLINFBInfo = psDevInfo->psLINFBInfo;
	struct module *psLINFBOwner;

	console_lock();

	psLINFBOwner = psLINFBInfo->fbops->owner;

	if (psLINFBInfo->fbops->fb_release != NULL) 
	{
		(void) psLINFBInfo->fbops->fb_release(psLINFBInfo, 0);
	}

	module_put(psLINFBOwner);

	console_unlock();
}

static DC_SUNXI_DEVINFO *DC_SUNXIInitDev(unsigned uiFBDevID)
{
	PFN_CMD_PROC   pfnCmdProcList[DC_SUNXI_COMMAND_COUNT];
	IMG_UINT32 aui32SyncCountList[DC_SUNXI_COMMAND_COUNT][2];
	DC_SUNXI_DEVINFO *psDevInfo = NULL;

	/* Allocate device info. structure */
	psDevInfo = (DC_SUNXI_DEVINFO *)kmalloc(sizeof(DC_SUNXI_DEVINFO), GFP_KERNEL);

	if(psDevInfo == NULL)
	{
		printk(KERN_ERR DRIVER_PREFIX
		": %s: Device %u: Couldn't allocate device information structure\n", __FUNCTION__, uiFBDevID);
		
		goto ErrorExit;
	}

	/* Any fields not set will be zero */
	memset(psDevInfo, 0, sizeof(DC_SUNXI_DEVINFO));

	psDevInfo->uiFBDevID = uiFBDevID;

	/* Get the kernel services function table */
	if(!(*gpfnGetPVRJTable)(&psDevInfo->sPVRJTable))
	{
		goto ErrorFreeDevInfo;
	}

	/* Save private fbdev information structure in the dev. info. */
	if(DC_SUNXIInitFBDev(psDevInfo) != DC_SUNXI_OK)
	{
		/*
		 * Leave it to DC_SUNXIInitFBDev to print an error message, if
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

	DC_SUNXIInitBufferForSwap(&psDevInfo->sSystemBuffer);

	/*
	 *		Setup the DC Jtable so SRVKM can call into this driver
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

	DEBUG_PRINTK((KERN_INFO DRIVER_PREFIX ": Device %u: PVR Device ID: %u\n",
				  psDevInfo->uiFBDevID, psDevInfo->uiPVRDevID));

	/* Setup private command processing function table ... */
	pfnCmdProcList[DC_FLIP_COMMAND] = ProcessFlip;

	/* ... and associated sync count(s) */
	aui32SyncCountList[DC_FLIP_COMMAND][0] = 0; /* writes */
	aui32SyncCountList[DC_FLIP_COMMAND][1] = 10; /* reads */

	if (psDevInfo->sPVRJTable.pfnPVRSRVRegisterCmdProcList(psDevInfo->uiPVRDevID,
		&pfnCmdProcList[0],
		aui32SyncCountList,
		DC_SUNXI_COMMAND_COUNT) != PVRSRV_OK)
	{
		printk(KERN_ERR DRIVER_PREFIX
			   ": %s: Device %u: Couldn't register command processing functions with PVR Services\n", __FUNCTION__, uiFBDevID);
		goto ErrorUnregisterDevice;
	}

	mutex_init(&psDevInfo->sCreateSwapChainMutex);

	atomic_set(&psDevInfo->sBlankEvents, 0);
	atomic_set(&psDevInfo->sFlushCommands, false);

	return psDevInfo;

ErrorUnregisterDevice:
	(void)psDevInfo->sPVRJTable.pfnPVRSRVRemoveDCDevice(psDevInfo->uiPVRDevID);
ErrorDeInitFBDev:
	DC_SUNXIDeInitFBDev(psDevInfo);
ErrorIonClientDestroy:
ErrorFreeDevInfo:
	kfree(psDevInfo);
ErrorExit:
	return NULL;
}

static DC_SUNXI_ERROR DC_SUNXIInit(void)
{
	unsigned int i, uiMaxFBDevIDPlusOne = DC_SUNXIMaxFBDevIDPlusOne();
	unsigned int uiDevicesFound = 0;

	gpfnGetPVRJTable = PVRGetDisplayClassJTable;

	/*
	 * We search for frame buffer devices backwards, as the last device
	 * registered with PVR Services will be the first device enumerated
	 * by PVR Services.
	 */
	for(i = uiMaxFBDevIDPlusOne; i-- != 0;)
	{
		DC_SUNXI_DEVINFO *psDevInfo = DC_SUNXIInitDev(i);

		if (psDevInfo != NULL)
		{
			/* Set the top-level anchor */
			DC_SUNXISetDevInfoPtr(psDevInfo->uiFBDevID, psDevInfo);
			uiDevicesFound++;
		}
	}

	return (uiDevicesFound != 0) ? DC_SUNXI_OK : DC_SUNXI_ERROR_INIT_FAILURE;
}

static bool DC_SUNXIDeInitDev(DC_SUNXI_DEVINFO *psDevInfo)
{
	PVRSRV_DC_DISP2SRV_KMJTABLE *psPVRJTable = &psDevInfo->sPVRJTable;

	if (psPVRJTable->pfnPVRSRVRemoveCmdProcList (psDevInfo->uiPVRDevID, DC_SUNXI_COMMAND_COUNT) != PVRSRV_OK)
	{
		printk(KERN_ERR DRIVER_PREFIX
			   ": %s: Device %u: PVR Device %u: Couldn't unregister command processing functions\n", __FUNCTION__, psDevInfo->uiFBDevID, psDevInfo->uiPVRDevID);
		return false;
	}

	if (psPVRJTable->pfnPVRSRVRemoveDCDevice(psDevInfo->uiPVRDevID) != PVRSRV_OK)
	{
		printk(KERN_ERR DRIVER_PREFIX
			   ": %s: Device %u: PVR Device %u: Couldn't remove device from PVR Services\n", __FUNCTION__, psDevInfo->uiFBDevID, psDevInfo->uiPVRDevID);
		return false;
	}

	mutex_destroy(&psDevInfo->sCreateSwapChainMutex);

	DC_SUNXIDeInitFBDev(psDevInfo);

	DC_SUNXISetDevInfoPtr(psDevInfo->uiFBDevID, NULL);

	/* De-allocate data structure */
	kfree(psDevInfo);

	return true;
}

static DC_SUNXI_ERROR DC_SUNXIDeInit(void)
{
	unsigned int i, uiMaxFBDevIDPlusOne = DC_SUNXIMaxFBDevIDPlusOne();
	bool bError = false;

	for(i = 0; i < uiMaxFBDevIDPlusOne; i++)
	{
		DC_SUNXI_DEVINFO *psDevInfo = DC_SUNXIGetDevInfoPtr(i);
		
		if (psDevInfo != NULL)
		{
			bError |= !DC_SUNXIDeInitDev(psDevInfo);
		}
	}

	return (bError) ? DC_SUNXI_ERROR_INIT_FAILURE : DC_SUNXI_OK;
}

#if defined(SUPPORT_DRI_DRM)
int PVR_DRM_MAKENAME(DISPLAY_CONTROLLER, _Init)(struct drm_device unref__ *dev)
#else
static int __init DC_SUNXI_Init(void)
#endif
{
	if(DC_SUNXIInit() != DC_SUNXI_OK)
	{
		printk(KERN_ERR DRIVER_PREFIX ": %s: DC_SUNXIInit failed\n", __FUNCTION__);
		return -ENODEV;
	}

	return 0;
}

#if defined(SUPPORT_DRI_DRM)
void PVR_DRM_MAKENAME(DISPLAY_CONTROLLER, _Cleanup)(struct drm_device unref__ *dev)
#else
static void __exit DC_SUNXI_Cleanup(void)
#endif
{
	if(DC_SUNXIDeInit() != DC_SUNXI_OK)
	{
		printk(KERN_ERR DRIVER_PREFIX ": %s: DC_SUNXIDeInit failed\n", __FUNCTION__);
	}
}

#if !defined(SUPPORT_DRI_DRM)
late_initcall(DC_SUNXI_Init);
module_exit(DC_SUNXI_Cleanup);
#endif
/******************************************************************************
 * End of file (dc_sunxi_displayclass.c)
 ******************************************************************************/
