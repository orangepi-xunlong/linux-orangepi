/*************************************************************************/ /*!
@File
@Title          RGX firmware RISC-V utility routines
@Copyright      Copyright (c) Imagination Technologies Ltd. All Rights Reserved
@Description    RGX firmware RISC-V utility routines
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

#include "rgxfwutils.h"
#include "rgxfwriscv.h"

#if defined(RGX_FEATURE_HOST_SECURITY_VERSION_MAX_VALUE_IDX)
#define RGX_GET_DMI_REG(psDevInfo, value) \
	((RGX_GET_FEATURE_VALUE(psDevInfo, HOST_SECURITY_VERSION) >= 4) ? \
	 RGX_CR_FWCORE_DMI_##value##__HOST_SECURITY_GEQ4 : RGX_CR_FWCORE_DMI_##value)
#else
#define RGX_GET_DMI_REG(psDevInfo, value) RGX_CR_FWCORE_DMI_##value
#endif

/*
 * RGXRiscvHalt
 */
PVRSRV_ERROR RGXRiscvHalt(PVRSRV_RGXDEV_INFO *psDevInfo)
{
	__maybe_unused IMG_UINT32 ui32_DMI_DMCONTROL_Reg = RGX_GET_DMI_REG(psDevInfo, DMCONTROL);
	__maybe_unused IMG_UINT32 ui32_DMI_DMSTATUS_Reg = RGX_GET_DMI_REG(psDevInfo, DMSTATUS);

#if defined(NO_HARDWARE) && defined(PDUMP)
	PDUMPCOMMENTWITHFLAGS(psDevInfo->psDeviceNode,
	                      PDUMP_FLAGS_CONTINUOUS, "Halt RISC-V FW");

	/* Send halt request (no need to select one or more harts on this RISC-V core) */
	PDUMPREG32(psDevInfo->psDeviceNode,
	           RGX_PDUMPREG_NAME, ui32_DMI_DMCONTROL_Reg,
	           RGX_CR_FWCORE_DMI_DMCONTROL_HALTREQ_EN |
	           RGX_CR_FWCORE_DMI_DMCONTROL_DMACTIVE_EN,
	           PDUMP_FLAGS_CONTINUOUS);

	/* Wait until hart is halted */
	PDUMPREGPOL(psDevInfo->psDeviceNode,
	            RGX_PDUMPREG_NAME,
	            ui32_DMI_DMSTATUS_Reg,
	            RGX_CR_FWCORE_DMI_DMSTATUS_ALLHALTED_EN,
	            RGX_CR_FWCORE_DMI_DMSTATUS_ALLHALTED_EN,
	            PDUMP_FLAGS_CONTINUOUS,
	            PDUMP_POLL_OPERATOR_EQUAL);

	/* Clear halt request */
	PDUMPREG32(psDevInfo->psDeviceNode,
	           RGX_PDUMPREG_NAME, ui32_DMI_DMCONTROL_Reg,
	           RGX_CR_FWCORE_DMI_DMCONTROL_DMACTIVE_EN,
	           PDUMP_FLAGS_CONTINUOUS);
#else
	IMG_UINT32 __iomem *pui32RegsBase = RGX_GET_RISCV_REGS_BASE(psDevInfo);

	/* Send halt request (no need to select one or more harts on this RISC-V core) */
	OSWriteUncheckedHWReg32(pui32RegsBase, ui32_DMI_DMCONTROL_Reg,
	               RGX_CR_FWCORE_DMI_DMCONTROL_HALTREQ_EN |
	               RGX_CR_FWCORE_DMI_DMCONTROL_DMACTIVE_EN);

	/* Wait until hart is halted */
	if (PVRSRVPollForValueKM(psDevInfo->psDeviceNode,
	                         pui32RegsBase + ui32_DMI_DMSTATUS_Reg/sizeof(IMG_UINT32),
	                         RGX_CR_FWCORE_DMI_DMSTATUS_ALLHALTED_EN,
	                         RGX_CR_FWCORE_DMI_DMSTATUS_ALLHALTED_EN,
	                         POLL_FLAG_LOG_ERROR, NULL) != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: Hart not halted (0x%x)",
		         __func__, OSReadUncheckedHWReg32(pui32RegsBase, ui32_DMI_DMSTATUS_Reg)));
		return PVRSRV_ERROR_TIMEOUT;
	}

	/* Clear halt request */
	OSWriteUncheckedHWReg32(pui32RegsBase, ui32_DMI_DMCONTROL_Reg,
	               RGX_CR_FWCORE_DMI_DMCONTROL_DMACTIVE_EN);
#endif

	return PVRSRV_OK;
}

/*
 * RGXRiscvIsHalted
 */
IMG_BOOL RGXRiscvIsHalted(PVRSRV_RGXDEV_INFO *psDevInfo)
{
#if defined(NO_HARDWARE)
	PVR_UNREFERENCED_PARAMETER(psDevInfo);
	/* Assume the core is always halted in nohw */
	return IMG_TRUE;
#else
	IMG_UINT32 __iomem *pui32RegsBase = RGX_GET_RISCV_REGS_BASE(psDevInfo);
	IMG_UINT32 ui32_DMI_DMSTATUS_Reg = RGX_GET_DMI_REG(psDevInfo, DMSTATUS);

	return (OSReadUncheckedHWReg32(pui32RegsBase, ui32_DMI_DMSTATUS_Reg) &
	        RGX_CR_FWCORE_DMI_DMSTATUS_ALLHALTED_EN) != 0U;
#endif
}

/*
 * RGXRiscvResume
 */
PVRSRV_ERROR RGXRiscvResume(PVRSRV_RGXDEV_INFO *psDevInfo)
{
	__maybe_unused IMG_UINT32 ui32_DMI_DMCONTROL_Reg = RGX_GET_DMI_REG(psDevInfo, DMCONTROL);
	__maybe_unused IMG_UINT32 ui32_DMI_DMSTATUS_Reg = RGX_GET_DMI_REG(psDevInfo, DMSTATUS);

#if defined(NO_HARDWARE) && defined(PDUMP)
	PDUMPCOMMENTWITHFLAGS(psDevInfo->psDeviceNode,
	                      PDUMP_FLAGS_CONTINUOUS, "Resume RISC-V FW");

	/* Send resume request (no need to select one or more harts on this RISC-V core) */
	PDUMPREG32(psDevInfo->psDeviceNode,
	           RGX_PDUMPREG_NAME, ui32_DMI_DMCONTROL_Reg,
	           RGX_CR_FWCORE_DMI_DMCONTROL_RESUMEREQ_EN |
	           RGX_CR_FWCORE_DMI_DMCONTROL_DMACTIVE_EN,
	           PDUMP_FLAGS_CONTINUOUS);

	/* Wait until hart is resumed */
	PDUMPREGPOL(psDevInfo->psDeviceNode,
	            RGX_PDUMPREG_NAME,
	            ui32_DMI_DMSTATUS_Reg,
	            RGX_CR_FWCORE_DMI_DMSTATUS_ALLRESUMEACK_EN,
	            RGX_CR_FWCORE_DMI_DMSTATUS_ALLRESUMEACK_EN,
	            PDUMP_FLAGS_CONTINUOUS,
	            PDUMP_POLL_OPERATOR_EQUAL);

	/* Clear resume request */
	PDUMPREG32(psDevInfo->psDeviceNode,
	           RGX_PDUMPREG_NAME, ui32_DMI_DMCONTROL_Reg,
	           RGX_CR_FWCORE_DMI_DMCONTROL_DMACTIVE_EN,
	           PDUMP_FLAGS_CONTINUOUS);
#else
	IMG_UINT32 __iomem *pui32RegsBase = RGX_GET_RISCV_REGS_BASE(psDevInfo);

	/* Send resume request (no need to select one or more harts on this RISC-V core) */
	OSWriteUncheckedHWReg32(pui32RegsBase, ui32_DMI_DMCONTROL_Reg,
	               RGX_CR_FWCORE_DMI_DMCONTROL_RESUMEREQ_EN |
	               RGX_CR_FWCORE_DMI_DMCONTROL_DMACTIVE_EN);

	/* Wait until hart is resumed */
	if (PVRSRVPollForValueKM(psDevInfo->psDeviceNode,
	                         pui32RegsBase + ui32_DMI_DMSTATUS_Reg/sizeof(IMG_UINT32),
	                         RGX_CR_FWCORE_DMI_DMSTATUS_ALLRESUMEACK_EN,
	                         RGX_CR_FWCORE_DMI_DMSTATUS_ALLRESUMEACK_EN,
	                         POLL_FLAG_LOG_ERROR, NULL) != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: Hart not resumed (0x%x)",
		         __func__, OSReadUncheckedHWReg32(pui32RegsBase, ui32_DMI_DMSTATUS_Reg)));
		return PVRSRV_ERROR_TIMEOUT;
	}

	/* Clear resume request */
	OSWriteUncheckedHWReg32(pui32RegsBase, ui32_DMI_DMCONTROL_Reg,
	               RGX_CR_FWCORE_DMI_DMCONTROL_DMACTIVE_EN);
#endif

	return PVRSRV_OK;
}

/*!
*******************************************************************************
@Function       RGXRiscvCheckAbstractCmdError

@Description    Check for RISC-V abstract command errors and clear them

@Input          psDevInfo    Pointer to GPU device info

@Return         RGXRISCVFW_ABSTRACT_CMD_ERR
******************************************************************************/
static RGXRISCVFW_ABSTRACT_CMD_ERR RGXRiscvCheckAbstractCmdError(PVRSRV_RGXDEV_INFO *psDevInfo)
{
	RGXRISCVFW_ABSTRACT_CMD_ERR eCmdErr;

	__maybe_unused IMG_UINT32 ui32_DMI_ABSTRACTCS_Reg = RGX_GET_DMI_REG(psDevInfo, ABSTRACTCS);

#if defined(NO_HARDWARE) && defined(PDUMP)
	eCmdErr = RISCV_ABSTRACT_CMD_NO_ERROR;

	/* Check error status */
	PDUMPREGPOL(psDevInfo->psDeviceNode,
	            RGX_PDUMPREG_NAME,
	            ui32_DMI_ABSTRACTCS_Reg,
	            RISCV_ABSTRACT_CMD_NO_ERROR << RGX_CR_FWCORE_DMI_ABSTRACTCS_CMDERR_SHIFT,
	            ~RGX_CR_FWCORE_DMI_ABSTRACTCS_CMDERR_CLRMSK,
	            PDUMP_FLAGS_CONTINUOUS,
	            PDUMP_POLL_OPERATOR_EQUAL);
#else
	void __iomem *pvRegsBaseKM = RGX_GET_RISCV_REGS_BASE(psDevInfo);

	/* Check error status */
	eCmdErr = (OSReadUncheckedHWReg32(pvRegsBaseKM, ui32_DMI_ABSTRACTCS_Reg)
	          & ~RGX_CR_FWCORE_DMI_ABSTRACTCS_CMDERR_CLRMSK)
	          >> RGX_CR_FWCORE_DMI_ABSTRACTCS_CMDERR_SHIFT;

	if (eCmdErr != RISCV_ABSTRACT_CMD_NO_ERROR)
	{
		PVR_DPF((PVR_DBG_WARNING, "RISC-V FW abstract command error %u", eCmdErr));

		/* Clear the error (note CMDERR field is write-1-to-clear) */
		OSWriteUncheckedHWReg32(pvRegsBaseKM, ui32_DMI_ABSTRACTCS_Reg,
		               ~RGX_CR_FWCORE_DMI_ABSTRACTCS_CMDERR_CLRMSK);
	}
#endif

	return eCmdErr;
}

/*
 * RGXRiscReadReg
 */
PVRSRV_ERROR RGXRiscvReadReg(PVRSRV_RGXDEV_INFO *psDevInfo,
                             IMG_UINT32 ui32RegAddr,
                             IMG_UINT32 *pui32Value)
{
	__maybe_unused IMG_UINT32 ui32_DMI_ABSTRACTCS_Reg = RGX_GET_DMI_REG(psDevInfo, ABSTRACTCS);
	__maybe_unused IMG_UINT32 ui32_DMI_COMMAND_Reg = RGX_GET_DMI_REG(psDevInfo, COMMAND);
	__maybe_unused IMG_UINT32 ui32_DMI_DATA0_Reg = RGX_GET_DMI_REG(psDevInfo, DATA0);

#if defined(NO_HARDWARE) && defined(PDUMP)
	PVR_UNREFERENCED_PARAMETER(psDevInfo);
	PVR_UNREFERENCED_PARAMETER(ui32RegAddr);
	PVR_UNREFERENCED_PARAMETER(pui32Value);

	/* Reading HW registers is not supported in nohw/pdump */
	return PVRSRV_ERROR_NOT_SUPPORTED;
#else
	IMG_UINT32 __iomem *pui32RegsBase = RGX_GET_RISCV_REGS_BASE(psDevInfo);

	/* Send abstract register read command */
	OSWriteUncheckedHWReg32(pui32RegsBase,
	               ui32_DMI_COMMAND_Reg,
	               (RGXRISCVFW_DMI_COMMAND_ACCESS_REGISTER << RGX_CR_FWCORE_DMI_COMMAND_CMDTYPE_SHIFT) |
	               RGXRISCVFW_DMI_COMMAND_READ |
	               RGXRISCVFW_DMI_COMMAND_AAxSIZE_32BIT |
	               ui32RegAddr);

	/* Wait until abstract command is completed */
	if (PVRSRVPollForValueKM(psDevInfo->psDeviceNode,
	                         pui32RegsBase + ui32_DMI_ABSTRACTCS_Reg/sizeof(IMG_UINT32),
	                         0U,
	                         RGX_CR_FWCORE_DMI_ABSTRACTCS_BUSY_EN,
	                         POLL_FLAG_LOG_ERROR, NULL) != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: Abstract command did not complete in time (abstractcs = 0x%x)",
		         __func__, OSReadUncheckedHWReg32(pui32RegsBase, ui32_DMI_ABSTRACTCS_Reg)));
		return PVRSRV_ERROR_TIMEOUT;
	}

	if (RGXRiscvCheckAbstractCmdError(psDevInfo) == RISCV_ABSTRACT_CMD_NO_ERROR)
	{
		/* Read register value */
		*pui32Value = OSReadUncheckedHWReg32(pui32RegsBase, ui32_DMI_DATA0_Reg);
	}
	else
	{
		*pui32Value = 0U;
	}

	return PVRSRV_OK;
#endif
}

/*
 * RGXRiscvPollReg
 */
PVRSRV_ERROR RGXRiscvPollReg(PVRSRV_RGXDEV_INFO *psDevInfo,
                             IMG_UINT32 ui32RegAddr,
                             IMG_UINT32 ui32Value)
{
	__maybe_unused IMG_UINT32 ui32_DMI_ABSTRACTCS_Reg = RGX_GET_DMI_REG(psDevInfo, ABSTRACTCS);
	__maybe_unused IMG_UINT32 ui32_DMI_COMMAND_Reg = RGX_GET_DMI_REG(psDevInfo, COMMAND);
	__maybe_unused IMG_UINT32 ui32_DMI_DATA0_Reg = RGX_GET_DMI_REG(psDevInfo, DATA0);

#if defined(NO_HARDWARE) && defined(PDUMP)
	PDUMPCOMMENTWITHFLAGS(psDevInfo->psDeviceNode, PDUMP_FLAGS_CONTINUOUS,
	                      "Poll RISC-V register 0x%x (expected 0x%08x)",
	                      ui32RegAddr, ui32Value);

	/* Send abstract register read command */
	PDUMPREG32(psDevInfo->psDeviceNode, RGX_PDUMPREG_NAME, ui32_DMI_COMMAND_Reg,
	           (RGXRISCVFW_DMI_COMMAND_ACCESS_REGISTER << RGX_CR_FWCORE_DMI_COMMAND_CMDTYPE_SHIFT) |
	           RGXRISCVFW_DMI_COMMAND_READ |
	           RGXRISCVFW_DMI_COMMAND_AAxSIZE_32BIT |
	           ui32RegAddr,
	           PDUMP_FLAGS_CONTINUOUS);

	/* Wait until abstract command is completed */
	PDUMPREGPOL(psDevInfo->psDeviceNode,
	            RGX_PDUMPREG_NAME,
	            ui32_DMI_ABSTRACTCS_Reg,
	            0U,
	            RGX_CR_FWCORE_DMI_ABSTRACTCS_BUSY_EN,
	            PDUMP_FLAGS_CONTINUOUS,
	            PDUMP_POLL_OPERATOR_EQUAL);

	RGXRiscvCheckAbstractCmdError(psDevInfo);

	/* Check read value */
	PDUMPREGPOL(psDevInfo->psDeviceNode,
	            RGX_PDUMPREG_NAME,
	            ui32_DMI_DATA0_Reg,
	            ui32Value,
	            0xFFFFFFFF,
	            PDUMP_FLAGS_CONTINUOUS,
	            PDUMP_POLL_OPERATOR_EQUAL);

	return PVRSRV_OK;
#else
	PVR_UNREFERENCED_PARAMETER(psDevInfo);
	PVR_UNREFERENCED_PARAMETER(ui32RegAddr);
	PVR_UNREFERENCED_PARAMETER(ui32Value);

	/* Polling HW registers is currently not required driverlive */
	return PVRSRV_ERROR_NOT_SUPPORTED;
#endif
}

/*
 * RGXRiscvWriteReg
 */
PVRSRV_ERROR RGXRiscvWriteReg(PVRSRV_RGXDEV_INFO *psDevInfo,
                              IMG_UINT32 ui32RegAddr,
                              IMG_UINT32 ui32Value)
{
	__maybe_unused IMG_UINT32 ui32_DMI_ABSTRACTCS_Reg = RGX_GET_DMI_REG(psDevInfo, ABSTRACTCS);
	__maybe_unused IMG_UINT32 ui32_DMI_COMMAND_Reg = RGX_GET_DMI_REG(psDevInfo, COMMAND);
	__maybe_unused IMG_UINT32 ui32_DMI_DATA0_Reg = RGX_GET_DMI_REG(psDevInfo, DATA0);

#if defined(NO_HARDWARE) && defined(PDUMP)
	PDUMPCOMMENTWITHFLAGS(psDevInfo->psDeviceNode, PDUMP_FLAGS_CONTINUOUS,
	                      "Write RISC-V register 0x%x (value 0x%08x)",
	                      ui32RegAddr, ui32Value);

	/* Prepare data to be written to register */
	PDUMPREG32(psDevInfo->psDeviceNode, RGX_PDUMPREG_NAME, ui32_DMI_DATA0_Reg,
	           ui32Value, PDUMP_FLAGS_CONTINUOUS);

	/* Send abstract register write command */
	PDUMPREG32(psDevInfo->psDeviceNode, RGX_PDUMPREG_NAME, ui32_DMI_COMMAND_Reg,
	           (RGXRISCVFW_DMI_COMMAND_ACCESS_REGISTER << RGX_CR_FWCORE_DMI_COMMAND_CMDTYPE_SHIFT) |
	           RGXRISCVFW_DMI_COMMAND_WRITE |
	           RGXRISCVFW_DMI_COMMAND_AAxSIZE_32BIT |
	           ui32RegAddr,
	           PDUMP_FLAGS_CONTINUOUS);

	/* Wait until abstract command is completed */
	PDUMPREGPOL(psDevInfo->psDeviceNode,
	            RGX_PDUMPREG_NAME,
	            ui32_DMI_ABSTRACTCS_Reg,
	            0U,
	            RGX_CR_FWCORE_DMI_ABSTRACTCS_BUSY_EN,
	            PDUMP_FLAGS_CONTINUOUS,
	            PDUMP_POLL_OPERATOR_EQUAL);
#else
	IMG_UINT32 __iomem *pui32RegsBase = RGX_GET_RISCV_REGS_BASE(psDevInfo);

	/* Prepare data to be written to register */
	OSWriteUncheckedHWReg32(pui32RegsBase, ui32_DMI_DATA0_Reg, ui32Value);

	/* Send abstract register write command */
	OSWriteUncheckedHWReg32(pui32RegsBase,
	               ui32_DMI_COMMAND_Reg,
	               (RGXRISCVFW_DMI_COMMAND_ACCESS_REGISTER << RGX_CR_FWCORE_DMI_COMMAND_CMDTYPE_SHIFT) |
	               RGXRISCVFW_DMI_COMMAND_WRITE |
	               RGXRISCVFW_DMI_COMMAND_AAxSIZE_32BIT |
	               ui32RegAddr);

	/* Wait until abstract command is completed */
	if (PVRSRVPollForValueKM(psDevInfo->psDeviceNode,
	                         pui32RegsBase + ui32_DMI_ABSTRACTCS_Reg/sizeof(IMG_UINT32),
	                         0U,
	                         RGX_CR_FWCORE_DMI_ABSTRACTCS_BUSY_EN,
	                         POLL_FLAG_LOG_ERROR, NULL) != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: Abstract command did not complete in time (abstractcs = 0x%x)",
		         __func__, OSReadUncheckedHWReg32(pui32RegsBase, ui32_DMI_ABSTRACTCS_Reg)));
		return PVRSRV_ERROR_TIMEOUT;
	}
#endif

	return PVRSRV_OK;
}

/*!
*******************************************************************************
@Function       RGXRiscvCheckSysBusError

@Description    Check for RISC-V system bus errors and clear them

@Input          psDevInfo    Pointer to GPU device info

@Return         RGXRISCVFW_SYSBUS_ERR
******************************************************************************/
static __maybe_unused RGXRISCVFW_SYSBUS_ERR RGXRiscvCheckSysBusError(PVRSRV_RGXDEV_INFO *psDevInfo)
{
	RGXRISCVFW_SYSBUS_ERR eSBError;

	__maybe_unused IMG_UINT32 ui32_DMI_SBCS_Reg = RGX_GET_DMI_REG(psDevInfo, SBCS);

#if defined(NO_HARDWARE) && defined(PDUMP)
	eSBError = RISCV_SYSBUS_NO_ERROR;

	PDUMPREGPOL(psDevInfo->psDeviceNode,
	            RGX_PDUMPREG_NAME,
	            ui32_DMI_SBCS_Reg,
	            RISCV_SYSBUS_NO_ERROR << RGX_CR_FWCORE_DMI_SBCS_SBERROR_SHIFT,
	            ~RGX_CR_FWCORE_DMI_SBCS_SBERROR_CLRMSK,
	            PDUMP_FLAGS_CONTINUOUS,
	            PDUMP_POLL_OPERATOR_EQUAL);
#else
	void __iomem *pvRegsBaseKM = RGX_GET_RISCV_REGS_BASE(psDevInfo);

	eSBError = (OSReadUncheckedHWReg32(pvRegsBaseKM, ui32_DMI_SBCS_Reg)
	         & ~RGX_CR_FWCORE_DMI_SBCS_SBERROR_CLRMSK)
	         >> RGX_CR_FWCORE_DMI_SBCS_SBERROR_SHIFT;

	if (eSBError != RISCV_SYSBUS_NO_ERROR)
	{
		PVR_DPF((PVR_DBG_WARNING, "RISC-V FW system bus error %u", eSBError));

		/* Clear the error (note SBERROR field is write-1-to-clear) */
		OSWriteUncheckedHWReg32(pvRegsBaseKM, ui32_DMI_SBCS_Reg,
		               ~RGX_CR_FWCORE_DMI_SBCS_SBERROR_CLRMSK);
	}
#endif

	return eSBError;
}

#if !defined(EMULATOR)
/*!
*******************************************************************************
@Function       RGXRiscvReadAbstractMem

@Description    Read a value at the given address in RISC-V memory space
                using RISC-V abstract memory commands

@Input          psDevInfo       Pointer to device info
@Input          ui32Addr        Address in RISC-V memory space

@Output         pui32Value      Read value

@Return         PVRSRV_ERROR
******************************************************************************/
static PVRSRV_ERROR
RGXRiscvReadAbstractMem(PVRSRV_RGXDEV_INFO *psDevInfo, IMG_UINT32 ui32Addr, IMG_UINT32 *pui32Value)
{
	__maybe_unused IMG_UINT32 ui32_DMI_ABSTRACTCS_Reg = RGX_GET_DMI_REG(psDevInfo, ABSTRACTCS);
	__maybe_unused IMG_UINT32 ui32_DMI_COMMAND_Reg = RGX_GET_DMI_REG(psDevInfo, COMMAND);
	__maybe_unused IMG_UINT32 ui32_DMI_DATA0_Reg = RGX_GET_DMI_REG(psDevInfo, DATA0);
	__maybe_unused IMG_UINT32 ui32_DMI_DATA1_Reg = RGX_GET_DMI_REG(psDevInfo, DATA0);
	__maybe_unused IMG_UINT32 ui32_DMI_SBCS_Reg = RGX_GET_DMI_REG(psDevInfo, SBCS);

#if defined(NO_HARDWARE) && defined(PDUMP)
	PVR_UNREFERENCED_PARAMETER(psDevInfo);
	PVR_UNREFERENCED_PARAMETER(ui32Addr);
	PVR_UNREFERENCED_PARAMETER(pui32Value);

	/* Reading memory is not supported in nohw/pdump */
	return PVRSRV_ERROR_NOT_SUPPORTED;
#else
	IMG_UINT32 __iomem *pui32RegsBase = RGX_GET_RISCV_REGS_BASE(psDevInfo);

	/* Prepare read address */
	OSWriteUncheckedHWReg32(pui32RegsBase, ui32_DMI_DATA1_Reg, ui32Addr);

	/* Send abstract memory read command */
	OSWriteUncheckedHWReg32(pui32RegsBase,
	               ui32_DMI_COMMAND_Reg,
	               (RGXRISCVFW_DMI_COMMAND_ACCESS_MEMORY << RGX_CR_FWCORE_DMI_COMMAND_CMDTYPE_SHIFT) |
	               RGXRISCVFW_DMI_COMMAND_READ |
	               RGXRISCVFW_DMI_COMMAND_AAxSIZE_32BIT);

	/* Wait until abstract command is completed */
	if (PVRSRVPollForValueKM(psDevInfo->psDeviceNode,
	                         pui32RegsBase + ui32_DMI_ABSTRACTCS_Reg/sizeof(IMG_UINT32),
	                         0U,
	                         RGX_CR_FWCORE_DMI_ABSTRACTCS_BUSY_EN,
	                         POLL_FLAG_LOG_ERROR, NULL) != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: Abstract command did not complete in time (abstractcs = 0x%x)",
		         __func__, OSReadUncheckedHWReg32(pui32RegsBase, ui32_DMI_ABSTRACTCS_Reg)));
		return PVRSRV_ERROR_TIMEOUT;
	}

	if (RGXRiscvCheckAbstractCmdError(psDevInfo) == RISCV_ABSTRACT_CMD_NO_ERROR)
	{
		/* Read memory value */
		*pui32Value = OSReadUncheckedHWReg32(pui32RegsBase, ui32_DMI_DATA0_Reg);
	}
	else
	{
		*pui32Value = 0U;
	}

	return PVRSRV_OK;
#endif
}
#endif /* !defined(EMULATOR) */

/*!
*******************************************************************************
@Function       RGXRiscvPollAbstractMem

@Description    Poll for a value at the given address in RISC-V memory space
                using RISC-V abstract memory commands

@Input          psDevInfo       Pointer to device info
@Input          ui32Addr        Address in RISC-V memory space
@Input          ui32Value       Expected value

@Return         PVRSRV_ERROR
******************************************************************************/
static PVRSRV_ERROR
RGXRiscvPollAbstractMem(PVRSRV_RGXDEV_INFO *psDevInfo, IMG_UINT32 ui32Addr, IMG_UINT32 ui32Value)
{
	__maybe_unused IMG_UINT32 ui32_DMI_ABSTRACTCS_Reg = RGX_GET_DMI_REG(psDevInfo, ABSTRACTCS);
	__maybe_unused IMG_UINT32 ui32_DMI_COMMAND_Reg = RGX_GET_DMI_REG(psDevInfo, COMMAND);
	__maybe_unused IMG_UINT32 ui32_DMI_DATA0_Reg = RGX_GET_DMI_REG(psDevInfo, DATA0);
	__maybe_unused IMG_UINT32 ui32_DMI_DATA1_Reg = RGX_GET_DMI_REG(psDevInfo, DATA1);

#if defined(NO_HARDWARE) && defined(PDUMP)
	PDUMPCOMMENTWITHFLAGS(psDevInfo->psDeviceNode,
	                      PDUMP_FLAGS_CONTINUOUS,
	                      "Poll RISC-V address 0x%x (expected 0x%08x)",
	                      ui32Addr, ui32Value);

	/* Prepare read address */
	PDUMPREG32(psDevInfo->psDeviceNode, RGX_PDUMPREG_NAME, ui32_DMI_DATA1_Reg,
	           ui32Addr, PDUMP_FLAGS_CONTINUOUS);

	/* Send abstract memory read command */
	PDUMPREG32(psDevInfo->psDeviceNode, RGX_PDUMPREG_NAME, ui32_DMI_COMMAND_Reg,
	           (RGXRISCVFW_DMI_COMMAND_ACCESS_MEMORY << RGX_CR_FWCORE_DMI_COMMAND_CMDTYPE_SHIFT) |
	           RGXRISCVFW_DMI_COMMAND_READ |
	           RGXRISCVFW_DMI_COMMAND_AAxSIZE_32BIT,
	           PDUMP_FLAGS_CONTINUOUS);

	/* Wait until abstract command is completed */
	PDUMPREGPOL(psDevInfo->psDeviceNode,
	            RGX_PDUMPREG_NAME,
	            ui32_DMI_ABSTRACTCS_Reg,
	            0U,
	            RGX_CR_FWCORE_DMI_ABSTRACTCS_BUSY_EN,
	            PDUMP_FLAGS_CONTINUOUS,
	            PDUMP_POLL_OPERATOR_EQUAL);

	RGXRiscvCheckAbstractCmdError(psDevInfo);

	/* Check read value */
	PDUMPREGPOL(psDevInfo->psDeviceNode,
	            RGX_PDUMPREG_NAME,
	            ui32_DMI_DATA0_Reg,
	            ui32Value,
	            0xFFFFFFFF,
	            PDUMP_FLAGS_CONTINUOUS,
	            PDUMP_POLL_OPERATOR_EQUAL);

	return PVRSRV_OK;
#else
	PVR_UNREFERENCED_PARAMETER(psDevInfo);
	PVR_UNREFERENCED_PARAMETER(ui32Addr);
	PVR_UNREFERENCED_PARAMETER(ui32Value);

	/* Polling memory is currently not required driverlive */
	return PVRSRV_ERROR_NOT_SUPPORTED;
#endif
}

#if !defined(EMULATOR)
/*!
*******************************************************************************
@Function       RGXRiscvReadSysBusMem

@Description    Read a value at the given address in RISC-V memory space
                using the RISC-V system bus

@Input          psDevInfo       Pointer to device info
@Input          ui32Addr        Address in RISC-V memory space

@Output         pui32Value      Read value

@Return         PVRSRV_ERROR
******************************************************************************/
static PVRSRV_ERROR
RGXRiscvReadSysBusMem(PVRSRV_RGXDEV_INFO *psDevInfo, IMG_UINT32 ui32Addr, IMG_UINT32 *pui32Value)
{
	__maybe_unused IMG_UINT32 ui32_DMI_ABSTRACTCS_Reg = RGX_GET_DMI_REG(psDevInfo, ABSTRACTCS);
	__maybe_unused IMG_UINT32 ui32_DMI_COMMAND_Reg = RGX_GET_DMI_REG(psDevInfo, COMMAND);
	__maybe_unused IMG_UINT32 ui32_DMI_DATA0_Reg = RGX_GET_DMI_REG(psDevInfo, DATA0);
	__maybe_unused IMG_UINT32 ui32_DMI_DATA1_Reg = RGX_GET_DMI_REG(psDevInfo, DATA1);
	__maybe_unused IMG_UINT32 ui32_DMI_SBCS_Reg = RGX_GET_DMI_REG(psDevInfo, SBCS);
	__maybe_unused IMG_UINT32 ui32_DMI_SBADDRESS0_Reg = RGX_GET_DMI_REG(psDevInfo, SBADDRESS0);

#if defined(NO_HARDWARE) && defined(PDUMP)
	PVR_UNREFERENCED_PARAMETER(psDevInfo);
	PVR_UNREFERENCED_PARAMETER(ui32Addr);
	PVR_UNREFERENCED_PARAMETER(pui32Value);

	/* Reading memory is not supported in nohw/pdump */
	return PVRSRV_ERROR_NOT_SUPPORTED;
#else
	IMG_UINT32 __iomem *pui32RegsBase = RGX_GET_RISCV_REGS_BASE(psDevInfo);

	/* Configure system bus to read 32 bit every time a new address is provided */
	OSWriteUncheckedHWReg32(pui32RegsBase,
	               ui32_DMI_SBCS_Reg,
	               (RGXRISCVFW_DMI_SBCS_SBACCESS_32BIT << RGX_CR_FWCORE_DMI_SBCS_SBACCESS_SHIFT) |
	               RGX_CR_FWCORE_DMI_SBCS_SBREADONADDR_EN);

	/* Perform read */
	OSWriteUncheckedHWReg32(pui32RegsBase, ui32_DMI_SBADDRESS0_Reg, ui32Addr);

	/* Wait until system bus is idle */
	if (PVRSRVPollForValueKM(psDevInfo->psDeviceNode,
	                         pui32RegsBase + ui32_DMI_SBCS_Reg/sizeof(IMG_UINT32),
	                         0U,
	                         RGX_CR_FWCORE_DMI_SBCS_SBBUSY_EN,
	                         POLL_FLAG_LOG_ERROR, NULL) != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: System Bus did not go idle in time (sbcs = 0x%x)",
		         __func__, OSReadUncheckedHWReg32(pui32RegsBase, ui32_DMI_SBCS_Reg)));
		return PVRSRV_ERROR_TIMEOUT;
	}

	if (RGXRiscvCheckSysBusError(psDevInfo) == RISCV_SYSBUS_NO_ERROR)
	{
		/* Read value from debug system bus */
		*pui32Value = OSReadUncheckedHWReg32(pui32RegsBase, ui32_DMI_DATA0_Reg);
	}
	else
	{
		*pui32Value = 0U;
	}

	return PVRSRV_OK;
#endif
}
#endif /* !defined(EMULATOR) */

/*!
*******************************************************************************
@Function       RGXRiscvPollSysBusMem

@Description    Poll for a value at the given address in RISC-V memory space
                using the RISC-V system bus

@Input          psDevInfo       Pointer to device info
@Input          ui32Addr        Address in RISC-V memory space
@Input          ui32Value       Expected value

@Return         PVRSRV_ERROR
******************************************************************************/
static PVRSRV_ERROR
RGXRiscvPollSysBusMem(PVRSRV_RGXDEV_INFO *psDevInfo, IMG_UINT32 ui32Addr, IMG_UINT32 ui32Value)
{
	__maybe_unused IMG_UINT32 ui32_DMI_ABSTRACTCS_Reg = RGX_GET_DMI_REG(psDevInfo, ABSTRACTCS);
	__maybe_unused IMG_UINT32 ui32_DMI_COMMAND_Reg = RGX_GET_DMI_REG(psDevInfo, COMMAND);
	__maybe_unused IMG_UINT32 ui32_DMI_DATA0_Reg = RGX_GET_DMI_REG(psDevInfo, DATA0);
	__maybe_unused IMG_UINT32 ui32_DMI_DATA1_Reg = RGX_GET_DMI_REG(psDevInfo, DATA1);
	__maybe_unused IMG_UINT32 ui32_DMI_SBCS_Reg = RGX_GET_DMI_REG(psDevInfo, SBCS);
	__maybe_unused IMG_UINT32 ui32_DMI_SBADDRESS0_Reg = RGX_GET_DMI_REG(psDevInfo, SBADDRESS0);
	__maybe_unused IMG_UINT32 ui32_DMI_SBDATA0_Reg = RGX_GET_DMI_REG(psDevInfo, SBDATA0);

#if defined(NO_HARDWARE) && defined(PDUMP)
	PDUMPCOMMENTWITHFLAGS(psDevInfo->psDeviceNode, PDUMP_FLAGS_CONTINUOUS,
	                      "Poll RISC-V address 0x%x (expected 0x%08x)",
	                      ui32Addr, ui32Value);

	/* Configure system bus to read 32 bit every time a new address is provided */
	PDUMPREG32(psDevInfo->psDeviceNode, RGX_PDUMPREG_NAME, ui32_DMI_SBCS_Reg,
	           (RGXRISCVFW_DMI_SBCS_SBACCESS_32BIT << RGX_CR_FWCORE_DMI_SBCS_SBACCESS_SHIFT) |
	           RGX_CR_FWCORE_DMI_SBCS_SBREADONADDR_EN,
	           PDUMP_FLAGS_CONTINUOUS);

	/* Perform read */
	PDUMPREG32(psDevInfo->psDeviceNode,
	           RGX_PDUMPREG_NAME, ui32_DMI_SBADDRESS0_Reg,
	           ui32Addr,
	           PDUMP_FLAGS_CONTINUOUS);

	/* Wait until system bus is idle */
	PDUMPREGPOL(psDevInfo->psDeviceNode,
	            RGX_PDUMPREG_NAME,
	            ui32_DMI_SBCS_Reg,
	            0U,
	            RGX_CR_FWCORE_DMI_SBCS_SBBUSY_EN,
	            PDUMP_FLAGS_CONTINUOUS,
	            PDUMP_POLL_OPERATOR_EQUAL);

	RGXRiscvCheckSysBusError(psDevInfo);

	/* Check read value */
	PDUMPREGPOL(psDevInfo->psDeviceNode,
	            RGX_PDUMPREG_NAME,
	            ui32_DMI_SBDATA0_Reg,
	            ui32Value,
	            0xFFFFFFFF,
	            PDUMP_FLAGS_CONTINUOUS,
	            PDUMP_POLL_OPERATOR_EQUAL);

	return PVRSRV_OK;
#else
	PVR_UNREFERENCED_PARAMETER(psDevInfo);
	PVR_UNREFERENCED_PARAMETER(ui32Addr);
	PVR_UNREFERENCED_PARAMETER(ui32Value);

	/* Polling memory is currently not required driverlive */
	return PVRSRV_ERROR_NOT_SUPPORTED;
#endif
}

#if !defined(EMULATOR)
/*
 * RGXRiscvReadMem
 */
PVRSRV_ERROR RGXRiscvReadMem(PVRSRV_RGXDEV_INFO *psDevInfo,
                             IMG_UINT32 ui32Addr,
                             IMG_UINT32 *pui32Value)
{
	if (ui32Addr >= RGXRISCVFW_COREMEM_BASE && ui32Addr <= RGXRISCVFW_COREMEM_END)
	{
		return RGXRiscvReadAbstractMem(psDevInfo, ui32Addr, pui32Value);
	}

	return RGXRiscvReadSysBusMem(psDevInfo, ui32Addr, pui32Value);
}
#endif /* !defined(EMULATOR) */

/*
 * RGXRiscvPollMem
 */
PVRSRV_ERROR RGXRiscvPollMem(PVRSRV_RGXDEV_INFO *psDevInfo,
                             IMG_UINT32 ui32Addr,
                             IMG_UINT32 ui32Value)
{
	if (ui32Addr >= RGXRISCVFW_COREMEM_BASE && ui32Addr <= RGXRISCVFW_COREMEM_END)
	{
		return RGXRiscvPollAbstractMem(psDevInfo, ui32Addr, ui32Value);
	}

	return RGXRiscvPollSysBusMem(psDevInfo, ui32Addr, ui32Value);
}

#if !defined(EMULATOR)
/*!
*******************************************************************************
@Function       RGXRiscvWriteAbstractMem

@Description    Write a value at the given address in RISC-V memory space
                using RISC-V abstract memory commands

@Input          psDevInfo       Pointer to device info
@Input          ui32Addr        Address in RISC-V memory space
@Input          ui32Value       Write value

@Return         PVRSRV_ERROR
******************************************************************************/
static PVRSRV_ERROR
RGXRiscvWriteAbstractMem(PVRSRV_RGXDEV_INFO *psDevInfo, IMG_UINT32 ui32Addr, IMG_UINT32 ui32Value)
{
	__maybe_unused IMG_UINT32 ui32_DMI_ABSTRACTCS_Reg = RGX_GET_DMI_REG(psDevInfo, ABSTRACTCS);
	__maybe_unused IMG_UINT32 ui32_DMI_COMMAND_Reg = RGX_GET_DMI_REG(psDevInfo, COMMAND);
	__maybe_unused IMG_UINT32 ui32_DMI_DATA0_Reg = RGX_GET_DMI_REG(psDevInfo, DATA0);
	__maybe_unused IMG_UINT32 ui32_DMI_DATA1_Reg = RGX_GET_DMI_REG(psDevInfo, DATA1);
	__maybe_unused IMG_UINT32 ui32_DMI_SBCS_Reg = RGX_GET_DMI_REG(psDevInfo, SBCS);
	__maybe_unused IMG_UINT32 ui32_DMI_SBADDRESS0_Reg = RGX_GET_DMI_REG(psDevInfo, SBADDRESS0);

#if defined(NO_HARDWARE) && defined(PDUMP)
	PDUMPCOMMENTWITHFLAGS(psDevInfo->psDeviceNode, PDUMP_FLAGS_CONTINUOUS,
	                      "Write RISC-V address 0x%x (value 0x%08x)",
	                      ui32Addr, ui32Value);

	/* Prepare write address */
	PDUMPREG32(psDevInfo->psDeviceNode, RGX_PDUMPREG_NAME, ui32_DMI_DATA1_Reg,
	           ui32Addr, PDUMP_FLAGS_CONTINUOUS);

	/* Prepare write data */
	PDUMPREG32(psDevInfo->psDeviceNode, RGX_PDUMPREG_NAME, ui32_DMI_DATA0_Reg,
	           ui32Value, PDUMP_FLAGS_CONTINUOUS);

	/* Send abstract register write command */
	PDUMPREG32(psDevInfo->psDeviceNode, RGX_PDUMPREG_NAME, ui32_DMI_COMMAND_Reg,
	           (RGXRISCVFW_DMI_COMMAND_ACCESS_MEMORY << RGX_CR_FWCORE_DMI_COMMAND_CMDTYPE_SHIFT) |
	           RGXRISCVFW_DMI_COMMAND_WRITE |
	           RGXRISCVFW_DMI_COMMAND_AAxSIZE_32BIT,
	           PDUMP_FLAGS_CONTINUOUS);

	/* Wait until abstract command is completed */
	PDUMPREGPOL(psDevInfo->psDeviceNode,
	            RGX_PDUMPREG_NAME,
	            ui32_DMI_ABSTRACTCS_Reg,
	            0U,
	            RGX_CR_FWCORE_DMI_ABSTRACTCS_BUSY_EN,
	            PDUMP_FLAGS_CONTINUOUS,
	            PDUMP_POLL_OPERATOR_EQUAL);
#else
	IMG_UINT32 __iomem *pui32RegsBase = RGX_GET_RISCV_REGS_BASE(psDevInfo);

	/* Prepare write address */
	OSWriteUncheckedHWReg32(pui32RegsBase, ui32_DMI_DATA1_Reg, ui32Addr);

	/* Prepare write data */
	OSWriteUncheckedHWReg32(pui32RegsBase, ui32_DMI_DATA0_Reg, ui32Value);

	/* Send abstract memory write command */
	OSWriteUncheckedHWReg32(pui32RegsBase,
	               ui32_DMI_COMMAND_Reg,
	               (RGXRISCVFW_DMI_COMMAND_ACCESS_MEMORY << RGX_CR_FWCORE_DMI_COMMAND_CMDTYPE_SHIFT) |
	               RGXRISCVFW_DMI_COMMAND_WRITE |
	               RGXRISCVFW_DMI_COMMAND_AAxSIZE_32BIT);

	/* Wait until abstract command is completed */
	if (PVRSRVPollForValueKM(psDevInfo->psDeviceNode,
	                         pui32RegsBase + ui32_DMI_ABSTRACTCS_Reg/sizeof(IMG_UINT32),
	                         0U,
	                         RGX_CR_FWCORE_DMI_ABSTRACTCS_BUSY_EN,
	                         POLL_FLAG_LOG_ERROR, NULL) != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: Abstract command did not complete in time (abstractcs = 0x%x)",
		         __func__, OSReadUncheckedHWReg32(pui32RegsBase, ui32_DMI_ABSTRACTCS_Reg)));
		return PVRSRV_ERROR_TIMEOUT;
	}
#endif

	return PVRSRV_OK;
}

/*!
*******************************************************************************
@Function       RGXRiscvWriteSysBusMem

@Description    Write a value at the given address in RISC-V memory space
                using the RISC-V system bus

@Input          psDevInfo       Pointer to device info
@Input          ui32Addr        Address in RISC-V memory space
@Input          ui32Value       Write value

@Return         PVRSRV_ERROR
******************************************************************************/
static PVRSRV_ERROR
RGXRiscvWriteSysBusMem(PVRSRV_RGXDEV_INFO *psDevInfo, IMG_UINT32 ui32Addr, IMG_UINT32 ui32Value)
{
	__maybe_unused IMG_UINT32 ui32_DMI_ABSTRACTCS_Reg = RGX_GET_DMI_REG(psDevInfo, ABSTRACTCS);
	__maybe_unused IMG_UINT32 ui32_DMI_COMMAND_Reg = RGX_GET_DMI_REG(psDevInfo, COMMAND);
	__maybe_unused IMG_UINT32 ui32_DMI_DATA0_Reg = RGX_GET_DMI_REG(psDevInfo, DATA0);
	__maybe_unused IMG_UINT32 ui32_DMI_SBDATA0_Reg = RGX_GET_DMI_REG(psDevInfo, SBDATA0);
	__maybe_unused IMG_UINT32 ui32_DMI_DATA1_Reg = RGX_GET_DMI_REG(psDevInfo, DATA1);
	__maybe_unused IMG_UINT32 ui32_DMI_SBCS_Reg = RGX_GET_DMI_REG(psDevInfo, SBCS);
	__maybe_unused IMG_UINT32 ui32_DMI_SBADDRESS0_Reg = RGX_GET_DMI_REG(psDevInfo, SBADDRESS0);

#if defined(NO_HARDWARE) && defined(PDUMP)
	PDUMPCOMMENTWITHFLAGS(psDevInfo->psDeviceNode, PDUMP_FLAGS_CONTINUOUS,
	                      "Write RISC-V address 0x%x (value 0x%08x)",
	                      ui32Addr, ui32Value);

	/* Configure system bus to read 32 bit every time a new address is provided */
	PDUMPREG32(psDevInfo->psDeviceNode, RGX_PDUMPREG_NAME, ui32_DMI_SBCS_Reg,
	           RGXRISCVFW_DMI_SBCS_SBACCESS_32BIT << RGX_CR_FWCORE_DMI_SBCS_SBACCESS_SHIFT,
	           PDUMP_FLAGS_CONTINUOUS);

	/* Prepare write address */
	PDUMPREG32(psDevInfo->psDeviceNode, RGX_PDUMPREG_NAME, ui32_DMI_SBADDRESS0_Reg,
	           ui32Addr, PDUMP_FLAGS_CONTINUOUS);

	/* Prepare write data and initiate write */
	PDUMPREG32(psDevInfo->psDeviceNode, RGX_PDUMPREG_NAME, ui32_DMI_SBDATA0_Reg,
	           ui32Value, PDUMP_FLAGS_CONTINUOUS);

	/* Wait until system bus is idle */
	PDUMPREGPOL(psDevInfo->psDeviceNode,
	            RGX_PDUMPREG_NAME,
	            ui32_DMI_SBCS_Reg,
	            0U,
	            RGX_CR_FWCORE_DMI_SBCS_SBBUSY_EN,
	            PDUMP_FLAGS_CONTINUOUS,
	            PDUMP_POLL_OPERATOR_EQUAL);
#else
	IMG_UINT32 __iomem *pui32RegsBase = RGX_GET_RISCV_REGS_BASE(psDevInfo);

	/* Configure system bus for 32 bit accesses */
	OSWriteUncheckedHWReg32(pui32RegsBase,
	               ui32_DMI_SBCS_Reg,
	               RGXRISCVFW_DMI_SBCS_SBACCESS_32BIT << RGX_CR_FWCORE_DMI_SBCS_SBACCESS_SHIFT);

	/* Prepare write address */
	OSWriteUncheckedHWReg32(pui32RegsBase, ui32_DMI_SBADDRESS0_Reg, ui32Addr);

	/* Prepare write data and initiate write */
	OSWriteUncheckedHWReg32(pui32RegsBase, ui32_DMI_SBDATA0_Reg, ui32Value);

	/* Wait until system bus is idle */
	if (PVRSRVPollForValueKM(psDevInfo->psDeviceNode,
	                         pui32RegsBase + ui32_DMI_SBCS_Reg/sizeof(IMG_UINT32),
	                         0U,
	                         RGX_CR_FWCORE_DMI_SBCS_SBBUSY_EN,
	                         POLL_FLAG_LOG_ERROR, NULL) != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: System Bus did not go idle in time (sbcs = 0x%x)",
		         __func__, OSReadUncheckedHWReg32(pui32RegsBase, ui32_DMI_SBCS_Reg)));
		return PVRSRV_ERROR_TIMEOUT;
	}
#endif

	return PVRSRV_OK;
}

/*
 * RGXRiscvWriteMem
 */
PVRSRV_ERROR RGXRiscvWriteMem(PVRSRV_RGXDEV_INFO *psDevInfo,
                              IMG_UINT32 ui32Addr,
                              IMG_UINT32 ui32Value)
{
	if (ui32Addr >= RGXRISCVFW_COREMEM_BASE && ui32Addr <= RGXRISCVFW_COREMEM_END)
	{
		return RGXRiscvWriteAbstractMem(psDevInfo, ui32Addr, ui32Value);
	}

	return RGXRiscvWriteSysBusMem(psDevInfo, ui32Addr, ui32Value);
}
#endif /* !defined(EMULATOR) */

/*
 * RGXRiscvDmiOp
 */
PVRSRV_ERROR RGXRiscvDmiOp(PVRSRV_RGXDEV_INFO *psDevInfo,
                           IMG_UINT64 *pui64DMI)
{
#if defined(NO_HARDWARE) && defined(PDUMP)
	PVR_UNREFERENCED_PARAMETER(psDevInfo);
	PVR_UNREFERENCED_PARAMETER(pui64DMI);

	/* Accessing DM registers is not supported in nohw/pdump */
	return PVRSRV_ERROR_NOT_SUPPORTED;
#else
#if defined(RGX_FEATURE_HOST_SECURITY_VERSION_MAX_VALUE_IDX)
#define DMI_BASE     ((RGX_GET_FEATURE_VALUE(psDevInfo, HOST_SECURITY_VERSION) >= 4) ? RGX_CR_FWCORE_DMI_RESERVED00__HOST_SECURITY_GEQ4 : RGX_CR_FWCORE_DMI_RESERVED00)
#else
#define DMI_BASE     RGX_CR_FWCORE_DMI_RESERVED00
#endif
#define DMI_STRIDE  (RGX_CR_FWCORE_DMI_RESERVED01 - RGX_CR_FWCORE_DMI_RESERVED00)
#define DMI_REG(r)  ((DMI_BASE) + (DMI_STRIDE) * (r))

#define DMI_OP_SHIFT            0U
#define DMI_OP_MASK             0x3ULL
#define DMI_DATA_SHIFT          2U
#define DMI_DATA_MASK           0x3FFFFFFFCULL
#define DMI_ADDRESS_SHIFT       34U
#define DMI_ADDRESS_MASK        0xFC00000000ULL

#define DMI_OP_NOP	            0U
#define DMI_OP_READ	            1U
#define DMI_OP_WRITE	        2U
#define DMI_OP_RESERVED	        3U

#define DMI_OP_STATUS_SUCCESS	0U
#define DMI_OP_STATUS_RESERVED	1U
#define DMI_OP_STATUS_FAILED	2U
#define DMI_OP_STATUS_BUSY	    3U

	PVRSRV_DEVICE_NODE *psDeviceNode = psDevInfo->psDeviceNode;
	PVRSRV_DEV_POWER_STATE ePowerState;
	PVRSRV_ERROR eError;
	IMG_UINT64 ui64Op, ui64Address, ui64Data;

	ui64Op      = (*pui64DMI & DMI_OP_MASK) >> DMI_OP_SHIFT;
	ui64Address = (*pui64DMI & DMI_ADDRESS_MASK) >> DMI_ADDRESS_SHIFT;
	ui64Data    = (*pui64DMI & DMI_DATA_MASK) >> DMI_DATA_SHIFT;

	eError = PVRSRVPowerLock(psDeviceNode);

	if (eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: failed to acquire powerlock (%s)",
				__func__, PVRSRVGetErrorString(eError)));
		ui64Op = DMI_OP_STATUS_FAILED;
		goto dmiop_update;
	}

	eError = PVRSRVGetDevicePowerState(psDeviceNode, &ePowerState);

	if (eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: failed to retrieve RGX power state (%s)",
				__func__, PVRSRVGetErrorString(eError)));
		ui64Op = DMI_OP_STATUS_FAILED;
		goto dmiop_release_lock;
	}

	if (ePowerState == PVRSRV_DEV_POWER_STATE_ON)
	{
		void __iomem *pvRegsBase = RGX_GET_RISCV_REGS_BASE(psDevInfo);
		switch (ui64Op)
		{
			case DMI_OP_NOP:
				ui64Op = DMI_OP_STATUS_SUCCESS;
				break;
			case DMI_OP_WRITE:
				OSWriteUncheckedHWReg32(pvRegsBase,
						DMI_REG(ui64Address),
						(IMG_UINT32)ui64Data);
				ui64Op = DMI_OP_STATUS_SUCCESS;
				break;
			case DMI_OP_READ:
				ui64Data = (IMG_UINT64)OSReadUncheckedHWReg32(pvRegsBase,
						DMI_REG(ui64Address));
				ui64Op = DMI_OP_STATUS_SUCCESS;
				break;
			default:
				PVR_DPF((PVR_DBG_ERROR, "%s: unknown op %u", __func__, (IMG_UINT32)ui64Op));
				ui64Op = DMI_OP_STATUS_FAILED;
				break;
		}
	}
	else
	{
		PVR_DPF((PVR_DBG_WARNING, "%s: Accessing RISC-V Debug Module is not "
					"possible while the GPU is powered off", __func__));

		ui64Op = DMI_OP_STATUS_FAILED;
	}

dmiop_release_lock:
	PVRSRVPowerUnlock(psDeviceNode);

dmiop_update:
	*pui64DMI = (ui64Op << DMI_OP_SHIFT) |
		(ui64Address << DMI_ADDRESS_SHIFT) |
		(ui64Data << DMI_DATA_SHIFT);

	return eError;
#endif
}

/******************************************************************************
 End of file (rgxfwriscv.c)
******************************************************************************/
