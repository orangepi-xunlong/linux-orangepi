include $(src)/../Makefile.interdrv.param

ifneq ($(KERNELRELEASE),)
$(CVIARCH_L)_2d_engine-y += vg_lite_hal.o
$(CVIARCH_L)_2d_engine-y += vg_lite_kernel.o
$(CVIARCH_L)_2d_engine-y += vg_lite_platform_bm1688.o
obj-m += $(CVIARCH_L)_2d_engine.o
%.o: %.c
	@echo "Skipping compilation of $< (using prebuilt .o files)"

KBUILD_EXTRA_SYMBOLS = $(src)/Module.symvers
KBUILD_EXTRA_SYMBOLS += $(src)/../sys/Module.symvers
KBUILD_EXTRA_SYMBOLS += $(src)/../base/Module.symvers
else
all:
	$(MAKE) ARCH=$(ARCH) -C $(KERNEL_DIR) M=$(src) V=$(V) modules

clean:
	@rm -f *.ko *.mod* .*.cmd modules.order Module.symvers
	@rm -f soph_2d_engine.o
	@rm -rf .tmp_versions
endif
