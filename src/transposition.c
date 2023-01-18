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



void allocate_table(tt_s* tt, size_t n) {
	assert(n >= N_BUCKETS); // minimum size

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


bool retrieve_entry(tt_s* tt, tt_entry_s* entry, uint64_t hash, bool qsearch) {

	tt_entry_s* best_entry = NULL;
	const size_t index = hash % tt->n_entries;

	for (size_t i = 0; i < tt->n_buckets; i++) {
		
		if (qsearch && best_entry)
			break; // get the first matching entry if qsearch
		
		tt_entry_s* current_entry = &(tt->entries[i][index]);

		if (!current_entry->depth)
			continue; // entry didn't exist
		
		if (current_entry->hash != hash)
			continue; // entry didin't match
		
		if (!best_entry) {
			best_entry = current_entry;
			continue;
		}
		
		if (current_entry->depth > best_entry->depth)
			best_entry = current_entry;
	}

	if (!best_entry)
		return false;
	
	memcpy(entry, best_entry, sizeof (tt_entry_s));
	return true;
}

// TODO: Implement qsearch replacement strategy
void store_move(tt_s* tt, uint64_t hash, uint8_t depth, uint8_t from, uint8_t to, uint8_t promoteto, bool qsearch) {
	
}

