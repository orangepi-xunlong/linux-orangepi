#ifndef __VO_INTERFACES_H__
#define __VO_INTERFACES_H__

#ifdef __cplusplus
	extern "C" {
#endif

#include "base_cb.h"
#include "dsi_phy.h"

/*******************************************************
 *  File operations for core
 ******************************************************/
long vo_ioctl(struct file *filp, u_int cmd, u_long arg);
int vo_open(struct inode *inode, struct file *filp);
int vo_release(struct inode *inode, struct file *filp);
unsigned int vo_poll(struct file *filp, struct poll_table_struct *wait);

/*******************************************************
 *  Common interface for core
 ******************************************************/
int vo_cb(void *dev, enum enum_modules_id caller, u32 cmd, void *arg);
irqreturn_t vo_irq_handler(int irq, void *data);
int vo_create_instance(struct platform_device *pdev);
int vo_destroy_instance(struct platform_device *pdev);

#ifdef __cplusplus
}
#endif

#endif /* __VO_INTERFACES_H__ */
