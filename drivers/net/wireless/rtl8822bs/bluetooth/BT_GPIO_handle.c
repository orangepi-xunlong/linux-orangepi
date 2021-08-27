#include "BT_GPIO.h"

int BT_EN_PIN_export(void)
{
	int ret;
	ret=gpio_export(BT_EN, true);
	if(ret)
		{
		printk("8822BS_BT: gpio_export_fail!\n");
		return -1;
		}
	printk("8822BS_BT: gpio_export_success!\n");
	return 0;
}

int BT_EN_PIN_unexport(void)
{
	gpio_unexport(BT_EN);
	printk("8822BS_BT: gpio_unexport(BT_EN) called!\n");
	return 0;
}



int rtl8822bs_BT_EN_PIN_pullup(void)
{
	int ret;
	int a;
	ret=gpio_direction_output(BT_EN, 1);
	if(ret)
		{
		printk("8822BS_BT: gpio_direction_output_fail!\n");
		return -1;
		}
	gpio_set_value(BT_EN,1);
	ssleep (1);
	a=gpio_get_value(BT_EN);
	printk("8822BS_BT: BT_EN: %d\n", a);
#ifdef DBG
	printk("8822BS_BT: gpio_pullup_success!\n");
#endif
	return 0;
}

int rtl8822bs_BT_EN_PIN_pulldown(void)
{
	int ret;
	int a;
	ret=gpio_direction_output(BT_EN, 0);
	if(ret)
		{
		printk("8822BS_BT: gpio_direction_output_fail!\n");
		return -1;
		}
	gpio_set_value(BT_EN,0);
	ssleep (1);
	a=gpio_get_value(BT_EN);
	printk("8822BS_BT: BT_EN: %d\n", a);
#ifdef DBG
	printk("8822BS_BT: gpio_pulldown_success!\n");
#endif
	return 0;
}


int rtl8822bs_BT_EN_PIN_init(void)
{
	int ret;
	ret=gpio_request(BT_EN,NULL);
	if(ret)
		{
		printk("8822BS_BT: gpio_init_fail!\n");
		return -1;
		}
	printk("8822BS_BT: gpio_init_success!\n");
	rtl8822bs_BT_EN_PIN_pullup();
	return 0;
}

int rtl8822bs_BT_EN_PIN_exit(void)
{
	rtl8822bs_BT_EN_PIN_pulldown();
	gpio_free(BT_EN);
#ifdef DBG
	printk("8822BS_BT: gpio_free(BT_EN) called!\n");
#endif
	return 0;
}