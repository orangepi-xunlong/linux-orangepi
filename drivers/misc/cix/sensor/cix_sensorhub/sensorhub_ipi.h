// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2020 MediaTek Inc.
 *
 * Copyright 2024 Cix Technology Group Co., Ltd.
 */

#ifndef _CIX_NANOHUB_IPI_H_
#define _CIX_NANOHUB_IPI_H_

#include <linux/list.h>
#include <linux/notifier.h>
#include "../scp/include/scp.h"

/* scp feature ID list */
enum feature_id {
	SENS_FEATURE_ID = 1,
	NUM_FEATURE_ID,
};

struct ipi_transfer {
	const unsigned char *tx_buf;
	unsigned char *rx_buf;
	unsigned int tx_len;
	unsigned int rx_len;
	struct list_head transfer_list;
};

struct ipi_message {
	struct list_head transfers;
	struct list_head list;
	void *context;
	int status;
	void (*complete)(void *context);
};

static inline void ipi_message_init(struct ipi_message *m)
{
	memset(m, 0, sizeof(*m));
	INIT_LIST_HEAD(&m->transfers);
}

static inline void ipi_message_add_tail(struct ipi_transfer *t, struct ipi_message *m)
{
	list_add_tail(&t->transfer_list, &m->transfers);
}

int cix_nanohub_ipi_sync(unsigned char *buffer, unsigned int len);
int cix_nanohub_ipi_async(struct ipi_message *m);
void cix_nanohub_ipi_complete(unsigned char *buffer, unsigned int len);
int cix_nanohub_ipi_init(void);

enum scp_ipi_status sfh_ipi_send(enum ipi_id id, void *buf,
	unsigned int len, unsigned int wait, enum scp_core_id scp_id);
enum scp_ipi_status is_sfh_ready(enum scp_core_id scp_id);

int sensorhub_external_write(const char *buffer, int length);
void sfh_register_feature(enum feature_id id);

enum scp_ipi_status sfh_ipi_registration(enum ipi_id id,
	void (*ipi_handler)(int id, void *data, unsigned int len),
	const char *name);
enum scp_ipi_status sfh_ipi_unregistration(enum ipi_id id);

void sfh_A_register_notify(struct notifier_block *nb);
void sfh_A_unregister_notify(struct notifier_block *nb);

#endif
