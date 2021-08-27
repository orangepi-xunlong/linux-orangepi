/*
 * Kernel based bootsplash.
 *
 * Authors:
 * Max Staudt <mstaudt@suse.de>
 *
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef __LINUX_BOOTSPLASH_H
#define __LINUX_BOOTSPLASH_H

#include <linux/fb.h>


#ifdef CONFIG_BOOTSPLASH

extern void bootsplash_render_full(struct fb_info *info);

extern bool bootsplash_would_render_now(void);

extern void bootsplash_mark_dirty(void);

extern bool bootsplash_is_enabled(void);
extern void bootsplash_disable(void);
extern void bootsplash_enable(void);

extern void bootsplash_init(void);

#else /* CONFIG_BOOTSPLASH */

#define bootsplash_render_full(x)

#define bootsplash_would_render_now() (false)

#define bootsplash_mark_dirty()

#define bootsplash_is_enabled() (false)
#define bootsplash_disable()
#define bootsplash_enable()

#define bootsplash_init()

#endif /* CONFIG_BOOTSPLASH */


#endif
