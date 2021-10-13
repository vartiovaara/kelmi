/*
All of the shit needed and stuff idk.
*/

#ifndef DEFS_H
#define DEFS_H

#include <stdint.h>
#include <stdbool.h>

enum piece_e {
	KING,
	QUEEN,
	ROOK,
	BISHOP,
	KNIGHT,
	PAWN,
	EMPTY_PIECE
};

enum color_e {
	WHITE,
	BLACK,
	EMPTY_COLOR
};

enum castle_e {
	CASTLE_WK = 1,
	CASTLE_WQ = 2,
	CASTLE_BK = 4,
	CASTLE_BQ = 8
};

static const char piece_char[] = {
	[KING] = 'k',
	[QUEEN] = 'q',
	[ROOK] = 'r',
	[BISHOP] = 'b',
	[KNIGHT] = 'n',
	[PAWN] = 'p',
	[EMPTY_PIECE] = '.'
};

// Represents a board with
// all of its necessary gamestate
typedef struct {
	uint8_t pieces[64];
	uint8_t color[64];
	uint8_t castle; // 1=shortW, 2=longW, 4=shortB, 8=longB
	uint8_t ply;
	bool whiteturn;
} board_t;

#endif