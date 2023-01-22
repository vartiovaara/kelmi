#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "transposition.h"

#include "lookup.h"

#include "defs.h"

// transposition tables will be split in this many "buckets"
#define N_BUCKETS 8


tt_s tt_normal; // transposition table for normal positions
tt_s tt_q; // transposition table for quiescense search


void allocate_table(tt_s* tt, size_t n);
void free_table(tt_s* tt);
size_t get_entry_index(const tt_s* tt, uint64_t hash);
void make_tt_entry(tt_entry_s* entry, uint64_t hash, eval_t eval, uint8_t search_depth, uint8_t node_depth, uint8_t from, uint8_t to, uint8_t promoteto);


void allocate_table(tt_s* tt, size_t n) {
	assert(n >= N_BUCKETS); // minimum size

	memset(tt, 0, sizeof (tt_s));

	tt->n_buckets = N_BUCKETS;

	// make the number of entries be divisible by N_BUCKETS
	n -= n % N_BUCKETS;

	tt->n_entries = n / N_BUCKETS;

	tt->entries = calloc(N_BUCKETS, sizeof (tt_entry_s*));
	
	if (!tt->entries)
		goto ALLOCATE_TABLE_ALLOC_ERROR;

	
	for (size_t i = 0; i < N_BUCKETS; i++) {
		tt->entries[i] = calloc(tt->n_entries, sizeof (tt_entry_s));
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


size_t get_entry_index(const tt_s* tt, uint64_t hash) {
	return hash % tt->n_entries;
}


void make_tt_entry(tt_entry_s* entry, uint64_t hash, eval_t eval, uint8_t search_depth, uint8_t node_depth, uint8_t from, uint8_t to, uint8_t promoteto) {
	assert(hash);
	assert(from < 64);
	assert(to < 64);

	// FIXME
	entry->hash = hash;
	entry->eval = eval;
	entry->search_depth = search_depth;
	entry->node_depth = node_depth;
	entry->bestmove_from = from;
	entry->bestmove_to = to;
	entry->bestmove_promoteto = promoteto;
}


bool retrieve_entry(tt_s* restrict tt, tt_entry_s* restrict entry, uint64_t hash) {

	const size_t index = get_entry_index(tt, hash);

	for (size_t i = 0; i < tt->n_buckets; i++) {
		
		tt_entry_s* current_entry = &(tt->entries[i][index]);

		if (!current_entry->bestmove_from)
			continue; // entry didn't exist
		
		if (current_entry->hash != hash)
			continue; // entry didin't match
		
		if (entry)
			memcpy(entry, current_entry, sizeof (tt_entry_s));
		
		return true;
	}

	return false;
}

// TODO: Implement qsearch replacement strategy
void store_move(tt_s* tt, uint64_t hash, eval_t eval, uint8_t search_depth, uint8_t node_depth, uint8_t from, uint8_t to, uint8_t promoteto, bool qsearch) {
	assert(hash);
	assert(from < 64);
	assert(to < 64);

	tt_entry_s entry;
	memset(&entry, 0, sizeof (tt_entry_s));
	make_tt_entry(&entry, hash, eval, search_depth, node_depth, from, to, promoteto);

	size_t index = get_entry_index(tt, hash);

	// n of bucket where to replace
	// FIXME: Current is just basically random why what the fuck
	size_t bucket_n = tt->counter++ % N_BUCKETS;

	memcpy(&(tt->entries[bucket_n][index]), &entry, sizeof (tt_entry_s));
}

