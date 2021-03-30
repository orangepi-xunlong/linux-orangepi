#ifndef SUN8IW11_TH_H
#define SUN8IW11_TH_H

#define SENSOR_CNT              (2)
#define THS_CLK                 (24000000)

/* temperature = -0.1125*sensor + 250
 *	= (2500000 - 1125*sensor)/10000
 *	= (xxx - 117964.8*sensor)/1024/1024
 *	= ( MINUPA - reg * MULPA) / 2^DIVPA
 *	sensor value range:
 *				= 0 - 0xffff,ffff/2/117964
 *				= 0 - 18204
 */
#define MULPA                   (u32)(0.1125*1024*1024)
#define DIVPA                   (20)
#define MINUPA                  (250*1024*1024)

#define THS_CTRL0_REG		(0x00)
#define THS_CTRL1_REG		(0x04)
#define ADC_CDAT_REG		(0x14)
#define THS_CTRL2_REG           (0x40)
#define THS_INT_CTRL_REG        (0x44)
#define THS_INT_STA_REG         (0x48)
#define THS_INT_ALM_TH_REG0     (0x50)
#define THS_INT_ALM_TH_REG1     (0x54)
#define THS_INT_SHUT_TH_REG0    (0x60)
#define THS_INT_SHUT_TH_REG1    (0x64)
#define THS_FILT_CTRL_REG       (0x70)
#define THS_0_1_CDATA_REG       (0x74)
#define THS_DATA_REG0           (0x80)
#define THS_DATA_REG1           (0x84)

#define THS_INT_ALM_TH_VALUE0   (0x50)
#define THS_INT_ALM_TH_VALUE1   (0x54)
#define THS_INT_SHUT_TH_VALUE0  (0x60)
#define THS_INT_SHUT_TH_VALUE1  (0x64)

/* acq1 == acq0, acq time = 20us */
#define THS_CTRL0_VALUE		(0x1df)
/* calibration en */
#define THS_CTRL1_VALUE		(0x1<<17)
#define THS_CTRL2_VALUE		(0x01df0000)

/* per sampling takes: 10ms */
#define THS_INT_CTRL_VALUE	(0x3a070)
#define THS_CLEAR_INT_STA	(0x333)
/* got 1 data for per 8 sampling: time = 10ms * 8 = 80ms */
#define THS_FILT_CTRL_VALUE	(0x06)

#define THS_INTS_DATA0		(0x100)
#define THS_INTS_DATA1		(0x200)
#define THS_INTS_SHT0		(0x010)
#define THS_INTS_SHT1		(0x020)
#define THS_INTS_ALARM0		(0x001)
#define THS_INTS_ALARM1		(0x002)

#define SENS0_ENABLE_BIT		(0x1<<0)
#define SENS1_ENABLE_BIT		(0x1<<1)

#endif /* SUN8IW11_TH_H */
