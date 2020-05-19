/**
 * implementation of Mchars.h
 */
#include <stdio.h>

#include "Mchars.h"

static int32_t const MODULE_ID=(4<<4);
static int32_t getOwnerId(uint16_t id){return(id>>12?0:(MODULE_ID<<12)+id);}

extern char const * const M_WARNING_PREFIX;

// I guess it's prudent to pass in how many initial characters you want to be able to store in the result
// NOTE this would mean that an external party should somehow keep track of the number of characters that can be stored in the character array
//      because this is a variable size dynamic memory type we need to use REALLOC
//      for actual use the number of blocks need to be defined
Mchars* __chars(size_t size,long long count,char type,int32_t oid){int32_t foid=(oid>0?oid:getOwnerId(1));
    // MDH@19MAY2020: it's a bit weird to pass the negative value of the owner id to REALLOC but this prevents us from having to call DISOWNED on the pointer returned by REALLOC
    //                this way we get a pointer that we know who created it by disowns it immediately
    Mchars* _chars=REALLOC(NULL,0,count,size,type,foid);
    return(oid>0?_chars:DISOWNED(_chars,sizeof(Mchars),foid));
}
// if you want to expand an Mchars by the number of characters should we return the new _chars or simply true or false??????
// ok, we're plugging in an Mchars pointer (which is the address of an Mchars structure)
Mchars* _resized(Mchars const * const _chars,size_t size,long long from_count,long long to_count,char type,int32_t oid){int32_t foid=(oid>0?oid:getOwnerId(1));
    // MDH@19MAY2020: NOTE that _chars needs to be DISOWNED by the owner in order for REALLOC to allow changing ownership
    return(_chars?REALLOC(_chars,from_count,to_count,size,type,foid):NULL);
}
Mchars* free_chars(Mchars const * const _chars,size_t size,long long count,char type,int32_t oid){int32_t foid=(oid>0?oid:getOwnerId(1));
    // typically the caller would need to tell us the current number of characters stored in _chars
    // ok, if we're freeing _chars we can pass any owner id into REALLOC but technically this means that REALLOC might fail, I suppose it makes sense than to return NULL on success and the original pointer on failure
    if(_chars)return REALLOC(_chars,count,0,size,type,foid);
    printf("%sNo Mchars to free.\n",M_WARNING_PREFIX);
    return NULL;
}

// if you want to host a known list of characters ('\0' delimited) call _getChars()
// you typically use _getChars() to store an immutable character array
// MDH@02MAY2020: always assuming _getChars() to b dynamic of type '\'' (single quote) with fixed size 1
// MDH@19MAY2020: if oid is positive, that's the owner id passed to _chars, otherwise I use my own as owner
Mchars* _getChars(char const * const chars,int32_t oid){int32_t foid=(oid>0?oid:getOwnerId(2));
    Mchars* _chars=NULL;
    if(chars){
        long long l=strlen(chars)+1;
        // output("Allocating %zd characters for storing '%s'.\n",l,chars);
        _chars=(chars?__chars(1,l,'\'',foid):NULL);
        if(_chars){
            memcpy(_chars->chars,chars,l);
            // if I'm the owner, I return a _chars disowned, otherwise I am returning as is because I never was the owner to start with
            return(oid>0?_chars:DISOWNED(_chars,l,foid));
        }
        outputError("Failed to store the character array");
    }
    return NULL;
}
// utility function to free an Mchars* created using _getChars
void freeChars(Mchars const * const _chars,int32_t oid){
    // if oid is not positive, assuming I was the owner to start with and use that as owner id
    free_chars(_chars,1,strlen(_chars->chars)+1,'\'',(oid>0?oid:getOwnerId(2)));
}
