# SPDX-License-Identifier: GPL-2.0-only
include ../scripts/Makefile.include

bindir ?= /usr/bin

ifeq ($(srctree),)
srctree := $(patsubst %/,%,$(dir $(CURDIR)))
srctree := $(patsubst %/,%,$(dir $(srctree)))
endif

# Do not use make's built-in rules
# (this improves performance and avoids hard-to-debug behaviour);
MAKEFLAGS += -r

CFLAGS += -O2 -Wall -g -D_GNU_SOURCE -I$(OUTPUT)include

ALL_TARGETS := fdleak_test
ALL_PROGRAMS := $(patsubst %,$(OUTPUT)%,$(ALL_TARGETS))

all: $(ALL_PROGRAMS)

export srctree OUTPUT CC LD CFLAGS
include $(srctree)/tools/build/Makefile.include

#
# We need the following to be outside of kernel tree
#
$(OUTPUT)include/linux/fdleak.h: ../../include/linux/soc/cix/dst_fdleak.h
	mkdir -p $(OUTPUT)include/linux/ 2>&1 || true
	ln -sf $(CURDIR)/../../include/linux/soc/cix/dst_fdleak.h $@

prepare: $(OUTPUT)include/linux/fdleak.h

#
# spidev_test
#
FDLEAK_TEST_IN := $(OUTPUT)fdleak_test-in.o
$(FDLEAK_TEST_IN): prepare FORCE
	$(Q)$(MAKE) $(build)=fdleak_test
$(OUTPUT)fdleak_test: $(FDLEAK_TEST_IN)
	$(QUIET_LINK)$(CC) $(CFLAGS) $(LDFLAGS) $< -o $@

clean:
	rm -f $(ALL_PROGRAMS)
	rm -rf $(OUTPUT)include/
	find $(if $(OUTPUT),$(OUTPUT),.) -name '*.o' -delete
	find $(if $(OUTPUT),$(OUTPUT),.) -name '\.*.o.d' -delete
	find $(if $(OUTPUT),$(OUTPUT),.) -name '\.*.o.cmd' -delete

install: $(ALL_PROGRAMS)
	install -d -m 755 $(DESTDIR)$(bindir);		\
	for program in $(ALL_PROGRAMS); do		\
		install $$program $(DESTDIR)$(bindir);	\
	done

FORCE:

.PHONY: all install clean FORCE prepare
