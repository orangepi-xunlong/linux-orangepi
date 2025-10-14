// SPDX-License-Identifier: GPL-2.0+
/*
 * Header file for the DSP IPC implementation
 *
 * Copyright 2024 Cix Technology Group Co., Ltd.
 */

#ifndef _CIX_DSP_IPC_H_
#define _CIX_DSP_IPC_H_

#include <linux/device.h>
#include <linux/types.h>
#include <linux/mailbox_client.h>

#define CIX_DSP_IPC_REQ		0
#define CIX_DSP_IPC_REP		1
#define CIX_DSP_IPC_OP_REQ	BIT(0)
#define CIX_DSP_IPC_OP_REP	BIT(1)

enum {
	CIX_DSP_MBOX_REPLY,
	CIX_DSP_MBOX_REQUEST,
	CIX_DSP_MBOX_NUM,
};

struct cix_dsp_ipc;

struct cix_dsp_ops {
	void (*handle_reply)(struct cix_dsp_ipc *dsp_ipc);
	void (*handle_request)(struct cix_dsp_ipc *dsp_ipc);
};

struct cix_dsp_chan {
	struct cix_dsp_ipc *ipc;
	struct mbox_client cl;
	struct mbox_chan *ch;
	int idx;
};

struct cix_dsp_ipc {
	struct device *dev;
	struct cix_dsp_ops *ops;
	struct cix_dsp_chan chans[CIX_DSP_MBOX_NUM];
	void *private_data;
};

static inline void cix_dsp_set_data(struct cix_dsp_ipc *dsp_ipc, void *data)
{
	if (!dsp_ipc)
		return;

	dsp_ipc->private_data = data;
}

static inline void *cix_dsp_get_data(struct cix_dsp_ipc *dsp_ipc)
{
	if (!dsp_ipc)
		return NULL;

	return dsp_ipc->private_data;
}

#if IS_ENABLED(CONFIG_CIX_DSP)

int cix_dsp_ipc_send(struct cix_dsp_ipc *ipc, unsigned int idx, uint32_t op);
int cix_dsp_request_mbox(struct cix_dsp_ipc *dsp_ipc);
void cix_dsp_free_mbox(struct cix_dsp_ipc *dsp_ipc);

#else

int cix_dsp_ipc_send(struct cix_dsp_ipc *ipc, unsigned int idx, uint32_t op)
{
	return -ENOTSUPP;
}

int cix_dsp_request_mbox(struct cix_dsp_ipc *dsp_ipc)
{
	return -ENOTSUPP;
}

void cix_dsp_free_mbox(struct cix_dsp_ipc *dsp_ipc) {};

#endif

#endif /* _CIX_DSP_IPC_H */
