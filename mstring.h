#ifndef __MSTRING_H__
#define __MSTRING_H__

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#define BLOCK_SIZE 8

typedef struct{
    char* chars;
    uint16_t length;
    uint16_t blocks;
}mstring;

mstring* string_create();

void free_mstring(mstring* str); // changed from string_dispose() to free_mstring() to be more compatible with the other free methods (see Mexecution.h/c)

bool string_empty(mstring* str);
uint16_t string_length(mstring* str);
bool string_setlength(mstring* str, uint16_t length); // MDH@26FEB2018: we might want to set the length (to a smaller one)

char string_char(mstring* str,uint16_t pos);
char string_last_char(mstring* str);

char* string_remainder(mstring* str,uint16_t pos);
char* string(mstring* str);

int16_t string_find(mstring* str,char c);

// changing the string
char string_removed_char(mstring* str,uint16_t pos);

mstring* string_insert_char(mstring* str,uint16_t pos,char c);
mstring* string_append_char(mstring* str,char c);

// MDH@26FEB2019: can we append a text as a whole???
mstring* string_append(mstring* str,const char* pc);

// copying 
bool string_copy(mstring* src,mstring* dst);

#endif /* __MSTRING_H__ */