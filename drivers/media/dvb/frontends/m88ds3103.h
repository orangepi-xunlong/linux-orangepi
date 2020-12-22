/*
    Montage Technology M88DS3103/M88TS2022 - DVBS/S2 Satellite demod/tuner driver

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#ifndef M88DS3103_H
#define M88DS3103_H

#include <linux/dvb/frontend.h>

struct m88ds3103_config {
	/* the demodulator's i2c address */
	u8 demod_address;
	u8 ci_mode;
	u8 pin_ctrl;
	u8 ts_mode; /* 0: Parallel, 1: Serial */

	/* Set device param to start dma */
	int (*set_ts_params)(struct dvb_frontend *fe, int is_punctured);
    /* Start to transfer data */
    int (*start_ctrl)(struct dvb_frontend *fe);
    /* Set LNB voltage */
    int (*set_voltage)(struct dvb_frontend* fe, fe_sec_voltage_t voltage);
};

#if defined(CONFIG_DVB_M88DS3103) || \
	(defined(CONFIG_DVB_M88DS3103_MODULE) && defined(MODULE))
extern struct dvb_frontend *m88ds3103_attach(
       const struct m88ds3103_config *config,
       struct i2c_adapter *i2c);
#else
static inline struct dvb_frontend *m88ds3103_attach(
       const struct m88ds3103_config *config,
       struct i2c_adapter *i2c)
{
	printk(KERN_WARNING "%s: driver disabled by Kconfig\n", __func__);
	return NULL;
}
#endif /* CONFIG_DVB_M88DS3103 */
#endif /* M88DS3103_H */
