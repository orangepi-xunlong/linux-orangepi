/* 
 * 
 * non-secure main function
 */

#include <linux/clk.h>
#include <linux/clockchips.h>
#include <linux/interrupt.h>
#include <linux/module.h>
#include <linux/irq.h>
#include <linux/irqreturn.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>
#include <linux/sunxi_timer.h>
#include <linux/clk/sunxi.h>
#include <linux/delay.h>
#include <linux/kthread.h>
#include <mach/platform.h>
#include <mach/irqs.h>
#include <mach/sunxi-smc.h>
#include <asm/cacheflush.h>
#include <asm/mcpm.h>
#include <asm/proc-fns.h>
#include <asm/cacheflush.h>
#include <asm/cputype.h>
#include <asm/cp15.h>
#include <asm/smp_plat.h>
#include <asm/bL_switcher.h>
#include <asm/hardware/gic.h>
#include <linux/secure/te_protocol.h>
#include <linux/secure/te_rpc.h>
#include "types.h"

static u32 do_smc_command(u32 arg0, u32 arg1, u32 arg2, u32 arg3)
{
	struct smc_param param;
	
	param.a0 = arg0;
	param.a1 = arg1;
	param.a2 = arg2;
	param.a3 = arg3;
	while (true) {
		sunxi_smc_call(&param);
		if (TEE_SMC_IS_RPC_CALL(param.a0)) {
			/* handle RPC command */
			te_rpc_handle(&param);
			
			/* return back to secure again */
			continue;
		} else {
			/* breakout for smc command return */
			break;
		}
	}
	
	return param.a0;
}

static int do_smc_request(struct te_request *req)
{
	u32 arg0 = (u32)(req->type);
	u32 arg1 = (u32)(virt_to_phys(req));
	u32 arg2 = (u32)(virt_to_phys(req->params));
	u32 arg3 = 0;
	
    	return do_smc_command(arg0, arg1, arg2, arg3);
}

struct te_request      request1;
struct te_oper_param   params[4];
char                   buffer[256];
struct te_request      request2;

int te_test_thread(void *arg)
{
	int i;
	unsigned int mpidr, cpu, cluster;
	struct te_request *req1 = &request1;
	struct te_request *req2 = &request2;
	
	pr_info("%s: enter...\n", __func__);
	
	mpidr = read_cpuid_mpidr();
	cpu = MPIDR_AFFINITY_LEVEL(mpidr, 0);
	cluster = MPIDR_AFFINITY_LEVEL(mpidr, 1);
	pr_info("%s: cluster-%x cpu-%x\n", __func__, cluster, cpu);
	
	memset(req1, 0, sizeof(struct te_request));
	memset(req2, 0, sizeof(struct te_request));
	memset(params, 0, sizeof(struct te_oper_param) * 4);
	
	/* open session of test1 */
	req1->type = TEE_SMC_OPEN_SESSION;
	req1->params = params;
	req1->params_size = 0;
	req1->dest_uuid[0] = 0xF0F0F0F0;
	do_smc_request(req1);
	pr_info("open session of test1 ta ok\n");
	
	/* open session of test2 */
	req2->type = TEE_SMC_OPEN_SESSION;
	req2->params = params;
	req2->params_size = 0;
	req2->dest_uuid[0] = 0xF0F0F0E0;
	do_smc_request(req2);
	pr_info("open session test2 ta ok\n");
	
	/* invoke command */
	for (i = 0; i < 0x100; i++) {
		
		/* call test1 ta */
		params[0].type = TE_PARAM_TYPE_INT_RW;
		params[0].u.Int.val_a = 0xFAFAFAFA;
		params[0].u.Int.val_b = 0xFEFEFEFE;
		
		strcpy(buffer, "share memory buffer from non-secure world test-ta1");
		params[1].type = TE_PARAM_TYPE_MEM_RW;
		params[1].u.Mem.base = buffer;
		params[1].u.Mem.phys = (void *)(virt_to_phys(buffer));
		params[1].u.Mem.len = 256;
		
		req1->type = TEE_SMC_INVOKE_COMMAND;
		req1->params = params;
		req1->params_size = 2;
		req1->dest_uuid[0] = 0xF0F0F0F0;
		req1->command_id = i;
		do_smc_request(req1);
		pr_info("invoke command-%x ok\n", i);
		pr_info("%s\n", buffer);
		
		/* call test2 ta */
		params[0].type = TE_PARAM_TYPE_INT_RW;
		params[0].u.Int.val_a = 0xFAFAFAFA;
		params[0].u.Int.val_b = 0xFEFEFEFE;
		
		strcpy(buffer, "share memory buffer from non-secure world test-ta2");
		params[1].type = TE_PARAM_TYPE_MEM_RW;
		params[1].u.Mem.base = buffer;
		params[1].u.Mem.phys = (void *)(virt_to_phys(buffer));
		params[1].u.Mem.len = 256;
		
		req2->type = TEE_SMC_INVOKE_COMMAND;
		req2->params = params;
		req2->params_size = 2;
		req2->dest_uuid[0] = 0xF0F0F0E0;
		req2->command_id = i;
		do_smc_request(req2);
		pr_info("invoke command-%x ok\n", i);
		pr_info("%s\n", buffer);
	}
	
	/* close test2 ta session */
	req2->type = TEE_SMC_INVOKE_COMMAND;
	req2->params = params;
	req2->params_size = 0;
	req2->dest_uuid[0] = 0xF0F0F0E0;
	do_smc_request(req2);
	pr_info("close test1 ta command ok\n");
	
	/* close test1 ta session */
	req1->type = TEE_SMC_INVOKE_COMMAND;
	req1->params = params;
	req1->params_size = 0;
	req1->dest_uuid[0] = 0xF0F0F0F0;
	do_smc_request(req1);
	pr_info("close test1 ta command ok\n");
	
	return 0;
}

static struct task_struct *te_task;
int __init te_test_init(void)
{
	/* create test task and bind to cpu0 */
	pr_info("%s: enter..\n", __func__);
	te_task = kthread_create(te_test_thread, NULL, "te_test_thread");
	kthread_bind(te_task, 0);
	
	get_task_struct(te_task);
	wake_up_process(te_task);
	
	pr_info("%s: exit..\n", __func__);
	return 0;
}
late_initcall(te_test_init);
