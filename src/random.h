#ifndef RANDOM_H
#define RANDOM_H

#include <stdint.h>

/* initializes melg[NN] and lung with a seed */
void init_genrand64(uint64_t seed);

/* initialize by an array with array-length */
/* init_key is the array for initializing keys */
/* key_length is its length */
void init_by_array64(const uint64_t init_key[],
		     uint64_t key_length);

// Function to generate 64 bit pseudo-random numbers
extern uint64_t (*genrand64_int64)(void);

#endif // RANDOM_H
