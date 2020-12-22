#ifndef	_LINUX_AXP_SPLY_H_
#define	_LINUX_AXP_SPLY_H_

static 	struct input_dev * powerkeydev;

/*      AXP22      */
#define AXP22_CHARGE_STATUS		AXP22_STATUS
#define AXP22_IN_CHARGE			(1 << 6)
#define AXP22_PDBC			(0x32)
#define AXP22_CHARGE_CONTROL1		AXP22_CHARGE1
#define AXP22_CHARGER_ENABLE		(1 << 7)
#define AXP22_CHARGE_CONTROL2		AXP22_CHARGE2
#define AXP22_CHARGE_VBUS		AXP22_IPS_SET
#define AXP22_CAP			(0xB9)
#define AXP22_BATCAP0			(0xe0)
#define AXP22_BATCAP1			(0xe1)
#define AXP22_RDC0			(0xba)
#define AXP22_RDC1			(0xbb)
#define AXP22_WARNING_LEVEL		(0xe6)
#define AXP22_ADJUST_PARA		(0xe8)
#define AXP22_FAULT_LOG1		AXP22_MODE_CHGSTATUS
#define AXP22_FAULT_LOG_CHA_CUR_LOW	(1 << 2)
#define AXP22_FAULT_LOG_BATINACT	(1 << 3)
#define AXP22_FAULT_LOG_OVER_TEMP	(1 << 7)
#define AXP22_FAULT_LOG2		AXP22_INTSTS2
#define AXP22_FAULT_LOG_COLD		(1 << 0)
#define AXP22_FINISH_CHARGE		(1 << 2)
#define AXP22_COULOMB_CONTROL		AXP22_COULOMB_CTL
#define AXP22_COULOMB_ENABLE		(1 << 7)
#define AXP22_COULOMB_SUSPEND		(1 << 6)
#define AXP22_COULOMB_CLEAR		(1 << 5)

#define AXP22_ADC_CONTROL				AXP22_ADC_EN
#define AXP22_ADC_BATVOL_ENABLE				(1 << 7)
#define AXP22_ADC_BATCUR_ENABLE				(1 << 6)
#define AXP22_ADC_DCINVOL_ENABLE			(1 << 5)
#define AXP22_ADC_DCINCUR_ENABLE			(1 << 4)
#define AXP22_ADC_USBVOL_ENABLE				(1 << 3)
#define AXP22_ADC_USBCUR_ENABLE				(1 << 2)
#define AXP22_ADC_APSVOL_ENABLE				(1 << 1)
#define AXP22_ADC_TSVOL_ENABLE				(1 << 0)
#define AXP22_ADC_INTERTEM_ENABLE			(1 << 7)
#define AXP22_ADC_GPIO0_ENABLE				(1 << 3)
#define AXP22_ADC_GPIO1_ENABLE				(1 << 2)
#define AXP22_ADC_GPIO2_ENABLE				(1 << 1)
#define AXP22_ADC_GPIO3_ENABLE				(1 << 0)
#define AXP22_ADC_CONTROL3				(0x84)
#define AXP22_VBATH_RES					(0x78)
#define AXP22_VTS_RES					(0x58)
#define AXP22_VBATL_RES					(0x79)
#define AXP22_OCVBATH_RES				(0xBC)
#define AXP22_OCVBATL_RES				(0xBD)
#define AXP22_INTTEMP					(0x56)
#define AXP22_DATA_BUFFER0				AXP22_BUFFER1
#define AXP22_DATA_BUFFER1				AXP22_BUFFER2
#define AXP22_DATA_BUFFER2				AXP22_BUFFER3
#define AXP22_DATA_BUFFER3				AXP22_BUFFER4
#define AXP22_DATA_BUFFER4				AXP22_BUFFER5
#define AXP22_DATA_BUFFER5				AXP22_BUFFER6
#define AXP22_DATA_BUFFER6				AXP22_BUFFER7
#define AXP22_DATA_BUFFER7				AXP22_BUFFER8
#define AXP22_DATA_BUFFER8				AXP22_BUFFER9
#define AXP22_DATA_BUFFER9				AXP22_BUFFERA
#define AXP22_DATA_BUFFERA				AXP22_BUFFERB
#define AXP22_DATA_BUFFERB				AXP22_BUFFERC

static const uint64_t AXP22_NOTIFIER_ON = (AXP22_IRQ_USBIN | AXP22_IRQ_USBRE |
                                    AXP22_IRQ_ACIN  | AXP22_IRQ_ACRE |
                                    AXP22_IRQ_BATIN | AXP22_IRQ_BATRE |
#ifdef CONFIG_AXP809
                                    AXP22_IRQ_BATINWORK | AXP22_IRQ_BATOVWORK |
                                    AXP22_IRQ_QBATINCHG | AXP22_IRQ_BATINCHG |
                                    AXP22_IRQ_QBATOVCHG | AXP22_IRQ_BATOVCHG |
#else
                                    AXP22_IRQ_TEMLO | AXP22_IRQ_TEMOV |
#endif
					AXP22_IRQ_CHAST |  AXP22_IRQ_CHAOV | 
					(uint64_t)AXP22_IRQ_PEKFE |
					(uint64_t)AXP22_IRQ_PEKRE);


#define AXP_CHG_ATTR(_name)					\
{								\
	.attr = { .name = #_name,.mode = 0644 },		\
	.show =  _name##_show,					\
	.store = _name##_store,					\
}

struct axp_adc_res {//struct change
	uint16_t vbat_res;
	uint16_t ocvbat_res;
	uint16_t ibat_res;
	uint16_t ichar_res;
	uint16_t idischar_res;
	uint16_t vac_res;
	uint16_t iac_res;
	uint16_t vusb_res;
	uint16_t iusb_res;
	uint16_t ts_res;
};

struct axp_charger {
	/*power supply sysfs*/
	struct power_supply batt;
	struct power_supply	ac;
	struct power_supply	usb;
	struct power_supply bubatt;

	/*i2c device*/
	struct device *master;

	/* adc */
	struct axp_adc_res *adc;
	unsigned int sample_time;

	/*monitor*/
	struct delayed_work work;
	unsigned int interval;

	/*battery info*/
	struct power_supply_info *battery_info;

	/*charger control*/
	bool chgen;
	bool limit_on;
	unsigned int chgcur;
	unsigned int chgvol;
	unsigned int chgend;

	/*charger time */
	int chgpretime;
	int chgcsttime;

	/*external charger*/
	bool chgexten;
	int chgextcur;

	/* charger status */
	bool bat_det;
	bool is_on;
	bool is_finish;
	bool ac_not_enough;
	bool ac_det;
	bool usb_det;
	bool ac_valid;
	bool usb_valid;
	bool usb_adapter_valid;
	bool ext_valid;
	bool bat_current_direction;
	bool in_short;
	bool batery_active;
	bool low_charge_current;
	bool int_over_temp;
	uint8_t fault;
	int charge_on;

	int vbat;
	int ibat;
	int pbat;
	int vac;
	int iac;
	int vusb;
	int iusb;
	int ocv;
	
	int disvbat;
	int disibat;

	/*rest time*/
	int rest_vol;
	int ocv_rest_vol;
	int base_restvol;
	int rest_time;

	/*ic temperature*/
	int ic_temp;
	int bat_temp;

	/*irq*/
	struct notifier_block nb;

	/* platform callbacks for battery low and critical events */
	void (*battery_low)(void);
	void (*battery_critical)(void);

	/* timer for report ac/usb type */
	struct timer_list usb_status_timer;

	struct dentry *debug_file;
};

static struct task_struct *main_task;
static struct axp_charger *axp_charger;
static int Total_Cap = 0;
static int flag_state_change = 0;
static int Bat_Cap_Buffer[AXP22_VOL_MAX];

#endif
