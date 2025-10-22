/* SPDX-License-Identifier: GPL-2.0-or-later */
/*******************************************************************************
--                                                                            --
--                    CedarX Multimedia Framework                             --
--                                                                            --
--          the Multimedia Framework for Linux/Android System                 --
--                                                                            --
--       This software is confidential and proprietary and may be used        --
--        only as expressly authorized by a licensing agreement from          --
--                         Softwinner Products.                               --
--                                                                            --
--                   (C) COPYRIGHT 2011 SOFTWINNER PRODUCTS                   --
--                            ALL RIGHTS RESERVED                             --
--                                                                            --
--                 The entire notice above must be reproduced                 --
--                  on all copies and should not be removed.                  --
--                                                                            --
*******************************************************************************/
#include <linux/module.h>
#include <linux/types.h>
#include <linux/mutex.h>

#define LOG_TAG "rt_message"


#include "rt_message.h"
#include "rt_common.h"

static int TMessageDeepCopyMessage(message_t *pDesMsg, message_t *pSrcMsg)
{
	pDesMsg->command = pSrcMsg->command;
	pDesMsg->para0 = pSrcMsg->para0;
	pDesMsg->para1 = pSrcMsg->para1;
	pDesMsg->wait_queue     = pSrcMsg->wait_queue;
	pDesMsg->wait_condition = pSrcMsg->wait_condition;
	return 0;
}

static int TMessageSetMessage(message_t *pDesMsg, message_t *pSrcMsg)
{
	pDesMsg->command = pSrcMsg->command;
	pDesMsg->para0 = pSrcMsg->para0;
	pDesMsg->para1 = pSrcMsg->para1;
	pDesMsg->wait_queue = pSrcMsg->wait_queue;
	pDesMsg->wait_condition = pSrcMsg->wait_condition;
	return 0;
}

static int TMessageIncreaseIdleMessageList(message_queue_t *pThiz)
{
	int ret = 0;
	message_t   *pMsg;
	int i;
	for (i = 0; i < MAX_MESSAGE_ELEMENTS; i++) {
		pMsg = kmalloc(sizeof(message_t), GFP_KERNEL);
		if (NULL == pMsg) {
			RT_LOGE(" fatal error! kmalloc fail");
			ret = -1;
			break;
		}
		list_add_tail(&pMsg->mList, &pThiz->mIdleMessageList);
	}
	return ret;
}

int message_create(message_queue_t *msg_queue)
{
	RT_LOGI("msg create\n");

	mutex_init(&msg_queue->mutex);
	msg_queue->mWaitMessageFlag = 0;
	init_waitqueue_head(&msg_queue->wait_msg);

	INIT_LIST_HEAD(&msg_queue->mIdleMessageList);
	INIT_LIST_HEAD(&msg_queue->mReadyMessageList);
	if (0 != TMessageIncreaseIdleMessageList(msg_queue)) {
		goto _err1;
	}
	msg_queue->message_count = 0;

	return 0;
_err1:
	return -1;
}


void message_destroy(message_queue_t *msg_queue)
{
	int cnt = 0;

	mutex_lock(&msg_queue->mutex);
	if (!list_empty(&msg_queue->mReadyMessageList)) {
		message_t *pEntry, *pTmp;
		list_for_each_entry_safe(pEntry, pTmp, &msg_queue->mReadyMessageList, mList) {
			RT_LOGD(" msg destroy: cmd[%x]", pEntry->command);
			list_move_tail(&pEntry->mList, &msg_queue->mIdleMessageList);
			msg_queue->message_count--;
		}
	}
	if (msg_queue->message_count != 0) {
		RT_LOGE(" fatal error! msg count[%d]!=0", msg_queue->message_count);
	}
	if (!list_empty(&msg_queue->mIdleMessageList)) {
		message_t   *pEntry, *pTmp;
		list_for_each_entry_safe(pEntry, pTmp, &msg_queue->mIdleMessageList, mList) {
			list_del(&pEntry->mList);
			kfree(pEntry);
			cnt++;
		}
	}
	INIT_LIST_HEAD(&msg_queue->mIdleMessageList);
	INIT_LIST_HEAD(&msg_queue->mReadyMessageList);
	mutex_unlock(&msg_queue->mutex);
}


void flush_message(message_queue_t *msg_queue)
{
	mutex_lock(&msg_queue->mutex);
	if (!list_empty(&msg_queue->mReadyMessageList)) {
		message_t   *pEntry, *pTmp;
		list_for_each_entry_safe(pEntry, pTmp, &msg_queue->mReadyMessageList, mList) {
			RT_LOGD(" msg destroy: cmd[%x]", pEntry->command);

			list_move_tail(&pEntry->mList, &msg_queue->mIdleMessageList);
			msg_queue->message_count--;
		}
	}
	if (msg_queue->message_count != 0) {
		RT_LOGE(" fatal error! msg count[%d]!=0", msg_queue->message_count);
	}
	mutex_unlock(&msg_queue->mutex);
}

/*******************************************************************************
Function name: put_message
Description:
    Do not accept mpData when call this function.
Parameters:

Return:

Time: 2015/3/6
*******************************************************************************/
int put_message(message_queue_t *msg_queue, message_t *msg_in)
{
	message_t message;
	memset(&message, 0, sizeof(message_t));
	message.command = msg_in->command;
	message.para0 = msg_in->para0;
	message.para1 = msg_in->para1;
	message.wait_queue = msg_in->wait_queue;
	message.wait_condition = msg_in->wait_condition;
	return putMessageWithData(msg_queue, &message);
}

int get_message(message_queue_t *msg_queue, message_t *msg_out)
{
	message_t   *pMessageEntry = NULL;

	mutex_lock(&msg_queue->mutex);

	if (list_empty(&msg_queue->mReadyMessageList)) {
		mutex_unlock(&msg_queue->mutex);
		return -1;
	}
	pMessageEntry = list_first_entry(&msg_queue->mReadyMessageList, message_t, mList);
	TMessageSetMessage(msg_out, pMessageEntry);
	list_move_tail(&pMessageEntry->mList, &msg_queue->mIdleMessageList);
	msg_queue->message_count--;

	mutex_unlock(&msg_queue->mutex);
	return 0;
}

int putMessageWithData(message_queue_t *msg_queue, message_t *msg_in)
{
	int ret = 0;
	message_t   *pMessageEntry = NULL;
	mutex_lock(&msg_queue->mutex);
	if (list_empty(&msg_queue->mIdleMessageList)) {
		RT_LOGW(" idleMessageList are all used, kmalloc more!");
		if (0 != TMessageIncreaseIdleMessageList(msg_queue)) {
			mutex_unlock(&msg_queue->mutex);
			return -1;
		}
	}
	pMessageEntry = list_first_entry(&msg_queue->mIdleMessageList, message_t, mList);
	if (0 == TMessageDeepCopyMessage(pMessageEntry, msg_in)) {
		list_move_tail(&pMessageEntry->mList, &msg_queue->mReadyMessageList);
		msg_queue->message_count++;
		RT_LOGI(" new msg command[%d], para[%d][%d]",
		pMessageEntry->command, pMessageEntry->para0, pMessageEntry->para1);
		if (msg_queue->mWaitMessageFlag) {
			msg_queue->wait_condition = 1;
			wake_up(&msg_queue->wait_msg);
		}
	} else {
		ret = -1;
	}
    mutex_unlock(&msg_queue->mutex);
	return ret;
}

int get_message_count(message_queue_t *msg_queue)
{
	int message_count;

	mutex_lock(&msg_queue->mutex);
	message_count = msg_queue->message_count;
	mutex_unlock(&msg_queue->mutex);

	return message_count;
}

int TMessage_WaitQueueNotEmpty(message_queue_t *msg_queue, unsigned int timeout)
{
	int message_count;

	mutex_lock(&msg_queue->mutex);
	msg_queue->mWaitMessageFlag = 1;
	msg_queue->wait_condition = 0;
	mutex_unlock(&msg_queue->mutex);
	if (timeout <= 0) {
		if (list_empty(&msg_queue->mReadyMessageList)) {
			wait_event(msg_queue->wait_msg, msg_queue->wait_condition);
		}
	} else {
		if (list_empty(&msg_queue->mReadyMessageList)) {
			wait_event_timeout(msg_queue->wait_msg, msg_queue->wait_condition, timeout*HZ/1000);
		}
	}
	mutex_lock(&msg_queue->mutex);
	msg_queue->mWaitMessageFlag = 0;
	message_count = msg_queue->message_count;
	mutex_unlock(&msg_queue->mutex);

	return message_count;
}

