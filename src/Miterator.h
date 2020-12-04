#include "Mvalue.h"

// MDH@03DEC2020: in order to iterate over array and list elements we define sequence as hosting two methods
//                the Next method
//                the iterator keeps track of the state of the iterator obviously
// as returned by iter_next() if index is 0 this means that it is not a valid sequence element (i.e. end reached)
typedef struct Msequenceelement{
    unsigned long long index;
    Mvalue* value;
}Msequenceelement;

typedef Msequenceelement (*Next)(void * const iterator);
typedef unsigned long long (*NextIndex)(void * const iterator);

typedef struct Miterator{
    Next next; // to be called to obtain the next element (if it exists)
    NextIndex nextindex; // to be called to check whether a next element exists
    unsigned long long index;
    unsigned long long lastindex;
    void** valueholder;
    Mvaluetype valuetype;
}Miterator;

// some convenience methods that do not require having to process the sequence element
// which we can forget about compiling if you do not want them
Mvalue* iter_next(Miterator const * const iterator);
unsigned long long iter_nextindex(Miterator const * const iterator);