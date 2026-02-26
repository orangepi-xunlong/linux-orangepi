/****************************************************************************
*
*    Copyright (c) 2005 - 2024 by Vivante Corp.  All rights reserved.
*
*    The material in this file is confidential and contains trade secrets
*    of Vivante Corporation. This is proprietary information owned by
*    Vivante Corporation. No part of this work may be disclosed,
*    reproduced, copied, transmitted, or used in any way for any purpose,
*    without the express written permission of Vivante Corporation.
*
*****************************************************************************/

#include "gc_hal_kernel_vxworks.h"
#include "gc_hal_kernel_allocator.h"

#define _GC_OBJ_ZONE    gcvZONE_DEVICE

static gckGALDEVICE galDevice;

extern gcTA globalTA[16];

/******************************************************************************\
*************************** Memory Allocation Wrappers *************************
\******************************************************************************/

static gceSTATUS
_AllocateMemory(
    IN gckGALDEVICE Device,
    IN gctSIZE_T Bytes,
    OUT gctPOINTER *Logical,
    OUT gctPHYS_ADDR *Physical,
    OUT gctUINT32 *PhysAddr
    )
{
    gceSTATUS status;
    gctPHYS_ADDR_T physAddr;

    gcmkHEADER_ARG("Device=0x%x Bytes=%lu", Device, Bytes);

    gcmkVERIFY_ARGUMENT(Device != NULL);
    gcmkVERIFY_ARGUMENT(Logical != NULL);
    gcmkVERIFY_ARGUMENT(Physical != NULL);
    gcmkVERIFY_ARGUMENT(PhysAddr != NULL);

    gcmkONERROR(gckOS_AllocateNonPagedMemory(
        Device->os, gcvFALSE, gcvALLOC_FLAG_CONTIGUOUS, &Bytes, Physical, Logical
        ));

    gcmkONERROR(gckOS_GetPhysicalFromHandle(
        Device->os, *Physical, 0, &physAddr
        ));

    *PhysAddr = physAddr;

OnError:
    gcmkFOOTER_ARG(
        "*Logical=%p *Physical=%p *PhysAddr=0x%llx",
        gcmOPT_POINTER(Logical), gcmOPT_POINTER(Physical), gcmOPT_VALUE(PhysAddr)
        );

    return status;
}

static gceSTATUS
_FreeMemory(
    IN gckGALDEVICE Device,
    IN gctPOINTER Logical,
    IN gctPHYS_ADDR Physical)
{
    gceSTATUS status;

    gcmkHEADER_ARG("Device=0x%x Logical=0x%x Physical=0x%x",
                   Device, Logical, Physical);

    gcmkVERIFY_ARGUMENT(Device != NULL);

    status = gckOS_FreeNonPagedMemory(
        Device->os, Physical, Logical,
        ((PVX_MDL) Physical)->numPages * PAGE_SIZE
        );

    gcmkFOOTER();
    return status;
}

static gceSTATUS
_SetupVidMem(
    IN gckGALDEVICE Device,
    IN gctUINT32 ContiguousBase,
    IN gctSIZE_T ContiguousSize,
    IN gctSIZE_T BankSize,
    IN gcsDEVICE_CONSTRUCT_ARGS * Args
    )
{
    gceSTATUS status;
    gctUINT32 physAddr = ~0U;
    gckGALDEVICE device = Device;

    /* set up the contiguous memory */
    device->contiguousBase = ContiguousBase;
    device->contiguousSize = ContiguousSize;

    if (ContiguousSize > 0)
    {
        if (ContiguousBase == 0)
        {
            while (device->contiguousSize > 0)
            {
                /* Allocate contiguous memory. */
                status = _AllocateMemory(
                    device,
                    device->contiguousSize,
                    &device->contiguousLogical,
                    &device->contiguousPhysical,
                    &physAddr
                    );

                if (gcmIS_SUCCESS(status))
                {
                    status = gckVIDMEM_Construct(
                        device->os,
                        physAddr | device->systemMemoryBaseAddress,
                        device->contiguousSize,
                        64,
                        BankSize,
                        &device->contiguousVidMem
                        );

                    if (gcmIS_SUCCESS(status))
                    {
                        gckALLOCATOR allocator = ((PVX_MDL)device->contiguousPhysical)->allocator;
                        device->contiguousVidMem->capability = allocator->capability | gcvALLOC_FLAG_MEMLIMIT;
                        device->contiguousVidMem->physical = device->contiguousPhysical;
                        device->contiguousBase = physAddr;
                        break;
                    }

                    gcmkONERROR(_FreeMemory(
                        device,
                        device->contiguousLogical,
                        device->contiguousPhysical
                        ));

                    device->contiguousLogical  = gcvNULL;
                    device->contiguousPhysical = gcvNULL;
                }

                if (device->contiguousSize <= (4 << 20))
                {
                    device->contiguousSize = 0;
                }
                else
                {
                    device->contiguousSize -= (4 << 20);
                }
            }
        }
        else
        {
            /* Create the contiguous memory heap. */
            status = gckVIDMEM_Construct(
                device->os,
                ContiguousBase | device->systemMemoryBaseAddress,
                ContiguousSize,
                64, BankSize,
                &device->contiguousVidMem
                );

            if (gcmIS_ERROR(status))
            {
                /* Error, disable contiguous memory pool. */
                device->contiguousVidMem = gcvNULL;
                device->contiguousSize   = 0;
            }
            else
            {
                gckALLOCATOR allocator;

                gcmkONERROR(gckOS_RequestReservedMemory(
                    device->os, ContiguousBase, ContiguousSize,
                    "gcContMem",
                    Args->contiguousRequested,
                    &device->contiguousPhysical
                    ));

                allocator = ((PVX_MDL)device->contiguousPhysical)->allocator;
                device->contiguousVidMem->capability = allocator->capability | gcvALLOC_FLAG_MEMLIMIT;
                device->contiguousVidMem->physical = device->contiguousPhysical;
                device->requestedContiguousBase = ContiguousBase;
                device->requestedContiguousSize = ContiguousSize;

                device->contiguousPhysName = 0;
                device->contiguousSize = ContiguousSize;
            }
        }
    }

    return gcvSTATUS_OK;
OnError:
    return status;
}

void
_SetupRegisterPhysical(
    IN gckGALDEVICE Device,
    IN gcsDEVICE_CONSTRUCT_ARGS * Args
    )
{
    gctINT *irqs = Args->irqs;
    gctUINT *registerBases = Args->registerBases;
    gctUINT *registerSizes = Args->registerSizes;

    gctINT i = 0;

    for (i = 0; i < gcvCORE_COUNT; i++)
    {
        if (irqs[i] != -1)
        {
            Device->requestedRegisterMemBases[i] = registerBases[i];
            Device->requestedRegisterMemSizes[i] = registerSizes[i];

            gcmkTRACE_ZONE(gcvLEVEL_INFO, _GC_OBJ_ZONE,
                           "Get register base %llx of core %d",
                           registerBases[i], i);
        }
    }
}

/******************************************************************************\
******************************* Interrupt Handler ******************************
\******************************************************************************/
static void isrRoutine(void *ctxt)
{
    gceSTATUS status;
    gckGALDEVICE device;
    gceCORE core = (gceCORE)gcmPTR2INT32(ctxt);

    device = galDevice;

    /* Call kernel interrupt notification. */
    status = gckHARDWARE_Interrupt(device->kernels[core]->hardware);

    if (gcmIS_SUCCESS(status))
    {
        semGive(device->semas[core]);
    }
}

static int threadRoutine(void *ctxt)
{
    gckGALDEVICE device = galDevice;
    gceCORE core = (gceCORE) gcmPTR2INT32(ctxt);
    gctUINT i;

    gcmkTRACE_ZONE(gcvLEVEL_INFO, gcvZONE_DRIVER,
                   "Starting isr Thread with extension=%p",
                   device);

    if (core != gcvCORE_VG)
    {
        /* Make kernel update page table of this thread to include entry related to command buffer.*/
        for (i = 0; i < gcdCOMMAND_QUEUES; i++)
        {
            gctUINT32 data = *(gctUINT32_PTR)device->kernels[core]->command->queues[i].logical;

            data = 0;
        }
    }

    for (;;)
    {
        semTake(device->semas[core], -1);

        if (device->killThread == gcvTRUE)
        {
            gckOS_Delay(device->os, 1);
            return 0;
        }

        gckKERNEL_Notify(device->kernels[core], gcvNOTIFY_INTERRUPT);
    }
}

static void isrRoutineVG(int irq, void *ctxt)
{
}

/******************************************************************************\
******************************* gckGALDEVICE Code ******************************
\******************************************************************************/

static gceSTATUS
_StartThread(
    IN int (*ThreadRoutine)(void *data),
    IN gceCORE Core
    )
{
    gckGALDEVICE device = galDevice;
    int taskId = 0;

    if (device->kernels[Core] != gcvNULL)
    {

        taskId = taskSpawn("galcore", 102, 8, 0x200000,
                           (FUNCPTR )ThreadRoutine, (void *)Core,
                           0, 0, 0, 0, 0, 0, 0, 0, 0);

        if(taskId != 0)
        {
            printf("galcoreid=0x%x\n",taskId);
        }
        else
        {
            return gcvSTATUS_FALSE;
        }

        device->threadCtxts[Core]         = taskId;
        device->threadInitializeds[Core]  = gcvTRUE;

    }
    else
    {
        device->threadInitializeds[Core]  = gcvFALSE;
        return gcvSTATUS_FALSE;
    }

    return gcvSTATUS_OK;
}

/*******************************************************************************
**
**  gckGALDEVICE_Construct
**
**  Constructor.
**
**  INPUT:
**
**  OUTPUT:
**
**      gckGALDEVICE * Device
**          Pointer to a variable receiving the gckGALDEVICE object pointer on
**          success.
*/
gceSTATUS
gckGALDEVICE_Construct(
    IN gctINT IrqLine,
    IN gctUINT32 RegisterMemBase,
    IN gctSIZE_T RegisterMemSize,
    IN gctINT IrqLine2D,
    IN gctUINT32 RegisterMemBase2D,
    IN gctSIZE_T RegisterMemSize2D,
    IN gctINT IrqLineVG,
    IN gctUINT32 RegisterMemBaseVG,
    IN gctSIZE_T RegisterMemSizeVG,
    IN gctUINT32 ContiguousBase,
    IN gctSIZE_T ContiguousSize,
    IN gctUINT32 ExternalBase,
    IN gctSIZE_T ExternalSize,
    IN gctSIZE_T BankSize,
    IN gctINT FastClear,
    IN gctINT Compression,
    IN gctUINT32 PhysBaseAddr,
    IN gctUINT32 PhysSize,
    IN gctINT Signal,
    IN gctUINT LogFileSize,
    IN gctINT PowerManagement,
    IN gctINT GpuProfiler,
    IN gcsDEVICE_CONSTRUCT_ARGS * Args,
    OUT gckGALDEVICE *Device
    )
{
    gctUINT32 internalBaseAddress = 0, internalAlignment = 0;
    gctUINT32 externalAlignment = 0;
    gctUINT32 physical;
    gckGALDEVICE device;
    gceSTATUS status;
    gctINT32 i;
    gceHARDWARE_TYPE type;
    gckKERNEL kernel = gcvNULL;

    gcmkHEADER_ARG("IrqLine=%d RegisterMemBase=0x%08x RegisterMemSize=%u "
                   "IrqLine2D=%d RegisterMemBase2D=0x%08x RegisterMemSize2D=%u "
                   "IrqLineVG=%d RegisterMemBaseVG=0x%08x RegisterMemSizeVG=%u "
                   "ContiguousBase=0x%08x ContiguousSize=%lu BankSize=%lu "
                   "FastClear=%d Compression=%d PhysBaseAddr=0x%x PhysSize=%d Signal=%d",
                   IrqLine, RegisterMemBase, RegisterMemSize,
                   IrqLine2D, RegisterMemBase2D, RegisterMemSize2D,
                   IrqLineVG, RegisterMemBaseVG, RegisterMemSizeVG,
                   ContiguousBase, ContiguousSize, BankSize, FastClear, Compression,
                   PhysBaseAddr, PhysSize, Signal);

#if !gcdENABLE_3D
    IrqLine = -1;
#endif

    IrqLine2D = -1;
    /* Allocate device structure. */
    device = (gckGALDEVICE) malloc(sizeof(struct _gckGALDEVICE));
    if (!device)
    {
        gcmkONERROR(gcvSTATUS_OUT_OF_MEMORY);
    }

    memset(device, 0, sizeof(struct _gckGALDEVICE));

    device->platform = Args->platform;

    device->args = *Args;

    /* set up the contiguous memory */
    device->contiguousSize = ContiguousSize;

    /* Clear irq lines. */
    for (i = 0; i < gcvCORE_COUNT; i++)
    {
        device->irqLines[i] = -1;
    }

    _SetupRegisterPhysical(device, Args);

    if (IrqLine != -1)
    {
        device->requestedRegisterMemBases[gcvCORE_MAJOR] = RegisterMemBase;
        device->requestedRegisterMemSizes[gcvCORE_MAJOR] = RegisterMemSize;
    }

    if (IrqLine2D != -1)
    {
        device->requestedRegisterMemBases[gcvCORE_2D] = RegisterMemBase2D;
        device->requestedRegisterMemSizes[gcvCORE_2D] = RegisterMemSize2D;
    }

    if (IrqLineVG != -1)
    {
        device->requestedRegisterMemBases[gcvCORE_VG] = RegisterMemBaseVG;
        device->requestedRegisterMemSizes[gcvCORE_VG] = RegisterMemSizeVG;
    }
#if gcdDEC_ENABLE_AHB
    {
        device->requestedRegisterMemBases[gcvCORE_DEC] = Args->registerMemBaseDEC300;
        device->requestedRegisterMemSizes[gcvCORE_DEC] = Args->registerMemSizeDEC300;
    }
#endif


    for (i = gcvCORE_MAJOR; i < gcvCORE_COUNT; i++)
    {
        if (Args->irqs[i] != -1)
        {
            device->requestedRegisterMemBases[i] = Args->registerBases[i];
            device->requestedRegisterMemSizes[i] = Args->registerSizes[i];

            gcmkTRACE_ZONE(gcvLEVEL_INFO, gcvZONE_DEVICE,
                           "%s(%d): Core = %d, RegiseterBase = %x",
                           __FUNCTION__, __LINE__,
                           i, Args->registerBases[i]
                           );
        }
    }

    /* Initialize the ISR. */
    device->irqLines[gcvCORE_MAJOR] = IrqLine;
    device->irqLines[gcvCORE_2D] = IrqLine2D;
    device->irqLines[gcvCORE_VG] = IrqLineVG;


    for (i = gcvCORE_MAJOR; i < gcdMAX_GPU_COUNT; i++)
    {
        if (Args->irqs[i] != -1)
        {
            device->irqLines[i] = Args->irqs[i];
        }
    }

    device->requestedContiguousBase  = 0;
    device->requestedContiguousSize  = 0;

    for (i = 0; i < gcdMAX_GPU_COUNT; i++)
    {
        physical = device->requestedRegisterMemBases[i];

        /* Set up register memory region. */
        if (physical != 0)
        {
            if (Args->registerMemMapped && device->irqLines[i] != -1)
            {
                device->registerBases[i] = Args->registerMemAddress;
                device->requestedRegisterMemBases[i] = 0;
            }
            else
            {
                /* Should map register region */
                gcmkONERROR(gcvSTATUS_INVALID_ARGUMENT);
            }

            physical += device->requestedRegisterMemSizes[i];
        }
    }

    /* Set the base address */
    device->baseAddress = device->physBase = PhysBaseAddr;
    device->physSize = PhysSize;

    /* Construct the gckOS object. */
    gcmkONERROR(gckOS_Construct(device, &device->os));

    /* Construct the gckOS object. */
    /* Construct the gckDEVICE object for os independent core management. */
    gcmkONERROR(gckDEVICE_Construct(device->os, &device->device));

    /* Construct the gckOS object. */

    if (device->irqLines[gcvCORE_MAJOR] != -1)
    {
        gcmkONERROR(gctaOS_ConstructOS(device->os, &device->taos));
    }

    /* Construct the gckOS object. */
    gcmkONERROR(_SetupVidMem(device, ContiguousBase, ContiguousSize, BankSize, Args));

    /* Set external base and size */
    device->externalBase = ExternalBase;
    device->externalSize = ExternalSize;

    if (device->irqLines[gcvCORE_MAJOR] != -1)
    {
        gcmkONERROR(gcTA_Construct(device->taos, gcvCORE_MAJOR, &globalTA[gcvCORE_MAJOR]));

        gcmkONERROR(gckDEVICE_AddCore(device->device, gcvCORE_MAJOR, Args->chipIDs[gcvCORE_MAJOR], device, &device->kernels[gcvCORE_MAJOR]));

        gcmkONERROR(gckHARDWARE_SetFastClear(
            device->kernels[gcvCORE_MAJOR]->hardware, FastClear, Compression
            ));

        gcmkONERROR(gckHARDWARE_EnablePowerManagement(
            device->kernels[gcvCORE_MAJOR]->hardware, PowerManagement
            ));

#if gcdENABLE_FSCALE_VAL_ADJUST
        gcmkONERROR(gckHARDWARE_SetMinFscaleValue(
            device->kernels[gcvCORE_MAJOR]->hardware, Args->gpu3DMinClock
            ));
#endif

        gcmkONERROR(gckHARDWARE_SetGpuProfiler(
            device->kernels[gcvCORE_MAJOR]->hardware, GpuProfiler
            ));
    }
    else
    {
        device->kernels[gcvCORE_MAJOR] = gcvNULL;
    }

    if (device->irqLines[gcvCORE_2D] != -1)
    {
        gcmkONERROR(gckDEVICE_AddCore(device->device, gcvCORE_2D, gcvCHIP_ID_DEFAULT, device, &device->kernels[gcvCORE_2D]));

        /* Verify the hardware type */
        gcmkONERROR(gckHARDWARE_GetType(device->kernels[gcvCORE_2D]->hardware, &type));

        if (type != gcvHARDWARE_2D)
        {
            gcmkTRACE_ZONE(
                gcvLEVEL_ERROR, gcvZONE_DRIVER,
                "%s(%d): Unexpected hardware type: %d\n",
                __FUNCTION__, __LINE__,
                type
                );

            gcmkONERROR(gcvSTATUS_INVALID_ARGUMENT);
        }

        gcmkONERROR(gckHARDWARE_EnablePowerManagement(
            device->kernels[gcvCORE_2D]->hardware, PowerManagement
            ));

#if gcdENABLE_FSCALE_VAL_ADJUST
        gcmkONERROR(gckHARDWARE_SetMinFscaleValue(
            device->kernels[gcvCORE_2D]->hardware, 1
            ));
#endif
    }
    else
    {
        device->kernels[gcvCORE_2D] = gcvNULL;
    }

    if (device->irqLines[gcvCORE_VG] != -1)
    {
    }
    else
    {
        device->kernels[gcvCORE_VG] = gcvNULL;
    }

    /* Add core for multiple core. */
    for (i = gcvCORE_3D1; i <= gcvCORE_3D_MAX; i++)
    {
        if (Args->irqs[i] != -1)
        {
            gcmkONERROR(gcTA_Construct(device->taos, (gceCORE)i, &globalTA[i]));
            gckDEVICE_AddCore(device->device, i, Args->chipIDs[i], device, &device->kernels[i]);

            gcmkONERROR(
            gckHARDWARE_SetFastClear(device->kernels[i]->hardware,
                 FastClear,
                Compression));

            gcmkONERROR(gckHARDWARE_EnablePowerManagement(
                device->kernels[i]->hardware, PowerManagement
                ));

            gcmkONERROR(gckHARDWARE_SetGpuProfiler(
                device->kernels[i]->hardware, GpuProfiler
                ));
        }
    }

    /* Initialize the kernel thread semaphores. */
    for (i = 0; i < gcdMAX_GPU_COUNT; i++)
    {
        if (device->irqLines[i] != -1)
        {
            device->semas[i] = semCCreate(SEM_Q_FIFO, 0);
        }
    }

    device->signal = Signal;

    for (i = 0; i < gcdMAX_GPU_COUNT; i++)
    {
        if (device->kernels[i] != gcvNULL) break;
    }

    if (i == gcdMAX_GPU_COUNT)
    {
        gcmkONERROR(gcvSTATUS_INVALID_ARGUMENT);
    }

    {
        /* Query the ceiling of the system memory. */
        gcmkONERROR(gckHARDWARE_QuerySystemMemory(
                device->kernels[i]->hardware,
                &device->systemMemorySize,
                &device->systemMemoryBaseAddress
                ));
    }

    /* Grab the first availiable kernel */
    for (i = 0; i < gcdMAX_GPU_COUNT; i++)
    {
        if (device->irqLines[i] != -1)
        {
            kernel = device->kernels[i];
            break;
        }
    }

    /* Set up the internal memory region. */
    if (device->internalSize > 0)
    {
        status = gckVIDMEM_Construct(
            device->os,
            internalBaseAddress, device->internalSize, internalAlignment,
            0, &device->internalVidMem
            );

        if (gcmIS_ERROR(status))
        {
            /* Error, disable internal heap. */
            device->internalSize = 0;
        }
        else
        {
            /* Map internal memory. */
            device->internalLogical = (gctPOINTER) PHYS_TO_KM(physical);

            if (device->internalLogical == gcvNULL)
            {
                gcmkONERROR(gcvSTATUS_OUT_OF_RESOURCES);
            }

            device->internalPhysical = (gctPHYS_ADDR)(gctUINTPTR_T) physical;
            physical += device->internalSize;
        }
    }

    if (device->externalSize > 0)
    {
        /* create the external memory heap */
        status = gckVIDMEM_Construct(
            device->os,
            device->externalBase, device->externalSize, externalAlignment,
            0, &device->externalVidMem
            );

        if (gcmIS_ERROR(status))
        {
            /* Error, disable external heap. */
            device->externalSize = 0;
        }
        else
        {
            /* Map external memory. */
            gcmkONERROR(gckOS_RequestReservedMemory(
                    device->os,
                    device->externalBase, device->externalSize,
                    "gcExtMem",
                    gcvTRUE,
                    &device->externalPhysical
                    ));
            device->externalVidMem->physical = device->externalPhysical;
        }
    }

    if (device->internalPhysical)
    {
        device->internalPhysName = gcmPTR_TO_NAME(device->internalPhysical);
    }

    if (device->externalPhysical)
    {
        device->externalPhysName = gcmPTR_TO_NAME(device->externalPhysical);
    }

    if (device->contiguousPhysical)
    {
        device->contiguousPhysName = gcmPTR_TO_NAME(device->contiguousPhysical);
    }

    /* Return pointer to the device. */
    *Device = galDevice = device;

    gcmkFOOTER_ARG("*Device=0x%x", * Device);
    return gcvSTATUS_OK;

OnError:
    /* Roll back. */
    gcmkVERIFY_OK(gckGALDEVICE_Destroy(device));

    gcmkFOOTER();
    return status;
}

/*******************************************************************************
**
**  gckGALDEVICE_Destroy
**
**  Class destructor.
**
**  INPUT:
**
**      Nothing.
**
**  OUTPUT:
**
**      Nothing.
**
**  RETURNS:
**
**      Nothing.
*/
gceSTATUS
gckGALDEVICE_Destroy(
    gckGALDEVICE Device)
{
    gctINT i;
    gckKERNEL kernel = gcvNULL;

    gcmkHEADER_ARG("Device=0x%x", Device);

    if (Device != gcvNULL)
    {
        /* Grab the first availiable kernel */
        for (i = 0; i < gcdMAX_GPU_COUNT; i++)
        {
            if (Device->irqLines[i] != -1)
            {
                kernel = Device->kernels[i];
                break;
            }
        }

        for (i = 0; i < gcdMAX_GPU_COUNT; i++)
        {
            if (Device->irqLines[i] != -1)
            {
                semDelete(Device->semas[i]);
            }
        }

        if (Device->internalPhysName != 0)
        {
            gcmRELEASE_NAME(Device->internalPhysName);
            Device->internalPhysName = 0;
        }
        if (Device->externalPhysName != 0)
        {
            gcmRELEASE_NAME(Device->externalPhysName);
            Device->externalPhysName = 0;
        }
        if (Device->contiguousPhysName != 0)
        {
            gcmRELEASE_NAME(Device->contiguousPhysName);
            Device->contiguousPhysName = 0;
        }

        for (i = 0; i < gcdMAX_GPU_COUNT; i++)
        {
            if (Device->kernels[i] != gcvNULL)
            {
                Device->kernels[i] = gcvNULL;
            }
        }

        if (Device->internalLogical != gcvNULL)
        {
            /* Unmap the internal memory. */
            Device->internalLogical = gcvNULL;
        }

        if (Device->internalVidMem != gcvNULL)
        {
            /* Destroy the internal heap. */
            gcmkVERIFY_OK(gckVIDMEM_Destroy(Device->internalVidMem));
            Device->internalVidMem = gcvNULL;
        }

        if (Device->externalPhysical != gcvNULL)
        {
            gckOS_ReleaseReservedMemory(
                Device->os,
                Device->externalPhysical
                );
            Device->externalPhysical = gcvNULL;
        }

        if (Device->externalLogical != gcvNULL)
        {
            Device->externalLogical = gcvNULL;
        }

        if (Device->externalVidMem != gcvNULL)
        {
            /* destroy the external heap */
            gcmkVERIFY_OK(gckVIDMEM_Destroy(Device->externalVidMem));
            Device->externalVidMem = gcvNULL;
        }

        if (Device->contiguousPhysical != gcvNULL)
        {
            if (Device->requestedContiguousBase == 0)
            {
                gcmkVERIFY_OK(_FreeMemory(
                    Device,
                    Device->contiguousLogical,
                    Device->contiguousPhysical
                    ));
            }
            else
            {
                gckOS_ReleaseReservedMemory(
                    Device->os,
                    Device->contiguousPhysical
                    );
                Device->contiguousPhysical = gcvNULL;
                Device->requestedContiguousBase = 0;
                Device->requestedContiguousSize = 0;
            }

            Device->contiguousLogical  = gcvNULL;
            Device->contiguousPhysical = gcvNULL;
        }

        if (Device->contiguousVidMem != gcvNULL)
        {
            /* Destroy the contiguous heap. */
            gcmkVERIFY_OK(gckVIDMEM_Destroy(Device->contiguousVidMem));
            Device->contiguousVidMem = gcvNULL;
        }

        if (Device->device)
        {
            gcmkVERIFY_OK(gckDEVICE_DestroyCores(Device->device));
        }

        for (i = 0; i < gcdMAX_GPU_COUNT; i++)
        {
            if (Device->registerBases[i])
            {
                Device->registerBases[i] = gcvNULL;
                Device->requestedRegisterMemBases[i] = 0;
                Device->requestedRegisterMemSizes[i] = 0;
            }
        }

        if (Device->device)
        {
            gcmkVERIFY_OK(gckDEVICE_Destroy(Device->os, Device->device));

            for (i = 0; i < gcdMAX_GPU_COUNT; i++)
            {
                if (globalTA[i])
                {
                    gcTA_Destroy(globalTA[i]);
                    globalTA[i] = gcvNULL;
                }
            }

            Device->device = gcvNULL;
        }

        if (Device->taos)
        {
            gcmkVERIFY_OK(gctaOS_DestroyOS(Device->taos));
            Device->taos = gcvNULL;
        }

        /* Destroy the gckOS object. */
        if (Device->os != gcvNULL)
        {
            gcmkVERIFY_OK(gckOS_Destroy(Device->os));
            Device->os = gcvNULL;
        }

        /* Free the device. */
        free(Device);
    }

    gcmkFOOTER_NO();
    return gcvSTATUS_OK;
}

/*******************************************************************************
**
**  gckGALDEVICE_Setup_ISR
**
**  Start the ISR routine.
**
**  INPUT:
**
**      gckGALDEVICE Device
**          Pointer to an gckGALDEVICE object.
**
**  OUTPUT:
**
**      Nothing.
**
**  RETURNS:
**
**      gcvSTATUS_OK
**          Setup successfully.
**      gcvSTATUS_GENERIC_IO
**          Setup failed.
*/
gceSTATUS
gckGALDEVICE_Setup_ISR(
    IN gceCORE Core
    )
{
    gceSTATUS status;
    gctINT ret = 0;
    gckGALDEVICE Device = galDevice;

    gcmkHEADER_ARG("Device=0x%x Core=%d", Device, Core);

    gcmkVERIFY_ARGUMENT(Device != NULL);

    if (Device->irqLines[Core] < 0)
    {
        gcmkONERROR(gcvSTATUS_GENERIC_IO);
    }

#if defined(__GNUC__) && ((__GNUC__ == 4 && __GNUC_MINOR__ >= 6) || (__GNUC__ > 4))
    {
        _Static_assert(gcvCORE_COUNT == gcmCOUNTOF(isrNames),
                       "Core count is lager than isrNames size");
    }
#endif

    /* Hook up the isr based on the irq line. */
    ret= intConnect((void *)(Device->irqLines[Core]), isrRoutine, (gctPOINTER)Core);

    (*((volatile unsigned int *)0xbfd0005c) ) = (*((volatile unsigned int *)0xbfd0005c) ) | (1 << 6);

    if (ret != 0)
    {
        gcmkTRACE_ZONE(
            gcvLEVEL_ERROR, gcvZONE_DRIVER,
            "%s(%d): Could not register irq line %d (error=%d)\n",
            __FUNCTION__, __LINE__,
            Device->irqLines[Core], ret
            );

        gcmkONERROR(gcvSTATUS_GENERIC_IO);
    }

    /* Mark ISR as initialized. */
    Device->isrInitializeds[Core] = gcvTRUE;

    gcmkFOOTER_NO();
    return gcvSTATUS_OK;

OnError:
    gcmkFOOTER();
    return status;
}

gceSTATUS
gckGALDEVICE_Setup_ISR_VG(
    IN gckGALDEVICE Device
    )
{
    gceSTATUS status;
    gctINT ret;

    gcmkHEADER_ARG("Device=0x%x", Device);

    gcmkVERIFY_ARGUMENT(Device != NULL);

    if (Device->irqLines[gcvCORE_VG] < 0)
    {
        gcmkONERROR(gcvSTATUS_GENERIC_IO);
    }

    /* Hook up the isr based on the irq line. */
    ret= intConnect((void *)(Device->irqLines[gcvCORE_VG]), isrRoutine, (gctPOINTER)gcvCORE_VG);

    if (ret != 0)
    {
        gcmkTRACE_ZONE(
            gcvLEVEL_ERROR, gcvZONE_DRIVER,
            "%s(%d): Could not register irq line %d (error=%d)\n",
            __FUNCTION__, __LINE__,
            Device->irqLines[gcvCORE_VG], ret
            );

        gcmkONERROR(gcvSTATUS_GENERIC_IO);
    }

    /* Mark ISR as initialized. */
    Device->isrInitializeds[gcvCORE_VG] = gcvTRUE;

    gcmkFOOTER_NO();
    return gcvSTATUS_OK;

OnError:
    gcmkFOOTER();
    return status;
}

/*******************************************************************************
**
**  gckGALDEVICE_Release_ISR
**
**  Release the irq line.
**
**  INPUT:
**
**      gckGALDEVICE Device
**          Pointer to an gckGALDEVICE object.
**
**  OUTPUT:
**
**      Nothing.
**
**  RETURNS:
**
**      Nothing.
*/
gceSTATUS
gckGALDEVICE_Release_ISR(
    IN gceCORE Core
    )
{
    gckGALDEVICE Device = galDevice;
    gcmkHEADER_ARG("Device=0x%x", Device);

    gcmkVERIFY_ARGUMENT(Device != NULL);

    /* release the irq */
    if (Device->isrInitializeds[Core])
    {
        intDisconnect((void *)(Device->irqLines[Core]), isrRoutine, (gctPOINTER)Core);

        Device->isrInitializeds[Core] = gcvFALSE;
    }

    gcmkFOOTER_NO();
    return gcvSTATUS_OK;
}

gceSTATUS
gckGALDEVICE_Release_ISR_VG(
    IN gckGALDEVICE Device
    )
{
    gcmkHEADER_ARG("Device=0x%x", Device);

    gcmkVERIFY_ARGUMENT(Device != NULL);

    /* release the irq */
    if (Device->isrInitializeds[gcvCORE_VG])
    {
        intDisconnect((void *)(Device->irqLines[gcvCORE_VG]), isrRoutineVG, (gctPOINTER)gcvCORE_VG);

        Device->isrInitializeds[gcvCORE_VG] = gcvFALSE;
    }

    gcmkFOOTER_NO();
    return gcvSTATUS_OK;
}

/*******************************************************************************
**
**  gckGALDEVICE_Start_Threads
**
**  Start the daemon threads.
**
**  INPUT:
**
**      gckGALDEVICE Device
**          Pointer to an gckGALDEVICE object.
**
**  OUTPUT:
**
**      Nothing.
**
**  RETURNS:
**
**      gcvSTATUS_OK
**          Start successfully.
**      gcvSTATUS_GENERIC_IO
**          Start failed.
*/
gceSTATUS
gckGALDEVICE_Start_Threads(
    IN gckGALDEVICE Device
    )
{
    gceSTATUS status;
    gctUINT i;

    gcmkHEADER_ARG("Device=0x%x", Device);

    gcmkVERIFY_ARGUMENT(Device != NULL);

    gcmkONERROR(_StartThread(threadRoutine, gcvCORE_MAJOR));
    gcmkONERROR(_StartThread(threadRoutine, gcvCORE_2D));

    gcmkONERROR(_StartThread(threadRoutine, gcvCORE_VG));

    for (i = gcvCORE_3D1; i <= gcvCORE_3D_MAX; i++)
    {
        gcmkONERROR(_StartThread(threadRoutine, i));
    }

    gcmkFOOTER_NO();
    return gcvSTATUS_OK;

OnError:
    gcmkFOOTER();
    return status;
}

/*******************************************************************************
**
**  gckGALDEVICE_Stop_Threads
**
**  Stop the gal device, including the following actions: stop the daemon
**  thread, release the irq.
**
**  INPUT:
**
**      gckGALDEVICE Device
**          Pointer to an gckGALDEVICE object.
**
**  OUTPUT:
**
**      Nothing.
**
**  RETURNS:
**
**      Nothing.
*/
gceSTATUS
gckGALDEVICE_Stop_Threads(
    gckGALDEVICE Device
    )
{
    gctINT i;

    gcmkHEADER_ARG("Device=0x%x", Device);

    gcmkVERIFY_ARGUMENT(Device != NULL);

    for (i = 0; i < gcdMAX_GPU_COUNT; i++)
    {
        /* Stop the kernel threads. */
        if (Device->threadInitializeds[i])
        {
            Device->killThread = gcvTRUE;
            semFlush(Device->semas[i]);

            taskSuspend(Device->threadCtxts[i]);
            Device->threadCtxts[i]        = 0;
            Device->threadInitializeds[i] = gcvFALSE;
        }
    }

    gcmkFOOTER_NO();
    return gcvSTATUS_OK;
}

gceSTATUS
gckGALDEVICE_QueryFrequency(
    IN gckGALDEVICE Device
    )
{
    gctUINT64 mcStart[gcvCORE_COUNT], shStart[gcvCORE_COUNT];
    gctUINT32 mcClk[gcvCORE_COUNT], shClk[gcvCORE_COUNT];
    gckHARDWARE hardware = gcvNULL;
    gceSTATUS status;
    gctUINT i;

    gcmkHEADER_ARG("Device=0x%p", Device);

    for (i = gcvCORE_MAJOR; i < gcvCORE_COUNT; i++)
    {

        if (Device->kernels[i])
        {
            hardware = Device->kernels[i]->hardware;

            mcStart[i] = shStart[i] = 0;

            gckHARDWARE_EnterQueryClock(hardware,
                                        &mcStart[i], &shStart[i]);
        }
    }

    gcmkONERROR(gckOS_Delay(Device->os, 50));

    for (i = gcvCORE_MAJOR; i < gcvCORE_COUNT; i++)
    {
        mcClk[i] = shClk[i] = 0;


        if (Device->kernels[i] && mcStart[i])
        {
            hardware = Device->kernels[i]->hardware;

            gckHARDWARE_ExitQueryClock(hardware,
                                       mcStart[i], shStart[i],
                                       &mcClk[i], &shClk[i]);

            hardware->mcClk = mcClk[i];
            hardware->shClk = shClk[i];
        }
    }

OnError:
    gcmkFOOTER_NO();

    return status;
}

/*******************************************************************************
**
**  gckGALDEVICE_Start
**
**  Start the gal device, including the following actions: setup the isr routine
**  and start the daemoni thread.
**
**  INPUT:
**
**      gckGALDEVICE Device
**          Pointer to an gckGALDEVICE object.
**
**  OUTPUT:
**
**      Nothing.
**
**  RETURNS:
**
**      gcvSTATUS_OK
**          Start successfully.
*/
gceSTATUS
gckGALDEVICE_Start(
    IN gckGALDEVICE Device
    )
{
    gceSTATUS status;
    gctUINT i;

    gcmkHEADER_ARG("Device=0x%x", Device);

    /* Start the kernel thread. */
    gcmkONERROR(gckGALDEVICE_Start_Threads(Device));

    gcmkONERROR(gckGALDEVICE_QueryFrequency(Device));

    for (i = 0; i < gcvCORE_COUNT; i++)
    {
        if (i == gcvCORE_VG)
        {
            continue;
        }

        if (Device->kernels[i] != gcvNULL)
        {
            /* Setup the ISR routine. */
            gcmkONERROR(gckGALDEVICE_Setup_ISR(i));

            /* Switch to SUSPEND power state. */
            gcmkONERROR(gckHARDWARE_SetPowerState(
                Device->kernels[i]->hardware, gcvPOWER_OFF_BROADCAST
                ));
        }
    }

    if (Device->kernels[gcvCORE_VG] != gcvNULL)
    {
        /* Setup the ISR routine. */
        gcmkONERROR(gckGALDEVICE_Setup_ISR_VG(Device));

    }

    gcmkFOOTER_NO();
    return gcvSTATUS_OK;

OnError:
    gcmkFOOTER();
    return status;
}

/*******************************************************************************
**
**  gckGALDEVICE_Stop
**
**  Stop the gal device, including the following actions: stop the daemon
**  thread, release the irq.
**
**  INPUT:
**
**      gckGALDEVICE Device
**          Pointer to an gckGALDEVICE object.
**
**  OUTPUT:
**
**      Nothing.
**
**  RETURNS:
**
**      Nothing.
*/
gceSTATUS
gckGALDEVICE_Stop(
    gckGALDEVICE Device
    )
{
    gceSTATUS status;
    gctUINT i;

    gcmkHEADER_ARG("Device=0x%x", Device);

    gcmkVERIFY_ARGUMENT(Device != NULL);

    for (i = 0; i < gcvCORE_COUNT; i++)
    {
        if (i == gcvCORE_VG)
        {
            continue;
        }

        if (Device->kernels[i] != gcvNULL)
        {
            gcmkONERROR(gckHARDWARE_EnablePowerManagement(
                Device->kernels[i]->hardware, gcvTRUE
                ));

            /* Switch to OFF power state. */
            gcmkONERROR(gckHARDWARE_SetPowerState(
                Device->kernels[i]->hardware, gcvPOWER_OFF
                ));

            /* Remove the ISR routine. */
            gcmkONERROR(gckGALDEVICE_Release_ISR(i));
        }
    }

    if (Device->kernels[gcvCORE_VG] != gcvNULL)
    {
        /* Setup the ISR routine. */
        gcmkONERROR(gckGALDEVICE_Release_ISR_VG(Device));

    }

    /* Stop the kernel thread. */
    gcmkONERROR(gckGALDEVICE_Stop_Threads(Device));

    gcmkFOOTER_NO();
    return gcvSTATUS_OK;

OnError:
    gcmkFOOTER();
    return status;
}

/*******************************************************************************
**
**  gckGALDEVICE_AddCore
**
**  Add a core after gckGALDevice is constructed.
**
**  INPUT:
**
**  OUTPUT:
**
*/
gceSTATUS
gckGALDEVICE_AddCore(
    IN gckGALDEVICE Device,
    IN gcsDEVICE_CONSTRUCT_ARGS * Args
    )
{
    gceSTATUS status;
    gceCORE core = gcvCORE_COUNT;
    gctUINT i = 0;

    gcmkHEADER();
    gcmkVERIFY_ARGUMENT(Device != gcvNULL);

    /* Find which core is added. */
    for (i = 0; i < gcvCORE_COUNT; i++)
    {
        if (Args->irqs[i] != -1)
        {
            core = i;
            break;
        }
    }

    if (i == gcvCORE_COUNT)
    {
        gcmkPRINT("[galcore]: No valid core information found");
        gcmkONERROR(gcvSTATUS_INVALID_ARGUMENT);
    }


    gcmkPRINT("[galcore]: add core[%d]", core);

    /* Record irq, registerBase, registerSize. */
    Device->irqLines[core] = Args->irqs[core];
    _SetupRegisterPhysical(Device, Args);

    /* Map register memory.*/

    /* Add a platform indepedent framework. */
    gcmkONERROR(gckDEVICE_AddCore(
        Device->device,
        core,
        Args->chipIDs[core],
        Device,
        &Device->kernels[core]
        ));

    /* Start thread routine. */
    _StartThread(threadRoutine, core);

    /* Register ISR. */
    gckGALDEVICE_Setup_ISR(core);

    /* Set default power management state. */
    gcmkONERROR(gckHARDWARE_SetPowerState(
        Device->kernels[core]->hardware, gcvPOWER_OFF_BROADCAST
        ));

    gcmkFOOTER_NO();
    return gcvSTATUS_OK;

OnError:
    gcmkFOOTER();
    return gcvSTATUS_OK;
}


