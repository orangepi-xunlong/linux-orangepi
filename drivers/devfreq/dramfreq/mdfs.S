	.globl	mdfs_bin_start
mdfs_bin_start:
#if defined CONFIG_ARCH_SUN8IW3P1
	.incbin	"drivers/devfreq/dramfreq/mdfs/mdfs_sun8iw3p1.code"
#elif defined CONFIG_ARCH_SUN8IW6P1
	.incbin "drivers/devfreq/dramfreq/mdfs/mdfs_sun8iw6p1.code"
#else
#error "please select a platform\n"
#endif
	.globl	mdfs_bin_end
mdfs_bin_end:
	.align	2
