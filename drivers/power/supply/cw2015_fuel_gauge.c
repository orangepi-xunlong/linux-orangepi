#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/platform_device.h>
#include <linux/power_supply.h>
#include <linux/workqueue.h>
#include <linux/i2c.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/time.h>
#include <linux/slab.h>
#include <linux/version.h>


#define CWFG_ENABLE_LOG 1 //CHANGE   Customer need to change this for enable/disable log
#define CWFG_I2C_BUSNUM 2 //CHANGE   Customer need to change this number according to the principle of hardware
#define DOUBLE_SERIES_BATTERY 0
/*
#define USB_CHARGING_FILE "/sys/class/power_supply/usb/online" // Chaman
#define DC_CHARGING_FILE "/sys/class/power_supply/ac/online"
*/
#define CW_PROPERTIES "cw-bat"

#define REG_VERSION             0x0
#define REG_VCELL               0x2
#define REG_SOC                 0x4
#define REG_RRT_ALERT           0x6
#define REG_CONFIG              0x8
#define REG_MODE                0xA
#define REG_VTEMPL              0xC
#define REG_VTEMPH              0xD
#define REG_BATINFO             0x10


#define MODE_SLEEP_MASK         (0x3<<6)
#define MODE_SLEEP              (0x3<<6)
#define MODE_NORMAL             (0x0<<6)
#define MODE_QUICK_START        (0x3<<4)
#define MODE_RESTART            (0xf<<0)

#define CONFIG_UPDATE_FLG       (0x1<<1)
#define ATHD                    (0x0<<3)        // ATHD = 0%
#define MASK_ATHD		GENMASK(7, 3)
#define MASK_SOC		GENMASK(12, 0)

#define queue_delayed_work_time  8000
#define BATTERY_CAPACITY_ERROR  40*1000
#define BATTERY_CHARGING_ZERO   1800*1000

#define UI_FULL 100
#define DECIMAL_MAX 80
#define DECIMAL_MIN 20

#define CHARGING_ON 1
#define NO_CHARGING 0


#define cw_printk(fmt, arg...)        \
	({                                    \
		if(CWFG_ENABLE_LOG){              \
			pr_debug("FG_CW2015 : %s : " fmt, __FUNCTION__ ,##arg);  \
		}else{}                           \
	})     //need check by Chaman


#define CWFG_NAME "cw2015"
#define SIZE_BATINFO    64

static unsigned char config_info[SIZE_BATINFO] = {};

static struct power_supply *chrg_usb_psy;
static struct power_supply *chrg_ac_psy;

struct cw_battery {
	struct i2c_client *client;

	struct workqueue_struct *cwfg_workqueue;
	struct delayed_work battery_delay_work;

#if LINUX_VERSION_CODE < KERNEL_VERSION(4, 1, 0)
	struct power_supply cw_bat;
#else
	struct power_supply *cw_bat;
#endif

	int charger_mode;
	int capacity;
	int voltage;
	int status;
	int change;
	int raw_soc;
	int time_to_empty;
};

int g_cw2015_capacity = 0;
int g_cw2015_vol = 0;

/*Define CW2015 iic read function*/
static int cw_read(struct i2c_client *client, unsigned char reg, unsigned char buf[])
{
	int ret = 0;
	ret = i2c_smbus_read_i2c_block_data( client, reg, 1, buf);
	return ret;
}
/*Define CW2015 iic write function*/
static int cw_write(struct i2c_client *client, unsigned char reg, unsigned char const buf[])
{
	int ret = 0;
	ret = i2c_smbus_write_i2c_block_data( client, reg, 1, &buf[0] );
	return ret;
}
/*Define CW2015 iic read word function*/
static int cw_read_word(struct i2c_client *client, unsigned char reg, unsigned char buf[])
{
	int ret = 0;
	ret = i2c_smbus_read_i2c_block_data( client, reg, 2, buf );
	return ret;
}

/*CW2015 update profile function, Often called during initialization*/
int cw_update_config_info(struct cw_battery *cw_bat)
{
	int ret;
	unsigned char reg_val;
	int i;
	unsigned char reset_val;

	cw_printk("\n");
	cw_printk("[FGADC] test config_info = 0x%x\n",config_info[0]);
	printk(KERN_INFO "%s\n", __func__);

	// make sure no in sleep mode
	ret = cw_read(cw_bat->client, REG_MODE, &reg_val);
	if(ret < 0) {
		return ret;
	}

	reset_val = reg_val;
	if((reg_val & MODE_SLEEP_MASK) == MODE_SLEEP) {
		return -1;
	}

	// update new battery info
	for (i = 0; i < SIZE_BATINFO; i++) {
		//printk(KERN_INFO "%X\n", config_info[i]);
		ret = cw_write(cw_bat->client, REG_BATINFO + i, &config_info[i]);
		if(ret < 0)
			return ret;
	}

	reg_val = 0x00;
	reg_val |= CONFIG_UPDATE_FLG;   // set UPDATE_FLAG
	reg_val &= 0x07;                // clear ATHD
	reg_val |= ATHD;                // set ATHD
	ret = cw_write(cw_bat->client, REG_CONFIG, &reg_val);
	if(ret < 0)
		return ret;

	msleep(50);
	// reset
	reg_val = 0x00;
	reset_val &= ~(MODE_RESTART);
	reg_val = reset_val | MODE_RESTART;
	ret = cw_write(cw_bat->client, REG_MODE, &reg_val);
	if(ret < 0) return ret;

	msleep(10);

	ret = cw_write(cw_bat->client, REG_MODE, &reset_val);
	if(ret < 0) return ret;

	msleep(100);
	cw_printk("cw2015 update config success!\n");

	return 0;
}

/*CW2015 init function, Often called during initialization*/
static int cw_init(struct cw_battery *cw_bat)
{
	int ret;
	int i;
	unsigned char reg_val = MODE_SLEEP;

	if ((reg_val & MODE_SLEEP_MASK) == MODE_SLEEP) {
		reg_val = MODE_NORMAL;
		ret = cw_write(cw_bat->client, REG_MODE, &reg_val);
		if (ret < 0)
			return ret;
	}

	ret = cw_read(cw_bat->client, REG_CONFIG, &reg_val);
	if (ret < 0)
		return ret;

	if ((reg_val & 0xf8) != ATHD) {
		reg_val &= 0x07;    /* clear ATHD */
		reg_val |= ATHD;    /* set ATHD */
		ret = cw_write(cw_bat->client, REG_CONFIG, &reg_val);
		if (ret < 0)
			return ret;
	}

	ret = cw_read(cw_bat->client, REG_CONFIG, &reg_val);
	if (ret < 0)
		return ret;

	if (!(reg_val & CONFIG_UPDATE_FLG)) {
		cw_printk("update config flg is true, need update config\n");
		ret = cw_update_config_info(cw_bat);
		if (ret < 0) {
			printk("%s : update config fail\n", __func__);
			return ret;
		}
	} else {
		for(i = 0; i < SIZE_BATINFO; i++) {
			ret = cw_read(cw_bat->client, (REG_BATINFO + i), &reg_val);
			if (ret < 0)
			return ret;

			cw_printk(KERN_INFO "%X\n", reg_val);
			if (config_info[i] != reg_val)
				break;
		}
		if (i != SIZE_BATINFO) {
			reg_val = MODE_SLEEP;
			ret = cw_write(cw_bat->client, REG_MODE, &reg_val);
			if (ret < 0)
				return ret;

			msleep(30);
			reg_val = MODE_NORMAL;
			ret = cw_write(cw_bat->client, REG_MODE, &reg_val);
			if (ret < 0)
				return ret;

			cw_printk("config didn't match, need update config\n");
			ret = cw_update_config_info(cw_bat);
			if (ret < 0){
				return ret;
			}
		}
	}

	msleep(10);
	for (i = 0; i < 30; i++) {
		ret = cw_read(cw_bat->client, REG_SOC, &reg_val);
		if (ret < 0)
			return ret;
		else if (reg_val <= 0x64)
			break;
		msleep(120);
	}

	if (i >= 30 ){
		reg_val = MODE_SLEEP;
		ret = cw_write(cw_bat->client, REG_MODE, &reg_val);
		cw_printk("cw2015 input unvalid power error, cw2015 join sleep mode\n");
		return -1;
	}

	cw_printk("cw2015 init success!\n");
	return 0;
}

/*Functions:< check_chrg_usb_psy check_chrg_ac_psy get_chrg_psy get_charge_state > for Get Charger Status from outside*/
static int check_chrg_usb_psy(struct device *dev, void *data)
{
    struct power_supply *psy = dev_get_drvdata(dev);

#if LINUX_VERSION_CODE < KERNEL_VERSION(4, 1, 0)
	if (psy->type == POWER_SUPPLY_TYPE_USB) {
#else
	if (psy->desc->type == POWER_SUPPLY_TYPE_USB) {
#endif
		chrg_usb_psy = psy;
		return 1;
	}
	return 0;
}

static int check_chrg_ac_psy(struct device *dev, void *data)
{
	struct power_supply *psy = dev_get_drvdata(dev);

#if LINUX_VERSION_CODE < KERNEL_VERSION(4, 1, 0)
	if (psy->type == POWER_SUPPLY_TYPE_MAINS) {
#else
	if (psy->desc->type == POWER_SUPPLY_TYPE_MAINS) {
#endif
		chrg_ac_psy = psy;
		return 1;
	}
	return 0;
}

static void get_chrg_psy(void)
{
	if(!chrg_usb_psy)
		class_for_each_device(power_supply_class, NULL, NULL, check_chrg_usb_psy);
	if(!chrg_ac_psy)
		class_for_each_device(power_supply_class, NULL, NULL, check_chrg_ac_psy);
}

static int get_charge_state(void)
{
	union power_supply_propval val;
	int ret = -ENODEV;
	int usb_online = 0;
	int ac_online = 0;

	if (!chrg_usb_psy || !chrg_ac_psy)
		get_chrg_psy();

	if(chrg_usb_psy) {
#if LINUX_VERSION_CODE < KERNEL_VERSION(4, 1, 0)
		ret = chrg_usb_psy->get_property(chrg_usb_psy, POWER_SUPPLY_PROP_ONLINE, &val);
#else
		ret = chrg_usb_psy->desc->get_property(chrg_usb_psy, POWER_SUPPLY_PROP_ONLINE, &val);
#endif
		if (!ret)
			usb_online = val.intval;
	}
	if(chrg_ac_psy) {
#if LINUX_VERSION_CODE < KERNEL_VERSION(4, 1, 0)
		ret = chrg_ac_psy->get_property(chrg_ac_psy, POWER_SUPPLY_PROP_ONLINE, &val);
#else
		ret = chrg_ac_psy->desc->get_property(chrg_ac_psy, POWER_SUPPLY_PROP_ONLINE, &val);
#endif
		if (!ret)
			ac_online = val.intval;
	}
	if(!chrg_usb_psy){
		cw_printk("Usb online didn't find\n");
	}
	if(!chrg_ac_psy){
		cw_printk("Ac online didn't find\n");
	}
	cw_printk("ac_online = %d    usb_online = %d\n", ac_online, usb_online);
	if(ac_online || usb_online){
		return 1;
	}
	return 0;
}


static int cw_por(struct cw_battery *cw_bat)
{
	int ret;
	unsigned char reset_val;

	reset_val = MODE_SLEEP;
	ret = cw_write(cw_bat->client, REG_MODE, &reset_val);
	if (ret < 0)
		return ret;
	reset_val = MODE_NORMAL;
	msleep(30);
	ret = cw_write(cw_bat->client, REG_MODE, &reset_val);
	if (ret < 0)
		return ret;
	ret = cw_init(cw_bat);
	if (ret)
		return ret;
	return 0;
}



static int cw_get_capacity(struct cw_battery *cw_bat)
{
	int ui_100 = UI_FULL;
	int remainder = 0;
	int real_SOC = 0;
	int digit_SOC = 0;
	int UI_SOC = 0;
	//int cw_capacity;
	int ret;
	unsigned char reg_val[2];
	//unsigned char temp_value;
	static int reset_loop = 0;
	static int charging_zero_loop = 0;

	ret = cw_read_word(cw_bat->client, REG_SOC, reg_val);
	if (ret < 0)
		return ret;

	real_SOC = reg_val[0];
	digit_SOC = reg_val[1];
	cw_bat->raw_soc = real_SOC;
	cw_printk(KERN_INFO "CW2015[%d]: real_SOC = %d, digit_SOC = %d\n", __LINE__, real_SOC, digit_SOC);

	/*case 1 : avoid IC error, read SOC > 100*/
	if ((real_SOC < 0) || (real_SOC > 100)) {
		cw_printk("Error:  real_SOC = %d\n", real_SOC);
		reset_loop++;
		if (reset_loop > (BATTERY_CAPACITY_ERROR / queue_delayed_work_time)){
			cw_por(cw_bat);
			reset_loop =0;
		}

		return cw_bat->capacity; //cw_capacity Chaman change because I think customer didn't want to get error capacity.
	}else {
		reset_loop =0;
	}

	/*case 2 : avoid IC error, battery SOC is 0% when long time charging*/
	if((cw_bat->charger_mode > 0) &&(real_SOC == 0))
	{
		charging_zero_loop++;
		if (charging_zero_loop > BATTERY_CHARGING_ZERO / queue_delayed_work_time) {
			cw_por(cw_bat);
			charging_zero_loop = 0;
		}
	}else if(charging_zero_loop != 0){
		charging_zero_loop = 0;
	}


	UI_SOC = ((real_SOC * 256 + digit_SOC) * 100)/ (ui_100*256);
	remainder = (((real_SOC * 256 + digit_SOC) * 100 * 100) / (ui_100*256)) % 100;
	cw_printk(KERN_INFO "CW2015[%d]: ui_100 = %d, UI_SOC = %d, remainder = %d\n", __LINE__, ui_100, UI_SOC, remainder);
	/*case 3 : aviod swing*/
	if(UI_SOC >= 100){
		cw_printk(KERN_INFO "CW2015[%d]: UI_SOC = %d larger 100!!!!\n", __LINE__, UI_SOC);
		UI_SOC = 100;
	}else{
		if((remainder > 70 || remainder < 30) && (UI_SOC >= (cw_bat->capacity - 1)) && (UI_SOC <= (cw_bat->capacity + 1)))
		{
			UI_SOC = cw_bat->capacity;
			cw_printk(KERN_INFO "CW2015[%d]: UI_SOC = %d, cw_bat->capacity = %d\n", __LINE__, UI_SOC, cw_bat->capacity);
		}
	}

	return UI_SOC;
}

/*This function called when get voltage from cw2015*/
static int cw_get_voltage(struct cw_battery *cw_bat)
{
	int ret;
	unsigned char reg_val[2];
	u16 value16, value16_1, value16_2, value16_3;
	int voltage;

	ret = cw_read_word(cw_bat->client, REG_VCELL, reg_val);
	if(ret < 0) {
		return ret;
	}
	value16 = (reg_val[0] << 8) + reg_val[1];

	ret = cw_read_word(cw_bat->client, REG_VCELL, reg_val);
	if(ret < 0) {
		return ret;
	}
	value16_1 = (reg_val[0] << 8) + reg_val[1];

	ret = cw_read_word(cw_bat->client, REG_VCELL, reg_val);
	if(ret < 0) {
		return ret;
	}
	value16_2 = (reg_val[0] << 8) + reg_val[1];

	if(value16 > value16_1) {
		value16_3 = value16;
		value16 = value16_1;
		value16_1 = value16_3;
	}

	if(value16_1 > value16_2) {
		value16_3 =value16_1;
		value16_1 =value16_2;
		value16_2 =value16_3;
	}

	if(value16 >value16_1) {
		value16_3 =value16;
		value16 =value16_1;
		value16_1 =value16_3;
	}

	voltage = value16_1 * 625 / 2048;

	if(DOUBLE_SERIES_BATTERY)
		voltage = voltage * 2;
	return voltage;
}

static int cw_get_time_to_empty(struct cw_battery *cw_bat)
{
	int ret;
	unsigned char reg_val[2];
	u16 value16;

	ret = cw_read_word(cw_bat->client, REG_RRT_ALERT, reg_val);
	if (ret)
		return ret;
	value16 = (reg_val[0] << 8) + reg_val[1];

	return value16 & MASK_SOC;
}

static void cw_update_charge_status(struct cw_battery *cw_bat)
{
	int cw_charger_mode;
	cw_charger_mode = get_charge_state();
	if(cw_bat->charger_mode != cw_charger_mode){
		cw_bat->charger_mode = cw_charger_mode;
		cw_bat->change = 1;
	}
}


static void cw_update_capacity(struct cw_battery *cw_bat)
{
	int cw_capacity;
	cw_capacity = cw_get_capacity(cw_bat);

	if ((cw_capacity >= 0) && (cw_capacity <= 100) && (cw_bat->capacity != cw_capacity)) {
		cw_bat->capacity = cw_capacity;
		cw_bat->change = 1;
	}
}

static void cw_update_vol(struct cw_battery *cw_bat)
{
	int ret;
	ret = cw_get_voltage(cw_bat);
	if ((ret >= 0) && (cw_bat->voltage != ret)) {
		cw_bat->voltage = ret;
		cw_bat->change = 1;
	}
}

static void cw_update_status(struct cw_battery *cw_bat)
{
	int status;

	if (cw_bat->charger_mode > 0) {
		if (cw_bat->capacity >= 100)
			status = POWER_SUPPLY_STATUS_FULL;
		else
			status = POWER_SUPPLY_STATUS_CHARGING;
	} else {
		status = POWER_SUPPLY_STATUS_DISCHARGING;
	}

	if (cw_bat->status != status) {
		cw_bat->status = status;
		cw_bat->change = 1;
	}
}

static void cw_update_time_to_empty(struct cw_battery *cw_bat)
{
	int time_to_empty;

	time_to_empty = cw_get_time_to_empty(cw_bat);
	if (time_to_empty < 0)
		cw_printk("Failed to get time to empty from gauge: %d\n",
			time_to_empty);
	else if (cw_bat->time_to_empty != time_to_empty) {
		cw_bat->time_to_empty = time_to_empty;
		cw_bat->change = 1;
	}
}

static void cw_bat_work(struct work_struct *work)
{
	struct delayed_work *delay_work;
	struct cw_battery *cw_bat;
	/*Add for battery swap start*/
	int ret;
	unsigned char reg_val;
	//u16 value16;
	int i = 0;
	/*Add for battery swap end*/

	delay_work = container_of(work, struct delayed_work, work);
	cw_bat = container_of(delay_work, struct cw_battery, battery_delay_work);

	/*Add for battery swap start*/
	ret = cw_read(cw_bat->client, REG_MODE, &reg_val);
	if(ret < 0){
		//battery is out , you can send new battery capacity vol here what you want set
		//for example
		cw_bat->capacity = 100;
		cw_bat->voltage = 4200;
		cw_bat->change = 1;
	} else {
		if((reg_val & MODE_SLEEP_MASK) == MODE_SLEEP){
			for(i = 0; i < 5; i++){
				if(cw_por(cw_bat) == 0)
					break;
			}
		}
		cw_update_charge_status(cw_bat);
		cw_update_vol(cw_bat);
		cw_update_capacity(cw_bat);
		cw_update_status(cw_bat);
		cw_update_time_to_empty(cw_bat);
	}
	/*Add for battery swap end*/
	cw_printk("charger_mod = %d, capacity = %d, voltage = %d raw_soc = %d\n", cw_bat->charger_mode, cw_bat->capacity,
	          cw_bat->voltage, cw_bat->raw_soc);

	#ifdef CW_PROPERTIES
	if (cw_bat->change == 1){
#if LINUX_VERSION_CODE < KERNEL_VERSION(4, 1, 0)
		power_supply_changed(&cw_bat->cw_bat);
#else
		power_supply_changed(cw_bat->cw_bat);
#endif
		cw_bat->change = 0;
	}
	#endif
	g_cw2015_capacity = cw_bat->capacity;
	g_cw2015_vol = cw_bat->voltage;

	queue_delayed_work(cw_bat->cwfg_workqueue, &cw_bat->battery_delay_work, msecs_to_jiffies(queue_delayed_work_time));
}

static bool cw_battery_valid_time_to_empty(struct cw_battery *cw_bat)
{
	return	cw_bat->time_to_empty > 0 &&
		cw_bat->time_to_empty < MASK_SOC &&
		cw_bat->status == POWER_SUPPLY_STATUS_DISCHARGING;
}

#ifdef CW_PROPERTIES
static int cw_battery_get_property(struct power_supply *psy,
                enum power_supply_property psp,
                union power_supply_propval *val)
{
	int ret = 0;

#if LINUX_VERSION_CODE < KERNEL_VERSION(4, 1, 0)
	struct cw_battery *cw_bat;
	cw_bat = container_of(psy, struct cw_battery, cw_bat);
#else
	struct cw_battery *cw_bat = power_supply_get_drvdata(psy);
#endif

	switch (psp) {
	case POWER_SUPPLY_PROP_CAPACITY:
		val->intval = cw_bat->capacity;
		break;

	case POWER_SUPPLY_PROP_STATUS:   //Chaman charger ic will give a real value
		val->intval = cw_bat->status;
		break;
	/*
	case POWER_SUPPLY_PROP_HEALTH:   //Chaman charger ic will give a real value
		val->intval= POWER_SUPPLY_HEALTH_GOOD;
		break;
	*/
	case POWER_SUPPLY_PROP_PRESENT:
		val->intval = cw_bat->voltage <= 0 ? 0 : 1;
		break;

	case POWER_SUPPLY_PROP_VOLTAGE_NOW:
		val->intval = cw_bat->voltage * 1000;
		break;

	case POWER_SUPPLY_PROP_TIME_TO_EMPTY_NOW:
		if (cw_battery_valid_time_to_empty(cw_bat))
			val->intval = cw_bat->time_to_empty;
		else
			val->intval = 0;
		break;

	case POWER_SUPPLY_PROP_TECHNOLOGY:  //Chaman this value no need
		val->intval = POWER_SUPPLY_TECHNOLOGY_LION;
		break;

	default:
		ret = -EINVAL;
		break;
	}
	return ret;
}

static enum power_supply_property cw_battery_properties[] = {
	POWER_SUPPLY_PROP_CAPACITY,
	POWER_SUPPLY_PROP_STATUS,
	//POWER_SUPPLY_PROP_HEALTH,
	POWER_SUPPLY_PROP_PRESENT,
	POWER_SUPPLY_PROP_VOLTAGE_NOW,
	POWER_SUPPLY_PROP_TIME_TO_EMPTY_NOW,
	POWER_SUPPLY_PROP_TECHNOLOGY,
};
#endif

static int cw_init_data(struct cw_battery *cw_bat)
{
	unsigned char reg_SOC[2];
	int real_SOC = 0;
	int digit_SOC = 0;
	int ret;
	int UI_SOC = 0;

	ret = cw_read_word(cw_bat->client, REG_SOC, reg_SOC);
	if (ret < 0)
		return ret;

	real_SOC = reg_SOC[0];
	digit_SOC = reg_SOC[1];
	UI_SOC = ((real_SOC * 256 + digit_SOC) * 100)/ (UI_FULL*256);
	cw_printk("[%d]: real_SOC = %d, digit_SOC = %d\n", __LINE__, real_SOC, digit_SOC);

	if(UI_SOC >= 100){
		UI_SOC = 100;
	}

	cw_update_vol(cw_bat);
	cw_update_charge_status(cw_bat);
	cw_bat->capacity = UI_SOC;
	cw_update_status(cw_bat);
	return 0;
}

static int cw2015_probe(struct i2c_client *client)
{
	int ret;
	int loop = 0;
	int length;
	struct cw_battery *cw_bat;

#ifdef CW_PROPERTIES
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 1, 0)
	struct power_supply_desc *psy_desc;
	struct power_supply_config psy_cfg = {0};
#endif
#endif
	cw_bat = devm_kzalloc(&client->dev, sizeof(*cw_bat), GFP_KERNEL);
	if (!cw_bat) {
		cw_printk("cw_bat create fail!\n");
		return -ENOMEM;
	}

	i2c_set_clientdata(client, cw_bat);

	cw_bat->client = client;
	cw_bat->capacity = 1;
	cw_bat->voltage = 0;
	cw_bat->status = 0;
	cw_bat->charger_mode = NO_CHARGING;
	cw_bat->change = 0;

	length = device_property_count_u8(&client->dev, "cellwise,battery-profile");
	if (length < 0) {
		dev_warn(&client->dev,
			 "No battery-profile found, using current flash contents\n");
	} else if (length != SIZE_BATINFO) {
		dev_err(&client->dev, "battery-profile must be %d bytes\n",
			SIZE_BATINFO);
		return -EINVAL;
	} else {
		ret = device_property_read_u8_array(&client->dev,
						"cellwise,battery-profile",
						config_info, length);
		if (ret)
			return ret;
	}

	ret = cw_init(cw_bat);
	while ((loop++ < 3) && (ret != 0)) {
		msleep(200);
		ret = cw_init(cw_bat);
	}
	if (ret) {
		printk("%s : cw2015 init fail!\n", __func__);
		return ret;
	}

	ret = cw_init_data(cw_bat);
	if (ret) {
		printk("%s : cw2015 init data fail!\n", __func__);
		return ret;
	}

#ifdef CW_PROPERTIES
#if LINUX_VERSION_CODE < KERNEL_VERSION(4, 1, 0)
	cw_bat->cw_bat.name = CW_PROPERTIES;
	cw_bat->cw_bat.type = POWER_SUPPLY_TYPE_BATTERY;
	cw_bat->cw_bat.properties = cw_battery_properties;
	cw_bat->cw_bat.num_properties = ARRAY_SIZE(cw_battery_properties);
	cw_bat->cw_bat.get_property = cw_battery_get_property;
	ret = power_supply_register(&client->dev, &cw_bat->cw_bat);
	if(ret < 0) {
	    power_supply_unregister(&cw_bat->cw_bat);
	    return ret;
	}
#else
	psy_desc = devm_kzalloc(&client->dev, sizeof(*psy_desc), GFP_KERNEL);
	if (!psy_desc)
		return -ENOMEM;

	psy_cfg.drv_data = cw_bat;
	psy_desc->name = CW_PROPERTIES;
	psy_desc->type = POWER_SUPPLY_TYPE_BATTERY;
	psy_desc->properties = cw_battery_properties;
	psy_desc->num_properties = ARRAY_SIZE(cw_battery_properties);
	psy_desc->get_property = cw_battery_get_property;
	cw_bat->cw_bat = power_supply_register(&client->dev, psy_desc, &psy_cfg);
	if(IS_ERR(cw_bat->cw_bat)) {
		ret = PTR_ERR(cw_bat->cw_bat);
	    printk(KERN_ERR"failed to register battery: %d\n", ret);
	    return ret;
	}
#endif
#endif

	cw_bat->cwfg_workqueue = create_singlethread_workqueue("cwfg_gauge");
	INIT_DELAYED_WORK(&cw_bat->battery_delay_work, cw_bat_work);
	queue_delayed_work(cw_bat->cwfg_workqueue, &cw_bat->battery_delay_work , msecs_to_jiffies(50));

	printk("cw2015 driver probe success!\n");
	return 0;
}

#ifdef CONFIG_PM
static int cw_bat_suspend(struct device *dev)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct cw_battery *cw_bat = i2c_get_clientdata(client);

	cancel_delayed_work(&cw_bat->battery_delay_work);
	return 0;
}

static int cw_bat_resume(struct device *dev)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct cw_battery *cw_bat = i2c_get_clientdata(client);

	queue_delayed_work(cw_bat->cwfg_workqueue, &cw_bat->battery_delay_work, msecs_to_jiffies(2));
	return 0;
}

static const struct dev_pm_ops cw_bat_pm_ops = {
	.suspend  = cw_bat_suspend,
	.resume   = cw_bat_resume,
};
#endif

static void cw2015_remove(struct i2c_client *client)
{
	cw_printk("\n");
	return;
}

static const struct i2c_device_id cw2015_id_table[] = {
	{CWFG_NAME, 0},
	{}
};

static struct of_device_id cw2015_match_table[] = {
	{ .compatible = "cellwise,cw2015", },
	{ },
};

static struct i2c_driver cw2015_driver = {
	.driver 	  = {
		.name = CWFG_NAME,
#ifdef CONFIG_PM
		.pm     = &cw_bat_pm_ops,
#endif
		.owner	= THIS_MODULE,
		.of_match_table = cw2015_match_table,
	},
	.probe		  = cw2015_probe,
	.remove 	  = cw2015_remove,
	.id_table = cw2015_id_table,
};

module_i2c_driver(cw2015_driver);

MODULE_AUTHOR("Chaman Qi");
MODULE_DESCRIPTION("CW2015 FGADC Device Driver V3.0");
MODULE_LICENSE("GPL");
