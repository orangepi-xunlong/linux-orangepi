// SPDX-License-Identifier: GPL-2.0
#include <linux/aw_rpmsg.h>

#include <linux/amp_timestamp.h>

void *g_rpmsg_amp_ts_dev;
EXPORT_SYMBOL(g_rpmsg_amp_ts_dev);

uint64_t rpmsg_perf_get_timestamp(void)
{
	int ret = 0;
	uint64_t ts = 0;

	if (!g_rpmsg_amp_ts_dev)
		return 0;

	ret = amp_ts_get_timestamp(g_rpmsg_amp_ts_dev, &ts);
	if (ret) {
		pr_err("amp_ts_get_timestamp failed, ret: %d", ret);
	}

	return ts;
}
EXPORT_SYMBOL(rpmsg_perf_get_timestamp);

int rpmsg_get_raw_perf_data(struct rpmsg_endpoint *ept, uint8_t *buf, uint32_t len)
{
	uint32_t target_len;
	struct aw_rpmsg_endpoint *aw_ept;

	if (!ept || !buf || !len) {
		pr_err("invalid arguments\n");
		return -EINVAL;
	}

	target_len = (RPMSG_TRACE_TIMESTAMP_NUM) * sizeof(uint64_t);
	if (len != target_len) {
		pr_err("perf data buf length(%u) is not equal to %u", len, target_len);
		return -EINVAL;
	}

	aw_ept = EPT_TO_AW_EPT(ept);
	memcpy(buf, aw_ept->trace, len);

	return 0;
}
EXPORT_SYMBOL(rpmsg_get_raw_perf_data);

int rpmsg_get_perf_data(struct rpmsg_endpoint *ept, rpmsg_perf_data_t *perf_data)
{
	int ret, i, j;
	uint64_t buf[RPMSG_TRACE_TIMESTAMP_NUM];

	if (((sizeof(perf_data->time_span) / sizeof(uint32_t)) != (RPMSG_TRACE_TIMESTAMP_NUM - 1))
		|| (sizeof(perf_data->time_span) != sizeof(perf_data->timestamp))) {
		pr_err("rpbuf perf data definition is not correct!\n");
		return -EINVAL;
	}

	ret = rpmsg_get_raw_perf_data(ept, (uint8_t *)buf, sizeof(buf));
	if (ret)
		return ret;

	perf_data->raw_timestamp = buf[RPMSG_TRACE_START];

	memset(perf_data->timestamp, 0, sizeof(perf_data->timestamp));
	for (i = RPMSG_TRACE_START + 1, j = 0; i <= RPMSG_TRACE_END; i++, j++) {
		perf_data->timestamp[j] = buf[i] - buf[RPMSG_TRACE_START];
	}

	memset(perf_data->time_span, 0, sizeof(perf_data->time_span));
	for (i = RPMSG_TRACE_START + 1, j = 0; i <= RPMSG_TRACE_END; i++, j++) {
		if (i == RPMSG_TRACE_R_PREPARE) {
			perf_data->time_span[j] = buf[i] - buf[RPMSG_TRACE_S_NOTIFY];
		} else {
			perf_data->time_span[j] = buf[i] - buf[i - 1];
		}
	}

	return 0;
}
EXPORT_SYMBOL(rpmsg_get_perf_data);

void rpmsg_dump_perf_data(const rpmsg_perf_data_t *perf_data)
{
	uint64_t quotient;
	uint32_t ts, time_span, remainder;

	quotient = div_u64_rem(perf_data->raw_timestamp, 1000, &remainder);
	pr_info("Timestamp:\n");
	pr_info("Sender  >>\n");
	pr_info("Start         : 0 us [%llu.%llu ms]\n", quotient, (uint64_t)remainder);

	ts = perf_data->timestamp[RPMSG_PERF_POINT_SENDER_FILL];
	pr_info("Fill buffer   : %u us\n", ts);

	ts = perf_data->timestamp[RPMSG_PERF_POINT_SENDER_NOTIFY];
	pr_info("Notify        : %u us\n", ts);

	ts = perf_data->timestamp[RPMSG_PERF_POINT_SENDER_END];
	pr_info("End           : %u us\n",  ts);

	pr_info("Receiver<<\n");
	ts = perf_data->timestamp[RPMSG_PERF_POINT_RECEIVER_PREPARE];
	pr_info("Prepare       : %u us\n", ts);

	ts = perf_data->timestamp[RPMSG_PERF_POINT_RECEIVER_EXEC_CB];
	pr_info("Exec CB       : %u us\n", ts);

	ts = perf_data->timestamp[RPMSG_PERF_POINT_RECEIVER_END];
	pr_info("End           : %u us\n", ts);


	pr_info("Time span:\n");

	pr_info("Sender  >>\n");
	time_span = perf_data->time_span[RPMSG_PERF_STAGE_SENDER_PREPARE];
	pr_info("Prepare       : %u us\n", time_span);

	time_span = perf_data->time_span[RPMSG_PERF_STAGE_SENDER_FILL];
	pr_info("Fill buffer   : %u us\n", time_span);

	time_span = perf_data->time_span[RPMSG_PERF_STAGE_SENDER_NOTIFY];
	pr_info("Notify        : %u us\n", time_span);

	pr_info("Receiver<<\n");
	time_span = perf_data->time_span[RPMSG_PERF_STAGE_RECEIVER_RECV];
	pr_info("Recv          : %u us\n", time_span);

	time_span = perf_data->time_span[RPMSG_PERF_STAGE_RECEIVER_PREPARE];
	pr_info("Prepare       : %u us\n", time_span);

	time_span = perf_data->time_span[RPMSG_PERF_STAGE_RECEIVER_EXEC_CB];
	pr_info("Exec CB       : %u us\n", time_span);
}
EXPORT_SYMBOL(rpmsg_dump_perf_data);
