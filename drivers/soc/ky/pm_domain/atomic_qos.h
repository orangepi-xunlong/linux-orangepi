#ifndef __ATOMIC_QOS_H__
#define __ATOMIC_QOS_H__

#include <linux/plist.h>
#include <linux/pm_qos.h>
#include <linux/notifier.h>
#include <linux/err.h>
#include <linux/spinlock_types.h>

/*
 * Note: The lockless read path depends on the CPU accessing target_value
 * or effective_flags atomically.  Atomic access is only guaranteed on all CPU
 * types linux supports for 32 bit quantites
 */
struct atomic_pm_qos_constraints {
	struct plist_head list;
	s32 target_value;	/* Do not change to 64 bit */
	s32 default_value;
	s32 no_constraint_value;
	enum pm_qos_type type;
	struct atomic_notifier_head *notifiers;
};

struct atomic_freq_constraints {
	struct atomic_pm_qos_constraints min_freq;
	struct atomic_notifier_head min_freq_notifiers;
	struct atomic_pm_qos_constraints max_freq;
	struct atomic_notifier_head max_freq_notifiers;
};

struct atomic_freq_qos_request {
	enum freq_qos_req_type type;
	struct plist_node pnode;
	struct atomic_freq_constraints *qos;
};

/**
 * atomic_freq_constraints_init - Initialize frequency QoS constraints.
 * @qos: Frequency QoS constraints to initialize.
 */
void atomic_freq_constraints_init(struct atomic_freq_constraints *qos);

/**
 * atomic_freq_qos_add_notifier - Add frequency QoS change notifier.
 * @qos: List of requests to add the notifier to.
 * @type: Request type.
 * @notifier: Notifier block to add.
 */
int atomic_freq_qos_add_notifier(struct atomic_freq_constraints *qos,
			  enum freq_qos_req_type type,
			  struct notifier_block *notifier);

/**
 * atomic_freq_qos_remove_notifier - Remove frequency QoS change notifier.
 * @qos: List of requests to remove the notifier from.
 * @type: Request type.
 * @notifier: Notifier block to remove.
 */
int atomic_freq_qos_remove_notifier(struct atomic_freq_constraints *qos,
			     enum freq_qos_req_type type,
			     struct notifier_block *notifier);
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
			 enum freq_qos_req_type type, s32 value);
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
int atomic_freq_qos_update_request(struct atomic_freq_qos_request *req, s32 new_value);

int atomic_freq_qos_remove_request(struct atomic_freq_qos_request *req);

#endif

