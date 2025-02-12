#ifndef __HEAD_DEFINE__
#define __HEAD_DEFINE__
#include <linux/time.h>
#include <linux/rtc.h>
#include <linux/mutex.h>
#include <linux/workqueue.h>
#include "platform.h"

#define LOG_LEVEL_E              KERN_EMERG
#define LOG_LEVEL_D              KERN_EMERG /*KERN_DEBUG*/
#define MODULE_NAME              "CHSC"
#define CHSC_DRIVER_VERSION      "v3.7.3"

#define MAX_CORE_WRITE_LEN       128

#if SEMI_TOUCH_APK_NODE_EN
#define MAX_IO_BUFFER_LEN        512
#define MAX_TX_RX_BUFF_LEN       (3 * 1024)
#else
#define MAX_IO_BUFFER_LEN        128
#define MAX_TX_RX_BUFF_LEN       4
#endif

extern struct sm_touch_dev st_dev;

#define HEAD "[%s] function = %-30s, line = %-4d: "
#define kernel_log_e(fmt, ...)   printk(LOG_LEVEL_E HEAD fmt, MODULE_NAME, __FUNCTION__, __LINE__, ##__VA_ARGS__)
#define kernel_log_d(fmt, ...)   printk(LOG_LEVEL_D HEAD fmt, MODULE_NAME, __FUNCTION__, __LINE__, ##__VA_ARGS__)
#define check_return_if_fail(x, complete)  do{ if(IS_ERR((void*)(long)x)) { kernel_log_e("err code = %ld\r\n", (long)x); if(complete > 0) ((de_init_fun)complete)(); return PTR_ERR((void*)(long)x); }}while(0)
#define check_return_if_zero(x, complete)  do{ if(NULL == (void*)(long)x) { kernel_log_e("err code = %d\r\n", -ENOMEM);  if(complete > 0) ((de_init_fun)complete)(); return -ENOMEM; }}while(0)
#define check_break_if_fail(x, complete)   { if(IS_ERR((void*)(long)x)) { kernel_log_e("err code = %ld\r\n", PTR_ERR((void*)(long)x));  if(complete > 0) ((de_init_fun)complete)(); break; }}

typedef void (*de_init_fun)(void);

#if TYPE_OF_IC(SEMI_TOUCH_IC) == TYPE_OF_IC(SEMI_TOUCH_5472)
#define TP_CMD_BUFF_ADDR             0x20000000
#define TP_RSP_BUFF_ADDR             0x20000000
#define TP_WR_BUFF_ADDR              0x20008000
#define TP_RD_BUFF_ADDR              0x20008400
#define TP_HOLD_MCU_ADDR             0x40008004
#define TP_AUTO_FEED_ADDR            0x4000800c
#define TP_REMAP_MCU_ADDR            0x40008008
#define TP_RELEASE_MCU_ADDR          0x40008004
#define TP_HOLD_MCU_VAL              0x4000005c
#define TP_AUTO_FEED_VAL             0x0000005a
#define TP_REMAP_MCU_VAL             0x00000100
#define TP_RELEASE_MCU_VAL           0xc000005c
#define CFG_ROM_ADDRESS              (96 * 1024)
#define VID_PID_BACKUP_ADDR          (100 * 1024 + 0X10)
#define ADC_NUM_MAX                  72
#elif TYPE_OF_IC(SEMI_TOUCH_IC) == TYPE_OF_IC(SEMI_TOUCH_5816)
#define TP_CMD_BUFF_ADDR             0x20000000
#define TP_RSP_BUFF_ADDR             0x20000000
#define TP_WR_BUFF_ADDR              0x20002000
#define TP_RD_BUFF_ADDR              0x20002400
#define TP_HOLD_MCU_ADDR             0x40007000
#define TP_AUTO_FEED_ADDR            0x40007010
#define TP_REMAP_MCU_ADDR            0x40007000
#define TP_RELEASE_MCU_ADDR          0x40007000
#define TP_HOLD_MCU_VAL              0x12044000
#define TP_AUTO_FEED_VAL             0x0000925a
#define TP_REMAP_MCU_VAL             0x12044002
#define TP_RELEASE_MCU_VAL           0x12044003
#define CFG_ROM_ADDRESS              0
#define VID_PID_BACKUP_ADDR          (40 * 1024 + 0X10)
#define ADC_NUM_MAX                  16
#endif

/*ctp work staus*/
#define CTP_POINTING_WORK            0x00000000
#define CTP_READY_UPGRADE            (1 << 1)
#define CPT_UPGRAD_RUNING            (1 << 2)
#define CTP_SUSPEND_GATE             (1 << 16)
#define CTP_GUESTURE_GATE            (1 << 17)
#define CTP_PROXIMITY_GATE           (1 << 18)
#define CTP_GLOVE_GATE               (1 << 19)
#define CTP_ORIENTATION_GATE         (1 << 20)
#define ack_pointing_action(x)       (0 == (x & 0x1ffff))
#define set_status_pointing(x)       do{((x) = CTP_POINTING_WORK); kernel_log_d("set status pointing...\n");}while(0)
#define set_status_ready_upgrade(x)  do{((x) = CTP_READY_UPGRADE); kernel_log_d("set status before reset tp...\n");}while(0)
#define set_status_upgrade_run(x)    do{((x) = CPT_UPGRAD_RUNING); kernel_log_d("set status upgrade running...\n");}while(0)

#define is_proximity_activate(x)     (((x) & CTP_PROXIMITY_GATE) > 0)
#define enter_proximity_gate(x)      do{((x) |= CTP_PROXIMITY_GATE); kernel_log_d("enter proximity gate...\n");}while(0)
#define leave_proximity_gate(x)      do{((x) &= (~CTP_PROXIMITY_GATE)); kernel_log_d("leave proximity gate...\n");}while(0)

#define is_suspend_activate(x)       (((x) & CTP_SUSPEND_GATE) > 0)
#define enter_suspend_gate(x)        do{((x) |= CTP_SUSPEND_GATE); kernel_log_d("enter suspend gate...\n");}while(0)
#define leave_suspend_gate(x)        do{((x) &= (~CTP_SUSPEND_GATE)); kernel_log_d("leave suspend gate...\n");}while(0)

#define is_guesture_activate(x)      (((x) & CTP_GUESTURE_GATE) > 0)
#define enter_guesture_gate(x)       do{((x) |= CTP_GUESTURE_GATE); kernel_log_d("enter guesture gate...\n");}while(0)
#define leave_guesture_gate(x)       do{((x) &= (~CTP_GUESTURE_GATE)); kernel_log_d("leave guesture gate...\n");}while(0)

#define is_glove_activate(x)         (((x) & CTP_GLOVE_GATE) > 0)
#define enter_glove_gate(x)          do{((x) |= CTP_GLOVE_GATE); kernel_log_d("enter glove gate...\n");}while(0)
#define leave_glove_gate(x)          do{((x) &= (~CTP_GLOVE_GATE)); kernel_log_d("leave glove gate...\n");}while(0)

#define is_orientation_activate(x)   (((x) & CTP_ORIENTATION_GATE) > 0)
#define enter_orientation_gate(x)    do{((x) |= CTP_ORIENTATION_GATE); kernel_log_d("orientation horizontal...\n");}while(0)
#define leave_orientation_gate(x)    do{((x) &= (~CTP_ORIENTATION_GATE)); kernel_log_d("orientation vertical...\n");}while(0)

/*ctp function switch*/ 
//#define SUSPEND_FUNCTION_EN          (1 << 0)
#define GUESTURE_FUNCTION_EN         (1 << 1)
#define PROXIMITY_FUNCTION_EN        (1 << 2)
#define GLOVE_FUNCTION_EN            (1 << 3)
#define ESD_FUNCTION_EN              (1 << 4)
#define open_proximity_function(x)   do{((x) |= PROXIMITY_FUNCTION_EN); kernel_log_d("open proximity function...\n");}while(0)
#define close_proximity_function(x)  do{((x) &= (~PROXIMITY_FUNCTION_EN)); kernel_log_d("close proximity function...\n");}while(0)
#define is_proximity_function_en(x)  (((x) & PROXIMITY_FUNCTION_EN) > 0)
#define open_guesture_function(x)    do{((x) |= GUESTURE_FUNCTION_EN); kernel_log_d("open guesture function...\n");}while(0)
#define close_guesture_function(x)   do{((x) &= (~GUESTURE_FUNCTION_EN)); kernel_log_d("close guesture function...\n");}while(0)
#define is_guesture_function_en(x)   (((x) & GUESTURE_FUNCTION_EN) > 0)
#define open_glove_function(x)       do{((x) |= GLOVE_FUNCTION_EN); kernel_log_d("open glove function...\n");}while(0)
#define close_glove_function(x)      do{((x) &= (~GLOVE_FUNCTION_EN)); kernel_log_d("close glove function...\n");}while(0)
#define is_glove_function_en(x)      (((x) & GLOVE_FUNCTION_EN) > 0)
#define open_esd_function(x)         do{((x) |= ESD_FUNCTION_EN); kernel_log_d("open esd function...\n");}while(0)
#define close_esd_function(x)        do{((x) &= (~ESD_FUNCTION_EN)); kernel_log_d("close esd function...\n");}while(0)
#define is_esd_function_en(x)        (((x) & ESD_FUNCTION_EN) > 0)

enum reset_action { no_report_after_reset = 0, do_report_after_reset = 1 };
enum startup_action { only_sp_check = 0, check_backup_if_fail = 1 };

enum SEMI_DRV_ERR 
{
    SEMI_DRV_ERR_OK = 0,
    SEMI_DRV_ERR_HAL_IO,
    SEMI_DRV_ERR_NO_INIT,
    SEMI_DRV_ERR_TIMEOUT,
    SEMI_DRV_ERR_CHECKSUM,
    SEMI_DRV_ERR_RESPONSE,
    SEMI_DRV_INVALID_CMD,
    SEMI_DRV_INVALID_PARAM,
    SEMI_DRV_ERR_NOT_MATCH,
};

enum CMD_TYPE_ID
{
    CMD_NA              = 0x0f,
    CMD_IDENTITY        = 0x01,
    CMD_CTP_SSCAN       = 0x02,
    CMD_CTP_IOCTL       = 0x03,
    CMD_CTP_RST         = 0x10,
    CMD_SHORT_TST       = 0x21,
    CMD_CHK_CFG         = 0x26,
    CMD_DATA_SYNC       = 0x28,
    CMD_MEM_WR          = 0x30,
    CMD_MEM_RD          = 0x31,
    CMD_BSPR_RW         = 0x37,
};

struct hal_io_packet
{
    unsigned int   io_register;
    unsigned char  io_buffer[MAX_IO_BUFFER_LEN];
    unsigned short io_length;  
    void*          hal_adapter;
};

struct semi_touch_init_d
{
    unsigned int rawdata_addr;
    unsigned int differ_addr;
    unsigned int base_addr;
    unsigned int touch_addr;
    bool initialize_ok;
    bool dog_feed_flag;
    unsigned int ctp_run_status;
    unsigned int custom_function_en;

    int vkey_num;
    int vkey_evt_arr[MAX_VKEY_NUMBER];
    int vkey_dim_map[MAX_VKEY_NUMBER][4];
};

struct chsc_updfile_header {
    unsigned int sig;
    unsigned int extend;
    unsigned int n_cfg;
    unsigned int match;
    unsigned int len_cfg;
    unsigned int len_boot;
    unsigned int vlist[1];
};

typedef int (*hal_write_bytes)(struct hal_io_packet* ppacket);
typedef int (*hal_read_bytes)(struct hal_io_packet* ppacket);
struct hal_io_function
{
    hal_write_bytes hal_write_fun;
    hal_read_bytes  hal_read_fun;
    struct mutex    bus_lock;
    void* hal_param;
};

#define ASYN_WORK_MAX      3
#define typename(x)        #x
enum work_queue_t { work_queue_interrupt, work_queue_custom_work, work_queue_max };

struct work_wraper
{
    unsigned long uid;
    struct delayed_work  work;
};

struct work_quene_wraper
{
    int new_work_idx;
    struct workqueue_struct *work_queue[work_queue_max];
    struct work_wraper works_list[ASYN_WORK_MAX];
};


typedef struct sm_touch_dev
{
    struct device_node *nd;
    struct input_dev *input;
    struct i2c_client *client;
    struct semi_touch_init_d stc;
    struct hal_io_function hal;
    void   *chsc_nodes_dir;
    struct proc_dir_entry* proc_entry;
    struct work_quene_wraper asyn_work;
    int int_pin;
    int rst_pin;
    unsigned short fw_ver;
    unsigned int vid_pid;    //0xVID_PID_CFGVER
    //int irq_no;
    void *pv;
    struct gpio_desc *pwron;
}sm_touch_dev, *psm_touch_dev;

//cammand struct for mcap
struct m_ctp_cmd_std_t
{
    unsigned short chk; // 16 bit checksum
    unsigned short d0;  //data 0
    unsigned short d1;  //data 1
    unsigned short d2;  //data 2
    unsigned short d3;  //data 3
    unsigned short d4;  //data 4
    unsigned short d5;  //data 5

    unsigned char  id;   //offset 15
    unsigned char  tag;  //offset 16 
};

//response struct for mcap
struct m_ctp_rsp_std_t
{
    unsigned short chk; // 16 bit checksum
    unsigned short d0;  //data 0
    unsigned short d1;  //data 1
    unsigned short d2;  //data 2
    unsigned short d3;  //data 3
    unsigned short d4;  //data 4
    unsigned short d5;  //data 5

    unsigned char  cc;  //offset 15
    unsigned char  id;  //offset 16 
};

struct sync_context
{
    atomic_t atomic_sync_flag;

    unsigned int sync_addr;
    unsigned short sync_size;
};

struct apk_complex_data
{
    unsigned char stm_cmd_buffer[16];
    unsigned char stm_rsp_buffer[16];
    unsigned char stm_rdy_buffer[4];
    unsigned char stm_ctp_buffer[4];
    unsigned char stm_txrx_buffer[MAX_TX_RX_BUFF_LEN];
    unsigned short op_type;
    unsigned long  op_args;
    struct sync_context sync;
};

union rpt_point_t
{
    struct 
    {
        unsigned char x_l8;
        unsigned char y_l8;
        unsigned char z;
        unsigned char x_h4:4;
        unsigned char y_h4:4;
        unsigned char id:4;
        unsigned char event:4;
    }rp;
    unsigned char data[5];
};

typedef struct _rpt_content_t 
{
    unsigned char act;
    unsigned char num;
    union rpt_point_t points[15];
}rpt_content_t;

typedef struct _img_header_t 
{
    unsigned short fw_ver;
    unsigned short resv;
    unsigned int sig;
    unsigned int vid_pid;
    unsigned short raw_offet;
    unsigned short dif_offet;
}img_header_t;

struct bin_code_chain
{
    const unsigned char* bin_code_addr;
    unsigned int bin_code_len;
    struct bin_code_chain* next;
};

#endif //__HEAD_DEFINE__
