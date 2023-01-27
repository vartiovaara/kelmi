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

eval_t eval_material(const board_s* board, const int phase);

int get_game_phase_value(const board_s* board);

// Sets move predict values for move
void set_move_predict_scores(const board_s* restrict board, move_s* restrict move);

// Gets the cheapest piece from pieces.
// Only considers pieces that are also on select_mask
// If multiple pieces of the same value is found, lowest by index is returned
// If no pieces are found, empty bitboard is returned
BitBoard get_cheapest_piece(const board_s* board, unsigned int side, BitBoard select_mask);

// SCORE IS FOR move->side SIDE NOT FOR WHITE ONLY
eval_t see(const board_s* restrict board, const move_s* restrict move);

#endif // EVAL_H
