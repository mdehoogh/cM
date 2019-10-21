#ifndef __MSTRING_H__
#define __MSTRING_H__

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#define BLOCK_SIZE 8

typedef struct{
    char* chars;
    size_t length;
    size_t blocks;
}Mstring;

void free_string(Mstring* str); // changed from string_dispose() to free_mstring() to be more compatible with the other free methods (see Mexecution.h/c)

// functions that create new string instances (and therefore start with _)
Mstring* __string();
Mstring* _getString(char const * const s); // convenient constructor
// copying
Mstring* _stringCopy(Mstring* const src,size_t length);

bool string_empty(Mstring const * const str);

size_t string_length(Mstring const * const str);
Mstring* string_setlength(Mstring * const str, size_t length); // MDH@26FEB2018: we might want to set the length (to a smaller one)

void string_synclength(Mstring * const str); // MDH@02JUN2019: check the length (if a \0 is in front of the current length)

char string_char(Mstring const * const str,size_t pos);
char string_last_char(Mstring const * const str);
char* string_remainder(Mstring * const str,size_t pos);
char* string(Mstring * const str);

char* _stringstart(Mstring const * const str,size_t length); // returns a copy of the first part of the string

bool string_shorten(Mstring * const str,size_t length);

long long string_find(Mstring const * const str,char c);

// changing the string
char string_removed_char(Mstring * const str,size_t pos);

size_t string_removed(Mstring * const str,size_t pos,size_t length); // MDH@03OCT2019: remove length characters from str starting at position pos

Mstring* string_insert_char(Mstring * const str,size_t pos,char c);
Mstring* string_append_char(Mstring * const str,char c);
Mstring* string_setchar(Mstring * const str,char c,size_t pos);

// MDH@26FEB2019: can we append a text as a whole???
Mstring* string_append(Mstring * const str,char const * const pc);
Mstring* string_prepend(Mstring * const str,char const * const pc);

void string_reverse(Mstring * const str);

Mstring* string_append_chars(Mstring * const str,char const * pc,size_t count); // MDH@24SEP2019: we need to be able to append count characters from pc
char string_replacedchar(Mstring * const str,char c,size_t pos); // returns the character at position pos replaced by c (but does not change the length ever)

size_t string_number_of_matching_chars(Mstring const * const str,char const * chars); // the number of matching character at the start

#endif /* __MSTRING_H__ */