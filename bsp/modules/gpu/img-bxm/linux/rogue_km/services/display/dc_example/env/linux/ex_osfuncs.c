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

#if defined(__linux__)

#include <asm/page.h>
#include <linux/io.h>
#include <linux/mm.h>
#include <linux/vmalloc.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/debugfs.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/version.h>
/* For MODULE_LICENSE */
#include "pvrmodule.h"

#include "dc_osfuncs.h"
#include "dc_example.h"

#ifndef DC_EXAMPLE_WIDTH
#define DC_EXAMPLE_WIDTH 640
#endif

#ifndef DC_EXAMPLE_HEIGHT
#define DC_EXAMPLE_HEIGHT 480
#endif

#ifndef DC_EXAMPLE_BIT_DEPTH
#define DC_EXAMPLE_BIT_DEPTH 32
#endif

#ifndef DC_EXAMPLE_FORMAT
#define DC_EXAMPLE_FORMAT 0
#endif

#ifndef DC_EXAMPLE_MEMORY_LAYOUT
#define DC_EXAMPLE_MEMORY_LAYOUT 0
#endif

#ifndef DC_EXAMPLE_FBC_FORMAT
#define DC_EXAMPLE_FBC_FORMAT 0
#endif

#ifndef DC_EXAMPLE_REFRESH_RATE
#define DC_EXAMPLE_REFRESH_RATE 60
#endif

#ifndef DC_EXAMPLE_DPI
#define DC_EXAMPLE_DPI 160
#endif

#ifndef DC_EXAMPLE_NUM_DEVICES_DEFAULT
#define DC_EXAMPLE_NUM_DEVICES_DEFAULT 1
#endif

#define DC_EXAMPLE_DEBUGFS_NUM_DEVICES "num_devices"

static struct dentry	*g_psDebugFSEntryDir;	/* Top-level handle */
static struct dentry	*g_psNumDevicesEntry;	/* 'num_devices' entry */

static IMG_UINT32 g_uiNumDCEXDevs;			/* Number of valid devices */

DC_EXAMPLE_MODULE_PARAMETERS sModuleParams =
{
	.ui32Width = DC_EXAMPLE_WIDTH,
	.ui32Height = DC_EXAMPLE_HEIGHT,
	.ui32Depth = DC_EXAMPLE_BIT_DEPTH,
	.ui32Format = DC_EXAMPLE_FORMAT,
	.ui32MemLayout = DC_EXAMPLE_MEMORY_LAYOUT,
	.ui32FBCFormat = DC_EXAMPLE_FBC_FORMAT,
	.ui32RefreshRate = DC_EXAMPLE_REFRESH_RATE,
	.ui32XDpi = DC_EXAMPLE_DPI,
	.ui32YDpi = DC_EXAMPLE_DPI,
	.ui32NumDevices = DC_EXAMPLE_NUM_DEVICES_DEFAULT
};

#define STR(s)	#s
#define STRINGIFY(s)	STR(s)

module_param_named(width,	sModuleParams.ui32Width, uint, S_IRUGO);
module_param_named(height,	sModuleParams.ui32Height, uint, S_IRUGO);
module_param_named(depth,	sModuleParams.ui32Depth, uint, S_IRUGO);
module_param_named(format,	sModuleParams.ui32Format, uint, S_IRUGO);
module_param_named(memlayout,	sModuleParams.ui32MemLayout, uint, S_IRUGO);
module_param_named(fbcformat,	sModuleParams.ui32FBCFormat, uint, S_IRUGO);
module_param_named(refreshrate,	sModuleParams.ui32RefreshRate, uint, S_IRUGO);
module_param_named(xdpi,	sModuleParams.ui32XDpi, uint, S_IRUGO);
module_param_named(ydpi,	sModuleParams.ui32YDpi, uint, S_IRUGO);
module_param_named(num_devices,	sModuleParams.ui32NumDevices, uint, S_IRUGO);
MODULE_PARM_DESC(num_devices,
        "Number of display devices to register (default: "
		STRINGIFY(DC_EXAMPLE_NUM_DEVICES_DEFAULT) " - max: "
		STRINGIFY(PVRSRV_MAX_DEVICES) ")");

const DC_EXAMPLE_MODULE_PARAMETERS *DCExampleGetModuleParameters(void)
{
	return &sModuleParams;
}

static int NumDevicesOpen(struct inode *psInode, struct file *psFile)
{
	psFile->private_data = psInode->i_private;

	return 0;
}

static ssize_t NumDevicesRead(struct file *psFile,
                              char __user *psUserBuffer,
                              size_t uiCount, loff_t *puiPosition)
{
	char aszBuffer[]="XX\n";
	loff_t uiPosition = *puiPosition;
	size_t uiBufferSize = ARRAY_SIZE(aszBuffer);
	size_t uiLocCount;

	PVR_UNREFERENCED_PARAMETER(psFile);

	if (uiPosition < 0)
	{
		return -EINVAL;
	}
	else if ((uiPosition >= (loff_t)uiBufferSize) || (uiCount == 0U))
	{
		return 0;
	}

	(void) DC_OSSNPrintf(aszBuffer, sizeof(aszBuffer), "%u", g_uiNumDCEXDevs);

	if (uiCount > (uiBufferSize - (size_t)uiPosition))
	{
		uiLocCount = uiBufferSize - (size_t)uiPosition;
	}
	else
	{
		uiLocCount = uiCount;
	}


	if (copy_to_user(psUserBuffer, &aszBuffer[uiPosition], uiLocCount) != 0)
	{
		return -EFAULT;
	}

	*puiPosition = uiPosition + (loff_t)uiLocCount;

	return (ssize_t)uiLocCount;
}

static const struct file_operations gsDCEXNumDevicesFileOps =
{
	.owner = THIS_MODULE,
	.open = NumDevicesOpen,
	.read = NumDevicesRead,
	.llseek = default_llseek
};

typedef struct _DC_EXAMPLE_PAGE_MAPPING
{
	void         *pvVirtAddr;
	unsigned int  uiPageCnt;
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 8, 0))
	struct page **ppsPageArray;
#endif
} DC_EXAMPLE_PAGE_MAPPING;

static void pvr_vfree(DC_EXAMPLE_PAGE_MAPPING *psMap)
{
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 8, 0))
	unsigned int i;
#endif

	if (!psMap)
	{
		return;
	}

	vfree(psMap->pvVirtAddr);

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 8, 0))
	/* Free up all the pages. */
	for (i = 0; i < psMap->uiPageCnt; i++)
	{
		if (psMap->ppsPageArray[i])
		{
			__free_page(psMap->ppsPageArray[i]);
		}
	}

	kfree(psMap->ppsPageArray);
#endif /* LINUX_VERSION_CODE >= KERNEL_VERSION(5, 8, 0) */

	kfree(psMap);
}

static void *pvr_vmalloc(size_t uiSize, gfp_t gfp_mask, pgprot_t prot)
{
	DC_EXAMPLE_PAGE_MAPPING *psMap;
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 8, 0))
	unsigned int i;
#endif

	psMap = kzalloc(sizeof(*psMap), GFP_KERNEL);
	if (!psMap)
	{
		return NULL;
	}

	psMap->uiPageCnt = DIV_ROUND_UP(uiSize, PAGE_SIZE);

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 8, 0))
	psMap->ppsPageArray = kcalloc(psMap->uiPageCnt, sizeof(*psMap->ppsPageArray), GFP_KERNEL);
	if (!psMap->ppsPageArray)
	{
		goto err_pvr_vfree;
	}

	/* Allocate kernel pages 1 by 1 - Keep reference in the "pages" struct. */
	for (i = 0; i < psMap->uiPageCnt; i++)
	{
		psMap->ppsPageArray[i] = alloc_page(gfp_mask);
		if (!psMap->ppsPageArray[i])
		{
			goto err_pvr_vfree;
		}
	}

	/* Map the pages into contiguous VM space. */
	psMap->pvVirtAddr = vmap(psMap->ppsPageArray, (uiSize >> PAGE_SHIFT), VM_MAP, prot);
#else
	psMap->pvVirtAddr = __vmalloc(uiSize, gfp_mask, prot);
#endif /* LINUX_VERSION_CODE >= KERNEL_VERSION(5, 8, 0) */

	if (!psMap->pvVirtAddr)
	{
		goto err_pvr_vfree;
	}

	return (void *) psMap;

err_pvr_vfree:
	pvr_vfree(psMap);
	return NULL;
}

void *DCExampleVirtualAllocUncached(size_t uiSize)
{
	unsigned int gfp_flags = GFP_USER | __GFP_NOWARN | __GFP_NOMEMALLOC;

#if defined(CONFIG_X86_64)
	gfp_flags |= __GFP_DMA32;
#else
#if !defined(CONFIG_X86_PAE)
	gfp_flags |= __GFP_HIGHMEM;
#endif
#endif

	return pvr_vmalloc(uiSize, gfp_flags, pgprot_noncached(PAGE_KERNEL));
}

IMG_BOOL DCExampleVirtualFree(void *pvAllocHandle)
{
	pvr_vfree((DC_EXAMPLE_PAGE_MAPPING *) pvAllocHandle);

	/* vfree does not return a value, so all we can do is hard code IMG_TRUE */
	return IMG_TRUE;
}

#define	VMALLOC_TO_PAGE_PHYS(vAddr) page_to_phys(vmalloc_to_page(vAddr))

PVRSRV_ERROR DCExampleGetLinAddr(void *pvAllocHandle, IMG_CPU_VIRTADDR *ppvLinAddr)
{
	DC_EXAMPLE_PAGE_MAPPING *psMap     = (DC_EXAMPLE_PAGE_MAPPING *) pvAllocHandle;
	IMG_CPU_VIRTADDR         pvLinAddr = psMap->pvVirtAddr;

	if (ppvLinAddr != NULL)
	{
		*ppvLinAddr = pvLinAddr;
	}

	return PVRSRV_OK;
}

PVRSRV_ERROR DCExampleGetDevPAddrs(void *pvAllocHandle, IMG_DEV_PHYADDR *pasDevPAddr,
				   uint32_t uiPageNo, size_t uiSize)
{
	DC_EXAMPLE_PAGE_MAPPING *psMap = (DC_EXAMPLE_PAGE_MAPPING *) pvAllocHandle;
	unsigned long ulPages      = DC_OS_BYTES_TO_PAGES(uiSize);
#if (LINUX_VERSION_CODE < KERNEL_VERSION(5, 8, 0))
	IMG_CPU_VIRTADDR pvLinAddr = (IMG_CPU_VIRTADDR) IMG_OFFSET_ADDR(psMap->pvVirtAddr, uiPageNo * PAGE_SIZE);
#endif /* (LINUX_VERSION_CODE < KERNEL_VERSION(5, 8, 0)) */
	int           i;

	if (ulPages > psMap->uiPageCnt)
	{
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	for (i = 0; i < ulPages; i++)
	{
#if (LINUX_VERSION_CODE < KERNEL_VERSION(5, 8, 0))
		pasDevPAddr[i].uiAddr = VMALLOC_TO_PAGE_PHYS(pvLinAddr);
		pvLinAddr += PAGE_SIZE;
#else
		pasDevPAddr[i].uiAddr = page_to_phys(psMap->ppsPageArray[uiPageNo + i]);
#endif /* (LINUX_VERSION_CODE < KERNEL_VERSION(5, 8, 0)) */
	}

	return PVRSRV_OK;
}

static int __init dc_example_init(void)
{
	if (DCExampleInit(&g_uiNumDCEXDevs) != PVRSRV_OK)
	{
		return -ENODEV;
	}

	/* Create a debugfs entry to return the number of devices discovered */
	g_psDebugFSEntryDir = debugfs_create_dir(MODNAME, NULL);
	if (IS_ERR_OR_NULL(g_psDebugFSEntryDir))
	{
		printk(KERN_WARNING DRVNAME " - %s: Failed to create '%s' debugfs root directory "
		       "(debugfs entries won't be available)\n", __func__, MODNAME);
		g_psDebugFSEntryDir = NULL;
	}

	g_psNumDevicesEntry = debugfs_create_file(DC_EXAMPLE_DEBUGFS_NUM_DEVICES,
	                                          S_IFREG | S_IRUGO,
	                                          g_psDebugFSEntryDir,
	                                          &sModuleParams.ui32NumDevices,
	                                          &gsDCEXNumDevicesFileOps);

	if (IS_ERR_OR_NULL(g_psNumDevicesEntry))
	{
		printk(KERN_WARNING DRVNAME " - %s: Failed to create '%s' debugfs file "
		       "(debugfs entries won't be available)\n", __func__,
		       DC_EXAMPLE_DEBUGFS_NUM_DEVICES);
		g_psNumDevicesEntry = NULL;
	}

	return 0;
}

/******************************************************************************
 Function Name: dc_example_deinit
 Description  : Remove the driver from the kernel.

                __exit places the function in a special memory section that
                the kernel frees once the function has been run.  Refer also
                to module_exit() macro call below.
******************************************************************************/
static void __exit dc_example_deinit(void)
{
	DCExampleDeinit();

	/* Remove the debugfs entry for the number of devices */
	if (g_psNumDevicesEntry != NULL)
	{
		debugfs_remove(g_psNumDevicesEntry);
		g_psNumDevicesEntry = NULL;
	}

	if (g_psDebugFSEntryDir != NULL)
	{
		debugfs_remove(g_psDebugFSEntryDir);
		g_psDebugFSEntryDir = NULL;
	}
}

module_init(dc_example_init);
module_exit(dc_example_deinit);

#endif /* defined(__linux__) */
