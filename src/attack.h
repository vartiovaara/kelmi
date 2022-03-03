#ifndef ATTACK_H
#define ATTACK_H

#include "defs.h"

// side: the side whose king to check(which side to check for)
bool is_in_check (const board_s* board, const unsigned int side);


// generates all the squares the specified piece could move
// currently just pseudo-legal so doesn't check for
movelist_s pseudo_legal_squares(const board_s* board, const BitBoard piecebb);


BitBoard pseudo_legal_squares_k(const board_s* board, const unsigned int side, const BitBoard piece);
BitBoard pseudo_legal_squares_n(const board_s* board, const unsigned int side, const BitBoard piece);
BitBoard pseudo_legal_squares_q(const board_s* board, const unsigned int side, const BitBoard piece);
BitBoard pseudo_legal_squares_b(const board_s* board, const unsigned int side, const BitBoard piece);
BitBoard pseudo_legal_squares_r(const board_s* board, const unsigned int side, const BitBoard piece);
BitBoard pseudo_legal_squares_p(const board_s* board, const unsigned int side, const BitBoard piece);

#endif // ATTACK_H