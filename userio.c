/*
This file handles everything about
user interaction.
*/

#ifndef USERIO_C
#define USERIO_C

#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#include "userio.h"

#include "board.h"


// CHANGE THIS AS NEEDED
#define HELPTXT \
"\
A bad chess program. \n\
Version: idgaf \n\
h - Print this help. \n\
p - Print the board. \n\
i - Information about gamestate. \n\
q - Exit the program. \n\
"

void printhelp() {
	printf("%s", HELPTXT);
}

int interprate_input(board_t* const board, const char* str, const int len) {
	// all the commands will be 1 char only
	if (len == 1) {
		switch (str[0]) {
			case 'h':
				printf(HELPTXT);
				return 0;
			case 'p':
				printboard(board);
				return 0;
			case 'i':
				// TODO: gamestate info printing
				printf("lol no\n");
				return 0;
			case 'q':
				return -1;
			default:
				printf("No such command exists!\n");
				return 1;
		}
	}
	else if (len == 4) {
		const char from[2] = {str[0], str[1]};
		const char to[2] = {str[2], str[3]};
		if (validalgcoord(from) && validalgcoord(to)) {
			// input shoud be an algebraic move
			movepiece(board, algtonum(from), algtonum(to));
			return 0;
		}
		return 1;
	}
	printf("What?\n");
	return 1;
}

int algtonum(const char* str) {
	int n = 0;
	n = (str[0] - 'a'); // x
	n += (str[1] - '1')*8;
	return n;
}

bool validalgcoord(const char* str) {
	if (str[0] < 'a' || str[0] > 'h')
		return false;
	if (str[1] < '1' || str[1] > '8')
		return false;
	return true;
}

#endif