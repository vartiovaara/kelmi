#ifndef TRANSPOSITION_H
#define TRANSPOSITION_H

#include <stdint.h>

#include "defs.h"

extern tt_s tt_normal; // transposition table for normal positions
extern tt_s tt_q; // transposition table for quiescense search


// Allocates transposition table with n entries
extern void allocate_table(tt_s* tt, size_t n);

extern void free_table(tt_s* tt);

// Searches for entry by hash and returns a pointer to it.
// Returns NULL if entry not found.
extern tt_entry_s* probe_table(tt_s* tt, uint64_t hash);

// Returns true if entry found. Copies the entry to entry
extern bool retrieve_entry(tt_s* restrict tt, tt_entry_s* restrict entry, uint64_t hash);

extern void store_move(tt_s* tt, uint64_t hash, eval_t eval, uint64_t bestmove_hash, int16_t node_depth, uint16_t move, bool full_node, bool pv_node);


#endif // TRANSPOSITION_H
