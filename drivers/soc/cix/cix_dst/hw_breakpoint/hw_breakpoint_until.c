#include <linux/kallsyms.h>
#include <linux/types.h>
#include <linux/version.h>
#include <linux/slab.h>
#include <linux/soc/cix/plat_hw_breakpoint.h>
#include "hw_breakpoint_until.h"

#define HW_SYMS_VAL(sym) g_##sym

static spinlock_t *g_vmap_area_lock = NULL; /*kernel vm spinlock*/
static struct list_head *g_vmap_area_list = NULL; /*kernel vm list*/

#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 10, 72)
#define VM_LAZY_FREE 0x02
#define VM_VM_AREA 0x04
#endif

#ifdef HW_PROC_CMD_DEBUG
static void print_cmd_params(int argc, char *argv[])
{
	int loop = 0;

	for (loop = 0; loop < argc; loop++) {
		pr_info("loop:%d, %s\n", loop, argv[loop]);
	}
}
#endif

void process_cmd_string(char *p_buf, int *p_argc, char *p_argv[])
{
	int i_argc;
	char *p_tmp = p_buf;

	p_argv[0] = p_buf;
	i_argc = 1;

	while (*p_tmp) {
		if (' ' == *p_tmp) {
			*p_tmp = '\0';
			p_argv[i_argc++] = p_tmp + 1;
		}

		p_tmp++;
	}
	*p_argc = i_argc;
#ifdef HW_PROC_CMD_DEBUG
	print_cmd_params(*pArgc, pArgv);
#endif
}

void free_iophys_info(iophys_info *info)
{
	iophys_info *node = NULL, *next = NULL;

	if (info) {
		list_for_each_entry_safe (node, next, &info->list, list) {
			list_del(&node->list);
			kfree(node);
		}
		kfree(info);
	}
}
EXPORT_SYMBOL_GPL(free_iophys_info);

iophys_info *get_iophys_info(u64 addr)
{
	struct vmap_area *va = NULL;
	struct vm_struct *area = NULL;
	struct vm_struct *next = NULL;
	iophys_info *head = NULL;
	iophys_info *node = NULL;

	if (!HW_SYMS_VAL(vmap_area_lock) || !HW_SYMS_VAL(vmap_area_lock)) {
		pr_info("vmap_area_list or vmap_area_lock is NULL, can not get virt");
		return head;
	}

	spin_lock(HW_SYMS_VAL(vmap_area_lock));
	list_for_each_entry (va, HW_SYMS_VAL(vmap_area_list), list) {
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 10, 72)
		if (!(va->flags & VM_VM_AREA)) {
			continue;
		}
#endif
		if (!va) {
			continue;
		}
		area = va->vm;
		if (!area) {
			continue;
		}
		if (!(area->flags & VM_IOREMAP) ||
		    area->flags & VM_UNINITIALIZED) {
			continue;
		}
		smp_rmb();
		/*If you find the I/O address, check whether the I/O address you want to query is within the I/O address range*/
		next = area;
		while (next) {
			if (next->phys_addr && next->size) {
				/*The IO address to be queried is within its range*/
				if (addr >= next->phys_addr &&
				    addr < next->phys_addr + next->size) {
					/*find it*/
					if (head == NULL) {
						head = kzalloc(
							sizeof(iophys_info),
							GFP_KERNEL);
						if (head == NULL) {
							goto err;
						}
						INIT_LIST_HEAD(&head->list);
						head->area = *next;
						head->virt_addr =
							(u64)next->addr + addr -
							next->phys_addr;
					}
					node = kzalloc(sizeof(iophys_info),
						       GFP_KERNEL);
					if (node == NULL) {
						goto free;
					}
					INIT_LIST_HEAD(&node->list);
					node->area = *next;
					node->virt_addr = (u64)next->addr +
							  addr -
							  next->phys_addr;
					list_add_tail(&node->list, &head->list);
				}
			}
			next = next->next;
			if (next == area) {
				break;
			}
		}
	}
	spin_unlock(HW_SYMS_VAL(vmap_area_lock));

	return head;

free:
	free_iophys_info(head);
err:
	spin_unlock(HW_SYMS_VAL(vmap_area_lock));
	return NULL;
}
EXPORT_SYMBOL_GPL(get_iophys_info);

void hw_until_init(void)
{
	g_vmap_area_lock = (void *)kallsyms_lookup_name("vmap_area_lock");
	g_vmap_area_list = (void *)kallsyms_lookup_name("vmap_area_list");
}
