/*
Stuff about bitboards.
*/

#include <stdint.h>
#include <limits.h>

#include "defs.h"

const int BitTable[64] = {
	63, 30, 3, 32, 25, 41, 22, 33, 15, 50, 42, 13, 11, 53, 19, 34, 61, 29, 2,
	51, 21, 43, 45, 10, 18, 47, 1, 54, 9, 57, 0, 35, 62, 31, 40, 4, 49, 5, 52,
	26, 60, 6, 23, 44, 46, 27, 56, 16, 7, 39, 48, 24, 59, 14, 12, 55, 38, 28,
	58, 20, 37, 17, 36, 8
};

// Originally taken from Vice bitboards.c
uint8_t pop_bit(uint64_t* bb) {
	uint64_t b = *bb ^ (*bb - 1);
	uint32_t fold = (unsigned) ((b & 0xffffffff) ^ (b >> 32));
	*bb &= (*bb - 1);
	return BitTable[(fold * 0x783a9b23) >> 26];
}

uint64_t pop_bitboard(uint64_t* bb) {
	return SQTOBB(pop_bit(bb));
}

int popcount(uint64_t x) {
	// TODO: may not work on systems
	// with 32 bit unsigned long.
	// See: https://gcc.gnu.org/onlinedocs/gcc/Other-Builtins.html
	return __builtin_popcountl(x);
}