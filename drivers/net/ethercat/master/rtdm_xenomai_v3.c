/*****************************************************************************
 *
 *  Copyright (C) 2009-2010  Moehwald GmbH B. Benner
 *                     2011  IgH Andreas Stewering-Bone
 *                     2012  Florian Pose <fp@igh.de>
 *
 *  This file is part of the IgH EtherCAT master.
 *
 *  The IgH EtherCAT master is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License as published
 *  by the Free Software Foundation; version 2 of the License.
 *
 *  The IgH EtherCAT master is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General
 *  Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with the IgH EtherCAT master. If not, see <http://www.gnu.org/licenses/>.
 *
 ****************************************************************************/

/** \file
 * RTDM interface.
 */

#include <linux/module.h>
#include <linux/vmalloc.h>
#include <rtdm/driver.h>

#include "master.h"
#include "ioctl.h"
#include "rtdm.h"
#include "rtdm_details.h"

/** Set to 1 to enable device operations debugging.
 */
#define DEBUG_RTDM 0

/****************************************************************************/

static int ec_rtdm_open(struct rtdm_fd *fd, int oflags)
{
	struct ec_rtdm_context *ctx = rtdm_fd_to_private(fd);
#if DEBUG_RTDM
	struct rtdm_device *dev = rtdm_fd_device(fd);
	ec_rtdm_dev_t *rtdm_dev = dev->device_data;
#endif

	ctx->user_fd = fd;

	ctx->ioctl_ctx.writable = oflags & O_WRONLY || oflags & O_RDWR;
	ctx->ioctl_ctx.requested = 0;
	ctx->ioctl_ctx.process_data = NULL;
	ctx->ioctl_ctx.process_data_size = 0;

#if DEBUG_RTDM
	EC_MASTER_INFO(rtdm_dev->master, "RTDM device %s opened.\n",
			dev->name);
#endif

	return 0;
}

/****************************************************************************/

static void ec_rtdm_close(struct rtdm_fd *fd)
{
	struct ec_rtdm_context *ctx = rtdm_fd_to_private(fd);
	struct rtdm_device *dev = rtdm_fd_device(fd);
	ec_rtdm_dev_t *rtdm_dev = dev->device_data;

	if (ctx->ioctl_ctx.requested)
		ecrt_release_master(rtdm_dev->master);

	if (ctx->ioctl_ctx.process_data)
		vfree(ctx->ioctl_ctx.process_data);

#if DEBUG_RTDM
	EC_MASTER_INFO(rtdm_dev->master, "RTDM device %s closed.\n",
			dev->name);
#endif
}

/****************************************************************************/

static int ec_rtdm_ioctl_rt_handler(struct rtdm_fd *fd, unsigned int request,
			 void __user *arg)
{
	int result;
	struct ec_rtdm_context *ctx = rtdm_fd_to_private(fd);
	struct rtdm_device *dev = rtdm_fd_device(fd);
	ec_rtdm_dev_t *rtdm_dev = dev->device_data;

	result =
        ec_ioctl_rtdm_rt(rtdm_dev->master, &ctx->ioctl_ctx, request, arg);

	if (result == -ENOTTY)
		/* Try again with nrt ioctl handler below in secondary mode. */
		return -ENOSYS;

	return result;
}

/****************************************************************************/

static int ec_rtdm_ioctl_nrt_handler(struct rtdm_fd *fd, unsigned int request,
			 void __user *arg)
{
	struct ec_rtdm_context *ctx = rtdm_fd_to_private(fd);
	struct rtdm_device *dev = rtdm_fd_device(fd);
	ec_rtdm_dev_t *rtdm_dev = dev->device_data;

	return ec_ioctl_rtdm_nrt(rtdm_dev->master, &ctx->ioctl_ctx, request, arg);
}

/****************************************************************************/

static int ec_rtdm_mmap(struct rtdm_fd *fd, struct vm_area_struct *vma)
{
	struct ec_rtdm_context *ctx =
        (struct ec_rtdm_context *) rtdm_fd_to_private(fd);
	return rtdm_mmap_kmem(vma, (void *)ctx->ioctl_ctx.process_data);
}

/****************************************************************************/

static struct rtdm_driver ec_rtdm_driver = {
	.profile_info		= RTDM_PROFILE_INFO(ec_rtdm,
						    RTDM_CLASS_EXPERIMENTAL,
						    222,
						    0),
	.device_flags		= RTDM_NAMED_DEVICE,
	.device_count		= EC_MAX_MASTERS,
	.context_size		= sizeof(struct ec_rtdm_context),
	.ops = {
		.open		= ec_rtdm_open,
		.close		= ec_rtdm_close,
		.ioctl_rt	= ec_rtdm_ioctl_rt_handler,
		.ioctl_nrt	= ec_rtdm_ioctl_nrt_handler,
		.mmap		= ec_rtdm_mmap,
	},
};

/****************************************************************************/

int ec_rtdm_dev_init(ec_rtdm_dev_t *rtdm_dev, ec_master_t *master)
{
	struct rtdm_device *dev;
	int ret;

	rtdm_dev->master = master;

	rtdm_dev->dev = kzalloc(sizeof(struct rtdm_device), GFP_KERNEL);
	if (!rtdm_dev->dev) {
		EC_MASTER_ERR(master,
				"Failed to reserve memory for RTDM device.\n");
		return -ENOMEM;
	}

	dev = rtdm_dev->dev;

	dev->driver = &ec_rtdm_driver;
	dev->device_data = rtdm_dev;
	dev->label = "EtherCAT%u";
	dev->minor = master->index;

	ret = rtdm_dev_register(dev);
	if (ret) {
		EC_MASTER_ERR(master, "Initialization of RTDM interface failed"
				" (return value %i).\n", ret);
		kfree(dev);
		return ret;
	}

	EC_MASTER_INFO(master, "Registered RTDM device %s.\n", dev->name);

	return 0;
}

/****************************************************************************/

void ec_rtdm_dev_clear(ec_rtdm_dev_t *rtdm_dev)
{
	rtdm_dev_unregister(rtdm_dev->dev);

	EC_MASTER_INFO(rtdm_dev->master, "Unregistered RTDM device %s.\n",
			rtdm_dev->dev->name);

	kfree(rtdm_dev->dev);
}

/****************************************************************************/
