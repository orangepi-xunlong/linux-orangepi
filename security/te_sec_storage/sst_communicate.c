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
#include <mach/hardware.h>
#include <mach/sys_config.h>
#include <linux/spinlock.h>
#include <asm/firmware.h>
#include <linux/secure/te_protocol.h>
#include <asm/cacheflush.h>

#include "sst.h"
#include "sst_debug.h"

/* Commucation pipe lock*/
DEFINE_MUTEX(com_mutex);

extern unsigned char * align_cmd_buf ;
extern unsigned char *align_resp_buf ;
/*
 * Transmit the NWd<->Swd message to sst command
 */
static int parse_resp(
		struct te_request * request, 
		struct te_oper_param  *param, 
		cmd_t *cmd
		)
{
	__command_dump(cmd);
	fprintk("align_resp_buf 0x%x, cmd->resp_buf 0x%x\n",
			(unsigned int)align_resp_buf ,
			(unsigned int)cmd->resp_buf);
	if(cmd->resp_buf != NULL ){
		fprintk("Resp buferr ...");
		sst_memcpy(cmd->resp_buf, align_resp_buf, cmd->resp_buf_len);
		fprintk("copy done\n");
	}
	fprintk("Parse response done\n");
	return 0 ;
}

static int package_cmd(
		struct te_request * request, 
		struct te_oper_param  *param, 
		cmd_t *cmd
		)
{
	unsigned int i =0 ;
	__command_dump(cmd);

	sst_memset(request,0, sizeof(*request))	;
	sst_memset(param,0, sizeof(*param))	;

	request->type = TEE_SMC_SST_COMMAND ; 
	request->params = param ;

	request->params_size = 3 ; 

	request->params->index = i =0 ;
	param[0].type =  (uint32_t)TE_PARAM_TYPE_MEM_RW;
	param[0].u.Mem.base = cmd;
	param[0].u.Mem.phys =(void *)virt_to_phys(cmd);
	param[0].u.Mem.len= sizeof(*cmd);

	i ++;
	sst_memset(&param[i],0, sizeof(*param));
	param[i].index = i ;
	if( cmd->cmd_buf != NULL ){
		memset(align_cmd_buf,0,SZ_4K);
		sst_memcpy(align_cmd_buf, cmd->cmd_buf,cmd->cmd_buf_len );
		param[i].type = TE_PARAM_TYPE_MEM_RO;
		param[i].u.Mem.base = align_cmd_buf;
		param[i].u.Mem.phys = (void *)virt_to_phys(align_cmd_buf);
		param[i].u.Mem.len= cmd->cmd_buf_len;

	}

	i ++;
	sst_memset(&param[i],0, sizeof(*param));
	param[i].index = i ;
	if( cmd->resp_buf != NULL ){
		memset(align_resp_buf,0,SZ_4K);
		sst_memcpy(align_resp_buf,cmd->resp_buf,cmd->resp_buf_len);
		param[i].type = (uint32_t)TE_PARAM_TYPE_MEM_RW;
		param[i].u.Mem.base = align_resp_buf;
		param[i].u.Mem.phys =(void *)virt_to_phys(align_resp_buf);
		param[i].u.Mem.len= cmd->resp_buf_len;
	}

	__request_dump(request);
	for(i =0 ;i< 3; i++)
		__param_dump(&param[i]);

	return 0;
}


/*
 * Post command to Swd and wait the resp 
 */
static int post_cmd_wait_resp( cmd_t *cmd )
{

	int ret = -1 ;
	struct te_request *request;
	struct te_oper_param *param;
	struct secure_storage_t *sst ;

	sst = sst_get_aw_storage();

	request = sst->request ; 
	param = sst->param ; 

	package_cmd(request,param, cmd);
	
	/* Send request to Swd and wait */
	dprintk("begin to enter smc\n");
	ret =te_generic_smc(
			TEE_SMC_SST_COMMAND,
			virt_to_phys(request), 
			virt_to_phys(param),
			0
			);
/*
	ret = call_firmware_op(
			send_command, 
			TEE_SMC_SST_COMMAND, 
			virt_to_phys(request), 
			virt_to_phys(param),
			0
			);
*/
	dprintk("after smc\n");

	/* Parse the return from Swd */
	parse_resp(request, param, cmd);

	return 0 ; 
}

/*
 * Send the polling command to Swd 
 *
 * @param:  pret 	[out]   SWd polling work type. 
 *							SST_RESP_READ/SST_RESP_WRITE/SST_RESP_DUMMY ... 
 * @param:  ptype 	[out]	read or write work from SWd, OEM or USER data
 * @param:  pid		[out]	read or write work from SWs, 0x01,0x02 ...
 * @param:  buf		[in]	buffer if polling write cmd from SWd
 * @param:	len		[in]	buffer  len
 * @param:	retLen		[out]	actual wirte buffer  len
 *
 * @return:	0 if ok, otherwise fail.
 *
 */ 
int sst_cmd_polling_swd(
		unsigned int    *pret,
		unsigned int	*ptype,
		unsigned int	*pid,
		char			*buf,
		ssize_t			len,
		ssize_t			*retLen
		)
{

	cmd_t *cmd = (sst_get_aw_storage())->cmd ;
	int	ret = -1 ;

	mutex_lock(&com_mutex);
	do{

		/* Updata command */
		spin_lock(&cmd->lock);

		sst_memset(cmd, 0 ,sizeof(*cmd)) ;
		cmd->cmd_type		= SST_CMD_POLLING;

		cmd->resp_ret[0]	= -1 ;
		cmd->status			= SST_STAS_CMD_RDY ;
		cmd->resp_buf		= buf ;
		cmd->resp_buf_len	= len ;

		spin_unlock(&cmd->lock);

		/* Send the command */
		ret =  post_cmd_wait_resp( cmd );
		if( ret != 0 ){
			derr(" respone a fail flag 0x%x\n", ret);
			mutex_unlock(&com_mutex);
			return (- EINVAL );
		}

		/* Check the response */
		if( cmd->resp_ret[0] != SST_RESP_RET_SUCCESS ){
			derr(" cmd response fail:0x%x error\n", cmd->resp_ret[0]);
			mutex_unlock(&com_mutex);
			return (- EINVAL );
		}
		if( cmd->status != SST_STAS_CMD_RET ){
			derr(" cmd status:0x%x error\n", cmd->status);
			mutex_unlock(&com_mutex);
			return (- EINVAL );
		}
		if( cmd->resp_type != SST_RESP_DUMMY &&\
				cmd->resp_type != SST_RESP_READ &&\
				cmd->resp_type != SST_RESP_WRITE &&\
				cmd->resp_type != SST_RESP_DELETE){
			derr(" error in Swd:response type = 0x%x \n",
					cmd->resp_type);
			mutex_unlock(&com_mutex);
			return (- EINVAL );
		}	

		*pret	= cmd->resp_type;
		*ptype	= cmd->resp_ret[1];
		*pid	= cmd->resp_ret[2];
		*retLen	= cmd->resp_ret[3];

	}while( false );
	mutex_unlock(&com_mutex);

	return (0);
}

/*
 * Send the command to Swd after the read/write work done
 *
 * @param:	type[in]	request store object type 
 * @param:	id	[in]	store object id
 * @param:  buf	[in]	buffer if read cmd work done 
 * @param:	len	[out]	actual buffer len
 *
 * @return:	0 if ok, otherwise fail.
 *
 */ 
int sst_cmd_work_done(
		unsigned int type,
		unsigned int id,
		char *buf,
		ssize_t len
		)
{
	cmd_t *cmd =(sst_get_aw_storage())->cmd ;
	int	ret = -1 ;

	mutex_lock(&com_mutex);
	do{

		/* Updata command */
		spin_lock(&cmd->lock);

		sst_memset(cmd, 0 ,sizeof(*cmd)) ;
		cmd->cmd_type		= SST_CMD_WORK_DONE;
		cmd->cmd_param[0]	= type ;
		cmd->cmd_param[1]	= id ;
		cmd->cmd_buf		= buf;
		cmd->cmd_buf_len	= len ;

		cmd->resp_ret[0]	= -1 ;
		cmd->status			= SST_STAS_CMD_RDY ;

		spin_unlock(&cmd->lock);

		/* Send the command */
		ret =  post_cmd_wait_resp( cmd );
		if( ret != 0 ){
			derr(" respone a fail flag 0x%x\n", ret);
			mutex_unlock(&com_mutex);
			return (- EINVAL );
		}

		/* Check response */
		if( cmd->resp_ret[0] != SST_RESP_RET_SUCCESS ){
			derr(" cmd response fail:0x%x error\n", cmd->resp_ret[0]);
			mutex_unlock(&com_mutex);
			return (- EINVAL );
		}
		if( cmd->status != SST_STAS_CMD_RET ){
			derr(" cmd status:0x%x error\n", cmd->status);
			mutex_unlock(&com_mutex);
			return (- EINVAL );
		}
		
		if( cmd->resp_type != SST_RESP_READ_DONE &&\
				cmd->resp_type != SST_RESP_WRITE_DONE ){
			derr(" error in Swd:response type = 0x%x \n",
					cmd->resp_type);
			mutex_unlock(&com_mutex);
			return (- EINVAL );
		}	
	}while( false );
	mutex_unlock(&com_mutex);

	return (0);
}

/*
 * send binding request for OEM data
 *
 * @param:	id	[in]	store object id
 * @param:	src	[in]	request store object buffer
 * @param:	len	[in]	buffer len
 * @param:	dst	[out]	binding result buffer
 *
 * @return:	0 if ok, otherwise fail.
 *
 */ 
int sst_cmd_binding_store(
		unsigned int id ,
		char *src, 
		ssize_t len,
		char *dst,
		ssize_t dst_len ,
		ssize_t *retLen
		)
{
	cmd_t *cmd = (sst_get_aw_storage())->cmd ;
	int ret = -1 ;

	mutex_lock(&com_mutex);
	do{
		/* Updata command */
		spin_lock(&cmd->lock);

		sst_memset(cmd, 0 ,sizeof(*cmd)) ;
		cmd->cmd_type		= SST_CMD_BIND_REQ;
		cmd->cmd_param[0]	= OEM_DATA ;
		cmd->cmd_param[1]	= id ;
		cmd->cmd_buf		= src ;
		cmd->cmd_buf_len	= len ;

		cmd->resp_buf		= dst;
		cmd->resp_buf_len	= dst_len;
		cmd->resp_ret[0]	= -1 ;
		cmd->status			= SST_STAS_CMD_RDY;

		spin_unlock(&cmd->lock);

		/* Send the command */
		ret =  post_cmd_wait_resp( cmd );
		if( ret != 0 ){
			derr(" command procedure error 0x%x\n", ret);
			mutex_unlock(&com_mutex);
			return (- EINVAL );
		}

		/* Check response */
		if( cmd->resp_ret[0] != SST_RESP_RET_SUCCESS ){
			derr(" cmd response fail:0x%x error\n", cmd->resp_ret[0]);
			mutex_unlock(&com_mutex);
			return (- EINVAL );
		}
		if( cmd->status != SST_STAS_CMD_RET ){
			derr(" cmd status:0x%x error\n", cmd->status);
			mutex_unlock(&com_mutex);
			return (- EINVAL );
		}

		if( cmd->resp_type != SST_RESP_BIND_DONE )	{
			derr(" bind error in Swd:\
					len = 0x%x , response type =0x%x \n",
					cmd->resp_buf_len, cmd->resp_type);
			mutex_unlock(&com_mutex);
			return (- EFAULT );
		}
		*retLen = cmd->resp_ret[1];

	} while( false ) ;

	mutex_unlock(&com_mutex);
	return (0);
}


/*
 * update secure object data to SWd
 *
 * @param:	type[in]	request store object type 
 * @param:	id	[in]	store object id
 * @param:	src	[in]	update store object buffer
 * @param:	len	[in]	buffer len
 *
 * @return:	0 if ok, otherwise fail.
 *
 */ 
int sst_cmd_update_object(
		unsigned int type ,
		unsigned int id ,
		char *src, 
		ssize_t len,
		char * name
		)
{
	cmd_t *cmd = (sst_get_aw_storage())->cmd ;
	int ret = -1 ;

	mutex_lock(&com_mutex);
	do{
		/* Updata command */
		spin_lock(&cmd->lock);

		sst_memset(cmd, 0 ,sizeof(*cmd)) ;
		cmd->cmd_type		= SST_CMD_UPDATE_REQ;
		cmd->cmd_param[0]	= type ;
		cmd->cmd_param[1]	= id ;
		cmd->cmd_buf		= src ;
		cmd->cmd_buf_len	= len ;

		cmd->resp_ret[0]	= -1 ;
		cmd->status			= SST_STAS_CMD_RDY;
		
		if(name != NULL && type == OEM_DATA ){
			cmd->resp_buf		= name ;
			cmd->resp_buf_len	= strnlen(name,64) ;
		}
		spin_unlock(&cmd->lock);

		/* Send the command */
		ret =  post_cmd_wait_resp( cmd );
		if( ret != 0 ){
			derr(" command procedure error 0x%x\n", ret);
			mutex_unlock(&com_mutex);
			return (- EINVAL );
		}
		/* Check response */
		if( cmd->resp_ret[0] != SST_RESP_RET_SUCCESS ){
			derr(" cmd response fail:0x%x error\n", cmd->resp_ret[0]);
			mutex_unlock(&com_mutex);
			return (- EINVAL );
		}
		if( cmd->status != SST_STAS_CMD_RET ){
			derr(" cmd status:0x%x error\n", cmd->status);
			mutex_unlock(&com_mutex);
			return (- EINVAL );
		}

		if( cmd->resp_type != SST_RESP_UPDATE_DONE )	{
			derr(" update error in Swd:\
					len = 0x%x , response type =0x%x \n",
					cmd->resp_buf_len, cmd->resp_type);
			mutex_unlock(&com_mutex);
			return (- EFAULT );
		}

	} while( false ) ;

	mutex_unlock(&com_mutex);
	return (0);
}


/*
 * Send init succeuss command to Swd 
 *
 * @param:	type[in]	request store object type 
 * @param:	id	[in]	store object id
 * @param:  buf	[in]	buffer if read cmd work done 
 * @param:	len	[out]	actual buffer len
 *
 * @return:	0 if ok, otherwise fail.
 *
 */ 
int sst_cmd_init_done(void)
{
	cmd_t *cmd =(sst_get_aw_storage())->cmd ;
	int	ret = -1 ;

	mutex_lock(&com_mutex);
	do{

		/* Updata command */
		spin_lock(&cmd->lock);

		sst_memset(cmd, 0 ,sizeof(*cmd)) ;
		cmd->cmd_type		= SST_CMD_INITIALIZED;

		cmd->resp_ret[0]	= -1 ;
		cmd->status			= SST_STAS_CMD_RDY ;

		spin_unlock(&cmd->lock);

		/* Send the command */
		ret =  post_cmd_wait_resp( cmd );
		if( ret != 0 ){
			derr(" respone a fail flag 0x%x\n", ret);
			mutex_unlock(&com_mutex);
			return (- EINVAL );
		}

		/* Check response */
		if( cmd->resp_ret[0] != SST_RESP_RET_SUCCESS ){
			derr(" cmd response fail:0x%x error\n", cmd->resp_ret[0]);
			mutex_unlock(&com_mutex);
			return (- EINVAL );
		}
		if( cmd->status != SST_STAS_CMD_RET ){
			derr(" cmd status:0x%x error\n", cmd->status);
			mutex_unlock(&com_mutex);
			return (- EINVAL );
		}
		if( cmd->resp_type !=SST_RESP_INITED_DONE ){
			derr(" error in Swd:response type = 0x%x \n",
					cmd->resp_type);
			mutex_unlock(&com_mutex);
			return (- EINVAL );
		}	
	}while( false );
	mutex_unlock(&com_mutex);

	return (0);
}

/*
 * Send Swd function test request
 *
 * @param:  func_type	[in]	buffer if read cmd work done 
 * @param:	tagt_type	[in]	actual buffer len
 *
 * @return:	0 if ok, otherwise fail.
 *
 */ 
static unsigned char func_buf[128];
int sst_cmd_func_test(char * func_type, char * tagt_type, 
		unsigned int id , unsigned int *pid)
{
	cmd_t *cmd ;
	int	ret = -1 ;

	cmd= (sst_get_aw_storage())->cmd ;

	dprintk("sst_cmd_func_test: function [%s] ,target [%s]\n",
			func_type, tagt_type);
	mutex_lock(&com_mutex);
	do{
		sst_memset(func_buf, 0 ,128 );
		strncpy(func_buf, func_type, 64);
		strncpy(func_buf+64, tagt_type,64);
		/* Updata command */
		spin_lock(&cmd->lock);

		sst_memset(cmd, 0 ,sizeof(*cmd)) ;
		cmd->cmd_type		= SST_CMD_FUNC_TEST;
		cmd->cmd_param[1]	= id ;

		cmd->resp_ret[0]	= -1 ;
		cmd->status			= SST_STAS_CMD_RDY ;
		cmd->cmd_buf		= (unsigned char *)func_buf ;
		cmd->cmd_buf_len	=128 ;
		spin_unlock(&cmd->lock);

		/* Send the command */
		ret =  post_cmd_wait_resp( cmd );
		if( ret != 0 ){
			derr(" respone a fail flag 0x%x\n", ret);
			mutex_unlock(&com_mutex);
			return (- EINVAL );
		}

		/* Check response */
		if( cmd->resp_ret[0] != SST_RESP_RET_SUCCESS ){
			derr(" cmd response fail:0x%x error\n", cmd->resp_ret[0]);
			mutex_unlock(&com_mutex);
			return (- EINVAL );
		}
		if( cmd->status != SST_STAS_CMD_RET ){
			derr(" cmd status:0x%x error\n", cmd->status);
			mutex_unlock(&com_mutex);
			return (- EINVAL );
		}
		if( cmd->resp_type !=SST_RESP_FUNC_TEST_DONE ){
			derr(" error in Swd:response type = 0x%x \n",
					cmd->resp_type);
			mutex_unlock(&com_mutex);
			return (- EINVAL );
		}
		if(pid)
			*pid = cmd->resp_ret[1];
	}while( false );
	mutex_unlock(&com_mutex);

	return (0);
}

