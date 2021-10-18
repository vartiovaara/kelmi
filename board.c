/*
Stuff about boards and bitboards.
*/

#ifndef BOARD_C
#define BOARD_C

#include <stdint.h>
#include <ctype.h>
#include <stdio.h>
#include <string.h>

#include "defs.h"

const char piece_chars[N_PIECES] = {
	[KING] = 'k',
	[QUEEN] = 'q',
	[BISHOP] = 'b',
	[KNIGHT] = 'n',
	[ROOK] = 'r',
	[PAWN] = 'p'
};

void printboard(const board_s* board) {
	uint64_t pos = A8; // top-left
	do {
		for (int piece = 0; piece < N_PIECES; piece++) {
			if (pos & board->pieces[WHITE][piece]) {
				printf("%c ", tolower(piece_chars[piece]));
				goto PRINTBOARD_NO_PIECE_FOUND;
			}
			else if (pos & board->pieces[BLACK][piece]) {
				printf("%c ", toupper(piece_chars[piece]));
				goto PRINTBOARD_NO_PIECE_FOUND;
			}
		}
		printf("%c ", NO_PIECE_CHAR);
		PRINTBOARD_NO_PIECE_FOUND:
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

	char fen[MAX_FEN_LEN];
	strcpy(fen, fen_str);
	
	// split the fen into its fields
	char* pieces = strtok(fen, " ");
	char* movingside = strtok(NULL, " ");
	char* castling = strtok(NULL, " ");
	char* en_passant = strtok(NULL, " ");
	char* halfmove = strtok(NULL, " ");
	char* fullmove = strtok(NULL, " ");

	board_s board;
	resetboard(&board);

	// Parse piece positions
	uint64_t pos = A8;
	const int pieces_len = strlen(pieces);
	for (int i = 0; i < pieces_len; i++) {
		if (isalpha(pieces[i])) {
			int piececode = 0;
			switch (tolower(pieces[i])) {
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
			}
			const int side = (isupper(pieces[i]) ? WHITE : BLACK);
			board.pieces[side][piececode] |= pos;
			if (!(pos & RIGHT_MASK))
				pos <<= 1;
		}
		else if (isdigit(pieces[i])) {
			pos <<= (pieces[i] - '1');
		}
		else if (pieces[i] == '/')
			pos >>= 15;
	}

#ifndef NDEBUG
	printf("%s\n", pieces);
	printf("%s\n", movingside);
	printf("%s\n", castling);
	printf("%s\n", en_passant);
	printf("%s\n", halfmove);
	printf("%s\n", fullmove);
#endif // NDEBUG

	return board;
}

void resetboard(board_s* board) {
	// we could just memset() the whole thing but idk
	for (int side = 0; side < 2; side++) {
		for (int piece_type = 0; piece_type < 6; piece_type++) {
			board->pieces[side][piece_type] = 0;
		}
		board->all_pieces[side] = 0;
	}
	board->whiteturn = true;
}

// flips a bitboard 90 degrees
// see:
// https://www.chessprogramming.org/Flipping_Mirroring_and_Rotating#Vertical
uint64_t bbverticalflip(uint64_t bb) {
	const uint64_t k1 = 0x00FF00FF00FF00FF;
	const uint64_t k2 = 0x0000FFFF0000FFFF;
	bb = ((bb >>  8) & k1) | ((bb & k1) <<  8);
	bb = ((bb >> 16) & k2) | ((bb & k2) << 16);
	bb = ( bb >> 32)       | ( bb       << 32);
	return bb;
}

#endif // BOARD_C