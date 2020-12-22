	.globl  sunxi_bromjump_start
	.globl  sunxi_bromjump_end
sunxi_bromjump_start:
	.incbin "arch/arm/mach-sunxi/power/brom/resumes.code"
sunxi_bromjump_end:
	.align  2
