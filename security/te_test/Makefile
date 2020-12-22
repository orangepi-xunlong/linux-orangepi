plus_sec := $(call as-instr,.arch_extension sec,+sec)
CFLAGS_test.o :=$(call as-instr,.arch_extension sec,-DREQUIRES_SEC=1)

te_test-objs += test.o

obj-$(CONFIG_TE_TEST) += te_test.o
