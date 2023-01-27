#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <math.h>
#include <time.h>

#include "search.h"

#include "movegen.h"
#include "bitboard.h"
#include "board.h"
#include "eval.h"
#include "uci.h"
#include "algebraic.h"
#include "transposition.h"

#include "defs.h"



// Private functions
uint16_t encode_compact_move(const move_s* move);
eval_t regular_search(board_s* restrict board, move_s* restrict bestmove, search_stats_s* restrict stats, pv_s* restrict pv, const size_t history_n_at_root, const clock_t time1, const clock_t time_available, const int search_depth, const int depth, const int actual_depth, const bool is_null_prune, eval_t alpha, eval_t beta);
eval_t q_search(board_s* restrict board, search_stats_s* restrict stats, const unsigned int search_depth, const int depth, unsigned int qdepth, eval_t alpha, eval_t beta);


eval_t uci_think(const uci_s* uci, board_s* restrict board, move_s* restrict bestmove, FILE* restrict f) {
	assert(uci->action != UCI_IDLE);

	//if (uci->action == UCI_PONDER)
	//	goto THINK_PONDER;

	move_s move = {0};
	eval_t move_eval = 0;

	
	pv_s pv = {0};

	// Total memory usage of the pv with max_depth 99 is as such:
	//2bytes×((99×99)/2) + 99×8bytes = 10.593kB

	const size_t max_depth = 99;

	pv.n_moves = calloc(max_depth, sizeof (size_t));
	pv.pv = calloc(max_depth, sizeof (uint16_t*));
	for (size_t i = 0; i < max_depth; i++) {
		pv.pv[i] = calloc(max_depth - i, sizeof (uint16_t));
	}
	pv.depth_allocated = max_depth;

	// from: https://www.talkchess.com/forum3/viewtopic.php?t=51135
	unsigned int predicted_moves_left = 20; // originally 15
	if (board->history_n <= 30) // originally 25
		predicted_moves_left = 43 - board->history_n; // originally 40

	const unsigned long time_left = (board->sidetomove == WHITE ? uci->wtime : uci->btime);
	const unsigned long increment = (board->sidetomove == WHITE ? uci->winc : uci->binc);
	const clock_t time_allotted = (time_left*(CLOCKS_PER_SEC/1000))/predicted_moves_left + increment;

	const time_t t = clock();
	
	for (int depth = 1; 1; depth++) {
		const time_t last_search_time = clock();

		memset(pv.n_moves, 0, max_depth * sizeof (size_t));
		
		for (size_t i = 0; i < pv.depth_allocated; i++)
			memset(pv.pv[i], 0, (pv.depth_allocated - i) * sizeof (uint16_t));

		// Normal search (alpha is maximizing and beta minimizing)
		eval_t current_eval = regular_search(board, &move, NULL, &pv, board->history_n, t, time_allotted, depth, depth, 0, false, EVAL_MIN, EVAL_MAX);

		if (clock() - t >= time_allotted)
			break;
		

		memcpy(bestmove, &move, sizeof (move_s));
		move_eval = current_eval;

		//char str[6];
		
		char pv_str[125];
		size_t current_str_index = 0;
		for (size_t i = 0; i < pv.n_moves[0]; i++) {
			char move[6];
			//move_to_uci_notation(&(pv.pv.moves[i]), move);
			compact_move_to_uci_notation(pv.pv[0][i], move);
			strcpy(pv_str + current_str_index, move);
			current_str_index += strlen(move);
			pv_str[current_str_index++] = ' ';
		}
		pv_str[current_str_index] = '\0';
		

		uci_write(f, "info depth %i score cp %i pv %s\n", depth, move_eval, pv_str);

		// mate
		if (move_eval == EVAL_MAX || move_eval == EVAL_MIN)
			break;

		// Check if we would have enough time for next search.
		// Next search is guessed to be at least 1.5x the duration of this search.
		const float t_mult = ((depth % 2 == 0) ? 1.5f : 2.2f);
		const time_t allotted_time_left = time_allotted - (clock() - t);
		if ((clock() - last_search_time) * t_mult >= allotted_time_left || allotted_time_left < 0)
			break;
	}

	//free(pv.pv.moves);
	for (size_t i = 0; i < max_depth; i++) {
		//pv.pv[i] = calloc(max_depth - i, sizeof (uint16_t));
		free(pv.pv[i]);
	}
	//pv.n_moves = calloc(max_depth, sizeof (size_t));
	free(pv.n_moves);
	//pv.pv = calloc(max_depth, sizeof (uint16_t));
	free(pv.pv);


	return move_eval;


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

	//stats->n_positions_size = depth+1;

	stats->fail_hard_cutoffs = calloc(depth+1, sizeof (unsigned long long));

	return regular_search(board, bestmove, stats, 0, (board->sidetomove == WHITE ? true : false), 0, 0, depth, depth, 0, false, EVAL_MIN, EVAL_MAX);
}



uint16_t encode_compact_move(const move_s* move) {
	uint16_t res = 0x0;
	
	res |= lowest_bitindex(move->from) << 10;
	res |= lowest_bitindex(move->to) << 4;
	
	if (move->flags & FLAG_PROMOTE) {
		assert(!(res & COMPACT_MOVE_PROMOTE_FLAG)); // Make sure flag wasn't set already for some reason
		assert(move->promoteto < 2*2*2); // make sure promoteto actually fits

		res |= COMPACT_MOVE_PROMOTE_FLAG;
		res |= move->promoteto;

		assert(COMPACT_MOVE_PROMOTETO(res) == move->promoteto);
	}

	assert(SQTOBB(COMPACT_MOVE_FROM(res)) == move->from);
	assert(SQTOBB(COMPACT_MOVE_TO(res)) == move->to);

	return res;
}



// always when calling set was_horizon_node to NULL 
eval_t regular_search(board_s* restrict board, move_s* restrict bestmove, search_stats_s* restrict stats, pv_s* restrict pv, const size_t history_n_at_root, const clock_t time1, const clock_t time_available, const int search_depth, const int depth, const int actual_depth, const bool is_null_prune, eval_t alpha, eval_t beta) {
	if (stats && depth >= 0 && depth <= search_depth)
		stats->n_positions[stats->n_plies - depth]++;
	
	if (time_available) {
		if (clock() - time1 > time_available)
			return 0; // every node henceforth will also return early
	}
	
	bool tt_entry_found = false;
	tt_entry_s tt_entry;
	tt_entry_s* entry = probe_table(&tt_normal, board->hash);
	
	if (entry) {
		if (stats)
			stats->hashtable_hits++;
		
		// (how many depth are supposed to be under the entry)  >= ( vs how many under this node)
		//if (entry->search_depth - entry->node_depth >= (signed int)search_depth - ((signed int)search_depth - depth) &&
		//     depth - search_depth != 0) { // and don't replace root node
		if (entry->depth > depth && actual_depth) {
			
			if (stats)
				stats->nodes++;

			if (pv && !is_null_prune) {
				pv->n_moves[actual_depth] = 1;
				pv->n_moves[actual_depth + 1] = 0;
				pv->pv[actual_depth][0] = entry->bestmove;
			}

			return entry->eval;
		}

		tt_entry_found = true;
		memcpy(&tt_entry, entry, sizeof (tt_entry_s));
	}
	

	/*while (entry) {
		if (stats)
			stats->hashtable_hits++;
		

	}*/

	if (depth <= 0) {
		if (pv && !is_null_prune)
			pv->n_moves[actual_depth] = 0;
		return q_search(board, stats, search_depth, depth - 1, 0, alpha, beta);
	}

	if (stats)
		stats->nodes++;
	
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
	//bool** move_visited = calloc(n_pieces, sizeof (bool*)); //[piece_index][move_index]
	bool move_visited[16][30];
	memset(&move_visited, 0, 16*30* sizeof (bool));
	

	bool do_null_move = false;
	if (depth > NULL_MOVE_PRUNING_R(depth) + 1 && search_depth-depth > 0 && !initially_in_check && !is_null_prune)
		do_null_move = true;

	BitBoard bestmove_from = 0x0;
	BitBoard bestmove_to = 0x0;
	unsigned int bestmove_promoteto = 0;
	
	move_s* best_move_here = NULL;

	// Go through every piece and create the moves
	BitBoard pieces_copy = board->all_pieces[board->sidetomove];
	for (size_t i = 0; i < n_pieces; i++) {
		all_moves[i] = get_pseudo_legal_squares(board, pop_bitboard(&pieces_copy));
		
		n_all_moves += all_moves[i].n;

		//if (all_moves[i].n)
		//	move_visited[i] = calloc(all_moves[i].n, sizeof (bool));
		
		// if tt entry was found for this board, set the score for the saved move to best
		// this means that move stored in tt will always be preferred and done first
		unsigned int tt_entry_from = COMPACT_MOVE_FROM(tt_entry.bestmove);
		unsigned int tt_entry_to = COMPACT_MOVE_TO(tt_entry.bestmove);
		unsigned int  tt_entry_promoteto = COMPACT_MOVE_PROMOTETO(tt_entry.bestmove);
		if (tt_entry_found) {
			for (size_t j = 0; j < all_moves[i].n; j++) {

				if (all_moves[i].moves[j].from == SQTOBB(tt_entry_from)) {

					if (all_moves[i].moves[j].to == SQTOBB(tt_entry_to)) {

						if (all_moves[i].moves[j].flags & FLAG_PROMOTE) { // check if promote
							if (all_moves[i].moves[j].promoteto == tt_entry_promoteto) // promoteto same
								all_moves[i].moves[j].move_score = EVAL_MAX;
						}
						else // no promote so just from and to need to be same
							all_moves[i].moves[j].move_score = EVAL_MAX;
					}
				}
			}
		}
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
				eval = regular_search(board, NULL, stats, pv, history_n_at_root, time1, time_available, search_depth, depth - NULL_MOVE_PRUNING_R(depth) - 1, actual_depth + 1, true, alpha - 1, alpha);
			else
				eval = regular_search(board, NULL, stats, pv, history_n_at_root, time1, time_available, search_depth, depth - NULL_MOVE_PRUNING_R(depth) - 1, actual_depth + 1, true, beta - 1, beta);
		}
		else {
			// check if that side got itself in check (or couldn't get out of one)
			if (is_in_check(board, initial_side)) {
				skipped_because_of_checks++;
				goto REGULAR_SEARCH_SKIP_MOVE;
			}

			n_legal_moves_done++;

			// --- MOVE WAS LEGAL AND IS DONE ---
		
			int depth_modifier = 0;

			// if (move->move_score == EVAL_MAX && actual_depth < search_depth - 4 && depth < 3)
			// 	depth_modifier++;
			if (move && !initially_in_check) {
				 if ((depth == 3 || depth == 4) && alpha != EVAL_MIN && beta != EVAL_MAX) {
				 	eval_t qeval = q_search(board, stats, search_depth, depth - 1, 0, alpha, beta);

					//if (initial_side == WHITE) {
					// if (qeval < alpha && qeval > beta && actual_depth < search_depth - 2)
					// 	//depth_modifier += 1;
					// else {
					// 	depth_modifier -= 1;
					// }
					//}
					if (!(qeval < alpha && qeval > beta))
						depth_modifier -= 1;
					/*
					else {
						if (qeval > beta )
							depth_modifier -= 1;
					}*/
				}
				if (move->flags & FLAG_CAPTURE && depth_modifier == 0) {
					//eval_t move_see = see(board, move);

					// if (move_see < -100 && depth < 2)
					//  	depth_modifier -= 1;
					
					
					// if (move_see < -200 && depth < 3)
					// 	depth_modifier--;
					// if (move->move_see <= -900 && depth == 5)
					// 	depth_modifier--;
					if (move->move_see <= -700 && depth == 4)
						depth_modifier--;
					if (move->move_see <= -200 && (depth == 3))
						depth_modifier--;
					// else if (move_see > 600 && depth > 1 && depth < 3)
					// 	depth_modifier++;
					if (move->move_see < -40 && depth <= 2)
						depth_modifier--;
					/*
					if (depth < 2 && depth_modifier >= 0) {
						eval_t material = eval_material(board, get_game_phase_value(board));
						if (board->sidetomove == WHITE) {
							if (material + move_see + 20 > alpha)
								depth_modifier--;
							// else
							// 	depth_modifier++;
						}
						else {
							if (material - move_see - 20 < beta)
								depth_modifier--;
							// else
							// 	depth_modifier++;
						}
					}
					*/
				}
				if (move->flags & FLAG_PROMOTE)
					depth_modifier++;
			}

			if (depth_modifier <= 0 && actual_depth < search_depth - 1) { // && depth < 2) {
				if (is_in_check(board, board->sidetomove))
					depth_modifier += 1;
			}

			eval = regular_search(board, NULL, stats, pv, history_n_at_root, time1, time_available, search_depth, depth-1+depth_modifier, actual_depth + 1, is_null_prune, alpha, beta);
			//printf("info string %f\n", eval);

			// if move was better, store it instead.
			// Or if no legal moves have been stored to yet; store it now
			if (is_eval_better(eval, bestmove_eval, initial_side) || bestmove_from == 0) {
				assert(popcount(move->from) == 1);
				assert(popcount(move->to) == 1);
				bestmove_eval = eval;
				bestmove_from = move->from;
				bestmove_to = move->to;
				if (move->flags & FLAG_PROMOTE)
					bestmove_promoteto = move->promoteto;
				else
					bestmove_promoteto = 0x0;
				
				if (bestmove)
					memcpy(bestmove, move, sizeof (move_s));
				
				best_move_here = move;
			}
		}

		// alpha is maximizing and beta is minimizing
		if (initial_side == WHITE)
			alpha = better_eval(alpha, eval, WHITE);
		else
			beta = better_eval(beta, eval, BLACK);
		
		if (beta <= alpha) {
			fail_low = true;
			if (stats && depth > 0 && depth <= search_depth)
				stats->fail_hard_cutoffs[stats->n_plies - depth]++;
			unmakemove(board);
			break; // at least 1 legal move has to have been made so we can just break search
		}

		// if (pv && !is_null_prune && move != NULL && move == best_move_here) {
		// 	const size_t node_depth = search_depth - depth;
		// 	// save this node to the pv
		// 	pv->n_moves[node_depth] = 1;
		// 	pv->pv[node_depth][0] = encode_compact_move(move);

		// 	// if the searched nodes saved something to pv, save it to this depth
		// 	if (pv->n_moves[node_depth + 1] > 0) {
		// 		memcpy(&pv->pv[node_depth][1], &pv->pv[node_depth+1][0], pv->n_moves[node_depth+1] * sizeof (uint16_t));
		// 		pv->n_moves[node_depth] = pv->n_moves[node_depth + 1] + 1;
		// 	}
		// }


		REGULAR_SEARCH_SKIP_MOVE:
		unmakemove(board);
		REGULAR_SEARCH_SKIP_MOVE_PRE_MAKE:
		continue;
	}

	assert(board->every_piece == every_piece_copy);

	if (time_available) {
		if (clock() - time1 > time_available) {
			//return 0; // every node henceforth will also return early
			goto REGULAR_SEARCH_SEARCH_END;
		}
	}

	// alpha-beta pruning
	if (fail_low) {
		if (initial_side == WHITE)
			bestmove_eval = alpha; //return alpha;
		else
			bestmove_eval = beta; //return beta;
		
		//if (pv)
		//	pv->n_moves[search_depth - depth] = 0;
		
		goto REGULAR_SEARCH_SEARCH_END;
	}

	// No moves were made??
	if (!n_legal_moves_done) {
		if (initially_in_check) { // is a checkmate (was in check and can't get out of it)
			// store the worst value possible for checkmate
			bestmove_eval = (is_eval_better(EVAL_MAX, EVAL_MIN, board->sidetomove) ? EVAL_MIN : EVAL_MAX);
		}
		else { // is a stalemate (wasn't in check and no legal moves)
			bestmove_eval = 0;
		}
		if (pv && !is_null_prune)
			pv->n_moves[actual_depth] = 0;
		goto REGULAR_SEARCH_SEARCH_END;
		//return bestmove_eval;
		//store_move(&tt_normal, board->hash, bestmove_eval, search_depth, (search_depth - depth), )
	}

	if (pv && !is_null_prune) {
		//const size_t node_depth = search_depth - depth;
		const size_t node_depth = actual_depth;
		// save this node to the pv
		pv->n_moves[node_depth] = 1;
		pv->pv[node_depth][0] = encode_compact_move(best_move_here);

		// if the searched nodes saved something to pv, save it to this depth
		if (pv->n_moves[node_depth + 1] > 0) {
			memcpy(&pv->pv[node_depth][1], &pv->pv[node_depth+1][0], pv->n_moves[node_depth+1] * sizeof (uint16_t));
			pv->n_moves[node_depth] = pv->n_moves[node_depth + 1] + 1;
		}
	}

	if (!is_null_prune)
		store_move(&tt_normal, board->hash, bestmove_eval, 0x0, depth, encode_compact_move(best_move_here));

	REGULAR_SEARCH_SEARCH_END:

	// Free this stuff
	for (unsigned int i = 0; i < n_pieces; i++) {
		if (all_moves[i].n) {
			free(all_moves[i].moves);
			//free(move_visited[i]);
		}
	}
	free(all_moves);
	//free(move_visited);

	return bestmove_eval;
}



// qdepth starts always at 0 and is incremented every qnode.
eval_t q_search(board_s* restrict board, search_stats_s* restrict stats, const unsigned int search_depth, const int depth, unsigned int qdepth, eval_t alpha, eval_t beta) {
	// if (depth <= 0)
	// 	return eval(board);

	assert(depth < 0);

	// Looking up entries here slows search around 300knps
	// TODO: Do more testing in future for this
	/*
	if (qdepth > 0) {
		const tt_entry_s* entry = probe_table(&tt_normal, board->hash);

		if (entry) {
			if (stats) {
				stats->nodes++;
				stats->hashtable_hits++;
			}
			return entry->eval;
		}
	}
	*/

	// if (qdepth > 4) {
	// 	return eval(board);
	// }

	/*
	if (stats) {
		const unsigned int curr_depth = search_depth + -depth;
		if (curr_depth + 1 > stats->n_plies) {
			unsigned long long* new_n_positions = realloc(stats->n_positions, sizeof (unsigned long long) * (curr_depth + 1));
			if (!new_n_positions) {
				puts("No memory to realloc n_positions! Aborting...");
				abort();
			}
			stats->n_positions = new_n_positions;
			stats->n_plies = curr_depth;
		}
	 	stats->n_positions[curr_depth]++;
	}
	*/

	
	
	
	/*
	if (depth <= Q_SEARCH_STANDPAT_PRUNING_DEPTH_TRESHOLD) {
		if (board->sidetomove == WHITE) {
			if (stand_pat <= alpha)
				return alpha;
			//alpha = MAX(stand_pat, alpha); //better_eval(stand_pat, alpha, board->sidetomove);
		}
		else {
			if (stand_pat >= beta)
				return beta;
			//beta = MIN(stand_pat, beta); //better_eval(stand_pat, beta, board->sidetomove);
		}
	}
	*/

	
	//if (qdepth >= 0) {
	// const eval_t stand_pat = eval_material(board, get_game_phase_value(board));//eval(board);

	// // Delta pruning i think is the term for the following pruning?
	// big_eval_t big_delta = EVAL_QUEEN_MATERIAL_VALUE - 100; // - 100;

	// if (promote_available(board, board->sidetomove))
	// 	big_delta -= EVAL_PAWN_MATERIAL_VALUE;
	
	// if (board->sidetomove == WHITE) {
	// 	if (stand_pat < (big_eval_t)alpha - big_delta)
	// 		return alpha;
	// 	if (qdepth > 0)
	// 		alpha = better_eval(alpha, stand_pat, board->sidetomove);
	// }
	// else {
	// 	if (stand_pat > (big_eval_t)beta + big_delta)
	// 		return beta;
	// 	if (qdepth > 0)
	// 		beta = better_eval(beta, stand_pat, board->sidetomove);
	// }
	//}
	
	
	const bool initially_in_check = is_in_check(board, board->sidetomove);

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
	
	
	const unsigned int n_pieces = popcount(board->all_pieces[board->sidetomove]);
	
	// !!!!REMEMBER TO FREE!!!!
	// for tracking, which moves have already been selected (selection sort kinda)
	//bool** move_visited = calloc(n_pieces, sizeof (bool*)); //[piece_index][move_index]
	bool move_visited[16][30];
	memset(&move_visited, 0, 16*30 * sizeof (bool));

	movelist_s* all_moves = calloc(n_pieces, sizeof (movelist_s));
	unsigned int n_all_moves = 0;

	// Go through every piece and create the moves
	BitBoard pieces_copy = board->all_pieces[board->sidetomove];
	for (size_t i = 0; i < n_pieces; i++) {
		all_moves[i] = get_pseudo_legal_squares(board, pop_bitboard(&pieces_copy));

		n_all_moves += all_moves[i].n;

		//if (all_moves[i].n)
			//move_visited[i] = calloc(all_moves[i].n, sizeof (bool));
	}
	
	if (stats)
		stats->n_moves_generated += n_all_moves;

	eval_t last_best_move = EVAL_MAX;

	for (unsigned int i = 0; i < n_all_moves; i++) {
		move_s* move = NULL;

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
					continue;
			}
		}

		// Quiescence pruning pre move-make

		bool failed_q_prune = true;

		if (move->flags & FLAG_CAPTURE) {
			//eval_t move_see = see(board, move);

			assert(-(eval_t)depth - 1 >= 0);

			// if (move->move_see > 0)
			// 	failed_q_prune = false;
			
			// if (move->move_see < -100)
			// 	failed_q_prune = true;
			if (board->sidetomove == WHITE) {
				const int phase = get_game_phase_value(board);
				if (eval_material(board, phase) + move->move_see + Q_SEARCH_PRUNE_TRESHOLD > alpha)
					failed_q_prune = false;
			}
			else {
				const int phase = get_game_phase_value(board);
				if (eval_material(board, phase) - move->move_see - Q_SEARCH_PRUNE_TRESHOLD < beta)
					failed_q_prune = false;
			}
			

			//if (move->piece_captured == PAWN && move->to & (move->side == BLACK ? Q_SEARCH_PAWN_SELECT_MASK_W : Q_SEARCH_PAWN_SELECT_MASK_B))
			//	failed_q_prune = false;
			//else if (move->fromtype == KING)
			//	failed_q_prune = false;
		}
		if (move->flags & FLAG_PROMOTE)
			failed_q_prune = false;
		// else
		// 	failed_q_prune = true;
		
		//if (move->fromtype == PAWN && move->from & (move->side == WHITE ? Q_SEARCH_PAWN_SELECT_MASK_W : Q_SEARCH_PAWN_SELECT_MASK_B))
		//	failed_q_prune = false;
		
		makemove(board, move);

		// worst eval by default
		eval_t eval = better_eval(EVAL_MAX, EVAL_MIN, OPPOSITE_SIDE(initial_side));
		
		// check if that side got itself in check (or couldn't get out of one)
		if (is_in_check(board, initial_side))
			goto Q_SEARCH_SKIP_MOVE;

		n_legal_moves_done++;

		// --- MOVE WAS LEGAL AND IS DONE ---

		if (stats)
			stats->nodes++;


		// Quiescence pruning after move-make

		// if (failed_q_prune && qdepth < 2) {
		// 	if (is_in_check(board, board->sidetomove)) {
		// 		failed_q_prune = false;
		// 	}
		// }

		if (failed_q_prune) {
			unmakemove(board);
			n_fail_q_prune++;
			continue;
		}

		eval = q_search(board, stats, search_depth, depth - 1, qdepth + 1, alpha, beta);
		//printf("info string %f\n", eval);

		// if move was better, store it instead
		if (is_eval_better(eval, bestmove_eval, initial_side))
			bestmove_eval = eval;

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


		Q_SEARCH_SKIP_MOVE:
		unmakemove(board);
	}

	// Free this stuff
	for (unsigned int i = 0; i < n_pieces; i++) {
		if (all_moves[i].n) {
			free(all_moves[i].moves);
			//free(move_visited[i]);
		}
	}
	free(all_moves);
	//free(move_visited);

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

