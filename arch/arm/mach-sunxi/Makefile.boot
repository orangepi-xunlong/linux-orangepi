
OBJCOPYFLAGS_Image := -R .note.gnu.build-id

ifneq ($(strip $(CONFIG_ARCH_SUN9I)),)
zreladdr-$(CONFIG_ARCH_SUNXI)	 += 0x20008000
else
zreladdr-$(CONFIG_ARCH_SUNXI)	 += 0x40008000
endif

params_phys-$(CONFIG_ARCH_SUNXI) := 0x00000100
initrd_phys-$(CONFIG_ARCH_SUNXI) := 0x00800000
