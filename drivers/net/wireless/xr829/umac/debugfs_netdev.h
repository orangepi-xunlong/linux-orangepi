/* routines exported for debugfs handling */

#ifndef __IEEE80211_DEBUGFS_NETDEV_H
#define __IEEE80211_DEBUGFS_NETDEV_H

#ifdef CONFIG_XRMAC_DEBUGFS
void mac80211_debugfs_add_netdev(struct ieee80211_sub_if_data *sdata);
void mac80211_debugfs_remove_netdev(struct ieee80211_sub_if_data *sdata);
void mac80211_debugfs_rename_netdev(struct ieee80211_sub_if_data *sdata);
#else
static inline void mac80211_debugfs_add_netdev(
	struct ieee80211_sub_if_data *sdata)
{}
static inline void mac80211_debugfs_remove_netdev(
	struct ieee80211_sub_if_data *sdata)
{}
static inline void mac80211_debugfs_rename_netdev(
	struct ieee80211_sub_if_data *sdata)
{}
#endif

#endif /* __IEEE80211_DEBUGFS_NETDEV_H */
