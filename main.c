#include <stdio.h>

#include "board.h"
#include "defs.h"

int main(int argc, char** argv) {
	board_t board = getstartingboard();
	printboard(&board);
	printf("Hello, World!\n");
	return 0;
}