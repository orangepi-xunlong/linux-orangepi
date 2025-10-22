/*************************************************************************/ /*!
@File
@Title          System Configuration
@Copyright      Copyright (c) Imagination Technologies Ltd. All Rights Reserved
@Description    System Configuration functions
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

#include "img_defs.h"
#include "pvr_debug.h"
#include "osfunc.h"
#include "allocmem.h"
#include "pvrsrv.h"
#include "pvrsrv_device.h"
#include "syscommon.h"
#include "power.h"
#include "sysinfo.h"
#include "apollo_regs.h"
#include "sysconfig.h"
#include "physheap.h"
#include "pci_support.h"
#include "interrupt_support.h"
#include "tcf_clk_ctrl.h"
#include "tcf_pll.h"
#include "sysdata.h"
#include "dc_server.h"
#if defined(__linux__)
#include <linux/dma-mapping.h>
#endif

#if (TC_MEMORY_CONFIG == TC_MEMORY_LOCAL)
#define	HOST_PCI_INIT_FLAGS	0
#else
#define	HOST_PCI_INIT_FLAGS	HOST_PCI_INIT_FLAG_BUS_MASTER
#endif

/* This is used only by guest drivers during interrupt handling */
#define IRQ_ACK_RETRY_CLEAR_MAX 2

/* Interrupt defines */
typedef enum _APOLLO_IRQ_
{
	APOLLO_IRQ_GPU = 0,
	APOLLO_IRQ_PDP,
	APOLLO_IRQ_MAX,
} APOLLO_IRQ;

/* Clock speed module parameters */
static IMG_UINT32 ui32MemClockSpeed  = RGX_TC_MEM_CLOCK_SPEED;
static IMG_UINT32 ui32CoreClockSpeed = RGX_TC_CORE_CLOCK_SPEED;
#if !defined(TC_APOLLO_ES2)
static IMG_UINT32 ui32SysClockSpeed = RGX_TC_SYS_CLOCK_SPEED;
#endif
#if defined(TC_APOLLO_TCF5)
static IMG_UINT32 ui32MemReadLatency = 0;
static IMG_UINT32 ui32MemWriteResponseLatency = 0;
#endif

#if defined(__linux__)
#include <linux/module.h>
#include <linux/moduleparam.h>
module_param_named(sys_mem_clk_speed,  ui32MemClockSpeed,  uint, S_IRUGO | S_IWUSR);
module_param_named(sys_core_clk_speed, ui32CoreClockSpeed, uint, S_IRUGO | S_IWUSR);
#if !defined(TC_APOLLO_ES2)
module_param_named(sys_sysif_clk_speed, ui32SysClockSpeed, uint, S_IRUGO | S_IWUSR);
#endif
#if defined(TC_APOLLO_TCF5)
module_param_named(sys_mem_latency, ui32MemReadLatency, uint, S_IRUGO | S_IWUSR);
module_param_named(sys_wresp_latency, ui32MemWriteResponseLatency, uint, S_IRUGO | S_IWUSR);
#endif
#endif

/* Specifies the maximum number of ApolloHardReset() attempts */
#define APOLLO_RESET_MAX_RETRIES	10

#define SYSTEM_INFO_FORMAT_STRING "FPGA Revision: %s - TCF Core Revision: %s - TCF Core Target Build ID: %s - PCI Version: %s - Macro Version: %s"
#define FPGA_REV_MAX_LEN      8 /* current longest format: "x.y.z" */
#define TCF_CORE_REV_MAX_LEN  8 /* current longest format: "x.y.z" */
#define TCF_CORE_CFG_MAX_LEN  4 /* current longest format: "x" */
#define PCI_VERSION_MAX_LEN   4 /* current longest format: "x" */
#define MACRO_VERSION_MAX_LEN 8 /* current longest format: "x.yz" */

#define HEX2DEC(hexIntVal)    ((((hexIntVal) >> 4) * 10) + ((hexIntVal) & 0x0F))

#if defined(SUPPORT_LINUX_DVFS) || defined(SUPPORT_PDVFS)
/* Fake DVFS configuration used purely for testing purposes */

static const IMG_OPP g_asOPPTable[] =
{
	{ 8,  25000000},
	{ 16, 50000000},
	{ 32, 75000000},
	{ 64, 100000000},
};

#define LEVEL_COUNT (sizeof(g_asOPPTable) / sizeof(IMG_OPP))

static void SetFrequency(IMG_HANDLE hSysData, IMG_UINT32 ui32Frequency)
{
	PVR_UNREFERENCED_PARAMETER(hSysData);

	PVR_DPF((PVR_DBG_ERROR, "SetFrequency %u", ui32Frequency));
}

static void SetVoltage(IMG_HANDLE hSysData, IMG_UINT32 ui32Voltage)
{
	PVR_UNREFERENCED_PARAMETER(hSysData);

	PVR_DPF((PVR_DBG_ERROR, "SetVoltage %u", ui32Voltage));
}
#endif

static PVRSRV_ERROR InitMemory(PVRSRV_DEVICE_CONFIG *psSysConfig, SYS_DATA *psSysData);
static INLINE void DeInitMemory(PVRSRV_DEVICE_CONFIG *psSysConfig, SYS_DATA *psSysData);

size_t GetMemorySize(IMG_CPU_PHYADDR sMemCpuPhyAddr, IMG_UINT64 pciBarSize);

static IMG_CHAR *GetDeviceVersionString(SYS_DATA *psSysData)
{
	IMG_CHAR apszFPGARev[FPGA_REV_MAX_LEN];
	IMG_CHAR apszCoreRev[TCF_CORE_REV_MAX_LEN];
	IMG_CHAR apszConfigRev[TCF_CORE_CFG_MAX_LEN];
	IMG_CHAR apszPCIVer[PCI_VERSION_MAX_LEN];
	IMG_CHAR apszMacroVer[MACRO_VERSION_MAX_LEN];
	IMG_CHAR *pszVersion;
	IMG_UINT32 ui32StringLength;
	IMG_UINT32 ui32Value;
	IMG_CPU_PHYADDR	sHostFPGARegCpuPBase;
	void *pvHostFPGARegCpuVBase;

	/* To get some of the version information we need to read from a register that we don't normally have
	   mapped. Map it temporarily (without trying to reserve it) to get the information we need. */
	sHostFPGARegCpuPBase.uiAddr	= OSPCIAddrRangeStart(psSysData->hRGXPCI, SYS_APOLLO_REG_PCI_BASENUM) + 0x40F0;
	pvHostFPGARegCpuVBase		= OSMapPhysToLin(sHostFPGARegCpuPBase, 0x04, PVRSRV_MEMALLOCFLAG_CPU_UNCACHED);
	if (pvHostFPGARegCpuVBase == NULL)
	{
		return NULL;
	}

	/* Create the components of the PCI and macro versions */
	ui32Value = OSReadHWReg32(pvHostFPGARegCpuVBase, 0);
	OSSNPrintf(apszPCIVer, sizeof(apszPCIVer), "%d",
	           HEX2DEC((ui32Value & 0x00FF0000) >> 16));
	OSSNPrintf(apszMacroVer, sizeof(apszMacroVer), "%d.%d",
	           ((ui32Value & 0x00000F00) >> 8),
	           HEX2DEC((ui32Value & 0x000000FF) >> 0));

	/* Unmap the register now that we no longer need it */
	OSUnMapPhysToLin(pvHostFPGARegCpuVBase, 0x04);

	/*
	 * Check bits 7:0 of register 0x28 (TCF_CORE_REV_REG or SW_IF_VERSION
	 * depending on its own value) to find out how the driver should
	 * generate the strings for FPGA and core revision.
	 */
	ui32Value = OSReadHWReg32(psSysData->pvSystemRegCpuVBase, TCF_CLK_CTRL_SW_IF_VERSION);
	ui32Value = (ui32Value & VERSION_MASK) >> VERSION_SHIFT;

	if (ui32Value == 0)
	{
		/* Create the components of the TCF core revision number */
		ui32Value = OSReadHWReg32(psSysData->pvSystemRegCpuVBase, TCF_CLK_CTRL_TCF_CORE_REV_REG);
		OSSNPrintf(apszCoreRev, sizeof(apszCoreRev), "%d.%d.%d",
		           HEX2DEC((ui32Value & TCF_CORE_REV_REG_MAJOR_MASK) >> TCF_CORE_REV_REG_MAJOR_SHIFT),
		           HEX2DEC((ui32Value & TCF_CORE_REV_REG_MINOR_MASK) >> TCF_CORE_REV_REG_MINOR_SHIFT),
		           HEX2DEC((ui32Value & TCF_CORE_REV_REG_MAINT_MASK) >> TCF_CORE_REV_REG_MAINT_SHIFT));

		/* Create the components of the FPGA revision number */
		ui32Value = OSReadHWReg32(psSysData->pvSystemRegCpuVBase, TCF_CLK_CTRL_FPGA_REV_REG);
		OSSNPrintf(apszFPGARev, sizeof(apszFPGARev), "%d.%d.%d",
		           HEX2DEC((ui32Value & FPGA_REV_REG_MAJOR_MASK) >> FPGA_REV_REG_MAJOR_SHIFT),
		           HEX2DEC((ui32Value & FPGA_REV_REG_MINOR_MASK) >> FPGA_REV_REG_MINOR_SHIFT),
		           HEX2DEC((ui32Value & FPGA_REV_REG_MAINT_MASK) >> FPGA_REV_REG_MAINT_SHIFT));
	}
	else if (ui32Value == 1)
	{
		/* Create the components of the TCF core revision number */
		OSSNPrintf(apszCoreRev, sizeof(apszCoreRev), "%d", ui32Value);

		/* Create the components of the FPGA revision number */
		ui32Value = OSReadHWReg32(psSysData->pvSystemRegCpuVBase, TCF_CLK_CTRL_REL);
		OSSNPrintf(apszFPGARev, sizeof(apszFPGARev), "%d.%d",
		           HEX2DEC((ui32Value & MAJOR_MASK) >> MAJOR_SHIFT),
		           HEX2DEC((ui32Value & MINOR_MASK) >> MINOR_SHIFT));
	}
	else
	{
		PVR_DPF((PVR_DBG_WARNING, "%s: unrecognised SW_IF_VERSION 0x%08x", __func__, ui32Value));

		/* Create the components of the TCF core revision number */
		OSSNPrintf(apszCoreRev, sizeof(apszCoreRev), "%d", ui32Value);

		/* Create the components of the FPGA revision number */
		OSSNPrintf(apszFPGARev, sizeof(apszFPGARev), "N/A");
	}

	/* Create the component of the TCF core target build ID */
	ui32Value = OSReadHWReg32(psSysData->pvSystemRegCpuVBase, TCF_CLK_CTRL_TCF_CORE_TARGET_BUILD_CFG);
	OSSNPrintf(apszConfigRev, sizeof(apszConfigRev), "%d",
	           (ui32Value & TCF_CORE_TARGET_BUILD_ID_MASK) >> TCF_CORE_TARGET_BUILD_ID_SHIFT);

	/* Calculate how much space we need to allocate for the string */
	ui32StringLength = OSStringLength(SYSTEM_INFO_FORMAT_STRING);
	ui32StringLength += OSStringLength(apszFPGARev);
	ui32StringLength += OSStringLength(apszCoreRev);
	ui32StringLength += OSStringLength(apszConfigRev);
	ui32StringLength += OSStringLength(apszPCIVer);
	ui32StringLength += OSStringLength(apszMacroVer);

	/* Create the version string */
	pszVersion = OSAllocMem(ui32StringLength * sizeof(IMG_CHAR));
	if (pszVersion)
	{
		OSSNPrintf(&pszVersion[0], ui32StringLength,
		           SYSTEM_INFO_FORMAT_STRING,
		           apszFPGARev,
		           apszCoreRev,
		           apszConfigRev,
		           apszPCIVer,
		           apszMacroVer);
	}
	else
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: failed to create format string", __func__));
	}

	return pszVersion;
}

static IMG_BOOL SystemISRHandler(void *pvData)
{
	IMG_UINT32 i;
	IMG_UINT32 ui32InterruptStatus;
	IMG_UINT32 ui32InterruptClear = 0;
	SYS_DATA *psSysData = (SYS_DATA *)pvData;

	/* Read device interrupt status register on entry */
	ui32InterruptStatus = OSReadHWReg32(psSysData->pvSystemRegCpuVBase, TCF_CLK_CTRL_INTERRUPT_STATUS);

	for (i = 0; i < ARRAY_SIZE(psSysData->sInterruptData); i++)
	{
		if ((ui32InterruptStatus & psSysData->sInterruptData[i].ui32InterruptFlag) != 0)
		{
			psSysData->sInterruptData[i].pfnLISR(psSysData->sInterruptData[i].pvData);
			ui32InterruptClear |= psSysData->sInterruptData[i].ui32InterruptFlag;
		}
	}

	if (ui32InterruptClear)
	{
		OSWriteHWReg32(psSysData->pvSystemRegCpuVBase, TCF_CLK_CTRL_INTERRUPT_CLEAR, ui32InterruptClear);

		/*
		   CPU-PCI-WRITE-BUFFER

		   On PC platforms, this BIOS performance enhancing feature controls the chipset's
		   CPU-to-PCI write buffer used to store PCI writes from the CPU before being written
		   onto the PCI bus. By reading from the register, this CPU-to-PCI write buffer is
		   flushed. Without this read, at times the kernel reports unhandled IRQs due to
		   above interrupt clear not being reflected on the IRQ line after exit from the
		   interrupt handler.
		 */
		(void)OSReadHWReg32(psSysData->pvSystemRegCpuVBase, TCF_CLK_CTRL_INTERRUPT_CLEAR);

		return IMG_TRUE;
	}

	return IMG_FALSE;
}

#if defined(TC_APOLLO_ES2) || defined(TC_APOLLO_BONNIE)

static PVRSRV_ERROR SPI_Read(void *pvLinRegBaseAddr,
							 IMG_UINT32 ui32Offset,
							 IMG_UINT32 *pui32Value)
{
	IMG_UINT32 ui32Count = 0;
	*pui32Value = 0;

	OSWriteHWReg32(pvLinRegBaseAddr,
				   TCF_CLK_CTRL_TCF_SPI_MST_ADDR_RDNWR,
				   TCF_SPI_MST_RDNWR_MASK | ui32Offset);
	OSWriteHWReg32(pvLinRegBaseAddr, TCF_CLK_CTRL_TCF_SPI_MST_GO, TCF_SPI_MST_GO_MASK);
	OSWaitus(100);

	while (((OSReadHWReg32(pvLinRegBaseAddr, TCF_CLK_CTRL_TCF_SPI_MST_STATUS)) != 0x08) &&
			(ui32Count < 10000))
	{
		ui32Count++;
	}

	if (ui32Count < 10000)
	{
		*pui32Value = OSReadHWReg32(pvLinRegBaseAddr, TCF_CLK_CTRL_TCF_SPI_MST_RDATA);
	}
	else
	{
		PVR_DPF((PVR_DBG_ERROR, "SPI_Read: Time out reading SPI register (0x%x)", ui32Offset));
		return PVRSRV_ERROR_TIMEOUT_POLLING_FOR_VALUE;
	}

	return PVRSRV_OK;
}

static void SPI_Write(void *pvLinRegBaseAddr,
					  IMG_UINT32 ui32Offset,
					  IMG_UINT32 ui32Value)
{
	OSWriteHWReg32(pvLinRegBaseAddr, TCF_CLK_CTRL_TCF_SPI_MST_ADDR_RDNWR, ui32Offset);
	OSWriteHWReg32(pvLinRegBaseAddr, TCF_CLK_CTRL_TCF_SPI_MST_WDATA, ui32Value);
	OSWriteHWReg32(pvLinRegBaseAddr, TCF_CLK_CTRL_TCF_SPI_MST_GO, TCF_SPI_MST_GO_MASK);
	OSWaitus(1000);
}

static IMG_BOOL IsInterfaceAligned(IMG_UINT32 ui32Eyes,
								   IMG_UINT32 ui32ClkTaps,
								   IMG_UINT32 ui32TrainAck)
{
	IMG_UINT32	ui32MaxEyeStart = ui32Eyes >> 16;
	IMG_UINT32	ui32MinEyeEnd = ui32Eyes & 0xffff;

	IMG_BOOL	bTrainingComplete = (ui32ClkTaps & 0x10000);

	IMG_BOOL	bTrainAckComplete = (ui32TrainAck & 0x100);
	IMG_UINT32	ui32FailedAcks = (ui32TrainAck & 0xf0) >> 4;


	//If either the training or training ack failed, we haven't aligned
	if (!bTrainingComplete || !bTrainAckComplete)
	{
		return IMG_FALSE;
	}

	//If the max eye >= min eye it means the readings are nonsense
	if (ui32MaxEyeStart >= ui32MinEyeEnd)
	{
		return IMG_FALSE;
	}

	//If we failed the ack pattern more than 4 times
	if (ui32FailedAcks > 4)
	{
		return IMG_FALSE;
	}

	//If there is less than 7 taps
	//(240ps @40ps/tap, this number should be lower for the fpga, since its taps are bigger
	//We should really calculate the "7" based on the interface clock speed.
	if ((ui32MinEyeEnd - ui32MaxEyeStart) < 7)
	{
		return IMG_FALSE;
	}

	return IMG_TRUE;
}

static IMG_UINT32 SAI_Read(void *base, IMG_UINT32 addr)
{
	OSWriteHWReg32(base, 0x300, 0x200 | addr);
	OSWriteHWReg32(base, 0x318, 1);
	return OSReadHWReg32(base, 0x310);
}

static PVRSRV_ERROR ApolloHardReset(SYS_DATA *psSysData)
{
	IMG_BOOL bAlignmentOK = IMG_FALSE;
	IMG_INT resetAttempts = 0;
	IMG_UINT32 ui32ClkResetN;
#if !defined(TC_APOLLO_BONNIE)
	IMG_UINT32 ui32DUTCtrl;
#endif
	//This is required for SPI reset which is not yet implemented.
	//IMG_UINT32 ui32AuxResetsN;

#if !defined(TC_APOLLO_BONNIE)
	//power down
	ui32DUTCtrl = OSReadHWReg32(psSysData->pvSystemRegCpuVBase, TCF_CLK_CTRL_DUT_CONTROL_1);
	ui32DUTCtrl &= ~DUT_CTRL_VCC_0V9EN;
	ui32DUTCtrl &= ~DUT_CTRL_VCC_1V8EN;
	ui32DUTCtrl |= DUT_CTRL_VCC_IO_INH;
	ui32DUTCtrl |= DUT_CTRL_VCC_CORE_INH;
	OSWriteHWReg32(psSysData->pvSystemRegCpuVBase, TCF_CLK_CTRL_DUT_CONTROL_1, ui32DUTCtrl);
#endif

	OSSleepms(500);

	//set clock speed here, before reset.
#if defined(TC_APOLLO_BONNIE)
	//The ES2 FPGA builds do not like their core clocks being set (it takes apollo down).
	OSWriteHWReg32(psSysData->pvSystemRegCpuVBase, 0x1000 | TCF_PLL_PLL_CORE_CLK0, ui32CoreClockSpeed / 1000000);
	OSWriteHWReg32(psSysData->pvSystemRegCpuVBase, 0x1000 | TCF_PLL_PLL_CORE_DRP_GO, 1);
#endif

	OSWriteHWReg32(psSysData->pvSystemRegCpuVBase, 0x1000 | TCF_PLL_PLL_MEMIF_CLK0, ui32MemClockSpeed / 1000000);
	OSWriteHWReg32(psSysData->pvSystemRegCpuVBase, 0x1000 | TCF_PLL_PLL_MEM_DRP_GO, 1);

	PVR_LOG(("Setting clocks to %uMHz/%uMHz", ui32CoreClockSpeed / 1000000, ui32MemClockSpeed / 1000000));

	OSWaitus(400);

	//Put DCM, DUT, DDR, PDP1, and PDP2 into reset
	ui32ClkResetN = (0x1 << GLB_CLKG_EN_SHIFT);
	ui32ClkResetN |= (0x1 << SCB_RESETN_SHIFT);
	OSWriteHWReg32(psSysData->pvSystemRegCpuVBase, TCF_CLK_CTRL_CLK_AND_RST_CTRL, ui32ClkResetN);

	OSSleepms(100);

#if !defined(TC_APOLLO_BONNIE)
	//Enable the voltage control regulators on DUT
	ui32DUTCtrl = OSReadHWReg32(psSysData->pvSystemRegCpuVBase, TCF_CLK_CTRL_DUT_CONTROL_1);
	ui32DUTCtrl |= DUT_CTRL_VCC_0V9EN;
	ui32DUTCtrl |= DUT_CTRL_VCC_1V8EN;
	ui32DUTCtrl &= ~DUT_CTRL_VCC_IO_INH;
	ui32DUTCtrl &= ~DUT_CTRL_VCC_CORE_INH;
	OSWriteHWReg32(psSysData->pvSystemRegCpuVBase, TCF_CLK_CTRL_DUT_CONTROL_1, ui32DUTCtrl);

	OSSleepms(300);
#endif

	//Take DCM, DDR, PDP1, and PDP2 out of reset
	ui32ClkResetN |= (0x1 << DDR_RESETN_SHIFT);
	ui32ClkResetN |= (0x1 << DUT_DCM_RESETN_SHIFT);
	ui32ClkResetN |= (0x1 << PDP1_RESETN_SHIFT);
	ui32ClkResetN |= (0x1 << PDP2_RESETN_SHIFT);
	OSWriteHWReg32(psSysData->pvSystemRegCpuVBase, TCF_CLK_CTRL_CLK_AND_RST_CTRL, ui32ClkResetN);

#if !defined(TC_APOLLO_BONNIE)
	//Set ODT to a specific value that seems to provide the most stable signals.
	SPI_Write(psSysData->pvSystemRegCpuVBase, 0x11, 0x413130);
#endif

	//Take DUT out of reset
	ui32ClkResetN |= (0x1 << DUT_RESETN_SHIFT);
	OSWriteHWReg32(psSysData->pvSystemRegCpuVBase, TCF_CLK_CTRL_CLK_AND_RST_CTRL, ui32ClkResetN);
	OSSleepms(100);

	//try to enable the core clock PLL
	{
		IMG_UINT32 ui32DUTResets;
		IMG_UINT32 ui32DUTGPIO1;
		IMG_UINT32 ui32PLLStatus = 0;

		/* Un-bypass the PLL on the DUT */
		SPI_Write(psSysData->pvSystemRegCpuVBase, 0x1, 0x0);
		ui32DUTResets = OSReadHWReg32(psSysData->pvSystemRegCpuVBase, 0x320);
		ui32DUTResets |= 0x1;
		OSWriteHWReg32(psSysData->pvSystemRegCpuVBase, 0x320, ui32DUTResets);
		ui32DUTResets &= 0xfffffffe;
		OSWriteHWReg32(psSysData->pvSystemRegCpuVBase, 0x320, ui32DUTResets);
		OSSleepms(1000);

		if (SPI_Read(psSysData->pvSystemRegCpuVBase, 0x2, &ui32PLLStatus) != PVRSRV_OK)
		{
			PVR_DPF((PVR_DBG_ERROR, "Unable to read PLL status"));
		}

		if (ui32PLLStatus != 0x1)
		{
			PVR_DPF((PVR_DBG_ERROR, "PLL has failed to lock, status = %x", ui32PLLStatus));
		}
		else
		{
			/* Select DUT PLL as core clock */
			ui32DUTGPIO1 = OSReadHWReg32(psSysData->pvSystemRegCpuVBase, 0x108);
			ui32DUTGPIO1 &= 0xfffffff7;
			OSWriteHWReg32(psSysData->pvSystemRegCpuVBase, 0x108, ui32DUTGPIO1);
		}

		PVR_DPF((PVR_DBG_MESSAGE, "PLL has been set to 6x"));
	}

	while (!bAlignmentOK && resetAttempts < 10)
	{
		IMG_UINT32 ui32Eyes     = 0;
		IMG_UINT32 ui32ClkTaps  = 0;
		IMG_UINT32 ui32TrainAck = 0;

		IMG_INT bank;


		resetAttempts += 1;

		//reset the DUT to allow the SAI to retrain
		ui32ClkResetN &= ~(0x1 << DUT_RESETN_SHIFT);
		OSWriteHWReg32(psSysData->pvSystemRegCpuVBase, TCF_CLK_CTRL_CLK_AND_RST_CTRL, ui32ClkResetN);
		OSWaitus(100);
		ui32ClkResetN |= (0x1 << DUT_RESETN_SHIFT);
		OSWriteHWReg32(psSysData->pvSystemRegCpuVBase, TCF_CLK_CTRL_CLK_AND_RST_CTRL, ui32ClkResetN);
		OSWaitus(100);

		//Assume alignment passed, if any bank fails on either DUT or FPGA
		//we will set this to false and try again for a max of 10 times.
		bAlignmentOK = IMG_TRUE;

		//for each of the banks
		for (bank = 0; bank < 10; bank++)
		{

			//check alignment on the DUT
			IMG_UINT32 ui32BankBase = 0x7000 + (0x1000 * bank);

			SPI_Read(psSysData->pvSystemRegCpuVBase, ui32BankBase + 0x4, &ui32Eyes);
			SPI_Read(psSysData->pvSystemRegCpuVBase, ui32BankBase + 0x3, &ui32ClkTaps);
			SPI_Read(psSysData->pvSystemRegCpuVBase, ui32BankBase + 0x6, &ui32TrainAck);

			if (!IsInterfaceAligned(ui32Eyes, ui32ClkTaps, ui32TrainAck))
			{
				bAlignmentOK = IMG_FALSE;
				break;
			}

			//check alignment on the FPGA
			ui32BankBase = 0xb0 + (0x10 * bank);

			ui32Eyes = SAI_Read(psSysData->pvSystemRegCpuVBase, ui32BankBase + 0x4);
			ui32ClkTaps = SAI_Read(psSysData->pvSystemRegCpuVBase, ui32BankBase + 0x3);
			ui32TrainAck = SAI_Read(psSysData->pvSystemRegCpuVBase, ui32BankBase + 0x6);

			if (!IsInterfaceAligned(ui32Eyes, ui32ClkTaps, ui32TrainAck))
			{
				bAlignmentOK = IMG_FALSE;
				break;
			}
		}
	}

	if (!bAlignmentOK)
	{
		PVR_DPF((PVR_DBG_ERROR, "Unable to initialise the testchip (interface alignment failure), please restart the system."));
		return PVRSRV_ERROR_INIT_FAILURE;
	}

	if (resetAttempts > 1)
	{
		PVR_LOG(("Note: The testchip required more than one reset to find a good interface alignment!"));
		PVR_LOG(("      This should be harmless, but if you do suspect foul play, please reset the machine."));
		PVR_LOG(("      If you continue to see this message you may want to report it to PowerVR Verification Platforms."));
	}

#if !defined(TC_APOLLO_BONNIE)
	/* Enable the temperature sensor */
	SPI_Write(psSysData->pvSystemRegCpuVBase, 0xc, 0); //power up
	SPI_Write(psSysData->pvSystemRegCpuVBase, 0xc, 2); //reset
	SPI_Write(psSysData->pvSystemRegCpuVBase, 0xc, 6); //init & run
#endif

	{
		IMG_UINT32 ui32HoodCtrl;
		SPI_Read(psSysData->pvSystemRegCpuVBase, 0xF, &ui32HoodCtrl);
		ui32HoodCtrl |= 0x1;
		SPI_Write(psSysData->pvSystemRegCpuVBase, 0xF, ui32HoodCtrl);
	}

	{
		IMG_UINT32 ui32RevID = OSReadHWReg32(psSysData->pvSystemRegCpuVBase, 0x10);

		IMG_UINT32 ui32BuildInc = (ui32RevID >> 12) & 0xff;

		if (ui32BuildInc)
		{
			PVR_LOG(("BE WARNED: You are not running a tagged release of the FPGA image!"));
			PVR_LOG(("Owner: 0x%01x, Inc: 0x%02x", (ui32RevID >> 20) & 0xf, ui32BuildInc));
		}

		PVR_LOG(("FPGA Release: %u.%02u", ui32RevID >> 8 & 0xf, ui32RevID & 0xff));
	}

	/* Clear all interrupts. Prevents interrupt storm if
	   device was in bad state after PCI reset. */
	OSWriteHWReg32(psSysData->pvSystemRegCpuVBase, TCF_CLK_CTRL_INTERRUPT_CLEAR, (1 << INTERRUPT_MASTER_CLEAR_SHIFT));

	return PVRSRV_OK;
}

static PVRSRV_ERROR HardResetDevice(SYS_DATA *psSysData, PVRSRV_DEVICE_CONFIG *psDevice)
{
	RGX_TIMING_INFORMATION *psTimingInfo;
	PVRSRV_ERROR eError;
	IMG_UINT32 uiValue;

	for (uiValue = 0; uiValue < APOLLO_RESET_MAX_RETRIES; uiValue++)
	{
		eError = ApolloHardReset(psSysData);
		if (eError == PVRSRV_OK)
		{
			break;
		}
	}
	PVR_LOG_RETURN_IF_ERROR(eError, "ApolloHardReset");

	psTimingInfo = ((RGX_DATA *) psDevice->hDevData)->psRGXTimingInfo;

	/* Make the core clock speed value available to the driver (matching the
	 * one set by ApolloHardReset()) */
#if defined(TC_APOLLO_BONNIE)
	/* For BonnieTC there seems to be an additional 5x multiplier that occurs to
	 * the clock as measured speed is 540Mhz not 108Mhz. */
	psTimingInfo->ui32CoreClockSpeed = ui32CoreClockSpeed * 6 * 5;
#else /* defined(TC_APOLLO_BONNIE) */
	psTimingInfo->ui32CoreClockSpeed = ui32CoreClockSpeed * 6;
#endif /* defined(TC_APOLLO_BONNIE) */

#if defined(TC_APOLLO_BONNIE)
	/* enable ASTC via SPI */
	eError = SPI_Read(psSysData->pvSystemRegCpuVBase, 0xf, &uiValue);
	PVR_LOG_IF_ERROR(eError, "SPI_Read");

	uiValue |= 0x1 << 4;

	SPI_Write(psSysData->pvSystemRegCpuVBase, 0xf, uiValue);
#endif /* defined(TC_APOLLO_BONNIE) */

	return PVRSRV_OK;
}

static PVRSRV_ERROR PCIInitDev(PVRSRV_DEVICE_CONFIG *psDevice, void *pvOSDevice)
{
	SYS_DATA *psSysData;
	IMG_CPU_PHYADDR	sApolloRegCpuPBase;
	IMG_UINT32 uiApolloRegSize;
	PVRSRV_ERROR eError;

	PVR_ASSERT(pvOSDevice);

#if defined(__linux__)
	dma_set_mask(pvOSDevice, DMA_BIT_MASK(32));
#endif

	psSysData = OSAllocZMem(sizeof(*psSysData));
	if (!psSysData)
	{
		return PVRSRV_ERROR_OUT_OF_MEMORY;
	}

	psSysData->hRGXPCI = OSPCISetDev(TO_PCI_COOKIE(pvOSDevice), HOST_PCI_INIT_FLAGS);
	if (!psSysData->hRGXPCI)
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: Failed to acquire PCI device", __func__));
		eError = PVRSRV_ERROR_PCI_DEVICE_NOT_FOUND;
		goto ErrorFreeSysData;
	}

	/* Get Apollo register information */
	sApolloRegCpuPBase.uiAddr = OSPCIAddrRangeStart(psSysData->hRGXPCI, SYS_APOLLO_REG_PCI_BASENUM);
	uiApolloRegSize	 = TRUNCATE_64BITS_TO_32BITS(OSPCIAddrRangeLen(psSysData->hRGXPCI, SYS_APOLLO_REG_PCI_BASENUM));

	/* Check the address range is large enough. */
	if (uiApolloRegSize < SYS_APOLLO_REG_REGION_SIZE)
	{
		PVR_DPF((PVR_DBG_ERROR,
			 "%s: Apollo register region isn't big enough (was 0x%08X, required 0x%08X)",
			 __func__, uiApolloRegSize, SYS_APOLLO_REG_REGION_SIZE));
		eError = PVRSRV_ERROR_PCI_REGION_TOO_SMALL;
		goto ErrorPCIReleaseDevice;
	}

	/* The Apollo register range contains several register regions. Request only the system control register region */
	eError = OSPCIRequestAddrRegion(psSysData->hRGXPCI, SYS_APOLLO_REG_PCI_BASENUM, SYS_APOLLO_REG_SYS_OFFSET, SYS_APOLLO_REG_SYS_SIZE);
	if (eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR,
				 "%s: Failed to request the system register region",
				 __func__));
		goto ErrorPCIReleaseDevice;
	}
	psSysData->sSystemRegCpuPBase.uiAddr = sApolloRegCpuPBase.uiAddr + SYS_APOLLO_REG_SYS_OFFSET;
	psSysData->uiSystemRegSize = SYS_APOLLO_REG_REGION_SIZE;

	/* Setup Rogue register information */
	psDevice->sRegsCpuPBase.uiAddr = OSPCIAddrRangeStart(psSysData->hRGXPCI, SYS_RGX_REG_PCI_BASENUM);
	psDevice->ui32RegsSize = TRUNCATE_64BITS_TO_32BITS(OSPCIAddrRangeLen(psSysData->hRGXPCI, SYS_RGX_REG_PCI_BASENUM));

	/* Save data for this device */
	psDevice->hSysData = (IMG_HANDLE)psSysData;

	/* Check the address range is large enough. */
	if (psDevice->ui32RegsSize < SYS_RGX_REG_REGION_SIZE)
	{
		PVR_DPF((PVR_DBG_ERROR,
				 "%s: Rogue register region isn't big enough "
				 "(was 0x%08x, required 0x%08x)",
				 __func__, psDevice->ui32RegsSize, SYS_RGX_REG_REGION_SIZE));
		eError = PVRSRV_ERROR_PCI_REGION_TOO_SMALL;
		goto ErrorSystemRegReleaseAddrRegion;
	}

	/* Reserve the address range */
	eError = OSPCIRequestAddrRange(psSysData->hRGXPCI, SYS_RGX_REG_PCI_BASENUM);
	if (eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR,
				 "%s: Rogue register memory region not available",
				 __func__));
		goto ErrorSystemRegReleaseAddrRegion;
	}

	/*
	   Map in system registers so we can:
	   - Configure the memory mode (LMA, UMA or LMA/UMA hybrid)
	   - Hard reset Apollo
	   - Clear interrupts
	*/
	psSysData->pvSystemRegCpuVBase = OSMapPhysToLin(psSysData->sSystemRegCpuPBase,
													psSysData->uiSystemRegSize,
													PVRSRV_MEMALLOCFLAG_CPU_UNCACHED);
	if (psSysData->pvSystemRegCpuVBase == NULL)
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: Failed to map system registers",
				 __func__));
		eError = PVRSRV_ERROR_OUT_OF_MEMORY;
		goto ErrorRGXRegReleaseAddrRange;
	}

	eError = HardResetDevice(psSysData, psDevice);
	PVR_GOTO_IF_ERROR(eError, ErrorUnMapAddrRange);

	eError = InitMemory(psDevice, psSysData);
	if (eError != PVRSRV_OK)
	{
		goto ErrorUnMapAddrRange;
	}

	psSysData->pszVersion = GetDeviceVersionString(psSysData);
	if (psSysData->pszVersion)
	{
		psDevice->pszVersion = psSysData->pszVersion;
	}

	eError = OSPCIIRQ(psSysData->hRGXPCI, &psSysData->ui32IRQ);
	if (eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: Couldn't get IRQ", __func__));
		goto ErrorDeInitMemory;
	}

	/* Register our handler */
	eError = OSInstallSystemLISR(&psSysData->hLISR,
								psSysData->ui32IRQ,
								psDevice->pszName,
								SystemISRHandler,
								psSysData,
								SYS_IRQ_FLAG_TRIGGER_DEFAULT | SYS_IRQ_FLAG_SHARED);
	if (eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR,
				 "%s: Failed to install the system device interrupt handler",
				 __func__));

		goto ErrorDeInitMemory;
	}

#if defined(SUPPORT_LMA_SUSPEND_TO_RAM) && defined(__x86_64__)
	psSysData->psDevConfig = psDevice;
#endif

	return PVRSRV_OK;

ErrorDeInitMemory:
	DeInitMemory(psDevice, psSysData);

ErrorUnMapAddrRange:
	OSUnMapPhysToLin(psSysData->pvSystemRegCpuVBase, psSysData->uiSystemRegSize);
	psSysData->pvSystemRegCpuVBase = NULL;

ErrorRGXRegReleaseAddrRange:
	OSPCIReleaseAddrRange(psSysData->hRGXPCI, SYS_RGX_REG_PCI_BASENUM);
	psDevice->sRegsCpuPBase.uiAddr	= 0;
	psDevice->ui32RegsSize		= 0;

ErrorSystemRegReleaseAddrRegion:
	OSPCIReleaseAddrRegion(psSysData->hRGXPCI, SYS_APOLLO_REG_PCI_BASENUM, SYS_APOLLO_REG_SYS_OFFSET, SYS_APOLLO_REG_SYS_SIZE);
	psSysData->sSystemRegCpuPBase.uiAddr	= 0;
	psSysData->uiSystemRegSize		= 0;

ErrorPCIReleaseDevice:
	OSPCIReleaseDev(psSysData->hRGXPCI);
	psSysData->hRGXPCI = NULL;

ErrorFreeSysData:
	OSFreeMem(psSysData);
	psDevice->hSysData = NULL;

	return eError;
}

PVRSRV_ERROR SysDebugInfo(PVRSRV_DEVICE_CONFIG *psDevice,
				DUMPDEBUG_PRINTF_FUNC *pfnDumpDebugPrintf,
				void *pvDumpDebugFile)
{
	PVRSRV_ERROR	eError;
	SYS_DATA		*psSysData = psDevice->hSysData;
	IMG_UINT32		ui32RegVal;

	PVR_DUMPDEBUG_LOG("------[ rgx_tc System Debug - Device ID:%u ]------", psDevice->psDevNode->sDevId.ui32InternalID);

#if !defined(TC_APOLLO_BONNIE)
	/* Read the temperature */
	eError = SPI_Read(psSysData->pvSystemRegCpuVBase, TCF_TEMP_SENSOR_SPI_OFFSET, &ui32RegVal);
	if (eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR, "SysDebugInfo: SPI_Read failed for register 0x%x (0x%x)", TCF_TEMP_SENSOR_SPI_OFFSET, eError));
		goto SysDebugInfo_exit;
	}

	PVR_DUMPDEBUG_LOG("Chip temperature: %d degrees C", TCF_TEMP_SENSOR_TO_C(ui32RegVal));
#endif

	eError = SPI_Read(psSysData->pvSystemRegCpuVBase, 0x2, &ui32RegVal);
	PVR_DUMPDEBUG_LOG("PLL status: %x", ui32RegVal);

#if !defined(TC_APOLLO_BONNIE)
SysDebugInfo_exit:
#endif
	return eError;
}

#else /* if defined(TC_APOLLO_ES2) else TC_APOLLO_ES1 or TC_APOLLO_TCF5 */

#define pol(base,reg,val,msk) \
	do { \
		int polnum; \
		for (polnum = 0; polnum < 500; polnum++) \
		{ \
			if ((OSReadHWReg32(base, reg) & msk) == val) \
			{ \
				break; \
			} \
			OSSleepms(1); \
		} \
		if (polnum == 500) \
		{ \
			PVR_DPF((PVR_DBG_WARNING, "Poll failed for register: 0x%08X", (unsigned int)reg)); \
		} \
	} while (0)

#define polrgx(reg,val,msk) pol(pvRegsBaseKM, reg, val, msk)

static void SPI_Write(void *pvLinRegBaseAddr,
					  IMG_UINT32 ui32Offset,
					  IMG_UINT32 ui32Value)
{
	OSWriteHWReg32(pvLinRegBaseAddr, TCF_CLK_CTRL_TCF_SPI_MST_ADDR_RDNWR, ui32Offset);
	OSWriteHWReg32(pvLinRegBaseAddr, TCF_CLK_CTRL_TCF_SPI_MST_WDATA, ui32Value);
	OSWriteHWReg32(pvLinRegBaseAddr, TCF_CLK_CTRL_TCF_SPI_MST_GO, TCF_SPI_MST_GO_MASK);
	OSWaitus(1000);

}

static PVRSRV_ERROR SPI_Read(void *pvLinRegBaseAddr,
							 IMG_UINT32 ui32Offset,
							 IMG_UINT32 *pui32Value)
{
	IMG_UINT32 ui32Count = 0;
	*pui32Value = ui32Count;

	OSWriteHWReg32(pvLinRegBaseAddr,
				   TCF_CLK_CTRL_TCF_SPI_MST_ADDR_RDNWR,
				   TCF_SPI_MST_RDNWR_MASK | ui32Offset);
	OSWriteHWReg32(pvLinRegBaseAddr, TCF_CLK_CTRL_TCF_SPI_MST_GO, TCF_SPI_MST_GO_MASK);
	OSWaitus(1000);

	while (((OSReadHWReg32(pvLinRegBaseAddr, TCF_CLK_CTRL_TCF_SPI_MST_STATUS)) != 0x08) &&
			(ui32Count < 10000))
	{
		ui32Count++;
	}

	if (ui32Count < 10000)
	{
		*pui32Value = OSReadHWReg32(pvLinRegBaseAddr, TCF_CLK_CTRL_TCF_SPI_MST_RDATA);
	}
	else
	{
		PVR_DPF((PVR_DBG_ERROR, "SPI_Read: Time out reading SPI register (0x%x)", ui32Offset));
		return PVRSRV_ERROR_TIMEOUT_POLLING_FOR_VALUE;
	}

	return PVRSRV_OK;
}

#if !defined(TC_APOLLO_TCF5)
#if !defined(VIRTUAL_PLATFORM)
/* This is a quick RGX bootup (built-in-self) test */
static void RGXInitBistery(IMG_CPU_PHYADDR sRegisters, IMG_UINT32 ui32Size)
{
	void *pvRegsBaseKM;
	IMG_UINT instance;
	IMG_UINT i;

	pvRegsBaseKM = OSMapPhysToLin(sRegisters, ui32Size, PVRSRV_MEMALLOCFLAG_CPU_UNCACHED);

	/* Force clocks on */
	OSWriteHWReg32(pvRegsBaseKM, 0, 0x55555555);
	OSWriteHWReg32(pvRegsBaseKM, 4, 0x55555555);

	polrgx(0xa18, 0x05000000, 0x05000000);
	OSWriteHWReg32(pvRegsBaseKM, 0xa10, 0x048000b0);
	OSWriteHWReg32(pvRegsBaseKM, 0xa08, 0x55111111);
	polrgx(0xa18, 0x05000000, 0x05000000);

	/* Clear PDS CSRM and USRM to prevent ERRORs at end of test */
	OSWriteHWReg32(pvRegsBaseKM, 0x630, 0x1);
	OSWriteHWReg32(pvRegsBaseKM, 0x648, 0x1);
	OSWriteHWReg32(pvRegsBaseKM, 0x608, 0x1);

	/* Run BIST for SLC (43) */
	/* Reset BIST */
	OSWriteHWReg32(pvRegsBaseKM, 0x7000, 0x8);
	OSWaitus(100);

	/* Clear BIST controller */
	OSWriteHWReg32(pvRegsBaseKM, 0x7000, 0x10);
	OSWriteHWReg32(pvRegsBaseKM, 0x7000, 0);
	OSWaitus(100);

	for (i = 0; i < 3; i++)
	{
		IMG_UINT32 ui32Pol = i == 2 ? 0x10000 : 0x20000;

		/* Start BIST */
		OSWriteHWReg32(pvRegsBaseKM, 0x7000, 0x4);

		OSWaitus(100);

		/* Wait for pause */
		polrgx(0x7000, ui32Pol, ui32Pol);
	}
	OSWaitus(100);

	/* Check results for 43 RAMs */
	polrgx(0x7010, 0xffffffff, 0xffffffff);
	polrgx(0x7014, 0x7, 0x7);

	OSWriteHWReg32(pvRegsBaseKM, 0x7000, 8);
	OSWriteHWReg32(pvRegsBaseKM, 0x7008, 0);
	OSWriteHWReg32(pvRegsBaseKM, 0x7000, 6);
	polrgx(0x7000, 0x00010000, 0x00010000);
	OSWaitus(100);

	polrgx(0x75B0, 0, ~0U);
	polrgx(0x75B4, 0, ~0U);
	polrgx(0x75B8, 0, ~0U);
	polrgx(0x75BC, 0, ~0U);
	polrgx(0x75C0, 0, ~0U);
	polrgx(0x75C4, 0, ~0U);
	polrgx(0x75C8, 0, ~0U);
	polrgx(0x75CC, 0, ~0U);

	/* Sidekick */
	OSWriteHWReg32(pvRegsBaseKM, 0x7040, 8);
	OSWaitus(100);

	OSWriteHWReg32(pvRegsBaseKM, 0x7040, 0x10);
	//OSWriteHWReg32(pvRegsBaseKM, 0x7000, 0);
	OSWaitus(100);

	for (i = 0; i < 3; i++)
	{
		IMG_UINT32 ui32Pol = i == 2 ? 0x10000 : 0x20000;

		OSWriteHWReg32(pvRegsBaseKM, 0x7040, 4);
		OSWaitus(100);
		polrgx(0x7040, ui32Pol, ui32Pol);
	}

	OSWaitus(100);
	polrgx(0x7050, 0xffffffff, 0xffffffff);
	polrgx(0x7054, 0xffffffff, 0xffffffff);
	polrgx(0x7058, 0x1, 0x1);

	/* USC */
	for (instance = 0; instance < 4; instance++)
	{
		OSWriteHWReg32(pvRegsBaseKM, 0x8010, instance);

		OSWriteHWReg32(pvRegsBaseKM, 0x7088, 8);
		OSWaitus(100);

		OSWriteHWReg32(pvRegsBaseKM, 0x7088, 0x10);
		OSWaitus(100);

		for (i = 0; i < 3; i++)
		{
			IMG_UINT32 ui32Pol = i == 2 ? 0x10000 : 0x20000;

			OSWriteHWReg32(pvRegsBaseKM, 0x7088, 4);
			OSWaitus(100);
			polrgx(0x7088, ui32Pol, ui32Pol);
		}

		OSWaitus(100);
		polrgx(0x7098, 0xffffffff, 0xffffffff);
		polrgx(0x709c, 0xffffffff, 0xffffffff);
		polrgx(0x70a0, 0x3f, 0x3f);
	}

	/* tpumcul0 DustA and DustB */
	for (instance = 0; instance < 2; instance++)
	{
		OSWriteHWReg32(pvRegsBaseKM, 0x8018, instance);

		OSWriteHWReg32(pvRegsBaseKM, 0x7380, 8);
		OSWaitus(100);

		OSWriteHWReg32(pvRegsBaseKM, 0x7380, 0x10);
		OSWaitus(100);

		for (i = 0; i < 3; i++)
		{
			IMG_UINT32 ui32Pol = i == 2 ? 0x10000 : 0x20000;

			OSWriteHWReg32(pvRegsBaseKM, 0x7380, 4);
			OSWaitus(100);
			polrgx(0x7380, ui32Pol, ui32Pol);
		}

		OSWaitus(100);
		polrgx(0x7390, 0x1fff, 0x1fff);
	}

	/* TA */
	OSWriteHWReg32(pvRegsBaseKM, 0x7500, 8);
	OSWaitus(100);

	OSWriteHWReg32(pvRegsBaseKM, 0x7500, 0x10);
	OSWaitus(100);

	for (i = 0; i < 3; i++)
	{
		IMG_UINT32 ui32Pol = i == 2 ? 0x10000 : 0x20000;

		OSWriteHWReg32(pvRegsBaseKM, 0x7500, 4);
		OSWaitus(100);
		polrgx(0x7500, ui32Pol, ui32Pol);
	}

	OSWaitus(100);
	polrgx(0x7510, 0x1fffffff, 0x1fffffff);

	/* Rasterisation */
	OSWriteHWReg32(pvRegsBaseKM, 0x7540, 8);
	OSWaitus(100);

	OSWriteHWReg32(pvRegsBaseKM, 0x7540, 0x10);
	OSWaitus(100);

	for (i = 0; i < 3; i++)
	{
		IMG_UINT32 ui32Pol = i == 2 ? 0x10000 : 0x20000;

		OSWriteHWReg32(pvRegsBaseKM, 0x7540, 4);
		OSWaitus(100);
		polrgx(0x7540, ui32Pol, ui32Pol);
	}

	OSWaitus(100);
	polrgx(0x7550, 0xffffffff, 0xffffffff);
	polrgx(0x7554, 0xffffffff, 0xffffffff);
	polrgx(0x7558, 0xf, 0xf);

	/* hub_bifpmache */
	OSWriteHWReg32(pvRegsBaseKM, 0x7588, 8);
	OSWaitus(100);

	OSWriteHWReg32(pvRegsBaseKM, 0x7588, 0x10);
	OSWaitus(100);

	for (i = 0; i < 3; i++)
	{
		IMG_UINT32 ui32Pol = i == 2 ? 0x10000 : 0x20000;

		OSWriteHWReg32(pvRegsBaseKM, 0x7588, 4);
		OSWaitus(100);
		polrgx(0x7588, ui32Pol, ui32Pol);
	}

	OSWaitus(100);
	polrgx(0x7598, 0xffffffff, 0xffffffff);
	polrgx(0x759c, 0xffffffff, 0xffffffff);
	polrgx(0x75a0, 0x1111111f, 0x1111111f);

	OSUnMapPhysToLin(pvRegsBaseKM, ui32Size);
}
#endif /* !defined(VIRTUAL_PLATFORM) */
#endif /* !defined(TC_APOLLO_TCF5) */

#if defined(TC_APOLLO_TCF5)
static void ApolloFPGAUpdateDUTClockFreq(SYS_DATA   *psSysData,
                                         IMG_UINT32 *pui32CoreClockSpeed,
                                         IMG_UINT32 *pui32MemClockSpeed)
{
#if defined(SUPPORT_FPGA_DUT_CLK_INFO)
	IMG_UINT32 ui32Reg;

	/* DUT_CLK_INFO available only if SW_IF_VERSION >= 1 */
	ui32Reg = OSReadHWReg32(psSysData->pvSystemRegCpuVBase, TCF_CLK_CTRL_SW_IF_VERSION);
	ui32Reg = (ui32Reg & VERSION_MASK) >> VERSION_SHIFT;

	if (ui32Reg >= 1)
	{
		ui32Reg = OSReadHWReg32(psSysData->pvSystemRegCpuVBase, TCF_CLK_CTRL_DUT_CLK_INFO);

		if ((ui32Reg != 0) && (ui32Reg != 0xbaadface) && (ui32Reg != 0xffffffff))
		{
			PVR_LOG(("TCF_CLK_CTRL_DUT_CLK_INFO = %08x", ui32Reg));
			PVR_LOG(("Overriding provided DUT clock values: core %u, mem %u",
			         *pui32CoreClockSpeed, *pui32MemClockSpeed));

			*pui32CoreClockSpeed = ((ui32Reg & CORE_MASK) >> CORE_SHIFT) * 1000000;
			*pui32MemClockSpeed = ((ui32Reg & MEM_MASK) >> MEM_SHIFT) * 1000000;
		}
	}
#endif

	PVR_LOG(("DUT clock values: core %u, mem %u",
	         *pui32CoreClockSpeed, *pui32MemClockSpeed));
}
#endif

static PVRSRV_ERROR SetClocks(SYS_DATA *psSysData,
							  IMG_UINT32 ui32CoreClock,
							  IMG_UINT32 ui32MemClock,
							  IMG_UINT32 ui32SysClock)
{
	IMG_CPU_PHYADDR	sPLLRegCpuPBase;
	void *pvPLLRegCpuVBase;
	IMG_UINT32 ui32Value;
	PVRSRV_ERROR eError;

	/* Reserve the PLL register region and map the registers in */
	eError = OSPCIRequestAddrRegion(psSysData->hRGXPCI, SYS_APOLLO_REG_PCI_BASENUM, SYS_APOLLO_REG_PLL_OFFSET, SYS_APOLLO_REG_PLL_SIZE);
	if (eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR,
				 "%s: Failed to request the PLL register region (%d)",
				 __func__, eError));
		return eError;
	}
	sPLLRegCpuPBase.uiAddr = OSPCIAddrRangeStart(psSysData->hRGXPCI, SYS_APOLLO_REG_PCI_BASENUM) + SYS_APOLLO_REG_PLL_OFFSET;

	pvPLLRegCpuVBase = OSMapPhysToLin(sPLLRegCpuPBase, SYS_APOLLO_REG_PLL_SIZE, PVRSRV_MEMALLOCFLAG_CPU_UNCACHED);
	if (pvPLLRegCpuVBase == NULL)
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: Failed to map PLL registers", __func__));
		return PVRSRV_ERROR_OUT_OF_MEMORY;
	}

	/* Modify the core clock */
	OSWriteHWReg32(pvPLLRegCpuVBase, TCF_PLL_PLL_CORE_CLK0, (ui32CoreClock / 1000000));

	ui32Value = 0x1 << PLL_CORE_DRP_GO_SHIFT;
	OSWriteHWReg32(pvPLLRegCpuVBase, TCF_PLL_PLL_CORE_DRP_GO, ui32Value);

	OSSleepms(600);

	/* Modify the memory clock */
	OSWriteHWReg32(pvPLLRegCpuVBase, TCF_PLL_PLL_MEMIF_CLK0, (ui32MemClock / 1000000));

	ui32Value = 0x1 << PLL_MEM_DRP_GO_SHIFT;
	OSWriteHWReg32(pvPLLRegCpuVBase, TCF_PLL_PLL_MEM_DRP_GO, ui32Value);

	OSSleepms(600);
#if defined(TC_APOLLO_TCF5)
	/* Modify the sys clock */
	OSWriteHWReg32(pvPLLRegCpuVBase, TCF_PLL_PLL_SYSIF_CLK0, (ui32SysClock / 1000000));

	ui32Value = 0x1 << PLL_MEM_DRP_GO_SHIFT;
	OSWriteHWReg32(pvPLLRegCpuVBase, TCF_PLL_PLL_SYS_DRP_GO, ui32Value);

	OSSleepms(600);
#else
	PVR_UNREFERENCED_PARAMETER(ui32SysClock);
#endif

	/* Unmap and release the PLL registers since we no longer need access to them */
	OSUnMapPhysToLin(pvPLLRegCpuVBase, SYS_APOLLO_REG_PLL_SIZE);
	OSPCIReleaseAddrRegion(psSysData->hRGXPCI, SYS_APOLLO_REG_PCI_BASENUM, SYS_APOLLO_REG_PLL_OFFSET, SYS_APOLLO_REG_PLL_SIZE);

	return PVRSRV_OK;
}

static void ApolloHardReset(SYS_DATA *psSysData)
{
	IMG_UINT32 ui32Value;
	IMG_UINT32 ui32PolValue;

#if defined(TC_APOLLO_TCF5)
#if defined(TC_APOLLO_TCF5_BVNC_NOT_SUPPORTED)
	PVR_DPF((PVR_DBG_WARNING,"\n********************************************************************************"));
	PVR_DPF((PVR_DBG_WARNING, "%s: You are using a core which is not supported on TCF5.", __func__));
	PVR_DPF((PVR_DBG_WARNING, "A set of fallback clock frequencies will be used. Depending on the core, you"));
	PVR_DPF((PVR_DBG_WARNING, "could experience some problems (driver running slower, failing after some time"));
	PVR_DPF((PVR_DBG_WARNING, "or unable to start at all)."));
	PVR_DPF((PVR_DBG_WARNING, "********************************************************************************\n"));
#endif

	/* Put everything under reset */
	ui32Value = (0x1 << GLB_CLKG_EN_SHIFT);
	OSWriteHWReg32(psSysData->pvSystemRegCpuVBase, TCF_CLK_CTRL_CLK_AND_RST_CTRL, ui32Value);

	OSSleepms(4);

	ApolloFPGAUpdateDUTClockFreq(psSysData, &ui32CoreClockSpeed, &ui32MemClockSpeed);

	if (SetClocks(psSysData, ui32CoreClockSpeed, ui32MemClockSpeed, ui32SysClockSpeed) != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR,
				 "%s: Failed to set the core/system/memory clocks",
				 __func__));
	}

	/* Remove units from reset */
	ui32Value |= (0x1 << DUT_DCM_RESETN_SHIFT);
	OSWriteHWReg32(psSysData->pvSystemRegCpuVBase, TCF_CLK_CTRL_CLK_AND_RST_CTRL, ui32Value);

	ui32Value |= (0x1 << SCB_RESETN_SHIFT);
	OSWriteHWReg32(psSysData->pvSystemRegCpuVBase, TCF_CLK_CTRL_CLK_AND_RST_CTRL, ui32Value);

	ui32Value |= (0x1 << PDP2_RESETN_SHIFT);
	ui32Value |= (0x1 << PDP1_RESETN_SHIFT);
	ui32Value |= (0x1 << DDR_RESETN_SHIFT);
	OSWriteHWReg32(psSysData->pvSystemRegCpuVBase, TCF_CLK_CTRL_CLK_AND_RST_CTRL, ui32Value);

	ui32Value |= (0x1 << DUT_RESETN_SHIFT);
	OSWriteHWReg32(psSysData->pvSystemRegCpuVBase, TCF_CLK_CTRL_CLK_AND_RST_CTRL, ui32Value);

	ui32PolValue = 0x4;
#else
	ui32Value = (0x1 << GLB_CLKG_EN_SHIFT);
	ui32Value |= (0x1 << SCB_RESETN_SHIFT);
	ui32Value |= (0x1 << PDP2_RESETN_SHIFT);
	ui32Value |= (0x1 << PDP1_RESETN_SHIFT);
	OSWriteHWReg32(psSysData->pvSystemRegCpuVBase, TCF_CLK_CTRL_CLK_AND_RST_CTRL, ui32Value);

	ui32Value |= (0x1 << DDR_RESETN_SHIFT);
	OSWriteHWReg32(psSysData->pvSystemRegCpuVBase, TCF_CLK_CTRL_CLK_AND_RST_CTRL, ui32Value);

	ui32Value |= (0x1 << DUT_RESETN_SHIFT);
	OSWriteHWReg32(psSysData->pvSystemRegCpuVBase, TCF_CLK_CTRL_CLK_AND_RST_CTRL, ui32Value);

	ui32Value |= (0x1 << DUT_DCM_RESETN_SHIFT);
	OSWriteHWReg32(psSysData->pvSystemRegCpuVBase, TCF_CLK_CTRL_CLK_AND_RST_CTRL, ui32Value);

	ui32PolValue = 0x7;
#endif

	OSSleepms(4);

	pol(psSysData->pvSystemRegCpuVBase, TCF_CLK_CTRL_DCM_LOCK_STATUS, ui32PolValue, DCM_LOCK_STATUS_MASK);
#undef pol
#undef polrgx
}

#if defined(TC_APOLLO_TCF5)
static void SetMemoryLatency(SYS_DATA *psSysData)
{
	IMG_UINT32 ui32Latency = 0, ui32RegVal = 0;
	PVRSRV_ERROR eError;

	if (ui32MemReadLatency <= 4)
	{
		/* The total memory read latency cannot be lower than the
		 * amount of cycles consumed by the hardware to do a read.
		 * Set the memory read latency to 0 cycles.
		 */
		ui32MemReadLatency = 0;
	}
	else
	{
		ui32MemReadLatency -= 4;

		PVR_LOG(("%s: Setting memory read latency to %u cycles",
		         __func__, ui32MemReadLatency));
	}

	if (ui32MemWriteResponseLatency <= 2)
	{
		/* The total memory write latency cannot be lower than the
		 * amount of cycles consumed by the hardware to do a write.
		 * Set the memory write latency to 0 cycles.
		 */
		ui32MemWriteResponseLatency = 0;
	}
	else
	{
		ui32MemWriteResponseLatency -= 2;

		PVR_LOG(("%s: Setting memory write response latency to %u cycles",
		         __func__, ui32MemWriteResponseLatency));
	}

	ui32Latency = (ui32MemWriteResponseLatency << 16) | ui32MemReadLatency;

	/* Configure latency register */
	SPI_Write(psSysData->pvSystemRegCpuVBase, 0x1009, ui32Latency);

	/* Read back latency register */
	eError = SPI_Read(psSysData->pvSystemRegCpuVBase, 0x1009, &ui32RegVal);

	if (eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: SPI_Read failed (%u)", __func__, eError));
	}

	if (ui32RegVal != ui32Latency)
	{
		PVR_DPF((PVR_DBG_ERROR,
		         "%s: Register value mismatch (actual: 0x%08x, expected: 0x%08x)",
		         __func__, ui32RegVal, ui32Latency));
	}
}
#endif

static PVRSRV_ERROR PCIInitDev(PVRSRV_DEVICE_CONFIG *psDevice, void *pvOSDevice)
{
	SYS_DATA *psSysData;
	IMG_CPU_PHYADDR	sApolloRegCpuPBase;
	IMG_UINT32 uiApolloRegSize;
	PVRSRV_ERROR eError;

	PVR_ASSERT(psDevice);

#if defined(__linux__)
	dma_set_mask(pvOSDevice, DMA_BIT_MASK(32));
#endif

	psSysData = OSAllocZMem(sizeof(*psSysData));
	if (!psSysData)
	{
		return PVRSRV_ERROR_OUT_OF_MEMORY;
	}

	psSysData->hRGXPCI = OSPCISetDev(TO_PCI_COOKIE(pvOSDevice), HOST_PCI_INIT_FLAGS);
	if (!psSysData->hRGXPCI)
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: Failed to acquire PCI device", __func__));
		eError = PVRSRV_ERROR_PCI_DEVICE_NOT_FOUND;
		goto ErrorFreeSysData;
	}

	/* Get Apollo register information */
	sApolloRegCpuPBase.uiAddr = OSPCIAddrRangeStart(psSysData->hRGXPCI, SYS_APOLLO_REG_PCI_BASENUM);
	uiApolloRegSize = TRUNCATE_64BITS_TO_32BITS(OSPCIAddrRangeLen(psSysData->hRGXPCI, SYS_APOLLO_REG_PCI_BASENUM));

	/* Check the address range is large enough. */
	if (uiApolloRegSize < SYS_APOLLO_REG_REGION_SIZE)
	{
		PVR_DPF((PVR_DBG_ERROR,
				 "%s: Apollo register region isn't big enough "
				 "(was 0x%08X, required 0x%08X)",
				 __func__, uiApolloRegSize, SYS_APOLLO_REG_REGION_SIZE));
		eError = PVRSRV_ERROR_PCI_REGION_TOO_SMALL;
		goto ErrorPCIReleaseDevice;
	}

	/* The Apollo register range contains several register regions. Request only the system control register region */
	eError = OSPCIRequestAddrRegion(psSysData->hRGXPCI, SYS_APOLLO_REG_PCI_BASENUM, SYS_APOLLO_REG_SYS_OFFSET, SYS_APOLLO_REG_SYS_SIZE);
	if (eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR,
				 "%s: Failed to request the system register region",
				 __func__));
		goto ErrorPCIReleaseDevice;
	}
	psSysData->sSystemRegCpuPBase.uiAddr = sApolloRegCpuPBase.uiAddr + SYS_APOLLO_REG_SYS_OFFSET;
	psSysData->uiSystemRegSize = SYS_APOLLO_REG_REGION_SIZE;

	/* Setup Rogue register information */
	psDevice->sRegsCpuPBase.uiAddr = OSPCIAddrRangeStart(psSysData->hRGXPCI, SYS_RGX_REG_PCI_BASENUM);
	psDevice->ui32RegsSize = TRUNCATE_64BITS_TO_32BITS(OSPCIAddrRangeLen(psSysData->hRGXPCI, SYS_RGX_REG_PCI_BASENUM));

	/* Save data for this device */
	psDevice->hSysData = (IMG_HANDLE)psSysData;

	/* Check the address range is large enough. */
	if (psDevice->ui32RegsSize < SYS_RGX_REG_REGION_SIZE)
	{
		PVR_DPF((PVR_DBG_ERROR,
				 "%s: Rogue register region isn't big enough "
				 "(was 0x%08x, required 0x%08x)",
				 __func__, psDevice->ui32RegsSize, SYS_RGX_REG_REGION_SIZE));
		eError = PVRSRV_ERROR_PCI_REGION_TOO_SMALL;
		goto ErrorSystemRegReleaseAddrRegion;
	}

	/* Reserve the address range */
	eError = OSPCIRequestAddrRange(psSysData->hRGXPCI, SYS_RGX_REG_PCI_BASENUM);
	if (eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR,
				 "%s: Rogue register memory region not available",
				 __func__));
		goto ErrorSystemRegReleaseAddrRegion;
	}

	/*
	   Map in system registers so we can:
	   - Configure the memory mode (LMA, UMA or LMA/UMA hybrid)
	   - Hard reset Apollo
	   - Run BIST
	   - Clear interrupts
	*/
	psSysData->pvSystemRegCpuVBase = OSMapPhysToLin(psSysData->sSystemRegCpuPBase,
													psSysData->uiSystemRegSize,
													PVRSRV_MEMALLOCFLAG_CPU_UNCACHED);
	if (psSysData->pvSystemRegCpuVBase == NULL)
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: Failed to map system registers",
				 __func__));
		eError = PVRSRV_ERROR_OUT_OF_MEMORY;
		goto ErrorRGXRegReleaseAddrRange;
	}

	ApolloHardReset(psSysData);

#if defined(TC_APOLLO_TCF5)
	SetMemoryLatency(psSysData);
#endif

#if !defined(TC_APOLLO_TCF5)
#if !defined(VIRTUAL_PLATFORM)
	RGXInitBistery(psDevice->sRegsCpuPBase, psDevice->ui32RegsSize);
	ApolloHardReset(psSysData);
#endif
	if (SetClocks(psSysData, ui32CoreClockSpeed, ui32MemClockSpeed, ui32SysClockSpeed) != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR,
				 "%s: Failed to set the core/system/memory clocks",
				 __func__));
	}
#endif

	eError = InitMemory(psDevice, psSysData);
	if (eError != PVRSRV_OK)
	{
		goto ErrorUnMapAddrRange;
	}

#if defined(TC_APOLLO_TCF5)
	((RGX_DATA *)psDevice->hDevData)->psRGXTimingInfo->ui32CoreClockSpeed = ui32CoreClockSpeed;
	OSSleepms(600);
#else
	{
		IMG_UINT32 ui32Value;
		/* Enable the rogue PLL (defaults to 3x), giving a Rogue clock of 3 x RGX_TC_CORE_CLOCK_SPEED */
		ui32Value = OSReadHWReg32(psSysData->pvSystemRegCpuVBase, TCF_CLK_CTRL_DUT_CONTROL_1);
		OSWriteHWReg32(psSysData->pvSystemRegCpuVBase, TCF_CLK_CTRL_DUT_CONTROL_1, ui32Value & 0xFFFFFFFB);

		((RGX_DATA *)psDevice->hDevData)->psRGXTimingInfo->ui32CoreClockSpeed = ui32CoreClockSpeed * 3;

		OSSleepms(600);

		/* Enable the temperature sensor */
		SPI_Write(psSysData->pvSystemRegCpuVBase, 0x3, 0x46);
	}

	{
		IMG_UINT32 ui32HoodCtrl;
		SPI_Read(psSysData->pvSystemRegCpuVBase, 0x9, &ui32HoodCtrl);
		ui32HoodCtrl |= 0x1;
		SPI_Write(psSysData->pvSystemRegCpuVBase, 0x9, ui32HoodCtrl);
	}
#endif

	psSysData->pszVersion = GetDeviceVersionString(psSysData);
	if (psSysData->pszVersion)
	{
		psDevice->pszVersion = psSysData->pszVersion;
	}

	eError = OSPCIIRQ(psSysData->hRGXPCI, &psSysData->ui32IRQ);
	if (eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: Couldn't get IRQ", __func__));
		goto ErrorDeInitMemory;
	}

	/* Register our handler */
	eError = OSInstallSystemLISR(&psSysData->hLISR,
								psSysData->ui32IRQ,
								psDevice->pszName,
								SystemISRHandler,
								psSysData,
								SYS_IRQ_FLAG_TRIGGER_DEFAULT | SYS_IRQ_FLAG_SHARED);
	if (eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR,
				 "%s: Failed to install the system device interrupt handler",
				 __func__));
		goto ErrorDeInitMemory;
	}

	return PVRSRV_OK;

ErrorDeInitMemory:
	DeInitMemory(psDevice, psSysData);

ErrorUnMapAddrRange:
	OSUnMapPhysToLin(psSysData->pvSystemRegCpuVBase, psSysData->uiSystemRegSize);
	psSysData->pvSystemRegCpuVBase = NULL;

ErrorRGXRegReleaseAddrRange:
	OSPCIReleaseAddrRange(psSysData->hRGXPCI, SYS_RGX_REG_PCI_BASENUM);
	psDevice->sRegsCpuPBase.uiAddr	= 0;
	psDevice->ui32RegsSize		= 0;

ErrorSystemRegReleaseAddrRegion:
	OSPCIReleaseAddrRegion(psSysData->hRGXPCI, SYS_APOLLO_REG_PCI_BASENUM, SYS_APOLLO_REG_SYS_OFFSET, SYS_APOLLO_REG_SYS_SIZE);
	psSysData->sSystemRegCpuPBase.uiAddr	= 0;
	psSysData->uiSystemRegSize		= 0;

ErrorPCIReleaseDevice:
	OSPCIReleaseDev(psSysData->hRGXPCI);
	psSysData->hRGXPCI = NULL;

ErrorFreeSysData:
	OSFreeMem(psSysData);
	psDevice->hSysData = NULL;

	return eError;
}

PVRSRV_ERROR SysDebugInfo(PVRSRV_DEVICE_CONFIG *psDevice,
				DUMPDEBUG_PRINTF_FUNC *pfnDumpDebugPrintf,
				void *pvDumpDebugFile)
{
#if defined(TC_APOLLO_TCF5)
	PVR_UNREFERENCED_PARAMETER(psDevice);
	PVR_UNREFERENCED_PARAMETER(pfnDumpDebugPrintf);
	return PVRSRV_OK;
#else
	PVRSRV_ERROR	eError;
	SYS_DATA		*psSysData = psDevice->hSysData;
	IMG_UINT32		ui32RegOffset;
	IMG_UINT32		ui32RegVal;

	PVR_DUMPDEBUG_LOG("------[ rgx_tc system debug ]------");

	/* Read the temperature */
	ui32RegOffset = 0x5;
	eError = SPI_Read(psSysData->pvSystemRegCpuVBase, ui32RegOffset, &ui32RegVal);
	if (eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR, "SysDebugInfo: SPI_Read failed for register 0x%x (0x%x)", ui32RegOffset, eError));
		goto SysDebugInfo_exit;
	}

	PVR_DUMPDEBUG_LOG("Chip temperature: %d degrees C", (ui32RegVal * 233 / 4096) - 66);

SysDebugInfo_exit:
	return eError;
#endif
}
#endif /* if defined(TC_APOLLO_ES2) else TC_APOLLO_TCF5 */

/*
 * GetMemorySize:
 * For a given PCI phys addr and size, go through the memory to check the actual size.
 * Do this by writing to address zero, and then checking positions in the memory to see if there
 * is any aliasing. e.g. some devices may advertise 1GB of memory but in reality only have half that.
 */
size_t GetMemorySize(IMG_CPU_PHYADDR sMemCpuPhyAddr, IMG_UINT64 pciBarSize)
{
#define MEM_PROBE_VALUE 0xa5a5a5a5
	void *pvHostMapped;
	volatile IMG_UINT32* pMemZero;
	size_t mem_size = 0;

	PVR_ASSERT((pciBarSize & (pciBarSize-1)) == 0);   // PCI BARs are always a power of 2
	pvHostMapped = OSMapPhysToLin(sMemCpuPhyAddr, pciBarSize, PVRSRV_MEMALLOCFLAG_CPU_UNCACHED);
	if (pvHostMapped == NULL)
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: Cannot map PCI memory at phys 0x%"
				IMG_UINT64_FMTSPECx" for size 0x%"IMG_UINT64_FMTSPECx,
				__func__, (IMG_UINT64) sMemCpuPhyAddr.uiAddr, pciBarSize));
	}
	else
	{
		pMemZero = (IMG_UINT32*)pvHostMapped;
		mem_size = 1024*1024;    // assume at least 1MB of memory
		while (mem_size < pciBarSize)
		{
			volatile IMG_UINT32* pMemProbe = pMemZero + mem_size/4;

			// check that the memory at offset actually exists and is not aliased
			*pMemProbe = MEM_PROBE_VALUE;
			*pMemZero = 0;
			if (*pMemProbe != MEM_PROBE_VALUE)
				break;
			*pMemProbe = 0;     // reset value
			mem_size <<= 1;     // double the offset we probe each time.
		}
		OSUnMapPhysToLin(pvHostMapped, pciBarSize);
		PVR_DPF((PVR_DBG_MESSAGE, "%s: Local memory size 0x" IMG_SIZE_FMTSPECX " (PCI BAR size 0x%" IMG_UINT64_FMTSPECx ")", __func__, mem_size, pciBarSize));
	}

	return mem_size;
}

#if (TC_MEMORY_CONFIG == TC_MEMORY_LOCAL) || (TC_MEMORY_CONFIG == TC_MEMORY_HYBRID)
static PVRSRV_ERROR AcquireLocalMemory(SYS_DATA *psSysData, IMG_CPU_PHYADDR *psMemCpuPAddr, size_t *puiMemSize)
{
	IMG_UINT16 uiVendorID, uiDevID;
	PVRSRV_ERROR eError;
	IMG_UINT32 uiMemSize;
	IMG_UINT32 ui32Value;
	IMG_UINT32 uiMemLimit;
	IMG_UINT32 ui32PCIVersion;
	void *pvHostFPGARegCpuVBase;
	IMG_CPU_PHYADDR	sHostFPGARegCpuPBase;

	OSPCIGetVendorDeviceIDs(psSysData->hRGXPCI, &uiVendorID, &uiDevID);
	if (uiDevID != SYS_RGX_DEV_DEVICE_ID)
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: Unexpected device ID 0x%X",
				 __func__, uiDevID));
		return PVRSRV_ERROR_PCI_DEVICE_NOT_FOUND;
	}

	/* To get some of the version information we need to read from a register that we don't normally have
	   mapped. Map it temporarily (without trying to reserve it) to get the information we need. */
	sHostFPGARegCpuPBase.uiAddr	= OSPCIAddrRangeStart(psSysData->hRGXPCI, SYS_APOLLO_REG_PCI_BASENUM) + 0x40F0;
	pvHostFPGARegCpuVBase = OSMapPhysToLin(sHostFPGARegCpuPBase, 0x04, PVRSRV_MEMALLOCFLAG_CPU_UNCACHED);

	ui32Value = OSReadHWReg32(pvHostFPGARegCpuVBase, 0);

	/* Unmap the register now that we no longer need it */
	OSUnMapPhysToLin(pvHostFPGARegCpuVBase, 0x04);

	ui32PCIVersion = HEX2DEC((ui32Value & 0x00FF0000) >> 16);

	if (ui32PCIVersion < 18)
	{
		PVR_DPF((PVR_DBG_WARNING,"\n********************************************************************************"));
		PVR_DPF((PVR_DBG_WARNING, "%s: You have an outdated test chip fpga image.", __func__));
		PVR_DPF((PVR_DBG_WARNING, "Restricting available graphics memory to 512mb as a workaround."));
		PVR_DPF((PVR_DBG_WARNING, "This restriction may cause test failures for those tests"));
		PVR_DPF((PVR_DBG_WARNING, "that require a large amount of graphics memory."));
		PVR_DPF((PVR_DBG_WARNING, "Please speak to your customer support representative about"));
		PVR_DPF((PVR_DBG_WARNING, "upgrading your test chip fpga image."));
		PVR_DPF((PVR_DBG_WARNING, "********************************************************************************\n"));

		/* limit to 512mb */
		uiMemLimit = 512 * 1024 * 1024;
		uiMemSize = TRUNCATE_64BITS_TO_32BITS(OSPCIAddrRangeLen(psSysData->hRGXPCI, SYS_DEV_MEM_PCI_BASENUM));
	}
	else
	{
		IMG_CPU_PHYADDR sMemCpuPhyAddr;
		IMG_UINT64 pciBarSize = OSPCIAddrRangeLen(psSysData->hRGXPCI, SYS_DEV_MEM_PCI_BASENUM);
		sMemCpuPhyAddr.uiAddr = IMG_CAST_TO_CPUPHYADDR_UINT(OSPCIAddrRangeStart(psSysData->hRGXPCI, SYS_DEV_MEM_PCI_BASENUM));

		uiMemSize = TRUNCATE_64BITS_TO_32BITS(GetMemorySize(sMemCpuPhyAddr, pciBarSize) - SYS_DEV_MEM_BROKEN_BYTES);
		uiMemLimit = SYS_DEV_MEM_REGION_SIZE;
	}

	if (uiMemLimit > uiMemSize)
	{
		PVR_DPF((PVR_DBG_WARNING,
				 "%s: Device memory region smaller than requested "
				 "(got 0x%08X, requested 0x%08X)",
				 __func__, uiMemSize, uiMemLimit));
	}
	else if (uiMemLimit < uiMemSize)
	{
		PVR_DPF((PVR_DBG_WARNING,
				 "%s: Limiting device memory region size to 0x%08X from 0x%08X",
				 __func__, uiMemLimit, uiMemSize));
		uiMemSize = uiMemLimit;
	}

	/* Reserve the address region */
	eError = OSPCIRequestAddrRegion(psSysData->hRGXPCI, SYS_DEV_MEM_PCI_BASENUM, 0, uiMemSize);
	if (eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR,
				"%s: Device memory region not available - 0..0x%x",
				 __func__, uiMemSize));
		return eError;
	}

	/* Clear any BIOS-configured MTRRs */
	eError = OSPCIClearResourceMTRRs(psSysData->hRGXPCI, SYS_DEV_MEM_PCI_BASENUM);
	if (eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_WARNING, "%s: Failed to clear BIOS MTRRs",
				 __func__));
		/* Soft-fail, the driver can limp along. */
	}

	psMemCpuPAddr->uiAddr = IMG_CAST_TO_CPUPHYADDR_UINT(OSPCIAddrRangeStart(psSysData->hRGXPCI, SYS_DEV_MEM_PCI_BASENUM));
	*puiMemSize = (size_t)uiMemSize;

	return PVRSRV_OK;
}

static INLINE void ReleaseLocalMemory(SYS_DATA *psSysData, IMG_CPU_PHYADDR *psMemCpuPAddr, size_t uiMemSize)
{
	IMG_CPU_PHYADDR sMemCpuPBaseAddr;
	IMG_UINT32 uiOffset;

	sMemCpuPBaseAddr.uiAddr = IMG_CAST_TO_CPUPHYADDR_UINT(OSPCIAddrRangeStart(psSysData->hRGXPCI, SYS_DEV_MEM_PCI_BASENUM));

	PVR_ASSERT(psMemCpuPAddr->uiAddr >= sMemCpuPBaseAddr.uiAddr);

	uiOffset = (IMG_UINT32)(uintptr_t)(psMemCpuPAddr->uiAddr - sMemCpuPBaseAddr.uiAddr);

	OSPCIReleaseAddrRegion(psSysData->hRGXPCI, SYS_DEV_MEM_PCI_BASENUM, uiOffset, (IMG_UINT32)uiMemSize);

	OSPCIReleaseResourceMTRRs(psSysData->hRGXPCI, SYS_DEV_MEM_PCI_BASENUM);
}

static inline
IMG_CHAR* GetHeapName(PHYS_HEAP_USAGE_FLAGS ui32Flags)
{
	if (BITMASK_HAS(ui32Flags,PHYS_HEAP_USAGE_GPU_LOCAL))   return "lma_gpu_local";
	if (BITMASK_HAS(ui32Flags,PHYS_HEAP_USAGE_CPU_LOCAL))   return "lma_cpu_local";
	if (BITMASK_HAS(ui32Flags,PHYS_HEAP_USAGE_GPU_PRIVATE)) return "lma_gpu_priv";
	if (BITMASK_HAS(ui32Flags,PHYS_HEAP_USAGE_DISPLAY))     return "lma_gpu_display";
	if (BITMASK_HAS(ui32Flags,PHYS_HEAP_USAGE_FW_SHARED))   return "lma_fw_shared";
	else                                                    return "Unexpected Heap";
}

static void
InitLocalHeap(PHYS_HEAP_CONFIG *psPhysHeap,
			  IMG_UINT64 uiGpuPhysicalBaseAddr,
			  IMG_UINT64 uiCpuPhysicalBaseAddr,
			  IMG_UINT64 uiSize,
			  IMG_HANDLE hPrivData,
			  PHYS_HEAP_FUNCTIONS *psFuncs,
			  PHYS_HEAP_USAGE_FLAGS ui32Flags)
{
	psPhysHeap->eType = PHYS_HEAP_TYPE_LMA;
	psPhysHeap->ui32UsageFlags = ui32Flags;
	psPhysHeap->uConfig.sLMA.pszPDumpMemspaceName = "LMA";
	psPhysHeap->uConfig.sLMA.psMemFuncs = psFuncs;
	psPhysHeap->uConfig.sLMA.pszHeapName = GetHeapName(ui32Flags);
	psPhysHeap->uConfig.sLMA.sStartAddr.uiAddr = IMG_CAST_TO_CPUPHYADDR_UINT(uiCpuPhysicalBaseAddr);
	psPhysHeap->uConfig.sLMA.sCardBase.uiAddr = uiGpuPhysicalBaseAddr;
	psPhysHeap->uConfig.sLMA.uiSize = uiSize;
	psPhysHeap->uConfig.sLMA.hPrivData = hPrivData;
}

static INLINE PVRSRV_ERROR InitLocalMemoryHeaps(PVRSRV_DEVICE_CONFIG *psDevConfig, SYS_DATA *psSysData, IMG_UINT32 *pui32HeapNumOut)
{
	IMG_UINT64 ui64GpuPhysicalBase = 0;
	IMG_UINT64 ui64CpuPhysicalBase = 0;
	IMG_UINT64 uiHeapSize;
	IMG_UINT64 uiOrigHeapSize;
	PHYS_HEAP_FUNCTIONS *psHeapFuncs;
	IMG_UINT32 ui32HeapNum = 0;
	PVRSRV_ERROR eError;
	PHYS_HEAP_CONFIG *psPhysHeap = NULL;
	IMG_UINT64 uiFwCarveoutSize;
	IMG_UINT64 uiReserved;

	eError = AcquireLocalMemory(psSysData, &psSysData->sLocalMemCpuPBase, &psSysData->uiLocalMemSize);
	if (eError != PVRSRV_OK)
	{
		return eError;
	}

#if (TC_MEMORY_CONFIG == TC_MEMORY_HYBRID)
	ui64GpuPhysicalBase = psSysData->sLocalMemCpuPBase.uiAddr;
	psHeapFuncs = &gsHybridPhysHeapFuncs;
#else
	ui64GpuPhysicalBase = 0U;
	psHeapFuncs = &gsLocalPhysHeapFuncs;
#endif

	ui64CpuPhysicalBase = psSysData->sLocalMemCpuPBase.uiAddr;

	uiOrigHeapSize = psSysData->uiLocalMemSize;

	/* Set up the GPU physheap */
	psPhysHeap = &psDevConfig->pasPhysHeaps[ui32HeapNum++];

	/* Carveout out enough LMA memory to hold the heaps of all supported VMs */
	uiFwCarveoutSize = (RGX_NUM_DRIVERS_SUPPORTED * RGX_FIRMWARE_RAW_HEAP_SIZE);

#if defined(SUPPORT_AUTOVZ)
	/* Increase carveout size to be able to hold the FW page tables section at the end */
	uiFwCarveoutSize += RGX_FIRMWARE_MAX_PAGETABLE_SIZE;
#endif

	uiReserved = uiFwCarveoutSize;
#if defined(SUPPORT_DISPLAY_CLASS)
	uiReserved += RGX_TC_RESERVE_DC_MEM_SIZE;
#endif

	uiHeapSize = SysRestrictGpuLocalPhysheap(uiOrigHeapSize);

	if (uiOrigHeapSize != uiHeapSize)
	{
		IMG_UINT64 uiPrivHeapSize = uiOrigHeapSize - (uiHeapSize + uiReserved);

		InitLocalHeap(psPhysHeap,
					  ui64GpuPhysicalBase,
					  0,
					  uiPrivHeapSize,
					  &psSysData->sLocalMemCpuPBase.uiAddr,
					  psHeapFuncs,
					  PHYS_HEAP_USAGE_GPU_PRIVATE);

		psPhysHeap = &psDevConfig->pasPhysHeaps[ui32HeapNum++];
		ui64CpuPhysicalBase += uiPrivHeapSize;
		ui64GpuPhysicalBase += uiPrivHeapSize;
	}
	else
	{
		uiHeapSize -= uiReserved;
	}


	InitLocalHeap(psPhysHeap,
				  ui64GpuPhysicalBase,
				  ui64CpuPhysicalBase,
				  uiHeapSize,
				  &psSysData->sLocalMemCpuPBase.uiAddr,
				  psHeapFuncs,
				  PHYS_HEAP_USAGE_GPU_LOCAL);

	/* Set up the Firmware physheap */
	psPhysHeap = &psDevConfig->pasPhysHeaps[ui32HeapNum++];
	ui64CpuPhysicalBase += uiHeapSize;
	ui64GpuPhysicalBase += uiHeapSize;

	/* The Fw Carveout can hold the heaps of all supported VMs,
	 * but this Fw physheap is declared for one OS only.*/
	uiHeapSize = RGX_FIRMWARE_RAW_HEAP_SIZE;

	InitLocalHeap(psPhysHeap,
				  ui64GpuPhysicalBase,
				  ui64CpuPhysicalBase,
				  uiHeapSize,
				  &psSysData->sLocalMemCpuPBase.uiAddr,
				  psHeapFuncs,
				  PHYS_HEAP_USAGE_FW_SHARED);

#if defined(SUPPORT_DISPLAY_CLASS)
	/* Set up the DC heap */
	psPhysHeap = &psDevConfig->pasPhysHeaps[ui32HeapNum++];

	ui64CpuPhysicalBase += uiHeapSize;
	ui64GpuPhysicalBase += uiHeapSize;

	uiHeapSize = RGX_TC_RESERVE_DC_MEM_SIZE;

	InitLocalHeap(psPhysHeap,
				  ui64GpuPhysicalBase,
				  ui64CpuPhysicalBase,
				  uiHeapSize,
				  &psSysData->sLocalMemCpuPBase.uiAddr,
				  psHeapFuncs,
				  PHYS_HEAP_USAGE_DISPLAY);
#endif

	*pui32HeapNumOut = ui32HeapNum;

	return PVRSRV_OK;
}

static INLINE void DeInitLocalMemoryHeaps(PVRSRV_DEVICE_CONFIG *psDevConfig, SYS_DATA *psSysData)
{
	IMG_UINT32 ui32HeapNum = 0;

	ReleaseLocalMemory(psSysData, &psSysData->sLocalMemCpuPBase, psSysData->uiLocalMemSize);

	psDevConfig->pasPhysHeaps[ui32HeapNum].uConfig.sLMA.sStartAddr.uiAddr = 0;
	psDevConfig->pasPhysHeaps[ui32HeapNum].uConfig.sLMA.sCardBase.uiAddr = 0;
	psDevConfig->pasPhysHeaps[ui32HeapNum].uConfig.sLMA.uiSize = 0;
	ui32HeapNum++;

#if defined(SUPPORT_DISPLAY_CLASS)
	psDevConfig->pasPhysHeaps[ui32HeapNum].uConfig.sLMA.sStartAddr.uiAddr = 0;
	psDevConfig->pasPhysHeaps[ui32HeapNum].uConfig.sLMA.sCardBase.uiAddr = 0;
	psDevConfig->pasPhysHeaps[ui32HeapNum].uConfig.sLMA.uiSize = 0;
	ui32HeapNum++;
#endif

}
#endif /* (TC_MEMORY_CONFIG == TC_MEMORY_LOCAL) || (TC_MEMORY_CONFIG == TC_MEMORY_HYBRID) */

#if (TC_MEMORY_CONFIG == TC_MEMORY_LOCAL)
static PVRSRV_ERROR InitLocalMemory(PVRSRV_DEVICE_CONFIG *psDevConfig, SYS_DATA *psSysData)
{
	IMG_UINT32 ui32Value;
	IMG_UINT32 ui32HeapNum = 0;
	PVRSRV_ERROR eError;

	eError = InitLocalMemoryHeaps(psDevConfig, psSysData, &ui32HeapNum);
	if (eError != PVRSRV_OK)
	{
		return eError;
	}

	PVR_LOG_RETURN_IF_INVALID_PARAM((ui32HeapNum == psDevConfig->ui32PhysHeapCount), "psDevConfig->ui32PhysHeapCount");

	/* Configure Apollo for regression compatibility (i.e. local memory) mode */
	ui32Value = OSReadHWReg32(psSysData->pvSystemRegCpuVBase, TCF_CLK_CTRL_TEST_CTRL);
	ui32Value &= ~(ADDRESS_FORCE_MASK | PCI_TEST_MODE_MASK | HOST_ONLY_MODE_MASK | HOST_PHY_MODE_MASK);
	ui32Value |= (0x1 << ADDRESS_FORCE_SHIFT);
	OSWriteHWReg32(psSysData->pvSystemRegCpuVBase, TCF_CLK_CTRL_TEST_CTRL, ui32Value);

	/* Flush register write */
	(void)OSReadHWReg32(psSysData->pvSystemRegCpuVBase, TCF_CLK_CTRL_TEST_CTRL);
	OSWaitus(10);

	return PVRSRV_OK;
}

static void DeInitLocalMemory(PVRSRV_DEVICE_CONFIG *psDevConfig, SYS_DATA *psSysData)
{
	DeInitLocalMemoryHeaps(psDevConfig, psSysData);

	/* Set the register back to the default value */
	OSWriteHWReg32(psSysData->pvSystemRegCpuVBase, TCF_CLK_CTRL_TEST_CTRL, 0x1 << ADDRESS_FORCE_SHIFT);

	/* Flush register write */
	(void)OSReadHWReg32(psSysData->pvSystemRegCpuVBase, TCF_CLK_CTRL_TEST_CTRL);
	OSWaitus(10);
}
#elif (TC_MEMORY_CONFIG == TC_MEMORY_HOST)
static PVRSRV_ERROR InitHostMemory(PVRSRV_DEVICE_CONFIG *psDevConfig, SYS_DATA *psSysData)
{
	IMG_UINT32 ui32Value;
	PVR_UNREFERENCED_PARAMETER(psDevConfig);

	psDevConfig->pasPhysHeaps[0].eType = PHYS_HEAP_TYPE_UMA;
	psDevConfig->pasPhysHeaps[0].ui32UsageFlags = PHYS_HEAP_USAGE_GPU_LOCAL;
	psDevConfig->pasPhysHeaps[0].uConfig.sUMA.pszPDumpMemspaceName = "SYSMEM";
	psDevConfig->pasPhysHeaps[0].uConfig.sUMA.psMemFuncs = &gsSystemPhysHeapFuncs;
	psDevConfig->pasPhysHeaps[0].uConfig.sUMA.pszHeapName = "uma_gpu_local";
	psDevConfig->pasPhysHeaps[0].uConfig.sUMA.hPrivData = (IMG_HANDLE)psDevConfig;

	/* Configure Apollo for host only mode */
	ui32Value = OSReadHWReg32(psSysData->pvSystemRegCpuVBase, TCF_CLK_CTRL_TEST_CTRL);
	ui32Value &= ~(ADDRESS_FORCE_MASK | PCI_TEST_MODE_MASK | HOST_ONLY_MODE_MASK | HOST_PHY_MODE_MASK);
	ui32Value |= (0x1 << HOST_ONLY_MODE_SHIFT);
	OSWriteHWReg32(psSysData->pvSystemRegCpuVBase, TCF_CLK_CTRL_TEST_CTRL, ui32Value);

	/* Flush register write */
	(void)OSReadHWReg32(psSysData->pvSystemRegCpuVBase, TCF_CLK_CTRL_TEST_CTRL);
	OSWaitus(10);

	return PVRSRV_OK;
}

static void DeInitHostMemory(PVRSRV_DEVICE_CONFIG *psDevConfig, SYS_DATA *psSysData)
{
	PVR_UNREFERENCED_PARAMETER(psDevConfig);

	/* Set the register back to the default value */
	OSWriteHWReg32(psSysData->pvSystemRegCpuVBase, TCF_CLK_CTRL_TEST_CTRL, 0x1 << ADDRESS_FORCE_SHIFT);

	/* Flush register write */
	(void)OSReadHWReg32(psSysData->pvSystemRegCpuVBase, TCF_CLK_CTRL_TEST_CTRL);
	OSWaitus(10);
}
#elif (TC_MEMORY_CONFIG == TC_MEMORY_HYBRID)
static PVRSRV_ERROR InitHybridMemory(PVRSRV_DEVICE_CONFIG *psDevConfig, SYS_DATA *psSysData)
{
	IMG_UINT32 ui32Value;
	IMG_UINT32 ui32HeapNum = 0;
	PVRSRV_ERROR eError;

	eError = InitLocalMemoryHeaps(psDevConfig, psSysData, &ui32HeapNum);
	if (eError != PVRSRV_OK)
	{
		return eError;
	}

	psDevConfig->pasPhysHeaps[ui32HeapNum].eType = PHYS_HEAP_TYPE_UMA;
	psDevConfig->pasPhysHeaps[ui32HeapNum].ui32UsageFlags = PHYS_HEAP_USAGE_CPU_LOCAL;
	psDevConfig->pasPhysHeaps[ui32HeapNum].uConfig.sUMA.pszPDumpMemspaceName = "SYSMEM";
	psDevConfig->pasPhysHeaps[ui32HeapNum].uConfig.sUMA.psMemFuncs = &gsHybridPhysHeapFuncs;
	psDevConfig->pasPhysHeaps[ui32HeapNum].uConfig.sUMA.pszHeapName = "uma_cpu_local";
	psDevConfig->pasPhysHeaps[ui32HeapNum].uConfig.sUMA.hPrivData = (IMG_HANDLE)psDevConfig;

	PVR_LOG_RETURN_IF_INVALID_PARAM((++ui32HeapNum == psDevConfig->ui32PhysHeapCount), "psDevConfig->ui32PhysHeapCount");

	/* No additional setup needed for the Rogue UMA heap */

	/* Configure Apollo for host physical (i.e. hybrid) mode */
	ui32Value = OSReadHWReg32(psSysData->pvSystemRegCpuVBase, TCF_CLK_CTRL_TEST_CTRL);
	ui32Value &= ~(ADDRESS_FORCE_MASK | PCI_TEST_MODE_MASK | HOST_ONLY_MODE_MASK | HOST_PHY_MODE_MASK);
	ui32Value |= ((0x1 << HOST_ONLY_MODE_SHIFT) | (0x1 << HOST_PHY_MODE_SHIFT));
	OSWriteHWReg32(psSysData->pvSystemRegCpuVBase, TCF_CLK_CTRL_TEST_CTRL, ui32Value);

	/* Flush register write */
	(void)OSReadHWReg32(psSysData->pvSystemRegCpuVBase, TCF_CLK_CTRL_TEST_CTRL);
	OSWaitus(10);

	/* Setup the start address of the 1GB window used by the _GPU_ to access local memory.
	 * The value used here is the start address of the 1GB window used by the _CPU_
	 * to access the local memory on the TC. This means that both CPU and GPU will use
	 * the same range of physical addresses to access local memory, and any access
	 * out of that range will be directed to system memory instead.
	 * In the hybrid config case the conversion cpuPaddr <-> devPaddr is an assignment.
	 */
	OSWriteHWReg32(psSysData->pvSystemRegCpuVBase,
					TCF_CLK_CTRL_HOST_PHY_OFFSET,
					psSysData->sLocalMemCpuPBase.uiAddr);
	OSWaitus(10);

	/* Flush register write */
	(void)OSReadHWReg32(psSysData->pvSystemRegCpuVBase, TCF_CLK_CTRL_HOST_PHY_OFFSET);
	OSWaitus(10);

	return PVRSRV_OK;
}

static void DeInitHybridMemory(PVRSRV_DEVICE_CONFIG *psDevConfig, SYS_DATA *psSysData)
{
	DeInitLocalMemoryHeaps(psDevConfig, psSysData);

	/* Set the register back to the default value */
	OSWriteHWReg32(psSysData->pvSystemRegCpuVBase, TCF_CLK_CTRL_TEST_CTRL, 0x1 << ADDRESS_FORCE_SHIFT);

	/* Flush register write */
	(void)OSReadHWReg32(psSysData->pvSystemRegCpuVBase, TCF_CLK_CTRL_TEST_CTRL);
	OSWaitus(10);
}
#endif

static PVRSRV_ERROR InitMemory(PVRSRV_DEVICE_CONFIG *psDevConfig, SYS_DATA *psSysData)
{
	IMG_UINT32 uiHeapCount = 1;			/* GPU heap present by default */
	PHYS_HEAP_CONFIG *pasPhysHeaps;
	PVRSRV_ERROR eError = PVRSRV_OK;

#if (TC_MEMORY_CONFIG == TC_MEMORY_HYBRID)
	/* additional UMA heap required on hybrid systems */
	uiHeapCount++;
#endif

#if (TC_MEMORY_CONFIG != TC_MEMORY_HOST)
	/* LMA systems use a dedicated Fw heap alongside the GPU heap */
	uiHeapCount++;
#endif

#if defined(SUPPORT_DISPLAY_CLASS) && (TC_MEMORY_CONFIG != TC_MEMORY_HOST)
	/* DC heap */
	uiHeapCount++;
#endif

	/* Check if we are restricting the GPU_LOCAL heap, if so add
	 * another heap for GPU_PRIVATE
	 */
	if (SysRestrictGpuLocalAddPrivateHeap())
	{
		uiHeapCount++;
		psDevConfig->bHasNonMappableLocalMemory = IMG_TRUE;
	}
	else
	{
		psDevConfig->bHasNonMappableLocalMemory = IMG_FALSE;
	}

	pasPhysHeaps = OSAllocZMem(sizeof(*pasPhysHeaps) * uiHeapCount);
	if (!pasPhysHeaps)
	{
		return PVRSRV_ERROR_OUT_OF_MEMORY;
	}

	psDevConfig->pasPhysHeaps = pasPhysHeaps;
	psDevConfig->ui32PhysHeapCount = uiHeapCount;

#if (TC_MEMORY_CONFIG == TC_MEMORY_LOCAL)
	eError = InitLocalMemory(psDevConfig, psSysData);
#elif (TC_MEMORY_CONFIG == TC_MEMORY_HOST)
	eError = InitHostMemory(psDevConfig, psSysData);
#elif (TC_MEMORY_CONFIG == TC_MEMORY_HYBRID)
	eError = InitHybridMemory(psDevConfig, psSysData);
#endif
	if (eError != PVRSRV_OK)
	{
		OSFreeMem(pasPhysHeaps);
	}

	return eError;
}

static INLINE void DeInitMemory(PVRSRV_DEVICE_CONFIG *psDevConfig, SYS_DATA *psSysData)
{
#if (TC_MEMORY_CONFIG == TC_MEMORY_LOCAL)
	DeInitLocalMemory(psDevConfig, psSysData);
#elif (TC_MEMORY_CONFIG == TC_MEMORY_HOST)
	DeInitHostMemory(psDevConfig, psSysData);
#elif (TC_MEMORY_CONFIG == TC_MEMORY_HYBRID)
	DeInitHybridMemory(psDevConfig, psSysData);
#endif

	OSFreeMem(psDevConfig->pasPhysHeaps);
}

static void EnableInterrupt(SYS_DATA *psSysData, IMG_UINT32 ui32InterruptFlag)
{
	IMG_UINT32 ui32Value;

	/* Set sense to active high */
	ui32Value = OSReadHWReg32(psSysData->pvSystemRegCpuVBase, TCF_CLK_CTRL_INTERRUPT_OP_CFG);
	ui32Value &= ~(INT_SENSE_MASK);
	OSWriteHWReg32(psSysData->pvSystemRegCpuVBase, TCF_CLK_CTRL_INTERRUPT_OP_CFG, ui32Value);
	OSWaitus(1000);

	/* Flush register write */
	(void)OSReadHWReg32(psSysData->pvSystemRegCpuVBase, TCF_CLK_CTRL_INTERRUPT_OP_CFG);

	/* Enable Rogue and PDP1 interrupts */
	ui32Value = OSReadHWReg32(psSysData->pvSystemRegCpuVBase, TCF_CLK_CTRL_INTERRUPT_ENABLE);
	ui32Value |= ui32InterruptFlag;
	OSWriteHWReg32(psSysData->pvSystemRegCpuVBase, TCF_CLK_CTRL_INTERRUPT_ENABLE, ui32Value);

	/* Flush register write */
	(void)OSReadHWReg32(psSysData->pvSystemRegCpuVBase, TCF_CLK_CTRL_INTERRUPT_ENABLE);
	OSWaitus(10);
}

static void DisableInterrupt(SYS_DATA *psSysData, IMG_UINT32 ui32InterruptFlag)
{
	IMG_UINT32 ui32Value;

	ui32Value = OSReadHWReg32(psSysData->pvSystemRegCpuVBase, TCF_CLK_CTRL_INTERRUPT_ENABLE);
	ui32Value &= ~(ui32InterruptFlag);
	OSWriteHWReg32(psSysData->pvSystemRegCpuVBase, TCF_CLK_CTRL_INTERRUPT_ENABLE, ui32Value);

	/* Flush register write */
	(void)OSReadHWReg32(psSysData->pvSystemRegCpuVBase, TCF_CLK_CTRL_INTERRUPT_ENABLE);
	OSWaitus(10);
}

static void DeviceConfigDestroy(PVRSRV_DEVICE_CONFIG *psDevConfig)
{
	RGX_DATA *psRGXData = (RGX_DATA *)psDevConfig->hDevData;

	OSFreeMem(psRGXData->psRGXTimingInfo);
	OSFreeMem(psRGXData);
	OSFreeMem(psDevConfig);
}

static void PCIDeInitDev(PVRSRV_DEVICE_CONFIG *psDevice)
{
	SYS_DATA *psSysData = (SYS_DATA *)psDevice->hSysData;

	psDevice->hSysData = NULL;
	psDevice->pszVersion = NULL;

	OSUninstallSystemLISR(psSysData->hLISR);

	if (psSysData->pszVersion)
	{
		OSFreeMem(psSysData->pszVersion);
		psSysData->pszVersion = NULL;
	}

	DeInitMemory(psDevice, psSysData);

	OSUnMapPhysToLin(psSysData->pvSystemRegCpuVBase, psSysData->uiSystemRegSize);
	OSPCIReleaseAddrRange(psSysData->hRGXPCI, SYS_RGX_REG_PCI_BASENUM);
	OSPCIReleaseAddrRegion(psSysData->hRGXPCI, SYS_APOLLO_REG_PCI_BASENUM, SYS_APOLLO_REG_SYS_OFFSET, SYS_APOLLO_REG_SYS_SIZE);

	OSPCIReleaseDev(psSysData->hRGXPCI);

	if (psSysData->pszVersion)
	{
		OSFreeMem(psSysData->pszVersion);
	}

	OSFreeMem(psSysData);

	psDevice->pvOSDevice = NULL;
}

#if (TC_MEMORY_CONFIG == TC_MEMORY_LOCAL)
static void TCLocalCpuPAddrToDevPAddr(IMG_HANDLE hPrivData,
					  IMG_UINT32 ui32NumOfAddr,
					  IMG_DEV_PHYADDR *psDevPAddr,
					  IMG_CPU_PHYADDR *psCpuPAddr)
{
	IMG_UINT64 uiAddr = *((IMG_UINT64*)hPrivData);

	/* Optimise common case */
	psDevPAddr[0].uiAddr = psCpuPAddr[0].uiAddr - uiAddr;
	if (ui32NumOfAddr > 1)
	{
		IMG_UINT32 ui32Idx;
		for (ui32Idx = 1; ui32Idx < ui32NumOfAddr; ++ui32Idx)
		{
			psDevPAddr[ui32Idx].uiAddr = psCpuPAddr[ui32Idx].uiAddr - uiAddr;
		}
	}
}

static void TCLocalDevPAddrToCpuPAddr(IMG_HANDLE hPrivData,
					  IMG_UINT32 ui32NumOfAddr,
					  IMG_CPU_PHYADDR *psCpuPAddr,
					  IMG_DEV_PHYADDR *psDevPAddr)
{
	IMG_UINT64 uiAddr = *((IMG_UINT64*)hPrivData);

	/* Optimise common case */
	psCpuPAddr[0].uiAddr = psDevPAddr[0].uiAddr + uiAddr;
	if (ui32NumOfAddr > 1)
	{
		IMG_UINT32 ui32Idx;
		for (ui32Idx = 1; ui32Idx < ui32NumOfAddr; ++ui32Idx)
		{
			psCpuPAddr[ui32Idx].uiAddr = psDevPAddr[ui32Idx].uiAddr + uiAddr;
		}
	}
}

#elif (TC_MEMORY_CONFIG == TC_MEMORY_HOST)
static void TCSystemCpuPAddrToDevPAddr(IMG_HANDLE hPrivData,
					   IMG_UINT32 ui32NumOfAddr,
					   IMG_DEV_PHYADDR *psDevPAddr,
					   IMG_CPU_PHYADDR *psCpuPAddr)
{
	PVR_UNREFERENCED_PARAMETER(hPrivData);

	/* Optimise common case */
	psDevPAddr[0].uiAddr = psCpuPAddr[0].uiAddr;
	if (ui32NumOfAddr > 1)
	{
		IMG_UINT32 ui32Idx;
		for (ui32Idx = 1; ui32Idx < ui32NumOfAddr; ++ui32Idx)
		{
			psDevPAddr[ui32Idx].uiAddr = psCpuPAddr[ui32Idx].uiAddr;
		}
	}
}

static void TCSystemDevPAddrToCpuPAddr(IMG_HANDLE hPrivData,
					   IMG_UINT32 ui32NumOfAddr,
					   IMG_CPU_PHYADDR *psCpuPAddr,
					   IMG_DEV_PHYADDR *psDevPAddr)
{
	PVR_UNREFERENCED_PARAMETER(hPrivData);

	/* Optimise common case */
	psCpuPAddr[0].uiAddr = IMG_CAST_TO_CPUPHYADDR_UINT(psDevPAddr[0].uiAddr);
	if (ui32NumOfAddr > 1)
	{
		IMG_UINT32 ui32Idx;
		for (ui32Idx = 1; ui32Idx < ui32NumOfAddr; ++ui32Idx)
		{
			psCpuPAddr[ui32Idx].uiAddr = IMG_CAST_TO_CPUPHYADDR_UINT(psDevPAddr[ui32Idx].uiAddr);
		}
	}
}

#elif (TC_MEMORY_CONFIG == TC_MEMORY_HYBRID)
static void TCHybridCpuPAddrToDevPAddr(IMG_HANDLE hPrivData,
					  IMG_UINT32 ui32NumOfAddr,
					  IMG_DEV_PHYADDR *psDevPAddr,
					  IMG_CPU_PHYADDR *psCpuPAddr)
{
	PVR_UNREFERENCED_PARAMETER(hPrivData);

	/* The CPU and GPU physical addresses are always the same in Hybrid model
	 * However on 64 bit platforms, RAM mapped above 4G of processor physical address space
	 * cannot be accessed by this version of the test chip due to its 32bit interface.
	 *
	 * In fact its possible to further optimise this function by eliminating this
	 * unnecessary copy
	*/
#if defined(CONFIG_64BIT)
	PVR_ASSERT(sizeof(IMG_CPU_PHYADDR) == sizeof(IMG_DEV_PHYADDR));
	OSCachedMemCopy(psDevPAddr, psCpuPAddr, (ui32NumOfAddr * sizeof(*psCpuPAddr)));
#else
	{
		IMG_UINT32 ui32Idx;
		for (ui32Idx = 0; ui32Idx < ui32NumOfAddr; ++ui32Idx)
		{
			psDevPAddr[ui32Idx].uiAddr = psCpuPAddr[ui32Idx].uiAddr;
		}
	}
#endif
}

static void TCHybridDevPAddrToCpuPAddr(IMG_HANDLE hPrivData,
					  IMG_UINT32 ui32NumOfAddr,
					  IMG_CPU_PHYADDR *psCpuPAddr,
					  IMG_DEV_PHYADDR *psDevPAddr)
{
	PVR_UNREFERENCED_PARAMETER(hPrivData);

	/* The CPU and GPU physical addresses are always the same in Hybrid model
	 * However on 64 bit platforms, RAM mapped above 4G of processor physical address space
	 * cannot be accessed by this version of the test chip due to its 32bit interface.
	 *
	 * In fact its possible to further optimise this function by eliminating this
	 * unnecessary copy
	*/

#if defined(CONFIG_64BIT)
	PVR_ASSERT(sizeof(IMG_CPU_PHYADDR) == sizeof(IMG_DEV_PHYADDR));
	OSCachedMemCopy(psCpuPAddr, psDevPAddr, (ui32NumOfAddr * sizeof(*psCpuPAddr)));
#else
	{
		IMG_UINT32 ui32Idx;
		for (ui32Idx = 0; ui32Idx < ui32NumOfAddr; ++ui32Idx)
		{
			psCpuPAddr[ui32Idx].uiAddr = psDevPAddr[ui32Idx].uiAddr;
		}
	}
#endif

}

#endif /* (TC_MEMORY_CONFIG == TC_MEMORY_HYBRID) */

#if defined(SUPPORT_LMA_SUSPEND_TO_RAM) && defined(__x86_64__)
// #define _DBG(...) PVR_LOG((__VA_ARGS__))
#define _DBG(...)

static PVRSRV_ERROR PrePower(IMG_HANDLE hSysData,
                             PVRSRV_SYS_POWER_STATE eNewPowerState,
                             PVRSRV_SYS_POWER_STATE eCurrentPowerState,
                             PVRSRV_POWER_FLAGS ePwrFlags)
{
	SYS_DATA *psSysData = (SYS_DATA *) hSysData;
	IMG_DEV_PHYADDR sDevPAddr = {0};
	IMG_UINT64 uiHeapTotalSize, uiHeapUsedSize, uiHeapFreeSize;
	IMG_UINT64 uiSize = 0, uiOffset = 0;
	IMG_UINT32 i;
	PVRSRV_ERROR eError;

	_DBG("(%s()) state: current=%s, new=%s; flags: 0x%08x", __func__,
	     PVRSRVSysPowerStateToString(eCurrentPowerState),
	     PVRSRVSysPowerStateToString(eNewPowerState), ePwrFlags);

	/* The transition might be both from ON or OFF states to OFF state so check
	 * only for the *new* state. Also this is only valid for suspend requests. */
	if (eNewPowerState != PVRSRV_SYS_POWER_STATE_OFF ||
	    !BITMASK_HAS(ePwrFlags, PVRSRV_POWER_FLAGS_OSPM_SUSPEND_REQ))
	{
		return PVRSRV_OK;
	}

	/* disable interrupts */
	for (i = 0; i < ARRAY_SIZE(psSysData->sInterruptData); i++)
	{
		IMG_UINT32 uiIntFlag = psSysData->sInterruptData[i].ui32InterruptFlag;
		if (uiIntFlag != 0)
		{
			DisableInterrupt(psSysData, uiIntFlag);
		}
	}

	/* dump video RAM content to system RAM */

	eError = LMA_HeapIteratorCreate(psSysData->psDevConfig->psDevNode,
	                                PVRSRV_PHYS_HEAP_GPU_LOCAL,
	                                &psSysData->psHeapIter);
	PVR_LOG_GOTO_IF_ERROR(eError, "LMA_HeapIteratorCreate", enable_interrupts);

	eError = LMA_HeapIteratorGetHeapStats(psSysData->psHeapIter, &uiHeapTotalSize,
	                                      &uiHeapUsedSize);
	PVR_LOG_GOTO_IF_ERROR(eError, "LMA_HeapIteratorGetHeapStats",
	                      enable_interrupts);
	uiHeapFreeSize = uiHeapTotalSize - uiHeapUsedSize;

	_DBG("(%s()) heap stats: total=0x%" IMG_UINT64_FMTSPECx ", "
	     "used=0x%" IMG_UINT64_FMTSPECx ", free=0x%" IMG_UINT64_FMTSPECx,
	     __func__, uiHeapTotalSize, uiHeapUsedSize, uiHeapFreeSize);

	psSysData->pvSuspendBuffer = OSAllocMem(uiHeapUsedSize);
	PVR_LOG_GOTO_IF_NOMEM(psSysData->pvSuspendBuffer, eError, destroy_iterator);

	_DBG("(%s()) buffer=%px", __func__, psSysData->pvSuspendBuffer);

	while (LMA_HeapIteratorNext(psSysData->psHeapIter, &sDevPAddr, &uiSize))
	{
		void *pvCpuVAddr;
		IMG_CPU_PHYADDR sCpuPAddr = {0};

		if (uiOffset + uiSize > uiHeapUsedSize)
		{
			PVR_DPF((PVR_DBG_ERROR, "uiOffset = %" IMG_UINT64_FMTSPECx ", "
			        "uiSize = %" IMG_UINT64_FMTSPECx, uiOffset, uiSize));

			PVR_LOG_GOTO_WITH_ERROR("LMA_HeapIteratorNext", eError,
			                        PVRSRV_ERROR_INVALID_OFFSET,
			                        free_buffer);
		}

		TCHybridDevPAddrToCpuPAddr(psSysData->psDevConfig, 1, &sCpuPAddr,
		                           &sDevPAddr);

		pvCpuVAddr = OSMapPhysToLin(sCpuPAddr, uiSize,
		                            PVRSRV_MEMALLOCFLAG_CPU_UNCACHED_WC);

		_DBG("(%s()) iterator: dev_paddr=%px, cpu_paddr=%px, cpu_vaddr=%px, "
		     "size=0x%05" IMG_UINT64_FMTSPECx, __func__,
		     (void *) sDevPAddr.uiAddr, (void *) sCpuPAddr.uiAddr,
		     pvCpuVAddr, uiSize);

		/* copy memory */
		memcpy((IMG_BYTE *) psSysData->pvSuspendBuffer + uiOffset, pvCpuVAddr,
		       uiSize);
#ifdef DEBUG
		/* and now poison it - for debugging purposes so we know that the
		 * resume procedure works */
		memset(pvCpuVAddr, 0x9b, uiSize);
#endif

		uiOffset += uiSize;

		OSUnMapPhysToLin(pvCpuVAddr, uiSize);
	}

	/* suspend the PCI device */
	OSPCISuspendDev(psSysData->hRGXPCI);

	return PVRSRV_OK;

free_buffer:
	OSFreeMem(psSysData->pvSuspendBuffer);
	psSysData->pvSuspendBuffer = NULL;
destroy_iterator:
	LMA_HeapIteratorDestroy(psSysData->psHeapIter);
	psSysData->psHeapIter = NULL;
enable_interrupts:
	for (i = 0; i < ARRAY_SIZE(psSysData->sInterruptData); i++)
	{
		IMG_UINT32 uiIntFlag = psSysData->sInterruptData[i].ui32InterruptFlag;
		if (uiIntFlag != 0)
		{
			DisableInterrupt(psSysData, uiIntFlag);
		}
	}

	return eError;
}

static PVRSRV_ERROR PostPower(IMG_HANDLE hSysData,
                              PVRSRV_SYS_POWER_STATE eNewPowerState,
                              PVRSRV_SYS_POWER_STATE eCurrentPowerState,
                              PVRSRV_POWER_FLAGS ePwrFlags)
{
	SYS_DATA *psSysData = (SYS_DATA *) hSysData;
	IMG_DEV_PHYADDR sDevPAddr = {0};
	IMG_UINT64 uiSize = 0, uiOffset = 0;
	IMG_UINT32 i;
	PVRSRV_ERROR eError;

	_DBG("(%s()) state: current=%s, new=%s; flags=0x%08x; buffer=%px", __func__,
	     PVRSRVSysPowerStateToString(eCurrentPowerState),
	     PVRSRVSysPowerStateToString(eNewPowerState), ePwrFlags,
	     psSysData->pvSuspendBuffer);

	/* restore the video RAM */

	/* The transition might be both to ON or OFF states from OFF state so check
	 * only for the *current* state. Also this is only valid for resume requests. */
	if ((eCurrentPowerState != eNewPowerState && eCurrentPowerState != PVRSRV_SYS_POWER_STATE_OFF) ||
	    !BITMASK_HAS(ePwrFlags, PVRSRV_POWER_FLAGS_OSPM_RESUME_REQ))
	{
		return PVRSRV_OK;
	}

	/* resume the PCI device */
	OSPCIResumeDev(psSysData->hRGXPCI);

	if (psSysData->pvSuspendBuffer != NULL)
	{
		eError = LMA_HeapIteratorReset(psSysData->psHeapIter);
		PVR_LOG_GOTO_IF_ERROR(eError, "LMA_HeapIteratorReset", free_buffer);

		while (LMA_HeapIteratorNext(psSysData->psHeapIter, &sDevPAddr, &uiSize))
		{
			void *pvCpuVAddr;
			IMG_CPU_PHYADDR sCpuPAddr = {0};

			TCHybridDevPAddrToCpuPAddr(psSysData->psDevConfig, 1, &sCpuPAddr,
			                           &sDevPAddr);

			pvCpuVAddr = OSMapPhysToLin(sCpuPAddr, uiSize,
			                            PVRSRV_MEMALLOCFLAG_CPU_UNCACHED_WC);

			_DBG("(%s()) iterator: dev_paddr=%px, cpu_paddr=%px, cpu_vaddr=%px, "
			     "size=0x%05" IMG_UINT64_FMTSPECx, __func__,
			     (void *) sDevPAddr.uiAddr, (void *) sCpuPAddr.uiAddr,
			     pvCpuVAddr, uiSize);

			/* copy memory */
			memcpy(pvCpuVAddr, (IMG_BYTE *) psSysData->pvSuspendBuffer + uiOffset,
			       uiSize);

			uiOffset += uiSize;

			OSUnMapPhysToLin(pvCpuVAddr, uiSize);
		}

		OSFreeMem(psSysData->pvSuspendBuffer);
		psSysData->pvSuspendBuffer = NULL;
	}

	LMA_HeapIteratorDestroy(psSysData->psHeapIter);
	psSysData->psHeapIter = NULL;

	/* reset the TestChip */

	eError = HardResetDevice(psSysData, psSysData->psDevConfig);
	PVR_GOTO_IF_ERROR(eError, free_buffer);

	/* enable interrupts */
	for (i = 0; i < ARRAY_SIZE(psSysData->sInterruptData); i++)
	{
		IMG_UINT32 uiIntFlag = psSysData->sInterruptData[i].ui32InterruptFlag;
		if (uiIntFlag != 0)
		{
			EnableInterrupt(psSysData, uiIntFlag);
		}
	}

	/* reset DC devices */
	{
		PVRSRV_DEVICE_NODE *psDevNode = psSysData->psDevConfig->psDevNode;
		IMG_UINT32 uiDeviceCount = 0, auiDeviceIndex[PVRSRV_MAX_DEVICES];

		eError = DCDevicesEnumerate(NULL, psDevNode, ARRAY_SIZE(auiDeviceIndex),
		                            &uiDeviceCount, auiDeviceIndex);
		PVR_LOG_IF_ERROR(eError, "DCDevicesEnumerate");

		for (i = 0; i < uiDeviceCount; i++)
		{
			DC_DEVICE *psDCDevice;
			eError = DCDeviceAcquire(NULL, psDevNode, auiDeviceIndex[i],
			                         &psDCDevice);
			if (eError != PVRSRV_OK)
			{
				PVR_LOG_ERROR(eError, "DCDeviceAcquire");
				continue;
			}

			eError = DCResetDevice(psDCDevice);
			PVR_LOG_IF_ERROR(eError, "DCResetDevice");

			eError = DCDeviceRelease(psDCDevice);
			PVR_LOG_IF_ERROR(eError, "DCDeviceRelease");
		}
	}

	return PVRSRV_OK;

free_buffer:
	OSFreeMem(psSysData->pvSuspendBuffer);
	psSysData->pvSuspendBuffer = NULL;

	return eError;
}
#endif /* defined(SUPPORT_LMA_SUSPEND_TO_RAM) && defined(__x86_64__) */

static PVRSRV_ERROR DeviceConfigCreate(void *pvOSDevice,
									   PVRSRV_DEVICE_CONFIG **ppsDevConfigOut)
{
	PVRSRV_DEVICE_CONFIG *psDevConfig;
	RGX_DATA *psRGXData;
	RGX_TIMING_INFORMATION *psRGXTimingInfo;
#if defined(SUPPORT_LINUX_DVFS) || defined(SUPPORT_PDVFS)
	int i;
	IMG_DVFS_DEVICE_CFG *psDVFSDeviceCfg;
#endif
#if defined(SUPPORT_LINUX_DVFS)
	IMG_DVFS_GOVERNOR_CFG *psDVFSGovernorCfg;
#endif

	psDevConfig = OSAllocZMem(sizeof(*psDevConfig));
	if (!psDevConfig)
	{
		return PVRSRV_ERROR_OUT_OF_MEMORY;
	}

	psRGXData = OSAllocZMem(sizeof(*psRGXData));
	if (!psRGXData)
	{
		OSFreeMem(psDevConfig);
		return PVRSRV_ERROR_OUT_OF_MEMORY;
	}

	psRGXTimingInfo = OSAllocZMem(sizeof(*psRGXTimingInfo));
	if (!psRGXTimingInfo)
	{
		OSFreeMem(psRGXData);
		OSFreeMem(psDevConfig);
		return PVRSRV_ERROR_OUT_OF_MEMORY;
	}

	/* Set up the RGX timing information */
	psRGXTimingInfo->ui32CoreClockSpeed = ui32CoreClockSpeed;

#if defined(SUPPORT_AUTOVZ)
	/* The AutoVz driver enables a virtualisation watchdog not compatible with APM */
	psRGXTimingInfo->bEnableActivePM = IMG_FALSE;
#else
	psRGXTimingInfo->bEnableActivePM = IMG_TRUE;
#endif
	psRGXTimingInfo->bEnableRDPowIsland = IMG_FALSE;
	psRGXTimingInfo->ui32ActivePMLatencyms = SYS_RGX_ACTIVE_POWER_LATENCY_MS;

	/* Set up the RGX data */
	psRGXData->psRGXTimingInfo = psRGXTimingInfo;

	psDevConfig->pvOSDevice = pvOSDevice;
	psDevConfig->pszName = TC_SYSTEM_NAME;

	psDevConfig->eDefaultHeap = PVRSRV_PHYS_HEAP_GPU_LOCAL;

	psDevConfig->eCacheSnoopingMode = PVRSRV_DEVICE_SNOOP_NONE;
	psDevConfig->bHasFBCDCVersion31 = IMG_FALSE;

	/* Only required for LMA but having this always set shouldn't be a problem */
	psDevConfig->bDevicePA0IsValid	= IMG_TRUE,

	psDevConfig->hDevData = psRGXData;
	psDevConfig->hSysData = NULL;

	psDevConfig->pfnSysDevFeatureDepInit = NULL;

	/* device error notify callback function */
	psDevConfig->pfnSysDevErrorNotify = NULL;

#if defined(SUPPORT_LINUX_DVFS) || defined(SUPPORT_PDVFS)
	psDVFSDeviceCfg = &psDevConfig->sDVFS.sDVFSDeviceCfg;

	PVR_DPF((PVR_DBG_MESSAGE,"--- OPP Table ---"));
	for (i = (LEVEL_COUNT - 1); i>=0; i--)
	{
		PVR_DPF((PVR_DBG_MESSAGE,
		         "%u/%u",
		         g_asOPPTable[i].ui32Volt,
		         g_asOPPTable[i].ui32Freq));
	}

	/* Fake DVFS configuration used purely for testing purposes */
	psDVFSDeviceCfg->pasOPPTable = g_asOPPTable;
	psDVFSDeviceCfg->ui32OPPTableSize = LEVEL_COUNT;
	psDVFSDeviceCfg->pfnSetFrequency = SetFrequency;
	psDVFSDeviceCfg->pfnSetVoltage = SetVoltage;
#endif
#if defined(SUPPORT_LINUX_DVFS)
	psDVFSDeviceCfg->ui32PollMs = 1000;
	psDVFSDeviceCfg->bIdleReq = IMG_TRUE;

	psDVFSGovernorCfg = &psDevConfig->sDVFS.sDVFSGovernorCfg;
	psDVFSGovernorCfg->ui32UpThreshold = 90;
	psDVFSGovernorCfg->ui32DownDifferential = 10;
#endif

	*ppsDevConfigOut = psDevConfig;

	return PVRSRV_OK;
}

PVRSRV_ERROR SysDevInit(void *pvOSDevice, PVRSRV_DEVICE_CONFIG **ppsDevConfig)
{
	PVRSRV_DEVICE_CONFIG *psDevConfig;
	PVRSRV_ERROR eError;

	eError = DeviceConfigCreate(pvOSDevice, &psDevConfig);
	if (eError != PVRSRV_OK)
	{
		return eError;
	}

	eError = PCIInitDev(psDevConfig, pvOSDevice);
	if (eError != PVRSRV_OK)
	{
		DeviceConfigDestroy(psDevConfig);

		return eError;
	}

	/* Set psDevConfig->pfnSysDevErrorNotify callback */
	psDevConfig->pfnSysDevErrorNotify = SysRGXErrorNotify;

#if defined(SUPPORT_LMA_SUSPEND_TO_RAM) && defined(__x86_64__)
	/* power functions */
	psDevConfig->pfnPrePowerState = PrePower;
	psDevConfig->pfnPostPowerState = PostPower;
#endif /* defined(SUPPORT_LMA_SUSPEND_TO_RAM) && defined(__x86_64__) */

	*ppsDevConfig = psDevConfig;

	return PVRSRV_OK;
}

void SysDevDeInit(PVRSRV_DEVICE_CONFIG *psDevConfig)
{
	PCIDeInitDev(psDevConfig);

	DeviceConfigDestroy(psDevConfig);
}

PVRSRV_ERROR SysInstallDeviceLISR(IMG_HANDLE hSysData,
				  IMG_UINT32 ui32IRQ,
				  const IMG_CHAR *pszName,
				  PFN_LISR pfnLISR,
				  void *pvData,
				  IMG_HANDLE *phLISRData)
{
	SYS_DATA *psSysData = (SYS_DATA *)hSysData;
	PVRSRV_ERROR eError = PVRSRV_OK;
	IMG_UINT32 ui32InterruptFlag;

	switch (ui32IRQ)
	{
		case APOLLO_IRQ_GPU:
			ui32InterruptFlag = (0x1 << EXT_INT_SHIFT);
			break;
		case APOLLO_IRQ_PDP:
			ui32InterruptFlag = (0x1 << PDP1_INT_SHIFT);
			break;
		default:
			PVR_DPF((PVR_DBG_ERROR, "%s: No device matching IRQ %d",
					 __func__, ui32IRQ));
			return PVRSRV_ERROR_UNABLE_TO_INSTALL_ISR;
	}

	if (psSysData->sInterruptData[ui32IRQ].pfnLISR)
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: ISR for %s already installed!",
				 __func__, pszName));
		return PVRSRV_ERROR_ISR_ALREADY_INSTALLED;
	}
	else if (ui32IRQ > ARRAY_SIZE(psSysData->sInterruptData))
	{
		PVR_DPF((PVR_DBG_ERROR,
				 "%s: Invalid IRQ %d, exceeds max. IRQ " IMG_SIZE_FMTSPEC "!",
				 __func__, ui32IRQ, ARRAY_SIZE(psSysData->sInterruptData)));
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	psSysData->sInterruptData[ui32IRQ].psSysData	= psSysData;
	psSysData->sInterruptData[ui32IRQ].pszName		= pszName;
	psSysData->sInterruptData[ui32IRQ].pfnLISR		= pfnLISR;
	psSysData->sInterruptData[ui32IRQ].pvData		= pvData;
	psSysData->sInterruptData[ui32IRQ].ui32InterruptFlag = ui32InterruptFlag;

	*phLISRData = &psSysData->sInterruptData[ui32IRQ];

	EnableInterrupt(psSysData, psSysData->sInterruptData[ui32IRQ].ui32InterruptFlag);

	PVR_LOG(("Installed device LISR %s on IRQ %d", pszName, psSysData->ui32IRQ));

	return eError;
}

PVRSRV_ERROR SysUninstallDeviceLISR(IMG_HANDLE hLISRData)
{
	SYS_INTERRUPT_DATA *psInterruptData = (SYS_INTERRUPT_DATA *)hLISRData;
	SYS_DATA *psSysData = psInterruptData->psSysData;
	PVRSRV_ERROR eError = PVRSRV_OK;

	PVR_ASSERT(psInterruptData);

	PVR_LOG(("Uninstalling device LISR %s on IRQ %d", psInterruptData->pszName, psSysData->ui32IRQ));

	/* Disable interrupts for this device */
	DisableInterrupt(psInterruptData->psSysData, psInterruptData->ui32InterruptFlag);

	/* Reset the interrupt data */
	psInterruptData->pszName = NULL;
	psInterruptData->psSysData = NULL;
	psInterruptData->pfnLISR = NULL;
	psInterruptData->pvData = NULL;
	psInterruptData->ui32InterruptFlag = 0;

	return eError;
}
