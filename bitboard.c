/*
Stuff about bitboards.
*/

#include <stdint.h>
#include <limits.h>

#include "defs.h"

#include <assert.h>

const int BitTable[64] = {
	63, 30, 3, 32, 25, 41, 22, 33, 15, 50, 42, 13, 11, 53, 19, 34, 61, 29, 2,
	51, 21, 43, 45, 10, 18, 47, 1, 54, 9, 57, 0, 35, 62, 31, 40, 4, 49, 5, 52,
	26, 60, 6, 23, 44, 46, 27, 56, 16, 7, 39, 48, 24, 59, 14, 12, 55, 38, 28,
	58, 20, 37, 17, 36, 8
};

// Returns index of the lowest bit and sets it to 0
// Originally taken from Vice bitboards.c
// TODO: there is probably some kind of instruction for this
// FIXME: Probably shits itself when bb is 0
unsigned int pop_bit(uint64_t* const bb) {
	assert(*bb > 0);
	uint64_t b = *bb ^ (*bb - 1);
	unsigned int fold = (unsigned) ((b & 0xffffffff) ^ (b >> 32));
	*bb &= (*bb - 1);
	return BitTable[(fold * 0x783a9b23) >> 26];
}

// Returns the lowest bit and sets it to 0
uint64_t pop_bitboard(uint64_t* const bb) {
	assert(*bb > 0);
	uint64_t bb_copy = *bb;
	*bb &= *bb - 1; // remove the lowest bit
	return *bb ^ bb_copy; // return what was changed
	//return (*bb ^ (*bb &= *bb-1)); // does the same thing but 1 liner
}

// Returns index of lowest bit
// TODO: make speed (__builtin_ffs(int) ?)
// see: https://gcc.gnu.org/onlinedocs/gcc/Other-Builtins.html
// __builtin_ffs(int)
// TODO: Test eligibility of this function
unsigned int lowest_bitindex(const uint64_t bb) {
	//uint64_t bb_copy = bb;
	//return pop_bit(&bb_copy);
	assert(bb > 0);
	uint64_t b = bb ^ (bb - 1);
	unsigned int fold = (unsigned) ((b & 0xffffffff) ^ (b >> 32));
	//*bb &= (*bb - 1);
	return BitTable[(fold * 0x783a9b23) >> 26];
}

// Returns the lowest bit as a bitboard
uint64_t lowest_bitboard(const uint64_t bb) {
	assert(bb > 0);
	return bb ^ (bb & (bb - 1));
}

int popcount(uint64_t x) {
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
