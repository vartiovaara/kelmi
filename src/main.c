#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <assert.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "init.h"
#include "search.h"
#include "perft.h"
#include "board.h"
#include "uci.h"
#include "transposition.h"
#include "eval.h"
#include "movegen.h"

#include "defs.h"


// TODO: NOT DONE NOT PLAYABLE
// side = side the human plays as
void play_against(unsigned int side) {
	unsigned int eng_side = OPPOSITE_SIDE(side);

	board_s board = boardfromfen(DEFAULT_FEN);
	
	char input[INPUT_BUFFER_SIZE];
	while(true) {
		// Do engine move
		if (board.sidetomove == eng_side) {
			move_s move;
			memset(&move, 0, sizeof (move));
		}

		fgets(input, INPUT_BUFFER_SIZE, stdin);
		size_t len = strlen(input);
		input[--len] = '\0'; // fgets() reads the newline too so replace it with null

		if (!strcmp(input, "p")) {
			printboard(&board);
			printf("%s to move.\n", (board.sidetomove == WHITE ? "White" : "Black"));
		}
	}
}


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

	puts(HELP_MESSAGE);
	
	init_all();

	/*
	board_s board = boardfromfen("r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1");//"1k1r3q/1ppn3p/p4b2/4p3/8/P2N2P1/1PP1R1BP/2K1Q3 w - - 0 1");//DEFAULT_FEN);
	printboard(&board);


	move_s move;
	uci_notation_to_move(&board, &move, "d5e6"); //"d3e5" "e2e5" "g2b7"

	//printbitboard(get_seeing_pieces(&board, move.to, move.from));

	printf("%i\n", see(&board, &move));
	*/
	

	/*
	for (int i = 0; i < 8; i++) {
		printbitboard(rowlookup(i*8));
		printbitboard(columnlookup(i));
	}*/
	
	
	char input[INPUT_BUFFER_SIZE];
	for (;;) {
		fgets(input, INPUT_BUFFER_SIZE, stdin);
		size_t len = strlen(input);
		input[--len] = '\0'; // fgets() reads the newline too so replace it with null
		//printf("got: %s with len: %lu \n", input, len);

		if (!strncmp(input, "play", 4) && len > 4) {
			play_against(input[4] == 'w' ? WHITE : BLACK);
		}
		else if (!strncmp(input, "perft ", 6)) {
			if (len != 7)
				continue;
			if(input[6] < 48 || input[6] > 57) {
				fprintf(stderr, "just give me a number wtf.\n");
				continue;
			}
			board_s board = boardfromfen(DEFAULT_FEN);
			perft(&board, input[6]-48);
#ifndef NDEBUG
			free_move_history(&board);
#endif // NDEBUG
		}
		else if (!strcmp(input, "xboard")) {
			printf("XBoard not supported.\n");
			goto MAIN_NORMAL_EXIT;
		}
		else if (!strcmp(input, "uci")) {
			const size_t filename_n = 30; // yyyy_mm_dd_hh_mm_ss_ + (pid len<=8 i guess) + '\0'
			char filename[filename_n];
			time_t curtime = time(NULL);
			struct tm* loctime = localtime(&curtime);
			strftime(filename, filename_n, "%Y_%m_%d_%H_%M_%S_", loctime);

			// replace current null-byte with the pid as a string
			sprintf(filename + strlen(filename), "%u", getpid());
			
			FILE* f = fopen(filename, "w+");
			setbuf(f, NULL);
			uci(f);
			fclose(f);

			goto MAIN_NORMAL_EXIT;
		}
		else if (!strcmp(input, "help"))
			puts(HELP_MESSAGE);
		else if (!strcmp(input, "exit") || !strcmp(input, "quit"))
			goto MAIN_NORMAL_EXIT;
		else
			puts("Unknown promt.");
	}

	//printbitboard(board.every_piece);

	//perft(&board, argv[1][0]-48);

	MAIN_NORMAL_EXIT:

	free_table(&tt_normal);

	return 0;
}
