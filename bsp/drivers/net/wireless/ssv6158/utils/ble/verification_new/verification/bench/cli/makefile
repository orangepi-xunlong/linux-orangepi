SRCS    := cli.c cli_cmd.c
OBJS    := $(subst .c,.o,$(SRCS))

#TARGET  := cli.o cli_cmd.o

all: $(OBJS)

%.o: %.c
	@echo "Compiling $@"
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	@echo "Do clean ..."
	@\rm -f *.o
