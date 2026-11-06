#include <linux/kernel.h>
#include <linux/io.h>
#include <linux/dma-mapping.h>
#include <linux/delay.h>
#include <linux/clk.h>
#include <linux/reset.h>
#include <linux/streamline_annotate.h>
#include <linux/clk.h>
#include <linux/version.h>
#include "mon_platform.h"

struct AXIMON_INFO_PORT {
	uint8_t port_name[16];
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
	uint32_t write_latency_his_cnt[11];
	uint32_t read_latency_his_cnt[11];
};


struct AXIMON_INFO {
	struct AXIMON_INFO_PORT m1;
	struct AXIMON_INFO_PORT m2;
	struct AXIMON_INFO_PORT m3;
	struct AXIMON_INFO_PORT m4;
	struct AXIMON_INFO_PORT m5;
	struct AXIMON_INFO_PORT m6;
	struct AXIMON_INFO_PORT total;
};

#define DRAMTYPE_STRLEN 30
struct DRAM_INFO {
	uint32_t bus_width;
	uint32_t data_rate;
	char type[DRAMTYPE_STRLEN];
};

static struct DRAM_INFO dram_info;
static struct AXIMON_INFO aximon_info;
static void __iomem *iomem_aximon_base;

//#define AXIMON_BASE 0x08008000
#define REMAPPING_BASE 0
#define AXIMON_M1_WRITE	(REMAPPING_BASE + 0x0)
#define AXIMON_M1_READ	(REMAPPING_BASE + 0x80)
#define AXIMON_M2_WRITE	(REMAPPING_BASE + 0x100)
#define AXIMON_M2_READ	(REMAPPING_BASE + 0x180)
#define AXIMON_M3_WRITE	(REMAPPING_BASE + 0x200)
#define AXIMON_M3_READ	(REMAPPING_BASE + 0x280)
#define AXIMON_M4_WRITE	(REMAPPING_BASE + 0x300)
#define AXIMON_M4_READ	(REMAPPING_BASE + 0x380)
#define AXIMON_M5_WRITE	(REMAPPING_BASE + 0x400)
#define AXIMON_M5_READ	(REMAPPING_BASE + 0x480)
#define AXIMON_M6_WRITE	(REMAPPING_BASE + 0x500)
#define AXIMON_M6_READ	(REMAPPING_BASE + 0x580)

#define AXIMON_OFFSET_CYCLE 0x24
#define AXIMON_OFFSET_BYTECNTS 0x2C
#define AXIMON_OFFSET_LATENCYCNTS 0x34
#define AXIMON_OFFSET_HITCNTS 0x28
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
//#define AXIMON_OFFSET_LATCNTS 0x4C

#define AXIMON_SNAPSHOT_REGVALUE_1 0x40004
#define AXIMON_SNAPSHOT_REGVALUE_2 0x40000

#define AXIMON_START_REGVALUE 0x30001
#define AXIMON_STOP_REGVALUE 0x30002

#define AXIMON_SELECT_CLK 0x01000100

static void axi_mon_snapshot(uint32_t base_register)
{
	writel(AXIMON_SNAPSHOT_REGVALUE_1, (iomem_aximon_base + base_register));
	writel(AXIMON_SNAPSHOT_REGVALUE_2, (iomem_aximon_base + base_register));
}

static void axi_mon_start(uint32_t base_register)
{
	//writel(AXIMON_START_REGVALUE, (iomem_aximon_base + base_register));
}

static void axi_mon_stop(uint32_t base_register)
{
	//writel(AXIMON_STOP_REGVALUE, (iomem_aximon_base + base_register));
}

static void axi_mon_set_lat_bin_size(uint32_t set_value)
{
	uint32_t rdata;

	writel(set_value, (iomem_aximon_base + AXIMON_M1_WRITE + AXIMON_OFFSET_LAT_BIN_SIZE_SEL));
	writel(set_value, (iomem_aximon_base + AXIMON_M1_READ + AXIMON_OFFSET_LAT_BIN_SIZE_SEL));

	writel(0x02000200, (iomem_aximon_base + AXIMON_M1_WRITE + 0x00));
	rdata = readl((iomem_aximon_base + AXIMON_M1_WRITE + 0x04));
	rdata = rdata & 0xfffffc00;
	rdata = rdata | 0x00000002;
	writel(rdata, (iomem_aximon_base + AXIMON_M1_WRITE + 0x04));

	rdata = readl((iomem_aximon_base + AXIMON_M1_WRITE + 0x18));
	rdata = rdata & 0xff000000;
	rdata = rdata | 0x00007000;
	writel(rdata, (iomem_aximon_base + AXIMON_M1_WRITE + 0x18));

	rdata = readl((iomem_aximon_base + AXIMON_M1_WRITE + 0x1c));
	rdata = rdata & 0xff000000;
	rdata = rdata | 0x00001000;
	writel(rdata, (iomem_aximon_base + AXIMON_M1_WRITE + 0x1c));

	writel(0x02000200, (iomem_aximon_base + AXIMON_M1_READ + 0x00));
	rdata = readl((iomem_aximon_base + AXIMON_M1_READ + 0x04));
	rdata = rdata & 0xfffffc00;
	rdata = rdata | 0x00000002;
	writel(rdata, (iomem_aximon_base + AXIMON_M1_READ + 0x04));

	rdata = readl((iomem_aximon_base + AXIMON_M1_READ + 0x18));
	rdata = rdata & 0xff000000;
	rdata = rdata | 0x00007000;
	writel(rdata, (iomem_aximon_base + AXIMON_M1_READ + 0x18));

	rdata = readl((iomem_aximon_base + AXIMON_M1_READ + 0x1c));
	rdata = rdata & 0xff000000;
	rdata = rdata | 0x00001000;
	writel(rdata, (iomem_aximon_base + AXIMON_M1_READ + 0x1c));

	writel(set_value, (iomem_aximon_base + AXIMON_M2_WRITE + AXIMON_OFFSET_LAT_BIN_SIZE_SEL));
	writel(set_value, (iomem_aximon_base + AXIMON_M2_READ + AXIMON_OFFSET_LAT_BIN_SIZE_SEL));
	writel(set_value, (iomem_aximon_base + AXIMON_M3_WRITE + AXIMON_OFFSET_LAT_BIN_SIZE_SEL));
	writel(set_value, (iomem_aximon_base + AXIMON_M3_READ + AXIMON_OFFSET_LAT_BIN_SIZE_SEL));
	writel(set_value, (iomem_aximon_base + AXIMON_M4_WRITE + AXIMON_OFFSET_LAT_BIN_SIZE_SEL));
	writel(set_value, (iomem_aximon_base + AXIMON_M4_READ + AXIMON_OFFSET_LAT_BIN_SIZE_SEL));
	writel(set_value, (iomem_aximon_base + AXIMON_M5_WRITE + AXIMON_OFFSET_LAT_BIN_SIZE_SEL));
	writel(set_value, (iomem_aximon_base + AXIMON_M5_READ + AXIMON_OFFSET_LAT_BIN_SIZE_SEL));

	writel(0x02000200, (iomem_aximon_base + AXIMON_M5_WRITE + 0x00));
	rdata = readl((iomem_aximon_base + AXIMON_M5_WRITE + 0x04));
	rdata = rdata & 0xfffffc00;
	rdata = rdata | 0x00000002;
	writel(rdata, (iomem_aximon_base + AXIMON_M5_WRITE + 0x04));

	rdata = readl((iomem_aximon_base + AXIMON_M5_WRITE + 0x18));
	rdata = rdata & 0xff000000;
	rdata = rdata | 0x00007000;
	writel(rdata, (iomem_aximon_base + AXIMON_M5_WRITE + 0x18));

	rdata = readl((iomem_aximon_base + AXIMON_M5_WRITE + 0x1c));
	rdata = rdata & 0xff000000;
	rdata = rdata | 0x00005000;
	writel(rdata, (iomem_aximon_base + AXIMON_M5_WRITE + 0x1c));

	writel(0x02000200, (iomem_aximon_base + AXIMON_M5_READ + 0x00));
	rdata = readl((iomem_aximon_base + AXIMON_M5_READ + 0x04));
	rdata = rdata & 0xfffffc00;
	rdata = rdata | 0x00000002;
	writel(rdata, (iomem_aximon_base + AXIMON_M5_READ + 0x04));

	rdata = readl((iomem_aximon_base + AXIMON_M5_READ + 0x18));
	rdata = rdata & 0xff000000;
	rdata = rdata | 0x00007000;
	writel(rdata, (iomem_aximon_base + AXIMON_M5_READ + 0x18));

	rdata = readl((iomem_aximon_base + AXIMON_M5_READ + 0x1c));
	rdata = rdata & 0xff000000;
	rdata = rdata | 0x00005000;
	writel(rdata, (iomem_aximon_base + AXIMON_M5_READ + 0x1c));

	writel(set_value, (iomem_aximon_base + AXIMON_M6_WRITE + AXIMON_OFFSET_LAT_BIN_SIZE_SEL));
	writel(set_value, (iomem_aximon_base + AXIMON_M6_READ + AXIMON_OFFSET_LAT_BIN_SIZE_SEL));
}

static uint32_t axi_mon_get_byte_cnts(uint32_t base_register)
{
	return readl(iomem_aximon_base + base_register + AXIMON_OFFSET_BYTECNTS);
}

static uint32_t axi_mon_get_hit_cnts(uint32_t base_register)
{
	return readl(iomem_aximon_base + base_register + AXIMON_OFFSET_HITCNTS);
}

static uint32_t axi_mon_get_latency_cnts(uint32_t base_register)
{
	return readl(iomem_aximon_base + base_register + AXIMON_OFFSET_LATENCYCNTS);
}

static uint32_t axi_mon_get_latency_his_cnts(uint32_t base_register, uint32_t his_n)
{
	return readl(iomem_aximon_base + base_register + AXIMON_OFFSET_LATENCY_HIS_0 + 4*his_n);
}

static void axi_mon_count_port_info(uint32_t duration, uint32_t byte_cnt, struct AXIMON_INFO_PORT *axi_info)
{
	uint32_t bw = byte_cnt / duration;

	pr_debug("duration=%d, byte_cnt=%d, count=%d\n", duration, byte_cnt, axi_info->count);
	if (axi_info->count != 0) {
		if (bw) {
			if (!axi_info->bw_min)
				axi_info->bw_min = bw;
			else if (bw < axi_info->bw_min)
				axi_info->bw_min = bw;
		}

		if (bw > axi_info->bw_max)
			axi_info->bw_max = bw;

		axi_info->bw_avg_sum += bw;

		if (axi_info->time_avg)
			axi_info->time_avg = (axi_info->time_avg + duration) >> 1;
		else
			axi_info->time_avg = duration;
	}
	axi_info->count += 1;

#if 0
	pr_err("%s bw=%d, bw_min=%d, bw_max=%d, bw_avg_sum=%d\n",
								axi_info->port_name, bw,
								axi_info->bw_min, axi_info->bw_max,
								axi_info->bw_avg_sum);

#endif

}

static void axi_mon_count_port_latency_info(uint32_t latency_write_cnt, uint32_t write_hit_cnt,
		uint32_t latency_read_cnt, uint32_t read_hit_cnt, struct AXIMON_INFO_PORT *axi_info)
{
	uint32_t avg_latency = 0;

	//if(strcmp(axi_info->port_name,"tpu") == 0)
		//pr_err("latency_write_cnt=%d, write_hit_cnt=%d, latency_read_cnt=%d, read_hit_cnt=%d\n",
		//latency_write_cnt  , write_hit_cnt , latency_read_cnt , read_hit_cnt);

	if (write_hit_cnt != 0)
		avg_latency = 1000 * latency_write_cnt/((dram_info.data_rate/4)*write_hit_cnt);
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
		avg_latency = 1000 * latency_read_cnt/((dram_info.data_rate/4)*read_hit_cnt);
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
#if 0
	//if(strcmp(axi_info->port_name,"tpu")==0)
	//{
		//pr_err("Write:Latency avg=%d, Latency min=%d, Latency max=%d\n",
				//axi_info->latency_write_avg, axi_info->latency_write_avg_min,
				//axi_info->latency_write_avg_max);
		//pr_err("Read:Latency avg=%d, Latency min=%d, Latency max=%d\n",
				//axi_info->latency_read_avg, axi_info->latency_read_avg_min,
				//axi_info->latency_read_avg_max);
	//}
	//pr_err("read dram_rate=%d\n", dram_info.data_rate);
	//pr_err("avg_write_latency=%d ns\n", axi_info->latency_write_avg);
	//pr_err("avg_read_latency=%d ns\n", axi_info->latency_read_avg);
#endif
}


void axi_mon_reset_all(void)
{
	//reset aximon info
	memset(&aximon_info, 0, sizeof(struct AXIMON_INFO));

	//input source select configure
	writel(AXIMON_SELECT_CLK, (iomem_aximon_base + AXIMON_M1_WRITE));
	writel(AXIMON_SELECT_CLK, (iomem_aximon_base + AXIMON_M1_READ));
	writel(AXIMON_SELECT_CLK, (iomem_aximon_base + AXIMON_M2_WRITE));
	writel(AXIMON_SELECT_CLK, (iomem_aximon_base + AXIMON_M2_READ));
	writel(AXIMON_SELECT_CLK, (iomem_aximon_base + AXIMON_M3_WRITE));
	writel(AXIMON_SELECT_CLK, (iomem_aximon_base + AXIMON_M3_READ));
	writel(AXIMON_SELECT_CLK, (iomem_aximon_base + AXIMON_M4_WRITE));
	writel(AXIMON_SELECT_CLK, (iomem_aximon_base + AXIMON_M4_READ));
	writel(AXIMON_SELECT_CLK, (iomem_aximon_base + AXIMON_M5_WRITE));
	writel(AXIMON_SELECT_CLK, (iomem_aximon_base + AXIMON_M5_READ));
	writel(AXIMON_SELECT_CLK, (iomem_aximon_base + AXIMON_M6_WRITE));
	writel(AXIMON_SELECT_CLK, (iomem_aximon_base + AXIMON_M6_READ));

	//configure port name
	strcpy(aximon_info.m1.port_name, "vip_rt");
	strcpy(aximon_info.m2.port_name, "vip_off");
	strcpy(aximon_info.m3.port_name, "cpu");
	strcpy(aximon_info.m4.port_name, "tpu");
	strcpy(aximon_info.m5.port_name, "vc");
	strcpy(aximon_info.m6.port_name, "hsperi");
	strcpy(aximon_info.total.port_name, "total");
}

void axi_mon_start_all(void)
{
	axi_mon_start(AXIMON_M1_WRITE);
	axi_mon_start(AXIMON_M1_READ);
	axi_mon_start(AXIMON_M2_WRITE);
	axi_mon_start(AXIMON_M2_READ);
	axi_mon_start(AXIMON_M3_WRITE);
	axi_mon_start(AXIMON_M3_READ);
	axi_mon_start(AXIMON_M4_WRITE);
	axi_mon_start(AXIMON_M4_READ);
	axi_mon_start(AXIMON_M5_WRITE);
	axi_mon_start(AXIMON_M5_READ);
	axi_mon_start(AXIMON_M6_WRITE);
	axi_mon_start(AXIMON_M6_READ);
}

void axi_mon_stop_all(void)
{
	axi_mon_stop(AXIMON_M1_WRITE);
	axi_mon_stop(AXIMON_M1_READ);
	axi_mon_stop(AXIMON_M2_WRITE);
	axi_mon_stop(AXIMON_M2_READ);
	axi_mon_stop(AXIMON_M3_WRITE);
	axi_mon_stop(AXIMON_M3_READ);
	axi_mon_stop(AXIMON_M4_WRITE);
	axi_mon_stop(AXIMON_M4_READ);
	axi_mon_stop(AXIMON_M5_WRITE);
	axi_mon_stop(AXIMON_M5_READ);
	axi_mon_stop(AXIMON_M6_WRITE);
	axi_mon_stop(AXIMON_M6_READ);
}

void axi_mon_snapshot_all(void)
{
	axi_mon_snapshot(AXIMON_M1_WRITE);
	axi_mon_snapshot(AXIMON_M1_READ);
	axi_mon_snapshot(AXIMON_M2_WRITE);
	axi_mon_snapshot(AXIMON_M2_READ);
	axi_mon_snapshot(AXIMON_M3_WRITE);
	axi_mon_snapshot(AXIMON_M3_READ);
	axi_mon_snapshot(AXIMON_M4_WRITE);
	axi_mon_snapshot(AXIMON_M4_READ);
	axi_mon_snapshot(AXIMON_M5_WRITE);
	axi_mon_snapshot(AXIMON_M5_READ);
	axi_mon_snapshot(AXIMON_M6_WRITE);
	axi_mon_snapshot(AXIMON_M6_READ);
}

void dump_axi_mon_reg(uint32_t base_register)
{
	uint i = 0;
	for (i = 0; i <= 0x7c; i = i+0x10) {
		pr_err("0x%08x: 0x%08x, 0x%08x, 0x%08x, 0x%08x\n", iomem_aximon_base + base_register+i,
					readl(iomem_aximon_base + base_register+i),
					readl(iomem_aximon_base + base_register+i+0x4),
					readl(iomem_aximon_base + base_register+i+0x8),
					readl(iomem_aximon_base + base_register+i+0xc));
	}
}

void axi_mon_get_info_all(uint32_t duration)
{
	uint32_t cur_byte_cnt = 0, sum_byte_cnt = 0;
	uint32_t snapshot_tick = 0;

	uint32_t cur_latency_write_cnt = 0, write_hit_cnt = 0;
	uint32_t cur_latency_read_cnt = 0, read_hit_cnt = 0;

	uint i = 0;

	snapshot_tick = readl(iomem_aximon_base + AXIMON_OFFSET_CYCLE);
	pr_debug("snapshot_tick=%d, dram_rate=%d\n", snapshot_tick, dram_info.data_rate);

	if (dram_info.data_rate) {
		duration = snapshot_tick / (dram_info.data_rate >> 2);

		cur_byte_cnt = axi_mon_get_byte_cnts(AXIMON_M1_WRITE) + axi_mon_get_byte_cnts(AXIMON_M1_READ);
		axi_mon_count_port_info(duration, cur_byte_cnt, &aximon_info.m1);
		sum_byte_cnt += cur_byte_cnt;

		cur_latency_write_cnt = axi_mon_get_latency_cnts(AXIMON_M1_WRITE);
		cur_latency_read_cnt = axi_mon_get_latency_cnts(AXIMON_M1_READ);
		write_hit_cnt = axi_mon_get_hit_cnts(AXIMON_M1_WRITE);
		read_hit_cnt = axi_mon_get_hit_cnts(AXIMON_M1_READ);

		for (i = 0; i <= 10; i++) {
			aximon_info.m1.write_latency_his_cnt[i] = axi_mon_get_latency_his_cnts(AXIMON_M1_WRITE, i);
			aximon_info.m1.read_latency_his_cnt[i] = axi_mon_get_latency_his_cnts(AXIMON_M1_READ, i);
			//pr_err("write_latency_his_cnt=%5dns, read_latency_his_cnt=%5dns\n",
				//aximon_info.m1.write_latency_his_cnt[i],
				//aximon_info.m1.read_latency_his_cnt[i]);
		}
		axi_mon_count_port_latency_info(cur_latency_write_cnt, write_hit_cnt,
						cur_latency_read_cnt, read_hit_cnt, &aximon_info.m1);

		cur_byte_cnt = axi_mon_get_byte_cnts(AXIMON_M2_WRITE) + axi_mon_get_byte_cnts(AXIMON_M2_READ);
		axi_mon_count_port_info(duration, cur_byte_cnt, &aximon_info.m2);
		sum_byte_cnt += cur_byte_cnt;

		cur_latency_write_cnt = axi_mon_get_latency_cnts(AXIMON_M2_WRITE);
		cur_latency_read_cnt = axi_mon_get_latency_cnts(AXIMON_M2_READ);
		write_hit_cnt = axi_mon_get_hit_cnts(AXIMON_M2_WRITE);
		read_hit_cnt = axi_mon_get_hit_cnts(AXIMON_M2_READ);

		for (i = 0; i <= 10; i++) {
			aximon_info.m2.write_latency_his_cnt[i] = axi_mon_get_latency_his_cnts(AXIMON_M2_WRITE, i);
			aximon_info.m2.read_latency_his_cnt[i] = axi_mon_get_latency_his_cnts(AXIMON_M2_READ, i);
		}
		axi_mon_count_port_latency_info(cur_latency_write_cnt, write_hit_cnt,
						cur_latency_read_cnt, read_hit_cnt, &aximon_info.m2);

		cur_byte_cnt = axi_mon_get_byte_cnts(AXIMON_M3_WRITE) + axi_mon_get_byte_cnts(AXIMON_M3_READ);
		axi_mon_count_port_info(duration, cur_byte_cnt, &aximon_info.m3);
		sum_byte_cnt += cur_byte_cnt;

		cur_latency_write_cnt = axi_mon_get_latency_cnts(AXIMON_M3_WRITE);
		cur_latency_read_cnt = axi_mon_get_latency_cnts(AXIMON_M3_READ);
		write_hit_cnt = axi_mon_get_hit_cnts(AXIMON_M3_WRITE);
		read_hit_cnt = axi_mon_get_hit_cnts(AXIMON_M3_READ);
		for (i = 0; i <= 10; i++) {
			aximon_info.m3.write_latency_his_cnt[i] = axi_mon_get_latency_his_cnts(AXIMON_M3_WRITE, i);
			aximon_info.m3.read_latency_his_cnt[i] = axi_mon_get_latency_his_cnts(AXIMON_M3_READ, i);
		}
		axi_mon_count_port_latency_info(cur_latency_write_cnt, write_hit_cnt,
						cur_latency_read_cnt, read_hit_cnt, &aximon_info.m3);

		cur_byte_cnt = axi_mon_get_byte_cnts(AXIMON_M4_WRITE) + axi_mon_get_byte_cnts(AXIMON_M4_READ);
		axi_mon_count_port_info(duration, cur_byte_cnt, &aximon_info.m4);
		sum_byte_cnt += cur_byte_cnt;

		cur_latency_write_cnt = axi_mon_get_latency_cnts(AXIMON_M4_WRITE);
		cur_latency_read_cnt = axi_mon_get_latency_cnts(AXIMON_M4_READ);
		write_hit_cnt = axi_mon_get_hit_cnts(AXIMON_M4_WRITE);
		read_hit_cnt = axi_mon_get_hit_cnts(AXIMON_M4_READ);

		for (i = 0; i <= 10; i++) {
			aximon_info.m4.write_latency_his_cnt[i] = axi_mon_get_latency_his_cnts(AXIMON_M4_WRITE, i);
			aximon_info.m4.read_latency_his_cnt[i] = axi_mon_get_latency_his_cnts(AXIMON_M4_READ, i);
		}
		axi_mon_count_port_latency_info(cur_latency_write_cnt, write_hit_cnt,
						cur_latency_read_cnt, read_hit_cnt, &aximon_info.m4);

		cur_byte_cnt = axi_mon_get_byte_cnts(AXIMON_M5_WRITE) + axi_mon_get_byte_cnts(AXIMON_M5_READ);
		axi_mon_count_port_info(duration, cur_byte_cnt, &aximon_info.m5);
		sum_byte_cnt += cur_byte_cnt;

		cur_latency_write_cnt = axi_mon_get_latency_cnts(AXIMON_M5_WRITE);
		cur_latency_read_cnt = axi_mon_get_latency_cnts(AXIMON_M5_READ);
		write_hit_cnt = axi_mon_get_hit_cnts(AXIMON_M5_WRITE);
		read_hit_cnt = axi_mon_get_hit_cnts(AXIMON_M5_READ);

		for (i = 0; i <= 10; i++) {
			aximon_info.m5.write_latency_his_cnt[i] = axi_mon_get_latency_his_cnts(AXIMON_M5_WRITE, i);
			aximon_info.m5.read_latency_his_cnt[i] = axi_mon_get_latency_his_cnts(AXIMON_M5_READ, i);
		}
		axi_mon_count_port_latency_info(cur_latency_write_cnt, write_hit_cnt,
						cur_latency_read_cnt, read_hit_cnt, &aximon_info.m5);

		cur_byte_cnt = axi_mon_get_byte_cnts(AXIMON_M6_WRITE) + axi_mon_get_byte_cnts(AXIMON_M6_READ);
		axi_mon_count_port_info(duration, cur_byte_cnt, &aximon_info.m6);
		sum_byte_cnt += cur_byte_cnt;

		cur_latency_write_cnt = axi_mon_get_latency_cnts(AXIMON_M6_WRITE);
		cur_latency_read_cnt = axi_mon_get_latency_cnts(AXIMON_M6_READ);
		write_hit_cnt = axi_mon_get_hit_cnts(AXIMON_M6_WRITE);
		read_hit_cnt = axi_mon_get_hit_cnts(AXIMON_M6_READ);

		for (i = 0; i <= 10; i++) {
			aximon_info.m6.write_latency_his_cnt[i] = axi_mon_get_latency_his_cnts(AXIMON_M6_WRITE, i);
			aximon_info.m6.read_latency_his_cnt[i] = axi_mon_get_latency_his_cnts(AXIMON_M6_READ, i);
		}
		axi_mon_count_port_latency_info(cur_latency_write_cnt, write_hit_cnt,
						cur_latency_read_cnt, read_hit_cnt, &aximon_info.m6);

		axi_mon_count_port_info(duration, sum_byte_cnt, &aximon_info.total);
	} else {
		pr_err("read dram_rate=%d\n", dram_info.data_rate);
	}

	//pr_err("AXI mon 0 register dump after capture.\n");
	//dump_axi_mon_reg(AXIMON_M1_WRITE);
	//pr_err("AXI mon 1 register dump after capture.\n");
	//dump_axi_mon_reg(AXIMON_M1_READ);
}

void axi_mon_dump_single(struct AXIMON_INFO_PORT *port_info)
{
	uint64_t dividend = port_info->bw_avg_sum;
	uint64_t divedend_w_lat = port_info->latency_write_avg;
	uint64_t divedend_r_lat = port_info->latency_read_avg;

	do_div(dividend, port_info->count);
	do_div(divedend_w_lat, port_info->count);
	do_div(divedend_r_lat, port_info->count);
	pr_err("\n");
	pr_err("%-8s bw_avg=%5dMB/s, bw_min=%5dMB/s, bw_max=%5dMB/s\n",
							port_info->port_name, (uint32_t)dividend,
							port_info->bw_min, port_info->bw_max);

	if (strcmp(port_info->port_name, "total") != 0)
		pr_err("avg_write_latency=%5dns, avg_write_latency_min=%5dns, avg_write_latency_max=%5dns\navg_read_latency=%5dns, avg_read_latency_min=%5dns, avg_read_latency_max=%5dns\n",
		divedend_w_lat, port_info->latency_write_avg_min, port_info->latency_write_avg_max,
		divedend_r_lat, port_info->latency_read_avg_min, port_info->latency_read_avg_max);

	if (strcmp(port_info->port_name, "vip_rt") == 0 | strcmp(port_info->port_name, "vc") == 0) {
		pr_err("Write_Lat_histogram 0~10: %5d,%5d,%5d,%5d,%5d,%5d,%5d,%5d,%5d,%5d,%5d.\n",
			port_info->write_latency_his_cnt[0], port_info->write_latency_his_cnt[1],
			port_info->write_latency_his_cnt[2], port_info->write_latency_his_cnt[3],
			port_info->write_latency_his_cnt[4], port_info->write_latency_his_cnt[5],
			port_info->write_latency_his_cnt[6], port_info->write_latency_his_cnt[7],
			port_info->write_latency_his_cnt[8], port_info->write_latency_his_cnt[9],
			port_info->write_latency_his_cnt[10]);
		pr_err("Read_Lat_histogram 0~10: %5d,%5d,%5d,%5d,%5d,%5d,%5d,%5d,%5d,%5d,%5d.\n",
			port_info->read_latency_his_cnt[0], port_info->read_latency_his_cnt[1],
			port_info->read_latency_his_cnt[2], port_info->read_latency_his_cnt[3],
			port_info->read_latency_his_cnt[4], port_info->read_latency_his_cnt[5],
			port_info->read_latency_his_cnt[6], port_info->read_latency_his_cnt[7],
			port_info->read_latency_his_cnt[8], port_info->read_latency_his_cnt[9],
			port_info->read_latency_his_cnt[10]);
	}
}

void axi_mon_dump(void)
{
	pr_err("==============================\n");
	pr_err("%s profiling window time_avg=%3dms\n",
		dram_info.type, aximon_info.total.time_avg / 1000);

	axi_mon_dump_single(&aximon_info.m1);
	axi_mon_dump_single(&aximon_info.m2);
	axi_mon_dump_single(&aximon_info.m3);
	axi_mon_dump_single(&aximon_info.m4);
	axi_mon_dump_single(&aximon_info.m5);
	axi_mon_dump_single(&aximon_info.m6);
	axi_mon_dump_single(&aximon_info.total);
	pr_err("==============================\n");
}

#define CLK_DIVIDEND_MAGIC 0x1770000000
#define CLK_RATE_OFFSET 0x34
#define CLK_DRAMRATE_REG_MAGIC (0x03002900 + 0x54)
static void axi_mon_get_dram_freq(struct cvi_mon_device *ndev)
{
	uint32_t reg = 0;
	char tmp_str[DRAMTYPE_STRLEN];
	uint64_t dividend = CLK_DIVIDEND_MAGIC;
	void __iomem *io_remapping;

#if (KERNEL_VERSION(5, 10, 0) <= LINUX_VERSION_CODE)
	io_remapping = ioremap(CLK_DRAMRATE_REG_MAGIC, 4);
#else
	io_remapping = ioremap_nocache(CLK_DRAMRATE_REG_MAGIC, 4);
#endif
	reg = readl(io_remapping);
	pr_debug("axi_mon_get_dram_freq() reg=%x\n", reg);
	iounmap(io_remapping);

	do_div(dividend, reg);
	dram_info.data_rate = ((uint32_t)dividend) << 4;

	//cv181x specific
	dram_info.data_rate = dram_info.data_rate >> 1;

	snprintf(tmp_str, DRAMTYPE_STRLEN, "_%dMhz", dram_info.data_rate);
	strncat(dram_info.type, tmp_str, DRAMTYPE_STRLEN);
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
		strncpy(dram_info.type, "DDR4", DRAMTYPE_STRLEN);
	else if (reg & 0x20)
		strncpy(dram_info.type, "LPDDR4", DRAMTYPE_STRLEN);
	else
		strncpy(dram_info.type, "DDR2", DRAMTYPE_STRLEN);
}

#define DDR_TOP 0x0800A000
#define OFFSET_AXI_CG_EN 0x14
#define AXIMON_BIT 0x100

static void axi_mon_cg_en(uint8_t enable)
{
	void __iomem *reg_ddrtop;
	uint32_t value;

	reg_ddrtop = ioremap(DDR_TOP, PAGE_SIZE);

	if (IS_ERR(reg_ddrtop)) {
		pr_err("axi_mon_cg_en remap failed\n");
		return;
	}

	value = readl(reg_ddrtop + OFFSET_AXI_CG_EN);

	if (enable)
		writel((value | AXIMON_BIT), (reg_ddrtop + OFFSET_AXI_CG_EN));
	else
		writel((value & ~AXIMON_BIT), (reg_ddrtop + OFFSET_AXI_CG_EN));

	iounmap(reg_ddrtop);
}

void axi_mon_init(struct cvi_mon_device *ndev)
{
	iomem_aximon_base = ndev->ddr_aximon_vaddr;

	memset(&dram_info, 0, sizeof(struct DRAM_INFO));
	//axi_mon_cg_en(1);
	axi_mon_get_dram_type(ndev);
	axi_mon_get_dram_freq(ndev);
	axi_mon_get_ddr_bus_width(ndev);
	//axi_mon_set_lat_bin_size(0x5);
}

