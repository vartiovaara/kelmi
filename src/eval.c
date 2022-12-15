#include <stdlib.h>
#include <stdbool.h>

#include "eval.h"
#include "bitboard.h"
#include "lookup.h"
#include "movegen.h"
#include "board.h"

#include "defs.h"



/*
 * Private data.
 */


const eval_t eval_material_value[N_PIECES] = {
	[PAWN] = EVAL_PAWN_MATERIAL_VALUE,
	[KNIGHT] = EVAL_KNIGHT_MATERIAL_VALUE,
	[BISHOP] = EVAL_BISHOP_MATERIAL_VALUE,
	[ROOK] = EVAL_ROOK_MATERIAL_VALUE,
	[QUEEN] = EVAL_QUEEN_MATERIAL_VALUE,
	[KING] = EVAL_MAX // not really needed as king is never needed in any eval
};

const eval_t mv_move_weight[N_PIECES] = {
	[PAWN] = MV_SCORE_MOVE_WEIGHT_PAWN,
	[KNIGHT] = MV_SCORE_MOVE_WEIGHT_KNIGHT,
	[BISHOP] = MV_SCORE_MOVE_WEIGHT_BISHOP,
	[ROOK] = MV_SCORE_MOVE_WEIGHT_ROOK,
	[QUEEN] = MV_SCORE_MOVE_WEIGHT_QUEEN,
	[KING] = MV_SCORE_MOVE_WEIGHT_KING
};


/*
	  A   B    C    D    E    F    G     H*/
/*const eval_t psqt_pawn[64] = {
	  0,   0,   0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,   0,   0,
	  5,   0,  11,  10,  10,  11,   0,   5,
	  4,   0,  13,  14,  15,  12,   0,  10,
	  0,   0,   5,   5,   5,   5,   0,   0,
	  0,   0,   0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,   0,   0,
	100, 100, 100, 100, 100, 100, 100, 100
};*/


eval_t psqt_pawn[64] = {
	100, 100, 100, 100, 100, 100, 100, 100,
	 11,  11,  11,  11,  11,  11,  11,  11,
	 10,  10,  10,  10,  10,  10,  10,  10,
	 10,   5,   7,  13,  13,   8,   5,   8,
	  5,  10,  13,  20,  25,  12,  -1,  15,
	  7,   9,  11,  12,  12,  11,  -1,  11,
	  0,   0,   3,  -1,  -1,   0,  18,   0,
	  0,   0,   0,   0,   0,   0,   0,   0
};


eval_t psqt_knight[64] = {
	-20, -10,  -10,  -10,  -10,  -10,  -10,  -20,
	-10,  -5,   -5,   -5,   -5,   -5,   -5,  -10,
	-10,  -5,   15,   15,   15,   15,   -5,  -10,
	-10,  -5,   15,   15,   15,   15,   -5,  -10,
	-10,  -5,   15,   15,   15,   15,   -5,  -10,
	-11,  -5,   10,   15,   15,   15,   -5,  -11,
	-10,  -5,   -5,   -5,   -5,   -5,   -5,  -10,
	-20,   0,  -10,  -10,  -10,  -10,    0,  -20
};

eval_t psqt_bishop[64] = {
	-29,   4, -82, -37, -25, -42,   7,  -8,
	-26,  16, -18, -13,  30,  59,  18, -47,
	-16,  37,  43,  40,  35,  50,  37,  -2,
	 -4,   5,  19,  50,  37,  37,   7,  -2,
	 -6,  13,  13,  26,  34,  12,  10,   4,
	  0,  15,  15,  15,  14,  27,  18,  10,
	  4,  15,  16,   0,   7,  21,  33,   1,
	-33,  -3, -14, -21, -13, -12, -39, -21,
};

eval_t psqt_rook[64] = {
	 0,   0,   0,   0,   0,   0,   0,   0,
	15,  15,  15,  20,  20,  15,  15,  15,
	 0,   0,   0,   0,   0,   0,   0,   0,
	 0,   0,   0,   0,   0,   0,   0,   0,
	 0,   0,   0,   0,   0,   0,   0,   0,
	 0,   0,   0,   0,   0,   0,   0,   0,
	 0,   0,   0,   0,   0,   0,   0,   0,
	 0,   0,   0,  10,  10,  10,   0,   0
};

eval_t psqt_king[64] = {
	 0,   0,   0,   0,   0,   0,   0,   0,
	 0,   0,   0,   0,   0,   0,   0,   0,
	 0,   0,   0,   0,   0,   0,   0,   0,
	 0,   0,   0,   0,   0,   0,   0,   0,
	 0,   0,   0,   0,   0,   0,   0,   0,
	 0,   0,   0,   0,   0,   0,   0,   0,
	 0,   0,   0,   0,   0,   0,   0,   0,
	 0,   10,  20,   0, -10,   0,  20,   0
};

eval_t* psqt_values[N_PIECES] = {
	[KING] = psqt_king,
	[QUEEN] = NULL,
	[ROOK] = psqt_rook,
	[BISHOP] = psqt_bishop,
	[KNIGHT] = psqt_knight,
	[PAWN] = psqt_pawn
};

// King is in the 42 index
eval_t psqt_king_guard_pieces[64] = {
	 0,   0,   0,   0,   0,   0,   0,   0,
	 0,   0,   0,   0,   0,   0,   0,   0,
	 0,   0,   0,   0,   0,   0,   0,   0,
	 8,   0,  17,   0,   8,   0,   0,   0,
	 2,  30,  38,  30,   2,   0,   0,   0,
	15,  31,   0,  38,  15,   0,   0,   0,
	 0,  35,  32,  35,   0,   0,   0,   0,
	 8,   0,  15,   0,   8,   0,   0,   0,
};

/*
eval_t psqt_king_guard_pieces[64] = {
	 0,   0,   0,   0,   0,   0,   0,   0,
	 0,   0,   0,   0,   0,   0,   0,   0,
	 0,   0,   0,   0,   0,   0,   0,   0,
	12,   0,  38,   0,  12,   0,   0,   0,
	 2,  35,  50,  35,   2,   0,   0,   0,
	15,  48,   0,  48,  15,   0,   0,   0,
	 0,  35,  50,  35,   0,   0,   0,   0,
	12,   0,  38,   0,  12,   0,   0,   0,
};
*/

/*
const eval_t psqt_blank[64] = {
	  0,   0,   0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,   0,   0
};
*/




/*
 * Private functions.
 */

// evaluation functions
//eval_t eval_material(const board_s* board); // public
eval_t eval_stacked_pawns(const board_s* board);
eval_t eval_psqt(const board_s* board);
eval_t eval_rook_open_file(const board_s* board);
eval_t eval_movable_squares(const board_s* board);
eval_t eval_king_guard(const board_s* board);
eval_t eval_castling_rights(const board_s* board);
eval_t eval_tempo_bonus(const board_s* board);




inline eval_t better_eval(eval_t a, eval_t b, unsigned int side) {
	return (is_eval_better(a, b, side) ? a : b);
}

inline bool is_eval_better(eval_t a, eval_t b, unsigned int side) {
	return ((side == WHITE) ? a > b : a < b);
}

inline eval_t eval(const board_s* board) {
	eval_t eval = 0;

	eval += eval_material(board);
	eval += eval_stacked_pawns(board);
	eval += eval_psqt(board);
	eval += eval_rook_open_file(board);
	eval += eval_movable_squares(board);
	//eval += eval_king_guard(board);
	eval += eval_castling_rights(board);
	//eval += eval_tempo_bonus(board);

	return eval;
	
	// NOTE: INTRERESTING this evaluation function gives coherent moves
	//return (10 / ((rand() % 20)+1)) * ((rand() % 2 == 1) ? -1 : 1);
}


// Evaluation function definitions
eval_t eval_material(const board_s* board) {
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

	// Maybe not needed???
	// Compare kings
	//const unsigned int n_kings_w = popcount(board->pieces[WHITE][KING]);
	//const unsigned int n_kings_b = popcount(board->pieces[BLACK][KING]);
	//res += (n_kings_w - n_kings_b) * EVAL_MAX;

	// Material imbalance is more accentuated when material is low
	// This makes the overall playing better but is bad for move ordering
	if (res > 0)
		res += (32 - popcount(board->every_piece)) * EVAL_MATERIAL_IMBALANCE_ACCENTUATE_MULT; // 32 is starting n of pieces
	else if (res < 0)
		res -= (32 - popcount(board->every_piece)) * EVAL_MATERIAL_IMBALANCE_ACCENTUATE_MULT;

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
		if (popcount(pawns_w & columnlookup(lowest_bitindex(pawn))) > 1)
			res -= EVAL_STACKED_PAWNS_PUNISHMENT;
	}

	// black
	const BitBoard pawns_b = board->pieces[BLACK][PAWN];
	BitBoard pawns_b_copy = pawns_b;
	while (pawns_b_copy) {
		BitBoard pawn = pop_bitboard(&pawns_b_copy);
		if (popcount(pawns_b & columnlookup(lowest_bitindex(pawn))) > 1)
			res += EVAL_STACKED_PAWNS_PUNISHMENT;
	}

	return res;
}

eval_t eval_psqt(const board_s* board) {
	eval_t res = 0;

	// Remember that the tables are from black. access mirrored for white
	
	// Pawns
	BitBoard pawns_w = flip_vertical(board->pieces[WHITE][PAWN]);
	while (pawns_w) {
		res += psqt_pawn[pop_bit(&pawns_w)];
	}

	BitBoard pawns_b = board->pieces[BLACK][PAWN];
	while (pawns_b) {
		res -= psqt_pawn[pop_bit(&pawns_b)];
	}

	// Knights
	BitBoard knights_w = flip_vertical(board->pieces[WHITE][KNIGHT]);
	while (knights_w) {
		res += psqt_knight[pop_bit(&knights_w)];
	}

	BitBoard knights_b = board->pieces[BLACK][KNIGHT];
	while (knights_b) {
		res -= psqt_knight[pop_bit(&knights_b)];
	}

	// Bishops
	BitBoard bishops_w = flip_vertical(board->pieces[WHITE][BISHOP]);
	while (bishops_w) {
		res += psqt_bishop[pop_bit(&bishops_w)];
	}

	BitBoard bishops_b = board->pieces[BLACK][BISHOP];
	while (bishops_b) {
		res -= psqt_bishop[pop_bit(&bishops_b)];
	}

	// Rooks
	BitBoard rooks_w = flip_vertical(board->pieces[WHITE][ROOK]);
	while (rooks_w) {
		res += psqt_rook[pop_bit(&rooks_w)];
	}

	BitBoard rooks_b = board->pieces[BLACK][ROOK];
	while (rooks_b) {
		res -= psqt_rook[pop_bit(&rooks_b)];
	}
	
	// Kings
	BitBoard kings_w = flip_vertical(board->pieces[WHITE][KING]);
	while (kings_w) {
		res += psqt_king[pop_bit(&kings_w)];
	}

	BitBoard kings_b = board->pieces[BLACK][KING];
	while (kings_b) {
		res -= psqt_king[pop_bit(&kings_b)];
	}

	return res;
}

eval_t eval_rook_open_file(const board_s* board) {
	eval_t res = 0;

	// white
	BitBoard rooks_w = board->pieces[WHITE][ROOK];
	while (rooks_w) {
		const BitBoard rook = pop_bitboard(&rooks_w);
		const BitBoard rook_file_pieces = board->all_pieces[WHITE] & columnlookup(lowest_bitindex(rook));
		if (popcount(rook_file_pieces) != 1) {
			// check if there are pieces above the rook
			if (!((rook_file_pieces & ~rook) > rook))
				res += EVAL_ROOK_OPEN_FILE;
		}
		else
			res += EVAL_ROOK_OPEN_FILE;
	}

	// black
	BitBoard rooks_b = board->pieces[BLACK][ROOK];
	while (rooks_b) {
		const BitBoard rook = pop_bitboard(&rooks_b);
		const BitBoard rook_file_pieces = board->all_pieces[BLACK] & columnlookup(lowest_bitindex(rook));
		if (popcount(rook_file_pieces) != 1) {
			// check if there are pieces below the rook
			if (!((rook_file_pieces & ~rook) < rook))
				res -= EVAL_ROOK_OPEN_FILE;
		}
		else
			res -= EVAL_ROOK_OPEN_FILE;
	}

	return res;
}

// TODO: Make those fucking attack maps
// FIXME: you lazy fuck
eval_t eval_movable_squares(const board_s* board) {
	eval_t res = 0;

	BitBoard w_pieces = board->all_pieces[WHITE];
	while (w_pieces) {
		const BitBoard piece = pop_bitboard(&w_pieces);
		movelist_s moves = get_pseudo_legal_squares(board, piece);
		res += moves.n * EVAL_MOVABLE_SQUARES_MULT;
		if (moves.n)
			free(moves.moves);
	}

	BitBoard b_pieces = board->all_pieces[BLACK];
	while (b_pieces) {
		const BitBoard piece = pop_bitboard(&b_pieces);
		movelist_s moves = get_pseudo_legal_squares(board, piece);
		res -= moves.n * EVAL_MOVABLE_SQUARES_MULT;
		if (moves.n)
			free(moves.moves);
	}

	return res;
}


eval_t eval_king_guard(const board_s* board) {
	eval_t res = 0;

	// signed because delta later might go negative
	const int w_king_pos = lowest_bitindex(board->pieces[WHITE][KING]);
	const int b_king_pos = lowest_bitindex(flip_vertical(board->pieces[BLACK][KING]));

	BitBoard guarding_pieces_w = board->all_pieces[WHITE] & king_guard_lookup(w_king_pos);
	BitBoard guarding_pieces_b = flip_vertical(board->all_pieces[BLACK]) & king_guard_lookup(b_king_pos);

	while (guarding_pieces_w) {
		const int piece_pos = pop_bit(&guarding_pieces_w);
		res += psqt_king_guard_pieces[42 + (piece_pos - w_king_pos)];
	}

	while (guarding_pieces_b) {
		const int piece_pos = pop_bit(&guarding_pieces_b);
		res -= psqt_king_guard_pieces[42 + (piece_pos - b_king_pos)];
	}

	return res;
}


eval_t eval_castling_rights(const board_s* board) {
	eval_t res = 0;

	res += (board->castling & WKCASTLE) * EVAL_CASTLING_RIGHTS_K;
	res -= (board->castling & BKCASTLE) * EVAL_CASTLING_RIGHTS_K;
	res += (board->castling & WQCASTLE) * EVAL_CASTLING_RIGHTS_Q;
	res -= (board->castling & BQCASTLE) * EVAL_CASTLING_RIGHTS_Q;

	return res;
}


// FIXME: Implement computer side. Function doesn't work otherwise
// https://www.chessprogramming.org/Tempo
eval_t eval_tempo_bonus(const board_s* board) {
	if (board->sidetomove == WHITE)
		return EVAL_TEMPO_BONUS;
	return -EVAL_TEMPO_BONUS;
}



eval_t get_move_predict_score(const board_s* board, const move_s* move) {
	eval_t score = 0;

	// check if opponent is in check
	// this works becouse opponent could not have already been in check before this move.
	//if (is_in_check(board, OPPOSITE_SIDE(move->side)))
	//	score += MV_SCORE_CHECK;

	//score += (move->flags & FLAG_PROMOTE) * MV_SCORE_CHECK;

	if (move->flags & FLAG_PROMOTE)
		score += eval_material_value[move->promoteto] - eval_material_value[PAWN];

	// TODO: Can be further improved by removing attackers and doing same again (discovered attacks)
	if (move->flags & FLAG_CAPTURE) {
		score += eval_material_value[move->piece_captured];
		//score -= eval_material_value[move->fromtype]/MV_SCORE_CAPTURER_VALUE_DIVIDE;

		BitBoard attackers = get_attackers(board, move->to, move->side); // & ~(move->from); // movers pieces
		BitBoard defenders = get_attackers(board, move->to, OPPOSITE_SIDE(move->side)) & ~(move->from);

		while (attackers)
			score -= eval_material_value[get_piece_type(board, move->side, pop_bitboard(&attackers))];

		while (defenders)
			score += eval_material_value[get_piece_type(board, OPPOSITE_SIDE(move->side), pop_bitboard(&defenders))];
	}


	 if (!(move->flags & FLAG_CAPTURE) && !(move->flags & (FLAG_KCASTLE | FLAG_QCASTLE)))
	 	score += mv_move_weight[move->fromtype];
	
	if (move->flags & FLAG_KCASTLE)
		score += MV_SCORE_KCASTLE;
	else if (move->flags & FLAG_QCASTLE)
		score += MV_SCORE_QCASTLE;

	if (psqt_values[move->fromtype] != NULL) {
		const unsigned int from_sq = lowest_bitindex(move->from);
		const unsigned int to_sq = lowest_bitindex(move->to);
		if (move->flags & FLAG_PROMOTE) {
			if (psqt_values[move->promoteto]) {
				score -= psqt_values[move->fromtype][from_sq];
				score += psqt_values[move->promoteto][to_sq];
			}
		}
		else {
			score -= psqt_values[move->fromtype][from_sq];
			score += psqt_values[move->fromtype][to_sq];
		}
	}


	return score;
}

eval_t see(board_s* restrict board, const move_s* move) {
	eval_t res = 0;

	res += eval_material_value[move->piece_captured];
	//score -= eval_material_value[move->fromtype]/MV_SCORE_CAPTURER_VALUE_DIVIDE;

	BitBoard attackers = get_attackers(board, move->to, move->side) & ~(move->from); // movers pieces
	BitBoard defenders = get_attackers(board, move->to, OPPOSITE_SIDE(move->side));

	//FIXME: Doesn't find discovered attacks
	while (attackers)
		res -= eval_material_value[get_piece_type(board, move->side, pop_bitboard(&attackers))];
	while (defenders)
		res += eval_material_value[get_piece_type(board, OPPOSITE_SIDE(move->side), pop_bitboard(&defenders))];


	/*while (defenders && attackers) {
		// n_pieces counted this loop
		const unsigned int n_pieces = MIN(popcount(attackers), popcount(defenders));



		// Next we will find the n_pieces lowest pieces




		BitBoard attackers_copy = attackers;
		while (attackers_copy)
			res -= eval_material_value[get_piece_type(board, move->side, pop_bitboard(&attackers_copy))];
		BitBoard defenders_copy = defenders;
		while (defenders_copy)
			res += eval_material_value[get_piece_type(board, OPPOSITE_SIDE(move->side), pop_bitboard(&defenders_copy))];
	}*/

	return res;
}
