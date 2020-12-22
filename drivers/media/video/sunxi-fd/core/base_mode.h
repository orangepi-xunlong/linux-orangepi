extern int base_run(struct iomap_para reg_addr,struct drammap_para_addr mem_addr,void* ptr_det_info);

/*----------------------------------*/
/*name:base_get_status              */
/*return value:0 running            */
/*						 1 finish             */
/*						 -1 error status;     */
/*----------------------------------*/    
extern int base_get_status(struct iomap_para reg_addr);
extern long base_load_ftr(struct drammap_para_addr mem_addr,void __user* ptr_ld_file,int size,int ld_idx);  
extern long base_get_rtl(struct iomap_para reg_addr,struct drammap_para_addr mem_addr,void __user* ptr_user_rtl_buf,int frame_num);  
extern void base_clear_angle_context(void);       


