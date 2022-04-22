/*
Attack stuff.
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>

#include "attack.h"

#include "bitboard.h"
#include "lookup.h"
#include "board.h"
#include "magicmoves/magicmoves.h"

#include "defs.h"


// Note to self: read god damn it
// https://essays.jwatzman.org/essays/chess-move-generation-with-magic-bitboards.html


/*
 * Private functions
 */
BitBoard pseudo_legal_squares_k(const board_s* board, const unsigned int side, const BitBoard piece);
BitBoard pseudo_legal_squares_n(const board_s* board, const unsigned int side, const BitBoard piece);
BitBoard pseudo_legal_squares_q(const board_s* board, const unsigned int side, const BitBoard piece);
BitBoard pseudo_legal_squares_b(const board_s* board, const unsigned int side, const BitBoard piece);
BitBoard pseudo_legal_squares_r(const board_s* board, const unsigned int side, const BitBoard piece);
BitBoard pseudo_legal_squares_p(const board_s* board, const unsigned int side, const BitBoard piece);



bool is_in_check (const board_s* board, const unsigned int side) {
	assert(side == WHITE || side == BLACK);
	assert(popcount(board->pieces[side][KING]) == 1);

	const unsigned int opposite_side = OPPOSITE_SIDE(side);
	const BitBoard king_bb = lowest_bitboard(board->pieces[side][KING]);
	const unsigned int king_pos = lowest_bitindex(king_bb);

	// first check knights, as they are cheap to check (only array accesses)
	if (board->pieces[opposite_side][KNIGHT] & piecelookup(king_pos, KNIGHT, 0))
		return true;
	
	// then check pawns, as they are also cheap
	if (board->pieces[opposite_side][PAWN] & piecelookup(king_pos, PAWN, side))
		return true;

	// check files and diagonals
	const BitBoard rb_mask = piecelookup(king_pos, QUEEN, 0);
	const BitBoard rb_magic_squares = Qmagic(king_pos, rb_mask & board->every_piece);
	
	// opposite side's rook, bishop and queens
	const BitBoard opposite_rbq = board->pieces[opposite_side][ROOK] |
	                              board->pieces[opposite_side][BISHOP] |
	                              board->pieces[opposite_side][QUEEN];
	if (rb_magic_squares & opposite_rbq)
		return true;
	
	return false;
}


movelist_s pseudo_legal_squares(const board_s* board, const BitBoard piecebb) {
	assert(popcount(piecebb));

	const unsigned int side = get_piece_side(board, piecebb);
	const unsigned int piece_type = get_piece_type(board, side, piecebb);

	BitBoard to;
	// TODO: have the corresponding function pointer in a array of function pointer
	//	thusly there are less branching
	if (piece_type == KING)
		to = pseudo_legal_squares_k(board, side, piecebb);
	else if (piece_type == KNIGHT)
		to = pseudo_legal_squares_n(board, side, piecebb);
	else if (piece_type == QUEEN)
		to = pseudo_legal_squares_q(board, side, piecebb);
	else if (piece_type == BISHOP)
		to = pseudo_legal_squares_b(board, side, piecebb);
	else if (piece_type == ROOK)
		to = pseudo_legal_squares_r(board, side, piecebb);
	else if (piece_type == PAWN)
		to = pseudo_legal_squares_p(board, side, piecebb);
	else {
		// should never get here
		fprintf(stderr, "pseudo_legal_squares(%p, %p)\n", (void*)board, (void*)piecebb);
		exit(1);
	}

	// now we have all of the proper "to" squares
	// now we just have to assign flags and properly encode them
	movelist_s moves;
	moves.n = popcount(to);
	if (moves.n == 0)
		return moves; // skip everything as there is no moves
	moves.moves = malloc(sizeof(move_s) * moves.n);
	if (!moves.moves) {
		fprintf(stderr, "malloc failed at pseeudo_legal_squares()\n");
		exit(1);
	}
	// TODO: move ordering would be done here and taken into account in search
	for (unsigned int i = 0; i < moves.n; i++) {
		moves.moves[i].from = piecebb;
		moves.moves[i].to = pop_bitboard(&to);
		moves.moves[i].fromtype = piece_type;
		moves.moves[i].side = side;
		moves.moves[i].flags = 0;
		
		// Setting capture flag
		// TODO: set piece_captured
		if (moves.moves[i].to & board->every_piece) { // move was a capture
			assert(!(board->pieces[OPPOSITE_SIDE(side)][KING] & moves.moves[i].to));
			assert(moves.moves[i].to & board->all_pieces[OPPOSITE_SIDE(side)]);

			moves.moves[i].flags |= FLAG_CAPTURE;
		}

		// Setting castling flag
		if (piece_type == KING) {
			// TODO: could be anothed #define
			const BitBoard kcastle = MV_E(moves.moves[i].from, 2);
			const BitBoard qcastle = MV_W(moves.moves[i].from, 2);
			if (moves.moves[i].to & kcastle)
				moves.moves[i].flags |= FLAG_KCASTLE;
			else if (moves.moves[i].to & qcastle)
				moves.moves[i].flags |= FLAG_QCASTLE;
		}

		// Setting pawn flag and double push flag
		if (piece_type == PAWN) {
			moves.moves[i].flags |= FLAG_PAWNMOVE;
			
			if (moves.moves[i].to == MV_N(moves.moves[i].from, 2)
			 || moves.moves[i].to == MV_S(moves.moves[i].from, 2)) {
				 moves.moves[i].flags |= FLAG_DOUBLEPUSH;
			}
		}
	}
	return moves;
}


BitBoard pseudo_legal_squares_k(const board_s* board, const unsigned int side, const BitBoard piece) {
	BitBoard squares = piecelookup(lowest_bitindex(piece), KING, 0);
	// don't eat own pieces
	squares &= ~board->all_pieces[side];
	// Castling (represented by moving 2 squares)
	if (side == WHITE) {
		if (board->castling & WQCASTLE && !(board->every_piece & WQ_CAST_CLEAR_MASK))
			squares |= MV_W(piece, 2);
		if (board->castling & WKCASTLE && !(board->every_piece & WK_CAST_CLEAR_MASK))
			squares |= MV_E(piece, 2);
	}
	else {
		if (board->castling & BQCASTLE && !(board->every_piece & BQ_CAST_CLEAR_MASK))
			squares |= MV_W(piece, 2);
		if (board->castling & BKCASTLE && !(board->every_piece & BK_CAST_CLEAR_MASK))
			squares |= MV_E(piece, 2);
	}
	return squares;
}


BitBoard pseudo_legal_squares_n(const board_s* board, const unsigned int side, const BitBoard piece) {
	BitBoard squares = piecelookup(lowest_bitindex(piece), KNIGHT, 0);
	// don't eat own pieces
	squares &= ~board->all_pieces[side]; 
	return squares;
}


BitBoard pseudo_legal_squares_q(const board_s* board, const unsigned int side, const BitBoard piece) {
	const unsigned int piece_index = lowest_bitindex(piece);
	const BitBoard squares = Qmagic(piece_index, piecelookup(piece_index, QUEEN, 0) & board->every_piece);
	return squares & ~board->all_pieces[side]; // don't go on own pieces

	/*
	BitBoard squares = 0x0;
	BitBoard pos;
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
	*/
}


BitBoard pseudo_legal_squares_b(const board_s* board, const unsigned int side, const BitBoard piece) {
	const unsigned int piece_index = lowest_bitindex(piece);
	const BitBoard squares = Bmagic(piece_index, piecelookup(piece_index, BISHOP, 0) & board->every_piece);
	return squares & ~board->all_pieces[side]; // don't go on own pieces

	/*
	BitBoard squares = 0x0;
	BitBoard pos;
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
	*/
}


BitBoard pseudo_legal_squares_r(const board_s* board, const unsigned int side, const BitBoard piece) {
	const unsigned int piece_index = lowest_bitindex(piece);
	const BitBoard squares = Rmagic(piece_index, piecelookup(piece_index, ROOK, 0) & (board->all_pieces[WHITE] | board->all_pieces[BLACK]));
	return squares & ~board->all_pieces[side]; // don't go on own pieces
	
	/*
	BitBoard squares = 0x0;
	BitBoard pos;
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
	*/
}


BitBoard pseudo_legal_squares_p(const board_s* board, const unsigned int side, const BitBoard piece) {
	BitBoard squares = 0x0;
	//BitBoard pos;
	const unsigned int opposite_side = OPPOSITE_SIDE(side);

	BitBoard first_forward;
	BitBoard second_forward;
	BitBoard w_capture;
	BitBoard e_capture;

	if (piece & TOP_MASK)
		return squares;

	if (side == WHITE) {
		first_forward = MV_N(piece, 1);
		// this takes care of checking if double-push is even allowed
		if (piece & BOTTOM_DPUSH_MASK)
			second_forward = MV_N(piece, 2);
		else
			second_forward = first_forward;
		w_capture = MV_NW(piece, 1);
		e_capture = MV_NE(piece, 1);
	}
	else {
		first_forward = MV_S(piece, 1);
		// this takes care of checking if double-push is even allowed
		if (piece & TOP_DPUSH_MASK)
			second_forward = MV_S(piece, 2);
		else
			second_forward = first_forward;
		w_capture = MV_SW(piece, 1);
		e_capture = MV_SE(piece, 1);
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
	//const BitBoard bw_pieces = board->all_pieces[WHITE] | board->all_pieces[BLACK];
	if (!(first_forward & board->every_piece)) {
		squares |= first_forward;
		// Second forward
		// No need to check for out of bounds as it will be 0x0 then
		if (!(second_forward & board->every_piece)) {
			squares |= second_forward;
		}
	}

	return squares;
}
