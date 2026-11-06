#ifndef DW_HDMI_HDCP_H
#define DW_HDMI_HDCP_H

#include <linux/miscdevice.h>

#define DW_HDCP_DRIVER_NAME "dw-hdmi-hdcp"
#define HDCP_PRIVATE_KEY_SIZE   280


struct hdcp_keys {
	u8 KSV[8];
	u8 devicekey[HDCP_PRIVATE_KEY_SIZE];
	u8 seeds[2];
};

struct dw_hdcp {
	bool enable;
	int retry_times;
	int remaining_times;
	int hdcp2_enable;
	int status;
	u32 reg_io_width;

	struct miscdevice mdev;
	struct hdcp_keys *keys;
	struct device *dev;
	struct dw_hdmi *hdmi;
	void __iomem *regs;

	void (*write)(struct dw_hdmi *hdmi, u8 val, int offset);
	u8 (*read)(struct dw_hdmi *hdmi, int offset);
	int (*hdcp_start)(struct dw_hdcp *hdcp);
	int (*hdcp_stop)(struct dw_hdcp *hdcp);
	void (*hdcp_isr)(struct dw_hdcp *hdcp, int hdcp_int);
};

#endif
