/*************************************************************************/ /*!
@Title          System Description Header
@Copyright      Copyright (c) Imagination Technologies Ltd. All Rights Reserved
@Description    This header provides system-specific declarations and macros
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

#if !defined(__SOCCONFIG_H__)
#define __SOCCONFIG_H__

#if defined(VS_PRODUCT_VERSION) && VS_PRODUCT_VERSION == 5
#define VS_PRODUCT_NAME	"OMAP5"
#else
#define VS_PRODUCT_NAME	"OMAP4"
#endif


#define SYS_SGX_HWRECOVERY_TIMEOUT_FREQ		(100)	// 10ms (100hz)
#define SYS_SGX_PDS_TIMER_FREQ				(1000)	// 1ms (1000hz)

/* Allow the AP latency to be overridden in the build config */
#if !defined(SYS_SGX_ACTIVE_POWER_LATENCY_MS)
#define SYS_SGX_ACTIVE_POWER_LATENCY_MS		(2)
#endif


#define SYS_OMAP_SGX_REGS_SYS_PHYS_BASE  0x56000000
#define SYS_OMAP_SGX_REGS_SIZE           0xFFFF

#define SYS_OMAP_SGX_IRQ				 53 /* OMAP4 IRQ's are offset by 32 */

#define SYS_OMAP_DSS_REGS_SYS_PHYS_BASE  0x58000000
#define SYS_OMAP_DSS_REGS_SIZE           0x7000

#define SYS_OMAP_DSS_HDMI_INTERRUPT_STATUS_REG 0x6028
#define SYS_OMAP_DSS_HDMI_INTERRUPT_ENABLE_REG 0x602c

#define SYS_OMAP_DSS_HDMI_INTERRUPT_VSYNC_ENABLE_MASK 0x10000
#define SYS_OMAP_DSS_HDMI_INTERRUPT_VSYNC_STATUS_MASK 0x10000

#define SYS_OMAP_DSS_LCD_INTERRUPT_STATUS_REG 0x1018
#define SYS_OMAP_DSS_LCD_INTERRUPT_ENABLE_REG 0x101c

#define SYS_OMAP_DSS_LCD_INTERRUPT_VSYNC_ENABLE_MASK 0x40002
#define SYS_OMAP_DSS_LCD_INTERRUPT_VSYNC_STATUS_MASK 0x40002


#define SYS_OMAP_GP11TIMER_ENABLE_SYS_PHYS_BASE	0x48088038
#define SYS_OMAP_GP11TIMER_REGS_SYS_PHYS_BASE	0x4808803C
#define SYS_OMAP_GP11TIMER_TSICR_SYS_PHYS_BASE	0x48088054

/* Interrupt bits */
#define DEVICE_SGX_INTERRUPT		(1<<0)
#define DEVICE_MSVDX_INTERRUPT		(1<<1)
#define DEVICE_DISP_INTERRUPT		(1<<2)

#if defined(__linux__)
/*
 * Recent OMAP4 kernels register SGX as platform device "omap_gpu".
 * This device must be used with the Linux power management calls
 * in sysutils_linux.c, in order for SGX to be powered on.
 */
#if defined(PVR_LDM_PLATFORM_PRE_REGISTERED_DEV)
#define	SYS_SGX_DEV_NAME	PVR_LDM_PLATFORM_PRE_REGISTERED_DEV
#else
#define	SYS_SGX_DEV_NAME	"omap_gpu"
#endif	/* defined(PVR_LDM_PLATFORM_PRE_REGISTERED_DEV) */
#endif	/* defined(__linux__) */

/*****************************************************************************
 * system specific data structures
 *****************************************************************************/
 
#endif	/* __SYSCONFIG_H__ */
