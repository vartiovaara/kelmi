#include <stdio.h>
#include <stdlib.h>
#include <limits.h>

#include "defs.h"

#include <assert.h>

#include "magicmoves/magicmoves.h"

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

	//printbitboard(board.every_piece);

	perft(&board, argv[1][0]-48);

	//printf("%u\n", search(&board, 6));

	//printbitboard(piecelookup(lowest_bitindex(board.all_pieces[WHITE]), ROOK, 0));
	
	/*const uint64_t piece_index = lowest_bitindex(board.pieces[WHITE][ROOK]);
	const uint64_t moves = Rmagic(
		piece_index,
		piecelookup(piece_index, ROOK, 0) | (board.all_pieces[WHITE] | board.all_pieces[BLACK]));
	printbitboard(moves);
	printbitboard(moves & ~board.all_pieces[WHITE]);*/

	/*
	board.sidetomove = BLACK;
	movelist_s moves = pseudo_legal_squares(&board, lowest_bitboard(board.pieces[BLACK][ROOK]));
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
	
	return 0;
}
