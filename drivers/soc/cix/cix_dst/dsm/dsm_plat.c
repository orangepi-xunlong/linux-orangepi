#include "dsm_core.h"
#include "linux/fs.h"
#include "linux/stddef.h"
#include "linux/types.h"
#include <linux/soc/cix/util.h>
#include <linux/namei.h>

#define DSM_MAXPATH 256
#define DSM_PATH "/var/log/cix/dsm/"
#define DSM_MAX_CACHE_NUM 5
#define DSM_REPORT_TIME_INTERVAL 5

typedef struct dsm_report_cache {
	time64_t time;
	char client_name[CLIENT_NAME_LEN];
	size_t used_size;
	char *dump_buff;
} dsm_report_cache;

static dsm_report_cache dsm_cache[DSM_MAX_CACHE_NUM] = { 0 };
static size_t dsm_cache_index = 0;

static bool dsm_dump_check(const struct dsm_client *client)
{
	int i = 0;

	if (client->used_size == 0)
		return false;

	for (i = 0; i < DSM_MAX_CACHE_NUM; i++) {
		if (!dsm_cache[i].dump_buff)
			continue;
		 if (time_after((ulong)(ktime_get_real_seconds() -
                       DSM_REPORT_TIME_INTERVAL),
                   (ulong)dsm_cache[i].time))
			continue;
		if (dsm_cache[i].used_size != client->used_size)
			continue;
		if (memcmp(dsm_cache[i].dump_buff, client->dump_buff,
			   client->used_size) != 0)
			continue;
		if (memcmp(dsm_cache[i].client_name, client->client_name,
			   CLIENT_NAME_LEN) != 0)
			continue;
		return false;
	}

	return true;
}

static int dsm_cache_put(const struct dsm_client *client)
{
	time64_t now = ktime_get_real_seconds();

	if (dsm_cache[dsm_cache_index].dump_buff) {
		kfree(dsm_cache[dsm_cache_index].dump_buff);
		dsm_cache[dsm_cache_index].dump_buff = NULL;
		dsm_cache[dsm_cache_index].used_size = 0;
		dsm_cache[dsm_cache_index].time = 0;
		memset(dsm_cache[dsm_cache_index].client_name, 0,
		       CLIENT_NAME_LEN);
	}

	dsm_cache[dsm_cache_index].dump_buff =
		kmemdup(client->dump_buff, client->used_size, GFP_KERNEL);
	if (!dsm_cache[dsm_cache_index].dump_buff)
		return -ENOMEM;

	memcpy(dsm_cache[dsm_cache_index].client_name, client->client_name,
		CLIENT_NAME_LEN);
	dsm_cache[dsm_cache_index].used_size = client->used_size;
	dsm_cache[dsm_cache_index].time = now;
	dsm_cache_index = (dsm_cache_index + 1) % DSM_MAX_CACHE_NUM;

	return 0;
}

int dsm_report(const struct dsm_client *client)
{
	char path[DSM_MAXPATH] = { 0 };
	struct kstat m_stat;
	struct tm tm;
	int err;
	time64_t now = ktime_get_real_seconds();

	time64_to_tm(now, 0, &tm);

	snprintf(path, DSM_MAXPATH, "%d-%04ld-%02d-%02d-%02d-%02d-%02d-%s",
		 client->error_no, tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour,
		 tm.tm_min, tm.tm_sec, client->client_name);

	if (rdr_vfs_stat(DSM_PATH, &m_stat) != 0) {
		err = rdr_create_dir(DSM_PATH);
		if (err) {
			dsm_log_err("dsm: create dir %s fail!!!\n", DSM_PATH);
			return err;
		}
	}

	if (!dsm_dump_check(client)) {
		dsm_log_info("dsm: %s is same as previous dump\n",
			client->client_name);
		return -EINVAL;
	}

	rdr_savebuf2fs(DSM_PATH, path, client->dump_buff, client->used_size, 0);
	dsm_cache_put(client);

	return 0;
}