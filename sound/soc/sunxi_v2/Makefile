# platform -> ahub
snd_soc_sunxi_ahub_dam-objs			+= snd_sunxi_ahub_dam.o
obj-$(CONFIG_SND_SOC_SUNXI_AHUB_DAM)		+= snd_soc_sunxi_ahub_dam.o

snd_soc_sunxi_ahub-objs				+= snd_sunxi_ahub.o
obj-$(CONFIG_SND_SOC_SUNXI_AHUB)		+= snd_soc_sunxi_ahub.o

# common -> machine (note: Finally compile, save system startup time)
snd_soc_sunxi_machine-objs			+= snd_sunxi_mach.o
snd_soc_sunxi_machine-objs			+= snd_sunxi_mach_utils.o
obj-$(CONFIG_SND_SOC_SUNXI_MACH)		+= snd_soc_sunxi_machine.o
