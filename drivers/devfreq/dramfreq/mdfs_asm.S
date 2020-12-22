#include <mach/memory.h>

__SRAM

.global  prefetch_and_prediction_disable
.global  prefetch_and_prediction_enable

prefetch_and_prediction_disable:
	@disable prediction
	mrc     p15, 0, r0, c1, c0, 0
	bic     r0, r0, #(0x1 << 11)      //Disable all forms of branch prediction
	mcr     p15, 0, r0, c1, c0, 0

	@enable invalidate btb ...
	mrc     p15, 0, r0, c1, c0, 1
	orr     r0, r0, #(0x11 << 0)      //Invalidate BTB, disable indirect predictor
	orr     r0, r0, #(0x1 << 15)      //Forces in-order issue in branch execution unit
	orr     r0, r0, #(0xE << 20)      //Disable L2 TLB prefetching, force in-order load issue, force in-order requests to the same set and way
	mcr     p15, 0, r0, c1, c0, 1

	mov     pc, lr

prefetch_and_prediction_enable:
	@enable prediction
	mrc     p15, 0, r0, c1, c0, 0
	orr     r0, r0, #(0x1 << 11)
	mcr     p15, 0, r0, c1, c0, 0

	@disable invalidate btb ...
	mrc     p15, 0, r0, c1, c0, 1
	bic     r0, r0, #(0x11 << 0)
	bic     r0, r0, #(0x1 << 15)
	bic     r0, r0, #(0xE << 20)
	mcr     p15, 0, r0, c1, c0, 1

	mov     pc, lr
.end
