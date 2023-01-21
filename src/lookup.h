#ifndef LOOKUP_H
#define LOOKUP_H

#include "defs.h"

extern uint64_t hash_rand_piece[2][N_PIECES][64]; // [side][piece][sq]
extern uint64_t hash_rand_castle[2*2*2*2]; // castle flags use only 4 bits
extern uint64_t hash_rand_enpassant[64]; // [sq]
extern uint64_t hash_rand_sidetomove[2]; // [side]

// side can be anything when using anything other than pawn
BitBoard piecelookup(unsigned int pos, unsigned int piece, unsigned int side);

BitBoard rowlookup(unsigned int pos);

BitBoard columnlookup(unsigned int pos);

BitBoard king_guard_lookup(unsigned int pos);

void reset_lookups();
void compute_lookups();
void set_lookup_pointers();


#endif // LOOKUP_H
