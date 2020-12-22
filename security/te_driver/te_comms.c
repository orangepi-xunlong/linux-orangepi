/*
 * Copyright (C) 2014 huangshr <huangshr@allwinnertech.com>
 *
 *	This program is free software; you can redistribute it and/or modify
 *  	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation, version 2.
 *
 * Authors:
 * 	huangshr <huangshr@allwinnertech.com>
 *
 */

#include <linux/atomic.h>
#include <linux/uaccess.h>
#include <linux/fs.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/printk.h>
#include <linux/ioctl.h>
#include <linux/sched.h>
#include <linux/mm.h>
#include <linux/pagemap.h>
#include <linux/slab.h>
#include <asm/cacheflush.h>
#include <asm/firmware.h>
#include <linux/smp.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/pagemap.h>
#include <linux/init.h>
#include <linux/io.h>
#include <linux/export.h>
#include <linux/time.h>
#include <mach/sunxi-smc.h>
#include <linux/secure/te_protocol.h>
#include <linux/secure/te_rpc.h>
#include "te_shmem.h"

bool verbose_smc;
DEFINE_RAW_SPINLOCK(sunxi_smc_kernel_lock);
core_param(verbose_smc, verbose_smc, bool, 0644);

#define ROUND_UP(N, S) ((((N) + (S) - 1) / (S)) * (S))
#define SET_RESULT(req, r, ro)	{ req->result = r; req->result_origin = ro; }

/*generic smc for cpu 0*/
u32  sunxi_smc_call(struct smc_param *param)
{	
	unsigned int cpu_id;

	cpu_id = get_cpu();
	if(cpu_id != 0)
		panic("CPU %d try to enter generic smc call which fix to CPU 0\n");
	put_cpu();

	return __sunxi_smc_call(param);
}

u32 te_smc_command_send(u32 arg0, u32 arg1, u32 arg2, u32 arg3)
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

struct smc_cdata {
    u32 arg0;
    u32 arg1;
    u32 arg2;
    u32 arg3;
    u32 ret_val;
};

static struct smc_cdata tee_smc_cd[NR_CPUS];
extern raw_spinlock_t sunxi_smc_kernel_lock;

void tee_secondary_smc_handle(void *info)
{
    struct smc_cdata *cd = (struct smc_cdata *)info;
    rmb();
    cd->ret_val = te_smc_command_send(cd->arg0,cd->arg1,cd->arg2, cd->arg3);
    wmb();
}

u32 tee_post_smc(int cpu_id, u32 arg0, u32 arg1, u32 arg2, u32 arg3)
{
    struct smc_cdata *cd = &tee_smc_cd[cpu_id];
    cd->arg0 = arg0;
    cd->arg1 = arg1;
    cd->arg2 = arg2;
    cd->arg3 = arg3;
    cd->ret_val = 0;
    wmb();
    smp_call_function_single(0, tee_secondary_smc_handle, (void *)cd, 1);
    rmb();
    return cd->ret_val;
}

u32 te_generic_smc(u32 arg0, u32 arg1, u32 arg2, u32 arg3)
{
    int cpu_id;
    int retval;
    
    raw_spin_lock(&sunxi_smc_kernel_lock);
    cpu_id = smp_processor_id();
    if (cpu_id != 0) {
    	/* secondary cpu post command to primary cpu */
        mb();
        retval = tee_post_smc(cpu_id, arg0, arg1, arg2, arg3);
    } else {
    	/* primary cpu call smc directly */
        retval = te_smc_command_send(arg0, arg1, arg2, arg3);
    }
    raw_spin_unlock(&sunxi_smc_kernel_lock);
    return retval;
}
EXPORT_SYMBOL(te_generic_smc);


/*
 * Do an SMC call
 */
static void do_smc(struct te_request *request, struct te_device *dev)
{
	u32 smc_args;
	u32 smc_params = 0;
	
	if (irqs_disabled()) {
		pr_info("%s: tee call when irq disable\n", __func__);
		dump_stack();
	}
	
	smc_args = (u32)virt_to_phys(request);
	if (request->params){
		smc_params = (u32)virt_to_phys(request->params);
	}
	te_generic_smc(request->type, smc_args, smc_params, 0);
}

/*
 * Open session SMC (supporting client-based te_open_session() calls)
 */
void te_open_session(struct te_opensession *cmd,
		struct te_request *request,
		struct te_context *context)
{

	memcpy(&request->dest_uuid, &cmd->dest_uuid, sizeof(struct te_service_id));
	pr_debug("OPEN_CLIENT_SESSION: 0x%x 0x%x 0x%x 0x%x\n",
			request->dest_uuid[0],
			request->dest_uuid[1],
			request->dest_uuid[2],
			request->dest_uuid[3]);
	request->type = TEE_SMC_OPEN_SESSION;
	do_smc(request, context->dev);
	if(request->result){
		pr_debug("Error opening session: return value-%x\n", request->result);
	}
}
/*
 * Close session SMC (supporting client-based te_close_session() calls)
 */
void te_close_session(struct te_closesession *cmd,
		struct te_request *request,
		struct te_context *context)
{
	request->session_id = cmd->session_id;
	request->type = TEE_SMC_CLOSE_SESSION;
	memcpy(&request->dest_uuid, &cmd->service_id, sizeof(struct te_service_id));
	do_smc(request, context->dev);
	if (request->result){
		pr_debug("Error closing session: %08x\n", request->result);
	}
}

static int te_get_kernel_addr(struct te_oper_param *params, struct te_context *context)
{
	int shared_mem_found = 0;
	struct te_shmem_desc *temp_shared_mem;
	list_for_each_entry(temp_shared_mem, &context->shmem_alloc_list,list){
		if(temp_shared_mem->index == (void *)params->index){
			shared_mem_found = 1;
			break;
		}
	}
	if(!shared_mem_found){
		return TE_ERROR_BAD_PARAMETERS;
	}
	params->u.Mem.base = (void *)((u32)temp_shared_mem->buffer + 
			((u32)params->u.Mem.base-(u32)temp_shared_mem->index));
	params->u.Mem.phys = (void *)virt_to_phys(params->u.Mem.base);

	pr_debug("params kerneladdr:%p    physaddr:%p\n",params->u.Mem.base, params->u.Mem.phys);

	return TE_SUCCESS;
}

static int te_map_kernel_addr(struct te_request *request,
		struct te_context *context)
{
	u32 i;
	int ret = TE_SUCCESS;
	struct te_oper_param *params = request->params;
	for (i = 0; i < request->params_size; i++) {
		switch (params[i].type) {
			case TE_PARAM_TYPE_NONE:
			case TE_PARAM_TYPE_INT_RO:
			case TE_PARAM_TYPE_INT_RW:
				break;
			case TE_PARAM_TYPE_MEM_RO:
			case TE_PARAM_TYPE_MEM_RW:
				ret = te_get_kernel_addr(params+i, context);
				if (ret != 0) {
					pr_err("%s failed with err (%d)\n",__func__, ret);
					ret = TE_ERROR_BAD_PARAMETERS;
					break;
				}
				break;
			default:
				pr_err("%s: TE_ERROR_BAD_PARAMETERS\n", __func__);
				ret = TE_ERROR_BAD_PARAMETERS;
				break;
		}
	}
	return ret;
}
static int te_get_user_addr(struct te_oper_param *params, struct te_context *context)
{
	int shared_mem_found = 0;
	struct te_shmem_desc *temp_shared_mem;
	list_for_each_entry(temp_shared_mem, &context->shmem_alloc_list,list){
		if(temp_shared_mem->index == (void *)params->index){
			shared_mem_found = 1;
			break;
		}
	}
	if(!shared_mem_found){
		return TE_ERROR_BAD_PARAMETERS;
	}
	params->u.Mem.base = (void *)((u32)temp_shared_mem->u_addr+ 
			((u32)params->u.Mem.base-(u32)temp_shared_mem->buffer));
	return TE_SUCCESS;
}

static void te_map_user_addr(struct te_request *request,
	struct te_context *context)
{
	u32 i;
	int ret;
	struct te_oper_param *params = request->params;

	for (i = 0; i < request->params_size; i++) {
		switch (params[i].type) {
			case TE_PARAM_TYPE_NONE:
			case TE_PARAM_TYPE_INT_RO:
			case TE_PARAM_TYPE_INT_RW:
				break;
			case TE_PARAM_TYPE_MEM_RO:
			case TE_PARAM_TYPE_MEM_RW:
				ret = te_get_user_addr(params+i, context);
				if (ret != 0) {
					pr_err("%s failed with err (%d)\n",__func__, ret);
					ret = TE_ERROR_BAD_PARAMETERS;
					break;
				}
				break;
			default:
				pr_err("%s: TE_ERROR_BAD_PARAMETERS\n", __func__);
				break;
		}
	}
}

/*
 * Launch operation SMC (supporting client-based te_launch_operation() calls)
 */
void te_launch_operation(struct te_launchop *cmd,
		struct te_request *request,
		struct te_context *context)
{
	int ret;
	ret = te_map_kernel_addr(request, context);
	if(ret != TE_SUCCESS){
		pr_err("te_setup_temp_buffers failed err (0x%x)\n", ret);
		SET_RESULT(request, ret, TE_RESULT_ORIGIN_API);
		return;
	}
	memcpy(&request->dest_uuid,
			&cmd->service_id,
			sizeof(struct te_service_id));
	request->session_id = cmd->session_id;
	request->command_id = cmd->operation.command;
	request->type = TEE_SMC_INVOKE_COMMAND;
	do_smc(request, context->dev);
	te_map_user_addr(request, context);
}

int te_tz_init(void)
{
	/* initialize shmem */
	te_shmem_init();
	
	return 0;
}
