#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <math.h>

#include "search.h"

#include "movegen.h"
#include "bitboard.h"
#include "board.h"
#include "eval.h"
#include "uci.h"
#include "algebraic.h"

#include "defs.h"



// Private functions
eval_t regular_search(board_s* restrict board, move_s* restrict bestmove, search_stats_s* restrict stats, const unsigned int depth, eval_t alpha, eval_t beta);



eval_t uci_think(const uci_s* uci, board_s* board, move_s* bestmove) {
	assert(uci->action != UCI_IDLE);

	//if (uci->action == UCI_PONDER)
	//	goto THINK_PONDER;
	
	// Normal search (alpha is maximizing and beta minimizing)
	return regular_search(board, bestmove, NULL, 9, EVAL_MIN, EVAL_MAX);

	//THINK_PONDER:
	// TODO
	//fputs("Pondering not supported!", stderr);

	//return NAN;
}


eval_t search_with_stats(board_s* restrict board, move_s* restrict bestmove, const unsigned int depth, search_stats_s* restrict stats) {
	memset(stats, 0, sizeof (search_stats_s));


	stats->n_plies = depth;

	// +1 becouse we start at 0 and end in depth, not depth-1
	// n_positions[0] should always be 1 becouse there is only 1 root node (duh!)
	stats->n_positions = calloc(depth+1, sizeof (unsigned long long));
	stats->fail_hard_cutoffs = calloc(depth+1, sizeof (unsigned long long));

	return regular_search(board, bestmove, stats, depth, EVAL_MIN, EVAL_MAX);
}



eval_t regular_search(board_s* restrict board, move_s* restrict bestmove, search_stats_s* restrict stats, const unsigned int depth, eval_t alpha, eval_t beta) {
	if (stats) {
		stats->n_positions[stats->n_plies - depth]++;
		stats->nodes++;
	}
	
	if (depth == 0)
		return eval(board);

	const unsigned int initial_side = board->sidetomove;

	const bool initially_in_check = is_in_check(board, board->sidetomove);

	// for checking if there were any legal moves in this position for board->sidetomove
	unsigned int n_legal_moves_done = 0;

	// Stores the best move for board->sidetomove
	eval_t bestmove_eval = (board->sidetomove == WHITE ? EVAL_MIN : EVAL_MAX); // set to worst possible

	BitBoard pieces_copy = board->all_pieces[board->sidetomove];
	unsigned int n_pieces = popcount(pieces_copy);

	// for checking if position is a checkmate
	unsigned int skipped_because_of_checks = 0;

	board_s boardcopy;
	memcpy(&boardcopy, board, sizeof (board_s));

	movelist_s* all_moves = calloc(n_pieces, sizeof (movelist_s));
	unsigned int n_all_moves = 0;

	// Go through every piece and create the moves
	for (size_t i = 0; i < n_pieces; i++) {
		all_moves[i] = get_pseudo_legal_squares(board, pop_bitboard(&pieces_copy));
		
		n_all_moves += all_moves[i].n;
	}
	
	if (stats)
		stats->n_moves_generated += n_all_moves;

	eval_t last_best_move = EVAL_MAX;
	// so that we don't calculate the same move twice
	unsigned int last_piece_index = UINT_MAX;
	unsigned int last_move_index = UINT_MAX;

	// Go through every move
	for (unsigned int i = 0; i < n_all_moves; i++) {
		move_s* move = NULL;

		unsigned int piece_index = UINT_MAX;
		unsigned int move_index = UINT_MAX;

		// get the next best move from all of the moves
		for (unsigned int j = 0; j < n_pieces; j++) {
			for (unsigned int k = 0; k < all_moves[j].n; k++) {
				// FIXME: NIGHTMARE NIGHTMARE NIGHTMARE NIGHTMARE NIGHTMARE NIGHTMARE NIGHTMARE NIGHTMARE NIGHTMARE NIGHTMARE
				if (move == NULL) {
					if (all_moves[j].moves[k].move_score <= last_best_move && last_piece_index != j && last_move_index != k) {
						move = all_moves[j].moves + k;
						piece_index = j;
					}
				}
				else if (all_moves[j].moves[k].move_score <= last_best_move && all_moves[j].moves[k].move_score > move->move_score && last_piece_index != j && last_move_index != k) {
					move = all_moves[j].moves + k;
					move_index = i;
				}
			}
		}

		last_best_move = move->move_score;
		last_piece_index = piece_index;
		last_move_index = move_index;


		// --- CHECK FOR LEGALITY OF MOVE ---

		// Check if castling is valid
		if (move->flags & (FLAG_KCASTLE | FLAG_QCASTLE)) {
			if (initially_in_check)
				continue; // can not castle while in check
			
			BitBoard between_rk;
			if (board->sidetomove == WHITE)
				between_rk = (move->flags & FLAG_KCASTLE ? WK_CAST_CLEAR_MASK : WQ_CAST_CLEAR_MASK);
			else
				between_rk = (move->flags & FLAG_KCASTLE ? BK_CAST_CLEAR_MASK : BQ_CAST_CLEAR_MASK);
			
			//FIXME: This shit is slow as fuck. Make those attack maps pls.
			while (between_rk) {
				if (is_side_attacking_sq(board, pop_bitboard(&between_rk), OPPOSITE_SIDE(board->sidetomove)))
					goto REGULAR_SEARCH_SKIP_MOVE;
			}
		}
		
		makemove(board, move);

		// check if that side got itself in check (or couldn't get out of one)
		if (is_in_check(board, initial_side)) {
			skipped_because_of_checks++;
			goto REGULAR_SEARCH_SKIP_MOVE;
		}

		n_legal_moves_done++;

		// --- MOVE WAS LEGAL AND IS DONE ---

		append_to_move_history(board, move);

		eval_t eval = regular_search(board, NULL, stats, depth-1, alpha, beta);
		//printf("info string %f\n", eval);

		// if move was better, store it instead
		if (is_eval_better(eval, bestmove_eval, initial_side)) {
			bestmove_eval = eval;
			if (bestmove)
				memcpy(bestmove, move, sizeof (move_s));
		}


		// alpha is maximizing and beta is minimizing
		if (initial_side == WHITE)
			alpha = better_eval(alpha, eval, WHITE);
		else
			beta = better_eval(beta, eval, BLACK);
		
		if (beta <= alpha) {
			if (stats)
				stats->fail_hard_cutoffs[stats->n_plies - depth]++;
			goto REGULAR_SEARCH_BREAK_SEARCH; // fail-hard cutoff (right term?)
		}


		REGULAR_SEARCH_SKIP_MOVE:
		restore_board(board, &boardcopy);
	}

	REGULAR_SEARCH_BREAK_SEARCH:

	for (unsigned int i = 0; i < n_pieces; i++) {
		if (all_moves[i].n)
			free(all_moves[i].moves);
	}
	free(all_moves);


	// No moves were made??
	if (!n_legal_moves_done) {
		if (initially_in_check) { // is a checkmate (was in check and can't get out of it)
			// store the worst value possible for checkmate
			return (is_eval_better(EVAL_MAX, EVAL_MIN, board->sidetomove) ? EVAL_MIN : EVAL_MAX);
		}
		else { // is a stalemate (wasn't in check and no legal moves)
			return 0;
		}
	}
	
	return bestmove_eval;
}

