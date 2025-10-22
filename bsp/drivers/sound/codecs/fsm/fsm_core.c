/* SPDX-License-Identifier: GPL-2.0-or-later */
/**
 * Copyright (C) Fourier Semiconductor Inc. 2016-2020. All rights reserved.
 * 2018-10-17 File created.
 */

#include "fsm_public.h"

#define CRC16_TABLE_SIZE      256
#define CRC16_POLY_NOMIAL     0xA001
#define FS1801_OTPPG2_DEFAULT 0xFF10

static int fsm_skip_device(fsm_dev_t *fsm_dev);
int fsm_stub_check_stable(fsm_dev_t *fsm_dev, int type);
static int fsm_try_init(void);

static LIST_HEAD(fsm_dev_list);
#define fsm_list_init(fsm_dev) \
	do { \
		INIT_LIST_HEAD(&fsm_dev->list); \
		list_add(&fsm_dev->list, &fsm_dev_list); \
	} while (0)

#define fsm_list_entry(fsm_dev, ops) \
	do { \
		list_for_each_entry(fsm_dev, &fsm_dev_list, list) { \
			if (!fsm_skip_device(fsm_dev) && ops) { \
				ops(fsm_dev); \
			} \
		} \
	} while (0)

#define fsm_list_func(fsm_dev, func) \
	do { \
		list_for_each_entry(fsm_dev, &fsm_dev_list, list) { \
			if (!fsm_skip_device(fsm_dev)) { \
				func(fsm_dev); \
			} \
		} \
	} while (0)

#define fsm_list_check(fsm_dev, type, ret) \
	do { \
		ret = 0; \
		list_for_each_entry(fsm_dev, &fsm_dev_list, list) { \
			if (!fsm_skip_device(fsm_dev)) { \
				ret |= fsm_stub_check_stable(fsm_dev, type); \
				if (ret) { \
					break; \
				} \
			} \
		} \
	} while (0)

#define fsm_list_return(fsm_dev, func, ret) \
	do { \
		ret = 0; \
		list_for_each_entry(fsm_dev, &fsm_dev_list, list) { \
			if (!fsm_skip_device(fsm_dev)) { \
				ret |= func(fsm_dev); \
			} \
		} \
	} while (0)

#define fsm_list_func_arg(fsm_dev, func, argv) \
	do { \
		list_for_each_entry(fsm_dev, &fsm_dev_list, list) { \
			if (!fsm_skip_device(fsm_dev)) { \
				func(fsm_dev, argv); \
			} \
		} \
	} while (0)


static uint16_t g_crc16table[CRC16_TABLE_SIZE] = {
	0x0000, 0xc0c1, 0xc181, 0x0140, 0xc301, 0x03c0, 0x0280, 0xc241,
	0xc601, 0x06c0, 0x0780, 0xc741, 0x0500, 0xc5c1, 0xc481, 0x0440,
	0xcc01, 0x0cc0, 0x0d80, 0xcd41, 0x0f00, 0xcfc1, 0xce81, 0x0e40,
	0x0a00, 0xcac1, 0xcb81, 0x0b40, 0xc901, 0x09c0, 0x0880, 0xc841,
	0xd801, 0x18c0, 0x1980, 0xd941, 0x1b00, 0xdbc1, 0xda81, 0x1a40,
	0x1e00, 0xdec1, 0xdf81, 0x1f40, 0xdd01, 0x1dc0, 0x1c80, 0xdc41,
	0x1400, 0xd4c1, 0xd581, 0x1540, 0xd701, 0x17c0, 0x1680, 0xd641,
	0xd201, 0x12c0, 0x1380, 0xd341, 0x1100, 0xd1c1, 0xd081, 0x1040,
	0xf001, 0x30c0, 0x3180, 0xf141, 0x3300, 0xf3c1, 0xf281, 0x3240,
	0x3600, 0xf6c1, 0xf781, 0x3740, 0xf501, 0x35c0, 0x3480, 0xf441,
	0x3c00, 0xfcc1, 0xfd81, 0x3d40, 0xff01, 0x3fc0, 0x3e80, 0xfe41,
	0xfa01, 0x3ac0, 0x3b80, 0xfb41, 0x3900, 0xf9c1, 0xf881, 0x3840,
	0x2800, 0xe8c1, 0xe981, 0x2940, 0xeb01, 0x2bc0, 0x2a80, 0xea41,
	0xee01, 0x2ec0, 0x2f80, 0xef41, 0x2d00, 0xedc1, 0xec81, 0x2c40,
	0xe401, 0x24c0, 0x2580, 0xe541, 0x2700, 0xe7c1, 0xe681, 0x2640,
	0x2200, 0xe2c1, 0xe381, 0x2340, 0xe101, 0x21c0, 0x2080, 0xe041,
	0xa001, 0x60c0, 0x6180, 0xa141, 0x6300, 0xa3c1, 0xa281, 0x6240,
	0x6600, 0xa6c1, 0xa781, 0x6740, 0xa501, 0x65c0, 0x6480, 0xa441,
	0x6c00, 0xacc1, 0xad81, 0x6d40, 0xaf01, 0x6fc0, 0x6e80, 0xae41,
	0xaa01, 0x6ac0, 0x6b80, 0xab41, 0x6900, 0xa9c1, 0xa881, 0x6840,
	0x7800, 0xb8c1, 0xb981, 0x7940, 0xbb01, 0x7bc0, 0x7a80, 0xba41,
	0xbe01, 0x7ec0, 0x7f80, 0xbf41, 0x7d00, 0xbdc1, 0xbc81, 0x7c40,
	0xb401, 0x74c0, 0x7580, 0xb541, 0x7700, 0xb7c1, 0xb681, 0x7640,
	0x7200, 0xb2c1, 0xb381, 0x7340, 0xb101, 0x71c0, 0x7080, 0xb041,
	0x5000, 0x90c1, 0x9181, 0x5140, 0x9301, 0x53c0, 0x5280, 0x9241,
	0x9601, 0x56c0, 0x5780, 0x9741, 0x5500, 0x95c1, 0x9481, 0x5440,
	0x9c01, 0x5cc0, 0x5d80, 0x9d41, 0x5f00, 0x9fc1, 0x9e81, 0x5e40,
	0x5a00, 0x9ac1, 0x9b81, 0x5b40, 0x9901, 0x59c0, 0x5880, 0x9841,
	0x8801, 0x48c0, 0x4980, 0x8941, 0x4b00, 0x8bc1, 0x8a81, 0x4a40,
	0x4e00, 0x8ec1, 0x8f81, 0x4f40, 0x8d01, 0x4dc0, 0x4c80, 0x8c41,
	0x4400, 0x84c1, 0x8581, 0x4540, 0x8701, 0x47c0, 0x4680, 0x8641,
	0x8201, 0x42c0, 0x4380, 0x8341, 0x4100, 0x81c1, 0x8081, 0x4040,
};

static struct fsm_config g_fsm_config = {
	.dev_count  = 0,
	.volume     = FSM_VOLUME_MAX,
	.next_scene = FSM_SCENE_MUSIC,
	.i2s_bclk   = 1536000,
	.i2s_srate  = 48000,
	.test_type  = TEST_NONE,
	.freq_start = FREQ_START,
	.freq_end   = FREQ_END,
	.freq_step  = F0_FREQ_STEP,
	.freq_count = FREQ_COUNT,
	.amb_tempr  = FSM_DFT_AMB_TEMPR,
	.cur_angle  = 0,
	.next_angle = 0,

	// flags
	.vddd_on = 0,
	.codec_inited = 0,
	.force_fw = 0,
	.force_init = 0,
	.force_scene = 0,
	.force_calib = 0,
	.store_otp = 1,
	.force_mute = 0,
	.stop_test = 0,
	.use_monitor = 0,
	.skip_monitor = 0,
	.stream_muted = 1,
	.nondsp_mode = 0,
	.skip_all = 0,
#ifdef DEBUG
	.i2c_debug = 1,
#endif
	.fw_name = FSM_FW_NAME,
	.preset = NULL,
};

static const struct fsm_srate g_srate_tbl[] = {
	{  8000, 0 },
	{ 16000, 3 },
	{ 32000, 8 },
	{ 44100, 7 },
	{ 48000, 8 },
	// fs1860 support below srate:
	{ 88200, 9 },
	{ 96000, 10 },
};
static struct fsm_vbat_state g_vbat_state = { 0 };

static int fsm_skip_device(fsm_dev_t *fsm_dev)
{
	fsm_config_t *cfg = fsm_get_config();

	if (!fsm_dev || !cfg) {
		return 1;
	}
	if (!fsm_dev->state.dev_inited || cfg->skip_all) {
		return 1;
	}
	if ((fsm_dev->own_scene & cfg->next_scene) == 0) {
		return 1;
	}

	return 0;
}

static int fsm_cal_re25_zmdata(fsm_dev_t *fsm_dev,
								struct re25_data *re25)
{
	uint16_t zmdata;
	int ret;

	if (!fsm_dev || !re25) {
		return -EINVAL;
	}
	if (!fsm_dev->state.re25_runin) {
		return 0;
	}
	if (re25->count >= RE25_TEST_COUNT)
		return 0;

	ret = fsm_reg_multiread(fsm_dev, REG(FSM_ZMDATA), &zmdata);
	if (ret) {
		pr_addr(err, "get zmdata fail:%d", ret);
		return -EINVAL;
	}
	if ((abs(re25->pre_val - zmdata) > FSM_ZMDELTA_MAX)
		|| re25->pre_val == 0) {
		re25->zmdata = 0;
		re25->pre_val = zmdata;
		re25->count = 1;
		re25->min_val = zmdata;
	} else {
		re25->count++;
		if (zmdata < re25->min_val) {
			re25->min_val = zmdata;
		}
	}
	pr_addr(info, "ZM[%2d]:%d", re25->count, zmdata);

	if (re25->count >= RE25_TEST_COUNT) {
		// finish calibration
		pr_addr(info, "ZM:%d", re25->min_val);
		re25->zmdata = re25->min_val;
		return 0;
	}

	return -EINVAL;
}

static int fsm_cal_f0_zmdata(fsm_dev_t *fsm_dev,
							struct f0_data *f0, int freq)
{
	uint16_t zmdata;
	int count;
	int ret;

	if (!fsm_dev || !f0) {
		return -EINVAL;
	}
	if (!fsm_dev->state.f0_runing || fsm_dev->is1603s) {
		return 0;
	}
	ret = fsm_reg_multiread(fsm_dev, REG(FSM_ZMDATA), &zmdata);
	if (ret) {
		pr_addr(err, "get zmdata fail:%d", ret);
		return -EINVAL;
	}
	if (zmdata == 0 || zmdata == 0xFFFF) {
		pr_addr(warning, "invalid data:%04X, skip", zmdata);
		return -EINVAL;
	}
	count = f0->count;
	pr_addr(info, "ZM[%4d]:%d", freq, zmdata);
	f0->zmdata[count] = zmdata;
	if (f0->count == 0 || zmdata < f0->min_zm) {
		f0->min_zm = zmdata;
		f0->min_idx = f0->count;
		fsm_dev->tdata->test_f0 = freq;
		fsm_dev->f0 = freq;
	}
	f0->count++;

	return 0;
}

static int fsm_cal_zmdata(fsm_dev_t *fsm_dev)
{
	fsm_config_t *cfg = fsm_get_config();
	int size;
	int ret;

	if (!fsm_dev || !cfg) {
		return -EINVAL;
	}
	if (fsm_skip_device(fsm_dev)) {
		return 0;
	}
	if (fsm_dev->dev_ops.cal_zmdata) {
		return fsm_dev->dev_ops.cal_zmdata(fsm_dev);
	}
	if (fsm_dev->tdata == NULL) {
		size = sizeof(struct fsm_test_data) + cfg->freq_count * sizeof(uint16_t);
		fsm_dev->tdata = fsm_alloc_mem(size);
		if (fsm_dev->tdata == NULL) {
			pr_addr(err, "alloc test data fail");
			return -EINVAL;
		}
		// memset(fsm_dev->tdata, 0, size);
	}
	if (cfg->test_type == TEST_RE25) {
		ret = fsm_cal_re25_zmdata(fsm_dev, &fsm_dev->tdata->re25);
	} else if (cfg->test_type == TEST_F0) {
		ret = fsm_cal_f0_zmdata(fsm_dev, &fsm_dev->tdata->f0, cfg->test_freq);
	} else {
		pr_addr(err, "invalid test type:%d", cfg->test_type);
		ret = -EINVAL;
	}

	return ret;
}

static int fsm_list_wait(int type)
{
	fsm_dev_t *fsm_dev = NULL;
	int retry;
	int ret;

	for (retry = 1; retry <= FSM_WAIT_STABLE_RETRY; retry++) {
		fsm_delay_ms(2);
		fsm_list_check(fsm_dev, type, ret);
		if (!ret) {
			break;
		}
	}
	if (retry > FSM_WAIT_STABLE_RETRY) {
		pr_err("type:%d, wait timeout!", type);
	}
	pr_debug("type:%d, wait %d times", type, retry);
	return ret;
}

int zero_bit_counter(uint8_t byte)
{
	int count = 0;

	byte = ~byte;
	while (byte) {
		byte &= byte - 1;
		++count;
	}
	return count;
}

int get_otp_counter(uint16_t byte)
{
	if (byte == FS1801_OTPPG2_DEFAULT) {
		return 0;
	}
	// OTP offset start from 0x10 to 0x20(Totally 16)
	return (LOW8(byte) - LOW8(FS1801_OTPPG2_DEFAULT) + 1);
}

void convert_data_to_bytes(uint32_t val, uint8_t *buf)
{
	buf[0] = (val >> 8) & 0xFF;
	buf[1] = (val) & 0xFF;
	buf[2] = (val >> 24) & 0xFF;
	buf[3] = (val >> 16) & 0xFF;
}

fsm_config_t *fsm_get_config(void)
{
	return &g_fsm_config;
}
#ifdef __KERNEL__
EXPORT_SYMBOL(fsm_get_config);
#endif

void fsm_get_version(fsm_version_t *version)
{
	sprintf(version->git_branch, "%s", FSM_GIT_BRANCH);
	sprintf(version->git_commit, "%s", FSM_GIT_COMMIT);
	sprintf(version->code_date, "%s", FSM_CODE_DATE);
	sprintf(version->code_version, "%s", FSM_CODE_VERSION);
	pr_info("version %s", version->code_version);
}

void *fsm_alloc_mem(int size)
{
#if defined(__KERNEL__)
	return kzalloc(size, GFP_KERNEL);
#elif defined(FSM_HAL_SUPPORT)
	return malloc(size);
#else
	return NULL;
#endif
}

void fsm_free_mem(void *buf)
{
	if (buf == NULL) {
		return;
	}

#if defined(__KERNEL__)
	kfree(buf);
#elif defined(FSM_HAL_SUPPORT)
	free(buf);
#endif
}

void fsm_delay_ms(uint32_t delay_ms)
{
	if (delay_ms == 0) {
		return;
	}
#if defined(__KERNEL__)
	usleep_range(delay_ms * 1000, delay_ms * 1000 + 1);
#elif defined(FSM_HAL_SUPPORT)
	usleep(delay_ms * 1000);
#endif
}

uint16_t set_bf_val(uint16_t *pval, const uint16_t bf, const uint16_t bf_val)
{
	uint8_t len = (bf >> 12) & 0x0F;
	uint8_t pos = (bf >> 8) & 0x0F;
	uint16_t new_val;
	uint16_t old_val;
	uint16_t msk;

	if (!pval) {
		return 0xFFFF;
	}
	old_val = new_val = *pval;
	msk = ((1 << (len + 1)) - 1) << pos;
	new_val &= ~msk;
	new_val |= bf_val << pos;
	*pval = new_val;

	return old_val;
}

uint16_t get_bf_val(const uint16_t bf, const uint16_t val)
{
	uint8_t len = (bf >> 12) & 0x0F;
	uint8_t pos = (bf >> 8) & 0x0F;
	uint16_t msk, value;

	msk = ((1 << (len + 1)) - 1) << pos;
	value = (val & msk) >> pos;

	return value;
}

int fsm_set_bf(fsm_dev_t *fsm_dev, const uint16_t bf, const uint16_t val)
{
	reg_unit_t reg;
	uint16_t oldval;
	uint16_t msk;
	int ret;

	if (fsm_dev == NULL) {
		return -EINVAL;
	}
	reg.len = (bf >> 12) & 0x0F;
	reg.pos = (bf >> 8) & 0x0F;
	reg.addr = bf & 0xFF;
	if (reg.len == 15) {
		return fsm_reg_write(fsm_dev, reg.addr, val);
	}
	ret = fsm_reg_read(fsm_dev, reg.addr, &oldval);
	if (ret) {
		pr_err("get bf:%04X failed", bf);
		return ret;
	}

	msk = ((1 << (reg.len + 1)) - 1) << reg.pos;
	reg.value = oldval & (~msk);
	reg.value |= val << reg.pos;

	if (oldval == reg.value) {
		return 0;
	}
	ret = fsm_reg_write(fsm_dev, reg.addr, reg.value);
	if (ret) {
		pr_err("set bf:%04X failed", bf);
		return ret;
	}

	return ret;
}

int fsm_get_bf(fsm_dev_t *fsm_dev, const uint16_t bf, uint16_t *pval)
{
	reg_unit_t reg;
	uint16_t msk;
	int ret;

	if (fsm_dev == NULL) {
		return -EINVAL;
	}
	reg.len = (bf >> 12) & 0x0F;
	reg.pos = (bf >> 8) & 0x0F;
	reg.addr = bf & 0xFF;
	ret = fsm_reg_multiread(fsm_dev, reg.addr, &reg.value);
	if (ret) {
		pr_err("get bf:%04X failed", bf);
		return ret;
	}
	msk = ((1 << (reg.len + 1)) - 1) << reg.pos;
	reg.value &= msk;
	if (pval) {
		*pval = reg.value >> reg.pos;
	}

	return ret;
}

struct fsm_dev *fsm_get_fsm_dev(uint8_t addr)
{
	fsm_config_t *cfg = fsm_get_config();
	fsm_dev_t *fsm_dev;

	if (!cfg || cfg->dev_count <= 0) {
		return NULL;
	}

	list_for_each_entry(fsm_dev, &fsm_dev_list, list) {
		if (fsm_dev && addr == fsm_dev->addr) {
			return fsm_dev;
		}
	}

	return NULL;
}

struct fsm_dev *fsm_get_fsm_dev_by_id(int id)
{
	fsm_config_t *cfg = fsm_get_config();
	fsm_dev_t *fsm_dev;

	if (!cfg || cfg->dev_count <= 0) {
		return NULL;
	}

	list_for_each_entry(fsm_dev, &fsm_dev_list, list) {
		if (fsm_dev && id == fsm_dev->id) {
			return fsm_dev;
		}
	}

	return NULL;
}

int fsm_reg_write(fsm_dev_t *fsm_dev, uint8_t reg, uint16_t val)
{
	int ret;

	if (fsm_dev == NULL) {
		return -EINVAL;
	}
#if defined(FSM_DEBUG_I2C)
	if (g_fsm_config.i2c_debug) {
		pr_addr(info, "%02X<-%04X", reg, val);
	}
#endif
#if defined(CONFIG_FSM_REGMAP)
	ret = fsm_regmap_write(fsm_dev, reg, val);
#elif defined(CONFIG_FSM_I2C)
	ret = fsm_i2c_reg_write(fsm_dev, reg, val);
#elif defined(FSM_HAL_SUPPORT)
	ret = fsm_hal_reg_write(fsm_dev, reg, val);
#else
	ret = -EINVAL;
#endif
	if (ret) {
		pr_addr(err, "%02X<-%04X fail:%d", reg, val, ret);
	}

	return ret;
}

int fsm_reg_read(fsm_dev_t *fsm_dev, uint8_t reg, uint16_t *pval)
{
	uint16_t value;
	int ret;

	if (fsm_dev == NULL) {
		return -EINVAL;
	}
#if defined(CONFIG_FSM_REGMAP)
	ret = fsm_regmap_read(fsm_dev, reg, &value);
#elif defined(CONFIG_FSM_I2C)
	ret = fsm_i2c_reg_read(fsm_dev, reg, &value);
#elif defined(FSM_HAL_SUPPORT)
	ret = fsm_hal_reg_read(fsm_dev, reg, &value);
#else
	ret = -EINVAL;
#endif
#if defined(FSM_DEBUG_I2C)
	if (g_fsm_config.i2c_debug) {
		pr_addr(info, " %02X->%04X", reg, value);
	}
#endif
	if (ret) {
		value = 0;
#if !defined(BUILD_FSTOOL)
		pr_addr(err, " %02X->%04X fail:%d", reg, value, ret);
#endif
	}
	if (pval) {
		*pval = value;
	}

	return ret;
}

int fsm_burst_write(fsm_dev_t *fsm_dev, uint8_t reg, uint8_t *data, int len)
{
	int ret;

	if (fsm_dev == NULL) {
		return -EINVAL;
	}
#if defined(FSM_DEBUG_I2C)
	if (len >= 4) {
		if (g_fsm_config.i2c_debug) {
			pr_addr(info, "%02X<-%02X %02X %02X %02X", reg,
					data[0], data[1], data[2], data[3]);
		}
	}
	// logprint("%s: %02X: %02X<-", __func__, fsm_dev->addr, reg);
	// for (ret = 0; ret < len; ret++) {
	// logprint("%02X ", data[ret]);
	// }
	// logprint("\n");
#endif

#if defined(CONFIG_FSM_REGMAP)
	ret = fsm_regmap_bulkwrite(fsm_dev, reg, data, len);
#elif defined(CONFIG_FSM_I2C)
	ret = fsm_i2c_bulkwrite(fsm_dev, reg, data, len);
#elif defined(FSM_HAL_SUPPORT)
	ret = fsm_hal_bulkwrite(fsm_dev, reg, data, len);
#else
	ret = -EINVAL;
#endif
	if (ret) {
		pr_addr(err, "BW %02X fail:%d", reg, ret);
	}

	return ret;
}

int fsm_reg_update_bits(fsm_dev_t *fsm_dev, reg_unit_t *reg)
{
	uint16_t temp;
	uint16_t mask;
	uint16_t val;
	uint8_t addr;
	int ret;

	if (!fsm_dev || !reg) {
		return -EINVAL;
	}
	addr = reg->addr;
	if (addr == REG(FSM_OTPACC)) {
		if (reg->value == 0 && fsm_dev->acc_count > 0) {
			fsm_dev->acc_count--;
		}
		if (reg->value != 0 && fsm_dev->acc_count < 0xFF) {
			fsm_dev->acc_count++;
		}
	}
	if (reg->pos == 0 && reg->len == 15) {
		return fsm_reg_write(fsm_dev, addr, reg->value);
	}
	mask = ((1 << (reg->len + 1)) - 1) << reg->pos;
	val = (reg->value << reg->pos);
	ret = fsm_reg_multiread(fsm_dev, addr, &temp);
	temp = ((~mask & temp) | (val & mask));
	ret |= fsm_reg_write(fsm_dev, addr, temp);

	return ret;
}

int fsm_reg_update(fsm_dev_t *fsm_dev, uint8_t reg, uint16_t val)
{
	uint16_t temp;
	int ret;

	ret = fsm_reg_multiread(fsm_dev, reg, &temp);
	if (!ret && temp != val) {
		ret = fsm_reg_write(fsm_dev, reg, val);
	}
	if (ret) {
		pr_addr(err, "update reg:%02X failed:%d", reg, ret);
	}

	return ret;
}

int fsm_reg_multiread(fsm_dev_t *fsm_dev, uint8_t reg, uint16_t *pval)
{
	uint16_t value;
	uint16_t old;
	int count = 0;
	int ret;

	if (fsm_dev == NULL) {
		return -EINVAL;
	}
	while (count++ < FSM_I2C_RETRY) {
		ret = fsm_reg_read(fsm_dev, reg, &value);
		if (ret)
			continue;
		if (count > 1 && old == value) {
			break;
		}
		old = value;
	}
	if (ret) {
		value = 0;
#if !defined(BUILD_FSTOOL)
		pr_addr(err, " %02X->%04X fail:%d", reg, value, ret);
#endif
	}
	if (pval) {
		*pval = value;
	}

	return ret;
}

uint16_t fsm_calc_checksum(uint16_t *data, int len)
{
	uint16_t crc = 0;
	uint8_t index;
	uint8_t b;
	int i;

	if (len <= 0) {
		return 0;
	}

	for (i = 0; i < len; i++) {
		b = (uint8_t)(data[i] & 0xFF);
		index = (uint8_t)(crc ^ b);
		crc = (uint16_t)((crc >> 8) ^ g_crc16table[index]);
		b = (uint8_t)((data[i] >> 8) & 0xFF);
		index = (uint8_t)(crc ^ b);
		crc = (uint16_t)((crc >> 8) ^ g_crc16table[index]);
	}
	return crc;
}

int fsm_get_srate_bits(fsm_dev_t *fsm_dev, uint32_t srate)
{
	int size;
	int idx;

	if (!fsm_dev) {
		return -EINVAL;
	}
	if (srate > 48000 && HIGH8(fsm_dev->version) != FS1860_DEV_ID) {
		pr_addr(err, "invalid srate:%d", srate);
		return -EINVAL;
	}
	if (srate == 32000 && fsm_dev->is1603s) {
		return 6; // I2SSR=6
	}
	size = sizeof(g_srate_tbl) / sizeof(struct fsm_srate);
	for (idx = 0; idx < size; idx++) {
		if (srate == g_srate_tbl[idx].srate)
			return g_srate_tbl[idx].bf_val;
	}
	return -EINVAL;
}

int fsm_access_key(fsm_dev_t *fsm_dev, int access)
{
	int ret = 0;

	if (!fsm_dev) {
		return -EINVAL;
	}
	if (access) {
		if (fsm_dev->acc_count == 0) {
			ret = fsm_reg_write(fsm_dev, REG(FSM_OTPACC), FSM_OTP_ACC_KEY2);
			fsm_dev->acc_count = !ret ? 1 : 0;
		} else if (fsm_dev->acc_count < 0xFF) {
			fsm_dev->acc_count++;
		}
	} else {
		if (fsm_dev->acc_count == 1) {
			ret = fsm_reg_write(fsm_dev, REG(FSM_OTPACC), 0);
			fsm_dev->acc_count = !ret ? 0 : 1;
		} else if (fsm_dev->acc_count > 0) {
			fsm_dev->acc_count--;
		}
	}
	// pr_addr(debug, "acc:%d, count:%d", access, fsm_dev->acc_count);

	return ret;
}

int fsm_reg_dump(fsm_dev_t *fsm_dev)
{
	char buf[LOG_BUF_SIZE];
	uint16_t value;
	int reg_addr;
	int idx = 0;
	int ret = 0;

	if (!fsm_dev) {
		return -EINVAL;
	}
	for (reg_addr = 0; reg_addr <= 0xE8; reg_addr++) {
		if (reg_addr == 0x0B)
			reg_addr = 0x46;
		else if (reg_addr == 0x49)
			reg_addr = 0x4D;
		else if (reg_addr == 0x50)
			reg_addr = 0x5B;
		else if (reg_addr == 0x6D)
			reg_addr = 0x89;
		else if (reg_addr == 0x8A)
			reg_addr = 0xA0;
		else if (reg_addr == 0xA2)
			reg_addr = 0xAF;
		else if (reg_addr == 0xB0)
			reg_addr = 0xBB;
		else if (reg_addr == 0xBE)
			reg_addr = 0xC0;
		else if (reg_addr == 0xC5)
			reg_addr = 0xC9;
		else if (reg_addr == 0xCE)
			reg_addr = 0xD0;
		else if (reg_addr == 0xD1)
			reg_addr = 0xD7;
		else if (reg_addr == 0xD8)
			reg_addr = 0xE8;
		if (reg_addr == 0xD0) {
			ret |= fsm_access_key(fsm_dev, 1);
		}
		ret |= fsm_reg_read(fsm_dev, reg_addr, &value);
		snprintf(buf + idx * 8, LOG_BUF_SIZE, "%02X:%04X ", reg_addr, value);
		idx++;
		if (idx % 8 == 0 || reg_addr == 0xE8) {
			buf[idx * 8 - 1] = '\0';
			pr_addr(info, "%s", buf);
			idx = 0;
		}
	}
	ret |= fsm_access_key(fsm_dev, 0);

	return ret;
}

static int fsm_search_list(fsm_dev_t *fsm_dev, struct preset_file *pfile)
{
	dev_list_t *dev_list;
	uint8_t *preset;
	uint16_t offset;
	uint16_t type;
	int idx;

	preset = (uint8_t *) pfile;
	for (idx = 0; idx < pfile->hdr.ndev; idx++) {
		type = pfile->index[idx].type;
		if (FSM_DSC_DEV_INFO != type) {
			continue;
		}
		offset = pfile->index[idx].offset;
		pr_addr(debug, "offset[%d]: %d", idx, offset);
		dev_list = (struct dev_list *) &preset[offset];
		if (fsm_dev->addr == dev_list->addr) {
			pr_addr(info, "found dev_list");
			fsm_dev->dev_list = dev_list;
			break;
		}
	}
	if (idx == pfile->hdr.ndev) {
		pr_addr(err, "not found dev_list");
		fsm_dev->dev_list = NULL;
		return -EINVAL;
	}

	return 0;
}

static int fsm_check_dev_type(fsm_dev_t *fsm_dev)
{
	uint16_t dev_type;

	if (!fsm_dev || !fsm_dev->dev_list) {
		pr_addr(err, "bad parameter");
		return -EINVAL;
	}
	if (!fsm_dev->dev_list) {
		pr_err("bad parameter");
		return -EINVAL;
	}

	dev_type = fsm_dev->dev_list->dev_type;
	if (HIGH8(fsm_dev->version) == 0x06 || HIGH8(fsm_dev->version) == 0x0B) {
		// fs1801 series
		if (HIGH8(dev_type) != 0x0A && HIGH8(dev_type) != 0x06
				&& HIGH8(dev_type) != 0x0B) {
			pr_addr(err, "invalid type:%04X", dev_type);
			return -EINVAL;
		}
		return 0;
	}
	// other series
	if (HIGH8(fsm_dev->version) != HIGH8(dev_type)) {
		pr_addr(err, "type:%02X not match version:%02X",
				HIGH8(dev_type), HIGH8(fsm_dev->version));
		return -EINVAL;
	}

	return 0;
}

int fsm_init_dev_list(fsm_dev_t *fsm_dev)
{
	struct preset_file *pfile;
	dev_list_t *dev_list;
	int ret;

	pfile = fsm_get_presets();
	if (!fsm_dev || !pfile) {
		pr_addr(err, "bad parameter or invalid FW");
		return -EINVAL;
	}

	pr_addr(debug, "ndev:%d", pfile->hdr.ndev);
	ret = fsm_search_list(fsm_dev, pfile);
	if (ret) {
		return ret;
	}
	dev_list = fsm_dev->dev_list;
	// assert(dev_list != NULL);
	ret = fsm_check_dev_type(fsm_dev);
	if (ret) {
		pr_addr(err, "type(%04X) not matched version(%04X)",
				dev_list->dev_type, fsm_dev->version);
		fsm_dev->dev_list = NULL;
		return ret;
	}
	pr_addr(info, "preset ver:%04X", dev_list->preset_ver);
	if ((dev_list->preset_ver & BIT(15)) == 0) {    // BIT_15 must be 1
		pr_addr(err, "invalid preset version:%04X",
				dev_list->preset_ver);
		return -EINVAL;
	}
	pr_addr(debug, "bus  : %d, len: %d",
			dev_list->bus, dev_list->len);
	pr_addr(debug, "type : %04X, npreset: %d",
			dev_list->dev_type, dev_list->npreset);
	pr_addr(debug, "scene: eq:%04X, reg:%04X",
			dev_list->eq_scenes, dev_list->reg_scenes);
	fsm_dev->own_scene = dev_list->reg_scenes | dev_list->eq_scenes;

	return 0;
}

void *fsm_get_list_by_idx(fsm_dev_t *fsm_dev, int idx)
{
	dev_list_t *dev_list;
	fsm_index_t *index;
	uint8_t *pdata;

	if (fsm_dev == NULL) {
		return NULL;
	}
	dev_list = fsm_dev->dev_list;
	if (dev_list == NULL) {
		return NULL;
	}
	index = &dev_list->index[0];
	pdata = (uint8_t *) index;

	return (void *)(&pdata[index[idx].offset]);
}

void *fsm_get_data_list(fsm_dev_t *fsm_dev, int type)
{
	dev_list_t *dev_list;
	fsm_index_t *index;
	int i;

	if (fsm_dev == NULL || fsm_dev->dev_list == NULL) {
		return NULL;
	}
	dev_list = fsm_dev->dev_list;
	index = dev_list->index;
	if (index == NULL) {
		return NULL;
	}
	for (i = 0; i < dev_list->len; i++) {
		if (index[i].type == type) {
			break;
		}
	}
	if (i >= dev_list->len) {
		return NULL;
	}

	return (void *)((uint8_t *) index + index[i].offset);
}

int fsm_get_spk_info(fsm_dev_t *fsm_dev, uint16_t info_type)
{
	info_list_t *info;

	if (fsm_dev == NULL) {
		return -EINVAL;
	}
	info = (info_list_t *) fsm_get_data_list(fsm_dev, FSM_DSC_SPK_INFO);
	if (!info || info_type > info->len) {
		return -EINVAL;
	}
	// pr_addr(debug, "spk info len: %d", info->len);

	return info->data[info_type];
}

int fsm_init_info(fsm_dev_t *fsm_dev)
{
	if (!fsm_dev || !fsm_dev->dev_list) {
		return -EINVAL;
	}
	fsm_dev->bus = fsm_dev->dev_list->bus;
	fsm_dev->addr = fsm_dev->dev_list->addr;
	fsm_dev->tmax = fsm_get_spk_info(fsm_dev, FSM_INFO_SPK_TMAX);
	fsm_dev->tcoef = fsm_get_spk_info(fsm_dev, FSM_INFO_SPK_TEMPR_COEF);
	fsm_dev->tsel = fsm_get_spk_info(fsm_dev, FSM_INFO_SPK_TEMPR_SEL);
	pr_addr(info, "tmax:%d, tcoef:%d, tsel:%d",
			fsm_dev->tmax, fsm_dev->tcoef, fsm_dev->tsel);
	fsm_dev->spkr = fsm_get_spk_info(fsm_dev, FSM_INFO_SPK_RES);
	fsm_dev->rapp = fsm_get_spk_info(fsm_dev, FSM_INFO_SPK_RAPP);
	if (IS_PRESET_V3(fsm_dev->dev_list->preset_ver)) {
		fsm_dev->pos_mask = fsm_get_spk_info(fsm_dev, FSM_INFO_SPK_POSITION);
		fsm_dev->compat.RS2RL_RATIO = \
									fsm_get_spk_info(fsm_dev, FSM_INFO_RSRL_RATIO);
	}
	pr_addr(info, "pos:%02X, rs_ratio:%d",
			fsm_dev->pos_mask, fsm_dev->compat.RS2RL_RATIO);
	pr_addr(info, "spkr:%d, rapp:%d",
			fsm_dev->spkr, fsm_dev->rapp);

	return 0;
}

void *fsm_get_presets(void)
{
	return g_fsm_config.preset;
}

void fsm_set_presets(void *preset)
{
	fsm_config_t *cfg = fsm_get_config();

	if (cfg) {
		fsm_free_mem(cfg->preset);
		cfg->preset = preset;
	}
}

/**
 * fsm device interface for external reference
 */
int fsm_parse_preset(const void *data, uint32_t size)
{
	struct preset_file *pfile;
	struct preset_header *hdr;
	uint16_t checksum;
	int crc_size;

	if (data == NULL || size == 0) {
		pr_err("bad parameter, size:%d", size);
		return -EINVAL;
	}
	pfile = fsm_get_presets();
	if (pfile) {
		pr_debug("already had presets, try init");
		fsm_try_init();
		return 0;
	}
	pfile = (struct preset_file *) fsm_alloc_mem(size);
	if (!pfile) {
		pr_err("alloc memery failed");
		return -ENOMEM;
	}
	memcpy(pfile, data, size);

	hdr = &pfile->hdr;
	pr_debug("version : %04X", hdr->version);
	pr_info("customer: %s", hdr->customer);
	pr_info("project : %s", hdr->project);
	pr_info("date    : %4d%02d%02d-%02d%02d", hdr->date.year, \
			hdr->date.month, hdr->date.day, hdr->date.hour, hdr->date.min);
	pr_debug("size    : %d", hdr->size);
	pr_debug("crc16   : %04X", hdr->crc16);

	crc_size = (size - sizeof(struct preset_header) + 2) / sizeof(uint16_t);
	if (hdr->size == 0 || hdr->size != size) {
		pr_err("invalid size: hdr:%d, fw:%d", hdr->size, size);
		return -EINVAL;
	}
	checksum = fsm_calc_checksum((uint16_t *)(&(pfile->hdr.ndev)), crc_size);
	if (checksum != hdr->crc16) {
		pr_err("checksum(%04X) not match(%04X)", checksum, hdr->crc16);
		return -EINVAL;
	} else {
		pr_info("checksum success!");
		fsm_set_presets(pfile);
	}
	fsm_try_init();

	return 0;
}

int fsm_swap_channel(fsm_dev_t *fsm_dev, int next_angle)
{
	uint16_t left_chn;
	uint16_t i2sctrl;
	uint16_t chs12;
	int ret;

	if (!fsm_dev) {
		return -EINVAL;
	}
	switch (next_angle) {
	case 90:
		left_chn = (FSM_POS_LBTM | FSM_POS_RBTM);
		break;
	case 180:
		left_chn = (FSM_POS_RTOP | FSM_POS_RBTM);
		break;
	case 270:
		left_chn = (FSM_POS_LTOP | FSM_POS_RTOP);
		break;
	case 0:
	default:
		left_chn = (FSM_POS_LTOP | FSM_POS_LBTM);
		break;
	}
	left_chn = (left_chn & fsm_dev->pos_mask);
	chs12 = ((left_chn != 0) ? 1 : 2);
	if (fsm_dev->pos_mask == FSM_POS_MONO) {
		chs12 = 3;
	}
	ret = fsm_reg_read(fsm_dev, REG(FSM_I2SCTRL), &i2sctrl);
	if (get_bf_val(FSM_CHS12, i2sctrl) == chs12) {
		return ret;
	}
	ret |= fsm_set_bf(fsm_dev, FSM_DACMUTE, 1);
	set_bf_val(&i2sctrl, FSM_CHS12, chs12);
	ret |= fsm_reg_write(fsm_dev, REG(FSM_I2SCTRL), i2sctrl);
	fsm_delay_ms(10);
	pr_addr(debug, "pos:%02X, CHS12:%d", fsm_dev->pos_mask, chs12);
	ret = fsm_set_bf(fsm_dev, FSM_DACMUTE, 0);

	return ret;
}

int fsm_wait_stable(fsm_dev_t *fsm_dev, int type)
{
	int retries = FSM_WAIT_STABLE_RETRY;
	int ret;

	if (fsm_dev == NULL) {
		return -EINVAL;
	}
	if (fsm_skip_device(fsm_dev)) {
		return 0;
	}
	// fsm_delay_ms(10);
	while (retries-- > 0) {
		fsm_delay_ms(5);  // delay 5 ms
		ret = fsm_stub_check_stable(fsm_dev, type);
		if (!ret)
			break;
	}
	if (retries <= 0) {
		pr_addr(err, "type: %d, wait timeout!", type);
		return -ETIMEDOUT;
	}
	return 0;
}

int fsm_set_spkset(fsm_dev_t *fsm_dev)
{
	int ret;

	if (fsm_dev == NULL) {
		return -EINVAL;
	}
	switch (fsm_dev->spkr) {
	case 4:
		ret = fsm_set_bf(fsm_dev, FSM_SPKR, 0);
		break;
	case 6:
		ret = fsm_set_bf(fsm_dev, FSM_SPKR, 1);
		break;
	case 7:
		ret = fsm_set_bf(fsm_dev, FSM_SPKR, 2);
		break;
	case 8:
	default:
		ret = fsm_set_bf(fsm_dev, FSM_SPKR, 3);
		break;
	}

	return ret;
}

int fsm_read_vbat(fsm_dev_t *fsm_dev, uint16_t *vbat)
{
	uint16_t batv;
	uint8_t ver;
	int ret;

	if (fsm_dev == NULL || !vbat) {
		return -EINVAL;
	}
	ret = fsm_get_bf(fsm_dev, FSM_BATV, &batv);
	ver = HIGH8(fsm_dev->version);
	switch (ver) {
	case FS1601S_DEV_ID:
	case FS1603_DEV_ID:
		*vbat = 10 * batv; // 10 mV per step
		break;
	case FS1818_DEV_ID:
	case FS1896_DEV_ID:
	case FS1860_DEV_ID:
		*vbat = 50 * batv / 4; // 50/4 mV per step
		break;
	default:
		pr_addr(err, "invalid dev_id:%02X", ver);
		return -EINVAL;
	}

	return ret;
}

static int fsm_get_vbat(fsm_dev_t *fsm_dev)
{
	struct fsm_vbat_state *vbat_state = &g_vbat_state;
	fsm_config_t *cfg = fsm_get_config();
	uint16_t batv;
	int ret;

	if (!cfg || fsm_dev == NULL || !vbat_state) {
		return -EINVAL;
	}
	ret = fsm_read_vbat(fsm_dev, &batv);
	if (ret) {
		return ret;
	}
	// pr_addr(info, "batv:%d", batv);
	// get min batv of all devices
	if (vbat_state->cur_batv == 0 || batv < vbat_state->cur_batv) {
		vbat_state->cur_batv = batv;
	}

	return ret;
}

int fsm_write_stereo_ceof(fsm_dev_t *fsm_dev)
{
	stereo_coef_t *stereo_coef;
	int ret = 0;
	int i;

	if (fsm_dev == NULL) {
		return -EINVAL;
	}
	if (fsm_dev->is1801) {
		return 0;
	}
	stereo_coef = (stereo_coef_t *) fsm_get_data_list(fsm_dev, FSM_DSC_STEREO_COEF);
	if (!stereo_coef) {
		pr_addr(info, "note: not found data");
		return -EINVAL;
	}
	pr_addr(debug, "stereo coef len: %d", stereo_coef->len);
	for (i = 0; i < stereo_coef->len - 2; i++) {
		ret |= fsm_reg_write(fsm_dev, REG(FSM_STERC1) + i, stereo_coef->data[i]);
	}
	ret |= fsm_reg_write(fsm_dev, REG(FSM_STERCTRL), stereo_coef->data[i++]);
	ret |= fsm_reg_write(fsm_dev, REG(FSM_STERGAIN), stereo_coef->data[i++]);

	FSM_FUNC_EXIT(ret);
	return ret;
}

int fsm_write_excer_ram(fsm_dev_t *fsm_dev)
{
	ram_data_t *excer_ram;
	uint8_t write_buf[4];
	int ret;
	int i;

	if (fsm_dev == NULL) {
		return -EINVAL;
	}
	excer_ram = fsm_get_data_list(fsm_dev, FSM_DSC_EXCER_RAM);
	if (!excer_ram) {
		pr_addr(err, "note: not found data");
		return -EINVAL;
	}
	if (excer_ram->len != fsm_dev->compat.excer_ram_len) {
		pr_addr(err, "invalid size: %d", excer_ram->len);
		return -EINVAL;
	}
	pr_addr(debug, "excer ram len: %d", excer_ram->len);
	ret = fsm_reg_write(fsm_dev, fsm_dev->compat.DACEQA,
						fsm_dev->compat.addr_excer_ram);
	for (i = 0; i < excer_ram->len; i++) {
		convert_data_to_bytes(excer_ram->data[i], write_buf);
		ret |= fsm_burst_write(fsm_dev, fsm_dev->compat.DACEQWL,
								write_buf, sizeof(uint32_t));
	}

	return ret;
}

int fsm_write_reg_tbl(fsm_dev_t *fsm_dev, uint16_t scene)
{
	reg_scene_t *reg_scene;
	reg_comm_t *reg_comm;
	reg_unit_t *reg;
	regs_unit_t *regs;
	int ret = 0;
	int i;

	if (fsm_dev == NULL) {
		return -EINVAL;
	}
	if (scene == FSM_SCENE_COMMON) {
		reg_comm = fsm_get_data_list(fsm_dev, FSM_DSC_REG_COMMON);
		if (!reg_comm) {
			pr_addr(info, "note: not found reg_common data");
			return 0;
		}
		reg = reg_comm->reg;
		pr_addr(debug, "reg comm len: %d", reg_comm->len);
		for (i = 0; i < reg_comm->len; i++) {
			ret |= fsm_reg_update_bits(fsm_dev, &reg[i]);
		}
	} else if (scene != FSM_SCENE_UNKNOW) {
		reg_scene = fsm_get_data_list(fsm_dev, FSM_DSC_REG_SCENES);
		if (!reg_scene) {
			pr_addr(info, "note: not found reg_scene data");
			return 0;
		}
		regs = reg_scene->regs;
		pr_addr(debug, "reg scene len: %d", reg_scene->len);
		for (i = 0; i < reg_scene->len; i++) {
			if ((regs[i].scene & scene) == 0) {
				continue;
			}
			ret |= fsm_reg_update_bits(fsm_dev, &regs[i].reg);
		}
	}

	FSM_FUNC_EXIT(ret);
	return ret;
}

static int parse_otp_write_count(fsm_dev_t *fsm_dev, uint16_t valOTP)
{
	uint16_t count = 0;
	uint8_t devid;

	if (fsm_dev == NULL) {
		return -EINVAL;
	}
	devid = HIGH8(fsm_dev->version);
	switch (devid) {
	case FS1603_DEV_ID:
		count = zero_bit_counter(valOTP & 0xFF);
		break;
	case FS1818_DEV_ID:
	case FS1896_DEV_ID:
		count = get_otp_counter(valOTP);
		break;
	default:
		pr_addr(err, "invalid DEVID:%02X", devid);
		break;
	}
	return count;
}

int fsm_write_nondsp_cfg(fsm_dev_t *fsm_dev)
{
	int ret;

	if (fsm_dev == NULL) {
		return -EINVAL;
	}
	if (!IS_FS1603S(fsm_dev->version)) {
		pr_info("not support id: %04X", fsm_dev->version);
		return 0;
	}
	ret = fsm_access_key(fsm_dev, 1);
	// adp mode, boost enable
	ret |= fsm_reg_write(fsm_dev, 0xC0, 0xBFAE);
	// bypass DSP
	ret |= fsm_reg_write(fsm_dev, 0xA1, 0x0C12);
	// bypass DCR
	ret |= fsm_reg_write(fsm_dev, 0xD7, 0x1020);
	// enable DSP, bypass EQ, post EQ, disable notch filter
	ret |= fsm_reg_write(fsm_dev, 0xA1, 0x1002);
	// BFL SET
	ret |= fsm_reg_write(fsm_dev, 0xA8, 0x00F7);
	// AGC SET
	ret |= fsm_reg_write(fsm_dev, 0xAA, 0x007F);
	// bypass EQ band0 and band1
	ret |= fsm_reg_write(fsm_dev, 0xB3, 0x1010);
	// disable acs, bypass EQ, disable mbdrc
	ret |= fsm_reg_write(fsm_dev, 0x89, 0x0000);
	// ret |= fsm_reg_write(fsm_dev, 0xAF, 0x162B);

	ret |= fsm_reg_write(fsm_dev, 0xA0, 0xD408);
	// ret |= fsm_reg_write(fsm_dev, 0x09, 0x0000);
	// fade enable, unmute
	// ret |= fsm_reg_write(fsm_dev, 0xAE, 0x0200);
	// d2a config
	ret |= fsm_reg_write(fsm_dev, 0xE3, 0x73C3);
	// enable I2S data output now
	// ret |= fsm_set_bf(fsm_dev, FSM_I2SDOE, 1);

	ret |= fsm_access_key(fsm_dev, 0);

	return ret;
}

int fsm_write_preset_eq(fsm_dev_t *fsm_dev,
						int ram_id, uint16_t scene)
{
	uint8_t write_buf[sizeof(uint32_t)];
	preset_list_t *preset_list = NULL;
	dev_list_t *dev_list;
	int ret;
	int i;

	if (fsm_dev == NULL || fsm_dev->dev_list == NULL) {
		return -EINVAL;
	}
	if ((fsm_dev->ram_scene[ram_id] != 0xFFFF)
			&& ((fsm_dev->ram_scene[ram_id] & scene) != 0)) {
		pr_addr(info, "RAM%d scene:%04X", ram_id,
				fsm_dev->ram_scene[ram_id]);
		return 0;
	}
	dev_list = fsm_dev->dev_list;
	if ((dev_list->eq_scenes & scene) == 0) {
		pr_addr(warning, "scene:%04X None-EQ", scene);
		if ((dev_list->reg_scenes & scene) != 0) {
			pr_addr(info, "scene:%04X has REG-TBL", scene);
			return 0;
		}
		return 0; // scene_1 maybe none
	}
	for (i = 0; i < dev_list->len; i++) {
		if (dev_list->index[i].type != FSM_DSC_PRESET_EQ) {
			continue;
		}
		preset_list = (preset_list_t *)((uint8_t *) dev_list->index
								+ dev_list->index[i].offset);
		if (preset_list && preset_list->scene & scene) {
			break;
		}
	}
	// pr_addr(debug, "preset offset: %04x", index[i].offset);
	if (!preset_list || i >= dev_list->len) {
		pr_addr(err, "not found preset: %04X", scene);
		return -EINVAL;
	}

	if (preset_list->len != fsm_dev->compat.preset_unit_len) {
		pr_addr(err, "invalid size: %d", preset_list->len);
		return -EINVAL;
	}
	ret = fsm_reg_write(fsm_dev, fsm_dev->compat.ACSEQA,
					((ram_id == FSM_EQ_RAM0) ? 0 : preset_list->len));
	for (i = 0; i < preset_list->len; i++) {
		convert_data_to_bytes(preset_list->data[i], write_buf);
		ret |= fsm_burst_write(fsm_dev, fsm_dev->compat.ACSEQWL,
								write_buf, sizeof(uint32_t));
	}
	if (!ret) {
		fsm_dev->ram_scene[ram_id] = preset_list->scene;
		pr_addr(info, "wroten ram_scene[%d]:%04X",
				ram_id, preset_list->scene);
	}

	FSM_FUNC_EXIT(ret);
	return ret;
}

int fsm_switch_preset(fsm_dev_t *fsm_dev)
{
	fsm_config_t *cfg = fsm_get_config();
	reg_temp_t sysctrl;
	reg_temp_t pllc4;
	uint16_t dspen;
	int ret;

	if (fsm_dev == NULL) {
		pr_err("fsm_dev is NULL ");
		return -EINVAL;
	}

	pr_addr(debug, "%s switching",
			(cfg->force_scene ? "force" : "auto"));

	if (!cfg->force_scene && fsm_dev->cur_scene == cfg->next_scene) {
		pr_addr(debug, "same scene, skip");
		return 0;
	}
	// need amplifier off
	ret = fsm_reg_read(fsm_dev, REG(FSM_SYSCTRL), &sysctrl.new_val);
	sysctrl.old_val = set_bf_val(&sysctrl.new_val, FSM_AMPE, 0);
	ret |= fsm_reg_write(fsm_dev, REG(FSM_SYSCTRL), sysctrl.new_val);
	// wait stable
	ret |= fsm_wait_stable(fsm_dev, FSM_WAIT_AMP_OFF);
	if (ret) {
		pr_addr(err, "wait timeout!");
		return ret;
	}
	// enable pll osc
	ret = fsm_reg_read(fsm_dev, REG(FSM_PLLCTRL4), &pllc4.new_val);
	pllc4.old_val = set_bf_val(&pllc4.new_val, FSM_OSCEN, 1);
	ret |= fsm_reg_write(fsm_dev, REG(FSM_PLLCTRL4), pllc4.new_val);
	fsm_delay_ms(5);  // 5ms

	// load reg table
	ret |= fsm_write_reg_tbl(fsm_dev, cfg->next_scene);

	ret |= fsm_get_bf(fsm_dev, FSM_DSPEN, &dspen);
	if (dspen == 0) { // bypass DSP
		pr_addr(info, "note: Bypass DSP mode");
		fsm_dev->state.bypass_dsp = true;
		ret |= fsm_reg_write(fsm_dev, REG(FSM_PLLCTRL4), pllc4.old_val);
		ret |= fsm_reg_write(fsm_dev, REG(FSM_SYSCTRL), sysctrl.old_val);
		fsm_dev->cur_scene = cfg->next_scene;
		return 0;
	}
	fsm_dev->state.bypass_dsp = false;
	if (cfg->nondsp_mode) {
		ret |= fsm_write_nondsp_cfg(fsm_dev);
		ret |= fsm_reg_write(fsm_dev, REG(FSM_PLLCTRL4), pllc4.old_val);
		ret |= fsm_reg_write(fsm_dev, REG(FSM_SYSCTRL), sysctrl.old_val);
		fsm_dev->cur_scene = cfg->next_scene;
		return 0;
	}

	ret |= fsm_access_key(fsm_dev, 1);
	if (cfg->next_scene & fsm_dev->ram_scene[FSM_EQ_RAM0]) {
		ret |= fsm_reg_write(fsm_dev, REG(FSM_ACSCTRL), 0x9880);   // eq ram0
	} else if (cfg->next_scene & fsm_dev->ram_scene[FSM_EQ_RAM1]) {
		ret |= fsm_reg_write(fsm_dev, REG(FSM_ACSCTRL), 0x9890);   // eq ram1
	} else if (fsm_dev->dev_list->eq_scenes & cfg->next_scene) {
		// use ram1 to switch preset
		ret |= fsm_write_preset_eq(fsm_dev, FSM_EQ_RAM1, cfg->next_scene);
		ret |= fsm_reg_write(fsm_dev, REG(FSM_ACSCTRL), 0x9890);   // eq ram1
		if (ret) {
			pr_addr(err, "update scene[%04X] fail:%d",
					cfg->next_scene, ret);
		}
	}
	ret |= fsm_access_key(fsm_dev, 0);
	if (!ret) {
		fsm_dev->cur_scene = cfg->next_scene;
		pr_addr(info, "switched scene:%04X", cfg->next_scene);
	} else {
		fsm_dev->cur_scene = FSM_SCENE_UNKNOW;
	}

	// recover pll and sysctrl
	ret |= fsm_reg_write(fsm_dev, REG(FSM_PLLCTRL4), pllc4.old_val);
	ret |= fsm_reg_write(fsm_dev, REG(FSM_SYSCTRL), sysctrl.old_val);
	// ret |= fsm_reg_read(fsm_dev, REG(FSM_STATUS), NULL);//TODO

	FSM_FUNC_EXIT(ret);
	return ret;
}

int fsm_write_preset(fsm_dev_t *fsm_dev)
{
	fsm_config_t *cfg = fsm_get_config();
	reg_temp_t sysctrl;
	int ret;

	if (!cfg || !fsm_dev || fsm_dev->own_scene == FSM_SCENE_UNKNOW) {
		return -EINVAL;
	}

	if (cfg->force_scene) {
		fsm_dev->cur_scene = FSM_SCENE_UNKNOW;
		fsm_dev->ram_scene[FSM_EQ_RAM0] = FSM_SCENE_UNKNOW;
		fsm_dev->ram_scene[FSM_EQ_RAM1] = FSM_SCENE_UNKNOW;
	}
	// need amplifier off, pll enable, power on
	ret = fsm_reg_read(fsm_dev, REG(FSM_SYSCTRL), &sysctrl.new_val);
	sysctrl.old_val = set_bf_val(&sysctrl.new_val, FSM_AMPE, 0);
	ret |= fsm_reg_write(fsm_dev, REG(FSM_SYSCTRL), sysctrl.new_val);

	ret |= fsm_set_spkset(fsm_dev);
	ret |= fsm_reg_write(fsm_dev, REG(FSM_TEMPSEL), (fsm_dev->tcoef << 1));
	ret = fsm_wait_stable(fsm_dev, FSM_WAIT_AMP_OFF);
	if (ret) {
		pr_addr(err, "wait timeout!");
		return ret;
	}

	ret |= fsm_access_key(fsm_dev, 1);
	if (!cfg->nondsp_mode) {
		ret |= fsm_write_stereo_ceof(fsm_dev);
		ret |= fsm_write_excer_ram(fsm_dev);
		// use music and voice scene config as default
		ret |= fsm_write_preset_eq(fsm_dev, FSM_EQ_RAM0, FSM_SCENE_MUSIC);
		ret |= fsm_write_preset_eq(fsm_dev, FSM_EQ_RAM1, FSM_SCENE_VOICE);
	}
	ret |= fsm_write_reg_tbl(fsm_dev, FSM_SCENE_COMMON);
	ret |= fsm_reg_write(fsm_dev, 0x98, 0x0F11);  // disable lnm
	if (fsm_dev->version == FS1603S_V1) {
		ret |= fsm_reg_update(fsm_dev, 0xC7, 0x5350);
	}
	ret |= fsm_access_key(fsm_dev, 0);

	// ret |= fsm_reg_write(fsm_dev, REG(FSM_PLLCTRL4), 0);

	ret |= fsm_switch_preset(fsm_dev);
	ret |= fsm_reg_write(fsm_dev, REG(FSM_SYSCTRL), sysctrl.old_val);

	FSM_FUNC_EXIT(ret);
	return ret;
}

static int fsm_set_tsctrl(fsm_dev_t *fsm_dev,
							bool enable, bool auto_off)
{
	uint16_t tsctrl;
	int ret;

	if (fsm_dev == NULL) {
		return -EINVAL;
	}
	ret = fsm_reg_multiread(fsm_dev, REG(FSM_TSCTRL), &tsctrl);
	set_bf_val(&tsctrl, FSM_TSEN, enable);
	set_bf_val(&tsctrl, FSM_OFF_AUTOEN, auto_off);
	ret |= fsm_reg_write(fsm_dev, REG(FSM_TSCTRL), tsctrl);

	FSM_FUNC_EXIT(ret);
	return ret;
}

static uint16_t fsm_cal_threshold(fsm_dev_t *fsm_dev,
									int mt_type, uint16_t zmdata)
{
	int16_t mt_tempr;
	uint32_t result;
	uint32_t spk_rt;
	int rapp;

	if (fsm_dev == NULL) {
		return -EINVAL;
	}
	mt_tempr = fsm_get_spk_info(fsm_dev, mt_type);
	pr_addr(info, "MT[%d]:%d", mt_type, mt_tempr);
	if (mt_tempr <= 0) {
		pr_addr(err, "get MT[%d] info failed", mt_type);
		return 0;
	}
	result = (uint32_t) zmdata * FSM_MAGNIF_TEMPR_COEF;
	result = result / (FSM_MAGNIF_TEMPR_COEF + (uint32_t) fsm_dev->tcoef *
					(mt_tempr * fsm_dev->tmax / 100 - (fsm_dev->tsel >> 1)));
	rapp = fsm_get_spk_info(fsm_dev, FSM_INFO_SPK_RAPP);
	spk_rt = fsm_cal_spkr_zmimp(fsm_dev, result);
	if (rapp > 0) {
		pr_addr(info, "rapp:%d, zm:%X", rapp, result);
		spk_rt += rapp;
		result = fsm_cal_spkr_zmimp(fsm_dev, spk_rt);
	}
	pr_addr(info, "MT[%3d]:%X, rt:%d", mt_tempr, result, spk_rt);

	return (uint16_t) result;
}

static int fsm_calc_step_unit(fsm_dev_t *fsm_dev)
{
	int step_unit;
	int spkr_mag;

	if (fsm_dev == NULL) {
		return 0;
	}
	spkr_mag = FSM_MAGNIF(fsm_dev->spkr);
	if (fsm_dev->spkr <= 10) {
		step_unit = spkr_mag * FSM_SPKR_ALLOWANCE / 12700;
	} else {
		step_unit = spkr_mag * FSM_RCVR_ALLOWANCE / 12700;
	}
	step_unit += 1; // fix integer precision
	pr_addr(info, "spkr:%d, step:%d", fsm_dev->spkr, step_unit);

	return step_unit;
}

static int fsm_check_re25_range(fsm_dev_t *fsm_dev, int re25)
{
	int zmimp_min;
	int step_unit;
	int spkr_mag;
	int re25_min;
	int re25_max;

	if (fsm_dev == NULL) {
		return -EINVAL;
	}
	spkr_mag = FSM_MAGNIF(fsm_dev->spkr);
	step_unit = fsm_calc_step_unit(fsm_dev);
	re25_min = spkr_mag - 0x7F * step_unit;
	zmimp_min = fsm_cal_spkr_zmimp(fsm_dev, 0xFFF0);
	if (re25_min < zmimp_min) {
		re25_min = zmimp_min;
	}
	re25_max = spkr_mag + 0x7F * step_unit;
	pr_addr(info, "spkr:%d, min:%d, max:%d",
			fsm_dev->spkr, re25_min, re25_max);
	if (re25 < re25_min || re25 > re25_max) {
		pr_addr(err, "invalid re25:%d", re25);
		fsm_dev->state.calibrated = false;
		return -EINVAL;
	}

	return 0;
}

static int fsm_set_threshold_v1(fsm_dev_t *fsm_dev, uint16_t zmdata)
{
	uint16_t value;
	uint16_t temp;
	int ret;

	if (fsm_dev == NULL) {
		return -EINVAL;
	}
	value = fsm_cal_threshold(fsm_dev, FSM_INFO_SPK_TRE, zmdata);
	ret = fsm_reg_write(fsm_dev, REG(FSM_SPKRE), value);
	value = fsm_cal_threshold(fsm_dev, FSM_INFO_SPK_TM6, zmdata);
	ret |= fsm_reg_write(fsm_dev, REG(FSM_SPKM6), value);
	temp = (((value & 0x3C00) >> 2) & 0x0F00);   // get bit[13..10] to bit[11..8]
	value = fsm_cal_threshold(fsm_dev, FSM_INFO_SPK_TM24, zmdata);
	ret |= fsm_reg_write(fsm_dev, REG(FSM_SPKM24), value);
	temp |= (((value & 0x3C00) << 2) & 0xF000);   // get bit[13..10] to bit[15..12]
	value = fsm_cal_threshold(fsm_dev, FSM_INFO_SPK_TERR, zmdata);
	ret |= fsm_reg_write(fsm_dev, REG(FSM_SPKERR), value);
	if (fsm_dev->is1603tu) {
		ret |= fsm_access_key(fsm_dev, 1);
		ret |= fsm_reg_read(fsm_dev, 0xE4, &value);  // get E4 value
		value = (value & 0x00FF) | temp; // update bit[15..8]
		pr_addr(info, "update for fs1603tu:%04X", value);
		ret |= fsm_reg_write(fsm_dev, 0xE4, value);  // update E4 value
		ret |= fsm_access_key(fsm_dev, 0);
	}

	return ret;
}

static int fsm_set_threshold_v2(fsm_dev_t *fsm_dev, uint16_t zmdata)
{
	uint16_t value;
	int ret;

	if (fsm_dev == NULL) {
		return -EINVAL;
	}
	value = fsm_cal_threshold(fsm_dev, FSM_INFO_SPK_TRE, zmdata);
	ret = fsm_reg_write(fsm_dev, REG(FSM_SPKRE), value);
	value = fsm_cal_threshold(fsm_dev, FSM_INFO_SPK_TM01, zmdata);
	ret |= fsm_reg_write(fsm_dev, REG(FSM_SPKMT01), value);
	value = fsm_cal_threshold(fsm_dev, FSM_INFO_SPK_TM02, zmdata);
	ret |= fsm_reg_write(fsm_dev, REG(FSM_SPKMT02), value);
	value = fsm_cal_threshold(fsm_dev, FSM_INFO_SPK_TM03, zmdata);
	ret |= fsm_reg_write(fsm_dev, REG(FSM_SPKMT03), value);
	value = fsm_cal_threshold(fsm_dev, FSM_INFO_SPK_TM04, zmdata);
	ret |= fsm_reg_write(fsm_dev, REG(FSM_SPKMT04), value);
	value = fsm_cal_threshold(fsm_dev, FSM_INFO_SPK_TM05, zmdata);
	ret |= fsm_reg_write(fsm_dev, REG(FSM_SPKMT05), value);
	value = fsm_cal_threshold(fsm_dev, FSM_INFO_SPK_TM06, zmdata);
	ret |= fsm_reg_write(fsm_dev, REG(FSM_SPKMT06), value);
	value = fsm_cal_threshold(fsm_dev, FSM_INFO_SPK_TERR, zmdata);
	ret |= fsm_reg_write(fsm_dev, REG(FSM_SPKERR), value);

	return ret;
}

uint32_t fsm_cal_spkr_zmimp(fsm_dev_t *fsm_dev, uint16_t data)
{
	uint32_t result;

	if (!fsm_dev) {
		return 0xFFFF;
	}
	if (fsm_dev->compat.RS2RL_RATIO == 0) {
		pr_addr(info, "invalid rs ratio");
		return 0;
	}
	if (data == 0) { // invalid data
		return 0xFFFF;
	}
	result = FSM_MAGNIF(fsm_dev->compat.RS2RL_RATIO * LOW8(fsm_dev->rstrim));
	result = (result / data);

	return result;
}

static int fsm_set_threshold(fsm_dev_t *fsm_dev,
							uint16_t data, int type)
{
	struct fsm_calib calib;
	uint16_t value;
	int temp_val;
	int rapp;
	int ret;

	if (fsm_dev == NULL) {
		return -EINVAL;
	}
	ret = fsm_reg_read(fsm_dev, REG(FSM_OTPPG1W2), &value);
	if (ret || value == 0) {
		pr_warning("use default rs_trim value");
		value = FSM_RS_TRIM_DEFAULT;
	}
	fsm_dev->rstrim = value;
	temp_val = fsm_cal_spkr_zmimp(fsm_dev, data);

	if (type == FSM_DATA_TYPE_ZMDATA) {
		rapp = fsm_get_spk_info(fsm_dev, FSM_INFO_SPK_RAPP);
		calib.cal_zm = data;
		calib.re25 = ((data == 0xFFFF) ? 2048 : temp_val);
		if (rapp > 0) {
			pr_addr(info, "re25(has rapp:%d):%d", rapp, calib.re25);
			calib.re25 -= rapp;
			// update cal_zm by re25 without rapp
			calib.cal_zm = fsm_cal_spkr_zmimp(fsm_dev, calib.re25);
		}
	} else {
		calib.cal_zm = temp_val;
		calib.re25 = data;
	}
	pr_addr(info, "zm:%04X, re25:%d", calib.cal_zm, calib.re25);

	fsm_dev->re25 = calib.re25;
	ret |= fsm_check_re25_range(fsm_dev, calib.re25);
	if (ret) {
		pr_err("check re25 fail:%d", ret);
		fsm_dev->re25 = calib.re25 = 0;
		fsm_dev->state.calibrated = false;
		fsm_reg_write(fsm_dev, REG(FSM_ADCTIME), 0x0031);
		return ret;
	}
	fsm_dev->state.calibrated = true;
	if (fsm_dev->is1603s && IS_PRESET_V3(fsm_dev->dev_list->preset_ver)) {
		ret |= fsm_set_threshold_v2(fsm_dev, calib.cal_zm);
	} else {
		ret |= fsm_set_threshold_v1(fsm_dev, calib.cal_zm);
	}

	ret |= fsm_reg_write(fsm_dev, REG(FSM_ADCTIME), 0x0031);

	FSM_FUNC_EXIT(ret);
	return ret;
}

int fsm_parse_otp(fsm_dev_t *fsm_dev, uint16_t value,
					int *re25, int *count)
{
	uint8_t byte;
	int step_unit;
	int cal_count;
	int spkr_mag;
	int offset;

	if (fsm_dev == NULL || re25 == NULL || count == NULL) {
		pr_err("bad parameters");
		return -EINVAL;
	}

	// parse calibration count
	cal_count = parse_otp_write_count(fsm_dev, value);
	pr_addr(info, "cal_count:%d", cal_count);
	fsm_dev->state.otp_stored = ((cal_count > 0) ? 1 : 0);
	*count = cal_count;
	if (cal_count <= 0) {
		*re25 = 0;
		return 0;
	}
	// parse re25
	if (re25 != NULL) {
		byte = (uint8_t)((value >> 8) & 0xFF);
		spkr_mag = FSM_MAGNIF(fsm_dev->spkr);
		step_unit = fsm_calc_step_unit(fsm_dev);
		offset = (byte & 0x7F) * step_unit;
		if ((byte & 0x80) != 0) {
			*re25 = spkr_mag - offset;
		} else {
			*re25 = spkr_mag + offset;
		}
		pr_addr(info, "offset:%d, step:%d, re25:%d mohm",
				offset, step_unit, *re25);
	}

	return 0;
}

int fsm_check_otp(fsm_dev_t *fsm_dev)
{
	uint16_t otppg2;
	uint16_t pllc4;
	int count;
	int re25;
	int ret;

	if (fsm_dev == NULL) {
		return -EINVAL;
	}
	ret = fsm_reg_read(fsm_dev, REG(FSM_PLLCTRL4), &pllc4);
	ret |= fsm_reg_write(fsm_dev, REG(FSM_PLLCTRL4), 0x000F);
	ret |= fsm_access_key(fsm_dev, 1);
	ret |= fsm_reg_read(fsm_dev, REG(FSM_OTPPG2), &otppg2);
	pr_addr(debug, "OTPPG2:%04X", otppg2);
	fsm_parse_otp(fsm_dev, otppg2, &re25, &count);
	fsm_dev->cal_count = count;

	if (count > 0 && count <= fsm_dev->compat.otp_max_count) {
		fsm_set_threshold(fsm_dev, re25, FSM_DATA_TYPE_RE25);
	} else if (count == 0) {
		// not calibrated yet, use default re25 of dts
		re25 = fsm_dev->re25_dft;
		pr_addr(info, "not calirated, use re25_dft:%d", re25);
		fsm_set_threshold(fsm_dev, re25, FSM_DATA_TYPE_RE25);
	} else {
		pr_addr(warning, "otp write count max");
		ret = -9527; // count limit
	}
	ret |= fsm_access_key(fsm_dev, 0);
	ret |= fsm_reg_write(fsm_dev, REG(FSM_PLLCTRL4), pllc4);

	FSM_FUNC_EXIT(ret);
	return ret;
}

int fsm_config_vol(fsm_dev_t *fsm_dev)
{
	fsm_config_t *cfg = fsm_get_config();
	uint16_t volume;

	if (!fsm_dev || !cfg) {
		return -EINVAL;
	}

	if (cfg->volume > FSM_VOLUME_MAX) {
		cfg->volume = FSM_VOLUME_MAX;
	}
	if (fsm_dev->re25_dft == 0 && !fsm_dev->state.calibrated) {
		volume = 0xDF; // -12dB
	} else {
		volume = cfg->volume;
	}
	volume = ((volume << 8) & 0xFF00);
	pr_addr(debug, "vol: %04X", volume);

	return fsm_reg_write(fsm_dev, REG(FSM_AUDIOCTRL), volume);
}

static uint8_t fsm_re25_to_byte(fsm_dev_t *fsm_dev, int re25)
{
	uint8_t valOTP;
	int step_unit;
	int spkr_mag;
	int offset;

	if (fsm_dev == NULL || re25 == 0) {
		return 0xFF;
	}

	spkr_mag = FSM_MAGNIF(fsm_dev->spkr);
	step_unit = fsm_calc_step_unit(fsm_dev);
	offset = re25 - spkr_mag;
	if (offset < 0) {
		offset *= -1;
		valOTP = (uint8_t)((offset / step_unit) & 0xFF) + 1;
		if (valOTP > 0x7F) {
			valOTP = 0x7F;
		}
		valOTP |= 0x80;
	} else {
		valOTP = (uint8_t)((offset / step_unit) & 0xFF) - 1;
		if (offset < step_unit) {
			valOTP = 0x00;
		}
		if (valOTP > 0x7F) {
			valOTP = 0x7F;
		}
	}
	pr_addr(info, "re25:%d mohm, step:%d, offset:%d",
			re25, step_unit, offset);

	return valOTP;
}

static int fsm_switch_cal_scene(fsm_dev_t *fsm_dev)
{
	reg_temp_t sysctrl;
	uint16_t bstctrl;
	int ret;

	if (!fsm_dev) {
		return -EINVAL;
	}
	// need amplifier off
	ret = fsm_reg_read(fsm_dev, REG(FSM_SYSCTRL), &sysctrl.new_val);
	sysctrl.old_val = set_bf_val(&sysctrl.new_val, FSM_AMPE, 0);
	ret |= fsm_reg_write(fsm_dev, REG(FSM_SYSCTRL), sysctrl.new_val);
	// wait stable
	ret |= fsm_wait_stable(fsm_dev, FSM_WAIT_AMP_OFF);
	if (ret) {
		pr_addr(err, "wait timeout!");
		return ret;
	}
	// boost mode
	ret = fsm_reg_read(fsm_dev, REG(FSM_BSTCTRL), &bstctrl);
	set_bf_val(&bstctrl, FSM_BSTEN, 1);
	set_bf_val(&bstctrl, FSM_DAC_GAIN, 3);
	if (fsm_dev->is1603s) {
		set_bf_val(&bstctrl, FSM_VOUT_SEL, 0);  // 6V
		set_bf_val(&bstctrl, FSM_MODE_CTRL, 1);  // boost
		// disable lnm
		ret |= fsm_set_bf(fsm_dev, FSM_LNM_EN, 0);
	} else {
		set_bf_val(&bstctrl, FSM_MODE_CTRL, 0);  // follow
	}
	ret |= fsm_reg_write(fsm_dev, REG(FSM_BSTCTRL), bstctrl);
	// dsp mode
	ret |= fsm_set_bf(fsm_dev, FSM_DSPEN, 1);
	// recover sysctrl
	ret |= fsm_reg_write(fsm_dev, REG(FSM_SYSCTRL), sysctrl.old_val);

	return ret;
}

int fsm_set_cal_scene(fsm_dev_t *fsm_dev)
{
	int ret;

	if (!fsm_dev) {
		return -EINVAL;
	}
	fsm_access_key(fsm_dev, 1);
	ret = fsm_set_tsctrl(fsm_dev, false, false);
	ret |= fsm_switch_cal_scene(fsm_dev);
	ret |= fsm_reg_write(fsm_dev, REG(FSM_ZMCONFIG), 0x0010);
	ret |= fsm_set_tsctrl(fsm_dev, true, false);
	ret |= fsm_reg_write(fsm_dev, REG(FSM_AUDIOCTRL), 0x8F00);
	ret |= fsm_reg_write(fsm_dev, REG(FSM_SPKERR), 0x0000);
	ret |= fsm_reg_write(fsm_dev, REG(FSM_SPKRE), 0x0000);
	if (fsm_dev->is1603s && IS_PRESET_V3(fsm_dev->dev_list->preset_ver)) {
		ret |= fsm_reg_write(fsm_dev, REG(FSM_SPKMT01), 0);
		ret |= fsm_reg_write(fsm_dev, REG(FSM_SPKMT02), 0);
		ret |= fsm_reg_write(fsm_dev, REG(FSM_SPKMT03), 0);
		ret |= fsm_reg_write(fsm_dev, REG(FSM_SPKMT04), 0);
		ret |= fsm_reg_write(fsm_dev, REG(FSM_SPKMT05), 0);
		ret |= fsm_reg_write(fsm_dev, REG(FSM_SPKMT06), 0);
	} else {
		ret |= fsm_reg_write(fsm_dev, REG(FSM_SPKM6), 0x0000);
		ret |= fsm_reg_write(fsm_dev, REG(FSM_SPKM24), 0x0000);
	}
	ret |= fsm_reg_write(fsm_dev, REG(FSM_ADCENV), 0xFFFF);
	ret |= fsm_reg_write(fsm_dev, REG(FSM_ADCTIME), 0x0031);
	fsm_access_key(fsm_dev, 0);

	return ret;
}

int fsm_get_spkr_tempr(fsm_dev_t *fsm_dev)
{
	fsm_config_t *cfg = fsm_get_config();
	uint16_t value;
	int spk_rt;
	int tempr;
	int ret;

	if (!cfg || fsm_dev == NULL) {
		return -EINVAL;
	}
	if (fsm_skip_device(fsm_dev)) {
		return 0;
	}
	if (!fsm_dev->state.calibrated) {
		pr_addr(info, "not calibrated yet");
		return 0;
	}
	ret = fsm_get_bf(fsm_dev, FSM_OFFSTA, &value);
	if (value) {
		pr_addr(info, "ts is auto off");
		return 0;
	}
	if (cfg->nondsp_mode) {
		pr_addr(info, "nondsp mode");
		return ret;
	}
	ret = fsm_reg_read(fsm_dev, REG(FSM_ZMDATA), &value);
	if (value == 0 || value == 0xFFFF) {
		pr_addr(info, "ts maybe auto off");
		return 0;
	}
	spk_rt = fsm_cal_spkr_zmimp(fsm_dev, value);
	tempr = 25 + FSM_MAGNIF_TEMPR_COEF * (spk_rt - fsm_dev->re25)
			/ (fsm_dev->tcoef * fsm_dev->re25);
	pr_addr(info, "ZM:%d, R25:%d, T:%d, R:%d",
			value, fsm_dev->re25, tempr, spk_rt);

	return ret;
}

int fsm_check_status(fsm_dev_t *fsm_dev)
{
	fsm_config_t *cfg = fsm_get_config();
	uint16_t status;
	uint16_t bf_val;
	int ready;
	int ret;

	if (!cfg || fsm_dev == NULL) {
		return -EINVAL;
	}
	if (!cfg->speaker_on) {
		return -EINVAL;
	}
	ret = fsm_reg_read(fsm_dev, REG(FSM_STATUS), &status);
	bf_val = get_bf_val(FSM_BOVDS, status);
	if (!bf_val)
		pr_addr(info, "overvoltage on boost");
	ready = bf_val;
	bf_val = get_bf_val(FSM_PLLS, status);
	if (!bf_val)
		pr_addr(info, "PLL is not in lock");
	ready &= bf_val;
	bf_val = get_bf_val(FSM_OTDS, status);
	if (!bf_val)
		pr_addr(info, "over temperature");
	ready &= bf_val;
	bf_val = get_bf_val(FSM_OVDS, status);
	if (!bf_val)
		pr_addr(info, "overvoltage on VBAT");
	ready &= bf_val;
	bf_val = get_bf_val(FSM_UVDS, status);
	if (!bf_val)
		pr_addr(info, "undervoltage on VBAT");
	ready &= bf_val;
	bf_val = get_bf_val(FSM_OCDS, status);
	if (bf_val)
		pr_addr(info, "overcurrent in amplifier");
	ready &= (!bf_val);
	bf_val = get_bf_val(FSM_CLKS, status);
	if (!bf_val)
		pr_addr(info, "bclk/ws is not stable");
	ready &= bf_val;
	if (!ret && ready) {
		return 0;
	}
	pr_addr(info, "status:%04X", status);

	return -EINVAL;
}

int fsm_check_spkcoef(fsm_dev_t *fsm_dev)
{
	uint8_t reg_tcoef;
	uint16_t value;
	int ret;

	if (fsm_dev == NULL) {
		return -EINVAL;
	}
	reg_tcoef = REG(FSM_TEMPSEL);
	ret = fsm_reg_read(fsm_dev, reg_tcoef, &value);
	if (value == 0x0010) {
		// not inited yet
		return -EINVAL;
	}

	return ret;
}

int fsm_ops_dummy(fsm_dev_t *fsm_dev)
{
	return 0;
}

/*
 * fsm stub api
 */

int fsm_stub_switch_preset(fsm_dev_t *fsm_dev)
{
	int ret;

	if (fsm_dev == NULL) {
		return -EINVAL;
	}
	if (fsm_dev->dev_ops.switch_preset) {
		return fsm_dev->dev_ops.switch_preset(fsm_dev);
	}
	ret = fsm_switch_preset(fsm_dev);

	FSM_FUNC_EXIT(ret);
	return ret;
}

int fsm_stub_dev_init(fsm_dev_t *fsm_dev)
{
	fsm_config_t *cfg = fsm_get_config();
	int ret;

	if (fsm_dev == NULL || cfg == NULL) {
		return -EINVAL;
	}

	if (cfg->force_init) {
		pr_addr(info, "force init");
	}
	ret = fsm_check_spkcoef(fsm_dev);
	if (ret) {
		pr_addr(info, "try init device");
		fsm_dev->state.dev_inited = false;
	}
	if (!cfg->force_init && fsm_dev->state.dev_inited) {
		return 0;
	}
	if (fsm_dev->dev_ops.dev_init) {
		return fsm_dev->dev_ops.dev_init(fsm_dev);
	}
	ret = fsm_init_dev_list(fsm_dev);
	if (ret || !fsm_dev->dev_list) {
		pr_addr(err, "init dev_list fail:%d", ret);
		return ret;
	}
	fsm_dev->state.dev_inited = true;
	ret = fsm_init_info(fsm_dev);

	if (fsm_dev->dev_ops.reg_init) {
		ret |= fsm_dev->dev_ops.reg_init(fsm_dev);
	}
	ret |= fsm_write_preset(fsm_dev);
	if (cfg->nondsp_mode) {
		fsm_dev->state.calibrated = !ret;
	} else {
		ret |= fsm_check_otp(fsm_dev);
	}

	ret |= fsm_reg_write(fsm_dev, REG(FSM_SYSCTRL), 0x0001);
	fsm_wait_stable(fsm_dev, FSM_WAIT_AMP_ADC_OFF);
	ret |= fsm_reg_write(fsm_dev, REG(FSM_PLLCTRL4), 0x0000);
	if (ret) {
		pr_addr(err, "init failed:%d", ret);
		fsm_dev->state.dev_inited = false;
	}
	fsm_dev->errcode = ret;

	return ret;
}

int fsm_stub_start_up(fsm_dev_t *fsm_dev)
{
	uint16_t sysctrl;
	int ret;

	if (fsm_dev == NULL) {
		return -EINVAL;
	}
	if (fsm_dev->dev_ops.start_up) {
		return fsm_dev->dev_ops.start_up(fsm_dev);
	}
	if (fsm_dev->dev_ops.i2s_config) {
		ret = fsm_dev->dev_ops.i2s_config(fsm_dev);
	}
	if (fsm_dev->dev_ops.pll_config) {
		ret = fsm_dev->dev_ops.pll_config(fsm_dev, true);
	}
	// power on device
	ret = fsm_reg_read(fsm_dev, REG(FSM_SYSCTRL), &sysctrl);
	// set_bf_val(&sysctrl, FSM_AMPE, 1);
	set_bf_val(&sysctrl, FSM_PWDN, 0);
	ret |= fsm_reg_write(fsm_dev, REG(FSM_SYSCTRL), sysctrl);
	fsm_dev->errcode = ret;

	FSM_FUNC_EXIT(ret);
	return ret;
}

int fsm_stub_check_stable(fsm_dev_t *fsm_dev, int type)
{
	uint16_t value;
	int ready;
	int ret;

	if (fsm_dev == NULL) {
		return -EINVAL;
	}
	if (fsm_dev->dev_ops.check_stable) {
		return fsm_dev->dev_ops.check_stable(fsm_dev, type);
	}
	switch (type) {
	case FSM_WAIT_STATUS_ON:
		ret = fsm_check_status(fsm_dev);
		ready = !ret;
		break;
	case FSM_WAIT_AMP_ON:
	case FSM_WAIT_AMP_ADC_ON:
		ret = fsm_reg_read(fsm_dev, REG(FSM_STATUS), &value);
		ready = get_bf_val(FSM_CLKS, value);
		ready &= get_bf_val(FSM_PLLS, value);
		ret |= fsm_get_bf(fsm_dev, FSM_SSEND, &value);
		ready &= value;
		break;
	case FSM_WAIT_AMP_OFF:
		ret = fsm_get_bf(fsm_dev, FSM_DACRUN, &value);
		ready = (value == 0 ? 1 : 0);
		break;
	case FSM_WAIT_AMP_ADC_OFF:
	case FSM_WAIT_AMP_ADC_PLL_OFF:
		ret = fsm_reg_read(fsm_dev, REG(FSM_DIGSTAT), &value);
		ready = get_bf_val(FSM_ADCRUN, value);
		ready |= get_bf_val(FSM_DACRUN, value);
		ready = !ready;
		break;
	case FSM_WAIT_TSIGNAL_OFF:
		ret = fsm_get_bf(fsm_dev, FSM_OFFSTA, &value);
		ready = value;
		break;
	case FSM_WAIT_OTP_READY:
		ret = fsm_get_bf(fsm_dev, FSM_OTPBUSY, &value);
		ready = !value;
		break;
	case FSM_WAIT_BOOST_SSEND:
		ret = fsm_get_bf(fsm_dev, FSM_SSEND, &value);
		ready = value;
		break;
	default:
		ret = -EINVAL;
		ready = 0;
		break;
	}
	if (ready && type == FSM_WAIT_AMP_ADC_PLL_OFF) {
		if (fsm_dev->dev_ops.pll_config) {
			fsm_dev->dev_ops.pll_config(fsm_dev, false);
		}
	}
	if (ready && type == FSM_WAIT_AMP_ADC_ON) {
		ret |= fsm_reg_read(fsm_dev, REG(FSM_DIGSTAT), &value);
		ready &= get_bf_val(FSM_DACRUN, value);
		ready &= get_bf_val(FSM_ADCRUN, value);
	}
	if (!ret && !ready) {
		ret = -EINVAL;
	}

	return ret;
}

int fsm_stub_set_tsignal(fsm_dev_t *fsm_dev, bool enable)
{
	int ret = 0;

	if (fsm_dev == NULL) {
		return -EINVAL;
	}
	if (fsm_dev->dev_ops.set_tsignal) {
		return fsm_dev->dev_ops.set_tsignal(fsm_dev, enable);
	}
	if (fsm_dev->is1603s && !enable) {
		ret |= fsm_set_bf(fsm_dev, 0x09AE, 1);  // set bit9=1 of 0xAE
	}
	ret |= fsm_set_tsctrl(fsm_dev, enable, true);
	if (fsm_dev->is1603s && enable) {
		fsm_delay_ms(20);
		ret |= fsm_set_bf(fsm_dev, 0x09AE, 0);  // set bit9=0 of 0xAE
	}

	return ret;
}

int fsm_stub_set_mute(fsm_dev_t *fsm_dev, bool mute)
{
	int ret;

	if (fsm_dev == NULL) {
		return -EINVAL;
	}
	if (fsm_dev->dev_ops.set_mute) {
		return fsm_dev->dev_ops.set_mute(fsm_dev, mute);
	}
	ret = fsm_config_vol(fsm_dev);
	ret |= fsm_set_bf(fsm_dev, FSM_AMPE, !mute);
	fsm_dev->errcode |= ret;

	return ret;
}

int fsm_stub_shut_down(fsm_dev_t *fsm_dev)
{
	uint16_t sysctrl;
	int ret;

	if (fsm_dev == NULL) {
		return -EINVAL;
	}
	if (fsm_dev->dev_ops.shut_down) {
		return fsm_dev->dev_ops.shut_down(fsm_dev);
	}
	// fsm_reg_read(fsm_dev, REG(FSM_DIGSTAT), NULL); // TODO
	// ret = fsm_config_vol(fsm_dev);
	ret = fsm_reg_read(fsm_dev, REG(FSM_SYSCTRL), &sysctrl);
	// amplifier off and power down device
	// set_bf_val(&sysctrl, FSM_AMPE, 0);
	set_bf_val(&sysctrl, FSM_PWDN, 1);
	ret |= fsm_reg_write(fsm_dev, REG(FSM_SYSCTRL), sysctrl);

	FSM_FUNC_EXIT(ret);
	return ret;
}
static int fsm_stub_store_otp(fsm_dev_t *fsm_dev, uint8_t valOTP)
{
	if (fsm_dev && fsm_dev->dev_ops.store_otp) {
		return fsm_dev->dev_ops.store_otp(fsm_dev, valOTP);
	} else {
		pr_err("fsm_dev not support");
		return -EINVAL;
	}
}

int fsm_stub_pre_calib(fsm_dev_t *fsm_dev)
{
	fsm_config_t *cfg = fsm_get_config();
	int force;
	int ret;

	if (fsm_dev == NULL || !cfg) {
		return -EINVAL;
	}
	if (fsm_dev->dev_ops.pre_calib) {
		return fsm_dev->dev_ops.pre_calib(fsm_dev);
	}
	force = cfg->force_calib;
	pr_addr(info, "force:%d, calibrated:%d",
			force, fsm_dev->state.calibrated);
	if (!force && fsm_dev->state.calibrated) {
		pr_addr(info, "already calibrated");
		return 0;
	}
	fsm_dev->state.calibrated = false;
	fsm_dev->re25 = 0;
	fsm_dev->errcode = 0;
	if (fsm_dev->tdata) {
		fsm_dev->tdata->sum_re25   = 0;
		fsm_dev->tdata->count_re25 = 0;
		fsm_dev->tdata->test_re25  = 0;
		memset(&fsm_dev->tdata->re25, 0, sizeof(struct re25_data));
	}
	ret = fsm_set_cal_scene(fsm_dev);
	ret |= fsm_check_status(fsm_dev);
	fsm_dev->errcode = ret;
	if (ret) {
		pr_addr(err, "error:%d, stop test", ret);
		fsm_dev->state.re25_runin = false;
		return ret;
	}
	fsm_dev->state.re25_runin = true;

	return ret;
}

int fsm_stub_post_calib(fsm_dev_t *fsm_dev)
{
	fsm_config_t *cfg = fsm_get_config();
	uint8_t valOTP;
	uint16_t cal_zm;
	int ret = 0;

	if (fsm_dev == NULL || !cfg || fsm_dev->tdata == NULL) {
		return -EINVAL;
	}
	if (!fsm_dev->state.re25_runin) {
		pr_addr(info, "invalid running state");
		return 0;
	}
	if (fsm_dev->dev_ops.post_calib) {
		return fsm_dev->dev_ops.post_calib(fsm_dev);
	}
	fsm_dev->state.re25_runin = false;
	if (cfg->nondsp_mode) {
		fsm_dev->state.calibrated = true;
		ret = fsm_access_key(fsm_dev, 1);
		ret |= fsm_reg_write(fsm_dev, REG(FSM_ZMCONFIG), 0x0000);
		ret |= fsm_access_key(fsm_dev, 0);
	} else {
		cal_zm = fsm_dev->tdata->re25.zmdata;
		// calculate spk-t thrd and re25
		fsm_access_key(fsm_dev, 1);
		ret = fsm_set_threshold(fsm_dev, cal_zm, FSM_DATA_TYPE_ZMDATA);
		fsm_dev->tdata->test_re25 = fsm_dev->re25;
		ret |= fsm_reg_write(fsm_dev, REG(FSM_ZMCONFIG), 0x0000);
		fsm_access_key(fsm_dev, 0);
		if (!ret) {
			valOTP = fsm_re25_to_byte(fsm_dev, fsm_dev->re25);
			if (valOTP != 0xFF && cfg->store_otp) {
				ret |= fsm_stub_store_otp(fsm_dev, valOTP);
			}
		}
	}
	ret |= fsm_set_tsctrl(fsm_dev, false, false);
	fsm_wait_stable(fsm_dev, FSM_WAIT_TSIGNAL_OFF);
	fsm_dev->cur_scene = FSM_SCENE_UNKNOW;
	fsm_switch_preset(fsm_dev);
	// ret |= fsm_config_vol(fsm_dev);
	fsm_dev->errcode |= ret;

	FSM_FUNC_EXIT(ret);
	return ret;
}

int fsm_stub_dev_deinit(fsm_dev_t *fsm_dev)
{
	if (fsm_dev == NULL) {
		return -EINVAL;
	}
	if (fsm_dev->dev_ops.deinit) {
		return fsm_dev->dev_ops.deinit(fsm_dev);
	}
	// deinit here

	return 0;
}

int fsm_dev_recover(fsm_dev_t *fsm_dev)
{
	fsm_config_t *cfg = fsm_get_config();
	int ret;

	if (!cfg || !fsm_dev) {
		return -EINVAL;
	}
	if (fsm_skip_device(fsm_dev)) {
		return 0;
	}
	if (fsm_dev->dev_ops.dev_recover) {
		return fsm_dev->dev_ops.dev_recover(fsm_dev);
	}
	ret = fsm_check_status(fsm_dev);
	if (!ret) {
		fsm_dev->rec_count = 0;
		return ret;
	}
	pr_addr(info, "recover device");
	cfg->force_init = true;
	cfg->force_scene = true;
	fsm_dev->rec_count++;
	ret |= fsm_stub_dev_init(fsm_dev);
	ret |= fsm_stub_start_up(fsm_dev);
	ret |= fsm_wait_stable(fsm_dev, FSM_WAIT_AMP_ON);
	ret |= fsm_stub_set_mute(fsm_dev, false);
	ret |= fsm_wait_stable(fsm_dev, FSM_WAIT_AMP_ADC_ON);
	ret |= fsm_stub_set_tsignal(fsm_dev, true);
	cfg->force_init = false;
	cfg->force_scene = false;

	return ret;
}

int fsm_get_livedata(fsm_msg_t *data)
{
	if (!data) {
		return -EINVAL;
	}
#if defined(FSM_HAL_SUPPORT)
	return fsm_hal_get_livedata(data);
#elif defined(CONFIG_FSM_NONDSP)
	return fsm_algo_get_livedata(data);
#else
	return -EINVAL;
#endif
}

int fsm_dev_count(void)
{
	return g_fsm_config.dev_count;
}

void fsm_set_i2s_clocks(uint32_t rate, uint32_t bclk)
{
	fsm_config_t *cfg = fsm_get_config();

	if (!cfg) {
		return;
	}
	fsm_mutex_lock();
	cfg->i2s_bclk = bclk;
	cfg->i2s_srate = rate;
	if (rate == 32000) {
		cfg->i2s_bclk += 32;
	}
	fsm_mutex_unlock();
}

void fsm_set_tx_format(uint32_t format)
{
	fsm_config_t *cfg = fsm_get_config();

	if (!cfg) {
		return;
	}
	fsm_mutex_lock();
	cfg->tx_fmt = format;
	fsm_mutex_unlock();
}

void fsm_set_scene(int scene)
{
	fsm_config_t *cfg = fsm_get_config();
	fsm_dev_t *fsm_dev = NULL;

	if (!cfg || scene < 0 || scene >= FSM_SCENE_MAX) {
		pr_err("invaild scene:%d", scene);
		return;
	}
	fsm_mutex_lock();
	cfg->next_scene = BIT(scene);
	fsm_list_func(fsm_dev, fsm_stub_switch_preset);
	if (!cfg->stream_muted && cfg->next_scene != FSM_SCENE_RCV) {
		fsm_delay_ms(10);
		fsm_list_func_arg(fsm_dev, fsm_stub_set_tsignal, true);
	}
	fsm_mutex_unlock();
}

void fsm_set_volume(int volume)
{
	fsm_config_t *cfg = fsm_get_config();
	fsm_dev_t *fsm_dev;

	if (!cfg) {
		return;
	}
	fsm_mutex_lock();
	if (volume < 0 || volume > FSM_VOLUME_MAX) {
		pr_warning("invalid volume: %d, default 0dB", volume);
		volume = FSM_VOLUME_MAX;
	}
	cfg->volume = volume;
	if (!cfg->stream_muted) {
		fsm_list_func(fsm_dev, fsm_config_vol);
	}
	fsm_mutex_unlock();
}

void fsm_set_fw_name(char *name)
{
	fsm_config_t *cfg = fsm_get_config();

	if (cfg) {
		cfg->fw_name = name;
	}
}

static int fsm_detect_fs1603tu(fsm_dev_t *fsm_dev)
{
	uint16_t value;
	int ret;

	ret = fsm_access_key(fsm_dev, 1);
	ret |= fsm_reg_read(fsm_dev, 0xD3, &value);
	ret |= fsm_access_key(fsm_dev, 0);
	value = (value >> 5) & 0x0007; // bit[7..5]
	if (!ret && value >= 4) {
		fsm_dev->is1603s = false; // as fs1603u
		fsm_dev->is1603tu = true;
	} else {
		fsm_dev->is1603tu = false;
	}

	return ret;
}

int fsm_detect_device(fsm_dev_t *fsm_dev, uint8_t dev_id)
{
	if (fsm_dev == NULL) {
		return -EINVAL;
	}
	fsm_dev->use_irq = false;
	switch (dev_id) {
	case FS1601S_DEV_ID:
		fs1601s_ops(fsm_dev);
		break;
	case FS1603_DEV_ID:
		fsm_detect_fs1603tu(fsm_dev);
		fs1603_ops(fsm_dev);
		break;
	case FS1818_DEV_ID:
	case FS1896_DEV_ID:
		fs1801_ops(fsm_dev);
		break;
	case FS1860_DEV_ID:
		fs1860_ops(fsm_dev);
		break;
	default:
		pr_addr(err, "invalid dev_id:%02X", dev_id);
		memset(&fsm_dev->dev_ops, 0, sizeof(fsm_dev->dev_ops));
		return -EINVAL;
	}

	return 0;
}

int fsm_probe(fsm_dev_t *fsm_dev, int addr)
{
	uint16_t id;
	int ret;

	if (fsm_dev == NULL) {
		return -EINVAL;
	}

	fsm_mutex_lock();
	fsm_dev->addr = addr;
	ret = fsm_reg_read(fsm_dev, REG(FSM_ID), &id);
	if (ret) {
		fsm_dev->addr = 0;
		fsm_mutex_unlock();
		return ret;
	}
	fsm_dev->is1603s = IS_FS1603S(id);
	ret = fsm_detect_device(fsm_dev, HIGH8(id));
	if (ret) {
		pr_addr(err, "Unknown DEVID: %04X", id);
		fsm_dev->addr = 0;
		fsm_mutex_unlock();
		return ret;
	}
#ifndef BUILD_FSTOOL
	pr_addr(info, "Found DEVID: %04X", id);
#endif
	fsm_dev->ram_scene[FSM_EQ_RAM0] = FSM_SCENE_UNKNOW;
	fsm_dev->ram_scene[FSM_EQ_RAM1] = FSM_SCENE_UNKNOW;
	fsm_dev->own_scene = FSM_SCENE_UNKNOW;
	fsm_dev->cur_scene = FSM_SCENE_UNKNOW;
	fsm_dev->version = id;
	fsm_dev->acc_count = 0;

	fsm_list_init(fsm_dev);
	g_fsm_config.dev_count++;
	fsm_mutex_unlock();

	return 0;
}

void fsm_remove(fsm_dev_t *fsm_dev)
{
	if (!fsm_dev) {
		return;
	}
	fsm_mutex_lock();
	list_del(&fsm_dev->list);
	if (fsm_dev->tdata) {
		fsm_free_mem(fsm_dev->tdata);
	}
	g_fsm_config.dev_count--;
	fsm_mutex_unlock();
}

/**
 * try init deivce without lock
 */
static int fsm_try_init(void)
{
	fsm_config_t *cfg = fsm_get_config();
	fsm_dev_t *fsm_dev = NULL;
	uint16_t next_scene;
	int ret;

	if (fsm_dev_count() <= 0) {
		pr_err("no found device");
		return -EINVAL;
	}
	cfg->force_scene = cfg->force_init;
	ret = fsm_firmware_init_sync(cfg->fw_name);
	if (!fsm_get_presets()) {
		pr_err("invalid firmware, ret:%d", ret);
		return -EINVAL;
	}
	next_scene = cfg->next_scene;
	cfg->next_scene = FSM_SCENE_MUSIC; // init all devices
	list_for_each_entry(fsm_dev, &fsm_dev_list, list) {
		fsm_stub_dev_init(fsm_dev);
	}
	cfg->next_scene = next_scene; // recovery scene
	cfg->force_scene = cfg->force_init = false;

	return 0;
}

void fsm_init(void)
{
	int ret;
	/*
	ret = fsm_hal_open();
	if (ret) {
	    pr_err("hal open fail:%d", ret);
	    return;
	}
	*/
	fsm_mutex_lock();
	pr_info("version: %s", FSM_CODE_VERSION);
	pr_info("branch : %s", FSM_GIT_BRANCH);
	pr_info("date   : %s", FSM_CODE_DATE);
	ret = fsm_try_init();
	if (ret) { // no device or firmware
		pr_err("init failed: %d", ret);
	}
	pr_debug("done");
	fsm_mutex_unlock();
}

void fsm_speaker_onn(void)
{
	fsm_config_t *cfg = fsm_get_config();
	struct fsm_vbat_state *vbat_state;
	fsm_dev_t *fsm_dev = NULL;
	int ret;

	pr_info("scene: %04X", cfg->next_scene);
	fsm_mutex_lock();
	cfg->stream_muted = false;
	ret = fsm_try_init();
	if (ret) { // no device or firmware
		pr_err("init failed: %d", ret);
		fsm_mutex_unlock();
		return;
	}
	// fsm_list_func(fsm_dev, fsm_stub_switch_preset);
	fsm_list_func(fsm_dev, fsm_stub_start_up);
	fsm_list_wait(FSM_WAIT_AMP_ON);
	vbat_state = &g_vbat_state;
	vbat_state->cur_batv = 0;
	if (cfg->use_monitor) {
		fsm_list_func(fsm_dev, fsm_get_vbat);
		pr_info("vbat:%d", vbat_state->cur_batv);
	}
	fsm_list_func_arg(fsm_dev, fsm_stub_set_mute, false);
	vbat_state->last_batv = vbat_state->cur_batv;
	fsm_list_wait(FSM_WAIT_AMP_ADC_ON);
	fsm_delay_ms(10);
	fsm_list_func_arg(fsm_dev, fsm_stub_set_tsignal, true);
	// cfg->skip_monitor = false;
	// fsm_list_func_arg(fsm_dev, fsm_set_monitor, true);
	cfg->cur_angle = cfg->next_angle;
	cfg->speaker_on = true;
	pr_debug("done");
	fsm_mutex_unlock();
}

void fsm_monitor(void)
{
	fsm_dev_t *fsm_dev = NULL;

	fsm_mutex_lock();
	fsm_list_func(fsm_dev, fsm_dev_recover);
	fsm_list_func(fsm_dev, fsm_get_spkr_tempr);
	fsm_mutex_unlock();
}

void fsm_speaker_off(void)
{
	fsm_config_t *cfg = fsm_get_config();
	fsm_dev_t *fsm_dev = NULL;
	int ret;

	pr_info("scene: %04X", cfg->next_scene);
	fsm_mutex_lock();
	cfg->stream_muted = true;
	// cfg->skip_monitor = true;
	// fsm_list_func_arg(fsm_dev, fsm_set_monitor, false);
	ret = fsm_try_init();
	if (ret) {
		pr_err("try init failed: %d", ret);
		fsm_mutex_unlock();
		return;
	}
	fsm_list_func_arg(fsm_dev, fsm_stub_set_tsignal, false);
	fsm_list_wait(FSM_WAIT_TSIGNAL_OFF);
	fsm_list_func_arg(fsm_dev, fsm_stub_set_mute, true);
	fsm_list_func(fsm_dev, fsm_stub_shut_down);
	fsm_list_wait(FSM_WAIT_AMP_ADC_PLL_OFF);
	cfg->speaker_on = false;
	fsm_mutex_unlock();
	fsm_set_scene(0);
	pr_debug("done");
}

void fsm_stereo_rotation(int next_angle)
{
	fsm_config_t *cfg = fsm_get_config();
	fsm_dev_t *fsm_dev = NULL;

	if (next_angle >= 360) { // angle range: [0 ~ 359]
		next_angle %= 360;
	}
	if (cfg->cur_angle == next_angle) {
		return;
	}
	fsm_mutex_lock();
	pr_info("scene:%04X, angle:%d", cfg->next_scene, next_angle);
	cfg->next_angle = next_angle;
	if (cfg->stream_muted || !(cfg->next_scene & FSM_SCENE_MUSIC)) {
		fsm_mutex_unlock();
		return;
	}
	fsm_list_func_arg(fsm_dev, fsm_swap_channel, cfg->next_angle);
	cfg->cur_angle = cfg->next_angle;
	fsm_mutex_unlock();
}

void fsm_re25_test(bool force)
{
	fsm_config_t *cfg = fsm_get_config();
	fsm_dev_t *fsm_dev = NULL;
	int retry;
	int ret;

	// check speaker on or not
	for (retry = 0; retry < FSM_WAIT_STABLE_RETRY; retry++) {
		if (cfg->speaker_on)
			break;
		fsm_delay_ms(50);
	}
	fsm_mutex_lock();
	pr_info("start testing");
	cfg->skip_monitor = true;
	cfg->test_type = TEST_RE25;
	cfg->force_calib = force;
	cfg->stop_test = false;
	fsm_mutex_unlock();
	// check play audio or not
	ret = fsm_list_wait(FSM_WAIT_STATUS_ON);
	if (ret) {
		pr_err("wait status timeout!");
		return;
	}
	fsm_mutex_lock();
	fsm_list_func(fsm_dev, fsm_stub_pre_calib);
	fsm_delay_ms(RE25_1ST_WAIT_MS);
	cfg->amb_tempr = fsm_get_amb_tempr();
	for (retry = 0; retry < FSM_CALIB_RE25_RETRY; retry++) {
		if (cfg->stop_test) {
			break;
		}
		fsm_delay_ms(RE25_DELAY_MS);
		fsm_list_return(fsm_dev, fsm_cal_zmdata, ret);
		if (!ret) {
			break;
		}
	}
	pr_info("retry %d times, max:%d", retry, FSM_CALIB_RE25_RETRY);
	fsm_list_func(fsm_dev, fsm_stub_post_calib);
	cfg->test_type = TEST_NONE;
	fsm_list_func_arg(fsm_dev, fsm_stub_set_mute, false);
	fsm_list_wait(FSM_WAIT_AMP_ADC_ON);
	fsm_delay_ms(10);  // ms
	fsm_list_func_arg(fsm_dev, fsm_stub_set_tsignal, true);
	cfg->force_calib = false;
	cfg->skip_monitor = false;
	cfg->stop_test = true;
	pr_debug("done");
	fsm_mutex_unlock();
}

void fsm_f0_test(void)
{
	fsm_config_t *cfg = fsm_get_config();
	fsm_dev_t *fsm_dev = NULL;
	int freq_1st;
	int freq;
	int ret;

	fsm_mutex_lock();
	pr_info("start testing");
	cfg->skip_monitor = true;
	cfg->test_type = TEST_F0;
	cfg->stop_test = false;
	fsm_mutex_unlock();
	ret = fsm_list_wait(FSM_WAIT_STATUS_ON);
	if (ret) {
		pr_err("wait status timeout!");
		return;
	}
	fsm_mutex_lock();
	fsm_list_entry(fsm_dev, fsm_dev->dev_ops.pre_f0_test);
	freq_1st = cfg->freq_start;
	fsm_delay_ms(50);  // 50ms
	for (freq = freq_1st; freq <= cfg->freq_end; freq += cfg->freq_step) {
		if (cfg->stop_test) {
			break;
		}
		cfg->test_freq = freq;
		fsm_list_entry(fsm_dev, fsm_dev->dev_ops.f0_test);
		fsm_delay_ms((freq == freq_1st ? F0_1ST_DELAY_MS : F0_TEST_DELAY_MS));
		fsm_list_return(fsm_dev, fsm_cal_zmdata, ret);
	}
	fsm_delay_ms(1400);
	fsm_list_entry(fsm_dev, fsm_dev->dev_ops.post_f0_test);
	fsm_delay_ms(35);  // 35ms
	cfg->test_type = TEST_NONE;
	// force init device
	pr_debug("done");
	if (cfg->nondsp_mode) {
		fsm_mutex_unlock();
		return;
	}
	cfg->force_init = cfg->force_scene = true;
	fsm_mutex_unlock();
	fsm_speaker_onn();
	cfg->force_init = cfg->force_scene = false;
}

int fsm_test_result(struct fsm_cal_result *result, int size)
{
	struct preset_file *pfile = fsm_get_presets();
	fsm_config_t *cfg = fsm_get_config();
	struct dev_list *dev_list;
	fsm_dev_t *fsm_dev;
	uint8_t *preset;
	int freq_count;
	int dev;

	if (!cfg || !result || !pfile) {
		pr_err("invalid parameters");
		return -EINVAL;
	}
	if (pfile->hdr.ndev > FSM_DEV_MAX) {
		pr_err("invalid ndev:%d", pfile->hdr.ndev);
		return -EINVAL;
	}
	pr_info("ndev:%d", pfile->hdr.ndev);
	fsm_mutex_lock();
	memset(result, 0, size);
	preset = (uint8_t *) pfile;
	result->ndev = pfile->hdr.ndev;
	result->freq_start = cfg->freq_start;
	result->freq_end = cfg->freq_end;
	result->freq_step = cfg->freq_step;
	result->freq_count = cfg->freq_count;
	for (dev = 0; dev < result->ndev; dev++) {
		if (FSM_DSC_DEV_INFO != pfile->index[dev].type) {
			continue;
		}
		dev_list = (struct dev_list *) &preset[pfile->index[dev].offset];
		if (!dev_list) {
			pr_err("invalid dev_list[%d]", dev);
			continue;
		}
		result->info[dev].addr = dev_list->addr;
		fsm_dev = fsm_get_fsm_dev(dev_list->addr);
		if (fsm_dev == NULL) {
			pr_info("invalid fsm_dev[%02X]", dev_list->addr);
			result->info[dev].re25 = 0;
			result->info[dev].errcode = 1000;
			continue;
		}
		result->info[dev].pos = fsm_dev->pos_mask;
		result->info[dev].count = fsm_dev->cal_count;
		result->info[dev].re25 = fsm_dev->re25;
		result->info[dev].f0 = fsm_dev->f0;
		result->info[dev].errcode = fsm_dev->errcode;
		if (fsm_dev->tdata == NULL || cfg->nondsp_mode) {
			pr_addr(info, "copy info only, nondsp:%d", cfg->nondsp_mode);
			continue;
		}
		result->info[dev].re25 = fsm_dev->tdata->test_re25;
		result->info[dev].f0 = fsm_dev->tdata->test_f0;
		freq_count = fsm_dev->tdata->f0.count;
		if (!cfg->stop_test) {
			if (cfg->test_type == TEST_F0)
				result->info[dev].f0 = 0;
			else if (cfg->test_type == TEST_RE25)
				result->info[dev].re25 = 0;
			else
				pr_addr(err, "invalid test type");
		}
		if (freq_count <= 0 || size == sizeof(struct fsm_cal_result)) {
			pr_addr(info, "copy info data only");
			continue;
		}
		if (result->freq_count != freq_count) {
			pr_addr(err, "invalid freq count:%d", freq_count);
			continue;
		}
		memcpy(&result->f0_zmdata[dev * freq_count],
		       fsm_dev->tdata->f0.zmdata, freq_count * sizeof(uint16_t));
	}
	fsm_mutex_unlock();
	pr_info("result->ndev:%d", result->ndev);

	return 0;
}

void fsm_dump(void)
{
	fsm_dev_t *fsm_dev = NULL;
	fsm_mutex_lock();
	fsm_list_func(fsm_dev, fsm_reg_dump);
	fsm_mutex_unlock();
}

void fsm_deinit(void)
{
	fsm_dev_t *fsm_dev = NULL;

	fsm_mutex_lock();
	if (fsm_dev_count() <= 0) {
		fsm_mutex_unlock();
		return;
	}
	fsm_list_func(fsm_dev, fsm_stub_dev_deinit);
	fsm_firmware_deinit();
	fsm_mutex_unlock();
	fsm_hal_close();
}
