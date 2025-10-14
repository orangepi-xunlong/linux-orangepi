// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright 2024 Cix Technology Group Co., Ltd.
 */

#include <linux/err.h>
#include <linux/kernel.h>
#include <linux/hw_random.h>
#include <linux/io.h>
#include <linux/iopoll.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/random.h>

/**
 *  RNG Pool control registers
 */
#define CE_CFG_BASE_ADDR 0x05050000UL // 0xA0040000 - 0xA0000000 + 0x05010000
#define RNP_OFS 0x05055300UL // 0x3300 + (0x1000 * 2)
#define RNP_CTRL_OFS 0x00
#define RNP_STAT_OFS 0x04
#define RNP_INTR_OFS 0x08
#define RNP_INTR_MSK_OFS 0x0C
#define RNP_DAT_WORD0_OFS 0x10

#define RNP_STAT_AUTOCORR_TEST_ERR (1 << 0U)
#define RNP_MAX_SIZE (32U)
#define RNP_MIN(a, b) ((a) < (b) ? (a) : (b))

#define to_sky1_rng(p)	container_of(p, struct sky1_rng, rng)

struct sky1_rng {
	void __iomem *base;
	struct hwrng rng;
};

static inline void trng_rnp_fill(struct sky1_rng *hrng)
{
	uint32_t val;
	/** wait for rnp idle */
	readl_poll_timeout(hrng->base + RNP_STAT_OFS, val,
				 ((val & (0x6)) == 0x0), 100, 1000000);
	/** trigger to fill rnp */
	val = readl_relaxed(hrng->base + RNP_CTRL_OFS);
	val |= 0x3U;
	writel_relaxed(val, hrng->base + RNP_CTRL_OFS);

	/** check fill status */
	readl_poll_timeout(hrng->base + RNP_INTR_OFS, val,
				 ((val & (0x1)) == 0x1), 100, 1000000);

	/** eoi */
	writel_relaxed(val, hrng->base + RNP_INTR_OFS);
}

static inline void trng_rnp_read(struct sky1_rng *hrng, uint8_t *buf, size_t len)
{
	uint32_t val;
	size_t cp_len;
	size_t left_len = len;
	size_t i = 0;

	do {
		/** assert that len shouldn't over 32 bytes*/
		val = readl_relaxed(hrng->base + RNP_DAT_WORD0_OFS + (sizeof(val) * i++));
		cp_len = RNP_MIN(left_len, sizeof(val));
		memcpy(buf + len - left_len, &val, cp_len);
		left_len -= cp_len;
	} while (left_len > 0);
}

static inline void trng_read(struct sky1_rng *hrng, uint8_t *buf, size_t len)
{
	size_t left_len = len;
	size_t read_len;

	while (left_len > 0) {
		/** trigger RNP to fill random number pool */
		trng_rnp_fill(hrng);
		/** read appropriate size of data */
		read_len = RNP_MIN(left_len, RNP_MAX_SIZE);
		trng_rnp_read(hrng, buf + len - left_len, read_len);
		left_len -= read_len;
	}

	printk("trng_read %d:0x%x\n", len, buf[0]);
}

static int sky1_rng_init(struct hwrng *rng)
{
	return 0;
}

static void sky1_rng_cleanup(struct hwrng *rng)
{

}

static int sky1_rng_read(struct hwrng *rng, void *buf, size_t max, bool wait)
{
	struct sky1_rng *hrng = to_sky1_rng(rng);
	uint8_t *data = buf;

	trng_read(hrng, data, max);

	return max;
}

static int sky1_rng_probe(struct platform_device *pdev)
{
	struct sky1_rng *rng;
	int ret;

	rng = devm_kzalloc(&pdev->dev, sizeof(*rng), GFP_KERNEL);
	if (!rng)
		return -ENOMEM;

	platform_set_drvdata(pdev, rng);

	rng->base = devm_platform_ioremap_resource(pdev, 0);
	if (IS_ERR(rng->base))
		return PTR_ERR(rng->base);

	rng->rng.name = pdev->name;
	rng->rng.init = sky1_rng_init;
	rng->rng.cleanup = sky1_rng_cleanup;
	rng->rng.read = sky1_rng_read;
	rng->rng.quality = 20;

	printk("sky1_rng_probe\n");

	ret = devm_hwrng_register(&pdev->dev, &rng->rng);
	if (ret) {
		dev_err(&pdev->dev, "failed to register hwrng\n");
		return ret;
	}

	return 0;
}

static const struct of_device_id sky1_rng_dt_ids[] __maybe_unused = {
	{ .compatible = "cix,sky1-rng" },
	{ }
};
MODULE_DEVICE_TABLE(of, sky1_rng_dt_ids);

static struct platform_driver sky1_rng_driver = {
	.probe		= sky1_rng_probe,
	.driver		= {
		.name	= "sky1-rng",
		.of_match_table = of_match_ptr(sky1_rng_dt_ids),
	},
};

module_platform_driver(sky1_rng_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Abel.shuai <abel.shuai@cixtech>");
MODULE_DESCRIPTION("Cix random number generator driver");
