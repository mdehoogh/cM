#include <stdlib.h>
#include <errno.h>
#include <stdio.h>
#include "Mmemory.h"

#include "Moutput.h"

/**
 * _strdup() adds writing a error message to strdup()
 */
char* _strdup(const char* const _c){
    if(_c){
        char* _hc=strdup(_c);
        if(_hc)return _hc;
        printf("\nERROR: Failed to make a dynamic copy of '%s'.",_c);
    }else
        printf("\nERROR: Nothing to copy!");
    return NULL;
}

// typically invalid should represent NaN (i.e. strtold("nan",NULL))
long long _strtoll(char* _c,long long invalid){
    size_t l=(_c?strlen(_c):0);
    if(!l)return invalid;
    char* eptr;
    long long ll=strtoll(_c,&eptr,0); // assume decimal (TODO allow other representations as well)
    if(!ll){
        if (errno==EINVAL){
            output("\nERROR: Conversion of '%s' to an integer failed.",_c);
            return invalid;
        }
        /* If the value provided was out of range, display a warning message */
        if (errno==ERANGE){
            output("\nERROR: The integer represented by  '%s' is out of range.",_c);
            return invalid;
        }
    }
    return ll;
}

long double _strtold(char* _c,long double invalid){
    size_t l=(_c?strlen(_c):0);
    if(!l)return invalid;
    char* end;
    long double ld=strtold(_c,&end);
    return(strlen(end)?invalid:ld);
}