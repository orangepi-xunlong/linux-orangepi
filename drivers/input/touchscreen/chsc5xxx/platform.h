#ifndef __PLATFORM_H__
#define __PLATFORM_H__

#define SEMI_TOUCH_5472                         0X0400
#define SEMI_TOUCH_5448                         0X0401
#define SEMI_TOUCH_5432                         0X0406
#define SEMI_TOUCH_5816                         0X0500
#define SEMI_TOUCH_IC                           SEMI_TOUCH_5448                             

#define SOC_PLATFORM_MTK                        0x0100
#define SOC_PLATFROM_QUAL                       0x0200
#define SOC_PLATFORM_SPRD                       0x0300
#define SOC_PLATFORM_KY                   0x0400
#define SOC_PLATFORM_SELECT                     SOC_PLATFORM_KY

#define MULTI_PROTOCOL_TYPE_A                   0
#define MULTI_PROTOCOL_TYPE_B                   1
#define MULTI_PROTOCOL_TYPE                     MULTI_PROTOCOL_TYPE_B

#define SEMI_TOUCH_PROXIMITY_OPEN               0
#define SEMI_TOUCH_ESD_CHECK_OPEN               1
#define SEMI_TOUCH_GESTURE_OPEN                 0
#define SEMI_TOUCH_GLOVE_OPEN                   0
#define SEMI_TOUCH_APK_NODE_EN                  1
#define SEMI_TOUCH_ONLINE_UPDATE_EN             0
#define SEMI_TOUCH_BOOTUP_UPDATE_EN             1

#define SEMI_TOUCH_DMA_TRANSFER                 0
#define SEMI_TOUCH_IRQ_VAR_QUEUE                0
#define SEMI_TOUCH_SUSPEND_BY_TPCMD             1
#define SEMI_TOUCH_VKEY_MAPPING                 1
#define SEMI_TOUCH_MAX_POINTS                   5
#define SEMI_TOUCH_SOLUTION_X                   720
#define SEMI_TOUCH_SOLUTION_Y                   1440
#define CHSC_DEVICE_NAME                        "semi_touch"
#define CHSC_AUTO_UPDATE_PACKET_BIN             "/sdcard/chsc_auto_update_packet.bin"

#define TYPE_OF_IC(X)                           ((X) & 0xff00)
#define MAX_VKEY_NUMBER                         5
#define SEMI_TOUCH_KEY_EVT                      {KEY_MENU, KEY_HOME, KEY_BACK, KEY_VOLUMEUP, KEY_VOLUMEDOWN}


/*******************************************************************************************/
#if SOC_PLATFORM_SELECT == SOC_PLATFORM_MTK
#include "tpd.h"
#define semi_touch_get_int()                GTP_INT_PORT
#define semi_touch_get_rst()                GTP_RST_PORT                        
#define semi_io_pin_low(pin)                tpd_gpio_output(pin, 0)
#define semi_io_pin_high(pin)               tpd_gpio_output(pin, 1)
//#define semi_io_pin_level(pin)              mt_get_gpio_in(pin)
#define semi_io_direction_out(pin, level)   tpd_gpio_output(pin, level)
#define semi_io_direction_in(pin)           tpd_gpio_as_int(pin)            
extern  int semi_touch_get_irq(int rst_pin);
extern  int semi_touch_work_done(void);
extern  int semi_touch_resource_release(void);
extern void semi_touch_suspend_entry(struct device* dev);
extern void semi_touch_resume_entry(struct device* dev);

#elif SOC_PLATFORM_SELECT == SOC_PLATFROM_QUAL
extern int semi_touch_get_int(void);
extern int semi_touch_get_rst(void);
#define semi_io_pin_low(pin)                gpio_set_value(pin,0)
#define semi_io_pin_high(pin)               gpio_set_value(pin,1)
//#define semi_io_pin_level(pin)              gpio_get_value(pin)
#define semi_io_direction_out(pin, level)   gpio_direction_output(pin, level)
#define semi_io_direction_in(pin)           gpio_direction_input(pin)
extern int semi_touch_get_irq(int rst_pin);
extern int semi_touch_work_done(void);
extern int semi_touch_resource_release(void);
extern int semi_touch_suspend_entry(struct device* dev);
extern int semi_touch_resume_entry(struct device* dev);

#elif SOC_PLATFORM_SELECT == SOC_PLATFORM_SPRD
extern int semi_touch_get_int(void);
extern int semi_touch_get_rst(void);
#define semi_io_pin_low(pin)                gpio_set_value(pin,0)
#define semi_io_pin_high(pin)               gpio_set_value(pin,1)
//#define semi_io_pin_level(pin)              gpio_get_value(pin)
#define semi_io_direction_out(pin, level)   gpio_direction_output(pin, level)
#define semi_io_direction_in(pin)           gpio_direction_input(pin)
extern int semi_touch_get_irq(int rst_pin);
extern int semi_touch_work_done(void);
extern int semi_touch_resource_release(void);
extern int semi_touch_suspend_entry(struct device* dev);
extern int semi_touch_resume_entry(struct device* dev);

#elif SOC_PLATFORM_SELECT == SOC_PLATFORM_KY
extern int semi_touch_get_int(void);
extern int semi_touch_get_rst(void);
#define semi_io_pin_low(pin)                gpio_set_value(pin,0)
#define semi_io_pin_high(pin)               gpio_set_value(pin,1)
//#define semi_io_pin_level(pin)              gpio_get_value(pin)
#define semi_io_direction_out(pin, level)   gpio_direction_output(pin, level)
#define semi_io_direction_in(pin)           gpio_direction_input(pin)
extern int semi_touch_get_irq(int rst_pin);
extern int semi_touch_work_done(void);
extern int semi_touch_resource_release(void);
extern int semi_touch_suspend_entry(struct device* dev);
extern int semi_touch_resume_entry(struct device* dev);
#endif

#if SEMI_TOUCH_PROXIMITY_OPEN
extern int semi_touch_proximity_init(void);
extern bool semi_touch_proximity_report(unsigned char proximity);
extern int semi_touch_proximity_stop(void);
#else
#define semi_touch_proximity_init()           0
#define semi_touch_proximity_report(x)        true 
#define semi_touch_proximity_stop()           0
#endif

#endif
