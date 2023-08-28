#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "transposition.h"

#include "lookup.h"

#include "defs.h"

// transposition tables will be split in this many "buckets"
#define BUCKET_SIZE 4

#define ALIGN_BOUNDARY (128)


tt_s tt_normal; // transposition table for normal positions


static inline tt_entry_s* get_bucket(const tt_s* tt, uint64_t hash);
static inline void make_tt_entry(tt_entry_s* entry, uint64_t hash, eval_t eval, uint8_t search_depth, uint16_t move, uint8_t flags);


void allocate_table(tt_s* tt, size_t n) {
	assert(n >= BUCKET_SIZE); // minimum size

	memset(tt, 0, sizeof (tt_s));

	// make the number of entries be divisible by BUCKET_SIZE
	n -= n % BUCKET_SIZE;

	tt->n_buckets = n / BUCKET_SIZE;

	tt->table_alloc = malloc((tt->n_buckets*BUCKET_SIZE) * sizeof (tt_entry_s) + ALIGN_BOUNDARY);
	
	tt->entries = (tt_entry_s*)(tt->table_alloc + (ALIGN_BOUNDARY - ((size_t)tt->table_alloc % ALIGN_BOUNDARY)));

	if (!tt->entries)
		goto ALLOCATE_TABLE_ALLOC_ERROR;

	if ((size_t)tt->entries % ALIGN_BOUNDARY != 0)
		goto ALLOCATE_TABLE_ALLOC_ERROR;


	return;

	ALLOCATE_TABLE_ALLOC_ERROR:
	printf("Error allocating memory for transposition table.\n");
	abort();
}


void free_table(tt_s* tt) {
	free(tt->table_alloc);
}


static inline tt_entry_s* get_bucket(const tt_s* tt, uint64_t hash) {
	return tt->entries + ((hash % tt->n_buckets) * BUCKET_SIZE);
}


static inline void make_tt_entry(tt_entry_s* entry, uint64_t hash, eval_t eval, uint8_t search_depth, uint16_t move, uint8_t flags) {
	assert(COMPACT_MOVE_FROM(move) < 64);
	assert(COMPACT_MOVE_TO(move) < 64);

	entry->hash = hash;
	entry->eval = eval;
	entry->search_depth = search_depth;
	entry->bestmove = move;
	entry->flags = flags;
}

const tt_entry_s* probe_table(const tt_s* tt, uint64_t hash) {

	const tt_entry_s* bucket = get_bucket(tt, hash);

	for (size_t i = 0; i < BUCKET_SIZE; i++) {
		
		if (bucket[i].hash == hash)
			return &bucket[i];
		
	}
	
	return NULL;
}

// TODO: Implement qsearch replacement strategy
void store_move(tt_s* tt, uint64_t hash, eval_t eval, uint8_t search_depth, uint16_t move, uint8_t flags, int ply) {
	assert(COMPACT_MOVE_FROM(move) < 64);
	assert(COMPACT_MOVE_TO(move) < 64);
	assert(eval <= MATE && eval >= -MATE);

	tt_entry_s* restrict bucket = get_bucket(tt, hash);

	// n of entry where to store
	size_t entry_n;

	// find first empty entry or the same hash
	bool need_to_replace = true;
	for (size_t i = 0; i < BUCKET_SIZE; i++) {
		if (bucket[i].hash != hash)
			continue;
		if (bucket[i].bestmove != 0x0)
			continue;
		
		need_to_replace = false;
		entry_n = i;
		break;
	}

	// TODO: Make a replacement stratgy
	if (need_to_replace)
		entry_n = tt->counter++ % BUCKET_SIZE; // Select a random entry to be replaced
	else {
		if (bucket[entry_n].search_depth >= search_depth)
			return; // already had a node with bigger depth here
		if (!(flags & TT_ENTRY_FLAG_EXACT) && bucket[entry_n].flags & TT_ENTRY_FLAG_EXACT)
			return; // already had a exact value
		if (!(flags & TT_ENTRY_FLAG_PV_NODE) && bucket[entry_n].flags & TT_ENTRY_FLAG_PV_NODE)
			return; // already had a pv node here
	}

	// Make the "mate in n ply" score relative to this position in tree by "ply"
	
	// Ply had been added during discovery of mate at deeper
	// ply, now only difference between nodes exist

	if (EVAL_IS_WIN(eval))
		eval += ply;
	else if (EVAL_IS_LOSE(eval))
		eval -= ply;


	make_tt_entry(&(bucket[entry_n]), hash, eval, search_depth, move, flags);
}

