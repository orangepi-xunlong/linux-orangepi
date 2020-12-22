/*************************************************************************/ /*!
@Title          Debug Functionality
@Copyright      Copyright (c) Imagination Technologies Ltd. All Rights Reserved
@Description    Provides kernel side Debug Functionality
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

#include <linux/version.h>

#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,38))
#ifndef AUTOCONF_INCLUDED
#include <linux/config.h>
#endif
#endif

#include <asm/io.h>
#include <asm/uaccess.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/hardirq.h>
#include <linux/module.h>
#include <linux/spinlock.h>
#include <linux/string.h>			// strncpy, strlen
#include <stdarg.h>
#include <linux/seq_file.h>
#include "img_types.h"
#include "servicesext.h"
#include "pvr_debug.h"
#include "srvkm.h"
#include "mutex.h"
#include "linkage.h"
#include "pvr_uaccess.h"

#if !defined(CONFIG_PREEMPT)
#define	PVR_DEBUG_ALWAYS_USE_SPINLOCK
#endif

#if defined(PVRSRV_NEED_PVR_DPF)

/******** BUFFERED LOG MESSAGES ********/

/* Because we don't want to have to handle CCB wrapping, each buffered
 * message is rounded up to PVRSRV_DEBUG_CCB_MESG_MAX bytes. This means
 * there is the same fixed number of messages that can be stored,
 * regardless of message length.
 */

#if defined(PVRSRV_DEBUG_CCB_MAX)

#define PVRSRV_DEBUG_CCB_MESG_MAX	PVR_MAX_DEBUG_MESSAGE_LEN

#include <linux/syscalls.h>
#include <linux/time.h>

typedef struct
{
	const IMG_CHAR *pszFile;
	IMG_INT iLine;
	IMG_UINT32 ui32TID;
	IMG_CHAR pcMesg[PVRSRV_DEBUG_CCB_MESG_MAX];
	struct timeval sTimeVal;
}
PVRSRV_DEBUG_CCB;

static PVRSRV_DEBUG_CCB gsDebugCCB[PVRSRV_DEBUG_CCB_MAX] = { { 0 } };

static IMG_UINT giOffset = 0;

static PVRSRV_LINUX_MUTEX gsDebugCCBMutex;

static void
AddToBufferCCB(const IMG_CHAR *pszFileName, IMG_UINT32 ui32Line,
			   const IMG_CHAR *szBuffer)
{
	LinuxLockMutex(&gsDebugCCBMutex);

	gsDebugCCB[giOffset].pszFile = pszFileName;
	gsDebugCCB[giOffset].iLine   = ui32Line;
	gsDebugCCB[giOffset].ui32TID = current->tgid;

	do_gettimeofday(&gsDebugCCB[giOffset].sTimeVal);

	strncpy(gsDebugCCB[giOffset].pcMesg, szBuffer, PVRSRV_DEBUG_CCB_MESG_MAX - 1);
	gsDebugCCB[giOffset].pcMesg[PVRSRV_DEBUG_CCB_MESG_MAX - 1] = 0;

	giOffset = (giOffset + 1) % PVRSRV_DEBUG_CCB_MAX;

	LinuxUnLockMutex(&gsDebugCCBMutex);
}

IMG_EXPORT IMG_VOID PVRSRVDebugPrintfDumpCCB(void)
{
	int i;

	LinuxLockMutex(&gsDebugCCBMutex);
	
	for(i = 0; i < PVRSRV_DEBUG_CCB_MAX; i++)
	{
		PVRSRV_DEBUG_CCB *psDebugCCBEntry =
			&gsDebugCCB[(giOffset + i) % PVRSRV_DEBUG_CCB_MAX];

		/* Early on, we won't have PVRSRV_DEBUG_CCB_MAX messages */
		if(!psDebugCCBEntry->pszFile)
			continue;

		printk("%s:%d:\t[%5ld.%6ld] %s\n",
			   psDebugCCBEntry->pszFile,
			   psDebugCCBEntry->iLine,
			   (long)psDebugCCBEntry->sTimeVal.tv_sec,
			   (long)psDebugCCBEntry->sTimeVal.tv_usec,
			   psDebugCCBEntry->pcMesg);
	}

	LinuxUnLockMutex(&gsDebugCCBMutex);
}

#else /* defined(PVRSRV_DEBUG_CCB_MAX) */
static INLINE void
AddToBufferCCB(const IMG_CHAR *pszFileName, IMG_UINT32 ui32Line,
               const IMG_CHAR *szBuffer)
{
	(void)pszFileName;
	(void)szBuffer;
	(void)ui32Line;
}

IMG_EXPORT IMG_VOID PVRSRVDebugPrintfDumpCCB(void)
{
	/* Not available */
}

#endif /* defined(PVRSRV_DEBUG_CCB_MAX) */

#endif /* defined(PVRSRV_NEED_PVR_DPF) */

static IMG_BOOL VBAppend(IMG_CHAR *pszBuf, IMG_UINT32 ui32BufSiz,
						 const IMG_CHAR* pszFormat, va_list VArgs)
						 IMG_FORMAT_PRINTF(3, 0);


#if defined(PVRSRV_NEED_PVR_DPF)

/* NOTE: Must NOT be static! Used in module.c.. */
IMG_UINT32 gPVRDebugLevel =
	(DBGPRIV_FATAL | DBGPRIV_ERROR | DBGPRIV_WARNING | DBGPRIV_BUFFERED);

#endif /* defined(PVRSRV_NEED_PVR_DPF) || defined(PVRSRV_NEED_PVR_TRACE) */

#define	PVR_MAX_MSG_LEN PVR_MAX_DEBUG_MESSAGE_LEN

#if !defined(PVR_DEBUG_ALWAYS_USE_SPINLOCK)
/* Message buffer for non-IRQ messages */
static IMG_CHAR gszBufferNonIRQ[PVR_MAX_MSG_LEN + 1];
#endif

/* Message buffer for IRQ messages */
static IMG_CHAR gszBufferIRQ[PVR_MAX_MSG_LEN + 1];

#if !defined(PVR_DEBUG_ALWAYS_USE_SPINLOCK)
/* The lock is used to control access to gszBufferNonIRQ */
static PVRSRV_LINUX_MUTEX gsDebugMutexNonIRQ;
#endif

#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,39))
/* The lock is used to control access to gszBufferIRQ */
/* PRQA S 0671,0685 1 */ /* ignore warnings about C99 style initialisation */
static spinlock_t gsDebugLockIRQ = SPIN_LOCK_UNLOCKED;
#else
static DEFINE_SPINLOCK(gsDebugLockIRQ);
#endif

#if !defined(PVR_DEBUG_ALWAYS_USE_SPINLOCK)
#if !defined (USE_SPIN_LOCK) /* to keep QAC happy */ 
#define	USE_SPIN_LOCK (in_interrupt() || !preemptible())
#endif
#endif

static inline void GetBufferLock(unsigned long *pulLockFlags)
{
#if !defined(PVR_DEBUG_ALWAYS_USE_SPINLOCK)
	if (USE_SPIN_LOCK)
#endif
	{
		spin_lock_irqsave(&gsDebugLockIRQ, *pulLockFlags);
	}
#if !defined(PVR_DEBUG_ALWAYS_USE_SPINLOCK)
	else
	{
		LinuxLockMutexNested(&gsDebugMutexNonIRQ, PVRSRV_LOCK_CLASS_PVR_DEBUG);
	}
#endif
}

static inline void ReleaseBufferLock(unsigned long ulLockFlags)
{
#if !defined(PVR_DEBUG_ALWAYS_USE_SPINLOCK)
	if (USE_SPIN_LOCK)
#endif
	{
		spin_unlock_irqrestore(&gsDebugLockIRQ, ulLockFlags);
	}
#if !defined(PVR_DEBUG_ALWAYS_USE_SPINLOCK)
	else
	{
		LinuxUnLockMutex(&gsDebugMutexNonIRQ);
	}
#endif
}

static inline void SelectBuffer(IMG_CHAR **ppszBuf, IMG_UINT32 *pui32BufSiz)
{
#if !defined(PVR_DEBUG_ALWAYS_USE_SPINLOCK)
	if (USE_SPIN_LOCK)
#endif
	{
		*ppszBuf = gszBufferIRQ;
		*pui32BufSiz = sizeof(gszBufferIRQ);
	}
#if !defined(PVR_DEBUG_ALWAYS_USE_SPINLOCK)
	else
	{
		*ppszBuf = gszBufferNonIRQ;
		*pui32BufSiz = sizeof(gszBufferNonIRQ);
	}
#endif
}

/*
 * Append a string to a buffer using formatted conversion.
 * The function takes a variable number of arguments, pointed
 * to by the var args list.
 */
static IMG_BOOL VBAppend(IMG_CHAR *pszBuf, IMG_UINT32 ui32BufSiz, const IMG_CHAR* pszFormat, va_list VArgs)
{
	IMG_UINT32 ui32Used;
	IMG_UINT32 ui32Space;
	IMG_INT32 i32Len;

	ui32Used = strlen(pszBuf);
	BUG_ON(ui32Used >= ui32BufSiz);
	ui32Space = ui32BufSiz - ui32Used;

	i32Len = vsnprintf(&pszBuf[ui32Used], ui32Space, pszFormat, VArgs);
	pszBuf[ui32BufSiz - 1] = 0;

	/* Return true if string was truncated */
	return (i32Len < 0 || i32Len >= (IMG_INT32)ui32Space) ? IMG_TRUE : IMG_FALSE;
}

/* Actually required for ReleasePrintf too */

IMG_VOID PVRDPFInit(IMG_VOID)
{
#if !defined(PVR_DEBUG_ALWAYS_USE_SPINLOCK)
	LinuxInitMutex(&gsDebugMutexNonIRQ);
#endif
#if defined(PVRSRV_DEBUG_CCB_MAX)
	LinuxInitMutex(&gsDebugCCBMutex);
#endif
}

/*!
******************************************************************************
	@Function    PVRSRVReleasePrintf
	@Description To output an important message to the user in release builds
	@Input       pszFormat - The message format string
	@Input       ... - Zero or more arguments for use by the format string
	@Return      None
 ******************************************************************************/
IMG_VOID PVRSRVReleasePrintf(const IMG_CHAR *pszFormat, ...)
{
	va_list vaArgs;
	unsigned long ulLockFlags = 0;
	IMG_CHAR *pszBuf;
	IMG_UINT32 ui32BufSiz;

	SelectBuffer(&pszBuf, &ui32BufSiz);

	va_start(vaArgs, pszFormat);

	GetBufferLock(&ulLockFlags);
	strncpy (pszBuf, "PVR_K: ", (ui32BufSiz -1));

	if (VBAppend(pszBuf, ui32BufSiz, pszFormat, vaArgs))
	{
		printk(KERN_INFO "PVR_K:(Message Truncated): %s\n", pszBuf);
	}
	else
	{
		printk(KERN_INFO "%s\n", pszBuf);
	}

	ReleaseBufferLock(ulLockFlags);
	va_end(vaArgs);
}

#if defined(PVRSRV_NEED_PVR_TRACE)

/*!
******************************************************************************
	@Function    PVRTrace
	@Description To output a debug message to the user
	@Input       pszFormat - The message format string
	@Input       ... - Zero or more arguments for use by the format string
	@Return      None
 ******************************************************************************/
IMG_VOID PVRSRVTrace(const IMG_CHAR* pszFormat, ...)
{
	va_list VArgs;
	unsigned long ulLockFlags = 0;
	IMG_CHAR *pszBuf;
	IMG_UINT32 ui32BufSiz;

	SelectBuffer(&pszBuf, &ui32BufSiz);

	va_start(VArgs, pszFormat);

	GetBufferLock(&ulLockFlags);

	strncpy(pszBuf, "PVR: ", (ui32BufSiz -1));

	if (VBAppend(pszBuf, ui32BufSiz, pszFormat, VArgs))
	{
		printk(KERN_INFO "PVR_K:(Message Truncated): %s\n", pszBuf);
	}
	else
	{
		printk(KERN_INFO "%s\n", pszBuf);
	}

	ReleaseBufferLock(ulLockFlags);

	va_end(VArgs);
}

#endif /* defined(PVRSRV_NEED_PVR_TRACE) */

#if defined(PVRSRV_NEED_PVR_DPF)

/*!
******************************************************************************
	@Function    PVRSRVDebugPrintf
	@Description To output a debug message to the user
	@Input       uDebugLevel - The current debug level
	@Input       pszFile - The source file generating the message
	@Input       uLine - The line of the source file
	@Input       pszFormat - The message format string
	@Input       ... - Zero or more arguments for use by the format string
	@Return      None
 ******************************************************************************/
IMG_VOID PVRSRVDebugPrintf	(
						IMG_UINT32	ui32DebugLevel,
						const IMG_CHAR*	pszFullFileName,
						IMG_UINT32	ui32Line,
						const IMG_CHAR*	pszFormat,
						...
					)
{
	IMG_BOOL bTrace;
	const IMG_CHAR *pszFileName = pszFullFileName;

	bTrace = (IMG_BOOL)(ui32DebugLevel & DBGPRIV_CALLTRACE) ? IMG_TRUE : IMG_FALSE;

	if (gPVRDebugLevel & ui32DebugLevel)
	{
		va_list vaArgs;
		unsigned long ulLockFlags = 0;
		IMG_CHAR *pszBuf;
		IMG_UINT32 ui32BufSiz;

		SelectBuffer(&pszBuf, &ui32BufSiz);

		va_start(vaArgs, pszFormat);

		GetBufferLock(&ulLockFlags);

		/* Add in the level of warning */
		if (bTrace == IMG_FALSE)
		{
			switch(ui32DebugLevel)
			{
				case DBGPRIV_FATAL:
				{
					strncpy (pszBuf, "PVR_K:(Fatal): ", (ui32BufSiz -1));
					break;
				}
				case DBGPRIV_ERROR:
				{
					strncpy (pszBuf, "PVR_K:(Error): ", (ui32BufSiz -1));
					break;
				}
				case DBGPRIV_WARNING:
				{
					strncpy (pszBuf, "PVR_K:(Warning): ", (ui32BufSiz -1));
					break;
				}
				case DBGPRIV_MESSAGE:
				{
					strncpy (pszBuf, "PVR_K:(Message): ", (ui32BufSiz -1));
					break;
				}
				case DBGPRIV_VERBOSE:
				{
					strncpy (pszBuf, "PVR_K:(Verbose): ", (ui32BufSiz -1));
					break;
				}
				case DBGPRIV_BUFFERED:
				{
					strncpy (pszBuf, "PVR_K: ", (ui32BufSiz -1));
					break;
				}
				default:
				{
					strncpy (pszBuf, "PVR_K:(Unknown message level): ", (ui32BufSiz -1));
					break;
				}
			}
		}
		else
		{
			strncpy (pszBuf, "PVR_K: ", (ui32BufSiz -1));
		}

		if (VBAppend(pszBuf, ui32BufSiz, pszFormat, vaArgs))
		{
			printk(KERN_INFO "PVR_K:(Message Truncated): %s\n", pszBuf);
		}
		else
		{
			if (ui32DebugLevel & DBGPRIV_BUFFERED)
			{
				/* We don't need the full path here */
				const IMG_CHAR *pszShortName = strrchr(pszFileName, '/') + 1;
				if(pszShortName)
					pszFileName = pszShortName;

				AddToBufferCCB(pszFileName, ui32Line, pszBuf);
			}
			else
			{
				printk(KERN_INFO "%s\n", pszBuf);
			}
		}

		ReleaseBufferLock(ulLockFlags);

		va_end (vaArgs);
	}
}

#endif /* PVRSRV_NEED_PVR_DPF */

#if defined(DEBUG)

IMG_INT PVRDebugProcSetLevel(struct file *file, const IMG_CHAR *buffer, IMG_UINT32 count, IMG_VOID *data)
{
#define	_PROC_SET_BUFFER_SZ		6
	IMG_CHAR data_buffer[_PROC_SET_BUFFER_SZ];

	PVR_UNREFERENCED_PARAMETER(file);
	PVR_UNREFERENCED_PARAMETER(data);

	if (count > _PROC_SET_BUFFER_SZ)
	{
		return -EINVAL;
	}
	else
	{
		if (pvr_copy_from_user(data_buffer, buffer, count))
			return -EINVAL;
		if (data_buffer[count - 1] != '\n')
			return -EINVAL;
		if (sscanf(data_buffer, "%i", &gPVRDebugLevel) == 0)
			return -EINVAL;
		gPVRDebugLevel &= (1 << DBGPRIV_DBGLEVEL_COUNT) - 1;
	}
	return (count);
}

void ProcSeqShowDebugLevel(struct seq_file *sfile, void* el)
{
	PVR_UNREFERENCED_PARAMETER(el);

	seq_printf(sfile, "%u\n", gPVRDebugLevel);
}

#endif /* defined(DEBUG) */
