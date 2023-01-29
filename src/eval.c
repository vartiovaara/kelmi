#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>

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

/*
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
*/

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


#define FLIP_PSQT_SQ(sq) (sq^56)

/* piece/sq tables */
/* values from Rofchade: http://www.talkchess.com/forum3/viewtopic.php?f=2&t=68311&start=19 */

const eval_t psqt[6][2][64] = { //[piece][mg/eg][sq]
	[PAWN] = {{
	  0,   0,   0,   0,   0,   0,  0,   0,
	 98, 134,  61,  95,  68, 126, 34, -11,
	 -6,   7,  26,  31,  65,  56, 25, -20,
	-14,  13,   6,  21,  23,  12, 17, -23,
	-27,  -2,  -5,  12,  17,   6, 10, -25,
	-26,  -4,  -4, -10,   3,   3, 33, -12,
	-35,  -1, -20, -23, -15,  24, 38, -22,
	  0,   0,   0,   0,   0,   0,  0,   0}, 
	{
	  0,   0,   0,   0,   0,   0,   0,   0,
	178, 173, 158, 134, 147, 132, 165, 187,
	 94, 100,  85,  67,  56,  53,  82,  84,
	 32,  24,  13,   5,  -2,   4,  17,  17,
	 13,   9,  -3,  -7,  -7,  -8,   3,  -1,
	  4,   7,  -6,   1,   0,  -5,  -1,  -8,
	 13,   8,   8,  10,  13,   0,   2,  -7,
	  0,   0,   0,   0,   0,   0,   0,   0}
	},
	[KNIGHT] = {{
	-167, -89, -34, -49,  61, -97, -15, -107,
	 -73, -41,  72,  36,  23,  62,   7,  -17,
	 -47,  60,  37,  65,  84, 129,  73,   44,
	  -9,  17,  19,  53,  37,  69,  18,   22,
	 -13,   4,  16,  13,  28,  19,  21,   -8,
	 -23,  -9,  12,  10,  19,  17,  25,  -16,
	 -29, -53, -12,  -3,  -1,  18, -14,  -19,
	-105, -21, -58, -33, -17, -28, -19,  -23}, 
	{
	-58, -38, -13, -28, -31, -27, -63, -99,
	-25,  -8, -25,  -2,  -9, -25, -24, -52,
	-24, -20,  10,   9,  -1,  -9, -19, -41,
	-17,   3,  22,  22,  22,  11,   8, -18,
	-18,  -6,  16,  25,  16,  17,   4, -18,
	-23,  -3,  -1,  15,  10,  -3, -20, -22,
	-42, -20, -10,  -5,  -2, -20, -23, -44,
	-29, -51, -23, -15, -22, -18, -50, -64}
	},
	[BISHOP] = {{
	-29,   4, -82, -37, -25, -42,   7,  -8,
	-26,  16, -18, -13,  30,  59,  18, -47,
	-16,  37,  43,  40,  35,  50,  37,  -2,
	 -4,   5,  19,  50,  37,  37,   7,  -2,
	 -6,  13,  13,  26,  34,  12,  10,   4,
	  0,  15,  15,  15,  14,  27,  18,  10,
	  4,  15,  16,   0,   7,  21,  33,   1,
	-33,  -3, -14, -21, -13, -12, -39, -21}, 
	{
	-14, -21, -11,  -8, -7,  -9, -17, -24,
	 -8,  -4,   7, -12, -3, -13,  -4, -14,
	  2,  -8,   0,  -1, -2,   6,   0,   4,
	 -3,   9,  12,   9, 14,  10,   3,   2,
	 -6,   3,  13,  19,  7,  10,  -3,  -9,
	-12,  -3,   8,  10, 13,   3,  -7, -15,
	-14, -18,  -7,  -1,  4,  -9, -15, -27,
	-23,  -9, -23,  -5, -9, -16,  -5, -17}
	},
	[ROOK] = {{
	 32,  42,  32,  51, 63,  9,  31,  43,
	 27,  32,  58,  62, 80, 67,  26,  44,
	 -5,  19,  26,  36, 17, 45,  61,  16,
	-24, -11,   7,  26, 24, 35,  -8, -20,
	-36, -26, -12,  -1,  9, -7,   6, -23,
	-45, -25, -16, -17,  3,  0,  -5, -33,
	-44, -16, -20,  -9, -1, 11,  -6, -71,
	-19, -13,   1,  17, 16,  7, -37, -26}, 
	{
	13, 10, 18, 15, 12,  12,   8,   5,
	11, 13, 13, 11, -3,   3,   8,   3,
	 7,  7,  7,  5,  4,  -3,  -5,  -3,
	 4,  3, 13,  1,  2,   1,  -1,   2,
	 3,  5,  8,  4, -5,  -6,  -8, -11,
	-4,  0, -5, -1, -7, -12,  -8, -16,
	-6, -6,  0,  2, -9,  -9, -11,  -3,
	-9,  2,  3, -1, -5, -13,   4, -20}
	},
	[QUEEN] = {{
	-28,   0,  29,  12,  59,  44,  43,  45,
	-24, -39,  -5,   1, -16,  57,  28,  54,
	-13, -17,   7,   8,  29,  56,  47,  57,
	-27, -27, -16, -16,  -1,  17,  -2,   1,
	 -9, -26,  -9, -10,  -2,  -4,   3,  -3,
	-14,   2, -11,  -2,  -5,   2,  14,   5,
	-35,  -8,  11,   2,   8,  15,  -3,   1,
	 -1, -18,  -9,  10, -15, -25, -31, -50},
	{
	 -9,  22,  22,  27,  27,  19,  10,  20,
	-17,  20,  32,  41,  58,  25,  30,   0,
	-20,   6,   9,  49,  47,  35,  19,   9,
	  3,  22,  24,  45,  57,  40,  57,  36,
	-18,  28,  19,  47,  31,  34,  39,  23,
	-16, -27,  15,   6,   9,  17,  10,   5,
	-22, -23, -30, -16, -16, -23, -36, -32,
	-33, -28, -22, -43,  -5, -32, -20, -41}
	},
	[KING] = {{
	-65,  23,  16, -15, -56, -34,   2,  13,
	 29,  -1, -20,  -7,  -8,  -4, -38, -29,
	 -9,  24,   2, -16, -20,   6,  22, -22,
	-17, -20, -12, -27, -30, -25, -14, -36,
	-49,  -1, -27, -39, -46, -44, -33, -51,
	-14, -14, -22, -46, -44, -30, -15, -27,
	  1,   7,  -8, -64, -43, -16,   9,   8,
	-15,  36,  12, -54,   8, -28,  24,  14},
	{
	-74, -35, -18, -18, -11,  15,   4, -17,
	-12,  17,  14,  17,  17,  38,  23,  11,
	 10,  17,  23,  15,  20,  45,  44,  13,
	 -8,  22,  24,  27,  26,  33,  26,   3,
	-18,  -4,  21,  24,  27,  23,   9, -11,
	-19,  -3,  11,  21,  23,  16,   7,  -9,
	-27, -11,   4,  13,  14,   4,  -5, -17,
	-53, -34, -21, -11, -28, -14, -24, -43}
	}
};

const unsigned int phase_values[N_PIECES] = {
	[PAWN] = 0,
	[KNIGHT] = 1,
	[BISHOP] = 1,
	[ROOK] = 2,
	[QUEEN] = 4,
	[KING] = 0 // Not needed
};

#define PHASE_TOTAL ((int)(phase_values[PAWN]*16 + phase_values[KNIGHT]*4 + phase_values[BISHOP]*4 + phase_values[ROOK]*4 + phase_values[QUEEN]*2))


const eval_t piece_values[2][6] = { // [mg/eg][piece]
	{
		[PAWN] = 82,
		[KNIGHT] = 337,
		[BISHOP] = 365,
		[ROOK] = 477,
		[QUEEN] = 1025,
		[KING] = EVAL_MAX // Not really needed
	},
	{
		[PAWN] = 94,
		[KNIGHT] = 281,
		[BISHOP] = 297,
		[ROOK] = 512,
		[QUEEN] = 936,
		[KING] = EVAL_MAX // Not really needed
	}
};

// for usage in get_cheapest_piece()
const unsigned int pieces_ordered_by_cheapness[N_PIECES] = {
	PAWN,
	KNIGHT,
	BISHOP,
	ROOK,
	QUEEN,
	KING
};

#define GET_PIECE_VALUE(piece, phase) ((int)((phase * piece_values[0][piece] + (PHASE_TOTAL - phase) * piece_values[1][piece]) / PHASE_TOTAL))


/*
 * Private functions.
 */

// evaluation functions
//eval_t eval_material(const board_s* board, const int phase); // public
eval_t eval_stacked_pawns(const board_s* board);
eval_t eval_psqt(const board_s* board, const int phase);
eval_t eval_rook_open_file(const board_s* board);
eval_t eval_movable_squares(const board_s* board);
eval_t eval_king_guard(const board_s* board);
eval_t eval_castling_rights(const board_s* board);
eval_t eval_tempo_bonus(const board_s* board);

//int get_game_phase_value(const board_s* board); // public




inline eval_t better_eval(eval_t a, eval_t b, unsigned int side) {
	return (is_eval_better(a, b, side) ? a : b);
}

inline bool is_eval_better(eval_t a, eval_t b, unsigned int side) {
	return ((side == WHITE) ? a > b : a < b);
}

inline eval_t eval(const board_s* board) {

	// TODO: Incrementally updated?
	const int phase = MIN(get_game_phase_value(board), PHASE_TOTAL);

	eval_t eval = 0;

	eval += eval_material(board, phase);
	eval += eval_stacked_pawns(board);
	eval += eval_psqt(board, phase);
	eval += eval_rook_open_file(board);
	//eval += eval_movable_squares(board);
	//eval += eval_king_guard(board);
	eval += eval_castling_rights(board);
	//eval += eval_tempo_bonus(board);

	return eval;
	
	// NOTE: INTRERESTING this evaluation function gives coherent moves
	//return (10 / ((rand() % 20)+1)) * ((rand() % 2 == 1) ? -1 : 1);
}


// Evaluation function definitions
// TODO: Rewrite this function
eval_t eval_material(const board_s* board, const int phase) {
	eval_t res = 0;

	// Compare pawns
	const int n_pawns_w = popcount(board->pieces[WHITE][PAWN]);
	const int n_pawns_b = popcount(board->pieces[BLACK][PAWN]);
	res += (n_pawns_w - n_pawns_b) * GET_PIECE_VALUE(PAWN, phase);

	// Compare knights
	//unsigned int n_pawns_w = popcount(board->pieces[WHITE][PAWN]);
	//unsigned int n_pawns_b = popcount(board->pieces[BLACK][PAWN]);
	//res += (float)(n_pawns_w - n_pawns_b);
	 
	const int n_knights_w = popcount(board->pieces[WHITE][KNIGHT]);
	const int n_knights_b = popcount(board->pieces[BLACK][KNIGHT]);
	res += (n_knights_w - n_knights_b) * GET_PIECE_VALUE(KNIGHT, phase);

	// Compare bishops
	const int n_bishops_w = popcount(board->pieces[WHITE][BISHOP]);
	const int n_bishops_b = popcount(board->pieces[BLACK][BISHOP]);
	res += (n_bishops_w - n_bishops_b) * GET_PIECE_VALUE(BISHOP, phase);
	// Add in bishop pairs (half a pawn more)
	if (n_bishops_w > 1)
		res += EVAL_BPAIR_VALUE;
	if (n_bishops_b > 1)
		res += -EVAL_BPAIR_VALUE;

	// Compare rooks
	const int n_rooks_w = popcount(board->pieces[WHITE][ROOK]);
	const int n_rooks_b = popcount(board->pieces[BLACK][ROOK]);
	res += (n_rooks_w - n_rooks_b) * GET_PIECE_VALUE(ROOK, phase);

	// Compare queens
	const int n_queens_w = popcount(board->pieces[WHITE][QUEEN]);
	const int n_queens_b = popcount(board->pieces[BLACK][QUEEN]);
	res += (n_queens_w - n_queens_b) * GET_PIECE_VALUE(QUEEN, phase);

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

eval_t eval_psqt(const board_s* board, const int phase) {
	eval_t res = 0;


	int mg[2] = {0, 0};
	int eg[2] = {0, 0};

	for (unsigned int i = 0; i < N_PIECES; i++) {
		BitBoard pieces_w = board->pieces[WHITE][i];
		while (pieces_w) {
			const unsigned int piece = pop_bit(&pieces_w);
			mg[WHITE] += psqt[i][0][FLIP_PSQT_SQ(piece)];
			eg[WHITE] += psqt[i][1][FLIP_PSQT_SQ(piece)];
		}
		
		BitBoard pieces_b = board->pieces[BLACK][i];
		while (pieces_b) {
			const unsigned int piece = pop_bit(&pieces_b);
			mg[BLACK] += psqt[i][0][piece];
			eg[BLACK] += psqt[i][1][piece];
		}
	}

	int mg_score = mg[WHITE] - mg[BLACK];
	int eg_score = eg[WHITE] - eg[BLACK];

	return (mg_score * phase + eg_score * (PHASE_TOTAL - phase)) / PHASE_TOTAL;


	
	/*
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
	*/

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
		movelist_s moves = get_pseudo_legal_squares(board, piece, false);
		res += moves.n * EVAL_MOVABLE_SQUARES_MULT;
		if (moves.n)
			free(moves.moves);
	}

	BitBoard b_pieces = board->all_pieces[BLACK];
	while (b_pieces) {
		const BitBoard piece = pop_bitboard(&b_pieces);
		movelist_s moves = get_pseudo_legal_squares(board, piece, false);
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


// FIXME: Make computer-side tracking. Function can't work otherwise
// https://www.chessprogramming.org/Tempo
eval_t eval_tempo_bonus(const board_s* board) {
	if (board->sidetomove == WHITE)
		return EVAL_TEMPO_BONUS;
	return -EVAL_TEMPO_BONUS;
}


// 0 = start, PHASE_TOTAL = end
int get_game_phase_value(const board_s* board) {
	int phase = PHASE_TOTAL;

	phase -= popcount(board->pieces[WHITE][PAWN]) * phase_values[PAWN];
	phase -= popcount(board->pieces[WHITE][KNIGHT]) * phase_values[KNIGHT];
	phase -= popcount(board->pieces[WHITE][BISHOP]) * phase_values[BISHOP];
	phase -= popcount(board->pieces[WHITE][ROOK]) * phase_values[ROOK];
	phase -= popcount(board->pieces[WHITE][QUEEN]) * phase_values[QUEEN];
	phase -= popcount(board->pieces[BLACK][PAWN]) * phase_values[PAWN];
	phase -= popcount(board->pieces[BLACK][KNIGHT]) * phase_values[KNIGHT];
	phase -= popcount(board->pieces[BLACK][BISHOP]) * phase_values[BISHOP];
	phase -= popcount(board->pieces[BLACK][ROOK]) * phase_values[ROOK];
	phase -= popcount(board->pieces[BLACK][QUEEN]) * phase_values[QUEEN];

	assert(phase <= PHASE_TOTAL);

	return phase;
}




void set_move_predict_scores(const board_s* restrict board, move_s* restrict move) {
	move->move_score = 0;

	const int phase = get_game_phase_value(board);

	// check if opponent is in check
	// this works becouse opponent could not have already been in check before this move.
	//if (is_in_check(board, OPPOSITE_SIDE(move->side)))
	//	score += MV_SCORE_CHECK;

	//score += (move->flags & FLAG_PROMOTE) * MV_SCORE_CHECK;

	if (move->flags & FLAG_PROMOTE)
		move->move_score += eval_material_value[move->promoteto] - eval_material_value[PAWN];

	// TODO: Can be further improved by removing attackers and doing same again (discovered attacks)
	if (move->flags & FLAG_CAPTURE) {
		const eval_t move_see = see(board, move);//score += eval_material_value[move->piece_captured];
		move->move_see = move_see;
		move->move_score += move_see;
		if (move_see > 0)
			move->move_score += 20;
	}

	/*
	else {
		//score -= eval_material_value[move->fromtype]/MV_SCORE_CAPTURER_VALUE_DIVIDE;

		BitBoard attackers = get_attackers(board, move->to, move->side, 0); // & ~(move->from); // movers pieces
		BitBoard defenders = get_attackers(board, move->to, OPPOSITE_SIDE(move->side), 0) & ~(move->from);

		int n_attackers = popcount(attackers);
		int n_defenders = popcount(defenders);

		if (!n_attackers)
			goto GET_MOVE_PREDICT_SCORE_ATTACK_DEFEND;

		if (!n_defenders) {
			score -= eval_material_value[move->fromtype]; //GET_PIECE_VALUE(move->fromtype, phase);
			goto GET_MOVE_PREDICT_SCORE_ATTACK_DEFEND;
		}

		while (attackers)
			score -= eval_material_value[get_piece_type(board, move->side, pop_bitboard(&attackers))]; //GET_PIECE_VALUE(get_piece_type(board, move->side, pop_bitboard(&attackers)), phase);

		while (defenders)
			score += eval_material_value[get_piece_type(board, OPPOSITE_SIDE(move->side), pop_bitboard(&defenders))];
	}
	*/
	
	//score += GET_PIECE_VALUE(get_piece_type(board, OPPOSITE_SIDE(move->side), pop_bitboard(&defenders)), phase);
	//}
	//GET_MOVE_PREDICT_SCORE_ATTACK_DEFEND:


	//  if (!(move->flags & FLAG_CAPTURE) && !(move->flags & (FLAG_KCASTLE | FLAG_QCASTLE)))
	//  	move->move_score += mv_move_weight[move->fromtype];
	
	// if (move->flags & FLAG_KCASTLE)
	// 	move->move_score += MV_SCORE_KCASTLE;
	// else if (move->flags & FLAG_QCASTLE)
	// 	move->move_score += MV_SCORE_QCASTLE;
	
	
	//if (psqt_values[move->fromtype] != NULL) {
	unsigned int from_sq = lowest_bitindex(move->from);
	unsigned int to_sq = lowest_bitindex(move->to);

	if (move->side == WHITE) {
	 	from_sq = FLIP_PSQT_SQ(from_sq);
	 	to_sq = FLIP_PSQT_SQ(to_sq);
	}

	if (move->flags & FLAG_PROMOTE) {
		//if (psqt_values[move->promoteto]) {
		move->move_score -= (psqt[move->fromtype][0][from_sq]*(phase) + (PHASE_TOTAL-phase)*psqt[move->fromtype][0][from_sq])/PHASE_TOTAL; // FIXME: Taper this shit
		move->move_score += (psqt[move->promoteto][0][to_sq]*(phase) + (PHASE_TOTAL-phase)*psqt[move->promoteto][0][to_sq])/PHASE_TOTAL;
		//}
	}
	else {
		move->move_score -= (psqt[move->fromtype][0][from_sq]*(phase) + (PHASE_TOTAL-phase)*psqt[move->fromtype][0][from_sq])/PHASE_TOTAL;
		move->move_score += (psqt[move->fromtype][0][to_sq]*(phase) + (PHASE_TOTAL-phase)*psqt[move->fromtype][0][to_sq])/PHASE_TOTAL;
	}
	//}
	


	//return score;
}


BitBoard get_cheapest_piece(const board_s* board, unsigned int side, BitBoard select_mask) {
	assert(side < 2);
	
	const BitBoard pieces = board->all_pieces[side] & select_mask;

	for (int i = 0; i < N_PIECES; i++) {
		const BitBoard current_selected_pieces = board->pieces[side][pieces_ordered_by_cheapness[i]] & pieces;
		// TODO: Order by tapered psqt and piece eval
		if (current_selected_pieces)
			return lowest_bitboard(current_selected_pieces);
	}

	return 0x0;
}



eval_t see(const board_s* restrict board, const move_s* restrict move) {
	// https://www.chessprogramming.org/SEE_-_The_Swap_Algorithm

	assert(move->flags & FLAG_CAPTURE);

	const int phase = get_game_phase_value(board);

	eval_t gain[32];
	int d = 0;
	
	// pieces that have done a capture get added here
	// so that they may be ignored by get_seeing_pieces()
	BitBoard removed_pieces = move->to;

	BitBoard attackdef = get_seeing_pieces(board, move->to, removed_pieces);

	unsigned int current_side = move->side;
	BitBoard current_attacker = move->from; //get_cheapest_piece(board, current_side, attackdef);

	gain[0] = GET_PIECE_VALUE(move->piece_captured, phase);


	// if (!current_attacker)
	// 	return gain[0];

	do {
		d++;
		gain[d] = GET_PIECE_VALUE(get_piece_type(board, current_side, current_attacker), phase) - gain[d-1];

		if (MAX(-gain[d-1], gain[d]) < 0) break;

		removed_pieces |= current_attacker;
		// HACK: Suboptimal.
		attackdef = get_seeing_pieces(board, move->to, removed_pieces);
		current_side = OPPOSITE_SIDE(current_side);
		current_attacker = get_cheapest_piece(board, current_side, attackdef);

	} while (current_attacker);

	while (--d)
		gain[d-1] = -MAX(-gain[d-1], gain[d]);

	return gain[0];
}
