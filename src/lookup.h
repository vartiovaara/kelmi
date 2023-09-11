#ifndef LOOKUP_H
#define LOOKUP_H

#include "defs.h"



extern BitBoard kinglookup[64];
extern BitBoard knightlookup[64];


extern uint64_t hash_rand_piece[2][N_PIECES][64]; // [side][piece][sq]
extern uint64_t hash_rand_castle[2*2*2*2]; // castle flags use only 4 bits
extern uint64_t hash_rand_enpassant[64]; // [sq]
extern uint64_t hash_rand_sidetomove[2]; // [side]

extern BitBoard ray_attacks[64][64]; // [from][to]

// is from the perspective of white
extern BitBoard passed_pawn_opponent_mask[64]; // [sq]

static const BitBoard columns[] = {
	0x101010101010101,
	0x202020202020202,
	0x404040404040404,
	0x808080808080808,
	0x1010101010101010,
	0x2020202020202020,
	0x4040404040404040,
	0x8080808080808080,
	0x0
};


// side can be anything when using anything other than pawn
BitBoard piecelookup(unsigned int pos, unsigned int piece, unsigned int side);

BitBoard rowlookup(unsigned int pos);

// BitBoard columnlookup(unsigned int pos);

BitBoard king_guard_lookup(unsigned int pos);

// // To will be a rey direction from a compass rose
// // Direction also can be any offset of two pieces.
// Gives ray attack from from to the direction of to.
// Ray will not terminate at to, will continue past.
// If no ray attack exists, empty bitboard is returned.
// BitBoard ray_attack_b(unsigned int from, unsigned int to);
// BitBoard ray_attack_r(unsigned int from, unsigned int to);

void reset_lookups();
void compute_lookups();
void set_lookup_pointers();


#endif // LOOKUP_H
