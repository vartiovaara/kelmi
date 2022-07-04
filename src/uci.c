#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>
#include <assert.h>
#include <math.h>

#include "uci.h"

#include "defs.h"

#include "board.h"
#include "algebraic.h"
#include "movegen.h"
#include "search.h"
#include "algebraic.h"
#include "eval.h"

// Private functions

// returns non-zero value on failure
int parse_go_command(uci_s* uci, board_s* board, char* input, FILE* f);

// returns non-zero value on failure
int parse_position_command(board_s* board, char* input, size_t n);


// may work, may not
int search_end_of_string(char* s, char* key) {
	size_t s_len = strlen(s);
	size_t key_len = strlen(key);

	unsigned int curr_correct = 0;

	for (int i = s_len-1; i >= 0; i--) {
		if (s[i] != key[key_len - (curr_correct + 1)]) {
			curr_correct = 0;
			// may be needed, may be not
			//i += curr_correct - 1;
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

/*
* go
	start calculating on the current position set up with the "position" command.
	There are a number of commands that can follow this command, all will be sent in the same string.
	If one command is not send its value should be interpreted as it would not influence the search.
	* searchmoves  .... 
		restrict search to this moves only
		Example: After "position startpos" and "go infinite searchmoves e2e4 d2d4"
		the engine should only search the two moves e2e4 and d2d4 in the initial position.
	* ponder
		start searching in pondering mode.
		Do not exit the search in ponder mode, even if it's mate!
		This means that the last move sent in in the position string is the ponder move.
		The engine can do what it wants to do, but after a "ponderhit" command
		it should execute the suggested move to ponder on. This means that the ponder move sent by
		the GUI can be interpreted as a recommendation about which move to ponder. However, if the
		engine decides to ponder on a different move, it should not display any mainlines as they are
		likely to be misinterpreted by the GUI because the GUI expects the engine to ponder
	   on the suggested move.
	* wtime 
		white has x msec left on the clock
	* btime 
		black has x msec left on the clock
	* winc 
		white increment per move in mseconds if x > 0
	* binc 
		black increment per move in mseconds if x > 0
	* movestogo 
      there are x moves to the next time control,
		this will only be sent if x > 0,
		if you don't get this and get the wtime and btime it's sudden death
	* depth 
		search x plies only.
	* nodes 
	   search x nodes only,
	* mate 
		search for a mate in x moves
	* movetime 
		search exactly x mseconds
	* infinite
		search until the "stop" command. Do not exit the search without being told so in this mode!
*/
// TODO: searchmoves, ponder, wtime, btime, binc, movestogo, depth, nodes, mate, movetime, infinite
int parse_go_command(uci_s* uci, board_s* board, char* input, FILE* f) {
	//enum uci_action_e action = UCI_IDLE;
	//enum uci_searchtype_e searchtype = UCI_SEARCH_REGULAR;
	uci->action = UCI_IDLE;
	uci->searchtype = UCI_SEARCH_REGULAR;

	// if these are not >0 then interpeted as unlimited
	uci->wtime = -1;
	uci->btime = -1;
	//long wtime = -1;
	//long btime = -1;

	uci->winc = 0;
	uci->binc = 0;
	//long winc = 0;
	//long binc = 0;

	// if this is not set but wtime and btime is set, it is sudden death
	uci->movestogo = -1;
	//long movestogo = -1;

	uci->sudden_death = false;

	char* token = strtok(input, " ");
	for (;;) {
		token = strtok(NULL, " ");

		if (!token)
			break;
		else if (!strcmp(token, "wtime")) {
			token = strtok(NULL, " ");
			uci->wtime = strtol(token, NULL, 10);
		}
		else if (!strcmp(token, "btime")) {
			token = strtok(NULL, " ");
			uci->btime = strtol(token, NULL, 10);
		}
		else if (!strcmp(token, "winc")) {
			token = strtok(NULL, " ");
			uci->winc = strtol(token, NULL, 10);
		}
		else if (!strcmp(token, "binc")) {
			token = strtok(NULL, " ");
			uci->binc = strtol(token, NULL, 10);
		}
		else if (!strcmp(token, "movestogo")) {
			token = strtok(NULL, " ");
			uci->movestogo = strtol(token, NULL, 10);
		}
	}

	uci->action = UCI_SEARCH;

	//const long time = (board->sidetomove == WHITE ? uci->wtime : uci->btime);
	
	if (uci->wtime < 0 && uci->btime < 0) {
		uci->searchtype = UCI_SEARCH_INFINITE;
	}
	else { // wtime and btime was set
		uci->searchtype = UCI_SEARCH_REGULAR;

		if (uci->movestogo == -1) {
			uci->sudden_death = true;
		}
	}


	move_s bestmove;

	//move_s bestmoveJ

	float bestmove_eval = uci_think(uci, board, &bestmove);

	/*if (bestmove_eval == NAN
		|| bestmove_eval == (is_eval_better(INFINITY, -INFINITY, board->sidetomove) ? -INFINITY : INFINITY))
		*/

	
	char from[2];
	bbtoalg(from, bestmove.from);

	char to[2];
	bbtoalg(to, bestmove.to);

	char promote[2];
	if (bestmove.flags & FLAG_PROMOTE) {
		promote[0] = piecetochar(bestmove.promoteto);
		promote[1] = ' ';
	}
	else {
		promote[0] = ' ';
		promote[1] = '\0';
	}
	//uci_write(f, "info currmove %c%c%c%c%s", from[0], from[1], to[0], to[1], promote);
	uci_write(f, "info score cp %f\n", bestmove_eval);
	uci_write(f, "bestmove %c%c%c%c%s\n", from[0], from[1], to[0], to[1], promote);

	return 0;
}


// FIXME: oh god why oh no
int parse_position_command(board_s* board, char* input, size_t n) {
	resetboard(board);

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
	char input[UCI_INPUT_BUFFER_SIZE];
	uci_read(f, input, UCI_INPUT_BUFFER_SIZE);

	if(strcmp(input, "isready"))
		return;

	uci_write(f, "readyok\n");

	board_s board;
	resetboard(&board);

	uci_s uci;

	uci.action = UCI_IDLE;

	for (;;) {
		size_t len = uci_read(f, input, UCI_INPUT_BUFFER_SIZE);

		if (!len)
			continue;

		if (!strcmp(input, "ucinewgame")) {
			free_move_history(&board);
			resetboard(&board);
		}
		else if (!strncmp(input, "position", 8)) {
			parse_position_command(&board, input, len);
		}
		else if (!strncmp(input, "go", 2)){
			parse_go_command(&uci, &board, input, f);
		}
		else if (!strcmp(input, "quit")) {
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


