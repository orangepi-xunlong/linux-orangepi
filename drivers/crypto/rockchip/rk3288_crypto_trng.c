// SPDX-License-Identifier: GPL-2.0
/*
 * rk3288_crypto_trng.c - hardware cryptographic offloader for rockchip
 *
 * Copyright (C) 2022-2023 Corentin Labbe <clabbe@baylibre.com>
 *
 * This file handle the TRNG
 */
#include "rk3288_crypto.h"
#include <linux/hw_random.h>
#include <linux/iopoll.h>
#include <linux/pm_runtime.h>

static int rk3288_trng_read(struct hwrng *hwrng, void *data, size_t max, bool wait)
{
	struct rk_crypto_info *rk;
	unsigned int todo;
	int err = 0;
	int i;
	u32 v;

	rk = container_of(hwrng, struct rk_crypto_info, hwrng);

	todo = min_t(size_t, max, RK_CRYPTO_MAX_TRNG_BYTE);

#ifdef CONFIG_CRYPTO_DEV_ROCKCHIP_DEBUG
	rk->hwrng_stat_req++;
	rk->hwrng_stat_bytes += todo;
#endif

	err = pm_runtime_resume_and_get(rk->dev);
	if (err < 0)
		goto err_pm;

	mutex_lock(&rk->lock);

#define HIWORD_UPDATE(val, mask, shift) \
			((val) << (shift) | (mask) << ((shift) + 16))
	v = RK_CRYPTO_OSC_ENABLE | RK_CRYPTO_RNG_SAMPLE;
	CRYPTO_WRITE(rk, RK_CRYPTO_TRNG_CTRL, v);

	v = HIWORD_UPDATE(RK_CRYPTO_TRNG_START, RK_CRYPTO_TRNG_START, 0);
	CRYPTO_WRITE(rk, RK_CRYPTO_CTRL, v);
	wmb();

	err = readl_poll_timeout(rk->reg + RK_CRYPTO_CTRL, v,
				 !(v & RK_CRYPTO_TRNG_START),
				 100, 2000);
	if (err) {
		dev_err(rk->dev, "HWRNG read timeout");
		goto readfail;
	}

	for (i = 0; i < todo / 4; i++) {
		v = readl(rk->reg + RK_CRYPTO_TRNG_DOUT_0 + i * 4);
		put_unaligned_le32(v, data + i * 4);
	}

	err = todo;

	v = HIWORD_UPDATE(0, RK_CRYPTO_TRNG_START, 0);
	CRYPTO_WRITE(rk, RK_CRYPTO_CTRL, v);

readfail:
	mutex_unlock(&rk->lock);

	pm_runtime_put(rk->dev);

err_pm:
	return err;
}

int rk3288_hwrng_register(struct rk_crypto_info *rk)
{
	int ret;

	dev_info(rk->dev, "Register TRNG with sample=%d\n", RK_CRYPTO_RNG_SAMPLE);

	rk->hwrng.name = "Rockchip rk3288 TRNG";
	rk->hwrng.read = rk3288_trng_read;
	rk->hwrng.quality = 300;

	ret = hwrng_register(&rk->hwrng);
	if (ret)
		dev_err(rk->dev, "Fail to register the TRNG\n");
	return ret;
}

void rk3288_hwrng_unregister(struct rk_crypto_info *rk)
{
	hwrng_unregister(&rk->hwrng);
}
