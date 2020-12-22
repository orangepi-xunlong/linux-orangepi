/*
 * AllWinner PWM driver
 *
 * Copyright (C) 2013 AllWinner
 *
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2. This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */
#include <linux/module.h>
#include<linux/init.h>
#include<linux/cdev.h>
#include <linux/pwm.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/err.h>
#include <linux/fs.h>
#include<linux/device.h>
#include<linux/gpio.h>
#include<linux/pinctrl/pinconf.h>
#include <linux/pinctrl/pinconf-sunxi.h>
#include <linux/pinctrl/consumer.h>

#include <linux/io.h>
#include "mach/platform.h"
#include "mach/sys_config.h"

#if ((defined CONFIG_ARCH_SUN8IW1P1) || (defined CONFIG_ARCH_SUN9IW1P1))
#define PWM_NUM 4
#define PWM_REG_NUM 8


#elif defined(CONFIG_ARCH_SUN8IW3P1) || defined(CONFIG_ARCH_SUN8IW5P1) || defined(CONFIG_ARCH_SUN8IW6P1) || defined(CONFIG_ARCH_SUN8IW9P1) || defined(CONFIG_ARCH_SUN8IW8P1) || defined(CONFIG_ARCH_SUN8IW7P1)
#define PWM_NUM 2
#define PWM_REG_NUM 3

#endif

#define PWM_DEBUG 0

#if PWM_DEBUG
#define pwm_debug(msg...) printk
#else
#define pwm_debug(msg...)
#endif

typedef unsigned int __u32;

__u32 record_reg[PWM_REG_NUM];

#define sys_get_wvalue(n)   (*((volatile __u32 *)(n)))          /* word input */
#define sys_put_wvalue(n,c) (*((volatile __u32 *)(n))  = (c))   /* word output */

static struct cdev *my_cdev;
static dev_t devid ;
static struct class *pwm_class;
struct device *pwm_dev;

struct sunxi_pwm_cfg {
    script_item_u *list;
};

struct sunxi_pwm_chip {
    struct pwm_chip chip;
};

struct sunxi_pwm_cfg pwm_cfg[4];

__u32 pwm_active_sta[4] = {1, 0, 0, 0};
__u32 pwm_pin_count[4] = {0};

static __u32 sunxi_pwm_read_reg(__u32 offset)
{
    __u32 value = 0;

   value = sys_get_wvalue(SUNXI_PWM_VBASE + offset);

    return value;
}

static __u32 sunxi_pwm_write_reg(__u32 offset, __u32 value)
{
    sys_put_wvalue(SUNXI_PWM_VBASE + offset, value);

    return 0;
}

void sunxi_pwm_get_sys_config(int pwm, struct sunxi_pwm_cfg *sunxi_pwm_cfg)
{
    char primary_key[25];
    script_item_u   val;
    script_item_value_type_e  type;

    sprintf(primary_key, "pwm%d_para", pwm);
    sunxi_pwm_cfg->list = NULL;
    type = script_get_item(primary_key, "pwm_used", &val);
    if((SCIRPT_ITEM_VALUE_TYPE_INT == type) && (val.val == 1)) {
        pwm_pin_count[pwm] = script_get_pio_list(primary_key, &sunxi_pwm_cfg->list);
        if(pwm_pin_count[pwm] == 0)
            pr_warn("get pwm%d gpio list fail!\n", pwm);
    }
}

static int sunxi_pwm_set_polarity(struct pwm_chip *chip, struct pwm_device *pwm, enum pwm_polarity polarity)
{
    __u32 temp;

#if ((defined CONFIG_ARCH_SUN8IW1P1) || (defined CONFIG_ARCH_SUN9IW1P1))

    temp = sunxi_pwm_read_reg(pwm->pwm * 0x10);
    if(polarity == PWM_POLARITY_NORMAL) {
        pwm_active_sta[pwm->pwm] = 1;
        temp |= 1 << 5;
    } else {
        pwm_active_sta[pwm->pwm] = 0;
        temp &= ~(1 << 5);
    }

    sunxi_pwm_write_reg(pwm->pwm * 0x10, temp);

#elif defined(CONFIG_ARCH_SUN8IW3P1) || defined(CONFIG_ARCH_SUN8IW5P1) || defined(CONFIG_ARCH_SUN8IW6P1) || defined(CONFIG_ARCH_SUN8IW9P1) || defined(CONFIG_ARCH_SUN8IW8P1) || defined(CONFIG_ARCH_SUN8IW7P1)

    temp = sunxi_pwm_read_reg(0);
    if(polarity == PWM_POLARITY_NORMAL) {
        pwm_active_sta[pwm->pwm] = 1;
        if(pwm->pwm == 0)
            temp |= 1 << 5;
        else
            temp |= 1 << 20;
        }else {
            pwm_active_sta[pwm->pwm] = 0;
            if(pwm->pwm == 0)
                temp &= ~(1 << 5);
            else
                temp &= ~(1 << 20);
            }

	sunxi_pwm_write_reg(0, temp);

#endif

    return 0;
}

static int sunxi_pwm_config(struct pwm_chip *chip, struct pwm_device *pwm,
		int duty_ns, int period_ns)
{
    #if ((defined CONFIG_ARCH_SUN8IW1P1) || (defined CONFIG_ARCH_SUN9IW1P1))

    __u32 pre_scal[7] = {1, 2, 4, 8, 16, 32, 64};
    __u32 freq;
    __u32 pre_scal_id = 0;
    __u32 entire_cycles = 256;
    __u32 active_cycles = 192;
    __u32 entire_cycles_max = 65536;
    __u32 temp;
    __u32 calc;

    if(period_ns < 10667)
        freq = 93747;
    else if(period_ns > 174762666) {
        freq = 6;
        calc = period_ns / duty_ns;
        duty_ns = 174762666 / calc;
        period_ns = 174762666;
        }
    else
        freq = 1000000000 / period_ns;

    entire_cycles = 24000000 / freq /pre_scal[pre_scal_id];

    while(entire_cycles > entire_cycles_max) {
        pre_scal_id++;

        if(pre_scal_id > 6)
            break;

        entire_cycles = 24000000 / freq / pre_scal[pre_scal_id];
        }

    if(period_ns < 5*100*1000)
        active_cycles = (duty_ns * entire_cycles + (period_ns/2)) /period_ns;
    else if(period_ns >= 5*100*1000 && period_ns < 6553500)
        active_cycles = ((duty_ns / 100) * entire_cycles + (period_ns /2 / 100)) / (period_ns/100);
    else
        active_cycles = ((duty_ns / 10000) * entire_cycles + (period_ns /2 / 10000)) / (period_ns/10000);

    temp = sunxi_pwm_read_reg(pwm->pwm * 0x10);

    temp = (temp & 0xfffffff0) | pre_scal_id;

    sunxi_pwm_write_reg(pwm->pwm * 0x10, temp);

    sunxi_pwm_write_reg(pwm->pwm * 0x10 + 0x04, ((entire_cycles - 1)<< 16) | active_cycles);

    pwm_debug("PWM _TEST: duty_ns=%d, period_ns=%d, freq=%d, per_scal=%d, period_reg=0x%x\n", duty_ns, period_ns, freq, pre_scal_id, temp);


    #elif defined(CONFIG_ARCH_SUN8IW3P1) || defined(CONFIG_ARCH_SUN8IW5P1) || defined(CONFIG_ARCH_SUN8IW6P1) || defined(CONFIG_ARCH_SUN8IW9P1) || defined(CONFIG_ARCH_SUN8IW8P1) || defined(CONFIG_ARCH_SUN8IW7P1)

    __u32 pre_scal[11][2] = {{15, 1}, {0, 120}, {1, 180}, {2, 240}, {3, 360}, {4, 480}, {8, 12000}, {9, 24000}, {10, 36000}, {11, 48000}, {12, 72000}};
    __u32 freq;
    __u32 pre_scal_id = 0;
    __u32 entire_cycles = 256;
    __u32 active_cycles = 192;
    __u32 entire_cycles_max = 65536;
    __u32 temp;

	if(period_ns < 42) {
		/* if freq lt 24M, then direct output 24M clock */
		temp = sunxi_pwm_read_reg(pwm->pwm * 0x10);
		temp |= (0x1 << 9);//pwm bypass
		sunxi_pwm_write_reg(pwm->pwm * 0x10, temp);

		return 0;
	}

    if(period_ns < 10667)
        freq = 93747;
    else if(period_ns > 1000000000)
        freq = 1;
    else
        freq = 1000000000 / period_ns;

    entire_cycles = 24000000 / freq / pre_scal[pre_scal_id][1];

    while(entire_cycles > entire_cycles_max) {
        pre_scal_id++;

        if(pre_scal_id > 10)
            break;

        entire_cycles = 24000000 / freq / pre_scal[pre_scal_id][1];
        }

    if(period_ns < 5*100*1000)
        active_cycles = (duty_ns * entire_cycles + (period_ns/2)) /period_ns;
    else if(period_ns >= 5*100*1000 && period_ns < 6553500)
        active_cycles = ((duty_ns / 100) * entire_cycles + (period_ns /2 / 100)) / (period_ns/100);
    else
        active_cycles = ((duty_ns / 10000) * entire_cycles + (period_ns /2 / 10000)) / (period_ns/10000);

    temp = sunxi_pwm_read_reg(0);

    if(pwm->pwm == 0)
        temp = (temp & 0xfffffff0) |pre_scal[pre_scal_id][0];
    else
        temp = (temp & 0xfff87fff) |pre_scal[pre_scal_id][0];

    sunxi_pwm_write_reg(0, temp);

    sunxi_pwm_write_reg((pwm->pwm + 1)  * 0x04, ((entire_cycles - 1)<< 16) | active_cycles);

    pwm_debug("PWM _TEST: duty_ns=%d, period_ns=%d, freq=%d, per_scal=%d, period_reg=0x%x\n", duty_ns, period_ns, freq, pre_scal_id, temp);

    #endif

    return 0;
}

static int sunxi_pwm_enable(struct pwm_chip *chip, struct pwm_device *pwm)
{
    __u32 temp;

    #ifndef CONFIG_FPGA_V4_PLATFORM

    int i;
    __u32 ret = 0;
    char pin_name[255];
    __u32 config;

    for(i = 0; i < pwm_pin_count[pwm->pwm]; i++) {
        ret = gpio_request(pwm_cfg[pwm->pwm].list[i].gpio.gpio, NULL);
        if(ret != 0) {
            pr_warn("pwm gpio %d request failed!\n", pwm_cfg[pwm->pwm].list[i].gpio.gpio);
        }
        if(!IS_AXP_PIN(pwm_cfg[pwm->pwm].list[i].gpio.gpio)) {
            sunxi_gpio_to_name(pwm_cfg[pwm->pwm].list[i].gpio.gpio, pin_name);
            config = SUNXI_PINCFG_PACK(SUNXI_PINCFG_TYPE_FUNC, pwm_cfg[pwm->pwm].list[i].gpio.mul_sel);
            pin_config_set(SUNXI_PINCTRL, pin_name, config);
        }
        else {
            pr_warn("this is axp pin!\n");
        }

        gpio_free(pwm_cfg[pwm->pwm].list[i].gpio.gpio);
    }

    #endif

    #if ((defined CONFIG_ARCH_SUN8IW1P1) || (defined CONFIG_ARCH_SUN9IW1P1))

    temp = sunxi_pwm_read_reg(pwm->pwm * 0x10);

    temp |= 1 << 4;
    temp |= 1 << 6;

    sunxi_pwm_write_reg(pwm->pwm * 0x10, temp);

    #elif defined(CONFIG_ARCH_SUN8IW3P1) || defined(CONFIG_ARCH_SUN8IW5P1) || defined(CONFIG_ARCH_SUN8IW6P1) || defined(CONFIG_ARCH_SUN8IW9P1) || defined(CONFIG_ARCH_SUN8IW8P1) || defined(CONFIG_ARCH_SUN8IW7P1)

    temp = sunxi_pwm_read_reg(0);

    if(pwm->pwm == 0) {
        temp |= 1 << 4;
        temp |= 1 << 6;
        } else {
            temp |= 1 << 19;
            temp |= 1 << 21;
            }

    sunxi_pwm_write_reg(0, temp);

    #endif

    return 0;
}

static void sunxi_pwm_disable(struct pwm_chip *chip, struct pwm_device *pwm)
{
    __u32 temp;

    #ifndef CONFIG_FPGA_V4_PLATFORM

    int i;
    __u32 ret = 0;
    char pin_name[255];
    __u32 config;

    for(i = 0; i < pwm_pin_count[pwm->pwm]; i++) {
        ret = gpio_request(pwm_cfg[pwm->pwm].list[i].gpio.gpio, NULL);
        if(ret != 0) {
            pr_warn("pwm gpio %d request failed!\n", pwm_cfg[pwm->pwm].list[i].gpio.gpio);
        }
        if(!IS_AXP_PIN(pwm_cfg[pwm->pwm].list[i].gpio.gpio)) {
            sunxi_gpio_to_name(pwm_cfg[pwm->pwm].list[i].gpio.gpio, pin_name);
            config = SUNXI_PINCFG_PACK(SUNXI_PINCFG_TYPE_FUNC, 0x7);
            pin_config_set(SUNXI_PINCTRL, pin_name, config);
        }
        else {
            pr_warn("this is axp pin!\n");
        }

        gpio_free(pwm_cfg[pwm->pwm].list[i].gpio.gpio);
    }

    #endif

    #if ((defined CONFIG_ARCH_SUN8IW1P1) || (defined CONFIG_ARCH_SUN9IW1P1))

    temp = sunxi_pwm_read_reg(pwm->pwm * 0x10);

    temp &= ~(1 << 4);
    temp &= ~(1 << 6);

    sunxi_pwm_write_reg(pwm->pwm * 0x10, temp);

    #elif defined(CONFIG_ARCH_SUN8IW3P1) || defined(CONFIG_ARCH_SUN8IW5P1) || defined(CONFIG_ARCH_SUN8IW6P1) || defined(CONFIG_ARCH_SUN8IW9P1) || defined(CONFIG_ARCH_SUN8IW8P1) || defined(CONFIG_ARCH_SUN8IW7P1)

    temp = sunxi_pwm_read_reg(0);

    if(pwm->pwm == 0) {
        temp &= ~(1 << 4);
        temp &= ~(1 << 6);
        } else {
            temp &= ~(1 << 19);
            temp &= ~(1 << 21);
            }

    #endif
}

static struct pwm_ops sunxi_pwm_ops = {
	.config = sunxi_pwm_config,
	.enable = sunxi_pwm_enable,
	.disable = sunxi_pwm_disable,
	.set_polarity = sunxi_pwm_set_polarity,
	.owner = THIS_MODULE,
};

static int sunxi_pwm_probe(struct platform_device *pdev)
{
    int ret;
    struct sunxi_pwm_chip*pwm;

    pwm = devm_kzalloc(&pdev->dev, sizeof(*pwm), GFP_KERNEL);
    if(!pwm) {
        dev_err(&pdev->dev, "failed to allocate memory!\n");
        return -ENOMEM;
    }

    platform_set_drvdata(pdev, pwm);

	pwm->chip.dev = &pdev->dev;
	pwm->chip.ops = &sunxi_pwm_ops;
	pwm->chip.base = -1;
	pwm->chip.npwm = PWM_NUM;

	ret = pwmchip_add(&pwm->chip);
	if (ret < 0) {
		dev_err(&pdev->dev, "pwmchip_add() failed: %d\n", ret);
		return ret;
	}

    return 0;
}

static int sunxi_pwm_remove(struct platform_device *pdev)
{
    struct sunxi_pwm_chip *pwm = platform_get_drvdata(pdev);

    return pwmchip_remove(&pwm->chip);
}

static int sunxi_pwm_suspend(struct platform_device *pdev, pm_message_t state)
{
    int i;

    #if ((defined CONFIG_ARCH_SUN8IW1P1) || (defined CONFIG_ARCH_SUN9IW1P1))
    int j = 0;
    for(i = 0; i < PWM_NUM; i++) {
        record_reg[j] = sunxi_pwm_read_reg(i * 0x10);
        j++;
        record_reg[j] = sunxi_pwm_read_reg(i * 0x10 + 0x4);
        j++;
        }

    #elif defined(CONFIG_ARCH_SUN8IW3P1) || defined(CONFIG_ARCH_SUN8IW5P1) || defined(CONFIG_ARCH_SUN8IW6P1) || defined(CONFIG_ARCH_SUN8IW9P1) || defined(CONFIG_ARCH_SUN8IW8P1) || defined(CONFIG_ARCH_SUN8IW7P1)
    for(i = 0; i < PWM_REG_NUM; i++)
        record_reg[i] = sunxi_pwm_read_reg(i * 0x4);

    #endif

    return 0;
}

static int sunxi_pwm_resume(struct platform_device *pdev)
{
    int i;

    #if ((defined CONFIG_ARCH_SUN8IW1P1) || (defined CONFIG_ARCH_SUN9IW1P1))

    int j = 0;
    for(i = 0; i < PWM_NUM; i++) {
        sunxi_pwm_write_reg(i * 0x10, record_reg[j]);
        j++;
        sunxi_pwm_write_reg(i * 0x10 + 0x4, record_reg[j]);
        j++;
        }

    #elif defined(CONFIG_ARCH_SUN8IW3P1) || defined(CONFIG_ARCH_SUN8IW5P1) || defined(CONFIG_ARCH_SUN8IW6P1) || defined(CONFIG_ARCH_SUN8IW9P1) || defined(CONFIG_ARCH_SUN8IW8P1) || defined(CONFIG_ARCH_SUN8IW7P1)
    for(i = 0; i < PWM_REG_NUM; i++)
        sunxi_pwm_write_reg(i * 0x4, record_reg[i]);

    #endif

    return 0;
}


static const struct file_operations sunxi_pwm_fops = {
    .owner = THIS_MODULE,
};


struct platform_device sunxi_pwm_device = {
    .name = "sunxi_pwm",
    .id = -1,
};

static struct platform_driver sunxi_pwm_driver = {
    .driver = {
        .name = "sunxi_pwm",
     },
    .probe = sunxi_pwm_probe,
    .remove = sunxi_pwm_remove,
    .suspend = sunxi_pwm_suspend,
    .resume = sunxi_pwm_resume,
};


static int __init pwm_module_init(void)
{
    int ret = 0;
    int err;
    int i;

    pr_info("pwm module init!\n");
    alloc_chrdev_region(&devid, 0, 1, "sunxi_pwm");
    my_cdev = cdev_alloc();
    cdev_init(my_cdev, &sunxi_pwm_fops);
    err = cdev_add(my_cdev, devid, 1);

    if(err) {
        pr_warn("cdev add fail!\n");
        return -1;
        }

    pwm_class = class_create(THIS_MODULE, "sunxi_pwm");
    if (IS_ERR(pwm_class))
    {
        pr_warn("class_create fail\n");
        return -1;
    }

    pwm_dev = device_create(pwm_class, NULL, devid, NULL, "sunxi_pwm");

    ret = platform_device_register(&sunxi_pwm_device);

    if (ret == 0)
    {
    ret = platform_driver_register(&sunxi_pwm_driver);
    }

    for(i=0; i<PWM_NUM; i++) {
        sunxi_pwm_get_sys_config(i, &pwm_cfg[i]);
    }

    return ret;

}

static void __exit pwm_module_exit(void)
{
    pr_info("pwm module exit!\n");

    platform_driver_unregister(&sunxi_pwm_driver);
    platform_device_unregister(&sunxi_pwm_device);

    device_destroy(pwm_class,  devid);
    class_destroy(pwm_class);

    cdev_del(my_cdev);
}

subsys_initcall(pwm_module_init);
module_exit(pwm_module_exit);

MODULE_AUTHOR("jiatl");
MODULE_DESCRIPTION("pwm driver");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:sunxi-pwm");
