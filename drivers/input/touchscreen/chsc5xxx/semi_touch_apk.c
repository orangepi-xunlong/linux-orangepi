#include <linux/version.h>
#include <linux/proc_fs.h>
#include <linux/kthread.h>
#include <linux/delay.h>
#include "semi_touch_interface.h"

#if SEMI_TOUCH_APK_NODE_EN

struct apk_complex_data apk_comlex;

struct stm_mapped_memory
{
    unsigned short map_addr;
    unsigned char* map_buffer;
    unsigned short buff_cnt;
};

typedef int (*semi_touch_host_action)(struct apk_complex_data* apk_comlex_addr);
int semi_touch_sync_ctp(struct apk_complex_data* apk_comlex_addr);
int semi_touch_config(struct apk_complex_data* apk_comlex_addr);
int semi_touch_apk_reset(struct apk_complex_data* apk_comlex_addr);
int semi_touch_get_adcshort(struct apk_complex_data* apk_comlex_addr);
int semi_touch_read_write(struct apk_complex_data* apk_comlex_addr);

//#define SYNC_THREAD_PRIORITY                4

#define MAGIC_NO                            0x8000
#define IOCTL_OP_TYPE(x)                    (MAGIC_NO | x)
#define MTK_CTP_LINK                        IOCTL_OP_TYPE(0x0001)
#define MTK_CMD_BUF                         0xb400
#define MTK_RSP_BUF                         0xb440
#define MTK_FUNC_BUF                        0xc000
#define MTK_TXRX_BUF                        0xc400
#define MTK_DATA_RDY                        0xd209
#define kind_of_mcap_cmd_packet(p)          (((struct m_ctp_cmd_std_t*)(p))->tag == 0xE9)

#define is_sync_ready()  (1 == apk_comlex.stm_rdy_buffer[0])
#define set_sync_ready(x) (apk_comlex.stm_rdy_buffer[0] = (x))


struct action_wrap
{
    unsigned char cmd;
    semi_touch_host_action action_fun;
};

static struct stm_mapped_memory mapped_memory_array[] = 
{
    { .map_addr = MTK_CTP_LINK, .map_buffer = apk_comlex.stm_ctp_buffer,  .buff_cnt = 4, },
    { .map_addr = MTK_CMD_BUF,  .map_buffer = apk_comlex.stm_cmd_buffer,  .buff_cnt = 16 },
    { .map_addr = MTK_RSP_BUF,  .map_buffer = apk_comlex.stm_rsp_buffer,  .buff_cnt = 16 },
    { .map_addr = MTK_TXRX_BUF, .map_buffer = apk_comlex.stm_txrx_buffer, .buff_cnt = MAX_TX_RX_BUFF_LEN },
    { .map_addr = MTK_DATA_RDY, .map_buffer = apk_comlex.stm_rdy_buffer,  .buff_cnt = 4},
};

static const struct action_wrap array_action[] = 
{
    {CMD_CTP_RST, semi_touch_apk_reset},
    {CMD_MEM_WR, semi_touch_memory_write},
    {CMD_MEM_RD, semi_touch_memory_read},
    {CMD_DATA_SYNC, semi_touch_sync_ctp},
    {CMD_CHK_CFG, semi_touch_config},
    {CMD_BSPR_RW, semi_touch_read_write},
    {CMD_SHORT_TST, semi_touch_get_adcshort},
};

int semi_touch_config(struct apk_complex_data* apk_comlex_addr)
{
    return 0;
}

int semi_touch_read_write(struct apk_complex_data* apk_comlex_addr)
{
    int ret = SEMI_DRV_ERR_OK, write_len = 0, read_len = 0;
    struct m_ctp_cmd_std_t *ptr_cmd = (struct m_ctp_cmd_std_t*)apk_comlex_addr->stm_cmd_buffer;
    struct m_ctp_rsp_std_t *ptr_rsp = (struct m_ctp_rsp_std_t*)apk_comlex_addr->stm_rsp_buffer;
    unsigned char* txrx_buffer = apk_comlex_addr->stm_txrx_buffer;
    unsigned int addr = swab32(*(unsigned int*)txrx_buffer);

    write_len = ptr_cmd->d0;
    read_len  = ptr_cmd->d1;

    if(ptr_cmd->d2 == caculate_checksum_u816(txrx_buffer, write_len))
    {
        if(read_len > 0)
        {
            ret = semi_touch_read_bytes(addr, txrx_buffer, read_len);
            check_return_if_fail(ret, NULL);

            ptr_rsp->d0 =caculate_checksum_u816(txrx_buffer, read_len);
        }
        else
        {
            ret = semi_touch_write_bytes(addr, txrx_buffer + 4, write_len - 4);
            check_return_if_fail(ret, NULL);
        }
    }

    return ret;
}

int semi_touch_apk_reset(struct apk_complex_data* apk_comlex_addr)
{
    return semi_touch_reset(do_report_after_reset);
}

int semi_touch_get_adcshort(struct apk_complex_data* apk_comlex_addr)
{
    int u32_para_buff[4];
    int ret = SEMI_DRV_ERR_OK, index = 0;
    struct m_ctp_rsp_std_t *ptr_rsp = (struct m_ctp_rsp_std_t*)apk_comlex_addr->stm_rsp_buffer;

    ret = semi_touch_run_ram_code(1/*RAM_CODE_SHORT_DATA_SHARE*/);
    check_return_if_fail(ret, NULL);

    for(index = 0; index < 10; index++)
    {
        mdelay(20);
        ret = semi_touch_read_bytes(0x20000000, (unsigned char *)u32_para_buff, 12);
        check_return_if_fail(ret, NULL);
        if(0x45000000 == u32_para_buff[0])
        {
            break;
        }
    }
    if(0x45000000 == u32_para_buff[0])
    {
        mdelay(10);
        ret = semi_touch_read_bytes(u32_para_buff[1], apk_comlex_addr->stm_txrx_buffer, ADC_NUM_MAX << 1);
        check_return_if_fail(ret, NULL);

        u32_para_buff[3] = caculate_checksum_ex(apk_comlex_addr->stm_txrx_buffer, ADC_NUM_MAX << 1);
        if(u32_para_buff[3] == u32_para_buff[2])
        {
            ptr_rsp->d0 = (unsigned short)(u32_para_buff[3]);
            ptr_rsp->d1 = (unsigned short)(u32_para_buff[3] >> 16);
            ret = SEMI_DRV_ERR_OK;
        }
        else
        {
            ret = -SEMI_DRV_ERR_CHECKSUM;
            check_return_if_fail(ret, NULL);
        }
    }
    else
    {
        ret = -SEMI_DRV_ERR_RESPONSE;
        check_return_if_fail(ret, NULL);
    }
    
    return ret;
}

void semi_touch_apk_work_fun(struct work_struct *work)
{
    int ret = 0;
    struct m_ctp_cmd_std_t cmd_send_tp;
    struct m_ctp_rsp_std_t ack_from_tp;
    unsigned short sync_once = MAX_TX_RX_BUFF_LEN;

    sync_once = min(sync_once, apk_comlex.sync.sync_size);

    if(is_sync_ready()) return;

    do
    {
        cmd_send_tp.id = CMD_CTP_SSCAN;
        cmd_send_tp.d0 = 0;

        cmd_send_to_tp(&cmd_send_tp, &ack_from_tp, 200);
        //check_break_if_fail(ret, NULL);

        ret = semi_touch_read_bytes(apk_comlex.sync.sync_addr, apk_comlex.stm_txrx_buffer, sync_once);
        check_break_if_fail(ret, NULL);

        cmd_send_tp.id = CMD_CTP_SSCAN;
        cmd_send_tp.d0 = 1;
        cmd_send_to_tp(&cmd_send_tp, &ack_from_tp, 200);
    }while(0);

    atomic_set(&apk_comlex.sync.atomic_sync_flag, 0);
    set_sync_ready(1);
}

int semi_touch_sync_ctp(struct apk_complex_data* apk_comlex_addr)
{
    int ret = SEMI_DRV_ERR_OK;
    struct m_ctp_cmd_std_t *ptr_cmd = (struct m_ctp_cmd_std_t*)apk_comlex_addr->stm_cmd_buffer;
    
    set_sync_ready(0);
    apk_comlex_addr->sync.sync_size = ptr_cmd->d1;
    apk_comlex_addr->sync.sync_addr = (ptr_cmd->d5 << 16) | ptr_cmd->d0;
    atomic_set(&apk_comlex.sync.atomic_sync_flag, 1);

    ret = semi_touch_queue_asyn_work(work_queue_custom_work, semi_touch_apk_work_fun, 0);

    kernel_log_d("host send sync command...\n");

    return ret;
}

static void should_wait_for_sync_task(unsigned char cmd)
{
    int retry = 100;
    if(cmd == CMD_DATA_SYNC) return;

    //max wait = 100 * 1 = 100ms
    for(retry = 0; retry < 100; retry++)
    {
        if(0 == atomic_read(&apk_comlex.sync.atomic_sync_flag))
        {
            break;
        }
        mdelay(1);
    }
}

static int host_command_parse_and_respons(void)
{
    int ret = -SEMI_DRV_INVALID_CMD;
    unsigned short index = 0, check_sum = 0, cmd_invoked = 0;
    struct m_ctp_cmd_std_t* host_cmd = (struct m_ctp_cmd_std_t*)apk_comlex.stm_cmd_buffer;
    struct m_ctp_rsp_std_t* driv_rsp = (struct m_ctp_rsp_std_t*)apk_comlex.stm_rsp_buffer;

    if(!kind_of_mcap_cmd_packet(host_cmd))  return -EINVAL;

    check_sum = caculate_checksum_u16((unsigned short*)host_cmd, sizeof(struct m_ctp_cmd_std_t));
    if(0 != check_sum)  return -SEMI_DRV_ERR_CHECKSUM;

    for(index = 0; index < sizeof(array_action) / sizeof(struct action_wrap); index++)
    {
        if(array_action[index].cmd == host_cmd->id)
        {
            cmd_invoked = 1;
            should_wait_for_sync_task(host_cmd->id);
            ret = array_action[index].action_fun(&apk_comlex);
            break;
        }
    }

    if(!cmd_invoked)
    {
        should_wait_for_sync_task(host_cmd->id);
        ret = cmd_send_to_tp(host_cmd, driv_rsp, 200);
    }

    if(SEMI_DRV_ERR_OK == ret)
    {
        driv_rsp->id = host_cmd->id;
        driv_rsp->cc = 0;
    }
    else
    {
        driv_rsp->cc = -SEMI_DRV_ERR_RESPONSE;
    }
    driv_rsp->chk = 1 + ~caculate_checksum_u16((unsigned short*)&driv_rsp->d0, sizeof(struct m_ctp_rsp_std_t) - 2);
        
    return ret;
}

static ssize_t semi_touch_proc_write(struct file* fp, const char __user *buff, size_t len, loff_t* data)
{
    int ret = 0, index = 0;

    //kernel_log_d("proc op_type = %x, ap_addr = %x, write datalen = %d\r\n", apk_comlex.op_type, (unsigned int)apk_comlex.op_args, (int)len);
    
    if(len <= 0)  return -EINVAL;

    for(index = 0; index < sizeof(mapped_memory_array) / sizeof(struct stm_mapped_memory); index++)
    {
        if(apk_comlex.op_type != mapped_memory_array[index].map_addr)
            continue;

        //kernel_log_d("len = %d, apk_comlex.op_args = %d, mapped_memory_array[index].buff_cnt = %d\r\n",
        //    len, apk_comlex.op_args, mapped_memory_array[index].buff_cnt);

        if((len + apk_comlex.op_args) > mapped_memory_array[index].buff_cnt) 
            ret = -EINVAL;
        check_break_if_fail(ret, NULL);

        ret = copy_from_user(mapped_memory_array[index].map_buffer + apk_comlex.op_args, buff, len);
        check_return_if_fail(ret, NULL);

        if(MTK_CMD_BUF == apk_comlex.op_type)
        {
            ret = host_command_parse_and_respons();
            check_break_if_fail(ret, NULL);
        }
        else if(MTK_DATA_RDY == apk_comlex.op_type)
        {
            atomic_set(&apk_comlex.sync.atomic_sync_flag, 1);
            semi_touch_queue_asyn_work(work_queue_custom_work, semi_touch_apk_work_fun, 0);
        }
    }

    return ret;
}

static ssize_t semi_touch_proc_read(struct file* fp, char __user *buff, size_t len, loff_t* data)
{
    int ret = 0, index = 0;
    
    if(len <= 0)  return -EINVAL;

    //kernel_log_d("proc op_type = %x, ap_addr = %x, read datalen = %d\r\n", apk_comlex.op_type, (unsigned int)apk_comlex.op_args, (int)len);

    for(index = 0; index < sizeof(mapped_memory_array) / sizeof(struct stm_mapped_memory); index++)
    {
        if(apk_comlex.op_type != mapped_memory_array[index].map_addr)
            continue;

        if((len + apk_comlex.op_args) > mapped_memory_array[index].buff_cnt) 
            ret = -EINVAL;
        check_break_if_fail(ret, NULL);

        ret = copy_to_user(buff, mapped_memory_array[index].map_buffer + apk_comlex.op_args, len);
        check_return_if_fail(ret, NULL);

        if(data) *data = len;
    }

    return ret;
}

static long semi_touch_ioctl(struct file* fp, unsigned int op_type, unsigned long args)
{
    apk_comlex.op_type = (unsigned short)op_type;
    apk_comlex.op_args = args;

    //kernel_log_d("proc op_type = %x, args = %d\r\n", op_type, (int)args);

    return 0;
}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 10, 0))
static const struct proc_ops semi_touch_proc_opts = 
{
    .proc_write = semi_touch_proc_write,
    .proc_read  = semi_touch_proc_read,
    .proc_ioctl = semi_touch_ioctl,
    #ifdef CONFIG_COMPAT
    .proc_compat_ioctl = semi_touch_ioctl,
    #endif
};
#endif

#endif//SEMI_TOUCH_APK_NODE_EN

int semi_touch_create_apk_proc(struct sm_touch_dev* st_dev)
{
    int ret = 0;

#if SEMI_TOUCH_APK_NODE_EN
    semi_touch_create_work_queue(work_queue_custom_work, typename(work_queue_custom_work));
    
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 10, 0))
    st_dev->proc_entry = proc_create(SEMI_TOUCH_PROC_NAME, 0777, NULL, &semi_touch_proc_opts);
    check_return_if_zero(st_dev->proc_entry, NULL);
#else
    st_dev->proc_entry = create_proc_entry(SEMI_TOUCH_PROC_NAME, 0777, NULL);
    check_return_if_zero(st_dev->proc_entry, NULL);
    //st_dev->proc_entry->write_proc = semi_touch_proc_write;
    //st_dev->proc_entry->read_proc = semi_touch_proc_read;
#endif    

    //apk_comlex.sync.sync_task = NULL;
    apk_comlex.sync.sync_addr = 0;
    apk_comlex.sync.sync_size = 0;
    apk_comlex.stm_ctp_buffer[0] = 0x5C;
    atomic_set(&apk_comlex.sync.atomic_sync_flag, 0);
#endif //SEMI_TOUCH_APK_NODE_EN
        
    return ret;
}
int semi_touch_remove_apk_proc(struct sm_touch_dev* st_dev)
{
    int ret = 0;

#if SEMI_TOUCH_APK_NODE_EN

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 10, 0))
    proc_remove(st_dev->proc_entry);
#else
    remove_proc_entry(SEMI_TOUCH_PROC_NAME, NULL);
#endif

#endif //SEMI_TOUCH_APK_NODE_EN

    return ret;
}

