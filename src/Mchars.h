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
	unsigned char chars[1];
}Mchars;

Mchars* disowned_chars(Mchars const * const _chars,Mallocationowner owner_chars);
Mchars* owned_chars(Mchars const * const _chars,Mallocationowner owner_chars);

Mchars* __chars(size_t size,long long count,signed char type);
void free_chars(Mchars const * const _chars/*,Mallocationowner owner_chars*/,size_t size,long long count,signed char type);
#define FREE_CHARS(_chars,size,count,type,owner_chars) free_chars(disowned_chars(_chars,owner_chars),size,count,type)

Mchars* _resized(Mchars const * const _chars,size_t size,long long from_count,long long to_count,signed char type);

// get Mchars* that contains exactly the characters in chars (nothing more), and the size is 1 (for single characters)
Mchars* _getChars(unsigned char const * const chars);
void freeChars(Mchars const * const _chars/*,Mallocationowner owner*/); // to free what was created with _getChars()

Mchars* _getReversedChars(unsigned char const * const chars);

#define FREECHARS(_chars,owner_chars) freeChars(disowned_chars(_chars,owner_chars))