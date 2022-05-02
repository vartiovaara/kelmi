#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <assert.h>
#include <string.h>
#include <time.h>

#include "init.h"
#include "search.h"
#include "board.h"
#include "uci.h"

#include "defs.h"



int main(void) {
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
	//board_s board = boardfromfen(DEFAULT_FEN);
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
			board_s board = boardfromfen(DEFAULT_FEN);
			perft(&board, input[6]-48);
			free_move_history(&board);
		}
		if (!strcmp(input, "xboard")) {
			printf("XBoard not supported.\n");
			goto MAIN_NORMAL_EXIT;
		}
		if (!strcmp(input, "uci")) {
			const size_t filename_n = 20; // yyyy_mm_dd_hh_mm_ss + '\0'
			char filename[filename_n];
			time_t curtime = time(NULL);
			struct tm* loctime = localtime(&curtime);
			strftime(filename, filename_n, "%Y_%m_%d_%H_%M_%S", loctime);
			
			FILE* f = fopen(filename, "w+");
			setbuf(f, NULL);
			uci(f);
			fclose(f);

			goto MAIN_NORMAL_EXIT;
		}
		if (!strcmp(input, "exit") || !strcmp(input, "quit"))
			goto MAIN_NORMAL_EXIT;
	}

	//printbitboard(board.every_piece);

	//perft(&board, argv[1][0]-48);

	MAIN_NORMAL_EXIT:


	return 0;
}
