#include <stdlib.h>
#include <stdio.h>
#include "Mmemory.h"

char* heap_string_copy(char* _c){
    if(_c){
        size_t sc=sizeof(_c);
        if(sc){
            char* _hc=strcpy(malloc(sc),_c);
            if(_hc)return _hc;
            printf("\nERROR: Failed to make a dynamic copy of '%s'.",_c);
        }
    }
    return NULL;
}