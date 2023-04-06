
#include <assert.h>
#include <string.h>

#include "movefactory.h"

#include "bitboard.h"
#include "movegen.h"

#include "defs.h"


static const eval_t good_capture_treshold = -50;


static void generate_captures(const board_s* restrict board, movefactory_s* restrict factory);


static void generate_captures(const board_s* restrict board, movefactory_s* restrict factory) {

	BitBoard pieces_copy = board->all_pieces[board->sidetomove];
	for (size_t i = 0; pieces_copy; i++) { // loop until pieces_copy with i counter
		const BitBoard piece = pop_bitboard(&pieces_copy);

		movelist_s mvlist;
		//mvlist.moves = factory->capture_moves[i];
		mvlist.moves = &factory->moves[factory->moves_index];

		// to-squares for capturing and en-passant
		BitBoard capture_squares = board->all_pieces[OPPOSITE_SIDE(board->sidetomove)];
		if (piece & board->pieces[board->sidetomove][PAWN])
			capture_squares |= board->en_passant;
		
		// Ignore moves that have been generated already.
		capture_squares &= ~factory->moves_generated[lowest_bitindex(piece)];
		
		get_pseudo_legal_moves(board, &mvlist, piece, true, ~capture_squares);
		factory->moves_index += mvlist.n;
	}
}



void init_movefactory(movefactory_s* restrict factory, const board_s* restrict board, const uint16_t* restrict special_moves, const size_t n_special_moves) {
	
	memset(factory->moves_generated, 0, 64 * sizeof (BitBoard));
	factory->phase = 0;
	factory->moves_index = 0;
	factory->n_promotions = 0;
	factory->n_winning_captures = 0;
	factory->n_losing_captures = 0;
	factory->promotions_index = 0;
	factory->winning_captures_index = 0;
	factory->losing_captures_index = 0;
}


move_s* get_next_move(const board_s* restrict board, movefactory_s* restrict factory, uint16_t hash_move) {
	
	GET_NEXT_MOVE_RESTART:

	switch (factory->phase) {

		// Special moves
		case 0:
			factory->phase++;

			// FIXME: Create the right promotion move according to the hash move
			if (!hash_move || COMPACT_MOVE_PROMOTE_FLAG & hash_move)
				goto GET_NEXT_MOVE_RESTART;


			//movelist_s mvlist;
			//mvlist.moves = &factory->moves[factory->moves_index];
			//get_pseudo_legal_moves(board, &mvlist, COMPACT_MOVE_FROM(hash_move), true, ~COMPACT_MOVE_TO(hash_move));

			//if (!mvlist.n) // move didn't exist
			//	goto GET_NEXT_MOVE_RESTART;

			//factory->moves_generated[lowest_bitindex(factory->hash_move.from)] |= factory->hash_move.to;
			//return &factory->hash_move;

		
		// Winning captures
		case 1:
			if (!factory->captures_generated) {
				generate_captures(board, factory);
				factory->captures_generated = true;
			}

			// Check if all captures have been consumed already
			if (factory->winning_captures_index >= factory->n_winning_captures) {
				factory->phase++;
				goto GET_NEXT_MOVE_RESTART;
			}

			return &factory->winning_captures[factory->winning_captures_index++];




		// Best quiet moves
		case 3:
			if (!factory->quiet_moves_generated)
			factory->phase++;
			goto GET_NEXT_MOVE_RESTART;

		// Losing captures
		case 4:
			// No need to check if captures have been generated, as that happens in a higher phase
			assert(factory->captures_generated == true);

			// Check if all captures have been consumed already
			if (factory->losing_captures_index >= factory->n_losing_captures) {
				factory->phase++;
				goto GET_NEXT_MOVE_RESTART;
			}

			return &factory->losing_captures[factory->losing_captures_index++];
		
		default:
			return NULL;
	}

}

