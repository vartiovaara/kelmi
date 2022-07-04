#ifndef EVAL_H
#define EVAL_H

#include <stdbool.h>

#include "defs.h"

// returns, whitch of the floats are better for side
float better_eval(float a, float b, unsigned int side);

// returns if a is better than b for side
bool is_eval_better(float a, float b, unsigned int side);

// evaluates the board 
float eval(board_s* board);

#endif // EVAL_H
