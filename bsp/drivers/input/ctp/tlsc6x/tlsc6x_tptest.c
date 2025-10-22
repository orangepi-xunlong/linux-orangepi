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
 * VERSION		DATE			AUTHOR
 *
 */
#include <linux/firmware.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/slab.h>
#include <linux/i2c.h>
#include <linux/input.h>
#include <asm/uaccess.h>
#include <linux/gpio.h>
#include <linux/input/mt.h>
#include <linux/of_device.h>
#include <linux/of_address.h>
#include <linux/of_gpio.h>
#include <linux/interrupt.h>
#include <linux/sched.h>
#include <linux/kthread.h>
#include <linux/wakelock.h>
#include <linux/completion.h>
#include <linux/err.h>
#include <linux/vmalloc.h>
#include <linux/firmware.h>
#include "tlsc6x_main.h"

#define TPTEST_INI_FILE_PATH         "/system/etc/"
#define TPTEST_RESULT_PATH           "/mnt/sdcard/"
#define TPTEST_DATA_FILE_NAME       "testdata.bin"
#define TPTEST_RESULT_FILE_NAME      "testresult.txt"

struct tlsc6x_stest_crtra *pt_crtra_data;
unsigned char *pt_test_lib;
unsigned int tptest_lib_size;
unsigned int ito_test_result;
unsigned int ito_test_status;
unsigned char test_ch_flag[48];
unsigned short test_raw_data[48];

/*
 * fts_test_get_testparams - get test parameter from ini
 */
static int tlsc6x_test_get_ini(char *ini_name)
{
	int inisize = 0;
	loff_t pos = 0;
	mm_segment_t old_fs;
	struct inode *inode = NULL;
	struct file *pfile = NULL;
	char filepath[256];

	pt_test_lib = NULL;
	tptest_lib_size = 0;
	pt_crtra_data = NULL;

	memset(filepath, 0, sizeof(filepath));
	sprintf(filepath, "%s%s", TPTEST_INI_FILE_PATH, ini_name);

	pfile = filp_open(filepath, O_RDONLY, 0);
	if (IS_ERR(pfile)) {
		tlsc_err("filp_open error:%s.", filepath);
		return -EIO;
	}

	inode = pfile->f_inode;
	inisize = inode->i_size;

	if (inisize <= (1024 + sizeof(struct tlsc6x_stest_crtra))) {
		filp_close(pfile, NULL);
		tlsc_err("wrong *.ini fail size:%s.", filepath);
		return -EIO;
	}

	pt_crtra_data = vmalloc(inisize + 8);
	if (NULL == pt_crtra_data) {
		filp_close(pfile, NULL);
		tlsc_err("alloc memory for *.ini fail:%s", __func__);
		return -ENOMEM;
	}

	pt_test_lib = (unsigned char *)(((unsigned char *)pt_crtra_data)
					+ sizeof(struct tlsc6x_stest_crtra));
	tptest_lib_size = inisize - sizeof(struct tlsc6x_stest_crtra);

	pos = 0;
	old_fs = get_fs();
	set_fs(KERNEL_DS);
	vfs_read(pfile, (char *)pt_crtra_data, inisize, &pos);
	filp_close(pfile, NULL);
	set_fs(old_fs);

	return 0;
}

static int osTest3536M3(unsigned short *posdata)
{
	int os3_delta[48];
	unsigned int m_os_nor[48];
	int m_os_nor_std;
	int k, avg, n, d;
	int st_nor_os_L1;
	int st_nor_os_L2;
	int st_nor_os_bar;
	int st_nor_os_key;
	int err;
	int halfBarNum;
	int aaNum, allNum;

	aaNum = pt_crtra_data->xch_n & 0xff;
	halfBarNum = (pt_crtra_data->xch_n >> 8) & 0xff;
	st_nor_os_L1 = pt_crtra_data->st_nor_os_L1;
	st_nor_os_L2 = pt_crtra_data->st_nor_os_L2;
	st_nor_os_bar = pt_crtra_data->st_nor_os_bar;
	st_nor_os_key = pt_crtra_data->st_nor_os_key;
	m_os_nor_std = pt_crtra_data->m_os_nor_std;
	if (st_nor_os_L1 == 0) {
		return 0;
	}

	err = 0;
	avg = 0;

	for (k = 0; k < 48; k++) {
		avg += posdata[k];
	}
	avg = avg / 48;
	if (avg == 0) {
		avg = 1;
	}

	for (k = 0; k < 48; k++) {
		m_os_nor[k] = posdata[k] * m_os_nor_std / avg;
		os3_delta[k] = m_os_nor[k] - pt_crtra_data->osSample[k];
	}

	for (k = 0; k < halfBarNum; k++) {
		n = pt_crtra_data->remap[k];
		d = pt_crtra_data->remap[k + halfBarNum];
		if ((os3_delta[n] < -st_nor_os_L2) && (os3_delta[d] < -st_nor_os_L2)) {	//200
			err |= 0x100;
			test_ch_flag[k] |= 0x04;
		}

		if ((os3_delta[n] < -(st_nor_os_L2 + st_nor_os_L1))
		    || (os3_delta[d] < -(st_nor_os_L2 + st_nor_os_L1))) {
			err |= 0x200;
			test_ch_flag[k] |= 0x04;
		}
		if ((os3_delta[n] > st_nor_os_bar)
		    || (os3_delta[d] > st_nor_os_bar)) {
			err |= 0x400;
			test_ch_flag[k] |= 0x08;
		}
	}
	if (halfBarNum * 2 < aaNum) {
		for (k = 2 * halfBarNum; k < 3 * halfBarNum; k++) {
			n = pt_crtra_data->remap[k];
			d = pt_crtra_data->remap[k + halfBarNum];
			if ((os3_delta[n] < -st_nor_os_L2)
			    && (os3_delta[d] < -st_nor_os_L2)) {	//200
				err |= 0x100;
				test_ch_flag[k] |= 0x04;
			}

			if ((os3_delta[n] < -(st_nor_os_L2 + st_nor_os_L1))
			    || (os3_delta[d] < -(st_nor_os_L2 + st_nor_os_L1))) {
				err |= 0x200;
				test_ch_flag[k] |= 0x08;
			}
			if ((os3_delta[n] > st_nor_os_bar)
			    || (os3_delta[d] > st_nor_os_bar)) {
				err |= 0x400;
				test_ch_flag[k] |= 0x08;
			}
		}
	}

	allNum = pt_crtra_data->allch_n & 0xff;
	for (k = aaNum; k < allNum; k++) {
		n = pt_crtra_data->remap[k];
		if (os3_delta[n] > 2 * (st_nor_os_key + st_nor_os_L1)) {
			err |= 0x400;
			test_ch_flag[k] |= 0x08;
		}
		if (os3_delta[n] < -(st_nor_os_key + st_nor_os_L1)) {
			err |= 0x200;
			test_ch_flag[k] |= 0x04;
		}
	}
	return err;
}

static int osTest3536M2(unsigned short *posdata)
{
	int k, n;
	int allNum;
	int ret = 0;

	allNum = pt_crtra_data->allch_n & 0xff;

	for (k = 0; k < allNum; k++) {
		n = pt_crtra_data->remap[k];
		if (posdata[n] > pt_crtra_data->rawmax[n]) {
			ret |= 0x02;
			test_ch_flag[n] |= 0x02;
		} else if (posdata[n] < pt_crtra_data->rawmin[n]) {
			ret |= 0x01;
			test_ch_flag[n] |= 0x01;
		}
	}

	return ret;
}

static int tlsc6x_test_entry(char *ini_name)
{
	int ret = 0;
	int loop = 0;
	unsigned int flen = 0;
	unsigned char *pstr_buf = NULL;
	char fileFullName[256] = { 0 };

	TLSC_FUNC_ENTER();

	ito_test_result = 0;
	ito_test_status = 0;
	memset(test_ch_flag, 0, sizeof(test_ch_flag));
	memset(test_raw_data, 0, sizeof(test_raw_data));

	if (tlsc6x_test_get_ini(ini_name)) {
		ito_test_status = 1;
		ret = -EPERM;
		goto exit;
	}

	if (tlsc6x_download_ramcode(pt_test_lib, tptest_lib_size)) {
		ito_test_status = 2;
		tlsc_err("%s():run test failed.", __func__);
		ret = -EPERM;
		goto exit;
	}

	if (tlsc6x_get_os_data(test_raw_data, sizeof(test_raw_data)) < 0) {
		ito_test_status = 3;
		tlsc_err("%s():get data failed.", __func__);
		ret = -EPERM;
		goto exit;
	}

	ito_test_result |= osTest3536M2(test_raw_data);
	ito_test_result |= osTest3536M3(test_raw_data);

exit:

	snprintf(fileFullName, sizeof(fileFullName), "%s%s", TPTEST_RESULT_PATH,
		 TPTEST_DATA_FILE_NAME);
	tlsc6x_fif_write(fileFullName, (u8 *) test_raw_data,
			 (u16) sizeof(test_raw_data));

	pstr_buf = kzalloc(1024, GFP_KERNEL);	/* auto clear */
	if (pstr_buf == NULL) {
		ito_test_status = 4;
		tlsc_err
		    ("Tlsc6x:tlsc6x_rawdata_test_allch error::alloc file buffer fail!\n");
		ret = -EPERM;
		goto exit;
	}

	flen = 0;
	if (ito_test_result || ito_test_status) {
		flen +=
		    snprintf(pstr_buf + flen, 1024 - flen, "Tp test failure\n");
	} else {
		flen +=
		    snprintf(pstr_buf + flen, 1024 - flen, "Tp test pass\n");
	}

	flen += snprintf(pstr_buf + flen, 1024 - flen,
			 "status=0x%0x, result=0x%x\n", ito_test_status,
			 ito_test_result);

	for (loop = 0; loop < 48; loop++) {
		if (loop && ((loop % 12) == 0)) {
			flen += snprintf(pstr_buf + flen, 1024 - flen, "\n");
		}
		flen += snprintf(pstr_buf + flen, 1024 - flen, "%05d,",
				 test_raw_data[loop]);
	}
	flen += snprintf(pstr_buf + flen, 1024 - flen, "\n");
	for (loop = 0; loop < 48; loop++) {
		if (loop && ((loop % 12) == 0)) {
			flen += snprintf(pstr_buf + flen, 1024 - flen, "\n");
		}
		flen += snprintf(pstr_buf + flen, 1024 - flen, "0x%02x,",
				 test_ch_flag[loop]);
	}

	snprintf(fileFullName, sizeof(fileFullName), "%s%s", TPTEST_RESULT_PATH,
		 TPTEST_RESULT_FILE_NAME);
	tlsc6x_fif_write(fileFullName, (u8 *) pstr_buf, (u16) (flen + 1));
	kfree(pstr_buf);

	if (pt_crtra_data) {
		vfree(pt_crtra_data);
	}
	pt_test_lib = NULL;
	tptest_lib_size = 0;
	pt_crtra_data = NULL;

	return ret;
}

static ssize_t tlsc6x_test_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	ssize_t num_read_chars = 0;

	tlsc_info("status=0x%0x, result=0x%x\n", ito_test_status,
		  ito_test_result);

	if (ito_test_result || ito_test_status) {
		num_read_chars = snprintf(buf, PAGE_SIZE, "Failed\n");
	} else {
		num_read_chars = snprintf(buf, PAGE_SIZE, "Pass\n");
	}

	return num_read_chars;
}

/*
 intput para: *.init name
 */
static ssize_t tlsc6x_test_store(struct device *dev,
				 struct device_attribute *attr, const char *buf,
				 size_t count)
{
	char fname[128] = { 0 };

	TLSC_FUNC_ENTER();

	if (count > 127) {
		tlsc_err("input too much:%d.", count);
		return 0;
	}

	memset(fname, 0, sizeof(fname));
	snprintf(fname, sizeof(fname), "%s", buf);
	fname[count - 1] = '\0';

	tlsc6x_irq_disable();
	g_tp_drvdata->esdHelperFreeze = 1;

	tlsc6x_test_entry(fname);
	tlsc6x_tpd_reset_force();
	g_tp_drvdata->esdHelperFreeze = 0;

	tlsc6x_irq_enable();

	TLSC_FUNC_EXIT();

	return count;
}

static DEVICE_ATTR(tlsc6x_test, S_IRUSR | S_IWUSR, tlsc6x_test_show,
		   tlsc6x_test_store);

static struct attribute *tlsc6x_test_attributes[] = {
	&dev_attr_tlsc6x_test.attr, NULL
};

static struct attribute_group tlsc6x_test_attribute_group = {.attrs =
	    tlsc6x_test_attributes
};

int tlsc6x_tptest_init(struct i2c_client *client)
{
	int ret = 0;

	TLSC_FUNC_ENTER();

	ret =
	    sysfs_create_group(&client->dev.kobj, &tlsc6x_test_attribute_group);
	if (0 != ret) {
		tlsc_err("%s(): sysfs_create_group() failed.", __func__);
	} else {
		tlsc_info("%s():sysfs_create_group() succeeded.", __func__);
	}

	return ret;
}

int tlsc6x_tptest_exit(struct i2c_client *client)
{
	TLSC_FUNC_ENTER();

	sysfs_remove_group(&client->dev.kobj, &tlsc6x_test_attribute_group);

	return 0;
}
