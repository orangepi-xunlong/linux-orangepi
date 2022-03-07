/*
 * Copyright (C) 2018 Spreadtrum Communications Inc.
 *
 * File:		wcn_misc.c
 * Description:	WCN misc file for drivers. Some feature or function
 * isn't easy to classify, then write it in this file.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the	1
 * GNU General Public License for more details.
 */

#include <linux/mutex.h>
#include <linux/version.h>
#include <linux/time.h>
#if KERNEL_VERSION(4, 14, 0) <= LINUX_VERSION_CODE
#include <linux/sched/clock.h>
#endif

#include "wcn_misc.h"
#include "wcn_procfs.h"
#include "wcn_txrx.h"
#include "mdbg_type.h"

static struct atcmd_fifo s_atcmd_owner;
static unsigned long int s_marlin_bootup_time;

void mdbg_atcmd_owner_init(void)
{
	memset(&s_atcmd_owner, 0, sizeof(s_atcmd_owner));
	mutex_init(&s_atcmd_owner.lock);
}

void mdbg_atcmd_owner_deinit(void)
{
	mutex_destroy(&s_atcmd_owner.lock);
}

static void mdbg_atcmd_owner_add(enum atcmd_owner owner)
{
	mutex_lock(&s_atcmd_owner.lock);
	s_atcmd_owner.owner[s_atcmd_owner.tail % ATCMD_FIFO_MAX] = owner;
	s_atcmd_owner.tail++;
	mutex_unlock(&s_atcmd_owner.lock);
}

enum atcmd_owner mdbg_atcmd_owner_peek(void)
{
	enum atcmd_owner owner;

	mutex_lock(&s_atcmd_owner.lock);
	owner = s_atcmd_owner.owner[s_atcmd_owner.head % ATCMD_FIFO_MAX];
	s_atcmd_owner.head++;
	mutex_unlock(&s_atcmd_owner.lock);

	WCN_DEBUG("owner=%d, head=%d\n", owner, s_atcmd_owner.head-1);
	return owner;
}

void mdbg_atcmd_clean(void)
{
	mutex_lock(&s_atcmd_owner.lock);
	memset(&s_atcmd_owner.owner[0], 0, ARRAY_SIZE(s_atcmd_owner.owner));
	s_atcmd_owner.head = s_atcmd_owner.tail = 0;
	mutex_unlock(&s_atcmd_owner.lock);
}

/*
 * Until now, CP2 response every AT CMD to AP side
 * without owner-id.AP side transfer every ATCMD
 * response info to WCND.If AP send AT CMD on kernel layer,
 * and the response info transfer to WCND,
 * WCND deal other owner's response CMD.
 * We'll not modify CP2 codes because some
 * products already released to customer.
 * We will save all of the owner-id to the atcmd fifo.
 * and dispatch the response ATCMD info to the matched owner.
 * We'd better send all of the ATCMD with this function
 * or caused WCND error
 */
long int mdbg_send_atcmd(char *buf, long int len, enum atcmd_owner owner)
{
	long int sent_size = 0;

	mdbg_atcmd_owner_add(owner);

	/* confirm write finish */
	mutex_lock(&s_atcmd_owner.lock);
	sent_size = mdbg_send(buf, len, MDBG_SUBTYPE_AT);
	mutex_unlock(&s_atcmd_owner.lock);

	WCN_DEBUG("%s, owner=%d\n", buf, owner);

	return sent_size;
}

/*
 * Only marlin poweron and marlin starts to run,
 * it can call this function.
 * The time will be sent to marlin with loopcheck CMD.
 * NOTES:If marlin power off, and power on again, it
 * should call this function again.
 */
void marlin_bootup_time_update(void)
{
	s_marlin_bootup_time = local_clock();
	WCN_INFO("s_marlin_bootup_time=%ld",
		s_marlin_bootup_time);
}

unsigned long int marlin_bootup_time_get(void)
{
	return s_marlin_bootup_time;
}

