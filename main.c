#include <stdio.h>

#include "defs.h"

int main(int argc, char** argv) {
	printf("Hello, World!\n");
	board_s board;
	printboard(&board);
	return 0;
}