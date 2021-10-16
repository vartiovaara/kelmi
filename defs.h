/*
All of the defines and structs and stuff
*/

#ifndef DEFS_H
#define DEFS_H

#include <stdint.h>
#include <stdbool.h>


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
typedef struct {
	uint64_t pieces[2][6]; // [side][piece_type]
	uint64_t all_pieces[2]; // [side]
	bool whiteturn;
	//uint8_t ply;
} board_t;


// Prototypes for different files

// init.c
extern board_t getdefaultboard();

// board.c
extern uint64_t bbvertivalflip(uint64_t);

#endif