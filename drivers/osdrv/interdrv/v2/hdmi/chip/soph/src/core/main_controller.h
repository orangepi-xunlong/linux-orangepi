#ifndef _MAINCONTROLLER_H_
#define _MAINCONTROLLER_H_

#include "core/hdmitx_dev.h"
#include "util/util.h"

void mc_disable_all_clocks(hdmi_tx_dev_t *dev);

void mc_enable_all_clocks(hdmi_tx_dev_t *dev);

void mc_audio_sampler_clock_enable(hdmi_tx_dev_t *dev, u8 bit);

void mc_tmds_clock_reset(hdmi_tx_dev_t *dev, u8 bit);

void mc_phy_reset(hdmi_tx_dev_t *dev, u8 bit);

/**
 * Initializes PHY and core clocks
 * @param baseAddr base address of controller
 * @param dataEnablePolarity data enable polarity
 * @param pixelClock pixel clock [10KHz]
 * @return TRUE if successful
 */
int control_initialize(hdmi_tx_dev_t *dev);

/**
 * Go into standby mode: stop all clocks from all modules except for the CEC (refer to CEC for more detail)
 * @param baseAddr base address of controller
 * @return TRUE if successful
 */
int control_standby(hdmi_tx_dev_t *dev);

/**
 * Clear all controller interrputs (except for hdcp)
 * @param baseAddr base address of controller
 * @return TRUE if successful
 */
int control_interrupt_clear_all(hdmi_tx_dev_t *dev);
#endif	/* HALMAINCONTROLLER_H_ */
