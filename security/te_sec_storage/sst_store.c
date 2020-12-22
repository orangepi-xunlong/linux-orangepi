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
#include <linux/err.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/crc32.h>
#include <linux/secure/te_protocol.h>

#include "sst.h"
#include "sst_debug.h"

char *user_path="/data/secure_store";
static  char oem_class[MAX_OEM_STORE_NUM][64];

static int sst_open( st_path_t path, type_t type);
static int sst_write( st_path_t path, char *src , ssize_t len, type_t type);
static int sst_read( st_path_t path, char *dst , ssize_t len, type_t type);
static int sst_probe(struct secure_storage_t *sst, type_t type);
static int sst_update(struct secure_storage_t *sst, type_t type);


extern int sunxi_secure_storage_write(const char *item_name, char *buffer, int length);
extern int sunxi_secure_storage_read(const char *item_name, char *buffer, int buffer_len, int *data_len);
extern int sunxi_secure_storage_update(const char *item_name, char *buffer, int length);
extern int sunxi_secure_storage_init(void);
extern int sunxi_secure_storage_probe(const char *item_name);
extern int sunxi_secure_storage_init_oem_class(void *oem_class);

static int __help_oem_object_encrypt(store_object_t *object)
{
	char	*buf = NULL ;
	int		ret	 = -1 ;	
	unsigned int retLen ;

	if( object->re_encrypt != STORE_REENCRYPT_MAGIC ){

		dprintk("oem object need to re-encrypt\n");	

		buf = kzalloc( MAX_STORE_LEN, GFP_KERNEL );
		if(!buf){
			derr("- out of memory\n ");
			return (- ENOMEM);
		}
		dprintk("bind param: resp buf:0x%x", (unsigned int)buf);
		/* Requset re-encrypt */
		ret =  sst_cmd_binding_store(
				object->id,
				object->data,
				object->actual_len,
				buf,
				MAX_STORE_LEN,
				&retLen);
		if( ret != 0 ){
			derr("-re_encrypt fail \n ");
			sst_memset(buf, 0, MAX_STORE_LEN);
			kfree(buf);
			return (- EFAULT);
		}

		sst_memset(object->data, 0, MAX_STORE_LEN);
		sst_memcpy( object->data,
				buf,
				retLen);

		sst_memset(buf, 0, MAX_STORE_LEN);
		kfree(buf);

		object->re_encrypt = STORE_REENCRYPT_MAGIC;
		object->actual_len = retLen ;
		object->crc = ~crc32_le(~0, (unsigned char const *)object, sizeof(*object)-4 );
		
		
		dprintk("After re-encrypt object:\n");	
		__object_dump(object);
		/* Write re-encrypt result to nand/emmc */
#ifndef	SST_BIND_DEBUG
		ret = sst_write( (st_path_t)object->name,
				(char *)object,
				sizeof(*object),
				OEM_DATA); 
		if(ret != 0){
			derr("-re_encrypt write to nand/emmc fail \n ");
			return (- EFAULT);
		}
		
#endif 
	}
	return (0);
}


static int _sst_oem_read( const char * name	, char *buf, ssize_t len)
{

	int retLen = -1;
	int ret  ;
	ret = sunxi_secure_storage_read(name, buf, len, &retLen);
	if(ret < 0){
		dprintk("oem secure storage read[%s] fail\n",name);
		return -1;
	}
	return retLen ;
}

static int _sst_oem_write( const char *name, char *buf, ssize_t len)
{
	return sunxi_secure_storage_write(name, buf , len ) ;
}

static int _sst_oem_open(void)
{

	return  sunxi_secure_storage_init();
}

int _sst_user_open(char *filename )
{
    struct file *fd;
    mm_segment_t old_fs ;

	if(!filename){
		derr("- filename NULL\n");
		return (-EINVAL);
	}

	dprintk(": file %s\n" , filename);
    old_fs = get_fs();
    set_fs(KERNEL_DS);
    
    fd = filp_open(filename, O_RDONLY, 0);

    if(IS_ERR(fd)){
		set_fs(old_fs);
        return -1;
    }   
	
	filp_close(fd, NULL);
    set_fs(old_fs);

	dprintk(": file %s open done \n",filename);
	return (0) ;
}

int _sst_user_ioctl(char *filename, int ioctl, void *param)
{
	struct file *fd;
	int ret = -1;

	mm_segment_t old_fs = get_fs();

	if(!filename ){
		dprintk("- filename NULL\n");
		return (-EINVAL);
	}

	old_fs = get_fs();
	set_fs(KERNEL_DS);

	fd = filp_open(filename, O_WRONLY|O_CREAT, 0666);

	if(IS_ERR(fd)) {
		dprintk(" -file open fail\n");
		return -1;
	}
	do{
		if ((fd->f_op == NULL) || (fd->f_op->unlocked_ioctl == NULL))
		{
			dprintk(" -file can't to write!!\n");
			break;
		} 

		ret = fd->f_op->unlocked_ioctl(
				fd,
				ioctl,
				(unsigned int)(param));			

	}while(false);

	vfs_fsync(fd, 0);
	filp_close(fd, NULL);
	set_fs(old_fs);

	return ret ;
}
int _sst_user_read(char *filename, char *buf, ssize_t len,int offset)
{	
	struct file *fd;
	//ssize_t ret;
	int retLen = -1;
	mm_segment_t old_fs ;

	if(!filename || !buf){
		derr("- filename/buf NULL\n");
		return (-EINVAL);
	}

	old_fs = get_fs();
	set_fs(KERNEL_DS);

	fd = filp_open(filename, O_RDONLY, 0);

	if(IS_ERR(fd)) {
		derr(" -file open fail\n");
		return -1;
	}
	do{
		if ((fd->f_op == NULL) || (fd->f_op->read == NULL))
		{
			derr(" -file can't to open!!\n");
			break;
		} 

		if (fd->f_pos != offset) {
			if (fd->f_op->llseek) {
				if(fd->f_op->llseek(fd, offset, 0) != offset) {
					derr(" -failed to seek!!\n");
					break;
				}
			} else {
				fd->f_pos = offset;
			}
		}    		

		retLen = fd->f_op->read(fd,
				buf,
				len,
				&fd->f_pos);			

	}while(false);

	filp_close(fd, NULL);
	set_fs(old_fs);

	return retLen;
}

int _sst_user_write(char *filename, char *buf, ssize_t len, int offset)
{	
	struct file *fd;
	//ssize_t ret;
	int retLen = -1;
	mm_segment_t old_fs = get_fs();

	dprintk("Write to %s\n",filename);
	if(!filename || !buf){
		derr("- filename/buf NULL\n");
		return (-EINVAL);
	}

	old_fs = get_fs();
	set_fs(KERNEL_DS);

	fd = filp_open(filename, O_WRONLY|O_CREAT, 0666);

	if(IS_ERR(fd)) {
		derr(" -file open fail %s \n", filename);
		return -1;
	}
	do{
		if ((fd->f_op == NULL) || (fd->f_op->write == NULL))
		{
			derr(" -file can't to write!!\n");
			break;
		} 

		if (fd->f_pos != offset) {
			if (fd->f_op->llseek) {
				if(fd->f_op->llseek(fd, offset, 0) != offset) {
					derr(" -failed to seek!!\n");
					break;
				}
			} else {
				fd->f_pos = offset;
			}
		}       		

		retLen = fd->f_op->write(fd,
				buf,
				len,
				&fd->f_pos);			

	}while(false);

	vfs_fsync(fd, 0);
	filp_close(fd, NULL);
	set_fs(old_fs);

	dprintk("Write %x to %s done\n",retLen,filename);
	return retLen;
}

static int _sst_user_create(char *filename )
{

	store_object_t * object ;

	object = (store_object_t *)kzalloc(
						sizeof(store_object_t),	
						GFP_KERNEL) ;
	if(!object){
		derr("out of memroy\n");
		return (- ENOMEM );
	}

	dprintk(": file %s\n" , filename);

	if( _sst_user_write(filename, (void *) object, sizeof(store_object_t),0 ) 
			!= sizeof(store_object_t)){
		derr("create file %s fail\n", filename);
		kfree(object);
		return -1 ;
	}

	kfree(object);
	dprintk(": file %s create done \n",filename);
	return (0) ;
}

/*
 * Test for user storage file operation
 */ 
int sst_user_file_test(void)
{
	int				ret = 0 ;
	store_map_t		*map = NULL	;
	u8				*buf = NULL ;	
	struct secure_storage_t *sst = sst_get_aw_storage();
	if(!sst ){
		derr("- input NULL\n");
		return (- EINVAL);
	}

	buf= (u8 *)kzalloc( sizeof(store_object_t),GFP_KERNEL);
	if(!buf){
		derr("- out of memory\n");
		return (-ENOMEM);
	}
	
	dprintk("(%p)\n", sst_user_file_test);
	list_for_each_entry(map, &sst->user_list,list)	{

		store_object_t *object =(store_object_t *)buf;
		object->magic = STORE_OBJECT_MAGIC;
		object->version = 1;
		object->actual_len = 0x100;
		object->id++;
		object->crc = ~crc32_le(~0,
								buf, 
								sizeof(store_object_t)-4
								);

		ret =  _sst_user_write(map->desc->dir, 
								(char *)object,
								sizeof(*object),
								0);

		if( ret != sizeof(*object) ){
			derr("-object write fail\n");
			ret = -1 ;
			goto out ;
		}


			ret = _sst_user_read(map->desc->dir,
					buf,	
					sizeof(store_object_t),
					0);

			if(ret != sizeof(store_object_t) ){
				derr("-object read fail\n");
				ret = -1 ;
				goto out ;
			}
			
		
			__object_dump( (store_object_t *)buf );
			fprintk("write/read 0x%s test done\n",map->desc->dir);

	}
	dprintk(" done\n");	
out:
	sst_memset(buf, 0, sizeof(store_object_t));
	kfree(buf);
	return ret ;
}

static int sst_read( st_path_t path, char *dst , ssize_t len, type_t type)
{
	int ret = -1 ;

	if( type == OEM_DATA)
		ret = _sst_oem_read( path.name, dst, len) ;
	else 
		ret = _sst_user_read( path.file, dst, len,0);

	if( ret != len ) {
		derr("sst read error\n");
		return ret ;
	}


	return ret ;
}

static int sst_delete(st_path_t path, type_t type)
{
	char * buf ;

	buf =(char *)kzalloc(sizeof(store_object_t), GFP_KERNEL);
	if(!buf){
		derr("-out of memory\n");
		return - ENOMEM ;
	}
	

	if( sst_write( path, buf,sizeof(store_object_t), type) 
			!= sizeof(store_object_t) ){
		derr("write fail\n");
		kfree(buf);
		return -1 ;
	}

	kfree(buf);
	return 0 ;
}

static int sst_update(struct secure_storage_t *sst, type_t type)
{	
	int	ret = 0 ,
		cnt = 0 ;
	store_map_t		*map	;
	struct list_head  *head;

	dprintk("(%p)\n", sst_update);

	if( sst_probe(sst, type) != 0 ){
		derr("- probe fail\n");
		return -1 ;
	}

	if( type == OEM_DATA ){
		cnt = sst->oem_size ;
		head = &sst->oem_list;
	}else{
		cnt = sst->user_size ;
		head = &sst->user_list;
	}

	dprintk(" Update %d object to SWd\n",cnt);

	list_for_each_entry(map, head ,list) {
		if(cnt <=0)
			goto out ;
		dprintk(" Try to update object %d:\n",cnt);
		__object_dump(map->object);	

		if(check_object_valid(map->object) ==0 ){
			dprintk(" Update object %d:\n",cnt);
			ret = sst_cmd_update_object(
					type ,
					map->object->id ,
					map->object->data, 
					map->object->actual_len	,
					map->object->name
					);
			if(ret != 0 ){
				derr("-object update fail\n");
				return -1 ;
			}
			dprintk("Update object %d done\n",cnt );
			map->desc->flag = STORE_UPDATE_DONE_MAGIC; 
			cnt-- ; 
		}

	}
out:
	dprintk(" done\n");	
	return 0 ;
}

static int sst_write( st_path_t path, char *src , ssize_t len, type_t type)
{

	if( type == OEM_DATA )
		return _sst_oem_write( path.name, src, len);
	else
		return _sst_user_write( path.file, src, len, 0);
}

static int sst_create( st_path_t path, type_t type )
{
	if( type == USER_DATA )
		return _sst_user_create( path.file );
	else
		return -1 ;
}

static int sst_open( st_path_t path, type_t type)
{
	if( type == OEM_DATA )
		return _sst_oem_open();
	else
		return _sst_user_open( path.file);
}

static int _sst_user_probe(struct secure_storage_t *sst )
{
	int	ret = 0 ;
	store_map_t		*map	;
	store_object_t	*object ;	
	st_op_t			*st_op ;
	struct list_head  *head;

	dprintk("(%p)\n", _sst_user_probe);

	object= ( store_object_t *)kzalloc(\
			sizeof(store_object_t),
			GFP_KERNEL
			);

	if(!object){
		derr("- out of memory\n");
		return (-ENOMEM);
	}

	st_op	= sst->st_operate;
	head = &sst->user_list;

	list_for_each_entry(map, head ,list) {

		/*
		 * Must check the whole file list
		 */ 

		if( st_op->open( (st_path_t)map->desc->dir, 
					USER_DATA) == 0 )
		{
			do{ 
				ret = st_op->read( (st_path_t)map->desc->dir,
						(char *)object,
						sizeof(store_object_t),
						USER_DATA	
						);

				if(ret != sizeof(store_object_t) ){
					derr("-object read fail\n");
					ret = -1 ;
					goto out ;
				}

				if(object->magic == STORE_OBJECT_MAGIC ){ /*valid file*/
					if( check_object_valid((store_object_t *)object ) != 0 ){
						derr("-sst read check fail\n");
						ret = -1 ;
						goto out ;
					}

					dprintk("Find a valid user object\n");
					sst->user_size ++ ;
			
					sst_memcpy( map->object,
							object,
							sizeof(store_object_t)) ;

					map->desc->vaild = true;
					map->desc->flag = STORE_PROBE_DONE_MAGIC ; 
				}
			}while(0);
		}else{ /*if not exist, create it with empty data */
			 st_op->create( (st_path_t)map->desc->dir, 
					USER_DATA); 
		}

	}
	ret = 0 ;
	dprintk(" done\n");	
out:
	sst_memset(object, 0, sizeof(store_object_t));
	kfree(object);
	return ret ;
}


extern char *ext_class ;

#if 0
static int __vaild_oem_class(void) 
{
	int i ;
	for( i=0 ; i<MAX_OEM_STORE_NUM ; ++i){
		if(oem_class[i] == 0 || *(oem_class[i]) == '\0' )
			return i ;
	}
	return 0 ;
}

static void  __fill_oem_class( char *name  )
{
	int size ;

	size = __vaild_oem_class();
	strncpy( oem_class[size], name, 64);

}
static int __find_oem_class( char *name  )
{
	int i ;
	int size ;

	size = __vaild_oem_class();
	for(i=0 ; i< size ; i++ )
		if( strncmp( name, oem_class[i], 64 ) == 0 ){
			return i ;
		}
	return -1 ;
}
static void  __fetch_oem_ext_class(void)
{
	int i =0 ;
	char class[64];

	char * p = ext_class ;
	dprintk("(%p) extern class :%s\n",__fetch_oem_ext_class , p );
	if(!p)
		return ;
	while( p[i] !='\0' ){
			if( p[i] == ',' && i <64 ){
				sst_memcpy(class, p, i)	;
				class[i]='\0';
				__fill_oem_class(class);
			}
			i++ ;
	}
}

#endif 

static int _sst_oem_probe( struct secure_storage_t *sst )
{
	int	ret = 0 ;
	store_map_t		*map	;
	store_object_t	*object ;	
	st_op_t			*st_op ;
	struct list_head  *head;
	int	index  , size;

	dprintk("(%p)\n", _sst_oem_probe);

	object= ( store_object_t *)kzalloc(\
			sizeof(store_object_t),
			GFP_KERNEL
			);

	if(!object){
		derr("- out of memory\n");
		return (-ENOMEM);
	}

	st_op	= sst->st_operate;
	head = &sst->oem_list;

	if( st_op->open( (st_path_t)( (char *)0) , OEM_DATA) !=0){
		derr("oem open fail\n");
		return - EINVAL ;
	}
	
	memset(oem_class, 0, 64*MAX_OEM_STORE_NUM);	
	size = sunxi_secure_storage_init_oem_class(oem_class);
#if 0
	__fetch_oem_ext_class();
	size= __vaild_oem_class();
#endif
	index =0 ;
	list_for_each_entry(map, head ,list) {
		if( sunxi_secure_storage_probe( oem_class[index ] ) < 0 ){
			dprintk("oem class[%s] secure data is invaild\n", oem_class[index]);

		}else{ // probe valid oem class

			do{
				sst_memset(object, 0 ,sizeof(object));
				ret = st_op->read((st_path_t) ((char *)oem_class[index]),
						(char *)object, 
						sizeof(*object), 
						OEM_DATA);

				if(ret != sizeof(store_object_t) ){
					derr("-object read fail\n");
					ret = -1 ;
					goto out ;
				}
				
				__object_dump(object);	
				if(object->magic == STORE_OBJECT_MAGIC ) 
					{ /*valid data*/
					if( check_object_valid((store_object_t *)object ) != 0 ){
						derr("-sst read check fail\n");
						ret = -1 ;
						goto out ;
					}
					
					object->id = index ;
/*					if( (ret = __help_oem_object_encrypt(object)) != 0 ){
						derr("oem re-encrypt fail\n");
						goto out ;	
					}
					*/
					sst->oem_size ++ ;

				}else{
					derr("invaild secure store \n");
					goto out ;
				}
				sst_memcpy( map->object,
						object,
						sizeof(store_object_t)) ;
				
				map->desc->id = index ;
				map->desc->vaild = true;
				map->desc->flag = STORE_PROBE_DONE_MAGIC ; 

			}while(0);

		}
		if( ++index   >= size )
			break ;

	} // end loop
	ret =0 ;
	__oem_map_dump(sst);
	dprintk("done\n");
out:
	sst_memset(object, 0, sizeof(store_object_t));
	kfree(object);
	return ret ;
}


static int sst_probe(struct secure_storage_t *sst, type_t type)
{
	if( type == OEM_DATA )
		return _sst_oem_probe(sst);
	else if( type == USER_DATA )
		return _sst_user_probe(sst);
	else{
		dprintk("Wrong type : %d\n", (unsigned int)type);
		return -1 ;
	}
		

}

int sst_operation_init( st_op_t * st_op)
{
	st_op->read	= sst_read;
	st_op->write = sst_write;
	st_op->open	= sst_open ;
	st_op->update = sst_update;
	st_op->delete = sst_delete;
	st_op->create = sst_create ;
	return 0 ;
}


