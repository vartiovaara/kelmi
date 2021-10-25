/*
Stuff with algabreic moves.
*/

#include <stdint.h>

#include "defs.h"

// Converts and algabraic square notation to a bitboard
uint64_t algsqtobb(const char* alg) {
	return SQTOBB(algsqtoint(alg));
}

// Converts and algabraic square notation to a square num
int algsqtoint(const char* alg) {
	return (8*(alg[1]-'1')+(alg[0]-'a'));
}
