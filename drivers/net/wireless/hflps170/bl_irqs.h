/**
 ******************************************************************************
 *
 * @file bl_irqs.h
 *
 * Copyright (C) BouffaloLab 2017-2018
 *
 ******************************************************************************
 */
#ifndef _BL_IRQS_H_
#define _BL_IRQS_H_

#include <linux/interrupt.h>

/* IRQ handler to be registered by platform driver */
//irqreturn_t bl_irq_hdlr(int irq, void *dev_id);
void bl_irq_hdlr(struct sdio_func *func);
void bl_task(unsigned long data);
void main_wq_hdlr(struct work_struct *work);
void bl_queue_main_work(struct bl_hw *bl_hw);

#endif /* _BL_IRQS_H_ */
