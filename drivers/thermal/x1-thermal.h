#ifndef __X1_THERMAL_H__
#define __X1_THERMAL_H__

#define BITS(_start, _end) ((BIT(_end) - BIT(_start)) + BIT(_end))

#define MAX_SENSOR_NUMBER		5
#define TEMPERATURE_OFFSET		278

#define REG_TSEN_INT_MASK		0x14
#define TSEN_EMERGENT_INT_OFFSET	23
#define REG_EMERGENT_REBOOT_TEMP_THR		0x68
#define REG_EMERGENT_REBOOT_TEMP_THR_MSK	0xffff

#define REG_TSEN_TIME_CTRL	0x0C
#define BITS_TIME_CTRL_MASK	BITS(0, 23)
#define VALUE_FILTER_PERIOD	(0x3000 << 8)
#define BITS_RST_ADC_CNT	BITS(4, 7)
#define VALUE_WAIT_REF_CNT	(0xf << 0)

#define BIT_TSEN_RAW_SEL	BIT(7)
#define BIT_TEMP_MODE		BIT(3)
#define BIT_EN_SENSOR		BIT(0)
#define BITS_TSEN_SW_CTRL	BITS(18, 21)
#define BITS_CTUNE		BITS(8, 11)
#define REG_TSEN_PCTRL		0x00

#define REG_TSEN_PCTRL2		0x04
#define BITS_SDM_CLK_SEL	BITS(14, 15)
#define BITS_SDM_CLK_SEL_24M	(0 << 14)

#define TSEN_INT_MASK		BIT(0)
#define BIT_HW_AUTO_MODE	BIT(23)

struct sensor_enable {
	unsigned int bjt_en;
	unsigned int offset;
	unsigned int bit_msk;
	unsigned int en_val;
};

struct sensor_data {
	unsigned int data_reg;
	unsigned int offset;
	unsigned int bit_msk;
};

struct sensor_thrsh {
	unsigned int temp_thrsh;
	unsigned int low_offset;
	unsigned int high_offset;
};

struct x1_thermal_sensor_desc {
	void __iomem *base;
	unsigned int int_msk;
	unsigned int int_clr;
	unsigned int int_sta;
	unsigned int offset;
	unsigned int bit_msk;
	struct sensor_enable se;
	struct sensor_data sd;
	struct sensor_thrsh sr;
	struct thermal_zone_device *tzd;
};

struct x1_thermal_sensor {
	int irq;
	void __iomem *base;
	struct clk *clk;
	struct reset_control *resets;
	struct device *dev;
	/* sensor range */
	unsigned int sr[2];
	struct x1_thermal_sensor_desc *sdesc;
};

#endif
