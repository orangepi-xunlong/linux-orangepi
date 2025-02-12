
#include <linux/gpio.h>
#include <linux/delay.h>
#include "semi_touch_interface.h"

enum HAL_IO_DIR { HAL_IO_WRITE, HAL_IO_READ, };

int semi_touch_reset(enum reset_action action)
{
    int ret = 0, self_check = 0;
    //struct i2c_client *client = st_dev.client;

    //ptr_cmd = NULL;
    set_status_ready_upgrade(st_dev.stc.ctp_run_status);
    //disable_irq(client->irq);

    semi_io_pin_low(st_dev.rst_pin);
    msleep(30);
    semi_io_pin_high(st_dev.rst_pin);

    mdelay(5);
    semi_touch_write_bytes(0x20000018, (unsigned char*)&self_check, 4);

    if(do_report_after_reset == action)
    {
        set_status_pointing(st_dev.stc.ctp_run_status);
        //enable_irq(client->irq);
    }

    return ret;
}

int semi_touch_suspend_ctrl(unsigned char en)
{
    int ret = 0;
    struct m_ctp_cmd_std_t cmd_send_tp;
    memset(&cmd_send_tp, 0, sizeof(cmd_send_tp));
    //struct m_ctp_rsp_std_t ack_from_tp;

    if(en)
    {
        cmd_send_tp.id = CMD_CTP_IOCTL;
        cmd_send_tp.d0 = 0x05;
        cmd_send_tp.d1 = en;

        ret = cmd_send_to_tp_no_check(&cmd_send_tp);
        check_return_if_fail(ret, NULL);
    }
    else
    {
        semi_touch_reset_and_detect();
    }

    if(en)
        enter_suspend_gate(st_dev.stc.ctp_run_status);
    else
        leave_suspend_gate(st_dev.stc.ctp_run_status);

    return ret;
}

int semi_touch_proximity_switch(unsigned char en)
{
    int ret = 0;

    struct m_ctp_cmd_std_t cmd_send_tp;
    struct m_ctp_rsp_std_t ack_from_tp;
    memset(&cmd_send_tp, 0, sizeof(cmd_send_tp));

    if(!is_proximity_function_en(st_dev.stc.custom_function_en)) 
        return ret;

    cmd_send_tp.id = CMD_CTP_IOCTL;
    cmd_send_tp.d0 = 0x11;
    cmd_send_tp.d1 = en;

    ret = cmd_send_to_tp(&cmd_send_tp, &ack_from_tp, 200);
    check_return_if_fail(ret, NULL);

    if(en)
        enter_proximity_gate(st_dev.stc.ctp_run_status);
    else
        leave_proximity_gate(st_dev.stc.ctp_run_status);
    
    return ret;
}

int semi_touch_glove_switch(unsigned char en)
{
    int ret = 0;
    struct m_ctp_cmd_std_t cmd_send_tp;
    struct m_ctp_rsp_std_t ack_from_tp;
    memset(&cmd_send_tp, 0, sizeof(cmd_send_tp));

    if(!is_glove_function_en(st_dev.stc.custom_function_en)) 
        return ret;

    cmd_send_tp.id = CMD_CTP_IOCTL;
    cmd_send_tp.d0 = 0x10;
    cmd_send_tp.d1 = en;

    ret = cmd_send_to_tp(&cmd_send_tp, &ack_from_tp, 200);
    check_return_if_fail(ret, NULL);

    if(en)
        enter_glove_gate(st_dev.stc.ctp_run_status);
    else
        leave_glove_gate(st_dev.stc.ctp_run_status);

    return ret;
}

int semi_touch_guesture_switch(unsigned char en)
{
    int ret = 0;
    struct m_ctp_cmd_std_t cmd_send_tp;
    struct m_ctp_rsp_std_t ack_from_tp;
    memset(&cmd_send_tp, 0, sizeof(cmd_send_tp));

    if(!is_guesture_function_en(st_dev.stc.custom_function_en)) 
        return ret;

    cmd_send_tp.id = CMD_CTP_IOCTL;
    cmd_send_tp.d0 = 0x02;
    cmd_send_tp.d1 = en;

    ret = cmd_send_to_tp(&cmd_send_tp, &ack_from_tp, 200);
    check_return_if_fail(ret, NULL);

    if(en)
        enter_guesture_gate(st_dev.stc.ctp_run_status);
    else
        leave_guesture_gate(st_dev.stc.ctp_run_status);

    return ret;
}

int semi_touch_orientation_switch(unsigned char en)
{
    int ret = 0;
    struct m_ctp_cmd_std_t cmd_send_tp;
    struct m_ctp_rsp_std_t ack_from_tp;
    memset(&cmd_send_tp, 0, sizeof(cmd_send_tp));

    cmd_send_tp.id = CMD_CTP_IOCTL;
    cmd_send_tp.d0 = 0x2A;
    cmd_send_tp.d1 = en;

    ret = cmd_send_to_tp(&cmd_send_tp, &ack_from_tp, 200);
    check_return_if_fail(ret, NULL);

    if(en)
        enter_orientation_gate(st_dev.stc.ctp_run_status);
    else
        leave_orientation_gate(st_dev.stc.ctp_run_status);

    return ret;
}

int semi_touch_heart_beat(void)
{
    int ret = 0;
    unsigned int regdata = 0;
    unsigned int ctp_stc_backup = st_dev.stc.ctp_run_status;

    if(is_suspend_activate(st_dev.stc.ctp_run_status)) 
        return ret;
    if(is_guesture_activate(st_dev.stc.ctp_run_status))
        return ret;
    if(!is_esd_function_en(st_dev.stc.custom_function_en))
        return ret;

    //slave report 0xFD to detect if ic is still alive
    if(semi_touch_check_watch_dog_feed(st_dev.stc.dog_feed_flag))
    {
        //dog feed in time
    }
    else
    {
        ret = semi_touch_read_bytes(0x20000018, (unsigned char*)&regdata, 4);
        if(SEMI_DRV_ERR_OK != ret)
        {
            //reset tp + iic detected
            ret = semi_touch_reset_and_detect();
            check_return_if_fail(ret, NULL);  

            if(is_guesture_activate(ctp_stc_backup))
            {
                ret = semi_touch_guesture_switch(1);
                check_return_if_fail(ret, NULL);
            }
            if(is_glove_activate(ctp_stc_backup))
            {
                ret = semi_touch_glove_switch(1);
                check_return_if_fail(ret, NULL);
            }
            if(is_proximity_activate(ctp_stc_backup))
            {
                ret = semi_touch_proximity_switch(1);
                check_return_if_fail(ret, NULL);
            }
        }
    }

    semi_touch_reset_watch_dog(st_dev.stc.dog_feed_flag);
    
    return ret;
}

int semi_touch_write_bytes(unsigned int reg, const unsigned char* buffer, unsigned short len)
{
    int ret = SEMI_DRV_ERR_OK;
    unsigned int addr = reg;
    unsigned short once;
    static struct hal_io_packet packet;
    const unsigned short max_len = MAX_IO_BUFFER_LEN;
    mutex_lock(&st_dev.hal.bus_lock);

    while(len > 0)
    {
        once = min(len, max_len);
        packet.io_register = swab32(addr);
        memcpy(packet.io_buffer, buffer, once);
        packet.io_length = once;
        packet.hal_adapter = st_dev.hal.hal_param;

        ret = (*st_dev.hal.hal_write_fun)(&packet);
        check_break_if_fail(ret, NULL);

        addr += once;
        buffer += once;
        len -= once;
    }

    mutex_unlock(&st_dev.hal.bus_lock);
    
    return ret >= 0 ? SEMI_DRV_ERR_OK : ret;
}

int semi_touch_read_bytes(unsigned int reg, unsigned char* buffer, unsigned short len)
{
    int ret = SEMI_DRV_ERR_OK;
    unsigned int addr = reg;
    unsigned short once;
    static struct hal_io_packet packet;
    const unsigned short max_len = MAX_IO_BUFFER_LEN;
    mutex_lock(&st_dev.hal.bus_lock);

    while(len > 0)
    {
        once = min(len, max_len);
        packet.io_register = swab32(addr);
        packet.io_length = once;
        packet.hal_adapter = st_dev.hal.hal_param;

        ret = (*st_dev.hal.hal_read_fun)(&packet);
        check_break_if_fail(ret, NULL);
        memcpy(buffer, packet.io_buffer, once);

        addr += once;
        buffer += once;
        len -= once;
    }

    mutex_unlock(&st_dev.hal.bus_lock);
    
    return ret >= 0 ? SEMI_DRV_ERR_OK : ret;
}

int cmd_send_to_tp(struct m_ctp_cmd_std_t *ptr_cmd, struct m_ctp_rsp_std_t *ptr_rsp, const int delay)
{
    int ret = -SEMI_DRV_ERR_HAL_IO;
    unsigned int retry = 0, cmd_rsp_ok = 0;
    int once_delay = delay;

    ptr_cmd->tag = 0xE9;
    ptr_cmd->chk = 1 + ~caculate_checksum_u16((unsigned short*)&ptr_cmd->d0, sizeof(struct m_ctp_cmd_std_t) - 2);
    ret = semi_touch_write_bytes(TP_CMD_BUFF_ADDR, (unsigned char*)ptr_cmd, sizeof(struct m_ctp_cmd_std_t));
    check_return_if_fail(ret, NULL);

    while(retry++ < 40)
    {
        once_delay = delay;
        do{ udelay(1000); once_delay-= 1000; }while(once_delay > 0);
        ret = semi_touch_read_bytes(TP_RSP_BUFF_ADDR, (unsigned char*)ptr_rsp, sizeof(struct m_ctp_rsp_std_t));
        check_return_if_fail(ret, NULL);

        if(ptr_cmd->id != ptr_rsp->id)
        {
            continue;
        }

        if(!caculate_checksum_u16((unsigned short*)ptr_rsp, sizeof(struct m_ctp_rsp_std_t)))
        {
            if(0 == ptr_rsp->cc)
            {
                cmd_rsp_ok = 1; //success
            }
            break;
        }
    }

    if(!cmd_rsp_ok) ret = -SEMI_DRV_ERR_TIMEOUT;

    return ret;
}

int cmd_send_to_tp_no_check(struct m_ctp_cmd_std_t *ptr_cmd)
{
    int ret = -SEMI_DRV_ERR_HAL_IO;

    ptr_cmd->tag = 0xE9;
    ptr_cmd->chk = 1 + ~caculate_checksum_u16((unsigned short*)&ptr_cmd->d0, sizeof(struct m_ctp_cmd_std_t) - 2);
    ret = semi_touch_write_bytes(TP_CMD_BUFF_ADDR, (unsigned char*)ptr_cmd, sizeof(struct m_ctp_cmd_std_t));
    check_return_if_fail(ret, NULL);

    return ret;
}


int read_and_report_touch_points(unsigned char *readbuffer, unsigned short len)
{
    int ret = 0, retry = 0;

    if(!st_dev.stc.initialize_ok)  
        ret = -SEMI_DRV_ERR_NO_INIT;
    check_return_if_fail(ret, NULL);
    
    if(is_guesture_activate(st_dev.stc.ctp_run_status))
    {
        for(retry = 0; retry < 30; retry++)
        {
            msleep(10);
            ret = semi_touch_read_bytes(st_dev.stc.touch_addr, readbuffer, len);
            if(ret == SEMI_DRV_ERR_OK) break;

            kernel_log_d("guesture read, retry = %d\n", retry);
        }
    }
    else
    {
        ret = semi_touch_read_bytes(st_dev.stc.touch_addr, readbuffer, len);
        check_return_if_fail(ret, NULL);
    }

    return ret;
}

int semi_touch_mode_init(struct sm_touch_dev *st_dev)
{
    int ret = -SEMI_DRV_ERR_HAL_IO;
    unsigned char bootCheckOk = 0;
    unsigned char readbuffer[8] = {0};

    semi_touch_start_up_check(&bootCheckOk, only_sp_check);

    if(!bootCheckOk)
    {
        //reset tp + iic detected
        semi_touch_reset_and_detect();
        semi_touch_start_up_check(&bootCheckOk, only_sp_check);

        if(!bootCheckOk)
        {
            ret = -SEMI_DRV_ERR_TIMEOUT;
            check_return_if_fail(ret, NULL);
        }
    }

    ret = semi_touch_read_bytes(0x20000020, readbuffer, sizeof(readbuffer));
    check_return_if_fail(ret, NULL);

    st_dev->chsc_nodes_dir = NULL;
    st_dev->stc.initialize_ok = true;
    set_status_pointing(st_dev->stc.ctp_run_status);
    st_dev->stc.rawdata_addr = 0x20000000 + ( (unsigned int)(readbuffer[0x01] << 8) + readbuffer[0x00] );
    st_dev->stc.differ_addr = 0x20000000 + ( (unsigned int)(readbuffer[0x03] << 8) + readbuffer[0x02] );
    st_dev->stc.base_addr = 0x20000000 + ( (unsigned int)(readbuffer[0x05] << 8) + readbuffer[0x04] );
    st_dev->stc.touch_addr = 0x20000000 + 0x2c;

    semi_touch_create_work_queue(work_queue_custom_work, typename(work_queue_custom_work));

    return ret;
}

/*
    Judge whether it's Chipsemi by IIC, if YES return SEMI_DRV_ERR_OK.
*/
int semi_touch_device_prob(void)
{
    int retry, regdata;
    int ret = -SEMI_DRV_ERR_HAL_IO;

    msleep(10);
    for(retry = 0; retry < 3; retry++)
    {
        ret = semi_touch_read_bytes(0x20000018, (unsigned char*)&regdata, 4);
        if(ret == SEMI_DRV_ERR_OK)
        {
            break;
        }
    }

    //regdata = 0xff0 | (1<<17);
    //semi_touch_write_bytes(0x4000700c, (unsigned char*)&regdata, 4);
    
    return (SEMI_DRV_ERR_OK == ret) ? SEMI_DRV_ERR_OK : -SEMI_DRV_ERR_HAL_IO;
}
int semi_touch_reset_and_detect(void)
{
    int ret = -SEMI_DRV_ERR_HAL_IO;

    semi_touch_reset(no_report_after_reset);

    ret = semi_touch_device_prob();

    msleep(50);
    set_status_pointing(st_dev.stc.ctp_run_status);

    return ret;
}
int semi_touch_start_up_check(unsigned char* checkOK, unsigned char opt)
{
    int ret = -EINVAL, retry = 0; 
    img_header_t image_header;

    st_dev.fw_ver = st_dev.vid_pid = 0;

    for(retry = 0; retry < 10; retry++)
    {
        ret = semi_touch_read_bytes(0x20000014, (unsigned char*)&image_header, sizeof(image_header));
        check_return_if_fail(ret, NULL);

        if(image_header.sig == 0x43534843) //"CHSC"
        { 
            *checkOK = 1;
            st_dev.fw_ver = image_header.fw_ver;
            st_dev.vid_pid = image_header.vid_pid;
            break;
        }
        else if(image_header.sig == 0x4F525245) //boot self check fail
        { 
            *checkOK = 0;
            kernel_log_d("boot self check fail, upgrade is needed\r\n");
            break;
        }
        else //may be impossible; no firmware? 
        { 
            *checkOK = 0;
            kernel_log_d("retry-%d, firmware is not ready\r\n", retry);
        }

        msleep(10);
    }

    if(0 == *checkOK && check_backup_if_fail == opt)
    {
        ret = semi_touch_get_backup_pid(&st_dev.vid_pid);
        check_return_if_fail(ret, NULL);
    }

    return ret;
}
int semi_touch_destroy_work_queue(void) 
{ 
    int index = 0;

    for(index = 0; index < st_dev.asyn_work.new_work_idx; index++)
    {
        cancel_delayed_work(&st_dev.asyn_work.works_list[index].work);
    }

    for(index = 0; index < work_queue_max; index++)
    {
        if(st_dev.asyn_work.work_queue[index])
        {
            destroy_workqueue(st_dev.asyn_work.work_queue[index]);
            st_dev.asyn_work.work_queue[index] = NULL;
        }
    }

    st_dev.asyn_work.new_work_idx = 0;

    return SEMI_DRV_ERR_OK;
}
int semi_touch_create_work_queue(enum work_queue_t queue_type, const char* queue_name)
{
    if(NULL == st_dev.asyn_work.work_queue[queue_type]) 
    { 
        st_dev.asyn_work.work_queue[queue_type] = create_singlethread_workqueue(queue_name); 
        check_return_if_zero(st_dev.asyn_work.work_queue[queue_type], NULL); 
    }
    else
    {
        //aready exist!!!
    }

    return SEMI_DRV_ERR_OK;
}
int semi_touch_queue_asyn_work(enum work_queue_t queue_type, work_func_t work_func, int ms)
{
    int ret = 0, work_index = 0;
    struct delayed_work *workimp = NULL;

    check_return_if_zero(st_dev.asyn_work.work_queue[queue_type], NULL);

    for(work_index = 0; work_index < st_dev.asyn_work.new_work_idx; work_index++)
    {
        if((unsigned long)work_func == st_dev.asyn_work.works_list[work_index].uid)
        {
            workimp = &st_dev.asyn_work.works_list[work_index].work;
            break;
        }
    }

    if(NULL == workimp)
    {
        if(st_dev.asyn_work.new_work_idx < ASYN_WORK_MAX)
        {
            work_index = st_dev.asyn_work.new_work_idx++;
            workimp = &st_dev.asyn_work.works_list[work_index].work;
            st_dev.asyn_work.works_list[work_index].uid = (unsigned long)work_func;
            INIT_DELAYED_WORK(workimp, work_func);
        }
    }

    if(NULL != workimp)
    {
        queue_delayed_work(st_dev.asyn_work.work_queue[queue_type], workimp, msecs_to_jiffies(ms));
    }
    else
    {
        ret = -SEMI_DRV_ERR_NOT_MATCH;
        check_return_if_fail(ret, NULL);
    }

    return ret;
}