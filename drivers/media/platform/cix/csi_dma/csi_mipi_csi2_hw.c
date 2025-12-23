// SPDX-License-Identifier: GPL-2.0
/*
 * V4L2 Capture CSI Subdev for Cix sky SOC
 *
 * Copyright 2024 Cix Technology Group Co., Ltd.
 *
 */
#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/errno.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/irq.h>
#include <linux/kernel.h>
#include <linux/memory.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_graph.h>
#include <linux/platform_device.h>
#include <linux/pm_runtime.h>
#include <linux/regulator/consumer.h>
#include <linux/slab.h>
#include <linux/spinlock.h>
#include <linux/videodev2.h>
#include <media/v4l2-fwnode.h>
#include <media/v4l2-subdev.h>
#include <media/v4l2-device.h>
#include <linux/pm_domain.h>
#include <linux/pm_runtime.h>
#include <linux/iopoll.h>

#include "csi_common.h"

#define CIX_MIPI_CSI2_DRIVER_NAME "cix-mipi-csi2"
#define CIX_MIPI_CSI2_SUBDEV_NAME CIX_MIPI_CSI2_DRIVER_NAME
#define CIX_MIPI_OF_NODE_NAME "cix-csi"
#define CIX_MIPI_MAX_DEVS (0x08)

#define DATA_PATH_WIDTH (8 * 4)

static const char *mipi_csi2_clk_names[4] = { "csi_p0clk", "csi_p1clk",
					      "csi_p2clk", "csi_p3clk" };

static inline u32 mipi_csi2_read(struct mipi_csi2_hw *csi2dev, u32 reg)
{
	return readl(csi2dev->base + reg);
}

static inline void mipi_csi2_write(struct mipi_csi2_hw *csi2dev, u32 reg,
				   u32 val)
{
	writel(val, csi2dev->base + reg);
}

void mipi_csi2_enable_irq(struct mipi_csi2_hw *mipi_hw)
{
	u32 val;

	val = MIPI_SP_RCVD_IRQ | MIPI_LP_RCVD_IRQ | MIPI_SLEEP_IRQ |
	      MIPI_WAKEUP_IRQ | MIPI_DESKEW_ENTRY_IRQ |
	      MIPI_SP_GENERIC_RCVD_IRQ | MIPI_EPD_OPTION1_DETECT_IRQ |
	      MIPI_STREAM0_STOP_IRQ | MIPI_STREAM0_ABORT_IRQ |
	      MIPI_STREAM1_STOP_IRQ | MIPI_STREAM1_ABORT_IRQ |
	      MIPI_STREAM2_STOP_IRQ | MIPI_STREAM2_ABORT_IRQ |
	      MIPI_STREAM3_STOP_IRQ | MIPI_STREAM3_ABORT_IRQ |
	      MIPI_STREAM4_STOP_IRQ | MIPI_STREAM4_ABORT_IRQ |
	      MIPI_STREAM5_STOP_IRQ | MIPI_STREAM5_ABORT_IRQ |
	      MIPI_STREAM6_STOP_IRQ | MIPI_STREAM6_ABORT_IRQ |
	      MIPI_STREAM7_STOP_IRQ | MIPI_STREAM7_ABORT_IRQ;

	mipi_csi2_write(mipi_hw, INFO_IRQS_MASK, val);
}

static void mipi_csi2_enable_err_irq(struct mipi_csi2_hw *mipi_hw)
{
	u32 val;
	u32 status;

	/* clean the err status before enable */
	status = mipi_csi2_read(mipi_hw, ERROR_IRQS);
	mipi_csi2_write(mipi_hw, ERROR_IRQS, status);

	status = mipi_csi2_read(mipi_hw, DPHY_ERR_STATUS_IRQ);
	mipi_csi2_write(mipi_hw, DPHY_ERR_STATUS_IRQ, status);

	val = FRONT_FIFO_OVERFLOW_IRQ | PAYLOAD_CRC_IRQ | HEADER_ECC_IRQ |
	      HEADER_CORRECTED_ECC_IRQ | DATA_ID_IRQ |
	      PROT_TRUNCATED_PACKET_IRQ | PROT_FRAME_MISMATCH_IRQ |
	      PROT_LINE_MISMATCH_IRQ | STREAM0_FIFO_OVERFLOW_IRQ |
	      STREAM1_FIFO_OVERFLOW_IRQ | STREAM2_FIFO_OVERFLOW_IRQ |
	      STREAM3_FIFO_OVERFLOW_IRQ | STREAM4_FIFO_OVERFLOW_IRQ |
	      STREAM5_FIFO_OVERFLOW_IRQ | STREAM6_FIFO_OVERFLOW_IRQ |
	      STREAM7_FIFO_OVERFLOW_IRQ;

	mipi_csi2_write(mipi_hw, ERROR_IRQS_MASK, val);

	val = DL0_ERRSOTHS_IRQ | DL0_ERRSOTSYNCHS_IRQ | DL1_ERRSOTHS_IRQ |
	      DL1_ERRSOTSYNCHS_IRQ | DL2_ERRSOTHS_IRQ | DL2_ERRSOTSYNCHS_IRQ |
	      DL3_ERRSOTHS_IRQ | DL3_ERRSOTSYNCHS_IRQ | DL4_ERRSOTHS_IRQ |
	      DL4_ERRSOTSYNCHS_IRQ | DL5_ERRSOTHS_IRQ | DL5_ERRSOTSYNCHS_IRQ |
	      DL6_ERRSOTHS_IRQ | DL6_ERRSOTSYNCHS_IRQ | DL7_ERRSOTHS_IRQ |
	      DL7_ERRSOTSYNCHS_IRQ;
	mipi_csi2_write(mipi_hw, DPHY_ERR_IRQ_MASK_CFG, val);
}

static void mipi_csi2_disable_irq(struct mipi_csi2_hw *mipi_hw)
{
	mipi_csi2_write(mipi_hw, INFO_IRQS_MASK, 0);
}

static void mipi_csi2_disable_err_irq(struct mipi_csi2_hw *mipi_hw)
{
	mipi_csi2_write(mipi_hw, ERROR_IRQS_MASK, 0);
	mipi_csi2_write(mipi_hw, DPHY_ERR_IRQ_MASK_CFG, 0);
}

static int mipi_csi2_global_start(struct mipi_csi2_hw *mipi_hw)
{
	int val = 0;

	val = CORE_CTRL_START;
	mipi_csi2_write(mipi_hw, CORE_CTRL, val);

	val = CL_ENABLE | DPHY_RESET;
	if (mipi_hw->num_lanes == 4) {
		val |= (DL0_ENABLE | DL1_ENABLE | DL2_ENABLE | DL3_ENABLE);
	} else if (mipi_hw->num_lanes == 2) {
		val |= (DL0_ENABLE | DL1_ENABLE);
	} else if (mipi_hw->num_lanes == 1) {
		val |= DL0_ENABLE;
	} else {
		dev_err(mipi_hw->dev, "mipi-csi-hw lanes invalid %d\n",
			mipi_hw->num_lanes);
		return -1;
	}

	mipi_csi2_write(mipi_hw, DPHY_LANE_CONTROL, val);

	val = (STATIC_CFG_ENABLE_LRTE | (mipi_hw->num_lanes << 4));
	mipi_csi2_write(mipi_hw, STATIC_CFG_REG, val);

	return 0;
}

static int mipi_csi2_stream_start(struct mipi_csi2_hw *mipi_hw, unsigned int stream_id, unsigned int virtual_channel)
{
	int val = 0;

	val = VC_SELECT(virtual_channel);
	mipi_csi2_write(mipi_hw, STREAM_DATA_CFG(stream_id), val);

	val = LARGE_BUFFER << FIFO_MODE_OFFSET;
	mipi_csi2_write(mipi_hw, STREAM_CFG(stream_id), val);

	val = STEAM_CTRL_START;
	mipi_csi2_write(mipi_hw, STREAM_CTRL(stream_id), val);

	return 0;
}

static int mipi_csi2_start(struct mipi_csi2_hw *mipi_hw, unsigned int stream_id, unsigned int virtual_channel)
{
	unsigned long flags;

	dev_info(mipi_hw->dev, "mipi_csi2 hw index %d start\n", mipi_hw->id);

	spin_lock_irqsave(&mipi_hw->slock, flags);

	mipi_csi2_enable_err_irq(mipi_hw);

	if (atomic_read(&mipi_hw->stream_cnt) == 0)
		mipi_csi2_global_start(mipi_hw);

	mipi_csi2_stream_start(mipi_hw, stream_id, virtual_channel);

	atomic_inc(&mipi_hw->stream_cnt);

	spin_unlock_irqrestore(&mipi_hw->slock, flags);

	return 0;
}

static int mipi_csi2_global_stop(struct mipi_csi2_hw *mipi_hw)
{
	int val = 0;

	val = mipi_csi2_read(mipi_hw, DPHY_LANE_CONTROL);
	val &= ~(CL_ENABLE | DPHY_RESET);
	mipi_csi2_write(mipi_hw, DPHY_LANE_CONTROL, val);

	val = mipi_csi2_read(mipi_hw, CORE_CTRL);
	val &= ~CORE_CTRL_START;
	mipi_csi2_write(mipi_hw, CORE_CTRL, val);

	return 0;
}

static int mipi_csi2_stream_stop(struct mipi_csi2_hw *mipi_hw, unsigned int stream_id)
{
	int val = 0;

	val = mipi_csi2_read(mipi_hw, STREAM_CTRL(stream_id));
	val &= ~STEAM_CTRL_START;
	mipi_csi2_write(mipi_hw, STREAM_CTRL(stream_id), val);

	return 0;
}

static int mipi_csi2_stop(struct mipi_csi2_hw *mipi_hw, unsigned int stream_id)
{
	unsigned long flags;

	dev_info(mipi_hw->dev, "mipi-csi2-hw index %d stop\n", mipi_hw->id);

	spin_lock_irqsave(&mipi_hw->slock, flags);

	mipi_csi2_stream_stop(mipi_hw, stream_id);

	atomic_dec(&mipi_hw->stream_cnt);

	if (atomic_read(&mipi_hw->stream_cnt) == 0) {
		mipi_csi2_disable_irq(mipi_hw);
		mipi_csi2_disable_err_irq(mipi_hw);
		mipi_csi2_global_stop(mipi_hw);
	}

	spin_unlock_irqrestore(&mipi_hw->slock, flags);

	return 0;
}

static int mipi_csi2_hw_irq_enable(struct mipi_csi2_hw *mipi_hw, unsigned int enable)
{
	unsigned long flags;

	if (mipi_hw == NULL) {
		pr_err("mipi_hw handler is NULL");
		return -EINVAL;
	}

	spin_lock_irqsave(&mipi_hw->slock, flags);

	if (enable)
		mipi_csi2_enable_err_irq(mipi_hw);
	else
		mipi_csi2_disable_err_irq(mipi_hw);

	spin_unlock_irqrestore(&mipi_hw->slock, flags);

	return 0;
}

static int mipi_csi2_hw_resume(struct mipi_csi2_hw *mipi_hw)
{
	int i;

	if (mipi_hw->platform_id == CIX_PLATFORM_SOC) {

		if (atomic_read(&mipi_hw->stream_cnt) == 0) {

			if (clk_prepare_enable(mipi_hw->sclk)) {
				dev_err(mipi_hw->dev, "sys_clk enable failed\n");
				goto err_csi_clks;
			}

			if (clk_prepare_enable(mipi_hw->pclk)) {
				dev_err(mipi_hw->dev, "p_clk enable failed\n");
				goto err_csi_clks;
			}

			if ((mipi_hw->id == 0) || (mipi_hw->id == 2)) {
				for (i = 0; i < 4; i++) {
					if (clk_prepare_enable(mipi_hw->pixel_clk[i])) {
						dev_err(mipi_hw->dev, "pixel_clk[%d] enable failed\n",
								i);
						goto err_csi_clks;
					}
				}
			} else {
				if (clk_prepare_enable(mipi_hw->pixel_clk[0])) {
					dev_err(mipi_hw->dev, "pixel_clk[0] enable failed\n");
					goto err_csi_clks;
				}
			}

			reset_control_deassert(mipi_hw->reset);
		}
	}

	return 0;
err_csi_clks:
	return -1;
}

static int mipi_csi2_hw_suspend(struct mipi_csi2_hw *mipi_hw)
{
	int i;

	if (mipi_hw->platform_id == CIX_PLATFORM_SOC) {

		if (atomic_read(&mipi_hw->stream_cnt) == 0) {

			clk_disable_unprepare(mipi_hw->sclk);
			clk_disable_unprepare(mipi_hw->pclk);

			if ((mipi_hw->id == 0) || (mipi_hw->id == 2)) {
				for (i = 0; i < 4; i++)
					clk_disable_unprepare(mipi_hw->pixel_clk[i]);
			} else
				clk_disable_unprepare(mipi_hw->pixel_clk[0]);

			reset_control_assert(mipi_hw->reset);
		}
	}

	return 0;
}

static u32 mipi_csi2_get_irq_status(struct mipi_csi2_hw *mipi_hw)
{
	return mipi_csi2_read(mipi_hw, INFO_IRQS);
}

static void mipi_csi2_clear_irq_status(struct mipi_csi2_hw *mipi_hw, u32 status)
{
	mipi_csi2_write(mipi_hw, INFO_IRQS, status);
}

static irqreturn_t mipi_csi2_irq_handler(int irq, void *priv)
{
	struct mipi_csi2_hw *mipi_hw = priv;
	unsigned long flags;
	u32 status;

	spin_lock_irqsave(&mipi_hw->slock, flags);
	status = mipi_csi2_get_irq_status(mipi_hw);
	mipi_csi2_clear_irq_status(mipi_hw, status);
	spin_unlock_irqrestore(&mipi_hw->slock, flags);

	return IRQ_HANDLED;
}

static struct err_status
mipi_csi2_get_err_irq_status(struct mipi_csi2_hw *mipi_hw)
{
	struct err_status status;

	status.csi_err_status = mipi_csi2_read(mipi_hw, ERROR_IRQS);
	status.dphy_err_status = mipi_csi2_read(mipi_hw, DPHY_ERR_STATUS_IRQ);

	return status;
}

static void mipi_csi2_clear_err_irq_status(struct mipi_csi2_hw *mipi_hw,
					   struct err_status status)
{
	mipi_csi2_write(mipi_hw, ERROR_IRQS, status.csi_err_status);
	mipi_csi2_write(mipi_hw, DPHY_ERR_STATUS_IRQ, status.dphy_err_status);
}

static irqreturn_t mipi_csi2_err_irq_handler(int irq, void *priv)
{
	struct mipi_csi2_hw *mipi_hw = priv;
	unsigned long flags;
	struct err_status status;

	spin_lock_irqsave(&mipi_hw->slock, flags);

	status = mipi_csi2_get_err_irq_status(mipi_hw);

	dev_info(mipi_hw->dev,
			"CSI ERROR status 0x%08x, DPHY error status 0x%08x\n",
			status.csi_err_status, status.dphy_err_status);

	mipi_csi2_clear_err_irq_status(mipi_hw, status);

	spin_unlock_irqrestore(&mipi_hw->slock, flags);

	return IRQ_HANDLED;
}

static int mipi_csi2_hw_parse(struct mipi_csi2_hw *mipi_hw)
{
	struct device *dev = mipi_hw->dev;
	struct platform_device *pdev = mipi_hw->pdev;
	int lanes = 0;
	int irq0, irq1;
	int i;
	int ret;

	if (has_acpi_companion(dev)) {
		ret = device_property_read_u8(dev, CIX_MIPI_CSI2_HW_OF_NODE_NAME,
				&mipi_hw->id);
	} else {
		ret = mipi_hw->id =
			of_alias_get_id(dev->of_node, CIX_MIPI_CSI2_HW_OF_NODE_NAME);
	}

	if ((ret < 0) || (mipi_hw->id >= CIX_MIPI_MAX_DEVS)) {
		dev_err(dev, "invalid mipi hw id (%d)\n", mipi_hw->id);
		return -EINVAL;
	}

	dev_info(dev, "mipi hw id %d\n", mipi_hw->id);

	mipi_hw->base = devm_platform_ioremap_resource(mipi_hw->pdev, 0);
	if (IS_ERR(mipi_hw->base))
		return PTR_ERR(mipi_hw->base);

	irq0 = platform_get_irq(pdev, 0);
	if (irq0 < 0)
		dev_err(dev, "Failed to get IRQ resource\n");

	ret = devm_request_irq(dev, irq0, mipi_csi2_irq_handler,
			       IRQF_ONESHOT | IRQF_SHARED, dev_name(dev),
			       mipi_hw);
	if (ret)
		dev_err(dev, "failed to install irq (%d)\n", ret);

	irq1 = platform_get_irq(pdev, 1);
	if (!irq1)
		dev_err(dev, "Failed to get IRQ resource\n");

	ret = devm_request_irq(dev, irq1, mipi_csi2_err_irq_handler,
			       IRQF_ONESHOT | IRQF_SHARED, dev_name(dev),
			       mipi_hw);
	if (ret)
		dev_err(dev, "failed to install irq (%d)\n", ret);

	ret = device_property_read_u32(&mipi_hw->pdev->dev, "lanes", &lanes);
	if (ret < 0) {
		dev_err(dev, "%s failed to get mipi-csi lanes property\n",
			__func__);
		return ret;
	}

	mipi_hw->num_lanes = lanes;
	dev_info(dev, "mipi-csi-hw get lanes %d\n", mipi_hw->num_lanes);

	dev_info(dev, "mipi get the clk & rest resource\n");

	mipi_hw->sclk = devm_clk_get_optional(&pdev->dev, "csi_sclk");
	if (IS_ERR(mipi_hw->sclk)) {
		dev_err(&pdev->dev, "Couldn't get sys clock\n");
		return PTR_ERR(mipi_hw->sclk);
	}

	mipi_hw->pclk = devm_clk_get_optional(&pdev->dev, "csi_pclk");
	if (IS_ERR(mipi_hw->pclk)) {
		dev_err(&pdev->dev, "Couldn't get csi_pclk clock\n");
		return PTR_ERR(mipi_hw->pclk);
	}

	if ((mipi_hw->id == 0) || (mipi_hw->id == 2)) {
		for (i = 0; i < 4; i++) {
			mipi_hw->pixel_clk[i] = devm_clk_get_optional(
				&pdev->dev, mipi_csi2_clk_names[i]);
			if (IS_ERR(mipi_hw->pixel_clk[i])) {
				dev_err(&pdev->dev, "Couldn't get %s clock\n",
					mipi_csi2_clk_names[i]);
				return PTR_ERR(mipi_hw->pixel_clk[i]);
			}
		}
	} else {
		mipi_hw->pixel_clk[0] = devm_clk_get_optional(
			&pdev->dev, mipi_csi2_clk_names[0]);
		if (IS_ERR(mipi_hw->pixel_clk[0])) {
			dev_err(&pdev->dev, "Couldn't get %s clock\n",
				mipi_csi2_clk_names[0]);
			return PTR_ERR(mipi_hw->pixel_clk[0]);
		}
	}

	mipi_hw->reset =
		devm_reset_control_get_optional_shared(&pdev->dev, "csi_reset");

	if (IS_ERR(mipi_hw->reset)) {
		if (PTR_ERR(mipi_hw->reset) != -EPROBE_DEFER)
			dev_err(&pdev->dev,
				"Failed to get csi  reset control\n");
		return PTR_ERR(mipi_hw->reset);
	}

	return 0;
}

static int mipi_csi2_hw_probe(struct platform_device *pdev)
{
	struct mipi_csi2_hw *mipi_hw = NULL;
	struct device *dev = &pdev->dev;

	dev_info(dev, "mipi-csi2-hw probe enter\n");

	mipi_hw = devm_kzalloc(dev, sizeof(*mipi_hw), GFP_KERNEL);
	if (!mipi_hw)
		return -ENOMEM;

	mipi_hw->dev = dev;
	mipi_hw->pdev = pdev;
	mipi_hw->drv_data = device_get_match_data(dev);

	mutex_init(&mipi_hw->lock);
	spin_lock_init(&mipi_hw->slock);

	/*parse the dts or ACPI table*/
	mipi_csi2_hw_parse(mipi_hw);

	mipi_hw->platform_id = CIX_PLATFORM_SOC;

	platform_set_drvdata(pdev, mipi_hw);

	dev_info(dev, "mipi-csi2-hw probe success\n");

	return 0;
}

static int mipi_csi2_hw_remove(struct platform_device *pdev)
{
	return 0;
}

static const struct mipi_csi2_hw_drv_data cix_mipi_hw_drv_data = {
	.stream_start = mipi_csi2_start,
	.stream_stop = mipi_csi2_stop,
	.hw_resume = mipi_csi2_hw_resume,
	.hw_suspend = mipi_csi2_hw_suspend,
	.mipi_csi2_irq_enable = mipi_csi2_hw_irq_enable,
};

static const struct of_device_id mipi_csi2_hw_of_match[] = {
	{ .compatible = "cix,cix-mipi-csi2-hw", .data = &cix_mipi_hw_drv_data},
	{ /* sentinel */ },
};
MODULE_DEVICE_TABLE(of, mipi_csi2_hw_of_match);

static struct platform_driver mipi_csi2_hw_driver = {
	.driver = {
		.name = CIX_MIPI_CSI2_HW_DRIVER_NAME,
		.of_match_table = mipi_csi2_hw_of_match,
	},
	.probe = mipi_csi2_hw_probe,
	.remove = mipi_csi2_hw_remove,
};

module_platform_driver(mipi_csi2_hw_driver);

MODULE_AUTHOR("Cix Semiconductor, Inc.");
MODULE_DESCRIPTION("Cix MIPI CSI2 HW driver");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:" CIX_MIPI_CSI2_HW_DRIVER_NAME);
