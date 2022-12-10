#ifndef SEARCH_H
#define SEARCH_H

#include "defs.h"

// sets bestmove toreturns evaluation of bestmove
eval_t uci_think(const uci_s* uci, board_s* restrict board, move_s* restrict bestmove);


// searches a board to depth and tallies statistics
// can be used for developing pruning strategies and stuff
// NOTE: Check the definition of search_stats_s for what to free after calling this function
eval_t search_with_stats(board_s* restrict board, move_s* restrict bestmove, const unsigned int depth, search_stats_s* restrict stats);


#endif // SEARCH_H
