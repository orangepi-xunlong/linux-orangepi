include ./config.mk

DIR_BENCH    := $(TOPDIR)/bench
DIR_TASK     := $(TOPDIR)/../task
DIR_PATTERN  := $(TOPDIR)/pattern
DIR_DUT      := $(TOPDIR)/dut
DIR_LOG      := $(TOPDIR)/../log
DIR_HCI_TOOL := $(TOPDIR)/hci_tools

## todo: These 2 items maybe move to argument ##

BENCH    := 2dut
TASK     := task_
PATTERN  := pattern_
TARGET   := ctrl_$(BENCH)
DUT	 := dut_
LOG      := log_
HCI_TOOL := hci_tools_

all:	$(TARGET)
.PHONY : all

$(TARGET): $(PATTERN) $(LOG) $(TASK) $(HCI_TOOL) $(BENCH) $(DUT)
	@echo "Linking $@"
#hci_tool obj link testing#
# $(DIR_BENCH)/$(BENCH)/bench.$(BENCH).o
	$(CC)   $(CFLAGS) -o ctrl$(BENCH)	\
	$(DIR_TASK)/task.o	\
	$(DIR_LOG)/log.o \
	$(DIR_PATTERN)/pattern2dut.o	\
	$(DIR_PATTERN)/param_pattern.o	\
	$(DIR_PATTERN)/cli_cmd_master.o \
	$(DIR_PATTERN)/cli_cmd_bcmd.o   \
	$(DIR_BENCH)/ctrl_bench.o	\
	$(DIR_BENCH)/cli/cli.o	\
	$(DIR_BENCH)/cli/cli_cmd.o\
		$(DIR_HCI_TOOL)/hci.o  \
		$(DIR_HCI_TOOL)/bluetooth.o  \
		$(DIR_HCI_TOOL)/bt_hci_cmd_declare.o        \
	$(DIR_HCI_TOOL)/bt_hci_cmdrh.o      \
	$(DIR_HCI_TOOL)/bt_hci_event_declare.o      \
	$(DIR_HCI_TOOL)/bt_hci_eventh.o     \
	$(DIR_HCI_TOOL)/bt_hci_lib.o        \
	$(DIR_HCI_TOOL)/host_cli.o  \
	$(DIR_HCI_TOOL)/ssv_lib.o

$(PATTERN):
	@echo "generate new ver_info.h for dump result"
	@ ../verification_info.pl ../include/ver_info.h
	@echo "Compiling pattern"
	@cd $(DIR_PATTERN); $(MAKE) all

$(TASK):
	@echo "Compiling task"
	@cd $(DIR_TASK); $(MAKE) all

$(BENCH):
	@echo "Compiling bench"
	@cd $(DIR_BENCH); $(MAKE)
	@cd ./bench/cli ; $(MAKE)
	@cd $(DIR_BENCH); $(MAKE) bench_hci

$(HCI_TOOL):
	@echo "Compiling hci_tools"
	@cd $(DIR_HCI_TOOL) ; $(MAKE) all

$(LOG):
	@echo "Compiling log"
	@cd $(DIR_LOG) ; $(MAKE) all

$(DUT):
	@echo "Compiling dut"
	@cd $(DIR_DUT) ; $(MAKE) all

%.o: %.c
	@echo "Compiling $@"

clean:
	@echo "Do clean ..."
	@rm -f ctrl2dut
	@cd $(DIR_TASK);    $(MAKE) clean
	@cd $(DIR_BENCH);   $(MAKE) clean
	@cd $(DIR_PATTERN);   $(MAKE) clean
	@cd $(DIR_DUT);		$(MAKE) clean
	@cd $(DIR_LOG);		$(MAKE) clean
	@cd $(DIR_HCI_TOOL);		$(MAKE) clean
