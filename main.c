#include <stdio.h>
#include <stdlib.h>
#include <limits.h>

#include "defs.h"

/*
TODO: Maybe start movegen again
*/

int main() {
	init_all();
	board_s board = boardfromfen(DEFAULT_FEN);
	//printboard(&board);

	perft(&board, 7);

	//printf("%u\n", search(&board, 6));

	
	/*movelist_s moves = pseudo_legal_squares(&board, lowest_bitboard(board.pieces[WHITE][KNIGHT]));
	printboard(&board);
	printf("From:\n");
	printbitboard(moves.moves[0].from);
	//printbitboard(moves.moves[0].from);
	board_s boardcopy = board;
	printf("To:\n");
	for (unsigned int i = 0; i < moves.n; i++) {
		printbitboard(moves.moves[i].to);
		makemove(&board, &moves.moves[i]);
		printboard(&board);
		board = boardcopy;
		//printbitboard(moves.moves[i].to);
	}
	free(moves.moves);*/
	

	/*
	printbitboard(board.pieces[WHITE][PAWN]);
	printbitboard(pop_bitboard(&board.pieces[WHITE][PAWN]));
	printbitboard(board.pieces[WHITE][PAWN]);
	*/
	return 0;
}
