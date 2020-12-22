	.align	2
	.globl	resume1_bin_start
resume1_bin_start:
	.incbin	"arch/arm/mach-sunxi/pm/standby/super/resume/resume1.code"
	.align 13
	.globl	resume1_bin_end
resume1_bin_end:
	.align	2
	
	
