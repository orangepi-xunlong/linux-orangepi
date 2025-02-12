/*****************************************************************************
 *
 *  Copyright (C)      2024  Bjarne von Horn <vh@igh.de>
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

#ifndef __EC_RTDM_DETAILS_H__
#define __EC_RTDM_DETAILS_H__

#include "../config.h"
#include "ioctl.h"

#include <linux/kernel.h>

#ifdef EC_RTDM_XENOMAI_V3

#include <rtdm/driver.h>
#define EC_RTDM_USERFD_T struct rtdm_fd

#else // EC_RTDM_XENOMAI_V3

#include <rtdm/rtdm_driver.h>

#define EC_RTDM_USERFD_T rtdm_user_info_t

#endif // EC_RTDM_XENOMAI_V3

/****************************************************************************/

/** Context structure for an open RTDM file handle.
 */
typedef struct ec_rtdm_context {
    EC_RTDM_USERFD_T *user_fd; /**< RTDM user data. */
    ec_ioctl_context_t ioctl_ctx; /**< Context structure. */
} ec_rtdm_context_t;

/****************************************************************************/

static inline EC_RTDM_USERFD_T *ec_ioctl_to_rtdm(ec_ioctl_context_t *ctx)
{
    return container_of(ctx, ec_rtdm_context_t, ioctl_ctx)->user_fd;
}

/****************************************************************************/

#endif  // __EC_RTDM_DETAILS_H__
