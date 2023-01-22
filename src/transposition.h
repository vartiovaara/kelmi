#ifndef TRANSPOSITION_H
#define TRANSPOSITION_H

#include <stdint.h>

#include "defs.h"

extern tt_s tt_normal; // transposition table for normal positions
extern tt_s tt_q; // transposition table for quiescense search


// Allocates transposition table with n entries
extern void allocate_table(tt_s* tt, size_t n);

extern void free_table(tt_s* tt);

// Returns true if entry found. Copies the entry to entry if entry != NULL
extern bool retrieve_entry(tt_s* tt, tt_entry_s* entry, uint64_t hash);

extern void store_move(tt_s* tt, uint64_t hash, eval_t eval, uint8_t search_depth, uint8_t node_depth, uint8_t from, uint8_t to, uint8_t promoteto, bool qsearch);


#endif // TRANSPOSITION_H
