/* Core header for MEMS 3-Axis Accelerometer's driver. ms
 *
 * msa_core.h - Linux kernel modules for MEMS 3-Axis Accelerometer
 *
 * Copyright (C) 2007-2016 MEMS Sensing Technology Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef __MSA_CORE_H__
#define __MSA_CORE_H__

#define CUST_VER                            ""                                          /* for Custom debug version */
#define CORE_VER                            "3.4.0_2016-07-18-15:05:30_"CUST_VER

#define MSA_SUPPORT_CHIP_LIST			MSA_NSA_NTO

#define MSA_BUFSIZE					  256

#define MSA_STK_TEMP_SOLUTION			0
#define MSA_OFFSET_TEMP_SOLUTION		 0
#if MSA_OFFSET_TEMP_SOLUTION
#define MSA_AUTO_CALIBRATE			   0
#else
#define MSA_AUTO_CALIBRATE			   0
#endif /* !MSA_OFFSET_TEMP_SOLUTION */
#if MSA_AUTO_CALIBRATE
#define MSA_SUPPORT_FAST_AUTO_CALI	   0
#else
#define MSA_SUPPORT_FAST_AUTO_CALI	   0
#endif
#define MSA_SENS_TEMP_SOLUTION		   0
#define FILTER_AVERAGE_ENHANCE			  0
#define FILTER_AVERAGE_EX				   0
#define MSA_SUPPORT_MULTI_LAYOUT		 0
#define YZ_CROSS_TALK_ENABLE				0

#define MSA_OFFSET_LEN				   9

#define MSA_OFFSET_MAX				   200
#define MSA_OFFSET_CUS				   130
#define MSA_OFFSET_SEN				   1024

typedef void *MSA_HANDLE;
typedef void *PLAT_HANDLE;


struct serial_manage_if_s {

	int						 (*read)(PLAT_HANDLE handle, unsigned char addr, unsigned char *data);
	int						 (*write)(PLAT_HANDLE handle, unsigned char addr, unsigned char data);
	int						 (*read_block)(PLAT_HANDLE handle, unsigned char base_addr, unsigned char count, unsigned char *data);
};

struct general_op_s {

	struct serial_manage_if_s   smi;

	int						 (*data_save)(unsigned char *data);
	int						 (*data_get)(unsigned char *data);
	int						 (*data_check)(void);
	int						 (*get_address)(PLAT_HANDLE handle);
	int						 (*support_fast_auto_cali)(void);

	//int						 (*myprintf)(const char *fmt, ...);
	int						 (*mysprintf)(char *buf, const char *fmt, ...);
	void					 (*msdelay)(int ms);
};

#define MSA_GENERAL_OPS_DECLARE(OPS_HDL, SMI_RD, SMI_RDBL, SMI_WR, DAT_SAVE, DAT_GET, DAT_CHECK, GET_ADDRESS, SUPPORT_FAST_AUTO_CALI, MDELAY,  MYSPRINTF)	\
		struct general_op_s	 OPS_HDL = {{SMI_RD, SMI_WR, SMI_RDBL}, DAT_SAVE, DAT_GET, DAT_CHECK, GET_ADDRESS, SUPPORT_FAST_AUTO_CALI, MYSPRINTF, MDELAY}

enum interrupt_src {

	INTERRUPT_ACTIVITY	 = 1,
	INTERRUPT_CLICK,

};

typedef enum _int_op_type {

	INTERRUPT_OP_INIT,
	INTERRUPT_OP_ENABLE,
	INTERRUPT_OP_CONFIG,
	INTERRUPT_OP_DISABLE,

} msa_int_op_type;

enum interrupt_pin {

	INTERRUPT_PIN1,
	INTERRUPT_PIN2,
};

enum pin_output_mode {

	OUTPUT_MOD_PULL_PUSH,
	OUTPUT_MOD_OD,
};

struct int_act_cfg_s {

	unsigned char		   threshold;
	unsigned char		   duration;
};

struct int_clk_cfg_s {

	unsigned char				   threshold;
	unsigned char				   click_time;	 /* click time */
	unsigned char				   quiet_time;	 /* quiet time after click */
	unsigned char				   window;		 /* for second click time window */
};

typedef union _int_src_configuration {

	struct int_act_cfg_s			act;
	struct int_clk_cfg_s			clk;

} msa_int_src_cfg_t;

typedef struct _int_configuration {

	enum interrupt_pin			  pin;
	enum interrupt_src			  int_src;

	msa_int_src_cfg_t			   int_cfg;

} msa_int_cfg_t;

typedef struct _int_init_data {

	enum pin_output_mode			pin_mod;

	unsigned char				   level;	  /* 1: high active, 0: low active */
	unsigned char				   latch;		  /* >0: latch time, 0: no latch */

} msa_int_init_t ;

typedef union _int_op_data {

	enum interrupt_src			  int_src;

	msa_int_init_t				  init;
	msa_int_cfg_t				   cfg;

} msa_int_op_data;

typedef struct _int_operations {

	msa_int_op_type				 type;
	msa_int_op_data				 data;

} msa_int_ops_t;

/* Register define for NSA asic */
#define NSA_REG_SPI_I2C				 0x00
#define NSA_REG_WHO_AM_I				0x01
#define NSA_REG_ACC_X_LSB			   0x02
#define NSA_REG_ACC_X_MSB			   0x03
#define NSA_REG_ACC_Y_LSB			   0x04
#define NSA_REG_ACC_Y_MSB			   0x05
#define NSA_REG_ACC_Z_LSB			   0x06
#define NSA_REG_ACC_Z_MSB			   0x07
#define NSA_REG_G_RANGE				 0x0f
#define NSA_REG_ODR_AXIS_DISABLE		0x10
#define NSA_REG_POWERMODE_BW			0x11
#define NSA_REG_SWAP_POLARITY		   0x12
#define NSA_REG_FIFO_CTRL			   0x14
#define NSA_REG_INTERRUPT_SETTINGS1	 0x16
#define NSA_REG_INTERRUPT_SETTINGS2	 0x17
#define NSA_REG_INTERRUPT_MAPPING1	  0x19
#define NSA_REG_INTERRUPT_MAPPING2	  0x1a
#define NSA_REG_INTERRUPT_MAPPING3	  0x1b
#define NSA_REG_INT_PIN_CONFIG		  0x20
#define NSA_REG_INT_LATCH			   0x21
#define NSA_REG_ACTIVE_DURATION		 0x27
#define NSA_REG_ACTIVE_THRESHOLD		0x28
#define NSA_REG_TAP_DURATION			0x2A
#define NSA_REG_TAP_THRESHOLD		   0x2B
#define NSA_REG_CUSTOM_OFFSET_X		 0x38
#define NSA_REG_CUSTOM_OFFSET_Y		 0x39
#define NSA_REG_CUSTOM_OFFSET_Z		 0x3a
#define NSA_REG_ENGINEERING_MODE		0x7f
#define NSA_REG_SENSITIVITY_TRIM_X	  0x80
#define NSA_REG_SENSITIVITY_TRIM_Y	  0x81
#define NSA_REG_SENSITIVITY_TRIM_Z	  0x82
#define NSA_REG_COARSE_OFFSET_TRIM_X	0x83
#define NSA_REG_COARSE_OFFSET_TRIM_Y	0x84
#define NSA_REG_COARSE_OFFSET_TRIM_Z	0x85
#define NSA_REG_FINE_OFFSET_TRIM_X	  0x86
#define NSA_REG_FINE_OFFSET_TRIM_Y	  0x87
#define NSA_REG_FINE_OFFSET_TRIM_Z	  0x88
#define NSA_REG_SENS_COMP			   0x8c
#define NSA_REG_MEMS_OPTION			 0x8f
#define NSA_REG_CHIP_INFO			   0xc0
#define NSA_REG_CHIP_INFO_SECOND		0xc1
#define NSA_REG_SENS_COARSE_TRIM		0xd1

#define MSA_ODR_50HZ				  0
#define MSA_ODR_100HZ				 1
#define MSA_ODR_200HZ				 2

#define MI_TAG						  "[MSA] "
enum{
	DEBUG_ERR = 1,
	DEBUG_ASSERT = 1 << 1,
	DEBUG_MSG = 1 << 2,
	DEBUG_FUNC = 1 << 3,
	DEBUG_DATA = 1 << 4,
};

/* register operation */
int msa_register_read(MSA_HANDLE handle, short reg, unsigned char *data);
int msa_register_write(MSA_HANDLE handle, short reg, unsigned char data);
int msa_register_read_continuously(MSA_HANDLE handle, short base_reg, unsigned char count, unsigned char *data);
int msa_register_mask_write(MSA_HANDLE handle, short addr, unsigned char mask, unsigned char data);

int msa_install_general_ops(struct general_op_s *ops);
/* chip init */
int msa_module_detect(PLAT_HANDLE handle);
MSA_HANDLE msa_core_init(PLAT_HANDLE handle);

/* data polling */
int msa_read_data(MSA_HANDLE handle, short *x, short *y, short *z);

/* filter configure */
#if FILTER_AVERAGE_ENHANCE
struct msa_filter_param_s{
	int filter_param_l;
	int filter_param_h;
	int filter_threhold;
};

int msa_get_filter_param(struct msa_filter_param_s *param);
int msa_set_filter_param(struct msa_filter_param_s *param);
#endif

#if MSA_STK_TEMP_SOLUTION
extern char bxstk;
extern char bystk;
extern char bzstk;
extern  int squareRoot(int val);
#endif

enum {
	GSENSOR_MOD_NSA_NTO = 0,
};

extern int  gsensor_mod;		/* Initial value */

/* CALI */
int msa_calibrate(MSA_HANDLE handle, int z_dir);

/* calibration */
#if MSA_OFFSET_TEMP_SOLUTION
enum file_check_statu {
	FILE_NO_EXIST,
	FILE_CHECKING,
	FILE_EXIST,
};
extern char bLoad;
void manual_load_cali_file(MSA_HANDLE handle);
#endif

/* Interrupt operations */
int msa_interrupt_ops(MSA_HANDLE handle, msa_int_ops_t *ops);

int cycle_read_xyz(MSA_HANDLE handle, int *x, int *y, int *z, int ncycle);

int msa_read_offset(MSA_HANDLE handle, unsigned char *offst);
int msa_write_offset(MSA_HANDLE handle, unsigned char *offset);

int msa_set_enable(MSA_HANDLE handle, char bEnable);
int msa_get_enable(MSA_HANDLE handle, char *bEnable);
int msa_get_reg_data(MSA_HANDLE handle, char *buf);
int msa_set_odr(MSA_HANDLE handle, int delay);
int msa_direction_remap(short *x, short *y, short *z, int direction);

int msa_chip_resume(MSA_HANDLE handle);
int msa_get_primary_offset(MSA_HANDLE handle, int *x, int *y, int *z);
int msa_temp_calibrate(int *x, int *y, int *z);
#endif	/* __MSA_CORE_H__ */


