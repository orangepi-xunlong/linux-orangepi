SRCS    := dut.c
#hciconfig.c hcitool.c
OBJS    := $(subst .c,.o,$(SRCS))
TARGET  := $(OBJS)


all: $(TARGET) bench_hci
	@echo "linking..."
	$(CC)   $(CFLAGS) -o dut	\
	./$(OBJS)	\
	../hci_tools/bt_hci_lib.o        \
	../hci_tools/host_cli.o	\
	../hci_tools/ssv_lib.o	\
	../hci_tools/bluetooth.o	\
	../hci_tools/hci.o	\
    ../hci_tools/ssv_hci_if.o  \
	../../log/log.o		\
	../../task/task.o

bench_hci:

%.o: %.c
	@echo "Compiling $@"
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	@echo "Do clean ..."
	@\rm -f *.o dut
