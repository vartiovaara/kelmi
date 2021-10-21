/*
Attack stuff.
*/

#include <stdio.h>
#include <stdlib.h>

#include "defs.h"

uint64_t pseudo_legal_squares(const board_s* board, const uint8_t side, const uint64_t piece) {
	const uint8_t piece_type = get_piece_type(board, side, piece);

	//
	if (piece_type == KING) {
		return pseudo_legal_squares_k(board, side, piece);
	}
	
	fprintf(stderr, "pseudo_legal_squares(board, %u, %p)", side, (void*)piece);
	printboard(board);
	exit(1);
}

uint64_t pseudo_legal_squares_k(const board_s* board, const uint8_t side, const uint64_t piece) {
	uint64_t squares = 0x0;
	if (!(piece & TOP_MASK)) {
		squares |= MV_N(piece);
		if (!(piece & LEFT_MASK))
			squares |= MV_DIAG_NW(piece);
		if (!(piece & RIGHT_MASK))
			squares |= MV_DIAG_NE(piece);
	}
	if (!(piece & BOTTOM_MASK)) {
		squares |= MV_S(piece);
		if (!(piece & LEFT_MASK))
			squares |= MV_DIAG_SW(piece);
		if (!(piece & RIGHT_MASK))
			squares |= MV_DIAG_SE(piece);
	}
	if (!(piece & RIGHT_MASK))
		squares |= MV_E(piece);
	if (!(piece & LEFT_MASK))
		squares |= MV_W(piece);
	// negate eating own pieces
	squares &= ~board->all_pieces[side]; 
	return squares;
}