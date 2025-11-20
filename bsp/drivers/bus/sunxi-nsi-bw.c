/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */

#include <linux/io.h>
#include <linux/platform_device.h>
#include <linux/err.h>
#include <linux/spinlock.h>
#include <linux/ioctl.h>
#include <linux/cdev.h>
#include "sunxi-nsi.h"

ssize_t nsi_pmu_bandwidth_rd_show(struct device *dev,
			struct device_attribute *da, char *buf)
{
	unsigned long bread, bandrw[MBUS_PMU_IAG_MAX];
	unsigned long bwtotal = 0;
	unsigned long cpu_bread = 0, cpu_total = 0;
	unsigned int port, len = 0;
	unsigned long flags = 0;
	char bwbuf[16];
	__maybe_unused u32 cpu_unit = sunxi_nsi.cpu_pmu_data_unit;
	u32 ia_unit = sunxi_nsi.ia_pmu_data_unit;
	__maybe_unused u32 ta_unit = sunxi_nsi.ta_pmu_data_unit;

	sunxi_nsi_distribute_mater_get(0xFFFFFFFF);
	spin_lock_irqsave(&hw_nsi_pmu.bwlock, flags);
	if (sunxi_nsi.sub_node_id_mapping) {
		for (port = 0; port < sunxi_nsi.master_cnt; port++) {
			if (sunxi_nsi.master[port].type == NSI_IA_MASTER) {
				u32 ia_index = sunxi_nsi.master[port].ia_master.ia_index;
				bread = readl_relaxed(sunxi_nsi.base + MBUS_PMU_DT_RD(ia_index));
				bandrw[port] = bread;
				len += sprintf(bwbuf, "%lu  ", (bandrw[port] * ia_unit) / 1024);
			} else {
				void *base = sunxi_nsi.master[port].cpu_direct.base;
				bread = readl_relaxed(base + CPU_CHL0_PMU_DATA_R);
				cpu_bread = bread;
				cpu_total += cpu_bread;
				bandrw[port] = bread;
				len += sprintf(bwbuf, "%lu  ", (bandrw[port] * cpu_unit) / 1024);
			}
			strcat(buf, bwbuf);
		}
		cpu_total *= cpu_unit;
		goto show_total;
	}

	/* read the iag pmu bandwidth, which is total master bandwidth */
#if defined(AW_NSI_CPU_CHANNEL) && defined(NSI_HARDCODED_PORT_MAPPING)
	cpu_bread = readl_relaxed(sunxi_nsi.cpu_base + CPU_CHL0_PMU_DATA_R);
	cpu_total = cpu_bread;
	len += sprintf(bwbuf, "%lu  ", (cpu_total * cpu_unit) / 1024);
	strcat(buf, bwbuf);
	cpu_total *= cpu_unit;
#endif
	/* read the iag pmu bandwidth, which is total master bandwidth */
	for (port = 0; port < MBUS_PMU_IAG_MAX; port++) {
		bread = readl_relaxed(sunxi_nsi.base + MBUS_PMU_DT_RD(port));
		bandrw[port] = bread;
		len += sprintf(bwbuf, "%lu  ", (bandrw[port] * ia_unit) / 1024);
		strcat(buf, bwbuf);
	}

show_total:
#if IS_ENABLED(CONFIG_ARCH_SUN55I)
	for (port = 0; port < MBUS_PMU_IAG_MAX; port++)
		bwtotal += bandrw[port];
	bwtotal *= ia_unit;
#else
	/* read the tag pmu bandwidth, which is total ddr bandwidth */
	bwtotal = readl_relaxed(sunxi_nsi.base + MBUS_PMU_DT_RD(MBUS_PMU_TAG));

	if (sunxi_nsi.channel_type == NSI_DUAL_CHANNEL) {
		bwtotal += readl_relaxed(sunxi_nsi.base + MBUS_PMU_DT_RD(MBUS_PMU_TAG + 1));
	}
	bwtotal *= ta_unit;
#endif

	len += sprintf(bwbuf, "%lu\n", (cpu_total + bwtotal) / 1024);
	strcat(buf, bwbuf);

	spin_unlock_irqrestore(&hw_nsi_pmu.bwlock, flags);
	sunxi_nsi_distribute_mater_put(0xFFFFFFFF);
	return len;
}

ssize_t nsi_pmu_bandwidth_wr_show(struct device *dev,
			struct device_attribute *da, char *buf)
{
	unsigned long bwrite, bandrw[MBUS_PMU_IAG_MAX];
	unsigned long bwtotal = 0;
	unsigned long cpu_bwrite = 0, cpu_total = 0;
	unsigned int port, len = 0;
	unsigned long flags = 0;
	char bwbuf[16];
	__maybe_unused u32 cpu_unit = sunxi_nsi.cpu_pmu_data_unit;
	u32 ia_unit = sunxi_nsi.ia_pmu_data_unit;
	__maybe_unused u32 ta_unit = sunxi_nsi.ta_pmu_data_unit;

	sunxi_nsi_distribute_mater_get(0xFFFFFFFF);
	spin_lock_irqsave(&hw_nsi_pmu.bwlock, flags);
	if (sunxi_nsi.sub_node_id_mapping) {
		for (port = 0; port < sunxi_nsi.master_cnt; port++) {
			if (sunxi_nsi.master[port].type == NSI_IA_MASTER) {
				u32 ia_index = sunxi_nsi.master[port].ia_master.ia_index;
				bwrite = readl_relaxed(sunxi_nsi.base + MBUS_PMU_DT_WR(ia_index));
				bandrw[port] = bwrite;
				len += sprintf(bwbuf, "%lu  ", (bandrw[port] * ia_unit) / 1024);
			} else {
				void *base = sunxi_nsi.master[port].cpu_direct.base;
				bwrite = readl_relaxed(base + CPU_CHL0_PMU_DATA_W);
				cpu_bwrite = bwrite;
				cpu_total += cpu_bwrite;
				bandrw[port] = bwrite;
				len += sprintf(bwbuf, "%lu  ", (bandrw[port] * cpu_unit) / 1024);
			}
			strcat(buf, bwbuf);
		}
		cpu_total *= cpu_unit;
		goto show_total;
	}

	/* read the iag pmu bandwidth, which is total master bandwidth */
#if defined(AW_NSI_CPU_CHANNEL) && defined(NSI_HARDCODED_PORT_MAPPING)
	cpu_bwrite = readl_relaxed(sunxi_nsi.cpu_base + CPU_CHL0_PMU_DATA_W);

	cpu_total = cpu_bwrite;
	len += sprintf(bwbuf, "%lu  ", (cpu_total* cpu_unit) / 1024);
	strcat(buf, bwbuf);
	cpu_total *= cpu_unit;
#endif
	/* read the iag pmu bandwidth, which is total master bandwidth */
	for (port = 0; port < MBUS_PMU_IAG_MAX; port++) {
		bwrite = readl_relaxed(sunxi_nsi.base + MBUS_PMU_DT_WR(port));
		bandrw[port] = bwrite;
		len += sprintf(bwbuf, "%lu  ", (bandrw[port] * ia_unit) / 1024);
		strcat(buf, bwbuf);
	}

show_total:
#if IS_ENABLED(CONFIG_ARCH_SUN55I)
	for (port = 0; port < MBUS_PMU_IAG_MAX; port++)
		bwtotal += bandrw[port];
	bwtotal *= ia_unit;
#else
	/* read the tag pmu bandwidth, which is total ddr bandwidth */
	bwtotal = readl_relaxed(sunxi_nsi.base + MBUS_PMU_DT_WR(MBUS_PMU_TAG));

	if (sunxi_nsi.channel_type == NSI_DUAL_CHANNEL) {
		bwtotal += readl_relaxed(sunxi_nsi.base + MBUS_PMU_DT_WR(MBUS_PMU_TAG + 1));
	}
	bwtotal *= ta_unit;
#endif

	len += sprintf(bwbuf, "%lu\n", (cpu_total + bwtotal) / 1024);
	strcat(buf, bwbuf);

	spin_unlock_irqrestore(&hw_nsi_pmu.bwlock, flags);
	sunxi_nsi_distribute_mater_put(0xFFFFFFFF);
	return len;
}

ssize_t nsi_pmu_latency_rd_show(struct device *dev,
			struct device_attribute *da, char *buf)
{
	unsigned long laread, larw[MBUS_PMU_IAG_MAX];
	unsigned long request;
	unsigned long cpu_request = 1, cpu_laread = 0;
	unsigned int port, len = 0;
	unsigned long flags = 0;
	char labuf[16];

	sunxi_nsi_distribute_mater_get(0xFFFFFFFF);
	spin_lock_irqsave(&hw_nsi_pmu.bwlock, flags);

	if (sunxi_nsi.sub_node_id_mapping) {
		for (port = 0; port < sunxi_nsi.master_cnt; port++) {
			if (sunxi_nsi.master[port].type == NSI_IA_MASTER) {
				u32 ia_index = sunxi_nsi.master[port].ia_master.ia_index;
				laread = readl_relaxed(sunxi_nsi.base + MBUS_PMU_LA_RD(ia_index));
				request = readl_relaxed(sunxi_nsi.base + MBUS_PMU_RQ_RD(ia_index));
			} else {
				void *base = sunxi_nsi.master[port].cpu_direct.base;
				laread = readl_relaxed(base + CPU_CHL0_PMU_LAT_R);
				request = readl_relaxed(base + CPU_CHL0_PMU_REQ_R);
				cpu_request = request;
				cpu_laread = laread;
			}
			if (request == 0) {
				larw[port] = 0;
			} else {
				larw[port] = laread / request;
			}
			len += sprintf(labuf, "%lu  ", larw[port]);
			strcat(buf, labuf);
		}
		goto show_total;
	}
	/* read the iag pmu latency and request */
	for (port = 0; port < MBUS_PMU_IAG_MAX; port++) {
		laread = readl_relaxed(sunxi_nsi.base + MBUS_PMU_LA_RD(port));
		request = readl_relaxed(sunxi_nsi.base + MBUS_PMU_RQ_RD(port));

		if (request == 0) {
			larw[port] = 0;
		} else {
			larw[port] = laread / request;
		}
	}
	for (port = 0; port < MBUS_PMU_IAG_MAX; port++) {
		len += sprintf(labuf, "%lu  ", larw[port]);
		strcat(buf, labuf);
	}

show_total:
	/* read the tag pmu latency and request, which is total ddr latency and request */
	laread = readl_relaxed(sunxi_nsi.base + MBUS_PMU_LA_RD(MBUS_PMU_TAG));
	request = readl_relaxed(sunxi_nsi.base + MBUS_PMU_RQ_RD(MBUS_PMU_TAG));

	if (sunxi_nsi.channel_type == NSI_DUAL_CHANNEL) {
		laread += readl_relaxed(sunxi_nsi.base + MBUS_PMU_LA_RD(MBUS_PMU_TAG + 1));
		request += readl_relaxed(sunxi_nsi.base + MBUS_PMU_RQ_RD(MBUS_PMU_TAG + 1));
	}

	len += sprintf(labuf, "%lu\n", (laread / request) + (cpu_laread / cpu_request));
	strcat(buf, labuf);

	spin_unlock_irqrestore(&hw_nsi_pmu.bwlock, flags);
	sunxi_nsi_distribute_mater_put(0xFFFFFFFF);
	return len;
}

ssize_t nsi_pmu_latency_wr_show(struct device *dev,
			struct device_attribute *da, char *buf)
{
	unsigned long lawrite, larw[MBUS_PMU_IAG_MAX];
	unsigned long request;
	unsigned long cpu_request = 1, cpu_lawrite = 0;
	unsigned int port, len = 0;
	unsigned long flags = 0;
	char labuf[16];

	sunxi_nsi_distribute_mater_get(0xFFFFFFFF);
	spin_lock_irqsave(&hw_nsi_pmu.bwlock, flags);

	if (sunxi_nsi.sub_node_id_mapping) {
		for (port = 0; port < sunxi_nsi.master_cnt; port++) {
			if (sunxi_nsi.master[port].type == NSI_IA_MASTER) {
				u32 ia_index = sunxi_nsi.master[port].ia_master.ia_index;
				lawrite = readl_relaxed(sunxi_nsi.base + MBUS_PMU_LA_WR(ia_index));
				request = readl_relaxed(sunxi_nsi.base + MBUS_PMU_RQ_WR(ia_index));
			} else {
				void *base = sunxi_nsi.master[port].cpu_direct.base;
				lawrite = readl_relaxed(base + CPU_CHL0_PMU_LAT_W);
				request = readl_relaxed(base + CPU_CHL0_PMU_REQ_W);
				cpu_request = request;
				cpu_lawrite = lawrite;
			}
			if (request == 0) {
				larw[port] = 0;
			} else {
				larw[port] = lawrite / request;
			}
			len += sprintf(labuf, "%lu  ", larw[port]);
			strcat(buf, labuf);
		}
		goto show_total;
	}
	/* read the iag pmu latency and request */
	for (port = 0; port < MBUS_PMU_IAG_MAX; port++) {
		lawrite = readl_relaxed(sunxi_nsi.base + MBUS_PMU_LA_WR(port));
		request = readl_relaxed(sunxi_nsi.base + MBUS_PMU_RQ_WR(port));

		if (request == 0) {
			larw[port] = 0;
		} else {
			larw[port] = lawrite / request;
		}
	}
	for (port = 0; port < MBUS_PMU_IAG_MAX; port++) {
		len += sprintf(labuf, "%lu  ", larw[port]);
		strcat(buf, labuf);
	}

show_total:
	/* read the tag pmu latency and request, which is total ddr latency and request */
	lawrite = readl_relaxed(sunxi_nsi.base + MBUS_PMU_LA_WR(MBUS_PMU_TAG));
	request = readl_relaxed(sunxi_nsi.base + MBUS_PMU_RQ_WR(MBUS_PMU_TAG));

	if (sunxi_nsi.channel_type == NSI_DUAL_CHANNEL) {
		lawrite += readl_relaxed(sunxi_nsi.base + MBUS_PMU_LA_WR(MBUS_PMU_TAG + 1));
		request += readl_relaxed(sunxi_nsi.base + MBUS_PMU_RQ_WR(MBUS_PMU_TAG + 1));
	}

	len += sprintf(labuf, "%lu\n", (lawrite / request) + (cpu_lawrite / cpu_request));
	strcat(buf, labuf);

	spin_unlock_irqrestore(&hw_nsi_pmu.bwlock, flags);
	sunxi_nsi_distribute_mater_put(0xFFFFFFFF);
	return len;
}


ssize_t nsi_pmu_cmd_rd_show(struct device *dev,
			struct device_attribute *da, char *buf)
{
	unsigned long larw[MBUS_PMU_IAG_MAX];
	unsigned long request, cpu_request = 0;
	unsigned int port, len = 0;
	unsigned long flags = 0;
	char labuf[16];

	sunxi_nsi_distribute_mater_get(0xFFFFFFFF);
	spin_lock_irqsave(&hw_nsi_pmu.bwlock, flags);

	if (sunxi_nsi.sub_node_id_mapping) {
		for (port = 0; port < sunxi_nsi.master_cnt; port++) {
			if (sunxi_nsi.master[port].type == NSI_IA_MASTER) {
				u32 ia_index = sunxi_nsi.master[port].ia_master.ia_index;
				larw[port] = readl_relaxed(sunxi_nsi.base + MBUS_PMU_RQ_RD(ia_index));
			} else {
				void *base = sunxi_nsi.master[port].cpu_direct.base;
				larw[port] = readl_relaxed(base + CPU_CHL0_PMU_REQ_R);
				cpu_request = larw[port];
			}
			len += sprintf(labuf, "%lu  ", larw[port]);
			strcat(buf, labuf);
		}
		goto show_total;
	}
	/* read the iag pmu latency and request */
	for (port = 0; port < MBUS_PMU_IAG_MAX; port++) {
		larw[port] = readl_relaxed(sunxi_nsi.base + MBUS_PMU_RQ_RD(port));
	}
	for (port = 0; port < MBUS_PMU_IAG_MAX; port++) {
		len += sprintf(labuf, "%lu  ", larw[port]);
		strcat(buf, labuf);
	}

show_total:
	/* read the tag pmu latency and request, which is total ddr latency and request */
	request = readl_relaxed(sunxi_nsi.base + MBUS_PMU_RQ_RD(MBUS_PMU_TAG));
	if (sunxi_nsi.channel_type == NSI_DUAL_CHANNEL) {
		request += readl_relaxed(sunxi_nsi.base + MBUS_PMU_RQ_RD(MBUS_PMU_TAG + 1));
	}

	len += sprintf(labuf, "%lu\n", request + cpu_request);
	strcat(buf, labuf);

	spin_unlock_irqrestore(&hw_nsi_pmu.bwlock, flags);
	sunxi_nsi_distribute_mater_put(0xFFFFFFFF);
	return len;
}

ssize_t nsi_pmu_cmd_wr_show(struct device *dev,
			struct device_attribute *da, char *buf)
{
	unsigned long larw[MBUS_PMU_IAG_MAX];
	unsigned long request, cpu_request = 0;
	unsigned int port, len = 0;
	unsigned long flags = 0;
	char labuf[16];

	sunxi_nsi_distribute_mater_get(0xFFFFFFFF);
	spin_lock_irqsave(&hw_nsi_pmu.bwlock, flags);

	if (sunxi_nsi.sub_node_id_mapping) {
		for (port = 0; port < sunxi_nsi.master_cnt; port++) {
			if (sunxi_nsi.master[port].type == NSI_IA_MASTER) {
				u32 ia_index = sunxi_nsi.master[port].ia_master.ia_index;
				larw[port] = readl_relaxed(sunxi_nsi.base + MBUS_PMU_RQ_WR(ia_index));
			} else {
				void *base = sunxi_nsi.master[port].cpu_direct.base;
				larw[port] = readl_relaxed(base + CPU_CHL0_PMU_REQ_W);
				cpu_request = larw[port];
			}
			len += sprintf(labuf, "%lu  ", larw[port]);
			strcat(buf, labuf);
		}
		goto show_total;
	}
	/* read the iag pmu latency and request */
	for (port = 0; port < MBUS_PMU_IAG_MAX; port++) {
		larw[port] = readl_relaxed(sunxi_nsi.base + MBUS_PMU_RQ_WR(port));
	}
	for (port = 0; port < MBUS_PMU_IAG_MAX; port++) {
		len += sprintf(labuf, "%lu  ", larw[port]);
		strcat(buf, labuf);
	}

show_total:
	/* read the tag pmu latency and request, which is total ddr latency and request */
	request = readl_relaxed(sunxi_nsi.base + MBUS_PMU_RQ_WR(MBUS_PMU_TAG));
	if (sunxi_nsi.channel_type == NSI_DUAL_CHANNEL) {
		request += readl_relaxed(sunxi_nsi.base + MBUS_PMU_RQ_WR(MBUS_PMU_TAG + 1));
	}

	len += sprintf(labuf, "%lu\n", request + cpu_request);
	strcat(buf, labuf);

	spin_unlock_irqrestore(&hw_nsi_pmu.bwlock, flags);
	sunxi_nsi_distribute_mater_put(0xFFFFFFFF);
	return len;
}

ssize_t nsi_pmu_bandwidth_show(struct device *dev,
			struct device_attribute *da, char *buf)
{
	unsigned long bandrw[MBUS_PMU_IAG_MAX];
	unsigned long bwtotal = 0;
	unsigned long cpu_total = 0;
	unsigned int port, len = 0;
	unsigned long flags = 0;
	char bwbuf[16];
	__maybe_unused u32 cpu_unit = sunxi_nsi.cpu_pmu_data_unit;
	u32 ia_unit = sunxi_nsi.ia_pmu_data_unit;
	__maybe_unused u32 ta_unit = sunxi_nsi.ta_pmu_data_unit;

	sunxi_nsi_distribute_mater_get(0xFFFFFFFF);
	spin_lock_irqsave(&hw_nsi_pmu.bwlock, flags);
	if (sunxi_nsi.sub_node_id_mapping) {
		for (port = 0; port < sunxi_nsi.master_cnt; port++) {
			if (sunxi_nsi.master[port].type == NSI_IA_MASTER) {
				u32 ia_index = sunxi_nsi.master[port].ia_master.ia_index;
				bandrw[port] = readl_relaxed(sunxi_nsi.base + MBUS_PMU_DT_RD(ia_index)) +
								readl_relaxed(sunxi_nsi.base + MBUS_PMU_DT_WR(ia_index));
				len += sprintf(bwbuf, "%lu  ", (bandrw[port] * ia_unit) / 1024);
			} else {
				void *base = sunxi_nsi.master[port].cpu_direct.base;
				bandrw[port] = readl_relaxed(base + CPU_CHL0_PMU_DATA_R) +
								readl_relaxed(base + CPU_CHL0_PMU_DATA_W);
				cpu_total += bandrw[port];
				len += sprintf(bwbuf, "%lu  ", (bandrw[port] * cpu_unit) / 1024);
			}
			strcat(buf, bwbuf);
		}
		cpu_total *= cpu_unit;
		goto show_total;
	}

	/* read the iag pmu bandwidth, which is total master bandwidth */
#if defined(AW_NSI_CPU_CHANNEL) && defined(NSI_HARDCODED_PORT_MAPPING)
	cpu_total = readl_relaxed(sunxi_nsi.cpu_base + CPU_CHL0_PMU_DATA_R) +
				readl_relaxed(sunxi_nsi.cpu_base + CPU_CHL0_PMU_DATA_W);
	len += sprintf(bwbuf, "%lu  ", (cpu_total* cpu_unit)/1024);
	strcat(buf, bwbuf);
	cpu_total *= cpu_unit;
#endif
	/* read the iag pmu bandwidth, which is total master bandwidth */
	for (port = 0; port < MBUS_PMU_IAG_MAX; port++) {
		bandrw[port] = readl_relaxed(sunxi_nsi.base + MBUS_PMU_DT_RD(port)) +
						readl_relaxed(sunxi_nsi.base + MBUS_PMU_DT_WR(port));
		len += sprintf(bwbuf, "%lu  ", (bandrw[port] * ia_unit)/1024);
		strcat(buf, bwbuf);
	}

show_total:
#if IS_ENABLED(CONFIG_ARCH_SUN55I)
	for (port = 0; port < MBUS_PMU_IAG_MAX; port++)
		bwtotal += bandrw[port];
	bwtotal *= ia_unit;
#else
	/* read the tag pmu bandwidth, which is total ddr bandwidth */
	bwtotal = readl_relaxed(sunxi_nsi.base + MBUS_PMU_DT_RD(MBUS_PMU_TAG)) +
				readl_relaxed(sunxi_nsi.base + MBUS_PMU_DT_WR(MBUS_PMU_TAG));

	if (sunxi_nsi.channel_type == NSI_DUAL_CHANNEL) {
		bwtotal += readl_relaxed(sunxi_nsi.base + MBUS_PMU_DT_RD(MBUS_PMU_TAG + 1)) +
				readl_relaxed(sunxi_nsi.base + MBUS_PMU_DT_WR(MBUS_PMU_TAG + 1));
	}
	bwtotal *= ta_unit;
#endif

	len += sprintf(bwbuf, "%lu\n", (cpu_total + bwtotal) / 1024);
	strcat(buf, bwbuf);

	spin_unlock_irqrestore(&hw_nsi_pmu.bwlock, flags);
	sunxi_nsi_distribute_mater_put(0xFFFFFFFF);
	return len;
}

ssize_t nsi_available_pmu_show(struct device *dev,
			struct device_attribute *da, char *buf)
{
	ssize_t i, len = 0;
	if (sunxi_nsi.sub_node_id_mapping) {
		for (i = 0; i < sunxi_nsi.master_cnt; i++) {
			len += sprintf(buf + len, "%s  ", sunxi_nsi.master[i].name);
		}
		len += sprintf(buf + len, "%s  ", "total");
		return len;
	}

	for (i = 0; i < ARRAY_SIZE(pmu_name); i++)
		len += sprintf(buf + len, "%s  ", get_name(i));

	len += sprintf(buf + len, "\n");

	return len;
}
