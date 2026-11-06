#include <linux/types.h>
#include <linux/version.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>

#include <linux/common.h>
#include <linux/base_uapi.h>
#include "base_debug.h"

#define LOG_PROC_NAME			"log"
#define LOG_PROC_PERMS			(0644)
#define MAX_PROC_STR_SIZE		(32)
#define DEFAULT_LOG_LEVEL		(4)
#define GENERATE_STRING(STRING)	(#STRING),

static void *shared_mem;
static const char *const MOD_STRING[] = FOREACH_MOD(GENERATE_STRING);
/*************************************************************************
 *	Log proc functions
 *************************************************************************/
static void _show_log_status(struct seq_file *m)
{
	int i;
	int32_t *log_level;

	log_level = (int32_t *)(shared_mem + BASE_LOG_LEVEL_OFFSET);

	seq_puts(m, "-----CURRENT LOG LEVEL-------------------------------------------------------------------------------------------------------------\n");
	for (i = 0; i < ID_BUTT; ++i) {
		seq_printf(m, "%-10s(%2d)\n", MOD_STRING[i], log_level[i]);
	}
	seq_puts(m, "\n-----------------------------------------------------------------------------------------------------------------------------------\n");
}

static int _log_proc_show(struct seq_file *m, void *v)
{
	_show_log_status(m);

	return 0;
}

static void _log_update_dbg_level(char *pcProcInputdata)
{
	int32_t *log_level;
	int32_t tmpLogLevel;
	int i;
	char cModPrefix[MAX_PROC_STR_SIZE];

	log_level = (int32_t *)(shared_mem + BASE_LOG_LEVEL_OFFSET);

	// check if set to all module
	memset(cModPrefix, 0, sizeof(cModPrefix));
	snprintf(cModPrefix, sizeof(cModPrefix), "%s=", "ALL");
	if (strncmp(pcProcInputdata, cModPrefix, strlen(cModPrefix)) == 0) {
		if (kstrtouint(pcProcInputdata + strlen(cModPrefix), 10, &tmpLogLevel) != 0) {
			TRACE_BASE(DBG_ERR, "fail to set Mod(%s) debug level to %s\n",
				"ALL", pcProcInputdata + strlen(cModPrefix));
		} else {
			for (i = 0; i < ID_BUTT; ++i)
				log_level[i] = tmpLogLevel;
		}
		return;
	}

	// check if set to specific module
	for (i = 0; i < ID_BUTT; ++i) {
		memset(cModPrefix, 0, sizeof(cModPrefix));
		snprintf(cModPrefix, sizeof(cModPrefix), "%s=", MOD_STRING[i]);
		if (strncmp(pcProcInputdata, cModPrefix, strlen(cModPrefix)) == 0) {
			if (kstrtouint(pcProcInputdata + strlen(cModPrefix), 10, &log_level[i]) != 0) {
				TRACE_BASE(DBG_ERR, "fail to set Mod(%s) debug level to %s\n",
					MOD_STRING[i], pcProcInputdata + strlen(cModPrefix));
			}
			break;
		}
	}

	if (i == ID_BUTT)
		TRACE_BASE(DBG_ERR, "unknown module, user input:%s\n", pcProcInputdata);
}

static ssize_t _log_proc_write(struct file *file, const char __user *user_buf, size_t count, loff_t *ppos)
{
	char cProcInputdata[MAX_PROC_STR_SIZE] = {'\0'};

	if (user_buf == NULL || count >= MAX_PROC_STR_SIZE) {
		TRACE_BASE(DBG_ERR, "Invalid input value\n");
		return -EINVAL;
	}

	if (copy_from_user(cProcInputdata, user_buf, count)) {
		TRACE_BASE(DBG_ERR, "copy_from_user fail\n");
		return -EFAULT;
	}

	_log_update_dbg_level(cProcInputdata);
	return count;
}

static int _log_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, _log_proc_show, NULL);
}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
static const struct proc_ops _log_proc_fops = {
	.proc_open = _log_proc_open,
	.proc_read = seq_read,
	.proc_write = _log_proc_write,
	.proc_lseek = seq_lseek,
	.proc_release = single_release,
};
#else
static const struct file_operations _log_proc_fops = {
	.owner = THIS_MODULE,
	.open = _log_proc_open,
	.read = seq_read,
	.write = _log_proc_write,
	.llseek = seq_lseek,
	.release = single_release,
};
#endif

int log_proc_init(struct proc_dir_entry *_proc_dir, void *shm)
{
	int rc = 0;
	int32_t *log_level;
	int i;

	/* create the /proc file */
	if (proc_create_data(LOG_PROC_NAME, LOG_PROC_PERMS, _proc_dir, &_log_proc_fops, NULL) == NULL) {
		TRACE_BASE(DBG_ERR, "log proc creation failed\n");
		rc = -1;
	}

	shared_mem = shm;
	log_level = (int32_t *)(shared_mem + BASE_LOG_LEVEL_OFFSET);

	for (i = 0; i < ID_BUTT; ++i)
		log_level[i] = DEFAULT_LOG_LEVEL;

	return rc;
}

int log_proc_remove(struct proc_dir_entry *_proc_dir)
{
	remove_proc_entry(LOG_PROC_NAME, _proc_dir);
	shared_mem = NULL;

	return 0;
}
