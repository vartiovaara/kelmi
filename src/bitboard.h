
#ifndef BITBOARD_H
#define BITBOARD_H

#include "defs.h"


// Returns index of the lowest bit and sets it to 0
// Originally taken from Vice bitboards.c
unsigned int pop_bit(BitBoard* const);

// Returns the lowest bit and sets it to 0
BitBoard pop_bitboard(BitBoard* const);

// Returns index of lowest bit
unsigned int lowest_bitindex(const BitBoard);

// Returns the lowest bit as a bitboard
BitBoard lowest_bitboard(const BitBoard);

int popcount(BitBoard);


#endif // BITBOARD_H