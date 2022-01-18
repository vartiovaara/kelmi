#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "defs.h"

#include <assert.h>

// The queen lookup will just be (rook | bishop)
uint64_t kinglookup[64];
uint64_t rooklookup[64];
uint64_t bishoplookup[64];
uint64_t knightlookup[64];
uint64_t pawnlookup[2][64]; // black and white pawns

// pointers to according lookups
// needs to be void* becouse pawnlookup has different size
void* lookup[N_PIECES]; //[piece_e]


// Private functions
void compute_king_lookup();
void compute_rook_lookup();
void compute_bishop_lookup();
void compute_knight_lookup();
void compute_white_pawn_lookup();


// side can be anything when using anything other than pawn
uint64_t piecelookup(unsigned int pos, unsigned int piece, unsigned int side) {
	assert(pos < 64);
	assert(piece < N_PIECES);
	assert(side == WHITE || side == BLACK);
	if (piece == QUEEN) {
		return ((uint64_t*)lookup[ROOK])[pos] | ((uint64_t*)lookup[BISHOP])[pos];
	}
	if (piece == PAWN) {
		return ((uint64_t**)lookup[PAWN])[side][pos];
	}
	assert(lookup[piece]); // make sure is not NULL
	return ((uint64_t*)lookup[piece])[pos];
}

void reset_lookups() {
	memset(&kinglookup, 0, sizeof kinglookup);
	memset(&rooklookup, 0, sizeof rooklookup);
	memset(&bishoplookup, 0, sizeof bishoplookup);
	memset(&knightlookup, 0, sizeof knightlookup);
	memset(&pawnlookup, 0, sizeof pawnlookup);
	memset(&lookup, 0, sizeof lookup);
}

void compute_lookups() {
	compute_king_lookup();
	compute_rook_lookup();
	compute_bishop_lookup();
	compute_knight_lookup();
	compute_white_pawn_lookup();
}

void set_lookup_pointers() {
	lookup[KING] = kinglookup;
	lookup[QUEEN] = NULL;
	lookup[ROOK] = rooklookup;
	lookup[BISHOP] = bishoplookup;
	lookup[KNIGHT] = knightlookup;
	lookup[PAWN] = pawnlookup;
}

void compute_king_lookup() {
	for (unsigned int i = 0; i < 64; i++) {
		const uint64_t pos = SQTOBB(i); // cuck thing to do
		 // north
		if (!(pos & TOP_MASK)) {
			kinglookup[i] |= pos<<8;
			if (!(pos & RIGHT_MASK)) // north-east
				kinglookup[i] |= pos << 9;
			if (!(pos & LEFT_MASK)) // north-west
				kinglookup[i] |= pos << 7;
		}
		// east
		if (!(pos & RIGHT_MASK))
			kinglookup[i] |= pos<<1;
		// south
		if (!(pos & BOTTOM_MASK)) {
			kinglookup[i] |= pos>>8;
			if(!(pos & RIGHT_MASK)) // south-east
				kinglookup[i] |= pos>>7;
			if(!(pos & LEFT_MASK)) // south-west
				kinglookup[i] |= pos>>9;
		}
		// west
		if (!(pos & LEFT_MASK))
			kinglookup[i] |= pos>>1;
	}
}

// TODO: Test eligibility
void compute_rook_lookup() {
	for (unsigned int i = 0; i < 64; i++) {
		const uint64_t pos = SQTOBB(i);

		// Left moves
		for (int x = (i%8); x > 0; x--) {
			rooklookup[i] |= pos>>x;
		}
		// Right moves
		for (int x = 7-(i%8); x > 0; x--) {
			rooklookup[i] |= pos<<x;
		}
		// Top moves
		uint64_t c_pos = pos;
		ROOK_TOP:
		if (!(c_pos & TOP_MASK)) {
			c_pos <<= 8;
			rooklookup[i] |= c_pos;
			goto ROOK_TOP;
		}
		// Bottom moves
		c_pos = pos;
		ROOK_BOTTOM:
		if (!(c_pos & BOTTOM_MASK)) {
			c_pos >>= 8;
			rooklookup[i] |= c_pos;
			goto ROOK_BOTTOM;
		}
	}
}

// TODO: Test eligibility
void compute_bishop_lookup() {
	for (unsigned int i = 0; i < 64; i++) {
		const uint64_t pos = SQTOBB(i);

		// north-east
		uint64_t c_pos = pos;
		BISHOP_NE:
		if (!(c_pos & TOP_MASK) && !(c_pos & RIGHT_MASK)) {
			c_pos <<= 9;
			bishoplookup[i] |= c_pos;
			goto BISHOP_NE;
		}
		// south-east
		c_pos = pos;
		BISHOP_SE:
		if(!(c_pos & RIGHT_MASK) && !(c_pos & BOTTOM_MASK)) {
			c_pos >>= 7;
			bishoplookup[i] |= c_pos;
			goto BISHOP_SE;
		}
		// south-west
		c_pos = pos;
		BISHOP_SW:
		if(!(c_pos & BOTTOM_MASK) && !(c_pos & LEFT_MASK)) {
			c_pos >>= 9;
			bishoplookup[i] |= c_pos;
			goto BISHOP_SW;
		}
		// north-west
		c_pos = pos;
		BISHOP_NW:
		if(!(c_pos & TOP_MASK) && !(c_pos & LEFT_MASK)) {
			c_pos <<= 7;
			bishoplookup[i] |= c_pos;
			goto BISHOP_NW;
		}
	}
}

void compute_knight_lookup() {
	for (unsigned int i = 0; i < 64; i++) {
		const uint64_t pos = SQTOBB(i);

		// north-north
		if (!(pos & TOP_MASK_N)) {
			if (!(pos & RIGHT_MASK)) // east
				knightlookup[i] |= pos<<17;
			if (!(pos & LEFT_MASK)) // west
				knightlookup[i] |= pos<<15;
		}
		// east-east
		if (!(pos & RIGHT_MASK_N)) {
			if (!(pos & TOP_MASK)) // north
				knightlookup[i] |= pos<<10;
			if (!(pos & BOTTOM_MASK)) // south
				knightlookup[i] |= pos>>6;
		}
		// south-south
		if (!(pos & BOTTOM_MASK_N)) {
			if (!(pos & RIGHT_MASK)) // east
				knightlookup[i] |= pos>>15;
			if (!(pos & LEFT_MASK)) // west
				knightlookup[i] |= pos>>17;
		}
		// west-west
		if (!(pos & LEFT_MASK_N)) {
			if (!(pos & TOP_MASK)) // north
				knightlookup[i] |= pos<<6;
			if (!(pos & BOTTOM_MASK)) // south
				knightlookup[i] |= pos>>10;
		}
	}
}

// TODO: Test eligibility
void compute_white_pawn_lookup() {
	for (unsigned int i = 0; i < 64; i++) {
		// without the cast, all hell breaks loose
		const uint64_t pos = SQTOBB(i);

		// On rank 8. attack will be empty
		if (pos & TOP_MASK)
			continue;
		// one forward
		pawnlookup[WHITE][i] |= pos<<8;
		// Not a-file. top-left capture available
		if (!(pos & LEFT_MASK))
			pawnlookup[WHITE][i] |= pos<<7;
		// Not h-file. top-right capture available
		if (!(pos & RIGHT_MASK))
			pawnlookup[WHITE][i] |= pos<<9;
		// On rank 2. double push available
		if(i >= 8 && i <= 15)
			pawnlookup[WHITE][i] |= pos<<16;
	}
}

// TODO: Test eligibility
void compute_black_pawn_lookup() {
	for (unsigned int i = 0; i < 64; i++) {
		const uint64_t pos = SQTOBB(i);

		// On rank 1. attack will be empty
		if (pos & BOTTOM_MASK)
			continue;
		// one forward
		pawnlookup[BLACK][i] |= pos>>8;
		// Not a-file. left capture available
		if (!(pos & LEFT_MASK))
			pawnlookup[BLACK][i] |= pos>>9;
		// Not h-file. right capture available
		if (!(pos & RIGHT_MASK))
			pawnlookup[BLACK][i] |= pos>>7;
		// On rank 7. double push available
		if(i >= 48 && i <= 55)
			pawnlookup[BLACK][i] |= pos>>16;
	}
}
