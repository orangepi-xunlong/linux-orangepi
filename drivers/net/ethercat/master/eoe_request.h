/*****************************************************************************
 *
 *  Copyright (C) 2006-2014  Florian Pose, Ingenieurgemeinschaft IgH
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

/**
   \file
   EtherCAT EoE request structure.
*/

/****************************************************************************/

#ifndef __EC_EOE_REQUEST_H__
#define __EC_EOE_REQUEST_H__

#include <linux/list.h>
#include <linux/etherdevice.h> // ETH_ALEN

#include "globals.h"

/****************************************************************************/

/** Ethernet-over-EtherCAT set IP parameter request.
 */
typedef struct {
    struct list_head list; /**< List item. */
    ec_internal_request_state_t state; /**< Request state. */
    unsigned long jiffies_sent; /**< Jiffies, when the request was sent. */

    uint8_t mac_address_included;
    uint8_t ip_address_included;
    uint8_t subnet_mask_included;
    uint8_t gateway_included;
    uint8_t dns_included;
    uint8_t name_included;

    unsigned char mac_address[ETH_ALEN];
    struct in_addr ip_address;
    struct in_addr subnet_mask;
    struct in_addr gateway;
    struct in_addr dns;
    char name[EC_MAX_HOSTNAME_SIZE];

    uint16_t result;
} ec_eoe_request_t;

/****************************************************************************/

void ec_eoe_request_init(ec_eoe_request_t *);
int ec_eoe_request_valid(const ec_eoe_request_t *);

/****************************************************************************/

#endif
