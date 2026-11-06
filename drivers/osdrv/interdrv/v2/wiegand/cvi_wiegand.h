#ifndef __CVI_WIEGAND_H__
#define __CVI_WIEGAND_H__

#include <linux/cdev.h>
#include <linux/completion.h>
#include <linux/wait.h>
#include <linux/list.h>

#define TX_CONFIG0			0x00
#define TX_CONFIG1			0x04
#define TX_CONFIG2			0x08
#define TX_BUFFER			0x0C
#define TX_BUFFER1			0x10
#define TX_TRIGGER			0x14
#define TX_BUSY				0x18

#define RX_CONFIG0			0x20
#define RX_CONFIG1			0x24
#define RX_CONFIG2			0x28
#define RX_BUFFER			0x2C
#define RX_BUFFER1			0x30
#define RX_BUFFER_FLAG		0x34
#define RX_BUFFER_VALID		0x38
#define RX_BUFFER_CLEAR		0x3C

#define IRQ_ENABLE			0x44
#define IRQ_FLAG			0x48
#define IRQ_CLEAR			0x4C

struct cvi_wiegand_device {
	struct device *dev;
	int id;
	struct reset_control *rst_wiegand;
	struct clk *clk_wiegand;
	struct clk *clk_wiegand1;
	int clk_wiegand_en;
	dev_t cdev_id;
	struct cdev cdev;
	u8 __iomem *wiegand_vaddr;
	int wiegand_irq;
	spinlock_t close_lock;
	uint64_t tx_data;
	void *private_data;
};
#endif /* __CVI_WIEGAND_H__ */
