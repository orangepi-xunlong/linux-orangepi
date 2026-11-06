#include "bsp/i2cm.h"
#include "phy/phy.h"
#include "bsp/access.h"
#include "core/hdmi_reg.h"
#include <linux/unistd.h>

#define I2CM_OPERATION_READ        0x01
#define I2CM_OPERATION_READ_EXT    0x02
#define I2CM_OPERATION_READ_SEQ    0x04
#define I2CM_OPERATION_READ_SEQ_EXT 0x08
#define I2CM_OPERATION_WRITE       0x10
#define I2C_DIV_FACTOR   100000
#define I2C_CLK_RATE_KHZ 25000

/*********************  PRIVATE FUNCTIONS ***********************/

/**
 * calculate the fast sped high time counter - round up
 */
u16 _scl_calc(u16 sfrClock, u16 sclMinTime)
{
	unsigned long tmp_scl_period = 0;
	if (((sfrClock * sclMinTime) % I2C_DIV_FACTOR) != 0) {
		tmp_scl_period = (unsigned long)((sfrClock * sclMinTime) +
				(I2C_DIV_FACTOR - ((sfrClock * sclMinTime) % I2C_DIV_FACTOR))) / I2C_DIV_FACTOR;
	}
	else {
		tmp_scl_period = (unsigned long)(sfrClock * sclMinTime) / I2C_DIV_FACTOR;
	}
	return (u16)(tmp_scl_period);
}

void _fast_speed_high_clk_ctrl(hdmi_tx_dev_t * dev, u16 value)
{
	dev_write(I2CM_FS_SCL_HCNT_1_ADDR, (u8) (value >> 8));
	dev_write(I2CM_FS_SCL_HCNT_0_ADDR, (u8) (value >> 0));
}

void _fast_speed_low_clk_ctrl(hdmi_tx_dev_t * dev, u16 value)
{
	dev_write(I2CM_FS_SCL_LCNT_1_ADDR, (u8) (value >> 8));
	dev_write(I2CM_FS_SCL_LCNT_0_ADDR, (u8) (value >> 0));
}

void _standard_speed_high_clk_ctrl(hdmi_tx_dev_t * dev, u16 value)
{
	dev_write(I2CM_SS_SCL_HCNT_1_ADDR, (u8) (value >> 8));
	dev_write(I2CM_SS_SCL_HCNT_0_ADDR, (u8) (value >> 0));
}

void _standard_speed_low_clk_ctrl(hdmi_tx_dev_t * dev, u16 value)
{
	dev_write(I2CM_SS_SCL_LCNT_1_ADDR, (u8) (value >> 8));
	dev_write(I2CM_SS_SCL_LCNT_0_ADDR, (u8) (value >> 0));
}

int _write(hdmi_tx_dev_t * dev, u8 i2c_addr, u8 addr, u8 data)
{
	int timeout = I2CDDC_TIMEOUT;
	u32 status = 0;

	dev_write_mask(I2CM_SLAVE, I2CM_SLAVE_SLAVEADDR_MASK, i2c_addr);
	dev_write(I2CM_ADDRESS, addr);
	dev_write(I2CM_DATAO, data);
	dev_write(I2CM_OPERATION, I2CM_OPERATION_WRITE);
	do {
		if (!phy_hot_plug_state(dev)) {
			pr_debug("%s:%d Hot Plug = %s\n",
				__FUNCTION__, __LINE__, phy_hot_plug_state(dev) ? "ON" : "OFF");
			return -2;
		}
		udelay(10);
		status = dev_read_mask(IH_I2CM_STAT0,
				IH_I2CM_STAT0_I2CMASTERERROR_MASK |
				IH_I2CM_STAT0_I2CMASTERDONE_MASK);
	} while (status == 0 && (timeout--));

	dev_write(IH_I2CM_STAT0, status); //clear read status

	if(status & IH_I2CM_STAT0_I2CMASTERERROR_MASK){
		pr_debug( "%s: I2C DDC write failed",__func__);
		return -1;
	}

	if(status & IH_I2CM_STAT0_I2CMASTERDONE_MASK){
		return 0;
	}

	pr_err("%s: ASSERT I2C Write timeout - check system - exiting",__func__);
	return -1;
}

int _read(hdmi_tx_dev_t * dev, u8 i2cAddr, u8 segment, u8 pointer, u8 addr,   u8 * value)
{
	int timeout = I2CDDC_TIMEOUT;
	u32 status = 0;

	dev_write_mask(I2CM_SLAVE, I2CM_SLAVE_SLAVEADDR_MASK, i2cAddr);
	dev_write(I2CM_ADDRESS, addr);
	dev_write(I2CM_SEGADDR, segment);
	dev_write(I2CM_SEGPTR, pointer);

	if(pointer)
		dev_write(I2CM_OPERATION, I2CM_OPERATION_READ_EXT);
	else
		dev_write(I2CM_OPERATION, I2CM_OPERATION_READ);

	do {
		if (!phy_hot_plug_state(dev)) {
			pr_debug("%s:%d Hot Plug = %s\n",
				__FUNCTION__, __LINE__, phy_hot_plug_state(dev) ? "ON" : "OFF");
			return -2;
		}
		udelay(1500);
		status = dev_read_mask(IH_I2CM_STAT0,
					IH_I2CM_STAT0_I2CMASTERERROR_MASK |
					IH_I2CM_STAT0_I2CMASTERDONE_MASK);
	} while (status == 0 && (timeout--));

	dev_write(IH_I2CM_STAT0, status); //clear read status

	if(status & IH_I2CM_STAT0_I2CMASTERERROR_MASK){
		pr_debug( "%s: I2C DDC Read failed for i2cAddr 0x%x seg 0x%x pointer 0x%x addr 0x%x",__func__,
					i2cAddr, segment, pointer, addr);
		return -1;
	}

	if(status & IH_I2CM_STAT0_I2CMASTERDONE_MASK){
		*value = (u8) dev_read(I2CM_DATAI);
		return 0;
	}

	pr_err("%s: ASSERT I2C DDC Read timeout - check system - exiting",__func__);
	return -1;
}

int _read8(hdmi_tx_dev_t * dev, u8 i2c_addr, u8 segment, u8 pointer, u8 addr, u8 * value)
{
	int timeout = I2CDDC_TIMEOUT;
	u32 status = 0;

	dev_write_mask(I2CM_SLAVE, I2CM_SLAVE_SLAVEADDR_MASK, i2c_addr);
	dev_write(I2CM_SEGADDR, segment);
	dev_write(I2CM_SEGPTR, pointer);
	dev_write(I2CM_ADDRESS, addr);

	if(pointer)
		dev_write(I2CM_OPERATION, I2CM_OPERATION_READ_SEQ_EXT);
	else
		dev_write(I2CM_OPERATION, I2CM_OPERATION_READ_SEQ);

	do {
		if (!phy_hot_plug_state(dev)) {
			pr_debug("%s:%d Hot Plug = %s\n",
				__FUNCTION__, __LINE__, phy_hot_plug_state(dev) ? "ON" : "OFF");
			return -2;
		}
		udelay(1500);
		status = dev_read_mask(IH_I2CM_STAT0,
				IH_I2CM_STAT0_I2CMASTERERROR_MASK |
				IH_I2CM_STAT0_I2CMASTERDONE_MASK);
	} while (status == 0 && (timeout--));

	dev_write(IH_I2CM_STAT0, status); //clear read status

	if(status & IH_I2CM_STAT0_I2CMASTERERROR_MASK){
		pr_debug("%s: I2C DDC Read8 extended failed for i2cAddr 0x%x seg 0x%x pointer 0x%x addr 0x%x",__func__,
				i2c_addr, segment, pointer, addr);
		return -1;
	}

	if(status & IH_I2CM_STAT0_I2CMASTERDONE_MASK){
		int i = 0;
		while(i < 8){ //read 8 bytes
			value[i] = (u8) dev_read(I2CM_READ_BUFF0 + (4 * i) );
			i +=1;
		}
		return 0;
	}

	pr_err("%s: ASSERT I2C DDC Read extended timeout - check system - exiting",__func__);
	return -1;
}

/*********************  PUBLIC FUNCTIONS ***********************/

void i2cddc_clk_config(hdmi_tx_dev_t * dev, u16 sfr_clock,
			u16 ss_low_ckl, u16 ss_high_ckl, u16 fs_low_ckl, u16 fs_high_ckl)
{
	_standard_speed_low_clk_ctrl(dev, _scl_calc(sfr_clock, ss_low_ckl));
	_standard_speed_high_clk_ctrl(dev, _scl_calc(sfr_clock, ss_high_ckl));
	_fast_speed_low_clk_ctrl(dev, _scl_calc(sfr_clock, fs_low_ckl));
	_fast_speed_high_clk_ctrl(dev, _scl_calc(sfr_clock, fs_high_ckl));
}

void i2cddc_clk_set_divs(hdmi_tx_dev_t * dev)
{
	u32 low_ns, high_ns;
	u32 div_low, div_high;

	/* Standard-mode */
	if (dev->i2c.scl_high_ns < 4000)
		high_ns = 4500;
	else
		high_ns = dev->i2c.scl_high_ns;

	if (dev->i2c.scl_low_ns < 4700)
		low_ns = 5200;
	else
		low_ns = dev->i2c.scl_low_ns;

	/* Adjust to avoid overflow */
	div_low = (I2C_CLK_RATE_KHZ * low_ns) / 1000000;
	if ((I2C_CLK_RATE_KHZ * low_ns) % 1000000)
		div_low++;

	div_high = (I2C_CLK_RATE_KHZ * high_ns) / 1000000;
	if ((I2C_CLK_RATE_KHZ * high_ns) % 1000000)
		div_high++;

	/* Maximum divider supported by hw is 0xffff */
	if (div_low > 0xffff)
		div_low = 0xffff;

	if (div_high > 0xffff)
		div_high = 0xffff;

	pr_debug("%s: div_low is 0x%02x, div_high is 0x%02x \n", __func__, div_low, div_high);

	dev_write(I2CM_SS_SCL_LCNT_0_ADDR, (div_low & 0xff));
	dev_write(I2CM_SS_SCL_LCNT_1_ADDR, (div_low >> 8) & 0xff);

	dev_write(I2CM_SS_SCL_HCNT_0_ADDR, (div_high >> 0));
	dev_write(I2CM_SS_SCL_HCNT_1_ADDR, ((div_high >> 8) & 0xff));
}

void i2cddc_fast_mode(hdmi_tx_dev_t * dev, u8 value)
{
	/* bit 4 selects between high and standard speed operation */
	dev_write_mask(I2CM_DIV, I2CM_DIV_FAST_STD_MODE_MASK, value);
}


void i2cddc_mask_interrupts(hdmi_tx_dev_t * dev, u8 mask)
{
	dev_write_mask(I2CM_INT, I2CM_INT_DONE_MASK, mask ? 1 : 0);
	dev_write_mask(I2CM_CTLINT, I2CM_CTLINT_ARBITRATION_MASK, mask ? 1 : 0);
	dev_write_mask(I2CM_CTLINT, I2CM_CTLINT_NACK_MASK, mask ? 1 : 0);
}

int ddc_write(hdmi_tx_dev_t * dev, u8 i2c_addr, u8 addr, u8 len, u8 * data)
{
	int i, status = 0;

	for(i = 0; i < len; i++){
		int tries = 3;
		do {
			if (!phy_hot_plug_state(dev)) {
				pr_debug("%s:%d Hot Plug = %s\n",
					__FUNCTION__, __LINE__, phy_hot_plug_state(dev) ? "ON" : "OFF");
				return -2;
			}
			status = _write(dev, i2c_addr, addr, data[i]);
		} while (status && tries--);

		if(status) //Error after 3 failed writes
			return status;
	}
	return 0;
}

int ddc_read(hdmi_tx_dev_t * dev, u8 i2c_addr, u8 segment, u8 pointer, u8 addr, u8 len, u8 * data)
{
	int i, status = 0;

	for(i = 0; i < len;){
		int tries = 3;
		if ((len - i) >= 8){
			do {
				if (!phy_hot_plug_state(dev)) {
					pr_debug("%s:%d Hot Plug = %s\n",
						__FUNCTION__, __LINE__, phy_hot_plug_state(dev) ? "ON" : "OFF");
					return -2;
				}
				status = _read8(dev, i2c_addr, segment, pointer, addr + i,  &(data[i]));
			} while (status && tries--);

			if(status) //Error after 3 failed writes
				return status;
			i +=8;
		} else {
			do {
				if (!phy_hot_plug_state(dev)) {
					pr_debug("%s:%d Hot Plug = %s\n",
						__FUNCTION__, __LINE__, phy_hot_plug_state(dev) ? "ON" : "OFF");
					return -2;
				}
				status = _read(dev, i2c_addr, segment, pointer, addr + i,  &(data[i]));
			} while (status && tries--);

			if(status) //Error after 3 failed writes
				return status;
			i++;
		}
	}
	return 0;
}

int i2c_bus_clear(hdmi_tx_dev_t * dev)
{
	dev_write_mask(I2CM_OPERATION, I2CM_OPERATION_BUSCLEAR_MASK, 1);

	return 0;
}

int i2c_reset(hdmi_tx_dev_t * dev)
{
	dev_write_mask(I2CM_SOFTRSTZ, I2CM_SOFTRSTZ_I2C_SOFTRSTZ_MASK, 1);

	return 0;
}