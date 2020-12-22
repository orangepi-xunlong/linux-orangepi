/*
*********************************************************************************************************
*                                                    LINUX-KERNEL
*                                        AllWinner Linux Platform Develop Kits
*                                                   Kernel Module
*
*                                    (c) Copyright 20014-2015, Ming Li China
*                                             All Rights Reserved
*
* File    : mian.c
* By      : Ming Li
* Version : v1.0
* Date    : 2014-11-13 17:01
* Descript: hibernate for allwinners chips platform.
* Update  : date                auther      ver     notes
*********************************************************************************************************
*/

#include <linux/module.h>
#include <linux/suspend.h>
#include <linux/reboot.h>
#include "../pm/pm_debug.h"

#define AW_PM_DBG   1
#undef PM_DBG
#if(AW_PM_DBG)
    #define PM_DBG(format,args...)   printk("[pm]"format,##args)
#else
    #define PM_DBG(format,args...)   do{}while(0)
#endif

extern standby_type_e standby_type;

extern standby_level_e standby_level;

extern void disable_hlt(void);
extern void enable_hlt(void);

/*
*********************************************************************************************************
*                           aw_pm_begin
*
*Description: Initialise a transition to given system sleep state;
*
*Arguments  : state     suspend state;
*
*Return     : return 0 for process successed;
*
*Notes      : this is a call-back function, registered into PM core, and this function
*             will be called before devices suspened;
*********************************************************************************************************
*/
static int aw_pm_begin(void)
{
	PM_DBG("aw_pm_begin\n");

	standby_type = SUPER_STANDBY;

	return 0;
}


/*
*********************************************************************************************************
*                           aw_pm_prepare
*
*Description: Prepare the platform for entering the system sleep state.
*
*Arguments  : none;
*
*Return     : return 0 for process successed, and negative code for error;
*
*Notes      : this is a call-back function, registered into PM core, this function
*             will be called after devices suspended, and before device late suspend
*             call-back functions;
*********************************************************************************************************
*/
static int aw_pm_prepare(void)
{
	PM_DBG("prepare\n");
	disable_hlt();
	return 0;
}


/*
*********************************************************************************************************
*                           aw_pm_prepare_late
*
*Description: Finish preparing the platform for entering the system sleep state.
*
*Arguments  : none;
*
*Return     : return 0 for process successed, and negative code for error;
*
*Notes      : this is a call-back function, registered into PM core.
*             prepare_late is called before disabling nonboot CPUs and after
*              device drivers' late suspend callbacks have been executed;
*********************************************************************************************************
*/
/*static int aw_pm_prepare_late(void)
{
	PM_DBG("prepare_late\n");

	return 0;
}*/

/*
*********************************************************************************************************
*                           aw_pm_enter
*
*Description: Enter the system sleep state;
*
*Arguments  : state     system sleep state;
*
*Return     : return 0 is process successed;
*
*Notes      : this function is the core function for platform sleep.
*********************************************************************************************************
*/
static int aw_pm_enter(void)
{
	PM_DBG("aw_pm_enter\n");
	machine_power_off();

	return 0;
}


/*
*********************************************************************************************************
*                           aw_pm_wake
*
*Description: platform wakeup;
*
*Arguments  : none;
*
*Return     : none;
*
*Notes      : This function called when the system has just left a sleep state, right after
*             the nonboot CPUs have been enabled and before device drivers' early resume
*             callbacks are executed. This function is opposited to the aw_pm_prepare_late;
*********************************************************************************************************
*/
static int aw_pm_pre_snapshot(void)
{
	PM_DBG("aw_pm_pre_snapshot.\n");
	return 0;
}

/*
*********************************************************************************************************
*                           aw_pm_finish
*
*Description: Finish wake-up of the platform;
*
*Arguments  : none
*
*Return     : none
*
*Notes      : This function is called right prior to calling device drivers' regular suspend
*              callbacks. This function is opposited to the aw_pm_prepare function.
*********************************************************************************************************
*/
static void aw_pm_finish(void)
{
	enable_hlt();
	PM_DBG("aw_pm_finish\n");
	return;
}

static void aw_pm_leave(void)
{
	PM_DBG("aw_pm_leave\n");
	return;
}



/*
*********************************************************************************************************
*                           aw_pm_end
*
*Description: Notify the platform that system is in work mode now.
*
*Arguments  : none
*
*Return     : none
*
*Notes      : This function is called by the PM core right after resuming devices, to indicate to
*             the platform that the system has returned to the working state or
*             the transition to the sleep state has been aborted. This function is opposited to
*             aw_pm_begin function.
*********************************************************************************************************
*/
static void aw_pm_end(void)
{
	PM_DBG("aw_pm_end!\n");
	return;
}


/*
*********************************************************************************************************
*                           aw_pm_recover
*
*Description: Recover platform from a suspend failure;
*
*Arguments  : none
*
*Return     : none
*
*Notes      : This function alled by the PM core if the suspending of devices fails.
*             This callback is optional and should only be implemented by platforms
*             which require special recovery actions in that situation.
*********************************************************************************************************
*/
static void aw_pm_recover(void)
{
	PM_DBG("aw_pm_recover\n");
	return;
}

static int aw_pm_pre_restore(void)
{
	PM_DBG("aw_pm_pre_restore\n");

	return 0;
}

static void aw_pm_restore_cleanup(void)
{
	PM_DBG("aw_pm_restore_cleanup\n");
	return;
}

/*
    define platform_suspend_ops which is registered into PM core.
*/
static struct platform_hibernation_ops aw_pm_ops = {
	.begin = aw_pm_begin,
	.end = aw_pm_end,
	.pre_snapshot = aw_pm_pre_snapshot,
	.finish = aw_pm_finish,
	.prepare = aw_pm_prepare,
	.enter = aw_pm_enter,
	.leave = aw_pm_leave,
	.pre_restore = aw_pm_pre_restore,
	.restore_cleanup = aw_pm_restore_cleanup,
	.recover = aw_pm_recover,
};

/*
*********************************************************************************************************
*                           aw_pm_init
*
*Description: initial pm sub-system for platform;
*
*Arguments  : none;
*
*Return     : result;
*
*Notes      :
*
*********************************************************************************************************
*/
static int __init aw_pm_init(void)
{
	PM_DBG("aw_pm_init!\n");

	hibernation_set_ops(&aw_pm_ops);

	return 0;
}


/*
*********************************************************************************************************
*                           aw_pm_exit
*
*Description: exit pm sub-system on platform;
*
*Arguments  : none
*
*Return     : none
*
*Notes      :
*
*********************************************************************************************************
*/
static void __exit aw_pm_exit(void)
{
	PM_DBG("aw_pm_exit!\n");
	hibernation_set_ops(NULL);
}

module_init(aw_pm_init);
module_exit(aw_pm_exit);

