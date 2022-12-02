#include "BT_GPIO.h"
MODULE_LICENSE("GPL");
MODULE_AUTHOR("ChalesYu");
MODULE_DESCRIPTION("Pull UP RTL8822BS BT_EN Pin");
MODULE_ALIAS("8822BS_BT_EN"); 

static  int  rtl8822bs_BT_init(void)

{
	printk("8822BS_BT: rtl8822bs_BT_init()\n");
	rtl8822bs_BT_EN_PIN_init();
	//BT_EN_PIN_export();
	return 0;
}
 

static  void  rtl8822bs_BT_exit(void)

{
	//BT_EN_PIN_unexport();
	rtl8822bs_BT_EN_PIN_exit();
        printk("8822BS_BT: rtl8822bs_BT_exit()\n");
}

module_init(rtl8822bs_BT_init);
module_exit(rtl8822bs_BT_exit);
