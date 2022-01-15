#include <stdio.h>
#include <stdlib.h>
#include <limits.h>

#include "defs.h"

#include <assert.h>

/*
TODO: Maybe start movegen again
*/

int main(int argc, char** argv) {
	if(argc != 2) {
		fprintf(stderr, "Wrong number of arguments!\n");
		return 1;
	}
	if(argv[1][0] < 48 || argv[1][0] > 57) {
		fprintf(stderr, "just give me a number wtf.\n");
		return 1;
	}
	
	init_all();
	board_s board = boardfromfen(DEFAULT_FEN);
	//printboard(&board);

	perft(&board, argv[1][0]-48);

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
