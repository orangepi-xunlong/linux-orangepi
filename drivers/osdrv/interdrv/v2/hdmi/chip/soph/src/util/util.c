#include "util.h"

u16 concat_bits(u8 bhi, u8 ohi, u8 nhi, u8 blo, u8 olo, u8 nlo)
{
	return (bit_field(bhi, ohi, nhi) << nlo) | bit_field(blo, olo, nlo);
}

u16 byte_to_word(const u8 hi, const u8 lo)
{
	return concat_bits(hi, 0, 8, lo, 0, 8);
}

u8 bit_field(const u16 data, u8 shift, u8 width)
{
	return ((data >> shift) & ((((u16) 1) << width) - 1));
}

u32 byte_to_dword(u8 b3, u8 b2, u8 b1, u8 b0)
{
	u32 retval = 0;
	retval |= b0 << (0 * 8);
	retval |= b1 << (1 * 8);
	retval |= b2 << (2 * 8);
	retval |= b3 << (3 * 8);
	return retval;
}

u8 is_equal(u32 a, u32 b){
	return (a == b);
}
