#ifndef I2CM_H_
#define I2CM_H_

#include "core/hdmitx_dev.h"
#include "util/util.h"

#define I2CDDC_TIMEOUT 100
#define I2C_MIN_FS_SCL_HIGH_TIME   61 //63 //75
#define I2C_MIN_FS_SCL_LOW_TIME    132 //137 //163
#define I2C_MIN_SS_SCL_HIGH_TIME   4592 //4737 //5625
#define I2C_MIN_SS_SCL_LOW_TIME    5102 //5263 //6250

#define I2C_CLK 100000
#define I2C_SCL_HIGH_TIME_NS 4500
#define I2C_SCL_LOW_TIME_NS  5200

/** I2C clock configuration
 *
 * @param sfr_clock external clock supplied to controller
 * @param value of standard speed low time counter (refer to HDMITXCTRL databook)
 * @param value of standard speed high time counter (refer to HDMITXCTRL databook)
 * @param value of fast speed low time counter (refer to HDMITXCTRL databook)
 * @param value of fast speed high time counter (refer to HDMITXCTRL databook)
 */
void i2cddc_clk_config(hdmi_tx_dev_t * dev, u16 sfr_clock, u16 ss_low_ckl, u16 ss_high_ckl, u16 fs_low_ckl, u16 fs_high_ckl);

void i2cddc_clk_set_divs(hdmi_tx_dev_t * dev);

/** Set the speed mode (standard/fast mode)
 *
 * @param fast mode selection, 0 standard - 1 fast
 */
void i2cddc_fast_mode(hdmi_tx_dev_t * dev, u8 fast);

/** Enable disable interrupts.
 *
 * @param mask to enable or disable the masking (u32 baseAddr, true to mask,
 * ie true to stop seeing the interrupt).
 */
void i2cddc_mask_interrupts(hdmi_tx_dev_t * dev, u8 mask);

/** Read from extended addresses, E-DDC.
 *
 * @param i2cAddr i2c device address to read data
 * @param addr base address of the module registers
 * @param segment segment to read from
 * @param pointer in segment to read
 * @param value pointer to data read
 * @returns 0 if ok and error in other cases
 */
int ddc_read(hdmi_tx_dev_t * dev, u8 i2c_addr, u8 segment, u8 pointer, u8 addr, u8 len, u8 * data);

/** Write from extended addresses, E-DDC.
 *
 * @param i2cAddr i2c device address to read data
 * @param addr base address of the module registers
 * @param len lenght to write
 * @param data pointer to data write
 * @returns 0 if ok and error in other cases
 */
int ddc_write(hdmi_tx_dev_t * dev, u8 i2c_addr, u8 addr, u8 len, u8 * data);

/** Activate I2C bus clear function.
 *
 * @returns 0 if ok and error in other cases
 */
int i2c_bus_clear(hdmi_tx_dev_t * dev);

int i2c_reset(hdmi_tx_dev_t * dev);


#endif /* I2CM_H_ */
