#
# arch/riscv/boot/Makefile
#
# This file is included by the global makefile so that you can add your own
# architecture-specific flags and dependencies.
#
# This file is subject to the terms and conditions of the GNU General Public
# License.  See the file "COPYING" in the main directory of this archive
# for more details.
#
# Copyright (C) 2018, Anup Patel.
# Author: Anup Patel <anup@brainfault.org>
#
# Based on the ia64 and arm64 boot/Makefile.
#

KCOV_INSTRUMENT := n

OBJCOPYFLAGS_Image :=-O binary -R .note -R .note.gnu.build-id -R .comment -S
OBJCOPYFLAGS_loader.bin :=-O binary
OBJCOPYFLAGS_xipImage :=-O binary -R .note -R .note.gnu.build-id -R .comment -S

targets := Image Image.* loader loader.o loader.lds loader.bin
targets := Image Image.* loader loader.o loader.lds loader.bin xipImage

ifeq ($(CONFIG_XIP_KERNEL),y)

quiet_cmd_mkxip = $(quiet_cmd_objcopy)
cmd_mkxip = $(cmd_objcopy)

$(obj)/xipImage: vmlinux FORCE
	$(call if_changed,mkxip)
	@$(kecho) '  Physical Address of xipImage: $(CONFIG_XIP_PHYS_ADDR)'

endif

ifdef CONFIG_RELOCATABLE
vmlinux.relocs: vmlinux
	@ (! [ -f vmlinux.relocs ] && echo "vmlinux.relocs can't be found, please remove vmlinux and try again") || true

$(obj)/Image: vmlinux.relocs FORCE
else
$(obj)/Image: vmlinux FORCE
endif
	$(call if_changed,objcopy)

$(obj)/Image.gz: $(obj)/Image FORCE
	$(call if_changed,gzip)

$(obj)/loader.o: $(src)/loader.S $(obj)/Image

$(obj)/loader: $(obj)/loader.o $(obj)/Image $(obj)/loader.lds FORCE
	$(Q)$(LD) -T $(obj)/loader.lds -o $@ $(obj)/loader.o

$(obj)/Image.bz2: $(obj)/Image FORCE
	$(call if_changed,bzip2)

$(obj)/Image.lz4: $(obj)/Image FORCE
	$(call if_changed,lz4)

$(obj)/Image.lzma: $(obj)/Image FORCE
	$(call if_changed,lzma)

$(obj)/Image.lzo: $(obj)/Image FORCE
	$(call if_changed,lzo)

$(obj)/Image.zst: $(obj)/Image FORCE
	$(call if_changed,zstd)

$(obj)/loader.bin: $(obj)/loader FORCE
	$(call if_changed,objcopy)

ifdef CONFIG_32BIT
ADDR_BITS := 32
ADDR_CELLS := 1
else
ADDR_BITS := 64
ADDR_CELLS := 2
endif

IMAGE_LOAD_ADDRESS := $(CONFIG_IMAGE_LOAD_OFFSET)
IMAGE_ENTRY_ADDRESS := $(CONFIG_IMAGE_LOAD_OFFSET)
IMAGE_ALGO := crc32

quiet_cmd_its_cat = CAT     $@
      cmd_its_cat = cat $(real-prereqs) >$@

$(obj)/Image.its.S: $(addprefix $(srctree)/arch/riscv/generic/,$(ITS_INPUTS)) FORCE
	$(call if_changed,its_cat)

quiet_cmd_cpp_its_S = ITS     $@
      cmd_cpp_its_S = $(CPP) -P -C -o $@ $< \
		-DKERNEL_NAME="\"Linux $(KERNELRELEASE)\"" \
		-DIMAGE_COMPRESSION="\"$(2)\"" \
		-DIMAGE_CHECK_ALGORITHM="\"$(3)\"" \
		-DIMAGE_BINARY="\"$(4)\"" \
		-DIMAGE_LOAD_ADDRESS=$(IMAGE_LOAD_ADDRESS) \
		-DIMAGE_ENTRY_ADDRESS=$(IMAGE_ENTRY_ADDRESS) \
		-DADDR_BITS=$(ADDR_BITS) \
		-DADDR_CELLS=$(ADDR_CELLS)

$(obj)/Image.its: $(obj)/Image.its.S $(obj)/Image FORCE
	$(call if_changed,cpp_its_S,none,$(IMAGE_ALGO),Image)

$(obj)/Image.gz.its: $(obj)/Image.its.S $(obj)/Image.gz FORCE
	$(call if_changed,cpp_its_S,gzip,$(IMAGE_ALGO),Image.gz)

quiet_cmd_itb-image = ITB     $@
      cmd_itb-image = \
		env PATH="$(objtree)/scripts/dtc:$(PATH)" \
		$(BASH) $(MKIMAGE) \
		-D "-I dts -O dtb -p 500 \
			--include $(objtree)/arch/riscv \
			--warning no-unit_address_vs_reg" \
		-f $(2) $@

$(obj)/Image.itb: $(obj)/Image.its $(obj)/Image FORCE
	$(call if_changed,itb-image,$<)

$(obj)/Image.%.itb: $(obj)/Image.%.its $(obj)/Image.% FORCE
	$(call if_changed,itb-image,$<)

EFI_ZBOOT_PAYLOAD	:= Image
EFI_ZBOOT_BFD_TARGET	:= elf$(BITS)-littleriscv
EFI_ZBOOT_MACH_TYPE	:= RISCV$(BITS)

include $(srctree)/drivers/firmware/efi/libstub/Makefile.zboot
