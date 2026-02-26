#ifndef _HOST_CLI_H_
#define _HOST_CLI_H_

int TI_LoadPatch(int uart_num);
int ttyInit(int *uart_num ,char* tty , int baud_rate);
int ttyReconfig(int uart_num);
int ttyEnableHwFlowCtrl(int uart_num, bool enable);
int ttyChangeBaudRate(int uart_num, int baud_rate);

#endif
