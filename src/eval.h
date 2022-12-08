#ifndef EVAL_H
#define EVAL_H

#include <stdbool.h>

#include "defs.h"

// returns, whitch of the floats are better for side
eval_t better_eval(eval_t a, eval_t b, unsigned int side);

// returns if a is better than b for side
bool is_eval_better(eval_t a, eval_t b, unsigned int side);

// evaluates the board 
eval_t eval(const board_s* board);

// rates, how good a move could be
eval_t get_move_predict_score(const board_s* board, const move_s* move);

#endif // EVAL_H
