#ifndef _FD_TYPE_
#define _FD_TYPE_

#define MAX_FD_NUM 15
#define MAX_ROI_NUM 15
#define MAX_LD_NUM 8
#define MAX_OD_NUM 15
#define FTR_FILE_SIZE 200*1024 //200KByte




struct phy_dram_para_addr{
	unsigned int phy_mem_src_img_addr;
	unsigned int phy_mem_scale_img_addr;
	unsigned int phy_mem_tmpbuf_addr;
	unsigned int phy_mem_roi_addr;
	unsigned int phy_mem_sys_flag_addr;
	unsigned int phy_mem_rtl_addr;
	unsigned int phy_mem_ftr_cfg_addr;
	             
	unsigned int phy_mem_ld0_addr;
	unsigned int phy_mem_ld1_addr;
	unsigned int phy_mem_ld2_addr;
	unsigned int phy_mem_ld3_addr;
	unsigned int phy_mem_ld4_addr;
	unsigned int phy_mem_ld5_addr;
	unsigned int phy_mem_ld6_addr;
	unsigned int phy_mem_ld7_addr;
	
	unsigned int phy_mem_src_img_size;
	unsigned int phy_mem_scale_img_size;
	unsigned int phy_mem_tmpbuf_size;
	unsigned int phy_mem_roi_size;
	unsigned int phy_mem_sys_flag_size;
	unsigned int phy_mem_rtl_size;
	unsigned int phy_mem_ftr_cfg_size;
	             
	unsigned int phy_mem_ld0_size;
	unsigned int phy_mem_ld1_size;
	unsigned int phy_mem_ld2_size;
	unsigned int phy_mem_ld3_size;
	unsigned int phy_mem_ld4_size;
	unsigned int phy_mem_ld5_size;
	unsigned int phy_mem_ld6_size;
	unsigned int phy_mem_ld7_size;
};

struct drammap_para_addr{
	unsigned char* mem_src_img_addr;
	unsigned char* mem_scale_img_addr;
	unsigned char* mem_tmpbuf_addr;
	unsigned char* mem_roi_addr;
	unsigned char* mem_sys_flag_addr;
	unsigned char* mem_rtl_addr;
	unsigned char* mem_ftr_cfg_addr;
	
	unsigned char* mem_ld0_addr;
	unsigned char* mem_ld1_addr;
	unsigned char* mem_ld2_addr;
	unsigned char* mem_ld3_addr;
	unsigned char* mem_ld4_addr;
	unsigned char* mem_ld5_addr;
	unsigned char* mem_ld6_addr;
	unsigned char* mem_ld7_addr;
	
	unsigned int mem_src_img_size;
	unsigned int mem_scale_img_size;
	unsigned int mem_tmpbuf_size;
	unsigned int mem_roi_size;
	unsigned int mem_sys_flag_size;
	unsigned int mem_rtl_size;
	unsigned int mem_ftr_cfg_size;
	             
	unsigned int mem_ld0_size;
	unsigned int mem_ld1_size;
	unsigned int mem_ld2_size;
	unsigned int mem_ld3_size;
	unsigned int mem_ld4_size;
	unsigned int mem_ld5_size;
	unsigned int mem_ld6_size;
	unsigned int mem_ld7_size;
};

typedef struct  fds_dev_info
{
   //image info
   unsigned int image_buf_phyaddr; 
   int img_width;
   int img_height; 
   int frame_num;
   
   //detect flag;
   short smile_flag;
   short blink_flag;
   //roi
   int roi_x;
   int roi_y;
   int roi_width;
   int roi_height;
   //angle
   short  angle0; 
   short  angle90; 
   short  angle180; 
   short  angle270; 
}fds_dev_info;

typedef struct  fdv_dev_info
{
   //image info
   unsigned int image_buf_phyaddr; 
   int img_width;
   int img_height; 
   int frame_num;
   
   //detect flag;
   short smile_flag;
   short blink_flag;
   //roi
   int roi_x;
   int roi_y;
   int roi_width;
   int roi_height;
   //angle
   short  angle0; 
   short  angle90; 
   short  angle180; 
   short  angle270; 
}fdv_dev_info;

typedef struct fd_rtl_info
{
   int x;
   int y;
   int width;
   int height;
   int angle_idx;
   int confidence;
   
   int smile_level;
   int blink_level; 
}fd_rtl_info;

typedef struct fd_rtl
{
   int frame_num;
   int fd_num; 
   fd_rtl_info fd_rtl_unit[MAX_FD_NUM];
}fd_rtl;


struct fds_dev{
	short  fds_dev_status; 
	short  fds_dev_step;                 
	struct iomap_para* ptr_iomap_addrs;   /* io remap addrs                     */
	struct drammap_para_addr* ptr_mem_addr; /*memory remap addrs                */
	//struct semaphore fds_sem;
	struct fds_dev_info dev_info;                 /*detect para*/ 
	   
};


struct fdv_dev{
	short  fdv_dev_status; 
	short  fdv_dev_step;                 
	struct iomap_para* ptr_iomap_addrs;   /* io remap addrs                     */
	struct drammap_para_addr* ptr_mem_addr; /*memory remap addrs                */
	//struct semaphore fds_sem;
	struct fdv_dev_info dev_info;                 /*detect para*/ 
	void* ptr_fdv_det_buffer;
};


#endif

extern int base_run_sl(struct iomap_para reg_addr,struct drammap_para_addr mem_addr,void* ptr_det_info); 
extern int base_get_dev_size_sl(void); 
extern int base_get_rtl_size_sl(void);
/*----------------------------------*/
/*name:base_get_status              */
/*return value:0 running            */
/*						 1 finish             */
/*						 -1 error status;     */
/*----------------------------------*/    
extern int base_get_status_sl(struct iomap_para reg_addr);
extern long base_get_rtl_sl(struct iomap_para reg_addr,struct drammap_para_addr mem_addr,unsigned char* ptr_rtl_det_buf,int frame_num);

//extern void reset_modules(volatile char* regs_modules);

extern void init_hw_para_sl(struct iomap_para reg_addr,struct drammap_para_addr mem_addr,struct phy_dram_para_addr phy_mem_addr);
extern void set_src_img_phy_addr_sl(struct iomap_para reg_addr,struct phy_dram_para_addr phy_mem_addr);

