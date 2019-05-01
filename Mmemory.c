#include <stdlib.h>
#include <stdio.h>
#include "Mmemory.h"

/**
 * _strdup() adds writing a error message to strdup()
 */
char* _strdup(const char* _c){
    if(_c){
        char* _hc=strdup(_c);
        if(_hc)return _hc;
        printf("\nERROR: Failed to make a dynamic copy of '%s'.",_c);
    }else
        printf("\nERROR: Nothing to copy!");
    return NULL;
}

long double _strtold(char* _c){char* end=_c;return strtold(_c,&end);}