#ifndef __KY_PWRSEQ_H
#define __KY_PWRSEQ_H

struct ky_pwrseq {
	struct device		*dev;
	bool clk_enabled;
	u32 power_on_delay_ms;

	struct clk *ext_clk;
	struct gpio_descs *pwr_gpios;
	struct regulator *vdd_supply;
	struct regulator *io_supply;
	int	vdd_voltage;
	int io_voltage;

	bool always_on;

	struct mutex pwrseq_mutex;
	atomic_t pwrseq_count;
};

void ky_power_on(struct ky_pwrseq *pwrseq, bool on_off);
struct ky_pwrseq *ky_get_pwrseq_from_dev(struct device*);
#endif
