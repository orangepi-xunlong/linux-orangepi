#ifndef __DEV_DISPLAY_H__
#define __DEV_DISPLAY_H__

#include "drv_hdmi_i.h"

int hdmi_open(struct inode *inode, struct file *file);
int hdmi_release(struct inode *inode, struct file *file);
ssize_t hdmi_read(struct file *file, char __user *buf, size_t count, loff_t *ppos);
ssize_t hdmi_write(struct file *file, const char __user *buf, size_t count, loff_t *ppos);
int hdmi_mmap(struct file *file, struct vm_area_struct * vma);
long hdmi_ioctl(struct file *file, unsigned int cmd, unsigned long arg);

extern __s32 Hdmi_init(void);
extern __s32 Hdmi_exit(void);
extern __s32 Fb_Init(__u32 from);
__s32 hdmi_hpd_state(__u32 state);
__s32 Hdmi_hpd_event(__u32 event);

typedef struct
{
	struct device   *dev;
	__bool bopen;
	__disp_tv_mode_t mode;
	__u32           base_hdmi;
	struct work_struct      hpd_work;
}hdmi_info_t;

extern hdmi_info_t ghdmi;

#endif
