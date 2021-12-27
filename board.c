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

#include "defs.h"

#include <assert.h>

const char piece_chars[N_PIECES] = {
	[KING] = 'k',
	[QUEEN] = 'q',
	[BISHOP] = 'b',
	[KNIGHT] = 'n',
	[ROOK] = 'r',
	[PAWN] = 'p'
};

/*
Prints the boards layout with symbols
defined in piece_chars[N_PIECES]
*/
void printboard(const board_s* board) {
	uint64_t pos = A8; // top-left
	do {
		char ch = NO_PIECE_CHAR;
		for (int piece = 0; piece < N_PIECES; piece++) {
			if (pos & board->pieces[WHITE][piece]) {
				ch = toupper(piece_chars[piece]);
				break;
			}
			else if (pos & board->pieces[BLACK][piece]) {
				ch = tolower(piece_chars[piece]);
				break;
			}
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

void printbitboard(const uint64_t bb) {
	uint64_t pos = A8; // top-left
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

board_s boardfromfen(const char* fen_str) {
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
		uint64_t pos = SQTOBB((7-y)*8);
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
				const int side = (isupper(pos_row[y][i]) ? WHITE : BLACK);
				board.pieces[side][piececode] |= pos;
				board.all_pieces[side] |= pos;
				pos <<= 1;
			}
			else if (isdigit(pos_row[y][i])) {
				pos <<= (pos_row[y][i] - '0');
			}
		}
	}

	// Parse side to move
	if (tolower(movingside[0]) == 'w')
		board.sidetomove = WHITE;
	else
		board.sidetomove = BLACK;
	
	// Parse castling ability
	if (strchr(castling, 'K'))
		board.castling |= WKCASTLE;
	if (strchr(castling, 'Q'))
		board.castling |= WQCASTLE;
	if (strchr(castling, 'k'))
		board.castling |= BKCASTLE;
	if (strchr(castling, 'q'))
		board.castling |= BQCASTLE;
	
	// Parse en passant
	if (en_passant[0] != '-')
		board.en_passant = algsqtobb(en_passant);
	else
		board.en_passant = 0; // no en passant
	
	// Move counters
	board.fiftym_counter = strtoul(halfmove, NULL, 10);
	board.fullmoves = strtoul(fullmove, NULL, 10);

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
	// we could just memset() the whole thing but idk
	memset(board, 0, sizeof (board_s));
	/*
	for (int side = 0; side < 2; side++) {
		for (int piece_type = 0; piece_type < 6; piece_type++) {
			board->pieces[side][piece_type] = 0;
		}
		board->all_pieces[side] = 0;
	}
	board->sidetomove = WHITE;
	board->movehistory = NULL;
	board->ply = 0
	*/
}

/*
Moves a piece from from to to
Doesn't "perform" a move (change en_passant, whiteturn etc.)
TODO: Test the eligibility of this function
*/
void movepiece(board_s* board, const unsigned int side, const uint64_t from, const uint64_t to) {
	// Otherwise we'd need to have a 4th argument
	unsigned int type = get_piece_type(board, side, from);
	
	// Change piece bitboards
	board->pieces[side][type] &= ~from; // remove from
	board->pieces[side][type] |= to; // add to
	board->all_pieces[side] &= ~from;
	board->all_pieces[side] |= to;
}

// Performs a move
// TODO: Finish this function
void makemove(board_s* board, const move_s* move) {
	movepiece(board, board->sidetomove, move->from, move->to);
	//if (move.flags & FLAGCAPTURE)
	// change side to move
	board->sidetomove = (board->sidetomove == WHITE ? BLACK : WHITE);
}

// Undoes the latest move done
// TODO: Finish this function
void unmakemove(board_s* board) {
	board->sidetomove = (board->sidetomove == WHITE ? BLACK : WHITE);
}

// Finds, which one of the bitboards holds the piece.
// exit(1) on not found
unsigned int get_piece_type(const board_s* board, const unsigned int side, const uint64_t piecebb) {
	assert(side == WHITE || side == BLACK);
	assert(popcount(piecebb) == 1);
	assert(piecebb & board->all_pieces[side]);
	for (int i = 0; i < N_PIECES; i++) {
		if (board->pieces[side][i] & piecebb)
			return i;
	}
	// should never get here
	fprintf(stderr, "get_piece_type(board, %u, %p)\n", side, (void*)piecebb);
	exit(1);
}

// Returns, what side the piece is
// exit(1) on not found
unsigned int get_piece_side(const board_s* board, const uint64_t piecebb) {
	assert((board->all_pieces[WHITE] | board->all_pieces[BLACK]) & piecebb);
	assert(popcount(piecebb) == 1);

	if (board->all_pieces[WHITE] & piecebb)
		return WHITE;
	return BLACK;
}



#endif // BOARD_C
