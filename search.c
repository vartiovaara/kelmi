#include <stdlib.h>
#include <stdio.h>
#include <string.h>

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

void perft(board_s* board, const unsigned int depth) {
	printf("Starting perft with depth %u\n", depth);

	const unsigned int search_res = search(board, depth);

	printf("Expected perft: %u\n", expected_perft[depth]);
	printf("Calculated perft: %u\n", search_res);
	printf("Error: %d\n", (int)search_res-(int)expected_perft[depth]);
	printf("Procentual error: %f%%\n", ((float)((int)search_res-(int)expected_perft[depth])/(float)expected_perft[depth])*100.f);
}

/*
a recursive search algorithm.
so far just a prototype.
starts with the side that is supposed to move 
according to board_s.whiteturn.
either returns the amount of leaves(nodes?) searched total
or the amount of positions reached in the end (depending on if
nleaves is 1 or 0)
*/
unsigned int search(board_s* board, const unsigned int depth) {
	//printf("Entered search with parameters %p, %u \n", (void*)board, depth);
	if (depth == 0) {
		//printf("Last move by %s.\nReached position:\n", (board->sidetomove==WHITE ? "white" : "black"));
		//printboard(board);
		return 1; // normally do eval here but nyehhh
	}
	
	const unsigned int initial_npos = 0;
	unsigned int npos = initial_npos;

	uint64_t pieces_copy = board->all_pieces[board->sidetomove];
	unsigned int npieces = popcount(pieces_copy);

	board_s boardcopy;// = *board;
	memcpy(&boardcopy, board, sizeof (board_s)); // for some reason, it's faster with memcpy

	for (unsigned int i = 0; i < npieces; i++) {
		// generate moves
		movelist_s moves = pseudo_legal_squares(board, pop_bitboard(&pieces_copy));
		if (!moves.n)
			continue; // otherwise we'd get segfault from freeing carbage data
		// go trough every move
		for (unsigned int j = 0; j < moves.n; j++) {
			makemove(board, &moves.moves[j]);
			npos += search(board, depth-1);
			memcpy(board, &boardcopy, sizeof (board_s));
			//*board = boardcopy;
		}
		free(moves.moves);
	}
	//memcpy(board, &boardcopy, sizeof (board_s));
	if (npos == initial_npos)
		return 1; // this position doesn't have any legal moves
	return npos;
}
