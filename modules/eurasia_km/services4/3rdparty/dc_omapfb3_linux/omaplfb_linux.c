/*************************************************************************/ /*!
@Title          OMAP linux display driver components
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

#include <linux/version.h>

#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,38))
#ifndef AUTOCONF_INCLUDED
#include <linux/config.h>
#endif
#endif

#include <asm/atomic.h>

#if defined(SUPPORT_DRI_DRM)
#include <drm/drmP.h>
#else
#include <linux/module.h>
#endif

#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/hardirq.h>
#include <linux/mutex.h>
#include <linux/workqueue.h>
#include <linux/fb.h>
#include <linux/console.h>
#include <linux/omapfb.h>
#include <linux/mutex.h>

#if defined(PVR_OMAPLFB_DRM_FB)
#if (LINUX_VERSION_CODE < KERNEL_VERSION(3,0,0))
#include <plat/display.h>
#else
#include <video/omapdss.h>
#endif
#include <linux/omap_gpu.h>
#else	/* defined(PVR_OMAPLFB_DRM_FB) */
/* OmapZoom.org OMAP3 2.6.29 kernel tree	- Needs mach/vrfb.h
 * OmapZoom.org OMAP3 2.6.32 kernel tree	- No additional header required
 * OmapZoom.org OMAP4 2.6.33 kernel tree	- No additional header required
 * OmapZoom.org OMAP4 2.6.34 kernel tree	- Needs plat/vrfb.h
 * Sholes 2.6.32 kernel tree			- Needs plat/vrfb.h
 */
#if defined(SYS_OMAP5_UEVM)
#define PVR_OMAPFB3_OMAP5_UEVM
#endif

#if defined(PVR_OMAPFB3_OMAP5_UEVM)
#define PVR_OMAPFB3_NEEDS_VIDEO_OMAPVRFB_H
#else
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,34))
#define PVR_OMAPFB3_NEEDS_PLAT_VRFB_H
#endif
#endif

#if defined(PVR_OMAPFB3_NEEDS_VIDEO_OMAPVRFB_H)
#include <video/omapvrfb.h>
#else
#if defined(PVR_OMAPFB3_NEEDS_PLAT_VRFB_H)
#include <plat/vrfb.h>
#else
#if defined(PVR_OMAPFB3_NEEDS_MACH_VRFB_H)
#include <mach/vrfb.h>
#endif
#endif
#endif

#if defined(DEBUG)
#define	PVR_DEBUG DEBUG
#undef DEBUG
#endif
#include <omapfb/omapfb.h>
#undef DBG
#if defined(DEBUG)
#undef DEBUG
#endif
#if defined(PVR_DEBUG)
#define	DEBUG PVR_DEBUG
#undef PVR_DEBUG
#endif
#endif	/* defined(PVR_OMAPLFB_DRM_FB) */

#if defined(CONFIG_DSSCOMP)
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

#include "img_defs.h"
#include "servicesext.h"
#include "kerneldisplay.h"
#include "omaplfb.h"
#include "pvrmodule.h"
#if defined(SUPPORT_DRI_DRM)
#include "pvr_drm.h"
#include "3rdparty_dc_drm_shared.h"
#endif

#if !defined(PVR_LINUX_USING_WORKQUEUES)
#error "PVR_LINUX_USING_WORKQUEUES must be defined"
#endif

MODULE_SUPPORTED_DEVICE(DEVNAME);

#if !defined(PVR_OMAPLFB_DRM_FB)
#if defined(PVR_OMAPFB3_OMAP5_UEVM)
#define OMAP_DSS_DRIVER(drv, dev) struct omap_dss_driver *drv = (dev)->driver
#define OMAP_DSS_MANAGER(man, dev) struct omap_overlay_manager *man = (dev)->output->manager
#define	WAIT_FOR_VSYNC(man)	((man)->wait_for_vsync)
#else	/* defined(PVR_OMAPFB3_OMAP5_UEVM) */
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,34))
#define OMAP_DSS_DRIVER(drv, dev) struct omap_dss_driver *drv = (dev) != NULL ? (dev)->driver : NULL
#define OMAP_DSS_MANAGER(man, dev) struct omap_overlay_manager *man = (dev) != NULL ? (dev)->manager : NULL
#define	WAIT_FOR_VSYNC(man)	((man)->wait_for_vsync)
#else	/* (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,34)) */
#define OMAP_DSS_DRIVER(drv, dev) struct omap_dss_device *drv = (dev)
#define OMAP_DSS_MANAGER(man, dev) struct omap_dss_device *man = (dev)
#define	WAIT_FOR_VSYNC(man)	((man)->wait_vsync)
#endif	/* (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,34)) */
#endif	/* defined(PVR_OMAPFB3_OMAP5_UEVM) */
#endif	/* !defined(PVR_OMAPLFB_DRM_FB) */

void *OMAPLFBAllocKernelMem(unsigned long ulSize)
{
	return kmalloc(ulSize, GFP_KERNEL);
}

void OMAPLFBFreeKernelMem(void *pvMem)
{
	kfree(pvMem);
}

void OMAPLFBCreateSwapChainLockInit(OMAPLFB_DEVINFO *psDevInfo)
{
	mutex_init(&psDevInfo->sCreateSwapChainMutex);
}

void OMAPLFBCreateSwapChainLockDeInit(OMAPLFB_DEVINFO *psDevInfo)
{
	mutex_destroy(&psDevInfo->sCreateSwapChainMutex);
}

void OMAPLFBCreateSwapChainLock(OMAPLFB_DEVINFO *psDevInfo)
{
	mutex_lock(&psDevInfo->sCreateSwapChainMutex);
}

void OMAPLFBCreateSwapChainUnLock(OMAPLFB_DEVINFO *psDevInfo)
{
	mutex_unlock(&psDevInfo->sCreateSwapChainMutex);
}

void OMAPLFBAtomicBoolInit(OMAPLFB_ATOMIC_BOOL *psAtomic, OMAPLFB_BOOL bVal)
{
	atomic_set(psAtomic, (int)bVal);
}

void OMAPLFBAtomicBoolDeInit(OMAPLFB_ATOMIC_BOOL *psAtomic)
{
}

void OMAPLFBAtomicBoolSet(OMAPLFB_ATOMIC_BOOL *psAtomic, OMAPLFB_BOOL bVal)
{
	atomic_set(psAtomic, (int)bVal);
}

OMAPLFB_BOOL OMAPLFBAtomicBoolRead(OMAPLFB_ATOMIC_BOOL *psAtomic)
{
	return (OMAPLFB_BOOL)atomic_read(psAtomic);
}

void OMAPLFBAtomicIntInit(OMAPLFB_ATOMIC_INT *psAtomic, int iVal)
{
	atomic_set(psAtomic, iVal);
}

void OMAPLFBAtomicIntDeInit(OMAPLFB_ATOMIC_INT *psAtomic)
{
}

void OMAPLFBAtomicIntSet(OMAPLFB_ATOMIC_INT *psAtomic, int iVal)
{
	atomic_set(psAtomic, iVal);
}

int OMAPLFBAtomicIntRead(OMAPLFB_ATOMIC_INT *psAtomic)
{
	return atomic_read(psAtomic);
}

void OMAPLFBAtomicIntInc(OMAPLFB_ATOMIC_INT *psAtomic)
{
	atomic_inc(psAtomic);
}

OMAPLFB_ERROR OMAPLFBGetLibFuncAddr (char *szFunctionName, PFN_DC_GET_PVRJTABLE *ppfnFuncTable)
{
	if(strcmp("PVRGetDisplayClassJTable", szFunctionName) != 0)
	{
		return (OMAPLFB_ERROR_INVALID_PARAMS);
	}

	/* Nothing to do - should be exported from pvrsrv.ko */
	*ppfnFuncTable = PVRGetDisplayClassJTable;

	return (OMAPLFB_OK);
}

/* Inset a swap buffer into the swap chain work queue */
void OMAPLFBQueueBufferForSwap(OMAPLFB_SWAPCHAIN *psSwapChain, OMAPLFB_BUFFER *psBuffer)
{
	int res = queue_work(psSwapChain->psWorkQueue, &psBuffer->sWork);

	if (res == 0)
	{
		printk(KERN_WARNING DRIVER_PREFIX ": %s: Device %u: Buffer already on work queue\n", __FUNCTION__, psSwapChain->uiFBDevID);
	}
}

/* Process an item on a swap chain work queue */
static void WorkQueueHandler(struct work_struct *psWork)
{
	OMAPLFB_BUFFER *psBuffer = container_of(psWork, OMAPLFB_BUFFER, sWork);

	OMAPLFBSwapHandler(psBuffer);
}

/* Create a swap chain work queue */
OMAPLFB_ERROR OMAPLFBCreateSwapQueue(OMAPLFB_SWAPCHAIN *psSwapChain)
{
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,37))
#if (LINUX_VERSION_CODE == KERNEL_VERSION(2,6,37))
#define WQ_FREEZABLE WQ_FREEZEABLE
#endif
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
#else
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,36))
	psSwapChain->psWorkQueue = create_freezable_workqueue(DEVNAME);
#else
	/*
	 * Create a single-threaded, freezable, rt-prio workqueue.
	 * Such workqueues are frozen with user threads when a system
	 * suspends, before driver suspend entry points are called.
	 * This ensures this driver will not call into the Linux
	 * framebuffer driver after the latter is suspended.
	 */
	psSwapChain->psWorkQueue = __create_workqueue(DEVNAME, 1, 1, 1);
#endif
#endif
	if (psSwapChain->psWorkQueue == NULL)
	{
		printk(KERN_ERR DRIVER_PREFIX ": %s: Device %u: Couldn't create workqueue\n", __FUNCTION__, psSwapChain->uiFBDevID);

		return (OMAPLFB_ERROR_INIT_FAILURE);
	}

	return (OMAPLFB_OK);
}

/* Prepare buffer for insertion into a swap chain work queue */
void OMAPLFBInitBufferForSwap(OMAPLFB_BUFFER *psBuffer)
{
	INIT_WORK(&psBuffer->sWork, WorkQueueHandler);
}

/* Destroy a swap chain work queue */
void OMAPLFBDestroySwapQueue(OMAPLFB_SWAPCHAIN *psSwapChain)
{
	destroy_workqueue(psSwapChain->psWorkQueue);
}

/* Flip display to given buffer */
void OMAPLFBFlip(OMAPLFB_DEVINFO *psDevInfo, OMAPLFB_BUFFER *psBuffer)
{
	struct fb_var_screeninfo sFBVar;
	int res;

	if (!lock_fb_info(psDevInfo->psLINFBInfo))
	{
		DEBUG_PRINTK((KERN_WARNING DRIVER_PREFIX
			": %s: Device %u: Couldn't lock FB info\n", __FUNCTION__,  psDevInfo->uiFBDevID));
		return;
	}
	OMAPLFB_CONSOLE_LOCK();

	sFBVar = psDevInfo->psLINFBInfo->var;

	sFBVar.xoffset = 0;
	sFBVar.yoffset = psBuffer->ulYOffset;

#if defined(CONFIG_DSSCOMP)
	/*
	 * If flipping to a NULL buffer, blank the screen to prevent
	 * warnings/errors from the display subsystem.
	 */
	if (psBuffer->sSysAddr.uiAddr == 0)
	{
		struct omap_dss_device *psDSSDev = fb2display(psDevInfo->psLINFBInfo);
		OMAP_DSS_MANAGER(psDSSMan, psDSSDev);

		if (psDSSMan != NULL && psDSSMan->blank != NULL)
		{
			res = psDSSMan->blank(psDSSMan, false);
			if (res != 0)
			{
				DEBUG_PRINTK((KERN_WARNING DRIVER_PREFIX ": %s: Device %u: DSS manager blank call failed (Y Offset: %lu, Error: %d)\n", __FUNCTION__, psDevInfo->uiFBDevID, psBuffer->ulYOffset, res));
			}
		}
	}

	{
		/*
		 * If using DSSCOMP, we need to use dsscomp queuing for normal
		 * framebuffer updates, so that previously used overlays get
		 * automatically disabled, and manager gets dirtied.  We can
		 * do that because DSSCOMP takes ownership of all pipelines on
		 * a manager.
		 */
		struct fb_fix_screeninfo sFBFix = psDevInfo->psLINFBInfo->fix;
		struct dsscomp_setup_dispc_data d =
		{
			.num_ovls = 1,
			.num_mgrs = 1,
			.mgrs[0].alpha_blending = 1,
			.ovls[0] =
			{
				.cfg =
				{
					.win.w = sFBVar.xres,
					.win.h = sFBVar.yres,
					.crop.x = sFBVar.xoffset,
					.crop.y = sFBVar.yoffset,
					.crop.w = sFBVar.xres,
					.crop.h = sFBVar.yres,
					.width = sFBVar.xres_virtual,
					.height = sFBVar.yres_virtual,
					.stride = sFBFix.line_length,
					.enabled = (psBuffer->sSysAddr.uiAddr != 0),
					.global_alpha = 255,
				},
			},
		};

		/* do not map buffer into TILER1D as it is contiguous */
		struct tiler_pa_info *pas[] = { NULL };

		d.ovls[0].ba = (u32) psBuffer->sSysAddr.uiAddr;

		omapfb_mode_to_dss_mode(&sFBVar, &d.ovls[0].cfg.color_mode);

		res = dsscomp_gralloc_queue(&d, pas, true, NULL, NULL);
		if (res != 0)
		{
			DEBUG_PRINTK((KERN_WARNING DRIVER_PREFIX ": %s: Device %u: dsscomp_gralloc_queue failed (Y Offset: %lu, Error: %d)\n", __FUNCTION__, psDevInfo->uiFBDevID, psBuffer->ulYOffset, res));
		}
	}
#else /* defined(CONFIG_DSSCOMP) */
	{
		unsigned long ulYResVirtual = psBuffer->ulYOffset + sFBVar.yres;

		/*
		 * PVR_OMAPLFB_DONT_USE_FB_PAN_DISPLAY should be defined to 
		 * work around flipping problems seen with the Taal LCDs on
		 * Blaze.
		 * The work around is safe to use with other types of screen
		 * on Blaze (e.g. HDMI) and on other platforms (e.g. Panda
		 * board).
		 */
#if !defined(PVR_OMAPLFB_DONT_USE_FB_PAN_DISPLAY)
		/*
		 * Attempt to change the virtual screen resolution if it is too
		 * small.  Note that fb_set_var also pans the display.
		 */
		if (sFBVar.xres_virtual != sFBVar.xres || sFBVar.yres_virtual < ulYResVirtual)
#endif /* !defined(PVR_OMAPLFB_DONT_USE_FB_PAN_DISPLAY) */
		{
			sFBVar.xres_virtual = sFBVar.xres;
			sFBVar.yres_virtual = ulYResVirtual;

			sFBVar.activate = FB_ACTIVATE_NOW | FB_ACTIVATE_FORCE;

			res = fb_set_var(psDevInfo->psLINFBInfo, &sFBVar);
			if (res != 0)
			{
				printk(KERN_ERR DRIVER_PREFIX ": %s: Device %u: fb_set_var failed (Y Offset: %lu, Error: %d)\n", __FUNCTION__, psDevInfo->uiFBDevID, psBuffer->ulYOffset, res);
			}
		}
#if !defined(PVR_OMAPLFB_DONT_USE_FB_PAN_DISPLAY)
		else
		{
			res = fb_pan_display(psDevInfo->psLINFBInfo, &sFBVar);
			if (res != 0)
			{
				printk(KERN_ERR DRIVER_PREFIX ": %s: Device %u: fb_pan_display failed (Y Offset: %lu, Error: %d)\n", __FUNCTION__, psDevInfo->uiFBDevID, psBuffer->ulYOffset, res);
			}
		}
#endif /* !defined(PVR_OMAPLFB_DONT_USE_FB_PAN_DISPLAY) */
	}
#endif /* defined(CONFIG_DSSCOMP) */

	OMAPLFB_CONSOLE_UNLOCK();
	unlock_fb_info(psDevInfo->psLINFBInfo);
}

/* Newer kernels don't have any update mode capability */

#if (LINUX_VERSION_CODE < KERNEL_VERSION(3,4,0))
#define	PVR_OMAPLFB_HAS_UPDATE_MODE
#endif

#if defined(PVR_OMAPLFB_HAS_UPDATE_MODE)
#if !defined(PVR_OMAPLFB_DRM_FB) || defined(DEBUG)
static OMAPLFB_BOOL OMAPLFBValidateDSSUpdateMode(enum omap_dss_update_mode eMode)
{
	switch (eMode)
	{
		case OMAP_DSS_UPDATE_AUTO:
		case OMAP_DSS_UPDATE_MANUAL:
		case OMAP_DSS_UPDATE_DISABLED:
			return OMAPLFB_TRUE;
		default:
			break;
	}

	return OMAPLFB_FALSE;
}

static OMAPLFB_UPDATE_MODE OMAPLFBFromDSSUpdateMode(enum omap_dss_update_mode eMode)
{
	switch (eMode)
	{
		case OMAP_DSS_UPDATE_AUTO:
			return OMAPLFB_UPDATE_MODE_AUTO;
		case OMAP_DSS_UPDATE_MANUAL:
			return OMAPLFB_UPDATE_MODE_MANUAL;
		case OMAP_DSS_UPDATE_DISABLED:
			return OMAPLFB_UPDATE_MODE_DISABLED;
		default:
			break;
	}

	return OMAPLFB_UPDATE_MODE_UNDEFINED;
}
#endif

static OMAPLFB_BOOL OMAPLFBValidateUpdateMode(OMAPLFB_UPDATE_MODE eMode)
{
	switch(eMode)
	{
		case OMAPLFB_UPDATE_MODE_AUTO:
		case OMAPLFB_UPDATE_MODE_MANUAL:
		case OMAPLFB_UPDATE_MODE_DISABLED:
			return OMAPLFB_TRUE;
		default:
			break;
	}

	return OMAPLFB_FALSE;
}

static enum omap_dss_update_mode OMAPLFBToDSSUpdateMode(OMAPLFB_UPDATE_MODE eMode)
{
	switch(eMode)
	{
		case OMAPLFB_UPDATE_MODE_AUTO:
			return OMAP_DSS_UPDATE_AUTO;
		case OMAPLFB_UPDATE_MODE_MANUAL:
			return OMAP_DSS_UPDATE_MANUAL;
		case OMAPLFB_UPDATE_MODE_DISABLED:
			return OMAP_DSS_UPDATE_DISABLED;
		default:
			break;
	}

	return -1;
}

#if defined(DEBUG)
static const char *OMAPLFBUpdateModeToString(OMAPLFB_UPDATE_MODE eMode)
{
	switch(eMode)
	{
		case OMAPLFB_UPDATE_MODE_AUTO:
			return "Auto Update Mode";
		case OMAPLFB_UPDATE_MODE_MANUAL:
			return "Manual Update Mode";
		case OMAPLFB_UPDATE_MODE_DISABLED:
			return "Update Mode Disabled";
		case OMAPLFB_UPDATE_MODE_UNDEFINED:
			return "Update Mode Undefined";
		default:
			break;
	}

	return "Unknown Update Mode";
}

static const char *OMAPLFBDSSUpdateModeToString(enum omap_dss_update_mode eMode)
{
	if (!OMAPLFBValidateDSSUpdateMode(eMode))
	{
		return "Unknown Update Mode";
	}

	return OMAPLFBUpdateModeToString(OMAPLFBFromDSSUpdateMode(eMode));
}
#endif /* defined(DEBUG) */

/* 
 * Get display update mode.
 * If the mode is AUTO, we can wait for VSync, if desired.
 */
OMAPLFB_UPDATE_MODE OMAPLFBGetUpdateMode(OMAPLFB_DEVINFO *psDevInfo)
{
#if defined(PVR_OMAPLFB_DRM_FB)
	struct drm_connector *psConnector;
	OMAPLFB_UPDATE_MODE eMode = OMAPLFB_UPDATE_MODE_UNDEFINED;

	/*
	 * There may be multiple displays connected. If at least one
	 * display is manual update mode, report all screens as being
	 * in that mode.
	 */
	for (psConnector = NULL;
		(psConnector = omap_fbdev_get_next_connector(psDevInfo->psLINFBInfo, psConnector)) != NULL;)
	{
		switch(omap_connector_get_update_mode(psConnector))
		{
			case OMAP_DSS_UPDATE_MANUAL:
				eMode = OMAPLFB_UPDATE_MODE_MANUAL;
				break;
			case OMAP_DSS_UPDATE_DISABLED:
				if (eMode == OMAPLFB_UPDATE_MODE_UNDEFINED)
				{
					eMode = OMAPLFB_UPDATE_MODE_DISABLED;
				}
				break;
			case OMAP_DSS_UPDATE_AUTO:
				/* Fall through to default case */
			default:
				/* Asssume auto update is possible */
				if (eMode != OMAPLFB_UPDATE_MODE_MANUAL)
				{
					eMode = OMAPLFB_UPDATE_MODE_AUTO;
				}
				break;
		}
	}

	return eMode;
#else	/* defined(PVR_OMAPLFB_DRM_FB) */
	struct omap_dss_device *psDSSDev = fb2display(psDevInfo->psLINFBInfo);
	OMAP_DSS_DRIVER(psDSSDrv, psDSSDev);

	enum omap_dss_update_mode eMode;

	if (psDSSDrv == NULL)
	{
		DEBUG_PRINTK((KERN_WARNING DRIVER_PREFIX ": %s: Device %u: No DSS device\n", __FUNCTION__, psDevInfo->uiFBDevID));
		return OMAPLFB_UPDATE_MODE_UNDEFINED;
	}

	if (psDSSDrv->get_update_mode == NULL)
	{
		if (strcmp(psDSSDev->name, "hdmi") == 0)
		{
			return OMAPLFB_UPDATE_MODE_AUTO;
		}
		DEBUG_PRINTK((KERN_WARNING DRIVER_PREFIX ": %s: Device %u: No get_update_mode function\n", __FUNCTION__, psDevInfo->uiFBDevID));
		return OMAPLFB_UPDATE_MODE_UNDEFINED;
	}

	eMode = psDSSDrv->get_update_mode(psDSSDev);
	if (!OMAPLFBValidateDSSUpdateMode(eMode))
	{
			DEBUG_PRINTK((KERN_WARNING DRIVER_PREFIX ": %s: Device %u: Unknown update mode (%d)\n", __FUNCTION__, psDevInfo->uiFBDevID, (int)eMode));
	}

	return OMAPLFBFromDSSUpdateMode(eMode);
#endif	/* defined(PVR_OMAPLFB_DRM_FB) */
}

/* Set display update mode */
OMAPLFB_BOOL OMAPLFBSetUpdateMode(OMAPLFB_DEVINFO *psDevInfo, OMAPLFB_UPDATE_MODE eMode)
{
#if defined(PVR_OMAPLFB_DRM_FB)
	struct drm_connector *psConnector;
	enum omap_dss_update_mode eDSSMode;
	OMAPLFB_BOOL bSuccess = OMAPLFB_FALSE;
	OMAPLFB_BOOL bFailure = OMAPLFB_FALSE;

	if (!OMAPLFBValidateUpdateMode(eMode))
	{
			DEBUG_PRINTK((KERN_WARNING DRIVER_PREFIX ": %s: Device %u: Unknown update mode (%d)\n", __FUNCTION__, psDevInfo->uiFBDevID, (int)eMode));
			return OMAPLFB_FALSE;
	}
	eDSSMode = OMAPLFBToDSSUpdateMode(eMode);

	for (psConnector = NULL;
		(psConnector = omap_fbdev_get_next_connector(psDevInfo->psLINFBInfo, psConnector)) != NULL;)
	{
		int iRes = omap_connector_set_update_mode(psConnector, eDSSMode);
		OMAPLFB_BOOL bRes = (iRes == 0);


		bSuccess |= bRes;
		bFailure |= !bRes;
	}

	if (!bFailure)
	{
		if (!bSuccess)
		{
			DEBUG_PRINTK((KERN_WARNING DRIVER_PREFIX ": %s: Device %u: No screens\n", __FUNCTION__, psDevInfo->uiFBDevID));
		}

		return OMAPLFB_TRUE;
	}

	if (!bSuccess)
	{
		DEBUG_PRINTK((KERN_WARNING DRIVER_PREFIX ": %s: Device %u: Couldn't set %s for any screen\n", __FUNCTION__, psDevInfo->uiFBDevID, OMAPLFBUpdateModeToString(eMode)));
		return OMAPLFB_FALSE;
	}

	if (eMode == OMAPLFB_UPDATE_MODE_AUTO)
	{
		DEBUG_PRINTK((KERN_WARNING DRIVER_PREFIX ": %s: Device %u: Couldn't set %s for all screens\n", __FUNCTION__, psDevInfo->uiFBDevID, OMAPLFBUpdateModeToString(eMode)));
		return OMAPLFB_FALSE;
	}

	DEBUG_PRINTK((KERN_INFO DRIVER_PREFIX ": %s: Device %u: %s set for some screens\n", __FUNCTION__, psDevInfo->uiFBDevID, OMAPLFBUpdateModeToString(eMode)));

	return OMAPLFB_TRUE;
#else	/* defined(PVR_OMAPLFB_DRM_FB) */
	struct omap_dss_device *psDSSDev = fb2display(psDevInfo->psLINFBInfo);
	OMAP_DSS_DRIVER(psDSSDrv, psDSSDev);
	enum omap_dss_update_mode eDSSMode;
	int res;

	if (psDSSDrv == NULL || psDSSDrv->set_update_mode == NULL)
	{
		DEBUG_PRINTK((KERN_WARNING DRIVER_PREFIX ": %s: Device %u: Can't set update mode\n", __FUNCTION__, psDevInfo->uiFBDevID));
		return OMAPLFB_FALSE;
	}

	if (!OMAPLFBValidateUpdateMode(eMode))
	{
			DEBUG_PRINTK((KERN_WARNING DRIVER_PREFIX ": %s: Device %u: Unknown update mode (%d)\n", __FUNCTION__, psDevInfo->uiFBDevID, (int)eMode));
			return OMAPLFB_FALSE;
	}
	eDSSMode = OMAPLFBToDSSUpdateMode(eMode);

	res = psDSSDrv->set_update_mode(psDSSDev, eDSSMode);
	if (res != 0)
	{
		DEBUG_PRINTK((KERN_WARNING DRIVER_PREFIX ": %s: Device %u: set_update_mode (%s) failed (%d)\n", __FUNCTION__, psDevInfo->uiFBDevID, OMAPLFBDSSUpdateModeToString(eDSSMode), res));
	}

	return (res == 0);
#endif	/* defined(PVR_OMAPLFB_DRM_FB) */
}
#else /* defined(PVR_OMAPLFB_HAS_UPDATE_MODE) */

OMAPLFB_UPDATE_MODE OMAPLFBGetUpdateMode(OMAPLFB_DEVINFO *psDevInfo)
{
#if defined(PVR_OMAPFB3_OMAP5_UEVM)
	return OMAPLFB_UPDATE_MODE_VSYNC;
#else
	return OMAPLFB_UPDATE_MODE_UNDEFINED;
#endif
}

#endif /* defined(PVR_OMAPLFB_HAS_UPDATE_MODE) */

#if defined(DEBUG)
void OMAPLFBPrintInfo(OMAPLFB_DEVINFO *psDevInfo)
{
#if defined(PVR_OMAPLFB_DRM_FB)
	struct drm_connector *psConnector;
	unsigned uConnectors;
	unsigned uConnector;

	DEBUG_PRINTK((KERN_INFO DRIVER_PREFIX ": Device %u: DRM framebuffer\n", psDevInfo->uiFBDevID));

	for (psConnector = NULL, uConnectors = 0;
		(psConnector = omap_fbdev_get_next_connector(psDevInfo->psLINFBInfo, psConnector)) != NULL;)
	{
		uConnectors++;
	}

	DEBUG_PRINTK((KERN_INFO DRIVER_PREFIX ": Device %u: Number of screens (DRM connectors): %u\n", psDevInfo->uiFBDevID, uConnectors));

	if (uConnectors == 0)
	{
		return;
	}

	for (psConnector = NULL, uConnector = 0;
		(psConnector = omap_fbdev_get_next_connector(psDevInfo->psLINFBInfo, psConnector)) != NULL; uConnector++)
	{
		enum omap_dss_update_mode eMode = omap_connector_get_update_mode(psConnector);

		DEBUG_PRINTK((KERN_INFO DRIVER_PREFIX ": Device %u: Screen %u: %s (%d)\n", psDevInfo->uiFBDevID, uConnector, OMAPLFBDSSUpdateModeToString(eMode), (int)eMode));

	}
#else	/* defined(PVR_OMAPLFB_DRM_FB) */
#if defined(PVR_OMAPLFB_HAS_UPDATE_MODE)
	OMAPLFB_UPDATE_MODE eMode = OMAPLFBGetUpdateMode(psDevInfo);

	DEBUG_PRINTK((KERN_INFO DRIVER_PREFIX ": Device %u: %s\n", psDevInfo->uiFBDevID, OMAPLFBUpdateModeToString(eMode)));
#endif
	DEBUG_PRINTK((KERN_INFO DRIVER_PREFIX ": Device %u: non-DRM framebuffer\n", psDevInfo->uiFBDevID));

#endif	/* defined(PVR_OMAPLFB_DRM_FB) */
}
#endif	/* defined(DEBUG) */

/* Wait for VSync */
OMAPLFB_BOOL OMAPLFBWaitForVSync(OMAPLFB_DEVINFO *psDevInfo)
{
#if defined(PVR_OMAPLFB_DRM_FB)
	struct drm_connector *psConnector;

	for (psConnector = NULL;
		(psConnector = omap_fbdev_get_next_connector(psDevInfo->psLINFBInfo, psConnector)) != NULL;)
	{
		(void) omap_encoder_wait_for_vsync(psConnector->encoder);
	}

	return OMAPLFB_TRUE;
#else	/* defined(PVR_OMAPLFB_DRM_FB) */
	struct omap_dss_device *psDSSDev = fb2display(psDevInfo->psLINFBInfo);
	OMAP_DSS_MANAGER(psDSSMan, psDSSDev);

	if (psDSSMan != NULL && WAIT_FOR_VSYNC(psDSSMan) != NULL)
	{
		int res = WAIT_FOR_VSYNC(psDSSMan)(psDSSMan);
		if (res != 0)
		{
			DEBUG_PRINTK((KERN_WARNING DRIVER_PREFIX ": %s: Device %u: Wait for vsync failed (%d)\n", __FUNCTION__, psDevInfo->uiFBDevID, res));
			return OMAPLFB_FALSE;
		}
	}

	return OMAPLFB_TRUE;
#endif	/* defined(PVR_OMAPLFB_DRM_FB) */
}

/*
 * Wait for screen to update.  If the screen is in manual or auto update
 * mode, we can call this function to wait for the screen to update.
 */
OMAPLFB_BOOL OMAPLFBManualSync(OMAPLFB_DEVINFO *psDevInfo)
{
#if defined(PVR_OMAPLFB_DRM_FB)
	struct drm_connector *psConnector;

	for (psConnector = NULL;
		(psConnector = omap_fbdev_get_next_connector(psDevInfo->psLINFBInfo, psConnector)) != NULL; )
	{
		/* Try manual sync first, then try wait for vsync */
		if (omap_connector_sync(psConnector) != 0)
		{
			(void) omap_encoder_wait_for_vsync(psConnector->encoder);
		}
	}

	return OMAPLFB_TRUE;
#else	/* defined(PVR_OMAPLFB_DRM_FB) */
	struct omap_dss_device *psDSSDev = fb2display(psDevInfo->psLINFBInfo);
	OMAP_DSS_DRIVER(psDSSDrv, psDSSDev);

	if (psDSSDrv != NULL && psDSSDrv->sync != NULL)
	{
		int res = psDSSDrv->sync(psDSSDev);
		if (res != 0)
		{
			printk(KERN_ERR DRIVER_PREFIX ": %s: Device %u: Sync failed (%d)\n", __FUNCTION__, psDevInfo->uiFBDevID, res);
			return OMAPLFB_FALSE;
		}
	}

	return OMAPLFB_TRUE;
#endif	/* defined(PVR_OMAPLFB_DRM_FB) */
}

/*
 * If the screen is manual or auto update mode, wait for the screen to
 * update.
 */
OMAPLFB_BOOL OMAPLFBCheckModeAndSync(OMAPLFB_DEVINFO *psDevInfo)
{
	OMAPLFB_UPDATE_MODE eMode = OMAPLFBGetUpdateMode(psDevInfo);

	switch(eMode)
	{
		case OMAPLFB_UPDATE_MODE_AUTO:
		case OMAPLFB_UPDATE_MODE_MANUAL:
			return OMAPLFBManualSync(psDevInfo);
		default:
			break;
	}

	return OMAPLFB_TRUE;
}

/* Linux Framebuffer event notification handler */
static int OMAPLFBFrameBufferEvents(struct notifier_block *psNotif,
                             unsigned long event, void *data)
{
	OMAPLFB_DEVINFO *psDevInfo;
	struct fb_event *psFBEvent = (struct fb_event *)data;
	struct fb_info *psFBInfo = psFBEvent->info;
	OMAPLFB_BOOL bBlanked;

	/* Only interested in blanking events */
	if (event != FB_EVENT_BLANK)
	{
		return 0;
	}

	bBlanked = (*(IMG_INT *)psFBEvent->data != 0) ? OMAPLFB_TRUE: OMAPLFB_FALSE;

	psDevInfo = OMAPLFBGetDevInfoPtr(psFBInfo->node);

#if 0
	if (psDevInfo != NULL)
	{
		if (bBlanked)
		{
			DEBUG_PRINTK((KERN_INFO DRIVER_PREFIX ": %s: Device %u: Blank event received\n", __FUNCTION__, psDevInfo->uiFBDevID));
		}
		else
		{
			DEBUG_PRINTK((KERN_INFO DRIVER_PREFIX ": %s: Device %u: Unblank event received\n", __FUNCTION__, psDevInfo->uiFBDevID));
		}
	}
	else
	{
		DEBUG_PRINTK((KERN_INFO DRIVER_PREFIX ": %s: Device %u: Blank/Unblank event for unknown framebuffer\n", __FUNCTION__, psFBInfo->node));
	}
#endif

	if (psDevInfo != NULL)
	{
		OMAPLFBAtomicBoolSet(&psDevInfo->sBlanked, bBlanked);
		OMAPLFBAtomicIntInc(&psDevInfo->sBlankEvents);
	}

	return 0;
}

/*
 * Blank or Unblank the screen. To be called where the unblank is being done
 * in user context.
 */
static OMAPLFB_ERROR OMAPLFBBlankOrUnblankDisplay(OMAPLFB_DEVINFO *psDevInfo, IMG_BOOL bBlank)
{
	int res;

	if (!lock_fb_info(psDevInfo->psLINFBInfo))
	{
		printk(KERN_ERR DRIVER_PREFIX
			": %s: Device %u: Couldn't lock FB info\n", __FUNCTION__,  psDevInfo->uiFBDevID);
		return (OMAPLFB_ERROR_GENERIC);
	}

	/*
	 * FBINFO_MISC_USEREVENT is set to avoid a deadlock resulting from
	 * fb_blank being called recursively due from within the fb_blank event
	 * notification.
	 */
	OMAPLFB_CONSOLE_LOCK();
	psDevInfo->psLINFBInfo->flags |= FBINFO_MISC_USEREVENT;
	res = fb_blank(psDevInfo->psLINFBInfo, bBlank ? 1 : 0);
	psDevInfo->psLINFBInfo->flags &= ~FBINFO_MISC_USEREVENT;
	OMAPLFB_CONSOLE_UNLOCK();
	unlock_fb_info(psDevInfo->psLINFBInfo);
	if (res != 0 && res != -EINVAL)
	{
		printk(KERN_ERR DRIVER_PREFIX
			": %s: Device %u: fb_blank failed (%d)\n", __FUNCTION__, psDevInfo->uiFBDevID, res);
		return (OMAPLFB_ERROR_GENERIC);
	}

	return (OMAPLFB_OK);
}

/* Unblank the screen */
OMAPLFB_ERROR OMAPLFBUnblankDisplay(OMAPLFB_DEVINFO *psDevInfo)
{
	return OMAPLFBBlankOrUnblankDisplay(psDevInfo, IMG_FALSE);
}

#ifdef CONFIG_HAS_EARLYSUSPEND

/* Blank the screen */
static void OMAPLFBEarlyUnblankDisplay(OMAPLFB_DEVINFO *psDevInfo)
{
	OMAPLFB_CONSOLE_LOCK();
	fb_blank(psDevInfo->psLINFBInfo, 0);
	OMAPLFB_CONSOLE_UNLOCK();
}

static void OMAPLFBEarlyBlankDisplay(OMAPLFB_DEVINFO *psDevInfo)
{
	OMAPLFB_CONSOLE_LOCK();
	fb_blank(psDevInfo->psLINFBInfo, 1);
	OMAPLFB_CONSOLE_UNLOCK();
}

static void OMAPLFBEarlySuspendHandler(struct early_suspend *h)
{
	unsigned uiMaxFBDevIDPlusOne = OMAPLFBMaxFBDevIDPlusOne();
	unsigned i;

	for (i=0; i < uiMaxFBDevIDPlusOne; i++)
	{
		OMAPLFB_DEVINFO *psDevInfo = OMAPLFBGetDevInfoPtr(i);

		if (psDevInfo != NULL)
		{
			OMAPLFBAtomicBoolSet(&psDevInfo->sEarlySuspendFlag, OMAPLFB_TRUE);
			OMAPLFBEarlyBlankDisplay(psDevInfo);
		}
	}
}

static void OMAPLFBEarlyResumeHandler(struct early_suspend *h)
{
	unsigned uiMaxFBDevIDPlusOne = OMAPLFBMaxFBDevIDPlusOne();
	unsigned i;

	for (i=0; i < uiMaxFBDevIDPlusOne; i++)
	{
		OMAPLFB_DEVINFO *psDevInfo = OMAPLFBGetDevInfoPtr(i);

		if (psDevInfo != NULL)
		{
			OMAPLFBEarlyUnblankDisplay(psDevInfo);
			OMAPLFBAtomicBoolSet(&psDevInfo->sEarlySuspendFlag, OMAPLFB_FALSE);
		}
	}
}

#endif /* CONFIG_HAS_EARLYSUSPEND */

/* Set up Linux Framebuffer event notification */
OMAPLFB_ERROR OMAPLFBEnableLFBEventNotification(OMAPLFB_DEVINFO *psDevInfo)
{
	int                res;
	OMAPLFB_ERROR         eError;

	/* Set up Linux Framebuffer event notification */
	memset(&psDevInfo->sLINNotifBlock, 0, sizeof(psDevInfo->sLINNotifBlock));

	psDevInfo->sLINNotifBlock.notifier_call = OMAPLFBFrameBufferEvents;

	OMAPLFBAtomicBoolSet(&psDevInfo->sBlanked, OMAPLFB_FALSE);
	OMAPLFBAtomicIntSet(&psDevInfo->sBlankEvents, 0);

	res = fb_register_client(&psDevInfo->sLINNotifBlock);
	if (res != 0)
	{
		printk(KERN_ERR DRIVER_PREFIX
			": %s: Device %u: fb_register_client failed (%d)\n", __FUNCTION__, psDevInfo->uiFBDevID, res);

		return (OMAPLFB_ERROR_GENERIC);
	}

	eError = OMAPLFBUnblankDisplay(psDevInfo);
	if (eError != OMAPLFB_OK)
	{
		printk(KERN_ERR DRIVER_PREFIX
			": %s: Device %u: UnblankDisplay failed (%d)\n", __FUNCTION__, psDevInfo->uiFBDevID, eError);
		return eError;
	}

#ifdef CONFIG_HAS_EARLYSUSPEND
	psDevInfo->sEarlySuspend.suspend = OMAPLFBEarlySuspendHandler;
	psDevInfo->sEarlySuspend.resume = OMAPLFBEarlyResumeHandler;
	psDevInfo->sEarlySuspend.level = EARLY_SUSPEND_LEVEL_DISABLE_FB + 1;
	register_early_suspend(&psDevInfo->sEarlySuspend);
#endif

	return (OMAPLFB_OK);
}

/* Disable Linux Framebuffer event notification */
OMAPLFB_ERROR OMAPLFBDisableLFBEventNotification(OMAPLFB_DEVINFO *psDevInfo)
{
	int res;

#ifdef CONFIG_HAS_EARLYSUSPEND
	unregister_early_suspend(&psDevInfo->sEarlySuspend);
#endif

	/* Unregister for Framebuffer events */
	res = fb_unregister_client(&psDevInfo->sLINNotifBlock);
	if (res != 0)
	{
		printk(KERN_ERR DRIVER_PREFIX
			": %s: Device %u: fb_unregister_client failed (%d)\n", __FUNCTION__, psDevInfo->uiFBDevID, res);
		return (OMAPLFB_ERROR_GENERIC);
	}

	OMAPLFBAtomicBoolSet(&psDevInfo->sBlanked, OMAPLFB_FALSE);

	return (OMAPLFB_OK);
}

#if defined(SUPPORT_DRI_DRM) && defined(PVR_DISPLAY_CONTROLLER_DRM_IOCTL)
static OMAPLFB_DEVINFO *OMAPLFBPVRDevIDToDevInfo(unsigned uiPVRDevID)
{
	unsigned uiMaxFBDevIDPlusOne = OMAPLFBMaxFBDevIDPlusOne();
	unsigned i;

	for (i=0; i < uiMaxFBDevIDPlusOne; i++)
	{
		OMAPLFB_DEVINFO *psDevInfo = OMAPLFBGetDevInfoPtr(i);

		if (psDevInfo->uiPVRDevID == uiPVRDevID)
		{
			return psDevInfo;
		}
	}

	printk(KERN_ERR DRIVER_PREFIX
		": %s: PVR Device %u: Couldn't find device\n", __FUNCTION__, uiPVRDevID);

	return NULL;
}

int PVR_DRM_MAKENAME(DISPLAY_CONTROLLER, _Ioctl)(struct drm_device unref__ *dev, void *arg, struct drm_file unref__ *pFile)
{
	drm_pvr_display_cmd *psArgs;
	OMAPLFB_DEVINFO *psDevInfo;
	int ret = 0;

	if (arg == NULL)
	{
		return -EFAULT;
	}

	psArgs = (drm_pvr_display_cmd *)arg;

	psDevInfo = OMAPLFBPVRDevIDToDevInfo(psArgs->dev);
	if (psDevInfo == NULL)
	{
		return -EINVAL;
	}


	switch (psArgs->cmd)
	{
		case PVR_DRM_DISP_CMD_LEAVE_VT:
		case PVR_DRM_DISP_CMD_ENTER_VT:
		{
			OMAPLFB_BOOL bLeaveVT = (psArgs->cmd == PVR_DRM_DISP_CMD_LEAVE_VT);
			DEBUG_PRINTK((KERN_WARNING DRIVER_PREFIX ": %s: PVR Device %u: %s\n",
				__FUNCTION__, psArgs->dev,
				bLeaveVT ? "Leave VT" : "Enter VT"));

			OMAPLFBCreateSwapChainLock(psDevInfo);
			
			OMAPLFBAtomicBoolSet(&psDevInfo->sLeaveVT, bLeaveVT);
			if (psDevInfo->psSwapChain != NULL)
			{
				flush_workqueue(psDevInfo->psSwapChain->psWorkQueue);

				if (bLeaveVT)
				{
					OMAPLFBFlip(psDevInfo, &psDevInfo->sSystemBuffer);
					(void) OMAPLFBCheckModeAndSync(psDevInfo);
				}
			}

			OMAPLFBCreateSwapChainUnLock(psDevInfo);
			(void) OMAPLFBUnblankDisplay(psDevInfo);
			break;
		}
		case PVR_DRM_DISP_CMD_ON:
		case PVR_DRM_DISP_CMD_STANDBY:
		case PVR_DRM_DISP_CMD_SUSPEND:
		case PVR_DRM_DISP_CMD_OFF:
		{
			int iFBMode;
#if defined(DEBUG)
			{
				const char *pszMode;
				switch(psArgs->cmd)
				{
					case PVR_DRM_DISP_CMD_ON:
						pszMode = "On";
						break;
					case PVR_DRM_DISP_CMD_STANDBY:
						pszMode = "Standby";
						break;
					case PVR_DRM_DISP_CMD_SUSPEND:
						pszMode = "Suspend";
						break;
					case PVR_DRM_DISP_CMD_OFF:
						pszMode = "Off";
						break;
					default:
						pszMode = "(Unknown Mode)";
						break;
				}
				printk(KERN_WARNING DRIVER_PREFIX ": %s: PVR Device %u: Display %s\n",
				__FUNCTION__, psArgs->dev, pszMode);
			}
#endif
			switch(psArgs->cmd)
			{
				case PVR_DRM_DISP_CMD_ON:
					iFBMode = FB_BLANK_UNBLANK;
					break;
				case PVR_DRM_DISP_CMD_STANDBY:
					iFBMode = FB_BLANK_HSYNC_SUSPEND;
					break;
				case PVR_DRM_DISP_CMD_SUSPEND:
					iFBMode = FB_BLANK_VSYNC_SUSPEND;
					break;
				case PVR_DRM_DISP_CMD_OFF:
					iFBMode = FB_BLANK_POWERDOWN;
					break;
				default:
					return -EINVAL;
			}

			OMAPLFBCreateSwapChainLock(psDevInfo);

			if (psDevInfo->psSwapChain != NULL)
			{
				flush_workqueue(psDevInfo->psSwapChain->psWorkQueue);
			}

			if (!lock_fb_info(psDevInfo->psLINFBInfo))
			{
				ret = -ENODEV;
			}
			else
			{
				OMAPLFB_CONSOLE_LOCK();
				psDevInfo->psLINFBInfo->flags |= FBINFO_MISC_USEREVENT;
				ret = fb_blank(psDevInfo->psLINFBInfo, iFBMode);
				psDevInfo->psLINFBInfo->flags &= ~FBINFO_MISC_USEREVENT;
				OMAPLFB_CONSOLE_UNLOCK();
				unlock_fb_info(psDevInfo->psLINFBInfo);
			}

			OMAPLFBCreateSwapChainUnLock(psDevInfo);

			break;
		}
		default:
		{
			ret = -EINVAL;
			break;
		}
	}

	return ret;
}
#endif

/* Insert the driver into the kernel */
#if defined(SUPPORT_DRI_DRM)
int PVR_DRM_MAKENAME(DISPLAY_CONTROLLER, _Init)(struct drm_device unref__ *dev)
#else
static int __init OMAPLFB_Init(void)
#endif
{

	if(OMAPLFBInit() != OMAPLFB_OK)
	{
		printk(KERN_ERR DRIVER_PREFIX ": %s: OMAPLFBInit failed\n", __FUNCTION__);
		return -ENODEV;
	}

	return 0;

}

/* Remove the driver from the kernel */
#if defined(SUPPORT_DRI_DRM)
void PVR_DRM_MAKENAME(DISPLAY_CONTROLLER, _Cleanup)(struct drm_device unref__ *dev)
#else
static void __exit OMAPLFB_Cleanup(void)
#endif
{    
	if(OMAPLFBDeInit() != OMAPLFB_OK)
	{
		printk(KERN_ERR DRIVER_PREFIX ": %s: OMAPLFBDeInit failed\n", __FUNCTION__);
	}
}

#if !defined(SUPPORT_DRI_DRM)
/*
 These macro calls define the initialisation and removal functions of the
 driver.  Although they are prefixed `module_', they apply when compiling
 statically as well; in both cases they define the function the kernel will
 run to start/stop the driver.
*/
late_initcall(OMAPLFB_Init);
module_exit(OMAPLFB_Cleanup);
#endif
