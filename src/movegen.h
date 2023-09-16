#ifndef ATTACK_H
#define ATTACK_H

#include "defs.h"

// attack_mask: who is attacking
bool is_attacking_squares(const board_s* board, const BitBoard sq, const BitBoard attack_mask);

bool is_attacking_square(const board_s* board, const BitBoard sq, const BitBoard attack_mask);

BitBoard opponent_attacks_squares(const board_s* restrict board, const BitBoard sq);

BitBoard own_attacks_squares(const board_s* restrict board, const BitBoard sq);



// same as above but sq can have multiple squares set
// bool is_side_attacking_squares(const board_s* board, const BitBoard sq, const unsigned int side);

// Returns the squares of every piece of side that "sees" square
// Does not check for legality of squares
// Pieces set in ignoremask are ignored.
// BitBoard get_attackers(const board_s* board, const BitBoard sq, const unsigned int side, const BitBoard ignoremask);

// Gets all pieces that see sq.
// Pieces set in ignoremask are ignored.
BitBoard get_seeing_pieces(const board_s* board, BitBoard sq, BitBoard ignoremask);

// side: the side whose king to check(which side to check for)
bool is_in_check(const board_s* board, const unsigned int side);

BitBoard in_check(const board_s* restrict board);

BitBoard opponent_in_check(const board_s* restrict board);

// Generates captures on attacker and attack blocks
BitBoard check_blocks(const board_s* restrict board, const BitBoard attacker);

// Generates pinned pieces
BitBoard pinned_pieces(const board_s* restrict board);

BitBoard is_move_illegal(const board_s* restrict board, move_s move);


// True if promote available in 1 move for side
// Does not care for board->sidetomove
// bool promote_available(const board_s* board, const unsigned int side);

// sets all of the flags and piece_captured.
// everything else exept flags needs to be already set: from, to, fromtype, side and promoteto ...
// NOTE: Does not set promotion flag
move_s set_move_flags(move_s move, const board_s* restrict board);

// sets everything needed in move
// just give valid board, form, to and if needed; promoteto
// void create_move(const board_s* board, move_s* move, BitBoard from, BitBoard to, unsigned int promoteto);

// Makes a null move of *move
// void construct_null_move(const board_s* restrict board, move_s* restrict move);

// generates all the moves the specified piece could move
// currently just pseudo-legal so doesn't check for
// ignoremask sets what to-squares are ignored. ofc. set to 0x0 for all moves.
void get_pseudo_legal_moves(const board_s* restrict board, movelist_s* restrict moves, const BitBoard piecebb, bool set_move_ordering, BitBoard ignoremask);

// If mask_defends==true, do not return squares with own pieces
//BitBoard get_pseudo_legal_squares(const board_s* restrict board, unsigned int side, unsigned int piece_type, BitBoard piecebb);
// BitBoard get_pseudo_legal_squares(const board_s* restrict board, unsigned int side, unsigned int piece_type, BitBoard piecebb, bool mask_defends);

BitBoard k_attacks(BitBoard kings);
BitBoard n_attacks(BitBoard knights);
// BitBoard p_attacks_own(BitBoard pawns);
// BitBoard p_attacks_opponent(BitBoard pawns);


BitBoard r_attacks(BitBoard rooks,   BitBoard empty);
BitBoard b_attacks(BitBoard bishops, BitBoard empty);
BitBoard q_attacks(BitBoard queens,  BitBoard empty);


#endif // ATTACK_H
