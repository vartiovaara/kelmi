#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>

#include "uci.h"

#include "defs.h"

#include "board.h"


// for more info, see: http://wbec-ridderkerk.nl/html/UCIProtocol.html
void uci(FILE* f) {
	// see: https://www.gnu.org/software/xboard/engine-intf.html#6
	setbuf(stdout, NULL);

	uci_write(f, "id name %s\n", ENGINE_NAME);
	uci_write(f, "id author %s\n", ENGINE_AUTHOR);

	uci_instance_s status;

	uci_write(f, "uciok\n");

	// get "isready" from gui
	char input[INPUT_BUFFER_SIZE];
	uci_read(f, input, INPUT_BUFFER_SIZE);

	if(strcmp(input, "isready"))
		return;

	uci_write(f, "readyok\n");

	board_s board;

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
			// divide the string to fields
			char** fields = NULL;
			size_t fields_n = divide_string(fields, input, " ");

			if (fields_n < 2)
				return;


			// TODO: read fen
			if (!strcmp(fields[1], "startpos")) {
				board = boardfromfen(DEFAULT_FEN);
			}

			//TODO: startpos moves x is not implemented
			if (fields_n > 2)
				return;

			free(fields);
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
				if (!strcmp(fields[i], ""));
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
	s[--len] = '\0'; // fget() reads the newline too so replace it with null
	
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


