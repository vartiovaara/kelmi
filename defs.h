/*
All of the defines and structs and stuff
*/

#ifndef DEFS_H
#define DEFS_H

#include <stdint.h>
#include <stdbool.h>

// Defines

#define SQTOBB(sq) ((uint64_t)0x1<<(sq))

// Border masks
#define TOP_MASK    0xff00000000000000
#define RIGHT_MASK  0x8080808080808080
#define BOTTOM_MASK 0x00000000000000ff
#define LEFT_MASK   0x0101010101010101

// Empty square char
#define NO_PIECE_CHAR ('.')

// Number of piece types
#define N_PIECES 6

// See: https://chess.stackexchange.com/a/30006
#define MAX_FEN_LEN 88 // includes trailing \0

// Enums
enum side_e {
	WHITE,
	BLACK
};

enum piece_e {
	KING,
	QUEEN,
	BISHOP,
	KNIGHT,
	ROOK,
	PAWN
};


// Structs
typedef struct board_s {
	uint64_t pieces[2][6]; // [side][piece_type]
	uint64_t all_pieces[2]; // [side]
	bool whiteturn;
	//uint8_t ply;
} board_s;


// Global variables

extern const char piece_chars[N_PIECES];

// Prototypes for different files

// init.c
extern board_s getdefaultboard();

// board.c
extern void printboard(const board_s*);
extern board_s boardfromfen(const char*);
extern void resetboard(board_s*);
extern uint64_t bbvertivalflip(uint64_t);

#endif