#ifndef __VI_INTERFACES_H__
#define __VI_INTERFACES_H__

#ifdef __cplusplus
	extern "C" {
#endif

const char * const clk_sys_name[] = {
	"clk_sys_0", "clk_sys_1", "clk_sys_2",
	"clk_sys_3", "clk_sys_4", "clk_sys_5"
};
const char * const clk_isp_name[] = {
	"clk_csi_be", "clk_raw", "clk_isp_top"
};
const char * const clk_mac_name[] = {
	"clk_csi_mac0", "clk_csi_mac1", "clk_csi_mac2",
	"clk_csi_mac3", "clk_csi_mac4", "clk_csi_mac5",
};

/*******************************************************
 *  File operations for core
 ******************************************************/
long vi_ioctl(struct file *filp, u_int cmd, u_long arg);
int vi_open(struct inode *inode, struct file *filp);
int vi_release(struct inode *inode, struct file *filp);
int vi_mmap(struct file *filp, struct vm_area_struct *vm);
unsigned int vi_poll(struct file *filp, struct poll_table_struct *wait);

/*******************************************************
 *  Common interface for core
 ******************************************************/
void vi_irq_handler(struct sop_vi_dev *vdev);
int vi_create_instance(struct platform_device *pdev);
int vi_destroy_instance(struct platform_device *pdev);

#ifdef __cplusplus
}
#endif

#endif /* __VI_INTERFACES_H__ */
