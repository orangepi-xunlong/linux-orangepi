/*
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * VERSION			DATE			AUTHOR
 *
 */

#ifndef __tlsc6x_main_h__
#define __tlsc6x_main_h__

/* #define TLSC_TPD_PROXIMITY */
#define TLSC_APK_DEBUG		/* apk debugger, close:undef */
//#define TLSC_ESD_HELPER_EN	/* esd helper, close:undef */
/* #define TLSC_FORCE_UPGRADE */
/* #define TLSC_GESTRUE */
/* #define TLSC_TP_PROC_SELF_TEST */
#define TLSC_BUILDIN_BOOT
#define TLSC_ITO_TEST	/* ITO test, close:undef */

/*********************************************************/

#define CONFIG_ADF

#if 0
#define tlsc_info(x...) pr_notice("[tlsc] " x)
#define tlsc_err(x...) pr_err("[tlsc][error] " x)
#define TLSC_FUNC_ENTER() pr_notice("[tlsc]%s: Enter\n", __func__)
#define TLSC_FUNC_EXIT() pr_notice("[tlsc]%s: Exit\n", __func__)
#else
#define tlsc_info(x...)
#define tlsc_err(x...) pr_err("[tlsc][error] " x)
#define TLSC_FUNC_ENTER()
#define TLSC_FUNC_EXIT()
#endif
struct tlsc6x_platform_data {
	u32 irq_gpio_number;
	u32 reset_gpio_number;
	u32 irq_gpio_flags;
	u32 reset_gpio_flags;
	const char *vdd_name;
	u32 virtualkeys[12];
	u32 x_res_max;
	u32 y_res_max;
	u32 revert_x_flag;
	u32 revert_y_flag;
	u32 exchange_x_y_flag;
};

struct ts_event {
	u16 x1;
	u16 y1;
	u16 x2;
	u16 y2;
	u16 x3;
	u16 y3;
	u16 x4;
	u16 y4;
	u16 x5;
	u16 y5;
	u16 pressure;
	u8 touch_point;
};

struct tlsc6x_data {
	struct input_dev *input_dev;
	struct input_dev *ps_input_dev;
	struct i2c_client *client;
	struct ts_event event;
#if defined(CONFIG_ADF)
	struct notifier_block fb_notif;
#elif defined(CONFIG_HAS_EARLYSUSPEND)
	struct early_suspend early_suspend;
#endif
	struct work_struct resume_work;
	struct workqueue_struct *tp_resume_workqueue;
	int irq_gpio_number;
	int reset_gpio_number;
	int isVddAlone;
	int needKeepRamCode;
	int esdHelperFreeze;
	int irq_disabled;
	struct regulator *reg_vdd;
	struct tlsc6x_platform_data *platform_data;
};

struct tlsc6x_stest_crtra {
	unsigned short xch_n;
	unsigned short ych_n;
	unsigned short allch_n;
	unsigned short st_nor_os_L1;
	unsigned short st_nor_os_L2;
	unsigned short st_nor_os_bar;
	unsigned short st_nor_os_key;
	unsigned short m_os_nor_std;
	unsigned short ffset;
	unsigned short fsset;
	unsigned short fsbse_max;
	unsigned short fsbse_bar;
	unsigned char remap[48];
	unsigned short rawmax[48];
	unsigned short rawmin[48];
	unsigned short osSample[48];
};

/*
 * struct tlsc6x_updfile_data - upgrade file description
 * @sig:		file tag
 *	@n_cfg:	contain tp-cfg number
 * @n_match:	supported vendor number
 * @size_cfg:	tp-cfg size if exist
 * @size_boot:	boot size if exist
 */
struct tlsc6x_updfile_header {
	u32 sig;
	u32 resv;
	u32 n_cfg;
	u32 n_match;
	u32 len_cfg;
	u32 len_boot;
};

extern struct tlsc6x_data *g_tp_drvdata;
extern struct mutex i2c_rw_access;

extern unsigned int g_tlsc6x_cfg_ver;
extern unsigned int g_tlsc6x_boot_ver;
extern unsigned short g_tlsc6x_chip_code;
extern unsigned int g_needKeepRamCode;

extern int tlsc6x_tp_dect(struct i2c_client *client);
extern int tlsc6x_auto_upgrade_buidin(void);
extern int tlsc6x_load_gesture_binlib(void);
extern void tlsc6x_data_crash_deal(void);

extern int tlsx6x_update_burn_cfg(u16 *ptcfg);
extern int tlsx6x_update_running_cfg(u16 *ptcfg);
extern int tlsc6x_set_dd_mode_sub(void);
extern int tlsc6x_set_nor_mode_sub(void);
extern int tlsc6x_download_ramcode(u8 *pcode, u16 len);
extern int tlsc6x_get_os_data(unsigned short *ptdata, int len);
extern int tlsc6x_write_reg(struct i2c_client *client, u8 regaddr, u8 regvalue);
extern int tlsc6x_read_reg(struct i2c_client *client, u8 regaddr, u8 *regvalue);
extern int tlsc6x_i2c_read(struct i2c_client *client, char *writebuf,
		int writelen, char *readbuf, int readlen);
extern int tlsc6x_i2c_write(struct i2c_client *client, char *writebuf,
		int writelen);
extern int tlsc6x_i2c_read_sub(struct i2c_client *client, char *writebuf,
		int writelen, char *readbuf, int readlen);
extern int tlsc6x_i2c_write_sub(struct i2c_client *client, char *writebuf,
		int writelen);

extern int tlsc6x_tptest_init(struct i2c_client *client);
extern int tlsc6x_tptest_exit(struct i2c_client *client);

extern void tlsc6x_irq_disable(void);
extern void tlsc6x_irq_enable(void);

#if (defined TPD_AUTO_UPGRADE_PATH) || (defined TLSC_APK_DEBUG)
extern int tlsc6x_proc_cfg_update(u8 *dir, int behave);
#endif
extern void tlsc6x_tpd_reset_force(void);
extern int tlsc6x_fif_write(char *fname, u8 *pdata, u16 len);
#endif
