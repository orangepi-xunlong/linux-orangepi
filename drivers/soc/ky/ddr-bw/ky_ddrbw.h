// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2024 Ky Co., Ltd.
 */
#ifndef __KY_DDRBW_H_
#define __KY_DDRBW_H_

#define DDR_FCLK		24
#define DDR_FC_NODE_LEN		20

#define DDR_UPTHRESHOLD		60
#define DDR_DOWNDIFF		10
#define DDR_THPT_CONS		(DDR_PORT_WIDTH * 2)

#define DDR_THPT_DENOM		1000
#define CMD_LEN			128

#define KHZ			1000
#define MHZ			1000000
/* default voltage 1v for opp registration */
#define DDR_FC_VOL		1000000
/* avoid to use too slow level0 freq, keep it in case corner cases */
#define DDR_SLOW_FREQ		100
#define MC_CONTROL_0		0x44
#define MC_DATA_WIDTH		8
#define MC_BURST_LEN		16

#define DDRC_ISR		0x140
#define DDRC_IER		0x144
#define DDRC_REFCTRL		0x378
#define DDR_RFTIMING		0x3fc
#define PEND_REF_OVERFLOW_CH0	BIT(16)
#define REFRT_INT_CH0		BIT(14)
#define REF_EXCEED_LIMIT_CH0	BIT(12)
#define MB_INT_CH0		BIT(10)
#define DECODE_ERR		BIT(2)
#define DDRC_INT_ISR		0x140
#define DDRC_INT_IER		0x144
#define DDRC_ERR_INFO		0x150
#define DDRC_ERR_ADR_L		0x154
#define DDRC_ERR_ADR_H		0x158
#define DDRC_ERR_ID		0x15

#define KY_PERF_EVT_NUM	4
#define DDR_FC_BW_SIZE		4

#define KY_PERF_CNT_NUM	0x8
#define PERF_CNT_NUM		0x7
#define KY_MON_CTRL_NUM	0x4
#define KY_ID_MON_CTRL_NUM	12
#define KY_ID_MASTER_NUM	15
#define KY_PERF_CNT_ALL	0xff

#define DDR_RD_WR_EVT_CH0	12
#define DDR_RD_WR_EVT_CH1	47

/* DDR port width 128bit(16byte) */
#define DDR_PORT_WIDTH		16
#define DDR_LEN_16		16
#define DDR_LEN_32		32
#define DDR_LEN_64		64

#define DDR_P0_WDW_LEN		6
#define DDR_P0_NOM		80
#define DDR_P0_DENOM		100
#define DDR_P0_DECAY		(DDR_P0_DENOM - DDR_P0_NOM)

#define KY_MON_EVT_NUM	6
#define KY_MON_EVT_MASK	0x1f
#define KY_MON_LAT	12
#define KY_MON_LAT_NUM	4
#define KY_MON_LAT_MASK	0xffff
#define KY_MON_ID_SEL	0x7

#define DDR_CLK_CYCLES		0
#define DDR_CH0_BUSY		0x38
#define DDR_RD_WR_CMD		0x56

#define DDR_PORT_DEBUG_BW	1
#define DDR_PORT_DEBUG_LAT	2

#define DDRBWTOOL_COUNT		1

#define DDR_MAX_FREQ_LV		8

#define PERF_CNT_CONF0		0x100
#define PERF_CNT_CONF1		0x104
/* check whether overflow or not */
#define PERF_CNT_STATUS		0x108
#define PERF_CNT_CTRL		0x10c

#define PERF_CNT_BASE		0x110

#define PERF_CNT_ISR		0x140
#define PERF_CNT_IER		0x144

/* reg range: 0x10 - 0x2c, step 0x4 */
#define perf_cnt_reg(n)		(PERF_CNT_BASE + (n << 2))

#define perf_cnt_conf_base(n)	(n << 3)
#define PERF_CNT_EN		(1 << 7)

#define is_cnt_valid(cnt)	((cnt < PERF_CNT_NUM) ? 0 : -EINVAL)

#define PERF_INVALID_EVT_SEL	0xffff
#define PERF_SYS_NODE_LEN	20

struct ky_ddr_pbw {
	u64 byte_bw;	/* bytes throughput */
	u64 req_bw;	/* requests throughtput */
	u32 rd_reqs;	/* read requests number */
	u32 wr_reqs;	/* write requests number */
};
struct ky_ddr_bw_info {
	u64 curr_freq;
	u64 total_bw;
	struct ky_ddr_pbw port_bw[4];
};

struct ddr_perf_cnt {
	u32 id;
	u32 evt_sel;
	u32 evt_num;
	bool init;
	bool enable;
	bool free;
	spinlock_t lock;
};

struct devfreq_throughput_table {
	u64 max;
	u64 min;
};

struct devfreq_throughput_data {
	struct devfreq_throughput_table *ddr_thpt_tbl;

	u32 tbl_len;
	u32 upthreshold;
	u32 downdifferential;
};

struct ddr_perf_mon {
	u32 id;
	u32 th_x;
	u32 th_y;
	u32 evt_sel;
	u32 evt_num[KY_PERF_EVT_NUM];
	bool init;
	bool enable;
	bool free;
	spinlock_t lock;
};

struct ddraxi_mon {
	u32 mid;
	u32 port_id;
	u32 id;
	u32 lat_th;
	u32 evt_num[KY_MON_EVT_NUM];
	u32 lat_num[KY_MON_LAT_NUM];
	bool master_is_lcd;
	bool init;
	bool enable;
	bool free;
	bool lat_en;
	spinlock_t lock;
};

struct ddr_dfreq_data {
	struct ddraxi_mon *axi_mon[KY_MON_CTRL_NUM + KY_ID_MASTER_NUM + 1];
	struct work_struct work_sum_qos;
	struct devfreq *devfreq;
	struct clk *ddr_clk;
	void __iomem *base;
	int ddr_mon_debug;
	bool cnt_en;

	unsigned long qos_min_freq;
	unsigned long qos_max_freq;
	const char *clk_name;

	spinlock_t lock;
	u64 last_poll;
	/* perf-cnt events */
	u64 clk_cycle;
	u64 busy_cycle;
	u64 cmd_number;
	u64 cmd_diff;

	/* axi monitor events */
	u64 rd_reqs[KY_MON_CTRL_NUM + KY_ID_MASTER_NUM + 1];
	u64 wr_reqs[KY_MON_CTRL_NUM + KY_ID_MASTER_NUM + 1];
	u64 tol_rd_lat[KY_MON_CTRL_NUM + KY_ID_MASTER_NUM + 1];
	u64 tol_wr_lat[KY_MON_CTRL_NUM + KY_ID_MASTER_NUM + 1];
	u64 total_rd_bytes[KY_MON_CTRL_NUM + KY_ID_MASTER_NUM + 1];
	u64 total_wr_bytes[KY_MON_CTRL_NUM + KY_ID_MASTER_NUM + 1];
	u64 overflow_rd_num[KY_MON_CTRL_NUM + KY_ID_MASTER_NUM + 1];
	u64 overflow_wr_num[KY_MON_CTRL_NUM + KY_ID_MASTER_NUM + 1];
	u32 ovfl_threshold;

	struct ddr_perf_cnt *perf_ch0;
	struct ddr_perf_cnt *perf_ch1;
	struct ddr_perf_cnt *perf_ch2;

	unsigned long throughput;
	unsigned long p0_r_thpt;
	unsigned long p0_w_thpt;
	unsigned long p0_thpt;
};

struct ddrbw_info {
	u64 last_poll;
	/* perf-cnt events */
	u64 clk_cycle;
	u64 busy_cycle;
	u64 cmd_number;
	int port_num;
	int total_num;
	u32 ovfl_threshold;
	unsigned long throughput;
	unsigned long p0_r_thpt;
	unsigned long p0_w_thpt;
	unsigned long p0_thpt;
	unsigned long total_time;
	unsigned long busy_time;
	unsigned long current_frequency;
};

struct aximon_info {
	u32 mid;
	u32 port_id;
	u32 id;
	u32 single_master;
	s64 rd_bytes;
	s64 wr_bytes;
	u64 max_rd;
	u64 max_wr;
	u64 avr_rd;
	u64 avr_wr;
	u64 rd_reqs;
	u64 wr_reqs;
	u64 wr_thpt;
	u64 rd_thpt;
	u64 byte_thpt;
	u64 req_thpt;
	char name[10];
};
struct aximon_info *mon_info;
char *master_name[20] = {
	"P0", "P1", "P2", "P3", "CPU", "GPU",
	"AES", "USBMMC", "GMAC1", "RCPU", "GMAC0",
	"VPU", "PCIE0", "PCIE1", "PCIE2", "ISP",
	"DSI", "V2D", "HDMI", "LCD"
};

/*
	mid0	mid1	mid2
  p0  	CPU	GPU	aes
  p1	USBMMC	GMAC1	RCPU	GMAC0
  p2	vpu	pcie0	PCIE1	pcie2
  p3	ISP	dsi	v2d	hdmi
*/

struct ky_ddrbw_status {
	/* both since the last measure */
	unsigned long total_time;
	unsigned long busy_time;
	unsigned long current_frequency;
	void *private_data;
	unsigned long throughput;
	unsigned long p0_r_thpt;
	unsigned long p0_w_thpt;
	unsigned long p0_thpt;
};

#define LCD_MON_BASE		0x200
#define KY_MON_BASE	0x0
#define KY_MON_ID_BASE	0x8
#define KY_MON_DATA	0xc
#define KY_MON_ID_REG	0x40

/* reg range: 0x0 - 0x30, step 0x10 */
#define mon_ctrl_reg(n)		(KY_MON_BASE + (n << 4))
/* reg range: 0x3c - 0x6c, step 0x10 */
#define mon_data_reg(n)		(KY_MON_DATA + (n << 4))

/* id monitor reg range 0x08 `0x38, step 0x10 */
#define mon_id_reg(n)		(KY_MON_ID_BASE + (n << 4))
/* id monitor sel range 0x40 `0x7c, step 0x4 */
#define mon_id_sel(n, m)	(KY_MON_ID_REG + (m << 4) + (n << 2))

#define MON_CTRL_SEL		(1 << 4)
#define MON_CTRL_LATCH		(1 << 5)
#define MON_CTRL_READ		(1 << 6)
#define MON_CTRL_EN		(1 << 31)

/* ID MON */
#define IDMON_CTRL_EN		24
#define IDMON_CLK_EN		16

#define is_mon_valid(mon)	((mon < (KY_MON_CTRL_NUM + KY_ID_MASTER_NUM + 1)) ? 0 : -EINVAL)

#define PERF_SYS_NODE_LEN	20

/*
 * bandwidth calculation:
 * event 16: total read bytes
 * event 18: total write bytes
 * event 17: counter numbers of read-lat exceed threshold
 * event 19: counter numbers of write-lat exceed threshold
 * event 1: total read request
 * event 9: total write request
 * int mon_evt_id[KY_MON_EVT_NUM] = {17, 19, 16, 18, 1, 9};
 */
/* Monitor bandwidth by default. */
int mon_evt_id[KY_MON_EVT_NUM] = {16, 18, 1, 9, 17, 19};

/* need to enable/disable monitor for max rd/wr latency data */
int mon_lat_id[KY_MON_LAT_NUM];

/* we will just use evnet 5 and event 13 */
int mon_lat_id_z1[KY_MON_LAT_NUM] = {5, 13, 5, 13};

/*
 * latency for a0:
 * event 0x14: max rd
 * event 0x15: max wr
 * event 0x18: total_read_latency on fclk
 * evnet 0x19: total_write_latency on fclk
 */
int mon_lat_id_a0[KY_MON_LAT_NUM] = {0x14, 0x15, 0x18, 0x19};
struct ky_ddraxi_mon_data {
	void __iomem *reg_base;
	void __iomem *ciu_base;
	struct device *dev;
	struct ddraxi_mon mon[KY_ID_MON_CTRL_NUM + KY_MON_CTRL_NUM + 1];
	struct ddr_dfreq_data *data;
	struct ky_ddrbw_status *stat;
};

static struct ky_ddraxi_mon_data *ddraxi_mon_data;

static u32 master_id[KY_ID_MASTER_NUM] = {
	0, 1, 2,
	0, 1, 2, 3,
	0, 1, 2, 3,
	0, 1, 2, 3
};

union perf_cnt_config {
	struct {
		u32 evt_sel:7;
		u32 enable:1;
		u32 reserved1:24;
	} bit;
	u32 conf;
};

struct ddr_perf_data {
	void __iomem *base;
	struct ddr_perf_cnt cnt[PERF_CNT_NUM];
};

#endif	/* __KY_DDRBW_H_ */
