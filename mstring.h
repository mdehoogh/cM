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
}mstring;

mstring* string_create();

mstring* new_mstring(char* s); // convenient constructor

void free_mstring(mstring* str); // changed from string_dispose() to free_mstring() to be more compatible with the other free methods (see Mexecution.h/c)

bool string_empty(mstring* str);
uint32_t string_length(mstring* str);
mstring* string_setlength(mstring* str, uint32_t length); // MDH@26FEB2018: we might want to set the length (to a smaller one)
void string_synclength(mstring* str); // MDH@02JUN2019: check the length (if a \0 is in front of the current length)

char string_char(mstring* str,uint32_t pos);
char string_last_char(mstring* str);

char* string_remainder(mstring* str,uint32_t pos);
char* string(mstring* str);
bool string_shorten(mstring* str,uint32_t length);

int32_t string_find(mstring* str,char c);

// changing the string
char string_removed_char(mstring* str,uint32_t pos);

mstring* string_insert_char(mstring* str,uint32_t pos,char c);
mstring* string_append_char(mstring* str,char c);

// MDH@26FEB2019: can we append a text as a whole???
mstring* string_append(mstring* str,const char* pc);

void string_reverse(mstring* str);

// copying
mstring* string_copy(mstring* src);

#endif /* __MSTRING_H__ */