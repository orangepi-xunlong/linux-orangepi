// SPDX-License-Identifier: GPL-3.0
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * Copyright (c) 2023 liujuan1@allwinnertech.com
 */

#ifndef SUNXI_SOFT_UART_QUEUE_H
#define SUNXI_SOFT_UART_QUEUE_H

#define QUEUE_MAX_SIZE  256

struct queue {
  int front;
  int rear;
  int size;
  unsigned char data[QUEUE_MAX_SIZE];
};


void initialize_queue(struct queue *queue);
int  enqueue_character(struct queue *queue, const unsigned char character);
int  dequeue_character(struct queue *queue, unsigned char *character);
int  enqueue_string(struct queue *queue, const unsigned char *string, int string_size);
int  get_queue_room(struct queue *queue);
int  get_queue_size(struct queue *queue);

#endif
