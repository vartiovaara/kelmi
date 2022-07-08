#ifndef LOOKUP_H
#define LOOKUP_H

#include "defs.h"

// side can be anything when using anything other than pawn
BitBoard piecelookup(unsigned int pos, unsigned int piece, unsigned int side);

BitBoard rowlookup(unsigned int pos);

BitBoard columnlookup(unsigned int pos);

void reset_lookups();
void compute_lookups();
void set_lookup_pointers();


#endif // LOOKUP_H
