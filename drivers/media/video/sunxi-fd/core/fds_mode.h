extern int fds_run_sl(struct iomap_para reg_addr,struct drammap_para_addr mem_addr,fds_dev_info* ptr_fds_info);
extern int fds_get_status_sl(struct iomap_para reg_addr);
extern long fds_get_rtl_sl(fds_dev_info* ptr_fds_info,struct iomap_para reg_addr,struct drammap_para_addr mem_addr,unsigned char* ptr_rtl_det_buf,int frame_num);




/*----------------------------------*/
/*name:base_get_status              */
/*return value:0 running            */
/*						 1 finish             */
/*						 -1 error status;     */
/*----------------------------------*/    
extern int fds_get_status(struct fds_dev* ptr_fds_dev);
extern long fds_load_ftr(struct fds_dev* ptr_fds_dev,void __user* ptr_ld_file,int size);  
extern long fds_load_ftr_cfg(struct fds_dev* ptr_fds_dev,void __user* ptr_cfg_file,int size);
extern long fds_get_rtl(struct fds_dev* ptr_fds_dev,void __user* ptr_user_rtl_buf,int frame_num);
 
extern int fds_get_run_status(struct fds_dev* ptr_fds_dev);
extern int fds_interrupt_process(struct fds_dev* ptr_fds_dev);
extern int fds_run(struct fds_dev* ptr_fds_dev,void __user* arg);

extern int fds_dev_init(struct fds_dev* ptr_fds_dev);
extern int fds_dev_exit(struct fds_dev* ptr_fds_dev);
extern int fds_dev_release(struct fds_dev* ptr_fds_dev);

extern void fds_clear_angle_context(void);

