#ifndef BOARD_H
#define BOARD_H

#include "defs.h"


/*
 * Prints the boards layout with symbols
 * defined in piece_chars[N_PIECES]
 */
void printboard(const board_s* board);

void printbitboard(const BitBoard bb);

board_s boardfromfen(const char* fen_str);

void resetboard(board_s* board);

/*
 * Moves a piece from from to to
 * Doesn't "perform" a move (change en_passant, whiteturn etc.)
 */
void movepiece(board_s* board, const unsigned int type, const BitBoard from, const BitBoard to);

void removepiece(board_s* board, const BitBoard pos, const unsigned int side, const unsigned int type);

// Performs a move. Do not use with wrong side piece.
// NOTE: Currently relies heavily on board->sidetomove in logic
// TODO: Maybe add a side variable for a move
void makemove(board_s* restrict board, const move_s* restrict move);

// Undoes the latest move done (function not done)
void unmakemove(board_s* board);

/*
 * Performs a castling. See castling_e enum in defs.h.
 * Also revokes castling sides castling permissions.
 */
void performcastle(board_s* board, const unsigned int castle);

// Finds, which one of the bitboards holds the piece.
// exit(1) on not found
unsigned int get_piece_type(const board_s* board, const unsigned int side, const BitBoard piecebb);

// Returns, what side the piece is
unsigned int get_piece_side(const board_s* board, const BitBoard piecebb);

void set_move_history_size(board_s* board, const size_t size);

void append_to_move_history(board_s* board, const move_s* move);

void free_move_history(const board_s* board);

// Made some changes to "to" but now want to revert to old copy ("from")?
// This function copies everything exept move history memory information.
void restore_board(board_s* restrict to, board_s* restrict from);

#endif // BOARD_H
