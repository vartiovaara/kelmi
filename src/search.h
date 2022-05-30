#ifndef SEARCH_H
#define SEARCH_H

#include "defs.h"

// sets bestmove toreturns evaluation of bestmove
float uci_think(const uci_s* uci, board_s* board, move_s* bestmove);


#endif // SEARCH_H