#include "phy/phy_i2c.h"
#include "bsp/access.h"
#include "core/hdmi_reg.h"
#include "phy/phy.h"

#define I2C_TIMEOUT				100

void phy_i2c_fast_mode(hdmi_tx_dev_t *dev, u8 bit)
{
	dev_write_mask(PHY_I2CM_DIV, PHY_I2CM_DIV_FAST_STD_MODE_MASK, bit);
}

void phy_i2c_master_reset(hdmi_tx_dev_t *dev)
{
	dev_write_mask(PHY_I2CM_SOFTRSTZ, PHY_I2CM_SOFTRSTZ_I2C_SOFTRSTZ_MASK, 1);
}

void phy_i2c_mask_interrupts(hdmi_tx_dev_t *dev, int mask)
{
	dev_write_mask(PHY_I2CM_INT, PHY_I2CM_INT_DONE_MASK_MASK, mask ? 1 : 0);
	dev_write_mask(PHY_I2CM_CTLINT, PHY_I2CM_CTLINT_ARBITRATION_MASK_MASK, mask ? 1 : 0);
	dev_write_mask(PHY_I2CM_CTLINT, PHY_I2CM_CTLINT_NACK_MASK_MASK, mask ? 1 : 0);
}

void phy_i2c_slave_address(hdmi_tx_dev_t *dev, u8 value)
{
	dev_write_mask(PHY_I2CM_SLAVE, PHY_I2CM_SLAVE_SLAVEADDR_MASK, value);
}

int phy_i2c_write(hdmi_tx_dev_t *dev, u8 addr, u16 data)
{
	int timeout = PHY_TIMEOUT;
	u32 status  = 0;

	//Set address
	dev_write(PHY_I2CM_ADDRESS, addr);

	//Set value
	dev_write(PHY_I2CM_DATAO_1, (u8) ((data >> 8) & 0xFF));
	dev_write(PHY_I2CM_DATAO_0, (u8) (data & 0xFF));

	dev_write(PHY_I2CM_OPERATION, PHY_I2CM_OPERATION_WR_MASK);

	do {
		udelay(10);
		status = dev_read_mask(IH_I2CMPHY_STAT0,
				IH_I2CMPHY_STAT0_I2CMPHYERROR_MASK | IH_I2CMPHY_STAT0_I2CMPHYDONE_MASK);
	} while (status == 0 && (timeout--));

	dev_write(IH_I2CMPHY_STAT0, status); //clear read status

	if(status & IH_I2CMPHY_STAT0_I2CMPHYERROR_MASK){
		pr_err( "%s: I2C PHY write failed\n",__func__);
		return -1;
	}

	if(status & IH_I2CMPHY_STAT0_I2CMPHYDONE_MASK){
		return 0;
	}

	pr_err( "%s: ASSERT I2C Write timeout - check PHY - exiting\n",__func__);
	return -1;
}

int phy_i2c_read(hdmi_tx_dev_t *dev, u8 addr, u16 * value)
{
	int timeout = PHY_TIMEOUT;
	u32 status  = 0;

	//Set address
	dev_write(PHY_I2CM_ADDRESS, addr);
	dev_write(PHY_I2CM_OPERATION, PHY_I2CM_OPERATION_RD_MASK);

	do {
		udelay(10);
		status = dev_read_mask(IH_I2CMPHY_STAT0,
				IH_I2CMPHY_STAT0_I2CMPHYERROR_MASK | IH_I2CMPHY_STAT0_I2CMPHYDONE_MASK);
	} while (status == 0 && (timeout--));

	dev_write(IH_I2CMPHY_STAT0, status); //clear read status

	if(status & IH_I2CMPHY_STAT0_I2CMPHYERROR_MASK){
		pr_err( "%s: I2C Read failed\n",__func__);
		return -1;
	}

	if(status & IH_I2CMPHY_STAT0_I2CMPHYDONE_MASK){

		*value = ((u16) (dev_read((PHY_I2CM_DATAI_1)) << 8)
				| dev_read((PHY_I2CM_DATAI_0)));
		return 0;
	}

	pr_err( "%s: ASSERT I2C Read timeout - check PHY - exiting\n",__func__);
	return -1;
}
