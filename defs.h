/*
All of the defines and structs and stuff
*/

#ifndef DEFS_H
#define DEFS_H

#include <stdint.h>
#include <stdbool.h>

//#define NDEBUG

/*
// Taken from Vice11 defs.h
#ifndef DEBUG
#define ASSERT(n)
#else
#define ASSERT(n) \
if(!(n)) { \
printf("%s - Failed",#n); \
printf("On %s ",__DATE__); \
printf("At %s ",__TIME__); \
printf("In File %s ",__FILE__); \
printf("At Line %d\n",__LINE__); \
exit(1);}
#endif
*/

// Defines

#define SQTOBB(sq) ((uint64_t)0x1<<(sq))

// Border masks
#define TOP_MASK    0xff00000000000000
#define RIGHT_MASK  0x8080808080808080
#define BOTTOM_MASK 0x00000000000000ff
#define LEFT_MASK   0x0101010101010101

// For diagonal traveling
#define TOP_RIGHT_MASK    0xFF80808080808080
#define TOP_LEFT_MASK     0xFF01010101010101
#define BOTTOM_LEFT_MASK  0x01010101010101FF
#define BOTTOM_RIGHT_MASK 0x80808080808080FF

// Knight masks
#define TOP_MASK_N    0xffff000000000000
#define RIGHT_MASK_N  0xc0c0c0c0c0c0c0c0
#define BOTTOM_MASK_N 0x000000000000ffff
#define LEFT_MASK_N   0x0303030303030303

// Double-push rank masks
#define TOP_DPUSH_MASK    0x00ff000000000000
#define BOTTOM_DPUSH_MASK 0x000000000000ff00

// Castling "piece-in-the-way" masks
#define WQ_CAST_CLEAR_MASK 0x000000000000000e
#define WK_CAST_CLEAR_MASK 0x0000000000000060
#define BQ_CAST_CLEAR_MASK 0x0e00000000000000
#define BK_CAST_CLEAR_MASK 0x6000000000000000

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
#define MV_NE(sq) MV_N(MV_E(sq)) //9
#define MV_SE(sq) MV_S(MV_E(sq)) //-7
#define MV_SW(sq) MV_S(MV_W(sq)) //-9
#define MV_NW(sq) MV_N(MV_W(sq)) //7


// Empty square char
#define NO_PIECE_CHAR ('.')

// Number of piece types
#define N_PIECES 6

// Number of non-sliding pieces (pawn is not included)
#define N_NOSLIDE_PIECES 2

// See: https://chess.stackexchange.com/a/30006
#define MAX_FEN_LEN 88 // includes trailing \0

#define DEFAULT_FEN "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"
//#define DEFAULT_FEN "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1"
//#define DEFAULT_FEN "rnbqkbnr/pp1ppppp/8/2p5/4P3/5N2/PPPP1PPP/RNBQKB1R b KQkq - 1 2"
//#define DEFAULT_FEN "rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8"

// Enums
enum side_e {
	WHITE,
	BLACK
};

// this ordering so all the non-sliding
// pieces are first (pawns not included)
enum piece_e {
	KING,
	KNIGHT,
	QUEEN,
	BISHOP,
	ROOK,
	PAWN
};

enum castling_e {
	WKCASTLE = 1,
	WQCASTLE = 2,
	BKCASTLE = 4,
	BQCASTLE = 8
};

enum moveflags_e {
	FLAGPAWNMOVE = 0x1,
	FLAGCAPTURE = 0x2,
	FLAGCHECK = 0x4,
	FLAG
};


// Structs

/*
Struct representing a board.
TODO: Implement efficiently updateable attack maps
*/
typedef struct board_s {
	uint64_t pieces[2][N_PIECES]; // [side][piece_type]
	uint64_t all_pieces[2]; // [side]
	uint8_t sidetomove; // see side_e enum
	uint64_t en_passant; // 0x0 if no en passant
	uint8_t castling; // see castling_e enum
	uint8_t fiftym_counter; // 50-move rule counter
	uint8_t fullmoves;

	unsigned int ply; // aka n of moves in movehistory
	//move_s* movehistory;
} board_s;

/*
Move
Castling will be represented by moving
king 2 sq left or right.
TODO: Implement flags.

Flags will also be in enum moveflags_e
Flags:
1000 0000:
0100 0000:
0010 0000:
0001 0000:
0000 1000:
0000 0100: 
0000 0010: Capture
0000 0001: Pawn move
*/
typedef struct move_s {
	uint64_t from;
	uint64_t to;
	uint8_t flags;
	uint8_t piecetaken; // marks what piece was eaten with this move
	uint8_t promoteto; // what to promote to
} move_s;

typedef struct movelist_s {
	move_s* moves; // remember to free this after done with this
	unsigned int n; // amount of moves
} movelist_s;



// Global variables
extern const char piece_chars[N_PIECES];
extern uint64_t movelookup[N_NOSLIDE_PIECES][64];


// Prototypes for different files

// algabreic.c
extern uint64_t algsqtobb(const char*);
extern int algsqtoint(const char*);

// attack.c
extern movelist_s pseudo_legal_squares(const board_s*, const uint64_t);
extern uint64_t pseudo_legal_squares_k(const board_s*, const unsigned int, const uint64_t);
extern uint64_t pseudo_legal_squares_n(const board_s*, const unsigned int, const uint64_t);
extern uint64_t pseudo_legal_squares_q(const board_s*, const unsigned int, const uint64_t);
extern uint64_t pseudo_legal_squares_b(const board_s*, const unsigned int, const uint64_t);
extern uint64_t pseudo_legal_squares_r(const board_s*, const unsigned int, const uint64_t);
extern uint64_t pseudo_legal_squares_p(const board_s*, const unsigned int, const uint64_t);

// bitboard.c
extern unsigned int pop_bit(uint64_t* const);
extern uint64_t pop_bitboard(uint64_t* const);
extern unsigned int lowest_bitindex(const uint64_t);
extern uint64_t lowest_bitboard(const uint64_t);
extern int popcount(uint64_t);

// init.c
extern int init_all();
extern void reset_lookup();
extern void compute_king_lookup();
extern void compute_knight_lookup();

// search.c
extern void perft(board_s*, const unsigned int);
extern unsigned int search(board_s*, const unsigned int);

// board.c
extern void printboard(const board_s*);
extern void printbitboard(const uint64_t);
extern board_s boardfromfen(const char*);
extern void resetboard(board_s*);
extern void movepiece(board_s* board, const unsigned int side, const uint64_t from, const uint64_t to);
extern void makemove(board_s* board, const move_s* move);
extern void unmakemove(board_s* board);
extern unsigned int get_piece_type(const board_s*, const unsigned int, const uint64_t);
extern unsigned int get_piece_side(const board_s*, const uint64_t);

#endif // DEFS_H
