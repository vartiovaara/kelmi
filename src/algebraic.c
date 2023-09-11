/*
Stuff with algabreic move representation.
*/

#include <stdint.h>

#include "algebraic.h"

#include "bitboard.h"

#include "defs.h"


const char piece_chars[] = {
	[KING] = 'k',
	[QUEEN] = 'q',
	[BISHOP] = 'b',
	[KNIGHT] = 'n',
	[ROOK] = 'r',
	[PAWN] = 'p'
};


char piecetochar(const unsigned int piece) {
	return piece_chars[piece];
}

BitBoard algsqtobb(const char* alg) {
	return SQTOBB(algsqtoint(alg));
}


int algsqtoint(const char* alg) {
	return (8*(alg[1]-'1')+(alg[0]-'a'));
}


void sqtoalg(char* alg, const unsigned int sq) {
	alg[0] = 'a' + (sq % 8);
	alg[1] = '1' + (int)(sq / 8);
}


void bbtoalg(char* alg, const BitBoard bb) {
	sqtoalg(alg, LOWEST_BITINDEX(bb));
}
