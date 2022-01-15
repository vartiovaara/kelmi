/*
Init stuff
*/

#ifndef INIT_C
#define INIT_C

#include <string.h>
//#include <stdio.h>

#include "defs.h"

int init_all() {
	reset_lookups();
	compute_lookups();
	set_lookup_pointers();
	/*
	unsigned int i; unsigned long l; uint64_t ll;
	printf("uint: %llu %llu\nulong: %llu %llu\nullong: %llu %llu\n",
		(unsigned long long)sizeof i,
		(unsigned long long)UINT_MAX,
		(unsigned long long)sizeof l,
		(unsigned long long)ULONG_MAX,
		(unsigned long long)sizeof ll,
		(unsigned long long)ULLONG_MAX);*/
	return 0;
}

#endif // INIT_C
