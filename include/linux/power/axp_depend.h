/*
 * power dependency driver for AW-POWERS
 *
 * Copyright (C) 2014 ALLWINNERTECH.
 *  Ming Li <liming@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#ifndef __AXP_DEPEND_H__
#define __AXP_DEPEND_H__

/*
 * PMU ID:
 * */
typedef enum AW_POWER_ID
{
	AXP_19X_ID = 0,
	AXP_209_ID,
	AXP_22X_ID,
	AXP_806_ID,
	AXP_808_ID,
	AXP_809_ID,
	AXP_803_ID,
	AXP_813_ID,
	AXP_152_ID,
	AXP_DUMMY0_ID = 0x1 << 7,
} aw_pm_id_e;

typedef enum AXP19X_POWER_LDO
{
	AXP19X_DCDC1 = 1U << 0,
	AXP19X_DCDC2 = 1U << 1,
	AXP19X_DCDC3 = 1U << 2,
	AXP19X_LDO1 = 1U << 3,
	AXP19X_LDO2 = 1U << 4,
	AXP19X_LDO3 = 1U << 5,
	AXP19X_LDO4 = 1U << 6,
} axp19X_ldo_e;

typedef enum AXP15X_POWER_LDO
{
	AXP15X_DCDC1  = 1U << 0,
	AXP15X_DCDC2  = 1U << 1,
	AXP15X_DCDC3  = 1U << 2,
	AXP15X_DCDC4  = 1U << 3,
	AXP15X_LDO0   = 1U << 4,
	AXP15X_RTC    = 1U << 5,
	AXP15X_ALDO1  = 1U << 6,
	AXP15X_ALDO2  = 1U << 7,
	AXP15X_DLDO1  = 1U << 8,
	AXP15X_DLDO2  = 1U << 9,
	AXP15X_LDOIO0 = 1U << 10,
} axp15x_ldo_e;

typedef enum AXP22X_POWER_LDO
{
	AXP22X_DCDC1 = 1U << 0,
	AXP22X_DCDC2 = 1U << 1,
	AXP22X_DCDC3 = 1U << 2,
	AXP22X_DCDC4 = 1U << 3,
	AXP22X_DCDC5 = 1U << 4,
	AXP22X_ALDO1 = 1U << 5,
	AXP22X_ALDO2 = 1U << 6,
	AXP22X_ALDO3 = 1U << 7,
	AXP22X_DLDO1 = 1U << 8,
	AXP22X_DLDO2 = 1U << 9,
	AXP22X_DLDO3 = 1U << 10,
	AXP22X_DLDO4 = 1U << 11,
	AXP22X_ELDO1 = 1U << 12,
	AXP22X_ELDO2 = 1U << 13,
	AXP22X_ELDO3 = 1U << 14,
	AXP22X_DC5LDO = 1U << 15,
	AXP22X_LDOIO0 = 1U << 16,
	AXP22X_LDOIO1 = 1U << 17,
	AXP22X_DC1SW = 1U << 18,
	AXP22X_RTC   = 1U << 19,
} axp22X_ldo_e;

typedef enum AXP806_808_POWER_LDO
{
	AXP806_808_DCDC1 = 1U << 0,
	AXP806_808_DCDC2 = 1U << 1,
	AXP806_808_DCDC3 = 1U << 2,
	AXP806_808_DCDC4 = 1U << 3,
	AXP806_808_DCDC5 = 1U << 4,
	AXP806_808_ALDO1 = 1U << 5,
	AXP806_808_ALDO2 = 1U << 6,
	AXP806_808_ALDO3 = 1U << 7,
	AXP806_808_BLDO1 = 1U << 8,
	AXP806_808_BLDO2 = 1U << 9,
	AXP806_808_BLDO3 = 1U << 10,
	AXP806_808_BLDO4 = 1U << 11,
	AXP806_808_CLDO1 = 1U << 12,
	AXP806_808_CLDO2 = 1U << 13,
	AXP806_808_CLDO3 = 1U << 14,
	AXP806_808_SW0 = 1U << 15,
} axp806_808_ldo_e;

typedef enum AXP809_POWER_LDO
{
	AXP809_DCDC1 = 1U << 0,
	AXP809_DCDC2 = 1U << 1,
	AXP809_DCDC3 = 1U << 2,
	AXP809_DCDC4 = 1U << 3,
	AXP809_DCDC5 = 1U << 4,
	AXP809_ALDO1 = 1U << 5,
	AXP809_ALDO2 = 1U << 6,
	AXP809_ALDO3 = 1U << 7,
	AXP809_DLDO1 = 1U << 8,
	AXP809_DLDO2 = 1U << 9,
	AXP809_ELDO1 = 1U << 10,
	AXP809_ELDO2 = 1U << 11,
	AXP809_ELDO3 = 1U << 12,
	AXP809_DC5LDO = 1U << 13,
	AXP809_LDOIO0 = 1U << 14,
	AXP809_LDOIO1 = 1U << 15,
	AXP809_SW0 = 1U << 16,
	AXP809_DC1SW = 1U << 17,
	AXP809_RTC   = 1U << 18,
} axp809_ldo_e;

typedef enum AXP803_813_POWER_LDO
{
	AXP803_813_DCDC1 = 1U << 0,
	AXP803_813_DCDC2 = 1U << 1,
	AXP803_813_DCDC3 = 1U << 2,
	AXP803_813_DCDC4 = 1U << 3,
	AXP803_813_DCDC5 = 1U << 4,
	AXP803_813_DCDC6 = 1U << 5,
	AXP803_813_DCDC7 = 1U << 6,
	AXP803_813_ALDO1 = 1U << 7,
	AXP803_813_ALDO2 = 1U << 8,
	AXP803_813_ALDO3 = 1U << 9,
	AXP803_813_DLDO1 = 1U << 10,
	AXP803_813_DLDO2 = 1U << 11,
	AXP803_813_DLDO3 = 1U << 12,
	AXP803_813_DLDO4 = 1U << 13,
	AXP803_813_ELDO1 = 1U << 14,
	AXP803_813_ELDO2 = 1U << 15,
	AXP803_813_ELDO3 = 1U << 16,
	AXP803_813_FLDO1 = 1U << 17,
	AXP803_813_FLDO2 = 1U << 18,
	AXP803_813_LDOIO0 = 1U << 19,
	AXP803_813_LDOIO1 = 1U << 20,
	AXP803_813_DC1SW = 1U << 21,
	AXP803_813_RTC   = 1U << 22,
} axp803_813_ldo_e;

typedef enum AXP20X_POWER_LDO
{
	AXP20X_DCDC2  = 1U << 0,
	AXP20X_DCDC3  = 1U << 1,
	AXP20X_LDO2   = 1U << 2,
	AXP20X_LDO3   = 1U << 3,
	AXP20X_LDO4   = 1U << 4,
	AXP20X_LDOIO0 = 1U << 5,
	AXP20X_RTC    = 1U << 6,
} axp20x_ldo_e;

typedef enum AXPDUMMY_POWER_LDO {
	AXPDUMMY_LDO1  = 1U << 0,
	AXPDUMMY_LDO2  = 1U << 1,
	AXPDUMMY_LDO3  = 1U << 2,
	AXPDUMMY_LDO4  = 1U << 3,
	AXPDUMMY_LDO5  = 1U << 4,
	AXPDUMMY_LDO6  = 1U << 5,
	AXPDUMMY_LDO7  = 1U << 6,
	AXPDUMMY_LDO8  = 1U << 7,
	AXPDUMMY_LDO9  = 1U << 8,
	AXPDUMMY_LDO10 = 1U << 9,
	AXPDUMMY_LDO11 = 1U << 10,
	AXPDUMMY_LDO12 = 1U << 11,
	AXPDUMMY_LDO13 = 1U << 12,
	AXPDUMMY_LDO14 = 1U << 13,
	AXPDUMMY_LDO15 = 1U << 14,
	AXPDUMMY_LDO16 = 1U << 15,
	AXPDUMMY_LDO17 = 1U << 16,
	AXPDUMMY_LDO18 = 1U << 17,
	AXPDUMMY_LDO19 = 1U << 18,
	AXPDUMMY_LDO20 = 1U << 19,
} axpdummy_ldo_e;

/* FIXME: if you modify this struct, you should
 * sync this change with cpus source,
 * by superm at 2015-05-15.
 */
enum VDD_BIT
{
	VDD_CPUA_BIT = 0,
	VDD_CPUB_BIT,
	VCC_DRAM_BIT,
	VDD_GPU_BIT,
	VDD_SYS_BIT,
	VDD_VPU_BIT,
	VDD_CPUS_BIT,
	VDD_DRAMPLL_BIT,
	VCC_ADC_BIT,
	VCC_PL_BIT,
	VCC_PM_BIT,
	VCC_IO_BIT,
	VCC_CPVDD_BIT,
	VCC_LDOIN_BIT,
	VCC_PLL_BIT,
	VCC_LPDDR_BIT,
	VDD_TEST_BIT,
	VDD_RES1_BIT,
	VDD_RES2_BIT,
#if (defined(CONFIG_ARCH_SUN8IW10) || defined(CONFIG_ARCH_SUN8IW11))
	VCC_PC_BIT,
#else
	VDD_RES3_BIT,
#endif
	VCC_MAX_INDEX,
};

typedef struct {
	unsigned int mask_bit;
	char id_name[20];
}bitmap_name_mapping_t;

#define AXP_GPIO_IRQF_TRIGGER_RISING	0x00000001
#define AXP_GPIO_IRQF_TRIGGER_FALLING	0x00000002

extern int axp_gpio_irq_request(int pmu_num, int gpio_no,
		u32 (*handler)(int, void *), void *data);
extern int axp_gpio_irq_enable(int pmu_num, int gpio_no);
extern int axp_gpio_irq_disable(int pmu_num, int gpio_no);
extern int axp_gpio_irq_set_type(int pmu_num, int gpio_no, unsigned long type);
extern int axp_gpio_irq_free(int pmu_num, int gpio_no);

extern const bitmap_name_mapping_t pwr_dm_bitmap_name_mapping[VCC_MAX_INDEX];
extern signed int pwr_dm_bitmap_name_mapping_cnt;

/* internal interface*/
void set_pwr_regu_tree(unsigned int value, unsigned int bitmap);
signed int axp_check_sys_id(const char *supply_id);
char *axp_get_sys_id(unsigned int bitmap);
#if defined(CONFIG_AW_AXP81X)
s32 get_ldo_dependence(const char *ldo_name, signed int count);
#else
s32 get_ldo_dependence(const char *ldo_name, signed int count,
				s32 (*get_dep_cb)(const char *));
#endif
void set_sys_pwr_dm_mask(unsigned int bitmap, unsigned int enable);

/* external interface*/
extern unsigned int get_sys_pwr_dm_mask(void);
extern int add_sys_pwr_dm(const char *id);
extern int del_sys_pwr_dm(const char *id);
extern int is_sys_pwr_dm_id(const char *id);
extern int is_sys_pwr_dm_active(unsigned int bitmap);
extern char *get_sys_pwr_dm_id(unsigned int bitmap);
extern void get_pwr_regu_tree(unsigned int *p);
extern ssize_t parse_pwr_dm_map(char *s, size_t size, unsigned int bitmap);
#endif /* __AXP_DEPEND_H__ */

