/*
 * Copyright (c) 2015 iComm Semiconductor Ltd.
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

#ifndef _SSV6020_HWIF_H_
#define _SSV6020_HWIF_H_

#include <linux/ktime.h>
#include <linux/mutex.h>

#define CHIP_TYPE_CHIP 0xA0
#define CHIP_TYPE_FPGA 0x0F

#ifdef CONFIG_PM
struct target_state {
    struct mutex    ts_mutex;
    ktime_t         tstamp;
    u32             int_mask;
    u32             int_15_submask;
    u32             gpio_manual_io;
    u32             gpio_int_thru_gpio;
    u32             gpio_polarity;
};
#endif

#endif
