/*
Stuff about boards and bitboards.
*/

#ifndef BOARD_C
#define BOARD_C

#include <stdint.h>
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include <assert.h>

#include "board.h"

#include "bitboard.h"
#include "algebraic.h"
#include "lookup.h"
#include "movegen.h"

#include "defs.h"



//void makemove_pawn(board_s* restrict board, const move_s* restrict move);
// static void movepiece(board_s* board, const unsigned int type, const BitBoard from, const BitBoard to);
// static void removepiece(board_s* board, const BitBoard pos, const unsigned int side, const unsigned int type);
// static void addpiece(board_s*, const BitBoard pos, const unsigned int side, const unsigned int type);
// static void move_castling_pieces(board_s* restrict board, const move_s* restrict move, const bool undo);



void printboard(const board_s* board) {
	BitBoard pos = A8; // top-left
	do {
		char ch = NO_PIECE_CHAR;
		// for (int piece = EMPTY; piece < PIECE_LAST; piece++) {
		// 	if (pos & PIECES(board, piece) & board->pm) {
		// 		ch = toupper(piecetochar(piece));
		// 		break;
		// 	}
		// 	else if (pos & PIECES(board, piece)) {
		// 		ch = tolower(piecetochar(piece));
		// 		break;
		// 	}
		// }
		if (pos & OCCUPANCY(board)) {
			ch = piecetochar(PIECE_SQ(board, LOWEST_BITINDEX(pos)));
			if (pos & board->pm)
				ch = toupper(ch);
			else
				ch = tolower(ch);
		}

		printf("%c ", ch);
		// check if pos is on h-file and nl
		if (pos & RIGHT_MASK) {
			printf("\n");
			pos >>= 15;
		}
		else
			pos <<= 1;
	} while (pos > 0);
}


void printbitboard(const BitBoard bb) {
	BitBoard pos = A8; // top-left
	do {
		char ch = ((pos & bb) ? '1' : '0');
		printf("%c ", ch);
		// check if pos is on h-file and nl
		if (pos & RIGHT_MASK) {
			printf("\n");
			pos >>= 15;
		}
		else
			pos <<= 1;
	} while (pos > 0);
	printf("%p \n", (void*)bb);
}


unsigned int piece_from_char(const unsigned char c) {
	unsigned int piece;

	switch (c) {
		case 'k':
			piece = KING;
			break;
		case 'q':
			piece = QUEEN;
			break;
		case 'r':
			piece = ROOK;
			break;
		case 'b':
			piece = BISHOP;
			break;
		case 'n':
			piece = KNIGHT;
			break;
		case 'p':
			piece = PAWN;
			break;
		default:
			fprintf(stderr, "Errer in piece_form_char(): piece char wrong. Got: %c\n", c);
			abort();
	}

	return piece;
}


board_s boardfromfen(const char* fen_str, enum side_e* computer_side) {
	// TODO: have a FEN validation function as the
	// behaviour of this function is undefined with invalid FENs

	assert(strlen(fen_str) <= MAX_FEN_LEN);

	char fen[MAX_FEN_LEN];
	strcpy(fen, fen_str);
	
	// split the fen into its fields
	char* pos_str = strtok(fen, " ");
	const char* movingside = strtok(NULL, " ");
	const char* castling = strtok(NULL, " ");
	const char* en_passant = strtok(NULL, " ");
	const char* halfmove = strtok(NULL, " ");
	const char* fullmove = strtok(NULL, " ");

	assert(strlen(pos_str) > 0);
	assert(strlen(movingside) > 0);
	assert(strlen(castling) > 0);
	assert(strlen(en_passant) > 0);
	assert(strlen(halfmove) > 0);
	assert(strlen(fullmove) > 0);

	// split the position string in its fields
	const char* pos_row[8];
	pos_row[0] = strtok(pos_str, "/");
	for (int i = 1; i < 8; i++)
		pos_row[i] = strtok(NULL, "/");

	board_s board;
	resetboard(&board);



	// Parse piece positions
	for (int y = 0; y < 8; y++) {
		BitBoard pos = SQTOBB((7-y)*8);
		const int row_len = strlen(pos_row[y]);
		for (int i = 0; i < row_len; i++) {
			if (isalpha(pos_row[y][i])) {
				int piececode = 0;
				switch (tolower(pos_row[y][i])) {
					case 'k':
						piececode = KING;
						break;
					case 'q':
						piececode = QUEEN;
						break;
					case 'b':
						piececode = BISHOP;
						break;
					case 'n':
						piececode = KNIGHT;
						break;
					case 'r':
						piececode = ROOK;
						break;
					case 'p':
						piececode = PAWN;
						break;
					default:
						assert(0);
				}
				//const int is_movers_piece = (bool)(isupper(pos_row[y][i]) ? (moving_side==WHITE) : (moving_side==BLACK));
				ADD_PIECE(&board, pos, isupper(pos_row[y][i])>=1, piececode);
				pos <<= 1;
			}
			else if (isdigit(pos_row[y][i])) {
				pos <<= (pos_row[y][i] - '0');
			}
		}
	}

	// Parse side to move
	
	unsigned int moving_side;

	if (tolower(movingside[0]) == 'w')
		moving_side = WHITE;
	else
		moving_side = BLACK;

	// Parse castling ability
	if (strchr(castling, 'K'))
		board.castling |= MKCASTLE;
	// board.castling |= (moving_side == WHITE ? (MKCASTLE) : (OKCASTLE));
	if (strchr(castling, 'Q'))
		board.castling |= MQCASTLE;
	// board.castling |= (moving_side == WHITE ? (MQCASTLE) : (OQCASTLE));
	if (strchr(castling, 'k'))
		board.castling |= OKCASTLE;
	// board.castling |= (moving_side == BLACK ? (MKCASTLE) : (OKCASTLE));
	if (strchr(castling, 'q'))
		board.castling |= OQCASTLE;
	// board.castling |= (moving_side == BLACK ? (MQCASTLE) : (OQCASTLE));
	
	// Parse en passant
	if (en_passant[0] != '-')
		board.en_passant = algsqtoint(en_passant) % 8;
	else
		board.en_passant = 8; // no en passant
	
	// Move counters
	board.fiftym_counter = strtoul(halfmove, NULL, 10);
	//board.fullmoves = strtoul(fullmove, NULL, 10);

	// set board hash
	//board.hash = calculate_board_hash(&board);

	// set side in check
	// if (is_in_check(&board, WHITE))
	// 	board.side_in_check = WHITE;
	// else if (is_in_check(&board, BLACK))
	// 	board.side_in_check = BLACK;
	// else
	// 	board.side_in_check = SIDE_NONE;

	if (moving_side == BLACK)
		FLIP_BOARD(&board);
	
	if (moving_side == WHITE)
		*computer_side = WHITE;
	else
		*computer_side = BLACK;


	/*
#ifndef NDEBUG
	printf("Start FEN parsing debug info\n");
	for (int i = 0; i < 8; i++)
		printf("%s\n", pos_row[i]);
	printf("%s: %s\n", movingside, (board.whiteturn ? "white" : "black"));
	printf("%s: %p\n", castling, (void*)(size_t)board.castling);
	printf("%s: %p\n", en_passant, (void*)board.en_passant);
	printf("%s: %u\n", halfmove, board.fiftym_counter);
	printf("%s: %u\n", fullmove, board.fullmoves);
	printf("End FEN parsing debug info\n");
#endif // NDEBUG
	*/
	return board;
}


void resetboard(board_s* board) {
	memset(board, 0, sizeof (board_s));
	board->en_passant = 8; // no en passant
}


/*
TODO: Test the eligibility of this function
*/
// static void movepiece(board_s* board, const unsigned int type, const BitBoard from, const BitBoard to) {
// 	assert(popcount(to) == 1);
// 	assert(popcount(from) == 1);
// 	assert(to ^ board->every_piece); //??

// 	assert(board->every_piece == (board->all_pieces[WHITE] | board->all_pieces[BLACK]));

// 	assert(!(board->every_piece & to));

// 	// Otherwise we'd need to have a 4th argument
// 	unsigned int side = get_piece_side(board, from);
	
	
// 	// FIXME: Why the fuck would the ^= (from | to) not work. Crash at Fine #70
// 	removepiece(board, from, side, type);
// 	addpiece(board, to, side, type);
// 	//board->pieces[side][type] ^= from | to;
// 	//board->all_pieces[side] ^= from | to;
// 	//board->every_piece ^= from | to;
// 	//board->hash ^= hash_rand_piece[side][type][lowest_bitindex(from)];
// 	//board->hash ^= hash_rand_piece[side][type][lowest_bitindex(to)];

// 	//assert(board->every_piece == (board->all_pieces[WHITE] | board->all_pieces[BLACK]));
// }


// static void removepiece(board_s* board, const BitBoard pos, const unsigned int side, const unsigned int type) {
// 	assert(popcount(pos) == 1);
// 	assert(side == WHITE || side == BLACK);
// 	assert(type < N_PIECES);
// 	assert(board->pieces[side][type] & pos);

// 	// board->pieces[side][type] &= ~pos;
// 	// board->all_pieces[side] &= ~pos;
// 	// board->every_piece &= ~pos;
// 	// //board->pieces[side][type] ^= pos;
// 	// //board->all_pieces[side] ^= pos;
// 	// //board->every_piece ^= pos;
// 	// board->hash ^= hash_rand_piece[side][type][lowest_bitindex(pos)];
// }


// static void addpiece(board_s* board, const BitBoard pos, const unsigned int side, const unsigned int type) {
// 	assert(popcount(pos) == 1);
// 	assert(side == WHITE || side == BLACK);
// 	assert(type < N_PIECES);
// 	assert(board->pieces[side][type] ^ pos); // check that there wasn't a piece already

// 	//board->hash ^= hash_rand_piece[side][type][lowest_bitindex(pos)];
// }

/*
void makemove_pawn(board_s* restrict board, const move_s* restrict move) {
	// If move was a promotion, remove it and add the relevant piece to its place
	if (move->flags & FLAG_PROMOTE) {
		assert(move->fromtype == PAWN);

		removepiece(board, move->from, move->side, PAWN);
		addpiece(board, move->to, move->side, move->promoteto);
		//pieces_moved = true;
		return;
	}
	
	// Set en passant target square
	if (move->flags & FLAG_DOUBLEPUSH) {
		board->en_passant = (move->side==WHITE ? MV_S(move->to, 1) : MV_N(move->to, 1));
	}
}
*/



// TODO: Finish this function
void makemove(board_s* restrict board, const move_s move) {
	// assert(popcount(board->en_passant) <= 1);
	CHECK_BOARD(board);

	assert(SQTOBB(move.from) & OCCUPANCY(board) & board->pm);

	const BitBoard frombb = SQTOBB(move.from);
	const BitBoard tobb = SQTOBB(move.to);

	board->en_passant = 8; // Reset en passant

	if (move.flags & FLAG_KCASTLE) {
		board->pm^= (BitBoard)0x00000000000000A0 | K_CASTLE_KING_TARGET | M_KING_DEFAULT_POS; // rook and king
		board->p2^= (BitBoard)0x00000000000000A0 | K_CASTLE_KING_TARGET | M_KING_DEFAULT_POS; // rook and king
		board->p1^= M_KING_DEFAULT_POS | K_CASTLE_KING_TARGET; // king
		board->castling &= ~(MKCASTLE | MQCASTLE);
		FLIP_BOARD(board);
		CHECK_BOARD(board);
		return;
	}
	if (move.flags & FLAG_QCASTLE) {
		board->pm^= (BitBoard)0x0000000000000009 | Q_CASTLE_KING_TARGET | M_KING_DEFAULT_POS; // rook and king
		board->p2^= (BitBoard)0x0000000000000009 | Q_CASTLE_KING_TARGET | M_KING_DEFAULT_POS; // rook and king
		board->p1^= M_KING_DEFAULT_POS | Q_CASTLE_KING_TARGET; // king
		board->castling &= ~(MKCASTLE | MQCASTLE);
		FLIP_BOARD(board);
		CHECK_BOARD(board);
		return;
	}

	// Remove piece from to
	// if (move.flags & FLAG_CAPTURE) {
	
	assert(!(tobb & KINGS(board)));

	REMOVE_PIECE(board, tobb);
	// Capture opponent rook?
	if (move.to == 56)
		board->castling &= ~OQCASTLE;
	if (move.to == 63)
		board->castling &= ~OKCASTLE;
	// board->castling &= ~(((move.to == 56) * OQCASTLE) | ((move.to == 63) * OKCASTLE));
	// }

	
	// Copy piece from --> to

	unsigned int type = PIECE_SQ(board, move.from);

	assert(type > EMPTY);
	assert(type < PIECE_LAST);

	// if (type == KING)
	// 	board->castling &= ~(MKCASTLE | MQCASTLE);
	// else if (move.from == 0)
	// 	board->castling &= ~MQCASTLE;
	// else if (move.from == 7)
	// 	board->castling &= ~MKCASTLE;

	// const uint8_t higher_castle_bits = board->castling & (OQCASTLE | OKCASTLE);
	// board->castling = ((MQCASTLE | MKCASTLE) * (type == KING));
	// board->castling |= (((move.from == 0) * MQCASTLE) | ((move.from == 7) * MKCASTLE));
	// board->castling = ~board->castling;
	// board->castling |= higher_castle_bits;

	board->castling &= ~((MQCASTLE | MKCASTLE) * (type == KING));
	board->castling &= ~(((move.from == 0) * MQCASTLE) | ((move.from == 7) * MKCASTLE));

	// const bool is_king = type == KING;
	// const bool remove_q_cast = move.from == 0;
	// const bool remove_k_cast = move.from == 7;
	// board->castling &= ~((MQCASTLE | MKCASTLE) * is_king);
	// board->castling &= ~((remove_q_cast*MQCASTLE) | (remove_k_cast*MKCASTLE));

	
	REMOVE_PIECE(board, SQTOBB(move.from));
	
	if (move.flags & FLAG_PROMOTE)
		type = move.promoteto;
	if (move.flags & FLAG_ENPASSANT)
		board->p0 ^= MV_S(tobb, 1);
	if (move.flags & FLAG_DOUBLEPUSH)
		board->en_passant = move.to % 8;

	
	ADD_PIECE(board, tobb, 1, type);

	
	FLIP_BOARD(board);
	
	CHECK_BOARD(board);
}



/*
// TODO: Finish this function
void unmakemove(board_s* restrict board, const move_s* restrict move) {
	// Get the reference to the last move
	//const move_s* move = board->movehistory.moves + (board->history_n - 1);


	// remove castling from zobrish hash
	board->hash ^= hash_rand_castle[board->castling];

	// remove enpassant sqare from zobrist hash
	if (board->en_passant)
		board->hash ^= hash_rand_enpassant[lowest_bitindex(board->en_passant)];
	
	// remove sidetomove from zobrist hash
	board->hash ^= hash_rand_sidetomove[board->sidetomove];

	//if (!move)
	if (!move->from && !move->to)
		goto UNMAKEMOVE_NULLMOVE;

	// restore a normal move
	// const uint8_t non_normal_move_flag_mask = FLAG_CAPTURE | FLAG_KCASTLE | FLAG_QCASTLE | FLAG_ENPASSANT | FLAG_PROMOTE;
	// if (!(move->flags & non_normal_move_flag_mask)) {
	// 	movepiece(board, move->fromtype, move->to, move->from);
	// 	goto UNMAKEMOVE_PIECES_MOVED;
	// }
	
	// restore a promote, works even if move was capture
	if (move->flags & FLAG_PROMOTE) {
		removepiece(board, move->to, move->side, move->promoteto); // remove to
		addpiece(board, move->from, move->side, move->fromtype); // add to from
		if (move->flags & FLAG_CAPTURE)
			addpiece(board, move->to, OPPOSITE_SIDE(move->side), move->piece_captured); // add back captured
		goto UNMAKEMOVE_PIECES_MOVED;
	}
	else if (move->flags & (FLAG_KCASTLE | FLAG_QCASTLE)) { // restore a castle
		move_castling_pieces(board, move, true);
		goto UNMAKEMOVE_PIECES_MOVED;
	}
	else if (move->flags & FLAG_CAPTURE) { // restore a capture, when move was not a promotion
		movepiece(board, move->fromtype, move->to, move->from);
		addpiece(board, move->to, OPPOSITE_SIDE(move->side), move->piece_captured);
		goto UNMAKEMOVE_PIECES_MOVED;
	}
	else if (move->flags & FLAG_ENPASSANT) { // restore en passant
		const BitBoard piece_removed = (move->side == WHITE ? MV_S(move->to, 1) : MV_N(move->to, 1));
		movepiece(board, PAWN, move->to, move->from);
		addpiece(board, piece_removed, OPPOSITE_SIDE(move->side), PAWN);
		goto UNMAKEMOVE_PIECES_MOVED;
	}
	else {
		movepiece(board, move->fromtype, move->to, move->from);
		goto UNMAKEMOVE_PIECES_MOVED;
	}


	UNMAKEMOVE_PIECES_MOVED:

	// Restore old en passant square
	board->en_passant = move->old_en_passant;

	// Restore castling rights
	board->castling = move->old_castling_flags;

#ifndef NDEBUG
	// Decrease history size
	board->history_n--;
#endif // NDEBUG

	// Decrement fullmove clock
	board->fullmoves -= (move->side == BLACK);

	board->sidetomove = OPPOSITE_SIDE(board->sidetomove);

	// add castling to zobrish hash
	board->hash ^= hash_rand_castle[board->castling];

	// add enpassant sqare to zobrist hash
	if (board->en_passant)
		board->hash ^= hash_rand_enpassant[lowest_bitindex(board->en_passant)];
	
	// add sidetomove to zobrist hash
	board->hash ^= hash_rand_sidetomove[board->sidetomove];

	board->rep_stack_n--;

	assert(calculate_board_hash(board) == board->hash);

	return;



	UNMAKEMOVE_NULLMOVE:

	board->sidetomove = OPPOSITE_SIDE(board->sidetomove);

#ifndef NDEBUG
	board->history_n--;
#endif // NDEBUG

	board->castling = move->old_castling_flags;
	board->en_passant = move->old_en_passant;

	// add castling to zobrish hash
	board->hash ^= hash_rand_castle[board->castling];

	// add enpassant sqare to zobrist hash
	if (board->en_passant)
		board->hash ^= hash_rand_enpassant[lowest_bitindex(board->en_passant)];
	
	// add sidetomove to zobrist hash
	board->hash ^= hash_rand_sidetomove[board->sidetomove];

	assert(calculate_board_hash(board) == board->hash);
}
*/


// unsigned int get_piece_type(const board_s* board, const unsigned int side, const BitBoard piecebb) {
// 	assert(side == WHITE || side == BLACK);
// 	assert(popcount(piecebb) == 1);
// 	assert(piecebb & board->all_pieces[side]);

// 	for (int i = 0; i < N_PIECES; i++) {
// 		if (board->pieces[side][i] & piecebb)
// 			return i;
// 	}
// 	// should never get here
// 	fprintf(stderr, "get_piece_type(board, %u, %p)\n", side, (void*)piecebb);
// 	assert(0);
// 	exit(1);
// }


// unsigned int get_piece_side(const board_s* board, const BitBoard piecebb) {
// 	assert(popcount(piecebb) == 1);
// 	assert(board->every_piece & piecebb);

// 	if (board->all_pieces[WHITE] & piecebb)
// 		return WHITE;
// 	return BLACK;
// }



// TODO: Test the eligibility of this logic and the constants.
// static void move_castling_pieces(board_s* restrict board, const move_s* restrict move, const bool undo) {
	// assert(popcount(move->flags & (FLAG_KCASTLE | FLAG_QCASTLE)) == 1); // ensure only 1 of the flags is set
	

	/*
	if (!undo) {
		// Revoke castling sides castling perms
		if (move->side == WHITE) //(WKCASTLE | WQCASTLE))
			board->castling &= ~(WKCASTLE | WQCASTLE);
		else
			board->castling &= ~(BKCASTLE | BQCASTLE);
	}
	*/
/*
	BitBoard king_from = 0x0;
	BitBoard king_to = 0x0;
	BitBoard rook_from = 0x0;
	BitBoard rook_to = 0x0;

	if (move->side == WHITE) {
		if (move->flags == FLAG_KCASTLE) { //WKCASTLE) {
			if (!undo) {
				assert(board->pieces[WHITE][KING] == W_KING_DEFAULT_POS);
				assert(board->pieces[WHITE][ROOK] & H1); // make sure there exists a rook at H1
			}

			king_from = W_KING_DEFAULT_POS;
			king_to = WK_CASTLE_KING_TARGET;

			rook_from = H1;
			rook_to = WK_CASTLE_ROOK_TARGET;
		}
		else { //(move->flags == WQCASTLE) {
			if (!undo) {
				assert(board->pieces[WHITE][KING] == W_KING_DEFAULT_POS);
				assert(board->pieces[WHITE][ROOK] & A1); // make sure there exists a rook at A1
			}

			king_from = W_KING_DEFAULT_POS;
			king_to = WQ_CASTLE_KING_TARGET;

			rook_from = A1;
			rook_to = WQ_CASTLE_ROOK_TARGET;
		}
	}
	else {
		if (move->flags == FLAG_KCASTLE) { // BKCASTLE) {
			if (!undo) {
				assert(board->pieces[BLACK][KING] == B_KING_DEFAULT_POS);
				assert(board->pieces[BLACK][ROOK] & H8); // make sure there exists a rook at H8
			}

			king_from = B_KING_DEFAULT_POS;
			king_to = BK_CASTLE_KING_TARGET;

			rook_from = H8;
			rook_to = BK_CASTLE_ROOK_TARGET;
		}
		else { // BQCASTLE
			if (!undo) {
				assert(board->pieces[BLACK][KING] == B_KING_DEFAULT_POS);
				assert(board->pieces[BLACK][ROOK] & A8); // make sure there exists a rook at H8
			}

			king_from = B_KING_DEFAULT_POS;
			king_to = BQ_CASTLE_KING_TARGET;

			rook_from = A8;
			rook_to = BQ_CASTLE_ROOK_TARGET;
		}
	}

	// Move rook and king
	if (undo) {
		movepiece(board, KING, king_to, king_from);
		movepiece(board, ROOK, rook_to, rook_from);
		return;
	}
	movepiece(board, KING, king_from, king_to);
	movepiece(board, ROOK, rook_from, rook_to);
}
*/

/*
// Movehistory is not used in non-debug builds
#ifndef NDEBUG

void set_move_history_size(board_s* board, const size_t size) {
	assert(size > 0);

	move_s* new_block = (move_s*)realloc(board->movehistory.moves, sizeof (move_s) * size);

	if (!new_block) {
		printf("No memory to realloc move history! Aborting... \n");
		abort();
	}

	board->movehistory.moves = new_block;
	board->movehistory.n = size;
}


void append_to_move_history(board_s* board, const move_s* move) {
	board->history_n++;

	if (board->movehistory.n < board->history_n) {
		set_move_history_size(board, board->history_n);
	}

	if (move) {
		memcpy(&board->movehistory.moves[board->history_n-1], move, sizeof (move_s));
		return;
	}

	// Mark move as null move
	board->movehistory.moves[board->history_n-1].from = 0x0;
	board->movehistory.moves[board->history_n-1].to = 0x0;
	board->movehistory.moves[board->history_n-1].old_en_passant = board->en_passant;
}


void free_move_history(const board_s* board) {
	free(board->movehistory.moves);
}


void restore_board(board_s* restrict to, board_s* restrict from) {
	movelist_s movehistory = to->movehistory;

	memcpy(to, from, sizeof (board_s)); // restore board
	to->movehistory = movehistory; // restore old movehistory
}
*/

void write_move_history(const move_s* restrict moves, const size_t n, FILE* restrict f) {
	
	for (unsigned int i = 0; i < n; i++) {
		
		char from[2];
		BitBoard frombb = SQTOBB(moves[i].from);
		if (i % 2 != 0)
			frombb = FLIP_VERTICAL(frombb);

		bbtoalg(from, frombb);

		char to[2];
		BitBoard tobb = SQTOBB(moves[i].to);
		if (i % 2 != 0)
			tobb = FLIP_VERTICAL(tobb);
		bbtoalg(to, tobb);

		char promote[3];
		if (moves[i].flags & FLAG_PROMOTE) {
			promote[0] = piecetochar(moves[i].promoteto);
			promote[1] = ' ';
			promote[2] = '\0';
		}
		else {
			promote[0] = ' ';
			promote[1] = '\0';
		}

		fprintf(f, "%c%c%c%c%s", from[0], from[1], to[0], to[1], promote);
	}
	fputs("\n", f);
}

/*
#endif // NDEBUG


// TODO: Test eligibility
uint64_t calculate_board_hash(const board_s* board) {
	assert(board->sidetomove == 0 || board->sidetomove == 1);
	assert(popcount(board->en_passant) < 2);
	assert(board->castling < 2*2*2*2);


	// sidetomove
	uint64_t hash = hash_rand_sidetomove[board->sidetomove];

	// pieces
	for (int i = 0; i < 2; i++) {
		for (int j = 0; j < N_PIECES; j++) {
			BitBoard pieces = board->pieces[i][j];
			while (pieces) {
				BitBoard piece = pop_bitboard(&pieces);
				hash ^= hash_rand_piece[i][j][lowest_bitindex(piece)];
			}
		}
	}

	// enpassant
	if (board->en_passant)
		hash ^= hash_rand_enpassant[lowest_bitindex(board->en_passant)];

	// castling
	hash ^= hash_rand_castle[board->castling];

	return hash;
}
*/


#endif // BOARD_C
