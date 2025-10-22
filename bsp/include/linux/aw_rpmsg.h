/* SPDX-License-Identifier: GPL-2.0 */

#ifndef _LINUX_AW_RPMSG_H
#define _LINUX_AW_RPMSG_H

#ifdef CONFIG_AW_RPMSG_NOTIFY
typedef int (*notify_callback)(void *dev, void *data, int len);
int rpmsg_notify_add(const char *ser_name, const char *name, notify_callback cb, void *dev);
int rpmsg_notify_del(const char *ser_name, const char *name);
#endif

#ifdef CONFIG_AW_RPROC_FAST_BOOT
#define fast_rpmsg_driver(__rpmsg_driver) \
static int __init __rpmsg_driver##_init(void) \
{ \
	return register_rpmsg_driver(&(__rpmsg_driver)); \
} \
postcore_initcall(__rpmsg_driver##_init); \
static void __exit __rpmsg_driver##_exit(void) \
{ \
	unregister_rpmsg_driver(&(__rpmsg_driver)); \
} \
module_exit(__rpmsg_driver##_exit);
#endif

#endif /* _LINUX_AW_RPMSG_H */
