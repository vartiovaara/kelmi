
#ifndef ALGEBRAIC_H
#define ALGEBRAIC_H

#include "defs.h"

// Converts and algabraic square notation to a bitboard
BitBoard algsqtobb(const char* alg);

// Converts and algabraic square notation to a square num
int algsqtoint(const char* alg);

#endif // ALGEBRAIC_H
