#ifndef _HDMITX_DEV_H_
#define _HDMITX_DEV_H_

#include "util/util.h"
#include <linux/string.h>
#include <linux/vmalloc.h>
#include <linux/delay.h>
#include "core/hdmi_includes.h"

#define HDMI_LICENSING_LLC_OUI 0x000C03

typedef enum {
	MODE_UNDEFINED = -1,
	DVI = 0,
	HDMI
} video_mode_t;

typedef enum {
	PHY_ACCESS_UNDEFINED = 0,
	PHY_I2C = 1,
	PHY_JTAG,
	PHY_EXTERN
} phy_access_t;

struct hdmi_tx_ctrl {
	int status;
	bool hpd;
	bool rx_sense;
	bool phy_enable;
	bool phy_power;
	bool src_scramble;
	bool sink_scramble;
	bool high_tmds_ratio;
	bool avmute;
	bool audio_mute;
	bool csc_on;
	bool audio_on;
	bool cec_on;
	bool hdcp_on;
	bool hdmi_on;
	u8 hdmi_force_output;
	u8 data_enable_polarity;
	u8 channel_cnt;
	u8 channel_alloc;
	u8 pixel_repetition;
	u8 color_resolution;
	u8 encoding;
	u8 avif_raw[20];
	u8 vsif_raw[20];
	u8 audif_raw[20];
	u16 hpd_count;
	u16 edid_fail;
	u16 Rsen_count;
	u16 Rsen_discount;
	u16 Hdcp_count;
	u32 n;
	u32 cts;
	u32 pixel_clock;
};

struct hdmi_tx_phy {
	int version;
	int generation;
	int status;
};

typedef struct {
	/** Bypass encryption */
	int bypass;

	/** Enable Feature 1.1 */
	int menable11_feature;

	/** Check Ri every 128th frame */
	int mricheck;

	/** I2C fast mode */
	int mi2c_fastmode;

	/** Enhanced link verification */
	int menhanced_link_verification;

	/** Number of supported devices
	 * (depending on instantiated KSV MEM RAM – Revocation Memory to support
	 * HDCP repeaters)
	 */
	u8 max_devices;

	/** KSV List buffer
	 * Shall be dimensioned to accommodate 5[bytes] x No. of supported devices
	 * (depending on instantiated KSV MEM RAM – Revocation Memory to support
	 * HDCP repeaters)
	 * plus 8 bytes (64-bit) M0 secret value
	 * plus 2 bytes Bstatus
	 * Plus 20 bytes to calculate the SHA-1 (VH0-VH4)
	 * Total is (30[bytes] + 5[bytes] x Number of supported devices)
	 */
	u8 *mksvList_buffer;

	/** aksv total of 14 chars**/
	u8 *maksv;

	/** Keys list
	 * 40 Keys of 14 characters each
	 * stored in segments of 8 bits (2 chars)
	 * **/
	u8 *mkeys;

	u8 *msw_enckey;
} hdcp_params_t;

typedef struct {
    u32 scl_high_ns;
    u32 scl_low_ns;
} i2c_param_t;

typedef struct hdmi_tx_dev{
	char device_name[20];

#ifdef KERNEL
	/** Device node */
	struct device *dev;
#endif

	/** Verbose */
	int	verbose;

	/** SYNOPSYS DATA */
	struct hdmi_tx_ctrl	snps_hdmi_ctrl;

	hdcp_params_t hdcp;

	struct hdmitx_dev* dev_hdmi;

	i2c_param_t i2c;

	bool is_init;
	bool is_deinit;
} hdmi_tx_dev_t;

#endif /* _HDMITX_DEV_H_ */