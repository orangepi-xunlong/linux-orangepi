// SPDX-License-Identifier: GPL-2.0-only
/*
 *
 * SKY1 System Reset Controller (SRC) driver
 *
 * Author: Jerry Zhu <jerry.zhu@cixtech.com>
 */

#include <linux/acpi.h>
#include <linux/delay.h>
#include <linux/mfd/syscon.h>
#include <linux/module.h>
#include <linux/of_device.h>
#include <linux/platform_device.h>
#include <linux/reset-controller.h>
#include <linux/regmap.h>
#include <dt-bindings/reset/sky1-reset.h>
#include <dt-bindings/reset/sky1-reset-fch.h>

#define SKY1_RESET_SLEEP_MIN_US		10000
#define SKY1_RESET_SLEEP_MAX_US		20000
struct sky1_src_signal {
	unsigned int offset, bit;
};

struct sky1_src_variant {
	const struct sky1_src_signal *signals;
	unsigned int signals_num;
	struct reset_control_ops ops;
};

struct sky1_src {
	struct reset_controller_dev rcdev;
	struct regmap *regmap;
	const struct sky1_src_signal *signals;
};

enum sky1_src_registers {
	CSU_PM_RESET				= 0x304,
	SENSORHUB_RESET				= 0x308,
	SENSORHUB_NOC_RESET			= 0x30c,

	RESET_GROUP0_S0_DOMAIN_0		= 0x400,
	RESET_GROUP0_S0_DOMAIN_1		= 0x404,
	RESET_GROUP1_USB_PHYS			= 0x408,
	RESET_GROUP1_USB_CONTROLLERS		= 0x40c,

	RESET_GROUP0_RCSU			= 0x800,
	RESET_GROUP1_RCSU			= 0x804,

};

static const struct sky1_src_signal sky1_src_signals[SKY1_RESET_NUM] = {
	/* reset group1 for s0 domain modules */
	[SKY1_CSU_PM_RESET_N]		= { CSU_PM_RESET, BIT(0) },
	[SKY1_SENSORHUB_RESET_N]	= { SENSORHUB_RESET, BIT(0) },
	[SKY1_SENSORHUB_NOC_RESET_N]	= { SENSORHUB_NOC_RESET, BIT(0) },
	[SKY1_DDRC_RESET_N]		= { RESET_GROUP0_S0_DOMAIN_0, BIT(0) },
	[SKY1_GIC_RESET_N]		= { RESET_GROUP0_S0_DOMAIN_0, BIT(1) },
	[SKY1_CI700_RESET_N]		= { RESET_GROUP0_S0_DOMAIN_0, BIT(2) },
	[SKY1_SYS_NI700_RESET_N]	= { RESET_GROUP0_S0_DOMAIN_0, BIT(3) },
	[SKY1_MM_NI700_RESET_N]		= { RESET_GROUP0_S0_DOMAIN_0, BIT(4) },
	[SKY1_PCIE_NI700_RESET_N]	= { RESET_GROUP0_S0_DOMAIN_0, BIT(5) },
	[SKY1_GPU_RESET_N]		= { RESET_GROUP0_S0_DOMAIN_0, BIT(6) },
	[SKY1_NPUTOP_RESET_N]		= { RESET_GROUP0_S0_DOMAIN_0, BIT(7) },
	[SKY1_NPUCORE0_RESET_N]		= { RESET_GROUP0_S0_DOMAIN_0, BIT(8) },
	[SKY1_NPUCORE1_RESET_N]		= { RESET_GROUP0_S0_DOMAIN_0, BIT(9) },
	[SKY1_NPUCORE2_RESET_N]		= { RESET_GROUP0_S0_DOMAIN_0, BIT(10) },
	[SKY1_VPU_RESET_N]		= { RESET_GROUP0_S0_DOMAIN_0, BIT(11) },
	[SKY1_ISP_SRESET_N]		= { RESET_GROUP0_S0_DOMAIN_0, BIT(12) },
	[SKY1_ISP_ARESET_N]		= { RESET_GROUP0_S0_DOMAIN_0, BIT(13) },
	[SKY1_ISP_HRESET_N]		= { RESET_GROUP0_S0_DOMAIN_0, BIT(14) },
	[SKY1_ISP_GDCRESET_N]		= { RESET_GROUP0_S0_DOMAIN_0, BIT(15) },
	[SKY1_DPU_RESET0_N]		= { RESET_GROUP0_S0_DOMAIN_0, BIT(16) },
	[SKY1_DPU_RESET1_N]		= { RESET_GROUP0_S0_DOMAIN_0, BIT(17) },
	[SKY1_DPU_RESET2_N]		= { RESET_GROUP0_S0_DOMAIN_0, BIT(18) },
	[SKY1_DPU_RESET3_N]		= { RESET_GROUP0_S0_DOMAIN_0, BIT(19) },
	[SKY1_DPU_RESET4_N]		= { RESET_GROUP0_S0_DOMAIN_0, BIT(20) },
	[SKY1_DP_RESET0_N]		= { RESET_GROUP0_S0_DOMAIN_0, BIT(21) },
	[SKY1_DP_RESET1_N]		= { RESET_GROUP0_S0_DOMAIN_0, BIT(22) },
	[SKY1_DP_RESET2_N]		= { RESET_GROUP0_S0_DOMAIN_0, BIT(23) },
	[SKY1_DP_RESET3_N]		= { RESET_GROUP0_S0_DOMAIN_0, BIT(24) },
	[SKY1_DP_RESET4_N]		= { RESET_GROUP0_S0_DOMAIN_0, BIT(25) },
	[SKY1_DP_PHY_RST_N]		= { RESET_GROUP0_S0_DOMAIN_0, BIT(26) },

	/* reset group1 for s0 domain modules */
	[SKY1_AUDIO_HIFI5_RESET_N]	= { RESET_GROUP0_S0_DOMAIN_1, BIT(0) },
	[SKY1_AUDIO_HIFI5_NOC_RESET_N]	= { RESET_GROUP0_S0_DOMAIN_1, BIT(1) },
	[SKY1_CSIDPHY_PRST0_N]		= { RESET_GROUP0_S0_DOMAIN_1, BIT(2) },
	[SKY1_CSIDPHY_CMNRST0_N]	= { RESET_GROUP0_S0_DOMAIN_1, BIT(3) },
	[SKY1_CSI0_RST_N]		= { RESET_GROUP0_S0_DOMAIN_1, BIT(4) },
	[SKY1_CSIDPHY_PRST1_N]		= { RESET_GROUP0_S0_DOMAIN_1, BIT(5) },
	[SKY1_CSIDPHY_CMNRST1_N]	= { RESET_GROUP0_S0_DOMAIN_1, BIT(6) },
	[SKY1_CSI1_RST_N]		= { RESET_GROUP0_S0_DOMAIN_1, BIT(7) },
	[SKY1_CSI2_RST_N]		= { RESET_GROUP0_S0_DOMAIN_1, BIT(8) },
	[SKY1_CSI3_RST_N]		= { RESET_GROUP0_S0_DOMAIN_1, BIT(9) },
	[SKY1_CSIBRDGE0_RST_N]		= { RESET_GROUP0_S0_DOMAIN_1, BIT(10) },
	[SKY1_CSIBRDGE1_RST_N]		= { RESET_GROUP0_S0_DOMAIN_1, BIT(11) },
	[SKY1_CSIBRDGE2_RST_N]		= { RESET_GROUP0_S0_DOMAIN_1, BIT(12) },
	[SKY1_CSIBRDGE3_RST_N]		= { RESET_GROUP0_S0_DOMAIN_1, BIT(13) },
	[SKY1_GMAC0_RST_N]		= { RESET_GROUP0_S0_DOMAIN_1, BIT(14) },
	[SKY1_GMAC1_RST_N]		= { RESET_GROUP0_S0_DOMAIN_1, BIT(15) },
	[SKY1_PCIE0_RESET_N]		= { RESET_GROUP0_S0_DOMAIN_1, BIT(16) },
	[SKY1_PCIE1_RESET_N]		= { RESET_GROUP0_S0_DOMAIN_1, BIT(17) },
	[SKY1_PCIE2_RESET_N]		= { RESET_GROUP0_S0_DOMAIN_1, BIT(18) },
	[SKY1_PCIE3_RESET_N]		= { RESET_GROUP0_S0_DOMAIN_1, BIT(19) },
	[SKY1_PCIE4_RESET_N]		= { RESET_GROUP0_S0_DOMAIN_1, BIT(20) },

	/* reset group1 for usb phys */
	[SKY1_USB_DP_PHY0_PRST_N]		= { RESET_GROUP1_USB_PHYS, BIT(0) },
	[SKY1_USB_DP_PHY1_PRST_N]		= { RESET_GROUP1_USB_PHYS, BIT(1) },
	[SKY1_USB_DP_PHY2_PRST_N]		= { RESET_GROUP1_USB_PHYS, BIT(2) },
	[SKY1_USB_DP_PHY3_PRST_N]		= { RESET_GROUP1_USB_PHYS, BIT(3) },
	[SKY1_USB_DP_PHY0_RST_N]		= { RESET_GROUP1_USB_PHYS, BIT(4) },
	[SKY1_USB_DP_PHY1_RST_N]		= { RESET_GROUP1_USB_PHYS, BIT(5) },
	[SKY1_USB_DP_PHY2_RST_N]		= { RESET_GROUP1_USB_PHYS, BIT(6) },
	[SKY1_USB_DP_PHY3_RST_N]		= { RESET_GROUP1_USB_PHYS, BIT(7) },
	[SKY1_USBPHY_SS_PST_N]			= { RESET_GROUP1_USB_PHYS, BIT(8) },
	[SKY1_USBPHY_SS_RST_N]			= { RESET_GROUP1_USB_PHYS, BIT(9) },
	[SKY1_USBPHY_HS0_PRST_N]		= { RESET_GROUP1_USB_PHYS, BIT(10) },
	[SKY1_USBPHY_HS1_PRST_N]		= { RESET_GROUP1_USB_PHYS, BIT(11) },
	[SKY1_USBPHY_HS2_PRST_N]		= { RESET_GROUP1_USB_PHYS, BIT(12) },
	[SKY1_USBPHY_HS3_PRST_N]		= { RESET_GROUP1_USB_PHYS, BIT(13) },
	[SKY1_USBPHY_HS4_PRST_N]		= { RESET_GROUP1_USB_PHYS, BIT(14) },
	[SKY1_USBPHY_HS5_PRST_N]		= { RESET_GROUP1_USB_PHYS, BIT(15) },
	[SKY1_USBPHY_HS6_PRST_N]		= { RESET_GROUP1_USB_PHYS, BIT(16) },
	[SKY1_USBPHY_HS7_PRST_N]		= { RESET_GROUP1_USB_PHYS, BIT(17) },
	[SKY1_USBPHY_HS8_PRST_N]		= { RESET_GROUP1_USB_PHYS, BIT(18) },
	[SKY1_USBPHY_HS9_PRST_N]		= { RESET_GROUP1_USB_PHYS, BIT(19) },

	/* reset group1 for usb controllers */
	[SKY1_USBC_SS0_PRST_N]		= { RESET_GROUP1_USB_CONTROLLERS, BIT(0) },
	[SKY1_USBC_SS1_PRST_N]		= { RESET_GROUP1_USB_CONTROLLERS, BIT(1) },
	[SKY1_USBC_SS2_PRST_N]		= { RESET_GROUP1_USB_CONTROLLERS, BIT(2) },
	[SKY1_USBC_SS3_PRST_N]		= { RESET_GROUP1_USB_CONTROLLERS, BIT(3) },
	[SKY1_USBC_SS4_PRST_N]		= { RESET_GROUP1_USB_CONTROLLERS, BIT(4) },
	[SKY1_USBC_SS5_PRST_N]		= { RESET_GROUP1_USB_CONTROLLERS, BIT(5) },
	[SKY1_USBC_SS0_RST_N]		= { RESET_GROUP1_USB_CONTROLLERS, BIT(6) },
	[SKY1_USBC_SS1_RST_N]		= { RESET_GROUP1_USB_CONTROLLERS, BIT(7) },
	[SKY1_USBC_SS2_RST_N]		= { RESET_GROUP1_USB_CONTROLLERS, BIT(8) },
	[SKY1_USBC_SS3_RST_N]		= { RESET_GROUP1_USB_CONTROLLERS, BIT(9) },
	[SKY1_USBC_SS4_RST_N]		= { RESET_GROUP1_USB_CONTROLLERS, BIT(10) },
	[SKY1_USBC_SS5_RST_N]		= { RESET_GROUP1_USB_CONTROLLERS, BIT(11) },
	[SKY1_USBC_HS0_PRST_N]		= { RESET_GROUP1_USB_CONTROLLERS, BIT(12) },
	[SKY1_USBC_HS1_PRST_N]		= { RESET_GROUP1_USB_CONTROLLERS, BIT(13) },
	[SKY1_USBC_HS2_PRST_N]		= { RESET_GROUP1_USB_CONTROLLERS, BIT(14) },
	[SKY1_USBC_HS3_PRST_N]		= { RESET_GROUP1_USB_CONTROLLERS, BIT(15) },
	[SKY1_USBC_HS0_RST_N]		= { RESET_GROUP1_USB_CONTROLLERS, BIT(16) },
	[SKY1_USBC_HS1_RST_N]		= { RESET_GROUP1_USB_CONTROLLERS, BIT(17) },
	[SKY1_USBC_HS2_RST_N]		= { RESET_GROUP1_USB_CONTROLLERS, BIT(18) },
	[SKY1_USBC_HS3_RST_N]		= { RESET_GROUP1_USB_CONTROLLERS, BIT(19) },

	/* reset group0 for rcsu */
	[SKY1_AUDIO_RCSU_RESET_N]		= { RESET_GROUP0_RCSU, BIT(0) },
	[SKY1_CI700_RCSU_RESET_N]		= { RESET_GROUP0_RCSU, BIT(1) },
	[SKY1_CSI_RCSU0_RESET_N]		= { RESET_GROUP0_RCSU, BIT(2) },
	[SKY1_CSI_RCSU1_RESET_N]		= { RESET_GROUP0_RCSU, BIT(3) },
	[SKY1_CSU_PM_RCSU_RESET_N]		= { RESET_GROUP0_RCSU, BIT(4) },
	[SKY1_DDR_BROADCAST_RCSU_RESET_N]	= { RESET_GROUP0_RCSU, BIT(5) },
	[SKY1_DDR_CTRL_RCSU_0_RESET_N]		= { RESET_GROUP0_RCSU, BIT(6) },
	[SKY1_DDR_CTRL_RCSU_1_RESET_N]		= { RESET_GROUP0_RCSU, BIT(7) },
	[SKY1_DDR_CTRL_RCSU_2_RESET_N]		= { RESET_GROUP0_RCSU, BIT(8) },
	[SKY1_DDR_CTRL_RCSU_3_RESET_N]		= { RESET_GROUP0_RCSU, BIT(9) },
	[SKY1_DDR_TZC400_RCSU_0_RESET_N]	= { RESET_GROUP0_RCSU, BIT(10) },
	[SKY1_DDR_TZC400_RCSU_1_RESET_N]	= { RESET_GROUP0_RCSU, BIT(11) },
	[SKY1_DDR_TZC400_RCSU_2_RESET_N]	= { RESET_GROUP0_RCSU, BIT(12) },
	[SKY1_DDR_TZC400_RCSU_3_RESET_N]	= { RESET_GROUP0_RCSU, BIT(13) },
	[SKY1_DP0_RCSU_RESET_N]			= { RESET_GROUP0_RCSU, BIT(14) },
	[SKY1_DP1_RCSU_RESET_N]			= { RESET_GROUP0_RCSU, BIT(15) },
	[SKY1_DP2_RCSU_RESET_N]			= { RESET_GROUP0_RCSU, BIT(16) },
	[SKY1_DP3_RCSU_RESET_N]			= { RESET_GROUP0_RCSU, BIT(17) },
	[SKY1_DP4_RCSU_RESET_N]			= { RESET_GROUP0_RCSU, BIT(18) },
	[SKY1_DPU0_RCSU_RESET_N]		= { RESET_GROUP0_RCSU, BIT(19) },
	[SKY1_DPU1_RCSU_RESET_N]		= { RESET_GROUP0_RCSU, BIT(20) },
	[SKY1_DPU2_RCSU_RESET_N]		= { RESET_GROUP0_RCSU, BIT(21) },
	[SKY1_DPU3_RCSU_RESET_N]		= { RESET_GROUP0_RCSU, BIT(22) },
	[SKY1_DPU4_RCSU_RESET_N]		= { RESET_GROUP0_RCSU, BIT(23) },
	[SKY1_DSU_RCSU_RESET_N]			= { RESET_GROUP0_RCSU, BIT(24) },
	[SKY1_FCH_RCSU_RESET_N]			= { RESET_GROUP0_RCSU, BIT(25) },
	[SKY1_GICD_RCSU_RESET_N]		= { RESET_GROUP0_RCSU, BIT(26) },
	[SKY1_GMAC_RCSU_RESET_N]		= { RESET_GROUP0_RCSU, BIT(27) },
	[SKY1_GPU_RCSU_RESET_N]			= { RESET_GROUP0_RCSU, BIT(28) },
	[SKY1_ISP_RCSU0_RESET_N]		= { RESET_GROUP0_RCSU, BIT(29) },
	[SKY1_ISP_RCSU1_RESET_N]		= { RESET_GROUP0_RCSU, BIT(30) },
	[SKY1_NI700_MMHUB_RCSU_RESET_N]		= { RESET_GROUP0_RCSU, BIT(31) },

	/* reset group1 for rcsu */
	[SKY1_NPU_RCSU_RESET_N]			= { RESET_GROUP1_RCSU, BIT(0) },
	[SKY1_NI700_PCIE_RCSU_RESET_N]		= { RESET_GROUP1_RCSU, BIT(1) },
	[SKY1_PCIE_X421_RCSU_RESET_N]		= { RESET_GROUP1_RCSU, BIT(2) },
	[SKY1_PCIE_X8_RCSU_RESET_N]		= { RESET_GROUP1_RCSU, BIT(3) },
	[SKY1_SF_RCSU_RESET_N]			= { RESET_GROUP1_RCSU, BIT(4) },
	[SKY1_RCSU_SMMU_MMHUB_RESET_N]		= { RESET_GROUP1_RCSU, BIT(5) },
	[SKY1_RCSU_SMMU_PCIEHUB_RESET_N]	= { RESET_GROUP1_RCSU, BIT(6) },
	[SKY1_RCSU_SYSHUB_RESET_N]		= { RESET_GROUP1_RCSU, BIT(7) },
	[SKY1_NI700_SMN_RCSU_RESET_N]		= { RESET_GROUP1_RCSU, BIT(8) },
	[SKY1_NI700_SYSHUB_RCSU_RESET_N]	= { RESET_GROUP1_RCSU, BIT(9) },
	[SKY1_RCSU_USB2_HOST0_RESET_N]		= { RESET_GROUP1_RCSU, BIT(10) },
	[SKY1_RCSU_USB2_HOST1_RESET_N]		= { RESET_GROUP1_RCSU, BIT(11) },
	[SKY1_RCSU_USB2_HOST2_RESET_N]		= { RESET_GROUP1_RCSU, BIT(12) },
	[SKY1_RCSU_USB2_HOST3_RESET_N]		= { RESET_GROUP1_RCSU, BIT(13) },
	[SKY1_RCSU_USB3_TYPEA_DRD_RESET_N]	= { RESET_GROUP1_RCSU, BIT(14) },
	[SKY1_RCSU_USB3_TYPEC_DRD_RESET_N]	= { RESET_GROUP1_RCSU, BIT(15) },
	[SKY1_RCSU_USB3_TYPEC_HOST0_RESET_N]	= { RESET_GROUP1_RCSU, BIT(16) },
	[SKY1_RCSU_USB3_TYPEC_HOST1_RESET_N]	= { RESET_GROUP1_RCSU, BIT(17) },
	[SKY1_RCSU_USB3_TYPEC_HOST2_RESET_N]	= { RESET_GROUP1_RCSU, BIT(18) },
	[SKY1_VPU_RCSU_RESET_N]			= { RESET_GROUP1_RCSU, BIT(19) },

};

enum sky1_src_fch_registers {
	FCH_SW_RST_FUNC			= 0x008,
	FCH_SW_RST_BUS			= 0x00c,
	FCH_SW_XSPI			= 0x010,
};

static const struct sky1_src_signal sky1_src_fch_signals[SKY1_FCH_RESET_NUM] = {
	/* resets for fch_sw_rst_func */
	[SW_I3C0_RST_FUNC_G_N]	= { FCH_SW_RST_FUNC, BIT(0) },
	[SW_I3C0_RST_FUNC_I_N]	= { FCH_SW_RST_FUNC, BIT(1) },
	[SW_I3C1_RST_FUNC_G_N]	= { FCH_SW_RST_FUNC, BIT(2) },
	[SW_I3C1_RST_FUNC_I_N]	= { FCH_SW_RST_FUNC, BIT(3) },
	[SW_UART0_RST_FUNC_N]	= { FCH_SW_RST_FUNC, BIT(4) },
	[SW_UART1_RST_FUNC_N]	= { FCH_SW_RST_FUNC, BIT(5) },
	[SW_UART2_RST_FUNC_N]	= { FCH_SW_RST_FUNC, BIT(6) },
	[SW_UART3_RST_FUNC_N]	= { FCH_SW_RST_FUNC, BIT(7) },
	[SW_TIMER_RST_FUNC_N]	= { FCH_SW_RST_FUNC, BIT(20) },

	/* resets for fch_sw_rst_bus */
	[SW_I3C0_RST_APB_N]	= { FCH_SW_RST_BUS, BIT(0) },
	[SW_I3C1_RST_APB_N]	= { FCH_SW_RST_BUS, BIT(1) },
	[SW_DMA_RST_AXI_N]	= { FCH_SW_RST_BUS, BIT(2) },
	[SW_UART0_RST_APB_N]	= { FCH_SW_RST_BUS, BIT(4) },
	[SW_UART1_RST_APB_N]	= { FCH_SW_RST_BUS, BIT(5) },
	[SW_UART2_RST_APB_N]	= { FCH_SW_RST_BUS, BIT(6) },
	[SW_UART3_RST_APB_N]	= { FCH_SW_RST_BUS, BIT(7) },
	[SW_SPI0_RST_APB_N]	= { FCH_SW_RST_BUS, BIT(8) },
	[SW_SPI1_RST_APB_N]	= { FCH_SW_RST_BUS, BIT(9) },
	[SW_I2C0_RST_APB_N]	= { FCH_SW_RST_BUS, BIT(12) },
	[SW_I2C1_RST_APB_N]	= { FCH_SW_RST_BUS, BIT(13) },
	[SW_I2C2_RST_APB_N]	= { FCH_SW_RST_BUS, BIT(14) },
	[SW_I2C3_RST_APB_N]	= { FCH_SW_RST_BUS, BIT(15) },
	[SW_I2C4_RST_APB_N]	= { FCH_SW_RST_BUS, BIT(16) },
	[SW_I2C5_RST_APB_N]	= { FCH_SW_RST_BUS, BIT(17) },
	[SW_I2C6_RST_APB_N]	= { FCH_SW_RST_BUS, BIT(18) },
	[SW_I2C7_RST_APB_N]	= { FCH_SW_RST_BUS, BIT(19) },
	[SW_GPIO_RST_APB_N]	= { FCH_SW_RST_BUS, BIT(21) },

	/* resets for fch_sw_xspi */
	[SW_XSPI_REG_RST_N]	= { FCH_SW_XSPI, BIT(0) },
	[SW_XSPI_SYS_RST_N]	= { FCH_SW_XSPI, BIT(1) },
};

static struct sky1_src *to_sky1_src(struct reset_controller_dev *rcdev)
{
	return container_of(rcdev, struct sky1_src, rcdev);
}

static int sky1_reset_update(struct sky1_src *sky1src,
			     unsigned long id, unsigned int value)
{
	const struct sky1_src_signal *signal = &sky1src->signals[id];

	return regmap_update_bits(sky1src->regmap,
				  signal->offset, signal->bit, value);
}

static int sky1_reset_set(struct reset_controller_dev *rcdev,
			  unsigned long id, bool assert)
{
	struct sky1_src *sky1src = to_sky1_src(rcdev);
	const unsigned int bit = sky1src->signals[id].bit;
	unsigned int value = assert ? 0 : bit;

	return sky1_reset_update(sky1src, id, value);
}

static int sky1_reset(struct reset_controller_dev *rcdev,
			     unsigned long id)
{
	sky1_reset_set(rcdev, id, true);
	usleep_range(SKY1_RESET_SLEEP_MIN_US,
		     SKY1_RESET_SLEEP_MAX_US);

	sky1_reset_set(rcdev, id, false);

	/*
	 * Ensure component is taken out reset state by sleeping also after
	 * deasserting the reset, Otherwise, the component may not be ready
	 * for operation.
	 */
	usleep_range(SKY1_RESET_SLEEP_MIN_US,
		     SKY1_RESET_SLEEP_MAX_US);
	return 0;
}

static int sky1_reset_assert(struct reset_controller_dev *rcdev,
			     unsigned long id)
{
	return sky1_reset_set(rcdev, id, true);
}

static int sky1_reset_deassert(struct reset_controller_dev *rcdev,
			       unsigned long id)
{
	return sky1_reset_set(rcdev, id, false);
}

static int sky1_reset_status(struct reset_controller_dev *rcdev,
			       unsigned long id)
{
	unsigned int value = 0;
	struct sky1_src *sky1_src = to_sky1_src(rcdev);
	const struct sky1_src_signal *signal = &sky1_src->signals[id];
	regmap_read(sky1_src->regmap, signal->offset, &value);
	return !(value & signal->bit);

}
static const struct sky1_src_variant variant_sky1 = {
	.signals = sky1_src_signals,
	.signals_num = ARRAY_SIZE(sky1_src_signals),
	.ops = {
		.reset	  = sky1_reset,
		.assert   = sky1_reset_assert,
		.deassert = sky1_reset_deassert,
		.status   = sky1_reset_status,
	},
};

static const struct sky1_src_variant variant_sky1_fch = {
	.signals = sky1_src_fch_signals,
	.signals_num = ARRAY_SIZE(sky1_src_fch_signals),
	.ops = {
		.reset	  = sky1_reset,
		.assert   = sky1_reset_assert,
		.deassert = sky1_reset_deassert,
		.status	  = sky1_reset_status,
	},
};

static int sky1_reset_probe(struct platform_device *pdev)
{
	struct sky1_src *sky1src;
	struct device *dev = &pdev->dev;
	struct resource *res;
	void __iomem *base;
	const struct sky1_src_variant *variant = ACPI_COMPANION(dev) ?
		acpi_device_get_match_data(dev) : of_device_get_match_data(dev);
	struct regmap_config config = {
		.reg_bits = 32,
		.val_bits = 32,
		.reg_stride = 4,
		.name = "src",
	};

	sky1src = devm_kzalloc(dev, sizeof(*sky1src), GFP_KERNEL);
	if (!sky1src)
		return -ENOMEM;

	sky1src->signals = variant->signals;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res)
		return -ENOENT;

	base = devm_ioremap(dev, res->start, resource_size(res));
	if (!base)
		return -ENOMEM;

	sky1src->regmap = devm_regmap_init_mmio(dev, base, &config);
	if (IS_ERR(sky1src->regmap)) {
		dev_err(dev, "Unable to get sky1-src regmap");
		return PTR_ERR(sky1src->regmap);
	}

	sky1src->rcdev.owner     = THIS_MODULE;
	sky1src->rcdev.nr_resets = variant->signals_num;
	sky1src->rcdev.ops       = &variant->ops;
	sky1src->rcdev.of_node   = dev->of_node;
	sky1src->rcdev.dev       = dev;

	return devm_reset_controller_register(dev, &sky1src->rcdev);
}

static const struct of_device_id sky1_reset_dt_ids[] = {
	{ .compatible = "cix,sky1-src", .data = &variant_sky1 },
	{ .compatible = "cix,sky1-src-fch", .data = &variant_sky1_fch },
	{ /* sentinel */ },
};
MODULE_DEVICE_TABLE(of, sky1_reset_dt_ids);

static const struct acpi_device_id sky1_reset_acpi_match[] = {
	{ "CIXHA020", .driver_data = (kernel_ulong_t)&variant_sky1 },
	{ "CIXHA021", .driver_data = (kernel_ulong_t)&variant_sky1_fch },
	{},
};
MODULE_DEVICE_TABLE(acpi, sky1_reset_acpi_match);

static struct platform_driver sky1_reset_driver = {
	.probe	= sky1_reset_probe,
	.driver = {
		.name		= KBUILD_MODNAME,
		.of_match_table	= sky1_reset_dt_ids,
		.acpi_match_table = ACPI_PTR(sky1_reset_acpi_match),
	},
};
static int __init reset_sky1_init(void)
{
	return platform_driver_register(&sky1_reset_driver);
}
subsys_initcall(reset_sky1_init);

static void __exit reset_sky1_exit(void)
{
	platform_driver_unregister(&sky1_reset_driver);
}
module_exit(reset_sky1_exit);

MODULE_AUTHOR("Jerry Zhu <jerry.zhu@cixtech.com>");
MODULE_DESCRIPTION("Cix Sky1 reset driver");
MODULE_LICENSE("GPL v2");
