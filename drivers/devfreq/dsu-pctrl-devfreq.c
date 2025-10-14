/*
 * A devfreq driver for ARM DynamIQ Shared Unit (DSU) CPU memory power
 * managemnt.
 *
 * Copyright (c) 2015-2017 ARM Ltd.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 *
 * Authors:
 *		Sean McGoogan <Sean.McGoogan@arm.com>
 *		Lukasz Luba <lukasz.luba@arm.com>
 *		Ionela Voinescu <Ionela.Voinescu@arm.com>
 *		David Guillen Fandos, ARM Ltd.
 *
 * The cache in DSU supports partial power down by splitting the cache into an
 * implementation-specific number of portions. This driver provides an
 * energy-cost justified demand-driven policy for controlling the number of
 * portions enabled.
 *
 * The policy maps the number of portions enabled to frequency (1 portion ==
 * 1Hz) and provides total/busy time statistics such that
 * busy_time = (total_time * hit_count) / access_count.
 *
 * This also provides a specific DevFreq governor that will implement the
 * desired energy-cost-justification policy as this version is not suitable
 * for use with the simple_ondemand governor.
 * DevFreq min/max/governor controls work as usual.
 *
 *
 * There is no relation to actual frequency control of the cache device.
 */

#include <linux/devfreq.h>
#include <linux/hrtimer.h>
#include <linux/ktime.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/pm_opp.h>
#include <linux/slab.h>
#include <linux/workqueue.h>

#include "governor.h"

#define CREATE_TRACE_POINTS
#include <trace/events/dsu_pctrl.h>

#include <linux/arm-smccc.h>

#define DSU_PCTRL_PLATFORM_DEVICE_NAME			"dsu_pctrl"
#define DSU_PCTRL_GOVERNOR_NAME				"dsu_pc_governor"

#define DSU_PCTRL_DEFAULT_POLLING_MS			10
#define POLLING_DOWN_INTERVAL				10
/* Required polling time in miliseconds */
#define DSU_PCTRL_DEFAULT_REQUIRED_GOV_POLLING		10000

/* Default cache line size */
#define DSU_PCTRL_DEFAULT_LINE_SIZE			64

/* Default size of the cache: 1MB */
#define DSU_PCTRL_DEFAULT_SIZE_KB			(8 * 1024)

/* Default static energy leaked by the cache per MB */
#define DSU_PCTRL_DEFAULT_CACHE_LEAKAGE			10000

#define CIX_SIP_DSU_HW_CTL				0xc2000003
#define CIX_SIP_DSU_SET_PD				0xc2000004
#define CIX_SIP_DSU_GET_PD				0xc2000005
#define CIX_SIP_DSU_SET_TH				0xc2000006
#define CIX_SIP_DSU_GET_HITCNT				0xc2000007
#define CIX_SIP_DSU_GET_MISSCNT				0xc2000008

/* Default size of the cache: 1MB */
#define CIX_L3_SIZE					8
#define CIX_DSU_T					8
#define CIX_DSU_L					(CIX_L3_SIZE * dsu->static_leakage_per_mb)
#define CIX_DSU_D					(dsu->dram_energy_per_mb)

#define CLUSTER_L3_DN_TH0_DEFAULT			(12288 * CIX_DSU_T * CIX_DSU_L / CIX_DSU_D / 1000)
#define CLUSTER_L3_DN_TH1_DEFAULT			(4096 * CIX_DSU_T * CIX_DSU_L / CIX_DSU_D / 1000)
#define CLUSTER_L3_UP_TH0_DEFAULT			(4096 * CIX_DSU_T * CIX_DSU_L / CIX_DSU_D / 1000)
#define CLUSTER_L3_UP_TH1_DEFAULT			(4096 * CIX_DSU_T * CIX_DSU_L / CIX_DSU_D / 1000)
#define CLUSTER_L3_UP_TH2_DEFAULT			(24576 * CIX_DSU_T * CIX_DSU_L / CIX_DSU_D / 1000)

/*
 * Amount of energy used by the DRAM system per MB
 * of transferred data (on average) expressed in uJ/MB
 */
#define DSU_PCTRL_DEFAULT_DRAM_ENERGY_PER_MB		130

#define SZ_1KB						(1ULL << 10)
#define SZ_1MB						(1ULL << 20)
#define ROUNDING_INTEGER_MUL				SZ_1MB
#define PERCENT_MAX					100

/*
 * Portion threshold to use for down-sizing (Td)
 * between 0 and 100
 * 90 for 90%
 */
#define DOWNSIZE_PORTION_THRESHOLD			90

/*
 * Portion threshold to use for up-sizing (Tu)
 * between 0 and 100
 * 90 for 90%
 */
#define UPSIZE_PORTION_THRESHOLD			90

/* Bit-field positions for the CLUSTERPWRCTLR_EL1 register */
#define PORTION_1					4
#define PORTION_2					5
#define PORTION_3					6
#define PORTION_4					7
/* Bit-masks for the CLUSTERPWRCTLR_EL1 register */
#define PORTION_BITS		((1UL << PORTION_1) | \
				 (1UL << PORTION_2) | \
				 (1UL << PORTION_3) | \
				 (1UL << PORTION_4))
#define PORTION_MASK		(~(PORTION_BITS))

#if defined(CONFIG_ARM64)
/*
 * Read the system register 'sysreg' (using MRS), and then
 * explicitly clear it, by writing zero to it (using MSR).
 * Put the value read into 'result'.
 */
#define SYS_REG_READ_THEN_CLEAR(sysreg, result)		\
do {							\
	__asm__ __volatile__(				\
		"mrs	%0, " #sysreg		"\n\t"	\
		"msr	" #sysreg ", XZR"		\
		: "=r" (result) /* only one output */	\
		/* no inputs */				\
		/* no clobbers */			\
	);						\
} while (0)

/*
 * Read the system register 'sysreg' (using MRS).
 * Put the value read into 'result'.
 */
#define SYS_REG_READ(sysreg, result)			\
do {							\
	__asm__ __volatile__(				\
		"mrs	%0, " #sysreg			\
		: "=r" (result) /* only one output */	\
		/* no inputs */				\
		/* no clobbers */			\
	);						\
} while (0)

/*
 * Write 'value' to the system register 'sysreg' (using MSR).
 */
#define SYS_REG_WRITE(sysreg, value)			\
do {							\
	__asm__ __volatile__(				\
		"msr	" #sysreg ", %0"		\
		: /* no outputs */			\
		: "r" (value) /* only one input */	\
		/* no clobbers */			\
	);						\
} while (0)

#else

#define SYS_REG_READ_THEN_CLEAR(sysreg, result)	do {} while (0)
#define SYS_REG_READ(sysreg, result)	do {} while (0)
#define SYS_REG_READ_THEN_CLEAR(sysreg, result)	do {} while (0)
#define SYS_REG_WRITE(sysreg, result)	do {} while (0)

#endif

enum arm_dsu_version {
	ARM_DSU_R0 = 0,
};

struct dsu_pctrl_data {
	u32 portion_min;
	u32 portion_max;
};

struct dsu_pctrl {
	int id;
	struct dsu_pctrl_data *dsu_data;
	struct devfreq *devfreq;
	struct platform_device *pdev;
	struct devfreq_dev_profile *devfreq_profile;
	u32 portions;
	u32 size;
	u32 line_size;
	u32 up_polling_ms;
	u32 down_polling_ms;
	u32 initial_freq;
	u32 static_leakage_per_mb;
	u32 dram_energy_per_mb;

	unsigned long *freq_table;
	int freq_table_len;

	struct workqueue_struct *update_wq;
	struct work_struct update_handle;
	struct mutex lock;
	struct hrtimer poll_timer;

	/* Leakage (static power) for a single portion (in uW) */
	unsigned long static_leakage;
	unsigned long downsize_threshold;
	unsigned long upsize_threshold;
	unsigned long max_threshold;
	unsigned long cur_num_portions;

	bool hw_control;
	unsigned int cluster_l3_dn_th0;
	unsigned int cluster_l3_dn_th1;
	unsigned int cluster_l3_up_th0;
	unsigned int cluster_l3_up_th1;
	unsigned int cluster_l3_up_th2;
	unsigned int cluster_l3_auto_interval;

	/* Contains state for the algorithm. It is clean during resume */
	struct {
		unsigned long accesses_up;
		unsigned long misses_up;
		unsigned long accesses_down;
		unsigned long misses_down;
		unsigned long usec_up;
		unsigned long usec_down;
		unsigned int last_update;
	} alg;
};

static atomic_t dsu_pctrl_device_id = ATOMIC_INIT(0);

static const struct dsu_pctrl_data device_data[] = {
	{.portion_min = 1, .portion_max = 3},
};

static const struct of_device_id dsu_pctrl_devfreq_id[] = {
	{.compatible = "arm,dsu_pctrl_r0", .data =
		(void *)&device_data[ARM_DSU_R0]},
	{}
};

static int dsu_pctrl_set_active_portions(struct device *dev,
					 unsigned long portions)
{
	struct dsu_pctrl *dsu = dev_get_drvdata(dev);
	struct dsu_pctrl_data *data = dsu->dsu_data;
	unsigned long portion_active;
	unsigned long portion_control;
	struct arm_smccc_res res;

	if (portions < data->portion_min || portions > data->portion_max) {
		dev_warn(dev, "%s: Target of %lu-portions is "
			 "outside range of %u..%u\n", __func__, portions,
			 data->portion_min, data->portion_max);

		return -EINVAL;
	}

	/*
	 * Set the number of portions in the DSU to portions
	 *
	 * portions	Set of bit-fields to Enable
	 * ---------	---------------------------
	 *	4	PORTION_1|PORTION_2|PORTION_3|PORTION_4
	 *	3	PORTION_1|PORTION_2|PORTION_3
	 *	2	PORTION_1|PORTION_2
	 *	1	PORTION_1
	 *	0	<none>
	 */
	portion_active = ((1UL << portions) - 1) << PORTION_1;

	arm_smccc_smc(CIX_SIP_DSU_GET_PD, 0, 0, 0, 0, 0, 0, 0, &res);
	if (res.a0)
		portion_control = res.a0;

	portion_control &= PORTION_MASK;
	portion_control |= portion_active;

	arm_smccc_smc(CIX_SIP_DSU_SET_PD, portion_control, 0, 0, 0, 0, 0, 0, &res);

	/* Update current number of portions */
	dsu->cur_num_portions = portions;

	return 0;
}

static int dsu_pctrl_devfreq_target(struct device *dev,
				    unsigned long *portions, u32 flags)
{
	/* Set requested portions */
	return dsu_pctrl_set_active_portions(dev, *portions);
}

static int dsu_pctrl_devfreq_get_dev_status(struct device *dev,
					    struct devfreq_dev_status *stat)
{
	struct dsu_pctrl *dsu = dev_get_drvdata(dev);
	unsigned int const usec = ktime_to_us(ktime_get());
	unsigned int delta;
	unsigned long hits = 0;
	unsigned long misses = 0;
	unsigned long accesses;
	struct arm_smccc_res res;

	delta = usec - dsu->alg.last_update;
	dsu->alg.last_update = usec;

	stat->current_frequency = dsu->cur_num_portions;

	arm_smccc_smc(CIX_SIP_DSU_GET_HITCNT, 0, 0, 0, 0, 0, 0, 0, &res);
	if (res.a0)
		hits = res.a0;

	arm_smccc_smc(CIX_SIP_DSU_GET_MISSCNT, 0, 0, 0, 0, 0, 0, 0, &res);
	if (res.a0)
		misses = res.a0;

	accesses = hits + misses;

	dsu->alg.accesses_up   += accesses;
	dsu->alg.accesses_down += accesses;
	dsu->alg.misses_up     += misses;
	dsu->alg.misses_down   += misses;
	dsu->alg.usec_up       += delta;
	dsu->alg.usec_down     += delta;

	if (!accesses)
		accesses = 1;

	/*
	 * Although this is not yet suitable for use with the simple ondemand
	 * governor we'll fill these usage statistics.
	 */
	stat->total_time = delta;
	stat->busy_time  = stat->total_time * hits / accesses;

	trace_dsu_pctrl_dev_status(dsu->id, hits, misses,
				   dsu->cur_num_portions, stat->busy_time,
				   stat->total_time);

	return 0;
}

static int dsu_pctrl_up_size_check(struct dsu_pctrl *dsu)
{
	struct dsu_pctrl_data *data = dsu->dsu_data;
	unsigned long cache_miss_bw;
	int ret = 0;

	/*
	 * If we are at the maximum number of active portions and we know
	 * there won't be a downsize attempt due to polling time constraints,
	 * we can skip calculations that won't lead to any action.
	 */
	if ((dsu->cur_num_portions >= data->portion_max) &&
	    (dsu->alg.usec_down < dsu->down_polling_ms))
		goto cleanup;

	cache_miss_bw = dsu->line_size * dsu->alg.misses_up;
	cache_miss_bw *= ROUNDING_INTEGER_MUL;
	cache_miss_bw /= dsu->alg.usec_up;

	if (cache_miss_bw > dsu->upsize_threshold)
		ret = 1;

cleanup:
	dsu->alg.usec_up = 0;
	dsu->alg.misses_up = 0;
	dsu->alg.accesses_up = 0;

	return ret;
}

static int dsu_pctrl_down_size_check(struct dsu_pctrl *dsu)
{
	struct dsu_pctrl_data *data = dsu->dsu_data;
	unsigned long cache_hit_bw;
	int ret = 0;

	if (dsu->cur_num_portions <= data->portion_min)
		goto cleanup;

	cache_hit_bw = dsu->alg.accesses_down - dsu->alg.misses_down;
	cache_hit_bw *= dsu->line_size;
	cache_hit_bw *= ROUNDING_INTEGER_MUL;
	cache_hit_bw /= dsu->alg.usec_down;

	if (cache_hit_bw < (dsu->max_threshold * dsu->cur_num_portions -
			    dsu->downsize_threshold))
		ret = 1;

cleanup:
	dsu->alg.usec_down = 0;
	dsu->alg.misses_down = 0;
	dsu->alg.accesses_down = 0;

	return ret;
}

static unsigned long dsu_pctrl_calc_next_portions(struct dsu_pctrl *dsu)
{
	unsigned long portions = dsu->cur_num_portions;
	int up = 0, down = 0;

	up = dsu_pctrl_up_size_check(dsu);
	if (up > 0)
		return dsu->cur_num_portions + up;

	if (dsu->alg.usec_down >= dsu->down_polling_ms) {
		down = dsu_pctrl_down_size_check(dsu);
		if (down > 0)
			portions = dsu->cur_num_portions - down;
	}

	return portions;
}

static int dsu_pctrl_governor_get_target_portions(struct devfreq *df,
						  unsigned long *portions)
{
	struct dsu_pctrl *dsu = dev_get_drvdata(df->dev.parent);
	int err;

	err = devfreq_update_stats(df);
	if (err)
		return err;

	*portions = dsu_pctrl_calc_next_portions(dsu);

	return 0;
}

static int dsu_pctrl_governor_event_handler(struct devfreq *devfreq,
					    unsigned int event, void *data)
{
	int ret = 0;

	switch (event) {
	case DEVFREQ_GOV_START:
		devfreq_monitor_start(devfreq);
		break;

	case DEVFREQ_GOV_STOP:
		devfreq_monitor_stop(devfreq);
		break;

	case DEVFREQ_GOV_SUSPEND:
		devfreq_monitor_suspend(devfreq);
		break;

	case DEVFREQ_GOV_RESUME:
		devfreq_monitor_resume(devfreq);
		break;
	}

	return ret;
}

static struct devfreq_governor dsu_pctrl_devfreq_governor = {
	.name		 = DSU_PCTRL_GOVERNOR_NAME,
	.get_target_freq = dsu_pctrl_governor_get_target_portions,
	.event_handler	 = dsu_pctrl_governor_event_handler,
	.flags = DEVFREQ_GOV_FLAG_IMMUTABLE | DEVFREQ_GOV_FLAG_IRQ_DRIVEN,
};

static void dsu_pctrl_handle_update(struct work_struct *work)
{
	struct dsu_pctrl *dsu = container_of(work, struct dsu_pctrl,
					     update_handle);

	mutex_lock(&dsu->devfreq->lock);
	update_devfreq(dsu->devfreq);
	mutex_unlock(&dsu->devfreq->lock);
}

static enum hrtimer_restart dsu_pctrl_polling(struct hrtimer *hrtimer)
{
	struct dsu_pctrl *dsu = container_of(hrtimer, struct dsu_pctrl,
					     poll_timer);

	queue_work(dsu->update_wq, &dsu->update_handle);

	hrtimer_forward_now(&dsu->poll_timer,
			    ms_to_ktime(dsu->up_polling_ms));
	return HRTIMER_RESTART;
}

static int dsu_pctrl_reinit_device(struct device *dev)
{
	struct dsu_pctrl *dsu = dev_get_drvdata(dev);

	/* Clean the algorithm statistics and start from scrach */
	memset(&dsu->alg, 0, sizeof(dsu->alg));
	dsu->alg.last_update = ktime_to_us(ktime_get());

	return dsu_pctrl_set_active_portions(dev, dsu->initial_freq);
}

static int dsu_pctrl_setup_devfreq_profile(struct platform_device *pdev)
{
	struct dsu_pctrl *dsu = platform_get_drvdata(pdev);
	struct devfreq_dev_profile *df_profile;

	dsu->devfreq_profile = devm_kzalloc(&pdev->dev,
			sizeof(struct devfreq_dev_profile), GFP_KERNEL);
	if (IS_ERR(dsu->devfreq_profile)) {
		dev_dbg(&pdev->dev, "no memory.\n");
		return PTR_ERR(dsu->devfreq_profile);
	}

	df_profile = dsu->devfreq_profile;

	df_profile->target = dsu_pctrl_devfreq_target;
	df_profile->get_dev_status = dsu_pctrl_devfreq_get_dev_status;
	df_profile->freq_table = dsu->freq_table;
	df_profile->max_state = dsu->freq_table_len;
	df_profile->polling_ms = DSU_PCTRL_DEFAULT_REQUIRED_GOV_POLLING;
	df_profile->initial_freq = dsu->initial_freq;

	return 0;
}

static int dsu_pctrl_parse_dt(struct platform_device *pdev)
{
	const struct of_device_id *of_id;
	struct device_node *node = pdev->dev.of_node;
	struct dsu_pctrl *dsu = platform_get_drvdata(pdev);
	int ret = 0;

	of_node_get(node);

	of_id = of_match_node(dsu_pctrl_devfreq_id, node);
	dsu->dsu_data = (struct dsu_pctrl_data *)of_id->data;

	ret = of_property_read_u32(node, "static-leakage-per-mb",
				   &dsu->static_leakage_per_mb);
	if (ret)
		dsu->static_leakage_per_mb = DSU_PCTRL_DEFAULT_CACHE_LEAKAGE;

	ret = of_property_read_u32(node, "dram-energy-per-mb",
				   &dsu->dram_energy_per_mb);
	if (ret)
		dsu->dram_energy_per_mb = DSU_PCTRL_DEFAULT_DRAM_ENERGY_PER_MB;

	ret = of_property_read_u32(node, "size", &dsu->size);
	if (ret)
		dsu->size = DSU_PCTRL_DEFAULT_SIZE_KB;

	ret = of_property_read_u32(node, "line-size", &dsu->line_size);
	if (ret)
		dsu->line_size = DSU_PCTRL_DEFAULT_LINE_SIZE;

	ret = of_property_read_u32(node, "polling", &dsu->up_polling_ms);
	if (ret)
		dsu->up_polling_ms = DSU_PCTRL_DEFAULT_POLLING_MS;
	dsu->down_polling_ms = POLLING_DOWN_INTERVAL * dsu->up_polling_ms;

	dsu->hw_control = of_property_read_bool(node, "hw-automatic-control");
	if (dsu->hw_control) {
		ret = of_property_read_u32(node, "cluster-l3-dn-th0",
                                           &dsu->cluster_l3_dn_th0);
		if (ret)
			dsu->cluster_l3_dn_th0 = CLUSTER_L3_DN_TH0_DEFAULT;

		ret = of_property_read_u32(node, "cluster-l3-dn-th1",
                                           &dsu->cluster_l3_dn_th1);
		if (ret)
			dsu->cluster_l3_dn_th1 = CLUSTER_L3_DN_TH1_DEFAULT;

		ret = of_property_read_u32(node, "cluster-l3-up-th0",
                                           &dsu->cluster_l3_up_th0);
		if (ret)
			dsu->cluster_l3_up_th0 = CLUSTER_L3_UP_TH0_DEFAULT;

		ret = of_property_read_u32(node, "cluster-l3-up-th1",
                                           &dsu->cluster_l3_up_th1);
		if (ret)
			dsu->cluster_l3_up_th1 = CLUSTER_L3_UP_TH1_DEFAULT;

		ret = of_property_read_u32(node, "cluster-l3-up-th2",
                                           &dsu->cluster_l3_up_th2);
		if (ret)
			dsu->cluster_l3_up_th2 = CLUSTER_L3_UP_TH2_DEFAULT;

		ret = of_property_read_u32(node, "cluster-l3-auto-interval",
                                           &dsu->cluster_l3_auto_interval);
		if (ret)
			dsu->cluster_l3_auto_interval = CIX_DSU_T;
	}

	of_node_put(node);

	return 0;
}

static int dsu_pctrl_create_configuration(struct platform_device *pdev)
{
	struct dsu_pctrl *dsu = platform_get_drvdata(pdev);
	struct dsu_pctrl_data *data = dsu->dsu_data;
	int i;
	unsigned long portion;

	dsu->initial_freq = data->portion_max;
	dsu->cur_num_portions = dsu->initial_freq;

	dsu->freq_table_len = data->portion_max - data->portion_min + 1;
	dsu->freq_table = devm_kcalloc(&pdev->dev, dsu->freq_table_len,
				       sizeof(*dsu->freq_table),
				       GFP_KERNEL);
	if (IS_ERR(dsu->freq_table))
		return -ENOMEM;

	portion = data->portion_min;
	for (i = 0; portion <= data->portion_max; i++)
		dsu->freq_table[i] = portion++;

	/* Leakage (static power) for a single portion (in uW) */
	dsu->static_leakage = dsu->static_leakage_per_mb * dsu->size;
	dsu->static_leakage /= SZ_1KB;
	dsu->static_leakage /= data->portion_max;

	/* Downsize and upsize thresholds are given in percentages */
	dsu->downsize_threshold = DOWNSIZE_PORTION_THRESHOLD;
	dsu->downsize_threshold *= ROUNDING_INTEGER_MUL / PERCENT_MAX;
	dsu->downsize_threshold *= dsu->static_leakage;
	dsu->downsize_threshold /= dsu->dram_energy_per_mb;

	dsu->upsize_threshold = PERCENT_MAX - UPSIZE_PORTION_THRESHOLD;
	dsu->upsize_threshold *= ROUNDING_INTEGER_MUL / PERCENT_MAX;
	dsu->upsize_threshold *= dsu->static_leakage;
	dsu->upsize_threshold /= dsu->dram_energy_per_mb;

	dsu->max_threshold = ROUNDING_INTEGER_MUL;
	dsu->max_threshold *= dsu->static_leakage;
	dsu->max_threshold /= dsu->dram_energy_per_mb;

	return 0;
}

static void dsu_pctrl_remove_opps(struct platform_device *pdev)
{
	struct dev_pm_opp *opp;
	int i, count;
	unsigned long freq;

	count = dev_pm_opp_get_opp_count(&pdev->dev);
	if (count <= 0)
		return;

	rcu_read_lock();
	for (i = 0, freq = 0; i < count; i++, freq++) {
		opp = dev_pm_opp_find_freq_ceil(&pdev->dev, &freq);
		if (!IS_ERR(opp))
			dev_pm_opp_remove(&pdev->dev, freq);
	}
	rcu_read_unlock();
}

static int dsu_pctrl_enable_opps(struct platform_device *pdev)
{
	struct dsu_pctrl *dsu = platform_get_drvdata(pdev);
	int i, ret;
	int opp_count = 0;

	if (dsu->freq_table_len <= 0)
		return -EINVAL;

	for (i = 0; i < dsu->freq_table_len; i++) {
		ret = dev_pm_opp_add(&pdev->dev, dsu->freq_table[i],
				     dsu->freq_table[i]);
		if (ret)
			dev_warn(&pdev->dev, "cannot add a new OPP.\n");
		else
			opp_count++;
	}

	if (opp_count == 0) {
		dev_err(&pdev->dev, "device has no OPP registered.\n");
		return -ENODEV;
	}

	return 0;
}

static int dsu_pctrl_setup(struct platform_device *pdev)
{
	struct dsu_pctrl *dsu = platform_get_drvdata(pdev);
	struct dsu_pctrl_data *data = dsu->dsu_data;
	int ret = 0;

	dsu->update_wq = create_workqueue("arm_dsu_pctrl_wq");
	if (IS_ERR(dsu->update_wq)) {
		dev_err(&pdev->dev, "cannot create workqueue.\n");
		return PTR_ERR(dsu->update_wq);
	}
	INIT_WORK(&dsu->update_handle, dsu_pctrl_handle_update);

	mutex_init(&dsu->lock);

	ret = dsu_pctrl_create_configuration(pdev);
	if (ret) {
		dev_err(&pdev->dev, "cannot create frequency table.\n");
		return ret;
	}

	ret = dsu_pctrl_enable_opps(pdev);
	if (ret) {
		dev_dbg(&pdev->dev, "device setup failed.\n");
		return ret;
	}

	ret = dsu_pctrl_setup_devfreq_profile(pdev);
	if (ret) {
		dev_dbg(&pdev->dev, "device setup failed.\n");
		return ret;
	}

	dsu->alg.last_update = ktime_to_us(ktime_get());

	dsu->devfreq = devm_devfreq_add_device(&pdev->dev,
					       dsu->devfreq_profile,
					       DSU_PCTRL_GOVERNOR_NAME, NULL);

	if (IS_ERR(dsu->devfreq)) {
		dev_err(&pdev->dev, "registering to devfreq failed.\n");
		return PTR_ERR(dsu->devfreq);
	}

	mutex_lock(&dsu->devfreq->lock);
	dsu->devfreq->scaling_min_freq = data->portion_min;
	dsu->devfreq->scaling_max_freq = data->portion_max;
	mutex_unlock(&dsu->devfreq->lock);

	return 0;
}


static int dsu_pctrl_init_device(struct platform_device *pdev)
{
	struct dsu_pctrl *dsu = platform_get_drvdata(pdev);

	dsu_pctrl_reinit_device(&pdev->dev);

	hrtimer_init(&dsu->poll_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	dsu->poll_timer.function = dsu_pctrl_polling;
	hrtimer_start(&dsu->poll_timer, ms_to_ktime(dsu->up_polling_ms),
		      HRTIMER_MODE_REL);

	return 0;
}

static int dsu_pctrl_suspend(struct device *dev)
{
	struct dsu_pctrl *dsu = dev_get_drvdata(dev);
	struct dsu_pctrl_data *data = dsu->dsu_data;
	int ret = 0;

	if (dsu->hw_control)
		return 0;

	if (hrtimer_active(&dsu->poll_timer))
	    hrtimer_cancel(&dsu->poll_timer);

	ret = devfreq_suspend_device(dsu->devfreq);
	if (ret < 0) {
		dev_err(dev, "failed to suspend devfreq device.\n");
		return ret;
	}

	/* Set the number of active portions to minimum during suspend */
	ret = dsu_pctrl_set_active_portions(dev, data->portion_min);
	if (ret < 0) {
		dev_err(dev, "failed to set portions to minimum.\n");
		return ret;
	}

	return ret;
}

static int dsu_pctrl_resume(struct device *dev)
{

	struct dsu_pctrl *dsu = dev_get_drvdata(dev);
	int ret = 0;

	if (dsu->hw_control)
		return 0;

	dsu_pctrl_reinit_device(dev);

	ret = devfreq_resume_device(dsu->devfreq);
	if (ret < 0)
		dev_err(dev, "failed to resume devfreq device.\n");

	if (!hrtimer_active(&dsu->poll_timer))
	    hrtimer_start(&dsu->poll_timer, ms_to_ktime(dsu->up_polling_ms),
			  HRTIMER_MODE_REL);

	return ret;
}

static SIMPLE_DEV_PM_OPS(dsu_pctrl_pm, dsu_pctrl_suspend, dsu_pctrl_resume);

static int dsu_pctrl_devfreq_probe(struct platform_device *pdev)
{
	struct dsu_pctrl *dsu;
	int ret = 0;
	struct arm_smccc_res res;

	dev_info(&pdev->dev, "registering DSU portion control device.\n");

	dsu = devm_kzalloc(&pdev->dev, sizeof(*dsu), GFP_KERNEL);
	if (!dsu)
		return -ENOMEM;

	platform_set_drvdata(pdev, dsu);
	dsu->pdev = pdev;
	dsu->id = atomic_inc_return(&dsu_pctrl_device_id);

	ret = dsu_pctrl_parse_dt(pdev);
	if (ret)
		goto failed;

	if (dsu->hw_control) {
		arm_smccc_smc(CIX_SIP_DSU_SET_TH, dsu->cluster_l3_dn_th0,
                              dsu->cluster_l3_dn_th1, dsu->cluster_l3_up_th0,
			      dsu->cluster_l3_up_th1, dsu->cluster_l3_up_th2,
			      0, 0, &res);
		if (!res.a0) {
			arm_smccc_smc(CIX_SIP_DSU_HW_CTL,
                                      dsu->cluster_l3_auto_interval, 0, 0, 0, 0,
                                      0, 0, &res);
			if (!res.a0)
				return 0;
		}
	}

	ret = dsu_pctrl_setup(pdev);
	if (ret)
		goto failed;

	ret = dsu_pctrl_init_device(pdev);
	if (ret)
		goto failed;

	return 0;
failed:
	dev_err(&pdev->dev, "failed to register driver, err %d.\n", ret);
	kfree(dsu);
	return ret;
}

static int dsu_pctrl_devfreq_remove(struct platform_device *pdev)
{
	struct dsu_pctrl *dsu = platform_get_drvdata(pdev);
	int ret = 0;

	dev_info(&pdev->dev, "unregistering DSU portion control device.\n");

	ret = dsu_pctrl_set_active_portions(&pdev->dev, dsu->initial_freq);

	/* Cancel hrtimer */
	if (hrtimer_active(&dsu->poll_timer))
		hrtimer_cancel(&dsu->poll_timer);
	/* Wait for pending work */
	flush_workqueue(dsu->update_wq);
	/* Destroy workqueue */
	destroy_workqueue(dsu->update_wq);

	devm_devfreq_remove_device(&pdev->dev, dsu->devfreq);
	dsu_pctrl_remove_opps(pdev);

	return ret;
}

MODULE_DEVICE_TABLE(of, dsu_pctrl_devfreq_id);

static struct platform_driver dsu_pctrl_devfreq_driver = {
	.probe	= dsu_pctrl_devfreq_probe,
	.remove = dsu_pctrl_devfreq_remove,
	.driver = {
		.name = DSU_PCTRL_PLATFORM_DEVICE_NAME,
		.of_match_table = dsu_pctrl_devfreq_id,
		.pm = &dsu_pctrl_pm,
		.owner = THIS_MODULE,
	},
};

static int __init dsu_pctrl_devfreq_init(void)
{
	int ret = 0;

	ret = devfreq_add_governor(&dsu_pctrl_devfreq_governor);
	if (ret) {
		pr_err("%s: failed to add governor: %d.\n", __func__, ret);
		return ret;
	}

	ret = platform_driver_register(&dsu_pctrl_devfreq_driver);
	if (ret)
		devfreq_remove_governor(&dsu_pctrl_devfreq_governor);

	return ret;
}

static void __exit dsu_pctrl_devfreq_exit(void)
{
	int ret;

	ret = devfreq_remove_governor(&dsu_pctrl_devfreq_governor);
	if (ret)
		pr_err("%s: failed to remove governor: %d.\n", __func__, ret);

	platform_driver_unregister(&dsu_pctrl_devfreq_driver);
}

module_init(dsu_pctrl_devfreq_init)
module_exit(dsu_pctrl_devfreq_exit)

MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("ARM DSU PCTRL devfreq driver");
MODULE_AUTHOR("ARM Ltd.");
MODULE_VERSION("1.0");
