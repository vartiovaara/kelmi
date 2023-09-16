
#ifndef BITBOARD_H
#define BITBOARD_H

#include "defs.h"

#include <assert.h>

// Following code was obtained from here:
// https://stackoverflow.com/a/46137633/17151125

#ifdef _MSC_VER

#include <stdlib.h>
#define bswap_32(x) _byteswap_ulong(x)
#define bswap_64(x) _byteswap_uint64(x)

#elif defined(__APPLE__)

// Mac OS X / Darwin features
#include <libkern/OSByteOrder.h>
#define bswap_32(x) OSSwapInt32(x)
#define bswap_64(x) OSSwapInt64(x)

#elif defined(__sun) || defined(sun)

#include <sys/byteorder.h>
#define bswap_32(x) BSWAP_32(x)
#define bswap_64(x) BSWAP_64(x)

#elif defined(__FreeBSD__)

#include <sys/endian.h>
#define bswap_32(x) bswap32(x)
#define bswap_64(x) bswap64(x)

#elif defined(__OpenBSD__)

#include <sys/types.h>
#define bswap_32(x) swap32(x)
#define bswap_64(x) swap64(x)

#elif defined(__NetBSD__)

#include <sys/types.h>
#include <machine/bswap.h>
#if defined(__BSWAP_RENAME) && !defined(__bswap_32)
#define bswap_32(x) bswap32(x)
#define bswap_64(x) bswap64(x)
#endif

#else

#include <byteswap.h>

#endif
// --- Stop stack overflow code ---






// Returns index of lowest bit
// NOTE: Shits itself when bb is 0	
// #if test which type is enough to hold 64 bits
// See: https://gcc.gnu.org/onlinedocs/gcc/Other-Builtins.html
#if UINT_MAX==18446744073709551615ULL
#define LOWEST_BITINDEX(bb) __builtin_ctz(bb)
#elif ULONG_MAX==18446744073709551615ULL
#define LOWEST_BITINDEX(bb) __builtin_ctzl(bb)
#elif ULLONG_MAX>=18446744073709551615ULL
#define LOWEST_BITINDEX(bb) __builtin_ctzll(bb)
#endif


// Returns the lowest bit as a bitboard
#define LOWEST_BITBOARD(bb) ((bb) ^ ((bb) & ((bb) - 1)))


// Returns the index of the highest bit set
#if UINT_MAX==18446744073709551615ULL
#define HIGHEST_BITINDEX(bb) (63 - __builtin_clz(bb))
#elif ULONG_MAX==18446744073709551615ULL
#define HIGHEST_BITINDEX(bb) (63 - __builtin_clzl(bb))
#elif ULLONG_MAX>=18446744073709551615ULL
#define HIGHEST_BITINDEX(bb) (63 - __builtin_clzll(bb))
#endif


// See: http://0x80.pl/articles/sse-popcount.html
// See: https://en.wikipedia.org/wiki/Hamming_weight#Efficient_implementation
// #if test which type is enough to hold 64 bits
// See: https://gcc.gnu.org/onlinedocs/gcc/Other-Builtins.html
#if UINT_MAX==18446744073709551615ULL
#define POPCOUNT(x) __builtin_popcount(x)
#elif ULONG_MAX==18446744073709551615ULL
#define POPCOUNT(x) __builtin_popcountl(x)
#elif ULLONG_MAX>=18446744073709551615ULL
#define POPCOUNT(x) __builtin_popcountll(x)
#endif



// Doesn't work for non x86-64 machines??? maybe??
//#define FLIP_VERTICAL(bb) bswap_64(bb)
#define FLIP_VERTICAL(bb) __builtin_bswap64(bb)



// Returns index of the lowest bit and sets it to 0
// Originally taken from Vice bitboards.c
// NOTE: Shits itself when bb is 0
static inline unsigned int pop_bit(BitBoard* const bb) {
	assert(*bb > 0);
	
	const unsigned int index = LOWEST_BITINDEX(*bb);
	*bb &= *bb - 1;
	return index;
	
	/*
	BitBoard b = *bb ^ (*bb - 1);
	unsigned int fold = (unsigned) ((b & 0xffffffff) ^ (b >> 32));
	*bb &= *bb - 1;
	return BitTable[(fold * 0x783a9b23) >> 26];
	*/
}

// Returns the lowest bit and sets it to 0
static inline BitBoard pop_bitboard(BitBoard* const bb) {
	assert(*bb > 0);
	const BitBoard bb_copy = *bb;
	*bb &= *bb - 1; // remove the lowest bit
	return *bb ^ bb_copy; // return what was changed
    //#define POP_BITBOARD(bb) (bb ^ (bb &= bb-1)) // does the same thing but 1 liner
}



#endif // BITBOARD_H
