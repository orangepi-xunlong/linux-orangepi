#include <linux/common.h>
#include <linux/comm_sys.h>
#include <linux/base_uapi.h>
#include "base_ctx.h"
#include "sys_proc.h"
#include "base_common.h"
#include "base_debug.h"

#define SYS_PROC_NAME			"sys"
#define SYS_PROC_PERMS			(0644)

static void *shared_mem;

extern bind_node_s bind_nodes[BIND_NODE_MAXNUM];

/*************************************************************************
 *	sys proc functions
 *************************************************************************/
static bool _is_fisrt_level_bind_node(bind_node_s *node)
{
	int i, j;
	bind_node_s *bindnodes;

	bindnodes = bind_nodes;
	for (i = 0; i < BIND_NODE_MAXNUM; ++i) {
		if ((bindnodes[i].used) && (bindnodes[i].dsts.num != 0)
			&& !CHN_MATCH(&bindnodes[i].src, &node->src)) {
			for (j = 0; j < bindnodes[i].dsts.num; ++j) {
				if (CHN_MATCH(&bindnodes[i].dsts.mmf_chn[j], &node->src))
					// find other source in front of this node
					return false;
			}
		}
	}

	return true;
}

static bind_node_s *_find_next_bind_node(const mmf_chn_s *pstSrcChn)
{
	int i;
	bind_node_s *bindnodes;

	bindnodes = bind_nodes;
	for (i = 0; i < BIND_NODE_MAXNUM; ++i) {
		if ((bindnodes[i].used) && CHN_MATCH(pstSrcChn, &bindnodes[i].src)
			&& (bindnodes[i].dsts.num != 0)) {
			return &bindnodes[i];
		}
	}

	return NULL; // didn't find next bind node
}

static void _show_sys_status(struct seq_file *m)
{
	int i, j, k;
	mmf_version_s *mmfversion;
	bind_node_s *bindnodes, *nextbindnode;
	mmf_chn_s *first, *second, *third;

	mmfversion = (mmf_version_s *)(shared_mem + BASE_VERSION_INFO_OFFSET);
	bindnodes = bind_nodes;

	seq_printf(m, "\nModule: [SYS], Version[%s], Build Time[%s]\n", mmfversion->version, UTS_VERSION);
	seq_puts(m, "-----BIND RELATION TABLE-----------------------------------------------------------------------------------------------------------\n");
	seq_printf(m, "%-10s%-10s%-10s%-10s%-10s%-10s%-10s%-10s%-10s\n",
		"1stMod", "1stDev", "1stChn", "2ndMod", "2ndDev", "2ndChn", "3rdMod", "3rdDev", "3rdChn");

	for (i = 0; i < BIND_NODE_MAXNUM; ++i) {
		//Check if the bind node is used / has destination / first level of bind chain
		if ((bindnodes[i].used) && (bindnodes[i].dsts.num != 0)
			&& (_is_fisrt_level_bind_node(&bindnodes[i]))) {

			first = &bindnodes[i].src; //bind chain first level

			for (j = 0; j < bindnodes[i].dsts.num; ++j) {
				second = &bindnodes[i].dsts.mmf_chn[j]; //bind chain second level

				nextbindnode = _find_next_bind_node(second);
				if (nextbindnode != NULL) {
					for (k = 0; k < nextbindnode->dsts.num; ++k) {
					third = &nextbindnode->dsts.mmf_chn[k]; //bind chain third level
					seq_printf(m, "%-10s%-10d%-10d%-10s%-10d%-10d%-10s%-10d%-10d\n",
						sys_get_modname(first->mod_id), first->dev_id, first->chn_id,
						sys_get_modname(second->mod_id), second->dev_id, second->chn_id,
						sys_get_modname(third->mod_id), third->dev_id, third->chn_id);
					}
				} else { //level 3 node not found
					seq_printf(m, "%-10s%-10d%-10d%-10s%-10d%-10d%-10s%-10d%-10d\n",
						sys_get_modname(first->mod_id), first->dev_id, first->chn_id,
						sys_get_modname(second->mod_id), second->dev_id, second->chn_id,
						"null", 0, 0);
				}
			}
		}
	}
	seq_puts(m, "\n-----------------------------------------------------------------------------------------------------------------------------------\n");
}

static int _sys_proc_show(struct seq_file *m, void *v)
{
	_show_sys_status(m);
	return 0;
}

static int _sys_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, _sys_proc_show, NULL);
}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
static const struct proc_ops _sys_proc_fops = {
	.proc_open = _sys_proc_open,
	.proc_read = seq_read,
	.proc_lseek = seq_lseek,
	.proc_release = single_release,
};
#else
static const struct file_operations _sys_proc_fops = {
	.owner = THIS_MODULE,
	.open = _sys_proc_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};
#endif

int sys_proc_init(struct proc_dir_entry *_proc_dir, void *shm)
{
	int rc = 0;

	/* create the /proc file */
	if (proc_create_data(SYS_PROC_NAME, SYS_PROC_PERMS, _proc_dir, &_sys_proc_fops, NULL) == NULL) {
		TRACE_BASE(DBG_ERR, "sys proc creation failed\n");
		rc = -1;
	}

	shared_mem = shm;
	return rc;
}

int sys_proc_remove(struct proc_dir_entry *_proc_dir)
{
	remove_proc_entry(SYS_PROC_NAME, _proc_dir);
	shared_mem = NULL;

	return 0;
}