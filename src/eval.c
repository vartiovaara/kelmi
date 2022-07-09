#include <stdlib.h>
#include <stdbool.h>

#include "eval.h"
#include "bitboard.h"
#include "lookup.h"

#include "defs.h"



/*
 * Private functions.
 */

// evaluation functions
eval_t eval_compare_material_amount(const board_s* board);
eval_t eval_stacked_pawns(const board_s* board);




eval_t better_eval(eval_t a, eval_t b, unsigned int side) {
	return (is_eval_better(a, b, side) ? a : b);
}

bool is_eval_better(eval_t a, eval_t b, unsigned int side) {
	return ((side == WHITE) ? a > b : a < b);
}

eval_t eval(const board_s* board) {
	eval_t eval = 0;

	eval += eval_compare_material_amount(board);
	eval += eval_stacked_pawns(board);

	return eval;
	
	// NOTE: INTRERESTING this evaluation function gives coherent moves
	//return (10 / ((rand() % 20)+1)) * ((rand() % 2 == 1) ? -1 : 1);
}


// Evaluation function definitions
eval_t eval_compare_material_amount(const board_s* board) {
	eval_t res = 0;

	// Compare pawns
	const unsigned int n_pawns_w = popcount(board->pieces[WHITE][PAWN]);
	const unsigned int n_pawns_b = popcount(board->pieces[BLACK][PAWN]);
	res += (n_pawns_w - n_pawns_b) * EVAL_PAWN_MATERIAL_VALUE;

	// Compare knights
	//unsigned int n_pawns_w = popcount(board->pieces[WHITE][PAWN]);
	//unsigned int n_pawns_b = popcount(board->pieces[BLACK][PAWN]);
	//res += (float)(n_pawns_w - n_pawns_b);
	res += 
		(popcount(board->pieces[WHITE][KNIGHT])
		- popcount(board->pieces[BLACK][KNIGHT])) * EVAL_KNIGHT_MATERIAL_VALUE;

	// Compare bishops
	const unsigned int n_bishops_w = popcount(board->pieces[WHITE][BISHOP]);
	const unsigned int n_bishops_b = popcount(board->pieces[BLACK][BISHOP]);
	res += (n_bishops_w - n_bishops_b) * EVAL_BISHOP_MATERIAL_VALUE;
	// Add in bishop pairs (half a pawn more)
	if (n_bishops_w > 1)
		res += EVAL_BPAIR_VALUE;
	if (n_bishops_b > 1)
		res += -EVAL_BPAIR_VALUE;

	// Compare rooks
	const unsigned int n_rooks_w = popcount(board->pieces[WHITE][ROOK]);
	const unsigned int n_rooks_b = popcount(board->pieces[BLACK][ROOK]);
	res += (n_rooks_w - n_rooks_b) * EVAL_ROOK_MATERIAL_VALUE;

	// Compare queens
	const unsigned int n_queens_w = popcount(board->pieces[WHITE][QUEEN]);
	const unsigned int n_queens_b = popcount(board->pieces[BLACK][QUEEN]);
	res += (n_queens_w - n_queens_b) * EVAL_QUEEN_MATERIAL_VALUE;

	return res;
}

eval_t eval_stacked_pawns(const board_s* board) {
	eval_t res = 0;

	// TODO: Maybe there is a more efficient way of doing this

	// white
	const BitBoard pawns_w = board->pieces[WHITE][PAWN];
	BitBoard pawns_w_copy = pawns_w;
	while (pawns_w_copy) {
		BitBoard pawn = pop_bitboard(&pawns_w_copy);
		if (popcount(pawns_w & columnlookup(lowest_bitindex(pawn))) > 1) {
			res -= EVAL_STACKED_PAWNS_PUNISHMENT;
		}
	}

	// black
	const BitBoard pawns_b = board->pieces[BLACK][PAWN];
	BitBoard pawns_b_copy = pawns_b;
	while (pawns_b_copy) {
		BitBoard pawn = pop_bitboard(&pawns_b_copy);
		if (popcount(pawns_b & columnlookup(lowest_bitindex(pawn))) > 1) {
			res += EVAL_STACKED_PAWNS_PUNISHMENT;
		}
	}

	return res;
}