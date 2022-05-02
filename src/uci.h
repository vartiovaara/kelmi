/*
 * Stuff about UCI integration.
 */

#ifndef UCI_H
#define UCI_H

#include <stdio.h>
#include <stdarg.h>

#include "defs.h"

// functions for logging UCI sessions
size_t uci_read(FILE* f, char* input, size_t n);

void uci_write(FILE* f, const char* s, ...);

void uci(FILE* f);

size_t divide_string(char** fields, char* s, const char* delim);

#endif // UCI_H
