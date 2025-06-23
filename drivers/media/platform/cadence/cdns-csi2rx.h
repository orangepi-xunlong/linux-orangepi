#ifndef __CDNS_CSI2RX_H__
#define __CDNS_CSI2RX_H__


#define CSI2RX_LANES_MAX	4
#define CSI2RX_STREAMS_MAX	4

enum csi2rx_pads {
	CSI2RX_PAD_SINK,
	CSI2RX_PAD_SOURCE_STREAM0,
	CSI2RX_PAD_SOURCE_STREAM1,
	CSI2RX_PAD_SOURCE_STREAM2,
	CSI2RX_PAD_SOURCE_STREAM3,
	CSI2RX_PAD_MAX,
};

struct csi2rx_priv {
	struct v4l2_subdev	subdev;
	struct device		*dev;
	unsigned int		count;

	struct mutex		lock;
	spinlock_t			slock;

	void __iomem		*base;
	struct clk			*sys_clk;
	struct clk			*p_clk;
	struct clk			*pixel_clk[CSI2RX_STREAMS_MAX];
	struct phy			*dphy;

	struct	reset_control   	*csi_reset;

	u8					lanes[CSI2RX_LANES_MAX];
	u8					num_lanes;
	u8					max_lanes;
	u8					max_streams;
	bool				has_internal_dphy;

	struct v4l2_async_notifier	notifier;
	struct media_pad			pads[CSI2RX_PAD_MAX];

	/* Remote source */
	struct v4l2_async_subdev	asd;
	struct v4l2_subdev			*source_subdev;
	struct platform_device		*pdev;

	struct v4l2_mbus_framefmt	format;
	u8	id;
	u32	status;
	int	source_pad;
	u16	sys_clk_freq;
	u64	data_rate_Mbit;
	u32	stream_on;
};
extern int csi2rx_get_resources(struct csi2rx_priv *csi2rx,
				struct platform_device *pdev);
extern int csi2rx_parse_dt(struct csi2rx_priv *csi2rx);
extern int csi2rx_start(struct csi2rx_priv *csi2rx);
extern void csi2rx_stop(struct csi2rx_priv *csi2rx);
extern struct csi2rx_priv *v4l2_subdev_to_csi2rx(struct v4l2_subdev *subdev);

#endif
