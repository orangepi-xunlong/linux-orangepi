#include <linux/export.h>
#include <linux/compiler.h>
#include <linux/types.h>
#include <arm_neon.h>

void eink_ctlstream_fill_data_neon(u32* cmd, u8* fb, int stride,
				   int sources, int gates, u32 masks[4])
{
	uint32x4_t mask;
	int g, s;
	int cutoff = sources / 4 / 8 / 3 * 2;

	mask = vdupq_n_u32(0);

	for (g = 0; g < gates; g++) {
		if (unlikely(g == 0))
			mask = vdupq_n_u32(masks[0]);
		else
			mask = vdupq_n_u32(masks[2]);

		for (s = 0; s < sources / 4 / 8; s++) {
			uint8x8_t src;
			uint16x8_t dst;
			uint32x4_t out;

			if (unlikely(s == cutoff))
				mask = vdupq_n_u32(g == 0 ? masks[1] : masks[3]);

			src = vld1_u8(fb);
			fb += 8;

			dst = vshll_n_u8(src, 3);
			dst = vsliq_n_u16(dst, vmovl_u8(vshr_n_u8(src, 3)), 8);

			out = vmovl_u16(vget_low_u16(dst));
			out = vorrq_u32(out, mask);
			vst1q_u32(cmd, out);
			cmd += 4;

			out = vmovl_u16(vget_high_u16(dst));
			out = vorrq_u32(out, mask);
			vst1q_u32(cmd, out);
			cmd += 4;
		}

		cmd += stride - sources / 4;
	}
}
EXPORT_SYMBOL_GPL(eink_ctlstream_fill_data_neon);
