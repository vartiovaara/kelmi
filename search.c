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
	printf("Entered search with parameters %p, %u \n", (void*)board, depth);
	if (depth == 0)
		return 1; // normally do eval here but nyehhh
	
	unsigned int nleaves = 1;
	// just make up random shit
	unsigned int n = 2;// rand() % 5; // amount of moves to make from this leaf
	printf("%u leaves from here\n", n);
	for (unsigned int i = 0; i < n; i++) {
		nleaves += search(board, depth-1);
	}
	return nleaves;
}