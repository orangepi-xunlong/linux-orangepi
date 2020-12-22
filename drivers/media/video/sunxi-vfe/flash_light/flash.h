/*
 * sunxi camera flash device header file
 */
#ifndef __FLASH_H__
#define __FLASH_H__

#include <linux/module.h>
#include <media/v4l2-subdev.h>
#include <media/v4l2-dev.h>

#include "../vfe_subdev.h"
#include "../device/camera_cfg.h"
#include "../vfe_os.h"

//for internel driver debug
//#define FL_DBG
#ifdef FL_DBG
#define fl_dbg(x,arg...)  printk(KERN_DEBUG"[FL_DBG]"x,##arg)
#else
#define fl_dbg(x,arg...) 
#endif
//print when error happens
#define fl_err(x,arg...) printk(KERN_DEBUG"[FL_ERR]"x,##arg)
#define fl_warn(x,arg...) printk(KERN_WARNING"[FL_WARN]"x,##arg)
//print unconditional, for important info
#define fl_prn(x,arg...) printk(KERN_NOTICE"[FL_PRN]"x,##arg)

typedef enum sunxi_flash_ctrl {
  SW_CTRL_FLASH_OFF = 0x100,
  SW_CTRL_FLASH_ON  = 0x101,
  SW_CTRL_TORCH_ON  = 0x102,
  
  CAM_CTRL_FLASH_OFF = 0x200,
  CAM_CTRL_FLASH_ON  = 0x201,
  CAM_CTRL_TORCH_ON  = 0x202,
  
  EXT_SYNC_FLASH_OFF = 0x300,
  EXT_SYNC_FLASH_ON  = 0x301,
  EXT_SYNC_TORCH_ON  = 0x302,
  
}__flash_ctrl_t;

typedef enum sunxi_flash_mode {
  MODE_FLASH_NONE,
  MODE_FLASH_ON,
  MODE_TORCH_ON,
  MODE_FLASH_AUTO,
  MODE_FLASH_RED_EYE,
}__flash_mode_t;

typedef enum sunxi_flash_driver_ic_type {
	FLASH_RELATING,
	FLASH_EN_INDEPEND,
}__flash_driver_ic_type;

typedef enum sunxi_flash_sync {
  NONE,
  LED_SINGLE,
  LED_CONTIN,
  LED_ALTERNATE,
  XENON_PULSE,
}__flash_sync_t;

struct flash_dev_info {
  unsigned int dev_if;              //0-io type 1-i2c type
  unsigned int en_pol;              //polarity
  unsigned int fl_mode_pol;         //polarity
  
  unsigned int light_src;           //0x01-LEDX1 0x02-LEDX2 0x10-XENON
  unsigned int light_temperature;   //in K
  
  unsigned int flash_intensity;     //flash intensity
  unsigned int flash_level;         //in lux
  unsigned int torch_intensity;     //torch intensity
  unsigned int torch_level;         //in lux
  
  unsigned int timeout_counter;     //in us
  
  unsigned int status;              //0-led_off/1-flash_on/2-torch_on/
  enum sunxi_flash_driver_ic_type flash_driver_ic;
  enum sunxi_flash_mode flash_mode;
  enum sunxi_flash_sync flash_sync;
};

int config_flash_mode(struct v4l2_subdev *sd, enum v4l2_flash_led_mode mode,
                      struct flash_dev_info *fls_info);

int io_set_flash_ctrl(struct v4l2_subdev *sd, enum sunxi_flash_ctrl ctrl,
                      struct flash_dev_info *fls_info);

extern int vfe_set_pmu_channel(struct v4l2_subdev *sd, enum pmic_channel pmic_ch, enum on_off on_off);
extern int vfe_set_mclk(struct v4l2_subdev *sd, enum on_off on_off);
extern int vfe_set_mclk_freq(struct v4l2_subdev *sd, unsigned long freq);
extern int vfe_gpio_write(struct v4l2_subdev *sd, enum gpio_type gpio_type, unsigned int status);
extern int vfe_gpio_set_status(struct v4l2_subdev *sd, enum gpio_type gpio_type, unsigned int status);
extern void vfe_get_standby_mode(struct v4l2_subdev *sd, enum standby_mode *stby_mode);

#endif  /* __FLASH_H__ */
