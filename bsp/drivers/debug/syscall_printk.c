// SPDX-License-Identifier: GPL-2.0-only
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
#include <linux/minmax.h>
#include <linux/syscalls.h>
#include <linux/uaccess.h>
#include <asm/unistd.h>

#define SYSCALL_PRINTK_BUF_LEN	1024

static char syscall_printk_buf[SYSCALL_PRINTK_BUF_LEN];

SYSCALL_DEFINE2(printk, const char __user *, buf, size_t, count)
{
	int err;
	size_t len = min(count, (size_t)(SYSCALL_PRINTK_BUF_LEN - 1));

	pr_debug("%s(): Begin\n", __func__);

	err = copy_from_user(syscall_printk_buf, buf, len);
	if (err) {
		printk("%s(): copy_from_user failed with %d\n", __func__, err);
		return err;
	}

	syscall_printk_buf[len] = '\0';
	pr_emerg(syscall_printk_buf);
	/**
	 * printk() will eat the trailing '\n' if there is.
	 * which means,
	 * - printk("Hello\n");
	 * and
	 * - printk("Hello");
	 * print out the same thing.
	 * And the return value of the above two printks are the same: ret = 5
	 * Hince, we don't use the return value of printk as our return value.
	 */

	pr_debug("%s(): End\n", __func__);
	return (int)len;
}

/**
 * Usage: Compile the following C app and run it.
 */
#if SYS_PRINTK_DEMO
#include <stdio.h>  /* printf */
#include <unistd.h>
#include <sys/syscall.h>

#define SYS_printk      449

static int print(const char *s)
{
	size_t len = 0;

	while (s[len] != '\0')
		len++;

	return syscall(SYS_printk, s, len);
}

int main(int argc, char **argv)
{
	int ret;

	ret = print("hello printk\n");

	printf("Exit with ret = %d\n", ret);  /* ret = 13 */
	return ret;
}
#endif  /* SYS_PRINTK_DEMO */
