#ifndef ATTACK_H
#define ATTACK_H

#include "defs.h"

// side: who is attacking
bool is_side_attacking_sq(const board_s* board, const BitBoard sq, const unsigned int side);

// side: the side whose king to check(which side to check for)
bool is_in_check(const board_s* board, const unsigned int side);

// generates all the squares the specified piece could move
// currently just pseudo-legal so doesn't check for
movelist_s pseudo_legal_squares(const board_s* board, const BitBoard piecebb);

#endif // ATTACK_H
