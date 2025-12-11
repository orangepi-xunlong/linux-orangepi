// SPDX-License-Identifier: GPL-2.0
/*
 *csi_bridge_hw.c - csi_dma register control
 *Copyright 2024 Cix Technology Group Co., Ltd.
 */
#include "csi_bridge_hw.h"

struct csi_dma_store_data {
	u32 offset;
	char *name;
	u32 value;
};

struct csi_dma_store_data store_table[] = {
	{ 0x118, "AXI_ADDR_START0_LOW" },
	{ 0x11C, "AXI_ADDR_START0_HIGH" },
	{ 0x120, "AXI_ADDR_START1_LOW" },
	{ 0x124, "AXI_ADDR_START1_HIGH" },
};

static int csi_bridge_store(struct csi_dma_hw_dev *csi_dma)
{
	u32 value;
	u32 i;

	for (i = 0; i < ARRAY_SIZE(store_table); i++) {
		value = readl(csi_dma->regs + store_table[i].offset);
		store_table[i].value = value;
	}

	return 0;
}

static int csi_bridge_restore(struct csi_dma_hw_dev *csi_dma)
{
	u32 i;

	for (i = 0; i < ARRAY_SIZE(store_table); i++) {
		writel(store_table[i].value,
		       csi_dma->regs + store_table[i].offset);
	}

	return 0;
}

#ifdef CSI_DMA_DEBUG
void dump_csi_bridge_regs(struct csi_dma_hw_dev *csi_dma)
{
	struct device *dev = &csi_dma->pdev->dev;
	struct {
		u32 offset;
		const char *const name;
	} registers[] = {
		{ 0x00, "CSI_BRIDGE_CTRL" },
		{ 0x04, "DMA_BRIDGE_CTRL" },
		{ 0x08, "BRIDGE_STATUS" },
		{ 0x0C, "CNT_EN_CTRL" },
		{ 0x10, "PIXETL_CNT_TOTAL" },
		{ 0x14, "LINE_CNT_TOTAL" },
		{ 0x18, "FRAME_CNT" },
		{ 0x1C, "PKT_INFO0" },
		{ 0x20, "PKT_INFO1" },
		{ 0x24, "PKT_INFO2" },
		{ 0x28, "FRAME_CFG" },
		{ 0x100, "OT_INFO_CFG" },
		{ 0x104, "OUT_CTRL" },
		{ 0x108, "AXI_CTRL" },
		{ 0x10C, "AXI_STAUS" },
		{ 0x110, "AXI_USER0" },
		{ 0x114, "AXI_USER1" },
		{ 0x118, "AXI_ADDR_START0_LOW" },
		{ 0x11C, "AXI_ADDR_START0_HIGH" },
		{ 0x120, "AXI_ADDR_START1_LOW" },
		{ 0x124, "AXI_ADDR_START1_HIGH" },
		{ 0x128, "CURRENT_ADDR0_LO" },
		{ 0x12C, "CURRENT_ADDR0_HI" },
		{ 0x130, "CURRENT_ADDR1_LO" },
		{ 0x134, "CURRENT_ADDR1_HI" },
		{ 0x138, "LINE_STRIDE" },
		{ 0x13C, "VSYNC_WIDTH" },
		/*{ 0x140, "NSAID" }, */
		/*{ 0x144, "AW_PORT" },*/
		{ 0x200, "INTERRUTP_EN" },
		{ 0x204, "INTERRUPT_STATUS" },
		{ 0x208, "INTERRUPT_CLR" },
		{ 0x20C, "PWR_CTRL" },
		{ 0x210, "TIMEOUT_CTRL" },
		{ 0x214, "LINE_INT_CTRL" },
		{ 0x300, "DBG_REG0" },
		{ 0x304, "DBG_REG1" },
		{ 0x308, "DBG_REG2" },
	};
	u32 i;

	dev_dbg(dev, "CIX Bridge register dump, cix-bridge.%d\n", csi_dma->id);
	for (i = 0; i < ARRAY_SIZE(registers); i++) {
		u32 reg = readl(csi_dma->regs + registers[i].offset);

		dev_info(dev, "%20s[0x%.2x]: %.2x\n", registers[i].name,
			 registers[i].offset, reg);
	}
}
#endif

static inline u32 csi_dma_read(struct csi_dma_hw_dev *csi_dma, u32 reg)
{
	return readl(csi_dma->regs + reg);
}

static inline void csi_dma_write(struct csi_dma_hw_dev *csi_dma, u32 reg, u32 val)
{
	writel(val, csi_dma->regs + reg);
}

void frame_star(struct csi_dma_hw_dev *csi_dma)
{
}

void line_count(struct csi_dma_hw_dev *csi_dma)
{
}

void every_n_line(struct csi_dma_hw_dev *csi_dma)
{
}

void dma_err_rst_int(struct csi_dma_hw_dev *csi_dma)
{
}

static void csi_dma_set_line_stride(struct csi_dma_hw_dev *csi_dma, u32 stride)
{
	csi_dma_write(csi_dma, LINE_STRIDE, stride);
}

void csi_bridge_set_plane_buffer_addr(struct csi_dma_hw_dev *csi_dma, int plane_no, u64 base_addr, u64 offset_addr)
{
	u64 y_addr = base_addr + offset_addr;
	u32 val = 0;

	if (plane_no == 0) {
		val = y_addr & ADDRESS0_LO_MASK;
		csi_dma_write(csi_dma, AXI_ADDR_START0_LOW, val);
		val = (y_addr >> 32) & ADDRESS0_HI_MASK;
		csi_dma_write(csi_dma, AXI_ADDR_START0_HIGH, val);
	} else {
		val = y_addr & ADDRESS1_LO_MASK;
		csi_dma_write(csi_dma, AXI_ADDR_START1_LOW, val);
		val = (y_addr >> 32) & ADDRESS1_HI_MASK;
		csi_dma_write(csi_dma, AXI_ADDR_START1_HIGH, val);
	}
}

static void csi_dma_reset(struct csi_dma_hw_dev *csi_dma)
{
	// TODO
}

static u32 csi_dma_axi_ot_config(struct csi_dma_hw_dev *csi_dma, u32 ot_y, u32 ot_u)
{
	u32 val;

	if ((ot_y < 1) || (ot_y > 64)) {
		dev_err(&csi_dma->pdev->dev, "invalid outstanding y %d\n",
			ot_y);
		return -1;
	} else if (ot_u > 64) {
		dev_err(&csi_dma->pdev->dev, "invalid outstanding u %d\n",
			ot_u);
		return -1;
	}

	val = csi_dma_read(csi_dma, OT_INFO_CFG);
	val |= (ot_y << OT_CFG_Y_OFFSET) | (ot_u << OT_CFG_U_OFFSET);
	csi_dma_write(csi_dma, OT_INFO_CFG, val);
	return 0;
}

static void csi_dma_axi_config(struct csi_dma_hw_dev *csi_dma)
{
	u32 val;

	val = csi_dma->axi_uid << STREAM_ID_OFFSET;
	csi_dma_write(csi_dma, AXI_USER0, val);
	csi_dma_write(csi_dma, AXI_CTRL, 0x3d3F);
	csi_dma_write(csi_dma, OT_INFO_CFG, 0x40);
}

static void csi_dma_frame_config(struct csi_dma_hw_dev *csi_dma,
				 struct csi_dma_frame *src_f)
{
	u32 val;
	u8 mem_layout_fmt;
	u8 frame_cfg_dt;

	dev_info(&csi_dma->pdev->dev, "%s enter src_f->fmt->mbus_code = 0x%x\n",
		 __func__, src_f->fmt->mbus_code);
	switch (src_f->fmt->mbus_code) {
	/* Src RGB888 -> Sink RGB888X*/
	case MEDIA_BUS_FMT_RGB888_1X24:
		mem_layout_fmt = MEM_LAYOUT_FMT_RGB888;
		frame_cfg_dt = IMG_DT_RGB888;
		break;
	case MEDIA_BUS_FMT_RGB565_1X16:
		mem_layout_fmt = MEM_LAYOUT_FMT_RGB565;
		frame_cfg_dt = IMG_DT_RGB565;
		break;
	case MEDIA_BUS_FMT_SBGGR8_1X8:
		mem_layout_fmt = MEM_LAYOUT_FMT_RAW16;
		frame_cfg_dt = IMG_DT_RAW8;
		break;
	case MEDIA_BUS_FMT_SBGGR10_1X10:
		mem_layout_fmt = MEM_LAYOUT_FMT_RAW;
		frame_cfg_dt = IMG_DT_RAW10;
		break;
	case MEDIA_BUS_FMT_SBGGR12_1X12:
		mem_layout_fmt = MEM_LAYOUT_FMT_RAW16;
		frame_cfg_dt = IMG_DT_RAW12;
		break;
	case MEDIA_BUS_FMT_SBGGR14_1X14:
		mem_layout_fmt = MEM_LAYOUT_FMT_RAW16;
		frame_cfg_dt = IMG_DT_RAW14;
		break;
	case MEDIA_BUS_FMT_SBGGR16_1X16:
		mem_layout_fmt = MEM_LAYOUT_FMT_RAW16;
		frame_cfg_dt = IMG_DT_RAW16;
		break;
	case MEDIA_BUS_FMT_UYYVYY8_0_5X24:
		mem_layout_fmt = MEM_LAYOUT_FMT_NV12;
		frame_cfg_dt = IMG_DT_LEGACY_YUV420_8BIT;
		break;
	case MEDIA_BUS_FMT_YVYU8_2X8:
		mem_layout_fmt = MEM_LAYOUT_FMT_NV12;
		/*here input is yuyv mipi do not have nv12 data type*/
		frame_cfg_dt = IMG_DT_YUV422_8BIT;
		break;
	case MEDIA_BUS_FMT_YUYV10_2X10:
		mem_layout_fmt = MEM_LAYOUT_FMT_P010;
		frame_cfg_dt = IMG_DT_YUV420_10BIT;
		break;
	case MEDIA_BUS_FMT_YUYV8_1X16:
		mem_layout_fmt = MEM_LAYOUT_FMT_YUYV;
		frame_cfg_dt = IMG_DT_YUV422_8BIT;
		break;
	case MEDIA_BUS_FMT_YUYV10_1X20:
		mem_layout_fmt = MEM_LAYOUT_FMT_YUYV10;
		frame_cfg_dt = IMG_DT_YUV422_10BIT;
		break;
	default:
		mem_layout_fmt = MEM_LAYOUT_FMT_RGB888X;
		frame_cfg_dt = IMG_DT_RGB888;
		dev_err(&csi_dma->pdev->dev, "invalid format %d\n",
			src_f->fmt->mbus_code);
		break;
	};

	dev_info(&csi_dma->pdev->dev, "image_size{%d %d} dt=%d\n",
		 src_f->height, src_f->width, frame_cfg_dt);
	val = (src_f->width << FRAME_NUM_OFFSET) |
	      (src_f->height << LINE_NUM_OFFSET);

	csi_dma_write(csi_dma, FRAME_CFG, val);

	val = (mem_layout_fmt << MEM_LAYOUT_FMT_OFFSET) |
	      (STRIDE_16BYTE << STRIDE_OFFSET) |
	      (frame_cfg_dt << DT_SYNC_FIFO_OFFSET);

	if (frame_cfg_dt == IMG_DT_LEGACY_YUV420_8BIT)
		val |= LEGACY_YUV << LEGACY_YUV_OFFSET;

	csi_dma_write(csi_dma, OUT_CTRL, val);
}

static void csi_dma_line_stride_config(struct csi_dma_hw_dev *csi_dma,
				       struct csi_dma_frame *src_f)
{
	u32 val = 0;

	switch (src_f->fmt->mbus_code) {
	case MEDIA_BUS_FMT_RGB888_1X24:
		val = src_f->width * 3;
		break;
	case MEDIA_BUS_FMT_SBGGR8_1X8:
	case MEDIA_BUS_FMT_SBGGR10_1X10:
	case MEDIA_BUS_FMT_SBGGR12_1X12:
	case MEDIA_BUS_FMT_SBGGR14_1X14:
	case MEDIA_BUS_FMT_SBGGR16_1X16:
	case MEDIA_BUS_FMT_YUYV8_1X16:
	case MEDIA_BUS_FMT_YUYV10_2X10:
		val = src_f->width * 2;
		break;
	case MEDIA_BUS_FMT_YUYV10_1X20:
		val = src_f->width * 2 * 10 / 8;
		break;
	case MEDIA_BUS_FMT_UYVY8_2X8:
	case MEDIA_BUS_FMT_VYUY8_2X8:
	case MEDIA_BUS_FMT_YVYU8_2X8:
	case MEDIA_BUS_FMT_YUYV8_2X8:
		val = src_f->width;
		break;
	default:
		dev_err(&csi_dma->pdev->dev, "invalid format %d\n",
			src_f->fmt->mbus_code);
		val = src_f->width;
		break;
	};
	dev_info(&csi_dma->pdev->dev,
		 "mbus_code = 0x%x, line_stride = %d, src_f->width = %d\n",
		 src_f->fmt->mbus_code, val, src_f->width);
	csi_dma_set_line_stride(csi_dma, val);
}

static void csi_dma_counter_ctrl(struct csi_dma_hw_dev *csi_dma)
{
	u32 val;

	val = (PIXEL_TOTAL_CNT_EN << PIXEL_TOTAL_CNT_EN_OFFSET) |
	      (LINE_TOTAL_CNT_EN << LINE_TOTAL_CNT_EN_OFFSET) |
	      (FRAME_CNT_EN << FRAME_CNT_EN_OFFSET) |
	      (PIXEL_CNT_PER_LINE_EN << PIXEL_CNT_PER_LINE_EN_OFFSET) |
	      (LINE_CNT_PER_FRAME_EN << LINE_CNT_PER_FRAME_EN_OFFSET);

	csi_dma_write(csi_dma, CNT_EN_CTRL, val);
}

static u32 csi_dma_get_irq_status(struct csi_dma_hw_dev *csi_dma_hw)
{
	return csi_dma_read(csi_dma_hw, INTERRUPT_STATUS);
}

static void csi_dma_clean_irq_status(struct csi_dma_hw_dev *csi_dma, u32 val)
{
	csi_dma_write(csi_dma, INTERRUPT_CLR, val);
}

static void csi_dma_enable_irq(struct csi_dma_hw_dev *csi_dma)
{
	u32 val;

	val = FRAME_END_INT_EN_MASK | LINE_CNT_INT_EN_MASK |
	      LINE_MODE_INT_EN_MASK | PIXEL_ERR_INT_EN_MAKS |
	      ASYNC_FIFO_OVF_INT_EN_MASK | ASYNC_FIFO_UNDF_INT_EN_MASK |
	      DMA_OVF_INT_EN_MASK | FRAME_STOP_INT_EN_MASK |
	      DMA_ERR_RST_INT_EN_MASK | UNSUPPORT_DT_INT_EN_MASK |
	      UNSUPPORT_STRIDE_INT_EN_MASK | LINE_MISMATCH_INT_EN_MASK |
	      PIXEL_MISMATCH_INT_EN_MASK | TIMEOUT_INT_EN_MASK;

	csi_dma_write(csi_dma, INTERRUTP_EN, val);
}

static void csi_dma_disable_irq(struct csi_dma_hw_dev *csi_dma)
{
	csi_dma_write(csi_dma, INTERRUTP_EN, 0);
}

void csi_dma_dma_start_stream(struct csi_dma_hw_dev *csi_dma)
{
	u32 val;

	val = DMA_BRIDGE_EN << DMA_BRIDGE_EN_OFFSET;

	csi_dma_write(csi_dma, CSI_BRIDGE_CTRL, val);

	val = START << DMA_START_OFFSET;

	csi_dma_write(csi_dma, DMA_BRIDGE_CTRL, val);
}

static void csi_dma_clean_registers(struct csi_dma_hw_dev *csi_dma)
{
	u32 status;

	status = csi_dma_get_irq_status(csi_dma);
	csi_dma_clean_irq_status(csi_dma, status);
}

/* start DMA Bridge
 * 1: release the reset
 * 2: setup the axi configurations including burst length, burst type, axcache,nsaid, streamID tec
 * 3: setup frame configuration (line number & pixel number)
 * 4: enable the counter as needed
 * 5: enable  interrupt as needed
 * 6: start the stream by configure the register DMA_BRIDGE_CTRL[0] to 1
 */
static void csi_dma_bridge_start(struct csi_dma_hw_dev *csi_dma,
			  struct csi_dma_frame *src_f)
{
	/* step1 */
	csi_dma_reset(csi_dma);

	/* step2 config axi*/
	csi_dma_axi_config(csi_dma);
	/* step3 */
	csi_dma_frame_config(csi_dma, src_f);

	csi_dma_line_stride_config(csi_dma, src_f);
	/* step4 */
	csi_dma_counter_ctrl(csi_dma);

	csi_dma_clean_registers(csi_dma);
	/* step5 */
	csi_dma_enable_irq(csi_dma);

	csi_dma_axi_ot_config(csi_dma, 32, 32);

	/* step6 */
	csi_dma_dma_start_stream(csi_dma);

	/* DUMP all csi_dma register*/
#ifdef CSI_DMA_DEBUG
	dump_csi_bridge_regs(csi_dma);
#endif
}

/* stop DMA Bridge
 * 1: When DMA bridge is working
 * 2: Read Status register, it should be in running status (address 04)
 * 3: Write Regiter bit DMA_BRIDGE_CTRL[1]
 * 4: Wait for stop interrupt
 * 5: Read Status it will be changed to stop status .
 */
static void csi_dma_bridge_stop(struct csi_dma_hw_dev *csi_dma)
{
	u32 val;
	u32 cnt = 0;

	csi_dma_disable_irq(csi_dma);
	/* step1 */
	val = csi_dma_read(csi_dma, CSI_BRIDGE_CTRL);
	if (val & DMA_BRIDGE_EN_MASK) {
		dev_info(&csi_dma->pdev->dev, "dma bridge is enable\n");

		/* step2 */
		val = csi_dma_read(csi_dma, DMA_BRIDGE_CTRL);
		if (val & DMA_START_MASK) {
			dev_info(&csi_dma->pdev->dev,
				 "dma bridge is working\n");
		} else {
			dev_info(&csi_dma->pdev->dev,
				 "dma bridge not working\n");
			return;
		}
	} else {
		dev_info(&csi_dma->pdev->dev, "dma bridge not enable\n");
		return;
	}

	/* step3 */
	val = STOP << DMA_START_OFFSET;
	csi_dma_write(csi_dma, DMA_BRIDGE_CTRL, val);
	/* Step4 - 1s timeout */
	do {
		val = csi_dma_get_irq_status(csi_dma);
		if (val & FRAME_STOP_INT_EN_MASK) {
			dev_info(&csi_dma->pdev->dev,
				 "success acquire stop interrupt\n");
			break;

		} else {
			//count timeout
			cnt++;
		}

	} while (cnt >= CHECK_MAX_CNT);

	if (cnt >= CHECK_MAX_CNT) {
		dev_err(&csi_dma->pdev->dev,
			"wait for stop interrupt timeout\n");
	}

	/* Step5 */
	val = csi_dma_read(csi_dma, DMA_BRIDGE_CTRL);
	if (val & DMA_STOP_MASK) {
		dev_info(&csi_dma->pdev->dev,
			 "dma bridge be changed to stop status\n");
	}
}

static int csi_bridge_buffer_done_callback_register(struct csi_dma_hw_dev *hw, void *callback, void *data)
{
	if ((hw == NULL) || (callback == NULL))
		return -1;

	hw->callback = callback;
	hw->data = data;

	return 0;
}

static int csi_bridge_hw_enable_irq(struct csi_dma_hw_dev *hw, int enable)
{
	if (hw == NULL)
		return -1;

	if (enable)
		csi_dma_enable_irq(hw);
	else
		csi_dma_disable_irq(hw);

	return 0;
}

static int csi_bridge_buffer_done(struct csi_dma_hw_dev *hw)
{
	if (hw == NULL)
		return -1;

	if (hw->callback != NULL)
		hw->callback(hw->data);

	return 0;
}

static irqreturn_t csi_dma_irq_handler(int irq, void *priv)
{
	struct csi_dma_hw_dev *csi_dma_hw = priv;
	unsigned long flags;
	u32 status;

	spin_lock_irqsave(&csi_dma_hw->slock, flags);
	status = csi_dma_get_irq_status(csi_dma_hw);
	csi_dma_hw->status = status;
	csi_dma_clean_irq_status(csi_dma_hw, status);

	/* frame start interrupt */
	if (status & FRAME_START_INT_EN_MASK)
		frame_star(csi_dma_hw);

	/* frame end interrupt */
	if (status & FRAME_END_INT_EN_MASK)
		csi_bridge_buffer_done(csi_dma_hw);

	/* specific line number interrupt */
	if (status & LINE_CNT_INT_EN_MASK)
		line_count(csi_dma_hw);

	/* every N lines interrupt */
	if (status & LINE_MODE_INT_EN_MASK)
		every_n_line(csi_dma_hw);

	if (status & PIXEL_ERR_INT_EN_MAKS)
		dev_err(csi_dma_hw->dev, "csi_dma pixsel err\n");

	if (status & ASYNC_FIFO_OVF_INT_EN_MASK)
		dev_err(csi_dma_hw->dev, "csi_dma async FIFO overflow\n");

	if (status & ASYNC_FIFO_UNDF_INT_EN_MASK)
		dev_err(csi_dma_hw->dev, "csi_dma async FIFO underrun\n");

	if (status & DMA_OVF_INT_EN_MASK)
		dev_err(csi_dma_hw->dev, "csi_dma overflow\n");

	if (status & DMA_UNDF_INT_EN_MASK)
		dev_err(csi_dma_hw->dev, "csi_dma underrun\n");

	if (status & DMA_ERR_RST_INT_EN_MASK)
		dma_err_rst_int(csi_dma_hw);

	if (status & UNSUPPORT_DT_INT_EN_MASK)
		dev_err(csi_dma_hw->dev, "csi_dma unsupport date type\n");

	if (status & UNSUPPORT_STRIDE_INT_EN_MASK)
		dev_err(csi_dma_hw->dev, "csi_dma unsupport stride\n");

	if (status & LINE_MISMATCH_INT_EN_MASK)
		dev_err(csi_dma_hw->dev, "csi_dma line mismatch\n");

	if (status & PIXEL_MISMATCH_INT_EN_MASK)
		dev_err(csi_dma_hw->dev, "csi_dma pixsel mismatch\n");

	if (status & TIMEOUT_INT_EN_MASK)
		dev_err(csi_dma_hw->dev, "csi_dma timeout\n");

	spin_unlock_irqrestore(&csi_dma_hw->slock, flags);

	return IRQ_HANDLED;
}

int csi_dma_clk_enable(struct csi_dma_hw_dev *csi_dma)
{
	struct device *dev = &csi_dma->pdev->dev;
	int ret;

	ret = clk_prepare_enable(csi_dma->apbclk);
	if (ret < 0) {
		dev_err(dev, "%s, enable clk error\n", __func__);
		return ret;
	}

	ret = clk_prepare_enable(csi_dma->sclk);
	if (ret < 0) {
		dev_err(dev, "%s, enable clk error\n", __func__);
		return ret;
	}

	return 0;
}

static void csi_dma_clk_disable(struct csi_dma_hw_dev *csi_dma)
{
	clk_disable_unprepare(csi_dma->apbclk);
	clk_disable_unprepare(csi_dma->sclk);
}

int csi_dma_resets_assert(struct csi_dma_hw_dev *csi_dma)
{
	struct device *dev = &csi_dma->pdev->dev;
	int ret;

	if (!csi_dma->csibridge_reset)
		return -EINVAL;

	ret = reset_control_assert(csi_dma->csibridge_reset);
	if (ret) {
		dev_err(dev, "Failed to assert isi proc reset control\n");
		return ret;
	}

	return ret;
}
EXPORT_SYMBOL_GPL(csi_dma_resets_assert);

int csi_dma_resets_deassert(struct csi_dma_hw_dev *csi_dma)
{
	if (!csi_dma->csibridge_reset)
		return -EINVAL;
	reset_control_deassert(csi_dma->csibridge_reset);

	return 0;
}
EXPORT_SYMBOL_GPL(csi_dma_resets_deassert);

#ifdef CONFIG_PM

static int csi_bridge_suspend(struct csi_dma_hw_dev *hw)
{
	struct csi_dma_hw_dev *csi_bridge_hw = hw;

	dev_info(csi_bridge_hw->dev, "csi bridge suspend enter\n");

	csi_dma_clk_disable(csi_bridge_hw);
	csi_dma_resets_assert(csi_bridge_hw);

	return 0;
}

static int csi_bridge_resume(struct csi_dma_hw_dev *hw)
{
	struct csi_dma_hw_dev *csi_bridge_hw = hw;
	int ret;
	u64 freq;

	dev_info(csi_bridge_hw->dev, "csi bridge resume enter\n");

	ret = csi_dma_clk_enable(csi_bridge_hw);
	if (ret < 0) {
		dev_err(csi_bridge_hw->dev, "CSI_DMA_%d enable clocks fail\n", csi_bridge_hw->id);
		return ret;
	}

	ret = csi_dma_resets_deassert(csi_bridge_hw);
	if (ret < 0) {
		dev_err(csi_bridge_hw->dev, "CSI_DMA_%d deassert resets fail\n", csi_bridge_hw->id);
		return ret;
	}

	/*Convert frequency to HZ*/
	freq = csi_bridge_hw->sys_clk_freq * 1000 * 1000;
#ifdef CIX_VI_SET_RATE
	clk_set_rate(csi_bridge_hw->sclk, freq);
#endif
	dev_info(csi_bridge_hw->dev, "CSI_DMA sckl is %lld\n", freq);

	return 0;
}
#endif

static const struct csi_bridge_hw_drv_data csi_bridge_drv_data = {
	.csi_bridge_hw_start = csi_dma_bridge_start,
	.csi_bridge_hw_stop = csi_dma_bridge_stop,
	.csi_bridge_updata_plane_addr = csi_bridge_set_plane_buffer_addr,
	.csi_bridge_hw_resume = csi_bridge_resume,
	.csi_bridge_hw_suspend = csi_bridge_suspend,
	.csi_bridge_callback_register = csi_bridge_buffer_done_callback_register,
	.csi_bridge_enable_irq = csi_bridge_hw_enable_irq,
	.csi_bridge_store = csi_bridge_store,
	.csi_bridge_restore = csi_bridge_restore,
};

static const struct of_device_id csi_dma_hw_of_match[] = {
	{ .compatible = "cix,cix-bridge-hw", .data = &csi_bridge_drv_data},
	{ /* sentinel */ },
};
MODULE_DEVICE_TABLE(of, csi_dma_hw_of_match);

static const struct acpi_device_id csi_dma_hw_acpi_match[] = {
	{ .id = "CIXH3028", .driver_data = (unsigned long)&csi_bridge_drv_data},
	{ /* sentinel */ },
};
MODULE_DEVICE_TABLE(acpi, csi_dma_hw_acpi_match);

static int csi_dma_hw_parse(struct csi_dma_hw_dev *csi_dma_hw)
{
	struct device *dev = &csi_dma_hw->pdev->dev;
	struct device_node *node = dev->of_node;
	struct resource *res;
	struct reset_control *reset;
	int irq = 0;
	int ret = 0;

	if (has_acpi_companion(dev)) {
		ret = device_property_read_u32(dev, "csi-bridge-id", &csi_dma_hw->id);
	} else {
		ret = csi_dma_hw->id =
			of_alias_get_id(node, CIX_BRIDGE_OF_NODE_NAME);
	}

	if ((ret < 0) || (csi_dma_hw->id >= CIX_BRIDGE_MAX_DEVS)) {
		dev_err(dev, "Invalid driver data or device id (%d)\n",
			csi_dma_hw->id);
		return -EINVAL;
	}

	dev_info(dev, "csi bridge hw id %d\n", csi_dma_hw->id);

	res = platform_get_resource(csi_dma_hw->pdev, IORESOURCE_MEM, 0);
	csi_dma_hw->regs = devm_ioremap_resource(dev, res);
	if (IS_ERR(csi_dma_hw->regs)) {
		dev_err(dev, "Failed to get csi-bridge register map\n");
		return PTR_ERR(csi_dma_hw->regs);
	}

	ret = device_property_read_u32(dev, "axi-uid", &csi_dma_hw->axi_uid);
	if (ret < 0) {
		dev_err(dev, "%s failed to get axi user id property\n",
			__func__);
		return ret;
	}

	/*clk & reset*/
	csi_dma_hw->sclk = devm_clk_get(dev, "dma_sclk");
	if (IS_ERR(csi_dma_hw->sclk)) {
		dev_err(dev, "failed to get csi bridge dma_sclk\n");
		return PTR_ERR(csi_dma_hw->sclk);
	}

	csi_dma_hw->apbclk = devm_clk_get(dev, "dma_pclk");
	if (IS_ERR(csi_dma_hw->apbclk)) {
		dev_err(dev, "failed to get csi bridge dma_pclk\n");
		return PTR_ERR(csi_dma_hw->apbclk);
	}

	reset = devm_reset_control_get(dev, "csibridge_reset");
	if (IS_ERR(reset)) {
		if (PTR_ERR(reset) != -EPROBE_DEFER)
			dev_err(dev, "Failed to get sky1  reset control\n");
		return PTR_ERR(reset);
	}

	csi_dma_hw->csibridge_reset = reset;
	irq = platform_get_irq(csi_dma_hw->pdev, 0);
	if (irq < 0) {
		dev_err(dev, ":irq = %d failed to get IRQ resource\n", irq);
		return -1;
	}

	ret = devm_request_irq(dev, irq, csi_dma_irq_handler,
			       IRQF_ONESHOT | IRQF_SHARED, dev_name(dev),
			       csi_dma_hw);
	if (ret) {
		dev_err(dev, "failed to install irq (%d)\n", ret);
		return -1;
	}

	return 0;
}

static int csi_dma_hw_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct csi_dma_hw_dev *csi_dma_hw;
	int ret = 0;

	csi_dma_hw = devm_kzalloc(dev, sizeof(*csi_dma_hw), GFP_KERNEL);
	if (!csi_dma_hw)
		return -ENOMEM;

	csi_dma_hw->pdev = pdev;
	csi_dma_hw->dev = dev;
	ret = csi_dma_hw_parse(csi_dma_hw);
	if (ret < 0)
		return ret;

	csi_dma_hw->drv_data = device_get_match_data(dev);

	if (csi_dma_hw->drv_data == NULL)
		return -1;

	spin_lock_init(&csi_dma_hw->slock);
	mutex_init(&csi_dma_hw->lock);

	platform_set_drvdata(pdev, csi_dma_hw);

	pm_runtime_enable(dev);
	dev_info(dev, "csi bridge_hw %d probe success\n", csi_dma_hw->id);

	return 0;
}

static int csi_dma_hw_remove(struct platform_device *pdev)
{
	struct csi_dma_hw_dev *csi_dma_hw = platform_get_drvdata(pdev);

	pm_runtime_disable(&csi_dma_hw->pdev->dev);
	dev_info(csi_dma_hw->dev, "csi_dma remove\n");

	return 0;
}

static struct platform_driver csi_dma_hw_driver = {
	.probe = csi_dma_hw_probe,
	.remove = csi_dma_hw_remove,
	.driver = {
		.of_match_table = csi_dma_hw_of_match,
		.acpi_match_table =
			   ACPI_PTR(csi_dma_hw_acpi_match),
		.name = CSI_DMA_HW_DRIVER_NAME,
	}
};
module_platform_driver(csi_dma_hw_driver);

MODULE_AUTHOR("Cixtech, Inc.");
MODULE_DESCRIPTION("SKY CSI Bridge Hardware driver");
MODULE_LICENSE("GPL");
