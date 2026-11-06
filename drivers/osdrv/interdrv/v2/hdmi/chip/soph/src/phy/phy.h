#ifndef _PHY_H_
#define _PHY_H_

#include "core/hdmitx_dev.h"
#include "core/video.h"
#include "util/util.h"

#define PHY_TIMEOUT          100
#define PHY_I2C_SLAVE_ADDR   0x69

struct phy_config{
	u32 pixel_clk;
	pixel_repetition_t pixel;
	color_depth_t color;
	operation_mode_t opmode;
	u16 oppllcfg;
	u16 pllcurrctrl;
	u16 pllgmpctrl;
	u16 txterm;
	u16 vlevctrl;
	u16 cksymtxctrl;
};

int phy_initialize(hdmi_tx_dev_t *dev);

char * phy_identification(hdmi_tx_dev_t *dev);

int phy_powerup(hdmi_tx_dev_t *dev);

int phy_preparation(hdmi_tx_dev_t *dev);

int phy_configure(hdmi_tx_dev_t *dev);

int phy_standby(hdmi_tx_dev_t *dev);

void phy_enable_hpd_sense(hdmi_tx_dev_t *dev, u8 bit);

int phy_disable_hpd_sense(hdmi_tx_dev_t *dev);

int phy_hot_plug_detected(hdmi_tx_dev_t *dev);

int phy_rx_s0_detected(hdmi_tx_dev_t *dev);

int phy_rx_s1_detected(hdmi_tx_dev_t *dev);

int phy_rx_s2_detected(hdmi_tx_dev_t *dev);

int phy_rx_s3_detected(hdmi_tx_dev_t *dev);

int phy_interrupt_enable(hdmi_tx_dev_t *dev, u8 value);

int phy_hpd_sense(hdmi_tx_dev_t *dev, int enable);

int phy_phase_lock_loop_state(hdmi_tx_dev_t *dev);

void phy_interrupt_mask(hdmi_tx_dev_t *dev, u8 mask);

void phy_interrupt_unmask(hdmi_tx_dev_t *dev, u8 mask);

u8 phy_rx_s0_state(hdmi_tx_dev_t *dev);

u8 phy_rx_s1_state(hdmi_tx_dev_t *dev);

u8 phy_rx_s2_state(hdmi_tx_dev_t *dev);

u8 phy_rx_s3_state(hdmi_tx_dev_t *dev);

u8 phy_rx_sense_state(hdmi_tx_dev_t *dev);

u8 phy_hot_plug_state(hdmi_tx_dev_t *dev);

int phy_write(hdmi_tx_dev_t *dev, u16 addr, u32 data);

int phy_read(hdmi_tx_dev_t *dev, u16 addr, u32 * value);

int phy_slave_address(hdmi_tx_dev_t *dev, u8 value);

int phy_set_interface(hdmi_tx_dev_t *dev, phy_access_t interface);

int phy_reconfigure_interface(hdmi_tx_dev_t *dev);

phy_access_t phy_get_interface(hdmi_tx_dev_t *dev);

u32 phy_get_freq(u32 pclk);

int phy316_configure_supported(hdmi_tx_dev_t *dev, u32 pclk, color_depth_t color, pixel_repetition_t pixel);

int phy316_configure(hdmi_tx_dev_t *dev, u32 pclk, color_depth_t color, pixel_repetition_t pixel);

#endif	/* _PHY_H_ */
