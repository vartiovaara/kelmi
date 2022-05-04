#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>
#include <assert.h>

#include "uci.h"

#include "defs.h"

#include "board.h"
#include "algebraic.h"
#include "movegen.h"


// Private functions

void parse_go_command(uci_context_s* status, char* command);

// returns non-zero value on failure
int parse_position_command(board_s* board, char* command, size_t n);



int search_end_of_string(char* s, char* key) {
	size_t s_len = strlen(s);
	size_t key_len = strlen(key);

	unsigned int curr_correct = 0;

	for (int i = s_len-1; i >= 0; i--) {
		if (s[i] != key[key_len - (curr_correct + 1)]) {
			curr_correct = 0;
		}

		if (s[i] == key[key_len - (curr_correct + 1)]) {
			curr_correct++;
		}

		if (curr_correct == key_len) {
			return i;
		}
	}
	return -1;
}


int parse_position_command(board_s* board, char* input, size_t n) {
	char input_copy[n + 1]; // +1 as to include \0

	strncpy(input_copy, input, n + 1);

	char* field = strtok(input_copy, " ");
	assert(!strcmp(input_copy, "position"));
	field = strtok(NULL, " ");

	// check if "moves" is seen in the end of the string
	// TODO: Find out if it even is supposed to always be here
	int moves_index = search_end_of_string(input, "moves");

	if (!strcmp(field, "fen")) {
		int fen_index = search_end_of_string(input, "fen");
		fen_index += 4; // skip "fen " to the supposed start of fen
		
		if (moves_index > 0) { // nothing after fen string
			// set null after supposed fen string (" moves" comes after fen string)
			input_copy[moves_index - 1] = '\0';
		}
		*board = boardfromfen(input_copy + fen_index);
	}
	else if (!strcmp(field, "startpos")) {
		*board = boardfromfen(DEFAULT_FEN);
	}
	else
		return 1;
	
	// ---- BOARD HAS BEEN SET ----

	if (moves_index < 0) // no moves
		return 0;

	// set moves_index to start from the start of moves
	moves_index += 6; // "moves "
	
	if ((size_t)moves_index > n - 4) // atleast 1 move needs to be here ("a2a4")
		return 1;
	
	char* uci_move = strtok(input_copy + moves_index, " ");

	if (!uci_move)
		uci_move = input_copy + moves_index;

	while (uci_move) {
		move_s move;
		memset(&move, 0, sizeof(move_s));

		// is move promotion?
		if (strlen(uci_move) > 4) {
			move.flags |= FLAG_PROMOTE;
			move.promoteto = piece_from_char(uci_move[4]);
		}

		move.from = SQTOBB(algsqtoint(uci_move));
		move.to = SQTOBB(algsqtoint(uci_move + 2));
		move.side = get_piece_side(board, move.from);
		move.fromtype = get_piece_type(board, move.side, move.from);

		set_move_flags(&move, board);
		makemove(board, &move);
		append_to_move_history(board, &move);

		uci_move = strtok(NULL, " ");
	}

	return 0;
}


// for more info, see: http://wbec-ridderkerk.nl/html/UCIProtocol.html
void uci(FILE* f) {
	// see: https://www.gnu.org/software/xboard/engine-intf.html#6
	setbuf(stdout, NULL);

	uci_write(f, "id name %s\n", ENGINE_NAME);
	uci_write(f, "id author %s\n", ENGINE_AUTHOR);

	uci_write(f, "uciok\n");

	// get "isready" from gui
	char input[INPUT_BUFFER_SIZE];
	uci_read(f, input, INPUT_BUFFER_SIZE);

	if(strcmp(input, "isready"))
		return;

	uci_write(f, "readyok\n");

	board_s board;
	resetboard(&board);

	uci_context_s context;

	for (;;) {
		size_t len = uci_read(f, input, INPUT_BUFFER_SIZE);

		if (!len)
			continue;

		if (!strcmp(input, "ucinewgame")) {
			free_move_history(&board);
			resetboard(&board);
			continue;
		}
		if (!strncmp(input, "position", 8)) {
			parse_position_command(&board, input, len);
			continue;
		}
		if (!strncmp(input, "go", 2)){
			if (len < 7) // shortest is "go winc/binc/mate"
				return;

			char** fields = NULL;
			size_t fields_n = divide_string(fields, input + 3, " ");
			
			if (fields_n < 1)
				return;

			//TODO: dawf
			for (size_t i = 0; i < fields_n; i++) {
				if (!strcmp(fields[i], ""))
					;
			}
		}
		if (!strcmp(input, "quit")) {
			return;
		}
	}
}


size_t uci_read(FILE* f, char* s, size_t n) {
	fgets(s, n, stdin);
	size_t len = strlen(s);
	s[--len] = '\0'; // fgets() reads the newline too so replace it with null
	
	if (f) {
		fprintf(f, "< %s\n", s);
	}
	
	return len;
}


void uci_write(FILE* f, const char* s, ...) {
	// See: 12.12.9 Variable Arguments Output Functions
	// From: https://www.gnu.org/software/libc/manual/pdf/libc.pdf

	va_list argptr;
	va_start(argptr, s);
	vprintf(s, argptr);
	va_end(argptr);
	

	if (f) {
		va_list argptr;
		va_start(argptr, s);

		const size_t n = strlen(s);
		char output_string[n + 3]; // "> " + %s
		output_string[0] = '>';
		output_string[1] = ' ';
		strcpy(output_string + 2, s);
		vfprintf(f, output_string, argptr);

		va_end(argptr);
	}
}


size_t divide_string(char** restrict fields, char* restrict s, const char* delim) {
	// divide the string to fields
	fields = NULL;
	size_t fields_n = 0;
	char* field = strtok(s, delim);

	while (field) {
		fields = realloc(fields, (fields_n + 1) * sizeof(char*));
		if (!fields) {
			fputs("realloc() failed while parsing position string", stderr);
			abort();
		}
		fields[fields_n] = field;
		field = strtok(NULL, delim);
		fields_n++;
	}

	return fields_n;
}


