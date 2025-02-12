/*
 * @File        pvr_sync_ioctl_common.c
 * @Title       Kernel driver for Android's sync mechanism
 * @Codingstyle LinuxKernel
 * @Copyright   Copyright (c) Imagination Technologies Ltd. All Rights Reserved
 * @License     Dual MIT/GPLv2
 *
 * The contents of this file are subject to the MIT license as set out below.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * Alternatively, the contents of this file may be used under the terms of
 * the GNU General Public License Version 2 ("GPL") in which case the provisions
 * of GPL are applicable instead of those above.
 *
 * If you wish to allow use of your version of this file only under the terms of
 * GPL, and not to allow others to use your version of this file under the terms
 * of the MIT license, indicate your decision by deleting the provisions above
 * and replace them with the notice and other provisions required by GPL as set
 * out in the file called "GPL-COPYING" included in this distribution. If you do
 * not delete the provisions above, a recipient may use your version of this file
 * under the terms of either the MIT license or GPL.
 *
 * This License is also included in this distribution in the file called
 * "MIT-COPYING".
 *
 * EXCEPT AS OTHERWISE STATED IN A NEGOTIATED AGREEMENT: (A) THE SOFTWARE IS
 * PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING
 * BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
 * PURPOSE AND NONINFRINGEMENT; AND (B) IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include <linux/slab.h>

#include "pvr_drm.h"
#include "pvr_sync_api.h"
#include "pvr_sync_ioctl_common.h"

#include "pvr_debug.h"
#include "pvr_uaccess.h"

/*
 * The PVR Sync API is unusual in that some operations configure the
 * timeline for use, and are no longer allowed once the timeline is
 * in use. A locking mechanism, such as a read/write semaphore, would
 * be one method of helping to ensure the API rules are followed, but
 * this would add unnecessary overhead once the timeline has been
 * configured, as read locks would continue to have to be taken after
 * the timeline is in use. To avoid locks, two atomic variables are used,
 * together with memory barriers. The in_setup variable indicates a "rename"
 * or "force software only" ioctl is in progress. At most one of these two
 * configuration ioctls can be in progress at any one time, and they can't
 * overlap with any other Sync ioctl. The in_use variable indicates one
 * of the other Sync ioctls has started. Once set, in_use stays set, and
 * prevents any further configuration ioctls. Non-configuration ioctls
 * are allowed to overlap.
 * It is possible for a configuration and non-configuration ioctl to race,
 * but at most one will be allowed to proceed, and perhaps neither.
 * Given the intended usage of the API in user space, where the timeline
 * is fully configured before being used, the race behaviour won't be
 * an issue.
 */

struct pvr_sync_file_data {
	atomic_t in_setup;
	atomic_t in_use;
	void *api_private;
	bool is_sw;
	bool is_export;
};

static bool pvr_sync_set_in_use(struct pvr_sync_file_data *fdata)
{
	if (atomic_read(&fdata->in_use) < 2) {
		atomic_set(&fdata->in_use, 1);
		/* Ensure in_use change is visible before in_setup is read */
		smp_mb();
		if (atomic_read(&fdata->in_setup) != 0)
			return false;

		atomic_set(&fdata->in_use, 2);
	} else {
		/* Ensure stale private data isn't read */
		smp_rmb();
	}

	return true;
}

static bool pvr_sync_set_in_setup(struct pvr_sync_file_data *fdata)
{
	int in_setup;

	in_setup = atomic_inc_return(&fdata->in_setup);
	if (in_setup > 1 || atomic_read(&fdata->in_use) != 0) {
		atomic_dec(&fdata->in_setup);
		return false;
	}

	return true;
}

static inline void pvr_sync_reset_in_setup(struct pvr_sync_file_data *fdata)
{
	/*
	 * Ensure setup changes are visible before allowing other
	 * operations to proceed.
	 */
	smp_mb__before_atomic();
	atomic_dec(&fdata->in_setup);
}

void *pvr_sync_get_api_priv_common(struct file *file)
{
	if (file != NULL && pvr_sync_is_timeline(file)) {
		struct pvr_sync_file_data *fdata = pvr_sync_get_private_data(file);

		if (fdata != NULL && pvr_sync_set_in_use(fdata))
			return fdata->api_private;
	}

	return NULL;
}

int pvr_sync_open_common(void *connection_data, void *file_handle)
{
	void *data = NULL;
	struct pvr_sync_file_data *fdata;
	int err;

	fdata = kzalloc(sizeof(*fdata), GFP_KERNEL);
	if (!fdata)
		return -ENOMEM;

	atomic_set(&fdata->in_setup, 0);
	atomic_set(&fdata->in_use, 0);

	if (!pvr_sync_set_private_data(connection_data, fdata)) {
		kfree(fdata);
		return -EINVAL;
	}

	err = pvr_sync_api_init(file_handle, &data);
	if (err)
		kfree(fdata);
	else
		fdata->api_private = data;

	return err;
}

int pvr_sync_close_common(void *connection_data)
{
	struct pvr_sync_file_data *fdata;

	fdata = pvr_sync_connection_private_data(connection_data);
	if (fdata) {
		int err;

		err = pvr_sync_api_deinit(fdata->api_private, fdata->is_sw);

		kfree(fdata);

		return err;
	}

	return 0;
}

#define PVR_SYNC_IOCTL_DISPATCH_INTERNAL_DECL(name, type) \
static inline int pvr_sync_ioctl_common_internal_ ## name(struct pvr_sync_file_data *fdata, type data)

PVR_SYNC_IOCTL_DISPATCH_INTERNAL_DECL(rename, struct pvr_sync_rename_ioctl_data *)
{
	return pvr_sync_api_rename(fdata->api_private, data);
}

PVR_SYNC_IOCTL_DISPATCH_INTERNAL_DECL(force_sw_only, void*)
{
	void *new_api_private = fdata->api_private;
	int err;

	PVR_UNREFERENCED_PARAMETER(data);

	err = pvr_sync_api_force_sw_only(fdata->api_private, &new_api_private);
	if (!err) {
		if (new_api_private != fdata->api_private)
			fdata->api_private = new_api_private;

		fdata->is_sw = true;
	}

	return err;
}

PVR_SYNC_IOCTL_DISPATCH_INTERNAL_DECL(force_exp_only, void*)
{
	int err;
	PVR_UNREFERENCED_PARAMETER(data);

	err = pvr_sync_api_force_exp_only(fdata->api_private, data);
	if (!err)
		fdata->is_export = true;

	return err;
}

PVR_SYNC_IOCTL_DISPATCH_INTERNAL_DECL(create_export_fence, void*)
{
	return pvr_sync_api_create_export_fence(fdata->api_private, data);
}

PVR_SYNC_IOCTL_DISPATCH_INTERNAL_DECL(sw_create_fence, struct pvr_sw_sync_create_fence_data *)
{
	return pvr_sync_api_sw_create_fence(fdata->api_private, data);
}

PVR_SYNC_IOCTL_DISPATCH_INTERNAL_DECL(sw_inc, struct pvr_sw_timeline_advance_data *)
{
	return pvr_sync_api_sw_inc(fdata->api_private, data);
}

/*
 * enum pvr_sync_ioctl_dispatch_type
 * @pvr_sync_ioctl_dispatch_type_setup: The command may only be used during the setup phase
 * @pvr_sync_ioctl_dispatch_type_export: The command may only be used with an export only timeline.
 * @pvr_sync_ioctl_dispatch_type_software: The command may only be used with a software only timeline.
 */
enum pvr_sync_ioctl_dispatch_type {
	pvr_sync_ioctl_dispatch_type_setup,
	pvr_sync_ioctl_dispatch_type_export,
	pvr_sync_ioctl_dispatch_type_software,
};

/* Generates a function `from` which performs validation on the data before passing it to `into` */
#define PVR_SYNC_IOCTL_DISPATCH_VALIDATE(name, structure, type)                                                                        \
PVR_SYNC_IOCTL_DISPATCH_DECL(name)                                                                                                     \
{                                                                                                                                      \
	int err = -ENOTTY;                                                                                                                 \
	structure server_data;                                                                                                             \
                                                                                                                                       \
	struct pvr_sync_file_data *fdata = pvr_sync_get_private_data(file);                                                                \
	if (unlikely(!fdata))                                                                                                              \
		return -EINVAL;                                                                                                                \
                                                                                                                                       \
	/* Check if the device is busy and the operation is valid for the timelines current state */                                       \
	if (type == pvr_sync_ioctl_dispatch_type_setup)                                                                                    \
	{                                                                                                                                  \
		if (!pvr_sync_set_in_setup(fdata))                                                                                             \
			return -EBUSY;                                                                                                             \
		if (fdata->is_sw || fdata->is_export)                                                                                          \
			goto return_;                                                                                                              \
	}                                                                                                                                  \
	else                                                                                                                               \
	{                                                                                                                                  \
		if (!pvr_sync_set_in_use(fdata))                                                                                               \
			return -EBUSY;                                                                                                             \
                                                                                                                                       \
		switch (type)                                                                                                                  \
		{                                                                                                                              \
		case pvr_sync_ioctl_dispatch_type_software: {                                                                                  \
			if (!fdata->is_sw) /* Not a software timeline, software operations cannot be used */                                       \
				goto return_;                                                                                                          \
		}                                                                                                                              \
		break;                                                                                                                         \
		case pvr_sync_ioctl_dispatch_type_export: {                                                                                    \
			if (!fdata->is_export) /* Not an export timeline, export operations cannot be used */                                      \
				goto return_;                                                                                                          \
		}                                                                                                                              \
		break;                                                                                                                         \
		default:                                                                                                                       \
			goto return_; /* Invalid Type */                                                                                           \
		}                                                                                                                              \
	}                                                                                                                                  \
                                                                                                                                       \
	/* copy_from_user */                                                                                                               \
	err = pvr_sync_ioctl_dispatch_copy_in__##name((structure __user *)user_data, &server_data);                                        \
	if (unlikely(err))                                                                                                                 \
		goto return_;                                                                                                                  \
                                                                                                                                       \
	/* Continue into api */                                                                                                            \
	err = pvr_sync_ioctl_common_internal_ ## name(fdata, (structure __force *) PVR_SYNC_IOCTL_DISPATCH_DATA(user_data, &server_data)); \
                                                                                                                                       \
	if (likely(!err))                                                                                                                  \
	{                                                                                                                                  \
		/* copy_to_user */                                                                                                             \
		err = pvr_sync_ioctl_dispatch_copy_out__##name((structure __user *)user_data, &server_data);                                   \
	}                                                                                                                                  \
                                                                                                                                       \
return_:                                                                                                                               \
	if (type == pvr_sync_ioctl_dispatch_type_setup)                                                                                    \
		pvr_sync_reset_in_setup(fdata);                                                                                                \
	return err;                                                                                                                        \
}

#if !defined(USE_PVRSYNC_DEVNODE)
/* drm_ioctl() already copies the data over, see comment on drm_ioctl_t */
#define PVR_SYNC_IOCTL_DISPATCH_DATA(pUM, pKM) pUM
#define PVR_SYNC_IOCTL_DISPATCH_COPY_WRAPPER(dir, name, structure, copy) \
INLINE static int pvr_sync_ioctl_dispatch_copy_ ## dir ## __ ## name (structure __user *pUM, structure *pKM) \
{ return 0; }
#else /* !defined(USE_PVRSYNC_DEVNODE) */
/* Generates a function to copy over the arguments to/from user-mode */
#define PVR_SYNC_IOCTL_DISPATCH_DATA(pUM, pKM) pKM
#define PVR_SYNC_IOCTL_DISPATCH_COPY_WRAPPER(dir, name, structure, copy)                                     \
INLINE static int pvr_sync_ioctl_dispatch_copy_ ## dir ## __ ## name (structure __user *pUM, structure *pKM) \
{                                                                                                            \
	/* May be unused if there are no in/out args */                                                          \
	PVR_UNREFERENCED_PARAMETER(pUM);                                                                         \
	PVR_UNREFERENCED_PARAMETER(pKM);                                                                         \
	/* Copy over the data */                                                                                 \
	{ copy }                                                                                                 \
	return 0;                                                                                                \
}
#endif /* !defined(USE_PVRSYNC_DEVNODE) */

/*************************************************************************/ /*!
@Function       PVR_SYNC_IOCTL_DISPATCH_COPY
@Description    Generates the code to copy from/to user mode depending on @dir.
@Input          dir  Either `from` or `to` corresponding to `copy_from_user`
                    and `copy_to_user` respectively.
@Input          to   Pointer for the dest.
@Input          from Pointer for the src.
*/ /**************************************************************************/
#define PVR_SYNC_IOCTL_DISPATCH_COPY(dir, to, from)        \
if (pvr_copy_##dir##_user(to, from, sizeof(*pKM)))         \
{                                                          \
	PVR_DPF((PVR_DBG_ERROR, "Failed copy " #dir " user")); \
	return -EFAULT;                                        \
}

/* Copy data from user */
#define PVR_SYNC_IOCTL_DISPATCH_COPY_IN \
PVR_SYNC_IOCTL_DISPATCH_COPY(from, pKM, pUM)

/* Copy data to user */
#define PVR_SYNC_IOCTL_DISPATCH_COPY_OUT \
PVR_SYNC_IOCTL_DISPATCH_COPY(to, pUM, pKM)

/* Copy no data */
#define PVR_SYNC_IOCTL_DISPATCH_COPY_NONE

/*************************************************************************/ /*!
@Function       PVR_SYNC_IOCTL_DISPATCH_FUNCTION
@Description    Generates a dispatch function which validates the ioctl command
                and dispatches it to the internal an function.
                Generates a dispatch function
                `pvr_sync_ioctl_common_##name(struct file*, void*)`
                which validates the ioctl command and dispatches it to
                an internal function
                `pvr_sync_ioctl_common_internal_##name(struct pvr_sync_file_data*, structure*)`
@Input          name      A name for the dispatch functions, must be unique.
@Input          structure The type of the user_data.
@Input          type      The type specifies when the ioctls are allowed or
                          if the ioctl is allowed on the current
                          timeline configuration.
@Input          copy_in   Either `COPY_IN` or `COPY_NONE`
@Input          copy_out  Either `COPY_OUT` or `COPY_NONE`
*/ /**************************************************************************/
#define PVR_SYNC_IOCTL_DISPATCH_FUNCTION(name, structure, type, copy_in, copy_out) \
	PVR_SYNC_IOCTL_DISPATCH_COPY_WRAPPER(in, name, structure, PVR_SYNC_IOCTL_DISPATCH_ ## copy_in) \
	PVR_SYNC_IOCTL_DISPATCH_COPY_WRAPPER(out, name, structure, PVR_SYNC_IOCTL_DISPATCH_ ## copy_out) \
	PVR_SYNC_IOCTL_DISPATCH_VALIDATE(name, structure, pvr_sync_ioctl_dispatch_type_ ## type)


PVR_SYNC_IOCTL_DISPATCH_FUNCTION(rename, struct pvr_sync_rename_ioctl_data,
                                 setup,
                                 COPY_IN,
                                 COPY_NONE);

PVR_SYNC_IOCTL_DISPATCH_FUNCTION(force_sw_only, void*,
                                 setup,
                                 COPY_NONE,
                                 COPY_NONE);

PVR_SYNC_IOCTL_DISPATCH_FUNCTION(force_exp_only, void*,
                                 setup,
                                 COPY_NONE,
                                 COPY_NONE);

PVR_SYNC_IOCTL_DISPATCH_FUNCTION(sw_create_fence, struct pvr_sw_sync_create_fence_data,
                                 software,
                                 COPY_IN,
                                 COPY_OUT);

PVR_SYNC_IOCTL_DISPATCH_FUNCTION(create_export_fence, pvr_exp_sync_create_fence_data_t,
                                 export,
                                 COPY_IN,
                                 COPY_OUT);

PVR_SYNC_IOCTL_DISPATCH_FUNCTION(sw_inc, struct pvr_sw_timeline_advance_data,
                                 software,
                                 COPY_NONE,
                                 COPY_OUT);
