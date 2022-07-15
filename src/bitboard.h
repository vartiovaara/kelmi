
#ifndef BITBOARD_H
#define BITBOARD_H

#include "defs.h"


// Returns index of the lowest bit and sets it to 0
// Originally taken from Vice bitboards.c
// NOTE: Shits itself when bb is 0
unsigned int pop_bit(BitBoard* const bb);

// Returns the lowest bit and sets it to 0
BitBoard pop_bitboard(BitBoard* const bb);

// Returns index of lowest bit
// NOTE: Shits itself when bb is 0
unsigned int lowest_bitindex(const BitBoard bb);

// Returns the lowest bit as a bitboard
BitBoard lowest_bitboard(const BitBoard bb);

unsigned int popcount(const BitBoard bb);

BitBoard flip_vertical(const BitBoard bb);


#endif // BITBOARD_H
