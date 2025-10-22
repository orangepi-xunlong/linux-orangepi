/*
 * ad82128.h  --  ad82128 ALSA SoC Audio driver
 *
 * Copyright 1998 Elite Semiconductor Memory Technology
 *
 * Author: ESMT Audio/Power Product BU Team
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#ifndef __AD82128_H__
#define __AD82128_H__


#define AD82128_REGISTER_COUNT		 			 134
#define AD82128_RAM_TABLE_COUNT          120

/* Register Address Map */
#define AD82128_STATE_CTRL1_REG	0x00
#define AD82128_STATE_CTRL2_REG	0x01
#define AD82128_STATE_CTRL3_REG	0x02
#define AD82128_VOLUME_CTRL_REG	0x03
#define AD82128_STATE_CTRL5_REG	0x1A

#define CFADDR    0x1d
#define A1CF1     0x1e
#define A1CF2     0x1f
#define A1CF3     0x20
#define A1CF4     0x21
#define CFUD      0x32

#define AD82128_DEVICE_ID_REG	0x37
#define AD82128_ANALOG_CTRL_REG		0x5e

#define AD82128_FAULT_REG		0x7D
#define AD82128_MAX_REG			0x85

/* AD82128_STATE_CTRL2_REG */
#define AD82128_SSZ_DS			BIT(5)

/* AD82128_STATE_CTRL1_REG */
#define AD82128_SAIF_I2S		(0x0 << 5)
#define AD82128_SAIF_LEFTJ		(0x1 << 5)
#define AD82128_SAIF_FORMAT_MASK	GENMASK(7, 5)

/* AD82128_STATE_CTRL3_REG */
#define AD82128_MUTE			BIT(6)

/* AD82128_STATE_CTRL5_REG */
#define AD82128_SW_RESET			BIT(5)

/* AD82128_DEVICE_ID_REG */
#define AD82128_DEVICE_ID 0x00

/* AD82128_ANALOG_CTRL_REG */
#define AD82128_ANALOG_GAIN_15_5DBV	(0x0)
#define AD82128_ANALOG_GAIN_14_0DBV	(0x1)
#define AD82128_ANALOG_GAIN_13_0DBV	(0x2)
#define AD82128_ANALOG_GAIN_11_5DBV	(0x3)
#define AD82128_ANALOG_GAIN_MASK	GENMASK(1, 0)
#define AD82128_ANALOG_GAIN_SHIFT	(0x0)
/* AD82128_FAULT_REG */
#define AD82128_OCE			BIT(7)
#define AD82128_OTE			BIT(6)
#define AD82128_UVE			BIT(5)
#define AD82128_DCE			BIT(4)
#define AD82128_BSUVE		BIT(3)
#define AD82128_CLKE		BIT(2)

#endif /* __AD82128_H__ */
