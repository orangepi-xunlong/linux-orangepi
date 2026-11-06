#include <linux/kernel.h>
#include <linux/io.h>
#include <linux/dma-mapping.h>
#include <linux/delay.h>
#include <linux/clk.h>
#include <linux/reset.h>
#include <linux/streamline_annotate.h>
#include <linux/clk.h>
#include <linux/version.h>
#include <linux/timex.h>
#include "mon_platform.h"

struct AXIMON_INFO_PORT {
	uint8_t port_name[16];
	uint8_t id_name[16];
	uint32_t bcnts_min;
	uint32_t bcnts_acc;
	uint32_t bcnts_max;
	uint32_t bcnts_avg;
	uint32_t bw_min;
	uint32_t bw_acc;
	uint32_t bw_max;
	uint64_t bw_avg_sum;
	uint64_t count;
	uint32_t time_avg;
	uint32_t latency_write_avg;
	uint32_t latency_write_avg_max;
	uint32_t latency_write_avg_min;
	uint32_t latency_read_avg;
	uint32_t latency_read_avg_max;
	uint32_t latency_read_avg_min;
	uint32_t latency_max;
	uint32_t latency_min;
	uint32_t axi_clk;
	uint64_t write_latency_his_cnt[11];
	uint64_t read_latency_his_cnt[11];
};


struct AXIMON_INFO {
	struct AXIMON_INFO_PORT real_m1;
	struct AXIMON_INFO_PORT offline_m1;
	struct AXIMON_INFO_PORT offline_m2;
	struct AXIMON_INFO_PORT offline_m3;
	struct AXIMON_INFO_PORT offline_m4;
	struct AXIMON_INFO_PORT offline_total;
	struct AXIMON_INFO_PORT bulk_m1;
	struct AXIMON_INFO_PORT bulk_m2;
	struct AXIMON_INFO_PORT bulk_total;
	//struct AXIMON_INFO_PORT ddrsys1_m1;
	//struct AXIMON_INFO_PORT ddrsys2_m1;
};

struct timespec64 ts_start, ts_end, ts_delta;

//struct timespec64 ts_start1, ts_end1, ts_delta1;

#define DRAMTYPE_STRLEN 30
struct DRAM_INFO {
	uint32_t bus_width;
	uint32_t data_rate;
	char type[DRAMTYPE_STRLEN];
};

static struct DRAM_INFO dram_info;
static struct AXIMON_INFO aximon_info;
static void __iomem *iomem_aximon_base;
static void __iomem *iomem_aximon_offline_base;
static void __iomem *iomem_aximon_bulk_base;
//static void __iomem *iomem_ddrmon_sys1_base;
//static void __iomem *iomem_ddrmon_sys2_base;

static uint r_hit_id = 0;
module_param(r_hit_id, uint, S_IRUGO);
static uint r_hit_sel = 0;
module_param(r_hit_sel, uint, S_IRUGO);

static uint o1_input_clk = 1;
module_param(o1_input_clk, uint, S_IRUGO);
static uint o1_hit_id = 0;
module_param(o1_hit_id, uint, S_IRUGO);

static uint o2_input_clk = 1;
module_param(o2_input_clk, uint, S_IRUGO);
static uint o2_hit_id = 0;
module_param(o2_hit_id, uint, S_IRUGO);

static uint ds_input_clk = 1;
module_param(ds_input_clk, uint, S_IRUGO);

//#define AXIMON_BASE 0x08008000
#define REMAPPING_BASE 0
#define AXIMON_real_M1_WRITE	(REMAPPING_BASE + 0x0)
#define AXIMON_real_M1_READ		(REMAPPING_BASE + 0x80)

#define AXIMON_offline_M1_WRITE	(REMAPPING_BASE + 0x0)
#define AXIMON_offline_M1_READ	(REMAPPING_BASE + 0x80)
#define AXIMON_offline_M2_WRITE	(REMAPPING_BASE + 0x100)
#define AXIMON_offline_M2_READ	(REMAPPING_BASE + 0x180)
#define AXIMON_offline_M3_WRITE	(REMAPPING_BASE + 0x200)
#define AXIMON_offline_M3_READ	(REMAPPING_BASE + 0x280)
#define AXIMON_offline_M4_WRITE	(REMAPPING_BASE + 0x300)
#define AXIMON_offline_M4_READ	(REMAPPING_BASE + 0x380)

#define AXIMON_bulk_M1_WRITE	(REMAPPING_BASE + 0x0)
#define AXIMON_bulk_M1_READ		(REMAPPING_BASE + 0x80)
#define AXIMON_bulk_M2_WRITE	(REMAPPING_BASE + 0x100)
#define AXIMON_bulk_M2_READ		(REMAPPING_BASE + 0x180)

//#define DDRMON_sys1_M1_WRITE	(REMAPPING_BASE + 0x0)
//#define DDRMON_sys1_M1_READ		(REMAPPING_BASE + 0x80)

//#define DDRMON_sys2_M1_WRITE	(REMAPPING_BASE + 0x0)
//#define DDRMON_sys2_M1_READ		(REMAPPING_BASE + 0x80)

#define AXIMON_OFFSET_CYCLE 		0x24
#define AXIMON_OFFSET_BYTECNTS 		0x2C
#define AXIMON_OFFSET_LATENCYCNTS 	0x34
#define AXIMON_OFFSET_HITCNTS 		0x28

#define AXIMON_OFFSET_LAT_BIN_SIZE_SEL 0x50

#define AXIMON_OFFSET_LATENCY_HIS_0 0x54
#define AXIMON_OFFSET_LATENCY_HIS_1 0x58
#define AXIMON_OFFSET_LATENCY_HIS_2 0x5c
#define AXIMON_OFFSET_LATENCY_HIS_3 0x60
#define AXIMON_OFFSET_LATENCY_HIS_4 0x64
#define AXIMON_OFFSET_LATENCY_HIS_5 0x68
#define AXIMON_OFFSET_LATENCY_HIS_6 0x6c
#define AXIMON_OFFSET_LATENCY_HIS_7 0x70
#define AXIMON_OFFSET_LATENCY_HIS_8 0x74
#define AXIMON_OFFSET_LATENCY_HIS_9 0x78
#define AXIMON_OFFSET_LATENCY_HIS_10 0x7c

#define AXIMON_SNAPSHOT_REGVALUE_1 0x40004
#define AXIMON_SNAPSHOT_REGVALUE_2 0x40000

#define AXIMON_START_REGVALUE 0x30001
#define AXIMON_STOP_REGVALUE 0x30002

#define AXIMON_real_SELECT_CLK 0x01000100
static uint32_t AXIMON_offline_m1_SELECT_CLK = 0x01000100;
static uint32_t AXIMON_offline_m2_SELECT_CLK = 0x01000100;
#define AXIMON_offline_m3_SELECT_CLK 0x01000100
#define AXIMON_offline_m4_SELECT_CLK 0x02000200
#define AXIMON_bulk_m1_SELECT_CLK 0x01000100
#define AXIMON_bulk_m2_SELECT_CLK 0x01000100

//static uint32_t DDRMON_sys1_m1_SELECT_CLK = 0x01000100;
//static uint32_t DDRMON_sys2_m1_SELECT_CLK = 0x01000100;

static void axi_mon_real_snapshot(uint32_t base_register)
{
	writel(AXIMON_SNAPSHOT_REGVALUE_1, (iomem_aximon_base + base_register));
	writel(AXIMON_SNAPSHOT_REGVALUE_2, (iomem_aximon_base + base_register));
}
static void axi_mon_offline_snapshot(uint32_t base_register)
{
	writel(AXIMON_SNAPSHOT_REGVALUE_1, (iomem_aximon_offline_base + base_register));
	writel(AXIMON_SNAPSHOT_REGVALUE_2, (iomem_aximon_offline_base + base_register));
}
static void axi_mon_bulk_snapshot(uint32_t base_register)
{
	writel(AXIMON_SNAPSHOT_REGVALUE_1, (iomem_aximon_bulk_base + base_register));
	writel(AXIMON_SNAPSHOT_REGVALUE_2, (iomem_aximon_bulk_base + base_register));
}
#if 0
static void axi_mon_ddrsys1_snapshot(uint32_t base_register)
{
	writel(AXIMON_SNAPSHOT_REGVALUE_1, (iomem_ddrmon_sys1_base + base_register));
	writel(AXIMON_SNAPSHOT_REGVALUE_2, (iomem_ddrmon_sys1_base + base_register));
}
static void axi_mon_ddrsys2_snapshot(uint32_t base_register)
{
	writel(AXIMON_SNAPSHOT_REGVALUE_1, (iomem_ddrmon_sys2_base + base_register));
	writel(AXIMON_SNAPSHOT_REGVALUE_2, (iomem_ddrmon_sys2_base + base_register));
}
#endif

static void axi_mon_real_start(uint32_t base_register)
{
	writel(AXIMON_START_REGVALUE, (iomem_aximon_base + base_register));
}
static void axi_mon_offline_start(uint32_t base_register)
{
	writel(AXIMON_START_REGVALUE, (iomem_aximon_offline_base + base_register));
}
static void axi_mon_bulk_start(uint32_t base_register)
{
	writel(AXIMON_START_REGVALUE, (iomem_aximon_bulk_base + base_register));
}
#if 0
static void axi_mon_ddrsys1_start(uint32_t base_register)
{
	writel(AXIMON_START_REGVALUE, (iomem_ddrmon_sys1_base + base_register));
}
static void axi_mon_ddrsys2_start(uint32_t base_register)
{
	writel(AXIMON_START_REGVALUE, (iomem_ddrmon_sys2_base + base_register));
}
#endif
static void axi_mon_real_stop(uint32_t base_register)
{
	writel(AXIMON_STOP_REGVALUE, (iomem_aximon_base + base_register));
}
static void axi_mon_offline_stop(uint32_t base_register)
{
	writel(AXIMON_STOP_REGVALUE, (iomem_aximon_offline_base + base_register));
}
static void axi_mon_bulk_stop(uint32_t base_register)
{
	writel(AXIMON_STOP_REGVALUE, (iomem_aximon_bulk_base + base_register));
}
#if 0
static void axi_mon_ddrsys1_stop(uint32_t base_register)
{
	writel(AXIMON_STOP_REGVALUE, (iomem_ddrmon_sys1_base + base_register));
}
static void axi_mon_ddrsys2_stop(uint32_t base_register)
{
	writel(AXIMON_STOP_REGVALUE, (iomem_ddrmon_sys2_base + base_register));
}
#endif
static void axi_mon_set_lat_bin_size(uint32_t set_value)
{
	writel(set_value, (iomem_aximon_base + AXIMON_real_M1_WRITE + AXIMON_OFFSET_LAT_BIN_SIZE_SEL));
	writel(set_value, (iomem_aximon_base + AXIMON_real_M1_READ + AXIMON_OFFSET_LAT_BIN_SIZE_SEL));

	writel(set_value, (iomem_aximon_offline_base + AXIMON_offline_M1_WRITE + AXIMON_OFFSET_LAT_BIN_SIZE_SEL));
	writel(set_value, (iomem_aximon_offline_base + AXIMON_offline_M1_READ + AXIMON_OFFSET_LAT_BIN_SIZE_SEL));

	//writel(set_value, (iomem_ddrmon_sys1_base + DDRMON_sys1_M1_WRITE + AXIMON_OFFSET_LAT_BIN_SIZE_SEL));
	//writel(set_value, (iomem_ddrmon_sys1_base + DDRMON_sys1_M1_READ + AXIMON_OFFSET_LAT_BIN_SIZE_SEL));

	//writel(set_value, (iomem_ddrmon_sys2_base + DDRMON_sys2_M1_WRITE + AXIMON_OFFSET_LAT_BIN_SIZE_SEL));
	//writel(set_value, (iomem_ddrmon_sys2_base + DDRMON_sys2_M1_READ + AXIMON_OFFSET_LAT_BIN_SIZE_SEL));
}

static uint32_t axi_mon_real_get_byte_cnts(uint32_t base_register)
{
	return readl(iomem_aximon_base + base_register + AXIMON_OFFSET_BYTECNTS);
}
static uint32_t axi_mon_offline_get_byte_cnts(uint32_t base_register)
{
	return readl(iomem_aximon_offline_base + base_register + AXIMON_OFFSET_BYTECNTS);
}
static uint32_t axi_mon_bulk_get_byte_cnts(uint32_t base_register)
{
	return readl(iomem_aximon_bulk_base + base_register + AXIMON_OFFSET_BYTECNTS);
}

static uint32_t axi_mon_real_get_hit_cnts(uint32_t base_register)
{
	return readl(iomem_aximon_base + base_register + AXIMON_OFFSET_HITCNTS);
}
static uint32_t axi_mon_offline_get_hit_cnts(uint32_t base_register)
{
	return readl(iomem_aximon_offline_base + base_register + AXIMON_OFFSET_HITCNTS);
}
static uint32_t axi_mon_bulk_get_hit_cnts(uint32_t base_register)
{
	return readl(iomem_aximon_bulk_base + base_register + AXIMON_OFFSET_HITCNTS);
}
#if 0
static uint32_t axi_mon_ddrsys1_get_hit_cnts(uint32_t base_register)
{
	return readl(iomem_ddrmon_sys1_base + base_register + AXIMON_OFFSET_HITCNTS);
}
static uint32_t axi_mon_ddrsys2_get_hit_cnts(uint32_t base_register)
{
	return readl(iomem_ddrmon_sys2_base + base_register + AXIMON_OFFSET_HITCNTS);
}
#endif
static uint32_t axi_mon_real_get_latency_cnts(uint32_t base_register)
{
	return readl(iomem_aximon_base + base_register + AXIMON_OFFSET_LATENCYCNTS);
}
static uint32_t axi_mon_offline_get_latency_cnts(uint32_t base_register)
{
	return readl(iomem_aximon_offline_base + base_register + AXIMON_OFFSET_LATENCYCNTS);
}
static uint32_t axi_mon_bulk_get_latency_cnts(uint32_t base_register)
{
	return readl(iomem_aximon_bulk_base + base_register + AXIMON_OFFSET_LATENCYCNTS);
}
#if 0
static uint32_t axi_mon_ddrsys1_get_latency_cnts(uint32_t base_register)
{
	return readl(iomem_ddrmon_sys1_base + base_register + AXIMON_OFFSET_LATENCYCNTS);
}
static uint32_t axi_mon_ddrsys2_get_latency_cnts(uint32_t base_register)
{
	return readl(iomem_ddrmon_sys2_base + base_register + AXIMON_OFFSET_LATENCYCNTS);
}
#endif
static uint32_t axi_mon_real_get_latency_his_cnts(uint32_t base_register, uint32_t his_n)
{
	return readl(iomem_aximon_base + base_register + AXIMON_OFFSET_LATENCY_HIS_0 + 4 * his_n);
}
static uint32_t axi_mon_offline_get_latency_his_cnts(uint32_t base_register, uint32_t his_n)
{
	return readl(iomem_aximon_offline_base + base_register + AXIMON_OFFSET_LATENCY_HIS_0 + 4 * his_n);
}
#if 0
static uint32_t axi_mon_bulk_get_latency_his_cnts(uint32_t base_register, uint32_t his_n)
{
	return readl(iomem_aximon_bulk_base + base_register + AXIMON_OFFSET_LATENCY_HIS_0 + 4 * his_n);
}
static uint32_t axi_mon_ddrsys1_get_latency_his_cnts(uint32_t base_register, uint32_t his_n)
{
	return readl(iomem_ddrmon_sys1_base + base_register + AXIMON_OFFSET_LATENCY_HIS_0 + 4 * his_n);
}
static uint32_t axi_mon_ddrsys2_get_latency_his_cnts(uint32_t base_register, uint32_t his_n)
{
	return readl(iomem_ddrmon_sys2_base + base_register + AXIMON_OFFSET_LATENCY_HIS_0 + 4 * his_n);
}
#endif
static void axi_mon_count_port_info(uint32_t duration, uint32_t byte_cnt, struct AXIMON_INFO_PORT *axi_info)
{
	uint32_t bw = byte_cnt / duration;

	pr_debug("duration=%u, byte_cnt=%u, count=%llu\n", duration, byte_cnt, axi_info->count);
	//pr_err("duration=%ld, byte_cnt=%ld, count=%ld\n", duration, byte_cnt, axi_info->count);

	if (axi_info->count != 0) {
		if (bw) {
			if (bw < axi_info->bw_min)
				axi_info->bw_min = bw;
		}
		else
			axi_info->bw_min = 0;

		if (bw > axi_info->bw_max)
			axi_info->bw_max = bw;

		axi_info->bw_avg_sum += bw;

		if (axi_info->time_avg)
			axi_info->time_avg = (axi_info->time_avg + duration) >> 1;
		else
			axi_info->time_avg = duration;
	}
	else
		axi_info->bw_min = 0xffffffff;

	axi_info->count += 1;

#if 0
	pr_err("%s bw=%d, bw_min=%d, bw_max=%d, bw_avg_sum=%d\n",
								axi_info->port_name, bw,
								axi_info->bw_min, axi_info->bw_max,
								axi_info->bw_avg_sum);

#endif
}

static void axi_mon_count_port_latency_his_info(uint64_t write_latency_his_cnt[11], uint64_t read_latency_his_cnt[11],
		struct AXIMON_INFO_PORT *axi_info)
{
	uint i = 0;

	if (axi_info->count != 0) {
		for (i = 0; i <= 10; i++) {
			axi_info->write_latency_his_cnt[i] += write_latency_his_cnt[i];
			axi_info->read_latency_his_cnt[i] += read_latency_his_cnt[i];
			//pr_err("axi_info->write_latency_his_cnt[%d]=%5d, axi_info->read_latency_his_cnt[%d]=%5d\n",
			//			i, axi_info->write_latency_his_cnt[i], i, axi_info->read_latency_his_cnt[i]);
		}
	}
}

static void axi_mon_count_port_latency_info(uint32_t latency_write_cnt, uint32_t write_hit_cnt,
		uint32_t latency_read_cnt, uint32_t read_hit_cnt, struct AXIMON_INFO_PORT *axi_info)
{
	uint32_t avg_latency = 0;

	if (write_hit_cnt != 0)
		//avg_latency = 1000 * latency_write_cnt/((dram_info.data_rate/4)*write_hit_cnt);
		avg_latency = 1000 * latency_write_cnt/((axi_info->axi_clk)*write_hit_cnt);
	else
		avg_latency = 0;

	if (avg_latency) {
		if (!axi_info->latency_write_avg_min)
			axi_info->latency_write_avg_min = avg_latency;
		else if (avg_latency < axi_info->latency_write_avg_min)
			axi_info->latency_write_avg_min = avg_latency;
	} else
		axi_info->latency_write_avg_min = 0;

	if (avg_latency > axi_info->latency_write_avg_max)
		axi_info->latency_write_avg_max = avg_latency;

	axi_info->latency_write_avg += avg_latency;

	if (read_hit_cnt != 0)
		//avg_latency = 1000 * latency_read_cnt/((dram_info.data_rate/4)*read_hit_cnt);
		avg_latency = 1000 * latency_read_cnt/((axi_info->axi_clk)*read_hit_cnt);
	else
		avg_latency = 0;

	if (avg_latency) {
		if (!axi_info->latency_read_avg_min)
			axi_info->latency_read_avg_min = avg_latency;
		else if (avg_latency < axi_info->latency_read_avg_min)
			axi_info->latency_read_avg_min = avg_latency;
	} else
		axi_info->latency_read_avg_min = 0;

	if (avg_latency > axi_info->latency_read_avg_max)
		axi_info->latency_read_avg_max = avg_latency;

	axi_info->latency_read_avg += avg_latency;
}

void customed_setting(void)
{
	uint32_t u4value;

	//pr_err("Customed_setting...\n");
	//real fabric setting
	//r_hit_sel setting
	u4value = readl(iomem_aximon_base + AXIMON_real_M1_WRITE + 0x4);
	u4value = ((u4value) & (~0x3ff)) | r_hit_sel;
	writel(u4value, (iomem_aximon_base + AXIMON_real_M1_WRITE + 0x4));
	u4value = readl(iomem_aximon_base + AXIMON_real_M1_READ + 0x4);
	u4value = ((u4value) & (~0x3ff)) | r_hit_sel;
	writel(u4value, (iomem_aximon_base + AXIMON_real_M1_READ + 0x4));

	//r_hit_id&id mask setting
	if(r_hit_sel == 2) {
		u4value = readl(iomem_aximon_base + AXIMON_real_M1_WRITE + 0x1c);
		u4value = ((u4value) & (~0x400)) | (r_hit_id << 10);
		writel(u4value, (iomem_aximon_base + AXIMON_real_M1_WRITE + 0x1c));
		u4value = readl(iomem_aximon_base + AXIMON_real_M1_READ + 0x1c);
		u4value = ((u4value) & (~0x400)) | (r_hit_id << 10);
		writel(u4value, (iomem_aximon_base + AXIMON_real_M1_READ + 0x1c));

		u4value = readl(iomem_aximon_base + AXIMON_real_M1_WRITE + 0x18);
		u4value = ((u4value) | (0x400));
		writel(u4value, (iomem_aximon_base + AXIMON_real_M1_WRITE + 0x18));
		u4value = readl(iomem_aximon_base + AXIMON_real_M1_READ + 0x18);
		u4value = ((u4value) | (0x400));
		writel(u4value, (iomem_aximon_base + AXIMON_real_M1_READ + 0x18));
	}

	//offline_fabric m1 setting
	//o1 hit sel fix to 2
	u4value = readl(iomem_aximon_offline_base + AXIMON_offline_M1_WRITE + 0x4);
	u4value = ((u4value) & (~0x3ff)) | 0x2;
	writel(u4value, (iomem_aximon_offline_base + AXIMON_offline_M1_WRITE + 0x4));
	u4value = readl(iomem_aximon_offline_base + AXIMON_offline_M1_READ + 0x4);
	u4value = ((u4value) & (~0x3ff)) | 0x2;
	writel(u4value, (iomem_aximon_offline_base + AXIMON_offline_M1_READ + 0x4));
	//o1_hit_id setting
	u4value = readl(iomem_aximon_offline_base + AXIMON_offline_M1_WRITE + 0x1c);
	u4value = ((u4value) & (~0x7800)) | (o1_hit_id << 11);
	writel(u4value, (iomem_aximon_offline_base + AXIMON_offline_M1_WRITE + 0x1c));
	u4value = readl(iomem_aximon_offline_base + AXIMON_offline_M1_READ + 0x1c);
	u4value = ((u4value) & (~0x7800)) | (o1_hit_id << 11);
	writel(u4value, (iomem_aximon_offline_base + AXIMON_offline_M1_READ + 0x1c));
	//o1 hit id mask setting
	u4value = readl(iomem_aximon_offline_base + AXIMON_offline_M1_WRITE + 0x18);
	u4value = ((u4value) | (0x7800));
	writel(u4value, (iomem_aximon_offline_base + AXIMON_offline_M1_WRITE + 0x18));
	u4value = readl(iomem_aximon_offline_base + AXIMON_offline_M1_READ + 0x18);
	u4value = ((u4value) | (0x7800));
	writel(u4value, (iomem_aximon_offline_base + AXIMON_offline_M1_READ + 0x18));

	//offline_fabric m2 setting
	//o2 hit sel fix to 2
	u4value = readl(iomem_aximon_offline_base + AXIMON_offline_M2_WRITE + 0x4);
	u4value = ((u4value) & (~0x3ff)) | 0x2;
	writel(u4value, (iomem_aximon_offline_base + AXIMON_offline_M2_WRITE + 0x4));
	u4value = readl(iomem_aximon_offline_base + AXIMON_offline_M2_READ + 0x4);
	u4value = ((u4value) & (~0x3ff)) | 0x2;
	writel(u4value, (iomem_aximon_offline_base + AXIMON_offline_M2_READ + 0x4));
	//o2_hit_id setting
	u4value = readl(iomem_aximon_offline_base + AXIMON_offline_M2_WRITE + 0x1c);
	u4value = ((u4value) & (~0x7800)) | (o2_hit_id << 11);
	writel(u4value, (iomem_aximon_offline_base + AXIMON_offline_M2_WRITE + 0x1c));
	u4value = readl(iomem_aximon_offline_base + AXIMON_offline_M2_READ + 0x1c);
	u4value = ((u4value) & (~0x7800)) | (o2_hit_id << 11);
	writel(u4value, (iomem_aximon_offline_base + AXIMON_offline_M2_READ + 0x1c));
	//o2 id mask setting
	u4value = readl(iomem_aximon_offline_base + AXIMON_offline_M2_WRITE + 0x18);
	u4value = ((u4value) | (0x7800));
	writel(u4value, (iomem_aximon_offline_base + AXIMON_offline_M2_WRITE + 0x18));
	u4value = readl(iomem_aximon_offline_base + AXIMON_offline_M2_READ + 0x18);
	u4value = ((u4value) | (0x7800));
	writel(u4value, (iomem_aximon_offline_base + AXIMON_offline_M2_READ + 0x18));
}

void axi_mon_reset_all(void)
{
	//reset aximon info
	memset(&aximon_info, 0, sizeof(struct AXIMON_INFO));

	customed_setting();

	//input source select configure
	writel(AXIMON_real_SELECT_CLK, (iomem_aximon_base + AXIMON_real_M1_WRITE));
	writel(AXIMON_real_SELECT_CLK, (iomem_aximon_base + AXIMON_real_M1_READ));
	writel(AXIMON_offline_m1_SELECT_CLK, (iomem_aximon_offline_base + AXIMON_offline_M1_WRITE));
	writel(AXIMON_offline_m1_SELECT_CLK, (iomem_aximon_offline_base + AXIMON_offline_M1_READ));
	writel(AXIMON_offline_m2_SELECT_CLK, (iomem_aximon_offline_base + AXIMON_offline_M2_WRITE));
	writel(AXIMON_offline_m2_SELECT_CLK, (iomem_aximon_offline_base + AXIMON_offline_M2_READ));
	writel(AXIMON_offline_m3_SELECT_CLK, (iomem_aximon_offline_base + AXIMON_offline_M3_WRITE));
	writel(AXIMON_offline_m3_SELECT_CLK, (iomem_aximon_offline_base + AXIMON_offline_M3_READ));
	writel(AXIMON_offline_m4_SELECT_CLK, (iomem_aximon_offline_base + AXIMON_offline_M4_WRITE));
	writel(AXIMON_offline_m4_SELECT_CLK, (iomem_aximon_offline_base + AXIMON_offline_M4_READ));
	writel(AXIMON_bulk_m1_SELECT_CLK, (iomem_aximon_bulk_base + AXIMON_bulk_M1_WRITE));
	writel(AXIMON_bulk_m1_SELECT_CLK, (iomem_aximon_bulk_base + AXIMON_bulk_M1_READ));
	writel(AXIMON_bulk_m2_SELECT_CLK, (iomem_aximon_bulk_base + AXIMON_bulk_M2_WRITE));
	writel(AXIMON_bulk_m2_SELECT_CLK, (iomem_aximon_bulk_base + AXIMON_bulk_M2_READ));
	//writel(DDRMON_sys1_m1_SELECT_CLK, (iomem_ddrmon_sys1_base + DDRMON_sys1_M1_WRITE));
	//writel(DDRMON_sys1_m1_SELECT_CLK, (iomem_ddrmon_sys1_base + DDRMON_sys1_M1_READ));
	//writel(DDRMON_sys2_m1_SELECT_CLK, (iomem_ddrmon_sys2_base + DDRMON_sys2_M1_WRITE));
	//writel(DDRMON_sys2_m1_SELECT_CLK, (iomem_ddrmon_sys2_base + DDRMON_sys2_M1_READ));

	//configure port name
	strcpy(aximon_info.real_m1.port_name, "rt_m1");
	strcpy(aximon_info.offline_m1.port_name, "off_m1");
	strcpy(aximon_info.offline_m2.port_name, "off_m2");
	strcpy(aximon_info.offline_m3.port_name, "off_m3");
	strcpy(aximon_info.offline_m4.port_name, "off_m4");
	strcpy(aximon_info.offline_total.port_name, "off_total");
	strcpy(aximon_info.bulk_m1.port_name, "bulk_m1");
	strcpy(aximon_info.bulk_m2.port_name, "bulk_m2");
	strcpy(aximon_info.bulk_total.port_name, "bulk_total");
	//strcpy(aximon_info.ddrsys1_m1.port_name, "ddrsys1_m1");
	//strcpy(aximon_info.ddrsys2_m1.port_name, "ddrsys2_m1");

	//configure AXI clk
	aximon_info.real_m1.axi_clk = 650;
	aximon_info.offline_m1.axi_clk = 1000;
	aximon_info.offline_m2.axi_clk = 1000;
	aximon_info.offline_m3.axi_clk = 1000;
	aximon_info.offline_m4.axi_clk = 1000;
	aximon_info.bulk_m1.axi_clk = 900;
	aximon_info.bulk_m2.axi_clk = 900;
	//aximon_info.ddrsys1_m1.axi_clk = 1066;
	//aximon_info.ddrsys2_m1.axi_clk = 1066;

	//configure id name
	if(r_hit_sel == 0)
		strcpy(aximon_info.real_m1.id_name, "rt_p1");
	else {
		if(r_hit_id == 0)
			strcpy(aximon_info.real_m1.id_name, "vi_rt");
		else
			strcpy(aximon_info.real_m1.id_name, "vo_rt");
	}

	switch (o1_hit_id) {
		case 0:
			strcpy(aximon_info.offline_m1.id_name, "vo_off");
			break;
		case 1:
			strcpy(aximon_info.offline_m1.id_name, "vi_off0");
			break;
		case 2:
			strcpy(aximon_info.offline_m1.id_name, "vi_off1");
			break;
		case 3:
			strcpy(aximon_info.offline_m1.id_name, "vda_off1");
			break;
		case 4:
			strcpy(aximon_info.offline_m1.id_name, "vda_off0");
			break;
		case 5:
			strcpy(aximon_info.offline_m1.id_name, "vdb_off1");
			break;
		case 6:
			strcpy(aximon_info.offline_m1.id_name, "vdb_off0");
			break;
		case 7:
			strcpy(aximon_info.offline_m1.id_name, "ve_off");
			break;
		case 8:
			strcpy(aximon_info.offline_m1.id_name, "hs_off");
			break;
		case 9:
			strcpy(aximon_info.offline_m1.id_name, "usb_off");
			break;
		case 10:
			strcpy(aximon_info.offline_m1.id_name, "pcie_off");
			break;
		case 11:
			strcpy(aximon_info.offline_m1.id_name, "ap_off");
			break;
		default:
			strcpy(aximon_info.offline_m1.id_name, "vo_off");
	}

	switch (o2_hit_id) {
		case 0:
			strcpy(aximon_info.offline_m2.id_name, "vo_off");
			break;
		case 1:
			strcpy(aximon_info.offline_m2.id_name, "vi_off0");
			break;
		case 2:
			strcpy(aximon_info.offline_m2.id_name, "vi_off1");
			break;
		case 3:
			strcpy(aximon_info.offline_m2.id_name, "vda_off1");
			break;
		case 4:
			strcpy(aximon_info.offline_m2.id_name, "vda_off0");
			break;
		case 5:
			strcpy(aximon_info.offline_m2.id_name, "vdb_off1");
			break;
		case 6:
			strcpy(aximon_info.offline_m2.id_name, "vdb_off0");
			break;
		case 7:
			strcpy(aximon_info.offline_m2.id_name, "ve_off");
			break;
		case 8:
			strcpy(aximon_info.offline_m2.id_name, "hs_off");
			break;
		case 9:
			strcpy(aximon_info.offline_m2.id_name, "usb_off");
			break;
		case 10:
			strcpy(aximon_info.offline_m2.id_name, "pcie_off");
			break;
		case 11:
			strcpy(aximon_info.offline_m2.id_name, "ap_off");
			break;
		default:
			strcpy(aximon_info.offline_m2.id_name, "vo_off");
	}

	strcpy(aximon_info.offline_m3.id_name, "off_p1");
	strcpy(aximon_info.offline_m4.id_name, "off_p2");
	strcpy(aximon_info.offline_total.id_name, "off_p1 + off_p2");

	strcpy(aximon_info.bulk_m1.id_name, "bulk_p1");
	strcpy(aximon_info.bulk_m2.id_name, "bulk_p2");
	strcpy(aximon_info.bulk_total.id_name, "bulk_p1 + bulk_p2");
#if 0
	if (ds_input_clk == 1) {
		strcpy(aximon_info.ddrsys1_m1.id_name, "rt");
		strcpy(aximon_info.ddrsys2_m1.id_name, "rt");
	} else if (ds_input_clk == 2)
	{
		strcpy(aximon_info.ddrsys1_m1.id_name, "off_p1");
		strcpy(aximon_info.ddrsys2_m1.id_name, "off_p1");
	} else if (ds_input_clk == 3)
	{
		strcpy(aximon_info.ddrsys1_m1.id_name, "off_p2");
		strcpy(aximon_info.ddrsys2_m1.id_name, "off_p2");
	}else if (ds_input_clk == 4)
	{
		strcpy(aximon_info.ddrsys1_m1.id_name, "bulk");
		strcpy(aximon_info.ddrsys2_m1.id_name, "bulk");
	} else {
		strcpy(aximon_info.ddrsys1_m1.id_name, "unknown");
		strcpy(aximon_info.ddrsys2_m1.id_name, "unknown");
	}
#endif
}
void show_key_reg_setting(void)
{
	//real
	pr_err("real_w:00 = 0x%08x, real_w:04 = 0x%08x, real_w:1c = 0x%08x.\n",readl(iomem_aximon_base + AXIMON_real_M1_WRITE), readl(iomem_aximon_base + AXIMON_real_M1_WRITE+0x4), readl(iomem_aximon_base + AXIMON_real_M1_WRITE+0x1c));
	pr_err("real_r:00 = 0x%08x, real_r:04 = 0x%08x, real_r:1c = 0x%08x.\n",readl(iomem_aximon_base + AXIMON_real_M1_READ), readl(iomem_aximon_base + AXIMON_real_M1_READ+0x4), readl(iomem_aximon_base + AXIMON_real_M1_READ+0x1c));

	//offline
	pr_err("off_m1_w:00 = 0x%08x, off_m1_w:04 = 0x%08x, off_m1_w:1c = 0x%08x.\n",readl(iomem_aximon_offline_base + AXIMON_offline_M1_WRITE), readl(iomem_aximon_offline_base + AXIMON_offline_M1_WRITE+0x4), readl(iomem_aximon_offline_base + AXIMON_offline_M1_WRITE+0x1c));
	pr_err("off_m1_r:00 = 0x%08x, off_m1_r:04 = 0x%08x, off_m1_r:1c = 0x%08x.\n",readl(iomem_aximon_offline_base + AXIMON_offline_M1_READ), readl(iomem_aximon_offline_base + AXIMON_offline_M1_READ+0x4), readl(iomem_aximon_offline_base + AXIMON_offline_M1_READ+0x1c));

	pr_err("off_m2_w:00 = 0x%08x, off_m2_w:04 = 0x%08x, off_m2_w:1c = 0x%08x.\n",readl(iomem_aximon_offline_base + AXIMON_offline_M2_WRITE), readl(iomem_aximon_offline_base + AXIMON_offline_M2_WRITE+0x4), readl(iomem_aximon_offline_base + AXIMON_offline_M2_WRITE+0x1c));
	pr_err("off_m2_r:00 = 0x%08x, off_m2_r:04 = 0x%08x, off_m2_r:1c = 0x%08x.\n",readl(iomem_aximon_offline_base + AXIMON_offline_M2_READ), readl(iomem_aximon_offline_base + AXIMON_offline_M2_READ+0x4), readl(iomem_aximon_offline_base + AXIMON_offline_M2_READ+0x1c));

	pr_err("off_m3_w:00 = 0x%08x, off_m3_w:04 = 0x%08x, off_m3_w:1c = 0x%08x.\n",readl(iomem_aximon_offline_base + AXIMON_offline_M3_WRITE), readl(iomem_aximon_offline_base + AXIMON_offline_M3_WRITE+0x4), readl(iomem_aximon_offline_base + AXIMON_offline_M3_WRITE+0x1c));
	pr_err("off_m3_r:00 = 0x%08x, off_m3_r:04 = 0x%08x, off_m3_r:1c = 0x%08x.\n",readl(iomem_aximon_offline_base + AXIMON_offline_M3_READ), readl(iomem_aximon_offline_base + AXIMON_offline_M3_READ+0x4), readl(iomem_aximon_offline_base + AXIMON_offline_M3_READ+0x1c));

	pr_err("off_m4_w:00 = 0x%08x, off_m4_w:04 = 0x%08x, off_m4_w:1c = 0x%08x.\n",readl(iomem_aximon_offline_base + AXIMON_offline_M4_WRITE), readl(iomem_aximon_offline_base + AXIMON_offline_M4_WRITE+0x4), readl(iomem_aximon_offline_base + AXIMON_offline_M4_WRITE+0x1c));
	pr_err("off_m4_r:00 = 0x%08x, off_m4_r:04 = 0x%08x, off_m4_r:1c = 0x%08x.\n",readl(iomem_aximon_offline_base + AXIMON_offline_M4_READ), readl(iomem_aximon_offline_base + AXIMON_offline_M4_READ+0x4), readl(iomem_aximon_offline_base + AXIMON_offline_M4_READ+0x1c));

	//bulk
	pr_err("bulk_m1_w:00 = 0x%08x, bulk_m1_w:04 = 0x%08x, bulk_m1_w:1c = 0x%08x.\n",readl(iomem_aximon_bulk_base + AXIMON_bulk_M1_WRITE), readl(iomem_aximon_bulk_base + AXIMON_bulk_M1_WRITE+0x4), readl(iomem_aximon_bulk_base + AXIMON_bulk_M1_WRITE+0x1c));
	pr_err("bulk_m1_r:00 = 0x%08x, bulk_m1_r:04 = 0x%08x, bulk_m1_r:1c = 0x%08x.\n",readl(iomem_aximon_bulk_base + AXIMON_bulk_M1_READ), readl(iomem_aximon_bulk_base + AXIMON_bulk_M1_READ+0x4), readl(iomem_aximon_bulk_base + AXIMON_bulk_M1_READ+0x1c));

	pr_err("bulk_m2_w:00 = 0x%08x, bulk_m2_w:04 = 0x%08x, bulk_m2_w:1c = 0x%08x.\n",readl(iomem_aximon_bulk_base + AXIMON_bulk_M2_WRITE), readl(iomem_aximon_bulk_base + AXIMON_bulk_M2_WRITE+0x4), readl(iomem_aximon_bulk_base + AXIMON_bulk_M2_WRITE+0x1c));
	pr_err("bulk_m2_r:00 = 0x%08x, bulk_m2_r:04 = 0x%08x, bulk_m2_r:1c = 0x%08x.\n",readl(iomem_aximon_bulk_base + AXIMON_bulk_M2_READ), readl(iomem_aximon_bulk_base + AXIMON_bulk_M2_READ+0x4), readl(iomem_aximon_bulk_base + AXIMON_bulk_M2_READ+0x1c));
}

void axi_mon_start_all(void)
{
	axi_mon_real_start(AXIMON_real_M1_WRITE);
	axi_mon_real_start(AXIMON_real_M1_READ);

	axi_mon_offline_start(AXIMON_offline_M1_WRITE);
	axi_mon_offline_start(AXIMON_offline_M1_READ);
	axi_mon_offline_start(AXIMON_offline_M2_WRITE);
	axi_mon_offline_start(AXIMON_offline_M2_READ);
	axi_mon_offline_start(AXIMON_offline_M3_WRITE);
	axi_mon_offline_start(AXIMON_offline_M3_READ);
	axi_mon_offline_start(AXIMON_offline_M4_WRITE);
	axi_mon_offline_start(AXIMON_offline_M4_READ);

	axi_mon_bulk_start(AXIMON_bulk_M1_WRITE);
	axi_mon_bulk_start(AXIMON_bulk_M1_READ);
	axi_mon_bulk_start(AXIMON_bulk_M2_WRITE);
	axi_mon_bulk_start(AXIMON_bulk_M2_READ);

	//axi_mon_ddrsys1_start(DDRMON_sys1_M1_WRITE);
	//axi_mon_ddrsys1_start(DDRMON_sys1_M1_READ);
	//axi_mon_ddrsys2_start(DDRMON_sys2_M1_WRITE);
	//axi_mon_ddrsys2_start(DDRMON_sys2_M1_READ);

	ktime_get_boottime_ts64(&ts_start);

	//ts_start1 = ktime_to_timespec64(ktime_get());

	//show_key_reg_setting();
}

void axi_mon_stop_all(void)
{
	axi_mon_real_stop(AXIMON_real_M1_WRITE);
	axi_mon_real_stop(AXIMON_real_M1_READ);

	axi_mon_offline_stop(AXIMON_offline_M1_WRITE);
	axi_mon_offline_stop(AXIMON_offline_M1_READ);
	axi_mon_offline_stop(AXIMON_offline_M2_WRITE);
	axi_mon_offline_stop(AXIMON_offline_M2_READ);
	axi_mon_offline_stop(AXIMON_offline_M3_WRITE);
	axi_mon_offline_stop(AXIMON_offline_M3_READ);
	axi_mon_offline_stop(AXIMON_offline_M4_WRITE);
	axi_mon_offline_stop(AXIMON_offline_M4_READ);

	axi_mon_bulk_stop(AXIMON_bulk_M1_WRITE);
	axi_mon_bulk_stop(AXIMON_bulk_M1_READ);
	axi_mon_bulk_stop(AXIMON_bulk_M2_WRITE);
	axi_mon_bulk_stop(AXIMON_bulk_M2_READ);

	//axi_mon_ddrsys1_stop(DDRMON_sys1_M1_WRITE);
	//axi_mon_ddrsys1_stop(DDRMON_sys1_M1_READ);
	//axi_mon_ddrsys2_stop(DDRMON_sys2_M1_WRITE);
	//axi_mon_ddrsys2_stop(DDRMON_sys2_M1_READ);

	ktime_get_boottime_ts64(&ts_end);
	//ts_end1 = ktime_to_timespec64(ktime_get());
	ts_delta = timespec64_sub(ts_end, ts_start);
	//ts_delta1 = timespec64_sub(ts_end1, ts_start1);

	pr_err("[DB] monitor time consumed: %lld (ns) = %lld (us)\n", timespec64_to_ns(&ts_delta), timespec64_to_ns(&ts_delta) / 1000);
	//pr_err("[DB] monitor time consumed: %lld (ns) = %lld (us)\n", timespec64_to_ns(&ts_delta1), timespec64_to_ns(&ts_delta1) / 1000);
}

void axi_mon_snapshot_all(void)
{
	axi_mon_real_snapshot(AXIMON_real_M1_WRITE);
	axi_mon_real_snapshot(AXIMON_real_M1_READ);

	axi_mon_offline_snapshot(AXIMON_offline_M1_WRITE);
	axi_mon_offline_snapshot(AXIMON_offline_M1_READ);
	axi_mon_offline_snapshot(AXIMON_offline_M2_WRITE);
	axi_mon_offline_snapshot(AXIMON_offline_M2_READ);
	axi_mon_offline_snapshot(AXIMON_offline_M3_WRITE);
	axi_mon_offline_snapshot(AXIMON_offline_M3_READ);
	axi_mon_offline_snapshot(AXIMON_offline_M4_WRITE);
	axi_mon_offline_snapshot(AXIMON_offline_M4_READ);

	axi_mon_bulk_snapshot(AXIMON_bulk_M1_WRITE);
	axi_mon_bulk_snapshot(AXIMON_bulk_M1_READ);
	axi_mon_bulk_snapshot(AXIMON_bulk_M2_WRITE);
	axi_mon_bulk_snapshot(AXIMON_bulk_M2_READ);

	//axi_mon_ddrsys1_snapshot(DDRMON_sys1_M1_WRITE);
	//axi_mon_ddrsys1_snapshot(DDRMON_sys1_M1_READ);
	//axi_mon_ddrsys2_snapshot(DDRMON_sys2_M1_WRITE);
	//axi_mon_ddrsys2_snapshot(DDRMON_sys2_M1_READ);
}

void axi_mon_get_info_all(uint32_t duration)
{
	uint32_t cur_byte_cnt = 0, sum_byte_cnt = 0;
	uint32_t snapshot_tick = 0;
	uint32_t cur_latency_write_cnt = 0, write_hit_cnt = 0;
	uint32_t cur_latency_read_cnt = 0, read_hit_cnt = 0;

	uint64_t cur_write_latency_his_cnt[11];
	uint64_t cur_read_latency_his_cnt[11];

	uint i = 0;

	snapshot_tick = readl(iomem_aximon_base + AXIMON_OFFSET_CYCLE);
	pr_debug("snapshot_tick=%d, dram_rate=%d\n", snapshot_tick, dram_info.data_rate);
	//pr_err("snapshot_tick=%d, dram_rate=%d\n", snapshot_tick, dram_info.data_rate);
	if (dram_info.data_rate) {
		//duration = snapshot_tick / (dram_info.data_rate >> 2);

		//real time
		duration = readl(iomem_aximon_base + AXIMON_real_M1_WRITE + AXIMON_OFFSET_CYCLE) / (aximon_info.real_m1.axi_clk);
		cur_byte_cnt = axi_mon_real_get_byte_cnts(AXIMON_real_M1_WRITE) + axi_mon_real_get_byte_cnts(AXIMON_real_M1_READ);
		axi_mon_count_port_info(duration, cur_byte_cnt, &aximon_info.real_m1);
		sum_byte_cnt += cur_byte_cnt;
		cur_latency_write_cnt = axi_mon_real_get_latency_cnts(AXIMON_real_M1_WRITE);
		cur_latency_read_cnt = axi_mon_real_get_latency_cnts(AXIMON_real_M1_READ);
		write_hit_cnt = axi_mon_real_get_hit_cnts(AXIMON_real_M1_WRITE);
		read_hit_cnt = axi_mon_real_get_hit_cnts(AXIMON_real_M1_READ);
		axi_mon_count_port_latency_info(cur_latency_write_cnt, write_hit_cnt, cur_latency_read_cnt, read_hit_cnt, &aximon_info.real_m1);

		for (i = 0; i <= 10; i++) {
			cur_write_latency_his_cnt[i] = axi_mon_real_get_latency_his_cnts(AXIMON_real_M1_WRITE, i);
			cur_read_latency_his_cnt[i] = axi_mon_real_get_latency_his_cnts(AXIMON_real_M1_READ, i);
		}
		axi_mon_count_port_latency_his_info(cur_write_latency_his_cnt, cur_read_latency_his_cnt, &aximon_info.real_m1);

		//offline
		sum_byte_cnt = 0;
		duration = readl(iomem_aximon_offline_base + AXIMON_offline_M1_WRITE + AXIMON_OFFSET_CYCLE) / (aximon_info.offline_m1.axi_clk);
		cur_byte_cnt = axi_mon_offline_get_byte_cnts(AXIMON_offline_M1_WRITE) + axi_mon_offline_get_byte_cnts(AXIMON_offline_M1_READ);
		axi_mon_count_port_info(duration, cur_byte_cnt, &aximon_info.offline_m1);
		//sum_byte_cnt += cur_byte_cnt;
		cur_latency_write_cnt = axi_mon_offline_get_latency_cnts(AXIMON_offline_M1_WRITE);
		cur_latency_read_cnt = axi_mon_offline_get_latency_cnts(AXIMON_offline_M1_READ);
		write_hit_cnt = axi_mon_offline_get_hit_cnts(AXIMON_offline_M1_WRITE);
		read_hit_cnt = axi_mon_offline_get_hit_cnts(AXIMON_offline_M1_READ);
		axi_mon_count_port_latency_info(cur_latency_write_cnt, write_hit_cnt, cur_latency_read_cnt, read_hit_cnt, &aximon_info.offline_m1);
		for (i = 0; i <= 10; i++) {
			cur_write_latency_his_cnt[i] = axi_mon_offline_get_latency_his_cnts(AXIMON_offline_M1_WRITE, i);
			cur_read_latency_his_cnt[i] = axi_mon_offline_get_latency_his_cnts(AXIMON_offline_M1_READ, i);
		}
		axi_mon_count_port_latency_his_info(cur_write_latency_his_cnt, cur_read_latency_his_cnt, &aximon_info.offline_m1);

		duration = readl(iomem_aximon_offline_base + AXIMON_offline_M2_WRITE + AXIMON_OFFSET_CYCLE) / (aximon_info.offline_m2.axi_clk);
		cur_byte_cnt = axi_mon_offline_get_byte_cnts(AXIMON_offline_M2_WRITE) + axi_mon_offline_get_byte_cnts(AXIMON_offline_M2_READ);
		axi_mon_count_port_info(duration, cur_byte_cnt, &aximon_info.offline_m2);
		//sum_byte_cnt += cur_byte_cnt;
		cur_latency_write_cnt = axi_mon_offline_get_latency_cnts(AXIMON_offline_M2_WRITE);
		cur_latency_read_cnt = axi_mon_offline_get_latency_cnts(AXIMON_offline_M2_READ);
		write_hit_cnt = axi_mon_offline_get_hit_cnts(AXIMON_offline_M2_WRITE);
		read_hit_cnt = axi_mon_offline_get_hit_cnts(AXIMON_offline_M2_READ);
		axi_mon_count_port_latency_info(cur_latency_write_cnt, write_hit_cnt, cur_latency_read_cnt, read_hit_cnt, &aximon_info.offline_m2);

		duration = readl(iomem_aximon_offline_base + AXIMON_offline_M3_WRITE + AXIMON_OFFSET_CYCLE) / (aximon_info.offline_m3.axi_clk);
		cur_byte_cnt = axi_mon_offline_get_byte_cnts(AXIMON_offline_M3_WRITE) + axi_mon_offline_get_byte_cnts(AXIMON_offline_M3_READ);
		axi_mon_count_port_info(duration, cur_byte_cnt, &aximon_info.offline_m3);
		sum_byte_cnt += cur_byte_cnt;
		cur_latency_write_cnt = axi_mon_offline_get_latency_cnts(AXIMON_offline_M3_WRITE);
		cur_latency_read_cnt = axi_mon_offline_get_latency_cnts(AXIMON_offline_M3_READ);
		write_hit_cnt = axi_mon_offline_get_hit_cnts(AXIMON_offline_M3_WRITE);
		read_hit_cnt = axi_mon_offline_get_hit_cnts(AXIMON_offline_M3_READ);
		axi_mon_count_port_latency_info(cur_latency_write_cnt, write_hit_cnt, cur_latency_read_cnt, read_hit_cnt, &aximon_info.offline_m3);

		duration = readl(iomem_aximon_offline_base + AXIMON_offline_M4_WRITE + AXIMON_OFFSET_CYCLE) / (aximon_info.offline_m4.axi_clk);
		cur_byte_cnt = axi_mon_offline_get_byte_cnts(AXIMON_offline_M4_WRITE) + axi_mon_offline_get_byte_cnts(AXIMON_offline_M4_READ);
		axi_mon_count_port_info(duration, cur_byte_cnt, &aximon_info.offline_m4);
		sum_byte_cnt += cur_byte_cnt;
		cur_latency_write_cnt = axi_mon_offline_get_latency_cnts(AXIMON_offline_M4_WRITE);
		cur_latency_read_cnt = axi_mon_offline_get_latency_cnts(AXIMON_offline_M4_READ);
		write_hit_cnt = axi_mon_offline_get_hit_cnts(AXIMON_offline_M4_WRITE);
		read_hit_cnt = axi_mon_offline_get_hit_cnts(AXIMON_offline_M4_READ);
		axi_mon_count_port_latency_info(cur_latency_write_cnt, write_hit_cnt, cur_latency_read_cnt, read_hit_cnt, &aximon_info.offline_m4);

		axi_mon_count_port_info(duration, sum_byte_cnt, &aximon_info.offline_total);

		//bulk
		sum_byte_cnt = 0;
		duration = readl(iomem_aximon_bulk_base + AXIMON_bulk_M1_WRITE + AXIMON_OFFSET_CYCLE) / (aximon_info.bulk_m1.axi_clk);
		cur_byte_cnt = axi_mon_bulk_get_byte_cnts(AXIMON_bulk_M1_WRITE) + axi_mon_bulk_get_byte_cnts(AXIMON_bulk_M1_READ);
		axi_mon_count_port_info(duration, cur_byte_cnt, &aximon_info.bulk_m1);
		sum_byte_cnt += cur_byte_cnt;
		cur_latency_write_cnt = axi_mon_bulk_get_latency_cnts(AXIMON_bulk_M1_WRITE);
		cur_latency_read_cnt = axi_mon_bulk_get_latency_cnts(AXIMON_bulk_M1_READ);
		write_hit_cnt = axi_mon_bulk_get_hit_cnts(AXIMON_bulk_M1_WRITE);
		read_hit_cnt = axi_mon_bulk_get_hit_cnts(AXIMON_bulk_M1_READ);
		axi_mon_count_port_latency_info(cur_latency_write_cnt, write_hit_cnt, cur_latency_read_cnt, read_hit_cnt, &aximon_info.bulk_m1);

		duration = readl(iomem_aximon_bulk_base + AXIMON_bulk_M2_WRITE + AXIMON_OFFSET_CYCLE) / (aximon_info.bulk_m2.axi_clk);
		cur_byte_cnt = axi_mon_bulk_get_byte_cnts(AXIMON_bulk_M2_WRITE) + axi_mon_bulk_get_byte_cnts(AXIMON_bulk_M2_READ);
		axi_mon_count_port_info(duration, cur_byte_cnt, &aximon_info.bulk_m2);
		sum_byte_cnt += cur_byte_cnt;
		cur_latency_write_cnt = axi_mon_bulk_get_latency_cnts(AXIMON_bulk_M2_WRITE);
		cur_latency_read_cnt = axi_mon_bulk_get_latency_cnts(AXIMON_bulk_M2_READ);
		write_hit_cnt = axi_mon_bulk_get_hit_cnts(AXIMON_bulk_M2_WRITE);
		read_hit_cnt = axi_mon_bulk_get_hit_cnts(AXIMON_bulk_M2_READ);
		axi_mon_count_port_latency_info(cur_latency_write_cnt, write_hit_cnt, cur_latency_read_cnt, read_hit_cnt, &aximon_info.bulk_m2);

		axi_mon_count_port_info(duration, sum_byte_cnt, &aximon_info.bulk_total);
#if 0
		//ddrsys
		cur_latency_write_cnt = axi_mon_ddrsys1_get_latency_cnts(DDRMON_sys1_M1_WRITE);
		cur_latency_read_cnt = axi_mon_ddrsys1_get_latency_cnts(DDRMON_sys1_M1_READ);
		write_hit_cnt = axi_mon_ddrsys1_get_hit_cnts(DDRMON_sys1_M1_WRITE);
		read_hit_cnt = axi_mon_ddrsys1_get_hit_cnts(DDRMON_sys1_M1_READ);
		axi_mon_count_port_latency_info(cur_latency_write_cnt, write_hit_cnt, cur_latency_read_cnt, read_hit_cnt, &aximon_info.ddrsys1_m1);
		for (i = 0; i <= 10; i++) {
			cur_write_latency_his_cnt[i] = axi_mon_ddrsys1_get_latency_his_cnts(DDRMON_sys1_M1_WRITE, i);
			cur_read_latency_his_cnt[i] = axi_mon_ddrsys1_get_latency_his_cnts(DDRMON_sys1_M1_READ, i);
		}
		axi_mon_count_port_latency_his_info(cur_write_latency_his_cnt, cur_read_latency_his_cnt, &aximon_info.ddrsys1_m1);

		cur_latency_write_cnt = axi_mon_ddrsys2_get_latency_cnts(DDRMON_sys2_M1_WRITE);
		cur_latency_read_cnt = axi_mon_ddrsys2_get_latency_cnts(DDRMON_sys2_M1_READ);
		write_hit_cnt = axi_mon_ddrsys2_get_hit_cnts(DDRMON_sys2_M1_WRITE);
		read_hit_cnt = axi_mon_ddrsys2_get_hit_cnts(DDRMON_sys2_M1_READ);
		axi_mon_count_port_latency_info(cur_latency_write_cnt, write_hit_cnt, cur_latency_read_cnt, read_hit_cnt, &aximon_info.ddrsys2_m1);
		for (i = 0; i <= 10; i++) {
			cur_write_latency_his_cnt[i] = axi_mon_ddrsys2_get_latency_his_cnts(DDRMON_sys2_M1_WRITE, i);
			cur_read_latency_his_cnt[i] = axi_mon_ddrsys2_get_latency_his_cnts(DDRMON_sys2_M1_READ, i);
		}
		axi_mon_count_port_latency_his_info(cur_write_latency_his_cnt, cur_read_latency_his_cnt, &aximon_info.ddrsys2_m1);
#endif

	} else {
		pr_err("read dram_rate=%d\n", dram_info.data_rate);
	}
}

void axi_mon_dump_single(struct AXIMON_INFO_PORT *port_info)
{
	//uint64_t total_write_lat_his = 0;
	//uint64_t total_read_lat_his = 0;
	//uint i = 0;
	uint64_t dividend = port_info->bw_avg_sum;
	uint64_t divedend_w_lat = port_info->latency_write_avg;
	uint64_t divedend_r_lat = port_info->latency_read_avg;

	do_div(dividend, port_info->count);
	do_div(divedend_w_lat, port_info->count);
	do_div(divedend_r_lat, port_info->count);
	pr_err("\n");

	if ((strcmp(port_info->port_name, "ddrsys1_m1") == 0) | (strcmp(port_info->port_name, "ddrsys2_m1") == 0)) {
		pr_err("%-8s : %-8s\n",	port_info->port_name, port_info->id_name);
		pr_err("avg_w_latency=%lluns, avg_w_latency_min=%uns, avg_w_latency_max=%uns\navg_r_latency=%lluns, avg_r_latency_min=%uns, avg_r_latency_max=%uns\n",
			divedend_w_lat, port_info->latency_write_avg_min, port_info->latency_write_avg_max,
			divedend_r_lat, port_info->latency_read_avg_min, port_info->latency_read_avg_max);

		pr_err("W_Lat_his 0~10: %lld, %lld, %lld, %lld, %lld, %lld, %lld, %lld, %lld, %lld, %lld\n",
								port_info->write_latency_his_cnt[0], port_info->write_latency_his_cnt[1],
								port_info->write_latency_his_cnt[2], port_info->write_latency_his_cnt[3],
								port_info->write_latency_his_cnt[4], port_info->write_latency_his_cnt[5],
								port_info->write_latency_his_cnt[6], port_info->write_latency_his_cnt[7],
								port_info->write_latency_his_cnt[8], port_info->write_latency_his_cnt[9],
								port_info->write_latency_his_cnt[10]);

		pr_err("R_Lat_his 0~10: %lld, %lld, %lld, %lld, %lld, %lld, %lld, %lld, %lld, %lld, %lld\n",
								port_info->read_latency_his_cnt[0], port_info->read_latency_his_cnt[1],
								port_info->read_latency_his_cnt[2], port_info->read_latency_his_cnt[3],
								port_info->read_latency_his_cnt[4], port_info->read_latency_his_cnt[5],
								port_info->read_latency_his_cnt[6], port_info->read_latency_his_cnt[7],
								port_info->read_latency_his_cnt[8], port_info->read_latency_his_cnt[9],
								port_info->read_latency_his_cnt[10]);
	} else {
		pr_err("%-8s : %-8s\n bw_avg=%5dMB/s, bw_min=%5dMB/s, bw_max=%5dMB/s\n",
								port_info->port_name, port_info->id_name, (uint32_t)dividend,
								port_info->bw_min, port_info->bw_max);

		if (((strcmp(port_info->port_name, "off_total") != 0) & (strcmp(port_info->port_name, "bulk_total") != 0) & (strcmp(port_info->port_name, "rt_m1") != 0)) | (strcmp(port_info->port_name, "rt_m1") == 0))
			pr_err("avg_w_latency=%5lluns, avg_w_latency_min=%5uns, avg_w_latency_max=%5uns\navg_r_latency=%5lluns, avg_r_latency_min=%5uns, avg_r_latency_max=%5uns\n",
			divedend_w_lat, port_info->latency_write_avg_min, port_info->latency_write_avg_max,
			divedend_r_lat, port_info->latency_read_avg_min, port_info->latency_read_avg_max);

		if ((strcmp(port_info->port_name, "rt_m1") == 0) | (strcmp(port_info->port_name, "off_m1") == 0)) {
			pr_err("W_Lat_his 0~10: %lld, %lld, %lld, %lld, %lld, %lld, %lld, %lld, %lld, %lld, %lld\n",
									port_info->write_latency_his_cnt[0], port_info->write_latency_his_cnt[1],
									port_info->write_latency_his_cnt[2], port_info->write_latency_his_cnt[3],
									port_info->write_latency_his_cnt[4], port_info->write_latency_his_cnt[5],
									port_info->write_latency_his_cnt[6], port_info->write_latency_his_cnt[7],
									port_info->write_latency_his_cnt[8], port_info->write_latency_his_cnt[9],
									port_info->write_latency_his_cnt[10]);

			pr_err("R_Lat_his 0~10: %lld, %lld, %lld, %lld, %lld, %lld, %lld, %lld, %lld, %lld, %lld\n",
									port_info->read_latency_his_cnt[0], port_info->read_latency_his_cnt[1],
									port_info->read_latency_his_cnt[2], port_info->read_latency_his_cnt[3],
									port_info->read_latency_his_cnt[4], port_info->read_latency_his_cnt[5],
									port_info->read_latency_his_cnt[6], port_info->read_latency_his_cnt[7],
									port_info->read_latency_his_cnt[8], port_info->read_latency_his_cnt[9],
									port_info->read_latency_his_cnt[10]);
		}
	}
}

void axi_mon_dump(void)
{
	pr_err("==============================\n");
	pr_err("%s profiling window time_avg=%3dms\n",
		dram_info.type, aximon_info.offline_total.time_avg / 1000);

	axi_mon_dump_single(&aximon_info.real_m1);

	axi_mon_dump_single(&aximon_info.offline_m1);
	axi_mon_dump_single(&aximon_info.offline_m2);
	axi_mon_dump_single(&aximon_info.offline_m3);
	axi_mon_dump_single(&aximon_info.offline_m4);
	axi_mon_dump_single(&aximon_info.offline_total);

	axi_mon_dump_single(&aximon_info.bulk_m1);
	axi_mon_dump_single(&aximon_info.bulk_m2);
	axi_mon_dump_single(&aximon_info.bulk_total);

	//axi_mon_dump_single(&aximon_info.ddrsys1_m1);
	//axi_mon_dump_single(&aximon_info.ddrsys2_m1);
	pr_err("==============================\n");
}

//#define CLK_DIVIDEND_MAGIC 0x1770000000
//#define CLK_RATE_OFFSET 0x34
#define CLK_DRAMRATE_REG_MAGIC (0x70006000 + 0x84)

static void axi_mon_get_dram_freq(struct cvi_mon_device *ndev)
{
#if 1
	uint32_t reg = 0;
	char tmp_str[DRAMTYPE_STRLEN];
	//uint64_t dividend = CLK_DIVIDEND_MAGIC;
	void __iomem *io_remapping;

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
	io_remapping = ioremap(CLK_DRAMRATE_REG_MAGIC, 4);
#else
	io_remapping = ioremap_nocache(CLK_DRAMRATE_REG_MAGIC, 4);
#endif
	reg = readl(io_remapping);
	pr_debug("axi_mon_get_dram_freq() reg=%x\n", reg);
	iounmap(io_remapping);

	dram_info.data_rate = 4266; //default: LP4/4x 4266

	if ((reg & 0xffffff) == 0x004fa0)
		dram_info.data_rate = 4266;

	if ((reg & 0xffffff) == 0x006800)
		dram_info.data_rate = 3733;

	if ((reg & 0xffffff) == 0x008d8f)
		dram_info.data_rate = 3200;

	if ((reg & 0xffffff) == 0x0FBA9)
		dram_info.data_rate = 2400;

	//cv181x specific
	dram_info.data_rate = dram_info.data_rate >> 1;

	snprintf(tmp_str, DRAMTYPE_STRLEN, "_%dMhz", dram_info.data_rate);
	strncat(dram_info.type, tmp_str, DRAMTYPE_STRLEN);
#else
	char tmp_str[DRAMTYPE_STRLEN];
	//cv181x specific
	dram_info.data_rate = 4266;

	snprintf(tmp_str, DRAMTYPE_STRLEN, "_%dMhz", dram_info.data_rate);
	strncat(dram_info.type, tmp_str, DRAMTYPE_STRLEN);
#endif
}


static void axi_mon_get_ddr_bus_width(struct cvi_mon_device *ndev)
{
	uint32_t reg = 0;

	reg = readl(ndev->ddr_ctrl_vaddr);

	if (((reg >> 12) & 0x3) == 0) {
		dram_info.bus_width = 32;
		strncat(dram_info.type, "_32bit", DRAMTYPE_STRLEN);
	} else if (((reg >> 12) & 0x3) == 1) {
		dram_info.bus_width = 16;
		strncat(dram_info.type, "_16bit", DRAMTYPE_STRLEN);
	} else
		pr_err("get DDR bus width error, value=(0x%08X)\n", reg);
}

static void axi_mon_get_dram_type(struct cvi_mon_device *ndev)
{
	uint32_t reg = 0;

	reg = readl(ndev->ddr_ctrl_vaddr);

	if (reg & 0x1)
		strncpy(dram_info.type, "DDR3", DRAMTYPE_STRLEN);
	else if (reg & 0x10)
	{
		strncpy(dram_info.type, "DDR4", DRAMTYPE_STRLEN);
	}
	else if (reg & 0x20)
	{
		strncpy(dram_info.type, "LPDDR4/4x", DRAMTYPE_STRLEN);
	}
	else
	{
		strncpy(dram_info.type, "DDR2", DRAMTYPE_STRLEN);
	}

}

#define TOP_FABRIC 0x6fff0000
#define OFFSET_AXI_CG_EN 0x4c
#define AXIMON_BIT 0x7

static void axi_mon_cg_en(uint8_t enable)
{
	void __iomem *reg_top_fabric;
	uint32_t value;

	reg_top_fabric = ioremap(TOP_FABRIC, PAGE_SIZE);

	if (IS_ERR(reg_top_fabric)) {
		pr_err("axi_mon_cg_en remap failed\n");
		return;
	}

	value = readl(reg_top_fabric + OFFSET_AXI_CG_EN);

	if (enable)
		writel((value | AXIMON_BIT), (reg_top_fabric + OFFSET_AXI_CG_EN));
	else
		writel((value & ~AXIMON_BIT), (reg_top_fabric + OFFSET_AXI_CG_EN));

	iounmap(reg_top_fabric);
}

//#define DDRSYS1_MON 0x7000a000
//#define DDRSYS2_MON 0x7800a000
#define OFFSET_DDRMON_CG_EN 0x14
#define DDRMON_BIT 0x80
#if 0
static void DDR_mon_cg_en(uint8_t enable)
{
	void __iomem *reg_ddrsys1_mon, *reg_ddrsys2_mon;
	uint32_t value1, value2;

	reg_ddrsys1_mon = ioremap(DDRSYS1_MON, PAGE_SIZE);
	reg_ddrsys2_mon = ioremap(DDRSYS2_MON, PAGE_SIZE);

	if (IS_ERR(reg_ddrsys1_mon) | IS_ERR(reg_ddrsys2_mon)) {
		pr_err("ddr_mon_cg_en remap failed\n");
		return;
	}

	value1 = readl(reg_ddrsys1_mon + OFFSET_DDRMON_CG_EN);
	value2 = readl(reg_ddrsys2_mon + OFFSET_DDRMON_CG_EN);

	if (enable) {
		writel((value1 | DDRMON_BIT), (reg_ddrsys1_mon + OFFSET_DDRMON_CG_EN));
		writel((value2 | DDRMON_BIT), (reg_ddrsys2_mon + OFFSET_DDRMON_CG_EN));
	} else {
		writel((value1 & ~DDRMON_BIT), (reg_ddrsys1_mon + OFFSET_DDRMON_CG_EN));
		writel((value2 & ~DDRMON_BIT), (reg_ddrsys2_mon + OFFSET_DDRMON_CG_EN));
	}
	iounmap(reg_ddrsys1_mon);
	iounmap(reg_ddrsys2_mon);
}
#endif
void show_init_setting(void)
{
	pr_err("r_hit_sel = %d\n", r_hit_sel);
	if((r_hit_sel != 0) & (r_hit_sel != 2))
	{
		r_hit_sel =0;
		pr_err("r_hit_sel should be 0 or 2, set to 0.\n");
	}

	pr_err("r_hit_id = %d\n", r_hit_id);
	if((r_hit_id != 0) & (r_hit_id !=1))
	{
		r_hit_id = 0;
		pr_err("r_hit_id should be 0 or 1, set to 0.\n");
	}

	pr_err("o1_input_clk = %d\n", o1_input_clk);
	if((o1_input_clk !=1) & (o1_input_clk !=2))
	{
		o1_input_clk = 1;
		pr_err("o1_input_clk should be 1 or 2, set to 1.\n");
	}
	if(o1_input_clk == 1)
		AXIMON_offline_m1_SELECT_CLK = 0x01000100;
	if(o1_input_clk == 2)
		AXIMON_offline_m1_SELECT_CLK = 0x02000200;

	pr_err("o1_hit_id = %d\n", o1_hit_id);
	if((o1_hit_id < 0) & (o1_hit_id > 11)) {
		o1_hit_id = 0;
		pr_err("o1_hit_id should be 0~11, set to 0.\n");
	}

	pr_err("o2_input_clk = %d\n", o2_input_clk);
	if((o2_input_clk !=1) & (o2_input_clk !=2))
	{
		o2_input_clk = 1;
		pr_err("o2_input_clk should be 1 or 2, set to 1.\n");
	}
	if(o2_input_clk == 1)
		AXIMON_offline_m2_SELECT_CLK = 0x01000100;
	if(o2_input_clk == 2)
		AXIMON_offline_m2_SELECT_CLK = 0x02000200;

	pr_err("o2_hit_id = %d\n", o2_hit_id);
	if((o2_hit_id < 0) & (o2_hit_id > 11)) {
		o2_hit_id = 0;
		pr_err("o2_hit_id should be 0~11, set to 0.\n");
	}

	pr_err("ds_input_clk = %d\n", ds_input_clk);
	if((ds_input_clk < 1) | (ds_input_clk > 4))
	{
		ds_input_clk = 1;
		pr_err("ds_input_clk should be 1~4, set to 1.\n");
	}
#if 0
	if(ds_input_clk == 1) {
		DDRMON_sys1_m1_SELECT_CLK = 0x01000100;
		DDRMON_sys2_m1_SELECT_CLK = 0x01000100;
	}
	if(ds_input_clk == 2) {
		DDRMON_sys1_m1_SELECT_CLK = 0x02000200;
		DDRMON_sys2_m1_SELECT_CLK = 0x02000200;
	}
	if(ds_input_clk == 3) {
		DDRMON_sys1_m1_SELECT_CLK = 0x03000300;
		DDRMON_sys2_m1_SELECT_CLK = 0x03000300;
	}
	if(ds_input_clk == 4) {
		DDRMON_sys1_m1_SELECT_CLK = 0x04000400;
		DDRMON_sys2_m1_SELECT_CLK = 0x04000400;
	}
#endif
}

void axi_mon_init(struct cvi_mon_device *ndev)
{
	iomem_aximon_base = ndev->ddr_aximon_real_vaddr;
	iomem_aximon_offline_base = ndev->ddr_aximon_offline_vaddr;
	iomem_aximon_bulk_base = ndev->ddr_aximon_bulk_vaddr;
	//iomem_ddrmon_sys1_base = ndev->ddr_ddrmon_sys1_vaddr;
	//iomem_ddrmon_sys2_base = ndev->ddr_ddrmon_sys2_vaddr;

	memset(&dram_info, 0, sizeof(struct DRAM_INFO));
	axi_mon_set_lat_bin_size(0xf);
	axi_mon_cg_en(1);
	//DDR_mon_cg_en(1);
	axi_mon_get_dram_type(ndev);
	axi_mon_get_dram_freq(ndev);
	axi_mon_get_ddr_bus_width(ndev);

	show_init_setting();
}

