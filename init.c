/*
Init stuff
*/

#ifndef INIT_C
#define INIT_C

#include <string.h>
//#include <stdio.h>

#include "defs.h"

uint64_t movelookup[N_NOSLIDE_PIECES][64];

int init_all() {
	reset_lookup();
	compute_king_lookup();
	compute_knight_lookup();
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

void reset_lookup() {
	memset(&movelookup, 0, sizeof movelookup);
}

void compute_king_lookup() {
	for (unsigned int i = 0; i < 64; i++) {
		const uint64_t pos = SQTOBB(i); // cuck thing to do
		 // north
		if (!(pos & TOP_MASK)) {
			movelookup[KING][i] |= pos<<8;
			if (!(pos & RIGHT_MASK)) // north-east
				movelookup[KING][i] |= pos << 9;
			if (!(pos & LEFT_MASK)) // north-west
				movelookup[KING][i] |= pos << 7;
		}
		// east
		if (!(pos & RIGHT_MASK))
			movelookup[KING][i] |= pos<<1;
		// south
		if (!(pos & BOTTOM_MASK)) {
			movelookup[KING][i] |= pos>>8;
			if(!(pos & RIGHT_MASK)) // south-east
				movelookup[KING][i] |= pos>>7;
			if(!(pos & LEFT_MASK)) // south-west
				movelookup[KING][i] |= pos>>9;
		}
		// west
		if (!(pos & LEFT_MASK))
			movelookup[KING][i] |= pos>>1;
	}
}

void compute_knight_lookup() {
	for (unsigned int i = 0; i < 64; i++) {
		const uint64_t pos = SQTOBB(i);

		// north-north
		if (!(pos & TOP_MASK_N)) {
			if (!(pos & RIGHT_MASK)) // east
				movelookup[KNIGHT][i] |= pos<<17;
			if (!(pos & LEFT_MASK)) // west
				movelookup[KNIGHT][i] |= pos<<15;
		}
		// east-east
		if (!(pos & RIGHT_MASK_N)) {
			if (!(pos & TOP_MASK)) // north
				movelookup[KNIGHT][i] |= pos<<10;
			if (!(pos & BOTTOM_MASK)) // south
				movelookup[KNIGHT][i] |= pos>>6;
		}
		// south-south
		if (!(pos & BOTTOM_MASK_N)) {
			if (!(pos & RIGHT_MASK)) // east
				movelookup[KNIGHT][i] |= pos>>15;
			if (!(pos & LEFT_MASK)) // west
				movelookup[KNIGHT][i] |= pos>>17;
		}
		// west-west
		if (!(pos & LEFT_MASK_N)) {
			if (!(pos & TOP_MASK)) // north
				movelookup[KNIGHT][i] |= pos<<6;
			if (!(pos & BOTTOM_MASK)) // south
				movelookup[KNIGHT][i] |= pos>>10;
		}
	}
}

#endif