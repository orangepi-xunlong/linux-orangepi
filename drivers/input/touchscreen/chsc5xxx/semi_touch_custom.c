#include <linux/device.h>
#include <linux/proc_fs.h>
#include <linux/kthread.h>
#include <linux/delay.h>
#include <linux/i2c.h>
#include <linux/input.h>
#include "semi_touch_interface.h"

enum entry_type{ chsc_version, chsc_tp_info, chsc_proximity, chsc_guesture, chsc_online_update, chsc_glove, chsc_suspend, chsc_orientation, chsc_esd_check, entry_max };

#if SEMI_TOUCH_MAKE_NODES_DIR == MAKE_NODE_UNDER_PROC
/******************************************************************************************************************************************/
/*make custom nodes under proc node*/
/******************************************************************************************************************************************/
static struct proc_dir_entry* custom_proc_entry[entry_max];

void semi_touch_create_nodes_dir(void) 
{
    if(NULL == st_dev.chsc_nodes_dir) 
    {
        st_dev.chsc_nodes_dir = proc_mkdir(SEMI_TOUCH_PROC_DIR, NULL);
    }
}

void semi_touch_release_nodes_dir(void)
{
    int index = 0;
    for(index = 0; index < entry_max; index++)
    {
        if(NULL != custom_proc_entry[index])
        {
            proc_remove(custom_proc_entry[index]);
        }
    }
    if(NULL != st_dev.chsc_nodes_dir) 
    {
        proc_remove((struct proc_dir_entry*)st_dev.chsc_nodes_dir);
    }
}

static int semi_touch_register_nodefun_imp(enum entry_type etype, char* node_name, const struct proc_ops* opt_addr)
{
    struct proc_dir_entry* entry = proc_create(node_name, 0777, st_dev.chsc_nodes_dir, opt_addr);
    check_return_if_zero(entry, NULL);

    custom_proc_entry[etype] = entry;

    return 0;
}

#define semi_touch_register_nodefun(etype, fun_write, fun_read) \
{ \
    static const struct proc_ops ops_##etype = { \
    .proc_write = fun_write, \
    .proc_read  = fun_read, \
    }; \
    semi_touch_register_nodefun_imp(etype, #etype, &ops_##etype); \
}

#define kernel_buffer_prepare(copy, ker_buf, size) \
char ker_buf[size], *copy = ker_buf; \
do{memset(ker_buf, 0, sizeof(ker_buf)); if(*ppos) return 0;} while(0)

#define kernel_buffer_to_entry(ker_buf, size) \
*ppos = size; \
ret = copy_to_user(buff, ker_buf, size); \
check_return_if_fail(ret, NULL);

#define kernel_buffer_from_entry(ker_buf, size) \
ret = copy_from_user(ker_buf, buff, size); \
check_return_if_fail(ret, NULL);

#define chsc_version_node_write_declare() chsc_version_node_write(struct file* fp, const char __user *buff, size_t len, loff_t* ppos)
#define chsc_version_node_read_declare() chsc_version_node_read(struct file* fp, char __user *buff, size_t len, loff_t* ppos)
#define chsc_tp_info_node_write_declare() chsc_tp_info_node_write(struct file* fp, const char __user *buff, size_t len, loff_t* ppos)
#define chsc_tp_info_node_read_declare() chsc_tp_info_node_read(struct file* fp, char __user *buff, size_t len, loff_t* ppos)
#define chsc_proximity_node_write_declare() chsc_proximity_node_write(struct file* fp, const char __user *buff, size_t len, loff_t* ppos)
#define chsc_proximity_node_read_declare() chsc_proximity_node_read(struct file* fp, char __user *buff, size_t len, loff_t* ppos)
#define chsc_guesture_node_write_declare() chsc_guesture_node_write(struct file* fp, const char __user *buff, size_t len, loff_t* ppos)
#define chsc_guesture_node_read_declare() chsc_guesture_node_read(struct file* fp, char __user *buff, size_t len, loff_t* ppos)
#define chsc_glove_node_write_declare() chsc_glove_node_write(struct file* fp, const char __user *buff, size_t len, loff_t* ppos)
#define chsc_glove_node_read_declare() chsc_glove_node_read(struct file* fp, char __user *buff, size_t len, loff_t* ppos)
#define chsc_suspend_node_write_declare() chsc_suspend_node_write(struct file* fp, const char __user *buff, size_t len, loff_t* ppos)
#define chsc_suspend_node_read_declare() chsc_suspend_node_read(struct file* fp, char __user *buff, size_t len, loff_t* ppos)
#define chsc_online_update_node_write_declare() chsc_online_update_node_write(struct file* fp, const char __user *buff, size_t len, loff_t* ppos)
#define chsc_online_update_node_read_declare() chsc_online_update_node_read(struct file* fp, char __user *buff, size_t len, loff_t* ppos)
#define chsc_orientation_node_write_declare() chsc_orientation_node_write(struct file* fp, const char __user *buff, size_t len, loff_t* ppos)
#define chsc_orientation_node_read_declare() chsc_orientation_node_read(struct file* fp, char __user *buff, size_t len, loff_t* ppos)
#define chsc_esd_check_node_write_declare() chsc_esd_check_node_write(struct file* fp, const char __user *buff, size_t len, loff_t* ppos)
#define chsc_esd_check_node_read_declare() chsc_esd_check_node_read(struct file* fp, char __user *buff, size_t len, loff_t* ppos)
#elif SEMI_TOUCH_MAKE_NODES_DIR == MAKE_NDDE_UNDER_SYS
/******************************************************************************************************************************************/
/*make custom nodes under sys file system*/
/******************************************************************************************************************************************/
void semi_touch_create_nodes_dir(void) 
{
    if(NULL == st_dev.chsc_nodes_dir) 
    {
        st_dev.chsc_nodes_dir = kobject_create_and_add(SEMI_TOUCH_PROC_DIR, NULL);
    }
}

void semi_touch_release_nodes_dir(void)
{
    if(NULL != st_dev.chsc_nodes_dir) 
    {
        kobject_put((struct kobject *)st_dev.chsc_nodes_dir);
    }
}

#define semi_touch_register_nodefun(etype, fun_store, fun_show) \
{ \
    static struct kobj_attribute etype = __ATTR(etype, 0664, fun_show, fun_store); \
    ret = sysfs_create_file((struct kobject *)st_dev->chsc_nodes_dir, &etype.attr); \
    check_return_if_fail(ret, NULL); \
}

#define kernel_buffer_prepare(copy, ker_buf, size) \
char ker_buf[size], *copy = ker_buf; \
do{memset(ker_buf, 0, sizeof(ker_buf));} while(0)

#define kernel_buffer_to_entry(ker_buf, size) \
memcpy(buff, ker_buf, size); \
ret = 0

#define kernel_buffer_from_entry(ker_buf, size) \
memcpy(ker_buf, buff, size); \
ret = 0

#define chsc_version_node_write_declare() chsc_version_node_write(struct kobject *dev, struct kobj_attribute *attr, const char *buff, size_t len)
#define chsc_version_node_read_declare() chsc_version_node_read(struct kobject* dev, struct kobj_attribute* attr, char* buff)
#define chsc_tp_info_node_write_declare() chsc_tp_info_node_write(struct kobject *dev, struct kobj_attribute *attr, const char *buff, size_t len)
#define chsc_tp_info_node_read_declare() chsc_tp_info_node_read(struct kobject* dev, struct kobj_attribute* attr, char* buff)
#define chsc_proximity_node_write_declare() chsc_proximity_node_write(struct kobject *dev, struct kobj_attribute *attr, const char *buff, size_t len)
#define chsc_proximity_node_read_declare() chsc_proximity_node_read(struct kobject* dev, struct kobj_attribute* attr, char* buff)
#define chsc_guesture_node_write_declare() chsc_guesture_node_write(struct kobject *dev, struct kobj_attribute *attr, const char *buff, size_t len)
#define chsc_guesture_node_read_declare() chsc_guesture_node_read(struct kobject* dev, struct kobj_attribute* attr, char* buff)
#define chsc_glove_node_write_declare() chsc_glove_node_write(struct kobject *dev, struct kobj_attribute *attr, const char *buff, size_t len)
#define chsc_glove_node_read_declare() chsc_glove_node_read(struct kobject* dev, struct kobj_attribute* attr, char* buff)
#define chsc_suspend_node_write_declare() chsc_suspend_node_write(struct kobject *dev, struct kobj_attribute *attr, const char *buff, size_t len)
#define chsc_suspend_node_read_declare() chsc_suspend_node_read(struct kobject* dev, struct kobj_attribute* attr, char* buff)
#define chsc_online_update_node_write_declare() chsc_online_update_node_write(struct kobject *dev, struct kobj_attribute *attr, const char *buff, size_t len)
#define chsc_online_update_node_read_declare() chsc_online_update_node_read(struct kobject* dev, struct kobj_attribute* attr, char* buff)
#define chsc_orientation_node_write_declare() chsc_orientation_node_write(struct kobject *dev, struct kobj_attribute *attr, const char *buff, size_t len)
#define chsc_orientation_node_read_declare() chsc_orientation_node_read(struct kobject* dev, struct kobj_attribute* attr, char* buff)
#define chsc_esd_check_node_write_declare() chsc_esd_check_node_write(struct kobject *dev, struct kobj_attribute *attr, const char *buff, size_t len)
#define chsc_esd_check_node_read_declare() chsc_esd_check_node_read(struct kobject* dev, struct kobj_attribute* attr, char* buff)
#endif //SEMI_TOUCH_MAKE_NODES_DIR == MAKE_NDDE_UNDER_SYS

const char* const mapping_ic_from_type(unsigned char ictype)
{
    static char *ic_name = "un-defined";

    switch (ictype)
    {
    case 0x00:
        ic_name = "CHSC5472";
        break;
    case 0x01:
        ic_name = "CHSC5448";
        break;
    case 0x02:
        ic_name = "CHSC5448A";
        break;
    case 0x03:
        ic_name = "CHSC5460";
        break;
    case 0x04:
        ic_name = "CHSC5468";
        break;
	case 0x05:
        ic_name = "CHSC5432";
        break;
    case 0x10:
        ic_name = "CHSC5816";
        break;
    case 0x11:
        ic_name = "CHSC1716";
        break;
    default:
        break;
    }

    return ic_name;
}

static ssize_t chsc_version_node_write_declare()
{
    return -EPERM;
}

static ssize_t chsc_version_node_read_declare()
{
    int ret, count;
    unsigned char readBuffer[8] = {0};
    kernel_buffer_prepare(szCopy, szKernel, 128);

    ret = semi_touch_read_bytes(0x20000000 + 0x80, readBuffer, 8);
    check_return_if_fail(ret, NULL);

    szCopy += sprintf(szCopy, "Ic type %s\n", mapping_ic_from_type(readBuffer[0]));

    szCopy += sprintf(szCopy, "config version is %02X\n", readBuffer[1]);
    szCopy += sprintf(szCopy, "vender id is %d, ", readBuffer[4]);
    szCopy += sprintf(szCopy, "product id is %d\n", (readBuffer[3] << 8) + readBuffer[2]);

    ret = semi_touch_read_bytes(0x20000000 + 0x10, readBuffer, 8);
    check_return_if_fail(ret, NULL);

    szCopy += sprintf(szCopy, "boot version is %04X\n", ((readBuffer[5] << 8) + readBuffer[4]));
    szCopy += sprintf(szCopy, "driver version is %s\n", CHSC_DRIVER_VERSION);

    count = szCopy - szKernel;

    kernel_buffer_to_entry(szKernel, count);

    return count;
}

static ssize_t chsc_tp_info_node_write_declare()
{
    return -EPERM;
}

static ssize_t chsc_tp_info_node_read_declare()
{
    int ret, count;
    struct i2c_client *client = st_dev.client;
    kernel_buffer_prepare(szCopy, szKernel, 128);
    
    szCopy += sprintf(szCopy, "Max finger number is %0d\n", SEMI_TOUCH_MAX_POINTS);
    szCopy += (int)sprintf(szCopy, "Int irq is %d\n", (int)client->irq);
    szCopy += sprintf(szCopy, "I2c address is 0x%02X(0x%02X)\n", client->addr, (client->addr) << 1);
    szCopy += sprintf(szCopy, "Run status is 0x%08X\n", st_dev.stc.ctp_run_status);
    szCopy += sprintf(szCopy, "Fun enable is 0x%04X\n", st_dev.stc.custom_function_en);
    count = szCopy - szKernel;

    kernel_buffer_to_entry(szKernel, count);

    return count;
}

static ssize_t chsc_proximity_node_write_declare()
{
    int ret;
    kernel_buffer_prepare(szCopy, szKernel, 8);
    kernel_buffer_from_entry(szKernel, (len > 8) ? 8 : len);

    if('o' == szCopy[0])
    {
        open_proximity_function(st_dev.stc.custom_function_en);
    }
    else if('c' == szCopy[0])
    {
        close_proximity_function(st_dev.stc.custom_function_en);
    }

    if(is_proximity_function_en(st_dev.stc.custom_function_en)) 
    {
        if('0' == szCopy[0])
        {
            semi_touch_proximity_switch(0);
        }
        else if('1' == szCopy[0])
        {
            semi_touch_proximity_switch(1);
        }
    }

    return len;
}

static ssize_t chsc_proximity_node_read_declare()
{
    int ret, count;
    kernel_buffer_prepare(szCopy, szKernel, 128);
    
    szCopy += sprintf(szCopy, "proximity switch is %d, status is %d.\n", 
        is_proximity_function_en(st_dev.stc.custom_function_en), is_proximity_activate(st_dev.stc.ctp_run_status));
    count = szCopy - szKernel;

    kernel_buffer_to_entry(szKernel, count);

    return count;
}

static ssize_t chsc_guesture_node_write_declare()
{
    int ret;
    kernel_buffer_prepare(szCopy, szKernel, 8);
    kernel_buffer_from_entry(szKernel, (len > 8) ? 8 : len);

    if('o' == szCopy[0])
    {
        open_guesture_function(st_dev.stc.custom_function_en);
    }
    else if('c' == szCopy[0])
    {
        close_guesture_function(st_dev.stc.custom_function_en);
    }

    if(is_guesture_function_en(st_dev.stc.custom_function_en)) 
    {
        if('0' == szCopy[0])
        {
            semi_touch_guesture_switch(0);
        }
        else if('1' == szCopy[0])
        {
            semi_touch_guesture_switch(1);
        }
    }

    return len;
}

static ssize_t chsc_guesture_node_read_declare()
{
    int ret, count;
    kernel_buffer_prepare(szCopy, szKernel, 128);
    
    szCopy += sprintf(szCopy, "guesture switch is %d, status is %d.\n", 
        is_guesture_function_en(st_dev.stc.custom_function_en), is_guesture_activate(st_dev.stc.ctp_run_status));
    count = szCopy - szKernel;

    kernel_buffer_to_entry(szKernel, count);

    return count;
}

static ssize_t chsc_glove_node_write_declare()
{
    int ret;
    kernel_buffer_prepare(szCopy, szKernel, 8);
    kernel_buffer_from_entry(szKernel, (len > 8) ? 8 : len);

    if('o' == szCopy[0])
    {
        open_glove_function(st_dev.stc.custom_function_en);
        semi_touch_glove_switch(1);
    }
    else if('c' == szCopy[0])
    {
        semi_touch_glove_switch(0);
        close_glove_function(st_dev.stc.custom_function_en);
    }

    if(is_glove_function_en(st_dev.stc.custom_function_en)) 
    {
        if('0' == szCopy[0])
        {
            semi_touch_glove_switch(0);
        }
        else if('1' == szCopy[0])
        {
            semi_touch_glove_switch(1);
        }
    }

    return len;
}

static ssize_t chsc_glove_node_read_declare()
{
    int ret, count;
    kernel_buffer_prepare(szCopy, szKernel, 128);
    
    szCopy += sprintf(szCopy, "glove switch is %d, status is %d.\n", 
        is_glove_function_en(st_dev.stc.custom_function_en), is_glove_activate(st_dev.stc.ctp_run_status));
    count = szCopy - szKernel;

    kernel_buffer_to_entry(szKernel, count);

    return count;
}

static ssize_t chsc_suspend_node_write_declare()
{
    int ret;
    kernel_buffer_prepare(szCopy, szKernel, 8);
    kernel_buffer_from_entry(szKernel, (len > 8) ? 8 : len);

    if('0' == szCopy[0])
    {
        semi_touch_resume_entry(&st_dev.client->dev);  //semi_touch_suspend_ctrl(0);
    }
    else if('1' == szCopy[0])
    {
        semi_touch_suspend_entry(&st_dev.client->dev);//semi_touch_suspend_ctrl(1);
    }

    return len;
}

static ssize_t chsc_suspend_node_read_declare()
{
    int ret, count;
    kernel_buffer_prepare(szCopy, szKernel, 128);
    
    szCopy += sprintf(szCopy, "suspend switch is %d, status is %d.\n", 
        1, is_suspend_activate(st_dev.stc.ctp_run_status));
    count = szCopy - szKernel;

    kernel_buffer_to_entry(szKernel, count);

    return count;
}

static ssize_t chsc_orientation_node_write_declare()
{
    int ret;
    kernel_buffer_prepare(szCopy, szKernel, 8);
    kernel_buffer_from_entry(szKernel, (len > 8) ? 8 : len);

    if('0' == szCopy[0])
    {
        semi_touch_orientation_switch(0);
    }
    else if('1' == szCopy[0])
    {
        semi_touch_orientation_switch(1);
    }

    return len;
}

static ssize_t chsc_orientation_node_read_declare()
{
    int ret, count;
    kernel_buffer_prepare(szCopy, szKernel, 128);
    
    szCopy += sprintf(szCopy, "orientation status is %s.\n", is_orientation_activate(st_dev.stc.ctp_run_status) ? "horizontal" : "vertical");
    count = szCopy - szKernel;

    kernel_buffer_to_entry(szKernel, count);

    return count;
}

static ssize_t chsc_esd_check_node_read_declare()
{
    int ret, count;
    kernel_buffer_prepare(szCopy, szKernel, 128);
    
    szCopy += sprintf(szCopy, "esd check switch is %d.\n", is_esd_function_en(st_dev.stc.custom_function_en));
    count = szCopy - szKernel;

    kernel_buffer_to_entry(szKernel, count);

    return count;
}

static ssize_t chsc_esd_check_node_write_declare()
{
    int ret;
    kernel_buffer_prepare(szCopy, szKernel, 8);
    kernel_buffer_from_entry(szKernel, (len > 8) ? 8 : len);

    if('o' == szCopy[0])
    {
        open_esd_function(st_dev.stc.custom_function_en);
    }
    else if('c' == szCopy[0])
    {
        close_esd_function(st_dev.stc.custom_function_en);
    }

    return len;
}

static ssize_t chsc_online_update_node_write_declare()
{
    int ret;
    kernel_buffer_prepare(szCopy, szKernel, 128);
    kernel_buffer_from_entry(szKernel, (len > 128) ? 128 : len);

    if('1' == szCopy[0])
    {
        sprintf(szCopy, "%s", CHSC_AUTO_UPDATE_PACKET_BIN);
    }
    else if(len > 1)
    {
        szCopy[len - 1] = 0;
    }

    ret = semi_touch_online_update_check((char*)szCopy);

    return len;
}

static ssize_t chsc_online_update_node_read_declare()
{
    int ret, count;
    kernel_buffer_prepare(szCopy, szKernel, 128);

    szCopy += sprintf(szCopy, "online update is %s\n", SEMI_TOUCH_ONLINE_UPDATE_EN ? "enabled" : "disabled" );
    
    count = szCopy - szKernel;

    kernel_buffer_to_entry(szKernel, count);

    return count ;
}

/********************************************************************************************************************************/
/*glove function*/
int semi_touch_glove_prepare(void)
{
#if SEMI_TOUCH_GLOVE_OPEN
    open_glove_function(st_dev.stc.custom_function_en);
#endif
    return 0;
}

/********************************************************************************************************************************/

/********************************************************************************************************************************/
/*guesture support*/
#if SEMI_TOUCH_GESTURE_OPEN
//#include <linux/wakelock.h>
#define GESTURE_LEFT                         0x20
#define GESTURE_RIGHT                        0x21
#define GESTURE_UP                           0x22
#define GESTURE_DOWN                         0x23
#define GESTURE_DOUBLECLICK                  0x24
#define GESTURE_SINGLECLICK                  0x25
#define GESTURE_O                            0x30
#define GESTURE_W                            0x31
#define GESTURE_M                            0x32
#define GESTURE_E                            0x33
#define GESTURE_C                            0x34
#define GESTURE_S                            0x46
#define GESTURE_V                            0x54
#define GESTURE_Z                            0x65
#define GESTURE_L                            0x44
//static struct wake_lock gesture_timeout_wakelock;
int semi_touch_gesture_prepare(void)
{
    //open_guesture_function(st_dev.stc.custom_function_en);
    //wake_lock_init(&gesture_timeout_wakelock, WAKE_LOCK_SUSPEND, "gesture_timeout_wakelock");

    input_set_capability(st_dev.input, EV_KEY, KEY_POWER);
    input_set_capability(st_dev.input, EV_KEY, KEY_U);
    input_set_capability(st_dev.input, EV_KEY, KEY_LEFT);
    input_set_capability(st_dev.input, EV_KEY, KEY_RIGHT);
    input_set_capability(st_dev.input, EV_KEY, KEY_UP);
    input_set_capability(st_dev.input, EV_KEY, KEY_DOWN);
    input_set_capability(st_dev.input, EV_KEY, KEY_D);
    input_set_capability(st_dev.input, EV_KEY, KEY_O);
    input_set_capability(st_dev.input, EV_KEY, KEY_W);
    input_set_capability(st_dev.input, EV_KEY, KEY_M);
    input_set_capability(st_dev.input, EV_KEY, KEY_E);
    input_set_capability(st_dev.input, EV_KEY, KEY_C);
    input_set_capability(st_dev.input, EV_KEY, KEY_S);
    input_set_capability(st_dev.input, EV_KEY, KEY_V);
    input_set_capability(st_dev.input, EV_KEY, KEY_Z);

    __set_bit(KEY_POWER, st_dev.input->keybit);
    __set_bit(KEY_U,     st_dev.input->keybit);
    __set_bit(KEY_LEFT,  st_dev.input->keybit);
    __set_bit(KEY_RIGHT, st_dev.input->keybit);
    __set_bit(KEY_UP,    st_dev.input->keybit);
    __set_bit(KEY_DOWN,  st_dev.input->keybit);
    __set_bit(KEY_D,     st_dev.input->keybit);
    __set_bit(KEY_O,     st_dev.input->keybit);
    __set_bit(KEY_W,     st_dev.input->keybit);
    __set_bit(KEY_M,     st_dev.input->keybit);
    __set_bit(KEY_E,     st_dev.input->keybit);
    __set_bit(KEY_C,     st_dev.input->keybit);
    __set_bit(KEY_S,     st_dev.input->keybit);
    __set_bit(KEY_V,     st_dev.input->keybit);
    __set_bit(KEY_Z,     st_dev.input->keybit);

    return 0;
}
int semi_touch_gesture_stop(void)
{
    //if(is_guesture_function_en(st_dev.stc.custom_function_en))
    //{
    //    wake_lock_destroy(&gesture_timeout_wakelock);
    //}

    return 0;
}
int semi_touch_wake_lock(void)
{
    // if(is_guesture_activate(st_dev.stc.ctp_run_status))
    // {
    //     //irq_set_irq_type(st_dev.client->irq, IRQF_TRIGGER_FALLING);
    //     //wake_lock_timeout(&gesture_timeout_wakelock, msecs_to_jiffies(2500));
    //     kernel_log_d("int interrupts, guesture on\n");
    // }
    return SEMI_DRV_ERR_OK;
}
bool semi_touch_gesture_report(unsigned char gesture_id)
{
    int keycode = 0;

    //wake_lock_timeout(&gesture_timeout_wakelock, msecs_to_jiffies(2000));

    switch(gesture_id)
    {
        case GESTURE_LEFT:
            keycode = KEY_LEFT;
            break;
        case GESTURE_RIGHT:
            keycode = KEY_RIGHT;
            break;
        case GESTURE_UP:
            keycode = KEY_UP;
            break;
        case GESTURE_DOWN:
            keycode = KEY_DOWN;
            break;
        case GESTURE_SINGLECLICK:
            keycode = KEY_POWER;//KEY_U;    
            break;
        case GESTURE_DOUBLECLICK:
            keycode = KEY_POWER;//KEY_U;    
            break;
        case GESTURE_O:
            keycode = KEY_O;
            break;
        case GESTURE_W:
            keycode = KEY_W;
            break;
        case GESTURE_M:
            keycode = KEY_M;
            break;
        case GESTURE_E:
            keycode = KEY_E;
            break;
        case GESTURE_C:
            keycode = KEY_C;
            break;
        case GESTURE_S:
            keycode = KEY_S;
            break;
            case GESTURE_V:
            keycode = KEY_V;
            break;
        case GESTURE_Z:
            keycode = KEY_UP;
            break;
        case GESTURE_L:
            keycode = KEY_L;
            break;
        default:
            break;
    }

    if(keycode)
    {
        input_report_key(st_dev.input, keycode, 1);
        input_sync(st_dev.input);
        input_report_key(st_dev.input, keycode, 0);
        input_sync(st_dev.input);
    }
    
    return true;
}
#else //SEMI_TOUCH_GESTURE_OPEN
int semi_touch_gesture_prepare(void) { return 0; }
int semi_touch_gesture_stop(void) { return 0; }
int semi_touch_wake_lock(void) { return 0; }
bool semi_touch_gesture_report(unsigned char gesture_id) { return 0; }
#endif //SEMI_TOUCH_GESTURE_OPEN

/********************************************************************************************************************************/
/*esd support*/
#if SEMI_TOUCH_ESD_CHECK_OPEN
static void semi_touch_esd_work_fun(struct work_struct *work);

int semi_touch_esd_check_prepare(void)
{
    open_esd_function(st_dev.stc.custom_function_en);
    semi_touch_queue_asyn_work(work_queue_custom_work, semi_touch_esd_work_fun, 4000);

    return 0;
}

static void semi_touch_esd_work_fun(struct work_struct *work)
{
    semi_touch_heart_beat();

    semi_touch_queue_asyn_work(work_queue_custom_work, semi_touch_esd_work_fun, 4000);
}

#else //SEMI_TOUCH_ESD_CHECK_OPEN
int semi_touch_esd_check_prepare(void) { return 0; }
#endif //SEMI_TOUCH_ESD_CHECK_OPEN


/********************************************************************************************************************************/
int semi_touch_custom_work(struct sm_touch_dev *st_dev)
{
    int ret = 0;
    //unsigned char readBuffer[8];

    semi_touch_create_nodes_dir();
    semi_touch_register_nodefun(chsc_version, chsc_version_node_write, chsc_version_node_read);
    semi_touch_register_nodefun(chsc_tp_info, chsc_tp_info_node_write, chsc_tp_info_node_read);
    semi_touch_register_nodefun(chsc_proximity, chsc_proximity_node_write, chsc_proximity_node_read);
    semi_touch_register_nodefun(chsc_guesture, chsc_guesture_node_write, chsc_guesture_node_read);
    semi_touch_register_nodefun(chsc_glove, chsc_glove_node_write, chsc_glove_node_read);
    semi_touch_register_nodefun(chsc_suspend, chsc_suspend_node_write, chsc_suspend_node_read);
    semi_touch_register_nodefun(chsc_online_update, chsc_online_update_node_write, chsc_online_update_node_read);
    semi_touch_register_nodefun(chsc_orientation, chsc_orientation_node_write, chsc_orientation_node_read);
    semi_touch_register_nodefun(chsc_esd_check, chsc_esd_check_node_write, chsc_esd_check_node_read);

    ret = semi_touch_proximity_init();
    check_return_if_fail(ret, NULL);

    ret = semi_touch_gesture_prepare();
    check_return_if_fail(ret, NULL);

    ret = semi_touch_esd_check_prepare();
    check_return_if_fail(ret, NULL);

    ret = semi_touch_glove_prepare();
    check_return_if_fail(ret, NULL);

    //this code tell us how to get tp infomation
    //ret = semi_touch_read_bytes(0x20000000 + 0x80, readBuffer, sizeof(readBuffer));
    //check_return_if_fail(ret, NULL);

    // strcat(TP_NAME, mapping_ic_from_type(readBuffer[0]));
    // TP_FW_VER  = readBuffer[1];
    // TP_VENDOR  = readBuffer[4];
    // TP_PRODUCT = readBuffer[3] << 8) + readBuffer[2];

    return ret;
}

int semi_touch_custom_clean_up(void)
{
    int ret;

    semi_touch_release_nodes_dir();

    ret = semi_touch_proximity_stop();
    check_return_if_fail(ret, NULL);

    ret = semi_touch_gesture_stop();
    check_return_if_fail(ret, NULL);

    return ret;
}
