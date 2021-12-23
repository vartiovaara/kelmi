#include <stdlib.h>
#include <stdio.h>

#include "defs.h"

/*
a recursive search algorithm.
so far just a prototype.
starts with the side that is supposed to move 
according to board_s.whiteturn.
returns the amount of leaves(nodes?) searched total
*/
unsigned int search(board_s* board, unsigned int depth) {
	//printf("Entered search with parameters %p, %u \n", (void*)board, depth);
	if (depth == 0)
		return 1; // normally do eval here but nyehhh
	
	unsigned int nleaves = 1;

	uint64_t pieces_copy = board->all_pieces[board->sidetomove];
	
	unsigned int npieces = popcount(pieces_copy);
	move_s* moves = (move_s*)malloc(sizeof(move_s) * npieces);
	
	for (unsigned int i = 0; i < npieces; i++) {
		moves[i] = pseudo_legal_squares(board, pop_bitboard(&pieces_copy));
	}

	//printf("%u leaves from here\n", n);
	for (unsigned int i = 0; i < npieces; i++) {
		//nleaves += search(board, depth-1);
		//movepiece();
		nleaves += search(board, depth-1);
	}

	// free movelist
	free(moves);

	return nleaves;
}