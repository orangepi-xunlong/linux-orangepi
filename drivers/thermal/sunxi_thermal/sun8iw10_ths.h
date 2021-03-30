#ifndef SUN8IW10_TH_H
#define SUN8IW10_TH_H

#define SENSOR_CNT              (2)
#define THS_CLK                 (24000000)

/* temperature = -0.118*sensor + 256
 *	= (256000 - 118*sensor)/1000
 *	= (262144 - 120.832*sensor)/1024
 *	= (268435456 - 123732*sensor)/1024/1024
 *	= ( MINUPA - reg * MULPA) / 2^DIVPA
 *	sensor value range:
 *				= 0 - 0xffff,ffff/2/123732
 *				= 0 - 17355
 */
#define MULPA                   (123732)
#define DIVPA                   (20)
#define MINUPA                  (268435456)

#define THS_CTRL_REG            (0x40)
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

#define THS_CTRL_VALUE		(0x1df<<16)
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

#endif /* SUN8IW10_TH_H */
