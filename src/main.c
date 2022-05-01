#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <assert.h>
#include <string.h>

#include "init.h"
#include "search.h"
#include "board.h"
#include "interface.h"

#include "defs.h"



int main(int argc, char** argv) {
	/*
	if(argc != 2) {
		fprintf(stderr, "Wrong number of arguments!\n");
		return 1;
	}
	if(argv[1][0] < 48 || argv[1][0] > 57) {
		fprintf(stderr, "just give me a number wtf.\n");
		return 1;
	}*/
	
	init_all();
	board_s board = boardfromfen(DEFAULT_FEN);
	//printboard(&board);
	
	char input[INPUT_BUFFER_SIZE];
	for (;;) {
		fgets(input, INPUT_BUFFER_SIZE, stdin);
		size_t len = strlen(input);
		input[--len] = '\0'; // fget() reads the newline too so replace it with null
		//printf("got: %s with len: %lu \n", input, len);

		if (!strncmp(input, "perft ", 6)) {
			if (len != 7)
				continue;
			if(input[6] < 48 || input[6] > 57) {
				fprintf(stderr, "just give me a number wtf.\n");
				continue;
			}
			perft(&board, input[6]-48);
		}
		if (!strcmp(input, "xboard")) {
			xboard(&board);
			goto MAIN_NORMAL_EXIT;
		}
		if (!strcmp(input, "uci")) {
			printf("UCI not implemented yet.\n");
			continue;
		}
		if (!strcmp(input, "exit") || !strcmp(input, "quit"))
			goto MAIN_NORMAL_EXIT;
	}

	//printbitboard(board.every_piece);

	//perft(&board, argv[1][0]-48);

	MAIN_NORMAL_EXIT:

	free_move_history(&board);

	return 0;
}
