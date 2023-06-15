/*
 * Papyrus epaper power control HAL
 *
 *      Copyright (C) 2009 Dimitar Dimitrov, MM Solutions
 *
 *  This file is subject to the terms and conditions of the GNU General Public
 *  License. See the file COPYING in the main directory of this archive for
 *  more details.
 *
 * TPS6518x power control is facilitated using I2C control and WAKEUP GPIO
 * pin control. The other VCC GPIO Papyrus' signals must be tied to ground.
 *
 * TODO:
 *	- Instead of polling, use interrupts to signal power up/down
 *	  acknowledge.
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/string.h>
#include <linux/mm.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/major.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/interrupt.h>
#include <linux/workqueue.h>
#include <linux/delay.h>
#include <linux/i2c.h>
#include <linux/version.h>
#include <linux/suspend.h>
#include <linux/gpio.h>
#include <linux/of.h>
#include <linux/of_gpio.h>
#include <linux/platform_device.h>
#include <linux/pinctrl/consumer.h>
#include <linux/pmic.h>


#define PAPYRUS2_1P1_I2C_ADDRESS 0x68
extern void papyrus_set_i2c_address(int address);

#include <linux/gpio.h>

struct ebc_pwr_ops {
	int (*power_on)(void);
	int (*power_down)(void);
};

struct pmic_mgr *g_pmic_mgr;

#define TPS65185_I2C_NAME "tps65185"
#define TPS65185_SLAVE_I2C_NAME "tps65185_slave"

#define PAPYRUS_VCOM_MAX_MV	0
#define PAPYRUS_VCOM_MIN_MV	-5110


#define CONTRUL_POWER_UP_PING (1)



#define PAPYRUS_POWER_UP_DELAY_MS   2

/* After waking up from sleep, Papyrus
   waits for VN to be discharged and all
   voltage ref to startup before loading
   the default EEPROM settings. So accessing
   registers too early after WAKEUP could
   cause the register to be overridden by
   default values */
#define PAPYRUS_EEPROM_DELAY_MS 50
/* Papyrus WAKEUP pin must stay low for
   a minimum time */
#define PAPYRUS_SLEEP_MINIMUM_MS 110
/* Temp sensor might take a little time to
   settle eventhough the status bit in TMST1
   state conversion is done - if read too early
   0C will be returned instead of the right temp */
#define PAPYRUS_TEMP_READ_TIME_MS 10

/* Powerup sequence takes at least 24 ms - no need to poll too frequently */
#define HW_GET_STATE_INTERVAL_MS 24

#define INVALID_GPIO -1

/* Addresses to scan */
static const unsigned short normal_i2c[2] = {PAPYRUS2_1P1_I2C_ADDRESS, I2C_CLIENT_END};
static const unsigned short normal_slave_i2c[2] = {PAPYRUS2_1P1_I2C_ADDRESS, I2C_CLIENT_END};

/*struct ctp_config_info config_info = {
	.input_type = CTP_TYPE,
	.name = NULL,
	.int_number = 0,
};*/


struct papyrus_sess {
	struct i2c_adapter *adap;
	struct i2c_client *client;
	uint8_t enable_reg_shadow;
	uint8_t enable_reg;
	uint8_t vadj;
	uint8_t vcom1;
	uint8_t vcom2;
	uint8_t vcom2off;
	uint8_t int_en1;
	uint8_t int_en2;
	uint8_t upseq0;
	uint8_t upseq1;
	uint8_t dwnseq0;
	uint8_t dwnseq1;
	uint8_t tmst1;
	uint8_t tmst2;

	/* Custom power up/down sequence settings */
	struct {
		/* If options are not valid we will rely on HW defaults. */
		bool valid;
		unsigned int dly[8];
	} seq;
	struct timespec standby_tv;
	unsigned int v3p3off_time_ms;
	int wake_up_pin;
	int vcom_ctl_pin;
	int power_up_pin;
	/* True if a high WAKEUP brings Papyrus out of reset. */
	int wakeup_active_high;
	int vcomctl_active_high;
	int power_active_heght;
	struct regulator *pmic_power_ldo;
};

#define tps65185_SPEED	(400*1000)


#define PAPYRUS_ADDR_TMST_VALUE		0x00
#define PAPYRUS_ADDR_ENABLE		0x01
#define PAPYRUS_ADDR_VADJ		0x02
#define PAPYRUS_ADDR_VCOM1_ADJUST	0x03
#define PAPYRUS_ADDR_VCOM2_ADJUST	0x04
#define PAPYRUS_ADDR_INT_ENABLE1	0x05
#define PAPYRUS_ADDR_INT_ENABLE2	0x06
#define PAPYRUS_ADDR_INT_STATUS1	0x07
#define PAPYRUS_ADDR_INT_STATUS2	0x08
#define PAPYRUS_ADDR_UPSEQ0		0x09
#define PAPYRUS_ADDR_UPSEQ1		0x0a
#define PAPYRUS_ADDR_DWNSEQ0		0x0b
#define PAPYRUS_ADDR_DWNSEQ1		0x0c
#define PAPYRUS_ADDR_TMST1		0x0d
#define PAPYRUS_ADDR_TMST2		0x0e
#define PAPYRUS_ADDR_PG_STATUS		0x0f
#define PAPYRUS_ADDR_REVID		0x10

// INT_ENABLE1
#define PAPYRUS_INT_ENABLE1_ACQC_EN	1
#define PAPYRUS_INT_ENABLE1_PRGC_EN 0

// INT_STATUS1
#define PAPYRUS_INT_STATUS1_ACQC	1
#define PAPYRUS_INT_STATUS1_PRGC	0

// VCOM2_ADJUST
#define PAPYRUS_VCOM2_ACQ	7
#define PAPYRUS_VCOM2_PROG	6
#define PAPYRUS_VCOM2_HIZ	5



#define PAPYRUS_MV_TO_VCOMREG(MV)	((MV) / 10)

#define V3P3_EN_MASK	0x20
#define PAPYRUS_V3P3OFF_DELAY_MS 10//100

struct papyrus_hw_state {
	uint8_t tmst_value;
	uint8_t int_status1;
	uint8_t int_status2;
	uint8_t pg_status;
};

//static uint8_t papyrus2_i2c_addr = PAPYRUS2_1P1_I2C_ADDRESS;

static int papyrus_resume_flag;



static int papyrus_hw_setreg(struct papyrus_sess *sess, uint8_t regaddr, uint8_t val)
{
	int stat;
	uint8_t txbuf[2] = { regaddr, val };
	struct i2c_msg msgs[] = {
		{
			.addr = sess->client->addr,//papyrus2_i2c_addr,
			.flags = 0,
			.len = 2,
			.buf = txbuf,
			//.scl_rate = tps65185_SPEED,
		}
	};

	stat = i2c_transfer(sess->adap, msgs, ARRAY_SIZE(msgs));

	if (stat < 0)
		pr_err("papyrus: I2C send error: %d\n", stat);
	else if (stat != ARRAY_SIZE(msgs)) {
		pr_err("papyrus: I2C send N mismatch: %d\n", stat);
		stat = -EIO;
	} else
		stat = 0;

	return stat;
}


static int papyrus_hw_getreg(struct papyrus_sess *sess, uint8_t regaddr, uint8_t *val)
{
	int stat;
	struct i2c_msg msgs[] = {
		{
			.addr = sess->client->addr,////papyrus2_i2c_addr,
			.flags = 0,
			.len = 1,
			.buf = &regaddr,
			//.scl_rate = tps65185_SPEED,
		},
		{
			.addr = sess->client->addr,//papyrus2_i2c_addr,
			.flags = I2C_M_RD,
			.len = 1,
			.buf = val,
			//.scl_rate = tps65185_SPEED,
		}
	};

	stat = i2c_transfer(sess->adap, msgs, ARRAY_SIZE(msgs));
	if (stat < 0) {
		pr_err("papyrus: I2C read error=%d,i2c_addr=0x%x\n", stat, sess->client->addr);
	} else if (stat != ARRAY_SIZE(msgs)) {
		pr_err("papyrus: I2C read N mismatch, ret=%d\n", stat);
		stat = -EIO;
	} else {
		stat = 0;
	}

	return stat;
}


static void papyrus_hw_get_pg(struct papyrus_sess *sess,
							  struct papyrus_hw_state *hwst)
{
	int stat;

	stat = papyrus_hw_getreg(sess,
				PAPYRUS_ADDR_PG_STATUS, &hwst->pg_status);
	if (stat)
		pr_err("papyrus: I2C error: %d\n", stat);
}

/*
static void papyrus_hw_get_state(struct papyrus_sess *sess, struct papyrus_hw_state *hwst)
{
	int stat;

	stat = papyrus_hw_getreg(sess,
				PAPYRUS_ADDR_TMST_VALUE, &hwst->tmst_value);
	stat |= papyrus_hw_getreg(sess,
				PAPYRUS_ADDR_INT_STATUS1, &hwst->int_status1);
	stat |= papyrus_hw_getreg(sess,
				PAPYRUS_ADDR_INT_STATUS2, &hwst->int_status2);
	stat |= papyrus_hw_getreg(sess,
				PAPYRUS_ADDR_PG_STATUS, &hwst->pg_status);
	if (stat)
		pr_err("papyrus: I2C error: %d\n", stat);
}
*/

static void papyrus_hw_send_powerup(struct papyrus_sess *sess)
{
	int stat = 0;

	//standby
	stat |= papyrus_hw_setreg(sess, PAPYRUS_ADDR_ENABLE, 0x40);

	// set VADJ
	stat |= papyrus_hw_setreg(sess, PAPYRUS_ADDR_VADJ, sess->vadj);

	// set UPSEQs & DWNSEQs
	stat |= papyrus_hw_setreg(sess, PAPYRUS_ADDR_UPSEQ0, sess->upseq0);
	stat |= papyrus_hw_setreg(sess, PAPYRUS_ADDR_UPSEQ1, sess->upseq1);
	stat |= papyrus_hw_setreg(sess, PAPYRUS_ADDR_DWNSEQ0, sess->dwnseq0);
	stat |= papyrus_hw_setreg(sess, PAPYRUS_ADDR_DWNSEQ1, sess->dwnseq1);

	// commit it, so that we can adjust vcom through "Rk_ebc_power_control_Release_v1.1"
	//stat |= papyrus_hw_setreg(sess, PAPYRUS_ADDR_VCOM1_ADJUST, sess->vcom1);
	//stat |= papyrus_hw_setreg(sess, PAPYRUS_ADDR_VCOM2_ADJUST, sess->vcom2);

#if 0
	/* Enable 3.3V switch to the panel */
	sess->enable_reg_shadow |= V3P3_EN_MASK;
	stat |= papyrus_hw_setreg(sess, PAPYRUS_ADDR_ENABLE, sess->enable_reg_shadow);
	msleep(sess->v3p3off_time_ms);
#endif

	/* switch to active mode, keep 3.3V & VEE & VDDH & VPOS & VNEG alive,
	 * don't enable vcom buffer
	 */
	sess->enable_reg_shadow = (0x80 | 0x20 | 0x0F);
	stat |= papyrus_hw_setreg(sess, PAPYRUS_ADDR_ENABLE, sess->enable_reg_shadow);
	if (stat)
		pr_err("[%s]----------papyrus: I2C error: %d\n", __func__, stat);
	return;
}


static void papyrus_hw_send_powerdown(struct papyrus_sess *sess)
{
	int stat;

	/* switch to standby mode, keep 3.3V & VEE & VDDH & VPOS & VNEG alive,
	 * don't enable vcom buffer
	 */
	sess->enable_reg_shadow = (0x40 | 0x20 | 0x0F);
	stat = papyrus_hw_setreg(sess, PAPYRUS_ADDR_ENABLE, sess->enable_reg_shadow);

#if 0
	/* 3.3V switch must be turned off last */
	msleep(sess->v3p3off_time_ms);
	sess->enable_reg_shadow &= ~V3P3_EN_MASK;
	stat |= papyrus_hw_setreg(sess, PAPYRUS_ADDR_ENABLE, sess->enable_reg_shadow);
	if (stat)
		pr_err("papyrus: I2C error: %d\n", stat);
#endif

	return;
}
static int papyrus_hw_read_temperature(struct pmic_sess *pmsess, int *t)
{
	struct papyrus_sess *sess = (struct papyrus_sess *)pmsess->drvpar;
	int stat;
	int ntries = 50;
	int nwait = 5;
	uint8_t tb;

	if (pmsess->id != MASTER_PMIC_ID) {
		*t = -1;
		pr_err("only master pmic have NTC\n");
		return -1;
	}

	while (!papyrus_resume_flag && nwait) {
		msleep(5);
		nwait = nwait - 1;
		pr_info("wait papyrus_pm_resume papyrus_resume_flag = %d nwait = %d\n", papyrus_resume_flag, nwait);
	}

	stat = papyrus_hw_setreg(sess, PAPYRUS_ADDR_TMST1, 0x80);
	do {
		stat = papyrus_hw_getreg(sess,
				PAPYRUS_ADDR_TMST1, &tb);
	} while (!stat && ntries-- && (((tb & 0x20) == 0) || (tb & 0x80)));

	if (stat) {
		*t = -1;  //communication fail, return -1 degree
		pr_err("papyrus_hw_read_temperature fail *t = %d stat = %d\n", *t, stat);
		return stat;
	}

	msleep(PAPYRUS_TEMP_READ_TIME_MS);
	stat = papyrus_hw_getreg(sess, PAPYRUS_ADDR_TMST_VALUE, &tb);
	*t = (int)(int8_t)tb;


	//tps65185_printk("current temperature is %d\n",*t);

	return stat;
}
static int papyrus_hw_get_revid(struct papyrus_sess *sess)
{
	int stat;
	uint8_t revid;

	stat = papyrus_hw_getreg(sess, PAPYRUS_ADDR_REVID, &revid);
	if (stat) {
		pr_err("papyrus: I2C error: %d\n", stat);
		return stat;
	} else
		return revid;
}

static int papyrus_hw_arg_init(struct papyrus_sess *sess)
{
	sess->vadj = 0x03;

	sess->upseq0 = SEQ_VEE(0) | SEQ_VNEG(1) | SEQ_VPOS(2) | SEQ_VDD(3);
	sess->upseq1 = UDLY_3ms(0) | UDLY_3ms(0) | UDLY_3ms(0) | UDLY_3ms(0);

	sess->dwnseq0 = SEQ_VDD(0) | SEQ_VPOS(1) | SEQ_VNEG(2) | SEQ_VEE(3);
	sess->dwnseq1 = DDLY_6ms(0) | DDLY_6ms(0) | DDLY_6ms(0) | DDLY_6ms(0);

	sess->vcom1 = (PAPYRUS_MV_TO_VCOMREG(2500) & 0x00FF);
	sess->vcom2 = ((PAPYRUS_MV_TO_VCOMREG(2500) & 0x0100) >> 8);

	return 0;
}


static int papyrus_hw_init(struct papyrus_sess *sess, PMIC_ID pmic_id)
{

	if (pmic_id != MASTER_PMIC_ID) {
		pr_info("not master pmic, do not control gpio\n");
		return 0;
	}

#if (CONTRUL_POWER_UP_PING)
	//make sure power up is low
	if (gpio_is_valid(sess->power_up_pin)) {
		gpio_direction_output(sess->power_up_pin, !sess->power_active_heght);
		msleep(PAPYRUS_POWER_UP_DELAY_MS);
	}
#endif
	if (gpio_is_valid(sess->wake_up_pin)) {
		gpio_direction_output(sess->wake_up_pin, sess->wakeup_active_high);
	}
	if (gpio_is_valid(sess->vcom_ctl_pin)) {
		gpio_direction_output(sess->vcom_ctl_pin, sess->vcomctl_active_high);
		msleep(PAPYRUS_EEPROM_DELAY_MS);
	}

	return 0;

//free_gpios:
	gpio_free(sess->wake_up_pin);
	gpio_free(sess->vcom_ctl_pin);
#if (CONTRUL_POWER_UP_PING)
	gpio_free(sess->power_up_pin);
#endif
	pr_err("papyrus: ERROR: could not initialize I2C papyrus!\n");
	return -1;
}

static void papyrus_hw_power_req(struct pmic_sess *pmsess, bool up)
{
	struct papyrus_sess *sess = pmsess->drvpar;

	pr_info("papyrus: i2c pwr req: %d\n", up);
	if (up) {
		papyrus_hw_send_powerup(sess);
	} else {
		papyrus_hw_send_powerdown(sess);
	}
	return;
}


static bool papyrus_hw_power_ack(struct pmic_sess *pmsess)
{
	struct papyrus_sess *sess = pmsess->drvpar;
	struct papyrus_hw_state hwst;
	int st;
	int retries_left = 10;

	do {
		papyrus_hw_get_pg(sess, &hwst);

		pr_debug("hwst: tmst_val=%d, ist1=%02x, ist2=%02x, pg=%02x\n",
				hwst.tmst_value, hwst.int_status1,
				hwst.int_status2, hwst.pg_status);
		hwst.pg_status &= 0xfa;
		if (hwst.pg_status == 0xfa)
			st = 1;
		else if (hwst.pg_status == 0x00)
			st = 0;
		else {
			st = -1;	/* not settled yet */
			msleep(HW_GET_STATE_INTERVAL_MS);
		}
		retries_left--;
	} while ((st == -1) && retries_left);

	if ((st == -1) && !retries_left)
		pr_err("papyrus: power up/down settle error (PG = %02x)\n", hwst.pg_status);

	return !!st;
}


static void papyrus_hw_cleanup(struct papyrus_sess *sess)
{
	gpio_free(sess->wake_up_pin);
	gpio_free(sess->vcom_ctl_pin);
#if (CONTRUL_POWER_UP_PING)
	gpio_free(sess->power_up_pin);
#endif

	i2c_put_adapter(sess->adap);
}


/* -------------------------------------------------------------------------*/

static int papyrus_set_enable(struct pmic_sess *pmsess, int enable)
{
	struct papyrus_sess *sess = pmsess->drvpar;

	sess->enable_reg = enable;
	return 0;
}

static int papyrus_set_vcom_voltage(struct pmic_sess *pmsess, int vcom_mv)
{
	struct papyrus_sess *sess = pmsess->drvpar;

	sess->vcom1 = (PAPYRUS_MV_TO_VCOMREG(-vcom_mv) & 0x00FF);
	sess->vcom2 = ((PAPYRUS_MV_TO_VCOMREG(-vcom_mv) & 0x0100) >> 8);
	return 0;
}

static int papyrus_set_vcom1(struct pmic_sess *pmsess, uint8_t vcom1)
{
	struct papyrus_sess *sess = pmsess->drvpar;

	sess->vcom1 = vcom1;
	return 0;
}

static int papyrus_set_vcom2(struct pmic_sess *pmsess, uint8_t vcom2)
{
	struct papyrus_sess *sess = pmsess->drvpar;
	// TODO; Remove this temporary solution to set custom vcom-off mode
	//       Add PMIC setting when this is to be a permanent feature
	pr_debug("papyrus_set_vcom2 vcom2off 0x%02x\n", vcom2);
	sess->vcom2off = vcom2;
	return 0;
}

static int papyrus_set_vadj(struct pmic_sess *pmsess, uint8_t vadj)
{
	struct papyrus_sess *sess = pmsess->drvpar;

	sess->vadj = vadj;
	return 0;
}

static int papyrus_set_int_en1(struct pmic_sess *pmsess, uint8_t int_en1)
{
	struct papyrus_sess *sess = pmsess->drvpar;

	sess->int_en1 = int_en1;
	return 0;
}

static int papyrus_set_int_en2(struct pmic_sess *pmsess, uint8_t int_en2)
{
	struct papyrus_sess *sess = pmsess->drvpar;

	sess->int_en2 = int_en2;
	return 0;
}

static int papyrus_set_upseq0(struct pmic_sess *pmsess, uint8_t upseq0)
{
	struct papyrus_sess *sess = pmsess->drvpar;

	sess->upseq0 = upseq0;
	return 0;
}

static int papyrus_set_upseq1(struct pmic_sess *pmsess, uint8_t upseq1)
{
	struct papyrus_sess *sess = pmsess->drvpar;

	sess->upseq1 = upseq1;
	return 0;
}

static int papyrus_set_dwnseq0(struct pmic_sess *pmsess, uint8_t dwnseq0)
{
	struct papyrus_sess *sess = pmsess->drvpar;

	sess->dwnseq0 = dwnseq0;
	return 0;
}

static int papyrus_set_dwnseq1(struct pmic_sess *pmsess, uint8_t dwnseq1)
{
	struct papyrus_sess *sess = pmsess->drvpar;

	sess->dwnseq1 = dwnseq1;
	return 0;
}

static int papyrus_set_tmst1(struct pmic_sess *pmsess, uint8_t tmst1)
{
	struct papyrus_sess *sess = pmsess->drvpar;

	sess->tmst1 = tmst1;
	return 0;
}

static int papyrus_set_tmst2(struct pmic_sess *pmsess, uint8_t tmst2)
{
	struct papyrus_sess *sess = pmsess->drvpar;

	sess->tmst2 = tmst2;
	return 0;
}

static int papyrus_vcom_switch(struct pmic_sess *pmsess, bool state)
{
	struct papyrus_sess *sess = pmsess->drvpar;
	int stat;

	sess->enable_reg_shadow &= ~((1u << 4) | (1u << 6) | (1u << 7));
	sess->enable_reg_shadow |= (state ? 1u : 0) << 4;

	stat = papyrus_hw_setreg(sess, PAPYRUS_ADDR_ENABLE,
						sess->enable_reg_shadow);

	/* set VCOM off output */
	if (!state && sess->vcom2off != 0) {
		stat = papyrus_hw_setreg(sess, PAPYRUS_ADDR_VCOM2_ADJUST,
						sess->vcom2off);
	}

	return stat;
}

static bool papyrus_standby_dwell_time_ready(struct pmic_sess *pmsess)
{
	struct papyrus_sess *sess = pmsess->drvpar;
	struct timespec current_tv;
	long total_secs;

	getnstimeofday(&current_tv);
	mb();
	total_secs = current_tv.tv_sec - sess->standby_tv.tv_sec;

	if (total_secs < PAPYRUS_STANDBY_DWELL_TIME)
		return false;

	return true;
}

static int papyrus_pm_sleep(struct pmic_sess *sess)
{
	struct papyrus_sess *s = sess->drvpar;

	pr_info("PMIC%d: enter to sleep\n", sess->id);

	if (sess->id == MASTER_PMIC_ID) {
		gpio_direction_output(s->vcom_ctl_pin, !s->vcomctl_active_high);
		gpio_direction_output(s->wake_up_pin, !s->wakeup_active_high);
		papyrus_resume_flag = 0;
	}

	if (regulator_disable(s->pmic_power_ldo) != 0) {
		printk("some error happen, fail to disable regulator!\n");
	}

	return 0;
}

static int papyrus_pm_resume(struct pmic_sess *sess)
{
	struct papyrus_sess *s = sess->drvpar;

	pr_info("PMIC%d: resume from sleep\n", sess->id);

	if (sess->id == MASTER_PMIC_ID) {
		gpio_direction_output(s->wake_up_pin, s->wakeup_active_high);
		gpio_direction_output(s->vcom_ctl_pin, s->vcomctl_active_high);
		papyrus_resume_flag = 1;
	}

	if (regulator_enable(s->pmic_power_ldo) != 0) {
		printk("some error happen, fail to enable regulator!\n");
	}

	return 0;
}

static int tps65185_get_pin(struct pmic_config *config, int i)
{
	char *pmic_node_array[MAX_PMIC_NUM] = {"allwinner,tps65185", "allwinner,tps65185_slave"};
	struct device_node *np = NULL;
	enum of_gpio_flags flags;

	if ((config == NULL) || (i >= MAX_PMIC_NUM)) {
		pr_warn("%s: input param is wrong\n", __func__);
		return -1;
	}
	np = of_find_compatible_node(NULL, NULL, pmic_node_array[i]);

	if (np == NULL) {
		pr_err("PMIC%d: cannot find config node(%s)\n", i, pmic_node_array[i]);
		return -2;
	}

	config->wake_up_pin = of_get_named_gpio_flags(np, "tps65185_wakeup", 0, &flags);

	if (!gpio_is_valid(config->wake_up_pin))
		pr_err("PMIC%d: tps65185_wakeup is invalid.\n", i);
	else
		pr_info("PMIC%d: tps65185_wakeup = %d\n", i, config->wake_up_pin);

	config->vcom_ctl_pin = of_get_named_gpio_flags(np,
			"tps65185_vcom", 0, &flags);

	if (!gpio_is_valid(config->vcom_ctl_pin))
		pr_err("PMIC%d: tps65185_vcom is invalid.\n", i);
	else
		pr_info("PMIC%d: tps65185_vcom = %d\n", i, config->vcom_ctl_pin);

	config->power_up_pin = of_get_named_gpio_flags(np, "tps65185_powerup", 0,
					&flags);

	if (!gpio_is_valid(config->power_up_pin))
		pr_err("PMIC%d: tps65185_powerup is invalid.\n", i);
	else
		pr_info("PMIC%d: tps65185_powerup = %d\n", i, config->power_up_pin);

	config->power_good_pin = of_get_named_gpio_flags(np,
				"tps65185_powergood", 0, &flags);
	if (!gpio_is_valid(config->power_good_pin))
		pr_err("PMIC%d: tps65185_powergood is invalid.\n", i);
	else
		pr_info("PMIC%d: tps65185_powergood = %d\n", i, config->power_good_pin);

	config->int_pin = of_get_named_gpio_flags(np, "tps65185_int", 0,
						&flags);
	if (!gpio_is_valid(config->int_pin))
		pr_err("PMIC%d: tps65185_int is invalid.\n", i);
	else
		pr_info("PMIC%d: tps65185_int = %d\n", i, config->int_pin);

	return 0;
}

static int tps65185_get_config(struct pmic_config *config, int i)
{
	char *pmic_node_array[MAX_PMIC_NUM] = {"allwinner,tps65185", "allwinner,tps65185_slave"};
	struct device_node *np = NULL;
	const char *str;

	if ((config == NULL) || (i >= MAX_PMIC_NUM)) {
		pr_warn("%s: input param is wrong\n", __func__);
		return -1;
	}
	np = of_find_compatible_node(NULL, NULL, pmic_node_array[i]);

	if (np == NULL) {
		pr_err("PMIC%d: cannot find config node(%s)\n", i, pmic_node_array[i]);
		return -2;
	}


	if (0 != of_property_read_u32_array(np, "tps65185_used", &(config->used_flag), 1))
		pr_err("PMIC%d: get tps65185_used fail\n", i);
	else
		pr_info("PMIC%d: tps65185_used = %d\n", i, config->used_flag);

	if (config->used_flag != true) {
		return 0;
	}

	memset(config->name, 0, sizeof(config->name));
	if (0 != of_property_read_string(np, "tps65185_name", &str))
		pr_err("PMIC%d: get tps65185_name fail\n", i);
	else {
		memcpy((void *)(config->name), str, strlen(str) + 1);
		pr_info("PMIC%d: tps65185_name = %s\n", i, config->name);
	}

	if (0 != of_property_read_u32_array(np, "tps65185_twi_id", &(config->twi_id), 1))
		pr_err("PMIC%d: get tps65185_twi_id fail\n", i);
	else
		pr_info("PMIC%d: tps65185_twi_id = %d\n", i, config->twi_id);

	if (0 != of_property_read_u32_array(np, "tps65185_twi_addr", &(config->twi_addr), 1))
		pr_err("PMIC%d: get tps65185_twi_addr fail\n", i);
	else
		pr_info("PMIC%d: tps65185_twi_addr = 0x%x\n", i, config->twi_addr);


	memset(config->pmic_power_name, 0, sizeof(config->pmic_power_name));
	if (0 != of_property_read_string(np, "tps65185_ldo", &str))
		pr_err("PMIC%d: get tps65185_ldo fail\n", i);
	else {
		memcpy((void *)(config->pmic_power_name), str, strlen(str) + 1);
		pr_info("PMIC%d: tps65185_ldo = %s\n", i, config->pmic_power_name);
	}

	return 0;
}

static int papyrus_probe(struct pmic_sess *pmsess, struct pmic_config *config, struct i2c_client *client)
{
	struct papyrus_sess *sess = NULL;
	struct papyrus_sess *tmp_sess = NULL;
	struct pmic_sess *tmp_pmsess = NULL;
	PMIC_ID id = 0;
	int stat;
	int ret = 0;

	if ((pmsess == NULL) || (config == NULL) || (client == NULL)) {
		pr_err("PMIC%d: input param is null\n", id);
		return -1;
	}

	sess = kzalloc(sizeof(*sess), GFP_KERNEL);
	if (!sess)
		return -ENOMEM;

	pr_info("[%s]-------------------\n", __func__);
	sess->client = client;
	sess->adap = client->adapter;

	sess->wake_up_pin = config->wake_up_pin;
	sess->vcom_ctl_pin = config->vcom_ctl_pin;
	sess->power_up_pin = config->power_up_pin;
	sess->wakeup_active_high = 1;
	sess->vcomctl_active_high = 1;
	sess->power_active_heght = 1;

	sess->enable_reg_shadow = 0;
#if IS_ENABLED(CONFIG_REGULATOR)
	sess->pmic_power_ldo = regulator_get(config->dev, (const char *)config->pmic_power_name);
	if (IS_ERR_OR_NULL(sess->pmic_power_ldo)) {
		pr_err("cannot get pmic ldo(%s)\n", config->pmic_power_name);
	} else {
		regulator_set_voltage(sess->pmic_power_ldo, 3300*1000, 3300*1000);
		ret = regulator_enable(sess->pmic_power_ldo);
		if (ret)
			pr_warn("[%s]: pmic regulator enable fail\n", __func__);
	}
#endif

	if (pmsess->v3p3off_time_ms == -1)
		sess->v3p3off_time_ms = PAPYRUS_V3P3OFF_DELAY_MS;
	else
		sess->v3p3off_time_ms = pmsess->v3p3off_time_ms;

	papyrus_hw_arg_init(sess);

	pmsess->drvpar = sess;

	if (pmsess->id == MASTER_PMIC_ID) {
		ret = tps65185_get_pin(&(g_pmic_mgr->config[pmsess->id]), pmsess->id);
		stat = papyrus_hw_init(sess, pmsess->id);
		if (stat)
			goto free_sess;

		for (id = 0; id < MAX_PMIC_NUM; id++) {
			if (g_pmic_mgr->config[id].used_flag != 1) {
				continue;
			}

			tmp_pmsess = &(g_pmic_mgr->sess[id]);
			tmp_sess = (struct papyrus_sess *)tmp_pmsess->drvpar;

			stat = papyrus_hw_setreg(tmp_sess, PAPYRUS_ADDR_ENABLE, tmp_sess->enable_reg_shadow);
			if (stat)
				goto free_sess;

			tmp_pmsess->revision = papyrus_hw_get_revid(tmp_sess);
			pr_info("PMIC%d: get revision = 0x%08x\n", id, tmp_pmsess->revision);
		}
	}

	return 0;

free_sess:
	kfree(sess);
	return stat;
}

static void papyrus_remove(struct pmic_sess *pmsess)
{
	struct papyrus_sess *sess = pmsess->drvpar;

	if (pmsess->id == MASTER_PMIC_ID)
		papyrus_hw_cleanup(sess);

	kfree(sess);
	pmsess->drvpar = 0;

}

const struct pmic_driver pmic_driver_tps65185_i2c = {
	.id = "tps65185-i2c",

	.vcom_min = PAPYRUS_VCOM_MIN_MV,
	.vcom_max = PAPYRUS_VCOM_MAX_MV,
	.vcom_step = 10,

	.hw_read_temperature = papyrus_hw_read_temperature,
	.hw_power_ack = papyrus_hw_power_ack,
	.hw_power_req = papyrus_hw_power_req,

	.set_enable = papyrus_set_enable,
	.set_vcom_voltage = papyrus_set_vcom_voltage,
	.set_vcom1 = papyrus_set_vcom1,
	.set_vcom2 = papyrus_set_vcom2,
	.set_vadj = papyrus_set_vadj,
	.set_int_en1 = papyrus_set_int_en1,
	.set_int_en2 = papyrus_set_int_en2,
	.set_upseq0 = papyrus_set_upseq0,
	.set_upseq1 = papyrus_set_upseq1,
	.set_dwnseq0 = papyrus_set_dwnseq0,
	.set_dwnseq1 = papyrus_set_dwnseq1,
	.set_tmst1 = papyrus_set_tmst1,
	.set_tmst2 = papyrus_set_tmst2,

	.hw_vcom_switch = papyrus_vcom_switch,

	//.hw_init = papyrus_probe,
	.hw_cleanup = papyrus_remove,

	.hw_standby_dwell_time_ready = papyrus_standby_dwell_time_ready,
	.hw_pm_sleep = papyrus_pm_sleep,
	.hw_pm_resume = papyrus_pm_resume,
};

static int tps65185_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	int ret = -1;
	PMIC_ID pmic_id = MASTER_PMIC_ID;
	struct pmic_sess *sess;
	struct pmic_config *config;

	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		pr_err("I2C check functionality failed\n");
		return -ENODEV;
	}

	sess = &(g_pmic_mgr->sess[pmic_id]);
	config = &(g_pmic_mgr->config[pmic_id]);
	config->dev = &client->adapter->dev;

	ret = papyrus_probe(sess, config, client);
	if (ret != 0) {
		pr_err("hw_init master pmic failed.");
		return -ENODEV;
	}

	papyrus_resume_flag = 1;
	sess->is_inited = 1;

	pr_info("[%s]:hw_init master pmic ok-------------------------\n", __func__);

	return 0;

}

static int tps65185_slave_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	int ret = -1;
	PMIC_ID pmic_id = SLAVE_PMIC_ID;
	struct pmic_sess *sess;
	struct pmic_config *config;

	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		pr_err("I2C check functionality failed\n");
		return -ENODEV;
	}

	sess = &(g_pmic_mgr->sess[pmic_id]);
	config = &(g_pmic_mgr->config[pmic_id]);

	ret = papyrus_probe(sess, config, client);
	if (ret != 0) {
		pr_err("hw_init slave pmic failed.");
		return -ENODEV;
	}

	papyrus_resume_flag = 1;
	sess->is_inited = 1;

	pr_info("hw_init slave pmic ok\n");

	return 0;
}


static int tps65185_remove(struct i2c_client *client)
{
	struct pmic_sess *pmic_sess = &(g_pmic_mgr->sess[MASTER_PMIC_ID]);
	struct papyrus_sess *sess = NULL;

	sess = (struct papyrus_sess *)pmic_sess->drvpar;
	pmic_driver_tps65185_i2c.hw_cleanup(pmic_sess);

	if (!IS_ERR(sess->pmic_power_ldo)) {
		regulator_disable(sess->pmic_power_ldo);
		regulator_put(sess->pmic_power_ldo);
	}

	kfree(sess);

	pmic_sess->drvpar = NULL;

	return 0;
}

static int tps65185_slave_remove(struct i2c_client *client)
{
	struct pmic_sess *pmic_sess = &(g_pmic_mgr->sess[SLAVE_PMIC_ID]);
	struct papyrus_sess *sess = NULL;

	if (g_pmic_mgr->config[SLAVE_PMIC_ID].used_flag != 1) {
		return 0;
	}

	sess = (struct papyrus_sess *)pmic_sess->drvpar;
	pmic_driver_tps65185_i2c.hw_cleanup(pmic_sess);

	if (!IS_ERR(sess->pmic_power_ldo)) {
		regulator_disable(sess->pmic_power_ldo);
		regulator_put(sess->pmic_power_ldo);
	}

	kfree(sess);

	pmic_sess->drvpar = NULL;

	return 0;
}


static int tps65185_suspend(struct device *dev)
{
	return pmic_driver_tps65185_i2c.hw_pm_sleep(&g_pmic_mgr->sess[MASTER_PMIC_ID]);
}
static int tps65185_resume(struct device *dev)
{
	return pmic_driver_tps65185_i2c.hw_pm_resume(&g_pmic_mgr->sess[MASTER_PMIC_ID]);
}

static int tps65185_slave_suspend(struct device *dev)
{
	return pmic_driver_tps65185_i2c.hw_pm_sleep(&g_pmic_mgr->sess[SLAVE_PMIC_ID]);
}
static int tps65185_slave_resume(struct device *dev)
{
	return pmic_driver_tps65185_i2c.hw_pm_resume(&g_pmic_mgr->sess[SLAVE_PMIC_ID]);
}

static int tps65185_i2c_detect(struct i2c_client *client, struct i2c_board_info *info)
{
	struct i2c_adapter *adapter = client->adapter;

	if (g_pmic_mgr->config[MASTER_PMIC_ID].twi_id == adapter->nr) {
		strlcpy(info->type, TPS65185_I2C_NAME, I2C_NAME_SIZE);

		pr_info("%s: Detected chip %s at adapter %d, address 0x%02x\n",
			 __func__, TPS65185_I2C_NAME, i2c_adapter_id(adapter), client->addr);
		return 0;
	}

	return -ENODEV;
}

static int tps65185_slave_i2c_detect(struct i2c_client *client, struct i2c_board_info *info)
{
	struct i2c_adapter *adapter = client->adapter;

	if (g_pmic_mgr->config[SLAVE_PMIC_ID].twi_id == adapter->nr) {
		strlcpy(info->type, TPS65185_SLAVE_I2C_NAME, I2C_NAME_SIZE);

		pr_info("%s: Detected chip %s at adapter %d, address 0x%02x\n",
			 __func__, TPS65185_SLAVE_I2C_NAME, i2c_adapter_id(adapter), client->addr);
		return 0;
	}

	return -ENODEV;
}

static const struct i2c_device_id tps65185_id[] = {
	{ TPS65185_I2C_NAME, 0 },
	{ }
};


static const struct i2c_device_id tps65185_slave_id[] = {
	{ TPS65185_SLAVE_I2C_NAME, 0 },
	{ }
};

static const struct dev_pm_ops tps65185_pm_ops = {
	.suspend = tps65185_suspend,
	.resume = tps65185_resume,
};

static const struct dev_pm_ops tps65185_slave_pm_ops = {
	.suspend = tps65185_slave_suspend,
	.resume = tps65185_slave_resume,
};


MODULE_DEVICE_TABLE(i2c, tps65185_id);

static struct i2c_driver tps65185_driver = {
	.class = I2C_CLASS_HWMON,
	.probe	= tps65185_probe,
	.remove	= tps65185_remove,
	.id_table	= tps65185_id,
	.detect   = tps65185_i2c_detect,
	.driver = {
		.name	  = TPS65185_I2C_NAME,
		.owner	  = THIS_MODULE,
		.pm             = &tps65185_pm_ops,
	},
	.address_list   = normal_i2c,
};

MODULE_DEVICE_TABLE(i2c, tps65185_slave_id);

static struct i2c_driver tps65185_slave_driver = {
	.class = I2C_CLASS_HWMON,
	.probe	= tps65185_slave_probe,
	.remove	= tps65185_slave_remove,
	.id_table	= tps65185_slave_id,
	.detect   = tps65185_slave_i2c_detect,
	.driver = {
		.name	  = TPS65185_SLAVE_I2C_NAME,
		.owner	  = THIS_MODULE,
		.pm             = &tps65185_slave_pm_ops,
	},
	.address_list   = normal_slave_i2c,
};


static int tps65185_manager_init(void)
{
	int i = 0;
	int ret = -1;

	g_pmic_mgr = kmalloc(sizeof(*g_pmic_mgr), GFP_KERNEL);
	if (g_pmic_mgr == NULL) {
		printk(KERN_ERR "%s: fail to alloc memory\n", __func__);
		return -1;
	}

	memset(g_pmic_mgr, 0, sizeof(*g_pmic_mgr));
	for (i = 0; i < MAX_PMIC_NUM; i++) {
		g_pmic_mgr->sess[i].id = i;
		ret = tps65185_get_config(&(g_pmic_mgr->config[i]), i);
		if (ret) {
			pr_err("PMIC%d: get config fail, ret=%d\n", i, ret);
			break;
		}
	}

	/* master pmic is always used */
	g_pmic_mgr->config[MASTER_PMIC_ID].used_flag = 1;

	return ret;
}

static void tps65185_manager_exit(void)
{
	kfree(g_pmic_mgr);
	return;
}

static int __init tps65185_init(void)
{
	int ret = -1;

	ret = tps65185_manager_init();
	if (ret) {
		pr_err("pmic manager init fail, ret=%d", ret);
		return -1;
	}

	if (g_pmic_mgr->config[SLAVE_PMIC_ID].used_flag) {
		ret = i2c_add_driver(&tps65185_slave_driver);
		if (ret) {
			pr_err("fail to init slave pmic, ret=%d\n", ret);
			return -2;
		}
		pr_info("init slave pmic ok\n");
	}

	ret = i2c_add_driver(&tps65185_driver);
	if (ret) {
		pr_err("fail to init master pmic, ret=%d\n", ret);
	} else {
		pr_info("init master pmic ok\n");
	}

	return ret;
}

static void __exit tps65185_exit(void)
{
	if (g_pmic_mgr->config[SLAVE_PMIC_ID].used_flag) {
		i2c_del_driver(&tps65185_slave_driver);
	}

	if (g_pmic_mgr->config[MASTER_PMIC_ID].used_flag) {
		i2c_del_driver(&tps65185_driver);
	}

	tps65185_manager_exit();

	return;
}

fs_initcall(tps65185_init);
module_exit(tps65185_exit);

MODULE_DESCRIPTION("ti tps65185 pmic");
MODULE_LICENSE("GPL");

int tps65185_vcom_set(int vcom_mv)
{
	struct pmic_sess *master_pmsess = NULL, *tmp_pmsess = NULL;
	struct papyrus_sess *master_sess = NULL, *tmp_sess = NULL;
	PMIC_ID id = 0;
	uint8_t rev_val = 0;
	int stat = 0;
	int cnt = 20;

	if (g_pmic_mgr->sess[MASTER_PMIC_ID].is_inited != 1) {
		pr_err("master pmic has not initial yet\n");
		return -2;
	}

	pr_info("[%s]: vcom = %d\n", __func__, vcom_mv);
	master_pmsess = &(g_pmic_mgr->sess[MASTER_PMIC_ID]);
	master_sess = (struct papyrus_sess *)master_pmsess->drvpar;

	//wakeup 65185, sleep mode --> standby mode
#if (CONTRUL_POWER_UP_PING)
	//make sure power up is low
	gpio_direction_output(master_sess->power_up_pin, !master_sess->power_active_heght);
	msleep(2);
#endif
	gpio_direction_output(master_sess->wake_up_pin, 1);
	msleep(10);

	for (id = 0; id < MAX_PMIC_NUM; id++) {
		if ((g_pmic_mgr->config[id].used_flag != 1) || (g_pmic_mgr->sess[id].is_inited != 1)) {
			continue;
		}

		tmp_pmsess = &(g_pmic_mgr->sess[id]);
		tmp_sess = (struct papyrus_sess *)tmp_pmsess->drvpar;

		// Set vcom voltage
		pmic_driver_tps65185_i2c.set_vcom_voltage((struct pmic_sess *)tmp_pmsess, vcom_mv);
		stat |= papyrus_hw_setreg(tmp_sess, PAPYRUS_ADDR_VCOM1_ADJUST, tmp_sess->vcom1);
		stat |= papyrus_hw_setreg(tmp_sess, PAPYRUS_ADDR_VCOM2_ADJUST, tmp_sess->vcom2);
		pr_debug("sess->vcom1 = 0x%x sess->vcom2 = 0x%x\n", tmp_sess->vcom1, tmp_sess->vcom2);

		// PROGRAMMING
		tmp_sess->vcom2 |= 1<<PAPYRUS_VCOM2_PROG;
		stat |= papyrus_hw_setreg(tmp_sess, PAPYRUS_ADDR_VCOM2_ADJUST, tmp_sess->vcom2);
		rev_val = 0;
		while (!(rev_val & (1<<PAPYRUS_INT_STATUS1_PRGC))) {
			stat |= papyrus_hw_getreg(tmp_sess, PAPYRUS_ADDR_INT_STATUS1, &rev_val);
			pr_debug("PAPYRUS_ADDR_INT_STATUS1 = 0x%x\n", rev_val);
			msleep(50);
			if (cnt-- <= 0) {
				pr_err("PMIC%d: set vcom timeout\n", id);
				break;
			}
		}
	}

	if (stat)
		pr_err("papyrus: I2C error: %d\n", stat);

	return 0;
}
EXPORT_SYMBOL(tps65185_vcom_set);

static int tps65185_power_on(void)
{
	struct pmic_sess *tmp_pmsess = NULL;
	struct papyrus_sess *tmp_sess = NULL;
	PMIC_ID id = 0;

	pr_info("[%s]:----------------------\n", __func__);

	for (id = 0; id < MAX_PMIC_NUM; id++) {
		if ((g_pmic_mgr->config[id].used_flag != 1) || (g_pmic_mgr->sess[id].is_inited != 1)) {
			continue;
		}

		tmp_pmsess = &(g_pmic_mgr->sess[id]);
		tmp_sess = (struct papyrus_sess *)tmp_pmsess->drvpar;
		pmic_driver_tps65185_i2c.hw_power_req((struct pmic_sess *)tmp_pmsess, 1);
	}

	return 0;
}
static int tps65185_power_down(void)
{
	struct pmic_sess *tmp_pmsess = NULL;
	struct papyrus_sess *tmp_sess = NULL;
	PMIC_ID id = 0;

	pr_info("[%s]:----------------------\n", __func__);

	for (id = 0; id < MAX_PMIC_NUM; id++) {
		if ((g_pmic_mgr->config[id].used_flag != 1) || (g_pmic_mgr->sess[id].is_inited != 1)) {
			continue;
		}

		tmp_pmsess = &(g_pmic_mgr->sess[id]);
		tmp_sess = (struct papyrus_sess *)tmp_pmsess->drvpar;
		pmic_driver_tps65185_i2c.hw_power_req((struct pmic_sess *)tmp_pmsess, 0);
	}

	return 0;
}
int tps65185_temperature_get(int *temp)
{
	struct pmic_sess *master_pmsess = NULL;

	master_pmsess = &g_pmic_mgr->sess[MASTER_PMIC_ID];
	if (master_pmsess->is_inited)
		return pmic_driver_tps65185_i2c.hw_read_temperature(master_pmsess, temp);
	else
		return 0;
}
EXPORT_SYMBOL(tps65185_temperature_get);

int register_ebc_pwr_ops(struct ebc_pwr_ops *ops)
{
	ops->power_on = tps65185_power_on;
	ops->power_down = tps65185_power_down;
	return 0;
}
EXPORT_SYMBOL(register_ebc_pwr_ops);


#ifdef CONFIG_EPD_TPS65185_SENSOR
int register_ebc_temp_ops(struct ebc_temperateure_ops *ops)
{
	ops->temperature_get = tps65185_temperature_get;
	return 0;
}

#ifdef CONFIG_PROC_FS
#include <linux/proc_fs.h>
#include <linux/seq_file.h>

static int proc_lm_show(struct seq_file *s, void *v)
{
	u32 value;

	tps65185_temperature_get(&value);
	seq_printf(s, "%d\n", value);

	return 0;
}

static int proc_lm_open(struct inode *inode, struct file *file)
{
	return single_open(file, proc_lm_show, NULL);
}

static const struct file_operations proc_lm_fops = {
	.open		= proc_lm_open,
	.read		= seq_read,
	.llseek	= seq_lseek,
	.release	= single_release,
};

static int __init lm_proc_init(void)
{
	proc_create("epdsensor", 0660, NULL, &proc_lm_fops);
	return 0;

}
late_initcall(lm_proc_init);
#endif
#endif
