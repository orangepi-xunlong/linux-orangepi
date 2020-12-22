#ifndef SUNXI_D7S_H
#define SUNXI_D7S_H

#define D7S_MINOR 199

/* sunxi d7s cmd */
#define D7S_ENABLE		(1<<0)
#define D7S_DISABLE		(1<<2)
#define D7S_SET_POWER_LEVEL	(1<<4)
#define D7S_SHOW_NUMBER		(1<<6)

struct d7s_info{
	struct gpio_config din;
	struct gpio_config clk;
	struct gpio_config stb;
};

void sunxi_d7s_show_number(u32 number);
void sunxi_d7s_set_power_level(u8 level);
void sunxi_d7s_display_on(void);
void sunxi_d7s_display_off(void);

#endif //SUNXI_D7S_H
