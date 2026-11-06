#ifndef _UTIL_H_
#define _UTIL_H_

typedef void (*handler_t) (void *);
typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;

u8 is_equal(u32 a, u32 b);

static inline u32 first_bit_set(u32 data)
{
	u32 n = 0;

	if (data != 0) {
		for (n = 0; (data & 1) == 0; n++) {
			data >>= 1;
		}
	}
	return n;
}

/**
 * Get bit field
 * @param[in] data raw data
 * @param[in] mask bit field mask
 * @return bit field value
 */
static inline u32 get(u32 data, u32 mask)
{
	return ((data & mask) >> first_bit_set(mask));
}

/**
 * Set bit field
 * @param[in] data raw data
 * @param[in] mask bit field mask
 * @param[in] value new value
 * @return new raw data
 */
static inline u32 set(u32 data, u32 mask, u32 value)
{
	return (((value << first_bit_set(mask)) & mask) | (data & ~mask));
}

/**
 * Get bit field
 * @param[in] data raw data
 * @param[in] mask bit field mask
 * @return bit field value
 */
static inline u8 get8(u8 data, u8 mask)
{
	return ((data & mask) >> first_bit_set(mask));
}

/**
 * Set bit field
 * @param[in] data raw data
 * @param[in] mask bit field mask
 * @param[in] value new value
 * @return new raw data
 */
static inline u8 set8(u8 data, u8 mask, u8 value)
{
	return (((value << first_bit_set(mask)) & mask) | (data & ~mask));
}

/**
 * Concatenate two parts of two 8-bit bytes into a new 16-bit word
 * @param bhi first byte
 * @param ohi shift part of first byte (to be place as most significant
 * bits)
 * @param nhi width part of first byte (to be place as most significant
 * bits)
 * @param blo second byte
 * @param olo shift part of second byte (to be place as least
 * significant bits)
 * @param nlo width part of second byte (to be place as least
 * significant bits)
 * @returns 16-bit concatenated word as part of the first byte and part of
 * the second byte
 */
u16 concat_bits(u8 bhi, u8 ohi, u8 nhi, u8 blo, u8 olo, u8 nlo);

/** Concatenate two full bytes into a new 16-bit word
 * @param hi
 * @param lo
 * @returns hi as most significant bytes concatenated with lo as least
 * significant bits.
 */
u16 byte_to_word(const u8 hi, const u8 lo);

/** Extract the content of a certain part of a byte
 * @param data 8bit byte
 * @param shift shift from the start of the bit (0)
 * @param width width of the desired part starting from shift
 * @returns an 8bit byte holding only the desired part of the passed on
 * data byte
 */
u8 bit_field(const u16 data, u8 shift, u8 width);

/** Concatenate four 8-bit bytes into a new 32-bit word
 * @param b3 assumed as most significant 8-bit byte
 * @param b2
 * @param b1
 * @param b0 assumed as least significant 8bit byte
 * @returns a 2D word, 32bits, composed of the 4 passed on parameters
 */
u32 byte_to_dword(u8 b3, u8 b2, u8 b1, u8 b0);

#endif
