#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "lookup.h"

#include "defs.h"


// The queen lookup will just be (rook | bishop)
BitBoard rows[8];
BitBoard columns[8];
BitBoard kinglookup[64];
BitBoard rooklookup[64];
BitBoard bishoplookup[64];
BitBoard knightlookup[64];
BitBoard pawnlookup[2][64]; // black and white pawns [side][sq_n]

// pointers to according lookups
BitBoard* lookup[N_PIECES]; //[piece_e]

// see eval.c
BitBoard kingguardlookup[64];



// Private functions
void compute_row_lookup();
void compute_column_lookup();

void compute_king_lookup();
void compute_rook_lookup();
void compute_bishop_lookup();
void compute_knight_lookup();
void compute_white_pawn_lookup();
void compute_black_pawn_lookup();

void compute_king_guard_lookup();



// side can be anything when using anything other than pawn
BitBoard piecelookup(unsigned int pos, unsigned int piece, unsigned int side) {
	assert(pos < 64);
	assert(piece < N_PIECES);
	assert(side == WHITE || side == BLACK);

	if (piece == QUEEN) {
		return lookup[ROOK][pos] | lookup[BISHOP][pos];
	}
	if (piece == PAWN) {
		return pawnlookup[side][pos];
	}
	assert(lookup[piece]); // make sure is not NULL
	return lookup[piece][pos];
}


BitBoard rowlookup(unsigned int pos) {
	return rows[pos / 8];
}


BitBoard columnlookup(unsigned int pos) {
	return columns[pos % 8];
}

BitBoard king_guard_lookup(unsigned int pos) {
	return kingguardlookup[pos];
}



void reset_lookups() {
	memset(&rows, 0, sizeof rows);
	memset(&columns, 0, sizeof columns);
	memset(&kinglookup, 0, sizeof kinglookup);
	memset(&rooklookup, 0, sizeof rooklookup);
	memset(&bishoplookup, 0, sizeof bishoplookup);
	memset(&knightlookup, 0, sizeof knightlookup);
	memset(&pawnlookup, 0, sizeof pawnlookup);
	memset(&lookup, 0, sizeof lookup);
	memset(&kingguardlookup, 0, sizeof kingguardlookup);
}

void compute_lookups() {
	compute_row_lookup();
	compute_column_lookup();
	compute_king_lookup();
	compute_rook_lookup();
	compute_bishop_lookup();
	compute_knight_lookup();
	compute_white_pawn_lookup();
	compute_black_pawn_lookup();
	compute_king_guard_lookup();
}

void set_lookup_pointers() {
	lookup[KING] = kinglookup;
	lookup[QUEEN] = NULL;
	lookup[ROOK] = rooklookup;
	lookup[BISHOP] = bishoplookup;
	lookup[KNIGHT] = knightlookup;
	lookup[PAWN] = NULL; // has a separate array
}


// TODO: Confirm validity
void compute_row_lookup() {
	for (unsigned int i = 0; i < 8; i++) {
		const BitBoard pos = SQTOBB(i*8);

		// moves
		for (int x = 0; x < 8; x++) {
			rows[i] |= pos<<x;
		}
	}
}


// TODO: Confirm validity
void compute_column_lookup() {
	for (unsigned int i = 0; i < 8; i++) {
		const BitBoard pos = SQTOBB(i);

		// moves
		for (int x = 0; x < 8; x++) {
			columns[i] |= pos<<(x*8);
		}
	}
}


void compute_king_lookup() {
	for (unsigned int i = 0; i < 64; i++) {
		const BitBoard pos = SQTOBB(i);
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
		const BitBoard pos = SQTOBB(i);

		// Left moves
		for (int x = (i%8); x > 0; x--) {
			rooklookup[i] |= pos>>x;
		}
		// Right moves
		for (int x = 7-(i%8); x > 0; x--) {
			rooklookup[i] |= pos<<x;
		}
		// Top moves
		BitBoard c_pos = pos;
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
		const BitBoard pos = SQTOBB(i);

		// north-east
		BitBoard c_pos = pos;
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
		const BitBoard pos = SQTOBB(i);

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
		const BitBoard pos = SQTOBB(i);

		// On rank 8. attack will be empty
		if (pos & TOP_MASK)
			continue;
		// one forward
		//pawnlookup[WHITE][i] |= pos<<8;
		// Not a-file. top-left capture available
		if (!(pos & LEFT_MASK))
			pawnlookup[WHITE][i] |= pos<<7;
		// Not h-file. top-right capture available
		if (!(pos & RIGHT_MASK))
			pawnlookup[WHITE][i] |= pos<<9;
		// On rank 2. double push available
		//if(i >= 8 && i <= 15)
		//	pawnlookup[WHITE][i] |= pos<<16;
	}
}

// TODO: Test eligibility
void compute_black_pawn_lookup() {
	for (unsigned int i = 0; i < 64; i++) {
		const BitBoard pos = SQTOBB(i);

		// On rank 1. attack will be empty
		if (pos & BOTTOM_MASK)
			continue;
		// one forward
		//pawnlookup[BLACK][i] |= pos>>8;
		// Not a-file. left capture available
		if (!(pos & LEFT_MASK))
			pawnlookup[BLACK][i] |= pos>>9;
		// Not h-file. right capture available
		if (!(pos & RIGHT_MASK))
			pawnlookup[BLACK][i] |= pos>>7;
		// On rank 7. double push available
		//if(i >= 48 && i <= 55)
		//	pawnlookup[BLACK][i] |= pos>>16;
	}
}


// TODO: Confirm validity
void compute_king_guard_lookup() {
	for (unsigned int i = 0; i < 64; i++) {
		const BitBoard pos = SQTOBB(i);

		unsigned int space_n = 0, space_e = 0, space_s = 0, space_w = 0;

		space_n = (!(pos & TOP_MASK_N) ? 2 : 1);
		space_e = (!(pos & RIGHT_MASK_N) ? 2 : 1);
		space_s = (!(pos & BOTTOM_MASK_N) ? 2 : 1);
		space_w = (!(pos & LEFT_MASK_N) ? 2 : 1);

		space_n = (pos & TOP_MASK ? 0 : space_n);
		space_e = (pos & RIGHT_MASK ? 0 : space_e);
		space_s = (pos & BOTTOM_MASK ? 0 : space_s);
		space_w = (pos & LEFT_MASK ? 0 : space_w);

		for (unsigned int n = 0; n <= space_n; n++) {
			kingguardlookup[i] |= MV_N(pos, n);
			for (unsigned int e = 0; e <= space_e; e++)
				kingguardlookup[i] |= MV_E(MV_N(pos, n), e);
			for (unsigned int w = 0; w <= space_w; w++)
				kingguardlookup[i] |= MV_W(MV_N(pos, n), w);
		}
		for (unsigned int s = 0; s <= space_s; s++) {
			kingguardlookup[i] |= MV_S(pos, s);
			for (unsigned int e = 0; e <= space_e; e++)
				kingguardlookup[i] |= MV_E(MV_S(pos, s), e);
			for (unsigned int w = 0; w <= space_w; w++)
				kingguardlookup[i] |= MV_W(MV_S(pos, s), w);
		}
		// now we have
		/*
		 * 1 1 1 1 1
		 * 1 1 1 1 1
		 * 1 1 1 1 1
		 * 1 1 1 1 1
		 * 1 1 1 1 1
		 */
	}
}
