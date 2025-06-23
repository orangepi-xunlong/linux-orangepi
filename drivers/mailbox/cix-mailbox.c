// SPDX-License-Identifier: GPL-2.0
// Copyright 2024 Cix Technology Group Co., Ltd.

#include <linux/device.h>
#include <linux/io.h>
#include <linux/err.h>
#include <linux/interrupt.h>
#include <linux/mailbox_controller.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/platform_device.h>
#include <linux/acpi.h>

#include "mailbox.h"

/* Register define */
#define REG_MSG(n)		(0x0 + 0x4*(n)) /* 0x0~0x7c */
#define REG_DB_ACK		REG_MSG(CIX_MBOX_MSG_LEN) /* 0x80 */
#define ERR_COMP		(REG_DB_ACK + 0x4) /* 0x84 */
#define ERR_COMP_CLR		(REG_DB_ACK + 0x8) /* 0x88 */
#define REG_F_INT(IDX)		(ERR_COMP_CLR + 0x4*(IDX+1)) /* 0x8c~0xa8 */
#define FIFO_WR			(REG_F_INT(MBOX_FAST_IDX+1)) /* 0xac */
#define FIFO_RD			(FIFO_WR + 0x4) /* 0xb0 */
#define FIFO_STAS		(FIFO_WR + 0x8) /* 0xb4 */
#define FIFO_WM			(FIFO_WR + 0xc) /* 0xb8 */
#define INT_ENABLE		(FIFO_WR + 0x10) /* 0xbc */
#define INT_ENABLE_SIDE_B	(FIFO_WR + 0x14) /* 0xc0 */
#define INT_CLEAR		(FIFO_WR + 0x18) /* 0xc4 */
#define INT_STATUS		(FIFO_WR + 0x1c) /* 0xc8 */
#define FIFO_RST		(FIFO_WR + 0x20) /* 0xcc */

/* [0~7] Fast channel
 * [8] doorbell base channel
 * [9]fifo base channel
 * [10] register base channel
 */
#define CIX_MBOX_CHANS		(11)
#define CIX_MBOX_MSG_LEN	(32)
#define MBOX_MSG_LEN_MASK	(0x7fL)

#define MBOX_FAST_IDX		(7)
#define MBOX_DB_IDX		(8)
#define MBOX_FIFO_IDX		(9)
#define MBOX_REG_IDX		(10)

#define MBOX_TX			(0)
#define MBOX_RX			(1)

#define DB_INT_BIT		BIT(0)
#define DB_ACK_INT_BIT		BIT(1)

#define FIFO_WM_DEFAULT		CIX_MBOX_MSG_LEN
#define FIFO_STAS_WMK		BIT(0)
#define FIFO_STAS_FULL		BIT(1)
#define FIFO_STAS_EMPTY		BIT(2)
#define FIFO_STAS_UFLOW		BIT(3)
#define FIFO_STAS_OFLOW		BIT(4)

#define FIFO_RST_BIT		BIT(0)

#define DB_INT			BIT(0)
#define ACK_INT			BIT(1)
#define FIFO_FULL_INT		BIT(2)
#define FIFO_EMPTY_INT		BIT(3)
#define FIFO_WM01_INT		BIT(4)
#define FIFO_WM10_INT		BIT(5)
#define FIFO_OFLOW_INT		BIT(6)
#define FIFO_UFLOW_INT		BIT(7)
#define FIFO_N_EMPTY_INT	BIT(8)
#define FAST_CH_INT(IDX)	BIT((IDX)+9)

enum cix_mbox_chan_type {
	CIX_MBOX_TYPE_DB,
	CIX_MBOX_TYPE_REG,
	CIX_MBOX_TYPE_FIFO,
	CIX_MBOX_TYPE_FAST,
};

struct cix_mbox_con_priv {
	enum cix_mbox_chan_type type;
	struct mbox_chan    *chan;
	int index;
};

struct cix_mbox_priv {
	struct device *dev;
	int irq;
	int dir;
	bool tx_irq_mode; /* flag of enabling tx's irq mode */
	void __iomem *base; /* region for mailbox */
	unsigned int chan_num;
	struct cix_mbox_con_priv con_priv[CIX_MBOX_CHANS];
	struct mbox_chan mbox_chans[CIX_MBOX_CHANS];
	struct mbox_controller mbox;
};

static struct cix_mbox_priv *to_cix_mbox_priv(struct mbox_controller *mbox)
{
	return container_of(mbox, struct cix_mbox_priv, mbox);
}

static void cix_mbox_write(struct cix_mbox_priv *priv, u32 val, u32 offset)
{
	iowrite32(val, priv->base + offset);
}

static u32 cix_mbox_read(struct cix_mbox_priv *priv, u32 offset)
{
	return ioread32(priv->base + offset);
}

static bool mbox_fifo_empty(struct mbox_chan *chan)
{
	struct cix_mbox_priv *priv = to_cix_mbox_priv(chan->mbox);

	return ((cix_mbox_read(priv, FIFO_STAS) & FIFO_STAS_EMPTY) ? true : false);
}

static int cix_mbox_send_data_db(struct mbox_chan *chan, void *data)
{
	struct cix_mbox_priv *priv = to_cix_mbox_priv(chan->mbox);

	/* trigger doorbell irq */
	cix_mbox_write(priv, DB_INT_BIT, REG_DB_ACK);

	return 0;
}

static int cix_mbox_send_data_reg(struct mbox_chan *chan, void *data)
{
	struct cix_mbox_priv *priv = to_cix_mbox_priv(chan->mbox);
	int i;
	u32 *arg = (u32 *)data;
	u32 len;

	if (!data)
		return -EINVAL;

	len = arg[0] & MBOX_MSG_LEN_MASK;
	for (i = 0; i < len; i++)
		cix_mbox_write(priv, arg[i], REG_MSG(i));

	/* trigger doorbell irq */
	cix_mbox_write(priv, DB_INT_BIT, REG_DB_ACK);

	return 0;
}

static int cix_mbox_send_data_fifo(struct mbox_chan *chan, void *data)
{
	struct cix_mbox_priv *priv = to_cix_mbox_priv(chan->mbox);
	int i;
	u32 *arg = (u32 *)data;
	u32 len, val_32;

	if (!data)
		return -EINVAL;

	len =  arg[0] & MBOX_MSG_LEN_MASK;
	cix_mbox_write(priv, len, FIFO_WM);
	for (i = 0; i < len; i++)
		cix_mbox_write(priv, arg[i], FIFO_WR);

	/* Enable fifo empty interrupt */
	val_32 = cix_mbox_read(priv, INT_ENABLE);
	val_32 |= FIFO_EMPTY_INT;
	cix_mbox_write(priv, val_32, INT_ENABLE);

	return 0;
}

static int cix_mbox_send_data_fast(struct mbox_chan *chan, void *data)
{
	struct cix_mbox_priv *priv = to_cix_mbox_priv(chan->mbox);
	struct cix_mbox_con_priv *cp = chan->con_priv;
	u32 *arg = (u32 *)data;
	int index = cp->index;

	if (!data)
		return -EINVAL;

	if (index < 0 || index > MBOX_FAST_IDX) {
		dev_err(priv->dev, "Invalid Mbox index %d\n", index);
		return -EINVAL;
	}

	cix_mbox_write(priv, arg[0], REG_F_INT(index));

	return 0;
}

static int cix_mbox_send_data(struct mbox_chan *chan, void *data)
{
	struct cix_mbox_priv *priv = to_cix_mbox_priv(chan->mbox);
	struct cix_mbox_con_priv *cp = chan->con_priv;

	if (priv->dir != MBOX_TX) {
		dev_err(priv->dev, "Invalid Mbox dir %d\n", priv->dir);
		return -EINVAL;
	}

	switch (cp->type) {
	case CIX_MBOX_TYPE_DB:
		cix_mbox_send_data_db(chan, data);
		break;
	case CIX_MBOX_TYPE_REG:
		cix_mbox_send_data_reg(chan, data);
		break;
	case CIX_MBOX_TYPE_FIFO:
		cix_mbox_send_data_fifo(chan, data);
		break;
	case CIX_MBOX_TYPE_FAST:
		cix_mbox_send_data_fast(chan, data);
		break;
	default:
		dev_err(priv->dev, "Invalid channel type: %d\n", cp->type);
		return -EINVAL;
	}
	return 0;
}

static void cix_mbox_isr_db(struct mbox_chan *chan)
{
	struct cix_mbox_priv *priv = to_cix_mbox_priv(chan->mbox);
	u32 int_status;

	int_status = cix_mbox_read(priv, INT_STATUS);

	if (priv->dir == MBOX_RX) {
		/* rx interrupt is triggered */
		if (int_status & DB_INT) {
			cix_mbox_write(priv, DB_INT, INT_CLEAR);
			mbox_chan_received_data(chan, NULL);
			/* trigger ack interrupt */
			cix_mbox_write(priv, DB_ACK_INT_BIT, REG_DB_ACK);
		}
	} else if (priv->dir == MBOX_TX) {
		/* tx ack interrupt is triggered */
		if (int_status & ACK_INT) {
			cix_mbox_write(priv, ACK_INT, INT_CLEAR);
			mbox_chan_received_data(chan, NULL);
		}
	}
}

static void cix_mbox_isr_reg(struct mbox_chan *chan)
{
	struct cix_mbox_priv *priv = to_cix_mbox_priv(chan->mbox);
	u32 int_status;
	u32 data[CIX_MBOX_MSG_LEN];
	int i;
	u32 len;

	int_status = cix_mbox_read(priv, INT_STATUS);

	if (priv->dir == MBOX_RX) {
		/* rx interrupt is triggered */
		if (int_status & DB_INT) {
			cix_mbox_write(priv, DB_INT, INT_CLEAR);
			data[0] = cix_mbox_read(priv, REG_MSG(0));
			len = data[0] & MBOX_MSG_LEN_MASK;
			for (i = 0; i < len; i++)
				data[i] = cix_mbox_read(priv, REG_MSG(i));

			/* trigger ack interrupt */
			cix_mbox_write(priv, DB_ACK_INT_BIT, REG_DB_ACK);
			mbox_chan_received_data(chan, data);
		}
	} else if (priv->dir == MBOX_TX) {
		/* tx ack interrupt is triggered */
		if (int_status & ACK_INT) {
			cix_mbox_write(priv, ACK_INT, INT_CLEAR);
			mbox_chan_txdone(chan, 0);
		}
	}
}

static void cix_mbox_isr_fifo(struct mbox_chan *chan)
{
	struct cix_mbox_priv *priv = to_cix_mbox_priv(chan->mbox);
	u32 data[CIX_MBOX_MSG_LEN];
	int i = 0;
	u32 int_status, status, val_32;

	int_status = cix_mbox_read(priv, INT_STATUS);

	if (priv->dir == MBOX_RX) {
		/* FIFO waterMark interrupt is generated */
		if (int_status & (FIFO_FULL_INT | FIFO_WM01_INT)) {
			cix_mbox_write(priv, (FIFO_FULL_INT | FIFO_WM01_INT), INT_CLEAR);
			do {
				data[i++] = cix_mbox_read(priv, FIFO_RD);
			} while (false == mbox_fifo_empty(chan));
			mbox_chan_received_data(chan, data);
		}
		/* FIFO underflow is generated */
		if (int_status & FIFO_UFLOW_INT) {
			status = cix_mbox_read(priv, FIFO_STAS);
			dev_err(priv->dev,
				"fifo underflow: int_stats %d\n",
				status);
			cix_mbox_write(priv, FIFO_UFLOW_INT, INT_CLEAR);
		}
	} else if (priv->dir == MBOX_TX) {
		/* FIFO empty interrupt is generated */
		if (int_status & FIFO_EMPTY_INT) {
			cix_mbox_write(priv, FIFO_EMPTY_INT, INT_CLEAR);
			/* Disable empty irq*/
			val_32 = cix_mbox_read(priv, INT_ENABLE);
			val_32 &= ~FIFO_EMPTY_INT;
			cix_mbox_write(priv, val_32, INT_ENABLE);
			mbox_chan_txdone(chan, 0);
		}
		/* FIFO overflow is generated */
		if (int_status & FIFO_OFLOW_INT) {
			status = cix_mbox_read(priv, FIFO_STAS);
			dev_err(priv->dev,
				"fifo overlow: int_stats %d\n",
				status);
			cix_mbox_write(priv, FIFO_OFLOW_INT, INT_CLEAR);
		}
	}
}

static void cix_mbox_isr_fast(struct mbox_chan *chan)
{
	struct cix_mbox_priv *priv = to_cix_mbox_priv(chan->mbox);
	struct cix_mbox_con_priv *cp = chan->con_priv;
	u32 int_status, data;

	/* no irq will be trigger for TX dir mbox */
	if (priv->dir != MBOX_RX)
		return;

	int_status = cix_mbox_read(priv, INT_STATUS);

	if (int_status & FAST_CH_INT(cp->index)) {
		cix_mbox_write(priv,
			       FAST_CH_INT(cp->index),
			       INT_CLEAR);
		data = cix_mbox_read(priv, REG_F_INT(cp->index));
		mbox_chan_received_data(chan, &data);
	}

}

static irqreturn_t cix_mbox_isr(int irq, void *arg)
{
	struct mbox_chan *chan = arg;
	struct cix_mbox_priv *priv = to_cix_mbox_priv(chan->mbox);
	struct cix_mbox_con_priv *cp = chan->con_priv;

	switch (cp->type) {
	case CIX_MBOX_TYPE_DB:
		cix_mbox_isr_db(chan);
		break;
	case CIX_MBOX_TYPE_REG:
		cix_mbox_isr_reg(chan);
		break;
	case CIX_MBOX_TYPE_FIFO:
		cix_mbox_isr_fifo(chan);
		break;
	case CIX_MBOX_TYPE_FAST:
		cix_mbox_isr_fast(chan);
		break;
	default:
		dev_err(priv->dev, "Invalid channel type: %d\n", cp->type);
		return IRQ_NONE;
	}

	return IRQ_HANDLED;
}

static int cix_mbox_startup(struct mbox_chan *chan)
{
	struct cix_mbox_priv *priv = to_cix_mbox_priv(chan->mbox);
	struct cix_mbox_con_priv *cp = chan->con_priv;
	int ret;
	int index = cp->index;
	u32 val_32;

	ret = request_irq(priv->irq,
			  cix_mbox_isr,
			  IRQF_NO_SUSPEND,
			  dev_name(priv->dev),
			  chan);
	if (ret) {
		dev_err(priv->dev,
			"Unable to acquire IRQ %d\n",
			priv->irq);
		return ret;
	}
	dev_info(priv->dev,
		 "%s, base %px, irq %d, dir %d, type %d, index %d\n",
		 __func__,
		 priv->base,
		 priv->irq,
		 priv->dir,
		 cp->type,
		 cp->index);

	switch (cp->type) {
	case CIX_MBOX_TYPE_DB:
		/* Overwrite txdone_method for DB channel */
		chan->txdone_method = TXDONE_BY_ACK;
		fallthrough;
	case CIX_MBOX_TYPE_REG:
		if (priv->dir == MBOX_TX) {
			/* Enable ACK interrupt */
			val_32 = cix_mbox_read(priv, INT_ENABLE);
			val_32 |= ACK_INT;
			cix_mbox_write(priv, val_32, INT_ENABLE);
		} else if (priv->dir == MBOX_RX) {
			/* Enable Doorbell interrupt */
			val_32 = cix_mbox_read(priv, INT_ENABLE_SIDE_B);
			val_32 |= DB_INT;
			cix_mbox_write(priv, val_32, INT_ENABLE_SIDE_B);
		}
		break;
	case CIX_MBOX_TYPE_FIFO:
		/* reset fifo */
		cix_mbox_write(priv, FIFO_RST_BIT, FIFO_RST);
		/* set default watermark */
		cix_mbox_write(priv, FIFO_WM_DEFAULT, FIFO_WM);
		if (priv->dir == MBOX_TX) {
			/* Enable fifo overflow interrupt */
			val_32 = cix_mbox_read(priv, INT_ENABLE);
			val_32 |= FIFO_OFLOW_INT;
			cix_mbox_write(priv, val_32, INT_ENABLE);
		} else if (priv->dir == MBOX_RX) {
			/* Enable fifo full/underflow interrupt */
			val_32 = cix_mbox_read(priv, INT_ENABLE_SIDE_B);
			val_32 |= FIFO_UFLOW_INT|FIFO_WM01_INT;
			cix_mbox_write(priv, val_32, INT_ENABLE_SIDE_B);
		}
		break;
	case CIX_MBOX_TYPE_FAST:
		/* Only RX channel has intterupt */
		if (priv->dir == MBOX_RX) {
			if (index < 0 || index > MBOX_FAST_IDX) {
				dev_err(priv->dev,
					"Invalid index %d\n", index);
				return ret;
			}
			/* enable fast channel interrupt */
			val_32 = cix_mbox_read(priv, INT_ENABLE_SIDE_B);
			val_32 |= FAST_CH_INT(index);
			cix_mbox_write(priv, val_32, INT_ENABLE_SIDE_B);
		}
		break;
	default:
		dev_err(priv->dev, "Invalid channel type: %d\n", cp->type);
		return -EINVAL;
	}
	return 0;
}

static void cix_mbox_shutdown(struct mbox_chan *chan)
{
	struct cix_mbox_priv *priv = to_cix_mbox_priv(chan->mbox);
	struct cix_mbox_con_priv *cp = chan->con_priv;
	u32 val_32;
	int index = cp->index;

	switch (cp->type) {
	case CIX_MBOX_TYPE_DB:
	case CIX_MBOX_TYPE_REG:
		if (priv->dir == MBOX_TX) {
			/* Disable ACK interrupt */
			val_32 = cix_mbox_read(priv, INT_ENABLE);
			val_32 &= ~ACK_INT;
			cix_mbox_write(priv, val_32, INT_ENABLE);
		} else if (priv->dir == MBOX_RX) {
			/* Disable Doorbell interrupt */
			val_32 = cix_mbox_read(priv, INT_ENABLE_SIDE_B);
			val_32 &= ~DB_INT;
			cix_mbox_write(priv, val_32, INT_ENABLE_SIDE_B);
		}
		break;
	case CIX_MBOX_TYPE_FIFO:
		if (priv->dir == MBOX_TX) {
			/* Disable empty/fifo overflow irq*/
			val_32 = cix_mbox_read(priv, INT_ENABLE);
			val_32 &= ~(FIFO_EMPTY_INT | FIFO_OFLOW_INT);
			cix_mbox_write(priv, val_32, INT_ENABLE);
		} else if (priv->dir == MBOX_RX) {
			/* Disable fifo WM01/underflow interrupt */
			val_32 = cix_mbox_read(priv, INT_ENABLE_SIDE_B);
			val_32 &= ~(FIFO_UFLOW_INT | FIFO_WM01_INT);
			cix_mbox_write(priv, val_32, INT_ENABLE_SIDE_B);
		}
		break;
	case CIX_MBOX_TYPE_FAST:
		if (priv->dir == MBOX_RX) {
			if (index < 0 || index > MBOX_FAST_IDX) {
				dev_err(priv->dev,
					"Invalid index %d\n", index);
				break;
			}
			/* Disable fast channel interrupt */
			val_32 = cix_mbox_read(priv, INT_ENABLE_SIDE_B);
			val_32 &= ~FAST_CH_INT(index);
			cix_mbox_write(priv, val_32, INT_ENABLE_SIDE_B);
		}
		break;

	default:
		dev_err(priv->dev, "Invalid channel type: %d\n", cp->type);
		break;
	}

	free_irq(priv->irq, chan);
}

static const struct mbox_chan_ops cix_mbox_chan_ops = {
	.send_data = cix_mbox_send_data,
	.startup = cix_mbox_startup,
	.shutdown = cix_mbox_shutdown,
};

static void cix_mbox_init(struct cix_mbox_priv *priv)
{
	int i;
	struct cix_mbox_con_priv *cp;

	for (i = 0; i < CIX_MBOX_CHANS; i++) {
		cp = &priv->con_priv[i];
		cp->index = i;
		cp->chan = &priv->mbox_chans[i];
		priv->mbox_chans[i].con_priv = cp;
		if (cp->index <= MBOX_FAST_IDX)
			cp->type = CIX_MBOX_TYPE_FAST;
		if (cp->index == MBOX_DB_IDX)
			cp->type = CIX_MBOX_TYPE_DB;
		if (cp->index == MBOX_FIFO_IDX)
			cp->type = CIX_MBOX_TYPE_FIFO;
		if (cp->index == MBOX_REG_IDX)
			cp->type = CIX_MBOX_TYPE_REG;
	}
}

static int cix_mbox_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct cix_mbox_priv *priv;
	int ret;
	u32 dir;

	dev_dbg(dev, "%s!\n", __func__);
	priv = devm_kzalloc(dev, sizeof(*priv), GFP_KERNEL);
	if (!priv)
		return -ENOMEM;

	priv->dev = dev;
	priv->base = devm_platform_ioremap_resource(pdev, 0);
	if (IS_ERR(priv->base))
		return PTR_ERR(priv->base);

	priv->irq = platform_get_irq(pdev, 0);
	if (priv->irq < 0)
		return priv->irq;

	if (device_property_read_u32(dev, "cix,mbox_dir", &dir)) {
		dev_err(priv->dev, "cix,mbox_dir property not found\n");
		return -EINVAL;
	}

	if ((dir != MBOX_TX)
	    && (dir != MBOX_RX)) {
		dev_err(priv->dev, "Dir value is not expected! dir %d\n", dir);
		return -EINVAL;
	}

	cix_mbox_init(priv);

	priv->dir = (int)dir;
	priv->mbox.dev = dev;
	priv->mbox.ops = &cix_mbox_chan_ops;
	priv->mbox.chans = priv->mbox_chans;
	priv->mbox.txdone_irq = true;
	priv->mbox.num_chans = CIX_MBOX_CHANS;
	priv->mbox.of_xlate = NULL;
	priv->mbox.acpi_xlate = NULL;
	dev_info(priv->dev,
		 "%s, base %px, irq %d, dir %d\n",
		 __func__,
		 priv->base,
		 priv->irq,
		 priv->dir);

	platform_set_drvdata(pdev, priv);
	ret = devm_mbox_controller_register(dev, &priv->mbox);
	if (ret)
		return ret;

	return 0;
}

static int cix_mbox_remove(struct platform_device *pdev)
{
	struct cix_mbox_priv *priv = platform_get_drvdata(pdev);

	dev_err(priv->dev, "cix mbox removed!\n");

	return 0;
}

static const struct of_device_id cix_mbox_dt_ids[] = {
	{ .compatible = "cix,sky1-mbox" },
	{ },
};
MODULE_DEVICE_TABLE(of, cix_mbox_dt_ids);

static const struct acpi_device_id cix_mbox_acpi_match[] = {
	{ "CIXHA001", 0 }, /* sky1 mailbox */
	{ },
};
MODULE_DEVICE_TABLE(acpi, cix_mbox_acpi_match);

static struct platform_driver cix_mbox_driver = {
	.probe = cix_mbox_probe,
	.remove = cix_mbox_remove,
	.driver = {
		.name = "cix_mbox",
		.of_match_table = cix_mbox_dt_ids,
		.acpi_match_table = ACPI_PTR(cix_mbox_acpi_match),
	},
};

static int __init cix_mailbox_init(void)
{
	return platform_driver_register(&cix_mbox_driver);
}
arch_initcall(cix_mailbox_init);

MODULE_AUTHOR("Lihua Liu <Lihua.Liu@cixcomputing.com>");
MODULE_DESCRIPTION("CIX mailbox driver");
MODULE_LICENSE("GPL v2");
