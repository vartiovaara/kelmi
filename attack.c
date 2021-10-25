/*
Attack stuff.
*/

#include <stdio.h>
#include <stdlib.h>

#include "defs.h"

// generates all the squares the specified pieces could move
// returns a movelist_s. one move has from and to
// "to" has all of the pseudo-legal moves the "from" piece
// could move. REMEMBER TO free({movelist_t}.moves)
movelist_s pseudo_legal_squares(const board_s* board, const unsigned int side, const unsigned int piece_type) {
	const uint64_t pieces = board->pieces[side][piece_type];
	const unsigned int count = popcount(pieces);
	// make and allocate movelist
	movelist_s moves;
	moves.n = count;
	moves.moves = (move_s*)malloc(count * sizeof(move_s));

	uint64_t piece_copy = pieces;
	
	for (unsigned int i = 0; i < count; i++) {
		const uint64_t curr_piece = pop_bitboard(&piece_copy);
		move_s move;
		move.from = curr_piece;
		if (piece_type == KING)
			move.to = pseudo_legal_squares_k(board, side, curr_piece);
		else if (piece_type == KNIGHT)
			move.to = pseudo_legal_squares_n(board, side, curr_piece);
		else if (piece_type == QUEEN)
			move.to = pseudo_legal_squares_q(board, side, curr_piece);
		else if (piece_type == BISHOP)
			move.to = pseudo_legal_squares_b(board, side, curr_piece);
		else if (piece_type == ROOK)
			move.to = pseudo_legal_squares_r(board, side, curr_piece);
		else if (piece_type == PAWN)
			move.to = pseudo_legal_squares_p(board, side, curr_piece);
		moves.moves[i] = move; 
	}

	return moves;
}

uint64_t pseudo_legal_squares_k(const board_s* board, const unsigned int side, uint64_t piece) {
	uint64_t squares = movelookup[KING][pop_bit(&piece)];
	// don't eat own pieces
	squares &= ~board->all_pieces[side]; 
	return squares;
}

uint64_t pseudo_legal_squares_n(const board_s* board, const unsigned int side, uint64_t piece) {
	uint64_t squares = movelookup[KNIGHT][pop_bit(&piece)];
	// don't eat own pieces
	squares &= ~board->all_pieces[side]; 
	return squares;
}

uint64_t pseudo_legal_squares_q(const board_s* board, const unsigned int side, const uint64_t piece) {
	uint64_t squares = 0x0;
	uint64_t pos;
	const unsigned int opposite_side = ((side == WHITE) ? BLACK : WHITE);

	// north
	pos = piece;
	while (!(pos & TOP_MASK) && !(pos & board->all_pieces[opposite_side])) {
		pos = MV_N(pos);
		// don't go on own pieces
		if (pos & board->all_pieces[side])
			break;
		squares |= pos;
	}
	
	// east
	pos = piece;
	while (!(pos & RIGHT_MASK) && !(pos & board->all_pieces[opposite_side])) {
		pos = MV_E(pos);
		// don't go on own pieces
		if (pos & board->all_pieces[side])
			break;
		squares |= pos;
	}

	// south
	pos = piece;
	while (!(pos & BOTTOM_MASK) && !(pos & board->all_pieces[opposite_side])) {
		pos = MV_S(pos);
		// don't go on own pieces
		if (pos & board->all_pieces[side])
			break;
		squares |= pos;
	}

	// west
	pos = piece;
	while (!(pos & LEFT_MASK) && !(pos & board->all_pieces[opposite_side])) {
		pos = MV_W(pos);
		// don't go on own pieces
		if (pos & board->all_pieces[side])
			break;
		squares |= pos;
	}

	// north-east
	pos = piece;
	while (!(pos & TOP_RIGHT_MASK) && !(pos & board->all_pieces[opposite_side])) {
		pos = MV_NE(pos);
		// don't go on own pieces
		if (pos & board->all_pieces[side])
			break;
		squares |= pos;
	}

	// south-east
	pos = piece;
	while (!(pos & BOTTOM_RIGHT_MASK) && !(pos & board->all_pieces[opposite_side])) {
		pos = MV_SE(pos);
		// don't go on own pieces
		if (pos & board->all_pieces[side])
			break;
		squares |= pos;
	}

	// south-west
	pos = piece;
	while (!(pos & BOTTOM_LEFT_MASK) && !(pos & board->all_pieces[opposite_side])) {
		pos = MV_SW(pos);
		// don't go on own pieces
		if (pos & board->all_pieces[side])
			break;
		squares |= pos;
	}

	// north-west
	pos = piece;
	while (!(pos & TOP_LEFT_MASK) && !(pos & board->all_pieces[opposite_side])) {
		pos = MV_NW(pos);
		// don't go on own pieces
		if (pos & board->all_pieces[side])
			break;
		squares |= pos;
	}

	return squares;
}

uint64_t pseudo_legal_squares_b(const board_s* board, const unsigned int side, const uint64_t piece) {
	uint64_t squares = 0x0;
	uint64_t pos;
	const unsigned int opposite_side = ((side == WHITE) ? BLACK : WHITE);
	
	// north-east
	pos = piece;
	while (!(pos & TOP_RIGHT_MASK) && !(pos & board->all_pieces[opposite_side])) {
		pos = MV_NE(pos);
		// don't go on own pieces
		if (pos & board->all_pieces[side])
			break;
		squares |= pos;
	}

	// south-east
	pos = piece;
	while (!(pos & BOTTOM_RIGHT_MASK) && !(pos & board->all_pieces[opposite_side])) {
		pos = MV_SE(pos);
		// don't go on own pieces
		if (pos & board->all_pieces[side])
			break;
		squares |= pos;
	}

	// south-west
	pos = piece;
	while (!(pos & BOTTOM_LEFT_MASK) && !(pos & board->all_pieces[opposite_side])) {
		pos = MV_SW(pos);
		// don't go on own pieces
		if (pos & board->all_pieces[side])
			break;
		squares |= pos;
	}

	// north-west
	pos = piece;
	while (!(pos & TOP_LEFT_MASK) && !(pos & board->all_pieces[opposite_side])) {
		pos = MV_NW(pos);
		// don't go on own pieces
		if (pos & board->all_pieces[side])
			break;
		squares |= pos;
	}

	return squares;
}

uint64_t pseudo_legal_squares_r(const board_s* board, const unsigned int side, const uint64_t piece) {
	uint64_t squares = 0x0;
	uint64_t pos;
	const unsigned int opposite_side = ((side == WHITE) ? BLACK : WHITE);

	// north
	pos = piece;
	while (!(pos & TOP_MASK) && !(pos & board->all_pieces[opposite_side])) {
		pos = MV_N(pos);
		// don't go on own pieces
		if (pos & board->all_pieces[side])
			break;
		squares |= pos;
	}
	
	// east
	pos = piece;
	while (!(pos & RIGHT_MASK) && !(pos & board->all_pieces[opposite_side])) {
		pos = MV_E(pos);
		// don't go on own pieces
		if (pos & board->all_pieces[side])
			break;
		squares |= pos;
	}

	// south
	pos = piece;
	while (!(pos & BOTTOM_MASK) && !(pos & board->all_pieces[opposite_side])) {
		pos = MV_S(pos);
		// don't go on own pieces
		if (pos & board->all_pieces[side])
			break;
		squares |= pos;
	}

	// west
	pos = piece;
	while (!(pos & LEFT_MASK) && !(pos & board->all_pieces[opposite_side])) {
		pos = MV_W(pos);
		// don't go on own pieces
		if (pos & board->all_pieces[side])
			break;
		squares |= pos;
	}
	return squares;
}

uint64_t pseudo_legal_squares_p(const board_s* board, const unsigned int side, const uint64_t piece) {
	uint64_t squares = 0x0;
	//uint64_t pos;
	const unsigned int opposite_side = ((side == WHITE) ? BLACK : WHITE);

	uint64_t first_forward;
	uint64_t second_forward;
	uint64_t w_capture;
	uint64_t e_capture;

	if (piece & TOP_MASK)
		return squares;

	if (side == WHITE) {
		first_forward = MV_N(piece);
		// this takes care of checking if double-push is even allowed
		if (piece & BOTTOM_DPUSH_MASK)
			second_forward = MV_N(MV_N(piece));
		else
			second_forward = first_forward;
		w_capture = MV_NW(piece);
		e_capture = MV_NE(piece);
	}
	else {
		first_forward = MV_S(piece);
		// this takes care of checking if double-push is even allowed
		if (piece & TOP_DPUSH_MASK)
			second_forward = MV_S(MV_S(piece));
		else
			second_forward = first_forward;
		w_capture = MV_SW(piece);
		e_capture = MV_SE(piece);
	}

	// Captures
	if (w_capture & board->all_pieces[opposite_side]
	 || w_capture & board->en_passant) {
		squares |= w_capture;
	}
	if (e_capture & board->all_pieces[opposite_side]
	 || e_capture & board->en_passant) {
		squares |= e_capture;
	}

	// First forward
	const uint64_t bw_pieces = board->all_pieces[WHITE] | board->all_pieces[BLACK];
	if (!(first_forward & bw_pieces)) {
		squares |= first_forward;
		// Second forward
		// No need to check for out of bounds as it will be 0x0 then
		if (!(second_forward & bw_pieces)) {
			squares |= second_forward;
		}
	}

	return squares;
}
