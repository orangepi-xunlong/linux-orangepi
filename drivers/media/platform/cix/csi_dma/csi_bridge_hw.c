/*
* csi_bridge_hw.c - csi_dma register control
*
* Copyright 2024 Cix Technology Group Co., Ltd.
*
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

void csi_dma_store(struct csi_dma_dev *csi_dma)
{
	u32 value;
	u32 i;

	for (i = 0; i < ARRAY_SIZE(store_table); i++) {
		value = readl(csi_dma->regs + store_table[i].offset);
		store_table[i].value = value;
	}
}

void csi_dma_restore(struct csi_dma_dev *csi_dma)
{
	u32 i;

	for (i = 0; i < ARRAY_SIZE(store_table); i++) {
		writel(store_table[i].value,csi_dma->regs + store_table[i].offset);
	}
}

#ifdef CSI_DMA_DEBUG
void dump_csi_bridge_regs(struct csi_dma_dev *csi_dma)
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
		/*{ 0x140, "NSAID" },
		{ 0x144, "AW_PORT" },*/
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
		dev_info(dev, "%20s[0x%.2x]: %.2x\n",
			registers[i].name, registers[i].offset, reg);
	}
}
#endif

static inline u32 csi_dma_read(struct csi_dma_dev *csi_dma, u32 reg)
{
	return readl(csi_dma->regs + reg);
}

static inline void csi_dma_write(struct csi_dma_dev *csi_dma, u32 reg, u32 val)
{
	writel(val, csi_dma->regs + reg);
}

int csi_dma_read_async_fifo_status(struct csi_dma_dev *csi_dma)
{
	u8 val;

	val = csi_dma_read(csi_dma, BRIDGE_STATUS);
	return (val & ASYNC_FIFO_STATUS_MASK) >> ASYNC_FIFO_STATUS_OFFSET;
}

int csi_dma_read_dma_engine_status(struct csi_dma_dev *csi_dma)
{
	u8 val;

	val = csi_dma_read(csi_dma, BRIDGE_STATUS);
	return (val & DMA_ENGINE_STATUS_MASK) >> DMA_ENGINE_STATUS_OFFSET;
}

static void csi_dma_set_line_stride(struct csi_dma_dev *csi_dma, u32 stride)
{
	csi_dma_write(csi_dma, LINE_STRIDE, stride);
}

struct device *csi_dma_dev_get_parent(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct device_node *parent;
	struct platform_device *parent_pdev;

	if (!pdev)
		return NULL;

	/* Get parent for isi capture device */
	parent = of_get_parent(dev->of_node);
	parent_pdev = of_find_device_by_node(parent);
	if (!parent_pdev) {
		of_node_put(parent);
		return NULL;
	}
	of_node_put(parent);

	return &parent_pdev->dev;
}
EXPORT_SYMBOL_GPL(csi_dma_dev_get_parent);

struct csi_dma_dev *csi_dma_get_hostdata(struct platform_device *pdev)
{
	struct csi_dma_dev *csi_dma;

	if (!pdev || !pdev->dev.parent)
		return NULL;

	csi_dma = (struct csi_dma_dev *)dev_get_drvdata(pdev->dev.parent);
	if (!csi_dma)
		return NULL;

	return csi_dma;
}
EXPORT_SYMBOL_GPL(csi_dma_get_hostdata);

void csi_dma_channel_set_outbuf(struct csi_dma_dev *csi_dma,
				struct csi_dma_buffer *buf)
{
	struct vb2_buffer *vb2_buf = &buf->v4l2_buf.vb2_buf;
	struct frame_addr *paddr = &buf->paddr;
	struct csi_dma_cap_dev *dma_cap;
	struct v4l2_pix_format_mplane *pix;
	u32 val = 0;

	if (buf->discard) {
		dma_cap = csi_dma->dma_cap;
		pix = &dma_cap->pix;

		paddr->y = dma_cap->discard_buffer_dma[0];
		if (CSI_DMA_MAX_PLANES == pix->num_planes) {
			paddr->cb = dma_cap->discard_buffer_dma[1];
		}
	} else {
		paddr->y = vb2_dma_contig_plane_dma_addr(vb2_buf, 0);
		if (CSI_DMA_MAX_PLANES == vb2_buf->num_planes) {
			paddr->cb = vb2_dma_contig_plane_dma_addr(vb2_buf, 1);
		}
	}

	val = paddr->y & ADDRESS0_LO_MASK;
	csi_dma_write(csi_dma, AXI_ADDR_START0_LOW, val);

	val = (paddr->y >> 32) & ADDRESS0_HI_MASK;
	csi_dma_write(csi_dma, AXI_ADDR_START0_HIGH, val);

	if (CSI_DMA_MAX_PLANES == vb2_buf->num_planes) {
		val = paddr->cb & ADDRESS1_LO_MASK;
		csi_dma_write(csi_dma, AXI_ADDR_START1_LOW, val);

		val = (paddr->cb >> 32) & ADDRESS1_HI_MASK;
		csi_dma_write(csi_dma, AXI_ADDR_START1_HIGH, val);
	}
}
EXPORT_SYMBOL_GPL(csi_dma_channel_set_outbuf);

static void csi_dma_reset(struct csi_dma_dev *csi_dma)
{
	// TODO
}

u32 csi_dma_axi_burst_config(struct csi_dma_dev *csi_dma, u32 burst_len)
{
	u32 val;

	if (burst_len > 32) {
		dev_err(&csi_dma->pdev->dev, "invalid burst length %d\n",
					burst_len);
		return -1;
	}

	val = csi_dma_read(csi_dma, AXI_CTRL);
	val |= AXI_CTRL_BURST_LEN << BURST_LEN_OFFSET;

	csi_dma_write(csi_dma, AXI_CTRL, val);
	return 0;
}
EXPORT_SYMBOL_GPL(csi_dma_axi_burst_config);

u32 csi_dma_axi_ot_config(struct csi_dma_dev *csi_dma, u32 ot_y, u32 ot_u)
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
	val |= (ot_y << OT_CFG_Y_OFFSET) |
		(ot_u << OT_CFG_U_OFFSET);
	csi_dma_write(csi_dma, OT_INFO_CFG, val);
	return 0;
}
EXPORT_SYMBOL_GPL(csi_dma_axi_ot_config);

u32 csi_dma_axi_pendreq_config(struct csi_dma_dev *csi_dma, u32 pend_y, u32 pend_u)
{
	u32 val;

	if ((pend_y > 64) || (pend_u > 64)) {
		dev_err(&csi_dma->pdev->dev, "invalid pending y %d pending u %d\n",
					pend_y, pend_u);
		return -1;
	}

	val = csi_dma_read(csi_dma, OT_INFO_CFG);
	val |= (pend_y << PENDING_REQ_NUM_Y_OFFSET) |
		(pend_u << PENDING_REQ_NUM_U_OFFSET);
	csi_dma_write(csi_dma, OT_INFO_CFG, val);
	return 0;
}
EXPORT_SYMBOL_GPL(csi_dma_axi_pendreq_config);

static void csi_dma_axi_config(struct csi_dma_dev *csi_dma)
{
	u32 val;

	val = csi_dma->axi_uid << STREAM_ID_OFFSET;
	csi_dma_write(csi_dma, AXI_USER0, val);
	csi_dma_write(csi_dma, AXI_CTRL, 0x3d3F);
	csi_dma_write(csi_dma, OT_INFO_CFG, 0x40);
}

static void csi_dma_frame_config(struct csi_dma_dev *csi_dma,
					struct csi_dma_frame *src_f)
{
	u32 val;
	u8 mem_layout_fmt;
	u8 frame_cfg_dt;

	dev_info(&csi_dma->pdev->dev, "%s enter src_f->fmt->mbus_code = 0x%x\n", __func__, src_f->fmt->mbus_code);
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
			frame_cfg_dt = IMG_DT_YUV420_8BIT;
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
		(src_f->height<< LINE_NUM_OFFSET);

	csi_dma_write(csi_dma, FRAME_CFG, val);

	val = (mem_layout_fmt << MEM_LAYOUT_FMT_OFFSET) |
		(STRIDE_16BYTE << STRIDE_OFFSET) | (frame_cfg_dt << DT_SYNC_FIFO_OFFSET);

	if (IMG_DT_LEGACY_YUV420_8BIT == frame_cfg_dt ) {
		val |= LEGACY_YUV << LEGACY_YUV_OFFSET;
	}

	csi_dma_write(csi_dma, OUT_CTRL, val);

	dev_info(&csi_dma->pdev->dev, "%s exit\n", __func__);
}

static void csi_dma_line_stride_config(struct csi_dma_dev *csi_dma,
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
	dev_info(&csi_dma->pdev->dev, "mbus_code = 0x%x, line_stride = %d, src_f->width = %d\n",
			src_f->fmt->mbus_code, val, src_f->width);
	csi_dma_set_line_stride(csi_dma, val);
}

static void csi_dma_counter_ctrl(struct csi_dma_dev *csi_dma)
{
	u32 val;

	val = (PIXEL_TOTAL_CNT_EN << PIXEL_TOTAL_CNT_EN_OFFSET) |
		(LINE_TOTAL_CNT_EN << LINE_TOTAL_CNT_EN_OFFSET) |
		(FRAME_CNT_EN << FRAME_CNT_EN_OFFSET) |
		(PIXEL_CNT_PER_LINE_EN << PIXEL_CNT_PER_LINE_EN_OFFSET) |
		(LINE_CNT_PER_FRAME_EN << LINE_CNT_PER_FRAME_EN_OFFSET);

	csi_dma_write(csi_dma, CNT_EN_CTRL, val);
}

void csi_dma_pwr_ctrl(struct csi_dma_dev *csi_dma)
{
	u32 val;

	val = SRAM_SD << SRAM_SD_OFFSET;

	csi_dma_write(csi_dma, PWR_CTRL, val);
}
EXPORT_SYMBOL_GPL(csi_dma_pwr_ctrl);

u32 csi_dma_get_pwr_status(struct csi_dma_dev *csi_dma)
{
	u32 val;

	val = csi_dma_read(csi_dma, PWR_CTRL);
	return (val & SRAM_PM_STA_MASK) >> SRAM_PM_STA_OFFSET;
}
EXPORT_SYMBOL_GPL(csi_dma_get_pwr_status);

void csi_dma_fifo_start_stream(struct csi_dma_dev *csi_dma)
{
	u32 val;

	val = ASYNC_BRIDGE_EN << ASYNC_BRIDGE_EN_OFFSET;

	csi_dma_write(csi_dma, CSI_BRIDGE_CTRL, val);

	val = START << FIFO_START_OFFSET;

	csi_dma_write(csi_dma, DMA_BRIDGE_CTRL, val);
}

void csi_dma_dma_start_stream(struct csi_dma_dev *csi_dma)
{
	u32 val;

	val = DMA_BRIDGE_EN << DMA_BRIDGE_EN_OFFSET;

	csi_dma_write(csi_dma, CSI_BRIDGE_CTRL, val);

	val = START << DMA_START_OFFSET;

	csi_dma_write(csi_dma, DMA_BRIDGE_CTRL, val);
}

void csi_dma_fifo_stop_stream(struct csi_dma_dev *csi_dma)
{
	u32 val;

	val = ASYNC_BRIDGE_OFF << ASYNC_BRIDGE_EN_OFFSET;

	csi_dma_write(csi_dma, CSI_BRIDGE_CTRL, val);

	val = STOP << FIFO_START_OFFSET;

	csi_dma_write(csi_dma, DMA_BRIDGE_CTRL, val);
}

void csi_dma_dma_stop_stream(struct csi_dma_dev *csi_dma)
{
	u32 val;

	val = DMA_BRIDGE_OFF << DMA_BRIDGE_EN_OFFSET;

	csi_dma_write(csi_dma, CSI_BRIDGE_CTRL, val);

	val = STOP << DMA_START_OFFSET;

	csi_dma_write(csi_dma, DMA_BRIDGE_CTRL, val);
}

/* start DMA Bridge
 * 1: release the reset
 * 2: setup the axi configurations including burst length, burst type, axcache,nsaid, streamID tec
 * 3: setup frame configuration (line number & pixel number)
 * 4: enable the counter as needed
 * 5: enable  interrupt as needed
 * 6: start the stream by configure the register DMA_BRIDGE_CTRL[0] to 1
 */
void csi_dma_bridge_start(struct csi_dma_dev *csi_dma,
			    struct csi_dma_frame *src_f)
{
	dev_info(&csi_dma->pdev->dev,"csi_dma_bridge_start: %px %px %px\n",csi_dma, src_f, src_f->fmt);

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

	/* step6 */
	csi_dma_dma_start_stream(csi_dma);

	/* DUMP all csi_dma register*/
#ifdef CSI_DMA_DEBUG
	dump_csi_bridge_regs(csi_dma);
#endif

	dev_info(&csi_dma->pdev->dev, "csi_dma_bridge_start exit\n");
}
EXPORT_SYMBOL_GPL(csi_dma_bridge_start);

/* stop DMA Bridge
 * 1: When DMA bridge is working
 * 2: Read Status register, it should be in running status (address 04)
 * 3: Write Regiter bit DMA_BRIDGE_CTRL[1]
 * 4: Wait for stop interrupt
 * 5: Read Status it will be changed to stop status .
 */
void csi_dma_bridge_stop(struct csi_dma_dev *csi_dma)
{
	u32 val;
	u32 cnt = 0;
	dev_info(&csi_dma->pdev->dev, "csi_dma_bridge_stop enter\n");

	csi_dma_disable_irq(csi_dma);
	/* step1 */
	val = csi_dma_read(csi_dma, CSI_BRIDGE_CTRL);
	if (val & DMA_BRIDGE_EN_MASK) {
		dev_info(&csi_dma->pdev->dev, "dma bridge is enable\n");

		/* step2 */
		val = csi_dma_read(csi_dma, DMA_BRIDGE_CTRL);
		if (val & DMA_START_MASK) {
			dev_info(&csi_dma->pdev->dev, "dma bridge is working\n");
		} else {
			dev_info(&csi_dma->pdev->dev, "dma bridge not working\n");
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
			dev_info(&csi_dma->pdev->dev, "success acquire stop interrupt\n");
			break;
		} else {
			cnt++;
			dev_info(&csi_dma->pdev->dev, "%s cnt = %d\n", __func__, cnt);
			//for emu here comment temporary
			//msleep(100);
		}
	} while(cnt >= CHECK_MAX_CNT);

	if (cnt >= CHECK_MAX_CNT) {
		dev_err(&csi_dma->pdev->dev, "wait for stop interrupt timeout\n");
	}

	/* Step5 */
	val = csi_dma_read(csi_dma, DMA_BRIDGE_CTRL);
	if (val & DMA_STOP_MASK) {
		dev_info(&csi_dma->pdev->dev, "dma bridge be changed to stop status \n");
	}

	dev_info(&csi_dma->pdev->dev, "csi_dma_bridge_stop exit\n");

}
EXPORT_SYMBOL_GPL(csi_dma_bridge_stop);

void csi_dma_clean_registers(struct csi_dma_dev *csi_dma)
{
	u32 status;

	status = csi_dma_get_irq_status(csi_dma);
	csi_dma_clean_irq_status(csi_dma, status);
}
EXPORT_SYMBOL_GPL(csi_dma_clean_registers);

void csi_dma_set_timeout(struct csi_dma_dev *csi_dma, u32 val)
{
	csi_dma_write(csi_dma, TIMEOUT_CTRL, val);
}
EXPORT_SYMBOL_GPL(csi_dma_set_timeout);

void csi_dma_channel_enable(struct csi_dma_dev *csi_dma)
{
	csi_dma_clean_registers(csi_dma);
	csi_dma_enable_irq(csi_dma);
#ifdef CSI_DMA_DEBUG
	dump_csi_bridge_regs(csi_dma);
#endif
#ifndef CSIDMA_DEBUG
	msleep(300);
#endif
}
EXPORT_SYMBOL_GPL(csi_dma_channel_enable);

void csi_dma_channel_disable(struct csi_dma_dev *csi_dma)
{
	csi_dma_disable_irq(csi_dma);
}
EXPORT_SYMBOL_GPL(csi_dma_channel_disable);

void csi_dma_enable_irq(struct csi_dma_dev *csi_dma)
{
	u32 val;

	val = FRAME_END_INT_EN_MASK |
		LINE_CNT_INT_EN_MASK |
		LINE_MODE_INT_EN_MASK |
		PIXEL_ERR_INT_EN_MAKS |
		ASYNC_FIFO_OVF_INT_EN_MASK |
		ASYNC_FIFO_UNDF_INT_EN_MASK |
		DMA_OVF_INT_EN_MASK |
		FRAME_STOP_INT_EN_MASK |
		DMA_ERR_RST_INT_EN_MASK |
		UNSUPPORT_DT_INT_EN_MASK |
		UNSUPPORT_STRIDE_INT_EN_MASK |
		LINE_MISMATCH_INT_EN_MASK |
		PIXEL_MISMATCH_INT_EN_MASK |
		TIMEOUT_INT_EN_MASK;

	csi_dma_write(csi_dma, INTERRUTP_EN, val);
}
EXPORT_SYMBOL_GPL(csi_dma_enable_irq);

void csi_dma_disable_irq(struct csi_dma_dev *csi_dma)
{
	csi_dma_write(csi_dma, INTERRUTP_EN, 0);
}
EXPORT_SYMBOL_GPL(csi_dma_disable_irq);

u32 csi_dma_get_irq_status(struct csi_dma_dev *csi_dma)
{
	return csi_dma_read(csi_dma, INTERRUPT_STATUS);
}
EXPORT_SYMBOL_GPL(csi_dma_get_irq_status);

void csi_dma_clean_irq_status(struct csi_dma_dev *csi_dma, u32 val)
{
	csi_dma_write(csi_dma, INTERRUPT_CLR, val);
}
EXPORT_SYMBOL_GPL(csi_dma_clean_irq_status);

int csi_dma_clk_enable(struct csi_dma_dev *csi_dma)
{
        struct device *dev = &csi_dma->pdev->dev;
        int ret;

	dev_info(dev, "%s enter\n", __func__);
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
EXPORT_SYMBOL_GPL(csi_dma_clk_enable);

void csi_dma_clk_disable(struct csi_dma_dev *csi_dma)
{
        struct device *dev = &csi_dma->pdev->dev;

	dev_info(dev, "%s enter\n", __func__);
        clk_disable_unprepare(csi_dma->apbclk);
        clk_disable_unprepare(csi_dma->sclk);
        dev_info(dev, "disable sky1 DMA clock done\n");
}
EXPORT_SYMBOL_GPL(csi_dma_clk_disable);

int csi_dma_resets_assert(struct csi_dma_dev *csi_dma)
{
        struct device *dev = &csi_dma->pdev->dev;
        int ret;

	dev_info(dev, "%s enter\n", __func__);
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

int csi_dma_resets_deassert(struct csi_dma_dev *csi_dma)
{
        struct device *dev = &csi_dma->pdev->dev;

        dev_info(dev, "%s enter\n", __func__);
        if (!csi_dma->csibridge_reset)
                return -EINVAL;
        reset_control_deassert(csi_dma->csibridge_reset);
        return 0;
}
EXPORT_SYMBOL_GPL(csi_dma_resets_deassert);

MODULE_AUTHOR("Cixtech, Inc.");
MODULE_DESCRIPTION("SKY CSI Bridge Hardware driver");
MODULE_LICENSE("GPL");
