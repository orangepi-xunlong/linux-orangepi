#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <termios.h>

#include "types.h"

#include "lib_bt/bt_hci_lib.h"

#define HW_FLOW_CTRL_EN
#define SW_FLOW_CTRL_DISABLE
#define BIT8_NOPARITY_1STOPBIT

#ifdef _CTRL_TI_CC2564_
#include <ti/ti_cc2564.h>
#endif

#ifdef _CTRL_TI_CC2564B_
#include <ti/ti_cc2564b.h>
#endif

#define _DEV_NAME_W_    40

// # variables for controller

int     fd_uart_array[7]={0};
char    uart_port[_DEV_NAME_W_] = {0};

// # control path buffer
// ## command
u8  buf_cmd[_BT_HCI_W_CMD_HEADER_ + _BT_HCI_W_PARAMTER_MAX_] = {0};

// ## event
// ### note: handle 1 event only.
u8  event_code;
u8  event_length;
u8  event_parameter[_BT_HCI_W_PARAMTER_MAX_] = {0};

// # data path buffer
// ## 2ctrl
// ## 2host

int TI_LoadPatch(int uart_num){

	// # down-load patch
	printf("# load TI patch ...\n");
#ifdef _CTRL_TI_CC2564_
	printf("## patch TI CC2564 ...\n");
	if (BT_HCI_VS_PATCH_BYCHARRAY(uart_num, ti_cc2564_BasePatch) != FX_SUCCESS) {
		return -1;
	}
	if (BT_HCI_VS_PATCH_BYCHARRAY(uart_num, ti_cc2564_LowEnergyPatch) != FX_SUCCESS) {
		return -1;
	}
#endif

#ifdef _CTRL_TI_CC2564B_
	printf("## patch TI CC2564B ...\n");
	if (BT_HCI_VS_PATCH_BYCHARRAY(uart_num, ti_cc2564b_BasePatch) != FX_SUCCESS) {
		return -1;
	}
	if (BT_HCI_VS_PATCH_BYCHARRAY(uart_num, ti_cc2564b_LowEnergyPatch) != FX_SUCCESS) {
		return -1;
	}
#endif


}

int ttyInit(int *uart_num ,char* tty , int baud_rate){
	// # variables for UART
	struct termios  config_uart;

    printf("######################\n");
	sprintf(uart_port, "/dev/%s", tty);
	*uart_num = open(uart_port, O_RDWR | O_NOCTTY | O_NDELAY);

	if (*uart_num <= 0) {
		printf("open %s is error ! \n", uart_port);
		goto initial_error;
	}
	printf("## open %s, fd_uart = %d!\n", uart_port, *uart_num);

	// # config uart
	if(!isatty(*uart_num)) {
		printf("## not a tty!\n");
		goto initial_error;
	}
	if(tcgetattr(*uart_num, &config_uart) < 0) {
		printf("## not configurable!\n");
		goto initial_error;
	}

	config_uart.c_cflag |= CSTOPB;

	if(cfsetispeed(&config_uart, baud_rate) < 0 || cfsetospeed(&config_uart, baud_rate) < 0) {
		printf("## configure baud-rate error!\n");
		goto initial_error;
	}

#ifdef HW_FLOW_CTRL_EN
    // # setting hw flow control
    config_uart.c_cflag |= CRTSCTS;
    printf("## enable hw flow control\n");
#else
    config_uart.c_cflag &= ~CRTSCTS;
    printf("## disable hw flow control\n");
#endif
#ifdef SW_FLOW_CTRL_DISABLE
    // # no software flow control
    config_uart.c_iflag &= ~(IXON | IXOFF | IXANY);
    printf("## disable sw flow control\n");
#endif
#ifdef BIT8_NOPARITY_1STOPBIT // #8N1
    config_uart.c_cflag &= ~PARENB;
    config_uart.c_cflag &= ~CSTOPB;
    config_uart.c_cflag &= ~CSIZE;
    config_uart.c_cflag |=    CS8;
    printf("## config 8/N/1\n");
#endif
    // # turn off I/O NL/CR auto replacemant
    config_uart.c_oflag &= ~(ONLCR|OCRNL);
    config_uart.c_iflag &= ~(INLCR|ICRNL);

    config_uart.c_iflag &= ~(ICANON|ECHO|ISIG);

	if(tcsetattr(*uart_num, TCSAFLUSH, &config_uart) < 0) {
		printf("## configure stop-bit error!\n");
		goto initial_error;
	}
	printf("## uart configuration ok!\n");
    printf("######################\n");

	return 0;

    // # initial error
initial_error:
    printf("# initial error\n");
    if (*uart_num >= 0) {
        close(*uart_num);
    }
    return -1;
}

int ttyReconfig(int tty_num){
	// # variables for UART
	struct termios  config_uart;
    int uart_num = 0;

	sprintf(uart_port, "/dev/ttyUSB%d" ,tty_num);
	uart_num = open(uart_port, O_RDWR | O_NOCTTY | O_NDELAY);


	if (uart_num <= 0) {
		printf("fd_uart %d error ! \n", uart_num);
		goto initial_error;
	}
	printf("## fd_uart = %d!\n", uart_num);

	// # config uart
	if(!isatty(uart_num)) {
		printf("## not a tty!\n");
		goto initial_error;
	}
	if(tcgetattr(uart_num, &config_uart) < 0) {
		printf("## not configurable!\n");
		goto initial_error;
	}

	config_uart.c_cflag |= CSTOPB;

//    if(cfsetospeed(&config_uart, B230400) < 0 || cfsetispeed(&config_uart, B230400) < 0) {
//    if(cfsetospeed(&config_uart, B460800) < 0 || cfsetispeed(&config_uart, B460800) < 0) {
      if(cfsetospeed(&config_uart, B921600) < 0 || cfsetispeed(&config_uart, B921600) < 0) {
		printf("## configure baud-rate with 230400 error!\n");
		goto initial_error;
	}
#ifdef HW_FLOW_CTRL_EN
        // # setting hw flow control
        config_uart.c_cflag |= CRTSCTS;
        printf("## enable hw flow control\n");
#else
        config_uart.c_cflag &= ~CRTSCTS;
        printf("## disable hw flow control\n");
#endif
#ifdef SW_FLOW_CTRL_DISABLE
        // # no software flow control
        config_uart.c_iflag &= ~(IXON | IXOFF | IXANY);
        printf("## disable sw flow control\n");
#endif
#ifdef BIT8_NOPARITY_1STOPBIT // #8N1
        config_uart.c_cflag &= ~PARENB;
        config_uart.c_cflag &= ~CSTOPB;
        config_uart.c_cflag &= ~CSIZE;
        config_uart.c_cflag |=    CS8;
        printf("## config 8/N/1\n");
#endif
        // # Send two stop bits, else one.
        //config_uart.c_cflag |= CSTOPB;

	if(tcsetattr(uart_num, TCSAFLUSH, &config_uart) < 0) {
		printf("## configure uart error!\n");
		goto initial_error;
	}
	printf("## uart configuration ok!\n");
    printf("######################\n");

	return 0;

    // # initial error
initial_error:
    printf("# initial error\n");
    if (uart_num >= 0) {
        close(uart_num);
    }
    return -1;
}

int ttyEnableHwFlowCtrl(int uart_num, bool enable){
	// # variables for UART
	struct termios  config_uart;

	// # config uart
	if(!isatty(uart_num)) {
		printf("## not a tty!\n");
		goto config_error;
	}
	if(tcgetattr(uart_num, &config_uart) < 0) {
		printf("## not configurable!\n");
		goto config_error;
	}

    if (enable == TRUE) {
        config_uart.c_cflag |= CRTSCTS;
    }
    else {
        config_uart.c_cflag &= ~CRTSCTS;
    }

	if(tcsetattr(uart_num, TCSAFLUSH, &config_uart) < 0) {
		printf("## configure stop-bit error!\n");
		goto config_error;
	}
	printf("## uart configuration ok!\n");
    printf("######################\n");

	return 0;

    // # initial error
config_error:
    printf("# config error\n");
    return -1;
}

int ttyChangeBaudRate(int uart_num, int baud_rate){
	// # variables for UART
	struct termios  config_uart;

	// # config uart
	if(!isatty(uart_num)) {
		printf("## not a tty!\n");
		goto config_error;
	}
	if(tcgetattr(uart_num, &config_uart) < 0) {
		printf("## not configurable!\n");
		goto config_error;
	}

    if(cfsetispeed(&config_uart, baud_rate) < 0 || cfsetospeed(&config_uart, baud_rate) < 0) {
		printf("## configure baud-rate error!\n");
		goto config_error;
	}

	if(tcsetattr(uart_num, TCSAFLUSH, &config_uart) < 0) {
		printf("## configure stop-bit error!\n");
		goto config_error;
	}
	printf("## uart configuration ok!\n");
    printf("######################\n");

	return 0;

    // # initial error
config_error:
    printf("# config error\n");
    return -1;
}
