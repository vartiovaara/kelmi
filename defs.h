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

// Handy squares
#define A8 0x0100000000000000


// Empty square char
#define NO_PIECE_CHAR ('.')

// Number of piece types
#define N_PIECES 6

// See: https://chess.stackexchange.com/a/30006
#define MAX_FEN_LEN 88 // includes trailing \0

#define DEFAULT_FEN "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"
//#define DEFAULT_FEN "rnbqkbnr/pp1ppppp/8/2p5/4P3/5N2/PPPP1PPP/RNBQKB1R b KQkq - 1 2"

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

enum castling_e {
	WKCASTLE = 1,
	WQCASTLE = 2,
	BKCASTLE = 4,
	BQCASTLE = 8
};


// Structs
typedef struct board_s {
	uint64_t pieces[2][6]; // [side][piece_type]
	uint64_t all_pieces[2]; // [side]
	bool whiteturn;
	uint64_t en_passant; // 0x0 if no en passant
	uint8_t castling; // see castling_e enum
	uint8_t fiftym_counter; // 50-move rule counter
	uint8_t fullmoves;
	//uint8_t ply;
} board_s;


// Global variables

extern const char piece_chars[N_PIECES];

// Prototypes for different files

// algabreic.c
extern uint64_t algsqtobb(const char*);
extern int algsqtoint(const char*);

// init.c

// board.c
extern void printboard(const board_s*);
extern void printbitboard(const uint64_t);
extern board_s boardfromfen(const char*);
extern void resetboard(board_s*);

#endif // DEFS_H