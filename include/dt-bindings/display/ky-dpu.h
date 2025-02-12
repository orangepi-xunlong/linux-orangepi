// SPDX-License-Identifier: GPL-2.0

#ifndef KY_DT_BINDINGS_DISPLAY_DPU_H
#define KY_DT_BINDINGS_DISPLAY_DPU_H

/* DPU type */
#define HDMI            0
#define DSI             1

/* DPU sub component */
#define ONLINE0		0
#define ONLINE1		1
#define ONLINE2		2
#define OFFLINE0	3
#define OFFLINE1	4

/* online/offline path id */
#define WB0		(1<<0)
#define WB1		(1<<1)
#define COMPOSER0	(1<<2)
#define COMPOSER1	(1<<3)
#define COMPOSER2	(1<<4)
#define COMPOSER3	(1<<5)
#define PP0		(1<<6)
#define PP1		(1<<7)
#define PP2		(1<<8)
#define SCALER0		(1<<9)
#define SCALER1		(1<<10)
#define SCALER2		(1<<11)
#define SCALER3		(1<<12)
#define SCALER4		(1<<13)
#define ACAD0		(1<<14)
#define ACAD1		(1<<15)
#define ACAD2		(1<<16)
#define LUT3D0		(1<<17)
#define LUT3D1		(1<<18)
#define LUT3D2		(1<<19)

#endif /* KY_DT_BINDINGS_DISPLAY_DPU_H */

