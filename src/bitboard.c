/*
Stuff about bitboards.
*/

#include <stdint.h>
#include <limits.h>

#include <assert.h>

#include "bitboard.h"

#include "defs.h"

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




// for pop_bit() and lowest_bitindex()
const int BitTable[64] = {
	63, 30, 3, 32, 25, 41, 22, 33, 15, 50, 42, 13, 11, 53, 19, 34, 61, 29, 2,
	51, 21, 43, 45, 10, 18, 47, 1, 54, 9, 57, 0, 35, 62, 31, 40, 4, 49, 5, 52,
	26, 60, 6, 23, 44, 46, 27, 56, 16, 7, 39, 48, 24, 59, 14, 12, 55, 38, 28,
	58, 20, 37, 17, 36, 8
};


// TODO: there is probably some kind of instruction for this
unsigned int pop_bit(BitBoard* const bb) {
	assert(*bb > 0);
	
	BitBoard b = *bb ^ (*bb - 1);
	unsigned int fold = (unsigned) ((b & 0xffffffff) ^ (b >> 32));
	*bb &= *bb - 1;
	return BitTable[(fold * 0x783a9b23) >> 26];
}


BitBoard pop_bitboard(BitBoard* const bb) {
	assert(*bb > 0);
	BitBoard bb_copy = *bb;
	*bb &= *bb - 1; // remove the lowest bit
	return *bb ^ bb_copy; // return what was changed
	//return (*bb ^ (*bb &= *bb-1)); // does the same thing but 1 liner
}


// TODO: make speed (__builtin_ffs(int) ?)
// see: https://gcc.gnu.org/onlinedocs/gcc/Other-Builtins.html
// __builtin_ffs(int)
// TODO: Test eligibility of this function
unsigned int lowest_bitindex(const BitBoard bb) {
	assert(bb > 0);
	
	BitBoard b = bb ^ (bb - 1);
	unsigned int fold = (unsigned) ((b & 0xffffffff) ^ (b >> 32));
	return BitTable[(fold * 0x783a9b23) >> 26];
	//return __builtin_ffsl(bb)-1;
}


BitBoard lowest_bitboard(const BitBoard bb) {
	assert(bb > 0);

	return bb ^ (bb & (bb - 1));
}


unsigned int popcount(const BitBoard x) {
	// See: http://0x80.pl/articles/sse-popcount.html
	// See: https://en.wikipedia.org/wiki/Hamming_weight#Efficient_implementation
	// #if test which type is enough to hold 64 bits
	// See: https://gcc.gnu.org/onlinedocs/gcc/Other-Builtins.html
	#if UINT_MAX==18446744073709551615ULL
	return __builtin_popcount(x);
	#elif ULONG_MAX==18446744073709551615ULL
	return __builtin_popcountl(x);
	#elif ULLONG_MAX>=18446744073709551615ULL
	return __builtin_popcountll(x);
	#endif
}

BitBoard flip_vertical(const BitBoard bb) {
	// Doesn't work for non x86-64 machines??? maybe??
	return bswap_64(bb);
}

