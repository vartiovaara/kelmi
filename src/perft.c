#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <string.h>
#include <assert.h>

#include "search.h"

#include "movegen.h"
#include "bitboard.h"
#include "board.h"
#include "uci.h"
#include "movefactory.h"

#include "defs.h"



//https://oeis.org/A048987
//https://www.chessprogramming.org/Perft_Results#Initial_Position
const unsigned long long expected_perft[] = {
	1, // ply 0
	20,
	400,
	8902,
	197281,
	4865609,
	119060324,
	3195901860, // ply 7
	84998978956
};

#define PERFT_USE_MOVEFACTORY 0

#ifndef NDEBUG
move_s move_history[100];
#endif // NDEBUG

board_s boards[100];
board_s* restrict position = &boards[0];



// Private functions

void search(const unsigned int depth, const unsigned int ply, pertf_result_s* restrict res, FILE* restrict f);




void init_perft_result(pertf_result_s* res, unsigned int depth) {
	memset(res, 0, sizeof(pertf_result_s));
	res->n_plies = depth;

	res->n_positions = (unsigned long long*)calloc(depth+1, sizeof res->n_positions);
	res->captures = (unsigned long long*)calloc(depth+1, sizeof res->captures);
	res->checks = (unsigned long long*)calloc(depth+1, sizeof res->checks);
	res->en_passant = (unsigned long long*)calloc(depth+1, sizeof res->en_passant);
	res->checkmates = (unsigned long long*)calloc(depth+1, sizeof res->checkmates);
	res->stalemates = (unsigned long long*)calloc(depth+1, sizeof res->stalemates);
	res->castles = (unsigned long long*)calloc(depth+1, sizeof res->castles);
	res->promotions = (unsigned long long*)calloc(depth+1, sizeof res->promotions);
}

void free_perft_result(pertf_result_s* res) {
	free(res->n_positions);
	free(res->captures);
	free(res->checks);
	free(res->en_passant);
	free(res->checkmates);
	free(res->stalemates);
	free(res->castles);
	free(res->promotions);
}


void perft(board_s* board, const unsigned int depth) {
	printf("Starting perft with depth %u...\n\n", depth);

	// initialize perft_result_s
	pertf_result_s res;
	init_perft_result(&res, depth);

	//FILE* f = fopen("history", "w+");
	
	memcpy(position, board, sizeof (board_s));

	clock_t t = clock();

	//__builtin_prefetch(board, 1, 3);
	//__builtin_prefetch(board, 0, 3);
	search(depth, 0, &res, NULL);
	
	t = clock() - t;
	double time_taken = ((double)t)/CLOCKS_PER_SEC;

	//fclose(f);

	printf("\n%llu leaves\n", res.n_leaves);
	printf("%llu nodes searched in %3fs\n", res.nodes, time_taken);
	printf("%f Nps\n\n", (float)res.nodes/time_taken);

	for (unsigned int i = 0; i <= res.n_plies; i++) {
		//printf("Positions at depth %u: %llu \n", i, res.n_positions[i]);
		printf("Depth %u:\n", i);
		printf("Expected perft: %llu\n", expected_perft[i]);
		printf("Calculated perft: %llu\n", res.n_positions[i]);
		printf("Error: %lld\n", (long long)res.n_positions[i]-(long long)expected_perft[i]);
		printf("Procentual error: %f%%\n", ((double)((long long)res.n_positions[i]-(long long)expected_perft[i])/(double)expected_perft[i])*(double)100);
		printf("Captures: %lld\n", res.captures[i]);
		printf("Checks: %lld\n", res.checks[i]);
		printf("En passants: %lld\n", res.en_passant[i]);
		printf("Checkmates: %lld\n", res.checkmates[i]);
		printf("Stalemates: %lld\n", res.stalemates[i]);
		printf("Castles: %lld\n", res.castles[i]);
		printf("Promotions: %lld\n", res.promotions[i]);
		printf("\n");
	}

	free_perft_result(&res);
}


void perft_suite() {

	const char* positions[] = {
		"r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1",
		"r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1",
		// "r2q1rk1/pP1p2pp/Q4n2/bbp1p3/Np6/1B3NBn/pPPP1PPP/R3K2R b KQ - 0 1 ",
		"rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8",
		"r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 10",
		// "r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 b - - 0 10"
	};
	const unsigned int depths[] = {
		4,
		5,
		// 4,
		5,
		4,
		// 4
	};
	const unsigned long long expected_counts[] = {
		4085603,
		15833292,
		// 422333,
		89941194,
		3894594,
		// 3894594 
	};

	unsigned long long total_nodes = 0;
	long long total_expected_perft = 0;
	long long total_calculated_perft = 0;
	clock_t start_time = clock();


	for (unsigned int i = 0; i < LENGTH(positions); i++) {

		boards[0] = boardfromfen(positions[i]);
		position = &boards[0];

		const unsigned int depth = depths[i];

		//printf("\nStarting perft with depth %u...\n\n", depth);
		printboard(position);

		// initialize perft_result_s
		pertf_result_s res;
		init_perft_result(&res, depth);
		

		clock_t t = clock();

		search(depth, 0, &res, NULL);
		
		t = clock() - t;
		double time_taken = ((double)t)/CLOCKS_PER_SEC;


		printf("\n%llu calculated | %llu expected\n", res.n_positions[depth], expected_counts[i]);
		printf("%llu nodes searched in %3fs\n", res.nodes, time_taken);
		printf("%f Nps\n\n", (float)res.nodes/time_taken);

		total_calculated_perft += res.n_positions[depth];
		total_expected_perft += expected_counts[i];
		total_nodes += res.nodes;

		
		free_perft_result(&res);
	}

	const double time_taken = (double)(clock()-start_time)/CLOCKS_PER_SEC;

	printf("\n%llu calculated | %llu expected | error %lli\n", total_calculated_perft, total_expected_perft, total_calculated_perft-total_expected_perft);
	printf("%llu nodes searched in %3fs\n", total_nodes, time_taken);
	printf("%f Nps\n\n", (long long)total_nodes/time_taken);
}



/*
 * a recursive search algorithm.
 * so far just a prototype.
 * Check perft_result_s
 */
void search(const unsigned int depth, const unsigned int ply, pertf_result_s* restrict res, FILE* restrict f) {
	res->n_positions[res->n_plies - depth]++;

	// is position a last one
	if (depth == 0) {
		goto SEARCH_LAST_NODE;
	}


	// for distincting stalemate and checkmate
	const BitBoard initially_in_check = in_check(position);

	// When there is only one piece checking the king, calculate check-blocking moves
	BitBoard check_blocking_moves = 0x0;
	if (POPCOUNT(initially_in_check) == 1)
		check_blocking_moves = check_blocks(position, initially_in_check);
	
	bool single_checking_piece_is_pawn = (PAWNS(position) & check_blocking_moves);

	// for checking, if any moves were made
	// NOTE: Index might overflow and shit the bed
	const unsigned long long initial_nodes = res->n_positions[(res->n_plies - depth) + 1];

	// unsigned long long initial_leaves = res->n_leaves;

	// for checking if position is a checkmate
	unsigned long long skipped_because_of_checks = 0;

	//const unsigned int initial_side = board->sidetomove;

	// BitBoard pieces_copy = MOVING_PIECES(position);
	BitBoard pieces_copy = position->pm;

	// If multiple pieces are attacking the king, only do king moves
	if (POPCOUNT(initially_in_check) > 1)
		pieces_copy = KINGS(position) & position->pm;

	const unsigned int npieces = POPCOUNT(pieces_copy);

	// When not in check, skip the legality check of moves by these pieces
	BitBoard pinned = ~0x0;
	size_t pinned_allowed_squares_index = 0;
	if (!initially_in_check)
		pinned = pinned_pieces(position);

	//board_s boardcopy;// = *board;
	// memcpy(position + 1, position, sizeof (board_s)); // for some reason, it's faster with memcpy
	// position++;

	movelist_s moves;
#if PERFT_USE_MOVEFACTORY == 1
	movefactory_s factory;
	init_movefactory(&factory, NULL, NULL, 0);
	while (true) {
#else
	move_s moves_array[32];
	moves.moves = moves_array;
	// go through every piece
	for (unsigned int i = 0; i < npieces; i++) {
	// while (pieces_copy) {
#endif
#if PERFT_USE_MOVEFACTORY == 1
		moves.moves = get_next_move(board, &factory, false);
		if (!moves.moves)
			moves.n = 0;
		else
			moves.n = 1;
		
		if (!moves.n)
			break;
#else
		// generate moves
		const BitBoard piece = pop_bitboard(&pieces_copy);
		const unsigned int piece_type = PIECE_SQ(position, LOWEST_BITINDEX(piece));
		BitBoard ignore_to_squares = 0x0;
		// Do not try to block the check with the king, bad idea
		// if (check_blocking_moves && !(piece & KINGS(position)))
		if (check_blocking_moves && piece_type != KING)
			ignore_to_squares = ~check_blocking_moves;
		// Allow pawn captures even if the king is in check.
		// if (piece & PAWNS(position))
		// 	ignore_to_squares &= ~(WEST_ONE(piece << 8) | (EAST_ONE(piece << 8)));
		// if (single_checking_piece_is_pawn && piece & PAWNS(position))
		if (single_checking_piece_is_pawn && piece == PAWN)
			ignore_to_squares &= ~(check_blocking_moves<<8);

		get_pseudo_legal_moves(position, &moves, piece, false, ignore_to_squares);

		assert(moves.n < LENGTH(moves_array));
		
		// if there aren't any moves, cont now.
		// otherwise we'd be freeing memory that has never
		// been allocated
		if (!moves.n)
			continue;
#endif
		
		
		// go trough every move
		for (unsigned int j = 0; j < moves.n; j++) {
			// res->nodes++;


			// Check if castling is valid
			if (moves.moves[j].flags & (FLAG_KCASTLE | FLAG_QCASTLE)) {
				if (initially_in_check)
					continue; // can not castle while in check
				
				BitBoard target_squares;
				target_squares = (moves.moves[j].flags & FLAG_KCASTLE ? K_CASTLE_ATTACK_MASK : Q_CASTLE_ATTACK_MASK);
				
				// while (target_squares) {
				// 	if (is_attacking_square(position, pop_bitboard(&target_squares), ~position->pm))
				// 		goto SEARCH_SKIP_MOVE_PRE_MAKE;
				// }
				if (is_attacking_squares(position, target_squares, ~position->pm))
					goto SEARCH_SKIP_MOVE_PRE_MAKE;
				// if (opponent_attacks_squares(position, target_squares))
				// 	goto SEARCH_SKIP_MOVE_PRE_MAKE;
			}
			else if (piece & pinned
				// || piece & KINGS(position)
				|| piece_type == KING
				|| moves.moves[j].flags & FLAG_ENPASSANT
				|| initially_in_check) {

				if (is_move_illegal(position, moves.moves[j])) {
					skipped_because_of_checks++;
					continue;
				}
			}

			res->nodes++;

			memcpy(position + 1, position, sizeof (board_s));
			// *(position+1) = *position;
			position++;
			makemove(position, moves.moves[j]);

			// check if that side got itself in check (or couldn't get out of one)
			// if (is_attacking_squares(position, KINGS(position) & ~position->pm, position->pm)) {
			// //if (opponent_in_check(position)) {
			// 	skipped_because_of_checks++;
			// 	goto SEARCH_SKIP_MOVE;
			// }

#ifndef NDEBUG
			memcpy(move_history+ply, &moves.moves[j], sizeof (move_s));
#endif // NDEBUG



			// MOVE WILL BE DONE


			/*
			// En passant will be considered a capture only in stats.
			// See: https://www.chessprogramming.org/Perft_Results#Initial_Position
			if (moves.moves[j].flags & (FLAG_CAPTURE | FLAG_ENPASSANT))
				res->captures[(res->n_plies - depth)+1]++; // +1 becouse that move got itself to that depth so it will be counted as such
			
			// this statistic is expensive. only count it in debug builds
			#ifndef NDEBUG
			//if (board->side_in_check == board->sidetomove)
			if (is_attacking_squares(position, KINGS(position) & position->pm, ~position->pm))
				res->checks[(res->n_plies - depth)+1]++;
			#endif

			if (moves.moves[j].flags & FLAG_ENPASSANT)
				res->en_passant[(res->n_plies - depth)+1]++;
			
			if (moves.moves[j].flags & (FLAG_KCASTLE | FLAG_QCASTLE))
				res->castles[(res->n_plies - depth)+1]++;
			
			if (moves.moves[j].flags & FLAG_PROMOTE)
				res->promotions[(res->n_plies - depth)+1]++;
			*/


			// const unsigned long long initial_leaves = res->n_leaves;

			search(depth-1, ply+1, res, f);

			// if (__builtin_expect(res->n_plies - depth == 0, 0)) {
			if (ply == 0) {
			// if (res->n_plies - depth == 0) {
				char move_str[6] = "?";
				//move_to_uci_notation(&moves.moves[j], move_str);
				//printf("%s: %llu\n", move_str, res->n_positions[(res->n_plies - depth) + 1] - initial_nodes);
				// printf("%s: %llu\n", move_str, res->n_leaves - initial_leaves);
				putc('.', stdout);
				fflush(stdout);
			}

			SEARCH_SKIP_MOVE: // if move was illegal, go here
			// unmakemove(board, &moves.moves[j]);
			//memcpy(board, &boardcopy, sizeof (board_s)); // restore board
			position--;
			//*board = boardcopy;
			SEARCH_SKIP_MOVE_PRE_MAKE:
			continue;
		}

		// free(moves.moves);
	}
	
	// No moves were made?
	if (res->n_positions[(res->n_plies - depth) + 1] == initial_nodes) {
		if (initially_in_check) { // is a checkmate (was in check and can't get out of it)
			res->checkmates[res->n_plies - depth]++;
		}
		else { //is a stalemate (wasn't in check and no legal moves)
			res->stalemates[res->n_plies - depth]++;
		}
		goto SEARCH_LAST_NODE; // this position doesn't have any legal moves
	}

	return;

	// Go here if no more moves are made
	SEARCH_LAST_NODE:

	res->nodes++;
	res->n_leaves++;

#ifndef NDEBUG
	if (!f)
		return;

	// Write history to file
	write_move_history(move_history, ply, f);
#endif // NDEBUG
}
