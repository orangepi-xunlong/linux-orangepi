#ifndef _SUN_FD_H_
#define _SUN_FD_H_



enum IOCTL_CMD {
	IOCTL_UNKOWN = 0x100,
	
	IOCTL_ENABLE_ENGINE   ,
	IOCTL_DISABLE_ENGINE  ,
	IOCTL_SET_FREQ        ,
	IOCTL_RESET_ENGINE    ,
	IOCTL_SET_MODE        ,
	                      
	//Base Mode           
	IOCTL_BASE_RUN        ,
	IOCTL_BASE_GET_RTL    ,
	IOCTL_BASE_GET_STA    ,
	IOCTL_BASE_WAIT_ENGINE,
	IOCTL_BASE_LOAD_FTR   ,
	                      
	//FD Video Mode       
  IOCTL_FD_V_SET_ORI    ,
  IOCTL_FD_V_ENABLE_SMILE  ,
  IOCTL_FD_V_DISABLE_SMILE  ,
  IOCTL_FD_V_ENABLE_BLINK  ,
  IOCTL_FD_V_DISABLE_BLINK  ,
  IOCTL_FD_V_RUN        ,
  IOCTL_FD_V_GET_RTL    ,
  IOCTL_FD_V_GET_STA    ,
  IOCTL_FD_V_WAIT_FD    ,
  IOCTL_FD_V_WAIT_SMILE ,
  IOCTL_FD_V_WAIT_BLINK ,
  IOCTL_FD_V_WAIT_ENGINE ,
  IOCTL_FD_V_LOAD_FD_FTR,
  IOCTL_FD_V_LOAD_FD_FTR_CFG,
  IOCTL_FD_V_LOAD_SE_FTR,
  IOCTL_FD_V_LOAD_BL_FTR,     
  
  //FD Static Frame Mode
  IOCTL_FD_S_SET_ORI    ,
  IOCTL_FD_S_ENABLE_SMILE  ,
  IOCTL_FD_S_DISABLE_SMILE  ,
  IOCTL_FD_S_ENABLE_BLINK  ,
  IOCTL_FD_S_DISABLE_BLINK  ,
  IOCTL_FD_S_RUN        ,
  IOCTL_FD_S_GET_RTL    ,
  IOCTL_FD_S_GET_STA    ,
  IOCTL_FD_S_WAIT_FD    ,
  IOCTL_FD_S_WAIT_SMILE ,
  IOCTL_FD_S_WAIT_BLINK ,
  IOCTL_FD_S_WAIT_ENGINE ,
  IOCTL_FD_S_LOAD_FD_FTR,
  IOCTL_FD_S_LOAD_FD_FTR_CFG,
  IOCTL_FD_S_LOAD_SE_FTR,
  IOCTL_FD_S_LOAD_BL_FTR,

	IOCTL_READ_REG = 0x300,
	IOCTL_WRITE_REG,
};




struct fd_env_infomation{
	unsigned int phymem_start;
	int  phymem_total_size;
	unsigned int  address_fd;
};

struct fd_cache_range{
	long start;
	long end;
};

struct reg_paras{
	unsigned int addr;
	unsigned int value;
};

struct __fd_task {
	int task_prio;
	int ID;
	unsigned long timeout;	
	unsigned int frametime;
	unsigned int block_mode;
};

struct fd_engine_task {
	struct __fd_task t;	
	struct list_head list;
	struct task_struct *task_handle;
	unsigned int status;
	unsigned int running;
	unsigned int is_first_task;
};

/*�������ȼ�task_prio��ѯ��ǰ����task��frametime���ͱ����ȼ�task_prio�ߵ�task�������е���ʱ��total_time*/
struct fd_engine_task_info {
	int task_prio;
	unsigned int frametime;
	unsigned int total_time;
};

struct fd_regop {
    unsigned int addr;
    unsigned int value;
};



struct fd_dev {
	struct cdev cdev;	             /* char device struct                 */
	struct device *dev;              /* ptr to class device struct         */
	struct class  *class;            /* class for auto create device node  */

	struct semaphore sem;            /* mutual exclusion semaphore         */
	
	wait_queue_head_t wq;            /* wait queue for poll ops            */

	struct iomap_para iomap_addrs;   /* io remap addrs                     */
	struct drammap_para_addr mem_addr; /*memory remap addrs                */
	struct phy_dram_para_addr phy_mem_addr;

  void *ptr_od_dev;                 /*detect para*/ 
  struct fds_dev *ptr_fds_dev;
  struct fdv_dev *ptr_fdv_dev;
  
	struct timer_list fd_engine_timer;
	struct timer_list fd_engine_timer_rel;
	
	u32 mode_idx;                    /*fd mode :1 base_mode,2 FD Static Mode,3 FD Video Mode*/
	u32 irq;                         /* fd  engine irq number      */
	u32 fd_irq_flag;                    /* flag of fd engine irq generated */
	u32 fd_irq_value;                   /* value of fd engine irq          */

	u32 irq_has_enable;
	u32 ref_count;
};
/*--------------------------------------------------------------------------------*/
#define REGS_pBASE			(0x03A00000)	 	      // register base addr

#define FD_REGS_pBASE     (REGS_pBASE + 0x00000)    // SRAM Controller
#define CCMU_REGS_pBASE     (REGS_pBASE + 0x20000)    // clock manager unit
#define SS_REGS_pBASE       (REGS_pBASE + 0x15000)    // Security System
#define SDRAM_REGS_pBASE    (REGS_pBASE + 0x01000)    // SDRAM Controller

#define FD_REGS_BASE      FD_REGS_pBASE           // SRAM Controller
#define CCMU_REGS_BASE      (0x06000000)           // Clock Control manager unit  OK

#define FD_REGS_SIZE      (64*1024)  // 4K
#define CCMU_REGS_SIZE      (4*1024)  // 1K
#define SS_REGS_SIZE        (4096)  // 4K

#define RTL_BUFFER_SIZE 6*1024
#define FTR_CFG__BUFFER_SIZE 8*1024
#define SCALE_IMG_BUFFER_SIZE 6*1024*1024
#define TMP_BUFFER_SIZE (16*1024*1024 + 1024)
#define HALF_TMP_BUFFER_SIZE (8*1024*1024 + 1024)
#define ROI_CFG_BUFFER_SIZE 1024
#define SYS_FLAG_BUFFER_SIZE 16
/*--------------------------------------------------------------------------------*/
extern void* get_fddevp(void);
#endif
