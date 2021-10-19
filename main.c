#include <stdio.h>

#include "defs.h"

int main(int argc, char** argv) {
	board_s board = boardfromfen(DEFAULT_FEN);
	printboard(&board);
	printf("%p\n", (void*)board.en_passant);
	return 0;
}