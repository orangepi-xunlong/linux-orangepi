#ifndef __LINUX_AXP_CFG_H_
#define __LINUX_AXP_CFG_H_
#include <mach/sys_config.h>

#define AXP22_ADDR		0x68 >> 1
#define BATRDC			100		//initial rdc
	
#define AXP22LDO1		3000

#define AXP22_VOL_MAX		1		// capability buffer length
#define AXP22_TIME_MAX		20
#define AXP22_AVER_MAX		10
#define AXP22_RDC_COUNT		10

#define ABS(x)			((x) >0 ? (x) : -(x) )

#define INTCHGCUR		300000		//set initial charging current limite
#define SUSCHGCUR		1000000		//set suspend charging current limite
#define RESCHGCUR		INTCHGCUR	//set resume charging current limite
#define CLSCHGCUR		SUSCHGCUR	//set shutdown charging current limite
#define INTCHGVOL		4200000		//set initial charing target voltage
#define INTCHGENDRATE		10		//set initial charing end current rate
#define INTCHGENABLED		1		//set initial charing enabled
#define INTADCFREQ		25		//set initial adc frequency
#define INTADCFREQC		100		//set initial coulomb adc coufrequency
#define INTCHGPRETIME		50		//set initial pre-charging time
#define INTCHGCSTTIME		480		//set initial pre-charging time
#define BATMAXVOL		4200000		//set battery max design volatge
#define BATMINVOL		3500000		//set battery min design volatge

#define OCVREG0			0x00		//2.99V
#define OCVREG1			0x00		//3.13V
#define OCVREG2			0x00		//3.27V
#define OCVREG3			0x00		//3.34V
#define OCVREG4			0x00		//3.41V
#define OCVREG5			0x00		//3.48V
#define OCVREG6			0x00		//3.52V
#define OCVREG7			0x00		//3.55V
#define OCVREG8			0x04		//3.57V
#define OCVREG9			0x05		//3.59V
#define OCVREGA			0x06		//3.61V
#define OCVREGB			0x07		//3.63V
#define OCVREGC			0x0a		//3.64V
#define OCVREGD			0x0d		//3.66V
#define OCVREGE			0x1a		//3.7V 
#define OCVREGF			0x24		//3.73V
#define OCVREG10		0x29		//3.77V
#define OCVREG11		0x2e		//3.78V
#define OCVREG12		0x32		//3.8V 
#define OCVREG13		0x35		//3.84V
#define OCVREG14		0x39		//3.85V
#define OCVREG15		0x3d		//3.87V
#define OCVREG16		0x43		//3.91V
#define OCVREG17		0x49		//3.94V
#define OCVREG18		0x4f		//3.98V
#define OCVREG19		0x54		//4.01V
#define OCVREG1A		0x58		//4.05V
#define OCVREG1B		0x5c		//4.08V
#define OCVREG1C		0x5e		//4.1V 
#define OCVREG1D		0x60		//4.12V
#define OCVREG1E		0x62		//4.14V
#define OCVREG1F		0x64		//4.15V

struct axp_config_info{
	int	pmu_used;
	int	pmu_twi_addr;
	int	pmu_twi_id;
	int	pmu_irq_id;
	int	pmu_battery_rdc;
	int	pmu_battery_cap;
	int	pmu_batdeten;
	int	pmu_chg_temp_en;
	int	pmu_runtime_chg_temp;
	int	pmu_earlysuspend_chg_temp;
	int	pmu_suspend_chg_temp;
	int	pmu_shutdown_chg_temp;
	int	pmu_runtime_chgcur;
	int	pmu_earlysuspend_chgcur;
	int	pmu_suspend_chgcur;
	int	pmu_shutdown_chgcur;
	int	pmu_init_chgvol;
	int	pmu_init_chgend_rate;
	int	pmu_init_chg_enabled;
	int	pmu_init_bc_en;
	int	pmu_init_adc_freq;
	int	pmu_init_adcts_freq;
	int	pmu_init_chg_pretime;
	int	pmu_init_chg_csttime;
	int	pmu_batt_cap_correct;
	int	pmu_bat_regu_en;

	int	pmu_bat_para1;
	int	pmu_bat_para2;
	int	pmu_bat_para3;
	int	pmu_bat_para4;
	int	pmu_bat_para5;
	int	pmu_bat_para6;
	int	pmu_bat_para7;
	int	pmu_bat_para8;
	int	pmu_bat_para9;
	int	pmu_bat_para10;
	int	pmu_bat_para11;
	int	pmu_bat_para12;
	int	pmu_bat_para13;
	int	pmu_bat_para14;
	int	pmu_bat_para15;
	int	pmu_bat_para16;
	int	pmu_bat_para17;
	int	pmu_bat_para18;
	int	pmu_bat_para19;
	int	pmu_bat_para20;
	int	pmu_bat_para21;
	int	pmu_bat_para22;
	int	pmu_bat_para23;
	int	pmu_bat_para24;
	int	pmu_bat_para25;
	int	pmu_bat_para26;
	int	pmu_bat_para27;
	int	pmu_bat_para28;
	int	pmu_bat_para29;
	int	pmu_bat_para30;
	int	pmu_bat_para31;
	int	pmu_bat_para32;

	int	pmu_usbvol_limit;
	int	pmu_usbcur_limit;
	int	pmu_usbvol;
	int	pmu_usbcur;
	int	pmu_usbvol_pc;
	int	pmu_usbcur_pc;
	int	pmu_pwroff_vol;
	int	pmu_pwron_vol;
	int	pmu_pekoff_time;
	int	pmu_pekoff_en;
	int     pmu_pekoff_delay_time;
	int	pmu_pekoff_func;
	int	pmu_peklong_time;
	int	pmu_pekon_time;
	int	pmu_pwrok_time;
	int     pmu_pwrok_shutdown_en;
	int     pmu_reset_shutdown_en;
	int	pmu_battery_warning_level1;
	int	pmu_battery_warning_level2;
	int	pmu_restvol_adjust_time;
	int	pmu_ocv_cou_adjust_time;
	int	pmu_chgled_func;
	int	pmu_chgled_type;
	int	pmu_vbusen_func;
	int	pmu_reset;
	int	pmu_IRQ_wakeup;
	int	pmu_hot_shutdowm;
	int	pmu_inshort;
	int	power_start;

	int	pmu_temp_enable;
	int	pmu_charge_ltf;
	int	pmu_charge_htf;
	int	pmu_discharge_ltf;
	int	pmu_discharge_htf;
	int	pmu_temp_para1;
	int	pmu_temp_para2;
	int	pmu_temp_para3;
	int	pmu_temp_para4;
	int	pmu_temp_para5;
	int	pmu_temp_para6;
	int	pmu_temp_para7;
	int	pmu_temp_para8;
	int	pmu_temp_para9;
	int	pmu_temp_para10;
	int	pmu_temp_para11;
	int	pmu_temp_para12;
	int	pmu_temp_para13;
	int	pmu_temp_para14;
	int	pmu_temp_para15;
	int	pmu_temp_para16;
};

enum {
	DEBUG_SPLY = 1U << 0,
	DEBUG_REGU = 1U << 1,
	DEBUG_INT = 1U << 2,
	DEBUG_CHG = 1U << 3,
};

#define AXP_LDOIO_ID_START      30
#define AXP_DCDC_ID_START       40

extern char pmu_type[20];
#ifdef CONFIG_AW_AXP81X
extern int axp_debug;
#define DBG_PSY_MSG(level_mask, fmt, arg...)	if (unlikely(axp_debug & level_mask)) \
	printk(KERN_DEBUG fmt , ## arg)
extern void axp81x_power_off(void);
#elif defined CONFIG_AW_AXP19
extern int axp_debug;
#define DBG_PSY_MSG(level_mask, fmt, arg...)	if (unlikely(axp_debug & level_mask)) \
	printk(KERN_DEBUG fmt , ## arg)
extern void axp19_power_off(int power_start);

#elif defined CONFIG_AW_AXP20

struct axp20_config_info{
	int pmu_used;
	int pmu_twi_id;
	int pmu_irq_id;
	int pmu_twi_addr;
	int pmu_battery_rdc;
	int pmu_battery_cap;
	int pmu_init_chgcur;
	int pmu_suspend_chgcur;
	int pmu_runtime_chgcur;
	int pmu_shutdown_chgcur;
	int pmu_init_chgvol;
	int pmu_init_chgend_rate;
	int pmu_init_chg_enabled;
	int pmu_init_adc_freq;
	int pmu_init_adc_freqc;
	int pmu_init_chg_pretime;
	int pmu_init_chg_csttime;

	int pmu_bat_para1;
	int pmu_bat_para2;
	int pmu_bat_para3;
	int pmu_bat_para4;
	int pmu_bat_para5;
	int pmu_bat_para6;
	int pmu_bat_para7;
	int pmu_bat_para8;
	int pmu_bat_para9;
	int pmu_bat_para10;
	int pmu_bat_para11;
	int pmu_bat_para12;
	int pmu_bat_para13;
	int pmu_bat_para14;
	int pmu_bat_para15;
	int pmu_bat_para16;

	int pmu_usbvol_limit;
	int pmu_usbvol;
	int pmu_usbcur_limit;
	int pmu_usbcur;

	int pmu_pwroff_vol;
	int pmu_pwron_vol;

	int pmu_pekoff_time;
	int pmu_pekoff_en;
	int pmu_peklong_time;
	int pmu_pekon_time;
	int pmu_pwrok_time;
	int pmu_pwrnoe_time;
	int pmu_hot_shutdown;
	int power_start;
	int pmu_irq_io_id;
	struct gpio_config pmu_irq_io;
};

#define AXP20_VOL_MAX			12 // capability buffer length
#define AXP20_TIME_MAX		20
#define AXP20_AVER_MAX		10
#define AXP20_RDC_COUNT		10

enum {
	AXP20_NOT_SUSPEND = 1U << 0,
	AXP20_AS_SUSPEND = 1U << 1,
	AXP20_SUSPEND_WITH_IRQ = 1U << 2,
};


extern int axp_debug;
#define DBG_PSY_MSG(level_mask, fmt, arg...)	if (unlikely(axp_debug & level_mask)) \
	printk(KERN_DEBUG fmt , ## arg)
extern void axp20_power_off(int power_start);
extern int axp20_fetch_sysconfig_para(char * pmu_type, struct axp20_config_info *axp_config);


#endif

extern struct axp_config_info axp22_config;
extern struct axp_config_info axp15_config;

extern int axp_fetch_sysconfig_para(char * pmu_type, struct axp_config_info *axp_config);
#endif
