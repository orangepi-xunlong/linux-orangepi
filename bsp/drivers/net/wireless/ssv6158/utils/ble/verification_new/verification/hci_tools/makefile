all:
#	gcc -I. -I./include -I./lib -I./lib_bt -I./lib_ctrl \
	./lib/ssv_lib.c \
	./lib_bt/bt_hci_eventh.c ./lib_bt/bt_hci_cmdrh.c \
        ./lib_bt/bt_hci_event_declare.c ./lib_bt/bt_hci_cmd_declare.c ./lib_bt/bt_hci_lib.c \
        ./cli/cli.c ./cli/cli_cmd.c host_cli.c -o host_cli
	$(CC) -D _CTRL_TI_CC2564B_ -I ../../include  -I. -I./lib -I./lib_bt -I./lib_ctrl \
	./lib/ssv_lib.c \
	./lib_bt/bt_hci_eventh.c \
	./lib_bt/bt_hci_cmdrh.c	\
	./lib_bt/bt_hci_event_declare.c \
	./lib_bt/bt_hci_cmd_declare.c	\
	./lib_bt/bt_hci_lib.c \
	./lib_bluez/bluetooth.c	\
	./lib_bluez/hci.c	\
	ssv_hci_if.c	\
	host_cli.c -c


clean:
	@echo "Do Clean..."
	@rm -rf *.o host_cli_ti
