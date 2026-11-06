#include "util/util.h"
#include "scdc/scdc.h"
#include "bsp/i2cm.h"
#include "bsp/access.h"
#include "core/irq.h"
#include "core/hdmi_reg.h"
#include "core/hdmi_core.h"
#include "core/fc.h"
#include "core/main_controller.h"


int scdc_read(hdmi_tx_dev_t *dev, u8 address, u8 size, u8 * data)
{
	if(ddc_read(dev, SCDC_SLAVE_ADDRESS, 0,0 , address, size, data)){
		pr_debug("%s: SCDC addr 0x%x read failed ",__func__, address);
		return HDMI_ERR_SCDC_FAILED;
	}
	return 0;
}

int scdc_write(hdmi_tx_dev_t *dev, u8 address, u8 size, u8 * data)
{
	if(ddc_write(dev, SCDC_SLAVE_ADDRESS, address, size, data)){
		pr_debug("%s: SCDC addr 0x%x write failed ",__func__, address);
		return HDMI_ERR_SCDC_FAILED;
	}
	return 0;
}

void scdc_enable_rr(hdmi_tx_dev_t *dev, u8 enable)
{

	if (enable == 1) {
		/* Enable Readrequest from the Tx controller */
		dev_write_mask(I2CM_SCDC_READ_UPDATE, I2CM_SCDC_READ_UPDATE_READ_REQUEST_EN_MASK, 1);
		scdc_set_rr_flag(dev, 0x01);
		irq_scdc_read_request(dev, TRUE);
	}
	if (enable == 0) {
		/* Disable ReadRequest on Tx controller */
		irq_scdc_read_request(dev, FALSE);
		scdc_set_rr_flag(dev, 0x00);
		dev_write_mask(I2CM_SCDC_READ_UPDATE, I2CM_SCDC_READ_UPDATE_READ_REQUEST_EN_MASK, 0);
	}
}

int scdc_scrambling_status(hdmi_tx_dev_t *dev)
{
	u8 read_value = 0;
	if(scdc_read(dev, SCDC_SCRAMBLER_STAT, 1, &read_value))
		pr_debug("%s: SCDC addr 0x%x read failed ",__func__, SCDC_SINK_VER);

	return (read_value & 0x01);
}

int scdc_scrambling_enable_flag(hdmi_tx_dev_t *dev, u8 enable)
{
	u8 read_value = 0;
	int res = 0;

	res = scdc_read(dev, SCDC_TMDS_CONFIG, 1 , &read_value);
	if(res != 0)
		pr_debug("%s: SCDC addr 0x%x read failed ",__func__, SCDC_TMDS_CONFIG);

	read_value = set(read_value, 0x1, enable ? 0x1 : 0x0);
	res = scdc_write(dev, SCDC_TMDS_CONFIG, 1, &read_value);
	if(res != 0)
		pr_debug("%s: SCDC addr 0x%x write failed ",__func__, SCDC_TMDS_CONFIG);

	return res;
}

int scdc_set_rr_flag(hdmi_tx_dev_t *dev, u8 enable)
{
	if(ddc_write(dev, SCDC_SLAVE_ADDRESS, SCDC_CONFIG_0, 1, &enable)){
		pr_debug("%s: SCDC addr 0x%x - 0x%x write failed ",__func__, SCDC_CONFIG_0, enable);
		return HDMI_ERR_SCDC_FAILED;
	}
	return 0;
}

int scdc_get_rr_flag(hdmi_tx_dev_t *dev, u8 * flag)
{
	if(ddc_read(dev, SCDC_SLAVE_ADDRESS, 0, 0 , SCDC_CONFIG_0, 1, flag)){
		pr_debug("%s: SCDC addr 0x%x read failed ",__func__, SCDC_CONFIG_0);
		return HDMI_ERR_SCDC_FAILED;
	}
	return 0;
}

void scdc_test_rr(hdmi_tx_dev_t *dev, u8 test_rr_delay)
{
	scdc_enable_rr(dev, 0x01);
	test_rr_delay = set(test_rr_delay, 0x80, 1);
	scdc_write(dev, SCDC_TEST_CFG_0, 1, &test_rr_delay);
}

int scdc_test_rr_update_flag(hdmi_tx_dev_t *dev)
{
	u8 read_value = 0;
	if(scdc_read(dev, SCDC_UPDATE_0, 1, &read_value))
		pr_debug("%s: SCDC addr 0x%x read failed ",__func__, SCDC_UPDATE_0);

	return read_value;
}

int scdc_read_cnt(hdmi_tx_dev_t *dev, u8 *data)
{
	if(ddc_read(dev, SCDC_SLAVE_ADDRESS, 0, 0, SCDC_ERR_DET_0_L, 8, data)){
		pr_debug("%s: SCDC addr 0x%x read failed ",__func__, SCDC_ERR_DET_0_L);
		return HDMI_ERR_SCDC_FAILED;
	}

	return 0;
}

int scrambling(hdmi_tx_dev_t *dev, u8 enable){
	int res = 0;

	if (enable == 1) {
		res = scdc_scrambling_enable_flag(dev, 1);

		udelay(100);

		/* Start/stop HDCP keep-out window generation not needed because it's always on */
		/* TMDS software reset request */
		mc_tmds_clock_reset(dev, TRUE);

		/* Enable/Disable Scrambling */
		scrambling_enable(dev, TRUE);
	} else {
		/* Enable/Disable Scrambling */
		scrambling_enable(dev, FALSE);
		res = scdc_scrambling_enable_flag(dev, 0);

		/* TMDS software reset request */
		mc_tmds_clock_reset(dev, FALSE);
	}
	return res;
}

void scrambling_enable(hdmi_tx_dev_t *dev, u8 bit)
{
	dev_write_mask(FC_SCRAMBLER_CTRL, FC_SCRAMBLER_CTRL_SCRAMBLER_ON_MASK, bit);
}

int tmds_high_rate(hdmi_tx_dev_t *dev, u8 enable)
{
	u8 value = 0;
	int res = 0;

	res = scdc_read(dev, SCDC_TMDS_CONFIG, 1 , &value);
	if(res != 0)
		pr_debug("%s: SCDC addr 0x%x read failed ",__func__, SCDC_TMDS_CONFIG);

	if(enable) {
		value |= 0x02;
	} else {
		value &= ~0x02;
	}

	res = scdc_write(dev, SCDC_TMDS_CONFIG, 1, &value);
	if(res != 0)
		pr_debug("%s: SCDC addr 0x%x write failed ",__func__, SCDC_TMDS_CONFIG);

	return res;
}

int scdc_read_channel_status(hdmi_tx_dev_t *dev)
{
	u8 read_value = 0;

	if(scdc_read(dev, SCDC_STATUS_FLAG_0, 1, &read_value))
		pr_debug("%s: SCDC addr 0x%x read failed ",__func__, SCDC_STATUS_FLAG_0);

	return read_value;
}

int scdc_read_err_characters(hdmi_tx_dev_t *dev, u8* data)
{
	return scdc_read(dev, SCDC_ERR_DET_0_L, 7, data);
}