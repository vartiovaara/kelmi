/*
Stuff about boards and bitboards.
*/

#ifndef BOARD_C
#define BOARD_C

#include <stdint.h>
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include <assert.h>

#include "board.h"

#include "bitboard.h"
#include "algebraic.h"

#include "defs.h"




void printboard(const board_s* board) {
	BitBoard pos = A8; // top-left
	do {
		char ch = NO_PIECE_CHAR;
		for (int piece = 0; piece < N_PIECES; piece++) {
			if (pos & board->pieces[WHITE][piece]) {
				ch = toupper(piecetochar(piece));
				break;
			}
			else if (pos & board->pieces[BLACK][piece]) {
				ch = tolower(piecetochar(piece));
				break;
			}
		}
		printf("%c ", ch);
		// check if pos is on h-file and nl
		if (pos & RIGHT_MASK) {
			printf("\n");
			pos >>= 15;
		}
		else
			pos <<= 1;
	} while (pos > 0);
}


void printbitboard(const BitBoard bb) {
	BitBoard pos = A8; // top-left
	do {
		char ch = ((pos & bb) ? '1' : '0');
		printf("%c ", ch);
		// check if pos is on h-file and nl
		if (pos & RIGHT_MASK) {
			printf("\n");
			pos >>= 15;
		}
		else
			pos <<= 1;
	} while (pos > 0);
	printf("%p \n", (void*)bb);
}


unsigned int piece_from_char(const unsigned char c) {
	unsigned int piece;

	switch (c) {
		case 'k':
			piece = KING;
			break;
		case 'q':
			piece = QUEEN;
			break;
		case 'r':
			piece = ROOK;
			break;
		case 'b':
			piece = BISHOP;
			break;
		case 'n':
			piece = KNIGHT;
			break;
		case 'p':
			piece = PAWN;
			break;
		default:
			fprintf(stderr, "Errer in piece_form_char(): piece char wrong. Got: %c\n", c);
			abort();
	}

	return piece;
}


board_s boardfromfen(const char* fen_str) {
	// TODO: have a FEN validation function as the
	// behaviour of this function is undefined with invalid FENs

	assert(strlen(fen_str) <= MAX_FEN_LEN);

	char fen[MAX_FEN_LEN];
	strcpy(fen, fen_str);
	
	// split the fen into its fields
	char* pos_str = strtok(fen, " ");
	const char* movingside = strtok(NULL, " ");
	const char* castling = strtok(NULL, " ");
	const char* en_passant = strtok(NULL, " ");
	const char* halfmove = strtok(NULL, " ");
	const char* fullmove = strtok(NULL, " ");

	assert(strlen(pos_str) > 0);
	assert(strlen(movingside) > 0);
	assert(strlen(castling) > 0);
	assert(strlen(en_passant) > 0);
	assert(strlen(halfmove) > 0);
	assert(strlen(fullmove) > 0);

	// split the position string in its fields
	const char* pos_row[8];
	pos_row[0] = strtok(pos_str, "/");
	for (int i = 1; i < 8; i++)
		pos_row[i] = strtok(NULL, "/");

	board_s board;
	resetboard(&board);

	// Parse piece positions
	for (int y = 0; y < 8; y++) {
		BitBoard pos = SQTOBB((7-y)*8);
		const int row_len = strlen(pos_row[y]);
		for (int i = 0; i < row_len; i++) {
			if (isalpha(pos_row[y][i])) {
				int piececode = 0;
				switch (tolower(pos_row[y][i])) {
					case 'k':
						piececode = KING;
						break;
					case 'q':
						piececode = QUEEN;
						break;
					case 'b':
						piececode = BISHOP;
						break;
					case 'n':
						piececode = KNIGHT;
						break;
					case 'r':
						piececode = ROOK;
						break;
					case 'p':
						piececode = PAWN;
						break;
					default:
						assert(0);
				}
				const int side = (isupper(pos_row[y][i]) ? WHITE : BLACK);
				board.pieces[side][piececode] |= pos;
				board.all_pieces[side] |= pos;
				board.every_piece |= pos;
				pos <<= 1;
			}
			else if (isdigit(pos_row[y][i])) {
				pos <<= (pos_row[y][i] - '0');
			}
		}
	}

	// Parse side to move
	if (tolower(movingside[0]) == 'w')
		board.sidetomove = WHITE;
	else
		board.sidetomove = BLACK;

	// Parse castling ability
	if (strchr(castling, 'K'))
		board.castling |= WKCASTLE;
	if (strchr(castling, 'Q'))
		board.castling |= WQCASTLE;
	if (strchr(castling, 'k'))
		board.castling |= BKCASTLE;
	if (strchr(castling, 'q'))
		board.castling |= BQCASTLE;
	
	// Parse en passant
	if (en_passant[0] != '-')
		board.en_passant = algsqtobb(en_passant);
	else
		board.en_passant = 0; // no en passant
	
	// Move counters
	board.fiftym_counter = strtoul(halfmove, NULL, 10);
	board.fullmoves = strtoul(fullmove, NULL, 10);

	/*
#ifndef NDEBUG
	printf("Start FEN parsing debug info\n");
	for (int i = 0; i < 8; i++)
		printf("%s\n", pos_row[i]);
	printf("%s: %s\n", movingside, (board.whiteturn ? "white" : "black"));
	printf("%s: %p\n", castling, (void*)(size_t)board.castling);
	printf("%s: %p\n", en_passant, (void*)board.en_passant);
	printf("%s: %u\n", halfmove, board.fiftym_counter);
	printf("%s: %u\n", fullmove, board.fullmoves);
	printf("End FEN parsing debug info\n");
#endif // NDEBUG
	*/
	return board;
}


void resetboard(board_s* board) {
	memset(board, 0, sizeof (board_s));
}


/*
TODO: Test the eligibility of this function
*/
void movepiece(board_s* board, const unsigned int type, const BitBoard from, const BitBoard to) {
	assert(popcount(to) == 1);
	assert(popcount(from) == 1);
	assert(to ^ board->every_piece); //??

	assert(board->every_piece == (board->all_pieces[WHITE] | board->all_pieces[BLACK]));

	// Otherwise we'd need to have a 4th argument
	unsigned int side = get_piece_side(board, from);
	
	// Change piece bitboards
	board->pieces[side][type] &= ~from; // remove from
	board->pieces[side][type] |= to; // add to
	board->all_pieces[side] &= ~from;
	board->all_pieces[side] |= to;
	board->every_piece &= ~from;
	board->every_piece |= to;

	assert(board->every_piece == (board->all_pieces[WHITE] | board->all_pieces[BLACK]));
}


void removepiece(board_s* board, const BitBoard pos, const unsigned int side, const unsigned int type) {
	assert(popcount(pos) == 1);
	assert(side == WHITE || side == BLACK);
	assert(type < N_PIECES);

	board->pieces[side][type] &= ~pos;
	board->all_pieces[side] &= ~pos;
	board->every_piece &= ~pos;
}


void addpiece(board_s* board, const BitBoard pos, const unsigned int side, const unsigned int type) {
	assert(popcount(pos) == 1);
	assert(side == WHITE || side == BLACK);
	assert(type < N_PIECES);

	board->pieces[side][type] |= pos;
	board->all_pieces[side] |= pos;
	board->every_piece |= pos;
}


// TODO: Finish this function
void makemove(board_s* restrict board, const move_s* restrict move) {
	assert(popcount(move->from) == 1);
	assert(popcount(move->to) == 1);
	//assert(move->from)
	assert(move->from & board->all_pieces[board->sidetomove]); // can trigger if wrong side tries to perform a move

	bool pieces_moved = false;

	// Clear en passant
	board->en_passant = 0x0;

	// If the piece was captured, remove it.
	if (move->flags & FLAG_CAPTURE) {
		assert(move->to & board->every_piece);
		const unsigned int captured_side = get_piece_side(board, move->to);
		const unsigned int captured_type = get_piece_type(board, captured_side, move->to);
		removepiece(board, move->to, captured_side, captured_type);
	}

	// If move was a promotion, remove it and add the relevant piece to its place
	if (move->flags & FLAG_PROMOTE) {
		assert(move->fromtype == PAWN);

		removepiece(board, move->from, move->side, PAWN);
		addpiece(board, move->to, move->side, move->promoteto);
		pieces_moved = true;
	}

	// Set en passant target square
	if (move->flags & FLAG_DOUBLEPUSH) {
		board->en_passant = (move->side==WHITE ? MV_S(move->to, 1) : MV_N(move->to, 1));
	}

	// Both flags set shouldn't be set
	assert(popcount(move->flags & (FLAG_DOUBLEPUSH | FLAG_ENPASSANT)) < 2);

	// Perform En passant
	if (move->flags & FLAG_ENPASSANT) {
		const BitBoard piece_to_remove = (move->side == WHITE ? MV_S(move->to, 1) : MV_N(move->to, 1));
		
		assert(piece_to_remove & board->pieces[OPPOSITE_SIDE(move->side)][PAWN]);

		removepiece(board, piece_to_remove, OPPOSITE_SIDE(move->side), PAWN);
	}

	// Do castling
	if (move->flags & FLAG_KCASTLE) {
		unsigned int castle_flags = WKCASTLE;
		if (board->sidetomove == BLACK)
			castle_flags = BKCASTLE;
		
		performcastle(board, castle_flags);
		pieces_moved = true;
	}
	else if (move->flags & FLAG_QCASTLE) {
		unsigned int castle_flags = (move->side == WHITE ? WQCASTLE : BQCASTLE);
		//if (board->sidetomove == BLACK)
		//	castle_flags = BQCASTLE;
		
		performcastle(board, castle_flags);
		pieces_moved = true;
	}

	// Revoking of castling rights by rook move
	if (move->fromtype == ROOK && board->castling) {
		// check which rook was moved and change correct flags
		if (board->sidetomove == WHITE) {
			if (move->from & RIGHT_MASK)
				board->castling &= ~WKCASTLE;
			if (move->from & LEFT_MASK)
				board->castling &= ~WQCASTLE;
		}
		else {
			if (move->from & RIGHT_MASK)
				board->castling &= ~BKCASTLE;
			if (move->from & LEFT_MASK)
				board->castling &= ~BQCASTLE;
		}
	}

	// Revoking of castling rights by king move
	if (move->fromtype == KING && board->castling) {
		if (move->side == WHITE)
			board->castling &= ~(WKCASTLE | WQCASTLE);
		else
			board->castling &= ~(BKCASTLE | BQCASTLE);
	}

	// some moves are already handled (like castling)
	if (!pieces_moved)
		movepiece(board, move->fromtype, move->from, move->to);

	// change side to move
	board->sidetomove = OPPOSITE_SIDE(board->sidetomove);
}


// TODO: Finish this function
void unmakemove(board_s* board) {
	board->sidetomove = OPPOSITE_SIDE(board->sidetomove);
}


unsigned int get_piece_type(const board_s* board, const unsigned int side, const BitBoard piecebb) {
	assert(side == WHITE || side == BLACK);
	assert(popcount(piecebb) == 1);
	assert(piecebb & board->all_pieces[side]);

	for (int i = 0; i < N_PIECES; i++) {
		if (board->pieces[side][i] & piecebb)
			return i;
	}
	// should never get here
	fprintf(stderr, "get_piece_type(board, %u, %p)\n", side, (void*)piecebb);
	exit(1);
}


unsigned int get_piece_side(const board_s* board, const BitBoard piecebb) {
	assert(popcount(piecebb) == 1);
	assert(board->every_piece & piecebb);

	if (board->all_pieces[WHITE] & piecebb)
		return WHITE;
	return BLACK;
}


// TODO: Test the eligibility of this logic and the constants.
void performcastle(board_s* board, const unsigned int castle) {
	assert(popcount(castle) == 1); // ensure only 1 of the flags is set
	

	// Revoke castling sides castling perms
	if (castle & (WKCASTLE | WQCASTLE))
		board->castling &= ~(WKCASTLE | WQCASTLE);
	else
		board->castling &= ~(BKCASTLE | BQCASTLE);

	BitBoard king_from = 0x0;
	BitBoard king_to = 0x0;
	BitBoard rook_from = 0x0;
	BitBoard rook_to = 0x0;

	if (castle == WKCASTLE) {
		assert(board->pieces[WHITE][KING] == W_KING_DEFAULT_POS);
		assert(board->pieces[WHITE][ROOK] & H1); // make sure there exists a rook at H1
		
		king_from = W_KING_DEFAULT_POS;
		king_to = WK_CASTLE_KING_TARGET;

		rook_from = H1;
		rook_to = WK_CASTLE_ROOK_TARGET;
	}
	else if (castle == WQCASTLE) {
		assert(board->pieces[WHITE][KING] == W_KING_DEFAULT_POS);
		assert(board->pieces[WHITE][ROOK] & A1); // make sure there exists a rook at A1

		king_from = W_KING_DEFAULT_POS;
		king_to = WQ_CASTLE_KING_TARGET;

		rook_from = A1;
		rook_to = WQ_CASTLE_ROOK_TARGET;
	}
	else if (castle == BKCASTLE) {
		assert(board->pieces[BLACK][KING] == B_KING_DEFAULT_POS);
		assert(board->pieces[BLACK][ROOK] & H8); // make sure there exists a rook at H8
		
		king_from = B_KING_DEFAULT_POS;
		king_to = BK_CASTLE_KING_TARGET;

		rook_from = H8;
		rook_to = BK_CASTLE_ROOK_TARGET;
	}
	else { // BQCASTLE
		assert(board->pieces[BLACK][KING] == B_KING_DEFAULT_POS);
		assert(board->pieces[BLACK][ROOK] & A8); // make sure there exists a rook at H8
		
		king_from = B_KING_DEFAULT_POS;
		king_to = BQ_CASTLE_KING_TARGET;

		rook_from = A8;
		rook_to = BQ_CASTLE_ROOK_TARGET;
	}

	// Move rook and king
	movepiece(board, KING, king_from, king_to);
	movepiece(board, ROOK, rook_from, rook_to);
}


void set_move_history_size(board_s* board, const size_t size) {
	assert(size > 0);

	move_s* new_block = (move_s*)realloc(board->movehistory.moves, sizeof(move_s) * size);

	if (!new_block) {
		printf("No memory to realloc move history! Aborting... \n");
		abort();
	}

	board->movehistory.moves = new_block;
	board->movehistory.n = size;
}


void append_to_move_history(board_s* board, const move_s* move) {
	board->history_n++;

	if (board->movehistory.n < board->history_n) {
		set_move_history_size(board, board->history_n);
	}

	memcpy(&board->movehistory.moves[board->history_n-1], move, sizeof(move_s));
}


void free_move_history(const board_s* board) {
	free(board->movehistory.moves);
}


void restore_board(board_s* restrict to, board_s* restrict from) {
	movelist_s movehistory = to->movehistory;

	memcpy(to, from, sizeof (board_s)); // restore board
	to->movehistory = movehistory; // restore old movehistory
}


void write_move_history(const board_s* board, FILE* f) {
	for (unsigned int i = 0; i < board->history_n; i++) {
		char from[2];
		bbtoalg(from, board->movehistory.moves[i].from);

		char to[2];
		bbtoalg(to, board->movehistory.moves[i].to);

		char promote[2];
		if (board->movehistory.moves[i].flags & FLAG_PROMOTE) {
			promote[0] = piecetochar(board->movehistory.moves[i].promoteto);
			promote[1] = ' ';
		}
		else {
			promote[0] = ' ';
			promote[1] = '\0';
		}

		fprintf(f, "%c%c%c%c%s", from[0], from[1], to[0], to[1], promote);
	}
	fputs("\n", f);
}


#endif // BOARD_C
