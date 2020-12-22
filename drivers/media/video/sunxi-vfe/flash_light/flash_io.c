/* 
 * sunxi io-type flash device driver
 */

#include "flash.h"
#if 0
int io_set_flash_ctrl(struct v4l2_subdev *sd, enum sunxi_flash_ctrl ctrl,
                      struct flash_dev_info *fls_info)
{
  int ret=0;
  unsigned int flash_en, flash_dis, flash_mode, torch_mode;
  //fl_prn("io_set_flash_ctrl!\n");
  
  if(fls_info==NULL)
  {
    fl_err("error flash config!\n");
    return -1;
  }
  
  flash_en=(fls_info->en_pol!=0)?1:0;       
  flash_dis=!flash_en;                      
  flash_mode=(fls_info->fl_mode_pol!=0)?1:0;
  torch_mode=!flash_mode;                   
  
//  fl_dbg("flash_en=%d\n",flash_en);
//  fl_dbg("flash_mode=%d\n",flash_mode);
  
  switch(ctrl) {
    case SW_CTRL_FLASH_OFF:
      fl_dbg("SW_CTRL_FLASH_OFF\n");
      vfe_gpio_set_status(sd,FLASH_EN,1);//set the gpio to output
      vfe_gpio_set_status(sd,FLASH_MODE,1);//set the gpio to output
      ret|=vfe_gpio_write(sd, FLASH_EN, flash_dis);
      ret|=vfe_gpio_write(sd, FLASH_MODE, torch_mode);
//      vfe_gpio_set_status(sd,FLASH_EN,0);//set the gpio to hi-z
//      vfe_gpio_set_status(sd,FLASH_MODE,0);//set the gpio to hi-z
      break;
	  case SW_CTRL_FLASH_ON:
      fl_dbg("SW_CTRL_FLASH_ON\n");
      vfe_gpio_set_status(sd,FLASH_EN,1);//set the gpio to output
      vfe_gpio_set_status(sd,FLASH_MODE,1);//set the gpio to output
      ret|=vfe_gpio_write(sd, FLASH_MODE, flash_mode);
      ret|=vfe_gpio_write(sd, FLASH_EN, flash_en);
      break;
	  case SW_CTRL_TORCH_ON:
      fl_dbg("SW_CTRL_TORCH_ON\n");
      vfe_gpio_set_status(sd,FLASH_EN,1);//set the gpio to output
      vfe_gpio_set_status(sd,FLASH_MODE,1);//set the gpio to output
      ret|=vfe_gpio_write(sd, FLASH_MODE, torch_mode);
      ret|=vfe_gpio_write(sd, FLASH_EN, flash_en);
      break;
	  default:
	    return -EINVAL;
	}
	if(ret!=0)
	{
	  fl_dbg("flash set ctrl fail, force shut off\n");
      ret|=vfe_gpio_write(sd, FLASH_EN, flash_dis);
      ret|=vfe_gpio_write(sd, FLASH_MODE, torch_mode);
	}
  return ret;
}
EXPORT_SYMBOL_GPL(io_set_flash_ctrl);


int config_flash_mode(struct v4l2_subdev *sd, enum v4l2_flash_led_mode mode,
                      struct flash_dev_info *fls_info)
{
  if(fls_info==NULL)
  {
    fl_err("camera flash not support!\n");
    return -1;
  }
  if((fls_info->light_src!=0x01)&&(fls_info->light_src!=0x02)&&
     (fls_info->light_src!=0x10))
  {
    fl_err("unsupported light source, force LEDx1\n");
    fls_info->light_src=0x01;
  }
  
  switch (mode) {
    case V4L2_FLASH_LED_MODE_NONE:
      fls_info->flash_mode = (enum sunxi_flash_mode)MODE_FLASH_NONE;
      break;
    case V4L2_FLASH_LED_MODE_FLASH:
      fls_info->flash_mode = (enum sunxi_flash_mode)MODE_FLASH_ON;
      break;
    case V4L2_FLASH_LED_MODE_TORCH:
      fls_info->flash_mode = (enum sunxi_flash_mode)MODE_TORCH_ON;
      break;
    case V4L2_FLASH_LED_MODE_AUTO:
      fls_info->flash_mode = (enum sunxi_flash_mode)MODE_FLASH_AUTO;
      break;
    case V4L2_FLASH_LED_MODE_RED_EYE:
      fls_info->flash_mode = (enum sunxi_flash_mode)MODE_FLASH_RED_EYE;
      break;
    default:
      return -EINVAL;
  }
  return 0;
}
EXPORT_SYMBOL_GPL(config_flash_mode);
#endif

MODULE_AUTHOR("zenglingying");
MODULE_LICENSE("Dual BSD/GPL");
MODULE_DESCRIPTION("flash light driver for sunxi");

