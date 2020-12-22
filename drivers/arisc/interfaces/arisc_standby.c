/*
 *  drivers/arisc/interfaces/arisc_standby.c
 *
 * Copyright (c) 2012 Allwinner.
 * 2012-05-01 Written by sunny (sunny@allwinnertech.com).
 * 2012-10-01 Written by superm (superm@allwinnertech.com).
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "../arisc_i.h"
#include <asm/tlbflush.h>
#include <asm/cacheflush.h>
#include <mach/sys_config.h>

/* record super-standby wakeup event */
static unsigned long wakeup_event = 0;
static unsigned long dram_crc_error = 0;
static unsigned long dram_crc_total_count = 0;
static unsigned long dram_crc_error_count = 0;

/*
 * enter cpu idle.
 * @para:  parameter for enter cpu idle.
 *    para->flag: 0x01-clear pending, 0x10-enter cpuidle
 *    para->resume_addr: the address cpu0 will run when exit idle
 *
 * return: result, 0 - super standby successed,
 *                !0 - super standby failed;
 */
int arisc_enter_cpuidle(arisc_cb_t cb, void *cb_arg, struct sunxi_cpuidle_para *para)
{
	struct arisc_message *pmessage;	/* allocate a message frame */
	pmessage = arisc_message_allocate(0);
	if (pmessage == NULL) {
		ARISC_ERR("allocate message for super-standby request failed\n");
		return -ENOMEM;
	}

	/* initialize message */
	pmessage->type          = ARISC_CPUIDLE_ENTER_REQ;
	pmessage->cb.handler    = cb;
	pmessage->cb.arg        = cb_arg;
	pmessage->state         = ARISC_MESSAGE_INITIALIZED;
	pmessage->paras[0]      = para->flags;
	pmessage->paras[1]      = para->mpidr;
	arisc_hwmsgbox_send_message(pmessage, ARISC_SEND_MSG_TIMEOUT);
	return 0;
}
EXPORT_SYMBOL(arisc_enter_cpuidle);

int arisc_config_cpuidle(arisc_cb_t cb, void *cb_arg, struct sunxi_cpuidle_para *para)
{
	int result = 0;
	struct arisc_message *pmessage;

	/* allocate a message frame */
	pmessage = arisc_message_allocate(ARISC_MESSAGE_ATTR_HARDSYN);
	if (pmessage == NULL) {
		ARISC_WRN("allocate message failed\n");
		return -ENOMEM;
	}

	/* initialize message */
	pmessage->type          = ARISC_CPUIDLE_CFG_REQ;
	pmessage->cb.handler    = cb;
	pmessage->cb.arg        = cb_arg;
	pmessage->state         = ARISC_MESSAGE_INITIALIZED;
	pmessage->paras[0]      = para->flags;
	pmessage->paras[1]      = para->mpidr;

	/* send request message */
	arisc_hwmsgbox_send_message(pmessage, ARISC_SEND_MSG_TIMEOUT);
	if (pmessage->result) {
		ARISC_WRN("config cpuidle fail!\n");
		result = -EINVAL;
	}

	/* free allocated message */
	arisc_message_free(pmessage);

	return result;
}
EXPORT_SYMBOL(arisc_config_cpuidle);

/**
 * enter super standby.
 * @para:  parameter for enter normal standby.
 *
 * return: result, 0 - super standby successed,
 *                !0 - super standby failed;
 */
int arisc_standby_super(struct super_standby_para *para, arisc_cb_t cb, void *cb_arg)
{
	struct arisc_message *pmessage;

	/* allocate a message frame */
	pmessage = arisc_message_allocate(0);
	if (pmessage == NULL) {
		ARISC_ERR("allocate message for super-standby request failed\n");
		return -ENOMEM;
	}

	/* check super_standby_para size valid or not */
	if (sizeof(struct super_standby_para) > sizeof(pmessage->paras)) {
		ARISC_ERR("super-standby parameters number too long\n");
		return -EINVAL;
	}

	/* initialize message */
	if (para->pextended_standby == NULL) {
		pmessage->type       = ARISC_SSTANDBY_ENTER_REQ;
	} else {
		pmessage->type       = ARISC_ESSTANDBY_ENTER_REQ;
	}
	pmessage->cb.handler = cb;
	pmessage->cb.arg     = cb_arg;
	memcpy((void *)pmessage->paras, (const void *)para, sizeof(struct super_standby_para));
	pmessage->state      = ARISC_MESSAGE_INITIALIZED;

	/* send enter super-standby request to arisc */
	arisc_hwmsgbox_send_message(pmessage, ARISC_SEND_MSG_TIMEOUT);

	return 0;
}
EXPORT_SYMBOL(arisc_standby_super);


/**
 * query super-standby wakeup source.
 * @para:  point of buffer to store wakeup event informations.
 *
 * return: result, 0 - query successed,
 *                !0 - query failed;
 */
int arisc_query_wakeup_source(unsigned int *event)
{
	*event = wakeup_event;

	return 0;
}
EXPORT_SYMBOL(arisc_query_wakeup_source);

/*
 * query super-standby infoation.
 * @para:  point of array to store power states informations during sst.
 * @op: 0:read, 1:set
 *
 * return: result, 0 - query successed,
 *                !0 - query failed;
 */
int arisc_query_standby_power_cfg(struct standby_info_para *para)
{
	memcpy((void *)para, (void *)&arisc_powchk_back, sizeof(struct standby_info_para));

	return 0;
}
EXPORT_SYMBOL(arisc_query_standby_power_cfg);

/*
 * query super-standby infoation.
 * @para:  point of array to store power states informations during sst.
 * @op: 0:read, 1:set
 *
 * return: result, 0 - query successed,
 *                !0 - query failed;
 */
int arisc_query_set_standby_info(struct standby_info_para *para, arisc_rw_type_e op)
{
	struct arisc_message *pmsg;
	int result;

	/* allocate a message frame */
	pmsg = arisc_message_allocate(ARISC_MESSAGE_ATTR_HARDSYN);
	if (pmsg == NULL) {
		ARISC_ERR("allocate message for query standby info failed\n");
		return -ENOMEM;
	}

	/* check standby_info_para size valid or not */
	if (sizeof(struct standby_info_para) > sizeof(pmsg->paras)) {
		ARISC_ERR("standby info parameters number too long\n");
		return -EINVAL;
	}

	/* initialize message */
	pmsg->type       = ARISC_STANDBY_INFO_REQ;
	pmsg->cb.handler = NULL;
	pmsg->cb.arg     = NULL;
	pmsg->private = (void *)op;
	if (ARISC_WRITE == op) {
		memcpy((void *)pmsg->paras, (const void *)para, sizeof(struct standby_info_para));
		memcpy((void *)&arisc_powchk_back, (const void *)para, sizeof(struct standby_info_para));
	}
	pmsg->state      = ARISC_MESSAGE_INITIALIZED;

	/* send query sst info request to arisc */
	arisc_hwmsgbox_send_message(pmsg, ARISC_SEND_MSG_TIMEOUT);
	if (ARISC_READ == op)
		memcpy((void *)para, (void *)pmsg->paras, sizeof(struct standby_info_para));

	/* free message */
	result = pmsg->result;
	arisc_message_free(pmsg);

	return result;
}
EXPORT_SYMBOL(arisc_query_set_standby_info);

/*
 * query super-standby dram crc result.
 * @para:  point of buffer to store dram crc result informations.
 *
 * return: result, 0 - query successed,
 *                !0 - query failed;
 */
int arisc_query_dram_crc_result(unsigned int *perror, unsigned int *ptotal_count,
	unsigned int *perror_count)
{
	*perror = dram_crc_error;
	*ptotal_count = dram_crc_total_count;
	*perror_count = dram_crc_error_count;

	return 0;
}
EXPORT_SYMBOL(arisc_query_dram_crc_result);

int arisc_set_dram_crc_result(unsigned int error, unsigned int total_count,
	unsigned int error_count)
{
	dram_crc_error = error;
	dram_crc_total_count = total_count;
	dram_crc_error_count = error_count;

	return 0;
}
EXPORT_SYMBOL(arisc_set_dram_crc_result);

/**
 * notify arisc cpux restored.
 * @para:  none.
 *
 * return: result, 0 - notify successed, !0 - notify failed;
 */
int arisc_cpux_ready_notify(void)
{
	struct arisc_message *pmessage;

	/* notify hwspinlock and hwmsgbox resume first */
	arisc_hwmsgbox_standby_resume();
	arisc_hwspinlock_standby_resume();

	/* allocate a message frame */
	pmessage = arisc_message_allocate(ARISC_MESSAGE_ATTR_HARDSYN);
	if (pmessage == NULL) {
		ARISC_WRN("allocate message failed\n");
		return -ENOMEM;
	}

	/* initialize message */
	pmessage->type     = ARISC_SSTANDBY_RESTORE_NOTIFY;
	pmessage->state    = ARISC_MESSAGE_INITIALIZED;

	arisc_hwmsgbox_send_message(pmessage, ARISC_SEND_MSG_TIMEOUT);

	/* record wakeup event */
	wakeup_event   = pmessage->paras[0];
	if (arisc_debug_dram_crc_en) {
		dram_crc_error = pmessage->paras[1];
		dram_crc_total_count++;
		dram_crc_error_count += (dram_crc_error ? 1 : 0);
	}

	/* free message */
	arisc_message_free(pmessage);

	return 0;
}
EXPORT_SYMBOL(arisc_cpux_ready_notify);

/**
 * enter talk standby.
 * @para:  parameter for enter talk standby.
 *
 * return: result, 0 - talk standby successed,
 *                !0 - talk standby failed;
 */
int arisc_standby_talk(struct super_standby_para *para, arisc_cb_t cb, void *cb_arg)
{
	struct arisc_message *pmessage;

	/* allocate a message frame */
	pmessage = arisc_message_allocate(0);
	if (pmessage == NULL) {
		ARISC_ERR("allocate message for talk-standby request failed\n");
		return -ENOMEM;
	}

	/* check super_standby_para size valid or not */
	if (sizeof(struct super_standby_para) > sizeof(pmessage->paras)) {
		ARISC_ERR("talk-standby parameters number too long\n");
		return -EINVAL;
	}

	/* initialize message */
	pmessage->type       = ARISC_TSTANDBY_ENTER_REQ;
	pmessage->cb.handler = cb;
	pmessage->cb.arg     = cb_arg;
	memcpy((void *)pmessage->paras, (const void *)para, sizeof(struct super_standby_para));
	pmessage->state      = ARISC_MESSAGE_INITIALIZED;

	/* send enter super-standby request to arisc */
	arisc_hwmsgbox_send_message(pmessage, ARISC_SEND_MSG_TIMEOUT);

	return 0;
}
EXPORT_SYMBOL(arisc_standby_talk);

/**
 * notify arisc cpux talk-standby restored.
 * @para:  none.
 *
 * return: result, 0 - notify successed, !0 - notify failed;
 */
int arisc_cpux_talkstandby_ready_notify(void)
{
	struct arisc_message *pmessage;

	/* notify hwspinlock and hwmsgbox resume first */
	arisc_hwmsgbox_standby_resume();
	arisc_hwspinlock_standby_resume();

	/* allocate a message frame */
	pmessage = arisc_message_allocate(ARISC_MESSAGE_ATTR_HARDSYN);
	if (pmessage == NULL) {
		ARISC_WRN("allocate message failed\n");
		return -ENOMEM;
	}

	/* initialize message */
	pmessage->type     = ARISC_TSTANDBY_RESTORE_NOTIFY;
	pmessage->state    = ARISC_MESSAGE_INITIALIZED;

	arisc_hwmsgbox_send_message(pmessage, ARISC_SEND_MSG_TIMEOUT);

	/* record wakeup event */
	wakeup_event   = pmessage->paras[0];
	if (arisc_debug_dram_crc_en) {
		dram_crc_error = pmessage->paras[1];
		dram_crc_total_count++;
		dram_crc_error_count += (dram_crc_error ? 1 : 0);
	}

	/* free message */
	arisc_message_free(pmessage);

	return 0;
}
EXPORT_SYMBOL(arisc_cpux_talkstandby_ready_notify);

/**
 * enter fake power off.
 */
 #if defined CONFIG_ARCH_SUN8IW1P1
void arisc_fake_power_off(void)
{
	struct arisc_message *pmessage;
	script_item_u script_val;
	script_item_value_type_e type;

	/* allocate a message frame */
	pmessage = arisc_message_allocate(ARISC_MESSAGE_ATTR_HARDSYN);
	if (pmessage == NULL) {
		ARISC_ERR("allocate message for fake power off request failed\n");
	}

	type = script_get_item("pmu1_para", "pmu_ir_power_key_code", &script_val);
	if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
		ARISC_ERR("get ir power off key config type err!");
		script_val.val = 0;
	}

	/* initialize message */
	pmessage->type       = ARISC_FAKE_POWER_OFF_REQ;
	pmessage->attr       = 0;
	pmessage->state      = ARISC_MESSAGE_INITIALIZED;
	pmessage->paras[0]   = script_val.val;

	/* send enter fake power off request to arisc */
	arisc_hwmsgbox_send_message(pmessage, ARISC_SEND_MSG_TIMEOUT);
}
EXPORT_SYMBOL(arisc_fake_power_off);
#endif

int arisc_config_ir_paras(u32 ir_code, u32 ir_addr)
{
	int result = 0;
	struct arisc_message *pmessage;

	/* allocate a message frame */
	pmessage = arisc_message_allocate(ARISC_MESSAGE_ATTR_HARDSYN);
	if (pmessage == NULL) {
		ARISC_WRN("allocate message failed\n");
		return -ENOMEM;
	}
	/* initialize message */
	pmessage->type       = ARISC_SET_IR_PARAS;
	pmessage->paras[0]   = ir_code;
	pmessage->paras[1]   = ir_addr;
	pmessage->state      = ARISC_MESSAGE_INITIALIZED;
	pmessage->cb.handler = NULL;
	pmessage->cb.arg     = NULL;

	ARISC_INF("ir power key:0x%x, addr:0x%x\n", ir_code, ir_addr);

	/* send request message */
	arisc_hwmsgbox_send_message(pmessage, ARISC_SEND_MSG_TIMEOUT);
	if (pmessage->result) {
		ARISC_WRN("config ir power key code [%d] fail\n", pmessage->paras[0]);
		result = -EINVAL;
	}

	/* free allocated message */
	arisc_message_free(pmessage);

	return result;
}
EXPORT_SYMBOL(arisc_config_ir_paras);

int arisc_sysconfig_ir_paras(void)
{
	u32    ir_power_key_code = 0;
	u32    ir_addr_code      = 0;
	int    result = 0;
	struct arisc_message *pmessage;
	script_item_u script_val;
	script_item_value_type_e type;
#if (defined CONFIG_ARCH_SUN8IW7P1)
	int i;
	char key_buf[32] = "ir_power_key_code";
	char add_buf[32] = "ir_addr_code";
#endif

	/* allocate a message frame */
	pmessage = arisc_message_allocate(ARISC_MESSAGE_ATTR_HARDSYN);
	if (pmessage == NULL) {
		ARISC_WRN("allocate message failed\n");
		return -ENOMEM;
	}

#if (defined CONFIG_ARCH_SUN8IW7P1)
	for (i = 0; i < ARISC_IR_KEY_SUP_NUM; i++) {
		sprintf(key_buf + 17, "%d", i);
		sprintf(add_buf + 12, "%d", i);

		type = script_get_item("s_cir0", key_buf, &script_val);
		if (SCIRPT_ITEM_VALUE_TYPE_INT != type)
			break;
		ir_power_key_code = script_val.val;

		type = script_get_item("s_cir0", add_buf, &script_val);
		if (SCIRPT_ITEM_VALUE_TYPE_INT != type)
			break;
		ir_addr_code = script_val.val;

		pmessage->type	     = ARISC_SET_IR_PARAS;
		pmessage->paras[0]   = ir_power_key_code;
		pmessage->paras[1]   = ir_addr_code;
		pmessage->state      = ARISC_MESSAGE_INITIALIZED;
		pmessage->cb.handler = NULL;
		pmessage->cb.arg     = NULL;
		ARISC_INF("ir power key:0x%x, addr:0x%x\n", ir_power_key_code, ir_addr_code);

		arisc_hwmsgbox_send_message(pmessage, ARISC_SEND_MSG_TIMEOUT);
	}
#else
	/* parse ir power key code */
	type = script_get_item("s_cir0", "ir_power_key_code0", &script_val);
	if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
		ARISC_WRN("parse ir power key code fail\n");
		return -EINVAL;
	}
	ir_power_key_code = script_val.val;

	/* parse ir address code */
	type = script_get_item("s_cir0", "ir_addr_code0", &script_val);
	if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
		ARISC_WRN("parse ir address code fail\n");
		return -EINVAL;
	}
	ir_addr_code = script_val.val;

	/* initialize message */
	pmessage->type       = ARISC_SET_IR_PARAS;
	pmessage->paras[0]   = ir_power_key_code;
	pmessage->paras[1]   = ir_addr_code;
	pmessage->state      = ARISC_MESSAGE_INITIALIZED;
	pmessage->cb.handler = NULL;
	pmessage->cb.arg     = NULL;

	ARISC_INF("ir power key code:0x%x\n", pmessage->paras[0]);

	/* send request message */
	arisc_hwmsgbox_send_message(pmessage, ARISC_SEND_MSG_TIMEOUT);
#endif

	//check config fail or not
	if (pmessage->result) {
		ARISC_WRN("config ir power key code [%d] fail\n", pmessage->paras[0]);
		result = -EINVAL;
	}

	/* free allocated message */
	arisc_message_free(pmessage);

	return result;
}

int arisc_sysconfig_sstpower_paras(void)
{
	script_item_u val;
	script_item_value_type_e type;

	type = script_get_item("s_powchk", "s_powchk_used", &val);
	if(SCIRPT_ITEM_VALUE_TYPE_INT != type)
	{
		ARISC_ERR("sst power para type err %d!", __LINE__);
		return -EINVAL;
	}
	ARISC_INF("powerchk_used is %u\n", val.val);
	arisc_powchk_back.power_state.enable = val.val;

	type = script_get_item("s_powchk", "s_power_reg", &val);
	if(SCIRPT_ITEM_VALUE_TYPE_INT != type)
	{
		ARISC_ERR("sst power para type err %d!", __LINE__);
		return -EINVAL;
	}
	ARISC_INF("power_regs is %#x\n", val.val);
	arisc_powchk_back.power_state.power_reg = val.val;

	type = script_get_item("s_powchk", "s_system_power", &val);
	if(SCIRPT_ITEM_VALUE_TYPE_INT != type)
	{
		ARISC_ERR("sst power para type err %d!", __LINE__);
		return -EINVAL;
	}
	ARISC_INF("system_power is %#x\n", val.val);
	arisc_powchk_back.power_state.system_power = val.val;
	arisc_set_standby_power_cfg(&arisc_powchk_back);

	return 0;
}
