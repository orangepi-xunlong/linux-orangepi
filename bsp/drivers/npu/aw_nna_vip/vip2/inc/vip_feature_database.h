/****************************************************************************
*
*    The MIT License (MIT)
*
*    Copyright (c) 2014 - 2024 Vivante Corporation
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
*    Copyright (C) 2014 - 2024 Vivante Corporation
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
/* auto generated on 2024-04-26 15:24:07 */
#ifndef _VIP_FEATUREDB_H
#define _VIP_FEATUREDB_H

struct _vip_hw_feature_db {
    vip_uint32_t pid;
    vip_uint32_t chip_revision             : 32; /* ChipRevision */
    vip_uint32_t nn_error_detect           : 1; /* gcFEATURE_BIT_NN_OUTPUT_OVERFLOW_MODE */
    vip_uint32_t job_cancel                : 1; /* gcFEATURE_BIT_NN_JOB_CANCELATION */
    vip_uint32_t mmu_pd_mode               : 1; /* gcFEATURE_BIT_MMU_PAGE_DESCRIPTOR */
    vip_uint32_t kernel_descriptor         : 1; /* gcFEATURE_BIT_NN_SINGLE_POSTMULT_FIELDS_IN_BITSTREAM */
    vip_uint32_t nn_transpose_phase2       : 1; /* gcFEATURE_BIT_NN_TRANSPOSE_PHASE2 */
    vip_uint32_t ocb_counter               : 1; /* gcFEATURE_BIT_OCB_COUNTER */
    vip_uint32_t nn_probe                  : 1; /* gcFEATURE_BIT_TP_NN_PROBE */
    vip_uint32_t sh_conformance            : 1; /* gcFEATURE_BIT_SH_CONFORMANCE_BRUTEFORCE_FIX */
    vip_uint32_t remove_mmu                : 1; /* gcFEATURE_BIT_VIP_REMOVE_MMU */
    vip_uint32_t nn_xydp0                  : 1; /* gcFEATURE_BIT_NN_XYDP0 */
    vip_uint32_t va_bits                   : 8; /* gcFEATURE_VALUE_VIRTUAL_ADDRESS_BITS */
    vip_uint32_t vip_core_count            : 8; /* gcFEATURE_VALUE_CoreCount */
    vip_uint32_t nn_core_count             : 8; /* gcFEATURE_VALUE_NNCoreCount */
    vip_uint32_t tp_core_count             : 8; /* gcFEATURE_VALUE_TPEngine_CoreCount */
    vip_uint32_t vip_sram_size             : 32; /* gcFEATURE_VALUE_VIP_SRAM_SIZE */
    vip_uint32_t axi_sram_size             : 32; /* gcFEATURE_VALUE_AXI_SRAM_SIZE */
    vip_uint32_t vip_sram_width            : 16; /* gcFEATURE_VALUE_PHYSICAL_VIP_SRAM_WIDTH_IN_BYTE */
    vip_uint32_t axi_bus_width             : 16; /* gcFEATURE_VALUE_AXI_BUS_WIDTH */
    vip_uint32_t latencyHiding_AtFullAxiBw : 16; /* gcFEATURE_VALUE_LATENCY_HIDING_AT_FULL_AXI_BW */
    vip_uint32_t min_axi_burst_size        : 16; /* gcFEATURE_VALUE_MIN_AXI_BURST_SIZE */
    vip_uint32_t ddr_kernel_burst_size     : 16; /* gcFEATURE_VALUE_DDR_KERNEL_BURST_SIZE */
};

static struct _vip_hw_feature_db vip_chip_info[] = {
    {
        0xee      , /* pid */
        0x8303    , /* chip_revision */
        0x0       , /* nn_error_detect */
        0x0       , /* job_cancel */
        0x0       , /* mmu_pd_mode */
        0x0       , /* kernel_descriptor */
        0x0       , /* nn_transpose_phase2 */
        0x1       , /* ocb_counter */
        0x1       , /* nn_probe */
        0x1       , /* sh_conformance */
        0x1       , /* remove_mmu */
        0x1       , /* nn_xydp0 */
        0x20      , /* va_bits */
        0x1       , /* vip_core_count */
        0x3       , /* nn_core_count */
        0x1       , /* tp_core_count */
        0x20000   , /* vip_sram_size */
        0x0       , /* axi_sram_size */
        0x40      , /* vip_sram_width */
        0x10      , /* axi_bus_width */
        0x80      , /* latencyHiding_AtFullAxiBw */
        0x100     , /* min_axi_burst_size */
        0x100     , /* ddr_kernel_burst_size */
    },
    {
        0x10000016, /* pid */
        0x9003    , /* chip_revision */
        0x0       , /* nn_error_detect */
        0x0       , /* job_cancel */
        0x0       , /* mmu_pd_mode */
        0x0       , /* kernel_descriptor */
        0x0       , /* nn_transpose_phase2 */
        0x1       , /* ocb_counter */
        0x1       , /* nn_probe */
        0x1       , /* sh_conformance */
        0x0       , /* remove_mmu */
        0x1       , /* nn_xydp0 */
        0x20      , /* va_bits */
        0x1       , /* vip_core_count */
        0x6       , /* nn_core_count */
        0x2       , /* tp_core_count */
        0x80000   , /* vip_sram_size */
        0x0       , /* axi_sram_size */
        0x40      , /* vip_sram_width */
        0x10      , /* axi_bus_width */
        0x80      , /* latencyHiding_AtFullAxiBw */
        0x100     , /* min_axi_burst_size */
        0x100     , /* ddr_kernel_burst_size */
    },
    {
        0x10000021, /* pid */
        0x9113    , /* chip_revision */
        0x0       , /* nn_error_detect */
        0x1       , /* job_cancel */
        0x1       , /* mmu_pd_mode */
        0x0       , /* kernel_descriptor */
        0x1       , /* nn_transpose_phase2 */
        0x1       , /* ocb_counter */
        0x1       , /* nn_probe */
        0x1       , /* sh_conformance */
        0x0       , /* remove_mmu */
        0x1       , /* nn_xydp0 */
        0x20      , /* va_bits */
        0x1       , /* vip_core_count */
        0xc       , /* nn_core_count */
        0x0       , /* tp_core_count */
        0x80000   , /* vip_sram_size */
        0x0       , /* axi_sram_size */
        0x80      , /* vip_sram_width */
        0x10      , /* axi_bus_width */
        0x80      , /* latencyHiding_AtFullAxiBw */
        0x100     , /* min_axi_burst_size */
        0x100     , /* ddr_kernel_burst_size */
    },
    {
        0x1000003b, /* pid */
        0x9202    , /* chip_revision */
        0x0       , /* nn_error_detect */
        0x1       , /* job_cancel */
        0x1       , /* mmu_pd_mode */
        0x0       , /* kernel_descriptor */
        0x1       , /* nn_transpose_phase2 */
        0x1       , /* ocb_counter */
        0x1       , /* nn_probe */
        0x1       , /* sh_conformance */
        0x0       , /* remove_mmu */
        0x1       , /* nn_xydp0 */
        0x20      , /* va_bits */
        0x1       , /* vip_core_count */
        0x8       , /* nn_core_count */
        0x0       , /* tp_core_count */
        0x80000   , /* vip_sram_size */
        0x0       , /* axi_sram_size */
        0x80      , /* vip_sram_width */
        0x10      , /* axi_bus_width */
        0x400     , /* latencyHiding_AtFullAxiBw */
        0x100     , /* min_axi_burst_size */
        0x100     , /* ddr_kernel_burst_size */
    },
};

#endif
