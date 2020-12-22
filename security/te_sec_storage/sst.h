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

#ifndef _SST_STORAGE_H_
#define _SST_STORAGE_H_

#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/list.h>
#include <linux/workqueue.h>
#include <linux/moduleparam.h>
#include <linux/secure/te_protocol.h>

#include "sst_communicate.h"

/* Internal Test Marco  */

/* Test re-encrypt flow by do re-encrypt every boot-up*/
//#define SST_BIND_DEBUG

/* Enable a test daemon to test SST function*/
//#define SST_FUNCTION_TEST_DAEMON

/*Test OEM storage in file-system */
//#define OEM_STORE_IN_FS


extern int sst_debug_mask;

#undef dprintk
#define dprintk(format, arg...)        \
	do { \
		if (sst_debug_mask) \
		printk(KERN_DEBUG "[Nwd-SST](%s) " format, \
				__FUNCTION__, ## arg);  \
	}while(0)

#define fprintk(format, arg...)        \
	do { \
		if (sst_debug_mask) \
		printk(KERN_DEBUG format, ## arg);    \
	}while(0)


#define derr(format, arg...)        \
	do { \
		printk(KERN_ERR "[Nwd-SST](%s): " format, \
				__FUNCTION__, ## arg);  \
	}while(0)


#define MAX_OEM_STORE_NUM		0x10
#define MAX_USER_STORE_NUM		0x100	


typedef  int id_t ;
typedef enum TYPE{
	OEM_DATA = 1,
    USER_DATA,
}type_t;

#define STORE_UPDATE_DONE_MAGIC 0x82743820
#define STORE_PROBE_DONE_MAGIC 0x26351894
typedef struct{
	type_t		type ; /*data type. OEM or user data*/
	char		dir[64];
	id_t		id;    /*store id , 0x01,0x02...*/
	
	bool		vaild ;/*store object crc check result*/
	int			flag ; /*probe result*/
}store_desc_t ;


#define MAX_STORE_LEN 0xc00 /*3K payload*/
#define STORE_OBJECT_MAGIC	0x17253948
#define STORE_REENCRYPT_MAGIC 0x86734716
typedef struct{
	unsigned int	magic ; /* store object magic*/
	id_t			id ;    /*store id, 0x01,0x02.. */
	char			name[64]; /*OEM name*/
    unsigned int	re_encrypt; /*flag for OEM object*/
	unsigned int	version ;	
	unsigned int	reserved[4];
	unsigned int	actual_len ; /*the actual len in data buffer*/
	unsigned char	data[MAX_STORE_LEN]; /*the payload of secure object*/
	unsigned int	crc ; /*crc to check the sotre_objce valid*/
}store_object_t;

typedef struct{
	struct list_head	list;
	store_desc_t		*desc ;
	store_object_t		*object ;
	struct mutex		mutex ;
}store_map_t;


struct secure_storage_t ;

/*secure storage operate */
typedef union storage_path{
	char	*name; // OEM store by name
	char	*file;   // user store by file name
}st_path_t;

typedef struct storage_operation{

	int ( *open )(st_path_t path ,
				  type_t	type);

	int ( *read )(st_path_t	path,
				   char		*dst,
				   ssize_t	len,
					type_t	type);
	int ( *write)(st_path_t	path,
					char	*src,
					ssize_t	len,
					type_t	type);
	int ( *delete)(st_path_t	path,
					type_t	type);

	int ( *update)(struct secure_storage_t *sst,
				  type_t	type);
	int ( *create)(st_path_t path ,
				  type_t	type);
}st_op_t ;

typedef	struct command_work{
		unsigned int resp;
		unsigned int type;
		unsigned int id;
		char	*buf;
		ssize_t	len ;
		int		ret ; /*work result*/
		struct work_struct work;
	}cmd_work_t;

struct secure_storage_t{

	struct task_struct	*	tsk ;
	struct task_struct	*	test_tsk ;

	struct list_head	    oem_list;
	int		oem_size;
	struct list_head		user_list;

	/*
	 * avaliable user list node
	 * avoid to travel the list every time*/
	int		user_size;	

	cmd_t					*cmd ;
	st_op_t					*st_operate;

	cmd_work_t				*cmd_work ;

	/* TE commucation interface*/
	struct te_request *request ;
	struct te_oper_param *param ;

};


#define ROUND_UP(N, S) ((((N) + (S) - 1) / (S)) * (S))

#define SECBLK_READ							_IO('V',20)
#define SECBLK_WRITE						_IO('V',21)
#define SECBLK_IOCTL						_IO('V',22)

struct secblc_op_t{
	int item;
	unsigned char *buf;
	unsigned int len ;
};

extern char *user_path;

/* Store APIs*/
extern struct secure_storage_t * sst_get_aw_storage(void );
extern int sst_operation_init( st_op_t * st_op);
extern int sst_user_file_test(void);


/*Internal help APIs*/
/*check crc and magic*/
extern int check_object_valid(store_object_t *object);

/*User file operateion in sst*/
extern int _sst_user_open(char *filename );
extern int _sst_user_read(char *filename, char *buf, ssize_t len,int offset);
extern int _sst_user_write(char *filename, char *buf, ssize_t len, int offset);
extern int _sst_user_ioctl(char *filename, int ioctl, void *param);
#endif
