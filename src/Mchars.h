/**
 * MDH@16APR2020: for storing character arrays that are dynamically allocated
 * 
 */
#include <string.h>

#include "Malloc.h"

typedef struct {
#ifndef __PRODUCTION__
    t_count allocationIndex;
#endif
    char chars[];
}Mchars;

Mchars* __chars(size_t l);
bool free_chars(Mchars const * const _chars,size_t l);

// get Mchars* that contains exactly the characters in chars (nothing more)
Mchars* _getChars(char const * const chars);

Mchars* _resized(Mchars const * const _chars,size_t from_l,size_t to_l);
