#include <stdio.h>

#include "defs.h"

int main(int argc, char** argv) {
	printf("Hello, World!\n");
	board_s board = boardfromfen(DEFAULT_FEN);
	printboard(&board);
	return 0;
}