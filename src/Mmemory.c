#include <stdlib.h>
#include <errno.h>
#include <stdio.h>

#include "Mmemory.h"

extern unsigned long long M_MODULE_DEBUGGING;

static Mallocationowner getOwner(uint16_t id){return (Mallocationowner){MI_MEMORY,id};}

extern char const * const M_ERROR_PREFIX;

/**
 * _strdup() adds writing a error message to strdup()
 */
char* _strdup(char const * const _c){Mallocationowner owner=getOwner(__LINE__);
    char* _hc=NULL;
    if(_c){
        size_t l=strlen(_c)+1;
        _hc=MALLOC(sizeof(char),l,-'"',owner); // a single character
        // replacing: char* _hc=MALLOC(l,1,'"'); // if MALLOC calls malloc it's size argument will be the product of l and sizeof(char)!!!!
        if(_hc)memcpy(_hc,_c,sizeof(char)*l);else output("%sFailed to allocate memory to store '%s'.\n",M_ERROR_PREFIX,_c);
        //*/
        /* replacing:
        char* _hc=strdup(_c);
        */
    }else
        output("No text to copy!\n");
    return DISOWNED(_hc,owner);
}

// typically invalid should represent NaN (i.e. strtold("nan",NULL))
long long _strtoll(char* _c,long long invalid){
    size_t l=(_c?strlen(_c):0);
    if(!l)return invalid;
    char* eptr;
    long long ll=strtoll(_c,&eptr,0); // assume decimal (TODO allow other representations as well)
    if(!ll){
        if (errno==EINVAL){
            output("ERROR: Conversion of '%s' to an integer failed.",_c);
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