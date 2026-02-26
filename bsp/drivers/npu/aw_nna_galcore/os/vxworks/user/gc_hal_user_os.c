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

/**
**  @file
**  OS object for hal user layers.
**
*/

#include "gc_hal_user_vxworks.h"
#include "gc_hal_user_os_atomic.h"

#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <pthread.h>

#include "gc_hal_user_platform.h"

#define _GC_OBJ_ZONE    gcvZONE_OS

char const * const GALDeviceName[] =
{
    "/dev/galcore",
    "/dev/graphics/galcore"
};

#define MAX_RETRY_IOCTL_TIMES 10000

#ifndef gcdBUILT_FOR_VALGRIND
#define gcdBUILT_FOR_VALGRIND 0
#endif

#define gcmGETPROCESSID() \
    getpid()

long int syscall(long int number, ...);
#   define gcmGETTHREADID() \
        (gctUINT32) pthread_self()


/******************************************************************************\
***************************** gcoOS Object Structure ***************************
\******************************************************************************/

typedef struct _gcsDRIVER_ARGS
{
    gctUINT64   InputBuffer;
    gctUINT64   InputBufferSize;
    gctUINT64   OutputBuffer;
    gctUINT64   OutputBufferSize;
}
gcsDRIVER_ARGS;

struct _gcoOS
{
    /* Object. */
    gcsOBJECT               object;

    /* Context. */
    gctPOINTER              context;

    /* Heap. */
    gcoHEAP                 heap;

    /* Base address. */
    gctUINT32               baseAddress;

#if VIVANTE_PROFILER_SYSTEM_MEMORY
    gctUINT64               startTick;
    gctUINT32               allocCount;
    gctSIZE_T               allocSize;
    gctSIZE_T               maxAllocSize;
    gctUINT32               freeCount;
    gctSIZE_T               freeSize;

#if gcdGC355_MEM_PRINT
    /* For a single collection. */
    gctINT32                oneSize;
    gctBOOL                 oneRecording;
#endif
#endif

    gcsPLATFORM             platform;

    gctUINT32               tps;

    /* Handle to the device. */
    int                     device;
};

/******************************************************************************\
*********************************** Globals ************************************
\******************************************************************************/

static pthread_key_t gcProcessKey;

gcsPLS gcPLS = gcPLS_INITIALIZER;
static pthread_mutex_t plsMutex = PTHREAD_MUTEX_INITIALIZER;

/******************************************************************************\
****************************** Internal Functions ******************************
\******************************************************************************/
#if gcmIS_DEBUG(gcdDEBUG_TRACE) || gcdGC355_MEM_PRINT
static void _ReportDB(
    void
    )
{
    gceSTATUS status;
    gcsHAL_INTERFACE iface;

    gcoOS_ZeroMemory(&iface, sizeof(iface));

    iface.command = gcvHAL_DATABASE;
    iface.u.Database.processID = (gctUINT32)(gctUINTPTR_T)gcoOS_GetCurrentProcessID();
    iface.u.Database.validProcessID = gcvTRUE;

    /* Call kernel service. */
    gcmONERROR(gcoOS_DeviceControl(
        gcvNULL,
        IOCTL_GCHAL_INTERFACE,
        &iface, gcmSIZEOF(iface),
        &iface, gcmSIZEOF(iface)
        ));

    if ((iface.u.Database.vidMem.counters.bytes     != 0) ||
        (iface.u.Database.nonPaged.counters.bytes   != 0) ||
        (iface.u.Database.contiguous.counters.bytes != 0))
    {
        gcmTRACE(gcvLEVEL_ERROR, "\n");
        gcmTRACE(gcvLEVEL_ERROR, "******* MEMORY LEAKS DETECTED *******\n");
    }

    if (iface.u.Database.vidMem.counters.bytes != 0)
    {
        gcmTRACE(gcvLEVEL_ERROR, "\n");
        gcmTRACE(gcvLEVEL_ERROR, "vidMem.bytes      = %d\n", iface.u.Database.vidMem.counters.bytes);
        gcmTRACE(gcvLEVEL_ERROR, "vidMem.maxBytes   = %d\n", iface.u.Database.vidMem.counters.maxBytes);
        gcmTRACE(gcvLEVEL_ERROR, "vidMem.totalBytes = %d\n", iface.u.Database.vidMem.counters.totalBytes);
    }

    if (iface.u.Database.nonPaged.counters.bytes != 0)
    {
        gcmTRACE(gcvLEVEL_ERROR, "\n");
        gcmTRACE(gcvLEVEL_ERROR, "nonPaged.bytes      = %d\n", iface.u.Database.nonPaged.counters.bytes);
        gcmTRACE(gcvLEVEL_ERROR, "nonPaged.maxBytes   = %d\n", iface.u.Database.nonPaged.counters.maxBytes);
        gcmTRACE(gcvLEVEL_ERROR, "nonPaged.totalBytes = %d\n", iface.u.Database.nonPaged.counters.totalBytes);
    }

    if (iface.u.Database.contiguous.counters.bytes != 0)
    {
        gcmTRACE(gcvLEVEL_ERROR, "\n");
        gcmTRACE(gcvLEVEL_ERROR, "contiguous.bytes      = %d\n", iface.u.Database.contiguous.counters.bytes);
        gcmTRACE(gcvLEVEL_ERROR, "contiguous.maxBytes   = %d\n", iface.u.Database.contiguous.counters.maxBytes);
        gcmTRACE(gcvLEVEL_ERROR, "contiguous.totalBytes = %d\n", iface.u.Database.contiguous.counters.totalBytes);
    }

    gcmPRINT("05) Video memory - current : %lld \n", iface.u.Database.vidMem.counters.bytes);
    gcmPRINT("06) Video memory - maximum : %lld \n", iface.u.Database.vidMem.counters.maxBytes);
    gcmPRINT("07) Video memory - total   : %lld \n", iface.u.Database.vidMem.counters.totalBytes);
    gcmPRINT("08) Video memory - allocation Count: %u \n", iface.u.Database.vidMem.counters.allocCount);
    gcmPRINT("09) Video memory - deallocation Count : %u \n", iface.u.Database.vidMem.counters.freeCount);

OnError:;
}
#endif

#if gcdDUMP || gcdDUMP_API || gcdDUMP_2D || gcdDUMP_2DVG
static void
_SetDumpFileInfo(
    )
{
    gceSTATUS status = gcvSTATUS_TRUE;
    gctBOOL dumpNew2D = gcvFALSE;
#if gcdDUMP_2D
    gcsTLS_PTR tls;
#endif

#if gcdDUMP || gcdDUMP_2D || gcdDUMP_2DVG
    #define DUMP_FILE_PREFIX   "hal"
#else
    #define DUMP_FILE_PREFIX   "api"
#endif

#if gcdDUMP_2D
    tls = (gcsTLS_PTR) pthread_getspecific(gcProcessKey);

    /* Enable new dump 2D by env variable "2D_DUMP_LEVEL". Level=2 dump all. Level=1 dump cmd only. */
    if (status != gcvSTATUS_TRUE)
    {
        gctSTRING dump = gcvNULL;
        gctINT level = 0;

        if (gcmIS_SUCCESS(gcoOS_GetEnv(gcvNULL, "2D_DUMP_LEVEL", &dump)) && dump)
        {
            gcoOS_StrToInt(dump, &level);

            if (level != 0 && level < 4)
            {
                tls->newDump2DFlag = level;
                dumpNew2D = gcvTRUE;
            }
        }
    }
    else
    {
        tls->newDump2DFlag = 3;
    }
#endif

    if (status == gcvSTATUS_TRUE || dumpNew2D)
    {
        char dump_file[128];
        gctUINT offset = 0;

        /* Customize filename as needed. */
        gcmVERIFY_OK(gcoOS_PrintStrSafe(dump_file,
                     gcmSIZEOF(dump_file),
                     &offset,
                     "%s%s_dump_pid-%d_tid-%d_%s.log",
                     gcdDUMP_PATH,
                     DUMP_FILE_PREFIX,
                     gcoOS_GetCurrentProcessID(),
                     gcmGETTHREADID(),
                     dumpNew2D ? "dump2D" : gcdDUMP_KEY));

        gcoOS_SetDebugFile(dump_file);
        gcoDUMP_SetDumpFlag(gcvTRUE);
    }
}
#endif

/******************************************************************************\
**************************** OS Construct/Destroy ******************************
\******************************************************************************/
static gceSTATUS
_DestroyOs(
    IN gcoOS Os
    )
{
    gceSTATUS status;

    gcmPROFILE_DECLARE_ONLY(gctUINT64 ticks);

    gcmHEADER();

    if (gcPLS.os != gcvNULL)
    {

        gcmPROFILE_QUERY(gcPLS.os->startTick, ticks);
        gcmPROFILE_ONLY(gcmTRACE_ZONE(gcvLEVEL_INFO, gcvZONE_OS,
                                      "Total ticks during gcoOS life: %llu",
                                      ticks));

        if (gcPLS.os->heap != gcvNULL)
        {
            gcoHEAP heap = gcPLS.os->heap;

#if VIVANTE_PROFILER_SYSTEM_MEMORY
            /* End profiler. */
            gcoHEAP_ProfileEnd(heap, "gcoOS_HEAP");
#endif

            /* Mark the heap as gone. */
            gcPLS.os->heap = gcvNULL;

            /* Destroy the heap. */
            gcmONERROR(gcoHEAP_Destroy(heap));
        }

        /* Close the handle to the kernel service. */
        if (gcPLS.os->device != -1)
        {
#if gcmIS_DEBUG(gcdDEBUG_TRACE) || gcdGC355_MEM_PRINT
            _ReportDB();
#endif

            close(gcPLS.os->device);
            gcPLS.os->device = -1;
        }

#if VIVANTE_PROFILER_SYSTEM_MEMORY && gcdGC355_MEM_PRINT
        /* End profiler. */
        gcoOS_ProfileEnd(gcPLS.os, gcvNULL);
#endif

        /* Mark the gcoOS object as unknown. */
        gcPLS.os->object.type = gcvOBJ_UNKNOWN;

        /* Free the gcoOS structure. */
        free(gcPLS.os);

        /* Reset PLS object. */
        gcPLS.os = gcvNULL;
    }

    /* Success. */
    gcmFOOTER_KILL();
    return gcvSTATUS_OK;

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}

static gceSTATUS
_ConstructOs(
    IN gctPOINTER Context,
    OUT gcoOS * Os
    )
{
    gcoOS os = gcPLS.os;
    gceSTATUS status;

    gcmPROFILE_DECLARE_ONLY(gctUINT64 freq);

    gcmHEADER_ARG("Context=0x%x", Context);

    if (os == gcvNULL)
    {
        /* Allocate the gcoOS structure. */
        os = malloc(gcmSIZEOF(struct _gcoOS));
        if (os == gcvNULL)
        {
            gcmONERROR(gcvSTATUS_OUT_OF_MEMORY);
        }

        /* Initialize the gcoOS object. */
        os->object.type = gcvOBJ_OS;
        os->context     = Context;
        os->heap        = gcvNULL;
        os->baseAddress = gcvINVALID_ADDRESS;
        os->device      = -1;
        os->tps         = sysClkRateGet();

        /* Set the object pointer to PLS. */
        gcmASSERT(gcPLS.os == gcvNULL);
        gcPLS.os = os;


        /* Construct heap. */
        status = gcoHEAP_Construct(gcvNULL, gcdHEAP_SIZE, &os->heap);

        if (gcmIS_ERROR(status))
        {
            gcmTRACE_ZONE(
                gcvLEVEL_WARNING, gcvZONE_OS,
                "%s(%d): Could not construct gcoHEAP (%d).",
                __FUNCTION__, __LINE__, status
                );

            os->heap = gcvNULL;
        }
#if VIVANTE_PROFILER_SYSTEM_MEMORY
        else
        {
            /* Start profiler. */
            gcoHEAP_ProfileStart(os->heap);
        }
#endif

        /* Get profiler start tick. */
        gcmPROFILE_INIT(freq, os->startTick);

        /* Get platform callback functions. */
        gcoPLATFORM_QueryOperations(&os->platform.ops);
    }

#if VIVANTE_PROFILER_SYSTEM_MEMORY
        /* Start profiler. */
    gcoOS_ProfileStart(os);
#endif

    /* Return pointer to the gcoOS object. */
    if (Os != gcvNULL)
    {
        *Os = os;
    }

    /* Success. */
    gcmFOOTER_ARG("*Os=0x%x", os);
    return gcvSTATUS_OK;

OnError:
    /* Roll back. */
    gcmVERIFY_OK(_DestroyOs(gcvNULL));
    gcmFOOTER();
    return status;
}

/******************************************************************************\
************************* Process/Thread Local Storage *************************
\******************************************************************************/

static void __attribute__((destructor)) _ModuleDestructor(void);
static void __attribute__((constructor)) _ModuleConstructor(void);

static gceSTATUS
_QueryVideoMemory(
    OUT gctUINT32 * InternalPhysName,
    OUT gctSIZE_T * InternalSize,
    OUT gctUINT32 * ExternalPhysName,
    OUT gctSIZE_T * ExternalSize,
    OUT gctUINT32 * ContiguousPhysName,
    OUT gctSIZE_T * ContiguousSize
    )
{
    gceSTATUS status;
    gcsHAL_INTERFACE iface;

    gcmHEADER();

    /* Call kernel HAL to query video memory. */
    iface.ignoreTLS    = gcvTRUE;
    iface.hardwareType = gcPLS.hal->defaultHwType,
    iface.coreIndex    = 0;

    iface.command = gcvHAL_QUERY_VIDEO_MEMORY;

    /* Call kernel service. */
    gcmONERROR(gcoOS_DeviceControl(gcvNULL,
                                   IOCTL_GCHAL_INTERFACE,
                                   &iface, gcmSIZEOF(iface),
                                   &iface, gcmSIZEOF(iface)));

    if (InternalPhysName != gcvNULL)
    {
        /* Verify arguments. */
        gcmDEBUG_VERIFY_ARGUMENT(InternalSize != gcvNULL);

        /* Save internal memory size. */
        *InternalPhysName = iface.u.QueryVideoMemory.internalPhysName;
        *InternalSize    = (gctSIZE_T)iface.u.QueryVideoMemory.internalSize;
    }

    if (ExternalPhysName != gcvNULL)
    {
        /* Verify arguments. */
        gcmDEBUG_VERIFY_ARGUMENT(ExternalSize != gcvNULL);

        /* Save external memory size. */
        *ExternalPhysName = iface.u.QueryVideoMemory.externalPhysName;
        *ExternalSize    = (gctSIZE_T)iface.u.QueryVideoMemory.externalSize;
    }

    if (ContiguousPhysName != gcvNULL)
    {
        /* Verify arguments. */
        gcmDEBUG_VERIFY_ARGUMENT(ContiguousSize != gcvNULL);

        /* Save contiguous memory size. */
        *ContiguousPhysName = iface.u.QueryVideoMemory.contiguousPhysName;
        *ContiguousSize    = (gctSIZE_T)iface.u.QueryVideoMemory.contiguousSize;
    }

    /* Success. */
    gcmFOOTER_ARG("*InternalPhysName=0x%08x *InternalSize=%lu "
                  "*ExternalPhysName=0x%08x *ExternalSize=%lu "
                  "*ContiguousPhysName=0x%08x *ContiguousSize=%lu",
                  gcmOPT_VALUE(InternalPhysName), gcmOPT_VALUE(InternalSize),
                  gcmOPT_VALUE(ExternalPhysName), gcmOPT_VALUE(ExternalSize),
                  gcmOPT_VALUE(ContiguousPhysName),
                  gcmOPT_VALUE(ContiguousSize));
    return gcvSTATUS_OK;

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}

static gceSTATUS
_MapMemory(
    IN gctUINT32 PhysName,
    IN gctSIZE_T NumberOfBytes,
    OUT gctPOINTER * Logical
    )
{
    gceSTATUS status;
    gcsHAL_INTERFACE iface;

    gcmHEADER_ARG("PhysName=0x%x NumberOfBytes=%lu", PhysName, NumberOfBytes);

    /* Verify the arguments. */
    gcmDEBUG_VERIFY_ARGUMENT(NumberOfBytes > 0);
    gcmDEBUG_VERIFY_ARGUMENT(Logical != gcvNULL);

    /* Call kernel API to unmap the memory. */
    iface.ignoreTLS    = gcvTRUE;
    iface.hardwareType = gcPLS.hal->defaultHwType,
    iface.coreIndex    = 0;

    iface.command              = gcvHAL_MAP_MEMORY;
    iface.u.MapMemory.physName = PhysName;
    iface.u.MapMemory.bytes    = NumberOfBytes;

    gcmONERROR(gcoOS_DeviceControl(
        gcvNULL,
        IOCTL_GCHAL_INTERFACE,
        &iface, gcmSIZEOF(iface),
        &iface, gcmSIZEOF(iface)
        ));

    /* Return logical address. */
    *Logical = gcmUINT64_TO_PTR(iface.u.MapMemory.logical);

    /* Success. */
    gcmFOOTER_ARG("*Logical=0x%x", *Logical);
    return gcvSTATUS_OK;

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}

static gceSTATUS
_UnmapMemory(
    IN gctUINT32 PhysName,
    IN gctSIZE_T NumberOfBytes,
    IN gctPOINTER Logical
    )
{
    gceSTATUS status;
    gcsHAL_INTERFACE iface;

    gcmHEADER_ARG("PhysName=0x%x NumberOfBytes=%lu Logical=0x%x",
                  PhysName, NumberOfBytes, Logical);

    /* Verify the arguments. */
    gcmDEBUG_VERIFY_ARGUMENT(NumberOfBytes > 0);
    gcmDEBUG_VERIFY_ARGUMENT(Logical != gcvNULL);

    /* Call kernel API to unmap the memory. */
    iface.ignoreTLS    = gcvTRUE;
    iface.hardwareType = gcPLS.hal ? gcPLS.hal->defaultHwType
                                   : gcvHARDWARE_2D;
    iface.coreIndex    = 0;

    iface.command                = gcvHAL_UNMAP_MEMORY;
    iface.u.UnmapMemory.physName = PhysName;
    iface.u.UnmapMemory.bytes    = NumberOfBytes;
    iface.u.UnmapMemory.logical  = gcmPTR_TO_UINT64(Logical);

    gcmONERROR(gcoOS_DeviceControl(
        gcvNULL,
        IOCTL_GCHAL_INTERFACE,
        &iface, sizeof(iface),
        &iface, sizeof(iface)
        ));

    /* Success. */
    gcmFOOTER_NO();
    return gcvSTATUS_OK;

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}

static void
_PLSDestructor(
    void
    )
{
    gctUINT i = 0;
    gcmHEADER();

#if gcdENABLE_3D
#if gcdSYNC
    if(gcPLS.globalFenceID) gcoOS_AtomDestroy(gcvNULL, gcPLS.globalFenceID);
#endif
#endif

    for (i = 0; i < gcmCOUNTOF(gcPLS.destructor); i++)
    {
        if (gcPLS.destructor[i] != gcvNULL)
        {
            gcPLS.destructor[i](&gcPLS);
            gcPLS.destructor[i] = gcvNULL;
        }
    }

    if (gcPLS.contiguousLogical != gcvNULL)
    {
        gcmVERIFY_OK(_UnmapMemory(
            gcPLS.contiguousPhysName,
            gcPLS.contiguousSize,
            gcPLS.contiguousLogical
            ));

        gcPLS.contiguousLogical = gcvNULL;
    }

    if (gcPLS.externalLogical != gcvNULL)
    {
        gcmVERIFY_OK(_UnmapMemory(
            gcPLS.externalPhysName,
            gcPLS.externalSize,
            gcPLS.externalLogical
            ));

        gcPLS.externalLogical = gcvNULL;
    }

    if (gcPLS.internalLogical != gcvNULL)
    {
        gcmVERIFY_OK(_UnmapMemory(
            gcPLS.internalPhysName,
            gcPLS.internalSize,
            gcPLS.internalLogical
            ));

        gcPLS.internalLogical = gcvNULL;
    }

#if gcdDUMP_2D
    gcmVERIFY_OK(gcoOS_DeleteMutex(gcPLS.os, dumpMemInfoListMutex));
    dumpMemInfoListMutex = gcvNULL;
#endif

    gcmVERIFY_OK(gcoOS_DeleteMutex(gcPLS.os, gcPLS.accessLock));
    gcPLS.accessLock = gcvNULL;

    gcmVERIFY_OK(gcoOS_DeleteMutex(gcPLS.os, gcPLS.glFECompilerAccessLock));
    gcPLS.glFECompilerAccessLock = gcvNULL;

    gcmVERIFY_OK(gcoOS_DeleteMutex(gcPLS.os, gcPLS.clFECompilerAccessLock));
    gcPLS.clFECompilerAccessLock = gcvNULL;

    gcmVERIFY_OK(gcoOS_DeleteMutex(gcPLS.os, gcPLS.vxContextGlobalLock));
    gcPLS.vxContextGlobalLock = gcvNULL;

    gcmVERIFY_OK(gcoOS_AtomDestroy(gcPLS.os, gcPLS.reference));
    gcPLS.reference = gcvNULL;

    if (gcPLS.hal != gcvNULL)
    {
        gcmVERIFY_OK(gcoHAL_DestroyEx(gcPLS.hal));
        gcPLS.hal = gcvNULL;
    }

    gcmVERIFY_OK(_DestroyOs(gcPLS.os));

    pthread_key_delete(gcProcessKey);

    gcmFOOTER_NO();
}

static void
_TLSDestructor(
    gctPOINTER TLS
    )
{
    gcsTLS_PTR tls;
    gctINT reference = 0;
    gctINT i;

    gcmHEADER_ARG("TLS=0x%x", TLS);

    tls = (gcsTLS_PTR) TLS;
    gcmASSERT(tls != gcvNULL);

    pthread_setspecific(gcProcessKey, tls);

    if (tls->copied)
    {
        /* Zero out all information if this TLS was copied. */
        gcoOS_ZeroMemory(tls, gcmSIZEOF(gcsTLS));
    }

    for (i = 0; i < gcvTLS_KEY_COUNT; i++)
    {
        gcsDRIVER_TLS_PTR drvTLS = tls->driverTLS[i];

        if (drvTLS && drvTLS->destructor != gcvNULL)
        {
            drvTLS->destructor(drvTLS);
        }

        tls->driverTLS[i] = gcvNULL;
    }

#if gcdENABLE_3D
    /* DON'T destroy tls->engine3D, which belongs to app context
    */
#endif

#if gcdUSE_VX
    if(tls->engineVX)
    {
        gcmVERIFY_OK(gcoVX_Destroy(tls->engineVX));
    }
#endif

    if (tls->defaultHardware != gcvNULL)
    {
        tls->currentType = gcvHARDWARE_3D;
        tls->currentHardware = tls->defaultHardware;
        tls->targetType = gcvHARDWARE_3D;

        gcmTRACE_ZONE(
            gcvLEVEL_VERBOSE, gcvZONE_HARDWARE,
            "%s(%d): destroying default hardware object 0x%08X.",
            __FUNCTION__, __LINE__, tls->defaultHardware
            );

        gcmVERIFY_OK(gcoHARDWARE_Destroy(tls->defaultHardware, gcvTRUE));

        tls->defaultHardware = gcvNULL;
        tls->currentHardware = gcvNULL;
    }

    if (tls->hardware2D != gcvNULL)
    {
        gceHARDWARE_TYPE type = tls->currentType;
        tls->currentType = gcvHARDWARE_2D;

        gcmTRACE_ZONE(
            gcvLEVEL_VERBOSE, gcvZONE_HARDWARE,
            "%s(%d): destroying hardware object 0x%08X.",
            __FUNCTION__, __LINE__, tls->hardware2D
            );

        tls->hardware2D = gcvNULL;
        tls->currentType = type;
    }


    if (gcPLS.reference != gcvNULL)
    {
        /* Decrement the reference. */
        gcmVERIFY_OK(gcoOS_AtomDecrement(gcPLS.os,
                                         gcPLS.reference,
                                         &reference));

        /* Check if there are still more references. */
        if (reference ==  1)
        {
            /* If all threads exit, destruct PLS */
            _PLSDestructor();
        }
    }

    gcmVERIFY_OK(gcoOS_FreeMemory(gcvNULL, tls));

    pthread_setspecific(gcProcessKey, gcvNULL);

    gcmFOOTER_NO();
}

static void
_InitializeProcess(
    void
    )
{
    /* Install thread destructor. */
    pthread_key_create(&gcProcessKey, _TLSDestructor);
}

static gceSTATUS
_ModuleConstructor(
    void
    )
{
    gceSTATUS status = gcvSTATUS_OK;
    int result;
    static pthread_once_t onceControl = PTHREAD_ONCE_INIT;

    gcmHEADER();

    if (gcPLS.processID)
    {
        gcmFOOTER_NO();
        return gcvSTATUS_OK;
    }

    /* Each process gets its own objects. */
    gcmASSERT(gcPLS.os  == gcvNULL);
    gcmASSERT(gcPLS.hal == gcvNULL);

    gcmASSERT(gcPLS.internalLogical   == gcvNULL);
    gcmASSERT(gcPLS.externalLogical   == gcvNULL);
    gcmASSERT(gcPLS.contiguousLogical == gcvNULL);

    /* Call _InitializeProcess function only one time for the process. */
    result = pthread_once(&onceControl, _InitializeProcess);

    if (result != 0)
    {
        gcmTRACE(
            gcvLEVEL_ERROR,
            "%s(%d): pthread_key_create returned %d",
            __FUNCTION__, __LINE__, result
            );

        gcmONERROR(gcvSTATUS_OUT_OF_MEMORY);
    }

    /* Construct OS object. */
    gcmONERROR(_ConstructOs(gcvNULL, gcvNULL));

    /* Construct PLS reference atom. */
    gcmONERROR(gcoOS_AtomConstruct(gcPLS.os, &gcPLS.reference));

    /* Increment PLS reference for main thread. */
    gcmONERROR(gcoOS_AtomIncrement(gcPLS.os, gcPLS.reference, gcvNULL));

    /* Record the process and thread that calling this constructor function */
    gcPLS.processID = gcmGETPROCESSID();
    gcPLS.threadID = gcmGETTHREADID();

    /* Construct access lock */
    gcmONERROR(gcoOS_CreateMutex(gcPLS.os, &gcPLS.accessLock));

    /* Construct gl FE compiler access lock */
    gcmONERROR(gcoOS_CreateMutex(gcPLS.os, &gcPLS.glFECompilerAccessLock));

    /* Construct cl FE compiler access lock */
    gcmONERROR(gcoOS_CreateMutex(gcPLS.os, &gcPLS.clFECompilerAccessLock));

    /* Construct vx context access lock */
    gcmONERROR(gcoOS_CreateMutex(gcPLS.os, &gcPLS.vxContextGlobalLock));

#if gcdDUMP_2D
    gcmONERROR(gcoOS_CreateMutex(gcPLS.os, &dumpMemInfoListMutex));
#endif

    gcmFOOTER_ARG(
        "gcPLS.os=0x%08X, gcPLS.hal=0x%08X"
        " internal=0x%08X external=0x%08X contiguous=0x%08X",
        gcPLS.os, gcPLS.hal,
        gcPLS.internalLogical, gcPLS.externalLogical, gcPLS.contiguousLogical
        );

    return status;

OnError:
    if (gcPLS.accessLock != gcvNULL)
    {
        /* Destroy access lock */
        gcmVERIFY_OK(gcoOS_DeleteMutex(gcPLS.os, gcPLS.accessLock));
        gcPLS.accessLock = gcvNULL;
    }

    if (gcPLS.glFECompilerAccessLock != gcvNULL)
    {
        /* Destroy access lock */
        gcmVERIFY_OK(gcoOS_DeleteMutex(gcPLS.os, gcPLS.glFECompilerAccessLock));
        gcPLS.glFECompilerAccessLock = gcvNULL;
    }

    if (gcPLS.clFECompilerAccessLock != gcvNULL)
    {
        /* Destroy access lock */
        gcmVERIFY_OK(gcoOS_DeleteMutex(gcPLS.os, gcPLS.clFECompilerAccessLock));
        gcPLS.clFECompilerAccessLock = gcvNULL;
    }

    if (gcPLS.vxContextGlobalLock != gcvNULL)
    {
        /* Destroy vx context access lock */
        gcmVERIFY_OK(gcoOS_DeleteMutex(gcPLS.os, gcPLS.vxContextGlobalLock));
        gcPLS.vxContextGlobalLock = gcvNULL;
    }

    if (gcPLS.reference != gcvNULL)
    {
        /* Destroy the reference. */
        gcmVERIFY_OK(gcoOS_AtomDestroy(gcPLS.os, gcPLS.reference));
        gcPLS.reference = gcvNULL;
    }

    gcmFOOTER();
    return status;
}

static gceSTATUS
_OpenDevice(
    IN gcoOS Os
    )
{
    gceSTATUS status = gcvSTATUS_OK;
    gctUINT tryCount;

    gcmHEADER();

    pthread_mutex_lock(&plsMutex);

    if (gcPLS.bDeviceOpen)
    {
        pthread_mutex_unlock(&plsMutex);
        gcmFOOTER_NO();
        return gcvSTATUS_OK;
    }

    for (tryCount = 0; tryCount < 5; tryCount++)
    {
        gctUINT i;

        if (tryCount > 0)
        {
            /* Sleep for a while when second and later tries. */
            gcoOS_Delay(Os, 1000);
            gcmPRINT("Failed to open device: %s, Try again...", strerror(errno));
        }

        /* Attempt to open the device. */
        for (i = 0; i < gcmCOUNTOF(GALDeviceName); i += 1)
        {
            gcmTRACE_ZONE(
                gcvLEVEL_VERBOSE, gcvZONE_OS,
                "%s(%d): Attempting to open device %s.",
                 __FUNCTION__, __LINE__, GALDeviceName[i]
                 );

            Os->device = open(GALDeviceName[i], O_RDWR, 0);

            if (Os->device >= 0)
            {
                gcmTRACE_ZONE(
                    gcvLEVEL_VERBOSE, gcvZONE_OS,
                    "%s(%d): Opened device %s.",
                    __FUNCTION__, __LINE__, GALDeviceName[i]
                    );

                break;
            }
        }

        if (Os->device >= 0)
        {
            /* Device opened. */
            break;
        }
    }

    if (Os->device < 0)
    {
        pthread_mutex_unlock(&plsMutex);

        gcmPRINT(
            "%s(%d): FATAL: Failed to open device, errno=%s.",
            __FUNCTION__, __LINE__, strerror(errno)
            );

        exit(1);

        /* Should not run here. */
        gcmFOOTER();
        return gcvSTATUS_GENERIC_IO;
    }

    /* Construct gcoHAL object. */
    gcmONERROR(gcoHAL_ConstructEx(gcvNULL, gcvNULL, &gcPLS.hal));

    /* Query the video memory sizes. */
    gcmONERROR(_QueryVideoMemory(
        &gcPLS.internalPhysName,
        &gcPLS.internalSize,
        &gcPLS.externalPhysName,
        &gcPLS.externalSize,
        &gcPLS.contiguousPhysName,
        &gcPLS.contiguousSize
        ));

    /* Map internal video memory. */
    if (gcPLS.internalSize != 0)
    {
        gcmONERROR(_MapMemory(
             gcPLS.internalPhysName,
             gcPLS.internalSize,
            &gcPLS.internalLogical
            ));
    }

    /* Map external video memory. */
    if (gcPLS.externalSize != 0)
    {
        gcmONERROR(_MapMemory(
             gcPLS.externalPhysName,
             gcPLS.externalSize,
            &gcPLS.externalLogical
            ));
    }

    /* Map contiguous video memory. */
    if (gcPLS.contiguousSize != 0)
    {
        gcmONERROR(_MapMemory(
             gcPLS.contiguousPhysName,
             gcPLS.contiguousSize,
            &gcPLS.contiguousLogical
            ));
    }

    /* make sure this instruction is at last */
    gcPLS.bDeviceOpen = gcvTRUE;

OnError:
    pthread_mutex_unlock(&plsMutex);

    gcmFOOTER();
    return status;
}

static void
_ModuleDestructor(
    void
    )
{
    gctINT reference = 0;
    gcmHEADER();

    if (gcPLS.reference != gcvNULL)
    {
        gcPLS.exiting = gcvTRUE;

        /* Decrement the reference for main thread. */
        gcmVERIFY_OK(gcoOS_AtomDecrement(gcPLS.os,
                                         gcPLS.reference,
                                         &reference));

        if (reference == 1)
        {
            /* If all threads exit, destruct PLS. */
            _PLSDestructor();
        }
        else
        {
            gcoOS_FreeThreadData();
        }
    }

    gcmFOOTER_NO();
}

/******************************************************************************\
********************************* gcoOS API Code *******************************
\******************************************************************************/

/*******************************************************************************
 **
 ** gcoOS_GetPLSValue
 **
 ** Get value associated with the given key.
 **
 ** INPUT:
 **
 **     gcePLS_VALUE key
 **         key to look up.
 **
 ** OUTPUT:
 **
 **     None
 **
 ** RETURN:
 **
 **     gctPOINTER
 **         Pointer to object associated with key.
 */
gctPOINTER
gcoOS_GetPLSValue(
    IN gcePLS_VALUE key
    )
{
    switch (key)
    {
        case gcePLS_VALUE_EGL_DISPLAY_INFO :
            return gcPLS.eglDisplayInfo;

        case gcePLS_VALUE_EGL_CONFIG_FORMAT_INFO :
            return (gctPOINTER) gcPLS.eglConfigFormat;

        case gcePLS_VALUE_EGL_DESTRUCTOR_INFO :
            return (gctPOINTER) gcPLS.destructor[gcvAPI_EGL];

        case gcePLS_VALUE_OPENCL_DESTRUCTOR_INFO:
            return (gctPOINTER)gcPLS.destructor[gcvAPI_OPENCL];
    }

    return gcvNULL;
}

/*******************************************************************************
 **
 ** gcoOS_SetPLSValue
 **
 ** Associated object represented by 'value' with the given key.
 **
 ** INPUT:
 **
 **     gcePLS_VALUE key
 **         key to associate.
 **
 **     gctPOINTER value
 **         value to associate with key.
 **
 ** OUTPUT:
 **
 **     None
 **
 */
void
gcoOS_SetPLSValue(
    IN gcePLS_VALUE key,
    IN gctPOINTER value
    )
{
    switch (key)
    {
        case gcePLS_VALUE_EGL_DISPLAY_INFO :
            gcPLS.eglDisplayInfo = value;
            return;

        case gcePLS_VALUE_EGL_CONFIG_FORMAT_INFO :
            gcPLS.eglConfigFormat = (gceSURF_FORMAT)(gctUINTPTR_T)value;
            return;

        case gcePLS_VALUE_EGL_DESTRUCTOR_INFO :
            gcPLS.destructor[gcvAPI_EGL] = (gctPLS_DESTRUCTOR) value;
            return;

        case gcePLS_VALUE_OPENCL_DESTRUCTOR_INFO:
            gcPLS.destructor[gcvAPI_OPENCL] = (gctPLS_DESTRUCTOR)value;
            return;
    }
}

static gcmINLINE gceSTATUS
_GetTLS(
    OUT gcsTLS_PTR * TLS
    )
{
    gceSTATUS status = gcvSTATUS_OK;
    gcsTLS_PTR tls = gcvNULL;
    int res;

    if (!gcPLS.processID)
    {
        pthread_mutex_lock(&plsMutex);
        status = _ModuleConstructor();
        pthread_mutex_unlock(&plsMutex);

        gcmONERROR(status);
    }

    tls = (gcsTLS_PTR) pthread_getspecific(gcProcessKey);

    if (tls == NULL)
    {
        gcmONERROR(gcoOS_AllocateMemory(
            gcvNULL, gcmSIZEOF(gcsTLS), (gctPOINTER *) &tls
            ));

        gcoOS_ZeroMemory(
            tls, gcmSIZEOF(gcsTLS)
            );

        /* Determine default hardware type later. */
        tls->currentType = gcvHARDWARE_INVALID;
        tls->targetType  = gcvHARDWARE_INVALID;

        res = pthread_setspecific(gcProcessKey, tls);

        if (res != 0)
        {
            gcmTRACE(
                gcvLEVEL_ERROR,
                "%s(%d): pthread_setspecific returned %d",
                __FUNCTION__, __LINE__, res
                );

            gcmONERROR(gcvSTATUS_GENERIC_IO);
        }

        if (gcPLS.reference != gcvNULL)
        {
            /* Increment PLS reference. */
            gcmONERROR(gcoOS_AtomIncrement(gcPLS.os, gcPLS.reference, gcvNULL));
        }

#if gcdDUMP || gcdDUMP_API || gcdDUMP_2D || gcdDUMP_2DVG
        _SetDumpFileInfo();
#endif
    }

    *TLS = tls;

    return gcvSTATUS_OK;

OnError:
    if (tls != gcvNULL)
    {
        gcmVERIFY_OK(gcoOS_FreeMemory(gcvNULL, (gctPOINTER) tls));
    }

    * TLS = gcvNULL;

    return status;
}

/*******************************************************************************
 **
 ** gcoOS_GetTLS
 **
 ** Get access to the thread local storage.
 **
 ** INPUT:
 **
 **     Nothing.
 **
 ** OUTPUT:
 **
 **     gcsTLS_PTR * TLS
 **         Pointer to a variable that will hold the pointer to the TLS.
 */
gceSTATUS
gcoOS_GetTLS(
    OUT gcsTLS_PTR * TLS
    )
{
    gceSTATUS status;
    gcsTLS_PTR tls = gcvNULL;

    gcmONERROR(_GetTLS(&tls));

    if (!gcPLS.bDeviceOpen)
    {
        status = _OpenDevice(gcPLS.os);
        gcmONERROR(status);
    }

    /* Assign default hardware type. */
    if ((tls->currentType == gcvHARDWARE_INVALID) && gcPLS.hal)
    {
        tls->currentType = gcPLS.hal->defaultHwType;
    }

    *TLS = tls;
    return gcvSTATUS_OK;

OnError:
    *TLS = gcvNULL;
    return status;
}

/*
 *  gcoOS_CopyTLS
 *
 *  Copy the TLS from a source thread and mark this thread as a copied thread, so the destructor won't free the resources.
 *
 *  NOTE: Make sure the "source thread" doesn't get kiiled while this thread is running, since the objects will be taken away. This
 *  will be fixed in a future version of the HAL when reference counters will be used to keep track of object usage (automatic
 *  destruction).
 */
gceSTATUS gcoOS_CopyTLS(IN gcsTLS_PTR Source)
{
    gceSTATUS   status;
    gcsTLS_PTR  tls;

    gcmHEADER();

    /* Verify the arguyments. */
    gcmDEBUG_VERIFY_ARGUMENT(Source != gcvNULL);

    /* Get the thread specific data. */
    tls = pthread_getspecific(gcProcessKey);

    if (tls != gcvNULL)
    {
        /* We cannot copy if the TLS has already been initialized. */
        gcmONERROR(gcvSTATUS_INVALID_REQUEST);
    }

    /* Allocate memory for the TLS. */
    gcmONERROR(gcoOS_AllocateMemory(gcvNULL, gcmSIZEOF(gcsTLS), (gctPOINTER *) &tls));

    /* Set the thread specific data. */
    pthread_setspecific(gcProcessKey, tls);

    if (gcPLS.reference != gcvNULL)
    {
        /* Increment PLS reference. */
        gcmONERROR(gcoOS_AtomIncrement(gcPLS.os, gcPLS.reference, gcvNULL));
    }

    /* Copy the TLS information. */
    gcoOS_MemCopy(tls, Source, sizeof(gcsTLS));

    /* Mark this TLS as copied. */
    tls->copied = gcvTRUE;

    tls->currentHardware = gcvNULL;

#if gcdDUMP || gcdDUMP_API || gcdDUMP_2D || gcdDUMP_2DVG
    _SetDumpFileInfo();
#endif

    /* Success. */
    gcmFOOTER_NO();
    return gcvSTATUS_OK;

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}


gceSTATUS
gcoOS_QueryTLS(
    OUT gcsTLS_PTR * TLS
    )
{
    gceSTATUS status = gcvSTATUS_OK;
    gcsTLS_PTR tls;

    gcmHEADER();

    /* Verify the arguyments. */
    gcmVERIFY_ARGUMENT(TLS != gcvNULL);

    /* Get the thread specific data. */
    tls = pthread_getspecific(gcProcessKey);

    /* Return pointer to user. */
    *TLS = tls;

    /* Return the status. */
    gcmFOOTER();
    return status;
}

/* Get access to driver tls. */
gceSTATUS
gcoOS_GetDriverTLS(
    IN gceTLS_KEY Key,
    OUT gcsDRIVER_TLS_PTR * TLS
    )
{
    gceSTATUS status;
    gcsTLS_PTR tls;

    gcmHEADER_ARG("Key=%d", Key);

    if (Key >= gcvTLS_KEY_COUNT)
    {
        gcmONERROR(gcvSTATUS_INVALID_ARGUMENT);
    }

    /* Get generic tls. */
    gcmONERROR(_GetTLS(&tls));

    *TLS = tls->driverTLS[Key];
    gcmFOOTER_NO();
    return gcvSTATUS_OK;

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}

/* Set driver tls. */
gceSTATUS
gcoOS_SetDriverTLS(
    IN gceTLS_KEY Key,
    IN gcsDRIVER_TLS * TLS
    )
{
    gceSTATUS status;
    gcsTLS_PTR tls;

    gcmHEADER_ARG("Key=%d", Key);

    if (Key >= gcvTLS_KEY_COUNT)
    {
        gcmONERROR(gcvSTATUS_INVALID_ARGUMENT);
    }

    /* Get generic tls. */
    gcmONERROR(_GetTLS(&tls));

    tls->driverTLS[Key] = TLS;
    gcmFOOTER_NO();
    return gcvSTATUS_OK;

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}


/*******************************************************************************
**
**  gcoOS_LockPLS
**
**  Lock mutext before access PLS if needed
**
**  INPUT:
**
**      Nothing.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gcoOS_LockPLS(
    void
    )
{
    gceSTATUS status = gcvSTATUS_OK;
    gcmHEADER();
    if (gcPLS.accessLock)
    {
        status = gcoOS_AcquireMutex(gcPLS.os, gcPLS.accessLock, gcvINFINITE);

    }
    gcmFOOTER_ARG("Lock PLS ret=%d", status);

    return status;
}

/*******************************************************************************
**
**  gcoOS_UnLockPLS
**
**  Release mutext after access PLS if needed
**
**  INPUT:
**
**      Nothing.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gcoOS_UnLockPLS(
    void
    )
{
    gceSTATUS status = gcvSTATUS_OK;
    gcmHEADER();
    if (gcPLS.accessLock)
    {
        status = gcoOS_ReleaseMutex(gcPLS.os, gcPLS.accessLock);

    }
    gcmFOOTER_ARG("Release PLS ret=%d", status);

    return status;
}

/*******************************************************************************
**
**  gcoOS_LockGLFECompiler
**
**  Lock mutext before access GL FE compiler if needed
**
**  INPUT:
**
**      Nothing.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gcoOS_LockGLFECompiler(
    void
    )
{
    gceSTATUS status = gcvSTATUS_OK;
    gcmHEADER();
    if (gcPLS.glFECompilerAccessLock)
    {
        status = gcoOS_AcquireMutex(gcPLS.os, gcPLS.glFECompilerAccessLock, gcvINFINITE);
    }
    gcmFOOTER_ARG("Lock GL FE compiler ret=%d", status);

    return status;
}

/*******************************************************************************
**
**  gcoOS_UnLockGLFECompiler
**
**  Release mutext after access GL FE compiler if needed
**
**  INPUT:
**
**      Nothing.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gcoOS_UnLockGLFECompiler(
    void
    )
{
    gceSTATUS status = gcvSTATUS_OK;
    gcmHEADER();
    if (gcPLS.glFECompilerAccessLock)
    {
        status = gcoOS_ReleaseMutex(gcPLS.os, gcPLS.glFECompilerAccessLock);
    }
    gcmFOOTER_ARG("Release GL FE compiler ret=%d", status);

    return status;
}

/*******************************************************************************
**
**  gcoOS_LockCLFECompiler
**
**  Lock mutext before access CL FE compiler if needed
**
**  INPUT:
**
**      Nothing.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gcoOS_LockCLFECompiler(
    void
    )
{
    gceSTATUS status = gcvSTATUS_OK;
    gcmHEADER();
    if (gcPLS.clFECompilerAccessLock)
    {
        status = gcoOS_AcquireMutex(gcPLS.os, gcPLS.clFECompilerAccessLock, gcvINFINITE);
    }
    gcmFOOTER_ARG("Lock CL FE compiler ret=%d", status);

    return status;
}

/*******************************************************************************
**
**  gcoOS_UnLockCLFECompiler
**
**  Release mutext after access CL FE compiler if needed
**
**  INPUT:
**
**      Nothing.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gcoOS_UnLockCLFECompiler(
    void
    )
{
    gceSTATUS status = gcvSTATUS_OK;
    gcmHEADER();
    if (gcPLS.clFECompilerAccessLock)
    {
        status = gcoOS_ReleaseMutex(gcPLS.os, gcPLS.clFECompilerAccessLock);
    }
    gcmFOOTER_ARG("Release CL FE compiler ret=%d", status);

    return status;
}

/*******************************************************************************
**
**  gcoOS_FreeThreadData
**
**  Destroy the objects associated with the current thread.
**
**  INPUT:
**
**      Nothing.
**
**  OUTPUT:
**
**      Nothing.
*/
void
gcoOS_FreeThreadData(
    void
    )
{
    gcsTLS_PTR tls;

    tls = (gcsTLS_PTR) pthread_getspecific(gcProcessKey);

    if (tls != NULL)
    {
        if (gcPLS.processID != (gctUINT32)(gctUINTPTR_T)gcmGETPROCESSID())
        {
            /* This process is not the one called construct function.
             * It maybe created by fork or clone, just return in this case . */
            return;
        }

        _TLSDestructor((gctPOINTER) tls);
    }
}

/*******************************************************************************
 **
 ** gcoOS_Construct
 **
 ** Construct a new gcoOS object. Empty function only for compatibility.
 **
 ** INPUT:
 **
 **     gctPOINTER Context
 **         Pointer to an OS specific context.
 **
 ** OUTPUT:
 **
 **     Nothing.
 **
 */
gceSTATUS
gcoOS_Construct(
    IN gctPOINTER Context,
    OUT gcoOS * Os
    )
{
    gceSTATUS status;
    gcsTLS_PTR tls;

    gcmONERROR(gcoOS_GetTLS(&tls));

    /* Return gcoOS object for compatibility to prevent any failure in applications. */
    *Os = gcPLS.os;

    return gcvSTATUS_OK;

OnError:
    return status;
}

/*******************************************************************************
 **
 ** gcoOS_Destroy
 **
 ** Destroys an gcoOS object. Empty function only for compatibility.
 **
 ** ARGUMENTS:
 **
 **     gcoOS Os
 **         Pointer to the gcoOS object that needs to be destroyed.
 **
 ** OUTPUT:
 **
 **     Nothing.
 **
 */
gceSTATUS
gcoOS_Destroy(
    IN gcoOS Os
    )
{
    return gcvSTATUS_OK;
}

/*******************************************************************************
**
**  gcoOS_GetPhysicalSystemMemorySize
**
**  Get the amount of system memory.
**
**  OUTPUT:
**
**      gctUINT64 * PhysicalSystemMemorySize
**          Pointer to a variable that will hold the size of the physical system
**          memory.
*/
gceSTATUS
gcoOS_GetPhysicalSystemMemorySize(
    OUT gctUINT64 * PhysicalSystemMemorySize
    )
{
    return (gctUINT64)memFindMax();
}

/*******************************************************************************
**
**  gcoOS_QueryVideoMemory
**
**  Query the amount of video memory.
**
**  INPUT:
**
**      gcoOS Os
**          Pointer to a gcoOS object.
**
**  OUTPUT:
**
**      gctUINT32 * InternalPhysName
**          Pointer to a variable that will hold the physical memory name of the
**          internal memory.  If 'InternalPhysName' is gcvNULL, no information
**          about the internal memory will be returned.
**
**      gctSIZE_T * InternalSize
**          Pointer to a variable that will hold the size of the internal
**          memory.  'InternalSize' cannot be gcvNULL if 'InternalPhysName' is
**          not gcvNULL.
**
**      gctUINT32 * ExternalPhysName
**          Pointer to a variable that will hold the physical memory name of the
**          external memory.  If 'ExternalPhysName' is gcvNULL, no information
**          about the external memory will be returned.
**
**      gctSIZE_T * ExternalSize
**          Pointer to a variable that will hold the size of the external
**          memory.  'ExternalSize' cannot be gcvNULL if 'ExternalPhysName' is
**          not gcvNULL.
**
**      gctUINT32 * ContiguousPhysName
**          Pointer to a variable that will hold the physical memory name of the
**          contiguous memory.  If 'ContiguousPhysName' is gcvNULL, no
**          information about the contiguous memory will be returned.
**
**      gctSIZE_T * ContiguousSize
**          Pointer to a variable that will hold the size of the contiguous
**          memory.  'ContiguousSize' cannot be gcvNULL if 'ContiguousPhysName'
**          is not gcvNULL.
*/
gceSTATUS
gcoOS_QueryVideoMemory(
    IN gcoOS Os,
    OUT gctUINT32 * InternalPhysName,
    OUT gctSIZE_T * InternalSize,
    OUT gctUINT32 * ExternalPhysName,
    OUT gctSIZE_T * ExternalSize,
    OUT gctUINT32 * ContiguousPhysName,
    OUT gctSIZE_T * ContiguousSize
    )
{
    gceSTATUS status;
    gcsHAL_INTERFACE iface;

    gcmHEADER();

    /* Call kernel HAL to query video memory. */
    iface.ignoreTLS = gcvFALSE;
    iface.command = gcvHAL_QUERY_VIDEO_MEMORY;

    /* Call kernel service. */
    gcmONERROR(gcoOS_DeviceControl(gcvNULL,
                                   IOCTL_GCHAL_INTERFACE,
                                   &iface, gcmSIZEOF(iface),
                                   &iface, gcmSIZEOF(iface)));

    if (InternalPhysName != gcvNULL)
    {
        /* Verify arguments. */
        gcmDEBUG_VERIFY_ARGUMENT(InternalSize != gcvNULL);

        /* Save internal memory size. */
        *InternalPhysName = iface.u.QueryVideoMemory.internalPhysName;
        *InternalSize    = (gctSIZE_T)iface.u.QueryVideoMemory.internalSize;
    }

    if (ExternalPhysName != gcvNULL)
    {
        /* Verify arguments. */
        gcmDEBUG_VERIFY_ARGUMENT(ExternalSize != gcvNULL);

        /* Save external memory size. */
        *ExternalPhysName = iface.u.QueryVideoMemory.externalPhysName;
        *ExternalSize    = (gctSIZE_T)iface.u.QueryVideoMemory.externalSize;
    }

    if (ContiguousPhysName != gcvNULL)
    {
        /* Verify arguments. */
        gcmDEBUG_VERIFY_ARGUMENT(ContiguousSize != gcvNULL);

        /* Save contiguous memory size. */
        *ContiguousPhysName = iface.u.QueryVideoMemory.contiguousPhysName;
        *ContiguousSize    = (gctSIZE_T)iface.u.QueryVideoMemory.contiguousSize;
    }

    /* Success. */
    gcmFOOTER_ARG("*InternalPhysName=0x%08x *InternalSize=%lu "
                  "*ExternalPhysName=0x%08x *ExternalSize=%lu "
                  "*ContiguousPhysName=0x%08x *ContiguousSize=%lu",
                  gcmOPT_VALUE(InternalPhysName), gcmOPT_VALUE(InternalSize),
                  gcmOPT_VALUE(ExternalPhysName), gcmOPT_VALUE(ExternalSize),
                  gcmOPT_VALUE(ContiguousPhysName),
                  gcmOPT_VALUE(ContiguousSize));
    return gcvSTATUS_OK;

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gcoOS_GetBaseAddress
**
**  Get the base address for the physical memory.
**
**  INPUT:
**
**      gcoOS Os
**          Pointer to the gcoOS object.
**
**  OUTPUT:
**
**      gctUINT32_PTR BaseAddress
**          Pointer to a variable that will receive the base address.
*/
gceSTATUS
gcoOS_GetBaseAddress(
    IN gcoOS Os,
    OUT gctUINT32_PTR BaseAddress
    )
{
    gceSTATUS status;
    gceHARDWARE_TYPE type = gcvHARDWARE_INVALID;
    gcsHAL_INTERFACE iface;

    gcmHEADER();

    /* Verify the arguments. */
    gcmDEBUG_VERIFY_ARGUMENT(BaseAddress != gcvNULL);

    gcmVERIFY_OK(gcoHAL_GetHardwareType(gcvNULL, &type));

    /* Return base address. */
    if (type == gcvHARDWARE_VG)
    {
        *BaseAddress = 0;
    }
    else
    {
        if (gcPLS.os->baseAddress == gcvINVALID_ADDRESS)
        {
            /* Query base address. */
            iface.ignoreTLS = gcvFALSE;
            iface.command = gcvHAL_GET_BASE_ADDRESS;

            /* Call kernel driver. */
            status = gcoOS_DeviceControl(
                gcvNULL,
                IOCTL_GCHAL_INTERFACE,
                &iface, gcmSIZEOF(iface),
                &iface, gcmSIZEOF(iface)
                );

            if (gcmIS_ERROR(status))
            {
                gcmFOOTER();
                return status;
            }

            gcPLS.os->baseAddress = iface.u.GetBaseAddress.baseAddress;
        }

        *BaseAddress = gcPLS.os->baseAddress;
    }

    /* Success. */
    gcmFOOTER_ARG("*BaseAddress=0x%08x", *BaseAddress);
    return gcvSTATUS_OK;
}

/*******************************************************************************
**
**  gcoOS_Allocate
**
**  Allocate memory from the user heap.
**
**  INPUT:
**
**      gcoOS Os
**          Pointer to an gcoOS object.
**
**      gctSIZE_T Bytes
**          Number of bytes to allocate.
**
**  OUTPUT:
**
**      gctPOINTER * Memory
**          Pointer to a variable that will hold the pointer to the memory
**          allocation.
*/
gceSTATUS
gcoOS_Allocate(
    IN gcoOS Os,
    IN gctSIZE_T Bytes,
    OUT gctPOINTER * Memory
    )
{
    gceSTATUS status;

    /*
     * The max of all additions to Bytes in following functions,
     * which is gcmSIZEOF(gcsHEAP) (16 bytes) + gcmSIZEOF(gcsNODE) (16 bytes)
     */
    gctSIZE_T bytesToAdd = 32;

    gcmHEADER_ARG("Bytes=%lu", Bytes);

    /* Verify the arguments. */
    gcmDEBUG_VERIFY_ARGUMENT(Bytes > 0);
    gcmDEBUG_VERIFY_ARGUMENT(Memory != gcvNULL);

    /* Make sure Bytes doesn't exceed MAX_SIZET after additions */
    if (Bytes > gcvMAXSIZE_T - bytesToAdd)
    {
        gcmFOOTER_ARG("%d", gcvSTATUS_DATA_TOO_LARGE);
        return gcvSTATUS_DATA_TOO_LARGE;
    }

    if ((gcPLS.os != gcvNULL) && (gcPLS.os->heap != gcvNULL))
    {
        gcmONERROR(gcoHEAP_Allocate(gcPLS.os->heap, Bytes, Memory));
    }
    else
    {
        gcmONERROR(gcoOS_AllocateMemory(gcPLS.os, Bytes, Memory));
    }

    /* Success. */
    gcmFOOTER_ARG("*Memory=0x%x", *Memory);
    return gcvSTATUS_OK;

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gcoOS_Realloc
**
**  Allocate memory from the user heap.
**
**  INPUT:
**
**      gcoOS Os
**          Pointer to an gcoOS object.
**
**      gctSIZE_T Bytes
**          Number of bytes to allocate.
**
**  OUTPUT:
**
**      gctPOINTER * Memory
**          Pointer to a variable that will hold the pointer to the memory
**          allocation.
*/
gceSTATUS
gcoOS_Realloc(
    IN gcoOS Os,
    IN gctSIZE_T Bytes,
    IN gctSIZE_T OrgBytes,
    OUT gctPOINTER * Memory
    )
{
    gceSTATUS status;

    /*
     * The max of all additions to Bytes in following functions,
     * which is gcmSIZEOF(gcsHEAP) (16 bytes) + gcmSIZEOF(gcsNODE) (16 bytes)
     */
    gctSIZE_T bytesToAdd = 32;

    gcmHEADER_ARG("Bytes=%lu", Bytes);

    /* Verify the arguments. */
    gcmDEBUG_VERIFY_ARGUMENT(Bytes > 0);
    gcmDEBUG_VERIFY_ARGUMENT(Memory != gcvNULL);

    /* Make sure Bytes doesn't exceed MAX_SIZET after additions */
    if (Bytes > gcvMAXSIZE_T - bytesToAdd)
    {
        gcmFOOTER_ARG("%d", gcvSTATUS_DATA_TOO_LARGE);
        return gcvSTATUS_DATA_TOO_LARGE;
    }

    if ((gcPLS.os != gcvNULL) && (gcPLS.os->heap != gcvNULL))
    {
         gcmPRINT("Not support heap realloc now.\n");
         gcmASSERT(0);
    }
    else
    {
        gcmONERROR(gcoOS_ReallocMemory(gcPLS.os, Bytes, OrgBytes, Memory));
    }

    /* Success. */
    gcmFOOTER_ARG("*Memory=0x%x", *Memory);
    return gcvSTATUS_OK;

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}


/*******************************************************************************
**
**  gcoOS_GetMemorySize
**
**  Get allocated memory from the user heap.
**
**  INPUT:
**
**      gcoOS Os
**          Pointer to an gcoOS object.
**
**      gctPOINTER  Memory
**          Pointer to the memory
**          allocation.
**
**  OUTPUT:
**
**      gctPOINTER MemorySize
**          Pointer to a variable that will hold the pointer to the memory
**          size.
*/
gceSTATUS
gcoOS_GetMemorySize(
    IN gcoOS Os,
    IN gctPOINTER Memory,
    OUT gctSIZE_T_PTR MemorySize
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Memory=0x%x", Memory);

    /* Verify the arguments. */
    gcmDEBUG_VERIFY_ARGUMENT(Memory != gcvNULL);
    gcmDEBUG_VERIFY_ARGUMENT(MemorySize != gcvNULL);

    /* Free the memory. */
    if ((gcPLS.os != gcvNULL) && (gcPLS.os->heap != gcvNULL))
    {
        gcmONERROR(gcoHEAP_GetMemorySize(gcPLS.os->heap, Memory, MemorySize));
    }
    else
    {
        *MemorySize = 0;
    }

    /* Success. */
    gcmFOOTER_NO();
    return gcvSTATUS_OK;

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}

/*******************************************************************************
 **
 ** gcoOS_Free
 **
 ** Free allocated memory from the user heap.
 **
 ** INPUT:
 **
 **     gcoOS Os
 **         Pointer to an gcoOS object.
 **
 **     gctPOINTER Memory
 **         Pointer to the memory allocation that needs to be freed.
 **
 ** OUTPUT:
 **
 **     Nothing.
 */
gceSTATUS
gcoOS_Free(
    IN gcoOS Os,
    IN gctPOINTER Memory
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Memory=0x%x", Memory);

    /* Verify the arguments. */
    gcmDEBUG_VERIFY_ARGUMENT(Memory != gcvNULL);

    /* Free the memory. */
    if ((gcPLS.os != gcvNULL) && (gcPLS.os->heap != gcvNULL))
    {
        gcmONERROR(gcoHEAP_Free(gcPLS.os->heap, Memory));
    }
    else
    {
        gcmONERROR(gcoOS_FreeMemory(gcPLS.os, Memory));
    }

    /* Success. */
    gcmFOOTER_NO();
    return gcvSTATUS_OK;

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gcoOS_AllocateSharedMemory
**
**  Allocate memory that can be used in both user and kernel.
**
**  INPUT:
**
**      gcoOS Os
**          Pointer to an gcoOS object.
**
**      gctSIZE_T Bytes
**          Number of bytes to allocate.
**
**  OUTPUT:
**
**      gctPOINTER * Memory
**          Pointer to a variable that will hold the pointer to the memory
**          allocation.
*/
gceSTATUS
gcoOS_AllocateSharedMemory(
    IN gcoOS Os,
    IN gctSIZE_T Bytes,
    OUT gctPOINTER * Memory
    )
{
    return gcoOS_Allocate(Os, Bytes, Memory);
}

/*******************************************************************************
 **
 ** gcoOS_FreeSharedMemory
 **
 ** Free allocated memory.
 **
 ** INPUT:
 **
 **     gcoOS Os
 **         Pointer to an gcoOS object.
 **
 **     gctPOINTER Memory
 **         Pointer to the memory allocation that needs to be freed.
 **
 ** OUTPUT:
 **
 **     Nothing.
 */
gceSTATUS
gcoOS_FreeSharedMemory(
    IN gcoOS Os,
    IN gctPOINTER Memory
    )
{
    return gcoOS_Free(Os, Memory);
}

#define VP_MALLOC_OFFSET        (16)

/*******************************************************************************
 **
 ** gcoOS_AllocateMemory
 **
 ** Allocate memory from the user heap.
 **
 ** INPUT:
 **
 **     gcoOS Os
 **         Pointer to an gcoOS object.
 **
 **     gctSIZE_T Bytes
 **         Number of bytes to allocate.
 **
 ** OUTPUT:
 **
 **     gctPOINTER * Memory
 **         Pointer to a variable that will hold the pointer to the memory
 **         allocation.
 */
gceSTATUS
gcoOS_AllocateMemory(
    IN gcoOS Os,
    IN gctSIZE_T Bytes,
    OUT gctPOINTER * Memory
    )
{
    gceSTATUS status;
    gctPOINTER memory;

    gcmHEADER_ARG("Bytes=%lu", Bytes);

    /* Verify the arguments. */
    gcmDEBUG_VERIFY_ARGUMENT(Bytes > 0);
    gcmDEBUG_VERIFY_ARGUMENT(Memory != gcvNULL);

    /* Allocate the memory. */
#if VIVANTE_PROFILER_SYSTEM_MEMORY
    memory = malloc(Bytes + VP_MALLOC_OFFSET);
#else
    memory = malloc(Bytes);
#endif

    if (memory == gcvNULL)
    {
        /* Out of memory. */
        gcmONERROR(gcvSTATUS_OUT_OF_MEMORY);
    }

#if VIVANTE_PROFILER_SYSTEM_MEMORY
    {
        gcoOS os = (gcPLS.os != gcvNULL) ? gcPLS.os : Os;

        if (os != gcvNULL)
        {
            ++ (os->allocCount);
            os->allocSize += Bytes;
            if (os->allocSize > os->maxAllocSize)
            {
                os->maxAllocSize = os->allocSize;
            }

#if gcdGC355_MEM_PRINT
            if (os->oneRecording == 1)
            {
                os->oneSize += (gctINT32)Bytes;
            }
#endif
        }

        /* Return pointer to the memory allocation. */
        *(gctSIZE_T *) memory = Bytes;
        *Memory = (gctPOINTER) ((gctUINT8 *) memory + VP_MALLOC_OFFSET);
    }
#else
    /* Return pointer to the memory allocation. */
    *Memory = memory;
#endif

    /* Success. */
    gcmFOOTER_ARG("*Memory=0x%x", *Memory);
    return gcvSTATUS_OK;

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}

/*******************************************************************************
 **
 ** gcoOS_ReallocMemory
 **
 ** Allocate memory from the user heap.
 **
 ** INPUT:
 **
 **     gcoOS Os
 **         Pointer to an gcoOS object.
 **
 **     gctSIZE_T Bytes
 **         Number of bytes to allocate.
 **
 ** OUTPUT:
 **
 **     gctPOINTER * Memory
 **         Pointer to a variable that will hold the pointer to the memory
 **         allocation.
 */
gceSTATUS
gcoOS_ReallocMemory(
    IN gcoOS Os,
    IN gctSIZE_T Bytes,
    IN gctSIZE_T OrgBytes,
    OUT gctPOINTER * Memory
    )
{
    gceSTATUS status;
    gctPOINTER memory;

    gcmHEADER_ARG("Bytes=%lu", Bytes);

    /* Verify the arguments. */
    gcmDEBUG_VERIFY_ARGUMENT(Bytes > 0);
    gcmDEBUG_VERIFY_ARGUMENT(Memory != gcvNULL);

    /* Allocate the memory. */
#if VIVANTE_PROFILER_SYSTEM_MEMORY
    memory = realloc(*(gctUINT8_PTR *)Memory - VP_MALLOC_OFFSET , Bytes + VP_MALLOC_OFFSET);
#else
    memory = realloc(*Memory, Bytes);
#endif

    if (memory == gcvNULL)
    {
        /* Out of memory. */
        gcmONERROR(gcvSTATUS_OUT_OF_MEMORY);
    }

#if VIVANTE_PROFILER_SYSTEM_MEMORY
    {
        gcoOS os = (gcPLS.os != gcvNULL) ? gcPLS.os : Os;

        if (os != gcvNULL)
        {
            ++ (os->allocCount);
            ++ (os->freeCount);
            os->allocSize += Bytes;
            os->freeSize += OrgBytes;
            if (os->allocSize > os->maxAllocSize)
            {
                os->maxAllocSize = os->allocSize;
            }

#if gcdGC355_MEM_PRINT
            if (os->oneRecording == 1)
            {
                os->oneSize -= (gctINT32)OrgBytes;
                os->oneSize += (gctINT32)Bytes;
            }
#endif
        }

        /* Return pointer to the memory allocation. */
        *(gctSIZE_T *) memory = Bytes;
        *Memory = (gctPOINTER) ((gctUINT8 *) memory + VP_MALLOC_OFFSET);
    }
#else
    /* Return pointer to the memory allocation. */
    *Memory = memory;
#endif

    /* Success. */
    gcmFOOTER_ARG("*Memory=0x%x", *Memory);
    return gcvSTATUS_OK;

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}


/*******************************************************************************
 **
 ** gcoOS_FreeMemory
 **
 ** Free allocated memory from the user heap.
 **
 ** INPUT:
 **
 **     gcoOS Os
 **         Pointer to an gcoOS object.
 **
 **     gctPOINTER Memory
 **         Pointer to the memory allocation that needs to be freed.
 **
 ** OUTPUT:
 **
 **     Nothing.
 */
gceSTATUS
gcoOS_FreeMemory(
    IN gcoOS Os,
    IN gctPOINTER Memory
    )
{
    gcmHEADER_ARG("Memory=0x%x", Memory);

    /* Verify the arguments. */
    gcmDEBUG_VERIFY_ARGUMENT(Memory != gcvNULL);

    /* Free the memory allocation. */
#if VIVANTE_PROFILER_SYSTEM_MEMORY
    {
        gcoOS os = (gcPLS.os != gcvNULL) ? gcPLS.os : Os;
        gctPOINTER memory;

        memory = (gctUINT8 *) Memory - VP_MALLOC_OFFSET;

        if (os != gcvNULL)
        {
#if gcdGC355_MEM_PRINT
            if (os->oneRecording == 1)
            {
                os->oneSize -= (gctINT32)(*(gctSIZE_T *) memory);
            }
#endif
            os->allocSize -= *(gctSIZE_T *) memory;
            os->freeSize += *(gctSIZE_T *) memory;
            free(memory);
            ++ (os->freeCount);
        }
    }
#else
    free(Memory);
#endif

    /* Success. */
    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

/*******************************************************************************
 **
 ** gcoOS_DeviceControl
 **
 ** Perform a device I/O control call to the kernel API.
 **
 ** INPUT:
 **
 **     gcoOS Os
 **         Pointer to an gcoOS object.
 **
 **     gctUINT32 IoControlCode
 **         I/O control code to execute.
 **
 **     gctPOINTER InputBuffer
 **         Pointer to the input buffer.
 **
 **     gctSIZE_T InputBufferSize
 **         Size of the input buffer in bytes.
 **
 **     gctSIZE_T outputBufferSize
 **         Size of the output buffer in bytes.
 **
 ** OUTPUT:
 **
 **     gctPOINTER OutputBuffer
 **         Output buffer is filled with the data returned from the kernel HAL
 **         layer.
 */

 gceSTATUS
 gcoOS_DeviceControl(
     IN gcoOS Os,
     IN gctUINT32 IoControlCode,
     IN gctPOINTER InputBuffer,
     IN gctSIZE_T InputBufferSize,
     OUT gctPOINTER OutputBuffer,
     IN gctSIZE_T OutputBufferSize
     )
 {
     gceSTATUS status;
     gcsHAL_INTERFACE_PTR inputBuffer;
     gcsHAL_INTERFACE_PTR outputBuffer;
     gcsDRIVER_ARGS args;
     gcsTLS_PTR tls;
     gctPOINTER logical = gcvNULL;
     gctUINT32 interrupt_count = 0;

     gcmHEADER_ARG("IoControlCode=%u InputBuffer=0x%x "
                   "InputBufferSize=%lu OutputBuffer=0x%x OutputBufferSize=%lu",
                   IoControlCode, InputBuffer, InputBufferSize,
                   OutputBuffer, OutputBufferSize);

     if (gcPLS.os == gcvNULL)
     {
         gcmONERROR(gcvSTATUS_DEVICE);
     }

     /* Cast the interface. */
     inputBuffer  = (gcsHAL_INTERFACE_PTR) InputBuffer;
     outputBuffer = (gcsHAL_INTERFACE_PTR) OutputBuffer;

     if (!inputBuffer->ignoreTLS)
     {
         /* Set current hardware type */
         if (gcPLS.processID)
         {
             gcmONERROR(gcoOS_GetTLS(&tls));
             inputBuffer->hardwareType = tls->currentType;
             inputBuffer->coreIndex = tls->currentCoreIndex;
         }
         else
         {
             inputBuffer->hardwareType = gcvHARDWARE_2D;
             inputBuffer->coreIndex = 0;
         }
     }

     switch (inputBuffer->command)
     {
     case gcvHAL_MAP_MEMORY:
         logical = mmap(
             gcvNULL, inputBuffer->u.MapMemory.bytes,
             PROT_READ | PROT_WRITE, MAP_SHARED,
             gcPLS.os->device, (off_t) 0
             );

         if (logical != MAP_FAILED)
         {
             inputBuffer->u.MapMemory.logical = gcmPTR_TO_UINT64(logical);
             inputBuffer->status = gcvSTATUS_OK;
             gcmFOOTER_NO();
             return gcvSTATUS_OK;
         }
         break;

     case gcvHAL_UNMAP_MEMORY:
         munmap(gcmUINT64_TO_PTR(inputBuffer->u.UnmapMemory.logical), inputBuffer->u.UnmapMemory.bytes);

         inputBuffer->status = gcvSTATUS_OK;
         gcmFOOTER_NO();
         return gcvSTATUS_OK;

     default:
         /* This has to be here so that GCC does not complain. */
         break;
     }

     /* Call kernel. */
     args.InputBuffer       = gcmPTR_TO_UINT64(InputBuffer);
     args.InputBufferSize  = InputBufferSize;
     args.OutputBuffer       = gcmPTR_TO_UINT64(OutputBuffer);
     args.OutputBufferSize = OutputBufferSize;

     while (ioctl(gcPLS.os->device, IoControlCode, &args) < 0)
     {
         if (errno != EINTR)
         {
             gcmTRACE(gcvLEVEL_ERROR, "ioctl failed; errno=%s\n", strerror(errno));
             gcmONERROR(gcvSTATUS_GENERIC_IO);
         }
         /* Retry MAX_RETRY_IOCTL_TIMES times at most when receiving interrupt */
         else if (++interrupt_count == MAX_RETRY_IOCTL_TIMES)
         {
             gcmTRACE(gcvLEVEL_ERROR, "ioctl failed; too many interrupt\n");
             gcmONERROR(gcvSTATUS_GENERIC_IO);
         }
     }

     /* Get the status. */
     status = outputBuffer->status;

     /* Eliminate gcmONERROR on gcoOS_WaitSignal timeout errors. */
#if gcmIS_DEBUG(gcdDEBUG_CODE)
     if ((inputBuffer->command == gcvHAL_USER_SIGNAL) &&
         (inputBuffer->u.UserSignal.command == gcvUSER_SIGNAL_WAIT))
     {
         if (status == gcvSTATUS_TIMEOUT)
         {
             goto OnError;
         }
     }
#endif

     /* Test for API error. */
     gcmONERROR(status);

OnError:
     /* Return the status. */
     gcmFOOTER();
     return status;
}

/*******************************************************************************
**
**  gcoOS_Open
**
**  Open or create a file.
**
**  INPUT:
**
**      gcoOS Os
**          Pointer to an gcoOS object.
**
**      gctCONST_STRING FileName
**          File name of file to open or create.
**
**      gceFILE_MODE Mode
**          Mode to open file with:
**
**              gcvFILE_CREATE      - Overwite any existing file.
**              gcvFILE_APPEND      - Append to an exisiting file or create a
**                                    new file if there is no exisiting file.
**              gcvFILE_READ        - Open an existing file for read only.
**              gcvFILE_CREATETEXT  - Overwite any existing text file.
**              gcvFILE_APPENDTEXT  - Append to an exisiting text file or create
**                                    a new text file if there is no exisiting
**                                    file.
**              gcvFILE_READTEXT    - Open an existing text file fir read only.
**
**  OUTPUT:
**
**      gctFILE * File
**          Pointer to a variable receivig the handle to the opened file.
*/
gceSTATUS
gcoOS_Open(
    IN gcoOS Os,
    IN gctCONST_STRING FileName,
    IN gceFILE_MODE Mode,
    OUT gctFILE * File
    )
{
    static gctCONST_STRING modes[] =
    {
        "wb",
        "ab",
        "rb",
        "w",
        "a",
        "r",
    };
    FILE * file;

    gcmHEADER_ARG("FileName=%s Mode=%d",
                  gcmOPT_STRING(FileName), Mode);

    /* Verify the arguments. */
    gcmDEBUG_VERIFY_ARGUMENT(File != gcvNULL);

    /* Open the file. */
    file = fopen(FileName, modes[Mode]);

    if (file == gcvNULL)
    {
        /* Error. */
        gcmFOOTER_ARG("status=%d", gcvSTATUS_GENERIC_IO);
        return gcvSTATUS_GENERIC_IO;
    }

    /* Return handle to file. */
    *File = (gctFILE) file;

    /* Success. */
    gcmFOOTER_ARG("*File=0x%x", *File);
    return gcvSTATUS_OK;
}

/*******************************************************************************
**
**  gcoOS_Close
**
**  Close a file.
**
**  INPUT:
**
**      gcoOS Os
**          Pointer to an gcoOS object.
**
**      gctFILE File
**          Pointer to an open file object.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gcoOS_Close(
    IN gcoOS Os,
    IN gctFILE File
    )
{
    gcmHEADER_ARG("File=0x%x", File);

    /* Verify the arguments. */
    gcmDEBUG_VERIFY_ARGUMENT(File != gcvNULL);

    /* Close the file. */
    fclose((FILE *) File);

    /* Success. */
    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

/*******************************************************************************
**
**  gcoOS_Read
**
**  Read data from an open file.
**
**  INPUT:
**
**      gcoOS Os
**          Pointer to an gcoOS object.
**
**      gctFILE File
**          Pointer to an open file object.
**
**      gctSIZE_T ByteCount
**          Number of bytes to read from the file.
**
**      gctCONST_POINTER Data
**          Pointer to the data to read from the file.
**
**  OUTPUT:
**
**      gctSIZE_T * ByteRead
**          Pointer to a variable receiving the number of bytes read from the
**          file.
*/
gceSTATUS
gcoOS_Read(
    IN gcoOS Os,
    IN gctFILE File,
    IN gctSIZE_T ByteCount,
    IN gctPOINTER Data,
    OUT gctSIZE_T * ByteRead
    )
{
    size_t byteRead;

    gcmHEADER_ARG("File=0x%x ByteCount=%lu Data=0x%x",
                  File, ByteCount, Data);

    /* Verify the arguments. */
    gcmDEBUG_VERIFY_ARGUMENT(File != gcvNULL);
    gcmDEBUG_VERIFY_ARGUMENT(ByteCount > 0);
    gcmDEBUG_VERIFY_ARGUMENT(Data != gcvNULL);

    /* Read the data from the file. */
    byteRead = fread(Data, 1, ByteCount, (FILE *) File);

    if (ByteRead != gcvNULL)
    {
        *ByteRead = (gctSIZE_T) byteRead;
        /* Success. */
        gcmFOOTER_ARG("*ByteRead=%lu", gcmOPT_VALUE(ByteRead));
        return gcvSTATUS_OK;
    }
    else
    {
        /* Failure. */
        gcmFOOTER_ARG("%d", gcvSTATUS_TRUE);
        return gcvSTATUS_TRUE;
    }
}

/*******************************************************************************
**
**  gcoOS_Write
**
**  Write data to an open file.
**
**  INPUT:
**
**      gcoOS Os
**          Pointer to an gcoOS object.
**
**      gctFILE File
**          Pointer to an open file object.
**
**      gctSIZE_T ByteCount
**          Number of bytes to write to the file.
**
**      gctCONST_POINTER Data
**          Pointer to the data to write to the file.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gcoOS_Write(
    IN gcoOS Os,
    IN gctFILE File,
    IN gctSIZE_T ByteCount,
    IN gctCONST_POINTER Data
    )
{
    size_t byteWritten;

    gcmHEADER_ARG("File=0x%x ByteCount=%lu Data=0x%x",
                  File, ByteCount, Data);

    /* Verify the arguments. */
    gcmDEBUG_VERIFY_ARGUMENT(File != gcvNULL);
    gcmDEBUG_VERIFY_ARGUMENT(ByteCount > 0);
    gcmDEBUG_VERIFY_ARGUMENT(Data != gcvNULL);

    /* Write the data to the file. */
    byteWritten = fwrite(Data, 1, ByteCount, (FILE *) File);

    if (byteWritten == ByteCount)
    {
        /* Success. */
        gcmFOOTER_NO();
        return gcvSTATUS_OK;
    }
    else
    {
        /* Error */
        gcmFOOTER_ARG("status=%d", gcvSTATUS_GENERIC_IO);
        return gcvSTATUS_GENERIC_IO;
    }
}

/* Flush data to a file. */
gceSTATUS
gcoOS_Flush(
    IN gcoOS Os,
    IN gctFILE File
    )
{
    gcmHEADER_ARG("File=0x%x", File);

    fflush((FILE *) File);

    /* Success. */
    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

gceSTATUS
gcoOS_FscanfI(
    IN gcoOS Os,
    IN gctFILE File,
    IN gctCONST_STRING Format,
    OUT gctUINT *result
    )
{
    gctINT ret;
    gcmHEADER_ARG("File=0x%x Format=0x%x", File, Format);

    /* Verify the arguments. */
    gcmVERIFY_ARGUMENT(File != gcvNULL);
    gcmVERIFY_ARGUMENT(Format != gcvNULL);

    /* Format the string. */
    ret = fscanf(File, Format, result);

    if (!ret)
    {
        gcmFOOTER_NO();
        return gcvSTATUS_GENERIC_IO;
    }

    /* Success. */
    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

gceSTATUS
gcoOS_DupFD(
    IN gcoOS Os,
    IN gctINT FD,
    OUT gctINT * FD2
    )
{
    int fd;
    gceSTATUS status;

    gcmHEADER_ARG("FD=%d", FD);
    fd = dup(FD);

    if (fd < 0)
    {
        status = gcvSTATUS_OUT_OF_RESOURCES;
        gcmFOOTER();
        return status;
    }

    *FD2 = fd;
    gcmFOOTER_ARG("FD2=%d", FD2);
    return gcvSTATUS_OK;
}

gceSTATUS
gcoOS_CloseFD(
    IN gcoOS Os,
    IN gctINT FD
    )
{
    int err;
    gceSTATUS status;
    gcmHEADER_ARG("FD=%d", FD);

    err = close(FD);

    if (err < 0)
    {
        status = gcvSTATUS_GENERIC_IO;
        gcmFOOTER();
        return status;
    }

    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

/*******************************************************************************
**
**  gcoOS_LockFile
**
**  Apply an advisory lock on an open file
**
**  INPUT:
**
**      gcoOS Os
**          Pointer to an gcoOS object.
**
**      gctFILE File
**          Pointer to an open file object.
**
**      gctBOOL Shared
**          Place a shared lock if true. More than one process may hold a
**          shared lock for a given file at a given time.
**          Place an exclusive lock. Only one process may hold an exclusive
**          lock for a given file at a given time.
**
**      gctBOOL Block
**          Block if an incompatible lock is held by another process.
**          Otherwise return immediately with gcvSTATUS_LOCKED error.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gcoOS_LockFile(
    IN gcoOS Os,
    IN gctFILE File,
    IN gctBOOL Shared,
    IN gctBOOL Block
    )
{
    return gcvSTATUS_OK;
}

/*******************************************************************************
**
**  gcoOS_UnlockFile
**
**  Remove an advisory lock on an open file.
**
**  INPUT:
**
**      gcoOS Os
**          Pointer to an gcoOS object.
**
**      gctFILE File
**          Pointer to an open file object.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gcoOS_UnlockFile(
    IN gcoOS Os,
    IN gctFILE File
    )
{
    return gcvSTATUS_OK;
}


/* Create an endpoint for communication. */
gceSTATUS
gcoOS_Socket(
    IN gcoOS Os,
    IN gctINT Domain,
    IN gctINT Type,
    IN gctINT Protocol,
    OUT gctINT * SockFd
    )
{
    gctINT fd;

    gcmHEADER_ARG("Domain=%d Type=%d Protocol=%d",
                  Domain, Type, Protocol);

    /* Create a socket. */
    fd = socket(Domain, Type, Protocol);

    if (fd >= 0)
    {
        /* Return socket descriptor. */
        *SockFd = fd;

        /* Success. */
        gcmFOOTER_ARG("*SockFd=%d", *SockFd);
        return gcvSTATUS_OK;
    }
    else
    {
        /* Error. */
        gcmFOOTER_ARG("status=%d", gcvSTATUS_GENERIC_IO);
        return gcvSTATUS_GENERIC_IO;
    }
}

/* Close a socket. */
gceSTATUS
gcoOS_WaitForSend(
    IN gcoOS Os,
    IN gctINT SockFd,
    IN gctINT Seconds,
    IN gctINT MicroSeconds
    )
{
    gcmHEADER_ARG("SockFd=%d Seconds=%d MicroSeconds=%d",
                  SockFd, Seconds, MicroSeconds);

    struct timeval tv;
    fd_set writefds;
    int ret;

    tv.tv_sec  = Seconds;
    tv.tv_usec = MicroSeconds;

    FD_ZERO(&writefds);
    FD_SET(SockFd, &writefds);

    ret = select(SockFd + 1, NULL, &writefds, NULL, &tv);

    if (ret == 0)
    {
        /* Timeout. */
        gcmFOOTER_ARG("status=%d", gcvSTATUS_TIMEOUT);
        return gcvSTATUS_TIMEOUT;
    }
    else if (ret == -1)
    {
        /* Error. */
        gcmFOOTER_ARG("status=%d", gcvSTATUS_GENERIC_IO);
        return gcvSTATUS_GENERIC_IO;
    }
    else
    {
        int error = 0;
        int len = sizeof(error);

        /* Get error code. */
        getsockopt(SockFd, SOL_SOCKET, SO_ERROR, (char*) &error, &len);

        if (! error)
        {
            /* Success. */
            gcmFOOTER_NO();
            return gcvSTATUS_OK;
        }
    }

    /* Error */
    gcmFOOTER_ARG("status=%d", gcvSTATUS_GENERIC_IO);
    return gcvSTATUS_GENERIC_IO;
}

/* Close a socket. */
gceSTATUS
gcoOS_CloseSocket(
    IN gcoOS Os,
    IN gctINT SockFd
    )
{
    gcmHEADER_ARG("SockFd=%d", SockFd);

    /* Close the socket file descriptor. */
    gcoOS_WaitForSend(gcvNULL, SockFd, 600, 0);
    close(SockFd);

    /* Success. */
    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

/* Initiate a connection on a socket. */
gceSTATUS
gcoOS_Connect(
    IN gcoOS Os,
    IN gctINT SockFd,
    IN gctCONST_POINTER HostName,
    IN gctUINT Port
    )
{
    gctINT rc;
    gctINT addrLen;
    struct sockaddr sockAddr;
    struct sockaddr_in *sockAddrIn;
    struct in_addr *inAddr;

    gcmHEADER_ARG("SockFd=0x%x HostName=0x%x Port=%d",
                  SockFd, HostName, Port);

    /* Get server address. */
    sockAddrIn = (struct sockaddr_in *) &sockAddr;
    sockAddrIn->sin_family = AF_INET;
    inAddr = &sockAddrIn->sin_addr;
    inAddr->s_addr = inet_addr(HostName);

    /* If it is a numeric host name, convert it now */
    if (inAddr->s_addr == INADDR_NONE)
    {
#if !gcdSTATIC_LINK
        struct hostent *hostEnt;
        struct in_addr *arrayAddr;

        /* It is a real name, we solve it */
        if ((hostEnt = gethostbyname(HostName)) == NULL)
        {
            /* Error */
            gcmFOOTER_ARG("status=%d", gcvSTATUS_GENERIC_IO);
            return gcvSTATUS_GENERIC_IO;
        }
        arrayAddr = (struct in_addr *) *(hostEnt->h_addr_list);
        inAddr->s_addr = arrayAddr[0].s_addr;
#endif /*gcdSTATIC_LINK*/
    }

    sockAddrIn->sin_port = htons((gctUINT16) Port);

    /* Currently, for INET only. */
    addrLen = sizeof(struct sockaddr);

    /*{
    gctINT arg = 1;
    ioctl(SockFd, FIONBIO, &arg);
    }*/

    /* Close the file descriptor. */
    rc = connect(SockFd, &sockAddr, addrLen);

    if (rc)
    {
        int err = errno;

        if (err == EINPROGRESS)
        {
            gceSTATUS status;

            /* Connect is not complete.  Wait for it. */
            status = gcoOS_WaitForSend(gcvNULL, SockFd, 600, 0);

            gcmFOOTER();
            return status;
        }

        /* Error */
        gcmFOOTER_ARG("status=%d", gcvSTATUS_GENERIC_IO);
        return gcvSTATUS_GENERIC_IO;
    }

    /* Success. */
    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

/* Shut down part of connection on a socket. */
gceSTATUS
gcoOS_Shutdown(
    IN gcoOS Os,
    IN gctINT SockFd,
    IN gctINT How
    )
{
    gcmHEADER_ARG("SockFd=%d How=%d", SockFd, How);

    /* Shut down connection. */
    shutdown(SockFd, How);

    /* Success. */
    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

/* Send a message on a socket. */
gceSTATUS
gcoOS_Send(
    IN gcoOS Os,
    IN gctINT SockFd,
    IN gctSIZE_T ByteCount,
    IN gctCONST_POINTER Data,
    IN gctINT Flags
    )
{
    gctINT byteSent;

    gcmHEADER_ARG("SockFd=0x%x ByteCount=%lu Data=0x%x Flags=%d",
                  SockFd, ByteCount, Data, Flags);

    /* Verify the arguments. */
    gcmDEBUG_VERIFY_ARGUMENT(ByteCount > 0);
    gcmDEBUG_VERIFY_ARGUMENT(Data != gcvNULL);

    /* Write the data to the file. */
    /*gcoOS_WaitForSend(gcvNULL, SockFd, 0, 50000);*/
    byteSent = send(SockFd, Data, ByteCount, Flags);

    if (byteSent == (gctINT) ByteCount)
    {
        /* Success. */
        gcmFOOTER_NO();
        return gcvSTATUS_OK;
    }
    else
    {
        /* Error */
        gcmFOOTER_ARG("status=%d", gcvSTATUS_GENERIC_IO);
        return gcvSTATUS_GENERIC_IO;
    }
}

/* Get environment variable value. */
gceSTATUS
gcoOS_GetEnv(
    IN gcoOS Os,
    IN gctCONST_STRING VarName,
    OUT gctSTRING * Value
    )
{
    return gcvSTATUS_OK;
}

/* Set environment variable value. */
gceSTATUS gcoOS_SetEnv(
    IN gcoOS Os,
    IN gctCONST_STRING VarName,
    IN gctSTRING Value
    )
{
    return gcvSTATUS_OK;
}

/* Get current working directory. */
gceSTATUS
gcoOS_GetCwd(
    IN gcoOS Os,
    IN gctINT SizeInBytes,
    OUT gctSTRING Buffer
    )
{
    gcmHEADER_ARG("SizeInBytes=%d", SizeInBytes);

    if (getcwd(Buffer, SizeInBytes))
    {
        gcmFOOTER_ARG("Buffer=%s", Buffer);
        return gcvSTATUS_NOT_SUPPORTED;
    }
    else
    {
        gcmFOOTER_ARG("status=%d", gcvSTATUS_NOT_SUPPORTED);
        return gcvSTATUS_NOT_SUPPORTED;
    }
}

/* Get file status info. */
gceSTATUS
gcoOS_Stat(
    IN gcoOS Os,
    IN gctCONST_STRING FileName,
    OUT gctPOINTER Buffer
    )
{
    gcmHEADER_ARG("FileName=%s", gcmOPT_STRING(FileName));

    if (stat(FileName, Buffer) == 0)
    {
        gcmFOOTER_NO();
        return gcvSTATUS_OK;
    }
    else
    {
        gcmFOOTER_ARG("status=%d", gcvSTATUS_NOT_SUPPORTED);
        return gcvSTATUS_NOT_SUPPORTED;
    }
}

/*******************************************************************************
**
**  gcoOS_GetPos
**
**  Get the current position of a file.
**
**  INPUT:
**
**      gcoOS Os
**          Pointer to an gcoOS object.
**
**      gctFILE File
**          Pointer to an open file object.
**
**  OUTPUT:
**
**      gctUINT32 * Position
**          Pointer to a variable receiving the current position of the file.
*/
gceSTATUS
gcoOS_GetPos(
    IN gcoOS Os,
    IN gctFILE File,
    OUT gctUINT32 * Position
    )
{
    gctINT64 ret;
    gcmHEADER_ARG("File=0x%x", File);

    /* Verify the arguments. */
    gcmDEBUG_VERIFY_ARGUMENT(File != gcvNULL);
    gcmDEBUG_VERIFY_ARGUMENT(Position != gcvNULL);

    /* Get the current file position. */
    ret = ftell((FILE *) File);

    /* Check if ftell encounters error. */
    if (ret == -1)
    {
        gcmFOOTER_ARG("%d", gcvSTATUS_TRUE);
        return gcvSTATUS_TRUE;
    }
    /* If no error, ret contains a file size of a positive number.
       Check if this file size is supported.
       Since file size will be stored in UINT32, up to 2^32 = 4Gb is supported.
    */
    else if (ret >= gcvMAXUINT32)
    {
        gcmFOOTER_ARG("%d", gcvSTATUS_DATA_TOO_LARGE);
        return gcvSTATUS_DATA_TOO_LARGE;
    }
    else
    {
        /* Now we're sure file size takes <= 32 bit and cast it to UINT32 safely. */
        *Position = (gctUINT32) ret;

        gcmFOOTER_ARG("*Position=%u", *Position);
        return gcvSTATUS_OK;
    }
}

/*******************************************************************************
**
**  gcoOS_SetPos
**
**  Set position for a file.
**
**  INPUT:
**
**      gcoOS Os
**          Pointer to an gcoOS object.
**
**      gctFILE File
**          Pointer to an open file object.
**
**      gctUINT32 Position
**          Absolute position of the file to set.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gcoOS_SetPos(
    IN gcoOS Os,
    IN gctFILE File,
    IN gctUINT32 Position
    )
{
    gcmHEADER_ARG("File=0x%x Position=%u", File, Position);

    /* Verify the arguments. */
    gcmDEBUG_VERIFY_ARGUMENT(File != gcvNULL);

    /* Set file position. */
    fseek((FILE *) File, Position, SEEK_SET);

    /* Success. */
    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

/*******************************************************************************
**
**  gcoOS_Seek
**
**  Set position for a file.
**
**  INPUT:
**
**      gcoOS Os
**          Pointer to an gcoOS object.
**
**      gctFILE File
**          Pointer to an open file object.
**
**      gctUINT32 Offset
**          Offset added to the position specified by Whence.
**
**      gceFILE_WHENCE Whence
**          Mode that specify how to add the offset to the position:
**
**              gcvFILE_SEEK_SET    - Relative to the start of the file.
**              gcvFILE_SEEK_CUR    - Relative to the current position.
**              gcvFILE_SEEK_END    - Relative to the end of the file.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gcoOS_Seek(
    IN gcoOS Os,
    IN gctFILE File,
    IN gctUINT32 Offset,
    IN gceFILE_WHENCE Whence
    )
{
    gctINT result = 0;

    gcmHEADER_ARG("File=0x%x Offset=%u Whence=%d",
                  File, Offset, Whence);

    /* Verify the arguments. */
    gcmDEBUG_VERIFY_ARGUMENT(File != gcvNULL);

    /* Set file position. */
    switch (Whence)
    {
    case gcvFILE_SEEK_SET:
        result = fseek((FILE *) File, Offset, SEEK_SET);
        break;

    case gcvFILE_SEEK_CUR:
        result = fseek((FILE *) File, Offset, SEEK_CUR);
        break;

    case gcvFILE_SEEK_END:
        result = fseek((FILE *) File, Offset, SEEK_END);
        break;
    }

    if (result == 0)
    {
        /* Success. */
        gcmFOOTER_NO();
        return gcvSTATUS_OK;
    }
    else
    {
        /* Error */
        gcmFOOTER_ARG("status=%d", gcvSTATUS_GENERIC_IO);
        return gcvSTATUS_GENERIC_IO;
    }
}

/*******************************************************************************
**
**  gcoOS_CreateThread
**
**  Create a new thread.
**
**  INPUT:
**
**      gcoOS Os
**          Pointer to an gcoOS object.
**
**  OUTPUT:
**
**      gctPOINTER * Thread
**          Pointer to a variable that will hold a pointer to the thread.
*/
gceSTATUS
gcoOS_CreateThread(
    IN gcoOS Os,
    IN gcTHREAD_ROUTINE Worker,
    IN gctPOINTER Argument,
    OUT gctPOINTER * Thread
    )
{
    pthread_t thread;

    gcmHEADER_ARG("Worker=0x%x Argument=0x%x", Worker, Argument);

    /* Validate the arguments. */
    gcmDEBUG_VERIFY_ARGUMENT(Thread != gcvNULL);

    thread = taskSpawn("display", 120, 0, 1024*1024, (FUNCPTR)Worker,
             Argument, 0, 0, 0, 0, 0, 0, 0, 0, 0);

    *Thread = (gctPOINTER) thread;

    /* Success. */
    gcmFOOTER_ARG("*Thread=0x%x", *Thread);
    return gcvSTATUS_OK;
}

/*******************************************************************************
**
**  gcoOS_CloseThread
**
**  Close a thread.
**
**  INPUT:
**
**      gcoOS Os
**          Pointer to an gcoOS object.
**
**      gctPOINTER Thread
**          Pointer to the thread to be deleted.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gcoOS_CloseThread(
    IN gcoOS Os,
    IN gctPOINTER Thread
    )
{
    gcmHEADER_ARG("Thread=0x%x", Thread);

    /* Validate the arguments. */
    gcmDEBUG_VERIFY_ARGUMENT(Thread != gcvNULL);

    pthread_join((pthread_t) Thread, gcvNULL);

    /* Success. */
    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

/*******************************************************************************
**
**  gcoOS_CreateMutex
**
**  Create a new mutex.
**
**  INPUT:
**
**      gcoOS Os
**          Pointer to an gcoOS object.
**
**  OUTPUT:
**
**      gctPOINTER * Mutex
**          Pointer to a variable that will hold a pointer to the mutex.
*/
gceSTATUS
gcoOS_CreateMutex(
    IN gcoOS Os,
    OUT gctPOINTER * Mutex
    )
{
    gceSTATUS status;
    pthread_mutex_t* mutex;
    pthread_mutexattr_t   mta;

    gcmHEADER();

    /* Validate the arguments. */
    gcmDEBUG_VERIFY_ARGUMENT(Mutex != gcvNULL);

    /* Allocate memory for the mutex. */
    gcmONERROR(gcoOS_Allocate(
        gcvNULL, gcmSIZEOF(pthread_mutex_t), (gctPOINTER *) &mutex
        ));

    pthread_mutexattr_init(&mta);

    /* Initialize the mutex. */
    pthread_mutex_init(mutex, &mta);

    /* Destroy mta.*/
    pthread_mutexattr_destroy(&mta);

    /* Return mutex to caller. */
    *Mutex = (gctPOINTER) mutex;

    /* Success. */
    gcmFOOTER_ARG("*Mutex = 0x%x", *Mutex);
    return gcvSTATUS_OK;

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gcoOS_DeleteMutex
**
**  Delete a mutex.
**
**  INPUT:
**
**      gcoOS Os
**          Pointer to an gcoOS object.
**
**      gctPOINTER Mutex
**          Pointer to the mutex to be deleted.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gcoOS_DeleteMutex(
    IN gcoOS Os,
    IN gctPOINTER Mutex
    )
{
    pthread_mutex_t *mutex;

    gcmHEADER_ARG("Mutex=0x%x", Mutex);

    /* Validate the arguments. */
    gcmDEBUG_VERIFY_ARGUMENT(Mutex != gcvNULL);

    /* Cast the pointer. */
    mutex = (pthread_mutex_t *) Mutex;

    /* Destroy the mutex. */
    pthread_mutex_destroy(mutex);

    /* Free the memory. */
    gcmVERIFY_OK(gcmOS_SAFE_FREE(gcvNULL, mutex));

    /* Success. */
    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

/*******************************************************************************
**
**  gcoOS_AcquireMutex
**
**  Acquire a mutex.
**
**  INPUT:
**
**      gcoOS Os
**          Pointer to an gcoOS object.
**
**      gctPOINTER Mutex
**          Pointer to the mutex to be acquired.
**
**      gctUINT32 Timeout
**          Timeout value specified in milliseconds.
**          Specify the value of gcvINFINITE to keep the thread suspended
**          until the mutex has been acquired.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gcoOS_AcquireMutex(
    IN gcoOS Os,
    IN gctPOINTER Mutex,
    IN gctUINT32 Timeout
    )
{
    gceSTATUS status;
    pthread_mutex_t *mutex;

    gcmHEADER_ARG("Mutex=0x%x Timeout=%u", Mutex, Timeout);

    /* Validate the arguments. */
    gcmDEBUG_VERIFY_ARGUMENT(Mutex != gcvNULL);

    /* Cast the pointer. */
    mutex = (pthread_mutex_t *) Mutex;

    /* Test for infinite. */
    if (Timeout == gcvINFINITE)
    {
        /* Lock the mutex. */
        if (pthread_mutex_lock(mutex))
        {
            /* Some error. */
            status = gcvSTATUS_GENERIC_IO;
        }
        else
        {
            /* Success. */
            status = gcvSTATUS_OK;
        }
    }
    else
    {
        /* Try locking the mutex. */
        if (pthread_mutex_trylock(mutex))
        {
            /* Assume timeout. */
            status = gcvSTATUS_TIMEOUT;

            /* Loop while not timeout. */
            while (Timeout-- > 0)
            {
                /* Try locking the mutex. */
                if (pthread_mutex_trylock(mutex) == 0)
                {
                    /* Success. */
                    status = gcvSTATUS_OK;
                    break;
                }

                /* Sleep 1 millisecond. */
                gcoOS_Delay(Os, 1);
            }
        }
        else
        {
            /* Success. */
            status = gcvSTATUS_OK;
        }
    }

    /* Return the status. */
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gcoOS_ReleaseMutex
**
**  Release an acquired mutex.
**
**  INPUT:
**
**      gcoOS Os
**          Pointer to an gcoOS object.
**
**      gctPOINTER Mutex
**          Pointer to the mutex to be released.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gcoOS_ReleaseMutex(
    IN gcoOS Os,
    IN gctPOINTER Mutex
    )
{
    pthread_mutex_t *mutex;

    gcmHEADER_ARG("Mutex=0x%x", Mutex);

    /* Validate the arguments. */
    gcmDEBUG_VERIFY_ARGUMENT(Mutex != gcvNULL);

    /* Cast the pointer. */
    mutex = (pthread_mutex_t *) Mutex;

    /* Release the mutex. */
    pthread_mutex_unlock(mutex);

    /* Success. */
    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

/*******************************************************************************
**
**  gcoOS_Delay
**
**  Delay execution of the current thread for a number of milliseconds.
**
**  INPUT:
**
**      gcoOS Os
**          Pointer to an gcoOS object.
**
**      gctUINT32 Delay
**          Delay to sleep, specified in milliseconds.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gcoOS_Delay(
    IN gcoOS Os,
    IN gctUINT32 Delay
    )
{
    gctUINT32 ticksPerSecond = Os->tps;
    gctUINT32 ticks = 0;

    gcmHEADER_ARG("Os=0x%X Delay=%u", Os, Delay);

    if (Delay > 0)
    {
        ticksPerSecond = 1000 / ticksPerSecond;
        ticks = Delay / ticksPerSecond + 1;

        taskDelay(ticks);
    }

    /* Success. */
    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

gceSTATUS
gcoOS_DelayUs(
    IN gcoOS Os,
    IN gctUINT32 Delay
    )
{
    gctUINT32 ticksPerSecond = Os->tps;
    gctUINT32 ticks = 0;

    gcmHEADER_ARG("Os=0x%X Delay=%u", Os, Delay);

    if (Delay > 0)
    {
        ticksPerSecond = 1000 / ticksPerSecond;
        ticks = Delay / 1000 / ticksPerSecond + 1;

        taskDelay(ticks);
    }

    /* Success. */
    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

gceSTATUS
gcoOS_StrStr(
    IN gctCONST_STRING String,
    IN gctCONST_STRING SubString,
    OUT gctSTRING * Output
    )
{
    gctCHAR* pos;
    gceSTATUS status;

    gcmHEADER_ARG("String=0x%x SubString=0x%x", String, SubString);

    /* Verify the arguments. */
    gcmVERIFY_ARGUMENT(String != gcvNULL);
    gcmVERIFY_ARGUMENT(SubString != gcvNULL);

    /* Call C. */
    pos = strstr(String, SubString);
    if (Output)
    {
        *Output = pos;
    }
    status = pos ? gcvSTATUS_TRUE : gcvSTATUS_FALSE;

    /* Success. */
    gcmFOOTER();
    return status;
}

gceSTATUS
gcoOS_StrFindReverse(
    IN gctCONST_STRING String,
    IN gctINT8 Character,
    OUT gctSTRING * Output
    )
{
    gcmHEADER_ARG("String=0x%x Character=%d", String, Character);

    /* Verify the arguments. */
    gcmDEBUG_VERIFY_ARGUMENT(String != gcvNULL);
    gcmDEBUG_VERIFY_ARGUMENT(Output != gcvNULL);

    /* Call C. */
    *Output = strrchr(String, Character);

    /* Success. */
    gcmFOOTER_ARG("*Output=0x%x", *Output);
    return gcvSTATUS_OK;
}

/*******************************************************************************
**
**  gcoOS_StrCopy
**
**  Copy a string.
**
**  INPUT:
**
**      gctSTRING Destination
**          Pointer to the destination string.
**
**      gctCONST_STRING Source
**          Pointer to the source string.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gcoOS_StrCopySafe(
    IN gctSTRING Destination,
    IN gctSIZE_T DestinationSize,
    IN gctCONST_STRING Source
    )
{
    gcmHEADER_ARG("Destination=0x%x DestinationSize=%lu Source=0x%x",
                  Destination, DestinationSize, Source);

    /* Verify the arguments. */
    gcmDEBUG_VERIFY_ARGUMENT(Destination != gcvNULL);
    gcmDEBUG_VERIFY_ARGUMENT(Source != gcvNULL);

    /* Don't overflow the destination buffer. */
    strncpy(Destination, Source, DestinationSize - 1);

    /* Put this there in case the strncpy overflows. */
    Destination[DestinationSize - 1] = '\0';

    /* Success. */
    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

/*******************************************************************************
**
**  gcoOS_StrCat
**
**  Append a string.
**
**  INPUT:
**
**      gctSTRING Destination
**          Pointer to the destination string.
**
**      gctCONST_STRING Source
**          Pointer to the source string.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gcoOS_StrCatSafe(
    IN gctSTRING Destination,
    IN gctSIZE_T DestinationSize,
    IN gctCONST_STRING Source
    )
{
    gctSIZE_T n;

    gcmHEADER_ARG("Destination=0x%x DestinationSize=%lu Source=0x%x",
                  Destination, DestinationSize, Source);

    /* Verify the arguments. */
    gcmDEBUG_VERIFY_ARGUMENT(Destination != gcvNULL);
    gcmDEBUG_VERIFY_ARGUMENT(Source != gcvNULL);

    /* Find the end of the destination. */
    n = strlen(Destination);
    if (n + 1 < DestinationSize)
    {
        /* Append the string but don't overflow the destination buffer. */
        strncpy(Destination + n, Source, DestinationSize - n - 1);

        /* Put this there in case the strncpy overflows. */
        Destination[DestinationSize - 1] = '\0';

        /* Success. */
        gcmFOOTER_NO();
        return gcvSTATUS_OK;
    }
    else
    {
        /* Failure */
        gcmFOOTER_ARG("%d", gcvSTATUS_TRUE);
        return gcvSTATUS_TRUE;
    }
}

/*******************************************************************************
**
**  gcoOS_StrCmp
**
**  Compare two strings and return whether they match or not.
**
**  INPUT:
**
**      gctCONST_STRING String1
**          Pointer to the first string to compare.
**
**      gctCONST_STRING String2
**          Pointer to the second string to compare.
**
**  OUTPUT:
**
**      Nothing.
**
**  RETURNS:
**
**      gcvSTATUS_OK if the strings match
**      gcvSTATUS_LARGER if String1 > String2
**      gcvSTATUS_SMALLER if String1 < String2
*/
gceSTATUS
gcoOS_StrCmp(
    IN gctCONST_STRING String1,
    IN gctCONST_STRING String2
    )
{
    int result;
    gceSTATUS status;

    gcmHEADER_ARG("String1=0x%x String2=0x%x", String1, String2);

    /* Verify the arguments. */
    gcmDEBUG_VERIFY_ARGUMENT(String1 != gcvNULL);
    gcmDEBUG_VERIFY_ARGUMENT(String2 != gcvNULL);

    /* Compare the strings and return proper status. */
    result = strcmp(String1, String2);

    status = (result == 0) ? gcvSTATUS_OK
           : (result >  0) ? gcvSTATUS_LARGER
                           : gcvSTATUS_SMALLER;

    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gcoOS_StrNCmp
**
**  Compare characters of two strings and return whether they match or not.
**
**  INPUT:
**
**      gctCONST_STRING String1
**          Pointer to the first string to compare.
**
**      gctCONST_STRING String2
**          Pointer to the second string to compare.
**
**      gctSIZE_T Count
**          Number of characters to compare.
**
**  OUTPUT:
**
**      Nothing.
**
**  RETURNS:
**
**      gcvSTATUS_OK if the strings match
**      gcvSTATUS_LARGER if String1 > String2
**      gcvSTATUS_SMALLER if String1 < String2
*/
gceSTATUS
gcoOS_StrNCmp(
    IN gctCONST_STRING String1,
    IN gctCONST_STRING String2,
    IN gctSIZE_T Count
    )
{
    int result;
    gceSTATUS status;

    gcmHEADER_ARG("String1=0x%x String2=0x%x Count=%lu",
                  String1, String2, Count);

    /* Verify the arguments. */
    gcmDEBUG_VERIFY_ARGUMENT(String1 != gcvNULL);
    gcmDEBUG_VERIFY_ARGUMENT(String2 != gcvNULL);

    /* Compare the strings and return proper status. */
    result = strncmp(String1, String2, Count);

    status = (result == 0)
            ? gcvSTATUS_OK
            : ((result > 0) ? gcvSTATUS_LARGER : gcvSTATUS_SMALLER);
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gcoOS_StrToFloat
**
**  Convert string to float.
**
**  INPUT:
**
**      gctCONST_STRING String
**          Pointer to the string to be converted.
**
**
**  OUTPUT:
**
**      gctFLOAT * Float
**          Pointer to a variable that will receive the float.
**
*/
gceSTATUS
gcoOS_StrToFloat(
    IN gctCONST_STRING String,
    OUT gctFLOAT * Float
    )
{
    gcmHEADER_ARG("String=%s", gcmOPT_STRING(String));

    /* Verify the arguments. */
    gcmDEBUG_VERIFY_ARGUMENT(String != gcvNULL);

    *Float = (gctFLOAT) atof(String);

    gcmFOOTER_ARG("*Float=%f", *Float);
    return gcvSTATUS_OK;
}

/*******************************************************************************
**
**  gcoOS_StrToDouble
**
**  Convert string to double.
**
**  INPUT:
**
**      gctCONST_STRING String
**          Pointer to the string to be converted.
**
**
**  OUTPUT:
**
**      gctDOUBLE * Double
**          Pointer to a variable that will receive the double.
**
*/
gceSTATUS
gcoOS_StrToDouble(
    IN gctCONST_STRING String,
    OUT gctDOUBLE* Double
)
{
    gcmHEADER_ARG("String=%s", gcmOPT_STRING(String));

    /* Verify the arguments. */
    gcmDEBUG_VERIFY_ARGUMENT(String != gcvNULL);

    *Double = atof(String);

    gcmFOOTER_ARG("*Double=%f", *Double);
    return gcvSTATUS_OK;
}

/* Converts a hex string to 32-bit integer. */
gceSTATUS gcoOS_HexStrToInt(IN gctCONST_STRING String,
               OUT gctINT * Int)
{
    gcmHEADER_ARG("String=%s", gcmOPT_STRING(String));
    gcmDEBUG_VERIFY_ARGUMENT(String != gcvNULL);
    gcmDEBUG_VERIFY_ARGUMENT(Int != gcvNULL);

    sscanf(String, "%x", ((gctUINT*)Int));

    gcmFOOTER_ARG("*Int=%d", *Int);
    return gcvSTATUS_OK;
}

/* Converts a hex string to float. */
gceSTATUS gcoOS_HexStrToFloat(IN gctCONST_STRING String,
               OUT gctFLOAT * Float)
{
    gctSTRING pch = gcvNULL;
    gctCONST_STRING delim = "x.p";
    gctFLOAT b=0.0, exp=0.0;
    gctINT s=0;
    gctSTRING saveptr;

    gcmHEADER_ARG("String=%s", gcmOPT_STRING(String));
    gcmDEBUG_VERIFY_ARGUMENT(String != gcvNULL);
    gcmDEBUG_VERIFY_ARGUMENT(Float != gcvNULL);

    pch = strtok_r((gctSTRING)String, delim, &saveptr);
    if (pch == NULL) goto onError;

    pch = strtok_r(NULL, delim, &saveptr);
    if (pch == NULL) goto onError;
    gcmVERIFY_OK(gcoOS_StrToFloat(pch, &b));

    pch = strtok_r(NULL, delim, &saveptr);
    if (pch == NULL) goto onError;
    gcmVERIFY_OK(gcoOS_HexStrToInt(pch, &s));

    pch = strtok_r(NULL, delim, &saveptr);
    if (pch == NULL) goto onError;
    gcmVERIFY_OK(gcoOS_StrToFloat(pch, &exp));

    *Float = (float)(b + s / (float)(1 << 24)) * (float)pow(2.0, exp);

    gcmFOOTER_ARG("*Float=%d", *Float);
    return gcvSTATUS_OK;

onError:
    gcmFOOTER_NO();
    return gcvSTATUS_INVALID_ARGUMENT;
}

/*******************************************************************************
**
**  gcoOS_StrToInt
**
**  Convert string to integer.
**
**  INPUT:
**
**      gctCONST_STRING String
**          Pointer to the string to be converted.
**
**
**  OUTPUT:
**
**      gctINT * Int
**          Pointer to a variable that will receive the integer.
**
*/
gceSTATUS
gcoOS_StrToInt(
    IN gctCONST_STRING String,
    OUT gctINT * Int
    )
{
    gcmHEADER_ARG("String=%s", gcmOPT_STRING(String));

    /* Verify the arguments. */
    gcmDEBUG_VERIFY_ARGUMENT(String != gcvNULL);

    *Int = (gctINT) atoi(String);

    gcmFOOTER_ARG("*Int=%d", *Int);
    return gcvSTATUS_OK;
}

/*******************************************************************************
**
**  gcoOS_MemCmp
**
**  Compare two memory regions and return whether they match or not.
**
**  INPUT:
**
**      gctCONST_POINTER Memory1
**          Pointer to the first memory region to compare.
**
**      gctCONST_POINTER Memory2
**          Pointer to the second memory region to compare.
**
**      gctSIZE_T Bytes
**          Number of bytes to compare.
**
**  OUTPUT:
**
**      Nothing.
**
**  RETURNS:
**
**      gcvSTATUS_OK if the memory regions match or gcvSTATUS_MISMATCH if the
**      memory regions don't match.
*/
gceSTATUS
gcoOS_MemCmp(
    IN gctCONST_POINTER Memory1,
    IN gctCONST_POINTER Memory2,
    IN gctSIZE_T Bytes
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Memory1=0x%x Memory2=0x%x Bytes=%lu",
                  Memory1, Memory2, Bytes);

    /* Verify the arguments. */
    gcmDEBUG_VERIFY_ARGUMENT(Memory1 != gcvNULL);
    gcmDEBUG_VERIFY_ARGUMENT(Memory2 != gcvNULL);
    gcmDEBUG_VERIFY_ARGUMENT(Bytes > 0);

    /* Compare the memory rregions and return proper status. */
    status = (memcmp(Memory1, Memory2, Bytes) == 0)
               ? gcvSTATUS_OK
               : gcvSTATUS_MISMATCH;
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gcoOS_PrintStr
**
**  Append a "printf" formatted string to a string buffer and adjust the offset
**  into the string buffer.  There is no checking for a buffer overflow, so make
**  sure the string buffer is large enough.
**
**  INPUT:
**
**      gctSTRING String
**          Pointer to the string buffer.
**
**      gctUINT_PTR Offset
**          Pointer to a variable that holds the current offset into the string
**          buffer.
**
**      gctCONST_STRING Format
**          Pointer to a "printf" style format to append to the string buffer
**          pointet to by <String> at the offset specified by <*Offset>.
**
**      ...
**          Variable number of arguments that will be used by <Format>.
**
**  OUTPUT:
**
**      gctUINT_PTR Offset
**          Pointer to a variable that receives the new offset into the string
**          buffer pointed to by <String> after the formatted string pointed to
**          by <Formnat> has been appended to it.
*/
gceSTATUS
gcoOS_PrintStrSafe(
    IN gctSTRING String,
    IN gctSIZE_T StringSize,
    IN OUT gctUINT_PTR Offset,
    IN gctCONST_STRING Format,
    ...
    )
{
    gctARGUMENTS arguments;
    gceSTATUS status = gcvSTATUS_OK;

    gcmHEADER_ARG("String=0x%x StringSize=%lu *Offset=%u Format=0x%x",
                  String, StringSize, gcmOPT_VALUE(Offset), Format);

    /* Verify the arguments. */
    gcmVERIFY_ARGUMENT(String != gcvNULL);
    gcmVERIFY_ARGUMENT(StringSize > 0);
    gcmVERIFY_ARGUMENT(Format != gcvNULL);

    /* Route through gcoOS_PrintStrVSafe. */
    gcmARGUMENTS_START(arguments, Format);
    gcmONERROR(gcoOS_PrintStrVSafe(String, StringSize,
                                   Offset,
                                   Format, arguments));

OnError:
    gcmARGUMENTS_END(arguments);

    gcmFOOTER_ARG("*Offset=%u", gcmOPT_VALUE(Offset));
    return status;
}

/*******************************************************************************
**
**  gcoOS_PrintStrV
**
**  Append a "vprintf" formatted string to a string buffer and adjust the offset
**  into the string buffer.  There is no checking for a buffer overflow, so make
**  sure the string buffer is large enough.
**
**  INPUT:
**
**      gctSTRING String
**          Pointer to the string buffer.
**
**      gctUINT_PTR Offset
**          Pointer to a variable that holds the current offset into the string
**          buffer.
**
**      gctCONST_STRING Format
**          Pointer to a "printf" style format to append to the string buffer
**          pointet to by <String> at the offset specified by <*Offset>.
**
**      gctPOINTER ArgPtr
**          Pointer to list of arguments.
**
**  OUTPUT:
**
**      gctUINT_PTR Offset
**          Pointer to a variable that receives the new offset into the string
**          buffer pointed to by <String> after the formatted string pointed to
**          by <Formnat> has been appended to it.
*/
gceSTATUS
gcoOS_PrintStrVSafe(
    OUT gctSTRING String,
    IN gctSIZE_T StringSize,
    IN OUT gctUINT_PTR Offset,
    IN gctCONST_STRING Format,
    IN gctARGUMENTS Arguments
    )
{
    gctUINT offset = gcmOPT_VALUE(Offset);
    gceSTATUS status = gcvSTATUS_OK;

    gcmHEADER_ARG("String=0x%x StringSize=%lu *Offset=%u Format=0x%x Arguments=0x%x",
                  String, StringSize, offset, Format, Arguments);

    /* Verify the arguments. */
    gcmVERIFY_ARGUMENT(String != gcvNULL);
    gcmVERIFY_ARGUMENT(StringSize > 0);
    gcmVERIFY_ARGUMENT(Format != gcvNULL);

    if (offset < StringSize - 1)
    {
        /* Print into the string. */
        gctINT n = vsnprintf(String + offset,
                             StringSize - offset,
                             Format,
                             Arguments);

        if (n < 0 || n >= (gctINT)(StringSize - offset))
        {
            status = gcvSTATUS_GENERIC_IO;
        }
        else if (Offset)
        {
            *Offset = offset + n;
        }
    }
    else
    {
        status = gcvSTATUS_BUFFER_TOO_SMALL;
    }

    /* Success. */
    gcmFOOTER_ARG("*Offset=%u", offset);
    return status;
}

/*******************************************************************************
**
**  gcoOS_StrDup
**
**  Duplicate the given string by copying it into newly allocated memory.
**
**  INPUT:
**
**      gcoOS Os
**          Pointer to gcoOS object.
**
**      gctCONST_STRING String
**          Pointer to string to duplicate.
**
**  OUTPUT:
**
**      gctSTRING * Target
**          Pointer to variable holding the duplicated string address.
*/
gceSTATUS
gcoOS_StrDup(
    IN gcoOS Os,
    IN gctCONST_STRING String,
    OUT gctSTRING * Target
    )
{
    gctSIZE_T bytes;
    gctSTRING string;
    gceSTATUS status;

    gcmHEADER_ARG("String=0x%x", String);

    gcmDEBUG_VERIFY_ARGUMENT(String != gcvNULL);
    gcmDEBUG_VERIFY_ARGUMENT(Target != gcvNULL);

    bytes = gcoOS_StrLen(String, gcvNULL);

    gcmONERROR(gcoOS_Allocate(gcvNULL, bytes + 1, (gctPOINTER *) &string));

    memcpy(string, String, bytes + 1);

    *Target = string;

    /* Success. */
    gcmFOOTER_ARG("*Target=0x%x", gcmOPT_VALUE(Target));
    return gcvSTATUS_OK;

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gcoOS_LoadLibrary
**
**  Load a library.
**
**  INPUT:
**
**      gcoOS Os
**          Pointer to gcoOS object.
**
**      gctCONST_STRING Library
**          Name of library to load.
**
**  OUTPUT:
**
**      gctHANDLE * Handle
**          Pointer to variable receiving the library handle.
*/

gceSTATUS
gcoOS_LoadLibrary(
    IN gcoOS Os,
    IN gctCONST_STRING Library,
    OUT gctHANDLE * Handle
    )
{
#if gcdSTATIC_LINK
    return gcvSTATUS_NOT_SUPPORTED;
#else
    gctSIZE_T length;
    gctSTRING library = gcvNULL;
    gceSTATUS status = gcvSTATUS_OK;

    gcmHEADER_ARG("Library=%s", gcmOPT_STRING(Library));

    /* Verify the arguments. */
    gcmDEBUG_VERIFY_ARGUMENT(Handle != gcvNULL);

    /* Reset the handle. */
    *Handle = gcvNULL;

    if (Library != gcvNULL)
    {
        /* Get the length of the library name. */
        length = strlen(Library);

        /* Test if the libray has ".so" at the end. */
        if (strcmp(Library + length - 3, ".so") != 0)
        {
            /* Allocate temporay string buffer. */
            gcmONERROR(gcoOS_Allocate(
                gcvNULL, length + 4 + 1, (gctPOINTER *) &library
                ));

            /* Copy the library name to the temporary string buffer. */
            strcpy(library, Library);

            /* Append the ".so" to the temporary string buffer. */
            strcat(library, ".so");

            /* Replace the library name. */
            Library = library;
        }

        *Handle = dlopen(Library, RTLD_NOW);

        /* Failed? */
        if (*Handle == gcvNULL)
        {
            gcmTRACE(
                gcvLEVEL_VERBOSE, "%s(%d): %s", __FUNCTION__, __LINE__, dlerror()
                );

            /* Library could not be loaded. */
            status = gcvSTATUS_NOT_FOUND;
        }
    }

OnError:
    /* Free the temporary string buffer. */
    if (library != gcvNULL)
    {
        gcmVERIFY_OK(gcmOS_SAFE_FREE(gcvNULL, library));
    }

    gcmFOOTER_ARG("*Handle=0x%x status=%d", *Handle, status);
    return status;
#endif
}

/*******************************************************************************
**
**  gcoOS_FreeLibrary
**
**  Unload a loaded library.
**
**  INPUT:
**
**      gcoOS Os
**          Pointer to gcoOS object.
**
**      gctHANDLE Handle
**          Handle of a loaded libarry.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gcoOS_FreeLibrary(
    IN gcoOS Os,
    IN gctHANDLE Handle
    )
{
#if gcdSTATIC_LINK
    return gcvSTATUS_NOT_SUPPORTED;
#else
    gcmHEADER_ARG("Handle=0x%x", Handle);

#if !gcdBUILT_FOR_VALGRIND
    /* Free the library. */
    dlclose(Handle);
#endif

    /* Success. */
    gcmFOOTER_NO();
    return gcvSTATUS_OK;
#endif
}

/*******************************************************************************
**
**  gcoOS_GetProcAddress
**
**  Get the address of a function inside a loaded library.
**
**  INPUT:
**
**      gcoOS Os
**          Pointer to gcoOS object.
**
**      gctHANDLE Handle
**          Handle of a loaded libarry.
**
**      gctCONST_STRING Name
**          Name of function to get the address of.
**
**  OUTPUT:
**
**      gctPOINTER * Function
**          Pointer to variable receiving the function pointer.
*/
gceSTATUS
gcoOS_GetProcAddress(
    IN gcoOS Os,
    IN gctHANDLE Handle,
    IN gctCONST_STRING Name,
    OUT gctPOINTER * Function
    )
{
#if gcdSTATIC_LINK
    return gcvSTATUS_NOT_SUPPORTED;
#else
    gceSTATUS status = gcvSTATUS_OK;

    gcmHEADER_ARG("Handle=0x%x Name=%s", Handle, gcmOPT_STRING(Name));

    /* Verify the arguments. */
    gcmDEBUG_VERIFY_ARGUMENT(Name != gcvNULL);
    gcmDEBUG_VERIFY_ARGUMENT(Function != gcvNULL);

    /* Get the address of the function. */
    *Function = dlsym(Handle, Name);

    if (*Function == gcvNULL)
    {
        gcmTRACE(
            gcvLEVEL_WARNING,
            "%s(%d): Function %s not found.",
            __FUNCTION__, __LINE__, Name
            );

        /* Function could not be found. */
        status = gcvSTATUS_NOT_FOUND;
    }

    /* Success. */
    gcmFOOTER_ARG("*Function=0x%x status=%d", *Function, status);
    return status;
#endif
}

#if VIVANTE_PROFILER_SYSTEM_MEMORY
gceSTATUS
gcoOS_ProfileStart(
    IN gcoOS Os
    )
{
    gcPLS.os->allocCount   = 0;
    gcPLS.os->allocSize    = 0;
    gcPLS.os->maxAllocSize = 0;
    gcPLS.os->freeCount    = 0;
    gcPLS.os->freeSize     = 0;

#if gcdGC355_MEM_PRINT
    gcPLS.os->oneRecording = 0;
    gcPLS.os->oneSize      = 0;
#endif

    return gcvSTATUS_OK;
}

gceSTATUS
gcoOS_ProfileEnd(
    IN gcoOS Os,
    IN gctCONST_STRING Title
    )
{
    gcmPRINT("10) System memory - current: %u \n", gcPLS.os->allocSize);
    gcmPRINT("11) System memory - maximum: %u \n", gcPLS.os->maxAllocSize);
    gcmPRINT("12) System memory - total: %u \n", (gcPLS.os->allocSize + gcPLS.os->freeSize));
    gcmPRINT("13) System memory - allocation count: %u \n", gcPLS.os->allocCount);
    gcmPRINT("14) System memory - deallocation count: %u \n", gcPLS.os->freeCount);

    return gcvSTATUS_OK;
}

/*******************************************************************************
**
**  gcoOS_SetProfileSetting
**
**  Set Vivante profiler settings.
**
**  INPUT:
**
**      gcoOS Os
**          Pointer to an gcoOS object.
**
**      gctBOOL Enable
**          Enable or Disable Vivante profiler.
**
**      gctCONST_STRING FileName
**          Specify FileName for storing profile data into.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gcoOS_SetProfileSetting(
    IN gcoOS Os,
    IN gctBOOL Enable,
    IN gctCONST_STRING FileName
    )
{
    gcsHAL_INTERFACE iface;

    if (strlen(FileName) >= gcdMAX_PROFILE_FILE_NAME)
    {
        return gcvSTATUS_INVALID_ARGUMENT;
    }

    /* Initialize the gcsHAL_INTERFACE structure. */
    iface.ignoreTLS = gcvFALSE;
    iface.command = gcvHAL_SET_PROFILE_SETTING;
    iface.u.SetProfileSetting.enable = Enable;

    /* Call the kernel. */
    return gcoOS_DeviceControl(gcvNULL,
                               IOCTL_GCHAL_INTERFACE,
                               &iface, gcmSIZEOF(iface),
                               &iface, gcmSIZEOF(iface));
}
#endif

gceSTATUS
gcoOS_Compact(
    IN gcoOS Os
    )
{
    return gcvSTATUS_OK;
}

/*----------------------------------------------------------------------------*/
/*----------------------------------- Atoms ----------------------------------*/
/* Create an atom. */
gceSTATUS
gcoOS_AtomConstruct(
    IN gcoOS Os,
    OUT gcsATOM_PTR * Atom
    )
{
    gceSTATUS status;
    gcsATOM_PTR atom = gcvNULL;

    gcmHEADER();

    /* Verify the arguments. */
    gcmDEBUG_VERIFY_ARGUMENT(Atom != gcvNULL);

    do
    {
        /* Allocate memory for the atom. */
        gcmERR_BREAK(gcoOS_Allocate(gcvNULL,
                                    gcmSIZEOF(struct gcsATOM),
                                    (gctPOINTER *) &atom));

        /* Initialize the atom to 0. */
        atom->counter = 0;

#if !gcdBUILTIN_ATOMIC_FUNCTIONS
        if (pthread_mutex_init(&atom->mutex, gcvNULL) != 0)
        {
            status = gcvSTATUS_OUT_OF_RESOURCES;
            break;
        }
#endif

        /* Return pointer to atom. */
        *Atom = atom;

        /* Success. */
        gcmFOOTER_ARG("*Atom=%p", *Atom);
        return gcvSTATUS_OK;
    }
    while (gcvFALSE);

    /* Free the atom. */
    if (atom != gcvNULL)
    {
        gcmOS_SAFE_FREE(gcvNULL, atom);
    }

    /* Return error status. */
    gcmFOOTER();
    return status;
}

/* Destroy an atom. */
gceSTATUS
gcoOS_AtomDestroy(
    IN gcoOS Os,
    IN gcsATOM_PTR Atom
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Atom=0x%x", Atom);

    /* Verify the arguments. */
    gcmDEBUG_VERIFY_ARGUMENT(Atom != gcvNULL);

    /* Free the atom. */
    status = gcmOS_SAFE_FREE(gcvNULL, Atom);

    /* Return the status. */
    gcmFOOTER();
    return status;
}

gceSTATUS
gcoOS_AtomGet(
    IN gcoOS Os,
    IN gcsATOM_PTR Atom,
    OUT gctINT32_PTR Value
    )
{
    gcmHEADER_ARG("Atom=0x%0x", Atom);

    /* Verify the arguments. */
    gcmVERIFY_ARGUMENT(Atom != gcvNULL);

    /* Get the atom value. */
    *Value = Atom->counter;

    /* Success. */
    gcmFOOTER_ARG("*Value=%d", *Value);
    return gcvSTATUS_OK;
}

gceSTATUS
gcoOS_AtomSet(
    IN gcoOS Os,
    IN gcsATOM_PTR Atom,
    IN gctINT32 Value
    )
{
    gcmHEADER_ARG("Atom=0x%0x Value=%d", Atom, Value);

    /* Verify the arguments. */
    gcmVERIFY_ARGUMENT(Atom != gcvNULL);

    /* Set the atom value. */
    Atom->counter = Value;

    /* Success. */
    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

/* Increment an atom. */
gceSTATUS
gcoOS_AtomIncrement(
    IN gcoOS Os,
    IN gcsATOM_PTR Atom,
    OUT gctINT32_PTR OldValue OPTIONAL
    )
{
    gctINT32 value;

    gcmHEADER_ARG("Atom=0x%x", Atom);

    /* Verify the arguments. */
    gcmDEBUG_VERIFY_ARGUMENT(Atom != gcvNULL);

    /* Increment the atom's counter. */
#if gcdBUILTIN_ATOMIC_FUNCTIONS
    value = __sync_fetch_and_add(&Atom->counter, 1);
#else
    /* Lock the mutex. */
    pthread_mutex_lock(&Atom->mutex);

    /* Get original value. */
    value = Atom->counter;

    /* Add given value. */
    Atom->counter += 1;

    /* Unlock the mutex. */
    pthread_mutex_unlock(&Atom->mutex);
#endif

    if (OldValue != gcvNULL)
    {
        /* Return the original value to the caller. */
        *OldValue = value;
    }

    /* Success. */
    gcmFOOTER_ARG("*OldValue=%d", gcmOPT_VALUE(OldValue));
    return gcvSTATUS_OK;
}

/* Decrement an atom. */
gceSTATUS
gcoOS_AtomDecrement(
    IN gcoOS Os,
    IN gcsATOM_PTR Atom,
    OUT gctINT32_PTR OldValue OPTIONAL
    )
{
    gctINT32 value;

    gcmHEADER_ARG("Atom=0x%x", Atom);

    /* Verify the arguments. */
    gcmDEBUG_VERIFY_ARGUMENT(Atom != gcvNULL);

    /* Decrement the atom's counter. */
#if gcdBUILTIN_ATOMIC_FUNCTIONS
    value = __sync_fetch_and_sub(&Atom->counter, 1);
#else
    /* Lock the mutex. */
    pthread_mutex_lock(&Atom->mutex);

    /* Get original value. */
    value = Atom->counter;

    /* Subtract given value. */
    Atom->counter -= 1;

    /* Unlock the mutex. */
    pthread_mutex_unlock(&Atom->mutex);
#endif

    if (OldValue != gcvNULL)
    {
        /* Return the original value to the caller. */
        *OldValue = value;
    }

    /* Success. */
    gcmFOOTER_ARG("*OldValue=%d", gcmOPT_VALUE(OldValue));
    return gcvSTATUS_OK;
}

gctHANDLE
gcoOS_GetCurrentProcessID(
    void
    )
{
    return (gctHANDLE)(gctUINTPTR_T) taskIdSelf();
}

gctHANDLE
gcoOS_GetCurrentThreadID(
    void
    )
{
    return (gctHANDLE) pthread_self();
}

/*----------------------------------------------------------------------------*/
/*----------------------------------- Time -----------------------------------*/

/*******************************************************************************
**
**  gcoOS_GetTicks
**
*/
gctUINT32
gcoOS_GetTicks(
    void
    )
{
    return 0;
}

/*******************************************************************************
**
**  gcoOS_GetTime
**
**  Get the number of ueconds since 1970/1/1.
**
**  INPUT:
**
**  OUTPUT:
**
**      gctUINT64_PTR Time
**          Pointer to a variable to get time.
**
*/
gceSTATUS
gcoOS_GetTime(
    OUT gctUINT64_PTR Time
    )
{
    struct timespec tv;

    clock_gettime(CLOCK_REALTIME, &tv);

    *Time = (tv.tv_sec * 1000000) + (tv.tv_nsec + 500 / 1000);
    return gcvSTATUS_OK;
}

/*******************************************************************************
**
**  gcoOS_GetCPUTime
**
**  Get CPU time usage in microseconds.
**
**  INPUT:
**
**  OUTPUT:
**
**      gctUINT64_PTR CPUTime
**          Pointer to a variable to get CPU time usage.
**
*/
gceSTATUS
gcoOS_GetCPUTime(
    OUT gctUINT64_PTR CPUTime
    )
{
    return gcvSTATUS_OK;
}

/*******************************************************************************
**
**  gcoOS_GetMemoryUsage
**
**  Get current processes resource usage.
**
**  INPUT:
**
**  OUTPUT:
**
**      gctUINT32_PTR MaxRSS
**          Total amount of resident set memory used.
**          The value will be in terms of memory pages used.
**
**      gctUINT32_PTR IxRSS
**          Total amount of memory used by the text segment
**          in kilobytes multiplied by the execution-ticks.
**
**      gctUINT32_PTR IdRSS
**          Total amount of private memory used by a process
**          in kilobytes multiplied by execution-ticks.
**
**      gctUINT32_PTR IsRSS
**          Total amount of memory used by the stack in
**          kilobytes multiplied by execution-ticks.
**
*/
gceSTATUS
gcoOS_GetMemoryUsage(
    OUT gctUINT32_PTR MaxRSS,
    OUT gctUINT32_PTR IxRSS,
    OUT gctUINT32_PTR IdRSS,
    OUT gctUINT32_PTR IsRSS
    )
{
    return gcvSTATUS_OK;
}

/*******************************************************************************
**
**  gcoOS_ReadRegister
**
**  Read data from a register.
**
**  INPUT:
**
**      gcoOS Os
**          Pointer to an gcoOS object.
**
**      gctUINT32 Address
**          Address of register.
**
**  OUTPUT:
**
**      gctUINT32 * Data
**          Pointer to a variable that receives the data read from the register.
*/
gceSTATUS
gcoOS_ReadRegister(
    IN gcoOS Os,
    IN gctUINT32 Address,
    OUT gctUINT32 * Data
    )
{
    gcsHAL_INTERFACE iface;
    gceSTATUS status;

    gcmHEADER_ARG("Address=0x%x", Address);

    /* Verify the arguments. */
    gcmDEBUG_VERIFY_ARGUMENT(Data != gcvNULL);

    /* Initialize the gcsHAL_INTERFACE structure. */
    iface.ignoreTLS = gcvFALSE;
    iface.command = gcvHAL_READ_REGISTER;
    iface.u.ReadRegisterData.address = Address;
    iface.u.ReadRegisterData.data    = 0xDEADDEAD;

    /* Call kernel driver. */
    gcmONERROR(gcoOS_DeviceControl(gcvNULL,
                                   IOCTL_GCHAL_INTERFACE,
                                   &iface, gcmSIZEOF(iface),
                                   &iface, gcmSIZEOF(iface)));

    /* Return the Data on success. */
    *Data = iface.u.ReadRegisterData.data;

    /* Success. */
    gcmFOOTER_ARG("*Data=0x%08x", *Data);
    return gcvSTATUS_OK;

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gcoOS_WriteRegister
**
**  Write data to a register.
**
**  INPUT:
**
**      gcoOS Os
**          Pointer to an gcoOS object.
**
**      gctUINT32 Address
**          Address of register.
**
**      gctUINT32 Data
**          Data for register.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gcoOS_WriteRegister(
    IN gcoOS Os,
    IN gctUINT32 Address,
    IN gctUINT32 Data
    )
{
    gcsHAL_INTERFACE iface;
    gceSTATUS status;

    gcmHEADER_ARG("Address=0x%x Data=0x%08x", Address, Data);

    /* Initialize the gcsHAL_INTERFACE structure. */
    iface.ignoreTLS = gcvFALSE;
    iface.command = gcvHAL_WRITE_REGISTER;
    iface.u.WriteRegisterData.address = Address;
    iface.u.WriteRegisterData.data    = Data;

    /* Call kernel driver. */
    gcmONERROR(gcoOS_DeviceControl(gcvNULL,
                                   IOCTL_GCHAL_INTERFACE,
                                   &iface, gcmSIZEOF(iface),
                                   &iface, gcmSIZEOF(iface)));

    /* Success. */
    gcmFOOTER_NO();
    return gcvSTATUS_OK;

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}

static gceSTATUS
gcoOS_Cache(
    IN gctUINT32 Node,
    IN gctPOINTER Logical,
    IN gctSIZE_T Bytes,
    IN gceCACHEOPERATION Operation
    )
{
    gcsHAL_INTERFACE ioctl;
    gceSTATUS status;

    gcmHEADER_ARG("Node=0x%x Logical=0x%x Bytes=%u Operation=%d",
                  Node, Logical, Bytes, Operation);

    /* Verify the arguments. */
    gcmDEBUG_VERIFY_ARGUMENT(Logical != gcvNULL);
    gcmDEBUG_VERIFY_ARGUMENT(Bytes > 0);

    /* Set up the cache. */
    ioctl.ignoreTLS          = gcvFALSE;
    ioctl.command            = gcvHAL_CACHE;
    ioctl.u.Cache.operation  = Operation;
    ioctl.u.Cache.node       = Node;
    ioctl.u.Cache.logical    = gcmPTR_TO_UINT64(Logical);
    ioctl.u.Cache.bytes      = Bytes;
    ioctl.u.Cache.offset     = Offset;

    /* Call the kernel. */
    gcmONERROR(gcoOS_DeviceControl(gcvNULL,
                                   IOCTL_GCHAL_INTERFACE,
                                   &ioctl, gcmSIZEOF(ioctl),
                                   &ioctl, gcmSIZEOF(ioctl)));

    /* Success. */
    gcmFOOTER_NO();
    return gcvSTATUS_OK;

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**  gcoOS_CacheClean
**
**  Clean the cache for the specified addresses.  The GPU is going to need the
**  data.  If the system is allocating memory as non-cachable, this function can
**  be ignored.
**
**  ARGUMENTS:
**
**      gcoOS Os
**          Pointer to gcoOS object.
**
**      gctUINT32 Node
**          Pointer to the video memory node that needs to be flushed.
**
**      gctPOINTER Logical
**          Logical address to flush.
**
**      gctSIZE_T Bytes
**          Size of the address range in bytes to flush.
*/
gceSTATUS
gcoOS_CacheClean(
    IN gcoOS Os,
    IN gctUINT32 Node,
    IN gctPOINTER Logical,
    IN gctSIZE_T Bytes
    )
{
    return gcoOS_CacheCleanEx(Os, Node, Logical, 0, Bytes);
}

gceSTATUS
gcoOS_CacheCleanEx(
    IN gcoOS Os,
    IN gctUINT32 Node,
    IN gctPOINTER Logical,
    IN gctSIZE_T Offset,
    IN gctSIZE_T Bytes
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Node=0x%x Logical=0x%x Offset=0x%zx Bytes=0x%zx",
                  Node, Logical, Offset, Bytes);

    /* Call common code. */
    gcmONERROR(gcoOS_Cache(Node, Logical, Offset, Bytes, gcvCACHE_CLEAN));

    /* Success. */
    gcmFOOTER_NO();
    return gcvSTATUS_OK;

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**  gcoOS_CacheFlush
**
**  Flush the cache for the specified addresses and invalidate the lines as
**  well.  The GPU is going to need and modify the data.  If the system is
**  allocating memory as non-cachable, this function can be ignored.
**
**  ARGUMENTS:
**
**      gcoOS Os
**          Pointer to gcoOS object.
**
**      gctUINT32 Node
**          Pointer to the video memory node that needs to be flushed.
**
**      gctPOINTER Logical
**          Logical address to flush.
**
**      gctSIZE_T Bytes
**          Size of the address range in bytes to flush.
*/
gceSTATUS
gcoOS_CacheFlush(
    IN gcoOS Os,
    IN gctUINT32 Node,
    IN gctPOINTER Logical,
    IN gctSIZE_T Bytes
    )
{
    return gcoOS_CacheFlushEx(Os, Node, Logical, 0, Bytes);
}

gceSTATUS
gcoOS_CacheFlushEx(
    IN gcoOS Os,
    IN gctUINT32 Node,
    IN gctPOINTER Logical,
    IN gctSIZE_T Offset,
    IN gctSIZE_T Bytes
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Node=0x%x Logical=0x%x Offset=0x%zx Bytes=0x%zx",
                  Node, Logical, Offset, Bytes);

    /* Call common code. */
    gcmONERROR(gcoOS_Cache(Node, Logical, Offset, Bytes, gcvCACHE_FLUSH));

    /* Success. */
    gcmFOOTER_NO();
    return gcvSTATUS_OK;

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}
/*******************************************************************************
**  gcoOS_CacheInvalidate
**
**  Invalidate the lines. The GPU is going modify the data.  If the system is
**  allocating memory as non-cachable, this function can be ignored.
**
**  ARGUMENTS:
**
**      gcoOS Os
**          Pointer to gcoOS object.
**
**      gctUINT32 Node
**          Pointer to the video memory node that needs to be invalidated.
**
**      gctPOINTER Logical
**          Logical address to flush.
**
**      gctSIZE_T Bytes
**          Size of the address range in bytes to invalidated.
*/
gceSTATUS
gcoOS_CacheInvalidate(
    IN gcoOS Os,
    IN gctUINT32 Node,
    IN gctPOINTER Logical,
    IN gctSIZE_T Bytes
    )
{
    return gcoOS_CacheInvalidateEx(Os, Node, Logical, 0, Bytes);
}

gceSTATUS
gcoOS_CacheInvalidateEx(
    IN gcoOS Os,
    IN gctUINT32 Node,
    IN gctPOINTER Logical,
    IN gctSIZE_T Offset,
    IN gctSIZE_T Bytes
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Node=0x%x Logical=0x%x Offset=0x%zx Bytes=0x%zx",
                  Node, Logical, Offset, Bytes);

    /* Call common code. */
    gcmONERROR(gcoOS_Cache(Node, Logical, Offset, Bytes, gcvCACHE_INVALIDATE));

    /* Success. */
    gcmFOOTER_NO();
    return gcvSTATUS_OK;

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**  gcoOS_MemoryBarrier
**  Make sure the CPU has executed everything up to this point and the data got
**  written to the specified pointer.
**  ARGUMENTS:
**
**      gcoOS Os
**          Pointer to gcoOS object.
**
**      gctPOINTER Logical
**          Logical address to flush.
**
*/
gceSTATUS
gcoOS_MemoryBarrier(
    IN gcoOS Os,
    IN gctPOINTER Logical
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Logical=0x%x", Logical);

    /* Call common code. */
    gcmONERROR(gcoOS_Cache(0, Logical, 0, 1, gcvCACHE_MEMORY_BARRIER));

    /* Success. */
    gcmFOOTER_NO();
    return gcvSTATUS_OK;

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}


/*----------------------------------------------------------------------------*/
/*----- Profiling ------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/

#if gcdENABLE_PROFILING
gceSTATUS
gcoOS_GetProfileTick(
    OUT gctUINT64_PTR Tick
    )
{
#ifdef CLOCK_MONOTONIC
    struct timespec time;

    clock_gettime(CLOCK_MONOTONIC, &time);

    *Tick = time.tv_nsec + time.tv_sec * 1000000000ULL;

    return gcvSTATUS_OK;
#else
    return gcvSTATUS_NOT_SUPPORTED;
#endif
}

gceSTATUS
gcoOS_QueryProfileTickRate(
    OUT gctUINT64_PTR TickRate
    )
{
#ifdef CLOCK_MONOTONIC
    struct timespec res;

    clock_getres(CLOCK_MONOTONIC, &res);

    *TickRate = res.tv_nsec + res.tv_sec * 1000000000ULL;

    return gcvSTATUS_OK;
#else
    return gcvSTATUS_NOT_SUPPORTED;
#endif
}

/*******************************************************************************
**  gcoOS_ProfileDB
**
**  Manage the profile database.
**
**  The database layout is very simple:
**
**      <RecordID> (1 byte) <record data>
**
**  The <RecordID> can be one of the following values:
**
**      1       Initialize a new function to be profiled. Followed by the NULL-
**              terminated name of the function, 4 bytes of the function ID and
**              8 bytes of the profile tick.
**      2       Enter a function to be profiled. Followed by 4 bytes of function
**              ID and 8 bytes of the profile tick.
**      3       Exit a function to be profiled. Followed by 8 bytes of the
**              profile tick.
**
**  There are three options to manage the profile database. One would be to
**  enter a function that needs to be profiled. This is specified with both
**  <Function> and <Initialized> pointers initialized to some value. Here
**  <Function> is pointing to a string with the function name and <Initialized>
**  is pointing to a boolean value that tells the profiler whether this function
**  has been initialized or not.
**
**  The second option would be to exit a function that was being profiled. This
**  is specified by <Function> pointing to a string with the function name and
**  <Initialized> set to gcvNULL.
**
**  The third and last option is to flush the profile database. This is
**  specified with <Function> set to gcvNULL.
**
***** PARAMETERS
**
**  Function
**
**      Pointer to a string with the function name being profiled or gcvNULL to
**      flush the profile database.
**
**  Initialized
**
**      Pointer to a boolean variable that informs the profiler if the entry of
**      a function has been initialized or not, or gcvNULL to mark the exit of a
**      function being profiled.
*/
void
gcoOS_ProfileDB(
    IN gctCONST_STRING Function,
    IN OUT gctBOOL_PTR Initialized
    )
{
    gctUINT64 nanos;
    static gctUINT8_PTR profileBuffer = gcvNULL;
    static gctSIZE_T profileSize, profileThreshold, totalBytes;
    static gctUINT32 profileIndex;
    static gctINT profileLevel;
    static FILE * profileDB = gcvNULL;
    int len, bytes;

    /* Check if we need to flush the profile database. */
    if (Function == gcvNULL)
    {
        if (profileBuffer != gcvNULL)
        {
            /* Check of the profile database exists. */
            if (profileIndex > 0)
            {
                if (profileDB == gcvNULL)
                {
                    /* Open the profile database file. */
                    profileDB = fopen("profile.database", "wb");
                }

                if (profileDB != gcvNULL)
                {
                    /* Write the profile database to the file. */
                    totalBytes += fwrite(profileBuffer,
                                         1, profileIndex,
                                         profileDB);
                }
            }

            if (profileDB != gcvNULL)
            {
                /* Convert the size of the profile database into a nice human
                ** readable format. */
                char buf[] = "#,###,###,###";
                int i;

                i = strlen(buf);
                while ((totalBytes != 0) && (i > 0))
                {
                    if (buf[--i] == ',') --i;

                    buf[i]      = '0' + (totalBytes % 10);
                    totalBytes /= 10;
                }

                /* Print the size of the profile database. */
                gcmPRINT("Closing the profile database: %s bytes.", &buf[i]);

                /* Close the profile database file. */
                fclose(profileDB);
                profileDB = gcvNULL;
            }

            /* Destroy the profile database. */
            free(profileBuffer);
            profileBuffer = gcvNULL;
        }
    }

    /* Check if we have to enter a function. */
    else if (Initialized != gcvNULL)
    {
        /* Check if the profile database exists or not. */
        if (profileBuffer == gcvNULL)
        {
            /* Allocate the profile database. */
            for (profileSize = 32 << 20;
                 profileSize > 0;
                 profileSize -= 1 << 20
            )
            {
                profileBuffer = malloc(profileSize);

                if (profileBuffer != gcvNULL)
                {
                    break;
                }
            }

            if (profileBuffer == gcvNULL)
            {
                /* Sorry - no memory. */
                gcmPRINT("Cannot create the profile buffer!");
                return;
            }

            /* Reset the profile database. */
            profileThreshold = gcmMIN(profileSize / 2, 4 << 20);
            totalBytes       = 0;
            profileIndex     = 0;
            profileLevel     = 0;
        }

        /* Increment the profile level. */
        ++profileLevel;

        /* Determine number of bytes to copy. */
        len   = strlen(Function) + 1;
        bytes = 1 + (*Initialized ? 0 : len) + 4 + 8;

        /* Check if the profile database has enough space. */
        if (profileIndex + bytes > profileSize)
        {
            gcmPRINT("PROFILE ENTRY: index=%lu size=%lu bytes=%d level=%d",
                     profileIndex, profileSize, bytes, profileLevel);

            if (profileDB == gcvNULL)
            {
                /* Open the profile database file. */
                profileDB = fopen("profile.database", "wb");
            }

            if (profileDB != gcvNULL)
            {
                /* Write the profile database to the file. */
                totalBytes += fwrite(profileBuffer, 1, profileIndex, profileDB);
            }

            /* Empty the profile databse. */
            profileIndex = 0;
        }

        /* Check whether this function is initialized or not. */
        if (*Initialized)
        {
            /* Already initialized - don't need to save name. */
            profileBuffer[profileIndex] = 2;
        }
        else
        {
            /* Not yet initialized, save name as well. */
            profileBuffer[profileIndex] = 1;
            memcpy(profileBuffer + profileIndex + 1, Function, len);
            profileIndex += len;

            /* Mark function as initialized. */
            *Initialized = gcvTRUE;
        }

        /* Copy the function ID into the profile database. */
        memcpy(profileBuffer + profileIndex + 1, &Initialized, 4);

        /* Get the profile tick. */
        gcoOS_GetProfileTick(&nanos);

        /* Copy the profile tick into the profile database. */
        memcpy(profileBuffer + profileIndex + 5, &nanos, 8);
        profileIndex += 1 + 4 + 8;
    }

    /* Exit a function, check whether the profile database is around. */
    else if (profileBuffer != gcvNULL)
    {
        /* Get the profile tick. */
        gcoOS_GetProfileTick(&nanos);

        /* Check if the profile database has enough space. */
        if (profileIndex + 1 + 8 > profileSize)
        {
            gcmPRINT("PROFILE EXIT: index=%lu size=%lu bytes=%d level=%d",
                     profileIndex, profileSize, 1 + 8, profileLevel);

            if (profileDB == gcvNULL)
            {
                /* Open the profile database file. */
                profileDB = fopen("profile.database", "wb");
            }

            if (profileDB != gcvNULL)
            {
                /* Write the profile database to the file. */
                totalBytes += fwrite(profileBuffer, 1, profileIndex, profileDB);
            }

            /* Empty the profile databse. */
            profileIndex = 0;
        }

        /* Copy the profile tick into the profile database. */
        profileBuffer[profileIndex] = 3;
        memcpy(profileBuffer + profileIndex + 1, &nanos, 8);
        profileIndex += 1 + 8;

        /* Decrease the profile level and check whether the profile database is
        ** getting too big if we exit a top-level function. */
        if ((--profileLevel == 0)
        &&  (profileSize - profileIndex < profileThreshold)
        )
        {
            if (profileDB == gcvNULL)
            {
                /* Open the profile database file. */
                profileDB = fopen("profile.database", "wb");
            }

            if (profileDB != gcvNULL)
            {
                /* Write the profile database to the file. */
                totalBytes += fwrite(profileBuffer, 1, profileIndex, profileDB);

                /* Flush the file now. */
                fflush(profileDB);
            }

            /* Empty the profile databse. */
            profileIndex = 0;
        }
    }
}
#endif

/******************************************************************************\
******************************* Signal Management ******************************
\******************************************************************************/

#undef _GC_OBJ_ZONE
#define _GC_OBJ_ZONE    gcvZONE_SIGNAL

/*******************************************************************************
**
**  gcoOS_CreateSignal
**
**  Create a new signal.
**
**  INPUT:
**
**      gcoOS Os
**          Pointer to an gcoOS object.
**
**      gctBOOL ManualReset
**          If set to gcvTRUE, gcoOS_Signal with gcvFALSE must be called in
**          order to set the signal to nonsignaled state.
**          If set to gcvFALSE, the signal will automatically be set to
**          nonsignaled state by gcoOS_WaitSignal function.
**
**  OUTPUT:
**
**      gctSIGNAL * Signal
**          Pointer to a variable receiving the created gctSIGNAL.
*/
gceSTATUS
gcoOS_CreateSignal(
    IN gcoOS Os,
    IN gctBOOL ManualReset,
    OUT gctSIGNAL * Signal
    )
{
    gceSTATUS status;
    gcsHAL_INTERFACE iface;

    gcmHEADER_ARG("ManualReset=%d", ManualReset);

    /* Verify the arguments. */
    gcmDEBUG_VERIFY_ARGUMENT(Signal != gcvNULL);

    /* Initialize the gcsHAL_INTERFACE structure. */
    iface.ignoreTLS = gcvFALSE;
    iface.command = gcvHAL_USER_SIGNAL;
    iface.u.UserSignal.command     = gcvUSER_SIGNAL_CREATE;
    iface.u.UserSignal.manualReset = ManualReset;

    /* Call kernel driver. */
    gcmONERROR(gcoOS_DeviceControl(gcvNULL,
                                   IOCTL_GCHAL_INTERFACE,
                                   &iface, gcmSIZEOF(iface),
                                   &iface, gcmSIZEOF(iface)));

    *Signal = (gctSIGNAL)(gctUINTPTR_T) iface.u.UserSignal.id;

    gcmFOOTER_ARG("*Signal=0x%x", *Signal);
    return gcvSTATUS_OK;

OnError:
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gcoOS_DestroySignal
**
**  Destroy a signal.
**
**  INPUT:
**
**      gcoOS Os
**          Pointer to an gcoOS object.
**
**      gctSIGNAL Signal
**          Pointer to the gctSIGNAL.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gcoOS_DestroySignal(
    IN gcoOS Os,
    IN gctSIGNAL Signal
    )
{
    gceSTATUS status;
    gcsHAL_INTERFACE iface;

    gcmHEADER_ARG("Signal=0x%x", Signal);

    gcmTRACE_ZONE(
        gcvLEVEL_VERBOSE, gcvZONE_OS,
        "gcoOS_DestroySignal: signal->%d.",
        (gctINT)(gctUINTPTR_T)Signal
        );

    /* Initialize the gcsHAL_INTERFACE structure. */
    iface.ignoreTLS = gcvFALSE;
    iface.command = gcvHAL_USER_SIGNAL;
    iface.u.UserSignal.command = gcvUSER_SIGNAL_DESTROY;
    iface.u.UserSignal.id      = (gctINT)(gctUINTPTR_T) Signal;

    /* Call kernel driver. */
    status = gcoOS_DeviceControl(gcvNULL,
                                 IOCTL_GCHAL_INTERFACE,
                                 &iface, gcmSIZEOF(iface),
                                 &iface, gcmSIZEOF(iface));

    /* Success. */
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gcoOS_Signal
**
**  Set a state of the specified signal.
**
**  INPUT:
**
**      gcoOS Os
**          Pointer to an gcoOS object.
**
**      gctSIGNAL Signal
**          Pointer to the gctSIGNAL.
**
**      gctBOOL State
**          If gcvTRUE, the signal will be set to signaled state.
**          If gcvFALSE, the signal will be set to nonsignaled state.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gcoOS_Signal(
    IN gcoOS Os,
    IN gctSIGNAL Signal,
    IN gctBOOL State
    )
{
    gceSTATUS status;
    gcsHAL_INTERFACE iface;

    gcmHEADER_ARG("Signal=0x%x State=%d", Signal, State);

    /* Initialize the gcsHAL_INTERFACE structure. */
    iface.ignoreTLS = gcvFALSE;
    iface.command = gcvHAL_USER_SIGNAL;
    iface.u.UserSignal.command = gcvUSER_SIGNAL_SIGNAL;
    iface.u.UserSignal.id      = (gctINT)(gctUINTPTR_T) Signal;
    iface.u.UserSignal.state   = State;

    /* Call kernel driver. */
    status = gcoOS_DeviceControl(gcvNULL,
                                 IOCTL_GCHAL_INTERFACE,
                                 &iface, gcmSIZEOF(iface),
                                 &iface, gcmSIZEOF(iface));

    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gcoOS_WaitSignal
**
**  Wait for a signal to become signaled.
**
**  INPUT:
**
**      gcoOS Os
**          Pointer to an gcoOS object.
**
**      gctSIGNAL Signal
**          Pointer to the gctSIGNAL.
**
**      gctUINT32 Wait
**          Number of milliseconds to wait.
**          Pass the value of gcvINFINITE for an infinite wait.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gcoOS_WaitSignal(
    IN gcoOS Os,
    IN gctSIGNAL Signal,
    IN gctUINT32 Wait
    )
{
#if gcdNULL_DRIVER
    return gcvSTATUS_OK;
#else
    gceSTATUS status;
    gcsHAL_INTERFACE iface;

    gcmHEADER_ARG("Signal=0x%x Wait=%u", Signal, Wait);

    /* Initialize the gcsHAL_INTERFACE structure. */
    iface.ignoreTLS = gcvFALSE;
    iface.command = gcvHAL_USER_SIGNAL;
    iface.u.UserSignal.command = gcvUSER_SIGNAL_WAIT;
    iface.u.UserSignal.id      = (gctINT)(gctUINTPTR_T) Signal;
    iface.u.UserSignal.wait    = Wait;

    /* Call kernel driver. */
    status = gcoOS_DeviceControl(gcvNULL,
                                 IOCTL_GCHAL_INTERFACE,
                                 &iface, gcmSIZEOF(iface),
                                 &iface, gcmSIZEOF(iface));

    gcmFOOTER_ARG("Signal=0x%x status=%d", Signal, status);
    return status;
#endif
}

/*******************************************************************************
**
**  gcoOS_MapSignal
**
**  Map a signal from another process.
**
**  INPUT:
**
**      gctSIGNAL  RemoteSignal
**
**  OUTPUT:
**
**      gctSIGNAL * LocalSignal
**          Pointer to a variable receiving the created gctSIGNAL.
*/
gceSTATUS
gcoOS_MapSignal(
    IN gctSIGNAL  RemoteSignal,
    OUT gctSIGNAL * LocalSignal
    )
{
    gceSTATUS status;
    gcsHAL_INTERFACE iface;

    gcmHEADER_ARG("RemoteSignal=%d", RemoteSignal);

    gcmDEBUG_VERIFY_ARGUMENT(RemoteSignal != gcvNULL);
    gcmDEBUG_VERIFY_ARGUMENT(LocalSignal != gcvNULL);
    iface.ignoreTLS = gcvFALSE;
    iface.command = gcvHAL_USER_SIGNAL;
    iface.u.UserSignal.command  = gcvUSER_SIGNAL_MAP;
    iface.u.UserSignal.id = (gctINT)(gctUINTPTR_T) RemoteSignal;

    gcmONERROR(gcoOS_DeviceControl(gcvNULL,
                                   IOCTL_GCHAL_INTERFACE,
                                   &iface, gcmSIZEOF(iface),
                                   &iface, gcmSIZEOF(iface)));

    *LocalSignal = (gctSIGNAL)(gctUINTPTR_T) iface.u.UserSignal.id;

    gcmFOOTER_ARG("*LocalSignal=0x%x", *LocalSignal);
    return gcvSTATUS_OK;

OnError:
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gcoOS_UnmapSignal
**
**  Unmap a signal mapped from another process.
**
**  INPUT:
**
**      gctSIGNAL Signal
**          Pointer to the gctSIGNAL.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gcoOS_UnmapSignal(
    IN gctSIGNAL Signal
    )
{
    gceSTATUS status;
    gcsHAL_INTERFACE iface;

    gcmHEADER_ARG("Signal=0x%x", Signal);

    gcmDEBUG_VERIFY_ARGUMENT(Signal != gcvNULL);

    gcmTRACE_ZONE(
        gcvLEVEL_VERBOSE, gcvZONE_OS,
        "gcoOS_UnmapSignal: signal->%d.",
        (gctINT)(gctUINTPTR_T)Signal
        );
    iface.ignoreTLS = gcvFALSE;
    iface.command = gcvHAL_USER_SIGNAL;
    iface.u.UserSignal.command = gcvUSER_SIGNAL_UNMAP;
    iface.u.UserSignal.id      = (gctINT)(gctUINTPTR_T) Signal;

    status = gcoOS_DeviceControl(gcvNULL,
                                 IOCTL_GCHAL_INTERFACE,
                                 &iface, gcmSIZEOF(iface),
                                 &iface, gcmSIZEOF(iface));

    gcmFOOTER();
    return status;
}


void _SignalHandlerForSIGFPEWhenSignalCodeIs0(
    int sig_num,
    siginfo_t * info,
    void * ucontext
    )
{
    gctINT signalCode;

    signalCode = ((info->si_code) & 0xffff);
    if (signalCode == 0)
    {
        /* simply ignore the signal, this is a temporary fix for bug 4203 */
        return;
    }

    /* Let OS handle the signal */
    gcoOS_Print("Process got signal (%d). To further debug the issue, you should run in debug mode", sig_num);
    signal (sig_num, SIG_DFL);
    raise (sig_num);
    return;
}

/*******************************************************************************
**
**  gcoOS_AddSignalHandler
**
**  Adds Signal handler depending on Signal Handler Type
**
**  INPUT:
**
**      gceSignalHandlerType SignalHandlerType
**          Type of handler to be added
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gcoOS_AddSignalHandler (
    IN gceSignalHandlerType SignalHandlerType
    )
{
    gceSTATUS status = gcvSTATUS_OK;

    gcmHEADER_ARG("SignalHandlerType=0x%x", SignalHandlerType);

    switch(SignalHandlerType)
    {
    case gcvHANDLE_SIGFPE_WHEN_SIGNAL_CODE_IS_0:
        {
#if gcdDEBUG
                /* Handler will not be registered in debug mode*/
                gcmTRACE(
                    gcvLEVEL_INFO,
                    "%s(%d): Will not register signal handler for type gcvHANDLE_SIGFPE_WHEN_SIGNAL_CODE_IS_0 in debug mode",
                    __FUNCTION__, __LINE__
                    );
#else
            struct sigaction temp;
            struct sigaction sigact;
            sigaction (SIGFPE, NULL, &temp);
            if (temp.sa_handler != (void *)_SignalHandlerForSIGFPEWhenSignalCodeIs0)
            {
                sigact.sa_handler = (void *)_SignalHandlerForSIGFPEWhenSignalCodeIs0;
                sigact.sa_flags = SA_RESTART | SA_SIGINFO;
                sigemptyset(&sigact.sa_mask);
                sigaction(SIGFPE, &sigact, (struct sigaction *)NULL);
            }
#endif
            break;
        }

    default:
        {
            /* Unknown handler type */
            gcmTRACE(
                gcvLEVEL_ERROR,
                "%s(%d): Cannot register a signal handler for type 0x%0x",
                __FUNCTION__, __LINE__, SignalHandlerType
                );
            break;
        }

    }
    gcmFOOTER();
    return status;
}

gceSTATUS
gcoOS_QueryCurrentProcessArguments(
    OUT gctCHAR Argv[gcdMAX_ARGUMENT_COUNT][gcdMAX_ARGUMENT_SIZE],
    OUT gctUINT32 *Argc,
    IN  gctUINT32 MaxArgc,
    IN  gctUINT32 MaxSizePerArg
)
{
    if (Argc)
        *Argc = 0;
    return gcvSTATUS_OK;
}

gceSTATUS
gcoOS_QueryCurrentProcessName(
    OUT gctSTRING Name,
    IN gctSIZE_T Size
    )
{
    return gcvSTATUS_OK;
}

/*******************************************************************************
**
**  gcoOS_DetectProcessByName
**
**  Detect if the current process is the executable specified.
**
**  INPUT:
**
**      gctCONST_STRING Name
**          Name (full or partial) of executable.
**
**  OUTPUT:
**
**      Nothing.
**
**
**  RETURN:
**
**      gcvSTATUS_TRUE
**              if process is as specified by Name parameter.
**      gcvSTATUS_FALSE
**              Otherwise.
**
*/
gceSTATUS
gcoOS_DetectProcessByName(
    IN gctCONST_STRING Name
    )
{
    gctCHAR curProcessName[gcdMAX_PATH];
    gceSTATUS status = gcvSTATUS_FALSE;

    gcmHEADER_ARG("Name=%s", Name);

    if (gcmIS_SUCCESS(gcoOS_QueryCurrentProcessName(curProcessName, gcdMAX_PATH)) &&
        (gcoOS_StrStr(curProcessName, Name, gcvNULL) == gcvSTATUS_TRUE)
       )
    {
        status = gcvSTATUS_TRUE;
    }

    gcmFOOTER();
    return status;
}

gceSTATUS
gcoOS_DetectProcessByEncryptedName(
    IN gctCONST_STRING Name
    )
{
    gceSTATUS status = gcvSTATUS_FALSE;
    gctCHAR *p, buff[gcdMAX_PATH];
    p = buff;

    gcmONERROR(gcoOS_StrCopySafe(buff, gcdMAX_PATH, Name));

    while (*p)
    {
        *p = ~(*p);
        p++;
    }

    status = gcoOS_DetectProcessByName(buff);

OnError:
    return status;
}

gceSTATUS
gcoOS_CPUPhysicalToGPUPhysical(
    IN gctPHYS_ADDR_T CPUPhysical,
    OUT gctPHYS_ADDR_T * GPUPhysical
    )
{
    gctPHYS_ADDR_T cpuPhysical = CPUPhysical, gpuPhysical;

    gcoOS os = gcPLS.os;
    gcoPLATFORM platform;

    gcmHEADER();
    gcmVERIFY_ARGUMENT(os);

    platform = &os->platform;

    if (platform->ops->getGPUPhysical)
    {
        platform->ops->getGPUPhysical(platform, cpuPhysical, &gpuPhysical);

        *GPUPhysical = (gctUINT32) gpuPhysical;
    }
    else
    {
        *GPUPhysical = CPUPhysical;
    }

    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

void
gcoOS_RecordAllocation(void)
{
#if gcdGC355_MEM_PRINT
    gcoOS os;
    if (gcPLS.os != gcvNULL)
    {
        os = gcPLS.os;

        os->oneSize = 0;
        os->oneRecording = 1;
    }
#endif
}

gctINT32
gcoOS_EndRecordAllocation(void)
{
    gctINT32   result = 0;
#if gcdGC355_MEM_PRINT
    gcoOS os;

    if (gcPLS.os != gcvNULL)
    {
        os = gcPLS.os;

        if (os->oneRecording == 1)
        {
            result = os->oneSize;

            os->oneSize = 0;
            os->oneRecording = 0;
        }
    }

#endif
    return result;
}

void
gcoOS_AddRecordAllocation(gctSIZE_T Size)
{
#if gcdGC355_MEM_PRINT
    gcoOS os;

    if (gcPLS.os != gcvNULL)
    {
        os = gcPLS.os;

        if (os->oneRecording == 1)
        {
            os->oneSize += (gctINT32)Size;
        }
    }
#endif
}

gceSTATUS
gcoOS_QuerySystemInfo(
    IN gcoOS Os,
    OUT gcsSystemInfo *Info
    )
{
    /* SH cycles = MC cycles * (SH clock/MC clock).
    ** Default value is 128 * 3 (cycles).
    */
    Info->memoryLatencySH = 128 * 3;

    return gcvSTATUS_OK;
}

#if VIVANTE_PROFILER_SYSTEM_MEMORY

gceSTATUS gcoOS_GetMemoryProfileInfo(size_t                      size,
                                     struct _memory_profile_info *info)
{
    return gcvSTATUS_NOT_SUPPORTED;
}

#endif
