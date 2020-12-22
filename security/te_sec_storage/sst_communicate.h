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
 
#ifndef _SST_COMMUNICATE_H_
#define _SST_COMMUNICATE_H_

#include <linux/spinlock.h>

/*communication structure*/
typedef struct command{
	unsigned int	cmd_type;
	unsigned int	cmd_param[4];
	unsigned char	*cmd_buf;
	unsigned int	cmd_buf_len ;

	unsigned int	resp_type;
	unsigned int	resp_ret[4];
	unsigned char   *resp_buf;
	unsigned int	resp_buf_len;

	unsigned int	status;
	spinlock_t	lock; 
}cmd_t; 

enum{/*cmd type*/
	SST_CMD_POLLING =1 ,
	SST_CMD_BIND_REQ ,
	SST_CMD_WORK_DONE,
	SST_CMD_UPDATE_REQ,
	SST_CMD_INITIALIZED,
	SST_CMD_FUNC_TEST,
};

enum{/*response type*/
	SST_RESP_DUMMY = 1,
	SST_RESP_READ,
	SST_RESP_READ_DONE,
	SST_RESP_WRITE,
	SST_RESP_WRITE_DONE,
	SST_RESP_DELETE,
	SST_RESP_BIND_DONE,
	SST_RESP_UPDATE_DONE,
	SST_RESP_INITED_DONE, 
	SST_RESP_FUNC_TEST_DONE,
};

enum{/*response result*/
	SST_RESP_RET_SUCCESS = 0,
	SST_RESP_RET_FAIL,
};
enum{ /*cmd status*/
	SST_STAS_CMD_INIT =1,/*Nwd*/
	SST_STAS_CMD_RDY , /*Nwd*/
	SST_STAS_CMD_POST ,/*Nwd*/
	SST_STAS_CMD_GET,  /*Swd*/
	SST_STAS_CMD_DEAL,/*Swd*/
	SST_STAS_CMD_RET, /*Swd*/
	SST_STAS_CMD_DONE,/*Nwd*/
};

extern int sst_cmd_polling_swd(
	unsigned int	*pret,
	unsigned int	*ptype,
	unsigned int	*pid,
	char			*buf,
	ssize_t			len,
	ssize_t			*retLen
	);

extern int sst_cmd_work_done(
		unsigned int id,
		unsigned int type,
		char *buf,
		ssize_t len
		);

extern int sst_cmd_binding_store(
		unsigned  id ,
		char *src, 
		ssize_t len,
		char *dst,
		ssize_t dst_len,
		ssize_t *retLen
		);

extern int sst_cmd_update_object(
		unsigned int type ,
		unsigned int id ,
		char *src, 
		ssize_t len,
		char *name
		);
extern int sst_cmd_init_done(void);
extern int sst_cmd_func_test(char * func_type, char * tagt_type, 
		unsigned int id , unsigned int *pid);
#endif
