#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <string.h>
#include <assert.h>

#include "search.h"

#include "movegen.h"
#include "bitboard.h"
#include "board.h"
#include "uci.h"
#include "movefactory.h"

#include "defs.h"



//https://oeis.org/A048987
//https://www.chessprogramming.org/Perft_Results#Initial_Position
const unsigned long long expected_perft[] = {
	1, // ply 0
	20,
	400,
	8902,
	197281,
	4865609,
	119060324,
	3195901860, // ply 7
	84998978956
};

#define PERFT_USE_MOVEFACTORY 1


// Private functions

void search(board_s* restrict board, const unsigned int depth, pertf_result_s* restrict res, FILE* restrict f);




void init_perft_result(pertf_result_s* res, unsigned int depth) {
	memset(res, 0, sizeof(pertf_result_s));
	res->n_plies = depth;

	res->n_positions = (unsigned long long*)calloc(depth+1, sizeof res->n_positions);
	res->captures = (unsigned long long*)calloc(depth+1, sizeof res->captures);
	res->checks = (unsigned long long*)calloc(depth+1, sizeof res->checks);
	res->en_passant = (unsigned long long*)calloc(depth+1, sizeof res->en_passant);
	res->checkmates = (unsigned long long*)calloc(depth+1, sizeof res->checkmates);
	res->stalemates = (unsigned long long*)calloc(depth+1, sizeof res->stalemates);
	res->castles = (unsigned long long*)calloc(depth+1, sizeof res->castles);
	res->promotions = (unsigned long long*)calloc(depth+1, sizeof res->promotions);
}

void free_perft_result(pertf_result_s* res) {
	free(res->n_positions);
	free(res->captures);
	free(res->checks);
	free(res->en_passant);
	free(res->checkmates);
	free(res->stalemates);
	free(res->castles);
	free(res->promotions);
}


void perft(board_s* board, const unsigned int depth) {
	printf("Starting perft with depth %u...\n\n", depth);

	// initialize perft_result_s
	pertf_result_s res;
	init_perft_result(&res, depth);

	//FILE* f = fopen("history", "w+");

	clock_t t = clock();

	//__builtin_prefetch(board, 1, 3);
	//__builtin_prefetch(board, 0, 3);
	search(board, depth, &res, NULL);
	
	t = clock() - t;
	double time_taken = ((double)t)/CLOCKS_PER_SEC;

	//fclose(f);

	printf("\n%llu leaves\n", res.n_leaves);
	printf("%llu nodes searched in %3fs\n", res.nodes, time_taken);
	printf("%f Nps\n\n", (float)res.nodes/time_taken);

	for (unsigned int i = 0; i <= res.n_plies; i++) {
		//printf("Positions at depth %u: %llu \n", i, res.n_positions[i]);
		printf("Depth %u:\n", i);
		printf("Expected perft: %llu\n", expected_perft[i]);
		printf("Calculated perft: %llu\n", res.n_positions[i]);
		printf("Error: %lld\n", (long long)res.n_positions[i]-(long long)expected_perft[i]);
		printf("Procentual error: %f%%\n", ((double)((long long)res.n_positions[i]-(long long)expected_perft[i])/(double)expected_perft[i])*(double)100);
		printf("Captures: %lld\n", res.captures[i]);
		printf("Checks: %lld\n", res.checks[i]);
		printf("En passants: %lld\n", res.en_passant[i]);
		printf("Checkmates: %lld\n", res.checkmates[i]);
		printf("Stalemates: %lld\n", res.stalemates[i]);
		printf("Castles: %lld\n", res.castles[i]);
		printf("Promotions: %lld\n", res.promotions[i]);
		printf("\n");
	}

	free_perft_result(&res);
}



/*
 * a recursive search algorithm.
 * so far just a prototype.
 * starts with the side that is supposed to move 
 * according to board_s.sidetomove.
 * Check perft_result_s
 */
void search(board_s* restrict board, const unsigned int depth, pertf_result_s* restrict res, FILE* restrict f) {
	res->n_positions[res->n_plies - depth]++;

	// is position a last one
	if (depth == 0) {
		goto SEARCH_LAST_NODE;
	}


	// for distincting stalemate and checkmate
	const bool initially_in_check = is_in_check(board, board->sidetomove);
	//const bool initially_in_check = (board->side_in_check == board->sidetomove); //is_in_check(board, board->sidetomove);

	// for checking, if any moves were made
	// NOTE: Index might overflow and shit the bed
	const unsigned long long initial_nodes = res->n_positions[(res->n_plies - depth) + 1];

	//unsigned long long initial_leaves = res->n_leaves;

	// for checking if position is a checkmate
	unsigned long long skipped_because_of_checks = 0;

	const unsigned int initial_side = board->sidetomove;

	BitBoard pieces_copy = board->all_pieces[board->sidetomove];
	unsigned int npieces = popcount(pieces_copy);

	//board_s boardcopy;// = *board;
	//memcpy(&boardcopy, board, sizeof (board_s)); // for some reason, it's faster with memcpy

	movelist_s moves;
#if PERFT_USE_MOVEFACTORY == 1
	movefactory_s factory;
	init_movefactory(&factory, NULL, NULL, 0);
	while (true) {
#else
	move_s moves_array[32];
	moves.moves = moves_array;
	// go through every piece
	for (unsigned int i = 0; i < npieces; i++) {
#endif
#if PERFT_USE_MOVEFACTORY == 1
		moves.moves = get_next_move(board, &factory, false);
		if (!moves.moves)
			moves.n = 0;
		else
			moves.n = 1;
		
		if (!moves.n)
			break;
#else
		// generate moves
		get_pseudo_legal_moves(board, &moves, pop_bitboard(&pieces_copy), false, 0x0);

		assert(moves.n < LENGTH(moves_array));
		
		// if there aren't any moves, cont now.
		// otherwise we'd be freeing memory that has never
		// been allocated
		if (!moves.n)
			continue;
#endif
		
		
		// go trough every move
		for (unsigned int j = 0; j < moves.n; j++) {
			res->nodes++;

			//__builtin_prefetch(&moves.moves[j], 0, 1);

			if (!moves.moves[j].from)
				continue;

			// Check if castling is valid
			if (__builtin_expect(moves.moves[j].flags & (FLAG_KCASTLE | FLAG_QCASTLE), 0)) {
				if (initially_in_check)
					continue; // can not castle while in check
				
				// BitBoard between_rk;
				// if (board->sidetomove == WHITE)
				// 	between_rk = (moves.moves[j].flags & FLAG_KCASTLE ? WK_CAST_CLEAR_MASK : WQ_CAST_CLEAR_MASK);
				// else
				// 	between_rk = (moves.moves[j].flags & FLAG_KCASTLE ? BK_CAST_CLEAR_MASK : BQ_CAST_CLEAR_MASK);
				
				BitBoard target_squares;
				if (board->sidetomove == WHITE)
					target_squares = (moves.moves[j].flags & FLAG_KCASTLE ? WK_CASTLE_ATTACK_MASK : WQ_CASTLE_ATTACK_MASK);
				else
					target_squares = (moves.moves[j].flags & FLAG_KCASTLE ? BK_CASTLE_ATTACK_MASK : BQ_CASTLE_ATTACK_MASK);
				
				//FIXME: This shit is slow as fuck. Make those attack maps pls.
				while (target_squares) {
					if (is_side_attacking_sq(board, pop_bitboard(&target_squares), OPPOSITE_SIDE(board->sidetomove)))
						goto SEARCH_SKIP_MOVE_PRE_MAKE;
				}
			}
			
			makemove(board, &moves.moves[j]);
			//append_to_move_history(board, &moves.moves[j]);

			// check if that side got itself in check (or couldn't get out of one)
			//if (moves.moves[j].fromtype == KING) {
				if (is_in_check(board, initial_side)) {
					skipped_because_of_checks++;
					goto SEARCH_SKIP_MOVE;
				}
			//}

			// if (is_in_check(board, board->sidetomove))
			// 	board->side_in_check = board->sidetomove;
			// else
			// 	board->side_in_check = SIDE_NONE;


			// MOVE WILL BE DONE


			// En passant will be considered a capture only in stats.
			// See: https://www.chessprogramming.org/Perft_Results#Initial_Position
			if (moves.moves[j].flags & (FLAG_CAPTURE | FLAG_ENPASSANT))
				res->captures[(res->n_plies - depth)+1]++; // +1 becouse that move got itself to that depth so it will be counted as such
			
			// this statistic is expensive. only count it in debug builds
			#ifndef NDEBUG
			//if (board->side_in_check == board->sidetomove)
			if (is_in_check(board, board->sidetomove))
				res->checks[(res->n_plies - depth)+1]++;
			#endif

			if (moves.moves[j].flags & FLAG_ENPASSANT)
				res->en_passant[(res->n_plies - depth)+1]++;
			
			if (moves.moves[j].flags & (FLAG_KCASTLE | FLAG_QCASTLE))
				res->castles[(res->n_plies - depth)+1]++;
			
			if (moves.moves[j].flags & FLAG_PROMOTE)
				res->promotions[(res->n_plies - depth)+1]++;


			const unsigned long long initial_leaves = res->n_leaves;

			search(board, depth-1, res, f);

			if (__builtin_expect(res->n_plies - depth == 0, 0)) {
				char move_str[6];
				move_to_uci_notation(&moves.moves[j], move_str);
				//printf("%s: %llu\n", move_str, res->n_positions[(res->n_plies - depth) + 1] - initial_nodes);
				printf("%s: %llu\n", move_str, res->n_leaves - initial_leaves);
				//initial_leaves = res->n_leaves;
			}

			SEARCH_SKIP_MOVE: // if move was illegal, go here
			//restore_board(board, &boardcopy);
			unmakemove(board, &moves.moves[j]);
			//memcpy(board, &boardcopy, sizeof (board_s)); // restore board
			//*board = boardcopy;
			SEARCH_SKIP_MOVE_PRE_MAKE:
			continue;
		}

		// free(moves.moves);
	}
	
	// No moves were made?
	if (res->n_positions[(res->n_plies - depth) + 1] == initial_nodes) {
		if (initially_in_check) { // is a checkmate (was in check and can't get out of it)
			res->checkmates[res->n_plies - depth]++;
		}
		else { //is a stalemate (wasn't in check and no legal moves)
			res->stalemates[res->n_plies - depth]++;
		}
		goto SEARCH_LAST_NODE; // this position doesn't have any legal moves
	}

	return;

	// Go here if no more moves are made
	SEARCH_LAST_NODE:

	res->nodes++;
	res->n_leaves++;

#ifndef NDEBUG
	if (!f)
		return;

	// Write history to file
	write_move_history(board, f);
#endif // NDEBUG
}
