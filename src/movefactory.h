
#ifndef GETMOVE_H
#define GETMOVE_H

#include "defs.h"


#define N_BEST_QUIET_MOVES 3



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

	BitBoard (*killer_moves)[2][2]; //[first / second][from / to]

	move_s* last_move;

	uint16_t special_moves[64];
	size_t n_special_moves;
	size_t special_moves_index;

	size_t killer_index;

	// Promotion array size explanation:
	// (three forward for pawn) * (board width) * (promoteto pieces)
	// 3*8*4=96
	move_s* promotions[96];
	size_t n_promotions;

	// Capture array size explanation:
	// (pawn captures) + (rest)
	//8*2 + 8*30 = 256
	move_s* winning_captures[128];
	move_s* losing_captures[128];
	size_t n_winning_captures;
	size_t n_losing_captures;

	move_s* quiet_moves;
	size_t n_quiet_moves;
	move_s* best_quiet_moves[N_BEST_QUIET_MOVES];
	size_t best_quiet_moves_index;

	// Tracks, witch move is next
	size_t promotions_index;
	size_t winning_captures_index;
	size_t losing_captures_index;
	size_t quiet_moves_index;

} movefactory_s;

// Initializes a movefactory_s
//extern void init_movefactory(movefactory_s* restrict factory, const board_s* restrict board, const tt_entry_s* restrict tt_entry);
extern void init_movefactory(movefactory_s* restrict factory, BitBoard (*restrict killer_moves)[2][2], const uint16_t* restrict special_moves, const size_t n_special_moves);

// When is_qsearch is set, only good captures are returned
extern move_s* get_next_move(const board_s* restrict board, movefactory_s* restrict factory, bool is_qsearch);



#endif // GETMOVE_H
