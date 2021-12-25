#include <stdlib.h>
#include <stdio.h>
#include <string.h>

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
	if (depth == 0) {
		printf("Last move by %s.\nReached position:\n", (board->sidetomove==WHITE ? "white" : "black"));
		printboard(board);
		return 1; // normally do eval here but nyehhh
	}
	
	unsigned int nleaves = 0;

	uint64_t pieces_copy = board->all_pieces[board->sidetomove];
	unsigned int npieces = popcount(pieces_copy);

	//move_s* moves = (move_s*)malloc(sizeof(move_s) * npieces);
	//for (unsigned int i = 0; i < npieces; i++) {
	//	moves[i] = pseudo_legal_squares(board, pop_bitboard(&pieces_copy));
	//}
	//printf("%u leaves from here\n", n);

	board_s boardcopy = *board;
	//memcpy(&boardcopy, board, sizeof (board_s));

	for (unsigned int i = 0; i < npieces; i++) {
		//nleaves += search(board, depth-1);
		//movepiece();
		// generate moves
		movelist_s moves = pseudo_legal_squares(board, pop_bitboard(&pieces_copy));
		// go trough every move
		for (unsigned int j = 0; j < moves.n; j++) {
			makemove(board, &moves.moves[j]);
			nleaves += search(board, depth-1);
			//memcpy(board, &boardcopy, sizeof (board_s));
			*board = boardcopy;
		}
		free(moves.moves);
	}
	memcpy(board, &boardcopy, sizeof (board_s));

	// free movelist
	//free(moves);

	return nleaves;
}