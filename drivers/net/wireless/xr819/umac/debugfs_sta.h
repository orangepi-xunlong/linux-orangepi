#ifndef __XRMAC_DEBUGFS_STA_H
#define __XRMAC_DEBUGFS_STA_H

#include "sta_info.h"

#ifdef CONFIG_XRMAC_DEBUGFS
void mac80211_sta_debugfs_add(struct sta_info *sta);
void mac80211_sta_debugfs_remove(struct sta_info *sta);
#else
static inline void mac80211_sta_debugfs_add(struct sta_info *sta) {}
static inline void mac80211_sta_debugfs_remove(struct sta_info *sta) {}
#endif

#endif /* __XRMAC_DEBUGFS_STA_H */
