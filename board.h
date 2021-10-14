/*
Board header file.
*/

#include "board.c"


// gives a default starting board
board_t getstartingboard();

void printboard(const board_t* restrict board);

void movepiece(board_t* restrict board, const int from, const int to);