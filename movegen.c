#ifndef MOVEGEN_C
#define MOVEGEN_C

#include <stdlib.h>

#include "defs.h"

movevec_t getrandommove(board_t* board, uint8_t side) {
	uint8_t from_positions[16] = {0};
	uint8_t to_positions[64] = {0};
	uint8_t n_frompos = 0;
	uint8_t n_topos = 0;

	for (int i = 0; i < 64; i++) {
		if (board->colour[i] == side)
			from_positions[n_frompos++] = i;
		else
			to_positions[n_topos++] = i;
	}
	movevec_t move;
	move.from = from_positions[rand() % n_frompos];
	move.to = to_positions[rand() % n_topos];
	return move;
}

#endif