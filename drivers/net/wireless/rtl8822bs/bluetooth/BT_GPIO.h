#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/gpio.h>
#include<linux/ktime.h>
#include<linux/delay.h>  
//#include <gpio.h>
//#include <linux/gpio/driver.h>
//#include <linux/gpio/consumer.h>

//#define DBG

#define meson
//#define sunxi

//meson
#define GPIOX_17 497
//sunxi
#define PM4 388

#ifdef meson
#define BT_EN GPIOX_17
#endif
#ifdef sunxi
#define BT_EN PM4
#endif

int BT_EN_PIN_export(void);
int BT_EN_PIN_unexport(void);
int rtl8822bs_BT_EN_PIN_pullup(void);
int rtl8822bs_BT_EN_PIN_pulldown(void);
int rtl8822bs_BT_EN_PIN_init(void);
int rtl8822bs_BT_EN_PIN_exit(void);
