SRCS    := pattern2dut.c param_pattern.c cli_cmd_master.c cli_cmd_bcmd.c
OBJS    := $(subst .c,.o,$(SRCS))

all: $(OBJS)
%.o: %.c
	@echo "Compiling $@"
	$(CC) $(CFLAGS) -c -o $@ $<
clean:
	@echo "Do clean ..."
	@rm -f *.o
