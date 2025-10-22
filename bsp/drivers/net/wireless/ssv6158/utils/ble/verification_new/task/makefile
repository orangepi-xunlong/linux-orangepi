SRCS    := task.c
OBJS    := $(subst .c,.o,$(SRCS))

TARGET  := $(OBJS)

all: $(TARGET)

%.o: %.c
	@echo "Compiling $@"
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	@echo "Do clean ..."
	@\rm -f *.o
