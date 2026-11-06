#include "phy/phy.h"
#include "phy/phy_i2c.h"
#include "core/main_controller.h"
#include "core/hdmi_core.h"
#include "bsp/access.h"
#include "util/util.h"
#include "core/hdmi_reg.h"
#include "phy.h"
#include "scdc/scdc.h"

#define OPMODE_PLLCFG	0x06 // Mode of Operation and PLL  Dividers Control Register
#define PLLCURRCTRL		0x10 // PLL Current Control Register
#define PLLDIVCTRL		0x11 // PLL Dividers Control Register
#define TXTERM			0x19 // Transmission Termination Register
#define VLEVCTRL		0x0E // Voltage Level Control Register
#define CKSYMTXCTRL		0x09 // Clock Symbol and Transmitter Control Register

#define LT_1_65GBPS_TXTERM 		0x0007
#define LT_1_65GBPS_VLEVCTRL 	0x01A0
#define LT_1_65GBPS_CKSYMTXCTRL 0x8088

#define LT_3_40GBPS_TXTERM 		0x0000
#define LT_3_40GBPS_VLEVCTRL 	0x0120
#define LT_3_40GBPS_CKSYMTXCTRL 0x83F8

#define GT_3_40GBPS_TXTERM 		0x0000
#define GT_3_40GBPS_VLEVCTRL 	0x0140
#define GT_3_40GBPS_CKSYMTXCTRL 0x80F6

#define LT_1_65GBPS LT_1_65GBPS_TXTERM, LT_1_65GBPS_VLEVCTRL, LT_1_65GBPS_CKSYMTXCTRL
#define LT_3_40GBPS LT_3_40GBPS_TXTERM, LT_3_40GBPS_VLEVCTRL, LT_3_40GBPS_CKSYMTXCTRL
#define GT_3_40GBPS GT_3_40GBPS_TXTERM, GT_3_40GBPS_VLEVCTRL, GT_3_40GBPS_CKSYMTXCTRL

//rterm		0x19	d_tx_term[2:0]	TXTERM
//txlvl		0x0E	sup_tx_lvl[4:0] TXLVL
//cksymon	0x09	ck_symon[3:0] 	SYMON
//traon		0x09 	tx_traon 		TRAON
//trbon		0x09	tx_trbon		TRBON

//	Data rate		rterm	txlvl/cklvl	cksymon		txsymon		pre-empth	slopeboost
//					0x19	0x0E		0x09		0x09		0x09		0x09
//																traon/trbon
//	<=1.65			3'b100	5'b01111	4'b1000		4'b1100		1'b0/1'b0	4'b0000
//	>1.65 & < 3.4	3'b100	5'b01100	4'b1000		4'b1100		1'b1/1'b1	4'b0000
//	>3.4			3'b000	5'b01001	4'b0101		4'b1111 	1'b0/1'b0	4'b0000


static struct phy_config phy316[] = {
	{0, 0, 8, HDMI_14, 0x0003, 0x0283, 0x0628, LT_1_65GBPS},
	{0, 0, 8, HDMI_14, 0x0003, 0x0285, 0x0228, LT_1_65GBPS},
	{0, 0, 8, HDMI_14, 0x0002, 0x1183, 0x0614, LT_1_65GBPS},
	{0, 0, 8, HDMI_14, 0x0002, 0x1142, 0x0214, LT_1_65GBPS},
	{0, 0, 8, HDMI_14, 0x0001, 0x20C0, 0x060A, LT_1_65GBPS},
	{0, 0, 8, HDMI_14, 0x0001, 0x2080, 0x020A, LT_1_65GBPS},
	{0, 0, 8, HDMI_14, 0x0001, 0x2080, 0x020A, LT_3_40GBPS},
	{0, 0, 8, HDMI_14, 0x0000, 0x3040, 0x0605, LT_3_40GBPS},
	{0, 0, 8, HDMI_14, 0x0000, 0x3041, 0x0205, LT_3_40GBPS},
	{0, 0, 8, HDMI_20, 0x0640, 0x3041, 0x0205, GT_3_40GBPS},
	{0, 0, 8, HDMI_20, 0x0640, 0x3080, 0x0005, GT_3_40GBPS},
	{0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
};

static struct phy_config phy316_pr[] = {
	{13500,  1, 8, HDMI_14, 0x0003, 0x0280, 0x0650, LT_1_65GBPS},
	{13500,  3, 8, HDMI_14, 0x0002, 0x1280, 0x0650, LT_1_65GBPS},
	{13500,  7, 8, HDMI_14, 0x0001, 0x2280, 0x0650, LT_1_65GBPS},
	{18000,  2, 8, HDMI_14, 0x0002, 0x1280, 0x063C, LT_1_65GBPS},
	{18000,  5, 8, HDMI_14, 0x0001, 0x2280, 0x063C, LT_1_65GBPS},
	{21600,  4, 8, HDMI_14, 0x0001, 0x2281, 0x0632, LT_1_65GBPS},
	{21600,  9, 8, HDMI_14, 0x0000, 0x3281, 0x0632, LT_3_40GBPS},
	{24000,  8, 8, HDMI_14, 0x0000, 0x3282, 0x062D, LT_3_40GBPS},
	{27000,  1, 8, HDMI_14, 0x0002, 0x1283, 0x0628, LT_1_65GBPS},
	{27000,  3, 8, HDMI_14, 0x0001, 0x2283, 0x0628, LT_1_65GBPS},
	{27000,  7, 8, HDMI_14, 0x0000, 0x3283, 0x0628, LT_3_40GBPS},
	{36000,  2, 8, HDMI_14, 0x0001, 0x2285, 0x061E, LT_1_65GBPS},
	{36000,  5, 8, HDMI_14, 0x0000, 0x3285, 0x061E, LT_3_40GBPS},
	{43200,  4, 8, HDMI_14, 0x0000, 0x3203, 0x0619, LT_3_40GBPS},
	{54000,  1, 8, HDMI_14, 0x0001, 0x2183, 0x0614, LT_1_65GBPS},
	{54000,  3, 8, HDMI_14, 0x0000, 0x3183, 0x0614, LT_3_40GBPS},
	{72000,  2, 8, HDMI_14, 0x0000, 0x3141, 0x060F, LT_3_40GBPS},
	{108000, 1, 8, HDMI_14, 0x0000, 0x30C0, 0x060A, LT_3_40GBPS},
	{108000, 2, 8, HDMI_20, 0x0640, 0x3142, 0x0214, LT_3_40GBPS},
	{0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
};

void _power_down(hdmi_tx_dev_t *dev, u8 bit)
{
	//TODO: Correct register mask - extract the information from IP-XACT
	dev_write_mask(PHY_CONF0, PHY_CONF0_SPARES_2_MASK, (bit ? 1 : 0));
	dev->snps_hdmi_ctrl.phy_power = bit ? TRUE : FALSE;
}

void _enable_tmds(hdmi_tx_dev_t *dev, u8 bit)
{
	//TODO: Correct register mask - extract the information from IP-XACT
	dev_write_mask(PHY_CONF0, PHY_CONF0_SPARES_1_MASK, (bit ? 1 : 0));
}

void _set_pddq(hdmi_tx_dev_t *dev, u8 bit)
{
	dev_write_mask(PHY_CONF0, PHY_CONF0_PDDQ_MASK, (bit ? 1 : 0));
}

void _tx_power_on(hdmi_tx_dev_t *dev, u8 bit)
{
	dev_write_mask(PHY_CONF0, PHY_CONF0_TXPWRON_MASK, (bit ? 1 : 0));
	dev->snps_hdmi_ctrl.phy_power = bit ? TRUE : FALSE;
}

void phy_enable_hpd_sense(hdmi_tx_dev_t *dev, u8 bit)
{
	dev_write_mask(PHY_CONF0, PHY_CONF0_ENHPDRXSENSE_MASK, (bit ? 1 : 0));
}
EXPORT_SYMBOL_GPL(phy_enable_hpd_sense);

void _data_enable_polarity(hdmi_tx_dev_t *dev, u8 bit)
{
	dev_write_mask(PHY_CONF0, PHY_CONF0_SELDATAENPOL_MASK, (bit ? 1 : 0));
}

void _interface_control(hdmi_tx_dev_t *dev, u8 bit)
{
	dev_write_mask(PHY_CONF0, PHY_CONF0_SELDIPIF_MASK, (bit ? 1 : 0));
}

void _test_clear(hdmi_tx_dev_t *dev, u8 bit)
{
	dev_write_mask(PHY_TST0, PHY_TST0_SPARE_4_MASK, (bit ? 1 : 0));
}

void _test_enable(hdmi_tx_dev_t *dev, u8 bit)
{
	dev_write_mask(PHY_TST0, PHY_TST0_SPARE_3_MASK, (bit ? 1 : 0));
}

void _test_clock(hdmi_tx_dev_t *dev, u8 bit)
{
	dev_write_mask(PHY_TST0, PHY_TST0_SPARE_0_MASK, (bit ? 1 : 0));
}

void _test_data_in(hdmi_tx_dev_t *dev, u8 data)
{
	dev_write((PHY_TST1), data);
}

u8 _test_data_out(hdmi_tx_dev_t *dev, u32 baseAddr)
{
	return dev_read(PHY_TST2);
}

u8 _interrupt_state(hdmi_tx_dev_t *dev)
{
	return dev_read(PHY_INT0);
}

u8 _interrupt_mask_status(hdmi_tx_dev_t *dev, u8 mask)
{
	return dev_read(PHY_MASK0) & mask;
}

void _interrupt_polarity(hdmi_tx_dev_t *dev, u8 bitShift, u8 value)
{
	dev_write_mask(PHY_POL0, (1 << bitShift), value);
}

u8 _interrupt_polarity_status(hdmi_tx_dev_t *dev, u8 mask)
{
	return dev_read(PHY_POL0) & mask;
}

/*************************************************************
 * External functions
 *************************************************************/

int phy_write(hdmi_tx_dev_t *dev, u16 addr, u32 data)
{
	return phy_i2c_write(dev, addr, (u16) data);
}

int phy_read(hdmi_tx_dev_t *dev, u16 addr, u32 * value)
{
	return phy_i2c_read(dev, addr, (u16 *)value);
}

int phy_set_interface(hdmi_tx_dev_t *dev, phy_access_t interface)
{

	dev_write(JTAG_PHY_CONFIG, JTAG_PHY_CONFIG_I2C_JTAGZ_MASK);
	phy_slave_address(dev,PHY_I2C_SLAVE_ADDR);

	return 0;
}

int phy_slave_address(hdmi_tx_dev_t *dev, u8 value)
{
	phy_i2c_slave_address(dev, value);
	return 0;

}

int phy_initialize(hdmi_tx_dev_t *dev)
{
	phy_set_interface(dev, PHY_I2C);
	_tx_power_on(dev, 0);
	_set_pddq(dev, 1);

	phy_interrupt_mask(dev, PHY_MASK0_TX_PHY_LOCK_MASK |
				PHY_MASK0_RX_SENSE_0_MASK |
				PHY_MASK0_RX_SENSE_1_MASK |
				PHY_MASK0_RX_SENSE_2_MASK |
				PHY_MASK0_RX_SENSE_3_MASK);
	_data_enable_polarity(dev, dev->snps_hdmi_ctrl.data_enable_polarity);
	_interface_control(dev, 0);
	_enable_tmds(dev, 0);
	_power_down(dev, 0);	/* disable PHY */
	phy_i2c_mask_interrupts(dev, 0);

	// Clean IH_I2CMPHY_STAT0
	dev_write_mask(IH_I2CMPHY_STAT0, IH_I2CMPHY_STAT0_I2CMPHYERROR_MASK | IH_I2CMPHY_STAT0_I2CMPHYDONE_MASK, 0);

	return TRUE;
}

int phy_configure(hdmi_tx_dev_t *dev)
{
	return phy316_configure(dev, dev->snps_hdmi_ctrl.pixel_clock,
								dev->snps_hdmi_ctrl.color_resolution,
								dev->snps_hdmi_ctrl.pixel_repetition);
}

int phy_standby(hdmi_tx_dev_t *dev)
{
	phy_interrupt_mask(dev, PHY_MASK0_TX_PHY_LOCK_MASK |
				PHY_MASK0_RX_SENSE_0_MASK |
				PHY_MASK0_RX_SENSE_1_MASK |
				PHY_MASK0_RX_SENSE_2_MASK |
				PHY_MASK0_RX_SENSE_3_MASK);	/* mask phy interrupts - leave HPD */
	_enable_tmds(dev, 0);
	_power_down(dev, 0);	/*  disable PHY */
	_tx_power_on(dev, 0);
	_set_pddq(dev, 1);

	return TRUE;
}

int phy_interrupt_enable(hdmi_tx_dev_t *dev, u8 value)
{
	dev_write(PHY_MASK0, value);
	return TRUE;
}

int phy_phase_lock_loop_state(hdmi_tx_dev_t *dev)
{
	return dev_read_mask((PHY_STAT0), PHY_STAT0_TX_PHY_LOCK_MASK);
}

void phy_interrupt_mask(hdmi_tx_dev_t *dev, u8 mask)
{
	// Mask will determine which bits will be enabled
	dev_write_mask(PHY_MASK0, mask, 0xff);
}

void phy_interrupt_unmask(hdmi_tx_dev_t *dev, u8 mask)
{
	// Mask will determine which bits will be enabled
	dev_write_mask(PHY_MASK0, mask, 0x0);
}

u8 phy_rx_s0_state(hdmi_tx_dev_t *dev)
{
	return dev_read_mask((PHY_STAT0), PHY_STAT0_RX_SENSE_0_MASK);
}

u8 phy_rx_s1_state(hdmi_tx_dev_t *dev)
{
	return dev_read_mask((PHY_STAT0), PHY_STAT0_RX_SENSE_1_MASK);
}
u8 phy_rx_s2_state(hdmi_tx_dev_t *dev)
{
	return dev_read_mask((PHY_STAT0), PHY_STAT0_RX_SENSE_2_MASK);
}

u8 phy_rx_s3_state(hdmi_tx_dev_t *dev)
{
	return dev_read_mask((PHY_STAT0), PHY_STAT0_RX_SENSE_3_MASK);
}

u8 phy_rx_sense_state(hdmi_tx_dev_t *dev)
{
	u8 state;

	state = phy_rx_s0_state(dev);
	state |= phy_rx_s1_state(dev);
	state |= phy_rx_s2_state(dev);
	state |= phy_rx_s3_state(dev);

	dev->snps_hdmi_ctrl.rx_sense = state ? 1 : 0;
	return state;
}

u8 phy_hot_plug_state(hdmi_tx_dev_t *dev)
{
	u8 state;
	state = dev_read_mask((PHY_STAT0), PHY_STAT0_HPD_MASK);
	dev->snps_hdmi_ctrl.hpd = state ? 1 : 0;
	return state;

}

int phy_hpd_sense(hdmi_tx_dev_t *dev, int enable)
{
	phy_enable_hpd_sense(dev, (enable ? 1 : 0));
	return TRUE;
}

int phy_hot_plug_detected(hdmi_tx_dev_t *dev)
{
	/* MASK         STATUS          POLARITY        INTERRUPT        HPD
	 *   0             0                 0               1             0
	 *   0             1                 0               0             1
	 *   0             0                 1               0             0
	 *   0             1                 1               1             1
	 *   1             x                 x               0             x
	 */

	int hpd_polarity = dev_read_mask(PHY_POL0, PHY_POL0_HPD_MASK);
	int hpd = dev_read_mask(PHY_STAT0, PHY_STAT0_HPD_MASK);

	// Mask interrupt
	phy_interrupt_mask(dev, PHY_MASK0_HPD_MASK);

	if (hpd_polarity == hpd) {
		dev_write_mask(PHY_POL0, PHY_POL0_HPD_MASK, !hpd_polarity);

		// Un-mask interrupts
		phy_interrupt_unmask(dev, PHY_MASK0_HPD_MASK);

		return hpd_polarity;
	}

	// Un-mask interrupts
	phy_interrupt_unmask(dev, PHY_MASK0_HPD_MASK);

	return !hpd_polarity;
}

int phy_rx_s0_detected(hdmi_tx_dev_t *dev)
{
	/* MASK         STATUS          POLARITY        INTERRUPT        RxS
	 *   0             0                 0               1             0
	 *   0             1                 0               0             1
	 *   0             0                 1               0             0
	 *   0             1                 1               1             1
	 *   1             x                 x               0             x
	 */

	int RxS_polarity = dev_read_mask(PHY_POL0, PHY_POL0_RX_SENSE_0_MASK);
	int RxS = dev_read_mask(PHY_STAT0, PHY_STAT0_RX_SENSE_0_MASK);

	// Mask interrupt
	phy_interrupt_mask(dev, PHY_MASK0_RX_SENSE_0_MASK);

	if (RxS_polarity == RxS) {
		dev_write_mask(PHY_POL0, PHY_POL0_RX_SENSE_0_MASK, !RxS_polarity);

		// Un-mask interrupts
		phy_interrupt_unmask(dev, PHY_MASK0_RX_SENSE_0_MASK);

		return RxS_polarity;
	}

	// Un-mask interrupts
	phy_interrupt_unmask(dev, PHY_MASK0_RX_SENSE_0_MASK);

	return !RxS_polarity;
}

int phy_rx_s1_detected(hdmi_tx_dev_t *dev)
{
	/* MASK         STATUS          POLARITY        INTERRUPT        RxS
	 *   0             0                 0               1             0
	 *   0             1                 0               0             1
	 *   0             0                 1               0             0
	 *   0             1                 1               1             1
	 *   1             x                 x               0             x
	 */

	int RxS_polarity = dev_read_mask(PHY_POL0, PHY_POL0_RX_SENSE_1_MASK);
	int RxS = dev_read_mask(PHY_STAT0, PHY_STAT0_RX_SENSE_1_MASK);

	// Mask interrupt
	phy_interrupt_mask(dev, PHY_MASK0_RX_SENSE_1_MASK);

	if (RxS_polarity == RxS) {
		dev_write_mask(PHY_POL0, PHY_POL0_RX_SENSE_1_MASK, !RxS_polarity);

		// Un-mask interrupts
		phy_interrupt_unmask(dev, PHY_MASK0_RX_SENSE_1_MASK);

		return RxS_polarity;
	}

	// Un-mask interrupts
	phy_interrupt_unmask(dev, PHY_MASK0_RX_SENSE_1_MASK);

	return !RxS_polarity;
}

int phy_rx_s2_detected(hdmi_tx_dev_t *dev)
{
	/* MASK         STATUS          POLARITY        INTERRUPT        RxS
	 *   0             0                 0               1             0
	 *   0             1                 0               0             1
	 *   0             0                 1               0             0
	 *   0             1                 1               1             1
	 *   1             x                 x               0             x
	 */

	int RxS_polarity = dev_read_mask(PHY_POL0, PHY_POL0_RX_SENSE_2_MASK);
	int RxS = dev_read_mask(PHY_STAT0, PHY_STAT0_RX_SENSE_2_MASK);

	// Mask interrupt
	phy_interrupt_mask(dev, PHY_MASK0_RX_SENSE_2_MASK);

	if (RxS_polarity == RxS) {
		dev_write_mask(PHY_POL0, PHY_POL0_RX_SENSE_2_MASK, !RxS_polarity);

		// Un-mask interrupts
		phy_interrupt_unmask(dev, PHY_MASK0_RX_SENSE_2_MASK);

		return RxS_polarity;
	}

	// Un-mask interrupts
	phy_interrupt_unmask(dev, PHY_MASK0_RX_SENSE_2_MASK);

	return !RxS_polarity;
}

int phy_rx_s3_detected(hdmi_tx_dev_t *dev)
{
	/* MASK         STATUS          POLARITY        INTERRUPT        RxS
	 *   0             0                 0               1             0
	 *   0             1                 0               0             1
	 *   0             0                 1               0             0
	 *   0             1                 1               1             1
	 *   1             x                 x               0             x
	 */

	int RxS_polarity = dev_read_mask(PHY_POL0, PHY_POL0_RX_SENSE_3_MASK);
	int RxS = dev_read_mask(PHY_STAT0, PHY_STAT0_RX_SENSE_3_MASK);

	// Mask interrupt
	phy_interrupt_mask(dev, PHY_MASK0_RX_SENSE_3_MASK);

	if (RxS_polarity == RxS) {
		dev_write_mask(PHY_POL0, PHY_POL0_RX_SENSE_3_MASK, !RxS_polarity);

		// Un-mask interrupts
		phy_interrupt_unmask(dev, PHY_MASK0_RX_SENSE_3_MASK);

		return RxS_polarity;
	}

	// Un-mask interrupts
	phy_interrupt_unmask(dev, PHY_MASK0_RX_SENSE_3_MASK);

	return !RxS_polarity;
}

struct phy_config * phy316_get_configs(u32 mpixelclock)
{
	pr_debug("mpixelclock:%u\n", mpixelclock);

	if(mpixelclock >= 25175 && mpixelclock < 36000)
		return &(phy316[0]);
	else if(mpixelclock >= 36000 && mpixelclock < 49500)
		return &(phy316[1]);
	else if(mpixelclock >= 49500 && mpixelclock < 72000)
		return &(phy316[2]);
	else if(mpixelclock >= 72000 && mpixelclock < 94500)
		return &(phy316[3]);
	else if(mpixelclock >= 94500 && mpixelclock < 144000)
		return &(phy316[4]);
	else if(mpixelclock >= 144000 && mpixelclock < 175500)
		return &(phy316[5]);
	else if(mpixelclock >= 175500 && mpixelclock < 185625)
		return &(phy316[6]);
	else if(mpixelclock >= 185625 && mpixelclock < 288000)
		return &(phy316[7]);
	else if(mpixelclock >= 288000 && mpixelclock < 348500)
		return &(phy316[8]);
	else if(mpixelclock >= 348500 && mpixelclock < 475200)
		return &(phy316[9]);
	else if(mpixelclock >= 475200 && mpixelclock <= 594000)
		return &(phy316[10]);

	return NULL;
}

struct phy_config * phy316_get_configs_pr(u32 mpixelclock, pixel_repetition_t pixel)
{
	int i = 0;
	while(phy316_pr[i].pixel_clk) {
		if((mpixelclock == phy316_pr[i].pixel_clk) && (pixel == phy316_pr[i].pixel)) {
			return &phy316_pr[i];
		}
		i++;
	}

	return NULL;
}

int phy316_configure(hdmi_tx_dev_t *dev, u32 pclk, color_depth_t color, pixel_repetition_t pixel)
{
	int i   = 0;
	int res = 0;
	u32 phyRead = 0;
	u8 lock = 0;
	struct phy_config * config = NULL;

	if(pixel){
		config = phy316_get_configs_pr(pclk, pixel);
	} else {
		config = phy316_get_configs(pclk);
	}

	if (config == NULL) {
		pr_err("Configuration for clk %x color depth %d"
				  " pixel repetition %d not found\n", pclk, color, pixel);
		return HDMI_ERR_PHY_NOT_CONFIG;
	}

	/*
	 * Some monitors may experience SCDC read/write failures, yet display normally.
	 * Therefore, in this case, we only provide an error message but not return.
	 */
	if(pclk > 340000) {
		dev_write_mask(FC_INVIDCONF, FC_INVIDCONF_HDCP_KEEPOUT_MASK, 0x1);
		res = scrambling(dev, TRUE);
		if (res != 0)
			pr_err("scrambling config failed\n");

		res = tmds_high_rate(dev, TRUE);
		if (res != 0)
			pr_err("tmds high rate config failed\n");

		dev->snps_hdmi_ctrl.src_scramble = dev->snps_hdmi_ctrl.sink_scramble
		 = dev->snps_hdmi_ctrl.high_tmds_ratio = TRUE;
	} else {
		dev_write_mask(FC_INVIDCONF, FC_INVIDCONF_HDCP_KEEPOUT_MASK, 0x0);
		res = scrambling(dev, FALSE);
		if (res != 0)
			pr_err("scrambling config failed\n");

		res = tmds_high_rate(dev, FALSE);
		if (res != 0)
			pr_err("tmds high rate config failed\n");

		dev->snps_hdmi_ctrl.src_scramble = dev->snps_hdmi_ctrl.sink_scramble
		 = dev->snps_hdmi_ctrl.high_tmds_ratio = FALSE;
	}

	phy_standby(dev);
	mc_phy_reset(dev, 1);
	msleep(5);

	phy_set_interface(dev, PHY_I2C);
	mc_phy_reset(dev, 0);

	phy_write(dev, OPMODE_PLLCFG, config->oppllcfg);
	if(phy_read(dev, OPMODE_PLLCFG, &phyRead) || (phyRead != config->oppllcfg))
		pr_err( "%s:OPMODE_PLLCFG Mismatch Write 0x%04x Read 0x%04x\n",
				__func__, config->oppllcfg , phyRead);

	phy_write(dev, PLLCURRCTRL, config->pllcurrctrl);
	if(phy_read(dev, PLLCURRCTRL, &phyRead) || (phyRead != config->pllcurrctrl))
		pr_err( "%s:PLLCURRCTRL Mismatch Write 0x%04x Read 0x%04x\n",
				__func__, config->pllcurrctrl , phyRead);

	phy_write(dev, PLLDIVCTRL, config->pllgmpctrl);
	if(phy_read(dev, PLLDIVCTRL, &phyRead) || (phyRead != config->pllgmpctrl))
		pr_err( "%s:PLLDIVCTRL Mismatch Write 0x%04x Read 0x%04x\n",
				__func__, config->pllgmpctrl , phyRead);

	phy_write(dev, TXTERM, config->txterm);
	if(phy_read(dev, TXTERM, &phyRead) || (phyRead != config->txterm))
		pr_err( "%s:TXTERM Mismatch Write 0x%04x Read 0x%04x\n",
				__func__, config->txterm , phyRead);

	phy_write(dev, VLEVCTRL, config->vlevctrl);
	if(phy_read(dev, VLEVCTRL, &phyRead) || (phyRead != config->vlevctrl))
		pr_err( "%s:VLEVCTRL Mismatch Write 0x%04x Read 0x%04x\n",
				__func__, config->vlevctrl , phyRead);

	phy_write(dev, CKSYMTXCTRL, config->cksymtxctrl);
	if(phy_read(dev, CKSYMTXCTRL, &phyRead) || (phyRead != config->cksymtxctrl))
		pr_err( "%s:CKSYMTXCTRL Mismatch Write 0x%04x Read 0x%04x\n",
				__func__, config->cksymtxctrl , phyRead);

	dev_write_mask(PHY_CONF0, PHY_CONF0_SVSRET_MASK, 1);
	dev_write_mask(PHY_CONF0, PHY_CONF0_TXPWRON_MASK, 1);
	dev_write_mask(PHY_CONF0, PHY_CONF0_PDDQ_MASK, 0);
	dev->snps_hdmi_ctrl.phy_power = TRUE;
	dev->snps_hdmi_ctrl.phy_enable = TRUE;

	/* wait PHY_TIMEOUT no of cycles at most for the PLL lock signal to raise ~around 20us max */
	// Wait for TX ready
	for (i = 0; i < PHY_TIMEOUT; i++) {
		udelay(10);
		lock = phy_phase_lock_loop_state(dev);
		if (lock & 0x1) {
			pr_debug("PHY PLL locked\n");
			return 0;
		}
	}

	pr_debug("PHY PLL not locked\n");

	return HDMI_ERR_PHY_PLL_NOT_LOCK;
}
