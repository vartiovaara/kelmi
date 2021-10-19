#include <stdio.h>

#include "defs.h"

int main(int argc, char** argv) {
	board_s board = boardfromfen(DEFAULT_FEN);
	printboard(&board);
	printf("%u\n", board.castling);
	return 0;
}