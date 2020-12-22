#ifndef _LINUX_FIVM_H
#define _LINUX_FIVM_H

#include <linux/fs.h>
#define CONFIG_FILE_INTEGRITY
struct fivm_operation{
	int ( *mmap_verify)(struct file * ,unsigned long);
	int ( *open_verify)(struct file *,const char *,int);
};

extern int fivm_mmap_verify(struct file *file, unsigned long prot);
extern int fivm_open_verify(struct file *file, const char *,int mask);
#if 0
static inline int fivm_open_verify(struct file *file,const char *pathname, int mask)
{
	return 0;
}

static inline int fivm_mmap_verify(struct file *file, unsigned long prot)
{
	return 0;
}

#endif /* CONFIG_FILE_INTEGRITY */

#endif /*_LINUX_FIVM_H*/
