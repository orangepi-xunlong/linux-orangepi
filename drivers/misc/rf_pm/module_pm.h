#ifndef RF__PM__H
#define RF__PM__H

struct rf_mod_power {
	char *power_axp;
	int power_gpio;
	unsigned int vol;
	unsigned int used;
};

//rf module common info
struct rf_mod_info {
	int       num;
	struct rf_mod_power power[5];
	int       power_switch;
	int       chip_en;
	char      *lpo_use_apclk;
};

//wl function info
struct wl_func_info {
	int  wifi_used;
	char *module_name;
	int  wl_reg_on;
	int  wl_power_state;

#ifdef CONFIG_PROC_FS
	struct proc_dir_entry		*proc_root;
	struct proc_dir_entry		*proc_power;
#endif
};

#endif
