#ifndef _AXP_SYS_H
#define _AXP_SYS_H

#include <linux/mfd/axp-mfd.h>

#if defined(CONFIG_AW_AXP19) || defined(CONFIG_AW_AXP22) || defined(CONFIG_AW_AXP81X)
#elif defined(CONFIG_AW_AXP20)
unsigned long axp_read_power_sply(void){ return 0; };
int axp_read_bat_cap(void){return 100;};
int axp_read_ac_chg(void){ return 0; };
#elif defined(CONFIG_AW_AXP)
int axp_usbcur(aw_charge_type type){ return 0; };
int axp_usbvol(aw_charge_type type){ return 0; };
int axp_usb_det(void){ return 0; };
int axp_powerkey_get(void){ return 0; };
void axp_powerkey_set(int value){};
unsigned long axp_read_power_sply(void){ return 0; };
int axp_read_bat_cap(void){return 100;};
int axp_read_ac_chg(void){ return 0; };
/* axp15 not define this function */
void axp_reg_debug(int reg, int len, uint8_t *val){return;};
#endif
#endif
