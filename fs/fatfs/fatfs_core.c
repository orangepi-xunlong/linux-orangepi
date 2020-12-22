#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/sched.h>
#include <linux/mutex.h>

#include <linux/timer.h>
#include <linux/timex.h>
#include <linux/rtc.h>
#include "integer.h"
#include "ff.h"
#include "diskio.h"

#define MAX_LIST_SIZE 4
#define MAX_BUFFER_SIZE (64*1024)
typedef struct
{
	pid_t pid;
	FIL   file;
	int   fd;
	char  path[128];                // check file whether it is opened again
	char  buf[MAX_BUFFER_SIZE];
}FILE_LIST;

typedef struct
{
	pid_t pid;
	DIR   dir;
}DIR_LIST;

static FATFS g_fs[MAX_DRIVES];
static FILE_LIST g_FileList[MAX_LIST_SIZE];
static DIR_LIST  g_DIRList[MAX_LIST_SIZE];

static int g_async_status = 0;
static int g_async_mount = 0;

DWORD get_fattime (void)
{
	struct timex  txc;
	struct rtc_time tm;
	do_gettimeofday(&(txc.time));
	rtc_time_to_tm(txc.time.tv_sec,&tm);
//	printk("UTC time :%d-%d-%d %d:%d:%d \n",  tm.tm_year+1900,tm.tm_mon, tm.tm_mday,tm.tm_hour,tm.tm_min,tm.tm_sec);

	return 	  ((DWORD)(tm.tm_year - 80) << 25)
		| ((DWORD)(tm.tm_mon + 1) << 21)
		| ((DWORD)tm.tm_mday << 16)
		| (DWORD)(tm.tm_hour << 11)
		| (DWORD)(tm.tm_min << 5)
		| (DWORD)(tm.tm_sec >> 1);
}

int getFIL(void)
{
	int i = 0;
	int res = -1;

	for(i=0; i<MAX_LIST_SIZE; i++)
	{
		if(0 == g_FileList[i].pid)
		{
			res = i;
			break;
		}
	}
	return res;
}

int closFIL(int fd)
{
	if(fd > MAX_LIST_SIZE)
	{
		printk("closFIL fd[%d] > MAX_LIST_SIZE \n", fd);
		return -1;
	}
	f_close(&g_FileList[fd].file);

	memset(&g_FileList[fd], 0, sizeof(FILE_LIST));
	g_FileList[fd].pid = 0;
	return 0;
}

int getDIR(void)
{
	int i = 0;
	int res = -1;

	for(i=0; i<MAX_LIST_SIZE; i++)
	{
		if(0 == g_DIRList[i].pid)
		{
			res = i;
			break;
		}
	}
	return res;
}

int closDIR(int fd)
{
	if(fd > MAX_LIST_SIZE)
	{
		printk("closDIR fd[%d] > MAX_LIST_SIZE \n", fd);
		return -1;
	}

	f_closedir(&g_DIRList[fd].dir);
	memset(&g_DIRList[fd], 0, sizeof(DIR_LIST));
	g_DIRList[fd].pid = 0;
	return 0;
}

static DEFINE_MUTEX(fatfs_lock);

static long fatfs_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
//lock
	int ret = 0;

	mutex_lock(&fatfs_lock);

	switch(cmd)
	{
		case ioctl_f_open:
			{
				FRESULT fr;
				FAT_OPEN_ARGS open_args;
				int i = 0;
				int fileOpened = 0;
				int Fil_fd;

				if(copy_from_user(&open_args, (int __user *)arg, sizeof(FAT_OPEN_ARGS)) ) {
					ret =  -EFAULT;
					break;
				}

				for (; i < MAX_LIST_SIZE; i++) {
					if ((g_FileList[i].pid != 0) && (strcmp(g_FileList[i].path, open_args.path) == 0)) {    // file has been opened before!
						ret = g_FileList[i].fd;
						fileOpened = 1;
						break;
					}
				}
				if (fileOpened == 1)
					break;

				Fil_fd  = getFIL();
				if(Fil_fd < 0) {
					printk("(Fil_fd < 0) \n");
					ret = -1;
					break;
				}

				fr = f_open(&g_FileList[Fil_fd].file, open_args.path, open_args.mode);
				if (fr) {
					printk("ioctl_f_open failed fr [%d]\n", fr);
					ret = -1;
				} else {
					struct task_struct * process = current;
					g_FileList[Fil_fd].pid = process->pid;
					strcpy(g_FileList[Fil_fd].path, open_args.path);
					ret = Fil_fd;
//					printk("ioctl_f_open ok Fil_fd = %d \n", Fil_fd);
				}
			}
			break;

		case ioctl_f_close:
			{
				int fd = (int)arg;
				closFIL(fd);
			}
			break;
		case ioctl_f_write:
			{
				FAT_BUFFER fat_buffer;
				int fd = 0;
				int wlen = 0;
				FRESULT fr;

				if(copy_from_user(&fat_buffer, (int __user *)arg, sizeof(FAT_BUFFER)) )
				{
					ret =  -EFAULT;
					break;
				}

				fd = fat_buffer.fd;

				if(copy_from_user(g_FileList[fd].buf, (int __user *)(arg+sizeof(FAT_BUFFER) ), fat_buffer.len))
				{
					ret =  -EFAULT;
					break;
				}

				if(fat_buffer.len > MAX_BUFFER_SIZE)
					fat_buffer.len = MAX_BUFFER_SIZE;

				fr = f_write(&g_FileList[fd].file, g_FileList[fd].buf, fat_buffer.len, &wlen);
				if(fr != 0) {
					printk(" fd[%d] f_write error fr = %d \n", fd, fr);
					ret = -fr;
					break;
				}
				ret = wlen;
			}
			break;
		case ioctl_f_read:
			{
				FAT_BUFFER fat_buffer;
				int fd = 0;
				int rlen = 0;
				FRESULT fr;

				memset(&fat_buffer, 0, sizeof(FAT_BUFFER ));
				if(copy_from_user(&fat_buffer, (int __user *)arg, sizeof(FAT_BUFFER)) )
				{
					ret =  -EFAULT;
					break;
				}

				fd = fat_buffer.fd;

				if(fat_buffer.len > MAX_BUFFER_SIZE)
					fat_buffer.len = MAX_BUFFER_SIZE;

				fr = f_read(&g_FileList[fd].file, g_FileList[fd].buf, fat_buffer.len, &rlen);
				if(fr != 0) {
					printk(" fd[%d] f_read error fr = %d \n", fd, fr);
					ret = -fr;
					break;
				}

				if(copy_to_user((int __user *)arg, g_FileList[fd].buf, rlen) )
				{
					ret =  -EFAULT;
					break;
				}
				ret = rlen;
			}
			break;
		case ioctl_f_lseek:
			{
				FAT_SEEK fat_seek;
				int fd = 0;

				if(copy_from_user(&fat_seek, (int __user *)arg, sizeof(FAT_SEEK)) )
				{
					ret =  -EFAULT;
					break;
				}

				fd = fat_seek.fd;
				ret = f_lseek(&g_FileList[fd].file, fat_seek.ofs);
			}
			break;
		case ioctl_f_truncate:
			{
				int fd = (int)arg;
				ret = f_truncate(&g_FileList[fd].file);
			}
			break;
		case ioctl_f_sync:
			{
				int fd = (int)arg;
				if (fd == MAX_LIST_SIZE){
					int i = 0;
					for (; i < MAX_LIST_SIZE; i++) {
						if (g_FileList[i].pid != 0) {
							ret = f_sync(&g_FileList[i].file);
						}
						if (ret != 0) {
							break;
						}
					}
				} else {
					ret = f_sync(&g_FileList[fd].file);
				}
			}
			break;
		case ioctl_f_tell:
			{
				FAT_FILE_LEN fat_pos;
				
				if (copy_from_user(&fat_pos, (int __user *)arg, sizeof(FAT_FILE_LEN))) {
					ret = -EFAULT;
					break;
				}

				fat_pos.len = (long long)f_tell(&g_FileList[fat_pos.fd].file);
				if (copy_to_user((int __user*)arg, &fat_pos, sizeof(FAT_FILE_LEN))) {
					ret = -EFAULT;
					break;
				}
			}
			break;
		case ioctl_f_size:
			{
				FAT_FILE_LEN fat_size;

				if (copy_from_user(&fat_size, (int __user*)arg, sizeof(FAT_FILE_LEN))) {
					ret = -EFAULT;
					break;
				}

				fat_size.len = (long long)f_size(&g_FileList[fat_size.fd].file);
				if (copy_to_user((int __user*)arg, &fat_size, sizeof(FAT_FILE_LEN))) {
					ret = -EFAULT;
					break;
				}
			}
			break;
		case ioctl_f_opendir:
			{
				int DIR_fd  = getDIR();
				char bufdir[128];
				FRESULT fr;

				if(DIR_fd < 0)
				{
					printk("(ioctl_f_opendir : DIR_fd < 0) \n");
					ret = -1;
					break;
				}

				if(copy_from_user(bufdir, (int __user *)arg, sizeof(bufdir)))
				{
					ret =  -EFAULT;
					break;
				}

				fr = f_opendir(&g_DIRList[DIR_fd].dir, bufdir);

				if(fr)
				{
					printk("ioctl_f_opendir  failed fr [%d]\n", fr);
					ret = -1;
				}
				else
				{
					struct task_struct * process = current;
					g_DIRList[DIR_fd].pid = process->pid;
					printk("(ioctl_f_opendir ok : DIR_fd [%d], pid [%d]) \n",DIR_fd, process->pid );
					ret = DIR_fd;
				}

			}
			break;
		case ioctl_f_closedir:
			{
				int fd = (int)arg;
				closDIR(fd);
			}
			break;
		case ioctl_f_readdir:
			{
				FAT_FILEINFO_FD file_info;
				int fd = 0;
				int rlen = 0;

				if(copy_from_user(&file_info, (int __user *)arg, sizeof(FAT_FILEINFO_FD)) )
				{
					ret =  -EFAULT;
					break;
				}

				fd = file_info.fd;

				memset(&file_info.fileinfo, 0, sizeof(FILINFO));
				file_info.fileinfo.lfsize = 128;

				rlen = f_readdir(&g_DIRList[fd].dir, &file_info.fileinfo);

				if(copy_to_user((int __user *)arg, &file_info, sizeof(FAT_FILEINFO_FD)) )
				{
					ret =  -EFAULT;
					break;
				}

				ret = rlen;

			}
			break;

		case ioctl_f_mkdir:
			{
				char path[128] = {0};
				if(copy_from_user(path, (int __user *)arg, 128) )
				{
					ret =  -EFAULT;
					break;
				}
				ret = f_mkdir(path);
			}
			break;

		case ioctl_f_unlink:
			{
				char path[128] = {0};
				if(copy_from_user(path, (int __user *)arg, 128) )
				{
					ret =  -EFAULT;
					break;
   				}

				ret = f_unlink(path);
			}
			break;
		case ioctl_f_rename:
			{
				char two_path[256] = {0};
				if(copy_from_user(two_path, (int __user *)arg, 256) ) {
					ret =  -EFAULT;
					break;
				}

				ret = f_rename(two_path, two_path+128);
			}
			break;

		case ioctl_f_stat:
			{
				FAT_FILEINFO_FD file_info;
				int res = 0, i = 0;

				if(copy_from_user(&file_info, (int __user *)arg, sizeof(FAT_FILEINFO_FD))) {
					ret =  -EFAULT;
					break;
				}

//				memset(&file_info.fileinfo, 0, sizeof(FILINFO));
//				file_info.fileinfo.lfsize = 128;

				for (; i < MAX_LIST_SIZE; i++) {
//					if (g_FileList[i].pid != 0) {
					if (strcmp(g_FileList[i].path, file_info.path) == 0) {
						f_sync(&g_FileList[i].file);
						break;
					}
				}

				res = f_stat(file_info.path, &file_info.fileinfo);

				if(copy_to_user((int __user *)arg, &file_info, sizeof(FAT_FILEINFO_FD)) ) {
					ret =  -EFAULT;
					break;
				}

				ret = res;
			}
			break;
		case ioctl_f_chmod:
			{
				FAT_FILE_ATTR  file_attr;

				if(copy_from_user(&file_attr, (int __user *)arg, sizeof(FAT_FILE_ATTR)) ) {
					ret =  -EFAULT;
					break;
				}

				ret = f_chmod(file_attr.path, file_attr.attr, file_attr.mask);
			}
			break;
		case ioctl_f_chdir:
			{
				char path[128] = {0};
				if(copy_from_user(path, (int __user *)arg, 128) )
				{
					ret =  -EFAULT;
					break;
				}
				ret = f_chdir(path);
			}
			break;
		case ioctl_f_mkfs:
			{
				char path[128] = {0};
				if(copy_from_user(path, (int __user *)arg, 128) )
				{
					ret =  -EFAULT;
					break;
				}
				ret = f_mkfs(path, 0, 0);
			}
			break;
		case ioctl_f_getfree:									// return struct FAT_STATFS, include free sectors
			{
				int nclust;
				int res;
				FATFS *pfs;
				int totalSector;
				FAT_STATFS  FatStatFs;

				res = f_getfree("0:", (DWORD *)&nclust, &pfs);	// default dev: "0:"-only one disk, path NOT use
				if (res != 0) {
					printk("(f:%s, l:%d) fatal error! f_getfree fail\n", __FUNCTION__, __LINE__);
					ret = res;
				} else {
					FatStatFs.f_type = pfs->fs_type;			// 1-fat12; 2-fat16; 3-fat32
					FatStatFs.f_bsize = pfs->ssize;				// sector size
					FatStatFs.f_bfree = nclust * pfs->csize;	// free sector count
					disk_ioctl(0, GET_SECTOR_COUNT, &totalSector);
					FatStatFs.f_blocks = totalSector;			// total sectors in disk, other data NOT implement yet
					if(copy_to_user((char __user *)arg, &FatStatFs, sizeof(FAT_STATFS)) ) {
					        printk("(f:%s, l:%d) fatal error! ioctl_f_getfree copy to user fail\n", __FUNCTION__, __LINE__);
						ret =  -EFAULT;
						break;
					}
					ret = res;
				}
			}
			break;
		case ioctl_card_status:
			{
				int cardStatus;
				disk_ioctl(0, MMC_GET_SDSTAT, &cardStatus);
				ret = cardStatus;
			}
			break;
		case ioctl_card_fstype:
			{
				ret = g_fs[0].fs_type;				// 1-fat12; 2-fat16; 3-fat32; 4-other filesystem
			}
			break;
		case ioctl_card_remount:
			{
				int i = 0;
				memset(&g_fs[0], 0, sizeof(FATFS));
				ret = f_mount(&g_fs[0], "0:", 1);
				if (ret != 0) {
					g_fs[0].fs_type = 4;
				}
				for(; i<MAX_LIST_SIZE; i++) {
					memset(&g_FileList[i], 0, sizeof(FILE_LIST));
					memset(&g_DIRList[i], 0, sizeof(DIR_LIST));
				}
			}
			break;
		case ioctl_card_format:
			{
				if ((g_fs[0].csize == 128) && (g_fs[0].fs_type != 4))
					ret = 0;
				else
					ret = 1;
			}
			break;
		case ioctl_card_prepare:
			{
				if (g_async_status == 1) {
					if (g_async_mount == 1) {
						if (g_fs[0].csize == 128)
							ret = 0;	// everything is ok, card can be read/write
						else
							ret = 3;	// card NOT 64KB/cluster
					} else {
						ret = 2;		// card NOT fatfs(unmounted)
					}
				} else {
					ret = 1;			// card NOT exist
				}
			}
			break;
		default:
			printk("[fatfs.ko] unknow cmd [%d] \n", cmd );
	}

	mutex_unlock(&fatfs_lock);
	return ret;
}

void fatfs_async_renewstatus(int cardstatus)
{
	printk("[fatfs.ko](%s) detect card status changed, present status: [%d] \n", __func__, cardstatus);
	g_async_status = cardstatus;
	if (g_async_status == 0)
		g_async_mount = 0;
}

int fatfs_async_mount(void)
{
	int ret,i;

	if (g_async_status != 1)
		return -1;
	printk("[fatfs.ko](%s) card is inserted now! ready to mount! \n", __func__);
	mutex_lock(&fatfs_lock);
	memset(&g_fs[0], 0, sizeof(FATFS));
	ret = f_mount(&g_fs[0], "0:", 1);
	if (ret != 0) {
		printk("[fatfs.ko](%s) oops, can NOT mount!!! ret: [%d] \n", __func__, ret);
		g_fs[0].fs_type = 4;
		g_async_mount = 0;
		mutex_unlock(&fatfs_lock);
		return -1;
	}
	g_async_mount = 1;
	for(i=0; i<MAX_LIST_SIZE; i++) {
		memset(&g_FileList[i], 0, sizeof(FILE_LIST));
		memset(&g_DIRList[i], 0, sizeof(DIR_LIST));
	}
	mutex_unlock(&fatfs_lock);
	return 0;
}

static int opend_times = 0;
static int fatfs_open(struct inode *inode, struct file *filp)
{
	FRESULT res = 0;

	struct task_struct * process = current;

	printk ("[fatfs.ko] misc  in fatfs_open by pid [%d] f_mount \n",process->pid);
	mutex_lock(&fatfs_lock);

	if(opend_times == 0)
	{
		res = f_mount(&g_fs[0], "0:", 1);
		if (res != 0) {
			g_fs[0].fs_type = 4;
		}
		printk("[fatfs.ko] f_mount(&g_fs[0]) \n");
	}

	opend_times++;

	mutex_unlock(&fatfs_lock);
	printk ("[fatfs.ko] misc  out fatfs_open by pid [%d], f_mount res: [%d] \n",process->pid, res);

	return 0;
}


static int fatfs_release(struct inode *inode, struct file *filp)
{
	struct task_struct * process = current;
	int i = 0;
	opend_times--;

	printk ("misc  in fatfs_release by pid [%d]\n",process->pid );
	mutex_lock(&fatfs_lock);

	for(i=0; i<MAX_LIST_SIZE; i++)
	{
		if(g_FileList[i].pid == 0)
			continue;

		if(process->pid == g_FileList[i].pid)
		{
			closFIL(i);
		}
	}

	for(i=0; i<MAX_LIST_SIZE; i++)
	{
		if(g_DIRList[i].pid == 0)
			continue;

		if(process->pid == g_DIRList[i].pid)
		{
			closDIR(i);
		}
	}
	mutex_unlock(&fatfs_lock);

	return 0;
}

static ssize_t fatfs_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos)
{
	printk ("misc fatfs_read\n");
	return 1;
}

static struct file_operations fatfs_fops =
{
	.owner   = THIS_MODULE,
	.read    = fatfs_read,
	.unlocked_ioctl = fatfs_ioctl,
	.open    = fatfs_open,
	.release = fatfs_release
};

static struct miscdevice misc = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "misc_fatfs",
	.fops = &fatfs_fops,
};

extern void mmc_fatfs_sethandler(void * status, void * mount);
static int __init dev_init(void)
{
	int i = 0;
	int ret = misc_register(&misc);
	printk ("misc fatfs initialized\n");

	for(i=0; i<MAX_LIST_SIZE; i++)
	{
		memset(&g_FileList[i], 0, sizeof(FILE_LIST));
		memset(&g_DIRList[i], 0, sizeof(DIR_LIST));
	}

	mmc_fatfs_sethandler(fatfs_async_renewstatus, fatfs_async_mount);

	return ret;
}

static void __exit dev_exit(void)
{
	misc_deregister(&misc);
	printk("misc fatfs unloaded\n");
}

module_init(dev_init);
module_exit(dev_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("martin.zhu");
