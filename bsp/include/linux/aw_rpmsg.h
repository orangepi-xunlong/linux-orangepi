/* SPDX-License-Identifier: GPL-2.0 */

#ifndef _LINUX_AW_RPMSG_H
#define _LINUX_AW_RPMSG_H

#include <linux/rpmsg.h>

#ifdef CONFIG_AW_RPMSG_NOTIFY
typedef int (*notify_callback)(void *dev, void *data, int len);
int rpmsg_notify_add(const char *ser_name, const char *name, notify_callback cb, void *dev);
int rpmsg_notify_del(const char *ser_name, const char *name);
#endif

#ifdef CONFIG_AW_RPROC_FAST_BOOT
#ifdef CONFIG_AW_IPC_FASTBOOT
#define fast_rpmsg_initcall(__rpmsg_driver) core_initcall(__rpmsg_driver)
#else
#define fast_rpmsg_initcall(__rpmsg_driver) postcore_initcall(__rpmsg_driver)
#endif

#define fast_rpmsg_driver(__rpmsg_driver) \
static int __init __rpmsg_driver##_init(void) \
{ \
	return register_rpmsg_driver(&(__rpmsg_driver)); \
} \
fast_rpmsg_initcall(__rpmsg_driver##_init); \
static void __exit __rpmsg_driver##_exit(void) \
{ \
	unregister_rpmsg_driver(&(__rpmsg_driver)); \
} \
module_exit(__rpmsg_driver##_exit);
#endif

/* Rpmsg performance trace point */
#define RPMSG_TRACE_S_START              (0)			/* sender start */
#define RPMSG_TRACE_S_FILL               (1)			/* sender fill data */
#define RPMSG_TRACE_S_NOTIFY             (2)			/* sender notify to remote */
#define RPMSG_TRACE_S_END                (3)			/* sender end */
#define RPMSG_TRACE_R_PREPARE            (4)			/* receiver prepare to handle rpmsg package */
#define RPMSG_TRACE_R_CB                 (5)			/* receiver execute cb */
#define RPMSG_TRACE_R_END                (6)			/* receiver end */

#define RPMSG_TRACE_START                (RPMSG_TRACE_S_START)
#define RPMSG_TRACE_END                  (RPMSG_TRACE_R_END)

#define RPMSG_TRACE_TIMESTAMP_NUM        (RPMSG_TRACE_R_END + 1)

#define RPMSG_TRACE_SENDER_INFO_START    (RPMSG_TRACE_S_START)
#define RPMSG_TRACE_SENDER_INFO_END      (RPMSG_TRACE_S_END)
#define RPMSG_TRACE_SENDER_INFO_SIZE     (RPMSG_TRACE_SENDER_INFO_END - \
												RPMSG_TRACE_SENDER_INFO_START + 1)


#define RPMSG_PERF_POINT_SENDER_FILL 0
#define RPMSG_PERF_POINT_SENDER_NOTIFY 1
#define RPMSG_PERF_POINT_SENDER_END 2
#define RPMSG_PERF_POINT_RECEIVER_PREPARE 3
#define RPMSG_PERF_POINT_RECEIVER_EXEC_CB 4
#define RPMSG_PERF_POINT_RECEIVER_END 5

#define RPMSG_PERF_STAGE_SENDER_PREPARE 0
#define RPMSG_PERF_STAGE_SENDER_FILL 1
#define RPMSG_PERF_STAGE_SENDER_NOTIFY 2
#define RPMSG_PERF_STAGE_RECEIVER_RECV 3 /* sender notify to receiver prepare */
#define RPMSG_PERF_STAGE_RECEIVER_PREPARE 4
#define RPMSG_PERF_STAGE_RECEIVER_EXEC_CB 5

#define RPMSG_PERF_STAGE_NUM (RPMSG_PERF_STAGE_RECEIVER_EXEC_CB + 1)
#define RPMSG_PERF_POINT_NUM RPMSG_PERF_STAGE_NUM

typedef struct rpmsg_perf_data {
	uint32_t timestamp[RPMSG_PERF_POINT_NUM];
	uint32_t time_span[RPMSG_PERF_STAGE_NUM];
	uint64_t raw_timestamp;
} rpmsg_perf_data_t;

#ifdef CONFIG_AW_RPMSG_PERF_TRACE
struct aw_rpmsg_endpoint {
	struct rpmsg_endpoint ept;
	uint64_t trace[RPMSG_TRACE_TIMESTAMP_NUM];
};

#define EPT_TO_AW_EPT(x) container_of(x, struct aw_rpmsg_endpoint, ept)

#define RPMSG_DELIVERY_PERF_DATA_MAGIC 0x46524550 /* PERF */

typedef struct rpmsg_delivery_perf_data {
	uint32_t magic;
	uint32_t data_len;
	rpmsg_perf_data_t data;
} rpmsg_delivery_perf_data_t;

extern void *g_rpmsg_amp_ts_dev;

uint64_t rpmsg_perf_get_timestamp(void);

int rpmsg_get_raw_perf_data(struct rpmsg_endpoint *ept, uint8_t *buf, uint32_t len);
int rpmsg_get_perf_data(struct rpmsg_endpoint *ept, rpmsg_perf_data_t *perf_data);
void rpmsg_dump_perf_data(const rpmsg_perf_data_t *perf_data);

void rpmsg_record_receiver_end_ts(struct rpmsg_endpoint *ept);
#else
#define EPT_TO_AW_EPT(x) (x)

static inline uint64_t rpmsg_perf_get_timestamp(void)
{
	return 0;
}

static inline int rpmsg_get_raw_perf_data(struct rpmsg_endpoint *ept, uint8_t *buf, uint32_t len)
{
	return -1;
}
static inline int rpmsg_get_perf_data(struct rpmsg_endpoint *ept, rpmsg_perf_data_t *perf_data)
{
	return -1;
}
static inline void rpmsg_dump_perf_data(const rpmsg_perf_data_t *perf_data) { }

static inline void rpmsg_record_receiver_end_ts(struct rpmsg_endpoint *ept) { }
#endif /* CONFIG_AW_RPMSG_PERF_TRACE */

#endif /* _LINUX_AW_RPMSG_H */
