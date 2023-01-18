/*
Init stuff
*/

#include "init.h"

#include <string.h>

#include "defs.h"
#include "lookup.h"
#include "magicmoves/magicmoves.h"
#include "random.h"

const uint64_t seed[] = {'b', 'o', 'o', 'b', 's'};

int init_all() {
	init_by_array64(seed, LENGTH(seed));
	reset_lookups();
	compute_lookups();
	set_lookup_pointers();
	initmagicmoves();
	/*
	unsigned int i; unsigned long l; BitBoard ll;
	printf("uint: %llu %llu\nulong: %llu %llu\nullong: %llu %llu\n",
		(unsigned long long)sizeof i,
		(unsigned long long)UINT_MAX,
		(unsigned long long)sizeof l,
		(unsigned long long)ULONG_MAX,
		(unsigned long long)sizeof ll,
		(unsigned long long)ULLONG_MAX);*/
	return 0;
}
