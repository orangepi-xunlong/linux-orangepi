/*****************************************************************************
 *
 *  Copyright (C) 2006-2008  Florian Pose, Ingenieurgemeinschaft IgH
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
   EtherCAT SoE errors.
*/

/****************************************************************************/

#include "globals.h"

/****************************************************************************/

/** SoE error codes.
 */
const ec_code_msg_t soe_error_codes[] = {
    {0x1001, "No IDN"},
    {0x1009, "Invalid access to element 1"},
    {0x2001, "No name"},
    {0x2002, "Name transmission too short"},
    {0x2003, "Name transmission too long"},
    {0x2004, "Name cannot be changed, read only"},
    {0x2005, "Name is write protected at this time"},
    {0x3002, "Attribute transmission too short"},
    {0x3003, "Attribute transmission too long"},
    {0x3004, "Attribute cannot be changed, read only"},
    {0x3005, "Attribute is write protected at this time"},
    {0x4001, "No unit"},
    {0x4002, "Unit transmission too short"},
    {0x4003, "Unit transmission too long"},
    {0x4004, "Unit cannot be changed, read only"},
    {0x4005, "Unit is write proteced at this time"},
    {0x5001, "No minimum input value"},
    {0x5002, "Minimum input value transmission too short"},
    {0x5003, "Minimum input value transmission too long"},
    {0x5004, "Minimum input value cannot be changed, read only"},
    {0x5005, "Minimum input value is write protected at this time"},
    {0x6001, "No maximum input value"},
    {0x6002, "Maximum input value transmission too short"},
    {0x6003, "Maximum input value transmission too long"},
    {0x6004, "Maximum input value cannot be changed, read only"},
    {0x6005, "Maximum input value is write protected at this time"},
    {0x7002, "Operation data value transmission too short"},
    {0x7003, "Operation data value transmission too long"},
    {0x7004, "Operation data value cannot be changed, read only"},
    {0x7005, "Operation data value is write protected at this time"},
    {0x7006, "Operation data value is smaller than the minimum input value"},
    {0x7007, "Operation data value is greater than the minimum input value"},
    {0x7008, "Invalid operation data"},
    {0x7009, "Operation data is write protected by a password"},
    {0x700A, "Operation data is write protected"},
    {0x700B, "Invalid indirect addressing"},
    {0x700C, "Operation data is write protected due to other settings"},
    {0x700D, "Reserved"},
    {0x7010, "Procedure command already active"},
    {0x7011, "Procedure command not interruptible"},
    {0x7012, "Procedure command is at this time not executable"},
    {0x7013, "Procedure command not executable"},
    {0x7014, "No data state"},
    {0x8001, "No default value"},
    {0x8002, "Default value transmission too long"},
    {0x8004, "Default value cannot be changed, read only"},
    {0x800A, "Invalid drive number"},
    {0x800B, "General error"},
    {0x800C, "No element addressed"},
    {}
};

/****************************************************************************/
