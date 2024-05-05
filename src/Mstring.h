#ifndef __MSTRING_H__
#define __MSTRING_H__

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>

#include "Mchars.h"

// MDH@17APR2020: it's more convenient to define BLOCK_SIZE as the actual number of bytes we need for a specific number of characters
#define M_BLOCK_CHARACTERS 16
#define M_BLOCK_SIZE M_BLOCK_CHARACTERS*sizeof(char)

typedef struct{
// #ifndef __PRODUCTION__
//     t_count allocationIndex;
// #endif
    Mchars* _chars; // MDH@17APR2020: replacing char* chars by Mchars* _chars so we can keep track of where it is allocated
    size_t length;
    long long blocks; // the number of allocated blocks of BLOCK_SIZE bytes of memory for _chars
}Mstring;


Mstring* disowned_string(Mstring* str,Mallocationowner owner_str);
Mstring* owned_string(Mstring* str,Mallocationowner owner_str);

Mstring* __string();
Mstring* free_string(Mstring* str/*,Mallocationowner owner_str*/); // changed from string_dispose() to free_mstring() to be more compatible with the other free methods (see Mexecution.h/c)

// MDH@11JUN2020: a useful macro
#define FREE_STRING(str,owner_str) free_string(disowned_string(str,owner_str))

// functions that create new string instances (and therefore start with _)
Mstring* _getString(char const * const s); // convenient constructor
// copying
// WARNING: if length==0 the entire string is returned!!!!!!
Mstring* _stringCopy(Mstring * const src,size_t length);

bool string_empty(Mstring const * const str);

size_t string_length(Mstring const * const str);
Mstring* string_setlength(Mstring * const str, size_t length); // MDH@26FEB2018: we might want to set the length (to a smaller one)

Mstring* string_synclength(Mstring * const str); // MDH@02JUN2019: check the length (if a \0 is in front of the current length)

Mstring* string_declength(Mstring * const str); // MDH@13DEC2023: decrements the length of the string

char string_char(Mstring const * const str,size_t pos);
char string_last_char(Mstring const * const str);
// MDH@13OCT2020: string_last_char_count() returns the number of times str ends with c
size_t string_last_char_count(const Mstring* const str,char c);
char* string_remainder(Mstring * const str,size_t pos);
char* string(Mstring * const str);

char* _stringstart(Mstring const * const str,size_t length); // returns a copy of the first part of the string

bool string_shorten(Mstring * const str,size_t length);

long long string_find_char(Mstring const * const str,char c,size_t pos); // MDH@21OCT2020: renamed to string_find_char, and appending pos being the first position to consider, returning -1 on failure

// changing the string
char string_removed_char(Mstring * const str,size_t pos);

size_t string_removed(Mstring * const str,size_t pos,size_t length); // MDH@03OCT2019: remove length characters from str starting at position pos

Mstring* string_insert_char(Mstring * const str,size_t pos,char c);
Mstring* string_append_char(Mstring * const str,char c);
Mstring* string_setchar(Mstring * const str,char c,size_t pos);
Mstring* string_setchars(Mstring * const str,size_t pos,char const * const pc); // MDH@23APR2020: if we want to quickly replace a substring we can use string_setchars (does NOT change the length!!!)

// MDH@26FEB2019: can we append a text as a whole???
Mstring* string_append(Mstring * const str,char const * const pc);
Mstring* string_prepend(Mstring * const str,char const * const pc);

void string_reverse(Mstring * const str);

Mstring* string_append_chars(Mstring * const str,char const * pc,size_t count); // MDH@24SEP2019: we need to be able to append count characters from pc
char string_replacedchar(Mstring * const str,char c,size_t pos); // returns the character at position pos replaced by c (but does not change the length ever)

size_t string_number_of_matching_chars(Mstring const * const str,char const * chars); // the number of matching character at the start

bool string_equal(Mstring const * const str1,Mstring const * const str2); // MDH@24OCT2019: whether or not two strings are considered equal

// MDH@13MAR2020: helper function implementations now here (moved from Mexecution.h/c)
Mstring* string_append_ull(Mstring* const ms,unsigned long long ll);
Mstring* string_append_ll(Mstring* const ms,long long ll);
Mstring* string_append_ld(Mstring* const ms,long double ld);

Mstring* _string_info(Mstring* str);

size_t string_trailing(Mstring* str,char c);

bool string_endswith(Mstring const * const str,char const * const pc);

// MDH@05MAY2024: reading a line of characters directly into an Mstring from an open file (similar to what getline() would)
Mstring* string_freadline(Mstring * const str,FILE* const file);

#endif /* __MSTRING_H__ */