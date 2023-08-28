#ifndef TRANSPOSITION_H
#define TRANSPOSITION_H

#include <stdint.h>

#include "defs.h"

extern tt_s tt_normal; // transposition table for normal positions


// Allocates transposition table with n entries
extern void allocate_table(tt_s* tt, size_t n);

extern void free_table(tt_s* tt);

// Searches for entry by hash and returns a pointer to it.
// Returns NULL if entry not found.
extern const tt_entry_s* probe_table(const tt_s* tt, uint64_t hash);

extern void store_move(tt_s* tt, uint64_t hash, eval_t eval, uint8_t search_depth, uint16_t move, uint8_t flags, int ply);


#endif // TRANSPOSITION_H
