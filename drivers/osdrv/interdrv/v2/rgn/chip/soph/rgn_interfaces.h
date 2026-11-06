#ifndef __RGN_INTERFACES_H__
#define __RGN_INTERFACES_H__

#ifdef __cplusplus
	extern "C" {
#endif

#include <base_cb.h>

/*******************************************************
 *  File operations for core
 ******************************************************/
long rgn_ioctl(struct file *filp, u_int cmd, u_long arg);
int rgn_open(struct inode *inode, struct file *filp);
int rgn_release(struct inode *inode, struct file *filp);
int rgn_mmap(struct file *filp, struct vm_area_struct *vm);
unsigned int rgn_poll(struct file *filp, struct poll_table_struct *wait);

/*******************************************************
 *  Common interface for core
 ******************************************************/
int rgn_cb(void *dev, enum enum_modules_id caller, unsigned int cmd, void *arg);
int rgn_create_instance(struct platform_device *pdev);
int rgn_destroy_instance(struct platform_device *pdev);

#ifdef __cplusplus
}
#endif

#endif /* __RGN_INTERFACES_H__ */
