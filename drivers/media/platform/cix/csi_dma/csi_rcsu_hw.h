#ifndef CSI_RCSU_H
#define CSI_RCSU_H

#define PHY_PSM_CLOCK_FREQ_OFFSET                   0
#define PHY_PSM_CLOCK_FREQ_MASK                     0xFF
struct csi_rcsu_dev {
	struct device *dev;
	struct platform_device *pdev;
	void __iomem *base;
	u8 id;
	struct mutex mutex;
	void (*chan_mux_sel)(struct csi_rcsu_dev *csi_rcsu_dev, unsigned int index ,unsigned int chan);
	void (*dphy_psm_config)(struct csi_rcsu_dev *csi_rcsu_dev);
};

#endif
