/*
Attack stuff.
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>

#include "movegen.h"

#include "bitboard.h"
#include "lookup.h"
#include "board.h"
#include "eval.h"
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

/*
 * Private data
 */
BitBoard (*pseudo_legal_squares[N_PIECES])(const board_s* board, const unsigned int side, const BitBoard piece) = {
	[KING] = pseudo_legal_squares_k,
	[QUEEN] = pseudo_legal_squares_q,
	[ROOK] = pseudo_legal_squares_r,
	[BISHOP] = pseudo_legal_squares_b,
	[KNIGHT] = pseudo_legal_squares_n,
	[PAWN] = pseudo_legal_squares_p
};



bool is_side_attacking_sq(const board_s* board, const BitBoard sq, const unsigned int side) {
	const unsigned int pos = lowest_bitindex(sq);

	// first check knights, as they are cheap to check (only array accesses)
	if (board->pieces[side][KNIGHT] & piecelookup(pos, KNIGHT, 0))
		return true;
	
	// then check pawns, as they are also cheap
	const unsigned int opposite_side = OPPOSITE_SIDE(side);
	if (board->pieces[side][PAWN] & piecelookup(pos, PAWN, opposite_side))
		return true;
	
	// Kings are also cheap to check
	if (board->pieces[side][KING] & piecelookup(pos, KING, 0))
		return true;

	// Check bishops and diagonal queen attacks
	const BitBoard bishop_mask = piecelookup(pos, BISHOP, 0);
	const BitBoard bishop_squares = Bmagic(pos, board->every_piece & bishop_mask);
	if (bishop_squares & (board->pieces[side][BISHOP] | board->pieces[side][QUEEN]))
		return true;
	
	// Check rooks and horizontal queen attacks
	const BitBoard rook_mask = piecelookup(pos, ROOK, 0);
	const BitBoard rook_squares = Rmagic(pos, board->every_piece & rook_mask);
	if (rook_squares & (board->pieces[side][ROOK] | board->pieces[side][QUEEN]))
		return true;
	
	return false;
}


BitBoard get_attackers(const board_s* board, const BitBoard sq, const unsigned int side, const BitBoard ignoremask) {
	assert(popcount(sq) == 1);

	const unsigned int pos = lowest_bitindex(sq);

	BitBoard attackers = 0x0;

	const BitBoard selectmask = ~ignoremask; // these squares are considered

	// Check knights
	attackers |= board->pieces[side][KNIGHT] & piecelookup(pos, KNIGHT, 0) & selectmask;
	
	// Check pawns
	attackers |= board->pieces[side][PAWN] & piecelookup(pos, PAWN, OPPOSITE_SIDE(side)) & selectmask;
	
	// Kings
	// NOTE: Does not check if attacked square is of a king 
	attackers |= board->pieces[side][KING] & piecelookup(pos, KING, 0) & selectmask;

	// Check bishops and diagonal queen attacks
	const BitBoard bishop_mask = piecelookup(pos, BISHOP, 0) & selectmask;
	const BitBoard bishop_squares = Bmagic(pos, board->every_piece & bishop_mask);
	attackers |= bishop_squares & (board->pieces[side][BISHOP] | board->pieces[side][QUEEN]);
	
	// Check rooks and horizontal queen attacks
	const BitBoard rook_mask = piecelookup(pos, ROOK, 0) & selectmask;
	const BitBoard rook_squares = Rmagic(pos, board->every_piece & rook_mask);
	attackers |= rook_squares & (board->pieces[side][ROOK] | board->pieces[side][QUEEN]);
	
	//return false;
	return attackers;
}


BitBoard get_seeing_pieces(const board_s* board, BitBoard sq, BitBoard ignoremask) {
	assert(popcount(sq) == 1);
	const unsigned int pos = lowest_bitindex(sq);

	BitBoard attackers = 0x0;

	const BitBoard selectmask = ~ignoremask; // these squares are considered

	// Check knights
	attackers |= (board->pieces[WHITE][KNIGHT] | board->pieces[BLACK][KNIGHT]) & piecelookup(pos, KNIGHT, 0) & selectmask;
	
	// Check pawns
	attackers |= board->pieces[WHITE][PAWN] & piecelookup(pos, PAWN, BLACK) & selectmask;
	attackers |= board->pieces[BLACK][PAWN] & piecelookup(pos, PAWN, WHITE) & selectmask;
	
	// Kings
	// NOTE: Does not check if attacked square is of a king 
	attackers |= (board->pieces[WHITE][KING] | board->pieces[BLACK][KING]) & piecelookup(pos, KING, 0) & selectmask;

	// Check bishops and diagonal queen attacks
	const BitBoard bishop_mask = piecelookup(pos, BISHOP, 0); // & selectmask;
	const BitBoard bishop_squares = Bmagic(pos, (board->every_piece & selectmask) & bishop_mask);
	attackers |= bishop_squares & (board->pieces[WHITE][BISHOP] | board->pieces[WHITE][QUEEN]) & selectmask;
	attackers |= bishop_squares & (board->pieces[BLACK][BISHOP] | board->pieces[BLACK][QUEEN]) & selectmask;
	
	// Check rooks and horizontal queen attacks
	const BitBoard rook_mask = piecelookup(pos, ROOK, 0); // & selectmask;
	const BitBoard rook_squares = Rmagic(pos, (board->every_piece & selectmask) & rook_mask);
	attackers |= rook_squares & (board->pieces[WHITE][ROOK] | board->pieces[WHITE][QUEEN]) & selectmask;
	attackers |= rook_squares & (board->pieces[BLACK][ROOK] | board->pieces[BLACK][QUEEN]) & selectmask;
	
	//return false;
	return attackers;
}


bool is_in_check(const board_s* board, const unsigned int side) {
	assert(side == WHITE || side == BLACK);
	assert(popcount(board->pieces[side][KING]) == 1);

	const unsigned int opposite_side = OPPOSITE_SIDE(side);
	return is_side_attacking_sq(board, lowest_bitboard(board->pieces[side][KING]), opposite_side);
}


//TODO: Check validity
bool promote_available(const board_s* board, const unsigned int side) {
	assert(side == WHITE || side == BLACK);

	BitBoard pawns_about_to_promote = board->pieces[side][PAWN] & (side == WHITE ? W_PROMOTE_FROM_MASK : B_PROMOTE_FROM_MASK);
	while (pawns_about_to_promote) {
		if (pseudo_legal_squares_p(board, side, pop_bitboard(&pawns_about_to_promote)) & TOP_MASK)
			return true;
	}
	return false;
}


void set_move_flags(move_s* move, const board_s* board) {
	// Setting capture flag
	if (move->to & board->every_piece) { // move was a capture
		// assert may cause issues when checking mobility
		// FIXME: Create separate movability calc function
		//assert(!(board->pieces[OPPOSITE_SIDE(move->side)][KING] & move->to));
		assert(move->to & board->all_pieces[OPPOSITE_SIDE(move->side)]);

		move->flags |= FLAG_CAPTURE;
	}

	// Setting castling flag
	if (move->fromtype == KING) {
		// TODO: could be anothed #define
		const BitBoard kcastle = MV_E(move->from, 2);
		const BitBoard qcastle = MV_W(move->from, 2);
		if (move->to & kcastle)
			move->flags |= FLAG_KCASTLE;
		else if (move->to & qcastle)
			move->flags |= FLAG_QCASTLE;
	}

	// Setting pawn flag
	if (move->fromtype == PAWN) {
		move->flags |= FLAG_PAWNMOVE;
		
		// Double push flag
		if (move->to == MV_N(move->from, 2)
			|| move->to == MV_S(move->from, 2)) {
				move->flags |= FLAG_DOUBLEPUSH;
		}

		if (move->to == board->en_passant) {
			move->flags |= FLAG_ENPASSANT;
		}

		// Promote
		if (move->from & (move->side==WHITE ? W_PROMOTE_FROM_MASK : B_PROMOTE_FROM_MASK))
			move->flags |= FLAG_PROMOTE;
	}

	// Setting probable-check flag
	// TODO: Fix this pos (make more checks for checks)
	if (move->fromtype != KING) {
		if (move->fromtype != PAWN) {
			if (move->to & (*pseudo_legal_squares[move->fromtype])(board, move->side, board->pieces[OPPOSITE_SIDE(move->side)][KING])) {
				move->flags |= FLAG_CHECK;
			}
		}
	}
}


void create_move(const board_s* board, move_s* move, BitBoard from, BitBoard to, unsigned int promoteto) {
	assert(popcount(from) == 1);
	assert(popcount(to) == 1);
	
	move->from = from;
	move->to = to;
	move->side = get_piece_side(board, from);
	assert(move->side == board->sidetomove); // may cause problems??
	move->fromtype = get_piece_type(board, move->side, from);

	set_move_flags(move, board);

	if (move->flags & FLAG_CAPTURE)
		move->piece_captured = get_piece_type(board, OPPOSITE_SIDE(move->side), to);
	
	if (move->flags & FLAG_PROMOTE) {
		assert(promoteto != KING);
		assert(promoteto != PAWN);
		move->promoteto = promoteto;
	}
	
	move->old_castling_flags = board->castling;
	move->old_en_passant = board->en_passant;
}


movelist_s get_pseudo_legal_squares(const board_s* board, const BitBoard piecebb, bool set_move_ordering) {
	assert(popcount(piecebb));

	const unsigned int side = get_piece_side(board, piecebb);
	const unsigned int piece_type = get_piece_type(board, side, piecebb);

	BitBoard to = (*pseudo_legal_squares[piece_type])(board, side, piecebb);

	// now we have all of the proper "to" squares
	// now we just have to assign flags and properly encode them
	movelist_s moves;
	moves.n = popcount(to);
	
	if (moves.n == 0)
		return moves; // skip everything as there is no moves

	bool promote = false;
	// Check if piece is about to promote
	// this check works becouse a pawn can't have promotions and non-promotions intermixed
	if (piece_type == PAWN && piecebb & (side == WHITE ? W_PROMOTE_FROM_MASK : B_PROMOTE_FROM_MASK)) {
		promote = true;
		moves.n *= N_PROM_PIECES;
	}

	moves.moves = malloc(sizeof(move_s) * moves.n);
	if (!moves.moves) {
		fprintf(stderr, "malloc failed at pseudo_legal_squares()\n");
		exit(1);
	}
	const uint8_t promote_piece_codes[N_PROM_PIECES] = {QUEEN, ROOK, BISHOP, KNIGHT};
	BitBoard last_pop = 0x0;

	// TODO: move ordering would be done here and taken into account in search
	for (unsigned int i = 0; i < moves.n; i++) {
		//moves.moves[i].from = piecebb;
		//moves.moves[i].fromtype = piece_type;
		//moves.moves[i].side = side;
		moves.moves[i].flags = 0x0;

		BitBoard from = piecebb;
		BitBoard to_sq;

		unsigned int promoteto = 0;
		
		// Change the to square only every N_PROM_PIECES
		if (promote) {
			if ((i % N_PROM_PIECES) == 0) {
				last_pop = pop_bitboard(&to);
			}
			assert(last_pop);
			//moves.moves[i].to = last_pop;
			to_sq = last_pop;
			//moves.moves[i].flags |= FLAG_PROMOTE;
			//moves.moves[i].promoteto = promote_piece_codes[i % N_PROM_PIECES];
			promoteto = promote_piece_codes[i % N_PROM_PIECES];
		}
		else
			to_sq = pop_bitboard(&to); //moves.moves[i].to = pop_bitboard(&to);
		
		create_move(board, moves.moves + i, from, to_sq, promoteto);

		// set flags
		//set_move_flags(moves.moves + i, board);

		// if (moves.moves[i].flags & FLAG_CAPTURE)
		// 	moves.moves[i].piece_captured = get_piece_type(board, OPPOSITE_SIDE(side), moves.moves[i].to);
		
		// moves.moves[i].old_castling_flags = board->castling;
		// moves.moves[i].old_en_passant = board->en_passant;
		
		//moves.moves[i].move_score = get_move_predict_score(board, moves.moves + i);
		if (set_move_ordering)
			set_move_predict_scores(board, moves.moves + i);
	}
	return moves;
}


BitBoard pseudo_legal_squares_k(const board_s* board, const unsigned int side, const BitBoard piece) {
	assert(popcount(piece) == 1);
	BitBoard squares = piecelookup(lowest_bitindex(piece), KING, 0);
	// don't eat own pieces
	squares &= ~board->all_pieces[side];
	// Castling (represented by moving 2 squares)
	if (side == WHITE) {
		if ((board->castling & WQCASTLE) && !(board->every_piece & WQ_CAST_CLEAR_MASK) && board->pieces[WHITE][ROOK] & A1)
			squares |= MV_W(piece, 2);
		if (board->castling & WKCASTLE && !(board->every_piece & WK_CAST_CLEAR_MASK) && board->pieces[WHITE][ROOK] & H1)
			squares |= MV_E(piece, 2);
	}
	else {
		if (board->castling & BQCASTLE && !(board->every_piece & BQ_CAST_CLEAR_MASK) && board->pieces[BLACK][ROOK] & A8)
			squares |= MV_W(piece, 2);
		if (board->castling & BKCASTLE && !(board->every_piece & BK_CAST_CLEAR_MASK) && board->pieces[BLACK][ROOK] & H8)
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
	const BitBoard squares = Rmagic(piece_index, piecelookup(piece_index, ROOK, 0) & board->every_piece);
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
	const unsigned int opposite_side = OPPOSITE_SIDE(side);

	BitBoard first_forward;
	BitBoard second_forward;

	if (piece & TOP_MASK)
		return squares;

	BitBoard attacks = piecelookup(lowest_bitindex(piece), PAWN, side);

	if (side == WHITE) {
		first_forward = MV_N(piece, 1);
		// this takes care of checking if double-push is even allowed
		if (piece & BOTTOM_DPUSH_MASK)
			second_forward = MV_N(piece, 2);
		else
			second_forward = first_forward;
	}
	else {
		first_forward = MV_S(piece, 1);
		// this takes care of checking if double-push is even allowed
		if (piece & TOP_DPUSH_MASK)
			second_forward = MV_S(piece, 2);
		else
			second_forward = first_forward;
	}

	// Captures (including en passant)
	squares |= attacks & (board->all_pieces[opposite_side] | board->en_passant);

	// First forward
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
