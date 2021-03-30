/*
 * drivers/char/sunxi-scr/smartcard.h
 *
 * Copyright (C) 2016 Allwinner.
 * fuzhaoke <fuzhaoke@allwinnertech.com>
 *
 * Header file ISO7816 smart card
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 */

#ifndef __SMARTCARD_H__
#define __SMARTCARD_H__

#include "sunxi-scr-user.h"

void smartcard_atr_decode(struct smc_atr_para *pscatr, struct smc_pps_para *psmc_pps,
			uint8_t *pdata, uint8_t with_ts);


#endif
