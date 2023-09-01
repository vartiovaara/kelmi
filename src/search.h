#ifndef SEARCH_H
#define SEARCH_H

#include "defs.h"

extern BitBoard killer_moves[MAX_DEPTH][2][2]; // [ply][first / second][from / to]
extern uint64_t hh_score[2][64][64]; // [side][from_sq][to_sq] // Counts cutoffs
extern uint64_t bf_score[2][64][64]; // [side][from_sq][to_sq] // Counts else

// sets bestmove toreturns evaluation of bestmove
eval_t uci_think(const uci_s* uci, board_s* restrict board, move_s* restrict bestmove, FILE* restrict f);



#endif // SEARCH_H
