#include <linux/plist.h>
#include <linux/pm_qos.h>
#include <linux/notifier.h>
#include <linux/err.h>
#include <linux/spinlock_types.h>
#include "atomic_qos.h"

/*
 * locking rule: all changes to constraints or notifiers lists
 * or pm_qos_object list and pm_qos_objects need to happen with pm_qos_lock
 * held, taken with _irqsave.  One lock to rule them all
 */
static DEFINE_SPINLOCK(atomic_pm_qos_lock);

/**
 * atomic_freq_constraints_init - Initialize frequency QoS constraints.
 * @qos: Frequency QoS constraints to initialize.
 */
void atomic_freq_constraints_init(struct atomic_freq_constraints *qos)
{
	struct atomic_pm_qos_constraints *c;

	c = &qos->min_freq;
	plist_head_init(&c->list);
	c->target_value = FREQ_QOS_MIN_DEFAULT_VALUE;
	c->default_value = FREQ_QOS_MIN_DEFAULT_VALUE;
	c->no_constraint_value = FREQ_QOS_MIN_DEFAULT_VALUE;
	c->type = PM_QOS_MAX;
	c->notifiers = &qos->min_freq_notifiers;
	ATOMIC_INIT_NOTIFIER_HEAD(c->notifiers);

	c = &qos->max_freq;
	plist_head_init(&c->list);
	c->target_value = FREQ_QOS_MAX_DEFAULT_VALUE;
	c->default_value = FREQ_QOS_MAX_DEFAULT_VALUE;
	c->no_constraint_value = FREQ_QOS_MAX_DEFAULT_VALUE;
	c->type = PM_QOS_MIN;
	c->notifiers = &qos->max_freq_notifiers;
	ATOMIC_INIT_NOTIFIER_HEAD(c->notifiers);
}


/**
 * atomic_freq_qos_add_notifier - Add frequency QoS change notifier.
 * @qos: List of requests to add the notifier to.
 * @type: Request type.
 * @notifier: Notifier block to add.
 */
int atomic_freq_qos_add_notifier(struct atomic_freq_constraints *qos,
			  enum freq_qos_req_type type,
			  struct notifier_block *notifier)
{
	int ret;

	if (IS_ERR_OR_NULL(qos) || !notifier)
		return -EINVAL;

	switch (type) {
	case FREQ_QOS_MIN:
		ret = atomic_notifier_chain_register(qos->min_freq.notifiers, notifier);
		break;
	case FREQ_QOS_MAX:
		ret = atomic_notifier_chain_register(qos->max_freq.notifiers, notifier);
		break;
	default:
		WARN_ON(1);
		ret = -EINVAL;
		break;
	}

	return ret;
}

/**
 * atomic_freq_qos_remove_notifier - Remove frequency QoS change notifier.
 * @qos: List of requests to remove the notifier from.
 * @type: Request type.
 * @notifier: Notifier block to remove.
 */
int atomic_freq_qos_remove_notifier(struct atomic_freq_constraints *qos,
			     enum freq_qos_req_type type,
			     struct notifier_block *notifier)
{
	int ret;

	if (IS_ERR_OR_NULL(qos) || !notifier)
		return -EINVAL;

	switch (type) {
	case FREQ_QOS_MIN:
		ret = atomic_notifier_chain_unregister(qos->min_freq.notifiers, notifier);
		break;
	case FREQ_QOS_MAX:
		ret = atomic_notifier_chain_unregister(qos->max_freq.notifiers, notifier);
		break;
	default:
		WARN_ON(1);
		ret = -EINVAL;
		break;
	}

	return ret;
}

static int pm_qos_get_value(struct atomic_pm_qos_constraints *c)
{
	if (plist_head_empty(&c->list))
		return c->no_constraint_value;

	switch (c->type) {
	case PM_QOS_MIN:
		return plist_first(&c->list)->prio;

	case PM_QOS_MAX:
		return plist_last(&c->list)->prio;

	default:
		WARN(1, "Unknown PM QoS type in %s\n", __func__);
		return PM_QOS_DEFAULT_VALUE;
	}
}

static void pm_qos_set_value(struct atomic_pm_qos_constraints *c, s32 value)
{
	WRITE_ONCE(c->target_value, value);
}

/**
 * pm_qos_update_target - Update a list of PM QoS constraint requests.
 * @c: List of PM QoS requests.
 * @node: Target list entry.
 * @action: Action to carry out (add, update or remove).
 * @value: New request value for the target list entry.
 *
 * Update the given list of PM QoS constraint requests, @c, by carrying an
 * @action involving the @node list entry and @value on it.
 *
 * The recognized values of @action are PM_QOS_ADD_REQ (store @value in @node
 * and add it to the list), PM_QOS_UPDATE_REQ (remove @node from the list, store
 * @value in it and add it to the list again), and PM_QOS_REMOVE_REQ (remove
 * @node from the list, ignore @value).
 *
 * Return: 1 if the aggregate constraint value has changed, 0  otherwise.
 */
static int atomic_pm_qos_update_target(struct atomic_pm_qos_constraints *c, struct plist_node *node,
			 enum pm_qos_req_action action, int value)
{
	int prev_value, curr_value, new_value;
	unsigned long flags;

	spin_lock_irqsave(&atomic_pm_qos_lock, flags);

	prev_value = pm_qos_get_value(c);
	if (value == PM_QOS_DEFAULT_VALUE)
		new_value = c->default_value;
	else
		new_value = value;

	switch (action) {
	case PM_QOS_REMOVE_REQ:
		plist_del(node, &c->list);
		break;
	case PM_QOS_UPDATE_REQ:
		/*
		 * To change the list, atomically remove, reinit with new value
		 * and add, then see if the aggregate has changed.
		 */
		plist_del(node, &c->list);
		fallthrough;
	case PM_QOS_ADD_REQ:
		plist_node_init(node, new_value);
		plist_add(node, &c->list);
		break;
	default:
		/* no action */
		break;
	}

	curr_value = pm_qos_get_value(c);
	pm_qos_set_value(c, curr_value);

	spin_unlock_irqrestore(&atomic_pm_qos_lock, flags);

	if (prev_value == curr_value)
		return 0;

	if (c->notifiers)
		atomic_notifier_call_chain(c->notifiers, curr_value, NULL);

	return 1;
}

/**
 * atomic_freq_qos_apply - Add/modify/remove frequency QoS request.
 * @req: Constraint request to apply.
 * @action: Action to perform (add/update/remove).
 * @value: Value to assign to the QoS request.
 *
 * This is only meant to be called from inside pm_qos, not drivers.
 */
static int atomic_freq_qos_apply(struct atomic_freq_qos_request *req,
			  enum pm_qos_req_action action, s32 value)
{
	int ret;

	switch(req->type) {
	case FREQ_QOS_MIN:
		ret = atomic_pm_qos_update_target(&req->qos->min_freq, &req->pnode,
					   action, value);
		break;
	case FREQ_QOS_MAX:
		ret = atomic_pm_qos_update_target(&req->qos->max_freq, &req->pnode,
					   action, value);
		break;
	default:
		ret = -EINVAL;
	}

	return ret;
}

/**
 * atomic_freq_qos_add_request - Insert new frequency QoS request into a given list.
 * @qos: Constraints to update.
 * @req: Preallocated request object.
 * @type: Request type.
 * @value: Request value.
 *
 * Insert a new entry into the @qos list of requests, recompute the effective
 * QoS constraint value for that list and initialize the @req object.  The
 * caller needs to save that object for later use in updates and removal.
 *
 * Return 1 if the effective constraint value has changed, 0 if the effective
 * constraint value has not changed, or a negative error code on failures.
 */
int atomic_freq_qos_add_request(struct atomic_freq_constraints *qos,
			 struct atomic_freq_qos_request *req,
			 enum freq_qos_req_type type, s32 value)
{
	int ret;

	if (IS_ERR_OR_NULL(qos) || !req || value < 0)
		return -EINVAL;

	req->qos = qos;
	req->type = type;
	ret = atomic_freq_qos_apply(req, PM_QOS_ADD_REQ, value);
	if (ret < 0) {
		req->qos = NULL;
		req->type = 0;
	}

	return ret;
}

/**
 * atomic_freq_qos_update_request - Modify existing frequency QoS request.
 * @req: Request to modify.
 * @new_value: New request value.
 *
 * Update an existing frequency QoS request along with the effective constraint
 * value for the list of requests it belongs to.
 *
 * Return 1 if the effective constraint value has changed, 0 if the effective
 * constraint value has not changed, or a negative error code on failures.
 */
int atomic_freq_qos_update_request(struct atomic_freq_qos_request *req, s32 new_value)
{
	if (!req || new_value < 0)
		return -EINVAL;

	if (req->pnode.prio == new_value)
		return 0;

	return atomic_freq_qos_apply(req, PM_QOS_UPDATE_REQ, new_value);
}

static inline int atomic_freq_qos_request_active(struct atomic_freq_qos_request *req)
{
	return !IS_ERR_OR_NULL(req->qos);
}

/**
 * atomic_freq_qos_remove_request - Remove frequency QoS request from its list.
 * @req: Request to remove.
 *
 * Remove the given frequency QoS request from the list of constraints it
 * belongs to and recompute the effective constraint value for that list.
 *
 * Return 1 if the effective constraint value has changed, 0 if the effective
 * constraint value has not changed, or a negative error code on failures.
 */
int atomic_freq_qos_remove_request(struct atomic_freq_qos_request *req)
{
	int ret;

	if (!req)
		return -EINVAL;

	if (WARN(!atomic_freq_qos_request_active(req),
		"%s() called for unknown object\n", __func__))
		return -EINVAL;

	ret = atomic_freq_qos_apply(req, PM_QOS_REMOVE_REQ, PM_QOS_DEFAULT_VALUE);
	req->qos = NULL;
	req->type = 0;

	return ret;
}
EXPORT_SYMBOL_GPL(atomic_freq_qos_remove_request);
