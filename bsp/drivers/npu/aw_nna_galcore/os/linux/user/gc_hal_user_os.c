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

#  define _XOPEN_SOURCE 600

#if defined(SOLARIS)
#  define __EXTENSIONS__
#endif

#include "gc_hal_user_linux.h"
#include "gc_hal_user_os_atomic.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/sysinfo.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/file.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <dlfcn.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>
#include <sys/syscall.h>
#include <dirent.h>
#include <poll.h>
#include <signal.h>

#if gcdENABLE_TTM
#include <xf86drm.h>
#include <xf86drmMode.h>
#include "gc_hal_drm.h"
#endif

#ifdef ANDROID
#include <elf.h>
#include <cutils/properties.h>
#endif

#include "gc_hal_user_platform.h"
#include "gc_hal_dump.h"

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

#ifdef ANDROID
#define gcmGETTHREADID() \
    (gctUINT32) gettid()
#else
#   define gcmGETTHREADID() \
    (gctUINT32)(gctUINTPTR_T) pthread_self()
#endif

#if gcdENABLE_TTM
int
drmIoctl(int fd, unsigned long request, void *arg)
{
    int ret;

    do {
        ret = ioctl(fd, request, arg);
    } while (ret == -1 && (errno == EINTR || errno == EAGAIN));

    return ret;
}
#endif

/*******************************************************************************
***** Version Signature *******************************************************/

#ifdef ANDROID
const char * _GAL_PLATFORM = "\n\0$PLATFORM$Android$\n";
#else
const char * _GAL_PLATFORM = "\n\0$PLATFORM$Linux$\n";
#endif

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

#if gcdENABLE_PROFILING
    gctUINT64               startTick;
#endif

#if VIVANTE_PROFILER_SYSTEM_MEMORY
#if gcdGC355_MEM_PRINT
    /* For a single collection. */
    gctINT32                oneSize;
    gctBOOL                 oneRecording;
#endif
#endif

    gcsPLATFORM             platform;

    /* Handle to the device. */
    int                     device;

    int                     devices[gcdDEVICE_COUNT];
    int                     deviceCount;

#if gcdENABLE_TTM
    int                     drmDevice;
#endif
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
#if (gcmIS_DEBUG(gcdDEBUG_TRACE) || gcdGC355_MEM_PRINT)
static void _ReportDB(
    void
    )
{
    gceSTATUS status = gcvSTATUS_OK;
    gcsHAL_INTERFACE iface = {0};

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
        (iface.u.Database.nonPaged.counters.bytes   != 0))
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

    gcmPRINT("05) Video memory - current : %lld \n", iface.u.Database.vidMem.counters.bytes);
    gcmPRINT("06) Video memory - maximum : %lld \n", iface.u.Database.vidMem.counters.maxBytes);
    gcmPRINT("07) Video memory - total   : %lld \n", iface.u.Database.vidMem.counters.totalBytes);
    gcmPRINT("08) Video memory - allocation Count: %u \n", iface.u.Database.vidMem.counters.allocCount);
    gcmPRINT("09) Video memory - deallocation Count : %u \n", iface.u.Database.vidMem.counters.freeCount);

OnError:;
}
#endif

#if gcdDUMP || gcdDUMP_API || gcdDUMP_2D
static void
_SetDumpFileInfo(
    )
{
    gceSTATUS status = gcvSTATUS_TRUE;
    gctBOOL dumpNew2D = gcvFALSE;
#if gcdDUMP_2D
    gcsTLS_PTR tls;
#endif

#if gcdDUMP || gcdDUMP_2D
    #define DUMP_FILE_PREFIX   "hal"
#else
    #define DUMP_FILE_PREFIX   "api"
#endif

#if defined(ANDROID)
    /* The default key of dump key for match use. Only useful in android. Do not change it! */
    #define DEFAULT_DUMP_KEY        "allprocesses"

    if (gcmIS_SUCCESS(gcoOS_StrCmp(gcdDUMP_KEY, DEFAULT_DUMP_KEY)))
        status = gcvSTATUS_TRUE;
    else
        status = gcoOS_DetectProcessByName(gcdDUMP_KEY);
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
#if !gcdDUMP_IN_KERNEL
        char dump_file[128];
        gctUINT offset = 0;

        /* Customize filename as needed. */
        gcmVERIFY_OK(gcoOS_PrintStrSafe(dump_file,
                     gcmSIZEOF(dump_file),
                     &offset,
                     "%s%s_dump_pid-%d_tid-%d_%s.log",
                     gcdDUMP_PATH,
                     DUMP_FILE_PREFIX,
                     (int) getpid(),
                     gcmGETTHREADID(),
                     dumpNew2D ? "dump2D" : gcdDUMP_KEY));

        gcoOS_SetDebugFile(dump_file);
#endif

        gcoOS_SetDumpFlag(gcvTRUE);
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

#if VIVANTE_PROFILER_SYSTEM_MEMORY
        /* End profiler. */
        gcoOS_ProfileEnd(gcPLS.os, "system memory");
#endif

        /* Close the handle to the kernel service. */
        if (gcPLS.os->device != -1)
        {
#if gcmIS_DEBUG(gcdDEBUG_TRACE) || gcdGC355_MEM_PRINT
            _ReportDB();
#endif

            close(gcPLS.os->device);
#if gcdENABLE_TTM
            close(gcPLS.os->drmDevice);
            gcPLS.os->drmDevice = -1;
#endif
            gcPLS.os->device = -1;
        }
        {
            gctUINT i = 0;

            gcPLS.os->devices[0] = -1;
            for (i = 1; i < gcdDEVICE_COUNT; i++)
            {
                if (gcPLS.os->devices[i] != -1)
                {
                    close(gcPLS.os->devices[i]);
                    gcPLS.os->devices[i] = -1;
                }
            }
        }

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

#if gcmIS_DEBUG(gcdDEBUG_STACK) && !defined(ANDROID)

static __sighandler_t handleSEGV;
static __sighandler_t handleINT;

static void
_DumpCallStackOnCrash(
    int signo
    )
{
    printf("libGAL: catch signal %d\n", signo);
    fflush(stdout);

    gcmSTACK_DUMP();

    /* Restore old signal handler. */
    switch (signo)
    {
    case SIGSEGV:
        signal(SIGSEGV, handleSEGV);
        break;
    case SIGINT:
        signal(SIGINT, handleINT);
        break;
    default:
        break;
    }

    raise(signo);
}

static void
_InstallSignalHandler(
    void
    )
{
    handleSEGV = signal(SIGSEGV, _DumpCallStackOnCrash);
    handleINT  = signal(SIGINT, _DumpCallStackOnCrash);
}

static void
_RestoreSignalHandler(
    void
    )
{
    signal(SIGSEGV, handleSEGV);
    signal(SIGINT, handleINT);
}
#endif

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
        os->device      = -1;
        gcoOS_MemFill(&os->devices, 0xFF, gcmSIZEOF(os->devices));

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
#if VIVANTE_PROFILER_SYSTEM_MEMORY
            /* Start profiler. */
        gcoOS_ProfileStart(os);
#endif
        /* Get profiler start tick. */
        gcmPROFILE_INIT(freq, os->startTick);

        /* Get platform callback functions. */
        gcoPLATFORM_QueryOperations(&os->platform.ops);
    }

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
#if !gcdSTATIC_LINK
static gceSTATUS __attribute__((constructor)) _ModuleConstructor(void);
#if defined(ANDROID) && (ANDROID_SDK_VERSION > 32)
static void _ModuleDestructor(void);
#else
static void __attribute__((destructor)) _ModuleDestructor(void);
#endif
#endif

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
    gceSTATUS status = gcvSTATUS_OK;
    gcsHAL_INTERFACE iface = {0};
    gctUINT32 index = 0;

    gcmHEADER();

    /* Call kernel HAL to query video memory. */
    iface.ignoreTLS    = gcvTRUE;
    iface.hardwareType = gcPLS.hal->defaultHwType;

    if (iface.hardwareType != gcvHARDWARE_2D)
    {
        gcoHAL_InitCoreIndexByType(gcvNULL, iface.hardwareType, gcvFALSE, &index);
    }

    iface.coreIndex = index;
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

static void
_OpenGalLib(
    gcsTLS_PTR TLS
    )
{
#if !gcdSTATIC_LINK
#if defined(ANDROID)
    gctHANDLE handle;

    gcmHEADER_ARG("TLS=0x%x", TLS);

    gcmASSERT(TLS != gcvNULL);

    /* Only two possibilities on android. */
#if defined(__LP64__)
    handle = dlopen("/system/vendor/lib64/libGAL.so", RTLD_NOW);
#     else
    handle = dlopen("/system/vendor/lib/libGAL.so", RTLD_NOW);
#     endif

    if (handle == gcvNULL)
    {
#if defined(__LP64__)
        handle = dlopen("/system/lib64/libGAL.so", RTLD_NOW);
#     else
        handle = dlopen("/system/lib/libGAL.so", RTLD_NOW);
#     endif
    }

    if (handle != gcvNULL)
    {
        TLS->handle = handle;
    }

    gcmFOOTER_NO();
#   else
    gctSTRING   path;
    gctSTRING   envPath = gcvNULL;
    gctSTRING   oneEnvPath;
    gctSTRING   fullPath = gcvNULL;
    gctINT32    len;
    gctHANDLE   handle = gcvNULL;
    gctSTRING   saveptr = gcvNULL;

    gcmHEADER_ARG("TLS=0x%x", TLS);

    gcmASSERT(TLS != gcvNULL);

    do
    {
        path = getenv("LD_LIBRARY_PATH");

        if (path != gcvNULL)
        {
            len = strlen(path) + 1;

            envPath  = malloc(len);
            fullPath = malloc(len + 10);

            if (envPath == NULL || fullPath == NULL)
            {
                break;
            }

            memset(envPath, 0, len);
            memcpy(envPath, path, len);
            oneEnvPath = strtok_r(envPath, ":", &saveptr);

            while (oneEnvPath != NULL)
            {
                sprintf(fullPath, "%s/libGAL.so", oneEnvPath);

                handle = dlopen(fullPath, RTLD_NOW | RTLD_NODELETE);

                if (handle != gcvNULL)
                {
                    break;
                }

                oneEnvPath = strtok_r(NULL, ":", &saveptr);
            }
        }

        if (handle == gcvNULL)
        {
            handle = dlopen("/usr/lib/libGAL.so", RTLD_NOW | RTLD_NODELETE);
        }

        if (handle == gcvNULL)
        {
            handle = dlopen("/lib/libGAL.so", RTLD_NOW | RTLD_NODELETE);
        }
    }
    while (gcvFALSE);

    if (envPath != gcvNULL)
    {
        free(envPath);
    }

    if (fullPath != gcvNULL)
    {
        free(fullPath);
    }

    if (handle != gcvNULL)
    {
        TLS->handle = handle;
    }

    gcmFOOTER_NO();
#   endif
#endif
}

static void
_CloseGalLib(
    gcsTLS_PTR TLS
    )
{
    gcmHEADER_ARG("TLS=0x%x", TLS);

    gcmASSERT(TLS != gcvNULL);

    if (TLS->handle != gcvNULL)
    {
        gcoOS_FreeLibrary(gcvNULL, TLS->handle);
        TLS->handle = gcvNULL;
    }

    gcmFOOTER_NO();
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

    gcmVERIFY_OK(_DestroyOs(gcPLS.os));

    if (gcPLS.hal != gcvNULL)
    {
        gcmVERIFY_OK(gcoHAL_DestroyEx(gcPLS.hal));
        gcPLS.hal = gcvNULL;
    }


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
        gceHARDWARE_TYPE savedHardwareType = gcvHARDWARE_INVALID, savedTargetType = gcvHARDWARE_INVALID;
        gceHARDWARE_TYPE hardwareType = gcvHARDWARE_INVALID;
        gctUINT32 savedDevIndex, devIndex;

        savedHardwareType = tls->currentType;
        savedTargetType = tls->targetType;
        savedDevIndex = tls->currentDevIndex;

        gcmVERIFY_OK(gcoHARDWARE_GetHardwareType(tls->defaultHardware, &hardwareType));

        tls->currentType = hardwareType;
        tls->currentHardware = tls->defaultHardware;
        tls->targetType = hardwareType;

        gcoHARDWARE_QueryHwDeviceIndex(tls->defaultHardware, &devIndex);

        tls->currentDevIndex = devIndex;

        gcmTRACE_ZONE(
            gcvLEVEL_VERBOSE, gcvZONE_HARDWARE,
            "%s(%d): destroying default hardware object 0x%08X.",
            __FUNCTION__, __LINE__, tls->defaultHardware
            );

#if gcdENABLE_SW_PREEMPTION
        gcmVERIFY_OK(gcoHARDWARE_End(tls->defaultHardware));
#endif

        gcmVERIFY_OK(gcoHARDWARE_Destroy(tls->defaultHardware, gcvTRUE));

        tls->defaultHardware = gcvNULL;
        tls->currentHardware = gcvNULL;
        tls->currentType = savedHardwareType;
        tls->targetType = savedTargetType;
        tls->currentDevIndex = savedDevIndex;
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


    if (gcPLS.threadID && gcPLS.threadID != gcmGETTHREADID() && !gcPLS.exiting)
    {
        _CloseGalLib(tls);
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

    free(tls);

    pthread_setspecific(gcProcessKey, gcvNULL);

    gcmFOOTER_NO();
}

static void
_AtForkChild(
    void
    )
{
    /* Reset PLS. */
    gcPLS.os = gcvNULL;
    gcPLS.hal = gcvNULL;
    gcPLS.contiguousSize = 0;
    gcPLS.processID = 0;
    gcPLS.reference = 0;
    gcPLS.patchID   = gcvPATCH_NOTINIT;

    pthread_key_delete(gcProcessKey);
}

static void
_OnceInit(
    void
    )
{
#if (defined(ANDROID) && (ANDROID_SDK_VERSION > 16)) || !defined(ANDROID)
    /* Use once control to avoid registering the handler multiple times. */
    pthread_atfork(gcvNULL, gcvNULL, _AtForkChild);
#endif

    /*
     * On Android, crash stack is in logcat log, so don't try to catch
     * these signals.
     */
#if gcmIS_DEBUG(gcdDEBUG_STACK) && !defined(ANDROID)
    _InstallSignalHandler();
#endif
}

static gceSTATUS
_ModuleConstructor(
    void
    )
{
    gceSTATUS status = gcvSTATUS_OK;
    int result;
    static pthread_once_t onceControl = {PTHREAD_ONCE_INIT};

#if VIVANTE_PROFILER_SYSTEM_MEMORY
    gcoOS_InitMemoryProfile();
#endif

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

    pthread_once(&onceControl, _OnceInit);

    /* Create tls key. */
    result = pthread_key_create(&gcProcessKey, _TLSDestructor);

    if (result != 0)
    {
        gcmTRACE(
            gcvLEVEL_ERROR,
            "%s(%d): pthread_key_create returned %d",
            __FUNCTION__, __LINE__, result
            );

        gcmONERROR(gcvSTATUS_OUT_OF_MEMORY);
    }

#if gcdSTATIC_LINK && defined(__linux__)
    atexit(gcoOS_ModuleDestructor);
#elif defined(ANDROID) && (ANDROID_SDK_VERSION > 32)
    atexit((void (*)(void)) _ModuleDestructor);
#endif

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
        "gcPLS.os=0x%08X, gcPLS.hal=0x%08X",
        gcPLS.os, gcPLS.hal
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


#if gcdSTATIC_LINK
void
gcoOS_ModuleConstructor(
    void
    )
{
    _ModuleConstructor();
}
#endif

static gceSTATUS
_OpenDevice(
    IN gcoOS Os
    )
{
    gceSTATUS status = gcvSTATUS_OK;

    gcmHEADER();

    pthread_mutex_lock(&plsMutex);

    if (gcPLS.bDeviceOpen)
    {
        pthread_mutex_unlock(&plsMutex);
        gcmFOOTER_NO();
        return gcvSTATUS_OK;
    }

    {
    gctUINT tryCount;
    gctSTRING galDeviceName = gcvNULL;

    /* Specify the device name through environment variable. */
    gcoOS_GetEnv(gcvNULL, "GAL_DEVICE_NAME", &galDeviceName);

    for (tryCount = 0; tryCount < 5; tryCount++)
    {
        gctUINT i;

        if (tryCount > 0)
        {
            /* Sleep for a while when second and later tries. */
            usleep(1000000);
            gcmPRINT("Failed to open device: %s, Try again...", strerror(errno));
        }

        /* Attempt to open user spcified device. */
        if (galDeviceName && galDeviceName[0] != '\0')
        {
            Os->device = open(galDeviceName, O_RDWR);

            if (Os->device >= 0)
            {
                gcmTRACE_ZONE(
                    gcvLEVEL_VERBOSE, gcvZONE_OS,
                    "%s(%d): Opened user specified device %s.",
                    __FUNCTION__, __LINE__, galDeviceName
                    );

                break;
            }
        }

        /* Attempt to open pre-defined device. */
        for (i = 0; i < gcmCOUNTOF(GALDeviceName); i += 1)
        {
            gcmTRACE_ZONE(
                gcvLEVEL_VERBOSE, gcvZONE_OS,
                "%s(%d): Attempting to open device %s.",
                __FUNCTION__, __LINE__, GALDeviceName[i]
                );

            Os->device = open(GALDeviceName[i], O_RDWR);

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
    if (Os->device >= 0)
    { /* open all other device */
        gctUINT devCount = gcdDEVICE_COUNT;
        gctUINT i, devIdx;

        Os->devices[0] = Os->device;
        for (devIdx = 1; devIdx < devCount; devIdx++)
        {
            /* Attempt to open pre-defined device. */
            for (i = 0; i < gcmCOUNTOF(GALDeviceName); i += 1)
            {
                gctCHAR devName[256] = { 0 };
                gctSIZE_T len = gcoOS_StrLen(GALDeviceName[i], gcvNULL);

                gcmTRACE_ZONE(
                    gcvLEVEL_VERBOSE, gcvZONE_OS,
                    "%s(%d): Attempting to open device %s.",
                    __FUNCTION__, __LINE__, devName
                );

                gcoOS_StrCopySafe(devName, gcmSIZEOF(devName), GALDeviceName[i]);
                gcmASSERT(devIdx < 100);
                if (devIdx > 10)
                {
                    devName[len] = '0' + devIdx / 10;
                    devName[len + 1] = '0' + devIdx % 10;
                }
                else
                {
                    devName[len] = '0' + devIdx;
                }

                Os->devices[devIdx] = open(devName, O_RDWR);

                if (Os->devices[devIdx] >= 0)
                {
                    gcmTRACE_ZONE(
                        gcvLEVEL_VERBOSE, gcvZONE_OS,
                        "%s(%d): Opened device %s.",
                        __FUNCTION__, __LINE__, devName
                    );
                    break;
                }
            }
            if (Os->devices[devIdx] == -1)
            {
                break;
            }
        }
        Os->deviceCount = devIdx;
    }
#if gcdENABLE_TTM
    /* try to open drm device */
    Os->drmDevice = open("/dev/dri/card0", O_RDWR);
#endif
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

    gcPLS.bDeviceOpen = gcvTRUE;

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

OnError:
    pthread_mutex_unlock(&plsMutex);

    gcmFOOTER();
    return status;
}

#if gcdSTATIC_LINK
void
gcoOS_ModuleDestructor(
    void
    )
#else
static void
_ModuleDestructor(
    void
    )
#endif
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

#if gcmIS_DEBUG(gcdDEBUG_STACK) && !defined(ANDROID)
    _RestoreSignalHandler();
#endif

#if VIVANTE_PROFILER_SYSTEM_MEMORY
    gcoOS_DeInitMemoryProfile();
#endif

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

    gcmHEADER_ARG("TLS=%p", TLS);

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
        tls = (gcsTLS_PTR) malloc(sizeof(gcsTLS));

        if (!tls)
        {
            gcmONERROR(gcvSTATUS_OUT_OF_MEMORY);
        }

        memset(tls, 0, sizeof(gcsTLS));

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

        if (gcPLS.threadID && gcPLS.threadID != gcmGETTHREADID())
            _OpenGalLib(tls);

        if (gcPLS.reference != gcvNULL)
        {
            /* Increment PLS reference. */
            gcmONERROR(gcoOS_AtomIncrement(gcPLS.os, gcPLS.reference, gcvNULL));
        }

#if gcdDUMP || gcdDUMP_API || gcdDUMP_2D
        _SetDumpFileInfo();
#endif
    }

    *TLS = tls;

    gcmFOOTER_NO();
    return gcvSTATUS_OK;

OnError:
    if (tls != gcvNULL)
    {
        free(tls);
    }

    * TLS = gcvNULL;

    gcmFOOTER();
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
    tls = malloc(gcmSIZEOF(gcsTLS));

    if (!tls)
    {
        gcmONERROR(gcvSTATUS_OUT_OF_MEMORY);
    }

    /* Set the thread specific data. */
    pthread_setspecific(gcProcessKey, tls);

    if (gcPLS.threadID && gcPLS.threadID != gcmGETTHREADID())
        _OpenGalLib(tls);

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

#if gcdDUMP || gcdDUMP_API || gcdDUMP_2D
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
    else
    {
        status = gcvSTATUS_INVALID_ARGUMENT;
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
    else
    {
        status = gcvSTATUS_INVALID_ARGUMENT;
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
#if defined(SOLARIS)
    if (PhysicalSystemMemorySize != gcvNULL)
    {
        *PhysicalSystemMemorySize = sysconf(_SC_PHYS_PAGES) * sysconf(_SC_PAGESIZE);
    }
#else
    struct sysinfo info;
    sysinfo( &info );
    if (PhysicalSystemMemorySize != gcvNULL)
    {
        *PhysicalSystemMemorySize = (gctUINT64)info.totalram * (gctUINT64)info.mem_unit;
    }
#endif
    return  gcvSTATUS_OK;
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
    gceSTATUS status = gcvSTATUS_OK;
    gcsHAL_INTERFACE iface = {0};

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
** Deprecated API: please use gcoHAL_GetBaseAddr() instead.
**                 This API was kept only for legacy BSP usage.
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
    gceSTATUS status = gcvSTATUS_OK;
    gceHARDWARE_TYPE type = gcvHARDWARE_INVALID;

    gcmHEADER();

    /* Verify the arguments. */
    gcmDEBUG_VERIFY_ARGUMENT(BaseAddress);

    gcmONERROR(gcoHAL_GetHardwareType(gcvNULL, &type));

    /* Return base address. */
    if (type == gcvHARDWARE_VG)
    {
        *BaseAddress = 0;
    }
    else
    {
        gcsHAL_INTERFACE iface = {0};

        /* Query base address. */
        iface.ignoreTLS = gcvFALSE;
        iface.command = gcvHAL_GET_BASE_ADDRESS;

        /* Call kernel driver. */
        gcmONERROR(gcoOS_DeviceControl(
            gcvNULL,
            IOCTL_GCHAL_INTERFACE,
            &iface, gcmSIZEOF(iface),
            &iface, gcmSIZEOF(iface)
            ));

        *BaseAddress = iface.u.GetBaseAddress.baseAddress;
    }

OnError:
    /* Success. */
    gcmFOOTER_ARG("*BaseAddress=0x%08x", *BaseAddress);
    return status;
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

    *Memory = gcvNULL;
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
    gctPOINTER memory = gcvNULL;

    gcmHEADER_ARG("Bytes=%lu", Bytes);

    /* Verify the arguments. */
    gcmDEBUG_VERIFY_ARGUMENT(Bytes > 0);
    gcmDEBUG_VERIFY_ARGUMENT(Memory != gcvNULL);

    /* Allocate the memory. */
#if VIVANTE_PROFILER_SYSTEM_MEMORY
    if (gcPLS.bMemoryProfile)
    {
        gcmONERROR(gcmCHECK_ADD_OVERFLOW(Bytes, VP_MALLOC_OFFSET));
        memory = malloc(Bytes + VP_MALLOC_OFFSET);
    }
    else
#endif
    {
        memory = malloc(Bytes);
    }

    if (memory == gcvNULL)
    {
        /* Out of memory. */
        gcmONERROR(gcvSTATUS_OUT_OF_MEMORY);
    }

#if VIVANTE_PROFILER_SYSTEM_MEMORY
    if (gcPLS.bMemoryProfile)
    {
        gcoOS os = (gcPLS.os != gcvNULL) ? gcPLS.os : Os;

        if (os != gcvNULL)
        {
#if gcdGC355_MEM_PRINT
            if (os->oneRecording == 1)
            {
                os->oneSize += (gctINT32)Bytes;
            }
#endif
        }

        if (gcPLS.profileLock)
        {
            gcmONERROR(gcoOS_AcquireMutex(os, gcPLS.profileLock, gcvINFINITE));
            ++ (gcPLS.allocCount);
            gcPLS.allocSize += Bytes;
            gcPLS.currentSize += Bytes;

            if (gcPLS.currentSize > gcPLS.maxAllocSize)
            {
                gcPLS.maxAllocSize = gcPLS.currentSize;
            }
            gcmONERROR(gcoOS_ReleaseMutex(os, gcPLS.profileLock));
        }

        /* Return pointer to the memory allocation. */
        *(gctSIZE_T *) memory = Bytes;
        *Memory = (gctPOINTER) ((gctUINT8 *) memory + VP_MALLOC_OFFSET);
    }
    else
#endif
    {
        /* Return pointer to the memory allocation. */
        *Memory = memory;
    }

    /* Success. */
    gcmFOOTER_ARG("*Memory=0x%x", *Memory);
    return gcvSTATUS_OK;

OnError:
#if VIVANTE_PROFILER_SYSTEM_MEMORY
    if (memory)
    {
        free(memory);
    }
#endif

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
    gctPOINTER memory = gcvNULL;

    gcmHEADER_ARG("Bytes=%lu", Bytes);

    /* Verify the arguments. */
    gcmDEBUG_VERIFY_ARGUMENT(Bytes > 0);
    gcmDEBUG_VERIFY_ARGUMENT(Memory != gcvNULL);

    /* Allocate the memory. */
#if VIVANTE_PROFILER_SYSTEM_MEMORY
    if (gcPLS.bMemoryProfile)
    {
        gcmONERROR(gcmCHECK_ADD_OVERFLOW(Bytes, VP_MALLOC_OFFSET));
        memory = realloc(*(gctUINT8_PTR *)Memory - VP_MALLOC_OFFSET , Bytes + VP_MALLOC_OFFSET);
    }
    else
#endif
    {
        memory = realloc(*Memory, Bytes);
    }

    if (memory == gcvNULL)
    {
        /* Out of memory. */
        gcmONERROR(gcvSTATUS_OUT_OF_MEMORY);
    }

#if VIVANTE_PROFILER_SYSTEM_MEMORY
    if (gcPLS.bMemoryProfile)
    {
        gcoOS os = (gcPLS.os != gcvNULL) ? gcPLS.os : Os;

        if (os != gcvNULL)
        {
#if gcdGC355_MEM_PRINT
            if (os->oneRecording == 1)
            {
                os->oneSize -= (gctINT32)OrgBytes;
                os->oneSize += (gctINT32)Bytes;
            }
#endif
        }

        if (gcPLS.profileLock)
        {
            /* realloc treated as free + malloc, even it is not always happen so */
            gcmONERROR(gcoOS_AcquireMutex(os, gcPLS.profileLock, gcvINFINITE));
            ++ (gcPLS.allocCount);
            ++ (gcPLS.freeCount);
            gcPLS.allocSize += Bytes;
            gcPLS.freeSize += OrgBytes;
            gcPLS.currentSize -= OrgBytes;
            gcPLS.currentSize += Bytes;

            if (gcPLS.currentSize > gcPLS.maxAllocSize)
            {
                gcPLS.maxAllocSize = gcPLS.currentSize;
            }
            gcmONERROR(gcoOS_ReleaseMutex(os, gcPLS.profileLock));
        }

        /* Return pointer to the memory allocation. */
        *(gctSIZE_T *) memory = Bytes;
        *Memory = (gctPOINTER) ((gctUINT8 *) memory + VP_MALLOC_OFFSET);
    }
    else
#endif
    {
        /* Return pointer to the memory allocation. */
        *Memory = memory;
    }

    /* Success. */
    gcmFOOTER_ARG("*Memory=0x%x", *Memory);
    return gcvSTATUS_OK;

OnError:
#if VIVANTE_PROFILER_SYSTEM_MEMORY
    if (memory)
    {
        free(memory);
    }
#endif

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
    if (gcPLS.bMemoryProfile)
    {
        gcoOS os = (gcPLS.os != gcvNULL) ? gcPLS.os : Os;
        gctPOINTER p = (gctPOINTER) ((gctUINT8 *) Memory - VP_MALLOC_OFFSET);
        gctSIZE_T size = *((gctSIZE_T *)p);
        free(p);

        if (os != gcvNULL)
        {
#if gcdGC355_MEM_PRINT
            if (os->oneRecording == 1)
            {
                os->oneSize -= (gctINT32)size;
            }
#endif
        }

        if (gcPLS.profileLock)
        {
            gcmVERIFY_OK(gcoOS_AcquireMutex(os, gcPLS.profileLock, gcvINFINITE));

            gcPLS.freeSize += size;
            ++ (gcPLS.freeCount);
            gcPLS.currentSize -= size;
            gcmVERIFY_OK(gcoOS_ReleaseMutex(os, gcPLS.profileLock));
        }
    }
    else
#endif
    {
        free(Memory);
    }

    /* Success. */
    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

#if gcdENABLE_TTM
gceSTATUS
gcoOS_DeviceControlWithDRM(IN gcoOS Os,
                    IN gcsHAL_INTERFACE_PTR InputBuffer,
                    IN gcsHAL_INTERFACE_PTR OutputBuffer)
{
    gceSTATUS status = gcvSTATUS_OK;
    struct drm_viv_gem_create create_args = {};
    struct drm_viv_gem_lock lock_args = {};
    struct drm_viv_gem_unlock unlock_args = {};
    struct drm_viv_gem_release release_args = {};
    struct drm_viv_gem_wrap wrap_args = {};
    gctINT32 ret = 0;

    gcmHEADER();
    switch (InputBuffer->command){
        case gcvHAL_ALLOCATE_LINEAR_VIDEO_MEMORY:
            create_args.size = InputBuffer->u.AllocateLinearVideoMemory.bytes;
            create_args.flags = InputBuffer->u.AllocateLinearVideoMemory.flag;
            create_args.type = InputBuffer->u.AllocateLinearVideoMemory.type;
            ret = drmIoctl(gcPLS.os->drmDevice, DRM_IOCTL_VIV_GEM_CREATE, &create_args);
            if (ret)
                gcmONERROR(gcvSTATUS_INVALID_ARGUMENT);
            OutputBuffer->u.AllocateLinearVideoMemory.node = create_args.node;
            break;
        case gcvHAL_RELEASE_VIDEO_MEMORY:
            release_args.node = InputBuffer->u.ReleaseVideoMemory.node;
            ret = drmIoctl(gcPLS.os->drmDevice, DRM_IOCTL_VIV_GEM_RELEASE, &release_args);
            if (ret)
                gcmONERROR(gcvSTATUS_INVALID_ARGUMENT);
            break;
        case gcvHAL_LOCK_VIDEO_MEMORY:
            lock_args.node = InputBuffer->u.LockVideoMemory.node;
            lock_args.priv = gcmPTR_TO_UINT64(InputBuffer);
            lock_args.priv_size = gcmSIZEOF(gcsHAL_INTERFACE);
            ret = drmIoctl(gcPLS.os->drmDevice, DRM_IOCTL_VIV_GEM_LOCK, &lock_args);
            if (ret)
                gcmONERROR(gcvSTATUS_INVALID_ARGUMENT);
            OutputBuffer = gcmUINT64_TO_PTR(lock_args.priv);
            break;
        case gcvHAL_UNLOCK_VIDEO_MEMORY:
            unlock_args.node = InputBuffer->u.UnlockVideoMemory.node;
            unlock_args.async = InputBuffer->u.UnlockVideoMemory.asynchroneous;
            unlock_args.priv = gcmPTR_TO_UINT64(InputBuffer);
            unlock_args.priv_size = gcmSIZEOF(gcsHAL_INTERFACE);
            ret = drmIoctl(gcPLS.os->drmDevice, DRM_IOCTL_VIV_GEM_UNLOCK, &unlock_args);
            if (ret)
                gcmONERROR(gcvSTATUS_INVALID_ARGUMENT);
            OutputBuffer = gcmUINT64_TO_PTR(unlock_args.priv);
            /*OutputBuffer->u.UnlockVideoMemory.bytes = */
            break;
        case gcvHAL_WRAP_USER_MEMORY:
            wrap_args.priv = gcmPTR_TO_UINT64(InputBuffer);
            wrap_args.priv_size = gcmSIZEOF(gcsHAL_INTERFACE);
            ret = drmIoctl(gcPLS.os->drmDevice, DRM_IOCTL_VIV_GEM_WRAP, &wrap_args);
            if (ret)
                gcmONERROR(gcvSTATUS_INVALID_ARGUMENT);
            OutputBuffer = gcmUINT64_TO_PTR(wrap_args.priv);
            break;
        default:
            gcmPRINT("unknow command in gcoOS_DeviceControlWithDRM");
    }

    OutputBuffer->status = gcvSTATUS_OK;
    gcmFOOTER();
    return gcvSTATUS_OK;
OnError:
    OutputBuffer->status = status;
    gcmFOOTER();
    return status;
}
#endif

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
    gceSTATUS status = gcvSTATUS_OK;
    gcsDRIVER_ARGS args = {0};
    gcsTLS_PTR tls = gcvNULL;
    gctUINT32 interrupt_count = 0;
    gctINT device = -1;
    gctUINT devIdx = 0;
#if gcdENABLE_TTM
    gceHAL_COMMAND_CODES command = 0;
#endif

    gcmHEADER_ARG("IoControlCode=%u InputBuffer=0x%x "
                  "InputBufferSize=%lu OutputBuffer=0x%x OutputBufferSize=%lu",
                  IoControlCode, InputBuffer, InputBufferSize,
                  OutputBuffer, OutputBufferSize);

    if (gcPLS.os == gcvNULL)
    {
        gcmONERROR(gcvSTATUS_DEVICE);
    }

    gcmONERROR(gcoOS_GetTLS(&tls));
    devIdx = tls->currentDevIndex;

    /* Cast the interface. */
    if (IoControlCode == IOCTL_GCHAL_PROFILER_INTERFACE)
    {
        gcsHAL_PROFILER_INTERFACE_PTR inputBuffer = (gcsHAL_PROFILER_INTERFACE_PTR)InputBuffer;

#if gcdENABLE_TTM
        command = inputBuffer->command;
#endif
        if (!inputBuffer->ignoreTLS)
        {
            /* Set current hardware type */
            if (gcPLS.processID)
            {
                gcmONERROR(gcoOS_GetTLS(&tls));
                inputBuffer->hardwareType = tls->currentType;
                inputBuffer->coreIndex = tls->currentCoreIndex;
#if gcdENABLE_MULTI_DEVICE_MANAGEMENT
                inputBuffer->devIndex = tls->currentDevIndex;
#endif
            }
            else
            {
                inputBuffer->hardwareType = gcvHARDWARE_2D;
                inputBuffer->coreIndex = 0;
#if gcdENABLE_MULTI_DEVICE_MANAGEMENT
                inputBuffer->devIndex = 0;
#endif
            }
        }
    }
    else
    {
        gcsHAL_INTERFACE_PTR inputBuffer  = (gcsHAL_INTERFACE_PTR)InputBuffer;

#if gcdENABLE_TTM
        command = inputBuffer->command;
#endif
        if (!inputBuffer->ignoreTLS)
        {
            /* Set current hardware type */
            if (gcPLS.processID)
            {
                gcmONERROR(gcoOS_GetTLS(&tls));
                inputBuffer->hardwareType = tls->currentType;
                inputBuffer->coreIndex = tls->currentCoreIndex;
#if gcdENABLE_MULTI_DEVICE_MANAGEMENT
                inputBuffer->devIndex = tls->currentDevIndex;
#endif
            }
            else
            {
                inputBuffer->hardwareType = gcvHARDWARE_2D;
                inputBuffer->coreIndex = 0;
#if gcdENABLE_MULTI_DEVICE_MANAGEMENT
                inputBuffer->devIndex = 0;
#endif
            }
        }
    }


    /* Call kernel. */
    args.InputBuffer      = gcmPTR_TO_UINT64(InputBuffer);
    args.InputBufferSize  = InputBufferSize;
    args.OutputBuffer     = gcmPTR_TO_UINT64(OutputBuffer);
    args.OutputBufferSize = OutputBufferSize;

    device = gcPLS.os->devices[devIdx];

#if gcdENABLE_TTM
    if (command >= gcvHAL_ALLOCATE_LINEAR_VIDEO_MEMORY &&
        command <= gcvHAL_UNMAP_MEMORY) {
        gcmONERROR(gcoOS_DeviceControlWithDRM(Os, (gcsHAL_INTERFACE_PTR)InputBuffer, (gcsHAL_INTERFACE_PTR)OutputBuffer));
    } else {
#endif
        while (ioctl(device, IoControlCode, &args) < 0)
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
#if gcdENABLE_TTM
    }
#endif

    /* Get the status. */
    if (IoControlCode == IOCTL_GCHAL_PROFILER_INTERFACE)
    {
        gcsHAL_PROFILER_INTERFACE_PTR outputBuffer = (gcsHAL_PROFILER_INTERFACE_PTR)OutputBuffer;
        status = outputBuffer->status;
    }
    else
    {
        gcsHAL_INTERFACE_PTR outputBuffer = (gcsHAL_INTERFACE_PTR)OutputBuffer;
        status = outputBuffer->status;
    }

    /* Eliminate gcmONERROR on gcoOS_WaitSignal timeout errors. */
#if gcmIS_DEBUG(gcdDEBUG_CODE)
    if (IoControlCode == IOCTL_GCHAL_INTERFACE)
    {
        gcsHAL_INTERFACE_PTR inputBuffer  = (gcsHAL_INTERFACE_PTR)InputBuffer;

        if ((inputBuffer->command == gcvHAL_USER_SIGNAL) &&
            (inputBuffer->u.UserSignal.command == gcvUSER_SIGNAL_WAIT))
        {
            if (status == gcvSTATUS_TIMEOUT)
            {
                goto OnError;
            }
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
**  gcoOS_AllocateVideoMemory (OBSOLETE, do NOT use it)
**
**  Allocate contiguous video memory from the kernel.
**
**
**  INPUT:
**
**      gcoOS Os
**          Pointer to an gcoOS object.
**
**      gctBOOL InUserSpace
**          gcvTRUE to map the memory into the user space.
**
**      gctBOOL InCacheable
**          gcvTRUE to allocate the cacheable memory.
**
**
**      gctSIZE_T * Bytes
**          Pointer to the number of bytes to allocate.
**
**  OUTPUT:
**
**      gctSIZE_T * Bytes
**          Pointer to a variable that will receive the aligned number of bytes
**          allocated.
**
**      gctUINT32 * Address
**          Pointer to a variable that will receive the GPU address of
**          the allocated memory.
**
**      gctPOINTER * Logical
**          Pointer to a variable that will receive the logical address of the
**          allocation.
**
**      gctPOINTER * Handle
**          Pointer to a variable that will receive the node handle of the
**          allocation.
*/
gceSTATUS
gcoOS_AllocateVideoMemory(
    IN gcoOS Os,
    IN gctBOOL InUserSpace,
    IN gctBOOL InCacheable,
    IN OUT gctSIZE_T * Bytes,
    OUT gctUINT32 * Address,
    OUT gctPOINTER * Logical,
    OUT gctPOINTER * Handle
    )
{
    gceSTATUS status = gcvSTATUS_OK;
    gcsHAL_INTERFACE iface = {0};
    gctUINT32 flag = 0;

    gcmHEADER_ARG("InUserSpace=%d *Bytes=%lu",
                  InUserSpace, gcmOPT_VALUE(Bytes));

    /* Verify the arguments. */
    gcmVERIFY_ARGUMENT(Bytes != gcvNULL);
    gcmVERIFY_ARGUMENT(Address != gcvNULL);
    gcmVERIFY_ARGUMENT(Logical != gcvNULL);

    flag |= gcvALLOC_FLAG_CONTIGUOUS;

    if (InCacheable)
    {
        flag |= gcvALLOC_FLAG_CACHEABLE;
    }

    iface.ignoreTLS = gcvFALSE;
    iface.command     = gcvHAL_ALLOCATE_LINEAR_VIDEO_MEMORY;
    iface.u.AllocateLinearVideoMemory.bytes     = *Bytes;

    iface.u.AllocateLinearVideoMemory.alignment = 64;
    iface.u.AllocateLinearVideoMemory.pool      = gcvPOOL_DEFAULT;
    iface.u.AllocateLinearVideoMemory.type      = gcvVIDMEM_TYPE_BITMAP;
    iface.u.AllocateLinearVideoMemory.flag      = flag;

     /* Call kernel driver. */
    gcmONERROR(gcoOS_DeviceControl(
        gcvNULL,
        IOCTL_GCHAL_INTERFACE,
        &iface, gcmSIZEOF(iface),
        &iface, gcmSIZEOF(iface)
        ));

    /* Return allocated number of bytes. */
    *Bytes = (gctSIZE_T)iface.u.AllocateLinearVideoMemory.bytes;

    /* Return the handle of allocated Node. */
    *Handle = gcmUINT64_TO_PTR(iface.u.AllocateLinearVideoMemory.node);

    /* Fill in the kernel call structure. */
    iface.ignoreTLS = gcvFALSE;
    iface.engine = gcvENGINE_RENDER;
    iface.command = gcvHAL_LOCK_VIDEO_MEMORY;
    iface.u.LockVideoMemory.op = gcvLOCK_VIDEO_MEMORY_OP_LOCK |
                                 gcvLOCK_VIDEO_MEMORY_OP_MAP;
    iface.u.LockVideoMemory.node = iface.u.AllocateLinearVideoMemory.node;
    iface.u.LockVideoMemory.cacheable = InCacheable;

    /* Call the kernel. */
    gcmONERROR(gcoOS_DeviceControl(
                gcvNULL,
                IOCTL_GCHAL_INTERFACE,
                &iface, sizeof(iface),
                &iface, sizeof(iface)
                ));

    /* Success? */
    gcmONERROR(iface.status);

    /* Return GPU address. */
    if (gcoHARDWARE_IsFeatureAvailable(gcvNULL, gcvFEATURE_MMU))
    {
        *Address = iface.u.LockVideoMemory.address;
    }
    else
    {
        *Address = iface.u.LockVideoMemory.physicalAddress;
    }

    /* Return logical address. */
    *Logical = gcmUINT64_TO_PTR(iface.u.LockVideoMemory.memory);

    /* Success. */
    gcmFOOTER_ARG("*Bytes=%lu *Address=0x%x *Logical=0x%x",
                  *Bytes, *Address, *Logical);
    return gcvSTATUS_OK;

OnError:
    gcmTRACE(
        gcvLEVEL_ERROR,
        "%s(%d): failed to allocate %lu bytes",
        __FUNCTION__, __LINE__, *Bytes
        );

    /* Return the status. */
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gcoOS_FreeVideoMemory (OBSOLETE, do NOT use it)
**
**  Free contiguous video memory from the kernel.
**
**  INPUT:
**
**      gcoOS Os
**          Pointer to an gcoOS object.
**
**      gctPOINTER  Handle
**          Pointer to a variable that indicate the node of the
**          allocation.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gcoOS_FreeVideoMemory(
    IN gcoOS Os,
    IN gctPOINTER Handle
    )
{
    gcsHAL_INTERFACE iface = {0};
    gceSTATUS status = gcvSTATUS_OK;

    gcmHEADER_ARG("Os=0x%x Handle=0x%x",
                  Os, Handle);

    do
    {
        gcuVIDMEM_NODE_PTR Node = (gcuVIDMEM_NODE_PTR)Handle;

        /* Free first to set freed flag since asynchroneous unlock is needed for the contiguous memory. */
        iface.ignoreTLS = gcvFALSE;
        iface.command = gcvHAL_RELEASE_VIDEO_MEMORY;
        iface.u.ReleaseVideoMemory.node = gcmPTR_TO_UINT64(Node);

        /* Call kernel driver. */
        gcmONERROR(gcoOS_DeviceControl(
            gcvNULL,
            IOCTL_GCHAL_INTERFACE,
            &iface, gcmSIZEOF(iface),
            &iface, gcmSIZEOF(iface)
            ));

        /* Unlock the video memory node in asynchronous mode. */
        iface.engine = gcvENGINE_RENDER;
        iface.command = gcvHAL_UNLOCK_VIDEO_MEMORY;
        iface.u.UnlockVideoMemory.op = gcvLOCK_VIDEO_MEMORY_OP_UNLOCK |
                                       gcvLOCK_VIDEO_MEMORY_OP_UNMAP;
        iface.u.UnlockVideoMemory.node = gcmPTR_TO_UINT64(Node);
        iface.u.UnlockVideoMemory.type = gcvVIDMEM_TYPE_BITMAP;
        iface.u.UnlockVideoMemory.asynchroneous = gcvTRUE;

         /* Call kernel driver. */
        gcmONERROR(gcoOS_DeviceControl(
            gcvNULL,
            IOCTL_GCHAL_INTERFACE,
            &iface, gcmSIZEOF(iface),
            &iface, gcmSIZEOF(iface)
            ));

        /* Success? */
        gcmONERROR(iface.status);

        /* Do we need to schedule an event for the unlock? */
        if (iface.u.UnlockVideoMemory.asynchroneous)
        {
            iface.u.UnlockVideoMemory.asynchroneous = gcvFALSE;
            gcmONERROR(gcoHAL_ScheduleEvent(gcvNULL, &iface));
            gcmONERROR(gcoHAL_Commit(gcvNULL, gcvFALSE));
        }

        /* Success. */
        gcmFOOTER_NO();

        return gcvSTATUS_OK;
    }
    while (gcvFALSE);

OnError:
       gcmTRACE(
               gcvLEVEL_ERROR,
               "%s(%d): failed to free handle %lu",
               __FUNCTION__, __LINE__, Handle
               );

    /* Return the status. */
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gcoOS_LockVideoMemory (OBSOLETE, do NOT use it)
**
**  Lock contiguous video memory for access.
**
**  INPUT:
**
**      gcoOS Os
**          Pointer to an gcoOS object.
**
**      gctPOINTER  Handle
**          Pointer to a variable that indicate the node of the
**          allocation.
**
**      gctBOOL InUserSpace
**          gcvTRUE to map the memory into the user space.
**
**      gctBOOL InCacheable
**          gcvTRUE to allocate the cacheable memory.
**
**  OUTPUT:
**
**      gctUINT32 * Address
**          Pointer to a variable that will receive the GPU addresses of
**          the allocated memory.
**
**      gctPOINTER * Logical
**          Pointer to a variable that will receive the logical address of the
**          allocation.
*/
gceSTATUS
gcoOS_LockVideoMemory(
    IN gcoOS Os,
    IN gctPOINTER Handle,
    IN gctBOOL InUserSpace,
    IN gctBOOL InCacheable,
    OUT gctUINT32 * Address,
    OUT gctPOINTER * Logical
    )
{
    gceSTATUS status = gcvSTATUS_OK;
    gcsHAL_INTERFACE iface = {0};
    gceHARDWARE_TYPE type;
    gcuVIDMEM_NODE_PTR Node = gcvNULL;

    gcmHEADER_ARG("Handle=%d,InUserSpace=%d InCacheable=%d",
                  Handle, InUserSpace, InCacheable);

    /* Verify the arguments. */
    gcmVERIFY_ARGUMENT(Handle != gcvNULL);
    gcmVERIFY_ARGUMENT(Address != gcvNULL);
    gcmVERIFY_ARGUMENT(Logical != gcvNULL);

    Node = (gcuVIDMEM_NODE_PTR)Handle;

    gcmVERIFY_OK(gcoHAL_GetHardwareType(gcvNULL, &type));
    gcoHAL_SetHardwareType(gcvNULL, gcvHARDWARE_3D);

    /* Fill in the kernel call structure. */
    iface.ignoreTLS = gcvFALSE;
    iface.engine = gcvENGINE_RENDER;
    iface.command = gcvHAL_LOCK_VIDEO_MEMORY;
    iface.u.LockVideoMemory.op = gcvLOCK_VIDEO_MEMORY_OP_LOCK |
                                 gcvLOCK_VIDEO_MEMORY_OP_MAP;
    iface.u.LockVideoMemory.node = gcmPTR_TO_UINT64(Node);
    iface.u.LockVideoMemory.cacheable = InCacheable;

    /* Call the kernel. */
    gcmONERROR(gcoOS_DeviceControl(
                gcvNULL,
                IOCTL_GCHAL_INTERFACE,
                &iface, sizeof(iface),
                &iface, sizeof(iface)
                ));

    /* Success? */
    gcmONERROR(iface.status);

    /* Return GPU address. */
    *Address = (gctUINT32)iface.u.LockVideoMemory.address;

    /* Return logical address. */
    *Logical = gcmUINT64_TO_PTR(iface.u.LockVideoMemory.memory);

    gcoHAL_SetHardwareType(gcvNULL, type);

    /* Success. */
    gcmFOOTER_ARG("*Address=0x%x *Logical=0x%x", *Address, *Logical);
    return gcvSTATUS_OK;

OnError:
    gcmTRACE(
        gcvLEVEL_ERROR,
        "%s(%d): failed to lock handle %x",
        __FUNCTION__, __LINE__, Handle
        );

    gcoHAL_SetHardwareType(gcvNULL, type);

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
**              gcvFILE_READTEXT    - Open an existing text file for read only.
**              gcvFILE_READWRITE   - Open an existing file for reading and writing.
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
        "rb+",
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

gceSTATUS
gcoOS_Remove(
    IN gcoOS Os,
    IN gctCONST_STRING FileName
    )
{
    int ret;
    gceSTATUS status = gcvSTATUS_OK;
    gcmHEADER_ARG("FileName=%s", FileName);

    /* Verify the arguments. */
    gcmDEBUG_VERIFY_ARGUMENT(FileName != gcvNULL);

    ret = remove((const char *)FileName);

    if (ret != 0)
    {
        status = gcvSTATUS_GENERIC_IO;
    }

    /* Success. */
    gcmFOOTER();
    return status;
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
    gceSTATUS status = gcvSTATUS_OK;
    size_t byteRead;

    gcmHEADER_ARG("File=0x%x ByteCount=%lu Data=0x%x",
                  File, ByteCount, Data);

    /* Verify the arguments. */
    gcmDEBUG_VERIFY_ARGUMENT(File != gcvNULL);
    gcmDEBUG_VERIFY_ARGUMENT(ByteCount > 0);
    gcmDEBUG_VERIFY_ARGUMENT(Data != gcvNULL);

    /* Read the data from the file. */
    byteRead = fread(Data, 1, ByteCount, (FILE *) File);

    if (byteRead != ByteCount)
    {
        if (ferror((FILE *) File))
        {
            status = gcvSTATUS_GENERIC_IO;
            clearerr((FILE *) File);
        }
        else if (feof((FILE *) File))
        {
            clearerr((FILE *) File);
        }
        else
        {
            status = gcvSTATUS_GENERIC_IO;
        }
    }

    if (ByteRead != gcvNULL)
    {
        *ByteRead = (gctSIZE_T) byteRead;
    }

    gcmFOOTER_ARG("status=%d", status);
    return status;
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
    gceSTATUS status = gcvSTATUS_OK;
    size_t byteWritten;

    gcmHEADER_ARG("File=0x%x ByteCount=%lu Data=0x%x",
                  File, ByteCount, Data);

    /* Verify the arguments. */
    gcmDEBUG_VERIFY_ARGUMENT(File != gcvNULL);
    gcmDEBUG_VERIFY_ARGUMENT(ByteCount > 0);
    gcmDEBUG_VERIFY_ARGUMENT(Data != gcvNULL);

    /* Write the data to the file. */
    byteWritten = fwrite(Data, 1, ByteCount, (FILE *) File);

    if (byteWritten != ByteCount)
    {
        if (ferror((FILE *) File))
        {
            status = gcvSTATUS_GENERIC_IO;
            clearerr((FILE *) File);
        }
        else if (feof((FILE *) File))
        {
            status = gcvSTATUS_DATA_TOO_LARGE;
            clearerr((FILE *) File);
        }
        else
        {
            status = gcvSTATUS_GENERIC_IO;
        }
    }

    gcmFOOTER_ARG("status=%d", status);
    return status;
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
    int flags;
    int err;
    gceSTATUS status = gcvSTATUS_OK;

    gcmHEADER_ARG("File=%p Shared=%d Block=%d", File, Shared, Block);

    flags  = Shared ? LOCK_SH : LOCK_EX;
    flags |= Block ? 0 : LOCK_NB;

    err = flock(fileno((FILE *)File), flags);

    if (err)
    {
        if (errno == EWOULDBLOCK)
        {
            gcmONERROR(gcvSTATUS_LOCKED);
        }
        else if (errno == EINTR)
        {
            gcmONERROR(gcvSTATUS_INTERRUPTED);
        }
        else
        {
            gcmONERROR(gcvSTATUS_INVALID_ARGUMENT);
        }
    }

    gcmFOOTER_NO();
    return gcvSTATUS_OK;

OnError:
    gcmFOOTER_ARG("status=%d", status);
    return status;
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
    int err;
    gceSTATUS status;

    gcmHEADER_ARG("File=%p", File);

    err = flock(fileno((FILE *)File), LOCK_UN);

    if (err)
    {
        gcmONERROR(gcvSTATUS_INVALID_ARGUMENT);
    }

    gcmFOOTER_NO();
    return gcvSTATUS_OK;

OnError:
    gcmFOOTER_ARG("status=%d", status);
    return status;
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

    /* Linux select() will overwrite the struct on return */
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
        socklen_t len = sizeof(error);

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
#ifdef ANDROID
    static char properties[512];
    static int currentIndex;
    static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
#if ANDROID_SDK_VERSION >= 30
    static gctCONST_STRING vendor_prefix = "vendor.";
    static gctCONST_STRING ro_prefix = "ro.";
#endif

    gctSTRING value = gcvNULL;
    gctSTRING property_name = gcvNULL;

    {
        int property_name_len = 0;
#if ANDROID_SDK_VERSION >= 30
        // For Android 11, we need add "vendor." as prefix for every property for VTS reason
        property_name_len = strlen(VarName) + strlen(vendor_prefix) + 1;
        property_name = (gctSTRING)malloc(property_name_len);
        memset(property_name, 0, property_name_len);
        if (strncmp(VarName, ro_prefix, strlen(ro_prefix)))
        {
            strncat(property_name, vendor_prefix, strlen(vendor_prefix));
            strncat(property_name + strlen(vendor_prefix), VarName, strlen(VarName));
        }
        else
        {
            property_name = strncpy(property_name, VarName, strlen(VarName));
        }
#else
        property_name_len = strlen(VarName) + 1;
        property_name = (gctSTRING)malloc(property_name_len);
        memset(property_name, 0, property_name_len);
        property_name = strncpy(property_name, VarName, strlen(VarName));
#endif
    }

    gcmHEADER_ARG("property_name=%s", gcmOPT_STRING(property_name));

    pthread_mutex_lock(&mutex);

    /* Get property value. */
    if (property_get(property_name, &properties[currentIndex], NULL) != 0)
    {
        /* Property exists. */
        value = &properties[currentIndex];

        /* Pointer to space for next property. */
        currentIndex += strlen(value) + 1;

        if (currentIndex + PROPERTY_VALUE_MAX >= 512)
        {
            /* Rewind, will overrite old values. */
            currentIndex = 0;
        }
    }

    pthread_mutex_unlock(&mutex);

    *Value = value;

    /* Success. */
    free(property_name);
    gcmFOOTER_ARG("*Value=%s", gcmOPT_STRING(*Value));
    return gcvSTATUS_OK;
#else
    gcmHEADER_ARG("VarName=%s", gcmOPT_STRING(VarName));

    *Value = getenv(VarName);

    /* Success. */
    gcmFOOTER_ARG("*Value=%s", gcmOPT_STRING(*Value));
    return gcvSTATUS_OK;
#endif
}

/* Set environment variable value. */
gceSTATUS gcoOS_SetEnv(
    IN gcoOS Os,
    IN gctCONST_STRING VarName,
    IN gctSTRING Value
    )
{
#ifdef ANDROID
    return gcvSTATUS_OK;
#endif
    gcmHEADER_ARG("VarName=%s", gcmOPT_STRING(VarName));

    setenv(VarName, Value, 1);

    /* Success. */
    gcmFOOTER_NO();
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
    gceSTATUS status;

    gcmHEADER_ARG("File=0x%x Position=%u", File, Position);

    /* Verify the arguments. */
    gcmDEBUG_VERIFY_ARGUMENT(File != gcvNULL);

    /* Set file position. */
    gcmONERROR(fseek((FILE *) File, Position, SEEK_SET));

OnError:
    /* Success. */
    gcmFOOTER_NO();
    return status;
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

    if (pthread_create(&thread, gcvNULL, Worker, Argument) != 0)
    {
        gcmFOOTER_ARG("status=%d", gcvSTATUS_OUT_OF_RESOURCES);
        return gcvSTATUS_OUT_OF_RESOURCES;
    }

    *Thread = (gctPOINTER)(uintptr_t)thread;

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

    pthread_join((pthread_t)(uintptr_t)Thread, gcvNULL);

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

    pthread_mutexattr_settype(&mta, PTHREAD_MUTEX_RECURSIVE);

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
                usleep(1000);
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
    gcmHEADER_ARG("Delay=%u", Delay);

    /* Sleep for a while. */
    usleep((Delay == 0) ? 1 : (1000 * Delay));

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
    gcmHEADER_ARG("Delay=%u", Delay);

    /* Sleep for a while. */
    usleep(Delay);

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
#ifdef __USE_XOPEN2K8
    n = (gctSIZE_T)strnlen(Destination, DestinationSize);
#else
    n = (gctSIZE_T)strlen(Destination);
#endif
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
        gcmFOOTER_NO();
        return gcvSTATUS_DATA_TOO_LARGE;
    }

    gcmFOOTER_NO();
    return gcvSTATUS_OK;
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

    gcmFOOTER_ARG("*Double=%lf", *Double);
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
    status = gcoOS_PrintStrVSafe(String, StringSize,
                                 Offset,
                                 Format, arguments);


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
                gcvNULL, length + 3 + 1, (gctPOINTER *) &library
                ));

            /* Copy the library name to the temporary string buffer. */
            strncpy(library, Library, length + 1);

            /* Append the ".so" to the temporary string buffer. */
            strncat(library, ".so", 3 + 1);

            /* Replace the library name. */
            Library = library;
        }

        *Handle = dlopen(Library, RTLD_NOW);

        /* Failed? */
        if (*Handle == gcvNULL)
        {
            gcmTRACE(
                gcvLEVEL_VERBOSE, "%s(%d): %s", __FUNCTION__, __LINE__, Library
                );

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
    gceSTATUS status = gcvSTATUS_OK;
    if (gcPLS.bMemoryProfile && gcPLS.os)
    {
        /* cache video memory infomation to PLS when Destroy OS */
        gcsHAL_INTERFACE iface;

        gcoOS_ZeroMemory(&iface, sizeof(iface));

        iface.command = gcvHAL_DATABASE;
        iface.u.Database.processID = gcmPTR2INT32(gcoOS_GetCurrentProcessID());
        iface.u.Database.validProcessID = gcvTRUE;

        /* Call kernel service. */
        gcmONERROR(gcoOS_DeviceControl(
            gcvNULL,
            IOCTL_GCHAL_INTERFACE,
            &iface, gcmSIZEOF(iface),
            &iface, gcmSIZEOF(iface)
            ));

        gcPLS.video_currentSize     = iface.u.Database.vidMem.counters.bytes      + iface.u.Database.nonPaged.counters.bytes;
        gcPLS.video_maxAllocSize    = iface.u.Database.vidMem.counters.maxBytes   + iface.u.Database.nonPaged.counters.maxBytes;
        gcPLS.video_allocSize       = iface.u.Database.vidMem.counters.totalBytes + iface.u.Database.nonPaged.counters.totalBytes;
        gcPLS.video_freeSize        = iface.u.Database.vidMem.counters.totalBytes - iface.u.Database.vidMem.counters.bytes + iface.u.Database.nonPaged.counters.totalBytes - iface.u.Database.nonPaged.counters.bytes;
        gcPLS.video_allocCount      = iface.u.Database.vidMem.counters.allocCount + iface.u.Database.nonPaged.counters.allocCount;
        gcPLS.video_freeCount       = iface.u.Database.vidMem.counters.freeCount  + iface.u.Database.nonPaged.counters.freeCount;
    }

OnError:
    return status;
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
**      gctBOOL ProfileMode
**          Vivante profiler mode.
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
    IN gceProfilerMode ProfileMode,
    IN gctCONST_STRING FileName
    )
{
    gcsHAL_PROFILER_INTERFACE iface = {0};

    if (strlen(FileName) >= gcdMAX_PROFILE_FILE_NAME)
    {
        return gcvSTATUS_INVALID_ARGUMENT;
    }

    /* Initialize the gcsHAL_INTERFACE structure. */
    iface.ignoreTLS = gcvFALSE;
    iface.command = gcvHAL_SET_PROFILE_SETTING;
    iface.u.SetProfileSetting.enable = Enable;
    iface.u.SetProfileSetting.profileMode = ProfileMode;

    /* Call the kernel. */
    return gcoOS_DeviceControl(gcvNULL,
                               IOCTL_GCHAL_PROFILER_INTERFACE,
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
    return (gctHANDLE)(gctUINTPTR_T) getpid();
}

gctHANDLE
gcoOS_GetCurrentThreadID(
    void
    )
{
    return (gctHANDLE)gcmINT2PTR(gcmGETTHREADID());
}

/*----------------------------------------------------------------------------*/
/*----------------------------------- Time -----------------------------------*/

/*******************************************************************************
**
**  gcoOS_GetTicks
**
**  Get the number of milliseconds since the system started.
**
**  INPUT:
**
**  OUTPUT:
**
*/
gctUINT32
gcoOS_GetTicks(
    void
    )
{
    struct timeval tv;

    /* Return the time of day in milliseconds. */
    gettimeofday(&tv, 0);
    return (tv.tv_sec * 1000) + (tv.tv_usec / 1000);
}

/*******************************************************************************
**
**  gcoOS_GetTime
**
**  Get the number of microseconds since 1970/1/1.
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
    struct timeval tv;

    /* Return the time of day in microseconds. */
    gettimeofday(&tv, 0);
    *Time = (tv.tv_sec * (gctUINT64)1000000) + tv.tv_usec;
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
    struct rusage usage;

    /* Return CPU time in microseconds. */
    if (getrusage(RUSAGE_SELF, &usage) == 0)
    {
        *CPUTime  = usage.ru_utime.tv_sec * (gctUINT64)1000000 + usage.ru_utime.tv_usec;
        *CPUTime += usage.ru_stime.tv_sec * (gctUINT64)1000000 + usage.ru_stime.tv_usec;
        return gcvSTATUS_OK;
    }
    else
    {
        *CPUTime = 0;
        return gcvSTATUS_INVALID_ARGUMENT;
    }
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
    struct rusage usage;

    /* Return memory usage. */
    if (getrusage(RUSAGE_SELF, &usage) == 0)
    {
        *MaxRSS = usage.ru_maxrss;
        *IxRSS  = usage.ru_ixrss;
        *IdRSS  = usage.ru_idrss;
        *IsRSS  = usage.ru_isrss;
        return gcvSTATUS_OK;
    }
    else
    {
        *MaxRSS = 0;
        *IxRSS  = 0;
        *IdRSS  = 0;
        *IsRSS  = 0;
        return gcvSTATUS_INVALID_ARGUMENT;
    }
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
    gcsHAL_INTERFACE iface = {0};
    gceSTATUS status = gcvSTATUS_OK;

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
    gcsHAL_INTERFACE iface = {0};
    gceSTATUS status = gcvSTATUS_OK;

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
    IN gctSIZE_T Offset,
    IN gctSIZE_T Bytes,
    IN gceCACHEOPERATION Operation
    )
{
    gcsHAL_INTERFACE ioctl = {0};
    gceSTATUS status = gcvSTATUS_OK;

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
            gcmPRINT("PROFILE ENTRY: index=%u size=%u bytes=%d level=%d",
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
            gcmPRINT("PROFILE EXIT: index=%u size=%u bytes=%d level=%d",
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
    gceSTATUS status = gcvSTATUS_OK;
    gcsHAL_INTERFACE iface = {0};

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
    gceSTATUS status = gcvSTATUS_OK;
    gcsHAL_INTERFACE iface = {0};

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
    gceSTATUS status = gcvSTATUS_OK;
    gcsHAL_INTERFACE iface = {0};

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
    gceSTATUS status = gcvSTATUS_OK;
    gcsHAL_INTERFACE iface = {0};

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

    if (gcmIS_SUCCESS(status))
    {
        if (iface.u.UserSignal.status == gcvSIGNAL_RECOVERY)
        {
            status = gcvSTATUS_RECOVERY;
        }
        else if (iface.u.UserSignal.status == gcvSIGNAL_CANCEL)
        {
            status = gcvSTATUS_CANCEL_JOB;
        }
    }

    gcmFOOTER_ARG("Signal=0x%x status=%d", Signal, status);
    return status;
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
    gceSTATUS status = gcvSTATUS_OK;
    gcsHAL_INTERFACE iface = {0};

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
    gceSTATUS status = gcvSTATUS_OK;
    gcsHAL_INTERFACE iface = {0};

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
            goto OnError;
#else
            struct sigaction temp;
            struct sigaction sigact;

            if ((sigaction (SIGFPE, NULL, &temp)) != 0)
                goto OnError;

            if (temp.sa_handler != (void *)_SignalHandlerForSIGFPEWhenSignalCodeIs0)
            {
                sigact.sa_handler = (void *)_SignalHandlerForSIGFPEWhenSignalCodeIs0;
                sigact.sa_flags = SA_RESTART | SA_SIGINFO;
                sigemptyset(&sigact.sa_mask);

                if ((sigaction(SIGFPE, &sigact, (struct sigaction *)NULL)) != 0)
                    goto OnError;
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

OnError:
    gcmFOOTER();
    status = gcvSTATUS_NOT_SUPPORTED;
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
    gctFILE fHandle = 0;
    gctUINT offset = 0;
    gctSIZE_T bytesRead = 0;
    gctCHAR procEntryName[gcdMAX_PATH];
    gctCHAR Name[gcdMAX_ARGUMENT_SIZE * gcdMAX_ARGUMENT_COUNT];
    gceSTATUS status = gcvSTATUS_OK;
    gctUINT32 begPos = 0;
    gctUINT32 i = 0;
    gctUINT32 argc = 0;
    gctBOOL   skip = gcvTRUE;

    gctHANDLE Pid = gcoOS_GetCurrentProcessID();

    gcmHEADER_ARG("Name=%s Pid=%d", Name, Pid);

    if (!Argv || !Argc || MaxArgc <= 0 || MaxSizePerArg <= 0)
    {
        gcmONERROR(gcvSTATUS_INVALID_ARGUMENT);
    }

    /* Construct the proc cmdline entry */
    gcmONERROR(gcoOS_PrintStrSafe(procEntryName,
                                  gcdMAX_PATH,
                                  &offset,
                                  "/proc/%d/cmdline",
                                  Pid));

    /* Open the procfs entry */
    gcmONERROR(gcoOS_Open(gcvNULL,
                          procEntryName,
                          gcvFILE_READ,
                          &fHandle));


    gcmONERROR(gcoOS_Read(gcvNULL,
                          fHandle,
                          gcdMAX_ARGUMENT_SIZE - 1,
                          Name,
                          &bytesRead));

    Name[bytesRead] = '\0';

    for (i = 0; i < bytesRead; ++i)
    {
        gctCHAR c = Name[i];

        if (0x0 == c) {
            if (skip)
            {
                skip = gcvFALSE;
                begPos = i + 1;
                continue;
            }
            gctSIZE_T perArgSize = gcmMIN((i - begPos), MaxSizePerArg - 1);
            gcoOS_MemCopy(Argv[argc], &(Name[begPos]), perArgSize);
            Argv[argc][perArgSize] = '\0';
            ++argc;
            begPos = i + 1;

            if (argc >= MaxArgc)
            {
                break;
            }
        }
    }

    *Argc = argc;

OnError:
    if (fHandle)
    {
        gcoOS_Close(gcvNULL, fHandle);
    }

    /* Return the status. */
    gcmFOOTER();
    return status;
}

gceSTATUS
gcoOS_QueryCurrentProcessName(
    OUT gctSTRING Name,
    IN gctSIZE_T Size
    )
{
    gctFILE fHandle = 0;
    gctUINT offset = 0;
    gctSIZE_T bytesRead = 0;
    char procEntryName[gcdMAX_PATH];
    gctSTRING processName = gcvNULL;
    gceSTATUS status = gcvSTATUS_OK;

    gctHANDLE Pid = gcoOS_GetCurrentProcessID();

    gcmHEADER_ARG("Name=%s Pid=%d", Name, Pid);

    if (!Name || Size <= 0)
    {
        gcmONERROR(gcvSTATUS_INVALID_ARGUMENT);
    }

    /* Construct the proc cmdline entry */
    gcmONERROR(gcoOS_PrintStrSafe(procEntryName,
                                  gcdMAX_PATH,
                                  &offset,
                                  "/proc/%d/cmdline",
                                  (int) (gctUINTPTR_T) Pid));

    /* Open the procfs entry */
    gcmONERROR(gcoOS_Open(gcvNULL,
                          procEntryName,
                          gcvFILE_READ,
                          &fHandle));


    gcmONERROR(gcoOS_Read(gcvNULL,
                          fHandle,
                          Size - 1,
                          Name,
                          &bytesRead));

    Name[bytesRead] = '\0';

    gcoOS_GetEnv(gcvNULL, "VIV_PROCESS_NAME", &processName);
    if (processName)
    {
        gctSIZE_T len;
        gcoOS_StrLen(processName, &len);
        gcoOS_StrCopySafe(Name, len+1, processName);
    }

OnError:
    if (fHandle)
    {
        gcoOS_Close(gcvNULL, fHandle);
    }

    /* Return the status. */
    gcmFOOTER();
    return status;
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

#if defined(ANDROID)
static gceSTATUS
ParseSymbol(
    IN gctCONST_STRING Library,
    IN gcsSYMBOLSLIST_PTR Symbols
    )
{
    gceSTATUS status = gcvSTATUS_FALSE;
    int fp;
    Elf32_Ehdr ehdr;
    Elf32_Shdr shdr;
    Elf32_Sym  sym;
    char secNameTab[2048];
    char *symNameTab = gcvNULL;
    int i = 0, j = 0, size_dynsym = 0, match = 0;
    unsigned int offset_dynsym = 0, offset_dynstr = 0, size_dynstr = 0;

    fp = open(Library, O_RDONLY);
    if (fp == -1)
        return gcvSTATUS_FALSE;

    if (read(fp, &ehdr, sizeof(Elf32_Ehdr)) == 0)
    {
        close(fp);
        return gcvSTATUS_FALSE;
    }

    if (ehdr.e_type != ET_DYN)
    {
        close(fp);
        return gcvSTATUS_FALSE;
    }

    if (lseek(fp, 0, SEEK_END) > 1024 * 1024 * 10)
    {
        close(fp);
        return gcvSTATUS_SKIP;
    }

    lseek(fp, ehdr.e_shoff + ehdr.e_shstrndx * 40, SEEK_SET);
    if (read(fp, &shdr, ehdr.e_shentsize) == 0)
    {
        close(fp);
        return gcvSTATUS_FALSE;
    }

    lseek(fp, (unsigned int)shdr.sh_offset, SEEK_SET);
    if (read(fp, secNameTab, shdr.sh_size) == 0)
    {
        close(fp);
        return gcvSTATUS_FALSE;
    }

    secNameTab[sizeof(secNameTab) - 1] = '\0';

    lseek(fp, ehdr.e_shoff, SEEK_SET);
    for (i = 0; i < ehdr.e_shnum; i++)
    {
        if (read(fp, &shdr, (ssize_t)ehdr.e_shentsize) == 0)
        {
            close(fp);
            return gcvSTATUS_FALSE;
        }
        if (sizeof(secNameTab) <= shdr.sh_name)
        {
            close(fp);
            return gcvSTATUS_FALSE;
        }
        if (strcmp(secNameTab + shdr.sh_name, ".dynsym") == 0)
        {
            offset_dynsym = (unsigned int)shdr.sh_offset;
            size_dynsym = (int)(shdr.sh_size / shdr.sh_entsize);
        }
        else if(strcmp(secNameTab + shdr.sh_name, ".dynstr") == 0)
        {
            offset_dynstr = (unsigned int)shdr.sh_offset;
            size_dynstr = (unsigned int)shdr.sh_size;
        }
    }

    if (size_dynstr == 0)
    {
        close(fp);
        return gcvSTATUS_FALSE;
    }
    else
        symNameTab = (char *)malloc(size_dynstr);

    if ((lseek(fp, (off_t)offset_dynstr, SEEK_SET) != (off_t)offset_dynstr) || (offset_dynstr == 0))
    {
        free(symNameTab);
        symNameTab = gcvNULL;
        close(fp);
        return gcvSTATUS_FALSE;
    }
    if (read(fp, symNameTab, (size_t)(size_dynstr)) == 0)
    {
        free(symNameTab);
        symNameTab = gcvNULL;
        close(fp);
        return gcvSTATUS_FALSE;
    }

    if (size_dynsym > 8 * 1000)
    {
        free(symNameTab);
        symNameTab = gcvNULL;
        close(fp);
        return gcvSTATUS_MISMATCH;
    }

    *(symNameTab + size_dynstr - 1) = '\0';

    lseek(fp, (off_t)offset_dynsym, SEEK_SET);
    for (i = 0; i < size_dynsym; i ++)
    {
        if (read(fp, &sym, (size_t)16) > 0)
        {
            for (j = 0; j < 10 && Symbols->symList[j]; j ++ )
            {
                gctCHAR *p, buffers[1024];
                p = buffers;
                buffers[0]='\0';
                strcat(buffers, Symbols->symList[j]);

                while (*p)
                {
                    *p = ~(*p);
                    p++;
                }

                if (size_dynstr <= sym.st_name)
                {
                    free(symNameTab);
                    symNameTab = gcvNULL;
                    close(fp);
                    return gcvSTATUS_FALSE;
                }

                if (strcmp(symNameTab + sym.st_name, buffers) == 0)
                    match++;
            }

            if (match >= j)
            {
                free(symNameTab);
                symNameTab = gcvNULL;
                close(fp);
                return gcvSTATUS_TRUE;
            }
        }
        else
        {
            status = gcvSTATUS_FALSE;
            break;
        }
    }

    if (symNameTab != gcvNULL)
    {
        free(symNameTab);
        symNameTab = gcvNULL;
    }

    close(fp);

    if (gcmIS_ERROR(status))
    {
        status = gcvSTATUS_FALSE;
    }
    return status;
}

gceSTATUS
gcoOS_DetectProgrameByEncryptedSymbols(
    IN gcsSYMBOLSLIST_PTR Symbols
    )
{
    gceSTATUS status = gcvSTATUS_FALSE;

    /* Detect shared library path. */
    char path[256] = "/data/data/";
    char * s = NULL, * f = NULL;
    ssize_t size;

    s = path + strlen(path);

    /* Open cmdline file. */
    int fd = open("/proc/self/cmdline", O_RDONLY);

    if (fd < 0)
    {
        gcmPRINT("open /proc/self/cmdline failed");
        return gcvSTATUS_FALSE;
    }

    /* Connect cmdline string to path. */
    size = read(fd, s, 64);
    close(fd);

    if (size < 0)
    {
        gcmPRINT("read /proc/self/cmdline failed");
        return gcvSTATUS_FALSE;
    }

    s[size] = '\0';

    f = strstr(s, ":");
    size = (f != NULL) ? (f - s) : size;

    s[size] = '\0';

    /* Connect /lib. */
    strcat(path, "/lib");

    /* Open shared library path. */
    DIR * dir = opendir(path);

    if (!dir)
    {
        return gcvSTATUS_FALSE;
    }

    //const char * found = NULL;
    struct dirent * dp;
    int count = 0;

    while ((dp = readdir(dir)) != NULL)
    {
        if (dp->d_type == DT_REG && ++count > 8)
        {
            status = gcvSTATUS_SKIP;
            goto OnError;
        }
    }
    rewinddir(dir);

    while ((dp = readdir(dir)) != NULL)
    {
        /* Only test regular file. */
        if (dp->d_type == DT_REG)
        {
            char buf[256];

            snprintf(buf, 256, "%s/%s", path, dp->d_name);

            gcmONERROR(ParseSymbol(buf, Symbols));

            if (status == gcvSTATUS_MISMATCH)
                continue;
            else if (status)
                break;
        }
    }

OnError:

    closedir(dir);

    if (gcmIS_ERROR(status))
    {
        status = gcvSTATUS_FALSE;
    }

    return status;
}
#endif

#if defined(ANDROID)
#include <sync/sync.h>
#else
int sync_wait(int fd, int timeout)
{
    struct pollfd fds;
    int ret;

    if (fd < 0) {
        errno = EINVAL;
        return -1;
    }

    fds.fd = fd;
    fds.events = POLLIN;

    do {
        ret = poll(&fds, 1, timeout);
        if (ret > 0) {
            if (fds.revents & (POLLERR | POLLNVAL)) {
                errno = EINVAL;
                return -1;
            }
            return 0;
        } else if (ret == 0) {
            errno = ETIME;
            return -1;
        }
    } while (ret == -1 && (errno == EINTR || errno == EAGAIN));

    return ret;
}
#endif

gceSTATUS
gcoOS_CreateNativeFence(
    IN gcoOS Os,
    IN gctSIGNAL Signal,
    OUT gctINT * FenceFD
    )
{
    gceSTATUS status = gcvSTATUS_OK;
    gcsHAL_INTERFACE iface = {0};

    gcmHEADER_ARG("Os=0x%x Signal=%d",
                  Os, (gctUINT) (gctUINTPTR_T) Signal);

    /* Call kernel API to dup signal to native fence fd. */
    iface.ignoreTLS                   = gcvFALSE;
    iface.command                     = gcvHAL_CREATE_NATIVE_FENCE;
    iface.u.CreateNativeFence.signal  = gcmPTR_TO_UINT64(Signal);
    iface.u.CreateNativeFence.fenceFD = -1;

    gcmONERROR(gcoOS_DeviceControl(
        gcvNULL,
        IOCTL_GCHAL_INTERFACE,
        &iface, gcmSIZEOF(iface),
        &iface, gcmSIZEOF(iface)
        ));

    /* Return fence fd. */
    *FenceFD = iface.u.CreateNativeFence.fenceFD;

    /* Success. */
    gcmFOOTER_ARG("*FenceFD=%d", iface.u.CreateNativeFence.fenceFD);
    return gcvSTATUS_OK;

OnError:
    *FenceFD = -1;

    /* Return the status. */
    gcmFOOTER();
    return status;
}

gceSTATUS
gcoOS_ClientWaitNativeFence(
    IN gcoOS Os,
    IN gctINT FenceFD,
    IN gctUINT32 Timeout
    )
{
    int err;
    int wait;
    gceSTATUS status;

    gcmASSERT(FenceFD != -1);
    gcmHEADER_ARG("Os=0x%x FenceFD=%d Timeout=%d", Os, FenceFD, Timeout);

    wait = (Timeout == gcvINFINITE) ? -1
         : (int) Timeout;

    /* err = ioctl(FenceFD, SYNC_IOC_WAIT, &wait); */
    err = sync_wait(FenceFD, wait);

    switch (err)
    {
    case 0:
        status = gcvSTATUS_OK;
        break;

    case -1:
        /*
         * libc ioctl:
         * On error, -1 is returned, and errno is set appropriately.
         * 'errno' is positive.
         */
        if (errno == ETIME)
        {
            status = gcvSTATUS_TIMEOUT;
            break;
        }

    default:
        status = gcvSTATUS_GENERIC_IO;
        break;
    }

    gcmFOOTER_ARG("status=%d", status);
    return status;
}

gceSTATUS
gcoOS_WaitNativeFence(
    IN gcoOS Os,
    IN gctINT FenceFD,
    IN gctUINT32 Timeout
    )
{
    gceSTATUS status = gcvSTATUS_OK;
    gcsHAL_INTERFACE iface = {0};

    gcmASSERT(FenceFD != -1);
    gcmHEADER_ARG("Os=0x%x FenceFD=%d Timeout=%d", Os, FenceFD, Timeout);

    /* Call kernel API to dup signal to native fence fd. */
    iface.ignoreTLS                 = gcvFALSE;
    iface.command                   = gcvHAL_WAIT_NATIVE_FENCE;
    iface.u.WaitNativeFence.fenceFD = FenceFD;
    iface.u.WaitNativeFence.timeout = Timeout;

    gcmONERROR(gcoOS_DeviceControl(
        gcvNULL,
        IOCTL_GCHAL_INTERFACE,
        &iface, gcmSIZEOF(iface),
        &iface, gcmSIZEOF(iface)
        ));

OnError:
    gcmFOOTER_ARG("status=%d", status);
    return status;
}

gceSTATUS
gcoOS_CPUPhysicalToGPUPhysical(
    IN gctPHYS_ADDR_T CPUPhysical,
    OUT gctPHYS_ADDR_T * GPUPhysical
    )
{
    gcoOS os = gcPLS.os;
    gcoPLATFORM platform;

    gcmHEADER();
    gcmVERIFY_ARGUMENT(os);

    platform = &os->platform;

    if (platform->ops->getGPUPhysical)
    {
        platform->ops->getGPUPhysical(platform, CPUPhysical, GPUPhysical);
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
#if VIVANTE_PROFILER_SYSTEM_MEMORY

#if gcdGC355_MEM_PRINT
    gcoOS os;
    if (gcPLS.os != gcvNULL)
    {
        os = gcPLS.os;

        os->oneSize = 0;
        os->oneRecording = 1;
    }
#endif

#endif
}

gctINT32
gcoOS_EndRecordAllocation(void)
{
    gctINT32   result = 0;
#if VIVANTE_PROFILER_SYSTEM_MEMORY

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

#endif
    return result;
}

void
gcoOS_AddRecordAllocation(gctSIZE_T Size)
{
#if VIVANTE_PROFILER_SYSTEM_MEMORY

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

/*** memory profile ***/
#if VIVANTE_PROFILER_SYSTEM_MEMORY

gceSTATUS
gcoOS_GetMemoryProfileInfo(
    size_t size,
    struct _memory_profile_info *info
    )
{
    gceSTATUS status = gcvSTATUS_OK;

    if (size != gcmSIZEOF(struct _memory_profile_info))
    {
        gcmONERROR(gcvSTATUS_INVALID_ARGUMENT);
    }

    if (gcPLS.bMemoryProfile && gcPLS.profileLock)
    {
        struct _memory_profile_info mem_info = { {0}, {0} };

        if (gcPLS.os) { /* video memory */
            gcsHAL_INTERFACE iface;

            gcoOS_ZeroMemory(&iface, gcmSIZEOF(iface));

            iface.command = gcvHAL_DATABASE;
            iface.u.Database.processID = gcmPTR2INT32(gcoOS_GetCurrentProcessID());
            iface.u.Database.validProcessID = gcvTRUE;

            /* Call kernel service. */
            gcmONERROR(gcoOS_DeviceControl(
                gcvNULL,
                IOCTL_GCHAL_INTERFACE,
                &iface, gcmSIZEOF(iface),
                &iface, gcmSIZEOF(iface)
                ));

            mem_info.gpu_memory.currentSize           = iface.u.Database.vidMem.counters.bytes      + iface.u.Database.nonPaged.counters.bytes;
            mem_info.gpu_memory.peakSize              = iface.u.Database.vidMem.counters.maxBytes   + iface.u.Database.nonPaged.counters.maxBytes;
            mem_info.gpu_memory.total_allocate        = iface.u.Database.vidMem.counters.totalBytes + iface.u.Database.nonPaged.counters.totalBytes;
            mem_info.gpu_memory.total_free            = 0;
            mem_info.gpu_memory.total_allocateCount   = iface.u.Database.vidMem.counters.allocCount + iface.u.Database.nonPaged.counters.allocCount;
            mem_info.gpu_memory.total_freeCount       = iface.u.Database.vidMem.counters.freeCount  + iface.u.Database.nonPaged.counters.freeCount;
        }
        else
        {
            mem_info.gpu_memory.currentSize           = gcPLS.video_currentSize;
            mem_info.gpu_memory.peakSize              = gcPLS.video_maxAllocSize;
            mem_info.gpu_memory.total_allocate        = gcPLS.video_allocSize;
            mem_info.gpu_memory.total_allocateCount   = gcPLS.video_allocCount;
            mem_info.gpu_memory.total_free            = gcPLS.video_freeSize;
            mem_info.gpu_memory.total_freeCount       = gcPLS.video_freeCount;
        }

        /* os memory */
        gcmONERROR(gcoOS_AcquireMutex(gcPLS.os, gcPLS.profileLock, gcvINFINITE));

        mem_info.system_memory.peakSize             = gcPLS.maxAllocSize;
        mem_info.system_memory.total_allocate       = gcPLS.allocSize;
        mem_info.system_memory.total_free           = gcPLS.freeSize;
        mem_info.system_memory.currentSize          = gcPLS.currentSize;
        mem_info.system_memory.total_allocateCount  = gcPLS.allocCount;
        mem_info.system_memory.total_freeCount      = gcPLS.freeCount;

        gcmONERROR(gcoOS_ReleaseMutex(gcPLS.os, gcPLS.profileLock));

        gcoOS_MemCopy(info, &mem_info, gcmSIZEOF(mem_info));
        return gcvSTATUS_OK;
    }
OnError:
    return status;
}

gceSTATUS gcoOS_DumpMemoryProfile(void)
{
    if (gcPLS.bMemoryProfile)
    {
        struct _memory_profile_info info;
        gcmVERIFY_OK(gcoOS_GetMemoryProfileInfo(gcmSIZEOF(info), &info));

        gcmPRINT("*************** Memory Profile Info Dump ****************\n");
        gcmPRINT("system memory:\n");
        gcmPRINT("  current allocation      : %lld\n", (long long)info.system_memory.currentSize);
        gcmPRINT("  maximum allocation      : %lld\n", (long long)info.system_memory.peakSize);
        gcmPRINT("  total allocation        : %lld\n", (long long)info.system_memory.total_allocate);
        gcmPRINT("  total free              : %lld\n", (long long)info.system_memory.total_free);
        gcmPRINT("  allocation count        : %u\n",              info.system_memory.total_allocateCount);
        gcmPRINT("  free count              : %u\n",              info.system_memory.total_freeCount);
        gcmPRINT("\n");
        gcmPRINT("gpu memory:\n");
        gcmPRINT("  current allocation      : %lld\n", (long long)info.gpu_memory.currentSize);
        gcmPRINT("  maximum allocation      : %lld\n", (long long)info.gpu_memory.peakSize);
        gcmPRINT("  total allocation        : %lld\n", (long long)info.gpu_memory.total_allocate);
        gcmPRINT("  total free              : %lld\n", (long long)info.gpu_memory.total_free);
        gcmPRINT("  allocation count        : %u\n",              info.gpu_memory.total_allocateCount);
        gcmPRINT("  free count              : %u\n",              info.gpu_memory.total_freeCount);
        gcmPRINT("************ end of Memory Profile Info Dump ************\n");
    }
    return gcvSTATUS_OK;
}

gceSTATUS gcoOS_InitMemoryProfile(void)
{
    gctSTRING str          = gcvNULL;
    gcoOS_GetEnv(gcPLS.os, "VIV_MEMORY_PROFILE", &str);

    if (str != gcvNULL && 0 != atoi(str))
    {
        gcmVERIFY_OK(gcoOS_CreateMutex(gcPLS.os, &gcPLS.profileLock));
        gcPLS.bMemoryProfile = gcvTRUE;
    }

    return gcvSTATUS_OK;
}

gceSTATUS gcoOS_DeInitMemoryProfile(void)
{
    if (gcPLS.bMemoryProfile)
    {
        gcmVERIFY_OK(gcoOS_DumpMemoryProfile());
        gcPLS.bMemoryProfile = gcvFALSE;

        gcmVERIFY_OK(gcoOS_DeleteMutex(gcPLS.os, gcPLS.profileLock));
    }

    return gcvSTATUS_OK;
}

#if gcdSKIP_ARM_DC_INSTRUCTION

#include "gc_hal_user_os_tweak.c"

#endif

#endif
