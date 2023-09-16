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

#include "defs.h"


// Note to self: read god damn it
// https://essays.jwatzman.org/essays/chess-move-generation-with-magic-bitboards.html


/*
 * Private functions
 */
static inline BitBoard get_pseudo_legal_squares(const board_s* restrict board, unsigned int side, unsigned int piece_type, BitBoard piecebb);
static inline BitBoard pseudo_legal_squares_k(BitBoard w_occupancy, BitBoard b_occupancy, const BitBoard piece);
static inline BitBoard pseudo_legal_squares_n(BitBoard w_occupancy, BitBoard b_occupancy, const BitBoard piece);
static inline BitBoard pseudo_legal_squares_q(BitBoard w_occupancy, BitBoard b_occupancy, const BitBoard piece);
static inline BitBoard pseudo_legal_squares_b(BitBoard w_occupancy, BitBoard b_occupancy, const BitBoard piece);
static inline BitBoard pseudo_legal_squares_r(BitBoard w_occupancy, BitBoard b_occupancy, const BitBoard piece);
static inline BitBoard pseudo_legal_squares_p(BitBoard w_occupancy, BitBoard b_occupancy, const BitBoard piece);

static inline BitBoard p_attacks_n(BitBoard pawns);
static inline BitBoard p_attacks_s(BitBoard pawns);

static inline BitBoard GenRook(unsigned int sq, BitBoard occupation);
static inline BitBoard GenBishop(unsigned int sq, BitBoard occupation);

static inline BitBoard south_attacks  (BitBoard pieces, BitBoard empty);
static inline BitBoard north_attacks  (BitBoard pieces, BitBoard empty);
static inline BitBoard east_attacks   (BitBoard pieces, BitBoard empty);
static inline BitBoard noeast_attacks (BitBoard pieces, BitBoard empty);
static inline BitBoard soeast_attacks (BitBoard pieces, BitBoard empty);
static inline BitBoard west_attacks   (BitBoard pieces, BitBoard empty);
static inline BitBoard sowest_attacks (BitBoard pieces, BitBoard empty);
static inline BitBoard nowest_attacks (BitBoard pieces, BitBoard empty);

static inline BitBoard soutOccl(BitBoard gen, BitBoard pro);
static inline BitBoard nortOccl(BitBoard gen, BitBoard pro);
static inline BitBoard eastOccl(BitBoard gen, BitBoard pro);
static inline BitBoard noEaOccl(BitBoard gen, BitBoard pro);
static inline BitBoard soEaOccl(BitBoard gen, BitBoard pro);
static inline BitBoard westOccl(BitBoard gen, BitBoard pro);
static inline BitBoard soWeOccl(BitBoard gen, BitBoard pro);
static inline BitBoard noWeOccl(BitBoard gen, BitBoard pro);




/*
 * Private data
 */
// static BitBoard (*pseudo_legal_squares[])(BitBoard w_occupancy, BitBoard b_occupancy, const BitBoard piece) = {
// 	[KING] = pseudo_legal_squares_k,
// 	[QUEEN] = pseudo_legal_squares_q,
// 	[ROOK] = pseudo_legal_squares_r,
// 	[BISHOP] = pseudo_legal_squares_b,
// 	[KNIGHT] = pseudo_legal_squares_n,
// 	[PAWN] = pseudo_legal_squares_p
// };



bool is_attacking_squares(const board_s* board, const BitBoard sq, const BitBoard attack_mask) {
	//const unsigned int pos = lowest_bitindex(sq);

	// first check knights, as they are cheap to check (only array accesses)
	if (KNIGHTS(board) & n_attacks(sq) & attack_mask)
		return true;
	
	// then check pawns, as they are also cheap
	//const unsigned int opposite_side = OPPOSITE_SIDE(side);
	// if (PAWNS(board) & ((p_attacks(sq, BLACK) & board->pm) | (p_attacks(sq, WHITE) & ~board->pm)) & attack_mask)
	if (PAWNS(board) & ((p_attacks_s(sq) & board->pm) | (p_attacks_n(sq) & ~board->pm)) & attack_mask)
		return true;
	
	// Kings are also cheap to check
	if (KINGS(board) & k_attacks(sq) & attack_mask)
		return true;

	// Check bishops and diagonal queen attacks
	// const BitBoard bishop_squares = b_attacks(sq, ~OCCUPANCY(board));
	// if (bishop_squares & (BISHOPS(board) | QUEENS(board)) & attack_mask)
	// 	return true;
	const BitBoard bishop_attacks = b_attacks((BISHOPS(board) | QUEENS(board)) & attack_mask, ~OCCUPANCY(board));
	if (sq & bishop_attacks)
		return true;
	
	// Check rooks and horizontal queen attacks
	// const BitBoard rook_squares = r_attacks(sq, ~OCCUPANCY(board));
	// if (rook_squares & (ROOKS(board) | QUEENS(board)) & attack_mask)
	// 	return true;
	const BitBoard rook_attacks = r_attacks((ROOKS(board) | QUEENS(board)) & attack_mask, ~OCCUPANCY(board));
	if (sq & rook_attacks)
		return true;
	
	return false;
}


bool is_attacking_square(const board_s* board, const BitBoard sq, const BitBoard attack_mask) {

	const unsigned int pos = LOWEST_BITINDEX(sq);

	// const unsigned int opposite_side = OPPOSITE_SIDE(side);

	// first check knights, as they are cheap to check (only array accesses)
	if (KNIGHTS(board) & n_attacks(sq) & attack_mask)
		return true;
	
	// then check pawns, as they are also cheap
	// if (PAWNS(board) & p_attacks(sq, WHITE) & p_attacks(sq, BLACK) & attack_mask)
	if (PAWNS(board) & p_attacks_n(sq) & p_attacks_s(sq) & attack_mask)
		return true;
	
	// Kings are also cheap to check
	if (KINGS(board) & k_attacks(sq) & attack_mask)
		return true;

	// Check bishops and diagonal queen attacks
	const BitBoard bishop_squares = GenBishop(pos, OCCUPANCY(board));
	// const BitBoard bishop_squares = b_attacks(sq, ~OCCUPANCY(board));
	if (bishop_squares & (BISHOPS(board) | QUEENS(board)) & attack_mask)
		return true;
	
	// Check rooks and horizontal queen attacks
	const BitBoard rook_squares = GenRook(pos, OCCUPANCY(board));
	// const BitBoard rook_squares = r_attacks(sq, ~OCCUPANCY(board));
	if (rook_squares & (ROOKS(board) | QUEENS(board)) & attack_mask)
		return true;
	
	return false;
}

BitBoard opponent_attacks_squares(const board_s* restrict board, const BitBoard sq) {
	const BitBoard attacks = (n_attacks(sq) & KNIGHTS(board))
		// | (p_attacks(sq, WHITE) & PAWNS(board))
		| (p_attacks_n(sq) & PAWNS(board))
		| (k_attacks(sq) & KINGS(board))
		| (b_attacks(sq, ~OCCUPANCY(board)) & (BISHOPS(board) | QUEENS(board)))
		| (r_attacks(sq, ~OCCUPANCY(board)) & (ROOKS(board) | QUEENS(board)));
	return attacks & ~board->pm;
}

BitBoard own_attacks_squares(const board_s* restrict board, const BitBoard sq) {
	const BitBoard attacks = (n_attacks(sq) & KNIGHTS(board))
		// | (p_attacks(sq, BLACK) & PAWNS(board))
		| (p_attacks_s(sq) & PAWNS(board))
		| (k_attacks(sq) & KINGS(board))
		| (b_attacks(sq, ~OCCUPANCY(board)) & (BISHOPS(board) | QUEENS(board)))
		| (r_attacks(sq, ~OCCUPANCY(board)) & (ROOKS(board) | QUEENS(board)));
	return attacks & board->pm;
}

/*

BitBoard get_attackers(const board_s* board, const BitBoard sq, const unsigned int side, const BitBoard ignoremask) {
	assert(popcount(sq) == 1);

	const unsigned int pos = lowest_bitindex(sq);

	BitBoard attackers = 0x0;

	const BitBoard selectmask = ~ignoremask; // these squares are considered

	// Check knights
	attackers |= board->pieces[side][KNIGHT] & n_attacks(sq) & selectmask;
	
	// Check pawns
	attackers |= board->pieces[side][PAWN] & p_attacks(sq, OPPOSITE_SIDE(side)) & selectmask;
	
	// Kings
	// NOTE: Does not check if attacked square is of a king 
	attackers |= board->pieces[side][KING] & k_attacks(sq) & selectmask;

	// Check bishops and diagonal queen attacks
	const BitBoard bishop_squares = b_attacks(sq, ~board->every_piece | ignoremask);
	attackers |= bishop_squares & (board->pieces[side][BISHOP] | board->pieces[side][QUEEN]);
	
	// Check rooks and horizontal queen attacks
	const BitBoard rook_squares = r_attacks(sq, ~board->every_piece | ignoremask);
	attackers |= rook_squares & (board->pieces[side][ROOK] | board->pieces[side][QUEEN]);
	
	//return false;
	return attackers;
}

*/


BitBoard get_seeing_pieces(const board_s* board, BitBoard sq, BitBoard ignoremask) {
	assert(POPCOUNT(sq) == 1);
	const unsigned int pos = LOWEST_BITINDEX(sq);

	BitBoard attackers = 0x0;

	const BitBoard selectmask = ~ignoremask; // these squares are considered

	// Check knights
	attackers |= KNIGHTS(board) & n_attacks(sq) & selectmask;
	
	// Check pawns
	attackers |= (PAWNS(board) & board->pm) & p_attacks_s(sq) & selectmask;
	attackers |= (PAWNS(board) & ~board->pm) & p_attacks_n(sq) & selectmask;
	
	// Kings
	// NOTE: Does not check if attacked square is of a king 
	// attackers |= (board->pieces[WHITE][KING] | board->pieces[BLACK][KING]) & k_attacks(sq) & selectmask;
	attackers |= (KINGS(board)) & k_attacks(sq) & selectmask;

	// Check bishops and diagonal queen attacks
	
	attackers |= GenBishop(pos, OCCUPANCY(board) & selectmask) & (BISHOPS(board) | QUEENS(board)) & selectmask;
	attackers |= GenRook(pos, OCCUPANCY(board) & selectmask) & (ROOKS(board) | QUEENS(board)) & selectmask;


	/*
	const BitBoard bishop_squares = b_attacks(sq, ~board->every_piece | ignoremask);
	attackers |= bishop_squares & (board->pieces[WHITE][BISHOP] | board->pieces[WHITE][QUEEN]) & selectmask;
	attackers |= bishop_squares & (board->pieces[BLACK][BISHOP] | board->pieces[BLACK][QUEEN]) & selectmask;
	
	
	// Check rooks and horizontal queen attacks
	const BitBoard rook_squares = r_attacks(sq, ~board->every_piece | ignoremask);
	attackers |= rook_squares & (board->pieces[WHITE][ROOK] | board->pieces[WHITE][QUEEN]) & selectmask;
	attackers |= rook_squares & (board->pieces[BLACK][ROOK] | board->pieces[BLACK][QUEEN]) & selectmask;
	*/

	return attackers;
}


bool is_in_check(const board_s* board, const unsigned int side) {
	assert(side == WHITE || side == BLACK);
	//assert(popcount(board->pieces[side][KING]) == 1);

	//const unsigned int opposite_side = OPPOSITE_SIDE(side);
	return is_attacking_squares(board, KINGS(board) & board->pm, ~board->pm);
	// return is_attacking_square(board, KINGS(board) & board->pm, ~board->pm);
}

BitBoard in_check(const board_s* restrict board) {
	const BitBoard kingsq = KINGS(board) & board->pm;
	const BitBoard kingindex = LOWEST_BITINDEX(kingsq);
	// const BitBoard attacks = (n_attacks(kingsq) & KNIGHTS(board))
	const BitBoard attacks = (knightlookup[kingindex] & KNIGHTS(board))
		// | (p_attacks(kingsq, WHITE) & PAWNS(board))
		| (p_attacks_n(kingsq) & PAWNS(board))
		| (k_attacks(kingsq) & KINGS(board))
		| (GenBishop(kingindex, OCCUPANCY(board)) & (BISHOPS(board) | QUEENS(board)))
		| (GenRook(kingindex, OCCUPANCY(board)) & (ROOKS(board) | QUEENS(board)));
	return attacks & ~board->pm;
}


BitBoard opponent_in_check(const board_s* restrict board) {
	const BitBoard kingsq = KINGS(board) & ~board->pm;
	const BitBoard kingindex = LOWEST_BITINDEX(kingsq);
	// const BitBoard attacks = (n_attacks(kingsq) & KNIGHTS(board))
	const BitBoard attacks = (knightlookup[kingindex] & KNIGHTS(board))
		// | (p_attacks(kingsq, BLACK) & PAWNS(board))
		| (p_attacks_s(kingsq) & PAWNS(board))
		| (k_attacks(kingsq) & KINGS(board))
		| (GenBishop(kingindex, OCCUPANCY(board)) & (BISHOPS(board) | QUEENS(board)))
		| (GenRook(kingindex, OCCUPANCY(board)) & (ROOKS(board) | QUEENS(board)));
	return attacks & board->pm;
}

// Generates captures on attacker and attack blocks
BitBoard check_blocks(const board_s* restrict board, const BitBoard attacker) {
	
	assert(POPCOUNT(attacker) <= 1);

	const unsigned int att_sq = LOWEST_BITINDEX(attacker);
	const unsigned int att_type = PIECE_SQ(board, att_sq);

	// Knight attacks can't be blocked
	if (att_type == KNIGHT || att_type == PAWN) return attacker;

	const unsigned int king_sq = LOWEST_BITINDEX(KINGS(board) & board->pm);
	return ray_attacks[king_sq][att_sq];
}

BitBoard pinned_pieces(const board_s* restrict board) {

	const BitBoard kingbb = KINGS(board) & board->pm;
	const BitBoard opponent_pieces = OCCUPANCY(board) & ~board->pm;
	// const BitBoard my_pieces = OCCUPANCY(board) & board->pm;
	const BitBoard my_pieces = board->pm;


	BitBoard pinned = 0x0;
	BitBoard attacks;

	attacks = north_attacks(kingbb, ~opponent_pieces);
	pinned |= (attacks & my_pieces) * (POPCOUNT(attacks & my_pieces) == 1 && (attacks & opponent_pieces & (ROOKS(board) | QUEENS(board))));
	attacks = east_attacks (kingbb, ~opponent_pieces);
	pinned |= (attacks & my_pieces) * (POPCOUNT(attacks & my_pieces) == 1 && (attacks & opponent_pieces & (ROOKS(board) | QUEENS(board))));
	attacks = south_attacks(kingbb, ~opponent_pieces);
	pinned |= (attacks & my_pieces) * (POPCOUNT(attacks & my_pieces) == 1 && (attacks & opponent_pieces & (ROOKS(board) | QUEENS(board))));
	attacks = west_attacks (kingbb, ~opponent_pieces);
	pinned |= (attacks & my_pieces) * (POPCOUNT(attacks & my_pieces) == 1 && (attacks & opponent_pieces & (ROOKS(board) | QUEENS(board))));

	attacks = noeast_attacks(kingbb, ~opponent_pieces);
	pinned |= (attacks & my_pieces) * (POPCOUNT(attacks & my_pieces) == 1 && (attacks & opponent_pieces & (BISHOPS(board) | QUEENS(board))));
	attacks = soeast_attacks(kingbb, ~opponent_pieces);
	pinned |= (attacks & my_pieces) * (POPCOUNT(attacks & my_pieces) == 1 && (attacks & opponent_pieces & (BISHOPS(board) | QUEENS(board))));
	attacks = sowest_attacks(kingbb, ~opponent_pieces);
	pinned |= (attacks & my_pieces) * (POPCOUNT(attacks & my_pieces) == 1 && (attacks & opponent_pieces & (BISHOPS(board) | QUEENS(board))));
	attacks = nowest_attacks(kingbb, ~opponent_pieces);
	pinned |= (attacks & my_pieces) * (POPCOUNT(attacks & my_pieces) == 1 && (attacks & opponent_pieces & (BISHOPS(board) | QUEENS(board))));


/*
	BitBoard pin_ray;

	attacks = sowest_attacks(kingbb, ~opponent_pieces);
	pin_ray = (attacks) * (POPCOUNT(attacks & my_pieces) == 1 && (attacks & opponent_pieces & (BISHOPS(board) | QUEENS(board))));
	if (pin_ray & my_pieces) *(pin_rays++) = pin_ray;
	pinned |= pin_ray & my_pieces;

	attacks = south_attacks(kingbb, ~opponent_pieces);
	pin_ray = (attacks) * (POPCOUNT(attacks & my_pieces) == 1 && (attacks & opponent_pieces & (ROOKS(board) | QUEENS(board))));
	if (pin_ray & my_pieces) *(pin_rays++) = pin_ray;
	pinned |= pin_ray & my_pieces;

	attacks = soeast_attacks(kingbb, ~opponent_pieces);
	pin_ray = (attacks) * (POPCOUNT(attacks & my_pieces) == 1 && (attacks & opponent_pieces & (BISHOPS(board) | QUEENS(board))));
	if (pin_ray & my_pieces) *(pin_rays++) = pin_ray;
	pinned |= pin_ray & my_pieces;

	attacks = west_attacks (kingbb, ~opponent_pieces);
	pin_ray = (attacks) * (POPCOUNT(attacks & my_pieces) == 1 && (attacks & opponent_pieces & (ROOKS(board) | QUEENS(board))));
	if (pin_ray & my_pieces) *(pin_rays++) = pin_ray;
	pinned |= pin_ray & my_pieces;

	attacks = east_attacks (kingbb, ~opponent_pieces);
	pin_ray = (attacks) * (POPCOUNT(attacks & my_pieces) == 1 && (attacks & opponent_pieces & (ROOKS(board) | QUEENS(board))));
	if (pin_ray & my_pieces) *(pin_rays++) = pin_ray;
	pinned |= pin_ray & my_pieces;

	attacks = nowest_attacks(kingbb, ~opponent_pieces);
	pin_ray = (attacks) * (POPCOUNT(attacks & my_pieces) == 1 && (attacks & opponent_pieces & (BISHOPS(board) | QUEENS(board))));
	if (pin_ray & my_pieces) *(pin_rays++) = pin_ray;
	pinned |= pin_ray & my_pieces;

	attacks = north_attacks(kingbb, ~opponent_pieces);
	// pinned |= (attacks & my_pieces) * (POPCOUNT(attacks & my_pieces) == 1 && (attacks & opponent_pieces & (ROOKS(board) | QUEENS(board))));
	pin_ray = (attacks) * (POPCOUNT(attacks & my_pieces) == 1 && (attacks & opponent_pieces & (ROOKS(board) | QUEENS(board))));
	if (pin_ray & my_pieces) *(pin_rays++) = pin_ray;
	pinned |= pin_ray & my_pieces;

	attacks = noeast_attacks(kingbb, ~opponent_pieces);
	pin_ray = (attacks) * (POPCOUNT(attacks & my_pieces) == 1 && (attacks & opponent_pieces & (BISHOPS(board) | QUEENS(board))));
	if (pin_ray & my_pieces) *(pin_rays++) = pin_ray;
	pinned |= pin_ray & my_pieces;

*/	

	return pinned;
}

/*
//TODO: Check validity
bool promote_available(const board_s* board, const unsigned int side) {
	assert(side == WHITE || side == BLACK);

	BitBoard pawns_about_to_promote = board->pieces[side][PAWN] & (side == WHITE ? W_PROMOTE_FROM_MASK : B_PROMOTE_FROM_MASK);
	while (pawns_about_to_promote) {
		if (pseudo_legal_squares_p(board->all_pieces[WHITE], board->all_pieces[BLACK], side, pop_bitboard(&pawns_about_to_promote), true) & TOP_MASK)
			return true;
	}
	return false;
}
*/

BitBoard is_move_illegal(const board_s* restrict board, move_s move) {

	const BitBoard frombb = SQTOBB(move.from);
	const BitBoard tobb = SQTOBB(move.to);

	BitBoard occ = OCCUPANCY(board);
	BitBoard opposing = (board->pm ^ occ) & ~tobb;
	occ ^= frombb;
	occ |= tobb;

	BitBoard kingbb = KINGS(board) & board->pm;

	if (frombb == kingbb) {
		kingbb = tobb;
	}
	else if (move.flags & FLAG_ENPASSANT) {
		opposing ^= MV_S(tobb, 1);
		occ ^= MV_S(tobb, 1);
	}

	const unsigned int kingindex = LOWEST_BITINDEX(kingbb);


	const BitBoard attacks = (knightlookup[kingindex] & KNIGHTS(board))
		// | (p_attacks(kingbb, WHITE) & PAWNS(board))
		| (p_attacks_n(kingbb) & PAWNS(board))
		| (k_attacks(kingbb) & KINGS(board))
		| (GenBishop(kingindex, occ) & (BISHOPS(board) | QUEENS(board)))
		| (GenRook(kingindex, occ) & (ROOKS(board) | QUEENS(board)));
	return attacks & opposing;

}


move_s set_move_flags(move_s move, const board_s* restrict board) {

	assert(move.from < 64);
	assert(move.to < 64);

	move.flags = 0x0;

	const unsigned int fromtype = PIECE_SQ(board, move.from);
	const BitBoard frombb = SQTOBB(move.from);
	const BitBoard tobb = SQTOBB(move.to);

	// Setting capture flag
	if (tobb & OCCUPANCY(board)) { // move was a capture

		move.flags |= FLAG_CAPTURE;
	}
	// move.flags |= FLAG_CAPTURE * ((tobb & OCCUPANCY(board)) > 0);


	// Setting castling flag
	if (fromtype == KING && frombb & M_KING_DEFAULT_POS) {
		
		// FIXME: EDGE CASES
		if (tobb & K_CASTLE_KING_TARGET)
			move.flags |= FLAG_KCASTLE;
		if (tobb & Q_CASTLE_KING_TARGET)
			move.flags |= FLAG_QCASTLE;
		//if (SQTOBB(move->to) & K_CASTLE_KING_TARGET)
		// move.flags |= FLAG_KCASTLE * (move.to == 6);
		//else if (SQTOBB(move->to) & Q_CASTLE_KING_TARGET)
		// move.flags |= FLAG_QCASTLE * (move.to == 2);

		return move;
	}
	else if (fromtype == PAWN) {
		
		// Setting pawn flags

		move.flags |= FLAG_PAWNMOVE;
		
		// Double push flag
		if (move.from == move.to - 16) {
			move.flags |= FLAG_DOUBLEPUSH;
		}
		if (move.to % 8 == board->en_passant
		   && ((move.to - move.from) % 8) != 0
		   && tobb & EN_PASSANT_TARGET_ROW) {
			move.flags |= FLAG_ENPASSANT;
		}
		if (move.to >= 56) // goes on the top rank
			move.flags |= FLAG_PROMOTE;
		
		// move.flags |= FLAG_DOUBLEPUSH * (move.from == move.to - 16);
		// move.flags |= FLAG_PROMOTE * (move.to >= 56);
		// move.flags |= FLAG_ENPASSANT * (move.to%8 == board->en_passant && ((move.to-move.from)%8) != 0 && tobb & EN_PASSANT_TARGET_ROW);

		// move.flags |= FLAG_DOUBLEPUSH * (move.from == move.to - 16);
		// move.flags |= FLAG_PROMOTE * (move.to >= 56);
		// move.flags |= FLAG_ENPASSANT * (move.to%8 == board->en_passant && ((move.to-move.from)%8) != 0 && tobb & EN_PASSANT_TARGET_ROW);
	}
	return move;
}

/*
void create_move(const board_s* board, move_s* move, BitBoard from, BitBoard to, unsigned int promoteto) {
	assert(popcount(from) == 1);
	assert(popcount(to) == 1);
	
	move->from = from;
	move->to = to;
	move->side = get_piece_side(board, from);
	assert(move->side == board->sidetomove); // may cause problems??
	move->fromtype = get_piece_type(board, move->side, from);

	set_move_flags(move, board);

	if (move->flags & FLAG_CAPTURE) {
		move->piece_captured = get_piece_type(board, OPPOSITE_SIDE(move->side), to);
		assert(move->piece_captured != KING);
	}
	
	if (move->flags & FLAG_PROMOTE) {
		assert(promoteto != KING);
		assert(promoteto != PAWN);
		move->promoteto = promoteto;
	}
	
	move->old_castling_flags = board->castling;
	move->old_en_passant = board->en_passant;
}


void construct_null_move(const board_s* restrict board, move_s* restrict move) {
	move->from = 0x0;
	move->to = 0x0;
	move->old_castling_flags = board->castling;
	move->old_en_passant = board->en_passant;
}

*/

void get_pseudo_legal_moves(const board_s* restrict board, movelist_s* restrict moves, const BitBoard piecebb, bool set_move_ordering, BitBoard ignoremask) {
	assert(POPCOUNT(piecebb) == 1);

	// const unsigned int side = get_piece_side(board, piecebb);
	const unsigned int piece_type = PIECE_SQ(board, LOWEST_BITINDEX(piecebb));
	

	//BitBoard to = (*pseudo_legal_squares[piece_type])(board->all_pieces[WHITE], board->all_pieces[BLACK], side, piecebb, true);
	BitBoard to = get_pseudo_legal_squares(board, WHITE, piece_type, piecebb);
	
	/*
	BitBoard to;

	switch (piece_type) {
	
		case PAWN:
			//to = get_pseudo_legal_squares(board, side, piece_type, piecebb, true);
			to = 0x0;
			const bool doublepush = (piecebb & (BOTTOM_DPUSH_MASK | TOP_DPUSH_MASK));
			if (side == WHITE) {
				to = MV_N(piecebb, 1) & ~board->every_piece;
				//BitBoard first_forward = MV_N(piecebb, 1) & ~board->every_piece;
				//to = first_forward;
				to |= (MV_N(to, 1) * doublepush) & ~board->every_piece;
			}
			else  {
				to = MV_S(piecebb, 1) & ~board->every_piece;
				//BitBoard first_forward = MV_S(piecebb, 1) & ~board->every_piece;
				//to = first_forward;
				to |= (MV_S(to, 1) * doublepush) & ~board->every_piece;
			}
			to |= p_attacks(piecebb, side) & (board->all_pieces[OPPOSITE_SIDE(side)] | board->en_passant);
			to &= ~board->all_pieces[side];

			break;
		
		case KING:
			to = k_attacks(piecebb);
			if (side == WHITE) {
				if ((board->castling & WQCASTLE) && !(board->every_piece & WQ_CAST_CLEAR_MASK) && board->pieces[WHITE][ROOK] & A1)
					to |= MV_W(piecebb, 2);
				if (board->castling & WKCASTLE && !(board->every_piece & WK_CAST_CLEAR_MASK) && board->pieces[WHITE][ROOK] & H1)
					to |= MV_E(piecebb, 2);
				}
			else {
				if (board->castling & BQCASTLE && !(board->every_piece & BQ_CAST_CLEAR_MASK) && board->pieces[BLACK][ROOK] & A8)
					to |= MV_W(piecebb, 2);
				if (board->castling & BKCASTLE && !(board->every_piece & BK_CAST_CLEAR_MASK) && board->pieces[BLACK][ROOK] & H8)
					to |= MV_E(piecebb, 2);
			}
			to &= ~board->all_pieces[side];
			break;
		case QUEEN:
			to = q_attacks(piecebb, ~board->every_piece) & ~board->all_pieces[side];
			break;
		case ROOK:
			to = r_attacks(piecebb, ~board->every_piece) & ~board->all_pieces[side];
			break;
		case BISHOP:
			to = b_attacks(piecebb, ~board->every_piece) & ~board->all_pieces[side];
			break;
		case KNIGHT:
			to = n_attacks(piecebb) & ~board->all_pieces[side];
			break;
		default:
			to = 0x0;
			break;
	}
	*/

	// Ignore to-squares that are in ignoremask
	to &= ~ignoremask;

	// now we have all of the proper "to" squares
	// now we just have to assign flags and properly encode them
	//movelist_s moves;
	moves->n = POPCOUNT(to);

	if (moves->n == 0)
		return; // skip everything as there is no moves

	bool promote = false;
	// Check if piece is about to promote
	// this check works becouse a pawn can't have promotions and non-promotions intermixed
	if (piece_type == PAWN && piecebb & W_PROMOTE_FROM_MASK) {
		promote = true;
		moves->n *= N_PROM_PIECES;
	}

	assert(moves->moves);
	//assert(LENGTH(moves->moves) >= moves->n);

	const uint8_t promote_piece_codes[N_PROM_PIECES] = {QUEEN, ROOK, BISHOP, KNIGHT};
	BitBoard last_pop = 0x0;

	// TODO: move ordering would be done here and taken into account in search
	for (unsigned int i = 0; i < moves->n; i++) {
		//moves->moves[i].flags = 0x0;
		
		moves->moves[i].from = LOWEST_BITINDEX(piecebb);

		// BitBoard from = piecebb;
		// BitBoard to_sq;


		//unsigned int promoteto = 0;
		
		// Change the to square only every N_PROM_PIECES
		if (__builtin_expect(promote, 0)) {
			if ((i % N_PROM_PIECES) == 0) {
				last_pop = pop_bitboard(&to);
			}
			assert(last_pop);
			// to_sq = last_pop;
			moves->moves[i].to = LOWEST_BITINDEX(last_pop);
			moves->moves[i].promoteto = promote_piece_codes[i % N_PROM_PIECES];
		}
		else
			moves->moves[i].to = pop_bit(&to);

		//assert(popcount(to_sq) == 1);
		
		//create_move(board, moves->moves + i, from, to_sq, promoteto);
		moves->moves[i] = set_move_flags(moves->moves[i], board);
		

		// if (set_move_ordering)
		// 	set_move_predict_scores(board, moves->moves + i);
		
	}
	// return moves;
}

static inline BitBoard get_pseudo_legal_squares(const board_s* restrict board, unsigned int side, unsigned int piece_type, BitBoard piecebb) {
	
	//BitBoard squares = (*pseudo_legal_squares[piece_type])(MOVING_PIECES(board), OPPONENT_PIECES(board), piecebb);
	BitBoard squares;

	switch (piece_type) {
		case KING:
			squares = pseudo_legal_squares_k(MOVING_PIECES(board), OPPONENT_PIECES(board), piecebb);
			const BitBoard occ = OCCUPANCY(board);
			// if ((board->castling & MQCASTLE) && !(occ & Q_CAST_CLEAR_MASK))
			// 	squares |= Q_CASTLE_KING_TARGET;
			// if (board->castling & MKCASTLE && !(occ & K_CAST_CLEAR_MASK))
			// 	squares |= K_CASTLE_KING_TARGET;

			// squares |= Q_CASTLE_KING_TARGET & ((!(board->castling & MQCASTLE && !(occ & Q_CAST_CLEAR_MASK)))-1);
			// squares |= K_CASTLE_KING_TARGET & ((!(board->castling & MKCASTLE && !(occ & Q_CAST_CLEAR_MASK)))-1);
			
			squares |= Q_CASTLE_KING_TARGET * (board->castling & MQCASTLE && !(occ & Q_CAST_CLEAR_MASK));
			squares |= K_CASTLE_KING_TARGET * (board->castling & MKCASTLE && !(occ & K_CAST_CLEAR_MASK));
			break;
		case PAWN:
			squares = pseudo_legal_squares_p(MOVING_PIECES(board), OPPONENT_PIECES(board), piecebb);
			// Pawn captures (including en passant)
			squares |= p_attacks_n(piecebb) & (OPPONENT_PIECES(board) | (columns[board->en_passant] & EN_PASSANT_TARGET_ROW));
			break;
		case ROOK:
			squares = pseudo_legal_squares_r(MOVING_PIECES(board), OPPONENT_PIECES(board), piecebb);
			break;
		case BISHOP:
			squares = pseudo_legal_squares_b(MOVING_PIECES(board), OPPONENT_PIECES(board), piecebb);
			break;
		case QUEEN:
			squares = pseudo_legal_squares_q(MOVING_PIECES(board), OPPONENT_PIECES(board), piecebb);
			break;
		case KNIGHT:
			squares = pseudo_legal_squares_n(MOVING_PIECES(board), OPPONENT_PIECES(board), piecebb);
			break;
		default:
			squares = 0x0;
	}

	return squares;

	/*
	
	// Add castling (represented by moving 2 squares)
	if (piece_type == KING) {
		
		const BitBoard occ = OCCUPANCY(board);
		if ((board->castling & MQCASTLE) && !(occ & Q_CAST_CLEAR_MASK))
			squares |= Q_CASTLE_KING_TARGET;
		if (board->castling & MKCASTLE && !(occ & K_CAST_CLEAR_MASK))
			squares |= K_CASTLE_KING_TARGET;
		
		// if (side == WHITE) {
		// 	if ((board->castling & MQCASTLE) && !(occ & WQ_CAST_CLEAR_MASK) && ROOKS(board) & A1)
		// 		squares |= MV_W(piecebb, 2);
		// 	if (board->castling & MKCASTLE && !(occ & WK_CAST_CLEAR_MASK) && ROOKS(board) & H1)
		// 		squares |= MV_E(piecebb, 2);
		// }
		// else {
		// 	if (board->castling & OQCASTLE && !(occ & BQ_CAST_CLEAR_MASK) && ROOKS(board) & A8)
		// 		squares |= MV_W(piecebb, 2);
		// 	if (board->castling & OKCASTLE && !(occ & BK_CAST_CLEAR_MASK) && ROOKS(board) & H8)
		// 		squares |= MV_E(piecebb, 2);
		// }
	}
	else if (piece_type == PAWN) {
		// Pawn captures (including en passant)
		squares |= p_attacks(piecebb, side) & (OPPONENT_PIECES(board) | (columns[board->en_passant] & EN_PASSANT_TARGET_ROW));
	}
	return squares;
	*/
}

/*
BitBoard get_legal_moves(const board_s* restrict board, unsigned int side, unsigned int piece_type, BitBoard piecebb) {
	
}
*/

static inline BitBoard pseudo_legal_squares_k(BitBoard w_occupancy, BitBoard b_occupancy, const BitBoard piece) {
	assert(POPCOUNT(piece) == 1);
	// don't eat own pieces
	return k_attacks(piece) & ~w_occupancy;
}


static inline BitBoard pseudo_legal_squares_n(BitBoard w_occupancy, BitBoard b_occupancy, const BitBoard piece) {
	// don't eat own pieces
	//return n_attacks(piece) & ~w_occupancy;
	return knightlookup[LOWEST_BITINDEX(piece)] & ~w_occupancy;
}


static inline BitBoard pseudo_legal_squares_q(BitBoard w_occupancy, BitBoard b_occupancy, const BitBoard piece) {
	// const BitBoard squares = q_attacks(piece, ~(w_occupancy | b_occupancy));
	const unsigned int sq = LOWEST_BITINDEX(piece);
	const BitBoard squares = GenBishop(sq, w_occupancy | b_occupancy)
		| GenRook(sq, w_occupancy | b_occupancy);
	return squares & ~w_occupancy; // don't go on own pieces
}


static inline BitBoard pseudo_legal_squares_b(BitBoard w_occupancy, BitBoard b_occupancy, const BitBoard piece) {
	// const BitBoard squares = b_attacks(piece, ~(w_occupancy | b_occupancy));
	const unsigned int sq = LOWEST_BITINDEX(piece);
	const BitBoard squares = GenBishop(sq, w_occupancy | b_occupancy);
	return squares & ~w_occupancy; // don't go on own pieces
}


static inline BitBoard pseudo_legal_squares_r(BitBoard w_occupancy, BitBoard b_occupancy, const BitBoard piece) {
	// const BitBoard squares = r_attacks(piece, ~(w_occupancy | b_occupancy));
	const unsigned int sq = LOWEST_BITINDEX(piece);
	const BitBoard squares = GenRook(sq, w_occupancy | b_occupancy);
	return squares & ~w_occupancy; // don't go on own pieces
}


static inline BitBoard pseudo_legal_squares_p(BitBoard w_occupancy, BitBoard b_occupancy, const BitBoard piece) {

	BitBoard first_forward;
	BitBoard second_forward;


	const bool doublepush = (piece & (BOTTOM_DPUSH_MASK | TOP_DPUSH_MASK));
	first_forward = MV_N(piece, 1) & ~(w_occupancy | b_occupancy);
	second_forward = (MV_N(first_forward, 1) * doublepush) & ~(w_occupancy | b_occupancy);

	return (first_forward | second_forward);
}


BitBoard k_attacks(BitBoard kings) {
	BitBoard attacks = EAST_ONE(kings) | WEST_ONE(kings);
	kings     |= attacks;
	attacks   |= NORTH_ONE(kings) | SOUTH_ONE(kings);
	return attacks;
}


BitBoard n_attacks(BitBoard knights) {
	BitBoard attacks = 0x0;
	attacks |= ((~TOP_MASK_N    & ~RIGHT_MASK  & knights) << 17);
	attacks |= ((~TOP_MASK_N    & ~LEFT_MASK   & knights) << 15);
	attacks |= ((~RIGHT_MASK_N  & ~TOP_MASK    & knights) << 10);
	attacks |= ((~RIGHT_MASK_N  & ~BOTTOM_MASK & knights) >> 6);
	attacks |= ((~BOTTOM_MASK_N & ~RIGHT_MASK  & knights) >> 15);
	attacks |= ((~BOTTOM_MASK_N & ~LEFT_MASK   & knights) >> 17);
	attacks |= ((~LEFT_MASK_N   & ~TOP_MASK    & knights) << 6);
	attacks |= ((~LEFT_MASK_N   & ~BOTTOM_MASK & knights) >> 10);
	return attacks;
}

// BitBoard p_attacks(BitBoard pawns, int side) {
// 	if (side == WHITE)
// 		pawns = (pawns & ~TOP_MASK) << 8;
// 	else
// 		pawns = (pawns & ~BOTTOM_MASK) >> 8;
// 	return ((pawns & ~RIGHT_MASK) << 1) | ((pawns & ~LEFT_MASK) >> 1);
// }

static inline BitBoard p_attacks_n(BitBoard pawns) {
	pawns <<= 8;
	return ((pawns & ~RIGHT_MASK) << 1) | ((pawns & ~LEFT_MASK) >> 1);
}

static inline BitBoard p_attacks_s(BitBoard pawns) {
	pawns >>= 8;
	return ((pawns & ~RIGHT_MASK) << 1) | ((pawns & ~LEFT_MASK) >> 1);
}


/* return the bitboard with the rook destinations */
static inline BitBoard GenRook(unsigned int sq, BitBoard occupation) {

	BitBoard piece = 1ULL<<sq;
	occupation ^= piece; /* remove the selected piece from the occupation */
	BitBoard piecesup=(0x0101010101010101ULL<<sq)&(occupation|0xFF00000000000000ULL); /* find the pieces up */
	BitBoard piecesdo=(0x8080808080808080ULL>>(63-sq))&(occupation|0x00000000000000FFULL); /* find the pieces down */
	BitBoard piecesri=(0x00000000000000FFULL<<sq)&(occupation|0x8080808080808080ULL); /* find pieces on the right */
	BitBoard piecesle=(0xFF00000000000000ULL>>(63-sq))&(occupation|0x0101010101010101ULL); /* find pieces on the left */
	return (((0x8080808080808080ULL>>(63-LOWEST_BITINDEX(piecesup)))&(0x0101010101010101ULL<<HIGHEST_BITINDEX(piecesdo))) |
			((0xFF00000000000000ULL>>(63-LOWEST_BITINDEX(piecesri)))&(0x00000000000000FFULL<<HIGHEST_BITINDEX(piecesle))))^piece;
	/* From every direction find the first piece and from that piece put a mask in the opposite direction.
		Put togheter all the 4 masks and remove the moving piece */
}

/* return the bitboard with the bishops destinations */
static inline BitBoard GenBishop(unsigned int sq,BitBoard occupation) {

	/* it's the same as the rook */
	BitBoard piece = 1ULL<<sq;
	occupation ^= piece;
	BitBoard piecesup=(0x8040201008040201ULL<<sq)&(occupation|0xFF80808080808080ULL);
	BitBoard piecesdo=(0x8040201008040201ULL>>(63-sq))&(occupation|0x01010101010101FFULL);
	BitBoard piecesle=(0x8102040810204081ULL<<sq)&(occupation|0xFF01010101010101ULL);
	BitBoard piecesri=(0x8102040810204081ULL>>(63-sq))&(occupation|0x80808080808080FFULL);
	return (((0x8040201008040201ULL>>(63-LOWEST_BITINDEX(piecesup)))&(0x8040201008040201ULL<<HIGHEST_BITINDEX(piecesdo))) |
			((0x8102040810204081ULL>>(63-LOWEST_BITINDEX(piecesle)))&(0x8102040810204081ULL<<HIGHEST_BITINDEX(piecesri))))^piece;
}


BitBoard r_attacks(BitBoard rooks, BitBoard empty) {
	return north_attacks(rooks, empty)
	  | east_attacks(rooks, empty)
	  | south_attacks(rooks, empty)
	  | west_attacks(rooks, empty);
}
BitBoard b_attacks(BitBoard bishops, BitBoard empty) {
	return noeast_attacks(bishops, empty)
	  | soeast_attacks(bishops, empty)
	  | sowest_attacks(bishops, empty)
	  | nowest_attacks(bishops, empty);
}
BitBoard q_attacks(BitBoard queens, BitBoard empty) {
	return r_attacks(queens, empty) | b_attacks(queens, empty);
}




// Kogge-Stone sliders

static inline BitBoard south_attacks  (BitBoard pieces, BitBoard empty) { return SOUTH_ONE (soutOccl(pieces, empty)); }
static inline BitBoard north_attacks  (BitBoard pieces, BitBoard empty) { return NORTH_ONE (nortOccl(pieces, empty)); }
static inline BitBoard east_attacks   (BitBoard pieces, BitBoard empty) { return EAST_ONE  (eastOccl(pieces, empty)); }
static inline BitBoard noeast_attacks (BitBoard pieces, BitBoard empty) { return NOEAST_ONE(noEaOccl(pieces, empty)); }
static inline BitBoard soeast_attacks (BitBoard pieces, BitBoard empty) { return SOEAST_ONE(soEaOccl(pieces, empty)); }
static inline BitBoard west_attacks   (BitBoard pieces, BitBoard empty) { return WEST_ONE  (westOccl(pieces, empty)); }
static inline BitBoard sowest_attacks (BitBoard pieces, BitBoard empty) { return SOWEST_ONE(soWeOccl(pieces, empty)); }
static inline BitBoard nowest_attacks (BitBoard pieces, BitBoard empty) { return NOWEST_ONE(noWeOccl(pieces, empty)); }

static inline BitBoard soutOccl(BitBoard gen, BitBoard pro) {
	gen |= pro & (gen >>  8);
	pro &=       (pro >>  8);
	gen |= pro & (gen >> 16);
	pro &=       (pro >> 16);
	gen |= pro & (gen >> 32);
	return gen;
}

static inline BitBoard nortOccl(BitBoard gen, BitBoard pro) {
	gen |= pro & (gen <<  8);
	pro &=       (pro <<  8);
	gen |= pro & (gen << 16);
	pro &=       (pro << 16);
	gen |= pro & (gen << 32);
	return gen;
}

static inline BitBoard eastOccl(BitBoard gen, BitBoard pro) {
	pro &= ~LEFT_MASK;
	gen |= pro & (gen << 1);
	pro &=       (pro << 1);
	gen |= pro & (gen << 2);
	pro &=       (pro << 2);
	gen |= pro & (gen << 4);
	return gen;
}

static inline BitBoard noEaOccl(BitBoard gen, BitBoard pro) {
	pro &= ~LEFT_MASK;
	gen |= pro & (gen <<  9);
	pro &=       (pro <<  9);
	gen |= pro & (gen << 18);
	pro &=       (pro << 18);
	gen |= pro & (gen << 36);
	return gen;
}

static inline BitBoard soEaOccl(BitBoard gen, BitBoard pro) {
	pro &= ~LEFT_MASK;
	gen |= pro & (gen >>  7);
	pro &=       (pro >>  7);
	gen |= pro & (gen >> 14);
	pro &=       (pro >> 14);
	gen |= pro & (gen >> 28);
	return gen;
}

static inline BitBoard westOccl(BitBoard gen, BitBoard pro) {
	pro &= ~RIGHT_MASK;
	gen |= pro & (gen >> 1);
	pro &=       (pro >> 1);
	gen |= pro & (gen >> 2);
	pro &=       (pro >> 2);
	gen |= pro & (gen >> 4);
	return gen;
}

static inline BitBoard soWeOccl(BitBoard gen, BitBoard pro) {
	pro &= ~RIGHT_MASK;
	gen |= pro & (gen >>  9);
	pro &=       (pro >>  9);
	gen |= pro & (gen >> 18);
	pro &=       (pro >> 18);
	gen |= pro & (gen >> 36);
	return gen;
}

static inline BitBoard noWeOccl(BitBoard gen, BitBoard pro) {
	pro &= ~RIGHT_MASK;
	gen |= pro & (gen <<  7);
	pro &=       (pro <<  7);
	gen |= pro & (gen << 14);
	pro &=       (pro << 14);
	gen |= pro & (gen << 28);
	return gen;
}


