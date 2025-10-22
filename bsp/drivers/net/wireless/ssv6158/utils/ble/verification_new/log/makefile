SRCS    := log.c
OBJS    := $(subst .c,.o,$(SRCS))

all: $(OBJS)
%.o: %.c
	@echo "Compiling $@"
	$(CC) $(CFLAGS) -c -o $@ $<
clean:
	@echo "Do clean ..."
	@rm -f *.o
