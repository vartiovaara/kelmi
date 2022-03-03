/*
Stuff with algabreic moves.
*/

#include <stdint.h>

#include "algebraic.h"

#include "defs.h"


BitBoard algsqtobb(const char* alg) {
	return SQTOBB(algsqtoint(alg));
}


int algsqtoint(const char* alg) {
	return (8*(alg[1]-'1')+(alg[0]-'a'));
}
