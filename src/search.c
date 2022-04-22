#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "search.h"

#include "attack.h"
#include "bitboard.h"
#include "board.h"

#include "defs.h"


//https://oeis.org/A048987
//https://www.chessprogramming.org/Perft_Results#Initial_Position
const unsigned int expected_perft[] = {
	1, // ply 0
	20,
	400,
	8902,
	197281,
	4865609,
	119060324,
	3195901860 // ply 7
};


// Private functions
void search(board_s* board, const unsigned int depth, pertf_result_s* res);


void perft(board_s* board, const unsigned int depth) {
	printf("Starting perft with depth %u...\n\n", depth);

	// initialize perft_result_s
	pertf_result_s res;
	memset(&res, 0, sizeof(pertf_result_s)); // set everithing to 0
	res.n_plies = depth;
	
	// +1 becouse even with depth=0 we need atleast len of 1 * ull
	res.n_positions = (unsigned long long*)malloc((depth * sizeof(res.n_positions)) + 1);
	memset(res.n_positions, 0, depth * sizeof res.n_positions + 1); // set it to 0

	res.captures = (unsigned long long*)malloc((depth * sizeof(res.n_positions)) + 1);
	memset(res.captures, 0, depth * sizeof res.captures + 1); // set it to 0

	clock_t t = clock();
	
	search(board, depth, &res);
	
	t = clock() - t;
	double time_taken = ((double)t)/CLOCKS_PER_SEC;

	printf("%llu nodes searched in %3fs\n", res.nodes, time_taken);
	printf("%f Nps\n\n", (float)res.nodes/time_taken);

	for (unsigned int i = 0; i <= res.n_plies; i++) {
		//printf("Positions at depth %u: %llu \n", i, res.n_positions[i]);
		printf("Depth %u:\n", i);
		printf("Expected perft: %u\n", expected_perft[i]);
		printf("Calculated perft: %llu\n", res.n_positions[i]);
		printf("Error: %lld\n", (long long)res.n_positions[i]-(long long)expected_perft[i]);
		printf("Procentual error: %f%%\n", ((double)((long long)res.n_positions[i]-(long long)expected_perft[i])/(double)expected_perft[i])*(double)100);
		printf("Captures: %lld\n", res.captures[i]);
		puts("\n");
	}

	free(res.n_positions);
	free(res.captures);
}

/*
 * a recursive search algorithm.
 * so far just a prototype.
 * starts with the side that is supposed to move 
 * according to board_s.sidetomove.
 * Check perft_result_s
 */
void search(board_s* board, const unsigned int depth, pertf_result_s* res) {
	res->n_positions[res->n_plies - depth]++;

	// is position a last one
	if (depth == 0) {
		res->end_positions++;
		res->nodes++;
		return;
	}
	
	//const unsigned int initial_npos = 0;
	//unsigned int npos = initial_npos;
	const unsigned long long initial_nodes = res->nodes;

	BitBoard pieces_copy = board->all_pieces[board->sidetomove];
	unsigned int npieces = popcount(pieces_copy);


	// used for not getting itself in check by moving own piece or not blocking
	//const bool initially_in_check = is_in_check(board, board->sidetomove);
	//printf("initially_in_check = %u\n", initially_in_check);
	const unsigned int initial_side = board->sidetomove;

	board_s boardcopy;// = *board;
	memcpy(&boardcopy, board, sizeof (board_s)); // for some reason, it's faster with memcpy

	for (unsigned int i = 0; i < npieces; i++) {
		// generate moves
		movelist_s moves = pseudo_legal_squares(board, pop_bitboard(&pieces_copy));
		
		// if there aren't any moves, cont now.
		// otherwise we'd be freeing memory that has never
		// been allocated
		if (!moves.n)
			continue;
		
		// go trough every move
		for (unsigned int j = 0; j < moves.n; j++) {
			res->nodes++;
			
			makemove(board, &moves.moves[j]);

			// check if that side got itself in check
			if (is_in_check(board, initial_side))
				goto SEARCH_SKIP_MOVE;
			
			// MOVE WILL BE DONE

			if (moves.moves[j].flags & FLAG_CAPTURE)
				res->captures[(res->n_plies - depth)+1]++; // +1 becouse that move got itself to that depth so it will be counted as such

			//res->nodes++; // currently only legal positions are consididired as "searched"
			search(board, depth-1, res);
			SEARCH_SKIP_MOVE: // if move was illegal, go here
			memcpy(board, &boardcopy, sizeof (board_s));
			//*board = boardcopy;
		}

		free(moves.moves);
	}

	if (res->nodes == initial_nodes) {
		res->end_positions++;
		res->nodes++;
		return; // this position doesn't have any legal moves
	}
}
