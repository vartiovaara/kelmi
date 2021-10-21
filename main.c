#include <stdio.h>

#include "defs.h"

int main() {
	board_s board = boardfromfen(DEFAULT_FEN);
	printboard(&board);
	printbitboard(pseudo_legal_squares(&board, WHITE, board.pieces[WHITE][KING]));
	return 0;
}