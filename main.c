#include <stdio.h>
#include <stdlib.h>

#include "defs.h"

int main() {
	init_all();
	board_s board = boardfromfen(DEFAULT_FEN);
	printboard(&board);
	printf("%d\n", popcount(board.pieces[WHITE][KNIGHT]));
	movelist_s moves = pseudo_legal_squares(&board, WHITE, board.pieces[WHITE][KNIGHT]);
	for (int i = 0; i < moves.n; i++) {
		printf("from:\n");
		printbitboard(moves.moves[i].from);
		printf("to:\n");
		printbitboard(moves.moves[i].to);
	}
	free(moves.moves);
	return 0;
}