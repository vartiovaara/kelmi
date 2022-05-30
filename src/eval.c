#include <stdlib.h>
#include <stdbool.h>

#include "eval.h"

#include "defs.h"

float better_eval(float a, float b, unsigned int side) {
	return (is_eval_better(a, b, side) ? a : b);
}

bool is_eval_better(float a, float b, unsigned int side) {
	return ((side == WHITE) ? a > b : a < b);
}

float eval(board_s* board) {
	return (10 / ((rand() % 20)+1)) * ((rand() % 2 == 1) ? -1 : 1);
}
