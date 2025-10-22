/*
 * ILITEK Touch IC driver
 *
 * Copyright (C) 2011 ILI Technology Corporation.
 *
 * Author: Dicky Chiang <dicky_chiang@ilitek.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

#include "ilitek_v3.h"

#define VALUE			0
#define RETRY_COUNT		3
#define INT_CHECK		0
#define POLL_CHECK		1

#define BENCHMARK		1
#define NODETYPE		1

#define TYPE_BENCHMARK		0
#define TYPE_NO_JUGE		1
#define TYPE_JUGE		2

#define NORMAL_CSV_PASS_NAME	"mp_pass"
#define NORMAL_CSV_FAIL_NAME	"mp_fail"
#define NORMAL_CSV_WARNING_NAME	"mp_warning"

#define CSV_FILE_SIZE		(1 * M)

#define PARSER_MAX_CFG_BUF		(512 * 3)
#define PARSER_MAX_KEY_NUM		(600 * 3)
#define PARSER_MAX_KEY_NAME_LEN		100
#define PARSER_MAX_KEY_VALUE_LEN	2000
#define MAX_SECTION_NUM			100
#define BENCHMARK_KEY_NAME		"benchmark_data"
#define NODE_TYPE_KEY_NAME		"node type"
#define PRECOMMAND_KEY_NAME		"_precommand"
#define INI_ERR_OUT_OF_LINE		-1

#define CMD_MUTUAL_DAC			0x1
#define CMD_MUTUAL_BG			0x2
#define CMD_MUTUAL_SIGNAL		0x3
#define CMD_MUTUAL_NO_BK		0x5
#define CMD_MUTUAL_HAVE_BK		0x8
#define CMD_MUTUAL_BK_DAC		0x10
#define CMD_SELF_DAC			0xC
#define CMD_SELF_BG			0xF
#define CMD_SELF_SIGNAL			0xD
#define CMD_SELF_NO_BK			0xE
#define CMD_SELF_HAVE_BK		0xB
#define CMD_SELF_BK_DAC			0x11
#define CMD_KEY_DAC			0x14
#define CMD_KEY_BG			0x16
#define CMD_KEY_NO_BK			0x7
#define CMD_KEY_HAVE_BK			0x15
#define CMD_KEY_OPEN			0x12
#define CMD_KEY_SHORT			0x13
#define CMD_ST_DAC			0x1A
#define CMD_ST_BG			0x1C
#define CMD_ST_NO_BK			0x17
#define CMD_ST_HAVE_BK			0x1B
#define CMD_ST_OPEN			0x18
#define CMD_TX_SHORT			0x19
#define CMD_RX_SHORT			0x4
#define CMD_RX_OPEN			0x6
#define CMD_TX_RX_DELTA			0x1E
#define CMD_CM_DATA			0x9
#define CMD_CS_DATA			0xA
#define CMD_TRCRQ_PIN			0x20
#define CMD_RESX2_PIN			0x21
#define CMD_MUTUAL_INTEGRA_TIME		0x22
#define CMD_SELF_INTEGRA_TIME		0x23
#define CMD_KEY_INTERGRA_TIME		0x24
#define CMD_ST_INTERGRA_TIME		0x25
#define CMD_PEAK_TO_PEAK		0x1D
#define CMD_GET_TIMING_INFO		0x30
#define CMD_DOZE_P2P			0x32
#define CMD_DOZE_RAW			0x33
#define CMD_PIN_TEST			0x61
#define CMD_FW_MP_PRE_CMD		0xF7
#define CMD_FW_MP_PRE_SET_CMD		0xF8


#define MP_DATA_PASS			0
#define MP_DATA_FAIL			-1

#define Mathabs(x) ({					\
		long ret;				\
		if (sizeof(x) == sizeof(long)) {	\
		long __x = (x);				\
		ret = (__x < 0) ? -__x : __x;		\
		} else {				\
		int __x = (x);				\
		ret = (__x < 0) ? -__x : __x;		\
		}					\
		ret;					\
	})

#define DUMP(fmt, arg...)		\
	do {				\
		if (debug_en)	\
		pr_cont(fmt, ##arg);	\
	} while (0)

static struct ini_file_data {
	char section_name[PARSER_MAX_KEY_NAME_LEN];
	char key_name[PARSER_MAX_KEY_NAME_LEN];
	char key_value[PARSER_MAX_KEY_VALUE_LEN];
	int section_len;
	int key_name_len;
	int key_val_len;
} *ini_info;

enum open_test_node_type {
	NO_COMPARE = 0x00,	/* Not A Area, No Compare */
	AA_Area = 0x01,		/* AA Area, Compare using Charge_AA */
	Border_Area = 0x02, 	/* Border Area, Compare using Charge_Border */
	Notch = 0x04,		/* Notch Area, Compare using Charge_Notch */
	Round_Corner = 0x08,	/* Round Corner, No Compare */
	Skip_Micro = 0x10	/* Skip_Micro, No Compare */
};

enum mp_test_catalog {
	MUTUAL_TEST = 0,
	SELF_TEST = 1,
	KEY_TEST = 2,
	ST_TEST = 3,
	TX_RX_DELTA = 4,
	UNTOUCH_P2P = 5,
	PIXEL = 6,
	OPEN_TEST = 7,
	PEAK_TO_PEAK_TEST = 8,
	SHORT_TEST = 9,
	PIN_TEST = 10,
	TIPRING_RAW = 11,
	TIPRING_P2P = 12,
};

enum mp_tipring_type {
	TIPRING_XY = 0,
	TIPRING_X,
	TIPRING_Y
};

struct mp_test_P540_open {
	s32 *tdf_700;
	s32 *tdf_250;
	s32 *tdf_200;
	s32 *cbk_700;
	s32 *cbk_250;
	s32 *cbk_200;
	s32 *charg_rate;
	s32 *full_Open;
	s32 *dac;
};

struct mp_test_open_c {
	s32 *cap_dac;
	s32 *cap_raw;
	s32 *dcl_cap;
};

static struct open_test_para {
	int tvch;
	int tvcl;
	int gain;
	int cbk_step;
	int cint;
	int invbk;
	int vadc_range;
} open_para;

static struct shor_test_para {
	int tvch;
	int tvcl;
	int variation;
	int rinternal;
	int cint;
} short_para;

static struct core_mp_test_data {
	u32 chip_pid;
	u16 chip_id;
	u8 chip_type;
	u8 chip_ver;
	u32 fw_ver;
	u32 protocol_ver;
	u32 core_ver;
	char ini_date[128];
	char ini_ver[64];
	int no_bk_shift;
	bool retry;
	bool m_signal;
	bool m_dac;
	bool s_signal;
	bool s_dac;
	bool key_dac;
	bool st_dac;
	bool p_no_bk;
	bool p_has_bk;
	bool open_integ;
	bool open_cap;
	bool isLongV;
	bool td_retry;
	bool pre_cmd_sp;

	int cdc_len;
	int xch_len;
	int ych_len;
	int nPxRaw;
	int nPyRaw;
	int stx_len;
	int srx_len;
	int key_len;
	int st_len;
	int frame_len;
	int mp_items;
	int final_result;
	int short_varia;
	int nPxRaw_tipring_len;
	int nPyRaw_tipring_len;
	int tipring_len;
	u32 overlay_start_addr;
	u32 overlay_end_addr;
	u32 mp_flash_addr;
	u32 mp_size;
	u8 dma_trigger_enable;

	/* Tx/Rx threshold & buffer */
	int TxDeltaMax;
	int TxDeltaMin;
	int RxDeltaMax;
	int RxDeltaMin;
	s32 *tx_delta_buf;
	s32 *rx_delta_buf;
	s32 *tx_max_buf;
	s32 *tx_min_buf;
	s32 *rx_max_buf;
	s32 *rx_min_buf;

	int tdf;
	int busy_cdc;
	bool ctrl_lcm;
	bool lost_benchmark;
	bool lost_parameter;
} core_mp = {0};

struct mp_test_items {
	/* The description must be the same as ini's section name */
	char *desp;
	char *result;
	int catalog;
	u8 cmd;
	u8 spec_option;
	u8 type_option;
	bool run;
	bool lcm;
	int bch_mrk_multi;
	int max;
	int max_res;
	int item_result;
	int min;
	int min_res;
	int max1;
	int max2;
	int min1;
	int min2;
	int frame_count;
	int trimmed_mean;
	int lowest_percentage;
	int highest_percentage;
	int v_tdf_1;
	int v_tdf_2;
	int h_tdf_1;
	int h_tdf_2;
	int goldenmode;
	int max_min_mode;
	int bch_mrk_frm_num;
	int retry_cnt;
	int pre_cmd_en;
	u8  delay_time;
	u8  test_int_pin;
	u8  int_pulse_test;
	s32 *result_buf;
	s32 *buf;
	s32 *max_buf;
	s32 *min_buf;
	s32 *bench_mark_max;
	s32 *bench_mark_min;
	s32 **bch_mrk_max;
	s32 **bch_mrk_min;
	s32 *node_type;
	int (*do_test)(int index);
};

#define MP_TEST_ITEM	55
static struct mp_test_items tItems[MP_TEST_ITEM] = {
	{.desp = "baseline data(bg)", .catalog = MUTUAL_TEST, .cmd = CMD_MUTUAL_BG, .lcm = ON},
	{.desp = "untouch signal data(bg-raw-4096) - mutual", .catalog = MUTUAL_TEST, .cmd = CMD_MUTUAL_SIGNAL, .lcm = ON},
	{.desp = "manual bk data(mutual)", .catalog = MUTUAL_TEST, .cmd = CMD_MUTUAL_BK_DAC, .lcm = ON},
	{.desp = "calibration data(dac) - self", .catalog = SELF_TEST, .cmd = CMD_SELF_DAC, .lcm = ON},
	{.desp = "baselin data(bg,self_tx,self_r)", .catalog = SELF_TEST, .cmd = CMD_SELF_BG, .lcm = ON},
	{.desp = "untouch signal data(bg-raw-4096) - self", .catalog = SELF_TEST, .cmd = CMD_SELF_SIGNAL, .lcm = ON},
	{.desp = "raw data(no bk) - self", .catalog = SELF_TEST, .cmd = CMD_SELF_NO_BK, .lcm = ON},
	{.desp = "raw data(have bk) - self", .catalog = SELF_TEST, .cmd = CMD_SELF_HAVE_BK, .lcm = ON},
	{.desp = "manual bk dac data(self_tx,self_rx)", .catalog = SELF_TEST, .cmd = CMD_SELF_BK_DAC, .lcm = ON},
	{.desp = "calibration data(dac/icon)", .catalog = KEY_TEST, .cmd = CMD_KEY_DAC, .lcm = ON},
	{.desp = "key baseline data", .catalog = KEY_TEST, .cmd = CMD_KEY_BG, .lcm = ON},
	{.desp = "key raw data", .catalog = KEY_TEST, .cmd = CMD_KEY_NO_BK, .lcm = ON},
	{.desp = "key raw bk dac", .catalog = KEY_TEST, .cmd = CMD_KEY_HAVE_BK, .lcm = ON},
	{.desp = "key raw open test", .catalog = KEY_TEST, .cmd = CMD_KEY_OPEN, .lcm = ON},
	{.desp = "key raw short test", .catalog = KEY_TEST, .cmd = CMD_KEY_SHORT, .lcm = ON},
	{.desp = "st calibration data(dac)", .catalog = ST_TEST, .cmd = CMD_ST_DAC, .lcm = ON},
	{.desp = "st baseline data(bg)", .catalog = ST_TEST, .cmd = CMD_ST_BG, .lcm = ON},
	{.desp = "st raw data(no bk)", .catalog = ST_TEST, .cmd = CMD_ST_NO_BK, .lcm = ON},
	{.desp = "st raw(have bk)", .catalog = ST_TEST, .cmd = CMD_ST_HAVE_BK, .lcm = ON},
	{.desp = "st open data", .catalog = ST_TEST, .cmd = CMD_ST_OPEN, .lcm = ON},
	{.desp = "tx short test", .catalog = MUTUAL_TEST, .cmd = CMD_TX_SHORT, .lcm = ON},
	{.desp = "rx open", .catalog = MUTUAL_TEST, .cmd = CMD_RX_OPEN, .lcm = ON},
	{.desp = "untouch cm data", .catalog = MUTUAL_TEST, .cmd = CMD_CM_DATA, .lcm = ON},
	{.desp = "untouch cs data", .catalog = MUTUAL_TEST, .cmd = CMD_CS_DATA, .lcm = ON},
	{.desp = "tx/rx delta", .catalog = TX_RX_DELTA, .cmd = CMD_TX_RX_DELTA, .lcm = ON},
	{.desp = "untouch peak to peak", .catalog = UNTOUCH_P2P, .cmd = CMD_MUTUAL_SIGNAL, .lcm = ON},
	{.desp = "pixel raw (no bk)", .catalog = PIXEL, .cmd = CMD_MUTUAL_NO_BK, .lcm = ON},
	{.desp = "pixel raw (have bk)", .catalog = PIXEL, .cmd = CMD_MUTUAL_HAVE_BK, .lcm = ON},
	{.desp = "noise peak to peak(cut panel)", .catalog = PEAK_TO_PEAK_TEST, .lcm = ON},
	{.desp = "open test(integration)", .catalog = OPEN_TEST, .cmd = CMD_RX_SHORT, .lcm = ON},
	{.desp = "open test(cap)", .catalog = OPEN_TEST, .cmd = CMD_RX_SHORT, .lcm = ON},
	/* Following is the new test items for protocol 5.4.0 above */
	{.desp = "pin test ( int and rst )", .catalog = PIN_TEST, .cmd = CMD_PIN_TEST, .lcm = ON},
	{.desp = "noise peak to peak(with panel)", .catalog = PEAK_TO_PEAK_TEST, .lcm = ON},
	{.desp = "noise peak to peak(ic only)", .catalog = PEAK_TO_PEAK_TEST, .cmd = CMD_PEAK_TO_PEAK, .lcm = ON},
	{.desp = "open test(integration)_sp", .catalog = OPEN_TEST, .lcm = ON},
	{.desp = "raw data(no bk)", .catalog = MUTUAL_TEST, .cmd = CMD_MUTUAL_NO_BK, .lcm = ON},
	{.desp = "raw data(have bk)", .catalog = MUTUAL_TEST, .cmd = CMD_MUTUAL_HAVE_BK, .lcm = ON},
	{.desp = "calibration data(dac)", .catalog = MUTUAL_TEST, .cmd = CMD_MUTUAL_DAC, .lcm = ON},
	{.desp = "short test -ili9881", .catalog = SHORT_TEST, .cmd = CMD_RX_SHORT, .lcm = ON},
	{.desp = "short test", .catalog = SHORT_TEST, .lcm = ON},
	{.desp = "doze raw data", .catalog = MUTUAL_TEST, .lcm = ON},
	{.desp = "doze peak to peak", .catalog = PEAK_TO_PEAK_TEST, .lcm = ON},
	{.desp = "open test_c", .catalog = OPEN_TEST, .lcm = ON},
	{.desp = "touch deltac", .catalog = MUTUAL_TEST, .lcm = ON},
	{.desp = "pen tipring raw test", .catalog = TIPRING_RAW, .lcm = ON},
	{.desp = "pen tipring p2p test", .catalog = TIPRING_P2P, .lcm = ON},
	{.desp = "open test_c (openx enable)", .catalog = OPEN_TEST, .lcm = ON},
	/* LCM OFF TEST */
	{.desp = "raw data(have bk) (lcm off)", .catalog = MUTUAL_TEST, .lcm = OFF},
	{.desp = "raw data(no bk) (lcm off)", .catalog = MUTUAL_TEST, .lcm = OFF},
	{.desp = "noise peak to peak(with panel) (lcm off)", .catalog = PEAK_TO_PEAK_TEST, .lcm = OFF},
	{.desp = "noise peak to peak(ic only) (lcm off)", .catalog = PEAK_TO_PEAK_TEST, .lcm = OFF},
	{.desp = "raw data_td (lcm off)", .catalog = MUTUAL_TEST, .lcm = OFF},
	{.desp = "peak to peak_td (lcm off)", .catalog = PEAK_TO_PEAK_TEST, .lcm = OFF},
	{.desp = "pen tipring raw test (lcm off)", .catalog = TIPRING_RAW, .lcm = OFF},
	{.desp = "pen tipring p2p test (lcm off)", .catalog = TIPRING_P2P, .lcm = OFF},
};

static struct run_index {
	int count;
	int index[MP_TEST_ITEM];
} ri;

static s32 *frame_buf;
static s32 **frm_buf;
static s32 *key_buf;
static s32 *frame1_cbk700, *frame1_cbk250, *frame1_cbk200;
static s32 *cap_dac, *cap_raw;
static s32 *openx_cap_dac, *openx_cap_raw;
static int g_ini_items;
static char csv_name[128] = {0};
static char seq_item[MAX_SECTION_NUM][PARSER_MAX_KEY_NAME_LEN] = {{0}};

static int isspace_t(int x)
{
	if (x == ' ' || x == '\t' || x == '\n' ||
			x == '\f' || x == '\b' || x == '\r')
		return 1;
	else
		return 0;
}

static void dump_benchmark_data(s32 *max_ptr, s32 *min_ptr)
{
	ili_dump_data(max_ptr, 32, core_mp.frame_len, 0, "Dump Benchmark Max : ");
	ili_dump_data(min_ptr, 32, core_mp.frame_len, 0, "Dump Benchmark Min : ");
}

static void dump_node_type_buffer(s32 *node_ptr, u8 *name)
{
	ili_dump_data(node_ptr, 32, core_mp.frame_len, 0, "Dump NodeType : ");
}

static int parser_get_ini_key_value(char *section, char *key, char *value)
{
	int i = 0;
	int ret = -2;

	for (i = 0; i < g_ini_items; i++) {
		if (ipio_strcmp(section, ini_info[i].section_name) != 0)
			continue;

		if (ipio_strcmp(key, ini_info[i].key_name) == 0) {
			ipio_memcpy(value, ini_info[i].key_value, ini_info[i].key_val_len, PARSER_MAX_KEY_VALUE_LEN);
			ILI_DBG("(key: %s, value:%s) => (ini key: %s, val: %s)\n",
					key,
					value,
					ini_info[i].key_name,
					ini_info[i].key_value);
			ret = 0;
			break;
		}
	}
	return ret;
}

static void parser_ini_nodetype(s32 *type_ptr, char *desp, int frame_len)
{
	int i = 0, j = 0, index1 = 0, temp, count = 0;
	char str[512] = {0}, record = ',';

	for (i = 0; i < g_ini_items; i++) {
		if (!strstr(ini_info[i].section_name, desp) ||
			ipio_strcmp(ini_info[i].key_name, NODE_TYPE_KEY_NAME) != 0) {
			continue;
		}

		record = ',';
		for (j = 0, index1 = 0; j <= ini_info[i].key_val_len; j++) {
			if (ini_info[i].key_value[j] == ';' || j == ini_info[i].key_val_len) {
				if (record != '.') {
					memset(str, 0, sizeof(str));
					ipio_memcpy(str, &ini_info[i].key_value[index1], (j - index1), sizeof(str));
					temp = ili_katoi(str);

					/* Over boundary, end to calculate. */
					if (count >= frame_len) {
						ILI_ERR("count(%d) is larger than frame length, break\n", count);
						break;
					}
					type_ptr[count] = temp;
					count++;
				}
				record = ini_info[i].key_value[j];
				index1 = j + 1;
			}
		}
	}
}

static int parser_sramtest_group(char *str, char (*strSramPartialArea)[SRAM_TEST_GROUP_STR_LEN])
{
	const char *delim = ";";
	char *temp = NULL; 
	char *pstrtemp = str;
	u8 group_num = 0;

	ILI_INFO("Parsing Sram group\n");

	temp = strsep(&pstrtemp, delim);
	strcpy(strSramPartialArea[group_num], temp);

	while(temp != NULL) {
		ILI_DBG("temp = %s, Sram Partial Area[%d] = %s\n", temp, group_num, strSramPartialArea[group_num]);
		group_num++;
		temp = strsep(&pstrtemp, delim);
		if (temp == NULL) {
			ILI_INFO("Parsing Sram group Done\n");
			break;
		}
		strcpy(strSramPartialArea[group_num], temp);
	}

	return group_num;
}

static void parser_ini_benchmark(s32 *max_ptr, s32 *min_ptr, int8_t type, char *desp, int frame_len, char *bchmrk_name)
{
	int i = 0, j = 0, index1 = 0, temp, count = 0;
	char str[512] = {0}, record = ',';
	s32 data[4];
	char benchmark_str[256] = {0};

	/* format complete string from the name of section "_Benchmark_Data". */
	snprintf(benchmark_str, sizeof(benchmark_str), "%s%s%s", desp, "_", bchmrk_name);
	ILI_DBG("benchmark_str = %s\n", benchmark_str);

	for (i = 0; i < g_ini_items; i++) {
		if ((ipio_strcmp(ini_info[i].section_name, benchmark_str) != 0))
			continue;
		record = ',';
		for (j = 0, index1 = 0; j <= ini_info[i].key_val_len; j++) {
			if (ini_info[i].key_value[j] == ',' || ini_info[i].key_value[j] == ';' ||
				ini_info[i].key_value[j] == '.' || j == ini_info[i].key_val_len) {

				if (record != '.') {
					memset(str, 0, sizeof(str));
					ipio_memcpy(str, &ini_info[i].key_value[index1], (j - index1), sizeof(str));
					temp = ili_katoi(str);
					data[(count % 4)] = temp;

					/* Over boundary, end to calculate. */
					if ((count / 4) >= frame_len) {
						ILI_ERR("count (%d) is larger than frame length, break\n", (count / 4));
						break;
					}

					if ((count % 4) == 3) {
						if (data[0] == 1) {
							if (type == VALUE) {
								max_ptr[count/4] = data[1] + data[2];
								min_ptr[count/4] = data[1] - data[3];
							} else {
								max_ptr[count/4] = data[1] + (data[1] * data[2]) / 100;
								min_ptr[count/4] = data[1] - (data[1] * data[3]) / 100;
							}
						} else {
							max_ptr[count/4] = INT_MAX;
							min_ptr[count/4] = INT_MIN;
						}
					}
					count++;
				}
				record = ini_info[i].key_value[j];
				index1 = j + 1;
			}
		}
	}
}

static char *get_csv_frame_name(char *name, int num)
{
	static char frame_name[128] = { 0 };
	memset(frame_name, 0, sizeof(frame_name));
	sprintf(frame_name, "%s%d", name, num);

	return frame_name;
}


static void parser_tip_ring_ini_benchmark(s32 *max_ptr, s32 *min_ptr, int8_t type, char *desp, int frame_len) {
	char benchmark_str[128] = {0};

	snprintf(benchmark_str, sizeof(benchmark_str), "%s%d", BENCHMARK_KEY_NAME, 1);
	parser_ini_benchmark(max_ptr, min_ptr, type, desp, core_mp.nPxRaw_tipring_len, benchmark_str);

	memset(benchmark_str, 0x0, sizeof(benchmark_str));
	snprintf(benchmark_str, sizeof(benchmark_str), "%s%d", BENCHMARK_KEY_NAME, 2);

	parser_ini_benchmark(max_ptr + core_mp.nPxRaw_tipring_len, min_ptr + core_mp.nPxRaw_tipring_len,
				type, desp, core_mp.nPyRaw_tipring_len, benchmark_str);

	ili_dump_data(max_ptr, 32, core_mp.tipring_len, 0, "Dump Tip Ring Benchmark Max");
	ili_dump_data(min_ptr, 32, core_mp.tipring_len, 0, "Dump Tip Ring Benchmark Min");
}

static int parser_get_tdf_value(char *str, int catalog)
{
	u32 i, ans, index = 0, flag = 0, count = 0, size = 0;
	char s[10] = {0};

	if (!str) {
		ILI_ERR("String is null\n");
		return -1;
	}

	size = strlen(str);
	for (i = 0, count = 0; i < size; i++) {
		if (str[i] == '.') {
			flag = 1;
			continue;
		}
		s[index++] = str[i];
		if (flag)
			count++;
	}
	ans = ili_katoi(s);

	/* Multiply by 100 to shift out of decimal point */
	if (catalog == SHORT_TEST) {
		if (count == 0)
			ans = ans * 100;
		else if (count == 1)
			ans = ans * 10;
	}

	return ans;
}

static int parser_get_u8_array(char *key, u8 *buf, u16 base, int len)
{
	char *s = key;
	char *pToken;
	int ret, conut = 0;
	long s_to_long = 0;

	if (strlen(s) == 0 || len <= 0) {
		ILI_ERR("Can't find any characters inside buffer\n");
		return -1;
	}

	/*
	 *	@base: The number base to use. The maximum supported base is 16. If base is
	 *	given as 0, then the base of the string is automatically detected with the
	 *	conventional semantics - If it begins with 0x the number will be parsed as a
	 *	hexadecimal (case insensitive), if it otherwise begins with 0, it will be
	 *	parsed as an octal number. Otherwise it will be parsed as a decimal.
	 */
	if (isspace_t((int)(unsigned char)*s) == 0) {
		while ((pToken = strsep(&s, ",")) != NULL) {
			ret = kstrtol(pToken, base, &s_to_long);
			if (ret == 0)
				buf[conut] = s_to_long;
			else
				ILI_INFO("convert string too long, ret = %d\n", ret);
			conut++;

			if (conut >= len)
				break;
		}
	}

	return conut;
}

static int parser_get_u32_array(char *key, u32 *buf, u16 base, int len)
{
	char *s = key;
	char *pToken;
	int ret, conut = 0;
	long s_to_long = 0;

	if (strlen(s) == 0 || len <= 0) {
		ILI_ERR("Can't find any characters inside buffer\n");
		return -1;
	}

	/*
	 *	@base: The number base to use. The maximum supported base is 16. If base is
	 *	given as 0, then the base of the string is automatically detected with the
	 *	conventional semantics - If it begins with 0x the number will be parsed as a
	 *	hexadecimal (case insensitive), if it otherwise begins with 0, it will be
	 *	parsed as an octal number. Otherwise it will be parsed as a decimal.
	 */
	if (isspace_t((int)(unsigned char)*s) == 0) {
		while ((pToken = strsep(&s, ";")) != NULL) {
			ret = kstrtol(pToken, base, &s_to_long);
			if (ret == 0)
				buf[conut] = s_to_long;
			else
				ILI_INFO("convert string too long, ret = %d\n", ret);
			conut++;

			if (conut >= len)
				break;
		}
	}

	return conut;
}

static int parser_get_int_data(char *section, char *keyname, char *rv, int rv_len)
{
	int len = 0;
	char value[512] = { 0 };

	if (rv == NULL || section == NULL || keyname == NULL) {
		ILI_ERR("Parameters are invalid\n");
		return -EINVAL;
	}

	/* return a white-space string if get nothing */
	if (parser_get_ini_key_value(section, keyname, value) < 0) {
		snprintf(rv, rv_len, "%s", value);
		return 0;
	}

	len = snprintf(rv, rv_len, "%s", value);
	return len;
}

/* Count the number of each line and assign the content to tmp buffer */
static int parser_get_ini_phy_line(char *data, char *buffer, int maxlen)
{
	int i = 0;
	int j = 0;
	int iRetNum = -1;
	char ch1 = '\0';

	for (i = 0, j = 0; i < maxlen; j++) {
		ch1 = data[j];
		iRetNum = j + 1;
		if (ch1 == '\n' || ch1 == '\r') {	/* line end */
			ch1 = data[j + 1];
			if (ch1 == '\n' || ch1 == '\r')
				iRetNum++;
			break;
		} else if (ch1 == 0x00) {
			break;	/* file end */
		}

		buffer[i++] = ch1;
	}

	buffer[i] = '\0';
	return iRetNum;
}

static char *parser_ini_str_trim_r(char *buf)
{
	int len, i;
	char *tmp = NULL;
	char x[512] = {0};
	char *y = NULL;
	char *empty = "";

	len = strlen(buf);

	if (len < sizeof(x)) {
		tmp = x;
		goto copy;
	}

	y = kzalloc(len, GFP_KERNEL);
	if (ERR_ALLOC_MEM(y)) {
		ILI_ERR("Failed to allocate tmp buf\n");
		return empty;
	}
	tmp = y;

copy:
	for (i = 0; i < len; i++) {
		if (buf[i] != ' ')
			break;
	}

	if (i < len)
		strncpy(tmp, (buf + i), (len - i));

	strncpy(buf, tmp, len);
	ipio_kfree((void **)&y);
	return buf;
}

static int parser_get_ini_phy_data(char *data, int fsize)
{
	int i, n = 0, ret = 0, empty_section;
	int offset = 0, isEqualSign = 0, scount = 0;
	char *ini_buf = NULL, *tmpSectionName = NULL;
	char M_CFG_SSL = '[';
	char M_CFG_SSR = ']';
/* char M_CFG_NIS = ':'; */
	char M_CFG_NTS = '#';
	char M_CFG_EQS = '=';

	if (data == NULL) {
		ILI_ERR("INI data is NULL\n");
		ret = -EINVAL;
		goto out;
	}

	ini_buf = kzalloc((PARSER_MAX_CFG_BUF + 1) * sizeof(char), GFP_KERNEL);
	if (ERR_ALLOC_MEM(ini_buf)) {
		ILI_ERR("Failed to allocate ini_buf memory, %ld\n", PTR_ERR(ini_buf));
		ret = -ENOMEM;
		goto out;
	}

	tmpSectionName = kzalloc((PARSER_MAX_CFG_BUF + 1) * sizeof(char), GFP_KERNEL);
	if (ERR_ALLOC_MEM(tmpSectionName)) {
		ILI_ERR("Failed to allocate tmpSectionName memory, %ld\n", PTR_ERR(tmpSectionName));
		ret = -ENOMEM;
		goto out;
	}

	memset(seq_item, 0, MP_TEST_ITEM * PARSER_MAX_KEY_NAME_LEN * sizeof(char));

	while (true) {
		empty_section = 0;
		if (g_ini_items > PARSER_MAX_KEY_NUM) {
			ILI_ERR("MAX_KEY_NUM: Out of length\n");
			goto out;
		}

		if (offset >= fsize)
			goto out;/*over size*/

		n = parser_get_ini_phy_line(data + offset, ini_buf, PARSER_MAX_CFG_BUF);
		if (n < 0) {
			ILI_ERR("End of Line\n");
			goto out;
		}

		offset += n;

		n = strlen(parser_ini_str_trim_r(ini_buf));

		if (n == 0 || ini_buf[0] == M_CFG_NTS)
			continue;

		/* Get section names */
		if (n > 2 && ((ini_buf[0] == M_CFG_SSL && ini_buf[n - 1] != M_CFG_SSR))) {
			ILI_ERR("Bad Section: %s\n", ini_buf);
			ret = -EINVAL;
			goto out;
		} else {
			if (ini_buf[0] == M_CFG_SSL) {
				ini_info[g_ini_items].section_len = n - 2;
				if (ini_info[g_ini_items].section_len > PARSER_MAX_KEY_NAME_LEN) {
					ILI_ERR("MAX_KEY_NAME_LEN: Out Of Length\n");
					ret = INI_ERR_OUT_OF_LINE;
					goto out;
				}

				if (scount > MAX_SECTION_NUM) {
					ILI_ERR("seq_item is over than its define (%d), abort\n", scount);
					ret = INI_ERR_OUT_OF_LINE;
					goto out;
				}

				ini_buf[n - 1] = 0x00;
				strncpy((char *)tmpSectionName, ini_buf + 1, (PARSER_MAX_CFG_BUF + 1) * sizeof(char));
				strncpy(seq_item[scount], tmpSectionName, PARSER_MAX_KEY_NAME_LEN);
				scount++;
				ILI_DBG("Section Name: %s, Len: %d, offset = %d\n", seq_item[scount], n - 2, offset);
				continue;
			}
		}

		/* copy section's name without square brackets to its real buffer */
		strncpy(ini_info[g_ini_items].section_name, tmpSectionName, (PARSER_MAX_KEY_NAME_LEN * sizeof(char)));
		ini_info[g_ini_items].section_len = strlen(tmpSectionName);

		isEqualSign = 0;
		for (i = 0; i < n; i++) {
			if (ini_buf[i] == M_CFG_EQS) {
				isEqualSign = i;
				break;
			}
			if (ini_buf[i] == M_CFG_SSL || ini_buf[i] == M_CFG_SSR) {
				empty_section = 1;
				break;
			}
		}

		if (isEqualSign == 0) {
			if (empty_section)
				continue;

			if (strstr(ini_info[g_ini_items].section_name, BENCHMARK_KEY_NAME) != NULL) {
				isEqualSign = -1;
				ini_info[g_ini_items].key_name_len = strlen(ini_info[g_ini_items].section_name);
				strncpy(ini_info[g_ini_items].key_name, ini_info[g_ini_items].section_name, (PARSER_MAX_KEY_NAME_LEN * sizeof(char)));
				ini_info[g_ini_items].key_val_len = n;
			} else if (strstr(ini_info[g_ini_items].section_name, NODE_TYPE_KEY_NAME) != NULL) {
				isEqualSign = -1;
				ini_info[g_ini_items].key_name_len = strlen(NODE_TYPE_KEY_NAME);
				strncpy(ini_info[g_ini_items].key_name, NODE_TYPE_KEY_NAME, (PARSER_MAX_KEY_NAME_LEN * sizeof(char)));
				ini_info[g_ini_items].key_val_len = n;
			} else if (strstr(ini_info[g_ini_items].section_name, PRECOMMAND_KEY_NAME) != NULL) {
				isEqualSign = -1;
				ini_info[g_ini_items].key_name_len = strlen(ini_info[g_ini_items].section_name);
				strncpy(ini_info[g_ini_items].key_name, ini_info[g_ini_items].section_name, (PARSER_MAX_KEY_NAME_LEN * sizeof(char)));
				ini_info[g_ini_items].key_val_len = n;
			} else {
				continue;
			}
		} else {
			ini_info[g_ini_items].key_name_len = isEqualSign;
			if (ini_info[g_ini_items].key_name_len > PARSER_MAX_KEY_NAME_LEN) {
				/* ret = CFG_ERR_OUT_OF_LEN; */
				ILI_ERR("MAX_KEY_NAME_LEN: Out Of Length\n");
				ret = INI_ERR_OUT_OF_LINE;
				goto out;
			}

			ipio_memcpy(ini_info[g_ini_items].key_name, ini_buf,
						ini_info[g_ini_items].key_name_len, PARSER_MAX_KEY_NAME_LEN);
			ini_info[g_ini_items].key_val_len = n - isEqualSign - 1;
		}

		if (ini_info[g_ini_items].key_val_len > PARSER_MAX_KEY_VALUE_LEN) {
			ILI_ERR("MAX_KEY_VALUE_LEN: Out Of Length\n");
			ret = INI_ERR_OUT_OF_LINE;
			goto out;
		}

		ipio_memcpy(ini_info[g_ini_items].key_value,
			   ini_buf + isEqualSign + 1, ini_info[g_ini_items].key_val_len, PARSER_MAX_KEY_VALUE_LEN);

		ILI_DBG("%s = %s\n", ini_info[g_ini_items].key_name,
			ini_info[g_ini_items].key_value);

		g_ini_items++;
	}
out:
	ipio_kfree((void **)&ini_buf);
	ipio_kfree((void **)&tmpSectionName);
	return ret;
}

int ilitek_tddi_mp_ini_parser(const char *path)
{
	int i, ret = 0, fsize = 0;
	char *tmp = NULL;
	const struct firmware *ini = NULL;
#if GENERIC_KERNEL_IMAGE
	ILI_ERR("GKI version not allow drivers to use filp_open\n");
	path = ilits->md_ini_rq_path;
	ILI_INFO("request path = %s\n", path);
	if (request_firmware(&ini, path, ilits->dev) < 0) {
		ILI_ERR("Request ini file failed\n");
		return -EINVAL;
	}

	fsize = ini->size;
#else
	struct file *f = NULL;
#if LINUX_VERSION_CODE <= KERNEL_VERSION(4, 14, 0)
	mm_segment_t old_fs;
#endif
	loff_t pos = 0;

	ILI_INFO("ini file path = %s\n", path);

	f = filp_open(path, O_RDONLY, 644);
	if (ERR_ALLOC_MEM(f)) {
		ILI_ERR("Failed to open ini file at %ld, try to request_firmware\n", PTR_ERR(f));
		f = NULL;
		path = ilits->md_ini_rq_path;
		ILI_INFO("request path = %s\n", path);
		if (request_firmware(&ini, path, ilits->dev) < 0) {
			ILI_ERR("Request ini file failed\n");
			return -EINVAL;
		}
	}

	if (f != NULL) {
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 19, 0))
		fsize = f->f_inode->i_size;
#else
		struct inode *inode;
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 18, 0))
		inode = f->f_dentry->d_inode;
#else
		inode = f->f_path.dentry->d_inode;
#endif
		fsize = inode->i_size;
#endif
	} else {
		fsize = ini->size;
	}
#endif

	ILI_INFO("ini file size = %d\n", fsize);
	if (fsize <= 0) {
		ILI_ERR("The size of file is invaild\n");
		ret = -EINVAL;
		goto out;
	}

	tmp = vmalloc(fsize+1);
	if (ERR_ALLOC_MEM(tmp)) {
		ILI_ERR("Failed to allocate tmp memory, %ld\n", PTR_ERR(tmp));
		ret = -ENOMEM;
		goto out;
	}
#if GENERIC_KERNEL_IMAGE
	memcpy(tmp, ini->data, fsize);
#else
	if (f != NULL) {
		pos = 0;
#if LINUX_VERSION_CODE <= KERNEL_VERSION(4, 14, 0)
		old_fs = get_fs();
		set_fs(KERNEL_DS);
		vfs_read(f, tmp, fsize, &pos);
		set_fs(old_fs);
#else
		kernel_read(f, tmp, fsize, &pos);
#endif
		tmp[fsize] = 0x0;
	} else {
		memcpy(tmp, ini->data, fsize);
	}
#endif
	g_ini_items = 0;

	/* Initialise ini strcture */
	for (i = 0; i < PARSER_MAX_KEY_NUM; i++) {
		memset(ini_info[i].section_name, 0, PARSER_MAX_KEY_NAME_LEN);
		memset(ini_info[i].key_name, 0, PARSER_MAX_KEY_NAME_LEN);
		memset(ini_info[i].key_value, 0, PARSER_MAX_KEY_VALUE_LEN);
		ini_info[i].section_len = 0;
		ini_info[i].key_name_len = 0;
		ini_info[i].key_val_len = 0;
	}

	/* change all characters to lower case */
	for (i = 0; i < fsize; i++)
		tmp[i] = tolower(tmp[i]);

	ret = parser_get_ini_phy_data(tmp, fsize);
	if (ret < 0) {
		ILI_ERR("Failed to get physical ini data, ret = %d\n", ret);
		goto out;
	}

	ILI_INFO("Parsed ini file done\n");
out:
	ipio_vfree((void **)&tmp);
#if GENERIC_KERNEL_IMAGE
	release_firmware(ini);
#else
	if (f != NULL)
		filp_close(f, NULL);
	else
		release_firmware(ini);
#endif
	return ret;
}

static int parser_ini_precmd(char *desp, u8 *buf)
{

	int i = 0, cmdlen = 0;
	u8 cmdnum = 0;
	u8 cmd[16] = {0};
	char precmd_str[128] = {0};

	/* format complete string from the name of section "_Benchmark_Data". */
	snprintf(precmd_str, sizeof(precmd_str), "%s_precommand", desp);
	ILI_INFO("precmd = %s\n", precmd_str);

	for (i = 0; i < g_ini_items; i++) {
		if ((ipio_strcmp(ini_info[i].section_name, precmd_str) != 0))
			continue;

		if (parser_get_u8_array(ini_info[i].key_value, cmd, 16, 11) < 0)
			return -1;
		cmdnum++;

		buf[0] = CMD_FW_MP_PRE_SET_CMD;
		buf[1] = cmdnum;
		strncpy(buf + 2 + ((cmdnum - 1) * 11), cmd, 11);

	}
	cmdlen = 2 + cmdnum * 11;

	return cmdlen;
}

static int precommd(u8 *cmd, int cmdlen, u8 *precmd, int precmdlen, u8 *rdata, u32 rlen, bool spi_irq, bool i2c_irq)
{
	u8 buf[2] = {0};

	buf[0] = CMD_FW_MP_PRE_CMD;
	buf[1] = 0x1;

	if (ilits->wrapper(buf, 2, NULL, 0, OFF, OFF) < 0) {
		ILI_ERR("Failed precommand cmd\n");
		return -EMP_CMD;
	}

	mdelay(1);

	if (ilits->wrapper(cmd, cmdlen, NULL, 0, OFF, OFF) < 0) {
		ILI_ERR("Failed to get cdc data\n");
		return -EMP_GET_CDC;
	}

	mdelay(1);

	if (ilits->wrapper(precmd, precmdlen, rdata, rlen, spi_irq, i2c_irq) < 0) {
		ILI_ERR("Failed to get cdc data\n");
		return -EMP_GET_CDC;
	}

	return 0;

}

static void run_pixel_test(int index)
{
	int i, x, y;
	s32 *p_comb = frame_buf;

	for (y = 0; y < core_mp.ych_len; y++) {
		for (x = 0; x < core_mp.xch_len; x++) {
			int tmp[4] = { 0 }, max = 0;
			int shift = y * core_mp.xch_len;
			int centre = p_comb[shift + x];

			/*
			 * if its position is in corner, the number of point
			 * we have to minus is around 2 to 3.
			 */
			if (y == 0 && x == 0) {
				tmp[0] = Mathabs(centre - p_comb[(shift + 1) + x]);	/* down */
				tmp[1] = Mathabs(centre - p_comb[shift + (x + 1)]);	/* right */
			} else if (y == (core_mp.ych_len - 1) && x == 0) {
				tmp[0] = Mathabs(centre - p_comb[(shift - 1) + x]);	/* up */
				tmp[1] = Mathabs(centre - p_comb[shift + (x + 1)]);	/* right */
			} else if (y == 0 && x == (core_mp.xch_len - 1)) {
				tmp[0] = Mathabs(centre - p_comb[(shift + 1) + x]);	/* down */
				tmp[1] = Mathabs(centre - p_comb[shift + (x - 1)]);	/* left */
			} else if (y == (core_mp.ych_len - 1) && x == (core_mp.xch_len - 1)) {
				tmp[0] = Mathabs(centre - p_comb[(shift - 1) + x]);	/* up */
				tmp[1] = Mathabs(centre - p_comb[shift + (x - 1)]);	/* left */
			} else if (y == 0 && x != 0) {
				tmp[0] = Mathabs(centre - p_comb[(shift + 1) + x]);	/* down */
				tmp[1] = Mathabs(centre - p_comb[shift + (x - 1)]);	/* left */
				tmp[2] = Mathabs(centre - p_comb[shift + (x + 1)]);	/* right */
			} else if (y != 0 && x == 0) {
				tmp[0] = Mathabs(centre - p_comb[(shift - 1) + x]);	/* up */
				tmp[1] = Mathabs(centre - p_comb[shift + (x + 1)]);	/* right */
				tmp[2] = Mathabs(centre - p_comb[(shift + 1) + x]);	/* down */

			} else if (y == (core_mp.ych_len - 1) && x != 0) {
				tmp[0] = Mathabs(centre - p_comb[(shift - 1) + x]);	/* up */
				tmp[1] = Mathabs(centre - p_comb[shift + (x - 1)]);	/* left */
				tmp[2] = Mathabs(centre - p_comb[shift + (x + 1)]);	/* right */
			} else if (y != 0 && x == (core_mp.xch_len - 1)) {
				tmp[0] = Mathabs(centre - p_comb[(shift - 1) + x]);	/* up */
				tmp[1] = Mathabs(centre - p_comb[shift + (x - 1)]);	/* left */
				tmp[2] = Mathabs(centre - p_comb[(shift + 1) + x]);	/* down */
			} else {
				/* middle minus four directions */
				tmp[0] = Mathabs(centre - p_comb[(shift - 1) + x]);	/* up */
				tmp[1] = Mathabs(centre - p_comb[(shift + 1) + x]);	/* down */
				tmp[2] = Mathabs(centre - p_comb[shift + (x - 1)]);	/* left */
				tmp[3] = Mathabs(centre - p_comb[shift + (x + 1)]);	/* right */
			}

			max = tmp[0];

			for (i = 0; i < 4; i++) {
				if (tmp[i] > max)
					max = tmp[i];
			}

			tItems[index].buf[shift + x] = max;
		}
	}
}

static void run_untouch_p2p_test(int index)
{
	int x, y;
	s32 *p_comb = frame_buf;

	for (y = 0; y < core_mp.ych_len; y++) {
		for (x = 0; x < core_mp.xch_len; x++) {
			int shift = y * core_mp.xch_len;

			if (p_comb[shift + x] > tItems[index].max_buf[shift + x])
				tItems[index].max_buf[shift + x] = p_comb[shift + x];

			if (p_comb[shift + x] < tItems[index].min_buf[shift + x])
				tItems[index].min_buf[shift + x] = p_comb[shift + x];

			tItems[index].buf[shift + x] =
				tItems[index].max_buf[shift + x] - tItems[index].min_buf[shift + x];
		}
	}
}

static int run_open_test(int index)
{
	int i, x, y, k, ret = 0;
	int border_x[] = {-1, 0, 1, 1, 1, 0, -1, -1};
	int border_y[] = {-1, -1, -1, 0, 1, 1, 1, 0};
	s32 *p_comb = frame_buf;

	if (ipio_strcmp(tItems[index].desp, "open test(integration)") == 0) {
		for (i = 0; i < core_mp.frame_len; i++)
			tItems[index].buf[i] = p_comb[i];
	} else if (ipio_strcmp(tItems[index].desp, "open test(cap)") == 0) {
		/*
		 * Each result is getting from a 3 by 3 grid depending on where the centre location is.
		 * So if the centre is at corner, the number of node grabbed from a grid will be different.
		 */
		for (y = 0; y < core_mp.ych_len; y++) {
			for (x = 0; x < core_mp.xch_len; x++) {
				int sum = 0, avg = 0, count = 0;
				int shift = y * core_mp.xch_len;
				int centre = p_comb[shift + x];

				for (k = 0; k < 8; k++) {
					if (((y + border_y[k] >= 0) && (y + border_y[k] < core_mp.ych_len)) &&
								((x + border_x[k] >= 0) && (x + border_x[k] < core_mp.xch_len))) {
						count++;
						sum += p_comb[(y + border_y[k]) * core_mp.xch_len + (x + border_x[k])];
					}
				}

				avg = (sum + centre) / (count + 1);	/* plus 1 because of centre */
				tItems[index].buf[shift + x] = (centre * 100) / avg;
			}
		}
	}
	return ret;
}

static void run_tx_rx_delta_test(int index)
{
	int x, y;
	s32 *p_comb = frame_buf;

	for (y = 0; y < core_mp.ych_len; y++) {
		for (x = 0; x < core_mp.xch_len; x++) {
			int shift = y * core_mp.xch_len;

			/* Tx Delta */
			if (y != (core_mp.ych_len - 1))
				core_mp.tx_delta_buf[shift + x] = Mathabs(p_comb[shift + x] - p_comb[(shift + 1) + x]);

			/* Rx Delta */
			if (x != (core_mp.xch_len - 1))
				core_mp.rx_delta_buf[shift + x] = Mathabs(p_comb[shift + x] - p_comb[shift + (x + 1)]);
		}
	}
}

static char *get_date_time_str(void)
{
	static char time_data_buf[128] = { 0 };

#if LINUX_VERSION_CODE <= KERNEL_VERSION(4, 14, 0)
	struct timespec now_time;
	struct rtc_time rtc_now_time;

	getnstimeofday(&now_time);
	rtc_time_to_tm(now_time.tv_sec, &rtc_now_time);
	snprintf(time_data_buf, sizeof(time_data_buf), "%04d%02d%02d-%02d%02d%02d",
		(rtc_now_time.tm_year + 1900), rtc_now_time.tm_mon + 1,
		rtc_now_time.tm_mday, rtc_now_time.tm_hour, rtc_now_time.tm_min,
		rtc_now_time.tm_sec);
#else
	struct tm tm;
	time64_to_tm(ktime_get_real_seconds(), 0, &tm);

	snprintf(time_data_buf, sizeof(time_data_buf), "%04d%02d%02d-%02d%02d%02d",
		(int)(tm.tm_year + 1900), tm.tm_mon + 1,
		tm.tm_mday, tm.tm_hour, tm.tm_min,
		tm.tm_sec);
#endif

	return time_data_buf;
}

static void mp_print_csv_header(char *csv, int *csv_len, int *csv_line, int file_size)
{
	int i, seq, tmp_len = *csv_len, tmp_line = *csv_line;

	/* header must has 19 line*/
	tmp_len += snprintf(csv + tmp_len, (file_size - tmp_len), "==============================================================================\n");
	tmp_line++;
	tmp_len += snprintf(csv + tmp_len, (file_size - tmp_len), "ILITek C-TP Utility V%s	%x : Driver Sensor Test\n", DRIVER_VERSION, core_mp.chip_pid);
	tmp_line++;
	tmp_len += snprintf(csv + tmp_len, (file_size - tmp_len), "Confidentiality Notice:\n");
	tmp_line++;
	tmp_len += snprintf(csv + tmp_len, (file_size - tmp_len), "Any information of this tool is confidential and privileged.\n");
	tmp_line++;
	tmp_len += snprintf(csv + tmp_len, (file_size - tmp_len), "@ ILI TECHNOLOGY CORP. All Rights Reserved.\n");
	tmp_line++;
	tmp_len += snprintf(csv + tmp_len, (file_size - tmp_len), "==============================================================================\n");
	tmp_line++;
	tmp_len += snprintf(csv + tmp_len, (file_size - tmp_len), "Firmware Version ,0x%x\n", core_mp.fw_ver);
	tmp_line++;
	tmp_len += snprintf(csv + tmp_len, (file_size - tmp_len), "Panel information ,XCH=%d, YCH=%d\n", core_mp.xch_len, core_mp.ych_len);
	tmp_len += snprintf(csv + tmp_len, (file_size - tmp_len), "INI Release Version ,%s\n", core_mp.ini_ver);
	tmp_line++;
	tmp_len += snprintf(csv + tmp_len, (file_size - tmp_len), "INI Release Date ,%s\n", core_mp.ini_date);
	tmp_line++;
	tmp_line++;
	tmp_len += snprintf(csv + tmp_len, (file_size - tmp_len), "Test Item:\n");
	tmp_line++;

	for (seq = 0; seq < ri.count; seq++) {
		i = ri.index[seq];
		tmp_len += snprintf(csv + tmp_len, (file_size - tmp_len), "	  ---%s\n", tItems[i].desp);
		tmp_line++;
	}

	while (tmp_line < 19) {
		tmp_len += snprintf(csv + tmp_len, (file_size - tmp_len), "\n");
		tmp_line++;
	}

	tmp_len += snprintf(csv + tmp_len, (file_size - tmp_len), "==============================================================================\n");

	*csv_len = tmp_len;
	*csv_line = tmp_line;
}

static void mp_print_csv_tail(char *csv, int *csv_len, int file_size)
{
	int i, seq, tmp_len = *csv_len;

	tmp_len += snprintf(csv + tmp_len, (file_size - tmp_len), "==============================================================================\n");
	tmp_len += snprintf(csv + tmp_len, (file_size - tmp_len), "Result_Summary			\n");

	for (seq = 0; seq < ri.count; seq++) {
		i = ri.index[seq];
		if (tItems[i].item_result == MP_DATA_PASS) {
			if (tItems[i].goldenmode && (tItems[i].spec_option != tItems[i].goldenmode)) {
				tmp_len += snprintf(csv + tmp_len, (file_size - tmp_len), "	  {%s}	   ,OK, Warning, No Golden\n", tItems[i].desp);
			} else {
				tmp_len += snprintf(csv + tmp_len, (file_size - tmp_len), "	  {%s}	   ,OK\n", tItems[i].desp);
			}
		} else {
			if (tItems[i].goldenmode && (tItems[i].spec_option != tItems[i].goldenmode)) {
				tmp_len += snprintf(csv + tmp_len, (file_size - tmp_len), "   {%s}	   ,NG, Warning, No Golden\n", tItems[i].desp);
			} else {
				tmp_len += snprintf(csv + tmp_len, (file_size - tmp_len), "	  {%s}	   ,NG\n", tItems[i].desp);
			}
		}
	}

	*csv_len = tmp_len;
}

static void mp_print_csv_cdc_cmd(char *csv, int *csv_len, int index, int file_size)
{
	int i, slen = 0, tmp_len = *csv_len, size;
	char str[128] = {0};
	char *open_sp_cmd[] = {"open dac", "open raw1", "open raw2", "open raw3"};
	char *open_c_cmd[] = {"open cap1 dac", "open cap1 raw"};
	char *open_x_cmd[] = {"open_x cap1 dac", "open_x cap1 raw"};
	char *name = tItems[index].desp;

	if (ipio_strcmp(name, "open test(integration)_sp") == 0) {
		size = ARRAY_SIZE(open_sp_cmd);
		for (i = 0; i < size; i++) {
			slen = parser_get_int_data("pv5_4 command", open_sp_cmd[i], str, sizeof(str));
			if (slen < 0)
				ILI_ERR("Failed to get CDC command %s from ini\n", open_sp_cmd[i]);
			else
				tmp_len += snprintf(csv + tmp_len, (file_size - tmp_len), "%s = ,%s\n", open_sp_cmd[i], str);
		}
	} else if (ipio_strcmp(name, "open test_c") == 0) {
		size = ARRAY_SIZE(open_c_cmd);
		for (i = 0; i < size; i++) {
			slen = parser_get_int_data("pv5_4 command", open_c_cmd[i], str, sizeof(str));
			if (slen < 0)
				ILI_ERR("Failed to get CDC command %s from ini\n", open_sp_cmd[i]);
			else
				tmp_len += snprintf(csv + tmp_len, (file_size - tmp_len), "%s = ,%s\n", open_c_cmd[i], str);
		}
	} else if (ipio_strcmp(name, "open test_c (openx enable)") == 0) {
		size = ARRAY_SIZE(open_x_cmd);
		for (i = 0; i < size; i++) {
			slen = parser_get_int_data("pv5_4 command", open_x_cmd[i], str, sizeof(str));
			if (slen < 0)
				ILI_ERR("Failed to get CDC command %s from ini\n", open_x_cmd[i]);
			else
				tmp_len += snprintf(csv + tmp_len, (file_size - tmp_len), "%s = ,%s\n", open_x_cmd[i], str);
		}
	} else {
		slen = parser_get_int_data("pv5_4 command", name, str, sizeof(str));
		if (slen < 0)
			ILI_ERR("Failed to get CDC command %s from ini\n", name);
		else
			tmp_len += snprintf(csv + tmp_len, (file_size - tmp_len), "CDC command = ,%s\n", str);

		/* Print short parameters */
		if (ipio_strcmp(name, "short test") == 0)
			tmp_len += snprintf(csv + tmp_len, (file_size - tmp_len), "Variation = ,%d\n", short_para.variation);

		/* Print td second command */
		if (core_mp.td_retry && (ipio_strcmp(name, "peak to peak_td (lcm off)") == 0)) {
			name = "peak to peak_td (lcm off)_2";
			parser_get_int_data("pv5_4 command", name, str, sizeof(str));
			tmp_len += snprintf(csv + tmp_len, (file_size - tmp_len), "CDC command 2 = ,%s\n", str);
		}
	}
	*csv_len = tmp_len;
}

static void mp_compare_cdc_show_result(int index, s32 *tmp, char *csv,
				int *csv_len, int type, s32 *max_ts,
				s32 *min_ts, const char *desp, int file_zise)
{
	int x, y, tmp_len = *csv_len;
	int mp_result = MP_DATA_PASS;

	if (ERR_ALLOC_MEM(tmp)) {
		ILI_ERR("The data of test item is null (%p)\n", tmp);
		mp_result = -EMP_INVAL;
		goto out;
	}

	/* print X raw only */
	for (x = 0; x < core_mp.xch_len; x++) {
		if (x == 0) {
			DUMP("\n %s ", desp);
			tmp_len += snprintf(csv + tmp_len, (file_zise - tmp_len), "\n	   %s ,", desp);
		}
		DUMP("  X_%d	,", (x+1));
		tmp_len += snprintf(csv + tmp_len, (file_zise - tmp_len), "	 X_%d  ,", (x+1));
	}

	DUMP("\n");
	tmp_len += snprintf(csv + tmp_len, (file_zise - tmp_len), "\n");

	for (y = 0; y < core_mp.ych_len; y++) {
		DUMP("  Y_%d	,", (y+1));
		tmp_len += snprintf(csv + tmp_len, (file_zise - tmp_len), "	 Y_%d  ,", (y+1));

		for (x = 0; x < core_mp.xch_len; x++) {
			int shift = y * core_mp.xch_len + x;

			/* In Short teset, we only identify if its value is low than min threshold. */
			if (tItems[index].catalog == SHORT_TEST) {
				if (tmp[shift] < min_ts[shift]) {
					DUMP(" #%7d ", tmp[shift]);
					tmp_len += snprintf(csv + tmp_len, (file_zise - tmp_len), "#%7d,", tmp[shift]);
					mp_result = MP_DATA_FAIL;
				} else {
					DUMP(" %7d ", tmp[shift]);
					tmp_len += snprintf(csv + tmp_len, (file_zise - tmp_len), " %7d, ", tmp[shift]);
				}
				continue;
			}

			if ((tmp[shift] <= max_ts[shift] && tmp[shift] >= min_ts[shift]) || (type != TYPE_JUGE)) {
				if ((tmp[shift] == INT_MAX || tmp[shift] == INT_MIN) && (type == TYPE_BENCHMARK)) {
					DUMP("%s", "BYPASS,");
					tmp_len += snprintf(csv + tmp_len, (file_zise - tmp_len), "BYPASS,");
				} else {
					DUMP(" %7d ", tmp[shift]);
					tmp_len += snprintf(csv + tmp_len, (file_zise - tmp_len), " %7d, ", tmp[shift]);
				}
			} else {
				if (tmp[shift] > max_ts[shift]) {
					DUMP(" *%7d ", tmp[shift]);
					tmp_len += snprintf(csv + tmp_len, (file_zise - tmp_len), "*%7d,", tmp[shift]);
				} else {
					DUMP(" #%7d ", tmp[shift]);
					tmp_len += snprintf(csv + tmp_len, (file_zise - tmp_len), "#%7d,", tmp[shift]);
				}
				mp_result = MP_DATA_FAIL;
			}
		}
		DUMP("\n");
		tmp_len += snprintf(csv + tmp_len, (file_zise - tmp_len), "\n");
	}

out:
	if (type == TYPE_JUGE) {
		if (mp_result == MP_DATA_PASS) {
			pr_info("\n Result : PASS\n");
			tmp_len += snprintf(csv + tmp_len, (file_zise - tmp_len), "Result : PASS\n");
		} else {
			pr_info("\n Result : FAIL\n");
			tmp_len += snprintf(csv + tmp_len, (file_zise - tmp_len), "Result : FAIL\n");
		}
	}
	*csv_len = tmp_len;
}

static void mp_compare_tip_ring_cdc_show_result(int index, s32 *tmp, char *csv,
				int *csv_len, int type, s32 *max_ts, s32 *min_ts, const char *desp,
				int file_zise, int trtype)
{
	int x, y, tmp_len = *csv_len;
	int row, col, typeshift;
	int mp_result = MP_DATA_PASS;

	if (ERR_ALLOC_MEM(tmp)) {
		ILI_ERR("The data of test item is null (%p)\n", tmp);
		mp_result = -EMP_INVAL;
		goto out;
	}
	if (trtype == TIPRING_X) {
		row = core_mp.xch_len;
		col = core_mp.nPxRaw * 2;
		typeshift = 0;
	} else {
		row = core_mp.nPyRaw * 2;
		col = core_mp.ych_len;
		typeshift = core_mp.nPxRaw_tipring_len;
	}

	/* print X row only */
	for (x = 0; x < row; x++) {
		if (x == 0) {
			DUMP("\n %s ", desp);
			tmp_len += snprintf(csv + tmp_len, (file_zise - tmp_len), "\n	   %s ,", desp);
		}
		DUMP("  X_%d	,", (x+1));
		tmp_len += snprintf(csv + tmp_len, (file_zise - tmp_len), "	 X_%d  ,", (x+1));
	}

	DUMP("\n");
	tmp_len += snprintf(csv + tmp_len, (file_zise - tmp_len), "\n");
	for (y = 0; y < col; y++) {
		DUMP("  Y_%d	,", (y+1));
		tmp_len += snprintf(csv + tmp_len, (file_zise - tmp_len), "	 Y_%d  ,", (y+1));

		for (x = 0; x < row; x++) {
			int shift = typeshift + y * row + x;
			if ((tmp[shift] <= max_ts[shift] && tmp[shift] >= min_ts[shift]) || (type != TYPE_JUGE)) {
				if ((tmp[shift] == INT_MAX || tmp[shift] == INT_MIN) && (type == TYPE_BENCHMARK)) {
					DUMP("%s", "BYPASS,");
					tmp_len += snprintf(csv + tmp_len, (file_zise - tmp_len), "BYPASS,");
				} else {
					DUMP(" %7d ", tmp[shift]);
					tmp_len += snprintf(csv + tmp_len, (file_zise - tmp_len), " %7d, ", tmp[shift]);
				}
			} else {
				if (tmp[shift] > max_ts[shift]) {
					DUMP(" *%7d ", tmp[shift]);
					tmp_len += snprintf(csv + tmp_len, (file_zise - tmp_len), "*%7d,", tmp[shift]);
				} else {
					DUMP(" #%7d ", tmp[shift]);
					tmp_len += snprintf(csv + tmp_len, (file_zise - tmp_len), "#%7d,", tmp[shift]);
				}
				mp_result = MP_DATA_FAIL;
			}
		}
		DUMP("\n");
		tmp_len += snprintf(csv + tmp_len, (file_zise - tmp_len), "\n");
	}

out:
	if (type == TYPE_JUGE) {
		if (mp_result == MP_DATA_PASS) {
			pr_info("\n Result : PASS\n");
			tmp_len += snprintf(csv + tmp_len, (file_zise - tmp_len), "Result : PASS\n");
		} else {
			pr_info("\n Result : FAIL\n");
			tmp_len += snprintf(csv + tmp_len, (file_zise - tmp_len), "Result : FAIL\n");
		}
	}
	*csv_len = tmp_len;
}

#define ABS(a, b) ((a > b) ? (a - b) : (b - a))
#define ADDR(x, y) ((y * core_mp.xch_len) + (x))

static int compare_charge(s32 *charge_rate, int x, int y, s32 *inNodeType,
		int Charge_AA, int Charge_Border, int Charge_Notch)
{
	int OpenThreadhold, tempY, tempX, ret, k;
	int sx[8] = {-1, 0, 1, -1, 1, -1, 0, 1};
	int sy[8] = {-1, -1, -1, 0, 0, 1, 1, 1};

	ret = charge_rate[ADDR(x, y)];

	/*Setting Threadhold from node type	 */
	if (charge_rate[ADDR(x, y)] == 0)
		return ret;
	else if ((inNodeType[ADDR(x, y)] & AA_Area) == AA_Area)
		OpenThreadhold = Charge_AA;
	else if ((inNodeType[ADDR(x, y)] & Border_Area) == Border_Area)
		OpenThreadhold = Charge_Border;
	else if ((inNodeType[ADDR(x, y)] & Notch) == Notch)
		OpenThreadhold = Charge_Notch;
	else
		return ret;

	/* compare carge rate with 3*3 node */
	/* by pass => 1.no compare 2.corner 3.Skip_Micro 4.full open fail node */
	for (k = 0; k < 8; k++) {
		tempX = x + sx[k];
		tempY = y + sy[k];

		/*out of range */
		if ((tempX < 0) || (tempX >= core_mp.xch_len) || (tempY < 0) || (tempY >= core_mp.ych_len))
			continue;

		if ((inNodeType[ADDR(tempX, tempY)] == NO_COMPARE) || ((inNodeType[ADDR(tempX, tempY)] & Round_Corner) == Round_Corner) ||
		((inNodeType[ADDR(tempX, tempY)] & Skip_Micro) == Skip_Micro) || charge_rate[ADDR(tempX, tempY)] == 0)
			continue;

		if ((charge_rate[ADDR(tempX, tempY)] - charge_rate[ADDR(x, y)]) > OpenThreadhold)
			return OpenThreadhold;
	}
	return ret;
}

static int full_open_rate_compare(s32 *full_open, s32 *cbk, int x, int y, s32 inNodeType, int full_open_rate)
{
	int ret = true;

	if ((inNodeType == NO_COMPARE) || ((inNodeType & Round_Corner) == Round_Corner))
		return true;

	if (full_open[ADDR(x, y)] < (cbk[ADDR(x, y)] * full_open_rate / 100))
		ret = false;

	return ret;
}

static s32 open_sp_formula(int dac, int raw, int tvch, int tvcl)
{
	s32 ret = 0;
	u16 id = core_mp.chip_id;

	if (id == ILI9881_CHIP) {
		ret = ((dac * 10000 * 161 / 100) - (16384 / 2 - raw) * 20000 * 7 / 16384 * 36 / 10) / (tvch - tvcl) / 2;
	} else {
		ret = ((dac * 10000 * 131 / 100) - (16384 / 2 - raw) * 20000 * 7 / 16384 * 36 / 10) / (tvch - tvcl) / 2;
	}
	return ret;
}

static s32 open_c_formula(int inCap1DAC, int inCap1Raw, int accuracy)
{
	s32 inCap1Value = 0;

	u16 id = core_mp.chip_id;
	u8 type = core_mp.chip_type;

	int inFoutRange = 16384;
	int inFoutRange_half = 8192;
	int inVadc_range = open_para.vadc_range;
	int inVbk = open_para.invbk;/* from mp.ini */
	int inCbk_step = open_para.cbk_step;/* from mp.ini */
	int inCint = open_para.cint;/* from mp.ini */
	int inVdrv = open_para.tvch - open_para.tvcl;/* from mp.ini */
	int inGain = open_para.gain;/*  from mp.ini */
	int inMagnification = 10;
	int inPartDac = 0;
	int inPartRaw = 0;

	if (inVbk == 0) {
		if ((id == ILI7807_CHIP && ((type == ILI_S) || (type == ILI_V)))
				|| (id == ILI9882_CHIP && type == ILI_T))
			inVbk = 42;
		else
			inVbk = 39;
	}

	if (inVadc_range == 0)
		inVadc_range = 36;

	if ((inCbk_step == 0) || (inCint == 0)) {
		if (id == ILI9881_CHIP) {
			if ((type == ILI_N) || (type == ILI_O)) {
				inCbk_step = 32;
				inCint = 70;
			}
		} else if (id == ILI9882_CHIP) {
			if (type == ILI_N) {
				inCbk_step = 42;
				inCint = 70;
			} else if (type == ILI_H) {
				inCbk_step = 42;
				inCint = 69;
			}
		} else if (id == ILI7807_CHIP) {
			if (type == ILI_Q) {
				inCbk_step = 28;
				inCint = 70;
			} else if (type == ILI_S) {
				inCbk_step = 38;
				inCint = 66;
				inVdrv = inVdrv / 10;
				inGain = inGain / 10;
			} else if (type == ILI_V) {
				inCbk_step = 42;
				inCint = 69;
			}
		} else if (id == ILI9883_CHIP) {
			if (type == ILI_A) {
				inCbk_step = 42;
				inCint = 70;
			}
		}
	}

	inPartDac = (inCap1DAC * inCbk_step * inVbk / 2);
	inPartRaw = ((inCap1Raw - inFoutRange_half) * inVadc_range * inCint * 10 / inFoutRange);

	if (accuracy) {
		inCap1Value = ((inPartDac + inPartRaw) * 10) / inVdrv / inMagnification / inGain;
	} else {
		inCap1Value = (inPartDac + inPartRaw) / inVdrv / inMagnification / inGain;
	}

	return inCap1Value;
}

static void allnode_open_cdc_result(int index, int *buf, int *dac, int *raw)
{
	int i;
	char *desp = tItems[index].desp;
	char str[32] = {0};
	int accuracy = 0;

	if (ipio_strcmp(desp, "open test(integration)_sp") == 0) {
		for (i = 0; i < core_mp.frame_len; i++)
			buf[i] = open_sp_formula(dac[i], raw[i], open_para.tvch, open_para.tvcl);
	} else if (ipio_strcmp(desp, "open test_c") == 0 || ipio_strcmp(desp, "open test_c (openx enable)") == 0) {

		if (parser_get_int_data(desp, "accuracy", str, sizeof(str)) > 0)
			accuracy = ili_katoi(str);

		for (i = 0; i < core_mp.frame_len; i++)
			buf[i] = open_c_formula(dac[i], raw[i], accuracy);
	}
}

static int codeToOhm(s32 *ohm, s32 *Code, u16 *v_tdf, u16 *h_tdf)
{
	u16 id = core_mp.chip_id;
	u8 type = core_mp.chip_type;

	int inTVCH = short_para.tvch;
	int inTVCL = short_para.tvcl;
	int inVariation = short_para.variation;
	int inTDF1 = 0;
	int inTDF2 = 0;
	int inCint = short_para.cint;
	int inRinternal = short_para.rinternal;
	s32 temp = 0;
	int j = 0;

	if (core_mp.isLongV) {
		inTDF1 = *v_tdf;
		inTDF2 = *(v_tdf + 1);
	} else {
		inTDF1 = *h_tdf;
		inTDF2 = *(h_tdf + 1);
	}

	if (inVariation == 0) {
		inVariation = 100;
	}

	if ((inCint == 0) ||  (inRinternal == 0)) {
		if (id == ILI9881_CHIP) {
			if ((type == ILI_N) || (type == ILI_O)) {
				inRinternal = 1915;
				inCint = 70;
			}
		} else if (id == ILI9882_CHIP) {
			if (type == ILI_N) {
				inRinternal = 2000;
				inCint = 70;
			} else if (type == ILI_H) {
				inRinternal = 2000;
				inCint = 69;
			}
		} else if (id == ILI7807_CHIP) {
			if (type == ILI_Q) {
				inRinternal = 1500;
				inCint = 70;
			} else if (type == ILI_S) {
				inRinternal = 1500;
				inCint = 66;
				inTVCH = inTVCH/10;
				inTVCL = inTVCL/10;
			} else if (type == ILI_V) {
				inRinternal = 2000;
				inCint = 69;
			}
		}  else if (id == ILI9883_CHIP) {
			if (type == ILI_A) {
				inRinternal = 2000;
				inCint = 70;
			}
		}
	}
	for (j = 0; j < core_mp.frame_len; j++) {
		if (Code[j] == 0) {
			ILI_ERR("code is invalid\n");
		} else {
			temp = ((inTVCH - inTVCL) * inVariation * (inTDF1 - inTDF2) * (1 << 12) / (9 * Code[j] * inCint)) * 1000;
			temp = (temp - inRinternal) / 1000;
		}
		ohm[j] = temp;
	}

	/* Unit = M Ohm */
	return temp;

}

static int short_test(int index, int frame_index)
{
	u32 pid = core_mp.chip_pid >> 8;
	int j = 0, ret = 0;
	u16 v_tdf[2] = {0};
	u16 h_tdf[2] = {0};
	char str[32] = {0};

	v_tdf[0] = tItems[index].v_tdf_1;
	v_tdf[1] = tItems[index].v_tdf_2;
	h_tdf[0] = tItems[index].h_tdf_1;
	h_tdf[1] = tItems[index].h_tdf_2;

	parser_get_int_data("short test", "tvch", str, sizeof(str));
	short_para.tvch = ili_katoi(str);

	parser_get_int_data("short test", "tvcl", str, sizeof(str));
	short_para.tvcl = ili_katoi(str);

	parser_get_int_data("short test", "variation", str, sizeof(str));
	short_para.variation = ili_katoi(str);

	parser_get_int_data("short test", "cint", str, sizeof(str));
	short_para.cint = ili_katoi(str);

	parser_get_int_data("short test", "rinternal", str, sizeof(str));
	short_para.rinternal = ili_katoi(str);

	/* 9881N, 9881O, 7807Q, 7807S not support cbk_step and cint read from mp.ini */
	if ((pid != 0x988117) && (pid != 0x988118) && (pid != 0x78071A) && (pid != 0x78071C)) {
		if ((short_para.cint == 0) || (short_para.rinternal == 0)) {
			ILI_ERR("Failed to get short parameter");
			core_mp.lost_parameter = true;
			return -1;
		}
	}

	ILI_INFO("TVCH = %d, TVCL = %d, Variation = %d, V_TDF1 = %d, V_TDF2 = %d, H_TDF1 = %d, H_TDF2 = %d, Cint = %d, Rinternal = %d\n"
		, short_para.tvch, short_para.tvcl, short_para.variation, tItems[index].v_tdf_1, tItems[index].v_tdf_2, tItems[index].h_tdf_1, tItems[index].h_tdf_2, short_para.cint, short_para.rinternal);

	if (core_mp.protocol_ver >= PROTOCOL_VER_540) {
		/* Calculate code to ohm and save to tItems[index].buf */
		ili_dump_data(frame_buf, 10, core_mp.frame_len, core_mp.xch_len, "Short Raw 1");
		codeToOhm(&tItems[index].buf[frame_index * core_mp.frame_len], frame_buf, v_tdf, h_tdf);
		ili_dump_data(&tItems[index].buf[frame_index * core_mp.frame_len], 10, core_mp.frame_len, core_mp.xch_len, "Short Ohm 1");
	} else {
		for (j = 0; j < core_mp.frame_len; j++)
			tItems[index].buf[frame_index * core_mp.frame_len + j] = frame_buf[j];
	}

	return ret;
}

static int allnode_key_cdc_data(int index)
{
	int i, ret = 0, len = 0;
	int inDACp = 0, inDACn = 0;
	u8 cmd[3] = {0};
	u8 *ori = NULL;

	len = core_mp.key_len * 2;

	ILI_DBG("Read key's length = %d\n", len);
	ILI_DBG("core_mp.key_len = %d\n", core_mp.key_len);

	if (len <= 0) {
		ILI_ERR("Length is invalid\n");
		ret = -1;
		goto out;
	}

	/* CDC init */
	cmd[0] = P5_X_SET_CDC_INIT;
	cmd[1] = tItems[index].cmd;
	cmd[2] = 0;

	/* Allocate a buffer for the original */
	ori = kcalloc(len, sizeof(u8), GFP_KERNEL);
	if (ERR_ALLOC_MEM(ori)) {
		ILI_ERR("Failed to allocate ori mem (%ld)\n", PTR_ERR(ori));
		ret = -1;
		goto out;
	}
	ret = ilits->wrapper(cmd, sizeof(cmd), ori, len, ON, ON);
	if (ret < 0) {
		ILI_ERR(" fail to get cdc data\n");
		goto out;
	}
	ili_dump_data(ori, 8, len, 0, "Key CDC original");

	if (key_buf == NULL) {
		key_buf = kcalloc(core_mp.key_len, sizeof(s32), GFP_KERNEL);
		if (ERR_ALLOC_MEM(key_buf)) {
			ILI_ERR("Failed to allocate FrameBuffer mem (%ld)\n", PTR_ERR(key_buf));
			goto out;
		}
	} else {
		memset(key_buf, 0x0, core_mp.key_len);
	}

	/* Convert original data to the physical one in each node */
	for (i = 0; i < core_mp.frame_len; i++) {
		if (tItems[index].cmd == CMD_KEY_DAC) {
			/* DAC - P */
			if (((ori[(2 * i) + 1] & 0x80) >> 7) == 1) {
				/* Negative */
				inDACp = 0 - (int)(ori[(2 * i) + 1] & 0x7F);
			} else {
				inDACp = ori[(2 * i) + 1] & 0x7F;
			}

			/* DAC - N */
			if (((ori[(1 + (2 * i)) + 1] & 0x80) >> 7) == 1) {
				/* Negative */
				inDACn = 0 - (int)(ori[(1 + (2 * i)) + 1] & 0x7F);
			} else {
				inDACn = ori[(1 + (2 * i)) + 1] & 0x7F;
			}

			key_buf[i] = (inDACp + inDACn) / 2;
		}
	}
	ili_dump_data(key_buf, 32, core_mp.frame_len, core_mp.xch_len, "Key CDC combined data");

out:
	ipio_kfree((void **)&ori);
	return ret;
}

static int mp_cdc_get_pv5_4_command(u8 *cmd, int len, int index)
{
	int slen = 0;
	char str[128] = {0};
	char *key = tItems[index].desp;

	if (core_mp.td_retry && (ipio_strcmp(key, "peak to peak_td (lcm off)") == 0))
		key = "peak to peak_td (lcm off)_2";

	slen = parser_get_int_data("pv5_4 command", key, str, sizeof(str));
	if (slen < 0)
		return -1;

	if (parser_get_u8_array(str, cmd, 16, len) < 0)
		return -1;

	return 0;
}

static int mp_cdc_init_cmd_common(u8 *cmd, int len, int index)
{
	int ret = 0;

	if (core_mp.protocol_ver >= PROTOCOL_VER_540) {
		core_mp.cdc_len = 15;
		return mp_cdc_get_pv5_4_command(cmd, len, index);
	}

	cmd[0] = P5_X_SET_CDC_INIT;
	cmd[1] = tItems[index].cmd;
	cmd[2] = 0;

	core_mp.cdc_len = 3;

	if (ipio_strcmp(tItems[index].desp, "open test(integration)") == 0)
		cmd[2] = 0x2;
	if (ipio_strcmp(tItems[index].desp, "open test(cap)") == 0)
		cmd[2] = 0x3;

	if (tItems[index].catalog == PEAK_TO_PEAK_TEST) {
		cmd[2] = ((tItems[index].frame_count & 0xff00) >> 8);
		cmd[3] = tItems[index].frame_count & 0xff;
		cmd[4] = 0;

		core_mp.cdc_len = 5;

		if (ipio_strcmp(tItems[index].desp, "noise peak to peak(cut panel)") == 0)
			cmd[4] = 0x1;

		ILI_DBG("P2P CMD: %d,%d,%d,%d,%d\n",
				cmd[0], cmd[1], cmd[2], cmd[3], cmd[4]);
	}

	return ret;
}

static int allnode_open_cdc_data(int mode, int *buf, int index)
{
	int i = 0, ret = 0, len = 0, precmd_len = 0;
	int inDACp = 0, inDACn = 0;
	u8 cmd[15] = {0};
	u8 precmd[256] = {0};
	u8 *ori = NULL;
	char str[128] = {0};
	char *key[] = {"open dac", "open raw1", "open raw2", "open raw3",
			"open cap1 dac", "open cap1 raw", "open_x cap1 dac", "open_x cap1 raw"};

	/* Multipling by 2 is due to the 16 bit in each node */
	len = (core_mp.xch_len * core_mp.ych_len * 2) + 2;

	ILI_DBG("Read X/Y Channel length = %d, mode = %d\n", len, mode);

	if (len <= 2) {
		ILI_ERR("Length is invalid\n");
		ret = -EMP_INVAL;
		goto out;
	}

	/*Pre-Command Init*/
	if (tItems[index].pre_cmd_en) {
		memset(precmd, 0xFF, sizeof(precmd));
		precmd_len = parser_ini_precmd(tItems[index].desp, precmd);

		ili_dump_data(precmd, 16, precmd_len, 0, "PreCommand");

		/*ini file no pre-command, disable precommand*/
		if (precmd_len < 13)
			tItems[index].pre_cmd_en = DISABLE;
	}

	/* CDC init. Read command from ini file */
	if (parser_get_int_data("pv5_4 command", key[mode], str, sizeof(str)) < 0) {
		ILI_ERR("Failed to parse PV54 command, ret = %d\n", ret);
		ret = -EMP_PARSE;
		goto out;
	}

	parser_get_u8_array(str, cmd, 16, sizeof(cmd));

	ili_dump_data(cmd, 8, sizeof(cmd), 0, "Open SP command");

	/* Allocate a buffer for the original */
	ori = kcalloc(len, sizeof(u8), GFP_KERNEL);
	if (ERR_ALLOC_MEM(ori)) {
		ILI_ERR("Failed to allocate ori, (%ld)\n", PTR_ERR(ori));
		ret = -EMP_NOMEM;
		goto out;
	}

	/* Get original frame(cdc) data */
#if (ENGINEER_FLOW)
	if (!ilits->eng_flow) {
		if (tItems[index].pre_cmd_en) {
			if (precommd(cmd, core_mp.cdc_len, precmd, precmd_len, ori, len, ON, ON) < 0)
				goto out;
		} else {
			if (ilits->wrapper(cmd, core_mp.cdc_len, ori, len, ON, ON) < 0) {
				ILI_ERR("Failed to get cdc data\n");
				ret = -EMP_GET_CDC;
				goto out;
			}
		}
	} else {
		msleep(200);
		if (ilits->wrapper(cmd, core_mp.cdc_len, ori, len, OFF, OFF) < 0) {
			ILI_ERR("Failed to get cdc data\n");
		}
	}
#else
	if (tItems[index].pre_cmd_en) {
		if (precommd(cmd, core_mp.cdc_len, precmd, precmd_len, ori, len, ON, ON) < 0)
			goto out;
	} else {
		if (ilits->wrapper(cmd, core_mp.cdc_len, ori, len, ON, ON) < 0) {
			ILI_ERR("Failed to get cdc data\n");
			ret = -EMP_GET_CDC;
			goto out;
		}
	}
#endif

	ili_dump_data(ori, 8, len, 0, "Open SP CDC original");

	/* Convert original data to the physical one in each node */
	for (i = 0; i < core_mp.frame_len; i++) {
		if ((mode == 0) || (mode == 4) || (mode == 6)) {
			/* DAC - P */
			if (((ori[(2 * i) + 1] & 0x80) >> 7) == 1) {
				/* Negative */
				inDACp = 0 - (int)(ori[(2 * i) + 1] & 0x7F);
			} else {
				inDACp = ori[(2 * i) + 1] & 0x7F;
			}

			/* DAC - N */
			if (((ori[(1 + (2 * i)) + 1] & 0x80) >> 7) == 1) {
				/* Negative */
				inDACn = 0 - (int)(ori[(1 + (2 * i)) + 1] & 0x7F);
			} else {
				inDACn = ori[(1 + (2 * i)) + 1] & 0x7F;
			}

			buf[i] = inDACp + inDACn;
		} else {
			/* H byte + L byte */
			s32 tmp = (ori[(2 * i) + 1] << 8) + ori[(1 + (2 * i)) + 1];
			if ((tmp & 0x8000) == 0x8000)
				buf[i] = tmp - 65536;
			else
				buf[i] = tmp;

		}
	}
	ili_dump_data(buf, 10, core_mp.frame_len,  core_mp.xch_len, "Open SP CDC combined");
out:

	ipio_kfree((void **)&ori);
	return ret;
}

static int allnode_peak_to_peak_cdc_data(int index)
{
	int i, k, ret = 0, len = 0, rd_frame_num = 1, cmd_len = 0, precmd_len = 0;
	u8 cmd[15] = {0};
	u8 precmd[256] = {0};
	u8 *ori = NULL;

	/* Multipling by 2 is due to the 16 bit in each node */
	if (tItems[index].catalog == TIPRING_P2P)
		len = (core_mp.tipring_len * 2) + 2;
	else
		len = (core_mp.frame_len * 2) + 2;

	ILI_DBG("Read X/Y Channel length = %d\n", len);

	if (len <= 2) {
		ILI_ERR("Length is invalid\n");
		ret = -EMP_INVAL;
		goto out;
	}

	memset(cmd, 0xFF, sizeof(cmd));

	/* Allocate a buffer for the original */
	ori = kcalloc(len, sizeof(u8), GFP_KERNEL);
	if (ERR_ALLOC_MEM(ori)) {
		ILI_ERR("Failed to allocate ori, (%ld)\n", PTR_ERR(ori));
		ret = -EMP_NOMEM;
		goto out;
	}

	if (tItems[index].bch_mrk_multi) {
		rd_frame_num = tItems[index].bch_mrk_frm_num - 1;
	}

	if (frm_buf == NULL) {
		frm_buf = (s32 **)kzalloc(tItems[index].bch_mrk_frm_num * sizeof(s32 *), GFP_KERNEL);
		if (ERR_ALLOC_MEM(frm_buf)) {
			ILI_ERR("Failed to allocate frm_buf mem (%ld)\n", PTR_ERR(frm_buf));
			ret = -EMP_NOMEM;
			goto out;
		}
		for (i = 0; i < tItems[index].bch_mrk_frm_num; i++) {
			frm_buf[i] = (s32 *)kzalloc(core_mp.frame_len * sizeof(s32), GFP_KERNEL);
			if (ERR_ALLOC_MEM(frm_buf)) {
				ILI_ERR("Failed to allocate frm_buf[%d] mem (%ld)\n", i, PTR_ERR(frm_buf));
				ret = -EMP_NOMEM;
				goto out;
			}
		}
	}

	/*Pre-Command Init*/
	if (tItems[index].pre_cmd_en) {
		memset(precmd, 0xFF, sizeof(precmd));
		precmd_len = parser_ini_precmd(tItems[index].desp, precmd);

		ili_dump_data(precmd, 16, precmd_len, 0, "PreCommand");

		/*ini file no pre-command, disable precommand*/
		if (precmd_len < 13)
			tItems[index].pre_cmd_en = DISABLE;
	}

	/* CDC init */
	if (mp_cdc_init_cmd_common(cmd, sizeof(cmd), index) < 0) {
		ILI_ERR("Failed to get cdc command\n");
		ret = -EMP_CMD;
		goto out;
	}

	ili_dump_data(cmd, 8, core_mp.cdc_len, 0, "Mutual CDC command");

	for (k = 0; k < rd_frame_num; k++) {
		if (k == 0)
			cmd_len = core_mp.cdc_len;
		else
			cmd_len = 0;
		memset(ori, 0, len);
		/* Get original frame(cdc) data */
#if (ENGINEER_FLOW)
		if (!ilits->eng_flow) {
			if (tItems[index].pre_cmd_en) {
				if (precommd(cmd, cmd_len, precmd, precmd_len, ori, len, ON, ON) < 0) {
					goto out;
				}
			} else {
				if (ilits->wrapper(cmd, cmd_len, ori, len, ON, ON) < 0) {
					ILI_ERR("Failed to get cdc data\n");
					ret = -EMP_GET_CDC;
					goto out;
				}
			}
		} else {
			msleep(200);
			if (ilits->wrapper(cmd, cmd_len, ori, len, OFF, OFF) < 0) {
				ILI_ERR("Failed to get cdc data\n");
			}
		}
#else
		if (tItems[index].pre_cmd_en) {
			if (precommd(cmd, cmd_len, precmd, precmd_len, ori, len, ON, ON) < 0) {
				goto out;
			}
		} else {
			if (ilits->wrapper(cmd, cmd_len, ori, len, ON, ON) < 0) {
				ILI_ERR("Failed to get cdc data\n");
				ret = -EMP_GET_CDC;
				goto out;
			}
		}
#endif

		ili_dump_data(ori, 8, len, 0, "Mutual CDC original");

		/* Convert original data to the physical one in each node */
		for (i = 0; i < (len / 2 - 1); i++) {
			/* H byte + L byte */
			s32 tmp = (ori[(2 * i) + 1] << 8) + ori[(1 + (2 * i)) + 1];

			if ((tmp & 0x8000) == 0x8000)
				frm_buf[k][i] = tmp - 65536;
			else
				frm_buf[k][i] = tmp;

			/* multiple case frame3 = frame1 - frame2 */
			if (tItems[index].bch_mrk_multi && rd_frame_num == k + 1)
				frm_buf[k+1][i] = frm_buf[k-1][i] - frm_buf[k][i];
		}
	}

	if (tItems[index].bch_mrk_multi) {
		ili_dump_data(frm_buf[0], 32, (len / 2 - 1), core_mp.xch_len, "Mutual CDC combined[0]/frame1");
		ili_dump_data(frm_buf[1], 32, (len / 2 - 1), core_mp.xch_len, "Mutual CDC combined[1]/frame2");
		ili_dump_data(frm_buf[2], 32, (len / 2 - 1), core_mp.xch_len, "Mutual CDC combined[2]/frame3");
	} else {
		ili_dump_data(frm_buf[0], 32, (len / 2 - 1), core_mp.xch_len, "Mutual CDC combined[0]/frame1");
	}

out:
	ipio_kfree((void **)&ori);
	return ret;
}

static int allnode_mutual_cdc_data(int index)
{
	int i, ret = 0, len = 0, precmd_len = 0, cdc_len = 0;
	int inDACp = 0, inDACn = 0;
	u8 cmd[15] = {0};
	u8 precmd[256] = {0};
	u8 *ori = NULL;

	if (tItems[index].catalog == TIPRING_RAW) {
		cdc_len = core_mp.tipring_len;
	} else {
		cdc_len = core_mp.frame_len;
	}

	/* Multipling by 2 is due to the 16 bit in each node */
	len = (cdc_len * 2) + 2;

	ILI_DBG("Read X/Y Channel length = %d\n", len);

	if (len <= 2) {
		ILI_ERR("Length is invalid\n");
		ret = -EMP_INVAL;
		goto out;
	}

	/*Pre-Command Init*/
	if (tItems[index].pre_cmd_en) {
		memset(precmd, 0xFF, sizeof(precmd));
		precmd_len = parser_ini_precmd(tItems[index].desp, precmd);

		ili_dump_data(precmd, 16, precmd_len, 0, "PreCommand");

		/*ini file no pre-command, disable precommand*/
		if (precmd_len < 13)
			tItems[index].pre_cmd_en = DISABLE;
	}

	memset(cmd, 0xFF, sizeof(cmd));

	/* CDC init */
	if (mp_cdc_init_cmd_common(cmd, sizeof(cmd), index) < 0) {
		ILI_ERR("Failed to get cdc command\n");
		ret = -EMP_CMD;
		goto out;
	}

	ili_dump_data(cmd, 8, core_mp.cdc_len, 0, "Mutual CDC command");

	/* Allocate a buffer for the original */
	ori = kcalloc(len, sizeof(u8), GFP_KERNEL);
	if (ERR_ALLOC_MEM(ori)) {
		ILI_ERR("Failed to allocate ori, (%ld)\n", PTR_ERR(ori));
		ret = -EMP_NOMEM;
		goto out;
	}

	/* Get original frame(cdc) data */
#if (ENGINEER_FLOW)
	if (!ilits->eng_flow) {
		if (tItems[index].pre_cmd_en) {
			if (precommd(cmd, core_mp.cdc_len, precmd, precmd_len, ori, len, ON, ON) < 0)
				goto out;
		} else {
			if (ilits->wrapper(cmd, core_mp.cdc_len, ori, len, ON, ON) < 0) {
				ILI_ERR("Failed to get cdc data\n");
				ret = -EMP_GET_CDC;
				goto out;
			}
		}
	} else {
		msleep(200);
		if (ilits->wrapper(cmd, core_mp.cdc_len, ori, len, OFF, OFF) < 0) {
			ILI_ERR("Failed to get cdc data\n");
		}
	}
#else
	if (tItems[index].pre_cmd_en) {
		if (precommd(cmd, core_mp.cdc_len, precmd, precmd_len, ori, len, ON, ON) < 0)
			goto out;
	} else {
		if (ilits->wrapper(cmd, core_mp.cdc_len, ori, len, ON, ON) < 0) {
			ILI_ERR("Failed to get cdc data\n");
			ret = -EMP_GET_CDC;
			goto out;
		}
	}
#endif

	ili_dump_data(ori, 8, len, 0, "Mutual CDC original");

	if (frame_buf == NULL) {
		frame_buf = kcalloc(core_mp.frame_len, sizeof(s32), GFP_KERNEL);
		if (ERR_ALLOC_MEM(frame_buf)) {
			ILI_ERR("Failed to allocate FrameBuffer mem (%ld)\n", PTR_ERR(frame_buf));
			ret = -EMP_NOMEM;
			goto out;
		}
	} else {
		memset(frame_buf, 0x0, core_mp.frame_len);
	}

	/* Convert original data to the physical one in each node */
	for (i = 0; i < cdc_len; i++) {
		if (ipio_strcmp(tItems[index].desp, "calibration data(dac)") == 0) {
			/* DAC - P */
			if (((ori[(2 * i) + 1] & 0x80) >> 7) == 1) {
				/* Negative */
				inDACp = 0 - (int)(ori[(2 * i) + 1] & 0x7F);
			} else {
				inDACp = ori[(2 * i) + 1] & 0x7F;
			}

			/* DAC - N */
			if (((ori[(1 + (2 * i)) + 1] & 0x80) >> 7) == 1) {
				/* Negative */
				inDACn = 0 - (int)(ori[(1 + (2 * i)) + 1] & 0x7F);
			} else {
				inDACn = ori[(1 + (2 * i)) + 1] & 0x7F;
			}

			frame_buf[i] = (inDACp + inDACn) / 2;
		} else {
			/* H byte + L byte */
			s32 tmp = (ori[(2 * i) + 1] << 8) + ori[(1 + (2 * i)) + 1];

			if ((tmp & 0x8000) == 0x8000)
				frame_buf[i] = tmp - 65536;
			else
				frame_buf[i] = tmp;

			if (ipio_strcmp(tItems[index].desp, "raw data(no bk)") == 0 ||
				ipio_strcmp(tItems[index].desp, "raw data(no bk) (lcm off)") == 0) {
					frame_buf[i] -= core_mp.no_bk_shift;
			}
		}
	}

	ili_dump_data(frame_buf, 32, cdc_len, core_mp.xch_len, "Mutual CDC combined");

out:
	ipio_kfree((void **)&ori);
	return ret;
}

static void compare_MaxMin_result(int index, s32 *data)
{
	int x, y;
	if (tItems[index].catalog == TIPRING_RAW || tItems[index].catalog == TIPRING_P2P) {
		for (x = 0; x < core_mp.tipring_len; x++) {
			if (tItems[index].max_buf[x] < data[x])
				tItems[index].max_buf[x] = data[x];

			if (tItems[index].min_buf[x] > data[x])
				tItems[index].min_buf[x] = data[x];
		}
	} else {
		for (y = 0; y < core_mp.ych_len; y++) {
			for (x = 0; x < core_mp.xch_len; x++) {
				int shift = y * core_mp.xch_len;

				if (tItems[index].catalog == UNTOUCH_P2P)
					return;
				else if (tItems[index].catalog == TX_RX_DELTA) {
					/* Tx max/min comparison */
					if (core_mp.tx_delta_buf[shift + x] < data[shift + x])
						core_mp.tx_max_buf[shift + x] = data[shift + x];

					if (core_mp.tx_delta_buf[shift + x] > data[shift + x])
						core_mp.tx_min_buf[shift + x] = data[shift + x];

					/* Rx max/min comparison */
					if (core_mp.rx_delta_buf[shift + x] < data[shift + x])
						core_mp.rx_max_buf[shift + x] = data[shift + x];

					if (core_mp.rx_delta_buf[shift + x] > data[shift + x])
						core_mp.rx_min_buf[shift + x] = data[shift + x];
				} else {
					if (tItems[index].max_buf[shift + x] < data[shift + x])
						tItems[index].max_buf[shift + x] = data[shift + x];

					if (tItems[index].min_buf[shift + x] > data[shift + x])
						tItems[index].min_buf[shift + x] = data[shift + x];
				}
			}
		}
	}

}

static int create_mp_test_frame_buffer(int index, int frame_count)
{
	int i;

	ILI_DBG("Create MP frame buffers (index = %d), count = %d\n",
			index, frame_count);

	if (tItems[index].catalog == TX_RX_DELTA) {
		if (core_mp.tx_delta_buf == NULL) {
			core_mp.tx_delta_buf = kcalloc(core_mp.frame_len, sizeof(s32), GFP_KERNEL);
			if (ERR_ALLOC_MEM(core_mp.tx_delta_buf)) {
				ILI_ERR("Failed to allocate tx_delta_buf mem\n");
				ipio_kfree((void **)&core_mp.tx_delta_buf);
				return -ENOMEM;
			}
		}

		if (core_mp.rx_delta_buf == NULL) {
			core_mp.rx_delta_buf = kcalloc(core_mp.frame_len, sizeof(s32), GFP_KERNEL);
			if (ERR_ALLOC_MEM(core_mp.rx_delta_buf)) {
				ILI_ERR("Failed to allocate rx_delta_buf mem\n");
				ipio_kfree((void **)&core_mp.rx_delta_buf);
				return -ENOMEM;
			}
		}

		if (core_mp.tx_max_buf == NULL) {
			core_mp.tx_max_buf = kcalloc(core_mp.frame_len, sizeof(s32), GFP_KERNEL);
			if (ERR_ALLOC_MEM(core_mp.tx_max_buf)) {
				ILI_ERR("Failed to allocate tx_max_buf mem\n");
				ipio_kfree((void **)&core_mp.tx_max_buf);
				return -ENOMEM;
			}
		}

		if (core_mp.tx_min_buf == NULL) {
			core_mp.tx_min_buf = kcalloc(core_mp.frame_len, sizeof(s32), GFP_KERNEL);
			if (ERR_ALLOC_MEM(core_mp.tx_min_buf)) {
				ILI_ERR("Failed to allocate tx_min_buf mem\n");
				ipio_kfree((void **)&core_mp.tx_min_buf);
				return -ENOMEM;
			}
		}

		if (core_mp.rx_max_buf == NULL) {
			core_mp.rx_max_buf = kcalloc(core_mp.frame_len, sizeof(s32), GFP_KERNEL);
			if (ERR_ALLOC_MEM(core_mp.rx_max_buf)) {
				ILI_ERR("Failed to allocate rx_max_buf mem\n");
				ipio_kfree((void **)&core_mp.rx_max_buf);
				return -ENOMEM;
			}
		}

		if (core_mp.rx_min_buf == NULL) {
			core_mp.rx_min_buf = kcalloc(core_mp.frame_len, sizeof(s32), GFP_KERNEL);
			if (ERR_ALLOC_MEM(core_mp.rx_min_buf)) {
				ILI_ERR("Failed to allocate rx_min_buf mem\n");
				ipio_kfree((void **)&core_mp.rx_min_buf);
				return -ENOMEM;
			}
		}
	} else {
		if (tItems[index].buf == NULL) {
			tItems[index].buf = vmalloc(frame_count * core_mp.frame_len * sizeof(s32));
			if (ERR_ALLOC_MEM(tItems[index].buf)) {
				ILI_ERR("Failed to allocate buf mem\n");
				ipio_kfree((void **)&tItems[index].buf);
				return -ENOMEM;
			}
		}

		if (tItems[index].result_buf == NULL) {
			tItems[index].result_buf = kcalloc(core_mp.frame_len, sizeof(s32), GFP_KERNEL);
			if (ERR_ALLOC_MEM(tItems[index].result_buf)) {
				ILI_ERR("Failed to allocate result_buf mem\n");
				ipio_kfree((void **)&tItems[index].result_buf);
				return -ENOMEM;
			}
		}

		if (tItems[index].max_buf == NULL) {
			tItems[index].max_buf = kcalloc(core_mp.frame_len, sizeof(s32), GFP_KERNEL);
			if (ERR_ALLOC_MEM(tItems[index].max_buf)) {
				ILI_ERR("Failed to allocate max_buf mem\n");
				ipio_kfree((void **)&tItems[index].max_buf);
				return -ENOMEM;
			}
		}

		if (tItems[index].min_buf == NULL) {
			tItems[index].min_buf = kcalloc(core_mp.frame_len, sizeof(s32), GFP_KERNEL);
			if (ERR_ALLOC_MEM(tItems[index].min_buf)) {
				ILI_ERR("Failed to allocate min_buf mem\n");
				ipio_kfree((void **)&tItems[index].min_buf);
				return -ENOMEM;
			}
		}

		if (tItems[index].spec_option == BENCHMARK) {
			if (tItems[index].bch_mrk_max == NULL) {
				tItems[index].bch_mrk_max = (s32 **)kzalloc(tItems[index].bch_mrk_frm_num * sizeof(s32 *), GFP_KERNEL);
				if (ERR_ALLOC_MEM(tItems[index].bch_mrk_max)) {
					ILI_ERR("Failed to allocate bch_mrk_max mem\n");
					ipio_kfree((void **)&tItems[index].bch_mrk_max);
					return -ENOMEM;
				}

				for (i = 0; i < tItems[index].bch_mrk_frm_num; i++) {
					tItems[index].bch_mrk_max[i] = (s32 *)kzalloc(core_mp.frame_len * sizeof(s32), GFP_KERNEL);
					if (ERR_ALLOC_MEM(tItems[index].bch_mrk_max[i])) {
						ILI_ERR("Failed to allocate bch_mrk_max[%d] mem\n", i);
						ipio_kfree((void **)&tItems[index].bch_mrk_max[i]);
						return -ENOMEM;
					}
				}
			}

			if (tItems[index].bch_mrk_min == NULL) {
				tItems[index].bch_mrk_min = (s32 **)kzalloc(tItems[index].bch_mrk_frm_num * sizeof(s32 *), GFP_KERNEL);
				if (ERR_ALLOC_MEM(tItems[index].bch_mrk_min)) {
					ILI_ERR("Failed to allocate bch_mrk_min mem\n");
					ipio_kfree((void **)&tItems[index].bch_mrk_min);
					return -ENOMEM;
				}
				for (i = 0; i < tItems[index].bch_mrk_frm_num; i++) {
					tItems[index].bch_mrk_min[i] = (s32 *)kzalloc(core_mp.frame_len * sizeof(s32), GFP_KERNEL);
					if (ERR_ALLOC_MEM(tItems[index].bch_mrk_min[i])) {
						ILI_ERR("Failed to allocate bch_mrk_min[%d] mem\n", i);
						ipio_kfree((void **)&tItems[index].bch_mrk_min[i]);
						return -ENOMEM;
					}
				}
			}

			if (tItems[index].bench_mark_max == NULL) {
				tItems[index].bench_mark_max = kcalloc(core_mp.frame_len, sizeof(s32), GFP_KERNEL);
				if (ERR_ALLOC_MEM(tItems[index].bench_mark_max)) {
					ILI_ERR("Failed to allocate bench_mark_max mem\n");
					ipio_kfree((void **)&tItems[index].bench_mark_max);
					return -ENOMEM;
				}
			}
			if (tItems[index].bench_mark_min == NULL) {
				tItems[index].bench_mark_min = kcalloc(core_mp.frame_len, sizeof(s32), GFP_KERNEL);
				if (ERR_ALLOC_MEM(tItems[index].bench_mark_min)) {
					ILI_ERR("Failed to allocate bench_mark_min mem\n");
					ipio_kfree((void **)&tItems[index].bench_mark_min);
					return -ENOMEM;
				}
			}
		}
	}
	return 0;
}

static int pin_test(int index)
{
	int ret = 0;
	u8 cmd[5] = {0};

	ILI_INFO("PIN test start");
	ILI_INFO("test_int_pin = 0x%x\n", tItems[index].test_int_pin);
	ILI_INFO("int_pulse_test = 0x%x\n", tItems[index].int_pulse_test);
	ILI_INFO("delay_time = 0x%x\n", tItems[index].delay_time);

	if (tItems[index].test_int_pin == ENABLE) {
		/* test HIGH level*/
		cmd[0] = tItems[index].cmd;
		cmd[1] = 0x1;
		ret = ilits->wrapper(cmd, 2, NULL, 0, OFF, OFF);
		if (ret < 0) {
			ILI_ERR("Write command failed\n");
			ret = -EMP_CMD;
			goto out;
		}

		ret =  ilits->detect_int_stat(false);
		if (ret < 0) {
			ret = -EMP_INT;
			goto out;
		}

		/* test LOW level*/
		cmd[1] = 0x0;
		ret = ilits->wrapper(cmd, 2, NULL, 0, OFF, OFF);
		if (ret < 0) {
			ILI_ERR("Write command failed\n");
			ret = -EMP_CMD;
			goto out;
		}

		ret =  ilits->detect_int_stat(false);
		if (ret < 0) {
			ret = -EMP_INT;
			goto out;
		}
	}

	if (tItems[index].int_pulse_test == ENABLE) {

		ILI_INFO("MP IRQ Rising Trigger Test\n");
		cmd[1] = 0x2;
		cmd[2] = tItems[index].delay_time;

		ili_irq_unregister();
		ret = ili_irq_register(IRQF_TRIGGER_RISING);
		if (ret < 0) {
			ret = -EMP_INT;
			goto out;
		}

		atomic_set(&ilits->cmd_int_check, ENABLE);
		ret = ilits->wrapper(cmd, 3, NULL, 0, OFF, OFF);
		if (ret < 0) {
			ILI_ERR("Write command failed\n");
			ret = -EMP_CMD;
			goto out;
		}

		ret = ilits->detect_int_stat(false);
		if (ret < 0) {
			ret = -EMP_INT;
			goto out;
		}

		ILI_INFO("MP IRQ Falling Trigger Test\n");
		ili_irq_unregister();
		ret = ili_irq_register(IRQF_TRIGGER_FALLING);
		if (ret < 0) {
			ret = -EMP_INT;
			goto out;
		}

		atomic_set(&ilits->cmd_int_check, ENABLE);

		ret = ilits->wrapper(cmd, 3, NULL, 0, OFF, OFF);
		if (ret < 0) {
			ILI_ERR("Write command failed\n");
			ret = -EMP_CMD;
			goto out;
		}

		ret = ilits->detect_int_stat(false);
		if (ret < 0) {
			ret = -EMP_INT;
			goto out;
		}
	}

	tItems[index].item_result = MP_DATA_PASS;

out:
	if (ret < 0)
		tItems[index].item_result = MP_DATA_FAIL;

	ILI_INFO("Change to defualt IRQ trigger type\n");
	ili_irq_unregister();
	ili_irq_register(ilits->irq_tirgger_type);
	atomic_set(&ilits->cmd_int_check, DISABLE);

	return ret;
}

static int mutual_test(int index)
{
	int i = 0, j = 0, x = 0, y = 0, ret = 0, get_frame_cont = 1;

	ILI_DBG("index = %d, desp = %s, Frame Count = %d\n",
		index, tItems[index].desp, tItems[index].frame_count);

	/*
	 * We assume that users who are calling the test forget to config frame count
	 * as 1, so we just help them to set it up.
	 */
	if (tItems[index].frame_count <= 0) {
		ILI_ERR("Frame count is zero, which is at least set as 1\n");
		tItems[index].frame_count = 1;
	}

	ret = create_mp_test_frame_buffer(index, tItems[index].frame_count);
	if (ret < 0) {
		ret = -EMP_NOMEM;
		goto out;
	}

	if (tItems[index].catalog == TIPRING_RAW) {
		/* Init Max/Min buffer */
		for (x = 0; x < core_mp.tipring_len; x++) {
			tItems[index].max_buf[x] = INT_MIN;
			tItems[index].min_buf[x] = INT_MAX;
		}

		get_frame_cont = tItems[index].frame_count;

		if (tItems[index].spec_option == BENCHMARK) {
			parser_tip_ring_ini_benchmark(tItems[index].bench_mark_max, tItems[index].bench_mark_min,
				tItems[index].type_option, tItems[index].desp, core_mp.tipring_len);
		}
	} else {
		/* Init Max/Min buffer */
		for (y = 0; y < core_mp.ych_len; y++) {
			for (x = 0; x < core_mp.xch_len; x++) {
				if (tItems[i].catalog == TX_RX_DELTA) {
					core_mp.tx_max_buf[y * core_mp.xch_len + x] = INT_MIN;
					core_mp.rx_max_buf[y * core_mp.xch_len + x] = INT_MIN;
					core_mp.tx_min_buf[y * core_mp.xch_len + x] = INT_MAX;
					core_mp.rx_min_buf[y * core_mp.xch_len + x] = INT_MAX;
				} else {
					tItems[index].max_buf[y * core_mp.xch_len + x] = INT_MIN;
					tItems[index].min_buf[y * core_mp.xch_len + x] = INT_MAX;
				}
			}
		}

		if (tItems[index].spec_option == BENCHMARK) {
			parser_ini_benchmark(tItems[index].bench_mark_max, tItems[index].bench_mark_min,
				tItems[index].type_option, tItems[index].desp, core_mp.frame_len, BENCHMARK_KEY_NAME);
			dump_benchmark_data(tItems[index].bench_mark_max, tItems[index].bench_mark_min);
		}
	}

	if (tItems[index].catalog != PEAK_TO_PEAK_TEST)
		get_frame_cont = tItems[index].frame_count;


	for (i = 0; i < get_frame_cont; i++) {
		ret = allnode_mutual_cdc_data(index);
		if (ret < 0) {
			ILI_ERR("Failed to initialise CDC data, %d\n", ret);
			goto out;
		}
		switch (tItems[index].catalog) {
		case PIXEL:
			run_pixel_test(index);
			break;
		case UNTOUCH_P2P:
			run_untouch_p2p_test(index);
			break;
		case OPEN_TEST:
			run_open_test(index);
			break;
		case TX_RX_DELTA:
			run_tx_rx_delta_test(index);
			break;
		case SHORT_TEST:
			ret = short_test(index, i);
			if (ret < 0) {
				ret = -EMP_PARA_NULL;
				goto out;
			}
			break;
		case TIPRING_RAW:
			for (j = 0; j < core_mp.tipring_len; j++)
				tItems[index].buf[i * core_mp.tipring_len + j] = frame_buf[j];
			break;
		default:
			for (j = 0; j < core_mp.frame_len; j++)
				tItems[index].buf[i * core_mp.frame_len + j] = frame_buf[j];
			break;
		}
		if (tItems[index].catalog == TIPRING_RAW)
			compare_MaxMin_result(index, &tItems[index].buf[i * core_mp.tipring_len]);
		else
			compare_MaxMin_result(index, &tItems[index].buf[i * core_mp.frame_len]);

	}

out:
	return ret;
}

static int  peak_to_peak_test(int index)
{
	int i = 0, j = 0, x = 0, y = 0, ret = 0, cdc_len = 0;
	char benchmark_str[128] = {0};

	if (tItems[index].catalog == TIPRING_P2P)
		cdc_len = core_mp.tipring_len;
	else
		cdc_len = core_mp.frame_len;

	ILI_DBG("index = %d, desp = %s bch_mrk_frm_num = %d, cdc len = %d\n"
		, index, tItems[index].desp, tItems[index].bch_mrk_frm_num, cdc_len);

	ret = create_mp_test_frame_buffer(index, tItems[index].bch_mrk_frm_num);
	if (ret < 0) {
		ret = -EMP_NOMEM;
		goto out;
	}

	if (tItems[index].spec_option == BENCHMARK) {
		if (tItems[index].catalog == TIPRING_P2P) {
			for (i = 0; i < tItems[index].bch_mrk_frm_num; i++) {
				if (tItems[index].bch_mrk_multi){
					parser_ini_benchmark(tItems[index].bch_mrk_max[i], tItems[index].bch_mrk_min[i], tItems[index].type_option,
						tItems[index].desp, core_mp.nPxRaw_tipring_len, get_csv_frame_name(BENCHMARK_KEY_NAME, i + 1));

					parser_ini_benchmark(tItems[index].bch_mrk_max[i] + core_mp.nPxRaw_tipring_len,
						tItems[index].bch_mrk_min[i] + core_mp.nPxRaw_tipring_len, tItems[index].type_option, tItems[index].desp,
						core_mp.nPyRaw_tipring_len, get_csv_frame_name(BENCHMARK_KEY_NAME, i + 4));
				} else {
					parser_ini_benchmark(tItems[index].bch_mrk_max[i], tItems[index].bch_mrk_min[i], tItems[index].type_option,
						tItems[index].desp, core_mp.nPxRaw_tipring_len, get_csv_frame_name(BENCHMARK_KEY_NAME, 1));

					parser_ini_benchmark(tItems[index].bch_mrk_max[i] + core_mp.nPxRaw_tipring_len,
						tItems[index].bch_mrk_min[i] + core_mp.nPxRaw_tipring_len, tItems[index].type_option,
						tItems[index].desp, core_mp.nPyRaw_tipring_len, get_csv_frame_name(BENCHMARK_KEY_NAME, 2));
				}

				if (debug_en) {
					ili_dump_data(tItems[index].bch_mrk_max[i], 32, core_mp.tipring_len, 0, get_csv_frame_name("Dump Tip Ring Benchmark Max ", i + 1));
					ili_dump_data(tItems[index].bch_mrk_min[i], 32, core_mp.tipring_len, 0, get_csv_frame_name("Dump Tip Ring Benchmark Min ", i + 1));
				}
			}
		} else {
			for (i = 0; i < tItems[index].bch_mrk_frm_num; i++) {
				if (tItems[index].bch_mrk_multi)
					snprintf(benchmark_str, sizeof(benchmark_str), "%s%d", BENCHMARK_KEY_NAME, i+1);
				else
					snprintf(benchmark_str, sizeof(benchmark_str), "%s", BENCHMARK_KEY_NAME);

				parser_ini_benchmark(tItems[index].bch_mrk_max[i], tItems[index].bch_mrk_min[i],
						tItems[index].type_option, tItems[index].desp, core_mp.frame_len, benchmark_str);

				dump_benchmark_data(tItems[index].bch_mrk_max[i], tItems[index].bch_mrk_min[i]);
			}
		}
	}

	/* Init Max/Min buffer */
	for (y = 0; y < core_mp.ych_len; y++) {
		for (x = 0; x < core_mp.xch_len; x++) {
			tItems[index].max_buf[y * core_mp.xch_len + x] = INT_MIN;
			tItems[index].min_buf[y * core_mp.xch_len + x] = INT_MAX;
		}
	}

	ret = allnode_peak_to_peak_cdc_data(index);
	if (ret < 0) {
		ILI_ERR("Failed to initialise CDC data, %d\n", ret);
		goto out;
	}
	for (i = 0; i < tItems[index].bch_mrk_frm_num; i++) {
		for (j = 0; j < cdc_len; j++)
			tItems[index].buf[i * cdc_len + j] = frm_buf[i][j];

		compare_MaxMin_result(index, &tItems[index].buf[i * cdc_len]);
	}

out:
	return ret;
}


static int open_test_sp(int index)
{
	struct mp_test_P540_open *open;
	int i = 0, x = 0, y = 0, ret = 0, addr = 0;
	int Charge_AA = 0, Charge_Border = 0, Charge_Notch = 0, full_open_rate = 0;
	char str[512] = {0};

	ILI_DBG("index = %d, desp = %s, Frame Count = %d\n",
		index, tItems[index].desp, tItems[index].frame_count);

	/*
	 * We assume that users who are calling the test forget to config frame count
	 * as 1, so we just help them to set it up.
	 */
	if (tItems[index].frame_count <= 0) {
		ILI_ERR("Frame count is zero, which is at least set as 1\n");
		tItems[index].frame_count = 1;
	}

	open = kzalloc(tItems[index].frame_count * sizeof(struct mp_test_P540_open), GFP_KERNEL);
	if (ERR_ALLOC_MEM(open)) {
		ILI_ERR("Failed to allocate open mem (%ld)\n", PTR_ERR(open));
		ret = -EMP_NOMEM;
		goto out;
	}

	ret = create_mp_test_frame_buffer(index, tItems[index].frame_count);
	if (ret < 0) {
		ret = -EMP_NOMEM;
		goto out;
	}

	if (frame1_cbk700 == NULL) {
		frame1_cbk700 = kcalloc(core_mp.frame_len, sizeof(s32), GFP_KERNEL);
		if (ERR_ALLOC_MEM(frame1_cbk700)) {
			ILI_ERR("Failed to allocate frame1_cbk700 buffer\n");
			return -EMP_NOMEM;
		}
	} else {
		memset(frame1_cbk700, 0x0, core_mp.frame_len);
	}

	if (frame1_cbk250 == NULL) {
		frame1_cbk250 = kcalloc(core_mp.frame_len, sizeof(s32), GFP_KERNEL);
		if (ERR_ALLOC_MEM(frame1_cbk250)) {
			ILI_ERR("Failed to allocate frame1_cbk250 buffer\n");
			ipio_kfree((void **)&frame1_cbk700);
			return -EMP_NOMEM;
		}
	} else {
		memset(frame1_cbk250, 0x0, core_mp.frame_len);
	}

	if (frame1_cbk200 == NULL) {
		frame1_cbk200 = kcalloc(core_mp.frame_len, sizeof(s32), GFP_KERNEL);
		if (ERR_ALLOC_MEM(frame1_cbk200)) {
			ILI_ERR("Failed to allocate cbk buffer\n");
			ipio_kfree((void **)&frame1_cbk700);
			ipio_kfree((void **)&frame1_cbk250);
			return -EMP_NOMEM;
		}
	} else {
		memset(frame1_cbk200, 0x0, core_mp.frame_len);
	}

	tItems[index].node_type = kcalloc(core_mp.frame_len, sizeof(s32), GFP_KERNEL);
	if (ERR_ALLOC_MEM(tItems[index].node_type)) {
		ILI_ERR("Failed to allocate node_type FRAME buffer\n");
		return -EMP_NOMEM;
	}

	/* Init Max/Min buffer */
	for (y = 0; y < core_mp.ych_len; y++) {
		for (x = 0; x < core_mp.xch_len; x++) {
			tItems[index].max_buf[y * core_mp.xch_len + x] = INT_MIN;
			tItems[index].min_buf[y * core_mp.xch_len + x] = INT_MAX;
		}
	}

	if (tItems[index].spec_option == BENCHMARK) {
		parser_ini_benchmark(tItems[index].bench_mark_max, tItems[index].bench_mark_min,
			tItems[index].type_option, tItems[index].desp, core_mp.frame_len, BENCHMARK_KEY_NAME);
		dump_benchmark_data(tItems[index].bench_mark_max, tItems[index].bench_mark_min);
	}

	parser_ini_nodetype(tItems[index].node_type, NODE_TYPE_KEY_NAME, core_mp.frame_len);
	dump_node_type_buffer(tItems[index].node_type, "node type");

	parser_get_int_data(tItems[index].desp, "charge_aa", str, sizeof(str));
	Charge_AA = ili_katoi(str);

	parser_get_int_data(tItems[index].desp, "charge_border", str, sizeof(str));
	Charge_Border = ili_katoi(str);

	parser_get_int_data(tItems[index].desp, "charge_notch", str, sizeof(str));
	Charge_Notch = ili_katoi(str);

	parser_get_int_data(tItems[index].desp, "full open", str, sizeof(str));
	full_open_rate = ili_katoi(str);

	parser_get_int_data(tItems[index].desp, "tvch", str, sizeof(str));
	open_para.tvch = ili_katoi(str);

	parser_get_int_data(tItems[index].desp, "tvcl", str, sizeof(str));
	open_para.tvcl = ili_katoi(str);

	ILI_DBG("AA = %d, Border = %d, Notch = %d, full_open_rate = %d, tvch = %d, tvcl = %d\n",
			Charge_AA, Charge_Border, Charge_Notch,
			full_open_rate, open_para.tvch, open_para.tvcl);

	for (i = 0; i < tItems[index].frame_count; i++) {
		open[i].tdf_700 = kcalloc(core_mp.frame_len, sizeof(s32), GFP_KERNEL);
		open[i].tdf_250 = kcalloc(core_mp.frame_len, sizeof(s32), GFP_KERNEL);
		open[i].tdf_200 = kcalloc(core_mp.frame_len, sizeof(s32), GFP_KERNEL);
		open[i].cbk_700 = kcalloc(core_mp.frame_len, sizeof(s32), GFP_KERNEL);
		open[i].cbk_250 = kcalloc(core_mp.frame_len, sizeof(s32), GFP_KERNEL);
		open[i].cbk_200 = kcalloc(core_mp.frame_len, sizeof(s32), GFP_KERNEL);
		open[i].charg_rate = kcalloc(core_mp.frame_len, sizeof(s32), GFP_KERNEL);
		open[i].full_Open = kcalloc(core_mp.frame_len, sizeof(s32), GFP_KERNEL);
		open[i].dac = kcalloc(core_mp.frame_len, sizeof(s32), GFP_KERNEL);
	}

	for (i = 0; i < tItems[index].frame_count; i++) {
		ret = allnode_open_cdc_data(0, open[i].dac, index);
		if (ret < 0) {
			ILI_ERR("Failed to get Open SP DAC data, %d\n", ret);
			goto out;
		}
		ret = allnode_open_cdc_data(1, open[i].tdf_700, index);
		if (ret < 0) {
			ILI_ERR("Failed to get Open SP Raw1 data, %d\n", ret);
			goto out;
		}
		ret = allnode_open_cdc_data(2, open[i].tdf_250, index);
		if (ret < 0) {
			ILI_ERR("Failed to get Open SP Raw2 data, %d\n", ret);
			goto out;
		}
		ret = allnode_open_cdc_data(3, open[i].tdf_200, index);
		if (ret < 0) {
			ILI_ERR("Failed to get Open SP Raw3 data, %d\n", ret);
			goto out;
		}
		allnode_open_cdc_result(index, open[i].cbk_700, open[i].dac, open[i].tdf_700);
		allnode_open_cdc_result(index, open[i].cbk_250, open[i].dac, open[i].tdf_250);
		allnode_open_cdc_result(index, open[i].cbk_200, open[i].dac, open[i].tdf_200);

		addr = 0;

		/* record fist frame for debug */
		if (i == 0) {
			ipio_memcpy(frame1_cbk700, open[i].cbk_700, core_mp.frame_len * sizeof(s32), core_mp.frame_len * sizeof(s32));
			ipio_memcpy(frame1_cbk250, open[i].cbk_250, core_mp.frame_len * sizeof(s32), core_mp.frame_len * sizeof(s32));
			ipio_memcpy(frame1_cbk200, open[i].cbk_200, core_mp.frame_len * sizeof(s32), core_mp.frame_len * sizeof(s32));
		}

		ili_dump_data(open[i].cbk_700, 10, core_mp.frame_len, core_mp.xch_len, "cbk 700");
		ili_dump_data(open[i].cbk_250, 10, core_mp.frame_len, core_mp.xch_len, "cbk 250");
		ili_dump_data(open[i].cbk_200, 10, core_mp.frame_len, core_mp.xch_len, "cbk 200");

		for (y = 0; y < core_mp.ych_len; y++) {
			for (x = 0; x < core_mp.xch_len; x++) {
				open[i].charg_rate[addr] = open[i].cbk_250[addr] * 100 / open[i].cbk_700[addr];
				open[i].full_Open[addr] = open[i].cbk_700[addr] - open[i].cbk_200[addr];
				addr++;
			}
		}

		ili_dump_data(open[i].charg_rate, 10, core_mp.frame_len, core_mp.xch_len, "origin charge rate");
		ili_dump_data(open[i].full_Open, 10, core_mp.frame_len, core_mp.xch_len, "origin full open");

		addr = 0;
		for (y = 0; y < core_mp.ych_len; y++) {
			for (x = 0; x < core_mp.xch_len; x++) {
				if (full_open_rate_compare(open[i].full_Open, open[i].cbk_700, x, y, tItems[index].node_type[addr], full_open_rate) == false) {
					tItems[index].buf[(i * core_mp.frame_len) + addr] = 0;
					open[i].charg_rate[addr] = 0;
				}
				addr++;
			}
		}

		ili_dump_data(&tItems[index].buf[(i * core_mp.frame_len)], 10, core_mp.frame_len, core_mp.xch_len, "after full_open_rate_compare");

		addr = 0;
		for (y = 0; y < core_mp.ych_len; y++) {
			for (x = 0; x < core_mp.xch_len; x++) {
				tItems[index].buf[(i * core_mp.frame_len) + addr] = compare_charge(open[i].charg_rate, x, y, tItems[index].node_type, Charge_AA, Charge_Border, Charge_Notch);
				addr++;
			}
		}

		ili_dump_data(&tItems[index].buf[(i * core_mp.frame_len)], 10, core_mp.frame_len, core_mp.xch_len, "after compare charge rate");

		compare_MaxMin_result(index, &tItems[index].buf[(i * core_mp.frame_len)]);
	}

out:
	ipio_kfree((void **)&tItems[index].node_type);

	if (open != NULL) {
		for (i = 0; i < tItems[index].frame_count; i++) {
			ipio_kfree((void **)&open[i].tdf_700);
			ipio_kfree((void **)&open[i].tdf_250);
			ipio_kfree((void **)&open[i].tdf_200);
			ipio_kfree((void **)&open[i].cbk_700);
			ipio_kfree((void **)&open[i].cbk_250);
			ipio_kfree((void **)&open[i].cbk_200);
			ipio_kfree((void **)&open[i].charg_rate);
			ipio_kfree((void **)&open[i].full_Open);
			ipio_kfree((void **)&open[i].dac);
		}
		kfree(open);
		open = NULL;
	}

	return ret;
}

static int open_test_cap(int index)
{
	u32 pid = core_mp.chip_pid >> 8;
	struct mp_test_open_c *open;
	int i = 0, x = 0, y = 0, ret = 0, addr = 0;
	bool isOpenX = DISABLE;
	char str[512] = {0};

	isOpenX = (ipio_strcmp(tItems[index].desp, "open test_c (openx enable)") == 0) ? ENABLE : DISABLE;

	ILI_DBG("index = %d, desp = %s, Frame Count = %d, isOpenX = %d\n",
		index, tItems[index].desp, tItems[index].frame_count, isOpenX);

	if (tItems[index].frame_count <= 0) {
		ILI_ERR("Frame count is zero, which is at least set as 1\n");
		tItems[index].frame_count = 1;
	}

	open = kzalloc(tItems[index].frame_count * sizeof(struct mp_test_open_c), GFP_KERNEL);
	if (ERR_ALLOC_MEM(open)) {
		ILI_ERR("Failed to allocate open mem (%ld)\n", PTR_ERR(open));
		ret = -EMP_NOMEM;
		goto out;
	}

	if (create_mp_test_frame_buffer(index, tItems[index].frame_count) < 0) {
		ret = -EMP_NOMEM;
		goto out;
	}

	if (isOpenX) {
		if (openx_cap_dac == NULL) {
			openx_cap_dac = kcalloc(core_mp.frame_len, sizeof(s32), GFP_KERNEL);
			if (ERR_ALLOC_MEM(openx_cap_dac)) {
				ILI_ERR("Failed to allocate cap_dac buffer\n");
				return -EMP_NOMEM;
			}
		} else {
			memset(openx_cap_dac, 0x0, core_mp.frame_len);
		}

		if (openx_cap_raw == NULL) {
			openx_cap_raw = kcalloc(core_mp.frame_len, sizeof(s32), GFP_KERNEL);
			if (ERR_ALLOC_MEM(openx_cap_raw)) {
				ILI_ERR("Failed to allocate cap_raw buffer\n");
				ipio_kfree((void **)&openx_cap_dac);
				return -EMP_NOMEM;
			}
		} else {
			memset(openx_cap_raw, 0x0, core_mp.frame_len);
		}
	} else {
		if (cap_dac == NULL) {
			cap_dac = kcalloc(core_mp.frame_len, sizeof(s32), GFP_KERNEL);
			if (ERR_ALLOC_MEM(cap_dac)) {
				ILI_ERR("Failed to allocate cap_dac buffer\n");
				return -EMP_NOMEM;
			}
		} else {
			memset(cap_dac, 0x0, core_mp.frame_len);
		}

		if (cap_raw == NULL) {
			cap_raw = kcalloc(core_mp.frame_len, sizeof(s32), GFP_KERNEL);
			if (ERR_ALLOC_MEM(cap_raw)) {
				ILI_ERR("Failed to allocate cap_raw buffer\n");
				ipio_kfree((void **)&cap_dac);
				return -EMP_NOMEM;
			}
		} else {
			memset(cap_raw, 0x0, core_mp.frame_len);
		}
	}

	/* Init Max/Min buffer */
	for (y = 0; y < core_mp.ych_len; y++) {
		for (x = 0; x < core_mp.xch_len; x++) {
			tItems[index].max_buf[y * core_mp.xch_len + x] = INT_MIN;
			tItems[index].min_buf[y * core_mp.xch_len + x] = INT_MAX;
		}
	}

	if (tItems[index].spec_option == BENCHMARK) {
		parser_ini_benchmark(tItems[index].bench_mark_max, tItems[index].bench_mark_min,
				tItems[index].type_option, tItems[index].desp, core_mp.frame_len, BENCHMARK_KEY_NAME);
		dump_benchmark_data(tItems[index].bench_mark_max, tItems[index].bench_mark_min);
	}

	parser_get_int_data(tItems[index].desp, "gain", str, sizeof(str));
	open_para.gain = ili_katoi(str);

	parser_get_int_data(tItems[index].desp, "tvch", str, sizeof(str));
	open_para.tvch = ili_katoi(str);

	parser_get_int_data(tItems[index].desp, "tvcl", str, sizeof(str));
	open_para.tvcl = ili_katoi(str);

	parser_get_int_data(tItems[index].desp, "cbk_step", str, sizeof(str));
	open_para.cbk_step = ili_katoi(str);

	parser_get_int_data(tItems[index].desp, "cint", str, sizeof(str));
	open_para.cint = ili_katoi(str);

	parser_get_int_data(tItems[index].desp, "vbk", str, sizeof(str));
	open_para.invbk = ili_katoi(str);

	parser_get_int_data(tItems[index].desp, "vadc_range", str, sizeof(str));
	open_para.vadc_range = ili_katoi(str);

	/* 9881N, 9881O, 7807Q, 7807S not support cbk_step and cint read from mp.ini */
	if ((pid != 0x988117) && (pid != 0x988118) && (pid != 0x78071A) && (pid != 0x78071C)) {
		if ((open_para.cbk_step == 0) || (open_para.cint == 0)) {
			ILI_ERR("Failed to get open parameter");
			ret = -EMP_PARA_NULL;
			core_mp.lost_parameter = true;
			goto out;
		}
	}

	ILI_INFO("gain = %d, tvch = %d, tvcl = %d, cbk_step = %d, cint = %d\n", open_para.gain, open_para.tvch, open_para.tvcl, open_para.cbk_step, open_para.cint);
	for (i = 0; i < tItems[index].frame_count; i++) {
		open[i].cap_dac = kcalloc(core_mp.frame_len, sizeof(s32), GFP_KERNEL);
		open[i].cap_raw = kcalloc(core_mp.frame_len, sizeof(s32), GFP_KERNEL);
		open[i].dcl_cap = kcalloc(core_mp.frame_len, sizeof(s32), GFP_KERNEL);
	}

	for (i = 0; i < tItems[index].frame_count; i++) {
		if (isOpenX) {
			ret = allnode_open_cdc_data(6, open[i].cap_dac, index);
			if (ret < 0) {
				ILI_ERR("Failed to get Open_X CAP DAC data, %d\n", ret);
				goto out;
			}
			ret = allnode_open_cdc_data(7, open[i].cap_raw, index);
			if (ret < 0) {
				ILI_ERR("Failed to get Open_X CAP RAW data, %d\n", ret);
				goto out;
			}
		} else {
			ret = allnode_open_cdc_data(4, open[i].cap_dac, index);
			if (ret < 0) {
				ILI_ERR("Failed to get Open CAP DAC data, %d\n", ret);
				goto out;
			}
			ret = allnode_open_cdc_data(5, open[i].cap_raw, index);
			if (ret < 0) {
				ILI_ERR("Failed to get Open CAP RAW data, %d\n", ret);
				goto out;
			}
		}

		allnode_open_cdc_result(index, open[i].dcl_cap, open[i].cap_dac, open[i].cap_raw);

		/* record fist frame for debug */
		if (i == 0) {
			if (isOpenX) {
				ipio_memcpy(openx_cap_dac, open[i].cap_dac, core_mp.frame_len * sizeof(s32), core_mp.frame_len * sizeof(s32));
				ipio_memcpy(openx_cap_raw, open[i].cap_raw, core_mp.frame_len * sizeof(s32), core_mp.frame_len * sizeof(s32));
			} else {
				ipio_memcpy(cap_dac, open[i].cap_dac, core_mp.frame_len * sizeof(s32), core_mp.frame_len * sizeof(s32));
				ipio_memcpy(cap_raw, open[i].cap_raw, core_mp.frame_len * sizeof(s32), core_mp.frame_len * sizeof(s32));
			}
		}

		ili_dump_data(open[i].dcl_cap, 10, core_mp.frame_len, core_mp.xch_len, "DCL_Cap");

		addr = 0;
		for (y = 0; y < core_mp.ych_len; y++) {
			for (x = 0; x < core_mp.xch_len; x++) {
				tItems[index].buf[(i * core_mp.frame_len) + addr] = open[i].dcl_cap[addr];
				addr++;
			}
		}
		compare_MaxMin_result(index, &tItems[index].buf[i * core_mp.frame_len]);
	}

out:
	if (open != NULL) {
		for (i = 0; i < tItems[index].frame_count; i++) {
			ipio_kfree((void **)&open[i].cap_dac);
			ipio_kfree((void **)&open[i].cap_raw);
			ipio_kfree((void **)&open[i].dcl_cap);
		}
		kfree(open);
		open = NULL;
	}

	return ret;
}

static int key_test(int index)
{
	int i, j = 0, ret = 0;

	ILI_DBG("Item = %s, Frame Count = %d\n",
		tItems[index].desp, tItems[index].frame_count);

	if (tItems[index].frame_count == 0) {
		ILI_ERR("Frame count is zero, which at least sets as 1\n");
		ret = -EINVAL;
		goto out;
	}

	ret = create_mp_test_frame_buffer(index, tItems[index].frame_count);
	if (ret < 0)
		goto out;

	for (i = 0; i < tItems[index].frame_count; i++) {
		ret = allnode_key_cdc_data(index);
		if (ret < 0) {
			ILI_ERR("Failed to initialise CDC data, %d\n", ret);
			goto out;
		}

		for (j = 0; j < core_mp.key_len; j++)
			tItems[index].buf[j] = key_buf[j];
	}

	compare_MaxMin_result(index, tItems[index].buf);

out:
	return ret;
}

static int self_test(int index)
{
	ILI_ERR("TDDI has no self to be tested currently\n");
	return -1;
}

static int st_test(int index)
{
	ILI_ERR("ST Test is not supported by the driver\n");
	return -1;
}

int ili_parsing_sram_param(void)
{
	char str[100] = {0};
	int i, ret = 0;

	memset(&ilits->sram_para, 0x0, sizeof(ilits->sram_para));

	if (ini_info == NULL) {
		ini_info = (struct ini_file_data *)vmalloc(sizeof(struct ini_file_data) * PARSER_MAX_KEY_NUM);
		if (ERR_ALLOC_MEM(ini_info)) {
			ILI_ERR("Failed to malloc ini_info\n");
			ret = -EMP_NOMEM;
			goto out;
		}
	}

	ret = ilitek_tddi_mp_ini_parser(ilits->md_ini_path);
	if (ret < 0) {
		ILI_ERR("Failed to parsing INI file\n");
		ret = -EMP_INI;
				goto out;
			}

	parser_get_int_data("sram test", "free run tpu clock enable", str, sizeof(str));
	ilits->sram_para.tpu_clock_en = ili_katoi(str);

	ret = parser_get_int_data("sram test", "sram group enable", str, sizeof(str));
	ilits->sram_para.sram_group_en = ili_katoi(str);

	if (ret <= 0) {
		ILI_ERR("Mp ini not support sram test, Please update tool version.\n");
		goto out;
	}

	parser_get_int_data("sram test", "check fail bus enable", str, sizeof(str));
	ilits->sram_para.check_failbus = ili_katoi(str);

	parser_get_int_data("sram test", "max addr", str, sizeof(str));
	parser_get_u32_array(str, &ilits->sram_para.MaxAddr, 16, sizeof(ilits->sram_para.MaxAddr));

	parser_get_int_data("sram test", "min addr", str, sizeof(str));
	parser_get_u32_array(str, &ilits->sram_para.MinAddr, 16, sizeof(ilits->sram_para.MinAddr));

	ILI_INFO("Free Run TPU Clock EN : %s, Sram Group EN : %s, Check Fail Bus EN : %s\n", ilits->sram_para.tpu_clock_en ? "ENABLE" : "DISABLE", ilits->sram_para.sram_group_en ? "ENABLE" : "DISABLE", ilits->sram_para.check_failbus ? "ENABLE" : "DISABLE");
	ILI_INFO("MaxAddr = 0x%X, MinAddr = 0x%X\n", ilits->sram_para.MaxAddr, ilits->sram_para.MinAddr);

	if (ilits->sram_para.sram_group_en == ENABLE) {
		parser_get_int_data("sram test", "sram group", str, sizeof(str));
		ilits->sram_para.group_num = parser_sramtest_group(str, ilits->sram_para.strSramPartialArea);

		parser_get_int_data("sram test", "group write addr", str, sizeof(str));
		parser_get_u32_array(str, &ilits->sram_para.write_addr, 16, sizeof(ilits->sram_para.write_addr));

		parser_get_int_data("sram test", "group write", str, sizeof(str));
		parser_get_u32_array(str, ilits->sram_para.SramPartialAreaTest_en, 16, sizeof(ilits->sram_para.SramPartialAreaTest_en));

		parser_get_int_data("sram test", "group read addr", str, sizeof(str));
		parser_get_u32_array(str, &ilits->sram_para.read_addr, 16, sizeof(ilits->sram_para.read_addr));

		parser_get_int_data("sram test", "group result", str, sizeof(str));
		parser_get_u32_array(str, ilits->sram_para.SramResult, 16, sizeof(ilits->sram_para.SramResult));

		ILI_INFO("Group Number : %d, Write Addr = 0x%X, Read Addr = 0x%X\n", ilits->sram_para.group_num, ilits->sram_para.write_addr, ilits->sram_para.read_addr);

		for( i = 0; i < ilits->sram_para.group_num; i++) {
			ILI_INFO("Sram Partial Area : %s, Sram Partial Area Test en = 0x%X, Sram Result = 0x%X\n", ilits->sram_para.strSramPartialArea[i], ilits->sram_para.SramPartialAreaTest_en[i], ilits->sram_para.SramResult[i]);
		}
	}

out:
	ipio_vfree((void **)&ini_info);
	return ret;
}

static int mp_get_timing_info(void)
{
	int slen = 0;
	char str[256] = {0};
	u8 info[64] = {0};
	char *key = "timing_info_raw";

	core_mp.isLongV = 0;

	slen = parser_get_int_data("pv5_4 command", key, str, sizeof(str));
	if (slen < 0)
		return -1;

	if (parser_get_u8_array(str, info, 16, slen) < 0)
		return -1;

	core_mp.isLongV = info[6];

	ILI_INFO("DDI Mode = %s\n", (core_mp.isLongV ? "Long V" : "Long H"));

	return 0;
}

static int mp_test_data_sort_average(s32 *oringin_data, int index, s32 *avg_result)
{
	int i, j, k, x, y, len = 5, size;
	s32 u32temp;
	int u32up_frame, u32down_frame;
	s32 *u32sum_raw_data = NULL;
	s32 *u32data_buff = NULL;
	int ret = 0;

	if (tItems[index].frame_count <= 1) {
		ret = 0;
		goto out;
	}

	if (ERR_ALLOC_MEM(oringin_data)) {
		ILI_ERR("Input wrong address\n");
		ret = -ENOMEM;
		goto out;
	}

	u32data_buff = kcalloc(core_mp.frame_len * tItems[index].frame_count, sizeof(s32), GFP_KERNEL);
	if (ERR_ALLOC_MEM(u32data_buff)) {
		ILI_ERR("Failed to allocate u32data_buff FRAME buffer\n");
		ret = -ENOMEM;
		goto out;
	}

	u32sum_raw_data = kcalloc(core_mp.frame_len, sizeof(s32), GFP_KERNEL);
	if (ERR_ALLOC_MEM(u32data_buff)) {
		ILI_ERR("Failed to allocate u32sum_raw_data FRAME buffer\n");
		ret = -ENOMEM;
		goto out;
	}

	size = core_mp.frame_len * tItems[index].frame_count;
	for (i = 0; i < size; i++) {
		u32data_buff[i] = oringin_data[i];
	}

	u32up_frame = tItems[index].frame_count * tItems[index].highest_percentage / 100;
	u32down_frame = tItems[index].frame_count * tItems[index].lowest_percentage / 100;
	ILI_DBG("Up=%d, Down=%d -%s\n", u32up_frame, u32down_frame, tItems[index].desp);

	if (debug_en) {
		pr_cont("\n[Show Original frist%d and last%d node data]\n", len, len);
		for (i = 0; i < core_mp.frame_len; i++) {
			for (j = 0 ; j < tItems[index].frame_count ; j++) {
				if ((i < len) || (i >= (core_mp.frame_len-len)))
					pr_cont("%d,", u32data_buff[j * core_mp.frame_len + i]);
			}
			if ((i < len) || (i >= (core_mp.frame_len-len)))
				pr_cont("\n");
		}
	}

	for (i = 0; i < core_mp.frame_len; i++) {
		for (j = 0; j < tItems[index].frame_count-1; j++) {
			for (k = 0; k < (tItems[index].frame_count-1-j); k++) {
				x = i+k*core_mp.frame_len;
				y = i+(k+1)*core_mp.frame_len;
				if (*(u32data_buff+x) > *(u32data_buff+y)) {
					u32temp = *(u32data_buff+x);
					*(u32data_buff+x) = *(u32data_buff+y);
					*(u32data_buff+y) = u32temp;
				}
			}
		}
	}

	if (debug_en) {
		pr_cont("\n[After sorting frist%d and last%d node data]\n", len, len);
		for (i = 0; i < core_mp.frame_len; i++) {
			for (j = u32down_frame; j < tItems[index].frame_count - u32up_frame; j++) {
				if ((i < len) || (i >= (core_mp.frame_len - len)))
					pr_cont("%d,", u32data_buff[i + j * core_mp.frame_len]);
			}
			if ((i < len) || (i >= (core_mp.frame_len-len)))
				pr_cont("\n");
		}
	}

	for (i = 0 ; i < core_mp.frame_len ; i++) {
		u32sum_raw_data[i] = 0;
		for (j = u32down_frame; j < tItems[index].frame_count - u32up_frame; j++)
			u32sum_raw_data[i] += u32data_buff[i + j * core_mp.frame_len];

		avg_result[i] = u32sum_raw_data[i] / (tItems[index].frame_count - u32down_frame - u32up_frame);
	}

	if (debug_en) {
		pr_cont("\n[Average result frist%d and last%d node data]\n", len, len);
		for (i = 0; i < core_mp.frame_len; i++) {
			if ((i < len) || (i >= (core_mp.frame_len-len)))
				pr_cont("%d,", avg_result[i]);
		}
		if ((i < len) || (i >= (core_mp.frame_len-len)))
			pr_cont("\n");
	}

out:
	ipio_kfree((void **)&u32data_buff);
	ipio_kfree((void **)&u32sum_raw_data);
	return ret;
}

static void mp_compare_cdc_result(int index, s32 *tmp, s32 *max_ts, s32 *min_ts, int *result)
{
	int i, cdclen;

	if (tItems[index].catalog == TIPRING_RAW || tItems[index].catalog == TIPRING_P2P)
		cdclen = core_mp.tipring_len;
	else
		cdclen = core_mp.frame_len;

	if (ERR_ALLOC_MEM(tmp)) {
		ILI_ERR("The data of test item is null (%p)\n", tmp);
		*result = MP_DATA_FAIL;
		return;
	}

	if (tItems[index].catalog == SHORT_TEST) {
		for (i = 0; i < core_mp.frame_len; i++) {
			if (tmp[i] < min_ts[i]) {
				*result = MP_DATA_FAIL;
				return;
			}
		}
	} else {
		for (i = 0; i < cdclen; i++) {
			if (tmp[i] > max_ts[i] || tmp[i] < min_ts[i]) {
				ILI_DBG("Fail No.%d: max=%d, val=%d, min=%d\n", i, max_ts[i], tmp[i], min_ts[i]);
				*result = MP_DATA_FAIL;
				return;
			}
		}
	}
}

static int mp_compare_test_result(int index)
{
	int i, test_result = MP_DATA_PASS, cdclen;
	s32 *max_threshold = NULL, *min_threshold = NULL;

	if (tItems[index].catalog == PIN_TEST)
		return tItems[index].item_result;

	max_threshold = kcalloc(core_mp.frame_len, sizeof(s32), GFP_KERNEL);
	if (ERR_ALLOC_MEM(max_threshold)) {
		ILI_ERR("Failed to allocate threshold FRAME buffer\n");
		test_result = MP_DATA_FAIL;
		goto out;
	}

	min_threshold = kcalloc(core_mp.frame_len, sizeof(s32), GFP_KERNEL);
	if (ERR_ALLOC_MEM(min_threshold)) {
		ILI_ERR("Failed to allocate threshold FRAME buffer\n");
		test_result = MP_DATA_FAIL;
		goto out;
	}

	if (tItems[index].catalog == TIPRING_RAW)
		cdclen = core_mp.tipring_len;
	else
		cdclen = core_mp.frame_len;

	/* Show test result as below */
	if (tItems[index].catalog == TX_RX_DELTA) {
		if (ERR_ALLOC_MEM(core_mp.rx_delta_buf) || ERR_ALLOC_MEM(core_mp.tx_delta_buf)) {
			ILI_ERR("This test item (%s) has no data inside its buffer\n", tItems[index].desp);
			test_result = MP_DATA_FAIL;
			goto out;
		}

		for (i = 0; i < core_mp.frame_len; i++) {
			max_threshold[i] = core_mp.TxDeltaMax;
			min_threshold[i] = core_mp.TxDeltaMin;
		}
		mp_compare_cdc_result(index, core_mp.tx_max_buf, max_threshold, min_threshold, &test_result);
		mp_compare_cdc_result(index, core_mp.tx_min_buf, max_threshold, min_threshold, &test_result);

		for (i = 0; i < core_mp.frame_len; i++) {
			max_threshold[i] = core_mp.RxDeltaMax;
			min_threshold[i] = core_mp.RxDeltaMin;
		}

		mp_compare_cdc_result(index, core_mp.rx_max_buf, max_threshold, min_threshold, &test_result);
		mp_compare_cdc_result(index, core_mp.rx_min_buf, max_threshold, min_threshold, &test_result);
	} else {
		if (ERR_ALLOC_MEM(tItems[index].buf) || ERR_ALLOC_MEM(tItems[index].max_buf) ||
				ERR_ALLOC_MEM(tItems[index].min_buf) || ERR_ALLOC_MEM(tItems[index].result_buf)) {
			ILI_ERR("This test item (%s) has no data inside its buffer\n", tItems[index].desp);
			test_result = MP_DATA_FAIL;
			goto out;
		}

		if (tItems[index].spec_option == BENCHMARK) {
			if (tItems[index].catalog == PEAK_TO_PEAK_TEST || tItems[index].catalog == TIPRING_P2P) {
				for (i = 0; i < cdclen; i++) {
					max_threshold[i] = tItems[index].bch_mrk_max[0][i];
					min_threshold[i] = tItems[index].bch_mrk_min[0][i];
				}
			} else {
				for (i = 0; i < cdclen; i++) {
					max_threshold[i] = tItems[index].bench_mark_max[i];
					min_threshold[i] = tItems[index].bench_mark_min[i];
				}
			}
		} else {
			if (tItems[index].catalog == TIPRING_RAW || tItems[index].catalog == TIPRING_P2P) {
				for (i = 0; i < core_mp.nPxRaw_tipring_len; i++) {
					max_threshold[i] = tItems[index].max1;
					min_threshold[i] = tItems[index].min1;
				}
				for (i = core_mp.nPxRaw_tipring_len; i < core_mp.tipring_len; i++) {
					max_threshold[i] = tItems[index].max2;
					min_threshold[i] = tItems[index].min2;
				}
			} else {
				for (i = 0; i < cdclen; i++) {
					max_threshold[i] = tItems[index].max;
					min_threshold[i] = tItems[index].min;
				}
			}
		}

		/* general result */
		if (tItems[index].trimmed_mean && tItems[index].catalog != PEAK_TO_PEAK_TEST) {
			mp_test_data_sort_average(tItems[index].buf, index, tItems[index].result_buf);
			mp_compare_cdc_result(index, tItems[index].result_buf, max_threshold, min_threshold, &test_result);
		} else {
			if (tItems[index].bch_mrk_multi) {
				for (i = 0; i < tItems[index].bch_mrk_frm_num; i++) {
					mp_compare_cdc_result(index, frm_buf[i], tItems[index].bch_mrk_max[i], tItems[index].bch_mrk_min[i], &test_result);
				}
			} else {
				mp_compare_cdc_result(index, tItems[index].max_buf, max_threshold, min_threshold, &test_result);
				mp_compare_cdc_result(index, tItems[index].min_buf, max_threshold, min_threshold, &test_result);
			}
		}
	}

out:
	ipio_kfree((void **)&max_threshold);
	ipio_kfree((void **)&min_threshold);
	if (frm_buf != NULL) {
		for (i = 0; i < tItems[index].bch_mrk_frm_num; i++)
			if (frm_buf[i] != NULL) {
				kfree(frm_buf[i]);
				frm_buf[i] = NULL;
			}
		kfree(frm_buf);
		frm_buf = NULL;
	}
	if (ilits->eng_flow == true) {
		test_result = MP_DATA_PASS;
	}
	tItems[index].item_result = test_result;
	return test_result;
}

static void mp_do_retry(int index, int count)
{
	if (count == 0) {
		ILI_INFO("Finish retry action\n");
		return;
	}

	ILI_INFO("retry = %d, item = %s\n", count, tItems[index].desp);

	tItems[index].do_test(index);

	if (mp_compare_test_result(index) < 0)
		return mp_do_retry(index, count - 1);
}

static int mp_show_result(bool lcm_on)
{
	int ret = MP_DATA_PASS, seq;
	int i, x, y, j, csv_len = 0, pass_item_count = 0, line_count = 0, get_frame_cont = 1;
	s32 *max_threshold = NULL, *min_threshold = NULL;
	char *csv = NULL;
	char *ret_pass_name = NULL, *ret_fail_name = NULL;
#if (!GENERIC_KERNEL_IMAGE)
	struct file *f = NULL;
#if LINUX_VERSION_CODE <= KERNEL_VERSION(4, 14, 0)
	mm_segment_t fs;
#endif
	loff_t pos;
#endif
	csv = vmalloc(CSV_FILE_SIZE);
	if (ERR_ALLOC_MEM(csv)) {
		ILI_ERR("Failed to allocate CSV mem\n");
		ret = -EMP_NOMEM;
		goto fail_open;
	}

	max_threshold = kcalloc(core_mp.frame_len, sizeof(s32), GFP_KERNEL);
	min_threshold = kcalloc(core_mp.frame_len, sizeof(s32), GFP_KERNEL);
	if (ERR_ALLOC_MEM(max_threshold) || ERR_ALLOC_MEM(min_threshold)) {
		ILI_ERR("Failed to allocate threshold FRAME buffer\n");
		ret = -EMP_NOMEM;
		goto fail_open;
	}

	if (parser_get_ini_key_value("pv5_4 command", "date", core_mp.ini_date) < 0)
		ipio_memcpy(core_mp.ini_date, "Unknown", strlen("Unknown"), strlen("Unknown"));
	if (parser_get_ini_key_value("pv5_4 command", "version", core_mp.ini_ver) < 0)
		ipio_memcpy(core_mp.ini_ver, "Unknown", strlen("Unknown"), strlen("Unknown"));

	mp_print_csv_header(csv, &csv_len, &line_count, CSV_FILE_SIZE);

	for (seq = 0; seq < ri.count; seq++) {
		i = ri.index[seq];
		get_frame_cont = 1;

		if (tItems[i].item_result == MP_DATA_PASS) {
			pr_info("\n[%s],OK \n\n", tItems[i].desp);
			csv_len += snprintf(csv + csv_len, (CSV_FILE_SIZE - csv_len), "\n[%s],OK\n\n", tItems[i].desp);
		} else {
			pr_info("\n[%s],NG \n\n", tItems[i].desp);
			csv_len += snprintf(csv + csv_len, (CSV_FILE_SIZE - csv_len), "\n[%s],NG\n\n", tItems[i].desp);
		}

		if (tItems[i].catalog == PIN_TEST) {
			pr_info("Test INT Pin = %d\n", tItems[i].test_int_pin);
			csv_len += snprintf(csv + csv_len, (CSV_FILE_SIZE - csv_len), "Test INT Pin = %d\n", tItems[i].test_int_pin);
			pr_info("Pulse Test = %d\n", tItems[i].int_pulse_test);
			csv_len += snprintf(csv + csv_len, (CSV_FILE_SIZE - csv_len), "Pulse Test = %d\n", tItems[i].int_pulse_test);
			pr_info("Delay Time = %d\n", tItems[i].delay_time);
			csv_len += snprintf(csv + csv_len, (CSV_FILE_SIZE - csv_len), "Delay Time = %d\n", tItems[i].delay_time);
			continue;
		}

		mp_print_csv_cdc_cmd(csv, &csv_len, i, CSV_FILE_SIZE);

		pr_info("Frame count = %d\n", tItems[i].frame_count);
		csv_len += snprintf(csv + csv_len, (CSV_FILE_SIZE - csv_len), "Frame count = %d\n", tItems[i].frame_count);

		if (tItems[i].trimmed_mean && tItems[i].catalog != PEAK_TO_PEAK_TEST) {
			pr_info("lowest percentage = %d\n", tItems[i].lowest_percentage);
			csv_len += snprintf(csv + csv_len, (CSV_FILE_SIZE - csv_len), "lowest percentage = %d\n", tItems[i].lowest_percentage);

			pr_info("highest percentage = %d\n", tItems[i].highest_percentage);
			csv_len += snprintf(csv + csv_len, (CSV_FILE_SIZE - csv_len), "highest percentage = %d\n", tItems[i].highest_percentage);
		}

		/* Show result of benchmark max and min */
		if (tItems[i].spec_option == BENCHMARK) {
			if (tItems[i].catalog == PEAK_TO_PEAK_TEST) {
				for (j = 0; j < core_mp.frame_len; j++) {
					max_threshold[j] = tItems[i].bch_mrk_max[0][j];
					min_threshold[j] = tItems[i].bch_mrk_min[0][j];
				}

				for (j = 0; j < tItems[i].bch_mrk_frm_num; j++) {
					char bench_name[128] = {0};
					snprintf(bench_name, (CSV_FILE_SIZE - csv_len), "Max_Bench%d", (j+1));
					mp_compare_cdc_show_result(i, tItems[i].bch_mrk_max[j], csv, &csv_len, TYPE_BENCHMARK, max_threshold, min_threshold, bench_name, CSV_FILE_SIZE);
					snprintf(bench_name, (CSV_FILE_SIZE - csv_len), "Min_Bench%d", (j+1));
					mp_compare_cdc_show_result(i, tItems[i].bch_mrk_min[j], csv, &csv_len, TYPE_BENCHMARK, max_threshold, min_threshold, bench_name, CSV_FILE_SIZE);
				}
			} else if (tItems[i].catalog == TIPRING_RAW) {
				for (j = 0; j < core_mp.tipring_len; j++) {
					max_threshold[j] = tItems[i].bench_mark_max[j];
					min_threshold[j] = tItems[i].bench_mark_min[j];
				}
				mp_compare_tip_ring_cdc_show_result(i, tItems[i].bench_mark_max, csv, &csv_len, TYPE_BENCHMARK, max_threshold, min_threshold, "Max_Bench1", CSV_FILE_SIZE, TIPRING_X);
				mp_compare_tip_ring_cdc_show_result(i, tItems[i].bench_mark_min, csv, &csv_len, TYPE_BENCHMARK, max_threshold, min_threshold, "Min_Bench1", CSV_FILE_SIZE, TIPRING_X);
				mp_compare_tip_ring_cdc_show_result(i, tItems[i].bench_mark_max, csv, &csv_len, TYPE_BENCHMARK, max_threshold, min_threshold, "Max_Bench2", CSV_FILE_SIZE, TIPRING_Y);
				mp_compare_tip_ring_cdc_show_result(i, tItems[i].bench_mark_min, csv, &csv_len, TYPE_BENCHMARK, max_threshold, min_threshold, "Min_Bench2", CSV_FILE_SIZE, TIPRING_Y);
			} else if (tItems[i].catalog == TIPRING_P2P) {
				for (j = 0; j < core_mp.frame_len; j++) {
					max_threshold[j] = tItems[i].bch_mrk_max[0][j];
					min_threshold[j] = tItems[i].bch_mrk_min[0][j];
				}

				for (j = 0; j < tItems[i].bch_mrk_frm_num; j++) {
					mp_compare_tip_ring_cdc_show_result(i, tItems[i].bch_mrk_max[j], csv, &csv_len, TYPE_BENCHMARK,
							max_threshold, min_threshold, get_csv_frame_name("Max_Bench", j + 1), CSV_FILE_SIZE, TIPRING_X);
					mp_compare_tip_ring_cdc_show_result(i, tItems[i].bch_mrk_min[j], csv, &csv_len, TYPE_BENCHMARK,
							max_threshold, min_threshold, get_csv_frame_name("Min_Bench", j + 1), CSV_FILE_SIZE, TIPRING_X);
				}

				for (j = 0; j < tItems[i].bch_mrk_frm_num; j++) {
					mp_compare_tip_ring_cdc_show_result(i, tItems[i].bch_mrk_max[j], csv, &csv_len, TYPE_BENCHMARK,
							max_threshold, min_threshold, get_csv_frame_name("Max_Bench", tItems[i].bch_mrk_frm_num + j + 1), CSV_FILE_SIZE, TIPRING_Y);
					mp_compare_tip_ring_cdc_show_result(i, tItems[i].bch_mrk_min[j], csv, &csv_len, TYPE_BENCHMARK,
							max_threshold, min_threshold, get_csv_frame_name("Min_Bench", tItems[i].bch_mrk_frm_num + j + 1), CSV_FILE_SIZE, TIPRING_Y);
				}
			} else {
				for (j = 0; j < core_mp.frame_len; j++) {
					max_threshold[j] = tItems[i].bench_mark_max[j];
					min_threshold[j] = tItems[i].bench_mark_min[j];
				}
				mp_compare_cdc_show_result(i, tItems[i].bench_mark_max, csv, &csv_len, TYPE_BENCHMARK, max_threshold, min_threshold, "Max_Bench", CSV_FILE_SIZE);
				mp_compare_cdc_show_result(i, tItems[i].bench_mark_min, csv, &csv_len, TYPE_BENCHMARK, max_threshold, min_threshold, "Min_Bench", CSV_FILE_SIZE);
			}
		} else {
			if (tItems[i].catalog == TIPRING_RAW || tItems[i].catalog == TIPRING_P2P) {
				for (j = 0; j < core_mp.nPxRaw_tipring_len; j++) {
					max_threshold[j] = tItems[i].max1;
					min_threshold[j] = tItems[i].min1;
				}
				for (j = core_mp.nPxRaw_tipring_len; j < core_mp.tipring_len; j++) {
					max_threshold[j] = tItems[i].max2;
					min_threshold[j] = tItems[i].min2;
				}

				pr_info("Max1 = %d\n", tItems[i].max1);
				csv_len += snprintf(csv + csv_len, (CSV_FILE_SIZE - csv_len), "Max1 = %d\n", tItems[i].max1);

				pr_info("Max2 = %d\n", tItems[i].max2);
				csv_len += snprintf(csv + csv_len, (CSV_FILE_SIZE - csv_len), "Max2 = %d\n", tItems[i].max2);

				pr_info("Min1 = %d\n", tItems[i].min1);
				csv_len += snprintf(csv + csv_len, (CSV_FILE_SIZE - csv_len), "Min1 = %d\n", tItems[i].min1);

				pr_info("Min2 = %d\n", tItems[i].min2);
				csv_len += snprintf(csv + csv_len, (CSV_FILE_SIZE - csv_len), "Min2 = %d\n", tItems[i].min2);
			} else {
				for (j = 0; j < core_mp.frame_len; j++) {
					max_threshold[j] = tItems[i].max;
					min_threshold[j] = tItems[i].min;
				}

				pr_info("Max = %d\n", tItems[i].max);
				csv_len += snprintf(csv + csv_len, (CSV_FILE_SIZE - csv_len), "Max = %d\n", tItems[i].max);

				pr_info("Min = %d\n", tItems[i].min);
				csv_len += snprintf(csv + csv_len, (CSV_FILE_SIZE - csv_len), "Min = %d\n", tItems[i].min);
			}
		}

		if (ipio_strcmp(tItems[i].desp, "open test(integration)_sp") == 0) {
			mp_compare_cdc_show_result(i, frame1_cbk700, csv, &csv_len, TYPE_NO_JUGE, max_threshold, min_threshold, "frame1 cbk700", CSV_FILE_SIZE);
			mp_compare_cdc_show_result(i, frame1_cbk250, csv, &csv_len, TYPE_NO_JUGE, max_threshold, min_threshold, "frame1 cbk250", CSV_FILE_SIZE);
			mp_compare_cdc_show_result(i, frame1_cbk200, csv, &csv_len, TYPE_NO_JUGE, max_threshold, min_threshold, "frame1 cbk200", CSV_FILE_SIZE);
		}

		if (ipio_strcmp(tItems[i].desp, "open test_c") == 0) {
			mp_compare_cdc_show_result(i, cap_dac, csv, &csv_len, TYPE_NO_JUGE, max_threshold, min_threshold, "CAP_DAC", CSV_FILE_SIZE);
			mp_compare_cdc_show_result(i, cap_raw, csv, &csv_len, TYPE_NO_JUGE, max_threshold, min_threshold, "CAP_RAW", CSV_FILE_SIZE);
		}

		if (ipio_strcmp(tItems[i].desp, "open test_c (openx enable)") == 0) {
			mp_compare_cdc_show_result(i, openx_cap_dac, csv, &csv_len, TYPE_NO_JUGE, max_threshold, min_threshold, "CAP_DAC", CSV_FILE_SIZE);
			mp_compare_cdc_show_result(i, openx_cap_raw, csv, &csv_len, TYPE_NO_JUGE, max_threshold, min_threshold, "CAP_RAW", CSV_FILE_SIZE);
		}

		if (tItems[i].catalog == TX_RX_DELTA) {
			if (ERR_ALLOC_MEM(core_mp.rx_delta_buf) || ERR_ALLOC_MEM(core_mp.tx_delta_buf)) {
				ILI_ERR("This test item (%s) has no data inside its buffer\n", tItems[i].desp);
				continue;
			}
		} else {
			if (ERR_ALLOC_MEM(tItems[i].buf) || ERR_ALLOC_MEM(tItems[i].max_buf) ||
					ERR_ALLOC_MEM(tItems[i].min_buf)) {
				ILI_ERR("This test item (%s) has no data inside its buffer\n", tItems[i].desp);
				continue;
			}
		}

		/* Show test result as below */
		if (tItems[i].catalog == KEY_TEST) {
			for (x = 0; x < core_mp.key_len; x++) {
				DUMP("KEY_%02d ", x);
				csv_len += snprintf(csv + csv_len, (CSV_FILE_SIZE - csv_len), "KEY_%02d,", x);
			}

			DUMP("\n");
			csv_len += snprintf(csv + csv_len, (CSV_FILE_SIZE - csv_len), "\n");

			for (y = 0; y < core_mp.key_len; y++) {
				DUMP(" %3d   ", tItems[i].buf[y]);
				csv_len += snprintf(csv + csv_len, (CSV_FILE_SIZE - csv_len), " %3d, ", tItems[i].buf[y]);
			}

			DUMP("\n");
			csv_len += snprintf(csv + csv_len, (CSV_FILE_SIZE - csv_len), "\n");
		} else if (tItems[i].catalog == TX_RX_DELTA) {
			for (j = 0; j < core_mp.frame_len; j++) {
				max_threshold[j] = core_mp.TxDeltaMax;
				min_threshold[j] = core_mp.TxDeltaMin;
			}
			mp_compare_cdc_show_result(i, core_mp.tx_max_buf, csv, &csv_len, TYPE_JUGE, max_threshold, min_threshold, "TX Max Hold", CSV_FILE_SIZE);
			mp_compare_cdc_show_result(i, core_mp.tx_min_buf, csv, &csv_len, TYPE_JUGE, max_threshold, min_threshold, "TX Min Hold", CSV_FILE_SIZE);

			for (j = 0; j < core_mp.frame_len; j++) {
				max_threshold[j] = core_mp.RxDeltaMax;
				min_threshold[j] = core_mp.RxDeltaMin;
			}
			mp_compare_cdc_show_result(i, core_mp.rx_max_buf, csv, &csv_len, TYPE_JUGE, max_threshold, min_threshold, "RX Max Hold", CSV_FILE_SIZE);
			mp_compare_cdc_show_result(i, core_mp.rx_min_buf, csv, &csv_len, TYPE_JUGE, max_threshold, min_threshold, "RX Min Hold", CSV_FILE_SIZE);
		} else if (tItems[i].catalog == TIPRING_RAW || tItems[i].catalog == TIPRING_P2P) {
			if (tItems[i].catalog == TIPRING_RAW)
				get_frame_cont = tItems[i].frame_count;

			if (tItems[i].catalog == TIPRING_P2P)
				get_frame_cont = tItems[i].bch_mrk_frm_num;

			for (j = 0; j < get_frame_cont; j++) {
				mp_compare_tip_ring_cdc_show_result(i,&tItems[i].buf[(j * core_mp.tipring_len)], csv, &csv_len, TYPE_NO_JUGE,
						max_threshold, min_threshold, get_csv_frame_name("Frame ", j + 1), CSV_FILE_SIZE, TIPRING_X);
			}
			for (j = 0; j < get_frame_cont; j++) {
				mp_compare_tip_ring_cdc_show_result(i,&tItems[i].buf[(j * core_mp.tipring_len)], csv, &csv_len, TYPE_NO_JUGE,
						max_threshold, min_threshold, get_csv_frame_name("Frame ", get_frame_cont + j + 1), CSV_FILE_SIZE, TIPRING_Y);
			}
		} else {
			/* general result */
			if (tItems[i].trimmed_mean && tItems[i].catalog != PEAK_TO_PEAK_TEST) {
				mp_compare_cdc_show_result(i, tItems[i].result_buf, csv, &csv_len, TYPE_JUGE, max_threshold, min_threshold, "Mean result", CSV_FILE_SIZE);
			} else {
				if (!tItems[i].bch_mrk_multi) {
					mp_compare_cdc_show_result(i, tItems[i].max_buf, csv, &csv_len, TYPE_JUGE, max_threshold, min_threshold, "Max Hold", CSV_FILE_SIZE);
					mp_compare_cdc_show_result(i, tItems[i].min_buf, csv, &csv_len, TYPE_JUGE, max_threshold, min_threshold, "Min Hold", CSV_FILE_SIZE);
				}

			}
			if (tItems[i].catalog != PEAK_TO_PEAK_TEST)
				get_frame_cont = tItems[i].frame_count;

			if (tItems[i].bch_mrk_multi)
				get_frame_cont = tItems[i].bch_mrk_frm_num;

			/* result of each frame */
			for (j = 0; j < get_frame_cont; j++) {
				mp_compare_cdc_show_result(i, &tItems[i].buf[(j*core_mp.frame_len)], csv, &csv_len, TYPE_NO_JUGE, max_threshold, min_threshold, get_csv_frame_name("Frame ", j + 1), CSV_FILE_SIZE);
			}
		}
	}

	memset(csv_name, 0, 128 * sizeof(char));

	mp_print_csv_tail(csv, &csv_len, CSV_FILE_SIZE);

	for (i = 0; i < MP_TEST_ITEM; i++) {
		if (tItems[i].run) {
			if (tItems[i].item_result < 0) {
				pass_item_count = 0;
				break;
			}
			pass_item_count++;
		}
	}

	/* define csv file name */
	ret_pass_name = NORMAL_CSV_PASS_NAME;
	ret_fail_name = NORMAL_CSV_FAIL_NAME;

	if (pass_item_count == 0) {
		core_mp.final_result = MP_DATA_FAIL;
		ret = MP_DATA_FAIL;

		if (core_mp.lost_parameter) {
			ret_fail_name = NULL;
			ret_fail_name = NORMAL_CSV_WARNING_NAME;
			ILI_ERR("WARNING! OPEN or SHORT parameter not found in ini file!!\n");
		}

		if (lcm_on) {
#if LINUX_VERSION_CODE <= KERNEL_VERSION(4, 14, 0)
			snprintf(csv_name, (CSV_FILE_SIZE - csv_len), "%s/%s_%s.csv", CSV_LCM_ON_PATH, get_date_time_str(), ret_fail_name);
#else
			snprintf(csv_name, (CSV_FILE_SIZE - csv_len), "%s/lcm_on_%s_%s.csv", CSV_LCM_DEF_PATH, get_date_time_str(), ret_fail_name);
#endif
		} else {
#if LINUX_VERSION_CODE <= KERNEL_VERSION(4, 14, 0)
			snprintf(csv_name, (CSV_FILE_SIZE - csv_len), "%s/%s_%s.csv", CSV_LCM_OFF_PATH, get_date_time_str(), ret_fail_name);
#else
			snprintf(csv_name, (CSV_FILE_SIZE - csv_len), "%s/lcm_off_%s_%s.csv", CSV_LCM_DEF_PATH, get_date_time_str(), ret_fail_name);
#endif
		}
	} else {
		core_mp.final_result = MP_DATA_PASS;
		ret = MP_DATA_PASS;

		if (core_mp.lost_benchmark) {
			ret_pass_name = NULL;
			ret_pass_name = NORMAL_CSV_WARNING_NAME;
			ILI_ERR("WARNING! Golden and SPEC in ini file aren't matched!!\n");
		}

		if (lcm_on) {
#if LINUX_VERSION_CODE <= KERNEL_VERSION(4, 14, 0)
			snprintf(csv_name, (CSV_FILE_SIZE - csv_len), "%s/%s_%s.csv", CSV_LCM_ON_PATH, get_date_time_str(), ret_pass_name);
#else
			snprintf(csv_name, (CSV_FILE_SIZE - csv_len), "%s/lcm_on_%s_%s.csv", CSV_LCM_DEF_PATH, get_date_time_str(), ret_pass_name);
#endif
		} else {
#if LINUX_VERSION_CODE <= KERNEL_VERSION(4, 14, 0)
			snprintf(csv_name, (CSV_FILE_SIZE - csv_len), "%s/%s_%s.csv", CSV_LCM_OFF_PATH, get_date_time_str(), ret_pass_name);
#else
			snprintf(csv_name, (CSV_FILE_SIZE - csv_len), "%s/lcm_off_%s_%s.csv", CSV_LCM_DEF_PATH, get_date_time_str(), ret_pass_name);
#endif
		}
	}

#if GENERIC_KERNEL_IMAGE
	ILI_ERR("GKI version not allow drivers to use filp_open\n");
	if (ilits->mp_result == NULL) {
		ilits->mp_result = vmalloc(CSV_FILE_SIZE);
		if (ERR_ALLOC_MEM(ilits->mp_result)) {
			ILI_ERR("Failed to allocate ilits->mp_result mem\n");
			ret = -EMP_NOMEM;
			goto fail_open;
		}
	}

	memset(ilits->mp_result, 0, CSV_FILE_SIZE);
	ipio_memcpy(ilits->mp_result, csv, csv_len, strlen(csv));
#else
	ILI_INFO("Open CSV : %s\n", csv_name);
	if (f == NULL)
		f = filp_open(csv_name, O_WRONLY | O_CREAT | O_TRUNC, 644);

	if (ERR_ALLOC_MEM(f)) {
		ILI_ERR("Failed to open CSV file");
		ret = -EMP_NOMEM;
		goto fail_open;
	}

	ILI_INFO("Open CSV succeed, its length = %d\n ", csv_len);

	if (csv_len >= CSV_FILE_SIZE) {
		ILI_ERR("The length saved to CSV is too long !\n");
		ret = -EMP_INVAL;
		goto fail_open;
	}

	pos = 0;
#if LINUX_VERSION_CODE <= KERNEL_VERSION(4, 14, 0)
	fs = get_fs();
	set_fs(KERNEL_DS);
	vfs_write(f, csv, csv_len, &pos);
	set_fs(fs);
#else
	kernel_write(f, csv, csv_len, &pos);
#endif
	filp_close(f, NULL);

	ILI_INFO("Writing Data into CSV succeed\n");
#endif

fail_open:
	ipio_vfree((void **)&csv);
	ipio_kfree((void **)&max_threshold);
	ipio_kfree((void **)&min_threshold);
	return ret;
}

static void ilitek_tddi_mp_init_item(void)
{
	int i = 0;

	memset(&core_mp, 0, sizeof(core_mp));
	memset(&open_para, 0, sizeof(open_para));
	memset(&short_para, 0, sizeof(short_para));

	core_mp.chip_pid = ilits->chip->pid;
	core_mp.chip_id = ilits->chip->id;
	core_mp.chip_type = ilits->chip->type;
	core_mp.chip_ver = ilits->chip->ver;
	core_mp.fw_ver = ilits->chip->fw_ver;
	core_mp.protocol_ver = ilits->protocol->ver;
	core_mp.core_ver = ilits->chip->core_ver;
	core_mp.cdc_len = ilits->protocol->cdc_len;
	core_mp.no_bk_shift = ilits->chip->no_bk_shift;
	core_mp.xch_len = ilits->xch_num;
	core_mp.ych_len = ilits->ych_num;

	if (ilits->PenType == POSITION_PEN_TYPE_ON) {
		core_mp.nPxRaw = ilits->pen_info_block.nPxRaw;
		core_mp.nPyRaw = ilits->pen_info_block.nPyRaw;
		core_mp.nPxRaw_tipring_len = 2 * core_mp.nPxRaw * core_mp.xch_len;
		core_mp.nPyRaw_tipring_len = 2 * core_mp.nPyRaw * core_mp.ych_len;
		core_mp.tipring_len = core_mp.nPxRaw_tipring_len + core_mp.nPyRaw_tipring_len;
	}

	core_mp.frame_len = core_mp.xch_len * core_mp.ych_len;
	core_mp.stx_len = 0;
	core_mp.srx_len = 0;
	core_mp.key_len = 0;
	core_mp.st_len = 0;
	core_mp.tdf = 240;
	core_mp.busy_cdc = INT_CHECK;
	core_mp.retry = ilits->mp_retry;
	core_mp.td_retry = false;
	core_mp.final_result = MP_DATA_FAIL;
	core_mp.lost_benchmark = false;
	core_mp.lost_parameter = false;
	core_mp.pre_cmd_sp = false;

	ILI_INFO("============== TP & Panel info ================\n");
	ILI_INFO("Driver version = %s\n", DRIVER_VERSION);
	ILI_INFO("TP Module = %s\n", ilits->md_name);
	ILI_INFO("CHIP = 0x%x\n", core_mp.chip_pid);
	ILI_INFO("Firmware version = %x\n", core_mp.fw_ver);
	ILI_INFO("Protocol version = %x\n", core_mp.protocol_ver);
	ILI_INFO("Core version = %x\n", core_mp.core_ver);
	ILI_INFO("Read CDC Length = %d\n", core_mp.cdc_len);
	ILI_INFO("X length = %d, Y length = %d\n", core_mp.xch_len, core_mp.ych_len);
	ILI_INFO("Frame length = %d\n", core_mp.frame_len);

	if (ilits->PenType == POSITION_PEN_TYPE_ON) {
		ILI_INFO("nPxRaw = %d,nPyRaw = %d\n", core_mp.nPxRaw, core_mp.nPyRaw);
		ILI_INFO("Tip Ring length = %d, nPxRaw buffer length = %d, nPyRaw buffer length = %d\n"
			, core_mp.tipring_len, core_mp.nPxRaw_tipring_len, core_mp.nPyRaw_tipring_len);
	}

	ILI_INFO("Check busy method = %s\n", (core_mp.busy_cdc ? "Polling" : "Interrupt"));
	ILI_INFO("===============================================\n");

	for (i = 0; i < MP_TEST_ITEM; i++) {
		tItems[i].spec_option = 0;
		tItems[i].type_option = 0;
		tItems[i].run = false;
		tItems[i].max = 0;
		tItems[i].max1 = 0;
		tItems[i].max2 = 0;
		tItems[i].max_res = MP_DATA_FAIL;
		tItems[i].item_result = MP_DATA_PASS;
		tItems[i].min = 0;
		tItems[i].min1 = 0;
		tItems[i].min2 = 0;
		tItems[i].min_res = MP_DATA_FAIL;
		tItems[i].frame_count = 0;
		tItems[i].trimmed_mean = 0;
		tItems[i].lowest_percentage = 0;
		tItems[i].highest_percentage = 0;
		tItems[i].v_tdf_1 = 0;
		tItems[i].v_tdf_2 = 0;
		tItems[i].h_tdf_1 = 0;
		tItems[i].h_tdf_2 = 0;
		tItems[i].max_min_mode = 0;
		tItems[i].bch_mrk_multi = false;
		tItems[i].bch_mrk_frm_num = 1;
		tItems[i].goldenmode = 0;
		tItems[i].retry_cnt = RETRY_COUNT;
		tItems[i].result_buf = NULL;
		tItems[i].buf = NULL;
		tItems[i].max_buf = NULL;
		tItems[i].min_buf = NULL;
		tItems[i].bench_mark_max = NULL;
		tItems[i].bench_mark_min = NULL;
		tItems[i].bch_mrk_max = NULL;
		tItems[i].bch_mrk_min = NULL;
		tItems[i].node_type = NULL;
		tItems[i].delay_time = 0;
		tItems[i].test_int_pin = 0;
		tItems[i].int_pulse_test = 0;
		tItems[i].pre_cmd_en = DISABLE;

		if (tItems[i].catalog == MUTUAL_TEST) {
			tItems[i].do_test = mutual_test;
		} else if (tItems[i].catalog == TX_RX_DELTA) {
			tItems[i].do_test = mutual_test;
		} else if (tItems[i].catalog == UNTOUCH_P2P) {
			tItems[i].do_test = mutual_test;
		} else if (tItems[i].catalog == PIXEL) {
			tItems[i].do_test = mutual_test;
		} else if (tItems[i].catalog == OPEN_TEST) {
			if (ipio_strcmp(tItems[i].desp, "open test(integration)_sp") == 0)
				tItems[i].do_test = open_test_sp;
			else if (ipio_strcmp(tItems[i].desp, "open test_c") == 0)
				tItems[i].do_test = open_test_cap;
			else if (ipio_strcmp(tItems[i].desp, "open test_c (openx enable)") == 0)
				tItems[i].do_test = open_test_cap;
			else
				tItems[i].do_test = mutual_test;
		} else if (tItems[i].catalog == KEY_TEST) {
			tItems[i].do_test = key_test;
		} else if (tItems[i].catalog == SELF_TEST) {
			tItems[i].do_test = self_test;
		} else if (tItems[i].catalog == ST_TEST) {
			tItems[i].do_test = st_test;
		} else if (tItems[i].catalog == PEAK_TO_PEAK_TEST) {
			tItems[i].do_test = peak_to_peak_test;
		} else if (tItems[i].catalog == SHORT_TEST) {
			tItems[i].do_test = mutual_test;
		} else if (tItems[i].catalog == PIN_TEST) {
			tItems[i].do_test = pin_test;
		} else if (tItems[i].catalog == TIPRING_RAW) {
			tItems[i].do_test = mutual_test;
		} else if (tItems[i].catalog == TIPRING_P2P) {
			tItems[i].do_test = peak_to_peak_test;
		}

		tItems[i].result = kmalloc(16, GFP_KERNEL);
		snprintf(tItems[i].result, 16, "%s", "FAIL");
	}
}

static void ilitek_tddi_fw_mp_precmd_support(void)
{
	int ret = 0;
	u8 cmd[2] = {0};
	u8 buftmp[3] = {0};

	core_mp.pre_cmd_sp = false;

	if (core_mp.core_ver < CORE_VER_1600) {
		ILI_INFO("FW no support MP pre command\n");
		core_mp.pre_cmd_sp = false;
		return;
	}

	cmd[0] = CMD_FW_MP_PRE_CMD;
	cmd[1] = 0x02;

	ret = ilits->wrapper(cmd, 2, buftmp, 3, ON, OFF);
	if (ret < 0)
		ILI_ERR("check FW pre command failed\n");

	if (buftmp[2] == 0xAA)
		core_mp.pre_cmd_sp = true;

	ILI_INFO("FW support MP Pre-Command : %s\n", core_mp.pre_cmd_sp ? "ENABLE" : "DISABLE");

}

static void mp_p2p_td_retry_after_ra_fail(int p2p_td)
{
	int i;

	for (i = 0; i < MP_TEST_ITEM; i++) {
		if (ipio_strcmp(tItems[i].desp, "noise peak to peak(with panel) (lcm off)") == 0)
			break;
	}

	if (i >= MP_TEST_ITEM)
		return;

	ILI_DBG("i = %d, p2p_noise_ret = %d, p2p_noise_run = %d\n",
		i, tItems[i].item_result, tItems[i].run);

	if (tItems[i].item_result == MP_DATA_PASS && tItems[i].run == 1)
		tItems[p2p_td].do_test(p2p_td);
}

static void mp_test_run(int index)
{
	int i = index, j;
	char str[512] = {0};


	/* Get parameters from ini */
	parser_get_int_data(tItems[i].desp, "spec option", str, sizeof(str));
	tItems[i].spec_option = ili_katoi(str);
	parser_get_int_data(tItems[i].desp, "type option", str, sizeof(str));
	tItems[i].type_option = ili_katoi(str);
	parser_get_int_data(tItems[i].desp, "frame count", str, sizeof(str));
	tItems[i].frame_count = ili_katoi(str);
	parser_get_int_data(tItems[i].desp, "trimmed mean", str, sizeof(str));
	tItems[i].trimmed_mean = ili_katoi(str);
	parser_get_int_data(tItems[i].desp, "lowest percentage", str, sizeof(str));
	tItems[i].lowest_percentage = ili_katoi(str);
	parser_get_int_data(tItems[i].desp, "highest percentage", str, sizeof(str));
	tItems[i].highest_percentage = ili_katoi(str);
	parser_get_int_data(tItems[i].desp, "goldenmode", str, sizeof(str));
	tItems[i].goldenmode = ili_katoi(str);
	parser_get_int_data(tItems[i].desp, "max min mode", str, sizeof(str));
	tItems[i].max_min_mode = ili_katoi(str);

	parser_get_int_data(tItems[i].desp, "retry count", str, sizeof(str));
	tItems[i].retry_cnt = ili_katoi(str);

	if (tItems[i].goldenmode && (tItems[i].spec_option != tItems[i].goldenmode))
		core_mp.lost_benchmark = true;

	if (core_mp.pre_cmd_sp) {
		parser_get_int_data(tItems[i].desp, "precommand", str, sizeof(str));
		tItems[i].pre_cmd_en = ili_katoi(str);
	}

	/* Get pin test delay time */
	if (tItems[i].catalog == PIN_TEST) {
		parser_get_int_data(tItems[i].desp, "test int pin", str, sizeof(str));
		tItems[i].test_int_pin = ili_katoi(str);
		parser_get_int_data(tItems[i].desp, "int pulse test", str, sizeof(str));
		tItems[i].int_pulse_test = ili_katoi(str);
		parser_get_int_data(tItems[i].desp, "delay time", str, sizeof(str));
		tItems[i].delay_time = ili_katoi(str);
	}

	/* Get TDF value from ini */
	if (tItems[i].catalog == SHORT_TEST) {
		parser_get_int_data(tItems[i].desp, "v_tdf_1", str, sizeof(str));
		tItems[i].v_tdf_1 = parser_get_tdf_value(str, tItems[i].catalog);
		parser_get_int_data(tItems[i].desp, "v_tdf_2", str, sizeof(str));
		tItems[i].v_tdf_2 = parser_get_tdf_value(str, tItems[i].catalog);
		parser_get_int_data(tItems[i].desp, "h_tdf_1", str, sizeof(str));
		tItems[i].h_tdf_1 = parser_get_tdf_value(str, tItems[i].catalog);
		parser_get_int_data(tItems[i].desp, "h_tdf_2", str, sizeof(str));
		tItems[i].h_tdf_2 = parser_get_tdf_value(str, tItems[i].catalog);
	} else {
		parser_get_int_data(tItems[i].desp, "v_tdf", str, sizeof(str));
		tItems[i].v_tdf_1 = parser_get_tdf_value(str, tItems[i].catalog);
		parser_get_int_data(tItems[i].desp, "h_tdf", str, sizeof(str));
		tItems[i].h_tdf_1 = parser_get_tdf_value(str, tItems[i].catalog);
	}

	/* Get threshold from ini structure in parser */
	if (ipio_strcmp(tItems[i].desp, "tx/rx delta") == 0) {
		parser_get_int_data(tItems[i].desp, "tx max", str, sizeof(str));
		core_mp.TxDeltaMax = ili_katoi(str);
		parser_get_int_data(tItems[i].desp, "tx min", str, sizeof(str));
		core_mp.TxDeltaMin = ili_katoi(str);
		parser_get_int_data(tItems[i].desp, "rx max", str, sizeof(str));
		core_mp.RxDeltaMax = ili_katoi(str);
		parser_get_int_data(tItems[i].desp, "rx min", str, sizeof(str));
		core_mp.RxDeltaMin = ili_katoi(str);
		ILI_DBG("%s: Tx Max = %d, Tx Min = %d, Rx Max = %d,  Rx Min = %d\n",
				tItems[i].desp, core_mp.TxDeltaMax, core_mp.TxDeltaMin,
				core_mp.RxDeltaMax, core_mp.RxDeltaMin);
	} else {
		parser_get_int_data(tItems[i].desp, "max", str, sizeof(str));
		tItems[i].max = ili_katoi(str);
		parser_get_int_data(tItems[i].desp, "min", str, sizeof(str));
		tItems[i].min = ili_katoi(str);
	}

	if (tItems[i].catalog == TIPRING_RAW || tItems[i].catalog == TIPRING_P2P) {
		parser_get_int_data(tItems[i].desp, "max1", str, sizeof(str));
		tItems[i].max1 = ili_katoi(str);
		parser_get_int_data(tItems[i].desp, "max2", str, sizeof(str));
		tItems[i].max2 = ili_katoi(str);
		parser_get_int_data(tItems[i].desp, "min1", str, sizeof(str));
		tItems[i].min1 = ili_katoi(str);
		parser_get_int_data(tItems[i].desp, "min2", str, sizeof(str));
		tItems[i].min2 = ili_katoi(str);
	}

	/* Get pin test delay time */
	if (tItems[i].catalog == PEAK_TO_PEAK_TEST) {
		if (tItems[i].max_min_mode == 1 && tItems[i].spec_option == 1) {
			tItems[i].bch_mrk_multi = true;
			for (j = 1; j < 11; j++) {
				char tmp[8] = {0};
				/* format complete string from the name of section "_Benchmark_Data". */
				snprintf(str, sizeof(str), "%s%s%s%d", tItems[index].desp, "_", BENCHMARK_KEY_NAME, j);
				if (parser_get_int_data(str, str, tmp, sizeof(tmp)) <= 0)
					break;

				tItems[i].bch_mrk_frm_num = j;
			}
			ILI_DBG("set bch_mrk_frm_num = %d\n", tItems[i].bch_mrk_frm_num);
		}
	}

	if (tItems[i].catalog == TIPRING_P2P && tItems[i].max_min_mode == 1) {
		tItems[i].bch_mrk_frm_num = 3;
		tItems[i].bch_mrk_multi = true;
		ILI_DBG("set bch_mrk_frm_num = %d\n", tItems[i].bch_mrk_frm_num);
	}

	ILI_DBG("%s: run = %d, max = %d, min = %d, frame_count = %d\n", tItems[i].desp,
			tItems[i].run, tItems[i].max, tItems[i].min, tItems[i].frame_count);

	ILI_DBG("v_tdf_1 = %d, v_tdf_2 = %d, h_tdf_1 = %d, h_tdf_2 = %d", tItems[i].v_tdf_1,
			tItems[i].v_tdf_2, tItems[i].h_tdf_1, tItems[i].h_tdf_2);

	if (tItems[i].catalog == TIPRING_RAW || tItems[i].catalog == TIPRING_P2P)
		ILI_DBG("max1 = %d, max2 = %d, min1 = %d, min2 = %d\n", tItems[i].max1, tItems[i].max2, tItems[i].min1, tItems[i].min2);

	ILI_INFO("Run MP Test Item : %s\n", tItems[i].desp);
	tItems[i].do_test(i);

	mp_compare_test_result(i);

	/* P2P TD retry after RA sample failed. */
	if (ipio_strcmp(tItems[i].desp, "peak to peak_td (lcm off)") == 0 &&
		tItems[i].item_result == MP_DATA_FAIL) {
		parser_get_int_data(tItems[i].desp, "recheck ptop lcm off", str, sizeof(str));
		ILI_INFO("Peak to Peak TD retry = %d\n", ili_katoi(str));
		core_mp.td_retry = ili_katoi(str);
		if (core_mp.td_retry)
			mp_p2p_td_retry_after_ra_fail(i);
	}

	if (core_mp.retry && tItems[i].item_result == MP_DATA_FAIL) {
		ILI_INFO("MP failed, doing retry %d times\n", tItems[i].retry_cnt);
		mp_do_retry(i, tItems[i].retry_cnt);
	}

}

static void mp_test_free(void)
{
	int i, j;

	ILI_INFO("Free all allocated mem for MP\n");

	core_mp.final_result = MP_DATA_FAIL;
	core_mp.td_retry = false;

	for (i = 0; i < MP_TEST_ITEM; i++) {
		tItems[i].run = false;
		tItems[i].max_res = MP_DATA_FAIL;
		tItems[i].min_res = MP_DATA_FAIL;
		tItems[i].item_result = MP_DATA_PASS;

		if (tItems[i].catalog == TX_RX_DELTA) {
				ipio_kfree((void **)&core_mp.rx_delta_buf);
				ipio_kfree((void **)&core_mp.tx_delta_buf);
				ipio_kfree((void **)&core_mp.tx_max_buf);
				ipio_kfree((void **)&core_mp.tx_min_buf);
				ipio_kfree((void **)&core_mp.rx_max_buf);
				ipio_kfree((void **)&core_mp.rx_min_buf);
		} else {
			if (tItems[i].spec_option == BENCHMARK) {
				ipio_kfree((void **)&tItems[i].bench_mark_max);
				ipio_kfree((void **)&tItems[i].bench_mark_min);
				if (tItems[i].bch_mrk_max != NULL) {
					for (j = 0; j < tItems[i].bch_mrk_frm_num; j++)
						if (tItems[i].bch_mrk_max[j] != NULL) {
							kfree(tItems[i].bch_mrk_max[j]);
							tItems[i].bch_mrk_max[j] = NULL;
						}
					kfree(tItems[i].bch_mrk_max);
					tItems[i].bch_mrk_max = NULL;
				}
				if (tItems[i].bch_mrk_min != NULL) {
					for (j = 0; j < tItems[i].bch_mrk_frm_num; j++)
						if (tItems[i].bch_mrk_min[j] != NULL) {
							kfree(tItems[i].bch_mrk_min[j]);
							tItems[i].bch_mrk_min[j] = NULL;
						}
					kfree(tItems[i].bch_mrk_min);
					tItems[i].bch_mrk_min = NULL;
				}
			}
			ipio_kfree((void **)&tItems[i].node_type);
			ipio_kfree((void **)&tItems[i].result);
			ipio_kfree((void **)&tItems[i].result_buf);
			ipio_kfree((void **)&tItems[i].max_buf);
			ipio_kfree((void **)&tItems[i].min_buf);
			ipio_vfree((void **)&tItems[i].buf);
		}
	}

	ipio_kfree((void **)&frame1_cbk700);
	ipio_kfree((void **)&frame1_cbk250);
	ipio_kfree((void **)&frame1_cbk200);
	ipio_kfree((void **)&frame_buf);
	ipio_kfree((void **)&key_buf);
	ipio_kfree((void **)&cap_dac);
	ipio_kfree((void **)&cap_raw);
	ipio_kfree((void **)&openx_cap_dac);
	ipio_kfree((void **)&openx_cap_raw);
	if (frm_buf != NULL) {
		for (j = 0; j < 3; j++)
			if (frm_buf[j] != NULL) {
				kfree(frm_buf[j]);
				frm_buf[j] = NULL;
			}
		kfree(frm_buf);
		frm_buf = NULL;
	}
}

/* The method to copy results to user depends on what APK needs */
static void mp_copy_ret_to_apk(char *buf)
{
	int i, seq, len = 2;

	if (!buf) {
		ILI_ERR("apk buffer is null\n");
		return;
	}
#if !GENERIC_KERNEL_IMAGE
	len += snprintf(buf + len, PAGE_SIZE - len, "CSV path: %s\n\n", csv_name);
#endif
	for (seq = 0; seq < ri.count; seq++) {
		i = ri.index[seq];
		if (tItems[i].item_result == MP_DATA_FAIL) {
			ILI_ERR("[%s] = FAIL\n", tItems[i].desp);
			len += snprintf(buf + len, PAGE_SIZE - len, "[%s] = FAIL\n", tItems[i].desp);
		} else {
			ILI_INFO("[%s] = PASS\n", tItems[i].desp);
			len += snprintf(buf + len, PAGE_SIZE - len, "[%s] = PASS\n", tItems[i].desp);
		}
	}
	ilits->mp_ret_len = len;
}

static int mp_sort_item(bool lcm_on)
{
	int i, j;
	char str[128] = {0};

	ri.count = 0;
	memset(ri.index, 0x0, sizeof(ri.index));

	for (i = 0; i < MAX_SECTION_NUM; i++) {
		for (j = 0; j < MP_TEST_ITEM; j++) {
			if (ipio_strcmp(seq_item[i], tItems[j].desp) != 0)
				continue;

			parser_get_int_data(tItems[j].desp, "enable", str, sizeof(str));
			tItems[j].run = ili_katoi(str);
			if (tItems[j].run != 1 || tItems[j].lcm != lcm_on)
				continue;

			if (ri.count > MP_TEST_ITEM) {
				ILI_ERR("Test item(%d) is invaild, abort\n", ri.count);
				return -EINVAL;
			}

			ri.index[ri.count] = j;
			ri.count++;
		}
	}
	return 0;
}

int ili_mp_test_main(char *apk, bool lcm_on)
{
	int i, ret = 0;
	char str[8] = {0}, ver[8] = {0};

	if (ilits->xch_num <= 0 || ilits->ych_num <= 0) {
		ILI_ERR("Invalid frame length (%d, %d)\n", ilits->xch_num, ilits->ych_num);
		ret = -EMP_INVAL;
		goto out;
	}

	if (ini_info == NULL) {
		ini_info = (struct ini_file_data *)vmalloc(sizeof(struct ini_file_data) * PARSER_MAX_KEY_NUM);
		if (ERR_ALLOC_MEM(ini_info)) {
			ILI_ERR("Failed to malloc ini_info\n");
			ret = -EMP_NOMEM;
			goto out;
		}
	}

	ilitek_tddi_mp_init_item();

	ilitek_tddi_fw_mp_precmd_support();

	ret = ilitek_tddi_mp_ini_parser(ilits->md_ini_path);
	if (ret < 0) {
		ILI_ERR("Failed to parsing INI file\n");
		ret = -EMP_INI;
		goto out;
	}

	/* Compare both protocol version of ini and firmware */
	parser_get_ini_key_value("pv5_4 command", "protocol", str);
	snprintf(ver, sizeof(ver), "0x%s", str);
	ILI_INFO("str = 0x%s, ver = %s\n", str, ver);
	if ((ili_str2hex(ver)) != (core_mp.protocol_ver >> 8)) {
		ILI_ERR("ERROR! MP Protocol version is invaild, 0x%x\n", ili_str2hex(ver));
		ret = -EMP_PROTOCOL;
		goto out;
	}

	/* Read timing info from ini file */
	if (mp_get_timing_info() < 0) {
		ILI_ERR("Failed to get timing info from ini\n");
		ret = -EMP_TIMING_INFO;
		goto out;
	}

#if MP_INT_LEVEL
	if (ili_ic_int_trigger_ctrl(true) < 0) {
		ILI_ERR("Failed to set INT as Level trigger");
		ret = -EMP_CMD;
		goto out;
	}
#endif

	/* Sort test item by ini file */
	if (mp_sort_item(lcm_on) < 0) {
		ILI_ERR("Failed to sort test item\n");
		ret = -EMP_INI;
		goto out;
	}

	for (i = 0; i < ri.count; i++)
		mp_test_run(ri.index[i]);

	ILI_INFO("Test item end\n");

	ret = mp_show_result(lcm_on);

	mp_copy_ret_to_apk(apk);

out:
	mp_test_free();

#if MP_INT_LEVEL
	if (ili_ic_int_trigger_ctrl(false) < 0) {
		ILI_ERR("Failed to set INT back to pluse trigger");
		ret = -EMP_CMD;
	}
#endif

	ipio_vfree((void **)&ini_info);
	return ret;
};
