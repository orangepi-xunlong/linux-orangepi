
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

LIBS = -I./gtest_install/include -L./gtest_install/lib -lgtest -lgtest_main -lpthread
CC = $(CROSS_COMPILE)gcc
CXX = $(CROSS_COMPILE)g++

source  = wdt_test.cpp
target = wdt_test
object  = $(source:%.cpp=%.o)

all: $(object)
	@$(CXX) $(object) $(CFLAG) $(LIBS) -o $(target) $(OPT_LEVEL)

%.o: %.c
	@$(CC) $(CCCFLAGS) -c $<

%.o: %.cpp
	@$(CXX) $(LIBS) -c $<

clean:
	@$(RM) *.o $(target)
