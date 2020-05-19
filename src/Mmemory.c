#include <stdlib.h>
#include <errno.h>
#include <stdio.h>

#include "Mmemory.h"

static int32_t const MODULE_ID=(8<<4);
static int32_t getOwnerId(uint16_t id){return(id>>12?0:(MODULE_ID<<12)+id);}

extern char const * const M_ERROR_PREFIX;

/**
 * _strdup() adds writing a error message to strdup()
 */
char* _strdup(char const * const _c){
    if(_c){
        // MDH@07QAPR2020: strdup worked fine BUT I want to register the allocation of character arrays
        ///* something wrong with this replacement code:
        size_t l=strlen(_c)+1;
        // MDH@09APR2020: in order to be able to keep track of memory we should NOT use MALLOC here anymore because what we allocate 
        //                is of varying size which means that we should from now on use REALLOC to do so
        char* _hc=NULL;
        // MDH@14APR2020: here we have a problem in that allocationIndex is stored in the first t_count occypying bytes
        //                so we can't use REALLOC here to do the allocation which is a nuisance
        _hc=REALLOC(_hc,0,l,sizeof(char),'"');
        // replacing: char* _hc=MALLOC(l,1,'"'); // if MALLOC calls malloc it's size argument will be the product of l and sizeof(char)!!!!
        if(_hc)memcpy(_hc,_c,sizeof(char)*l);else output("%sFailed to allocate memory to store '%s'.\n",M_ERROR_PREFIX,_c);
        //*/
        /* replacing:
        char* _hc=strdup(_c);
        */
        if(_hc)return _hc;
        output("%sFailed to make a dynamic copy of '%s'.\n",M_ERROR_PREFIX,_c);
    }else
        output("No text to copy!\n");
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