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
eval_t regular_search(board_s* restrict board, move_s* restrict bestmove, search_stats_s* restrict stats, const unsigned int search_depth, const int depth, const bool is_null_prune, eval_t alpha, eval_t beta);
eval_t q_search(board_s* restrict board, search_stats_s* restrict stats, const unsigned int search_depth, eval_t alpha, eval_t beta);


eval_t uci_think(const uci_s* uci, board_s* restrict board, move_s* restrict bestmove) {
	assert(uci->action != UCI_IDLE);

	//if (uci->action == UCI_PONDER)
	//	goto THINK_PONDER;
	
	// Normal search (alpha is maximizing and beta minimizing)
	return regular_search(board, bestmove, NULL, 6, 6, false, EVAL_MIN, EVAL_MAX);

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

	return regular_search(board, bestmove, stats, depth, depth, false, EVAL_MIN, EVAL_MAX);
}



eval_t regular_search(board_s* restrict board, move_s* restrict bestmove, search_stats_s* restrict stats, const unsigned int search_depth, const int depth, const bool is_null_prune, eval_t alpha, eval_t beta) {
	if (stats)
		stats->nodes++;
	
	if (stats && depth >= 0)
		stats->n_positions[stats->n_plies - depth]++;
	
	if (depth <= 0)
		return q_search(board, stats, search_depth, alpha, beta);

	const unsigned int initial_side = board->sidetomove;

	const bool initially_in_check = is_in_check(board, board->sidetomove);

	// for checking if there were any legal moves in this position for board->sidetomove
	unsigned int n_legal_moves_done = 0;

	// Stores the best move for board->sidetomove
	eval_t bestmove_eval = (board->sidetomove == WHITE ? EVAL_MIN : EVAL_MAX); // set to worst possible

	const unsigned int n_pieces = popcount(board->all_pieces[board->sidetomove]);

	// I think the right term is fail-low?? Not sure
	// This set to true if beta <= alpha
	bool fail_low = false;

	// for checking if position is a checkmate
	unsigned int skipped_because_of_checks = 0;

	//board_s boardcopy;
	//memcpy(&boardcopy, board, sizeof (board_s));

#ifndef NDEBUG
	BitBoard every_piece_copy = board->every_piece;
#endif // NDEBUG

	movelist_s* all_moves = calloc(n_pieces, sizeof (movelist_s));
	unsigned int n_all_moves = 0;
	
	// !!!!REMEMBER TO FREE!!!!
	// for tracking, which moves have already been selected (selection sort kinda)
	bool** move_visited = calloc(n_pieces, sizeof (bool*)); //[piece_index][move_index]
	

	bool do_null_move = false;
	if (depth >= NULL_MOVE_PRUNING_R + 1 && search_depth-depth > 0 && !initially_in_check && !is_null_prune)
		do_null_move = true;

	// Go through every piece and create the moves
	BitBoard pieces_copy = board->all_pieces[board->sidetomove];
	for (size_t i = 0; i < n_pieces; i++) {
		all_moves[i] = get_pseudo_legal_squares(board, pop_bitboard(&pieces_copy));
		
		n_all_moves += all_moves[i].n;

		if (all_moves[i].n)
			move_visited[i] = calloc(all_moves[i].n, sizeof (bool));
	}
	
	if (stats)
		stats->n_moves_generated += n_all_moves;

	eval_t last_best_move = EVAL_MAX;

	// Go through every move
	// +1 becouse of null move
	if (do_null_move)
		n_all_moves++;
	for (unsigned int i = 0; i < n_all_moves; i++) {
		move_s* move = NULL;

		
		// Do a null move
		if (do_null_move && i == 0 )
			goto REGULAR_SEARCH_SKIP_MOVE_SELECTION;

		unsigned int piece_index = UINT_MAX;
		unsigned int move_index = UINT_MAX;

		// get the next best move from all of the moves
		for (unsigned int j = 0; j < n_pieces; j++) {
			if (!all_moves[j].n)
				continue;
			for (unsigned int k = 0; k < all_moves[j].n; k++) {
				if (move_visited[j][k])
					continue;
				// FIXME: NIGHTMARE NIGHTMARE NIGHTMARE NIGHTMARE NIGHTMARE NIGHTMARE NIGHTMARE NIGHTMARE NIGHTMARE NIGHTMARE
				if (move == NULL) {
					if (all_moves[j].moves[k].move_score <= last_best_move) {
						move = all_moves[j].moves + k;
						piece_index = j;
						move_index = k;
					}
				}
				else if (all_moves[j].moves[k].move_score <= last_best_move && all_moves[j].moves[k].move_score > move->move_score) {
					move = all_moves[j].moves + k;
					piece_index = j;
					move_index = k;
				}
			}
		}

		assert(move);
		assert(piece_index != UINT_MAX);
		assert(move_index != UINT_MAX);

		// set move as visited
		move_visited[piece_index][move_index] = true;

		last_best_move = move->move_score;


		// --- CHECK FOR LEGALITY OF MOVE ---

		// Check if castling is valid
		if (move->flags & (FLAG_KCASTLE | FLAG_QCASTLE)) {
			if (initially_in_check)
				continue; // can not castle while in check
			
			BitBoard target_squares;
			if (board->sidetomove == WHITE)
				target_squares = (move->flags & FLAG_KCASTLE ? WK_CASTLE_ATTACK_MASK : WQ_CASTLE_ATTACK_MASK);
			else
				target_squares = (move->flags & FLAG_KCASTLE ? BK_CASTLE_ATTACK_MASK : BQ_CASTLE_ATTACK_MASK);
			
			//FIXME: This shit is slow as fuck. Make those attack maps pls.
			while (target_squares) {
				if (is_side_attacking_sq(board, pop_bitboard(&target_squares), OPPOSITE_SIDE(board->sidetomove)))
					goto REGULAR_SEARCH_SKIP_MOVE_PRE_MAKE;
			}
		}
		
		REGULAR_SEARCH_SKIP_MOVE_SELECTION:
		
		makemove(board, move);
		//append_to_move_history(board, move);

		// worst eval by default
		eval_t eval = better_eval(EVAL_MAX, EVAL_MIN, OPPOSITE_SIDE(initial_side));
		
		if (i == 0 && do_null_move) {
			if (initial_side == WHITE)
				eval = regular_search(board, NULL, stats, search_depth, depth - NULL_MOVE_PRUNING_R - 1, true, alpha - 1, alpha);
			else
				eval = regular_search(board, NULL, stats, search_depth, depth - NULL_MOVE_PRUNING_R - 1, true, beta - 1, beta);
		}
		else {
			// check if that side got itself in check (or couldn't get out of one)
			if (is_in_check(board, initial_side)) {
				skipped_because_of_checks++;
				goto REGULAR_SEARCH_SKIP_MOVE;
			}

			n_legal_moves_done++;

			// --- MOVE WAS LEGAL AND IS DONE ---


			eval = regular_search(board, NULL, stats, search_depth, depth-1, is_null_prune, alpha, beta);
			//printf("info string %f\n", eval);

			// if move was better, store it instead
			if (is_eval_better(eval, bestmove_eval, initial_side)) {
				bestmove_eval = eval;
				if (bestmove)
					memcpy(bestmove, move, sizeof (move_s));
			}
		}

		// alpha is maximizing and beta is minimizing
		if (initial_side == WHITE)
			alpha = better_eval(alpha, eval, WHITE);
		else
			beta = better_eval(beta, eval, BLACK);
		
		if (beta <= alpha) {
			fail_low = true;
			if (stats)
				stats->fail_hard_cutoffs[stats->n_plies - depth]++;
			unmakemove(board);
			break; // at least 1 legal move has to have been made so we can just break search
		}


		REGULAR_SEARCH_SKIP_MOVE:
		unmakemove(board);
		REGULAR_SEARCH_SKIP_MOVE_PRE_MAKE:
		continue;
	}

	// Free this stuff
	for (unsigned int i = 0; i < n_pieces; i++) {
		if (all_moves[i].n) {
			free(all_moves[i].moves);
			free(move_visited[i]);
		}
	}
	free(all_moves);
	free(move_visited);

	assert(board->every_piece == every_piece_copy);

	// alpha-beta pruning
	if (fail_low) {
		if (initial_side == WHITE)
			return alpha;
		else
			return beta;
	}

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


eval_t q_search(board_s* restrict board, search_stats_s* restrict stats, const unsigned int search_depth, eval_t alpha, eval_t beta) {
	
	// if (stats && depth >= 0)
	// 	stats->n_positions[stats->n_plies - depth]++;
	
	// if (depth <= 0)
	// 	return eval(board);

	const bool initially_in_check = is_in_check(board, board->sidetomove);

	
	
	/*
	const eval_t stand_pat = eval(board);
	
	eval_t big_delta = EVAL_QUEEN_MATERIAL_VALUE;

	if (promote_available(board, board->sidetomove))
		big_delta = EVAL_QUEEN_MATERIAL_VALUE - EVAL_PAWN_MATERIAL_VALUE;
	
	if (board->sidetomove == WHITE) {
		if (stand_pat < alpha - big_delta)
			return alpha;
		alpha = better_eval(alpha, stand_pat, board->sidetomove);
	}
	else {
		if (stand_pat > beta + big_delta)
			return beta;
		beta = better_eval(beta, stand_pat, board->sidetomove);
	}
	*/
	
	/*
	if (board->sidetomove == WHITE) {
		if (stand_pat >= beta)
			return beta;
		//alpha = MAX(stand_pat, beta); //better_eval(stand_pat, alpha, board->sidetomove);
	}
	else {
		if (stand_pat <= alpha)
			return alpha;
		//beta = MIN(stand_pat, beta); //better_eval(stand_pat, beta, board->sidetomove);
	}
	*/
	

	const unsigned int initial_side = board->sidetomove;

	// for checking if there were any legal moves in this position for board->sidetomove
	unsigned int n_legal_moves_done = 0;

	// Stores the best move for board->sidetomove
	eval_t bestmove_eval = (board->sidetomove == WHITE ? EVAL_MIN : EVAL_MAX); // set to worst possible

	// I think the right term is fail-low?? Not sure
	// This set to true if beta <= alpha
	bool fail_low = false;

	// N moves were found that don't fit the prune constraits
	unsigned int n_fail_q_prune = 0;

#ifndef NDEBUG
	BitBoard every_piece_copy = board->every_piece;
#endif // NDEBUG
	

	// bool do_null_move = false;
	// if (depth >= NULL_MOVE_PRUNING_R + 1 && search_depth-depth > 0 && !initially_in_check)// && !is_null_prune)
	// 	do_null_move = false; // null move pruning disabled in q-search
	
	const unsigned int n_pieces = popcount(board->all_pieces[board->sidetomove]);
	
	// !!!!REMEMBER TO FREE!!!!
	// for tracking, which moves have already been selected (selection sort kinda)
	bool** move_visited = calloc(n_pieces, sizeof (bool*)); //[piece_index][move_index]

	movelist_s* all_moves = calloc(n_pieces, sizeof (movelist_s));
	unsigned int n_all_moves = 0;

	// Go through every piece and create the moves
	BitBoard pieces_copy = board->all_pieces[board->sidetomove];
	for (size_t i = 0; i < n_pieces; i++) {
		all_moves[i] = get_pseudo_legal_squares(board, pop_bitboard(&pieces_copy));

		n_all_moves += all_moves[i].n;

		if (all_moves[i].n)
			move_visited[i] = calloc(all_moves[i].n, sizeof (bool));
	}
	
	if (stats)
		stats->n_moves_generated += n_all_moves;

	eval_t last_best_move = EVAL_MAX;

	/*
	// Go through every move
	// +1 becouse of null move
	if (do_null_move)
		n_all_moves++; */
	for (unsigned int i = 0; i < n_all_moves; i++) {
		move_s* move = NULL;
		
		// Do a null move
		// if (do_null_move && i == 0 )
		// 	goto REGULAR_SEARCH_SKIP_MOVE_SELECTION;

		unsigned int piece_index = UINT_MAX;
		unsigned int move_index = UINT_MAX;

		// get the next best move from all of the moves
		for (unsigned int j = 0; j < n_pieces; j++) {
			if (!all_moves[j].n)
				continue;
			for (unsigned int k = 0; k < all_moves[j].n; k++) {
				if (move_visited[j][k])
					continue;
				// FIXME: NIGHTMARE NIGHTMARE NIGHTMARE NIGHTMARE NIGHTMARE NIGHTMARE NIGHTMARE NIGHTMARE NIGHTMARE NIGHTMARE
				if (move == NULL) {
					if (all_moves[j].moves[k].move_score <= last_best_move) {
						move = all_moves[j].moves + k;
						piece_index = j;
						move_index = k;
					}
				}
				else if (all_moves[j].moves[k].move_score <= last_best_move && all_moves[j].moves[k].move_score > move->move_score) {
					move = all_moves[j].moves + k;
					piece_index = j;
					move_index = k;
				}
			}
		}

		assert(move);
		assert(piece_index != UINT_MAX);
		assert(move_index != UINT_MAX);

		// set move as visited
		move_visited[piece_index][move_index] = true;

		last_best_move = move->move_score;

		// --- CHECK FOR LEGALITY OF MOVE ---

		// Check if castling is valid
		if (move->flags & (FLAG_KCASTLE | FLAG_QCASTLE)) {
			if (initially_in_check)
				continue; // can not castle while in check
			
			BitBoard target_squares;
			if (board->sidetomove == WHITE)
				target_squares = (move->flags & FLAG_KCASTLE ? WK_CASTLE_ATTACK_MASK : WQ_CASTLE_ATTACK_MASK);
			else
				target_squares = (move->flags & FLAG_KCASTLE ? BK_CASTLE_ATTACK_MASK : BQ_CASTLE_ATTACK_MASK);
			
			//FIXME: This shit is slow as fuck. Make those attack maps pls.
			while (target_squares) {
				if (is_side_attacking_sq(board, pop_bitboard(&target_squares), OPPOSITE_SIDE(board->sidetomove)))
					goto REGULAR_SEARCH_SKIP_MOVE_PRE_MAKE;
			}
		}

		// Quiescence moving pre move-make

		bool failed_q_prune = true;

		if (move->flags & FLAG_CAPTURE) {
			eval_t move_see = see(board, move);

			if (move_see < 0)
				failed_q_prune = true;
			else if (board->sidetomove == WHITE) {
				if (eval_material(board) + move_see - EVAL_PAWN_MATERIAL_VALUE*1.5 > alpha) {
					failed_q_prune = false;
				}
			}
			else {
				if (eval_material(board) - move_see + EVAL_PAWN_MATERIAL_VALUE*1.5 < beta) {
					failed_q_prune = false;
				}
			}
		}
		else if (move->flags & FLAG_PROMOTE)
			failed_q_prune = false;
		else{
			//n_fail_q_prune++;
			failed_q_prune = true;
		}
		

		REGULAR_SEARCH_SKIP_MOVE_SELECTION:
		
		makemove(board, move);
		//append_to_move_history(board, move);


		// worst eval by default
		eval_t eval = better_eval(EVAL_MAX, EVAL_MIN, OPPOSITE_SIDE(initial_side));
		
		// check if that side got itself in check (or couldn't get out of one)
		if (is_in_check(board, initial_side))
			goto REGULAR_SEARCH_SKIP_MOVE;

		n_legal_moves_done++;

		// --- MOVE WAS LEGAL AND IS DONE ---

		if (stats)
			stats->nodes++;


		// Quiescence pruning after move-make

		if (failed_q_prune) {
			if (is_in_check(board, board->sidetomove)) {
				failed_q_prune = false;
			}
		}

		if (failed_q_prune) {
			unmakemove(board);
			n_fail_q_prune++;
			continue;
		}

		eval = q_search(board, stats, search_depth, alpha, beta);
		//printf("info string %f\n", eval);

		// if move was better, store it instead
		if (is_eval_better(eval, bestmove_eval, initial_side)) {
			bestmove_eval = eval;
			// if (bestmove)
			// 	memcpy(bestmove, move, sizeof (move_s));
		}

		// alpha is maximizing and beta is minimizing
		if (initial_side == WHITE)
			alpha = better_eval(alpha, eval, WHITE);
		else
			beta = better_eval(beta, eval, BLACK);
		
		if (beta <= alpha) {
			fail_low = true;
			// if (stats)
			// 	stats->fail_hard_cutoffs[stats->n_plies - depth]++;
			unmakemove(board);
			break; // at least 1 legal move has to have been made so we can just break search
		}


		REGULAR_SEARCH_SKIP_MOVE:
		unmakemove(board);
		REGULAR_SEARCH_SKIP_MOVE_PRE_MAKE:
		continue;
	}

	// Free this stuff
	for (unsigned int i = 0; i < n_pieces; i++) {
		if (all_moves[i].n) {
			free(all_moves[i].moves);
			free(move_visited[i]);
		}
	}
	free(all_moves);
	free(move_visited);

	assert(board->every_piece == every_piece_copy);

	// alpha-beta pruning
	if (fail_low) {
		if (initial_side == WHITE)
			return alpha;
		else
			return beta;
	}


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

	if (n_fail_q_prune == n_legal_moves_done) {
		return eval(board);
	}
	
	return bestmove_eval;
}

