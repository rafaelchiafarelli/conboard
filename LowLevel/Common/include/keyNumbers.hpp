/* ANSI-C code produced by gperf version 3.1 */
/* Command-line: gperf -t -D -r -E --output-file=keyNumbers.hpp keyNumbers.hash  */
/* Computed positions: -k'1-3,$' */

#pragma once       
#include <assert.h>
#include <iostream>
#include <string.h>
#include <stdlib.h>


//#line 11 "keyNumbers.hash"
typedef struct { char const *name; unsigned char number;}namedKeyCodes;
namedKeyCodes *in_word_set ( const char *str,  size_t len);
unsigned int lhash ( const char *str,  size_t len);
/* maximum key range = 803, duplicates = 0 */

