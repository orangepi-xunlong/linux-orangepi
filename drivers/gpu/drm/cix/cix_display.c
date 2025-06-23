// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright 2024 Cix Technology Group Co., Ltd.
 */

#include <linux/device.h>
#include <linux/module.h>
#include <linux/of_platform.h>
#include <linux/platform_device.h>
#include <linux/io.h>

#define DPU0_RESET_MASK BIT(16)
#define DPU1_RESET_MASK BIT(17)
#define DPU2_RESET_MASK BIT(18)
#define DPU3_RESET_MASK BIT(19)
#define DPU4_RESET_MASK BIT(20)

#define DP0_RESET_MASK BIT(21)
#define DP1_RESET_MASK BIT(22)
#define DP2_RESET_MASK BIT(23)
#define DP3_RESET_MASK BIT(24)
#define DP4_RESET_MASK BIT(25)

#define MMHUB_RESET_MASK BIT(4)

#define DISPLAY0_RESET_MASK (DPU0_RESET_MASK | DP0_RESET_MASK)
#define DISPLAY1_RESET_MASK (DPU1_RESET_MASK | DP1_RESET_MASK)
#define DISPLAY2_RESET_MASK (DPU2_RESET_MASK | DP2_RESET_MASK)
#define DISPLAY3_RESET_MASK (DPU3_RESET_MASK | DP3_RESET_MASK)
#define DISPLAY4_RESET_MASK (DPU4_RESET_MASK | DP4_RESET_MASK)

#define DISPLAY_RESET_REG 0x16000400

struct cix_display_dev {
       struct device *dev;
       u32 reset_mask[5];
       bool reset_need[5];
};

static const struct of_device_id cix_display_dt_ids[] = {
       { .compatible = "cix,display", },
       { /* sentinel */ },
};
MODULE_DEVICE_TABLE(of, cix_display_dt_ids);

static int cix_display_probe(struct platform_device *pdev)
{
       struct cix_display_dev *cix_display;
       struct device *dev = &pdev->dev;
       void *base;
       int i, ret;
       u32 control, value, count;

       dev_info(dev, "%s enter.\n", __func__);

       cix_display = devm_kzalloc(dev, sizeof(*cix_display), GFP_KERNEL);
       if (!cix_display)
               return -ENOMEM;

       ret = of_property_read_u32(dev->of_node, "reset-control", (u32 *)&control);
       if (ret) {
               control = 0;
               dev_info(dev, "failed to get reset-control, use default value\n");
       }

       cix_display->reset_mask[0] = DISPLAY0_RESET_MASK;
       cix_display->reset_mask[1] = DISPLAY1_RESET_MASK;
       cix_display->reset_mask[2] = DISPLAY2_RESET_MASK;
       cix_display->reset_mask[3] = DISPLAY3_RESET_MASK;
       cix_display->reset_mask[4] = DISPLAY4_RESET_MASK;

       base = ioremap(DISPLAY_RESET_REG, 0x4);

       value = readl(base);
       dev_info(dev, "current reset value = 0x%x\n", value);

       count = 0;
       for (i = 0; i < 5; i++) {
               cix_display->reset_need[i] = (control >> i) & 0x1;
               if (cix_display->reset_need[i]) {
                       count++;
                       value &= (~cix_display->reset_mask[i]);
               }
       }

       if (count) {
               writel(value, base);
               dev_info(dev, "assert reset, value = 0x%x\n", value);
       }

       for (i = 0; i < 5; i++) {
               if (cix_display->reset_need[i]) {
                       value |= cix_display->reset_mask[i];
               }
       }

       if (count) {
               writel(value, base);
               dev_info(dev, "deassert reset, value = 0x%x\n", value);
       }

       value = readl(base);
       dev_info(dev, "current reset value = 0x%x\n", value);

       iounmap(base);

       cix_display->dev = dev;

       dev_set_drvdata(dev, cix_display);

       return 0;
}

static int cix_display_remove(struct platform_device *pdev)
{
       struct device *dev = &pdev->dev;

       dev_set_drvdata(dev, NULL);

       return 0;
}

struct platform_driver cix_display_driver = {
       .probe  = cix_display_probe,
       .remove = cix_display_remove,
       .driver = {
               .name = "cix-display",
               .of_match_table = of_match_ptr(cix_display_dt_ids),
       },
};

module_platform_driver(cix_display_driver);

MODULE_DESCRIPTION("Cix Display Driver");
MODULE_LICENSE("GPL v2");
MODULE_ALIAS("platform:cix-display");

