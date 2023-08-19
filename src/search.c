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
#include "lookup.h"
#include "movefactory.h"

#include "defs.h"


#define CONTEMPT_FACTOR (20)


BitBoard killer_moves[MAX_DEPTH][2][2]; // [ply][first / second][from / to]
uint64_t hh_score[2][64][64] = {0}; // [side][from_sq][to_sq] // Counts cutoffs
uint64_t bf_score[2][64][64] = {0}; // [side][from_sq][to_sq] // Counts else


bool abort_search = false;


// Private functions
uint16_t encode_compact_move(const move_s* move);
static eval_t regular_search(board_s* restrict board, move_s* restrict bestmove, search_stats_s* restrict stats, pv_s* restrict pv, const clock_t time1, const clock_t time_available, const int search_depth, int depth, const int actual_depth, const bool is_null_prune, eval_t alpha, eval_t beta);
static eval_t q_search(board_s* restrict board, search_stats_s* restrict stats, const unsigned int search_depth, const int depth, unsigned int qdepth, eval_t alpha, eval_t beta);
static eval_t search_root_node(board_s* restrict board, move_s* restrict bestmove, search_stats_s* restrict stats, pv_s* restrict pv, const clock_t time1, const clock_t time_available, const int depth, const eval_t last_eval);
static eval_t pv_search(board_s* restrict board, int depth, const int ply, search_stats_s* restrict stats, pv_s* restrict pv, const clock_t start_time, const clock_t time_available, const bool is_leftmost_path, eval_t alpha, eval_t beta);
static eval_t zw_search(board_s* restrict board, int depth, const int ply, search_stats_s* restrict stats, const clock_t start_time, const clock_t time_available, const bool is_null_move, eval_t alpha, eval_t beta);
static eval_t new_q_search(board_s* restrict board, const int qdepth, const int ply, search_stats_s* restrict stats, eval_t alpha, eval_t beta);





eval_t uci_think(const uci_s* uci, board_s* restrict board, move_s* restrict bestmove, FILE* restrict f) {
	assert(uci->action != UCI_IDLE);

	//if (uci->action == UCI_PONDER)
	//	goto THINK_PONDER;

	memset(killer_moves, 0, MAX_DEPTH*2*2 * sizeof (BitBoard));
	memset(hh_score, 0, 2*64*64 * sizeof (uint64_t));
	memset(bf_score, 0, 2*64*64 * sizeof (uint64_t));

	move_s move = {0};
	eval_t move_eval = 0;


	search_stats_s stats;

	pv_s pv = {0};

	// Allocate pv tables

	// Total memory usage of the pv with max_depth 99 is as such:
	//2bytes×((99×99)/2) + 99×8bytes = 10.593kB

	pv.n_moves = calloc(MAX_DEPTH, sizeof (size_t));
	pv.pv = calloc(MAX_DEPTH, sizeof (uint16_t*));
	for (size_t i = 0; i < MAX_DEPTH; i++) {
		pv.pv[i] = calloc(MAX_DEPTH - i, sizeof (uint16_t));
	}
	pv.depth_allocated = MAX_DEPTH;

	// from: https://www.talkchess.com/forum3/viewtopic.php?t=51135
	unsigned int predicted_moves_left = 18; // originally 15
	if (board->fullmoves <= 25) // originally 25
		predicted_moves_left = 40 - board->fullmoves; // originally 40

	const double time_left = (double)(board->sidetomove == WHITE ? uci->wtime : uci->btime);
	const double increment = (double)(board->sidetomove == WHITE ? uci->winc : uci->binc);
	const clock_t time_allotted = (((time_left/1000.0)*(double)CLOCKS_PER_SEC)/predicted_moves_left) + increment; // -20 reserved for exiting search

	const clock_t t = clock();
	
	for (int depth = 1; 1; depth++) {
		const clock_t last_search_time = clock();

		memset(&stats, 0, sizeof (search_stats_s));

		abort_search = false;

		
		// Don't null the lowest level because that needs to be followed by the next iteration
		memset(&pv.n_moves[1], 0, (MAX_DEPTH - 1) * sizeof (size_t));
		
		for (size_t i = 1; i < pv.depth_allocated; i++)
			memset(pv.pv[i], 0, (pv.depth_allocated - i) * sizeof (uint16_t));
		

		// Normal search (alpha is maximizing and beta minimizing)
		//eval_t current_eval = regular_search(board, &move, &stats, &pv, t, time_allotted, depth, depth, 0, false, EVAL_MIN, EVAL_MAX);
		eval_t current_eval = search_root_node(board, &move, &stats, &pv, t, time_allotted, depth, 0);

		if (clock() - t >= time_allotted)
			break;


		memcpy(bestmove, &move, sizeof (move_s));
		move_eval = current_eval;

		//char str[6];
		
		char pv_str[125];
		size_t current_str_index = 0;
		for (size_t i = 0; i < pv.n_moves[0]; i++) {
			char move[6];
			compact_move_to_uci_notation(pv.pv[0][i], move);
			strcpy(pv_str + current_str_index, move);
			current_str_index += strlen(move);
			pv_str[current_str_index++] = ' ';
		}
		pv_str[current_str_index] = '\0';
		
		const unsigned long long nps = (unsigned long long)((double)stats.nodes/((double)(clock() - last_search_time)/(double)CLOCKS_PER_SEC));
		const unsigned int search_time = (unsigned int)(((double)(clock() - t)/(double)CLOCKS_PER_SEC)*1000);
		uci_write(f, "info depth %i score cp %i time %u nodes %llu nps %d pv %s\n", depth, move_eval, search_time, stats.nodes, nps, pv_str);

		// mate
		if (((move_eval == EVAL_MAX || move_eval == EVAL_MIN) && depth > 70) || depth >= MAX_DEPTH)
			break;

		// Check if we would have enough time for next search.
		// Next search is guessed to be at least 1.5x the duration of this search.
		const float t_mult = ((depth % 2 == 0) ? 1.5f : 2.2f);
		const time_t allotted_time_left = time_allotted - (clock() - t);
		if ((clock() - last_search_time) * t_mult >= allotted_time_left || allotted_time_left < 0)
			break;
	}


	// Free the pv table

	for (size_t i = 0; i < MAX_DEPTH; i++) {
		free(pv.pv[i]);
	}
	free(pv.n_moves);
	free(pv.pv);


	return move_eval;


	//THINK_PONDER:
	// TODO
	//fputs("Pondering not supported!", stderr);

	//return NAN;
}


eval_t search_with_stats(board_s* restrict board, move_s* restrict bestmove, const unsigned int depth, search_stats_s* restrict stats) {
	memset(stats, 0, sizeof (search_stats_s));

	memset(killer_moves, 0, MAX_DEPTH*2*2 * sizeof (BitBoard));
	memset(hh_score, 0, 2*64*64 * sizeof (uint64_t));
	memset(bf_score, 0, 2*64*64 * sizeof (uint64_t));

	stats->n_plies = depth;

	// +1 becouse we start at 0 and end in depth, not depth-1
	// n_positions[0] should always be 1 becouse there is only 1 root node (duh!)
	stats->n_positions = calloc(depth+1, sizeof (unsigned long long));

	//stats->n_positions_size = depth+1;

	stats->fail_hard_cutoffs = calloc(depth+1, sizeof (unsigned long long));

	return regular_search(board, bestmove, stats, NULL, 0, 0, depth, depth, 0, false, EVAL_MIN, EVAL_MAX);
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


// TODO: Test this function. Have no idea if this actually works or not.
// Get a pointer to the next best move.
static move_s* get_next_best_move(const movelist_s* restrict all_moves, size_t n_of_movelists, const move_s* restrict last_move) {
	const eval_t last_move_score = (last_move != NULL ? last_move->move_score : EVAL_MAX);
	eval_t currently_selected_move_score = EVAL_MIN;
	move_s* currently_selected_move = NULL;
	bool last_move_seen = false; // set to true if one of the pointers seen are the same as last_move

	// Loops through the moves until it finds a same score as last_move_score if last_move_seen was set to true
	// Else it just gets the next biggest(as big as possible but smaller than last_move_score)

	for (size_t i = 0; i < n_of_movelists; i++) {
		for (size_t j = 0; j < all_moves[i].n; j++) {
			if (!all_moves[i].moves[j].from)
				continue;
			
			// smaller than previous best but bigger than the currently selected
			if (all_moves[i].moves[j].move_score > currently_selected_move_score
			   && all_moves[i].moves[j].move_score < last_move_score) {
				
				currently_selected_move = &all_moves[i].moves[j];
				currently_selected_move_score = all_moves[i].moves[j].move_score;
			}

			// TODO: if score == last_move_score+1 : return

			// if the previously selected move has been seen, allow returning a move with the same score.
			if (last_move_seen
			   && all_moves[i].moves[j].move_score == last_move_score) {
				return &all_moves[i].moves[j];
			}
			
			// AAAAAAAA
			if (&all_moves[i].moves[j] == last_move)
				last_move_seen = true;
		}
	}

	return currently_selected_move;
}



static eval_t search_root_node(board_s* restrict board, move_s* restrict bestmove, search_stats_s* restrict stats, pv_s* restrict pv, const clock_t time1, const clock_t time_available, const int depth, const eval_t last_eval) {

	if (stats)
		stats->nodes++;


	const bool initially_in_check = is_in_check(board, board->sidetomove);


	uint16_t special_moves[2];
	size_t n_special_moves = 0;

	if (pv->n_moves[0] >= 1) {
		special_moves[0] = pv->pv[0][0];
		assert(popcount(SQTOBB(COMPACT_MOVE_FROM(special_moves[0]))) == 1);
		assert(popcount(SQTOBB(COMPACT_MOVE_TO(special_moves[0]))) == 1);
		assert(SQTOBB(COMPACT_MOVE_FROM(special_moves[0])) & board->all_pieces[board->sidetomove]);
		assert(!(SQTOBB(COMPACT_MOVE_TO(special_moves[0])) & board->all_pieces[board->sidetomove]));
		n_special_moves++;
	}

	// if  hash move found
	//    make up something

	movefactory_s movefactory;
	//init_movefactory(&movefactory, &killer_moves[0], special_moves, n_special_moves);
	init_movefactory(&movefactory, NULL, special_moves, n_special_moves);


	unsigned int n_legal_moves_done = 0;
	move_s* move = NULL;
	eval_t best_score = EVAL_MIN;

	eval_t alpha = EVAL_MIN, beta = EVAL_MAX;

	for (size_t i = 0; 1; i++) {

		move = get_next_move(board, &movefactory, false);
		
		if (!move)
			break;
		
		if (time_available) {
			if (clock() - time1 >= time_available)
				return 0; // every node henceforth will also return early
		}
		
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
					goto SEARCH_ROOT_NODE_SKIP_MOVE_PRE_MAKE;
			}
		}
		
		
		makemove(board, move);

		if (is_in_check(board, move->side)) {
			unmakemove(board, move);
			continue;
		}
		
		// Move was legal
		n_legal_moves_done++;

		eval_t score;


		bool full_search = false;

		if (n_legal_moves_done == 1) full_search = true;

		SEARCH_ROOT_NODE_RE_SEARCH:

		if (full_search)
			score = -pv_search(board, depth-1, 1, stats, pv, time1, time_available, i == 0, -beta, -alpha);
		else
			score = -zw_search(board, depth-1, 1, stats, time1, time_available, false, -alpha-1, -alpha);
		
		if (abort_search) {
			unmakemove(board, move);
			return 0;
		}

		if (!full_search) {
			if (score > best_score) {
				full_search = true;
				goto SEARCH_ROOT_NODE_RE_SEARCH;
			}
		}


		// Without (score == EVAL_MAX) it basically is shit at mating
		if (score > best_score) {
			best_score = score;
			memcpy(bestmove, move, sizeof (move_s));

			if (pv) {
				// save this node to the pv
				pv->n_moves[0] = 1;
				pv->pv[0][0] = encode_compact_move(move);

				// if the searched nodes saved something to pv, save it to this depth
				if (pv->n_moves[1] > 0) {
					memcpy(&pv->pv[0][1], &pv->pv[1][0], pv->n_moves[1] * sizeof (uint16_t));
					pv->n_moves[0] = pv->n_moves[1] + 1;
				}
			}
		}

		alpha = MAX(score, alpha);

		unmakemove(board, move);

		SEARCH_ROOT_NODE_SKIP_MOVE_PRE_MAKE:
		continue;
	}


	if (!n_legal_moves_done) {
		if (initially_in_check) { // is a checkmate (was in check and can't get out of it)
			return EVAL_MIN;
		}
		else { // is a stalemate (wasn't in check and no legal moves)
			return 0;
		}
	}

	return best_score;
}


static eval_t pv_search(board_s* restrict board, int depth, const int ply, search_stats_s* restrict stats, pv_s* restrict pv, const clock_t start_time, const clock_t time_available, const bool is_leftmost_path, eval_t alpha, eval_t beta) {

	if (time_available) {
			if (clock() - start_time >= time_available) {
				abort_search = true;
				return 0; // every node henceforth will also return early
			}
	}

	// If moves are still left in the pv, extend
	if (is_leftmost_path && depth <= 0 && pv) {
		if (pv->n_moves[0] > ply)
			depth++;
	}

	const bool initially_in_check = is_in_check(board, board->sidetomove);

	// Do not enter qsearch in check, get longer pv's PROBABLY(????) for free
	if (depth <= 0 && initially_in_check)
		depth++;

	if (depth <= 0) {
		if (pv)
			pv->n_moves[ply] = 0;
		return new_q_search(board, 0, ply+1, stats, alpha, beta);
	}

	if (abort_search) return 0;

	// Check for repetitions
	// -2 so that current position would not trigger if statement and to start comparing from last move by this side
	for (int i = board->rep_stack_n - 2; i >= 0; i--) {
		if (board->hash == board->rep_stack[i]) {
			if (pv)
				pv->n_moves[ply] = 0;
			return (ply % 2 == 0 ? CONTEMPT_FACTOR : -CONTEMPT_FACTOR);
		}
	}

	if (stats)
		stats->nodes++;
	


	const unsigned int n_pieces = popcount(board->all_pieces[board->sidetomove]);


	uint16_t special_moves[2];
	size_t n_special_moves = 0;

	if (pv) {
		if (is_leftmost_path && pv->n_moves[0] > ply) {
			special_moves[0] = pv->pv[0][ply];
			assert(popcount(SQTOBB(COMPACT_MOVE_FROM(special_moves[0]))) == 1);
			assert(popcount(SQTOBB(COMPACT_MOVE_TO(special_moves[0]))) == 1);
			assert(SQTOBB(COMPACT_MOVE_FROM(special_moves[0])) & board->all_pieces[board->sidetomove]);
			assert(!(SQTOBB(COMPACT_MOVE_TO(special_moves[0])) & board->all_pieces[board->sidetomove]));
			n_special_moves++;
		}
	}
	// if  hash move found
	//    make up something

	movefactory_s movefactory;
	init_movefactory(&movefactory, &killer_moves[ply], special_moves, n_special_moves);

	unsigned int n_legal_moves_done = 0;
	move_s* move = NULL;
	eval_t best_score = EVAL_MIN;

	for (size_t i = 0; 1; i++) {

		move = get_next_move(board, &movefactory, false);
		
		if (!move)
			break;

		
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
					goto PV_SEARCH_SKIP_MOVE_PRE_MAKE;
			}
		}

		assert(move->from & board->all_pieces[board->sidetomove]);
		assert(!(move->to & board->all_pieces[board->sidetomove]));
		
		makemove(board, move);

		if (is_in_check(board, move->side)) {
			unmakemove(board, move);
			continue;
		}
		
		// Move was legal
		n_legal_moves_done++;

		bool move_was_check = is_in_check(board, board->sidetomove);


		int depth_modifier = 0;

		if (move_was_check && ply < 20 && depth < 3)
			depth_modifier++;
		else if (move_was_check
		  && move->flags & FLAG_CAPTURE
		  && move->move_see > 0) {
			depth_modifier++;
		}

		// if (move_was_check
		//   && !(move->flags & FLAG_CAPTURE
		//    && move->move_see < 0)) {
		// 	depth_modifier++;
		// }

		// Check for pawn moves that threaten pieces (may trap pieces?)
		// if (move->fromtype == PAWN
		//   && !(move->flags & FLAG_PROMOTE)
		//   && depth_modifier == 0
		//   && depth == 1) {
		// 	if (piecelookup(lowest_bitindex(move->to), PAWN, move->side) & (board->all_pieces[board->sidetomove] & ~(board->pieces[board->sidetomove][PAWN])))
		// 		depth_modifier++;
		// }

		if (move->flags & FLAG_CAPTURE
		  && move->move_see >= 120 + (depth * 100)
		  && depth < 4
		  && depth_modifier == 0) {
			depth_modifier++;
		}
		// if (move->flags & FLAG_CAPTURE
		//   && move->move_see <= -40 - (depth * 60)
		//   && n_legal_moves_done > 4
		//   //&& depth <= 6
		//   && depth_modifier == 0) {
		// 	depth_modifier--;
		// }


		eval_t score = EVAL_MIN;

		bool full_search = false;

		if (depth == 1) full_search = true;

		if (n_legal_moves_done == 1) full_search = true;

		PV_SEARCH_RE_SEARCH:

		if (full_search)
			score = -pv_search(board, depth-1+depth_modifier, ply+1, stats, pv, start_time, time_available, (i == 0 && is_leftmost_path), -beta, -alpha);
		else
			score = -zw_search(board, depth-1+depth_modifier+(depth <= 2 ? 0 : 0), ply+1, stats, start_time, time_available, false, -alpha-1, -alpha);
		
		if (abort_search) {
			unmakemove(board, move);
			return 0;
		}


		if (!full_search) {
			if (score > alpha) {
				full_search = true;
				goto PV_SEARCH_RE_SEARCH;
			}
		}

		PV_SEARCH_SKIP_SEARCH:

		unmakemove(board, move);


		if (score > best_score || ((n_legal_moves_done == 1) && score > alpha)) {

			if (score >= beta) {

				if (pv)
					pv->n_moves[ply] = 0;

				if (!(move->flags & FLAG_CAPTURE)) {
					// If move is not already in killer, store it
					if (killer_moves[ply][0][0] != move->from || killer_moves[ply][0][1] != move->to) {
						killer_moves[ply][1][0] = killer_moves[ply][0][0];
						killer_moves[ply][1][1] = killer_moves[ply][0][1];
						killer_moves[ply][0][0] = move->from;
						killer_moves[ply][0][1] = move->to;
					}
					// move has been undone already
					if (depth > 1)
						hh_score[move->side][lowest_bitindex(move->from)][lowest_bitindex(move->to)] += 1;
				}

				return score;
			}
			else if (depth > 1 && !(move->flags & FLAG_CAPTURE))
				bf_score[move->side][lowest_bitindex(move->from)][lowest_bitindex(move->to)] += 1;

			if (score > alpha) {

				assert(full_search);

				if (pv) {
					// save this node to the pv
					pv->n_moves[ply] = 1;
					pv->pv[ply][0] = encode_compact_move(move);

					// if the searched nodes saved something to pv, save it to this depth
					if (pv->n_moves[ply + 1] > 0) {
						memcpy(&pv->pv[ply][1], &pv->pv[ply+1][0], pv->n_moves[ply+1] * sizeof (uint16_t));
						pv->n_moves[ply] = pv->n_moves[ply + 1] + 1;
					}
				}

				alpha = score;
			}
			best_score = score;
		}

		PV_SEARCH_SKIP_MOVE_PRE_MAKE:
		continue;
	}

	if (!n_legal_moves_done) {
		if (pv)
			pv->n_moves[ply] = 0;
		if (initially_in_check) { // is a checkmate (was in check and can't get out of it)
			return EVAL_MIN + ply;
		}
		else { // is a stalemate (wasn't in check and no legal moves)
			return (ply % 2 == 0 ? CONTEMPT_FACTOR : -CONTEMPT_FACTOR);
		}
	}

	return best_score;
}


static eval_t zw_search(board_s* restrict board, int depth, const int ply, search_stats_s* restrict stats, const clock_t start_time, const clock_t time_available, const bool is_null_move, eval_t alpha, eval_t beta) {

	if (time_available && depth >= 4) {
		if (clock() - start_time >= time_available) {
			abort_search = true;
			return 0; // every node henceforth will also return early
		}
	}

	if (abort_search) return 0;

	if (depth <= 0) {
		return new_q_search(board, 0, ply+1, stats, alpha, beta);
	}

	if (stats)
		stats->nodes++;

	// Check for repetitions
	// -2 so that current position would not trigger if statement
	//const size_t lower_rep_stack_limit = MAX(((int)board->rep_stack_n) - 2 - 4, 0); // search max 4 moves back
	for (int i = board->rep_stack_n - 2; i >= 0; i--) {
		if (board->hash == board->rep_stack[i])
			return (ply % 2 == 0 ? CONTEMPT_FACTOR : -CONTEMPT_FACTOR);
	}

	const bool initially_in_check = is_in_check(board, board->sidetomove);


	bool q_stand_pat_calculated = false;
	eval_t q_stand_pat;
	
	if (depth == 2
	   && !initially_in_check
	   && alpha != EVAL_MAX) {

		//const eval_t stand_pat = eval(board) * (board->sidetomove == WHITE ? 1 : -1);
		if (!q_stand_pat_calculated) {
			q_stand_pat = new_q_search(board, 0, ply, stats, alpha, beta);
			q_stand_pat_calculated = true;
		}
		
		if (q_stand_pat + (80 * depth) < alpha || q_stand_pat == EVAL_MAX || q_stand_pat == EVAL_MIN)
			return q_stand_pat;
	}

	
	// Reverse futility pruning

	if (depth > 1 // 2
	   && !initially_in_check
	   && alpha != EVAL_MAX) {

		const eval_t margin = depth * 95 + ((depth-2) * 20);
		// const eval_t margin = depth * 90;

		//const eval_t stand_pat = eval(board) * (board->sidetomove == WHITE ? 1 : -1);
		if (!q_stand_pat_calculated) {
			q_stand_pat = new_q_search(board, 0, ply, stats, alpha, beta);
			q_stand_pat_calculated = true;
		}
		
		if (q_stand_pat - margin >= beta)
			return q_stand_pat - margin;
	}


	/*
	if (depth <= 3
	   && !initially_in_check
	   && alpha != EVAL_MAX) {

		//const eval_t q_stand_pat = new_q_search(board, 0, stats, alpha, beta);
		const eval_t stand_pat = eval(board) * (board->sidetomove == WHITE ? 1 : -1);
		if (stand_pat + (80 * depth) < alpha) // || q_stand_pat == EVAL_MAX || q_stand_pat == EVAL_MIN)
			return new_q_search(board, 0, stats, alpha, beta);
	}
	*/

	/*
	// Razoring???
	if (depth <= 3
	   && !initially_in_check
	   && alpha != EVAL_MIN) {
		const eval_t stand_pat = eval(board) * (board->sidetomove == WHITE ? 1 : -1);
		// https://github.com/nescitus/cpw-engine/blob/2e3cf29ab0a79e65873c9f450832f9d3d5f02575/search.cpp#L338
		const eval_t threshold = alpha - 300 - (depth - 1) * 60;
		if (stand_pat < threshold) {
			
			//const eval_t q_stand_pat = new_q_search(board, 0, stats, alpha, beta);
			if (!q_stand_pat_calculated) {
				q_stand_pat = new_q_search(board, 0, ply, stats, alpha, beta);
				q_stand_pat_calculated = true;
			}
			
			if (q_stand_pat < threshold)
				return q_stand_pat;
		}
		//if (stand_pat >= beta && beta != EVAL_MAX)
		//	return stand_pat;
	}
	*/

	// Null move reduction
	if (!initially_in_check
	   && depth > NULL_MOVE_PRUNING_R(depth) + 1
	   && !is_null_move) {
		move_s move;
		construct_null_move(board, &move);
		makemove(board, &move);
		eval_t score = -zw_search(board, depth-1 - NULL_MOVE_PRUNING_R(depth), ply+1, stats, start_time, time_available, true, -beta, -alpha);
		unmakemove(board, &move);

		if (abort_search) return 0;
		
		if (score >= beta) {

			//return beta;

			
			depth -= 4;
			if (depth <= 0) {
				return eval(board) * (board->sidetomove == WHITE ? 1 : -1);
			}
			
		}
	}


	movefactory_s movefactory;
	init_movefactory(&movefactory, &killer_moves[ply], 0x0, 0);

	unsigned int n_legal_moves_done = 0;
	unsigned int n_legal_moves_skipped = 0;
	unsigned int n_legal_moves_total = 0;
	move_s* move = NULL;
	eval_t best_score = EVAL_MIN;

	for (size_t i = 0; 1; i++) {

		move = get_next_move(board, &movefactory, false);

		if (!move)
			break;
		
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
					goto ZW_SEARCH_SKIP_MOVE_PRE_MAKE;
			}
		}


		makemove(board, move);

		if (is_in_check(board, move->side)) {
			unmakemove(board, move);
			continue;
		}
		
		// Move was legal
		n_legal_moves_total++;

		bool move_was_check = is_in_check(board, board->sidetomove);

		// At depth <= 2, do not do losing captures
		if (!move_was_check
		   && !initially_in_check
		   && depth <= 2
		   && move->flags & FLAG_CAPTURE
		   && move->move_see < 0) {
			n_legal_moves_skipped++;
			unmakemove(board, move);
			continue;
		}
		// Move count based pruning
		// else if (!move_was_check
		//    && !initially_in_check
		// //    && depth <= 4
		// //    && n_legal_moves_total >= 4 + (depth)
		//    && depth <= 2
		//    && n_legal_moves_total >= 7
		//    && !(move->fromtype == PAWN
		//        && move->to & (move->side==WHITE ? TOP_THREE_ROWS : BOTTOM_THREE_ROWS)))
		// {
		// 	n_legal_moves_skipped++;
		// 	unmakemove(board, move);
		// 	continue;
		// }

		// move will be done
		n_legal_moves_done++;

		int depth_modifier = 0;

		// if (move_was_check && ply < 10 && depth < 3)
		// 	depth_modifier++;
		if (move_was_check
		  && move->flags & FLAG_CAPTURE
		  && move->move_see > 0) {
			depth_modifier++;
		}
		
		// if (move->flags & FLAG_CAPTURE
		//   && move->move_see >= 100 + (depth * 120)
		//   //&& depth <= 3
		//   && depth_modifier == 0) {
		// 	depth_modifier++;
		// }
		// if (move->flags & FLAG_CAPTURE
		//   && move->move_see <= -20 - (depth * 120)
		// //   && move->move_see <= -1000 + (10 - depth)*100
		//   && n_legal_moves_done > 4
		//   //&& depth <= 3
		//   && depth_modifier == 0) {
		// 	depth_modifier--;
		// }

		if (!initially_in_check
		   && !(move->flags & FLAG_CAPTURE)
		   && depth > 1
		   && !move_was_check
		   && !depth_modifier
		   && (move->from != killer_moves[ply][0][0]
		      || move->to != killer_moves[ply][0][1])
		   && (move->from != killer_moves[ply][1][0]
		      || move->to != killer_moves[ply][1][1])
		   && (move->from != killer_moves[ply+2][0][0]
		      || move->to != killer_moves[ply+2][0][1])
		   && (move->from != killer_moves[ply+2][1][0]
		      || move->to != killer_moves[ply+2][1][1])
		   && move->fromtype != QUEEN
		   && !(move->flags & FLAG_ENPASSANT)) {
			
			// LMR is allowed

			if (n_legal_moves_total > 5) depth_modifier -= 1;
			if (n_legal_moves_total > 10) depth_modifier -= 1;
			if (n_legal_moves_total > 20) depth_modifier -= 1;
			//if (depth <= 4 && n_legal_moves_done > 9) depth_modifier -= 1;
			//if (depth >= 4 && n_legal_moves_done > 9) depth_modifier -= 1;
			//if (depth > 7 && n_legal_moves_done > 12) depth_modifier -= 1;
		}

		ZW_SEARCH_RE_SEARCH:
		;

		eval_t score = -zw_search(board, depth-1+depth_modifier, ply+1, stats, start_time, time_available, is_null_move, -beta, -alpha);

		if (abort_search) {
			unmakemove(board, move);
			return 0;
		}


		if (score >= beta) {
			if (depth_modifier < 0) {
				depth_modifier = 0;
				goto ZW_SEARCH_RE_SEARCH;
			}
			unmakemove(board, move);

			if (!(move->flags & FLAG_CAPTURE)) {
				// If move is not already in killer, store it
				if (killer_moves[ply][0][0] != move->from || killer_moves[ply][0][1] != move->to) {
					killer_moves[ply][1][0] = killer_moves[ply][0][0];
					killer_moves[ply][1][1] = killer_moves[ply][0][1];
					killer_moves[ply][0][0] = move->from;
					killer_moves[ply][0][1] = move->to;
				}
				// move has been undone already
				if (depth > 1)
					hh_score[move->side][lowest_bitindex(move->from)][lowest_bitindex(move->to)] += 1;
			}

			return score;
		}
		else if (depth > 1 && !(move->flags & FLAG_CAPTURE))
				bf_score[move->side][lowest_bitindex(move->from)][lowest_bitindex(move->to)] += 1;


		alpha = MAX(score, alpha);
		best_score = MAX(score, best_score);


		unmakemove(board, move);
		
		ZW_SEARCH_SKIP_MOVE_PRE_MAKE:
		continue;
	}


	if (!n_legal_moves_done) {
		if (n_legal_moves_skipped) {
			// Legal moves were actually just skipped
			if (q_stand_pat_calculated)
				return q_stand_pat;
			return new_q_search(board, 0, ply+1, stats, alpha, beta);
		}
		else {
			// No legal moves
			if (initially_in_check) { // is a checkmate (was in check and can't get out of it)
				return EVAL_MIN + ply;
			}
			else { // is a stalemate (wasn't in check and no legal moves)
				return (ply % 2 == 0 ? CONTEMPT_FACTOR : -CONTEMPT_FACTOR);
			}
		}
	}

	return best_score;
}



static eval_t new_q_search(board_s* restrict board, const int qdepth, const int ply, search_stats_s* restrict stats, eval_t alpha, eval_t beta) {

	if (stats)
		stats->nodes++;
	
	if (abort_search) return 0;

	const bool initially_in_check = is_in_check(board, board->sidetomove);


	// Do not do evaluation of a position when in check
	eval_t stand_pat = EVAL_MIN;
	if (!initially_in_check) {

		stand_pat = eval(board) * (board->sidetomove == WHITE ? 1 : -1);
		
		if (stand_pat >= beta)
			return stand_pat;
		if (alpha < stand_pat)
			alpha = stand_pat;
	}


	movefactory_s movefactory;
	init_movefactory(&movefactory, NULL, 0x0, 0);

	unsigned int n_legal_moves_done = 0;
	move_s* move = NULL;
	eval_t best_score = EVAL_MIN;

	best_score = stand_pat;

	for (size_t i = 0; 1; i++) {

		// When not in check, use the qsearch movegen
		move = get_next_move(board, &movefactory, !initially_in_check);
		
		if (!move)
			break;

		// HACK: Since qsearch movegen currently returns moves that are sse >= 0, filter
		// sse == 0 out when that is used
		// TODO: NEEDS TESTING
		if (!initially_in_check && move->flags & FLAG_CAPTURE && move->move_see <= 0)
			continue;


		makemove(board, move);

		if (is_in_check(board, move->side)) {
			unmakemove(board, move);
			continue;
		}
		
		// Move was legal
		n_legal_moves_done++;
		

		eval_t score;

		score = -new_q_search(board, qdepth + 1, ply + 1, stats, -beta, -alpha);

		unmakemove(board, move);


		if (score > best_score) {
		 	if (score >= beta)
		 		return score;

		 	best_score = score;
		}
		alpha = MAX(score, alpha);
	}

	if (!n_legal_moves_done) {
		if (initially_in_check) // is a checkmate (was in check and can't get out of it and all moves were generated)
			return EVAL_MIN + ply;

		// Could have been a stalemate, but since only winning captures was generated, can't know
		assert(stand_pat != EVAL_MIN);
		return stand_pat;
	}


	return best_score;

}






// always when calling set was_horizon_node to NULL 
static eval_t regular_search(board_s* restrict board, move_s* restrict bestmove, search_stats_s* restrict stats, pv_s* restrict pv, const clock_t time1, const clock_t time_available, const int search_depth, int depth, const int actual_depth, const bool is_null_prune, eval_t alpha, eval_t beta) {
	if (stats && depth >= 0 && depth <= search_depth) {
		if (stats->n_positions) {
			stats->n_positions[stats->n_plies - depth]++;
		}
	}
	
	if (time_available) {
		if (clock() - time1 > time_available)
			return 0; // every node henceforth will also return early
	}


	// TODO: Move to its own function
	if (actual_depth) {
		// Check for repetitions
		// -2 so that current position would not trigger if statement
		for (int i = board->rep_stack_n - 2; i >= 0; i--) {
			if (board->hash == board->rep_stack[i])
				return 80 * (board->sidetomove==WHITE ? -1 : 1);
		}
	}

	
	bool tt_entry_found = false;
	//tt_entry_s tt_entry;
	uint16_t tt_entry_bestmove = 0x0;
	tt_entry_s* entry = probe_table(&tt_normal, board->hash);
	
	if (entry) {
		if (stats)
			stats->hashtable_hits++;

		// Small check for hash collisions
		if (!(SQTOBB(COMPACT_MOVE_FROM(entry->bestmove)) & board->all_pieces[board->sidetomove]))
			goto REGULAR_SEARCH_HASH_MOVE_COLLISION;
		else if (entry->bestmove & COMPACT_MOVE_PROMOTE_FLAG) {
			if (!(SQTOBB(COMPACT_MOVE_FROM(entry->bestmove)) & board->pieces[board->sidetomove][PAWN]))
				goto REGULAR_SEARCH_HASH_MOVE_COLLISION;
		}
		
		// (how many depth are supposed to be under the entry)  >= ( vs how many under this node)
		//if (entry->search_depth - entry->node_depth >= (signed int)search_depth - ((signed int)search_depth - depth) &&
		//     depth - search_depth != 0) { // and don't replace root node
		if (entry->depth >= depth - 1
		   && actual_depth
		   //actual_depth
		   && entry->flags & TT_ENTRY_FLAG_FULL_NODE
		   && (board->sidetomove == WHITE ? entry->eval <= alpha : entry->eval >= beta)) {
		   //&& (entry->eval < alpha && entry->eval > beta)) {

			if (depth <= 3 && entry->depth < depth)
				goto REGULAR_SEARCH_NO_HASH_CUT;

			if (stats)
				stats->nodes++;

			if (pv && !is_null_prune) {
				pv->n_moves[actual_depth] = 1;
				pv->n_moves[actual_depth + 1] = 0;
				pv->pv[actual_depth][0] = entry->bestmove;
			}

			// TODO: See if the bounds of the entry need to be >= the bounds of this node
			return entry->eval;
		}
		REGULAR_SEARCH_NO_HASH_CUT:
		// TODO: Figure out if this is safe when tt entry window is bigger or equal to current
		// else if (entry->depth >= depth && actual_depth) {
		// 	if (board->sidetomove == WHITE) {
		// 		if (beta <= MAX(alpha, entry->eval))
		// 			return entry->eval;
		// 	}
		// 	else {
		// 		if (MIN(beta, entry->eval) <= alpha)
		// 			return entry->eval;
		// 	}
		// }

		tt_entry_found = true;
		tt_entry_bestmove = entry->bestmove;
		// memcpy(&tt_entry, entry, sizeof (tt_entry_s));
	}
	REGULAR_SEARCH_HASH_MOVE_COLLISION:


	if (depth <= 0) {
		if (pv && !is_null_prune)
			pv->n_moves[actual_depth] = 0;
		return q_search(board, stats, search_depth, depth - 1, 0, alpha, beta);
		//return eval(board);
	}

	if (stats)
		stats->nodes++;
	
	const unsigned int initial_side = board->sidetomove;

	const bool initially_in_check = is_in_check(board, board->sidetomove);
	//const bool initially_in_check = (board->side_in_check == board->sidetomove);

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


	//bool do_null_move = false;
	if (false && depth > NULL_MOVE_PRUNING_R(depth) + 1 && !initially_in_check && actual_depth > 3 && !is_null_prune) {
		//do_null_move = true;
		
		eval_t null_score;
		makemove(board, NULL);
		if (initial_side == WHITE)
			null_score = regular_search(board, NULL, stats, pv, time1, time_available, search_depth, depth - NULL_MOVE_PRUNING_R(depth) - 1, actual_depth + 1, true, alpha, beta); //, alpha - 1, alpha);
		else
			null_score = regular_search(board, NULL, stats, pv, time1, time_available, search_depth, depth - NULL_MOVE_PRUNING_R(depth) - 1, actual_depth + 1, true, alpha, beta); //, beta, beta + 1);
		unmakemove(board, NULL);

		if (initial_side == WHITE) {
			if (null_score >= beta) {
				depth -= 4;
				if (depth <= 0) {
					if (pv && !is_null_prune)
						pv->n_moves[actual_depth] = 0;
					return q_search(board, stats, search_depth, depth - 1, 0, alpha, beta);
				}
			}
			//alpha = MAX(null_score, alpha);
		}
		else {
			if (null_score <= alpha) {
				depth -= 4;
				if (depth <= 0) {
					if (pv && !is_null_prune)
						pv->n_moves[actual_depth] = 0;
					return q_search(board, stats, search_depth, depth - 1, 0, alpha, beta);
				}
			}
			//beta = MIN(null_score, beta);
		}
	}


	//movelist_s* all_moves = calloc(n_pieces, sizeof (movelist_s));
	assert(n_pieces <= 16);

	// This being not alloc() slows search sometimes. WHY????
	// Maybe preallocate these and pass them in a struct or something?
	// TODO: Do more testing on why this is so slow
	// [from piece][move]
	move_s move_arrays[16][30];
	movelist_s all_moves[16];
	unsigned int n_all_moves = 0;
	
	// !!!!REMEMBER TO FREE!!!!
	// for tracking, which moves have already been selected (selection sort kinda)
	//bool** move_visited = calloc(n_pieces, sizeof (bool*)); //[piece_index][move_index]
	bool move_visited[16][30]; // 28 is the max movable squares for a queen, 30 for safety
	memset(&move_visited, 0, 16*30* sizeof (bool));
	

	unsigned int tt_entry_from = COMPACT_MOVE_FROM(tt_entry_bestmove);
	unsigned int tt_entry_to = COMPACT_MOVE_TO(tt_entry_bestmove);
	unsigned int tt_entry_promoteto = COMPACT_MOVE_PROMOTETO(tt_entry_bestmove);

	// used for extending when little legal moves are available
	unsigned int legal_moves_total = 0;

	// Go through every piece and create the moves
	BitBoard pieces_copy = board->all_pieces[board->sidetomove];
	for (size_t i = 0; i < n_pieces; i++) {
		all_moves[i].moves = move_arrays[i];

		get_pseudo_legal_moves(board, &all_moves[i], pop_bitboard(&pieces_copy), true, 0x0);
		
		n_all_moves += all_moves[i].n;

		//if (all_moves[i].n)
		//	move_visited[i] = calloc(all_moves[i].n, sizeof (bool));
		

		// if tt entry was found for this board, set the score for the saved move to best
		// this means that move stored in tt will always be preferred and done first
		for (size_t j = 0; j < all_moves[i].n; j++) {
			if (all_moves[i].moves[j].from == 0) {
				move_visited[i][j] = true;
				continue;
			}
			else
				legal_moves_total++;

			bool killer_found = false;
			//bool hash_move_found = false;
			if (!(all_moves[i].moves[j].flags & FLAG_CAPTURE)) {
				// Killer moves
				for (int k = 0; k < 2; k++) {
					if (all_moves[i].moves[j].from == killer_moves[actual_depth][k][0]
					   && all_moves[i].moves[j].to == killer_moves[actual_depth][k][1]
					   && !killer_found) {
						all_moves[i].moves[j].move_score += 30;
						killer_found = true;
						break;
					}
					if (actual_depth > 1) {
						if (all_moves[i].moves[j].from == killer_moves[actual_depth-2][k][0]
						   && all_moves[i].moves[j].to == killer_moves[actual_depth-2][k][1]
						   && !killer_found) {
							all_moves[i].moves[j].move_score += 20;
							killer_found = true;
							break;
						}
					}
					if (all_moves[i].moves[j].from == killer_moves[actual_depth+2][k][0]
					   && all_moves[i].moves[j].to == killer_moves[actual_depth+2][k][1]
					   && !killer_found) {
							all_moves[i].moves[j].move_score += 10;
							killer_found = true;
							break;
					}
				}
				// History moves
				// if (depth > 1 && actual_depth && !killer_found)
				// 	all_moves[i].moves[j].move_score += history_moves[initial_side][lowest_bitindex(all_moves[i].moves[j].from)][lowest_bitindex(all_moves[i].moves[j].to)] / (2*(actual_depth*actual_depth));
				//if (!killer_found && bf_score[initial_side][lowest_bitindex(all_moves[i].moves[j].from)][lowest_bitindex(all_moves[i].moves[j].to)] > 0) {
				if (bf_score[initial_side][lowest_bitindex(all_moves[i].moves[j].from)][lowest_bitindex(all_moves[i].moves[j].to)] > 0) {
					const eval_t hscore = hh_score[initial_side][lowest_bitindex(all_moves[i].moves[j].from)][lowest_bitindex(all_moves[i].moves[j].to)] / bf_score[initial_side][lowest_bitindex(all_moves[i].moves[j].from)][lowest_bitindex(all_moves[i].moves[j].to)];
					all_moves[i].moves[j].move_score += hscore; // Cap the rel. history score to 5
				}
			}
			if (pv) {
				if (pv->n_moves[actual_depth]) {
					if (COMPACT_MOVE_FROM(pv->pv[actual_depth][0]) == all_moves[i].moves[j].from
					   && COMPACT_MOVE_TO(pv->pv[actual_depth][0]) == all_moves[i].moves[j].to) {
						all_moves[i].moves[j].move_score += 40;
					}
				}
			}
			if (tt_entry_found) {
				assert(tt_entry_bestmove);

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

	/*
	if ((depth == 3) && ((board->sidetomove==WHITE ? alpha : beta) != EVAL_MIN && (board->sidetomove==WHITE ? alpha : beta) != EVAL_MAX)) {
		const eval_t q_standpat = q_search(board, stats, search_depth, depth - 1, 0, alpha, beta);
		if (board->sidetomove == WHITE) {
			if (q_standpat <= alpha - 500)
				depth -= 1;
			else if (q_standpat >= beta)
				return q_standpat;
		}
		else {
			if (q_standpat >= beta + 500)
				depth -= 1;
			else if (q_standpat <= alpha)
				return q_standpat;
		}
	}
	*/

	// Now all moves have been generated and checked for hash- and killer moves
	
	if (stats)
		stats->n_moves_generated += n_all_moves;
	
	move_s* best_move_here = NULL;

	bool best_move_here_is_pv = true;

	eval_t last_best_move = EVAL_MAX;

	for (unsigned int i = 0; i < n_all_moves; i++) {
		if (actual_depth < 5 && time_available) {
			if (clock() - time1 > time_available)
				return 0; // every node henceforth will also return early
		}
		// if (time_available && i % 3 == 0) {
		// 	if (clock() - time1 > time_available)
		// 		return 0; // every node henceforth will also return early
		// }

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

		if (!move)
			break;

		assert(move);
		assert(piece_index != UINT_MAX);
		assert(move_index != UINT_MAX);

		assert(move->from);

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

		makemove(board, move);
		//append_to_move_history(board, move);

		// worst eval by default
		eval_t move_eval = better_eval(EVAL_MAX, EVAL_MIN, OPPOSITE_SIDE(initial_side));
		
		// check if that side got itself in check (or couldn't get out of one)
		//if (move->fromtype == KING) {
		if (is_in_check(board, initial_side)) {
			skipped_because_of_checks++;
			goto REGULAR_SEARCH_SKIP_MOVE;
		}
		//}

		n_legal_moves_done++;

		// --- MOVE WAS LEGAL AND IS DONE ---
	
		// Skip all promotions that weren't queen or
		// knight as other pieces are just basically just a worse queen
		// (Maybe a promotion to a lesser piece could be used to force opponent into zuzwang)
		if (move->flags & FLAG_PROMOTE) {
			if (move->promoteto != QUEEN && move->promoteto != KNIGHT)
				goto REGULAR_SEARCH_SKIP_MOVE;
		}

		int depth_modifier = 0;

		// if (move->move_score == EVAL_MAX && actual_depth < search_depth - 4 && depth < 5)
		// 	depth_modifier++;

		bool move_was_check = is_in_check(board, board->sidetomove);

		if (move_was_check && actual_depth + depth <= search_depth + 4 && depth < 3)
			depth_modifier++;
		
		//if (n_legal_moves_done < 3 && actual_depth == 3) // actual_depth < search_depth - 1 && search_depth > depth - 1 && actual_depth % 2)
		//	depth_modifier++;
		// if (n_legal_moves_done < 4 && actual_depth == 4)
		// 	depth_modifier++;
		// if (n_legal_moves_done < 1 && actual_depth == 6)
		// 	depth_modifier++;
		// if (n_legal_moves_done < 2 && search_depth - actual_depth == 1)
		// 	depth_modifier++;

		if (move->flags & FLAG_PROMOTE) {
			if (move->promoteto == QUEEN)
				depth_modifier++; //+= 1 + (actual_depth % 2);
			else if (move->promoteto == KNIGHT)
				depth_modifier++;
		}
		else if (move->fromtype == PAWN && depth < 3) {
			if (move->from & (move->side==WHITE ? Q_SEARCH_PAWN_SELECT_MASK_W : Q_SEARCH_PAWN_SELECT_MASK_B))
				depth_modifier++;
		}

		if (depth == 3
			&& move->flags & FLAG_CAPTURE
			&& move->move_see < 0
			&& depth_modifier <= 0
			&& move->move_score != EVAL_MAX) {
			depth_modifier--;
		}

		//const unsigned int lmr_treshold = 5; //(actual_depth%2 == 0 ? 7 : 10);
		//const int lmr_reduction = ((n_legal_moves_done > lmr_treshold + 5 ) ? (depth / 5) : (1));
		//int lmr_reduction = ((n_legal_moves_done > lmr_treshold + 5 ) ? (2) : (1));
		//if (depth > 10) lmr_reduction += lmr_reduction;
		//else if (depth > 8) lmr_reduction += 1;

		//const unsigned int lmr_treshold = 1;
		//int lmr_reduction = ((n_legal_moves_done > lmr_treshold + 5 ) ? (2) : (1));

		const unsigned int lmr_treshold = 1;
		int lmr_reduction = ((n_legal_moves_done > lmr_treshold + 3 ) ? (2) : (1));
		//if (depth > 5 && n_legal_moves_done > lmr_treshold + 3 + 7) lmr_reduction = (3);
		//if (depth > 8 && n_legal_moves_done > lmr_treshold + 3 + 4 + 5) lmr_reduction = 4;
		//if (depth > 8) lmr_reduction++;

		//int lmr_reduction = 1;
		if (n_legal_moves_done > lmr_treshold
			&& !initially_in_check
			&& !depth_modifier
			&& (move->from != killer_moves[actual_depth][0][0]
				|| move->to != killer_moves[actual_depth][0][1])
			&& (move->from != killer_moves[actual_depth][1][0]
				|| move->to != killer_moves[actual_depth][1][1])
			&& (move->from != killer_moves[actual_depth+2][0][0]
				|| move->to != killer_moves[actual_depth+2][0][1])
			&& (move->from != killer_moves[actual_depth+2][1][0]
				|| move->to != killer_moves[actual_depth+2][1][1])
			&& move) {
			
			if (move->fromtype != QUEEN && !(move->flags & FLAG_CAPTURE) && !(move->flags & FLAG_PROMOTE) && !(move->flags & FLAG_ENPASSANT) && actual_depth >= 1 && depth > 2) { //&& depth < 4) { // && (actual_depth == 4 || actual_depth == 5)) { //&& actual_depth > 2 && depth < 3) {
				// Check if the move gave check
				if (!move_was_check) {
					depth_modifier = -lmr_reduction;
					//const eval_t early_return_eval = q_search(board, stats, search_depth, depth, 0, alpha, beta);//depth_modifier = -lmr_reduction;
					//unmakemove(board, move);
					//break;
					//return early_return_eval;
				}
				// else if (actual_depth < search_depth && depth < 5)
				// 	depth_modifier += 1;
			}
		}

		//if (depth_modifier == 0 && depth > 2 && (board->sidetomove==WHITE ? (eval(board) < beta) : eval(board) > alpha))
		//	depth_modifier--;
		

		//if (depth < 2 && move->flags & FLAG_CAPTURE && move->move_see < 0 && depth_modifier == 0) {
		//	//unmakemove(board, move);
		//	//return q_search(board, stats, search_depth, depth - 1, 0, alpha, beta);
		//	depth_modifier--;
		//}

		// if (actual_depth == 3) {
		// 	// (-q_search) becouse move is already made
		// 	const eval_t qscore = -q_search(board, stats, search_depth, depth - 1, 0, alpha, beta);

		// 	if (initial_side == WHITE) {
		// 		if (qscore < alpha - 100)
		// 			depth_modifier -= 2;
		// 	}
		// 	else {
		// 		if (qscore > beta + 100)
		// 			depth_modifier -= 2;
		// 	}

		// }

		
		

		// if (depth_modifier <= 0 && actual_depth < search_depth - 1) { // && depth < 2) {
		// 	if (is_in_check(board, board->sidetomove))
		// 		depth_modifier += 1;
		// }

		// if (depth_modifier != 0) {
		// 	if (depth - 1 + depth_modifier <= 0 && move->move_score < 400 && best_move_here)
		// 		goto REGULAR_SEARCH_SKIP_MOVE;
		// }

		// A leaf node will never be in check.
		//if (initially_in_check && depth <= 1)
		//	depth_modifier = 1;

		bool move_is_pv;

		if (depth_modifier < 0 && depth > 2 && actual_depth > 0) {
			eval_t reduced_result;
			//if (is_null_prune)
			reduced_result = regular_search(board, NULL, stats, pv, time1, time_available, search_depth, depth-1+depth_modifier, actual_depth + 1, is_null_prune, alpha, beta);
			if (initial_side == WHITE) {
				//if (!is_null_prune)
				//	reduced_result = regular_search(board, NULL, stats, pv, time1, time_available, search_depth, depth-1+depth_modifier, actual_depth + 1, true, alpha, alpha+1);
				// if (reduced_result != EVAL_MAX && reduced_result != EVAL_MIN)
				// 	reduced_result += 2;
				if (reduced_result >= alpha) {
					move_is_pv = true;
					// TODO: test depth - 1 + (actual_depth%4 == 0)
					if (false && reduced_result >= alpha + 20 && alpha != EVAL_MAX)
						depth_modifier = 1;
					else
						depth_modifier = 0;
					
					move_eval = regular_search(board, NULL, stats, pv, time1, time_available, search_depth, depth-1+depth_modifier, actual_depth + 1, is_null_prune, alpha, beta);
				}
				else {
					move_is_pv = false;
					move_eval = reduced_result;
				}
			}
			else {
				//if (!is_null_prune)
				//	reduced_result = regular_search(board, NULL, stats, pv, time1, time_available, search_depth, depth-1+depth_modifier, actual_depth + 1, true, beta-1, beta);
				// if (reduced_result != EVAL_MAX && reduced_result != EVAL_MIN)
				// 	reduced_result += 2;
				if (reduced_result <= beta) {
					move_is_pv = true;

					if (false && reduced_result <= beta - 20 && beta != EVAL_MIN)
						depth_modifier = 1;
					else
						depth_modifier = 0;
					
					move_eval = regular_search(board, NULL, stats, pv, time1, time_available, search_depth, depth-1+depth_modifier, actual_depth + 1, is_null_prune, alpha, beta);
				}
				else {
					move_is_pv = false;
					move_eval = reduced_result;
				}
			}
		}
		else {
			move_is_pv = true;
			move_eval = regular_search(board, NULL, stats, pv, time1, time_available, search_depth, depth-1+depth_modifier, actual_depth + 1, is_null_prune, alpha, beta);
		}

		// if (time_available && i % 3 == 0) {
		// 	if (clock() - time1 > time_available)
		// 		return 0; // every node henceforth will also return early
		// }
		
		//printf("info string %f\n", eval);

		// if (eval != EVAL_MAX && eval != EVAL_MIN) {
		// 	// Trust reduced nodes less and extended nodes more
		// 	if (depth_modifier < 0 && depth > 2 && actual_depth > 2)
		// 		eval -= better_eval(-40, 40, initial_side);
		// 	else if (depth_modifier > 0 && depth > 2 && actual_depth > 2)
		// 		eval -= better_eval(-40, 40, initial_side);
		// }

		// if move was better, store it instead.
		// Or if no legal moves have been stored to yet; store it now
		if (is_eval_better(move_eval, bestmove_eval, initial_side) || !best_move_here) {
			assert(popcount(move->from) == 1);
			assert(popcount(move->to) == 1);
			bestmove_eval = move_eval;
			best_move_here_is_pv = move_is_pv;

			if (bestmove)
				memcpy(bestmove, move, sizeof (move_s));
			
			best_move_here = move;
			
			if (move_is_pv) {
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
			else
				pv->n_moves[actual_depth] = 0;
		}

		eval_t killer_store_treshold; // will be equivelant to nega-max beta

		// alpha is maximizing and beta is minimizing
		if (initial_side == WHITE) {
			alpha = better_eval(alpha, move_eval, WHITE);
			killer_store_treshold = beta;
		}
		else {
			beta = better_eval(beta, move_eval, BLACK);
			killer_store_treshold = alpha;
		}
		
		if (beta <= alpha) {
			fail_low = true;
			if (stats && depth > 0 && depth <= search_depth) {
				if (stats->fail_hard_cutoffs)
					stats->fail_hard_cutoffs[stats->n_plies - depth]++;
			}


			if (move && !is_null_prune) {
				if (!(move->flags & FLAG_CAPTURE)) {
					if (is_eval_better(move_eval, killer_store_treshold, initial_side)) {
						// If move is not already in killer, store it
						if (killer_moves[actual_depth][0][0] != move->from || killer_moves[actual_depth][0][1] != move->to) {
							killer_moves[actual_depth][1][0] = killer_moves[actual_depth][0][0];
							killer_moves[actual_depth][1][1] = killer_moves[actual_depth][0][1];
							killer_moves[actual_depth][0][0] = move->from;
							killer_moves[actual_depth][0][1] = move->to;
						}
					}
					if (depth > 1)
						hh_score[initial_side][lowest_bitindex(move->from)][lowest_bitindex(move->to)] += 1;
				}
			}

			unmakemove(board, move);
			break; // at least 1 legal move has to have been made so we can just break search
		}
		else if (move && !is_null_prune && depth > 1 && actual_depth) {
			if (!(move->flags & FLAG_CAPTURE))
				bf_score[initial_side][lowest_bitindex(move->from)][lowest_bitindex(move->to)] += 1;
		}
		
		// if (pv && !is_null_prune && move != NULL && move == best_move_here && move_is_pv) {
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
		unmakemove(board, move);
		REGULAR_SEARCH_SKIP_MOVE_PRE_MAKE:
		continue;
	}

	assert(board->every_piece == every_piece_copy);

	if (time_available) {
		if (clock() - time1 > time_available)
			return 0; // every node henceforth will also return early
	}

	// alpha-beta pruning
	if (fail_low) {
		if (pv && !is_null_prune)
			pv->n_moves[actual_depth] = 0;
		
		if (pv && !is_null_prune && best_move_here && (bestmove_eval == EVAL_MIN || bestmove_eval == EVAL_MAX)) {
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
		// else
		// 	pv->n_moves[actual_depth] = 0;
		
		if (!is_null_prune && best_move_here)
			store_move(&tt_normal, board->hash, bestmove_eval, 0x0, depth, encode_compact_move(best_move_here), false);

		//return bestmove_eval;

		if (initial_side == WHITE)
			return alpha;
		else
			return beta;
	}

	// No moves were made??
	if (!n_legal_moves_done) {
		
		if (pv && !is_null_prune)
			pv->n_moves[actual_depth] = 0;
		
		if (initially_in_check) { // is a checkmate (was in check and can't get out of it)
			// store the worst value possible for checkmate
			return (is_eval_better(EVAL_MAX, EVAL_MIN, board->sidetomove) ? EVAL_MIN : EVAL_MAX);
		}
		else { // is a stalemate (wasn't in check and no legal moves)
			return 80 * (board->sidetomove==WHITE ? 1 : -1); //return 0;
		}
		//goto REGULAR_SEARCH_SEARCH_END;
		//return bestmove_eval;
		//store_move(&tt_normal, board->hash, bestmove_eval, search_depth, (search_depth - depth), )
	}

	if (false && pv && !is_null_prune && best_move_here) {
		if (best_move_here_is_pv) {
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
		else
			pv->n_moves[actual_depth] = 0;
	}

	if (!is_null_prune && best_move_here)
		store_move(&tt_normal, board->hash, bestmove_eval, 0x0, depth, encode_compact_move(best_move_here), best_move_here_is_pv);

	return bestmove_eval;
}



// qdepth starts always at 0 and is incremented every qnode.
static eval_t q_search(board_s* restrict board, search_stats_s* restrict stats, const unsigned int search_depth, const int depth, unsigned int qdepth, eval_t alpha, eval_t beta) {

	if (stats)
		stats->nodes++;


	/*
	// Looking up entries here slows search around 300knps
	// TODO: Do more testing in future for this
	if (qdepth > 0) {
		const tt_entry_s* entry = probe_table(&tt_normal, board->hash);

		if (entry) {
			if (stats) {
				//stats->nodes++;
				stats->hashtable_hits++;
			}
			return entry->eval;
		}
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

	const eval_t eval_here = eval(board);


	const bool initially_in_check = is_in_check(board, board->sidetomove);

	/*
	int stand_pat = Evaluate();
    if( stand_pat >= beta )
        return beta;
    if( alpha < stand_pat )
        alpha = stand_pat;
	*/
	
	// Above is code from https://www.chessprogramming.org/Quiescence_Search
	// And below is that but adapted to minimax
	if (board->sidetomove == WHITE) {
		if (eval_here + 1000 <= alpha && qdepth > 0) // (eval_here >= beta && eval_here <= alpha)
		 	return eval_here;
		if (eval_here >= beta)
			return eval_here;
		if (alpha < eval_here && !initially_in_check)
			alpha = eval_here;
	}
	else {
		if (eval_here - 1000 >= beta && qdepth > 0)//(eval_here <= alpha && eval_here >= beta)
		 	return eval_here;
		if (eval_here <= alpha)
			return eval_here;
		if (beta > eval_here && !initially_in_check)
			beta = eval_here;
	}
	


	const unsigned int initial_side = board->sidetomove;

	// for checking if there were any legal moves in this position for board->sidetomove
	unsigned int n_legal_moves_done = 0;

	// Stores the best move for board->sidetomove
	eval_t bestmove_eval = eval_here;
	if (initially_in_check) // never eval() in check
		bestmove_eval = (board->sidetomove == WHITE ? EVAL_MIN : EVAL_MAX); // set to worst possible

	// I think the right term is fail-low?? Not sure
	// This set to true if beta <= alpha
	bool fail_low = false;

	// N moves were found that don't fit the prune constraits
	unsigned int n_fail_q_prune = 0;

#ifndef NDEBUG
	BitBoard every_piece_copy = board->every_piece;
#endif // NDEBUG
	
	
	const unsigned int n_pieces = popcount(board->all_pieces[board->sidetomove]);
	
	// for tracking, which moves have already been selected (selection sort kinda)
	bool move_visited[16][30]; // 28 is the max movable squares for a queen, 30 for safety
	memset(&move_visited, 0, 16*30 * sizeof (bool));

	move_s move_arrays[16][30];
	movelist_s all_moves[16];

	unsigned int n_all_moves = 0;

	// Go through every piece and create the moves
	BitBoard pieces_copy = board->all_pieces[board->sidetomove];
	for (size_t i = 0; i < n_pieces; i++) {
		all_moves[i].moves = move_arrays[i];

		get_pseudo_legal_moves(board, &all_moves[i], pop_bitboard(&pieces_copy), true, 0x0);

		n_all_moves += all_moves[i].n;

		for (size_t j = 0; j < all_moves[i].n; j++) {
			if (all_moves[i].moves[j].from == 0) {
				move_visited[i][j] = true;
				continue;
			}
		}
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

		if (!move)
			break;

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

			//ssert(-(eval_t)depth - 1 >= 0);

			if (move->move_see > 0)
				failed_q_prune = false;
			
			// if (move->move_see < -100)
			// 	failed_q_prune = true;
			// if (board->sidetomove == WHITE) {
			// 	const int phase = get_game_phase_value(board);
			// 	if (eval_material(board, phase) + move->move_see + Q_SEARCH_PRUNE_TRESHOLD > alpha)
			// 		failed_q_prune = false;
			// }
			// else {
			// 	const int phase = get_game_phase_value(board);
			// 	if (eval_material(board, phase) - move->move_see - Q_SEARCH_PRUNE_TRESHOLD < beta)
			// 		failed_q_prune = false;
			// }
			

			//if (move->piece_captured == PAWN && move->to & (move->side == BLACK ? Q_SEARCH_PAWN_SELECT_MASK_W : Q_SEARCH_PAWN_SELECT_MASK_B))
			//	failed_q_prune = false;
			//else if (move->fromtype == KING)
			//	failed_q_prune = false;
		}
		if (move->flags & FLAG_PROMOTE && (move->promoteto == QUEEN || move->promoteto == KNIGHT))
			failed_q_prune = false;
		if (move->flags & FLAG_ENPASSANT)
			failed_q_prune = false;
		// else
		// 	failed_q_prune = true;
		
		//if (move->fromtype == PAWN && move->from & (move->side == WHITE ? Q_SEARCH_PAWN_SELECT_MASK_W : Q_SEARCH_PAWN_SELECT_MASK_B))
		//	failed_q_prune = false;
		
		makemove(board, move);

		// worst eval by default
		eval_t eval = better_eval(EVAL_MAX, EVAL_MIN, OPPOSITE_SIDE(initial_side));
		
		// check if that side got itself in check (or couldn't get out of one)
		//if (move->fromtype == KING) {
		if (is_in_check(board, initial_side))
			goto Q_SEARCH_SKIP_MOVE;
		//}

		n_legal_moves_done++;

		// --- MOVE WAS LEGAL AND IS DONE ---

		if (stats)
			stats->nodes++;


		// Quiescence pruning after move-make
		
		// Do moves that check the king and threaten material
		if (failed_q_prune) {
			// See if piece threatens more than one piece (king and something else)
			// No need to check for promotions here
			if (popcount(board->all_pieces[board->sidetomove] & get_pseudo_legal_squares(board, move->side, move->fromtype, move->to, true)) > 1) {
				if (is_in_check(board, board->sidetomove)) {
					failed_q_prune = false;
				}
			}
		}

		// Check evasions
		if (initially_in_check)
			failed_q_prune = false;


		if (failed_q_prune) {
			unmakemove(board, move);
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
			unmakemove(board, move);
			break; // at least 1 legal move has to have been made so we can just break search
		}


		Q_SEARCH_SKIP_MOVE:
		unmakemove(board, move);
	}

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
		return eval_here; //eval(board);
	}
	// if (bestmove_eval == (initial_side == WHITE ? EVAL_MIN : EVAL_MAX))
	// 	return eval(board);
	
	//return (board->sidetomove == WHITE ? alpha : beta);
	return bestmove_eval;
}

