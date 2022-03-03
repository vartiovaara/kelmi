/*
 *All of the defines and structs and stuff
 */

#ifndef DEFS_H
#define DEFS_H

#include <stdint.h>
#include <stdbool.h>


//#define NDEBUG


/*
 * Defines
 */

// Macros
#define SQTOBB(sq) ((BitBoard)0x1<<(sq))
#define OPPOSITE_SIDE(side) ((side == WHITE) ? BLACK : WHITE)

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

// Double-push rank masks (when is it allowed)
#define TOP_DPUSH_MASK    0x00ff000000000000
#define BOTTOM_DPUSH_MASK 0x000000000000ff00

// Castling "piece-in-the-way" masks
#define WQ_CAST_CLEAR_MASK 0x000000000000000e
#define WK_CAST_CLEAR_MASK 0x0000000000000060
#define BQ_CAST_CLEAR_MASK 0x0e00000000000000
#define BK_CAST_CLEAR_MASK 0x6000000000000000

// Handy squares
#define A8 0x0100000000000000

// Normalized piece vectors
// TODO: make these something else and stuff idk

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

// See: https://chess.stackexchange.com/a/30006
#define MAX_FEN_LEN 88 // includes trailing \0

#define DEFAULT_FEN "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"
//#define DEFAULT_FEN "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1"
//#define DEFAULT_FEN "rnbqkbnr/pp1ppppp/8/2p5/4P3/5N2/PPPP1PPP/RNBQKB1R b KQkq - 1 2"
//#define DEFAULT_FEN "rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8"


/*
 * Typedefs
 */

typedef uint64_t BitBoard;


/*
 * Enums
 */

enum side_e {
	WHITE,
	BLACK
};

enum piece_e {
	KING,
	QUEEN,
	ROOK,
	BISHOP,
	KNIGHT,
	PAWN
};

enum castling_e {
	WKCASTLE = 1,
	WQCASTLE = 2,
	BKCASTLE = 4,
	BQCASTLE = 8
};

enum moveflags_e {
	FLAG_PAWNMOVE = 0x1,
	FLAG_CAPTURE = 0x2,
	FLAG_CHECK = 0x4,
};


/*
 * Structs
 */

/*
 * Move
 * Castling will be represented by moving
 * king 2 sq left or right.
 * TODO: Implement flags.
 * 
 * Flags will also be in enum moveflags_e
 * Flags:
 * 1000 0000:
 * 0100 0000:
 * 0010 0000:
 * 0001 0000:
 * 0000 1000:
 * 0000 0100: 
 * 0000 0010: Capture
 * 0000 0001: Pawn move
 */
typedef struct move_s {
	BitBoard from;
	BitBoard to;
	uint8_t flags;
	//uint8_t piece_captured; // marks what piece was eaten with this move
	uint8_t promoteto; // what to promote to
} move_s;

typedef struct movelist_s {
	move_s* moves; // remember to free this after done with this
	unsigned int n; // amount of moves
} movelist_s;

/*
 * Struct representing a board.
 * TODO: Implement efficiently updateable attack maps
 * TODO: Make and unmake moves
 */
typedef struct board_s {
	BitBoard pieces[2][N_PIECES]; // [side][piece_type]
	BitBoard every_piece; // holds all the pieces
	BitBoard all_pieces[2]; // [side]
	uint8_t sidetomove; // see side_e enum
	BitBoard en_passant; // 0x0 if no en passant
	uint8_t castling; // see castling_e enum
	uint8_t fiftym_counter; // 50-move rule counter
	uint8_t fullmoves;

	unsigned int ply; // aka n of moves in movehistory

	move_s* movehistory;
	unsigned int mvhistory_size;
} board_s;

// Defines perft results
typedef struct {
	unsigned long long end_positions;
	unsigned long long nodes;
} pertf_result_s;


/*
 * Global variables
 */

//extern const char piece_chars[N_PIECES];


/*
 * Prototypes for different files
 * Those which are commented out are supposed to be "private"
 */

// algabreic.c
////extern BitBoard algsqtobb(const char*);
////extern int algsqtoint(const char*);

// attack.c
////extern bool is_in_check (const board_s*, const unsigned int);
////extern movelist_s pseudo_legal_squares(const board_s*, const BitBoard);

// bitboard.c
////extern unsigned int pop_bit(BitBoard* const);
////extern BitBoard pop_bitboard(BitBoard* const);
////extern unsigned int lowest_bitindex(const BitBoard);
////extern BitBoard lowest_bitboard(const BitBoard);
////extern int popcount(BitBoard);

// init.c
////extern int init_all();

// lookup.c
////extern BitBoard piecelookup(unsigned int, unsigned int, unsigned int);
////extern void reset_lookups();
////extern void compute_lookups();
////extern void set_lookup_pointers();
//extern void compute_king_lookup();
//extern void compute_rook_lookup();
//extern void compute_bishop_lookup();
//extern void compute_knight_lookup();
//extern void compute_white_pawn_lookup();

// search.c
////extern void perft(board_s*, const unsigned int);
////extern void search(board_s* restrict, const unsigned int, pertf_result_s* restrict);

// board.c
////extern void printboard(const board_s*);
////extern void printbitboard(const BitBoard);
////extern board_s boardfromfen(const char*);
////extern void resetboard(board_s*);
////extern void movepiece(board_s* board, const unsigned int side, const BitBoard from, const BitBoard to);
////extern void removepiece(board_s*, const BitBoard, const unsigned int, const unsigned int);
////extern void makemove(board_s* board, const move_s* move);
////extern void unmakemove(board_s* board);
////extern unsigned int get_piece_type(const board_s*, const unsigned int, const BitBoard);
////extern unsigned int get_piece_side(const board_s*, const BitBoard);

#endif // DEFS_H
