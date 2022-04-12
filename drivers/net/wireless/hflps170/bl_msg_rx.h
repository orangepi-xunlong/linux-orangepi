/**
 ****************************************************************************************
 *
 * @file bl_msg_rx.h
 *
 * @brief RX function declarations
 *
 * Copyright (C) BouffaloLab 2017-2018
 *
 ****************************************************************************************
 */

#ifndef _BL_MSG_RX_H_
#define _BL_MSG_RX_H_

void bl_rx_handle_msg(struct bl_hw *bl_hw, struct ipc_e2a_msg *msg);

#endif /* _BL_MSG_RX_H_ */
