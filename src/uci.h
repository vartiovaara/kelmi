/*
 * Stuff about UCI integration.
 */

#ifndef UCI_H
#define UCI_H

#include <stdio.h>
#include <stdarg.h>

#include "defs.h"



// Searches for key from the end of s.
// returns -1 if key not found
// FIXME: Maybe works, maybe not. may not work with repeating keys
int search_end_of_string(char* s, char* key);

// functions for logging UCI sessions
size_t uci_read(FILE* f, char* input, size_t n);

void uci_write(FILE* f, const char* s, ...);

void uci(FILE* f);

size_t divide_string(char** fields, char* s, const char* delim);

#endif // UCI_H
