#ifndef _IDENTIFICATION_H_
#define _IDENTIFICATION_H_

#include "core/hdmitx_dev.h"

#define PRODUCT_HDMI_TX  0xA0
#define HDCP_22_SNPS  0x1
#define HDCP_22_EXT  0x2

/**
 * Read product design information
 * @param baseAddr base address of controller
 * @return the design number stored in the hardware
 */
u8 id_design(hdmi_tx_dev_t *dev);

/**
 * Read product revision information
 * @param baseAddr base address of controller
 * @return the revision number stored in the hardware
 */
u8 id_revision(hdmi_tx_dev_t *dev);

/**
 * Read product line information
 * @param baseAddr base address of controller
 * @return the product line stored in the hardware
 */
u8 id_product_line(hdmi_tx_dev_t *dev);

/**
 * Read product type information
 * @param baseAddr base address of controller
 * @return the product type stored in the hardware
 */
u8 id_product_type(hdmi_tx_dev_t *dev);

/**
 * Check if HDCP is instantiated in hardware
 * @param baseAddr base address of controller
 * @return TRUE if hardware supports HDCP encryption
 */
int id_hdcp_support(hdmi_tx_dev_t *dev);

int id_hdcp14_support(hdmi_tx_dev_t *dev);

int id_hdcp22_support(hdmi_tx_dev_t *dev);

int id_phy(hdmi_tx_dev_t *dev);

#endif /* _IDENTIFICATION_H_ */


