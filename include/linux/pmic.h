/*
 * PMIC management for epaper power control HAL
 *
 *      Copyright (C) 2009 Dimitar Dimitrov, MM Solutions
 *
 *  This file is subject to the terms and conditions of the GNU General Public
 *  License. See the file COPYING in the main directory of this archive for
 *  more details.
 *
 */

#ifndef PMIC_H
#define PMIC_H

#include <linux/kernel.h>
#include <linux/workqueue.h>
#include <linux/workqueue.h>
#include <linux/regulator/consumer.h>
#include <linux/i2c.h>


struct pmic_sess;

#define PMIC_DEFAULT_DWELL_TIME_MS	1111
#define PMIC_DEFAULT_VCOMOFF_TIME_MS	20
#define PMIC_DEFAULT_V3P3OFF_TIME_MS	-1

#if defined(FB_OMAP3EP_PAPYRUS_PM_VZERO)
  #define PAPYRUS_STANDBY_DWELL_TIME	4 /*sec*/
#else
  #define PAPYRUS_STANDBY_DWELL_TIME	0
#endif

#define	SEQ_VDD(index)		((index % 4) << 6)
#define SEQ_VPOS(index)		((index % 4) << 4)
#define SEQ_VEE(index)		((index % 4) << 2)
#define SEQ_VNEG(index)		((index % 4) << 0)

/* power up seq delay time */
#define UDLY_3ms(index)		(0x00 << ((index%4) * 2))
#define UDLY_6ms(index)		(0x01 << ((index%4) * 2))
#define UDLY_9ms(index)		(0x10 << ((index%4) * 2))
#define UDLY_12ms(index)	(0x11 << ((index%4) * 2))

/* power down seq delay time */
#define DDLY_6ms(index)		(0x00 << ((index%4) * 2))
#define DDLY_12ms(index)	(0x01 << ((index%4) * 2))
#define DDLY_24ms(index)	(0x10 << ((index%4) * 2))
#define DDLY_48ms(index)	(0x11 << ((index%4) * 2))


#define NUMBER_PMIC_REGS	10

struct pmic_driver {
	const char *id;

	int vcom_min;
	int vcom_max;
	int vcom_step;

	int (*hw_read_temperature)(struct pmic_sess *sess, int *t);
	bool (*hw_power_ack)(struct pmic_sess *sess);
	void (*hw_power_req)(struct pmic_sess *sess, bool up);

	int (*set_enable)(struct pmic_sess *sess, int enable);
	int (*set_vcom_voltage)(struct pmic_sess *sess, int vcom_mv);
	int (*set_vcom1)(struct pmic_sess *sess, uint8_t vcom1);
	int (*set_vcom2)(struct pmic_sess *sess, uint8_t vcom2);
	int (*set_vadj)(struct pmic_sess *sess, uint8_t vadj);
	int (*set_int_en1)(struct pmic_sess *sess, uint8_t int_en1);
	int (*set_int_en2)(struct pmic_sess *sess, uint8_t int_en2);
	int (*set_upseq0)(struct pmic_sess *sess, uint8_t upseq0);
	int (*set_upseq1)(struct pmic_sess *sess, uint8_t upseq1);
	int (*set_dwnseq0)(struct pmic_sess *sess, uint8_t dwnseq0);
	int (*set_dwnseq1)(struct pmic_sess *sess, uint8_t dwnseq1);
	int (*set_tmst1)(struct pmic_sess *sess, uint8_t tmst1);
	int (*set_tmst2)(struct pmic_sess *sess, uint8_t tmst2);

	int (*set_vp_adjust)(struct pmic_sess *sess, uint8_t vp_adjust);
	int (*set_vn_adjust)(struct pmic_sess *sess, uint8_t vn_adjust);
	int (*set_vcom_adjust)(struct pmic_sess *sess, uint8_t vcom_adjust);
	int (*set_pwr_seq0)(struct pmic_sess *sess, uint8_t pwr_seq0);
	int (*set_pwr_seq1)(struct pmic_sess *sess, uint8_t pwr_seq1);
	int (*set_pwr_seq2)(struct pmic_sess *sess, uint8_t pwr_seq2);
	int (*set_tmst_config)(struct pmic_sess *sess, uint8_t tmst_config);
	int (*set_tmst_os)(struct pmic_sess *sess, uint8_t tmst_os);
	int (*set_tmst_hyst)(struct pmic_sess *sess, uint8_t tmst_hyst);

	int (*hw_vcom_switch)(struct pmic_sess *sess, bool state);

	int (*hw_set_dvcom)(struct pmic_sess *sess, int state);

//	int (*hw_init)(struct pmic_sess *pmsess, struct pmic_config *config, struct i2c_client *client);
	void (*hw_cleanup)(struct pmic_sess *sess);

	bool (*hw_standby_dwell_time_ready)(struct pmic_sess *sess);
	int (*hw_pm_sleep)(struct pmic_sess *sess);
	int (*hw_pm_resume)(struct pmic_sess *sess);


};

typedef enum {
	MASTER_PMIC_ID = 0,
	SLAVE_PMIC_ID = 1,
	MAX_PMIC_NUM = 2
} PMIC_ID;


struct pmic_sess {
	PMIC_ID id;
	bool powered;
	struct delayed_work powerdown_work;
	unsigned int dwell_time_ms;
	struct delayed_work vcomoff_work;
	unsigned int vcomoff_time_ms;
	int v3p3off_time_ms;
	const struct pmic_driver *drv;
	void *drvpar;
	int temp_man_offset;
	int revision;
	int is_inited;
};

struct pmic_config {
	u32 used_flag;
	char name[64];
	u32 twi_id;
	u32 twi_addr;
	struct device *dev;
	int wake_up_pin;
	int vcom_ctl_pin;
	int power_up_pin;
	int power_good_pin;
	int int_pin;
	char pmic_power_name[64];
};

struct pmic_mgr {
	struct pmic_config config[MAX_PMIC_NUM];
	struct pmic_sess sess[MAX_PMIC_NUM];
};

extern int pmic_probe(struct pmic_sess **sess, const char *id,
					unsigned int dwell_time_ms,
					unsigned int vcomoff_time_ms,
					int v3p3_time_ms);

extern void pmic_remove(struct pmic_sess **sess);

extern int pmic_set_registers_papyrus_1(struct pmic_sess *sess, uint8_t *vals);
extern int pmic_set_registers_papyrus_2(struct pmic_sess *sess, uint8_t *vals);
/*
 * Set VCOM voltage, in millivolts.
 * NB! Change will take effect with the next pmic power up!
 */
extern int pmic_set_vcom_voltage(struct pmic_sess *sess, int vcom_mv);

/*Runtame Change VCOM state */
extern int pmic_set_dvcom(struct pmic_sess *sess, int state);

/* Request asynchronous power up. */
extern int pmic_req_powerup(struct pmic_sess *sess);

/* Ensure that power is OK. Called after a pmic_req_powerup(). */
extern int pmic_sync_powerup(struct pmic_sess *sess);

/* Get pmic temperature sensor measurement. */
extern int pmic_get_temperature(struct pmic_sess *sess, int *t);

/*
 * Asynchronously release the power up requirement. Power may go down
 * up to a few seconds after this call in order to avoid power up/down
 * cycles with frequent screen updates.
 */
extern void pmic_release_powerup_req(struct pmic_sess *sess);

/* Get that enough delay between standby and sleep*/
extern bool pmic_standby_dwell_time_ready(struct pmic_sess *sess);

extern void pmic_pm_sleep(struct pmic_sess *sess);

extern void pmic_pm_resume(struct pmic_sess *sess);
#endif	/* PMIC_H */
