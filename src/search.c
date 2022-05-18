#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "search.h"

#include "movegen.h"
#include "bitboard.h"
#include "board.h"

#include "defs.h"



// Private functions
void regular_search(const uci_s* uci, const board_s* board, move_s* bestmove);



void uci_think(const uci_s* uci, const board_s* board, move_s* bestmove) {
	assert(uci->action != UCI_IDLE);

	if (uci->action == UCI_PONDER)
		goto THINK_PONDER;
	
	// Normal search
	regular_search(uci, board, bestmove);
	return;

	THINK_PONDER:
	// TODO
	fputs("Pondering not supported!", stderr);

	return;
}



// TODO: Test with lazy movegen
void regular_search(const uci_s* uci, const board_s* board, move_s* bestmove) {
	// TODO
	assert(uci); // to suppress warning about unused warnings
	
	const bool initially_in_check = is_in_check(board, board->sidetomove);

	// pseudo legal move list
	move_s* pl_moves = NULL;
	size_t n_pl_moves = 0;

	BitBoard pieces_copy = board->all_pieces[board->sidetomove];
	unsigned int n_pieces = popcount(pieces_copy);
	for (size_t i = 0; i < n_pieces; i++) {
		movelist_s moves = pseudo_legal_squares(board, pieces_copy);

		pl_moves = realloc(pl_moves, sizeof(size_t) * n_pl_moves);
		if (!pl_moves) {
			fputs("realloc() failed in regular_search()\n", stderr);
			abort();
		}
		memcpy(pl_moves + n_pl_moves, moves.moves, moves.n);
		n_pl_moves += moves.n;
	}

	unsigned int chose_move = rand() % n_pl_moves;
}

