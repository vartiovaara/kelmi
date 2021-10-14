/*
Stuff for cli user interaction.
*/

#include <stdbool.h>

#include "board.h"
#include "userio.c"

#define INPUT_BUF_LEN 256

void printhelp();

// interprates the users input
// if function returns -1, program needs to exit
// returns 1 if error in parsing input
int interprate_input(board_t* const board, const char* str, const int len);

// converts algebraic notation (e.g b5) to
// square num
int algtonum(const char* str);

// Checks if a algebraic square
// representation is valid (a1 or smthn)
bool validalgcoord(const char* str);