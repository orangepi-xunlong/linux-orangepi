#include <linux/input.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/interrupt.h>
#include <mach/gpio.h>
#include <linux/gpio.h>
#include <linux/timer.h>
#include <mach/sys_config.h>
#include <linux/spinlock_types.h>
#include <mach/sun8i/platform-sun8iw7p1.h>
#include <linux/err.h>
#include <linux/jiffies.h>
#include <linux/hrtimer.h>
#include <linux/kernel.h>

#define KEY_MIC_VOLUMEUP    370
#define KEY_MIC_VOLUMEDOWN  371
#define KEY_KOUT_VOLUMEUP   372
#define KEY_KOUT_VOLUMEDOWN 373

#define readl(addr) (*((volatile unsigned long *)(addr)))
#define writel(v, addr) (*((volatile unsigned long *)(addr)) =(unsigned long )(v))
#define IO_ADDRESS(x) ((x) + 0xf0000000)

static struct __shaft_encoder_gpio {
        u32 speaker_change_gpio;
        u32 speaker_level_gpio;
        u32 mic_change_gpio;
        u32 mic_level_gpio;
}se_gpio;

static struct input_dev *speaker_vol_input_dev = NULL;
static unsigned int speaker_irq_count = 0;
static unsigned int speaker_direction_irq_count = 0;
static unsigned int speaker_cw_count = 0;
static unsigned int speaker_ccw_count = 0;
static unsigned int speaker_change_flag = 0;
static unsigned int speaker_prev_level = 1;
static unsigned int speaker_cur_level = 1;
static int speaker_direction = -1;
static struct hrtimer speaker_timer;

static struct input_dev *mic_vol_input_dev = NULL;
static unsigned int mic_irq_count = 0;
static unsigned int mic_direction_irq_count = 0;
static unsigned int mic_cw_count = 0;
static unsigned int mic_ccw_count = 0;
static unsigned int mic_change_flag = 0;
static unsigned int mic_prev_level = 1;
static unsigned int mic_cur_level = 1;
static int mic_direction = -1;
static struct hrtimer mic_timer;

static ktime_t se_kt;

spinlock_t se_lock;

static enum hrtimer_restart speaker_hrtimer_handler(struct hrtimer *timer)
{
        if (speaker_direction == 0) {
                input_report_key(speaker_vol_input_dev, KEY_KOUT_VOLUMEUP, 0);
                input_sync(speaker_vol_input_dev);
        } else if (speaker_direction == 1) {
                input_report_key(speaker_vol_input_dev, KEY_KOUT_VOLUMEDOWN, 0);
                input_sync(speaker_vol_input_dev);
        }
        return HRTIMER_NORESTART;
}

static enum hrtimer_restart mic_hrtimer_handler(struct hrtimer *timer)
{
        if (mic_direction == 0) {
                input_report_key(mic_vol_input_dev, KEY_MIC_VOLUMEUP, 0);
                input_sync(mic_vol_input_dev);
        } else if (mic_direction == 1) {
                input_report_key(mic_vol_input_dev, KEY_MIC_VOLUMEDOWN, 0);
                input_sync(mic_vol_input_dev);
        }
        return HRTIMER_NORESTART;
}

irqreturn_t speaker_encoder_trigger_int(int val, void *args)
{
        unsigned long flags;
        spin_lock_irqsave(&se_lock, flags);
        speaker_change_flag = 1;
        /* check the direction level */
        speaker_prev_level = gpio_get_value(se_gpio.speaker_level_gpio);
        speaker_irq_count++;
        spin_unlock_irqrestore(&se_lock, flags);
        return 0;
}

irqreturn_t speaker_encoder_direction_int(int val, void *args)
{
        unsigned long flags;
        spin_lock_irqsave(&se_lock, flags);
        if (speaker_change_flag) {
                speaker_direction_irq_count++;
                speaker_change_flag = 0;
                speaker_cur_level = gpio_get_value(se_gpio.speaker_level_gpio);
                if (speaker_prev_level == 0) {
                        /* send volume up event */
                        if (speaker_vol_input_dev && speaker_cur_level == 1) {
                                speaker_cw_count++;
                                speaker_direction = 0;
                                input_report_key(speaker_vol_input_dev, KEY_KOUT_VOLUMEUP, 1);
                                hrtimer_start(&speaker_timer, se_kt, HRTIMER_MODE_REL);
                        }
                } else {
                        /* send volume down event */
                        if (speaker_vol_input_dev && speaker_cur_level == 0) {
                                speaker_ccw_count++;
                                speaker_direction = 1;
                                input_report_key(speaker_vol_input_dev, KEY_KOUT_VOLUMEDOWN, 1);
                                hrtimer_start(&speaker_timer, se_kt, HRTIMER_MODE_REL);
                        }
                }
        }
        spin_unlock_irqrestore(&se_lock, flags);
        return 0;
}

irqreturn_t mic_encoder_trigger_int(int val, void *args)
{
        unsigned long flags;
         spin_lock_irqsave(&se_lock, flags);
        mic_change_flag = 1;
        /* check the direction level */
        mic_prev_level = gpio_get_value(se_gpio.mic_level_gpio);
        mic_irq_count++;
        spin_unlock_irqrestore(&se_lock, flags);
        return 0;
}

irqreturn_t mic_encoder_direction_int(int val, void *args)
{
        unsigned long flags;
        spin_lock_irqsave(&se_lock, flags);
        if (mic_change_flag) {
                mic_direction_irq_count++;
                mic_change_flag = 0;
                mic_cur_level = gpio_get_value(se_gpio.mic_level_gpio);
                if (mic_prev_level == 0) {
                        /* send volume up event */
                        if (mic_vol_input_dev && mic_cur_level == 1) {
                                mic_cw_count++;
                                mic_direction = 0;
                                input_report_key(mic_vol_input_dev, KEY_MIC_VOLUMEUP, 1);
                                hrtimer_start(&mic_timer, se_kt, HRTIMER_MODE_REL);
                        }
                } else {
                        /* send volume down event */
                        if (mic_vol_input_dev && mic_cur_level == 0) {
                                mic_ccw_count++;
                                mic_direction = 1;
                                input_report_key(mic_vol_input_dev, KEY_MIC_VOLUMEDOWN, 1);
                                hrtimer_start(&mic_timer, se_kt, HRTIMER_MODE_REL);
                        }
                }
        }
        spin_unlock_irqrestore(&se_lock, flags);
        return 0;
}


static ssize_t shaft_encoder_show(struct class *class, struct class_attribute *attr, char *buf)
{

        int cnt = 3;

        buf[1] = '\n';
        buf[2] = '\0';

        return cnt;
}

static ssize_t shaft_encoder_store(struct class *class, struct class_attribute *attr,
                              const char *buf, size_t size)
{

        if(strlen(buf) >= 3) {
                pr_err("%s(%d) err: name \"%s\" too long\n", __func__, __LINE__, buf);
                return -EINVAL;
        }
        if ('d' == buf[0]) {
                printk(KERN_ERR "+++++++++++++++ SPEAKER +++++++++\n");
                printk(KERN_ERR "trigger:%u direction:%u\n", speaker_irq_count, speaker_direction_irq_count);
                printk(KERN_ERR "prev data: %u-%u\n", speaker_prev_level, speaker_cur_level);
                printk(KERN_ERR "cur data: %u\n", gpio_get_value(se_gpio.speaker_level_gpio));
                printk(KERN_ERR "cw_count=%d,ccw_count=%d\n", speaker_cw_count, speaker_ccw_count);

                printk(KERN_ERR "+++++++++++++++ MIC +++++++++\n");
                printk(KERN_ERR "trigger:%u direction:%u\n", mic_irq_count, mic_direction_irq_count);
                printk(KERN_ERR "prev data: %u-%u\n", mic_prev_level, mic_cur_level);
                printk(KERN_ERR "cur data: %u\n", gpio_get_value(se_gpio.mic_level_gpio));
                printk(KERN_ERR "cw_count=%d,ccw_count=%d\n", mic_cw_count, mic_ccw_count);
        }

        return size;
}

static struct class_attribute shaft_encoder_class_attrs[] =
{

        __ATTR(shaft_encoder, 0664, shaft_encoder_show, shaft_encoder_store),
        __ATTR_NULL,
};


static struct class shaft_encoder_class =
{

        .name = "shaft_encoder",
        .owner = THIS_MODULE,
        .class_attrs = shaft_encoder_class_attrs,
};


static int __init shaft_encoder_sysfs_init(void)
{

        int status;

        status = class_register(&shaft_encoder_class);
        if (status < 0)
                pr_err("%s: status %d\n", __func__, status);
        else
                pr_info("%s success\n", __func__);

        return status;
}

static int  shaft_encoder_platform_probe(struct platform_device *pdev)
{
        int virq, ret = -1;
        int cnt, used=0;
        script_item_u val;
        script_item_u   *list = NULL;
        script_item_value_type_e  type;

#if 1
        type = script_get_item("shaft_encoder", "used", &val);
        if(SCIRPT_ITEM_VALUE_TYPE_INT != type){
                printk(KERN_ERR "failed to get gpio_para used information\n");
                goto INIT_END;
        }
        used = val.val;
        if(!used){
                printk(KERN_ERR "this module is used not!\n");
                goto INIT_END;
        }

        cnt = script_get_pio_list("shaft_encoder", &list);
        if(cnt != 4){
                printk(KERN_ERR "invalid number for gpio\n");
                goto INIT_END;
        }

        se_gpio.speaker_change_gpio = list[0].gpio.gpio;
        se_gpio.speaker_level_gpio = list[1].gpio.gpio;
        se_gpio.mic_change_gpio = list[2].gpio.gpio;
        se_gpio.mic_level_gpio = list[3].gpio.gpio;
        printk(KERN_ERR "%s:%d shaft encoder sys_config init ok\n", __func__, __LINE__);
#endif

        /* se_gpio.speaker_change_gpio = GPIOA(0); */
        /* se_gpio.speaker_level_gpio = GPIOA(1); */
        /* se_gpio.mic_change_gpio = GPIOA(2); */
        /* se_gpio.mic_level_gpio = GPIOA(3); */
        virq = gpio_to_irq(se_gpio.speaker_change_gpio);
        if (IS_ERR_VALUE(virq)) {
                printk(KERN_ERR "map gpio [%d] to virq failed, errno = %d\n",
                        se_gpio.speaker_change_gpio, virq);
                return -EINVAL;
        }
        ret = devm_request_irq(&pdev->dev, virq, speaker_encoder_trigger_int, \
                               IRQF_TRIGGER_FALLING, "PA0_EINT", NULL);
        if (IS_ERR_VALUE(ret)) {
                pr_warn("request virq %d failed, errno = %d\n", virq, ret);
                return -EINVAL;
        }
        virq = gpio_to_irq(se_gpio.speaker_level_gpio);
        if (IS_ERR_VALUE(virq)) {
                printk(KERN_ERR "map gpio [%d] to virq failed, errno = %d\n",
                        se_gpio.speaker_level_gpio, virq);
                return -EINVAL;
        }
        ret = devm_request_irq(&pdev->dev, virq, speaker_encoder_direction_int, \
                               IRQF_TRIGGER_FALLING | IRQF_TRIGGER_RISING, "PA1_EINT", NULL);
        if (IS_ERR_VALUE(ret)) {
                printk("request virq %d failed, errno = %d\n", virq, ret);
                return -EINVAL;
        }

        virq = gpio_to_irq(se_gpio.mic_change_gpio);
        if (IS_ERR_VALUE(virq)) {
                printk(KERN_ERR "map gpio [%d] to virq failed, errno = %d\n",
                        se_gpio.mic_change_gpio, virq);
                return -EINVAL;
        }
        ret = devm_request_irq(&pdev->dev, virq, mic_encoder_trigger_int, \
                               IRQF_TRIGGER_FALLING, "PA2_EINT", NULL);
        if (IS_ERR_VALUE(ret)) {
                pr_warn("request virq %d failed, errno = %d\n", virq, ret);
                return -EINVAL;
        }
        virq = gpio_to_irq(se_gpio.mic_level_gpio);
        if (IS_ERR_VALUE(virq)) {
                printk(KERN_ERR "map gpio [%d] to virq failed, errno = %d\n",
                        se_gpio.mic_level_gpio, virq);
                return -EINVAL;
        }
        ret = devm_request_irq(&pdev->dev, virq, mic_encoder_direction_int, \
                               IRQF_TRIGGER_FALLING | IRQF_TRIGGER_RISING, "PA3_EINT", NULL);
        if (IS_ERR_VALUE(ret)) {
                printk("request virq %d failed, errno = %d\n", virq, ret);
                return -EINVAL;
        }

        speaker_vol_input_dev = input_allocate_device();
        if (!speaker_vol_input_dev) {
                printk(KERN_ERR "speaker not enough memory for input device\n");
                return -ENOMEM;
        }
        speaker_vol_input_dev->name = "sunxi-speaker-vol";
        speaker_vol_input_dev->phys = "ShaftEncoder/input1";
        speaker_vol_input_dev->id.bustype = BUS_HOST;
        speaker_vol_input_dev->id.vendor = 0x0001;
        speaker_vol_input_dev->id.product = 0x0001;
        speaker_vol_input_dev->id.version = 0x0100;

        speaker_vol_input_dev->evbit[0] = BIT_MASK(EV_KEY);
        set_bit(KEY_KOUT_VOLUMEUP, speaker_vol_input_dev->keybit);
        set_bit(KEY_KOUT_VOLUMEDOWN, speaker_vol_input_dev->keybit);
        ret =  input_register_device(speaker_vol_input_dev);
        if (ret) {
                printk(KERN_ERR "register sunxi-speaker-vol input-dev fail\n");
                goto INIT_END;
        }

        mic_vol_input_dev = input_allocate_device();
        if (!mic_vol_input_dev) {
                printk(KERN_ERR "mic: not enough memory for input device\n");
                return -ENOMEM;
        }
        mic_vol_input_dev->name = "sunxi-mic-vol";
        mic_vol_input_dev->phys = "ShaftEncoder/input2";
        mic_vol_input_dev->id.bustype = BUS_HOST;
        mic_vol_input_dev->id.vendor = 0x0001;
        mic_vol_input_dev->id.product = 0x0001;
        mic_vol_input_dev->id.version = 0x0100;

        mic_vol_input_dev->evbit[0] = BIT_MASK(EV_KEY);
        set_bit(KEY_MIC_VOLUMEUP, mic_vol_input_dev->keybit);
        set_bit(KEY_MIC_VOLUMEDOWN, mic_vol_input_dev->keybit);
        ret =  input_register_device(mic_vol_input_dev);
        if (ret) {
                printk(KERN_ERR "register sunxi-mic-vol input-dev fail\n");
                goto INIT_END;
        }

        se_kt = ktime_set(0, 1000);
        hrtimer_init(&speaker_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
        speaker_timer.function = speaker_hrtimer_handler;

        hrtimer_init(&mic_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
        mic_timer.function = mic_hrtimer_handler;

        spin_lock_init(&se_lock);
        shaft_encoder_sysfs_init();
        printk(KERN_ERR "%s:%d shaft encoder init ok\n", __func__, __LINE__);

INIT_END:
        return ret;
}

static struct platform_device shaft_encoder_platform_device = {
        .name               = "shaft_encoder",
        .id                 = PLATFORM_DEVID_NONE,
};

static struct platform_driver shaft_encoder_platform_driver = {
        .probe          = shaft_encoder_platform_probe,
        .driver         = {
                .name   = "shaft_encoder",
                .owner  = THIS_MODULE,
        },
};

int __init shaft_encoder_init(void)
{
        int ret;

        ret = platform_driver_register(&shaft_encoder_platform_driver);
        if (IS_ERR_VALUE(ret)) {
                printk("register shaft encoder platform driver failed\n");
                return ret;
        }
        ret = platform_device_register(&shaft_encoder_platform_device);
        if (IS_ERR_VALUE(ret)) {
                printk("register shaft encoder platform device failed\n");
                return ret;
        }
        return ret;
}

void __exit shaft_encoder_exit(void)
{

}

module_init(shaft_encoder_init);
module_exit(shaft_encoder_exit);

MODULE_AUTHOR("Bill Guo");
MODULE_LICENSE("Dual BSD/GPL");

