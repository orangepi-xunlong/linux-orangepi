// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2020 MediaTek Inc.
 *
 * Copyright 2024 Cix Technology Group Co., Ltd.
 */

#define pr_fmt(fmt) "[cix_sensorhub]" fmt

#include <linux/slab.h>
#include <linux/workqueue.h>
#include <linux/kthread.h>
#include <linux/platform_device.h>
#include <linux/dma-mapping.h>
#include <linux/atomic.h>
#include <linux/types.h>
#include <linux/module.h>
#include <linux/suspend.h>
#include <linux/pm_wakeup.h>
#include <linux/timer.h>
#include <linux/timekeeping.h>
#include <asm/arch_timer.h>
#include <linux/math64.h>
#include <linux/delay.h>
#include <uapi/linux/sched/types.h>
#include <scp.h>

#include "cix_sensorhub.h"
#include "hf_manager.h"
#include "sensor_list.h"
#include "sensorhub_ipi.h"

/* ALGIN TO SCP SENSOR_IPI_SIZE AT FILE CONTEXTHUB_FW.H, ALGIN
 * TO SCP_SENSOR_HUB_DATA UNION, ALGIN TO STRUCT DATA_UNIT_T
 * SIZEOF(STRUCT DATA_UNIT_T) = SCP_SENSOR_HUB_DATA = SENSOR_IPI_SIZE
 * BUT AT THE MOMENT AP GET DATA THROUGH IPI, WE ONLY TRANSFER
 * 44 BYTES DATA_UNIT_T, THERE ARE 4 BYTES HEADER IN SCP_SENSOR_HUB_DATA
 * HEAD
 */
#define SENSOR_IPI_SIZE 48
/*
 * experience number for delay_count per DELAY_COUNT sensor input delay 10ms
 * msleep(10) system will schedule to hal process then read input node
 */
#define SENSOR_IPI_HEADER_SIZE 4
#define SENSOR_IPI_PACKET_SIZE (SENSOR_IPI_SIZE - SENSOR_IPI_HEADER_SIZE)
#define SENSOR_DATA_SIZE 44

#if SENSOR_DATA_SIZE > SENSOR_IPI_PACKET_SIZE
#error "SENSOR_DATA_SIZE > SENSOR_IPI_PACKET_SIZE, out of memory"
#endif

enum {
	CUST_CMD_CALI,
	MAX_CUST_CMD,
};

struct curr_wp_queue {
	spinlock_t buffer_lock;
	uint32_t head;
	uint32_t tail;
	uint32_t bufsize;
	uint32_t *ringbuffer;
};

struct cix_nanohub_device {
	struct hf_device hf_dev;
	struct timer_list sync_time_timer;
	struct work_struct sync_time_worker;

	struct sensor_fifo *scp_sensor_fifo;
	struct curr_wp_queue wp_queue;
	phys_addr_t shub_dram_phys;
	phys_addr_t shub_dram_virt;

	atomic_t cfg_data_after_reboot;
	atomic_t create_manager_first_boot;

	int32_t acc_config_data[6];
	int32_t gyro_config_data[12];
	int32_t mag_config_data[9];
	int32_t light_config_data[1];
	int32_t proximity_config_data[2];
	int32_t pressure_config_data[2];
	int32_t sar_config_data[4];
	int32_t ois_config_data[2];
};

static struct SensorState sensor_state[SENSOR_TYPE_SENSOR_MAX];
static int64_t raw_ts_reverse_debug[SENSOR_TYPE_SENSOR_MAX];
static int64_t comp_ts_reverse_debug[SENSOR_TYPE_SENSOR_MAX];
static struct sensor_info support_sensors[SENSOR_TYPE_SENSOR_MAX];
static int support_size;
static DEFINE_MUTEX(sensor_state_mtx);
static DEFINE_MUTEX(flush_mtx);
static atomic_t power_status = ATOMIC_INIT(SENSOR_POWER_DOWN);
static DECLARE_WAIT_QUEUE_HEAD(chre_kthread_wait);
static DECLARE_WAIT_QUEUE_HEAD(power_reset_wait);
static uint8_t chre_kthread_wait_condition;
static DEFINE_SPINLOCK(scp_state_lock);
static DEFINE_SPINLOCK(config_data_lock);
static uint8_t scp_system_ready;
static uint8_t scp_chre_ready;
static struct cix_nanohub_device *cix_nanohub_dev;

static int cix_nanohub_server_dispatch_data(uint32_t *currWp);
static int cix_nanohub_report_to_manager(struct data_unit_t *data);
static int cix_nanohub_create_manager(void);

struct cix_nanohub_cmd {
	uint32_t reason;
	void (*handler)(union SCP_SENSOR_HUB_DATA *rsp, unsigned int rx_len);
};

#define cix_NANOHUB_CMD(_reason, _handler) \
	{.reason = _reason, .handler = _handler}

#define type_to_id(type) (type - ID_OFFSET)
#define id_to_type(id) (id + ID_OFFSET)

int cix_nanohub_req_send(union SCP_SENSOR_HUB_DATA *data)
{
	int ret = 0;

	if (data->req.sensorType >= ID_SENSOR_MAX) {
		pr_err("invalid sensor type %d when req send ipi\n",
			data->rsp.sensorType);
		return -1;
	}
	ret = cix_nanohub_ipi_sync((unsigned char *)data,
		SENSOR_IPI_SIZE);
	if (ret != 0 || data->rsp.errCode != 0)
		return -1;
	return 0;
}

static void cix_nanohub_write_wp_queue(union SCP_SENSOR_HUB_DATA *rsp)
{
	struct cix_nanohub_device *device = cix_nanohub_dev;
	struct curr_wp_queue *wp_queue = &device->wp_queue;

	spin_lock(&wp_queue->buffer_lock);
	wp_queue->ringbuffer[wp_queue->head++] = rsp->notify_rsp.currWp;
	wp_queue->head &= wp_queue->bufsize - 1;
	if (unlikely(wp_queue->head == wp_queue->tail))
		pr_err_ratelimited("%s drop wp\n", __func__);
	spin_unlock(&wp_queue->buffer_lock);
}

static int cix_nanohub_fetch_next_wp(uint32_t *currWp)
{
	int have_event;
	struct cix_nanohub_device *device = cix_nanohub_dev;
	struct curr_wp_queue *wp_queue = &device->wp_queue;

	spin_lock_irq(&wp_queue->buffer_lock);

	have_event = wp_queue->head != wp_queue->tail;
	if (have_event) {
		*currWp = wp_queue->ringbuffer[wp_queue->tail++];
		wp_queue->tail &= wp_queue->bufsize - 1;
	}
	spin_unlock_irq(&wp_queue->buffer_lock);

	return have_event;
}

static int cix_nanohub_read_wp_queue(void)
{
	uint32_t currWp = 0;

	while (cix_nanohub_fetch_next_wp(&currWp)) {
		if (cix_nanohub_server_dispatch_data(&currWp))
			return -EFAULT;
	}
	return 0;
}

static int cix_nanohub_direct_push_work(void *data)
{
	for (;;) {
		wait_event(chre_kthread_wait,
			READ_ONCE(chre_kthread_wait_condition));
		WRITE_ONCE(chre_kthread_wait_condition, false);
		cix_nanohub_read_wp_queue();
	}
	return 0;
}

static void cix_nanohub_common_cmd(union SCP_SENSOR_HUB_DATA *rsp,
					unsigned int rx_len)
{
	cix_nanohub_ipi_complete((unsigned char *)rsp, rx_len);
}

static void cix_nanohub_notify_cmd(union SCP_SENSOR_HUB_DATA *rsp,
		unsigned int rx_len)
{
	unsigned long flags = 0;

	switch (rsp->notify_rsp.event) {
	case SCP_DIRECT_PUSH:
	case SCP_FIFO_FULL:
		cix_nanohub_write_wp_queue(rsp);
		WRITE_ONCE(chre_kthread_wait_condition, true);
		wake_up(&chre_kthread_wait);
		break;
	case SCP_NOTIFY:
		break;
	case SCP_INIT_DONE:
		pr_info("notify cmd SCP_INIT_DONE\n");
		spin_lock_irqsave(&scp_state_lock, flags);
		WRITE_ONCE(scp_chre_ready, true);
		if (READ_ONCE(scp_system_ready) && READ_ONCE(scp_chre_ready)) {
			spin_unlock_irqrestore(&scp_state_lock, flags);
			atomic_set(&power_status, SENSOR_POWER_UP);
			wake_up(&power_reset_wait);
		} else
			spin_unlock_irqrestore(&scp_state_lock, flags);
		break;
	default:
		break;
	}
}

static const struct cix_nanohub_cmd cix_nanohub_cmds[] = {
	cix_NANOHUB_CMD(SENSOR_HUB_NOTIFY,
		cix_nanohub_notify_cmd),
	cix_NANOHUB_CMD(SENSOR_HUB_GET_DATA,
		cix_nanohub_common_cmd),
	cix_NANOHUB_CMD(SENSOR_HUB_SET_CONFIG,
		cix_nanohub_common_cmd),
	cix_NANOHUB_CMD(SENSOR_HUB_SET_CUST,
		cix_nanohub_common_cmd),
	cix_NANOHUB_CMD(SENSOR_HUB_SET_TIMESTAMP,
		cix_nanohub_common_cmd),
	cix_NANOHUB_CMD(SENSOR_HUB_RAW_DATA,
		cix_nanohub_common_cmd),
};

const struct cix_nanohub_cmd *
cix_nanohub_find_cmd(uint32_t packetReason)
{
	int i;
	const struct cix_nanohub_cmd *cmd;

	for (i = 0; i < ARRAY_SIZE(cix_nanohub_cmds); i++) {
		cmd = &cix_nanohub_cmds[i];
		if (cmd->reason == packetReason)
			return cmd;
	}
	return NULL;
}

void cix_nanohub_ipi_handler(int id,
		void *data, unsigned int len)
{
	union SCP_SENSOR_HUB_DATA *rsp = (union SCP_SENSOR_HUB_DATA *)data;
	const struct cix_nanohub_cmd *cmd;

	if (len > SENSOR_IPI_SIZE) {
		pr_err("IPI_SENSOR len=%d error\n", len);
		return;
	}

	cmd = cix_nanohub_find_cmd(rsp->rsp.action);
	if (cmd != NULL)
		cmd->handler(rsp, len);
	else
		pr_err("IPI_SENSOR cannot find cmd!\n");
}

static void cix_nanohub_get_sensor_info(void)
{
	int i = 0, j = 0, k = 0;
	int count = ARRAY_SIZE(sensor_state);
	int size = ARRAY_SIZE(support_sensors);

	k = (count < size) ? count : size;
	for (i = 0; i < k; ++i) {
		if (!sensor_state[i].sensorType)
			continue;
		support_sensors[j].sensor_type = sensor_state[i].sensorType;
		support_sensors[j].gain = sensor_state[i].gain;
		strlcpy(support_sensors[j].name, sensor_state[i].name,
				sizeof(support_sensors[j].name));
		strlcpy(support_sensors[j].vendor, sensor_state[i].vendor,
				sizeof(support_sensors[j].vendor));
		j++;
	}
	support_size = j;
}

static void cix_nanohub_set_sensor_info(struct sensor_info *info,
		bool status)
{
	struct SensorState *p = NULL;

	if (info->sensor_type >= SENSOR_TYPE_SENSOR_MAX)
		return;

	p = &sensor_state[info->sensor_type];
	if (!status)
		p->sensorType = 0;
	else {
		strlcpy(p->name, info->name, sizeof(p->name));
		if (strlen(info->vendor))
			strlcpy(p->vendor, info->vendor, sizeof(p->vendor));
	}
}

static void cix_nanohub_init_sensor_info(void)
{
	struct SensorState *p = NULL;

	// gain and vendor info to be modified

	p = &sensor_state[SENSOR_TYPE_ACCELEROMETER];
	p->sensorType = SENSOR_TYPE_ACCELEROMETER;
	p->gain = 1000;
	strlcpy(p->name, "accelerometer", sizeof(p->name));
	strlcpy(p->vendor, "cix", sizeof(p->vendor));

	p = &sensor_state[SENSOR_TYPE_GYROSCOPE];
	p->sensorType = SENSOR_TYPE_GYROSCOPE;
	p->gain = 1000000;
	strlcpy(p->name, "gyroscope", sizeof(p->name));
	strlcpy(p->vendor, "cix", sizeof(p->vendor));

	p = &sensor_state[SENSOR_TYPE_MAGNETIC_FIELD];
	p->sensorType = SENSOR_TYPE_MAGNETIC_FIELD;
	p->gain = 100;
	strlcpy(p->name, "magnetic", sizeof(p->name));
	strlcpy(p->vendor, "cix", sizeof(p->vendor));

	p = &sensor_state[SENSOR_TYPE_LIGHT];
	p->sensorType = SENSOR_TYPE_LIGHT;
	p->gain = 1;
	strlcpy(p->name, "light", sizeof(p->name));
	strlcpy(p->vendor, "cix", sizeof(p->vendor));

	p = &sensor_state[SENSOR_TYPE_PROXIMITY];
	p->sensorType = SENSOR_TYPE_PROXIMITY;
	p->gain = 1;
	strlcpy(p->name, "proximity", sizeof(p->name));
	strlcpy(p->vendor, "cix", sizeof(p->vendor));

	p = &sensor_state[SENSOR_TYPE_PRESSURE];
	p->sensorType = SENSOR_TYPE_PRESSURE;
	p->gain = 100;
	strlcpy(p->name, "pressure", sizeof(p->name));
	strlcpy(p->vendor, "cix", sizeof(p->vendor));

}

static void init_sensor_config_cmd(struct ConfigCmd *cmd,
		int sensor_type)
{
	uint8_t alt = sensor_state[sensor_type].alt;
	bool enable = 0;

	memset(cmd, 0x00, sizeof(*cmd));

	cmd->evtType = EVT_NO_SENSOR_CONFIG_EVENT;
	cmd->sensorType = sensor_state[sensor_type].sensorType;

	if (alt && sensor_state[alt].enable &&
			sensor_state[sensor_type].enable) {
		cmd->cmd = CONFIG_CMD_ENABLE;
		if (sensor_state[alt].rate > sensor_state[sensor_type].rate)
			cmd->rate = sensor_state[alt].rate;
		else
			cmd->rate = sensor_state[sensor_type].rate;
		if (sensor_state[alt].latency <
				sensor_state[sensor_type].latency)
			cmd->latency = sensor_state[alt].latency;
		else
			cmd->latency = sensor_state[sensor_type].latency;
	} else if (alt && sensor_state[alt].enable) {
		enable = sensor_state[alt].enable;
		cmd->cmd =  enable ? CONFIG_CMD_ENABLE : CONFIG_CMD_DISABLE;
		cmd->rate = sensor_state[alt].rate;
		cmd->latency = sensor_state[alt].latency;
	} else { /* !alt || !sensor_state[alt].enable */
		enable = sensor_state[sensor_type].enable;
		cmd->cmd = enable ? CONFIG_CMD_ENABLE : CONFIG_CMD_DISABLE;
		cmd->rate = sensor_state[sensor_type].rate;
		cmd->latency = sensor_state[sensor_type].latency;
	}
}

static int cix_nanohub_report_data(struct data_unit_t *data_t)
{
	int err = 0, sensor_type = 0, sensor_id = 0;
	int64_t raw_time = 0, comp_time = 0;

	sensor_id = data_t->sensor_type;
	if (sensor_id >= ID_SENSOR_MAX || sensor_id < 0) {
		pr_err("invalid sensor id %d\n", sensor_id);
		return 0;
	}

	sensor_type = id_to_type(sensor_id);
	raw_time = data_t->time_stamp;
	data_t->time_stamp += 0;
	comp_time = data_t->time_stamp;

	do {
		/* must check report return value for retry sending */
		if (data_t->flush_action != FLUSH_ACTION) {
			err = cix_nanohub_report_to_manager(data_t);
		} else {
			/* for flush only !err true we decrease flushcnt */
			mutex_lock(&flush_mtx);
			if (sensor_state[sensor_type].flushcnt > 0) {
				err = cix_nanohub_report_to_manager(data_t);
				if (!err)
					sensor_state[sensor_type].flushcnt--;
			} else {
				/* no need flush err must reset to 0 */
				err = 0;
			}
			mutex_unlock(&flush_mtx);
		}
		/* for debugging timestamp reverse */
		if (data_t->flush_action == DATA_ACTION && !err) {
			if (unlikely(raw_time <
					raw_ts_reverse_debug[sensor_type])) {
				pr_err("raw reverse %d,%lld,%lld\n",
					sensor_type,
					raw_ts_reverse_debug[sensor_type],
					raw_time);
			}
			raw_ts_reverse_debug[sensor_type] = raw_time;
			if (unlikely(comp_time <
					comp_ts_reverse_debug[sensor_type])) {
				pr_err("comp reverse %d,%lld,%lld,%lld\n",
					sensor_type,
					raw_ts_reverse_debug[sensor_type],
					comp_ts_reverse_debug[sensor_type],
					comp_time);
			}
			comp_ts_reverse_debug[sensor_type] = comp_time;
		}
		if (err < 0)
			usleep_range(2000, 4000);
	} while (err < 0);

	return err;
}

static int cix_nanohub_server_dispatch_data(uint32_t *currWp)
{
	struct cix_nanohub_device *device = cix_nanohub_dev;
	char *pStart, *pEnd, *rp, *wp;
	struct data_unit_t event;
	uint32_t wp_copy;

	pStart = (char *)READ_ONCE(device->scp_sensor_fifo) +
		offsetof(struct sensor_fifo, data);
	pEnd = pStart +  READ_ONCE(device->scp_sensor_fifo->fifo_size);
	wp_copy = *currWp * SENSOR_DATA_SIZE;
	rp = pStart + READ_ONCE(device->scp_sensor_fifo->rp);
	wp = pStart + wp_copy;

	if (wp < pStart || pEnd < wp) {
		pr_err("FIFO wp invalid : %p, %p, %p\n", pStart, pEnd, wp);
		return -5;
	}
	if (rp == wp) {
		pr_err("FIFO empty\n");
		return 0;
	}
	/*
	 * opimize for dram,no cache,we should cpy data to cacheable ram
	 * event and event_copy are cacheable ram, cix_nanohub_report_data
	 * will change time_stamp field, so when cix_nanohub_report_data fail
	 * we should reinit the time_stamp by memcpy to event_copy;
	 * why memcpy_fromio(&event_copy), because rp is not cacheable
	 */
	if (rp < wp) {
		while (rp < wp) {
			memcpy_fromio(&event, rp, SENSOR_DATA_SIZE);
			cix_nanohub_report_data(&event);
			rp += SENSOR_DATA_SIZE;
		}
	} else if (rp > wp) {
		while (rp < pEnd) {
			memcpy_fromio(&event, rp, SENSOR_DATA_SIZE);
			cix_nanohub_report_data(&event);
			rp += SENSOR_DATA_SIZE;
		}
		rp = pStart;
		while (rp < wp) {
			memcpy_fromio(&event, rp, SENSOR_DATA_SIZE);
			cix_nanohub_report_data(&event);
			rp += SENSOR_DATA_SIZE;
		}
	}
	/*
	 * must device->scp_sensor_fifo->rp = wp,
	 * not device->scp_sensor_fifo->rp = device->scp_sensor_fifo->wp
	 */
	WRITE_ONCE(device->scp_sensor_fifo->rp, wp_copy);
	return 0;
}

static int cix_nanohub_send_dram_info_to_hub(void)
{
	struct cix_nanohub_device *device = cix_nanohub_dev;
	union SCP_SENSOR_HUB_DATA data;
	unsigned int len = 0;
	int err = 0, retry = 0, total = 10;

	device->shub_dram_phys = scp_get_reserve_mem_phys(SENS_MEM_ID);
	device->shub_dram_virt = scp_get_reserve_mem_virt(SENS_MEM_ID);

	data.set_config_req.sensorType = 0;
	data.set_config_req.action = SENSOR_HUB_SET_CONFIG;
	data.set_config_req.bufferBase =
		(unsigned int)(device->shub_dram_phys & 0xFFFFFFFF);

	len = sizeof(data.set_config_req);
	for (retry = 0; retry < total; ++retry) {
		err = cix_nanohub_req_send(&data);
		if (err < 0 || data.rsp.action != SENSOR_HUB_SET_CONFIG) {
			pr_err("%s fail!\n", __func__);
			continue;
		}
		break;
	}
	if (retry < total)
		pr_notice("%s success\n", __func__);
	return SCP_SENSOR_HUB_SUCCESS;
}

static int cix_nanohub_enable_rawdata_to_hub(int sensor_id,
		int en)
{
	int err = 0;
	union SCP_SENSOR_HUB_DATA req;

	req.req.sensorType = sensor_id;
	req.req.action = SENSOR_HUB_RAW_DATA;
	req.req.data[0] = en;

	err = cix_nanohub_req_send(&req);
	if (err < 0 || sensor_id != req.rsp.sensorType ||
			req.rsp.action != SENSOR_HUB_RAW_DATA) {
		pr_err("%s fail!\n", __func__);
		return -1;
	}
	return err;
}

static void cix_nanohub_disable_report_flush(uint8_t sensor_id)
{
	uint8_t sensor_type = id_to_type(sensor_id);
	struct data_unit_t data_t;
	int ret = 0, retry = 0;

	/*
	 * disable sensor only check func return err 5 times
	 */
	mutex_lock(&flush_mtx);
	while (sensor_state[sensor_type].flushcnt > 0) {
		sensor_state[sensor_type].flushcnt--;
		memset(&data_t, 0, sizeof(struct data_unit_t));
		data_t.sensor_type = sensor_id;
		data_t.flush_action = FLUSH_ACTION;
		do {
			ret = cix_nanohub_report_to_manager(&data_t);
			if (ret < 0)
				usleep_range(2000, 4000);
		} while (ret < 0 && retry++ < 5);
		if (ret < 0)
			pr_err("%d flush complete err when disable\n",
				sensor_type);
	}
	mutex_unlock(&flush_mtx);
}

int cix_nanohub_enable_to_hub(uint8_t sensor_id, int enabledisable)
{
	uint8_t sensor_type = id_to_type(sensor_id);
	struct ConfigCmd cmd;
	int ret = 0;

	if (enabledisable == 1 && (READ_ONCE(scp_system_ready)))
		sfh_register_feature(SENS_FEATURE_ID);
	mutex_lock(&sensor_state_mtx);
	if (sensor_id >= ID_SENSOR_MAX) {
		pr_err("invalid id %d\n", sensor_id);
		mutex_unlock(&sensor_state_mtx);
		return -1;
	}
	if (!sensor_state[sensor_type].sensorType) {
		pr_err("unhandle id %d, is inited?\n", sensor_id);
		mutex_unlock(&sensor_state_mtx);
		return -1;
	}
	sensor_state[sensor_type].enable = enabledisable;
	init_sensor_config_cmd(&cmd, sensor_type);
	if (atomic_read(&power_status) == SENSOR_POWER_UP) {
		ret = sensorhub_external_write((const uint8_t *)&cmd,
			sizeof(struct ConfigCmd));
		if (ret < 0)
			pr_err("fail enable: [%d,%d]\n", sensor_id, cmd.cmd);
	}
	if (!enabledisable)
		cix_nanohub_disable_report_flush(sensor_id);
	mutex_unlock(&sensor_state_mtx);
	return ret < 0 ? ret : 0;
}

int cix_nanohub_batch_to_hub(uint8_t sensor_id,
		int flag, int64_t samplingPeriodNs,
		int64_t maxBatchReportLatencyNs)
{
	uint8_t sensor_type = id_to_type(sensor_id);
	struct ConfigCmd cmd;
	int ret = 0;
	uint64_t rate = 1024000000000ULL;

	mutex_lock(&sensor_state_mtx);
	if (sensor_id >= ID_SENSOR_MAX) {
		pr_err("invalid id %d\n", sensor_id);
		mutex_unlock(&sensor_state_mtx);
		return -1;
	}
	if (!sensor_state[sensor_type].sensorType) {
		pr_err("unhandle type %d, is inited?\n", sensor_type);
		mutex_unlock(&sensor_state_mtx);
		return -1;
	}
	if (samplingPeriodNs > 0 &&
		sensor_state[sensor_type].rate != SENSOR_RATE_ONCHANGE &&
		sensor_state[sensor_type].rate != SENSOR_RATE_ONESHOT) {
		rate = div64_u64(rate, samplingPeriodNs);
		sensor_state[sensor_type].rate = rate;
	}
	sensor_state[sensor_type].latency = maxBatchReportLatencyNs;
	init_sensor_config_cmd(&cmd, sensor_type);
	if (atomic_read(&power_status) == SENSOR_POWER_UP) {
		ret = sensorhub_external_write((const uint8_t *)&cmd,
			sizeof(struct ConfigCmd));
		if (ret < 0)
			pr_err("failed batch: [%d,%d,%lld,%d]\n",
				sensor_id, cmd.rate, cmd.latency, cmd.cmd);
	}
	mutex_unlock(&sensor_state_mtx);
	return ret < 0 ? ret : 0;
}

int cix_nanohub_flush_to_hub(uint8_t sensor_id)
{
	uint8_t sensor_type = id_to_type(sensor_id);
	struct ConfigCmd cmd;
	int ret = 0;

	mutex_lock(&sensor_state_mtx);
	if (sensor_id >= ID_SENSOR_MAX) {
		pr_err("invalid id %d\n", sensor_id);
		mutex_unlock(&sensor_state_mtx);
		return -1;
	}
	if (!sensor_state[sensor_type].sensorType) {
		pr_err("unhandle id %d, is inited?\n", sensor_id);
		mutex_unlock(&sensor_state_mtx);
		return -1;
	}
	/*
	 * add count must before flush, if we add count after
	 * flush right return and flush callback directly report
	 * flush will lose flush complete
	 */
	mutex_lock(&flush_mtx);
	sensor_state[sensor_type].flushcnt++;
	mutex_unlock(&flush_mtx);
	init_sensor_config_cmd(&cmd, sensor_type);
	cmd.cmd = CONFIG_CMD_FLUSH;
	if (atomic_read(&power_status) == SENSOR_POWER_UP) {
		ret = sensorhub_external_write((const uint8_t *)&cmd,
			sizeof(struct ConfigCmd));
		if (ret < 0) {
			pr_err("failed flush: [%d]\n", sensor_id);
			mutex_lock(&flush_mtx);
			if (sensor_state[sensor_type].flushcnt > 0)
				sensor_state[sensor_type].flushcnt--;
			mutex_unlock(&flush_mtx);
		}
	}
	mutex_unlock(&sensor_state_mtx);
	return ret < 0 ? ret : 0;
}

int cix_nanohub_cfg_to_hub(uint8_t sensor_id, uint8_t *data, uint8_t count)
{
	struct ConfigCmd *cmd = NULL;
	int ret = 0;

	if (sensor_id >= ID_SENSOR_MAX) {
		pr_err("invalid id %d\n", sensor_id);
		return -1;
	}
	cmd = vzalloc(sizeof(struct ConfigCmd) + count);
	if (!cmd)
		return -1;
	cmd->evtType = EVT_NO_SENSOR_CONFIG_EVENT;
	cmd->sensorType = id_to_type(sensor_id);
	cmd->cmd = CONFIG_CMD_CFG_DATA;
	memcpy(cmd->data, data, count);
	if (atomic_read(&power_status) == SENSOR_POWER_UP) {
		ret = sensorhub_external_write((const uint8_t *)cmd,
			sizeof(struct ConfigCmd) + count);
		if (ret < 0)
			pr_err("failed cfg: [%d,%d]\n", sensor_id, cmd->cmd);
	}
	vfree(cmd);
	return ret < 0 ? ret : 0;
}

int cix_nanohub_calibration_to_hub(uint8_t sensor_id)
{
	uint8_t sensor_type = id_to_type(sensor_id);
	struct ConfigCmd cmd;
	int ret = 0;

	if (sensor_id >= ID_SENSOR_MAX) {
		pr_err("invalid id %d\n", sensor_id);
		return -1;
	}
	if (!sensor_state[sensor_type].sensorType) {
		pr_err("unhandle id %d, is inited?\n", sensor_id);
		return -1;
	}
	init_sensor_config_cmd(&cmd, sensor_type);
	cmd.cmd = CONFIG_CMD_CALIBRATE;
	if (atomic_read(&power_status) == SENSOR_POWER_UP) {
		ret = sensorhub_external_write((const uint8_t *)&cmd,
			sizeof(struct ConfigCmd));
		if (ret < 0)
			pr_err("failed calibration: [%d]\n", sensor_id);
	}
	return ret < 0 ? ret : 0;
}

int cix_nanohub_selftest_to_hub(uint8_t sensor_id)
{
	uint8_t sensor_type = id_to_type(sensor_id);
	struct ConfigCmd cmd;
	int ret = 0;

	if (sensor_id >= ID_SENSOR_MAX) {
		pr_err("invalid id %d\n", sensor_id);
		return -1;
	}
	if (!sensor_state[sensor_type].sensorType) {
		pr_err("unhandle id %d, is inited?\n", sensor_id);
		return -1;
	}
	init_sensor_config_cmd(&cmd, sensor_type);
	cmd.cmd = CONFIG_CMD_SELF_TEST;
	if (atomic_read(&power_status) == SENSOR_POWER_UP) {
		ret = sensorhub_external_write((const uint8_t *)&cmd,
			sizeof(struct ConfigCmd));
		if (ret < 0)
			pr_err("failed selfttest: [%d]\n", sensor_id);
	}
	return ret < 0 ? ret : 0;
}

int cix_nanohub_get_data_from_hub(uint8_t sensor_id,
		struct data_unit_t *data)
{
	union SCP_SENSOR_HUB_DATA req;
	struct data_unit_t *data_t;
	int len = 0, err = 0;

	if (atomic_read(&power_status) == SENSOR_POWER_DOWN) {
		pr_err("scp power down, we can not access scp\n");
		return -1;
	}

	req.get_data_req.sensorType = sensor_id;
	req.get_data_req.action = SENSOR_HUB_GET_DATA;
	len = sizeof(req.get_data_req);
	err = cix_nanohub_req_send(&req);
	if (err < 0) {
		pr_err("get_data fail:%d!\n", err);
		return -1;
	}
	if (sensor_id != req.get_data_rsp.sensorType ||
		req.get_data_rsp.action != SENSOR_HUB_GET_DATA ||
		req.get_data_rsp.errCode != 0) {
		pr_err("req id: %d, rsp Type:%d action:%d, errcode:%d\n",
			sensor_id, req.get_data_rsp.sensorType,
			req.get_data_rsp.action, req.get_data_rsp.errCode);

		return req.get_data_rsp.errCode;
	}

	data_t = (struct data_unit_t *)req.get_data_rsp.data.int8_Data;
	switch (sensor_id) {
	case ID_ACCELEROMETER:
		data->time_stamp = data_t->time_stamp;
		data->accelerometer_t.x = data_t->accelerometer_t.x;
		data->accelerometer_t.y = data_t->accelerometer_t.y;
		data->accelerometer_t.z = data_t->accelerometer_t.z;
		data->accelerometer_t.x_bias = data_t->accelerometer_t.x_bias;
		data->accelerometer_t.y_bias = data_t->accelerometer_t.y_bias;
		data->accelerometer_t.z_bias = data_t->accelerometer_t.z_bias;
		data->accelerometer_t.status = data_t->accelerometer_t.status;
		break;
	case ID_LIGHT:
		data->time_stamp = data_t->time_stamp;
		data->light = data_t->light;
		break;
	case ID_PROXIMITY:
		data->time_stamp = data_t->time_stamp;
		data->proximity_t.steps = data_t->proximity_t.steps;
		data->proximity_t.oneshot = data_t->proximity_t.oneshot;
		break;
	case ID_PRESSURE:
		data->time_stamp = data_t->time_stamp;
		data->pressure_t.pressure = data_t->pressure_t.pressure;
		data->pressure_t.status = data_t->pressure_t.status;
		break;
	case ID_GYROSCOPE:
		data->time_stamp = data_t->time_stamp;
		data->gyroscope_t.x = data_t->gyroscope_t.x;
		data->gyroscope_t.y = data_t->gyroscope_t.y;
		data->gyroscope_t.z = data_t->gyroscope_t.z;
		data->gyroscope_t.x_bias = data_t->gyroscope_t.x_bias;
		data->gyroscope_t.y_bias  = data_t->gyroscope_t.y_bias;
		data->gyroscope_t.z_bias  = data_t->gyroscope_t.z_bias;
		data->gyroscope_t.status = data_t->gyroscope_t.status;
		break;
	case ID_MAGNETIC_FIELD:
		data->time_stamp = data_t->time_stamp;
		data->magnetic_t.x = data_t->magnetic_t.x;
		data->magnetic_t.y = data_t->magnetic_t.y;
		data->magnetic_t.z = data_t->magnetic_t.z;
		data->magnetic_t.x_bias = data_t->magnetic_t.x_bias;
		data->magnetic_t.y_bias = data_t->magnetic_t.y_bias;
		data->magnetic_t.z_bias = data_t->magnetic_t.z_bias;
		data->magnetic_t.status = data_t->magnetic_t.status;
		break;
	default:
		err = -1;
		break;
	}
	return err;
}

int cix_nanohub_set_cmd_to_hub(uint8_t sensor_id,
		enum CUST_ACTION action, void *data)
{
	union SCP_SENSOR_HUB_DATA req;
	int len = 0, err = 0;

	req.get_data_req.sensorType = sensor_id;
	req.get_data_req.action = SENSOR_HUB_SET_CUST;

	if (atomic_read(&power_status) == SENSOR_POWER_DOWN) {
		pr_err("scp power down, we can not access scp\n");
		return -1;
	}

	switch (sensor_id) {
	case ID_ACCELEROMETER:
		req.set_cust_req.sensorType = ID_ACCELEROMETER;
		req.set_cust_req.action = SENSOR_HUB_SET_CUST;
		switch (action) {
		case CUST_ACTION_GET_SENSOR_INFO:
			req.set_cust_req.getInfo.action =
				CUST_ACTION_GET_SENSOR_INFO;
			len = offsetof(struct SCP_SENSOR_HUB_SET_CUST_REQ,
				custData) + sizeof(req.set_cust_req.getInfo);
			break;
		default:
			return -1;
		}
		break;
	case ID_LIGHT:
		req.set_cust_req.sensorType = ID_LIGHT;
		req.set_cust_req.action = SENSOR_HUB_SET_CUST;
		switch (action) {
		case CUST_ACTION_GET_SENSOR_INFO:
			req.set_cust_req.getInfo.action =
				CUST_ACTION_GET_SENSOR_INFO;
			len = offsetof(struct SCP_SENSOR_HUB_SET_CUST_REQ,
				custData) + sizeof(req.set_cust_req.getInfo);
			break;
		default:
			return -1;
		}
		break;
	case ID_PROXIMITY:
		req.set_cust_req.sensorType = ID_PROXIMITY;
		req.set_cust_req.action = SENSOR_HUB_SET_CUST;
		switch (action) {
		case CUST_ACTION_GET_SENSOR_INFO:
			req.set_cust_req.getInfo.action =
				CUST_ACTION_GET_SENSOR_INFO;
			len = offsetof(struct SCP_SENSOR_HUB_SET_CUST_REQ,
				custData) + sizeof(req.set_cust_req.getInfo);
			break;
		default:
			return -1;
		}
		break;
	case ID_PRESSURE:
		req.set_cust_req.sensorType = ID_PRESSURE;
		req.set_cust_req.action = SENSOR_HUB_SET_CUST;
		switch (action) {
		case CUST_ACTION_GET_SENSOR_INFO:
			req.set_cust_req.getInfo.action =
				CUST_ACTION_GET_SENSOR_INFO;
			len = offsetof(struct SCP_SENSOR_HUB_SET_CUST_REQ,
				custData) + sizeof(req.set_cust_req.getInfo);
			break;
		default:
			return -1;
		}
		break;
	case ID_GYROSCOPE:
		req.set_cust_req.sensorType = ID_GYROSCOPE;
		req.set_cust_req.action = SENSOR_HUB_SET_CUST;
		switch (action) {
		case CUST_ACTION_GET_SENSOR_INFO:
			req.set_cust_req.getInfo.action =
				CUST_ACTION_GET_SENSOR_INFO;
			len = offsetof(struct SCP_SENSOR_HUB_SET_CUST_REQ,
				custData) + sizeof(req.set_cust_req.getInfo);
			break;
		default:
			return -1;
		}
		break;
	case ID_MAGNETIC_FIELD:
		req.set_cust_req.sensorType = ID_MAGNETIC_FIELD;
		req.set_cust_req.action = SENSOR_HUB_SET_CUST;
		switch (action) {
		case CUST_ACTION_GET_SENSOR_INFO:
			req.set_cust_req.getInfo.action =
				CUST_ACTION_GET_SENSOR_INFO;
			len = offsetof(struct SCP_SENSOR_HUB_SET_CUST_REQ,
				custData) + sizeof(req.set_cust_req.getInfo);
			break;
		default:
			return -1;
		}
		break;

	default:
		return -1;
	}

	err = cix_nanohub_req_send(&req);
	if (err < 0) {
		pr_err("set_cust fail!\n");
		return -1;
	}
	if (sensor_id != req.get_data_rsp.sensorType
		|| SENSOR_HUB_SET_CUST != req.get_data_rsp.action
		|| 0 != req.get_data_rsp.errCode) {
		pr_err("error : %d\n", req.get_data_rsp.errCode);
		return req.get_data_rsp.errCode;
	}

	switch (action) {
	case CUST_ACTION_GET_SENSOR_INFO:
		if (req.set_cust_rsp.getInfo.action !=
			CUST_ACTION_GET_SENSOR_INFO) {
			pr_info("get_info failed!\n");
			return -1;
		}
		memcpy((struct sensorInfo_t *)data,
			&req.set_cust_rsp.getInfo.sensorInfo,
			sizeof(struct sensorInfo_t));
		break;
	default:
		break;
	}
	return err;
}

static void cix_nanohub_restoring_sensor(int sensor_id)
{
	uint8_t sensor_type = id_to_type(sensor_id);
	int ret = 0;
	int i = 0;
	struct ConfigCmd cmd;

	if (sensor_state[sensor_type].sensorType &&
			sensor_state[sensor_type].enable) {
		init_sensor_config_cmd(&cmd, sensor_type);
		pr_debug("restoring: [%d,%d,%d,%lld]\n",
			sensor_id, sensor_state[sensor_type].enable,
			sensor_state[sensor_type].rate,
			sensor_state[sensor_type].latency);
		ret = sensorhub_external_write((const uint8_t *)&cmd,
			sizeof(struct ConfigCmd));
		if (ret < 0)
			pr_notice("failed registerlistener [%d,%d]\n",
				sensor_id, cmd.cmd);

		cmd.cmd = CONFIG_CMD_FLUSH;
		mutex_lock(&flush_mtx);
		for (i = 0; i < sensor_state[sensor_type].flushcnt; i++) {
			ret = sensorhub_external_write((const uint8_t *)&cmd,
				sizeof(struct ConfigCmd));
			if (ret < 0)
				pr_notice("failed flush:%d\n", sensor_id);
		}
		mutex_unlock(&flush_mtx);
	}
}

static void cix_nanohub_get_devinfo(void)
{
	struct cix_nanohub_device *dev = cix_nanohub_dev;
	bool find_sensor = true;
	int id = 0, sensor = 0;
	struct sensorInfo_t hubinfo;
	struct sensor_info info;

	for (id = 0; id < ID_SENSOR_MAX; ++id) {
		sensor = id_to_type(id);
		if (sensorlist_sensor_to_handle(sensor) < 0)
			continue;
		memset(&hubinfo, 0, sizeof(hubinfo));
		memset(&info, 0, sizeof(info));
		info.sensor_type = sensor;
		if (cix_nanohub_set_cmd_to_hub(id,
				CUST_ACTION_GET_SENSOR_INFO, &hubinfo) < 0) {
			pr_err("type(%d) not registered\n", sensor);
			find_sensor = false;
		} else {
			find_sensor = true;
			strlcpy(info.name, hubinfo.name, sizeof(info.name));
			/* restore mag lib info */
			if (sensor == SENSOR_TYPE_MAGNETIC_FIELD) {
				strlcpy(info.vendor,
					hubinfo.mag_dev_info.libname,
					sizeof(info.vendor));
			}
		}
		if (unlikely(!atomic_read(&dev->create_manager_first_boot)))
			cix_nanohub_set_sensor_info(&info, find_sensor);
	}
}

static void cix_nanohub_restoring_config(void)
{
	int length = 0;
	struct cix_nanohub_device *device = cix_nanohub_dev;
	uint8_t *data = NULL;

	if (unlikely(!atomic_xchg(&device->cfg_data_after_reboot, 1)))
		return;

	pr_notice("restoring sensor config\n");

	length = sizeof(device->acc_config_data);
	data = vzalloc(length);
	if (data) {
		spin_lock(&config_data_lock);
		memcpy(data, device->acc_config_data, length);
		spin_unlock(&config_data_lock);
		cix_nanohub_cfg_to_hub(ID_ACCELEROMETER, data, length);
		vfree(data);
	}

	length = sizeof(device->gyro_config_data);
	data = vzalloc(length);
	if (data) {
		spin_lock(&config_data_lock);
		memcpy(data, device->gyro_config_data, length);
		spin_unlock(&config_data_lock);
		cix_nanohub_cfg_to_hub(ID_GYROSCOPE, data, length);
		vfree(data);
	}

	length = sizeof(device->mag_config_data);
	data = vzalloc(length);
	if (data) {
		spin_lock(&config_data_lock);
		memcpy(data, device->mag_config_data, length);
		spin_unlock(&config_data_lock);
		cix_nanohub_cfg_to_hub(ID_MAGNETIC_FIELD, data, length);
		vfree(data);
	}

	length = sizeof(device->light_config_data);
	data = vzalloc(length);
	if (data) {
		spin_lock(&config_data_lock);
		memcpy(data, device->light_config_data, length);
		spin_unlock(&config_data_lock);
		cix_nanohub_cfg_to_hub(ID_LIGHT, data, length);
		vfree(data);
	}

	length = sizeof(device->proximity_config_data);
	data = vzalloc(length);
	if (data) {
		spin_lock(&config_data_lock);
		memcpy(data, device->proximity_config_data, length);
		spin_unlock(&config_data_lock);
		cix_nanohub_cfg_to_hub(ID_PROXIMITY, data, length);
		vfree(data);
	}

	length = sizeof(device->pressure_config_data);
	data = vzalloc(length);
	if (data) {
		spin_lock(&config_data_lock);
		memcpy(data, device->pressure_config_data, length);
		spin_unlock(&config_data_lock);
		cix_nanohub_cfg_to_hub(ID_PRESSURE, data, length);
		vfree(data);
	}
}

void cix_nanohub_power_up_loop(void *data)
{
	int id = 0;
	struct cix_nanohub_device *device = cix_nanohub_dev;
	unsigned long flags = 0;

	wait_event(power_reset_wait,
		READ_ONCE(scp_system_ready) && READ_ONCE(scp_chre_ready));

	pr_info("SCP power up\n");
	spin_lock_irqsave(&scp_state_lock, flags);
	WRITE_ONCE(scp_chre_ready, false);
	WRITE_ONCE(scp_system_ready, false);
	spin_unlock_irqrestore(&scp_state_lock, flags);

	/* firstly we should update dram information */
	/* 1. reset wp queue head and tail */
	device->wp_queue.head = 0;
	device->wp_queue.tail = 0;

	/* 2. init dram information */
	WRITE_ONCE(device->scp_sensor_fifo,
		(struct sensor_fifo *)
		(long)scp_get_reserve_mem_virt(SENS_MEM_ID));
	BUG_ON(device->scp_sensor_fifo == NULL);
	WRITE_ONCE(device->scp_sensor_fifo->wp, 0);
	WRITE_ONCE(device->scp_sensor_fifo->rp, 0);
	WRITE_ONCE(device->scp_sensor_fifo->fifo_size,
		((long)scp_get_reserve_mem_size(SENS_MEM_ID) -
			offsetof(struct sensor_fifo, data)) /
				SENSOR_DATA_SIZE * SENSOR_DATA_SIZE);
	pr_debug("scp_sensor_fifo =%p, wp =%d, rp =%d, size =%d\n",
		READ_ONCE(device->scp_sensor_fifo),
		READ_ONCE(device->scp_sensor_fifo->wp),
		READ_ONCE(device->scp_sensor_fifo->rp),
		READ_ONCE(device->scp_sensor_fifo->fifo_size));
	/* 3. send dram information to scp */
	cix_nanohub_send_dram_info_to_hub();
	/* 4. get device info for mag lib and dynamic list */
	cix_nanohub_get_devinfo();
	/* 5. start timesync */
		// TBD 
	/* 6. we restore sensor calibration data when scp reboot */
	cix_nanohub_restoring_config();
	/* 7. we enable sensor which sensor is enable by framework */
	mutex_lock(&sensor_state_mtx);
	for (id = 0; id < ID_SENSOR_MAX; id++)
		cix_nanohub_restoring_sensor(id);
	mutex_unlock(&sensor_state_mtx);

	/* 8. create mamanger last */
	cix_nanohub_create_manager();
}

static int cix_nanohub_power_up_work(void *data)
{
	for (;;)
		cix_nanohub_power_up_loop(data);
	return 0;
}

static int send_sensor_init_start_event(void)
{
	enum scp_ipi_status ipi_status = SCP_IPI_ERROR;
	uint32_t sensor_init_start_event = 0;
	uint32_t retry = 0;

	do {
		ipi_status = sfh_ipi_send(IPI_SENSOR_INIT_START,
			&sensor_init_start_event,
			sizeof(sensor_init_start_event),
			0, SCP_A_ID);
		if (ipi_status == SCP_IPI_ERROR) {
			pr_err("IPI_SENSOR_INIT_START send fail\n");
			return -1;
		}
		if (ipi_status == SCP_IPI_BUSY) {
			if (retry++ == 1000) {
				pr_err("IPI_SENSOR_INIT_START retry fail\n");
				return -1;
			}
			if (retry % 10 == 0)
				usleep_range(1000, 2000);
		}
	} while (ipi_status == SCP_IPI_BUSY);

	return 0;
}

int cix_nanohub_ready_event(unsigned long event)
{
	unsigned long flags = 0;

	pr_info("notify event:%lu\n", event);
	if (event == SCP_EVENT_STOP) {
		spin_lock_irqsave(&scp_state_lock, flags);
		WRITE_ONCE(scp_system_ready, false);
		spin_unlock_irqrestore(&scp_state_lock, flags);
		atomic_set(&power_status, SENSOR_POWER_DOWN);
		//scp_power_monitor_notify(SENSOR_POWER_DOWN, ptr);
	}

	if (event == SCP_EVENT_READY) {
		if (send_sensor_init_start_event())
			return NOTIFY_BAD;

		spin_lock_irqsave(&scp_state_lock, flags);
		WRITE_ONCE(scp_system_ready, true);
		WRITE_ONCE(scp_chre_ready, true);
		if (READ_ONCE(scp_system_ready) && READ_ONCE(scp_chre_ready)) {
			spin_unlock_irqrestore(&scp_state_lock, flags);
			atomic_set(&power_status, SENSOR_POWER_UP);
			//scp_power_monitor_notify(SENSOR_POWER_UP, ptr);
			/* schedule_work(&device->power_up_work); */
			wake_up(&power_reset_wait);
		} else
			spin_unlock_irqrestore(&scp_state_lock, flags);
	}

	return NOTIFY_DONE;
}
EXPORT_SYMBOL(cix_nanohub_ready_event);

static int cix_nanohub_enable(struct hf_device *hfdev,
		int sensor_type, int en)
{
	if (sensor_type <= 0)
		return 0;
	/* pr_notice("%s [%d,%d]\n", __func__, sensor_type, en); */
	return cix_nanohub_enable_to_hub(type_to_id(sensor_type), en);
}

static int cix_nanohub_batch(struct hf_device *hfdev,
		int sensor_type, int64_t delay, int64_t latency)
{
	if (sensor_type <= 0)
		return 0;
	/* pr_notice("%s [%d,%lld,%lld]\n", __func__,
	 *	sensor_type, delay, latency);
	 */
	return cix_nanohub_batch_to_hub(type_to_id(sensor_type),
		0, delay, latency);
}

static int cix_nanohub_flush(struct hf_device *hfdev,
		int sensor_type)
{
	if (sensor_type <= 0)
		return 0;
	pr_notice("%s [%d]\n", __func__, sensor_type);
	return cix_nanohub_flush_to_hub(type_to_id(sensor_type));
}

static int cix_nanohub_calibration(struct hf_device *hfdev,
		int sensor_type)
{
	if (sensor_type <= 0)
		return 0;
	pr_notice("%s [%d]\n", __func__, sensor_type);
	return cix_nanohub_calibration_to_hub(type_to_id(sensor_type));
}

static int cix_nanohub_config(struct hf_device *hfdev,
		int sensor_type, void *data, uint8_t length)
{
	struct cix_nanohub_device *device = cix_nanohub_dev;

	if (sensor_type <= 0)
		return 0;

	pr_notice("%s [%d]\n", __func__, sensor_type);
	switch (type_to_id(sensor_type)) {
	case ID_ACCELEROMETER:
		if (sizeof(device->acc_config_data) < length)
			length = sizeof(device->acc_config_data);
		spin_lock(&config_data_lock);
		memcpy(device->acc_config_data, data, length);
		spin_unlock(&config_data_lock);
		break;
	case ID_GYROSCOPE:
		if (sizeof(device->gyro_config_data) < length)
			length = sizeof(device->gyro_config_data);
		spin_lock(&config_data_lock);
		memcpy(device->gyro_config_data, data, length);
		spin_unlock(&config_data_lock);
		break;
	case ID_MAGNETIC_FIELD:
		if (sizeof(device->mag_config_data) < length)
			length = sizeof(device->mag_config_data);
		spin_lock(&config_data_lock);
		memcpy(device->mag_config_data, data, length);
		spin_unlock(&config_data_lock);
		break;
	case ID_LIGHT:
		if (sizeof(device->light_config_data) < length)
			length = sizeof(device->light_config_data);
		spin_lock(&config_data_lock);
		memcpy(device->light_config_data, data, length);
		spin_unlock(&config_data_lock);
		break;
	case ID_PROXIMITY:
		if (sizeof(device->proximity_config_data) < length)
			length = sizeof(device->proximity_config_data);
		spin_lock(&config_data_lock);
		memcpy(device->proximity_config_data, data, length);
		spin_unlock(&config_data_lock);
		break;
	case ID_PRESSURE:
		if (sizeof(device->pressure_config_data) < length)
			length = sizeof(device->pressure_config_data);
		spin_lock(&config_data_lock);
		memcpy(device->pressure_config_data, data, length);
		spin_unlock(&config_data_lock);
		break;
	}
	if (!length) {
		pr_err("%s type(%d) length fail\n", __func__, sensor_type);
		return 0;
	}
	return cix_nanohub_cfg_to_hub(type_to_id(sensor_type),
		(uint8_t *)data, length);
}

static int cix_nanohub_selftest(struct hf_device *hfdev,
		int sensor_type)
{
	if (sensor_type <= 0)
		return 0;
	pr_notice("%s [%d]\n", __func__, sensor_type);
	return cix_nanohub_selftest_to_hub(type_to_id(sensor_type));
}

static int cix_nanohub_rawdata(struct hf_device *hfdev,
		int sensor_type, int en)
{
	if (sensor_type <= 0)
		return 0;
	pr_notice("%s [%d,%d]\n", __func__, sensor_type, en);
	return cix_nanohub_enable_rawdata_to_hub(type_to_id(sensor_type), en);
}

static int cix_nanohub_custom_cmd(struct hf_device *hfdev,
		int sensor_type, struct custom_cmd *cust_cmd)
{
	struct cix_nanohub_device *device = cix_nanohub_dev;
	int cust_action = cust_cmd->command;
	int ret = 0;

	/*
	 * User can use the cust_action to distinguish their own operations
	 * the default value(0) means the action to get sensors calibrated
	 * values.
	 */
	if (cust_action == CUST_CMD_CALI) {
		switch (sensor_type) {
		case SENSOR_TYPE_ACCELEROMETER:
			if (sizeof(cust_cmd->data) <
					sizeof(device->acc_config_data))
				return -EINVAL;
			cust_cmd->rx_len = sizeof(device->acc_config_data);
			spin_lock(&config_data_lock);
			memcpy(cust_cmd->data, device->acc_config_data,
					sizeof(device->acc_config_data));
			spin_unlock(&config_data_lock);
			break;
		case SENSOR_TYPE_GYROSCOPE:
			if (sizeof(cust_cmd->data) <
					sizeof(device->gyro_config_data))
				return -EINVAL;
			cust_cmd->rx_len = sizeof(device->gyro_config_data);
			spin_lock(&config_data_lock);
			memcpy(cust_cmd->data, device->gyro_config_data,
					sizeof(device->gyro_config_data));
			spin_unlock(&config_data_lock);
			break;
		case SENSOR_TYPE_MAGNETIC_FIELD:
			if (sizeof(cust_cmd->data) <
					sizeof(device->mag_config_data))
				return -EINVAL;
			cust_cmd->rx_len = sizeof(device->mag_config_data);
			spin_lock(&config_data_lock);
			memcpy(cust_cmd->data, device->mag_config_data,
					sizeof(device->mag_config_data));
			spin_unlock(&config_data_lock);
			break;
		case SENSOR_TYPE_LIGHT:
			if (sizeof(cust_cmd->data) <
					sizeof(device->light_config_data))
				return -EINVAL;
			cust_cmd->rx_len = sizeof(device->light_config_data);
			spin_lock(&config_data_lock);
			memcpy(cust_cmd->data, device->light_config_data,
					sizeof(device->light_config_data));
			spin_unlock(&config_data_lock);
			break;
		case SENSOR_TYPE_PROXIMITY:
			if (sizeof(cust_cmd->data) <
					sizeof(device->proximity_config_data))
				return -EINVAL;
			cust_cmd->rx_len =
				sizeof(device->proximity_config_data);
			spin_lock(&config_data_lock);
			memcpy(cust_cmd->data, device->proximity_config_data,
					sizeof(device->proximity_config_data));
			spin_unlock(&config_data_lock);
			break;
		case SENSOR_TYPE_PRESSURE:
			if (sizeof(cust_cmd->data) <
					sizeof(device->pressure_config_data))
				return -EINVAL;
			cust_cmd->rx_len =
				sizeof(device->pressure_config_data);
			spin_lock(&config_data_lock);
			memcpy(cust_cmd->data, device->pressure_config_data,
					sizeof(device->pressure_config_data));
			spin_unlock(&config_data_lock);
			break;
		default:
			pr_notice("SensorType:%d not support CUST_CMD_CALI!\n",
				sensor_type);
			break;
		}
	} else {
		pr_notice("CUSTOM_CMD(%d) need implementation\n", cust_action);
		ret = -1;
	}
	return ret;
}

static int cix_nanohub_report_to_manager(struct data_unit_t *data)
{
	struct hf_manager *manager = cix_nanohub_dev->hf_dev.manager;
	struct hf_manager_event event;

	if (!manager)
		return 0;

	memset(&event, 0, sizeof(struct hf_manager_event));
	if (data->flush_action == DATA_ACTION) {
		switch (data->sensor_type) {
		case ID_ACCELEROMETER:
			event.timestamp = data->time_stamp;
			event.sensor_type = id_to_type(data->sensor_type);
			event.accurancy = data->accelerometer_t.status;
			event.action = data->flush_action;
			event.word[0] = data->accelerometer_t.x;
			event.word[1] = data->accelerometer_t.y;
			event.word[2] = data->accelerometer_t.z;
			event.word[3] = data->accelerometer_t.x_bias;
			event.word[4] = data->accelerometer_t.y_bias;
			event.word[5] = data->accelerometer_t.z_bias;
			break;
		case ID_MAGNETIC_FIELD:
			event.timestamp = data->time_stamp;
			event.sensor_type = id_to_type(data->sensor_type);
			event.accurancy = data->magnetic_t.status;
			event.action = data->flush_action;
			event.word[0] = data->magnetic_t.x;
			event.word[1] = data->magnetic_t.y;
			event.word[2] = data->magnetic_t.z;
			event.word[3] = data->magnetic_t.x_bias;
			event.word[4] = data->magnetic_t.y_bias;
			event.word[5] = data->magnetic_t.z_bias;
			break;
		case ID_GYROSCOPE:
			event.timestamp = data->time_stamp;
			event.sensor_type = id_to_type(data->sensor_type);
			event.accurancy = data->gyroscope_t.status;
			event.action = data->flush_action;
			event.word[0] = data->gyroscope_t.x;
			event.word[1] = data->gyroscope_t.y;
			event.word[2] = data->gyroscope_t.z;
			event.word[3] = data->gyroscope_t.x_bias;
			event.word[4] = data->gyroscope_t.y_bias;
			event.word[5] = data->gyroscope_t.z_bias;
			break;
		case ID_LIGHT:
			event.timestamp = data->time_stamp;
			event.sensor_type = id_to_type(data->sensor_type);
			event.action = data->flush_action;
			event.word[0] = data->light;
			break;
		case ID_PROXIMITY:
			event.timestamp = data->time_stamp;
			event.sensor_type = id_to_type(data->sensor_type);
			event.action = data->flush_action;
			event.word[0] = data->proximity_t.oneshot;
			event.word[1] = data->proximity_t.steps;
			break;
		case ID_PRESSURE:
			event.timestamp = data->time_stamp;
			event.sensor_type = id_to_type(data->sensor_type);
			event.action = data->flush_action;
			event.accurancy = data->pressure_t.status;
			event.word[0] = data->pressure_t.pressure;
			break;
		default:
			event.timestamp = data->time_stamp;
			event.sensor_type = id_to_type(data->sensor_type);
			event.action = data->flush_action;
			event.word[0] = data->data[0];
			event.word[1] = data->data[1];
			event.word[2] = data->data[2];
			event.word[3] = data->data[3];
			event.word[4] = data->data[4];
			event.word[5] = data->data[5];
			break;
		}
	} else if (data->flush_action == FLUSH_ACTION) {
		event.timestamp = data->time_stamp;
		event.sensor_type = id_to_type(data->sensor_type);
		event.action = data->flush_action;
		event.word[0] = data->data[0];
		event.word[1] = data->data[1];
		event.word[2] = data->data[2];
		event.word[3] = data->data[3];
		event.word[4] = data->data[4];
		event.word[5] = data->data[5];
		pr_notice("%s [%d] flush complete\n",
					__func__, event.sensor_type);
	} else if (data->flush_action == BIAS_ACTION) {
		event.timestamp = data->time_stamp;
		event.sensor_type = id_to_type(data->sensor_type);
		event.action = data->flush_action;
		switch (data->sensor_type) {
		case ID_ACCELEROMETER:
			event.word[0] = data->accelerometer_t.x_bias;
			event.word[1] = data->accelerometer_t.y_bias;
			event.word[2] = data->accelerometer_t.z_bias;
			break;
		case ID_MAGNETIC_FIELD:
			event.word[0] = data->magnetic_t.x_bias;
			event.word[1] = data->magnetic_t.y_bias;
			event.word[2] = data->magnetic_t.z_bias;
			break;
		case ID_GYROSCOPE:
			event.word[0] = data->gyroscope_t.x_bias;
			event.word[1] = data->gyroscope_t.y_bias;
			event.word[2] = data->gyroscope_t.z_bias;
			break;
		}
	} else if (data->flush_action == CALI_ACTION) {
		event.timestamp = data->time_stamp;
		event.sensor_type = id_to_type(data->sensor_type);
		event.action = data->flush_action;
		switch (data->sensor_type) {
		case ID_ACCELEROMETER:
			event.timestamp = data->time_stamp;
			event.sensor_type = id_to_type(data->sensor_type);
			event.accurancy = data->accelerometer_t.status;
			event.action = data->flush_action;
			event.word[0] = data->accelerometer_t.x_bias;
			event.word[1] = data->accelerometer_t.y_bias;
			event.word[2] = data->accelerometer_t.z_bias;
			break;
		case ID_GYROSCOPE:
			event.timestamp = data->time_stamp;
			event.sensor_type = id_to_type(data->sensor_type);
			event.accurancy = data->gyroscope_t.status;
			event.action = data->flush_action;
			event.word[0] = data->gyroscope_t.x_bias;
			event.word[1] = data->gyroscope_t.y_bias;
			event.word[2] = data->gyroscope_t.z_bias;
			break;
		case ID_MAGNETIC_FIELD:
			event.timestamp = data->time_stamp;
			event.sensor_type = id_to_type(data->sensor_type);
			event.accurancy = data->magnetic_t.status;
			event.action = data->flush_action;
			event.word[0] = data->data[0];
			event.word[1] = data->data[1];
			event.word[2] = data->data[2];
			event.word[3] = data->data[3];
			event.word[4] = data->data[4];
			event.word[5] = data->data[5];
			break;
		case ID_PROXIMITY:
			event.timestamp = data->time_stamp;
			event.sensor_type = id_to_type(data->sensor_type);
			event.action = data->flush_action;
			event.word[0] = data->data[0];
			event.word[1] = data->data[1];
			break;
		case ID_LIGHT:
			event.timestamp = data->time_stamp;
			event.sensor_type = id_to_type(data->sensor_type);
			event.action = data->flush_action;
			event.word[0] = data->data[0];
			break;
		case ID_PRESSURE:
			event.timestamp = data->time_stamp;
			event.sensor_type = id_to_type(data->sensor_type);
			event.action = data->flush_action;
			event.word[0] = data->data[0];
			event.word[1] = data->data[1];
			break;
		}
	} else if (data->flush_action == TEMP_ACTION) {
		event.timestamp = data->time_stamp;
		event.sensor_type = id_to_type(data->sensor_type);
		event.action = data->flush_action;
		switch (data->sensor_type) {
		case ID_GYROSCOPE:
			event.timestamp = data->time_stamp;
			event.sensor_type = id_to_type(data->sensor_type);
			event.accurancy = data->gyroscope_t.status;
			event.action = data->flush_action;
			event.word[0] = data->data[0];
			event.word[1] = data->data[1];
			event.word[2] = data->data[2];
			event.word[3] = data->data[3];
			event.word[4] = data->data[4];
			event.word[5] = data->data[5];
			break;
		}
	} else if (data->flush_action == TEST_ACTION) {
		event.timestamp = data->time_stamp;
		event.sensor_type = id_to_type(data->sensor_type);
		event.action = data->flush_action;
		switch (data->sensor_type) {
		case ID_ACCELEROMETER:
			event.timestamp = data->time_stamp;
			event.sensor_type = id_to_type(data->sensor_type);
			event.action = data->flush_action;
			event.word[0] = data->accelerometer_t.status;
			break;
		case ID_MAGNETIC_FIELD:
			event.timestamp = data->time_stamp;
			event.sensor_type = id_to_type(data->sensor_type);
			event.action = data->flush_action;
			event.word[0] = data->magnetic_t.status;
			break;
		case ID_GYROSCOPE:
			event.timestamp = data->time_stamp;
			event.sensor_type = id_to_type(data->sensor_type);
			event.action = data->flush_action;
			event.word[0] = data->gyroscope_t.status;
			break;
		}
	} else {
		event.timestamp = data->time_stamp;
		event.sensor_type = id_to_type(data->sensor_type);
		event.action = data->flush_action;
		event.word[0] = data->data[0];
		event.word[1] = data->data[1];
		event.word[2] = data->data[2];
		event.word[3] = data->data[3];
		event.word[4] = data->data[4];
		event.word[5] = data->data[5];
	}

	return manager->report(manager, &event);
}

static int cix_nanohub_create_manager(void)
{
	int err = 0;
	struct cix_nanohub_device *device = cix_nanohub_dev;

	if (likely(atomic_xchg(&device->create_manager_first_boot, 1)))
		return 0;

	/* must update support_sensors firstly */
	cix_nanohub_get_sensor_info();
	/* refill support_list and support_size then create manager */
	device->hf_dev.support_list = support_sensors;
	device->hf_dev.support_size = support_size;
	err = hf_manager_create(&device->hf_dev);
	if (err < 0) {
		pr_err("%s hf_manager_create fail\n", __func__);
		return err;
	}
	pr_info("%s hf_manager_create success\n", __func__);
	return err;
}

static int cix_nanohub_probe(struct platform_device *pdev)
{
	int err = 0;
	struct cix_nanohub_device *device;
	struct task_struct *task = NULL, *task_power_reset = NULL;
	//struct sched_param param = { .sched_priority = MAX_RT_PRIO - 1 };

	cix_nanohub_init_sensor_info();
	device = kzalloc(sizeof(*device), GFP_KERNEL);
	if (!device) {
		err = -ENOMEM;
		goto exit;
	}
	device->hf_dev.dev_name = "cix_nanohub";
	device->hf_dev.device_poll = HF_DEVICE_IO_INTERRUPT;
	device->hf_dev.device_bus = HF_DEVICE_IO_ASYNC;
	device->hf_dev.support_list = support_sensors;
	device->hf_dev.support_size = support_size;
	device->hf_dev.enable = cix_nanohub_enable;
	device->hf_dev.batch = cix_nanohub_batch;
	device->hf_dev.flush = cix_nanohub_flush;
	device->hf_dev.calibration = cix_nanohub_calibration;
	device->hf_dev.config_cali = cix_nanohub_config;
	device->hf_dev.selftest = cix_nanohub_selftest;
	device->hf_dev.rawdata = cix_nanohub_rawdata;
	device->hf_dev.custom_cmd = cix_nanohub_custom_cmd;
	err = hf_device_register(&device->hf_dev);
	if (err < 0) {
		pr_err("register hf device fail!\n");
		goto exit_kfree;
	}
	cix_nanohub_dev = device;

	/* init sensor share dram write pointer event queue */
	spin_lock_init(&device->wp_queue.buffer_lock);
	device->wp_queue.head = 0;
	device->wp_queue.tail = 0;
	device->wp_queue.bufsize = 32;
	device->wp_queue.ringbuffer =
		vzalloc(device->wp_queue.bufsize * sizeof(uint32_t));
	if (!device->wp_queue.ringbuffer) {
		err = -ENOMEM;
		goto exit_device;
	}

	/* init scp boot flags */
	atomic_set(&device->cfg_data_after_reboot, 0);
	atomic_set(&device->create_manager_first_boot, 0);

	/* init nanohub ipi */
	err = cix_nanohub_ipi_init();
	if (err)
		goto exit_wakeup_source_unreg_2;

	/* register ipi interrupt handler */
	err = sfh_ipi_registration(IPI_SENSOR, cix_nanohub_ipi_handler,
				   "cix_nanohub");
	if (err != SCP_IPI_DONE) {
		pr_err("IPI_SENSOR register fail, err:%d\n", err);
		goto exit_wakeup_source_unreg_2;
	}

	/* init data path */
	WRITE_ONCE(chre_kthread_wait_condition, false);
	task = kthread_run(cix_nanohub_direct_push_work,
			   NULL, "chre_kthread");
	if (IS_ERR(task)) {
		err = -ENOMEM;
		pr_err("cix_nanohub_direct_push_work create fail!\n");
		goto exit_scp_ipi_reg;
	}
	//sched_setscheduler(task, SCHED_FIFO, &param);
	sched_set_fifo(task);

	/* this call back can get scp power UP status */
	task_power_reset = kthread_run(cix_nanohub_power_up_work,
				       NULL, "scp_power_reset");
	if (IS_ERR(task_power_reset)) {
		err = -ENOMEM;
		pr_err("cix_nanohub_power_up_work create fail!\n");
		goto exit_scp_ipi_reg;
	}

	pr_info("init done, data_unit_t:%d, SCP_SENSOR_HUB_DATA:%d\n",
		(int)sizeof(struct data_unit_t),
		(int)sizeof(union SCP_SENSOR_HUB_DATA));
	return 0;

exit_scp_ipi_reg:
	sfh_ipi_unregistration(IPI_SENSOR);
exit_wakeup_source_unreg_2:
	vfree(device->wp_queue.ringbuffer);
exit_device:
	hf_device_unregister(&device->hf_dev);
exit_kfree:
	kfree(device);
exit:
	pr_err("%s fail.err = %d\n", __func__, err);
	return err;
}

static int cix_nanohub_remove(struct platform_device *pdev)
{
	struct cix_nanohub_device *device = cix_nanohub_dev;

	del_timer_sync(&device->sync_time_timer);
	hf_manager_destroy(device->hf_dev.manager);
	sfh_ipi_unregistration(IPI_SENSOR);
	vfree(device->wp_queue.ringbuffer);
	hf_device_unregister(&device->hf_dev);
	kfree(device);
	return 0;
}

static void cix_nanohub_shutdown(struct platform_device *pdev)
{
	int id = 0;
	uint8_t sensor_type;
	struct ConfigCmd cmd;
	int ret = 0;

	mutex_lock(&sensor_state_mtx);
	for (id = 0; id < ID_SENSOR_MAX; id++) {
		sensor_type = id_to_type(id);
		if (sensor_state[sensor_type].sensorType &&
				sensor_state[sensor_type].enable) {
			sensor_state[sensor_type].enable = false;
			init_sensor_config_cmd(&cmd, sensor_type);

			ret = sensorhub_external_write((const uint8_t *)&cmd,
				sizeof(struct ConfigCmd));
			if (ret < 0)
				pr_warn("fail to set sensor:%d %d in shutdwn\n",
					id, cmd.cmd);
		}
	}
	mutex_unlock(&sensor_state_mtx);
}

static struct platform_device cix_nanohub_pdev = {
	.name = "cix_nanohub",
	.id = -1,
};

static struct platform_driver cix_nanohub_pdrv = {
	.driver = {
	   .name = "cix_nanohub",
	},
	.probe = cix_nanohub_probe,
	.remove = cix_nanohub_remove,
	.shutdown = cix_nanohub_shutdown,
};

static int __init cix_nanohub_init(void)
{
	int ret = 0;

	ret = platform_device_register(&cix_nanohub_pdev);
	if (ret) {
		pr_err("platform dev reg fail,ret:%d\n", ret);
		goto err_exit;
	}

	ret = platform_driver_register(&cix_nanohub_pdrv);
	if (ret) {
		pr_err("platform drv reg fail,ret:%d\n", ret);
		goto err_unregister_device;
	}

	return 0;

err_unregister_device:
	platform_device_unregister(&cix_nanohub_pdev);
err_exit:
	return ret;
}

static void __exit cix_nanohub_exit(void)
{
	platform_driver_unregister(&cix_nanohub_pdrv);
	platform_device_unregister(&cix_nanohub_pdev);
}

module_init(cix_nanohub_init);
module_exit(cix_nanohub_exit);

MODULE_DESCRIPTION("cix sensorhub kernel driver");
MODULE_LICENSE("GPL");