/*
 * include/linux/arisc/arisc.h
 *
 * Copyright 2012 (c) Allwinner.
 * sunny (sunny@allwinnertech.com)
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 */

#ifndef	__ASM_ARCH_ARISC_H
#define	__ASM_ARCH_ARISC_H

#include <linux/power/aw_pm.h>

#define NMI_INT_TYPE_PMU (0)
#define NMI_INT_TYPE_RTC (1)
#define NMI_INT_TYPE_PMU_OFFSET (0x1 << NMI_INT_TYPE_PMU)
#define NMI_INT_TYPE_RTC_OFFSET (0x1 << NMI_INT_TYPE_RTC)

/* the modes of arisc dvfs */
#define	ARISC_DVFS_SYN		(1<<0)

/* message attributes(only use 8bit) */
#define	ARISC_MESSAGE_ATTR_ASYN		    (0<<0)	/* need asyn with another cpu     */
#define	ARISC_MESSAGE_ATTR_SOFTSYN	    (1<<0)	/* need soft syn with another cpu */
#define	ARISC_MESSAGE_ATTR_HARDSYN	    (1<<1)	/* need hard syn with another cpu */

/* axp driver interfaces */
#define AXP_TRANS_BYTE_MAX	(4)
#define RSB_TRANS_BYTE_MAX	(4)
#define P2WI_TRANS_BYTE_MAX	(8)

/* RSB devices' address */
#define RSB_DEVICE_SADDR1   	(0x3A3) /* (0x01d1)AXP22x(AW1669) */
#define RSB_DEVICE_SADDR3  		(0x745) /* (0x03a2)AXP15x(AW1657) */
#define RSB_DEVICE_SADDR7  		(0xE89) /* (0x0744)Audio codec, AC100 */

/* RSB run time address */
#define RSB_RTSADDR_AXP809  (0x2d)
#define RSB_RTSADDR_AXP806  (0x3a)
#define RSB_RTSADDR_AC100   (0x4e)

/* audio sram base address */
#define AUDIO_SRAM_BASE_PALY            (0x08117000)
#define AUDIO_SRAM_BASE_CAPTURE         (0x0811f000)

#define AUDIO_SRAM_BUF_SIZE_02K  (2048)     /* buffer size 2k  = 0x800  = 2048  */
#define AUDIO_SRAM_BUF_SIZE_04K  (4096)     /* buffer size 4k  = 0x1000 = 4096  */
#define AUDIO_SRAM_BUF_SIZE_08K  (8192)     /* buffer size 8k  = 0x2000 = 8192  */
#define AUDIO_SRAM_BUF_SIZE_16K (16384)     /* buffer size 16k = 0x4000 = 16384 */
#define AUDIO_SRAM_BUF_SIZE_32K (32768)     /* buffer size 32k = 0x8000 = 32768 */

#define AUDIO_SRAM_PER_SIZE_02K  (2048)     /* period size 2k  = 0x800  = 2048  */
#define AUDIO_SRAM_PER_SIZE_04K  (4096)     /* period size 4k  = 0x1000 = 4096  */
#define AUDIO_SRAM_PER_SIZE_08K  (8192)     /* period size 8k  = 0x2000 = 8192  */
#define AUDIO_SRAM_PER_SIZE_16K (16384)     /* period size 16k = 0x4000 = 16384 */
#define AUDIO_SRAM_PER_SIZE_32K (32768)     /* period size 32k = 0x8000 = 32768 */

//pmu voltage types
typedef enum power_voltage_type
{
	AXP809_POWER_VOL_DCDC1 = 0x0,
	AXP809_POWER_VOL_DCDC2,
	AXP809_POWER_VOL_DCDC3,
	AXP809_POWER_VOL_DCDC4,
	AXP809_POWER_VOL_DCDC5,
	AXP809_POWER_VOL_DC5LDO,
	AXP809_POWER_VOL_ALDO1,
	AXP809_POWER_VOL_ALDO2,
	AXP809_POWER_VOL_ALDO3,
	AXP809_POWER_VOL_DLDO1,
	AXP809_POWER_VOL_DLDO2,
	AXP809_POWER_VOL_ELDO1,
	AXP809_POWER_VOL_ELDO2,
	AXP809_POWER_VOL_ELDO3,

	AXP806_POWER_VOL_DCDCA,
	AXP806_POWER_VOL_DCDCB,
	AXP806_POWER_VOL_DCDCC,
	AXP806_POWER_VOL_DCDCD,
	AXP806_POWER_VOL_DCDCE,
	AXP806_POWER_VOL_ALDO1,
	AXP806_POWER_VOL_ALDO2,
	AXP806_POWER_VOL_ALDO3,
	AXP806_POWER_VOL_BLDO1,
	AXP806_POWER_VOL_BLDO2,
	AXP806_POWER_VOL_BLDO3,
	AXP806_POWER_VOL_BLDO4,
	AXP806_POWER_VOL_CLDO1,
	AXP806_POWER_VOL_CLDO2,
	AXP806_POWER_VOL_CLDO3,

	OZ80120_POWER_VOL_DCDC,

	POWER_VOL_MAX,
} power_voltage_type_e;

/* the pll of arisc dvfs */
typedef enum arisc_pll_no {
	ARISC_DVFS_PLL1 = 1,
	ARISC_DVFS_PLL2 = 2
} arisc_pll_no_e;

/* rsb transfer data type */
typedef enum arisc_rsb_datatype {
	RSB_DATA_TYPE_BYTE  = 1,
	RSB_DATA_TYPE_HWORD = 2,
	RSB_DATA_TYPE_WORD  = 4
} arisc_rsb_datatype_e;

#if defined CONFIG_ARCH_SUN8IW1P1
typedef enum arisc_p2wi_bits_ops {
	P2WI_CLR_BITS,
	P2WI_SET_BITS
} arisc_p2wi_bits_ops_e;
#elif (defined CONFIG_ARCH_SUN8IW3P1) || (defined CONFIG_ARCH_SUN8IW5P1) || (defined CONFIG_ARCH_SUN8IW6P1) || \
      (defined CONFIG_ARCH_SUN8IW7P1) || (defined CONFIG_ARCH_SUN8IW9P1) || (defined CONFIG_ARCH_SUN9IW1P1)
/* rsb transfer data type */
typedef enum arisc_rsb_bits_ops {
	RSB_CLR_BITS,
	RSB_SET_BITS
} arisc_rsb_bits_ops_e;
#endif

typedef enum arisc_audio_mode {
	AUDIO_PLAY,                   /* play    mode */
	AUDIO_CAPTURE                 /* capture mode */
} arisc_audio_mode_e;

typedef struct arisc_audio_mem
{
    unsigned int mode;
	unsigned int sram_base_addr;
	unsigned int buffer_size;
	unsigned int period_size;
}arisc_audio_mem_t;

typedef struct arisc_audio_tdm
{
    unsigned int mode;
	unsigned int samplerate;
	unsigned int channum;
}arisc_audio_tdm_t;

/* arisc call-back */
typedef int (*arisc_cb_t)(void *arg);

/* sunxi_perdone_cbfn
 *
 * period done callback routine type
*/
/* audio callback struct */
typedef struct audio_cb {
	arisc_cb_t	handler;	/* dma callback fuction */
	void 		*arg;	    /* args of func         */
}audio_cb_t;

#if defined CONFIG_ARCH_SUN8IW1P1
/*
 * @len :       number of read registers, max len:8;
 * @msgattr:    message attribute, 0:async, 1:soft sync, 2:hard aync
 * @addr:       point of registers address;
 * @data:       point of registers data;
 */
typedef struct arisc_p2wi_block_cfg
{
    unsigned int len;
    unsigned int msgattr;
	unsigned char *addr;
	unsigned char *data;
}arisc_p2wi_block_cfg_t;

/*
 * @len  :       number of operate registers, max len:8;
 * @msgattr:     message attribute, 0:async, 1:soft sync, 2:hard aync
 * @ops:         bits operation, 0:clear bits, 1:set bits
 * @addr :       point of registers address;
 * @mask :       point of mask bits data;
 * @delay:       point of delay times;
 */
typedef struct arisc_p2wi_bits_cfg
{
    unsigned int len;
    unsigned int msgattr;
    unsigned int ops;
	unsigned char *addr;
	unsigned char *mask;
	unsigned char *delay;
}arisc_p2wi_bits_cfg_t;
#elif (defined CONFIG_ARCH_SUN8IW3P1) || (defined CONFIG_ARCH_SUN8IW5P1) || (defined CONFIG_ARCH_SUN8IW6P1) || \
      (defined CONFIG_ARCH_SUN8IW7P1) || (defined CONFIG_ARCH_SUN8IW9P1) || (defined CONFIG_ARCH_SUN9IW1P1)
/*
 * @len :       number of read registers, max len:4;
 * @datatype:   type of the data, 0:byte(8bits), 1:halfword(16bits), 2:word(32bits)
 * @msgattr:    message attribute, 0:async, 1:soft sync, 2:hard aync
 * @devaddr:    devices address;
 * @regaddr:    array of registers address;
 * @data:       array of registers data;
 */
typedef struct arisc_rsb_block_cfg
{
    unsigned int len;
    unsigned int datatype;
    unsigned int msgattr;
    unsigned int devaddr;
	unsigned char *regaddr;
	unsigned int *data;
}arisc_rsb_block_cfg_t;

/*
 * @len  :       number of operate registers, max len:4;
 * @datatype:    type of the data, 0:byte(8bits), 1:halfword(16bits), 2:word(32bits)
 * @msgattr:     message attribute, 0:async, 1:soft sync, 2:hard aync
 * @ops:         bits operation, 0:clear bits, 1:set bits
 * @devaddr :    devices address;
 * @regaddr :    point of registers address;
 * @mask :       point of mask bits data;
 * @delay:       point of delay times;
 */
typedef struct arisc_rsb_bits_cfg
{
    unsigned int len;
    unsigned int datatype;
    unsigned int msgattr;
    unsigned int ops;
    unsigned int devaddr;
	unsigned char *regaddr;
	unsigned char *delay;
	unsigned int *mask;
}arisc_rsb_bits_cfg_t;
#endif

typedef enum arisc_rw_type
{
	ARISC_READ = 0x0,
	ARISC_WRITE = 0x1,
} arisc_rw_type_e;

typedef struct nmi_isr
{
	arisc_cb_t   handler;
	void        *arg;
} nmi_isr_t;

extern nmi_isr_t nmi_isr_node[2];

/*
 * @flags: bit1:dram enter self-refresh, bit16:C1/C2 state.
 * @mpidr: cpu info.
 */
typedef struct sunxi_cpuidle_para{
	unsigned long flags;
	unsigned long mpidr;
}sunxi_cpuidle_para_t;

/* ====================================dvfs interface==================================== */
/*
 * set specific pll target frequency.
 * @freq:    target frequency to be set, based on KHZ;
 * @pll:     which pll will be set
 * @mode:    the attribute of message, whether syn or asyn;
 * @cb:      callback handler;
 * @cb_arg:  callback handler arguments;
 *
 * return: result, 0 - set frequency successed,
 *                !0 - set frequency failed;
 */
int arisc_dvfs_set_cpufreq(unsigned int freq, unsigned int pll, unsigned long mode, arisc_cb_t cb, void *cb_arg);

/*
 * enter cpu idle.
 * @cb: callback function.
 * @cb_arg: arg for callback function.
 * @pars: arg for enter cpuidle.
 *
 * return: result, 0 - enter cpuidle successed, !0 - failed;
 */
extern int arisc_enter_cpuidle(arisc_cb_t cb, void *cb_arg, \
                               struct sunxi_cpuidle_para *para);
extern int arisc_config_cpuidle(arisc_cb_t cb, void *cb_arg, \
                                struct sunxi_cpuidle_para *para);

/* ====================================standby interface==================================== */
/**
 * enter super standby.
 * @para:  parameter for enter normal standby.
 *
 * return: result, 0 - super standby successed, !0 - super standby failed;
 */
int arisc_standby_super(struct super_standby_para *para, arisc_cb_t cb, void *cb_arg);

/**
 * query super-standby wakeup source.
 * @para:  point of buffer to store wakeup event informations.
 *
 * return: result, 0 - query successed, !0 - query failed;
 */
int arisc_query_wakeup_source(unsigned int *event);

extern int arisc_query_set_standby_info(struct standby_info_para *para, arisc_rw_type_e op);

/*
 * query config of standby power state and consumer.
 * @para:  point of buffer to store power informations.
 *
 * return: result, 0 - query successed, !0 - query failed;
 */
extern int arisc_query_standby_power_cfg(struct standby_info_para *para);

/*
 * set config of standby power state and consumer.
 * @para:  point of buffer to store power informations.
 *
 * return: result, 0 - set successed, !0 - set failed;
 */
#define arisc_set_standby_power_cfg(para) \
	arisc_query_set_standby_info(para, ARISC_WRITE)

/*
 * query standby power state and consumer.
 * @para:  point of buffer to store power informations.
 *
 * return: result, 0 - query successed, !0 - query failed;
 */
#define arisc_query_standby_power(para) \
	arisc_query_set_standby_info(para, ARISC_READ)

/**
 * query super-standby dram crc result.
 * @perror:  pointer of dram crc result.
 * @ptotal_count: pointer of dram crc total count
 * @perror_count: pointer of dram crc error count
 *
 * return: result, 0 - query successed,
 *                !0 - query failed;
 */
int arisc_query_dram_crc_result(unsigned int *perror, unsigned int *ptotal_count,
	unsigned int *perror_count);

int arisc_set_dram_crc_result(unsigned int error, unsigned int total_count,
	unsigned int error_count);

/**
 * notify arisc cpux restored.
 * @para:  none.
 *
 * return: result, 0 - notify successed, !0 - notify failed;
 */
int arisc_cpux_ready_notify(void);

/* talk-standby interfaces */
int arisc_standby_talk(struct super_standby_para *para, arisc_cb_t cb, void *cb_arg);
int arisc_cpux_talkstandby_ready_notify(void);

#if defined CONFIG_ARCH_SUN8IW1P1
void arisc_fake_power_off(void);
#endif

/* ====================================axp interface==================================== */
/**
 * register call-back function, call-back function is for arisc notify some event to ac327,
 * axp/rtc interrupt for external interrupt NMI.
 * @type:  nmi type, pmu/rtc;
 * @func:  call-back function;
 * @para:  parameter for call-back function;
 *
 * @return: result, 0 - register call-back function successed;
 *                 !0 - register call-back function failed;
 * NOTE: the function is like "int callback(void *para)";
 *       this function will execute in system ISR.
 */
int arisc_nmi_cb_register(u32 type, arisc_cb_t func, void *para);

/**
 * unregister call-back function.
 * @type:  nmi type, pmu/rtc;
 * @func:  call-back function which need be unregister;
 */
void arisc_nmi_cb_unregister(u32 type, arisc_cb_t func);

int arisc_disable_nmi_irq(void);
int arisc_enable_nmi_irq(void);

int arisc_axp_get_chip_id(unsigned char *chip_id);
#if (defined CONFIG_ARCH_SUN8IW5P1)
int arisc_adjust_pmu_chgcur(unsigned int max_chgcur, unsigned int chg_ic_temp);
#endif

int arisc_set_led_bln(unsigned long led_rgb, unsigned long led_onms,  \
                      unsigned long led_offms, unsigned long led_darkms);

/* ====================================audio interface==================================== */
/**
 * start audio play or capture.
 * @mode:    start audio in which mode ; 0:play, 1;capture.
 *
 * return: result, 0 - start audio play or capture successed,
 *                !0 - start audio play or capture failed.
 */
int arisc_audio_start(int mode);

/**
 * stop audio play or capture.
 * @mode:    stop audio in which mode ; 0:play, 1;capture.
 *
 * return: result, 0 - stop audio play or capture successed,
 *                !0 - stop audio play or capture failed.
 */
int arisc_audio_stop(int mode);

/**
 * set audio buffer and period parameters.
 * @audio_mem:
 *             mode          :which mode be set; 0:paly, 1:capture;
 *             sram_base_addr:sram base addr of buffer;
 *             buffer_size   :the size of buffer;
 *             period_size   :the size of period;
 *
 * |period|period|period|period|...|period|period|period|period|...|
 * | paly                   buffer | capture                buffer |
 * |                               |
 * 1                               2
 * 1:paly sram_base_addr,          2:capture sram_base_addr;
 * buffer size = capture sram_base_addr - paly sram_base_addr.
 *
 * return: result, 0 - set buffer and period parameters successed,
 *                !0 - set buffer and period parameters failed.
 *
 */
int arisc_buffer_period_paras(struct arisc_audio_mem audio_mem);

/**
 * get audio play or capture real-time address.
 * @mode:    in which mode; 0:play, 1;capture;
 * @addr:    real-time address in which mode.
 *
 * return: result, 0 - get real-time address successed,
 *                !0 - get real-time address failed.
 */
int arisc_get_position(int mode, unsigned int *addr);

/**
 * register audio callback function.
 * @mode:    in which mode; 0:play, 1;capture;
 * @handler: audio callback handler which need be register;
 * @arg    : the pointer of audio callback arguments.
 *
 * return: result, 0 - register audio callback function successed,
 *                !0 - register audio callback function failed.
 */
int arisc_audio_cb_register(int mode, arisc_cb_t handler, void *arg);

/**
 * unregister audio callback function.
 * @mode:    in which mode; 0:play, 1;capture;
 * @handler: audio callback handler which need be register;
 * @arg    : the pointer of audio callback arguments.
 *
 * return: result, 0 - unregister audio callback function successed,
 *                !0 - unregister audio callback function failed.
 */
int arisc_audio_cb_unregister(int mode, arisc_cb_t handler);

/**
 * set audio tdm parameters.
 * @tdm_cfg: audio tdm struct
 *           mode      :in which mode; 0:play, 1;capture;
 *           samplerate:tdm samplerate depend on audio data;
 *           channel   :audio channel number, 1 or 2.

 * return: result, 0 - set buffer and period parameters successed,
 *                !0 - set buffer and period parameters failed.
 *
 */
int arisc_tdm_paras(struct arisc_audio_tdm tdm_cfg);

/**
 * add audio period.
 * @mode:    start audio in which mode ; 0:play, 1;capture.
 * @addr:    period address which will be add in buffer
 *
 * return: result, 0 - add audio period successed,
 *                !0 - add audio period failed.
 *
 */
int arisc_add_period(u32 mode, u32 addr);

/* ====================================p2wi/rsb interface==================================== */
#if defined CONFIG_ARCH_SUN8IW1P1
/**
 * p2wi read block data.
 * @cfg:    point of arisc_p2wi_block_cfg struct;
 *
 * return: result, 0 - read register successed,
 *                !0 - read register failed or the len more then max len;
 */
int arisc_p2wi_read_block_data(struct arisc_p2wi_block_cfg *cfg);

/**
 * p2wi write block data.
 * @cfg:    point of arisc_p2wi_block_cfg struct;
 *
 * return: result, 0 - write register successed,
 *                !0 - write register failedor the len more then max len;
 */
int arisc_p2wi_write_block_data(struct arisc_p2wi_block_cfg *cfg);

/**
 * p2wi bits operation sync.
 * @cfg:    point of arisc_p2wi_bits_cfg struct;
 *
 * return: result, 0 - bits operation successed,
 *                !0 - bits operation failed, or the len more then max len;
 *
 * p2wi clear bits internal:
 * data = p2wi_read(addr);
 * data = data & (~mask);
 * p2wi_write(addr, data);
 *
 * p2wi set bits internal:
 * data = p2wi_read(addr);
 * data = data | mask;
 * p2wi_write(addr, data);
 *
 */
int p2wi_bits_ops_sync(struct arisc_p2wi_bits_cfg *cfg);

#elif (defined CONFIG_ARCH_SUN8IW3P1) || (defined CONFIG_ARCH_SUN8IW5P1) || (defined CONFIG_ARCH_SUN8IW6P1) || \
      (defined CONFIG_ARCH_SUN8IW7P1) || (defined CONFIG_ARCH_SUN8IW9P1) || (defined CONFIG_ARCH_SUN9IW1P1)
/**
 * rsb read block data.
 * @cfg:    point of arisc_rsb_block_cfg struct;
 *
 * return: result, 0 - read register successed,
 *                !0 - read register failed or the len more then max len;
 */
int arisc_rsb_read_block_data(struct arisc_rsb_block_cfg *cfg);

/**
 * rsb write block data.
 * @cfg:    point of arisc_rsb_block_cfg struct;
 *
 * return: result, 0 - write register successed,
 *                !0 - write register failedor the len more then max len;
 */
int arisc_rsb_write_block_data(struct arisc_rsb_block_cfg *cfg);

/**
 * rsb bits operation sync.
 * @cfg:    point of arisc_rsb_bits_cfg struct;
 *
 * return: result, 0 - bits operation successed,
 *                !0 - bits operation failed, or the len more then max len;
 *
 * rsb clear bits internal:
 * data = rsb_read(regaddr);
 * data = data & (~mask);
 * rsb_write(regaddr, data);
 *
 * rsb set bits internal:
 * data = rsb_read(addr);
 * data = data | mask;
 * rsb_write(addr, data);
 *
 */
int rsb_bits_ops_sync(struct arisc_rsb_bits_cfg *cfg);

/**
 * rsb set interface mode.
 * @devaddr:  rsb slave device address;
 * @regaddr:  register address of rsb slave device;
 * @data:     data which to init rsb slave device interface mode;
 *
 * return: result, 0 - set interface mode successed,
 *                !0 - set interface mode failed;
 */
int arisc_rsb_set_interface_mode(u32 devaddr, u32 regaddr, u32 data);

/**
 * rsb set runtime slave address.
 * @devaddr:  rsb slave device address;
 * @rtsaddr:  rsb slave device's runtime slave address;
 *
 * return: result, 0 - set rsb runtime address successed,
 *                !0 - set rsb runtime address failed;
 */
int arisc_rsb_set_rtsaddr(u32 devaddr, u32 rtsaddr);
#endif

/**
 * set pmu voltage.
 * @type:     pmu regulator type;
 * @voltage:  pmu regulator voltage;
 *
 * return: result, 0 - set pmu voltage successed,
 *                !0 - set pmu voltage failed;
 */
int arisc_pmu_set_voltage(u32 type, u32 voltage);

/**
 * get pmu voltage.
 * @type:     pmu regulator type;
 *
 * return: pmu regulator voltage;
 */
unsigned int arisc_pmu_get_voltage(u32 type);

/* ====================================debug interface==================================== */
int arisc_message_loopback(void);
int arisc_config_ir_paras(u32 ir_code, u32 ir_addr);

#endif	/* __ASM_ARCH_A100_H */
