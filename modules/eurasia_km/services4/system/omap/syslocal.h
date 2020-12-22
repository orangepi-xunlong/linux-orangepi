/*************************************************************************/ /*!
@Title          Local system definitions
@Copyright      Copyright (c) Imagination Technologies Ltd. All Rights Reserved
@Description    This header provides local system declarations and macros
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

#if !defined(__SYSLOCAL_H__)
#define __SYSLOCAL_H__

#if defined(__linux__)

#include <linux/version.h>
#include <linux/clk.h>
#if defined(PVR_LINUX_USING_WORKQUEUES)
#include <linux/mutex.h>
#else
#include <linux/spinlock.h>
#endif
#include <asm/atomic.h>

#if (LINUX_VERSION_CODE > KERNEL_VERSION(2,6,26))
#include <linux/semaphore.h>
#include <linux/resource.h>
#else /* (LINUX_VERSION_CODE > KERNEL_VERSION(2,6,26)) */
#include <asm/semaphore.h>
#if (LINUX_VERSION_CODE > KERNEL_VERSION(2,6,22))
#include <asm/arch/resource.h>
#endif /* (LINUX_VERSION_CODE > KERNEL_VERSION(2,6,22)) */
#endif /* (LINUX_VERSION_CODE > KERNEL_VERSION(2,6,26)) */


#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,35))
#if !defined(LDM_PLATFORM)
#error "LDM_PLATFORM must be set"
#endif
#define	PVR_LINUX_DYNAMIC_SGX_RESOURCE_INFO
#include <linux/platform_device.h>
#endif

#if ((defined(DEBUG) || defined(TIMING)) && \
    (LINUX_VERSION_CODE == KERNEL_VERSION(2,6,34))) && \
    !defined(PVR_NO_OMAP_TIMER)
/*
 * We need to explicitly enable the GPTIMER11 clocks, or we'll get an
 * abort when we try to access the timer registers.
 */
#define	PVR_OMAP4_TIMING_PRCM
#endif

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,35))
#include <plat/gpu.h>
#if !defined(PVR_NO_OMAP_TIMER)
#define	PVR_OMAP_USE_DM_TIMER_API
#include <plat/dmtimer.h>
#endif
#endif

#if !defined(PVR_NO_OMAP_TIMER)
#define PVR_OMAP_TIMER_BASE_IN_SYS_SPEC_DATA
#endif
#endif /* defined(__linux__) */

#if !defined(NO_HARDWARE) && \
     defined(SYS_USING_INTERRUPTS)
#define SGX_OCP_REGS_ENABLED
#endif

#if defined(__linux__)
#if defined(SGX_OCP_REGS_ENABLED)
/* FIXME: Temporary workaround for OMAP4470 and active power off in 4430 */
#if !defined(SGX544) && defined(SUPPORT_ACTIVE_POWER_MANAGEMENT)
#define SGX_OCP_NO_INT_BYPASS
#endif
#endif
#endif

#if defined (__cplusplus)
extern "C" {
#endif

/*****************************************************************************
 * system specific data structures
 *****************************************************************************/
 
/*****************************************************************************
 * system specific function prototypes
 *****************************************************************************/
 
IMG_VOID DisableSystemClocks(SYS_DATA *psSysData);
PVRSRV_ERROR EnableSystemClocks(SYS_DATA *psSysData);

IMG_VOID DisableSGXClocks(SYS_DATA *psSysData);
PVRSRV_ERROR EnableSGXClocks(SYS_DATA *psSysData);

/*
 * Various flags to indicate what has been initialised, and what
 * has been temporarily deinitialised for power management purposes.
 */
#define SYS_SPECIFIC_DATA_ENABLE_SYSCLOCKS	0x00000001
#define SYS_SPECIFIC_DATA_ENABLE_LISR		0x00000002
#define SYS_SPECIFIC_DATA_ENABLE_MISR		0x00000004
#define SYS_SPECIFIC_DATA_ENABLE_ENVDATA	0x00000008
#define SYS_SPECIFIC_DATA_ENABLE_LOCDEV		0x00000010
#define SYS_SPECIFIC_DATA_ENABLE_REGDEV		0x00000020
#define SYS_SPECIFIC_DATA_ENABLE_PDUMPINIT	0x00000040
#define SYS_SPECIFIC_DATA_ENABLE_INITDEV	0x00000080
#define SYS_SPECIFIC_DATA_ENABLE_LOCATEDEV	0x00000100

#define	SYS_SPECIFIC_DATA_PM_UNINSTALL_LISR	0x00000200
#define	SYS_SPECIFIC_DATA_PM_DISABLE_SYSCLOCKS	0x00000400
#define SYS_SPECIFIC_DATA_ENABLE_OCPREGS	0x00000800
#define SYS_SPECIFIC_DATA_ENABLE_PM_RUNTIME	0x00001000
#define SYS_SPECIFIC_DATA_IRQ_ENABLED		0x00002000
#define SYS_SPECIFIC_DATA_DVFS_INIT			0x00004000

#define	SYS_SPECIFIC_DATA_SET(psSysSpecData, flag) ((IMG_VOID)((psSysSpecData)->ui32SysSpecificData |= (flag)))

#define	SYS_SPECIFIC_DATA_CLEAR(psSysSpecData, flag) ((IMG_VOID)((psSysSpecData)->ui32SysSpecificData &= ~(flag)))

#define	SYS_SPECIFIC_DATA_TEST(psSysSpecData, flag) (((psSysSpecData)->ui32SysSpecificData & (flag)) != 0)
 
typedef struct _SYS_SPECIFIC_DATA_TAG_
{
	IMG_UINT32	ui32SysSpecificData;
	PVRSRV_DEVICE_NODE *psSGXDevNode;
	IMG_BOOL	bSGXInitComplete;
#if defined(PVR_OMAP_TIMER_BASE_IN_SYS_SPEC_DATA)
	IMG_CPU_PHYADDR	sTimerRegPhysBase;
#endif
#if !defined(__linux__)
	IMG_BOOL	bSGXClocksEnabled;
#endif
	IMG_UINT32	ui32SrcClockDiv;
#if defined(__linux__)
	IMG_BOOL	bSysClocksOneTimeInit;
	atomic_t	sSGXClocksEnabled;
#if defined(PVR_LINUX_USING_WORKQUEUES)
	struct mutex	sPowerLock;
#else
	IMG_BOOL	bConstraintNotificationsEnabled;
	spinlock_t	sPowerLock;
	atomic_t	sPowerLockCPU;
	spinlock_t	sNotifyLock;
	atomic_t	sNotifyLockCPU;
	IMG_BOOL	bCallVDD2PostFunc;
#endif
#if defined(DEBUG) || defined(TIMING)
	struct clk	*psGPT11_FCK;
	struct clk	*psGPT11_ICK;
#endif
#if defined(PVR_OMAP_USE_DM_TIMER_API)
	struct omap_dm_timer *psGPTimer;
#endif
	IMG_UINT32 ui32SGXFreqListSize;
	IMG_UINT32 *pui32SGXFreqList;
	IMG_UINT32 ui32SGXFreqListIndex;
#endif	/* defined(__linux__) */
} SYS_SPECIFIC_DATA;

extern SYS_SPECIFIC_DATA *gpsSysSpecificData;

#if defined(SGX_OCP_REGS_ENABLED) && defined(SGX_OCP_NO_INT_BYPASS)
IMG_VOID SysEnableSGXInterrupts(SYS_DATA* psSysData);
IMG_VOID SysDisableSGXInterrupts(SYS_DATA* psSysData);
#else
#define	SysEnableSGXInterrupts(psSysData)
#define SysDisableSGXInterrupts(psSysData)
#endif

#if defined(SYS_CUSTOM_POWERLOCK_WRAP)
IMG_BOOL WrapSystemPowerChange(SYS_SPECIFIC_DATA *psSysSpecData);
IMG_VOID UnwrapSystemPowerChange(SYS_SPECIFIC_DATA *psSysSpecData);
#endif

#if defined(__linux__)

PVRSRV_ERROR SysPMRuntimeRegister(void);
PVRSRV_ERROR SysPMRuntimeUnregister(void);

PVRSRV_ERROR SysDvfsInitialize(SYS_SPECIFIC_DATA *psSysSpecificData);
PVRSRV_ERROR SysDvfsDeinitialize(SYS_SPECIFIC_DATA *psSysSpecificData);

#else /* defined(__linux__) */

#ifdef INLINE_IS_PRAGMA
#pragma inline(SysPMRuntimeRegister)
#endif
static INLINE PVRSRV_ERROR SysPMRuntimeRegister(void)
{
	return PVRSRV_OK;
}

#ifdef INLINE_IS_PRAGMA
#pragma inline(SysPMRuntimeUnregister)
#endif
static INLINE PVRSRV_ERROR SysPMRuntimeUnregister(void)
{
	return PVRSRV_OK;
}

#ifdef INLINE_IS_PRAGMA
#pragma inline(SysDvfsInitialize)
#endif
static INLINE PVRSRV_ERROR SysDvfsInitialize(SYS_SPECIFIC_DATA *psSysSpecificData)
{
	PVR_UNREFERENCED_PARAMETER(psSysSpecificData);
	return PVRSRV_OK;
}

#ifdef INLINE_IS_PRAGMA
#pragma inline(SysDvfsDeinitialize)
#endif
static INLINE PVRSRV_ERROR SysDvfsDeinitialize(SYS_SPECIFIC_DATA *psSysSpecificData)
{
	PVR_UNREFERENCED_PARAMETER(psSysSpecificData);
	return PVRSRV_OK;
}

#endif /* defined(__linux__) */

#if defined(__cplusplus)
}
#endif

#endif	/* __SYSLOCAL_H__ */


