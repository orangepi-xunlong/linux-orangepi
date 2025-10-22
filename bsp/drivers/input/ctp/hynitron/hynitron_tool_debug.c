/*
 *Name        : hynintron_ex_fun.c
 *Author      : steven
 *Version     : V2.0
 *Create      : 2019-11-11
 *Copyright   : zxzz
 */

#include <asm/uaccess.h>
#include <asm/unistd.h>
#include <linux/delay.h>
#include <linux/gpio.h>
#include <linux/mount.h>
#include <linux/netdevice.h>
#include <linux/string.h>
#include <linux/syscalls.h>

#include <linux/file.h>
#include <linux/fs.h>
#include <linux/proc_fs.h>

//#include <linux/stdlib.h>

#include "hynitron_core.h"
#include "hynitron_update_firmware.h"

#define SWITCH_ESD_OFF 0
#define SWITCH_ESD_ON 1

#define HYN_FACTORY_TEST_LOG_TXT "/sdcard/hyn_factory_test.txt"
#define HYN_TP_OPEN_VALUE_MAX 2100
#define HYN_TP_OPEN_VALUE_MIN 700
#define HYN_TP_OPEN_LOW_VALUE_MIN 500
#define HYN_TP_OPEN_DRV_VALUE_MIN 1200
#define HYN_TP_OPEN_DROP_RATIO 15
#define HYN_TP_OPEN_TX_DROP_RATIO 18
#define HYN_TP_SHORT_RX_TX_RESISTANCE 600

#if HYN_AUTO_FACTORY_TEST_EN
char factory_test[512];
int hyn_factory_touch_test(void);
void hynitron_wait_event_wait(void);

uint16 open_value_max;
uint16 open_value_min;
uint16 open_drv_value_min;
uint8 open_drop_ratio;
uint8 open_tx_drop_ratio;

int hyn_factory_test_judge(unsigned char *pdata) {
	unsigned int open_low, open_high, short_value;
	unsigned int open_delta, open_delta_last, open_delta_tx_last;
	unsigned short rx, tx, rx_num, tx_num;
	unsigned char *p_data;

	int test_fail_flag = 0;
	open_value_max = HYN_TP_OPEN_VALUE_MAX;
	open_value_min = HYN_TP_OPEN_VALUE_MIN;
	open_drv_value_min = HYN_TP_OPEN_DRV_VALUE_MIN;

	open_drop_ratio = HYN_TP_OPEN_DROP_RATIO;
	open_tx_drop_ratio = HYN_TP_OPEN_TX_DROP_RATIO;

	struct file *fp;
	mm_segment_t fs;
	loff_t pos;
	struct inode *inode;
	unsigned long magic;
	int length;
	unsigned int data_len;
	unsigned char p_test_data[80];
	// int length;

	p_data = pdata;
	test_fail_flag = 0;

	open_delta = 1;
	open_delta_last = 1;
	open_delta_tx_last = 1;

	HYN_DEBUG("hyn_factory_test_judge enter.\n");

	data_len = hyn_ts_data->sensor_tx * hyn_ts_data->sensor_rx * 4 +
		   (hyn_ts_data->sensor_tx + hyn_ts_data->sensor_tx) * 2;
	HYN_DEBUG("hyn_factory_test_judge, data_len=%d, sensor_tx=%d, "
		  "sensor_rx=%d.\n",
		  data_len, hyn_ts_data->sensor_tx, hyn_ts_data->sensor_rx);

	rx_num = hyn_ts_data->sensor_rx;
	tx_num = hyn_ts_data->sensor_tx;
	HYN_DEBUG(" detect sensor:tx_num:%d, rx_num:%d .\n", tx_num, rx_num);
	HYN_DEBUG(" detect sensor:open_max:%d, open_min:%d, open_drv_min:%d.\n",
		  open_value_max, open_value_min, open_drv_value_min);

	fp = filp_open(HYN_FACTORY_TEST_LOG_TXT, O_RDWR | O_CREAT, 0644);
	if (IS_ERR(fp)) {
		HYN_ERROR("create test data file error.\n");
		// return -1;
	} else {
		inode = fp->f_inode;
		magic = inode->i_sb->s_magic;
		pos = inode->i_size;

		memset((unsigned char *)p_test_data, 0x00, 80);
		length = 80;
		fs = get_fs();
		set_fs(KERNEL_DS);
		pos = fp->f_pos;
		snprintf(p_test_data, 80, "touch_test enter.\n");
		length = strlen(p_test_data);
		length += 1;
		vfs_write(fp, p_test_data, length, &pos);

		// update pos num
		snprintf(p_test_data, 80,
			 "test param:open_max=%d, open_min=%d, high_min=%d, "
			 "ratio=%d, "
			 "tx_ratio=%d.\n",
			 open_value_max, open_value_min, open_drv_value_min,
			 open_drop_ratio, open_tx_drop_ratio);
		length = strlen(p_test_data);
		length += 1;
		vfs_write(fp, p_test_data, length, &pos);
	}

	for (tx = 0; tx < tx_num; tx++) {
		for (rx = 0; rx < rx_num; rx++) {
			open_low = ((*(p_data + 1)) << 8) + (*p_data);
			open_high =
				((*(p_data + 1 + tx_num * rx_num * 2)) << 8) +
				*(p_data + tx_num * rx_num * 2);
			open_delta = open_high - open_low;

			if (!IS_ERR(fp)) {
				snprintf(p_test_data, 80,
					 "tx=%d, rx=%d, open_low=%d, "
					 "open_high=%d, open_delta=%d.\n",
					 tx, rx, open_low, open_high,
					 open_delta);
				length = strlen(p_test_data);
				length += 1;
				vfs_write(fp, p_test_data, length, &pos);
			}
			if (open_high < open_drv_value_min) {
				HYN_DEBUG("factory_test_judge test open fail, "
					  "tx=%d, rx=%d, open_high=%d.\n",
					  tx, rx, open_high);
				sprintf(factory_test,
					"0, factory_test_judge test open fail, "
					"tx=%d, rx=%d, "
					"open_high=%d.\n",
					tx, rx, open_high);
				test_fail_flag = -1;
			}
			if ((open_delta < open_value_min) ||
			    (open_delta > open_value_max)) {
				HYN_DEBUG("factory_test_judge test open fail, "
					  "tx=%d, rx=%d, open_delta=%d.\n",
					  tx, rx, open_delta);
				sprintf(factory_test,
					"0, factory_test_judge test open fail, "
					"tx=%d, rx=%d, "
					"open_delta=%d.\n",
					tx, rx, open_delta);
				test_fail_flag = -1;
			}
			if (rx > 0) {
				if (((open_delta_last * open_drop_ratio / 10) <
				     open_delta) ||
				    ((open_delta_last * 10 / open_drop_ratio) >
				     open_delta)) {
					HYN_DEBUG("factory_test_judge test "
						  "open fail, tx=%d, rx=%d, "
						  "open_delta=%d.open_delta_"
						  "last=%d.\n",
						  tx, rx, open_delta,
						  open_delta_last);
					sprintf(factory_test,
						"0, factory_test_judge test "
						"open fail, tx=%d, rx=%d, "
						"open_delta=%d, "
						"open_delta_last=%d.\n",
						tx, rx, open_delta,
						open_delta_last);
					test_fail_flag = -1;
				}
			}
			if (tx > 0) {
				unsigned int low, high, delta;
				low = ((*(p_data + 1 - rx_num * 2)) << 8) +
				      (*(p_data - rx_num * 2));
				high = ((*(p_data + 1 +
					   (tx_num - 1) * rx_num * 2))
					<< 8) +
				       (*(p_data + (tx_num - 1) * rx_num * 2));
				delta = high - low;
				open_delta_tx_last = delta;

				if (((open_delta_tx_last * open_tx_drop_ratio /
				      10) < open_delta) ||
				    ((open_delta_tx_last * 10 /
				      open_tx_drop_ratio) > open_delta)) {
					HYN_DEBUG("factory_test_judge test "
						  "open fail, tx=%d, rx=%d, "
						  "open_delta=%d.open_delta_tx_"
						  "last=%d.\n",
						  tx, rx, open_delta,
						  open_delta_tx_last);
					sprintf(factory_test,
						"0, factory_test_judge test "
						"open fail, tx=%d, rx=%d, "
						"open_delta=%d, "
						"open_delta_tx_last=%d.\n",
						tx, rx, open_delta,
						open_delta_tx_last);
					test_fail_flag = -1;
				}
			}
			open_delta_last = open_delta;
			p_data += 2;
		}
	}
	HYN_DEBUG("factory_test_judge test short.\n");
	p_data = pdata + (hyn_ts_data->sensor_tx * hyn_ts_data->sensor_rx * 4);
	rx = 0;
	for (tx = 0; tx < (hyn_ts_data->sensor_tx + hyn_ts_data->sensor_rx);
	     tx++) {
		short_value = ((*(p_data + 1)) << 8) + (*p_data);
		short_value = (2000 / (short_value));
		if (!IS_ERR(fp)) {
			snprintf(p_test_data, 80,
				 "tx=%d, rx=%d, short_value=%d.\n", tx, rx,
				 short_value);
			length = strlen(p_test_data);
			length += 1;
			vfs_write(fp, p_test_data, length, &pos);
		}
		if (short_value < HYN_TP_SHORT_RX_TX_RESISTANCE) {
			HYN_DEBUG("factory_test_judge test short fail, tx=%d, "
				  "short_value=%d.\n",
				  tx, short_value);
			sprintf(factory_test,
				"0, factory_test_judge test short fail, tx=%d, "
				"short_value=%d.\n",
				tx, short_value);
			test_fail_flag = -1;
		}
		p_data += 2;
	}

	if (test_fail_flag == 0) {
		sprintf(factory_test, "1, factory_test_judge pass=1.\n");
		if (!IS_ERR(fp)) {
			snprintf(p_test_data, 80,
				 "1, factory_test_judge psss=1.\n");
			length = strlen(p_test_data);
			length += 1;
			vfs_write(fp, p_test_data, length, &pos);
		}
	} else {
		sprintf(factory_test, "0, factory_test_judge fail=0.\n");
		if (!IS_ERR(fp)) {
			snprintf(p_test_data, 80,
				 "0, factory_test_judge fail=0.\n");
			length = strlen(p_test_data);
			length += 1;
			vfs_write(fp, p_test_data, length, &pos);
		}
	}
	if (!IS_ERR(fp)) {
		snprintf(p_test_data, 80,
			 "******hyn_factory_test_judge done*****.\n");
		length = strlen(p_test_data);
		length += 1;
		vfs_write(fp, p_test_data, length, &pos);

		fp->f_pos = pos;
		set_fs(fs);
		filp_close(fp, NULL);
	}

	HYN_DEBUG("factory_test_judge test end, test_fail_flag=%d .\n",
		  test_fail_flag);
	return test_fail_flag;
}

int hyn_factory_touch_test(void) {
	unsigned char *buf;
	int ret;
	unsigned short rx, tx;
	int data_len = 0;
	unsigned char buf1[5];

	HYN_DEBUG("touch_test enter.\n");

	memset((unsigned char *)buf1, 0, 5);

	buf1[0] = 0x00;
	buf1[1] = 0x05;
	buf1[2] = 0x00;
	ret = cst3xx_i2c_write(hyn_ts_data->client, buf1, 3);
	if (ret < 0) {
		HYN_ERROR("cst3xx_i2c_write 0X000500.error:%d.\n", ret);
		return -1;
	}

	if (hyn_ts_data->sensor_rx == 0 || hyn_ts_data->sensor_tx == 0) {
		buf1[0] = 0xD1;
		buf1[1] = 0x01;
		ret = cst3xx_i2c_write(hyn_ts_data->client, buf1, 2);
		if (ret < 0) {
			HYN_ERROR("touch_test enter debug_info "
				  "failed.error:%d.\n",
				  ret);
			return -1;
		}

		mdelay(10);

		buf1[0] = 0xD1;
		buf1[1] = 0xF4;
		ret = cst3xx_i2c_read_register(hyn_ts_data->client, buf1, 4);
		if (ret < 0) {
			HYN_ERROR("touch_test read TX/RX failed.error:%d.\n",
				  ret);
			return -1;
		}

		hyn_ts_data->sensor_tx = buf1[0];
		hyn_ts_data->sensor_rx = buf1[2];
	}
	rx = hyn_ts_data->sensor_rx;
	tx = hyn_ts_data->sensor_tx;

	data_len = rx * tx * 4 + (tx + rx + 4) * 2;

	HYN_DEBUG("hyn_factory_touch_test, data_len=%d, sensor_tx=%d, "
		  "sensor_rx=%d.\n",
		  data_len, hyn_ts_data->sensor_tx, hyn_ts_data->sensor_rx);

	buf = kzalloc(sizeof(char) * data_len, GFP_KERNEL);
	if (buf == NULL) {
		HYN_ERROR("touch_test GFP_KERNEL memory fail.\n");
		return -1;
	}
	hyn_ts_data->work_mode = HYN_WORK_MODE_FACTORY;

	buf1[0] = 0xD1;
	buf1[1] = 0x19;
	data_len = rx * tx * 2;

	ret = cst3xx_i2c_write(hyn_ts_data->client, buf1, 2);
	if (ret < 0) {
		HYN_ERROR("touch_test factory test mode failed.error:%d.\n",
			  ret);
		return -1;
	}
	mdelay(14);
	hyn_ts_data->hyn_irq_flag = 0;
	hynitron_wait_event_wait();
	hyn_ts_data->hyn_irq_flag = 0;

	// 1xx data
	// buf[0] = 0x12;
	// buf[1] = 0x15;

	// 3xx_data
	buf1[0] = 0x10;
	buf1[1] = 0x00;
	ret = cst3xx_i2c_write(hyn_ts_data->client, buf1, 2);
	if (ret < 0) {
		HYN_ERROR("touch_test write register failed.error:%d.\n", ret);
		return -1;
	}

	ret = cst3xx_i2c_read(hyn_ts_data->client, &buf[2], data_len);
	if (ret < 0) {
		HYN_ERROR("touch_test read fatory data failed.error:%d.\n",
			  ret);
		return -1;
	}
	// HighDrv_Data
	buf1[0] = 0x30;
	buf1[1] = 0x00;
	ret = cst3xx_i2c_write(hyn_ts_data->client, buf1, 2);
	if (ret < 0) {
		HYN_ERROR("touch_test write register failed.error:%d.\n", ret);
		return -1;
	}
	ret = cst3xx_i2c_read(hyn_ts_data->client, &buf[2 + data_len],
			      data_len);
	if (ret < 0) {
		HYN_ERROR("touch_test read fatory data failed.error:%d.\n",
			  ret);
		return -1;
	}
	// short
	buf1[0] = 0x50;
	buf1[1] = 0x00;
	data_len = (rx + tx) * 2;
	ret = cst3xx_i2c_write(hyn_ts_data->client, buf1, 2);
	if (ret < 0) {
		HYN_ERROR("touch_test write register failed.error:%d.\n", ret);
		return -1;
	}
	ret = cst3xx_i2c_read(hyn_ts_data->client, &buf[2 + (rx * tx * 4)],
			      data_len);
	if (ret < 0) {
		HYN_ERROR("touch_test read fatory data failed.error:%d.\n",
			  ret);
		return -1;
	}

	ret = hyn_factory_test_judge((unsigned char *)&buf[2]);
	if (ret < 0) {
		HYN_ERROR("hyn_factory_test_judge failed.error:%d.\n", ret);
		return -1;
	} else {
		HYN_DEBUG("hyn_factory_test_judge. success .\n");
		return 0;
	}
}

#endif

#if ANDROID_TOOL_SURPORT
static struct mutex g_procrw_mutex;
static DEFINE_MUTEX(g_procrw_mutex);

#define HYNITRON_PROC_DIR_NAME "cst1xx_ts"
#define HYNITRON_PROC_FILE_NAME "cst1xx-update"

#define HYNITRON_PROC_DIR_NAME_2 "cst8xx_ts"
#define HYNITRON_PROC_FILE_NAME_2 "cst8xx-update"

extern int hyn_boot_update_fw(struct i2c_client *client);
extern int cst3xx_firmware_info(struct i2c_client *client);

DECLARE_WAIT_QUEUE_HEAD(debug_waiter);
static struct proc_dir_entry *g_proc_dir;
static struct proc_dir_entry *g_update_file;
static int CMDIndex = 0;

static struct file *hynitron_open_fw_file(char *path, mm_segment_t *old_fs_p) {
	struct file *filp;
	int ret;

	*old_fs_p = get_fs();
	// set_fs(KERNEL_DS);
	filp = filp_open(path, O_RDONLY, 0);
	if (IS_ERR(filp)) {
		ret = PTR_ERR(filp);
		HYN_INFO("hynitron_open_fw_file:filp_open error.\n");
		return NULL;
	}
	filp->f_op->llseek(filp, 0, 0);

	return filp;
}

static void hynitron_close_fw_file(struct file *filp, mm_segment_t old_fs) {
	// set_fs(old_fs);
	if (filp)
		filp_close(filp, NULL);
}

static int hynitron_read_fw_file(unsigned char *filename, unsigned char **pdata,
				 int *plen) {
	struct file *fp;
	mm_segment_t old_fs;
	// int size;
	// int length;
	int ret = -1;
	loff_t pos;
	off_t fsize;
	struct inode *inode;
	unsigned long magic;

	HYN_INFO("hynitron_read_fw_file enter.\n");

	if ((pdata == NULL) || (strlen(filename) == 0))
		return ret;

	fp = hynitron_open_fw_file(filename, &old_fs);
	if (fp == NULL) {
		HYN_INFO("Open bin file faild.path:%s.\n", filename);
		goto clean;
	}

	if (IS_ERR(fp)) {
		HYN_INFO("error occured while opening file %s.\n", filename);
		return -EIO;
	}
	inode = fp->f_inode;
	magic = inode->i_sb->s_magic;
	fsize = inode->i_size;
	*pdata = (unsigned char *)vmalloc(fsize);
	old_fs = get_fs();
	set_fs(KERNEL_DS);
	pos = 0;
	ret = vfs_read(fp, *pdata, fsize, &pos);

	if (ret == fsize) {
		HYN_INFO("vfs_read success.ret:%d.\n", ret);
	} else {
		HYN_INFO("vfs_read fail.ret:%d.\n", ret);
	}
	filp_close(fp, NULL);
	set_fs(old_fs);

	HYN_INFO("vfs_read done.\n");

clean:
	hynitron_close_fw_file(fp, old_fs);
	return ret;
}
static int hynitron_apk_fw_dowmload(struct i2c_client *client,
				    unsigned char *pdata, int length) {

	hyn_ts_data->apk_upgrade_flag = 1;
	hyn_ts_data->p_hynitron_upgrade_firmware = (unsigned char *)pdata;
	HYN_INFO("hynitron_apk_fw_dowmload enter.\n");
	hyn_boot_update_fw(client);
	hyn_ts_data->apk_upgrade_flag = 0;
	return 0;
}
void hynitron_wake_up_wait(void) {
	if (hyn_ts_data->work_mode > 0) {
		wake_up_interruptible(&debug_waiter);
		hyn_ts_data->hyn_irq_flag = 1;
		HYN_INFO("hynitron_wake_up_wait "
			 "exit********hyn_ts_data->hyn_irq_flag:%d. \n",
			 hyn_ts_data->hyn_irq_flag);
	}
}
void hynitron_wait_event_wait(void) {
	HYN_FUNC_ENTER();
	if (hyn_ts_data->hyn_irq_flag == 0) // if irq had happend
		wait_event_interruptible(debug_waiter,
					 hyn_ts_data->hyn_irq_flag != 0);
	HYN_FUNC_EXIT();
}

int hynitron_touch_setmode(struct i2c_client *client, int mode, int isMult) {
	unsigned char buf[2];
	int ret;
	HYN_FUNC_ENTER();
	hyn_ts_data->work_mode = mode;
	hyn_ts_data->hyn_irq_flag = 0;

	buf[0] = isMult ? 0xD1 : 0x00;
	switch (mode) {
	case HYN_WORK_MODE_DIFF:
		buf[1] = isMult ? 0x0D : 0x07;
		break;
	case HYN_WORK_MODE_RAWDATA:
		buf[1] = isMult ? 0x0A : 0x06;
		break;
	case HYN_WORK_MODE_FACTORY:
		buf[1] = isMult ? 0x19 : 0x04;
		break;
	default: // reset
		buf[1] = isMult ? 0x09 : 0x00;
		break;
	}

	if (isMult)
		ret = i2c_master_send(client, buf, 2);
	else
		ret = hyn_i2c_write(client, buf, 2);
	if (ret < 0) {
		HYN_ERROR(
			" cst3xx Write command normal mode failed.error:%d.\n",
			ret);
	}
	HYN_FUNC_EXIT();

	return ret;
}

#define RESET_IC 0x0A
#define WRITE_IIC 0x1A
#define READ_IIC 0x2A
#define DELAY_MS 0x3A

static unsigned char g_cmdpack[2048];
static unsigned int g_cmdlen;
static char g_cmdret[258];
static char g_exc_flag = 0;
static struct task_struct *gExchandle = NULL;
static struct i2c_client *gExClient = NULL;
static DECLARE_WAIT_QUEUE_HEAD(run_cmd_waiter);
static int hyn_exc_cmdpack(void *unused) //(void *iic_client)
{
	/*
		---------------------------------------------------------------------------
		|head|retry|retrydelay|type|lenL|lenH|data|...|type|lenL|lenH|data|.......|
		---------------------------------------------------------------------------
	*/
	unsigned char readbuf[256];

	unsigned char retry_times, loop;
	unsigned char retry_delay;

	// struct i2c_client *client = (struct i2c_client *)iic_client;

	HYN_FUNC_ENTER();

	while (!kthread_should_stop()) {
		int run_state = -1;
		schedule_timeout(1);

		g_exc_flag = 0;
		wait_event(run_cmd_waiter, g_exc_flag != 0);

		g_cmdret[0] = -1;

		retry_times = g_cmdpack[1];
		retry_delay = g_cmdpack[2];

		for (loop = 0; loop < (retry_times + 1); loop++) {
			int cur_pos = 3;
			g_cmdret[1] = 0;

			/*debug*/
			HYN_INFO("in cmdpack retry_times = %d retry_delay = %d "
				 "cmd = %d\n",
				 retry_times, retry_delay, g_cmdpack[cur_pos]);
			/*debug*/

			while ((g_cmdpack[cur_pos] == RESET_IC ||
				g_cmdpack[cur_pos] == WRITE_IIC ||
				g_cmdpack[cur_pos] == READ_IIC ||
				g_cmdpack[cur_pos] == DELAY_MS) &&
			       (cur_pos < (g_cmdlen - 1))) {
				unsigned char tmp_addr;
				unsigned char comp_condition = 0;
				unsigned char cmd_type = g_cmdpack[cur_pos];
				unsigned int data_len =
					(g_cmdpack[cur_pos + 1] |
					 (g_cmdpack[cur_pos + 2] << 8));
				/*debug*/
				HYN_INFO("cur_pos= %d g_cmdlen = %d \n",
					 cur_pos, g_cmdlen);
				HYN_INFO(" cmd_type = 0x%x datalen = %d\n",
					 cmd_type, data_len);
				/*debug*/

				if (data_len == 0)
					break;

				switch (cmd_type) {
				case RESET_IC:
					run_state = hyn_reset_proc(
						g_cmdpack[cur_pos + 3]);
					break;
				case WRITE_IIC:
					tmp_addr = gExClient->addr;
					gExClient->addr =
						g_cmdpack[cur_pos + 3];
					run_state = hyn_i2c_write(
						gExClient,
						&g_cmdpack[cur_pos + 4],
						data_len - 1);
					gExClient->addr = tmp_addr;
					break;
				case READ_IIC:
					tmp_addr = gExClient->addr;
					gExClient->addr =
						g_cmdpack[cur_pos + 3];
					comp_condition = g_cmdpack[cur_pos + 6];
					run_state = hyn_i2c_read(
						gExClient,
						&g_cmdpack[cur_pos + 4], 2,
						readbuf,
						data_len -
							4); // 4// 4  iic_addr +
							    // regH + regL +
							    // comp_condition
					gExClient->addr = tmp_addr;
					if (run_state >= 0) {
						int j = 0;
						HYN_INFO(" compare len = %d\n",
							 data_len - 4);

						if ((unsigned char)g_cmdret[1] <
						    255) {
							int cp_len =
								(data_len - 4 +
									 g_cmdret[1] >
								 255)
									? (255 -
									   g_cmdret[1])
									: (data_len -
									   4);
							memcpy(&g_cmdret
								       [(int)g_cmdret
										[1] +
									2],
							       readbuf, cp_len);
							g_cmdret[1] += cp_len;
						}

						// check read data
						for (j = 0; j < data_len - 4;
						     j++) {
							HYN_INFO(
								" cp %d = %d\n",
								readbuf[j],
								g_cmdpack
									[cur_pos +
									 7 +
									 j]);
							if (comp_condition ==
							    0) // need ecqual
							{
								if (readbuf[j] !=
								    g_cmdpack
									    [cur_pos +
									     7 +
									     j]) {
									run_state =
										-1;
									break;
								}
							} else if (
								comp_condition ==
								1) // need not
								   // ecqual
							{
								if (readbuf[j] ==
								    g_cmdpack
									    [cur_pos +
									     7 +
									     j]) {
									run_state =
										-1;
									break;
								}
							}
						}
					}
					break;
				case DELAY_MS:
					mdelay(g_cmdpack[cur_pos + 3]);
					run_state = 0;
					break;
				default:
					run_state = -1;
					break;
				}

				/*debug*/
				HYN_INFO("run_state = %d\n", run_state);
				/*debug*/

				if (run_state < 0)
					break;
				else
					cur_pos += data_len + 3;

				HYN_INFO("update cur_pos= %d \n", cur_pos);

				if (cur_pos >= g_cmdlen) {
					/*debug*/
					HYN_INFO("cur_pos >= g_cmdlen\n");
					/*debug*/
					break;
				}
			}

			if (run_state >= 0)
				break;
			else if (loop < retry_times) {
				HYN_INFO("retry---\n");
				mdelay(retry_delay);
			}
		}

		g_cmdret[0] = (run_state < 0) ? -1 : 0;
		/*debug*/
		HYN_INFO("in cmdpack g_cmdret = %d \n", g_cmdret[0]);
		/*debug*/
	}

	HYN_FUNC_EXIT();

	return g_cmdret[0];
}

static ssize_t hynitron_proc_read_foobar(struct file *page,
					 char __user *user_buf, size_t count,
					 loff_t *f_pos) {
	unsigned char *buf = NULL;
	int ret_len = 0;
	int ret = 0;
	unsigned short rx, tx;
	int data_len;

	struct i2c_client *client =
		(struct i2c_client *)PDE_DATA(file_inode(page));
	mutex_lock(&g_procrw_mutex);
	HYN_INFO("hynitron_proc_read_foobar********CMDIndex:%d. \n", CMDIndex);

	if (*f_pos != 0)
		goto END;
#if HYN_ESDCHECK_EN
	hyn_esd_switch(SWITCH_ESD_OFF);
#endif

	rx = hyn_ts_data->sensor_rx;
	tx = hyn_ts_data->sensor_tx;

	data_len = rx * tx * 4 + (tx + rx + 4) * 2;
	if (data_len < 2500)
		data_len = 2500;
	HYN_DEBUG("hyn_factory_touch_test, data_len=%d, sensor_tx=%d, "
		  "sensor_rx=%d.\n",
		  data_len, hyn_ts_data->sensor_tx, hyn_ts_data->sensor_rx);

	buf = kzalloc(sizeof(char) * data_len, GFP_KERNEL);
	if (buf == NULL) {
		HYN_INFO("zalloc GFP_KERNEL memory fail.\n");
		return -ENOMEM;
	}
	if (hyn_ts_data->config_chip_product_line ==
	    HYN_CHIP_PRODUCT_LINE_MUT_CAP) {

		if (CMDIndex == 0) {

			sprintf(buf,
				"Hynitron MUTCAP touchscreen driver 1.0.\n");
			// strcpy(page, buf);
			ret_len = strlen(buf);
			ret = copy_to_user(user_buf, buf, ret_len);
			if (ret < 0)
				goto END;
		} else if (CMDIndex == 1) {

			buf[0] = 0xD1;
			buf[1] = 0x01;
			ret = cst3xx_i2c_write(client, buf, 2);
			if (ret < 0)
				goto END;

			mdelay(10);

			buf[0] = 0xD1;
			buf[1] = 0xF4;
			ret = cst3xx_i2c_read_register(client, buf, 28);
			if (ret < 0)
				goto END;

			hyn_ts_data->sensor_tx = buf[0];
			hyn_ts_data->sensor_rx = buf[2];

			HYN_INFO("  cst3xx_proc_read_foobar:g_cst3xx_tx:%d, "
				 "g_cst3xx_rx:%d.\n",
				 hyn_ts_data->sensor_tx,
				 hyn_ts_data->sensor_rx);

			ret_len = 28;
			ret = copy_to_user(user_buf, buf, ret_len);

			buf[0] = 0xD1;
			buf[1] = 0x09;
			ret = cst3xx_i2c_write(client, buf, 2);

		} else if (CMDIndex == 2 || CMDIndex == 3 || CMDIndex == 4) {
			unsigned short rx, tx;

			if (hyn_ts_data->sensor_rx == 0 ||
			    hyn_ts_data->sensor_tx == 0) {
				buf[0] = 0xD1;
				buf[1] = 0x01;
				ret = cst3xx_i2c_write(client, buf, 2);
				if (ret < 0)
					goto END;

				mdelay(10);

				buf[0] = 0xD1;
				buf[1] = 0xF4;
				ret = cst3xx_i2c_read_register(client, buf, 4);
				if (ret < 0)
					goto END;

				hyn_ts_data->sensor_tx = buf[0];
				hyn_ts_data->sensor_rx = buf[2];
			}

			rx = hyn_ts_data->sensor_rx;
			tx = hyn_ts_data->sensor_tx;

			if (CMDIndex == 2) // read diff
				ret = (hyn_ts_data->work_mode !=
				       HYN_WORK_MODE_DIFF)
					      ? hynitron_touch_setmode(
							client,
							HYN_WORK_MODE_DIFF, 1)
					      : 0;
			else if (CMDIndex == 3) // rawdata
				ret = (hyn_ts_data->work_mode !=
				       HYN_WORK_MODE_RAWDATA)
					      ? hynitron_touch_setmode(
							client,
							HYN_WORK_MODE_RAWDATA,
							1)
					      : 0;
			else if (CMDIndex == 4) // factory test
				ret = (hyn_ts_data->work_mode !=
				       HYN_WORK_MODE_RAWDATA)
					      ? hynitron_touch_setmode(
							client,
							HYN_WORK_MODE_FACTORY,
							1)
					      : 0;

			if (ret < 0) {
				HYN_ERROR(" cst3xx Write command raw/diff mode "
					  "failed.error:%d.\n",
					  ret);
				goto END;
			}

			hynitron_wait_event_wait();
			hyn_ts_data->hyn_irq_flag = 0;

			HYN_INFO(" cst3xx Read wait_event interrupt");
			if (hyn_ts_data->config_chip_series ==
				    HYN_CHIP_CST3XX ||
			    hyn_ts_data->config_chip_series ==
				    HYN_CHIP_CST1XX ||
			    hyn_ts_data->config_chip_series ==
				    HYN_CHIP_CST6XX ||
			    hyn_ts_data->config_chip_series ==
				    HYN_CHIP_CST644K) {

				int read_len = 0;
				int start_pos = 0;

				buf[0] = 0x10;
				buf[1] = 0x00;

				start_pos = 2;
				read_len = rx * tx * 2;

				if (cst3xx_i2c_write(client, buf, 2) < 0 ||
				    cst3xx_i2c_read(client, &buf[start_pos],
						    read_len) < 0) {
					ret = -1;
					HYN_ERROR(" cst3xx Write command1 "
						  "fail\n");
					goto END;
				}

				if (CMDIndex == 3) // rawdata
				{
					char tmpBuf[10];
					tmpBuf[0] = 0x00;
					tmpBuf[1] = 0x00;

					if (cst3xx_i2c_write(client, tmpBuf,
							     2) < 0 ||
					    cst3xx_i2c_read(client, &tmpBuf[2],
							    5) < 0) {
						ret = -1;
						HYN_ERROR(" cst3xx Write "
							  "command2 fail\n");
						goto END;
					}

					HYN_INFO(" sizeof(rawdata_type) = %d\n",
						 tmpBuf[6]);
					if (tmpBuf[6] ==
					    1) // rawdata_type = uint8  need
					       // resize buf
					{
						int i;
						HYN_INFO("resize buf\n");
						for (i = (rx * tx - 1); i >= 0;
						     i--) {
							buf[start_pos + i * 2 +
							    1] = 0x00;
							buf[start_pos + i * 2] =
								buf[start_pos +
								    i];
						}
					}
				}

				if (CMDIndex == 4) {
					buf[0] = 0x30;
					buf[1] = 0x00;

					start_pos += read_len;
					read_len = rx * tx * 2;

					if (cst3xx_i2c_write(client, buf, 2) <
						    0 ||
					    cst3xx_i2c_read(client,
							    &buf[start_pos],
							    read_len) < 0) {
						ret = -1;
						HYN_ERROR(" cst3xx Write "
							  "command3 fail\n");
						goto END;
					}

					buf[0] = 0x50;
					buf[1] = 0x00;

					start_pos += read_len;
					read_len = (rx + tx) * 2;

					if (cst3xx_i2c_write(client, buf, 2) <
						    0 ||
					    cst3xx_i2c_read(client,
							    &buf[start_pos],
							    read_len) < 0) {
						ret = -1;
						HYN_ERROR(" cst3xx Write "
							  "command fail\n");
						goto END;
					}
				}

				ret_len = start_pos + read_len;

				buf[ret_len] = 0x00; // buf too short
				buf[ret_len + 1] = 0x05;
				buf[ret_len + 2] = 0x00;
				ret = cst3xx_i2c_write(client, &buf[ret_len],
						       3);
				if (ret < 0) {
					HYN_ERROR("clear irq flag Faile!!\n");
					goto END;
				}

				buf[0] = rx;
				buf[1] = tx;

			} else {
				int start_pos = 2;
				int read_len = 0;
				if (CMDIndex == 2 || CMDIndex == 3) {
					buf[0] = 0x80;
					buf[1] = 0x01;
					read_len = rx * tx * 2 + 4 +
						   (tx + rx) * 2 + rx + rx;
				} else {
					buf[0] = 0x12;
					buf[1] = 0x15;
					read_len =
						rx * tx * 4 + (4 + tx + rx) * 2;
				}

				if (cst3xx_i2c_write(client, buf, 2) < 0 ||
				    cst3xx_i2c_read(client, &buf[start_pos],
						    read_len) < 0) {
					ret = -1;
					HYN_ERROR(
						" cst3xx Write command fail\n");
					goto END;
				}
				ret_len = start_pos + read_len;
			}

			// if get factory data, need reset to back idle
			if (CMDIndex == 4)
				hyn_reset_proc(10);

			ret = copy_to_user(user_buf, buf, ret_len);

			if (CMDIndex == 4) {

				buf[0] = 0xD1;
				buf[1] = 0x09;
				ret = cst3xx_i2c_write(client, buf, 2);
			}
		}

	} else if (hyn_ts_data->config_chip_product_line ==
		   HYN_CHIP_PRODUCT_LINE_SEL_CAP) {

		if (CMDIndex == 0) {
			sprintf(buf,
				"Hynitron SELCAP touchscreen driver 1.0.\n");
			// strcpy(page, buf);
			ret_len = strlen(buf);
			ret = copy_to_user(user_buf, buf, ret_len);
			if (ret < 0)
				goto END;

		} else if (CMDIndex == 1) {
			buf[0] = 0xA6;
			ret = hyn_i2c_read(client, (u8 *)buf, 1, (u8 *)buf, 8);
			if (ret < 0) {
				HYN_INFO("hynitron_proc_read_foobar "
					 "hyn_i2c_read fail. \n");
			} else {
				ret_len = 8;
				ret = copy_to_user(user_buf, buf, ret_len);
				if (ret < 0)
					goto END;
			}
		}
		if (CMDIndex == 2 || CMDIndex == 3 || CMDIndex == 4) {

			int data_len = 80;
			int report_count = 0;

			if (CMDIndex == 2) // read diff
				ret = (hyn_ts_data->work_mode !=
				       HYN_WORK_MODE_DIFF)
					      ? hynitron_touch_setmode(
							client,
							HYN_WORK_MODE_DIFF, 0)
					      : 0;
			else if (CMDIndex == 3) // rawdata
				ret = (hyn_ts_data->work_mode !=
				       HYN_WORK_MODE_RAWDATA)
					      ? hynitron_touch_setmode(
							client,
							HYN_WORK_MODE_RAWDATA,
							0)
					      : 0;
			else if (CMDIndex == 4) // factory
				ret = (hyn_ts_data->work_mode !=
				       HYN_WORK_MODE_FACTORY)
					      ? hynitron_touch_setmode(
							client,
							HYN_WORK_MODE_FACTORY,
							0)
					      : 0;

			if (ret < 0) {
				HYN_INFO("Write command raw/diff mode "
					 "failed.error:%d.\n",
					 ret);
				goto END;
			}

			mdelay(10);

			for (report_count = 0; report_count < 16;
			     report_count++) {
				unsigned char temp_buf[7];
				ret = i2c_master_recv(client, temp_buf, 6);
				if (ret < 0) {
					HYN_INFO("Read raw/diff data "
						 "failed.error:%d.\n",
						 ret);
					goto END;
				}
				memcpy((unsigned char *)buf + 2 +
					       report_count * 5,
				       (unsigned char *)temp_buf + 1, 5);
			}
			/*
						buf[0] = 0x00;
						buf[1] = 0x00;
						ret = hyn_i2c_write(client, buf,
			   2); if(ret < 0)
						{
							HYN_INFO("Write command
			   raw/diff mode failed.error:%d.\n", ret); goto END;
						}
			*/
			buf[0] = 4;
			buf[1] = 10;
			ret_len = data_len + 2;
			ret = copy_to_user(user_buf, buf, ret_len);
			if (ret < 0)
				goto END;
		}
	}

	if (CMDIndex == 0xCA) {
		buf[0] = 0xAC;
		if (g_exc_flag == 1) {
			buf[1] = 3;
			HYN_INFO("run_cmd_thread is running\n");
		} else {
			if (g_cmdret[0] == 0) {
				buf[1] = 0;
				HYN_INFO("run_cmd_thread cmdpac exc pass!\n");
			} else {
				buf[1] = 1;
				HYN_INFO("run_cmd_thread cmdpac exc fail!\n");
			}
		}

		memcpy(&buf[2], &g_cmdret[2], g_cmdret[1]);

		ret_len = 2 + g_cmdret[1];
		ret = copy_to_user(user_buf, buf, ret_len);
	}

END:
	// hyn_ts_data->work_mode = HYN_WORK_MODE_NORMAL;
	// CMDIndex = 0;

	if (buf != NULL) {
		kfree(buf);
		buf = NULL;
	}

#if HYN_ESDCHECK_EN
	hyn_esd_switch(SWITCH_ESD_ON);
#endif
	HYN_INFO("copy len = %d\r\n", ret_len);
	mutex_unlock(&g_procrw_mutex);
	HYN_FUNC_EXIT();

	if (ret < 0) {
		return -EFAULT;
	}
	*f_pos += ret_len;

	return ret_len;
}

static ssize_t hynitron_proc_write_foobar(struct file *file,
					  const char __user *buffer,
					  size_t count, loff_t *data) {
	unsigned char cmd[400]; // 1200, CONFIG_FRAME_WARN, stack limit
	unsigned char *pdata = NULL;
	int copy_len;
	int ret = 0;
	int length;
	struct i2c_client *client =
		(struct i2c_client *)PDE_DATA(file_inode(file));

	HYN_FUNC_ENTER();

	mutex_lock(&g_procrw_mutex);
	if (count > 1024)
		copy_len = 1024;
	else
		copy_len = count;

	if (copy_from_user(cmd, buffer, copy_len)) {
		HYN_INFO("copy data from user space failed.\n");
		ret = -1;
		goto Exit;
	}
#if HYN_ESDCHECK_EN
	hyn_esd_switch(SWITCH_ESD_OFF);
#endif

	HYN_INFO("hynitron_proc_write_foobar*********cmd:%d*****%d******len:%d "
		 ".\r\n",
		 cmd[0], cmd[1], copy_len);

	if (client == NULL) {
		client = hyn_ts_data->client;
		HYN_INFO("client is null.\n");
	}

	if (cmd[0] == 0) {
		if (hyn_ts_data->fw_length != 0) {
			length = hyn_ts_data->fw_length;
		} else {
			HYN_ERROR("hyn_ts_data->fw_length error:%d.\n",
				  hyn_ts_data->fw_length);
			return -ENOMEM;
		}

		HYN_INFO("config length:%d.\n", length);

		ret = hynitron_read_fw_file(&cmd[1], &pdata, &length);
		if (ret < 0) {
			HYN_INFO("hynitron_read_fw_file fail.\n");
			goto Exit;
		}
		ret = hynitron_apk_fw_dowmload(client, pdata, length);
		if (ret < 0) {
			HYN_INFO("update firmware failed.\n");
			goto Exit;
		}

	} else if (cmd[0] == 2) {
		// hynitron_touch_release();
		CMDIndex = cmd[1];
	} else if (cmd[0] == 3) {
		CMDIndex = 0;
		ret = hynitron_touch_setmode(
			client, HYN_WORK_MODE_NORMAL,
			hyn_ts_data->config_chip_product_line ==
				HYN_CHIP_PRODUCT_LINE_MUT_CAP);
		if (ret < 0) {
			HYN_INFO("hynitron_touch_setmode fail.\n");
			goto Exit;
		}
	} else if (cmd[0] == 0xCA) {
		if (copy_len > sizeof(g_cmdpack)) {
			HYN_INFO("copy_len is too big\n");
			ret = -1;
			goto Exit;
		}
		if (g_exc_flag == 0) {
			CMDIndex = 0xCA;
			g_cmdlen = copy_len;
			memset(g_cmdpack, 0, sizeof(g_cmdpack));
			memcpy(g_cmdpack, cmd, copy_len);
			gExClient = client;

			g_exc_flag = 1;
			wake_up(&run_cmd_waiter);
		} else {
			HYN_INFO("thread is running\n");
			ret = -1;
			goto Exit;
		}

	} else {

		HYN_INFO("cmd[0] error:%d.\n", cmd[0]);
	}

#if HYN_ESDCHECK_EN
	hyn_esd_switch(SWITCH_ESD_ON);
#endif

Exit:
	mutex_unlock(&g_procrw_mutex);

	if (ret < 0) {
		if (pdata != NULL) {
			kfree(pdata);
			pdata = NULL;
		}
		return -EPERM;
	}
	HYN_FUNC_EXIT();
	return count;
}
static const struct file_operations proc_tool_debug_fops = {

	.owner = THIS_MODULE,

	.read = hynitron_proc_read_foobar,

	.write = hynitron_proc_write_foobar,

};

int hynitron_proc_fs_init(void) {

	int ret;
	HYN_FUNC_ENTER();

	mutex_init(&g_procrw_mutex);

	if (hyn_ts_data->config_chip_product_line ==
	    HYN_CHIP_PRODUCT_LINE_MUT_CAP) {
		// mutcap
		g_proc_dir = proc_mkdir(HYNITRON_PROC_DIR_NAME, NULL);
		if (g_proc_dir == NULL) {
			ret = -ENOMEM;
			HYN_INFO("proc_mkdir HYNITRON_PROC_DIR_NAME failed.\n");
			goto out;
		}

	} else if (hyn_ts_data->config_chip_product_line ==
		   HYN_CHIP_PRODUCT_LINE_SEL_CAP) {
		// selfcap
		g_proc_dir = proc_mkdir(HYNITRON_PROC_DIR_NAME_2, NULL);
		if (g_proc_dir == NULL) {
			ret = -ENOMEM;
			HYN_INFO("proc_mkdir HYNITRON_PROC_DIR_NAME_2 "
				 "failed.\n");
			goto out;
		}
	}

	if (hyn_ts_data->config_chip_product_line ==
	    HYN_CHIP_PRODUCT_LINE_MUT_CAP) {
		// mutcap
		g_update_file = proc_create_data(
			HYNITRON_PROC_FILE_NAME, 0777 | S_IFREG, g_proc_dir,
			&proc_tool_debug_fops, (void *)hyn_ts_data->client);
		if (NULL == g_update_file) {
			ret = -ENOMEM;
			HYN_INFO("proc_create_data HYNITRON_PROC_FILE_NAME "
				 "failed.\n");
			goto no_foo;
		}
	} else if (hyn_ts_data->config_chip_product_line ==
		   HYN_CHIP_PRODUCT_LINE_SEL_CAP) {
		// selfcap
		g_update_file = proc_create_data(
			HYNITRON_PROC_FILE_NAME_2, 0777 | S_IFREG, g_proc_dir,
			&proc_tool_debug_fops, (void *)hyn_ts_data->client);
		if (NULL == g_update_file) {
			ret = -ENOMEM;
			HYN_INFO("proc_create_data HYNITRON_PROC_FILE_NAME_2 "
				 "failed.\n");
			goto no_foo;
		}
	}

	if (gExchandle == NULL)
		gExchandle =
			kthread_run(hyn_exc_cmdpack, NULL, "hyn-debug-thread");

	HYN_FUNC_EXIT();
	return 0;

no_foo:
	if (hyn_ts_data->config_chip_product_line ==
	    HYN_CHIP_PRODUCT_LINE_MUT_CAP) {
		remove_proc_entry(HYNITRON_PROC_FILE_NAME, g_proc_dir);
	} else if (hyn_ts_data->config_chip_product_line ==
		   HYN_CHIP_PRODUCT_LINE_SEL_CAP) {
		remove_proc_entry(HYNITRON_PROC_FILE_NAME_2, g_proc_dir);
	}
out:
	return ret;
}
void hynitron_proc_fs_exit(void) {

	mutex_destroy(&g_procrw_mutex);

	if (g_proc_dir == NULL)
		return;

	g_proc_dir = proc_mkdir(HYNITRON_PROC_DIR_NAME, NULL);
	if (g_proc_dir != NULL) {
		remove_proc_entry(HYNITRON_PROC_FILE_NAME, g_proc_dir);
		g_proc_dir = NULL;
	}

	g_proc_dir = proc_mkdir(HYNITRON_PROC_DIR_NAME_2, NULL);
	if (g_proc_dir != NULL) {
		remove_proc_entry(HYNITRON_PROC_DIR_NAME_2, g_proc_dir);
		g_proc_dir = NULL;
	}
	if (gExchandle) {
		kthread_stop(gExchandle);
		gExchandle = NULL;
	}
}

#endif

#if HYN_SYSFS_NODE_EN
static struct mutex g_device_mutex;
static DEFINE_MUTEX(g_device_mutex);
#if 1
#define HYN_LOG_TXT "/sdcard/hyn_log.txt"
#define HYN_NOISE_LOG_TXT "/sdcard/hyn_noise_log.txt"

void hyn_save_log(unsigned char *buf) {
	unsigned char p_test_data[180];
	struct file *fp;
	mm_segment_t fs;
	loff_t pos;
	struct inode *inode;
	unsigned long magic;
	int length;
	unsigned char tx_num;

	memset((unsigned char *)p_test_data, 0x00, 180);
	fp = filp_open(HYN_LOG_TXT, O_RDWR | O_CREAT, 0644);
	if (IS_ERR(fp)) {
		printk("hyn log filp_open file error.\n");
	} else {
		inode = fp->f_inode;
		magic = inode->i_sb->s_magic;

		printk("*************f_pos:0x%x, i_size:0x%x.\t\n",
		       (unsigned int)fp->f_pos, (unsigned int)inode->i_size);
		fp->f_pos = inode->i_size;
		pos = fp->f_pos;
		length = 0;

		{
			fs = get_fs();
			set_fs(KERNEL_DS);
			printk("start save log.\t\n");
			snprintf(p_test_data, 180,
				 "pos:0x%04x********start save "
				 "log**********\t\n",
				 (unsigned int)pos);
			length = strlen(p_test_data);
			length += 1;
			vfs_write(fp, p_test_data, length, &pos);
			fp->f_pos = pos;

			for (tx_num = 0; tx_num < 14; tx_num++) {
				pos = fp->f_pos;
				printk("tx_num=%d: 0x%02x. 0x%02x. 0x%02x. "
				       "0x%02x. 0x%02x. 0x%02x. "
				       "0x%02x. 0x%02x. 0x%02x. 0x%02x.0x%02x. "
				       "0x%02x.\t\n",
				       tx_num, buf[tx_num * 12 + 0],
				       buf[tx_num * 12 + 1],
				       buf[tx_num * 12 + 2],
				       buf[tx_num * 12 + 3],
				       buf[tx_num * 12 + 4],
				       buf[tx_num * 12 + 5],
				       buf[tx_num * 12 + 6],
				       buf[tx_num * 12 + 7],
				       buf[tx_num * 12 + 8],
				       buf[tx_num * 12 + 9],
				       buf[tx_num * 12 + 10],
				       buf[tx_num * 12 + 11]);
				snprintf(p_test_data, 180,
					 "0x%02x. 0x%02x. 0x%02x. 0x%02x. "
					 "0x%02x. 0x%02x. 0x%02x. 0x%02x. "
					 "0x%02x. 0x%02x.0x%02x. 0x%02x.\t\n",
					 buf[tx_num * 12 + 0],
					 buf[tx_num * 12 + 1],
					 buf[tx_num * 12 + 2],
					 buf[tx_num * 12 + 3],
					 buf[tx_num * 12 + 4],
					 buf[tx_num * 12 + 5],
					 buf[tx_num * 12 + 6],
					 buf[tx_num * 12 + 7],
					 buf[tx_num * 12 + 8],
					 buf[tx_num * 12 + 9],
					 buf[tx_num * 12 + 10],
					 buf[tx_num * 12 + 11]);
				length = strlen(p_test_data);
				length += 1;
				vfs_write(fp, p_test_data, length, &pos);
				fp->f_pos = pos;
			}
			set_fs(fs);
			filp_close(fp, NULL);
		}
	}
}

void hyn_save_noise_log(unsigned char *buf) {
	unsigned char p_test_data[180];
	struct file *fp;
	mm_segment_t fs;
	loff_t pos;
	struct inode *inode;
	unsigned long magic;
	int length;
	// unsigned char tx_num;

	memset((unsigned char *)p_test_data, 0x00, 180);
	fp = filp_open(HYN_NOISE_LOG_TXT, O_RDWR | O_CREAT, 0644);
	if (IS_ERR(fp)) {
		printk("hyn log filp_open file error.\n");
	} else {
		inode = fp->f_inode;
		magic = inode->i_sb->s_magic;
		printk("*************f_pos:0x%x, i_size:0x%x.\t\n",
		       (unsigned int)fp->f_pos, (unsigned int)inode->i_size);
		fp->f_pos = inode->i_size;
		pos = fp->f_pos;
		length = 0;
		{
			fs = get_fs();
			set_fs(KERNEL_DS);

			printk("start save log.\t\n");
			fp->f_pos = pos;
			pos = fp->f_pos;
			printk("get noise:0x%02x. 0x%02x. 0x%02x. 0x%02x. "
			       "0x%02x. 0x%02x. "
			       "0x%02x. 0x%02x. 0x%02x. 0x%02x.\t\n",
			       buf[0], buf[1], buf[2], buf[3], buf[4], buf[5],
			       buf[6], buf[7], buf[8], buf[9]);
			snprintf(p_test_data, 180,
				 "0x%02x. 0x%02x. 0x%02x. 0x%02x. 0x%02x. "
				 "0x%02x. 0x%02x. "
				 "0x%02x. 0x%02x. 0x%02x.\t\n",
				 buf[0], buf[1], buf[2], buf[3], buf[4], buf[5],
				 buf[6], buf[7], buf[8], buf[9]);
			length = strlen(p_test_data);
			length += 1;
			vfs_write(fp, p_test_data, length, &pos);
			fp->f_pos = pos;

			set_fs(fs);
			filp_close(fp, NULL);
		}
	}
}

static int hyn_check_diff(void) {
	u8 buf1[180];
	u8 buf2[180];
	u8 buf3[180];
	int ret = -1;
	u8 read_timer;

	memset((u8 *)buf1, 0, 180);
	memset((u8 *)buf2, 0, 180);
	memset((u8 *)buf3, 0, 180);

	mdelay(5);
	printk("%s ---> GET TP VALUE 0X1000.\n", __func__);
	buf1[0] = 0x10;
	buf1[1] = 0x00;
	ret = cst3xx_i2c_write(hyn_ts_data->client, buf1, 2);
	if (ret < 0) {
		printk("%s : ret = %d. GET TP VALUE 0X1000.\n", __func__, ret);
		return ret;
	}
	mdelay(20);
	buf1[0] = 0x10;
	buf1[1] = 0x00;
	ret = cst3xx_i2c_write(hyn_ts_data->client, buf1, 2);
	if (ret < 0) {
		printk("%s : ret = %d. GET TP VALUE 0X1000.\n", __func__, ret);
		return ret;
	}
	mdelay(1);
	// ret = cst3xx_i2c_read(hyn_ts_data->client, buf1, 168);
	for (read_timer = 0; read_timer < 168; read_timer += 6) {
		ret = cst3xx_i2c_read(hyn_ts_data->client,
				      ((u8 *)buf1 + read_timer), 6);
		if (ret < 0) {
			printk("%s : ret = %d. GET TP VALUE 0X1000.\n",
			       __func__, ret);
			return ret;
		}
	}

	mdelay(1);
	printk("%s ---> GET TP VALUE 0X2000.\n", __func__);
	buf2[0] = 0x20;
	buf2[1] = 0x00;
	ret = cst3xx_i2c_write(hyn_ts_data->client, buf2, 2);
	if (ret < 0) {
		printk("%s : ret = %d. GET TP VALUE 0X2000.\n", __func__, ret);
		return ret;
	}
	mdelay(1);
	// ret = cst3xx_i2c_read(hyn_ts_data->client, buf2, 168);
	for (read_timer = 0; read_timer < 168; read_timer += 6) {
		ret = cst3xx_i2c_read(hyn_ts_data->client,
				      ((u8 *)buf2 + read_timer), 6);
		if (ret < 0) {
			printk("%s : ret = %d. GET TP VALUE 0X2000.\n",
			       __func__, ret);
			return ret;
		}
	}

	mdelay(1);
	printk("%s ---> GET TP VALUE 0X3000.\n", __func__);
	buf3[0] = 0x30;
	buf3[1] = 0x00;
	ret = cst3xx_i2c_write(hyn_ts_data->client, buf3, 2);
	if (ret < 0) {
		printk("%s : ret = %d. GET TP VALUE 0X3000.\n", __func__, ret);
		return ret;
	}
	mdelay(1);
	// ret = cst3xx_i2c_read(hyn_ts_data->client, buf3, 168);
	for (read_timer = 0; read_timer < 168; read_timer += 6) {
		ret = cst3xx_i2c_read(hyn_ts_data->client,
				      ((u8 *)buf3 + read_timer), 6);
		if (ret < 0) {
			printk("%s : ret = %d. GET TP VALUE 0X3000.\n",
			       __func__, ret);
			return ret;
		}
	}

	hyn_save_log((u8 *)buf1);
	hyn_save_log((u8 *)buf2);
	hyn_save_log((u8 *)buf3);
	printk("%s ---> GET TP VALUE DONE.\n", __func__);
	return ret;
}

#endif
static ssize_t hyn_tpfwver_show(struct device *dev,
				struct device_attribute *attr, char *buf) {
	ssize_t num_read_chars = 0;
	u8 buf1[10];
	unsigned int chip_version, module_version, project_version, chip_type,
		checksum;

	// struct i2c_client *client = container_of(dev, struct i2c_client,
	// dev);

	memset((u8 *)buf1, 0, 10);
	mutex_lock(&g_device_mutex);

#if HYN_ESDCHECK_EN
	hyn_esd_switch(SWITCH_ESD_OFF);
#endif

	chip_version = 0;
	module_version = 0;
	project_version = 0;
	chip_type = 0;
	checksum = 0;

	if (hyn_ts_data->config_chip_product_line ==
	    HYN_CHIP_PRODUCT_LINE_MUT_CAP) {
		int ret;
		buf1[0] = 0xD1;
		buf1[1] = 0x01;
		ret = cst3xx_i2c_write(hyn_ts_data->client, buf1, 2);
		if (ret < 0)
			return -1;

		mdelay(10);

		buf1[0] = 0xD2;
		buf1[1] = 0x04;
		ret = cst3xx_i2c_read_register(hyn_ts_data->client, buf1, 4);
		if (ret < 0)
			return -1;

		chip_type = buf1[3];
		chip_type <<= 8;
		chip_type |= buf1[2];

		project_version |= buf1[1];
		project_version <<= 8;
		project_version |= buf1[0];

		buf1[0] = 0xD2;
		buf1[1] = 0x08;
		ret = cst3xx_i2c_read_register(hyn_ts_data->client, buf1, 4);
		if (ret < 0)
			return -1;

		chip_version = buf1[3];
		chip_version <<= 8;
		chip_version |= buf1[2];
		chip_version <<= 8;
		chip_version |= buf1[1];
		chip_version <<= 8;
		chip_version |= buf1[0];

		buf1[0] = 0xD2;
		buf1[1] = 0x0C;
		ret = cst3xx_i2c_read_register(hyn_ts_data->client, buf1, 4);
		if (ret < 0)
			return -1;

		checksum = buf1[3];
		checksum <<= 8;
		checksum |= buf1[2];
		checksum <<= 8;
		checksum |= buf1[1];
		checksum <<= 8;
		checksum |= buf1[0];

		buf1[0] = 0xD1;
		buf1[1] = 0x09;
		ret = cst3xx_i2c_write(hyn_ts_data->client, buf1, 2);

		num_read_chars =
			snprintf(buf, 128,
				 "chip_version: 0x%02X, module_version:0x%02X, "
				 "project_version:0x%02X, "
				 "chip_type:0x%02X, checksum:0x%02X .\n",
				 chip_version, module_version, project_version,
				 chip_type, checksum);

	} else if (hyn_ts_data->config_chip_product_line ==
		   HYN_CHIP_PRODUCT_LINE_SEL_CAP) {

		buf1[0] = 0xA6;
		if (hyn_i2c_read(hyn_ts_data->client, (u8 *)buf1, 1, (u8 *)buf1,
				 8) < 0)
			num_read_chars =
				snprintf(buf, 128, "get tp fw version fail!\n");
		else {
			chip_version = buf1[0];
			chip_version |= buf1[1] << 8;

			module_version = buf1[2];
			project_version = buf1[3];

			chip_type = buf1[4];
			chip_type |= buf1[5] << 8;

			checksum = buf1[6];
			checksum |= buf1[7] << 8;

			num_read_chars = snprintf(
				buf, 128,
				"chip_version: 0x%02X, module_version:0x%02X, "
				"project_version:0x%02X, chip_type:0x%02X, "
				"checksum:0x%02X .\n",
				chip_version, module_version, project_version,
				chip_type, checksum);
		}
	}
	mutex_unlock(&g_device_mutex);

#if HYN_ESDCHECK_EN
	hyn_esd_switch(SWITCH_ESD_ON);
#endif
	return num_read_chars;
}

static ssize_t hyn_tpfwver_store(struct device *dev,
				 struct device_attribute *attr, const char *buf,
				 size_t count) {
	/*place holder for future use*/
	return -EPERM;
}

static ssize_t hyn_tprwreg_show(struct device *dev,
				struct device_attribute *attr, char *buf) {
	ssize_t num_read_chars = 0;
	u8 buf1[20];
	u8 retry;
	int ret = -1;
	u16 value_0, value_1, value_2, value_3, value_4;

	mutex_lock(&g_device_mutex);
	memset((u8 *)buf1, 0, 20);

#if HYN_ESDCHECK_EN
	hyn_esd_switch(SWITCH_ESD_OFF);
#endif

	retry = 0;
	value_0 = 0;
	value_1 = 0;
	value_2 = 0;
	value_3 = 0;
	value_4 = 0;

START:
	mdelay(5);
	buf1[0] = 0xD1;
	buf1[1] = 0x09;
	ret = cst3xx_i2c_write(hyn_ts_data->client, buf1, 2);
	if (ret < 0) {
		num_read_chars = snprintf(
			buf, 50,
			"hyn_tprwreg_show:write debug info command fail.\n");
		printk("%s : ret = %d. hyn_tpfwver_show:write debug info "
		       "command fail.\n",
		       __func__, ret);
		goto err_return;
	}

	mdelay(5);

	buf1[0] = 0xD0;
	buf1[1] = 0x4F;
	ret = cst3xx_i2c_read_register(hyn_ts_data->client, buf1, 10);
	if (ret < 0) {
		num_read_chars = snprintf(
			buf, 50,
			"hyn_tprwreg_show:Read version resgister fail.\n");
		printk("%s : ret = %d. hyn_tprwreg_show:Read version resgister "
		       "fail.\n",
		       __func__, ret);
		goto err_return;
	}
	value_0 = buf1[1];
	value_0 <<= 8;
	value_0 |= buf1[0];

	value_1 = buf1[3];
	value_1 <<= 8;
	value_1 |= buf1[2];

	value_2 = buf1[5];
	value_2 <<= 8;
	value_2 |= buf1[4];

	value_3 = buf1[7];
	value_3 <<= 8;
	value_3 |= buf1[6];

	value_4 = buf1[9];
	value_4 <<= 8;
	value_4 |= buf1[8];

#if 1
	if (value_0 > 40)
		hyn_check_diff();
#endif

	num_read_chars =
		snprintf(buf, 240,
			 "value_0: 0x%02X, value_1:0x%02X, value_2:0x%02X, "
			 "value_3:0x%02X, value_4:0x%02X.\n",
			 value_0, value_1, value_2, value_3, value_4);

	hyn_save_noise_log((u8 *)buf1);

	buf1[0] = 0xD1;
	buf1[1] = 0x09;
	ret = cst3xx_i2c_write(hyn_ts_data->client, buf1, 2);
	if (ret < 0) {
		num_read_chars = snprintf(
			buf, 50, "hyn_tprwreg_show:write normal mode fail.\n");
		printk("%s : ret = %d. hyn_tprwreg_show:write normal mode "
		       "fail.\n",
		       __func__, ret);
		goto err_return;
	}

#if HYN_ESDCHECK_EN
	hyn_esd_switch(SWITCH_ESD_ON);
#endif

	mutex_unlock(&g_device_mutex);
	printk("%s : num_read_chars = %ld.\n", __func__, num_read_chars);
	return num_read_chars;

err_return:
	if (retry < 5) {
		retry++;
		goto START;
	} else {
		printk("%s : num_read_chars = %ld.\n", __func__,
		       num_read_chars);
		buf1[0] = 0xD1;
		buf1[1] = 0x09;
		ret = cst3xx_i2c_write(hyn_ts_data->client, buf1, 2);

#if HYN_ESDCHECK_EN
		hyn_esd_switch(SWITCH_ESD_ON);
#endif
		mutex_unlock(&g_device_mutex);
		return -1;
	}
}

static ssize_t hyn_tprwreg_store(struct device *dev,
				 struct device_attribute *attr, const char *buf,
				 size_t count) {
	// struct i2c_client *client = container_of(dev, struct i2c_client,
	// dev);
	ssize_t num_read_chars = 0;
	int retval;
	u16 regaddr = 0xff;
	u8 valbuf[16] = {0};

#if HYN_ESDCHECK_EN
	hyn_esd_switch(SWITCH_ESD_OFF);
#endif
	memset(valbuf, 0, sizeof(valbuf));
	mutex_lock(&g_device_mutex);
	num_read_chars = count - 1;

	if (num_read_chars > sizeof(valbuf))
		num_read_chars = sizeof(valbuf);

	HYN_INFO(" input character:%ld.\n", num_read_chars);

	memcpy(valbuf, buf, num_read_chars);

	if (1 == num_read_chars) {

		regaddr = valbuf[0];
		HYN_INFO("register(0x%x).\n", regaddr);

		/**********************************************
				//0-ascll-0x30:2¨¦?¡ä¡ã?¡À?o?
				//1-ascll-0x31:1?¡À??D??
				//2-ascll-0x32:¡ä¨°?a?D??
				//3-ascll-0x33:?¡ä??D???
				//4-ascll-0x34:¨¦y????¨¨?1¨¬?t
				//5-ascll-0x35:
				//6-ascll-0x36:
				//7-ascll-0x37:
				//8-ascll-0x38:
				//9-ascll-0x39:
		****************************************/
		if (regaddr == 0x30) {
			cst3xx_firmware_info(hyn_ts_data->client);
		} else if (regaddr == 0x31) {
			HYN_INFO("hyn_irq_disable enter.\n");
			hyn_irq_disable();
		} else if (regaddr == 0x32) {
			HYN_INFO("hyn_irq_enable enter.\n");
			hyn_irq_enable();
		} else if (regaddr == 0x33) {
			HYN_INFO("hyn_reset_proc enter.\n");
			hyn_reset_proc(10);
		} else if (regaddr == 0x34) {
			HYN_INFO("hyn_boot_update_fw enter.\n");
#if HYN_ESDCHECK_EN
			hyn_esd_switch(SWITCH_ESD_OFF);
#endif
			hyn_ts_data->apk_upgrade_flag = 0;
			hyn_boot_update_fw(hyn_ts_data->client);
#if HYN_ESDCHECK_EN
			hyn_esd_switch(SWITCH_ESD_ON);
#endif
		}
	} else if (2 == num_read_chars) {
		/*read register*/

		regaddr = valbuf[0] << 8;
		regaddr |= valbuf[1];
		HYN_INFO("register(0x%02x).\n", regaddr);

		/**********************************************
				//11-ascll-0x3131: ¨¦¨¨??1¡è¡Á¡Â?¡ê¨º?1
				***
				//19-ascll-0x3139: ¨¦¨¨??1¡è¡Á¡Â?¡ê¨º?9
		****************************************/
		if ((regaddr >> 8) == 0x31) {
			if ((regaddr & 0xff) == 0x30) {
				hyn_ts_data->work_mode = HYN_WORK_MODE_NORMAL;
			} else if ((regaddr & 0xff) == 0x31) {
				hyn_ts_data->work_mode = HYN_WORK_MODE_FACTORY;
			} else if ((regaddr & 0xff) == 0x32) {
				hyn_ts_data->work_mode = HYN_WORK_MODE_RAWDATA;
			} else if ((regaddr & 0xff) == 0x33) {
				hyn_ts_data->work_mode = HYN_WORK_MODE_DIFF;
			} else if ((regaddr & 0xff) == 0x34) {
				hyn_ts_data->work_mode = HYN_WORK_MODE_UPDATE;
			} else {
				hyn_ts_data->work_mode = HYN_WORK_MODE_NORMAL;
			}

			HYN_INFO("set work_mode(0x%02x).\n",
				 hyn_ts_data->work_mode);
			hynitron_touch_setmode(
				hyn_ts_data->client, hyn_ts_data->work_mode,
				hyn_ts_data->config_chip_product_line ==
					HYN_CHIP_PRODUCT_LINE_MUT_CAP);
		}
	} else if (num_read_chars > 2) {
		regaddr = valbuf[0] << 8;
		regaddr |= valbuf[1];
		HYN_INFO("register(0x%02x).\n", regaddr);
		retval = cst3xx_i2c_write(hyn_ts_data->client, valbuf,
					  num_read_chars);
		if (retval < 0)
			count = -1;
	}

	mutex_unlock(&g_device_mutex);
#if HYN_ESDCHECK_EN
	hyn_esd_switch(SWITCH_ESD_ON);
#endif

	return count;
}

static ssize_t hyn_fwupdate_show(struct device *dev,
				 struct device_attribute *attr, char *buf) {
	/* place holder for future use */
	return -EPERM;
}

/*upgrade from *.i*/
static ssize_t hyn_fwupdate_store(struct device *dev,
				  struct device_attribute *attr,
				  const char *buf, size_t count) {

	// struct i2c_client *client = container_of(dev, struct i2c_client,
	// dev);
	HYN_INFO("hyn_fwupdate_store enter.\n");
	mutex_lock(&g_device_mutex);

#if HYN_ESDCHECK_EN
	hyn_esd_switch(SWITCH_ESD_OFF);
#endif

	hyn_ts_data->apk_upgrade_flag = 0;
	hyn_boot_update_fw(hyn_ts_data->client);
	mutex_unlock(&g_device_mutex);

#if HYN_ESDCHECK_EN
	hyn_esd_switch(SWITCH_ESD_ON);
#endif
	return count;
}

static ssize_t hyn_fwupgradeapp_show(struct device *dev,
				     struct device_attribute *attr, char *buf) {
	/*place holder for future use*/
	return -EPERM;
}

/*upgrade from app.bin*/
static ssize_t hyn_fwupgradeapp_store(struct device *dev,
				      struct device_attribute *attr,
				      const char *buf, size_t count) {
	char fwname[256];
	int ret;
	unsigned char *pdata = NULL;
	int length;
	// struct i2c_client *client = container_of(dev, struct i2c_client,
	// dev);

#if HYN_ESDCHECK_EN
	hyn_esd_switch(SWITCH_ESD_OFF);
#endif
	HYN_INFO("hyn_fwupgradeapp_store enter.\n");

	memset(fwname, 0, sizeof(fwname));
	sprintf(fwname, "/mnt/%s", buf);
	fwname[count - 1 + 5] = '\0';

	if (hyn_ts_data->fw_length != 0) {
		length = hyn_ts_data->fw_length;
	} else {
		HYN_ERROR("hyn_ts_data->fw_length error:%d.\n",
			  hyn_ts_data->fw_length);
		return -ENOMEM;
	}
	HYN_INFO("fwname:%s.length:%d.\n", fwname, length);

	pdata = kzalloc(sizeof(char) * length, GFP_KERNEL);
	if (pdata == NULL) {
		HYN_INFO("hyn_fwupgradeapp_store GFP_KERNEL memory fail.\n");
		return -ENOMEM;
	}

	mutex_lock(&g_device_mutex);

	ret = hynitron_read_fw_file(fwname, &pdata, &length);
	if (ret < 0) {
		HYN_INFO("hynitron_read_fw_file fail.\n");
		if (pdata != NULL) {
			kfree(pdata);
			pdata = NULL;
		}
	} else {

		ret = hynitron_apk_fw_dowmload(hyn_ts_data->client, pdata,
					       length);
		if (ret < 0) {
			HYN_INFO("hynitron_apk_fw_dowmload failed.\n");
			if (pdata != NULL) {
				kfree(pdata);
				pdata = NULL;
			}
		}
	}
	mutex_unlock(&g_device_mutex);

#if HYN_ESDCHECK_EN
	hyn_esd_switch(SWITCH_ESD_ON);
#endif
	HYN_INFO("hyn_fwupgradeapp_store exit.\n");

	return count;
}

static ssize_t hyntpfactorytest_show(struct device *dev,
				     struct device_attribute *attr, char *buf) {
	/* place holder for future use */
	ssize_t num_read_chars = 0;
	HYN_INFO("hyntpfactory_show enter.\n");
#if HYN_AUTO_FACTORY_TEST_EN
	num_read_chars = sprintf(buf, "%s.\n", factory_test);
#endif
	return num_read_chars;
}
static ssize_t hyntpfactorytest_store(struct device *dev,
				      struct device_attribute *attr,
				      const char *buf, size_t count) {
	// struct i2c_client *client = container_of(dev, struct i2c_client,
	// dev);

	HYN_INFO("hyntpfactory_store enter.\n");
#if HYN_ESDCHECK_EN
	hyn_esd_switch(SWITCH_ESD_OFF);
#endif
#if HYN_AUTO_FACTORY_TEST_EN
	{
		int test_count;
		int ret = -1;
		unsigned char buf1[10];
		for (test_count = 0; test_count < 3; test_count++) {
			mdelay(30);
			ret = hyn_factory_touch_test();
			HYN_INFO("hyn_factory_touch_test test_count=%d, "
				 "ret:%d\n",
				 test_count, ret);
			buf1[0] = 0xD1;
			buf1[1] = 0x09;
			cst3xx_i2c_write(hyn_ts_data->client, buf1, 2);
			mdelay(30);
			if (ret < 0)
				HYN_INFO("hyntpfactorytest_store "
					 "hyn_factory_touch_test fail !");
			else
				break;
		}
		// hyn_reset_proc (200);
		buf1[0] = 0xD1;
		buf1[1] = 0x02;
		cst3xx_i2c_write(hyn_ts_data->client, buf1, 2);
		hyn_ts_data->work_mode = HYN_WORK_MODE_NORMAL;
		mdelay(100);
	}
#endif
#if HYN_ESDCHECK_EN
	hyn_esd_switch(SWITCH_ESD_ON);
#endif
	return count;
}

static ssize_t hyn_checkbuf_show(struct device *dev,
				 struct device_attribute *attr, char *buf) {
	ssize_t num_read_chars = 0;
	u8 retry;
	int ret = -1;

	mutex_lock(&g_device_mutex);
#if HYN_ESDCHECK_EN
	hyn_esd_switch(SWITCH_ESD_OFF);
#endif

	retry = 0;
START:
	ret = hyn_check_diff();
	if (ret < 0) {
		goto err_return;
	}

	num_read_chars = ret;

#if HYN_ESDCHECK_EN
	hyn_esd_switch(SWITCH_ESD_ON);
#endif

	mutex_unlock(&g_device_mutex);
	printk("%s : num_read_chars = %ld.\n", __func__, num_read_chars);
	return num_read_chars;

err_return:
	if (retry < 3) {
		retry++;
		goto START;
	} else {
		printk("%s : num_read_chars = %ld.\n", __func__,
		       num_read_chars);

#if HYN_ESDCHECK_EN
		hyn_esd_switch(SWITCH_ESD_ON);
#endif

		mutex_unlock(&g_device_mutex);
		return -1;
	}
}
/*upgrade from *.i*/
static ssize_t hyn_checkbuf_store(struct device *dev,
				  struct device_attribute *attr,
				  const char *buf, size_t count) {
	return -EPERM;
}

/*sysfs */
/*get the fw version
 *example:cat hyntpfwver
 */
static DEVICE_ATTR(hyntpfwver, S_IRUGO | S_IWUSR, hyn_tpfwver_show,
		   hyn_tpfwver_store);

/*upgrade from *.i
 *example: echo 1 > hynfwupdate
 */
static DEVICE_ATTR(hynfwupdate, S_IRUGO | S_IWUSR, hyn_fwupdate_show,
		   hyn_fwupdate_store);

/*read and write register
 *read example: echo 88 > hyntprwreg ---read register 0x88
 *write example:echo 8807 > hyntprwreg ---write 0x07 into register 0x88
 *
 *note:the number of input must be 2 or 4.if it not enough, please fill in the
 *0.
 */
static DEVICE_ATTR(hyntprwreg, S_IRUGO | S_IWUSR, hyn_tprwreg_show,
		   hyn_tprwreg_store);

/*upgrade from app.bin
 *example:echo "*_app.bin" > hynfwupgradeapp
 */
static DEVICE_ATTR(hynfwupgradeapp, S_IRUGO | S_IWUSR, hyn_fwupgradeapp_show,
		   hyn_fwupgradeapp_store);

/*factory test
 *example:cat hynfactorytest
 */
static DEVICE_ATTR(hyntpfactorytest, S_IRUGO | S_IWUSR, hyntpfactorytest_show,
		   hyntpfactorytest_store);

/*checkdiff
 */
static DEVICE_ATTR(hyncheckbuf, S_IRUGO | S_IWUSR, hyn_checkbuf_show,
		   hyn_checkbuf_store);

/*add your attr in here*/
static struct attribute *hyn_attributes[] = {&dev_attr_hyntpfwver.attr,
					     &dev_attr_hynfwupdate.attr,
					     &dev_attr_hyntprwreg.attr,
					     &dev_attr_hynfwupgradeapp.attr,
					     &dev_attr_hyntpfactorytest.attr,
					     &dev_attr_hyncheckbuf.attr,
					     NULL};

static struct attribute_group hyn_attribute_group = {.attrs = hyn_attributes};
/*create sysfs for debug*/

int hyn_create_sysfs(struct i2c_client *client) {
	int err;
	HYN_FUNC_ENTER();
	hyn_ts_data->client = client;
	if ((hyn_ts_data->k_obj =
		     kobject_create_and_add("hynitron_debug", NULL)) == NULL) {
		HYN_INFO("hynitron_debug sys node create error.\n");
		return -EIO;
	}
	err = sysfs_create_group(hyn_ts_data->k_obj, &hyn_attribute_group);
	if (0 != err) {
		HYN_INFO("%s() - ERROR: sysfs_create_group() failed.\n",
			 __func__);
		sysfs_remove_group(hyn_ts_data->k_obj, &hyn_attribute_group);
		return -EIO;
	} else {
		mutex_init(&g_device_mutex);
		HYN_INFO("%s() - sysfs_create_group() succeeded.\n", __func__);
	}
	HYN_FUNC_EXIT();
	return err;
}

void hyn_release_sysfs(struct i2c_client *client) {
	if (hyn_ts_data->k_obj == NULL)
		return;

	sysfs_remove_group(hyn_ts_data->k_obj, &hyn_attribute_group);
	kobject_unregister(hyn_ts_data->k_obj);
	mutex_destroy(&g_device_mutex);
}
#endif
