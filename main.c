#include <stdio.h>
#include <string.h>

#include "board.h"
#include "defs.h"
#include "userio.h"

int main(int argc, char** argv) {
	if (argc != 1) {
		printf ("This program doesn't take any arguments.\n");
	}

	board_t board = getstartingboard();
	printboard(&board);
	
	char input_buf[INPUT_BUF_LEN];
	while (1) {
		// get user input and interprate it
		fgets(input_buf, INPUT_BUF_LEN, stdin);
		
		// -1 becouse we don't want \n
		const int len = strlen(input_buf)-1;
		if (interprate_input(&board, input_buf, len) < 0)
			return 0;
	}

	return 0;
}