/*
*
* Copyright 2024 Cix Technology Group Co., Ltd.
*
*/

#ifndef __MXC_COMMON_H__
#define __MXC_COMMON_H__

#include <linux/of.h>
#include <linux/i2c.h>
#include <linux/bug.h>
#include <linux/clk.h>
#include <linux/reset.h>
#include <linux/list.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/types.h>
#include <linux/errno.h>
#include <linux/device.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/of_graph.h>
#include <linux/of_device.h>
#include <linux/videodev2.h>
#include <linux/interrupt.h>
#include <linux/pm_runtime.h>
#include <linux/of_platform.h>
#include <linux/clk-provider.h>
#include <linux/platform_device.h>

#include <media/v4l2-async.h>
#include <media/v4l2-ctrls.h>
#include <media/v4l2-event.h>
#include <media/v4l2-ioctl.h>
#include <media/v4l2-device.h>
#include <media/v4l2-fwnode.h>
#include <media/v4l2-mem2mem.h>
#include <media/media-device.h>
#include <media/videobuf2-core.h>
#include <media/videobuf2-dma-contig.h>

/* For dump register */
#define CSI_DMA_DEBUG

/* CIX Media Device & Subdev's Name */
#define CIX_MD_DRIVER_NAME		"cix-md"
#define CIX_BRIDGE_OF_NODE_NAME		"cix-bridge"
#define CIX_MIPI_CSI2_OF_NODE_NAME	"cix-csi"
#define CIX_MIPI_DPHY_OF_NODE_NAME	"cix-dphy"
#define CIX_MIPI_DPHY_HW_OF_NODE_NAME	"cix-dphy-hw"

#define CIX_BRIDGE_NODE_NAME		"cix_bridge"
#define CIX_MIPI_CSI2_NODE_NAME		"csi"
#define CIX_MIPI_DPHY_NODE_NAME		"phy"

/* Driver Name base on devicetree */
#define CSI_DMA_DRIVER_NAME		"cix-bridge"
#define CSI_DMA_CAPTURE_NAME		"cix-cap"

#define CIX_BRIDGE_MAX_DEVS	4
#define CIX_SENSORS_MAX_DEVS	2
#define CIX_MIPI_CSI2_MAX_DEVS	4
#define CIX_MIPI_DPHY_MAX_DEVS	2
#define CIX_MIPI_DPHY_RX_MAX_DEVS	6

#define CIX_NAME_LENS 32

/* The subdevices' group IDs */
#define GRP_ID_CIX_SENSOR	BIT(1)
#define GRP_ID_CIX_BRIDGE	BIT(2)
#define GRP_ID_CIX_MIPI_DPHY	BIT(3)
#define GRP_ID_CIX_MIPI_CSI2	BIT(4)

/* CIX Bridge PADS */
#define CIX_BRIDGE_SD_PAD_SINK_CSI0	0
#define CIX_BRIDGE_SD_PAD_SINK_CSI1	1
#define CIX_BRIDGE_SD_PAD_SOURCE_MEM	2
#define CIX_BRIDGE_SD_PADS_NUM		3

/* CIX MIPI CSI PADS */
#define CIX_MIPI_CSI2_PAD_SINK_DPHY	0
#define CIX_MIPI_CSI2_PAD_SOURCE_BRIDGE	1
#define CIX_MIPI_CSI2_PADS_NUM		2

/* CIX MIPI DPHY PADS */
#define CIX_MIPI_DPHY_PAD_SINK_SENSOR0	0
#define CIX_MIPI_DPHY_PAD_SINK_SENSOR1	1
#define CIX_MIPI_DPHY_PAD_SOURCE_CSI0	2
#define CIX_MIPI_DPHY_PAD_SOURCE_CSI1	3
#define CIX_MIPI_DPHY_PADS_NUM		4

/* CIX MIPI CSI ID */
#define CIX_CSI0_ID		0
#define CIX_CSI1_ID		1
#define CIX_CSI2_ID		2
#define CIX_CSI3_ID		3

#define CIX_DPHY_COEFF	2

#define CIX_PIPE_LINE_SUBDEV_NUM	(3)

/* Define FOURCC*/
#define V4L2_PIX_FMT_YUYV10 v4l2_fourcc('Y', 'U', 'V', '1')

/* CIX CSI-DMA Support Resolution */
/* Maxmum support 4K */
#define CSI_DMA_2K		2048U
#define CSI_DMA_4K		4096U

/* CIX CSI-DMA Support Maxmum 2 planes color format */
#define CSI_DMA_MAX_PLANES	2

/* Alignment */
#define CSI_DMA_ALIGN	16

#define STATIC_CFG_REG	(0x08)
#define STATIC_CFG_ENABLE_LRTE	(1<<2)
/* MIPI-CSI2 IRQ */
#define INFO_IRQS	0x40
#define ERROR_IRQS	0x48
#define DPHY_ERR_STATUS_IRQ		0x58

/* MIPI-CSI2 IRQ MASK */
#define INFO_IRQS_MASK	0x44
#define ERROR_IRQS_MASK	0x4C
#define DPHY_ERR_IRQ_MASK_CFG	0x5C
#define DPHY_LANE_CONTROL		0x50

/* MIPI-CSI2 INFO_IRQS */
#define MIPI_SP_RCVD_IRQ		(1<<0)
#define MIPI_LP_RCVD_IRQ		(1<<1)
#define MIPI_SLEEP_IRQ			(1<<2)
#define MIPI_WAKEUP_IRQ			(1<<3)
#define MIPI_DESKEW_ENTRY_IRQ		(1<<5)
#define MIPI_SP_GENERIC_RCVD_IRQ	(1<<6)
#define MIPI_EPD_OPTION1_DETECT_IRQ	(1<<7)
#define MIPI_STREAM0_STOP_IRQ		(1<<8)
#define MIPI_STREAM0_ABORT_IRQ		(1<<9)
#define MIPI_STREAM1_STOP_IRQ		(1<<10)
#define MIPI_STREAM1_ABORT_IRQ		(1<<11)
#define MIPI_STREAM2_STOP_IRQ		(1<<12)
#define MIPI_STREAM2_ABORT_IRQ		(1<<13)
#define MIPI_STREAM3_STOP_IRQ		(1<<14)
#define MIPI_STREAM3_ABORT_IRQ		(1<<15)
#define MIPI_STREAM4_STOP_IRQ		(1<<16)
#define MIPI_STREAM4_ABORT_IRQ		(1<<17)
#define MIPI_STREAM5_STOP_IRQ		(1<<18)
#define MIPI_STREAM5_ABORT_IRQ		(1<<19)
#define MIPI_STREAM6_STOP_IRQ		(1<<20)
#define MIPI_STREAM6_ABORT_IRQ		(1<<21)
#define MIPI_STREAM7_STOP_IRQ		(1<<22)
#define MIPI_STREAM7_ABORT_IRQ		(1<<23)

/* MIPI-CSI2 ERROR_IRQS */
#define FRONT_FIFO_OVERFLOW_IRQ		(1<<0)
#define PAYLOAD_CRC_IRQ				(1<<4)
#define HEADER_ECC_IRQ				(1<<5)
#define HEADER_CORRECTED_ECC_IRQ	(1<<6)
#define DATA_ID_IRQ					(1<<7)
#define PROT_TRUNCATED_PACKET_IRQ	(1<<11)
#define PROT_FRAME_MISMATCH_IRQ		(1<<12)
#define PROT_LINE_MISMATCH_IRQ		(1<<13)
#define STREAM0_FIFO_OVERFLOW_IRQ	(1<<16)
#define STREAM1_FIFO_OVERFLOW_IRQ	(1<<17)
#define STREAM2_FIFO_OVERFLOW_IRQ	(1<<18)
#define STREAM3_FIFO_OVERFLOW_IRQ	(1<<19)
#define STREAM4_FIFO_OVERFLOW_IRQ	(1<<20)
#define STREAM5_FIFO_OVERFLOW_IRQ	(1<<21)
#define STREAM6_FIFO_OVERFLOW_IRQ	(1<<22)
#define STREAM7_FIFO_OVERFLOW_IRQ	(1<<23)

/*DPHY_ERROR_STATUS_IRQ*/
/* MIPI-CSI2 ERROR_IRQS */
#define DL0_ERRSOTHS_IRQ			(1<<0)
#define DL0_ERRSOTSYNCHS_IRQ		(1<<1)
#define DL1_ERRSOTHS_IRQ			(1<<4)
#define DL1_ERRSOTSYNCHS_IRQ		(1<<5)
#define DL2_ERRSOTHS_IRQ			(1<<8)
#define DL2_ERRSOTSYNCHS_IRQ		(1<<9)
#define DL3_ERRSOTHS_IRQ			(1<<12)
#define DL3_ERRSOTSYNCHS_IRQ		(1<<13)
#define DL4_ERRSOTHS_IRQ			(1<<16)
#define DL4_ERRSOTSYNCHS_IRQ		(1<<17)
#define DL5_ERRSOTHS_IRQ			(1<<20)
#define DL5_ERRSOTSYNCHS_IRQ		(1<<21)
#define DL6_ERRSOTHS_IRQ			(1<<24)
#define DL6_ERRSOTSYNCHS_IRQ		(1<<25)
#define DL7_ERRSOTHS_IRQ			(1<<28)
#define DL7_ERRSOTSYNCHS_IRQ		(1<<29)

#define ENABLE                                      1
#define DISABLE                                     0
#define DPHY_LANES_MIN                              1
#define DPHY_LANES_MAX                              4
#define DPHY_DATA_LANE_NUM_LEFT                     2
#define DPHY_DATA_LANE_NUM_RIGHT                    2
/*MIPI DPHY register*/
/* PCS Test */
#define PMA_CMN_REG                                 0x000
#define DPHY_PMA_CMN(reg)                           (PMA_CMN_REG + (reg))

#define DPHY_CMN_DIG_TBIT2                          DPHY_PMA_CMN(0x20)
#define CMN_DIG_TBIT56                              0xF0

#define CMN_SSM_EN_OFFSET                           0
#define CMN_SSM_EN_MASK                             0x1

#define CMN_RX_BANDGAP_TIMER_OFFSET                 1
#define DPHY_CMN_RX_BANDGAP_TIMER_MASK              0x1FE
#define DPHY_CMN_RX_BANDGAP_TIMER                   0x14

#define CMN_RX_MODE_EN_OFFSET                       10
#define CMN_RX_MODE_EN_MASK                         0x400
#define PCS_REG                                     0xB00
#define DPHY_PCS(reg)                               (PCS_REG + (reg))

#define DPHY_PCS_BAND_CFG                           DPHY_PCS(0x0)

#define DPHY_POWER_ISLAND_EN_DATA                   DPHY_PCS(0x8)
#define DPHY_POWER_ISLAND_EN_DATA_VAL               0xaaaaaaaa

#define DPHY_POWER_ISLAND_EN_CLK                    DPHY_PCS(0xc)
#define DPHY_POWER_ISLAND_EN_CLK_VAL                0x2aa

/* PHY Isolation */
#define ISO_REG                                     0xC00
#define DPHY_ISO(reg)                               (ISO_REG + (reg))

#define DPHY_ISO_CL_CTRL_L                          DPHY_ISO(0x10)
#define DPHY_ISO_DL_CTRL_L0                         DPHY_ISO(0x14)
#define DPHY_ISO_DL_CTRL_L1                         DPHY_ISO(0x20)
#define DPHY_ISO_DL_CTRL_R0                         DPHY_ISO(0x54)
#define DPHY_ISO_DL_CTRL_R1                         DPHY_ISO(0x60)
#define DPHY_ISO_LANE_READY_BIT                     0
#define DPHY_ISO_LANE_READY_TIMEOUT_US              (100*1000)
#define DPHY_ISO_LANE_POLL_DELAY_US                 10

#define DPHY_BAND_CFG_LEFT_LANE_OFFSET              0
#define DPHY_BAND_CFG_LEFT_LANE_MASK                0x1F
#define DPHY_BAND_CFG_RIGHT_LANE_OFFSET             5
#define DPHY_BAND_CFG_RIGHT_LANE_MASK               0x3E0

#define CLK0_RX_ANA_TBIT0                           0x100
#define CLK0_RX_DIG_TBIT2                           0x114
#define RXDA_FREQ_BAND_STG2_OFFSET                  15
#define RXDA_FREQ_BAND_STG2_MASK                    0x78000
#define RXDA_FREQ_BAND_STG3_OFFSET                  10
#define RXDA_FREQ_BAND_STG3_MASK                    0x3c00
#define CLK1_RX_ANA_TBIT0                           0x600

#define DL0_LEFT_RX_ANA_TBIT0                       0x200
#define DL0_LEFT_RX_DIG_TBIT0                       0x208
#define TM_1P5TO2P5G_MODE_EN_OFFSET                 21
#define TM_1P5TO2P5G_MODE_EN_MASK                   0x200000
#define TM_SETTLE_COUNT_OFFSET                      9
#define TM_SETTLE_COUNT_MASK                        0x1FE00

#define DL0_LEFT_RX_DIG_TBIT3                       0x214
#define TM_PREPAMP_CAL_INIT_WAIT_TIME_OFFSET        0
#define TM_PREPAMP_CAL_INIT_WAIT_TIME_MASK          0xFF
#define DL0_LEFT_RX_DIG_TBIT5                       0x21c
#define TM_DCC_COMP_CAL_ITER_WAIT_TIME_OFFSET       0
#define TM_DCC_COMP_CAL_ITER_WAIT_TIME_MASK         0xFF
#define DL0_LEFT_RX_DIG_TBIT7                       0x224
#define TM_MIXER_COMP_CAL_INIT_WAIT_TIME_OFFSET     0
#define TM_MIXER_COMP_CAL_INIT_WAIT_TIME_MASK       0xFF
#define DL0_LEFT_RX_DIG_TBIT9                       0x22c
#define TM_POS_SAMP_CAL_INIT_WAIT_TIME_OFFSET       0
#define TM_POS_SAMP_CAL_INIT_WAIT_TIME_MASK         0xFF
#define DL0_LEFT_RX_DIG_TBIT12                      0x238
#define TM_NEG_SAMP_CAL_INIT_WAIT_TIME_OFFSET       0
#define TM_NEG_SAMP_CAL_INIT_WAIT_TIME_MASK         0xFF

#define DL1_LEFT_RX_ANA_TBIT0                       0x300
#define DL2_LEFT_RX_ANA_TBIT0                       0x400
#define DL3_LEFT_RX_ANA_TBIT0                       0x500
#define DL0_RIGHT_RX_ANA_TBIT0                      0x700
#define DL0_RIGHT_RX_DIG_TBIT0                      0x708
#define DL0_RIGHT_RX_DIG_TBIT3                      0x714
#define DL0_RIGHT_RX_DIG_TBIT5                      0x71c
#define DL0_RIGHT_RX_DIG_TBIT7                      0x724
#define DL0_RIGHT_RX_DIG_TBIT9                      0x72c
#define DL0_RIGHT_RX_DIG_TBIT12                     0x738

/* dphy lane control */
#define DL0_ENABLE			(1<<0)
#define DL1_ENABLE			(1<<1)
#define DL2_ENABLE			(1<<2)
#define DL3_ENABLE			(1<<3)
#define CL_ENABLE			(1<<8)
#define DPHY_RESET			(1<<16)
/* Stream0 Ctrl*/
#define STEAM_CTRL_START		(1<<0)
#define STEAM0_CTRL_STOP		(1<<1)

/* Stream0 Data Cfg */
#define VC_SELECT(x)			(1<<(16 + x))

/* Stream0 Cfg */
#define FIFO_MODE_OFFSET			8
#define INTERFACE_MODE_OFFSET		0

#define FULL_LINE_BUFFER			0
#define LARGE_BUFFER				1

#define PIXEL_MODE		0
#define PACKED_MODE		1

#define STREAM_CTRL(x)		(0x100 + x*0x100 + 0x00)
#define STREAM_STATUS(x)	(0x100 + x*0x100 + 0x04)
#define STREAM_DATA_CFG(x)	(0x100 + x*0x100 + 0x08)
#define STREAM_CFG(x)		(0x100 + x*0x100 + 0x0C)

/* Core Ctrl */
#define CORE_CTRL		0x4
#define CORE_CTRL_START		(1<<0)

struct csi_dma_dev;

enum dphy_rx_pads {
	DPHY_RX_PAD_SINK0,
	DPHY_RX_PAD_SINK1,
	DPHY_RX_PAD_SOURCE0,
	DPHY_RX_PAD_SOURCE1,
	DPHY_RX_PAD_MAX,
};

struct dphy_rx {
	struct v4l2_subdev subdev;
	struct device *dev;
	struct platform_device *pdev;
	struct cdns_dphy_rx *dphy_hw;
	u8			id;
	struct v4l2_async_notifier	notifier;
	struct media_pad	pads[DPHY_RX_PAD_MAX];
	/* Remote source */
	struct v4l2_async_subdev asd;
	struct v4l2_subdev	*source_subdev;
	int			source_pad;
	struct v4l2_mbus_framefmt format;

	struct v4l2_ctrl_handler hdl;
	struct v4l2_ctrl *pixel_rate;
	struct v4l2_subdev sd;
	unsigned 		lane_num;

	u64 data_rate;
	u16 data_rate_mbps;
};

struct dphy_hw_drv_data {
	int (*stream_on)(struct dphy_rx *dphy, unsigned int id ,unsigned int lane_rate);
	int (*stream_off)(struct dphy_rx *dphy, unsigned int id );
	int (*dphy_hw_resume)(struct dphy_rx *dphy);
	int (*dphy_hw_suspend)(struct dphy_rx *dphy);
};

/* Rates are in Mbps. */
struct cdns_dphy_rx_band {
    unsigned int min_rate;
    unsigned int max_rate;
};

struct dph_psm_dynamic_param
{
    u32 rx_stage2_cutoff_freq;
    u32 rx_stage3_cutoff_freq;
    u32 data_rate_select;
    u32 hs_settle_counter_value;
};

static const u32 data_lane_ctrl[DPHY_LANES_MAX] =
{
    DPHY_ISO_DL_CTRL_L0,
    DPHY_ISO_DL_CTRL_L1,
    DPHY_ISO_DL_CTRL_R0,
    DPHY_ISO_DL_CTRL_R1
};

enum csi_dma_out_fmt {
	CSI_DMA_OUT_FMT_RGB888,
	CSI_DMA_OUT_FMT_RGB888X,
	CSI_DMA_OUT_FMT_RGB565	= 2,
	CSI_DMA_OUT_FMT_RAW16	= 2,
	CSI_DMA_OUT_FMT_NV12,
	CSI_DMA_OUT_FMT_P010,
	CSI_DMA_OUT_FMT_YUYV,
	CSI_DMA_OUT_FMT_YUYV10,
};

enum csi_dma_buf_id {
	CSI_DMA_BUF1,
	CSI_DMA_BUF2,
};

enum {
	IN_PORT,
	OUT_PORT,
	MAX_PORTS,
};

typedef enum
{
    MIPI_1LANE_EN   = 0x1,
    MIPI_2LANES_EN  = 0x3,
    MIPI_4LANES_EN  = 0xf,
} lane_mask_type_t;

struct csi_dma_fmt {
	char *name;
	u32	mbus_code;
	u32	fourcc;
	u32	color;
	u16	memplanes;
	u16	colplanes;
	u8	colorspace;
	u8	depth[CSI_DMA_MAX_PLANES];
	u16	mdataplanes;
	u16	flags;
};

struct csi_dma_ctrls {
	struct v4l2_ctrl_handler handler;
	struct v4l2_ctrl *hflip;
	struct v4l2_ctrl *vflip;
	struct v4l2_ctrl *alpha;
	struct v4l2_ctrl *num_cap_buf;
	struct v4l2_ctrl *num_out_buf;
	bool ready;
};

/**
 * struct addr -  physical address set for DMA
 * @y:	 luminance plane physical address
 * @cb:	 Cb plane physical address
 */
struct frame_addr {
	u64	y;
	u64	cb;
};

/**
 * struct csi_dma_frame - source/target frame properties
 * width:	out image pixel width
 * height:	out image pixel weight
 * bytesperline: bytesperline value for each plane
 * paddr:	image frame buffer physical addresses
 * fmt:	color format pointer
 */
struct csi_dma_frame {
	u32	width;
	u32	height;
	unsigned int sizeimage[CSI_DMA_MAX_PLANES];
	unsigned int bytesperline[CSI_DMA_MAX_PLANES];
	struct csi_dma_fmt *fmt;
};

struct csi_dma_buffer {
	struct vb2_v4l2_buffer  v4l2_buf;
	struct list_head list;
	struct frame_addr paddr;
	enum csi_dma_buf_id	id;
	bool discard;
};

struct csi_dma_ctx {
	struct v4l2_fh fh;
};

struct csi_dma_chan_src {
	u32 src_csi0;
	u32 src_csi2;
};

struct csi_dma_reg {
	u32 offset;
	u32 mask;
};

struct csi_dma_dev_ops {
	int (*clk_get)(struct csi_dma_dev *csi_dma);
	int (*clk_enable)(struct csi_dma_dev *csi_dma);
	void (*clk_disable)(struct csi_dma_dev *csi_dma);
};

struct csi_dma_panic_thd {
	u32 mask;
	u32 offset;
	u32 threshold;
};

struct csi_dma_set_thd {
	struct csi_dma_panic_thd panic_set_thd_y;
	struct csi_dma_panic_thd panic_set_thd_u;
	struct csi_dma_panic_thd panic_set_thd_v;
};

struct csi_dma_rst_ops {
	int (*parse)(struct csi_dma_dev *csi_dma);
	int (*assert)(struct csi_dma_dev *csi_dma);
	int (*deassert)(struct csi_dma_dev *csi_dma);
};

struct csi_dma_gate_clk_ops {
	int (*gclk_get)(struct csi_dma_dev *csi_dma);
	int (*gclk_enable)(struct csi_dma_dev *csi_dma);
	int (*gclk_disable)(struct csi_dma_dev *csi_dma);
};

struct csi_dma_plat_data {
	struct csi_dma_dev_ops *ops;
	struct csi_dma_chan_src *chan_src;
	struct csi_dma_set_thd *set_thd;
	struct csi_dma_rst_ops *rst_ops;
	struct csi_dma_gate_clk_ops *gclk_ops;
};

struct csi_dma_cap_dev {
	struct video_device vdev;
	struct v4l2_fh fh;
	struct vb2_queue vb2_q;
	struct v4l2_pix_format_mplane pix;
	struct csi_dma_dev *csi_dma;
	struct device *dev;
	struct platform_device *pdev;
	struct csi_dma_ctrls   ctrls;
	struct csi_dma_buffer  buf_discard[2];
	struct media_pad cap_pad;
	int source_pad;
	struct v4l2_subdev *source_subdev;
	struct v4l2_subdev *sensor_subdev;
	struct list_head out_pending;
	struct list_head out_active;
	struct list_head out_discard;
	struct csi_dma_frame src_f;
	u32 frame_count;
	u32 id;
	u32 is_streaming;
	pid_t streaming_pid;
	bool is_link_setup;
	struct mutex lock;
	struct mutex qlock;
	struct mutex vlock;
	spinlock_t   slock;
	/* dirty buffer */
	size_t     discard_size[CSI_DMA_MAX_PLANES];
	void       *discard_buffer[CSI_DMA_MAX_PLANES];
	dma_addr_t discard_buffer_dma[CSI_DMA_MAX_PLANES];
};

struct csi_dma_pipeline {
	struct media_pipeline pipe;
	int num_subdevs;
	struct v4l2_subdev *subdevs[CIX_PIPE_LINE_SUBDEV_NUM];
	int (*open)(struct csi_dma_pipeline *p,
			struct v4l2_subdev ** sd, bool prepare);
	int (*close)(struct csi_dma_pipeline *p);
	int (*set_stream)(struct csi_dma_pipeline *p, bool on);
};

struct csi_dma_dev {
	struct csi_dma_cap_dev *dma_cap;
	struct device *dev;
	struct platform_device *pdev;
	struct v4l2_subdev *sensor_sd;
	struct csi_rcsu_dev *rcsu_dev;
	const struct csi_dma_plat_data *pdata;
	struct v4l2_device v4l2_dev;
	struct media_device media_dev;
	struct v4l2_async_notifier notifier;
	struct csi_dma_pipeline pipe;
	struct clk *sclk;
	struct clk *apbclk;
	void __iomem *regs;
	struct reset_control *csibridge_reset;
	struct mutex lock;
	spinlock_t slock;
	u8 chain_buf;
	u32 status;
	u32 axi_uid;
	u32 stream_on;
	int id;
	int interface[MAX_PORTS];
	unsigned int is_streaming;
	bool cap_enabled;
	bool buf_active_reverse;
	atomic_t usage_count;

	u16	sys_clk_freq;
};

struct err_status{
	u32 csi_err_status;
	u32 dphy_err_status;
};

static inline void set_frame_bounds(struct csi_dma_frame *f,
				    u32 width, u32 height)
{
	f->width  = width;
	f->height = height;
}

#endif /* CSI_DMA_CORE_H_ */
