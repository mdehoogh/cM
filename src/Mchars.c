/**
 * implementation of Mchars.h
 */
#include "Mchars.h"

// I guess it's prudent to pass in how many initial characters you want to be able to store in the result
// NOTE this would mean that an external party should somehow keep track of the number of characters that can be stored in the character array
Mchars* __chars(size_t size,long long count){
    // because this is a variable size dynamic memory type we need to use REALLOC
    // for actual use the number of blocks need to be defined
    return REALLOC(NULL,0,count,size,'s');
}
bool free_chars(Mchars const * const _chars,size_t size,long long count){
    // typically the caller would need to tell us the current number of characters stored in _chars
    return(!_chars||!REALLOC(_chars,count,0,size,'s'));
}

// if you want to host a known list of characters ('\0' delimited) call _getChars()
// you typically use _getChars() to store an immutable character array
Mchars* _getChars(char const * const chars){
    Mchars* _chars=NULL;
    if(chars){
        size_t l=strlen(chars)+1;
        _chars=(chars?__chars(1,l):NULL);
        if(_chars)memcpy(_chars->chars,chars,l);
    }
    return _chars;
}

// if you want to expand an Mchars by the number of characters should we return the new _chars or simply true or false??????
// ok, we're plugging in an Mchars pointer (which is the address of an Mchars structure)
Mchars* _resized(Mchars const * const _chars,size_t size,long long from_count,long long to_count){
    return(_chars?REALLOC(_chars,from_count,to_count,size,'s'):NULL);
}

