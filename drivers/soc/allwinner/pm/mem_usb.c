/*
 * Copyright (c) 2011-2020 yanggq.young@allwinnertech.com
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published by
 * the Free Software Foundation.
 */

#include "pm_i.h"

/* =========================================================
 * USB CHECK FOR WAKEUP SYSTEM FROM MEM
 * =========================================================
 */

/*
***************************************************************
*                                     mem_usb_init
*
* Description: init usb for mem.
*
* Arguments  : none;
*
* Returns    : none;
***************************************************************
*/
__s32 mem_usb_init(void)
{
	return 0;
}

/*
**************************************************************
*                                     mem_usb_exit
*
* Description: exit usb for mem.
*
* Arguments  : none;
*
* Returns    : none;
*************************************************************
*/
__s32 mem_usb_exit(void)
{
	return 0;
}

/*
*************************************************************
*                           mem_is_usb_status_change
*
*Description: check if usb status is change.
*
*Arguments  : port  usb port number;
*
*Return     : result, 0 status not change, !0 status
*
*Notes      :
*
***************************************************************
*/
__s32 mem_is_usb_status_change(__u32 port)
{
	return 0;
}

/*
***************************************************************
*                                     mem_query_usb_event
*
* Description: query usb event for wakeup system from mem.
*
* Arguments  : none;
*
* Returns    : result;
*               EPDK_TRUE,  some usb event happenned;
*               EPDK_FALSE, none usb event;
***************************************************************
*/
__s32 mem_query_usb_event(void)
{
	return -1;
}
