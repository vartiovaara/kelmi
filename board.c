/*
Describes a board and has all the
things related to modifying a board.
*/

#ifndef BOARD_C
#define BOARD_C

#include <stdbool.h>
#include <stdint.h>
#include <ctype.h>

#include "defs.h"


board_t getstartingboard() {
	board_t board;

	uint8_t default_board[64] = {
		ROOK, KNIGHT, BISHOP, QUEEN, KING, BISHOP, KNIGHT, ROOK,
		PAWN, PAWN, PAWN, PAWN, PAWN, PAWN, PAWN, PAWN,
		EMPTY_PIECE, EMPTY_PIECE, EMPTY_PIECE, EMPTY_PIECE, EMPTY_PIECE, EMPTY_PIECE, EMPTY_PIECE, EMPTY_PIECE,
		EMPTY_PIECE, EMPTY_PIECE, EMPTY_PIECE, EMPTY_PIECE, EMPTY_PIECE, EMPTY_PIECE, EMPTY_PIECE, EMPTY_PIECE,
		EMPTY_PIECE, EMPTY_PIECE, EMPTY_PIECE, EMPTY_PIECE, EMPTY_PIECE, EMPTY_PIECE, EMPTY_PIECE, EMPTY_PIECE,
		EMPTY_PIECE, EMPTY_PIECE, EMPTY_PIECE, EMPTY_PIECE, EMPTY_PIECE, EMPTY_PIECE, EMPTY_PIECE, EMPTY_PIECE,
		PAWN, PAWN, PAWN, PAWN, PAWN, PAWN, PAWN, PAWN,
		ROOK, KNIGHT, BISHOP, QUEEN, KING, BISHOP, KNIGHT, ROOK
	};
	for (int i = 0; i < 64; i++) {
		board.pieces[i] = default_board[i];
	}

	// Setting the colours
	for (int i = 0; i < 3*8; i++) {
		board.colour[i] = WHITE;
	}
	for (int i = 3*8; i < 7*8; i++) {
		board.colour[i] = EMPTY_COLOR;
	}
	for (int i = 7*8; i < 64; i++) {
		board.colour[i] = BLACK;
	}

	// bitboards
	board.w_bitboard = 0xffff000000000000;
	board.b_bitboard = 0x000000000000ffff;

	board.ply = 0;
	board.castle = CASTLE_WK | CASTLE_WQ | CASTLE_BK | CASTLE_BQ;
	board.whiteturn = true;

	return board;
}

void printboard(const board_t* restrict board) {
	for (int i = 64-8; i >= 0; i=i-8) {
		for (int j = 0; j < 8; j++) {
			char ch = piece_char[board->pieces[i+j]];
			if (board->colour[i+j] == WHITE)
				ch = toupper(ch);
			printf("%c", ch);
		}
		printf("\n");
	}
}

void movepiece(board_t* restrict board, const int from, const int to) {
	// move piece
	board->pieces[to] = board->pieces[from];
	board->pieces[from] = EMPTY_PIECE;

	// change colour
	board->colour[to] = board->colour[from];

	// change bitboards
	board->w_bitboard |= ~SQTOBB(from);
	board->b_bitboard |= ~SQTOBB(from);
	board->w_bitboard |= SQTOBB(to);
	board->b_bitboard |= SQTOBB(to);
}

#endif
