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

#include "cdns-csi2rx.h"
#include "csi_common.h"

#ifdef CONFIG_VI_BBOX
#include <linux/soc/cix/rdr_pub.h>
#include <mntn_public_interface.h>
#endif

#define CIX_MIPI_CSI2_DRIVER_NAME	"cix-mipi-csi2"
#define CIX_MIPI_CSI2_SUBDEV_NAME	CIX_MIPI_CSI2_DRIVER_NAME
#define CIX_MIPI_OF_NODE_NAME		"cix-csi"
#define CIX_MIPI_MAX_DEVS		(0x04)

#define DATA_PATH_WIDTH		(8 * 4)

enum cix_csi2_pads {
	CIX_MIPI_CSI2_PAD_SINK,
	CIX_MIPI_CSI2_PAD_SOURCE,
	CIX_MIPI_CSI2_PAD_MAX,
};

struct mipi_csi2_info {
	irqreturn_t (*mipi_csi2_isr)(s32 irq, void *subdev);
	irqreturn_t (*mipi_csi2_err_isr)(s32 irq, void *subdev);
};

static const char *mipi_csi2_clk_names[4] = {
	"csi_p0clk", "csi_p1clk", "csi_p2clk", "csi_p3clk"
};

#ifdef CONFIG_VI_BBOX
#define MIPI_CSI2_RDR_NUM  25
static int rdr_writen_num = 0;
static u32 rdr_register_probe;
u64 g_csi_addr;

enum RDR_CSI_MODID {
	RDR_CSI_MODID_START = PLAT_BB_MOD_CSI_START,
	RDR_CSI_SOC_ISR_ERR_MODID,
	RDR_CSI_MODID_END = PLAT_BB_MOD_CSI_END,
};

static struct rdr_register_module_result g_current_info;

static struct rdr_exception_info_s g_csi_einfo[] = {
	{ { 0, 0 }, RDR_CSI_SOC_ISR_ERR_MODID, RDR_CSI_SOC_ISR_ERR_MODID, RDR_ERR,
	 RDR_REBOOT_NO, RDR_CSI, RDR_CSI, RDR_CSI,
	 (u32)RDR_REENTRANT_DISALLOW, CSI_S_EXCEPTION, 0, (u32)RDR_UPLOAD_YES,
	 "csi", "csi isr proc", 0, 0, 0 },
};


static struct completion g_rdr_dump_comp;

/*
 * Description : Dump function of the AP when an exception occurs
 */
static void cix_csi_rproc_rdr_dump(u32 modid, u32 etype,
				   u64 coreid, char *log_path,
				   pfn_cb_dump_done pfn_cb)
{
	if (pfn_cb)
		pfn_cb(modid, coreid);
}

/*
 * Description : register exception with the rdr
 */
static void cix_csi_rproc_rdr_register_exception(void)
{
	unsigned int i;
	int ret;

	for (i = 0; i < sizeof(g_csi_einfo) / sizeof(struct rdr_exception_info_s); i++) {
		pr_debug("register exception:%u", g_csi_einfo[i].e_exce_type);
		ret = rdr_register_exception(&g_csi_einfo[i]);
		if (ret == 0) {
			pr_err("rdr_register_exception fail, ret = [%d]\n", ret);
			return;
		}
	}
}

static void cix_csi_rproc_rdr_unregister_exception(void)
{
	unsigned int i;

	for (i = 0; i < sizeof(g_csi_einfo) / sizeof(struct rdr_exception_info_s); i++) {
		pr_debug("unregister exception:%u", g_csi_einfo[i].e_exce_type);
		rdr_unregister_exception(g_csi_einfo[i].e_modid);
	}
}

/*
 * Description : Register the dump and reset functions to the rdr
 */
static int cix_csi_rproc_rdr_register_core(void)
{
	struct rdr_module_ops_pub s_csi_ops;
	struct rdr_register_module_result retinfo;
	u64 coreid = RDR_CSI;
	int ret;

	s_csi_ops.ops_dump = cix_csi_rproc_rdr_dump;
	s_csi_ops.ops_reset = NULL;

	ret = rdr_register_module_ops(coreid, &s_csi_ops, &retinfo);
	if (ret < 0) {
		pr_err("rdr_register_module_ops fail, ret = [%d]\n", ret);
		return ret;
	}

	g_current_info.log_addr = retinfo.log_addr;
	g_current_info.log_len = retinfo.log_len;
	g_current_info.nve = retinfo.nve;

	g_csi_addr =
		(uintptr_t)rdr_bbox_map(g_current_info.log_addr, g_current_info.log_len);
	if (!g_csi_addr) {
		pr_err("hisi_bbox_map g_hisiap_addr fail\n");
		return -1;
	}

	pr_debug("%s,%d: addr=0x%llx   [0x%llx], len=0x%x\n",
		__func__, __LINE__, g_current_info.log_addr, g_csi_addr, g_current_info.log_len);

	return ret;
}

static void cix_csi_rproc_rdr_unregister_core(void)
{
	u64 coreid = RDR_CSI;

	rdr_unregister_module_ops(coreid);
}

#endif

static inline u32 mipi_csi2_read(struct csi2rx_priv *csi2dev, u32 reg)
{
	return readl(csi2dev->base + reg);
}

static inline void mipi_csi2_write(struct csi2rx_priv *csi2dev, u32 reg, u32 val)
{
	writel(val, csi2dev->base + reg);
}

static struct media_pad *cix_csi2_get_remote_sensor_pad(struct csi2rx_priv *csi2dev)
{
	struct v4l2_subdev *subdev = &csi2dev->subdev;
	struct media_pad *sink_pad, *source_pad;
	int i;

	source_pad = NULL;
	for (i = 0; i < subdev->entity.num_pads; i++) {
		sink_pad = &subdev->entity.pads[i];

		if (sink_pad->flags & MEDIA_PAD_FL_SINK) {
			source_pad = media_pad_remote_pad_first(sink_pad);
			if (source_pad)
				/* return first pad point in the loop  */
				return source_pad;
		}
	}

	if (i == subdev->entity.num_pads)
		v4l2_err(&csi2dev->subdev, "%s, No remote pad found!\n", __func__);

	return NULL;
}
static struct v4l2_subdev *cix_get_remote_subdev(struct csi2rx_priv *csi2dev,
						 const char * const label)
{
	struct media_pad *source_pad;
	struct v4l2_subdev *sen_sd;

	/* Get remote source pad */
	source_pad = cix_csi2_get_remote_sensor_pad(csi2dev);
	if (!source_pad) {
		v4l2_err(&csi2dev->subdev, "%s, No remote pad found!\n", label);
		return NULL;
	}

	/* Get remote source pad subdev */
	sen_sd = media_entity_to_v4l2_subdev(source_pad->entity);
	if (!sen_sd) {
		v4l2_err(&csi2dev->subdev, "%s, No remote subdev found!\n", label);
		return NULL;
	}

	return sen_sd;
}

static int cix_csi2_get_sensor_fmt(struct csi2rx_priv *csi2rx)
{
	struct v4l2_mbus_framefmt *mf = &csi2rx->format;
	struct v4l2_subdev *sen_sd;
	struct v4l2_subdev_format src_fmt;
	struct media_pad *source_pad;
	int ret;

	/* Get remote source pad */
	source_pad = cix_csi2_get_remote_sensor_pad(csi2rx);
	if (!source_pad) {
		v4l2_err(&csi2rx->subdev, "%s, No remote pad found!\n", __func__);
		return -EINVAL;
	}

	sen_sd = cix_get_remote_subdev(csi2rx, __func__);
	if (!sen_sd)
		return -EINVAL;

	memset(&src_fmt, 0, sizeof(src_fmt));
	src_fmt.pad = source_pad->index;
	src_fmt.which = V4L2_SUBDEV_FORMAT_ACTIVE;
	ret = v4l2_subdev_call(sen_sd, pad, get_fmt, NULL, &src_fmt);
	if (ret < 0 && ret != -ENOIOCTLCMD)
		return -EINVAL;

	/* Update input frame size and formate  */
	memcpy(mf, &src_fmt.format, sizeof(struct v4l2_mbus_framefmt));

	dev_dbg(csi2rx->dev, "width=%d, height=%d, fmt.code=0x%x\n",
		mf->width, mf->height, mf->code);
	return 0;
}

static int cix_csi2_cal_clk_freq(struct csi2rx_priv *csi2rx)
{
	/***********************TODO***********************************
	**********calibrate the frequency of csi***********************
	**********internal datapath width 32-bits**********************
	**sys_clk = Input Data rate（Mbytes/s）/datapath width(Bytes)**
        **        = Input Data rate（Mbites/s）/8/4********************
	**************************************************************/

	csi2rx->sys_clk_freq = (csi2rx->data_rate_Mbit * csi2rx->num_lanes) / DATA_PATH_WIDTH;

	return 0;

}
static int mipi_csi2_open(struct v4l2_subdev *sd, struct v4l2_subdev_fh *fh)
{
	return 0;
}

static int mipi_csi2_link_setup(struct media_entity *entity,
				const struct media_pad *local,
				const struct media_pad *remote, u32 flags)
{
	if (local->flags & MEDIA_PAD_FL_SOURCE) {

	} else if (local->flags & MEDIA_PAD_FL_SINK) {

	}

	return 0;
}

static const struct media_entity_operations mipi_csi2_sd_media_ops = {
	.link_setup = mipi_csi2_link_setup,
};

/*
 * V4L2 subdev operations
 */

static int mipi_csi2_s_power(struct v4l2_subdev *sd, int on)
{
	struct csi2rx_priv *csi2rx = v4l2_subdev_to_csi2rx(sd);
	struct v4l2_subdev *sen_sd;

	dev_info(csi2rx->dev, "%s enter\n", __func__);

	if(on) {
		dev_info(csi2rx->dev, "mipi-csi2 pm get sync\n");
		pm_runtime_get_sync(csi2rx->dev);
	} else {
		dev_info(csi2rx->dev, "mipi-csi2 pm put sync\n");
		pm_runtime_put(csi2rx->dev);
	}

	sen_sd = cix_get_remote_subdev(csi2rx, __func__);
	if (!sen_sd)
		return -EINVAL;

	return v4l2_subdev_call(sen_sd, core, s_power, on);
}

static int mipi_csi2_g_frame_interval(struct v4l2_subdev *sd,
				struct v4l2_subdev_frame_interval *interval)
{

	struct csi2rx_priv *csi2rx = v4l2_subdev_to_csi2rx(sd);
	struct v4l2_subdev *sen_sd;

	sen_sd = cix_get_remote_subdev(csi2rx, __func__);
	if (!sen_sd)
		return -EINVAL;

	return v4l2_subdev_call(sen_sd, video, g_frame_interval, interval);
}

static int mipi_csi2_s_frame_interval(struct v4l2_subdev *sd,
				struct v4l2_subdev_frame_interval *interval)
{
	struct csi2rx_priv *csi2rx = v4l2_subdev_to_csi2rx(sd);
	struct v4l2_subdev *sen_sd;

	sen_sd = cix_get_remote_subdev(csi2rx, __func__);
	if (!sen_sd)
		return -EINVAL;

	return v4l2_subdev_call(sen_sd, video, s_frame_interval, interval);
}

void mipi_csi2_enable_irq(struct csi2rx_priv *csi2rx)
{
	u32 val;
	dev_info(csi2rx->dev, "%s enter\n", __func__);

	val = MIPI_SP_RCVD_IRQ |
			MIPI_LP_RCVD_IRQ |
			MIPI_SLEEP_IRQ |
			MIPI_WAKEUP_IRQ |
			MIPI_DESKEW_ENTRY_IRQ |
			MIPI_SP_GENERIC_RCVD_IRQ |
			MIPI_EPD_OPTION1_DETECT_IRQ |
			MIPI_STREAM0_STOP_IRQ |
			MIPI_STREAM0_ABORT_IRQ |
			MIPI_STREAM1_STOP_IRQ |
			MIPI_STREAM1_ABORT_IRQ |
			MIPI_STREAM2_STOP_IRQ |
			MIPI_STREAM2_ABORT_IRQ |
			MIPI_STREAM3_STOP_IRQ |
			MIPI_STREAM3_ABORT_IRQ |
			MIPI_STREAM4_STOP_IRQ |
			MIPI_STREAM4_ABORT_IRQ |
			MIPI_STREAM5_STOP_IRQ |
			MIPI_STREAM5_ABORT_IRQ |
			MIPI_STREAM6_STOP_IRQ |
			MIPI_STREAM6_ABORT_IRQ |
			MIPI_STREAM7_STOP_IRQ |
			MIPI_STREAM7_ABORT_IRQ;

	mipi_csi2_write(csi2rx, INFO_IRQS_MASK, val);
	dev_info(csi2rx->dev, "%s exit\n", __func__);
}

static void mipi_csi2_enable_err_irq(struct csi2rx_priv *csi2rx)
{
	u32 val;
	u32 status;

	//clean the err status before enable
	status = mipi_csi2_read(csi2rx, ERROR_IRQS);
	mipi_csi2_write(csi2rx, ERROR_IRQS,status);

	status = mipi_csi2_read(csi2rx, DPHY_ERR_STATUS_IRQ);
	mipi_csi2_write(csi2rx, DPHY_ERR_STATUS_IRQ,status);

	dev_info(csi2rx->dev, "%s enter\n", __func__);
	val = FRONT_FIFO_OVERFLOW_IRQ |
			PAYLOAD_CRC_IRQ |
			HEADER_ECC_IRQ |
			HEADER_CORRECTED_ECC_IRQ |
			DATA_ID_IRQ |
			PROT_TRUNCATED_PACKET_IRQ |
			PROT_FRAME_MISMATCH_IRQ |
			PROT_LINE_MISMATCH_IRQ |
			STREAM0_FIFO_OVERFLOW_IRQ |
			STREAM1_FIFO_OVERFLOW_IRQ |
			STREAM2_FIFO_OVERFLOW_IRQ |
			STREAM3_FIFO_OVERFLOW_IRQ |
			STREAM4_FIFO_OVERFLOW_IRQ |
			STREAM5_FIFO_OVERFLOW_IRQ |
			STREAM6_FIFO_OVERFLOW_IRQ |
			STREAM7_FIFO_OVERFLOW_IRQ;

	mipi_csi2_write(csi2rx, ERROR_IRQS_MASK, val);

	val = DL0_ERRSOTHS_IRQ |
			DL0_ERRSOTSYNCHS_IRQ |
			DL1_ERRSOTHS_IRQ |
			DL1_ERRSOTSYNCHS_IRQ |
			DL2_ERRSOTHS_IRQ |
			DL2_ERRSOTSYNCHS_IRQ |
			DL3_ERRSOTHS_IRQ |
			DL3_ERRSOTSYNCHS_IRQ |
			DL4_ERRSOTHS_IRQ |
			DL4_ERRSOTSYNCHS_IRQ |
			DL5_ERRSOTHS_IRQ |
			DL5_ERRSOTSYNCHS_IRQ |
			DL6_ERRSOTHS_IRQ |
			DL6_ERRSOTSYNCHS_IRQ |
			DL7_ERRSOTHS_IRQ |
			DL7_ERRSOTSYNCHS_IRQ;
	mipi_csi2_write(csi2rx, DPHY_ERR_IRQ_MASK_CFG, val);
	dev_info(csi2rx->dev, "%s exit\n", __func__);
}

static void mipi_csi2_disable_irq(struct csi2rx_priv *csi2rx)
{
	dev_info(csi2rx->dev, "%s enter\n", __func__);
	mipi_csi2_write(csi2rx, INFO_IRQS_MASK, 0);
	dev_info(csi2rx->dev, "%s exit\n", __func__);
}

static void mipi_csi2_disable_err_irq(struct csi2rx_priv *csi2rx)
{
	dev_info(csi2rx->dev, "%s enter\n", __func__);
	mipi_csi2_write(csi2rx, ERROR_IRQS_MASK, 0);
	mipi_csi2_write(csi2rx, DPHY_ERR_IRQ_MASK_CFG, 0);
	dev_info(csi2rx->dev, "%s exit\n", __func__);
}

static int mipi_csi2_stream_start(struct csi2rx_priv *csi2rx)
{
	int val = 0;
	int stream_id = 0;

	dev_info(csi2rx->dev, "mipi_csi2 index %d start\n",csi2rx->id);

	val = CORE_CTRL_START;
	mipi_csi2_write(csi2rx, CORE_CTRL, val);

	val = CL_ENABLE | DPHY_RESET;
	if (csi2rx->num_lanes == 4) {
		val |= (DL0_ENABLE | DL1_ENABLE | DL2_ENABLE | DL3_ENABLE);
	} else if (csi2rx->num_lanes == 2) {
		val |= (DL0_ENABLE | DL1_ENABLE);
	} else if (csi2rx->num_lanes == 1) {
		val |= DL0_ENABLE;
	} else {
		dev_err(csi2rx->dev, "mipi-csi lanes invalid %d \n", csi2rx->num_lanes);
		return -1;
	}

	mipi_csi2_write(csi2rx, DPHY_LANE_CONTROL, val);

	val = (STATIC_CFG_ENABLE_LRTE | (csi2rx->num_lanes<<4));
	mipi_csi2_write(csi2rx,STATIC_CFG_REG, val);

	val = VC_SELECT(0);

	if (csi2rx->id == 2) {
		stream_id = 2;
	}

	mipi_csi2_write(csi2rx, STREAM_DATA_CFG(stream_id), val);

	val = LARGE_BUFFER << FIFO_MODE_OFFSET;
	mipi_csi2_write(csi2rx, STREAM_CFG(stream_id), val);

	val = STEAM_CTRL_START;
	mipi_csi2_write(csi2rx, STREAM_CTRL(stream_id), val);

	return 0;
}

static int mipi_csi2_stream_stop(struct csi2rx_priv *csi2rx)
{
	int val = 0;
	int stream_id = 0;

	dev_info(csi2rx->dev, "mipi_csi2 index %d  stop \n",csi2rx->id);

	if (csi2rx->id == 2) {
		stream_id = 2;
	}

	val = mipi_csi2_read(csi2rx, STREAM_CTRL(stream_id));
	val &= ~STEAM_CTRL_START;
	mipi_csi2_write(csi2rx, STREAM_CTRL(stream_id), val);

	val = mipi_csi2_read(csi2rx, DPHY_LANE_CONTROL);
	val &= ~(CL_ENABLE | DPHY_RESET);
	mipi_csi2_write(csi2rx, DPHY_LANE_CONTROL, val);

	val = mipi_csi2_read(csi2rx, CORE_CTRL);
	val &= ~CORE_CTRL_START;
	mipi_csi2_write(csi2rx, CORE_CTRL, val);

	return 0;
}

static int mipi_csi2_s_stream(struct v4l2_subdev *sd, int enable)
{
	int ret = 0;
	struct csi2rx_priv *csi2rx = v4l2_subdev_to_csi2rx(sd);

	dev_info(csi2rx->dev, "%s: %d, csi2rx %px  count : 0x%x\n",
		__func__, enable, csi2rx, csi2rx->count);

	if (enable) {
		if (!csi2rx->count) {
			mipi_csi2_enable_err_irq(csi2rx);
			csi2rx->count++;
			csi2rx->stream_on = 1;
			ret = mipi_csi2_stream_start(csi2rx);
		}
	} else {
		csi2rx->count--;
		if (!csi2rx->count){
			mipi_csi2_disable_irq(csi2rx);
			mipi_csi2_disable_err_irq(csi2rx);
			mipi_csi2_stream_stop(csi2rx);
			csi2rx->stream_on = 0;
		}
	}

	dev_info(csi2rx->dev, "%s exit csi2rx->count = 0x%x\n", __func__, csi2rx->count);
	return ret;
}

static int mipi_csi2_enum_framesizes(struct v4l2_subdev *sd,
				     struct v4l2_subdev_state *cfg,
				     struct v4l2_subdev_frame_size_enum *fse)
{
	struct csi2rx_priv *csi2rx = v4l2_subdev_to_csi2rx(sd);
	struct v4l2_subdev *sen_sd;

	sen_sd = cix_get_remote_subdev(csi2rx, __func__);
	if (!sen_sd)
		return -EINVAL;

	return v4l2_subdev_call(sen_sd, pad, enum_frame_size, NULL, fse);
}

static int mipi_csi2_enum_frame_interval(struct v4l2_subdev *sd,
					 struct v4l2_subdev_state *cfg,
					 struct v4l2_subdev_frame_interval_enum *fie)
{
	struct csi2rx_priv *csi2rx = v4l2_subdev_to_csi2rx(sd);
	struct v4l2_subdev *sen_sd;

	sen_sd = cix_get_remote_subdev(csi2rx, __func__);

	if (!sen_sd)
		return -EINVAL;

	return v4l2_subdev_call(sen_sd, pad, enum_frame_interval, NULL, fie);
}

static int mipi_csi2_get_fmt(struct v4l2_subdev *sd,
			     struct v4l2_subdev_state *cfg,
			     struct v4l2_subdev_format *fmt)
{
	struct csi2rx_priv *csi2rx = v4l2_subdev_to_csi2rx(sd);
	struct v4l2_mbus_framefmt *mf = &fmt->format;

	dev_info(csi2rx->dev, "mipi_csi2_get_fmt\n");
	cix_csi2_get_sensor_fmt(csi2rx);

	memcpy(mf, &csi2rx->format, sizeof(struct v4l2_mbus_framefmt));
	/* Source/Sink pads crop rectangle size */

	csi2rx->data_rate_Mbit = mf->reserved[0];
	dev_info(csi2rx->dev, " data_rate_mbyte  =0x%x, \n", mf->reserved[0]);
	cix_csi2_cal_clk_freq(csi2rx);

	dev_info(csi2rx->dev, " format.reserved[0]=0x%x, format.reserved[1]=%x, \n",
			mf->reserved[0], mf->reserved[1]);

	mf->reserved[0] = csi2rx->sys_clk_freq;

	dev_info(csi2rx->dev, " format.reserved[0]=0x%x, format.reserved[1]=%x, \n",
			mf->reserved[0], mf->reserved[1]);

	return 0;
}

static u32 mipi_csi2_get_irq_status(struct csi2rx_priv *csi2rx)
{
	return mipi_csi2_read(csi2rx, INFO_IRQS);
}

static void mipi_csi2_clear_irq_status(struct csi2rx_priv *csi2rx, u32 status)
{
	mipi_csi2_write(csi2rx, INFO_IRQS, status);
}

static irqreturn_t mipi_csi2_irq_handler(int irq, void *priv)
{
	struct csi2rx_priv *csi2rx = priv;
	unsigned long flags;
	u32 status;

	spin_lock_irqsave(&csi2rx->slock, flags);

	status = mipi_csi2_get_irq_status(csi2rx);
	mipi_csi2_clear_irq_status(csi2rx, status);

	spin_unlock_irqrestore(&csi2rx->slock, flags);
	return IRQ_HANDLED;
}

static struct err_status mipi_csi2_get_err_irq_status(struct csi2rx_priv *csi2rx)
{
	struct err_status status;

	status.csi_err_status = mipi_csi2_read(csi2rx, ERROR_IRQS);
	status.dphy_err_status = mipi_csi2_read(csi2rx, DPHY_ERR_STATUS_IRQ);

	return status;
}

static void mipi_csi2_clear_err_irq_status(struct csi2rx_priv *csi2rx, struct err_status status)
{
	mipi_csi2_write(csi2rx, ERROR_IRQS, status.csi_err_status);
	mipi_csi2_write(csi2rx, DPHY_ERR_STATUS_IRQ, status.dphy_err_status);
}

#ifdef CONFIG_VI_BBOX
static void err_irq_status_write(struct csi2rx_priv *csi2rx, u64 read_addr, u64* write_addr)
{
	u32 val;
	val = mipi_csi2_read(csi2rx, read_addr);
	writel(val, (volatile void __iomem *)(*write_addr));
	*write_addr = *write_addr + 0x4;
}
#endif

static irqreturn_t mipi_csi2_err_irq_handler(int irq, void *priv)
{
	struct csi2rx_priv *csi2rx = priv;
	unsigned long flags;
	struct err_status status;
	#ifdef CONFIG_VI_BBOX
	const char start_flag_str[] = "csiDumpStart";
	int start_flag_len          = sizeof(start_flag_str) - 1;
	int start_flag_num          = (start_flag_len % 4 ? 1 : 0) + start_flag_len / 4;
	int stream_id               = 0;
	u64 write_shift             = g_csi_addr + rdr_writen_num * 0x4;
	#endif

	dev_info(csi2rx->dev, "---- mipi_csi2_err_irq_handler ----\n");
	spin_lock_irqsave(&csi2rx->slock, flags);

	status = mipi_csi2_get_err_irq_status(csi2rx);
	#ifdef CONFIG_VI_BBOX
	if (status.csi_err_status || status.dphy_err_status) {
		if (((rdr_writen_num + MIPI_CSI2_RDR_NUM + start_flag_num) * 4 <  g_current_info.log_len)) {
			memcpy((void*)write_shift, start_flag_str, start_flag_len);
			write_shift += start_flag_num * 4;
			rdr_writen_num += start_flag_num;

			/* save IRQ ERR_IRQ status */
			err_irq_status_write(csi2rx, INFO_IRQS, &write_shift);
			err_irq_status_write(csi2rx, ERROR_IRQS, &write_shift);
			err_irq_status_write(csi2rx, DPHY_ERR_STATUS_IRQ, &write_shift);

			/* save IRQ / ERR_IRQ MASK status */
			err_irq_status_write(csi2rx, INFO_IRQS_MASK, &write_shift);
			err_irq_status_write(csi2rx, ERROR_IRQS_MASK, &write_shift);
			err_irq_status_write(csi2rx, DPHY_ERR_IRQ_MASK_CFG, &write_shift);

			err_irq_status_write(csi2rx, CORE_CTRL, &write_shift);
			err_irq_status_write(csi2rx, STATIC_CFG_REG, &write_shift);
			err_irq_status_write(csi2rx, DPHY_LANE_CONTROL, &write_shift);

			for (stream_id = 0; stream_id < 4; stream_id++) {
				err_irq_status_write(csi2rx, STREAM_CTRL(stream_id), &write_shift);
				err_irq_status_write(csi2rx, STREAM_STATUS(stream_id), &write_shift);
				err_irq_status_write(csi2rx, STREAM_DATA_CFG(stream_id), &write_shift);
				err_irq_status_write(csi2rx, STREAM_CFG(stream_id), &write_shift);
			}
			rdr_writen_num += MIPI_CSI2_RDR_NUM;
		}
		/* asynchronous api */
		rdr_system_error(RDR_CSI_SOC_ISR_ERR_MODID, 0, 0);
	}
	#endif

	dev_info(csi2rx->dev, "CSI ERROR status 0x%08x, DPHY error status 0x%08x\n",
			status.csi_err_status, status.dphy_err_status);

	mipi_csi2_clear_err_irq_status(csi2rx, status);

	spin_unlock_irqrestore(&csi2rx->slock, flags);
	return IRQ_HANDLED;
}

#ifdef CONFIG_PM
static int mipi_csi2_dev_rpm_suspend(struct device *dev)
{
	struct csi2rx_priv *csi2rx = dev_get_drvdata(dev);
	int i;

	dev_info(dev, "%s() enter!\n", __func__);

	clk_disable_unprepare(csi2rx->sys_clk);
	clk_disable_unprepare(csi2rx->p_clk);

	if ((csi2rx->id == 0) || (csi2rx->id == 2)) {
		for (i = 0; i < 4; i++) {
			clk_disable_unprepare(csi2rx->pixel_clk[i]);
		}
	} else {
		clk_disable_unprepare(csi2rx->pixel_clk[0]);
	}

	reset_control_assert(csi2rx->csi_reset);
	dev_info(dev, "%s() exit!\n", __func__);
	return 0;
}

static int mipi_csi2_dev_rpm_resume(struct device *dev)
{
	struct csi2rx_priv *csi2rx = dev_get_drvdata(dev);
	int i;
	u64 rate;
	dev_info(dev, "%s enter\n", __func__);
	if (clk_prepare_enable(csi2rx->sys_clk)) {
		dev_err(dev, "sys_clk enable failed\n");
		goto err_csi_clks;
	}
	/********************TODO**************************
	****************Convert frequency to HZ************
	***************************************************/
	rate = csi2rx->sys_clk_freq * 1000 * 1000;	//Convert frequency to HZ
	dev_info(dev, "sys_clk freq %lld\n",rate);

	if (clk_prepare_enable(csi2rx->p_clk)) {
		dev_err(dev, "p_clk enable failed\n");
		goto err_csi_clks;
	}

	if ((csi2rx->id == 0) || (csi2rx->id == 2)) {
		for (i = 0; i < 4; i++) {
			if (clk_prepare_enable(csi2rx->pixel_clk[i])) {
				dev_err(dev, "pixel_clk[%d] enable failed\n", i);
				goto err_csi_clks;
			}
		}
	} else {
		if (clk_prepare_enable(csi2rx->pixel_clk[0])) {
			dev_err(csi2rx->dev, "pixel_clk[0] enable failed\n");
			goto err_csi_clks;
		}
	}

#ifdef CIX_VI_SET_RATE
	clk_set_rate(csi2rx->sys_clk,rate);
#endif
	reset_control_deassert(csi2rx->csi_reset);
	return 0;
err_csi_clks:
	return -1;
}
#endif

static int mipi_csi2_async_bound(struct v4l2_async_notifier *notifier,
					struct v4l2_subdev *s_subdev,struct v4l2_async_subdev *asd)
{
	struct v4l2_subdev *subdev = notifier->sd;
	struct csi2rx_priv *csi2rx = v4l2_subdev_to_csi2rx(subdev);

	dev_info(csi2rx->dev, "mipi sync bound enter\n");

	/*find remote entity source pad */
	csi2rx->source_pad = media_entity_get_fwnode_pad(&s_subdev->entity,s_subdev->fwnode,
			MEDIA_PAD_FL_SOURCE);

	if (csi2rx->source_pad < 0) {
		dev_err(csi2rx->dev, "Couldn't find output pad for subdev %s\n",
			s_subdev->name);
		return -1;
	}

	csi2rx->source_subdev = s_subdev;
	dev_info(csi2rx->dev,"mipi bound source subdev %s index %d sink name %s index %d \n",
		csi2rx->source_subdev->name,csi2rx->source_pad,
		csi2rx->subdev.name,CIX_MIPI_CSI2_PAD_SINK);

	return media_create_pad_link(&csi2rx->source_subdev->entity,
				     csi2rx->source_pad,
				     &csi2rx->subdev.entity,CIX_MIPI_CSI2_PAD_SINK,
				     MEDIA_LNK_FL_ENABLED |
				     MEDIA_LNK_FL_IMMUTABLE);
}

static int mipi_csi2_parse_endpoint(struct device *dev,
                               struct v4l2_fwnode_endpoint *vep,
                               struct v4l2_async_subdev *asd)
{
	dev_info(dev, "mipi parse the endpoints\n");

	if (vep->base.port != 0) {
		dev_info(dev, "mipi do not need remote  endpoints port %d id %d \n",vep->base.port,vep->base.id);
		return -ENOTCONN;
	}

	return 0;
}

#ifdef CONFIG_PM_SLEEP
static int mipi_csi2_dev_suspend(struct device *dev)
{
	struct csi2rx_priv *csi2rx = dev_get_drvdata(dev);
	dev_info(dev, "%s enter!\n", __func__);

	if (csi2rx->stream_on == 1) {
		mipi_csi2_disable_err_irq(csi2rx);
	}

	return pm_runtime_force_suspend(dev);
}

static int mipi_csi2_dev_resume(struct device *dev)
{
	struct csi2rx_priv *csi2rx = dev_get_drvdata(dev);
	dev_info(dev, "%s enter!\n", __func__);

	pm_runtime_force_resume(dev);

	if (csi2rx->stream_on == 1) {
		mipi_csi2_enable_err_irq(csi2rx);
		mipi_csi2_stream_start(csi2rx);
	}

	return 0;
}
#endif

static const struct dev_pm_ops mipi_csi2_dev_pm_ops = {
#ifdef CONFIG_PM_SLEEP
	SET_SYSTEM_SLEEP_PM_OPS(mipi_csi2_dev_suspend, mipi_csi2_dev_resume)
#endif
#ifdef CONFIG_PM
	SET_RUNTIME_PM_OPS(mipi_csi2_dev_rpm_suspend, mipi_csi2_dev_rpm_resume, NULL)
#endif
};

static const struct v4l2_subdev_internal_ops mipi_csi2_sd_internal_ops = {
	.open = mipi_csi2_open,
};

static struct v4l2_subdev_pad_ops mipi_csi2_pad_ops = {
	.enum_frame_size = mipi_csi2_enum_framesizes,
	.enum_frame_interval = mipi_csi2_enum_frame_interval,
	.get_fmt = mipi_csi2_get_fmt,
};

static struct v4l2_subdev_core_ops mipi_csi2_core_ops = {
	.s_power = mipi_csi2_s_power,
};

static struct v4l2_subdev_video_ops mipi_csi2_video_ops = {
	.g_frame_interval = mipi_csi2_g_frame_interval,
	.s_stream	  = mipi_csi2_s_stream,
	.s_frame_interval = mipi_csi2_s_frame_interval,
};

static struct v4l2_subdev_ops mipi_csi2_subdev_ops = {
	.core = &mipi_csi2_core_ops,
	.video = &mipi_csi2_video_ops,
	.pad = &mipi_csi2_pad_ops,
};

static const struct v4l2_async_notifier_operations mipi_csi2_notifier_ops = {
	.bound		= mipi_csi2_async_bound,
};

static int mipi_csi2_parse(struct csi2rx_priv *csi2rx)
{
	struct device *dev = csi2rx->dev;
	struct platform_device *pdev = csi2rx->pdev;
	struct mipi_csi2_info *hw_info = NULL;
	int lanes = 0;
	int irq0, irq1;
	int i;
	int ret;

	if (has_acpi_companion(dev)) {
		ret = device_property_read_u8(dev, CIX_MIPI_OF_NODE_NAME, &csi2rx->id);
	} else {
		ret = csi2rx->id = of_alias_get_id(dev->of_node, CIX_MIPI_OF_NODE_NAME);
	}

	if ((ret < 0) || (csi2rx->id >= CIX_MIPI_MAX_DEVS)) {
		dev_err(dev, "invalid mipi device id (%d)\n",
			csi2rx->id);
		return -EINVAL;
	}

	dev_info(dev,"mipi id %d \n",csi2rx->id);

	csi2rx->base = devm_platform_ioremap_resource(csi2rx->pdev, 0);
	if (IS_ERR(csi2rx->base))
		return PTR_ERR(csi2rx->base);

	hw_info = (struct mipi_csi2_info *)device_get_match_data(dev);
	if (!hw_info) {
		dev_err(dev, "Failed to get device data\n");
	}

	irq0 = platform_get_irq(pdev, 0);
	if (irq0 < 0) {
		dev_err(dev, "Failed to get IRQ resource\n");
	}

	ret = devm_request_irq(dev, irq0, hw_info->mipi_csi2_isr,
			       IRQF_ONESHOT | IRQF_SHARED, dev_name(dev), csi2rx);
	if (ret) {
		dev_err(dev, "failed to install irq (%d)\n", ret);
	}

	irq1 = platform_get_irq(pdev, 1);
	if (!irq1) {
		dev_err(dev, "Failed to get IRQ resource\n");
	}

	ret = devm_request_irq(dev, irq1, hw_info->mipi_csi2_err_isr,
					IRQF_ONESHOT | IRQF_SHARED, dev_name(dev), csi2rx);
	if (ret) {
		dev_err(dev, "failed to install irq (%d)\n", ret);
	}

	if (has_acpi_companion(dev)) {
		ret = device_property_read_u8(dev, CIX_MIPI_CSI2_OF_NODE_NAME, &csi2rx->id);
	}
	else {
		ret = csi2rx->id = of_alias_get_id(csi2rx->pdev->dev.of_node, CIX_MIPI_CSI2_OF_NODE_NAME);
	}
	if ((ret < 0) || (csi2rx->id >= CIX_MIPI_MAX_DEVS)) {
		dev_err(dev, "invalid mipi device id (%d)\n",
			csi2rx->id);
		return -EINVAL;
	}
	
	ret = device_property_read_u32(&csi2rx->pdev->dev, "lanes", &lanes);
	if (ret < 0) {
		dev_err(dev, "%s failed to get mipi-csi lanes property\n",
				__func__);
		return ret;
	}

	dev_info(dev,"mipi get the clk & rest resource \n");
	csi2rx->sys_clk = devm_clk_get_optional(&pdev->dev, "csi_sclk");
        if (IS_ERR(csi2rx->sys_clk)) {
                dev_err(&pdev->dev, "Couldn't get sys clock\n");
                return PTR_ERR(csi2rx->sys_clk);
        }

        csi2rx->p_clk = devm_clk_get_optional(&pdev->dev, "csi_pclk");
        if (IS_ERR(csi2rx->p_clk)) {
                dev_err(&pdev->dev, "Couldn't get csi_pclk clock\n");
                return PTR_ERR(csi2rx->p_clk);
        }

	if((csi2rx->id == 0) || (csi2rx->id == 2)) {
		for(i = 0;i < 4;i++) {
			csi2rx->pixel_clk[i] = devm_clk_get_optional(&pdev->dev, mipi_csi2_clk_names[i]);
			if (IS_ERR(csi2rx->p_clk)) {
				dev_err(&pdev->dev, "Couldn't get %s clock\n",mipi_csi2_clk_names[i]);
				return PTR_ERR(csi2rx->p_clk);
			}
		}
	} else {
		csi2rx->pixel_clk[0] = devm_clk_get_optional(&pdev->dev,mipi_csi2_clk_names[0]);
		if (IS_ERR(csi2rx->p_clk)) {
			dev_err(&pdev->dev, "Couldn't get %s clock\n",mipi_csi2_clk_names[0]);
			return PTR_ERR(csi2rx->p_clk);
		}
	}

	csi2rx->csi_reset = devm_reset_control_get_optional_shared(&pdev->dev, "csi_reset");
	if (IS_ERR(csi2rx->csi_reset)) {
		if (PTR_ERR(csi2rx->csi_reset) != -EPROBE_DEFER)
			dev_err(&pdev->dev, "Failed to get csi  reset control\n");
		return PTR_ERR(csi2rx->csi_reset);
	}

	csi2rx->num_lanes = lanes;
	dev_info(dev, "mipi-csi get lanes %d\n", csi2rx->num_lanes);

	return 0;
}

static int mipi_csi2_media_init(struct csi2rx_priv *csi2rx)
{
	int ret;

	/* Create our media pads */
	csi2rx->pads[CIX_MIPI_CSI2_PAD_SINK].flags = MEDIA_PAD_FL_SINK | MEDIA_PAD_FL_MUST_CONNECT;
	csi2rx->pads[CIX_MIPI_CSI2_PAD_SOURCE].flags = MEDIA_PAD_FL_SOURCE | MEDIA_PAD_FL_MUST_CONNECT;
	csi2rx->subdev.entity.ops = &mipi_csi2_sd_media_ops;
	ret = media_entity_pads_init(&csi2rx->subdev.entity,CIX_MIPI_CSI2_PAD_MAX,
				csi2rx->pads);

	return ret;
}

static int mipi_csi2_probe(struct platform_device *pdev)
{
	struct csi2rx_priv *csi2rx = NULL;
	struct device *dev = &pdev->dev;
	struct v4l2_subdev *sd;
	int ret = -1;

	dev_info(dev, "mipi-csi2 probe enter\n");

	csi2rx = devm_kzalloc(dev, sizeof(*csi2rx), GFP_KERNEL);
	if (!csi2rx)
		return -ENOMEM;

	csi2rx->dev = dev;
	csi2rx->pdev = pdev;

	mutex_init(&csi2rx->lock);
	spin_lock_init(&csi2rx->slock);

	/*parse the dts or ACPI table*/
	mipi_csi2_parse(csi2rx);

	/*init the subdev*/
	sd = &csi2rx->subdev;
	sd->dev = &pdev->dev;
	sd->owner = THIS_MODULE;
	v4l2_subdev_init(sd, &mipi_csi2_subdev_ops);
	v4l2_set_subdevdata(sd, csi2rx);
	snprintf(sd->name, sizeof(sd->name), "%s.%d",
			CIX_MIPI_CSI2_SUBDEV_NAME, csi2rx->id);

	sd->flags |= V4L2_SUBDEV_FL_HAS_DEVNODE;

	/* Create our media pads */
	mipi_csi2_media_init(csi2rx);

	/*notifier & async subdev init*/
	v4l2_async_nf_init(&csi2rx->notifier);

	ret = v4l2_async_nf_parse_fwnode_endpoints(dev, &csi2rx->notifier,
			sizeof(struct v4l2_async_subdev),mipi_csi2_parse_endpoint);
	if (ret < 0) {
		dev_err(dev, "mipi-csi async notifier parse endpoints failed \n");
		return ret;
	}

	csi2rx->notifier.ops = &mipi_csi2_notifier_ops;
	ret = v4l2_async_subdev_nf_register(sd, &csi2rx->notifier);
	if (ret) {
		dev_err(dev, "mipi-csi async register notifier failed \n");
		v4l2_async_nf_cleanup(&csi2rx->notifier);
		return ret;
	}

	ret = v4l2_async_register_subdev(sd);
	platform_set_drvdata(pdev, csi2rx);

	pm_runtime_enable(dev);
#ifdef CONFIG_VI_BBOX
	if (rdr_register_probe == 0) {
		init_completion(&g_rdr_dump_comp);

		cix_csi_rproc_rdr_register_exception();

		ret = cix_csi_rproc_rdr_register_core();
		if (ret) {
			dev_err(dev, "cix_csi_rproc_rdr_register_core fail, ret = [%d]\n", ret);
			return ret;
		}
		rdr_register_probe = 1;
	}
#endif

	dev_info(dev, "mipi-csi2 probe exit %s \n",ret == 0 ? "success":"failed");

	return ret;
}

static int mipi_csi2_remove(struct platform_device *pdev)
{
	struct csi2rx_priv *csi2rx = platform_get_drvdata(pdev);
	struct v4l2_subdev *sd = &csi2rx->subdev;
	struct device *dev = &pdev->dev;

#ifdef CONFIG_VI_BBOX
	if (rdr_register_probe == 1) {
		cix_csi_rproc_rdr_unregister_core();
		cix_csi_rproc_rdr_unregister_exception();
		rdr_register_probe = 0;
	}
#endif
	media_entity_cleanup(&csi2rx->subdev.entity);
	v4l2_async_nf_unregister(&csi2rx->notifier);
	v4l2_async_nf_cleanup(&csi2rx->notifier);
	v4l2_async_unregister_subdev(sd);

	pm_runtime_disable(dev);

	return 0;
}

struct mipi_csi2_info mipi_csi2_hw_info = {
	.mipi_csi2_isr     = mipi_csi2_irq_handler,
	.mipi_csi2_err_isr = mipi_csi2_err_irq_handler,
};

static const struct of_device_id mipi_csi2_of_match[] = {
	{ .compatible = "cix,cix-mipi-csi2", .data = &mipi_csi2_hw_info },
	{ /* sentinel */ },
};
MODULE_DEVICE_TABLE(of, mipi_csi2_of_match);

static const struct acpi_device_id mipi_csi2_acpi_match[] = {
	{ .id = "CIXH3029", .driver_data = (long unsigned int)&mipi_csi2_hw_info },
	{ /* sentinel */ },
};
MODULE_DEVICE_TABLE(acpi, mipi_csi2_acpi_match);

static struct platform_driver mipi_csi2_driver = {
	.driver = {
		.name = CIX_MIPI_CSI2_DRIVER_NAME,
		.of_match_table = mipi_csi2_of_match,
		.acpi_match_table = ACPI_PTR(mipi_csi2_acpi_match),
		.pm = &mipi_csi2_dev_pm_ops,
	},
	.probe = mipi_csi2_probe,
	.remove = mipi_csi2_remove,
};

module_platform_driver(mipi_csi2_driver);

MODULE_AUTHOR("Cix Semiconductor, Inc.");
MODULE_DESCRIPTION("Cix MIPI CSI2 driver");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:" CIX_MIPI_CSI2_DRIVER_NAME);
