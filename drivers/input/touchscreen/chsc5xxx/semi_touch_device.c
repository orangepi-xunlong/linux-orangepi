#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/cdev.h>
//#include <linux/ide.h>
#include <linux/io.h>
#include <linux/gpio.h>
#include <linux/of_gpio.h>
#include <linux/irq.h>
#include <linux/of_irq.h>
#include <linux/delay.h>
#include <linux/input.h>
#include <linux/input/mt.h>
#include <linux/irq.h>
#include <linux/of_irq.h>
#include <linux/kthread.h>
#include <linux/i2c.h>

#include "platform.h"
#include "semi_touch_interface.h"

struct sm_touch_dev st_dev = 
{
    .stc.initialize_ok = false, 
    .stc.ctp_run_status = 0,
    .stc.custom_function_en = 0,
};

static int input_device_init(struct sm_touch_dev *st_dev)
{
    int ret = 0;
    struct i2c_client *client = st_dev->client;

    st_dev->input = devm_input_allocate_device(&st_dev->client->dev);
    check_return_if_fail(st_dev->input, NULL);

    st_dev->input->name = client->name;
    st_dev->input->id.bustype = BUS_I2C;
    st_dev->input->dev.parent = &client->dev;

    st_dev->input->evbit[0] = BIT_MASK(EV_SYN) | BIT_MASK(EV_KEY) | BIT_MASK(EV_ABS);
    st_dev->input->keybit[BIT_WORD(BTN_TOUCH)] = BIT_MASK(BTN_TOUCH);
    __set_bit(INPUT_PROP_DIRECT, st_dev->input->propbit);

    //input_set_abs_params(st_dev->input, ABS_X, 0, SEMI_TOUCH_SOLUTION_X, 0, 0);
    //input_set_abs_params(st_dev->input, ABS_Y, 0, SEMI_TOUCH_SOLUTION_Y, 0, 0);
    input_set_abs_params(st_dev->input, ABS_MT_POSITION_X, 0, SEMI_TOUCH_SOLUTION_X, 0, 0);
    input_set_abs_params(st_dev->input, ABS_MT_POSITION_Y, 0, SEMI_TOUCH_SOLUTION_Y, 0, 0);
    input_set_abs_params(st_dev->input, ABS_MT_WIDTH_MAJOR, 0, 255, 0, 0);
    input_set_abs_params(st_dev->input, ABS_MT_TOUCH_MAJOR, 0, 255, 0, 0);

#if MULTI_PROTOCOL_TYPE_A == MULTI_PROTOCOL_TYPE
    input_set_abs_params(st_dev->input, ABS_MT_TRACKING_ID, 0, 255, 0, 0);
#elif MULTI_PROTOCOL_TYPE_B == MULTI_PROTOCOL_TYPE
    input_mt_init_slots(st_dev->input, SEMI_TOUCH_MAX_POINTS, INPUT_MT_DIRECT /*1*/);
#endif

    ret = input_register_device(st_dev->input);
    check_return_if_fail(ret, NULL);

    return 0;
}

static int input_device_deinit(void)
{
    int ret = 0;

    ret = semi_touch_i2c_i2c_exit();

    if(st_dev.input)
    {
        input_unregister_device(st_dev.input);  
    }

    return ret;  
}

bool semi_touch_vkey_handled(bool pointed, unsigned int x, unsigned int y)
{
#if SEMI_TOUCH_VKEY_MAPPING
    int key = 0;
    for(key = 0; key < st_dev.stc.vkey_num; key++)
    {
        //kernel_log_d("x = %d, y = %d, key_x = %d, key_y = %d\n", x, y, st_dev.stc.vkey_dim_map[key][0], st_dev.stc.vkey_dim_map[key][1]);
        if(x == st_dev.stc.vkey_dim_map[key][0] && y == st_dev.stc.vkey_dim_map[key][1])
        {
            input_report_key(st_dev.input, st_dev.stc.vkey_evt_arr[key], pointed ? 1 : 0);
            return true;
        }
    }
#endif

    return false;
}

#define SEMI_TOUCH_RDBUF_SIZE	0x100
static irqreturn_t semi_touch_irq_handler_imp(int irq, void *p)
{
    bool pointed = 0;
    int  index = 0, pointNum = 0/*, ioLevel = 0*/;
    union rpt_point_t* ppt;
    const unsigned char report_size = ((SEMI_TOUCH_MAX_POINTS * 5 + 2) + 3) & 0xfc;
    unsigned char readbuffer[SEMI_TOUCH_RDBUF_SIZE];

    //kernel_log_d("st_dev.stc.ctp_run_status = 0x%x\n", st_dev.stc.ctp_run_status);

    //ioLevel = semi_io_pin_level(st_dev.int_pin);
    //if(1 == ioLevel) return IRQ_RETVAL(IRQ_HANDLED);

    if(!ack_pointing_action(st_dev.stc.ctp_run_status)) return IRQ_RETVAL(IRQ_HANDLED);

    if((int)read_and_report_touch_points(readbuffer, report_size) >= 0)
    {
        //int & iic is ok, dog feed
        semi_touch_watch_dog_feed(st_dev.stc.dog_feed_flag);

        pointNum = (readbuffer[1] & 0x0f);
        ppt = (union rpt_point_t*)&readbuffer[2];

        if(0xFD == readbuffer[0]) 
        {
            //firmware report heatbeat packet
            return IRQ_RETVAL(IRQ_HANDLED);
        }
        else if(0xFC == readbuffer[0] && semi_touch_proximity_report(readbuffer[1]))
        {
            return IRQ_RETVAL(IRQ_HANDLED);
        }
        else if(0xFE == readbuffer[0] && semi_touch_gesture_report(readbuffer[1]))
        {
            return IRQ_RETVAL(IRQ_HANDLED);
        }

#if MULTI_PROTOCOL_TYPE_A == MULTI_PROTOCOL_TYPE
        for(index = 0; index < SEMI_TOUCH_MAX_POINTS; index++)
        { 
            //EVENT_UP = 0x04
            pointed = (0x04 == ppt->rp.event) ? false : true;

            if(ppt->rp.id != 0x0f)
            {
                if(semi_touch_vkey_handled(pointed, (unsigned int)(ppt->rp.x_h4 << 8) | ppt->rp.x_l8, (unsigned int)(ppt->rp.y_h4 << 8) | ppt->rp.y_l8))
                {

                }
                else
                {
                    if(pointed)
                    {
                        input_report_key(st_dev.input, BTN_TOUCH, 1);
                        input_report_abs(st_dev.input, ABS_MT_POSITION_X, (unsigned int)(ppt->rp.x_h4 << 8) | ppt->rp.x_l8);
                        input_report_abs(st_dev.input, ABS_MT_POSITION_Y, (unsigned int)(ppt->rp.y_h4 << 8) | ppt->rp.y_l8);
                        input_report_abs(st_dev.input, ABS_MT_TOUCH_MAJOR, ppt->rp.z);
                        input_report_abs(st_dev.input, ABS_MT_WIDTH_MAJOR, ppt->rp.z);
                        input_report_abs(st_dev.input, ABS_MT_TRACKING_ID, ppt->rp.id);
                        input_mt_sync(st_dev.input);
                    }
                    else
                    {
                    }
                }
            }
            ppt++;
        }
#elif MULTI_PROTOCOL_TYPE_B == MULTI_PROTOCOL_TYPE
        for(index = 0; index < SEMI_TOUCH_MAX_POINTS; index++)
        { 
            //EVENT_UP = 0x04
            pointed = (0x04 == ppt->rp.event) ? false : true;

            if(ppt->rp.id != 0x0f)
            {
                if(semi_touch_vkey_handled(pointed, (unsigned int)(ppt->rp.x_h4 << 8) | ppt->rp.x_l8, (unsigned int)(ppt->rp.y_h4 << 8) | ppt->rp.y_l8))
                {

                }
                else
                {
                    input_mt_slot(st_dev.input, ppt->rp.id);
                    input_mt_report_slot_state(st_dev.input, MT_TOOL_FINGER, pointed);
                    if(pointed)
                    {
                        input_report_abs(st_dev.input, ABS_MT_POSITION_X, (unsigned int)(ppt->rp.x_h4 << 8) | ppt->rp.x_l8);
                        input_report_abs(st_dev.input, ABS_MT_POSITION_Y, (unsigned int)(ppt->rp.y_h4 << 8) | ppt->rp.y_l8);
                        input_report_abs(st_dev.input, ABS_MT_TOUCH_MAJOR, ppt->rp.z);
                        input_report_abs(st_dev.input, ABS_MT_WIDTH_MAJOR, ppt->rp.z);
                        input_report_key(st_dev.input, BTN_TOUCH, 1);
                    }
                    else
                    {
                    }
                }
            }
            ppt++;
        }
#endif
    }
    if(0 == pointNum) 
    {
        semi_touch_clear_report();
    }
    else
    {
        input_sync(st_dev.input);
    }

    return IRQ_RETVAL(IRQ_HANDLED);
}

#if SEMI_TOUCH_IRQ_VAR_QUEUE
static void semi_touch_irq_work_fun(struct work_struct *work)
{
    semi_touch_irq_handler_imp(st_dev.client->irq, &st_dev);
}

static irqreturn_t semi_touch_irq_handler(int irq, void *p)
{
    if(SEMI_DRV_ERR_OK == semi_touch_wake_lock())
    {
        semi_touch_queue_asyn_work(work_queue_interrupt, semi_touch_irq_work_fun, 0);
    }

    return IRQ_RETVAL(IRQ_HANDLED);
}
#endif

irqreturn_t semi_touch_clear_report(void)
{
#if MULTI_PROTOCOL_TYPE_A == MULTI_PROTOCOL_TYPE
    input_report_key(st_dev.input, BTN_TOUCH, 0);
    input_mt_sync(st_dev.input);
    input_sync(st_dev.input);
#elif MULTI_PROTOCOL_TYPE_B == MULTI_PROTOCOL_TYPE
    int index = 0;
    for(index = 0; index < SEMI_TOUCH_MAX_POINTS; index++)
    {
        input_mt_slot(st_dev.input, index);
        input_mt_report_slot_state(st_dev.input, MT_TOOL_FINGER, false);
        
    }
    input_report_key(st_dev.input, BTN_TOUCH, 0);
    input_sync(st_dev.input);
#endif
    
    return IRQ_RETVAL(IRQ_HANDLED);
}

static int semi_touch_irq_init(struct sm_touch_dev *st_dev)
{
    int ret = 0;
    struct i2c_client *client = st_dev->client;

    client->irq = semi_touch_get_irq(st_dev->int_pin);
    check_return_if_fail(client->irq, NULL);

#if SEMI_TOUCH_IRQ_VAR_QUEUE
    semi_touch_create_work_queue(work_queue_interrupt, typename(work_queue_interrupt));
    ret = devm_request_irq(&client->dev, client->irq, semi_touch_irq_handler, IRQF_TRIGGER_FALLING | IRQF_ONESHOT | IRQF_NO_SUSPEND, client->name, st_dev);
#else
    ret = devm_request_threaded_irq(&client->dev, client->irq, NULL, semi_touch_irq_handler_imp, IRQF_TRIGGER_FALLING | IRQF_ONESHOT | IRQF_NO_SUSPEND, client->name, st_dev);
#endif
    check_return_if_fail(ret, NULL);

    return 0;
}

int semi_touch_resolution_adaption(struct sm_touch_dev *st_dev)
{
    int ret = -SEMI_DRV_ERR_HAL_IO;
    unsigned char readbuffer[256] = {0};
    unsigned short index, pix_x, pix_y;
    const int vkey_evt[] = SEMI_TOUCH_KEY_EVT;

    for(index = 0; index < 3; index++)
    {
        ret = semi_touch_read_bytes(0x20000080, readbuffer, sizeof(readbuffer));
        check_return_if_fail(ret, NULL);
        if(0 == caculate_checksum_u16((unsigned short*)readbuffer, sizeof(readbuffer)))
        {
            break;
        }
        kernel_log_d("config checksum mismatch, retry = %d\n", index);
    }
    
    //xy switch
    if(readbuffer[0x0f] & 0x02)
    {
        pix_x = (unsigned short)((readbuffer[0x09] << 8) + readbuffer[0x08]);
        pix_y = (unsigned short)((readbuffer[0x07] << 8) + readbuffer[0x06]);
    }
    else
    {
        pix_x = (unsigned short)((readbuffer[0x07] << 8) + readbuffer[0x06]);
        pix_y = (unsigned short)((readbuffer[0x09] << 8) + readbuffer[0x08]);
    }
    
    input_set_abs_params(st_dev->input, ABS_MT_POSITION_X, 0, pix_x, 0, 0);
    input_set_abs_params(st_dev->input, ABS_MT_POSITION_Y, 0, pix_y, 0, 0);

    kernel_log_d("resolution = (%d, %d)\n", pix_x, pix_y);

    st_dev->stc.vkey_num = readbuffer[0x52];
    memcpy(st_dev->stc.vkey_evt_arr, vkey_evt, sizeof(int) * MAX_VKEY_NUMBER);
    //key xy switch
    for(index = 0; (index < st_dev->stc.vkey_num) && (index < MAX_VKEY_NUMBER); index++)
    {
        if(readbuffer[0x53] & 0x02)
        {
            pix_x = (readbuffer[0x55] << 8) + readbuffer[0x54];
            pix_y = st_dev->stc.vkey_num > 5 ? (index + 1) : (readbuffer[0x57 + index * 2] << 8) + readbuffer[0x56 + index * 2];
        }
        else
        {
            pix_y = (readbuffer[0x55] << 8) + readbuffer[0x54];
            pix_x = st_dev->stc.vkey_num > 5 ? (index + 1) : (readbuffer[0x57 + index * 2] << 8) + readbuffer[0x56 + index * 2];
        }

        st_dev->stc.vkey_dim_map[index][0] = pix_x;
        st_dev->stc.vkey_dim_map[index][1] = pix_y;
        st_dev->stc.vkey_dim_map[index][2] = 10;
        st_dev->stc.vkey_dim_map[index][3] = 10;

        set_bit(st_dev->stc.vkey_evt_arr[index], st_dev->input->keybit);   
        input_set_capability(st_dev->input, EV_KEY, st_dev->stc.vkey_evt_arr[index]);

        kernel_log_d("vkey index = %d, xy = (%d, %d), event = %d\n", index, pix_x, pix_y, st_dev->stc.vkey_evt_arr[index]);
    }

    return ret;
}

int semi_touch_init(struct i2c_client *client)
{
    int ret = 0;
    memset(&st_dev, 0, sizeof(st_dev));
    st_dev.client = client;

    st_dev.pwron = devm_gpiod_get(&(client->dev), "pwr", GPIOD_OUT_LOW);
    if (IS_ERR(st_dev.pwron))
	    printk("failed to get tp enable gpio\n");
    gpiod_set_value(st_dev.pwron, 1);
    st_dev.int_pin = semi_touch_get_int();
    check_return_if_fail(st_dev.int_pin, NULL);
    st_dev.rst_pin = semi_touch_get_rst();
    check_return_if_fail(st_dev.rst_pin, NULL);

    semi_io_direction_in(st_dev.int_pin);
    semi_io_direction_out(st_dev.rst_pin, 1);

    mutex_init(&st_dev.hal.bus_lock);

    st_dev.hal.hal_write_fun = i2c_write_bytes;
    st_dev.hal.hal_read_fun = i2c_read_bytes;
    st_dev.hal.hal_param = client;

    ret = semi_touch_i2c_init();
    check_return_if_fail(ret, NULL);

    ret = semi_touch_device_prob();
    check_return_if_fail(ret, NULL);

    ret = input_device_init(&st_dev);
    check_return_if_fail(ret, NULL);

    ret = semi_touch_create_apk_proc(&st_dev);
    check_return_if_fail(ret, NULL);

    ret = semi_touch_reset(do_report_after_reset);
    check_return_if_fail(ret, NULL);

    ret = semi_touch_bootup_update_check();
    check_return_if_fail(ret, NULL);

    ret = semi_touch_mode_init(&st_dev);
    check_return_if_fail(ret, NULL);

    ret = semi_touch_resolution_adaption(&st_dev);
    check_return_if_fail(ret, NULL);

    ret = semi_touch_irq_init(&st_dev);
    check_return_if_fail(ret, NULL);

    ret = semi_touch_custom_work(&st_dev);
    check_return_if_fail(ret, NULL);

    ret = semi_touch_work_done();
    check_return_if_fail(ret, NULL);

    return ret;
}

int semi_touch_deinit(struct i2c_client *client)
{
    int ret = 0;

    ret = semi_touch_custom_clean_up();
    check_return_if_fail(ret, NULL);

    ret = semi_touch_destroy_work_queue();
    check_return_if_fail(ret, NULL);

    ret = semi_touch_remove_apk_proc(&st_dev);
    check_return_if_fail(ret, NULL);

    input_device_deinit();

    semi_touch_resource_release();
    gpiod_set_value(st_dev.pwron, 0);

    return ret;
}
