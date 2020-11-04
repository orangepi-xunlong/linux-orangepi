#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/mm.h>

#include "dbg_ini_util.h"
#include "sprdwl.h"

#define LOAD_BUF_SIZE 1024
#define MAX_PATH_NUM  3

static char *dbg_ini_file_path[MAX_PATH_NUM] = {
	"/lib/firmware/wifi_dbg.ini",
       	"/vendor/etc/wifi/wifi_dbg.ini",
       	"/etc/wifi_dbg.ini"
};

static int dbg_load_ini_resource(char *path[], char *buf, int size)
{
	int ret;
	int index = 0;
	mm_segment_t oldfs;
	struct file *filp = (struct file *)-ENOENT;

	for (index = 0; index < MAX_PATH_NUM; index++) {
		filp = filp_open(path[index], O_RDONLY, S_IRUSR);
		if (!IS_ERR(filp)) {
			pr_info("find wifi_dbg.ini file in %s\n", path[index]);
			break;
		}
	}
	if (IS_ERR(filp))
		return -ENOENT;

	oldfs = get_fs();
	set_fs(KERNEL_DS);
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 14, 1)
	ret = kernel_read(filp, buf, size, &filp->f_pos);
#else
	ret = kernel_read(filp, filp->f_pos, buf, size);
#endif
	set_fs(oldfs);

	filp_close(filp, NULL);

	return ret;
}

static int get_valid_line(char *buf, int buf_size, char *line, int line_size)
{
	int i = 0;
	int rem = 0;
	char *p = buf;

	while (1) {
		if (p - buf >= buf_size)
			break;

		if (i >= line_size)
			break;

		if (*p == '#' || *p == ';')
			rem = 1;

		switch (*p) {
		case '\0':
		case '\r':
		case '\n':
			if (i != 0) {
				line[i] = '\0';
				return p - buf + 1;
			} else {
				rem = 0;
			}

			break;

		case ' ':
			break;

		default:
			if (rem == 0)
				line[i++] = *p;
			break;
		}
		p++;
	}

	return -1;
}

static void dbg_ini_parse(struct dbg_ini_cfg *cfg, char *buf, int size)
{
	int ret;
	int sec = 0;
	int index = 0;
	int left = size;
	char *pos = buf;
	char line[256];
	int status[MAX_SEC_NUM] = {0};
	unsigned long value;

	while (1) {
		ret = get_valid_line(pos, left, line, sizeof(line));
		if (ret < 0 || left < ret)
			break;

		left -= ret;
		pos += ret;

		if (line[0] == '[') {
			if (strcmp(line, "[SOFTAP]") == 0)
				sec = SEC_SOFTAP;
			else if (strcmp(line, "[DEBUG]") == 0)
				sec = SEC_DEBUG;
			else
				sec = SEC_INVALID;

			status[sec]++;
			if (status[sec] != 1) {
				pr_info("invalid section %s\n", line);
				sec = SEC_INVALID;
			}
		} else {
			while (line[index] != '=' && line[index] != '\0')
				index++;

			if (line[index] != '=')
				continue;

			line[index++] = '\0';

			switch (sec) {
			case SEC_SOFTAP:
				if (strcmp(line, "channel") == 0) {
					if (!kstrtoul(&line[index], 0, &value))
						cfg->softap_channel = value;
				}

				break;

			case SEC_DEBUG:
				if (strcmp(line, "log_level") == 0) {
					if (!kstrtoul(&line[index], 0, &value))
						if (value >= L_NONE)
							sprdwl_debug_level = value;
				}

				break;

			default:
				pr_info("drop: %s\n", line);
				break;
			}
		}
	}
}

int dbg_util_init(struct dbg_ini_cfg *cfg)
{
	int ret;
	char *buf;

	buf = kmalloc(LOAD_BUF_SIZE, GFP_KERNEL);
	if (buf == NULL)
		return -ENOMEM;

	ret = dbg_load_ini_resource(dbg_ini_file_path, buf, LOAD_BUF_SIZE);
	if (ret <= 0) {
		kfree(buf);
		return -EINVAL;
	}

	cfg->softap_channel = -1;
	dbg_ini_parse(cfg, buf, ret);

	kfree(buf);
	return 0;
}
