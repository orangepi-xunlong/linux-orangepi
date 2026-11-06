#include <linux/types.h>

int rwnx_plat_userconfig_load_8800d80(struct rwnx_hw *rwnx_hw);
#ifdef CONFIG_POWER_LIMIT
int rwnx_plat_powerlimit_load_8800d80(struct rwnx_hw *rwnx_hw);
#endif
int aicwf_set_rf_config_8800d80(struct rwnx_hw *rwnx_hw, struct mm_set_rf_calib_cfm *cfm);


