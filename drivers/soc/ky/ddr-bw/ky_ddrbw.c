// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2024 Ky Co., Ltd.
 */
#include "linux/export.h"
#include <linux/clk.h>
#include <linux/clk-provider.h>
#include <linux/delay.h>
#include <linux/devfreq.h>
#include <linux/devfreq_cooling.h>
#include <linux/io.h>
#include <linux/jiffies.h>
#include <linux/kthread.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>
#include <linux/platform_device.h>
#include <linux/pm_qos.h>
#include <linux/regulator/machine.h>
#include <linux/slab.h>
#include <linux/spinlock.h>
#include <linux/string.h>
#include <linux/uaccess.h>
#include <linux/cdev.h>
#include "ky_ddrbw.h"

struct ddr_dev {
	struct cdev chrdevcdev;
	int major;
	dev_t dev;
	struct class *dev_class;
};
static struct ddr_dev ddrdev;

static u32 ddr_bw_info_index;
static struct ddr_perf_data *ddr_perf_data;
static struct ky_ddr_bw_info ddr_bw_info[DDR_FC_BW_SIZE];


static int get_ddr_burst(void __iomem *base)
{
	static bool flag = false;
	static int cmds_len = DDR_LEN_64;
	u32 burst_len, data_width;
	u32 val, tmp;

	if (flag)
		return cmds_len;

	val = readl(base + MC_CONTROL_0);
	tmp = (val >> MC_DATA_WIDTH) & 0x7;
	if (tmp == 0x2)
		data_width = DDR_LEN_16;
	else if (tmp == 0x3)
		data_width = DDR_LEN_32;
	else if (tmp == 0x4)
		data_width = DDR_LEN_64;
	else {
		pr_err("Wrong device data width\n");
		return cmds_len;
	}

	tmp = (val >> MC_BURST_LEN) & 0x7;
	if (tmp == 0x2)
		burst_len = 4;
	else if (tmp == 0x3)
		burst_len = 8;
	else if (tmp == 0x4)
		burst_len = 16;
	else {
		pr_err("Wrong device burst length\n");
		return cmds_len;
	}
	cmds_len = (burst_len * data_width) / 8;
	flag = true;
	return cmds_len;
}

static void ddraxi_mon_enable(struct ddraxi_mon *monitor, int enable)
{
	u32 mon_ctrl;

	if (monitor->master_is_lcd) {
		mon_ctrl = readl(ddraxi_mon_data->ciu_base + LCD_MON_BASE);
		if (enable)
			mon_ctrl |= (MON_CTRL_EN | MON_CTRL_LATCH) ;
		else
			mon_ctrl &= ~(MON_CTRL_EN | MON_CTRL_LATCH);

		writel(mon_ctrl, ddraxi_mon_data->ciu_base + LCD_MON_BASE);
		return;
	}
	if (monitor->id == 0) {
		mon_ctrl = readl(ddraxi_mon_data->reg_base	\
			+ mon_ctrl_reg(monitor->port_id));
		if (enable)
			mon_ctrl |= (MON_CTRL_EN | MON_CTRL_LATCH) ;
		else
			mon_ctrl &= ~(MON_CTRL_EN | MON_CTRL_LATCH);

		writel(mon_ctrl, ddraxi_mon_data->reg_base	\
			+ mon_ctrl_reg(monitor->port_id));
	}
	else {
		mon_ctrl = readl(ddraxi_mon_data->reg_base	\
			+ mon_id_reg(monitor->port_id));
		if (enable)
			mon_ctrl |= (1 << ((monitor->id - 1)  + IDMON_CTRL_EN));
		else
			mon_ctrl &= ~(1 << ((monitor->id - 1)  + IDMON_CTRL_EN));


		if (enable)
			mon_ctrl &= ~(1 << ((monitor->id - 1)  + IDMON_CLK_EN));
		else
			mon_ctrl |= (1 << ((monitor->id - 1)  + IDMON_CLK_EN));

		writel(mon_ctrl, ddraxi_mon_data->reg_base	\
			 + mon_id_reg(monitor->port_id));

		mon_ctrl = readl(ddraxi_mon_data->reg_base	\
			+ mon_id_sel(monitor->id, monitor->port_id));
		mon_ctrl = (0x3 << 16) | monitor->mid;
		writel(mon_ctrl, ddraxi_mon_data->reg_base 	\
			+ mon_id_sel(monitor->id, monitor->port_id));
	}

}

static void ddraxi_mon_latch(struct ddraxi_mon *monitor, int latch)
{
	u32 mon_ctrl;

	if (monitor->master_is_lcd)
		mon_ctrl = readl(ddraxi_mon_data->ciu_base + LCD_MON_BASE);
	else
		mon_ctrl = readl(ddraxi_mon_data->reg_base	\
			+ mon_ctrl_reg(monitor->port_id));

	if (latch)
		mon_ctrl |= MON_CTRL_READ;
	else
		mon_ctrl &= ~MON_CTRL_READ;
	if (monitor->master_is_lcd)
		writel(mon_ctrl, ddraxi_mon_data->ciu_base + LCD_MON_BASE);
	else
		writel(mon_ctrl, ddraxi_mon_data->reg_base	\
			+ mon_ctrl_reg(monitor->port_id));
}

static void ddraxi_mon_get_id_mon(struct ddraxi_mon *monitor)
{
	u32 mon_ctrl;

	mon_ctrl = readl(ddraxi_mon_data->reg_base + mon_id_reg(monitor->port_id));
	mon_ctrl &= ~KY_MON_ID_SEL;
	mon_ctrl |= monitor->id;
	writel(mon_ctrl, ddraxi_mon_data->reg_base + mon_id_reg(monitor->port_id));
}

static void ddraxi_mon_get_evt(struct ddraxi_mon *monitor)
{
	u32 mon_ctrl, i;
	u32 data;

	if (monitor->master_is_lcd)
		mon_ctrl = readl(ddraxi_mon_data->ciu_base + LCD_MON_BASE);
	else
		mon_ctrl = readl(ddraxi_mon_data->reg_base	\
			+ mon_ctrl_reg(monitor->port_id));

	for (i = 0; i < KY_MON_EVT_NUM; i++) {
		mon_ctrl &= ~KY_MON_EVT_MASK;
		mon_ctrl |= mon_evt_id[i];
		if (monitor->master_is_lcd) {
			writel(mon_ctrl, ddraxi_mon_data->ciu_base + LCD_MON_BASE);
			data = readl(ddraxi_mon_data->ciu_base	\
				+ LCD_MON_BASE + KY_MON_DATA);
			monitor->evt_num[i] = data;
		}
		else {
			writel(mon_ctrl, ddraxi_mon_data->reg_base	\
				+ mon_ctrl_reg(monitor->port_id));
			/* read monitor data register */
			monitor->evt_num[i] = readl(ddraxi_mon_data->reg_base	\
				+ mon_data_reg(monitor->port_id));
		}
	}
}

static void ddraxi_mon_get_lat(struct ddraxi_mon *monitor)
{
	u32 mon_ctrl, i;
	u32 data;

	if (monitor->master_is_lcd)
		mon_ctrl = readl(ddraxi_mon_data->ciu_base + LCD_MON_BASE);
	else
		mon_ctrl = readl(ddraxi_mon_data->reg_base	\
			+ mon_ctrl_reg(monitor->port_id));

	for (i = 0; i < KY_MON_LAT_NUM; i++) {
		mon_ctrl &= ~KY_MON_EVT_MASK;
		mon_ctrl |= mon_lat_id[i];
		if (monitor->master_is_lcd) {
			writel(mon_ctrl, ddraxi_mon_data->ciu_base + LCD_MON_BASE);
			data = readl(ddraxi_mon_data->ciu_base	\
				+ LCD_MON_BASE + KY_MON_DATA);
			monitor->lat_num[i] = data;
		}
		else {
			writel(mon_ctrl, ddraxi_mon_data->reg_base	\
				+ mon_ctrl_reg(monitor->port_id));
			/* read monitor data register */
			monitor->lat_num[i] = readl(ddraxi_mon_data->reg_base	\
				+ mon_data_reg(monitor->port_id));
		}


	}
}

struct ddraxi_mon *ddraxi_mon_get_mon(void)
{
	int i;

	for (i = 0; i < KY_ID_MON_CTRL_NUM + KY_MON_CTRL_NUM + 1; i++)
		if (ddraxi_mon_data->mon[i].free)
			return &ddraxi_mon_data->mon[i];
	return NULL;
}

int ddraxi_mon_enable_mon(struct ddraxi_mon *monitor)
{
	if (is_mon_valid(monitor->id) < 0 || !ddraxi_mon_data)
		return -EINVAL;

	monitor->init = true;

	spin_lock(&monitor->lock);
	monitor->free = false;

	/* enable the monitor */
	ddraxi_mon_enable(monitor, 1);

	spin_unlock(&monitor->lock);

	return 0;
}

int ddraxi_mon_update_mon(struct ddraxi_mon *monitor)
{
	if (!ddraxi_mon_data || is_mon_valid(monitor->id) < 0 || !monitor->init)
		return -EINVAL;

	/* disable monitor latch */
	ddraxi_mon_latch(monitor, 0);

	/* latch and update monitor */
	ddraxi_mon_latch(monitor, 1);

	if (monitor->master_is_lcd == false) {
		ddraxi_mon_get_id_mon(monitor);
	}
	ddraxi_mon_get_evt(monitor);

	if (monitor->lat_en)
		ddraxi_mon_get_lat(monitor);

	return 0;
}

int ddraxi_mon_disable_mon(struct ddraxi_mon *monitor)
{
	if (is_mon_valid(monitor->id) < 0 || !ddraxi_mon_data)
		return -EINVAL;

	/* read monitor data */
	ddraxi_mon_update_mon(monitor);

	spin_lock(&monitor->lock);
	/* disable the monitor */
	ddraxi_mon_enable(monitor, 0);

	monitor->free = true;
	spin_unlock(&monitor->lock);

	return 0;
}

static void ddraxi_mon_init(struct ky_ddraxi_mon_data *data)
{
	int i;

	for (i = 0; i < KY_MON_CTRL_NUM; i++) {
		data->mon[i].id = 0;
		data->mon[i].port_id = i;
		data->mon[i].lat_th = 0;
		data->mon[i].init = false;
		data->mon[i].enable = false;
		data->mon[i].lat_en = false;
		data->mon[i].free = true;
		data->mon[i].master_is_lcd = false;
		spin_lock_init(&data->mon[i].lock);

		writel(0, data->reg_base + mon_ctrl_reg(data->mon[i].port_id));
	}

	for (i = KY_MON_CTRL_NUM; i < KY_MON_CTRL_NUM	\
			+ KY_ID_MON_CTRL_NUM + 1; i++) {
		data->mon[i].id = ((i - 1) % 3) + 1;
		data->mon[i].lat_th = 0;
		data->mon[i].init = false;
		data->mon[i].enable = false;
		data->mon[i].lat_en = false;
		data->mon[i].free = true;
		/* last monitor for lcd */
		if (i == KY_MON_CTRL_NUM + KY_ID_MASTER_NUM)
			data->mon[i].master_is_lcd = true;
		spin_lock_init(&data->mon[i].lock);
	}

}

static int ddr_mon_enable(struct ddr_dfreq_data *data, int en)
{
	int ret, i;

	/* enable port monitor */
	if (((en == DDR_PORT_DEBUG_LAT) || (en == DDR_PORT_DEBUG_BW))	\
			&& !data->ddr_mon_debug) {

		for (i = 0; i < KY_MON_CTRL_NUM; i++) {
			data->axi_mon[i] = ddraxi_mon_get_mon();
			if (!data->axi_mon[i]) {
				pr_err("no free ddr axi mon\n");
				return -EINVAL;
			}

			/*
			 * each port get 3 id  monitors
			 * id 0 is configured to noraml port id
			 * without master id mask
			 */
			data->axi_mon[i]->id = 0;
			data->axi_mon[i]->port_id = i;
			data->axi_mon[i]->mid = 0;
			if(en == DDR_PORT_DEBUG_LAT)
				data->axi_mon[i]->lat_en = true;
			ret = ddraxi_mon_enable_mon(data->axi_mon[i]);
			if (ret < 0)
				return ret;
		}
		for (i = KY_MON_CTRL_NUM; i < KY_ID_MASTER_NUM	\
				+ KY_MON_CTRL_NUM; i++) {
			if((i == 10) || (i == 14) || (i == 18)) {
				data->axi_mon[i] = kmalloc(sizeof(struct ddraxi_mon), GFP_KERNEL);
				data->axi_mon[i]->id =  \
					((i - (KY_MON_CTRL_NUM + 3)) % 4) + 1;
				data->axi_mon[i]->port_id = ((i - 3) / 4);
				data->axi_mon[i]->mid = master_id[i - KY_MON_CTRL_NUM];
				continue;
			} else {
				data->axi_mon[i] = ddraxi_mon_get_mon();
				if (!data->axi_mon[i]) {
					pr_err("no free ddr id axi mon\n");
					return -EINVAL;
				}
				if(i >= KY_MON_CTRL_NUM + 3) {
					data->axi_mon[i]->id =  \
						((i - (KY_MON_CTRL_NUM + 3)) % 4) + 1;
					data->axi_mon[i]->port_id = ((i - 3) / 4);
				} else {
					data->axi_mon[i]->id = ((i - 1) % 3) + 1;
					data->axi_mon[i]->port_id = ((i - 1) / 3) - 1;
				}
				data->axi_mon[i]->mid = master_id[i - KY_MON_CTRL_NUM];
				if(en == DDR_PORT_DEBUG_LAT)
					data->axi_mon[i]->lat_en = true;
				ret = ddraxi_mon_enable_mon(data->axi_mon[i]);
				if (ret < 0)
					return ret;
			}
		}
		i = KY_ID_MASTER_NUM + KY_MON_CTRL_NUM;
		data->axi_mon[i] = ddraxi_mon_get_mon();
		if (!data->axi_mon[i]) {
			pr_err("no free ddr id axi mon\n");
			return -EINVAL;
		}
		data->axi_mon[i]->id = 0;
		data->axi_mon[i]->port_id = 0;
		data->axi_mon[i]->mid = 0;
		data->axi_mon[i]->master_is_lcd = true;
		if(en == DDR_PORT_DEBUG_LAT)
				data->axi_mon[i]->lat_en = true;
		ret = ddraxi_mon_enable_mon(data->axi_mon[i]);
		if (ret < 0)
			return ret;

		data->ddr_mon_debug = en;
	} else if (!en && data->ddr_mon_debug) {
		data->ddr_mon_debug = en;

		for (i = 0; i < KY_ID_MASTER_NUM	\
				+ KY_MON_CTRL_NUM +1; i++) {
			if(data->axi_mon[i]->mid == 3)
				continue;
			ddraxi_mon_disable_mon(data->axi_mon[i]);
		}
	}

	return 0;
}

/* Get DDR port bandwidth status */
static int ddr_get_port_bw_a0(struct ddr_dfreq_data *data,
	u32 time_diff, struct ky_ddrbw_status *stat)
{
	u64 bw_num, req_num;
	s64 rd_bytes, wr_bytes, rd_reqs, wr_reqs;
	u64 max_rd, max_wr;
	s64 avr_rd, avr_wr;
	struct ddraxi_mon *t_mon;
	u32 bw_fixed, req_fixed;
	int i, burst_len;
	u64 tmp;

	burst_len = get_ddr_burst(data->base);

	ddr_bw_info[ddr_bw_info_index].curr_freq = stat->current_frequency;
	ddr_bw_info[ddr_bw_info_index].total_bw = data->throughput;

	for (i = 0; i < KY_MON_CTRL_NUM + KY_ID_MASTER_NUM + 1; i++) {
		if(data->axi_mon[i]->mid != 3)
			ddraxi_mon_update_mon(data->axi_mon[i]);
	}
	/* bandwidth len is 256 */
	bw_fixed = 16;
	if (unlikely(!bw_fixed))
		bw_fixed = 1;
	/* burst len is 64 */
	req_fixed = burst_len;
	if (unlikely(!req_fixed))
		req_fixed = 1;

	for (i = 0; i < KY_MON_CTRL_NUM + KY_ID_MASTER_NUM; i++) {
		/* calculate port bandwidth by total rd/wr bytes */
		if(data->axi_mon[i]->mid != 3) {
			t_mon = data->axi_mon[i];
			if (t_mon->evt_num[0] >= data->total_rd_bytes[i])
				rd_bytes = t_mon->evt_num[0] - data->total_rd_bytes[i];
			else
				rd_bytes = (u32)(-1) - data->total_rd_bytes[i] + t_mon->evt_num[0];

			if (t_mon->evt_num[1] >= data->total_wr_bytes[i])
				wr_bytes = (t_mon->evt_num[1] - data->total_wr_bytes[i]);
			else
				wr_bytes = (u32)(-1) - data->total_wr_bytes[i] + t_mon->evt_num[1];
			/* calculate port bandwidth by total rd/wr requests */
			if (t_mon->evt_num[2] >= data->rd_reqs[i])
				rd_reqs = (t_mon->evt_num[2] - data->rd_reqs[i]);
			else
				rd_reqs = (u32)(-1) - data->rd_reqs[i] + t_mon->evt_num[2];
			if (t_mon->evt_num[3] >= data->wr_reqs[i])
				wr_reqs = (t_mon->evt_num[3] - data->wr_reqs[i]);
			else
				wr_reqs = (u32)(-1) - data->wr_reqs[i] + t_mon->evt_num[3];

			/* get latency data */
			if (data->ddr_mon_debug == DDR_PORT_DEBUG_LAT) {
				/* prepare latency parameters */
				t_mon = data->axi_mon[i];
				max_rd = t_mon->lat_num[0];
				max_wr = t_mon->lat_num[1];

				tmp = max_rd * 1000;
				do_div(tmp, DDR_FCLK);
				max_rd = tmp;
				tmp = max_wr * 1000;
				do_div(tmp, DDR_FCLK);
				max_wr = tmp;
				/* total ltency*/
				if (t_mon->lat_num[2] >= data->tol_rd_lat[i])
					avr_rd = (t_mon->lat_num[2]	\
						- data->tol_rd_lat[i]);
				else
					avr_rd = (u32)(-1) - data->tol_rd_lat[i]	\
						+ t_mon->lat_num[2];

				if (t_mon->lat_num[3] >= data->tol_wr_lat[i])
					avr_wr = (t_mon->lat_num[3]	\
						- data->tol_wr_lat[i]);
				else
					avr_wr = (u32)(-1) - data->tol_wr_lat[i]	\
						+ t_mon->lat_num[3];

				if (rd_reqs == 0)
					avr_rd = 0;
				else {
					tmp = ((avr_rd << 8) * 1000);
					do_div(tmp, rd_reqs);

					do_div(tmp, DDR_FCLK);
					avr_rd = tmp;
				}

				if (wr_reqs == 0)
					avr_wr = 0;
				else {
					tmp = ((avr_wr << 8) * 1000);
					do_div(tmp, wr_reqs);
					do_div(tmp, DDR_FCLK);
					avr_wr = tmp;
				}
				data->tol_rd_lat[i] = t_mon->lat_num[2];
				data->tol_wr_lat[i] = t_mon->lat_num[3];
				mon_info[i].max_rd = max_rd;
				mon_info[i].max_wr = max_wr;
				mon_info[i].avr_wr = avr_wr;
				mon_info[i].avr_rd = avr_rd;
			}
			bw_num = ((rd_bytes + wr_bytes) >> 10) * bw_fixed;
			if (i == 0) {
				data->p0_r_thpt = max(rd_bytes * bw_fixed, rd_reqs * bw_fixed);
				data->p0_w_thpt = max(wr_bytes * bw_fixed, wr_reqs * bw_fixed);
				data->p0_thpt = data->p0_r_thpt + data->p0_w_thpt;
			}

			data->total_rd_bytes[i] = t_mon->evt_num[0];
			data->total_wr_bytes[i] = t_mon->evt_num[1];
			data->rd_reqs[i] = t_mon->evt_num[2];
			data->wr_reqs[i] = t_mon->evt_num[3];
			mon_info[i].wr_bytes = wr_bytes;
			mon_info[i].rd_bytes = rd_bytes;
			mon_info[i].rd_reqs = rd_reqs;
			mon_info[i].wr_reqs = wr_reqs;
			mon_info[i].byte_thpt = bw_num>>10;
			mon_info[i].req_thpt = req_num>>10;
			bw_num = (rd_bytes >> 10) * bw_fixed;
			mon_info[i].rd_thpt = bw_num>>10;
			bw_num = (wr_bytes >> 10) * bw_fixed;
			mon_info[i].wr_thpt = bw_num>>10;
		}
		else {
			int port_index = data->axi_mon[i]->port_id;
			if(mon_info[port_index].wr_bytes >= (mon_info[i-3].wr_bytes +
						mon_info[i-2].wr_bytes + mon_info[i-1].wr_bytes))
				mon_info[i].wr_bytes = mon_info[port_index].wr_bytes	\
					- mon_info[i-3].wr_bytes - mon_info[i-2].wr_bytes	\
					- mon_info[i-1].wr_bytes;
			else
				 mon_info[i].wr_bytes = 0;
			if(mon_info[port_index].rd_bytes >= (mon_info[i-3].rd_bytes +
						mon_info[i-2].rd_bytes + mon_info[i-1].rd_bytes))
				mon_info[i].rd_bytes = mon_info[port_index].rd_bytes	\
					- mon_info[i-3].rd_bytes - mon_info[i-2].rd_bytes	\
					- mon_info[i-1].rd_bytes;
			else
				mon_info[i].rd_bytes = 0;
			if(mon_info[port_index].wr_reqs >= (mon_info[i-3].wr_reqs +
						mon_info[i-2].wr_reqs + mon_info[i-1].wr_reqs))
				mon_info[i].wr_reqs = mon_info[port_index].wr_reqs	\
					- mon_info[i-3].wr_reqs - mon_info[i-2].wr_reqs	\
					- mon_info[i-1].wr_reqs;
			else
				 mon_info[i].wr_reqs = 0;
			if(mon_info[port_index].rd_reqs >= (mon_info[i-3].rd_reqs +
						mon_info[i-2].rd_reqs + mon_info[i-1].rd_reqs))
				mon_info[i].rd_reqs = mon_info[port_index].rd_reqs	\
					- mon_info[i-3].rd_reqs - mon_info[i-2].rd_reqs	\
					- mon_info[i-1].rd_reqs;
			else
				 mon_info[i].rd_reqs = 0;
			if(mon_info[port_index].byte_thpt >= (mon_info[i-3].byte_thpt +
						mon_info[i-2].byte_thpt + mon_info[i-1].byte_thpt))
				mon_info[i].byte_thpt = mon_info[port_index].byte_thpt	\
					- mon_info[i-3].byte_thpt - mon_info[i-2].byte_thpt	\
					- mon_info[i-1].byte_thpt;
			else
				 mon_info[i].byte_thpt = 0;
			if(mon_info[port_index].req_thpt >= (mon_info[i-3].req_thpt +
						 mon_info[i-2].req_thpt + mon_info[i-1].req_thpt))
				mon_info[i].req_thpt = mon_info[port_index].req_thpt	\
					- mon_info[i-3].req_thpt - mon_info[i-2].req_thpt	\
					- mon_info[i-1].req_thpt;
			else
				mon_info[i].req_thpt = 0;
			bw_num = (mon_info[i].rd_bytes >> 10) * bw_fixed;
			mon_info[i].rd_thpt = bw_num>>10;
			bw_num = (mon_info[i].wr_bytes >> 10) * bw_fixed;
			mon_info[i].wr_thpt = bw_num>>10;
		}
	}

	/* lcd monitor */
	i = KY_MON_CTRL_NUM + KY_ID_MASTER_NUM;
	t_mon = data->axi_mon[i];
	if (t_mon->evt_num[0] >= data->total_rd_bytes[i])
		rd_bytes = t_mon->evt_num[0] - data->total_rd_bytes[i];
	else
		rd_bytes = (u32)(-1) - data->total_rd_bytes[i] + t_mon->evt_num[0];

	if (t_mon->evt_num[1] >= data->total_wr_bytes[i])
		wr_bytes = (t_mon->evt_num[1] - data->total_wr_bytes[i]);
	else
		wr_bytes = (u32)(-1) - data->total_wr_bytes[i] + t_mon->evt_num[1];
	/* calculate port bandwidth by total rd/wr requests */
	if (t_mon->evt_num[2] >= data->rd_reqs[i])
		rd_reqs = (t_mon->evt_num[2] - data->rd_reqs[i]);
	else
		rd_reqs = (u32)(-1) - data->rd_reqs[i] + t_mon->evt_num[2];
	if (t_mon->evt_num[3] >= data->wr_reqs[i])
		wr_reqs = (t_mon->evt_num[3] - data->wr_reqs[i]);
	else
		wr_reqs = (u32)(-1) - data->wr_reqs[i] + t_mon->evt_num[3];

	/* get latency data */
	if (data->ddr_mon_debug == DDR_PORT_DEBUG_LAT) {
		/* prepare latency parameters */
		t_mon = data->axi_mon[i];

		max_rd = t_mon->lat_num[0];
		max_wr = t_mon->lat_num[1];

		tmp = max_rd * 1000;
		do_div(tmp, DDR_FCLK);
		max_rd = tmp;
		tmp = max_wr * 1000;
		do_div(tmp, DDR_FCLK);
		max_wr = tmp;
		/* total ltency*/
		if (t_mon->lat_num[2] >= data->tol_rd_lat[i])
			avr_rd = (t_mon->lat_num[2] - data->tol_rd_lat[i]);
		else
			avr_rd = (u32)(-1) - data->tol_rd_lat[i] + t_mon->lat_num[2];

		if (t_mon->lat_num[3] >= data->tol_wr_lat[i])
			avr_wr = (t_mon->lat_num[3] - data->tol_wr_lat[i]);
		else
			avr_wr = (u32)(-1) - data->tol_wr_lat[i] + t_mon->lat_num[3];
		if (rd_reqs == 0)
			avr_rd = 0;
		else {
			tmp = ((avr_rd << 8) * 1000);
			do_div(tmp, rd_reqs);
			do_div(tmp, DDR_FCLK);
			avr_rd = tmp;
		}
		if (wr_reqs == 0)
			avr_rd = 0;
		else {
			tmp = ((avr_wr << 8) * 1000);
			do_div(tmp, wr_reqs);
			do_div(tmp, DDR_FCLK);
			avr_wr = tmp;
		}
		data->tol_rd_lat[i] = t_mon->lat_num[2];
		data->tol_wr_lat[i] = t_mon->lat_num[3];
		mon_info[i].max_rd = max_rd;
		mon_info[i].max_wr = max_wr;
		mon_info[i].avr_wr = avr_wr;
		mon_info[i].avr_rd = avr_rd;
	}

	bw_num = ((rd_bytes + wr_bytes) >> 10) * bw_fixed;

	req_num = ((rd_reqs + wr_reqs) >> 10) * req_fixed;

	data->total_rd_bytes[i] = t_mon->evt_num[0];
	data->total_wr_bytes[i] = t_mon->evt_num[1];
	data->rd_reqs[i] = t_mon->evt_num[2];
	data->wr_reqs[i] = t_mon->evt_num[3];

	mon_info[i].wr_bytes = wr_bytes;
	mon_info[i].rd_bytes = rd_bytes;
	mon_info[i].rd_reqs = rd_reqs;
	mon_info[i].wr_reqs = wr_reqs;
	mon_info[i].byte_thpt = bw_num>>10;
	mon_info[i].req_thpt = req_num>>10;
	bw_num = (rd_bytes >> 10) * bw_fixed;
	mon_info[i].rd_thpt = bw_num>>10;
	bw_num = (wr_bytes >> 10) * bw_fixed;
	mon_info[i].wr_thpt = bw_num>>10;
	ddr_bw_info_index = (ddr_bw_info_index + 1) % DDR_FC_BW_SIZE;

	return 0;
}

static void ddr_perf_counter_init(struct ddr_perf_data *data)
{
	int i;

	for (i = 0; i < PERF_CNT_NUM; i++) {
		data->cnt[i].id = i;
		data->cnt[i].evt_sel = PERF_INVALID_EVT_SEL;
		data->cnt[i].enable = false;
		data->cnt[i].free = true;
		spin_lock_init(&data->cnt[i].lock);
	}
}

static int ddr_perf_conf(struct ddr_perf_data *data)
{
	/* 0: start counting when enabled; 1: start counting on first data access */
	u32 pc_start_cond = 0, pc_start_index = 0;
	/* 0: continue count when overflow; 1: stop counting on any overflow */
	u32 pc_stop_cond = 1, pc_stop_index = 0x4;
	/* clock should be done in clock tree */
	u32 pc_control = readl(data->base + PERF_CNT_CTRL);

	pc_control &= ~((0x1 << pc_start_index) | (0x1 << pc_stop_index));
	pc_control |= ((pc_start_cond << pc_start_index) | (pc_stop_cond << pc_stop_index));

	writel_relaxed(pc_control, data->base + PERF_CNT_CTRL);

	ddr_perf_counter_init(data);

	return 0;
}

int ddr_perf_init_cnt(struct device *dev, void __iomem *base)
{
	ddr_perf_data = devm_kzalloc(dev, sizeof(struct ddr_perf_data), GFP_KERNEL);
	if (ddr_perf_data == NULL)
		return -ENOMEM;

	ddr_perf_data->base = base;
	ddr_perf_conf(ddr_perf_data);

	return 0;
}

static int ddr_perf_read_cnt(u32 cnt)
{
	int val;

	val = readl(ddr_perf_data->base + perf_cnt_reg(cnt));

	return val;
}

struct ddr_perf_cnt *ddr_perf_cnt_get(void)
{
	int i;

	for (i = 0; i < PERF_CNT_NUM; i++)
		if (ddr_perf_data->cnt[i].free)
			return &ddr_perf_data->cnt[i];
	return NULL;
}

int ddr_perf_cnt_update(struct ddr_perf_cnt *counter)
{
	/* read event number */
	counter->evt_num = ddr_perf_read_cnt(counter->id);

	return 0;
}

static int ddr_perf_cnt_enable(struct ddr_perf_cnt *counter)
{
	union perf_cnt_config cnt_conf;
	u32 conf, cnt = counter->id, reg_offset = PERF_CNT_CONF0;

	if (is_cnt_valid(cnt) < 0 || !ddr_perf_data	\
			|| counter->evt_sel == PERF_INVALID_EVT_SEL) {
		pr_err("wrong performance counter number: 0x%x\n", cnt);
		return -EINVAL;
	}

	spin_lock(&counter->lock);
	counter->enable = true;
	counter->free = false;

	cnt_conf.bit.enable = 1;
	cnt_conf.bit.evt_sel = counter->evt_sel;
	/* clear non-used bit */
	cnt_conf.conf &= 0xff;

	/* performance counter: 4 - 7 */
	if (cnt > 3) {
		cnt = cnt - 4;
		reg_offset = PERF_CNT_CONF1;
	}

	conf = readl(ddr_perf_data->base + reg_offset);
	conf &= ~(0xff << perf_cnt_conf_base(cnt));
	conf |= (cnt_conf.conf << perf_cnt_conf_base(cnt));

	writel(conf, ddr_perf_data->base + reg_offset);
	spin_unlock(&counter->lock);

	return 0;
}

int ddr_perf_cnt_disable(struct ddr_perf_cnt *counter)
{
	u32 conf, cnt = counter->id, reg_offset = PERF_CNT_CONF0;

	if (is_cnt_valid(cnt) < 0 || !ddr_perf_data) {
		pr_err("wrong performance counter number: 0x%x\n", cnt);
		return -EINVAL;
	}

	/* performance counter: 4 - 7 */
	if (cnt > 3) {
		cnt = cnt - 4;
		reg_offset = PERF_CNT_CONF1;
	}

	ddr_perf_cnt_update(counter);

	spin_lock(&counter->lock);
	/* disable performance counter */
	conf = readl(ddr_perf_data->base + reg_offset);
	conf &= ~(PERF_CNT_EN << perf_cnt_conf_base(cnt));
	writel(conf, ddr_perf_data->base + reg_offset);
	counter->free = true;
	counter->enable = false;
	spin_unlock(&counter->lock);

	return 0;
}

static int ddr_get_dev_status(struct device *dev, struct ky_ddrbw_status *stat)
{
	/* add dev freq-change code later */
	struct ky_ddraxi_mon_data *mon_data = dev_get_drvdata(dev);
	struct ddr_dfreq_data *data = mon_data->data;
	int ret, burst_len;
	u64 now;
	u64 busy_cycle, busy_diff, cmd_number, cmd_diff;
	u64 clk_diff, clk_cycle, utility, time_diff;
	u64 last_count = (u32)-1;
	unsigned long flags;
	u64 thpt;
	u64 tmp;

	if (!data)
		return 0;

	burst_len = get_ddr_burst(data->base);

	//stat->current_frequency = ky_get_ddr_freq();
	stat->current_frequency = 2400000000;
	/* read ddr channel 0/1 read/write event number */
	if (!data->cnt_en) {
		/* clock cycles */
		data->perf_ch0 = ddr_perf_cnt_get();
		if (!data->perf_ch0) {
			dev_err(dev, "no free ddr performance counter\n");
			return -EINVAL;
		}
		data->perf_ch0->evt_sel = DDR_CLK_CYCLES;
		ret = ddr_perf_cnt_enable(data->perf_ch0);
		if (ret < 0)
			return ret;

		/* busy cycles */
		data->perf_ch1 = ddr_perf_cnt_get();
		if (!data->perf_ch1) {
			dev_err(dev, "no free ddr performance counter\n");
			return -EINVAL;
		}

		data->perf_ch1->evt_sel = DDR_CH0_BUSY;
		ret = ddr_perf_cnt_enable(data->perf_ch1);
		if (ret < 0)
			return ret;

		/* read/write command number */
		data->perf_ch2 = ddr_perf_cnt_get();
		if (!data->perf_ch2) {
			dev_err(dev, "no free ddr performance counter\n");
			return -EINVAL;
		}

		data->perf_ch2->evt_sel = DDR_RD_WR_CMD;
		ret = ddr_perf_cnt_enable(data->perf_ch2);
		if (ret < 0)
			return ret;

		data->ddr_mon_debug = 0;
		ddr_mon_enable(data, DDR_PORT_DEBUG_LAT);

		data->last_poll = jiffies;
		data->cnt_en = true;

		data->throughput = 0;
		return 0;
	}

	spin_lock_irqsave(&data->lock, flags);
	now = jiffies;

	/* DDR counter may be udpated and enabled in one jiffies */
	if (unlikely(now == data->last_poll)) {
		spin_unlock_irqrestore(&data->lock, flags);
		return 0;
	}

	ddr_perf_cnt_update(data->perf_ch0);
	ddr_perf_cnt_update(data->perf_ch1);
	ddr_perf_cnt_update(data->perf_ch2);

	clk_cycle = data->perf_ch0->evt_num;
	busy_cycle = data->perf_ch1->evt_num;
	cmd_number = data->perf_ch2->evt_num;

	if (clk_cycle < data->clk_cycle)
		clk_diff = last_count - data->clk_cycle + clk_cycle;
	else
		clk_diff = clk_cycle - data->clk_cycle;

	if (busy_cycle < data->busy_cycle)
		busy_diff = last_count - data->busy_cycle + busy_cycle;
	else
		busy_diff = busy_cycle - data->busy_cycle;

	if (cmd_number < data->cmd_number)
		cmd_diff = last_count - data->cmd_number + cmd_number;
	else
		cmd_diff = cmd_number - data->cmd_number;

	if (now < data->last_poll)
		time_diff = last_count - data->last_poll + now;
	else
		time_diff = now - data->last_poll;

	time_diff = jiffies_to_msecs(time_diff);

	thpt = cmd_diff & last_count;
	thpt = thpt * burst_len * 1000;
	do_div(thpt, time_diff);

	tmp = (busy_diff + cmd_diff * 8) * 100;
	do_div(tmp, (clk_diff * 2));
	utility = tmp;
	data->last_poll = now;

	data->clk_cycle = clk_cycle;
	data->busy_cycle = busy_cycle;
	data->cmd_number = cmd_number;
	data->cmd_diff = cmd_diff;
	stat->total_time = time_diff;
	data->throughput = thpt >> 20;

	if (data->ddr_mon_debug) {
		ddr_get_port_bw_a0(data, time_diff, stat);

	}

	spin_unlock_irqrestore(&data->lock, flags);
	return 0;
}

static void ddr_init_mon_data(struct ky_ddraxi_mon_data *mon_data)
{
	struct ddr_dfreq_data *data = mon_data->data;

	data->clk_cycle = 0;
	data->busy_cycle = 0;
	data->cmd_diff = 0;

	memset(data->rd_reqs, 0, sizeof(data->rd_reqs));
	memset(data->wr_reqs, 0, sizeof(data->wr_reqs));
	memset(data->total_rd_bytes, 0, sizeof(data->total_rd_bytes));
	memset(data->total_wr_bytes, 0, sizeof(data->total_wr_bytes));
	memset(data->tol_rd_lat, 0, sizeof(data->tol_rd_lat));
	memset(data->tol_wr_lat, 0, sizeof(data->total_wr_bytes));
	memset(data->overflow_rd_num, 0, sizeof(data->overflow_rd_num));
	memset(data->overflow_wr_num, 0, sizeof(data->overflow_wr_num));
}

static int ky_ddrbw_open(struct inode *inode, struct file *file)
{
	mon_info = (struct aximon_info*)kcalloc((KY_MON_CTRL_NUM	\
		+ KY_ID_MASTER_NUM + 1), 	\
		sizeof(struct aximon_info), GFP_KERNEL);
	ddr_init_mon_data(ddraxi_mon_data);
	return 0;
}

static int ky_ddrbw_release(struct inode *inode, struct file *file)
{
	int i;
	ddr_mon_enable(ddraxi_mon_data->data, 0);
	ddraxi_mon_data->data->cnt_en = 0;
	ddr_perf_cnt_disable(ddraxi_mon_data->data->perf_ch0);
	ddr_perf_cnt_disable(ddraxi_mon_data->data->perf_ch1);
	ddr_perf_cnt_disable(ddraxi_mon_data->data->perf_ch2);
	for(i = 0;i < (KY_MON_CTRL_NUM + KY_ID_MASTER_NUM + 1);i++) {
		memset(&mon_info[i],0,sizeof(struct aximon_info));
		if(ddraxi_mon_data->data->axi_mon[i]->mid == 3)
			kfree(ddraxi_mon_data->data->axi_mon[i]);
	}

	kfree(mon_info);
	return 0;
}

static ssize_t ky_ddrbw_read(struct file *fp,	\
		char __user *buf, size_t size, loff_t *pos)
{
	int rc;
	struct ddrbw_info *bw_info;

	bw_info = kzalloc(sizeof(struct ddrbw_info), GFP_KERNEL);
	rc = ddr_get_dev_status(ddraxi_mon_data->dev, ddraxi_mon_data->stat);
	if(rc < 0) {
		pr_err("get dev status failed!");
		return -EFAULT;
	}

	bw_info->total_time = ddraxi_mon_data->stat->total_time;
	bw_info->clk_cycle = ddraxi_mon_data->data->clk_cycle;
	bw_info->busy_cycle = ddraxi_mon_data->data->busy_cycle;
	bw_info->current_frequency = ddraxi_mon_data->stat->current_frequency;
	bw_info->throughput = ddraxi_mon_data->data->throughput;
	bw_info->p0_r_thpt = ddraxi_mon_data->data->p0_r_thpt;
	bw_info->p0_w_thpt = ddraxi_mon_data->data->p0_w_thpt;
	bw_info->p0_thpt = ddraxi_mon_data->data->p0_thpt;
	bw_info->cmd_number = ddraxi_mon_data->data->cmd_diff;
	bw_info->total_num = KY_MON_CTRL_NUM + KY_ID_MASTER_NUM + 1;
	bw_info->port_num = KY_MON_CTRL_NUM;

	rc = copy_to_user(buf, bw_info, sizeof(struct ddrbw_info));
	if(rc < 0) {
		pr_err("copy_to_user failed!");
		return -EFAULT;
	}
	return size;
}

static long ky_ddrbw_ioctl(struct file *file,	\
		unsigned int cmd, unsigned long arg)
{
	int err, i;
	void __user *argp = (void __user *)arg;
	for(i = 0; i < KY_MON_CTRL_NUM + KY_ID_MASTER_NUM + 1; i++)
	{
		strcpy(mon_info[i].name,master_name[i]);
		mon_info[i].id = ddraxi_mon_data->data->axi_mon[i]->id;
		mon_info[i].port_id = ddraxi_mon_data->data->axi_mon[i]->port_id;
		mon_info[i].mid = ddraxi_mon_data->data->axi_mon[i]->mid;
		if(ddraxi_mon_data->data->axi_mon[i]->master_is_lcd == true)
			mon_info[i].single_master = 1;
		else
			mon_info[i].single_master = 0;
	}
	switch(cmd){
	case 0:
		err = copy_to_user(argp, mon_info, (KY_MON_CTRL_NUM	\
			+ KY_ID_MASTER_NUM + 1)	\
			* sizeof(struct aximon_info)) ? -EFAULT : 0;
		break;
	}
	return err;
}

static const struct file_operations ky_ddrbw_fops = {
	.owner		= THIS_MODULE,
	.unlocked_ioctl	= ky_ddrbw_ioctl,
	.read		= ky_ddrbw_read,
	.open		= ky_ddrbw_open,
	.release	= ky_ddrbw_release,
};

static int ddrbw_detect_init(void)
{
	int rc = 0, ret = 0;
	struct device *devices;

	rc = alloc_chrdev_region(&ddrdev.dev, 0, DDRBWTOOL_COUNT, "ddr_dw");
	if(rc < 0){
		printk("alloc_chrdev_region error\r\n");
		ret =  -EBUSY;
		goto fail;
	}
	printk("[ddrbw] MAJOR is %d\n", MAJOR(ddrdev.dev));
	printk("[ddrbw] MINOR is %d\n", MINOR(ddrdev.dev));

	ddrdev.major = MAJOR(ddrdev.dev);
	cdev_init(&ddrdev.chrdevcdev, &ky_ddrbw_fops);
	rc = cdev_add(&ddrdev.chrdevcdev, ddrdev.dev, DDRBWTOOL_COUNT);
	if (rc < 0) {
		printk("cdev_add error\r\n");
		ret =  -EBUSY;
		goto fail1;
	}

	ddrdev.dev_class = class_create("ddr_bw_class");
	if (IS_ERR(ddrdev.dev_class)) {
		printk("class_create error\r\n");
		ret =  -EBUSY;
		goto fail2;
	}

	devices = device_create(ddrdev.dev_class, NULL, 	\
			MKDEV(ddrdev.major,0), NULL, "ddr_bw");
	if(NULL == devices){
		printk("device_create error\r\n");
		ret =  -EBUSY;
		goto fail3;
	}

	return 0;

fail3:
	class_destroy(ddrdev.dev_class);

fail2:
	cdev_del(&ddrdev.chrdevcdev);
fail1:
	unregister_chrdev_region(ddrdev.dev,DDRBWTOOL_COUNT);
fail:
	return ret;
}

static int ddraxi_mon_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct ky_ddraxi_mon_data *data = NULL;
	struct resource resource;
	int i = 0;
	resource_size_t map_base, map_size;
	int ret = 0;

	data = devm_kzalloc(dev, sizeof(struct ky_ddraxi_mon_data), GFP_KERNEL);
	if (data == NULL)
		return -ENOMEM;

	data->data = devm_kzalloc(dev, sizeof(struct ddr_dfreq_data), GFP_KERNEL);
	if (data->data == NULL)
		return -ENOMEM;

	ret = of_address_to_resource(pdev->dev.of_node, 0, &resource);
	if (ret) {
		dev_err(dev, "invalid address\n");
		return -EINVAL;
	}

	map_base = resource.start;
	map_size = resource_size(&resource);

	data->reg_base = devm_ioremap(dev, map_base, map_size);
	if (!data->reg_base) {
		dev_err(dev, "Failed to get register\n");
		return -EIO;
	}
	data->ciu_base = ioremap(0xd4282c00, 0x1000);
	data->data->base = ioremap(0xc0000000, 0x1800);

	platform_set_drvdata(pdev, data);

	for (i = 0; i < KY_MON_LAT_NUM; i++)
		mon_lat_id[i] = mon_lat_id_a0[i];

	ddraxi_mon_data = data;
	ddraxi_mon_data->dev = dev;
	ddraxi_mon_data->stat = devm_kzalloc(dev,	\
		sizeof(struct ky_ddrbw_status), GFP_KERNEL);
	ddraxi_mon_init(data);

	ddr_init_mon_data(ddraxi_mon_data);
	ddr_perf_init_cnt(dev, data->data->base);
	ret = ddrbw_detect_init();
	if(ret){
		dev_err(dev, "register ddr detect tool failed\n");
		return -EINVAL;
	}
	return 0;
}

static int ddraxi_mon_remove(struct platform_device *pdev)
{
	struct ky_ddraxi_mon_data *mon_data = platform_get_drvdata(pdev);
	mon_data = NULL;

	return 0;
}

static const struct of_device_id ky_ddraxi_dt_match[] = {
	{.compatible = "ky,ddraxi-mon" },
	{},
};
MODULE_DEVICE_TABLE(of, ky_ddraxi_dt_match);


static struct platform_driver ky_ddraxi_mon_driver = {
	.probe = ddraxi_mon_probe,
	.remove = ddraxi_mon_remove,
	.driver = {
		.name = "ky-ddraxi-mon",
		.of_match_table = of_match_ptr(ky_ddraxi_dt_match),
	},
};

static int __init ky_ddraxi_mon_init(void)
{
	return platform_driver_register(&ky_ddraxi_mon_driver);
}
module_init(ky_ddraxi_mon_init);

static void __exit ky_ddraxi_mon_exit(void)
{
	platform_driver_unregister(&ky_ddraxi_mon_driver);
}
module_exit(ky_ddraxi_mon_exit);

MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("KY ddr-axi monitor driver");
