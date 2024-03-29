/*
 *All of the defines and structs and stuff
 */

#ifndef DEFS_H
#define DEFS_H

#include <stdint.h>
#include <stdbool.h>
#include <limits.h>
#include <math.h>
#include <stdlib.h>


/*
 * Defines
 */

#define ENGINE_NAME "Kelmi"
#define ENGINE_AUTHOR "Pyry Vartiovaara"

#define HELP_MESSAGE \
"Commands: \n\
   playw - Start playing a game against computer as white. \n\
   playb - Start playing a game against computer as black \n\
   perft n - Start perft with n plies \n\
   pruned_perft n - Start perft to n plies but with prunings enabled\n\
   uci - Start uci. \n\
Commands while playing: \n\
   p - Print the board. \n\
General commands: \n\
   help - Print this help \n\
   quit - Exit this prompt. \n\
To make a move, give it in uci format."



// Macros
#define SQTOBB(sq) ((BitBoard)0x1<<(sq))
#define OPPOSITE_SIDE(side) ((side == WHITE) ? BLACK : WHITE)
#define MIN(x, y) (x < y ? x : y)
#define MAX(x, y) (x > y ? x : y)
#define LENGTH(x) (sizeof (x) / sizeof (x[0]))
#define COMPACT_MOVE_FROM(x) ((x & 0xfc00) >> 10) // 0b1111_1100_0000_0000 = 0xFC00
#define COMPACT_MOVE_TO(x) ((x & 0x03f0) >> 4) // 0b0000_0011_1111_0000 = 0x3F0
#define COMPACT_MOVE_PROMOTETO(x) ((x & 0x0007)) // 0b0000_0000_0000_0111 = 0x7
#define EVAL_IS_MATE(eval) (EVAL_IS_WIN(eval) || EVAL_IS_LOSE(eval))
#define EVAL_IS_WIN(eval) ((eval) > EVAL_MAX)
#define EVAL_IS_LOSE(eval) ((eval) < EVAL_MIN)


// Compact move promote flag
#define COMPACT_MOVE_PROMOTE_FLAG 0x8 // 0b0000_0000_0000_1000 = 0x8


#define TOP_HALF_MASK    0xffffffff00000000
#define BOTTOM_HALF_MASK 0x00000000ffffffff
#define RIGHT_HALF_MASK  0xf0f0f0f0f0f0f0f0
#define LEFT_HALF_MASK   0x0f0f0f0f0f0f0f0f


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

// About-to-promote masks
#define W_PROMOTE_FROM_MASK 0x00ff000000000000
#define B_PROMOTE_FROM_MASK 0x000000000000ff00

// Defines the areas in which castling happens (king and rook and everything between)
#define WQ_CASTLE_AREA 0x000000000000001f
#define WK_CASTLE_AREA 0x00000000000000f0
#define BK_CASTLE_AREA 0x1f00000000000000
#define BQ_CASTLE_AREA 0xf000000000000000

// Standard king starting position
#define W_KING_DEFAULT_POS 0x0000000000000010
#define B_KING_DEFAULT_POS 0x1000000000000000

// Castling king target squares (where the king ends up at)
#define WK_CASTLE_KING_TARGET 0x0000000000000040
#define WQ_CASTLE_KING_TARGET 0x0000000000000004
#define BK_CASTLE_KING_TARGET 0x4000000000000000
#define BQ_CASTLE_KING_TARGET 0x0400000000000000

// Castling rook target squares (where the rook ends up at)
#define WK_CASTLE_ROOK_TARGET 0x0000000000000020
#define WQ_CASTLE_ROOK_TARGET 0x0000000000000008
#define BK_CASTLE_ROOK_TARGET 0x2000000000000000
#define BQ_CASTLE_ROOK_TARGET 0x0800000000000000

// Defines squares that can not be attacked for castling (combined targets)
#define WK_CASTLE_ATTACK_MASK (WK_CASTLE_KING_TARGET | WK_CASTLE_ROOK_TARGET)
#define WQ_CASTLE_ATTACK_MASK (WQ_CASTLE_KING_TARGET | WQ_CASTLE_ROOK_TARGET)
#define BK_CASTLE_ATTACK_MASK (BK_CASTLE_KING_TARGET | BK_CASTLE_ROOK_TARGET)
#define BQ_CASTLE_ATTACK_MASK (BQ_CASTLE_KING_TARGET | BQ_CASTLE_ROOK_TARGET)

// Castling "piece-in-the-way" masks
#define WQ_CAST_CLEAR_MASK 0x000000000000000e
#define WK_CAST_CLEAR_MASK 0x0000000000000060
#define BQ_CAST_CLEAR_MASK 0x0e00000000000000
#define BK_CAST_CLEAR_MASK 0x6000000000000000

// Quiescence search pawn selection mask
#define Q_SEARCH_PAWN_SELECT_MASK_W 0xffffff0000000000 // top 3 rows
#define Q_SEARCH_PAWN_SELECT_MASK_B 0x0000000000ffffff // bottom 3 rows

#define TOP_THREE_ROWS 0xffffff0000000000 // top 3 rows
#define BOTTOM_THREE_ROWS 0x0000000000ffffff // bottom 3 rows


// Handy squares
#define A1 0x0000000000000001
#define A8 0x0100000000000000
#define H1 0x0000000000000080
#define H8 0x8000000000000000

// Normalized piece vectors
// TODO: make these something else and stuff idk

// horizontal and vertical
#define MV_N(sq, n) (sq << 8*n)
#define MV_E(sq, n) (sq << 1*n)
#define MV_S(sq, n) (sq >> 8*n)
#define MV_W(sq, n) (sq >> 1*n)

// diagonals
#define MV_NE(sq, n) MV_N(MV_E(sq, n), n)
#define MV_SE(sq, n) MV_S(MV_E(sq, n), n)
#define MV_SW(sq, n) MV_S(MV_W(sq, n), n)
#define MV_NW(sq, n) MV_N(MV_W(sq, n), n)

// Direction
// https://www.chessprogramming.org/Direction
#define DIRECTION_N 8
#define DIRECTION_NE 9
#define DIRECTION_E 1
#define DIRECTION_SE (-7)
#define DIRECTION_S (-8)
#define DIRECTION_SW (-9)
#define DIRECTION_W (-1)
#define DIRECTION_NW 7


// Eval max and min
#define EVAL_MAX (50000)
#define EVAL_MIN (-EVAL_MAX)
#define EVAL_INF (70000)
#define MATE     (60000)

// Eval values
#define EVAL_PAWN_MATERIAL_VALUE 100
#define EVAL_KNIGHT_MATERIAL_VALUE 300//305
#define EVAL_BISHOP_MATERIAL_VALUE 300//333
#define EVAL_ROOK_MATERIAL_VALUE 620//563
#define EVAL_QUEEN_MATERIAL_VALUE 950
#define EVAL_MATERIAL_IMBALANCE_ACCENTUATE_MULT 1
#define EVAL_BPAIR_VALUE 10
#define EVAL_STACKED_PAWNS_PUNISHMENT 7 // applied for every stacked pawn
#define EVAL_ROOK_OPEN_FILE 5
#define EVAL_MOVABLE_SQUARES_MULT 1
#define EVAL_CASTLING_RIGHTS_Q 4
#define EVAL_CASTLING_RIGHTS_K 15
#define KING_GUARD_OUTSIDE_BOARD 49 // TODO: Not done yet
#define EVAL_TEMPO_BONUS 8 // https://www.chessprogramming.org/Tempo

// Move predict(ordering) weights
#define MV_SCORE_MOVE_WEIGHT_PAWN 20
#define MV_SCORE_MOVE_WEIGHT_KNIGHT 30
#define MV_SCORE_MOVE_WEIGHT_BISHOP 22
#define MV_SCORE_MOVE_WEIGHT_ROOK 31
#define MV_SCORE_MOVE_WEIGHT_QUEEN 60
#define MV_SCORE_MOVE_WEIGHT_KING 5
#define MV_SCORE_KCASTLE 10
#define MV_SCORE_QCASTLE 5
#define MV_SCORE_PROMOTE 200 // added on top of material addition(piece to promote to)
#define MV_SCORE_CHECK 500
#define MV_SCORE_CAPTURER_VALUE_DIVIDE 5

#define NULL_MOVE_PRUNING_R(depth) ((depth > 6) ? 3 : 2)

//#define Q_SEARCH_INITIAL_PRUNE_TRESHOLD -100 // (MV_SCORE_MOVE_WEIGHT_PAWN)
// enter in graphical calculator
// depth in q-search is always -1 or less
//#define Q_SEARCH_PRUNE_TRESHOLD(depth) ((eval_t)(Q_SEARCH_INITIAL_PRUNE_TRESHOLD + (-(eval_t)depth - 1)*0)) // (int)powf(-1 - depth, 0.7))
#define Q_SEARCH_PRUNE_TRESHOLD (70) //100 // 78

#define Q_SEARCH_STANDPAT_PRUNING_DEPTH_TRESHOLD (0)

// Empty square char
#define NO_PIECE_CHAR ('.')

// Number of piece types
#define N_PIECES 6

// Number of pieces to promote to (no king or pawn)
#define N_PROM_PIECES (N_PIECES - 2)

// See: https://chess.stackexchange.com/a/30006
#define MAX_FEN_LEN 88 // includes trailing \0

#define INPUT_BUFFER_SIZE 256

#define UCI_INPUT_BUFFER_SIZE 4096

#define DEFAULT_FEN "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"
//#define DEFAULT_FEN "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1"
//#define DEFAULT_FEN "rnbqkbnr/pp1ppppp/8/2p5/4P3/5N2/PPPP1PPP/RNBQKB1R b KQkq - 1 2"
//#define DEFAULT_FEN "rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8"
//#define DEFAULT_FEN "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1"
//#define DEFAULT_FEN "8/k7/3p4/p2P1p2/P2P1P2/8/8/K7 w - - 0 1"
//#define DEFAULT_FEN "8/8/7p/3KNN1k/2p4p/8/3P2p1/8 w - - 0 1"
//#define DEFAULT_FEN "4k3/8/8/8/8/8/4P3/4K3 w - - 0 1"
//#define DEFAULT_FEN "q2k2q1/2nqn2b/1n1P1n1b/2rnr2Q/1NQ1QN1Q/3Q3B/2RQR2B/Q2K2Q1 w - - 0 1"
//#define DEFAULT_FEN "1r6/p2r1p1p/6p1/2b1kp2/2Bn3P/2N5/PP1R1PP1/4K2R w K - 4 27"
//#define DEFAULT_FEN "r5k1/pp2pr1p/2n3p1/1qP3Q1/4P3/4KN2/P4PPP/b1B4R w - - 6 16"
//#define DEFAULT_FEN "8/8/3k4/1Rp5/P1Pp2K1/8/8/8 w - - 1 64"
//#define DEFAULT_FEN "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1"
//#define DEFAULT_FEN "8/2p5/3p4/KP5r/1R2Pp1k/8/6P1/8 b - - 0 1"
//#define DEFAULT_FEN "rnb1kbnr/pp1pp1pp/1qp2p2/8/Q1P5/N7/PP1PPPPP/1RB1KBNR b Kkq - 2 4"
//#define DEFAULT_FEN "rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8"
//#define DEFAULT_FEN "r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 10"
//#define DEFAULT_FEN "1r3rk1/3q1ppp/3p4/p1pNpP2/PpP1P1P1/1P3Q2/6KP/5R2 w - - 0 1"

#define MAX_DEPTH 99


/*
 * Typedefs
 */

typedef uint64_t BitBoard;
typedef signed int eval_t; // IF THIS IS CHANGED; CHANGE THE MAX AND MIN DEFS TOO
// An eval type quaranteed to be bigger than eval_t. Use for delta calc when values might underflow
typedef signed long big_eval_t;  // IF THIS IS CHANGED; CHANGE THE MAX AND MIN DEFS TOO

/*
 * Enums
 */

enum side_e {
	WHITE,
	BLACK,
	SIDE_NONE
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
	WKCASTLE = 0x1,
	WQCASTLE = 0x2,
	BKCASTLE = 0x4,
	BQCASTLE = 0x8
};

enum moveflags_e {
	FLAG_PAWNMOVE     = 0x1 << 0,
	FLAG_CAPTURE      = 0x1 << 1,
	FLAG_KCASTLE      = 0x1 << 2,
	FLAG_QCASTLE      = 0x1 << 3,
	FLAG_DOUBLEPUSH   = 0x1 << 4,
	FLAG_ENPASSANT    = 0x1 << 5,
	FLAG_PROMOTE      = 0x1 << 6,
	FLAG_CHECK        = 0x1 << 7
};

/*
 * UCI_IDLE: Doing nothing, waiting for input or opponent to make move.
 * UCI_SEARCH: Search with more detail in enum uci_searchtype_e
 * UCI_PONDER: Ponder (maybe sometime will be used)
 */
enum uci_action_e {
	UCI_IDLE,
	UCI_SEARCH,
	UCI_PONDER
};

/*
 * UCI_SEARCH_REGULAR: Normal search with considerations for move time.
 * UCI_SEARCH_INFINITE: Normal search, but only stopped by "stop" command.
 * UCI_SEARCH
 */
enum uci_searchtype_e {
	UCI_SEARCH_REGULAR,
	UCI_SEARCH_INFINITE,
};

enum tt_entry_flags_e {
	TT_ENTRY_FLAG_FAIL_HIGH = 0x1 << 0,
	TT_ENTRY_FLAG_FAIL_LOW  = 0x1 << 1,
	TT_ENTRY_FLAG_EXACT     = 0x1 << 2,
	TT_ENTRY_FLAG_PV_NODE   = 0x1 << 3
	//TT_ENTRY_FLAG_FULL_NODE = 0x1 << 0,
	//TT_ENTRY_FLAG_PROMOTION = 0x1 << 0,
	//TT_ENTRY_FLAG_CUT_NODE  = 0x1 << 1
	//TT_ENTRY_FLAG_EXIST     = 0x1 << 0,
};


/*
 * Structs
 */

/*
 * Move
 * Castling will be represented by moving
 * king 2 sq left or right.
 * TODO: Implement flags.
 * TODO: Implement piece captured for undoing moves
 * 
 * If from and to is 0c0, move is null move.
 * 
 * Flags will also be in enum moveflags_e
 * Flags:
 * 1000 0000: Probable check. Could very well be a check. Only not a check if move happened to be illegal. Only to be used in move-ordering.
 * 0100 0000: Promote
 * 0010 0000: En passant
 * 0001 0000: Pawn Double push
 * 0000 1000: Queen Castle
 * 0000 0100: King Castle
 * 0000 0010: Regular capture
 * 0000 0001: Pawn move
 */
typedef struct move_s {
	BitBoard from;
	BitBoard to;
	uint8_t flags;
	uint8_t fromtype; // what type was the from piece
	uint8_t side;
	uint8_t piece_captured; // marks what type of piece was eaten with this move
	uint8_t promoteto; // what to promote to
	
	uint8_t old_castling_flags; // castling flags of the board previous to the move (for move undoing)
	BitBoard old_en_passant;

	eval_t move_score; // assigned move score for move ordering. Only set in the search algorithm
	eval_t move_see;
} move_s;

typedef struct movelist_s {
	move_s* moves; // remember to free this after done with this
	unsigned int n; // amount of moves
} movelist_s;

typedef struct {
	// triangular array with 1 -> depth entries in each layer
	// moves are encoded as compact moves
	// 6 bits from + 6 bits to + 1 bit promote flag + 3 bits promoteto = 16 bits = 2 bytes
	// Normal move_s would at the time of writing take 40 bytes
	uint16_t** pv;
	// number of moves in each layer
	size_t* n_moves;

	size_t depth_allocated;

	eval_t alpha;
	eval_t beta;
} pv_s;

/*
 * Struct representing a board.
 * TODO: Implement efficiently updateable attack maps
 * TODO: Make and unmake moves
 */
typedef struct board_s {
	BitBoard pieces[2][N_PIECES]; // [side][piece_type]
	BitBoard every_piece; // holds all the pieces
	BitBoard all_pieces[2]; // [side]

	// [side] holds the pinned pieces of that side
	//BitBoard pinned_pieces[2]; // [side]

	// WHITE, BLACK or SIDE_NONE
	uint8_t side_in_check;
	//__attribute__ ((aligned))
	// Only one check mask needed as only 1 side can be in check
	//BitBoard check_mask;

	// holds 3 last board hashes
	// new hash will be added when fiftym_counter % 2 == 0
	//uint64_t repeated_positions[6];

	// Holds last 6 moves made
	// Is used for checking draw by repeat
	//uint16_t repeated_moves[6];

	// Holds last 3 board hashes
	// hashes added when (fiftym_counter % 2 == 0)
	//uint64_t last_positions[3];



	uint8_t sidetomove; // see side_e enum
	BitBoard en_passant; // 0x0 if no en passant
	uint8_t castling; // see castling_e enum
	uint8_t fiftym_counter; // 50-move rule counter
	uint8_t fullmoves;
	uint64_t hash; // hash of current position
	
	unsigned int rep_stack_n;
	uint64_t rep_stack[1024];

#ifndef NDEBUG
	// Moves will be copied to movehistory only in debug builds
	unsigned int history_n; // aka n of moves in movehistory
	movelist_s movehistory; // movelist_s.n = n of moves allocated
#endif // NDEBUG
} board_s;

// Defines perft results
// Remember to free n_positions after use
typedef struct {
	unsigned int n_plies; // Number of plies computed
	unsigned long long* n_positions; // malloc -> Number of positions in nth ply
	unsigned long long nodes;
	unsigned long long n_leaves;

	// "advanced" information
	unsigned long long* captures;
	unsigned long long* checks;
	unsigned long long* en_passant;
	unsigned long long* checkmates;
	unsigned long long* stalemates;
	unsigned long long* castles;
	unsigned long long* promotions;
} pertf_result_s;


/*
 * remember to free n_positions and fail_hard_cutoffs
 */
typedef struct {
	unsigned int n_plies; // Number of plies computed (rename to depth??)
	unsigned long long* n_positions; // malloc -> Number of positions in nth ply
	unsigned long long* fail_hard_cutoffs; // malloc -> Number of hard-cutoffs in nth ply
	unsigned long long nodes;
	unsigned long long n_moves_generated;
	unsigned long long hashtable_hits;
} search_stats_s;


// Holds data from a uci instance
// See: http://wbec-ridderkerk.nl/html/UCIProtocol.html
// TODO: Will hold future settings that gui can set and all that stuff
typedef struct {
	enum uci_action_e action;

	// defines the type of search ocurring
	enum uci_searchtype_e searchtype;

	// Go command variables
	// See "go" command from: http://wbec-ridderkerk.nl/html/UCIProtocol.html
	long wtime; // msec white has left on clock
	long btime; // msec black has left on clock
	long winc; // white increment per move in msec
	long binc; // black increment per move in msec
	int movestogo; // there are x moves to the next time control

	bool sudden_death;
} uci_s;


typedef struct {
	uint64_t hash;
	eval_t eval;
	uint8_t search_depth; // depth of the search
	uint8_t flags;
	uint16_t bestmove;
} tt_entry_s;

typedef struct {
	char* table_alloc; // Base of malloc. tt_s->entries could be offset to be alogned
	tt_entry_s* entries; // [(bucket_n*BUCKET_SIZE) + entry]
	size_t n_buckets; // n of buckets
	//size_t n_entries; // n entries per bucket
	unsigned int counter; // FIXME: counter to be used for just random replacement
} tt_s;




#endif // DEFS_H
