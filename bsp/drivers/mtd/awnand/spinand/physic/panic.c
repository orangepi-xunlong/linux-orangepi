/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */

#include <sunxi-log.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/mtd/aw-spinand.h>
#if IS_ENABLED(CONFIG_AW_SPINAND_ENABLE_PHY_CRC16)
#include <linux/crc16.h>
#endif
#include <linux/spi/spi.h>

#include "physic.h"

#if IS_ENABLED(CONFIG_SPI_SUNXI_ATOMIC_XFER)
extern int sunxi_spi_sync_atomic(struct spi_device *spi, struct spi_message *message);

int aw_spi_sync_atomic(struct spi_device *spi, struct spi_message *message)
{
	return sunxi_spi_sync_atomic(spi, message);
}
#else
int aw_spi_sync_atomic(struct spi_device *spi, struct spi_message *message)
{
	return spi_sync(spi, message);
}
#endif

int
aw_spi_sync_transfer_atomic(struct spi_device *spi, struct spi_transfer *xfers,
	unsigned int num_xfers)
{
	struct spi_message msg;

	spi_message_init_with_transfers(&msg, xfers, num_xfers);

#if IS_ENABLED(CONFIG_SPI_SUNXI_ATOMIC_XFER)
	return sunxi_spi_sync_atomic(spi, &msg);
#else
	sunxi_warn(NULL, "CONFIG_SPI_SUNXI_ATOMIC_XFER is disabled, can't guarantee write success\n");
	return spi_sync(spi, &msg);
#endif
}

int
aw_spi_write_atomic(struct spi_device *spi, const void *buf, size_t len)
{
	struct spi_transfer	t = {
			.tx_buf		= buf,
			.len		= len,
		};

#if IS_ENABLED(CONFIG_SPI_SUNXI_ATOMIC_XFER)
	return aw_spi_sync_transfer_atomic(spi, &t, 1);
#else
	sunxi_warn(NULL, "CONFIG_SPI_SUNXI_ATOMIC_XFER is disabled, can't guarantee write success\n");
	return spi_sync_transfer(spi, &t, 1);
#endif
}

int aw_spi_write_then_read_atomic(struct spi_device *spi,
		const void *txbuf, unsigned int n_tx,
		void *rxbuf, unsigned int n_rx)
{
#define	SPI_BUFSIZ	max(32, SMP_CACHE_BYTES)
	static DEFINE_MUTEX(lock);
	int			status;
	struct spi_message	message;
	struct spi_transfer	x[2];
	u8			*local_buf;

	if ((n_tx + n_rx) > SPI_BUFSIZ) {
		local_buf = kmalloc(max((unsigned int)SPI_BUFSIZ, n_tx + n_rx),
				    GFP_KERNEL | GFP_DMA);
		if (!local_buf)
			return -ENOMEM;
	} else {
			local_buf = kmalloc(SPI_BUFSIZ, GFP_KERNEL);
		if (!local_buf) {
			status = -ENOMEM;
			goto err0;
		}
	}

	spi_message_init(&message);
	memset(x, 0, sizeof(x));
	if (n_tx) {
		x[0].len = n_tx;
		spi_message_add_tail(&x[0], &message);
	}
	if (n_rx) {
		x[1].len = n_rx;
		spi_message_add_tail(&x[1], &message);
	}

	memcpy(local_buf, txbuf, n_tx);
	x[0].tx_buf = local_buf;
	x[1].rx_buf = local_buf + n_tx;

	/* do the i/o */
#if IS_ENABLED(CONFIG_SPI_SUNXI_ATOMIC_XFER)
	status = sunxi_spi_sync_atomic(spi, &message);
#else
	sunxi_warn(NULL, "CONFIG_SPI_SUNXI_ATOMIC_XFER is disabled, can't guarantee write success\n");
	status = spi_sync(spi, &message);
#endif
	if (status == 0)
		memcpy(rxbuf, x[1].rx_buf, n_rx);

	kfree(local_buf);

err0:
	return status;
}
