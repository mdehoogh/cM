#include <stdlib.h>
#include <errno.h>
#include <stdio.h>

#include "Mmemory.h"

/**
 * _strdup() adds writing a error message to strdup()
 */
char* _strdup(const char* const _c){
    if(_c){
        /* something wrong with this replacement code:
        size_t l=strlen(_c)+1;
        char* _hc=MALLOC(l,sizeof(char),':'); // if MALLOC calls malloc it's size argument will be the product of l and sizeof(char)!!!!
        if(_hc)strcpy(_hc,_c);else printf("Failed to allocate memory to store '%s'.\n",_c);
        */
        ///* replacing:
        char* _hc=strdup(_c);
        //*/
        if(_hc)return _hc;
        printf("\nERROR: Failed to make a dynamic copy of '%s'.\n",_c);
    }else
        printf("No text to copy!\n");
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