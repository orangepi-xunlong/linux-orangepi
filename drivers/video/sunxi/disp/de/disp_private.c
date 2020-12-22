#include "disp_private.h"

s32 bsp_disp_delay_ms(u32 ms)
{
#if defined(__LINUX_PLAT__)
	u32 timeout = msecs_to_jiffies(ms);

	set_current_state(TASK_UNINTERRUPTIBLE);
	schedule_timeout(timeout);
#endif
#ifdef __BOOT_OSAL__
	wBoot_timer_delay(ms);//assume cpu runs at 1000Mhz,10 clock one cycle
#endif
#ifdef __UBOOT_OSAL__
    __msdelay(ms);
#endif
	return 0;
}


s32 bsp_disp_delay_us(u32 us)
{
#if defined(__LINUX_PLAT__)
	udelay(us);
#endif
#ifdef __BOOT_OSAL__
	volatile u32 time;

	for(time = 0; time < (us*700/10);time++);//assume cpu runs at 700Mhz,10 clock one cycle
#endif
#ifdef __UBOOT_OSAL__
    __usdelay(us);
#endif
	return 0;
}

static struct disp_notifier_block disp_notifier_list;

s32 disp_notifier_init(void)
{
	INIT_LIST_HEAD(&disp_notifier_list.list);
	return 0;
}

s32 disp_notifier_register(struct disp_notifier_block *nb)
{
	if((NULL == nb)) {
		DE_WRN("hdl is NULL\n");
		return -1;
	}
	list_add_tail(&(nb->list), &(disp_notifier_list.list));
	return 0;
}

s32 disp_notifier_unregister(struct disp_notifier_block *nb)
{
	struct disp_notifier_block *ptr;
	if((NULL == nb)) {
		DE_WRN("hdl is NULL\n");
		return -1;
	}
	list_for_each_entry(ptr, &disp_notifier_list.list, list) {
		if(ptr == nb) {
			list_del(&ptr->list);
			return 0;
		}
	}
	return -1;
}

s32 disp_notifier_call_chain(u32 event, u32 sel, void *v)
{
	struct disp_notifier_block *ptr;
	list_for_each_entry(ptr, &disp_notifier_list.list, list) {
		if(ptr->notifier_call)
			ptr->notifier_call(ptr, event, sel, v);
	}

	return 0;
}

