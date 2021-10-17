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
	// TODO: test which of the implementations works and choose one

	uint64_t pos = SQTOBB(7*8); // a8, >>1 so that loop 1 is correct
	uint64_t endsq = SQTOBB(7); // h1
	while (1) {
		// Loop through all of the bitboards of both sides
		for (int piece = 0; piece < N_PIECES; piece++) {
			// Expects that both sides do not occupy
			// the same square
			if (pos & board->pieces[WHITE][piece]) {
				printf("%c", toupper(piece_chars[piece]));
				break;
			}
			else if (pos & board->pieces[BLACK][piece]) {
				printf("%c", tolower(piece_chars[piece]));
				break;
			}
			else {
				printf("%c", NO_PIECE_CHAR);
				break;
			}
		}
		if (pos & endsq)
			break;
		if (pos & RIGHT_MASK) {
			pos >>= 15;
			printf("\n");
		}
		else
			pos <<= 1;
	}
	printf("\n");
	/*
	// starting at y=7 becouse we stop at 0 
	for (int y = 7; y >= 0; y--) {
		//for (uint64_t pos = SQTOBB(y*8); pos & ~RIGHT_MASK; pos <<= 1) {
		uint64_t pos = SQTOBB(y*8)>>1; // >>1 so that at first loop we start right
		do {
			pos <<= 1;
			//printf("%p\n", (void*)pos);
			// Loop through all of the bitboards of both sides
			for (int piece = 0; piece < N_PIECES; piece++) {
				// Expects that both sides do not occupy
				// the same square
				if (pos & board->pieces[WHITE][piece]) {
					printf("%c", toupper(piece_chars[piece]));
					break;
				}
				else if (pos & board->pieces[BLACK][piece]) {
					printf("%c", tolower(piece_chars[piece]));
					break;
				}
				else {
					printf("%c", NO_PIECE_CHAR);
					break;
				}
			}
		} while (pos & ~RIGHT_MASK);
		printf("\n");
	}*/
}

board_s boardfromfen(const char* fen_str) {
	// TODO: have a FEN validation function as the
	// behaviour of this function is undefined with invalid FENs

	char fen[MAX_FEN_LEN];
	strcpy(fen, fen_str);
	
	// split the fen into its fields
	

	board_s board;

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

#endif