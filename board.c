/*
Stuff about boards and bitboards.
*/

#ifndef BOARD_C
#define BOARD_C

#include <stdint.h>

#include "defs.h"

// flips a bitboard 90 degrees
// see:
// https://www.chessprogramming.org/Flipping_Mirroring_and_Rotating#Vertical
uint64_t bbverticalflip(uint64_t bb) {
	const uint64_t k1 = 0x00FF00FF00FF00FF;
	const uint64_t k2 = 0x0000FFFF0000FFFF;
	bb = ((bb >>  8) & k1) | ((bb & k1) <<  8);
	bb = ((bb >> 16) & k2) | ((bb & k2) << 16);
	bb = ( bb >> 32)       | ( bb       << 32);
	return bb;
}

#endif