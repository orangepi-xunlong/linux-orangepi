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

void *sst_memcpy(void *dest, const void *src, size_t count) 
{
	return memcpy(dest,src,count);
}

void *sst_memset(void *dest,int val,  size_t count) 
{
	return memset(dest,val,count);
}

int __data_dump( char * name ,void *data, unsigned int len)
{
	if( sst_debug_mask !=0 ){
		print_hex_dump_bytes( name, DUMP_PREFIX_OFFSET,
				data, len);
	}
	return 0 ;
}
int __desc_dump(store_desc_t  *desc)
{
	if(sst_debug_mask !=0){

		fprintk("	->store_desc:\n ");	
		fprintk("		: type	= 0x%x\n", desc->type);
		fprintk("		: ID	= 0x%x\n", desc->id);
		fprintk("		: vaild	= 0x%x\n", desc->vaild);
		fprintk("		: flag	= 0x%x\n", desc->flag);
	}
	return 0 ;
}

int __object_dump(store_object_t	*object)
{	
	if(sst_debug_mask != 0){

		fprintk("	->store_object:\n ");	
		fprintk("		: magic = 0x%x\n", object->magic);
		fprintk("		: ID	= 0x%x\n", object->id );
		fprintk("		: re_encrypt= 0x%x\n", object->re_encrypt );
		fprintk("		: version	= 0x%x\n", object->version );
		fprintk("		: reserved  = 0x%x\n", object->reserved[0] );
		fprintk("		: reserved  = 0x%x\n", object->reserved[1] );
		fprintk("		: reserved  = 0x%x\n", object->reserved[2] );
		fprintk("		: reserved  = 0x%x\n", object->reserved[3] );
		fprintk("		: act_len	= 0x%x\n", object->actual_len );
		fprintk("		: crc		= 0x%x\n", object->crc );
		fprintk("		: data 		= 0x%x\n", (u32)object->data );
		fprintk("		: name 		= %s\n", (u32)object->name );

	}
	return (0);
}

int __oem_map_dump(struct secure_storage_t *sst)
{
	
	if(sst_debug_mask !=0){
		store_map_t		*map ;
		dprintk("[Dump sst oem map data]:\n ");	
		list_for_each_entry(map, &sst->oem_list, list){
			dprintk("Dump descript:\n");
			__desc_dump( map->desc );
			dprintk("Dump object:\n");
			__object_dump( map->object );
		}
	}
	return (0);
}


int __sst_map_dump(struct secure_storage_t  *sst)
{
	return 0 ;
	if(sst_debug_mask !=0){

		store_map_t		*map ;

		dprintk("[Dump sst oem map data]:\n ");	
		list_for_each_entry(map, &sst->oem_list, list){
			__desc_dump( map->desc );
			__object_dump( map->object );
		}

		dprintk("[Dump sst user map data]:\n ");	
		list_for_each_entry(map, &sst->user_list, list){
			__desc_dump( map->desc );
			__object_dump( map->object );
		}
	}
	return (0);
}


int __cmd_work_dump( cmd_work_t *cmd )
{

	if(sst_debug_mask !=0){

		dprintk("Dump cmd_work:\n");
		printk("   ->resp: 0x%x\n",cmd->resp );
		printk("   ->type: 0x%x\n",cmd->type );
		printk("   ->id: 0x%x\n",cmd->id );
		printk("   ->buf: 0x%x\n",(u32)cmd->buf );
		printk("   ->buf_len: 0x%x\n",cmd->len );
		printk("   ->ret: 0x%x\n",cmd->ret );

	}
	return (0);
}




int __command_dump(cmd_t *cmd)
{
	if(sst_debug_mask != 0){

		fprintk("	->command\n ");	

		fprintk("		: cmd_tyep		= " );
		
		switch(cmd->cmd_type){
			case SST_CMD_POLLING:
				fprintk(" SST_CMD_POLLING\n");
				break;
			case SST_CMD_BIND_REQ:
				fprintk(" SST_CMD_BIND_REQ\n");
				break;
			case SST_CMD_WORK_DONE:
				fprintk(" SST_CMD_WORK_DONE\n");
				break;
			case SST_CMD_UPDATE_REQ:
				fprintk(" SST_CMD_UPDATE_REQ\n");
				break;
			case SST_CMD_INITIALIZED:
				fprintk(" SST_CMD_INITIALIZED\n");
				break;
			default:
				break;
		}

		fprintk("		: cmd_parma[0]	= 0x%x\n", cmd->cmd_param[0] );
		fprintk("		: cmd_parma[1]	= 0x%x\n", cmd->cmd_param[1] );
		fprintk("		: cmd_parma[2]	= 0x%x\n", cmd->cmd_param[2] );
		fprintk("		: cmd_parma[3]	= 0x%x\n", cmd->cmd_param[3] );
		fprintk("		: cmd_buf		= 0x%x\n", (unsigned int)(cmd->cmd_buf) );
		fprintk("		: cmd_buf_len	= 0x%x\n", cmd->cmd_buf_len );
		fprintk("		: resp_type		= "  );

		switch(cmd->resp_type){
			case SST_RESP_DUMMY:
				fprintk(" SST_RESP_DUMMY\n");
				break;
			case SST_RESP_READ:
				fprintk(" SST_RESP_READ\n");
				break;
			case SST_RESP_READ_DONE:
				fprintk(" SST_RESP_READ_DONE\n");
				break;
			case SST_RESP_WRITE:
				fprintk(" SST_RESP_WRITE\n");
				break;
			case SST_RESP_WRITE_DONE:
				fprintk(" SST_RESP_WRITE_DONE\n");
				break;		
			case SST_RESP_DELETE:
				fprintk(" SST_RESP_DELETE\n");
				break;
			case SST_RESP_BIND_DONE:
				fprintk(" SST_RESP_BIND_DONE\n");
				break;
			case SST_RESP_UPDATE_DONE:
				fprintk(" SST_RESP_UPDATE_DONE\n");
				break;
			case SST_RESP_INITED_DONE:
				fprintk(" SST_RESP_INITED_DONE\n");
				break;
			default:
				fprintk("\n");
				break;
		}

		fprintk("		: resp_ret[0]	= 0x%x\n", cmd->resp_ret[0] );
		fprintk("		: resp_ret[1]	= 0x%x\n", cmd->resp_ret[1] );
		fprintk("		: resp_ret[2]	= 0x%x\n", cmd->resp_ret[2] );
		fprintk("		: resp_ret[3]	= 0x%x\n", cmd->resp_ret[3] );
		fprintk("		: resp_buf		= 0x%x\n", (unsigned int)(cmd->resp_buf) );
		fprintk("		: resp_buf_len	= 0x%x\n", cmd->resp_buf_len );

	}
	return (0);
}

void __param_dump(struct te_oper_param *param)
{
	fprintk("Param dump :0x%x\n", (unsigned int)param );	
	fprintk("	param->index = %d\n", param->index);
	fprintk("	param->type = 0x%x\n", param->type);
	fprintk("	param->Mem.base = 0x%x\n", (unsigned int)param->u.Mem.base);
	fprintk("	param->Mem.phys= 0x%x\n", (unsigned int)param->u.Mem.phys);
	fprintk("	param->Mem.len= 0x%x\n", param->u.Mem.len);
	return ;
}

void __request_dump(struct te_request *r )
{
	fprintk("Request dump :0x%x\n", (unsigned int)r);
	fprintk("	request->type = 0x%x\n", r->type);
	fprintk("	request->session_id = 0x%x\n", r->session_id);
	fprintk("	request->command_id = 0x%x\n", r->command_id);
	fprintk("	request->params = 0x%x\n", (unsigned int)(r->params));
	fprintk("	request->params_size = 0x%x\n", r->params_size);
}

#ifdef SST_FUNCTION_TEST_DAEMON  

static char *tagt_type[]={
	"permanet" , "temporary" ,
};

int sst_test_daemon_kthread(void *data)
{
	int ret ;
	unsigned int i,id ;

	msleep(5000);
	for(i =0 ; i<3; i++){
		/*Init USER array*/
		ret = sst_cmd_func_test("wrap", "temporary", 0 , &id);
			if(ret != 0 ){
			derr("-[USER wrap] function fail\n");
			goto fail ;
			}
	
		msleep(1000);
		ret = sst_cmd_func_test("wrap", "permanet",0,&id);
			if(ret != 0 ){
			derr("-[USER wrap] function fail\n");
			goto fail ;
			}
	}

	while(1){
		msleep(2000);
		/*OEM type operation*/
		ret = sst_cmd_func_test("oem", "Widevine", 0 , NULL);
		if(ret != 0 ){
			derr("-[OEM unwrap] function fail\n");
			goto fail ;
		}
		dprintk("oem Widevine test pass");

		/*USER type operation*/
		for( i =0 ;i<2 ;i++){
			/* Wrap */
			ret = sst_cmd_func_test("wrap", tagt_type[i],0,&id);
			if(ret != 0 ){
				derr("-[USER wrap] function fail at %s\n", 
						 tagt_type[i]);
				goto fail ;
			}
			dprintk("wrap %s at id[0x%x] pass\n",tagt_type[i], id) ;

			/* unwrap */
			ret = sst_cmd_func_test("unwrap", tagt_type[i],id, NULL);
			if(ret != 0 ){
				derr("-[USER unwrap] function fail at %s\n", 
						 tagt_type[i]);
				goto fail ;
			}
			dprintk("unwrap %s at id[0x%x] pass\n",tagt_type[i], id) ;

			/* update */
			ret = sst_cmd_func_test("update", tagt_type[i],id, NULL);
			if(ret != 0 ){
				derr("-[USER update] function fail at %s\n", 
						 tagt_type[i]);
				goto fail ;
			}
			dprintk("update %s at id[0x%x] pass\n",tagt_type[i], id) ;

			/* delete */
			ret = sst_cmd_func_test("delete", tagt_type[i],id, NULL);
			if(ret != 0 ){
				derr("-[USER delete] function fail at %s\n", 
						 tagt_type[i]);
				goto fail ;
			}
			dprintk("delete %s at id[0x%x] pass\n",tagt_type[i], id) ;
		}

	}

fail:
	derr("SST test daemon fail\n");
	return -1 ;
}

#endif

