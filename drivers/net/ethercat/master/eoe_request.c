/*****************************************************************************
 *
 *  Copyright (C) 2006-2024  Florian Pose, Ingenieurgemeinschaft IgH
 *
 *  This file is part of the IgH EtherCAT Master.
 *
 *  The IgH EtherCAT Master is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License version 2, as
 *  published by the Free Software Foundation.
 *
 *  The IgH EtherCAT Master is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General
 *  Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with the IgH EtherCAT Master; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 ****************************************************************************/

/** \file
 * Ethernet-over-EtherCAT request functions.
 */

/****************************************************************************/

#include <linux/module.h>
#include <linux/jiffies.h>
#include <linux/slab.h>

#include "eoe_request.h"

/****************************************************************************/

/** EoE request constructor.
 */
void ec_eoe_request_init(
        ec_eoe_request_t *req /**< EoE request. */
        )
{
    INIT_LIST_HEAD(&req->list);
    req->state = EC_INT_REQUEST_INIT;
    req->jiffies_sent = 0U;

    req->mac_address_included = 0;
    req->ip_address_included = 0;
    req->subnet_mask_included = 0;
    req->gateway_included = 0;
    req->dns_included = 0;
    req->name_included = 0;

    memset(req->mac_address, 0x00, ETH_ALEN);
    req->ip_address.s_addr = 0;
    req->subnet_mask.s_addr = 0;
    req->gateway.s_addr = 0;
    req->dns.s_addr = 0;
    req->name[0] = 0x00;

    req->result = 0x0000;
}

/****************************************************************************/

/** Checks if EoE request has something to set.
 */
int ec_eoe_request_valid(
        const ec_eoe_request_t *req /**< EoE request. */
        )
{
    return
        req->mac_address_included ||
        req->ip_address_included ||
        req->subnet_mask_included ||
        req->gateway_included ||
        req->dns_included ||
        req->name_included;
}

/****************************************************************************/
