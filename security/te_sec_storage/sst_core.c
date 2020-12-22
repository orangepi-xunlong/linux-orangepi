/* secure storage driver for sunxi platform 
 *
 * Copyright (C) 2014 Allwinner Ltd. 
 *
 * Author:
 *	Ryan Chen <ryanchen@allwinnertech.com>
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */
 
#include <linux/kernel.h>
#include <linux/printk.h>
#include <linux/module.h>
#include <linux/err.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/mutex.h>
#include <linux/kthread.h>
#include <linux/smp.h>
#include <linux/crc32.h>
#include <mach/hardware.h>
#include <mach/sys_config.h>
#include <linux/secure/te_rpc.h>
#include <linux/secure/te_sec_storage.h>

#include "sst.h"
#include "sst_debug.h"

int sst_debug_mask  = 0; /*Disable debug info as default 0*/

char *ext_class= NULL ;

extern int sst_rpc_register(void *handle);
extern int sst_rpc_unregister(void) ;

unsigned char * align_cmd_buf, *align_resp_buf ;

static struct secure_storage_t *aw_sst = NULL;  

struct secure_storage_t * sst_get_aw_storage(void )
{
	return aw_sst ;
}

int check_object_valid(store_object_t *object)
{
	unsigned int crc ;
	if( object->magic != STORE_OBJECT_MAGIC){
		derr("-object magic error\n");
		return -1 ;
	}
	crc = ~crc32_le(~0, 
			(const unsigned char *)object,
			sizeof(store_object_t)-4);
	if( object->crc !=crc )
	{
		print_hex_dump_bytes( "secure object:", DUMP_PREFIX_OFFSET,
				object, sizeof(store_object_t));	

		derr("-object crc error, read 0x%x, calc 0x%x\n",object->crc, crc);
		return -1 ;
	}
	
	return 0 ;
}

/* 
 * read store object data to cmd_work->buf 
 */
static int _read_work(cmd_work_t * cmd_work)
{

	struct secure_storage_t * sst = sst_get_aw_storage();
	store_map_t	*map; 
	struct list_head *head  ;

	if( cmd_work->type == USER_DATA ){
		head = &sst->user_list;
	}else if(cmd_work->type == OEM_DATA ){
		head = &sst->oem_list; 
	}else
		head = NULL ;

	list_for_each_entry(map, head, list ){
		if( (map->desc->id == cmd_work->id) &&
				(map->object->id == cmd_work->id) &&	
				(map->desc->flag == STORE_PROBE_DONE_MAGIC) &&
				check_object_valid(map->object))	

			sst_memcpy(cmd_work->buf,
					map->object->data, 
					map->object->actual_len);
			cmd_work->len = map->object->actual_len ;
			return (0);

	}
	derr("-can't find object/desc in map list id 0x%x ?|",cmd_work->id);
	return -1; 
}

/* 
 * write store object data 
 * from cmd_work->buf to nand 
 */
static int _write_work(cmd_work_t *cmd_work)
{
	struct secure_storage_t * sst = sst_get_aw_storage();
	store_map_t	*map; 
	struct list_head *head  ;
	int		ret = -1 ;
	
	dprintk("(%p)\n",_write_work );

	if( cmd_work->type == USER_DATA ){
		head = &sst->user_list;
	}else{
		derr("-cant write OEM data !!!\n");
		return -1 ;
	}

	/*
	 * Write to 
	 */ 
	list_for_each_entry(map, head, list ){
		if( (map->desc->id == cmd_work->id) &&
				(map->object->id == cmd_work->id) )	
		{
			sst_memcpy(map->object->data, cmd_work->buf, cmd_work->len);
			map->object->magic = STORE_OBJECT_MAGIC ;
			map->object->actual_len = cmd_work->len ;
			map->object->crc = 	~crc32_le(~0, 
					(const unsigned char *)map->object,
					sizeof(store_object_t)-4
					);

			ret = sst->st_operate->write(
					(st_path_t)map->desc->dir,
					(char*)map->object,
					sizeof(*(map->object)),
					USER_DATA
					);
			if( ret != sizeof( *(map->object)) ){
				derr("-write user data to fs fail\n");
				return -1;
			}

			map->desc->flag = STORE_PROBE_DONE_MAGIC;
			return 0 ;
		}
	}
	derr("-can't find object/desc in map list id 0x%x ?|",cmd_work->id);
	return -1 ;
}

static int _delete_work(cmd_work_t *cmd_work)
{

	struct secure_storage_t * sst = sst_get_aw_storage();
	store_map_t	*map; 
	struct list_head *head  ;
	int		ret = -1 ;

	dprintk("(%p)\n",_delete_work);

	if( cmd_work->type == USER_DATA ){
		head = &sst->user_list;
	}else{
		derr("-cant delete OEM data !!!\n");
		return -1 ;
	}

	/*
	 * Write to 
	 */ 
	list_for_each_entry(map, head, list ){
		if( (map->desc->id == cmd_work->id) &&
				(map->object->id == cmd_work->id) )	
		{
			map->object->magic = 0 ;
			map->object->actual_len = 0;
			map->object->crc = 0;	

			ret = sst->st_operate->delete(
					(st_path_t)map->desc->dir,
					USER_DATA
					);
			if( ret != 0){
				derr("-write user data to fs fail\n");
				return -1;
			}

			map->desc->flag = 0;
			return 0 ;
		}
	}
	derr("-can't find object/desc in map list id 0x%x ?|",cmd_work->id);
	return -1 ;
}
/*
 * deal with the command actual work
 */ 
DEFINE_MUTEX(work_mutex);
static void  cmd_worker(struct work_struct * my_work)
{
	cmd_work_t  *cmd_work =
		container_of( my_work, cmd_work_t , work );

	int ret = -1 ;
	mutex_lock(&work_mutex);

	switch(cmd_work->resp){
		case SST_RESP_READ:
			ret = _read_work(cmd_work);
			break;
		case SST_RESP_WRITE:
			ret = _write_work(cmd_work);
			break;
		case SST_RESP_DELETE:
			ret = _delete_work(cmd_work) ;
			break;
		default:
			derr("-wrong cmd_work \n");
			ret =  - EINVAL ;
	}

	cmd_work->ret = ret ;
	mutex_unlock(&work_mutex);
}

static unsigned int _sst_rpc_handle(
		unsigned int cmd, 
		unsigned int args, 
		unsigned int len)
{
	cmd_work_t *cmd_work = aw_sst->cmd_work ;
	int ret = -1;
	unsigned int nw_va;

	dprintk("(%p)\n", _sst_rpc_handle);
	nw_va = te_shmem_pa2va(args);
	if(nw_va == 0){
		derr("sst sw/nw share memroy error\n");	
		return -1 ;
	}

	memcpy(cmd_work, (void*)nw_va, len);
		
	__cmd_work_dump( cmd_work );
	
	switch(cmd_work->resp){
		case  SST_RESP_DUMMY :
			break;
		case SST_RESP_READ	:
		case SST_RESP_WRITE :
		case SST_RESP_DELETE :
			if(cmd_work->buf )	
				cmd_work->buf = (void *)te_shmem_pa2va(
							(unsigned int)cmd_work->buf);

			dprintk("SST rpc buf: cmd 0x%x, buf 0x%x",
					(unsigned int)args, (unsigned int)cmd_work->buf);

			schedule_work(&cmd_work->work);
			break;
		default:
			break ;
	}
	return ret  ;
}

extern int secure_store_in_fs_test(void);
static int sst_daemon_kthread(void *data)
{
	int	ret ;
	struct secure_storage_t *sst;
	sst = (struct secure_storage_t *)data ;

	dprintk("SST daemon work\n");
#ifdef OEM_STORE_IN_FS
	secure_store_in_fs_test();
#endif
	if( sst->st_operate->update( sst , OEM_DATA ) <0){
		derr(" - update oem data failed\n");
		ret = (- EINVAL);
		goto fail;
	}

	if( sst->st_operate->update( sst , USER_DATA ) <0 ){
		derr(" - update user data failed\n");
		ret = (- EINVAL);
		goto fail;
	}

	if( sst_cmd_init_done() <0){
		derr(" - send normal-world init done cmd failed\n");
		ret = (- EINVAL);
		goto fail;
	}

#ifdef SST_FUNCTION_TEST_DAEMON 
	sst->test_tsk= kthread_create(sst_test_daemon_kthread, sst, "sst_test_daemon"); 
	if(!IS_ERR(sst->test_tsk)){
		wake_up_process(sst->test_tsk);
	}else{
		derr("Test Tk request fail!\n");
		ret = (- EINVAL);
		goto fail;
	}

#endif	
	ret = 0 ;
fail:

	return ret ;
}

static int _init_sst_map(struct secure_storage_t *sst)
{

	u32 i =0 ;	
	store_desc_t	*desc ;
	store_object_t	*object ;
	store_map_t		* map;

	/* Init user map data struct*/
	dprintk("init user map\n" );
	INIT_LIST_HEAD(&sst->user_list);
	sst->user_size =0 ;
	for(i =0 ; i<MAX_USER_STORE_NUM; i++ ){
		
		desc = kzalloc(sizeof(*desc),GFP_KERNEL);
		object = kzalloc(sizeof(*object),GFP_KERNEL);
		map = kzalloc(sizeof(*map),GFP_KERNEL);
		if(!desc || !object || !map){
			derr(" - Malloc failed\n");
			goto fail ;
		}

		desc->type = USER_DATA;
		desc->vaild = false ;
		desc->id = i;
		sprintf(desc->dir, "%s/%08d.bin",
								user_path, i);
		fprintk("user file :%s\n",desc->dir);

		object->id =i ;
		mutex_init(&map->mutex);
		map->desc = desc;
		map->object=object; 

		list_add_tail(&map->list,&sst->user_list);

	}

	/* Init OEM map data struct*/
	dprintk("init OEM map\n" );

	sst->oem_size =0 ;
	INIT_LIST_HEAD(&sst->oem_list)	;
	for(i =0 ; i<MAX_OEM_STORE_NUM; i++ ){
		
		desc = kzalloc(sizeof(*desc),GFP_KERNEL);
		object = kzalloc(sizeof(*object),GFP_KERNEL);
		map = kzalloc(sizeof(*map),GFP_KERNEL);
		if(!desc || !object || !map){
			derr(" - Malloc failed\n");
			goto fail ;
		}

		desc->type = OEM_DATA;
		desc->id = 0;
		desc->vaild = false ;

		object->id = 0 ;

		mutex_init(&map->mutex);
		map->desc = desc;
		map->object=object; 

		list_add_tail(&map->list,&sst->oem_list);
	}
	return (0) ;
	
fail:
	if(desc){
		sst_memset(desc, 0, sizeof(*desc));
		kfree(desc);
	}
	if(object){
		sst_memset(object, 0, sizeof(*object));
		kfree(object);
	}
	if(map){
		sst_memset(map, 0, sizeof(*map));
		kfree(map);
	}
	return (- ENOMEM) ;	
}


static int _free_sst_map(struct secure_storage_t  *sst)
{
	store_desc_t	*desc ;
	store_object_t	*object ;
	store_map_t		* map; 
	
	struct list_head * head = &sst->oem_list ;
	list_for_each_entry(map, head, list ){
		desc = map->desc ;
		sst_memset(desc, 0, sizeof(*desc));
		kfree(desc);

		object = map->object;
		sst_memset(object , 0, sizeof(*object));
		kfree(object);

		mutex_destroy(&map->mutex);
	}
	
	head = &sst->user_list ;
	list_for_each_entry(map, head, list ){
		desc = map->desc ;
		sst_memset(desc, 0, sizeof(*desc));
		kfree(desc);

		object = map->object;
		sst_memset(object , 0, sizeof(*object));
		kfree(object);

		mutex_destroy(&map->mutex);
	}

	return (0);
}

static int __init aw_sst_init(void)
{
	int ret;
	struct secure_storage_t		*sst = NULL; 
	struct command				*cmd = NULL; 
	struct storage_operation	*st_op =  NULL;
	
	cmd_work_t	*cmd_work = NULL;

	struct te_request * request = NULL ;
	struct te_oper_param * param = NULL ;

	dprintk("(%p)\n", aw_sst_init);

	sst = kzalloc(sizeof(*sst), GFP_KERNEL);
	st_op = (st_op_t *)kzalloc(sizeof(*st_op), GFP_KERNEL );

	if (!sst || !st_op ) {
		derr(" - Malloc failed\n");
		ret = - ENOMEM;
		goto fail ;
	}

	cmd = (cmd_t *)__get_free_pages(GFP_KERNEL, 
				get_order(ROUND_UP( sizeof(*cmd), SZ_4K)));
	if(!cmd ){
		derr(" cmd buffer get fail\n");
		ret = - ENOMEM ;
		goto fail ;
	}
	/* Parepare the request to Swd */
	request = (struct te_request *)__get_free_pages(GFP_KERNEL,
			get_order(ROUND_UP( sizeof(*request), SZ_4K)));
	if(!request ){
		derr(" request buffer get fail\n");
		ret = - ENOMEM ;
		goto fail ;
	}
	param = (struct te_oper_param *)__get_free_pages(GFP_KERNEL,
			get_order(ROUND_UP( sizeof(*param)*3, SZ_4K)));

	if(!param ){
		derr(" param buffer get fail\n");
		ret = - ENOMEM ;
		goto fail ;
	}
	
	align_cmd_buf = (unsigned char *)__get_free_pages(GFP_KERNEL, 
												get_order(SZ_4K) );
	if(!align_cmd_buf ){
		derr(" align_cmd buffer get fail\n");
		ret = - ENOMEM ;
		goto fail ;
	}

	align_resp_buf = (unsigned char *)__get_free_pages(GFP_KERNEL, 
											get_order(SZ_4K) );
	if(!align_resp_buf ){
		derr(" align_resp buffer get fail\n");
		ret = - ENOMEM ;
		goto fail ;
	}

	dprintk("param:0x%x, request:0x%x, align_cmd_buf:%x,align_resp_buf:0x%x\n",
			(unsigned int)param, (unsigned int)request,
			(unsigned int)align_cmd_buf, (unsigned int)align_resp_buf);

	aw_sst = sst ;

	cmd_work = (cmd_work_t *)kzalloc( sizeof(cmd_work_t),GFP_KERNEL);
	if(!cmd_work){
		derr("out of memory\n");
		ret = - ENOMEM;
		goto fail ;
	}
	INIT_WORK(&cmd_work->work, cmd_worker);

	sst->cmd_work = cmd_work ;
	sst->request = request ;
	sst->param = param ;

	sst->cmd = cmd ;
	spin_lock_init(&cmd->lock);
	cmd->status = SST_STAS_CMD_INIT ;

	sst->st_operate = st_op;
	sst_operation_init( st_op);

	if ( _init_sst_map(sst) ){
		derr(" - oem_map failed\n");
		ret = (- EINVAL);
		goto fail;
	}

	__sst_map_dump(sst);	

	sst_rpc_register(_sst_rpc_handle);

	sst->tsk= kthread_create(sst_daemon_kthread, sst, "sst_daemon"); 
	if(!IS_ERR(sst->tsk)){
		wake_up_process(sst->tsk);
	}else{
		derr("Tsk request fail!\n");
		ret = (- EINVAL);
		goto fail;
	}
	return(0);

fail:
	if(sst){
		sst_memset(sst,0, sizeof(*sst));
		kfree(sst);
	}
	
	if(cmd_work)
		kfree(cmd_work);

	if(st_op){
		sst_memset(st_op,0,sizeof(*st_op));
		kfree(st_op);
	}

	if(cmd)
		free_pages((unsigned long)cmd, 
				get_order(ROUND_UP(sizeof(*cmd), SZ_4K)));
	if(param )
		free_pages((unsigned long)param, 
				get_order(ROUND_UP( sizeof(*param)*3, SZ_4K)) );
	if(request )
		free_pages((unsigned long)request, 
				get_order(ROUND_UP( sizeof(*request), SZ_4K)) );

	if(align_cmd_buf )
		free_pages((unsigned long)align_cmd_buf, 
						get_order( SZ_4K) );
	if(align_resp_buf )
		free_pages((unsigned long)align_resp_buf, 
				get_order( SZ_4K) );
	aw_sst = NULL ;
	return ret ;
}

static void __exit aw_sst_exit(void)
{
	struct secure_storage_t *sst =aw_sst;

	sst_rpc_unregister();

	kthread_stop(sst->tsk);
#ifdef SST_FUNCTION_TEST_DAEMON
	kthread_stop(sst->test_tsk);
#endif
	
	/*clean up  memroy*/
	_free_sst_map(sst);

	if(sst->cmd)
		free_pages((unsigned long)sst->cmd, 
				get_order(ROUND_UP(sizeof(*sst->cmd), SZ_4K)));
	if(sst->param )
		free_pages((unsigned long)sst->param, 
				get_order(ROUND_UP( sizeof(*sst->param)*3, SZ_4K)) );
	if(sst->request )
		free_pages((unsigned long)sst->request, 
				get_order(ROUND_UP( sizeof(*sst->request), SZ_4K)) );

	if(align_cmd_buf )
		free_pages((unsigned long)align_cmd_buf, 
						get_order( SZ_4K) );
	if(align_resp_buf )
		free_pages((unsigned long)align_resp_buf, 
				get_order( SZ_4K) );

	if( sst->cmd_work ){
		sst_memset(sst->cmd_work, 0,sizeof(*sst->cmd_work));
		kfree(sst->cmd_work);
	}

	if(sst->st_operate){
		sst_memset(sst->st_operate, 0,sizeof(*sst->st_operate));
		kfree(sst->st_operate);
	}

	if( sst ){
		sst_memset(sst,0,sizeof(*sst));
		kfree(sst);	
	}

	aw_sst =NULL;
}

module_init(aw_sst_init);
module_exit(aw_sst_exit);

module_param(sst_debug_mask, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(sst_debug_mask, "Secure storage Enable debug");

module_param(ext_class, charp, S_IRUGO | S_IWUSR );
MODULE_PARM_DESC(ext_class, "Secure storage extension oem class");

MODULE_LICENSE("GPL");
MODULE_AUTHOR("ryan chen<ryanchen@allsoftwinnertech.com>");
MODULE_DESCRIPTION("Sunxi secure storage daemon for normal world operation");

