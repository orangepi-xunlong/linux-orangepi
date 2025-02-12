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
#include <linux/slab.h>
#include <linux/mman.h>


#include "master.h"
#include "ioctl.h"
#include "rtdm.h"
#include "rtdm_details.h"

/* include last because it does some redefinitions */
#include <rtdm/rtdm_driver.h>

/** Set to 1 to enable device operations debugging.
 */
#define DEBUG 0

/****************************************************************************/

static int ec_rtdm_open(struct rtdm_dev_context *, rtdm_user_info_t *, int);
static int ec_rtdm_close(struct rtdm_dev_context *, rtdm_user_info_t *);
static int ec_rtdm_ioctl_nrt_handler(struct rtdm_dev_context *,
        rtdm_user_info_t *, unsigned int, void __user *);
static int ec_rtdm_ioctl_rt_handler(struct rtdm_dev_context *,
        rtdm_user_info_t *, unsigned int, void __user *);

/****************************************************************************/

/** Initialize an RTDM device.
 *
 * \return Zero on success, otherwise a negative error code.
 */
int ec_rtdm_dev_init(
        ec_rtdm_dev_t *rtdm_dev, /**< EtherCAT RTDM device. */
        ec_master_t *master /**< EtherCAT master. */
        )
{
    int ret;

    rtdm_dev->master = master;

    rtdm_dev->dev = kzalloc(sizeof(struct rtdm_device), GFP_KERNEL);
    if (!rtdm_dev->dev) {
        EC_MASTER_ERR(master, "Failed to reserve memory for RTDM device.\n");
        return -ENOMEM;
    }

    rtdm_dev->dev->struct_version = RTDM_DEVICE_STRUCT_VER;
    rtdm_dev->dev->device_flags = RTDM_NAMED_DEVICE;
    rtdm_dev->dev->context_size = sizeof(ec_rtdm_context_t);
    snprintf(rtdm_dev->dev->device_name, RTDM_MAX_DEVNAME_LEN,
            "EtherCAT%u", master->index);
    rtdm_dev->dev->open_nrt = ec_rtdm_open;
    rtdm_dev->dev->ops.close_nrt = ec_rtdm_close;
    rtdm_dev->dev->ops.ioctl_rt = ec_rtdm_ioctl_rt_handler;
    rtdm_dev->dev->ops.ioctl_nrt = ec_rtdm_ioctl_nrt_handler;
    rtdm_dev->dev->device_class = RTDM_CLASS_EXPERIMENTAL;
    rtdm_dev->dev->device_sub_class = 222;
    rtdm_dev->dev->driver_name = "EtherCAT";
    rtdm_dev->dev->driver_version = RTDM_DRIVER_VER(1, 0, 2);
    rtdm_dev->dev->peripheral_name = rtdm_dev->dev->device_name;
    rtdm_dev->dev->provider_name = "EtherLab Community";
    rtdm_dev->dev->proc_name = rtdm_dev->dev->device_name;
    rtdm_dev->dev->device_data = rtdm_dev; /* pointer to parent */

    EC_MASTER_INFO(master, "Registering RTDM device %s.\n",
            rtdm_dev->dev->driver_name);
    ret = rtdm_dev_register(rtdm_dev->dev);
    if (ret) {
        EC_MASTER_ERR(master, "Initialization of RTDM interface failed"
                " (return value %i).\n", ret);
        kfree(rtdm_dev->dev);
    }

    return ret;
}

/****************************************************************************/

/** Clear an RTDM device.
 */
void ec_rtdm_dev_clear(
        ec_rtdm_dev_t *rtdm_dev /**< EtherCAT RTDM device. */
        )
{
    int ret;

    EC_MASTER_INFO(rtdm_dev->master, "Unregistering RTDM device %s.\n",
            rtdm_dev->dev->driver_name);
    ret = rtdm_dev_unregister(rtdm_dev->dev, 1000 /* poll delay [ms] */);
    if (ret < 0) {
        EC_MASTER_WARN(rtdm_dev->master,
                "Failed to unregister RTDM device (code %i).\n", ret);
    }

    kfree(rtdm_dev->dev);
}

/****************************************************************************/

/** Driver open.
 *
 * \return Always zero (success).
 */
static int ec_rtdm_open(
        struct rtdm_dev_context *context, /**< Context. */
        rtdm_user_info_t *user_info, /**< User data. */
        int oflags /**< Open flags. */
        )
{
    ec_rtdm_context_t *ctx = (ec_rtdm_context_t *) context->dev_private;
#if DEBUG
    ec_rtdm_dev_t *rtdm_dev = (ec_rtdm_dev_t *) context->device->device_data;
#endif

    ctx->user_fd = user_info;
    ctx->ioctl_ctx.writable = oflags & O_WRONLY || oflags & O_RDWR;
    ctx->ioctl_ctx.requested = 0;
    ctx->ioctl_ctx.process_data = NULL;
    ctx->ioctl_ctx.process_data_size = 0;

#if DEBUG
    EC_MASTER_INFO(rtdm_dev->master, "RTDM device %s opened.\n",
            context->device->device_name);
#endif
    return 0;
}

/****************************************************************************/

/** Driver close.
 *
 * \return Always zero (success).
 */
static int ec_rtdm_close(
        struct rtdm_dev_context *context, /**< Context. */
        rtdm_user_info_t *user_info /**< User data. */
        )
{
    ec_rtdm_context_t *ctx = (ec_rtdm_context_t *) context->dev_private;
    ec_rtdm_dev_t *rtdm_dev = (ec_rtdm_dev_t *) context->device->device_data;

    if (ctx->ioctl_ctx.requested) {
        ecrt_release_master(rtdm_dev->master);
	}

#if DEBUG
    EC_MASTER_INFO(rtdm_dev->master, "RTDM device %s closed.\n",
            context->device->device_name);
#endif
    return 0;
}

/****************************************************************************/

/** Driver ioctl.
 *
 * \return ioctl() return code.
 */
static int ec_rtdm_ioctl_nrt_handler(
        struct rtdm_dev_context *context, /**< Context. */
        rtdm_user_info_t *user_info, /**< User data. */
        unsigned int request, /**< Request. */
        void __user *arg /**< Argument. */
        )
{
    ec_rtdm_context_t *ctx = (ec_rtdm_context_t *) context->dev_private;
    ec_rtdm_dev_t *rtdm_dev = (ec_rtdm_dev_t *) context->device->device_data;

#if DEBUG
    EC_MASTER_INFO(rtdm_dev->master, "ioctl(request = %u, ctl = %02x)"
            " on RTDM device %s.\n", request, _IOC_NR(request),
            context->device->device_name);
#endif
    return ec_ioctl_rtdm_nrt(rtdm_dev->master, &ctx->ioctl_ctx, request, arg);
}

/****************************************************************************/

static int ec_rtdm_ioctl_rt_handler(
        struct rtdm_dev_context *context, /**< Context. */
        rtdm_user_info_t *user_info, /**< User data. */
        unsigned int request, /**< Request. */
        void __user *arg /**< Argument. */
        )
{
    int result;
    ec_rtdm_context_t *ctx = (ec_rtdm_context_t *) context->dev_private;
    ec_rtdm_dev_t *rtdm_dev = (ec_rtdm_dev_t *) context->device->device_data;

#if DEBUG
    EC_MASTER_INFO(rtdm_dev->master, "ioctl(request = %u, ctl = %02x)"
            " on RTDM device %s.\n", request, _IOC_NR(request),
            context->device->device_name);
#endif
    result =
        ec_ioctl_rtdm_rt(rtdm_dev->master, &ctx->ioctl_ctx, request, arg);

    if (result == -ENOTTY) {
		/* Try again with nrt ioctl handler above in secondary mode. */
        return -ENOSYS;
    }
    return result;
}

/****************************************************************************/

/** Memory-map process data to user space.
 *
 * \return Zero on success, otherwise a negative error code.
 */
int ec_rtdm_mmap(
        ec_ioctl_context_t *ioctl_ctx, /**< Context. */
        void **user_address /**< Userspace address. */
        )
{
    ec_rtdm_context_t *ctx =
        container_of(ioctl_ctx, ec_rtdm_context_t, ioctl_ctx);
    int ret;

    ret = rtdm_mmap_to_user(ctx->user_fd,
            ioctl_ctx->process_data, ioctl_ctx->process_data_size,
            PROT_READ | PROT_WRITE,
            user_address,
            NULL, NULL);
    if (ret < 0) {
        return ret;
    }

    return 0;
}

/****************************************************************************/
