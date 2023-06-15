/*
 * Copyright (C) 2019 Allwinnertech,  <liuli@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */

#include "include/eink_driver.h"
#include "include/eink_sys_source.h"
#include "include/fmt_convert.h"

extern u32 force_fresh_mode;

/*static unsigned int continue_gu16_cnt;*/

bool is_upd_list_empty(struct buf_manager *buffer_mgr)
{
	bool ret = false;

	mutex_lock(&buffer_mgr->mlock);

	if (list_empty(&buffer_mgr->img_used_list)) {
		ret = true;
	} else {
		ret = false;
	}

	mutex_unlock(&buffer_mgr->mlock);

	return ret;
}

bool is_coll_list_empty(struct buf_manager *buffer_mgr)
{
	bool ret = false;

	mutex_lock(&buffer_mgr->mlock);

	if (list_empty(&buffer_mgr->img_collision_list)) {
		ret = true;
	} else {
		ret = false;
	}

	mutex_unlock(&buffer_mgr->mlock);

	return ret;
}

struct img_node *request_new_node(struct buf_manager *buf_mgr)
{
	struct img_node *img_node = NULL;

	img_node = (struct img_node *)kmalloc(sizeof(struct img_node), GFP_KERNEL | __GFP_ZERO);
	if (img_node == NULL) {
		pr_err("[%s]:img node strcut malloc failed!\n", __func__);
		return NULL;
	}

	img_node->upd_order = 0;
	img_node->coll_flag = 0;
	img_node->update_master = -1;
	img_node->img = (struct eink_img *)kmalloc(sizeof(struct eink_img), GFP_KERNEL | __GFP_ZERO);
	if (img_node->img == NULL) {
		pr_err("%s:img strcut malloc failed!\n", __func__);
		kfree(img_node);
		return NULL;
	}
	memset(img_node->img, 0, sizeof(struct eink_img));

	img_node->img->size.width = buf_mgr->width;
	img_node->img->size.height = buf_mgr->height;
	img_node->img->size.align = 4;
	img_node->img->pitch = EINKALIGN(img_node->img->size.width,
			img_node->img->size.align);
	img_node->img->upd_mode = EINK_INIT_MODE;
	img_node->img->out_fmt = EINK_Y8;
	img_node->img->upd_all_en = false;

	return img_node;
}


void free_unused_node(struct img_node *img_node)
{
	EINK_INFO_MSG("[%s] order %d\n", __func__, img_node->upd_order);

	if (img_node->item) {
		eink_dma_unmap(img_node->item);
	}
	if (img_node->img) {
		kfree(img_node->img);
		img_node->img = NULL;
	}
	if (img_node) {
		kfree(img_node);
	}
	return;
}

int __reset_all(struct buf_manager *buf_mgr)
{
	struct img_node *cur_img_node = NULL, *tmp_img_node = NULL;

	mutex_lock(&buf_mgr->mlock);

	list_for_each_entry_safe(cur_img_node, tmp_img_node, &buf_mgr->img_collision_list, node) {
		list_move_tail(&cur_img_node->node, &buf_mgr->img_free_list);
	}
	list_for_each_entry_safe(cur_img_node, tmp_img_node, &buf_mgr->img_used_list, node) {
		list_move_tail(&cur_img_node->node, &buf_mgr->img_free_list);
	}
	list_for_each_entry_safe(cur_img_node, tmp_img_node, &buf_mgr->img_free_list, node) {
		list_del(&cur_img_node->node);
		free_unused_node(cur_img_node);
	}
	mutex_unlock(&buf_mgr->mlock);
	return 0;
}

bool get_buf_msg(struct buf_manager *buffer_mgr)
{
	bool ret = false;

	mutex_lock(&buffer_mgr->mlock);
	if (!list_empty(&buffer_mgr->img_free_list)) {
		ret = true;
	} else {
		ret = false;
	}
	mutex_unlock(&buffer_mgr->mlock);
	return ret;
}

int get_free_buffer_slot(struct buf_manager *buf_mgr, struct buf_slot *slot)
{
	u32 count = 0;
	struct img_node *cur_img_node = NULL, *tmp_img_node = NULL, *free_node = NULL;

	mutex_lock(&buf_mgr->mlock);

	if (list_empty(&buf_mgr->img_free_list)) {
		/*pr_err("[%s]Free list is empty!, Dont need free", __func__);*/
		slot->count = 0;
		return 0;
	}

	list_for_each_entry_safe(cur_img_node, tmp_img_node, &buf_mgr->img_free_list, node) {
		if (cur_img_node && count < 32) {
			slot->upd_order[count] = cur_img_node->upd_order;
			count++;
			free_node = cur_img_node;
			list_del(&free_node->node);
			free_unused_node(free_node);
			free_node = NULL;
		} else if (count > 32) {
			pr_err("count is exceed max err! May user forget free buffer\n");
			count = 32;
			break;
		}
	}
	slot->count = count;
	EINK_INFO_MSG("Free buf count is %d\n", count);

	mutex_unlock(&buf_mgr->mlock);
	return 0;
}

int wait_for_newest_img_node(struct buf_manager *buf_mgr)
{
	struct img_node *cur_img_node = NULL, *tmp_img_node = NULL;
	struct img_node *newest_node = NULL, *free_node = NULL;

	mutex_lock(&buf_mgr->mlock);
	/* free all coll buf node */
	if (!list_empty(&buf_mgr->img_collision_list)) {
		list_for_each_entry_safe(cur_img_node, tmp_img_node, &buf_mgr->img_collision_list, node) {
			free_node = cur_img_node;
			if (free_node->img != NULL) {
				free_node->img->upd_mode = EINK_INIT_MODE;
				free_node->img->win_calc_en = true;
				free_node->update_master = -1;
				list_move_tail(&free_node->node, &buf_mgr->img_free_list);
			}
		}
	}

	if (list_empty(&buf_mgr->img_used_list)) {
		EINK_INFO_MSG("Both empty!\n");
		goto out;
	}

	list_for_each_entry_safe(cur_img_node, tmp_img_node, &buf_mgr->img_used_list, node) {
		if (newest_node == NULL) {
			newest_node = cur_img_node;
		}
		if (cur_img_node->upd_order < newest_node->upd_order) {
			free_node = cur_img_node;
			if (free_node->img != NULL) {
				free_node->img->upd_mode = EINK_INIT_MODE;
				free_node->img->win_calc_en = true;
				free_node->update_master = -1;
				list_move_tail(&free_node->node, &buf_mgr->img_free_list);
			}
		} else
			newest_node = cur_img_node;
	}

	/* full screen */
out:
	mutex_unlock(&buf_mgr->mlock);
	return 0;
}

bool rect_is_overlap(struct upd_win a_area, struct upd_win b_area)
{
	bool overlap_flag = true;
	s32 w = 0, h = 0;

	if (is_upd_win_zero(a_area) || is_upd_win_zero(b_area)) {
		return false;
	}

	w = (s32)min(a_area.right, b_area.right) - (s32)max(a_area.left, b_area.left);
	h = (s32)min(a_area.bottom, b_area.bottom) - (s32)max(a_area.top, b_area.top);
	if (w >= 20 && h >= 20) {
		overlap_flag = true;
	} else {
		overlap_flag = false;
	}

	return overlap_flag;
}

/*
rectangle A include rectangle B, then return true;otherwise, return false
*/
static bool rect_include_judge(struct upd_win a_rect, struct upd_win b_rect)
{
	if ((a_rect.left <= b_rect.left) &&
		(a_rect.right >= b_rect.right) &&
		(a_rect.top <= b_rect.top) &&
		(a_rect.bottom >= b_rect.bottom)) {
		return true;
	} else {
		return false;
	}
}

#if 0
int rect_subtract_judge(struct upd_win org_area, struct upd_win sub_area, struct upd_win *output_area)
{
	bool partial_overlap = false;

	if (output_area == NULL) {
		pr_err("input param is NULL\n");
		return -1;
	}

	//is org_area and sub_area overlap?
	if (false == rect_is_overlap(org_area, sub_area)) {
		pr_err("two area is no overlap, cannot subtract\n");
		return -2;
	}

	//is sub_area include org_area?
	if (rect_include_judge(sub_area, org_area)) {
		pr_info("whole area is included,ORG:(%d,%d)~(%d,%d),SUB:(%d,%d)~(%d,%d)\n",
			org_area.left, org_area.top, org_area.right, org_area.bottom,
			sub_area.left, sub_area.top, sub_area.right, sub_area.bottom);
		return 0;
	}

	//is partial overlap
	if ((org_area.left == sub_area.left) && (org_area.right == sub_area.right)) {
		output_area->left = org_area.left;
		output_area->right = org_area.right;
		if (org_area.top == sub_area.top) {
			output_area->top = sub_area.bottom + 1;
			output_area->bottom = org_area.bottom;
			return 1;
		}

		if (org_area.bottom == sub_area.bottom) {
			output_area->top = org_area.top;
			output_area->bottom = sub_area.top - 1;
			return 1;
		}
	}


	if ((org_area.top == sub_area.top) && (org_area.bottom == sub_area.bottom)) {
		output_area->top = org_area.top;
		output_area->bottom = org_area.bottom;
		if (org_area.left == sub_area.left) {
			output_area->left = sub_area.right + 1;
			output_area->right = org_area.right;
			return 1;
		}

		if (org_area.right == sub_area.right) {
			output_area->left = org_area.left;
			output_area->right = sub_area.left - 1;
			return 1;
		}
	}

	pr_err("partial area cannot subtract\n");
	return -3;
}
#endif

bool rect_merge_check(struct upd_win new_rect, struct upd_win old_rect)
{
	bool ret = false;

	if (false == rect_is_overlap(new_rect, old_rect)) {
		return false;
	}

	if (rect_include_judge(new_rect, old_rect)) {
		ret = true;
	} else {
		if (((new_rect.left == old_rect.left) && (new_rect.right == old_rect.right)) ||
			((new_rect.top == old_rect.top) && (new_rect.bottom == old_rect.bottom))) {
			ret = true;
		} else {
			ret = false;
		}
	}

	return ret;
}

static struct img_node *try_to_merge_image(struct img_node *anode, struct img_node *bnode)
{
	struct img_node *tmp_anode = NULL, *tmp_bnode = NULL;

	if ((anode == NULL) || (bnode == NULL)) {
		pr_err("input node is null\n");
		return NULL;
	}

	if ((anode->img == NULL) || (bnode->img == NULL)) {
		pr_err("input image is null\n");
		return NULL;
	}

	tmp_anode = (anode->upd_order >= bnode->upd_order) ? anode : bnode;//new update
	tmp_bnode = (tmp_anode == anode) ? bnode : anode;//old update

	/* old update image is specify no merge, then return NULL*/
	if (tmp_bnode->force_fresh == true) {
		pr_warn("image specity no merge, mode=0x%x, order=%d\n",
			tmp_bnode->img->upd_mode, tmp_bnode->upd_order);
		return NULL;
	}

	if (false == rect_merge_check(tmp_anode->img->upd_win, tmp_bnode->img->upd_win)) {
		return NULL;
	}

	if ((tmp_bnode->img->upd_mode & 0xff) == EINK_DU_MODE) {
		pr_warn("du try merge\n");
	}

	tmp_anode->coll_flag |= tmp_bnode->coll_flag;

	tmp_anode->img->upd_win.left = min(tmp_anode->img->upd_win.left, tmp_bnode->img->upd_win.left);
	tmp_anode->img->upd_win.right = max(tmp_anode->img->upd_win.right, tmp_bnode->img->upd_win.right);
	tmp_anode->img->upd_win.top = min(tmp_anode->img->upd_win.top, tmp_bnode->img->upd_win.top);
	tmp_anode->img->upd_win.bottom = max(tmp_anode->img->upd_win.bottom, tmp_bnode->img->upd_win.bottom);

	return tmp_anode;
}

static struct img_node *get_img_from_coll_list(struct buf_manager *buffer_mgr)
{
	struct img_node *img_node = NULL, *combine_node = NULL, *free_node = NULL;
	struct img_node *cur_img_node = NULL, *tmp_img_node = NULL;
	unsigned int tmp_order = 0;

	mutex_lock(&buffer_mgr->mlock);

	if (list_empty(&buffer_mgr->img_collision_list)) {
		mutex_unlock(&buffer_mgr->mlock);
		return NULL;
	}

	/* merge image node */
	list_for_each_entry_safe(cur_img_node, tmp_img_node, &buffer_mgr->img_collision_list, node) {
		if (img_node == NULL) {
			img_node = cur_img_node;
		} else {
			//NO MERGE
			combine_node = try_to_merge_image(img_node, cur_img_node);
			if (combine_node == NULL) {
				EINK_DEBUG_MSG("merge block\n");
				break;
			} else {
				//merge success, free older image
				if (combine_node == img_node) {
					free_node = cur_img_node;
				} else {
					free_node = img_node;
					img_node = combine_node;
				}

				if (free_node->img != NULL) {
					tmp_order = free_node->upd_order;
					free_node->img->upd_mode = EINK_INIT_MODE;
					free_node->img->win_calc_en = true;
					free_node->update_master = -1;
					list_move_tail(&free_node->node, &buffer_mgr->img_free_list);
					EINK_INFO_MSG("coll merge successfully, free image(order=%d)\n", tmp_order);
				}
			}
		}
	}

	img_node = NULL;
	list_for_each_entry_safe(cur_img_node, tmp_img_node, &buffer_mgr->img_collision_list, node) {
		if (cur_img_node->coll_flag == 0) {
			img_node = cur_img_node;
			break;
		}
	}

	if (img_node != NULL) {
		if (img_node->img == NULL) {
			EINK_DEBUG_MSG("[%s]img_node NULL\n", __func__);
			img_node = NULL;
		} else {
			EINK_INFO_MSG("get image node from collision list, order=%d\n", img_node->upd_order);
		}
	} else {
		EINK_DEBUG_MSG("no valid collision node, please wait\n");
	}

	mutex_unlock(&buffer_mgr->mlock);
	return img_node;
}

struct img_node *get_img_from_upd_list(struct buf_manager *buffer_mgr)
{
	struct img_node *cur_img_node = NULL, *tmp_img_node = NULL;
	struct img_node *img_node = NULL;
	u8 overlap = 0;
#if 0
	struct img_node *combine_node = NULL, *free_node = NULL;
	unsigned int tmp_order = 0;
#endif

	mutex_lock(&buffer_mgr->mlock);
	if (list_empty(&buffer_mgr->img_used_list)) {
		EINK_INFO_MSG("is empty\n");
		mutex_unlock(&buffer_mgr->mlock);
		return NULL;
	}

	list_for_each_entry_safe(cur_img_node, tmp_img_node, &buffer_mgr->img_used_list, node) {
		if (img_node == NULL) {
			img_node = cur_img_node;
		} else
			break;
	}

	if (img_node != NULL) {
		if (img_node->img == NULL) {
			EINK_INFO_MSG("img_node is NULL\n");
			img_node = NULL;
			goto out;
		} else {
			//check with collision list
			if (list_empty(&buffer_mgr->img_collision_list)) {
				//no collision
				EINK_DEBUG_MSG("get img node sucess from upd list, order=%d, imgaddr=%p\n",
						img_node->upd_order, img_node->img);
			} else {
				list_for_each_entry_safe(cur_img_node, tmp_img_node, &buffer_mgr->img_collision_list, node) {
					if (rect_is_overlap(img_node->img->upd_win, cur_img_node->img->upd_win) &&
							(img_node->upd_order > cur_img_node->upd_order)) {
						overlap = 1;
						break;
					}
				}

				if (overlap) {
					img_node = NULL;
					EINK_DEBUG_MSG("current update overlap with collision list\n");
				} else {
					EINK_DEBUG_MSG("get img node sucess from upd list, order=%d, imgaddr=%p\n",
							img_node->upd_order, img_node->img);
				}
			}
		}
	} else {
		EINK_DEBUG_MSG("no valid image node for update\n");
	}

out:
	mutex_unlock(&buffer_mgr->mlock);

	return img_node;
}


bool check_upd_coll_state(struct buf_manager *mgr, u64 pipe_free_state)
{
	bool can_upd_flag = false;
	struct img_node *curnode = NULL, *tmpnode = NULL;

	mutex_lock(&mgr->mlock);
	list_for_each_entry_safe(curnode, tmpnode, &mgr->img_collision_list, node) {
		if (curnode)
			curnode->coll_flag &= (~pipe_free_state);
		if (curnode->coll_flag == 0) {
			can_upd_flag = true;
		}
	}
	mutex_unlock(&mgr->mlock);

	return can_upd_flag;
}

static int add_img_to_coll_list(struct buf_manager *buf_mgr, struct img_node *img_node)
{
	int ret = 0;
	struct img_node *curnode = NULL, *tmp_node = NULL;

	if ((buf_mgr == NULL) || (img_node == NULL)) {
		pr_err("%s:input param is null\n", __func__);
		return -1;
	}

	/* for buf list debug */
	if (eink_get_print_level() == 3) {
		EINK_INFO_MSG("Before Add Coll Image\n");
		print_coll_img_list(buf_mgr);
	}

	mutex_lock(&buf_mgr->mlock);
	list_for_each_entry_safe(curnode, tmp_node, &buf_mgr->img_collision_list, node) {
		if (curnode == img_node) {	//already add
			pr_debug("%s:image node is already here\n", __func__);
			goto out;
		}

	}

	__list_del_entry(&img_node->node);
	list_add_tail(&img_node->node, &buf_mgr->img_collision_list);
	/* if img is collison we cant set upd all en hardware limit */
	img_node->img->upd_all_en = false;

out:
	mutex_unlock(&buf_mgr->mlock);

	/* for debug */
	if (eink_get_print_level() == 3) {
		EINK_INFO_MSG("After Add Coll Image\n");
		print_coll_img_list(buf_mgr);
	}

	return ret;
}

static int remove_img_from_coll_list(struct buf_manager *buffer_mgr, struct img_node *img_node)
{
	int ret = -1;
	struct img_node *cur_img_node = NULL, *tmp_img_node = NULL;
	unsigned int tmp_order = 0;

	if ((buffer_mgr == NULL) || (img_node == NULL)) {
		pr_err("%s:input param is null\n", __func__);
		return -1;
	}

	/* for debug */
	if (eink_get_print_level() == 3) {
		EINK_INFO_MSG("Before Remove Coll Image\n");
		print_coll_img_list(buffer_mgr);
	}

	mutex_lock(&buffer_mgr->mlock);
	if (list_empty(&buffer_mgr->img_collision_list)) {
		pr_err("no collision image can be free\n");
		ret = 0;
		goto out;
	}

	list_for_each_entry_safe(cur_img_node, tmp_img_node, &buffer_mgr->img_collision_list, node) {
		if (img_node == cur_img_node) {
			tmp_order = cur_img_node->upd_order;
			if (cur_img_node->img != NULL) {
				cur_img_node->img->upd_mode = EINK_INIT_MODE;
				cur_img_node->img->win_calc_en = true;
				cur_img_node->update_master = -1;
			}
			list_move_tail(&cur_img_node->node, &buffer_mgr->img_free_list);
			EINK_INFO_MSG("remove image node from collision list, order=%d\n", tmp_order);
			ret = 0;
			break;
		}
	}
out:
	mutex_unlock(&buffer_mgr->mlock);

	/* for debug */
	if (eink_get_print_level() == 3) {
		EINK_INFO_MSG("After Remove Coll Image\n");
		print_coll_img_list(buffer_mgr);
	}

	return ret;
}

int auto_mode_select(struct eink_img *last_img, struct eink_img *cur_img, u32 *auto_mode)
{
	int ret = 0;
	struct fmt_convert_manager *cvt_mgr = NULL;

	if (last_img == NULL || cur_img == NULL) {
		pr_err("%s:input para err or first!\n", __func__);
		return -EINVAL;
	}

	EINK_INFO_MSG("AUTO MODE SEL\n");
	cvt_mgr = get_fmt_convert_mgr(0);

	if ((last_img->upd_mode == EINK_DU_MODE) || (cur_img->upd_mode == EINK_DU_MODE))
		*auto_mode = EINK_DU_MODE;
	else {
		ret = cvt_mgr->fmt_auto_mode_select(cvt_mgr, last_img, cur_img);
		if (ret < 0)
			pr_err("%s:auto_mode calc failed!\n", __func__);
		*auto_mode = ret;
	}

	return ret;
}

static s32 set_global_clean_counter(struct buf_manager *buf_mgr, unsigned int cnt)
{
	if (cnt > 20) {
		pr_err("%s:global clean counter is to large\n", __func__);
		return -1;
	}

	mutex_lock(&buf_mgr->mlock);
	buf_mgr->global_clean_cnt = cnt;
	mutex_unlock(&buf_mgr->mlock);

	return 0;
}

bool check_valid_update_mode(enum upd_mode mode)
{
	bool ret = false;

	switch (mode & 0xff) {
	case EINK_INIT_MODE:
	case EINK_DU_MODE:
	case EINK_GC16_MODE:
	case EINK_GC4_MODE:
	case EINK_A2_MODE:
	case EINK_GL16_MODE:
	case EINK_GLR16_MODE:
	case EINK_GLD16_MODE:
	case EINK_GU16_MODE:
	case EINK_GC4L_MODE:
	case EINK_GCC16_MODE:
	case EINK_CLEAR_MODE:
		ret = true;
		break;
	default:
		ret = false;
		break;
	}

	return ret;
}

bool is_local_update_mode(enum upd_mode mode)
{
	bool ret = false;

	switch (mode & 0xff) {
	case EINK_DU_MODE:
		ret = true;
		break;
	default:
		ret = false;
		break;
	}
	return ret;
}

int buf_queue_image(struct buf_manager *buf_mgr, struct eink_upd_cfg *upd_cfg)
{
	int ret = 0;
	struct img_node *curnode = NULL;
	struct eink_img *cur_img = &upd_cfg->img;

	if (buf_mgr == NULL || cur_img == NULL) {
		pr_err("%s:buf_mgr or cur_img is null, please check\n", __func__);
		return -EINVAL;
	}

	/* for calc queue image use time */
	if (eink_get_print_level() == 8) {
		getnstimeofday(&buf_mgr->stimer);
	}

	if (check_valid_update_mode(GET_UPDATE_MODE(cur_img->upd_mode)) == false) {
		pr_warn("%s:unknown mode(0x%08x), set GU16 MODE\n", __func__, cur_img->upd_mode);
		cur_img->upd_mode = EINK_GU16_MODE;
	}

	if (force_fresh_mode > 0)
		cur_img->upd_mode = force_fresh_mode;

	EINK_INFO_MSG("caller info: order=%d out_fmt=0x%x upd_mode=0x%x, dither_mode=0x%x, area(%d,%d)~(%d,%d)\n",
				upd_cfg->order, cur_img->out_fmt, cur_img->upd_mode,
				cur_img->dither_mode,
				cur_img->upd_win.left, cur_img->upd_win.top,
				cur_img->upd_win.right, cur_img->upd_win.bottom);
	EINK_INFO_MSG("caller info: upd_all_en=%d force_fresh=%d, img addr=%p\n",
				cur_img->upd_all_en, upd_cfg->force_fresh, cur_img);

	if (is_upd_win_zero(cur_img->upd_win) == true) {
		pr_err("[%s]:upd win is Zero not fresh!\n", __func__);
		return 0;
	}

	curnode = request_new_node(buf_mgr);
	if (curnode == NULL) {
		pr_err("[%s]:current node is NULL!\n", __func__);
		return -ENOMEM;
	}

	if (cur_img->fd >= 0) {
		curnode->item = eink_dma_map(cur_img->fd);
		if (curnode->item == NULL) {
			pr_err("%s:[EINK]dma map item failed!\n", __func__);
			free_unused_node(curnode);
			curnode = NULL;
			return -EINVAL;
		}

		cur_img->paddr = curnode->item->dma_addr;
	}
	EINK_DEBUG_MSG("[%s]cur node dma_addr = 0x%lx\n", __func__, (unsigned long)cur_img->paddr);

	/* for debug */
	if (eink_get_print_level() == 3) {
		EINK_INFO_MSG("Before Queue Image\n");
		print_used_img_list(buf_mgr);
		print_free_img_list(buf_mgr);
	}

	mutex_lock(&buf_mgr->mlock);

	buf_mgr->upd_order = upd_cfg->order;
	curnode->upd_order = upd_cfg->order;

	if ((curnode->upd_order == 0) && (GET_UPDATE_MODE(cur_img->upd_mode) != EINK_INIT_MODE)) {
		cur_img->upd_mode = GET_UPDATE_INFO(cur_img->upd_mode) | EINK_GC16_MODE;
	}
	curnode->force_fresh = upd_cfg->force_fresh;

	if (cur_img->out_fmt > 0xe || (cur_img->out_fmt < 0x9 && cur_img->out_fmt != 0x0)) {
		pr_warn("[%s]fmt set invalid use default Y8\n", __func__);
		cur_img->out_fmt = EINK_Y8;
	}

	if (cur_img->upd_win.left >= buf_mgr->width)
		cur_img->upd_win.left = buf_mgr->width - 1;

	if (cur_img->upd_win.top >= buf_mgr->height)
		cur_img->upd_win.top = buf_mgr->height - 1;

	if (cur_img->upd_win.right >= buf_mgr->width)
		cur_img->upd_win.right = buf_mgr->width - 1;

	if (cur_img->upd_win.bottom >= buf_mgr->height)
		cur_img->upd_win.bottom = buf_mgr->height - 1;

#if 0
	/* eink200 GC16 is like GU16, if want upd all screen must set 1 to upd_all_en */
	/* prevent local flicker upd_win must full screen */
	if (buf_mgr->global_clean_cnt != 0) {
		if (cur_img->upd_mode == EINK_GU16_MODE) {
			continue_gu16_cnt++;
			if (continue_gu16_cnt >= buf_mgr->global_clean_cnt) {
				continue_gu16_cnt = 0;
				cur_img->upd_all_en = true;
				cur_img->upd_mode = EINK_GC16_MODE;
			}
		} else {
			continue_gu16_cnt = 0;
		}
	} else {
		continue_gu16_cnt = 0;
	}
#endif

	//curnode->img = cur_img;//upd_cfg->img;
	memcpy(curnode->img, cur_img, sizeof(struct eink_img));
	ret = 0;

	list_add_tail(&curnode->node, &buf_mgr->img_used_list);

	if ((cur_img != NULL) && (curnode != NULL)) {
		EINK_INFO_MSG("add to update list: order=%d, mode=0x%x, (%d,%d)~(%d,%d)\n",
				curnode->upd_order, cur_img->upd_mode,
				cur_img->upd_win.left, cur_img->upd_win.top,
				cur_img->upd_win.right, cur_img->upd_win.bottom);
	}

	/* debug */
	if (eink_get_print_level() == 8) {
		getnstimeofday(&buf_mgr->etimer);

		pr_info("order=%d, queue image take %d ms\n", curnode->upd_order,
				get_delt_ms_timer(buf_mgr->stimer, buf_mgr->etimer));
	}

	if (eink_get_print_level() == 9) {
		eink_save_img(cur_img->fd, 3, buf_mgr->width,
				buf_mgr->height, curnode->upd_order, NULL, 0);
	}

	if (eink_get_print_level() == 5) {
		eink_kmap_img(curnode);
	}

	mutex_unlock(&buf_mgr->mlock);

	/* for debug */
	if (eink_get_print_level() == 3) {
		EINK_INFO_MSG("After Queue Image\n");
		print_used_img_list(buf_mgr);
		print_free_img_list(buf_mgr);
	}

	return ret;
}

s32 buf_dequeue_image(struct buf_manager *buf_mgr, struct img_node *img_node)
{
	int ret = 0;
	struct img_node *image_node = NULL, *tmp_node = NULL;
	unsigned int order = img_node->upd_order;

	if (img_node == NULL) {
		pr_err("image buffer node is null\n");
		return 0;
	}

	mutex_lock(&buf_mgr->mlock);
	list_for_each_entry_safe(image_node, tmp_node, &buf_mgr->img_used_list, node) {
		if (image_node != img_node) {
			continue;
		}

		order = image_node->upd_order;
		if (image_node->img != NULL) {
			EINK_DEBUG_MSG("img = %p\n", image_node->img);
			image_node->img->upd_mode = EINK_INIT_MODE;
			image_node->img->win_calc_en = true;
			image_node->update_master = -1;
		}
		list_move_tail(&image_node->node, &buf_mgr->img_free_list);
		EINK_DEBUG_MSG("dequeue image order %d\n", order);
	}

	mutex_unlock(&buf_mgr->mlock);

	/* EINK_INFO_MSG("dequeue image buffer %d, order %d\n", img_node->buf_id, order); */

	return ret;
}

int buf_mgr_init(struct eink_manager *eink_mgr)
{
	int ret = 0;
	u32 gray_level_cnt = 0;
	struct buf_manager *buf_mgr = NULL;

	buf_mgr = (struct buf_manager *)kmalloc(sizeof(struct buf_manager), GFP_KERNEL | __GFP_ZERO);
	if (buf_mgr == NULL) {
		pr_err("buf manager malloc failed!\n");
		ret = -ENOMEM;
		goto mgr_err_out;
	}
	memset((void *)buf_mgr, 0, sizeof(struct buf_manager));

	gray_level_cnt = eink_mgr->panel_info.gray_level_cnt;

	buf_mgr->gray_level_cnt = gray_level_cnt;
	buf_mgr->width = eink_mgr->panel_info.width;
	buf_mgr->height = eink_mgr->panel_info.height;
	buf_mgr->buf_size = buf_mgr->width * buf_mgr->height;
	buf_mgr->upd_order = 0;
	buf_mgr->global_clean_cnt = DEFAULT_GC_COUNTER;
	buf_mgr->processing_img_node = NULL;

	mutex_init(&buf_mgr->mlock);
	INIT_LIST_HEAD(&buf_mgr->img_free_list);
	INIT_LIST_HEAD(&buf_mgr->img_used_list);
	INIT_LIST_HEAD(&buf_mgr->img_collision_list);

	buf_mgr->coll_img_workqueue = alloc_workqueue("EINK_COLL_WORK",
			WQ_HIGHPRI | WQ_MEM_RECLAIM | WQ_UNBOUND, 1);
	INIT_WORK(&buf_mgr->coll_handle_work, upd_coll_win_irq_handler);

	buf_mgr->is_upd_list_empty = is_upd_list_empty;
	buf_mgr->is_coll_list_empty = is_coll_list_empty;
	buf_mgr->add_img_to_coll_list = add_img_to_coll_list;
	buf_mgr->remove_img_from_coll_list = remove_img_from_coll_list;
	buf_mgr->check_upd_coll_state = check_upd_coll_state;
	buf_mgr->get_img_from_upd_list = get_img_from_upd_list;
	buf_mgr->get_img_from_coll_list = get_img_from_coll_list;
	buf_mgr->queue_image = buf_queue_image;
	buf_mgr->dequeue_image = buf_dequeue_image;
	buf_mgr->set_global_clean_counter = set_global_clean_counter;
	buf_mgr->wait_for_newest_img_node = wait_for_newest_img_node;
	buf_mgr->get_buf_msg = get_buf_msg;
	buf_mgr->reset_all = __reset_all;
	buf_mgr->get_free_buffer_slot = get_free_buffer_slot;

	eink_mgr->buf_mgr = buf_mgr;

	return ret;

mgr_err_out:
	kfree(buf_mgr);
	return ret;

}
