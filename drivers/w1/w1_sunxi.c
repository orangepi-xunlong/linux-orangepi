#include <linux/device.h>
#include <linux/module.h>
#include <linux/w1-gpio.h>
#include <linux/platform_device.h>
#include <linux/gpio.h>
//#include <plat/sys_config.h>
#include <mach/sys_config.h>
#include <mach/platform.h>

static int gpio = -1;
module_param(gpio, int, 0444);
MODULE_PARM_DESC(gpio, "w1 gpio pin number");

static struct w1_gpio_platform_data w1_gpio_pdata = {
    .pin = -1,
    .is_open_drain = 0,
};

static void w1_gpio_release(struct device *dev)
{
    printk("w1_gpio_release good !\n");
}

static struct platform_device w1_device = {
    .name = "w1-gpio",
    .id = -1,
    .dev    = {
            .platform_data  = &w1_gpio_pdata,
            .release        = w1_gpio_release,
    }
};

static int __init w1_sunxi_init(void)
{
    int ret;
    script_item_u val;
    script_item_value_type_e  type;
    
    w1_gpio_pdata.pin = gpio;
    
    if (!gpio_is_valid(w1_gpio_pdata.pin)) {
            // *** We have to determine the pin num from fex definition
            /*val.val = 0;
            type = script_get_item("w1_para", "w1_used", &val);
            if((SCIRPT_ITEM_VALUE_TYPE_INT != type) || (val.val != 1)) {
                printk(KERN_ERR "W1: not used in fex configuration (%d, %d - %d)\n", type, id, val.val);
                return -EINVAL;
            }*/
        
            val.val = gpio;
            type = script_get_item("w1_para", "gpio", &val);
            if(SCIRPT_ITEM_VALUE_TYPE_INT != type) {
                printk(KERN_ERR "W1_SUNXI: invalid gpio pin in fex configuration\n");
                return -EINVAL;
            }
            w1_gpio_pdata.pin = val.val;
            if (!gpio_is_valid(w1_gpio_pdata.pin)) {
                printk(KERN_ERR "W1_SUNXI: invalid gpio pin in fex configuration %d\n", val.val);
                return -EINVAL;
            }
    }
    ret = platform_device_register(&w1_device);
    if (ret) {
            printk(KERN_ERR "W1_SUNXI: error registering w1-gpio device on GPIO-%d\n", w1_gpio_pdata.pin);
            return ret;
    }
    printk(KERN_INFO "W1_SUNXI: Added w1-gpio on GPIO-%d\n", w1_gpio_pdata.pin);
    gpio_free(w1_gpio_pdata.pin);
    return 0;
}

static void __exit w1_sunxi_exit(void)
{
    platform_device_unregister(&w1_device);
}

module_init(w1_sunxi_init);
module_exit(w1_sunxi_exit);

MODULE_DESCRIPTION("GPIO w1 sunxi platform device");
MODULE_AUTHOR("Damien Nicolet <zardam@gmail.com> mod by LoBo <loboris@gmail.com");
MODULE_LICENSE("GPL");
