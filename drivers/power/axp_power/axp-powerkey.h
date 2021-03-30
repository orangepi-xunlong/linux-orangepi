#ifndef AXP_POWERKEY_H
#define AXP_POWERKEY_H

#include "axp-charger.h"

struct axp_powerkey_info {
	struct input_dev *idev;
	struct axp_dev *chip;
};

extern struct axp_interrupts axp_powerkey_irq[4];
extern int axp_powerkey_dt_parse(struct device_node *node,
			struct axp_config_info *axp_config);

#endif /* AXP_POWERKEY_H */
