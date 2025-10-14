// SPDX-License-Identifier: GPL-2.0
/* Copyright 2025 Cix Technology Group Co., Ltd.*/
#include <linux/mailbox_client.h>
#include <linux/platform_device.h>
#include <mntn_public_interface.h>
#include <linux/io.h>
#include <linux/module.h>
#include <linux/syscore_ops.h>
#include <linux/soc/cix/cix_ap2se_ipc.h>
#include <linux/mutex.h>
#include <linux/mailbox_controller.h>

#define MBOX_SEND_TIMEOUT 100
#define MAX_SERVICES_NUM 16
#define CIX_AP2SE_IPC_DEBUG 0

struct cix_ap2se_ipc_dev {
	struct platform_device *pdev;
	struct mbox_client cl;
	struct mbox_chan *tx_ch;
	struct mbox_chan *rx_ch;
	struct completion rsp_comp;
	struct mutex mbox_lock;
	unsigned char init_status; // 0: not init, 1: init success;
};

typedef struct _services_cb_group {
	uint32_t FID; /* function number, refer to fw_dispatcher.h */
	ipc_rx_callback_t fun_cb;
} services_cb_group;

static services_cb_group services_cbs[MAX_SERVICES_NUM];

static struct cix_ap2se_ipc_dev g_ap2se_ipc_dev;

static int get_rx_callback(uint32_t cmd_id, ipc_rx_callback_t *cb)
{
	int ret;
	uint32_t index = 0U;

	if (!g_ap2se_ipc_dev.init_status) {
		pr_err("ipc not init\n");
		return -EIO;
	}

	for (index = 0U; index < MAX_SERVICES_NUM; index++) {
		if (cmd_id == services_cbs[index].FID) {
			*cb = services_cbs[index].fun_cb;
			break;
		}
	}

	if (index != MAX_SERVICES_NUM) {
		pr_debug("Add function ID: 0x%x \n", cmd_id);
		ret = 0;
	} else {
		pr_err("Mismatch function ID: 0x%x, it's not registered\n",
		       cmd_id);
		*cb = NULL;
		ret = -EFAULT;
	}

	return ret;
}

int cix_ap2se_register_rx_cbk(uint32_t cmd_id, ipc_rx_callback_t cbk)
{
	int ret;
	int index;

	if (!g_ap2se_ipc_dev.init_status) {
		pr_err("%s, ipc not init\n", __func__);
		return -EIO;
	}

	mutex_lock(&g_ap2se_ipc_dev.mbox_lock);

	for (index = 0U; index < MAX_SERVICES_NUM; index++) {
		if (0U == services_cbs[index].FID) {
			services_cbs[index].FID = cmd_id;
			services_cbs[index].fun_cb = cbk;
			break;
		}
	}

	if (index != MAX_SERVICES_NUM) {
		pr_info("%s, Register functio ID: 0x%x successful\n", __func__,
			cmd_id);
		ret = 0;
	} else {
		pr_err("Register handle fail, services_cbs was full\n");
		ret = -EIO;
	}

	mutex_unlock(&g_ap2se_ipc_dev.mbox_lock);
	return ret;
}
EXPORT_SYMBOL_GPL(cix_ap2se_register_rx_cbk);

static void cix_ap2se_ipc_rx_callback(struct mbox_client *cl, void *message)
{
#if CIX_AP2SE_IPC_DEBUG
	int i;
#endif
	struct mbox_msg_t *msg = (struct mbox_msg_t *)message;
	struct cix_ap2se_ipc_dev *mdev = dev_get_drvdata(cl->dev);
	ipc_rx_callback_t fun_cb;

	if (get_rx_callback(msg->cmd_id, &fun_cb)) {
		dev_err(cl->dev, "unknown cmd_id: %d\n", msg->cmd_id);
		return;
	}

#if CIX_AP2SE_IPC_DEBUG
	pr_info("cmd_id: 0x%x, size: %d\n", msg->cmd_id, msg->size);

	for (i = 0; i < (msg->size - MBOX_HEADER_SIZE) / sizeof(uint32_t);
	     i++) {
		pr_info("data[%d]: 0x%x\n", i, msg->data[i]);
	}
#endif

	fun_cb((char *)msg->data, msg->size - MBOX_HEADER_SIZE);

	complete(&mdev->rsp_comp); // notify send message
}

int cix_ap2se_ipc_send(uint32_t cmd_id, char *data, size_t len,
			bool need_reply)
{
	int ret;
	struct cix_ap2se_ipc_dev *mdev = &g_ap2se_ipc_dev;
	uint32_t *msg;

	if (!mdev->init_status) {
		pr_err("%s, ipc not init\n", __func__);
		return -EIO;
	}

	if (len > (CIX_MBOX_MSG_LEN - MBOX_HEADER_NUM) * sizeof(uint32_t)) {
		pr_err("%s, data size is too large\n", __func__);
		return -EINVAL;
	}

	msg = (uint32_t *)kzalloc(MBOX_HEADER_SIZE + len, GFP_KERNEL);
	if (!msg) {
		return -ENOMEM;
	}

	msg[0] = MBOX_HEADER_SIZE + len;
	msg[1] = cmd_id;
	memcpy(msg + MBOX_HEADER_NUM, data, len);

	if (!in_irq())
		mutex_lock(&mdev->mbox_lock);
	if (in_irq())
		need_reply = 0;
	if (need_reply)
		reinit_completion(&mdev->rsp_comp);

	ret = mbox_send_message(mdev->tx_ch, (void *)msg);
	if (ret < 0) {
		dev_info(&mdev->pdev->dev, "failed to send message: %d\n", ret);
		goto out;
	}

	if (need_reply) {
		if (!wait_for_completion_timeout(
			    &mdev->rsp_comp,
			    msecs_to_jiffies(MBOX_SEND_TIMEOUT * 2))) {
			pr_err("%s,%d: wait for completion timeout\n", __func__,
			       __LINE__);
			ret = -ETIMEDOUT;
		}
	}

out:
	kfree(msg);
	if (!in_irq())
		mutex_unlock(&mdev->mbox_lock);
	return ret;
}
EXPORT_SYMBOL_GPL(cix_ap2se_ipc_send);

static void cix_ap2se_ipc_free_mbox(struct cix_ap2se_ipc_dev *mdev)
{
	if (!IS_ERR_OR_NULL(mdev->tx_ch)) {
		mbox_free_channel(mdev->tx_ch);
		mdev->tx_ch = NULL;
	}

	if (!IS_ERR_OR_NULL(mdev->rx_ch)) {
		mbox_free_channel(mdev->rx_ch);
		mdev->rx_ch = NULL;
	}

	mdev->init_status = 0;
}

static int cix_ap2se_ipc_setup_mbox(struct cix_ap2se_ipc_dev *mdev)
{
	struct device *dev = &mdev->pdev->dev;
	struct mbox_client *cl;
	int ret = 0;

	cl = &mdev->cl;
	cl->dev = dev;
	cl->tx_block = false;
	cl->tx_tout = MBOX_SEND_TIMEOUT;
	cl->knows_txdone = false;
	cl->rx_callback = cix_ap2se_ipc_rx_callback;
	mdev->tx_ch = mbox_request_channel_byname(cl, "tx4");
	if (IS_ERR_OR_NULL(mdev->tx_ch)) {
		ret = PTR_ERR(mdev->tx_ch);
		dev_err(cl->dev, "failed to request tx mailbox channel: %d\n",
			ret);
		goto out;
	}
	mdev->rx_ch = mbox_request_channel_byname(cl, "rx4");
	if (IS_ERR_OR_NULL(mdev->rx_ch)) {
		ret = PTR_ERR(mdev->rx_ch);
		dev_err(cl->dev, "failed to request rx mailbox channel: %d\n",
			ret);
		mbox_free_channel(mdev->tx_ch);
		goto out;
	}
out:
	return ret;
}
static int cix_ap2se_ipc_probe(struct platform_device *pdev)
{
	int ret = 0;
	struct cix_ap2se_ipc_dev *mdev = &g_ap2se_ipc_dev;

	mdev->pdev = pdev;
	platform_set_drvdata(pdev, mdev);
	ret = cix_ap2se_ipc_setup_mbox(mdev);
	if (ret) {
		dev_err(&pdev->dev, "ap2se ipc setup mbox failed\n");
		return ret;
	}
	init_completion(&mdev->rsp_comp);
	mutex_init(&mdev->mbox_lock);
	mdev->init_status = 1;
	return ret;
}
static int cix_ap2se_ipc_remove(struct platform_device *pdev)
{
	struct cix_ap2se_ipc_dev *mdev = platform_get_drvdata(pdev);

	cix_ap2se_ipc_free_mbox(mdev);
	return 0;
}

static int cix_ap2se_ipc_suspend(struct device *dev)
{
	struct cix_ap2se_ipc_dev *mdev = dev_get_drvdata(dev);

	dev_dbg(dev, "cix_ap2se_ipc suspend %d\n", mdev->init_status);
	if (mdev->init_status)
		cix_ap2se_ipc_free_mbox(mdev);

	return 0;
}

static int cix_ap2se_ipc_resume(struct device *dev)
{
	struct cix_ap2se_ipc_dev *mdev = dev_get_drvdata(dev);
	int ret;

	dev_dbg(dev, "cix_ap2se_ipc resume %d\n", mdev->init_status);
	if (mdev->init_status) {
		pr_info("cix_ap2se_ipc already initialized\n");
		return 0;
	}

	ret = cix_ap2se_ipc_setup_mbox(mdev);
	if (ret) {
		pr_err("cix_ap2se_ipc setup failed\n");
		return ret;
	}
	mdev->init_status = 1;
	return 0;
}

static const struct of_device_id cix_ap2se_ipc_of_match[] = {
	{ .compatible = "cix,cix_se2ap_mbox" },
	{ /* end node */ },
};
MODULE_DEVICE_TABLE(of, cix_ap2se_ipc_of_match);

static const struct dev_pm_ops cix_ap2se_ipc_pm_ops = {
	SET_SYSTEM_SLEEP_PM_OPS(cix_ap2se_ipc_suspend, cix_ap2se_ipc_resume)
};

static struct platform_driver cix_ap2se_ipc_driver = {
	.probe = cix_ap2se_ipc_probe,
	.remove = cix_ap2se_ipc_remove,
	.driver = {
		.name = "cix_ap2se_mbox",
		.of_match_table = cix_ap2se_ipc_of_match,
		.pm = &cix_ap2se_ipc_pm_ops,
	},
};

static int __init cix_ap2se_ipc_init(void)
{
	return platform_driver_register(&cix_ap2se_ipc_driver);
}
subsys_initcall(cix_ap2se_ipc_init);

MODULE_AUTHOR("Vincent Wu <vincent.wu@cixtech.com>");
MODULE_DESCRIPTION("CIX AP2SE IPC driver");
MODULE_LICENSE("GPL v2");
