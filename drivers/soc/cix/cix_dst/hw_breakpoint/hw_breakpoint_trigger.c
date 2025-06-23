#include <linux/slab.h>
#include <linux/list.h>
#include <linux/hw_breakpoint.h>
#include <linux/soc/cix/plat_hw_breakpoint.h>
#include <linux/crc32.h>

#define VALUE_TO_STR(val) #val

char *condition_type_str[CONDITION_TYPE_MAX + 1] = {
	VALUE_TO_STR(CONDITION_TYPE_NONE),
	VALUE_TO_STR(CONDITION_TYPE_EQUAL),
	VALUE_TO_STR(CONDITION_TYPE_NOT_EQUAL),
	VALUE_TO_STR(CONDITION_TYPE_LESS),
	VALUE_TO_STR(CONDITION_TYPE_LESS_EQUAL),
	VALUE_TO_STR(CONDITION_TYPE_GREATER),
	VALUE_TO_STR(CONDITION_TYPE_GREATER_EQUAL),
	VALUE_TO_STR(CONDITION_TYPE_RANGE),
	VALUE_TO_STR(CONDITION_TYPE_NOT_RANGE),
	VALUE_TO_STR(CONDITION_TYPE_NONE),
};

char *value_type_str[VALUE_TYPE_MAX + 1] = {
	VALUE_TO_STR(VALUE_TYPE_OLD), VALUE_TO_STR(VALUE_TYPE_NEW),
	VALUE_TO_STR(VALUE_TYPE_PC),  VALUE_TO_STR(VALUE_TYPE_SP),
	VALUE_TO_STR(VALUE_TYPE_X30), VALUE_TO_STR(VALUE_TYPE_X29),
	VALUE_TO_STR(VALUE_TYPE_X28), VALUE_TO_STR(VALUE_TYPE_X27),
	VALUE_TO_STR(VALUE_TYPE_X26), VALUE_TO_STR(VALUE_TYPE_X25),
	VALUE_TO_STR(VALUE_TYPE_X24), VALUE_TO_STR(VALUE_TYPE_X23),
	VALUE_TO_STR(VALUE_TYPE_X22), VALUE_TO_STR(VALUE_TYPE_X21),
	VALUE_TO_STR(VALUE_TYPE_X20), VALUE_TO_STR(VALUE_TYPE_X19),
	VALUE_TO_STR(VALUE_TYPE_X18), VALUE_TO_STR(VALUE_TYPE_X17),
	VALUE_TO_STR(VALUE_TYPE_X16), VALUE_TO_STR(VALUE_TYPE_X15),
	VALUE_TO_STR(VALUE_TYPE_X14), VALUE_TO_STR(VALUE_TYPE_X13),
	VALUE_TO_STR(VALUE_TYPE_X12), VALUE_TO_STR(VALUE_TYPE_X11),
	VALUE_TO_STR(VALUE_TYPE_X10), VALUE_TO_STR(VALUE_TYPE_X9),
	VALUE_TO_STR(VALUE_TYPE_X8),  VALUE_TO_STR(VALUE_TYPE_X7),
	VALUE_TO_STR(VALUE_TYPE_X6),  VALUE_TO_STR(VALUE_TYPE_X5),
	VALUE_TO_STR(VALUE_TYPE_X4),  VALUE_TO_STR(VALUE_TYPE_X3),
	VALUE_TO_STR(VALUE_TYPE_X2),  VALUE_TO_STR(VALUE_TYPE_X1),
	VALUE_TO_STR(VALUE_TYPE_X0),
};

extern void hw_manage_lock(void);
extern void hw_manage_unlock(void);
extern struct list_head *hw_get_rules(u64 addr);

typedef bool (*hw_bp_contion_callback)(u64 value, u64 mask, u64 val1, u64 val2);

static bool handle_condition_none(u64 value, u64 mask, u64 val1, u64 val2)
{
	(void)value;
	(void)mask;
	(void)val1;
	(void)val2;

	return true;
}

static bool handle_condition_equal(u64 value, u64 mask, u64 val1, u64 val2)
{
	(void)val2;

	if ((value & mask) == val1) {
		return true;
	}
	return false;
}

static bool handle_condition_not_equal(u64 value, u64 mask, u64 val1, u64 val2)
{
	(void)val2;

	if ((value & mask) != val1) {
		return true;
	}
	return false;
}

static bool handle_condition_less(u64 value, u64 mask, u64 val1, u64 val2)
{
	(void)val2;

	if ((value & mask) < val1) {
		return true;
	}
	return false;
}

static bool handle_condition_less_equal(u64 value, u64 mask, u64 val1, u64 val2)
{
	(void)val2;

	if ((value & mask) <= val1) {
		return true;
	}
	return false;
}

static bool handle_condition_greater(u64 value, u64 mask, u64 val1, u64 val2)
{
	(void)val2;

	if ((value & mask) > val1) {
		return true;
	}
	return false;
}

static bool handle_condition_greater_equal(u64 value, u64 mask, u64 val1,
					   u64 val2)
{
	(void)val2;

	if ((value & mask) < val1) {
		return true;
	}
	return false;
}

static bool handle_condition_range(u64 value, u64 mask, u64 val1, u64 val2)
{
	u64 tmp = (value & mask);

	if (tmp >= val1 && tmp <= val2) {
		return true;
	}
	return false;
}

static bool handle_condition_not_range(u64 value, u64 mask, u64 val1, u64 val2)
{
	u64 tmp = (value & mask);

	if (tmp < val1 || tmp > val2) {
		return true;
	}
	return false;
}

static hw_bp_contion_callback gcontion_callback[CONDITION_TYPE_MAX + 1] = {
	handle_condition_none, /* none */
	handle_condition_equal, /* equal */
	handle_condition_not_equal, /* not equal */
	handle_condition_less, /* less */
	handle_condition_less_equal, /* less equal */
	handle_condition_greater, /* greater */
	handle_condition_greater_equal, /* greater equal */
	handle_condition_range, /* range */
	handle_condition_not_range, /* not range */
	handle_condition_none, /* true */
};

int hw_add_contion(u64 addr, hw_bp_condition *cond)
{
	struct list_head *head = hw_get_rules(addr);
	struct hw_bp_trigger *rule = NULL;

	if (!head)
		return 0;

	rule = kzalloc(sizeof(*rule), GFP_KERNEL);
	if (!rule)
		return 0;

	rule->rule = *cond;
	rule->rule.bp_type = HW_BREAKPOINT_W; /*only support write now*/
	rule->magic = crc32(jiffies, &rule->rule, sizeof(rule->rule));

	hw_manage_lock();
	list_add_tail(&rule->list, head);
	hw_manage_unlock();

	return rule->magic;
}
EXPORT_SYMBOL_GPL(hw_add_contion);

void hw_del_contion(u64 addr, int magic)
{
	struct list_head *head = hw_get_rules(addr);
	struct hw_bp_trigger *rule = NULL, *next = NULL;

	if (!head)
		return;

	hw_manage_lock();
	list_for_each_entry_safe(rule, next, head, list) {
		if (rule->magic == magic) {
			list_del(&rule->list);
			kfree(rule);
			break;
		}
	}
	hw_manage_unlock();
}
EXPORT_SYMBOL_GPL(hw_del_contion);

void hw_del_all_contion(u64 addr)
{
	struct list_head *head = hw_get_rules(addr);
	struct hw_bp_trigger *rule = NULL, *next = NULL;

	if (!head)
		return;

	hw_manage_lock();
	list_for_each_entry_safe(rule, next, head, list) {
		list_del(&rule->list);
		kfree(rule);
	}
	hw_manage_unlock();
}

static void hw_show_contion(hw_bp_trigger *rule)
{
	hw_bp_condition *cond = &rule->rule;

	pr_info("rule[0x%x]:", rule->magic);
	pr_info("\tmask: 0x%llx", cond->mask);
	pr_info("\tvalue0: 0x%llx", cond->condition_value[0]);
	pr_info("\tvalue2: 0x%llx", cond->condition_value[1]);
	pr_info("\tcondition: %s", condition_type_str[cond->condition]);
	pr_info("\tvalue_type: %s", value_type_str[cond->value_type]);
}

void hw_show_all_contion(u64 addr)
{
	struct list_head *head = hw_get_rules(addr);
	struct hw_bp_trigger *rule = NULL, *next = NULL;

	if (!head)
		return;

	list_for_each_entry_safe(rule, next, head, list) {
		hw_show_contion(rule);
	}
}

bool hw_check_contion(struct list_head *head, const struct pt_regs *regs,
		      const hw_bp_value *value, u64 access)
{
	struct hw_bp_trigger *rule = NULL, *next = NULL;
	u64 tmp = 0;
	bool ret = true;

	if (!head)
		return true;

	list_for_each_entry_safe(rule, next, head, list) {
		hw_show_contion(rule);
		if (rule->rule.bp_type != access) {
			continue;
		}
		switch (rule->rule.value_type) {
		case VALUE_TYPE_OLD: {
			if (value->old_flag) {
				continue;
			}
			tmp = value->old;
			break;
		}
		case VALUE_TYPE_NEW: {
			if (value->new_flag) {
				continue;
			}
			tmp = value->new;
			break;
		}
		case VALUE_TYPE_PC: {
			tmp = regs->pc;
			break;
		}
		case VALUE_TYPE_SP: {
			tmp = regs->sp;
			break;
		}
		case VALUE_TYPE_X30 ... VALUE_TYPE_X0: {
			tmp = regs->regs[30 - (rule->rule.value_type -
					       VALUE_TYPE_X30)];
			break;
		}
		default: {
			continue;
		}
		}
		if (rule->rule.condition >= CONDITION_TYPE_MAX ||
		    rule->rule.condition < 0) {
			continue;
		}
		ret = gcontion_callback[rule->rule.condition](
			tmp, rule->rule.mask, rule->rule.condition_value[0],
			rule->rule.condition_value[1]);
		if (!ret) {
			break;
		}
	}

	return ret;
}
