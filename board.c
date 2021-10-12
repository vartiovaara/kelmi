/*
Describes a board and has all the
things related to modifying a board.
*/

#ifndef BOARD_C
#define BOARD_C

#include <stdbool.h>
#include <stdint.h>

enum piece;

// Represents a board with
// all of its necessary gamestate
struct board_s {
	unsigned int pieces[64];
	bool whiteturn;
};
typedef struct board_s board_t;


board_t startingboard() {
	board_t board;
	return board;
}

#endif
