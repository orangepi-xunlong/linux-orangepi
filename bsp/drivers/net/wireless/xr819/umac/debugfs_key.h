#ifndef __XRMAC_DEBUGFS_KEY_H
#define __XRMAC_DEBUGFS_KEY_H

#ifdef CONFIG_XRMAC_DEBUGFS
void xrmac80211_debugfs_key_add(struct ieee80211_key *key);
void mac80211_debugfs_key_remove(struct ieee80211_key *key);
void mac80211_debugfs_key_update_default(struct ieee80211_sub_if_data *sdata);
void mac80211_debugfs_key_add_mgmt_default(
	struct ieee80211_sub_if_data *sdata);
void mac80211_debugfs_key_remove_mgmt_default(
	struct ieee80211_sub_if_data *sdata);
void mac80211_debugfs_key_sta_del(struct ieee80211_key *key,
				   struct sta_info *sta);
#else
static inline void xrmac80211_debugfs_key_add(struct ieee80211_key *key)
{}
static inline void mac80211_debugfs_key_remove(struct ieee80211_key *key)
{}
static inline void mac80211_debugfs_key_update_default(
	struct ieee80211_sub_if_data *sdata)
{}
static inline void mac80211_debugfs_key_add_mgmt_default(
	struct ieee80211_sub_if_data *sdata)
{}
static inline void mac80211_debugfs_key_remove_mgmt_default(
	struct ieee80211_sub_if_data *sdata)
{}
static inline void mac80211_debugfs_key_sta_del(struct ieee80211_key *key,
						 struct sta_info *sta)
{}
#endif

#endif /* __XRMAC_DEBUGFS_KEY_H */
