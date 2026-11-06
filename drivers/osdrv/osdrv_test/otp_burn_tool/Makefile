
ifeq ($(SDK_VER), 32bit)
  export CROSS_COMPILE = $(CROSS_COMPILE_32)
  OPT_LEVEL := -O3
else ifeq ($(SDK_VER), 64bit)
  export CROSS_COMPILE = $(CROSS_COMPILE_64)
  OPT_LEVEL := -O3
else ifeq ($(SDK_VER), uclibc)
  export CROSS_COMPILE = $(CROSS_COMPILE_UCLIBC)
  OPT_LEVEL := -Os
else ifeq ($(SDK_VER), glibc_riscv64)
  export CROSS_COMPILE = $(CROSS_COMPILE_GLIBC_RISCV64)
  OPT_LEVEL := -Os
  OPT_LEVEL += -march=rv64imafdcvxthead -mcmodel=medany -mabi=lp64dv -static
else ifeq ($(SDK_VER), musl_riscv64)
  export CROSS_COMPILE = $(CROSS_COMPILE_MUSL_RISCV64)
  OPT_LEVEL := -Os
  OPT_LEVEL += -march=rv64imafdcvxthead -mcmodel=medany -mabi=lp64d -static
endif

LIBS =
CCCFLAGS = -I ../../../linux_5.10/usr/include/ -I include/
CC = $(CROSS_COMPILE)gcc
CXX = $(CROSS_COMPILE)g++

source = $(wildcard src/*.c)
target = otp_burn_tool
object = $(source:%.c=%.o)

all: $(object)
	@$(CXX) $(object) $(CFLAG) $(LIBS) -o $(target) $(OPT_LEVEL)

%.o: %.c
	@$(CC) $(CCCFLAGS) -c $< -o $@

%.o: %.cpp
	@$(CXX) $(LIBS) -c $< -o $@

clean:
	@$(RM) src/*.o $(target)
