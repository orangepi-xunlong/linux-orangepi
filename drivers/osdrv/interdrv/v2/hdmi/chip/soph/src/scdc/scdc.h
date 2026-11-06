#ifndef _SCDC_H_
#define _SCDC_H_

#include "core/hdmitx_dev.h"
#include "util/util.h"


/* SCDC Registers */
#define SCDC_SLAVE_ADDRESS 	0x54
#define SCDC_SINK_VER  		    0x01	/* sink version                                   */
#define SCDC_SOURCE_VER  	    0x02	/* source version                                 */
#define SCDC_UPDATE_0  		    0x10	/* Update_0                                       */
#define SCDC_UPDATE_0_STATUS  	0x1	    /* Status update flag                             */
#define SCDC_UPDATE_0_CED  		0x2	    /* Character error update flag                   */
#define SCDC_UPDATE_0_RR_TEST  	0x4	    /* Read request test                              */
#define SCDC_UPDATE_1 		    0x11	/* Update_1                                       */
#define SCDC_UPDATE_RESERVED  	0x12	/* 0x12-0x1f - Reserved for Update Related Uses   */
#define SCDC_TMDS_CONFIG  	    0x20	/* TMDS_Config                                    */
#define SCDC_SCRAMBLER_STAT     0x21	/* Scrambler_Status                               */
#define SCDC_CONFIG_0  		    0x30	/* Config_0                                       */
#define SCDC_CONFIG_RESERVED  	0x31	/* 0x31-0x3f - Reserved for configuration         */
#define SCDC_STATUS_FLAG_0  	0x40	/* Status_Flag_0                                  */
#define SCDC_STATUS_FLAG_0_CLK	0x1	    /* Valid TMDS Clock Detected                      */
#define SCDC_STATUS_FLAG_0_CH0	0x2	    /* Valid Data at Chanel0                          */
#define SCDC_STATUS_FLAG_0_CH1	0x4	    /* Valid Data at Chanel1                          */
#define SCDC_STATUS_FLAG_0_CH2	0x8	    /* Valid Data at Chanel2                          */
#define SCDC_STATUS_FLAG_1  	0x41	/* Status_Flag_1                                  */
#define SCDC_STATUS_RESERVED 	0x42	/* 0x42-0x4f - Reserved for Status Related Uses   */
#define SCDC_ERR_DET_0_L  	    0x50	/* Err_Det_0_L                                    */
#define SCDC_ERR_DET_0_H  	    0x51	/* Err_Det_0_H                                    */
#define SCDC_ERR_DET_1_L  	    0x52	/* Err_Det_1_L                                    */
#define SCDC_ERR_DET_1_H  	    0x53	/* Err_Det_1_H                                    */
#define SCDC_ERR_DET_2_L  	    0x54	/* Err_Det_2_L                                    */
#define SCDC_ERR_DET_2_H  	    0x55	/* Err_Det_2_H                                    */
#define SCDC_ERR_DET_CHKSUM  	0x56	/* Err_Det_Checksum                               */
#define SCDC_TEST_CFG_0  	    0xc0	/* Test_config_0                                  */
#define SCDC_TEST_RESERVED  	0xc1	/* 0xc1-0xcf - Reserved for test features         */
#define SCDC_MAN_OUI_3RD  	    0xd0	/* Manufacturer IEEE OUI, Third Octet             */
#define SCDC_MAN_OUI_2ND  	    0xd1	/* Manufacturer IEEE OUI, Second Octet            */
#define SCDC_MAN_OUI_1ST  	    0xd2	/* Manufacturer IEEE OUI, First Octet             */
#define SCDC_DEVICE_ID  	    0xd3	/* 0xd3-0xdd - Device ID                          */
#define SCDC_MAN_SPECIFIC  	    0xde	/* 0xde-0xff - ManufacturerSpecific               */

int scdc_read(hdmi_tx_dev_t *dev, u8 address, u8 size, u8 * data);

int scdc_write(hdmi_tx_dev_t *dev, u8 address, u8 size, u8 * data);

void scdc_enable_rr(hdmi_tx_dev_t *dev, u8 enable);

int scdc_scrambling_status(hdmi_tx_dev_t *dev);

int scdc_scrambling_enable_flag(hdmi_tx_dev_t *dev, u8 flag);

int scdc_set_rr_flag(hdmi_tx_dev_t *dev, u8 enable);

int scdc_get_rr_flag(hdmi_tx_dev_t *dev, u8 * flag);

void scdc_test_rr(hdmi_tx_dev_t *dev, u8 test_rr_delay);

int scdc_test_rr_update_flag(hdmi_tx_dev_t *dev);

int scdc_read_cnt(hdmi_tx_dev_t *dev, u8 * data);

int scrambling(hdmi_tx_dev_t *dev, u8 enable);

void scrambling_enable(hdmi_tx_dev_t *dev, u8 bit);

int tmds_high_rate(hdmi_tx_dev_t *dev, u8 enable);

int scdc_read_channel_status(hdmi_tx_dev_t *dev);

int scdc_read_err_characters(hdmi_tx_dev_t *dev, u8* data);

#endif	/* _SCDC_H_ */
