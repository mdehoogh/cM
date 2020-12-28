#include "Miterator.h"

extern const unsigned long long M_MODULE_DEBUGGING;

// some convenience methods that do not require calling next yourself in essence hiding the Msequenceelement structure
// however, it is a nuisance that next should also return index?????? NO, this means you can use iterator->next() only
Mvalue* iter_next(Miterator const * const iterator){
    return(iterator?iterator->next(iterator).value:NULL);
}
unsigned long long iter_nextindex(Miterator const * const iterator){
    return(iterator?iterator->nextindex(iterator):0);
}