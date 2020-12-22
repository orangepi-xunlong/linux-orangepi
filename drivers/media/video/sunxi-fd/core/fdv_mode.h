extern int fdv_get_status_sl(struct iomap_para reg_addr);
extern int fdv_run_sl(struct fdv_dev* ptr_fdv_dev,int flag);

extern long fdv_step_complete_notify(struct fdv_dev* ptr_fdv_dev,int step,int frame_num);
extern long fdv_get_rtl_sl(struct fdv_dev* ptr_fdv_dev,unsigned char* ptr_rtl_det_buf,int frame_num);
extern int get_sizeof_fdv_buffer(void);
extern void clear_fdv_det_buf_sl(struct fdv_dev* ptr_fdv_dev);

/*----------------------------------*/
/*name:base_get_status              */
/*return value:0 running            */
/*						 1 finish             */
/*						 -1 error status;     */
/*----------------------------------*/    
extern int fdv_get_status(struct fdv_dev* ptr_fdv_dev);
extern long fdv_load_ftr(struct fdv_dev* ptr_fdv_dev,void __user* ptr_ld_file,int size);  
extern long fdv_load_ftr_cfg(struct fdv_dev* ptr_fdv_dev,void __user* ptr_cfg_file,int size);
extern long fdv_get_rtl(struct fdv_dev* ptr_fdv_dev,void __user* ptr_user_rtl_buf,int frame_num);
extern int fdv_run(struct fdv_dev* ptr_fdv_dev,void __user* arg);
extern int fdv_get_run_status(struct fdv_dev* ptr_fdv_dev);
extern int fdv_interrupt_process(struct fdv_dev* ptr_fdv_dev);

extern int fdv_dev_init(struct fdv_dev* ptr_fdv_dev);
extern int fdv_dev_exit(struct fdv_dev* ptr_fdv_dev);
extern int fdv_dev_release(struct fdv_dev* ptr_fdv_dev);

extern int fdv_clear_angle_context(void);
extern void wake_up_fdv(void);
