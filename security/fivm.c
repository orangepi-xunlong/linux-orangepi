/*
 *
 * Copyright (C) 2014 Allwinner Ltd. 
 *
 * Author:
 *	Ryan Chen <ryanchen@allwinnertech.com>
 *
 *	This program is free software; you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation; either version 2 of the License, or
 *	(at your option) any later version.
 */


#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/security.h>
#include <linux/fivm.h>

static struct fivm_operation fivm_ops ={NULL, NULL};
int fivm_mmap_verify(struct file *file, unsigned long prot)
{
	if(fivm_ops.mmap_verify)
		return fivm_ops.mmap_verify(file, prot) ;
	else
		return 0 ;
}
int fivm_open_verify(struct file *file, const char *pathname,int mask)
{
	if(fivm_ops.open_verify)
		return fivm_ops.open_verify(file,pathname,mask);
	else
		return 0 ;
}

int fivm_register_func( 
		int (* mmap_verify)(struct file *, unsigned long ),
		int (* open_verify)(struct file *,const char *, int)
		)
{
	memset(&fivm_ops, 0 , sizeof(fivm_ops));
	fivm_ops.mmap_verify = mmap_verify;
	fivm_ops.open_verify = open_verify ;
	return 0 ;
}
EXPORT_SYMBOL(fivm_register_func);

int fivm_unregister_func(void)
{
	memset(&fivm_ops, 0 , sizeof(fivm_ops));
	return 0 ;
}
EXPORT_SYMBOL(fivm_unregister_func);

