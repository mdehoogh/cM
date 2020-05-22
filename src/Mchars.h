/**
 * MDH@16APR2020: for storing character arrays that are dynamically allocated
 * 
 */
#include <string.h>

#include "Malloc.h"

typedef struct {
// #ifndef __PRODUCTION__
//     t_count allocationIndex;
// #endif
    char chars[1];
}Mchars;

Mchars* __chars(size_t size,long long count,char type);
Mchars* _resized(Mchars const * const _chars,size_t size,long long from_count,long long to_count,char type);
Mchars* free_chars(Mchars const * const _chars,size_t size,long long count,char type,Mallocationowner owner);

// get Mchars* that contains exactly the characters in chars (nothing more), and the size is 1 (for single characters)
Mchars* _getChars(char const * const chars);
void freeChars(Mchars const * const _chars,Mallocationowner owner);