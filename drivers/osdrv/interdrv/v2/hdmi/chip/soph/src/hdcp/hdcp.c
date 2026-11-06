#include "core/hdmi_reg.h"
#include "core/hdmi_core.h"
#include "hdcp/hdcp.h"
#include "identification/identification.h"
#include "bsp/access.h"

#define KSV_LEN  5 // KSV value size
#define SIZE	(160/8)
#define KSIZE	(1024/8)

/* HDCP Interrupt fields */
#define INT_KSV_ACCESS		(A_APIINTSTAT_KSVACCESSINT_MASK)
#define INT_KSV_SHA1		(A_APIINTSTAT_KSVSHA1CALCINT_MASK)
#define INT_KSV_SHA1_DONE	(A_APIINTSTAT_KSVSHA1CALCDONEINT_MASK)
#define INT_HDCP_FAIL		(A_APIINTSTAT_HDCP_FAILED_MASK)
#define INT_HDCP_ENGAGED	(A_APIINTSTAT_HDCP_ENGAGED_MASK)

void _set_device_mode(hdmi_tx_dev_t *dev, video_mode_t mode)
{
	u8 set_mode = (mode == HDMI ? 1 : 0) ; // 1 - HDMI : 0 - DVI
	dev_write_mask(A_HDCPCFG0, A_HDCPCFG0_HDMIDVI_MASK, set_mode);
}

void _enable_feature11(hdmi_tx_dev_t *dev, u8 bit)
{
	dev_write_mask(A_HDCPCFG0, A_HDCPCFG0_EN11FEATURE_MASK, bit);
}

void _override_hdcp2p2_switch(hdmi_tx_dev_t *dev, u8 bit)
{
	dev_write_mask(HDCP22REG_CTRL, HDCP22REG_CTRL_OVR_EN_MASK,  bit);
}

void hdcp_rxdetect(hdmi_tx_dev_t *dev, u8 enable)
{
	dev_write_mask(A_HDCPCFG0, A_HDCPCFG0_RXDETECT_MASK, enable);
}

void _enable_avmute(hdmi_tx_dev_t *dev, u8 bit)
{
	dev_write_mask(A_HDCPCFG0, A_HDCPCFG0_AVMUTE_MASK, bit);
}

void _ri_check(hdmi_tx_dev_t *dev, u8 bit)
{
	dev_write_mask(A_HDCPCFG0, A_HDCPCFG0_SYNCRICHECK_MASK, bit);
}

void _bypass_encryption(hdmi_tx_dev_t *dev, u8 bit)
{
	dev_write_mask(A_HDCPCFG0, A_HDCPCFG0_BYPENCRYPTION_MASK, bit);
}

void _enable_i2c_fast_mode(hdmi_tx_dev_t *dev, u8 bit)
{
	dev_write_mask(A_HDCPCFG0, A_HDCPCFG0_I2CFASTMODE_MASK, bit);
}

void _enhanced_link_verification(hdmi_tx_dev_t *dev, u8 bit)
{
	dev_write_mask(A_HDCPCFG0, A_HDCPCFG0_ELVENA_MASK, bit);
}

void hdcp_sw_reset(hdmi_tx_dev_t *dev)
{
	//Software reset signal, active by writing a zero and auto cleared to 1 in the following cycle
	dev_write_mask(A_HDCPCFG1, A_HDCPCFG1_SWRESET_MASK, 0);
}

void _disable_encryption(hdmi_tx_dev_t *dev, u8 bit)
{
	dev_write_mask(A_HDCPCFG1, A_HDCPCFG1_ENCRYPTIONDISABLE_MASK, bit);
}

void _encoding_packet_header(hdmi_tx_dev_t *dev, u8 bit)
{
	dev_write_mask(A_HDCPCFG1, A_HDCPCFG1_PH2UPSHFTENC_MASK, bit);
}

void _disable_ksv_list_check(hdmi_tx_dev_t *dev, u8 bit)
{
	dev_write_mask(A_HDCPCFG1, A_HDCPCFG1_DISSHA1CHECK_MASK, bit);
}

u8 _hdcp_engaged(hdmi_tx_dev_t *dev)
{
	return dev_read_mask(A_HDCPOBS0, A_HDCPOBS0_HDCPENGAGED_MASK);
}

u8 _authentication_state(hdmi_tx_dev_t *dev)
{
	return dev_read_mask(A_HDCPOBS0, A_HDCPOBS0_SUBSTATEA_MASK | A_HDCPOBS0_STATEA_MASK);
}

u8 _cipher_state(hdmi_tx_dev_t *dev)
{
	return dev_read_mask(A_HDCPOBS2, A_HDCPOBS2_STATEE_MASK);
}

u8 _revocation_state(hdmi_tx_dev_t *dev)
{
	return dev_read_mask((A_HDCPOBS1), A_HDCPOBS1_STATER_MASK);
}

u8 _oess_state(hdmi_tx_dev_t *dev)
{
	return dev_read_mask((A_HDCPOBS1), A_HDCPOBS1_STATEOEG_MASK);
}

u8 _eess_state(hdmi_tx_dev_t *dev)
{
	return dev_read_mask((A_HDCPOBS2), A_HDCPOBS2_STATEEEG_MASK);
}

u8 _debug_info(hdmi_tx_dev_t *dev)
{
	return dev_read(A_HDCPOBS3);
}

void _interrupt_clear(hdmi_tx_dev_t *dev, u8 value)
{
	dev_write((A_APIINTCLR), value);
}

u8 _interrupt_status(hdmi_tx_dev_t *dev)
{
	return dev_read(A_APIINTSTAT);
}

void _interrupt_mask(hdmi_tx_dev_t *dev, u8 value)
{;
	dev_write((A_APIINTMSK), value);
}

u8 _hdcp_interrupt_mask_status(hdmi_tx_dev_t *dev)
{
	return dev_read(A_APIINTMSK);
}

void _hsync_polarity(hdmi_tx_dev_t *dev, u8 bit)
{
	dev_write_mask(A_VIDPOLCFG, A_VIDPOLCFG_HSYNCPOL_MASK, bit);
}


void _vsync_polarity(hdmi_tx_dev_t *dev, u8 bit)
{
	dev_write_mask(A_VIDPOLCFG, A_VIDPOLCFG_VSYNCPOL_MASK, bit);
}

void _hdcp_data_enable_polarity(hdmi_tx_dev_t *dev, u8 bit)
{
	dev_write_mask(A_VIDPOLCFG, A_VIDPOLCFG_DATAENPOL_MASK, bit);
}

void _unencrypted_video_color(hdmi_tx_dev_t *dev, u8 value)
{

	dev_write_mask(A_VIDPOLCFG, A_VIDPOLCFG_UNENCRYPTCONF_MASK, value);
}

void _oess_window_size(hdmi_tx_dev_t *dev, u8 value)
{
	dev_write((A_OESSWCFG), value);
}

u16 _core_version(hdmi_tx_dev_t *dev)
{
	u16 version = 0;
	version = dev_read(A_COREVERLSB);
	version |= dev_read(A_COREVERMSB) << 8;
	return version;
}

u8 _controller_version(hdmi_tx_dev_t *dev)
{
	return dev_read(A_HDCPCFG0);
}

void _memory_access_request(hdmi_tx_dev_t *dev, u8 bit)
{
	dev_write_mask(A_KSVMEMCTRL, A_KSVMEMCTRL_KSVMEMREQUEST_MASK, bit);
}

u8 _memory_access_granted(hdmi_tx_dev_t *dev)
{
	return (u8)((dev_read(A_KSVMEMCTRL) & A_KSVMEMCTRL_KSVMEMACCESS_MASK) >> 1);
}

void _update_ksv_list_state(hdmi_tx_dev_t *dev, u8 bit)
{
	dev_write_mask(A_KSVMEMCTRL, A_KSVMEMCTRL_SHA1FAIL_MASK, bit);
	dev_write_mask(A_KSVMEMCTRL, A_KSVMEMCTRL_KSVCTRLUPD_MASK, 1);
	dev_write_mask(A_KSVMEMCTRL, A_KSVMEMCTRL_KSVCTRLUPD_MASK, 0);
}

u8 _ksv_sha1_status(hdmi_tx_dev_t *dev)
{
	return (u8)((dev_read(A_KSVMEMCTRL) & A_KSVMEMCTRL_KSVSHA1STATUS_MASK) >> 4);
}

u16 _bstatus_read(hdmi_tx_dev_t *dev)
{
	u16 bstatus = 0;

	bstatus	= dev_read(HDCP_BSTATUS) ;
	bstatus	|= dev_read(HDCP_BSTATUS + ADDR_JUMP) << 8;
	return bstatus;
}

void _m0_read(hdmi_tx_dev_t *dev, u8 * data)
{
	u8 i = 0;
	for (i = 0 ; i < HDCP_M0_SIZE; i++ ) {
		data[i] = dev_read(HDCP_M0 + (i * ADDR_JUMP));
	}
}
#if 0
int _ksv_list_read(hdmi_tx_dev_t *dev, u16 size, u8 * data)
{
	u8 i = 0;

	if(size > HDCP_KSV_SIZE) {
		LOGGER(SNPS_ERROR,"Invalid number of devices");
		return -1;
	}

	for (i = 0 ; i < size; i++ ){
		data[i] = dev_read(HDCP_KSV + (i * ADDR_JUMP));
	}
	return 0;
}
#endif
void _sha1vh_read(hdmi_tx_dev_t *dev, u8 * data)
{
	u8 i = 0;
	for (i = 0 ; i < HDCP_VH_SIZE; i++ ) {
		data[i] = dev_read(HDCP_VH + (i * ADDR_JUMP));
	}
}

void _revoc_list_write(hdmi_tx_dev_t *dev, u16 addr, u8 data)
{
	dev_write(HDCP_REVOC_LIST + addr, data);
}

void _an_write(hdmi_tx_dev_t *dev, u8 * data)
{
	short i = 0;
	if (data != 0) {
		for (i = 0; i <= (HDCPREG_AN7 - HDCPREG_AN0); i++) {
			dev_write((HDCPREG_AN0 + (i * ADDR_JUMP)), data[i]);
		}
		dev_write_mask(HDCPREG_ANCONF, HDCPREG_ANCONF_OANBYPASS_MASK, 1);
	} else {
		dev_write_mask(HDCPREG_ANCONF, HDCPREG_ANCONF_OANBYPASS_MASK, 0);
	}
}

u8 _bksv_read(hdmi_tx_dev_t *dev, u8 * bksv)
{
	short i = 0;
	if (bksv != 0) {
		for (i = 0; i <= (HDCPREG_BKSV4 - HDCPREG_BKSV0); i++) {
			bksv[i] =  dev_read(HDCPREG_BKSV0 + (i * 4));
		}
		return i;
	} else {
		return 0;
	}
}

u8 _hdcp_2p2_version(hdmi_tx_dev_t *dev)
{
	pr_err("%s:TBI", __func__);
	//1 - Configure the RX FIFO to read HDCP 2.2 version:
	//	a. Write 8'h50 to h22s_rxmsg_addr.rxmsg_addr.
	//	b. Write 8'h01 to h22s_rxmsg_nbyteslow.rxmsg_nbytes_lo.
	//	c. Write 2'b00 to h22s_rxmsg_nbyteshigh.rxmsg_nbytes_hi.
	//	d. Write 1'b0 to h22s_rxmsg_cfg.rxmsg_rcv_autostart.
	//	e. Write 1'b1 to h22s_rxmsg_ctrl.rxmsg_rcv_start.

	//2 - Wait for the interrupt hdcp2version_chg to be asserted.

	//3 - The HDCP 2.2 version can be read by the software using two different procedures:
	//	a. Read h22s_hdcp2version_sts.hdcp2version_sts.
	//	Or
	//	b. Read from h22s_rxmsg_byte.rxmsg_byte.
	//	Write 1'b1 to h22s_rxmsg_ff_ctrl.rxmsgfifo_pop
	return 0x0;
}

u8 _hdcp_2p2_reset_engine(hdmi_tx_dev_t *dev)
{
	pr_err("%s:TBI", __func__);
	//Reset the HDCP 2.2 engine, write “1” in the h22s_ctrl.swrstreq bit field register.
	return TRUE;
}

u8 _hdcp_2p2_authentication(hdmi_tx_dev_t *dev)
{
	pr_err("%s:TBI", __func__);
	//Perform the HDCP 2.2 authentication as described in the “HDCP 2.2 Authentication” on page 228.
	//Skip the remaining steps as they are related to HDMI 1.4.
	return TRUE;
}

#ifdef ROMLESS
void _write_aksv(hdmi_tx_dev_t *dev, u8 aksv[7])
{
	access_CoreWrite(dev, aksv[0], (A_HDCPREG_DPK6), 0, 8);
	access_CoreWrite(dev, aksv[1], (A_HDCPREG_DPK5), 0, 8);
	access_CoreWrite(dev, aksv[2], (A_HDCPREG_DPK4), 0, 8);
	access_CoreWrite(dev, aksv[3], (A_HDCPREG_DPK3), 0, 8);
	access_CoreWrite(dev, aksv[4], (A_HDCPREG_DPK2), 0, 8);
	access_CoreWrite(dev, aksv[5], (A_HDCPREG_DPK1), 0, 8);
	access_CoreWrite(dev, aksv[6], (A_HDCPREG_DPK0), 0, 8);
}

void _wait_mem_access(hdmi_tx_dev_t *dev)
{
	while (!access_CoreRead(dev, (A_HDCPREG_RMLSTS), 6, 1)) ;
}

void _write_seed(hdmi_tx_dev_t *dev, u8 encKey[2])
{
	access_CoreWrite(dev, encKey[0], (A_HDCPREG_SEED1), 0, 8);
	access_CoreWrite(dev, encKey[1], (A_HDCPREG_SEED0), 0, 8);
}

void _enable_encrypt(hdmi_tx_dev_t *dev, u8 enable)
{
	access_CoreWrite(dev, enable, (A_HDCPREG_RMLCTL), 0, 1);
}

void _store_encrypt_keys(hdmi_tx_dev_t *dev,u8 keys[560])
{
	int key_nr = 0;
	for (key_nr = 0; key_nr < 280; key_nr = key_nr + 7) {
		access_CoreWrite(dev, keys[key_nr + 0], (A_HDCPREG_DPK6), 0, 8);
		access_CoreWrite(dev, keys[key_nr + 1], (A_HDCPREG_DPK5), 0, 8);
		access_CoreWrite(dev, keys[key_nr + 2], (A_HDCPREG_DPK4), 0, 8);
		access_CoreWrite(dev, keys[key_nr + 3], (A_HDCPREG_DPK3), 0, 8);
		access_CoreWrite(dev, keys[key_nr + 4], (A_HDCPREG_DPK2), 0, 8);
		access_CoreWrite(dev, keys[key_nr + 5], (A_HDCPREG_DPK1), 0, 8);
		access_CoreWrite(dev, keys[key_nr + 6], (A_HDCPREG_DPK0), 0, 8);
		_wait_mem_access(dev);
	}
}
#endif

void hdcp_key_write(hdmi_tx_dev_t *dev)
{
	int i, j;
	u8 key[44][7] = {
		{0xDA, 0x0D, 0x4F, 0x5C, 0x2F, 0xA8, 0x95},
		{0x2B, 0x96, 0x43, 0x22, 0x18, 0xD5, 0xEB},
		{0x13, 0xEB, 0x8A, 0xFE, 0x88, 0x2D, 0x65},
		{0x47, 0xCD, 0xCD, 0xFF, 0xFF, 0x36, 0x6E},
		{0x7D, 0x08, 0x49, 0x9B, 0x51, 0x05, 0x02},
		{0x72, 0x64, 0x35, 0x24, 0xB6, 0x05, 0xC8},
		{0xDA, 0x67, 0xB0, 0x43, 0x0D, 0x56, 0xB4},
		{0x2D, 0x2D, 0xAC, 0x58, 0x79, 0x5D, 0xF3},
		{0x04, 0xEF, 0xFC, 0xF3, 0xA8, 0xFF, 0x39},
		{0x8A, 0x53, 0x95, 0x68, 0x17, 0x9C, 0xB2},
		{0x31, 0x36, 0x30, 0x90, 0xB7, 0xD9, 0x85},
		{0x3D, 0xC5, 0xCC, 0x25, 0x5A, 0xFB, 0x21},
		{0x44, 0x8C, 0xE9, 0x0A, 0x37, 0x5C, 0xD9},
		{0x4E, 0x37, 0xA2, 0x0D, 0xB1, 0x73, 0xD8},
		{0x8F, 0xD3, 0x9E, 0x08, 0x8D, 0xAF, 0x7F},
		{0x68, 0x4C, 0x91, 0x0D, 0xDE, 0x9D, 0x1F},
		{0xC5, 0xDE, 0x7B, 0x6A, 0x0E, 0xD3, 0x2C},
		{0xA3, 0x12, 0x01, 0x42, 0x36, 0x3F, 0x3F},
		{0x7C, 0x15, 0x47, 0xDA, 0x1F, 0x09, 0xD1},
		{0x54, 0xC4, 0x13, 0x3C, 0x55, 0xD9, 0x10},
		{0xEB, 0x5E, 0x36, 0xC8, 0x19, 0x41, 0x28},
		{0xEB, 0x54, 0x8E, 0x24, 0xED, 0xF6, 0xBF},
		{0x81, 0x4F, 0xAC, 0x81, 0xB9, 0xF0, 0xAE},
		{0xA6, 0xE0, 0x34, 0xBB, 0xCB, 0x1E, 0x2C},
		{0x6D, 0x39, 0xC5, 0x9F, 0x0A, 0x5F, 0x6C},
		{0x52, 0x2E, 0xEE, 0xB3, 0x6A, 0xFA, 0xEF},
		{0x15, 0xA3, 0x84, 0x76, 0xD0, 0xD0, 0xCD},
		{0x6E, 0x63, 0xC9, 0x46, 0xFB, 0x7D, 0xB6},
		{0xF0, 0xCE, 0xB8, 0x21, 0xB0, 0xC5, 0x64},
		{0xFF, 0x82, 0x79, 0x3D, 0xDD, 0xE5, 0x55},
		{0x87, 0xB5, 0x21, 0xFF, 0x7C, 0x58, 0xF1},
		{0x56, 0x35, 0x95, 0x9D, 0xC1, 0x93, 0xBF},
		{0x60, 0x4D, 0x5D, 0xF0, 0x73, 0x68, 0xE2},
		{0x92, 0xE6, 0x72, 0x4B, 0xCD, 0x99, 0xA2},
		{0x69, 0xA2, 0x55, 0x42, 0x34, 0x88, 0x92},
		{0xC0, 0xB7, 0xC8, 0xDE, 0xE8, 0x95, 0x53},
		{0x86, 0xA0, 0x03, 0xF0, 0x89, 0x94, 0x61},
		{0x67, 0x7E, 0xF1, 0xF4, 0xAA, 0x5F, 0xBC},
		{0xDF, 0x80, 0x92, 0x61, 0x71, 0x1C, 0x11},
		{0x7F, 0x1B, 0x94, 0xBA, 0x58, 0x7B, 0x5C},
		{0x9D, 0xEA, 0x6C, 0x6F, 0x80, 0x76, 0x40},
		{0x2E, 0x8C, 0xC3, 0x6B, 0xCC, 0x9B, 0xC5},
		{0x91, 0x2C, 0xB3, 0x20, 0xBE, 0xCE, 0x25},
		{0x00, 0x00, 0x00, 0x3D, 0x5D, 0x61, 0x8D}
	};

	dev_write(HDCPREG_RMLCTL , 0x0);  // disable rmctl
	udelay(5);

	while(!dev_read_mask(HDCPREG_RMLSTS, 0x40)); // poll hdcpreg_rmsts

	dev_write(HDCPREG_DPK6, 0x0);
	dev_write(HDCPREG_DPK5, 0x0);
	dev_write(HDCPREG_DPK4, 0x72);
	dev_write(HDCPREG_DPK3, 0x9E);
	dev_write(HDCPREG_DPK2, 0xA2);
	dev_write(HDCPREG_DPK1, 0xC2);
	dev_write(HDCPREG_DPK0, 0xDA);

	while(!dev_read_mask(HDCPREG_RMLSTS, 0x40)); // poll hdcpreg_rmsts

	dev_write( HDCPREG_RMLCTL , 0x1);   //enable rmctl

#if 0
	dev_write(dev, HDCPREG_SEED0 , 0x1F);
	dev_write(dev, HDCPREG_SEED1 , 0x1F);
#endif

	for(i = 0; i < 40; i++)
	{
		for(j = 0; j <= 6; j++)
		{
			pr_debug("key value:%u\n",key[i][6-j]);
			dev_write(HDCPREG_DPK6 - j * 4, (u8)key[i][6-j]);
			udelay(5);
		}
		udelay(5);

		while( !(dev_read_mask(HDCPREG_RMLSTS, 0x40)) ); // poll hdcpreg_rmsts

	}

	pr_debug("DPK write num:%u\n", dev_read_mask(HDCPREG_RMLSTS, 0x3f));

}

int hdcp_initialize(hdmi_tx_dev_t *dev)
{
	hdcp_rxdetect(dev, 0);
	_hdcp_data_enable_polarity(dev, dev->snps_hdmi_ctrl.data_enable_polarity);
	_disable_encryption(dev, 1);
	return 0;
}

void hdcp_1p4_configure(hdmi_tx_dev_t *dev, hdcp_params_t * hdcp)
{
	//_OverrideHDCP2p2Switch(dev, TRUE);

	/* HDCP only */
	_enable_feature11(dev,(hdcp->menable11_feature > 0) ? 1 : 0);
	_ri_check(dev,(hdcp->mricheck > 0) ? 1 : 0);
	_enable_i2c_fast_mode(dev,	(hdcp->mi2c_fastmode > 0) ? 1 : 0);
	_enhanced_link_verification(dev,(hdcp->menhanced_link_verification > 0) ? 1 : 0);

	/* fixed */
	_enable_avmute(dev, FALSE);
	_unencrypted_video_color(dev, 0x00);
	_encoding_packet_header(dev, TRUE);

	//9 - Set encryption
	_oess_window_size(dev, 64);
	_bypass_encryption(dev, FALSE);
	_disable_encryption(dev, FALSE);

	//10 - Reset the HDCP 1.4 engine
	hdcp_sw_reset(dev);

	//11 - Configure Device Private Keys (required when DWC_HDMI_HDCP_DPK_ROMLESS configuration
	//	parameter is set to True [1]), which is illustrated in Figure 3-7 on page 82. For required memory
	//	contents, refer to the “HDCP DPK 56-bit Memory Mapping” table in Chapter 2 of the DesignWare
	//	HDMI Transmitter Controller Databook.
	hdcp_key_write(dev);   // writr ksv
#if 0
#ifdef ROMLESS
	/* check if controller is version 1.4a or higher to support DPK keys external storage */
	if (_ControllerVersion(dev) >= 0x14) {
		hdcp_write_dpk_keys(dev, hdcp);
	}
#endif
#endif

	//12 - Enable encryption
	hdcp_rxdetect(dev, 1);

	pr_debug("HDCP enable interrupts");
	_interrupt_clear(dev, A_APIINTCLR_HDCP_FAILED_MASK |
						 A_APIINTCLR_HDCP_ENGAGED_MASK |
						 A_APIINTSTAT_KSVSHA1CALCDONEINT_MASK);
	/* enable KSV list SHA1 verification interrupt */
	_interrupt_mask(dev, (~(A_APIINTMSK_HDCP_FAILED_MASK |
					     A_APIINTMSK_HDCP_ENGAGED_MASK |
					     A_APIINTSTAT_KSVSHA1CALCDONEINT_MASK)) & _hdcp_interrupt_mask_status(dev));
}

int hdcp_configure(hdmi_tx_dev_t *dev, hdcp_params_t * hdcp, video_params_t *video)
{
	video_mode_t mode = dev->snps_hdmi_ctrl.hdmi_on;
	u8 hspol = video->mdtd.m_hsync_polarity;
	u8 vspol = video->mdtd.m_vsync_polarity;
	static int hdcp_2p2 = 0;

	if(dev->snps_hdmi_ctrl.hdcp_on == 0){
		pr_debug("HDCP is not active");
		return 0;
	}

	// Before configure HDCP we should configure the internal parameters
	hdcp->max_devices = 128;
	hdcp->mi2c_fastmode = 0;
	if(hdcp->mksvList_buffer == NULL)
		hdcp->mksvList_buffer = vmalloc(sizeof(u8) * 670);
	memcpy(&dev->hdcp, hdcp, sizeof(hdcp_params_t));

	//1 - To determine if the controller supports HDCP
	if(id_product_type(dev) != 0xC1){
		pr_err("Controller does not supports HDCP");
		return HDMI_ERR_HDCP_NOT_SUPORRT;
	}

	//2 - To determine the HDCP version of the transmitter
	if(id_hdcp22_support(dev) == HDCP_22_SNPS){
		pr_debug("HDCP 2.2 SNPS is present (both HDCP 1.4 and HDCP 2.2 versions supported)");
		hdcp_2p2 = 1;
	}
	else if(id_hdcp22_support(dev) == HDCP_22_EXT){
		pr_debug("HDCP 2.2 External is present (both HDCP 1.4 and HDCP 2.2 versions supported)");
		hdcp_2p2 = 2;
	}
	else{
		pr_debug("HDCP 2.2 is not present (HDCP 1.4 support only)");
		hdcp_2p2 = 0;
	}

	//3 - Select DVI or HDMI mode
	pr_debug("Set HDCP %s", mode == HDMI ? "HDMI" : "DVI");
	dev_write_mask(A_HDCPCFG0, A_HDCPCFG0_HDMIDVI_MASK, (mode == HDMI) ? 1 : 0);
	dev_write_mask(FC_INVIDCONF, FC_INVIDCONF_HDCP_KEEPOUT_MASK, 1);  //setting fc_invidconf.HDCP_keepout = 1
	//4 - Set the Data enable, Hsync, and VSync polarity
	_hsync_polarity(dev, (hspol > 0) ? 1 : 0);
	_vsync_polarity(dev, (vspol > 0) ? 1 : 0);
	_hdcp_data_enable_polarity(dev, (dev->snps_hdmi_ctrl.data_enable_polarity > 0) ? 1 : 0);

	//5 - If hdcp22_snps read in Step 2 is 0 (Synopsys HDCP 2.2 not supported), skip to Step 9.
	if((hdcp_2p2 == 0x00) || (hdcp_2p2 == 0x02) ){
		pr_debug("Configuring HDCP 1.4");
		hdcp_1p4_configure(dev, hdcp);
		return 0;
	}
	else{
		pr_debug("Configuring HDCP 2.2 SNPS");
		//6 - If hdcp22_snps read in Step 2 is 1 (Synopsys HDCP 2.2 is supported):
		//Read HDCP2Version through the HDCP 2.2 Synopsys RX Message FIFO and select HDCP version in
		//the Transmitter (refer to “Reading HDCP 2.2 Version (HDCP Port 0x50)” on page 83).

			//b. If HDCP2Version is 0x04, write “1” to mc_opctrl.h22s_ovr_val (this selects HDCP 2.2).
			dev_write_mask(0x1000C, 1 << 5, 0x1);
			//c. Write “1” to mc_opctrl.h22s_switch_lck.
			dev_write_mask(0x1000C, 1 << 4, 0x1);

			//7 - Reset the HDCP 2.2 engine
			_hdcp_2p2_reset_engine(dev);

			//8 - Perform the HDCP 2.2 authentication
			_hdcp_2p2_authentication(dev);
	}
	return 0;
}

// SHA-1 calculation by Software
u8 _read_ksv_list(hdmi_tx_dev_t *dev, int *param)
{
	int timeout = 1000;
	u16 bstatus = 0;
	u16 device_count = 0;
	int valid = HDCP_IDLE;
	int size = 0;
	int i = 0;

	u8 *hdcp_ksv_list_buffer = dev->hdcp.mksvList_buffer;

	// 1 - Wait for an interrupt to be triggered (a_apiintstat.KSVSha1calcint)
	// This is called from the INT_KSV_SHA1 irq so nothing is required for this step

	// 2 - Request access to KSV memory through setting a_ksvmemctrl.KSVMEMrequest to 1'b1 and
	// pool a_ksvmemctrl.KSVMEMaccess until this value is 1'b1 (access granted).
	_memory_access_request(dev,TRUE);
	while(_memory_access_granted(dev) == 0 && timeout--){
		asm volatile ("nop");
	}

	if (_memory_access_granted(dev) == 0){
		_memory_access_request(dev,FALSE);
		pr_err("KSV List memory access denied");
		*param = 0;
		return HDCP_KSV_LIST_ERR_MEM_ACCESS;
	}

	// 3 - Read VH', M0, Bstatus, and the KSV FIFO. The data is stored in the revocation memory, as
	// provided in the "Address Mapping for Maximum Memory Allocation" table in the databook.
	bstatus = _bstatus_read(dev);
	device_count = bstatus & BSTATUS_DEVICE_COUNT_MASK;

	if(device_count > dev->hdcp.max_devices) {
		*param = 0;
		pr_err("depth exceeds KSV List memory");
		return HDCP_KSV_LIST_ERR_DEPTH_EXCEEDED;
	}

	size = device_count * KSV_LEN + HEADER + SHAMAX;

	for (i = 0; i < size; i++){
		if (i < HEADER) { /* BSTATUS & M0 */
			hdcp_ksv_list_buffer[(device_count * KSV_LEN) + i] = (u8)dev_read(HDCP_BSTATUS + (i * ADDR_JUMP));
		}
		else if (i < (HEADER + (device_count * KSV_LEN))) { /* KSV list */
			hdcp_ksv_list_buffer[i - HEADER] = (u8)dev_read(HDCP_BSTATUS + (i * ADDR_JUMP));
		}
		else { /* SHA */
			hdcp_ksv_list_buffer[i] = (u8)dev_read(HDCP_BSTATUS + (i * ADDR_JUMP));
		}
	}

	// 4 - Calculate the SHA-1 checksum (VH) over M0, Bstatus, and the KSV FIFO.
	if(hdcp_verify_ksv(dev, hdcp_ksv_list_buffer, size) == TRUE){
		valid = HDCP_KSV_LIST_READY;
		pr_debug("HDCP_KSV_LIST_READY");
	}
	else{
		valid = HDCP_ERR_KSV_LIST_NOT_VALID;
		pr_err("HDCP_ERR_KSV_LIST_NOT_VALID");
	}

	// 5 - If the calculated VH equals the VH', set a_ksvmemctrl.SHA1fail to 0 and set
	// a_ksvmemctrl.KSVCTRLupd to 1. If the calculated VH is different from VH' then set
	// a_ksvmemctrl.SHA1fail to 1 and set a_ksvmemctrl.KSVCTRLupd to 1, forcing the controller
	// to re-authenticate from the beginning.
	_memory_access_request(dev,0);
	_update_ksv_list_state(dev,(valid == HDCP_KSV_LIST_READY) ? 0 : 1);

	return valid;
}

u8 hdcp_event_handler(hdmi_tx_dev_t *dev, int *param)
{
	u8 interrupt_status = 0;
	int valid = HDCP_IDLE;

	interrupt_status = hdcp_interrupt_status(dev);
	pr_debug("hdcp_interrupt_status %d", interrupt_status);

	if (interrupt_status == 0){
		pr_err("HDCP_IDLE\n");
		return HDCP_IDLE;
	}

	hdcp_interrupt_clear(dev, interrupt_status);

	if(interrupt_status & INT_KSV_SHA1){
		pr_debug("INT_KSV_SHA1\n");
		return _read_ksv_list(dev, param);
	}

	if ((interrupt_status & INT_HDCP_FAIL) != 0) {
		*param = 0;
		pr_err("HDCP_FAILED\n");
		return HDCP_FAILED;
	}

	if ((interrupt_status & INT_HDCP_ENGAGED) != 0) {
		*param = 1;
		pr_debug("HDCP_ENGAGED\n");
		return HDCP_ENGAGED;
	}

	return valid;
}

void hdcp_av_mute(hdmi_tx_dev_t *dev, int enable)
{
	_enable_avmute(dev,
			(enable == TRUE) ? 1 : 0);
}

void hdcp_disable_encryption(hdmi_tx_dev_t *dev, int disable)
{
	_disable_encryption(dev,	(disable == TRUE) ? 1 : 0);
}

u8 hdcp_interrupt_status(hdmi_tx_dev_t *dev)
{
	return _interrupt_status(dev);
}

int hdcp_interrupt_clear(hdmi_tx_dev_t *dev, u8 value)
{
	_interrupt_clear(dev, value);
	return TRUE;
}

#if 0
	#ifdef ROMLESS
	void hdcp_write_dpk_keys(hdmi_tx_dev_t *dev, hdcp_params_t * params)
	{
		_wait_mem_access(dev );
		_write_aksv(dev, params->maksv);
		_wait_mem_access(dev);
		_enable_encrypt(dev, 1);
		_write_seed(dev, params->msw_enckey);
		_store_encrypt_keys(dev, params->mkeys);
	}
	#endif
#endif

void sha_reset(hdmi_tx_dev_t *dev, sha_t * sha)
{
	size_t i = 0;
	sha->mindex = 0;
	sha->mcomputed = FALSE;
	sha->mcorrupted = FALSE;
	for (i = 0; i < sizeof(sha->mlength); i++) {
		sha->mlength[i] = 0;
	}
	sha->mdigest[0] = 0x67452301;
	sha->mdigest[1] = 0xEFCDAB89;
	sha->mdigest[2] = 0x98BADCFE;
	sha->mdigest[3] = 0x10325476;
	sha->mdigest[4] = 0xC3D2E1F0;
}

int sha_result(hdmi_tx_dev_t *dev, sha_t * sha)
{
	if (sha->mcorrupted == TRUE) {
		return FALSE;
	}
	if (sha->mcomputed == FALSE) {
		sha_pad_message(dev, sha);
		sha->mcomputed = TRUE;
	}
	return TRUE;
}

void sha_input(hdmi_tx_dev_t *dev, sha_t * sha, const u8 * data, size_t size)
{
	int i = 0;
	unsigned j = 0;
	int rc = TRUE;
	if (data == 0 || size == 0) {
		pr_err("invalid input data");
		return;
	}
	if (sha->mcomputed == TRUE || sha->mcorrupted == TRUE) {
		sha->mcorrupted = TRUE;
		return;
	}
	while (size-- && sha->mcorrupted == FALSE) {
		sha->mblock[sha->mindex++] = *data;

		for (i = 0; i < 8; i++) {
			rc = TRUE;
			for (j = 0; j < sizeof(sha->mlength); j++) {
				sha->mlength[j]++;
				if (sha->mlength[j] != 0) {
					rc = FALSE;
					break;
				}
			}
			sha->mcorrupted = (sha->mcorrupted == TRUE
					   || rc == TRUE) ? TRUE : FALSE;
		}
		/* if corrupted then message is too long */
		if (sha->mindex == 64) {
			sha_process_block(dev, sha);
		}
		data++;
	}
}

void sha_process_block(hdmi_tx_dev_t *dev, sha_t * sha)
{
#define shaCircularShift(bits,word) ((((word) << (bits)) & 0xFFFFFFFF) | ((word) >> (32-(bits))))

	const unsigned kk[] = {	/* constants defined in SHA-1 */
		0x5A827999, 0x6ED9EBA1, 0x8F1BBCDC, 0xCA62C1D6
	};
	unsigned ww[80];		/* word sequence */
	unsigned a, b, c, d, e;	/* word buffers */
	unsigned temp = 0;
	int t = 0;

	/* Initialize the first 16 words in the array W */
	for (t = 0; t < 80; t++) {
		if (t < 16) {
			ww[t] = ((unsigned)sha->mblock[t * 4 + 0]) << 24;
			ww[t] |= ((unsigned)sha->mblock[t * 4 + 1]) << 16;
			ww[t] |= ((unsigned)sha->mblock[t * 4 + 2]) << 8;
			ww[t] |= ((unsigned)sha->mblock[t * 4 + 3]) << 0;
		} else {
			ww[t] =
			    shaCircularShift(1,
					     ww[t - 3] ^ ww[t - 8] ^ ww[t -
								     14] ^ ww[t -
									     16]);
		}
	}

	a = sha->mdigest[0];
	b = sha->mdigest[1];
	c = sha->mdigest[2];
	d = sha->mdigest[3];
	e = sha->mdigest[4];

	for (t = 0; t < 80; t++) {
		temp = shaCircularShift(5, a);
		if (t < 20) {
			temp += ((b & c) | ((~b) & d)) + e + ww[t] + kk[0];
		} else if (t < 40) {
			temp += (b ^ c ^ d) + e + ww[t] + kk[1];
		} else if (t < 60) {
			temp += ((b & c) | (b & d) | (c & d)) + e + ww[t] + kk[2];
		} else {
			temp += (b ^ c ^ d) + e + ww[t] + kk[3];
		}
		e = d;
		d = c;
		c = shaCircularShift(30, b);
		b = a;
		a = (temp & 0xFFFFFFFF);
	}

	sha->mdigest[0] = (sha->mdigest[0] + a) & 0xFFFFFFFF;
	sha->mdigest[1] = (sha->mdigest[1] + b) & 0xFFFFFFFF;
	sha->mdigest[2] = (sha->mdigest[2] + c) & 0xFFFFFFFF;
	sha->mdigest[3] = (sha->mdigest[3] + d) & 0xFFFFFFFF;
	sha->mdigest[4] = (sha->mdigest[4] + e) & 0xFFFFFFFF;

	sha->mindex = 0;
}

void sha_pad_message(hdmi_tx_dev_t *dev, sha_t * sha)
{
	/*
	 *  Check to see if the current message block is too small to hold
	 *  the initial padding bits and length.  If so, we will pad the
	 *  block, process it, and then continue padding into a second
	 *  block.
	 */
	if (sha->mindex > 55) {
		sha->mblock[sha->mindex++] = 0x80;
		while (sha->mindex < 64) {
			sha->mblock[sha->mindex++] = 0;
		}
		sha_process_block(dev, sha);
		while (sha->mindex < 56) {
			sha->mblock[sha->mindex++] = 0;
		}
	} else {
		sha->mblock[sha->mindex++] = 0x80;
		while (sha->mindex < 56) {
			sha->mblock[sha->mindex++] = 0;
		}
	}

	/* Store the message length as the last 8 octets */
	sha->mblock[56] = sha->mlength[7];
	sha->mblock[57] = sha->mlength[6];
	sha->mblock[58] = sha->mlength[5];
	sha->mblock[59] = sha->mlength[4];
	sha->mblock[60] = sha->mlength[3];
	sha->mblock[61] = sha->mlength[2];
	sha->mblock[62] = sha->mlength[1];
	sha->mblock[63] = sha->mlength[0];

	sha_process_block(dev, sha);
}

int hdcp_array_add(hdmi_tx_dev_t *dev, u8 * r, const u8 * a, const u8 * b, size_t n)
{
	u8 c = 0;
	size_t i = 0;
	for (i = 0; i < n; i++) {
		u16 s = a[i] + b[i] + c;
		c = (u8) (s >> 8);
		r[i] = (u8) s;
	}
	return c;
}

int hdcp_array_cmp(hdmi_tx_dev_t *dev, const u8 * a, const u8 * b, size_t n)
{
	int i = 0;
	for (i = n; i > 0; i--) {
		if (a[i - 1] > b[i - 1]) {
			return 1;
		} else if (a[i - 1] < b[i - 1]) {
			return -1;
		}
	}
	return 0;
}

void hdcp_array_cpy(hdmi_tx_dev_t *dev, u8 * dst, const u8 * src, size_t n)
{
	size_t i = 0;
	for (i = 0; i < n; i++) {
		dst[i] = src[i];
	}
}

int hdcp_array_mac(hdmi_tx_dev_t *dev, u8 * r, const u8 * mm, const u8 m, size_t n)
{
	u16 c = 0;
	size_t i = 0;
	for (i = 0; i < n; i++) {
		u16 p = (mm[i] * m) + c + r[i];
		c = p >> 8;
		r[i] = (u8) p;
	}
	return (u8) c;
}

int hdcp_array_mul(hdmi_tx_dev_t *dev, u8 * r, const u8 * mm, const u8 * m, size_t n)
{
	size_t i = 0;
	if (r == mm || r == m) {
		pr_err("invalid input data");
		return FALSE;
	}
	hdcp_array_set(dev, r, 0, n);
	for (i = 0; i < n; i++) {
		if (m[i] == 0) {
			continue;
		} else if (m[i] == 1) {
			hdcp_array_add(dev, &r[i], &r[i], mm, n - i);
		} else {
			hdcp_array_mac(dev, &r[i], mm, m[i], n - i);
		}
	}
	return TRUE;
}

void hdcp_array_set(hdmi_tx_dev_t *dev, u8 * dst, const u8 src, size_t n)
{
	size_t i = 0;
	for (i = 0; i < n; i++) {
		dst[i] = src;
	}
}

int hdcp_array_usb(hdmi_tx_dev_t *dev, u8 * r, const u8 * a, const u8 * b, size_t n)
{
	u8 c = 1;
	size_t i = 0;
	for (i = 0; i < n; i++) {
		u16 s = ((u8) a[i] + (u8) (~b[i])) + c;
		c = (u8) (s >> 8);
		r[i] = (u8) s;
	}
	return c;
}

void hdcp_array_swp(hdmi_tx_dev_t *dev, u8 * r, size_t n)
{
	size_t i = 0;
	for (i = 0; i < (n / 2); i++) {
		u8 tmp = r[i];
		r[i] = r[n - 1 - i];
		r[n - 1 - i] = tmp;
	}
}

int hdcp_array_tst(hdmi_tx_dev_t *dev, const u8 * a, const u8 b, size_t n)
{
	size_t i = 0;
	for (i = 0; i < n; i++) {
		if (a[i] != b) {
			return FALSE;
		}
	}
	return TRUE;
}

int hdcp_array_div(hdmi_tx_dev_t *dev, u8 * r, const u8 * dd, const u8 * d, size_t n)
{
	int i = 0;
	if (r == dd || r == d || (TRUE == !hdcp_array_tst(dev, d, 0, n))) {
		pr_err("invalid input data");
		return FALSE;
	}
	hdcp_array_set(dev, &r[n], 0, n);
	hdcp_array_cpy(dev, r, dd, n);
	for (i = n; i > 0; i--) {
		r[i - 1 + n] = 0;
		while (hdcp_array_cmp(dev, &r[i - 1], d, n) >= 0) {
			hdcp_array_usb(dev, &r[i - 1], &r[i - 1], d, n);
			r[i - 1 + n] += 1;
		}
	}
	return TRUE;
}

int hdcp_compute_exp(hdmi_tx_dev_t *dev, u8 * c, const u8 * m, const u8 * e, const u8 * p,
			  size_t n, size_t ne)
{
	int i = 8 * ne - 1;
	int rc = TRUE;

	/* LR Binary Method */
	if ((e[i / 8] & (1 << (i % 8))) != 0) {
		hdcp_array_cpy(dev, c, m, n);
	} else {
		hdcp_array_set(dev, c, 0, n);
		c[0] = 1;
	}
	for (i -= 1; i >= 0; i--) {
		rc |= hdcp_compute_mul(dev, c, c, c, p, n);
		if ((e[i / 8] & (1 << (i % 8))) != 0) {
			rc &= hdcp_compute_mul(dev, c, c, m, p, n);
		}
	}
	return rc;
}

int hdcp_compute_inv(hdmi_tx_dev_t *dev, u8 * out, const u8 * z, const u8 * a, size_t n)
{
	u8 w[2][SIZE];
	u8 x[2][SIZE];
	u8 y[2][SIZE];
	u8 r[2 * SIZE];
	u8 *i, *j, *q, *t;
	u8 *x1, *x2;
	u8 *y1, *y2;

	if ((n > SIZE) || (hdcp_array_tst(dev, z, 0, n) == TRUE)
	    || (hdcp_array_tst(dev, a, 0, n) == TRUE)
	    || (hdcp_array_cmp(dev, z, a, n) >= 0)) {
		pr_err("invalid input data");
		return FALSE;
	}

	hdcp_array_cpy(dev, w[0], a, n);
	hdcp_array_cpy(dev, w[1], z, n);
	i = w[0];
	j = w[1];

	hdcp_array_set(dev, x[1], 0, n);
	x[1][0] = 1;
	hdcp_array_set(dev, x[0], 0, n);
	x2 = x[1];
	x1 = x[0];

	hdcp_array_set(dev, y[1], 0, n);
	hdcp_array_set(dev, y[0], 0, n);
	y[0][0] = 1;
	y2 = y[1];
	y1 = y[0];

	do {
		hdcp_array_div(dev, r, i, j, n);
		hdcp_array_cpy(dev, i, r, n);
		q = &r[n];
		t = i;		/* swap i <-> j */
		i = j;
		j = t;

		hdcp_array_mul(dev, r, x1, q, n);
		hdcp_array_usb(dev, x2, x2, r, n);
		t = x2;		/* swap x1 <-> x2 */
		x2 = x1;
		x1 = t;

		hdcp_array_mul(dev, r, y1, q, n);
		hdcp_array_usb(dev, y2, y2, r, n);
		t = y2;		/* swap y1 <-> y2 */
		y2 = y1;
		y1 = t;

	} while (hdcp_array_tst(dev, j, 0, n) == FALSE);

	j[0] = 1;
	if (hdcp_array_cmp(dev, i, j, n) != 0) {
		pr_err("i != 1");
		return FALSE;
	}
	hdcp_array_cpy(dev, out, y2, n);
	return TRUE;
}

int hdcp_compute_mod(hdmi_tx_dev_t *dev, u8 * dst, const u8 * src, const u8 * p, size_t n)
{
	u8 aux[KSIZE];
	u8 ext[SIZE + 1];
	u8 tmp[2 * (KSIZE + 1)];
	int i = 0;

	if (n > SIZE) {
		pr_err("invalid input data");
		return FALSE;
	}
	hdcp_array_cpy(dev, aux, src, sizeof(aux));
	hdcp_array_cpy(dev, ext, p, n);
	ext[n] = 0;
	for (i = sizeof(aux) - n - 1; i >= 0; i--) {
		hdcp_array_div(dev, tmp, &aux[i], ext, n + 1);
		hdcp_array_cpy(dev, &aux[i], tmp, n + 1);
	}
	hdcp_array_cpy(dev, dst, aux, n);
	return TRUE;
}

int hdcp_compute_mul(hdmi_tx_dev_t *dev, u8 * p, const u8 * a, const u8 * b, const u8 * m,
			  size_t n)
{
	u8 aux[2 * KSIZE + 1];
	u8 ext[KSIZE + 1];
	u8 tmp[2 * (KSIZE + 1)];
	size_t i = 0;
	int j = 0;
	if (n > KSIZE) {
		pr_err("invalid input data");
		return FALSE;
	}
	hdcp_array_set(dev, aux, 0, sizeof(aux));
	for (i = 0; i < n; i++) {
		aux[n + i] = hdcp_array_mac(dev, &aux[i], a, b[i], n);
	}
	hdcp_array_cpy(dev, ext, m, n);
	ext[n] = 0;
	for (j = n; j >= 0; j--) {
		hdcp_array_div(dev, tmp, &aux[j], ext, n + 1);
		hdcp_array_cpy(dev, &aux[j], tmp, n + 1);
	}
	hdcp_array_cpy(dev, p, aux, n);
	return TRUE;
}

int hdcp_verify_ksv(hdmi_tx_dev_t *dev, const u8 * data, size_t size)
{
	size_t i = 0;
	sha_t sha;

	if (data == 0 || size < (HEADER + SHAMAX)) {
		pr_err("invalid input data");
		return FALSE;
	}
	sha_reset(dev, &sha);
	sha_input(dev, &sha, data, size - SHAMAX);

	if (sha_result(dev, &sha) == FALSE) {
		pr_err("cannot process SHA digest");
		return FALSE;
	}

	for (i = 0; i < SHAMAX; i++) {
		if (data[size - SHAMAX + i] != (u8) (sha.mdigest[i / 4] >> ((i % 4) * 8))) {
			pr_err("SHA digest does not match");
			return FALSE;
		}
	}
	return TRUE;
}

int hdcp_verify_srm(hdmi_tx_dev_t *dev, const u8 * data, size_t size)
{
	if (data == 0 || size < (VRL_HEADER + VRL_NUMBER + 2 * DSAMAX)) {
		pr_err("invalid input data");
		return FALSE;
	}
	/* M, n, r, s */
	return hdcp_verify_dsa(dev, data, size - 2 * DSAMAX, &data[size - 2 * DSAMAX],
			      &data[size - DSAMAX]);
}

int hdcp_verify_dsa(hdmi_tx_dev_t *dev, const u8 * m, size_t n, const u8 * r, const u8 * s)
{
	int i = 0;
	sha_t sha;
	static const u8 q[] = {
			0xE7, 0x08, 0xC7, 0xF9, 0x4D, 0x3F, 0xEF, 0x97, 0xE2, 0x14, 0x6D,
			0xCD, 0x6A, 0xB5, 0x6D, 0x5E, 0xCE, 0xF2, 0x8A, 0xEE
	};
	static const u8 p[] = {
			0x27, 0x75, 0x28, 0xF3, 0x2B, 0x80, 0x59, 0x8C, 0x11, 0xC2, 0xED,
			0x46, 0x1C, 0x95, 0x39, 0x2A, 0x54, 0x19, 0x89, 0x96, 0xFD, 0x49,
			0x8A, 0x02, 0x3B, 0x73, 0x75, 0x32, 0x14, 0x9C, 0x7B, 0x5C, 0x49,
			0x20, 0x98, 0xB9, 0x07, 0x32, 0x3F, 0xA7, 0x30, 0x15, 0x72, 0xB3,
			0x09, 0x55, 0x71, 0x10, 0x3A, 0x4C, 0x97, 0xD1, 0xBC, 0xA0, 0x04,
			0xF4, 0x35, 0xCF, 0x47, 0x54, 0x0E, 0xA7, 0x2B, 0xE5, 0x83, 0xB9,
			0xC6, 0xD4, 0x47, 0xC7, 0x44, 0xB8, 0x67, 0x76, 0x7C, 0xAE, 0x0C,
			0xDC, 0x34, 0x4F, 0x4B, 0x9E, 0x96, 0x1D, 0x82, 0x84, 0xD2, 0xA0,
			0xDC, 0xE0, 0x00, 0xF5, 0x64, 0xA1, 0x7F, 0x8E, 0xFF, 0x58, 0x70,
			0x6A, 0xC3, 0x4F, 0xA2, 0xA1, 0xB8, 0xC7, 0x52, 0x5A, 0x35, 0x5B,
			0x39, 0x17, 0x6B, 0x78, 0x43, 0x93, 0xF7, 0x75, 0x8D, 0x01, 0xB7,
			0x61, 0x17, 0xFD, 0xB2, 0xF5, 0xC3, 0xD3
	};
	static const u8 g[] = {
			0xD9, 0x0B, 0xBA, 0xC2, 0x42, 0x24, 0x46, 0x69, 0x5B, 0x40, 0x67,
			0x2F, 0x5B, 0x18, 0x3F, 0xB9, 0xE8, 0x6F, 0x21, 0x29, 0xAC, 0x7D,
			0xFA, 0x51, 0xC2, 0x9D, 0x4A, 0xAB, 0x8A, 0x9B, 0x8E, 0xC9, 0x42,
			0x42, 0xA5, 0x1D, 0xB2, 0x69, 0xAB, 0xC8, 0xE3, 0xA5, 0xC8, 0x81,
			0xBE, 0xB6, 0xA0, 0xB1, 0x7F, 0xBA, 0x21, 0x2C, 0x64, 0x35, 0xC8,
			0xF7, 0x5F, 0x58, 0x78, 0xF7, 0x45, 0x29, 0xDD, 0x92, 0x9E, 0x79,
			0x3D, 0xA0, 0x0C, 0xCD, 0x29, 0x0E, 0xA9, 0xE1, 0x37, 0xEB, 0xBF,
			0xC6, 0xED, 0x8E, 0xA8, 0xFF, 0x3E, 0xA8, 0x7D, 0x97, 0x62, 0x51,
			0xD2, 0xA9, 0xEC, 0xBD, 0x4A, 0xB1, 0x5D, 0x8F, 0x11, 0x86, 0x27,
			0xCD, 0x66, 0xD7, 0x56, 0x5D, 0x31, 0xD7, 0xBE, 0xA9, 0xAC, 0xDE,
			0xAF, 0x02, 0xB5, 0x1A, 0xDE, 0x45, 0x24, 0x3E, 0xE4, 0x1A, 0x13,
			0x52, 0x4D, 0x6A, 0x1B, 0x5D, 0xF8, 0x92
	};
#ifndef FACSIMILE
	static const u8 y[] = {
			0x99, 0x37, 0xE5, 0x36, 0xFA, 0xF7, 0xA9, 0x62, 0x83, 0xFB, 0xB3,
			0xE9, 0xF7, 0x9D, 0x8F, 0xD8, 0xCB, 0x62, 0xF6, 0x66, 0x8D, 0xDC,
			0xC8, 0x95, 0x10, 0x24, 0x6C, 0x88, 0xBD, 0xFF, 0xB7, 0x7B, 0xE2,
			0x06, 0x52, 0xFD, 0xF7, 0x5F, 0x43, 0x62, 0xE6, 0x53, 0x65, 0xB1,
			0x38, 0x90, 0x25, 0x87, 0x8D, 0xA4, 0x9E, 0xFE, 0x56, 0x08, 0xA7,
			0xA2, 0x0D, 0x4E, 0xD8, 0x43, 0x3C, 0x97, 0xBA, 0x27, 0x6C, 0x56,
			0xC4, 0x17, 0xA4, 0xB2, 0x5C, 0x8D, 0xDB, 0x04, 0x17, 0x03, 0x4F,
			0xE1, 0x22, 0xDB, 0x74, 0x18, 0x54, 0x1B, 0xDE, 0x04, 0x68, 0xE1,
			0xBD, 0x0B, 0x4F, 0x65, 0x48, 0x0E, 0x95, 0x56, 0x8D, 0xA7, 0x5B,
			0xF1, 0x55, 0x47, 0x65, 0xE7, 0xA8, 0x54, 0x17, 0x8A, 0x65, 0x76,
			0x0D, 0x4F, 0x0D, 0xFF, 0xAC, 0xA3, 0xE0, 0xFB, 0x80, 0x3A, 0x86,
			0xB0, 0xA0, 0x6B, 0x52, 0x00, 0x06, 0xC7
	};
#else
	static const u8 y[] = {
		0x46, 0xB9, 0xC2, 0xE5, 0xBE, 0x57, 0x3B, 0xA6,
		0x22, 0x7B, 0xAA, 0x83, 0x81, 0xA9, 0xD2, 0x0F,
		0x03, 0x2E, 0x0B, 0x70, 0xAC, 0x96, 0x42, 0x85,
		0x4E, 0x78, 0x8A, 0xDF, 0x65, 0x35, 0x97, 0x6D,
		0xE1, 0x8D, 0xD1, 0x7E, 0xA3, 0x83, 0xCA, 0x0F,
		0xB5, 0x8E, 0xA4, 0x11, 0xFA, 0x14, 0x6D, 0xB1,
		0x0A, 0xCC, 0x5D, 0xFF, 0xC0, 0x8C, 0xD8, 0xB1,
		0xE6, 0x95, 0x72, 0x2E, 0xBD, 0x7C, 0x85, 0xDE,
		0xE8, 0x52, 0x69, 0x92, 0xA0, 0x22, 0xF7, 0x01,
		0xCD, 0x79, 0xAF, 0x94, 0x83, 0x2E, 0x01, 0x1C,
		0xD7, 0xEF, 0x86, 0x97, 0xA3, 0xBB, 0xCB, 0x64,
		0xA6, 0xC7, 0x08, 0x5E, 0x8E, 0x5F, 0x11, 0x0B,
		0xC0, 0xE8, 0xD8, 0xDE, 0x47, 0x2E, 0x75, 0xC7,
		0xAA, 0x8C, 0xDC, 0xB7, 0x02, 0xC4, 0xDF, 0x95,
		0x31, 0x74, 0xB0, 0x3E, 0xEB, 0x95, 0xDB, 0xB0,
		0xCE, 0x11, 0x0E, 0x34, 0x9F, 0xE1, 0x13, 0x8D
	};
#endif

	u8 w[SIZE];
	u8 z[SIZE];
	u8 u1[SIZE];
	u8 u2[SIZE];
	u8 gu1[KSIZE];
	u8 yu2[KSIZE];
	u8 pro[KSIZE];
	u8 v[SIZE];

	/* adapt to the expected format by aritmetic functions */
	u8 r1[SIZE];
	u8 s1[SIZE];
	sha_reset(dev, &sha);
	hdcp_array_cpy(dev, r1, r, sizeof(r1));
	hdcp_array_cpy(dev, s1, s, sizeof(s1));
	hdcp_array_swp(dev, r1, sizeof(r1));
	hdcp_array_swp(dev, s1, sizeof(s1));

	hdcp_compute_inv(dev, w, s1, q, sizeof(w));
	sha_input(dev, &sha, m, n);
	if (sha_result(dev, &sha) == TRUE) {
		for (i = 0; i < 5; i++) {
			z[i * 4 + 0] = sha.mdigest[i] >> 24;
			z[i * 4 + 1] = sha.mdigest[i] >> 16;
			z[i * 4 + 2] = sha.mdigest[i] >> 8;
			z[i * 4 + 3] = sha.mdigest[i] >> 0;
		}
		hdcp_array_swp(dev, z, sizeof(z));
	} else {
		pr_err("cannot digest message");
		return FALSE;
	}
	if (hdcp_compute_mul(dev, u1, z, w, q, sizeof(u1)) == FALSE) {
		return FALSE;
	}
	if (hdcp_compute_mul(dev, u2, r1, w, q, sizeof(u2)) == FALSE) {
		return FALSE;
	}
	if (hdcp_compute_exp(dev, gu1, g, u1, p, sizeof(gu1), sizeof(u1)) ==
	    FALSE) {
		return FALSE;
	}
	if (hdcp_compute_exp(dev, yu2, y, u2, p, sizeof(yu2), sizeof(u2)) ==
	    FALSE) {
		return FALSE;
	}
	if (hdcp_compute_mul(dev, pro, gu1, yu2, p, sizeof(pro)) == FALSE) {
		return FALSE;
	}
	if (hdcp_compute_mod(dev, v, pro, q, sizeof(v)) == FALSE) {
		return FALSE;
	}
	return (hdcp_array_cmp(dev, v, r1, sizeof(v)) == 0);
}

void hdcp_params_reset(hdmi_tx_dev_t *dev, hdcp_params_t * params)
{
	params->bypass = TRUE;
	params->menable11_feature = 0;
	params->mricheck = 1;
	params->mi2c_fastmode = 0;
	params->menhanced_link_verification = 0;
	params->max_devices = 0;

	if(params->mksvList_buffer != NULL)
		vfree(params->mksvList_buffer);
	params->mksvList_buffer = NULL;
}