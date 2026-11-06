#ifndef _PHY_I2C_H_
#define _PHY_I2C_H_

#include "core/hdmitx_dev.h"
#include "util/util.h"

void phy_i2c_fast_mode(hdmi_tx_dev_t *dev, u8 bit);

void phy_i2c_master_reset(hdmi_tx_dev_t *dev);

void phy_i2c_mask_interrupts(hdmi_tx_dev_t *dev, int mask);

void phy_i2c_slave_address(hdmi_tx_dev_t *dev, u8 value);

int phy_i2c_write(hdmi_tx_dev_t *dev, u8 addr, u16 data);

int phy_i2c_read(hdmi_tx_dev_t *dev, u8 addr, u16 * value);

#endif	/* _PHY_I2C_H_ */
