
#ifndef ALGEBRAIC_H
#define ALGEBRAIC_H

#include "defs.h"

char piecetochar(const unsigned int piece);

// Converts and algabraic square notation to a bitboard
BitBoard algsqtobb(const char* alg);

// Converts and algabraic square notation to a square num
int algsqtoint(const char* alg);

void sqtoalg(char* alg, const unsigned int sq);

void bbtoalg(char* alg, const BitBoard bb);


#endif // ALGEBRAIC_H
