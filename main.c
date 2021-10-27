#include <stdio.h>
#include <stdlib.h>
#include <limits.h>

#include "defs.h"

int main() {
	init_all();
	board_s board = boardfromfen(DEFAULT_FEN);
	printboard(&board);

	movelist_s moves = pseudo_legal_squares(&board, WHITE, KING);
	printf("%u pieces: \n", moves.n);
	for (unsigned int i = 0; i < moves.n; i++) {
		printf("from:\n");
		printbitboard(moves.moves[i].from);
		printf("to:\n");
		printbitboard(moves.moves[i].to);
	}
	free(moves.moves);
	return 0;
}
