/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * Allwinner msgbox driver for Linux.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2. This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

//#define DEBUG
#include <sunxi-common.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/err.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/of_irq.h>
#include <linux/platform_device.h>
#include <linux/clk.h>
#include <linux/reset.h>
#include <linux/spinlock.h>
#include <linux/bitops.h>
#include <linux/mailbox_controller.h>

#define SUNXI_MSGBOX_OFFSET(n)			(0x100 * (n))
#define SUNXI_MSGBOX_READ_IRQ_ENABLE(n)		(0x20 + SUNXI_MSGBOX_OFFSET(n))
#define SUNXI_MSGBOX_READ_IRQ_STATUS(n)		(0x24 + SUNXI_MSGBOX_OFFSET(n))
#define SUNXI_MSGBOX_WRITE_IRQ_ENABLE(n)	(0x30 + SUNXI_MSGBOX_OFFSET(n))
#define SUNXI_MSGBOX_WRITE_IRQ_STATUS(n)	(0x34 + SUNXI_MSGBOX_OFFSET(n))
#define SUNXI_MSGBOX_DEBUG_REGISTER(n)		(0x40 + SUNXI_MSGBOX_OFFSET(n))
#define SUNXI_MSGBOX_FIFO_STATUS(n, p)		(0x50 + SUNXI_MSGBOX_OFFSET(n) + 0x4 * (p))
#define SUNXI_MSGBOX_MSG_STATUS(n, p)		(0x60 + SUNXI_MSGBOX_OFFSET(n) + 0x4 * (p))
#define SUNXI_MSGBOX_MSG_FIFO(n, p)		(0x70 + SUNXI_MSGBOX_OFFSET(n) + 0x4 * (p))
#define SUNXI_MSGBOX_WRITE_IRQ_THRESHOLD(n, p)	(0x80 + SUNXI_MSGBOX_OFFSET(n) + 0x4 * (p))

/* SUNXI_MSGBOX_READ_IRQ_ENABLE */
#define RD_IRQ_EN_MASK			0x1
#define RD_IRQ_EN_SHIFT(p)		((p) * 2)

/* SUNXI_MSGBOX_READ_IRQ_STATUS */
#define RD_IRQ_PEND_MASK		0x1
#define RD_IRQ_PEND_SHIFT(p)		((p) * 2)

/* SUNXI_MSGBOX_WRITE_IRQ_ENABLE */
#define WR_IRQ_EN_MASK			0x1
#define WR_IRQ_EN_SHIFT(p)		((p) * 2 + 1)

/* SUNXI_MSGBOX_WRITE_IRQ_STATUS */
#define WR_IRQ_PEND_MASK		0x1
#define WR_IRQ_PEND_SHIFT(p)		((p) * 2 + 1)

/* SUNXI_MSGBOX_MSG_STATUS */
#define MSG_NUM_MASK			0xF
#define MSG_NUM_SHIFT			0

/* SUNXI_MSGBOX_WRITE_IRQ_THRESHOLD */
#define WR_IRQ_THR_MASK			0x3
#define WR_IRQ_THR_SHIFT		0

/*
 * AW msgbox hardware data information
 * Each msgox can be used for RX by current processor, it can trigger
 * remote processor interrupt to notify remote processor and can receive
 * interrupt if has incoming message.
 *
 * @processors_max:	The number of process that have msgbox controller in this SOC
 * @channels_max:	The number of channels in every msgbox controller
 * @fifo_msg_max:	The depth of FIFO
 * @mbox_num_chans:	All the num of channels in this SOC, mbox_num_chans = (processors_max - 1) * channels_max
 * @to_coef_n:		The coef N, N = to_coef_n[local_id][remote_id]
 * @to_remote_id:	The remote process id for current process, remote_id = to_coef_n[local_id][coef_n]
 */
struct sunxi_msgbox_hwdata {
	int processors_max;
	int channels_max;
	int fifo_msg_max;
	int mbox_num_chans;
	int to_coef_n[4][4];
	int to_remote_id[4][4];
};

#if IS_ENABLED(CONFIG_PM)
/* Registers which needs to be saved and restored before and after sleeping */
static u32 sunxi_msgbox_regs_offset[] = {
	SUNXI_MSGBOX_READ_IRQ_ENABLE(0),
	SUNXI_MSGBOX_WRITE_IRQ_ENABLE(0),
	SUNXI_MSGBOX_READ_IRQ_ENABLE(1),
	SUNXI_MSGBOX_WRITE_IRQ_ENABLE(1),
	SUNXI_MSGBOX_READ_IRQ_ENABLE(2),
	SUNXI_MSGBOX_WRITE_IRQ_ENABLE(2),
};
#endif /* CONFIG_PM  */

/**
 * AW msgbox controller data
 *
 * msbox controller support allocate channel for message transferring.
 *
 * @controller:	Representation of a communication channel controller
 * @hwdata:	Msgbox hardware data
 * @pdev:	Platform device
 * @dev:	Device to which it is attached
 * fllowing all of dts data
 * @res:	Base address of the register mapping region
 * @clk:	The msgbox working clk
 * @reset:	The msgbox reset clk
 * @irq:	The msgbox irq num for all the msgbox controller in this SOC
 * @irq_cnt:	The msgbox irq num, irq_cnt should equal to hwdata->processors_max
 * @local_id:	Curren process id num for all msgbox controller
 * @regs_backup Save msgbox status register values during sleep
 * @base_addr:	Base address of the register mapping region
 */
struct sunxi_msgbox {
	struct mbox_controller controller;
	struct sunxi_msgbox_hwdata *hwdata;
	struct platform_device *pdev;
	struct device *dev;
	struct resource *res;
	struct clk *clk;
	struct reset_control *reset;
	int *irq;
	int irq_cnt;
	int local_id;
#if IS_ENABLED(CONFIG_PM)
	u32 regs_backup[ARRAY_SIZE(sunxi_msgbox_regs_offset)];
#endif /* CONFIG_PM  */
	void __iomem *base_addr[0];
};

static bool sunxi_msgbox_peek_data(struct mbox_chan *chan);

static void reg_val_update(void __iomem *reg, u32 mask, u32 shift, u32 val)
{
	u32 reg_val;

	reg_val = readl(reg);
	reg_val &= ~(mask << shift);
	reg_val |= ((val & mask) << shift);
	writel(reg_val, reg);
}

/*
 * For local processor, mailbox channel index is defined from coefficient N(local_n remote_n)
 * and coefficient P(the used channel num). eg:
 *	arm to dsp
 *	- local_id:  0
 *	- local_n:   1(DSP->ARM)
 *	- remote_id: 1
 *	- remote_n:  0(ARM->DSP)
 *
 *	mailbox channel index(in dts:<&msgbox x>) = local_n * SUNXI_MSGBOX_CHANNELS_MAX + P
 */
static inline void mbox_chan_id_to_coef_n_p(struct sunxi_msgbox *chip, int mbox_chan_id, int *local_n, int *coef_p)
{
	*local_n = mbox_chan_id / chip->hwdata->channels_max;
	*coef_p = mbox_chan_id % chip->hwdata->channels_max;
}

static inline void mbox_chan_to_coef_n_p(struct sunxi_msgbox *chip, struct mbox_chan *chan, int *coef_n, int *coef_p)
{
	/* chan is an element in the array chans[] --> assignment in the of_xlate */
	mbox_chan_id_to_coef_n_p(chip, chan - chan->mbox->chans, coef_n, coef_p);
}

/*
 * when sender_id is local_id  and receiver_id is remote_id, return local_n
 * when sender_id is remote_id and receiver_id is local_id,  return remote_n
 * */
static inline int sunxi_msgbox_coef_n(struct sunxi_msgbox *chip, int sender_id, int receiver_id)
{
	BUG_ON(chip->hwdata->to_coef_n[sender_id][receiver_id] == -1);
	return chip->hwdata->to_coef_n[sender_id][receiver_id];
}

static inline int sunxi_msgbox_remote_id(struct sunxi_msgbox *chip, int local_id, int coef_n)
{
	BUG_ON(chip->hwdata->to_remote_id[local_id][coef_n] == -1);
	return chip->hwdata->to_remote_id[local_id][coef_n];
}

static void *sunxi_msgbox_reg_base(struct sunxi_msgbox *chip, int index)
{
	void *base;

	BUG_ON(index >= chip->hwdata->processors_max);
	base = chip->base_addr[index];
	BUG_ON(!base);

	return base;
}

static inline struct sunxi_msgbox *to_sunxi_msgbox(struct mbox_chan *chan)
{
	return chan->con_priv;
}

/* set the msgbox's fifo trigger level */
static void sunxi_msgbox_set_write_irq_threshold(struct sunxi_msgbox *chip,
							void __iomem *base, int n, int p,
							int threshold)
{
	u32 thr_val;
	void __iomem *reg = base + SUNXI_MSGBOX_WRITE_IRQ_THRESHOLD(n, p);

	switch (threshold) {
	case 8:
		thr_val = 3;
		break;
	case 4:
		thr_val = 2;
		break;
	case 2:
		thr_val = 1;
		break;
	case 1:
		thr_val = 0;
		break;
	default:
		dev_warn(chip->dev, "Invalid write irq threshold (%d). Use 1 instead\n", threshold);
		thr_val = 0;
		break;
	}

	reg_val_update(reg, WR_IRQ_THR_MASK, WR_IRQ_THR_SHIFT, thr_val);
}

static void sunxi_msgbox_read_handler(struct sunxi_msgbox *chip, struct mbox_chan *chan,
				  void __iomem *base, int local_n, int p)
{
	unsigned long timeout = jiffies + msecs_to_jiffies(10);
	u32 msg;
	int ret = -1;

	while (sunxi_msgbox_peek_data(chan) && time_before(jiffies, timeout)) {
		msg = readl(base + SUNXI_MSGBOX_MSG_FIFO(local_n, p));
		dev_dbg(chip->dev, "process-%d read data [0x%x] by channel %d from processor-%d success\n",
				chip->local_id, msg, p, sunxi_msgbox_remote_id(chip, chip->local_id, local_n));
		mbox_chan_received_data(chan, &msg);
		ret = 0;
	}
	if (ret)
		dev_err(chip->dev, "read data timeout\n");

	/* The IRQ pending can be cleared only once the FIFO is empty. */
	set_bits(base + SUNXI_MSGBOX_READ_IRQ_STATUS(local_n), RD_IRQ_PEND_MASK << RD_IRQ_PEND_SHIFT(p));
}

#if IS_ENABLED(CONFIG_AW_MAILBOX_SUPPORT_TXDONE_IRQ)
static void sunxi_msgbox_write_handler(struct sunxi_msgbox *chip, struct mbox_chan *chan,
					void __iomem *base, int remote_n, int p)
{
	/*
	 * In msgbox hardware, the write IRQ will be triggered if the empty
	 * level in FIFO reaches the write IRQ threshold. It means that there
	 * is empty space in FIFO for local processor to write. Here we use
	 * the write IRQ to indicate TX is done, to ensure that there is empty
	 * space in FIFO for next message to send.
	 */

	/* Disable write IRQ */
	clear_field(base + SUNXI_MSGBOX_WRITE_IRQ_ENABLE(remote_n), WR_IRQ_EN_MASK << WR_IRQ_EN_SHIFT(p));

	mbox_chan_txdone(chan, 0);

	dev_dbg(chip->dev, "process-%d write data by channel %d to processor-%d done\n", remote_n, p, chip->local_id);

	/* Clear write IRQ pending */
	set_bits(base + SUNXI_MSGBOX_WRITE_IRQ_STATUS(remote_n), WR_IRQ_PEND_MASK << WR_IRQ_PEND_SHIFT(p));
}
#endif

static irqreturn_t sunxi_msgbox_handler(int irq, void *dev_id)
{
	struct sunxi_msgbox *chip = dev_id;
	struct mbox_chan *chan;
	int local_id, local_n, remote_id, remote_n, p;
	void __iomem *read_reg_base;
	void __iomem *write_reg_base;
	u32 read_irq_en, read_irq_pending;
	u32 write_irq_en, write_irq_pending;
	int i;

	for (i = 0; i < chip->hwdata->mbox_num_chans; i++) {
		chan = &chip->controller.chans[i];

		mbox_chan_id_to_coef_n_p(chip, i, &local_n, &p);
		local_id = chip->local_id;
		remote_id = sunxi_msgbox_remote_id(chip, local_id, local_n);
		remote_n = sunxi_msgbox_coef_n(chip, remote_id, local_id);

		read_reg_base = sunxi_msgbox_reg_base(chip, local_id);
		write_reg_base = sunxi_msgbox_reg_base(chip, remote_id);

		read_irq_en = get_field(
				read_reg_base + SUNXI_MSGBOX_READ_IRQ_ENABLE(local_n),
				RD_IRQ_EN_MASK << RD_IRQ_EN_SHIFT(p));
		read_irq_pending = get_field(
				read_reg_base + SUNXI_MSGBOX_READ_IRQ_STATUS(local_n),
				RD_IRQ_PEND_MASK << RD_IRQ_PEND_SHIFT(p));
		write_irq_en = get_field(
				write_reg_base + SUNXI_MSGBOX_WRITE_IRQ_ENABLE(remote_n),
				WR_IRQ_EN_MASK << WR_IRQ_EN_SHIFT(p));
		write_irq_pending = get_field(
				write_reg_base + SUNXI_MSGBOX_WRITE_IRQ_STATUS(remote_n),
				WR_IRQ_PEND_MASK << WR_IRQ_PEND_SHIFT(p));

		if (read_irq_en && read_irq_pending)
			sunxi_msgbox_read_handler(chip, chan, read_reg_base, local_n, p);

#if IS_ENABLED(CONFIG_AW_MAILBOX_SUPPORT_TXDONE_IRQ)
		if (write_irq_en && write_irq_pending)
			sunxi_msgbox_write_handler(chip, chan, write_reg_base, remote_n, p);
#endif
	}

	return IRQ_HANDLED;
}

static int sunxi_msgbox_startup(struct mbox_chan *chan)
{
	struct sunxi_msgbox *chip = to_sunxi_msgbox(chan);
	int local_id, remote_id, local_n, remote_n, p;
	void __iomem *read_reg_base;
	void __iomem *write_reg_base;
	unsigned long timeout = jiffies + msecs_to_jiffies(10);

	mbox_chan_to_coef_n_p(chip, chan, &local_n, &p);
	local_id = chip->local_id;
	remote_id = sunxi_msgbox_remote_id(chip, local_id, local_n);
	remote_n = sunxi_msgbox_coef_n(chip, remote_id, local_id);
	read_reg_base = sunxi_msgbox_reg_base(chip, local_id);
	write_reg_base = sunxi_msgbox_reg_base(chip, remote_id);

	/* Flush read FIFO before transfer */
	while (sunxi_msgbox_peek_data(chan) && time_before(jiffies, timeout))
		readl(read_reg_base + SUNXI_MSGBOX_MSG_FIFO(local_n, p));

	/* Clear read IRQ pending before transfer */
	set_bits(read_reg_base + SUNXI_MSGBOX_READ_IRQ_STATUS(local_n), RD_IRQ_PEND_MASK << RD_IRQ_PEND_SHIFT(p));

	/* Enable read IRQ */
	set_bits(read_reg_base + SUNXI_MSGBOX_READ_IRQ_ENABLE(local_n), RD_IRQ_EN_MASK << RD_IRQ_EN_SHIFT(p));

	/* Clear remote process's write IRQ pending */
	set_bits(write_reg_base + SUNXI_MSGBOX_WRITE_IRQ_STATUS(remote_n), WR_IRQ_PEND_MASK << WR_IRQ_PEND_SHIFT(p));

#if IS_ENABLED(CONFIG_AW_MAILBOX_SUPPORT_TXDONE_IRQ)
	/*
	 * Enable write IRQ after writing message to FIFO
	 * Because we use the write IRQ to indicate whether FIFO has empty space for "next message"
	 * rather than "this message" to send.
	 */
	set_bits(write_reg_base + SUNXI_MSGBOX_WRITE_IRQ_ENABLE(remote_n), WR_IRQ_EN_MASK << WR_IRQ_EN_SHIFT(p));
#endif

	/*
	 * Configure the FIFO empty level to trigger the write IRQ to 1.
	 * It means that if the write IRQ is enabled, once the FIFO is not full,
	 * the write IRQ will be triggered.
	 */
	sunxi_msgbox_set_write_irq_threshold(chip, write_reg_base, remote_n, p, 1);

	dev_dbg(chip->dev, "process-%d --> proccss-%d by channel %d startup complete\n",
			local_id, remote_id, p);
	return 0;
}

static int sunxi_msgbox_send_data(struct mbox_chan *chan, void *data)
{
	struct sunxi_msgbox *chip = to_sunxi_msgbox(chan);
	int local_id = chip->local_id;
	int local_n, remote_id, remote_n, p;
	u32 remaining_space_in_fifo, msg;
	void __iomem *write_reg_base; /* the base addr of the msgbox controller that you want to send */

	/*
	 * Here we consider the data is always a pointer to u32.
	 * Should we define a data structure, e.g. 'struct sunxi_mailbox_message',
	 * to hide the actual data type for mailbox client users?
	 */
	msg = *(u32 *)data;

	mbox_chan_to_coef_n_p(chip, chan, &local_n, &p);
	remote_id = sunxi_msgbox_remote_id(chip, local_id, local_n);
	remote_n = sunxi_msgbox_coef_n(chip, remote_id, local_id);
	write_reg_base = sunxi_msgbox_reg_base(chip, remote_id);

	/*
	 * Check whether FIFO is full.
	 *
	 * Ordinarily the 'tx_done' of previous message ensures the FIFO has
	 * empty space for this message to send. But in case the FIFO is already
	 * full before sending the first message, we check the number of messages
	 * in FIFO anyway.
	 */
	remaining_space_in_fifo = chip->hwdata->fifo_msg_max - get_field(write_reg_base + SUNXI_MSGBOX_MSG_STATUS(remote_n, p), MSG_NUM_MASK);
	if (remaining_space_in_fifo <= 0) {
		dev_err(chip->dev, "Channel %d to processor %d: FIFO is full\n", p, remote_id);
		return -EBUSY;
	}

	/* Write message to remote process's msgbox controller's FIFO */
	writel(msg, write_reg_base + SUNXI_MSGBOX_MSG_FIFO(remote_n, p));

	dev_dbg(chip->dev, "processor-%d use channel %d send data [0x%x] to processor-%d success\n",
			local_id, p, msg, remote_id);
	return 0;
}

static void sunxi_msgbox_shutdown(struct mbox_chan *chan)
{
	struct sunxi_msgbox *chip = to_sunxi_msgbox(chan);
	int local_id, remote_id;
	int local_n, remote_n, p;
	void __iomem *read_reg_base;
	void __iomem *write_reg_base;
	unsigned long timeout = jiffies + msecs_to_jiffies(10);

	mbox_chan_to_coef_n_p(chip, chan, &local_n, &p);
	local_id = chip->local_id;
	remote_id = sunxi_msgbox_remote_id(chip, local_id, local_n);
	remote_n = sunxi_msgbox_coef_n(chip, remote_id, local_id);
	read_reg_base = sunxi_msgbox_reg_base(chip, local_id);
	write_reg_base = sunxi_msgbox_reg_base(chip, remote_id);

	/* Disable the write IRQ */
	clear_field(write_reg_base + SUNXI_MSGBOX_WRITE_IRQ_ENABLE(remote_n), WR_IRQ_EN_MASK << WR_IRQ_EN_SHIFT(p));
	/* Clear write IRQ pending */
	set_bits(write_reg_base + SUNXI_MSGBOX_WRITE_IRQ_STATUS(remote_n), WR_IRQ_PEND_MASK << WR_IRQ_PEND_SHIFT(p));

	/* Attempt to flush the receive FIFO until the IRQ is cleared. */
	do {
		while (sunxi_msgbox_peek_data(chan) && time_before(jiffies, timeout))
			readl(read_reg_base + SUNXI_MSGBOX_MSG_FIFO(local_n, p));
		/* Disable the read IRQ */
		clear_field(read_reg_base + SUNXI_MSGBOX_READ_IRQ_ENABLE(local_n),
				RD_IRQ_EN_MASK << RD_IRQ_EN_SHIFT(p));
		/* Clear the read IRQ pending */
		set_bits(read_reg_base + SUNXI_MSGBOX_READ_IRQ_STATUS(local_n),
				RD_IRQ_PEND_MASK << RD_IRQ_PEND_SHIFT(p));
	} while (get_field(read_reg_base + SUNXI_MSGBOX_READ_IRQ_STATUS(local_n),
				RD_IRQ_PEND_MASK << RD_IRQ_PEND_SHIFT(p)) && time_before(jiffies, timeout));

	dev_dbg(chip->dev, "process-%d --> process-%d by channel %d shutdown complete\n", local_id, remote_id, p);
}

#if !IS_ENABLED(CONFIG_AW_MAILBOX_SUPPORT_TXDONE_IRQ)
static bool sunxi_msgbox_last_tx_done(struct mbox_chan *chan)
{
	/*
	 * Here we consider a transmission is done if the FIFO is not full.
	 * This ensures that the next message can be written to FIFO, and local
	 * processor need not to wait until remote processor has read this
	 * message from FIFO.
	 */
	struct sunxi_msgbox *chip = to_sunxi_msgbox(chan);
	int local_id = chip->local_id;
	int local_n, remote_id, remote_n, p;
	void __iomem *write_reg_base;
	void __iomem *status_reg;
	u32 msg_num;

	mbox_chan_to_coef_n_p(chip, chan, &local_n, &p);
	remote_id = sunxi_msgbox_remote_id(chip, local_id, local_n);
	remote_n = sunxi_msgbox_coef_n(chip, remote_id, local_id);


	write_reg_base = sunxi_msgbox_reg_base(chip, remote_id);
	status_reg = write_reg_base + SUNXI_MSGBOX_MSG_STATUS(remote_n, p);
	msg_num = get_field(status_reg, MSG_NUM_MASK);

	return msg_num < chip->hwdata->fifo_msg_max ? true : false;
}
#endif

static bool sunxi_msgbox_peek_data(struct mbox_chan *chan)
{
	struct sunxi_msgbox *chip = to_sunxi_msgbox(chan);
	int local_n, p;
	u32 msg_num;
	void __iomem *status_reg;

	mbox_chan_to_coef_n_p(chip, chan, &local_n, &p);
	status_reg = sunxi_msgbox_reg_base(chip, chip->local_id) + SUNXI_MSGBOX_MSG_STATUS(local_n, p);
	msg_num = get_field(status_reg, MSG_NUM_MASK);

	return !!msg_num;
}

/*
 * Each maibox channel is bidirectional and can send and receive data
 * when local process read from remote process use the channel of local process msgbox ctroller
 * when local process write data to remote process use the channel of remote process msgbox ctroller
 * */
static struct mbox_chan *sunxi_mbox_xlate(struct mbox_controller *mbox, const struct of_phandle_args *sp)
{
	int ind = sp->args[0];
	if (ind >= mbox->num_chans)
		return ERR_PTR(-EINVAL);

	return &mbox->chans[ind];
}

static const struct mbox_chan_ops sunxi_msgbox_chan_ops = {
	.startup      = sunxi_msgbox_startup,
	.send_data    = sunxi_msgbox_send_data,
	.shutdown     = sunxi_msgbox_shutdown,
#if IS_ENABLED(CONFIG_AW_MAILBOX_SUPPORT_TXDONE_IRQ)
	.last_tx_done = NULL,
#else
	.last_tx_done = sunxi_msgbox_last_tx_done,
#endif
	.peek_data    = sunxi_msgbox_peek_data,
};

/* Note:
 * When support new platform, msgbox driver maintainers need to
 * add coefficients N, remote_id and local_id table according to
 * spec.
 *
 * The '-1' in the table means Allwinnertech msgbox not supporting.
 *
 * In sunxi msgbox, the coefficient N represents "the index of other processor
 * that communicates with local processor". For one specific local processor,
 * different coefficient N means differnt remote processor.
 *
 * sun8iw20:
 *		   ARM:0,DSP:1,CPUS:2
 *     --------------------------------------------
 *         local_id        N        remote_id
 *     --------------------------------------------
 *             0           0            1
 *             0           1            2
 *     --------------------------------------------
 *             1           0            0
 *             1           1            2
 *     --------------------------------------------
 *             2           0            0
 *             2           1            1
 *     --------------------------------------------
 */
static const struct sunxi_msgbox_hwdata sun8iw20_hwdata = {
	.processors_max = 3,
	.channels_max = 4,
	.fifo_msg_max = 8,
	.mbox_num_chans = 8,
	.to_coef_n = {
		{-1, 0, 1, -1},
		{0, -1, 1, -1},
		{0, 1, -1, -1},
		{-1, -1, -1, -1}
	},
	.to_remote_id = {
		{1, 2, -1, -1},
		{0, 2, -1, -1},
		{0, 1, -1, -1},
		{-1, -1, -1, -1}
	}
};

/* sun55iw3:
 *		ARM:0,DSP:1,CPUS:2,RV:3
 *    --------------------------------------------
 *         local_id        N        remote_id
 *     --------------------------------------------
 *             0           1            1
 *             0           0            2
 *             0	   2		3
 *     --------------------------------------------
 *             1           0            0
 *             1           1            2
 *             1	   2		3
 *     --------------------------------------------
 *             2           0            0
 *             2           1            1
 *             2	   2		3
 *     --------------------------------------------
 *	       3	   2		0
 *	       3	   1		1
 *	       3	   0		2
 *     --------------------------------------------
 */
static const struct sunxi_msgbox_hwdata sun55iw3_hwdata = {
	.processors_max = 4,
	.channels_max = 4,
	.fifo_msg_max = 8,
	.mbox_num_chans = 12,
	.to_coef_n = {
		{-1, 1, 0, 2},
		{0, -1, 1, 2},
		{0, 1, -1, 2},
		{2, 1, 0, -1}
	},
	.to_remote_id = {
		{2, 1, 3, -1},
		{0, 2, 3, -1},
		{0, 1, 3, -1},
		{2, 1, 0, -1},
	}
};

/* sun55iw6:
 *		ARM:0,CPUS:1,RV:2
 *    --------------------------------------------
 *         local_id        N        remote_id
 *     --------------------------------------------
 *             0           0            1
 *             0           1            2
 *     --------------------------------------------
 *             1           0            0
 *             1           1            2
 *     --------------------------------------------
 *             2           0            0
 *             2           1            1
 *     --------------------------------------------
 */
static const struct sunxi_msgbox_hwdata sun55iw6_hwdata = {
	.processors_max = 3,
	.channels_max = 4,
	.fifo_msg_max = 8,
	.mbox_num_chans = 8,
	.to_coef_n = {
		{-1, 0, 1, -1},
		{0, -1, 1, -1},
		{0, 1, -1, -1},
		{-1, -1, -1, -1}
	},
	.to_remote_id = {
		{1, 2, -1, -1},
		{0, 2, -1, -1},
		{0, 1, -1, -1},
		{-1, -1, -1, -1},
	}
};

/* sun60iw1:
 *		ARM:0,CPUS:1,DSP:2,RV:3
 *    --------------------------------------------
 *         local_id        N        remote_id
 *     --------------------------------------------
 *             0           0            1
 *             0           1            2
 *             0	   2		3
 *     --------------------------------------------
 *             1           0            0
 *             1           1            2
 *             1	   2		3
 *     --------------------------------------------
 *             2           0            0
 *             2           1            1
 *             2	   2		3
 *     --------------------------------------------
 *	       3	   0		0
 *	       3	   1		1
 *	       3	   2		2
 *     --------------------------------------------
 */
static const struct sunxi_msgbox_hwdata sun60iw1_hwdata = {
	.processors_max = 4,
	.channels_max = 4,
	.fifo_msg_max = 8,
	.mbox_num_chans = 12,
	.to_coef_n = {
		{-1, 0, 1, 2},
		{0, -1, 1, 2},
		{0, 1, -1, 2},
		{0, 1, 2, -1}
	},
	.to_remote_id = {
		{1, 2, 3, -1},
		{0, 2, 3, -1},
		{0, 1, 3, -1},
		{0, 1, 2, -1},
	}
};

/* sun55iw5:
 *		ARM:0,CPUS:1
 *    --------------------------------------------
 *         local_id        N        remote_id
 *     --------------------------------------------
 *             0           0            1
 *     --------------------------------------------
 *             1           0            0
 *     --------------------------------------------
 */
static const struct sunxi_msgbox_hwdata sun55iw5_hwdata = {
	.processors_max = 2,
	.channels_max = 4,
	.fifo_msg_max = 8,
	.mbox_num_chans = 4,
	.to_coef_n = {
		{-1, 0, -1, -1},
		{0, -1, -1, -1},
		{-1, -1, -1, -1},
		{-1, -1, -1, -1}
	},
	.to_remote_id = {
		{1, -1, -1, -1},
		{0, -1, -1, -1},
		{-1, -1, -1, -1},
		{-1, -1, -1, -1},
	}
};

static const struct of_device_id sunxi_msgbox_of_match[] = {
	{ .compatible = "allwinner,sun8iw20-msgbox", .data = &sun8iw20_hwdata},
	{ .compatible = "allwinner,sun55iw3-msgbox", .data = &sun55iw3_hwdata},
	{ .compatible = "allwinner,sun55iw6-msgbox", .data = &sun55iw6_hwdata},
	{ .compatible = "allwinner,sun60iw1-msgbox", .data = &sun60iw1_hwdata},
	/* The msgbox resources of sun60iw2 and sun55iw5 are the same, so use sun55iw5_hwdata uniformly */
	{ .compatible = "allwinner,sun55iw5-msgbox", .data = &sun55iw5_hwdata},
	{ .compatible = "allwinner,sun60iw2-msgbox", .data = &sun55iw5_hwdata},
	{ .compatible = "allwinner,sun8iw21-msgbox", .data = &sun55iw5_hwdata},
	{},
};
MODULE_DEVICE_TABLE(of, sunxi_msgbox_of_match);

static void sunxi_msgbox_disable_irq(struct sunxi_msgbox *chip)
{
	int i, j;

	for (i = 0; i < chip->hwdata->processors_max - 1; i++) {
		int local_n = i;
		int local_id = chip->local_id;
		int remote_id = sunxi_msgbox_remote_id(chip, local_id, local_n);
		int remote_n = sunxi_msgbox_coef_n(chip, remote_id, local_id);
		void __iomem *read_reg_base = sunxi_msgbox_reg_base(chip, local_id);
		void __iomem *write_reg_base = sunxi_msgbox_reg_base(chip, remote_id);

		void __iomem *read_irq_en_reg = read_reg_base +
			SUNXI_MSGBOX_READ_IRQ_ENABLE(local_n);
		void __iomem *write_irq_en_reg = write_reg_base +
			SUNXI_MSGBOX_WRITE_IRQ_ENABLE(remote_n);

		u32 read_irq_en_value = readl(read_irq_en_reg);
		u32 write_irq_en_value = readl(write_irq_en_reg);

		for (j = 0; j < chip->hwdata->channels_max; ++j) {
			read_irq_en_value &= ~(RD_IRQ_EN_MASK << RD_IRQ_EN_SHIFT(j));
			write_irq_en_value &= ~(WR_IRQ_EN_MASK << WR_IRQ_EN_SHIFT(j));
		}

		writel(read_irq_en_value, read_irq_en_reg);
		writel(write_irq_en_value, write_irq_en_reg);
	}
}

static int sunxi_msgbox_resource_get(struct sunxi_msgbox *chip)
{
	int ret, i;

	chip->clk = devm_clk_get(chip->dev, "msgbox");
	if (IS_ERR_OR_NULL(chip->clk)) {
		dev_err(chip->dev, "Error: Failed to get clock\n");
		return PTR_ERR(chip->clk);
	}

	chip->reset = devm_reset_control_get(chip->dev, NULL);
	if (IS_ERR_OR_NULL(chip->reset)) {
		ret = PTR_ERR(chip->reset);
		dev_err(chip->dev, "Error: Failed to get reset control: %d\n", ret);
		return ret;
	}

	for (i = 0; i < chip->hwdata->processors_max; i++) {
		chip->res = platform_get_resource(chip->pdev, IORESOURCE_MEM, i);
		if (!chip->res) {
			dev_err(chip->dev, "Error: Failed to get resource %d\n", i);
			return -ENODEV;
		}
		chip->base_addr[i] = devm_ioremap_resource(chip->dev, chip->res);
		if (IS_ERR(chip->base_addr[i])) {
			ret = PTR_ERR(chip->base_addr[i]);
			dev_err(chip->dev, "Error: Failed to map resource %d: %d\n", i, ret);
			return ret;
		}
	}

	ret = of_property_read_u32(chip->dev->of_node, "local_id", &chip->local_id);
	if (ret) {
		dev_err(chip->dev, "Error: Failed to get local_id\n");
		return ret;
	}

	chip->irq_cnt = of_irq_count(chip->dev->of_node);
	chip->irq = devm_kcalloc(chip->dev, chip->irq_cnt, sizeof(int), GFP_KERNEL);
	if (!chip->irq)
		return -ENOMEM;

	for (i = 0; i < chip->irq_cnt; i++) {
		ret = of_irq_get(chip->dev->of_node, i);
		if (ret < 0) {
			dev_err(chip->dev, "of_irq_get [%d] failed: %d\n", i, ret);
			return ret;
		} else if (ret == 0) {
			dev_err(chip->dev, "of_irq_get [%d] mapping failure\n", i);
			return -EINVAL;
		}
		chip->irq[i] = ret;
	}

	for (i = 0; i < chip->irq_cnt; i++) {
		ret = devm_request_irq(&chip->pdev->dev, chip->irq[i], sunxi_msgbox_handler, 0,
					dev_name(chip->dev), chip);
		if (ret) {
			dev_err(chip->dev, "Error: Failed to reuqest IRQ handler %d: %d\n", i, ret);
			return ret;
		}
	}

	return 0;
}

static void sunxi_msgbox_resource_put(struct sunxi_msgbox *chip)
{
	/* add you want here with sunxi_msgbox_resource_get() in the future  */
	return ;
}

static int sunxi_msgbox_hw_init(struct sunxi_msgbox *chip)
{
	int ret;

	ret = reset_control_deassert(chip->reset);
	if (ret) {
		dev_err(chip->dev, "Error: Failed to deassert reset clk: %d\n", ret);
		return ret;
	}

	ret = clk_prepare_enable(chip->clk);
	if (ret) {
		dev_err(chip->dev, "Error: Failed to enable clock: %d\n", ret);
		return ret; /* reset_control_deassert no need free */
	}

	/* Disable all IRQs for every msgbox */
	sunxi_msgbox_disable_irq(chip);

	return 0;
}

static void sunxi_msgbox_hw_deinit(struct sunxi_msgbox *chip)
{
	clk_disable_unprepare(chip->clk);
}

static int sunxi_msgbox_probe(struct platform_device *pdev)
{
	struct mbox_chan *chans;
	struct sunxi_msgbox *chip;
	struct sunxi_msgbox_hwdata *priv_data;
	int i, ret, processors_max_tmp;

	dev_info(&pdev->dev, "%s(): sunxi msgbox start probe\n", __func__);
	priv_data = (struct sunxi_msgbox_hwdata *)of_device_get_match_data(&pdev->dev);
	processors_max_tmp = priv_data->processors_max;
	chip = devm_kzalloc(&pdev->dev, sizeof(*chip) + (sizeof(void __iomem *) * processors_max_tmp), GFP_KERNEL);
	if (!chip)
		return -ENOMEM;

	chip->hwdata = priv_data;
	chip->pdev = pdev;
	chip->dev = &pdev->dev;

	chans = devm_kcalloc(chip->dev, chip->hwdata->mbox_num_chans, sizeof(*chans), GFP_KERNEL);
	if (!chans) {
		ret = -ENOMEM;
		return ret;
	}
	for (i = 0; i < chip->hwdata->mbox_num_chans; i++)
		chans[i].con_priv = chip;

	ret = sunxi_msgbox_resource_get(chip);
	if (ret) {
		dev_err(chip->dev, "Error: Failed to get resource\n");
		return ret;
	}

	ret = sunxi_msgbox_hw_init(chip);
	if (ret) {
		dev_err(chip->dev, "Error: Failed to init hardware\n");
		goto err0;
	}

	chip->controller.dev		= chip->dev;
	chip->controller.ops		= &sunxi_msgbox_chan_ops;
	chip->controller.chans		= chans;
	chip->controller.num_chans	= chip->hwdata->mbox_num_chans;
	chip->controller.of_xlate	= sunxi_mbox_xlate;
#if IS_ENABLED(CONFIG_AW_MAILBOX_SUPPORT_TXDONE_IRQ)
	chip->controller.txdone_irq	= true;
#else
	chip->controller.txdone_irq	= false;
	chip->controller.txdone_poll	= true;
	chip->controller.txpoll_period	= 5;
#endif
	platform_set_drvdata(pdev, chip);

	ret = devm_mbox_controller_register(chip->dev, &chip->controller);
	if (ret) {
		dev_err(chip->dev, "Error: Failed to register controller: %d\n", ret);
		goto err1;
	}

	dev_info(chip->dev, "%s(): sunxi msgbox probe success\n", __func__);
	return 0;

err1:
	sunxi_msgbox_hw_deinit(chip);

err0:
	sunxi_msgbox_resource_put(chip);
	return ret;
}

static int sunxi_msgbox_remove(struct platform_device *pdev)
{
	struct sunxi_msgbox *chip = platform_get_drvdata(pdev);

	sunxi_msgbox_hw_deinit(chip);
	sunxi_msgbox_resource_put(chip);

	return 0;
}

#if IS_ENABLED(CONFIG_PM)
static void sunxi_msgbox_save_regs(struct sunxi_msgbox *chip)
{
	int i;
	void __iomem *read_reg_base;

	read_reg_base = sunxi_msgbox_reg_base(chip, chip->local_id);
	for (i = 0; i < ARRAY_SIZE(sunxi_msgbox_regs_offset); i++)
		chip->regs_backup[i] = readl(read_reg_base + sunxi_msgbox_regs_offset[i]);
}

static void sunxi_msgbox_restore_regs(struct sunxi_msgbox *chip)
{
	int i;
	void __iomem *read_reg_base;

	read_reg_base = sunxi_msgbox_reg_base(chip, chip->local_id);
	for (i = 0; i < ARRAY_SIZE(sunxi_msgbox_regs_offset); i++)
		writel(chip->regs_backup[i], read_reg_base + sunxi_msgbox_regs_offset[i]);
}

static int sunxi_msgbox_suspend(struct device *dev)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct sunxi_msgbox *chip = platform_get_drvdata(pdev);

	/*
	 * When performing standby operations, CPUs call the device_wakeup_arm_wake_irqs() function,
	 * which is invoked later than the suspend() function. In addition, the clk and power of the
	 * msgbox will be turned off after the system sleeps, and it cannot be used after being reset
	 * after waking up. Therefore, during the standby process, there is no need to perform any
	 * operations other than saving/restoring register contents.
	 */
	sunxi_msgbox_save_regs(chip);

	return 0;
}

static int sunxi_msgbox_resume(struct device *dev)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct sunxi_msgbox *chip = platform_get_drvdata(pdev);

	sunxi_msgbox_restore_regs(chip);

	return 0;
}

static const struct dev_pm_ops sunxi_msgbox_dev_pm_ops = {
	.suspend = sunxi_msgbox_suspend,
	.resume = sunxi_msgbox_resume,
};

#define SUNXI_MSGBOX_DEV_PM_OPS (&sunxi_msgbox_dev_pm_ops)
#else
#define SUNXI_MSGBOX_DEV_PM_OPS NULL
#endif /* CONFIG_PM  */

static struct platform_driver sunxi_msgbox_driver = {
	.driver = {
		.name = "sunxi-msgbox",
		.of_match_table = sunxi_msgbox_of_match,
		.pm = SUNXI_MSGBOX_DEV_PM_OPS,
	},
	.probe  = sunxi_msgbox_probe,
	.remove = sunxi_msgbox_remove,
};

#ifdef CONFIG_AW_RPROC_FAST_BOOT
static int __init sunxi_mailbox_init(void)
{
    int ret;

    ret = platform_driver_register(&sunxi_msgbox_driver);

    return ret;
}

static void __exit sunxi_mailbox_exit(void)
{
    platform_driver_unregister(&sunxi_msgbox_driver);
}

postcore_initcall(sunxi_mailbox_init);
module_exit(sunxi_mailbox_exit);
#else
module_platform_driver(sunxi_msgbox_driver);
#endif

MODULE_DESCRIPTION("Sunxi Msgbox Driver");
MODULE_AUTHOR("wujiayi <wujiayi@allwinnertech.com>");
MODULE_AUTHOR("xuminghui <xuminghui@allwinnertech.com>");
MODULE_AUTHOR("zhaiyaya <zhaiyaya@allwinnertech.com>");
MODULE_LICENSE("GPL v2");
MODULE_VERSION("1.1.5");
