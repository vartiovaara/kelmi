/*
All of the shit needed and stuff idk.
*/

#ifndef DEFS_H
#define DEFS_H

#include <stdint.h>
#include <stdbool.h>

// Border masks
#define TOP_MASK    0xff00000000000000
#define RIGHT_MASK  0x8080808080808080
#define BOTTOM_MASK 0x00000000000000ff
#define LEFT_MASK   0x0101010101010101

// row. starts at 0
#define ROW(sq) (floor(sq / 8))

// column. starts at 0
#define COL(sq) (sq % 8)

#define SQTOBB(sq) ((uint64_t)0x1 << sq)

enum piece_e {
	KING,
	QUEEN,
	ROOK,
	BISHOP,
	KNIGHT,
	PAWN,
	EMPTY_PIECE
};

enum colour_e {
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

const char piece_char[] = {
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
	uint8_t colour[64];
	uint8_t castle; // 1=shortW, 2=longW, 4=shortB, 8=longB
	uint8_t ply;
	bool whiteturn;
	uint64_t w_bitboard;
	uint64_t b_bitboard;
} board_t;

typedef struct {
	uint8_t from;
	uint8_t to; 
} movevec_t;

#endif