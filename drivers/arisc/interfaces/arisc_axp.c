/*
 *  drivers/arisc/interfaces/arisc_axp.c
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

/* nmi isr node, record current nmi interrupt handler and argument */
nmi_isr_t nmi_isr_node[2];
EXPORT_SYMBOL(nmi_isr_node);

/**
 * register call-back function, call-back function is for arisc notify some event to ac327,
 * axp/rtc interrupt for external interrupt NMI.
 * @type:  nmi type, pmu/rtc;
 * @func:  call-back function;
 * @para:  parameter for call-back function;
 *
 * @return: result, 0 - register call-back function successed;
 *                 !0 - register call-back function failed;
 * NOTE: the function is like "int callback(void *para)";
 *       this function will execute in system ISR.
 */
int arisc_nmi_cb_register(u32 type, arisc_cb_t func, void *para)
{
	if (nmi_isr_node[type].handler) {
		if(func == nmi_isr_node[type].handler) {
			ARISC_WRN("nmi interrupt handler register already\n");
			return 0;
		}
		/* just output warning message, overlay handler */
		ARISC_WRN("nmi interrupt handler register already\n");
		return -EINVAL;
	}
	nmi_isr_node[type].handler = func;
	nmi_isr_node[type].arg     = para;

	return 0;
}
EXPORT_SYMBOL(arisc_nmi_cb_register);


/**
 * unregister call-back function.
 * @type:  nmi type, pmu/rtc;
 * @func:  call-back function which need be unregister;
 */
void arisc_nmi_cb_unregister(u32 type, arisc_cb_t func)
{
	if ((u32)(nmi_isr_node[type].handler) != (u32)(func)) {
		/* invalid handler */
		ARISC_WRN("invalid handler for unreg\n\n");
		return ;
	}
	nmi_isr_node[type].handler = NULL;
	nmi_isr_node[type].arg     = NULL;
}
EXPORT_SYMBOL(arisc_nmi_cb_unregister);

int arisc_disable_nmi_irq(void)
{
	int                   result;
	struct arisc_message *pmessage;

	/* allocate a message frame */
	pmessage = arisc_message_allocate(ARISC_MESSAGE_ATTR_HARDSYN);
	if (pmessage == NULL) {
		ARISC_WRN("allocate message failed\n");
		return -ENOMEM;
	}

	/* initialize message */
	pmessage->type       = ARISC_AXP_DISABLE_IRQ;
	pmessage->state      = ARISC_MESSAGE_INITIALIZED;
	pmessage->cb.handler = NULL;
	pmessage->cb.arg     = NULL;

	/* send message use hwmsgbox */
	arisc_hwmsgbox_send_message(pmessage, ARISC_SEND_MSG_TIMEOUT);

	/* free message */
	result = pmessage->result;
	arisc_message_free(pmessage);

	return result;
}
EXPORT_SYMBOL(arisc_disable_nmi_irq);

int arisc_enable_nmi_irq(void)
{
	int                   result;
	struct arisc_message *pmessage;

	/* allocate a message frame */
	pmessage = arisc_message_allocate(ARISC_MESSAGE_ATTR_HARDSYN);
	if (pmessage == NULL) {
		ARISC_WRN("allocate message failed\n");
		return -ENOMEM;
	}

	/* initialize message */
	pmessage->type       = ARISC_AXP_ENABLE_IRQ;
	pmessage->state      = ARISC_MESSAGE_INITIALIZED;
	pmessage->cb.handler = NULL;
	pmessage->cb.arg     = NULL;

	/* send message use hwmsgbox */
	arisc_hwmsgbox_send_message(pmessage, ARISC_SEND_MSG_TIMEOUT);

	/* free message */
	result = pmessage->result;
	arisc_message_free(pmessage);

	return result;
}
EXPORT_SYMBOL(arisc_enable_nmi_irq);

int arisc_axp_get_chip_id(unsigned char *chip_id)
{
	int                   i;
	int                   result;
	struct arisc_message *pmessage;

	/* allocate a message frame */
	pmessage = arisc_message_allocate(ARISC_MESSAGE_ATTR_HARDSYN);
	if (pmessage == NULL) {
		ARISC_WRN("allocate message failed\n");
		return -ENOMEM;
	}

	/* initialize message */
	pmessage->type       = ARISC_AXP_GET_CHIP_ID;
	pmessage->state      = ARISC_MESSAGE_INITIALIZED;
	pmessage->cb.handler = NULL;
	pmessage->cb.arg     = NULL;

	memset((void *)pmessage->paras, 0, 16);

	/* send message use hwmsgbox */
	arisc_hwmsgbox_send_message(pmessage, ARISC_SEND_MSG_TIMEOUT);

	/* |paras[0]    |paras[1]    |paras[2]     |paras[3]      |
	 * |chip_id[0~3]|chip_id[4~7]|chip_id[8~11]|chip_id[12~15]|
	 */
	/* copy message readout data to user data buffer */
	for (i = 0; i < 4; i++) {
			chip_id[i] = (pmessage->paras[0] >> (i * 8)) & 0xff;
			chip_id[4 + i] = (pmessage->paras[1] >> (i * 8)) & 0xff;
			chip_id[8 + i] = (pmessage->paras[2] >> (i * 8)) & 0xff;
			chip_id[12 + i] = (pmessage->paras[3] >> (i * 8)) & 0xff;
	}

	/* free message */
	result = pmessage->result;
	arisc_message_free(pmessage);

	return result;
}
EXPORT_SYMBOL(arisc_axp_get_chip_id);

int arisc_set_led_bln(unsigned long led_rgb, unsigned long led_onms,  \
                      unsigned long led_offms, unsigned long led_darkms)
{
	int result;
	struct arisc_message *pmessage;

	/* allocate a message frame */
	pmessage = arisc_message_allocate(ARISC_MESSAGE_ATTR_HARDSYN);
	if (pmessage == NULL) {
		ARISC_WRN("allocate message failed\n");
		return -ENOMEM;
	}

	/* initialize message */
	pmessage->type       = ARISC_SET_LED_BLN;
	pmessage->private    = (void *)0; /* set charge magic flag */
	pmessage->paras[0]   = led_rgb;
	pmessage->paras[1]   = led_onms;
	pmessage->paras[2]   = led_offms;
	pmessage->paras[3]   = led_darkms;

	/* send message use hwmsgbox */
	arisc_hwmsgbox_send_message(pmessage, ARISC_SEND_MSG_TIMEOUT);

	/* free message */
	result = pmessage->result;
	arisc_message_free(pmessage);

	return result;

}
EXPORT_SYMBOL(arisc_set_led_bln);

#if (defined CONFIG_ARCH_SUN8IW5P1)
int arisc_adjust_pmu_chgcur(unsigned int max_chgcur, unsigned int chg_ic_temp)
{
	int                   result;
	struct arisc_message *pmessage;

	/* allocate a message frame */
	pmessage = arisc_message_allocate(ARISC_MESSAGE_ATTR_HARDSYN);
	if (pmessage == NULL) {
		ARISC_WRN("allocate message failed\n");
		return -ENOMEM;
	}

	/* initialize message */
	pmessage->type       = ARISC_AXP_SET_PARAS;
	pmessage->private    = (void *)0x62; /* set charge current flag */
	pmessage->paras[0]   = chg_ic_temp;
	pmessage->paras[1]   = max_chgcur;
	pmessage->paras[2]   = 0;

	/* send message use hwmsgbox */
	arisc_hwmsgbox_send_message(pmessage, ARISC_SEND_MSG_TIMEOUT);

	/* free message */
	result = pmessage->result;
	arisc_message_free(pmessage);

	return result;
}
EXPORT_SYMBOL(arisc_adjust_pmu_chgcur);
#endif

int arisc_axp_int_notify(struct arisc_message *pmessage)
{
	u32 type = pmessage->paras[0];
	u32 ret = 0;

	if (type & NMI_INT_TYPE_PMU_OFFSET) {
		/* call pmu interrupt handler */
		if (nmi_isr_node[NMI_INT_TYPE_PMU].handler == NULL) {
			ARISC_WRN("pmu irq handler not install\n");
			return 1;
		}

		ARISC_INF("call pmu interrupt handler\n");
		ret |= (*(nmi_isr_node[NMI_INT_TYPE_PMU].handler))(nmi_isr_node[NMI_INT_TYPE_PMU].arg);
	}
	if (type & NMI_INT_TYPE_RTC_OFFSET)
	{
		/* call rtc interrupt handler */
		if (nmi_isr_node[NMI_INT_TYPE_RTC].handler == NULL) {
			ARISC_WRN("rtc irq handler not install\n");
			return 1;
		}

		ARISC_INF("call rtc interrupt handler\n");
		ret |= (*(nmi_isr_node[NMI_INT_TYPE_RTC].handler))(nmi_isr_node[NMI_INT_TYPE_RTC].arg);
	}

	return ret;
}

static int arisc_get_pmu_cfg(char *main, char *sub, u32 *val)
{
	script_item_u script_val;
	script_item_value_type_e type;
	type = script_get_item(main, sub, &script_val);
	if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
		ARISC_ERR("%s-%s config type err or can not be found!", main, sub);
		return -EINVAL;
	}
	*val = script_val.val;
	ARISC_INF("arisc pmu paras config [%s] [%s] : %d\n", main, sub, *val);
	return 0;
}

int arisc_config_pmu_paras(void)
{
	u32 pmu_discharge_ltf = 0;
	u32 pmu_discharge_htf = 0;

	int result = 0;
	struct arisc_message *pmessage;

	/* parse pmu temperature paras */
	if (arisc_get_pmu_cfg("pmu1_para", "pmu_discharge_ltf", &pmu_discharge_ltf)) {
		ARISC_WRN("parse pmu discharge ltf fail\n");
		return -EINVAL;
	}
	if (arisc_get_pmu_cfg("pmu1_para", "pmu_discharge_htf", &pmu_discharge_htf)) {
		ARISC_WRN("parse pmu discharge htf fail\n");
		return -EINVAL;
	}

	/* allocate a message frame */
	pmessage = arisc_message_allocate(ARISC_MESSAGE_ATTR_HARDSYN);
	if (pmessage == NULL) {
		ARISC_WRN("allocate message failed\n");
		return -ENOMEM;
	}

	/* initialize message */
	pmessage->type       = ARISC_AXP_SET_PARAS;
	pmessage->private    = (void *)0x00; /* init pmu paras flag */
	pmessage->paras[0]   = pmu_discharge_ltf;
	pmessage->paras[1]   = pmu_discharge_htf;
	pmessage->paras[2]   = 0;

	pmessage->state      = ARISC_MESSAGE_INITIALIZED;
	pmessage->cb.handler = NULL;
	pmessage->cb.arg     = NULL;

	ARISC_INF("pmu_discharge_ltf:0x%x\n", pmessage->paras[0]);
	ARISC_INF("pmu_discharge_htf:0x%x\n", pmessage->paras[1]);

	/* send request message */
	arisc_hwmsgbox_send_message(pmessage, ARISC_SEND_MSG_TIMEOUT);

	//check config fail or not
	if (pmessage->result) {
		ARISC_WRN("config pmu paras [%d] [%d] fail\n", pmessage->paras[0], pmessage->paras[1]);
		result = -EINVAL;
	}

	/* free allocated message */
	arisc_message_free(pmessage);

	return result;
}

int arisc_pmu_set_voltage(u32 type, u32 voltage)
{
	int                   result;
	struct arisc_message *pmessage;

	/* allocate a message frame */
	pmessage = arisc_message_allocate(ARISC_MESSAGE_ATTR_HARDSYN);
	if (pmessage == NULL) {
		ARISC_WRN("allocate message failed\n");
		return -ENOMEM;
	}

	/* initialize message */
	pmessage->type       = ARISC_SET_PMU_VOLT;
	pmessage->state      = ARISC_MESSAGE_INITIALIZED;
	pmessage->cb.handler = NULL;
	pmessage->cb.arg     = NULL;
	pmessage->paras[0]   = type;
	pmessage->paras[1]   = voltage;

	/* send message use hwmsgbox */
	arisc_hwmsgbox_send_message(pmessage, ARISC_SEND_MSG_TIMEOUT);

	/* free message */
	result = pmessage->result;
	arisc_message_free(pmessage);

	return result;
}
EXPORT_SYMBOL(arisc_pmu_set_voltage);

unsigned int arisc_pmu_get_voltage(u32 type)
{
	u32                   voltage;
	struct arisc_message *pmessage;

	/* allocate a message frame */
	pmessage = arisc_message_allocate(ARISC_MESSAGE_ATTR_HARDSYN);
	if (pmessage == NULL) {
		ARISC_WRN("allocate message failed\n");
		return -ENOMEM;
	}

	/* initialize message */
	pmessage->type       = ARISC_GET_PMU_VOLT;
	pmessage->state      = ARISC_MESSAGE_INITIALIZED;
	pmessage->cb.handler = NULL;
	pmessage->cb.arg     = NULL;
	pmessage->paras[0]   = type;

	/* send message use hwmsgbox */
	arisc_hwmsgbox_send_message(pmessage, ARISC_SEND_MSG_TIMEOUT);
	voltage = pmessage->paras[1];

	/* free message */
	arisc_message_free(pmessage);

	return voltage;
}
EXPORT_SYMBOL(arisc_pmu_get_voltage);

#if (defined CONFIG_ARCH_SUN8IW7P1)
/* type:0:shutdown, 1:IR wait, 2:reboot */
static int arisc_pmu_reboot(u32 type)
{
	struct arisc_message *pmessage;

	/* allocate a message frame */
	pmessage = arisc_message_allocate(ARISC_MESSAGE_ATTR_ASYN);
	if (pmessage == NULL) {
		ARISC_WRN("allocate message failed\n");
		return -ENOMEM;
	}

	/* initialize message */
	pmessage->type       = ARISC_AXP_REBOOT;
	pmessage->state      = ARISC_MESSAGE_INITIALIZED;
	pmessage->cb.handler = NULL;
	pmessage->cb.arg     = NULL;
	pmessage->paras[0]   = type;
	pmessage->paras[1]   = 0;

	/* send message use hwmsgbox */
	arisc_hwmsgbox_send_message(pmessage, ARISC_SEND_MSG_TIMEOUT);

	return 0;
}

void arisc_power_off(void)
{
	ARISC_LOG("arisc power off\n");
	arisc_pmu_reboot(0);
}
#endif
