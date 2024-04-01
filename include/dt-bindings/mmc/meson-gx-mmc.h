/* SPDX-License-Identifier: (GPL-2.0+ or MIT) */
/*
 * Copyright (c) 2022 Vyacheslav Bocharov
 * Author: Vyacheslav Bocharov <adeep@lexina.in>
 */

#ifndef _DT_BINDINGS_MESON_GX_MMC_H
#define _DT_BINDINGS_MESON_GX_MMC_H

/*
 * Cfg_rx_phase: RX clock phase
 * bits: 9:8 R/W
 * default: 0
 * Recommended value: 0
 *
 * Cfg_tx_phase: TX clock phase
 * bits: 9:8 R/W
 * default: 0
 * Recommended value: 2
 *
 * Cfg_co_phase: Core clock phase
 * bits: 9:8 R/W
 * default: 0
 * Recommended value: 2
 *
 * values: 0: 0 phase, 1: 90 phase, 2: 180 phase, 3: 270 phase.
 */

#define   CLK_PHASE_0 0
#define   CLK_PHASE_90 1
#define   CLK_PHASE_180 2
#define   CLK_PHASE_270 3


#endif
