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

// TODO: make these something else and stuff idk
// Normalized piece vectors

// horizontal and vertical
#define MV_N(sq) (sq << 8)
#define MV_E(sq) (sq << 1)
#define MV_S(sq) (sq >> 8)
#define MV_W(sq) (sq >> 1)

// diagonals
#define MV_DIAG_NE(sq) MV_N(MV_E(sq)) //9
#define MV_DIAG_SE(sq) MV_S(MV_E(sq)) //-7
#define MV_DIAG_SW(sq) MV_S(MV_W(sq)) //-9
#define MV_DIAG_NW(sq) MV_N(MV_W(sq)) //7

// Knight moves
#define MV_NNE (2*MV_N + MV_E)
#define MV_NNW (2*MV_N + MV_W)
#define MV_EEN (2*MV_E + MV_N)
#define MV_EES (2*MV_E + MV_S)
#define MV_SSE (2*MV_S + MV_E)
#define MV_SSW (2*MV_S + MV_W)
#define MV_WWS (2*MV_W + MV_S)
#define MV_WWN (2*MV_W + MV_N)

// Empty square char
#define NO_PIECE_CHAR ('.')

// Number of piece types
#define N_PIECES 6

// See: https://chess.stackexchange.com/a/30006
#define MAX_FEN_LEN 88 // includes trailing \0

//#define DEFAULT_FEN "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"
#define DEFAULT_FEN "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1"
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
	uint64_t pieces[2][N_PIECES]; // [side][piece_type]
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

// attack.c
extern uint64_t pseudo_legal_squares(const board_s*, const uint8_t, const uint64_t);
extern uint64_t pseudo_legal_squares_k(const board_s*, const uint8_t, const uint64_t);

// init.c

// board.c
extern void printboard(const board_s*);
extern void printbitboard(const uint64_t);
extern board_s boardfromfen(const char*);
extern void resetboard(board_s*);
extern uint8_t get_piece_type(const board_s*, const uint8_t, const uint64_t);

#endif // DEFS_H