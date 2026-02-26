/*
 * Copyright (c) 2019 iComm Semiconductor Ltd.
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#define SSV6020_TURISMOE_EXT_DPLL_TABLE_VER "8.00"

ssv_cabrio_reg ssv6020_turismoE_ext_dpll_setting[]={
    {0xCBD0B000,0x24989815}, ///// XO frequency adjusting
// PMU
    {0xCBD0B00C,0x038EB5C0}, ///// rg_dcdc_pfm_sel_vref/rg_dcdc_pwm_sel_vref/rg_dcdc_hdrldo_sel_vref 1to0

// OrionA T40 PMU
// PAD MUX for HS5W
    {0xCBD0B070,0x00000001}, // bit[3:0] rg_pad_mux_sel set to 0x1
// PMU 40M reference clock for FPGA
    {0xCBD0B07C,0x00000011}, // bit[0] rg_fpga_clk_ref_40m_en

// change PMU mode after RF register initialization
// load rf table ready
    {0xCBD0B004,0xA4221000}, // bit[31] RG_LOAD_RFTABLE_RDY
};
