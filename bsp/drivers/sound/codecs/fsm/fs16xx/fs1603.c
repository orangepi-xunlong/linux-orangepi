/* SPDX-License-Identifier: GPL-2.0-or-later */
/**
 * Copyright (C) Fourier Semiconductor Inc. 2016-2020. All rights reserved.
 * 2019-01-29 File created.
 */

#include "../fsm_public.h"
#if defined(CONFIG_FSM_FS1603)
#include "fs1603_reg_bf.h"

#define FS1603_FW_NAME        "fs1603.fsm"
#define FS1603S_FW_NAME       "fs1603s.fsm"
#define FS1603_PRESET_EQ_LEN  (0x0090)
#define FS1603_EXCER_RAM_LEN  (0x0070)
#define FS1603_RS2RL_RATIO    (2700)
#define FS1603_OTP_COUNT_MAX  (5)
#define FS1603_EXCER_RAM_ADDR (0x0)
#define FS1603_OTP_BST_CFG    (0xB9DE)

const static fsm_pll_config_t g_fs1603_pll_tbl[] = {
	/* bclk,    0xC1,   0xC2,   0xC3 */
	{  256000, 0x0260, 0x0540, 0x0001 }, //  8000*16*2
	{  512000, 0x0260, 0x0540, 0x0002 }, // 16000*16*2 &  8000*32*2
	{ 1024000, 0x0260, 0x0540, 0x0004 }, //            & 16000*32*2
	{ 1024032, 0x0260, 0x0540, 0x0006 }, // 32000*16*2+32
	{ 1411200, 0x0260, 0x0460, 0x0005 }, // 44100*16*2
	{ 1536000, 0x0260, 0x0540, 0x0006 }, // 48000*16*2
	{ 2048032, 0x0260, 0x0540, 0x000C }, //            & 32000*32*2+32
	{ 2822400, 0x0260, 0x0460, 0x000A }, //            & 44100*32*2
	{ 3072000, 0x0260, 0x0540, 0x000C }, //            & 48000*32*2
};

const static struct fsm_bpcoef g_fs1603_bpcoef_table[] = {
	{  200, { 0x00160EF6, 0x001603AD, 0xFFE9F108, 0x0009E1D6, 0xFFE61915 } },
	{  250, { 0x00160EF6, 0x001603AD, 0xFFE9F108, 0x0009E344, 0xFFE61915 } },
	{  300, { 0x00160EF6, 0x001603AD, 0xFFE9F108, 0x0009DD50, 0xFFE6191A } },
	{  350, { 0x00160EF6, 0x001603AD, 0xFFE9F108, 0x0009DF15, 0xFFE6191A } },
	{  400, { 0x00160EF7, 0x001603AD, 0xFFE9F10B, 0x0009D9B4, 0xFFE6191B } },
	{  450, { 0x00160EF7, 0x001603AD, 0xFFE9F10B, 0x0009D48C, 0xFFE61918 } },
	{  500, { 0x00160EF4, 0x001603AD, 0xFFE9F10A, 0x0009D07D, 0xFFE61919 } },
	{  550, { 0x00160EF4, 0x001603AD, 0xFFE9F10A, 0x0009D38B, 0xFFE6191E } },
	{  600, { 0x00160EF5, 0x001603AD, 0xFFE9F105, 0x0009CF8F, 0xFFE6191F } },
	{  650, { 0x00160EF5, 0x001603AD, 0xFFE9F105, 0x0009C468, 0xFFE6191C } },
	{  700, { 0x00160EFA, 0x001603AD, 0xFFE9F104, 0x0009C0A2, 0xFFE61902 } },
	{  750, { 0x00160EFB, 0x001603AD, 0xFFE9F107, 0x0009BDAD, 0xFFE61903 } },
	{  800, { 0x00160EFB, 0x001603AD, 0xFFE9F107, 0x0009BB35, 0xFFE61900 } },
	{  850, { 0x00160EF8, 0x001603AD, 0xFFE9F106, 0x0009B17A, 0xFFE61906 } },
	{  900, { 0x00160EF9, 0x001603AD, 0xFFE9F101, 0x0009AF13, 0xFFE61904 } },
	{  950, { 0x00160EFE, 0x001603AD, 0xFFE9F100, 0x0009A5E6, 0xFFE6190A } },
	{ 1000, { 0x00160EFF, 0x001603AD, 0xFFE9F103, 0x00099CD2, 0xFFE6190B } },
	{ 1050, { 0x00160EFC, 0x001603AD, 0xFFE9F102, 0x00099BF6, 0xFFE61909 } },
	{ 1100, { 0x00160EFD, 0x001603AD, 0xFFE9F11D, 0x00099373, 0xFFE6190C } },
	{ 1150, { 0x00160EE2, 0x001603AD, 0xFFE9F11C, 0x00098AA4, 0xFFE61932 } },
	{ 1200, { 0x00160EE3, 0x001603AD, 0xFFE9F11F, 0x00098376, 0xFFE61930 } },
	{ 1250, { 0x00160EE0, 0x001603AD, 0xFFE9F11E, 0x00097BF8, 0xFFE61936 } },
	{ 1300, { 0x00160EE6, 0x001603AD, 0xFFE9F118, 0x00096CDA, 0xFFE61935 } },
	{ 1350, { 0x00160EE7, 0x001603AD, 0xFFE9F11B, 0x000965EC, 0xFFE6193B } },
	{ 1400, { 0x00160EE4, 0x001603AD, 0xFFE9F11A, 0x00095F1F, 0xFFE6193E } },
	{ 1450, { 0x00160EEA, 0x001603AD, 0xFFE9F114, 0x00095166, 0xFFE6193D } },
	{ 1500, { 0x00160EEB, 0x001603AD, 0xFFE9F117, 0x00094B20, 0xFFE61920 } },
	{ 1550, { 0x00160EE9, 0x001603AD, 0xFFE9F111, 0x00093E5A, 0xFFE61927 } },
	{ 1600, { 0x00160EEE, 0x001603AD, 0xFFE9F110, 0x000930A4, 0xFFE6192A } },
	{ 1650, { 0x00160EEC, 0x001603AD, 0xFFE9F112, 0x00092469, 0xFFE61929 } },
	{ 1700, { 0x00160EED, 0x001603AD, 0xFFE9F16D, 0x00091F8A, 0xFFE6192C } },
	{ 1750, { 0x00160E93, 0x001603AD, 0xFFE9F16F, 0x00091399, 0xFFE619D3 } },
	{ 1800, { 0x00160E91, 0x001603AD, 0xFFE9F169, 0x00090044, 0xFFE619D7 } },
	{ 1850, { 0x00160E96, 0x001603AD, 0xFFE9F168, 0x0008F4EA, 0xFFE619DA } },
	{ 1900, { 0x00160E94, 0x001603AD, 0xFFE9F16A, 0x0008E9E3, 0xFFE619DE } },
	{ 1950, { 0x00160E9A, 0x001603AD, 0xFFE9F164, 0x0008DF53, 0xFFE619C2 } },
	{ 2000, { 0x00160E98, 0x001603AD, 0xFFE9F166, 0x0008CCF5, 0xFFE619C6 } },
};

static int g_f0_test_done;

static int fs1603_i2c_reset(fsm_dev_t *fsm_dev)
{
	u16 val;
	int ret;
	int i;

	if (!fsm_dev)
		return -EINVAL;

	fsm_dev->acc_count = 0;
	for (i = 0; i < FSM_I2C_RETRY; i++) {
		fsm_reg_write(fsm_dev, REG(FSM_SYSCTRL), 0x0002);   // reset nack
		fsm_reg_read(fsm_dev, REG(FSM_SYSCTRL), NULL);   // dummy read
		fsm_delay_ms(15);  // 15ms
		ret = fsm_reg_write(fsm_dev, REG(FSM_SYSCTRL), 0x0001);
		ret |= fsm_reg_read(fsm_dev, REG(FSM_CHIPINI), &val);
		if ((val == 0x0003) || (val == 0x0300)) {   // init finished
			break;
		}
	}
	if (i == FSM_I2C_RETRY) {
		pr_addr(err, "retry timeout");
		ret = -ETIMEDOUT;
	}

	return ret;
}

static int fs1603_config_pll(fsm_dev_t *fsm_dev, bool on)
{
	fsm_config_t *cfg = fsm_get_config();
	int idx;
	int ret;

	if (!fsm_dev || !cfg) {
		return -EINVAL;
	}
	// config pll need disable pll firstly
	ret = fsm_reg_write(fsm_dev, REG(FSM_PLLCTRL4), 0);
	fsm_delay_ms(1);
	if (!on) {
		// disable pll
		return ret;
	}

	for (idx = 0; idx < ARRAY_SIZE(g_fs1603_pll_tbl); idx++) {
		if (g_fs1603_pll_tbl[idx].bclk == cfg->i2s_bclk) {
			break;
		}
	}
	pr_addr(debug, "bclk[%d]: %d", idx, cfg->i2s_bclk);
	if (idx >= ARRAY_SIZE(g_fs1603_pll_tbl)) {
		pr_addr(err, "Not found bclk: %d, rate: %d",
				cfg->i2s_bclk, cfg->i2s_srate);
		return -EINVAL;
	}

	ret |= fsm_access_key(fsm_dev, 1);
	if (cfg->i2s_srate == 32000) {
		ret |= fsm_reg_write(fsm_dev, REG(FSM_ANACTRL), 0x0101);
	} else {
		ret |= fsm_reg_write(fsm_dev, REG(FSM_ANACTRL), 0x0100);
	}
	ret |= fsm_reg_write(fsm_dev, REG(FSM_PLLCTRL1), g_fs1603_pll_tbl[idx].c1);
	ret |= fsm_reg_write(fsm_dev, REG(FSM_PLLCTRL2), g_fs1603_pll_tbl[idx].c2);
	ret |= fsm_reg_write(fsm_dev, REG(FSM_PLLCTRL3), g_fs1603_pll_tbl[idx].c3);
	ret |= fsm_reg_write(fsm_dev, REG(FSM_PLLCTRL4), 0x000F);
	ret |= fsm_access_key(fsm_dev, 0);

	FSM_FUNC_EXIT(ret);
	return ret;
}

static int fs1603_config_i2s(fsm_dev_t *fsm_dev)
{
	fsm_config_t *cfg = fsm_get_config();
	uint16_t i2sctrl;
	int i2ssr;
	int ret;

	if (!fsm_dev || !cfg) {
		return -EINVAL;
	}
	i2ssr = fsm_get_srate_bits(fsm_dev, cfg->i2s_srate);
	if (i2ssr < 0) {
		pr_addr(err, "unsupport srate:%d", cfg->i2s_srate);
		return -EINVAL;
	}
	ret = fsm_reg_read(fsm_dev, REG(FSM_I2SCTRL), &i2sctrl);
	set_bf_val(&i2sctrl, FSM_I2SSR, i2ssr);
	set_bf_val(&i2sctrl, FSM_I2SF, FSM_FMT_I2S);
	pr_addr(debug, "srate:%d, val:%04X", cfg->i2s_srate, i2sctrl);
	ret |= fsm_reg_write(fsm_dev, REG(FSM_I2SCTRL), i2sctrl);
	ret |= fsm_swap_channel(fsm_dev, cfg->next_angle);

	FSM_FUNC_EXIT(ret);
	return ret;
}

static int fs1603_store_otp(fsm_dev_t *fsm_dev, uint8_t valOTP)
{
	uint16_t pllctrl4;
	uint16_t bstctrl;
	uint16_t otprdata;
	int count;
	int delta;
	int re25_otp;
	int ret;

	if (!fsm_dev) {
		return -EINVAL;
	}
	ret = fsm_access_key(fsm_dev, 1);
	ret |= fsm_reg_multiread(fsm_dev, REG(FSM_BSTCTRL), &bstctrl);
	ret |= fsm_reg_write(fsm_dev, REG(FSM_BSTCTRL), FS1603_OTP_BST_CFG);
	ret |= fsm_reg_read(fsm_dev, REG(FSM_PLLCTRL4), &pllctrl4);
	ret |= fsm_reg_write(fsm_dev, REG(FSM_PLLCTRL4), 0x000F);
	fsm_delay_ms(20);
	ret |= fsm_reg_multiread(fsm_dev, REG(FSM_BSTCTRL), &bstctrl);
	pr_info("boost config readback: %d\n", bstctrl);

	ret |= fsm_reg_write(fsm_dev, REG(FSM_OTPADDR), 0x0010);
	ret |= fsm_reg_write(fsm_dev, REG(FSM_OTPCMD), 0x0000);
	ret |= fsm_reg_write(fsm_dev, REG(FSM_OTPCMD), 0x0001);

	do {
		ret |= fsm_wait_stable(fsm_dev, FSM_WAIT_OTP_READY);
		if (ret) {
			pr_addr(err, "wait OTP ready fail:%d", ret);
			break;
		}
		ret |= fsm_reg_write(fsm_dev, REG(FSM_OTPCMD), 0x0000);
		ret = fsm_reg_multiread(fsm_dev, REG(FSM_OTPRDATA), &otprdata);
		fsm_parse_otp(fsm_dev, otprdata, &re25_otp, &count);
		pr_addr(info, "re25 old:%d, new:%d", re25_otp, fsm_dev->re25);
		delta = abs(re25_otp - fsm_dev->re25);
		if (delta < (FSM_MAGNIF(fsm_dev->spkr) / 20)) {
			pr_addr(info, "not need to update otp, delta: %d", delta);
			break;
		}
		if (count >= FS1603_OTP_COUNT_MAX) {
			pr_addr(err, "count exceeds max: %d", count);
			ret = -EINVAL;
			break;
		}
		ret |= fsm_reg_write(fsm_dev, REG(FSM_OTPADDR), 0x0010);
		ret |= fsm_reg_write(fsm_dev, REG(FSM_OTPWDATA), (uint16_t) valOTP);
		ret |= fsm_reg_write(fsm_dev, REG(FSM_OTPCMD), 0x0000);
		ret |= fsm_reg_write(fsm_dev, REG(FSM_OTPCMD), 0x0002);
		ret |= fsm_wait_stable(fsm_dev, FSM_WAIT_OTP_READY);
		if (ret) {
			pr_addr(err, "wait OTP ready failed!");
			break;
		}
		ret |= fsm_reg_write(fsm_dev, REG(FSM_OTPCMD), 0x0000);   // clear cmd
		ret |= fsm_reg_write(fsm_dev, REG(FSM_BSTCTRL), 0x0001);   // discharge
		fsm_delay_ms(5);  // wait
		ret |= fsm_reg_write(fsm_dev, REG(FSM_BSTCTRL), 0x0000);   // clear
		fsm_delay_ms(5);  // write otp done
		// read back otp and check
		ret |= fsm_reg_write(fsm_dev, REG(FSM_BSTCTRL), 0x0008);   // enable boost
		fsm_delay_ms(20);  // wait boost

		ret = fsm_reg_write(fsm_dev, REG(FSM_OTPADDR), 0x0010);
		ret |= fsm_reg_write(fsm_dev, REG(FSM_OTPCMD), 0x0000);
		ret |= fsm_reg_write(fsm_dev, REG(FSM_OTPCMD), 0x0001);
		ret |= fsm_wait_stable(fsm_dev, FSM_WAIT_OTP_READY);
		if (ret) {
			pr_addr(err, "wait OTP ready failed!");
			break;
		}
		ret |= fsm_reg_write(fsm_dev, REG(FSM_OTPCMD), 0x0000);   // clear cmd
		ret |= fsm_reg_write(fsm_dev, REG(FSM_BSTCTRL), 0x0000);   // disable boost
		fsm_delay_ms(5);  // wait

		ret = fsm_reg_multiread(fsm_dev, REG(FSM_OTPRDATA), &otprdata);
		if (HIGH8(otprdata) != valOTP) {
			pr_addr(err, "read back failed: %04X(expect: %04X)",
					otprdata, valOTP);
			ret = -EINVAL;
			break;
		}
		fsm_parse_otp(fsm_dev, otprdata, &re25_otp, &count);
		fsm_dev->cal_count = count;
		pr_addr(info, "read back count:%d", count);
	} while (0);

	ret |= fsm_reg_write(fsm_dev, REG(FSM_PLLCTRL4), pllctrl4);
	ret |= fsm_reg_write(fsm_dev, REG(FSM_BSTCTRL), bstctrl);
	ret |= fsm_access_key(fsm_dev, 0);

	FSM_FUNC_EXIT(ret);
	return ret;
}

int fs1603s_reg_init(fsm_dev_t *fsm_dev)
{
	uint16_t val;
	int ret;

	if (!fsm_dev || !fsm_dev->dev_list) {
		return -EINVAL;
	}
	val = ((fsm_dev->version == FS1603S_V1) ? 0x5350 : 0x504B);
	ret = fsm_reg_write(fsm_dev, 0xC7, val);
	ret |= fsm_access_key(fsm_dev, 1);
	ret |= fsm_set_bf(fsm_dev, 0x32E1, 0xA);  // bit[5:2]=1010
	ret |= fsm_access_key(fsm_dev, 0);
	if (!IS_PRESET_V3(fsm_dev->dev_list->preset_ver)) {
		ret |= fsm_reg_write(fsm_dev, 0x46, 0xca8b);
		ret |= fsm_reg_write(fsm_dev, 0x47, 0xb4be);
		ret |= fsm_reg_write(fsm_dev, 0x48, 0x1008);
		ret |= fsm_reg_write(fsm_dev, 0x49, 0x1311);
		ret |= fsm_reg_write(fsm_dev, 0xCE, 0x8000);
		ret |= fsm_reg_write(fsm_dev, 0xCF, 0x0010);
	}

	return ret;
}

int fs1603_reg_init(fsm_dev_t *fsm_dev)
{
	int ret;

	if (!fsm_dev || !fsm_dev->dev_list) {
		return -EINVAL;
	}
	ret = fs1603_i2c_reset(fsm_dev);
	ret |= fsm_reg_write(fsm_dev, 0xC4, 0x000A);
	fsm_delay_ms(5);  // 5ms
	ret |= fsm_reg_write(fsm_dev, 0x06, 0x0000);
	ret |= fsm_access_key(fsm_dev, 1);
	ret |= fsm_reg_write(fsm_dev, 0xC0, 0x29C0);
	ret |= fsm_reg_write(fsm_dev, 0xC4, 0x000F);
	ret |= fsm_reg_write(fsm_dev, 0xAE, 0x0210);
	ret |= fsm_reg_write(fsm_dev, 0xB9, 0xFFFF);
	ret |= fsm_reg_write(fsm_dev, 0x09, 0x0009);
	ret |= fsm_reg_write(fsm_dev, 0xCD, 0x2004);
	ret |= fsm_reg_write(fsm_dev, 0xA1, 0x1C92);
	if (fsm_dev->is1603s) {
		ret |= fs1603s_reg_init(fsm_dev);
	}
	ret |= fsm_access_key(fsm_dev, 0);
	fsm_dev->errcode = ret;

	return ret;
}

static int fs1603_f0_reg_init(fsm_dev_t *fsm_dev)
{
	int ret;

	if (!fsm_dev) {
		return -EINVAL;
	}
	ret = fs1603_i2c_reset(fsm_dev);
	ret |= fsm_access_key(fsm_dev, 1);
	ret |= fsm_reg_write(fsm_dev, REG(FSM_AUDIOCTRL), 0xFF00);
	ret |= fsm_reg_write(fsm_dev, REG(FSM_DACCTRL), 0x0210);
	fs1603_config_i2s(fsm_dev);
	fs1603_config_pll(fsm_dev, true);
	// Make sure 0xC4 is 0x000F
	ret |= fsm_set_bf(fsm_dev, FSM_CHS12, 3);
	// ValD0 Bypass OT
	ret |= fsm_reg_write(fsm_dev, REG(FSM_ANACTRL), 0x0120);
	//DCR Bypass
	ret |= fsm_reg_write(fsm_dev, REG(FSM_AUXCFG), 0x1020);
	// Power up
	ret |= fsm_reg_write(fsm_dev, REG(FSM_SYSCTRL), 0x0000);
	// Boost control
	ret |= fsm_reg_write(fsm_dev, REG(FSM_BSTCTRL), 0xB70E);
	// ret |= fsm_reg_read(fsm_dev, REG(FSM_STATUS), NULL);
	// DSP control
	ret |= fsm_reg_write(fsm_dev, REG(FSM_DSPCTRL), 0x1012);
	// ret |= fsm_reg_write(fsm_dev, REG(FSM_ACSCTRL), 0x9880);
	// ADC control, adc env, adc time
	ret |= fsm_reg_write(fsm_dev, REG(FSM_ADCCTRL), 0x0300);
	ret |= fsm_reg_write(fsm_dev, REG(FSM_ADCENV), 0x9FFF);
	ret |= fsm_reg_write(fsm_dev, REG(FSM_ADCTIME), 0x0038);
	// BFL, AGC
	ret |= fsm_reg_write(fsm_dev, REG(FSM_BFLCTRL), 0x0006);
	ret |= fsm_reg_write(fsm_dev, REG(FSM_BFLSET), 0x0093);
	ret |= fsm_reg_write(fsm_dev, REG(FSM_AGC), 0x00B5);
	ret |= fsm_reg_write(fsm_dev, REG(FSM_DRPARA), 0x0001);
	ret |= fsm_access_key(fsm_dev, 0);
	// TS control
	ret |= fsm_reg_write(fsm_dev, REG(FSM_TSCTRL), 0x162F);
	// wait stable: DACRUN=0
	ret |= fsm_wait_stable(fsm_dev, FSM_WAIT_AMP_OFF);
	if (ret) {
		pr_addr(err, "wait timeout");
		return ret;
	}

	return ret;
}

static int fs1603_f0_ram_init(fsm_dev_t *fsm_dev)
{
	int32_t coef_acs_dft[COEF_LEN] = {0x000603ac, 0x001603ac, 0x001603ac, 0x001603ac, 0x001603ac};
	int32_t coef_acs_gain[COEF_LEN] = {0x003b1bb5, 0x001603ac, 0x001603ac, 0x001603ac, 0x001603ac};
	int32_t coef_ad[COEF_LEN] = {0x0069b1cd, 0x001603ad, 0x001603ad, 0x001603ad, 0x001603ad};
	uint8_t buf[4];
	int count;
	int ret;
	int i;

	if (!fsm_dev) {
		return -EINVAL;
	}
	ret = fsm_access_key(fsm_dev, 1);
	// Set ADC coef(B0, B1)
	ret |= fsm_reg_write(fsm_dev, REG(FSM_ADCEQA), 0x0000);
	for (i = 0; i < COEF_LEN; i++) {
		convert_data_to_bytes(coef_ad[i], buf);
		ret |= fsm_burst_write(fsm_dev, REG(FSM_ADCEQWL), buf, sizeof(uint32_t));
	}
	for (i = 0; i < COEF_LEN; i++) {
		convert_data_to_bytes(coef_ad[i], buf);
		ret |= fsm_burst_write(fsm_dev, REG(FSM_ADCEQWL), buf, sizeof(uint32_t));
	}
	// Set acs to default coef
	ret |= fsm_reg_write(fsm_dev, fsm_dev->compat.ACSEQA, 0x0000);
	for (count = 0; count < ACS_COEF_COUNT; count++) {
		for (i = 0; i < COEF_LEN; i++) {
			convert_data_to_bytes(coef_acs_dft[i], buf);
			ret |= fsm_burst_write(fsm_dev, fsm_dev->compat.ACSEQWL, buf, 4);
		}
	}
	// Prescale default
	convert_data_to_bytes(0x000603AC, buf);
	ret |= fsm_burst_write(fsm_dev, fsm_dev->compat.ACSEQWL, buf, 4);

	// Set acs b2, b3
	ret |= fsm_reg_write(fsm_dev, fsm_dev->compat.ACSEQA, 0x000A);
	for (i = 0; i < COEF_LEN; i++) {
		convert_data_to_bytes(coef_acs_gain[i], buf);
		ret |= fsm_burst_write(fsm_dev, fsm_dev->compat.ACSEQWL, buf, 4);
	}
	for (i = 0; i < COEF_LEN; i++) {
		convert_data_to_bytes(coef_acs_gain[i], buf);
		ret |= fsm_burst_write(fsm_dev, fsm_dev->compat.ACSEQWL, buf, 4);
	}
	ret |= fsm_access_key(fsm_dev, 0);

	return ret;
}

int fs1603_pre_f0_test(fsm_dev_t *fsm_dev)
{
	int ret;

	if (!fsm_dev) {
		return -EINVAL;
	}
	fsm_dev->f0 = 0;
	fsm_dev->state.f0_runing = true;
	fsm_dev->errcode = 0;
	if (fsm_dev->tdata) {
		fsm_dev->tdata->test_f0 = 0;
		memset(&fsm_dev->tdata->f0, 0, sizeof(struct f0_data));
	}
	ret = fs1603_f0_reg_init(fsm_dev);
	// make sure amp off here
	// ret |= fsm_reg_read(fsm_dev, REG(FSM_SYSCTRL), NULL);
	ret = fsm_access_key(fsm_dev, 1);
	ret |= fs1603_f0_ram_init(fsm_dev);
	// ADC on
	ret |= fsm_reg_write(fsm_dev, REG(FSM_ADCCTRL), 0x1300);
	ret |= fsm_access_key(fsm_dev, 0);
	fsm_dev->errcode = ret;

	FSM_ADDR_EXIT(ret);
	return ret;
}

int fs1603_f0_test(fsm_dev_t *fsm_dev)
{
	fsm_config_t *cfg = fsm_get_config();
	const uint32_t *coef_acsbp = NULL;
	uint8_t buf[sizeof(uint32_t)];
	int freq;
	int ret;
	int i;

	if (!fsm_dev || !cfg) {
		return -EINVAL;
	}

	// CalculateBPCoef
	freq = cfg->test_freq;
	for (i = 0; i < ARRAY_SIZE(g_fs1603_bpcoef_table); i++) {
		if (freq == g_fs1603_bpcoef_table[i].freq) {
			coef_acsbp = g_fs1603_bpcoef_table[i].coef;
			break;
		}
	}
	if (!coef_acsbp) {
		pr_addr(err, "freq no matched: %d", freq);
		return -EINVAL;
	}
	// Keep amp off here.
	ret = fsm_reg_write(fsm_dev, REG(FSM_SYSCTRL), 0x0000);
	// wait stable
	ret = fsm_wait_stable(fsm_dev, FSM_WAIT_AMP_OFF);
	if (ret) {
		return ret;
	}
	// ACS EQ band 0 and band 1
	ret |= fsm_access_key(fsm_dev, 1);
	ret |= fsm_reg_write(fsm_dev, fsm_dev->compat.ACSEQA, 0x0000);
	for (i = 0; i < COEF_LEN; i++) {
		convert_data_to_bytes(coef_acsbp[i], buf);
		ret |= fsm_burst_write(fsm_dev, fsm_dev->compat.ACSEQWL,
								buf, sizeof(uint32_t));
	}
	for (i = 0; i < COEF_LEN; i++) {
		convert_data_to_bytes(coef_acsbp[i], buf);
		ret |= fsm_burst_write(fsm_dev, fsm_dev->compat.ACSEQWL,
								buf, sizeof(uint32_t));
	}
	ret |= fsm_access_key(fsm_dev, 0);
	// Amp on
	ret |= fsm_reg_write(fsm_dev, REG(FSM_SYSCTRL), 0x0008);
	// now need sleep 350ms, first time need 700ms
	fsm_dev->errcode |= ret;

	FSM_ADDR_EXIT(ret);
	return ret;
}

int fs1603_post_f0_test(fsm_dev_t *fsm_dev)
{
	int ret;

	if (!fsm_dev) {
		return -EINVAL;
	}
	fsm_dev->state.f0_runing = false;
	ret = fsm_reg_write(fsm_dev, REG(FSM_SYSCTRL), 0x0001);
	fsm_dev->errcode |= ret;

	return ret;
}

int fs1603s_start_up(fsm_dev_t *fsm_dev)
{
	uint16_t sysctrl;
	uint16_t adptrk;
	int ret;

	if (!fsm_dev) {
		return -EINVAL;
	}
	ret = fs1603_config_i2s(fsm_dev);
	ret |= fs1603_config_pll(fsm_dev, true);
	// power on device
	ret = fsm_reg_read(fsm_dev, REG(FSM_SYSCTRL), &sysctrl);
	// set_bf_val(&sysctrl, FSM_AMPE, 1);
	set_bf_val(&sysctrl, FSM_PWDN, 0);
	ret |= fsm_reg_read(fsm_dev, 0xCE, &adptrk);
	set_bf_val(&adptrk, 0x70CE, 0x9F);  // bit[7..0]=9F
	ret |= fsm_reg_write(fsm_dev, 0xCE, adptrk);
	ret |= fsm_reg_write(fsm_dev, REG(FSM_SYSCTRL), sysctrl);
	fsm_delay_ms(5);
	set_bf_val(&adptrk, 0x70CE, 0x00);  // bit[7..0]=0
	ret |= fsm_reg_write(fsm_dev, 0xCE, adptrk);
	ret |= fsm_config_vol(fsm_dev);
	fsm_dev->errcode = ret;

	return ret;
}

int fs1603s_pre_f0_test(fsm_dev_t *fsm_dev)
{
	uint16_t pos_mask;
	uint16_t value;
	int ret;

	if (!fsm_dev) {
		return -EINVAL;
	}
	fsm_dev->f0 = 0;
	fsm_dev->state.f0_runing = true;
	fsm_dev->errcode = 0;
	g_f0_test_done = 0;
	if (fsm_dev->tdata) {
		fsm_dev->tdata->test_f0 = 0;
		memset(&fsm_dev->tdata->f0, 0, sizeof(struct f0_data));
	}
	ret = fsm_reg_write(fsm_dev, REG(FSM_TSCTRL), 0x7623);   // avoid pop
	fsm_delay_ms(35);
	fsm_reg_write(fsm_dev, REG(FSM_SYSCTRL), 0x0002);   // reset chip
	fsm_reg_read(fsm_dev, REG(FSM_SYSCTRL), NULL);   // dummy read
	fsm_delay_ms(15);  // 15ms
	pos_mask = fsm_dev->pos_mask;
	fsm_dev->pos_mask = FSM_POS_MONO;
	ret = fs1603_config_i2s(fsm_dev);
	fsm_dev->pos_mask = pos_mask;
	ret |= fsm_reg_write(fsm_dev, REG(FSM_AUDIOCTRL), 0x0000);
	ret |= fsm_reg_write(fsm_dev, REG(FSM_SYSCTRL), 0x0000);
	ret |= fsm_access_key(fsm_dev, 1);
	ret |= fs1603_config_pll(fsm_dev, true);
	// power on chip and enable classD block(
	ret |= fsm_reg_write(fsm_dev, REG(FSM_SYSCTRL), 0x0008);
	// Set dem to 1st order
	ret |= fsm_reg_write(fsm_dev, 0xB0, 0x800A);
	ret |= fsm_reg_write(fsm_dev, REG(FSM_BSTCTRL), 0x831E);
	// Disable lnm
	ret |= fsm_reg_write(fsm_dev, 0x98, 0x0F11);
	// Get E1 for performance
	ret |= fsm_reg_write(fsm_dev, 0xE1, 0x2102);
	// Get E2 for performance
	ret |= fsm_reg_write(fsm_dev, 0xE2, 0x60FF);
	// DSP is Enable
	ret |= fsm_reg_write(fsm_dev, 0xA1, 0x1C13);
	// UnMute
	ret |= fsm_reg_write(fsm_dev, 0xAE, 0x0200);
	ret |= fsm_reg_write(fsm_dev, 0xAF, 0x1628);
	ret |= fsm_reg_write(fsm_dev, 0xB9, 0xBFFF);
	ret |= fsm_reg_write(fsm_dev, 0xB3, 0x1000);
	ret |= fsm_reg_write(fsm_dev, 0xBA, 0x001C);
	ret |= fsm_reg_write(fsm_dev, 0xB9, 0xBFFF);
	// bit[2:0] = 2 of 0x4F
	ret |= fsm_set_bf(fsm_dev, 0x204F, 2);  // 2bits left shift
	ret |= fsm_reg_write(fsm_dev, 0x4D, 0x2301);
	ret |= fsm_reg_read(fsm_dev, 0x4D, &value);
	set_bf_val(&value, 0x0F4D, 0);
	ret |= fsm_reg_write(fsm_dev, 0x4D, value);
	set_bf_val(&value, 0x0F4D, 1);
	ret |= fsm_reg_write(fsm_dev, 0x4D, value);
	ret |= fsm_access_key(fsm_dev, 0);
	fsm_dev->errcode = ret;

	return ret;
}

int fs1603s_f0_test(fsm_dev_t *fsm_dev)
{
	fsm_config_t *cfg = fsm_get_config();
	uint16_t value;
	int f0;
	int ret;

	if (!fsm_dev || !cfg) {
		return -EINVAL;
	}
	if (fsm_dev->f0 != 0) {
		return 0;
	}
	ret = fsm_reg_read(fsm_dev, 0x4E, &value);
	if (!ret && (value & 0x8000) == 0) { // bit[15] = 0
		f0 = 48 * ((value >> 8) & 0x003F) + 192;
		if (fsm_dev->tdata) {
			fsm_dev->tdata->test_f0 = f0;
		}
		fsm_dev->f0 = f0;
		pr_info("F0(Hz):%d", f0);
		// calc r0
		ret |= fsm_reg_write(fsm_dev, 0xB3, 0x1200);
		// checktime 100ms
		ret |= fsm_reg_write(fsm_dev, 0xBA, 0x0028);
		// average 8 times
		ret |= fsm_reg_write(fsm_dev, 0xB9, 0xDFFF);
		// adc eq disable
		//ret |= fsm_reg_write(fsm_dev, 0xB3, 0x1010);
		// set ts is 192Hz
		ret |= fsm_reg_write(fsm_dev, 0x4D, 0x2308);
		// set ts amp is +18dB
		ret |= fsm_reg_write(fsm_dev, 0xAF, 0x1628);
		g_f0_test_done++;
		if (g_f0_test_done == cfg->dev_count) {
			cfg->stop_test = true;
		}
	}
	fsm_dev->errcode |= ret;

	FSM_ADDR_EXIT(ret);
	return ret;
}

static int fs1603s_get_f0_zmdata(fsm_dev_t *fsm_dev)
{
	uint16_t value = 0;
	int ret = 0;
	int idx = 0;

	if (fsm_dev == NULL)
		return 0;

	ret = fsm_reg_write(fsm_dev, 0x4F, 0X0002);
	for (idx = 0; idx < FSM_F0_COUNT; idx++) {
		// set bit[13:8] = 0
		ret |= fsm_reg_write(fsm_dev, 0x4E, (value & 0xC0FF) | (idx << 8));
		// get bit[7:0]
		ret |= fsm_reg_read(fsm_dev, 0x4E, &value);
		fsm_dev->zmdata[idx] = value & 0x00FF;
		pr_info("zmdata[%d] : 0x%2x, 4E = 0x%04x", idx, fsm_dev->zmdata[idx], value);
	}

	return ret;
}

int fs1603s_post_f0_test(fsm_dev_t *fsm_dev)
{
	uint16_t value;
	int rs_trim;
	int r0;
	int ret;

	if (!fsm_dev) {
		return -EINVAL;
	}
	fsm_access_key(fsm_dev, 1);
	fs1603s_get_f0_zmdata(fsm_dev);
	// calc r0
	ret = fsm_reg_read(fsm_dev, 0xE6, &value);
	fsm_access_key(fsm_dev, 0);
	rs_trim = ((value == 0) ? 0x8F : (value & 0xFF));
	ret |= fsm_reg_multiread(fsm_dev, 0xBB, &value);
	r0 = FSM_MAGNIF(FS1603_RS2RL_RATIO * rs_trim) / value / 8;
	pr_addr(info, "zm:%04X, R0:%d", value, r0);
	fsm_dev->state.f0_runing = false;
	ret |= fsm_reg_write(fsm_dev, 0xAF, 0x1620);  // avoid pop
	fsm_delay_ms(35);
	ret = fsm_reg_write(fsm_dev, REG(FSM_SYSCTRL), 0x0001);
	fsm_dev->errcode |= ret;

	return ret;
}

void fs1603_ops(fsm_dev_t *fsm_dev)
{
	if (!fsm_dev) {
		return;
	}
	fsm_set_fw_name(FS1603_FW_NAME);
	fsm_dev->dev_ops.reg_init = fs1603_reg_init;
	fsm_dev->dev_ops.i2s_config = fs1603_config_i2s;
	fsm_dev->dev_ops.pll_config = fs1603_config_pll;
	fsm_dev->dev_ops.store_otp = fs1603_store_otp;
	fsm_dev->dev_ops.pre_f0_test = fs1603_pre_f0_test;
	fsm_dev->dev_ops.f0_test = fs1603_f0_test;
	fsm_dev->dev_ops.post_f0_test = fs1603_post_f0_test;
	if (fsm_dev->is1603s) {
		fsm_set_fw_name(FS1603S_FW_NAME);
		fsm_dev->dev_ops.pre_f0_test = fs1603s_pre_f0_test;
		fsm_dev->dev_ops.f0_test = fs1603s_f0_test;
		fsm_dev->dev_ops.post_f0_test = fs1603s_post_f0_test;
	}
	if (fsm_dev->version == FS1603S_V1) {
		fsm_dev->dev_ops.start_up = fs1603s_start_up;
	}
	fsm_dev->compat.preset_unit_len = FS1603_PRESET_EQ_LEN;
	fsm_dev->compat.excer_ram_len = FS1603_EXCER_RAM_LEN;
	fsm_dev->compat.addr_excer_ram = FS1603_EXCER_RAM_ADDR;
	fsm_dev->compat.otp_max_count = FS1603_OTP_COUNT_MAX;
	fsm_dev->compat.ACSEQA = REG(FS1603_ACSEQA);
	fsm_dev->compat.ACSEQWL = REG(FS1603_ACSEQWL);
	fsm_dev->compat.DACEQA = REG(FS1603_DACEQA);
	fsm_dev->compat.DACEQWL = REG(FS1603_DACEQWL);
	fsm_dev->compat.RS2RL_RATIO = FS1603_RS2RL_RATIO;
}
#endif
