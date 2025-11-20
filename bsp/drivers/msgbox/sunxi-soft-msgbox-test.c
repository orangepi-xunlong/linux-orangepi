// SPDX-License-Identifier: GPL-2.0

#include <linux/bitops.h>
#include <linux/clk.h>
#include <linux/device.h>
#include <linux/err.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/kernel.h>
#include <linux/mailbox_client.h>
#include <linux/mailbox_controller.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_irq.h>
#include <linux/of_address.h>
#include <linux/platform_device.h>
#include <linux/reset.h>
#include <linux/spinlock.h>


struct sunxi_mbox_test {
	spinlock_t lock;
	struct mbox_chan *test_chan;
	struct class *class;
};


static struct sunxi_mbox_test *priv;

static void sunxi_mbox_rx_callback(struct mbox_client *cl, void *data)
{
	u32 msg = *((u32 *)data);

	printk("--- msg = %d ---\n", msg);
}

static void sunxi_mbox_tx_done(struct mbox_client *cl, void *msg, int r) {}

static void sunxi_mbox_prepare_message(struct mbox_client *client, void *message) {}


static struct mbox_chan *
sunxi_mbox_request_channel(struct platform_device *pdev, const char *name)
{
	struct mbox_client *client;
	struct mbox_chan *channel;

	client = devm_kzalloc(&pdev->dev, sizeof(*client), GFP_KERNEL);
	if (!client)
		return ERR_PTR(-ENOMEM);

	client->dev		= &pdev->dev;
	client->rx_callback	= sunxi_mbox_rx_callback;
	client->tx_prepare	= sunxi_mbox_prepare_message;
	client->tx_done		= sunxi_mbox_tx_done;
	client->tx_block	= false;
	client->knows_txdone	= false;
	//client->tx_tout		= 500;

	channel = mbox_request_channel_byname(client, name);
	if (IS_ERR(channel)) {
		dev_err(&pdev->dev, "Failed to request %s channel\n", name);
		return NULL;
	}

	return channel;
}

static ssize_t sunxi_mbox_show(struct class *class, struct class_attribute *attr, char *buf)
{
	return sprintf(buf, "1\n");
}


static ssize_t sunxi_mbox_store(struct class *class, struct class_attribute *attr,
	   const char *buf, size_t count)
{
	unsigned int user_data;
	int ret;

	if (kstrtouint(buf, 10, &user_data))
		return -EINVAL;

	ret = mbox_send_message(priv->test_chan, &user_data);
	if (ret < 0)
		pr_err("Failed to send message via mailbox\n");

	return count;
}

static struct class_attribute mbox_class_attrs[] = {
	__ATTR(data,  0664, sunxi_mbox_show,  sunxi_mbox_store),
};



static int sunxi_mbox_test_probe(struct platform_device *pdev)
{
	int ret;
	int i;
	struct sunxi_mbox_test *mbox_test;
	struct device *dev = &pdev->dev;

	pr_info("--- %s register init ---\n", __func__);

	mbox_test = devm_kzalloc(dev, sizeof(*mbox_test), GFP_KERNEL);
	if (!mbox_test)
		return -ENOMEM;

	mbox_test->test_chan = sunxi_mbox_request_channel(pdev, "mbox-test");
	if (!mbox_test->test_chan) {
		return -ENOMEM;
	}

	/* sys/class/mbox */
	mbox_test->class = class_create(THIS_MODULE, "mbox-test");
	if (IS_ERR(mbox_test->class)) {
		pr_err("%s:%u class_create() failed\n", __func__, __LINE__);
		return PTR_ERR(mbox_test->class);
	}

	/* sys/class/sunxi_dump/xxx */
	for (i = 0; i < ARRAY_SIZE(mbox_class_attrs); i++) {
		ret = class_create_file(mbox_test->class, &mbox_class_attrs[i]);
		if (ret) {
			pr_err("%s:%u class_create_file() failed. err=%d\n", __func__, __LINE__, ret);
			while (i--) {
				class_remove_file(mbox_test->class, &mbox_class_attrs[i]);
			}
			class_destroy(mbox_test->class);
			mbox_test->class = NULL;
			return ret;
		}
	}

	priv = mbox_test;
	platform_set_drvdata(pdev, mbox_test);

	pr_info("--- %s register success ---\n", __func__);

	return 0;
}


static int sunxi_mbox_test_remove(struct platform_device *pdev)
{
	int i;
	struct sunxi_mbox_test *mbox_test = platform_get_drvdata(pdev);

	for (i = 0; i < ARRAY_SIZE(mbox_class_attrs); i++) {
		class_destroy(mbox_test->class);
		mbox_test->class = NULL;
	}
	return 0;
}

static const struct of_device_id sunxi_mbox_test_of_match[] = {
	{ .compatible = "allwinner,irq-msgbox-test", },
	{},
};

MODULE_DEVICE_TABLE(of, sunxi_mbox_test_of_match);

static struct platform_driver sunxi_mbox_test_driver = {
	.driver = {
		.name = "sunxi-irq-msgbox-test",
		.of_match_table = sunxi_mbox_test_of_match,
	},
	.probe  = sunxi_mbox_test_probe,
	.remove = sunxi_mbox_test_remove,
};

module_platform_driver(sunxi_mbox_test_driver);

MODULE_AUTHOR("sw1@allwinnertech.com");
MODULE_DESCRIPTION("Allwinner sunxi irq Message Box");
MODULE_LICENSE("GPL v2");
