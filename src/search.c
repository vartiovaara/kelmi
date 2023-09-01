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
static eval_t search_root_node(board_s* restrict board, move_s* restrict bestmove, search_stats_s* restrict stats, pv_s* restrict pv, const clock_t time1, const clock_t time_available, const int depth, const eval_t last_eval);
static eval_t pv_search(board_s* restrict board, int depth, const int ply, search_stats_s* restrict stats, pv_s* restrict pv, const clock_t start_time, const clock_t time_available, const bool is_leftmost_path, eval_t alpha, eval_t beta);
static eval_t zw_search(board_s* restrict board, int depth, const int ply, search_stats_s* restrict stats, const clock_t start_time, const clock_t time_available, const bool is_null_move, eval_t alpha, eval_t beta);
static eval_t q_search(board_s* restrict board, const int qdepth, const int ply, search_stats_s* restrict stats, eval_t alpha, eval_t beta);





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

		
		// Follow the pv and then follow the hash table for longer pv's
		//move_s pv_moves[128];
		uint16_t pv_compact_moves[128];
		size_t n_pv_moves = 0;

		for (size_t i = 0; i < pv.n_moves[0]; i++) {

			pv_compact_moves[n_pv_moves] = pv.pv[0][i];
			
			//create_move(board, &pv_moves[n_pv_moves], SQTOBB(COMPACT_MOVE_FROM(pv.pv[0][i])), SQTOBB(COMPACT_MOVE_TO(pv.pv[0][i])), COMPACT_MOVE_PROMOTETO(pv.pv[0][i]));
			//makemove(board, &pv_moves[n_pv_moves]);
			
			n_pv_moves++;
		}

		/*
		// Follow the hash table
		
		tt_entry_s* seen_tt_entries[128];
		size_t n_seen_tt_entries = 0;

		for (size_t i = n_pv_moves; i < 128; i++) {
			
			tt_entry_s* tt_entry = probe_table(&tt_normal, board->hash);
			
			// Check if this entry has been seen before
			bool circular_path_found = false;
			for (int j = n_seen_tt_entries-1; j >= 0; j--) {
				if (tt_entry == seen_tt_entries[j]) {
					circular_path_found = true;
					break;
				}
			}
			if (circular_path_found) break;

			seen_tt_entries[n_seen_tt_entries++] = tt_entry;

			if (!tt_entry) break;
			if (!tt_entry->bestmove) break;
			if (!(tt_entry->flags & TT_ENTRY_FLAG_EXACT)) break;

			pv_compact_moves[n_pv_moves] = tt_entry->bestmove;
			create_move(board, &pv_moves[n_pv_moves], SQTOBB(COMPACT_MOVE_FROM(tt_entry->bestmove)), SQTOBB(COMPACT_MOVE_TO(tt_entry->bestmove)), COMPACT_MOVE_PROMOTETO(tt_entry->bestmove));
			makemove(board, &pv_moves[n_pv_moves]);
			n_pv_moves++;
		}

		// Undo the moves
		for (int i = n_pv_moves-1; i >= 0; i--)
			unmakemove(board, &pv_moves[i]);
		*/



		char pv_str[1024];
		size_t current_str_index = 0;
		
		// Add collected moves to pv string
		for (size_t i = 0; i < n_pv_moves; i++) {
			char move[6];
			compact_move_to_uci_notation(pv_compact_moves[i], move);
			strcpy(pv_str + current_str_index, move);
			current_str_index += strlen(move);
			pv_str[current_str_index++] = ' ';
		}
		pv_str[current_str_index] = '\0';


		bool move_is_mate = EVAL_IS_MATE(move_eval);
		eval_t mate_in_n = (move_eval > 0 ? (MATE - move_eval) : (-MATE - move_eval));
		
		// if mate-in-n-ply is not even, the last ply will be counted as a full move
		if (mate_in_n % 2 != 0)
			mate_in_n += (mate_in_n > 0 ? 1 : -1);
		
		mate_in_n /= 2; // plies to moves
		
		const unsigned long long nps = (unsigned long long)((double)stats.nodes/((double)(clock() - last_search_time)/(double)CLOCKS_PER_SEC));
		const unsigned int search_time = (unsigned int)(((double)(clock() - t)/(double)CLOCKS_PER_SEC)*1000);
		uci_write(f, "info depth %i score %s %i time %u nodes %llu nps %d pv %s\n", depth, (move_is_mate ? "mate" : "cp"), (move_is_mate ? mate_in_n : move_eval), search_time, stats.nodes, nps, pv_str);

		// mate
		if (((move_is_mate) && depth > 70) || depth >= MAX_DEPTH)
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
	eval_t best_score = -EVAL_INF;

	eval_t alpha = -EVAL_INF, beta = EVAL_INF;

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
			return -MATE;
		}
		else { // is a stalemate (wasn't in check and no legal moves)
			return 0;
		}
	}

	return best_score;
}


static eval_t pv_search(board_s* restrict board, int depth, const int ply, search_stats_s* restrict stats, pv_s* restrict pv, const clock_t start_time, const clock_t time_available, const bool is_leftmost_path, eval_t alpha, eval_t beta) {

	if (time_available && (depth > 2 || ply <= 3)) {
			if (clock() - start_time >= time_available) {
				abort_search = true;
				return 0; // every node henceforth will also return early
			}
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


	bool tt_entry_found = false;
	uint8_t tt_entry_flags = 0x0;
	uint16_t tt_entry_bestmove = 0x0;
	tt_entry_s* entry = probe_table(&tt_normal, board->hash);
	
	if (entry) {
		if (stats)
			stats->hashtable_hits++;

		// Small check for hash collisions
		if (!(SQTOBB(COMPACT_MOVE_FROM(entry->bestmove)) & board->all_pieces[board->sidetomove]))
			goto PV_SEARCH_HASH_MOVE_COLLISION;
		else if (entry->bestmove & COMPACT_MOVE_PROMOTE_FLAG) {
			if (!(SQTOBB(COMPACT_MOVE_FROM(entry->bestmove)) & board->pieces[board->sidetomove][PAWN]))
				goto PV_SEARCH_HASH_MOVE_COLLISION;
		}

		// Correct the entry eval by shifting mate values towards zero by ply

		eval_t entry_eval = entry->eval;

		if (EVAL_IS_WIN(entry_eval))
			entry_eval -= ply;
		else if (EVAL_IS_LOSE(entry_eval))
			entry_eval += ply;

		if (entry->search_depth >= depth) {

			if (entry->flags & TT_ENTRY_FLAG_EXACT) {

				if (stats)
					stats->nodes++;

				if (pv) {
					pv->n_moves[ply] = 1;
					pv->n_moves[ply + 1] = 0;
					pv->pv[ply][0] = entry->bestmove;
				}

				return entry_eval;
			}
			else if (entry->flags & TT_ENTRY_FLAG_FAIL_HIGH
			   && entry_eval >= beta) {
				
				// Score is lower bound

				if (pv)
					pv->n_moves[ply] = 0;
				return entry_eval;
			}
			else if (entry->flags & TT_ENTRY_FLAG_FAIL_LOW
			   && entry_eval <= alpha) {

				// Score is higher bound

				if (pv)
					pv->n_moves[ply] = 0;
				return entry_eval;
			}
		}
		//PV_SEARCH_NO_HASH_CUT:

		tt_entry_found = true;
		tt_entry_bestmove = entry->bestmove;
		tt_entry_flags = entry->flags;
	}
	PV_SEARCH_HASH_MOVE_COLLISION:



	// Extensions at the leaves

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
		return q_search(board, 0, ply, stats, alpha, beta);
	}





	if (stats)
		stats->nodes++;



	// Internal Iterative Deepening

	if (depth >= 4 && !tt_entry_found) {

		int new_depth = depth - 2;
		if (depth > 6) new_depth--;
		//ASSERT(new_depth>0);

		eval_t value = pv_search(board, new_depth, ply, stats, pv, start_time, time_available, false, alpha, beta);
		if (value <= alpha && !abort_search) value = pv_search(board, new_depth, ply, stats, pv, start_time, time_available, false, -EVAL_INF, beta);

		if (abort_search)
			return 0;


		if (pv->n_moves[ply] > 0) {
			tt_entry_found = true;
			tt_entry_bestmove = pv->pv[ply][0];
			pv->n_moves[ply] = 0;
		}

	}






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
	if (tt_entry_found) {
		// Make sure that the pv and hash move is not the same
		if (!(n_special_moves == 1 && tt_entry_bestmove == special_moves[0]))
			special_moves[n_special_moves++] = tt_entry_bestmove;
	}


	movefactory_s movefactory;
	init_movefactory(&movefactory, &killer_moves[ply], special_moves, n_special_moves);

	unsigned int n_legal_moves_done = 0;
	move_s* move = NULL;
	eval_t best_score = -EVAL_INF;
	move_s* best_move = NULL;
	bool best_move_is_fail_low = true;

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

		if (move_was_check && ply < 20)
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


		eval_t score = -EVAL_INF;

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


		if (score > best_score) {
			best_score = score;
			best_move = move;
			best_move_is_fail_low = (score <= alpha);
		}

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

			store_move(&tt_normal, board->hash, score, depth, encode_compact_move(move),  TT_ENTRY_FLAG_FAIL_HIGH | TT_ENTRY_FLAG_PV_NODE, ply);

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

		PV_SEARCH_SKIP_MOVE_PRE_MAKE:
		continue;
	}

	if (!n_legal_moves_done) {
		if (pv)
			pv->n_moves[ply] = 0;
		if (initially_in_check) { // is a checkmate (was in check and can't get out of it)
			return -MATE + ply;
		}
		else { // is a stalemate (wasn't in check and no legal moves)
			return (ply % 2 == 0 ? CONTEMPT_FACTOR : -CONTEMPT_FACTOR);
		}
	}


	if (best_move_is_fail_low) {
		if (pv)
			pv->n_moves[ply] = 0;
		store_move(&tt_normal, board->hash, best_score, depth, encode_compact_move(best_move), TT_ENTRY_FLAG_FAIL_LOW | TT_ENTRY_FLAG_PV_NODE, ply);
	}
	else
		store_move(&tt_normal, board->hash, best_score, depth, encode_compact_move(best_move), TT_ENTRY_FLAG_EXACT | TT_ENTRY_FLAG_PV_NODE, ply);

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


	// Check for repetitions
	// -2 so that current position would not trigger if statement
	//const size_t lower_rep_stack_limit = MAX(((int)board->rep_stack_n) - 2 - 4, 0); // search max 4 moves back
	for (int i = board->rep_stack_n - 2; i >= 0; i--) {
		if (board->hash == board->rep_stack[i])
			return (ply % 2 == 0 ? CONTEMPT_FACTOR : -CONTEMPT_FACTOR);
	}



	bool tt_entry_found = false;
	uint16_t tt_entry_bestmove = 0x0;
	tt_entry_s* entry = probe_table(&tt_normal, board->hash);
	
	if (entry) {
		if (stats)
			stats->hashtable_hits++;

		// Small check for hash collisions
		if (!(SQTOBB(COMPACT_MOVE_FROM(entry->bestmove)) & board->all_pieces[board->sidetomove]))
			goto ZW_SEARCH_HASH_MOVE_COLLISION;
		else if (entry->bestmove & COMPACT_MOVE_PROMOTE_FLAG) {
			if (!(SQTOBB(COMPACT_MOVE_FROM(entry->bestmove)) & board->pieces[board->sidetomove][PAWN]))
				goto ZW_SEARCH_HASH_MOVE_COLLISION;
		}

		// Correct the entry eval by shifting mate values towards zero by ply

		eval_t entry_eval = entry->eval;

		if (EVAL_IS_WIN(entry_eval))
			entry_eval -= ply;
		else if (EVAL_IS_LOSE(entry_eval))
			entry_eval += ply;

		if (entry->search_depth >= depth) {
			if (entry->flags & TT_ENTRY_FLAG_EXACT) {

				if (stats)
					stats->nodes++;
				
				return entry_eval;
			}
			else if (entry->flags & TT_ENTRY_FLAG_FAIL_HIGH
			   && entry_eval >= beta) {
				
				// Score is lower bound
				
				if (stats)
					stats->nodes++;

				return entry_eval;
			}
			else if (entry->flags & TT_ENTRY_FLAG_FAIL_LOW
			   && entry_eval <= alpha) {

				// Score is higher bound

				if (stats)
					stats->nodes++;
				
				return entry_eval;
			}
		}
		//Zw_SEARCH_NO_HASH_CUT:

		tt_entry_found = true;
		tt_entry_bestmove = entry->bestmove;
	}
	ZW_SEARCH_HASH_MOVE_COLLISION:








	if (depth <= 0) {
		return q_search(board, 0, ply, stats, alpha, beta);
	}

	if (stats)
		stats->nodes++;


	const bool initially_in_check = is_in_check(board, board->sidetomove);


	bool q_stand_pat_calculated = false;
	eval_t q_stand_pat;
	
	if (depth == 2
	   && !initially_in_check
	   && !EVAL_IS_LOSE(alpha)) {

		//const eval_t stand_pat = eval(board) * (board->sidetomove == WHITE ? 1 : -1);
		if (!q_stand_pat_calculated) {
			q_stand_pat = q_search(board, 0, ply, stats, alpha, beta);
			q_stand_pat_calculated = true;

			if (EVAL_IS_MATE(q_stand_pat)) return q_stand_pat;
		}
		
		if (q_stand_pat + 500 < alpha)
			return q_stand_pat;
	}

	
	// Reverse futility pruning

	if (depth > 1 // 2
	   && !initially_in_check
	   && !EVAL_IS_LOSE(beta)
	   && false) {

		const eval_t margin = depth * 95 + ((depth-2) * 20);
		// const eval_t margin = depth * 90;

		if (!q_stand_pat_calculated) {
			q_stand_pat = q_search(board, 0, ply, stats, alpha, beta);
			q_stand_pat_calculated = true;
			
			if (EVAL_IS_MATE(q_stand_pat)) return q_stand_pat;
		}
		
		if (q_stand_pat - margin >= beta)
			return q_stand_pat - margin;
	}


	/*
	if (depth <= 3
	   && !initially_in_check
	   && alpha != EVAL_MAX) {

		//const eval_t q_stand_pat = q_search(board, 0, stats, alpha, beta);
		const eval_t stand_pat = eval(board) * (board->sidetomove == WHITE ? 1 : -1);
		if (stand_pat + (80 * depth) < alpha) // || q_stand_pat == EVAL_MAX || q_stand_pat == EVAL_MIN)
			return q_search(board, 0, stats, alpha, beta);
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
			
			//const eval_t q_stand_pat = q_search(board, 0, stats, alpha, beta);
			if (!q_stand_pat_calculated) {
				q_stand_pat = q_search(board, 0, ply, stats, alpha, beta);
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
	   && !is_null_move
	   && !EVAL_IS_LOSE(beta)
	   && eval(board) * (board->sidetomove == WHITE ? 1 : -1) >= beta) {

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


	const uint16_t special_moves[1] = {tt_entry_bestmove};
	const size_t n_special_moves = tt_entry_found;

	movefactory_s movefactory;
	init_movefactory(&movefactory, &killer_moves[ply], special_moves, n_special_moves);

	unsigned int n_legal_moves_done = 0;
	unsigned int n_legal_moves_skipped = 0;
	unsigned int n_legal_moves_total = 0;
	move_s* move = NULL;
	eval_t best_score = -EVAL_INF;
	move_s* best_move = NULL;

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
		//    && n_legal_moves_total >= 7)
		// //   && !(move->fromtype == PAWN
		// //       && move->to & (move->side==WHITE ? TOP_THREE_ROWS : BOTTOM_THREE_ROWS)))
		// {
		// 	n_legal_moves_skipped++;
		// 	unmakemove(board, move);
		// 	//continue;
		// 	break;
		// }

		// move will be done
		n_legal_moves_done++;

		int depth_modifier = 0;

		if (move_was_check && ply < 20)
			depth_modifier++;
		// if (move_was_check
		//   && move->flags & FLAG_CAPTURE
		//   && move->move_see > 0) {
		// 	depth_modifier++;
		// }
		
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
			//if (n_legal_moves_total > 10) depth_modifier -= 1;
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

			if (!is_null_move)
				store_move(&tt_normal, board->hash, score, depth, encode_compact_move(move), TT_ENTRY_FLAG_FAIL_HIGH, ply);

			return score;
		}
		else if (depth > 1 && !(move->flags & FLAG_CAPTURE))
				bf_score[move->side][lowest_bitindex(move->from)][lowest_bitindex(move->to)] += 1;


		//alpha = MAX(score, alpha);
		//best_score = MAX(score, best_score);
		if (score > best_score) {
			best_score = score;
			best_move = move;
		}


		unmakemove(board, move);
		
		ZW_SEARCH_SKIP_MOVE_PRE_MAKE:
		continue;
	}


	if (!n_legal_moves_done) {
		if (n_legal_moves_skipped) {
			// Legal moves were actually just skipped
			if (q_stand_pat_calculated)
				return q_stand_pat;
			return q_search(board, 0, ply, stats, alpha, beta);
		}
		else {
			// No legal moves
			if (initially_in_check) { // is a checkmate (was in check and can't get out of it)
				return -MATE + ply;
			}
			else { // is a stalemate (wasn't in check and no legal moves)
				return (ply % 2 == 0 ? CONTEMPT_FACTOR : -CONTEMPT_FACTOR);
			}
		}
	}

	if (!is_null_move)
		store_move(&tt_normal, board->hash, alpha, depth, encode_compact_move(best_move), TT_ENTRY_FLAG_FAIL_LOW, ply);

	return best_score;
}



static eval_t q_search(board_s* restrict board, const int qdepth, const int ply, search_stats_s* restrict stats, eval_t alpha, eval_t beta) {

	if (stats)
		stats->nodes++;
	
	if (abort_search) return 0;

	const bool initially_in_check = is_in_check(board, board->sidetomove);


	// Do not do evaluation of a position when in check
	eval_t stand_pat = -EVAL_INF;
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
	eval_t best_score = -EVAL_INF;

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

		score = -q_search(board, qdepth + 1, ply + 1, stats, -beta, -alpha);

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
			return -MATE + ply;

		// Could have been a stalemate, but since only winning captures was generated, can't know
		assert(stand_pat != -EVAL_INF);
		return stand_pat;
	}


	return best_score;

}
