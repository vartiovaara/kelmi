#ifndef SEARCH_H
#define SEARCH_H

#include "defs.h"

void perft(board_s* board, const unsigned int depth);
//void search(board_s* board, const unsigned int depth, pertf_result_s* res);

void init_perft_result(pertf_result_s* res, unsigned int depth);

void free_perft_result(pertf_result_s* res);

#endif // SEARCH_H