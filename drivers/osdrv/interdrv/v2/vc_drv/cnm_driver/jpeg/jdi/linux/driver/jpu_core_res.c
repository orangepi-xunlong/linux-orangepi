#include <linux/list.h>
#include <linux/mutex.h>
#include <linux/sched.h>
#include <linux/wait.h>
#include <linux/slab.h>
#include <linux/spinlock.h>
#include <linux/delay.h>


typedef struct jpuvdrv_core_list_t {
    int id;
    bool is_used;
    struct list_head list;
} jpuvdrv_core_list_t;

static unsigned int s_max_num_core = 4;
//static spinlock_t jpeg_spinlock;

static DEFINE_SPINLOCK(jpeg_spinlock);

static DEFINE_MUTEX(jpuvdrv_core_list_lock);

static DECLARE_WAIT_QUEUE_HEAD(jpuvdrv_core_wait_queue);
static LIST_HEAD(jpuvdrv_core_resource_list_head);
static int next_id = 0;
static bool isCoreIdle = false;
int jpu_core_request_resource(int timeout) {
    jpuvdrv_core_list_t *res;
    unsigned long flags;
    int id = -1;
    unsigned long long elapse, cur;
    struct timespec64 ts;

    ktime_get_ts64(&ts);
    elapse = ts.tv_sec * 1000 + ts.tv_nsec/1000000;

    while (id == -1) {
        spin_lock_irqsave(&jpeg_spinlock, flags);
        list_for_each_entry(res, &jpuvdrv_core_resource_list_head, list) {
            if (!res->is_used) {
                res->is_used = true;
                id = res->id;
                break;
            }
        }
        isCoreIdle = false;
        spin_unlock_irqrestore(&jpeg_spinlock, flags);

        if (id == -1) {
            if (timeout > 0 )
                wait_event_timeout(jpuvdrv_core_wait_queue, isCoreIdle, msecs_to_jiffies(timeout));
            else
                wait_event(jpuvdrv_core_wait_queue, isCoreIdle);
        }

        ktime_get_ts64(&ts);
        cur = ts.tv_sec * 1000 + ts.tv_nsec/1000000;

        if (timeout > 0 && (cur - elapse) > timeout) {
            return -1;
        }
    }

    return id;
}

int jpu_core_release_resource(int id) {
    jpuvdrv_core_list_t *res;
    unsigned long		flags;
    int ret = -1;

    spin_lock_irqsave(&jpeg_spinlock, flags);
    list_for_each_entry(res, &jpuvdrv_core_resource_list_head, list) {
        if (res->id == id) {
            res->is_used = false;
            ret = 0;
            isCoreIdle = true;
            wake_up(&jpuvdrv_core_wait_queue);
            break;
        }
    }
    spin_unlock_irqrestore(&jpeg_spinlock, flags);

    return ret;
}

int jpu_core_init_resources(unsigned int core_num) {
    jpuvdrv_core_list_t *res;
    int i;

    s_max_num_core = core_num;
    for (i = 0; i < s_max_num_core; i++) {
        res = kzalloc(sizeof(*res), GFP_KERNEL);
        if (!res) {
            printk(KERN_ERR "Failed to allocate memory for resource\n");
            return -ENOMEM;
        }

        res->id = next_id++;
        res->is_used = false;
        INIT_LIST_HEAD(&res->list);

        mutex_lock(&jpuvdrv_core_list_lock);
        list_add_tail(&res->list, &jpuvdrv_core_resource_list_head);
        mutex_unlock(&jpuvdrv_core_list_lock);
    }

    return 0;
}

void jpu_core_cleanup_resources(void) {
    jpuvdrv_core_list_t *res, *tmp;

    mutex_lock(&jpuvdrv_core_list_lock);
    list_for_each_entry_safe(res, tmp, &jpuvdrv_core_resource_list_head, list) {
        list_del(&res->list);
        kfree(res);
    }
    mutex_unlock(&jpuvdrv_core_list_lock);
}

