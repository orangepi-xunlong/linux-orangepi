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
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/printk.h>
#include <linux/ioctl.h>
#include <linux/miscdevice.h>
#include <linux/mm.h>
#include <linux/fs.h>
#include <asm/cacheflush.h>
#include <asm/outercache.h>
#include <linux/list.h>
#include <linux/dma-mapping.h>

#include <linux/secure/te_protocol.h>

#define SET_ANSWER(a, r, ro)	{ a.result = r; a.result_origin = ro; }
#define ROUND_UP(N, S) ((((N) + (S) - 1) / (S)) * (S))


struct te_device te_dev;
static DEFINE_MUTEX(smc_lock);
static DEFINE_MUTEX(mem_free_lock);

static int te_create_free_cmd_list(struct te_device *dev)
{
	int cmd_desc_count, ret = 0;
	struct te_cmd_req_desc *req_desc;
	int bitmap_size;

	dev->req_param_buf = NULL;
	dev->req_addr = kzalloc(PAGE_SIZE, GFP_KERNEL);
	dev->param_addr = kzalloc(PAGE_SIZE, GFP_KERNEL);
	dev->req_addr_phys = virt_to_phys(dev->req_addr);
	dev->param_addr_phys = virt_to_phys(dev->req_addr);

	if ((dev->req_addr == NULL) || (dev->param_addr == NULL)) {
		ret = -ENOMEM;
		goto error;
	}
	/* alloc param bitmap allocator */
	bitmap_size = BITS_TO_LONGS(TE_PARAM_MAX) * sizeof(long);
	dev->param_bitmap = kzalloc(bitmap_size, GFP_KERNEL);
	
	for (cmd_desc_count = 0; cmd_desc_count < TE_CMD_DESC_MAX; cmd_desc_count++) {
		req_desc = kzalloc(sizeof(struct te_cmd_req_desc), GFP_KERNEL);
		if (req_desc == NULL) {
			pr_err("Failed to allocate cmd req descriptor\n");
			ret = -ENOMEM;
			goto error;
		}
		req_desc->req_addr = dev->req_addr + cmd_desc_count;
		INIT_LIST_HEAD(&(req_desc->list));
		/* Add the cmd param descriptor to free list */
		list_add_tail(&req_desc->list, &(dev->free_cmd_list));
	}
error:
	return ret;
}

static struct te_oper_param *te_get_free_params(struct te_device *dev,
		unsigned int nparams)
{
	struct te_oper_param *params = NULL;
	int idx, nbits;

	if (nparams) {
		nbits = get_count_order(nparams);
		idx = bitmap_find_free_region(dev->param_bitmap, TE_PARAM_MAX, nbits);
		if (idx >= 0){
			params = dev->param_addr + idx;
		}
	}
	return params;
}

static void te_put_free_params(struct te_device *dev,
		struct te_oper_param *params, uint32_t nparams)
{
	int idx, nbits;
	idx = (params - dev->param_addr);
	nbits = get_count_order(nparams);
	bitmap_release_region(dev->param_bitmap, idx, nbits);
}

static struct te_cmd_req_desc *te_get_free_cmd_desc(struct te_device *dev)
{
	struct te_cmd_req_desc *cmd_desc = NULL;
	if (!(list_empty(&(dev->free_cmd_list)))) {
		cmd_desc = list_first_entry(&(dev->free_cmd_list), struct te_cmd_req_desc, list);
		list_del(&(cmd_desc->list));
		list_add_tail(&cmd_desc->list, &(dev->used_cmd_list));
	}
	return cmd_desc;
}

static void te_put_used_cmd_desc(struct te_device *dev,
		struct te_cmd_req_desc *cmd_desc)
{
	struct te_cmd_req_desc *param_desc, *tmp_param_desc;
	if (cmd_desc) {
		list_for_each_entry_safe(param_desc, tmp_param_desc, &(dev->used_cmd_list), list) {
			if (cmd_desc->req_addr == param_desc->req_addr) {
				list_del(&param_desc->list);
				list_add_tail(&param_desc->list, &(dev->free_cmd_list));
			}
		}
	}
}

static void __attribute__((unused)) te_print_cmd_list(
		struct te_device *dev, int used_list)
{
	struct te_cmd_req_desc *param_desc;
	if (!used_list) {
		pr_debug("Printing free cmd list\n");
		if (!(list_empty(&(dev->free_cmd_list)))) {
			list_for_each_entry(param_desc, &(dev->free_cmd_list), list){
				pr_debug("Phys addr for cmd req desc (%p)\n",param_desc->req_addr);
			}
		}
	} else {
		pr_debug("Printing used cmd list\n");
		if (!(list_empty(&(dev->used_cmd_list)))) {
			list_for_each_entry(param_desc, &(dev->used_cmd_list), list){
				pr_debug("Phys addr for cmd req desc (%p)\n", param_desc->req_addr);
			}
		}
	}
}
static int te_device_open(struct inode *inode, struct file *file)
{
	struct te_context *context;
	int ret = 0;

	context = kzalloc(sizeof(struct te_context), GFP_KERNEL);
	if (!context) {
		ret = -ENOMEM;
		goto error;
	}
	context->dev = &te_dev;
	INIT_LIST_HEAD(&(context->shmem_alloc_list));

	file->private_data = context;
	return 0;
error:
	return ret;
}

static int te_device_release(struct inode *inode, struct file *file)
{
	kfree(file->private_data);
	file->private_data = NULL;
	return 0;
}

static int copy_params_from_user(struct te_request *req,
		struct te_operation *operation)
{
	struct te_oper_param *param_array;
	struct te_oper_param *user_param;
	uint32_t i;

	if (operation->list_count == 0){
		return 0;
	}
	param_array = req->params;
	if (param_array == NULL) {
		pr_err("param_array empty\n");
		return 1;
	}
	user_param = operation->list_head;
	pr_debug("[%s]first userspace param addr: %p\n", __func__, user_param);
	for (i = 0; i < operation->list_count && user_param != NULL; i++) {
		if (copy_from_user(param_array + i, user_param,sizeof(struct te_oper_param))) {
			pr_err("Failed to copy operation parameter:%d, %p, list_count: %d\n" ,i, user_param, operation->list_count);
			return 1;
		}
		user_param = param_array[i].next_ptr_user;
	}
	return 0;
}

static int copy_params_to_user(struct te_request *req,
		struct te_operation *operation)
{

	struct te_oper_param *param_array;
	struct te_oper_param *user_param;
	uint32_t i;

	if (operation->list_count == 0){
		return 0;
	}
	param_array = req->params;
	if (param_array == NULL) {
		pr_err("param_array empty\n");
		return 1;
	}
	user_param = operation->list_head;
	for (i = 0; i < req->params_size; i++) {
		if (copy_to_user(user_param, param_array + i, sizeof(struct te_oper_param))) {
			pr_err("Failed to copy back parameter:%d %p\n", i, user_param);
			return 1;
		}
		user_param = param_array[i].next_ptr_user;
	}
	return 0;
}

static long te_handle_trustedapp_ioctl(struct file *file,
		unsigned int ioctl_num, unsigned long ioctl_param)
{
	long err = 0;
	union te_cmd cmd;
	struct te_answer *ptr_user_answer = NULL;
	struct te_operation *operation = NULL;
	struct te_oper_param *params = NULL;
	struct te_answer answer;
	struct te_request *request;
	struct te_cmd_req_desc *cmd_desc = NULL;
	struct te_context *context = file->private_data;
	struct te_device *dev = context->dev;
	
	if (copy_from_user(&cmd, (void __user *)ioctl_param, sizeof(union te_cmd))) {
		pr_err("Failed to copy command request\n");
		err = -EFAULT;
		goto error;
	}
	memset(&answer, 0, sizeof(struct te_answer));

	switch (ioctl_num) {
		case TE_IOCTL_OPEN_CLIENT_SESSION:
			operation = &cmd.opensession.operation;
			ptr_user_answer = &cmd.opensession.answer;
			cmd_desc = te_get_free_cmd_desc(dev);
			params = te_get_free_params(dev, operation->list_count);

			if (!cmd_desc || (operation->list_count && !params)) {
				SET_ANSWER(answer, TE_ERROR_OUT_OF_MEMORY, TE_RESULT_ORIGIN_COMMS);
				pr_err("failed to get cmd_desc/params\n");
				goto error;
			}

			request = cmd_desc->req_addr;
			memset(request, 0, sizeof(struct te_request));
			request->params = params;
			request->params_size = operation->list_count;
			if (copy_params_from_user(request, operation)) {
				err = -EFAULT;
				pr_debug("failed to copy params from user\n");
				goto error;
			}
			te_open_session(&cmd.opensession, request, context);
			SET_ANSWER(answer, request->result, request->result_origin);
			answer.session_id = request->session_id;
			err = request->result;
			break;

		case TE_IOCTL_CLOSE_CLIENT_SESSION:
			ptr_user_answer = &cmd.closesession.answer;
			cmd_desc = te_get_free_cmd_desc(dev);

			if (!cmd_desc) {
				SET_ANSWER(answer, TE_ERROR_OUT_OF_MEMORY, TE_RESULT_ORIGIN_COMMS);
				pr_err("failed to get cmd_desc\n");
				goto error;
			}

			request = cmd_desc->req_addr;
			memset(request, 0, sizeof(struct te_request));
			/* close session cannot fail */
			te_close_session(&cmd.closesession, request, context);
			SET_ANSWER(answer, request->result, request->result_origin);
			err = request->result;
			break;

		case TE_IOCTL_LAUNCH_OPERATION:
			operation = &cmd.launchop.operation;
			ptr_user_answer = &cmd.launchop.answer;
			cmd_desc = te_get_free_cmd_desc(dev);
			params = te_get_free_params(dev, operation->list_count);

			if (!cmd_desc || (operation->list_count && !params)) {
				SET_ANSWER(answer, TE_ERROR_OUT_OF_MEMORY, TE_RESULT_ORIGIN_COMMS);
				pr_err("failed to get cmd_desc/params\n");
				goto error;
			}
			request = cmd_desc->req_addr;
			memset(request, 0, sizeof(struct te_request));
			request->params = params;
			request->params_size = operation->list_count;

			if (copy_params_from_user(request, operation)) {
				err = -EFAULT;
				pr_debug("failed to copy params from user\n");
				goto error;
			}
			te_launch_operation(&cmd.launchop, request, context);
			SET_ANSWER(answer, request->result, request->result_origin);
			err = request->result;
			break;

		default:
			pr_err("Invalid IOCTL Cmd\n");
			err = -EINVAL;
			goto error;
	}
	if (ptr_user_answer && !err) {
		ptr_user_answer->result = answer.result;
		ptr_user_answer->result_origin = answer.result_origin;
		ptr_user_answer->session_id = answer.session_id;
	}
	if (request->params) {
		if (copy_params_to_user(request, operation)) {
			pr_err("Failed to copy return params\n");
			err = -EFAULT;
		}
	}
	if(copy_to_user((void __user *)ioctl_param, &cmd, sizeof(union te_cmd))) {
		pr_err("copy to user failed \n");
		err = -EFAULT;
	}

error:
	if (cmd_desc){
		te_put_used_cmd_desc(dev, cmd_desc);
	}
	if (params){
		te_put_free_params(dev, params, operation->list_count);
	}
	return err;
}
static long te_client_shared_mem_free(struct file *file,
		unsigned int ioctl_num, unsigned long ioctl_param)
{
	struct te_context *context = file->private_data;
	void * shard_mem_user_addr;
	struct te_shmem_desc *temp_shmem_desc = NULL;
	struct list_head *l;
	int found;

	shard_mem_user_addr = (void *)ioctl_param;
	list_for_each(l, &context->shmem_alloc_list) {
		temp_shmem_desc = list_entry(l, struct te_shmem_desc, list);
		if (temp_shmem_desc->u_addr == shard_mem_user_addr) {
			found = 1;
			break;
		}
	}
	if(found) {
		free_pages((unsigned long)temp_shmem_desc->buffer, get_order(ROUND_UP(temp_shmem_desc->size, SZ_4K)));
		list_del(&temp_shmem_desc->list);
		if(temp_shmem_desc){
			pr_debug("free temp sharemem desc %p\n", temp_shmem_desc);
			kfree(temp_shmem_desc);
		}
		return TE_SUCCESS;
	}else{
		return TE_ERROR_BAD_PARAMETERS;
	}
}

static long te_device_ioctl(struct file *file, unsigned int ioctl_num,
		unsigned long ioctl_param)
{
	int err;
	switch (ioctl_num) {
		case TE_IOCTL_OPEN_CLIENT_SESSION:
		case TE_IOCTL_CLOSE_CLIENT_SESSION:
		case TE_IOCTL_LAUNCH_OPERATION:
			mutex_lock(&smc_lock);
			err = te_handle_trustedapp_ioctl(file, ioctl_num, ioctl_param);
			mutex_unlock(&smc_lock);
			break;
		case TE_IOCTL_SHARED_MEM_FREE_REQUEST:
			mutex_lock(&smc_lock);
			err = te_client_shared_mem_free(file, ioctl_num, ioctl_param);
			mutex_unlock(&smc_lock);
			if (err)
				pr_debug("failed shared memory release: %d", err);
			break;
		default:
			pr_err("%s: Invalid IOCTL (0x%x)\n", __func__,ioctl_num);
			err = -EINVAL;
	}
	return err;
}
static int te_device_mmap(struct file *file, struct vm_area_struct *vma)
{
	int ret = 0;
	struct te_shmem_desc *shmem_desc = NULL;
	u32 *alloc_addr;
	long length = vma->vm_end -vma->vm_start;
	struct te_context *context = file->private_data;

	mutex_lock(&smc_lock);
	alloc_addr =  (void*) __get_free_pages(GFP_KERNEL, get_order(ROUND_UP(length, SZ_4K)));
	if(!alloc_addr) {
		pr_err("%s: get free pages failed \n",__func__);
		ret = -ENOMEM;
		goto return_func;
	}
	if (remap_pfn_range(vma,
				vma->vm_start,
				((virt_to_phys(alloc_addr)) >> PAGE_SHIFT),
				length,
				vma->vm_page_prot)) {
		ret = -EAGAIN;
		goto return_func;
	}
	shmem_desc = kzalloc(sizeof(struct te_shmem_desc), GFP_KERNEL);
	if (shmem_desc) {
		INIT_LIST_HEAD(&(shmem_desc->list));
		shmem_desc->buffer = alloc_addr;
		shmem_desc->size = length;
		shmem_desc->u_addr = (void*)vma->vm_start;
		shmem_desc->index = shmem_desc->u_addr;
		list_add_tail(&shmem_desc->list, &(context->shmem_alloc_list));
	}
	pr_debug("mmap kernel addr:%p user space addr %p\n", shmem_desc->buffer, shmem_desc->u_addr);
	mutex_unlock(&smc_lock);
return_func:
	return ret;	
}
/*
 * te_driver function definitions.
 */
static const struct file_operations te_device_fops = {
	.owner 		= THIS_MODULE,
	.open 		= te_device_open,
	.release 	= te_device_release,
	.mmap 		= te_device_mmap,
	.unlocked_ioctl = te_device_ioctl,
};

struct miscdevice te_misc_device = {
	.minor 	= MISC_DYNAMIC_MINOR,
	.name 	= "te_device",
	.fops 	= &te_device_fops,
};

int te_tz_init(void);

static int __init te_init(void)
{
	int ret;
	INIT_LIST_HEAD(&(te_dev.used_cmd_list));
	INIT_LIST_HEAD(&(te_dev.free_cmd_list));
	ret = te_create_free_cmd_list(&te_dev);
	if (ret != 0){
		return ret;
	}
	
	te_tz_init();
	
	return misc_register(&te_misc_device);
}
module_init(te_init);
