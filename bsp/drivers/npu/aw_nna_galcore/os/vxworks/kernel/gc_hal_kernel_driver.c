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
#include "gc_hal_driver.h"

/* Zone used for header/footer. */
#define _GC_OBJ_ZONE    gcvZONE_DRIVER

static gcsPLATFORM *platform;

gckGALDEVICE galDevice;

static int irqLine = -1;

static unsigned long registerMemBase = 0;

static unsigned long registerMemSize = 1 << 12;

static int irqLine2D = -1;

static unsigned long registerMemBase2D = 0x00000000;

static unsigned long registerMemSize2D = 2 << 10;

static int irqLineVG = -1;

static unsigned long registerMemBaseVG = 0x00000000;

static unsigned long registerMemSizeVG = 2 << 10;

#if gcdDEC_ENABLE_AHB
static unsigned long registerMemBaseDEC300 = 0x00000000;

static unsigned long registerMemSizeDEC300 = 2 << 10;
#endif

#ifndef gcdDEFAULT_CONTIGUOUS_SIZE
#define gcdDEFAULT_CONTIGUOUS_SIZE (4 << 20)
#endif
static unsigned long contiguousSize = 0;

static unsigned long contiguousBase = 0;

static unsigned long externalSize = 0;

static unsigned long externalBase = 0;

static int fastClear = -1;

static int compression = -1;

static int powerManagement = 1;

static int gpuProfiler = 0;

static unsigned long baseAddress = 0;

static unsigned long physSize = 0;

static unsigned int logFileSize = 0;

static unsigned int recovery = 1;

/* Middle needs about 40KB buffer, Maximal may need more than 200KB buffer. */
static unsigned int stuckDump = 0;

static int showArgs = 0;

static int mmu = 1;

static int irqs[gcvCORE_COUNT] = {[0 ... gcvCORE_COUNT - 1] = -1};

static unsigned int registerBases[gcvCORE_COUNT];

static unsigned int registerSizes[gcvCORE_COUNT] = {[0 ... gcvCORE_COUNT - 1] = 2 << 10};

static unsigned int chipIDs[gcvCORE_COUNT] = {[0 ... gcvCORE_COUNT - 1] = gcvCHIP_ID_DEFAULT};

static int gpu3DMinClock = 1;

static int contiguousRequested = 0;

static gctBOOL registerMemMapped = gcvFALSE;
static gctPOINTER registerMemAddress = gcvNULL;

static unsigned long bankSize = 0;
static int gSignal = 48;

void
_UpdateModuleParam(
    gcsMODULE_PARAMETERS *Param
    )
{
    irqLine           = Param->irqLine ;
    registerMemBase   = Param->registerMemBase;
    registerMemSize   = Param->registerMemSize;
    irqLine2D         = Param->irqLine2D      ;
    registerMemBase2D = Param->registerMemBase2D;
    registerMemSize2D = Param->registerMemSize2D;
    contiguousSize    = Param->contiguousSize;
    contiguousBase    = Param->contiguousBase;
    externalSize      = Param->externalSize;
    externalBase      = Param->externalBase;
    bankSize          = Param->bankSize;
    fastClear         = Param->fastClear;
    compression       = (gctINT)Param->compression;
    powerManagement   = Param->powerManagement;
    gpuProfiler       = Param->gpuProfiler;
    gSignal            = Param->gSignal;
    baseAddress       = Param->baseAddress;
    physSize          = Param->physSize;
    logFileSize       = Param->logFileSize;
    recovery          = Param->recovery;
    stuckDump         = Param->stuckDump;
    showArgs          = Param->showArgs;
    contiguousRequested = Param->contiguousRequested;
    gpu3DMinClock     = Param->gpu3DMinClock;
    registerMemMapped    = Param->registerMemMapped;
    registerMemAddress    = Param->registerMemAddress;

    memcpy(irqs, Param->irqs, gcmSIZEOF(gctINT) * gcvCORE_COUNT);
    memcpy(registerBases, Param->registerBases, gcmSIZEOF(gctUINT) * gcvCORE_COUNT);
    memcpy(registerSizes, Param->registerSizes, gcmSIZEOF(gctUINT) * gcvCORE_COUNT);
    memcpy(chipIDs, Param->chipIDs, gcmSIZEOF(gctUINT) * gcvCORE_COUNT);
}

void
gckOS_DumpParam(
    void
    )
{
    gctINT i;

    gcmkPRINT("Galcore options:\n");
    if (irqLine != -1)
    {
        gcmkPRINT("  irqLine           = %d\n",      irqLine);
        gcmkPRINT("  registerMemBase   = 0x%08lX\n", registerMemBase);
        gcmkPRINT("  registerMemSize   = 0x%08lX\n", registerMemSize);
    }

    if (irqLine2D != -1)
    {
        gcmkPRINT("  irqLine2D         = %d\n",      irqLine2D);
        gcmkPRINT("  registerMemBase2D = 0x%08lX\n", registerMemBase2D);
        gcmkPRINT("  registerMemSize2D = 0x%08lX\n", registerMemSize2D);
    }

    if (irqLineVG != -1)
    {
        gcmkPRINT("  irqLineVG         = %d\n",      irqLineVG);
        gcmkPRINT("  registerMemBaseVG = 0x%08lX\n", registerMemBaseVG);
        gcmkPRINT("  registerMemSizeVG = 0x%08lX\n", registerMemSizeVG);
    }

#if gcdDEC_ENABLE_AHB
    gcmkPRINT("  registerMemBaseDEC300 = 0x%08lX\n", registerMemBaseDEC300);
    gcmkPRINT("  registerMemSizeDEC300 = 0x%08lX\n", registerMemSizeDEC300);
#endif

    gcmkPRINT("  contiguousSize    = 0x%08lX\n", contiguousSize);
    gcmkPRINT("  contiguousBase    = 0x%08lX\n", contiguousBase);
    gcmkPRINT("  externalSize      = 0x%08lX\n", externalSize);
    gcmkPRINT("  externalBase      = 0x%08lX\n", externalBase);
    gcmkPRINT("  bankSize          = 0x%08lX\n", bankSize);
    gcmkPRINT("  fastClear         = %d\n",      fastClear);
    gcmkPRINT("  compression       = %d\n",      compression);
    gcmkPRINT("  gSignal           = %d\n",      gSignal);
    gcmkPRINT("  powerManagement   = %d\n",      powerManagement);
    gcmkPRINT("  baseAddress       = 0x%08lX\n", baseAddress);
    gcmkPRINT("  physSize          = 0x%08lX\n", physSize);
    gcmkPRINT("  logFileSize       = %d KB \n",  logFileSize);
    gcmkPRINT("  recovery          = %d\n",      recovery);
    gcmkPRINT("  stuckDump         = %d\n",      stuckDump);
    gcmkPRINT("  gpuProfiler       = %d\n",      gpuProfiler);

    gcmkPRINT("  irqs              = ");
    for (i = 0; i < gcvCORE_COUNT; i++)
    {
        gcmkPRINT("%d, ", irqs[i]);
    }
    gcmkPRINT("\n");

    gcmkPRINT("  registerBases     = ");
    for (i = 0; i < gcvCORE_COUNT; i++)
    {
        gcmkPRINT("0x%08X, ", registerBases[i]);
    }
    gcmkPRINT("\n");

    gcmkPRINT("  registerSizes     = ");
    for (i = 0; i < gcvCORE_COUNT; i++)
    {
        gcmkPRINT("0x%08X, ", registerSizes[i]);
    }
    gcmkPRINT("\n");

    gcmkPRINT("  chipIDs     = ");
    for (i = 0; i < gcvCORE_COUNT; i++)
    {
        gcmkPRINT("0x%08X, ", chipIDs[i]);
    }
    gcmkPRINT("\n");

    gcmkPRINT("Build options:\n");
    gcmkPRINT("  gcdGPU_TIMEOUT    = %d\n", gcdGPU_TIMEOUT);
    gcmkPRINT("  gcdGPU_2D_TIMEOUT = %d\n", gcdGPU_2D_TIMEOUT);
    gcmkPRINT("  gcdINTERRUPT_STATISTIC = %d\n", gcdINTERRUPT_STATISTIC);
}

static int drv_init(void)
{
    int result = -EINVAL;
    gceSTATUS status;
    gckGALDEVICE device = gcvNULL;

    gcsDEVICE_CONSTRUCT_ARGS args = {
        .recovery           = recovery,
        .stuckDump          = stuckDump,
        .gpu3DMinClock      = gpu3DMinClock,
        .contiguousRequested = contiguousRequested,
        .platform           = platform,
        .mmu                = mmu,
        .registerMemMapped = registerMemMapped,
        .registerMemAddress = registerMemAddress,
#if gcdDEC_ENABLE_AHB
        .registerMemBaseDEC300 = registerMemBaseDEC300,
        .registerMemSizeDEC300 = registerMemSizeDEC300,
#endif
    };

    gcmkHEADER();

    memcpy(args.irqs, irqs, gcmSIZEOF(gctINT) * gcvCORE_COUNT);
    memcpy(args.registerBases, registerBases, gcmSIZEOF(gctUINT) * gcvCORE_COUNT);
    memcpy(args.registerSizes, registerSizes, gcmSIZEOF(gctUINT) * gcvCORE_COUNT);
    memcpy(args.chipIDs, chipIDs, gcmSIZEOF(gctUINT) * gcvCORE_COUNT);

    gcmkPRINT("Galcore version %s\n", gcvVERSION_STRING);

    args.powerManagement = powerManagement;
    args.gpuProfiler = gpuProfiler;

    if (showArgs)
    {
        gckOS_DumpParam();
    }

    /* Create the GAL device. */
    status = gckGALDEVICE_Construct(
        irqLine,
        registerMemBase, registerMemSize,
        irqLine2D,
        registerMemBase2D, registerMemSize2D,
        irqLineVG,
        registerMemBaseVG, registerMemSizeVG,
        contiguousBase, contiguousSize,
        externalBase, externalSize,
        bankSize, fastClear, compression, baseAddress, physSize, gSignal,
        logFileSize,
        powerManagement,
        gpuProfiler,
        &args,
        &device
    );

    if (gcmIS_ERROR(status))
    {
        gcmkTRACE_ZONE(gcvLEVEL_ERROR, gcvZONE_DRIVER,
                       "%s(%d): Failed to create the GAL device: status=%d\n",
                       __FUNCTION__, __LINE__, status);

        goto OnError;
    }

    /* Start the GAL device. */
    gcmkONERROR(gckGALDEVICE_Start(device));

    if ((physSize != 0)
       && (device->kernels[gcvCORE_MAJOR] != gcvNULL)
       && (device->kernels[gcvCORE_MAJOR]->hardware->mmuVersion != 0))
    {
        /* Reset the base address */
        device->baseAddress = 0;
    }

    /* Set global galDevice pointer. */
    galDevice = device;

    gcmkTRACE_ZONE(
        gcvLEVEL_INFO, gcvZONE_DRIVER,
        "%s(%d): irqLine=%d, contiguousSize=%lu, memBase=0x%lX\n",
        __FUNCTION__, __LINE__,
        irqLine, contiguousSize, registerMemBase
        );

    /* Success. */
    gcmkFOOTER_NO();
    return 0;

OnError:
    if (device != gcvNULL)
    {
        gcmkVERIFY_OK(gckGALDEVICE_Stop(device));
        gcmkVERIFY_OK(gckGALDEVICE_Destroy(device));
    }

    gcmkFOOTER();
    return result;
}

static void drv_exit(void)
{
    gcmkHEADER();

    gcmkVERIFY_OK(gckGALDEVICE_Stop(galDevice));
    gcmkVERIFY_OK(gckGALDEVICE_Destroy(galDevice));

    gcmkFOOTER_NO();
}

DEV_HDR *devHdr = gcvNULL;

GPU_DEV *gpu_drv_open(
    DEV_HDR *pDevHdr,
    char *name,int flags,int mode
    )
{
    gceSTATUS status;
    gctBOOL attached = gcvFALSE;
    gctINT i;
    GPU_DEV *pGpuDev;

    pGpuDev = (GPU_DEV *) malloc(sizeof(GPU_DEV));

    if (pGpuDev == gcvNULL)
    {
        gcmkTRACE_ZONE(
            gcvLEVEL_ERROR, gcvZONE_DRIVER,
            "%s(%d): private_data is NULL\n",
            __FUNCTION__, __LINE__
            );

        gcmkONERROR(gcvSTATUS_OUT_OF_MEMORY);
    }

    pGpuDev->pDevHdr = *pDevHdr;
    pGpuDev->device  = galDevice;
    pGpuDev->pidOpen = _GetProcessID();

    /* Attached the process. */
    for (i = 0; i < gcdMAX_GPU_COUNT; i++)
    {
        if (galDevice->kernels[i] != gcvNULL)
        {
            gcmkONERROR(gckKERNEL_AttachProcess(galDevice->kernels[i], gcvTRUE));
        }
    }

    attached = gcvTRUE;

    devHdr = pDevHdr;

    /* Success. */
    return pGpuDev;

OnError:
    if (pGpuDev != gcvNULL)
    {
        free(pGpuDev);
    }

    if (attached)
    {
        for (i = 0; i < gcdMAX_GPU_COUNT; i++)
        {
            if (galDevice->kernels[i] != gcvNULL)
            {
                gcmkVERIFY_OK(gckKERNEL_AttachProcess(galDevice->kernels[i], gcvFALSE));
            }
        }
    }

    return gcvNULL;
}

static long gpu_drv_ioctl(
    int fd,
    unsigned int ioctlCode,
    unsigned long arg
    )
{
    gceSTATUS status;
    gcsHAL_INTERFACE iface;
    DRIVER_ARGS drvArgs;
    gckGALDEVICE device;

    device = galDevice;

    if (device == gcvNULL)
    {
        printf("%s(%d): device is NULL\n",
            __FUNCTION__, __LINE__
            );

        gcmkONERROR(gcvSTATUS_INVALID_ARGUMENT);
    }

    if ((ioctlCode != IOCTL_GCHAL_INTERFACE)
    &&  (ioctlCode != IOCTL_GCHAL_KERNEL_INTERFACE)
    )
    {
        printf("%s(%d): unknown command %d\n",
                __FUNCTION__, __LINE__,
                ioctlCode
                );

        gcmkONERROR(gcvSTATUS_INVALID_ARGUMENT);
    }

    /* Get the drvArgs. */
    memcpy(&drvArgs, (void *) arg, sizeof(DRIVER_ARGS));

    /* Now bring in the gcsHAL_INTERFACE structure. */
    if ((drvArgs.InputBufferSize  != sizeof(gcsHAL_INTERFACE))
    ||  (drvArgs.OutputBufferSize != sizeof(gcsHAL_INTERFACE))
    )
    {
        printf("%s(%d): input or/and output structures are invalid.\n",
            __FUNCTION__, __LINE__
            );

        gcmkONERROR(gcvSTATUS_INVALID_ARGUMENT);
    }

    memcpy(&iface, gcmUINT64_TO_PTR(drvArgs.InputBuffer), sizeof(gcsHAL_INTERFACE));

    gcmkONERROR(gckDEVICE_Dispatch(device->device, &iface));

    memcpy(gcmUINT64_TO_PTR(drvArgs.OutputBuffer), &iface, sizeof(gcsHAL_INTERFACE));

    /* Success. */
    return 0;

OnError:
    return -ENOTTY;
}

static int gpu_drv_read(GPU_DEV *pGpuDev, char  *buf, unsigned int nbytes)
{
    return 0;
}

static int gpu_drv_write(GPU_DEV *pGpuDev, char  *buf, unsigned int nbytes)
{
    return 0;
}

static int gpu_drv_delete(GPU_DEV *pGpuDev)
{

   return 0;


}

static int gpu_drv_close(
    GPU_DEV *pGpuDev
    )
{
    gceSTATUS status;
    gckGALDEVICE device;
    gctINT i;

    device = pGpuDev->device;

    if (device == gcvNULL)
    {
        gcmkTRACE_ZONE(
            gcvLEVEL_ERROR, gcvZONE_DRIVER,
            "%s(%d): device is NULL\n",
            __FUNCTION__, __LINE__
            );

        gcmkONERROR(gcvSTATUS_INVALID_ARGUMENT);
    }

    /* A process gets detached. */
    for (i = 0; i < gcdMAX_GPU_COUNT; i++)
    {
        if (galDevice->kernels[i] != gcvNULL)
        {
            gcmkONERROR(gckKERNEL_AttachProcessEx(galDevice->kernels[i], gcvFALSE, pGpuDev->pidOpen));
        }
    }

    free(pGpuDev);

    /* Success. */
    return 0;

OnError:
    return -ENOTTY;
}

static int gpudrvInitNum = -1;

void gpu_drv_install(void)
{
    gpudrvInitNum = iosDrvInstall(
        gpu_drv_open,
        gpu_drv_delete,
        gpu_drv_open,
        gpu_drv_close,
        gpu_drv_read,
        gpu_drv_write,
        gpu_drv_ioctl
    );
}

int gpu_dev_create(void)
{
    char *devName = "/dev/galcore";
    GPU_DEV *pGpuDev;

    pGpuDev = (GPU_DEV *) malloc(sizeof(GPU_DEV));

    memset(pGpuDev, 0, sizeof(GPU_DEV));

    if(iosDevAdd(&pGpuDev->pDevHdr, devName, gpudrvInitNum) == -1)
    {
        free(pGpuDev);
        return -1;
    }

    devHdr = &pGpuDev->pDevHdr;

    return 0;
}

static int gpu_probe(void)
{
    int ret;

    gcsMODULE_PARAMETERS moduleParam = {
        .irqLine            = irqLine,
        .registerMemBase    = registerMemBase,
        .registerMemSize    = registerMemSize,
        .irqLine2D          = irqLine2D,
        .registerMemBase2D  = registerMemBase2D,
        .registerMemSize2D  = registerMemSize2D,
        .irqLineVG          = irqLineVG,
        .registerMemBaseVG  = registerMemBaseVG,
        .registerMemSizeVG  = registerMemSizeVG,
        .contiguousSize     = contiguousSize,
        .contiguousBase     = contiguousBase,
        .bankSize           = bankSize,
        .fastClear          = fastClear,
        .compression        = compression,
        .powerManagement    = powerManagement,
        .gpuProfiler        = gpuProfiler,
        .gSignal            = gSignal,
        .baseAddress        = baseAddress,
        .physSize           = physSize,
        .logFileSize        = logFileSize,
        .recovery           = recovery,
        .stuckDump          = stuckDump,
        .showArgs           = showArgs,
        .gpu3DMinClock      = gpu3DMinClock,
        .registerMemMapped    = registerMemMapped,
    };

    memcpy(moduleParam.irqs, irqs, gcmSIZEOF(gctINT) * gcvCORE_COUNT);
    memcpy(moduleParam.registerBases, registerBases, gcmSIZEOF(gctUINT) * gcvCORE_COUNT);
    memcpy(moduleParam.registerSizes, registerSizes, gcmSIZEOF(gctUINT) * gcvCORE_COUNT);
    memcpy(moduleParam.chipIDs, chipIDs, gcmSIZEOF(gctUINT) * gcvCORE_COUNT);

    if (platform->ops->getPower)
    {
        platform->ops->getPower(platform);
    }

    if (platform->ops->adjustParam)
    {
        /* Override default module param. */
        platform->ops->adjustParam(platform, &moduleParam);

        /* Update module param because drv_init() uses them directly. */
        _UpdateModuleParam(&moduleParam);
    }

    ret = drv_init();
    if (ret != 0)
    {
        printf("drv init fail!\n");
        return ret;
    }

    gpu_drv_install();

    gpu_dev_create();

    return 0;
}

void gpu_remove(void)
{
    iosDevDelete(devHdr);

    iosDrvRemove(gpudrvInitNum, gcvTRUE);

    drv_exit();

    if (platform->ops->putPower)
    {
        platform->ops->putPower(platform);
    }
}

/* GPU init function call. */
int gpu_init(void)
{
    int ret = 0;

    ret = soc_platform_init(&platform);
    if (ret || !platform)
    {
        printf("galcore: Soc platform init failed.\n");
        return -ENODEV;
    }

    if (gpu_probe() != 0)
    {
        printf("gpu probe fail !\n");
    }
    else
    {
        printf("gpu probe success !\n");
    }

    return 0;
}

void gpu_exit(void)
{
    gpu_remove();

    soc_platform_terminate(platform);
    platform = NULL;
}

void dump_gpu_state(void)
{
    gckHARDWARE hardware;

    if (galDevice->kernels[gcvCORE_MAJOR])
    {
        hardware = galDevice->kernels[gcvCORE_MAJOR]->hardware;

        gckHARDWARE_DumpGPUState(hardware);
    }
}
