/*
Attack stuff.
*/

#include <stdio.h>
#include <stdlib.h>

#include "defs.h"

// generates all the squares the specified pieces could move
uint64_t pseudo_legal_squares(const board_s* board, const uint8_t side, const uint64_t pieces) {
	const uint8_t piece_type = get_piece_type(board, side, pieces);
	uint64_t squares = 0x0;
	uint64_t piece_pos = pieces;
	
	while (piece_pos) {
		uint64_t curr_piece = pop_bitboard(&piece_pos);
		if (piece_type == KING)
			squares |= pseudo_legal_squares_k(board, side, curr_piece);
		else if (piece_type == KNIGHT)
			squares |= pseudo_legal_squares_n(board, side, curr_piece);
	}

	return squares;
}

uint64_t pseudo_legal_squares_k(const board_s* board, const uint8_t side, uint64_t piece) {
	uint64_t squares = movelookup[KING][pop_bit(&piece)];
	// negate eating own pieces
	squares &= ~board->all_pieces[side]; 
	return squares;
}

uint64_t pseudo_legal_squares_n(const board_s* board, const uint8_t side, uint64_t piece) {
	uint64_t squares = movelookup[KNIGHT][pop_bit(&piece)];
	// don't eat own pieces
	squares &= ~board->all_pieces[side]; 
	return squares;
}