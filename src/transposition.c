#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "transposition.h"

#include "lookup.h"

#include "defs.h"

// transposition tables will be split in this many "buckets"
#define N_BUCKETS 4


tt_s tt_normal; // transposition table for normal positions
tt_s tt_q; // transposition table for quiescense search


void allocate_table(tt_s* tt, size_t n);
void free_table(tt_s* tt);
static inline size_t get_entry_index(const tt_s* tt, uint64_t hash);
static inline void make_tt_entry(tt_entry_s* entry, uint64_t hash, eval_t eval, uint8_t search_depth, uint16_t move, uint8_t flags);


void allocate_table(tt_s* tt, size_t n) {
	assert(n >= N_BUCKETS); // minimum size

	memset(tt, 0, sizeof (tt_s));

	tt->n_buckets = N_BUCKETS;

	// make the number of entries be divisible by N_BUCKETS
	n -= n % N_BUCKETS;

	tt->n_entries = n / N_BUCKETS;

	//tt->entries = calloc(N_BUCKETS, sizeof (tt_entry_s*));
	tt->entries = malloc(N_BUCKETS * sizeof (tt_entry_s*));
	
	if (!tt->entries)
		goto ALLOCATE_TABLE_ALLOC_ERROR;

	
	for (size_t i = 0; i < N_BUCKETS; i++) {
		//tt->entries[i] = calloc(tt->n_entries, sizeof (tt_entry_s));
		tt->entries[i] = malloc(tt->n_entries * sizeof (tt_entry_s));
		if (!tt->entries[i])
			goto ALLOCATE_TABLE_ALLOC_ERROR;
	}

	return;

	ALLOCATE_TABLE_ALLOC_ERROR:
	printf("Error allocating memory for transposition table.\n");
	abort();
}


void free_table(tt_s* tt) {
	for (size_t i = 0; i < tt->n_buckets; i++)
		free(tt->entries[i]);
	free(tt->entries);
}


static inline size_t get_entry_index(const tt_s* tt, uint64_t hash) {
	return hash % tt->n_entries;
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

tt_entry_s* probe_table(const tt_s* tt, uint64_t hash) {

	const size_t index = get_entry_index(tt, hash);

	for (size_t i = 0; i < tt->n_buckets; i++) {
		
		const tt_entry_s* current_entry = &(tt->entries[i][index]);

		if (!current_entry->bestmove)
			continue; // entry didn't exist
		
		if (current_entry->hash != hash)
			continue; // entry didin't match
		
		return current_entry;
	}
	
	return NULL;
}

bool retrieve_entry(tt_s* restrict tt, tt_entry_s* restrict entry, uint64_t hash) {

	tt_entry_s* tt_entry = probe_table(tt, hash);

	if (!tt_entry)
		return false;

	memcpy(entry, tt_entry, sizeof (tt_entry_s));
	
	return true;
}

// TODO: Implement qsearch replacement strategy
void store_move(tt_s* tt, uint64_t hash, eval_t eval, uint8_t search_depth, uint16_t move, uint8_t flags, int ply) {
	assert(COMPACT_MOVE_FROM(move) < 64);
	assert(COMPACT_MOVE_TO(move) < 64);
	assert(eval <= MATE && eval >= -MATE);

	const size_t index = get_entry_index(tt, hash);

	// n of bucket where to store
	size_t bucket_n;

	// find first empty entry or the same hash
	bool need_to_replace = true;
	for (size_t i = 0; i < N_BUCKETS; i++) {
		if (tt->entries[i][index].hash != hash)
			continue;
		if (tt->entries[i][index].bestmove != 0x0)
			continue;
		
		need_to_replace = false;
		bucket_n = i;
		break;
	}

	// TODO: Make a replacement stratgy
	if (need_to_replace)
		bucket_n = tt->counter++ % N_BUCKETS; // Select a random entry to be replaced
	else {
		if (tt->entries[bucket_n][index].search_depth >= search_depth)
			return; // already had a node with bigger depth here
		if (!(flags & TT_ENTRY_FLAG_EXACT) && tt->entries[bucket_n][index].flags & TT_ENTRY_FLAG_EXACT)
			return; // already had a exact value
		if (!(flags & TT_ENTRY_FLAG_PV_NODE) && tt->entries[bucket_n][index].flags & TT_ENTRY_FLAG_PV_NODE)
			return; // already had a pv node here
	}

	// Make the "mate in n ply" score relative to this position in tree by "ply"
	
	// Ply had been added during discovery of mate at deeper
	// ply, now only difference between nodes exist

	if (EVAL_IS_WIN(eval))
		eval += ply;
	else if (EVAL_IS_LOSE(eval))
		eval -= ply;


	make_tt_entry(&(tt->entries[bucket_n][index]), hash, eval, search_depth, move, flags);
}

