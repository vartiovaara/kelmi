#ifndef SEARCH_H
#define SEARCH_H

#include "defs.h"

extern BitBoard killer_moves[MAX_DEPTH][2][2]; // [ply][first / second][from / to]
extern uint64_t hh_score[2][64][64]; // [side][from_sq][to_sq] // Counts cutoffs
extern uint64_t bf_score[2][64][64]; // [side][from_sq][to_sq] // Counts else

// sets bestmove toreturns evaluation of bestmove
eval_t uci_think(const uci_s* uci, board_s* restrict board, move_s* restrict bestmove, FILE* restrict f);


// searches a board to depth and tallies statistics
// can be used for developing pruning strategies and stuff
// NOTE: Check the definition of search_stats_s for what to free after calling this function
eval_t search_with_stats(board_s* restrict board, move_s* restrict bestmove, const unsigned int depth, search_stats_s* restrict stats);


#endif // SEARCH_H
