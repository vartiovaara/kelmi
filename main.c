#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

#include "board.h"
#include "defs.h"
#include "userio.h"
#include "movegen.h"

int main(int argc, char** argv) {
	if (argc != 1) {
		printf ("This program doesn't take any arguments.\n");
	}

	srand(time(0));

	board_t board = getstartingboard();

	printhelp();

	char input_buf[INPUT_BUF_LEN];
	while (1) {
		START:
		// get user input and interprate it
		if (fgets(input_buf, INPUT_BUF_LEN, stdin) == NULL) {
			fputs("Error reading stdin!\n", stderr);
		}

		// -1 becouse we don't want \n
		const int len = strlen(input_buf)-1;

		if (len == 1) {
			if (interprate_command(&board, input_buf[0]) < 0) {
				break;
			}
			goto START;
		}
		else if (len == 4) {
			const char from[2] = {input_buf[0], input_buf[1]};
			const char to[2] = {input_buf[2], input_buf[3]};
			if (validalgcoord(from) && validalgcoord(to)) {
				// input shoud be an algebraic move
				movepiece(&board, algtonum(from), algtonum(to));
			}
			else {
				printf("That didn't seem like a move.\n");
				goto START;
			}
		}
		else {
			printf("Invalid input.\n");
			goto START;
		}
		// input done

		// do our move
		movevec_t move = getrandommove(&board, BLACK);
		movepiece(&board, move.from, move.to);
		printf("My move is %d -> %d\n", move.from, move.to);
	}

	// Do all the exit stuff here
	// or maybe use atexit() or smting

	return 0;
}