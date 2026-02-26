################################################################
# Define the make variables
################################################################
CC      := gcc
TOPDIR  := $(CURDIR)

INCLUDE := -I.
INCLUDE := -I$(TOPDIR)
INCLUDE += -I$(TOPDIR)/../include
INCLUDE += -I$(TOPDIR)/../task
INCLDUE += -I$(TOPDIR)/../log
INCLUDE += -I$(TOPDIR)/bench
INCLUDE += -I$(TOPDIR)/pattern
INCLUDE += -I$(TOPDIR)/hci_tools
CFLAGS  := $(INCLUDE) -Wall -fno-stack-protector -g -DREAD_EVT_DEBUG -DBLOCK_SIGALRM

################################################################
export CC TOPDIR CFLAGS
all:
	@echo $(CFLAGS)
################################################################
