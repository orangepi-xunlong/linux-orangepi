/*
 * dsm_test.c
 *
 *the file is for dsm test
 *
 * Copyright (c) 2015-2019 Huawei Technologies Co., Ltd.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 */

#include <linux/module.h>
#include <linux/kthread.h>
#include <linux/random.h>
#include <uapi/linux/sched/types.h>
#include <linux/soc/cix/dsm_pub.h>
#include "dsm_core.h"

#define dsm_log_info(x...)	pr_info(x)
#define dsm_log_err(x...) pr_err(x)
#define dsm_log_debug(x...)	pr_debug(x)

#define DSM_TEST_BUFF		16
#define DSM_TEST_DEVICE_SUM	5
#define DEV_ONE_BUFF_SIZE	500
#define DEV_TWO_BUFF_SIZE	1024
#define DEV_THREE_BUFF_SIZE	10240
#define DEV_FOUR_BUFF_SIZE	300
#define DEV_FIVE_BUFF_SIZE	500
#define DSM_RANDOM_ODD_RESULT	0
#define DSM_RANDOM_EVEN_RESULT	5

int poll(void)
{
	unsigned int random;
	int result;

	random = get_random_u32();
	result = (random % 2) ? DSM_RANDOM_ODD_RESULT : DSM_RANDOM_EVEN_RESULT;
	dsm_log_info("%s enter random num %u, result %d\n",
		     __func__, random, result);
	return result;
}

int dump(int type, void *buff, int size)
{
	int used_size = 0;
	char *ptr = buff;

	dsm_log_info("%s called, type %d\n", __func__, type);
	used_size += snprintf(ptr, size - used_size, "%s called:\n", __func__);
	ptr = buff + used_size;
	used_size += snprintf(ptr, size - used_size, "Today is saturday\n");
	ptr = buff + used_size;
	used_size += snprintf(ptr, size - used_size, "Tempeature is 10 degree\n");
	ptr = buff + used_size;
	used_size += snprintf(ptr, size - used_size, "Wheather is sunny\n");
	ptr = buff + used_size;
	used_size += snprintf(ptr, size - used_size, "Fit to do sports or have a picnic\n");

	return used_size;
}

struct dsm_client_ops ops = {
	.poll_state = poll,
	.dump_func = dump,
};

struct dsm_dev dev1 = {
	.name = "test_dev_1",
	.device_name = NULL,
	.ic_name = NULL,
	.module_name = NULL,
	.fops = &ops,
	.buff_size = DEV_ONE_BUFF_SIZE,
};

struct dsm_dev dev2 = {
	.name = "test_dev_2",
	.device_name = NULL,
	.ic_name = NULL,
	.module_name = NULL,
	.fops = &ops,
	.buff_size = DEV_TWO_BUFF_SIZE,
};

struct dsm_dev dev3 = {
	.name = "test_dev_3",
	.device_name = NULL,
	.ic_name = NULL,
	.module_name = NULL,
	.fops = &ops,
	.buff_size = DEV_THREE_BUFF_SIZE,
};

struct dsm_dev dev4 = {
	.name = "test_dev_4",
	.device_name = NULL,
	.ic_name = NULL,
	.module_name = NULL,
	.fops = &ops,
	.buff_size = DEV_FOUR_BUFF_SIZE,
};

struct dsm_dev dev5 = {
	.name = "test_dev_5",
	.device_name = NULL,
	.ic_name = NULL,
	.module_name = NULL,
	.fops = &ops,
	.buff_size = DEV_FIVE_BUFF_SIZE,
};

struct dsm_client *c1;
struct dsm_client *c2;
struct dsm_client *c3;
struct dsm_client *c4;
struct dsm_client *c5;

static u8 test_buff[DSM_TEST_BUFF] = {'a', 'b', 'c', 'd',
				      'e', 'f', 'g', 'h',
				      'i', 'j', 'k', 'l',
				      'm', 'n', 'o', '\n'};

static void test_copy(struct dsm_client *client, int count)
{
	while (count / DSM_TEST_BUFF) {
		dsm_client_copy(client, test_buff, DSM_TEST_BUFF);
		count -= DSM_TEST_BUFF;
	}
	if (count)
		dsm_client_copy(client, test_buff, count);
}

void notify_work(void)
{
	struct dsm_client *curr = NULL;
	unsigned int random;
	int result;

	random = get_random_u32();
	result = random % DSM_TEST_DEVICE_SUM;
	dsm_log_info("%s enter random num %u, result %d\n",
		     __func__, random, result);
	switch (result) {
	case 0:
		goto notify_1;
		/* go through down */
	case 1:
		goto notify_2;
		/* go through down */
	case 2:
		goto notify_3;
		/* go trrough down */
	case 3:
		goto notify_4;
		/* go through down */
	case 4:
		goto notify_5;
		/* go through down */
	default:
		dsm_log_info("%s out , finish %d dsm_dev notify\n",
			     __func__, result);
	}

notify_5:
	curr = c1;
	if (!dsm_client_ocuppy(curr)) {
		test_copy(curr, DEV_ONE_BUFF_SIZE);
		dsm_client_notify(curr, DSM_TEST_DEVICE_SUM);
		dsm_log_info("%s dsm_client_notify\n", curr->client_name);
	}

notify_4:
	curr = c2;
	if (!dsm_client_ocuppy(curr)) {
		test_copy(curr, DEV_TWO_BUFF_SIZE);
		dsm_client_notify(curr, DSM_TEST_DEVICE_SUM);
		dsm_log_info("%s dsm_client_notify\n", curr->client_name);
	}

notify_3:
	curr = c3;
	if (!dsm_client_ocuppy(curr)) {
		test_copy(curr, DEV_THREE_BUFF_SIZE);
		dsm_client_notify(curr, DSM_TEST_DEVICE_SUM);
		dsm_log_info("%s dsm_client_notify\n", curr->client_name);
	}

notify_2:
	curr = c4;
	if (!dsm_client_ocuppy(curr)) {
		dsm_client_record(curr, "Kaka is an amazing palyer\n");
		dsm_client_record(curr, "Ronaldo is an amazing palyer\n");
		dsm_client_record(curr, "Ronney is an amazing palyer\n");
		dsm_client_record(curr, "Henny is an amazing palyer\n");
		dsm_client_record(curr, "Veri is an amazing palyer\n");
		dsm_client_notify(curr, DSM_TEST_DEVICE_SUM);
		dsm_log_info("%s dsm_client_notify\n", curr->client_name);
	}

notify_1:
	curr = c5;
	if (!dsm_client_ocuppy(curr)) {
		dsm_client_record(curr, "National derby:\n");
		dsm_client_record(curr, "Real Madrid vs  Barcelona\n");
		dsm_client_record(curr, "Manchester United vs Chelsea\n");
		dsm_client_record(curr, "AC milan vs Juventus\n");
		dsm_client_record(curr, "Bayern Munich vs Hertha BSC\n");
		dsm_client_notify(curr, DSM_TEST_DEVICE_SUM);
		dsm_log_info("%s dsm_client_notify\n", curr->client_name);
	}
}

static int dsm_test_thread(void *data)
{
	static const struct sched_param param = {
		.sched_priority = MAX_RT_PRIO / 2,
	};

	sched_setscheduler(current, SCHED_FIFO, &param);

	while (!kthread_should_stop()) {
		set_current_state(TASK_INTERRUPTIBLE);
		dsm_log_info("%s sleep in\n", __func__);
		schedule_timeout(2 * HZ);
		dsm_log_info("%s sleep out\n", __func__);
		set_current_state(TASK_RUNNING);
		notify_work();
	}
	return 0;
}

struct task_struct *dsm_task;

static int __init dsm_test(void)
{
	c1 = dsm_register_client(&dev1);
	if (!c1)
		dsm_log_err("dsm1 reg failed\n");

	c2 = dsm_register_client(&dev2);
	if (!c2)
		dsm_log_err("dsm2 reg failed\n");

	c3 = dsm_register_client(&dev3);
	if (!c3)
		dsm_log_err("dsm3 reg failed\n");

	c4 = dsm_register_client(&dev4);
	if (!c4)
		dsm_log_err("dsm4 reg failed\n");

	c5 = dsm_register_client(&dev5);
	if (!c5)
		dsm_log_err("dsm5 reg failed\n");

	dsm_task = kthread_create(dsm_test_thread, NULL, "dsm_thread");
	wake_up_process(dsm_task);

	return 0;
}

module_init(dsm_test);
MODULE_LICENSE("GPL v2");
