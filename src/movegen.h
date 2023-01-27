#ifndef ATTACK_H
#define ATTACK_H

#include "defs.h"

// side: who is attacking
bool is_side_attacking_sq(const board_s* board, const BitBoard sq, const unsigned int side);

// Returns the squares of every piece of side that "sees" square
// Does not check for legality of squares
// Pieces set in ignoremask are ignored.
BitBoard get_attackers(const board_s* board, const BitBoard sq, const unsigned int side, const BitBoard ignoremask);

// Gets all pieces that see sq.
// Pieces set in ignoremask are ignored.
BitBoard get_seeing_pieces(const board_s* board, BitBoard sq, BitBoard ignoremask);

// side: the side whose king to check(which side to check for)
bool is_in_check(const board_s* board, const unsigned int side);

// True if promote available in 1 move for side
// Does not care for board->sidetomove
bool promote_available(const board_s* board, const unsigned int side);

// sets all of the flags and piece_captured.
// everything else exept flags needs to be already set: from, to, fromtype, side and promoteto ...
// NOTE: Does not set promotion flag
void set_move_flags(move_s* move, const board_s* board);

// sets everything needed in move
// just give valid board, form, to and if needed; promoteto
void create_move(const board_s* board, move_s* move, BitBoard from, BitBoard to, unsigned int promoteto);

// generates all the squares the specified piece could move
// currently just pseudo-legal so doesn't check for
movelist_s get_pseudo_legal_squares(const board_s* board, const BitBoard piecebb);

#endif // ATTACK_H
