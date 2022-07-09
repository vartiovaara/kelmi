#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <math.h>

#include "search.h"

#include "movegen.h"
#include "bitboard.h"
#include "board.h"
#include "eval.h"
#include "uci.h"
#include "algebraic.h"

#include "defs.h"



// Private functions
eval_t regular_search(board_s* restrict board, move_s* restrict bestmove, const bool is_first_ply, const unsigned int depth);



eval_t uci_think(const uci_s* uci, board_s* board, move_s* bestmove) {
	assert(uci->action != UCI_IDLE);

	//if (uci->action == UCI_PONDER)
	//	goto THINK_PONDER;
	
	// Normal search
	return regular_search(board, bestmove, true, 5);

	//THINK_PONDER:
	// TODO
	//fputs("Pondering not supported!", stderr);

	//return NAN;
}



eval_t regular_search(board_s* restrict board, move_s* restrict bestmove, const bool is_first_ply, const unsigned int depth) {
	if (depth == 0)
		return eval(board);

	const bool initially_in_check = is_in_check(board, board->sidetomove);

	// for checking if there were any legal moves in this position for board->sidetomove
	unsigned int n_legal_moves_done = 0;

	// Stores the best move for board->sidetomove
	eval_t bestmove_eval = (board->sidetomove == WHITE ? EVAL_MIN : EVAL_MAX); // set to worst possible

	BitBoard pieces_copy = board->all_pieces[board->sidetomove];
	unsigned int n_pieces = popcount(pieces_copy);

	// for checking if position is a checkmate
	unsigned long long skipped_because_of_checks = 0;

	board_s boardcopy;
	memcpy(&boardcopy, board, sizeof (board_s));

	// Go through every piece
	for (size_t i = 0; i < n_pieces; i++) {
		movelist_s moves = pseudo_legal_squares(board, pop_bitboard(&pieces_copy));

		if (!moves.n)
			continue;
		
		// Go through every move
		for (unsigned int j = 0; j < moves.n; j++) {
			// --- CHECK FOR LEGALITY OF MOVE ---

			// Check if castling is valid
			if (moves.moves[j].flags & (FLAG_KCASTLE | FLAG_QCASTLE)) {
				if (initially_in_check)
					continue; // can not castle while in check
				
				BitBoard between_rk;
				if (board->sidetomove == WHITE)
					between_rk = (moves.moves[j].flags & FLAG_KCASTLE ? WK_CAST_CLEAR_MASK : WQ_CAST_CLEAR_MASK);
				else
					between_rk = (moves.moves[j].flags & FLAG_KCASTLE ? BK_CAST_CLEAR_MASK : BQ_CAST_CLEAR_MASK);
				
				//FIXME: This shit is slow as fuck. Make those attack maps pls.
				while (between_rk) {
					if (is_side_attacking_sq(board, pop_bitboard(&between_rk), OPPOSITE_SIDE(board->sidetomove)))
						goto REGULAR_SEARCH_SKIP_MOVE;
				}
			}
			
			makemove(board, &moves.moves[j]);

			// check if that side got itself in check (or couldn't get out of one)
			if (is_in_check(board, OPPOSITE_SIDE(board->sidetomove))) {
				skipped_because_of_checks++;
				goto REGULAR_SEARCH_SKIP_MOVE;
			}

			n_legal_moves_done++;

			// --- MOVE WAS LEGAL AND IS DONE ---

			append_to_move_history(board, &moves.moves[j]);

			eval_t eval = regular_search(board, bestmove, false, depth-1);
			//printf("info string %f\n", eval);

			// if move was better, store it instead
			if (is_eval_better(eval, bestmove_eval, OPPOSITE_SIDE(board->sidetomove))) {
				bestmove_eval = eval;
				if (is_first_ply)
					memcpy(bestmove, &moves.moves[j], sizeof (move_s));
			}

			REGULAR_SEARCH_SKIP_MOVE:
			restore_board(board, &boardcopy);
		}
		free(moves.moves);
	}

	// No moves were made??
	if (!n_legal_moves_done) {
		if (initially_in_check) { // is a checkmate (was in check and can't get out of it)
			// store the worst value possible for checkmate
			return (is_eval_better(EVAL_MAX, EVAL_MIN, board->sidetomove) ? EVAL_MIN : EVAL_MAX);
		}
		else { // is a stalemate (wasn't in check and no legal moves)
			return 0;
		}
	}
	
	return bestmove_eval;
}

