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
b - Print the bitboards. \n\
i - Information about gamestate. \n\
q - Exit the program. \n\
"

void printhelp() {
	printf("%s", HELPTXT);
}

int interprate_command(board_t* const board, const char ch) {
	// all the commands will be 1 char only
	switch (ch) {
		case 'h':
			printf(HELPTXT);
			return 0;
		case 'p':
			printboard(board);
			return 0;
		case 'b':
			printf("White:\n");
			printbitboard(board->w_bitboard);
			printf("Black:\n");
			printbitboard(board->b_bitboard);
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
	return 1; // shouldn't get here
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