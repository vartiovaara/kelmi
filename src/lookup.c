#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "lookup.h"

#include "random.h"
#include "bitboard.h"

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

/*
// South attacks will be just vert-flipped north attacks
BitBoard b_ray_lookup_ne[64];
// BitBoard b_ray_lookup_se[64];
BitBoard b_ray_lookup_nw[64];
// BitBoard b_ray_lookup_sw[64];

// // s and w attacks will be just vert-flipped n and e attacks
BitBoard r_ray_lookup_n[64];
BitBoard r_ray_lookup_e[64];
BitBoard r_ray_lookup_s[64];
BitBoard r_ray_lookup_w[64];
*/


// These are public
uint64_t hash_rand_piece[2][N_PIECES][64];
uint64_t hash_rand_castle[2*2*2*2]; // castle flags use only 4 bits
uint64_t hash_rand_enpassant[64];
uint64_t hash_rand_sidetomove[2];

BitBoard ray_attacks[64][64]; // [from][to]

// is from the perspective of white
BitBoard passed_pawn_opponent_mask[64]; // [sq]



// Private functions
void compute_row_lookup();
void compute_column_lookup();

void compute_king_lookup();
void compute_rook_lookup();
void compute_bishop_lookup();
void compute_knight_lookup();
void compute_white_pawn_lookup();
void compute_black_pawn_lookup();

void compute_ray_lookups();

void compute_king_guard_lookup();

void compute_passed_pawn_lookup();

void compute_hash_rand();



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


/*
BitBoard ray_attack_b(unsigned int from, unsigned int to) {
	assert(from < 64);
	assert(to < 64);

	BitBoard ray = 0x0;

	const int diff = (int)to - from;

	if (diff % DIRECTION_NE == 0)
		ray = b_ray_lookup_ne[from];
	else if (diff % DIRECTION_NW == 0)
		ray = b_ray_lookup_nw[from];

	if (diff < 0)
		return flip_vertical(ray);
	return ray;
}

BitBoard ray_attack_r(unsigned int from, unsigned int to) {
	assert(from < 64);
	assert(to < 64);

	BitBoard ray = 0x0;

	const int diff = (int)to - from;

	if (diff % DIRECTION_N == 0)
		ray = r_ray_lookup_n[from];
	else if (from/8 == to/8) // are on the same rank
		ray = r_ray_lookup_e[from];

	if (diff < 0)
		return flip_vertical(ray);
	return ray;
}
*/



void reset_lookups() {
	memset(rows, 0, sizeof rows);
	memset(columns, 0, sizeof columns);
	memset(kinglookup, 0, sizeof kinglookup);
	memset(rooklookup, 0, sizeof rooklookup);
	memset(bishoplookup, 0, sizeof bishoplookup);
	memset(knightlookup, 0, sizeof knightlookup);
	memset(pawnlookup, 0, sizeof pawnlookup);
	memset(lookup, 0, sizeof lookup);
	memset(kingguardlookup, 0, sizeof kingguardlookup);
	memset(hash_rand_piece, 0, sizeof hash_rand_piece);
	memset(hash_rand_castle, 0, sizeof hash_rand_castle);
	memset(hash_rand_enpassant, 0, sizeof hash_rand_enpassant);
	/*
	memset(r_ray_lookup_n, 0, sizeof r_ray_lookup_n);
	memset(r_ray_lookup_e, 0, sizeof r_ray_lookup_e);
	memset(b_ray_lookup_ne, 0, sizeof b_ray_lookup_ne);
	memset(b_ray_lookup_nw, 0, sizeof b_ray_lookup_nw);
	*/
	memset(ray_attacks, 0, sizeof ray_attacks);
	memset(passed_pawn_opponent_mask, 0, sizeof passed_pawn_opponent_mask);
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
	compute_hash_rand();
	compute_ray_lookups();
	compute_passed_pawn_lookup();
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


void compute_ray_lookups() {

	for (unsigned int from = 0; from < 64; from++) {

		const BitBoard frombb = SQTOBB(from);
		
		const int dir[] = {9, 1,-7,-8,-9,-1, 7, 8};

		for (unsigned int i = 0; i < LENGTH(dir); i++) {

			if (frombb & TOP_MASK && (dir[i] == DIRECTION_NW || dir[i] == DIRECTION_N || dir[i] == DIRECTION_NE))
				continue;
			if (frombb & BOTTOM_MASK && (dir[i] == DIRECTION_SE || dir[i] == DIRECTION_S || dir[i] == DIRECTION_SW))
				continue;
			if (frombb & RIGHT_MASK && (dir[i] == DIRECTION_E || dir[i] == DIRECTION_NE || dir[i] == DIRECTION_SE))
				continue;
			if (frombb & LEFT_MASK && (dir[i] == DIRECTION_SW || dir[i] == DIRECTION_W || dir[i] == DIRECTION_NW))
				continue;
			

			BitBoard curr_bb;
			if (dir[i] > 0)
				curr_bb = frombb << dir[i];
			else
				curr_bb = frombb >> -dir[i];
			
			BitBoard tobb = curr_bb;
			int to = from + dir[i];
			assert(curr_bb);
			while (true) {
				ray_attacks[from][to] = tobb;

				if (curr_bb & TOP_MASK && (dir[i] == DIRECTION_NW || dir[i] == DIRECTION_N || dir[i] == DIRECTION_NE))
					break;
				if (curr_bb & BOTTOM_MASK && (dir[i] == DIRECTION_SE || dir[i] == DIRECTION_S || dir[i] == DIRECTION_SW))
					break;
				if (curr_bb & RIGHT_MASK && (dir[i] == DIRECTION_E || dir[i] == DIRECTION_NE || dir[i] == DIRECTION_SE))
					break;
				if (curr_bb & LEFT_MASK && (dir[i] == DIRECTION_SW || dir[i] == DIRECTION_W || dir[i] == DIRECTION_NW))
					break;
				
				if (dir[i] > 0)
					curr_bb <<= dir[i];
				else
					curr_bb >>= -dir[i];

				tobb |= curr_bb; // add next square
				to += dir[i];
			}
			//ray_attacks[from]
		}
	}

	// essentially just bishop lookup computing code copied here
	// for (unsigned int i = 0; i < 64; i++) {
	// 	const BitBoard pos = SQTOBB(i);

	// 	// north-east
	// 	BitBoard c_pos = pos;
	// 	B_RAY_NE:
	// 	if (!(c_pos & TOP_MASK) && !(c_pos & RIGHT_MASK)) {
	// 		c_pos <<= 9;
	// 		b_ray_lookup_ne[i] |= c_pos;
	// 		goto B_RAY_NE;
	// 	}
	// 	/*
	// 	// south-east
	// 	c_pos = pos;
	// 	B_RAY_SE:
	// 	if(!(c_pos & RIGHT_MASK) && !(c_pos & BOTTOM_MASK)) {
	// 		c_pos >>= 7;
	// 		bishoplookup[i] |= c_pos;
	// 		goto B_RAY_SE;
	// 	}
	// 	// south-west
	// 	c_pos = pos;
	// 	B_RAY_SW:
	// 	if(!(c_pos & BOTTOM_MASK) && !(c_pos & LEFT_MASK)) {
	// 		c_pos >>= 9;
	// 		bishoplookup[i] |= c_pos;
	// 		goto B_RAY_SW;
	// 	}
	// 	*/
	// 	// north-west
	// 	c_pos = pos;
	// 	B_RAY_NW:
	// 	if(!(c_pos & TOP_MASK) && !(c_pos & LEFT_MASK)) {
	// 		c_pos <<= 7;
	// 		b_ray_lookup_nw[i] |= c_pos;
	// 		goto B_RAY_NW;
	// 	}
	// }

	// for (unsigned int i = 0; i < 64; i++) {
	// 	const BitBoard pos = SQTOBB(i);

	// 	/*
	// 	// Left moves
	// 	for (int x = (i%8); x > 0; x--) {
	// 		rooklookup[i] |= pos>>x;
	// 	}
	// 	*/
	// 	// Right moves
	// 	for (int x = 7-(i%8); x > 0; x--) {
	// 		r_ray_lookup_e[i] |= pos<<x;
	// 	}
	// 	// Top moves
	// 	BitBoard c_pos = pos;
	// 	R_RAY_TOP:
	// 	if (!(c_pos & TOP_MASK)) {
	// 		c_pos <<= 8;
	// 		r_ray_lookup_n[i] |= c_pos;
	// 		goto R_RAY_TOP;
	// 	}
	// 	/*
	// 	// Bottom moves
	// 	c_pos = pos;
	// 	ROOK_BOTTOM:
	// 	if (!(c_pos & BOTTOM_MASK)) {
	// 		c_pos >>= 8;
	// 		rooklookup[i] |= c_pos;
	// 		goto ROOK_BOTTOM;
	// 	}
	// 	*/
	// }
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


void compute_passed_pawn_lookup() {
	for (int sq = 0; sq < 64; sq++) {
		BitBoard starting_squares = SQTOBB(sq);
		if (!(SQTOBB(sq) & LEFT_MASK))
			starting_squares |= SQTOBB(sq + DIRECTION_W);
		if (!(SQTOBB(sq) & RIGHT_MASK))
			starting_squares |= SQTOBB(sq + DIRECTION_E);

		while (starting_squares) {
			BitBoard c_pos = pop_bitboard(&starting_squares);
			ROOK_TOP:
			if (!(c_pos & TOP_MASK)) {
				c_pos <<= 8;
				passed_pawn_opponent_mask[sq] |= c_pos;
				goto ROOK_TOP;
			}
		}
	}
	//passed_pawn_opponent_mask;
}


void compute_hash_rand() {
	// uint64_t hash_rand_piece[2][N_PIECES][64];
	for (int i = 0; i < 2; i++) {
		for (int j = 0; j < N_PIECES; j++) {
			for (int k = 0; k < 64; k++) {
				hash_rand_piece[i][j][k] = genrand64_int64();
			}
		}
	}
	// uint64_t hash_rand_castle[2*2*2*2];
	for (int i = 0; i < 2*2*2*2; i++) {
		hash_rand_castle[i] = genrand64_int64();
	}
	// uint64_t hash_rand_enpassant[64];
	for (int i = 0; i < 64; i++) {
		hash_rand_enpassant[i] = genrand64_int64();
	}
	// uint64_t hash_rand_sidetomove[2];
	hash_rand_sidetomove[0] = genrand64_int64(); // TODO: Have other just be 0x0?? investigate
	//hash_rand_sidetomove[1] = genrand64_int64();
	hash_rand_sidetomove[1] = 0x0;
}
