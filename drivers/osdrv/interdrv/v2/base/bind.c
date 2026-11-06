#include <linux/types.h>
#include <linux/module.h>
#include <linux/spinlock.h>
#include <linux/mm.h>
#include <linux/vmalloc.h>
#include <linux/comm_sys.h>
#include <linux/comm_errno.h>
#include <linux/base_uapi.h>
#include <queue.h>
#include "base_debug.h"


#ifndef TAILQ_FOREACH_SAFE
#define TAILQ_FOREACH_SAFE(var, head, field, tvar)			\
	for ((var) = TAILQ_FIRST((head));				\
		(var) && ((tvar) = TAILQ_NEXT((var), field), 1);	\
		(var) = (tvar))
#endif

#define CHN_MATCH(x, y) (((x)->mod_id == (y)->mod_id) && ((x)->dev_id == (y)->dev_id)             \
	&& ((x)->chn_id == (y)->chn_id))

struct bind_t {
	TAILQ_ENTRY(bind_t) tailq;
	bind_node_s *node;
};

TAILQ_HEAD(bind_head, bind_t) binds;

static struct mutex bind_lock;
bind_node_s bind_nodes[BIND_NODE_MAXNUM];


static int32_t bind(mmf_chn_s *src_chn, mmf_chn_s *dest_chn)
{
	struct bind_t *item, *item_tmp;
	int32_t ret = 0, i;

	TRACE_BASE(DBG_DEBUG, "%s: src(mId=%d, dId=%d, cId=%d), dst(mId=%d, dId=%d, cId=%d)\n",
		__func__,
		src_chn->mod_id, src_chn->dev_id, src_chn->chn_id,
		dest_chn->mod_id, dest_chn->dev_id, dest_chn->chn_id);

	mutex_lock(&bind_lock);

	// check if dst already bind to src
	TAILQ_FOREACH_SAFE(item, &binds, tailq, item_tmp) {
		for (i = 0; i < item->node->dsts.num; ++i) {
			if (CHN_MATCH(&item->node->dsts.mmf_chn[i], dest_chn)) {
				TRACE_BASE(DBG_ERR, "Dst(%d-%d-%d) already bind to Src(%d-%d-%d)\n",
					dest_chn->mod_id, dest_chn->dev_id, dest_chn->chn_id,
					item->node->src.mod_id, item->node->src.dev_id, item->node->src.chn_id);
				ret = -1;
				goto BIND_EXIT;
			}
		}
	}

	TAILQ_FOREACH_SAFE(item, &binds, tailq, item_tmp) {
		if (!CHN_MATCH(&item->node->src, src_chn))
			continue;

		// check if dsts have enough space for one more bind
		if (item->node->dsts.num >= BIND_DEST_MAXNUM) {
			TRACE_BASE(DBG_ERR, "Over max bind Dst number\n");
			ret = -1;
			goto BIND_EXIT;
		}
		item->node->dsts.mmf_chn[item->node->dsts.num++] = *dest_chn;

		goto BIND_SUCCESS;
	}

	// if src not found
	for (i = 0; i < BIND_NODE_MAXNUM; ++i) {
		if (!bind_nodes[i].used) {
			memset(&bind_nodes[i], 0, sizeof(bind_nodes[i]));
			bind_nodes[i].used = true;
			bind_nodes[i].src = *src_chn;
			bind_nodes[i].dsts.num = 1;
			bind_nodes[i].dsts.mmf_chn[0] = *dest_chn;
			break;
		}
	}

	if (i == BIND_NODE_MAXNUM) {
		TRACE_BASE(DBG_ERR, "No free bind node\n");
		ret = -1;
		goto BIND_EXIT;
	}

	item = vzalloc(sizeof(*item));
	if (item == NULL) {
		memset(&bind_nodes[i], 0, sizeof(bind_nodes[i]));
		ret = ERR_SYS_NOMEM;
		goto BIND_EXIT;
	}

	item->node = &bind_nodes[i];
	TAILQ_INSERT_TAIL(&binds, item, tailq);

BIND_SUCCESS:
	ret = 0;
BIND_EXIT:
	mutex_unlock(&bind_lock);

	return ret;
}

static int32_t unbind(mmf_chn_s *src_chn, mmf_chn_s *dest_chn)
{
	struct bind_t *item, *item_tmp;
	uint32_t i;

	mutex_lock(&bind_lock);
	TAILQ_FOREACH_SAFE(item, &binds, tailq, item_tmp) {
		if (!CHN_MATCH(&item->node->src, src_chn))
			continue;

		for (i = 0; i < item->node->dsts.num; ++i) {
			if (CHN_MATCH(&item->node->dsts.mmf_chn[i], dest_chn)) {
				if (--item->node->dsts.num) {
					for (; i < item->node->dsts.num; i++)
						item->node->dsts.mmf_chn[i] = item->node->dsts.mmf_chn[i + 1];
				} else {
					item->node->used = false;
					TAILQ_REMOVE(&binds, item, tailq);
					vfree(item);
				}
				mutex_unlock(&bind_lock);
				return 0;
			}
		}
	}
	mutex_unlock(&bind_lock);
	return 0;
}

int32_t bind_get_dst(mmf_chn_s *src_chn, mmf_bind_dest_s *bind_dest)
{
	struct bind_t *item, *item_tmp;
	uint32_t i;

	TRACE_BASE(DBG_DEBUG, "%s: src(.mod_id=%d, .dev_id=%d, .chn_id=%d)\n",
		__func__, src_chn->mod_id,
		src_chn->dev_id, src_chn->chn_id);

	mutex_lock(&bind_lock);
	TAILQ_FOREACH_SAFE(item, &binds, tailq, item_tmp) {
		for (i = 0; i < item->node->dsts.num; ++i) {
			if (CHN_MATCH(&item->node->src, src_chn)) {
				*bind_dest = item->node->dsts;
				mutex_unlock(&bind_lock);
				return 0;
			}
		}
	}
	mutex_unlock(&bind_lock);

	return -1;
}
EXPORT_SYMBOL_GPL(bind_get_dst);

int32_t bind_get_src(mmf_chn_s *dest_chn, mmf_chn_s *src_chn)
{
	struct bind_t *item, *item_tmp;
	uint32_t i;

	TRACE_BASE(DBG_DEBUG, "%s: dst(.mod_id=%d, .dev_id=%d, .chn_id=%d)\n",
		__func__, dest_chn->mod_id,
		dest_chn->dev_id, dest_chn->chn_id);

	mutex_lock(&bind_lock);
	TAILQ_FOREACH_SAFE(item, &binds, tailq, item_tmp) {
		for (i = 0; i < item->node->dsts.num; ++i) {
			if (CHN_MATCH(&item->node->dsts.mmf_chn[i], dest_chn)) {
				*src_chn = item->node->src;
				mutex_unlock(&bind_lock);
				return 0;
			}
		}
	}
	mutex_unlock(&bind_lock);

	return -1;
}
EXPORT_SYMBOL_GPL(bind_get_src);


void bind_init(void)
{
	TAILQ_INIT(&binds);
	mutex_init(&bind_lock);
	memset(bind_nodes, 0, sizeof(bind_nodes));
}

void bind_deinit(void)
{
	struct bind_t *item, *item_tmp;

	mutex_lock(&bind_lock);
	TAILQ_FOREACH_SAFE(item, &binds, tailq, item_tmp) {
		TAILQ_REMOVE(&binds, item, tailq);
		vfree(item);
	}
	memset(bind_nodes, 0, sizeof(bind_nodes));
	mutex_unlock(&bind_lock);
}

int32_t bind_set_cfg_user(unsigned long arg)
{
	int32_t ret = 0;
	struct sys_bind_cfg ioctl_arg;

	ret = copy_from_user(&ioctl_arg,
			     (struct sys_bind_cfg __user *)arg,
			     sizeof(struct sys_bind_cfg));
	if (ret) {
		TRACE_BASE(DBG_ERR, "copy_from_user failed, sys_set_mod_cfg\n");
		return ret;
	}

	if (ioctl_arg.is_bind)
		ret = bind(&ioctl_arg.mmf_chn_src, &ioctl_arg.mmf_chn_dst);
	else
		ret = unbind(&ioctl_arg.mmf_chn_src, &ioctl_arg.mmf_chn_dst);

	return ret;
}

int32_t bind_get_cfg_user(unsigned long arg)
{
	int32_t ret = 0;
	struct sys_bind_cfg ioctl_arg;

	ret = copy_from_user(&ioctl_arg,
			     (struct sys_bind_cfg __user *)arg,
			     sizeof(struct sys_bind_cfg));
	if (ret) {
		TRACE_BASE(DBG_ERR, "copy_from_user failed, sys_set_mod_cfg\n");
		return ret;
	}

	if (ioctl_arg.get_by_src)
		ret = bind_get_dst(&ioctl_arg.mmf_chn_src, &ioctl_arg.bind_dst);
	else
		ret = bind_get_src(&ioctl_arg.mmf_chn_dst, &ioctl_arg.mmf_chn_src);

	if (ret) {
		TRACE_BASE(DBG_ERR, "sys_ctx_getbind %s failed\n",
			ioctl_arg.get_by_src ? "dst" : "src");
		return ret;
	}

	ret = copy_to_user((struct sys_bind_cfg __user *)arg,
			     &ioctl_arg,
			     sizeof(struct sys_bind_cfg));

	if (ret)
		TRACE_BASE(DBG_ERR, "copy_to_user fail, sys_get_bind_cfg\n");

	return ret;
}