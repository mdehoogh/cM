/**
 * implementation of Mchars.h
 */
#include <stdio.h>

#include "Mchars.h"

extern char const * const M_WARNING_PREFIX;

// I guess it's prudent to pass in how many initial characters you want to be able to store in the result
// NOTE this would mean that an external party should somehow keep track of the number of characters that can be stored in the character array
//      because this is a variable size dynamic memory type we need to use REALLOC
//      for actual use the number of blocks need to be defined
Mchars* __chars(size_t size,long long count,char type){return REALLOC(NULL,0,count,size,type);}
// if you want to expand an Mchars by the number of characters should we return the new _chars or simply true or false??????
// ok, we're plugging in an Mchars pointer (which is the address of an Mchars structure)
Mchars* _resized(Mchars const * const _chars,size_t size,long long from_count,long long to_count,char type){
    return(_chars?REALLOC(_chars,from_count,to_count,size,type):NULL);
}
void free_chars(Mchars const * const _chars,size_t size,long long count,char type){
    // typically the caller would need to tell us the current number of characters stored in _chars
    if(!_chars)printf("%sNo Mchars to free.\n",M_WARNING_PREFIX);else REALLOC(_chars,count,0,size,type);
}

// if you want to host a known list of characters ('\0' delimited) call _getChars()
// you typically use _getChars() to store an immutable character array
// MDH@02MAY2020: always assuming _getChars() to b dynamic of type '\'' (single quote) with fixed size 1
Mchars* _getChars(char const * const chars){
    Mchars* _chars=NULL;
    if(chars){
        long long l=strlen(chars)+1;
        // output("Allocating %zd characters for storing '%s'.\n",l,chars);
        _chars=(chars?__chars(1,l,'\''):NULL);
        if(_chars)memcpy(_chars->chars,chars,l);else outputError("Failed to store the character array");
    }
    return _chars;
}
// utility function to free an Mchars* created using _getChars
void freeChars(Mchars const * const _chars){free_chars(_chars,1,strlen(_chars->chars)+1,'\'');}
