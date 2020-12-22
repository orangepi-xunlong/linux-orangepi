obj-$(CONFIG_SUNXI_GETH) += sunxi_gmac.o

sunxi_gmac-objs := sunxi_geth.o

ifneq ($(strip $(CONFIG_ARCH_SUN9I) $(CONFIG_ARCH_SUN8IW1) $(CONFIG_ARCH_SUN8IW2)),)

sunxi_gmac-objs += sunxi_libgeth.a

quiet_cmd_libgeth = CP      $(@)
      cmd_libgeth = cp -fr $(filter-out FORCE,$<) $@

$(obj)/%.a: $(obj)/sunxi_libgeth FORCE
	$(call if_changed,libgeth)

ifneq ($(wildcard $(obj)/hw_ops/Makefile),)
$(obj)/sunxi_libgeth: FORCE
	$(Q)rm -fr $@
	$(Q)$(MAKE) $(build)=$(obj)/hw_ops libgeth_install

subdir- += hw_ops
clean-files += sunxi_libgeth
endif

else

sunxi_gmac-objs += sunxi_nops.o

endif

