/****************************************************************************
*
*    The MIT License (MIT)
*
*    Copyright (c) 2017 - 2024 Vivante Corporation
*
*    Permission is hereby granted, free of charge, to any person obtaining a
*    copy of this software and associated documentation files (the "Software"),
*    to deal in the Software without restriction, including without limitation
*    the rights to use, copy, modify, merge, publish, distribute, sublicense,
*    and/or sell copies of the Software, and to permit persons to whom the
*    Software is furnished to do so, subject to the following conditions:
*
*    The above copyright notice and this permission notice shall be included in
*    all copies or substantial portions of the Software.
*
*    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
*    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
*    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
*    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
*    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
*    FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
*    DEALINGS IN THE SOFTWARE.
*
*****************************************************************************
*
*    The GPL License (GPL)
*
*    Copyright (C) 2017 - 2024 Vivante Corporation
*
*    This program is free software; you can redistribute it and/or
*    modify it under the terms of the GNU General Public License
*    as published by the Free Software Foundation; either version 2
*    of the License, or (at your option) any later version.
*
*    This program is distributed in the hope that it will be useful,
*    but WITHOUT ANY WARRANTY; without even the implied warranty of
*    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*    GNU General Public License for more details.
*
*    You should have received a copy of the GNU General Public License
*    along with this program; if not, write to the Free Software Foundation,
*    Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
*
*****************************************************************************
*
*    Note: This software is released under dual MIT and GPL licenses. A
*    recipient may use this file under the terms of either the MIT license or
*    GPL License. If you wish to use only one license not the other, you can
*    indicate your decision by deleting one of the above license notices in your
*    version of this file.
*
*****************************************************************************/

#include <vip_drv_fusa.h>
#include <vip_drv_debug.h>
#include <vip_drv_context.h>

#if vpmdENABLE_FUSA
vip_status_e vipdrv_fusa_init_interrupt(
    vipdrv_hardware_t *hardware
    )
{
    vip_status_e status = VIP_SUCCESS;
    vip_uint32_t value = 0;

    vipdrv_register_dump_wrapper((void*)vipdrv_read_register, hardware, 0x00344, &value);
    value |= 0xFFFFFFF; /* bit0 - bit27 to 1 */
    vipdrv_register_dump_wrapper((void*)vipdrv_write_register, hardware, 0x00344, value);


    vipdrv_register_dump_wrapper((void*)vipdrv_read_register, hardware, 0x00354, &value);
    value |= 0x1FFFFFFF; /* bit0 - bit28 to 1 */
    vipdrv_register_dump_wrapper((void*)vipdrv_write_register, hardware, 0x00354, value);

    return status;
}

vip_status_e vipdrv_fusa_check_irq_value(
    vipdrv_hardware_t *hardware
    )
{
    vip_status_e status = VIP_SUCCESS;
    vip_uint32_t value = 0, value_ext = 0;

    vipdrv_read_register(hardware, 0x00348, &value);
    vipdrv_write_register(hardware, 0x00348, 0xFFFFFFF);
    vipdrv_read_register(hardware, 0x00358, &value_ext);
    vipdrv_write_register(hardware, 0x00358, 0x1FFFFFFF);

    PRINTK_E("Check fusa value=0x%08x, value_ext=0x%08x\n", value, value_ext);

    return status;
}

vip_status_e vipdrv_fusa_polling(
    vipdrv_hardware_t *hardware
    )
{
    vip_status_e status = VIP_SUCCESS;
    vip_uint32_t value = 0, value_ext = 0;

    vipdrv_read_register(hardware, 0x00348, &value);
    value = SETBIT(value, 1, 0);
    vipdrv_read_register(hardware, 0x00358, &value_ext);

    if ((value != 0) || (value_ext != 0)) {
        PRINTK_E("dev%d hw%d fusa value_ext=0x%x value=0x%x\n",
            hardware->device->device_index, hardware->core_index, value_ext, value);
        vipdrv_write_register(hardware, 0x00348, 0xFFFFFFF);
        vipdrv_write_register(hardware, 0x00358, 0x1FFFFFFF);
        /* return the error code to application */
        status = VIP_ERROR_FUSA;
    }

    return status;
}
#endif

