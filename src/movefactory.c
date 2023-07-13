
#include <assert.h>
#include <string.h>

#include "movefactory.h"

#include "bitboard.h"
#include "movegen.h"

#include "defs.h"


enum GENERATE_TYPE {
	GENERATE_CAPTURES,
	GENERATE_QUIET_MOVES
};


static const eval_t good_capture_treshold = 0;//-50;


static void generate_captures(const board_s* restrict board, movefactory_s* restrict factory);


static void generate_captures(const board_s* restrict board, movefactory_s* restrict factory) {

	BitBoard pieces_copy = board->all_pieces[board->sidetomove];
	for (size_t i = 0; pieces_copy; i++) { // loop until pieces_copy with i counter
		const BitBoard piece = pop_bitboard(&pieces_copy);
		const unsigned int piece_index = lowest_bitindex(piece);

		// Set up the movelist
		movelist_s mvlist;
		mvlist.moves = &factory->moves[factory->moves_index];

		// Select the from and to square masks

		// to-squares for capturing and en-passant
		BitBoard capture_squares = board->all_pieces[OPPOSITE_SIDE(board->sidetomove)];
		if (piece & board->pieces[board->sidetomove][PAWN])
			capture_squares |= board->en_passant;
		
		// Ignore moves that have been generated already.
		capture_squares &= ~factory->moves_generated[piece_index];
		
		if (!capture_squares)
			continue;

		// Generate the moves
		get_pseudo_legal_moves(board, &mvlist, piece, true, ~capture_squares);
		factory->moves_index += mvlist.n;

		// Mark the current to-squares as generated
		factory->moves_generated[piece_index] |= capture_squares;

		// Save the captures to winning_captures and losing_captures
		for (size_t j = 0; j < mvlist.n; j++) {
			assert(mvlist.moves[j].flags & FLAG_CAPTURE || mvlist.moves[j].flags & FLAG_ENPASSANT);

			// Save to appropriate list
			if (mvlist.moves[j].move_score >= good_capture_treshold) {
				factory->winning_captures[factory->n_winning_captures++] = mvlist.moves + j;
				//factory->n_winning_captures++;
			}
			else {
				factory->losing_captures[factory->n_losing_captures++] = mvlist.moves + j;
				//factory->n_losing_captures++;
			}
		}
	}
}

static void generate_quiet_moves(const board_s* restrict board, movefactory_s* restrict factory) {

	factory->quiet_moves = &factory->moves[factory->moves_index];

	BitBoard pieces_copy = board->all_pieces[board->sidetomove];
	for (size_t i = 0; pieces_copy; i++) { // loop until pieces_copy with i counter
		const BitBoard piece = pop_bitboard(&pieces_copy);
		const unsigned int piece_index = lowest_bitindex(piece);

		// Select the from and to square masks

		// to-squares for capturing and en-passant
		BitBoard to_squares = ~board->all_pieces[OPPOSITE_SIDE(board->sidetomove)];
		
		// Ignore moves that have been generated already.
		to_squares &= ~factory->moves_generated[piece_index];
		
		if (!to_squares)
			continue;

		// Set up the movelist
		movelist_s mvlist;
		mvlist.moves = &factory->moves[factory->moves_index];

		// Generate the moves
		get_pseudo_legal_moves(board, &mvlist, piece, true, ~to_squares);
		factory->moves_index += mvlist.n;

		// Mark the current to-squares as generated
		factory->moves_generated[piece_index] |= to_squares;
		
		// Save the moves to quiet_moves
		factory->n_quiet_moves += mvlist.n;
	}
}




void init_movefactory(movefactory_s* restrict factory, BitBoard (*restrict killer_moves)[2][2], const uint16_t* restrict special_moves, const size_t n_special_moves) {
	
	memset(factory->moves_generated, 0, 64 * sizeof (BitBoard));
	factory->phase = 0;
	factory->moves_index = 0;
	factory->captures_generated = false;
	factory->quiet_moves_generated = false;
	factory->killer_moves = killer_moves;
	factory->killer_index = 0;
	factory->n_special_moves = 0;
	factory->n_promotions = 0;
	factory->n_winning_captures = 0;
	factory->n_losing_captures = 0;
	factory->n_quiet_moves = 0;
	factory->promotions_index = 0;
	factory->winning_captures_index = 0;
	factory->losing_captures_index = 0;
	factory->quiet_moves_index = 0;
}


move_s* get_next_move(const board_s* restrict board, movefactory_s* restrict factory) {
	
	GET_NEXT_MOVE_RESTART:

	switch (factory->phase) {

		// Special moves
		case 0:
			factory->phase++;
			goto GET_NEXT_MOVE_RESTART;

			// FIXME: Create the right promotion move according to the hash move
			// if (!hash_move || COMPACT_MOVE_PROMOTE_FLAG & hash_move)
			// 	goto GET_NEXT_MOVE_RESTART;


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

			return factory->winning_captures[factory->winning_captures_index++];

		// Killers
		case 2:

			if (factory->killer_index >= 2 || factory->killer_moves == NULL) {
				factory->phase++;
				goto GET_NEXT_MOVE_RESTART;
			}

			const BitBoard from_sq = board->all_pieces[board->sidetomove] & (*factory->killer_moves)[factory->killer_index][0];
			const BitBoard to_sq = (*factory->killer_moves)[factory->killer_index][1] & ~factory->moves_generated[lowest_bitindex(from_sq)];
			if (!from_sq || !to_sq || (to_sq & board->all_pieces[OPPOSITE_SIDE(board->sidetomove)])) {
				factory->killer_index++;
				goto GET_NEXT_MOVE_RESTART;
			}
			assert(popcount(from_sq) == 1);
			assert(popcount(to_sq) == 1);
			movelist_s mvlist = {.moves = &factory->moves[factory->moves_index]};
			get_pseudo_legal_moves(board, &mvlist, from_sq, false, ~to_sq);
			factory->killer_index++;
			if (!mvlist.n)
				goto GET_NEXT_MOVE_RESTART;
			assert(popcount(mvlist.n) == 1);
			factory->moves_generated[lowest_bitindex(mvlist.moves[0].from)] |= mvlist.moves[0].to;
			
			return &factory->moves[factory->moves_index++];

		// Best quiet moves
		case 3:
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

			return factory->losing_captures[factory->losing_captures_index++];
		
		// Rest of the quiet moves
		case 5:

			if (!factory->quiet_moves_generated) {
				generate_quiet_moves(board, factory);
				factory->quiet_moves_generated = true;
			}

			if (factory->quiet_moves_index >= factory->n_quiet_moves) {
				factory->phase++;
				goto GET_NEXT_MOVE_RESTART;
			}
			
			return &factory->quiet_moves[factory->quiet_moves_index++];


		
		default:
			return NULL;
	}

}

