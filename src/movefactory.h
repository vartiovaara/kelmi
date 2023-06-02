
#ifndef GETMOVE_H
#define GETMOVE_H

#include "defs.h"

typedef struct {

	/*
	 * 0 = special moves (hash, pv, ?) move
	 * 1 = winning captures
	 * 2 = killers
	 * 3 = best quiet moves
	 * 4 = losing captures
	 * 5 = rest of the quiet moves
	 */
	unsigned int phase;

	// Stores the to-squares of already generated moves
	BitBoard moves_generated[64]; // [sq]


	// Will never be accessed from here. from winning_captures[] and losing_captures[]
	// https://www.chess.com/forum/view/general/max-number-of-moves
	move_s moves[256];
	size_t moves_index;

	//move_s capture_moves[16][8]; // [piece_index][move]
	//move_s hash_move;


	bool captures_generated;
	bool quiet_moves_generated;

	BitBoard ** killer_moves; //[first / second][from / to]

	move_s* special_moves[64];
	size_t n_special_moves;

	// Promotion array size explanation:
	// (three forward for pawn) * (board width) * (promoteto pieces)
	// 3*8*4=96
	move_s* promotions[96];
	size_t n_promotions;

	// Capture array size explanation:
	// (pawn captures) + (rest)
	//8*2 + 8*30 = 256
	move_s* winning_captures[256];
	move_s* losing_captures[256];
	size_t n_winning_captures;
	size_t n_losing_captures;

	// Tracks, witch move is next
	size_t special_moves_index;
	size_t promotions_index;
	size_t winning_captures_index;
	size_t losing_captures_index;

} movefactory_s;

// Initializes a movefactory_s
//extern void init_movefactory(movefactory_s* restrict factory, const board_s* restrict board, const tt_entry_s* restrict tt_entry);
extern void init_movefactory(movefactory_s* restrict factory, BitBoard** restrict killer_moves, const uint16_t* restrict special_moves, const size_t n_special_moves);

extern move_s* get_next_move(const board_s* restrict board, movefactory_s* restrict factory);



#endif // GETMOVE_H
