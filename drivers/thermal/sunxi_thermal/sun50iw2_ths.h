#ifndef SUN50IW2_TH_H
#define SUN50IW2_TH_H

#define SENSOR_CNT              (2)
#define THS_CLK                 (24000000)

/* when  temperature <= 70C (sample_data >= 0x500)
 *	temperature = -0.1191*sensor + 223
 *		= (2230000 - 1191*sensor)/10000
 *		= (233832448 - 124885.4016*sensor)/1024/1024
 *		= (233832448 - 124885*sensor)/1024/1024
 *		= ( MINUPA - reg * MULPA) / 2^DIVPA
 *		sensor value range:
 *				= 0 - 0xffff,ffff/2/124885
 *				= 0 - 17195
 */
#define MULPA                   (u32)(0.1191*1024*1024)
#define DIVPA                   (20)
#define MINUPA                  (223*1024*1024)

/* when  temperature > 70C (sample_data < 0x500)
 *	cpu_temperature = -0.1452*sensor + 259
 *		= (2590000 - 1452*sensor)/10000
 *		= (271581184 - 152253.2352*sensor)/1024/1024
 *		= (271581184 - 152253*sensor)/1024/1024
 *		= ( MINUPA - reg * MULPA) / 2^DIVPA
 *		sensor value range:
 *				= 0 - 0xffff,ffff/2/152253
 *				= 0 - 14104
 *
 *	gpu_temperature = -0.159*sensor + 276
 *		= (2760000 - 1590*sensor)/10000
 *		= (289406976 - 166723.5840*sensor)/1024/1024
 *		= (289406976 - 166724*sensor)/1024/1024
 *		= ( MINUPA - reg * MULPA) / 2^DIVPA
 *		sensor value range:
 *				= 0 - 0xffff,ffff/2/166724
 *				= 0 - 2047
 */
#define CPU_MULPA                   (u32)(0.1452*1024*1024)
#define CPU_DIVPA                   (20)
#define CPU_MINUPA                  (259*1024*1024)

#define GPU_MULPA                   (u32)(0.159*1024*1024)
#define GPU_DIVPA                   (20)
#define GPU_MINUPA                  (276*1024*1024)

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

/* acq1 == acq0, acq time = 20us: freq = 24M/479, => acq=1/freq=20us */
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

#endif /* SUN50IW2_TH_H */
