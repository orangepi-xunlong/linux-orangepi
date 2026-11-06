// SPDX-License-Identifier: GPL-2.0
/* Copyright (c) 2019 Sophgo Limited. */

#include <linux/acpi.h>
#include <linux/err.h>
#include <linux/hw_random.h>
#include <linux/io.h>
#include <linux/iopoll.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/random.h>
#include <linux/arm-smccc.h>

#define DW_TRNG_QUALITY		512

#if defined(CONFIG_ARM) || defined(__arm__) || defined(__aarch64__)

#define OPTEE_SMC_CALL_CV_TRNG_INIT		0x0300000B
#define OPTEE_SMC_CALL_CV_TRNG_READ	 0x0300000C

static int dw_trng_init(struct hwrng *rng)
{
	struct arm_smccc_res res = {0};

	arm_smccc_smc(OPTEE_SMC_CALL_CV_TRNG_INIT, 0, 0, 0, 0, 0, 0, 0, &res);
	if (res.a0 < 0) {
		pr_err("smc trng init failed\n");
		return -1;
	}

	return 0;
}

static int dw_trng_read(struct hwrng *rng, void *buf, size_t max, bool wait)
{
	int currsize = 0;
	u32 ret;
	struct arm_smccc_res res = {0};

	do {
		arm_smccc_smc(OPTEE_SMC_CALL_CV_TRNG_READ, 0, 0, 0, 0, 0, 0, 0, &res);

		pr_debug("a0 : 0x%lx\n", res.a0);
		pr_debug("a1 : 0x%lx\n", res.a1);
		pr_debug("a2 : 0x%lx\n", res.a2);
		pr_debug("a3 : 0x%lx\n", res.a3);

		ret = max - currsize;
		if (ret >= 16) {
			memcpy(buf + currsize, &res.a0, 4);
			memcpy(buf + currsize + 4, &res.a1, 4);
			memcpy(buf + currsize + 8, &res.a2, 4);
			memcpy(buf + currsize + 12, &res.a3, 4);
			currsize += 16;
		} else if (ret >  12) {
			memcpy(buf + currsize, &res.a0, 4);
			memcpy(buf + currsize + 4, &res.a1, 4);
			memcpy(buf + currsize + 8, &res.a2, 4);
			memcpy(buf + currsize + 12, &res.a3, ret - 12);
			currsize += ret;
		} else if (ret > 8) {
			memcpy(buf + currsize, &res.a0, 4);
			memcpy(buf + currsize + 4, &res.a1, 4);
			memcpy(buf + currsize + 8, &res.a2, ret - 8);
			currsize += ret;
		} else if (ret > 4) {
			memcpy(buf + currsize, &res.a0, 4);
			memcpy(buf + currsize + 4, &res.a1, ret - 4);
			currsize += ret;
		} else {
			memcpy(buf + currsize, &res.a0, ret);
			currsize += ret;
		}
	} while (currsize < max);

	return currsize;
}
#else
static int dw_trng_init(struct hwrng *rng)
{
	return 0;
}

static int dw_trng_read(struct hwrng *rng, void *buf, size_t max, bool wait)
{
	return 0;
}
#endif

static int dw_trng_probe(struct platform_device *pdev)
{
	struct hwrng *rng;
	int ret;

	rng = devm_kzalloc(&pdev->dev, sizeof(*rng), GFP_KERNEL);
	if (!rng)
		return -ENOMEM;

	rng->name = pdev->name;
	rng->init = dw_trng_init;
	rng->read = dw_trng_read;
	rng->quality = DW_TRNG_QUALITY;

	ret = devm_hwrng_register(&pdev->dev, rng);
	if (ret)
		dev_err(&pdev->dev, "failed to register hwrng!\n");

	return ret;
}

#ifdef CONFIG_OF
static const struct of_device_id dw_trng_of_match[] = {
	{ .compatible = "snps,dw-trng", },
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, dw_trng_of_match);
#endif

static struct platform_driver dw_trng_driver = {
	.probe = dw_trng_probe,
	.driver = {
		.name	= "dw-trng",
		.of_match_table = of_match_ptr(dw_trng_of_match),
	},
};

module_platform_driver(dw_trng_driver);

MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("<leon.liao@sophgo.com>");
MODULE_DESCRIPTION("Dw true random number generator driver");
