/*************************************************************************/ /*!
@Title          OMAP Linux display driver structures and prototypes
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
#ifndef __OMAPLFB_H__
#define __OMAPLFB_H__

#include <linux/version.h>

#include <asm/atomic.h>

#include <linux/kernel.h>
#include <linux/console.h>
#include <linux/fb.h>
#include <linux/module.h>
#include <linux/string.h>
#include <linux/notifier.h>
#include <linux/mutex.h>

#ifdef CONFIG_HAS_EARLYSUSPEND
#include <linux/earlysuspend.h>
#endif

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,38))
#define	OMAPLFB_CONSOLE_LOCK()		console_lock()
#define	OMAPLFB_CONSOLE_UNLOCK()	console_unlock()
#else
#define	OMAPLFB_CONSOLE_LOCK()		acquire_console_sem()
#define	OMAPLFB_CONSOLE_UNLOCK()	release_console_sem()
#endif

#if defined(CONFIG_ION_OMAP)
#include <linux/ion.h>
#include <linux/omap_ion.h>
#endif /* defined(CONFIG_ION_OMAP) */

#define unref__ __attribute__ ((unused))

typedef void *       OMAPLFB_HANDLE;

typedef bool OMAPLFB_BOOL, *OMAPLFB_PBOOL;
#define	OMAPLFB_FALSE false
#define OMAPLFB_TRUE true

typedef	atomic_t	OMAPLFB_ATOMIC_BOOL;

typedef atomic_t	OMAPLFB_ATOMIC_INT;

/* OMAPLFB buffer structure */
typedef struct OMAPLFB_BUFFER_TAG
{
	struct OMAPLFB_BUFFER_TAG	*psNext;
	struct OMAPLFB_DEVINFO_TAG	*psDevInfo;

	struct work_struct sWork;

	/* Position of this buffer in the virtual framebuffer */
	unsigned long		     	ulYOffset;

	/* IMG structures used, to minimise API function code */
	/* replace with own structures where necessary */
	IMG_SYS_PHYADDR              	sSysAddr;
	IMG_CPU_VIRTADDR             	sCPUVAddr;
	PVRSRV_SYNC_DATA            	*psSyncData;

	OMAPLFB_HANDLE      		hCmdComplete;
	unsigned long    		ulSwapInterval;
} OMAPLFB_BUFFER;

/* OMAPLFB swapchain structure */
typedef struct OMAPLFB_SWAPCHAIN_TAG
{
	/* Swap chain ID */
	unsigned int			uiSwapChainID;

	/* number of buffers in swapchain */
	unsigned long       		ulBufferCount;

	/* list of buffers in the swapchain */
	OMAPLFB_BUFFER     		*psBuffer;

	/* Swap chain work queue */
	struct workqueue_struct   	*psWorkQueue;

	/*
	 * Set if we didn't manage to wait for VSync on last swap,
	 * or if we think we need to wait for VSync on the next flip.
	 * The flag helps to avoid jitter when the screen is
	 * unblanked, by forcing an extended wait for VSync before
	 * attempting the next flip.
	 */
	OMAPLFB_BOOL			bNotVSynced;

	/* Previous number of blank events */
	int				iBlankEvents;

	/* Framebuffer Device ID for messages (e.g. printk) */
	unsigned int            	uiFBDevID;
} OMAPLFB_SWAPCHAIN;

typedef struct OMAPLFB_FBINFO_TAG
{
	unsigned long       ulFBSize;
	unsigned long       ulBufferSize;
	unsigned long       ulRoundedBufferSize;
	unsigned long       ulWidth;
	unsigned long       ulHeight;
	unsigned long       ulByteStride;
	unsigned long       ulPhysicalWidthmm;
	unsigned long       ulPhysicalHeightmm;

	/* IMG structures used, to minimise API function code */
	/* replace with own structures where necessary */
	IMG_SYS_PHYADDR     sSysAddr;//system physical address
	IMG_CPU_VIRTADDR    sCPUVAddr;

	/* pixelformat of system/primary surface */
	PVRSRV_PIXEL_FORMAT ePixelFormat;

#if defined(CONFIG_DSSCOMP)
	OMAPLFB_BOOL		bIs2D;
	IMG_SYS_PHYADDR		*psPageList;
	struct ion_handle	*psIONHandle;
	IMG_UINT32			uiBytesPerPixel;
#endif
} OMAPLFB_FBINFO;

/* kernel device information structure */
typedef struct OMAPLFB_DEVINFO_TAG
{
	/* Framebuffer Device ID */
	unsigned int            uiFBDevID;

	/* PVR Device ID */
	unsigned int            uiPVRDevID;

	/* Swapchain create/destroy mutex */
	struct mutex		sCreateSwapChainMutex;

	/* system surface info */
	OMAPLFB_BUFFER          sSystemBuffer;

	/* jump table into PVR services */
	PVRSRV_DC_DISP2SRV_KMJTABLE	sPVRJTable;
	
	/* jump table into DC */
	PVRSRV_DC_SRV2DISP_KMJTABLE	sDCJTable;

	/* fb info structure */
	OMAPLFB_FBINFO          sFBInfo;

	/* Only one swapchain supported by this device so hang it here */
	OMAPLFB_SWAPCHAIN      *psSwapChain;

	/* Swap chain ID */
	unsigned int		uiSwapChainID;

	/* True if PVR Services is flushing its command queues */
	OMAPLFB_ATOMIC_BOOL     sFlushCommands;

	/* pointer to linux frame buffer information structure */
	struct fb_info         *psLINFBInfo;

	/* Linux Framebuffer event notification block */
	struct notifier_block   sLINNotifBlock;

	/* IMG structures used, to minimise API function code */
	/* replace with own structures where necessary */

	/* Address of the surface being displayed */
	IMG_DEV_VIRTADDR	sDisplayDevVAddr;

	DISPLAY_INFO            sDisplayInfo;

	/* Display format */
	DISPLAY_FORMAT          sDisplayFormat;
	
	/* Display dimensions */
	DISPLAY_DIMS            sDisplayDim;

	/* True if screen is blanked */
	OMAPLFB_ATOMIC_BOOL	sBlanked;

	/* Number of blank/unblank events */
	OMAPLFB_ATOMIC_INT	sBlankEvents;

#ifdef CONFIG_HAS_EARLYSUSPEND
	/* Set by early suspend */
	OMAPLFB_ATOMIC_BOOL	sEarlySuspendFlag;

	struct early_suspend    sEarlySuspend;
#endif

#if defined(SUPPORT_DRI_DRM)
	OMAPLFB_ATOMIC_BOOL     sLeaveVT;
#endif

#if defined(CONFIG_ION_OMAP)
	struct ion_client      *psIONClient;
#endif

}  OMAPLFB_DEVINFO;

#define	OMAPLFB_PAGE_SIZE 4096

/* DEBUG only printk */
#ifdef	DEBUG
#define	DEBUG_PRINTK(x) printk x
#else
#define	DEBUG_PRINTK(x)
#endif

#define DISPLAY_DEVICE_NAME "PowerVR OMAP Linux Display Driver"
#define	DRVNAME	"omaplfb"
#define	DEVNAME	DRVNAME
#define	DRIVER_PREFIX DRVNAME

/*!
 *****************************************************************************
 * Error values
 *****************************************************************************/
typedef enum _OMAPLFB_ERROR_
{
	OMAPLFB_OK                             =  0,
	OMAPLFB_ERROR_GENERIC                  =  1,
	OMAPLFB_ERROR_OUT_OF_MEMORY            =  2,
	OMAPLFB_ERROR_TOO_FEW_BUFFERS          =  3,
	OMAPLFB_ERROR_INVALID_PARAMS           =  4,
	OMAPLFB_ERROR_INIT_FAILURE             =  5,
	OMAPLFB_ERROR_CANT_REGISTER_CALLBACK   =  6,
	OMAPLFB_ERROR_INVALID_DEVICE           =  7,
	OMAPLFB_ERROR_DEVICE_REGISTER_FAILED   =  8,
	OMAPLFB_ERROR_SET_UPDATE_MODE_FAILED   =  9
} OMAPLFB_ERROR;

typedef enum _OMAPLFB_UPDATE_MODE_
{
	OMAPLFB_UPDATE_MODE_UNDEFINED			= 0,
	OMAPLFB_UPDATE_MODE_MANUAL			= 1,
	OMAPLFB_UPDATE_MODE_AUTO			= 2,
	OMAPLFB_UPDATE_MODE_DISABLED			= 3,
	OMAPLFB_UPDATE_MODE_VSYNC			= 4
} OMAPLFB_UPDATE_MODE;

#ifndef UNREFERENCED_PARAMETER
#define	UNREFERENCED_PARAMETER(param) (param) = (param)
#endif

OMAPLFB_ERROR OMAPLFBInit(void);
OMAPLFB_ERROR OMAPLFBDeInit(void);

/* OS Specific APIs */
OMAPLFB_DEVINFO *OMAPLFBGetDevInfoPtr(unsigned uiFBDevID);
unsigned OMAPLFBMaxFBDevIDPlusOne(void);
void *OMAPLFBAllocKernelMem(unsigned long ulSize);
void OMAPLFBFreeKernelMem(void *pvMem);
OMAPLFB_ERROR OMAPLFBGetLibFuncAddr(char *szFunctionName, PFN_DC_GET_PVRJTABLE *ppfnFuncTable);
OMAPLFB_ERROR OMAPLFBCreateSwapQueue (OMAPLFB_SWAPCHAIN *psSwapChain);
void OMAPLFBDestroySwapQueue(OMAPLFB_SWAPCHAIN *psSwapChain);
void OMAPLFBInitBufferForSwap(OMAPLFB_BUFFER *psBuffer);
void OMAPLFBSwapHandler(OMAPLFB_BUFFER *psBuffer);
void OMAPLFBQueueBufferForSwap(OMAPLFB_SWAPCHAIN *psSwapChain, OMAPLFB_BUFFER *psBuffer);
void OMAPLFBFlip(OMAPLFB_DEVINFO *psDevInfo, OMAPLFB_BUFFER *psBuffer);
OMAPLFB_UPDATE_MODE OMAPLFBGetUpdateMode(OMAPLFB_DEVINFO *psDevInfo);
OMAPLFB_BOOL OMAPLFBSetUpdateMode(OMAPLFB_DEVINFO *psDevInfo, OMAPLFB_UPDATE_MODE eMode);
OMAPLFB_BOOL OMAPLFBWaitForVSync(OMAPLFB_DEVINFO *psDevInfo);
OMAPLFB_BOOL OMAPLFBManualSync(OMAPLFB_DEVINFO *psDevInfo);
OMAPLFB_BOOL OMAPLFBCheckModeAndSync(OMAPLFB_DEVINFO *psDevInfo);
OMAPLFB_ERROR OMAPLFBUnblankDisplay(OMAPLFB_DEVINFO *psDevInfo);
OMAPLFB_ERROR OMAPLFBEnableLFBEventNotification(OMAPLFB_DEVINFO *psDevInfo);
OMAPLFB_ERROR OMAPLFBDisableLFBEventNotification(OMAPLFB_DEVINFO *psDevInfo);
void OMAPLFBCreateSwapChainLockInit(OMAPLFB_DEVINFO *psDevInfo);
void OMAPLFBCreateSwapChainLockDeInit(OMAPLFB_DEVINFO *psDevInfo);
void OMAPLFBCreateSwapChainLock(OMAPLFB_DEVINFO *psDevInfo);
void OMAPLFBCreateSwapChainUnLock(OMAPLFB_DEVINFO *psDevInfo);
void OMAPLFBAtomicBoolInit(OMAPLFB_ATOMIC_BOOL *psAtomic, OMAPLFB_BOOL bVal);
void OMAPLFBAtomicBoolDeInit(OMAPLFB_ATOMIC_BOOL *psAtomic);
void OMAPLFBAtomicBoolSet(OMAPLFB_ATOMIC_BOOL *psAtomic, OMAPLFB_BOOL bVal);
OMAPLFB_BOOL OMAPLFBAtomicBoolRead(OMAPLFB_ATOMIC_BOOL *psAtomic);
void OMAPLFBAtomicIntInit(OMAPLFB_ATOMIC_INT *psAtomic, int iVal);
void OMAPLFBAtomicIntDeInit(OMAPLFB_ATOMIC_INT *psAtomic);
void OMAPLFBAtomicIntSet(OMAPLFB_ATOMIC_INT *psAtomic, int iVal);
int OMAPLFBAtomicIntRead(OMAPLFB_ATOMIC_INT *psAtomic);
void OMAPLFBAtomicIntInc(OMAPLFB_ATOMIC_INT *psAtomic);

#if defined(DEBUG)
void OMAPLFBPrintInfo(OMAPLFB_DEVINFO *psDevInfo);
#else
#define	OMAPLFBPrintInfo(psDevInfo)
#endif

#endif /* __OMAPLFB_H__ */

/******************************************************************************
 End of file (omaplfb.h)
******************************************************************************/

