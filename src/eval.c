#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>
#include <string.h>

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
	[KING] = EVAL_MAX>>1 // not really needed as king is never needed in any eval
};

const eval_t mv_move_weight[N_PIECES] = {
	[PAWN] = MV_SCORE_MOVE_WEIGHT_PAWN,
	[KNIGHT] = MV_SCORE_MOVE_WEIGHT_KNIGHT,
	[BISHOP] = MV_SCORE_MOVE_WEIGHT_BISHOP,
	[ROOK] = MV_SCORE_MOVE_WEIGHT_ROOK,
	[QUEEN] = MV_SCORE_MOVE_WEIGHT_QUEEN,
	[KING] = MV_SCORE_MOVE_WEIGHT_KING
};


// King is in the 42 index
eval_t psqt_king_guard_pieces[64] = {
	 0,   0,   0,   0,   0,   0,   0,   0,
	 0,   0,   0,   0,   0,   0,   0,   0,
	 0,   0,   0,   0,   0,   0,   0,   0,
	 4,   0,  12,   0,   1,   0,   0,   0,
	 1,  15,  17,  14,   2,   0,   0,   0,
	15,  16,   0,  14,   9,   0,   0,   0,
	 0,  16,  14,  13,   0,   0,   0,   0,
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
		[KING] = EVAL_MAX>>1 // Not really needed
	},
	{
		[PAWN] = 94,
		[KNIGHT] = 281,
		[BISHOP] = 297,
		[ROOK] = 512,
		[QUEEN] = 936,
		[KING] = EVAL_MAX>>1 // Not really needed
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

#define GET_PIECE_VALUE(piece, phase) ((int)(((PHASE_TOTAL - phase) * piece_values[0][piece] + phase * piece_values[1][piece]) / PHASE_TOTAL))

// From: https://www.chessprogramming.org/King_Safety#Attack_Units
static const eval_t safety_table[100] = {
	  0,   0,   1,   2,   3,   5,   7,   9,  12,  15,
	 18,  22,  26,  30,  35,  39,  44,  50,  56,  62,
	 68,  75,  82,  85,  89,  97, 105, 113, 122, 131,
	140, 150, 169, 180, 191, 202, 213, 225, 237, 248,
	260, 272, 283, 295, 307, 319, 330, 342, 354, 366,
	377, 389, 401, 412, 424, 436, 448, 459, 471, 483,
	494, 500, 500, 500, 500, 500, 500, 500, 500, 500,
	500, 500, 500, 500, 500, 500, 500, 500, 500, 500,
	500, 500, 500, 500, 500, 500, 500, 500, 500, 500,
	500, 500, 500, 500, 500, 500, 500, 500, 500, 500
};

static const int king_attack_value[N_PIECES] = {
	[KING] = 1,
	[QUEEN] = 4, // 5
	[ROOK] = 2,
	[BISHOP] = 2,
	[KNIGHT] = 1, // 2
	[PAWN] = 0
};


/*
 * Private structs
 */
// TODO: Make a struct wherein stuff calculated during different eval stages will be collected to

typedef struct {

	int phase;

	// piece moves are calculated here
	// moves on own pieces are stored too.
	BitBoard moves[2][16];

	// pointer to a bitboard with the pieces movable squares or NULL if no piece
	BitBoard* piece_moves[64];

	// Number of each piece type per side
	unsigned int material_amount[2][N_PIECES];

	// Stores the value of attacks on opponent king
	unsigned int king_attacks[2];
	unsigned int n_king_attackers[2]; // number of -

} eval_data_s;



/*
 * Private functions.
 */

static void calculate_eval_data(const board_s* restrict board, eval_data_s* restrict data);

// evaluation functions
//eval_t eval_material(const board_s* board, const int phase); // public
static eval_t eval_stacked_pawns(const board_s* board);
static eval_t eval_psqt(const board_s* board, const int phase);
static eval_t eval_rook_open_file(const board_s* board);
static eval_t eval_movable_squares(const board_s* board);
static eval_t eval_pawn_structure(const board_s* board);
static eval_t eval_passed_pawn(const board_s* board, const int phase);
static eval_t eval_pawn_shield(const board_s* board);
static eval_t eval_king_guard(const board_s* board);
static eval_t eval_king_safety(const board_s* restrict board, eval_data_s* restrict eval_data);
static eval_t eval_knight_outpost(const board_s* board, int phase);
static eval_t eval_hanging_pieces(const board_s* restrict board, eval_data_s* restrict eval_data);


//int get_game_phase_value(const board_s* board); // public




inline eval_t better_eval(eval_t a, eval_t b, unsigned int side) {
	return (is_eval_better(a, b, side) ? a : b);
}

inline bool is_eval_better(eval_t a, eval_t b, unsigned int side) {
	return ((side == WHITE) ? a > b : a < b);
}

eval_t eval(const board_s* board) {

	//eval_data_s eval_data = {0};
	//memset(&eval_data, 0, sizeof (eval_data_s));

	//calculate_eval_data(board, &eval_data);

	// TODO: Incrementally updated?
	const int phase = MIN(get_game_phase_value(board), PHASE_TOTAL);

	eval_t eval = 0;

	eval += eval_material(board, phase);
	eval += eval_stacked_pawns(board);
	eval += eval_psqt(board, phase);
	eval += eval_rook_open_file(board);
	eval += eval_movable_squares(board);
	eval += eval_pawn_structure(board);
	eval += eval_passed_pawn(board, phase);
	//eval += eval_king_guard(board);
	//eval += eval_king_safety(board, &eval_data);
	eval += eval_knight_outpost(board, phase);

	if (eval == 0) {
		if (board->hash % 2)
			eval += 1;
		else
			eval -= 1;
	}

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
	 
	const int n_knights_w = popcount(board->pieces[WHITE][KNIGHT]);
	const int n_knights_b = popcount(board->pieces[BLACK][KNIGHT]);
	res += (n_knights_w - n_knights_b) * GET_PIECE_VALUE(KNIGHT, phase);

	// Compare bishops
	const int n_bishops_w = popcount(board->pieces[WHITE][BISHOP]);
	const int n_bishops_b = popcount(board->pieces[BLACK][BISHOP]);
	res += (n_bishops_w - n_bishops_b) * GET_PIECE_VALUE(BISHOP, phase);
	// Add in bishop pairs
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

	// Material imbalance is more accentuated when material is low
	// This makes the overall playing better but is bad for move ordering
	if (res > 0)
		res += (32 - popcount(board->every_piece)) * EVAL_MATERIAL_IMBALANCE_ACCENTUATE_MULT; // 32 is starting n of pieces
	else if (res < 0)
		res -= (32 - popcount(board->every_piece)) * EVAL_MATERIAL_IMBALANCE_ACCENTUATE_MULT;

	return res;
}


static void calculate_eval_data(const board_s* restrict board, eval_data_s* restrict data) {
	// TODO: Do this in this function
	data->phase = get_game_phase_value(board);


	for (int side = 0; side < 2; side++) {

		unsigned int piece_index = 0; // Holds current piece index of the piece for data->moves
		
		for (int piece = 0; piece < N_PIECES; piece++) {
			// Store material amount
			data->material_amount[side][piece] = popcount(board->pieces[side][piece]);

			// Store moves for these pieces
			BitBoard pieces_copy = board->pieces[side][piece];
			while (pieces_copy) {
				BitBoard current_piece = pop_bitboard(&pieces_copy);
				BitBoard moves = get_pseudo_legal_squares(board, side, piece, current_piece, false);
				data->moves[side][piece_index] = moves;
				data->piece_moves[lowest_bitindex(current_piece)] = &(data->moves[side][piece_index]);
				piece_index++;

				
				BitBoard king_attacks = moves & piecelookup(lowest_bitindex(board->pieces[OPPOSITE_SIDE(side)][KING]), KING, 0);
				data->king_attacks[side] += king_attack_value[piece] * popcount(king_attacks);
				if (king_attacks) data->n_king_attackers[side]++;
			}
		}
	}
}


static eval_t eval_stacked_pawns(const board_s* board) {
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

static eval_t eval_psqt(const board_s* board, const int phase) {

	int mg[2] = {0, 0};
	int eg[2] = {0, 0};

	for (unsigned int i = 0; i < N_PIECES; i++) {
		BitBoard pieces_w = flip_vertical(board->pieces[WHITE][i]);
		while (pieces_w) {
			const unsigned int piece = pop_bit(&pieces_w);
			//mg[WHITE] += psqt[i][0][FLIP_PSQT_SQ(piece)];
			//eg[WHITE] += psqt[i][1][FLIP_PSQT_SQ(piece)];
			mg[WHITE] += psqt[i][0][piece];
			eg[WHITE] += psqt[i][1][piece];
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

	return (mg_score * (PHASE_TOTAL - phase) + eg_score * phase) / PHASE_TOTAL;
}

static eval_t eval_rook_open_file(const board_s* board) {
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


static const unsigned int movable_squares_types [] = {
	//KING,
	QUEEN,
	ROOK,
	BISHOP,
	KNIGHT,
	//PAWN
};
// TODO: Make those fucking attack maps
// FIXME: you lazy fuck
static eval_t eval_movable_squares(const board_s* board) {
	eval_t res = 0;

	// movelist_s moves;
	// move_s move[30];


	for (unsigned int i = 0; i < LENGTH(movable_squares_types); i++) {
		BitBoard w_pieces = board->pieces[WHITE][movable_squares_types[i]];//board->all_pieces[WHITE] & ~(board->pieces[WHITE][KING]);
		while (w_pieces) {
			const BitBoard piece = pop_bitboard(&w_pieces);
			const BitBoard movable_squares = get_pseudo_legal_squares(board, WHITE, movable_squares_types[i], piece, true);
			// Pawn mobility matters less than everything else
			// if (piece & board->pieces[WHITE][PAWN])
			// 	res += popcount(movable_squares) * EVAL_MOVABLE_SQUARES_MULT / 2;
			// else
			// 	res += popcount(movable_squares) * EVAL_MOVABLE_SQUARES_MULT;
			res += popcount(movable_squares) * EVAL_MOVABLE_SQUARES_MULT;
			// TODO: Do pawns on their own
			// if (moves.n)
			// 	free(moves.moves);
		}

		BitBoard b_pieces = board->pieces[BLACK][movable_squares_types[i]];//board->all_pieces[BLACK] & ~(board->pieces[BLACK][KING]);
		while (b_pieces) {
			const BitBoard piece = pop_bitboard(&b_pieces);
			const BitBoard movable_squares = get_pseudo_legal_squares(board, BLACK, movable_squares_types[i], piece, true);
			// Pawn mobility matters less than everything else
			// if (piece & board->pieces[BLACK][PAWN])
			// 	res -= popcount(movable_squares) * EVAL_MOVABLE_SQUARES_MULT / 2;
			// else
			// 	res -= popcount(movable_squares) * EVAL_MOVABLE_SQUARES_MULT;
			res -= popcount(movable_squares) * EVAL_MOVABLE_SQUARES_MULT;
			// if (moves.n)
			// 	free(moves.moves);
		}
	}

	return res;
}


static eval_t eval_pawn_structure(const board_s* board) {
	eval_t res = 0;

	res += popcount(board->pieces[WHITE][PAWN] & MV_NE((board->pieces[WHITE][PAWN] & ~RIGHT_MASK), 1))*4;
	res += popcount(board->pieces[WHITE][PAWN] & MV_NW((board->pieces[WHITE][PAWN] & ~LEFT_MASK), 1))*4;
	res -= popcount(board->pieces[BLACK][PAWN] & MV_SE((board->pieces[BLACK][PAWN] & ~RIGHT_MASK), 1))*4;
	res -= popcount(board->pieces[BLACK][PAWN] & MV_SW((board->pieces[BLACK][PAWN] & ~LEFT_MASK), 1))*4;

	BitBoard pawns_w_copy = board->pieces[WHITE][PAWN];
	while (pawns_w_copy) {
		const BitBoard current_piece = pop_bitboard(&pawns_w_copy);
		const unsigned int current_piece_index = lowest_bitindex(current_piece);

		if (!(current_piece & LEFT_MASK)) {
			if (columnlookup(current_piece_index + DIRECTION_W) & board->pieces[WHITE][PAWN])
				res += 3;
		}
		if (!(current_piece & RIGHT_MASK)) {
			if (columnlookup(current_piece_index + DIRECTION_E) & board->pieces[WHITE][PAWN])
				res += 3;
		}
	}
	BitBoard pawns_b_copy = board->pieces[BLACK][PAWN];
	while (pawns_b_copy) {
		const BitBoard current_piece = pop_bitboard(&pawns_b_copy);
		const unsigned int current_piece_index = lowest_bitindex(current_piece);

		if (!(current_piece & LEFT_MASK)) {
			if (columnlookup(current_piece_index + DIRECTION_W) & board->pieces[BLACK][PAWN])
				res -= 3;
		}
		if (!(current_piece & RIGHT_MASK)) {
			if (columnlookup(current_piece_index + DIRECTION_E) & board->pieces[BLACK][PAWN])
				res -= 3;
		}
	}

	return res;
}


static eval_t eval_passed_pawn(const board_s* board, const int phase) {
	eval_t res = 0;

	const eval_t passed_pawn_value = (15 * (PHASE_TOTAL - phase) + 30 * phase) / PHASE_TOTAL;

	BitBoard pawns_w_copy = board->pieces[WHITE][PAWN];

	while (pawns_w_copy) {
		const BitBoard current_piece = pop_bitboard(&pawns_w_copy);
		const unsigned int current_piece_index = lowest_bitindex(current_piece);

		if (passed_pawn_opponent_mask[current_piece_index] & board->pieces[BLACK][PAWN])
			continue;
		else if (popcount(board->pieces[WHITE][PAWN] & columnlookup(current_piece_index)) > 1) {
			// check if this pawn is the lower stacked pawn, then not considered a passed pawn
			if (lowest_bitboard(board->pieces[WHITE][PAWN] & columnlookup(current_piece_index)) == current_piece)
				continue;
		}

		res += passed_pawn_value;
	}

	BitBoard pawns_b_copy = board->pieces[BLACK][PAWN];

	while (pawns_b_copy) {
		const BitBoard current_piece = pop_bitboard(&pawns_b_copy);
		const unsigned int current_piece_index = lowest_bitindex(current_piece);

		if (flip_vertical(passed_pawn_opponent_mask[lowest_bitindex(flip_vertical(current_piece))]) & board->pieces[WHITE][PAWN])
			continue;
		else if (popcount(board->pieces[BLACK][PAWN] & columnlookup(current_piece_index)) > 1) {
			// check if this pawn is the lower stacked pawn, then not considered a passed pawn
			if (highest_bitindex(board->pieces[BLACK][PAWN] & columnlookup(current_piece_index)) == current_piece_index)
				continue;
		}

		res -= passed_pawn_value;
	}

	return res;
}


static eval_t eval_pawn_shield(const board_s* board) {

}


static eval_t eval_king_guard(const board_s* board) {
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


static eval_t eval_king_safety(const board_s* restrict board, eval_data_s* restrict eval_data) {
	eval_t res = 0;

	res += safety_table[MIN(eval_data->king_attacks[WHITE] * (eval_data->n_king_attackers[WHITE] > 2 && board->pieces[WHITE][QUEEN]), 99)];
	res -= safety_table[MIN(eval_data->king_attacks[BLACK] * (eval_data->n_king_attackers[BLACK] > 2 && board->pieces[BLACK][QUEEN]), 99)];

	return res;
}


static eval_t eval_knight_outpost(const board_s* board, int phase) {
	eval_t res = 0;

	const eval_t outpost_value = (12 * (PHASE_TOTAL - phase) + 3 * phase) / PHASE_TOTAL;

	BitBoard w_knights_copy = board->pieces[WHITE][KNIGHT] & TOP_HALF_MASK;
	while (w_knights_copy) {
		BitBoard piece = pop_bitboard(&w_knights_copy);
		res += outpost_value * ((piecelookup(lowest_bitindex(piece), PAWN, BLACK) & board->pieces[WHITE][PAWN]) > 0);
	}

	BitBoard b_knights_copy = board->pieces[BLACK][KNIGHT] & BOTTOM_HALF_MASK;
	while (b_knights_copy) {
		BitBoard piece = pop_bitboard(&b_knights_copy);
		res -= outpost_value * ((piecelookup(lowest_bitindex(piece), PAWN, WHITE) & board->pieces[BLACK][PAWN]) > 0);
	}

	return res;
}


static eval_t eval_hanging_pieces(const board_s* restrict board, eval_data_s* restrict eval_data) {
	eval_t res = 0;

	for (int side = 0; side < 2; side++) {

	}

	return res;
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
		else
			move->move_score += -move_see;
	}

	if (move->flags & FLAG_ENPASSANT) {
		move->move_score += 50;
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

	eval_t psqt_diff = 0;
	if (move->flags & FLAG_PROMOTE) {
		//if (psqt_values[move->promoteto]) {
		psqt_diff -= (psqt[move->fromtype][0][from_sq]*((PHASE_TOTAL-phase)) + phase*psqt[move->fromtype][1][from_sq])/PHASE_TOTAL; // FIXME: Taper this shit
		psqt_diff += (psqt[move->promoteto][0][to_sq]*((PHASE_TOTAL-phase)) + phase*psqt[move->promoteto][1][to_sq])/PHASE_TOTAL;
		//}
	}
	else {
		psqt_diff -= (psqt[move->fromtype][0][from_sq]*((PHASE_TOTAL-phase)) + phase*psqt[move->fromtype][1][from_sq])/PHASE_TOTAL;
		psqt_diff += (psqt[move->fromtype][0][to_sq]*((PHASE_TOTAL-phase)) + phase*psqt[move->fromtype][1][to_sq])/PHASE_TOTAL;
	}
	move->move_score += psqt_diff / 5;
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
