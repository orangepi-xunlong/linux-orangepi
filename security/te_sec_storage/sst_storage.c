/* secure no-voliate memory(nand/emmc) driver for sunxi platform 
 *
 * Copyright (C) 2014 Allwinner Ltd. 
 *
 * Author:
 *	Ryan Chen <ryanchen@allwinnertech.com>
 *
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
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/crc32.h>
#include <asm/firmware.h>

#include "sst.h"
#include "sst_debug.h"

#ifdef OEM_STORE_IN_FS 
#define EMMC_SEC_STORE	"/data/oem_secure_store"
#endif

#define SEC_BLK_SIZE						(4096)
#define MAX_SECURE_STORAGE_MAX_ITEM          (32)
static unsigned char secure_storage_map[SEC_BLK_SIZE] = {0};
static unsigned int  secure_storage_inited = 0;

/*
 * EMMC parameters 
 */
#define SDMMC_SECTOR_SIZE				(512)
#define SDMMC_SECURE_STORAGE_START_ADD  (6*1024*1024/512)//6M
#define SDMMC_ITEM_SIZE                                 (4*1024/512)//4K
static char *sd_oem_path="/dev/block/mmcblk0" ;

/*
 * Nand parameters
 */
static char *nand_oem_path="/dev/block/by-name/bootloader";
static struct secblc_op_t secblk_op ;
/*
 *  secure storage map
 *
 *  section 0:
 *		name1:length1
 *		name2:length2
 *			...
 *	section 1:
 *		data1 ( name1 data )
 *	section 2 :
 *		data2 ( name2 data ) 
 *			... 
 */

#define FLASH_TYPE_NAND  0
#define FLASH_TYPE_SD1  1
#define FLASH_TYPE_SD2  2
#define FLASH_TYPE_UNKNOW	-1

static int flash_boot_type = FLASH_TYPE_UNKNOW ;
static char cmdline[1024];
#define CMDLINE_FILE_PATH "/proc/cmdline"
static int getInfoFromCmdline(char* key, char* value)
{
	char *p = NULL ;
	int line_len , key_len;
	int ret ;

	memset(cmdline,0,1024);

	ret = _sst_user_read( CMDLINE_FILE_PATH, cmdline,1024,0);
	if(ret < 0 || ret >1024 ){
		derr("Cmd line read fail\n");	
		return -1;
	}

	line_len = ret ;
	key_len = strnlen(key,1024);
	p=cmdline ;
	while( p < cmdline + line_len - key_len){
		if( strncmp(key, p , key_len ) )
			p ++ ;
		else{
	memcpy(value, p+key_len,1);	
			return 0 ;
		}
	}

    strcpy(value, "-1");
    return -1;

}

static int get_flash_type(void)
{
	char ctype[16];

	memset(ctype, 0, 16);
	if( getInfoFromCmdline("boot_type=", ctype) ){
		derr("Get boot type cmd line fail\n");
		return -1 ;
	}
	
	flash_boot_type= simple_strtol(ctype, NULL, 10);
	dprintk("Boot type %d\n", flash_boot_type);
	return 0;
}

/*nand secure storage read/write*/
static int _nand_read(int id, char *buf, ssize_t len)
{	
	if(!buf){
		dprintk("-buf NULL\n");
		return (-EINVAL);
	}
	if(id >MAX_SECURE_STORAGE_MAX_ITEM){
		dprintk("out of range id %x\n", id);
		return (-EINVAL);
	}
	secblk_op.item = id ;
	secblk_op.buf = buf;
	secblk_op.len = len ;

	return  _sst_user_ioctl(nand_oem_path, SECBLK_READ,&secblk_op);			
}

static int _nand_write( int	id, char *buf, ssize_t len)
{	
	if(!buf){
		dprintk("- buf NULL\n");
		return (-EINVAL);
	}

	if(id >MAX_SECURE_STORAGE_MAX_ITEM){
		dprintk("out of range id %x\n", id);
		return (-EINVAL);
	}
	secblk_op.item = id ;
	secblk_op.buf = buf;
	secblk_op.len = len ;

	return  _sst_user_ioctl(nand_oem_path, SECBLK_WRITE,&secblk_op);			

}

/*emmc secure storage read/write*/
static int _sd_read(int id, char *buf, ssize_t len)
{
	int offset ,ret ;
	char *align, *sd_align_buffer ;

	if(!buf){
		dprintk("-buf NULL\n");
		return (-EINVAL);
	}

	if(id >MAX_SECURE_STORAGE_MAX_ITEM){
		dprintk("out of range id %x\n", id);
		return (-EINVAL);
	}

	sd_align_buffer = kmalloc(SEC_BLK_SIZE + 64 , GFP_KERNEL);
	if(!sd_align_buffer){
		dprintk("out of memory\n");
		return - ENOMEM;
	}

	align = (char *)(((unsigned int)sd_align_buffer+0x20)&(~0x1f));

	offset = (SDMMC_SECURE_STORAGE_START_ADD+SDMMC_ITEM_SIZE*2*id) *SDMMC_SECTOR_SIZE ;

	ret =  _sst_user_read( sd_oem_path, align, len, offset);
	if(ret != len){
		dprintk("_sst_user_read: read request len 0x%x, actual read 0x%x\n",
				len, ret);
		kfree(sd_align_buffer);
		return -1;
	}
	memcpy(buf,align,len);

	kfree(sd_align_buffer);
	return 0 ;
}

static int _sd_write( int id, char *buf, ssize_t len)
{
	int offset, ret ; 
	char *align, *sd_align_buffer ;

	if(!buf){
		dprintk("- buf NULL\n");
		return (-EINVAL);
	}

	if(id >MAX_SECURE_STORAGE_MAX_ITEM){
		dprintk("out of range id %x\n", id);
		return (-EINVAL);
	}

	sd_align_buffer = kmalloc(SEC_BLK_SIZE + 64 , GFP_KERNEL);
	if(!sd_align_buffer){
		dprintk("out of memory\n");
		return - ENOMEM;
	}

	align = (char *)(((unsigned int)sd_align_buffer+0x20)&(~0x1f));
	memcpy(align, buf, len);

	offset = (SDMMC_SECURE_STORAGE_START_ADD+SDMMC_ITEM_SIZE*2*id)*SDMMC_SECTOR_SIZE  ;

	ret =  _sst_user_write( sd_oem_path, align, len, offset);
	if(ret != len){
		dprintk("_sst_user_write: write request len 0x%x, actual write 0x%x\n",
				len, ret);
		kfree(sd_align_buffer);
		return -1;
	}

	kfree(sd_align_buffer);
	return 0 ;

}
static int nv_write( int id,char *buf, ssize_t len )
{
	int ret ;
	switch(flash_boot_type){
		case FLASH_TYPE_NAND:
			ret = _nand_write(id, buf, len);
			break;
		case FLASH_TYPE_SD1:
		case FLASH_TYPE_SD2:
			ret = _sd_write(id, buf,len);
			break;
		default:
			pr_err("Unknown no-volatile device\n");
			ret = -1 ;
			break; 
	}

	return ret ;

}

static int nv_read( int	id, char *buf, ssize_t len)
{
	int ret ;
	switch(flash_boot_type){
		case FLASH_TYPE_NAND:
			ret = _nand_read(id, buf, len);
			break;
		case FLASH_TYPE_SD1:
		case FLASH_TYPE_SD2:
			ret = _sd_read(id,buf,len); 
			break;
		default:
			pr_err("Unknown no-volatile device\n");
			ret = -1 ;
			break; 
	}

	return ret ;

}

/*Low-level operation*/
static int sunxi_secstorage_read(int item, unsigned char *buf, unsigned int len)
{
#ifdef OEM_STORE_IN_FS 
	int ret ;
	struct secure_storage_t * sst ;

	char file[128];
	sst =  sst_get_aw_storage();
	sst_memset(file,0, 128);

	sprintf(file, "%s/08%d.bin", EMMC_SEC_STORE, item);
	ret=(sst->st_operate->read((st_path_t)file, buf, len , USER_DATA) );
	return (ret != len);
#else
	return nv_read(item, buf, len);
#endif
}

static int sunxi_secstorage_write(int item, unsigned char *buf, unsigned int len)
{
#ifdef OEM_STORE_IN_FS 
	int ret ;
	struct secure_storage_t * sst ;
	char file[128];

	sst =  sst_get_aw_storage();
	sst_memset(file,0, 128);
	sprintf(file, "%s/08%d.bin", EMMC_SEC_STORE, item);
	ret= (sst->st_operate->write((st_path_t)file, buf, len , USER_DATA) );
	return (ret != len );

#else
	return nv_write(item, buf, len);
#endif
}


/*
 * Map format:
 *		name:length\0
 *		name:length\0
 */
static int __probe_name_in_map(unsigned char *buffer, const char *item_name, int *len)
{
	unsigned char *buf_start = buffer;
	int   index = 1;
	char  name[64], length[32];
	int   i,j, name_len ;

	dprintk("__probe_name_in_map\n");

	while(*buf_start != '\0')
	{
		sst_memset(name, 0, 64);
		sst_memset(length, 0, 32);
		i=0;
		while(buf_start[i] != ':')
		{
			name[i] = buf_start[i];
			i ++;
		}
		name_len=i ;
		i ++;j=0;
		while( (buf_start[i] != ' ') && (buf_start[i] != '\0') )
		{
			length[j] = buf_start[i];
			i ++;j++;
		}

		if(memcmp(item_name, name,name_len ) ==0 )
		{
			buf_start += strlen(item_name) + 1;
			*len = simple_strtoul((const char *)length, NULL, 10);
			return index;
		}
		index ++;
		buf_start += strlen((const char *)buf_start) + 1;
	}

	return -1;
}

static int __fill_name_in_map(unsigned char *buffer, const char *item_name, int length)
{
	unsigned char *buf_start = buffer;
	int   index = 1;
	int   name_len;

	while(*buf_start != '\0')
	{
		dprintk("name in map %s\n", buf_start);

		name_len = 0;
		while(buf_start[name_len] != ':')
			name_len ++;
		if(!memcmp((const char *)buf_start, item_name, name_len))
		{
			return index;
		}
		index ++;
		buf_start += strlen((const char *)buf_start) + 1;
	}
	if(index >= 32)
		return -1;

	sprintf((char *)buf_start, "%s:%d", item_name, length);

	return index;
}


int sunxi_secure_storage_init_oem_class(void *oem_class)
{
	int id ,i ,size ;
	char * buf, *name;
	unsigned int re_encrypt; 

	buf = (char *)kmalloc(SEC_BLK_SIZE,GFP_KERNEL);
	if(!buf){
		dprintk("out of memory\n");
		return - ENOMEM;
	}

	/*Skip map sector*/
	for(id = 0,i=1 ;id < MAX_OEM_STORE_NUM && i<32 ; i++){
		memset(buf, 0 ,SEC_BLK_SIZE);
		if( sunxi_secstorage_read(i, buf, SEC_BLK_SIZE) <0 ){
			dprintk("%s : secstorage read fail\n",__func__);
			kfree(buf);
			return - EIO ;
		}
		/*Find oem object*/
		if( (((store_object_t *)buf)->re_encrypt  == STORE_REENCRYPT_MAGIC) && 
				( check_object_valid((store_object_t *)buf ) ==0 ) ){

			 memcpy((char *)((int)oem_class+id*64), 
					 ((store_object_t *)buf)->name, 
					 64);
			 dprintk("Fill OEM name %s to oem_map %d:%s\n",((store_object_t *)buf)->name,id,
					 (char *)((int)oem_class+id*64));
			 id ++; 
		}
	}
	kfree(buf);
	
	return  (size = id);
}

int sunxi_secure_storage_exit(void)
{
	int ret;

	if(!secure_storage_inited)
	{
		dprintk("%s err: secure storage has not been inited\n", __func__);

		return -1;
	}
	ret = sunxi_secstorage_write(0, secure_storage_map, SEC_BLK_SIZE);
	if(ret)
	{
		dprintk("write secure storage map\n");

		return -1;
	}

	return 0;
}

int sunxi_secure_storage_probe(const char *item_name)
{
	int ret;
	int len;

	if(!secure_storage_inited)
	{
		dprintk("%s err: secure storage has not been inited\n", __func__);

		return -1;
	}
	ret = __probe_name_in_map(secure_storage_map, item_name, &len);
	if(ret < 0)
	{
		dprintk("no item name %s in the map\n", item_name);

		return -1;
	}

	return ret;
}

int sunxi_secure_storage_read(const char *item_name, char *buffer, int buffer_len, int *data_len)
{
	int ret, index;
	int len_in_store;
	unsigned char * buffer_to_sec ;

	dprintk("secure storage read %s \n", item_name);
	if(!secure_storage_inited)
	{
		dprintk("%s err: secure storage has not been inited\n", __func__);
		return -1;
	}
	
	buffer_to_sec = (unsigned char *)kmalloc(SEC_BLK_SIZE,GFP_KERNEL);
	if(!buffer_to_sec ){
		dprintk("%s out of memory",__func__);
		return -1 ;
	}

	index = __probe_name_in_map(secure_storage_map, item_name, &len_in_store);
	if(index < 0)
	{
		dprintk("no item name %s in the map\n", item_name);
		kfree(buffer_to_sec);
		return -1;
	}
	sst_memset(buffer, 0, buffer_len);
	ret = sunxi_secstorage_read(index, buffer_to_sec, SEC_BLK_SIZE);
	if(ret)
	{
		dprintk("read secure storage block %d name %s err\n", index, item_name);
		kfree(buffer_to_sec);
		return -1;
	}
	if(len_in_store > buffer_len)
	{
		sst_memcpy(buffer, buffer_to_sec, buffer_len);
	}
	else
	{
		sst_memcpy(buffer, buffer_to_sec, len_in_store);
	}
	*data_len = len_in_store;

	dprintk("secure storage read %s done\n",item_name);

	kfree(buffer_to_sec);
	return 0;
}

/*Add new item to secure storage*/
int sunxi_secure_storage_write(const char *item_name, char *buffer, int length)
{
	int ret, index;

	if(!secure_storage_inited)
	{
		dprintk("%s err: secure storage has not been inited\n", __func__);

		return -1;
	}
	index = __fill_name_in_map(secure_storage_map, item_name, length);
	if(index < 0)
	{
		dprintk("write secure storage block %d name %s overrage\n", index, item_name);

		return -1;
	}

	ret = sunxi_secstorage_write(index, (unsigned char *)buffer, SEC_BLK_SIZE);
	if(ret)
	{
		dprintk("write secure storage block %d name %s err\n", index, item_name);

		return -1;
	}
	dprintk("write secure storage: %d ok\n", index);

	return 0;
}

/*load source data to secure_object struct
 **
 *    src		: secure_object 
 *    len		: secure_object buffer len 
 *    payload	: taregt payload 
 *    retLen	: target payload actual length
 **/
static int unwrap_secure_object(void * src,  unsigned int len, void * payload,   int *retLen )
{
	store_object_t *obj;

	if(len != sizeof(store_object_t)){
		pr_err("Input length not equal secure object size 0x%x\n",len);
		return -1 ;
	}

	obj = (store_object_t *) src ;

	if( obj->magic != STORE_OBJECT_MAGIC ){
		pr_err("Input object magic fail [0x%x]\n", obj->magic);
		return -1 ;
	}

	if( obj->re_encrypt == STORE_REENCRYPT_MAGIC){
		pr_err("secure object is encrypt by chip\n");
	}

	if( obj->crc != ~crc32_le(~0 , (void *)obj, sizeof(*obj)-4 ) ){
		pr_err("Input object crc fail [0x%x]\n", obj->crc);
		return -1 ;
	}

	memcpy(payload, obj->data ,obj->actual_len);
	*retLen = obj->actual_len ;

	return 0 ;
}

int sunxi_secure_object_read(const char *item_name, char *buffer, int buffer_len, int *data_len)
{
	char *secure_object;
	int retLen ,ret ;
	store_object_t *so ;

	secure_object=kzalloc(4096,GFP_KERNEL);
	if(!secure_object){
		pr_err("sunxi secure storage out of memory\n");	
		return -1 ;
	}

	ret = sunxi_secure_storage_read(item_name, secure_object, 4096, &retLen);
	if(ret){
		pr_err("sunxi storage read fail\n");
		kfree(secure_object);
		return -1 ;
	}
	so = (store_object_t *)secure_object;

	ret = unwrap_secure_object((char *)so, retLen, buffer, data_len);
	if(ret){
		pr_err("unwrap secure object fail\n");
		kfree(secure_object);
		return -1 ;
	}
	kfree(secure_object);
	return 0 ;
}
static int _cache_secure_hdcp(void)
{
	int ret ;
	extern struct sunxi_sst_hdcp *sunxi_hdcp;

	sunxi_hdcp =(struct sunxi_sst_hdcp*)kzalloc(sizeof(*sunxi_hdcp), GFP_KERNEL);
	if(!sunxi_hdcp){
		pr_err("%s: out of memroy\n",__func__);
		return -1 ;
	}
	ret =sunxi_secure_object_read("hdcpkey", sunxi_hdcp->key, SZ_4K, &(sunxi_hdcp->act_len));
	if(ret){
		pr_err("%s: can't read hdcpkey in keystore\n",__func__);
		kfree(sunxi_hdcp);
		sunxi_hdcp = NULL ;
		return -1 ;
	}
	pr_info("Cached encrypted hdcpkey\n");

	return 0 ;
}

int sunxi_secure_storage_init(void)
{
	int ret;

	if(!secure_storage_inited)
	{
		get_flash_type();
		ret = sunxi_secstorage_read(0, secure_storage_map, SEC_BLK_SIZE);
		if(ret < 0)
		{
			dprintk("get secure storage map err\n");

			return -1;
		}
		else if(ret > 0)
		{
			dprintk("the secure storage map is empty\n");
			sst_memset(secure_storage_map, 0, SEC_BLK_SIZE);
		}
	}
	
	secure_storage_inited = 1;
	_cache_secure_hdcp();

	return 0;
}
#ifdef OEM_STORE_IN_FS 
/*Store source data to secure_object struct
 *
 * src		: payload data to secure_object
 * name		: input payloader data name
 * tagt		: taregt secure_object
 * len		: input payload data length
 * retLen	: target secure_object length
 * */
int wrap_secure_object(void * src, char *name,  unsigned int len, void * tagt,  unsigned int *retLen )
{
	store_object_t *obj;

	if(len >MAX_STORE_LEN){
		dprintk("Input length larger then secure object payload size\n");
		return -1 ;
	}

	obj = (store_object_t *) tagt ;
	*retLen= sizeof( store_object_t );

	obj->magic = STORE_OBJECT_MAGIC ;
	strncpy( obj->name, name, 64 );
	obj->re_encrypt = 0 ;
	obj->version = 0;
	obj->id = 0;
	sst_memset(obj->reserved, 0, sizeof(obj->reserved) );
	obj->actual_len = len ;
	sst_memcpy( obj->data, src, len);
	
	obj->crc = ~crc32_le(~0 , (const unsigned char *)obj, sizeof(*obj)-4 );

	return 0 ;
}


/*
 * Test data and function
 */ 
static unsigned char hdmi_data[]={
	0x44,0xe5,0xb5,0xac,0x2b,0x53,0xbc,0xb9,0xbf,0x89,0x67,0x96,0x1e,0xbb,0xbd,0xfb
};

static unsigned char widevine_data[] ={
	0xeb,0x8f,0x55,0x26,0x0d,0x7a,0xab,0xf3,0x58,0x3b,0xf9,0xc0,0x5e,0x12,0x79,0x85
};

static unsigned char sec_buf[SEC_BLK_SIZE];
static int secure_object_op_test(void)
{
	unsigned char * tagt ;
	unsigned int LEN = SEC_BLK_SIZE , retLen ;
	int ret ;
	sunxi_secure_storage_init();
	tagt = (unsigned char *)kzalloc(LEN,GFP_KERNEL);
	if(!tagt){
		dprintk("out of memory\n");
		return -1 ;
	}

	sst_memset(tagt,0, LEN);
	ret = wrap_secure_object( hdmi_data, "HDMI",  
							sizeof(hdmi_data), tagt, &retLen ) ;
	if( ret <0){
		dprintk("Error: wrap secure object fail\n");
		kfree(tagt);
		return -1 ;
	}

	ret = sunxi_secure_storage_write( "HDMI" , tagt, retLen )	;
	if(ret <0){
		dprintk("Error: store HDMI object fail\n");
		kfree(tagt);
		return -1 ;
	}


	ret = sunxi_secure_storage_read( "HDMI" , sec_buf, SEC_BLK_SIZE , &retLen )	;
	if(ret <0){
		dprintk("Error: store HDMI object read fail\n");
		kfree(tagt);
		return -1 ;
	}
	
	if( memcmp(tagt, sec_buf, retLen ) !=0 ){
		dprintk("Error: HDMI write/read fail\n");
		return -1 ;
	}
	

	sst_memset(tagt,0, LEN);
	ret = wrap_secure_object(widevine_data, "Widevine",  
							sizeof(widevine_data), tagt, &retLen ) ;
	if( ret <0){
		dprintk("Error: wrap secure object fail\n");
		kfree(tagt);
		return -1 ;
	}

	ret = sunxi_secure_storage_write( "Widevine" , tagt, retLen )	;
	if(ret <0){
		dprintk("Error: store Widevine object fail\n");
		kfree(tagt);
		return -1 ;
	}

	ret = sunxi_secure_storage_read( "Widevine" , sec_buf, SEC_BLK_SIZE , &retLen )	;
	if(ret <0){
		dprintk("Error: store Widevine object read fail\n");
		kfree(tagt);
		return -1 ;
	}
	
	if( memcmp(tagt, sec_buf, retLen ) !=0 ){
		dprintk("Error: Widevine write/read fail\n");
		kfree(tagt);
		return -1 ;
	}

	sunxi_secure_storage_exit();
		
	kfree(tagt);
	return 0 ;
}
static unsigned char buffer[SEC_BLK_SIZE];
static int clear_secure_store(int index)
{
	sst_memset( buffer, 0, SEC_BLK_SIZE);

	if(index == 0xffff){
		int i =0 ;
		dprintk("clean whole secure store");
		for( ; i<32 ;i++){
			dprintk("..");
			sunxi_secstorage_write(i, (unsigned char *)buffer, SEC_BLK_SIZE);
			dprintk("clearn %d done\n",i);
		}

	}else{
		dprintk("clean secure store %d\n", index);
		dprintk("..");
		sunxi_secstorage_write(index, (unsigned char *)buffer, SEC_BLK_SIZE);
		dprintk("clearn %d done\n",index);
	}
	return 0 ;
}

int secure_store_in_fs_test(void)
{
	clear_secure_store(0);
	clear_secure_store(1);
	clear_secure_store(2);
	secure_object_op_test();
	return 0 ;
}

#endif
