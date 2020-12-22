/*************************************************************************/ /*!
@Title          Proc files implementation.
@Copyright      Copyright (c) Imagination Technologies Ltd. All Rights Reserved
@Description    Functions for creating and reading proc filesystem entries.
                Proc filesystem support must be built into the kernel for
                these functions to be any use.
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

#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>

#include "services_headers.h"

#include "queue.h"
#include "resman.h"
#include "pvrmmap.h"
#include "pvr_debug.h"
#include "pvrversion.h"
#include "proc.h"
#include "perproc.h"
#include "env_perproc.h"
#include "linkage.h"

#include "lists.h"

struct pvr_proc_dir_entry {
	struct proc_dir_entry *pde;
	
	pvr_next_proc_seq_t *next;
	pvr_show_proc_seq_t *show;	
	pvr_off2element_proc_seq_t *off2element;
	pvr_startstop_proc_seq_t *startstop;
	
	pvr_proc_write_t *write;

	IMG_VOID *data;
};

// The proc entry for our /proc/pvr directory
static struct proc_dir_entry * dir;

static const IMG_CHAR PVRProcDirRoot[] = "pvr";

static IMG_INT pvr_proc_open(struct inode *inode,struct file *file);
static ssize_t pvr_proc_write(struct file *file, const char __user *buffer, size_t count, loff_t *ppos);

static struct file_operations pvr_proc_operations =
{
	.open		= pvr_proc_open,
	.read		= seq_read,
	.write		= pvr_proc_write,
	.llseek		= seq_lseek,
	.release	= seq_release,
};

static void *pvr_proc_seq_start (struct seq_file *m, loff_t *pos);
static void *pvr_proc_seq_next (struct seq_file *m, void *v, loff_t *pos);
static void pvr_proc_seq_stop (struct seq_file *m, void *v);
static int pvr_proc_seq_show (struct seq_file *m, void *v);

static struct seq_operations pvr_proc_seq_operations =
{
	.start =	pvr_proc_seq_start,
	.next =		pvr_proc_seq_next,
	.stop =		pvr_proc_seq_stop,
	.show =		pvr_proc_seq_show,
};

#if defined(SUPPORT_PVRSRV_DEVICE_CLASS)
static struct pvr_proc_dir_entry* g_pProcQueue;
#endif
static struct pvr_proc_dir_entry* g_pProcVersion;
static struct pvr_proc_dir_entry* g_pProcSysNodes;

#ifdef DEBUG
static struct pvr_proc_dir_entry* g_pProcDebugLevel;
#endif

#ifdef PVR_MANUAL_POWER_CONTROL
static struct pvr_proc_dir_entry* g_pProcPowerLevel;
#endif


static void ProcSeqShowVersion(struct seq_file *sfile,void* el);

static void ProcSeqShowSysNodes(struct seq_file *sfile,void* el);
static void* ProcSeqOff2ElementSysNodes(struct seq_file * sfile, loff_t off);


#if LINUX_VERSION_CODE < KERNEL_VERSION(3,10,0)
#define PDE_DATA(x)    PDE(x)->data;
#endif

#ifdef DEBUG

/*!
******************************************************************************

 @Function : ProcSeq1ElementOff2Element

 @Description

 Heleper Offset -> Element function for /proc files with only one entry
 without header.

 @Input  sfile : seq_file object related to /proc/ file

 @Input  off : the offset into the buffer (id of object)

 @Return : Pointer to element to be shown.

*****************************************************************************/
static void* ProcSeq1ElementOff2Element(struct seq_file *sfile, loff_t off)
{
	PVR_UNREFERENCED_PARAMETER(sfile);
	// Return anything that is not PVR_RPOC_SEQ_START_TOKEN and NULL
	if(!off)
		return (void*)2;
	return NULL;
}

#endif

/*!
******************************************************************************

 @Function : ProcSeq1ElementHeaderOff2Element

 @Description

 Heleper Offset -> Element function for /proc files with only one entry
 with header.

 @Input  sfile : seq_file object related to /proc/ file

 @Input  off : the offset into the buffer (id of object)

 @Return : Pointer to element to be shown.

*****************************************************************************/
static void* ProcSeq1ElementHeaderOff2Element(struct seq_file *sfile, loff_t off)
{
	PVR_UNREFERENCED_PARAMETER(sfile);

	if(!off)
	{
		return PVR_PROC_SEQ_START_TOKEN;
	}

	// Return anything that is not PVR_RPOC_SEQ_START_TOKEN and NULL
	if(off == 1)
		return (void*)2;

	return NULL;
}


/*!
******************************************************************************

 @Function : pvr_proc_open

 @Description
 File opening function passed to proc_dir_entry->proc_fops for /proc entries
 created by CreateProcReadEntrySeq.

 @Input  inode : inode entry of opened /proc file

 @Input  file : file entry of opened /proc file

 @Return      : 0 if no errors

*****************************************************************************/
static IMG_INT pvr_proc_open(struct inode *inode,struct file *file)
{
	IMG_INT ret = seq_open(file, &pvr_proc_seq_operations);

	struct seq_file *seq = (struct seq_file*)file->private_data;
	struct pvr_proc_dir_entry* ppde = PDE_DATA(inode);

	/* Add pointer to handlers to seq_file structure */
	seq->private = ppde;
	return ret;
}

/*!
******************************************************************************

 @Function : pvr_proc_write

 @Description
 File writing function passed to proc_dir_entry->proc_fops for /proc files.
 It's exacly the same function that is used as default one (->fs/proc/generic.c),
 it calls proc_dir_entry->write_proc for writing procedure.

*****************************************************************************/
static ssize_t pvr_proc_write(struct file *file, const char __user *buffer,
		size_t count, loff_t *ppos)
{
	struct inode *inode = file->f_path.dentry->d_inode;
	struct pvr_proc_dir_entry * ppde;

	PVR_UNREFERENCED_PARAMETER(ppos);
	ppde = PDE_DATA(inode);

	if (!ppde->write)
		return -EIO;

	return ppde->write(file, buffer, count, ppde->data);
}


/*!
******************************************************************************

 @Function : pvr_proc_seq_start

 @Description
 Seq_file start function. Detailed description of seq_file workflow can
 be found here: http://tldp.org/LDP/lkmpg/2.6/html/x861.html.
 This function ises off2element handler.

 @Input  proc_seq_file : sequence file entry

 @Input  pos : offset within file (id of entry)

 @Return      : Pointer to element from we start enumeration (0 ends it)

*****************************************************************************/
static void *pvr_proc_seq_start (struct seq_file *proc_seq_file, loff_t *pos)
{
	struct pvr_proc_dir_entry *ppde = (struct pvr_proc_dir_entry*)proc_seq_file->private;
	if(ppde->startstop != NULL)
		ppde->startstop(proc_seq_file, IMG_TRUE);
	return ppde->off2element(proc_seq_file, *pos);
}

/*!
******************************************************************************

 @Function : pvr_proc_seq_stop

 @Description
 Seq_file stop function. Detailed description of seq_file workflow can
 be found here: http://tldp.org/LDP/lkmpg/2.6/html/x861.html.

 @Input  proc_seq_file : sequence file entry

 @Input  v : current element pointer

*****************************************************************************/
static void pvr_proc_seq_stop (struct seq_file *proc_seq_file, void *v)
{
	struct pvr_proc_dir_entry *ppde = (struct pvr_proc_dir_entry*)proc_seq_file->private;
	PVR_UNREFERENCED_PARAMETER(v);

	if(ppde->startstop != NULL)
		ppde->startstop(proc_seq_file, IMG_FALSE);
}

/*!
******************************************************************************

 @Function : pvr_proc_seq_next

 @Description
 Seq_file next element function. Detailed description of seq_file workflow can
 be found here: http://tldp.org/LDP/lkmpg/2.6/html/x861.html.
 It uses supplied 'next' handler for fetching next element (or 0 if there is no one)

 @Input  proc_seq_file : sequence file entry

 @Input  pos : offset within file (id of entry)

 @Input  v : current element pointer

 @Return   : next element pointer (or 0 if end)

*****************************************************************************/
static void *pvr_proc_seq_next (struct seq_file *proc_seq_file, void *v, loff_t *pos)
{
	struct pvr_proc_dir_entry *ppde = (struct pvr_proc_dir_entry*)proc_seq_file->private;
	(*pos)++;
	if(ppde->next != NULL)
		return ppde->next( proc_seq_file, v, *pos );
	return ppde->off2element(proc_seq_file, *pos);
}

/*!
******************************************************************************

 @Function : pvr_proc_seq_show

 @Description
 Seq_file show element function. Detailed description of seq_file workflow can
 be found here: http://tldp.org/LDP/lkmpg/2.6/html/x861.html.
 It call proper 'show' handler to show (dump) current element using seq_* functions

 @Input  proc_seq_file : sequence file entry

 @Input  v : current element pointer

 @Return   : 0 if everything is OK

*****************************************************************************/
static int pvr_proc_seq_show (struct seq_file *proc_seq_file, void *v)
{
	struct pvr_proc_dir_entry *ppde = (struct pvr_proc_dir_entry*)proc_seq_file->private;
	ppde->show( proc_seq_file,v );
	return 0;
}



/*!
******************************************************************************

 @Function : CreateProcEntryInDirSeq

 @Description

 Create a file under the given directory.  These dynamic files can be used at
 runtime to get or set information about the device. Whis version uses seq_file
 interface

 @Input  pdir : parent directory

 @Input name : the name of the file to create

 @Input data : aditional data that will be passed to handlers

 @Input next_handler : the function to call to provide the next element. OPTIONAL, if not
						supplied, then off2element function is used instead

 @Input show_handler : the function to call to show element

 @Input off2element_handler : the function to call when it is needed to translate offest to element

 @Input startstop_handler : the function to call when output memory page starts or stops. OPTIONAL.

 @Input  whandler : the function to interpret writes from the user

 @Return Ptr to proc entry , 0 for failure


*****************************************************************************/
static struct pvr_proc_dir_entry* CreateProcEntryInDirSeq(struct proc_dir_entry *pdir,
                                                          const IMG_CHAR * name,
                                                          IMG_VOID* data,
                                                          pvr_next_proc_seq_t next_handler,
                                                          pvr_show_proc_seq_t show_handler,
                                                          pvr_off2element_proc_seq_t off2element_handler,
                                                          pvr_startstop_proc_seq_t startstop_handler,
                                                          pvr_proc_write_t whandler)
{

	struct pvr_proc_dir_entry * ppde;
	mode_t mode;

	if (!dir)
	{
		PVR_DPF((PVR_DBG_ERROR, "CreateProcEntryInDirSeq: cannot make proc entry /proc/%s/%s: no parent", PVRProcDirRoot, name));
		return NULL;
	}

	mode = S_IFREG;

	if (show_handler)
	{
		mode |= S_IRUGO;
	}

	if (whandler)
	{
		mode |= S_IWUSR;
	}

	ppde = kmalloc(sizeof(struct pvr_proc_dir_entry), GFP_KERNEL);
	if (!ppde)
	{
		PVR_DPF((PVR_DBG_ERROR, "CreateProcEntryInDirSeq: cannot make proc entry /proc/%s/%s: no memory", PVRProcDirRoot, name));
		return NULL;
	}
	
	ppde->next        = next_handler;
	ppde->show        = show_handler;
	ppde->off2element = off2element_handler;
	ppde->startstop   = startstop_handler;
	ppde->write       = whandler;
	ppde->data        = data;

	ppde->pde=proc_create_data(name, mode, pdir, &pvr_proc_operations, ppde);

	if (!ppde->pde)
	{
		PVR_DPF((PVR_DBG_ERROR, "CreateProcEntryInDirSeq: cannot make proc entry /proc/%s/%s: proc_create_data failed", PVRProcDirRoot, name));
		kfree(ppde);
		return NULL;
	}
	return ppde;
}


/*!
******************************************************************************

 @Function :  CreateProcReadEntrySeq

 @Description

 Create a file under /proc/pvr.  These dynamic files can be used at runtime
 to get information about the device.  Creation WILL fail if proc support is
 not compiled into the kernel.  That said, the Linux kernel is not even happy
 to build without /proc support these days. This version uses seq_file structure
 for handling content generation.

 @Input name : the name of the file to create

 @Input data : aditional data that will be passed to handlers

 @Input next_handler : the function to call to provide the next element. OPTIONAL, if not
						supplied, then off2element function is used instead

 @Input show_handler : the function to call to show element

 @Input off2element_handler : the function to call when it is needed to translate offest to element

 @Input startstop_handler : the function to call when output memory page starts or stops. OPTIONAL.

 @Return Ptr to proc entry , 0 for failure

*****************************************************************************/
struct pvr_proc_dir_entry* CreateProcReadEntrySeq (const IMG_CHAR * name,
                                                   IMG_VOID* data,
                                                   pvr_next_proc_seq_t next_handler,
                                                   pvr_show_proc_seq_t show_handler,
                                                   pvr_off2element_proc_seq_t off2element_handler,
                                                   pvr_startstop_proc_seq_t startstop_handler)
{
	return CreateProcEntrySeq(name,
	                          data,
	                          next_handler,
	                          show_handler,
	                          off2element_handler,
	                          startstop_handler,
	                          NULL);
}

/*!
******************************************************************************

 @Function : CreateProcEntrySeq

 @Description

 @Description

 Create a file under /proc/pvr.  These dynamic files can be used at runtime
 to get information about the device.  Creation WILL fail if proc support is
 not compiled into the kernel.  That said, the Linux kernel is not even happy
 to build without /proc support these days. This version uses seq_file structure
 for handling content generation and is fuller than CreateProcReadEntrySeq (it
 supports write access);

 @Input name : the name of the file to create

 @Input data : aditional data that will be passed to handlers

 @Input next_handler : the function to call to provide the next element. OPTIONAL, if not
						supplied, then off2element function is used instead

 @Input show_handler : the function to call to show element

 @Input off2element_handler : the function to call when it is needed to translate offest to element

 @Input startstop_handler : the function to call when output memory page starts or stops. OPTIONAL.

 @Input  whandler : the function to interpret writes from the user

 @Return Ptr to proc entry , 0 for failure

*****************************************************************************/
struct pvr_proc_dir_entry* CreateProcEntrySeq (const IMG_CHAR * name,
                                               IMG_VOID* data,
                                               pvr_next_proc_seq_t next_handler,
                                               pvr_show_proc_seq_t show_handler,
                                               pvr_off2element_proc_seq_t off2element_handler,
                                               pvr_startstop_proc_seq_t startstop_handler,
                                               pvr_proc_write_t whandler)
{
	return CreateProcEntryInDirSeq(dir,
	                               name,
	                               data,
	                               next_handler,
	                               show_handler,
	                               off2element_handler,
	                               startstop_handler,
	                               whandler);
}



/*!
******************************************************************************

 @Function : CreatePerProcessProcEntrySeq

 @Description

 Create a file under /proc/pvr/<current process ID>.  Apart from the
 directory where the file is created, this works the same way as
 CreateProcEntry. It's seq_file version.



 @Input name : the name of the file to create

 @Input data : aditional data that will be passed to handlers

 @Input next_handler : the function to call to provide the next element. OPTIONAL, if not
						supplied, then off2element function is used instead

 @Input show_handler : the function to call to show element

 @Input off2element_handler : the function to call when it is needed to translate offest to element

 @Input startstop_handler : the function to call when output memory page starts or stops. OPTIONAL.

 @Input  whandler : the function to interpret writes from the user

 @Return Ptr to proc entry , 0 for failure

*****************************************************************************/
struct pvr_proc_dir_entry* CreatePerProcessProcEntrySeq (const IMG_CHAR * name,
                                                         IMG_VOID* data,
                                                         pvr_next_proc_seq_t next_handler,
                                                         pvr_show_proc_seq_t show_handler,
                                                         pvr_off2element_proc_seq_t off2element_handler,
                                                         pvr_startstop_proc_seq_t startstop_handler,
                                                         pvr_proc_write_t whandler)
{
    PVRSRV_ENV_PER_PROCESS_DATA *psPerProc;
    IMG_UINT32 ui32PID;

    if (!dir)
    {
        PVR_DPF((PVR_DBG_ERROR, "CreatePerProcessProcEntrySeq: /proc/%s doesn't exist", PVRProcDirRoot));
        return NULL;
    }

    ui32PID = OSGetCurrentProcessIDKM();

    psPerProc = PVRSRVPerProcessPrivateData(ui32PID);
    if (!psPerProc)
    {
        PVR_DPF((PVR_DBG_ERROR, "CreatePerProcessProcEntrySeq: no per process data"));
        return NULL;
    }

    if (!psPerProc->psProcDir)
    {
        IMG_CHAR dirname[16];
        IMG_INT ret;

        ret = snprintf(dirname, sizeof(dirname), "%u", ui32PID);

        if (ret <=0 || ret >= (IMG_INT)sizeof(dirname))
        {
            PVR_DPF((PVR_DBG_ERROR, "CreatePerProcessProcEntries: couldn't generate per process proc directory name \"%u\"", ui32PID));
            return NULL;
        }
        else
        {
            psPerProc->psProcDir = proc_mkdir(dirname, dir);
            if (!psPerProc->psProcDir)
            {
                PVR_DPF((PVR_DBG_ERROR, "CreatePerProcessProcEntries: couldn't create per process proc directory /proc/%s/%u",
                         PVRProcDirRoot, ui32PID));
                return NULL;
            }
	}
    }

    return CreateProcEntryInDirSeq(psPerProc->psProcDir, name, data, next_handler,
                                   show_handler,off2element_handler,startstop_handler,whandler);
}


/*!
******************************************************************************

 @Function : CreateProcEntries

 @Description

 Create a directory /proc/pvr and the necessary entries within it.  These
 dynamic files can be used at runtime to get information about the device.
 Creation might fail if proc support is not compiled into the kernel or if
 there is no memory

 @Input none

 @Return nothing

*****************************************************************************/
IMG_INT CreateProcEntries(IMG_VOID)
{
    dir = proc_mkdir (PVRProcDirRoot, NULL);

    if (!dir)
    {
        PVR_DPF((PVR_DBG_ERROR, "CreateProcEntries: cannot make /proc/%s directory", PVRProcDirRoot));

        return -ENOMEM;
    }

#if defined(SUPPORT_PVRSRV_DEVICE_CLASS)
	g_pProcQueue = CreateProcReadEntrySeq("queue", NULL, NULL, ProcSeqShowQueue, ProcSeqOff2ElementQueue, NULL);
#endif
	g_pProcVersion = CreateProcReadEntrySeq("version", NULL, NULL, ProcSeqShowVersion, ProcSeq1ElementHeaderOff2Element, NULL);
	g_pProcSysNodes = CreateProcReadEntrySeq("nodes", NULL, NULL, ProcSeqShowSysNodes, ProcSeqOff2ElementSysNodes, NULL);

	if(!g_pProcVersion || !g_pProcSysNodes
#if defined(SUPPORT_PVRSRV_DEVICE_CLASS)
		|| !g_pProcQueue
#endif
		)
    {
        PVR_DPF((PVR_DBG_ERROR, "CreateProcEntries: couldn't make /proc/%s files", PVRProcDirRoot));

        return -ENOMEM;
    }


#ifdef DEBUG

	g_pProcDebugLevel = CreateProcEntrySeq("debug_level", NULL, NULL,
											ProcSeqShowDebugLevel,
											ProcSeq1ElementOff2Element, NULL,
										    (IMG_VOID*)PVRDebugProcSetLevel);
	if(!g_pProcDebugLevel)
    {
        PVR_DPF((PVR_DBG_ERROR, "CreateProcEntries: couldn't make /proc/%s/debug_level", PVRProcDirRoot));

        return -ENOMEM;
    }

#ifdef PVR_MANUAL_POWER_CONTROL
	g_pProcPowerLevel = CreateProcEntrySeq("power_control", NULL, NULL,
											ProcSeqShowPowerLevel,
											ProcSeq1ElementOff2Element, NULL,
										    PVRProcSetPowerLevel);
	if(!g_pProcPowerLevel)
    {
        PVR_DPF((PVR_DBG_ERROR, "CreateProcEntries: couldn't make /proc/%s/power_control", PVRProcDirRoot));

        return -ENOMEM;
    }
#endif
#endif

    return 0;
}


/*!
******************************************************************************

 @Function : RemoveProcEntrySeq

 @Description

 Remove a single node (created using *Seq function) under /proc/pvr.

 @Input proc_entry : structure returned by Create function.

 @Return nothing

*****************************************************************************/
IMG_VOID RemoveProcEntrySeq(struct pvr_proc_dir_entry* ppde)
{
#if LINUX_VERSION_CODE < KERNEL_VERSION(3,10,0)
	remove_proc_entry(ppde->pde->name, dir);
#else
	proc_remove(ppde->pde);
#endif
	kfree(ppde);
}


/*!
******************************************************************************

 @Function : RemovePerProcessProcEntrySeq

 @Description

 Remove a single node under the per process proc directory (created by *Seq function).

 Remove a single node (created using *Seq function) under /proc/pvr.

 @Input proc_entry : structure returned by Create function.

 @Return nothing

*****************************************************************************/
IMG_VOID RemovePerProcessProcEntrySeq(struct pvr_proc_dir_entry* ppde)
{
    PVRSRV_ENV_PER_PROCESS_DATA *psPerProc;

    psPerProc = LinuxTerminatingProcessPrivateData();
    if (!psPerProc)
    {
        psPerProc = PVRSRVFindPerProcessPrivateData();
        if (!psPerProc)
        {
            PVR_DPF((PVR_DBG_ERROR, "CreatePerProcessProcEntries: can't remove proc entry, no per process data"));
            return;
        }
    }

    if (psPerProc->psProcDir)
    {
        PVR_DPF((PVR_DBG_MESSAGE, "Removing per-process proc entry"));
#if LINUX_VERSION_CODE < KERNEL_VERSION(3,10,0)
        remove_proc_entry(ppde->pde->name, psPerProc->psProcDir);
#else
	proc_remove(ppde->pde);
#endif
	kfree(ppde);
    }
}

#if LINUX_VERSION_CODE < KERNEL_VERSION(3,10,0)
/*!
******************************************************************************

 @Function : RemoveProcEntry

 @Description

 Remove a single node under /proc/pvr.

 @Input name : the name of the node to remove

 @Return nothing

*****************************************************************************/
static IMG_VOID RemoveProcEntry(const IMG_CHAR * name)
{
    if (dir)
    {
        remove_proc_entry(name, dir);
        PVR_DPF((PVR_DBG_MESSAGE, "Removing /proc/%s/%s", PVRProcDirRoot, name));
    }
}


/*!
******************************************************************************

 @Function : RemovePerProcessProcDir

 @Description

 Remove the per process directorty under /proc/pvr.

 @Input psPerProc : environment specific per process data

 @Return nothing

*****************************************************************************/
IMG_VOID RemovePerProcessProcDir(PVRSRV_ENV_PER_PROCESS_DATA *psPerProc)
{
    if (psPerProc->psProcDir)
    {
        while (psPerProc->psProcDir->subdir)
        {
            PVR_DPF((PVR_DBG_WARNING, "Belatedly removing /proc/%s/%s/%s", PVRProcDirRoot, psPerProc->psProcDir->name, psPerProc->psProcDir->subdir->name));

            RemoveProcEntry(psPerProc->psProcDir->subdir->name);
        }
        RemoveProcEntry(psPerProc->psProcDir->name);
    }
}
#else
IMG_VOID RemovePerProcessProcDir(PVRSRV_ENV_PER_PROCESS_DATA *psPerProc)
{
	proc_remove(psPerProc->psProcDir);
}
#endif
/*!
******************************************************************************

 @Function    : RemoveProcEntries

 Description

 Proc filesystem entry deletion - Remove all proc filesystem entries for
 the driver.

 @Input none

 @Return nothing

*****************************************************************************/
IMG_VOID RemoveProcEntries(IMG_VOID)
{
#ifdef DEBUG
	RemoveProcEntrySeq( g_pProcDebugLevel );
#ifdef PVR_MANUAL_POWER_CONTROL
	RemoveProcEntrySeq( g_pProcPowerLevel );
#endif /* PVR_MANUAL_POWER_CONTROL */
#endif

#if defined(SUPPORT_PVRSRV_DEVICE_CLASS)
	RemoveProcEntrySeq(g_pProcQueue);
#endif
	RemoveProcEntrySeq(g_pProcVersion);
	RemoveProcEntrySeq(g_pProcSysNodes);

#if LINUX_VERSION_CODE < KERNEL_VERSION(3,10,0)
	while (dir->subdir)
	{
		PVR_DPF((PVR_DBG_WARNING, "Belatedly removing /proc/%s/%s", PVRProcDirRoot, dir->subdir->name));

		RemoveProcEntry(dir->subdir->name);
	}
	remove_proc_entry(PVRProcDirRoot, NULL);
#else
	proc_remove(dir);
#endif

}

/*************************************************************************/ /*!
@Function PVRProcGetData
@Description Extract data from PVR proc object.
@Input pointer to pvr_proc_dir_entr object
@Return pointer to data object passed in to Proc create function.
*/ /**************************************************************************/
void *PVRProcGetData(struct pvr_proc_dir_entry *ppde)
{
    return ppde->data;
}

/*****************************************************************************
 FUNCTION	:	ProcSeqShowVersion

 PURPOSE	:	Print the content of version to /proc file

 PARAMETERS	:	sfile - /proc seq_file
				el - Element to print
*****************************************************************************/
static void ProcSeqShowVersion(struct seq_file *sfile, void* el)
{
	SYS_DATA *psSysData;
	IMG_CHAR *pszSystemVersionString = "None";

	if(el == PVR_PROC_SEQ_START_TOKEN)
	{
		seq_printf(sfile,
				"Version %s (%s) %s\n",
				PVRVERSION_STRING,
				PVR_BUILD_TYPE, PVR_BUILD_DIR);
		return;
	}

	psSysData = SysAcquireDataNoCheck();
	if(psSysData != IMG_NULL && psSysData->pszVersionString != IMG_NULL)
	{
		pszSystemVersionString = psSysData->pszVersionString;
	}

	seq_printf( sfile, "System Version String: %s\n", pszSystemVersionString);
}

static const IMG_CHAR *deviceTypeToString(PVRSRV_DEVICE_TYPE deviceType)
{
    switch (deviceType)
    {
        default:
        {
            static IMG_CHAR text[10];

            sprintf(text, "?%x", (IMG_UINT)deviceType);

            return text;
        }
    }
}


static const IMG_CHAR *deviceClassToString(PVRSRV_DEVICE_CLASS deviceClass)
{
    switch (deviceClass)
    {
	case PVRSRV_DEVICE_CLASS_3D:
	{
	    return "3D";
	}
	case PVRSRV_DEVICE_CLASS_DISPLAY:
	{
	    return "display";
	}
	case PVRSRV_DEVICE_CLASS_BUFFER:
	{
	    return "buffer";
	}
	default:
	{
	    static IMG_CHAR text[10];

	    sprintf(text, "?%x", (IMG_UINT)deviceClass);
	    return text;
	}
    }
}

static IMG_VOID* DecOffPsDev_AnyVaCb(PVRSRV_DEVICE_NODE *psNode, va_list va)
{
	off_t *pOff = va_arg(va, off_t*);
	if (--(*pOff))
	{
		return IMG_NULL;
	}
	else
	{
		return psNode;
	}
}

/*****************************************************************************
 FUNCTION	:	ProcSeqShowSysNodes

 PURPOSE	:	Print the content of version to /proc file

 PARAMETERS	:	sfile - /proc seq_file
				el - Element to print
*****************************************************************************/
static void ProcSeqShowSysNodes(struct seq_file *sfile,void* el)
{
	PVRSRV_DEVICE_NODE *psDevNode;

	if(el == PVR_PROC_SEQ_START_TOKEN)
	{
		seq_printf( sfile,
						"Registered nodes\n"
						"Addr     Type     Class    Index Ref pvDev     Size Res\n");
		return;
	}

	psDevNode = (PVRSRV_DEVICE_NODE*)el;

	seq_printf( sfile,
			  "%p %-8s %-8s %4d  %2u  %p  %3u  %p\n",
			  psDevNode,
			  deviceTypeToString(psDevNode->sDevId.eDeviceType),
			  deviceClassToString(psDevNode->sDevId.eDeviceClass),
			  psDevNode->sDevId.eDeviceClass,
			  psDevNode->ui32RefCount,
			  psDevNode->pvDevice,
			  psDevNode->ui32pvDeviceSize,
			  psDevNode->hResManContext);
}

/*****************************************************************************
 FUNCTION	:	ProcSeqOff2ElementSysNodes

 PURPOSE	:	Transale offset to element (/proc stuff)

 PARAMETERS	:	sfile - /proc seq_file
				off - the offset into the buffer

 RETURNS    :   element to print
*****************************************************************************/
static void* ProcSeqOff2ElementSysNodes(struct seq_file * sfile, loff_t off)
{
    SYS_DATA *psSysData;
    PVRSRV_DEVICE_NODE*psDevNode = IMG_NULL;
    
    PVR_UNREFERENCED_PARAMETER(sfile);
    
    if(!off)
    {
	return PVR_PROC_SEQ_START_TOKEN;
    }

    psSysData = SysAcquireDataNoCheck();
    if (psSysData != IMG_NULL)
    {
	/* Find Dev Node */
	psDevNode = (PVRSRV_DEVICE_NODE*)
			List_PVRSRV_DEVICE_NODE_Any_va(psSysData->psDeviceNodeList,
													DecOffPsDev_AnyVaCb,
													&off);
    }

    /* Return anything that is not PVR_RPOC_SEQ_START_TOKEN and NULL */
    return (void*)psDevNode;
}

/*****************************************************************************
 End of file (proc.c)
*****************************************************************************/
