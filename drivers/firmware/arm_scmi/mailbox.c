// SPDX-License-Identifier: GPL-2.0
/*
 * System Control and Management Interface (SCMI) Message Mailbox Transport
 * driver.
 *
 * Copyright (C) 2019 ARM Ltd.
 */

#include <linux/err.h>
#include <linux/device.h>
#include <linux/mailbox_client.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/slab.h>
#include <linux/acpi.h>
#include <linux/platform_device.h>

#include "common.h"

/**
 * struct scmi_mailbox - Structure representing a SCMI mailbox transport
 *
 * @cl: Mailbox Client
 * @chan: Transmit/Receive mailbox uni/bi-directional channel
 * @chan_receiver: Optional Receiver mailbox unidirectional channel
 * @cinfo: SCMI channel info
 * @shmem: Transmit/Receive shared memory area
 * @chan_lock: Lock that prevents multiple xfers from being queued
 */
struct scmi_mailbox {
	struct mbox_client cl;
	struct mbox_chan *chan;
	struct mbox_chan *chan_receiver;
	struct scmi_chan_info *cinfo;
	struct scmi_shared_mem __iomem *shmem;
	struct mutex chan_lock;
};

#define client_to_scmi_mailbox(c) container_of(c, struct scmi_mailbox, cl)

static void tx_prepare(struct mbox_client *cl, void *m)
{
	struct scmi_mailbox *smbox = client_to_scmi_mailbox(cl);

	shmem_tx_prepare(smbox->shmem, m, smbox->cinfo);
}

static void rx_callback(struct mbox_client *cl, void *m)
{
	struct scmi_mailbox *smbox = client_to_scmi_mailbox(cl);

	/*
	 * An A2P IRQ is NOT valid when received while the platform still has
	 * the ownership of the channel, because the platform at first releases
	 * the SMT channel and then sends the completion interrupt.
	 *
	 * This addresses a possible race condition in which a spurious IRQ from
	 * a previous timed-out reply which arrived late could be wrongly
	 * associated with the next pending transaction.
	 */
	if (cl->knows_txdone && !shmem_channel_free(smbox->shmem)) {
		dev_warn(smbox->cinfo->dev, "Ignoring spurious A2P IRQ !\n");
		return;
	}

	scmi_rx_callback(smbox->cinfo, shmem_read_header(smbox->shmem), NULL);
}

static __maybe_unused void tx_done_callback(struct mbox_client *cl, void *m, int r)
{
	struct scmi_mailbox *smbox = client_to_scmi_mailbox(cl);

	scmi_rx_callback(smbox->cinfo, shmem_read_header(smbox->shmem), NULL);
}

static bool mailbox_chan_available(struct fwnode_handle *fwnode, int idx)
{
	int num_mb;
	struct fwnode_reference_args fwnode_args;

	/*
	 * Just check if bidirrectional channels are involved, and check the
	 * index accordingly; proper full validation will be made later
	 * in mailbox_chan_setup().
	 */
	num_mb = fwnode_count_reference_with_args(fwnode, "mboxes", "#mbox-cells");
	if (num_mb == 3 && idx == 1)
		idx = 2;

	return !fwnode_property_get_reference_args(fwnode, "mboxes", "#mbox-cells",
										1, idx, &fwnode_args);
}

/**
 * mailbox_chan_validate  - Validate transport configuration and map channels
 *
 * @cdev: Reference to the underlying transport device carrying the
 *	  of_node descriptor to analyze.
 * @a2p_rx_chan: A reference to an optional unidirectional channel to use
 *		 for replies on the a2p channel. Set as zero if not present.
 * @p2a_chan: A reference to the optional p2a channel.
 *	      Set as zero if not present.
 *
 * At first, validate the transport configuration as described in terms of
 * 'mboxes' and 'shmem', then determin which mailbox channel indexes are
 * appropriate to be use in the current configuration.
 *
 * Return: 0 on Success or error
 */
static int mailbox_chan_validate(struct device *cdev,
				 int *a2p_rx_chan, int *p2a_chan)
{
	int num_mb, num_sh, ret = 0;
	struct fwnode_handle *fwnode = cdev->fwnode;

	num_mb = fwnode_count_reference_with_args(fwnode, "mboxes", "#mbox-cells");
	num_sh = fwnode_count_reference_with_args(fwnode, "shmem", NULL);
	dev_dbg(cdev, "Found %d mboxes and %d shmems !\n", num_mb, num_sh);

	/* Bail out if mboxes and shmem descriptors are inconsistent */
	if (num_mb <= 0 || num_sh <= 0 || num_sh > 2 || num_mb > 3 ||
	    (num_mb == 1 && num_sh != 1) || (num_mb == 3 && num_sh != 2)) {
		dev_warn(cdev,
			 "Invalid channel descriptor for '%s' - mbs:%d  shm:%d\n",
			 fwnode_get_name(fwnode), num_mb, num_sh);
		return -EINVAL;
	}

	/* Bail out if provided shmem descriptors do not refer distinct areas  */
	if (num_sh > 1) {
		struct fwnode_handle *np_tx, *np_rx;

		np_tx = fwnode_find_reference(fwnode, "shmem", 0);
		np_rx = fwnode_find_reference(fwnode, "shmem", 1);
		if (!np_tx || !np_rx || np_tx == np_rx) {
			dev_warn(cdev, "Invalid shmem descriptor for '%s'\n",
				 fwnode_get_name(fwnode));
			ret = -EINVAL;
		}

		fwnode_handle_put(np_tx);
		fwnode_handle_put(np_rx);
	}

	/* Calculate channels IDs to use depending on mboxes/shmem layout */
	if (!ret) {
		switch (num_mb) {
		case 1:
			*a2p_rx_chan = 0;
			*p2a_chan = 0;
			break;
		case 2:
			if (num_sh == 2) {
				*a2p_rx_chan = 0;
				*p2a_chan = 1;
			} else {
				*a2p_rx_chan = 1;
				*p2a_chan = 0;
			}
			break;
		case 3:
			*a2p_rx_chan = 1;
			*p2a_chan = 2;
			break;
		}
	}

	return ret;
}

static int mailbox_chan_setup(struct scmi_chan_info *cinfo, struct device *dev,
			      bool tx)
{
	const char *desc = tx ? "Tx" : "Rx";
	struct device *cdev = cinfo->dev;
	struct device *rdev;
	struct scmi_mailbox *smbox;
	struct fwnode_handle *shmem;
	int ret, a2p_rx_chan, p2a_chan, idx = tx ? 0 : 1;
	struct mbox_client *cl;
	resource_size_t size;
	struct resource *res;
	struct platform_device *pdev;
	const char *str[1];

	ret = mailbox_chan_validate(cdev, &a2p_rx_chan, &p2a_chan);
	if (ret)
		return ret;

	if (!tx && !p2a_chan)
		return -ENODEV;

	smbox = devm_kzalloc(dev, sizeof(*smbox), GFP_KERNEL);
	if (!smbox)
		return -ENOMEM;

	shmem = fwnode_find_reference(cdev->fwnode, "shmem", idx);

	if (IS_ERR_OR_NULL(shmem))
		return -ENODEV;

	rdev = bus_find_device_by_fwnode(&platform_bus_type, shmem);
	pdev = rdev ? to_platform_device(rdev) : NULL;

	if (IS_ERR_OR_NULL(pdev))
		return ENODEV;

	if (fwnode_property_present(shmem, "compatible")) {
		ret = fwnode_property_read_string(shmem, "compatible", (const char **)&str);

		if (!strcmp(str[0], "arm,scmi-shmem")) {
			res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
			fwnode_handle_put(shmem);
		} else {
			ret = -ENXIO;
		}
	} else {
		ret = -ENXIO;
	}

	if (ret) {
		dev_err(cdev, "failed to get SCMI %s shared memory\n", desc);
		return ret;
	}

	size = resource_size(res);
	smbox->shmem = devm_ioremap(dev, res->start, size);
	if (!smbox->shmem) {
		dev_err(dev, "failed to ioremap SCMI %s shared memory\n", desc);
		return -EADDRNOTAVAIL;
	}

	cl = &smbox->cl;
	cl->dev = cdev;
	cl->tx_prepare = tx ? tx_prepare : NULL;
	cl->rx_callback = rx_callback;
	cl->tx_block = false;
	cl->knows_txdone = tx;

	smbox->chan = mbox_request_channel(cl, tx ? 0 : p2a_chan);
	if (IS_ERR(smbox->chan)) {
		ret = PTR_ERR(smbox->chan);
		if (ret != -EPROBE_DEFER)
			dev_err(cdev,
				"failed to request SCMI %s mailbox\n", desc);
		return ret;
	}

	/* Additional unidirectional channel for TX if needed */
	if (tx && a2p_rx_chan) {
		smbox->chan_receiver = mbox_request_channel(cl, a2p_rx_chan);
		if (IS_ERR(smbox->chan_receiver)) {
			ret = PTR_ERR(smbox->chan_receiver);
			if (ret != -EPROBE_DEFER)
				dev_err(cdev, "failed to request SCMI Tx Receiver mailbox\n");
			return ret;
		}
	}

	cinfo->transport_info = smbox;
	smbox->cinfo = cinfo;
	mutex_init(&smbox->chan_lock);

	return 0;
}

static int mailbox_chan_free(int id, void *p, void *data)
{
	struct scmi_chan_info *cinfo = p;
	struct scmi_mailbox *smbox = cinfo->transport_info;

	if (smbox && !IS_ERR(smbox->chan)) {
		mbox_free_channel(smbox->chan);
		mbox_free_channel(smbox->chan_receiver);
		cinfo->transport_info = NULL;
		smbox->chan = NULL;
		smbox->chan_receiver = NULL;
		smbox->cinfo = NULL;
	}

	return 0;
}

static int mailbox_send_message(struct scmi_chan_info *cinfo,
				struct scmi_xfer *xfer)
{
	struct scmi_mailbox *smbox = cinfo->transport_info;
	int ret;

	/*
	 * The mailbox layer has its own queue. However the mailbox queue
	 * confuses the per message SCMI timeouts since the clock starts when
	 * the message is submitted into the mailbox queue. So when multiple
	 * messages are queued up the clock starts on all messages instead of
	 * only the one inflight.
	 */
	mutex_lock(&smbox->chan_lock);

	ret = mbox_send_message(smbox->chan, xfer);
	/* mbox_send_message returns non-negative value on success */
	if (ret < 0) {
		mutex_unlock(&smbox->chan_lock);
		return ret;
	}

	return 0;
}

static void mailbox_mark_txdone(struct scmi_chan_info *cinfo, int ret,
				struct scmi_xfer *__unused)
{
	struct scmi_mailbox *smbox = cinfo->transport_info;

	mbox_client_txdone(smbox->chan, ret);

	/* Release channel */
	mutex_unlock(&smbox->chan_lock);
}

static void mailbox_fetch_response(struct scmi_chan_info *cinfo,
				   struct scmi_xfer *xfer)
{
	struct scmi_mailbox *smbox = cinfo->transport_info;

	shmem_fetch_response(smbox->shmem, xfer);
}

static void mailbox_fetch_notification(struct scmi_chan_info *cinfo,
				       size_t max_len, struct scmi_xfer *xfer)
{
	struct scmi_mailbox *smbox = cinfo->transport_info;

	shmem_fetch_notification(smbox->shmem, max_len, xfer);
}

static void mailbox_clear_channel(struct scmi_chan_info *cinfo)
{
	struct scmi_mailbox *smbox = cinfo->transport_info;

	shmem_clear_channel(smbox->shmem);
}

static bool
mailbox_poll_done(struct scmi_chan_info *cinfo, struct scmi_xfer *xfer)
{
	struct scmi_mailbox *smbox = cinfo->transport_info;

	return shmem_poll_done(smbox->shmem, xfer);
}

static const struct scmi_transport_ops scmi_mailbox_ops = {
	.chan_available = mailbox_chan_available,
	.chan_setup = mailbox_chan_setup,
	.chan_free = mailbox_chan_free,
	.send_message = mailbox_send_message,
	.mark_txdone = mailbox_mark_txdone,
	.fetch_response = mailbox_fetch_response,
	.fetch_notification = mailbox_fetch_notification,
	.clear_channel = mailbox_clear_channel,
	.poll_done = mailbox_poll_done,
};

const struct scmi_desc scmi_mailbox_desc = {
	.ops = &scmi_mailbox_ops,
	.max_rx_timeout_ms = 300, /* We may increase this if required */
	.max_msg = 20, /* Limited by MBOX_TX_QUEUE_LEN */
	.max_msg_size = 128,
};
