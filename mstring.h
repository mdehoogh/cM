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

void string_dispose(mstring* str);

bool string_empty(mstring* str);
uint16_t string_length(mstring* str);
char string_char(mstring* str,uint16_t pos);
char string_last_char(mstring* str);
char* string_remainder(mstring* str,uint16_t pos);
char* string(mstring* str);
int16_t string_find(mstring* str,char c);

// changing the string
char string_removed_char(mstring* str,uint16_t pos);
void string_insert_char(mstring* str,uint16_t pos,char c);
void string_append_char(mstring* str,char c);

// copying 
bool string_copy(mstring* src,mstring* dst);

#endif /* __MSTRING_H__ */