#ifndef __MSTRING_H__
#define __MSTRING_H__

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#define BLOCK_SIZE 8

typedef struct{
    char* chars;
    uint32_t length;
    uint32_t blocks;
}Mstring;

void free_string(Mstring* str); // changed from string_dispose() to free_mstring() to be more compatible with the other free methods (see Mexecution.h/c)

// functions that create new string instances (and therefore start with _)
Mstring* __string();
Mstring* _getString(char* s); // convenient constructor
// copying
Mstring* _stringCopy(Mstring* src);

bool string_empty(Mstring* str);
uint32_t string_length(Mstring* str);
Mstring* string_setlength(Mstring* str, uint32_t length); // MDH@26FEB2018: we might want to set the length (to a smaller one)
void string_synclength(Mstring* str); // MDH@02JUN2019: check the length (if a \0 is in front of the current length)

char string_char(Mstring* str,uint32_t pos);
char string_last_char(Mstring* str);

char* string_remainder(Mstring* str,uint32_t pos);
char* string(Mstring* str);
bool string_shorten(Mstring* str,uint32_t length);

int32_t string_find(Mstring* str,char c);

// changing the string
char string_removed_char(Mstring* str,uint32_t pos);

Mstring* string_insert_char(Mstring* str,uint32_t pos,char c);
Mstring* string_append_char(Mstring* str,char c);

// MDH@26FEB2019: can we append a text as a whole???
Mstring* string_append(Mstring* str,const char* pc);

void string_reverse(Mstring* str);

#endif /* __MSTRING_H__ */