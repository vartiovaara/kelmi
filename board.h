/*
Board header file.
*/

#include <stdint.h>

#include "board.c"

// gives a default starting board
board_t getstartingboard();

void printboard(const board_t* restrict board);
void printbitboard(const uint64_t bb);

void movepiece(board_t* restrict board, const int from, const int to);