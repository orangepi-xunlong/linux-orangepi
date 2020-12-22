	.globl  arisc_binary_start
	.globl  arisc_binary_end
arisc_binary_start:
#if defined CONFIG_ARCH_SUN8IW1P1
	.incbin "drivers/arisc/binary/arisc_sun8iw1p1.bin"
#elif defined CONFIG_ARCH_SUN8IW3P1
	.incbin "drivers/arisc/binary/arisc_sun8iw3p1.bin"
#elif defined CONFIG_ARCH_SUN8IW5P1
	.incbin "drivers/arisc/binary/arisc_sun8iw5p1.bin"
#elif defined CONFIG_ARCH_SUN8IW6P1
	.incbin "drivers/arisc/binary/arisc_sun8iw6p1.bin"
#elif defined CONFIG_ARCH_SUN8IW7P1
	.incbin "drivers/arisc/binary/arisc_sun8iw7p1.bin"
#elif defined CONFIG_ARCH_SUN8IW9P1
	.incbin "drivers/arisc/binary/arisc_sun8iw9p1.bin"
#elif defined CONFIG_ARCH_SUN9IW1P1
	.incbin "drivers/arisc/binary/arisc_sun9iw1p1.bin"
#else
#error "please select a platform\n"
#endif
arisc_binary_end:
	.align  2
