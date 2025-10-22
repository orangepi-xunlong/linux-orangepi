#ifndef __XRMAC_DEBUGFS_H
#define __XRMAC_DEBUGFS_H

#ifdef CONFIG_XRMAC_DEBUGFS
extern void xrmac_debugfs_hw_add(struct ieee80211_local *local);
extern int mac80211_open_file_generic(struct inode *inode, struct file *file);
extern int xrmac_format_buffer(char __user *userbuf, size_t count,
				  loff_t *ppos, char *fmt, ...);
#else
static inline void xrmac_debugfs_hw_add(struct ieee80211_local *local)
{
}
#endif

#endif /* __XRMAC_DEBUGFS_H */
